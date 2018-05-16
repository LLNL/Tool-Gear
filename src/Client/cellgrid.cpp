// cellgrid.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.02                                               Sept 8, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** A Qt Widget (CellGrid) designed to support the display and annotation 
** of source code and data.  CellGrid supports a flexible annotation facility
** and allows clicks on these annotations to be captured.  Also allows a
** tree hierarchy to be used to collapse the display in a structured way.
** Represented in the API as a 2D grid of cells, the cells may be sorted
** and rearranged by the user without affecting the recordId and attrId
** address of each cell.  The grid can be also searched  and 
** is searched in the current display order.
**
** Designed by John May and John Gyllenhaal at LLNL
** Initial implementation by John Gyllenhaal 8/18/00
**
*****************************************************************************/

#include <cellgrid.h>
#include <qheader.h>
#include <stdio.h>
#include <qstring.h>
#include <qkeycode.h>
#include <stdlib.h>
#include <qapplication.h>
#include <qwhatsthis.h>
#include <qclipboard.h>

#include "heapsort.h"
#include <iostream>
#include "tg_time.h"
using namespace std;

// The default pixmap for a closed row tree node is a 3d right arrow
static const char * closedTree_xpm[] = {
"12 10 4 1",
"       c None",
".      c #D000D000D000",
"O      c #900090009000",
"X      c #600060006000",
" ..         ",
" ....       ",
" ......     ",
" ..OO....   ",
" ..OOOO.... ",
" ..OOOOXXXX ",
" ..OOXXXX   ",
" ..XXXX     ",
" .XXX       ",
" .X         "};

// The default pixmap for a opened row tree node is a 3d down arrow
static const char * openedTree_xpm[] = {
"12 10 4 1",
"       c None",
".      c #D000D000D000",
"O      c #900090009000",
"X      c #600060006000",
" .......... ",
" ........XX ",
"  ..OOOOXX  ",
"  ..OOOOXX  ",
"   ..OOXX   ",
"   ..OOXX   ",
"    ..XX    ",
"    ..XX    ",
"     .X     ",
"     .X     "};

// Declare invalid CellStyleId for checking returned style ids
const CellStyleId CellStyleInvalid = -1;

// Declare invalid GridPixmapId for checking returned pixmaps ids
const GridPixmapId GridPixmapInvalid = -1;

// Creates a grid of cells where strings and pixmaps can
// be displayed.  Use minRowHeight to set the minimim
// row high in pixels (otherwise set only by font), useful
// if have fixed high-annotations. 
CellGrid::CellGrid(int recordIds, int attrIds, QFont font, int minRowHeight,
		   QWidget* parent, const char *name) :
    // For less flicker, use WRepaintNoErase(otherwise greys out area 
    // before redrawing it).  Very important to draw *everything* in
    // in the contents area (even out of bounds), otherwise will get
    // apparent stuttering during scrolling.
    QScrollView (parent,name, WRepaintNoErase|WNorthWestGravity),

    // Allocate table to hold grid of cells, initial size estimated to
    // be gridSize (it will dynamically grow, as necessary, so could pick 0).
    // Delete all cells on exit
    cellTable ("Grid", DeleteData, recordIds * attrIds),

#if 0
    // Set default font to fixed-width font
#if defined(TG_LINUX)
    // Courier doesn't look good on linux
//    defaultCellFont("console", 12),
    defaultCellFont("courier", 10, QFont::Bold),
#elif defined(TG_MAC) 
    defaultCellFont("courier", 10),
#else
    defaultCellFont("courier", 8),
#endif
#endif
    defaultCellFont( font ),


    // Pixmap that cell contents is drawn into before bitblting to screen
    // Pick some reasonable initial size, will be resized as necessary
    paintPixmap(100,20),

    // CellLayout class that manages and places slots for paintCell
    cellLayout(),

    // Allocate table to hold all declared cell style, delete styles on exit
    styleTable("Style", DeleteData, 0),

    // Allocate table indexed by pixmap id to hold all declared grid pixmaps, 
    // delete pixmaps on exit
    pixmapIdTable("PixmapId", DeleteData, 0),

    // Allocate table indexed by name to hold all declared grid pixmaps, 
    // Do not delete pixmaps on exit, deleted by above table
    pixmapNameTable("PixmapName", NoDealloc, 0),

    // Colors to paint tree hierachry lines with
    // Making look 3d, so have light, main, dark versions of color
    lightTreeLineColor(0xD0, 0xD0, 0xD0),
    mainTreeLineColor(0x90, 0x90, 0x90),
    darkTreeLineColor(0x60, 0x60, 0x60),

    // Keep track of requested minRowHeight so we can use it as the
    // font size changes
    minRHeight( minRowHeight )
{

//#ifdef TG_LINUX
//    defaultCellFont.setFixedPitch(TRUE);
//    defaultCellFont.setWeight(QFont::Bold);
//#endif
#if 0
    // DEBUG, PRINT OUT FONT INFO
    QFontInfo fontInfo(defaultCellFont);
    fprintf (stderr, 
	     "CellGrid font selected: %s point %i weight %i fixed %i\n", 
	     fontInfo.family().latin1(), fontInfo.pointSize(),
	     fontInfo.weight(), 
	     (int)(fontInfo.fixedPitch()==TRUE));
#endif

    // Prevent negative recordIds/attrIds.  Set to 0 if necessary.
    // May want to set to 1 later...
    if (recordIds < 0)
	recordIds = 0;
    if (attrIds < 0)
	attrIds = 0;

    // Prevent huge recordIds/attrIds that may cause the infinite loops
    // before running out of memory. :)  
    if (recordIds > 1000000000)
    {
	fprintf (stderr, 
		 "CellGrid: recordIds (%i) > 1000000000.  To big!\n", 
		 recordIds);
	exit (1);
    }
    if (attrIds > 1000000000)
    {
	fprintf (stderr, 
		 "CellGrid: attrIds (%i) > 1000000000.  To big!\n", 
		attrIds);
	exit (1);
    }

    // Set number of recordIds and attrIds
    numRecordIds = recordIds;
    numAttrIds = attrIds;

    // Default, all records visible 
    numRowsVisible = recordIds;

    // Default to grid lines off
    gridOn = FALSE;
#if 0
    // DEBUG ONLY
    gridOn = TRUE;
#endif

    // Start with no cell styles declared
    maxStyleId = -1;

    // Start with no pixmaps declared
    maxPixmapId = -1;

    // Set default font to cell's default font (set above)
    setFont(defaultCellFont);

    // Track where mouse is so can support MovedOver functionality
    // (like cellAnnotMovedOver)
    viewport()->setMouseTracking(TRUE);

    // Use optimized pixmap (faster, more memory) for target of paintCell
    paintPixmap.setOptimization (QPixmap::BestOptim);

    // This is needed to give this object the keystokes hit
    // For Qt 2.x, do not do setFocusPolicy (QWidget::StrongFocus), causes
    // flickering when moving mouse (I believe)
    // setFocus();
    // For QT 3.x, using setFocusPolicy is the right thing to do again.
//    setFocusPolicy (QWidget::WheelFocus);
// JMM: testing alternatives
    setFocusPolicy( QWidget::StrongFocus );


    // Create the default style for the grid
    defaultStyleId = newCellStyle ("Grid default");
    setStyleFgColor(defaultStyleId, QColor("Black"));
    setStyleBgColor(defaultStyleId, QColor("White"));

    // Get quick access pointer to default style
    defaultStyle = getStyle (defaultStyleId);

    // To allow efficient resizing, allocate remapping arrays larger
    // than needed to hold recordIds. 
    // Start with a minumum size of 32
    rowMapAllocSize = 32; 

    // Double rowMapAllocSize until recordIds is reached or exceeded 
    // This requires recordIds to be less than 2^30 (test above enforces)
    while (rowMapAllocSize < recordIds)
	rowMapAllocSize = rowMapAllocSize << 1;
    
    // Allocate mapping arrays for recordIds, using rowMapAllocSize.
    // This gives some growing room.  
    
    row2RecordId = new int[rowMapAllocSize];
    TG_checkAlloc(row2RecordId);

    // Array specifying the order to display recordIds if they are visible
    recordIdOrder = new int[rowMapAllocSize];
    TG_checkAlloc(recordIdOrder);

    // Array of structures for specifying recordId specific info
    recordIdInfo = new RecordIdInfo[rowMapAllocSize];
    TG_checkAlloc(recordIdInfo);

    // Initially, no recordId's in tree
    treeFirstChild = NULL;
    treeLastChild = NULL;
    treeFirstPlacedChild = NULL;
    treeLastPlacedChild = NULL;


    // Initially, no area selected in grid
    areaSelected = FALSE;
    startSelectX = -1;
    startSelectY = -1;
    selectedX1 = -1;
    selectedX2 = -1;
    selectedY1 = -1;
    selectedY2 = -1;

    // Only scroll this cell grid when selecting text and move cursor off
    // of window.
    scrollDX = 0;
    scrollDY = 0;
    currentSelectX = 0;
    currentSelectY = 0;

    // Create scroll timer that scrolls based on scrollDX and scrollDY
    scrollTimer = new QTimer (this, "scrollTimer");
    
    // Do scrolling at regular intervals if selecting text
    connect (scrollTimer, SIGNAL(timeout()), this, SLOT(scrollTimerHandler()));

    // Get clipboard for ease of use
    clipboard = QApplication::clipboard();

    // Monitor clipboard for changes
    connect (clipboard, SIGNAL (selectionChanged()), this,
	     SLOT(clipboardChangedHandler()));

    // Initially, nothing being watched by search engine
    watchRecordId = -1;  
    watchAttrId = -1;    
    watchpointTriggered = FALSE; 

    // Initialize to "unsorted" mapping, only initialize part going to use
    for (int i=0; i < recordIds; i++)
    {
	recordIdInfo[i].recordId = i;
	recordIdInfo[i].row = i;
	recordIdInfo[i].visible = 1; // By default, all rows visible
	recordIdInfo[i].treeHides = 0; // Make all new rows not hidden
	recordIdInfo[i].userHides = 0; // Make all new rows not hidden

	// Initialize tree child pointers to NULL
	recordIdInfo[i].firstChild = NULL;
	recordIdInfo[i].lastChild = NULL;
	recordIdInfo[i].firstPlacedChild = NULL;
	recordIdInfo[i].lastPlacedChild = NULL;
	
	// Sanity check, initialize rest of pointers to NULL
	// Although should be reset by attachRecordInfo below
	recordIdInfo[i].parent = NULL;
	recordIdInfo[i].prevSib = NULL;
	recordIdInfo[i].nextSib = NULL;
	recordIdInfo[i].prevPlacedSib = NULL;
	recordIdInfo[i].nextPlacedSib = NULL;

	// Use internal routine to place after last child in the 
	// top level tree
	attachRecordInfo(&recordIdInfo[i], NULL, treeLastChild);
	
	row2RecordId[i] = i;
	recordIdOrder[i] = i;
	recordIdInfo[i].displayOrder = i;  // Maps back to recordIdOrder index

	recordIdInfo[i].treeOpen = 0;       // Tree closed by default
	recordIdInfo[i].treeExpandable = 0; // Expandable only if w/ child

	// Initialize fields used by placeRecordId
	recordIdInfo[i].beingMoved = 0;
    }

    // The documentation on this call was vague, but it sounded like
    // I would probably end up in Manual mode anyways, and the idea
    // of my view automatically being resized for me (if I didn't set to
    // Manual) sounded like an opportunity to add random visual bugs.
    setResizePolicy( Manual );

    // For now, by default create a 640x480 window
    resize(640,480);

    // Create "unscrolled" margin at top of scrollview window
    // to place the QHeader in.  Make 4 pixels bigger than
    // need by the font used and will place Qheader in
    // UpdateGeometries down 2 pixels.
    setMargins( 0, fontMetrics().height() + 4, 0, 0 );

#if 0
    // Now done in setFont()
    // Determine default cell height from height of font 
    // (pad 1 pixels on top and bottom + 1 for space between cells)
    // Get font metrics for cell font
    // If minRowHeight larger than font height, use it
    QFontMetrics cfm(defaultCellFont);
    if ((cfm.height() + 3) >= (minRowHeight + 1))
	recordIdFixedHeight = cfm.height() + 3;
    else
	recordIdFixedHeight = minRowHeight + 1;
#endif
	
    
    // Create attrId header at top of scroll view window
    // This allows resizing and reordering of attrIds.
    // For now, bogus info in top
    // Create header field for each attrId
    topHeader = new QHeader( attrIds, this );
    TG_checkAlloc(topHeader);
    topHeader->setOrientation( Horizontal );

    // Default to visible
    topHeaderVisible = TRUE; 

    // Initialize headers to blank bars
    int i = 0;
    for ( i = 0; i < attrIds; ++i ) {
	topHeader->setLabel(i, "");
	topHeader->resizeSection( i, 100 );
    }

    // Turn off tracking when user resizes attrIds... 
    // Unacceptably slow to incrementally resize the attrId 
    // through sshed x-events
    topHeader->setTracking( FALSE );  

    // Allow reordering of attrIds.  Catch position changes to fix display!
    connect( topHeader, SIGNAL( indexChange( int, int, int ) ),
             this, SLOT( attrIdMoved( int, int, int ) ) );
    topHeader->setMovingEnabled( TRUE );
    topHeader->setClickEnabled(TRUE);  // Required for MovingEnabled to work!

    // Allow resizing of attrIds.  Catch size changes to fix display!
    connect( topHeader, SIGNAL( sizeChange( int, int, int ) ),
             this, SLOT( attrIdWidthChanged( int, int, int ) ) );
    topHeader->setResizeEnabled(TRUE);

    // Use scrollview's contentsMoving signal to cause topHeader
    // to scroll with the window contents
    connect( this, SIGNAL( contentsMoving( int, int ) ),
             this , SLOT( contentsMoved( int, int ) ) );

    // Sort the view based on the attrId's content when the header is clicked
    connect ( topHeader, SIGNAL( clicked( int ) ),
	      this, SLOT( attrIdHeaderClicked( int )));

    // Keep track of the last attrId and direction passed to sortAttrIdView
    sortAttrId = -1;
    sortDirection = 0;

    // No updates pending
    updatePending = FALSE;
    completeUpdatePending = FALSE;
    reconstructionPending = FALSE;
    syncPending = FALSE;

    // Define the default closed/opened hierarchy pixmaps
    defaultClosedTreePixmap 
	= newGridPixmap (QPixmap(closedTree_xpm),
			       "Default Closed Tree");
    defaultOpenedTreePixmap 
	= newGridPixmap (QPixmap(openedTree_xpm),
			       "Default Opened Tree");

    // Set closed/opened heirarchy pixmaps to their defaults
    // User may change these
    closedTreePixmap = &(getPixmap(defaultClosedTreePixmap)->pixmap);
    openedTreePixmap = &(getPixmap(defaultOpenedTreePixmap)->pixmap);

    // DEBUG, pick somewhat bogus amounts for testing
    treeAnnotSize = closedTreePixmap->width();
    if (treeAnnotSize < openedTreePixmap->width())
	treeAnnotSize = openedTreePixmap->width();
    treeIndentation = treeAnnotSize;


    // Default to attrId 0 for tree annotations, user can change
    treeAttrId = 0;

    // Try to set up custom What's This handling by specifying
    // some bogus What's this help
    QWhatsThis::add (this, "This is the source and data display window implemented using CellGrid.  Location-specific 'What's this' text not yet implemented for this window.");
}

// Free of memory allocated for the grid
CellGrid::~CellGrid()
{
    // Free recordId mapping arrays;
    delete[] recordIdInfo;
    delete[] row2RecordId;
    delete[] recordIdOrder;

    // Notify anyone who cares that this object is going away
    // (QObject destroy should do this, but I can't get it to
    // work 4/1/04 jmm)
    emit destroyed( this );
    // destructor for tableGrid automatically deletes all cells allocated
}

// Internal routine to sync the underlining scrollview widget with
// the new grid size.
void CellGrid::syncGridSize ()
{

#if 0
    static int count = 0;
    count++;
    // DEBUG
    TG_timestamp ("%s: Entering syncGridSize(), times called=%i\n",
		  name(), count);
#endif

#if 0
    // TOO EXPENSIVE ON MAC OS, believe requireCompleteUpdate below resolves
    // this problem better. -JCG 3/31/04
    //
    // Must flush all outstanding screen updates before resizing
    // the grid...  Resizing can cause scrolling, and it appears
    // that QT can get confused when this happens.  May need
    // to call some sort of flush 'update()' cache in the future
    // to fix all graphical problems!
    flushUpdate();
#endif

    // Flush any placement array reconstructions, since used by 
    // gridSize() to determine visible rows
    flushReconstruction();

    // Resize the scrollable area to fit the grid size (in pixels)
    QSize gs = gridSize();
    resizeContents( gs.width(), gs.height() );

    // Resizing the window can mess up all outstanding updates.
    // Originally, I called flushUpdate() before each resize, but
    // this turned out to cause a huge graphics load on MacOS.
    // I believe a more optimal solution is to register that the
    // entire screen needs to be redrawn next time an flushUpdate()
    // is called (so multiple resizes can be combined).
    requireCompleteUpdate();

    // Adjust top header, etc for current sizes.
    updateGeometries();

    // Clear any pending grid size syncs
    syncPending = FALSE;
}

// Clears current grid contents (removes all tree hierarchy, styles,
// and cell contents) without resizing the grid and emits signal
// gridContentsCleared.
// 
// Should call whenever putting totally unrelated content
// into a cellGrid (such as changing source files or message folders).
// The grid searcher uses this call's signal to reset context.
void CellGrid::clearGridContents ()
{
    // Delete all entries in cellTable
    cellTable.deleteAllEntries();

    // Sweep over every recordIds 
    for (int recordId = 0; recordId < numRecordIds; ++recordId)
    {
	// Get record info
	RecordIdInfo *recordInfo = &recordIdInfo[recordId];

	// Clear any existing expandable and open state
	recordInfo->treeExpandable = 0;
	recordInfo->treeOpen = 0;
    }

    // Unhide all user hidden records, with no fixup, we are fixing
    // up all the records with placeRecordId
    unhideRecordId (0, numRecordIds-1, UserHides, TRUE);

    // Place all recordIds back in orignal order not under a hierarchy
    // This also fixes up the tables possibly corrupted by unhideRecordId
    placeRecordId (0, numRecordIds-1, NULL_INT, NULL_INT);

    // Signal interested parties that contents have been cleared.
    emit gridContentsCleared( this );
}


// Resize grid, deleting items outside new bounds
void CellGrid::resizeGrid(int newNumRecordIds, int newNumAttrIds)
{

#if 0
    // DEBUG
    TG_timestamp ("%s: start resizeGrid(%i, %i)\n", name(), newNumRecordIds, 
		  newNumAttrIds);
#endif
    
    // Prevent negative newNumRecordIds/newNumAttrIds.  Set to 0 if necessary.
    // May want to set to 1 later...
    if (newNumRecordIds < 0)
	newNumRecordIds = 0;
    if (newNumAttrIds < 0)
	newNumAttrIds = 0;

    // Prevent huge newNumRecordIds/newNumAttrIds that may cause the 
    // infinite loops in resize algorithm below before running out of memory.
    if (newNumRecordIds > 1000000000)
    {
	fprintf (stderr, 
		 "CellGrid::resizeGrid: Error newNumRecordIds (%i) "
		 "> 1000000000!\n",
		 newNumRecordIds);
	exit (1);
    }
    if (newNumAttrIds > 1000000000)
    {
	fprintf (stderr, 
		 "CellGrid::resizeGrid: Error newNumAttrIds (%i) "
		 "> 1000000000!\n",
		 newNumAttrIds);
	exit (1);
    }
    
    // Get the minimum of the old and new sizes, so know how to redraw
    int minRecordId, minAttrId;
    if (newNumRecordIds < numRecordIds)
	minRecordId = newNumRecordIds;
    else
	minRecordId = numRecordIds;
    if (newNumAttrIds < numAttrIds)
	minAttrId = newNumAttrIds;
    else
	minAttrId = numAttrIds;
    

    // If have less record Ids, delete cells that fall in the deleted area
    if (newNumRecordIds < numRecordIds)
    {
	// Sweep over recordIds that need to be deleted
	for (int recordId = newNumRecordIds; recordId < numRecordIds;
	     recordId++)
	{
	    // Sweep over all attrIds that could have cells
	    for (int attrId = 0; attrId < numAttrIds; attrId++)
	    {
		// Delete cell (does nothing if not there)
		cellTable.deleteEntry (recordId, attrId);
	    }

	    // If recordId visible, reduce number of visible records
	    if (recordIdInfo[recordId].visible)
	    {
		numRowsVisible--;
	    }

	    // Remove this recordId from tree hiearchy appropriately
	    RecordIdInfo *recordInfo = &recordIdInfo[recordId];


	    // Move all recordInfo children same level as recordInfo
	    // and placed just after recordInfo.  Move from bottom down,
	    // so when we place them after recordInfo, they stay
	    // in the same order
	    while (recordInfo->lastChild != NULL)
	    {
		// Get child before moving out of recordInfo
		RecordIdInfo *childInfo = recordInfo->lastChild;

		// Use internal routine to remove child from RecordInfo
		detachRecordInfo (childInfo);

		// Use internal routine to place after recordInfo in
		// recordInfo's parent's list
		attachRecordInfo(childInfo, recordInfo->parent, recordInfo);
	    }

	    // Use internal routine to detach recordInfo from parent's tree
	    // It is now safe to ignore recordInfo.
	    detachRecordInfo (recordInfo);
	}
	
	// Set the new number of records
	numRecordIds = newNumRecordIds;
    }
    
    // Handle case where newNumRecordIds is bigger, grow arrays appropriately
    else if (newNumRecordIds > numRecordIds)
    {
	// If need to grow array, do it now
	if (newNumRecordIds >= rowMapAllocSize)
	{
	    // Double map size until newNumRecordIds is reached or exceeded */
	    while (rowMapAllocSize < newNumRecordIds)
		rowMapAllocSize = rowMapAllocSize << 1;
	    
	    // Allocate new mapping arrays 
	    RecordIdInfo *newRecordIdInfo = new RecordIdInfo[rowMapAllocSize];
	    TG_checkAlloc(newRecordIdInfo);
	    int *newRow2RecordId = new int[rowMapAllocSize];
	    TG_checkAlloc(row2RecordId);
	    int *newRecordIdOrder = new int[rowMapAllocSize];
	    TG_checkAlloc(newRecordIdOrder);

	    // Copy over old mapping arrays to new ones
	    for (int i=0; i < numRecordIds; i++)
	    {
		newRecordIdInfo[i] = recordIdInfo[i];
		newRow2RecordId[i] = row2RecordId[i];
		newRecordIdOrder[i] = recordIdOrder[i];

		// For now, patch up all tree pointers.  
		// May want to move away from pointers in these 
		// resizable structures
		RecordIdInfo *ri = &newRecordIdInfo[i];
		if (ri->parent != NULL)
		    ri->parent = &newRecordIdInfo[ri->parent->recordId];
		else
		    ri->parent = NULL;
		
		if (ri->prevSib != NULL)
		    ri->prevSib = &newRecordIdInfo[ri->prevSib->recordId];
		else
		    ri->prevSib = NULL;

		if (ri->nextSib != NULL)
		    ri->nextSib = &newRecordIdInfo[ri->nextSib->recordId];
		else
		    ri->nextSib = NULL;

		if (ri->firstChild != NULL)
		    ri->firstChild =&newRecordIdInfo[ri->firstChild->recordId];
		else
		    ri->firstChild = NULL;

		if (ri->lastChild != NULL)
		    ri->lastChild = &newRecordIdInfo[ri->lastChild->recordId];
		else
		    ri->lastChild = NULL;

		if (ri->prevPlacedSib != NULL)
		    ri->prevPlacedSib = 
			&newRecordIdInfo[ri->prevPlacedSib->recordId];
		else
		    ri->prevPlacedSib = NULL;

		if (ri->nextPlacedSib != NULL)
		    ri->nextPlacedSib = 
			&newRecordIdInfo[ri->nextPlacedSib->recordId];
		else
		    ri->nextPlacedSib = NULL;

		if (ri->firstPlacedChild != NULL)
		    ri->firstPlacedChild =
			&newRecordIdInfo[ri->firstPlacedChild->recordId];
		else
		    ri->firstPlacedChild = NULL;

		if (ri->lastPlacedChild != NULL)
		    ri->lastPlacedChild = 
			&newRecordIdInfo[ri->lastPlacedChild->recordId];
		else
		    ri->lastPlacedChild = NULL;
	    }

	    // Move treeFirstChild and treeLastChild pointers to
	    // same position in new array
	    if (numRecordIds > 0)
	    {
		treeFirstChild = &newRecordIdInfo[treeFirstChild->recordId];
		treeLastChild = &newRecordIdInfo[treeLastChild->recordId];
		treeFirstPlacedChild = 
		    &newRecordIdInfo[treeFirstPlacedChild->recordId];
		treeLastPlacedChild = 
		    &newRecordIdInfo[treeLastPlacedChild->recordId];
	    }
	    
	    // Delete old arrays
	    delete[] recordIdInfo;
	    delete[] row2RecordId;
	    delete[] recordIdOrder;


	    // Point at the new arrays
	    recordIdInfo = newRecordIdInfo;
	    row2RecordId = newRow2RecordId;
	    recordIdOrder = newRecordIdOrder;
	}

	// Place new elements after last visible row!
	int row = rowsVisible();

	// Initialize those new elements to sane value (put at end of display)
	for (int i=numRecordIds; i < newNumRecordIds; i++)
	{
	    recordIdInfo[i].recordId = i;
	    recordIdInfo[i].row = row;   // Last visible row
	    recordIdInfo[i].visible = 1; // Make all new rows visible
	    recordIdInfo[i].treeHides = 0; // Make all new rows not hidden
	    recordIdInfo[i].userHides = 0; // Make all new rows not hidden

	    // Initialize tree child pointers to NULL
	    recordIdInfo[i].firstChild = NULL;
	    recordIdInfo[i].lastChild = NULL;
	    recordIdInfo[i].firstPlacedChild = NULL;
	    recordIdInfo[i].lastPlacedChild = NULL;

	    // Sanity check, initialize rest of pointers to NULL
	    // Although should be reset by attachRecordInfo below
	    recordIdInfo[i].parent = NULL;
	    recordIdInfo[i].prevSib = NULL;
	    recordIdInfo[i].nextSib = NULL;
	    recordIdInfo[i].prevPlacedSib = NULL;
	    recordIdInfo[i].nextPlacedSib = NULL;

	    // Use internal routine to place new recordIds after last child 
	    // in the top level tree
	    attachRecordInfo(&recordIdInfo[i], NULL, treeLastChild);

	    row2RecordId[row] = i;
	    recordIdOrder[i] = i;
	    recordIdInfo[i].displayOrder = i;  // RecordIdOrder index at

	    recordIdInfo[i].treeOpen = 0;       // Tree closed by default
	    recordIdInfo[i].treeExpandable = 0; // Expandable only if w/ child

	    // Initialize field used by placeRecordId
	    recordIdInfo[i].beingMoved = 0;

	    // Goto next row
	    row++;
	}
	
	// Update number of rows visible, all new rows visible
	numRowsVisible += newNumRecordIds - numRecordIds;

	// Set the new number of records
	numRecordIds = newNumRecordIds;
    }

    // COME BACK and modify raw viewer to do dynamic updates
    // COME BACK, do the AttrId portion!!!
    if (newNumAttrIds != numAttrIds)
    {
	fprintf (stderr, 
		 "CellGrid::resizeGrid: AttrId resizing not "
		 "implemented yet!!!\n");
    }
    

    // Now delay resize sync as long a possible
    requireSync();

    // For now update whole screen, not worth trying to minimize
    // update (since syncGridSize will require the same update,
    // this operation is really a no-op)
    requireCompleteUpdate();

#if 0
    // DEBUG
    TG_timestamp ("%s: end resizeGrid(%i, %i)\n", name(), newNumRecordIds, 
		  newNumAttrIds);
#endif

}

