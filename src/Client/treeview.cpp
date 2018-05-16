// treeview.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** Widget TreeView, which builds a tree-based viewer window on
** top of a CellGrid window.
**
** Created by John Gyllenhaal 11/05/01
**
*****************************************************************************/
#include "treeview.h"
#include <qcursor.h>
#include <qmainwindow.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <tg_types.h>
#include <qmessagebox.h>
#include "tg_time.h"
using namespace std;

static const char * treeviewHourglassXpm[] = {
"11 15 4 1 XPMEXT",
"       c None",
".      c #000000000000",
"O      c #900090009000",
"W      c #FFFFFFFFFFFF",
" ......... ",
" ......... ",
"  .WWWWW.  ",
"  .WWWWW.  ",
"  .WWWWW.  ",
"  .OOOOO.  ",
"   .OOO.   ",
"    .O.    ",
"   .WOW.   ",
"  .WWWWW.  ",
"  .WWOWW.  ",
"  .WOOOW.  ",
"  .OOOOO.  ",
" ......... ",
" ......... ",
"EXPEXT TG_TEST",
"test string",
"XPMENDEXT"};

static const char * treeviewActionMenuXpm[] = {
"12 15 3 1",
"       c None",
".      c #000000000000",
"O      c #900090009000",
"            ",
" OO  OO  OO ",
" O        O ",
"            ",
" O        O ",
" O        O ",
"            ",
" O        O ",
" O        O ",
"            ",
" O        O ",
" O        O ",
"            ",
" O        O ",
" OO  OO  OO "};

static const char * treeviewHeaderSpacerXpm[] = {
"1 25 2 1",
"       c None",
".      c #000000000000",
".",
".",
".",
".",
".",
".",
".",
".",
".",
".",
" ",
" ",
".",
".",
".",
".",
".",
".",
".",
".",
".",
".",
".",
".",
"."};


TreeView::TreeView(const char *progName, UIManager *m, 
		   CellGridSearcher * grid_searcher, QWidget *parent,
		   const char *name, //QStatusBar *statusBar,
		   bool viewingSnapshot) : 
      QWidget (parent, name),

      // Assume top-level parent is a QMainWindow
      status( ((QMainWindow *)(topLevelWidget()))->statusBar() ),

      // Header spacer to allow two lines of text in header
      headerSpacer (QPixmap(treeviewHeaderSpacerXpm)),

      // Preallocate generic/common popup menu for treeview's use
      commonPopupMenu(this),

      // Preallocate specific popup actionMenu so can attach listener
      // to give help info on action items
      actionPopupMenu(this),

      // Look up all colors used here, requires talking to Xserver
      // which is expensive (so do only once per color)
      blueColor ("blue"),
      redColor ("red"),

      // Maps recordIds to MapInfo structures.  
      // Only mapping to MapInfo that should delete MapInfo structs on exit
      recordIdTable ("recordId", DeleteData, 0),

      // Maps fileIds to MapInfo.  Info deleted by recordIdTable!
      fileIdTable ("fileId", NoDealloc, 0),
      
      // Maps funcId to MapInfo.  Info deleted by recordIdTable!
      funcIdTable ("funcId", NoDealloc, 0),

      // Maps funcId+entryId to MapInfo.  Info deleted by recordIdTable!
      entryIdTable ("entryId", NoDealloc, 100),

      // Maps funcId+lineNo to MapInfo.  Info deleted by recordIdTable!
      lineNoTable ("lineNo", NoDealloc, 100),

      // Maps UIManager DataAttrIndex's to DataInfo structures, deletes on exit
      dataInfoTable ("DataInfo", DeleteData, 0),

      // Records which recordId tree nodes have been expanded
      expandedRecordId("expanded"),

      // Maps pixmapName to PixmapInfo structure, deletes PixmapInfo on exit
      pixmapInfoTable ("PixmapInfo", DeleteData, 0)
{
    // Get the UImanager pointer 
    um = m;

    // Initially, don't compare to another snapshot
    diffUm = NULL;

    // Get the program name
    programName = progName;

    // Are we just viewing a snapshot?
    snapshotView = viewingSnapshot;

    if (status != NULL)
    {
	statusStatType = new QLabel(status);
	statusStatType->setIndent(4);  // Put space before stat type
	status->addWidget (statusStatType, 0, TRUE);

	// These don't appear to do anything, why don't they?
	statusStatType->clear();
	statusStatType->setFrameStyle(QFrame::NoFrame|QFrame::Plain);

	statusLineNo = new QLabel(status);
	statusLineNo->setIndent(4);  // Put space before number
	status->addWidget (statusLineNo, 0, TRUE);
    }
    // If doesn't exist, flag that we don't want to display info
    else
    {
	statusLineNo = NULL;
	statusStatType = NULL;
    }
    statusLastRecordId = NULL_INT;
    statusLastAttrId = NULL_INT;
    statusLastHandle = NULL_INT;

#ifdef SEPARATE_TREE
    // Determine the attrId to assign each column
    lineAttrId = 0;
    treeAttrId = 1;
    sourceAttrId = 2;

    // Start data columns after source columns
    dataStartAttrId = 3;
#else
    // Determine the attrId to assign each column
    lineAttrId = 0;
    treeAttrId = 1;
    sourceAttrId = 1;

    // Start data columns after source columns
    dataStartAttrId = 2;
#endif

    // Until CellGrid allows attrIds to be added, need to
    // have all the data columsn up front
    int numAttrIds = dataStartAttrId + um->dataAttrCount();

    // Records pending dataCell updates (combine for scalability)
    cellUpdateTable = INT_ARRAY_new_symbol_table ("cellUpdateTable", 0);

    // Records pending function rollup updates (combine for scalability)
    funcUpdateTable = INT_ARRAY_new_symbol_table ("funcUpdateTable", 0);

    // Records pending file rollup updates (combine for scalability)
    fileUpdateTable = INT_ARRAY_new_symbol_table ("fileUpdateTable", 0);

    // Records pending app rollup updates (combine for scalability)
    appUpdateTable = INT_ARRAY_new_symbol_table ("appUpdateTable", 0);

    // Records pending entry insertions (combine for scalability)
    entryInsertTable = INT_ARRAY_new_symbol_table ("entryInsertTable", 0);

    // Create smallest grid, with all the attrIds we are going to get
    // Set min row height to 15, to make room for internal pixmaps
    grid = new CellGrid (0, numAttrIds, um->getMainFont(), 15, this, name);
    TG_checkAlloc(grid);

    // Register the new cellgrid
    grid_searcher->addCellGrid( grid );

    // Set fixed headers for grid, also pick widths for now
    grid->setHeaderText (lineAttrId, "Line(s)");
    grid->setHeaderWidth (lineAttrId, 70);
#ifdef SEPARATE_TREE
    grid->setHeaderText (treeAttrId, "Tree");
    grid->setHeaderWidth (treeAttrId, 60);
#endif
    grid->setHeaderText (sourceAttrId, "Source");
    grid->setHeaderWidth (sourceAttrId, 500);

    // Set data attribute headers to data attr's 'Text' 
    int numData = um->dataAttrCount();
    for (int dataIndex=0; dataIndex < numData; dataIndex++)
    {
	// Set the column header and width
	QString dataAttrTag = um->dataAttrAt(dataIndex);
	int dataAttrId = dataStartAttrId + dataIndex;
	grid->setHeaderText (dataAttrId, um->dataAttrText(dataAttrTag));
	grid->setHeaderWidth (dataAttrId, 90);

	// Get the tool designer suggested stats for the initial
	// AttrStat and EntryStat state
	UIManager::AttrStat suggestedAttrStat = 
	    um->dataAttrSuggestedAttrStat (dataAttrTag);
	UIManager::EntryStat suggestedEntryStat = 
	    um->dataAttrSuggestedEntryStat (dataAttrTag);

	// Create display info for data, using suggested stats
	DataInfo *dataInfo = new DataInfo (suggestedAttrStat,
					   suggestedEntryStat);
	dataInfoTable.addEntry (dataIndex, dataInfo);
    }

    // Set grid's tree marker to treeAttrId
    grid->setTreeAttrId (treeAttrId);

    // Allocate a hourglass pixmap to tell user when busy parsing
    hourglassPixmap = grid->newGridPixmap(QPixmap(treeviewHourglassXpm),
					  "TreeView::hourglass");

    // Pick (hopefully) non-conflicting handles.  Believe using
    // handles >= 0 for action menus, so pick distinct negative
    // handles for standard annotations
    hourglassHandle = -1;
    dataMenuHandle = -2;

    // Allocate a actionMenu pixmap to tell user when there is
    // something that can be done at an action point
    actionMenuPixmap = grid->newGridPixmap(QPixmap(treeviewActionMenuXpm),
					  "TreeView::actionMenu");


    // Tell grid about all the current tool pixmap declared
    int pixmapCount = um->pixmapCount();
    for (int pixmapIndex = 0; pixmapIndex < pixmapCount; pixmapIndex ++)
    {
	// Get name of declared pixmap
	QString pixmapName = um->pixmapAt (pixmapIndex);

	// Get char * version of name and of Xpm, for consistency
	const char *pixmapNameAscii = pixmapName.latin1();
	const char **pixmapXpm = um->pixmapXpm (pixmapNameAscii);

	// Call the pixmapDeclaredHandler for all these pixmaps
	pixmapDeclaredHandler (pixmapNameAscii, pixmapXpm);
    }

    // Denote no action popup menu in play
    actionMenuRecordId = -1;
    actionMenuAttrId = -1;
    actionMenuEntryId = -1;

    // Denote no data popup menu in play
    dataMenuDataIndex = -1;

    // Set of various styles we are using in grid
    programStyle = grid->newCellStyle("Program");
    grid->setStyleFgColor(programStyle, blueColor);
    fileStyle = grid->newCellStyle("File");;
    grid->setStyleFgColor(fileStyle, blueColor);
    funcStyle = grid->newCellStyle("Function");;
    grid->setStyleFgColor(funcStyle, blueColor);

    // For "diffed values", use red text
    diffedStyle = grid->newCellStyle("diffed values");
    grid->setStyleFgColor (diffedStyle, redColor);


    // The prefix delimiter is used to separate internal tags from external
    // names.  (I.e., ' ' is the delimiter for "0_2_1 main").
    // For simplicity, assume same delimiter for files and functions.
    // Evenually this will be specified in the UIManager
    // but will default to ' ' for now.
    prefixDelimiter = ' ';

    // Allocated recordId to hold the overall program name
    programRecordId = createRecordId();
    grid->setCellText (programRecordId, sourceAttrId, programName);
    grid->setCellStyle (programRecordId, sourceAttrId, programStyle);

    // Make this row expandable
    grid->setTreeExpandable (programRecordId, TRUE);

    // Set to opened state
    grid->setTreeOpen (programRecordId, TRUE);

    // Scan all data attributes for the application rollup
    for (int dataAttrIndex =0; dataAttrIndex < numData;
	 dataAttrIndex++)
    {
	QString dataTag = um->dataAttrAt(dataAttrIndex);
	updateAppRollup (dataTag);
    }


    // List all files and functions currently found for the program,
    // starting at the first ones
    lastFileIndexDisplayed = -1;
    lastFuncIndexDisplayed = -1;
    flushFileFunctionEntryUpdates();
    

    // Install our mouse click and movement handlers
    connect (grid, SIGNAL(cellAnnotMovedOver(int, int, ButtonState, int, int,
                                             int, int, int, int, int, int)),
             this, SLOT(annotMovedOverHandler(int, int, ButtonState, int, int,
                                              int, int, int, int, int, int)));

    connect (grid, SIGNAL(cellSlotMovedOver(int, int, ButtonState, int, int,
                                            int, int, int, int)),
             this, SLOT(slotMovedOverHandler(int, int, ButtonState, int, int,
                                             int, int, int, int)));

    connect (grid, SIGNAL(cellAnnotClicked(int, int, ButtonState, int, int,
                                           int, int, int, int, int, int)),
             this, SLOT(annotClickHandler(int, int, ButtonState, int, int,
					  int, int, int, int, int, int)));
    
    connect (grid, SIGNAL(cellTreeClicked(int, int, ButtonState, int, int,
                                          bool, int, int, int, int)),
             this, SLOT(treeClickHandler(int, int, ButtonState, int, int,
                                         bool, int, int, int, int)));
    connect (grid, SIGNAL (cellSlotClicked (int, int, ButtonState, int, int, 
					    int, int, int, int)),
	     this, SLOT(cellSlotClickHandler (int, int, ButtonState, int, int, 
					      int, int, int, int)));
    connect (grid, SIGNAL (emptyAreaClicked (int, int, ButtonState, 
						    int, int)),
	     this, SLOT(emptyAreaClickHandler (int, int, ButtonState, 
					       int, int)));


    connect (grid, SIGNAL(cellTreeMovedOver(int, int, ButtonState, int, int,
                                          bool, int, int, int, int)),
             this, SLOT(treeMovedOverHandler(int, int, ButtonState, int, int,
					     bool, int, int, int, int)));
    connect (grid, SIGNAL(emptyAreaMovedOver (int, int, ButtonState, 
					      int, int)),
             this, SLOT(emptyAreaMovedOverHandler (int, int, ButtonState, 
						   int, int)));


    // Install our UImanage data update handlers

    // Listens for file state changes (parsing, done parsing)
    connect( um, 
	     SIGNAL(fileStateChanged (const char *, UIManager::fileState)),
	     this, 
	     SLOT(fileStateChanged (const char *, UIManager::fileState)));


#if 0
    // List all newly inserted file names in display tree
    // Now done periodically -JCG 2/19/04
    connect( um, SIGNAL(fileInserted(const char *)),
	     this, SLOT(listFileName(const char *)));
#endif

    // Listens for function state changes (parsing, done parsing)
    connect( um, 
	     SIGNAL(functionStateChanged (const char *, 
					  UIManager::functionState)),
	     this,
	     SLOT(functionStateChanged (const char *, 
					UIManager::functionState)));

    // List all newly inserted function names in display tree
#if 0
    // Now done periodically -JCG 2/19/04
    connect( um, 
	     SIGNAL(functionInserted(const char *, const char *, int, int)),
	     this, 
	     SLOT(functionInserted(const char *, const char *, int, int)));
#endif

    connect(um, 
	    SIGNAL(entryInserted(const char *, const char *, int, 
				 TG_InstPtType, TG_InstPtLocation,
				 const char *, int, const char *)),
	    this,
	    SLOT(entryInserted(const char *, const char *, int, 
			       TG_InstPtType, TG_InstPtLocation,
			       const char *, int, const char *)));

    // Listens for action enable/disable/activate/deactivate
    connect (um, SIGNAL(actionEnabled(const char *, const char *, 
				      const char *)),
	     this, SLOT(actionStateChanged (const char *, const char *, 
					    const char *)));
    connect (um, SIGNAL(actionDisabled(const char *, const char *, 
				       const char *)),
	     this, SLOT(actionStateChanged (const char *, const char *, 
					    const char *)));
    connect (um, SIGNAL(actionActivated(const char *, const char *, 
					const char *)),
	     this, SLOT(actionStateChanged (const char *, const char *, 
					    const char *)));
    connect (um, SIGNAL(actionDeactivated(const char *, const char *, 
					  const char *)),
	     this, SLOT(actionStateChanged (const char *, const char *, 
					    const char *)));

    // Listens for action enable/disable/activate/deactivate
    // Control of particular tasks/threads not yet designed but
    // enable partical support for debugging reasons.
    connect (um, SIGNAL(actionActivated(const char *, const char *, 
					const char *, int, int)),
	     this, SLOT(actionStateChanged (const char *, const char *, 
					    const char *, int, int)));
    connect (um, SIGNAL(actionDeactivated(const char *, const char *, 
					  const char *, int, int)),
	     this, SLOT(actionStateChanged (const char *, const char *, 
					    const char *, int, int)));

    // Install our data update handlers
    connect(um, SIGNAL(intSet (const char *, const char *, const char *, 
			       int, int, int)),
   	    this, SLOT(intSet (const char *, const char *, const char *, 
			       int, int, int)));
    connect(um, SIGNAL(doubleSet (const char *, const char *, const char *, 
				  int, int, double)),
   	    this, SLOT(doubleSet (const char *, const char *, const char *, 
				  int, int, double)));

    // Listen for pixmap additions
    connect (um, SIGNAL (pixmapDeclared(const char *, const char **)),
	     this, SLOT (pixmapDeclaredHandler(const char *, const char **)));

    // Listen for actionPopupMenu highlights
    connect (&actionPopupMenu, SIGNAL (highlighted(int)), 
	     this, SLOT (actionMenuHighlightHandler(int)));
    connect (&actionPopupMenu, SIGNAL (aboutToHide()), 
	     this, SLOT (actionMenuClearStatus()));
    connect (&actionPopupMenu, SIGNAL (aboutToShow()), 
	     this, SLOT (actionMenuClearStatus()));

    // Create screen update timer
    updateTimer = new QTimer (this, "updateTimer");
    
    // flush pending updates every timer signal
    connect (updateTimer, SIGNAL(timeout()), 
	     this, SLOT(updateTimerHandler()));

    // Handle when paths to source code change
    connect (um, SIGNAL(clearedSourceCache()),
	     this, SLOT(reloadSource()));

    // Respond to a signal from the main view to change fonts
    connect( um, SIGNAL( mainFontChanged(QFont&) ),
		    this, SLOT( resetFont(QFont&) ) );

    // Set size policy so that the treeview will try to expand
    // to fill all available space (versus grabbing half of it)
    setSizePolicy (QSizePolicy(QSizePolicy::Expanding, 
			       QSizePolicy::Expanding));

    // For now, update the screen every 100ms
    updateTimer->start(100);
}


TreeView::~TreeView()
{
    // Need to figure out all the things to delete when done!
    // No need to delete grid, since it's a child of this widget
    // delete grid;
}

