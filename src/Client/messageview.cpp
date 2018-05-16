// messsageview.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** Widget MessageView, which builds a Message viewer window on
** top of a CellGrid window.
**
** Created by John Gyllenhaal 11/04/03
**
*****************************************************************************/

#include "messageview.h"
#include <qmainwindow.h>
#include <qapplication.h>
#include <stdio.h>
#include <stdlib.h>
#include "tg_time.h"
#include "tempcharbuf.h"
#include "qbitmap.h"

// Instantiate static object;
QTimer * MessageView::updateTimer = 0;

#define TIME_MESSAGE_VIEW 0

MessageView::MessageView(const char *progName, UIManager *m, 
			 CellGridSearcher * grid_searcher, QWidget *parent,
			 const char *name, 
			 bool /*viewingSnapshot*/) :
    QVBox (parent, name),
    
    // Assume top-level parent is a QMainWindow
    // No longer use statusBar in message viewer -JCG
//    status( ((QMainWindow *)(topLevelWidget()))->statusBar() ),

    // Get the UIManager pointer
    um (m),

    // Get the program name
    programName (progName),

    // Maps recordIds to messageIndexes, return -1 map on not found
    recordId2messageIndex ("recordId to MessageIndexes", -1),

    // Maps messageIndexes to recordIds, return -1 on not found
    messageIndex2recordId ("MessageIndexes to recordId", -1),

    // Maps comboBoxIndexes to folderIndexes, return -1 on not found
    comboBoxIndex2folderIndex ("comboBoxIndexes to folderIndex", -1),

    // Maps folderIndexes to comboBoxIndexes, return -1 on not found
    folderIndex2comboBoxIndex ("folderIndex to comboBoxIndex", -1),

    // Maps folderIndex to last displayed count, returns -1 on not found
    displayedCount ("folderIndex to displayedCount", -1),

    // Maps folderIndex to the state of the message folder
    messageFolderState ("folderIndex to MessageFolderState", DeleteData, 0)
{
#if TIME_MESSAGE_VIEW
    // DEBUG
    TG_timestamp ("Starting Messageviewer\n");
#endif

    messageGroupBox = new QGroupBox (this, "Message Folder Displayed");
    messageGroupBox->setColumns (1);
    messageGroupBox->setTitle ("Message Folder Displayed:");
    messageGroupBox->setInsideMargin(7);

    // Create pull down folder of message folders declared
    // Putting a FALSE first changes to Motif 2.0 style, which left
    // justifies the selected item and changes the selection stytle
    messageFolder = new QComboBox (FALSE, messageGroupBox, "Message Folder");

    // Try to increase the number of message lines allowed from the 
    // default 10 to 15 (after this limit, a scrollbar is inserted -JCG 11/7/05
    messageFolder->setSizeLimit(15);

#if 0
    // Set default font to fixed-width font
#if defined(TG_LINUX)
    QFont folderFont("courier", 10, QFont::Bold);
#elif defined(TG_MAC) 
    QFont folderFont("courier", 10);
#else
    QFont folderFont("courier", 8);
#endif
    // Not sure yet I want something this small
//    messageFolder->setFont (folderFont);
#endif

    // Create smallest grid, with all the attrIds we are going to get
    // Set min row height to 15, to make room for internal pixmaps
    messageGrid = new CellGrid (1, 1, m->getMainFont(),
		    15, this, "Message window");
    TG_checkAlloc(messageGrid);

    // Register the new cellgrid
    grid_searcher->addCellGrid( messageGrid );

    // Try to hide the header in the messageGrid
    messageGrid->setHeaderVisible (FALSE);

    // Initially, no messageFolder selected
    selectedMessageFolder = -1;

    // Initially, no message folder displayed
    displayedMessageFolder = "";
    displayedFolderState = NULL;

    // Initially, no messages are displayed
    displayedMessageCount = 0;

    // Initially, no lines are needed to display messages
    displayedLineCount = 0;

    // Initially, no lines are longest so mark as such with -1
    maxHeaderId = -1;
    maxHeaderLen = -1;
    maxBodyId = -1;
    maxBodyLen = -1;
    displayedMaxWidth = -1;

    // Initially assume messages have been added
    messagesAdded = TRUE;

    // Initially assume don't need to popup messageFolder list
    // (Set to TRUE when user selects disabled title, so they can try again)
    popupFolderSelection = FALSE;

    // Create style to display selected message with
    selectedMessageStyle = messageGrid->newCellStyle ("Message Selected");
    QColor whiteColor("white");
//    QColor blackColor("black");	// Not currently used
    QColor blueColor("blue");
//	QColorGroup cg = colorGroup();
//	QColor hlfgColor = cg.highlightedText();
//	QColor hlColor = cg.highlight();
    messageGrid->setStyleFgColor (selectedMessageStyle, whiteColor);
    messageGrid->setStyleBgColor (selectedMessageStyle, blueColor);
//	messageGrid->setStyleFgColor( selectedMessageStyle, hlfgColor );
//	messageGrid->setStyleBgColor( selectedMessageStyle, hlColor );

    headerStyle = messageGrid->newCellStyle ("Message Header");
    messageGrid->setStyleFgColor (headerStyle, blueColor);
//    messageGrid->setStyleFgColor (headerStyle, hlColor);

    // Install new messageFolder and message handlers
    connect (um, SIGNAL(messageFolderDeclared (const char *, const char *,
					       UIManager::PolicyIfEmpty)),
	     this, 
	     SLOT (messageFolderDeclaredHandler(const char *, const char *,
						UIManager::PolicyIfEmpty)));
    
    connect (um, 
	     SIGNAL(messageAdded(const char *, const char *, const char *)),
	     this, 
	     SLOT (messageAddedHandler(const char *, const char *, const char *)));

    connect (messageFolder, SIGNAL(activated (int)), 
	     this, SLOT(messageFolderSelectedHandler (int)));

    connect (messageGrid, 
	     SIGNAL(cellSlotClicked(int, int, ButtonState, int, int, int, 
				    int, int, int)),
	     this,
	     SLOT (messageClickedHandler(int, int, ButtonState,
					 int, int, int, int, int, int)));

    connect (messageGrid, 
	     SIGNAL(cellSlotDoubleClicked(int, int, ButtonState, int, int, 
					  int, int, int, int)),
	     this,
	     SLOT (messageDoubleClickedHandler(int, int, ButtonState,
					       int, int, int, int, int, int)));
    
    connect (messageGrid, 
	     SIGNAL(cellTreeClicked(int, int, ButtonState, int, int,
				    bool, int, int, int, int)),
             this, 
	     SLOT(treeClickHandler(int, int, ButtonState, int, int,
				   bool, int, int, int, int)));

    connect (messageGrid, 
	     SIGNAL(cellTreeDoubleClicked(int, int, ButtonState, int, int,
				    bool, int, int, int, int)),
             this, 
	     SLOT(treeDoubleClickHandler(int, int, ButtonState, int, int,
					 bool, int, int, int, int)));

    // Populate messageFolder, if there are no folders declared yet, 
    // indicate that
    int folderCount = um->messageFolderCount();

    // If no folders yet, update message display with no folder's info
    if (folderCount <= 0)
    {
	// Clear existing grid contents
	messageGrid->clearGridContents();

	// Resize grid so it has only one cell
	messageGrid->resizeGrid (1,1);
	
	// Put message that there are not folders currently
	messageGrid->setCellText (0,0, 
				  "(Message folders currently not available.)");

	// Use the above message to set display width
	maxHeaderId = 0;

	// Mark that display width needs to be recalculated
	displayedMaxWidth = -1;
    }
    else
    {
	// Otherwise, call declareFolderHandler for each folder
	for (int folderIndex = 0; folderIndex < folderCount; folderIndex++)
	{
	    QString folderTag = um->messageFolderAt (folderIndex);
	    QString folderTitle = um->messageFolderTitle (folderTag);
	    UIManager::PolicyIfEmpty folderIfEmpty = 
		um->messageFolderIfEmpty (folderTag);


	    messageFolderDeclaredHandler (folderTag.latin1(), 
					  folderTitle.latin1(),
					  folderIfEmpty);
	}
    }

    // Catch window resize events, so can adjust column width to
    // make everything look right
    connect (messageGrid, SIGNAL(gridResizeEvent(QResizeEvent *)),
	     this, SLOT(messageGridResized( QResizeEvent *)));

    // Catch window move events so we can record each folder's current position
    connect (messageGrid, SIGNAL(contentsMoving(int, int)),
	     this, SLOT(contentsMovingHandler (int, int)));


    // Create screen update timer, if it doesn't exist already
    // (we share one timer for all instance of this object)
    if( updateTimer == NULL ) {
	    updateTimer = new QTimer (NULL, "updateTimer");
    }
    
    // flush pending data updates every timer signal
    connect (updateTimer, SIGNAL(timeout()), 
	     this, SLOT(flushMessageUpdates()));

    // Respond to a signal from the main view to change fonts
    connect( um, SIGNAL( mainFontChanged(QFont&) ),
	     this, SLOT( resetFont(QFont&) ) );

    // Respond to a signal from the main view to change fonts
    connect( um, SIGNAL( labelFontChanged(QFont&) ),
	     this, SLOT( labelFontChanged(QFont&) ) );

    // Set size policy so that the treeview will try to expand
    // to fill all available space (versus grabbing half of it)
    setSizePolicy (QSizePolicy(QSizePolicy::Expanding, 
			       QSizePolicy::Expanding));

    // For now, update the screen every 200ms
    // Messages are not usually time-critical and are usually
    // added in bunches.  This helps X11 tunnelled thru ssh keep up.
    if( ! updateTimer->isActive() ) {
	    updateTimer->start(200);
    }
}