/* The main/only draw routine in CellGrid.  Only the portion
 *  of the screen requested is/should be drawn.  All the
 *  clipping stuff appears to be handled appropriately, so
 *  if go slightly out of bounds, it still seems to work. */
void CellGrid::drawContents(QPainter* p, int clipx, int clipy, 
			    int clipw, int cliph)
{
    // Do nothing, if clipw or cliph is 0
    if ((clipw <= 0) || (cliph <= 0))
    {
#if 0
	//DEBUG
	printf ("Ignoring zero-sized drawContents: x %i y %i w %i h %i\n",
		clipx, clipy, clipw, cliph);
#endif
	return;
    }

    // Figure out what recordIds/attrIds to paint given the clip coordinates
    // Note: These calls return attrId/recordId ids, which are independent from
    // the current display position of those attrIds/recordIds.
    int firstAttrId = attrIdAt( clipx );
    int lastAttrId = attrIdAt( clipx + clipw - 1);
    int firstRecordId = recordIdAt( clipy );
    int lastRecordId = recordIdAt( clipy + cliph - 1);

#if 0
    // DEBUG
    printf ("drawContents: clipx=%i clipy=%i clipw=%i cliph=%i recordIds %i to %i attrId %i to %i\n",
	    clipx, clipy, clipw, cliph, firstRecordId, lastRecordId, 
	    firstAttrId, lastAttrId);
#endif
    
    // If have no data in this rectangle, paint the out-of-bounds area
    // Get's rid of left over pixels during scrolling.
    if ( firstRecordId == -1 || firstAttrId == -1 ) {
	// Since using real painter, pass clipx and clipy as real
	// position to paint empty error.  See call below for more info.
	paintEmptyArea( p, clipx, clipy, clipw, cliph, clipx, clipy );
	return;
    }

    // Get Painter pixmap's current max width and height
    int pw = paintPixmap.width();
    int ph = paintPixmap.height();
    
    // Grow paintPixmap size as necessary (never shrink), 
    // so that region we are drawin will be covered
    if ((clipw > pw) || (cliph > ph))
    {
	// Get new sizes
	if (clipw > pw)
	    pw = clipw;
	if (cliph > ph)
	    ph = cliph;

//	printf ("Resizing pixmap to %i, %i\n", pw, ph);
	
	// Do resize of pixmap
	paintPixmap.resize(pw, ph);
    }

    // Create painter that draws to pixmap, will bitblt result 
    // at bottom
    QPainter cellPainter(&paintPixmap);
    
    // Set the painter font (must before font metrics are done)
    cellPainter.setFont(p->font());

    // Figure out over what "position" range that we need to redraw
    // to cover the space we are asked to draw.  I.e., we may need to redraw
    // display rows 2-4 to to cover the coordinates, even those those three
    // positions contain attrIds 6,1,3.

    // Get the column for the first attrId we need to redraw
    int firstCol = mapAttrIdToCol(firstAttrId);

    // Get the column for the last attrId we need to redraw
    // If last attrId is out of bounds, repaint until the last attrId
    int lastCol;
    if ( lastAttrId == -1 )
    {
	// The last display column is attrIds() -1.
	lastCol = attrIds() - 1;
    }
    else
    {
	// Map the last attrId to paint to its display position
	lastCol = mapAttrIdToCol(lastAttrId);
    }

    // Get the display position for the first recordId we need to redraw
    int firstRow = mapRecordIdToRow(firstRecordId);

    // Get the display position for the last recordId we need to redraw
    // If last recordId is out of bounds, repaint until the last recordId
    int lastRow;
    if ( lastRecordId == -1 )
    {
	// The last display row is rowsVisible() -1!
	lastRow = rowsVisible() - 1;
    }
    else
    {
	// Map the last recordId to paint to its display position
	lastRow = mapRecordIdToRow(lastRecordId);
    }

    // March through the row and col of each visible cell 
    for (int r=firstRow; r <= lastRow; r++)
    {
	for (int c=firstCol; c <= lastCol; c++)
	{
	    // Map the attrId position back to the actual attrId id
	    // before repainting it
	    int attrId = mapColToAttrId(c);

	    // Map the recordId position back to the actual recordId id 
	    // before repainting it
	    int recordId = mapRowToRecordId(r);

	    // Get x, y coordinates of cell
	    int x = attrIdXPos(attrId);
	    int y = recordIdYPos(recordId);
	    
	    // Get width, height of cell
	    int w = attrIdWidth(attrId);
	    int h = recordIdHeight(recordId);

	    // Disable clipping (reenabled in paintCell)
	    cellPainter.setClipping(FALSE);
	    
	    // Draw the cell (it must draw only in boundarys!)
	    // Drawing into a relative position in pixmap, so
	    // adjust offsets by clipx, and clipy so fall in correct place
	    paintCell(cellPainter, recordId, attrId, x-clipx, y-clipy, w, h,
		      clipx, clipy);
	    
	}
    }

    // Disable clipping before painting empty area
    cellPainter.setClipping(FALSE);

    // Paint empty area on right and/or bottom, if necessary
    // Pass 0,0 at end since clipx,clipy is at 0,0 in cellPainter
    paintEmptyArea( &cellPainter, clipx, clipy, clipw, cliph, 0, 0 );


#if 0
    // Debug, put red rectangle around 
    static QColor blueColor("blue");
    static QColor greenColor("green");
    static QColor purpleColor("purple");
    static QColor orangeColor("orange");
    static int colorCount = 0;
    colorCount++;
    colorCount %= 5;
    switch (colorCount)
    {
      case 0:
	printf ("Red bounding box\n");
	cellPainter.setPen(redColor);
	break;
      case 1:
	printf ("Blue bounding box\n");
	cellPainter.setPen(blueColor);
	break;
      case 2:
	printf ("Green bounding box\n");
	cellPainter.setPen(greenColor);
	break;
      case 3:
	printf ("Purple bounding box\n");
	cellPainter.setPen(purpleColor);
	break;
      case 4:
	printf ("Orange bounding box\n");
	cellPainter.setPen(orangeColor);
	break;
      default:
	printf ("Default Red bounding box\n");
	cellPainter.setPen(redColor);
	break;
    }
    printf ("Contents width %i Contents height %i\n", 
	    contentsWidth(), contentsHeight());
    cellPainter.drawRect(0, 0, clipw, cliph);
#endif
    
    // Finish with pixmap, write it to the screen in the cell 
    cellPainter.end();
    

    p->drawPixmap(clipx, clipy, paintPixmap, 0, 0, clipw, cliph);
}

/* Lays out everything in cell.  Called by paintCell() and cellClicked()
 *  and they need to use the same layout to get the correct results.
 *  If cellValue != NULL, string version of value will be returned.
 *  Use attrId to determine if indentation is needed.
 */
void CellGrid::layoutCell (CellLayout &layout, Cell *cell, int x, int y, 
			   int w, int h, QString *returnCellValue,
			   int recordId, int attrId)
{
    // Get font metrics for cell font
    QFontMetrics fm(defaultCellFont);

    // Calculate where to put the baseline with space evenly distributed.
    int baseLine = y + ((h + 1 - fm.height()) / 2) + fm.ascent() + 1;

    // Reset layout class for this cell's layout
    layout.resetInfo(x, y, w, h, baseLine);

    // If attrId matches treeAttrId, indenting this cell with tree hiearchy
    if ((attrId >= 0) && (attrId == treeAttrId))
    {
	// Calculate indentation level, if no parent, level 0
	int level = 0;
	RecordIdInfo *ri = &recordIdInfo[recordId];
	while (ri->parent != NULL)
	{
	    level++;
	    ri = ri->parent;
	}

	// Tell layout indentation level required
	layout.indentRequired (level, treeAnnotSize, treeIndentation);
    }
    

    // Return now if have NULL cell
    if (cell == NULL)
    {
	// If returning cell's value, return NULL string
	if (returnCellValue != NULL)
	    *returnCellValue = QString::null;


	// I believe cell's look best if text starts at pixel 1 
	// and not at pixel 0.  So, start layout at pixel 1
	// Need to layout the 'nothing' we have.
	layout.calcPlacement (x + 1, TRUE);

	return;
    }

    // Create string to hold cell's value
    QString cellValue;

    // Default text to left justify and numbers to right justify
    bool shouldLeftJustify = TRUE;

    // For text cells, just use contents
    if ((cell->type == textType) || (cell->type == textRefType))
    {
	cellValue = cell->textVal;
	shouldLeftJustify = TRUE;
    }
    // For int cell, convert to string
    else if (cell->type == intType)
    {
	cellValue.sprintf ("%d", cell->intVal);
	shouldLeftJustify = FALSE;
    }
    // For double cell, convert to string
    else if (cell->type == doubleType)
    {
	cellValue.sprintf ("%g", cell->doubleVal);
	shouldLeftJustify = FALSE;
    }
    else 
    {
	TG_error ("CellGrid::layoutCell: cell type %i not yet!\n", 
		  cell->type);
    }


    // Layout the slots needed for the cell's value (if has value)
    if (!cellValue.isNull())
    {
	// Get C version of string we are laying out
	const char *cstring = cellValue.latin1();

	// In order to handle tabs properly, we start need to do
	// a pixel level layout of the start position of each slot
	// as if this string is the only thing in the cell (no annotations)
	int slotStart = 0;


	// Specify slots needed and the width needed by this string
	// Need some trickyness to handle tabs
	for (int index = 0; cstring[index] != 0; index++)
	{
	    // Calculate width of slot into this variable
	    int slotWidth;

	    // If have tab, calculate out slot width that would put us at 
	    // next tabstop (without other annotations)
	    if (cstring[index] == '\t')
	    {
		// Calculate the tab size in pixels, for now 8 space tabs
		int tabWidth = fm.width(' ') * 8;

		// Calculate where next tab stop should be
		int tabStop = ((slotStart/tabWidth) + 1) * tabWidth;

		// Set slot width so next slot starts at tab stop
		slotWidth = tabStop - slotStart;
	    }

	    // Give no space for newlines 
	    else if (cstring[index] == '\n')
	    {
		slotWidth = 0;
	    }

	    // Otherwise, use character width (handle proportional font)
	    else
	    {
		slotWidth = fm.width(cstring[index]);
	    }

	    // Register that we need a slot for this character and
	    // need at least slotWidth
	    layout.slotRequired (index, 0, slotWidth);

	    // Advance slotStart to next start position
	    slotStart += slotWidth;
	}
    }

    // Loop through all cell annotations asking them to specify there
    // slot requirements
    for (CellAnnotNode *annotNode = cell->annotList; annotNode != NULL;
	 annotNode = annotNode->next)
    {
	// Calls virtual function to let annotation specify what
	// layout constraints it is going to put on the cell.
	annotNode->annot->requirements (layout);
    }


    // 
    // At this point, all slot requirements must be set!
    // 

    // FUTURE: put left/right/center justification specification
    // in the style.  Let user pick what they want.

    // Handle left justify case
    if (shouldLeftJustify)
    {
	// I believe cell's look best if text starts at pixel 1 
	// and not at pixel 0.  So, start layout at pixel 1
	layout.calcPlacement (x + 1, TRUE);
    }
    // Otherwise, handle right justify case
    else
    {
	// I believe cell's look best if text starts at one pixel
	// before end of cell.  
	int endPixel = w - 1;
	layout.calcPlacement (x + endPixel, FALSE);
    }

    // If returning cell's value, write it back
    if (returnCellValue != NULL)
	*returnCellValue = cellValue;
}


/* Paints everything in the cell at recordId, attrId 
 *  Handles a pixmap and/or a string at each cell.
 *  Also, allows a grid to be drawn.
 *
 *  Should probably only be called by drawContents. */
void CellGrid::paintCell( QPainter &cellPainter, int recordId, int attrId,
			  int x, int y, int w, int h, int clipx, int clipy)
{
    // Assume cell doesn't contain selected text
    bool cellContainsSelectedText = FALSE;

    // Base cell selected x1, x2 region on global selected region but since
    // we are doing automatic line selection when selecting across 
    // mulitple lines, it is likely that the cell selected region will
    // be totally different in x1 and x2 after being adjusted
    int cellSelectedX1 = selectedX1;
    int cellSelectedX2 = selectedX2;

    // Determine if any text in cell falls in selected range of rows
    // Is part or all of the cell covered by the selected region?
    if (areaSelected && ((y+clipy) < selectedY2) && ((y+clipy+h) > selectedY1))
    {
	// Is last part of cell automatically selected?
	// (Does selection region extend below cell?)
	if (selectedY2 > (y+clipy+h))
	{
	    // Yes, the last part of the cell is automatically selected
	    cellSelectedX2 = x+clipx+w;
	}

	// Is the first part of the cell automatically selected?
	// (Does the selection region start above the cell?)
	if (selectedY1 < (y+clipy))
	{
	    // Yes, the first part of the cell is automatically selected
	    cellSelectedX1 = x+clipx;
	}

	// At this point, it is safe to flip cellSelectedX2 and cellSelectedX1
	// if necessary to make cellSelectedX1 < cellSelectedX2
	if (cellSelectedX2 < cellSelectedX1)
	{
	    int tempX = cellSelectedX1;
	    cellSelectedX1 = cellSelectedX2;
	    cellSelectedX2 = tempX;
	}

	cellContainsSelectedText = TRUE;
    }

    // Get the cell for this location, may not exist
    Cell *cell = getCell (recordId, attrId);

    // Start with default color settings for cell
    QColor *boxColor = defaultStyle->boxColor;
    QColor *bgColor = defaultStyle->bgColor;
    QColor *fgColor = defaultStyle->fgColor;

    // If cell and cell style exists, override colors it sets
    if ((cell != NULL) && (cell->style != NULL))
    {
	CellStyle *style = cell->style;
	if (style->boxColor != NULL)
	    boxColor = style->boxColor;
	if (style->fgColor != NULL)
	    fgColor = style->fgColor;
	if (style->bgColor != NULL)
	    bgColor = style->bgColor;
    }

    // Found for selected text that using the reverse of the defaultStyle
    // looked better than using the revers of the cell's style, so
    // do that
    QColor *selectedBgColor = defaultStyle->fgColor;
    QColor *selectedFgColor = defaultStyle->bgColor;

    // First, erase cell's existing contents using background color
    // This is done before any layers (the far right and bottom 
    // will be filled in with the grid lines)
    cellPainter.fillRect( x, y, w-1, h-1, *bgColor);

    // if grid is on, draw two sides of grid for each cell, in gray,
    // otherwise, if grid is off, do in defaultStyle->bgColor.
    if (gridOn)
    {
	// Make pen gray
	cellPainter.setPen( gray );
    }
    else
    {
	// Make pen default backround color (not the cell's background)
	cellPainter.setPen( *defaultStyle->bgColor );
    }

    // The grid lines is put in the normally "undrawable" pixels on 
    // the far right on the far bottom.  Always draw the grid lines
    // to make filled Rectangles look right 

    // draw line at right side of cell
    cellPainter.drawLine( x+w-1, y+0, x+w-1, y+h-1 );

    // draw line at bottom of cell
    cellPainter.drawLine( x+0, y+h-1, x+w-1, y+h-1 );

    // Subtract 1 from the height and width so the rest of this
    // algorithm will draw only inside the "drawable" pixels in cell
    w--;
    h--;


    // To enforce the drawable area, force clipping to this drawable area.
    cellPainter.setClipRect (x, y, w, h);



    // Let layoutCell calculate the cellValue
    QString cellValue;

    // Layout the cell now, all layout functionality must be in this
    // call so that the cellClicked handler will work properly
    layoutCell (cellLayout, cell, x, y, w, h, &cellValue, recordId, attrId);


    // Get indentation (in pixels) and identation level for this cell
    int indentWidth = cellLayout.getTotalIndent();
    int indentLevel = cellLayout.getIndentLevel();

    // Draw indentation graphics, if necessary for this cell
    if (indentLevel >= 0)
    {
	// Draw open/closed icon if has children or if expandable
	if ((recordIdInfo[recordId].firstChild != NULL) ||
	    (recordIdInfo[recordId].treeExpandable))
	{
	    // Debug, draw things at level 0 only for now
	    int indentPos = cellLayout.getIndentPos(0);
	    if (recordIdInfo[recordId].treeOpen)
	    {
		int startY = cellLayout.calcTopY (SlotVCenter, 
						  openedTreePixmap->height());
		cellPainter.drawPixmap(indentPos, startY, *openedTreePixmap);
	    }
	    else
	    {
		int startY = cellLayout.calcTopY (SlotVCenter, 
						  closedTreePixmap->height());
		cellPainter.drawPixmap(indentPos, startY, *closedTreePixmap);
	    }
	}

	// Center line on opened tree pixmap (should be point, I hope)
	int lineOffset = openedTreePixmap->width()/2;

	if (indentLevel > 0)
	{
	    // Allow line to draw one more pixel down, so
	    // don't get dotted line
	    cellPainter.setClipRect (x, y, w, h+1);
	}

	// Draw lines for all levels greater than 0
	for (int level = indentLevel; level > 0; level--)
	{
	    // Get position of this indent level
	    int indentPos = cellLayout.getIndentPos(level);

	    // Draw 3d line (1 pixel for light, main, and dark colors)
	    int centerX = indentPos+lineOffset;
	    QPen lpen (lightTreeLineColor, 1, Qt::SolidLine,
			Qt::SquareCap, Qt::MiterJoin);

	    cellPainter.setPen (lpen);
	    cellPainter.drawLine (centerX-1, y, 
				  centerX-1, y + h); 

	    lpen.setColor(mainTreeLineColor);
	    cellPainter.setPen (lpen);
	    cellPainter.drawLine (centerX, y, 
				  centerX, y + h); 

	    lpen.setColor(darkTreeLineColor);
	    cellPainter.setPen (lpen);
	    cellPainter.drawLine (centerX+1, y, 
				  centerX+1, y + h); 
	}

	if (indentLevel > 0)
	{
	    // Restore enforcement of the drawable area
	    cellPainter.setClipRect (x, y, w, h);
	}
	
#if 0
	printf ("Indenting recordId %i attrId %i level %i indentWidth %i\n", 
		recordId, attrId, indentLevel, indentWidth);
#endif
    
	// If indent covers entire viewable cell, return now
	if (w < indentWidth)
	    return;

	// To enforce the drawable area, force clipping to 
	// area after indentation
	cellPainter.setClipRect (x+indentWidth, y, w-indentWidth, h);
    }

//    printf ("Drawing RecordId %i AttrId %i at x %i y %i w %i h %i cell %p\n",
//	    recordId, attrId, x, y, w, h, cell);

    //
    // Now draw in cell contents in layers
    // 

    // Get the head of the annotation list, which should be the
    // most negative layer specified.
    CellAnnotNode *annotNode;
    if (cell != NULL)
	annotNode = cell->annotList;
    else
	annotNode = NULL;

    // Draw all the annotations that are below layer 0
    while ((annotNode != NULL) && (annotNode->annot->layer() < 0))
    {
	// Calls virtual function to draw annotation
	annotNode->annot->draw (cellPainter, cellLayout);

	annotNode = annotNode->next;
    }
    
    
    // Draw cell's value just before layer 0 
    // (Could be considered the first thing drawn on layer 0
    if (!cellValue.isNull())
    {
	// Get C version of string we are laying out
	const char *cstring = cellValue.latin1();

	// Get where to put the baseline with space evenly distributed.
	int baseLine = cellLayout.getBaseLine();

	// Get font metrics for cell font
	QFontMetrics fm(defaultCellFont);

	// If cell has selected text, reverse background color of selected
	// characters before draw text loop because some fonts have the 
	// characters extend beyond the slot (i.e. italics).   If we do 
	// this background reversal in the loop (like I originally did), 
	// that part of the font will be chopped off.
	// If cell contains selected text, see if slot does
	// Selected colors made uniform in 2.02 (may not be reversed colors)
	if (cellContainsSelectedText)
	{
	    // Turn off clipping so can reverse color of last line
	    // of pixels (for gridding)
	    cellPainter.setClipping(FALSE);

	    // Reverse selected text backgrounds
	    for (int index = 0; cstring[index] != 0; index++)
	    {
		// Get the width and position of the character slot
		int slotStart = cellLayout.getPos(index, 0);
		int slotWidth = cellLayout.getWidth(index, 0, FALSE);

		// If cell falls in selected region, reverse colors
		if (((slotStart+clipx) < cellSelectedX2) && 
		    ((slotStart+clipx+slotWidth) > cellSelectedX1))
		{
		    // Fill slot with selected style background color
		    cellPainter.fillRect(slotStart, y, slotWidth, h+1, 
					 *selectedBgColor);
		}
	    }
		
	    // Turn back on clipping before drawing text
	    cellPainter.setClipping(TRUE);
	}

	// Now loop through each character and draw characters at placement
	int ch;
	QString cbuf;
	cellPainter.setPen (*fgColor);
	for (int index = 0; cstring[index] != 0; index++)
	{
	    // Get character at this index
	    ch = cstring[index];
	   
	    // Do nothing for tabs and newlines
	    if ((ch == '\t') || (ch == '\n'))
		continue;

	    // Get the width and position of the character slot
	    int slotStart = cellLayout.getPos(index, 0);
	    int slotWidth = cellLayout.getWidth(index, 0, FALSE);
	    int charWidth = fm.width(ch);

	    // Get character into QString buffer
	    cbuf = cstring[index];

	    // Assume slot doesn't contain selected text
	    bool slotContainsSelectedText = FALSE;

	    // If cell contains selected text, see if slot does
	    if (cellContainsSelectedText)
	    {
		// If cell falls in selected region, reverse fg colors
		// (Background reversed outside the loop) 
		if (((slotStart+clipx) < cellSelectedX2) && 
		    ((slotStart+clipx+slotWidth) > cellSelectedX1))
		{
		    // Set pen color to selected foreground color
		    cellPainter.setPen(*selectedFgColor);

		    // Mark that drawing as selected text
		    slotContainsSelectedText = TRUE;
		}
	    }

	    // Use fast drawer, other version slow for printing characters
	    // Use calculated baseline to vertically center font
	    if (slotWidth == charWidth)
		cellPainter.drawText (slotStart, baseLine, cbuf, 1);
	    else
		cellPainter.drawText (slotStart + (slotWidth-charWidth)/2, 
			   baseLine, cbuf, 1);

	    // If slot contained selected text, we changed pen
	    // Restore pen to fg value for next slot (in case not selected)
	    if (slotContainsSelectedText)
	    {
		cellPainter.setPen (*fgColor);
	    }
	}
    }

    // Draw all the annotations that are on layers 0-3
    // Because using same annotNode, should already be on layer >= 0
    while ((annotNode != NULL) && (annotNode->annot->layer() < 3))
    {
	// Calls virtual function to draw annotation
	annotNode->annot->draw (cellPainter, cellLayout);

	annotNode = annotNode->next;
    }
    

    // Place on layer 4, in front of the text
    // Draw box in color if not NULL
    if (boxColor != NULL)
    {
	// Make pen cell's box color
	cellPainter.setPen (*boxColor);

	// Draw rectangle around cell (in drawable area)
	// Handle indentation by moving leftmost side of rectangle
	cellPainter.drawRect (x+indentWidth, y, w-indentWidth, h);
    }

    // Draw all the annotations that are on layers >= 4
    // Because using same annotNode, should already be on layer >= 4
    while ((annotNode != NULL))
    {
	// Calls virtual function to draw annotation
	annotNode->annot->draw (cellPainter, cellLayout);

	annotNode = annotNode->next;
    }

#if 0
    TG_timestamp ("DEBUG highlighting\n");
    static QColor whiteColor("White");
    cellPainter.setPen(whiteColor);
    cellPainter.setRasterOp(Qt::NotROP);
    cellPainter.fillRect(x+indentWidth, y, w-indentWidth, h, whiteColor);
    cellPainter.setRasterOp(Qt::CopyROP);
#endif

}

/* In cases where the window is bigger than the contents area, we
 *  need to paint that section (especially since we set WRepaintNoErase,
 *  which will leave random pixels sets in the outside areas).
 *  Since now allow painter to be pixmap that is offset, use
 *  cx, cy for position calculations but use px, py when actually painting.
 *  Expect px,py to be either cx, cy or 0, 0. */
void CellGrid::paintEmptyArea( QPainter *p, int cx, int cy, int cw, int ch,
			       int px, int py)
{
    // Get the size of the grid, so we can figure out what to paint
    QSize gs = gridSize();
    int gh = gs.height();
    int gw = gs.width();

    // Return now, if there is no empty area to paint
    if ((gw >= cx+cw) && (gh >= cy+ch))
    {
	return;
    }

    // Get color brush to paint empty area with.  
    // For now, use grid Base color (i.e. white) instead of the
    // background color (gray).  It seems to be the convention
    // that other programs use to paint empty areas.
    QBrush emptyBrush = *(defaultStyle->bgColor);

    // If entirely in empty area at right or at bottom, just paint it
    if ((cx >= gw) || (cy >= gh))
    {
	// Fill in background for empty area 
	p->fillRect( px, py, cw, ch, emptyBrush);

	// Done, return now
	return;
    }

    // If bottom is partially filled with the grid, paint empty portion
    if ((cy+ch) > gh)
    {
	// Calculate how far down empty space starts
	int dh = gh - cy;

	// Fill in background for empty area 
	p->fillRect( px, py+dh, cw, ch-dh, emptyBrush);
    }

    // If right side  is partially filled with the grid, paint empty portion
    // May paint bottom, right empty space twice.. Oh well...
    if ((cx+cw) > gw)
    {
	// Calculate how far over empty space starts
	int dw = gw - cx;

	// Fill in background for empty area with grid base color
	p->fillRect( px+dw, py, cw-dw, ch, emptyBrush);
    }
}

/* Registers requirement for screen update.  Will delay update
 *  as long as overlapping updates are required or until flushUpdate() 
 *  is called.  Overlapping areas are combined appropriately.
 *  An update request to a different (non-overlapping) portion of the
 *  screen will flush the current pending update.  
 *
 *  The goal of this routine is reduce the multiple updates to
 *  the same cell, when multiple attributes, etc. are being added.
 *  
 */