// Compare results to 'compareTo'.  Undo comparison by setting
// compareTo to NULL
void TreeView::compareTo(UIManager *compareTo)
{
    // Make sure not already comparing to this UIManager
    if (diffUm == compareTo)
	return;
    
    // Set the UI manager to compare result to
    diffUm = compareTo;

    // Update the grid for any data cell that had data in either um or diffUm
    // Since display will only show um contents, use it as list of
    // function count, etc. (i.e., diffUm may have more stuff, just ignore it)
    int numDataAttrs = um->dataAttrCount();

    // Scan all data attributes for the application rollup
    for (int dataAttrIndex =0; dataAttrIndex < numDataAttrs;
	 dataAttrIndex++)
    {
	QString dataTag = um->dataAttrAt(dataAttrIndex);
	updateAppRollup (dataTag);
    }


    // Update all the files in the grid
    // It may perform (cache wise) better if do this update in
    // hierarchy order (scan over files, over functions in each file,
    // over entries in each function).  For now, do all the file updates
    // up front.
    int numFiles = um->fileCount();
    for (int fileIndex = 0; fileIndex < numFiles; fileIndex++)
    {
	QString fileName = um->fileAt(fileIndex);

	// Scan all data attributes for the file rollup
	for (int dataAttrIndex =0; dataAttrIndex < numDataAttrs;
	     dataAttrIndex++)
	{
	    QString dataTag = um->dataAttrAt(dataAttrIndex);
	    updateFileRollup (fileName, dataTag);
	}
    }
    
    // Update all function and entry stats in the grid
    int numFuncs = um->functionCount();
    for (int funcIndex = 0; funcIndex < numFuncs; funcIndex++)
    {
        QString funcName = um->functionAt(funcIndex);

	// Scan all data attributes for the function rollup
	for (int dataAttrIndex =0; dataAttrIndex < numDataAttrs;
	     dataAttrIndex++)
	{
	    QString dataTag = um->dataAttrAt(dataAttrIndex);
	    updateFunctionRollup (funcName, dataTag);
	}
	
	// Scan all entries in each function
	int numEntries = um->entryCount (funcName);
	for (int entryIndex = 0; entryIndex < numEntries; entryIndex++)
	{
	    QString entryKey = um->entryKeyAt (funcName, entryIndex);

	    // Scan all data attributes for each entry
	    for (int dataAttrIndex =0; dataAttrIndex < numDataAttrs;
		 dataAttrIndex++)
	    {
		QString dataTag = um->dataAttrAt(dataAttrIndex);

		// Update dataCell only if data set for um or diffUm
		if ((um->entryDataStat (funcName, entryKey, dataTag,
					UIManager::EntryCount) > 0) || 
		    ((diffUm != NULL) &&
		     (diffUm->entryDataStat (funcName, entryKey, dataTag,
					     UIManager::EntryCount) > 0)))
		{
		    updateDataCell(funcName, entryKey, dataTag);
		}
		
		// Otherwise, make sure no contents in data cell
		else 
		{
		    resetDataCell(funcName, entryKey, dataTag);
		}
	    }
	}
    }
    
    // Make sure update takes right away
    grid->flushUpdate();
}

// Set program State message (NULL_QSTRING) for no message
void TreeView::setProgramState (QString stateMessage)
{
    printf ("In setProgramState with message '%s'\n", stateMessage.latin1());

    // Do nothing if already have this state message
    if (programStateMessage == stateMessage)
	return;

    // Change programName in display to reflect stateMessage
    if (stateMessage == NULL_QSTRING)
    {
	grid->setCellText (programRecordId, sourceAttrId, programName);
    }
    else
    {
	statusBuf.sprintf ("%s (%s)", programName.latin1(), 
			   stateMessage.latin1());
	grid->setCellText (programRecordId, sourceAttrId, statusBuf);
    }
}

// Override QWidget function to resize grid appropriately to fill
// entire window area.
void TreeView::resizeEvent (QResizeEvent *e )
{
    QSize size = e->size();
    grid->resize(size.width(), size.height());
}

// Internal routine to update stats display portion of status bar
void TreeView::updateStatusBarStats (int recordId, int attrId)
{
    // For simplicity, initially clear Stats messages
    // Will not actually be displayed until updateStatusMessage() is called.
    // For now, not displaying anything in lineMessage, so don't clear.
    setStatusMessage(mainMessage, dataDisplay, "");
    setStatusMessage(typeMessage, dataDisplay, "");

    // Do nothing, if attrId is not data column, do nothing else
    if (attrId < dataStartAttrId)
	return;

    // If cell has nothing in it, assume no stats to display for it
    if (grid->cellType (recordId, attrId) == CellGrid::invalidType)
	return;

    // Get mapInfo for this recordId,  
    MapInfo *mapInfo = recordIdTable.findEntry (recordId);
    
    // Determine which dataTagIndex to use
    int dataTagIndex = attrId - dataStartAttrId;
    
    // Get the dataTag for this Index
    QString dataTag = um->dataAttrAt (dataTagIndex);
    
    // Get dataInfo for this dataTagIndex
    DataInfo *dataInfo = dataInfoTable.findEntry (dataTagIndex);
    if (dataInfo == NULL)
    {
	TG_error ("TreeView::updateStatusBarStats:: "
		  "dataInfo not found!");
    }
    
    
    // If recordId == programRecordId, this is the application line
    if (recordId == programRecordId)
    {
	// Get all the stats for this application over the
	// specified entryStat (specified in dataInfo->entryStat);
	int entryCount = 
	    (int) um->applicationDataStat (dataTag, 
					   UIManager::AttrCount,
					   dataInfo->entryStat);
	
	QString minFuncName, minEntryKey;
	double minVal = 
	    um->applicationDataStat (dataTag, UIManager::AttrMin, 
				     dataInfo->entryStat, 
				     &minFuncName, 
				     &minEntryKey);
	int minLineNo = um->entryLine (minFuncName, minEntryKey);
	
	// Get minFileName from minFuncName
	QString minFileName = um->functionFileName (minFuncName);
	const char *cleanMinFileName = 
	    cleanName (minFileName.latin1());
	const char *strippedMinFileName = 
	    strippedName (cleanMinFileName);
	
	
	QString maxFuncName, maxEntryKey;
	double maxVal = 
	    um->applicationDataStat (dataTag, UIManager::AttrMax, 
				     dataInfo->entryStat, 
				     &maxFuncName,
				     &maxEntryKey);
	int maxLineNo = um->entryLine (maxFuncName, maxEntryKey);
	
	// Get maxFileName from maxFuncName
	QString maxFileName = um->functionFileName (maxFuncName);
	const char *cleanMaxFileName = 
	    cleanName (maxFileName.latin1());
	const char *strippedMaxFileName = 
	    strippedName (cleanMaxFileName);
	
	double meanVal = 
	    um->applicationDataStat (dataTag, 
				     UIManager::AttrMean,
				     dataInfo->entryStat);
	
	double stdDevVal = 
	    um->applicationDataStat (dataTag, 
				     UIManager::AttrStdDev,
				     dataInfo->entryStat);
	
	double sumVal = 
	    um->applicationDataStat (dataTag, 
				     UIManager::AttrSum,
				     dataInfo->entryStat);
	
	// If have only one point, do simpler version
	if (entryCount == 1)
	{
	    // Use maxVal version
	    statusBuf.sprintf ("1 entry: "
			       "value %g "
			       "(File %s, Line %i, Function %s)", 
			       maxVal, 
			       strippedMaxFileName,
			       maxLineNo,
			       maxFuncName.latin1());
	}
	
	else
	{
	    statusBuf.sprintf ("%i entries: "
			       "Max %g (File %s Line %i)  "
			       "Min %g (File %s Line %i)  "
			       "Mean %g  StdDev %g  Sum %g", 
			       entryCount, 
			       maxVal, 
			       strippedMaxFileName,
			       maxLineNo,
			       minVal, 
			       strippedMinFileName,
			       minLineNo,
			       meanVal, stdDevVal, sumVal);
	}
	
	// Write out stat message to main message area
	setStatusMessage(mainMessage, dataDisplay, statusBuf);
	
	// Put current state in type space on status bar
	statusBuf.sprintf ("%s of %ss", 
			   columnStatName(dataInfo->columnStat), 
			   entryStatName(dataInfo->entryStat));
	setStatusMessage(typeMessage, dataDisplay, statusBuf);
    }

    // If mapInfo not present, say so 
    else if (mapInfo == NULL)
    {
	statusBuf.sprintf ("No MapInfo for recordId %i attrId %i!", 
			   recordId, attrId);
	// Write out stat message to main message area
	setStatusMessage(mainMessage, dataDisplay, statusBuf);
    }
    
    else if (mapInfo->type == fileType)
    {
	// Get fileName from mapInfo
	QString fileName = um->fileAt (mapInfo->fileId);
	
	// Get all the stats for this file over the
	// specified entryStat (specified in dataInfo->entryStat);
	int entryCount = 
	    (int) um->fileDataStat (fileName,  dataTag, 
				    UIManager::AttrCount,
				    dataInfo->entryStat);
	
	QString minFuncName, minEntryKey;
	double minVal = 
	    um->fileDataStat (fileName, dataTag,  UIManager::AttrMin, 
			      dataInfo->entryStat, &minFuncName, 
			      &minEntryKey);
	int minLineNo = um->entryLine (minFuncName, minEntryKey);
	
	QString maxFuncName, maxEntryKey;
	double maxVal = 
	    um->fileDataStat (fileName, dataTag,  UIManager::AttrMax, 
			      dataInfo->entryStat, &maxFuncName,
			      &maxEntryKey);
	int maxLineNo = um->entryLine (maxFuncName, maxEntryKey);
	
	double meanVal = 
	    um->fileDataStat (fileName, dataTag, 
			      UIManager::AttrMean,
			      dataInfo->entryStat);
	
	double stdDevVal = 
	    um->fileDataStat (fileName, dataTag, 
			      UIManager::AttrStdDev,
			      dataInfo->entryStat);
	
	double sumVal = 
	    um->fileDataStat (fileName, dataTag, 
			      UIManager::AttrSum,
			      dataInfo->entryStat);
	
	// If have only one point, do simpler version
	if (entryCount == 1)
	{
	    // Use maxVal version
	    statusBuf.sprintf ("1 entry: "
			       "value %g (Line %i, Function %s)", 
			       maxVal, maxLineNo,
			       maxFuncName.latin1());
	}
	
	else
	{
	    statusBuf.sprintf ("%i entries: "
			       "Max %g (Line %i)  "
			       "Min %g (Line %i)  "
			       "Mean %g  StdDev %g  Sum %g", 
			       entryCount, 
			       maxVal, maxLineNo,
			       minVal, minLineNo,
			       meanVal, stdDevVal, sumVal);
	}
	
	// Write out stat message to main message area
	setStatusMessage(mainMessage, dataDisplay, statusBuf);
	
	// Put current state in type space on status bar
	statusBuf.sprintf ("%s of %ss", 
			   columnStatName(dataInfo->columnStat), 
			   entryStatName(dataInfo->entryStat));
	setStatusMessage(typeMessage, dataDisplay, statusBuf);
    }
    
    else if (mapInfo->type == funcType)
    {
	// Get funcName from mapInfo
	QString funcName = um->functionAt (mapInfo->funcId);
	
	// Get all the stats for this function over the
	// specified entryStat (specified in dataInfo->entryStat);
	int entryCount = 
	    (int) um->functionDataStat (funcName,  dataTag, 
					UIManager::AttrCount,
					dataInfo->entryStat);
	
	QString minEntryKey;
	double minVal = 
	    um->functionDataStat (funcName, dataTag, 
				  UIManager::AttrMin, 
				  dataInfo->entryStat, &minEntryKey);
	int minLineNo = um->entryLine (funcName, minEntryKey);
	
	QString maxEntryKey;
	double maxVal = 
	    um->functionDataStat (funcName, dataTag, 
				  UIManager::AttrMax, 
				  dataInfo->entryStat, &maxEntryKey);
	int maxLineNo = um->entryLine (funcName, maxEntryKey);
	
	double meanVal = 
	    um->functionDataStat (funcName, dataTag, 
				  UIManager::AttrMean,
				  dataInfo->entryStat);
	
	double stdDevVal = 
	    um->functionDataStat (funcName, dataTag, 
				  UIManager::AttrStdDev,
				  dataInfo->entryStat);
	
	double sumVal = 
	    um->functionDataStat (funcName, dataTag, 
				  UIManager::AttrSum,
				  dataInfo->entryStat);
	
	// If have only one point, do simpler version
	if (entryCount == 1)
	{
	    // Use maxVal version
	    statusBuf.sprintf ("1 entry: "
			       "value %g (Line %i)", 
			       maxVal, maxLineNo);
	}
	
	else
	{
	    statusBuf.sprintf ("%i entries: "
			       "Max %g (Line %i)  "
			       "Min %g (Line %i)  "
			       "Mean %g  StdDev %g  Sum %g", 
			       entryCount, 
			       maxVal, maxLineNo,
			       minVal, minLineNo,
			       meanVal, stdDevVal, sumVal);
	}
	
	// Write out stat message to main message area
	setStatusMessage(mainMessage, dataDisplay, statusBuf);
	
	// Put current state in type space on status bar
	statusBuf.sprintf ("%s of %ss", 
			   columnStatName(dataInfo->columnStat), 
			   entryStatName(dataInfo->entryStat));
	setStatusMessage(typeMessage, dataDisplay, statusBuf);
	
    }
    
    else if (mapInfo->type == entryType)
    {
	statusBuf.sprintf ("Entry Type for recordId %i attrId %i!", 
			   recordId, attrId);
	// Write out stat message to main message area
	setStatusMessage(mainMessage, dataDisplay, statusBuf);
    }
    
    // Get the entries for this line that have data
    else if (mapInfo->type == lineType)
    {
	
	int setCount = 0;
	QString firstEntryKey;
	
	// Get funcName and lineNo from mapInfo
	QString funcName = um->functionAt (mapInfo->funcId);
	int lineNo = mapInfo->lineNo;
	
	
	// Scan all entries on this line for data
	int lineEntryCount = um->entryCount(funcName, lineNo);
	for (int lineEntryIndex = 0; lineEntryIndex < lineEntryCount;
	     lineEntryIndex++)
	{
	    // Get entry at entryIndex on this line
	    QString entryKey = um->entryKeyAt (funcName, lineNo,
					       lineEntryIndex);
	    
	    // Is there any data for this entry/dataTag?
	    if (um->entryDataStat (funcName, entryKey, dataTag,
				   UIManager::EntryCount) > 0)
	    {
		// Get the first key that has data
		if (setCount == 0)
		    firstEntryKey = entryKey;
		
		// Update setCount
		setCount++;
	    }
	}
	
	// Because get here only for cell's will some contents,
	// expect at least one cell to have data!
	if (setCount < 1)
	{
	    TG_error ("TreeView::updateStatusBarStats: "
		      "algorithm error! setCount == 0!");
	}
	
	
	// Get all the stats for this entry and dataTag
	int dataCount = 
	    (int) um->entryDataStat (funcName, firstEntryKey, 
				     dataTag, UIManager::EntryCount);
	
	int minTaskId, minThreadId;
	double minVal = 
	    um->entryDataStat (funcName, firstEntryKey, 
			       dataTag, UIManager::EntryMin,
			       &minTaskId, &minThreadId);
	
	
	int maxTaskId, maxThreadId;
	double maxVal = 
	    um->entryDataStat (funcName, firstEntryKey, 
			       dataTag, UIManager::EntryMax,
			       &maxTaskId, &maxThreadId);
	
	
	double meanVal = 
	    um->entryDataStat (funcName, firstEntryKey, 
			       dataTag, UIManager::EntryMean);
	
	double stdDevVal = 
	    um->entryDataStat (funcName, firstEntryKey, 
			       dataTag, UIManager::EntryStdDev);
	
	double sumVal = 
	    um->entryDataStat (funcName, firstEntryKey, 
			       dataTag, UIManager::EntrySum);
	
	// If have only one point, do simpler version
	if (dataCount == 1)
	{
	    // Use maxVal version
	    statusBuf.sprintf ("1 data pt: "
			       "value %g (task %i/thread %i)", 
			       maxVal, maxTaskId, maxThreadId);
	}
	
	else
	{
	    statusBuf.sprintf ("%i data pts: "
			       "Max %g (Rank %i/Thread %i)  "
			       "Min %g (%i/%i)  "
			       "Mean %g  StdDev %g  Sum %g", 
			       dataCount, 
			       maxVal, maxTaskId, maxThreadId, 
			       minVal, minTaskId, minThreadId, 
			       meanVal, stdDevVal, sumVal);
	}
	
	// If have multiple entries with data, indicate that
	// only showing first ont
	if (setCount > 1)
	{
	    QString preBuf;
	    preBuf.sprintf ("(1st of %i entries) ", setCount);
	    statusBuf.prepend (preBuf);
	}
	
	// Write out stat message to main message area
	setStatusMessage(mainMessage, dataDisplay, statusBuf);
	
	// Put current state in type space on status bar
	statusBuf.sprintf ("%s", entryStatName(dataInfo->entryStat));
	setStatusMessage(typeMessage, dataDisplay, statusBuf);
    }
}


// Internal routine to allow prioritized access to the limited 
// status bar space.  The status bar is not actually updated 
// until updateStatusMessage() is called.
void TreeView::setStatusMessage (TreeView::StatusType statusType, 
				    TreeView::StatusSource statusSource,
				    const QString &message)
{
    // Sanity check, make sure statusType in range
    if (((int)statusType < 0) || ((int)statusType > 2))
	TG_error ("TreeView::updateStatusMessage: statusType (%i) "
		  "out of range!", statusType);

    // Sanity check, make sure statusSource in range
    if (((int)statusSource < 0) || ((int)statusSource > 2))
	TG_error ("TreeView::updateStatusMessage: statusSource (%i) "
		  "out of range!", statusSource);

    // Update statusMessageQueue
    statusMessageQueue[(int)statusType][(int)statusSource] = message;
}

// Internal routine to update the status Bar to reflect the messages
// set by setStatusMessage().  Updates only those messages that change
// and give priority to the statusSource with the lowest number
void TreeView::updateStatusMessage()
{
    // Do nothing, if not status bar
    if (status == NULL)
	return;
    
    // Scan the three status bar sections for new message
    for (int statusType = 0; statusType < 3; statusType ++)
    {
	QString *newMessage = NULL;

	// Scan in priority order for a non-empty message
	for (int statusSource = 0; statusSource < 3; statusSource++)
	{
	    if (!statusMessageQueue[statusType][statusSource].isEmpty())
	    {
		// Point at the highest priority non-empty message
		newMessage = &statusMessageQueue[statusType][statusSource];

		// Stop the search, this is the one we want
		break;
	    }
	}
	
	// If no message, clear text if not already
	if ((newMessage == NULL) && 
	    (!statusMessages[statusType].isEmpty()))
	{
	    // Clear the appropriate window
	    switch ((StatusType)statusType)
	    {
	      case mainMessage:
		status->clear();
		break;
	      case typeMessage:
		statusStatType->clear();
		break;
	      case lineMessage:
		statusLineNo->clear();
		break;
	      default:
		TG_error ("TreeView::updateStatusMessage: unsupported type %i",
			  statusType);
	    }
	    // Record that statusType is cleared
	    statusMessages[statusType] = "";
	}
	// If have new message, set it
	else if ((newMessage != NULL) &&
		 (*newMessage != statusMessages[statusType]))
	{
	    // Set the text in the appropriate window
	    switch ((StatusType)statusType)
	    {
	      case mainMessage:
		status->message(*newMessage);
		break;
	      case typeMessage:
		statusStatType->setText(*newMessage);
		break;
	      case lineMessage:
		statusLineNo->setText(*newMessage);
		break;
	      default:
		TG_error ("TreeView::updateStatusMessage: unsupported type %i",
			  statusType);
	    }

	    // Record that message statusType now has
	    statusMessages[statusType] = *newMessage;
	}
    }
}

