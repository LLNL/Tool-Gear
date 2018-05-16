//! \file tracebackview.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** Widget TracebackView, which builds a source traceback viewer window on
** top of a TreeView window.
**
** Created by John Gyllenhaal 11/06/03
**
*****************************************************************************/
#include "tracebackview.h"
#include "qtooltip.h"
#include <qmainwindow.h>
#include <qregexp.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <tg_types.h>
#include "tg_time.h"
#include "tg_error.h"
#include "messagebuffer.h"

TracebackView::TracebackView(UIManager *m, CellGridSearcher * grid_searcher,
			     bool showTracebackTitle,
			     QWidget *parent,  const char *name) :
    QVBox (parent, name),
    
    // If not NULL, display extra information about contents (for now)
//    status (statusBar),
      // Assume top-level parent is a QMainWindow
    // StatusBar no longer used in message viewer -JCG
//    status( ((QMainWindow *)(topLevelWidget()))->statusBar() ),

    // Get the UIManager pointer
    um (m),

    showTitle(showTracebackTitle),

    // Initially the traceback string is empty
    tracebackString()
{

    // Set parent of traceback windows
    QWidget *tracebackParent = this;

    // If displaying traceback title, create GroupBox for title
    if (showTitle)
    {
	tracebackGroupBox = new QGroupBox (this, "Traceback");
	tracebackGroupBox->setColumns (1);
	tracebackGroupBox->setTitle ("Traceback:");
	tracebackGroupBox->setInsideMargin(3);

	// Make this the traceback window parent
	tracebackParent = tracebackGroupBox;
    }
    
#if 0
    // Create tab bar of possible traceback's
    tracebackTabBar = new QTabBar (this, "Traceback Tab Bar");
    tracebackTabBar->addTab (new QTab("Traceback"));
    tracebackTabBar->addTab (new QTab("Raw MpiP Data"));

    // Create tab widget to see what it looks like
    tracebackTabWidget = new QTabWidget (this, "Traceback Tab Widget");
#endif


    // Create pull down list of traceback lists declared
    // By making first argument FALSE, uses motif 2.0 style, which
    // left justifies the contents (and displays a different selection box)

    tracebackList = new QComboBox (FALSE, tracebackParent, "Traceback List");
    tracebackList->insertItem ("(No traceback location specified)", 0);

#if 0
    // DEBUG
    tracebackTabWidget->addTab(tracebackList, "Traceback List");
#endif


    // Create treeView used to look at source
//    tracebackView = new TreeView (progName, m, this, "Source Traceback",
//				  statusBar, viewingSnapshot);
    tracebackView = new CellGrid (2, 1, um->getMainFont(),
		    15, this, "Source Traceback");
    TG_checkAlloc(tracebackView);

    // Register the new cellgrid
    grid_searcher->addCellGrid( tracebackView );

    tracebackView->setHeaderText (0, "Source");

    // Don't have any tree hierarchy, so don't save space for markers
    tracebackView->setTreeAttrId (NULL_INT);

    // Try to hide the header in the source display
    tracebackView->setHeaderVisible (FALSE);

    // Initially no traceback is selected
    tracebackIndex = -1;

    // Initially, title not hidden in menu
    tracebackBase = 0;


    // Pick the highlight colors for selected lines
    selectedLineStyle =  tracebackView->newCellStyle ("Line Selected");
    QColor whiteColor("white");
    QColor blueColor("blue");
    tracebackView->setStyleFgColor (selectedLineStyle, whiteColor);
    tracebackView->setStyleBgColor (selectedLineStyle, blueColor);

    // no file and line currently being displayed
    displayedFileName = "" ;
    displayedLineNo = -1;

    // Initially no longest line, mark with -1
    maxSourceId = -1;
    maxSourceLen = -1;
    maxSourceWidth = -1;


    // Handle clicks to select traceback level
    connect (tracebackList, SIGNAL(activated (int)), 
	     this, SLOT(tracebackIndexSelectedHandler (int)));

    // Catch window resize events, so can adjust column width to
    // make everything look right
    connect (tracebackView, SIGNAL(gridResizeEvent(QResizeEvent *)),
	     this, SLOT(tracebackViewResized( QResizeEvent *)));

    connect (um, SIGNAL(clearedSourceCache()),
	     this, SLOT(reloadSource()));

    // Respond to a signal from the main view to change fonts
    connect( um, SIGNAL( mainFontChanged(QFont&) ),
		    this, SLOT( resetFont(QFont&) ) );

    // Set size policy so that the treeview will try to expand
    // to fill all available space (versus grabbing half of it)
    setSizePolicy (QSizePolicy(QSizePolicy::Expanding, 
			       QSizePolicy::Expanding));
}

TracebackView::~TracebackView()
{
	// No need to delete these, sinc they are children of this widget
//    delete tracebackList;
//    delete tracebackGroupBox;
//    delete tracebackView;

    // DEBUG, notifiy when being deleted 
//    TG_timestamp("TracbackView::~TracebackView() called for %p\n", this);
}