void CellGrid::requireUpdate(int x, int y, int w, int h)
{
    // Do nothing if zero sized updates
    // Can happen when nothing in window and when openning/closing 
    // trees with nothing in them
    if ((h <= 0) || (w <= 0))
    {
	// Don't think negative widths should happen
	if ((h < 0) || (w < 0))
	{
	    fprintf (stderr, 
		     "Warning: requireUpdate (x %i, y %i, w %i, h %i)\n",
		    x, y, w, h);
	}
	return;
    }

    // Do nothing if given a box that is all negative
    if (((y+h) <= 0) || ((x+w) <= 0))
    {
	fprintf (stderr, 
		 "Warning: requireUpdate (x %i, y %i, w %i, h %i)\n",
		 x, y, w, h);
	return;
    }
    
    // Do nothing if there is a pending complete update.
    // (Check here instead of first so will always get warnings even
    //  when a complete update is pending!)
    if (completeUpdatePending)
    {
	return;
    }


    // Post flushUpdate event if don't already have a pending update
    if ((!updatePending) && (!completeUpdatePending))
    {
	// Create "vanilla" event with id 1100 to indicate flushUpdate
	QCustomEvent *ce = new QCustomEvent (1100);

#if 0
	// DEBUG
	const char *winName = name();
	if (winName != NULL)
	{
	    TG_timestamp ("requireUpdate: "
			  "posting flushUpdate for %s\n", winName);
	}
	else
	{
	    TG_timestamp ("requireUpdate: Posting flushUpdate\n");;
	}
#endif

	// Submit flush event for this grid after current update if finished
	// QT will delete ce when done
	QApplication::postEvent (this, ce); 
    }
#if 0
    // DEBUG
    else
    {
	const char *winName = name();
	if (winName != NULL)
	{
	    TG_timestamp ("requireUpdate: "
			  "already posted flushUpdate for %s\n", winName);
	}
	else
	{
	    TG_timestamp ("requireUpdate: already posted flushUpdate\n");;
	}
    }
#endif


    // If pending request does not overlap this one, flush pending
    // request before processing this one
    if (updatePending &&  
	((x >= updateX2) || (y >= updateY2) ||
	 ((x+w) <= updateX1) || ((y+h) <= updateY1)))
    {
#if 0
	// Debug 
	printf ("requestUpdate: flushing x1 %i x2 %i y1 %i y2 %i with "
		"%i %i %i %i\n",
		updateX1, updateX2, updateY1, updateY2, x, x+w, y, y+h);
#endif
	
	flushUpdate();
    }

    // If no pending request, just cache this one
    // Note X2, Y2 are one past where they should be to make the
    // math easy (no -1).  Will take into account when flushing update.
    if (!updatePending)
    {
#if 0
	// Debug 
	printf ("requestUpdate: caching x1 %i x2 %i y1 %i y2 %i\n",
		x, x+w, y, y+h);
#endif

	updateX1 = x;
	updateX2 = x + w;
	updateY1 = y;
	updateY2 = y + h;
	updatePending = TRUE;
    }
    // Otherwise, expand rectangle as appropriate
    else
    {
#if 0
	// Debug 
	printf ("requestUpdate: combining x1 %i x2 %i y1 %i y2 %i with "
		"%i %i %i %i\n",
		updateX1, updateX2, updateY1, updateY2, x, x+w, y, y+h);
#endif

	if (x < updateX1)
	    updateX1 = x;
	if ((x+w) > updateX2)
	    updateX2 = x+w;
	if (y < updateY1)
	    updateY1 = y;
	if ((y+h) > updateY2)
	    updateY2 = y+h;
    }

}

// If update pending, calls updateContents() to update screen
void CellGrid::flushUpdate()
{
    // Flush pending update registered by requireUpdate()
    if (updatePending || completeUpdatePending)
    {
	// Flush any pending placement array reconstructions
	flushReconstruction();

	// Flush any pending grid size syncs
	flushSync();

	// If doing complete update, set update range to entire visible
	// region of the cellGrid.
	if (completeUpdatePending)
	{
#if 0
	    static int count = 0;
	    count++;
	    // DEBUG
	    TG_timestamp ("%s: doingCompleteUpdate(), times called=%i\n",
			  name(), count);
#endif

	    updateX1 = contentsX();
	    updateX2 = updateX1 + visibleWidth();
	    updateY1 = contentsY();
	    updateY2 = updateY1 + visibleHeight();
	}

	// Updates will be flushed, must do before updateContents
	// so it will not try to nullify this update!
	updatePending = FALSE;
	completeUpdatePending = FALSE;

	// Qt 2.3's updateContents causes redrawing of
	// the last visible line even if the area to
	// be updated is off screen (probably for compatibility reasons).  
	// This can burn a lot of time if that line has a lot of
	// graphics on it.  So, filter out as many unnecessary
	// update contents as possible.
	// Note, X2/Y2 is be calculated without the -1, to match
	// updateX2/Y2.
	int visibleX1 = contentsX();
	int visibleX2 = visibleX1 + visibleWidth();
	int visibleY1 = contentsY();
	int visibleY2 = visibleY1 + visibleHeight();

	// If update not visible, do nothing (don't call updateContents)
	if ((visibleX2 <= updateX1) || (updateX2 <= visibleX1) ||
	    (visibleY2 <= updateY1) || (updateY2 <= visibleY1))
	{
#if 0
	// DEBUG
	    printf ("flushUpdate (%i->%i, %i->%i) discarded, only (%i->%i, %i->%i) visible!\n", 
		    updateX1, updateX2-1, updateY1, updateY2-1,
		    visibleX1, visibleX2-1, visibleY1, visibleY2-1);
#endif
	    return;
	}

	// It appears QT handles much bigger updateContents areas than
	// necessary properly, but since I have the info, I am
	// going to limit the update to the visible area. -JCG 4/12/04
	if (updateX1 < visibleX1)
	    updateX1 = visibleX1;
	if (updateX2 > visibleX2)
	    visibleX2 = updateX2;
	if (updateY1 < visibleY1)
	    updateY1 = visibleY1;
	if (updateY2 > visibleY2)
	    updateY2 = visibleY2;

#if 0
	// DEBUG
	const char *winName = name();
	if (winName != NULL)
	{
	    TG_timestamp ("flushUpdate(%s): x1 %i x2 %i y1 %i y2 %i "
			  "(visible %i %i %i %i)\n", 
			  winName, 
			  updateX1, updateX2, updateY1, updateY2,
			  visibleX1, visibleX2, visibleY1, visibleY2);

	}
	else
	{
	    TG_timestamp ("flushUpdate: x1 %i x2 %i y1 %i y2 %i "
			  "(visible %i %i %i %i)\n", 
			  updateX1, updateX2, updateY1, updateY2,
			  visibleX1, visibleX2, visibleY1, visibleY2);
	}

#endif


	// updateContents gives much better performance for common
	// tasks (than repaint Contents) but can cause graphical
	// defects when the screen scrolls between the time update
	// is called and it is actually performed!

	// Extract w, h from x2, y2.  Note the +1 is not necessary
	// since x2, y2 where calculated with an inplicit +1 (no -1)
	updateContents (updateX1, updateY1, 
			updateX2 - updateX1, updateY2 - updateY1);
	

    }
    else
    {
#if 0
	// DEBUG
	const char *winName = name();
	if (winName != NULL)
	{
	    TG_timestamp ("flushUpdate(%s): nothing to flush\n", winName);
	}
	else
	{
	    TG_timestamp ("flushUpdate: nothing to flush\n");
	}
#endif
    }

}

/* Register requirement for a complete screen update.  Will delay complete
 * update until flushUpdate() is called.  Combines appropriately with
 * calls to requireUpdate() (basically everything will be updated
 * next flushUpdate, so combines will all requireUpdate calls).
 * Very useful when resizing the grid, the screen, or calling
 * placeRecordId().
 */
void CellGrid::requireCompleteUpdate()
{
#if 0
    // DEBUG
    TG_timestamp ("CellGrid::requireCompleteUpdate\n");
#endif

    // post flushUpdate event if don't already have a pending update
    if ((!updatePending) && (!completeUpdatePending))
    {
	// Create "vanilla" event with id 1100 to indicate flushUpdate
	QCustomEvent *ce = new QCustomEvent (1100);

#if 0
	// DEBUG
	const char *winName = name();
	if (winName != NULL)
	{
	    TG_timestamp ("requireCompleteUpdate: "
			  "posting flushUpdate for %s\n", winName);
	}
	else
	{
	    TG_timestamp ("requireCompleteUpdate: Posting flushUpdate\n");;
	}
#endif

	// Submit flush event for this grid after current update if finished
	// QT will delete ce when done
	QApplication::postEvent (this, ce); 
    }

    // Indicate a complete update is pending
    completeUpdatePending = TRUE;
}

// Sets the visibility of the header
// Adding for messageFolder support
void CellGrid::setHeaderVisible (bool visible)
{
    topHeaderVisible = visible;

    if (visible)
    {
	setMargins( 0, fontMetrics().height() + 4, 0, 0 );
	topHeader->show();
    }
    else
    {
	topHeader->hide();
	setMargins( 0, 0, 0, 0 );
    }

    updateGeometries();
}

// Sets attrId header text.  Ignore's out of bounds attrIds.
void CellGrid::setHeaderText( int attrId, const QString &text )
{
    // Ignore out of bounds attrIds.
    if ((attrId < 0) || (attrId >= numAttrIds))
	return;

    // Set the header text in the give attrId.
    topHeader->setLabel(attrId, text);
}

// Sets attrId header iconset and text.  Ignore's out of bounds attrIds.
void CellGrid::setHeaderText( int attrId, const QIconSet &iconset, 
			      const QString &text)
{
    // Ignore out of bounds attrIds.
    if ((attrId < 0) || (attrId >= numAttrIds))
	return;

    // Set the header iconset and text in the give attrId.
    topHeader->setLabel(attrId, iconset, text);
}

// Returns header text.  Returns NULL_QSTRING for out of bounds attrId
QString CellGrid::headerText( int attrId ) const
{
    // Return QString::null out of bounds attrIds.
    if ((attrId < 0) || (attrId >= numAttrIds))
	return (QString::null);

    // Return the header text in this attrId
    return (topHeader->label(attrId));
}

// Returns header iconset.  Returns NULL if no iconSet or out of bounds attrId
QIconSet *CellGrid::headerIconSet (int attrId) const
{
    // Return QString::null out of bounds attrIds.
    if ((attrId < 0) || (attrId >= numAttrIds))
	return (NULL);

    // Return the header iconSet in this attrId
    return (topHeader->iconSet(attrId));
}

// Sets attrId header width.  Ignore's out of bounds attrIds or widths
void CellGrid::setHeaderWidth( int attrId, int pixels )
{
    // Ignore out of bounds attrIds.
    if ((attrId < 0) || (attrId >= numAttrIds))
	return;

    // Require set to at least 1 pixel
    if (pixels <= 0)
	pixels = 1;

    // Get the current header width 
    int oldWidth = topHeader->sectionSize (attrId);

    // Return now if already set to this
    if (pixels == oldWidth)
	return;

    // Resize the attrId header (which resizes the attrId)
    topHeader->resizeSection( attrId, pixels );

    // Call the user's resize event handler to fix up all the demensions
    attrIdWidthChanged( attrId, 0, 0);
}

// Returns the actual attrId which is displayed at col
// Return -1 for out of bounds col values
int CellGrid::mapColToAttrId(int col) const
{
    // Managed by attrId header, use it's mapping routine
    return (topHeader->mapToSection(col));
}

// Returns the col at which the 'attrId' is currently displayed 
// Return -1 for out of bounds attrId values
int CellGrid::mapAttrIdToCol(int attrId) const
{
    // Managed by attrId header, use it's mapping routine
    return (topHeader->mapToIndex(attrId));
}

// Returns the recordId which is displayed at 'row'
// Return -1 for out of bounds row values
int CellGrid::mapRowToRecordId(int row)
{
    // Return -1 for out of bounds row values
    if ((row < 0) || (row >= numRowsVisible))
	return (-1);

    // Flush any outstanding reconstructions before reading row
    flushReconstruction();

    // Return value out of mapping array (assume correctly maintained)
    return (row2RecordId[row]);
}

// Returns the row at which the 'recordId' is currently displayed 
// Return -1 for out of bounds recordId values
int CellGrid::mapRecordIdToRow(int recordId)
{
    // Return -1 for out of bounds recordId values
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (-1);

    // Flush any outstanding reconstructions before reading row
    flushReconstruction();

    // Return value out of mapping array (assume correctly maintained)
    return (recordIdInfo[recordId].row);
}


// Returns the cell pointer at recordId,attrId.  NULL if doesn't exist or
// out of bounds.  Does not allocate cells!  See getCreatedCell().
CellGrid::Cell *CellGrid::getCell(int recordId, int attrId) const
{
    // Return NULL, if recordId/attrId out of bounds
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (Cell *)0;
    if ((attrId < 0) || (attrId >= numAttrIds))
	return (Cell *)0;

    // Get and return cell, if any at this location from table
    Cell *cell = cellTable.findEntry (recordId, attrId);
    return (cell);
}

// Returns the cell pointer at recordId, attrId (if it exists), otherwise
// creates a new "empty" cell at that location and returns a pointer to it.
// Returns NULL if recordId/attrId is out of bounds.
CellGrid::Cell *CellGrid::getCreatedCell(int recordId, int attrId) 
{
    // Return NULL, if recordId/attrId out of bounds
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (Cell *)0;
    if ((attrId < 0) || (attrId >= numAttrIds))
	return (Cell *)0;

    Cell *cell;

    // Get cell and create if doesn't exist
    if ((cell = cellTable.findEntry(recordId, attrId)) == NULL)
    {
	// Doesn't exist, create cell and add it to the table
	cell = new Cell();
	TG_checkAlloc(cell);
	cellTable.addEntry(recordId, attrId, cell);
    }

    // Return the cell
    return (cell);
}

// Returns the slot width in pixels.  Returns 0 if slot doesn't exist
// or there is no width requirements on slot.
int CellGrid::getSlotWidth (int recordId, int attrId, int index, int slot)
{
    // Get cell pointer from grid (may be NULL)
    Cell *cell = getCell (recordId, attrId);

    // If have cell, see what we are over exactly (may be cell annotation)
    if (cell != NULL)
    {
	// Get actual height and width of cell (including 1 pixel border)
	int cellw = attrIdWidth (attrId);
	int cellh = recordIdHeight (recordId);

	// Layout the cell now, pass 1 less pixel for width/height to make
	// match paintCell call (border pixels)
	layoutCell (cellLayout, cell, 0, 0, cellw-1, cellh-1);

	return (cellLayout.getWidth (index, slot, TRUE));
    }
    // Otherwise, return 0
    return (0);
}

// Returns the actual width (in pixels) used by the cell (when drawn).
// Note that not all of the pixels may be currently visible
// due to the attrid's header width.  This call actually does
// cell layout, so somewhat expensive.
// Return 0 for out of bounds cells.
int CellGrid::cellWidth (int recordId, int attrId)
{
    // Get cell pointer from grid (may be NULL)
    Cell *cell = getCell (recordId, attrId);

    // If have cell, see what layout width is
    if (cell != NULL)
    {
	// Get actual height and width of cell (including 1 pixel border)
	int cellw = attrIdWidth (attrId);
	int cellh = recordIdHeight (recordId);

	// Layout the cell now, pass 1 less pixel for width/height to make
	// match paintCell call (border pixels)
	// Must pass recordId and attrId if want accurate indent info
	layoutCell (cellLayout, cell, 0, 0, cellw-1, cellh-1, 
		    NULL, recordId, attrId);

	// Get indentation (in pixels) and identation level for this cell
	int indentWidth = cellLayout.getTotalIndent();

	// Get the text/graphics width of cell (ignores current cell width)
	// Doesn't appear to count indentation in this width
	int cellWidth = cellLayout.getTotalWidth ();

	// Get the total width of the cell, add one pixel for "grid"
	// and add one pixel so last pixel of text doesn't merge with
	// the "grid" (it also makes highlighting look correct)
	return (indentWidth + cellWidth + 2);
    }

    // Otherwise, return 0
    return (0);
}


// Returns number of annotations in the cell with the given handle
// Returns 0 if cell doesn't exist or handle does not occur.  
// Lineaerly scan each annotation to determine number of times exists.
int CellGrid::handleExist (int recordId, int attrId, int handle)
{
    int handleCount = 0;

    // Get cell pointer from grid (may be NULL)
    Cell *cell = getCell (recordId, attrId);

    // If have cell, does it have this handle?
    if (cell != NULL)
    {
	// Scan thru all cell annotations
	for (CellAnnotNode *annotNode = cell->annotList; annotNode != NULL;
	     annotNode = annotNode->next)
	{
	    // Does it have my handle?
	    if (annotNode->annot->handle() == handle)
	    {
		handleCount++;
	    }
	}
    }

    // Return the handle count
    return (handleCount);
}

// Returns the maximum annotation handle in the cell
// Returns NULL_INT if no annotations in the cell
// Linearly scans each annotation to determine max handle.
int CellGrid::maxHandle (int recordId, int attrId)
{
    // Get cell pointer from grid (may be NULL)
    Cell *cell = getCell (recordId, attrId);

    // If have not cell, return NULL_INT, no max handle
    if (cell == NULL)
	return (NULL_INT);

    // If no annotation, return NULL_INT, no max handle
    if (cell->annotList == NULL)
	return (NULL_INT);

    // Get the first annotation
    CellAnnotNode *firstAnnotNode = cell->annotList;

    // Use it to seed maxHandle
    int maxHandle = firstAnnotNode->annot->handle();

    // Scan thru all the rest of the cell annotations
    for (CellAnnotNode *annotNode = firstAnnotNode->next; annotNode != NULL;
	 annotNode = annotNode->next)
    {
	// Get this annotations handle
	int scanHandle = annotNode->annot->handle();

	// Update max, if necessary
	if (scanHandle > maxHandle)
	    maxHandle = scanHandle;
    }

    // Return the max handle
    return (maxHandle);
}


// Adds pixmap at the given index and slot with the given position flags
// If replace == TRUE, removes all annotations with 'handle' before adding
void CellGrid::addCellPixmapAnnot (int recordId, int attrId, int handle,
				   bool replace, int layer, bool clickable,
				   int index, int slot, int posFlags, 
				   GridPixmapId pixmapId)
{
    // Remove existing annotations with this handle if replace == TRUE
    // Don't redraw screen (last parm FALSE)
    if (replace)
	removeCellAnnot (recordId, attrId, handle, FALSE);

    // Look up GridPixmap for id using internal routine
    GridPixmap *gridPixmap = getPixmap (pixmapId);

    // If NULL, punt
    if (gridPixmap == NULL)
	TG_error ("CellGrid::addCellPixmapAnnot: pixmapId %i invalid!\n", 
		  pixmapId);
    
    // Allocate annotation using pointer to pixmap
    CellPixmapAnnot *annot = new CellPixmapAnnot (handle, layer, clickable,
						  index, slot, posFlags, 
						  &gridPixmap->pixmap);
    TG_checkAlloc(annot);


    // Add pixmap to cell, which updates screen
    addCellAnnot ( recordId, attrId, annot);
}

// Adds pixmap at the given index and slot with the given position flags
// Version that takes pixmap name instead of GridPixmapId
// If replace == TRUE, removes all annotations with 'handle' before adding
void CellGrid::addCellPixmapAnnot (int recordId, int attrId, int handle,
				   bool replace, int layer, bool clickable,
				   int index, int slot, int posFlags, 
				   const QString &pixmapName)
{
    // Remove existing annotations with this handle if replace == TRUE
    // Don't redraw screen (last parm FALSE)
    if (replace)
	removeCellAnnot (recordId, attrId, handle, FALSE);

    // Look up GridPixmap for id using internal routine
    GridPixmap *gridPixmap = getPixmap (pixmapName);

    // If NULL, punt
    if (gridPixmap == NULL)
	TG_error ("CellGrid::addCellPixmapAnnot: pixmapName %s invalid!\n", 
		  pixmapName.latin1());
    
    // Allocate annotation using pointer to pixmap
    CellPixmapAnnot *annot = new CellPixmapAnnot (handle, layer, clickable,
						  index, slot, posFlags, 
						  &gridPixmap->pixmap);
    TG_checkAlloc(annot);

    // Add pixmap to cell, which updates screen
    addCellAnnot ( recordId, attrId, annot);
}

// Makes clickable range in cell (nothing drawn for annotation)
void CellGrid::addCellClickableAnnot (int recordId, int attrId, int handle,
				      bool replace, int layer, 
				      int startIndex, int startSlot,
				      int stopIndex, int stopSlot)
{
    // Remove existing annotations with this handle if replace == TRUE
    // Don't redraw screen (last parm FALSE)
    if (replace)
	removeCellAnnot (recordId, attrId, handle, FALSE);
    
    // Allocate annotation using pointer to line
    CellClickableAnnot *annot = new CellClickableAnnot (handle, layer,
							startIndex, startSlot, 
							stopIndex, stopSlot);
    TG_checkAlloc(annot);

    // Add line to cell, which updates screen (unnecessarily in this case)
    addCellAnnot ( recordId, attrId, annot);
}

// Adds line between the two points give with the given color.
/* If color is QColor() (an invalid color), the line is not actually drawn
  but is still potentially clickable and does reserve space at endpoints.
  If replace == TRUE, removes all annotations with 'handle' before adding
  May want to add arrow types, etc info in future
*/
void CellGrid::addCellLineAnnot (int recordId, int attrId, int handle, 
				 bool replace, int layer, bool clickable,
				 int startIndex, int startSlot, 
				 int startPosFlags, 
				 int stopIndex, int stopSlot,
				 int stopPosFlags, const QColor &color,
				 int lineWidth)
{
    // Remove existing annotations with this handle if replace == TRUE
    // Don't redraw screen (last parm FALSE)
    if (replace)
	removeCellAnnot (recordId, attrId, handle, FALSE);

    
    // Allocate annotation using pointer to line
    CellLineAnnot *annot = new CellLineAnnot (handle, layer, clickable,
					      startIndex, startSlot, 
					      startPosFlags, 
					      stopIndex, stopSlot,
					      stopPosFlags, color,
					      lineWidth);
    TG_checkAlloc(annot);
						

    // Add line to cell, which updates screen
    addCellAnnot ( recordId, attrId, annot);
}

// Adds rectagle between the two corner points give with the given 
// line color, and width, and an optional fill color.
/* If either color is QColor() (an invalid color), that portion of
  of the rectagle is not drawn (or filled in) but is still potentially 
  clickable and does reserve space at endpoints.
  If replace == TRUE, removes all annotations with 'handle' before adding
*/
void CellGrid::addCellRectAnnot (int recordId, int attrId, int handle, 
				 bool replace, int layer, bool clickable,
				 int startIndex, int startSlot, 
				 int startPosFlags, 
				 int stopIndex, int stopSlot,
				 int stopPosFlags, const QColor &lineColor,
				 int lineWidth, const QColor &fillColor)
{
    // Remove existing annotations with this handle if replace == TRUE
    // Don't redraw screen (last parm FALSE)
    if (replace)
	removeCellAnnot (recordId, attrId, handle, FALSE);

    
    // Allocate annotation using pointer to rectangle
    CellRectAnnot *annot = new CellRectAnnot (handle, layer, clickable,
					      startIndex, startSlot, 
					      startPosFlags, 
					      stopIndex, stopSlot,
					      stopPosFlags, lineColor,
					      lineWidth, fillColor);
    TG_checkAlloc(annot);
		
    // Add rectangle to cell, which updates screen
    addCellAnnot ( recordId, attrId,  annot);
}

// Adds ellipse between the two corner points give with the given 
// line color, and width, and an optional fill color.
/* If either color is QColor() (an invalid color), that portion of
  of the ellipse is not drawn (or filled in) but is still potentially 
  clickable and does reserve space at endpoints.
  If replace == TRUE, removes all annotations with 'handle' before adding
*/
void CellGrid::addCellEllipseAnnot (int recordId, int attrId, int handle, 
				   bool replace, int layer, bool clickable,
				   int startIndex, int startSlot, 
				   int startPosFlags, 
				   int stopIndex, int stopSlot,
				   int stopPosFlags, const QColor &lineColor,
				   int lineWidth, const QColor &fillColor)
{
    // Remove existing annotations with this handle if replace == TRUE
    // Don't redraw screen (last parm FALSE)
    if (replace)
	removeCellAnnot (recordId, attrId, handle, FALSE);

    
    // Allocate annotation using pointer to ellipse
    CellEllipseAnnot *annot = new CellEllipseAnnot (handle, layer, clickable,
						    startIndex, startSlot, 
						    startPosFlags, 
						    stopIndex, stopSlot,
						    stopPosFlags, lineColor,
						    lineWidth, fillColor);
    TG_checkAlloc(annot);
		
    // Add ellipse to cell, which updates screen
    addCellAnnot ( recordId, attrId,  annot);
}

/*
 Routine to add custom annotation to cell (derived from annot interface) 
 Doesn't add out of bounds recordId/attrId coordinates and deletes annot
 if coordinates invalid.  Doesn't copy annot and will delete annot
 when annotation removed. Blindly adds annotation regardless of handle.  
 If don't want multiple annotations with same handle, must call 
 deleteCellAnnot() routine first. Redraws cell after adding annotation. */
void CellGrid::addCellAnnot(int recordId, int attrId, CellAnnot *annot)

{
    // Get cell at recordId, attrId.  Create if necessary
    Cell *cell = getCreatedCell(recordId, attrId);

    // Ignore out of bounds requests, delete annot to prevent memory leak
    if (cell == (Cell *)0)
    {
	delete annot;
        return;
    }
    
    // Add annotation to cell
    addCellAnnot (cell, annot);

    // Make sure screen is updated, if visible and complete update
    // is not already pending (otherwise recordIdYPos can cause
    // expensive placement array reconstruction to occur)
    if (!completeUpdatePending && !recordIdHidden(recordId))
    {
	requireUpdate( attrIdXPos(attrId), recordIdYPos(recordId),
		       attrIdWidth(attrId), recordIdHeight(recordId) );
    }
}

/*
 External routine to remove all annotations from cell with the given handle
 Ignores out of bounds recordId/attrId coordinates.
 If redraw TRUE and something deleted, redraws cell without annotations.
 Set redraw to FALSE if calling this routine in addCellAnnot routine.
 Returns number of annotations deleted. */
int CellGrid::removeCellAnnot(int recordId, int attrId, int handle,
			      bool redraw)
{
    // Get the cell pointer for the specified location
    Cell *cell = getCell(recordId, attrId);

    // Return 0 items deleted if cell doesn't exist
    if (cell == NULL)
	return (0);

    // Remove annotates with matching handle, get number deleted
    int numDeleted = removeCellAnnot (cell, handle);

    // If 1 or more deleted and redraw allowed, update cell
    if (redraw && (numDeleted > 0))
    {
	// Make sure screen is updated, if visible and complete update
	// is not already pending (otherwise recordIdYPos can cause
	// expensive placement array reconstruction to occur)
	if (!completeUpdatePending && !recordIdHidden(recordId))
	{
	    requireUpdate( attrIdXPos(attrId), recordIdYPos(recordId),
			   attrIdWidth(attrId), recordIdHeight(recordId) );
	}
    }
    
    return (numDeleted);
}