// Internal routine to update status bar to reflect current mouse position
// If handle == NULL_INT, assumes not over annotation
// If index == NULL_INT, assumes over tree hierarchy control
void TreeView::updateStatusBar (int recordId, int attrId,
				ButtonState /* buttonState */,
				int /* index */, int /* slot */,
				int handle, int layer)
{
    // Update statusBar, if one exists
    if (status != NULL)
    {
	// Update location info only if recordId changes
	if (recordId != statusLastRecordId)
	{
	    // For simplicity, initially, clear all location messages
	    // Will not update until updateStatusMessage is called.
	    setStatusMessage(mainMessage, locationDisplay, "");
	    setStatusMessage(typeMessage, locationDisplay, "");
	    setStatusMessage(lineMessage, locationDisplay, "");

	    // Is there line info for this record Id?
	    MapInfo *mapInfo = recordIdTable.findEntry (recordId);

	    // If it exists, set as much of placement info as possible
	    if (mapInfo != NULL)
	    {
		// Set lineNo, if exists
		if ((mapInfo->lineNo != NULL_INT))
		{
		    statusBuf.sprintf ("L%i", mapInfo->lineNo);
		    setStatusMessage(lineMessage, locationDisplay, statusBuf);
		}

		// For files, functions and lines, 
		// set mainMessage to fileName
		if ((mapInfo->type == fileType) || 
		    (mapInfo->type == funcType) || 
		    (mapInfo->type == lineType))
		{
		    // Get fileName from mapInfo
		    QString fileName = um->fileAt (mapInfo->fileId);
		    
		    // Just get actual file name with no path info
		    const char *cleanFileName =
			cleanName (fileName.latin1());
		    const char *strippedFileName =
			strippedName (cleanFileName);

		    // Set mainMessage to fileName
		    setStatusMessage(mainMessage, locationDisplay, 
				     strippedFileName);
		}
		
		// For lines, set typeMessage to function Name
		if ((mapInfo->type == lineType) ||
		    (mapInfo->type == funcType))
		{
		    // Get funcName from mapInfo
		    QString funcName = um->functionAt (mapInfo->funcId);

		    // Get the clean name for the func
		    const char *cleanFuncName = cleanName (funcName.latin1());
		    
		    // Set typeMessage to funcName
		    setStatusMessage(typeMessage, locationDisplay, 
				     cleanFuncName);
		}
	    }
	}

	// For now, update data stat info only if cell changes
	if ((attrId != statusLastAttrId) || (recordId != statusLastRecordId))
	{
	    // Update any dataDisplay messages, if any
	    updateStatusBarStats (recordId, attrId);
	}

	// Update usageHelp if over annotation 
	if (handle != NULL_INT)
	{
	    // Handle over action menu
	    if (layer == -1)
	    {
		statusBuf.sprintf ("Left click for action menu");
		// Set help text
		setStatusMessage(mainMessage, usageHelp, statusBuf);
	    }
	    // Handle over data menu
	    else if (layer == -2)
	    {
		statusBuf.sprintf ("Left click for datamenu");
		// Set help text
		setStatusMessage(mainMessage, usageHelp, statusBuf);
	    }
	    // Handle over specific action icon (layer == actionIndex)
	    else if ((layer >= 0) && (layer < um->actionCount()))
	    {
		// Is there line info for this record Id?
		MapInfo *mapInfo = recordIdTable.findEntry (recordId);

		// Better be and funcId better be set
		if ((mapInfo == NULL) || (mapInfo->funcId == NULL_INT))
		    TG_error ("TreeView::updateStatusBar: No mapInfo!");

		// Get funcName from mapInfo
		QString funcName = um->functionAt(mapInfo->funcId);

		// Get entryKey from handle (encodes entryId)
		QString entryKey = um->entryKeyAt(funcName, handle);

		// Get actionTag at layer (encodes actionIndex)
		QString actionTag = um->actionAt (layer);

		// COME BACK, for now, just look at first PTPair's info
		int taskId = um->PTPairTaskAt(0);
		int threadId = um->PTPairThreadAt(0);

		// Get the current action state
                // DEBUG FOR NOW, activated maps to "In" and
                // deactivated maps to "Out".
                QString stateTag;

                // If one or more is activated, set flag
                if (um->isActionActivated(funcName, entryKey, actionTag, 
					  taskId, threadId) != 0)
                {
                    stateTag = "In";
                }
                else
                {
                    stateTag = "Out";
                }

		// Get the tool tip for actions in this state
		QString inToolTip = um->actionStateInToolTip (actionTag,
							      stateTag);

		// Append menu click message
		statusBuf.sprintf ("%s; Left click for menu",
				   inToolTip.latin1());
		// Set help text
		setStatusMessage(mainMessage, usageHelp, statusBuf);
	    }
	    // Otherwise, don't know
	    else
	    {
		TG_error ("TreeView::updateStatusBar: Unhandled annot "
			  "layer %i!", layer);
	    }
	}
	// Otherwise, clear help text
	else
	{
	    // Clear help text
	    setStatusMessage(mainMessage, usageHelp, "");
	}

	// Remember that we have updated status for this recordId
	statusLastRecordId = recordId;

	// Remember that we have updated status for this attrId
	statusLastAttrId = attrId;

	// Remember the last handle we have updated 
	statusLastHandle = handle;
    }

    // Actually update the statusBar with any changes
    updateStatusMessage();
}

// If over clickable cell annotation, change into pointing hand
void TreeView::annotMovedOverHandler(int recordId, int attrId,
				     ButtonState buttonState,
				     int handle, int layer, 
				     int index, int slot,
				     int /* x */, int /* y */, int /* w */, int /* h */)
{
    // Indicate that this item is clickable, if not already
    // Assume only special cursor currently set (need more state later)
    if (!grid->ownCursor())
        grid->setCursor(QCursor (pointingHandCursor));

    // Update the status bar
    updateStatusBar(recordId, attrId, buttonState, index, slot,
		    handle, layer);
}


// Removes pointing hand (if set), since not over clickable annotation
void TreeView::slotMovedOverHandler(int recordId, int attrId,
				    ButtonState buttonState,
				    int index, int slot,
				    int /*x*/, int /*y*/, int /*w*/, int /*h*/)
{
    // Restore to default cursor if not already
    if (grid->ownCursor())
	grid->unsetCursor();

    // Update the status bar
    updateStatusBar(recordId, attrId, buttonState, index, slot);
}

// Handles mouse over the tree hierachy control area events
void TreeView::treeMovedOverHandler (int recordId, int attrId,
				     ButtonState buttonState,
				     int /*indentLevel*/, int /*parentRecordId*/,
				     bool /*treeOpen*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/)
{
    // Update the status bar
    updateStatusBar(recordId, attrId, buttonState);
}

// Handles mouse over empty area events
void TreeView::emptyAreaMovedOverHandler (int recordId, int attrId,
					  ButtonState buttonState, int /*gridx*/, 
					  int /*gridy*/)
{
    // Update the status bar, one or both of recordId or attrId will be -1
    updateStatusBar(recordId, attrId, buttonState);
}

// Internal helper function to add menu items for the state transitions
// allowed from the current action state.
// Returns TRUE if items added and FALSE if no menu items added
bool TreeView::addStateTransToMenu (QPopupMenu *popupMenu, 
				    const QString &funcName, 
				    const QString &entryKey, 
				    const QString &actionTag)
{
    // Return FALSE if user not allowed to change this action's state
    if (!um->isActionEnabled (funcName, entryKey, actionTag))
    {
	// DEBUG
	printf ("User not allowed to affect action '%s'\n",
		actionTag.latin1());

	return FALSE;
    }
    
    // COME BACK AND HANDLE PARALLEL STATE (FOR NOW JUST LOOKS
    // AT STATE FOR THE FIRST PROCESSES/THREAD PAIR DECLARED)
    int taskId = um->PTPairTaskAt(0);
    int threadId = um->PTPairThreadAt(0);

    // Get current action state for this entry
    // DEBUG, map activated to 'In' and deactivated to 'Out'
    QString stateTag;
    if (um->isActionActivated(funcName, entryKey, actionTag,
			      taskId, threadId))
    {
	// Yes, for now map to 'In' until states functions implemented
	stateTag = "In";
    }
    else
    {
	// No, for now map to 'Out' until states functions implemented
	stateTag = "Out";
    }

    // Get the state transitions allowed from this state
    int transitionCount = um->actionStateTransitionCount(actionTag,
							 stateTag);

    // Sanity check, should not get NULL_INT here!
    if (transitionCount == NULL_INT)
    {
	TG_error ("TreeView::addStateTransToMenu: "
		  "error getting trans count for '%s' '%s'",
		  actionTag.latin1(), stateTag.latin1());
    }

    // If not transitions allowed, return FALSE now
    if (transitionCount <= 0)
	return (FALSE);

    // Loop thru valid transitions adding them to the menu
    for (int transIndex = 0; transIndex < transitionCount; transIndex++)
    {
	// Get the 'toStateTag' that user may transition to
	QString toStateTag = um->actionStateTransitionAt (actionTag, 
							  stateTag,
							  transIndex);
	
	// Get the menuText for transitioning to this state
	QString toMenuText = um->actionStateToMenuText (actionTag, 
							toStateTag);

	// Get the pixmapTag for transitioning to this state (may be empty)
	QString toPixmapTag = um->actionStateToPixmapTag (actionTag, 
							  toStateTag);

	// Get the toolTip for transitioning to this state
	QString toMenuToolTip = um->actionStateToToolTip (actionTag,
							  toStateTag);
	
	// Get pixmapInfo, if pixmapTag not empty
	PixmapInfo *pixmapInfo = NULL;
	if (!toPixmapTag.isEmpty())
	{
	    // Find pixmap info, expect it to be there!
	    pixmapInfo = 
		pixmapInfoTable.findEntry (toPixmapTag.latin1());
	    if (pixmapInfo == NULL)
	    {
		TG_error ("TreeView::addStateTransToMenu: "
			  "Error pixmap '%s' not found for to state '%s'!",
			  toPixmapTag.latin1(), toStateTag.latin1());
	    }
	}
	
	// Compute Id that encodes both the action and state id.
	// For now, use up 16 bits for action id and the lower 16 bits
	// for the transition index for this action's state
	int actionIndex = um->actionIndex (actionTag);
//	int menuId = (actionIndex << 16) + transIndex;
	// DEBUG, use actionIndex for now
	int menuId = actionIndex;

	// If have pixmap, use pixmap version of insertItem for 
	if (pixmapInfo != NULL)
	{
	    popupMenu->insertItem(pixmapInfo->iconSet, toMenuText.latin1(),
				  this, SLOT(actionMenuClickHandler(int)), 
				  0 /* No keyboard accelerator for now */, 
				  menuId);
	}
	// Otherwise, us plain text version
	else
	{
	    popupMenu->insertItem(toMenuText.latin1(),
				  this, SLOT(actionMenuClickHandler(int)), 
				  0 /* No keyboard accelerator for now */, 
				  menuId);
	}

	// Set the What's this text to the toolTip for the transition
	popupMenu->setWhatsThis(menuId, toMenuToolTip);
    }

    // Return TRUE, did add at least one item to menu
    return (TRUE);
}

// Internal routine that moves popup menu just below cell clicked on
// aligned with index/slot passed
void TreeView::placeMenuBelowCell (QPopupMenu *popupMenu, int recordId,
				   int attrId, int index, int slot)
{
    // Get starting point of cell in grid
    int startx = grid->attrIdXPos (attrId);
    int starty = grid->recordIdYPos (recordId);

    // Get offset of annotation, if cell exists
    int xOffset;
    if (grid->cellType(recordId, attrId) != CellGrid::invalidType)
	xOffset = grid->slotXOffset (recordId, attrId, index, slot);

    // Otherwise, pick 0 offset for empty/non-existant cells
    else
	xOffset = 0;

    // Get height of one row popup menu???
    int yOffset = 2*grid->recordIdHeight(recordId);

    // Map click point in Contents area to location in viewport
    int winX, winY;
    grid->contentsToViewport (startx+xOffset, starty+yOffset, winX, winY);

    // Map the passed location to a global coordinates
    QPoint loc = grid->mapToGlobal(QPoint(winX, winY));

    // Move the menu to the calculate location
    popupMenu->move(loc);
}

// Internel helper routine that returns column stat name as const char *
const char *TreeView::columnStatName (UIManager::AttrStat columnStat)
{
    const char *name;
    switch (columnStat)
    {
      case UIManager::AttrInvalid:
	name = "(Invalid Column Stat)";
	break;
	
      case UIManager::AttrSum:
	name = "Sum";
	break;

      case UIManager::AttrMean:
	name = "Mean";
	break;

      case UIManager::AttrMax:
	name = "Max";
	break;

      case UIManager::AttrMin:
	name = "Min";
	break;

      case UIManager::AttrCount:
	name = "Count";
	break;

      case UIManager::AttrStdDev:
	name = "Std Dev";
	break;

      case UIManager::AttrSumOfSquares:
	name = "Sum of Squares";
	break;

      default:
	TG_error ("TreeView::columnStatName: Unknown stat %i!", columnStat);
	name = ""; // Avoid compiler warning
    }

    // Return the stat name
    return (name);
}

// Internel helper routine that returns entry stat name as const char *
const char *TreeView::entryStatName (UIManager::EntryStat entryStat)
{
    const char *name;
    switch (entryStat)
    {
      case UIManager::EntryInvalid:
	name = "(Invalid Column Stat)";
	break;
	
      case UIManager::EntrySum:
	name = "Sum";
	break;

      case UIManager::EntryMean:
	name = "Mean";
	break;

      case UIManager::EntryMax:
	name = "Max";
	break;

      case UIManager::EntryMin:
	name = "Min";
	break;

      case UIManager::EntryCount:
	name = "Count";
	break;

      case UIManager::EntrySumOfSquares:
	name = "Sum of Squares";
	break;


      case UIManager::EntryStdDev:
	name = "Std Dev";
	break;

      default:
	TG_error ("TreeView::entryStatName: Unknown stat %i!", entryStat);
	name = "";      // Avoid compiler warning
    }

    // Return the stat name
    return (name);
}

// Handles clicks on data columns (for now, may want do something differnt
// for data values)
void TreeView::dataClickHandler (int recordId, int attrId,
				 ButtonState /*buttonState*/,
				 int /*handle*/, int /*layer*/, int /*index*/, int /*slot*/,
				 int /*x*/, int y, int /*w*/, int /*h*/)
{
    // Alias commonPopupMenu to statsMenu for clarity
    QPopupMenu *statsMenu = &commonPopupMenu;

    // Clear old stats menu contents
    statsMenu->clear();

    // Clear search highlighting, if any, on any mouse click in grid
#if 0
    clearSearchHighlighting();
#endif

    // Figure out dataAttrTag from attrId
    int dataTagIndex = attrId - dataStartAttrId;
    QString dataAttrTag = um->dataAttrAt(dataTagIndex);

    // Better not be NULL_QSTRING
    if (dataAttrTag == NULL_QSTRING)
    {
	TG_error ("TreeView::dataClickHandler:: algorithm error, "
		  "can't find tag for attrId %i!", attrId);
    }

    // Get dataInfo for this dataTagIndex
    DataInfo *dataInfo = dataInfoTable.findEntry (dataTagIndex);
    if (dataInfo == NULL)
	TG_error ("TreeView::dataClickHandler:: dataInfo not found!");

    // Save the dataAttrTagIndex for menu selection
    dataMenuDataIndex = dataTagIndex;

    // Get the type of line we are on
    MapInfoType recordIdType = TreeView::invalidType;
    MapInfo *mapInfo = recordIdTable.findEntry (recordId);
    if (mapInfo != NULL)
	recordIdType = mapInfo->type;
    
    
    // Create menu header
    QString buf;
    buf.sprintf ("%s Rollup Settings", um->dataAttrText(dataAttrTag).latin1());
    statsMenu->insertItem(buf, 0);
    statsMenu->insertSeparator();

    // If not a source line (not lineType or entryType), display
    // fully menu with func/file/app rollup options first
    if ((recordIdType != lineType) && (recordIdType != entryType))
    {
	statsMenu->insertItem ("Sum of column values", 
			       this, SLOT(dataMenuClickHandler(int)),
			       0 /* No keyboard accelerator for now*/,
			       100+UIManager::AttrSum);
	statsMenu->insertItem ("Mean of column values", 
			       this, SLOT(dataMenuClickHandler(int)),
			       0 /* No keyboard accelerator for now*/,
			       100+UIManager::AttrMean);
	statsMenu->insertItem ("Max of column values", 
			       this, SLOT(dataMenuClickHandler(int)),
			       0 /* No keyboard accelerator for now*/,
			       100+UIManager::AttrMax);
	statsMenu->insertItem ("Min of column values", 
			       this, SLOT(dataMenuClickHandler(int)),
			       0 /* No keyboard accelerator for now*/,
			       100+UIManager::AttrMin);

	// Test all stats functionality, may only want subset of options 
	// for user's use
	statsMenu->insertItem ("Number of column values", 
			       this, SLOT(dataMenuClickHandler(int)),
			       0 /* No keyboard accelerator for now*/,
			       100+UIManager::AttrCount);
	statsMenu->insertItem ("Sum of squares of column values", 
			       this, SLOT(dataMenuClickHandler(int)),
			       0 /* No keyboard accelerator for now*/,
			       100+UIManager::AttrSumOfSquares);
	statsMenu->insertItem ("Std dev of column values", 
			       this, SLOT(dataMenuClickHandler(int)),
			       0 /* No keyboard accelerator for now*/,
			       100+UIManager::AttrStdDev);
	statsMenu->insertSeparator();

	// Check the current setting
	statsMenu->setItemChecked (100+dataInfo->columnStat, TRUE);
    }

    statsMenu->insertItem ("Sum of task/thread values", 
			   this, SLOT(dataMenuClickHandler(int)),
			   0 /* No keyboard accelerator for now*/,
			   UIManager::EntrySum);
    statsMenu->insertItem ("Mean of task/thread values", 
			   this, SLOT(dataMenuClickHandler(int)),
			   0 /* No keyboard accelerator for now*/,
			   UIManager::EntryMean);
    statsMenu->insertItem ("Max of task/thread values", 
			   this, SLOT(dataMenuClickHandler(int)),
			   0 /* No keyboard accelerator for now*/,
			   UIManager::EntryMax);
    statsMenu->insertItem ("Min of task/thread values", 
			   this, SLOT(dataMenuClickHandler(int)),
			   0 /* No keyboard accelerator for now*/,
			   UIManager::EntryMin);

    // Test all stats functionality, may only want subset of options 
    // for user's use
    statsMenu->insertItem ("Number of task/thread values", 
			   this, SLOT(dataMenuClickHandler(int)),
			   0 /* No keyboard accelerator for now*/,
			   UIManager::EntryCount);
    statsMenu->insertItem ("Sum of squares of task/thread values", 
			   this, SLOT(dataMenuClickHandler(int)),
			   0 /* No keyboard accelerator for now*/,
			   UIManager::EntrySumOfSquares);
    statsMenu->insertItem ("Std dev of task/thread values", 
			   this, SLOT(dataMenuClickHandler(int)),
			   0 /* No keyboard accelerator for now*/,
			   UIManager::EntryStdDev);
    statsMenu->setCheckable (TRUE);

    // Check the current setting
    statsMenu->setItemChecked (dataInfo->entryStat, TRUE);


    // Move statsMenu to just below cell clicked on (if exists)
    if ((recordId >= 0) && (attrId >= 0))
    {
	placeMenuBelowCell (statsMenu, recordId, attrId, 0, 0);
    }
    // Otherwise place using attrId and y location
    else
    {
	int startx = grid->attrIdXPos (attrId);

	// Get height of one row popup menu???
	int yOffset = 2*grid->recordIdHeight(0);

	// Map click point in Contents area to location in viewport
	int winX, winY;
	grid->contentsToViewport (startx, y+yOffset, winX, winY);

	// Map the passed location to a global coordinates
	QPoint loc = grid->mapToGlobal(QPoint(winX, winY));

	// Move the menu to the calculate location
	statsMenu->move(loc);
    }


    // Done, show the menu and wait for user to click on something
    statsMenu->show();
}