// Set the traceback to display and index in the traceback to show
// (level -1, the default, picks the highest priority level, 
//  and in the case of ties, picks the lowest level number (i.e., the 
//  top level).
// traceback should be of the form 
// "<level0Func:line_no0>func_name0\n<level1Func:line_no1>func_name1\n..."
void TracebackView::setTraceback (const QString &traceback, int index)
{
    // Record the trace string (modified below)
    tracebackString = traceback;

    // Confusing, give tool maker what they specify
#if 0
    // Remove any trailing newlines or spaces from the traceback passed in
    int len = tracebackString.length();
    while ((len > 0) && tracebackString[len-1].isSpace())
    {
	len--;
    }
    tracebackString.truncate(len);
#endif

    // Remove all existing items from the traceback list
    tracebackList->clear();

    QString fileName, funcName, lineBuf, title;
    bool titleSpecifier;
    int lineNo, tracebackLevel, maxTracebackLevel;
    int tracebackId, maxTracebackId;
    int listIndex=0;


    // Initially, not hiding traceback title so no base offset for traceback
    tracebackBase = 0;

    // Add a traceback entry for every line in the traceback
    while (getTracebackInfo (listIndex, fileName, lineNo, funcName, 
			     tracebackId, maxTracebackId,
			     tracebackLevel, maxTracebackLevel,
			     title, titleSpecifier))
    {
	bool haveFunc = !funcName.isEmpty();
	bool haveLine = (lineNo >= 0);
	bool haveFile = !fileName.isEmpty();

	// If maxTracebackLevel greater than 1, indicate trace depth
	// in printout, other don't by making depthTag empty string.
	char depthTag[50];
	if (maxTracebackLevel <= 1)
	    sprintf (depthTag, " ");
	else
	    sprintf (depthTag, " [%i]  ", tracebackLevel);

	// If have only one traceback level and if listIndex 0 is a title,
	// hide title from menu 
	if ((listIndex == 0) && (maxTracebackId == 1) && titleSpecifier)
	{
	    // Hide menu by subtracting one from each menu item index
	    tracebackBase = 1;

	    // Goto next listIndex
	    listIndex ++;

	    // Skip the rest of the loop
	    continue;
	}

	// Handle if this is a title specifier
	if (titleSpecifier)
	{
	    lineBuf.sprintf ("______ %s ______", title.latin1());
	}
	// Handle when we have all the info
	else if (haveFunc && haveLine && haveFile)
	{
	    lineBuf.sprintf ("%s%s:%i (%s)", depthTag, 
			     fileName.latin1(), lineNo, funcName.latin1());
	}

	// Handle some common subsets
	else if (haveLine && haveFile)
	{
	    lineBuf.sprintf ("%s%s:%i", depthTag, fileName.latin1(), lineNo);
	}
	else if (haveFunc)
	{
	    lineBuf.sprintf ("%s%s", depthTag, funcName.latin1());
	}
	else
	{
	    lineBuf.sprintf ("%s(No traceback specified)", 
			     depthTag);
	}
	
	// DEBUG
//	fprintf (stderr, "Inserting traceback '%s'\n", lineBuf.latin1());

	// Put string in traceback list at this listIndex (tracebackBase may
	// be 1, to effectively hide the list title)
	tracebackList->insertItem (lineBuf, listIndex-tracebackBase);
	
	// Goto next listIndex
	listIndex ++;
    }

    // Reset current index to invalid value
    tracebackIndex = -1;
	   
    // Set tracebackIndex and highlight text for index
    tracebackIndexSelectedHandler (index);
}

// Returns the number of levels specified by the traceback string
int TracebackView::getLevelCount ()
{
    // If the tracebackString is Empty or NULL, return 0 now
    if (tracebackString.isEmpty())
	return (0);

    // Get the start of the traceback string
    const char *startPtr = tracebackString.latin1();

    // Count the newlines, plus the last line (if doesn't have newline)
    int levels = 0;

    while (1)
    {
	// Advance pointer to next newline (or end of string)
	while ((*startPtr != 0) && (*startPtr != '\n'))
	    startPtr++;

	// Advance past newline, if exists (and count it)
	if (*startPtr == '\n')
	{
	    levels++;
	    startPtr++;

	    // If end of string just after newline, end now but
	    // don't count extra line
	    if (*startPtr == 0)
		break;
	}

	// Hit end of string, stop but count one more line
	if (*startPtr == 0)
	{
	    levels++;
	    break;
	}
    }

    // Return the number of non-empty lines counted
    return (levels);
}