// Removes all memory used by MessageViewer
MessageView::~MessageView ()
{
//    delete messageGrid;
}

// Update message folder title (either create it or update it)
void MessageView::
updateMessageFolderTitle (const char *messageFolderTag)
{
    // Get the index of the messageFolder
    int messageFolderIndex = um->messageFolderIndex (messageFolderTag);

    // Get the number of messages in this folder currently
    int messageCount = um->messageCount (messageFolderTag);

    // Get how many currently displayed (-1 if not displayed)
    int currentCount = displayedCount.findEntry (messageFolderIndex);

    // Do nothing if message count the same
    if (messageCount == currentCount)
    {
	// Nothing to update
	return;
    }

    // Delete old count, if exists
    if (currentCount != -1)
	displayedCount.deleteEntry (messageFolderIndex);

    // Put in new count
    displayedCount.addEntry (messageFolderIndex, messageCount);

    // Get the display policy if empty
    UIManager::PolicyIfEmpty emptyPolicy = 
	um->messageFolderIfEmpty(messageFolderTag);

    // If it is empty and we are to hide it if empty, do nothing
    if ((messageCount == 0) && (emptyPolicy == UIManager::HideIfEmpty))
    {
	// Not displaying this empty folder right now
	return;
    }

    // Get the current selected FolderIndex before we potentially change the
    // combo box indexes
    int selectedFolderIndex = NULL_INT;
    int oldComboBoxIndex = -1;
    if (selectedMessageFolder >= 0)
    {
	// Get the message folder index from the currently selected combo box
	selectedFolderIndex = selectedMessageFolder;
	oldComboBoxIndex = 
	    folderIndex2comboBoxIndex.findEntry(selectedFolderIndex);
    }

    // If we have gotten here, we will be changing the combo box contents

    // Get the message folder title
    QString messageFolderTitle = um->messageFolderTitle (messageFolderTag);

    // Put number of messages in each title
    QString annotatedTitle;
    annotatedTitle.sprintf ("%s  [%i items]", messageFolderTitle.latin1(), 
			    messageCount);

    // Get the index in the combo box for this folder (-1 if not mapped)
    int comboBoxIndex = 
	folderIndex2comboBoxIndex.findEntry(messageFolderIndex);


    // Calculate appropriate comboBoxIndex for this folder, if not mapped
    if (comboBoxIndex == -1)
    {
	// Find the index of the previous folder that as a ComboBox index
	for (int scanIndex=messageFolderIndex-1; scanIndex >=0; --scanIndex)
	{
	    // Get the index of previous folders until find one set
	    int prevComboBoxIndex = 
		folderIndex2comboBoxIndex.findEntry(scanIndex);

	    // If set, we have found where we go
	    if (prevComboBoxIndex != -1)
	    {
		// Place this folder after the one we found
		comboBoxIndex = prevComboBoxIndex + 1;

		// Stop search
		break;
	    }
	}

	// If didn't find one before, make this the first entry
	if (comboBoxIndex == -1)
	{
	    comboBoxIndex = 0;
	}

	
	// We are going to insert an item in a comboBox at comboBoxIndex,
	// so all later folder entries will be incremented by one. 
	// Update the comboBoxIndex mapping for all the folders following
	// this one, or else will get punts from the addEntries below.
	// Must do in reverse order so don't have conflicting comboBox entries.
	int folderCount = um->messageFolderCount();
	for (int scanIndex=folderCount-1; scanIndex > messageFolderIndex;
	     --scanIndex)
	{
	    // Get the comboBox index of folders after this one
	    int scanComboBoxIndex = 
		folderIndex2comboBoxIndex.findEntry(scanIndex);

	    // If set, adjust mapping by one
	    if (scanComboBoxIndex != -1)
	    {
		// Undo the previous mapping
		folderIndex2comboBoxIndex.deleteEntry(scanIndex);
		comboBoxIndex2folderIndex.deleteEntry(scanComboBoxIndex);
		
		// Add mapping with comboBoxIndex incremented
		folderIndex2comboBoxIndex.addEntry(scanIndex, 
						   scanComboBoxIndex+1);
		comboBoxIndex2folderIndex.addEntry(scanComboBoxIndex+1,
						   scanIndex);
	    }
	}
	

	// Set up mapping between folderIndex and comboBoxIndex
	folderIndex2comboBoxIndex.addEntry(messageFolderIndex, comboBoxIndex);
	comboBoxIndex2folderIndex.addEntry(comboBoxIndex, messageFolderIndex);

	// DEBUG, insert title at indexes (may be set below later)
	messageFolder->insertItem (annotatedTitle, comboBoxIndex);
    }


    // If empty and marked as disableIfEmpty, use pixmap to generate
    // grayed out text
    if ((messageCount == 0) && (emptyPolicy == UIManager::DisableIfEmpty))
    {
	// Get font metrics for messageFolder font
	QFontMetrics fm(messageFolder->font());

	// Calculate size needed for string for pixmap
	int titleWidth = fm.width (annotatedTitle);
	int titleHeight = fm.height();
	int titleBaseline = fm.ascent();

	// Create pixmap of this size (may need to optimize creation of pixmap 
	// later) if becomes bottleneck
	QPixmap titlePixmap(titleWidth, titleHeight);
	QBitmap titleMask(titleWidth, titleHeight);

	// Create painter so we can write title in pixmap
	QPainter titlePainter(&titlePixmap);
	QPainter maskPainter(&titleMask);
	
	// Set the font we are using to draw text (in mask only)
	maskPainter.setFont(messageFolder->font());
	
	// Get the two mask colors for ease of use
	QColor mask0Color(color0);
	QColor mask1Color(color1);
	
	// Get disabled foreground color from messageFolder
	QColor disabledColor(messageFolder->palette().disabled().foreground());
	
	// Fill in background color for title using disabled color
	// The maskPainter below is actually used to write the text
	titlePainter.fillRect( 0, 0, titleWidth, titleHeight, disabledColor);
	
#if 0
	// DEBUG
	// Since we are using the mask to write the text, the text color 
	// can be wild.   Not useful except perhaps for debugging, 
	// but can make striped text. :)
	for (int i=0; i <= titleWidth; i+=7)
	{
	    titlePainter.fillRect( 0, i+0, titleWidth, 2,QColor(red));
	    titlePainter.fillRect( 0, i+1, titleWidth, 2,QColor(darkRed));
	    titlePainter.fillRect( 0, i+2, titleWidth, 2,QColor(darkGreen));
	    titlePainter.fillRect( 0, i+3, titleWidth, 2,QColor(blue));
	    titlePainter.fillRect( 0, i+4, titleWidth, 2,QColor(darkBlue));
	    titlePainter.fillRect( 0, i+5, titleWidth, 2,QColor(magenta));
	    titlePainter.fillRect( 0, i+6, titleWidth, 2,QColor(darkMagenta));
	}
#endif
	
	// Fill in mask to all 0 (all masked)
	maskPainter.fillRect( 0, 0, titleWidth, titleHeight, mask0Color);
	
	// Draw mask in mask1Color (this actually is going to write the text)
	maskPainter.setPen (mask1Color);
	maskPainter.drawText (0, titleBaseline, annotatedTitle);
			  
	// Make all but the 'text' transparent, so mask draws text in the
	// color stored in titlePixmap
	titlePixmap.setMask (titleMask);

	// Change item in combo box to grayed-out pixmap
	messageFolder->changeItem (titlePixmap, comboBoxIndex);

	// For now, disabled mode just grays out the title.  It still
	// can be select like a normal empty folder.
    }

    // Otherwise, if not "disabled", just change text of item to new title
    else
    {
	messageFolder->changeItem (annotatedTitle, comboBoxIndex);

	// If no messageFolder is currently selected, selecte this one
	if (selectedMessageFolder < 0)
	{
	    // Set current folder item, doesn't appear to trigger
	    // messageFolderSelectedHandler, so call that also
	    messageFolder->setCurrentItem (comboBoxIndex);
	    messageFolderSelectedHandler (comboBoxIndex);
	}
    }

    // See if we need to change the selectedMessageFolder
    if ((selectedFolderIndex != NULL_INT) && (selectedFolderIndex >= 0))
    {
	int newComboBoxIndex = 
	    folderIndex2comboBoxIndex.findEntry(selectedFolderIndex);
	if (newComboBoxIndex != oldComboBoxIndex)
	{
//	    fprintf (stderr, "Changing combo box from %i to %i due to %s\n",
//		     oldComboBoxIndex, newComboBoxIndex, messageFolderTag );
	    messageFolder->setCurrentItem (newComboBoxIndex);
	}
    }
}