// Handles clicks on data popUp menus
void TreeView::dataMenuClickHandler(int id)
{
    // Sanity check, better have popup menu in play
    if (dataMenuDataIndex == -1)
	TG_error ("TreeView::dataMenuClickHandler:: no popup menu in play!");

#if 0
    // DEBUG
    printf ("TreeView::dataMenuClickHandler: id %i clicked for dataIndex %i\n",
	    id, dataMenuDataIndex);
#endif

    // Get dataInfo for this dataTagIndex
    DataInfo *dataInfo = dataInfoTable.findEntry (dataMenuDataIndex);
    if (dataInfo == NULL)
	TG_error ("TreeView::dataMenuClickHandler:: dataInfo not found!");

    // Change entryStat if id < 100
    if (id < 100)
    {
	// Do updates only if value changes
	if (dataInfo->entryStat != (UIManager::EntryStat)id)
	{

	    // Update entryStat with new value
	    dataInfo->entryStat = (UIManager::EntryStat)id;

	    // Get dataTag for ease of use
	    QString dataTag = um->dataAttrAt(dataMenuDataIndex);

	    // Update application rollup
	    updateAppRollup (dataTag);

	    // Update all the file rollups in the grid
	    // May want to do in heirarchy order in future to 
	    // improve cache performance.
	    int numFiles = um->fileCount();
	    for (int fileIndex = 0; fileIndex < numFiles; fileIndex++)
	    {
		QString fileName = um->fileAt(fileIndex);
		updateFileRollup (fileName, dataTag);
	    }

	    // Update all entries and function rollups that have data in it
	    int numFuncs = um->functionCount();
	    for (int funcIndex = 0; funcIndex < numFuncs; funcIndex++)
	    {
		QString funcName = um->functionAt(funcIndex);
		updateFunctionRollup (funcName, dataTag);

		// Scan all entries in each function
		int numEntries = um->entryCount (funcName);
		for (int entryIndex = 0; entryIndex < numEntries; entryIndex++)
		{
		    QString entryKey = um->entryKeyAt (funcName, entryIndex);

		    // Update dataCell only if data set for um or diffUm
		    if ((um->entryDataStat (funcName, entryKey, dataTag,
					    UIManager::EntryCount) > 0) || 
			((diffUm != NULL) && 
			 (diffUm->entryDataStat (funcName, entryKey, dataTag,
						 UIManager::EntryCount) > 0)))
		    {
			updateDataCell(funcName, entryKey, dataTag);
		    }
		}
	    }

	    // Make sure update takes right away
	    grid->flushUpdate();
	}
    }

    // Otherwise change columStat
    else
    {
	// Do updates only if value changes
	if (dataInfo->columnStat != (UIManager::AttrStat)(id - 100))
	{
	    // Set columnStat to new value
	    dataInfo->columnStat = (UIManager::AttrStat)(id - 100);

	    // Get dataTag for ease of use
	    QString dataTag = um->dataAttrAt(dataMenuDataIndex);

	    // Update application rollup
	    updateAppRollup (dataTag);

	    // Update all the file rollups in the grid
	    // May want to do in heirarchy order later
	    int numFiles = um->fileCount();
	    for (int fileIndex = 0; fileIndex < numFiles; fileIndex++)
	    {
		QString fileName = um->fileAt(fileIndex);
		updateFileRollup (fileName, dataTag);
	    }

	    // Update all the function rollups in the grid
	    int numFuncs = um->functionCount();
	    for (int funcIndex = 0; funcIndex < numFuncs; funcIndex++)
	    {
		QString funcName = um->functionAt(funcIndex);
		updateFunctionRollup (funcName, dataTag);
	    }

	    // Make sure update takes right away
	    grid->flushUpdate();
	}
    }

    // Indicate that dataMenu no longer in play
    dataMenuDataIndex = -1;
}


// Handles clicks not on annotations
void TreeView::cellSlotClickHandler (int recordId, int attrId, 
				     ButtonState buttonState, 
				     int index, int slot, int x, int y, 
				     int w, int h)
{
    // Clear search highlighting, if any, on any mouse click in grid
#if 0
    clearSearchHighlighting();
#endif

    // If click on a data column, handle it even if no data value
    if (attrId >= dataStartAttrId)
    {
	dataClickHandler (recordId, attrId,
			  buttonState,
			  index, slot, NULL_INT, NULL_INT, 
			  x, y, w, h);
    }
}

// Handles clicks on empty area events
void TreeView::emptyAreaClickHandler (int recordId, int attrId,
				      ButtonState buttonState, int gridx, 
				      int gridy)
{
#if 0
    // Clear search highlighting, if any, on any mouse click in grid
    clearSearchHighlighting();
#endif

    // If on a data column, handle even if in empty area (for now
    if (attrId >= dataStartAttrId)
    {
	dataClickHandler (recordId, attrId,
			  buttonState,
			  NULL_INT, NULL_INT, NULL_INT, NULL_INT, 
			  gridx, gridy, NULL_INT, NULL_INT);
    }
}


// Handles clicks on clickable cell annotations
void TreeView::annotClickHandler(int recordId, int attrId,
				 ButtonState buttonState,
				 int handle, int layer, int index, int slot,
				 int x, int y, int w, int h)
{
#if 0
    // Clear search highlighting, if any, on any mouse click in grid
    clearSearchHighlighting();
#endif

    // For now, if layer == -2, this is really a data click and
    // let that handler handle it (need to change clickable stuff
    // to catch this the way I want to)
    if (layer == -2)
    {
	// Call data click handler
	dataClickHandler(recordId, attrId, buttonState, handle, layer,
			 index, slot, x, y, w, h);

	// Return now
	return;
    }
    
    // Use actionPopupMenu instead of commonPopupMenu in order to allow
    // on-line help listener.  Still use actionMenu alias for clarity.
    QPopupMenu *actionMenu = &actionPopupMenu;
    

    // Record popUp Menu state in class variables, for use by
    // actionMenuClickHandler
    actionMenuRecordId = recordId;
    actionMenuAttrId = attrId;
    actionMenuEntryId = handle;

    // Get the action the user clicked on so we can construct the
    // menu properly
    MapInfo *funcInfo = recordIdTable.findEntry(actionMenuRecordId);
    QString funcName = um->functionAt(funcInfo->funcId);
    QString entryKey = um->entryKeyAt(funcName, actionMenuEntryId);

#if 0
    // DEBUG 
    printf ("annotClickHandler (rec %i, att %i, handle %i layer %i "
	    "index %i slot %i, x %i, y %i, w %i, h %i (%s %s))\n", 
	    recordId, attrId, handle, layer, index, slot, x, y, w, h,
	    funcName.latin1(), entryKey.latin1());
#endif


    // Get the number of actions defined
    int actionCount = um->actionCount();

    actionMenu->clear();

    // Make sure there is something in the menu
    bool menuEmpty = TRUE;

    // If layer -1, then clicked on menu icon, build menu out of current state
    if (layer == -1)
    {
	int actionCount = um->actionCount();

	// Add appropriate menu items for actions that it might be possible
	// for the user to modify at this point.
	for (int actionIndex = 0; actionIndex < actionCount; actionIndex++)
	{
	    // Get actionTag at index
	    QString actionTag = um->actionAt (actionIndex);

	    // Add any applicable menu items for this state
	    // (This call determines what to add and returns TRUE
	    //  if something is added.)
	    if (addStateTransToMenu (actionMenu, funcName, 
				     entryKey, actionTag))
	    {
		// Yes, mark that menu items were added
		menuEmpty = FALSE;
	    }
	}
    }
    // Otherwise clicked on a specific action icon (layer == actionIndex)
    else if ((layer >= 0) && (layer < actionCount))
    {
	    // Get actionTag at layer (encodes actionIndex)
	    QString actionTag = um->actionAt (layer);

	    // Yes, update menu to reflect options, were there any?
	    if (addStateTransToMenu (actionMenu, funcName, 
				     entryKey, actionTag))
	    {
		// Yes, mark that menu items were added
		menuEmpty = FALSE;
	    }
    }
    else
    {
	// DEBUG, for now
	printf ("I don't know how to handle click on layer %i (%i actions)\n",
		layer, actionCount);
	return;
    }

    // Put something in menu if user cannot do anything for this item.
    // May want to just not show menu later...
    if (menuEmpty)
    {
	actionMenu->insertItem("(User cannot modify this action)");
    }

    // Place menu just below cell, aligned to slot clicked
    placeMenuBelowCell (actionMenu, recordId, attrId, index, slot);

    // Show completed actionMenu and wait for user to select something
    actionMenu->show();
}

// Handles highlighting options on action popUp menus
void TreeView::actionMenuHighlightHandler(int id)
{
    // Get help text from menu system
    QString helpText = actionPopupMenu.whatsThis (id);

    // Set help text
    setStatusMessage (mainMessage, usageHelp, helpText);

    // Actually update the statusBar with new message
    updateStatusMessage();
}

// Handles signals that clears the actionMenu help
void TreeView::actionMenuClearStatus()
{
    // Clear help text
    setStatusMessage (mainMessage, usageHelp, "");

    // Actually update the statusBar 
    updateStatusMessage();
}

// Handles clicks on action popUp menus
void TreeView::actionMenuClickHandler(int id)
{
    // Sanity check, better have popup menu in play
    if (actionMenuRecordId == -1)
	TG_error ("TreeView::actionMenuClickHandler:: no popup menu in play!");

    MapInfo *funcInfo = recordIdTable.findEntry(actionMenuRecordId);
    QString funcTag = um->functionAt(funcInfo->funcId);
    QString entryKey = um->entryKeyAt(funcTag, actionMenuEntryId);
    QString actionTag = um->actionAt(id);

#if 0
// DEBUG
    printf ("actionMenuClickHandler (%i) entered (recordId %i attrId %i "
	    "entryId %i %s  %s %s\n",
	    id, actionMenuRecordId, actionMenuAttrId, actionMenuEntryId, 
	    funcTag.latin1(), entryKey.latin1(), actionTag.latin1());
#endif

    // COME BACK, for now, just look at first PTPair's info
    int taskId = um->PTPairTaskAt(0);
    int threadId = um->PTPairThreadAt(0);
    if (um->isActionActivated(funcTag, entryKey, actionTag,
			      taskId, threadId))
    {
	um->deactivateAction(funcTag, entryKey, actionTag);
    }
    else
    {
	um->activateAction(funcTag, entryKey, actionTag);
    }

    // Denote no popup menu in play
    actionMenuRecordId = -1;
    actionMenuAttrId = -1;
    actionMenuEntryId = -1;
}

// Handles clicks on action collapse tree popUp menus
void TreeView::collapseTreeHandler(int treeRecordId)
{
    // Want tree that we are closing to be on the screen (not scrolled off).
    // Determine if the row for treeRecordId is scrolled off
    bool scrolledOff = grid->cellScrolledOff (treeRecordId, NULL_INT);

    // If so, need to scroll it onto the screen
    // However, scrolling and closing the tree will both cause the
    // entire screen to be redrawn.  So, takes steps to make it happen
    // just once!
    if (scrolledOff)
    {
#if 0
	// Combine scroll update with close screen update by surpressing
	// grid updates
	// NOTE: This didn't seem to help, try other techniques later
	setUpdatesEnabled(FALSE);
#endif

	grid->scrollCellIntoView (treeRecordId, NULL_INT);
    }

    // Close the tree passed into the handler
    grid->setTreeOpen(treeRecordId, FALSE);


#if 0
    if (scrolledOff)
    {
	// Reenable updates and force redraw (I hope)
	// NOTE: This didn't seem to help, try other techniques later
	setUpdatesEnabled(TRUE);
    }
#endif

}

// Handles clicks on the tree hierachy control area]
// Flips open/closed state and issues parse requests
// where appropriate
void TreeView::treeClickHandler (int recordId, int attrId,
				 ButtonState /*buttonState*/,
				 int /*indentLevel*/, int parentRecordId,
				 bool treeOpen, int x, int /*y*/, int /*w*/, int h)
{
#if 0
    // Clear search highlighting, if any, on any mouse click in grid
    clearSearchHighlighting();
#endif

    // Create treeMenu alias for commonPopupMenu for clarity
    QPopupMenu *treeMenu = &commonPopupMenu;

    // Flip open/closed state for tree parent.   
    // CellGrid sends us the click info, up to us what to do with it
    // Only accept clicks on arrows now (recordId == parentRecordId)
    // Found it disconcerting to have whole tree disappear if
    // hit bar, so now pull up menu in those cases.
    if ((parentRecordId >= 0) && (recordId == parentRecordId))
    {
	// Flip open/closed state for tree parent.   
	grid->setTreeOpen (parentRecordId, !treeOpen);

	
	// If opening tree and corresponds to source file or function, 
	// and has not been expanded before, ask UIManager to parse 
	// this file or function
	if ((!treeOpen) && !expandedRecordId.in(parentRecordId))
	{
	    // Add parentRecordId to the set of records that have been expanded
 	    expandedRecordId.add(parentRecordId);

	    // Get map info for parent record Id
	    MapInfo *mapInfo = recordIdTable.findEntry(parentRecordId);
	    
	    // If have info and of file Type, request parse if unparsed
	    if ((mapInfo != NULL) && (mapInfo->type == fileType))
	    {
		// Get fileName for this recordId
		QString fileName = um->fileAt (mapInfo->fileId);

		// Get current parse state for file
		UIManager::fileState state = um->fileGetState (fileName);

		// If unparsed, request parsing
		if (state == UIManager::fileUnparsed)
		{
		    // Signal service listener that we want this parsed
		    um->fileSetState (fileName, UIManager::fileParseRequested);
		}
	    }

	    // If have info and of func Type, request parse if parsed
	    else if ((mapInfo != NULL) && (mapInfo->type == funcType))
	    {
		// Get funcName for this recordId
		QString funcName = um->functionAt (mapInfo->funcId);

		// Get current parse state for func
		UIManager::functionState state = 
		    um->functionGetState (funcName);

		// If unparsed, request parsing
		if (state == UIManager::functionUnparsed)
		{
		    // Signal service listener that we want this parsed
		    um->functionSetState (funcName, 
					  UIManager::functionParseRequested);
		}

		// Also list source, if possible for this function
		listFunctionSource (funcName);
	    }
#if 0
	    // DEBUG
	    else
	    {
		if (mapInfo == NULL)
		{
		    printf ("Click on tree parent '%d' has NULL mapInfo\n",
			    parentRecordId);
		}
		else
		{
		    printf ("Unhandled MapInfo type '%d'\n", mapInfo->type);
		}
		
	    }
#endif
	}
    }

    // Pull up menu for now, if has parentRecordId 
    else if (parentRecordId >= 0) 
    {
	// Sanity check, expect tree to be open in this case
	if (!treeOpen)
	    TG_error ("TreeView::treeClickHandler: Expected! treeOpen False!");



	// Start with clean slate for popup menu
	treeMenu->clear();

	// Build up menu items by walking back up the tree to the root node
	QString menuBuf;
	for (int parentId = grid->recordIdParent(recordId); 
	     parentId != NULL_INT;
	     parentId = grid->recordIdParent(parentId))
	{
	    // Make menu item "Collapse '(parent contents)'"
	    menuBuf = grid->cellScreenText( parentId, sourceAttrId);
	    menuBuf.prepend("Collapse '");
	    menuBuf.append ("'");

	    // The last 0 in insertItem puts the items at the beginning
	    // of the menu.  I like having them in the same order
	    // as they occur in the tree (from parent to child).
	    // Use parentId as 'id', which is passed to collapseTreeHandler.
	    treeMenu->insertItem (menuBuf, this,  
				  SLOT(collapseTreeHandler(int)), 
				  0, parentId, 0);
	}
	
	// Get starting point of cell in grid
	int startx = grid->attrIdXPos (attrId);
	int starty = grid->recordIdYPos (recordId);
	
	// This offset appears to put menu under cell clicked on
	int yOffset = 2*h;
	
	// Map click point in Contents area to location in viewport
	// Need to play with location more, now based on cell location
	// and lines up horizontally with cursor but puts under current
	// cell. 
	int winX, winY;
	grid->contentsToViewport (startx + x, starty + yOffset, 
				  winX, winY);
	
	// Map the passed location to a global coordinates
	QPoint loc = grid->mapToGlobal(QPoint(winX, winY));
	
	
	treeMenu->move(loc);
	treeMenu->show();
	
//	COME BACK
	
    }

}