// Internal routine to add annotation to cell (used by external version)
// Does not redraw cell.  Does not check for duplicate handles
void CellGrid::addCellAnnot(Cell *cell, CellAnnot *annot)
{
    // Get layer of annotation (will determine insertion placement
    int layer = annot->layer();

    // Create Node to hold pointer to annotation
    CellAnnotNode *newNode = new CellAnnotNode (annot);
    TG_checkAlloc(newNode);

    // Find existing annotation to place this one before
    // For now, put lowest layers at front of list.
    // In case of ties, put newNode after those of the same layer
    // This is the order that the annotates should be drawn.
    // It is the reverse order from how they should be resolved when 
    // clicked on.
    
    CellAnnotNode *afterNode = NULL;
    CellAnnotNode *beforeNode = cell->annotList;
    while ((beforeNode != NULL) && (beforeNode->annot->layer() <= layer))
    {
	// After this node, advance pointers
	afterNode = beforeNode;
	beforeNode = beforeNode->next;
    }
    
    // Place newNode between afterNode and beforeNode
    newNode->prev = afterNode;
    newNode->next = beforeNode;
    if (afterNode != NULL)
	afterNode->next = newNode;
    else
	cell->annotList = newNode;
    if (beforeNode != NULL)
	beforeNode->prev = newNode;
}

// Internal routine to remove annotations with handle from cell
// Does not redraw cell.  Will remove all annotations with handle.
// Returns number of annotations deleted.
int CellGrid::removeCellAnnot(Cell *cell, int handle)
{
    int count = 0;

    // Scan annotList, deleting all nodes that have this handle
    CellAnnotNode *node = cell->annotList;
    while (node != NULL)
    {
	// Get next node before deleting anything
	CellAnnotNode *nextNode = node->next;

	// If annotation matches this handle, delete it
	if (node->annot->handle() == handle)
	{
	    // Update delete count
	    count++;

	    // Remove node from linked list
	    if (node->prev != NULL)
		node->prev->next = node->next;
	    else
		cell->annotList = node->next;
	    
	    if (node->next != NULL)
		node->next->prev = node->prev;
	    
	    // Delete note which will delete annot also
	    delete node;
	}

	// Get next node to scan
	node = nextNode;
    }
    
    // Return number of annotations deleted
    return (count);
}


// Get the cell's data type, returns invalidType if empty or never created
CellGrid::CellType CellGrid::cellType ( int recordId, int attrId) const
{
    // Get the cell pointer for the specified location
    Cell *cell = getCell(recordId, attrId);

    // If have cell allocated for that location, return it's type
    if (cell != NULL)
    {
	return (cell->type);
    }
    // Otherwise, return invalidType
    else
    {
	return (invalidType);
    }
}

// Invalidates cell value (if any)
void CellGrid::clearCellValue( int recordId, int attrId)
{
    // Get the cell pointer for the specified location (do not create)
    Cell *cell = getCell(recordId, attrId);

    // Do nothing, if cell doesn't exist
    if (cell == NULL)
	return;

    // If string, free the space
    if (cell->type == textType)
	delete [] cell->textVal;

    // Set type to invalid
    cell->type = invalidType;

    // If watching this cell for changes, flag watchpoint trigger
    if ((recordId == watchRecordId) && (attrId == watchAttrId))
	watchpointTriggered = TRUE;
}


// Returns the text that will be displayed for the cell (may be
// string, double, or int type).  Useful if need to know how
// a number is being printed out.
QString CellGrid::cellScreenText( int recordId, int attrId ) const
{
    // Get the cell pointer for the specified location
    Cell *cell = getCell(recordId, attrId);

    // If have cell allocated for that location convert to a string
    // (if not already) and return it
    if (cell != NULL)
    { 
	// Return string (Qstring type conversion automatic)
	if ((cell->type == textType) || (cell->type == textRefType))
	{
	    return cell->textVal;
	}
	// Return double how it will be displayed on the screen
	else if (cell->type == intType)
	{
	    QString buf;
	    buf.sprintf ("%d", cell->intVal);
	    return (buf);
	}
	// Return double how it will be displayed on the screen
	else if (cell->type == doubleType)
	{
	    QString buf;
	    buf.sprintf ("%g", cell->doubleVal);
	    return (buf);
	}
    }

    // Otherwise, return null
    return QString::null;
}

// Returns the cell's text or Qstring::null if no text (i.e., if int or double)
QString CellGrid::cellText( int recordId, int attrId ) const
{
    // Get the cell pointer for the specified location
    Cell *cell = getCell(recordId, attrId);

    // If have cell allocated for that location convert to a string
    // (if not already) and return it
    if (cell != NULL)
    { 
	// Return string (Qstring type conversion automatic)
	if ((cell->type == textType) || (cell->type == textRefType))
	{
	    return cell->textVal;
	}
    }

    // Otherwise, return null
    return QString::null;
}

// Returns the cell's int value or NULL_INT if int not stored in cell
int CellGrid::cellInt( int recordId, int attrId ) const
{
    // Get the cell pointer for the specified location
    Cell *cell = getCell(recordId, attrId);

    // If have cell allocated, see if int
    if (cell != NULL)
    { 
	// If int, return int value 
	if (cell->type == intType)
	{
	    return cell->intVal;
	}
    }

    // Otherwise, return NULL_INT
    return NULL_INT;
}

// Returns the cell's double value or NULL_DOUBLE if double not stored in cell
double CellGrid::cellDouble( int recordId, int attrId ) const
{
    // Get the cell pointer for the specified location
    Cell *cell = getCell(recordId, attrId);

    // If have cell allocated, see if double
    if (cell != NULL)
    { 
	// If double, return double value 
	if (cell->type == doubleType)
	{
	    return cell->doubleVal;
	}
    }

    // Otherwise, return NULL_DOUBLE
    return NULL_DOUBLE;
}

// Sets cell's text at recordId, attrId.  
// Ignore's out of bounds recordId/attrId coordinates.
// Cell can only have one value, setting to double, etc. will destroy string
void CellGrid::setCellText( int recordId, int attrId, const char *text )
{
    // Get cell at recordId, attrId.  Create if necessary
    Cell *cell = getCreatedCell(recordId, attrId);

    // Ignore out of bounds requests
    if (cell == (Cell *)0)
	return;

    // Free existing contents, if necessary
    if (cell->type == textType)
    {
	// Use array destructor for char array
	delete [] cell->textVal;

	// Sanity check
	cell->textVal = NULL;
    }

    // Set cell type
    cell->type = textType;

    // Get length of incoming text
    int len = strlen (text);
    
    // Allocate memory to hold string
    char *textVal = new char[len+1];
    TG_checkAlloc(textVal);

    // Copy over contents to new buffer
    strcpy (textVal, text);

    // Place in cell textVal const char * pointer
    cell->textVal = textVal;

    // Make sure screen is updated, if visible and complete update
    // is not already pending (otherwise recordIdYPos can cause
    // expensive placement array reconstruction to occur)
    if (!completeUpdatePending && !recordIdHidden(recordId))
    {
	requireUpdate( attrIdXPos(attrId), recordIdYPos(recordId), 
		       attrIdWidth(attrId), recordIdHeight(recordId) );
    }

    // If watching this cell for changes, flag watchpoint trigger
    if ((recordId == watchRecordId) && (attrId == watchAttrId))
	watchpointTriggered = TRUE;
}

//! QString version for setCellText() for user's convenience 
void CellGrid::setCellText( int recordId, int attrId, const QString &text )
{
    // Call the const char * version
    setCellText (recordId, attrId, text.latin1());
}

// Sets cell's text using a reference to "stable memory" (memory
// that will not be changed or freed while the cell points at
// it) at recordId, attrId.  Saves a copy of the pointer, not
// the buffer, so faster and uses less memory, but will die
// if the "stable memory" changes or is freed.
// Ignores out of bounds recordId/attrId coordinates.
// Cell can only have one value, setting to double/int will 
// remove reference to string
void CellGrid::setCellTextRef( int recordId, int attrId, const char *text )
{
    // Get cell at recordId, attrId.  Create if necessary
    Cell *cell = getCreatedCell(recordId, attrId);

    // Ignore out of bounds requests
    if (cell == (Cell *)0)
	return;

    // Free existing contents, if necessary
    if (cell->type == textType)
    {
	// Use array destructor for char array
	delete [] cell->textVal;

	// Sanity check
	cell->textVal = NULL;
    }

    // Set cell type
    cell->type = textRefType;

    // Place pointer in cell textVal const char * pointer
    cell->textVal = text;

    // Make sure screen is updated, if visible and complete update
    // is not already pending (otherwise recordIdYPos can cause
    // expensive placement array reconstruction to occur)
    if (!completeUpdatePending && !recordIdHidden(recordId))
    {
	requireUpdate( attrIdXPos(attrId), recordIdYPos(recordId), 
		       attrIdWidth(attrId), recordIdHeight(recordId) );
    }

    // If watching this cell for changes, flag watchpoint trigger
    if ((recordId == watchRecordId) && (attrId == watchAttrId))
	watchpointTriggered = TRUE;
}


// Sets cell's value to integer at recordId, attrId.  
// Printed to screen using sprintf's "%d" format.
// Ignore's out of bounds recordId/attrId coordinates.
// Cell can only have one value, setting to string, etc. will destroy int
void CellGrid::setCellInt( int recordId, int attrId, int value )
{
    // Get cell at recordId, attrId.  Create if necessary
    Cell *cell = getCreatedCell(recordId, attrId);

    // Ignore out of bounds requests
    if (cell == (Cell *)0)
	return;

    // Free existing contents, if necessary
    if (cell->type == textType)
    {
	// Use array destructor for char array
	delete [] cell->textVal;

	// Sanity check
	cell->textVal = NULL;
    }

    // Set cell type
    cell->type = intType;

    // Place value in cell
    cell->intVal = value;

    // Make sure screen is updated, if visible and complete update
    // is not already pending (otherwise recordIdYPos can cause
    // expensive placement array reconstruction to occur)
    if (!completeUpdatePending && !recordIdHidden(recordId))
    {
	requireUpdate( attrIdXPos(attrId), recordIdYPos(recordId), 
		       attrIdWidth(attrId), recordIdHeight(recordId) );
    }

    // If watching this cell for changes, flag watchpoint trigger
    if ((recordId == watchRecordId) && (attrId == watchAttrId))
	watchpointTriggered = TRUE;
}

//! Sets cell's value to a double at recordId, attrId.  
//! Printed to screen using sprintf's "%g" format.
//! Ignore's out of bounds recordId/attrId coordinates.
//! Cell can only have one value, setting to int/string will destroy double
void CellGrid::setCellDouble( int recordId, int attrId, double value )
{
    // Get cell at recordId, attrId.  Create if necessary
    Cell *cell = getCreatedCell(recordId, attrId);

    // Ignore out of bounds requests
    if (cell == (Cell *)0)
	return;

    // Free existing contents, if necessary
    if (cell->type == textType)
    {
	// Use array destructor for char array
	delete [] cell->textVal;

	// Sanity check
	cell->textVal = NULL;
    }

    // Set cell type
    cell->type = doubleType;

    // Place value in cell
    cell->doubleVal = value;

    // Make sure screen is updated, if visible and complete update
    // is not already pending (otherwise recordIdYPos can cause
    // expensive placement array reconstruction to occur)
    if (!completeUpdatePending && !recordIdHidden(recordId))
    {
	requireUpdate( attrIdXPos(attrId), recordIdYPos(recordId), 
		       attrIdWidth(attrId), recordIdHeight(recordId) );
    }

    // If watching this cell for changes, flag watchpoint trigger
    if ((recordId == watchRecordId) && (attrId == watchAttrId))
	watchpointTriggered = TRUE;
}


// Registers a new pixmap with the grid.  The returned id then can
// be used to set a cell's value to a pixmap.  
// Now can also look up by name, so will punt on duplicate names!
GridPixmapId CellGrid::newGridPixmap (const QPixmap &pixmap, 
				      const QString &name)
{
    // Get next available pixmap id
    GridPixmapId pixmapId = maxPixmapId + 1;

    // Update max pixmap id
    maxPixmapId = pixmapId;

    // Create structure to hold pixmap, constructor sets default values
    GridPixmap *newPixmap = new GridPixmap(pixmap, pixmapId, name);
    TG_checkAlloc(newPixmap);

    // Insert id to pixmap mapping into pixmap table
    pixmapIdTable.addEntry((int)pixmapId, newPixmap);

    // Also allow lookup by pixmap name
    pixmapNameTable.addEntry((char *)name.latin1(), newPixmap);

    // Return the new pixmap's id
    return (pixmapId);
}


// Creates a new style which can be used to set the style for a row or cell
// Styles are the *only* way to set colors, fonts, etc. for a row or cell
CellStyleId CellGrid::newCellStyle (const QString &name)
{
    // Get next available style id
    CellStyleId styleId = maxStyleId + 1;

    // Update max style id
    maxStyleId = styleId;

    // Create structure to hold style, constructor sets default values
    CellStyle *newStyle = new CellStyle(name, styleId);
    TG_checkAlloc(newStyle);

    // Insert id to style mapping into style table
    styleTable.addEntry((int)styleId, newStyle);

    // Return the new style's id
    return (styleId);
}

// Internal routine to set pointer to style color pointer appropriately 
void CellGrid::setStyleColor (QColor * &styleColor, const QColor &color)
{
    // If color invalid (QColor() passed), free up existing color if any
    // at set to NULL to specify the default behavior for that item should
    // be used
    if (!color.isValid())
    {
	if (styleColor != NULL)
	{
	    delete styleColor;
	    styleColor = NULL;
	}
    }
    // Otherwise, set style color, allocating style color if necessary
    else
    {
	// Handle case where need to allocate color
	if (styleColor == NULL)
	{
	    styleColor = new QColor(color);
	    TG_checkAlloc(styleColor);
	}
	// Handles case where need to change existing color to new color
	else
	{
	    *(styleColor) = color;
	}
    }
}

// Note: QColor("Red"), etc. talks to the X server each time
// the constructor is called, which can be long-latency.
// Create each color only once!

// Set box color, passing QColor() indicates default behavior should be used
void CellGrid::setStyleBoxColor(CellStyleId styleId, const QColor &color)
{
    // Use internal routine to look up style pointer
    CellStyle *cellStyle = getStyle (styleId);

    // Sanity check, make sure style found (return with warning if not)
    if (cellStyle == NULL)
    {
	fprintf (stderr, 
		 "Warning CellGrid::setStyleBoxColor: style %i not found!\n",
		 (int)styleId);
	return;
    }

    // Use internal routine to set box color
    setStyleColor(cellStyle->boxColor, color);
}

// Set fg color, passing QColor() indicates default behavior should be used
void CellGrid::setStyleFgColor(CellStyleId styleId, const QColor &color)
{
    // Use internal routine to look up style pointer
    CellStyle *cellStyle = getStyle (styleId);

    // Sanity check, make sure style found (return with warning if not)
    if (cellStyle == NULL)
    {
	fprintf (stderr, 
		 "Warning CellGrid::setStyleFgColor: style %i not found!\n",
		 (int)styleId);
	return;
    }

    // Use internal routine to set box color
    setStyleColor(cellStyle->fgColor, color);
}

// Set bg color, passing QColor() indicates default behavior should be used
void CellGrid::setStyleBgColor(CellStyleId styleId, const QColor &color)
{
    // Use internal routine to look up style pointer
    CellStyle *cellStyle = getStyle (styleId);

    // Sanity check, make sure style found (return with warning if not)
    if (cellStyle == NULL)
    {
	fprintf (stderr, 
		 "Warning CellGrid::setStyleBgColor: style %i not found!\n",
		 (int)styleId);
	return;
    }

    // Use internal routine to set box color
    setStyleColor(cellStyle->bgColor, color);
}

// Sets the display style for the cell, pass CellStyleInvalid to clear setting
void CellGrid::setCellStyle (int recordId, int attrId, CellStyleId styleId)
{
    // Get cell at recordId, attrId.  Create if necessary
    Cell *cell = getCreatedCell(recordId, attrId);

    // Ignore out of bounds requests
    if (cell == (Cell *)0)
	return;

    // Ignore requests if already set to styleId
    if ((cell->style != NULL) && (cell->style->styleId == styleId))
	return;

    // Ignore clear style requests if already cleared
    if ((styleId == CellStyleInvalid) && (cell->style == NULL))
	return;

    // Set the cell style directly, will clear style if styleId invalid
    cell->style = getStyle (styleId);

    // Make sure screen is updated, if visible and complete update
    // is not already pending (otherwise recordIdYPos can cause
    // expensive placement array reconstruction to occur)
    if (!completeUpdatePending && !recordIdHidden(recordId))
    {
	requireUpdate( attrIdXPos(attrId), recordIdYPos(recordId), 
		       attrIdWidth(attrId), recordIdHeight(recordId) );
    }
}

// Returns styleId, returns CellStyleInvalid if not set
CellStyleId CellGrid::cellStyle (int recordId, int attrId)
{
    // Get the cell pointer for the specified location
    Cell *cell = getCell(recordId, attrId);

    // If have cell and style for that location, return style Id
    if ((cell != NULL) && (cell->style != NULL))
    {
	// Return the style id from style struct
	return (cell->style->styleId);
    }
    // Otherwise, return CellStyleInvalid
    else
    {
	return CellStyleInvalid;
    }
}

// Resets cell back to default settings (no contents, etc.)
void CellGrid::resetCell (int recordId, int attrId)
{
    // Only reset cell if actually exists (to prevent needless updates)
    if (getCell(recordId, attrId) != NULL)
    {
	// Delete cell (does nothing if not there)
	cellTable.deleteEntry (recordId, attrId);

	// Make sure screen is updated, if visible and complete update
	// is not already pending (otherwise recordIdYPos can cause
	// expensive placement array reconstruction to occur)
	if (!completeUpdatePending && !recordIdHidden(recordId))
	{
	    requireUpdate( attrIdXPos(attrId), recordIdYPos(recordId), 
			   attrIdWidth(attrId), recordIdHeight(recordId) );
	}
    }
}

// Override Scrollview function to allow us to
// adjust window geometries.  This event (unlike CellGrid::resizeEvent)
// also catches when scrollbars appear and disappear!
void CellGrid::viewportResizeEvent ( QResizeEvent *e )
{
    QScrollView::resizeEvent( e );

#if 0
    QSize size = e->size();
    fprintf (stderr, "CellGrid::viewportResizeEvent (width %i, height %i)\n",
	     size.width(), size.height());
#endif

    // Adjust top header, etc for current sizes.
    updateGeometries();

    // Let interested listeners know the grid has been resized 
    emit gridResizeEvent(e);
}

#if 0
// Believe that viewportResizeEvent() the correct thing
// to override, not resizeEvent.  This didn't handle
// scrollbars appearing and disappearing correctly!
// Override Scrollview function to allow us to
// adjust window geometries.
void CellGrid::resizeEvent( QResizeEvent *e )
{
    QScrollView::resizeEvent( e );

    // DEBUG
    fprintf (stderr, "CellGrid::resizeEvent()\n");

    // Adjust top header, etc for current sizes.
    updateGeometries();

    // Let interested listeners know the grid has been resized 
    emit gridResizeEvent(e);
}
#endif

// Handle attrIds moving around.  
// Redraw from the smallest col number passed.
void CellGrid::attrIdMoved(int, int fromCol, int toCol)
{
    // Get the X  position of the attrId farthest to the right (smallest index)
    // cellPos() expects actual attrId ids, not attrId position ids, so map to 
    // actual attrId ids first!
    int redrawXPos;
    if (fromCol < toCol)
	redrawXPos = attrIdXPos(mapColToAttrId(fromCol));
    else
	redrawXPos = attrIdXPos(mapColToAttrId(toCol));

    // Tell scrollview to redraw everything from that position to the right
    // before control returned to the user
    requireUpdate(redrawXPos, 0, 
		  contentsWidth()-redrawXPos, contentsHeight() );

    // Adjust top header, etc for current sizes.
    updateGeometries();
}

// Overload virtual function to catch mouse movements
void CellGrid::contentsMouseMoveEvent (QMouseEvent* e)
{
    // Grab the focus to this widget when move over (used to be in pressEvent)
    setFocus();

    // Use generic mouse event handler to handle
    // all the different types of mouse events
    MouseEventHandler (e);
}

// Overload virtual funtion to handle single clicks
void CellGrid::contentsMousePressEvent(QMouseEvent* e)
{
    // Don't  need to grab focus here, since moveEvent will do it

    // Use generic mouse event handler to handle
    // all the different types of mouse events
    MouseEventHandler (e);
}


// Overload virtual funtion to handle double clicks
void CellGrid::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    // Don't  need to grab focus here, since moveEvent will do it

    // Use generic mouse event handler to handle
    // all the different types of mouse events
    MouseEventHandler (e);
}

// Overload virtual funtion to handle button release events
void CellGrid::contentsMouseReleaseEvent(QMouseEvent* e)
{
    // Don't  need to grab focus here, since moveEvent will do it

    // Use generic mouse event handler to handle
    // all the different types of mouse events
    MouseEventHandler (e);
}

// Handles clipboard selection change events
void CellGrid::clipboardChangedHandler()
{
    // Only care if this cellgrid widget has something currently selected
    if (areaSelected)
    {
	// This will be called when this widget sets the clipboard, so 
	// only care if clipboard contents are different from what this
	// widget set it to.
	if ((strcmp(selectedText.contents(), 
		    (const char *)clipboard->text(QClipboard::Selection)) != 0))
	{
#if 0
	    TG_timestamp ("clipboard reports changed to '%s'\n",
			  (const char *)clipboard->text (QClipboard::Selection));
#endif
	    // Clear previous selection by calling with old 'start point'
	    // Don't allow scrolling due to this 'clear' operation
	    updateSelection (startSelectX, startSelectY, FALSE);

	    // Sanity check, clear fields
	    areaSelected = FALSE;
	    selectedX1 = -1;
	    selectedX2 = -1;
	    selectedY1 = -1;
	    selectedY2 = -1;
	    startSelectX = -1;
	    startSelectY = -1;
	    scrollDX = 0;
	    scrollDY = 0;
	    currentSelectX = 0;
	    currentSelectY = 0;
	}
    }
}

// Handles scrollTimer ticks to scroll text during text selection
void CellGrid::scrollTimerHandler()
{
#if 0
    TG_timestamp ("scrollTimerHandler called with dx %i dy %i\n",
		  scrollDX, scrollDY);
#endif

    // If have a scroll setting, scroll cellgrid by that amount
    // Sometimes this scroll handler is called after the scroll events
    // should occur, so must guard any action by testing scrollDX and scrollDY
    if ((scrollDX != 0) || (scrollDY != 0))
    {
	// Scroll the window
	scrollBy (scrollDX, scrollDY);

	// In order to update the selection bar, call updateSelection
	// with a new projected mouse position due to the scrolling
	int newX = currentSelectX + scrollDX;
	int newY = currentSelectY + scrollDY;

	// Bound this new mouse position within the legal bounds of
	// the cell grid to prevent a exponential scroll feedback loop
	// once we hit the end of the cell grid
	if (newX < 0)
	    newX = 0;
	if (newX > contentsWidth())
	    newX = contentsWidth();
	if (newY < 0)
	    newY = 0;
	if (newY > contentsHeight())
	    newY = contentsHeight();

	// Update the selection due to the scrolling.   Need to
	// use projected new position to make scrolling continue.
	updateSelection (newX, newY, TRUE);
    }
}

// Internal routine to update display of selected text based on
// mouse move to x,y.  Used by mouse handler in a few spots.
// Should only be called when mouse moving or being released with
// left button pushed down.
// noScroll indicates whether the screen should scroll based on mouse location
void CellGrid::updateSelection (int x, int y, bool allowScroll)
{
    // Get the bounds on the visible area in the window
    int visibleX1 = contentsX();
    int visibleX2 = visibleX1 + visibleWidth();
    int visibleY1 = contentsY();
    int visibleY2 = visibleY1 + visibleHeight();

    // Figure out how much to scroll that would make the point off screen
    // visible
    scrollDX = 0;
    scrollDY = 0;
   
    // In case we do scroll, record the latest mouse position
    currentSelectX = x;
    currentSelectY = y;

    // Bound x,y to be within visible area, otherwise get strange
    // results from selection of areas.   Also determine how much
    // we need to scroll to bring the out of bounds point into range.
    if (x < visibleX1)
    {
	scrollDX = x - visibleX1;
	x = visibleX1;
    }
    else if (x > visibleX2)
    {
	scrollDX = x - visibleX2;
	x = visibleX2;
    }
    if (y < visibleY1)
    {
	scrollDY = y - visibleY1;
	y = visibleY1;
    }
    else if (y > visibleY2)
    {
	scrollDY = y - visibleY2;
	y = visibleY2;
    }

    // If not in scroll mode, suppress any scroll decisions
    if (!allowScroll)
    {
	scrollDX = 0;
	scrollDY = 0;
    }

    // Do we have a pending scroll?
    if ((scrollDX != 0) || (scrollDY != 0))
    {
	// Yes, scroll timer if not already going
	if (!scrollTimer->isActive())
	{
	    // Scroll 10 times a second for now
	    scrollTimer->start(100);
	}
    }
    else
    {
	// No, stop scroll timer if going
	if (scrollTimer->isActive())
	{
	    scrollTimer->stop();
	}
    }

    // Keep track of how much of the vertical axis needs to be update
    // (may be reducing selection range, so take min/max of old and
    //  new values to determine what to update.)
    int minY = y;
    int maxY = y;
    
    // Update minY and maxY with old values if >= 0
    if (selectedY1 >= 0) 
    {
	if (selectedY1 < minY)
	    minY = selectedY1;
	if (selectedY1 > maxY)
	    maxY = selectedY1;
    }
    if (selectedY2 >= 0)
    {
	if (selectedY2 < minY)
	    minY = selectedY2;
	if (selectedY2 > maxY)
	    maxY = selectedY2;
    }
    // Also update minY and maxY with the new startSelectY
    if (startSelectY >= 0)
    {
	if (startSelectY < minY)
	    minY = startSelectY;
	if (startSelectY > maxY)
	    maxY = startSelectY;
    }
    
    // Have we moved from initial spot?
    if ((startSelectX != -1) && (startSelectY != -1) && 
	((startSelectX != x) || (startSelectY != y)))
    {
	// Adjust X1, X2, Y1, Y2 so that Y1 < Y2 (move them in pairs)
	// Note X1 may be > or < than X2
	if (startSelectY > y)
	{
	    selectedX1 = x;
	    selectedY1 = y;
	    selectedX2 = startSelectX;
	    selectedY2 = startSelectY;
	}
	else
	{
	    selectedX1 = startSelectX;
	    selectedY1 = startSelectY;
	    selectedX2 = x;
	    selectedY2 = y;
	}
#if 0
	TG_timestamp ("Selecting coordinates x1 %i y1 %i x2 %i y2 %i!\n",
		      selectedX1, selectedY1, selectedX2, selectedY2);
#endif

	// Mark that selected area should be drawn when screen updated
	areaSelected = TRUE;
    }

    // If have not moved, mark as not selected
    else
    {
	// Sanity check, set values to -1
	selectedY1 = -1;
	selectedY2 = -1;
	selectedX1 = -1;
	selectedX2 = -1;

	// Mark that no selected area should be drawn
	areaSelected = FALSE;
    }
	
    // Update the entire width of the screen for selected rows
    int minUpdateX = 0;
    int maxUpdateX = contentsWidth();
    
    // Update just the rows covered by minY to maxY
    int minRecordId, maxRecordId, minUpdateY, maxUpdateY;
    
    // Get the starting Y position of row starting at minY
    minRecordId = recordIdAt (minY);
    if (minRecordId >= 0)
    {
	// Update from top of minRecordId row
	minUpdateY = recordIdYPos(minRecordId);
    }
    else
    {
	// Update to top of screen
	minUpdateY = 0;
    }
    
    // Get ending Y position of row starting at maxY 
    maxRecordId = recordIdAt (maxY);
    if (maxRecordId >= 0)
    {
	// Update to bottom of maxRecordId row
	maxUpdateY = recordIdYPos(maxRecordId) + 
	    recordIdHeight(maxRecordId);
    }
    else
    {
	// Update to bottom of screen
	maxUpdateY = contentsHeight();
    }
    
#if 0    
    TG_timestamp ("requireUpdate X %i Y %i X2 %i y2 %i\n",
		  minUpdateX, minUpdateY, maxUpdateX, maxUpdateY);
#endif
    
    // When mouse event returns, require update of area that is now highlighted
    // or was highlighted and now is not.
    requireUpdate (minUpdateX, minUpdateY,   maxUpdateX - minUpdateX, 
		   maxUpdateY- minUpdateY);
}