// Returns the location info for 'index' in the traceback string 
// (via parameters) and TRUE if info for 'index' exists, FALSE otherwise.
// Note: With tracebackTitle specifiers and multiple tracebacks in a 
// string that 'index' is typically not the same as 'tracebackLevel',
// so made 'tracebackLevel' a return parameter and renamed variables.
// To support informational messages, now return maxTracebackLevel also.
bool TracebackView::getTracebackInfo (int index, QString &fileName, 
				      int &lineNo, QString &funcName, 
				      int &tracebackId, 
				      int &maxTracebackId,
				      int &tracebackLevel,
				      int &maxTracebackLevel,
				      QString &tracebackTitle,
				      bool &titleSpecifier)
{
    // Initialize to no-info specified 
    fileName = NULL_QSTRING;
    funcName = NULL_QSTRING;
    tracebackTitle = NULL_QSTRING;
    lineNo = -1;
    tracebackId = -1;
    maxTracebackId = -1;
    tracebackLevel = -1;
    maxTracebackLevel = 0;  // Return 0 for poorly formed traceback strings
    titleSpecifier = FALSE;

    // Be sure we have a valid traceback string
    if (tracebackString.isNull()) return (FALSE);

    // Get the start of the traceback string
    const char *startPtr = tracebackString.latin1();

    // Make tracebackLevel start at 1, reset at every title specifier
    tracebackLevel = 1;

    // Make tracebackId start at 1, increment at evey title specifier
    tracebackId = 1;

    // Advance thru (index-1) newlines to get to the correct index
    for (int curIndex=0; curIndex < index; curIndex++)
    {
	// If at end of string now, return FALSE, this index is not allowed
	if (*startPtr == 0)
	{
	    tracebackId = -11;
	    tracebackLevel = -11;
	    return (FALSE);
	}

	// Skip all leading whitespace except '\n'
	while ((*startPtr != 0) && (*startPtr != '\n') &&
	       isspace (*startPtr))
	    startPtr++;

	// Handle case where traceback Name is specified (starts with '^')
	if ((*startPtr == '^'))
	{
	    QString tracebackTitleBuf("");

	    // Skip ^ and copy rest of line for title
	    startPtr++;
	    
	    while((*startPtr != 0) && (*startPtr != '\n'))
	    {
		tracebackTitleBuf.append(*startPtr);
		startPtr++;
	    }

	    // Strip leading and trailing whitespace and assign to 
	    // tracebackTitle
	    tracebackTitle = tracebackTitleBuf.stripWhiteSpace();

	    // Reset traceback level back to 1 after increment below, so
	    // set to 0 here
	    tracebackLevel = 0;

	    // Increment tracebackId if not title for first traceback
	    if (curIndex != 0)
		tracebackId++;
	}

	// Advance pointer to next newline (or end of string)
	while ((*startPtr != 0) && (*startPtr != '\n'))
	    startPtr++;


	// Advance past newline, if exists
	if (*startPtr == '\n')
	{
	    startPtr++;
	}

	// Otherwise, return FALSE, index not valid
	else
	{
	    tracebackId = -11;
	    tracebackLevel = -11;
	    return (FALSE);
	}

	// Increment traceback level, next line should be at next level
	++tracebackLevel;
    }

    // Skip all leading whitespace except '\n'
    while ((*startPtr != 0) && (*startPtr != '\n') &&
	   isspace (*startPtr))
	startPtr++;
    
    // If at the end of the string, return TRUE, allow blank tracebacks
    // to be specified for the first traceback at each level
    if (*startPtr == 0)
    {
	maxTracebackId = tracebackId;
	maxTracebackLevel = tracebackLevel;  
	return (TRUE);
    }


    // Handle case where traceback Titl is specified at this index 
    // (That is, starts with '^')
    if ((*startPtr == '^'))
    {
	QString tracebackTitleBuf("");
	
	// Skip ^ and copy rest of line for title
	startPtr++;
	    
	while((*startPtr != 0) && (*startPtr != '\n'))
	{
	    tracebackTitleBuf.append(*startPtr);
	    startPtr++;
	}
	
	// Strip leading and trailing whitespace and assign to tracebackTitle
	tracebackTitle = tracebackTitleBuf.stripWhiteSpace();

	// Indicate this index specifies the title
	titleSpecifier = TRUE;

	// Set tracebackLevel to -1 to specify title not part of traceback
	tracebackLevel = -1;

	// Increment tracebackId if not title for first traceback
	// Use index here, not curIndex like above!
	if (index != 0)
	    tracebackId++;
    }

    // Otherwise, parse traceback info at the specified index
    else
    {
	// If have '<', have file and/or line info in format
	// "<file:line>func"
	if (*startPtr == '<')
	{
	    // Advance past '<'
	    startPtr++;
	    
	    // Skip all leading whitespace except '\n'
	    while ((*startPtr != 0) && (*startPtr != '\n') &&
		   isspace (*startPtr))
		startPtr++;
	    
	    // Get fileName unless have ':' or '>' now
	    if ((*startPtr != 0) && (*startPtr  != ':') && 
		(*startPtr != '>') && (*startPtr != '\n'))
	    {
		QString fileNameBuf("");
		
		while((*startPtr != 0) && (*startPtr  != ':') && 
		      (*startPtr != '>') && (*startPtr != '\n'))
		{
		    fileNameBuf.append(*startPtr);
		    startPtr++;
		}
		// Strip leading and trailing whitespace and assign to fileName
		fileName = fileNameBuf.stripWhiteSpace();
	    }
	    
	    // Get lineNo if we have : now
	    if (*startPtr == ':')
	    {
		// Advance past ':'
		startPtr++;
		
		QString lineNoBuf("");
		
		while((*startPtr != 0) && (*startPtr  != '>') && 
		      (*startPtr != '\n'))
	    {
		lineNoBuf.append(*startPtr);
		startPtr++;
	    }
		
		// Convert string to number
		bool ok;
		lineNo = lineNoBuf.toInt (&ok, 10);
		// If conversion failed, set to -1
		if (!ok)
		    lineNo = -1;
	    }	

	    // If have '>', skip it now
	    if (*startPtr == '>')
		startPtr++;
	}
	
	// Skip all leading whitespace except '\n'
	while ((*startPtr != 0) && (*startPtr != '\n') &&
	       isspace (*startPtr))
	    startPtr++;
	
	// get function name if don't have '\n'
	if ((*startPtr != 0) && (*startPtr != '\n'))
	{
	    QString funcNameBuf("");
	    
	    // Allow anything (even : and >) in function name
	    while((*startPtr != 0) && (*startPtr != '\n'))
	    {
		funcNameBuf.append(*startPtr);
		startPtr++;
	    }
	    // Strip leading and trailing whitespace and assign to funcName
	    funcName = funcNameBuf.stripWhiteSpace();
	}
    }

    // Determine maxTracebackLevel, must be at least current level
    maxTracebackLevel = tracebackLevel;

    // Increment max level for each line until title specifier
    // or end of traceback
    while (*startPtr != 0)
    {
	// Count each level after this one, unless ^ after \n
	if (*startPtr == '\n')
	{
	    // Goto next character
	    startPtr++;

	    // Skip all leading whitespace except '\n'
	    while ((*startPtr != 0) && (*startPtr != '\n') &&
		   isspace (*startPtr))
		startPtr++;
	    
	    // Increment traceback level except when the next character
	    // is an ^, which means it starts a new traceback section
	    if (*startPtr != '^')
	    {
		maxTracebackLevel++;
	    }

	    // Otherwise, if a title specifier, stop
	    else
	    {
		break;
	    }
	}
	else
	{
	    // Goto next character
	    startPtr++;
	}
    }

   // Determine maxTracebackId, must be at least current id
    maxTracebackId = tracebackId;

    // If currently at ^, at start of title specifier, increment
    // maxTracebackId
    if (*startPtr == '^')
    {
	maxTracebackId++;
	startPtr++;
    }

    // Increment max id for each title specifier (^) encountered after
    // a newline (white space allowed) in the rest of the traceback string.   
    while (*startPtr != 0)
    {
	// Skip all characters until '\n' or end of string
	while ((*startPtr != 0) && (*startPtr != '\n'))
		startPtr++;

	// If at newline, goto next character
	if (*startPtr == '\n')
	    startPtr++;

	// Skip all leading whitespace except '\n'
	while ((*startPtr != 0) && (*startPtr != '\n') &&
	       isspace (*startPtr))
	    startPtr++;

	// If have title specifier,  increment maxTracebackId
	if (*startPtr == '^')
	    maxTracebackId++;
    }

    // If got here, no parse errors, return TRUE
    return (TRUE);
}