// Returns pointer to "clean" name, advanced past the prefix delimiter
// If prefixDelimiter==0, returns entire name (no prefixes)
const char *TreeView::cleanName(const char *name)
{
    // Return now if no prefix character
    if (prefixDelimiter == (char)0)
        return (name);

    // Search for delimiter
    for (const char *ptr = name; *ptr != 0; ptr++)
    {
        // If find delimiter, return pointer to string starting
        // at next character
        if (*ptr == prefixDelimiter)
        {
            return (ptr + 1);
        }
    }

    // If didn't find delimiter, punt
    // See what happens if you just return the name... JMM 2/27/02
//    fprintf (stderr,
 //            "TreeView::cleanName: Error delimiter '%c' not found in '%s'!\n",
  //           prefixDelimiter, name);
   // exit (1);
    return name;

    // To remove warning
//    return ((const char *)NULL);
}
// Returns pointer to a "stripped" name, where all the leading directories
// have been stripped off.  (Advanced past the last '/' or '\'.)
const char *TreeView::strippedName(const char *name)
{
    const char *strippedPtr = name;
    
    // Search for '\' or '/'
    for (const char *ptr = name; *ptr != 0; ptr++)
    {
	// If find forward or backward slash, advance strippedPtr to
	// the character after it.
	if ((*ptr == '\\') || (*ptr == '/'))
	{
	    strippedPtr = ptr + 1;
	}
    }

    // Return the strippedPtr, may be the same pointer passed in.
    return (strippedPtr);
}

// Expands the grid and returns the new recordId created by expansion
int TreeView::createRecordId (void)
{
    // The new recordId is one past current size 
    // (0 based, so don't need to add 1)
    int newRecordId = grid->recordIds();

    // Resize grid so it contains this newRecordId
    grid->resizeGrid(newRecordId+1, grid->attrIds());

#if 0
    // DEBUG, put recordId in attrId 2 for debugging purposes
    QString buf;
    buf.sprintf ("%i", newRecordId);
    grid->setCellText(newRecordId, 2, buf);
#endif

    // Return the new recordId
    return (newRecordId);
}

// Listens for file state changes (parsing, done parsing)
void TreeView::fileStateChanged (const char *fileName, 
				 UIManager::fileState state)
{
    // Get fileId for this file, make sure valid
    int fileId = um->fileIndex (fileName);
    if (fileId == NULL_INT)
    {
        fprintf (stderr, "TreeView::fileStateChanged: Error '%s' not found!\n",
                 fileName);
        exit (1);
    }

    // If don't have MapInfo for this fileId, do nothing
    if (!fileIdTable.entryExists(fileId))
	return;

    // Get the MapInfo for this fileName
    MapInfo *mapInfo = fileIdTable.findEntry(fileId);

    // Limit hourglass markings to those files that are expanded in
    // this window.   Otherwise, get random hourglasses, which looks
    // weird to me.
    // Also, no hourglasses in snapshot views
    if (expandedRecordId.in(mapInfo->recordId) && !snapshotView)
    {
	// If parsing file, put hourglass there, 
	// otherwise remove it (if exists)
	if (state == UIManager::fileParseRequested)
	{
	    // Parsing file, put hourglass before first character of file name
	    grid->addCellPixmapAnnot(mapInfo->recordId, sourceAttrId, 
				     hourglassHandle,TRUE, 1, FALSE, 0, -1,
				     SlotHCenter|SlotVCenter,hourglassPixmap);
	}
	// Otherwise (currently only if parsed), remove hourglass
	else
	{
	    // Remove hourglass annotation (if any)
	    grid->removeCellAnnot(mapInfo->recordId, sourceAttrId, 
				  hourglassHandle);
	}

	// Increase responsiveness, flush updates now
	grid->flushUpdate();
    }
}

// Flushes all pending updates that are driven by the timer
void TreeView::updateTimerHandler()
{
    // Flushes all new File, Function, Entry additions to the UIManager
    // to the screen.  
    // Probably better if done before data cell updates, not sure
    // if required (is it possible that the other way around would be faster).
    flushFileFunctionEntryUpdates ();

    // Flushes all data cell updates (including rollups) recorded by
    // recordDataCellUpdate(). 
    flushDataCellUpdates ();
}


// Flushes all new File, Function, and Entry additions to the UIManager
// to the screen. 
void TreeView::flushFileFunctionEntryUpdates ()
{
    // List all files currently found for program
    int fileCount = um->fileCount();

#if 0
    // DEBUG
    if (fileCount > (lastFileIndexDisplayed +1))
	printf ("Flushing File Updates (%i - %i)\n",
		lastFileIndexDisplayed+1, fileCount-1);
#endif
	

    for (int fileIndex = lastFileIndexDisplayed+1; 
	 fileIndex < fileCount; fileIndex++)
    {
	// Get the file name at this index
	QString fileName = um->fileAt(fileIndex);

	// Show it on the display
	listFileName(fileName.latin1());
    }

    // Update last displayed indexes
    lastFileIndexDisplayed = fileCount - 1;

    // List all the functions currently found for the program
    int funcCount = um->functionCount();

#if 0
    // DEBUG
    if (funcCount > (lastFuncIndexDisplayed +1))
	printf ("Flushing Function Updates (%i - %i)\n",
		lastFuncIndexDisplayed+1, funcCount-1);
#endif

    for (int funcIndex = lastFuncIndexDisplayed+1; 
	 funcIndex < funcCount; funcIndex++)
    {
	// Get the func name at this index
	QString funcName = um->functionAt(funcIndex);

	// Show it on the display
	listFunctionName(funcName.latin1());
    }    

    // Update last displayed indexes
    lastFuncIndexDisplayed = funcCount - 1;

    // Process all pending entry insertions, deleting queue as we go
    // Must do after adding pending function insertions!
    INT_ARRAY_Symbol *entrySymbol;
    while ((entrySymbol = entryInsertTable->head_symbol) != NULL)
    {
	// Parse entry key into indexes
	int funcId = entrySymbol->int_array[0];
	int entryId = entrySymbol->int_array[1];

	// Insert entry now
	listEntry (funcId, entryId);

	// Delete this symbol, making head_symbol the next one to process
	INT_ARRAY_delete_symbol (entrySymbol, NULL);
    }
}

// List file in tree, if not already there
void TreeView::listFileName(const char *fileName)
{
    // Get fileId for this file, make sure valid
    int fileId = um->fileIndex (fileName);
    if (fileId == NULL_INT)
    {
        fprintf (stderr, "TreeView::listFileName: Error '%s' not found!\n",
                 fileName);
        exit (1);
    }

    // If already have MapInfo for this fileId, do nothing
    if (fileIdTable.entryExists(fileId))
	return;
  
    // createRecordId for this fileName
    int fileRecordId = createRecordId();

    // Create mapInfo structure for this fileName
    MapInfo *mapInfo = new MapInfo (fileType, fileRecordId, fileId, NULL_INT);
    TG_checkAlloc(mapInfo);


    // Insert MapInfo into fileIdTable and RecordIdTable
    fileIdTable.addEntry(fileId, mapInfo);
    recordIdTable.addEntry(fileRecordId, mapInfo);

    // Get the clean name for the file
    const char *cleanFileName = cleanName (fileName);

// If defined, sorts base on strippped file name (no directories)
// For now, sort by directories until get color highlighting of names
#undef SORT_STRIPPED

#ifdef SORT_STRIPPED
    // Get file name without delimiter or directories
    // Must use cleanFile name!!!
    const char *strippedFileName = strippedName (cleanFileName);
#endif

    // Place fileRecordId's in alphabetical order. 
    // Search existing children of programRecordId to
    // find proper place to place it
    int belowRecordId = NULL_INT;
    int scanRecordId = grid->recordIdFirstChild (programRecordId);
    while (scanRecordId != NULL_INT)
    {
	// Find map info for scanRecordId so can get file name
	MapInfo *scanMapInfo = recordIdTable.findEntry (scanRecordId);

	// Sanity check, should never be NULL
	if (scanMapInfo == NULL)
	{
	    TG_error ("TreeView::listFileName: scanRecordId %i not found!",
		      scanRecordId);
	}

	// Get file name for scanRecordId
	QString scanFileName = um->fileAt(scanMapInfo->fileId);
	const char *cleanScanFileName = cleanName(scanFileName.latin1());
#ifdef SORT_STRIPPED
	// Get file name without delimiter or directories
	// Must use cleanFile name!!!
	const char *strippedScanFileName = strippedName (cleanScanFileName);
	int diff = strcmp (strippedFileName, strippedScanFileName);
	// For ties, use directory name to break tie
	if (diff == 0)
	    diff = strcmp (cleanFileName, cleanScanFileName);
#else
	int diff = strcmp (cleanFileName, cleanScanFileName);

#endif

	// If next name bigger, stop, found place to insert
	// Use fileId to break ties
	if ((diff < 0) || ((diff == 0) || (fileId < scanMapInfo->fileId)))
	{
#if 0
	    printf (" SORT (stopping): %s < %s\n", fileName, 
		    cleanScanFileName);
#endif
	    break;
	}

#if 0
	printf (" SORT (moving below %i): %s > %s\n", 
		scanRecordId, 
		cleanFileName, 
		cleanScanFileName);
#endif

	// Otherwise, will go below this scanRecordId 
	belowRecordId = scanRecordId;

	// Scan next child (sibling of this child)
	scanRecordId = grid->recordIdNextSibling (scanRecordId);
    }

#if 0
    printf (" SORT placing %s below %i\n",
	    cleanFileName, belowRecordId);
#endif
    // Place file in alphabetical order due to scan above
    grid->placeRecordId (fileRecordId, belowRecordId, programRecordId);

#if 0
    // Make fileRecordId last child of programRecordId
    int belowRecordId = grid->recordIdLastChild (programRecordId);
    grid->placeRecordId (fileRecordId, belowRecordId, programRecordId);
#endif


    // Put file name in new recordId in source Attribute
    grid->setCellText (fileRecordId, sourceAttrId, cleanFileName);
    grid->setCellStyle (fileRecordId, sourceAttrId, fileStyle);

    // Need to rewrite to use asynchronous requests the the source
#if 0
    // Indicate line number range for this file, if source exists,
    // or indicate source doesn't exist
    int fileSize = um->getSourceSize(cleanFileName);
    QString lineRange;
    if (fileSize != 0)
	lineRange.sprintf ("(%3d-%3d):", 1, fileSize);
    else
	lineRange.sprintf ("(source unavailable for %s)", cleanFileName);

    grid->setCellText (fileRecordId, lineAttrId, lineRange.latin1());
    grid->setCellStyle (fileRecordId, lineAttrId, fileStyle);
#endif

    // Make this row expandable (openning will cause file to be parsed)
    grid->setTreeExpandable (fileRecordId, TRUE); 

    // Set to closed state, don't do anything until opened
    grid->setTreeOpen (fileRecordId, FALSE);

    // Scan all data attributes for the file rollup
    int numDataAttrs = um->dataAttrCount();
    for (int dataAttrIndex =0; dataAttrIndex < numDataAttrs;
	 dataAttrIndex++)
    {
	QString dataTag = um->dataAttrAt(dataAttrIndex);
	updateFileRollup (fileName, dataTag);
    }    
}

// Listens for function state changes (parsing, done parsing)
void TreeView::functionStateChanged (const char *funcName, 
				     UIManager::functionState state)
{
    // Get funcId for this func, make sure valid
    int funcId = um->functionIndex (funcName);
    if (funcId == NULL_INT)
    {
        fprintf (stderr, "TreeView::functiongStateChanged: Error '%s' "
		 "not found!\n", funcName);
        exit (1);
    }

    // If don't have MapInfo for this funcId, do nothing
    if (!funcIdTable.entryExists(funcId))
	return;

    // Get the MapInfo for this funcName
    MapInfo *mapInfo = funcIdTable.findEntry(funcId);

    // Limit hourglass markings to those functions that are expanded in
    // this window.   Otherwise, get random hourglasses, which looks
    // weird to me.
    // Also, no hourglasses in snapshot views
    if (expandedRecordId.in(mapInfo->recordId) && !snapshotView)
    {
	// If parsing func, put hourglass there, 
	// otherwise remove it (if exists)
	if (state == UIManager::functionParseRequested)
	{
	    // Parsing func, put hourglass before first character 
	    // of func name
	    grid->addCellPixmapAnnot(mapInfo->recordId, sourceAttrId, 
				     hourglassHandle,TRUE, 1, FALSE, 0, -1,
				     SlotHCenter|SlotVCenter,
				     hourglassPixmap);
	}
	// Otherwise (currently only if parsed), remove hourglass
	else
	{
	    // Remove hourglass annotation (if any)
	    grid->removeCellAnnot(mapInfo->recordId, sourceAttrId, 
				  hourglassHandle);
	}
	
	// Increase responsiveness, flush updates now
	grid->flushUpdate();
    }
}

// Listen's for functions being inserted, calls listFunctionName
void TreeView::functionInserted(const char *funcName, const char */*fileName*/, 
				int /*startLine*/, int /*endLine*/)
{
    // Make sure function is listed in display de
    listFunctionName(funcName);

}

// Lists function name in tree, if not already there
void TreeView::listFunctionName(const char *funcName)
{
    // Get funcId for funcName and make sure valid
    int funcId = um->functionIndex(funcName);
    if (funcId == NULL_INT)
    {
        TG_error ("TreeView::listFunctionName: Error func '%s' not found!",
		  funcName);
    }

    // If already have MapInfo for this funcId, do nothing
    if (funcIdTable.entryExists(funcId))
	return;

    // Get fileId for this function
    QString fileName = um->functionFileName(funcName);
    int fileId = um->fileIndex(fileName);
    if ((fileName == NULL_QSTRING) || (fileId == NULL_INT))
    {
	TG_error ("TreeView::listFunctionName: Error getting file for func %s",
		  funcName);
    }
    
    // If don't have MapInfo for this fileId, cause it to be listed first
    // Should never be needed, but just in case
    if (!fileIdTable.entryExists(fileId))
    {
	fprintf (stderr, "Warning: TreeView::listFunctionName: file '%s' "
		 "not already added!\n", fileName.latin1());
	listFileName (fileName.latin1());
    }
  
    // createRecordId for this funcName
    int funcRecordId = createRecordId();

    // Create mapInfo structure for this funcName
    MapInfo *mapInfo = new MapInfo (funcType, funcRecordId, fileId, funcId);
    TG_checkAlloc(mapInfo);


    // Insert MapInfo into funcIdTable and RecordIdTable
    funcIdTable.addEntry(funcId, mapInfo);
    recordIdTable.addEntry(funcRecordId, mapInfo);

    // Find recordId for fileId
    MapInfo *fileInfo = fileIdTable.findEntry(fileId);
    if (fileInfo == NULL)
	TG_error ("TreeView::listFunctionName: error finding fileInfo!");
    
    // Make funcRecordId last child of file's recordId
    int belowRecordId = grid->recordIdLastChild (fileInfo->recordId);
    grid->placeRecordId (funcRecordId, belowRecordId, fileInfo->recordId);

    // Get the clean name for the func
    const char *cleanFuncName = cleanName (funcName);

    // Put func name in new recordId in source Attribute
    grid->setCellText (funcRecordId, sourceAttrId, cleanFuncName);
    grid->setCellStyle (funcRecordId, sourceAttrId, funcStyle);

    // Set guestimate for line numbers in line column for function
    QString lineRange;
    int startLine = um->functionStartLine(funcName);
    int endLine = um->functionEndLine(funcName);
    if ((startLine != NULL_INT) && (endLine != NULL_INT))
	lineRange.sprintf("(%3d-%3d):", startLine, endLine);
    else
	lineRange.sprintf("(no lineNo info)");
    grid->setCellText(funcRecordId, lineAttrId, lineRange);
    grid->setCellStyle (funcRecordId, lineAttrId, funcStyle);    

    // Make this row expandable (will cause to parse function)
    grid->setTreeExpandable (funcRecordId, TRUE); 

    // Set to closed state, don't do anything until opened
    grid->setTreeOpen (funcRecordId, FALSE);

    // Scan all data attributes for the function rollup
    int numDataAttrs = um->dataAttrCount();
    for (int dataAttrIndex =0; dataAttrIndex < numDataAttrs;
	 dataAttrIndex++)
    {
	QString dataTag = um->dataAttrAt(dataAttrIndex);
	updateFunctionRollup (funcName, dataTag);
    }
}

// Updates the rollup display for function and dataAttrTag
void TreeView::updateFunctionRollup (const char *funcName, 
				     const char *dataAttrTag)
{
    // Get funcId for funcName and make sure valid
    int funcId = um->functionIndex(funcName);
    if (funcId == NULL_INT)
    {
        TG_error ("TreeView::updateFunctionRollup: Error func '%s' not found!",
		  funcName);
    }

    // Get MapInfo for this function
    MapInfo *funcInfo = funcIdTable.findEntry(funcId);

    // Do nothing if viewer not displaying function
    if (funcInfo == NULL)
    {
#if 0
	// DEBUG
	printf ("Not displaying function '%s'\n", funcName);
#endif

	return;
    }

    // Get recordId from funcInfo
    int recordId = funcInfo->recordId;

    // Get attrId from dataAttrTag
    int dataTagIndex = um->dataAttrIndex(dataAttrTag);
    int attrId = dataStartAttrId + dataTagIndex;

    // Get dataInfo for this dataTagIndex
    DataInfo *dataInfo = dataInfoTable.findEntry (dataTagIndex);
    if (dataInfo == NULL)
	TG_error ("TreeView::updateFunctionRollup:: dataInfo not found!");


    // Use user settings for rollup 
    UIManager::AttrStat functionStat = dataInfo->columnStat;
    UIManager::EntryStat ofEntryStat = dataInfo->entryStat;

    // Get functionStat ofEntryStat for this function
    double actualRollup = um->functionDataStat (funcName, dataAttrTag,
						functionStat, ofEntryStat);

    // If comparing values, get and use diff value for this rollup
    if (diffUm != NULL)
    {
	// Get functionStat ofEntrystat for the function being compared to
	double diffRollup = diffUm->functionDataStat (funcName, dataAttrTag,
						      functionStat, 
						      ofEntryStat);

	// Only display something if have a value!
	if ((actualRollup != NULL_DOUBLE) || (diffRollup != NULL_DOUBLE))
	{
	    // Set NULL_DOUBLE values to 0.0 for diff
	    if (actualRollup == NULL_DOUBLE)
		actualRollup = 0.0;
	    if (diffRollup == NULL_DOUBLE)
		diffRollup = 0.0;

	    // Actually write double now for values 
	    grid->setCellDouble (recordId, attrId, actualRollup - diffRollup);

#if 0
	    QString message;
	    message.sprintf ("%g", actualRollup - diffRollup);
	    grid->setCellText (recordId, attrId, message);
#endif

	    // Set to diffed Style, if not already
	    grid->setCellStyle (recordId, attrId, diffedStyle);
	}
    }

    // Otherwise, if not diffing value
    else
    {
	// Display actual rollup value, if have one
	if (actualRollup != NULL_DOUBLE)
	{
	    // Write double value directly to cell now
	    grid->setCellDouble (recordId, attrId, actualRollup);	   
#if 0
	    QString message;
	    message.sprintf ("%g", actualRollup);
	    grid->setCellText (recordId, attrId, message);	    
#endif

	    // Use function style to show that it is function rollup
	    grid->setCellStyle (recordId, attrId, funcStyle);

	}
	// Otherwise, clear cell of everything if no data, 
	// if not already cleared
	else
	{
	    // Only call setCellStyle on diffs since cellStyle creates cells.
	    if (grid->cellStyle(recordId, attrId) != CellStyleInvalid)
		grid->resetCell (recordId, attrId);
	}
    }

    // If the statusBar is displaying this functionRollupCell, update it
    if ((recordId == statusLastRecordId) &&
	(attrId == statusLastAttrId))
    {
	updateStatusBarStats (recordId, attrId);

	// Actually update the statusBar with any changes
	updateStatusMessage();
    }
}