// Handle declarations of new message Folders
void MessageView::
messageFolderDeclaredHandler (const char *messageFolderTag,
			      const char */*messageFolderTitle*/,
			      UIManager::PolicyIfEmpty /*ifEmpty*/)
{
    // Get the index of the messageFolder
    int messageFolderIndex = um->messageFolderIndex (messageFolderTag);

    // Create a MessageFolderState struct to hold folder state
    messageFolderState.addEntry (messageFolderIndex, new MessageFolderState);

    // Actually add title to comboBox (if not hidden)
    // (It will automagically select first non-hidden/non-disabled title
    //  drawn and update message count.)
    updateMessageFolderTitle (messageFolderTag);
}

// Handle selection of a new message folder to display
void MessageView::messageFolderSelectedHandler (int comboBoxIndex)
{

    // Convert comboBoxIndex into folderIndex
    int folderIndex = comboBoxIndex2folderIndex.findEntry(comboBoxIndex);

    // Expect a mapping to exist, warn if not
    if (folderIndex == -1)
    {
	fprintf (stderr, 
		 "Warning in MessageView::messageFolderSelectedHandler: "
		 "unmapped comboBox index %i!\n", comboBoxIndex);
	return;
    }

    // If this combobox entry not really selected, make sure it is
    if (messageFolder->currentItem() != comboBoxIndex)
    {
	fprintf (stderr, "Warning: messageFolder displaying %i not %i\n",
		 messageFolder->currentItem(), comboBoxIndex);
	messageFolder->setCurrentItem (comboBoxIndex);
    }

    // Do something, only if different folder is selected
    if (selectedMessageFolder != folderIndex)
    {
	// Set messageFolderSelected to this messageFolder
	selectedMessageFolder = folderIndex;

	// Get the folderTag at the selected index
	QString folderTag = um->messageFolderAt (folderIndex);

	// Call helper routine to update message display
	updateMessageDisplay (folderTag);
	
	// If folder exists, emit signal to any interested listeners
	if (!folderTag.isEmpty())
	{
	    // Get the messageFolderTitle
	    QString folderTitle = um->messageFolderTitle (folderTag);
	    
	    // Emit signal that folder selected
	    emit messageFolderSelected (folderTag.latin1(), folderTitle.latin1());
	}
    }
}