// Calculate the priority of this site location, initially based only on
// file name (probably will want more in future)
double TracebackView::calcLevelPriority (double basePriority, 
					 const QString &fileName,
					 int line,
					 const QString &desc)
{
    // Initially, no priority offset
    double priorityOffset = 0.0;

    // Convert line to string for match purposes
    QString lineText = "";
    if (line != NULL_INT)
	lineText.sprintf ("%i", line);

    int priorityCount = um->sitePriorityCount();
    
    for (int priorityIndex = 0; priorityIndex < priorityCount;
	 ++priorityIndex)
    {
	// Get contents of site priority filter
	QString priorityTag = um->sitePriorityAt(priorityIndex);

	QString fileRegExp = um->sitePriorityFile(priorityTag);
	QString descRegExp = um->sitePriorityDesc(priorityTag);
	QString lineRegExp = um->sitePriorityLine(priorityTag);

	// Assume priority modifier applies until proved otherwise
	bool match = TRUE;

	// Must have at least one match to actually apply
	int matchCount = 0;

	// Only test if regular expression specified
	if (fileRegExp != NULL_QSTRING)
	{
	    // If not match, set match to FALSE
	    if (fileName.find(QRegExp (fileRegExp, TRUE, FALSE), 0) == -1)
	    {
		match = FALSE;
	    }
	    else
	    {
		matchCount++;
	    }
	}

	// Only test if match still possible and if regular expression 
	// specified
	if (match && (descRegExp != NULL_QSTRING))
	{
	    // If not match, set match to FALSE
	    if (desc.find(QRegExp (descRegExp, TRUE, FALSE), 0) == -1)
	    {
		match = FALSE;
	    }
	    else
	    {
		matchCount++;
	    }
	}

	// Only test if match still possible and if regular expression 
	// specified
	if (match && (lineRegExp != NULL_QSTRING))
	{
	    // If not match, set match to FALSE
	    if (lineText.find(QRegExp (lineRegExp, TRUE, FALSE), 0) == -1)
	    {
		match = FALSE;
	    }
	    else
	    {
		matchCount++;
	    }
	}
	
	// If after all that have match, update priority offset
	if ((match) && (matchCount > 0))
	{
	    double modifier = um->sitePriorityModifier(priorityTag);
	    priorityOffset += modifier;
	}
    }

    // Return the base priority plus any offset we have calculated
    return (basePriority + priorityOffset);
}