// Updates the rollup display for file and dataAttrTag
void TreeView::updateFileRollup (const char *fileName, 
				 const char *dataAttrTag)
{
    // Get fileId for fileName and make sure valid
    int fileId = um->fileIndex(fileName);
    if (fileId == NULL_INT)
    {
        TG_error ("TreeView::updateFileRollup: Error file '%s' not found!",
		  fileName);
    }

    // Get MapInfo for this file
    MapInfo *fileInfo = fileIdTable.findEntry(fileId);

    // Do nothing if viewer not displaying file
    if (fileInfo == NULL)
    {
#if 0
	// DEBUG
	printf ("Not displaying file '%s'\n", fileName);
#endif

	return;
    }

    // Get recordId from fileInfo
    int recordId = fileInfo->recordId;

    // Get attrId from dataAttrTag
    int dataTagIndex = um->dataAttrIndex(dataAttrTag);
    int attrId = dataStartAttrId + dataTagIndex;

    // Get dataInfo for this dataTagIndex
    DataInfo *dataInfo = dataInfoTable.findEntry (dataTagIndex);
    if (dataInfo == NULL)
	TG_error ("TreeView::updateFileRollup:: dataInfo not found!");


    // Use user settings for rollup 
    UIManager::AttrStat fileStat = dataInfo->columnStat;
    UIManager::EntryStat ofEntryStat = dataInfo->entryStat;

    // Get fileStat ofEntryStat for this file
    double actualRollup = um->fileDataStat (fileName, dataAttrTag,
					    fileStat, ofEntryStat);

    // If comparing values, get and use diff value for this rollup
    if (diffUm != NULL)
    {
	// Get fileStat ofEntrystat for the file being compared to
	double diffRollup = diffUm->fileDataStat (fileName, dataAttrTag,
						  fileStat, ofEntryStat);

	// Only display something if have a value!
	if ((actualRollup != NULL_DOUBLE) || (diffRollup != NULL_DOUBLE))
	{
	    // Set NULL_DOUBLE values to 0.0 for diff
	    if (actualRollup == NULL_DOUBLE)
		actualRollup = 0.0;
	    if (diffRollup == NULL_DOUBLE)
		diffRollup = 0.0;

	    // Set cell value directly to double now
	    grid->setCellDouble (recordId, attrId, actualRollup - diffRollup);
#if 0
	    QString message;
	    message.sprintf ("%g", actualRollup - diffRollup);
	    grid->setCellText (recordId, attrId, message);
#endif

	    // Set to diffed Style, if not already
	    grid->setCellStyle (recordId, attrId, diffedStyle);
	}
    }

    // Otherwise, if not diffing value
    else
    {
	// Display actual rollup value, if have one
	if (actualRollup != NULL_DOUBLE)
	{
	    // Set cell value directly to double now
	    grid->setCellDouble (recordId, attrId, actualRollup);

#if 0
	    QString message;
	    message.sprintf ("%g", actualRollup);
	    grid->setCellText (recordId, attrId, message);	    
#endif

	    // Use file style to show that it is file rollup
	    grid->setCellStyle (recordId, attrId, fileStyle);

	}
	// Otherwise, clear cell of everything if no data, 
	// if not already cleared
	else
	{
	    // Only call setCellStyle on diffs since cellStyle creates cells.
	    if (grid->cellStyle(recordId, attrId) != CellStyleInvalid)
		grid->resetCell (recordId, attrId);
	}
    }

    // If the statusBar is displaying this fileRollupCell, update it
    if ((recordId == statusLastRecordId) &&
	(attrId == statusLastAttrId))
    {
	updateStatusBarStats (recordId, attrId);

	// Actually update the statusBar with any changes
	updateStatusMessage();
    }
}

// Updates the application rollup display for dataAttrTag
void TreeView::updateAppRollup (const char *dataAttrTag)
{
    // Get recordId (rename for ease of use)
    int recordId = programRecordId;

    // Get attrId from dataAttrTag
    int dataTagIndex = um->dataAttrIndex(dataAttrTag);
    int attrId = dataStartAttrId + dataTagIndex;

    // Get dataInfo for this dataTagIndex
    DataInfo *dataInfo = dataInfoTable.findEntry (dataTagIndex);
    if (dataInfo == NULL)
	TG_error ("TreeView::updateAppRollup:: dataInfo not found!");


    // Use user settings for rollup 
    UIManager::AttrStat appStat = dataInfo->columnStat;
    UIManager::EntryStat ofEntryStat = dataInfo->entryStat;

    // Get appStat ofEntryStat for this app
    double actualRollup = um->applicationDataStat (dataAttrTag, appStat, 
						   ofEntryStat);

    // If comparing values, get and use diff value for this rollup
    if (diffUm != NULL)
    {
	// Get appStat ofEntrystat for the app being compared to
	double diffRollup = diffUm->applicationDataStat ( dataAttrTag, 
							  appStat, 
							  ofEntryStat);

	// Only display something if have a value!
	if ((actualRollup != NULL_DOUBLE) || (diffRollup != NULL_DOUBLE))
	{
	    // Set NULL_DOUBLE values to 0.0 for diff
	    if (actualRollup == NULL_DOUBLE)
		actualRollup = 0.0;
	    if (diffRollup == NULL_DOUBLE)
		diffRollup = 0.0;

	    // Set cell value directly to double now
	    grid->setCellDouble (recordId, attrId, actualRollup - diffRollup);

#if 0
	    QString message;
	    message.sprintf ("%g", actualRollup - diffRollup);
	    grid->setCellText (recordId, attrId, message);
#endif

	    // Set to diffed Style, if not already
	    grid->setCellStyle (recordId, attrId, diffedStyle);
	}
    }

    // Otherwise, if not diffing value
    else
    {
	// Display actual rollup value, if have one
	if (actualRollup != NULL_DOUBLE)
	{
	    // Set cell value directly to double now
	    grid->setCellDouble (recordId, attrId, actualRollup);

#if 0
	    QString message;
	    message.sprintf ("%g", actualRollup);
	    grid->setCellText (recordId, attrId, message);	    
#endif

	    // Use program style to show that it is app rollup
	    grid->setCellStyle (recordId, attrId, programStyle);

	}
	// Otherwise, clear cell of everything if no data, 
	// if not already cleared
	else
	{
	    // Only call setCellStyle on diffs since cellStyle creates cells.
	    if (grid->cellStyle(recordId, attrId) != CellStyleInvalid)
		grid->resetCell (recordId, attrId);
	}
    }

    // If the statusBar is displaying this appRollupCell, update it
    if ((recordId == statusLastRecordId) &&
	(attrId == statusLastAttrId))
    {
	updateStatusBarStats (recordId, attrId);

	// Actually update the statusBar with any changes
	updateStatusMessage();
    }
}

// Adds source to display for this function, if possible
// Does nothing if source already exists for all lines of the function
void TreeView::listFunctionSource (const char *funcName)
{
    // Get funcId for funcName and make sure valid
    int funcId = um->functionIndex(funcName);
    if (funcId == NULL_INT)
    {
        TG_error ("TreeView::listFunctionSouce: Error func '%s' not found!",
		  funcName);
    }

    // Get MapInfo for this function
    MapInfo *funcInfo = funcIdTable.findEntry(funcId);
    if (funcInfo == NULL)
	TG_error ("TreeView::listFunctionSource: error finding funcInfo!");

    // Get start and end line for function
    int startLine = um->functionStartLine(funcName);
    int endLine = um->functionEndLine(funcName);


    // Do nothing, if no source info available
    if ((startLine == NULL_INT) || (endLine == NULL_INT))
	return;

    // Get file assocated with function
    QString fileName = um->fileAt(funcInfo->fileId);

    // Want all the source in file associated with some function,
    // move startLine up until hit a line mapped to another function
    // or hit line 1.
    while ((startLine > 1) && 
	   (um->fileFunctionAtLine(fileName, startLine-1) == NULL_QSTRING))
    {
	// Move up startLine, assocating this line with this function
	startLine--;
    }
   
    // Are there any functions that start after this one?
    bool funcAfter = FALSE;
    int funcCount = um->fileFunctionCount (fileName);
    for (int index = funcCount-1; index >= 0; index--)
    {
	QString scanFuncName = um->fileFunctionAt (fileName, index);
	int scanStartLine = um->functionStartLine(scanFuncName);
	// If this function starts after funcName, there is a func after
	// this one in the file
	if ((scanStartLine != NULL_INT) && (scanStartLine > endLine))
	{
	    funcAfter = TRUE;
	    break;
	}
    }

    // If there is no file after, move endLine to last line in file
    if (!funcAfter)
    {
	// Get the clean name for the file
	const char *cleanFileName = cleanName (fileName);

	// Get last line for this file (may return 0, no source available)
	int lastLine = um->getSourceSize (cleanFileName);

	// If after endLine, increase it
	if (lastLine > endLine)
	    endLine = lastLine;
    }


    // Update line numbers in line column for function
    QString lineRange;
    lineRange.sprintf("(%3d-%3d):", startLine, endLine);
    grid->setCellText(funcInfo->recordId, lineAttrId, lineRange);
    grid->setCellStyle (funcInfo->recordId, lineAttrId, funcStyle);

    // Place source lines in function in source line order
    int belowRecordId = NULL_INT;

    // See if all the source lines are mapped
    for (int lineNo = startLine; lineNo <= endLine; lineNo++)
    {
	// Create lineInfo entry if doesn't already exist
	MapInfo *lineInfo = NULL;

	// Is a record already created for this line?
	if (lineNoTable.entryExists(funcInfo->fileId, lineNo))
	{
	    // Yes, just get lineInfo entry
	    lineInfo = lineNoTable.findEntry(funcInfo->fileId, lineNo);

	    // If currently placed for different functionId, move
	    // it to this function id!  This theoretically can 
	    // happen when user requests source before dpcl has parsed
	    // all the functions in the file.  We are seeing the functions
	    // being returned out of order, so a function below this one
	    // may be placed (and source listed) before this call
	    if (lineInfo->funcId != funcId)
	    {
		printf ("***TreeView::listFunctionSource: stealing source "
			"line %i from funcId %i for %s!\n",
			lineNo, lineInfo->funcId, funcName);

		// place it after 'placeAfter' in the function's tree
		grid->placeRecordId (lineInfo->recordId, belowRecordId,
				     funcInfo->recordId);

		// Change it's funcId mapping
		lineInfo->funcId = funcId;
	    }

	    // DEBUG, STOP FIRST TIME THIS HAPPENS!
//	    TG_error ("Record already created for line %i!", lineNo);

	}
	// Otherwise, create record Id and lineInfo entry for this line
	else
	{
	    // create recordId for this lineNo
	    int lineRecordId = createRecordId();

	    // place it after 'placeAfter' in the function's tree
	    grid->placeRecordId (lineRecordId, belowRecordId,
				 funcInfo->recordId);

	    // Create new lineInfo for this lineNo
	    lineInfo = new MapInfo (lineType, lineRecordId,
				    funcInfo->fileId, funcId, NULL_INT,
				    lineNo);
	    TG_checkAlloc(lineInfo);

	    // Add to lineNoTable and recordIdTable
	    lineNoTable.addEntry(funcInfo->fileId, lineNo, lineInfo);
	    recordIdTable.addEntry(lineRecordId, lineInfo);
	}

	// Paint the contents of the source line with the current state
	paintSourceLine (lineInfo);

	// For any entries on this line, paint their data (if any)
	int entryCount = um->entryCount(funcName, lineNo);
	int dataCount = um->dataAttrCount();
	if ((entryCount != NULL_INT) && (dataCount > 0))
	{
	    // Loop though all entries on this line, painting any data
	    for (int lineIndex = 0; lineIndex < entryCount;  lineIndex ++)
	    {
		// Get the entryKey for this line & entry index
		QString entryKey = um->entryKeyAt (funcName, lineNo, 
						   lineIndex);

		// Loop though all data attrs looking for set data
		for (int dataIndex = 0; dataIndex < dataCount; dataIndex++)
		{
		    // Get dataAttr at this index
		    QString dataAttrTag = um->dataAttrAt (dataIndex);

		    // Update dataCell only if data set for um or diffUm
		    // use char * in entryDataStat for efficiency
		    if ((um->entryDataStat (funcName, entryKey.latin1(),
					    dataAttrTag.latin1(),
					    UIManager::EntryCount) > 0) || 
			((diffUm != NULL) && 
			 (diffUm->entryDataStat (funcName, entryKey.latin1(),
						 dataAttrTag.latin1(),
						 UIManager::EntryCount) > 0)))
		    {
			updateDataCell(funcName, entryKey, dataAttrTag);
		    }
		}
	    }
	}
	

	// The next line should go after this line
	belowRecordId = lineInfo->recordId;
    }
}