//! Updates the message display for a message folder
void MessageView::updateMessageDisplay (const QString &folderTag)
{
    // Do nothing if folderTag is empty or null
    if (folderTag.isNull() || folderTag.isEmpty())
    {
	// This message can happen randomly due to timer race conditions,
	// so stop warning -JCG 11/10/05
//	fprintf (stderr, "Warning: Ignoring empty tag passed to "
//		 "MessageView::updateMessageDisplay!\n");
	return;
    }
    
    // Indicate whether we are displaying a new folder
    bool newFolder = FALSE;

    // Are we displaying a differently folder that currently shown?
    if (displayedMessageFolder.compare(folderTag) != 0)
    {

#if TIME_MESSAGE_VIEW
    // DEBUG
    TG_timestamp ("MessageView::updateMessageDisplay (%s), start new folder prep\n",
		  folderTag.latin1());
#endif


	// Yes, delete current map of recordIds to messages
	recordId2messageIndex.deleteAllEntries();
	messageIndex2recordId.deleteAllEntries();

	// No messages currently displayed for this new folder
	displayedMessageCount = 0;

	// No lines currently displayed
	displayedLineCount = 0;

	// Initially, no lines are longest so mark as such with -1
	maxHeaderId = -1;
	maxHeaderLen = -1;
	maxBodyId = -1;
	maxBodyLen = -1;
	displayedMaxWidth = -1;

	// Clear existing grid contents
	// This will optimize the display of messages below since
	// we will not have to worry about the grid having some
	// unusually hierarchy from the previous message folder.
	messageGrid->clearGridContents();

	// Set title of bar to title of new messageFolder (for now)
	QString messageFolderTitle = um->messageFolderTitle (folderTag);
	messageGrid->setHeaderText (0, messageFolderTitle);

	// We are now displaying this messageFolder
	displayedMessageFolder = folderTag;

	// Get th state for this message folder
	int folderIndex = um->messageFolderIndex (folderTag);
	displayedFolderState = messageFolderState.findEntry (folderIndex);

	//Sanity check
	if (displayedFolderState == NULL)
	{
	    TG_error ("displayedFolderState is NULL for '%s' index %i!\n",
		      folderTag.latin1(), folderIndex);
	}

	// Now displaying a new folder
	newFolder = TRUE;

	// Handle case where there are no messages yet
	if (um->messageCount(folderTag) < 1)
	{
	    // Resize grid so it has only one cell
	    messageGrid->resizeGrid (1,1);
	    
	    // Put message that there are not folders currently
	    messageGrid->setCellText (0,0, "(No messages yet)");
	    
	    // Use the above message as the longest header line
	    maxHeaderId = 0;
	    
	    // Mark that display width needs to be recalculated
	    displayedMaxWidth = -1;

	    // Update the display width so all text is visible
	    updateMessageDisplayWidth();

	    // Get the number of messages for this new folder
	    int messageCount = um->messageCount (folderTag);

	    // If there are no messages, we need to clear the traceback
	    // by 'selecting' a new empty default message
	    if (messageCount == 0)
	    {
		// Flag that we are drawing this folder for the first time
		displayedFolderState->refreshView = TRUE;
		
		// Create "vanilla" event with id 1101 to indicate that we want
		// to select the default message
		QCustomEvent *ce = new QCustomEvent (1101);
		
		// Submit selectDefaultMessage event for processing after 
		// current event is done.  This prevents race conditions 
		// when creating a new window, since the traceback window 
		// is not connected yet.
		
		// QT will delete ce when done
		QApplication::postEvent (this->parent(), ce); 

		// Will return below  (may want to return here in future)
	    }
	}

#if TIME_MESSAGE_VIEW
    // DEBUG
    TG_timestamp ("MessageView::updateMessageDisplay (%s), end new folder prep\n",
		  folderTag.latin1());
#endif
    }

    // Add all the messages for this folder not currently displayed
    int messageCount = um->messageCount (folderTag);

    // Return now, if there are no new messages
    if (messageCount == displayedMessageCount)
    {
	return;
    }

    //Sanity check
    if (displayedFolderState == NULL)
    {
	TG_error ("displayedFolderState is NULL for '%s'!\n",
		  folderTag.latin1());
    }


    // Get the openMessages state from displayed folder state
    IntSet *openMessages = &displayedFolderState->openMessages;

#if TIME_MESSAGE_VIEW
    // DEBUG
    TG_timestamp ("*** MessageView::updateMessageDisplay (%s): %i new messages\n",
		  folderTag.latin1(), messageCount - displayedMessageCount);
#endif

    // Calculate how many additional lines are required to add the
    // new messages
    int additionalLines = 0;
    
    // Loop through all new messages and count the additional lines needed
    for (int messageIndex = displayedMessageCount; messageIndex < messageCount;
	 messageIndex ++)
    {
	QString messageTag = um->messageAt (folderTag, messageIndex);
	int lineCount = um->messageTextLineCount (folderTag, messageTag);

	// Add in the lines needed for this message
	additionalLines += lineCount;
    }

    // Calculate the new grid needed to hold all the new line
    int totalLines = displayedLineCount + additionalLines;

    // Resize grid to hold all the messages (including children)
    messageGrid->resizeGrid (totalLines, 1);


    // RecordId2messageIndex will have totalLines in it at the end,
    // so give hint to resize algorithm to make it more efficient
    recordId2messageIndex.tweakTableResize (totalLines);
	    
    // Add the contents of the new messages to grid, 
    // making all lines after the first line children 
    // of the message title (starting after currently displayed messages)
    int curRecordId = displayedLineCount;

    // Loop all the new messages adding their contents to the display
    for (int messageIndex = displayedMessageCount; messageIndex < messageCount;
	 messageIndex ++)
    {
	// Use the first line of the messageBuf as a header
	int headerRecordId = curRecordId;

	// Get the messageTag at this index
	const QString messageTag = 
	    um->messageAt (folderTag, messageIndex);

	// Get the number of messages in the message directly
	int lineCount = um->messageTextLineCount (folderTag, messageTag);

	// Get a direct reference to the header line
	const char *headerRef = 
	    um->messageTextLineRef (folderTag, messageTag, 0);

	// Put header contents on the first line
	messageGrid->setCellTextRef (headerRecordId, 0, headerRef);
	messageGrid->setCellStyle (headerRecordId, 0, headerStyle);

	// Map this recordId to the messageIndex
	recordId2messageIndex.addEntry (headerRecordId, messageIndex);
	messageIndex2recordId.addEntry (messageIndex, headerRecordId);

	// Get the length of the header
	int headerLen = strlen (headerRef);

	// Update longest message header line, if necessary
	if (headerLen > maxHeaderLen)
	{
	    // Record recordId of the new longest header line
	    maxHeaderId = headerRecordId;
	    maxHeaderLen = headerLen;
	    
	    // Mark that display width needs to be recalculated
	    displayedMaxWidth = -1;
	}

	// Start the body just after the header
	curRecordId++;
	int bodyStartId = curRecordId;

	// Close the tree by default (for now), if have at least one child
	// Also, put all "children" lines below it here at once
	if (lineCount > 1)
	{
	    
#if 0
	    // THIS SHOULD BE THE DEFAULT NOW, SO NOT NECESSARY
	    // Force expandable and closed before adding children
	    // to make one less redraw needed
	    messageGrid->setTreeExpandable (headerRecordId, TRUE);
	    messageGrid->setTreeOpen (headerRecordId, FALSE);
#endif

	    // Place all the recordIds for the other lines below the
	    // header in one group 
	    messageGrid->placeRecordId (bodyStartId, bodyStartId+lineCount-2,
					headerRecordId, headerRecordId);

	    // If message in open folder, open it now
	    if (openMessages->in(messageIndex))
	    {
		messageGrid->setTreeOpen (headerRecordId, TRUE);
#if 0
		// DEBUG
		TG_timestamp ("message %i in open set\n", messageIndex);
#endif
	    }
	}
#if 0
	// THIS SHOULD NOT BE NECESSARY NOW, BECAUSE ABOVE COMMENTED OUT
	else
	{
	    // Don't force expandable, message only one line
	    messageGrid->setTreeExpandable (headerRecordId, FALSE);
	}
#endif

	// Add all other lines as children of the header
	for (int lineNo = 1; lineNo < lineCount; lineNo++)
	{
	    // Calculate body line location
	    int bodyRecordId = bodyStartId + lineNo -1;

	    // Map this recordId to the messageIndex
	    recordId2messageIndex.addEntry (bodyRecordId, messageIndex);

	    // Get a pointer to the actual message line (for efficiency)
	    const char *lineRef = um->messageTextLineRef (folderTag, messageTag,
							  lineNo);

	    // Put body contents at this location, using reference for
	    // memory and speed efficiency
	    messageGrid->setCellTextRef (bodyRecordId, 0, lineRef);

	    // Update longest message body line, if necessary
	    int bodyLen = strlen (lineRef);
	    if (bodyLen > maxBodyLen)
	    {
		// Record recordId of the new longest body line
		// (all body lines are placed in the records below
		//  the header, subtract 1 from lineCount to get
		//  correct index)
		maxBodyId = headerRecordId + lineCount - 1;
		maxBodyLen = bodyLen;
		
		// Mark that display width needs to be recalculated
		displayedMaxWidth = -1;
	    }
	}

	// Update curRecordId to point to first recordId after message
	curRecordId += lineCount -1;
    }

#if TIME_MESSAGE_VIEW
    TG_timestamp ("Mid updateMessageDisplay(%s): %i new messages, %i new lines, old %i, %i\n", folderTag.latin1(),
		  messageCount-displayedMessageCount,
		  additionalLines, displayedMessageCount, displayedLineCount); 
#endif


    // Save previous lineCount for use in tests below
    int prevLineCount = displayedLineCount;

    // Update displayedLineCount and displayedMessageCount now that we
    // are done
    displayedLineCount = totalLines;
    displayedMessageCount = messageCount;

    // Update the display width so all text is visible
    updateMessageDisplayWidth();

    // If viewing new folder, flag that we need to refresh the message view
    // and perhaps automagically pick a message, if necessary
    if (prevLineCount == 0)
    {
	// Flag that we are drawing this folder for the first time
	displayedFolderState->refreshView = TRUE;

	// Create "vanilla" event with id 1101 to indicate that we want
	// to select the default message
	QCustomEvent *ce = new QCustomEvent (1101);

	// Submit selectDefaultMessage event for processing after current
	// event is done.  This prevents race conditions when creating
	// a new window, since the traceback window is not connected yet.
	
	// QT will delete ce when done
	QApplication::postEvent (this->parent(), ce); 
    }

#if TIME_MESSAGE_VIEW
    TG_timestamp ("Ending updateMessageDisplay(%s)\n", folderTag.latin1());
#endif
}