// Handle selection of a new traceback source level to display
void TracebackView::tracebackIndexSelectedHandler (int levelIndex)
{
    // Hold's this level's info (no longer always correct from first if)
    QString fileName, funcName, lineBuf, title;
    bool titleSpecifier;
    int lineNo, tracebackLevel, maxTracebackLevel;
    int tracebackId, maxTracebackId;

    // Assume didn't adjust index
    bool adjustedIndex = FALSE;

    // if levelIndex == -1, and nothing is picked yet, 
    // auto pick the highest priority index from the list.
    if ((levelIndex == -1) && (tracebackIndex == -1))
    {
	// Initially, assume the first levelIndex will be picked
	levelIndex = 0;
	double maxPriority = -100000.0;  // Initialize for sanity only
	
	// Get the max traceback level and the info for the first item
	if (getTracebackInfo (0+tracebackBase, 
			      fileName, lineNo, funcName, 
			      tracebackId, maxTracebackId,
			      tracebackLevel, maxTracebackLevel,
			      title, titleSpecifier))
	{
	    // Use priority for index to initialize max Priority
	    maxPriority = calcLevelPriority (0.0, fileName, lineNo, funcName);

	    // Loop through the remaining levels, looking for higher priority
	    // levels
	    int maxScanLevel = maxTracebackLevel -1;
	    for (int scanLevel = 1; scanLevel <= maxScanLevel; ++scanLevel)
	    {
		// Get info for level we are scanning
		if (getTracebackInfo (scanLevel+tracebackBase, 
				      fileName, lineNo, funcName, 
				      tracebackId, maxTracebackId,
				      tracebackLevel, maxTracebackLevel,
				      title, titleSpecifier))
		{
		    double scanPriority = calcLevelPriority (0.0, fileName,
							     lineNo, funcName);

		    // On ties, use previous level
		    if (scanPriority > maxPriority)
		    {
			maxPriority = scanPriority;
			levelIndex = scanLevel;
		    }
		}
	    }
	}
    }
    
    // If have different level, adjust levelIndex if necessary to
    // make it not point at a titleSpecifier, if possible
    if (levelIndex != tracebackIndex)
    {
	// Get traceback info from level and traceback string
	// Need to add in tracebackBase to levelIndex when getting info
	// in order to handle hidden titles
	if (!getTracebackInfo (levelIndex+tracebackBase, 
			       fileName, lineNo, funcName, 
			       tracebackId, maxTracebackId,
			       tracebackLevel, maxTracebackLevel,
			       title, titleSpecifier))
	{
	    // No info means no titleSpecifier
	    titleSpecifier=FALSE;
	}

	// If title specifier, try to automatcally move to the line
	// above or below (as long as they are legal and not title specifiers)
	if (titleSpecifier)
	{
	    // If coming for the index immediately below, go up one evel
	    // unless at level 0.   This will allow us to us a scrollwheel
	    // to get past a title specifier to the traceback above the
	    // current one
	    if ((levelIndex == (tracebackIndex -1)) && (levelIndex > 0))
	    {
		// Goto previous index (so scrollwheel works)
		--levelIndex;

		// Mark that adjusted index
		adjustedIndex = TRUE;

		// Change tracebackList selection to adjusted Index
		tracebackList->setCurrentItem(levelIndex);
	    }
	    // Otherwise, goto next level unless it is invalid or
	    // another title specifier (only happens in poorly formed 
	    // traceback specifier strings)
	    else
	    {
		// Is next index valid
		// Need to add in tracebackBase to levelIndex when getting info
		// in order to handle hidden titles
		if (getTracebackInfo (levelIndex+1+tracebackBase, 
				      fileName, lineNo, 
				      funcName, tracebackId, maxTracebackId,
				      tracebackLevel, maxTracebackLevel,
				      title, titleSpecifier))
		{
		    // Yes it is valid.  Is it a non-title specifier?
		    if (!titleSpecifier)
		    {
			// Goto next index (so don't show menu)
			++levelIndex;
			
			// Mark that adjusted index
			adjustedIndex = TRUE;

			// Change tracebackList selection to adjusted Index
			tracebackList->setCurrentItem(levelIndex);
		    }
		}
	    }
	}
    }

    // Update info if a different level is selected (after adjusting for
    // title specifiers)
    if (levelIndex != tracebackIndex)
    {
	// Reread info if adjusted index since last read
	if (adjustedIndex)
	{
	    // Get traceback info from level and traceback string
	    // Need to add in tracebackBase to levelIndex when getting info
	    // in order to handle hidden titles
	    if (!getTracebackInfo (levelIndex+tracebackBase, 
				   fileName, lineNo, funcName, 
				   tracebackId, maxTracebackId,
				   tracebackLevel, maxTracebackLevel,
				   title, titleSpecifier))
	    {
		TG_error ("TracebackView::tracebackIndexSelectedHandler: "
			  "level %i invalid!\n", levelIndex);
	    }
	}

	// Record the new tracebackIndex
	tracebackIndex = levelIndex;
	
	// If tracebackList not currently set to levelIndex, set it
	// This makes reloadSource() work properly, since resetting
	// everything without user selecting a particular item
	if (levelIndex != tracebackList->currentItem())
	    tracebackList->setCurrentItem(levelIndex);


	// If no title, default to "Traceback"
	if (title.isEmpty())
	    title = "Traceback";


	// Make full title from traceback title and depth and traceId tags
	QString fullTitle, depthTag(""), tracebackTag("");
	
	// Add depth marker, if not 1 (that is, 0, or 2+)
	// 0 depth indicates bad traceback string, which I want to show
	if (maxTracebackLevel != 1)
	{
	    depthTag.sprintf ("  [%i deep]", maxTracebackLevel);
	}

	// Add traceback id, if have more than 1 tracebacks
	if (maxTracebackId > 1)
	{
	    tracebackTag.sprintf (" (%i of %i)", 
				  tracebackId, maxTracebackId);
	}
	
	// Create full title using possile empty depth and traceback tags
	fullTitle.sprintf ("%s%s%s", title.latin1(), tracebackTag.latin1(),
			   depthTag.latin1());
			   

	// If showing title and if title different the current title, 
	// update it 
	if (showTitle && (title != tracebackGroupBox->title()))
	{
	    tracebackGroupBox->setTitle (fullTitle);
	}

	// Update name of traceback view with title (for potential use by
	// search dialog)
	if (title != tracebackView->name())
	{
	    tracebackView->setName(title);
	}

	//
	// The traceback info should have been read in above, don't repeat
	//

#ifdef USE_ORIG_TOOL_TIP
	// Try adding tool tip with level info in it
	// I believe it looks better with a space at the end of each line
	QString levelTip;
	levelTip.sprintf ("File: %s \n"
			  "Line #: %i \n"
			  "Function: %s \n"
			  "Traceback Depth: %i ",
			  fileName.latin1(), 
			  lineNo,
			  funcName.latin1(), 
			  tracebackLevel);

	// Try to remove tip first, so it works even if tip is being
	// currently displayed (selecting traceback level with scroll wheel
	// caused add to fail, so old wrong info was presented)
	QToolTip::remove (tracebackList);
	QToolTip::add (tracebackList, levelTip);
#endif


	
	// HACK, need to get back to passed in fileName.
	// May need to search for tag, or assume 0_0 for now!
	QString tempFileName;
//	tempFileName.sprintf("0_0 %s", fileName.latin1());

	// Show this the file and line on the screen, highlight source
//	tracebackView->showFileLine (tempFileName.latin1(), lineNo, 1);

	showFileLine (fileName.latin1(), lineNo);

#ifndef USE_ORIG_TOOL_TIP
	// This version is similar to the original tool tip code except
	// that it comes after the source file is displayed, so we know
	// that the file has been loaded if available.  This means
	// that the full path should also be available.  Also, we show
	// the full path in the tool tip instead of the regular file name.
	QString levelTip;
	QString fullPath = um->getSourcePath( fileName );
	// If fileName not specified, indicate that instead for full path
	if ( fileName.isEmpty() )
	    fullPath = "(not specified)";
	// We'll get a null result if the file wasn't loaded
	else if( fullPath.isNull() ) 
	    fullPath = "(not found)";

	    

	// Calculate the display priority for this site
	double displayPriority = 
	    calcLevelPriority (0.0, fileName, lineNo, funcName);

	levelTip.sprintf ("Description: %s \n"
			  "File location: %s \n"
			  "Line #: %i \n"
			  "Traceback Level: %i \n"
			  "Display Priority: %g ",
			  funcName.latin1(), fullPath.latin1(), lineNo,
			  tracebackLevel, 
			  displayPriority);
	// Try to remove tip first, so it works even if tip is being
	// currently displayed (selecting traceback level with scroll wheel
	// caused add to fail, so old wrong info was presented)
	QToolTip::remove (tracebackList);
	QToolTip::add (tracebackList, levelTip);
#endif
    }
}