// Paints the contents of the source line with the current state
void TreeView::paintSourceLine (MapInfo *lineInfo)
{
    // Punt if not a lineType type
    if (lineInfo->type != lineType)
    {
	TG_error ("TreeView::paintSourceLine: not of lineType!");
    }

    // Get fileName function is in
    QString fileName = um->fileAt(lineInfo->fileId);

    // Get the clean file name for this file (use outside of um)
    const char *cleanFileName = cleanName (fileName.latin1());

    // Get the source line text, if exists
    QString line = um->getSourceLine (cleanFileName, lineInfo->lineNo);

    // For now, only handle case where have source info
    if (line == NULL_QSTRING)
    {
	// Get the function associated with this line
	QString funcName = um->fileFunctionAtLine (fileName, lineInfo->lineNo);

	// Start out with empty string
	line = "";

	// If no function associated, print some indicator that have no 
	// source or info for this line
	if (funcName == NULL_QSTRING)
	{
#if 0
	    // DEBUG
	    printf ("No func name associated with %s line %i\n",
		    fileName.latin1(), lineInfo->lineNo);
#endif
	    line = "...";

	    // Hide this line for now
	    grid->hideRecordId(lineInfo->recordId);
	}
	// Otherwise, construct line from any entry info for this line
	else
	{
	    bool needSeparator = TRUE;

	    // Get the number of entries associated with this line
	    int entryCount = um->entryCount (funcName, lineInfo->lineNo);
	    if (entryCount == NULL_INT)
	    {
		line = "...";
#if 0
		// DEBUG
		printf ("Hiding %s line %i, no entries\n",
			funcName.latin1(), lineInfo->lineNo);
#endif
		// Hide this line for now
		grid->hideRecordId(lineInfo->recordId);
	    }
	    else
	    {
		// Make sure this line is visible.  May have hid
		// earlier if no entries associated with this line
		grid->unhideRecordId(lineInfo->recordId);

#if 0
	    // DEBUG
		printf ("%i entries associated with %s line %i (%s)\n",
			entryCount, fileName.latin1(), lineInfo->lineNo,
			funcName.latin1());
#endif
		// Loop thru the entries for this line
		for (int index = 0; index < entryCount; index++)
		{
		    // Get the entry at this index for this line
		    QString entryKey = um->entryKeyAt (funcName, 
						       lineInfo->lineNo,
						       index);
		    
		    TG_InstPtType type = um->entryType (funcName, entryKey);
		    switch (type)
		    {
		      case TG_IPT_function_entry:
		      {
			  QString buf;
			  // Get the clean func name 
			  const char *cleanFuncName = 
			      cleanName (funcName.latin1());
			  buf.sprintf ("[Entry: %s] ", cleanFuncName);
			  line.append(buf);
			  break;
		      }
		      
		      case TG_IPT_function_exit:
		      {
			  QString buf;
			  // Get the clean func name 
			  const char *cleanFuncName = 
			      cleanName (funcName.latin1());
			  buf.sprintf ("[Exit: %s] ", cleanFuncName);
			  line.append(buf);
			  break;
		      }
		      
		      
		      case TG_IPT_function_call:
		      {
			  // Only put of function call for first point
			  // Assumes have both before and after inst point
			  TG_InstPtLocation loc = um->entryLocation (funcName,
								     entryKey);
			  if (loc == TG_IPL_before)
			  {
#if 1
			      if (needSeparator)
				  line.append("... ");
#endif
			      needSeparator = FALSE;
				  
			      QString funcCalled = 
				  um->entryFuncCalled (funcName, entryKey);
	
#if 1
#if 0
			      printf ("line %i index %i got %s to add to %s\n",
				      lineInfo->lineNo, index, 
				      funcCalled.latin1(),  line.latin1());
#endif
			      line.append(funcCalled);
			      line.append("() ... ");
#else
			      line.append("[Call: ");
			      line.append(funcCalled);
			      line.append("] ");
#endif
			  }
			  break;
		      }
		      
		      
		      default:
			TG_error ("TreeView::paintSourceLine: Unknown "
				  "TG_InstPtType %i\n", type);
			break;
		    }
		}
	    }
	}
    }
    // Otherwise, have source line
    else
    {
	// Make sure this line is visible.  May have hid
	// earlier if no entries associated with this line
	// and didn't have source before
	grid->unhideRecordId(lineInfo->recordId);
    }
    
    // Clear existing contents of cell
    grid->resetCell(lineInfo->recordId, sourceAttrId);

    // Put source line into cell
    grid->setCellText (lineInfo->recordId, sourceAttrId, line);

    // Write line numbers into line column for now
    QString lineRange;
    lineRange.sprintf("%4d:", lineInfo->lineNo);
    grid->setCellText(lineInfo->recordId, lineAttrId, lineRange);

    // Get function name
    QString funcName = um->functionAt(lineInfo->funcId);
    

#ifdef LOCAL_ONLY
    // DEBUG POPUP MENU, give something to click on
    printf ("Adding pixmap to recordId %i\n", lineInfo->recordId);
    grid->addCellPixmapAnnot (lineInfo->recordId, 
			      sourceAttrId,
			      1,  /* handle (encodes entry)*/ 
			      FALSE,       /* shared handle for entry */ 
			      1, /* layer (encodes actionIndex)*/
			      TRUE,      /* clickable */ 
			      3,  /* index */
			      -1,  /* slot */
			      SlotHCenter|SlotVCenter,
			      actionMenuPixmap);
#endif

    // Get number of entries for this line
    int entryCount = um->entryCount (funcName, lineInfo->lineNo);

    // Return now, if no entries for this line
    if ((entryCount == NULL_INT) || (entryCount < 1))
	return;

    // Get number of actions declared
    int actionCount = um->actionCount();

    // Return now, if no actions declared
    if ((actionCount < 1) || (actionCount == NULL_INT))
	return;

    // COME BACK, for now, just look at first PTPair's info
    int taskId = um->PTPairTaskAt(0);
    int threadId = um->PTPairThreadAt(0);

    // Go thru each entry looking for opportunties to annotate
    for (int entryIndex = 0; entryIndex < entryCount; entryIndex++)
    {
	// Get the key at this index
	QString entryKey = um->entryKeyAt (funcName, lineInfo->lineNo, 
					   entryIndex);

	// Sanity check
	if (entryKey == NULL_QSTRING)
	    TG_error ("TreeView::paintSourceLine: entryKey NULL!");

	// Are any actions enabled for this entry point?
	bool actionEnabled = FALSE;
	bool actionActivated = FALSE;
	for (int actionIndex=0; actionIndex < actionCount; actionIndex++)
	{
	    QString actionTag = um->actionAt (actionIndex);

	    // Sanity check
	    if (actionTag == NULL_QSTRING)
		TG_error ("TreeView::paintSourceLine: actionTag NULL!");

	    // If one or more is enabled, set flag 
	    if (um->isActionEnabled(funcName, entryKey, actionTag))
	    {
		actionEnabled = TRUE;
	    }

	    // If one or more is activated, set flag
	    if (um->isActionActivated(funcName, entryKey, actionTag,
				      taskId, threadId))
	    {
		actionActivated = TRUE;
	    }
	}
	
	// If nothing to annotate for this entry, goto next one
	if (!actionEnabled && !actionActivated)
	    continue;

	// Calculate index into line that this entry should be placed
	// to the left of.  That is, all slots should be negative.
	int pixmapIndex = calcEntryIndex (funcName, entryKey, line,
					  lineInfo->lineNo);

	// Give big negative slot so can put activated actions
	// on either side of menu slot and still be negative
	// Pick negative 1 billion, since activation order ids are between
	// 1 and 1 billion.
	int menuSlot = -1000000000;

	// If there is one or more actions enabled, place menu pixmap
	// at entry location
	if (actionEnabled)
	{
	    // Get the entry index into function, verses line for menu
	    int myEntryIndex = um->entryIndex(funcName, entryKey);
	    grid->addCellPixmapAnnot (lineInfo->recordId, sourceAttrId,
				      myEntryIndex,/*handle (encodes entry)*/ 
				      
				      FALSE,     /* shared handle for entry */ 
				      -1,        /* layer (-1 indicates menu)*/
				      TRUE,      /* clickable */ 
				      pixmapIndex, 
				      menuSlot,  /* slot */
				      SlotHCenter|SlotVCenter,
				      actionMenuPixmap);
	}

	// If there is is one or more actions activated, place
	// action pixmap at entry location
	if (actionActivated)
	{
	    // Determine placement based on type and location
	    // before menu for entry and before func call
	    bool beforeMenu = TRUE;
	    
	    // Always put before menu now, since menu logically adds
	    // items from left to right, and the menu should stay to
	    // the right.
#if 0
	    // Get the type and location of instrumentation point
	    TG_InstPtType type = um->entryType (funcName, entryKey);
	    TG_InstPtLocation location = um->entryLocation (funcName, 
							    entryKey);

	    if ((type == TG_IPT_function_exit) || 
		((type == TG_IPT_function_call) &&
		 (location == TG_IPL_after)))
	    {
		beforeMenu = FALSE;
	    }
#endif
	    
	    // Scan actions, drawing pixmaps based on current state
	    for (int actionIndex=0; actionIndex < actionCount; actionIndex++)
	    {
		QString actionTag = um->actionAt (actionIndex);
		
		// Sanity check
		if (actionTag == NULL_QSTRING)
		    TG_error ("TreeView::paintSourceLine: actionTag NULL!");
		
		// Now action activation order is kept, use it
		unsigned int orderId = 0;

		// Get the current action state
		// DEBUG FOR NOW, activated maps to "In" and 
		// deactivated maps to "Out".
		QString stateTag;

		// If one or more is activated, set flag
		if ((orderId = um->isActionActivated(funcName, entryKey, 
						     actionTag, taskId, 
						     threadId)) != 0)
		{
		    stateTag = "In";
		}
		else
		{
		    stateTag = "Out";
		}

		// Get pixmap for current state
		QString pixmapTag = um->actionStateInPixmapTag(actionTag,
							       stateTag);

		// If don't have pixmap, continue to next action
		if (pixmapTag.isEmpty())
		    continue;

		// Place before or after menu as appropriate
		int actionSlot;
		
		if (beforeMenu)
		    actionSlot = menuSlot - (1000000000-orderId);
		else
		    actionSlot = menuSlot + (1000000000-orderId);
		
		// HACK
		// Get the entry index into function, verses line for menu
		int myEntryIndex = um->entryIndex(funcName, entryKey);
		
		grid->addCellPixmapAnnot (lineInfo->recordId, 
  		          sourceAttrId,
			  myEntryIndex,  /* handle (encodes entry)*/ 
			  FALSE,       /* shared handle for entry */ 
			  actionIndex, /* layer (encodes actionIndex)*/
  		          TRUE,      /* clickable */ 
			  pixmapIndex, 
			  actionSlot,  /* slot */
			  SlotHCenter|SlotVCenter,
			  pixmapTag);
	    }
	}
    }

}


// Calculate index into line that this entry should be placed
// to the left of.  That is, all slots should be negative.
int TreeView::calcEntryIndex (const QString &funcName, 
			      const QString &entryKey, 
			      const QString &line,
			      int lineNo)
{
    // Get the type of instrumentation point
    TG_InstPtType type = um->entryType (funcName, entryKey);
    if (type != TG_IPT_function_call)
    {
	return (0);
    }

    // Get the call index in line (independent of name)
    int callIndex = um->entryCallIndex (funcName, entryKey);


    // Assume DPCL indexes function names are in same order
    // that appear in source line.  Skip over the function names
    // that occur before this function and find index of start
    // of the called function for entryKey.  Need to skip
    // over all function names to avoid problems with fprint, printf, etc.
    // where one name is a subset of another.
    int startIndex = -1;
    for (int callNo=1; callNo <= callIndex; callNo++)
    {
	// Get the function name at the call index 'callNo'
	QString funcCalled = getFuncCalledAt (funcName, lineNo, callNo);

	// Skip function's that are not found for callNo yet
	if (funcCalled.isNull())
	    continue;

	// Start search at one past current search location
	startIndex = line.find (funcCalled, startIndex+1, TRUE);
	if (startIndex < 0)
	    break;
    }

    // If could not find function name, return 0
    if (startIndex < 0)
    {
	QString funcCalled = um->entryFuncCalled (funcName, entryKey);
	printf ("%s line %i entry %s: \n"
		"  Could not find call '%s' callIndex %i in '%s'\n",
		funcName.latin1(), lineNo, entryKey.latin1(),
		funcCalled.latin1(), callIndex, line.latin1());

	int startIndex = -1;
	for (int callNo=1; callNo <= callIndex; callNo++)
	{
	    // Get the function name at the call index 'callNo'
	    QString funcCalled = getFuncCalledAt (funcName, lineNo, callNo);
	    
	    // Start search at one past current search location
	    startIndex = line.find (funcCalled, startIndex+1, TRUE);
	    printf ("   Call %i: found '%s' at %i in '%s'\n",
		    callNo, funcCalled.latin1(), startIndex, 
		    line.latin1());
	}

	return (0);
    }

    // Get the location call-type instrumentation point
    TG_InstPtLocation location = um->entryLocation (funcName, entryKey);

    // If entry before function, we have correct index now
    if (location == TG_IPL_before)
    {
	return (startIndex);
    }

    // Otherwise, entry after function, move to character just
    // after function name
    else
    {
	QString funcCalled = um->entryFuncCalled (funcName, entryKey);
	return (startIndex+funcCalled.length());
    }
}

// Returns function called at callIndex on the given line
// Punts if callIndex not found!  Used by calcEntryIndex.
QString TreeView::getFuncCalledAt (const QString &funcName,
				   int lineNo, int callIndex)
{
    int lineEntryCount = um->entryCount(funcName, lineNo);

    // March thru entries for this line until find one matching call index
    for (int lineEntryIndex = 0; lineEntryIndex < lineEntryCount;
                 lineEntryIndex++)
    {
        // Get entry at entryIndex on this line
        QString entryKey = um->entryKeyAt (funcName, lineNo,
                                           lineEntryIndex);

        // Get call index of this entry
        int call = um->entryCallIndex (funcName, entryKey);

        // If match call index, return funcCalled
        if (call == callIndex)
        {
            return (um->entryFuncCalled (funcName, entryKey));
        }
    }

    // Should not get here, but can happen when DPCL returns callIndexes
    // out of order (i.e., gives callIndex 2 before callIndex 1 sent!
    fprintf (stderr, "Warning TreeView::getFuncCalledAt: line %i call %i "
	     "not found in %i entries!\n", lineNo, callIndex, lineEntryCount);

    return (NULL_QSTRING);
}


// Listens for entries being inserted, calls listEntry
void TreeView::entryInserted (const char *funcName, const char *entryKey, 
			      int /*line*/, TG_InstPtType /*type*/, 
			      TG_InstPtLocation /*location*/,
			      const char */*funcCalled*/, int /*callIndex*/, 
			      const char */*toolTip*/)
{
    // Get funcId for funcName and make sure valid
    int funcId = um->functionIndex(funcName);
    if (funcId == NULL_INT)
    {
        TG_error ("Error func '%s' not found!",
		  funcName);
    }

    // Get entryId for this entry Key
    int entryId = um->entryIndex (funcName, entryKey);
    if (entryId == NULL_INT)
    {
        TG_error ("Error entry '%s' not found!",
		  entryKey);
    }

    // Store funcId, entryId pair in key array
    int entryKeyArray[2];
    entryKeyArray[0] = funcId;
    entryKeyArray[1] = entryId;

    // Add update to entryInsertTable, with 'NULL' data
    INT_ARRAY_add_symbol (entryInsertTable, entryKeyArray, 2, NULL);
}

// Displays entry (if no source) and calculates menu point placements
// for actions on this entry
void TreeView::listEntry (int funcId, int entryId)
{
    // If already listed, do nothing
    if (entryIdTable.entryExists(funcId, entryId))
	return;

    // Convert Ids to strings
    QString funcName = um->functionAt(funcId);
    QString entryKey = um->entryKeyAt (funcName, entryId);

    // Sanity check, make sure ids valid
    if (funcName == NULL_QSTRING)
    {
	TG_error ("funcId %i not found!\n", funcId);
    }
    if (entryKey == NULL_QSTRING)
    {
	TG_error ("entryId %i not found for function '%s'\n",
		  entryId, funcName.latin1());
    }

    // If function not already listed, list function
    // Should never happen, but just in case
    if (!funcIdTable.entryExists(funcId))
    {
	fprintf (stderr, "Warning: TreeView::listEntry: func '%s' "
		 "not already added!\n", funcName.latin1());
	listFunctionName (funcName.latin1());
    }

    // Get fileId for this function
    QString fileName = um->functionFileName(funcName);
    int fileId = um->fileIndex(fileName);
    if ((fileName == NULL_QSTRING) || (fileId == NULL_INT))
    {
	TG_error ("Error getting file for func %s", funcName.latin1());
    }
    
    // Get the line this entry is associated with
    int line = um->entryLine (funcName, entryKey);

    // Map all out of bounds line numbers to NULL_INT
    if ((line < 1) || (line > 100000000))
	line = NULL_INT;
    
    
#if 0
    // createRecordId for this entryKey
    int entryRecordId = createRecordId();

    // Create mapInfo structure for this entryKey
    MapInfo *mapInfo = new MapInfo (entryType, entryRecordId, fileId, funcId);
    TG_checkAlloc(mapInfo);

    // Insert MapInfo into entryIdTable and RecordIdTable
    entryIdTable.addEntry(entryId, mapInfo);
    recordIdTable.addEntry(entryRecordId, mapInfo);

    // Find recordId for fileId
    MapInfo *fileInfo = fileIdTable.findEntry(fileId);
    if (fileInfo == NULL)
	TG_error ("Error finding fileInfo!");
    
    // Make entryRecordId last child of file's recordId
    int belowRecordId = grid->recordIdLastChild (fileInfo->recordId);
    grid->placeRecordId (entryRecordId, belowRecordId, fileInfo->recordId);

    // Get the clean name for the entry
    const char *cleanFuncName = cleanName (entryKey);

    // Put entry name in new recordId in source Attribute
    grid->setCellText (entryRecordId, sourceAttrId, cleanFuncName);

    // Make this row expandable (will cause to parse entrytion)
    grid->setTreeExpandable (entryRecordId, TRUE); 

    // Set to closed state, don't do anything until opened
    grid->setTreeOpen (entryRecordId, FALSE);
    

#endif
}

// Listens for state changes to action's.
// Handles enabled/disabled/activated/deactivated signals
void TreeView::actionStateChanged (const char *funcName, const char *entryKey,
				   const char */*actionTag*/)
{
    // Get lineNo for this entry
    int lineNo = um->entryLine (funcName, entryKey);
    
    // If lineNo not present, or out of bounds, make up negative
    // line number based on entry index
    if ((lineNo < 1) || (lineNo == NULL_INT))
    {
	lineNo = -um->entryIndex (funcName, entryKey);
    }

    // Get fileId for this funcName's fileName
    QString fileName = um->functionFileName (funcName);
    int fileId = um->fileIndex (fileName);
    
    // Do we have this line in the display somewhere?
    MapInfo *lineInfo = lineNoTable.findEntry(fileId, lineNo);

    // If so, repaint it so it reflects new action state
    if (lineInfo != NULL)
    {
	// Get the funcId for the funcName
	int funcId = um->functionIndex (funcName);

	// Only repaint if this lineInfo is currently mapped to this
	// function.   Some tools mistakenly indicated the same line
	// for multiple functions, which may updates very slow
	if (lineInfo->funcId == funcId)
	{
	    paintSourceLine (lineInfo);
	}
#if 1
	// DEBUG
	else
	{
	    fprintf (stderr, "Warning: actionStateChanged for '%s' entry '%s' but line %i belongs to '%s'\n",
		     funcName, entryKey, lineNo, um->functionAt(lineInfo->funcId).latin1());
	}
#endif
	
    }
#if 0
    // DEBUG
    else
    {
	printf ("No line for %s (%i), %s %s line %i\n",
		funcName, fileId, entryKey, actionTag, lineNo);
    }
#endif
}

// Listens for action state changes for a particular task/thread.
// Mixed task/thread support not designed yet, so whine when called.
// Handles enabled/disabled/activated/deactivated signals
void TreeView::actionStateChanged (const char *funcName, const char *entryKey,
				   const char *actionTag,
				   int taskId, int threadId)
{
    // WARNING: Support for changing action state for particular tasks
    // not designed yet.  Warn that we are doing something hacked!
    printf ("Warning: actionStateChanged (%s, %s, %s, %i, %i) called!\n"
	    "         Thread/task specific action state changes only "
	    "partially supported!\n",
	    funcName, entryKey, actionTag, taskId, threadId);

    // Get lineNo for this entry
    int lineNo = um->entryLine (funcName, entryKey);
    
    // If lineNo not present, or out of bounds, make up negative
    // line number based on entry index
    if ((lineNo < 1) || (lineNo == NULL_INT))
    {
	lineNo = -um->entryIndex (funcName, entryKey);
    }

    // Get fileId for this funcName's fileName
    QString fileName = um->functionFileName (funcName);
    int fileId = um->fileIndex (fileName);
    
    // Do we have this line in the display somewhere?
    MapInfo *lineInfo = lineNoTable.findEntry(fileId, lineNo);

    // If so, repaint it so it reflects new action state
    if (lineInfo != NULL)
    {
	paintSourceLine (lineInfo);
    }
}

// Catch data changes and update cell grid
void TreeView::intSet (const char *funcName, const char *entryKey, 
		       const char *dataAttrTag, 
		       int /*taskId*/, int /*threadId*/, int /*value*/)
{
#if 1
    // Record that this data cell needs updating 
    recordDataCellUpdate(funcName, entryKey, dataAttrTag);
#else
    // Update the grid cell for this entry/data
    updateDataCell(funcName, entryKey, dataAttrTag);

    // Also update the function rollup display
    updateFunctionRollup (funcName, dataAttrTag);

    // Also update the function rollup display
    QString fileName = um->functionFileName (funcName);
    updateFileRollup (fileName, dataAttrTag);

    // Also update application rollup
    updateAppRollup (dataAttrTag);

    // Flush all outstanding visual changes to reduce redraw lag
    grid->flushUpdate();
#endif
}

// Catch data changes and update cell grid
void TreeView::doubleSet (const char *funcName, const char *entryKey, 
			  const char *dataAttrTag, 
			  int /*taskId*/, int /*threadId*/, double /*value*/)
{
#if 1
    // Record that this data cell needs updating 
    recordDataCellUpdate(funcName, entryKey, dataAttrTag);
#else
    // Update the grid cell for this entry/data
    updateDataCell(funcName, entryKey, dataAttrTag);

    // Also update the function rollup display
    updateFunctionRollup (funcName, dataAttrTag);

    // Also update the function rollup display
    QString fileName = um->functionFileName (funcName);
    updateFileRollup (fileName, dataAttrTag);

    // Also update application rollup
    updateAppRollup (dataAttrTag);

    // Flush all outstanding visual changes to reduce redraw lag
    grid->flushUpdate();
#endif
}