// Selects a default message, if one not already selected
// Used by messageViewer->customEvent() to selected the
// first message if the user hasn't already selected one
void MessageView::selectDefaultMessage()
{
    // Can get multiple of these events overlapping
    // Prevent this, can overflow stack!
    static bool midUpdate = FALSE;

    // If already in this routine, return now
    if (midUpdate)
    {
#if 0
	// DEBUG
	TG_timestamp("Called selectDefaultMessage update! Returning!\n");
#endif
	return;
    }

    // Mark that we are in this routine
    midUpdate = TRUE;

    // Are we displaying a new folder?
    if (displayedFolderState->refreshView)
    {
	// Get currently selected message, if any
	int selectedMessage = displayedFolderState->selectedMessage;

	// By default, select the first message
	int selectedRecordId = 0;

	// Unless we have a message already selected
	if (selectedMessage >= 0)
	{
	    // Select the old message, map back to the appropriate recordId
	    selectedRecordId = 
		messageIndex2recordId.findEntry(selectedMessage);
	}
	
	// Sanity check, if no mapping, select the first record
	if (selectedRecordId < 0)
	    selectedRecordId = 0;

	// Clear old state so messageClickedHandler will do something
	displayedFolderState->selectedRecordId = -1;
	displayedFolderState->selectedMessage = -1;

	// "Click" on the header of message selected by default on new views
	messageClickedHandler (selectedRecordId, 0, Qt::NoButton , 0, 0, 0, 
			       0, 0, 0);


	// Move window to previous location (or top, left if first time)
	messageGrid->setContentsPos (displayedFolderState->curX,
				     displayedFolderState->curY);

	// Mark that we are finished refreshing the view
	displayedFolderState->refreshView = FALSE;
    }

    // Mark that we are no longer in this routine
    midUpdate = FALSE;
}