// Display the indicated file and line number, highlighting the source line
void TracebackView::showFileLine (const char *fileName, int lineNo)
{
    // Undo last highlighting, if present
    if (displayedLineNo > 0)
    {
	tracebackView->setCellStyle (displayedLineNo-1, 0, CellStyleInvalid);

	// Mark it as cleared
	displayedLineNo = -1;
    }

    // Get last line for this file (may return 0, no source available)
    int lastLine = um->getSourceSize (fileName);

    // Determine how wide the source column needs to be to show everything

    // Handle case where no file specified
    if (fileName == NULL || fileName[0] == 0)
    {
	// Clear the existing grid contents
	tracebackView->clearGridContents();
	
	// Create four lines for our message
	tracebackView->resizeGrid(1, 1);
	tracebackView->setCellText (0, 0, "(No source location specified)");
	// Mark it as nothing displayed, since didn't find
	displayedFileName = "";

	// Mark that the width needs to be calculated
	maxSourceWidth = -1;

	// The first line is the longest line
	maxSourceId = 0;
	maxSourceLen = tracebackView->cellScreenText(0, 0).length();

	// Update the display width so all text is visible
	updateTracebackDisplayWidth();
    }

    // Handle case where cannot find file
    else if (lastLine <= 0)
    {
	// Clear the existing grid contents
	tracebackView->clearGridContents();
	
	// Create four lines for our message
	tracebackView->resizeGrid(4, 1);
	tracebackView->setCellText (0, 0, "Unable to locate the source file:");
	QString errorMsg;
	errorMsg.sprintf ("  '%s'", fileName);
	tracebackView->setCellText (1, 0, errorMsg);
	tracebackView->setCellText (2, 0, "");
#ifdef TG_MAC
	QString pathInstructions = 
	"The search path may be modified via 'Edit->Set Search Path...' or "
	"'Command-P'"; 
#else
	QString pathInstructions =
	"The search path may be modified via 'Edit->Set Search Path...' or "
        "'Ctrl-P'";
#endif
	tracebackView->setCellText (3, 0, pathInstructions);

	// Mark it as nothing displayed, since didn't find
	displayedFileName = "";

	// Mark that the width needs to be calculated
	maxSourceWidth = -1;

	// Initially no longest line
	maxSourceId = -1;
	maxSourceLen = -1;

	// Find the longest line of the message above
	for (int line = 0; line <= 3; line++)
	{
	    int len = tracebackView->cellScreenText(line, 0).length();

	    // If longest line, record length and recordId
	    if (len > maxSourceLen)
	    {
		maxSourceLen = len;
		maxSourceId = line;
	    }
	}

	// Update the display width so all text is visible
	updateTracebackDisplayWidth();
    }
    
    // Handle case where source exists
    else
    {
	// Display source, if not already displaying source for the file
	if (displayedFileName.compare(fileName) != 0)
	{
	    // Clear the existing grid contents
	    tracebackView->clearGridContents();

//	    TG_timestamp ("Resizing traceback grid start\n");
	    // Create one line per source line
	    tracebackView->resizeGrid(lastLine, 1);
//	    TG_timestamp ("Resizing traceback grid end\n");
	    
	    // Mark that the width will need to be update
	    maxSourceWidth = -1;

	    // Initialize maxSourceLen and maxSourceId to -1, updated below
	    maxSourceLen = -1;
	    maxSourceId = -1;

	    // DEBUG
	    maxSourceLen = 50;
	    maxSourceId = 1;

	    // Create a MessageBuffer to efficiently handle the sprintf
	    // to prepend line numbers.  QString.sprintf() was found to
	    // be a bottleneck for huge (90000 line) files. 
	    MessageBuffer lineBuf;

	    // Temp buffer for soruce line
	    QString line;

//	    TG_timestamp ("Writing in source for %s start\n", fileName);
	    // Loop through source, displaying it
	    for (int lineNum = 1; lineNum <= lastLine; lineNum ++)
	    {
		// Get the source line text
		line = um->getSourceLine (fileName, lineNum);

		// Prepend line number, getting output length
		// Must make prepending size a multiple of tab size (i.e. 8)
		int len = lineBuf.sprintf ("%6d: %s", lineNum, line.latin1());

		// Display it in the only column
		tracebackView->setCellText (lineNum-1, 0, lineBuf.contents());

		// If longest line, record length and recordId
		if (len > maxSourceLen)
		{
		    maxSourceLen = len;
		    maxSourceId = lineNum-1;
		}
	    }
//	    TG_timestamp ("Writing in source for %s end\n", fileName);
	}

	// Highline the source line specified
	tracebackView->setCellStyle (lineNo-1, 0, selectedLineStyle);

	// Update the display width so all text is visible
	updateTracebackDisplayWidth();

	// Scroll the cell with the lineNo into view
	tracebackView->centerRecordIdInView (lineNo-1);

	// Indicate what fileName and line we are displaying
	displayedFileName = fileName;
	displayedLineNo = lineNo;
    }
}