// Records that a data cell needs updating.  The cell's value is
// not actually updated until flushDataCellUpdates() is called.
// This allows parallel updates to a cell to be combined and
// updated on a regular interval (to prevent swamping the redraw routines).
void TreeView::recordDataCellUpdate (const QString &funcName, 
				     const QString &entryKey,  
				     const QString &dataTag)
{
    // Convert strings to ints for space-efficient recording
    int funcIndex = um->functionIndex(funcName);
    int entryIndex = um->entryIndex (funcName, entryKey);
    int dataIndex = um->dataAttrIndex (dataTag);

    // Pack into an cellKeyArray for the C table lookup 
    int cellKeyArray[3];
    cellKeyArray[0] = funcIndex;
    cellKeyArray[1] = entryIndex;
    cellKeyArray[2] = dataIndex;

    // Do nothing further if update for this cell is already pending
    if (INT_ARRAY_find_symbol (cellUpdateTable, cellKeyArray, 3) != NULL)
    {
	// Update already pending, nothing more needed
	return;
    }

    // Add update to cellUpdateTable, with 'NULL' data
    INT_ARRAY_add_symbol (cellUpdateTable, cellKeyArray, 3, NULL);

    // Also record the function touched, pack into array for lookup
    int funcKeyArray[2];
    funcKeyArray[0] = funcIndex;
    funcKeyArray[1] = dataIndex;

    // Do nothing further if update for this function is already pending
    if (INT_ARRAY_find_symbol (funcUpdateTable, funcKeyArray, 2) != NULL)
    {
	// Update already pending, nothing more needed
	return;
    }

    // Add update to funcUpdateTable, with 'NULL' data
    INT_ARRAY_add_symbol (funcUpdateTable, funcKeyArray, 2, NULL);

    // Also record the file touched, pack into array for lookup
    QString fileName = um->functionFileName (funcName);
    int fileIndex = um->fileIndex (fileName);
    int fileKeyArray[2];
    fileKeyArray[0] = fileIndex;
    fileKeyArray[1] = dataIndex;

    // Do nothing further if update for this fileis already pending
    if (INT_ARRAY_find_symbol (fileUpdateTable, fileKeyArray, 2) != NULL)
    {
	// Update already pending, nothing more needed
	return;
    }
    
    // Add update to fileUpdateTable, with 'NULL' data
    INT_ARRAY_add_symbol (fileUpdateTable, fileKeyArray, 2, NULL);

    // Also record the app dataTag touched, pack into array for lookup
    int appKeyArray[1];
    appKeyArray[0] = dataIndex;

    // Do nothing further if update for this app dataTag is already pending
    if (INT_ARRAY_find_symbol (appUpdateTable, appKeyArray, 1) != NULL)
    {
	// Update already pending, nothing more needed
	return;
    }
    
    // Add update to appUpdateTable, with 'NULL' data
    INT_ARRAY_add_symbol (appUpdateTable, appKeyArray, 1, NULL);
}

// Flushes all data cell updates (including rollups) recorded by
// recordDataCellUpdate().
void TreeView::flushDataCellUpdates ()
{
    // This routine uses the internals of INT_ARRAY_Symbol_Table, etc.
    // to do the updates efficiently (if in an ugly fashion).

    // Do nothing if no pending updates
    if (cellUpdateTable->symbol_count < 1)
	return;

    // Process all pending cell updates, deleting as we go
    INT_ARRAY_Symbol *cellSymbol;
    while ((cellSymbol = cellUpdateTable->head_symbol) != NULL)
    {
	// Parse cell key into indexes
	int funcIndex = cellSymbol->int_array[0];
	int entryIndex = cellSymbol->int_array[1];
	int dataIndex = cellSymbol->int_array[2];

	// Convert to QStrings
	QString funcName = um->functionAt (funcIndex);
	QString entryKey = um->entryKeyAt (funcName, entryIndex);
	QString dataTag = um->dataAttrAt(dataIndex);

	// Update contents for this cell
	updateDataCell (funcName, entryKey, dataTag);

	// Delete this symbol, making head_symbol the next one to process
	INT_ARRAY_delete_symbol (cellSymbol, NULL);
    }

    // Process all pending func updates, deleting as we go
    INT_ARRAY_Symbol *funcSymbol;
    while ((funcSymbol = funcUpdateTable->head_symbol) != NULL)
    {
	// Parse func key into indexes
	int funcIndex = funcSymbol->int_array[0];
	int dataIndex = funcSymbol->int_array[1];

	// Convert to QStrings
	QString funcName = um->functionAt (funcIndex);
	QString dataTag = um->dataAttrAt(dataIndex);

	// Update rollup for this func and dataTag
	updateFunctionRollup (funcName, dataTag);

	// Delete this symbol, making head_symbol the next one to process
	INT_ARRAY_delete_symbol (funcSymbol, NULL);
    }

    // Process all pending file updates, deleting as we go
    INT_ARRAY_Symbol *fileSymbol;
    while ((fileSymbol = fileUpdateTable->head_symbol) != NULL)
    {
	// Parse file key into indexes
	int fileIndex = fileSymbol->int_array[0];
	int dataIndex = fileSymbol->int_array[1];

	// Convert to QStrings
	QString fileName = um->fileAt (fileIndex);
	QString dataTag = um->dataAttrAt(dataIndex);

	// Update rollup for this file and dataTag
	updateFileRollup (fileName, dataTag);

	// Delete this symbol, making head_symbol the next one to process
	INT_ARRAY_delete_symbol (fileSymbol, NULL);
    }

    // Process all pending app updates, deleting as we go
    INT_ARRAY_Symbol *appSymbol;
    while ((appSymbol = appUpdateTable->head_symbol) != NULL)
    {
	// Parse app key into indexes
	int dataIndex = appSymbol->int_array[0];

	// Convert to QStrings
	QString dataTag = um->dataAttrAt(dataIndex);
	
	// Update app rollup for this dataTag
	updateAppRollup (dataTag);

	// Delete this symbol, making head_symbol the next one to process
	INT_ARRAY_delete_symbol (appSymbol, NULL);
    }

    // Flush all outstanding visual changes to reduce redraw lag
    grid->flushUpdate();
}

// Update contents for specified data cell
void TreeView::updateDataCell (const QString &funcName, 
				const QString &changedEntryKey,  
				const QString &dataTag)
{
    // Get fileName and fileId from funcName
    QString fileName = um->functionFileName (funcName);
    int fileId = um->fileIndex(fileName);
//    cout << "in updateDataCell " << funcName << " " << changedEntryKey
//	    << " "  << dataTag << endl;

    // Get the lineNo and recordId for the source line for the changedEntryKey
    int lineNo = um->entryLine (funcName, changedEntryKey);
    MapInfo *lineInfo = lineNoTable.findEntry(fileId, lineNo);

    // Do nothing if viewer not displaying cell
    if (lineInfo == NULL) 
	return;


    // Get attrId from dataTag
    int dataTagIndex = um->dataAttrIndex(dataTag);
    int attrId = dataStartAttrId + dataTagIndex;

    
    // First time updating data cell, add clickable annotation for
    // pulling up rollup menu for data.  This is not currently
    // strickly necessary, since viewer will handle and click on
    // data column, but may want to display something different
    // if actually click on data
    if (grid->cellType (lineInfo->recordId, attrId) == CellGrid::invalidType)
    {
	grid->addCellClickableAnnot (lineInfo->recordId, attrId,
				     dataMenuHandle,  /*handle*/ 
				     TRUE,/* Should be only data menu entry */
				     -2, /* layer (-2 indicates data menu)*/
				     /* Cover entire data field */
				     0, 0,
				     1000000, 0);
    }


    // Get dataInfo for this dataTagIndex
    DataInfo *dataInfo = dataInfoTable.findEntry (dataTagIndex);
    if (dataInfo == NULL)
	TG_error ("TreeView::updateDataCell:: dataInfo not found!");



    // Treat both ints and doubles as doubles to simply life.
    int setCount = 0;
    double sum = 0.0;
    double val = 0.0;
    double diffVal = 0.0;
    QString message;

    int lineEntryCount = um->entryCount(funcName, lineNo);

    // Sum up data for all entries on this line
    for (int lineEntryIndex = 0; lineEntryIndex < lineEntryCount;
	 lineEntryIndex++)
    {
	// Get entry at entryIndex on this line
	QString entryKey = um->entryKeyAt (funcName, lineNo,
					   lineEntryIndex);

	// Use stats function for this entry to get data to display
	
	// How many data points were set for this entry
	int count = (int) um->entryDataStat(funcName, entryKey, dataTag,
					    UIManager::EntryCount);

	// Use user setting for entryStat
	UIManager::EntryStat entryStat = dataInfo->entryStat;
#if 1
	val = um->entryDataStat (funcName, entryKey, dataTag, entryStat);
#else
	// Handle INT value (need NULL_INT->NULL_DOUBLE conversion)
	int intVal = um->getMaxTaskId (funcName, entryKey, dataTag);
	if (intVal != NULL_INT)
	    val = (double)intVal;
	else
	    val = NULL_DOUBLE;
#endif

		
	// If comparing values, get diff value for this PTPair
	if (diffUm != NULL)
	{
#if 1
	    diffVal = diffUm->entryDataStat (funcName, entryKey, dataTag,
					     entryStat);
#else
	    // Handle INT value (need NULL_INT->NULL_DOUBLE conversion)
	    int intDiffVal = 
		diffUm->getMaxTaskId (funcName, entryKey,  dataTag);
	    if (intDiffVal != NULL_INT)
		diffVal = (double)intDiffVal;
	    else
		diffVal = NULL_DOUBLE;
#endif
	}
	else
	{
	    diffVal = NULL_DOUBLE;
	}
	
	if ((val != NULL_DOUBLE) || 
	    (diffVal != NULL_DOUBLE))
	{
	    // Convert NULL_DOUBLE's to 0.0
	    if (val == NULL_DOUBLE)
		val = 0.0;
	    if (diffVal == NULL_DOUBLE)
		diffVal = 0.0;
	    
	    double diff = val - diffVal;
	    // Hide precision issues
	    if ((diff < 0.00000001) && 
		(diff > -0.00000001))
	    {
		diff = 0.0;
	    }
	    sum += diff;
	    setCount += count;
	}
    }

    // Now write double value directly to cell
#if 0
    // Let user know summed over several PTPairs
    if (setCount > 1)
    {
	// I don't like the (4), etc after the data
	// I need to figure out a better way to indicate this
//	message.sprintf ("%g (%i)", sum, setCount);
	message.sprintf ("%g", sum);
    }
    else
    {
	message.sprintf ("%g", sum);
    }
#endif

    // If diffing cells, print in red to let user know
    if (diffUm != NULL)
    {
	grid->setCellStyle (lineInfo->recordId, attrId, diffedStyle);
    }
    // Otherwise, use default color
    else
    {
	grid->setCellStyle (lineInfo->recordId, attrId, CellStyleInvalid);
    }

    // Print out calculated message
    // Set cell value to double directly now
    grid->setCellDouble(lineInfo->recordId, attrId, sum);

#if 0
    grid->setCellText(lineInfo->recordId, attrId, message);
#endif

    // If the statusBar is displaying this dataCell, update it
    if ((lineInfo->recordId == statusLastRecordId) &&
	(attrId == statusLastAttrId))
    {
	updateStatusBarStats (lineInfo->recordId, attrId);

	// Actually update the statusBar with any changes
	updateStatusMessage();
    }
}

// Removes all contents from dataCell.  Used when using comparisons
// (to clean up after comparison done or changed)
void TreeView::resetDataCell (const QString &funcName, 
				const QString &changedEntryKey,  
				const QString &dataTag)
{
    // Get fileName and fileId from funcName
    QString fileName = um->functionFileName (funcName);
    int fileId = um->fileIndex(fileName);

    // Get the lineNo and recordId for the source line for the changedEntryKey
    int lineNo = um->entryLine (funcName, changedEntryKey);
    MapInfo *lineInfo = lineNoTable.findEntry(fileId, lineNo);

    // Do nothing if viewer not displaying cell
    if (lineInfo == NULL) 
	return;

    // Get attrId from dataTag
    int dataTagIndex = um->dataAttrIndex(dataTag);
    int attrId = dataStartAttrId + dataTagIndex;

    // Make sure cellType is invalid, if it is not, reset it
    if (grid->cellType (lineInfo->recordId, attrId) != CellGrid::invalidType)
    {
	grid->resetCell (lineInfo->recordId, attrId);
    }
}


// Updates internal tables for each pixmap declared in UIManager
void TreeView::pixmapDeclaredHandler (const char *pixmapName, const char **/*xpm*/)
{
    // Sanity check, should not see same pixmapName multiple times
    if (pixmapInfoTable.findEntry (pixmapName) != NULL)
    {
	TG_error ("TreeView::pixmapDeclareHandler: Algorithm error, "
		  "pixmap '%s' declared muliple times!", 
		  pixmapName);
    }
    

    // For now, just declare pixmap in cellGrid (it will punt on duplicates)
    GridPixmapId id = grid->newGridPixmap (um->pixmapQPixmap(pixmapName), 
					   pixmapName);

    // Create PixmapInfo structure to hold id and iconSet for this pixmap
    PixmapInfo *pixmapInfo = new PixmapInfo (um->pixmapQPixmap(pixmapName),
					     id);
    TG_checkAlloc(pixmapInfo);

    // Add pixmapInfo to pixmapInfoTable for quick access
    pixmapInfoTable.addEntry (pixmapName, pixmapInfo);
}


// Expands tree if necessary and center on fileName and lineNo
void TreeView::showFileLine (const char *fileName, int lineNo, int attrId)
{
    // Clear search highlighting, if any, if going to a new location
#if 0
    clearSearchHighlighting();
#endif

    // Get FileId for this file, make sure valid
    int fileId = um->fileIndex (fileName);
    if (fileId == NULL_INT)
    {
	fprintf (stderr, "TreeView::showFileLine: Error file '%s' not found!",
		 fileName);

	// For now, don't exit, just ignore command
	return;
    }
    
    // If don't have MapInfo for this fileId, do nothing
    if (!fileIdTable.entryExists(fileId))
    {
	fprintf (stderr,  "TreeView::showFileLine: No MapInfo for file '%s'!",
		 fileName);
	return;
    }

    // Get the MapInfo for this fileName
    MapInfo *fileMapInfo = fileIdTable.findEntry (fileId);

    // Get the fileName's recordId
    int fileRecordId = fileMapInfo->recordId;

    // Set the file's state to open
    grid->setTreeOpen (fileRecordId, TRUE);

    // Get the function name at the line
    QString funcName = um->fileFunctionAtLine (fileName, lineNo);

    // For now, just print warning and return if lineNo not in any function 
    if (funcName.isEmpty())
    {
	fprintf (stderr,  
		 "TreeView::showFileLine: No function mapped to '%s' line %i!",
		 fileName, lineNo);
	return;
    }
    
    int funcId = um->functionIndex (funcName);
    if (fileId == NULL_INT)
    {
	fprintf (stderr, 
		 "TreeView::showFileLine: Error function '%s' not found!",
		 funcName.latin1());

	// For now, don't exit, just ignore command
	return;
    }

    // If don't have MapInfo for this funcId, do nothing
    if (!funcIdTable.entryExists(funcId))
    {
	fprintf (stderr,  "TreeView::showFileLine: No MapInfo for func '%s'!",
		 funcName.latin1());
	return;
    }

    // Get the MapInfo for this funcName
    MapInfo *funcMapInfo = funcIdTable.findEntry (funcId);

    // Get the funcName's recordId
    int funcRecordId = funcMapInfo->recordId;

    // Set the func's state to open
    grid->setTreeOpen (funcRecordId, TRUE);

    // List all function source (if necessary)
    listFunctionSource (funcName.latin1());

    // Does this line exist in the display?
    if (!lineNoTable.entryExists (fileId, lineNo))
    {
	fprintf (stderr,  
		 "TreeView::showFileLine: No recordId for line %i of %s!",
		 lineNo, funcName.latin1());
	return;
    }

    // Yes, get lineInfo
    MapInfo *lineInfo = lineNoTable.findEntry (fileId, lineNo);

    // Get lineRecordId
    int lineRecordId = lineInfo->recordId;

    // Cache highligh color for efficiency
    static QColor searchHighlightColor("yellow");

    // Get the text in the target cell
    QString lineText = grid->cellScreenText (lineRecordId, attrId);
    //int textLength = lineText.length();
    // Hightlight the text found
#if 0
    // FIX: do we need to reenable this?? jmm
    grid->addCellRectAnnot (lineRecordId, 
			    attrId, 
			    searchHighlightHandle,
			    FALSE, // No need to clear handle
			    -1,    // Layer, put behind text with -1
			    FALSE, // Not clickable 
			    // Start at top/left side of first char
			    0, 
			    0, 
			    SlotLeft|SlotTop, 
			    // Stop at bottom/right side of last char
			    textLength -1,
			    0,
			    SlotRight|SlotBottom,
			    // No line color
			    QColor(),
			    0,
			    // Only fill color for rectangle, don't put
			    // named color here for efficiency reasons
			    searchHighlightColor);

    // Record that have added annotation
    searchHighlightRecordId = lineRecordId;
    searchHighlightAttrId = attrId;
#endif

    // Scroll the cell into view
    grid->scrollCellIntoView (lineInfo->recordId, 0);
}

// Catch changes to the search path and redisplay the source
void TreeView::reloadSource()
{
    // Get the number of recordIds to scan
    int numRecordId = grid->recordIds();

    // Scan the grid looking for expanded functions (ones with source)
    for (int recordId = 0; recordId < numRecordId; recordId++)
    {
	// Do nothing if not an expanded recordId (file or func)
	if (!expandedRecordId.in(recordId))
	    continue;

	// Get map info for parent record Id
	MapInfo *mapInfo = recordIdTable.findEntry(recordId);
	    
	// Only repaint source for expanded functions
	if ((mapInfo != NULL) && (mapInfo->type == funcType))
	{
	    // Get funcName for this recordId
	    QString funcName = um->functionAt (mapInfo->funcId);

	    // Also list source again, if possible for this function
	    listFunctionSource (funcName);
	}
    }
}

void TreeView::resetFont( QFont& f )
{
	grid->setFont( f );
	// Redraw the grid
	grid->resizeGrid( grid->recordIds(),
			grid->attrIds() );
}



//#include "treeview.moc"
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