// Internal routine that grabs the text in the selection area and 
// places it in selectedText. Clears selectedText if nothing is seleted.
void CellGrid::updateSelectedText()
{
    // Clear existing selectedText buffer
    selectedText.clear();

    // Figure out over what rows that we need to grab the selected text from.
    // I.e., we may need to grab text from display rows 2-4 to to cover the
    // coordinates, even those those three positions contain recordIds 6,1,3.

    // Get the display position for the first recordId we need to redraw
    int firstRecordId = recordIdAt (selectedY1);
    int firstRow;
    if (firstRecordId == -1)
    {
	// If nothing selected, pick end point for starting point
	// This handles corner case where empty area is slected
	firstRow = rowsVisible() - 1;
    }
    else
    {
	firstRow= mapRecordIdToRow(firstRecordId);
    }

    // Get the display position for the last recordId we need to redraw
    // If last recordId is out of bounds, repaint until the last recordId
    // (Use -1 offset to handle row boundary correctly.)
    int lastRecordId = recordIdAt (selectedY2-1);
    int lastRow;
    if ( lastRecordId == -1 )
    {
	// The last display row is rowsVisible() -1!
	lastRow = rowsVisible() - 1;
    }
    else
    {
	// Map the last recordId to paint to its display position
	lastRow = mapRecordIdToRow(lastRecordId);
    }

    // Scan all visible columns
    int firstCol = 0;
    // The last display column is attrIds() -1.
    int lastCol = attrIds() - 1;

    // March through the row and col of each visible cell 
    for (int r=firstRow; r <= lastRow; r++)
    {
	for (int c=firstCol; c <= lastCol; c++)
	{
	    // Map the attrId position back to the actual attrId id
	    // before repainting it
	    int attrId = mapColToAttrId(c);

	    // Map the recordId position back to the actual recordId id 
	    // before repainting it
	    int recordId = mapRowToRecordId(r);

	    // Get x, y coordinates of cell
	    int x = attrIdXPos(attrId);
	    int y = recordIdYPos(recordId);

	    // Get width, height of cell
            int w = attrIdWidth(attrId);
            int h = recordIdHeight(recordId);

	    // Get the cell for this location, may not exist
	    Cell *cell = getCell (recordId, attrId);

	    // Let layoutCell calculate the cellValue
	    QString cellValue;

	    // Layout the cell now, all layout functionality must be in this
	    // call so that the cellClicked handler will work properly
	    layoutCell (cellLayout, cell, x, y, w, h, &cellValue, recordId, 
			attrId);

	    // Assume cell doesn't contain selected text
	    bool cellContainsSelectedText = FALSE;

	    // Base cell selected x1, x2 region on global selected region but 
	    // since we are doing automatic line selection when selecting 
	    // across mulitple lines, it is likely that the cell selected 
	    // region will be totally different in x1 and x2 after being 
	    // adjusted
	    int cellSelectedX1 = selectedX1;
	    int cellSelectedX2 = selectedX2;
	    
	    // Determine if any text in cell falls in selected row
	    if (areaSelected &&	((y) < selectedY2) && ((y+h) > selectedY1))
	    {
		// Is last part of cell automatically selected?
		// (Does selection region extend below cell?)
		if (selectedY2 > (y+h))
		{
		    // Yes, the last part of the cell is automatically selected
		    cellSelectedX2 = x+w;
		}
		
		// Is the first part of the cell automatically selected?
		// (Does the selection region start above the cell?)
		if (selectedY1 < (y))
		{
		    // Yes, first part of the cell is automatically selected
		    cellSelectedX1 = x;
		}

		// At this point, it is safe to flip cellSelectedX2 and
		// cellSelectedX1 if necessary to make 
		// cellSelectedX1 < cellSelectedX2
		if (cellSelectedX2 < cellSelectedX1)
		{
		    int tempX = cellSelectedX1;
		    cellSelectedX1 = cellSelectedX2;
		    cellSelectedX2 = tempX;
		}

		cellContainsSelectedText = TRUE;
	    }

	    if (cellContainsSelectedText && !cellValue.isNull())
	    {
		// Get C version of string we are laying out
		const char *cstring = cellValue.latin1();

		// Copy selected characters to selectedText buffer
		for (int index = 0; cstring[index] != 0; index++)
		{
		    // Get the width and position of the character slot
		    int slotStart = cellLayout.getPos(index, 0);
		    int slotWidth = cellLayout.getWidth(index, 0, FALSE);
		    
		    // If cell falls in selected region, append character
		    if (((slotStart) < cellSelectedX2) &&
			((slotStart+slotWidth) > cellSelectedX1))
		    {
			// Append selected character
			selectedText.appendChar(cstring[index]);
		    }
		}
	    }
	}

	// If not last row, and not empty, add newline to selected text
	// The strlen() > 0 handles initial empty area being in selected region
	if ((r < lastRow) && selectedText.strlen() > 0)
	{
	    selectedText.appendSprintf("\n");
	}
    }
#if 0
    TG_timestamp ("Selected:\n'%s'\n", selectedText.contents());
#endif
}


// This handler assumes passed contents mouse events
// Currently handles mouse single click, double click,
// and move events
void CellGrid::MouseEventHandler (QMouseEvent *e)
{
    int indentLevel;

    // Get x, y coords of click for convience
    int x = e->x();
    int y = e->y();

    // Map to actual indexes (reordering doesn't matter)
    int recordId = recordIdAt(y);
    int attrId = attrIdAt(x);

    // Get mouse event type
    QEvent::Type type = e->type();

    // Put out a general notice that this grid has been clicked
    if( type == QEvent::MouseButtonPress ) 
    {
#if 0
	// DEBUG
	TG_timestamp ("CellGrid::MouseEventHandler: got mouse press at "
		      "%i, %i!\n", x, y);
#endif

	// If area previously selected, clear area 
	if (areaSelected)
	{
	    // Clear previous selection by calling with old 'start point'
	    // Don't allow scrolling due to this 'clear' operation
	    updateSelection (startSelectX, startSelectY, FALSE);

	    // Sanity check, clear fields
	    areaSelected = FALSE;
	    selectedX1 = -1;
	    selectedX2 = -1;
	    selectedY1 = -1;
	    selectedY2 = -1;
	}

	// Record start position for drag (ignoring button for now)
	startSelectX = x;
	startSelectY = y;
	currentSelectX = x;
	currentSelectY = y;
	scrollDX = 0;
	scrollDY = 0;

	emit gridClicked (this);
    }
    else if (type == QEvent::MouseButtonDblClick)
    {
#if 0
	// DEBUG
	TG_timestamp ("CellGrid::MouseEventHandler: got doubleclick at "
		      "%i, %i!\n", x, y);
#endif

	// May need to clear selected area here in the future

	emit gridDoubleClicked (this);
    }
    else if (type == QEvent::MouseMove)
    {
	ButtonState moveButtonState = e->state();

	if (moveButtonState & LeftButton)
	{
#if 0
	    // DEBUG
	    TG_timestamp ("CellGrid::MouseEventHandler: got Moved over at "
			  "%i, %i with buttons %x!\n", x, y, (int)moveButtonState);

#endif
	    // Update the drawing of the selected region with current
	    // mouse position.  If hit same position, will turn itself off.
	    // Allow scrolling if mouse goes off area
	    updateSelection (x, y, TRUE);
	}
	emit gridMovedOver (this);
    }
    else if (type == QEvent::MouseButtonRelease)
    {
#if 0
	// DEBUG
	TG_timestamp ("CellGrid::MouseEventHandler: button released at "
		      "%i, %i!\n", x, y);
#endif
	
	// Get button state before release
	ButtonState releaseButtonState = e->state();

	// If released left button, may be left selection
	if (releaseButtonState & LeftButton)
	{
	    // Update the drawing of the selected region with current
	    // mouse position.  If hit same position, will turn itself off.
	    // Do not scroll due to this update
	    updateSelection (x, y, FALSE);

	    // Was there an area actually selected?
	    if (areaSelected)
	    {
		// Grab selected text into selectedText buffer
		updateSelectedText();

		// Set the clipboard (selection mode) to the selected text
		QClipboard *cb = QApplication::clipboard();
		cb->setText (selectedText.contents(), QClipboard::Selection);

		// COME BACK AND COPY TEXT
	    }
	}

	// Clear any pending scrolling
	scrollDX = 0;
	scrollDY = 0;

	// Turn off any scroll timers that may be active
	if (scrollTimer->isActive())
	{
	    scrollTimer->stop();
	}
    }

    else
    {
	fprintf (stderr, "CellGrid::MouseEventHandler:"
		 "Unknown/unhandled general mouse event %i!\n", (int)type);
    }

    // Get what button state will be after this event.
    // NOTE: This is a bit map ORing together button and control/shift/alt
    // key info.  Don't do a == to this.
    // May need to grab state before/after depending on type.
    ButtonState buttonState = e->stateAfter();

    // For now, ignore mouse events outside of defined grid area
    // but emit signal in case someone cares (-1 passed for recordId or
    // attrId that is invalid)
    if ((recordId < 0) || (attrId < 0))
    {
	// Handle mouse click events
	if (type == QEvent::MouseButtonPress)
	{
	    // Emit signal that mouse over emptyArea (where no cells live)
	    emit emptyAreaClicked (recordId, attrId, buttonState, x, y);
	}

	// Handle mouse double click events
	else if (type == QEvent::MouseButtonDblClick)
	{
	    // Emit signal that mouse over emptyArea (where no cells live)
	    emit emptyAreaDoubleClicked (recordId, attrId, buttonState, x, y);
	}
	
	// Handle mouse movement events
	// Pass same stuff as mouse click events
	else if (type == QEvent::MouseMove)
	{
	    // Emit signal that mouse over emptyArea (where no cells live)
	    emit emptyAreaMovedOver (recordId, attrId, buttonState, x, y);
	}
	else if (type == QEvent::MouseButtonRelease)
	{
#if 0
	// DEBUG
	TG_timestamp ("CellGrid::MouseEventHandler: empty area? button released at "
		      "%i, %i!\n", x, y);
#endif
	}
	// Sanity check
	else
	{
	    fprintf (stderr, "CellGrid::MouseEventHandler:"
		     "Unknown/unhandled mouse event for emptyArea!\n");
	}
	return;
    }

    // Get starting x,y address for cell
    int startx = attrIdXPos (attrId);
    int starty = recordIdYPos (recordId);

    // Calculate relative x,y coordinates in cell
    int cellx = x - startx;
    int celly = y - starty;

    // Get actual height and width of cell (including 1 pixel border)
    int cellw = attrIdWidth (attrId);
    int cellh = recordIdHeight (recordId);
    
    // Initially set index/slot to NULL_INT to specify nothing in cell hit
    int index = NULL_INT;
    int slot = NULL_INT;

    // Get cell pointer from grid (may be NULL)
    Cell *cell = getCell (recordId, attrId);

    // Layout the cell now, pass 1 less pixel for width/height to make
    // match paintCell call (border pixels)
    // 'cell' may be NULL
    layoutCell (cellLayout, cell, 0, 0, cellw-1, cellh-1, NULL, 
		recordId, attrId);

    // Handle case where cursor over tree hiearchy annotations
    // Explicitly excluded out of bounds recordIds although layoutCell
    // should handle this properly anyways.
    if ((recordId >= 0) && 
	(cellLayout.pixelToIndentLevel (cellx, indentLevel)))
    {
	// Get recordInfo for this record
	RecordIdInfo *recordInfo = & recordIdInfo[recordId];


	// Find parent recordId associated with this pixel
	// Initially, set to -1 (no tree parent associated with this location)
	int parentRecordId = -1;

	// If level == 0 and this record has children or is expandable,
	// it is the tree parent for this pixel
	if ((indentLevel == 0) && 
	    ((recordInfo->firstChild != NULL) || recordInfo->treeExpandable))
	{
	    parentRecordId = recordId;
	}
	// Otherwise, find parent record id associated with this pixel
	// If level > 0 (otherwise if case above does nothing!)
	else if (indentLevel > 0)
	{
	    // Climb parent tree 'indentLevel' times to find parent
	    RecordIdInfo *parentInfo = recordInfo;
	    for (int i =0; i < indentLevel; i++)
	    {
		// Sanity check, don't dereference NULL pointers
		if (parentInfo != NULL)
		    parentInfo = parentInfo->parent;
	    }
	    // Sanity check, parentInfo should never be NULL
	    if (parentInfo == NULL)
	    {
		TG_error ("CellGrid::MouseEventHandler: Algorithm error, "
			  "parentInfo NULL after climbing tree!");
	    }
	    // Get parentRecordId from parentInfo found above
	    parentRecordId = parentInfo->recordId;
	}

	// Determine if tree is open for parent
	// Initially, set to 1 and fixed up later if possible
	bool treeOpen = 1;
	
	// Read parent tree state, if possible
	if (parentRecordId != -1)
	    treeOpen = (bool)recordIdInfo[parentRecordId].treeOpen;

	// Handle mouse click events
	if (type == QEvent::MouseButtonPress)
	{
	    emit cellTreeClicked (recordId, attrId, buttonState, 
				  indentLevel, parentRecordId, treeOpen,
				  cellx, celly, cellw, cellh);
	}

	// Handle mouse double click events
	else if (type == QEvent::MouseButtonDblClick)
	{
	    emit cellTreeDoubleClicked (recordId, attrId, buttonState, 
					indentLevel, parentRecordId, treeOpen,
					cellx, celly, cellw, cellh);
	}
	
	// Handle mouse movement events
	// Pass same stuff as mouse click events
	else if (type == QEvent::MouseMove)
	{
	    emit cellTreeMovedOver (recordId, attrId, buttonState,
				    indentLevel, parentRecordId, treeOpen,
				    cellx, celly, cellw, cellh);
	}
	else if (type == QEvent::MouseButtonRelease)
	{
#if 0
	// DEBUG
	TG_timestamp ("CellGrid::MouseEventHandler: cellTree button released at "
		      "%i, %i!\n", x, y);
#endif
	}
	// Sanity check
	else
	{
	    fprintf (stderr, "CellGrid::MouseEventHandler:"
		     "Unknown/unhandled mouse event for cellTree!\n");
	}

	// If signaled tree event, don't also signal 
	// the cell slot event
	return;
    }

    // If have cell, see what we are over exactly (may be cell annotation)
    if (cell != NULL)
    {
	// Convert cellx position to index and slot, returns TRUE if found
	// If hit cell, detect if over clickable annotation
	if (cellLayout.pixelToLocation (cellx, index, slot))
	{
	    // Get last cell annotation in list, so can start scan from
	    // last annotation at the highest layer
	    CellAnnotNode *annotNode = cell->annotList;
	    while ((annotNode != NULL) && (annotNode->next != NULL))
		annotNode = annotNode->next;

	    // Get the click annotation to assign this event to
	    while (annotNode != NULL)
	    {
		// Break out of loop if found clickable object at this location
		if (annotNode->annot->clickableAt(index, slot))
		    break;

		// Go to the next lower annotation
		annotNode = annotNode->prev;
	    }

	    // Handle case where mouse over clickable object
	    if (annotNode != NULL)
	    {
		// Get handle and layer for object
		int handle = annotNode->annot->handle();
		int layer = annotNode->annot->layer();

		// Handle mouse click events
		if (type == QEvent::MouseButtonPress)
		{

		    // Signal to any connected listener that a cell annotation
		    // has been clicked on.  In addition to the info passed
		    // to cellSlotClicked (see below), pass the handle and
		    // layer for the object clicked.
		    emit cellAnnotClicked (recordId, attrId, buttonState, 
					   handle, layer, index, slot, 
					   cellx, celly, cellw, cellh);
		}

		// Handle mouse click events
		else if (type == QEvent::MouseButtonDblClick)
		{

		    // Signal to any connected listener that a cell annotation
		    // has been clicked on.  In addition to the info passed
		    // to cellSlotClicked (see below), pass the handle and
		    // layer for the object clicked.
		    emit cellAnnotDoubleClicked (recordId, attrId, buttonState,
						 handle, layer, index, slot, 
						 cellx, celly, cellw, cellh);
		}

		// Handle mouse movement events
		// Pass same stuff as mouse click events
		else if (type == QEvent::MouseMove)
		{
		    emit cellAnnotMovedOver (recordId, attrId, buttonState,
					     handle, layer, index, slot, 
					     cellx, celly, cellw, cellh);
		}
		else
		{
		    fprintf (stderr, "CellGrid::MouseEventHandler:"
			     "Unknown/unhandled mouse event for cellAnnot!\n");
		}

		// If signaled annotation event, don't also signal 
		// the cell slot event
		return;
	    }
	}
    }

    // Handle mouse click events
    if (type == QEvent::MouseButtonPress)
    {
	// If got here (not over clickable annotation), 
	// signal to any connected listener that a index/slot in this cell 
	// has been clicked on (with the button specified by buttonState).
	//
	// Clicking on non-clickable annotates may yield index/slot values
	// that don't correspond directly to the text.  Clicking outside
	// the defined index/slot region (depending on justification, may be
	// to the right or left of the value) will yield NULL_INT for 
	// index/slot.
	// 
	// To provide as much info to the listener as possible, the relative
	// x,y position clicked in the cell, as well as, the current 
	// height/width (0,0 is top left corner of cell) is also passed.
	emit cellSlotClicked (recordId, attrId, buttonState, index, slot,
			      cellx, celly, cellw, cellh);
    }

    // Handle mouse double click events
    else if (type == QEvent::MouseButtonDblClick)
    {
	// If got here (not over clickable annotation), 
	// signal to any connected listener that a index/slot in this cell 
	// has been clicked on (with the button specified by buttonState).
	//
	// Clicking on non-clickable annotates may yield index/slot values
	// that don't correspond directly to the text.  Clicking outside
	// the defined index/slot region (depending on justification, may be
	// to the right or left of the value) will yield NULL_INT for 
	// index/slot.
	// 
	// To provide as much info to the listener as possible, the relative
	// x,y position clicked in the cell, as well as, the current 
	// height/width (0,0 is top left corner of cell) is also passed.
	emit cellSlotDoubleClicked (recordId, attrId, buttonState, index, slot,
				    cellx, celly, cellw, cellh);
    }

    // Handle mouse movement events
    // Pass same stuff as for clicked events
    else if (type == QEvent::MouseMove)
    {
	emit cellSlotMovedOver (recordId, attrId, buttonState, index, slot,
				cellx, celly, cellw, cellh);

    }
    else if (type == QEvent::MouseButtonRelease)
    {
#if 0
	TG_timestamp("Release button event for cellSlot!\n");
#endif
    }

    else
    {
	fprintf (stderr, "CellGrid::MouseEventHandler:"
		 "Unknown/unhandled mouse event for cellSlot!\n");
    }

}

// Internal sortAttrIdView helper routine that sorts firstChild and 
// all its siblings. Destroys contents of recordIdOrder, which it uses 
// as temp space.  Results put into nextPlacedSib and prevPlacedSib 
// and the first and last placed child is returned in the last
// two parameters.
void CellGrid::sortTreeChildren (RecordIdInfo *firstChild, 
				 _CompareData *cd,
				 int direction,
				 RecordIdInfo **firstPlacedChild,
				 RecordIdInfo **lastPlacedChild)
			 
{
    // If no children, return NULL now
    if (firstChild == NULL)
    {
	// Return that there are no children in this tree
	*firstPlacedChild = NULL;
	*lastPlacedChild = NULL;

	return;
    }

    int sortSize = 0;
    // Put firstChild and all its sibling into the recordIdOrder array
    // for sorting.  Guarenteed to fit.
    for (RecordIdInfo *childInfo = firstChild; childInfo != NULL;
	 childInfo = childInfo->nextSib)
    {
	recordIdOrder[sortSize] = childInfo->recordId;
	sortSize++;
    }

    // Do an initial scan to move any "null" elements to the bottom
    // of the list.  The null elements will remain in their
    // original order, but the nonnull elements may be shuffled.
    int filledRecordIds;
    presort( recordIdOrder, sortSize, &filledRecordIds, isNull, cd );
    tg_heapsort( recordIdOrder, filledRecordIds, 
	      ((direction > 0) ? compareUp : compareDown), cd );
    
    // Get the info for the first placed child
    RecordIdInfo *firstPlacedInfo = &recordIdInfo[recordIdOrder[0]];

    // Start the linkage with the first placed child
    // The nextPlacedSib for the first child set in loop below
    firstPlacedInfo->prevPlacedSib = NULL;

    // Scan thru in sorted order linking recordIdInfo's thru 
    // nextPlacedSib and prevPlacedSib
    RecordIdInfo *prevPlacedInfo = firstPlacedInfo;
    for (int index = 1; index < sortSize; index++)
    {
	// Get the recordId at this index
	int recordId = recordIdOrder [index];

	// Get the recordIdInfo for this recordId
	RecordIdInfo *placedInfo = &recordIdInfo[recordId];

	// Link the past placed record with this one
	placedInfo->prevPlacedSib = prevPlacedInfo;
	prevPlacedInfo->nextPlacedSib = placedInfo;

	// Update prevPlacedInfo
	prevPlacedInfo = placedInfo;
    }

    // Last placedInfo record should point to NULL (this may be the
    // firstPlacedInfo!)
    prevPlacedInfo->nextPlacedSib = NULL;

    // Return the first and last placed child in the tree
    *firstPlacedChild = firstPlacedInfo;
    *lastPlacedChild = &recordIdInfo[recordIdOrder[sortSize-1]];
}

// Sorts the recordIds based on the attrId's values.
// If direction > 0, sort ascending (values go up as the rows go down 
// the display).  If < 0, sort decending. 
// If =0, unsorted (restores recordId order).
// Use line number as tie breaker
// Ignores out of bounds attrId values
void CellGrid::sortAttrIdView (int attrId, int direction)
{
    // Ignore out of bounds attrIds
    if ((attrId < 0) || (attrId >= attrIds()))
	return;
    
    // Keep track of the sort parameters passed
    sortAttrId = attrId;
    sortDirection = direction;

    // Get the number of recordIds
    int numberRecordIds = recordIds();

    // If want to restore to original direction (direction == 0),
    // place in tree order
    if (direction == 0)
    {
	// Copy default tree ordering over to placed display order
	// for every recordId
	for (int index=0; index < numberRecordIds; index++)
	{
	    // Get the recordInfo at this index
	    RecordIdInfo *recordInfo = &recordIdInfo[index];

	    // Copy default tree to 'placement' tree
	    recordInfo->firstPlacedChild = recordInfo->firstChild;
	    recordInfo->lastPlacedChild = recordInfo->lastChild;
	    recordInfo->prevPlacedSib = recordInfo->prevSib;
	    recordInfo->nextPlacedSib = recordInfo->nextSib;
	}
	
	// Set the top level placed tree opinters
	treeFirstPlacedChild = treeFirstChild;
	treeLastPlacedChild = treeLastChild;
    }

    // Otherwise, sort based on data (direction != 0)
    else 
    {
	_CompareData cd;
	cd.cg = this;
	cd.attrId = attrId;

	// Sort the top level of the tree 
	sortTreeChildren (treeFirstChild, &cd, direction, 
			  &treeFirstPlacedChild, &treeLastPlacedChild);

	// Sort all the subtrees, sorting each subtree independently.
	int depth = 0;
	for (RecordIdInfo *childInfo = treeFirstChild; childInfo != NULL;
	     childInfo = nextTreeNode (childInfo, depth, FALSE))
	{
	    // Sort the children (if any) of this childInfo
	    sortTreeChildren (childInfo->firstChild, &cd, direction,
			      &childInfo->firstPlacedChild,
			      &childInfo->lastPlacedChild);
	}
    }


    // Reconstruct the placement arrays that have been destroyed
    // by the above loops
    reconstructPlacementArrays();

    // Set sort indicator for attrId
    // Qt seems to do the indicator opposite of normal: pointing
    // down means "sorted lowest-to-highest."  So I reverse the
    // sense to get the right appearance.
    if (direction > 0)
    {
	topHeader->setSortIndicator(attrId, FALSE);
    }
    else if (direction < 0)
    {
	topHeader->setSortIndicator(attrId, TRUE);
    }
    else
    {
	// If direction == 0, don't draw any sort indicator
	topHeader->setSortIndicator(-1, FALSE);
    }
	

    // Tell cellgrid redraw everything before control is returned to
    // the user
    requireCompleteUpdate();
}

// Handles attrId header clicks, will call routine
// to sort the view based on the attrId's value
void CellGrid::attrIdHeaderClicked( int attrId)
{
    // Sort the recordIds based on the attrId's value
    // On first click on a column (or if nothing sorted),
    // sort in accending order
    if ((sortAttrId != attrId) || (sortDirection == 0))
	sortAttrIdView (attrId, 1);

    // On second click on the same column, sort in decending order
    else if (sortDirection > 0)
	sortAttrIdView (attrId, -1);

    // On the third click on the same column, unsort the rows.
    else 
	sortAttrIdView (attrId, 0);
	
}



// Override Scrollview function to allow us to adjust
// window properties just before it is shown to user
// (both the first time and after being restored from minimized state).
// Use this to set contents area size, etc.
void CellGrid::showEvent( QShowEvent *e )
{
    QScrollView::showEvent( e );

    // Resize scrollable area to fit the new grid size
    syncGridSize();

    // Tell the world we're appearing
    emit cellgridShown( this );
}

void CellGrid:: hideEvent( QHideEvent *e )
{
    QScrollView::hideEvent( e );
    emit cellgridHidden( this );
}

// Attached to topHeader's sizeChanged signal.  
//  attrId and sizes no longer used (attrId was used in #if 0'd code below)
void CellGrid::attrIdWidthChanged( int /* attrId */, int, int )
{

    // Now delay resize sync as long a possible
    requireSync();

#if 0
    // Resize scrollable area to fit the new grid size
    syncGridSize();
#endif

    // For now update whole screen, not worth trying to minimize
    // update (since syncGridSize will require the same update,
    // this operation is really a no-op)
    requireCompleteUpdate();

#if 0
    // Require redraw everything from the attrId changed outward
    // before control returns to user
    int xPos = attrIdXPos( attrId );
    requireUpdate( xPos, 0, 
		   contentsWidth()-xPos, contentsHeight() );
#endif
}


// Signal called just before contents of scrollview window moved.
/*
 Found better to use this signal to scroll the topHeader 
 instead of the scrollbar's valueChanged signal.  This
 event happens *before* the rest of the window scrolls
 and I think it makes the header scroll look much better. */
void CellGrid::contentsMoved(int x, int )
{
    // If they are moving in the x direction (horizonal),
    // change offset.  Otherwise do nothing.
    if (x != topHeader->offset())
	topHeader->setOffset(x);

}