// Update the display width so all text is visible
void MessageView::updateMessageDisplayWidth()
{
    // If displayedMaxWidth is -1, we need to recalculate it from
    // the longest header and body lines.  Calculating the line
    // length in pixels is very expensive, so just do it on the
    // longest lines (works correctly only with fixed fonts, but the
    // message display looks so bad with proportional font, that only 
    // a fixed font should be used!).  Taking the width of 1500 lines
    // was taking .5 seconds, which just doesn't scale!  -JCG 4/5/04
    if (displayedMaxWidth == -1)
    {
	// Use a minimim of 50 pixels width
	displayedMaxWidth = 50;

	// Update width with maxHeaderId, if not -1
	if (maxHeaderId != -1)
	{
	    // Get the width, in pixels, of the longest header line
	    int headerWidth = messageGrid->cellWidth(maxHeaderId, 0);

	    // Update displayedMaxWidth, if bigger
	    if (headerWidth > displayedMaxWidth)
		displayedMaxWidth = headerWidth;
	}

	// Update width with maxBodyId, if not -1
	if (maxBodyId != -1)
	{
	    // Get the width, in pixels, of the longest message body line
	    int bodyWidth = messageGrid->cellWidth(maxBodyId, 0);

	    // Update displayedMaxWidth, if bigger
	    if (bodyWidth > displayedMaxWidth)
		displayedMaxWidth = bodyWidth;
	}
    }

    // Get the visisible width of the messageGrid window used to display 
    // the message (different than width, since scrollbars change)
    int winWidth = messageGrid->visibleWidth();

    // Get the current width of the attrId we are using for everything
    int currentWidth = messageGrid->attrIdWidth (0);

    // Set new width to at least the visible window width, so
    // the lightlight goes across the entire window.
    int newWidth = winWidth;

    // If the displayMaxWidth is bigger (contents is bigger), use that
    // width, so entire message can be seen (using scrollbars).
    if (displayedMaxWidth > newWidth)
    {
	newWidth = displayedMaxWidth;

	// Enable scrollbars, probably are necessary now
	messageGrid->setHScrollBarMode (QScrollView::Auto);
    }
    // Otherwise, disable scrollbars, they behave poorly when the
    // window is resized.
    else
    {
	// If already at this width, return now
	if (currentWidth == newWidth)
	    return;

	// Disable scrollbars, not needed and makes resizes ugly
	messageGrid->setHScrollBarMode (QScrollView::AlwaysOff);
    }

    // DEBUG
#if 0
    printf ("displayedMaxWidth = %i, maxHeaderId %i, maxBodyId %i, window width = %i, newWidth = %i\n", 
	    displayedMaxWidth, maxHeaderId, maxBodyId, winWidth, newWidth);
#endif

    // Set the column width so the entire message is visible and 
    // the entire window is filled in (if not already this width).
    messageGrid->setHeaderWidth (0, newWidth);
}