// Update the display width so all text is visible
void TracebackView::updateTracebackDisplayWidth()
{
    // If displayedMaxWidth is -1, we need to recalculate it from
    // the longest source line.  Calculating the line
    // length in pixels is very expensive, so just do it on the
    // longest line (works correctly only with fixed fonts, but the
    // message display looks so bad with proportional font, that only 
    // a fixed font should be used!).  Taking the width of 1500 lines
    // was taking .5 seconds, which just doesn't scale!  -JCG 4/5/04
    if (maxSourceWidth == -1)
    {
	// Use a minimim of 50 pixels width
	maxSourceWidth = 50;
	
	// Update width with maxSourceId, if not -1
	if (maxSourceId != -1)
	{
	    // Get the width, in pixels, of the longest source line
	    int lineWidth = tracebackView->cellWidth(maxSourceId, 0);

	    // Update displayedMaxWidth, if bigger
	    if (lineWidth > maxSourceWidth)
		maxSourceWidth = lineWidth;
	}
    }

    // Get the visible width of the tracebackView window used to display 
    // the message (different than width, since scrollbars change)
    int winWidth = tracebackView->visibleWidth();

    // Set new width to at least the visible window width, so
    // the lightlight goes across the entire window.
    int newWidth = winWidth;

    // If the displayMaxWidth is bigger (contents is bigger), use that
    // width, so entire message can be seen (using scrollbars)
    if (maxSourceWidth > newWidth)
    {
	newWidth = maxSourceWidth;
	
	// Enable scrollbars, probably are necessary now
	tracebackView->setHScrollBarMode (QScrollView::Auto);
    }
    // Otherwise, disable scrollbars, they behave poorly when the
    // window is resized.
    else
    {
	// Disable scrollbars, not needed and makes resizes ugly
	tracebackView->setHScrollBarMode (QScrollView::AlwaysOff);
    }

    // DEBUG
#if 0
    printf ("maxSourceWidth = %i, window width = %i, newWidth = %i, maxLine = %i\n", 
	   maxSourceWidth, winWidth, newWidth, maxSourceId+1);
#endif


    // Set the column width so the entire message is visible and 
    // the entire window is filled (if not already this width).
    int currentWidth = tracebackView->attrIdWidth (0);
    if (currentWidth != newWidth)
	tracebackView->setHeaderWidth (0, newWidth);
}