// Update visible geometries after resize events (and upon initialization)
void CellGrid::updateGeometries()
{
    // Optimization, only do something if width has changed.
    // Otherwise, getting a lot of needless redraws of the headers
    // and scrollbars.
    QRect curSize = topHeader->geometry();

    // The header width and heights need to be calculated from the edges
    int oldWidth = (curSize.right() - curSize.left()) + 1;
    int oldHeight = (curSize.bottom() - curSize.top()) + 1;

    // If already have these settings, do nothing
    if ((oldWidth == visibleWidth()) && 
	(oldHeight == (fontMetrics().height() + 4)))
	return;

#if 0 
    // I believe I fixed this by overriding viewportResizeEvent and 
    // not resizeEvent above.  Do not believe this is still necessary!
   
    // DEBUG, try updating scroll bars (since not always updated properly)
    // The documentation says this should never be necessary but it
    // for qt 3.0.4, unless I called this before adjusting topHeader,
    // it appeared topHeader got old, improper information when the
    // scroll bar was about to become unnecessary or about to become
    // necessary.
    updateScrollBars();
#endif

    // Resize top header to fit exactly in the unscrolled
    // area on top of the scrollview window.  Experimentation
    // shows that it looks best to give a 2 pixel border on
    // the top and left, but to go 2 pixels over the allocated
    // space on the right and bottom.  Don't understand but do it anyway.
    topHeader->setGeometry( 2, 2, visibleWidth(), 
			    fontMetrics().height()+ 4);
}

// topHeader manages attrId width, use it's info
// Returns 0 for out of bounds attrIds
int CellGrid::attrIdWidth( int attrId ) const
{
    return topHeader->sectionSize( attrId );
}



// Use stored recordId height for each recordId.
// Constant for each recordId for now, take recordId number of future
// enhancements.  Returns 0 for out of bounds recordIds.
int CellGrid::recordIdHeight(int recordId) const
{
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (0);

    return recordIdFixedHeight;
}

// Get starting pixel position of attrId
// Returns 0 for out of bounds attrIds
int CellGrid::attrIdXPos( int attrId ) const
{
    return topHeader->sectionPos( attrId );
}



// Get leftmost pixel offset of index/slot within cell
// That is, returns number between 0 and (width of cell)-1.
// Returns 0 for out of bounds recordId/attrIds
// Currently punts if invalid index/slot passed, so better only
// call for index/slots that really exist!
int CellGrid::slotXOffset (int recordId, int attrId, int index, int slot)
{
    if ((recordId < 0) || (recordId >= numRecordIds) ||
	(attrId < 0) || (attrId >= numAttrIds))
	return (0);

    // Get cell pointer from grid (may be NULL)
    Cell *cell = getCell (recordId, attrId);

    // Get actual height and width of cell (including 1 pixel border)
    int cellw = attrIdWidth (attrId);
    int cellh = recordIdHeight (recordId);

    // Layout the cell now, pass 1 less pixel for width/height to make
    // match paintCell call (border pixels)
    // 'cell' may be NULL
    layoutCell (cellLayout, cell, 0, 0, cellw-1, cellh-1, NULL,
                recordId, attrId);

    // Get leftmost pixel of slot, currently punts if index/slot
    // not valid combo!
    int xOffset = cellLayout.getPos(index, slot);
    
    // Return offset of leftmost pixel of index/slot
    return (xOffset);
}

// Internal worker routine for finding the Y value that needs to be
// visible in order to have the recordId visible.
// If NULL_INT passed, picks topmost pixel on screen.
// Used by scrollCellIntoView and cellScrolledOff.
int CellGrid::calcLeastVisibleY (int recordId)
{
    // Flush any pending reconstructions and grid syncs, so that
    // the Y calculations below will be correct
    flushReconstruction();
    flushSync();

    // Calculate the pixel that, if made visible, would satisfy the request.
    int makeYVisible;

    // If recordId NULL_INT or invalid, just pick topmost visible pixel
    // to make visible (makes vertical scrolling a no-op)
    if ((recordId == NULL_INT) || (recordId < 0) || (recordId >= numRecordIds))
    {
	makeYVisible = contentsY();
    }

    // Otherwise, pick Y pixel based on direction need to scroll
    else
    {
        // Get current position of the top of the screen 
	int screenTopY = contentsY();

	// Get the Y postion of the top pixel of recordId's row
	makeYVisible = recordIdYPos(recordId);

	// If recordId below top of screen, make sure bottom pixel of
	// record is visible after scroll (otherwise, top pixel is fine)
	if (screenTopY <= makeYVisible)
	    makeYVisible += recordIdHeight(recordId);
    }

    // Return the calculated Y
    return (makeYVisible);
}

// Internal worker routine for finding the Y value that needs to be
// visible in order to have the recordId visible.
// If NULL_INT passed, picks leftmost pixel on screen.
// Used by scrollCellIntoView and cellScrolledOff.
int CellGrid::calcLeastVisibleX (int attrId)
{
    // Flush any pending reconstructions and grid syncs, so that
    // the Y calculations below will be correct
    flushReconstruction();
    flushSync();

    // Calculate the pixel that, if made visible, would satisfy the request.
    int makeXVisible;

    // If attrId NULL_INT or invalid, just pick leftmost visible pixel
    // to make visible (makes horizonal scrolling a no-op)
    if ((attrId == NULL_INT) || (attrId < 0) || (attrId >= numAttrIds))
    {
	makeXVisible = contentsX();
    }

    // Otherwise, pick X pixel based on direction need to scroll
    else
    {
        // Get current position of the left edge of the screen 
	int screenLeftX = contentsX();

	// Get the X postion of the leftmost pixel of attrId's column
	makeXVisible = attrIdXPos(attrId);

	// If attrId to the right of the left edge of the screen, make 
	// sure rightmose pixel is visible, if possible.
	if (screenLeftX <= makeXVisible)
	{
	    int colWidth = attrIdWidth(attrId);
	    int screenWidth = visibleWidth();

	    // If col wider than screen, get as many pixels of column as 
	    // possible
	    if (screenWidth < colWidth)
	    {
		makeXVisible += screenWidth -1;
	    }

	    // Otherwise, make it all the column visible
	    else
	    {
		makeXVisible +=  colWidth -1;
	    }
	}
    }

    // Return the calculated X
    return (makeXVisible);
}

// Scrolls window so cell at recordId and attrId is visible in window.
// If recordId or attrId are NULL_INT, focuses on row or column
// specified.  Scrolls minimum possible to ensure visibility.
// Does nothing if recordId hidden (scrolling will not help).
// To determine if scrolling will take place, use 'cellScrolledOff()'.
void CellGrid::scrollCellIntoView (int recordId, int attrId, int rowMargin)
{
    // Do nothing if recordId hidden
    if ((recordId != NULL_INT) && recordIdHidden(recordId))
	return;

    // Do nothing if cell is visible
    if (!cellScrolledOff (recordId, attrId))
	return;

    // Sync grid resize before scrolling display, otherwise scroll may
    // not happen (since place to scroll to doesn't exist yet)
    flushSync();

    // Calculate the pixel that, if made visible, would satisfy the request.
    int needXVisible = calcLeastVisibleX (attrId);
    int needYVisible = calcLeastVisibleY (recordId);

    // Put rowMargin line(s) from top or bottom, use 
    // recordId height to set margin
    int heightMargin = recordIdHeight (recordId) * rowMargin;

    // Make height Margin at least 1 pixel
    if (heightMargin < 1)
	heightMargin = 1;

    // Make needXVisible,needYVisible visible with one row of margin
    // The above call prevent movement unless it is needed
    ensureVisible(needXVisible, needYVisible, 1, heightMargin);

    // With QT 3.0.4, ensureVisible messes up all pending redraws
    // Really bad visual errors caused by this for TreeViews->doSearch()
    // routine.  For now, force entire window to be redrawn!
    // Make sure screen is updated before control returns to user
    requireCompleteUpdate();
}

// Centers cell (the center pixel of cell) in display, with the given margins.
// xmargin and ymargin must be between 0.0 and 1.0.
// Margin 1.0 ensures centered.  Margin 0.5 is middle 50% of visible area.
// Margin 0.0 is just visible somewhere.
void CellGrid::centerCellInView (int recordId, int attrId,
				 float xmargin, float ymargin)
{
    // Do nothing if recordId invalid
    if ((recordId == NULL_INT) || (recordId < 0) || (recordId >= numRecordIds))
	return;

    // Do nothing if recordId hidden
    if ((recordId != NULL_INT) && recordIdHidden(recordId))
	return;

    // Do nothing if attrId invalid
    if ((attrId == NULL_INT) || (attrId < 0) || (attrId >= numAttrIds))
	return;

    // Sync grid resize before scrolling display, otherwise scroll may
    // not happen (since place to scroll to doesn't exist yet)
    flushSync();

    // Pick the center pixel of the cell to manipulate
    int needXVisible = attrIdXPos(attrId) + (attrIdWidth(attrId)/2);
    int needYVisible = recordIdYPos(recordId) + (recordIdHeight(recordId)/2);

    // Make needXVisible,needYVisible visible with specified margins
    // Should not move anything unless necessary.
    center(needXVisible, needYVisible, xmargin, ymargin);

    // With QT 3.0.4, ensureVisible messes up all pending redraws
    // Really bad visual errors caused by this for TreeViews->doSearch()
    // routine.  For now, force entire window to be redrawn!
    // Make sure screen is updated before control returns to user
    requireCompleteUpdate();
}

// Centers recordId in display, without changing the horizonal position
void CellGrid::centerRecordIdInView (int recordId)
{
    // Do nothing if recordId invalid
    if ((recordId == NULL_INT) || (recordId < 0) || (recordId >= numRecordIds))
	return;

    // Do nothing if recordId hidden
    if ((recordId != NULL_INT) && recordIdHidden(recordId))
	return;

    // Sync grid resize before scrolling display, otherwise scroll may
    // not happen (since place to scroll to doesn't exist yet)
    flushSync();

    // Pick the center pixel of the cell to manipulate
    int needYVisible = recordIdYPos(recordId) + (recordIdHeight(recordId)/2);

    // Pick visible pixel, so nothing will change
    int needXVisible = contentsX() + visibleWidth()/2;

    // Place needYVisible in center, don't move y dimention
    center(needXVisible, needYVisible, 0, 1);

    // With QT 3.0.4, ensureVisible messes up all pending redraws
    // Really bad visual errors caused by this for TreeViews->doSearch()
    // routine.  For now, force entire window to be redrawn!
    // Make sure screen is updated before control returns to user
    requireCompleteUpdate();
}

// Returns TRUE iff scrollCellIntoView will scroll the view window.
bool CellGrid::cellScrolledOff (int recordId, int attrId)
{
    // Return FALSE if recordId hidden, scrolling will not help (or be done)
    if ((recordId != NULL_INT) && recordIdHidden(recordId))
	return (FALSE);

    // Calculate the pixel that, if made visible, would satisfy the request.
    int needXVisible = calcLeastVisibleX (attrId);
    int needYVisible = calcLeastVisibleY (recordId);

    // Get visible area of window
    int leftX = contentsX();
    int width = visibleWidth();
    int topY = contentsY();
    int height = visibleHeight();

    // Return TRUE if falls out of visible region (scrollCellIntoView will
    // cause scrolling).  
    // (May need to test more to see if off by one pixel, but believe not)
    if ((needXVisible < leftX) || (needXVisible >= (leftX + width)) ||
	(needYVisible < topY) || (needYVisible >= (topY + height)))
    {
	return (TRUE);
    }
    
    // Otherwise, cell must be as visible as possible 
    // Return FALSE, scrollCellIntoView will do nothing
    else
    {
	return (FALSE);
    }
}



// Get starting pixel position of recordId in y dimension (down)
// Assumes fixed recordId height and only topHeader at top.
// Returns 0 for out of bounds recordIds
int CellGrid::recordIdYPos( int recordId ) 
{
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (0);

    // Get the row that recordId is currently displayed at
    int row = mapRecordIdToRow (recordId);

    // Calculate recordId starting position from row currently at.
    // Apparently, don't have to worry about topMargin of scrollview.
    // 0,0 is top left corner of scrollable area...
    int recordIdYPos = row * recordIdHeight(recordId);

    return recordIdYPos;
}

// Returns the attrId for the specified x direction (across) pixel
// Returns -1 if pixel not in a valid attrId.
int CellGrid::attrIdAt( int xPos ) const
{
    return topHeader->sectionAt( xPos );
}

// Returns recordId of the specified y direction (down) pixel.
// Returns -1 if pixel not in a valid recordId
int CellGrid::recordIdAt( int yPos )
{
    // Assumes fixed recordId height.
    // Apparently, don't have to worry about topMargin of scrollview.
    // 0,0 is top left corner of scrollable area...
    int calcRow = yPos/recordIdFixedHeight;

    // Map row of clicked on recordId to the actual recordId
    int recordId = mapRowToRecordId(calcRow);

    return recordId;
}

// Return size, in pixels, of the grid
/* Need to use actual id of the last attrId, not just the largest actual id
 in order to get right grid size!  The last attrId can be moved to the 
 beginning! */
QSize CellGrid::gridSize() 
{
    // Flush any pending reconstructions and grid syncs, so that
    // the Y calculations below will be correct
    flushReconstruction();

    // Get the actual id of attrId displayed at the far right 
    // (position at attrIds()-1).
    int lastAttrId = mapColToAttrId(attrIds()-1);
    
    // Calculate grid width using section id of the last attrId
    int w = attrIdXPos(lastAttrId) + attrIdWidth(lastAttrId);

    // Get the actual id of recordId displayed at the far bottom
    // (position at rowsVisible()-1).
    int lastRecordId = mapRowToRecordId(rowsVisible()-1);

    // Calculate grid height using last recordId displayed in grid
    int h = recordIdYPos( lastRecordId ) + recordIdHeight( lastRecordId );


    if ((h <= 0) && (lastRecordId > 0))
    {
	int yPos = recordIdYPos (lastRecordId);
	int height = recordIdHeight( lastRecordId );
	printf ("**** Grid Height (%i) <= 0! yPos %i height %i\n", h,
		yPos, height);
    }

    // Return the size in an easy to use object
    return QSize(w, h);
}

/* Internal routine that hides range of recordIds, so it will not be 
 *  displayed to user.   Hidden recordIds are still valid for all 
 *  cell updates, etc.   Ignores out of bounds recordIds or recordIds 
 *  already hidden by the same hiddenBy.
 *  Records can currently be hidden by trees (TreeHidden) or the user
 *  (UserHidden).
 *
 * NOTE: Only hideTreeChildren should set externalFixup to TRUE,
 *       which requires the caller (or caller's caller) to fix up the
 *       arrays and screen!  (Makes update much more efficient!)
 */
void CellGrid::hideRecordId(int startRecordId, int endRecordId,
			    HiddenBy hiddenBy, bool externalFixup)
{
#if 0
    // DEBUG
    printf ("hideRecordId (start %i, end %i, by %i)\n", 
	    startRecordId, endRecordId, hiddenBy);
#endif

    // Make startRecordId <= endRecordId
    if (startRecordId > endRecordId)
    {
	int temp = startRecordId;
	startRecordId = endRecordId;
	endRecordId = temp;
    }

    // Ignore ranges that are totally out of bounds 
    if ((endRecordId < 0) || (startRecordId >= numRecordIds))
	return;

    // Force recordId range to be legal
    if (startRecordId < 0)
	startRecordId = 0;
    if (endRecordId >= numRecordIds)
	endRecordId = numRecordIds - 1;

    // If all recordIds are already hidden, don't need to update screen
    bool allHidden = TRUE;

    // Hide those records that are not hidden
    for (int recordId = startRecordId; recordId <= endRecordId; 
	 recordId++)
    {
	// Record who is hiding record
	switch (hiddenBy)
	{
	  case TreeHides:
	    recordIdInfo[recordId].treeHides = 1;
	    break;

	  case UserHides:
	    recordIdInfo[recordId].userHides = 1;
	    break;

	  default:
	    TG_error ("CellGrid::hideRecordId: unsupported hiddenBy (%i)\n", 
		      hiddenBy);
	}

	// Only handle visible records
	if (recordIdInfo[recordId].visible)
	{
	    // Mark that we found a visible record, so need to redraw screen
	    allHidden = FALSE;

	    // Mark record as not visible
	    recordIdInfo[recordId].visible = 0;
	    
	    // Mark recordIdInfo[].row mapping as not valid
	    recordIdInfo[recordId].row = -1;

	    // Decrease number of rows visible
	    numRowsVisible--;
	}
    }

    // If nothing new hidden, do nothing more
    if (allHidden)
	return;

    // If fixing up externally (i.e., called by placeRecordId),
    // don't fix up arrays or draw screen.  Caller routine
    // will do it for you.
    if (externalFixup)
	return;

    // Now delay reconstruction of placement arrays as long as possible
    requireReconstruction();

    // Now delay resize sync as long a possible
    requireSync();

    // For now update whole screen, not worth trying to minimize
    // update (since syncGridSize will require the same update,
    // this operation is really a no-op)
    requireCompleteUpdate();
}

/* Unhides a range of recordIds, so they will now be displayed to user. 
 * Ignores out of bounds recordIds or already visible recordIds.
 * RecordIds may still not be visible if hidden by someone else.
 * Currently, recordIds may be hidden (unhidden) by trees (TreeHides)
 * or by the user (UserHides).
 *
 * NOTE: Only hideTreeChildren should set externalFixup to TRUE,
 *       which requires the caller (or caller's caller) to fix up the
 *       arrays and screen!  (Makes update much more efficient!)
 */
void CellGrid::unhideRecordId(int startRecordId, int endRecordId,
			      HiddenBy hiddenBy, bool externalFixup)
{
#if 0
    // DEBUG
    printf ("unhideRecordId (start %i, end %i, by %i)\n", 
	    startRecordId, endRecordId, hiddenBy);
#endif

    // Make startRecordId <= endRecordId
    if (startRecordId > endRecordId)
    {
	int temp = startRecordId;
	startRecordId = endRecordId;
	endRecordId = temp;
    }

    // Ignore ranges that are totally out of bounds 
    if ((endRecordId < 0) || (startRecordId >= numRecordIds))
	return;

    // Force recordId range to be legal
    if (startRecordId < 0)
	startRecordId = 0;
    if (endRecordId >= numRecordIds)
	endRecordId = numRecordIds - 1;


    // If all recordIds are already visible, don't need to update screen
    bool allVisible = TRUE;

    // Unhide those records that are hidden
    for (int recordId = startRecordId; recordId <= endRecordId; 
	 recordId++)
    {
	// No longer hide due to 'hiddenBy'.  Still may be hidden by others
	switch (hiddenBy)
	{
	  case TreeHides:
	    recordIdInfo[recordId].treeHides = 0;
	    break;

	  case UserHides:
	    recordIdInfo[recordId].userHides = 0;
	    break;

	  default:
	    TG_error ("CellGrid::unhideRecordId: unsupported hiddenBy (%i)\n", 
		      hiddenBy);
	}

	// Don't make visible if still hidden by someone
	if (recordIdInfo[recordId].treeHides ||
	    recordIdInfo[recordId].userHides)
	{
	    continue;
	}

	// Only handle hidden records
	if (!recordIdInfo[recordId].visible)
	{
	    // Mark that we found a hidden record, so need to redraw screen
	    allVisible = FALSE;

	    // Mark record as visible
	    recordIdInfo[recordId].visible = 1;
	    
	    // Mark recordIdInfo[].row mapping valid (if inaccurate)
	    recordIdInfo[recordId].row = numRowsVisible;

	    // Increase number of rows visible
	    numRowsVisible++;
	}
    }

    // If nothing new made visible, do nothing more
    if (allVisible)
	return;

    // If fixing up externally (i.e., called by placeRecordId),
    // don't fix up arrays or draw screen.  Caller routine
    // will do it for you.
    if (externalFixup)
	return;

    // Now delay reconstruction of placement arrays as long as possible
    requireReconstruction();

    // Now delay resize sync as long a possible
    requireSync();


    // For now update whole screen, not worth trying to minimize
    // update (since syncGridSize will require the same update,
    // this operation is really a no-op)
    requireCompleteUpdate();
}

/* Returns TRUE if recordId is hidden (or out of bounds), FALSE if not */
bool CellGrid::recordIdHidden (int recordId)
{
    // Return TRUE, for out of bounds
    if ((recordId < 0) || (recordId >= numRecordIds))
	return TRUE;
    
    // Return TRUE if not visible
    return (!recordIdInfo[recordId].visible);
}

// Returns TRUE if recordId is hidden by 'hiddenBy' (or out of bounds), 
// FALSE if not.  TreeHides and UserHides are current valid values for
// hiddenBy.  Punts on invalid hiddenBy values.
bool CellGrid::recordIdHiddenBy (int recordId, HiddenBy hiddenBy)
{
    // Return TRUE, for out of bounds
    if ((recordId < 0) || (recordId >= numRecordIds))
	return TRUE;
    
    // Handle various hiddenBy values
    switch (hiddenBy)
    {
      case TreeHides:
	return (recordIdInfo[recordId].treeHides);

      case UserHides:
	return (recordIdInfo[recordId].userHides);

      default:
	TG_error ("CellGrid::recordIdHiddenBy: invalid hiddenBy (%i)!",
		  hiddenBy);
    }

    // Should never get here
    return (FALSE);
}


/* Returns the recordId above the passed recordId, based on the
 *  current recordId display order (i.e., modified by sorting).  
 *  Independent of "hidden" status of recordId. (i.e., can return 
 *  hidden recordIds).  
 *  Returns NULL_INT if nothing above (passed top recordId or 
 *  invalid recordId).
 */
int CellGrid::recordIdAbove(int recordId)
{
    // Return NULL_INT if out of bounds
    if ((recordId < 0) || (recordId >= numRecordIds))
	return NULL_INT;

    // Flush pending placement array reconstructions so that
    // displayOrder is correct
    flushReconstruction();

    // Get the index this record displayed at
    int index = recordIdInfo[recordId].displayOrder;

    // If top recordId, return NULL_INT (nothing above)
    if (index == 0)
	return NULL_INT;

    // Otherwise, return recordId of record above this one
    else
	return (recordIdOrder[index-1]);
}

/* Returns the recordId below the passed recordId, based on the
 *  current recordId display order (i.e., modified by sorting).  
 *  Independent of "hidden" status of recordId. (i.e., can return 
 *  hidden recordIds).  
 *  Returns NULL_INT if nothing above (passed bottom recordId or 
 *  invalid recordId).
 */
int CellGrid::recordIdBelow(int recordId)
{
    // Return NULL_INT if out of bounds
    if ((recordId < 0) || (recordId >= numRecordIds))
	return NULL_INT;

    // Flush pending placement array reconstructions so that
    // displayOrder is correct
    flushReconstruction();

    // Get the number of recordIds
    int numberRecordIds = recordIds();

    // Get the index this record displayed at
    int index = recordIdInfo[recordId].displayOrder;

    // If bottom recordId, return NULL_INT (nothing below)
    if (index >= (numberRecordIds-1))
	return NULL_INT;

    // Otherwise, return recordId of record below this one
    else
	return (recordIdOrder[index+1]);
}

// Internal routine that detaches this record info from current position.
//
// NOTE: After calling this routine, recordInfo must be either 
// immediately attached with attachRecordInfo or be considered invalid 
// (i.e., not used after shrinking the grid).  
//
// Do not call for something already detached!  It will destroy
// the tree!
void CellGrid::detachRecordInfo (RecordIdInfo *recordInfo)
{
#if 0
    // DEBUG
    TG_timestamp ("%s: detachRecordInfo (%p (%i)) first %p last %p\n", 
		  name(), recordInfo,
		  recordInfo->recordId, treeFirstPlacedChild, treeLastPlacedChild);
#endif
    //
    // Disconnect from parent's tree
    //

    // Patch up link from above to this recordIdInfo
    if (recordInfo->prevSib != NULL)
    {
	recordInfo->prevSib->nextSib = recordInfo->nextSib;
    }
    else
    {
	if (recordInfo->parent != NULL)
	    recordInfo->parent->firstChild = recordInfo->nextSib;
	else
	    treeFirstChild = recordInfo->nextSib;
    }
    
    // Patch up link from below to this recordIdInfo
    if (recordInfo->nextSib != NULL)
    {
	recordInfo->nextSib->prevSib = recordInfo->prevSib;
    }
    else
    {
	if (recordInfo->parent != NULL)
	    recordInfo->parent->lastChild = recordInfo->prevSib;
	else
	    treeLastChild = recordInfo->prevSib;
    }

    //
    // Also disconnect from parent's placement tree
    //

    // Patch up placement link from above to this recordIdInfo
    if (recordInfo->prevPlacedSib != NULL)
    {
	recordInfo->prevPlacedSib->nextPlacedSib = recordInfo->nextPlacedSib;
    }
    else
    {
	if (recordInfo->parent != NULL)
	    recordInfo->parent->firstPlacedChild = recordInfo->nextPlacedSib;
	else
	    treeFirstPlacedChild = recordInfo->nextPlacedSib;
    }
    
    // Patch up placement link from below to this recordIdInfo
    if (recordInfo->nextPlacedSib != NULL)
    {
	recordInfo->nextPlacedSib->prevPlacedSib = recordInfo->prevPlacedSib;
    }
    else
    {
	if (recordInfo->parent != NULL)
	    recordInfo->parent->lastPlacedChild = recordInfo->prevPlacedSib;
	else
	    treeLastPlacedChild = recordInfo->prevPlacedSib;
    }
}

// Internal routine that attaches recordInfo with the specified parent
// below 'belowInfo'.  If parentInfo is NULL, puts in top level of tree.
// If belowInfo is NULL, makes first child of parent.
//
// NOTE: Before calling this routine, recordInfo either must have been
// detached using detachRecordInfo or have been just created
// (i.e., created while expanding the grid.).  
void CellGrid::attachRecordInfo(RecordIdInfo *recordInfo, 
				RecordIdInfo *parentInfo, 
				RecordIdInfo *belowInfo)
{
#if 0
    // DEBUG
    TG_timestamp ("%s: attachRecordInfo (%p (%i), %p, %p) first %p last %p\n",
		  name(), recordInfo,
		  recordInfo->recordId, parentInfo, belowInfo, treeFirstPlacedChild,
		  treeLastPlacedChild);
#endif

    // Update nextSib and link above for this record for all the cases
    // Also update placement links
    
    // If belowInfo not NULL, use/update its links
    if (belowInfo != NULL)
    {
	recordInfo->nextSib = belowInfo->nextSib;
	belowInfo->nextSib = recordInfo;

	recordInfo->nextPlacedSib = belowInfo->nextPlacedSib;
	belowInfo->nextPlacedSib = recordInfo;
    }
    
    // Otherwise, if parentInfo not NULL, use/update its links
    else if (parentInfo != NULL)
    {
	recordInfo->nextSib = parentInfo->firstChild;
	parentInfo->firstChild = recordInfo;

	recordInfo->nextPlacedSib = parentInfo->firstPlacedChild;
	parentInfo->firstPlacedChild = recordInfo;
    }
    
    // Otherwise, use/update top level tree's links
    else
    {
	recordInfo->nextSib = treeFirstChild;
	treeFirstChild = recordInfo;

	recordInfo->nextPlacedSib = treeFirstPlacedChild;
	treeFirstPlacedChild = recordInfo;
    }
    
    // recordInfo's prev Sib is belowInfo (even if NULL, is correct)
    // Also update placement links
    recordInfo->prevSib = belowInfo;
    recordInfo->prevPlacedSib = belowInfo;
    
    // Update link below to point at recordInfo
    if (recordInfo->nextSib != NULL)
	recordInfo->nextSib->prevSib = recordInfo;
    else
    {
	// Update appropriate lastChild link to point at recordInfo
	if (parentInfo != NULL)
	    parentInfo->lastChild = recordInfo;
	else
	    treeLastChild = recordInfo;
    }

    // Update placement link below to point at recordInfo
    if (recordInfo->nextPlacedSib != NULL)
	recordInfo->nextPlacedSib->prevPlacedSib = recordInfo;
    else
    {
	// Update appropriate lastPlacedChild link to point at recordInfo
	if (parentInfo != NULL)
	    parentInfo->lastPlacedChild = recordInfo;
	else
	    treeLastPlacedChild = recordInfo;
    }

    // Update parent pointer for this recordInfo
    recordInfo->parent = parentInfo;
}