// Catch changes to the messageGrid display window size
void MessageView::messageGridResized( QResizeEvent * /*e*/ )
{
    // Update the message display width, if necessary
    updateMessageDisplayWidth();
}


// Catch message window movements so we can record possitions
void MessageView::contentsMovingHandler (int x, int y)
{
    if (displayedFolderState != NULL)
    {
	// Record the current possible of the top left corner
	displayedFolderState->curX = x;
	displayedFolderState->curY = y;

//	TG_timestamp ("Setting folder position info to %i, %i\n", x, y);
    }
}



// Returns selected line out of a multiline string (using newlines)
// where lineNo == 0 returns the first line.  Returns TRUE if lineNo
// exists in string, FALSE otherwise
bool MessageView::getIndexedLine (const QString &src, int lineNo, 
				  QString &dest)
{
    // Initialize to no-info specified 
    dest = NULL_QSTRING;

    // Get pointer to beginning of string
    const char * ptr = src.latin1();

    // Skip the specified number of lines
    int linesSkipped = 0;
    while (linesSkipped < lineNo)
    {
	// If don't have anything left, line doesn't exist
	if (*ptr == 0)
	    return (FALSE);

	// If hit newline, have gone to next line
	if (*ptr == '\n')
	    linesSkipped++;

	ptr++;
    }

    // Start with empty line buffer
    dest = "";

    // Copy over string one character at a time
    while ((*ptr != 0) && (*ptr != '\n'))
    {
	dest.append(*ptr);
	ptr++;
    }
    
    // Return TRUE, got line
    return (TRUE);
}
    
// Handle new messages added to a folder
void MessageView::messageAddedHandler (const char * /*messageFolderTag*/, 
				       const char * /*messageText*/,
				       const char * /*messageTraceback*/)
{
    // Flag that a message has been added, handled by flushMessageUpdates()
    messagesAdded = TRUE;
}

// Flushes all message updates pending (added by messageAddedHandler())
void MessageView::flushMessageUpdates ()
{
#if 0
    TG_timestamp ("MessageView::flushMessageUpdates (added %i)\n",
		  messagesAdded);
#endif

    // Just call updateMessageDisplay on current folder, it knows
    // when it needs to do something.
    updateMessageDisplay(displayedMessageFolder);

    // Update the message count for all message folders if messages added
    if (messagesAdded)
    {
	// Loop over all message folders
	int folderCount = um->messageFolderCount();
	for (int folderIndex = 0; folderIndex < folderCount; ++folderIndex)
	{
	    // Get the index of the messageFolder
	    QString messageFolderTag = um->messageFolderAt (folderIndex);


	    // Call updateMessageFolderTitle to change title, if necessary.
	    // It knows when it needs to do something.
	    updateMessageFolderTitle (messageFolderTag);
	}

	// Clear messages added flag
	messagesAdded = FALSE;
    }

    // If need to popup folder selection because user selected disabled
    // version, do it now
    if (popupFolderSelection)
    {
	// Clear flag before popup, it may be set again in popup
	popupFolderSelection = FALSE;
	
	// Popup the message folder selection list
	messageFolder->popup();
    }
}