// Catch changes to the trackbackView display window size
void TracebackView::tracebackViewResized( QResizeEvent * /*e*/ )
{
    // Update the message display width, if necessary
    updateTracebackDisplayWidth();
}

// Catch changes to the search path and redisplay the traceback
void TracebackView::reloadSource()
{
    // Reload source by reissuing the setTraceback command with
    // the cached data
    setTraceback (tracebackString, tracebackIndex);
}

void TracebackView::resetFont( QFont& f )
{
	tracebackView->setFont( f );
	// Redraw the grid
	tracebackView->resizeGrid( tracebackView->recordIds(),
			tracebackView->attrIds() );
}
/******************************************************************************
COPYRIGHT AND LICENSE

Copyright (c) 2006, The Regents of the University of California.
Produced at the Lawrence Livermore National Laboratory
Written by John Gyllenhaal (gyllen@llnl.gov), John May (johnmay@llnl.gov),
and Martin Schulz (schulz6@llnl.gov).
UCRL-CODE-220834.
All rights reserved.

This file is part of Tool Gear.  For details, see www.llnl.gov/CASC/tool_gear.

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the disclaimer below.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the disclaimer (as noted below) in
  the documentation and/or other materials provided with the distribution.

* Neither the name of the UC/LLNL nor the names of its contributors may
  be used to endorse or promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OF THE UNIVERSITY 
OF CALIFORNIA, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ADDITIONAL BSD NOTICE

1. This notice is required to be provided under our contract with the 
   U.S. Department of Energy (DOE). This work was produced at the 
   University of California, Lawrence Livermore National Laboratory 
   under Contract No. W-7405-ENG-48 with the DOE.

2. Neither the United States Government nor the University of California 
   nor any of their employees, makes any warranty, express or implied, 
   or assumes any liability or responsibility for the accuracy, completeness,
   or usefulness of any information, apparatus, product, or process disclosed,
   or represents that its use would not infringe privately-owned rights.

3. Also, reference herein to any specific commercial products, process,
   or services by trade name, trademark, manufacturer or otherwise does not
   necessarily constitute or imply its endorsement, recommendation, or
   favoring by the United States Government or the University of California.
   The views and opinions of authors expressed herein do not necessarily
   state or reflect those of the United States Government or the University
   of California, and shall not be used for advertising or product
   endorsement purposes.
******************************************************************************/