// Internal helper routine that gets the next tree node in tree order.
// Given the first child of a parent (and a depth set initially to 0), 
// it walk the tree in order to return all the children, grandchildren of 
// that parent in tree order.  Returns NULL when there is nothing else
// to return for that parent.  
//
// If openOnly is TRUE, only will return children of
// open nodes, (otherwise ignores open/closed state).
CellGrid::RecordIdInfo *CellGrid::nextTreeNode (RecordIdInfo *childInfo, 
						int &depth, bool openOnly)
{

    // If has children, goto children next (unless not open and
    // only traversing the children of open nodes)
    if ((childInfo->firstChild != NULL) && (!openOnly || childInfo->treeOpen))
    {
	// Diving, increase depth count
	depth++;
	
	// Goto first child of this child
	childInfo = childInfo->firstChild;
    }
    
    // Otherwise, if has sibling afterwards, goto this sibling next
    else if (childInfo->nextSib != NULL)
    {
	// Doesn't effect depth
	childInfo = childInfo->nextSib;
    }
    
    // Otherwise, if this child has no siblings or children and
    // have previously dived on children (depth > 0), we need to
    // go back up tree, looking for next sibling of parent.  If
    // parent doesn't have sibling, keep climbing tree until find
    // sibling or depth becomes 0.
    else if (depth > 0)
    {
	// Sanity check, parent should never be NULL
	if (childInfo->parent == NULL)
	    TG_error ("CellGrid::nextTreeNode: error parent == NULL!");
	
	// If this child's parent doesn't have a sibling (and 
	// at some greater than 1 depth in the tree), pop up to the
	// next level in the tree looking for the next sibling
	while ((depth > 1) && (childInfo->parent->nextSib == NULL))
	{
	    // Move up one level in the tree (lower depth)
	    depth--;
	    childInfo = childInfo->parent;
	    
	    // Sanity check, parent should never be NULL
	    if (childInfo->parent == NULL)
		TG_error ("CellGrid::nextTreeNode: parent==NULL!");
	}
	
	// Goto next sibling of parent.  
	// May be NULL if depth == 0 after decrement below
	depth--;
	childInfo = childInfo->parent->nextSib;
	
	// Sanity check, childInfo may only be NULL if depth == 0 now
	if ((childInfo == NULL) && (depth != 0))
	{
	    TG_error ("CellGrid::nextTreeNode: algorithm error, "
		      "child NULL but depth %i != 0!", depth);
	}
    }
    
    // If get here, must be done with tree
    else
    {
	childInfo = NULL;
    }

    // Return new childInfo
    return (childInfo);
}

// Internal helper routine that gets the next *placed* tree node in tree order.
// Given the first placed child of a parent (and a depth set initially to 0), 
// it walk the tree in order to return all the placed children, 
// grandchildren of that parent in tree placement order.  
// Returns NULL when there is nothing else to return for that parent.  
//
// If openOnly is TRUE, only will return children of
// open nodes, (otherwise ignores open/closed state).
CellGrid::RecordIdInfo *CellGrid::nextPlacedTreeNode (RecordIdInfo *childInfo, 
						      int &depth, 
						      bool openOnly)
{

    // If has children, goto children next (unless not open and
    // only traversing the children of open nodes)
    if ((childInfo->firstPlacedChild != NULL) && 
	(!openOnly || childInfo->treeOpen))
    {
	// Diving, increase depth count
	depth++;
	
	// Goto first placed child of this child
	childInfo = childInfo->firstPlacedChild;
    }
    
    // Otherwise, if has placed sibling afterwards, goto this sibling next
    else if (childInfo->nextPlacedSib != NULL)
    {
	// Doesn't effect depth
	childInfo = childInfo->nextPlacedSib;
    }
    
    // Otherwise, if this child has no siblings or children and
    // have previously dived on children (depth > 0), we need to
    // go back up tree, looking for next sibling of parent.  If
    // parent doesn't have sibling, keep climbing tree until find
    // sibling or depth becomes 0.
    else if (depth > 0)
    {
	// Sanity check, parent should never be NULL
	if (childInfo->parent == NULL)
	{
	    TG_error ("CellGrid::nextPlacedTreeNode "
		      "(child->recordId %i, depth %i): "
		      "error parent == NULL!", childInfo->recordId, depth);
	}
	
	// If this child's parent doesn't have a sibling (and 
	// at some greater than 1 depth in the tree), pop up to the
	// next level in the tree looking for the next sibling
	while ((depth > 1) && (childInfo->parent->nextPlacedSib == NULL))
	{
	    // Move up one level in the tree (lower depth)
	    depth--;
	    childInfo = childInfo->parent;
	    
	    // Sanity check, parent should never be NULL
	    if (childInfo->parent == NULL)
		TG_error ("CellGrid::nextPlacedTreeNode: parent==NULL!");
	}
	
	// Goto next sibling of parent.  
	// May be NULL if depth == 0 after decrement below
	depth--;
	childInfo = childInfo->parent->nextPlacedSib;
	
	// Sanity check, childInfo may only be NULL if depth == 0 now
	if ((childInfo == NULL) && (depth != 0))
	{
	    TG_error ("CellGrid::nextPlacedTreeNode: algorithm error, "
		      "child NULL but depth %i != 0!", depth);
	}
    }
    
    // If get here, must be done with tree
    else
    {
	childInfo = NULL;
    }

    // Return new childInfo
    return (childInfo);
}

// Internal heler routine that reconstructs the recordId placement 
// arrays used to display the records, namely recordIdOrder, 
// recordIdInfo, and row2RecordId.
// Called after sorting or placing records.
void CellGrid::reconstructPlacementArrays()
{
#if 0
    static int count = 0;
    count++;
    // DEBUG
    TG_timestamp ("%s: Entering reconstructPlacementArrays(), numRecords = %i, times called=%i\n",
		  name(),  numRecordIds, count);
#endif
    
#if 0
    // DEBUG, print out first 12 items of tree
    int lastIndex = 12;
    if (lastIndex > numRecordIds)
	lastIndex = numRecordIds;

    for (int recordId =0; recordId < lastIndex; ++recordId)
    {
	RecordIdInfo *recordInfo = &recordIdInfo[recordId];
	TG_timestamp ("** current tree: %s %p (%i): next %p prev %p first child %p last child %p\n",
		      name(), recordInfo, recordId, recordInfo->nextPlacedSib,
		      recordInfo->prevPlacedSib, recordInfo->firstPlacedChild,
		      recordInfo->lastPlacedChild);
    }
#endif

    // Using placed tree order, build recordIdOrder array
    // Inline traversal since spend a lot of time in this routine
    // and nextPlacedTreeNode has much more logic than is need
    int placeId = 0;
    int row = 0;
    RecordIdInfo *placedInfo = treeFirstPlacedChild; 
    while (placedInfo != NULL)
    {
	// Get the recordId we are placing at this placeId
	int recordId = placedInfo->recordId;

#if 0
	// DEBUG
	TG_timestamp ("%s: reconstruct: recordId %p (%i) depth %i: firstPlacedChild %p lastPlacedChild %p nextPlacedSib %p\n", name(), placedInfo,
		      recordId, depth, placedInfo->firstPlacedChild, 
		      placedInfo->lastPlacedChild, placedInfo->nextPlacedSib);
#endif

	// Place child at placeId in display order list
	recordIdOrder[placeId] = recordId;

	// Map this RecordId back to the display order placeId
	recordIdInfo[recordId].displayOrder = placeId;
	
	// If this record is visible, put in order arrays
	if (recordIdInfo[recordId].visible)
	{
	    row2RecordId[row] = recordId;
	    recordIdInfo[recordId].row = row;

	    // Goto next row
	    row++;
	}
	// Otherwise, if not visible, map it to row -1
	else
	{
	    recordIdInfo[recordId].row = -1;
	}

	// Advance to next placement spot
	placeId++;


	//
	// Goto next recordInfo in the placement tree.  Basically
	// doing a depth first traversal of the placement tree
	//

	// If has children, goto first child
	if (placedInfo->firstPlacedChild != NULL)
	    placedInfo = placedInfo->firstPlacedChild;
	
	// Otherwise, if has sibling, to sibling next
	else if (placedInfo->nextPlacedSib != NULL)
	    placedInfo = placedInfo->nextPlacedSib;

	// Otherwise, go up the tree to find next unprocessed sibling
	else
	{
	    // Pop up one level in the tree
	    placedInfo = placedInfo->parent;

	    // Stop as soon processed entire tree
	    while (placedInfo != NULL)
	    {
		// If now have sibling, goto it and exit loop
		if (placedInfo->nextPlacedSib != NULL)
		{
		    placedInfo = placedInfo->nextPlacedSib;
		    break;
		}
		// Otherwise, pop up one level in the tree and try again
		else
		{
		    placedInfo = placedInfo->parent;
		}
	    }
	}
    }
    

    // COME BACK, DO WE WANT TO SET numRowsVisible or check it for consistency
    // Update number of visible rows
    numRowsVisible = row;

    // Sanity check, placeId better == numberRecordIds
    if (placeId != numRecordIds)
    {
	TG_error ("CellGrid::recontructPlacementArrays: "
		  "reconstruction algorithm error, "
		  "placeId = %i but numberRecordIds = %i!", placeId, 
		  numRecordIds);
    }

    // Mark that any pending reconstructions are no longer required
    reconstructionPending = FALSE;

#if 0
    // DEBUG
    TG_timestamp ("%s:Leaving reconstructPlacementArrays(), numRecordIds %i\n",
		  name(), numRecordIds);
#endif
}

// Places range of recordIds below 'belowRecordId', and if parentId is valid,
// places in tree structure with parentId as parent.  If belowRecordId
// is not valid (doesn't exist or is not child of parentId), will
// place recordId(s) as high as possible in current tree heirarchy level.
// If both belowRecordId and parentId are invalid (NULL_INT), will
// move recordId(s) to top level of tree, after the end of the current tree
//
// Single recordId version just is a call to this version with startRecordId
// and endRecordId the same.
//
// First time parents default to closed.   Children added later
// will not change the parents open/closed state.
void CellGrid::placeRecordId(int startRecordId, int endRecordId, 
			     int belowRecordId, int parentRecordId)
{
#if 0
    // DEBUG
    TG_timestamp ("%s: placeRecordId(start %i, end %i, below %i, parent %i)\n",
		  name(),  startRecordId, endRecordId, belowRecordId, 
		  parentRecordId);
#endif

    // Do nothing if startRecordId or endRecordId are invalid
    if ((startRecordId < 0) || (startRecordId >= numRecordIds) ||
	(endRecordId < 0) || (endRecordId >= numRecordIds))
	return;

    // If endRecordId < startRecordId, swap them and continue 
    if (endRecordId < startRecordId)
    {
	int tempId = startRecordId;
	startRecordId = endRecordId;
	endRecordId = tempId;
    }

    // Mark invalid belowRecordId and parentId by setting them to NULL_INT
    if ((belowRecordId < 0) || (belowRecordId >= numRecordIds))
	belowRecordId = NULL_INT;

    if ((parentRecordId < 0) || (parentRecordId >= numRecordIds))
	parentRecordId = NULL_INT;

    // If belowRecordId is in start/end range, treat it as invalid
    if ((belowRecordId >= startRecordId) && (belowRecordId <= endRecordId))
	belowRecordId = NULL_INT;

    // If parentRecordId is in start/end range, treat it as invalid
    if ((parentRecordId >= startRecordId) && (parentRecordId <= endRecordId))
	parentRecordId = NULL_INT;

    // If belowRecordId is not a direct child of parent, treat as invalid.
    if (belowRecordId != NULL_INT)
    {
	// Get the recordInfo before potentially destroying belowRecordId
	RecordIdInfo *belowParentInfo = recordIdInfo[belowRecordId].parent;

	if ((belowParentInfo == NULL) && (parentRecordId != NULL_INT))
	    belowRecordId = NULL_INT;
	
	if ((belowParentInfo != NULL) &&
	    (parentRecordId != belowParentInfo->recordId))
	    belowRecordId = NULL_INT;
    }

    RecordIdInfo *parentInfo, *belowInfo;

    if (parentRecordId != NULL_INT)
	parentInfo = &recordIdInfo[parentRecordId];
    else
	parentInfo = NULL;

    if (belowRecordId != NULL_INT)
	belowInfo = &recordIdInfo[belowRecordId];
    else
	belowInfo = NULL;

    // Determine if the placed records should be hidden by the tree 
    // infrastructure (because the parent, grandparent, etc. is closed) or 
    // if they should be unhidden by the tree infrastructure.
    // Use unsigned not bool so can compare to bit field below legally
    unsigned parentTreeHides = 0;  
    if ((parentInfo != NULL) && 
	(parentInfo->treeHides || !parentInfo->treeOpen))
    {
	parentTreeHides = 1;
    }

    //
    // Register the need for reconstruction, sync, and completeUpdate early
    // so can detect if any of the routines we call flush these updates.
    // This means they are using data in flux and could cause subtle
    // and hard to dedect problems.
    //

    // Register that we will need reconstruction of placement arrays.
    // This will allow overlap of multiple placeRecordId calls
    // See above reason for registering before actually doing work
    requireReconstruction();
    
    // Register that we will need to resync the grid size
    // This will allow overlap of multiple placeRecordId calls
    // See above reason for registering before actually doing work    
    requireSync();

    // Register that we will need to redraw the entire screen.
    // This will allow overlap of multiple placeRecordId calls
    // See above reason for registering before actually doing work    
    requireCompleteUpdate();


    // Loop through recordIds adding to appropriate place in tree
    for (int recordId = startRecordId; recordId <= endRecordId; 
	 recordId++)
    {
	RecordIdInfo *recordInfo = &recordIdInfo[recordId];

	// Disconnect this record info from current position
	detachRecordInfo (recordInfo);

	// Attach back in the desired position
	attachRecordInfo(recordInfo, parentInfo, belowInfo);

	// If placedRecord state doesn't match state enforced by
	// parent, flip 'hidden/unhidden by tree' state
	if (recordInfo->treeHides != parentTreeHides)
	{
	    // Make recordInfo's (and all it's children, grandchildren, etc.)
	    // hidden state match parentTreeHides
	    //
	    // Inline state change to prevent huge overhead (does tons of
	    // unnecessary checks) and calls that flush geometry changes
	    // from being delayed as desired
	    if (parentTreeHides)
	    {
		// Update this record and all children, grandchildren, etc.
		RecordIdInfo *updateInfo = recordInfo;

		// Keep track of depth of search, so stop after this 
		// procssing this recordInfo and any children  children
		int depth = 0;

		// Traverse list of all children, grand children, etc.
		// In the first pass, process recordInfo itself!
		while (updateInfo != NULL)
		{
		    // Mark as hidden by a closed tree
		    updateInfo->treeHides = 1;

		    // Make currently visible records, invisible
		    if(updateInfo->visible)
		    {
			// Mark record as not visible
			updateInfo->visible = 0;
			
			// Mark row mapping as not valid
			updateInfo->row = -1;

			// Decrease number of rows visible
			--numRowsVisible;
		    }

		    // User helper routine to get next child to look
		    // at.  Only look at children of open nodes
		    updateInfo = nextTreeNode (updateInfo, depth, TRUE);

		    // Since started with recordInfo, must end traversal
		    // when get back to depth 0 (or less)
		    if (depth <= 0)
			break;
		}
	    }
	    else
	    {
		// Update this record and all children, grandchildren, etc.
		RecordIdInfo *updateInfo = recordInfo;

		// Keep track of depth of search, so stop after this 
		// procssing this recordInfo and any children  children
		int depth = 0;

		// Traverse list of all children, grand children, etc.
		// In the first pass, process recordInfo itself!
		while (updateInfo != NULL)
		{
		    // Mark as no longer hidden by a closed tree
		    updateInfo->treeHides = 0;
		    
		    // Make visible unless user hides it
		    if (!updateInfo->userHides)
		    {
			// Mark record as visible
			updateInfo->visible = 1;
			
			// Mark row mapping valid (if inaccurate)
			updateInfo->row = numRowsVisible;
			
			// Increase number of rows visible
			numRowsVisible++;
		    }

		    // User helper routine to get next child to look
		    // at.  Only look at children of open nodes
		    updateInfo = nextTreeNode (updateInfo, depth, TRUE);

		    // Since started with recordInfo, must end traversal
		    // when get back to depth 0 (or less)
		    if (depth <= 0)
			break;
		}
	    }
	}

	// The next record should go below this one, so they
	// are inserted in recordId order
	belowInfo = recordInfo;
    }


    // Sanity check, nothing we have done during this placeRecordId
    // should have flushed the reconstruction, sync, or completeUpdate!
    // Things will not work right (and also run slowly) if this
    // has happenned, so punt!
    if (!reconstructionPending)
	TG_error ("CellGrid::placeRecordId: reconstruction not pending!");

    // Sanity check, see above
    if (!syncPending)
	TG_error ("CellGrid::placeRecordId: sync not pending!");

    // Sanity check, see above
    if (!completeUpdatePending)
	TG_error ("CellGrid::placeRecordId: completeUpdate not pending!");


#if 0
    // DEBUG
    TG_timestamp ("%s: End placeRecordId(start %i, end %i, below %i, parent %i)\n",
		  name(),
		  startRecordId, endRecordId, belowRecordId, parentRecordId);
#endif
}

// Sets the attrId to place the tree annotations at
// Setting to NULL_INT will hide all tree annotations
// Ignores invalid attrIds other than NULL_INT
void CellGrid::setTreeAttrId (int attrId)
{
    // Do nothing, if attrId invalid (and not NULL_INT)
    if ((attrId != NULL_INT) && ((attrId < 0) || (attrId >= numAttrIds)))
	return;

    // Do nothing if already the treeAttrId
    if (treeAttrId == attrId)
	return;

    // Change the treeAttrId to new value
    treeAttrId = attrId;

    // Potentially can modify every line, so redraw everything
    requireCompleteUpdate();
}

//! Return TRUE if recordId has children or is expandable and is
//! currently in the open state.  Otherwise, returns FALSE
bool CellGrid::isTreeOpen (int recordId)
{
    // Return FALSE, if recordId invalid
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (FALSE);

    // Get record info
    RecordIdInfo *recordInfo = &recordIdInfo[recordId];

    // Return FALSE if not a tree node (no children or not expandable)
    if ((recordInfo->firstChild == NULL) && !recordInfo->treeExpandable)
	return (FALSE);

    // Return tre state
    if (recordInfo->treeOpen == 1)
	return (TRUE);
    else
	return (FALSE);
}

// Opens or closes a tree node (i.e., shows or hides tree nodes children)
// Does nothing if recordId is not a tree node.
void CellGrid::setTreeOpen(int recordId, bool open)
{
    // Do nothing, if recordId invalid
    if ((recordId < 0) || (recordId >= numRecordIds))
	return;

    // Get record info
    RecordIdInfo *recordInfo = &recordIdInfo[recordId];

    // Do nothing if not a tree node (no children or not expandable)
    if ((recordInfo->firstChild == NULL) && !recordInfo->treeExpandable)
	return;

    // Do nothing if already in correct state
    if (((bool)recordInfo->treeOpen) == open)
	return;

    // Set to new state
    recordInfo->treeOpen = (uint)open;

    // If there are no children, this state change only changes
    // the tree state arrow.  Prevent the massive work below if
    // this is the case. -JCG 2/19/04
    if (recordInfo->firstChild == NULL)
    {
	// Make sure screen is updated, if visible and complete update
	// is not already pending (otherwise recordIdYPos can cause
	// expensive placement array reconstruction to occur)
	if (!completeUpdatePending && !recordIdHidden(recordId))
	{
	    requireUpdate( attrIdXPos(treeAttrId), recordIdYPos(recordId), 
			   attrIdWidth(treeAttrId), recordIdHeight(recordId));
	}

	return;
    }

    // Set hidden state for all tree children to correspond to 
    // open/closed state of tree.  Get back indication if all
    // children which flipped state are below the parent on the display
    // (Result not used, so not storing it any more)
    // bool allChildrenBelowParent = 
    hideTreeChildren (recordInfo, !open);

    // Because hideTreeChildren doesn't update the screen or display
    // arrays (for efficiency), do it here now.

    // Now delay reconstruction of placement arrays as long as possible
    requireReconstruction();

    // Now delay resize sync as long a possible
    requireSync();

    // For now update whole screen, not worth trying to minimize
    // update (since syncGridSize will require the same update,
    // this operation is really a no-op)
    requireCompleteUpdate();
}

// Internal routine to hide/unhide the children of a tree node.
// Used by both setTreeOpen and placeRecordId.  
// NOTE: Does not redraw or update row2RecordId, etc. arrays.
//       Calling routine *MUST* do the updates!
// By requiring caller to do updates (versus letting (un)hideRecordId do
// them, it prevents O(n^2) effort for some common cases.
// Returns TRUE if all children hidden/unhidden are displayed *strictly*
// after parent, FALSE otherwise.  (I.e., if TRUE, redraw only below parent,
// if FALSE, redraw everything.)
bool CellGrid::hideTreeChildren (RecordIdInfo *parentInfo, bool hide)
{
    // Initially assume all children fall below the parent
    bool allChildrenBelowParent = TRUE;
    
    // Get first child for this parent
    RecordIdInfo *childInfo = parentInfo->firstChild;

    // Keep track of depth of search, don't want to pop above 
    // level of parentInfo's children
    int depth = 0;

#if 0
    // DEBUG
    printf ("Parent %i :", parentInfo->recordId);
#endif

    // Traverse list of all children, grand children, etc.
    while ((childInfo != NULL) && (depth >= 0))
    {
#if 0
	// DEBUG
	printf (" %i", childInfo->recordId);
#endif

	// Is child in different hidden state than desired
	if (childInfo->treeHides != hide)
	{
	    // Hide this child (parent or ancestor tree is closed)
	    if (hide)
	    {
		// Hide this child, don't update arrays, screen, etc.
		hideRecordId(childInfo->recordId, TreeHides, TRUE);
	    }
	    // Otherwise, unhide child (parent or ancestor tree is open)
	    else
	    {
		// Unhide this child, don't update arrays, screen, etc.
		unhideRecordId(childInfo->recordId, TreeHides, TRUE);
	    }

	    // Check child position relative to parent only for those
	    // children whose hidden state was changed
	    if (childInfo->displayOrder < parentInfo->displayOrder)
		allChildrenBelowParent = FALSE;
		
	}

	// Use helper routine to get next child to look at 
	// Only look at children of open nodes
	childInfo = nextTreeNode (childInfo, depth, TRUE);
    }

#if 0
    // DEBUG
    printf ("\n");
#endif
    
    // Sanity check, depth better be 0 here
    if (depth != 0)
    {
	TG_error ("CellGrid::hideTreeChildren: algorithm error, ended "
		  "but depth %i != 0!", depth);
    }

    // Return TRUE if all children whose hidden state was changed fell
    // strickly below the parent (as far as display order)
    return (allChildrenBelowParent);
}

// Makes node expandable (or not expandable) even if doesn't currently
// have any children.  Make recordId look like a tree node and
// only does something if recordId doesn't have any children
void CellGrid::setTreeExpandable (int recordId, bool expandable)
{
    // Do nothing, if recordId invalid
    if ((recordId < 0) || (recordId >= numRecordIds))
	return;

    // Get record info
    RecordIdInfo *recordInfo = &recordIdInfo[recordId];

    // Do nothing if already in correct state
    if (((bool)recordInfo->treeExpandable) == expandable)
	return;

    // Set to new state
    recordInfo->treeExpandable = (uint)expandable;

    // Make sure screen is updated, if visible and complete update
    // is not already pending (otherwise recordIdYPos can cause
    // expensive placement array reconstruction to occur)
    if (!completeUpdatePending && !recordIdHidden(recordId))
    {
	requireUpdate( attrIdXPos(treeAttrId), recordIdYPos(recordId), 
		       attrIdWidth(treeAttrId), recordIdHeight(recordId) );
    }
}


// Returns parent recordId in tree, if available, or NULL_INT if not
int CellGrid::recordIdParent (int recordId)
{
    // If recordId invalid, return NULL_INT
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (NULL_INT);

    // Get record info
    RecordIdInfo *recordInfo = &recordIdInfo[recordId];

    // If no parent, return NULL_INT
    if (recordInfo->parent == NULL)
	return (NULL_INT);

    // Otherwise, return recordId of parent
    return (recordInfo->parent->recordId);
}

// Returns recordId of first child of 'recordId' in tree, if available, 
// or NULL_INT if not.  If passed in 'recordId' is NULL_INT, returns
// first child of top level tree, if any (i.e., if grid not empty).
int CellGrid::recordIdFirstChild (int recordId)
{
    // If passed NULL_INT, return first child of top level tree
    if (recordId == NULL_INT)
    {
	if (treeFirstChild != NULL)
	    return (treeFirstChild->recordId);
	else
	    return (NULL_INT);
    }

    // If recordId invalid, return NULL_INT
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (NULL_INT);

    // Get record info
    RecordIdInfo *recordInfo = &recordIdInfo[recordId];

    // If no children, return NULL_INT
    if (recordInfo->firstChild == NULL)
	return (NULL_INT);

    // Otherwise, return recordId of the first child
    return (recordInfo->firstChild->recordId);
}

// Returns recordId of last child of 'recordId' in tree, if available, 
// or NULL_INT if not.  If passed in 'recordId' is NULL_INT, returns
// last child of top level tree, if any (i.e., if grid not empty).
int CellGrid::recordIdLastChild (int recordId)
{

    // If passed NULL_INT, return first child of top level tree
    if (recordId == NULL_INT)
    {
	if (treeLastChild != NULL)
	    return (treeLastChild->recordId);
	else
	    return (NULL_INT);
    }

    // If recordId invalid, return NULL_INT
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (NULL_INT);

    // Get record info
    RecordIdInfo *recordInfo = &recordIdInfo[recordId];

    // If no children, return NULL_INT
    if (recordInfo->lastChild == NULL)
	return (NULL_INT);

    // Otherwise, return recordId of the last child
    return (recordInfo->lastChild->recordId);
}