// Handle clicks on messages
void MessageView::messageClickedHandler (int recordId, int /*attrId*/, ButtonState,
					 int, int, int, int, int, int)
{
    // Do nothing if messageFolder is not selected
    if (selectedMessageFolder < 0)
    {
	return;
    }

    // Use recordId2messageIndex map to find messageIndex
    int messageIndex = recordId2messageIndex.findEntry(recordId);

    // Handle case where no messages exist, so none are selected
    if (messageIndex < 0)
    {
	// Sanity check, we expect not to have this message index for
	// only recordId 0
	if (recordId != 0)
	{
	    TG_error ("MessageView::messageClickedHandler: "
		      "message not found for recordId %i!\n", recordId);
	}

	// This is expected if no messages in folder yet
	QString folderTag = um->messageFolderAt(selectedMessageFolder);

	// Emit 'no message' selected message
	emit messageSelected (folderTag.latin1(), NULL, NULL);

	// Done, return now
	return;
    }

    // Only do something if message changed from last click
    if (displayedFolderState->selectedMessage != messageIndex)
    {
	// Clear previously selected cell
	if (displayedFolderState->selectedRecordId >= 0)
	    messageGrid->setCellStyle (displayedFolderState->selectedRecordId, 
				       0, headerStyle);
    
	// Treat clicks on message text as click on header by converting
	// recordId to parent's recordid
	int parentId = messageGrid->recordIdParent (recordId);
	if (parentId != NULL_INT)
	    recordId = parentId;
	
	// Set the style of the newly selected message
	messageGrid->setCellStyle (recordId, 0, selectedMessageStyle);
    
	// Record the selectedMessage id
	displayedFolderState->selectedMessage = messageIndex;

	// And what recordId was selected
	displayedFolderState->selectedRecordId = recordId;

	// Emit detailed event for interested listeners
	QString folderTag = um->messageFolderAt(selectedMessageFolder);
	QString messageTag =
	    um->messageAt (folderTag, displayedFolderState->selectedMessage);
	QString messageTraceback = um->messageTraceback (folderTag, messageTag);
	emit messageSelected (folderTag.latin1(), messageTag.latin1(), 
			      messageTraceback.latin1());
    }
}

// Handle doubleclicks on messages
void MessageView::messageDoubleClickedHandler (int recordId, int /*attrId*/, 
					       ButtonState,
					       int, int, int, int, int, int)
{
    // Do nothing if messageFolder is not selected
    if (selectedMessageFolder < 0)
	return;

    // Use recordId2messageIndex map to find messageIndex
    int messageIndex = recordId2messageIndex.findEntry(recordId);

    // Sanity check, make sure messageIndex found (-1 on not found)
    if (messageIndex < 0)
    {
	// Ok if recordId 0 (empty folders get here)
	if (recordId == 0)
	    return;

	// Otherwise, punt
	TG_error ("MessageView::messageDoubleClickedHandler: "
		  "message not found for recordId %i!\n", recordId);
    }

    // Treat double clicks on message text as click on header by converting
    // recordId to parent's recordid
    int parentId = messageGrid->recordIdParent (recordId);
    if (parentId != NULL_INT)
	recordId = parentId;

    // Get the set of open messages we are maintaining for this messageFolder
    IntSet *openMessages = &displayedFolderState->openMessages;

    // Is the tree currently open?
    bool treeOpen = messageGrid->isTreeOpen(recordId);

    // If yes, close and make sure header is visible
    if (treeOpen)
    {
	// Close the tree
	messageGrid->setTreeOpen(recordId, FALSE);

	// Remove from set of open messages
	openMessages->remove(messageIndex);

	// Make sure header visible, minimum margin
	messageGrid->scrollCellIntoView (recordId, 0, 0);
	
    }
    // Otherwise, just open the tree
    else
    {
	messageGrid->setTreeOpen(recordId, TRUE);

	// Add to set of open messages
	openMessages->add(messageIndex);

#if 0
	// DEBUG
	TG_timestamp ("Adding message %i to open set for folder %i\n", 
		      messageIndex, messageFolderIndex);
#endif
    }
}

// Handles clicks on the tree hierachy control area]
// Flips open/closed state 
void MessageView::treeClickHandler (int recordId, int attrId,
                                 ButtonState buttonState,
                                 int /*indentLevel*/, int parentRecordId,
                                 bool treeOpen, int /*x*/, int /*y*/, int /*w*/, int /*h*/)
{
    // If click on arrow, flip state
    if (recordId == parentRecordId)
    {
	// Tree click on arrow as double click on message, will flip state
	messageDoubleClickedHandler (recordId, attrId, buttonState,
				     0,0,0,0,0,0);

	// If opening message, also select it (but not if closing it)
	if (!treeOpen)
	{
	    messageClickedHandler (recordId, attrId, buttonState,
				   0,0,0,0,0,0);
	}
    }
    
    // Otherwise, do nothing for now
}

// Handles double clicks on the tree hierachy control area
// Just treat like double click on message unless on arrow
void MessageView::treeDoubleClickHandler (int recordId, int attrId,
					  ButtonState buttonState,
					  int /*indentLevel*/, int parentRecordId,
					  bool /*treeOpen*/,
					  int /*x*/, int /*y*/, int /*w*/, int /*h*/)
{
    // If double click on something besides arrow, tree like message click
    if (recordId != parentRecordId)
    {
	messageDoubleClickedHandler (recordId, attrId, buttonState,
				     0,0,0,0,0,0);
    }
    // Otherwise, do nothing for now
}

void MessageView::resetFont( QFont& f )
{
	messageGrid->setFont( f );
	// Redraw the grid
	messageGrid->resizeGrid( messageGrid->recordIds(),
			messageGrid->attrIds() );
//	messageGrid->update();
}

// Catch changes to the label font.  Initially, need to redraw message
// folder titles with new font
void MessageView::labelFontChanged( QFont & /*f*/)
{
    // Use a hack to update titles when font changes.
    // I hope I don't regret this hack. :)

    // Delete all state, so will redraw all titles
    displayedCount.deleteAllEntries();

    // Set messages added flag to make it redraw titles
    messagesAdded = TRUE;

    // All the actual redraw work will be done next clock tick by
    // MessageView::flushMessageUpdates ()    
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