// Returns recordId of next sibling of 'recordId' at same level in the tree, 
// if available, or NULL_INT if not. 
int CellGrid::recordIdNextSibling (int recordId)
{
    // If recordId invalid, return NULL_INT
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (NULL_INT);

    // Get record info
    RecordIdInfo *recordInfo = &recordIdInfo[recordId];

    // If no sibling, return NULL_INT
    if (recordInfo->nextSib == NULL)
	return (NULL_INT);

    // Otherwise, return recordId of the next sibling
    return (recordInfo->nextSib->recordId);
}

// Returns recordId of previous sibling of 'recordId' at same level in 
// the tree, if available, or NULL_INT if not. 
int CellGrid::recordIdPrevSibling (int recordId)
{
    // If recordId invalid, return NULL_INT
    if ((recordId < 0) || (recordId >= numRecordIds))
	return (NULL_INT);

    // Get record info
    RecordIdInfo *recordInfo = &recordIdInfo[recordId];

    // If no sibling, return NULL_INT
    if (recordInfo->prevSib == NULL)
	return (NULL_INT);

    // Otherwise, return recordId of the prev sibling
    return (recordInfo->prevSib->recordId);
}

// Redefine customWhatsThis() to return false so that clicks
// on the cell grid get passed on in WhatsThis mode
bool CellGrid::customWhatsThis() const
{
    printf ("CellGrid:customWhatsThis called!\n");
    return TRUE;
}

void CellGrid::setName( const char * name )
{
    QObject:: setName( name );
    nameChanged( this );
}

// Handle key press events, mainly page up/down
void CellGrid::keyPressEvent(QKeyEvent *e)
{
    int xPos, attrId, newPos, diff;

    // Handle the keys we know 
    switch (e->key())
    {
      case Key_PageDown:
	scrollBy (0, visibleHeight());
	break;
      case Key_PageUp:
	scrollBy (0, -visibleHeight());
	break;
      case Key_Down:
	scrollBy (0, recordIdFixedHeight);
	break;
      case Key_Up:
	scrollBy (0, -recordIdFixedHeight);
	break;
      case Key_End:
	scrollBy (0, contentsHeight());
	break;
      case Key_Home:
	scrollBy (0, -contentsHeight());
	break;
      case Key_Left:
	// Move to beginning of attrId that starts on pixel to the left
	// Get the position of the left most pixel on screen
	xPos = contentsX();
	// Get the attrId one pixel to the left of this point
	attrId = attrIdAt (xPos-1);
	// Get the left most position of attrId
	newPos = attrIdXPos (attrId);
	// Find how much we need to shift to get there
	diff = newPos - xPos;
	// Scroll window by that amount
	scrollBy (diff, 0);
	break;
      case Key_Right:
	// Move to end of attrId that start on this pixel
	// Get the position of the left most pixel on screen
	xPos = contentsX();
	// Get the attrId this pixel is in
	attrId = attrIdAt (xPos);
	// Get the last pixel in this attrId (plus 1)
	newPos = attrIdXPos (attrId) + attrIdWidth(attrId);
	// Find how much we need to shift to get there
	diff = newPos - xPos;
	// Scroll window by that amount
	scrollBy (diff, 0);
	break;

	// Let next level of widgets handle other keys
      default:
	e->ignore();
    }
}

// Handle custom events, mainly flushUpdates
void CellGrid::customEvent (QCustomEvent *e)
{
    // For any custom event, see if the search engine's cell watchpoint
    // has been triggered and emit a signal if it has.
    if (watchpointTriggered)
    {
	// Clear watchpoint flag before signally
	watchpointTriggered = FALSE;

	// Signal watchpoint triggered
	emit searchEngineWatchpointTriggered(this, watchRecordId, watchAttrId);
    }

    switch (e->type())
    {
	// Handle FlushUpdate event
      case 1100:
#if 0
      {
	// DEBUG
	const char *winName = name();
	if (winName != NULL)
	{
	    TG_timestamp ("customEvent(%s): flushUpdate()\n", 
			  winName, e->type());
	}
	else
	{
	    TG_timestamp ("customEvent: flushUpdate()\n", e->type());
	}
      }
#endif
			  
	flushUpdate();
	break;

      default:
	fprintf (stderr, 
		 "CellGrid::customEvent: Warning unknown event type %i\n",
		 e->type());
	break;
    }
}


#if 0
void CellGrid::focusInEvent(QFocusEvent* e)
{
//    printf ("focusInEvent()\n");
//    fflush (stdout);
    // Do nothing, trying to prevent redraw
}

void CellGrid::focusOutEvent(QFocusEvent* e)
{
//    printf ("focusOutEvent()\n");
//    fflush (stdout);
    // Do nothing, trying to prevent redraw
}
#endif

int CellGrid::compareUp( int a, int b, const void * extraData )
{
	const _CompareData * cd = (_CompareData *)extraData;

	CellGrid * cg = cd->cg;
	int attrId = cd->attrId;
	int retval;

	// Get original position cell text
	QString otext = cg->cellScreenText(a, attrId);
	QString ntext = cg->cellScreenText(b, attrId);

	// Is it a number?
	bool oisNumber, nisNumber;
	double ovalue = otext.toDouble(&oisNumber);
	double nvalue = ntext.toDouble(&nisNumber);

	if( oisNumber && nisNumber ) {
		if( ovalue > nvalue ) retval = 1;
		else if( ovalue < nvalue ) retval = -1;
		else retval = 0;
	} else {
		retval = otext.compare(ntext);
	}

	// If there's a tie, choose the entry with the lower
	// index, since that one went in the listing first
 	if( retval == 0 ) retval = ((a > b ) ? 1 : -1);

//	cout << "comparing " << otext << "(row2RecordId["<< a<<"]) to "
//		<< ntext << "(row2RecordId["<<b<<"]), got "<<  retval << endl;
	return retval;
}
	
int CellGrid::compareDown( int a, int b, const void * extraData )
{
	const _CompareData * cd = (_CompareData *)extraData;

	CellGrid * cg = cd->cg;
	int attrId = cd->attrId;
	int retval;

	// Get original position cell text
	QString otext = cg->cellScreenText(a, attrId);
	QString ntext = cg->cellScreenText(b, attrId);

	// Is it a number?
	bool oisNumber, nisNumber;
	double ovalue = otext.toDouble(&oisNumber);
	double nvalue = ntext.toDouble(&nisNumber);

	if( oisNumber && nisNumber ) {
		if( ovalue > nvalue ) retval = -1;
		else if( ovalue < nvalue ) retval = 1;
		else retval = 0;
	} else {
		retval = -(otext.compare(ntext));
	}

	// If there's a tie, choose the entry with the higher
	// index, since that one went in the listing last
 	if( retval == 0 ) retval = ((a > b ) ? -1 : 1);
//	cout << "comparing " << otext << "(row2RecordId["<< a<<"]) to "
//		<< ntext << "(row2RecordId["<<b<<"]), got "<<  retval << endl;
	return retval;
}

int CellGrid::isNull( int i, const void * extraData )
{
        const _CompareData * cd = (_CompareData *)extraData;

        CellGrid * cg = cd->cg;
        int attrId = cd->attrId;
        QString text = cg->cellScreenText(i, attrId);

        return ( text.isNull() || text.isEmpty() );
}

// Finds text in grid and allows search direction to be specified
// by parameter forwardSearch.  See findTextForward() and 
// findTextBackward() for details, since they do all the work.
bool CellGrid::findText (int &startRecordId, int &startAttrId, 
			 int &startIndex, const QString &searchText, 
			 bool forwardSearch, bool caseSensitive, 
			 bool openTreesOnly, bool wrapSearch,
			 int endRecordId, int endAttrId, int endIndex)
{
    if (forwardSearch)
    {
	return (findTextForward (startRecordId, startAttrId, startIndex,
				 searchText, caseSensitive, openTreesOnly,
				 wrapSearch, endRecordId, endAttrId,
				 endIndex));
    }
    else
    {
	return (findTextBackward (startRecordId, startAttrId, startIndex,
				  searchText, caseSensitive, openTreesOnly,
				  wrapSearch, endRecordId, endAttrId,
				  endIndex));
    }
}

// Searches forward for searchText in the grid, starting at startRecordId,
// startAttrId, and startIndex in display order (that is, in the
// order rows and columns are displayed on the screen, not how they
// are indexed by recordId and AttrId).  These start positions may 
// be NULL_INT, in which case the left/topmost position will be chosen.
// Search can be made case-sensitive by setting caseSensitive to TRUE.  
// In order to search both open and closed trees, set openTreesOnly 
// to FALSE. Does not search recordIds hidden with hideRecordId().  
// If wrapSearch == FALSE, stops unsuccessful searches after search last
// recordId.  Otherwise if wrapSearch == TRUE, will wrap around and 
// continue search until hits starting point (or match is found).
//
// If match for searchText found, returns TRUE and the exact leftmost 
// character location of matching text is stored in startRecordId, 
// startAttrId, and startIndex.
// Otherwise, if no match, FALSE is returned otherwise and startRecordId,
// startAttrId, and startIndex set to NULL_INT.
//
// Siliently treats invalid startRecordId and startAttrId as if they were 
// set to NULL_INT.   Silently treats out-of-bounds startIndex as 0 or
// a directive to goto the next cell, as appropriate.
//
// To continue search from last found text position, just add 1 to 
// startIndex.  
//
// See findTextBackward() in order to search the grid backwards and
// findText() to specify direction in a parameter.
bool CellGrid::findTextForward (int &startRecordId, int &startAttrId, 
				int &startIndex, const QString &searchText, 
			        bool caseSensitive, bool openTreesOnly, 
				bool wrapSearch, int endRecordId,
				int endAttrId, int endIndex)
{
    // Variables that specify where to start the scan
    int initialRecordPos;
    int initialCol;
    int initialTextIndex;

    // Return not-found now if empty string passed in
    if (searchText.isEmpty())
    {
	// Mark that search failed
	startRecordId = NULL_INT;
	startAttrId = NULL_INT;
	startIndex = NULL_INT;
	
	// Return that search failed
	return (FALSE);
    }

    // Flush pending placement array reconstructions
    // so displayOrder is correct
    flushReconstruction();

    // 
    // Determine where to start the scan
    // 

    // If startRecordId not valid, start at the top
    if ((startRecordId == NULL_INT) || (startRecordId < 0) ||
	(startRecordId >= recordIds()))
    {
	// Start at first record in current display order (may be hidden)
	initialRecordPos = 0;
    }

    // Otherwise, start at startRecordId
    else
    {
	// Start at startRecordId's position in the display order
	initialRecordPos = recordIdInfo[startRecordId].displayOrder;
    }

    // If startAttrId not valid, start at left column
    if ((startAttrId == NULL_INT) || (startAttrId < 0) ||
	(startAttrId >= attrIds()))
    {
	// Start at left column, column 0
	initialCol = 0;
    }

    // Otherwise, start at startAttrId
    else
    {
	// Start at the column startAttrId currently displayed in
	initialCol = mapAttrIdToCol (startAttrId);
    }

    // If startIndex 0, negative or NULL_INT, start at begining of cell text
    if ((startIndex <= 0) || (startIndex == NULL_INT))
    {
	// Start search at first character in cell text
	initialTextIndex = 0;
    }

    // Otherwise, need to check startIndex further
    else
    {
	// Get the cell location for intial cell
	int initialRecordId = recordIdOrder [initialRecordPos];
	int initialAttrId = mapColToAttrId (initialCol);

	// Get the cell screen text for the intial cell
	QString initialCellText = cellScreenText (initialRecordId, 
						  initialAttrId);

	// If startIndex falls within length, use it
	if (startIndex < (int) initialCellText.length())
	{
	    initialTextIndex = startIndex;
	}
	
	// Otherwise, advance to beginning of next cell 
	else
	{
	    initialTextIndex = 0;

	    // Advance to next column
	    initialCol++;

	    // Wrap around to beginning of next line, if necessary
	    if (initialCol >= attrIds())
	    {
		// Advance to next recordPos
		initialRecordPos++;

		// Start at begining of this line
		initialCol = 0;

		// Wrap to top of screen, if necessary
		if (initialRecordPos >= recordIds())
		{
		    // Wrap only if allowed
		    if (wrapSearch)
		    {
			initialRecordPos = 0;
		    }

		    // Otherwise, done with search, text not found
		    else
		    {
			// Mark that search failed
			startRecordId = NULL_INT;
			startAttrId = NULL_INT;
			startIndex = NULL_INT;

			// Return that search failed
			return (FALSE);
		    }
		}
	    }
	}
    }

    //
    // Determine where to end the scan
    //
    int endRecordPos;
    int endCol;

    if( endRecordId == NULL_INT ) {
	    endRecordPos = NULL_INT;
    } else {
	endRecordPos = recordIdInfo[endRecordId].displayOrder;
    }

    if( endAttrId == NULL_INT ) {
	    endCol = NULL_INT;
    } else {
	endCol = mapAttrIdToCol (endAttrId);
    }

    // We don't mess with the endIndex because we assume that it
    // came from a previous search and points legitimately into the
    // cell text.

    // 
    // Scan for text in cellGrid
    //

    // Temp string for holding cell text
    QString scanText;

    // After scaning first cell, scanTextIndex will be set to 0
    int scanTextIndex = initialTextIndex;

    // After scanning first recordId, scanCol will be set to 0
    int scanCol = initialCol;
    
    // Scan recordIds in display order (includes hidden and unhidden rows),
    // starting at initialRecordPos.  Scan until find text, or have searched
    // the entire grid (or if !wrapSearch, the portion below the starting 
    // point).
    for (int scanRecordPos = 0; scanRecordPos <= numRecordIds; scanRecordPos++)
    {
	// Modify ScanRecordPos with intialRecordPos, so start scan
	// at initialRecordPos.
	int modifiedScanRecordPos = initialRecordPos + scanRecordPos;

	// Do we need to wrap modifiedScanRecordPos up to top of grid?
	if (modifiedScanRecordPos >= numRecordIds)
	{
	    // Needs wrapping and caller allows wraps, so wrap around
	    if (wrapSearch)
	    {
		// At most, only need to wrap once so can use subtraction
		modifiedScanRecordPos -= numRecordIds;
	    }

	    // Needs wrapping, but caller disallows, so done
	    else
	    {
		// Indicate search failed
		startRecordId = NULL_INT;
		startAttrId = NULL_INT;
		startIndex = NULL_INT;

		// Return FALSE, search failed
		return (FALSE);
	    }
	}
	
	// If an end position is set, see if we've gone past it
	if( endRecordPos != NULL_INT
			&& modifiedScanRecordPos > endRecordPos ) {
		// Search has failed, so we're done
		startRecordId = NULL_INT;
		startAttrId = NULL_INT;
		startIndex = NULL_INT;
		return (FALSE);
	}

	// Get recordId at modifiedScanRecordPos
	int scanRecordId = recordIdOrder[modifiedScanRecordPos];

	// Don't scan this recordId if hidden by user, or if hidden
	// by collapsed tree and not scanning collapsed trees
	if (recordIdHiddenBy(scanRecordId, UserHides) ||
	    ((openTreesOnly) && recordIdHiddenBy (scanRecordId, TreeHides)))
	{
	    // After first recordId, always start scan from column 0, index 0
	    scanCol = 0;
	    scanTextIndex = 0;

	    continue;
	}

	// Scan attrIds in column order, the first time starting at
	// intialScanCol and the rest from column 0
	for (; scanCol < numAttrIds; scanCol++)
	{
	    // If an enpoint is set on this record, see if we passed it
	    if( endRecordPos == modifiedScanRecordPos && endCol != NULL_INT
				    && scanCol > endCol ) {
		    // Search failed; we're done
		    startRecordId = NULL_INT;
		    startAttrId = NULL_INT;
		    startIndex = NULL_INT;
		    return (FALSE);
	     }

	    // Get attrId in this column (since attrIds cannot be hidden yet)
	    int scanAttrId = mapColToAttrId (scanCol);

	    // Get the screen text for this cell
	    scanText = cellScreenText(scanRecordId, scanAttrId);

	    // Is searchText in this cell's text?
	    int matchLocation = scanText.find (searchText, scanTextIndex, 
					       caseSensitive);

	    // Test for match found
	    if (matchLocation >= 0)
	    {
		// Did the found text start after a specified endpoint?
		if( endRecordPos == modifiedScanRecordPos
				&& endCol == scanCol
				&& endIndex != NULL_INT
				&& matchLocation >= endIndex ) {
			// So close! But too far along in the text, so fail
			startRecordId = NULL_INT;
			startAttrId = NULL_INT;
			startIndex = NULL_INT;
			return (FALSE);
		}

		// Record where the match was found
		startRecordId = scanRecordId;
		startAttrId = scanAttrId;
		startIndex = matchLocation;

		// Return that match found
		return (TRUE);
	    }

	    // All but the first test starts at beginning of text
	    scanTextIndex = 0;
	}

	// All but the first recordId scan starts at first column
	scanCol = 0;
    }
	    
    // If got here, search failed.  Set return indexes to NULL_INT.
    startRecordId = NULL_INT;
    startAttrId = NULL_INT;
    startIndex = NULL_INT;

    // Return FALSE, search failed
    return (FALSE);
}

//! Searches backward for searchText in the grid.
//! Similiar to findTextForwards() except searches backwards thru grid.
//!
//! To continue search from last found text position, just subtract 1 
//! from startIndex.  
bool CellGrid::findTextBackward (int &startRecordId, int &startAttrId, 
				int &startIndex, const QString &searchText, 
			        bool caseSensitive, bool openTreesOnly, 
				bool wrapSearch, int endRecordId,
				int endAttrId, int endIndex)
{
    // Variables that specify where to start the scan
    int initialRecordPos;
    int initialCol;
    int initialTextIndex;

    // Return not-found now if empty string passed in
    if (searchText.isEmpty())
    {
	// Mark that search failed
	startRecordId = NULL_INT;
	startAttrId = NULL_INT;
	startIndex = NULL_INT;
	
	// Return that search failed
	return (FALSE);
    }

    // Flush any pending placement array reconstructions,
    // so displayOrder is correct
    flushReconstruction();

    // 
    // Determine where to start the scan
    // 

    // If startRecordId not valid, start at the bottom
    if ((startRecordId == NULL_INT) || (startRecordId < 0) ||
	(startRecordId >= recordIds()))
    {
	// Start at last record in current display order (may be hidden)
	initialRecordPos = recordIds() - 1;
    }

    // Otherwise, start at startRecordId
    else
    {
	// Start at startRecordId's position in the display order
	initialRecordPos = recordIdInfo[startRecordId].displayOrder;
    }

    // If startAttrId not valid, start at right column
    if ((startAttrId == NULL_INT) || (startAttrId < 0) ||
	(startAttrId >= attrIds()))
    {
	// Start at right column, the last colum
	initialCol = attrIds() - 1;
    }

    // Otherwise, start at startAttrId
    else
    {
	// Start at the column startAttrId currently displayed in
	initialCol = mapAttrIdToCol (startAttrId);
    }

    // If startIndex NULL_INT, start at end of cell text
    if (startIndex == NULL_INT)
    {
	// Start search at last character in cell text (-1 for findRev)
	initialTextIndex = -1;
    }

    // Otherwise, need to check startIndex further
    else
    {
	// Get the cell location for intial cell
	int initialRecordId = recordIdOrder [initialRecordPos];
	int initialAttrId = mapColToAttrId (initialCol);

	// Get the cell screen text for the intial cell
	QString initialCellText = cellScreenText (initialRecordId, 
						  initialAttrId);

	// Get the textLength for ease of use (and to make signed!!!)
	// Must do otherwise -1 will be greater than len 11 (in unsigned mode)
	int textLength = initialCellText.length();

	// If startIndex falls within length, use it
	if ((startIndex >= 0) && (startIndex < (int)initialCellText.length()))
	{
	    initialTextIndex = startIndex;
	}

	// If startIndex falls past end of text, start at end of text
	else if (startIndex >= textLength)
	{
	    // Start search at last character in cell text (-1 for findRev)
	    initialTextIndex = -1;
	}
	
	// Otherwise, advance to end of previous cell 
	else
	{
	    // Start search at last character in cell text (-1 for findRev)
	    initialTextIndex = -1;

	    // Advance to previous column
	    initialCol--;

	    // Wrap around to beginning of next line, if necessary
	    if (initialCol < 0)
	    {
		// Advance to previous recordPos
		initialRecordPos--;

		// Start at end of this line
		initialCol = attrIds()-1;

		// Wrap to bottom of screen, if necessary
		if (initialRecordPos < 0)
		{
		    // Wrap only if allowed
		    if (wrapSearch)
		    {
			initialRecordPos = recordIds()-1;
		    }

		    // Otherwise, done with search, text not found
		    else
		    {
			// Mark that search failed
			startRecordId = NULL_INT;
			startAttrId = NULL_INT;
			startIndex = NULL_INT;

			// Return that search failed
			return (FALSE);
		    }
		}
	    }
	}
    }

    //
    // Determine where to end the scan
    //
    int endRecordPos;
    int endCol;

    if( endRecordId == NULL_INT ) {
	    endRecordPos = NULL_INT;
    } else {
	endRecordPos = recordIdInfo[endRecordId].displayOrder;
    }

    if( endAttrId == NULL_INT ) {
	    endCol = NULL_INT;
    } else {
	endCol = mapAttrIdToCol (endAttrId);
    }

    // We don't mess with the endIndex because we assume that it
    // came from a previous search and points legitimately into the
    // cell text.

    // 
    // Scan backwards for text in cellGrid
    //

    // Temp string for holding cell text
    QString scanText;

    // After scaning first cell, scanTextIndex will be set to -1 (last char)
    int scanTextIndex = initialTextIndex;

    // Get last column for ease of use
    int maxCol = attrIds() -1;

    // After scanning first recordId, scanCol will be set to maxCol
    int scanCol = initialCol;
    
    // Scan recordIds in display order (includes hidden and unhidden rows),
    // starting at initialRecordPos.  Scan until find text, or have searched
    // the entire grid (or if !wrapSearch, the portion above the starting 
    // point).
    for (int scanRecordPos = 0; scanRecordPos <= numRecordIds; scanRecordPos++)
    {
	// Modify ScanRecordPos with initialRecordPos, so start scan
	// at initialRecordPos (and subtract scanRecordPos to go backwards)
	int modifiedScanRecordPos = initialRecordPos - scanRecordPos;

	// Do we need to wrap modifiedScanRecordPos up to bottom of grid?
	if (modifiedScanRecordPos < 0)
	{
	    // Needs wrapping and caller allows wraps, so wrap around
	    if (wrapSearch)
	    {
		// At most, only need to wrap once so can use addition
		modifiedScanRecordPos += numRecordIds;
	    }

	    // Needs wrapping, but caller disallows, so done
	    else
	    {
		// Indicate search failed
		startRecordId = NULL_INT;
		startAttrId = NULL_INT;
		startIndex = NULL_INT;

		// Return FALSE, search failed
		return (FALSE);
	    }
	}
	
	// If an end position is set, see if we've gone past it
	if( endRecordPos != NULL_INT
			&& modifiedScanRecordPos < endRecordPos ) {
		// Search has failed, so we're done
		startRecordId = NULL_INT;
		startAttrId = NULL_INT;
		startIndex = NULL_INT;
		return (FALSE);
	}

	// Get recordId at modifiedScanRecordPos
	int scanRecordId = recordIdOrder[modifiedScanRecordPos];

	// Don't scan this recordId if hidden by user, or if hidden
	// by collapsed tree and not scanning collapsed trees
	if (recordIdHiddenBy(scanRecordId, UserHides) ||
	    ((openTreesOnly) && recordIdHiddenBy (scanRecordId, TreeHides)))
	{
	    // After first recordId, always start scan from maxCol, index -1
	    scanCol = maxCol;
	    scanTextIndex = -1;

	    continue;
	}

	// Scan attrIds in column order (backwards), the first time starting at
	// intialScanCol and the rest from column maxCol
	for (; scanCol >= 0; scanCol--)
	{
	    // If an enpoint is set on this record, see if we passed it
	    if( endRecordPos == modifiedScanRecordPos && endCol != NULL_INT
				    && scanCol < endCol ) {
		    // Search failed; we're done
		    startRecordId = NULL_INT;
		    startAttrId = NULL_INT;
		    startIndex = NULL_INT;
		    return (FALSE);
	     }

	    // Get attrId in this column (since attrIds cannot be hidden yet)
	    int scanAttrId = mapColToAttrId (scanCol);

	    // Get the screen text for this cell (including numbers converted
	    // to text)
	    scanText = cellScreenText(scanRecordId, scanAttrId);

	    // Is searchText in this cell's text (search backwards)?
	    int matchLocation = scanText.findRev (searchText, scanTextIndex, 
						  caseSensitive);

	    // Test for match found
	    if (matchLocation >= 0)
	    {
		// Did the found text start at or to the left of 
		// the specified endpoint?
	        if( endRecordPos == modifiedScanRecordPos
				&& endCol == scanCol
				&& endIndex != NULL_INT
				&& matchLocation >= endIndex ) {
			// Text found but in wrong place
			startRecordId = NULL_INT;
			startAttrId = NULL_INT;
			startIndex = NULL_INT;
			return (FALSE);
		}

		// Record where the match was found
		startRecordId = scanRecordId;
		startAttrId = scanAttrId;
		startIndex = matchLocation;

		// Return that match found
		return (TRUE);
	    }

	    // All but the first test starts at end of text
	    scanTextIndex = -1;
	}

	// All but the first recordId scan starts at last column
	scanCol = maxCol;
    }
	    
    // If got here, search failed.  Set return indexes to NULL_INT.
    startRecordId = NULL_INT;
    startAttrId = NULL_INT;
    startIndex = NULL_INT;

    // Return FALSE, search failed
    return (FALSE);
}

// Registers a single cell watchpoint for John May's search algorithm
// to detect when a cell contents has been touched (don't detect if
// set to exactly the same value).  The detection is delayed until
// the custom 'flushUpdate' event occurs, to allow the search engine
// to do anything with the signal it wants to.
// If necessary, in the future we will create a general case interface 
// to do this, but for now, a specialized case is much easier to do 
// (so don't use this call unless you are John May's search algorithm).
// 
// Will cause searchEngineWatchpointTriggered(CellGrid *grid, 
// int recordId, int attrId) to be emitted when detected.
void CellGrid::setSearchEngineWatchpoint(int recordId, int attrId)
{
    // Set cellgrid to watch
    watchRecordId = recordId;  
    watchAttrId = attrId;   

    // Clear any existing watchpoint triggers
    watchpointTriggered = FALSE; 
}

// Clears the watchpoint set above
void CellGrid::clearSearchEngineWatchpoint()
{
    // Set cellgrid to watch nothing
    watchRecordId = -1;  
    watchAttrId = -1;   

    // Clear any existing watchpoint triggers
    watchpointTriggered = FALSE; 
}

void CellGrid::setFont( const QFont& font )
{
    QWidget::setFont( font );
    defaultCellFont = font;

    // Determine default cell height from height of font 
    // (pad 1 pixels on top and bottom + 1 for space between cells)
    // Get font metrics for cell font
    // If minRHeight larger than font height, use it
    QFontMetrics cfm(defaultCellFont);
    if ((cfm.height() + 3) >= (minRHeight + 1))
	recordIdFixedHeight = cfm.height() + 3;
    else
	recordIdFixedHeight = minRHeight + 1;
}

// Include the QT specific code that is automatically
// generated from the cellgrid.h
//#include "cellgrid.moc"



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

