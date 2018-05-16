//! \file cellgrid.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
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

#ifndef CELLGRID_H
#define CELLGRID_H

#include <qscrollview.h>
#include <qheader.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qcolor.h>
#include <qfont.h>
#include <intarraytable.h>
#include <stringtable.h>
#include <l_punt.h>
#include <qtimer.h>

// DEBUG
#include <stdio.h>

// Include ToolGear types, such as NULL_INT, NULL_DOUBLE, etc.
#include <tg_types.h>

#include "messagebuffer.h"

class QHeader;

//! Create CellStyleId type for style ids (and type checking)
typedef int CellStyleId;

//! Declare invalid CellStyleId for checking returned style ids
extern const CellStyleId CellStyleInvalid;

//! Create GridPixmapId type for pixmap ids (and type checking)
typedef int GridPixmapId;

//! Declare invalid GridPixmapId for checking returned pixmaps ids
extern const GridPixmapId GridPixmapInvalid;


#include <celllayout.h>
#include <cellannot.h>


//! Scrollable, sortable, annotatable, and clickable grid of text cells

/*!
 * A Qt Widget (CellGrid) designed to support the display and annotation 
 * of source code and data.  CellGrid supports a flexible annotation facility
 * and allows clicks on these annotations to be captured.  Also allows a
 * tree hierarchy to be used to collapse the display in a structured way.
 * Represented in the API as a 2D grid of cells, the cells may be sorted
 * and rearranged by the user without affecting the recordId and attrId
 * address of each cell.  The grid can be also searched  and 
 * is searched in the current display order.
 */

class CellGrid : public QScrollView
{
    Q_OBJECT

public:

    // Create grid with initial size, may be resized later
    //
    //! Creates a grid of cells where strings and pixmaps can
    //! be displayed.  Use minRowHeight to set the minimim
    //! row high in pixels (otherwise set only by font), useful
    //! if have fixed high-annotations.
    CellGrid(int recordIds, int attrIds, QFont font, int minRowHeight = 0,
	     QWidget *parent=NULL, const char *name=NULL);

    //! Free memory allocated for the grid
    //!
    ~CellGrid();

    //! Clears current grid contents (removes all tree hierarchy, styles,
    //! and cell contents) without resizing the grid and emits signal
    //! gridContentsCleared.
    //! 
    //! Should call whenever putting totally unrelated content
    //! into a cellGrid (such as changing source files or message folders).
    //! The grid searcher uses this call's signal to reset context.
    void clearGridContents ();

    //! Resize grid, deleting items outside new bounds
    void resizeGrid(int recordIds, int attrIds);

    //! Various cell types, currently only textType supported
    enum CellType{
	invalidType = 0,
	textType,     //<! Makes copy of text, must delete at end
	textRefType,  //<! Holds only pointer to text, don't delete it!
	doubleType,  
	intType      
    };

    //! A row can be hidden by the user (creater of CellGrid)
    //! or by the tree hierarchy (or both)
    enum HiddenBy 
    {
	invalidHides = 0,
	TreeHides,
	UserHides
    };

    //! Get the cell's data type, returns invalidType if empty or never created
    CellType cellType ( int recordId, int attrId) const;
    
    //! Sets cell's text at recordId, attrId.  
    //! Ignores out of bounds recordId/attrId coordinates.
    //! Cell can only have one value, setting to double/int will destroy string
    void setCellText( int recordId, int attrId, const char *text );
    
    //! QString version for setCellText() for user's convenience 
    void setCellText( int recordId, int attrId, const QString &text );

    //! Sets cell's text using a reference to "stable memory" (memory
    //! that will not be changed or freed while the cell points at
    //! it) at recordId, attrId.  Saves a copy of the pointer, not
    //! the buffer, so faster and uses less memory, but will die
    //! if the "stable memory" changes or is freed.
    //! Ignores out of bounds recordId/attrId coordinates.
    //! Cell can only have one value, setting to double/int will 
    //! remove reference to string
    void setCellTextRef( int recordId, int attrId, const char *text );

    //! Sets cell's value to an integer at recordId, attrId.  
    //! Printed to screen using sprintf's "%d" format.
    //! Ignore's out of bounds recordId/attrId coordinates.
    //! Cell can only have one value, setting to double/string will destroy int
    void setCellInt( int recordId, int attrId, int value );

    //! Sets cell's value to a double at recordId, attrId.  
    //! Printed to screen using sprintf's "%g" format.
    //! Ignore's out of bounds recordId/attrId coordinates.
    //! Cell can only have one value, setting to int/string will destroy double
    void setCellDouble( int recordId, int attrId, double value );

    //! Returns the text that will be displayed for the cell (may be
    //! string, double, or int type).  Useful if need to know how
    //! a number is being printed out.
    QString cellScreenText( int recordId, int attrId ) const;

    //! Returns the cell's text or Qstring::null if no text (i.e., 
    //! if int or double)
    QString cellText( int recordId, int attrId ) const;

    //! Returns the cell's int value or NULL_INT if int not stored in cell
    int cellInt( int recordId, int attrId ) const;

    //! Returns the cell's double value or NULL_DOUBLE if not a double
    double cellDouble( int recordId, int attrId ) const;

    //! Clear cell value (in any), and puts into "invalid" state
    void clearCellValue ( int recordId, int attrId);


    //! Creates a new style which can be used to set the style for a row/cell
    //! Styles are the *only* way to set colors, fonts, etc. for a row or cell
    CellStyleId newCellStyle (const QString &name = "unnamed");

    // Note: QColor("Red"), etc. talks to the X server each time
    // the constructor is called, which can be long-latency.
    // Create each color only once!

    //! Set box color, passing QColor() causes default behavior should be used
    void setStyleBoxColor(CellStyleId styleId, const QColor &color);
    //! Set fg color, passing QColor() causes default behavior should be used
    void setStyleFgColor(CellStyleId styleId, const QColor &color);
    //! Set bg color, passing QColor() causes default behavior should be used
    void setStyleBgColor(CellStyleId styleId, const QColor &color);

    //! Sets the display style for the cell, ignores invalid styleIds
    void setCellStyle (int recordId, int attrId, CellStyleId styleId);
    //! Sets the display style for the cell, pass CellStyleInvalid to clear
    CellStyleId cellStyle (int recordId, int attrId);

    //! Resets cell back to default settings (no contents, etc.)
    void resetCell (int recordId, int attrId);

    //! Adds pixmap at the given index and slot with the given position flags
    //! If replace == TRUE, removes all annotations with 'handle' before adding
    void addCellPixmapAnnot (int recordId, int attrId, int handle, 
			     bool replace, int layer, bool clickable,
			     int index, int slot, 
			     int posFlags, GridPixmapId pixmapId);

    //! Adds pixmap at the given index and slot with the given position flags
    //! Version that takes pixmap name instead of GridPixmapId
    //! If replace == TRUE, removes all annotations with 'handle' before adding
    void addCellPixmapAnnot (int recordId, int attrId, int handle,
			     bool replace, int layer, bool clickable,
			     int index, int slot, int posFlags, 
			     const QString &pixmapName);

    //! Makes clickable range in cell (nothing drawn for annotation)
    void addCellClickableAnnot (int recordId, int attrId, int handle,
				bool replace, int layer, 
				int startIndex, int startSlot,
				int stopIndex, int stopSlot);

    //! Adds line between the two points give with the given color.
    //! If color is QColor() (an invalid color), the line is not actually drawn
    //! but is still potentially clickable and does reserve space at endpoints.
    //! If replace == TRUE, removes all annotations with 'handle' before 
    //! adding.
    //! May want to add arrow types, etc info in future
    void addCellLineAnnot (int recordId, int attrId, int handle, 
			   bool replace, int layer, bool clickable,
			   int startIndex, int startSlot, 
			   int startPosFlags, int stopIndex, int stopSlot,
			   int stopPosFlags, const QColor &color, 
			   int lineWidth = 1);

    //! Adds rectagle between the two corner points give with the given 
    //! line color, and width, and an optional fill color.
    //! If either color is QColor() (an invalid color), that portion of
    //! of the rectagle is not drawn (or filled in) but is still potentially 
    //! clickable and does reserve space at endpoints.
    //! If replace == TRUE, removes all annotations with 'handle' before adding
    void addCellRectAnnot (int recordId, int attrId, int handle, 
			   bool replace, int layer, bool clickable,
			   int startIndex, int startSlot, 
			   int startPosFlags, 
			   int stopIndex, int stopSlot,
			   int stopPosFlags, const QColor &lineColor,
			   int lineWidth, const QColor &fillColor = QColor());

    //! Adds ellipse between the two corner points give with the given 
    //! line color, and width, and an optional fill color.
    //! If either color is QColor() (an invalid color), that portion of
    //! of the ellipse is not drawn (or filled in) but is still potentially 
    //! clickable and does reserve space at endpoints.
    //! If replace == TRUE, removes all annotations with 'handle' before adding
    void addCellEllipseAnnot (int recordId, int attrId, int handle, 
			      bool replace, int layer, bool clickable,
			      int startIndex, int startSlot, 
			      int startPosFlags, 
			      int stopIndex, int stopSlot,
			      int stopPosFlags, const QColor &lineColor,
			      int lineWidth, 
			      const QColor &fillColor = QColor());

    //! Routine to add custom annotation to cell (derived from CellAnnot 
    //! interface).
    //! Doesn't add out of bounds recordId/attrId coordinates and deletes annot
    //! if coordinates invalid.  Doesn't copy annot and will delete annot
    //! when annotation removed. Blindly adds annotation regardless of handle. 
    //! If don't want multiple annotations with same handle, must call 
    //! deleteCellAnnot() routine first. Redraws cell after adding annotation.
    void addCellAnnot(int recordId, int attrId, CellAnnot *annot);

    //! External routine to remove all annotations from cell with 'handle'.
    //! If redraw TRUE and something deleted, redraws cell without annotations.
    //! Set redraw to FALSE if calling this routine in addCellAnnot routine.
    //! Returns number of annotations deleted.
    //! Ignores out of bounds recordId/attrId coordinates. 
    int removeCellAnnot(int recordId, int attrId, int handle, 
			bool redraw = TRUE);


    //! Returns the slot width in pixels.  Returns 0 if slot doesn't exist
    //! or there is no width requirements on slot.
    int getSlotWidth (int recordId, int attrId, int index, int slot);

    //! Returns the actual width (in pixels) used by the cell (when drawn).
    //! Note that not all of the pixels may be currently visible
    //! due to the attrid's header width.  This call actually does
    //! the cell layout used by the graphics engine, so is somewhat expensive.
    //! Return 0 for out of bounds cells.
    int cellWidth (int recordId, int attrId);
    
    //! Returns number of annotations in the cell with the given handle.
    //! Returns 0 if cell doesn't exist or handle does not occur.  
    //! Lineaerly scan each annotation to determine number of times exists.
    int handleExist (int recordId, int attrId, int handle);

    //! Returns the maximum annotation handle in the cell.
    //! Returns NULL_INT if no annotations in the cell.
    //! Linearly scans each annotation to determine max handle.
    int maxHandle (int recordId, int attrId);

    //! Registers a new pixmap with the grid.  The returned id then can
    //! be used to set a cell's value to a pixmap.  
    //! Now can also look up by name, so will punt on duplicate names!
    GridPixmapId newGridPixmap (const QPixmap &pixmap, 
				const QString &name);

    //! Sets the visibility of the header
    void setHeaderVisible (bool visible);

    //! Sets attrId header text.  Ignore's out of bounds attrIds.
    void setHeaderText( int attrId, const QString &text);

    //! Sets attrId header iconset and text.  Ignore's out of bounds attrIds.
    void setHeaderText( int attrId, const QIconSet &iconset, 
			const QString &text);

    //! Returns header text.  Returns NULL_QSTRING for out of bounds attrId
    QString headerText( int attrId ) const;
    
    //! Returns header iconset.  Returns NULL if no iconSet or out of bounds
    QIconSet *headerIconSet (int attrId) const;

    //! Sets attrId header width.  Ignore's out of bounds attrIds or widths
    void setHeaderWidth( int attrId, int pixels);

    // All the calls below return/take values in terms of scrollview pixels
    //! topHeader manages attrId width, use its info
    //! Returns 0 for out of bounds attrIds
    int attrIdWidth( int attrId ) const;

    //! Use stored recordId height for each recordId.
    //! Constant for each recordId for now, take recordId number of future
    //! enhancements.  Returns 0 for out of bounds recordIds.
    int recordIdHeight( int ) const;

    //! Get starting pixel position of attrId
    //! Returns 0 for out of bounds attrIds
    int attrIdXPos( int attrId ) const;

    //! Get starting pixel position of recordId in y dimension (down)
    //! Assumes fixed recordId height and only topHeader at top.
    //! Returns 0 for out of bounds recordIds
    int recordIdYPos( int recordId );

    //! Returns the attrId for the specified x direction (across) pixel
    //! Returns -1 if pixel not in a valid attrId.
    int attrIdAt( int xPos ) const;

    //! Returns recordId of the specified y direction (down) pixel.
    //! Returns -1 if pixel not in a valid recordId
    int recordIdAt( int yPos );

    //! Return size, in pixels, of the grid.
    QSize gridSize();

    //! Returns offset (from start of cell) to leftmost pixel of slot.
    //! That is, returns number between 0 and (width of cell)-1.
    //! Returns 0 for out of bounds recordId/attrIds.
    //! Currently punts if invalid index/slot passed, so better only
    //! call for index/slots that really exist!
    int slotXOffset (int recordId, int attrId, int index, int slot);

    //! Returns the number of recordIds in the grid.
    int recordIds() const {return numRecordIds;}

    //! Returns the number of attrIds in the grid.
    int attrIds() const { return numAttrIds;}

    //! Returns the number of rows currently visible to user. 
    //! Independent of window size (i.e., visible if using scroll bar)
    int rowsVisible() const {return numRowsVisible;}

    //! Scrolls window so cell at recordId and attrId is visible in window.
    //! If recordId or attrId are NULL_INT, focuses on row or column
    //! specified.  Scrolls minimum possible to ensure visibility.
    //! Does nothing if recordId hidden (scrolling will not help).
    //! To determine if scrolling will take place, use 'cellScrolledOff()'.
    //! Now takes rowMargin, default place at least a row from the edge.
    void scrollCellIntoView (int recordId, int attrId, int rowMargin=1);

    //! Returns TRUE iff scrollCellIntoView will scroll the view window.
    bool cellScrolledOff (int recordId, int attrId);

    //! Centers cell (the cell's center pixel)in display,
    //! with the given margins.
    //! xmargin and ymargin must be between 0.0 and 1.0.
    //! Margin 1.0 ensures centered.  Margin 0.5 is middle 50% of visible area.
    //! Margin 0.0 is just visible somewhere.
    void centerCellInView (int recordId, int attrId, float xmargin = 0.5,
			   float ymargin = 0.5);

    //! Centers recordId in display, without changing the horizonal position
    void centerRecordIdInView (int recordId);


    //! Returns TRUE if recordId is hidden (or out of bounds), FALSE if not 
    bool recordIdHidden (int recordId);

    //! Returns TRUE if recordId is hidden by 'hiddenBy' (or out of bounds), 
    //! FALSE if not.  TreeHides and UserHides are current valid values for
    //! hiddenBy.
    bool recordIdHiddenBy (int recordId, HiddenBy hiddenBy);

    //! Hides a range of recordIds, so they will not be displayed to user.  
    //! Hidden recordIds are still valid for all cell updates, etc.  
    //! Ignores out of bounds recordIds.
    void hideRecordId(int startRecordId, int endRecordId)
	{hideRecordId (startRecordId, endRecordId, UserHides);}
    //! Variant of above that hides a single recordId
    void hideRecordId(int recordId) 
	{hideRecordId(recordId, recordId, UserHides);}

    //! Unhides recordId, so it will now be displayed to user. 
    //! Ignores out of bounds recordIds or already visible recordIds.
    void unhideRecordId(int recordId) 
	{unhideRecordId(recordId, recordId, UserHides);}
    //! Variant of above that unhides a range of recordIds
    void unhideRecordId(int startRecordId, int endRecordId)
	{unhideRecordId(startRecordId, endRecordId, UserHides);}


    //! Returns the recordId above the passed recordId, based on the
    //! current recordId display order (i.e., modified by sorting).  
    //! Independent of "hidden" status of recordId. (i.e., can return 
    //! hidden recordIds).  
    //! Returns NULL_INT if nothing above (passed top recordId or
    //! invalid recordId).
    int recordIdAbove(int recordId);

    //! Returns the recordId below the passed recordId, see recordIdAbove()
    //! for details.
    int recordIdBelow(int recordId);

    //! Places recordId below 'belowRecordId', and if parentId is valid,
    //! places in tree structure with parentId as parent.  If belowRecordId
    //! is not valid (doesn't exist or is not child of parentId), will
    //! place record as high as possible in current tree heirarchy level.
    //! If both belowRecordId and parentId are invalid (NULL_INT), will
    //! move recordId to top level of tree, after the end of the current tree
    void placeRecordId (int recordId, int belowRecordId, 
			int parentRecordId = NULL_INT)
	{placeRecordId(recordId, recordId, belowRecordId, parentRecordId);}

    //! This variant places a range of recordIds, in order, as
    //! specified by placeRecordId (int recordId, int belowRecordId,
    //!                        int parentRecordId = NULL_INT).
    //! Don't default arguments do can distiguish from above version.
    void placeRecordId(int startRecordId, int endRecordId, int belowRecordId, 
		       int parentRecordId);

    //! Sets the attrId to place the tree annotations at.
    //! Setting to NULL_INT will hide all tree annotations.
    //! Ignores invalid attrIds other than NULL_INT.
    void setTreeAttrId (int attrId);

    //! Return TRUE if recordId has children or is expandable and is
    //! currently in the open state.  Otherwise, returns FALSE
    bool isTreeOpen (int recordId);

    //! Opens or closes a tree node (i.e., shows or hides tree nodes children).
    //! Does nothing if recordId is not a tree node. 
    //! Trees currently default to closed when initially created.
    void setTreeOpen(int recordId, bool open);

    //! Makes node expandable (or not expandable) even if doesn't currently
    //! have any children.  Make recordId look like a tree node and
    //! only does something if recordId doesn't have any children.
    void setTreeExpandable (int recordId, bool expandable);

    //! Returns parent recordId in tree, if available, or NULL_INT if not.
    int recordIdParent (int recordId);
    
    //! Returns recordId of first child of 'recordId' in tree, if available, 
    //! or NULL_INT if not.  If passed in 'recordId' is NULL_INT, returns
    //! first child of top level tree, if any (i.e., if grid not empty).
    int recordIdFirstChild (int recordId);
    
    //! Returns recordId of last child of 'recordId' in tree, if available, 
    //! or NULL_INT if not.  If passed in 'recordId' is NULL_INT, returns
    //! last child of top level tree, if any (i.e., if grid not empty).
    int recordIdLastChild (int recordId);

    //! Returns recordId of next sibling of 'recordId' at same level in the 
    //! tree, if available, or NULL_INT if not. 
    int recordIdNextSibling (int recordId);

    //! Returns recordId of previous sibling of 'recordId' at same level in 
    //! the tree, if available, or NULL_INT if not. 
    int recordIdPrevSibling (int recordId);

    //! If update pending, calls updateContents() to update screen
    //! Automatically called when control returned to user, so should
    //! not be ever necessary to call, unless you want the screen
    //! to be updated but the GUI not to be responsive (i.e. in spin
    //! loop)
    void flushUpdate();

    //! If reconstruction pending, calls reconstructPlacementArrays() to
    //! completely reconstruct the placement arrays.
    void flushReconstruction()
	{
	    if (reconstructionPending)
		reconstructPlacementArrays();
	}

#if 0
// DEBUG
#define flushReconstruction()  if(reconstructionPending){fprintf (stderr, "Flush required '%s' line %i\n", __FILE__, __LINE__);	reconstructPlacementArrays();}
#endif


    //! If visible area resize is pending, calls syncGridSize() to
    //! make sure the scrollable area is sized appropriately
    void flushSync()
	{
	    if (syncPending)
	    {
		syncGridSize();
	    }
	}
    
    //! Sorts the recordIds based on the attrId's values.
    //! If direction > 0, sort ascending (values go up as the rows go down 
    //! the display).  If < 0, sort decending. 
    //! If =0, unsorted (restores recordId order).
    //! Use line number as tie breaker.
    //! Ignores out of bounds attrId values.
    void sortAttrIdView (int attrId, int direction);

    //! Finds text in grid and allows search direction to be specified
    //! by parameter forwardSearch.  See findTextForward() and 
    //! findTextBackward() for details, since they do all the work.
    //! The optional end... parameters specify an endpoint for the
    //! search other than the end of grid.  (See findTextForward()
    //! and findTextBackward() for exact semantics.)  These endpoints
    //! override setting wrapSearch to TRUE, so starting a search
    //! after the endpoint will fail immediately.
    bool findText (int &startRecordId, int &startAttrId, 
		   int &startIndex, const QString &searchText, 
		   bool forwardSearch, bool caseSensitive, 
		   bool openTreesOnly, bool wrapSearch,
		   int endRecordId = NULL_INT, int endAttrId = NULL_INT,
		   int endIndex = NULL_INT);

    //! Searches forward for searchText in the grid, starting at startRecordId,
    //! startAttrId, and startIndex in display order (that is, in the
    //! order rows and columns are displayed on the screen, not how they
    //! are indexed by recordId and AttrId).  These start positions may 
    //! be NULL_INT, in which case the left/topmost position will be chosen.
    //! Search can be made case-sensitive by setting caseSensitive to TRUE.  
    //! In order to search both open and closed trees, set openTreesOnly 
    //! to FALSE. Does not search recordIds hidden with hideRecordId().  
    //! If wrapSearch == FALSE, stops unsuccessful searches after search last
    //! recordId.  Otherwise if wrapSearch == TRUE, will wrap around and 
    //! continue search until hits starting point (or match is found).
    //!
    //! If match for searchText found, returns TRUE and the exact leftmost 
    //! character location of matching text is stored in startRecordId, 
    //! startAttrId, and startIndex.
    //! Otherwise, if no match, FALSE is returned otherwise and startRecordId,
    //! startAttrId, and startIndex set to NULL_INT.
    //!
    //! If an endpoint is specified, any item that begins at or beyond
    //! the endpoint will not be found (and no wrap-around will happen).
    //! If a valid value is given for endRecordId and NULL_INT is given
    //! for endAttrId, then the endpoint of the search will be at the
    //! beginning of the first record after the endRecordId (i.e., items
    //! beginning there will not be found).  Likewise, if valid values
    //! are given for both endRecordId and endAttrId, the search endpoint
    //! will be at the first attr following the specified record and attr.
    //!
    //! Siliently treats invalid startRecordId and startAttrId as if they were 
    //! set to NULL_INT.   Silently treats out-of-bounds startIndex as 0 or
    //! a directive to goto the next cell, as appropriate.
    //!
    //! To continue search from last found text position, just add 1 to 
    //! startIndex.  
    //!
    //! See findTextBackward() in order to search the grid backwards and
    //! findText() to specify direction in a parameter.
    bool findTextForward (int &startRecordId, int &startAttrId, 
			  int &startIndex, const QString &searchText, 
			  bool caseSensitive, bool openTreesOnly, 
			  bool wrapSearch, int endRecordId = NULL_INT,
			  int endAttrId = NULL_INT, int endIndex = NULL_INT);
    
    //! Searches backward for searchText in the grid.
    //! Similiar to findTextForwards() except searches backwards thru grid.
    //!
    //! If an endpoint is specified, and item that begins at or before
    //! the endpoint will not be found.
    //!
    //! To continue search from last found text position, just subtract 1 
    //! from startIndex.  
    bool findTextBackward (int &startRecordId, int &startAttrId, 
			   int &startIndex, const QString &searchText, 
			   bool caseSensitive, bool openTreesOnly, 
			  bool wrapSearch, int endRecordId = NULL_INT,
			  int endAttrId = NULL_INT, int endIndex = NULL_INT);

    //! Registers a single cell watchpoint for John May's search algorithm
    //! to detect when a cell contents has been touched (don't detect if
    //! set to exactly the same value).  The detection is delayed until
    //! the custom 'flushUpdate' event occurs, to allow the search engine
    //! to do anything with the signal it wants to.
    //! If necessary, in the future we will create a general case interface 
    //! to do this, but for now, a specialized case is much easier to do 
    //! (so don't use this call unless you are John May's search algorithm).
    //! 
    //! Will cause searchEngineWatchpointTriggered(CellGrid *grid, 
    //! int recordId, int attrId) to be emitted when detected.
    void setSearchEngineWatchpoint(int recordId, int attrId);

    //! Clears the watchpoint set above
    void clearSearchEngineWatchpoint();

    //! Generic handler for mouse events.
    //! Currently handles mouse single, double click, and move events.
    //!
    //! Currently assumes both callers flush graphics updates to reduce
    //! visual lag, so not done inside this routine.
    void MouseEventHandler (QMouseEvent *e);

    //! Overload virtual function to catch mouse movements.
    void contentsMouseMoveEvent (QMouseEvent* e);

    // Overload virtual funtion to handle singleclicks
    void contentsMousePressEvent(QMouseEvent* e);

    // Overload virtual funtion to handle double clicks
    void contentsMouseDoubleClickEvent(QMouseEvent* e);

    // Overload virtual funtion to handle mouse button releases
    void contentsMouseReleaseEvent(QMouseEvent* e);

    //! Redefine customWhatsThis() to return false so that clicks
    //! on the cell grid get passed on in WhatsThis mode.
    bool customWhatsThis() const;

    //! Handle key press events, mainly page up/down
    void keyPressEvent(QKeyEvent *e);

    //! Handle custom events, mainly flushUpdates
    void customEvent (QCustomEvent *e);
    
    //! Reimplementation of QObject version so we can emit a signal.
    void setName(const char * name);

    //! Overloaded version resets some sizes based on new
    //! font size.
    void setFont( const QFont& f );

#if 0
    //! Does nothing with focus-in event
    void focusInEvent(QFocusEvent*);

    //! Does nothing with focus-out event
    void focusOutEvent(QFocusEvent*);
#endif


    //! Internal style structure that is directly accessable by
    //! protected redraw routines (for efficiency)
    struct CellStyle {
	CellStyle(const QString &name, CellStyleId id) :
	    styleName(name),
	    styleId(id)
	{
	    // NULL specifies color not specified, use default color
	    // (which may be row default or global default color)
	    boxColor = NULL;
	    fgColor = NULL;
	    bgColor = NULL;
	}

	~CellStyle() 
	{
	    if (boxColor != NULL)
		delete boxColor;
	    if (fgColor != NULL)
		delete fgColor;
	    if (bgColor != NULL)
		delete bgColor;
	}
    private:
	//! Only allow CellGrid routines to access data!
	friend class CellGrid; 

	QString styleName;
	CellStyleId styleId;
	QColor *boxColor; 
	QColor *fgColor;
	QColor *bgColor;
    };

    //! Internal pixmap structure to allow more efficient storage/setting of
    //! pixmaps in the grid. Directly accessable by protected redraw routines
    struct GridPixmap {
	GridPixmap (const QPixmap &pix, GridPixmapId id, const QString &name):
	    pixmapName (name), pixmapId(id), pixmap(pix) 
	{}

	
    private:
	//! Only allow CellGrid routines to access data!
	friend class CellGrid; 

	QString pixmapName;
	GridPixmapId pixmapId;
	QPixmap pixmap;
    };

    //! Lightweight container for CellAnnot pointers.
    //! May want to come back and investigate STL versions later,
    //! but need something very light-weight.
    struct CellAnnotNode
    {
    public:
	// Creates node that points to CellAnnot
	CellAnnotNode (CellAnnot *a) : 
	    annot (a), next(NULL), prev(NULL) {}
	
	// When deleted, deletes cell annotation
	~CellAnnotNode () {delete annot;}
	
	// Access these directly for now
	CellAnnot *annot;
	CellAnnotNode *next;
	CellAnnotNode *prev;
    };


    //! Contents of one cell in a CellGrid
    //! Created in default, empty state.
    class Cell
    {
    public:
	// Allow libraries CellGrid uses to create/delete cells
	Cell( ) 
	{
	    // Initialize new cell contents
	    style = NULL;
	    annotList = NULL;
	    type = invalidType;
	} 

	~Cell()
	    {
		// Delete any hanging strings in cell
		if (type == textType)
		{
		    delete [] textVal;
		}
	    }

	//! Public interface to query value of cell internals
	//! Intended for sorting purposes only and user better
	//! use getType to get the correct value, otherwise will
	//! get gibberish!
	inline CellType getType() const { return type; }
	inline const char *getText() const { return textVal;}
	inline double getDouble() const { return doubleVal;}
	inline double getInt() const { return intVal;}

    private:
	// Only allow CellGrid routines to set data values!
	friend class CellGrid; 

	//! Pointer to cell's style, NULL if not explicitly set
	CellGrid::CellStyle *style;

	//! List of cell annotations, sorted by layer and then by
	//! order inserted.  This is the head of the list to minimize space
	CellGrid::CellAnnotNode *annotList;

	//! Indicates which union value (if any) is valid
	CellType type;

	//! Union of possible values, used CellType
	union {
	    const char   *textVal;
	    double  doubleVal;  //!< Not yet implemented
	    int     intVal;     //!< Not yet implemented
	};
    };



signals:
    //! Signals that a clickable cell annotation has been clicked on
    //! (with button buttonState), with handle at the given layer.
    //! In addition to index/slot clicked on, the relative x,y of the click
    //! in cell of dimentions w,h is also provided.  
    void cellAnnotClicked (int recordId, int attrId, ButtonState buttonState, 
			   int handle, int layer, int index, int slot, 
			   int x, int y, int w, int h);

    //! Signals that a clickable cell annotation has been double clicked on
    //! (with button buttonState), with handle at the given layer.
    //! In addition to index/slot clicked on, the relative x,y of the click
    //! in cell of dimentions w,h is also provided.  
    void cellAnnotDoubleClicked (int recordId, int attrId, 
				 ButtonState buttonState, 
				 int handle, int layer, int index, int slot, 
				 int x, int y, int w, int h);

    //! Signals that a clickable cell annotation has been moved over 
    //! by the mouse, passes same info as cellAnnotClicked().
    void cellAnnotMovedOver (int recordId, int attrId, ButtonState buttonState,
			     int handle, int layer, int index, int slot, 
			     int x, int y, int w, int h);

    //! Signals the location clicked on in a cell (annotation not clicked on).
    //! In addition to index/slot clicked on, the relative x,y of the click
    //! in cell of dimentions w,h is also provided.  
    void cellSlotClicked (int recordId, int attrId, ButtonState buttonState, 
			  int index, int slot, int x, int y, int w, int h);

    //! Signals the location double clicked on in a cell 
    //! (annotation not double clicked on).
    //! In addition to index/slot clicked on, the relative x,y of the click
    //! in cell of dimentions w,h is also provided.  
    void cellSlotDoubleClicked (int recordId, int attrId, 
				ButtonState buttonState, 
				int index, int slot, int x, int y,
				int w, int h);

    //! Signals the mouse is over a cell (but not over an annotation).
    //! Passes same info as cellSlotClicked().
    void cellSlotMovedOver (int recordId, int attrId, ButtonState buttonState, 
			    int index, int slot, int x, int y, int w, int h);


    //! Signals the tree hierarchy control area has been clicked on
    //! treeOpen reflects the tree state *after* the click has been
    //! processed (click usually flips state).  If treeOpen == FALSE,
    //! and parentRecordId != recordId, then recordId is probably no longer
    //! visible.
    void cellTreeClicked (int recordId, int attrId, ButtonState buttonState, 
			  int indentLevel, int parentRecordId, bool treeOpen, 
			  int cellx, int celly, int cellw, int cellh);

    //! Signals the tree hierarchy control area has been double clicked on
    //! treeOpen reflects the current tree state (the first click of the 
    //! double click probably flips state).  If treeOpen == FALSE,
    //! and parentRecordId != recordId, then recordId is probably no longer
    //! visible.
    void cellTreeDoubleClicked (int recordId, int attrId, 
				ButtonState buttonState, 
				int indentLevel, int parentRecordId,
				bool treeOpen, int cellx, int celly, 
				int cellw, int cellh);
    
    //! Signals the mouse is over the tree hiearchy control area.
    //! Passes same info as cellTreeClicked().
    void cellTreeMovedOver (int recordId, int attrId, ButtonState buttonState,
			    int indentLevel, int parentRecordId, 
			    bool treeOpen, int cellx, int celly, int cellw, 
			    int cellh);

    //! Signals an empty area where no cells live has been clicked.
    //! One or both of recordId or attrId will be -1, for no mapping possible.
    void emptyAreaClicked (int recordId, int attrId, 
			   ButtonState buttonState, int gridx, int gridy);

    //! Signals an empty area where no cells live has been double clicked.
    //! One or both of recordId or attrId will be -1, for no mapping possible.
    void emptyAreaDoubleClicked (int recordId, int attrId, 
				 ButtonState buttonState, 
				 int gridx, int gridy);

    //! Signals the mouse is over an empty area where no cells live.
    //! One or both of recordId or attrId will be -1, for no mapping possible.
    void emptyAreaMovedOver (int recordId, int attrId, 
			     ButtonState buttonState, int gridx, int gridy);

    //! Signals that the CellGrid has received a resizeEvent
    void gridResizeEvent( QResizeEvent *e );

    //! Signals a mouse click anywhere in the grid (emitted before the
    //! more specific click signals)
    void gridClicked( CellGrid * );

    //! Signals a mouse double click anywhere in the grid (emitted before the
    //! more specific double click signals)
    void gridDoubleClicked( CellGrid * );

    //! Signals a mouse is over anywhere in the grid (emitted before the
    //! more specific moved over signals)
    void gridMovedOver( CellGrid * );

    //! Signals that this object is being destroyed (QObject signal should
    //! do the same thing, but it doesn't seem to work)
    void destroyed( CellGrid * );

    //! Signals that the entire grid contents have been cleared.
    //! Useful you have content-specific context (such as search location)
    void gridContentsCleared( CellGrid * );

    //! Signals that the search engine watchpoint was triggered during
    //! a recent redraw event on the cell being watched.
    void searchEngineWatchpointTriggered(CellGrid *grid, int recordId, 
					 int attrId);

    //! Signals that the name of this object has changed
    void nameChanged( CellGrid * );

    //! Signals that the grid has been hidden
    void cellgridHidden( CellGrid * );

    //! Signals that the grid has been shown
    void cellgridShown( CellGrid * );

protected:

    //! Internal routine to hide a range of recordIds, so they will not be 
    //! displayed to user.   Hidden recordIds are still valid for all cell 
    //! updates, etc.   Ignores out of bounds recordIds.
    //! Records currently can be hidden by Trees(TreeHides) or 
    //! by the user (UserHides)
    //! NOTE: Only hideTreeChildren() should set externalFixup to TRUE,
    //! which requires the caller (or caller's caller) to fix up the
    //! arrays and screen!  (Makes update much more efficient!)
    void hideRecordId(int startRecordId, int endRecordId, HiddenBy hiddenBy,
		      bool externalFixup = FALSE);

    //! Internal version of above for one record.
    //! NOTE: Must define this version or else using HiddenBy as second
    //! argument will resolve to public version, causing a low number to
    //! be passed as the end point, hiding everything!
    void hideRecordId(int recordId, HiddenBy hiddenBy, 
		      bool externalFixup = FALSE)
	{hideRecordId(recordId, recordId, hiddenBy, externalFixup);}
	    
    

    //! Internal routine to unhide a range of recordIds, so it will now be 
    //! displayed to user.  Ignores out of bounds recordIds or already 
    //! visible recordIds.
    //! RecordId may still be hidden after call if both tree and user
    //! hid the record!
    //! NOTE: Only hideTreeChildren() should set externalFixup to TRUE,
    //! which requires the caller (or caller's caller) to fix up the
    //! arrays and screen!  (Makes update much more efficient!)
    void unhideRecordId(int startRecordId, int endRecordId, HiddenBy hiddenBy,
			bool externalFixup = FALSE);

    //! Internal version of above for one record.
    //! NOTE: Must define this version or else using HiddenBy as second
    //! argument will resolve to public version, causing a low number to
    //! be passed as the end point, unhiding everything!
    void unhideRecordId(int recordId, HiddenBy hiddenBy,
			bool externalFixup = FALSE)
	{unhideRecordId(recordId, recordId, hiddenBy, externalFixup);}


    //! Internal routine that gets CellStyle pointer from id.
    inline CellStyle *getStyle (CellStyleId styleId)
    {
	return (styleTable.findEntry((int)styleId));
    }

    //! Internal routine that gets GridPixmap pointer from id.
    inline GridPixmap *getPixmap (GridPixmapId pixmapId)
    {
	return (pixmapIdTable.findEntry((int)pixmapId));
    }

    //! Internal routine that gets GridPixmap pointer from name.
    inline GridPixmap *getPixmap (const QString &pixmapName)
    {
	return (pixmapNameTable.findEntry((char *)pixmapName.latin1()));
    }

    //! Internal routine to set pointer to style color pointer appropriately.
    void setStyleColor (QColor * &styleColor, const QColor &color);

    //! The main/only draw routine in CellGrid.  Only the portion
    //! of the screen requested is/should be drawn.  All the
    //! clipping stuff appears to be handled appropriately, so
    //! if go slightly out of bounds, it still seems to work.
    void drawContents(QPainter* p, int cx, int cy, int cw, int ch);

    //! Override Scrollview function to allow us to
    //! adjust window geometries, even those caused by scroll bars disappearing
    void viewportResizeEvent ( QResizeEvent *e );

    //! Override Scrollview function to allow us to adjust
    //! window properties just before it is shown to user
    //! (both the first time and after being restored from minimized state).
    //! Use this to set contents area size, etc.
    //! Also emits the signal cellgridShown()
    void showEvent( QShowEvent *e );

    //! Reimplementation of QObject version so we can emit 
    //! the signal cellgridHidden().
    void hideEvent( QHideEvent * );


    //! Allow derived classes to change empty area fill algorithm.
    //! In cases where the window is bigger than the contents area, we
    //! need to paint that section (especially since we set WRepaintNoErase,
    //! which will leave random pixels sets in the outside areas).
    //! Since now allow painter to be pixmap that is offset, use
    //! cx, cy for position calculations but use px, py when actually painting.
    //! Expect px,py to be either cx, cy or 0, 0.
    virtual void paintEmptyArea( QPainter *p, int cx, int cy, 
				 int cw, int ch, int px, int py );

    //! Returns the cell pointer at recordId,attrId.  NULL if doesn't exist or
    //! out of bounds.  Does not allocate cells!  See getCreatedCell().
    Cell *getCell(int recordId, int attrId) const; 

    //! Returns the cell pointer at recordId, attrId (if it exists), otherwise
    //! creates a new "empty" cell at that location and returns a pointer
    //! to it.  Returns NULL if recordId/attrId is out of bounds.
    Cell *getCreatedCell(int recordId, int attrId);

    //! Internal routine to add annotation to cell.
    //! Does not redraw cell.  Does not check for duplicate handles.
    //! Used by external routine of same name and by other annotation routines.
    void addCellAnnot(Cell *cell, CellAnnot *annot);

    //! Internal routine to remove annotations with handle from cell.
    //! Does not redraw cell.  Will remove all annotations with handle.
    //! Used by external routine of same name and by other annotation routines.
    int removeCellAnnot(Cell *cell, int handle);

    //! Mapping routines between attrId display position and actual attrId ids.
    //! Returns the actual attrId which is displayed at col.
    //! Return -1 for out of bounds col values.
    int mapColToAttrId(int col) const;

    //! Returns the col at which the 'attrId' is currently displayed.
    //! Return -1 for out of bounds attrId values.
    int mapAttrIdToCol(int attrId) const;

    //! Mapping routines between display row and actual recordId ids.
    //! Returns the recordId which is displayed at 'row'.
    //! Return -1 for out of bounds row values.
    int mapRowToRecordId(int row);

    //! Returns the row at which the 'recordId' is currently displayed.
    //! Return -1 for out of bounds recordId values.
    int mapRecordIdToRow(int recordId);

    //! Internal routine to sync the underlining scrollview widget with
    //! the new grid size.
    void syncGridSize ();

    //! Internal worker routine for finding the Y value that needs to be
    //! visible in order to have the recordId visible.
    //! If NULL_INT passed, picks topmost pixel on screen.
    //! Used by scrollCellIntoView() and cellScrolledOff().
    int calcLeastVisibleY (int recordId);

    //! Internal worker routine for finding the Y value that needs to be
    //! visible in order to have the recordId visible.
    //! If NULL_INT passed, picks leftmost pixel on screen.
    //! Used by scrollCellIntoView() and cellScrolledOff().
    int calcLeastVisibleX (int attrId);

private slots:
    //! Slot called just before contents of scrollview window moved.
    //! Found better to use this signal to scroll the topHeader 
    //! instead of the scrollbar's valueChanged signal.  This
    //! event happens *before* the rest of the window scrolls
    //! and I think it makes the header scroll look much better.
    void contentsMoved(int x, int y);

    //! Attached to topHeader's sizeChanged signal.
    //! Only need attrId id, not sizes.
    void attrIdWidthChanged( int attrId, int, int );

    //! Handle attrIds moving around.  
    //! Redraw from the smallest col number passed.
    void attrIdMoved(int, int from, int to);

    //! Handles attrId header clicks, will call routine
    //! to sort the view based on the attrId's value
    void attrIdHeaderClicked( int attrId);

    //! Handles scrollTimer ticks to scroll text during text selection
    void scrollTimerHandler();

    //! Handles clipboard selection change signals
    void clipboardChangedHandler();
private:
    struct _CompareData 
    {
	    class CellGrid * cg;
	    int attrId;
    };			// Used to pass data to compare function when sorting

    // All recordId specific info, such as what row it is displayed in,
    // is it visible, its default ordering, is it in a tree, etc.
    struct RecordIdInfo 
    {
	// Tried to group items used together next to eachother in orer
	// to be cache friendly as possible

	// Tree management info
	RecordIdInfo *parent;     //!< Parent in tree structure (may be NULL)
	RecordIdInfo *firstChild; //!< First child (may be NULL)
	RecordIdInfo *lastChild;  //!< Last child (may be NULL)
	RecordIdInfo *prevSib;    //!< Previous sibling  (may be NULL)
	RecordIdInfo *nextSib;    //!< Next sibling (may be NULL)

	//! Used by placeRecordId() to maintain display order while placing
	//! and sorting recordIds.  Always determines order that recordIds
	//! are displayed on the screen
	RecordIdInfo *firstPlacedChild;  //!< First child in placement list
	RecordIdInfo *lastPlacedChild;   //!< Last child in placement list
	RecordIdInfo *prevPlacedSib;     //!< Next sib in placement list
	RecordIdInfo *nextPlacedSib;     //!< Next sib in placement list

	// General info about this recordId
	int recordId;  //!< RecordId this info is for (used in tree management)
	int row;       //!< Converts recordId ids to row (allows sorting)
	int displayOrder;   //!< Current index into recordIdOrder array

	// NOTE: Do group bit-field variables together for efficiency!

	// Use bit fields for varous recordIdInfo state settings
	uint visible: 1;   //!< 1 if recordId visible (allows recordId hiding)

	// Current reasons why a recordId may not be visible
	uint userHides: 1; //!< 1 if user is hiding RecordId 
	uint treeHides: 1; //!< 1 if tree (a parent closed) is hiding RecordId

	// Use bit fields for various tree state flags
	uint treeOpen: 1;       //!< Tree is open (children shown)
	uint treeExpandable: 1; //!< Tree can be expanded (even if no children)

	// Use bit fields for temporary flags used by various algorithms
	uint beingMoved: 1;     //!< Temp flag indicated record being moved
    };

    //! Internal routine that detaches this record info from current position.
    //!
    //! NOTE: After calling this routine, recordInfo must be either 
    //! immediately attached with attachRecordInfo or be considered invalid 
    //! (i.e., not used after shrinking the grid).  
    //!
    //! Do not call for something already detached!  It will destroy
    //! the tree!
    void detachRecordInfo (RecordIdInfo *recordInfo);

    //! Internal routine that attaches recordInfo with the specified parent
    //! below 'belowInfo'.  If parentInfo is NULL, puts in top level of tree.
    //! If belowInfo is NULL, makes first child of parent.
    //!
    //! NOTE: Before calling this routine, recordInfo either must have been
    //! detached using detachRecordInfo or have been just created
    //! (i.e., created while expanding the grid.).
    void attachRecordInfo(RecordIdInfo *recordInfo, RecordIdInfo *parentInfo, 
			  RecordIdInfo *belowInfo);

    //! Internal helper routine that gets the next tree node in tree order.
    //! Given the first child of a parent (and a depth set initially to 0), 
    //! it walk the tree in order to return all the children, grandchildren of 
    //! that parent in tree order.  Returns NULL when there is nothing else
    //! to return for that parent.  
    //!
    //! If openOnly is TRUE, only will return children of
    //! open nodes (otherwise ignores open/closed state).
    RecordIdInfo *nextTreeNode (RecordIdInfo *childInfo, 
				int &depth, bool openOnly);

    //! Internal helper routine that gets the next *placed* tree node in 
    //! tree order.  Given the first placed child of a parent (and a depth 
    //! set initially to 0), it walk the tree in order to return all the 
    //! placed children, grandchildren of that parent in tree placement order.
    //! Returns NULL when there is nothing else to return for that parent.  
    //!
    //! If openOnly is TRUE, only will return children of
    //! open nodes, (otherwise ignores open/closed state).
    RecordIdInfo *nextPlacedTreeNode (RecordIdInfo *childInfo, 
				      int &depth,  bool openOnly);

    //! Internal heler routine that reconstructs the recordId placement 
    //! arrays used to display the records, namely recordIdOrder, 
    //! recordIdInfo, and row2RecordId.
    //! Called after sorting or placing records.
    void reconstructPlacementArrays();


    //! Internal sortAttrIdView helper routine that sorts firstChild and 
    //! all its siblings. Destroys contents of recordIdOrder, which it uses 
    //! as temp space.  Results put into nextPlacedSib and prevPlacedSib 
    //! and the first and last placed child is returned in the last
    //! two parameters.
    void sortTreeChildren (RecordIdInfo *firstChild, 
			   _CompareData *cd,
			   int direction,
			   RecordIdInfo **firstPlacedChild,
			   RecordIdInfo **lastPlacedChild);
			 
    //! Internal sortAttrIdView helper routine that sorts firstChild and 
    //! all its siblings.  Destroys contents of recordIdOrder, which it uses 
    //! as temp space.  Results put into nextPlacedSib and the first placed 
    //! child is returned.
    RecordIdInfo *sortTreeChildren (RecordIdInfo *firstChild, 
				    _CompareData *cd, int direction);

    //! Internal routine to hide/unhide the children of a tree node.
    //! Used by both setTreeOpen() and placeRecordId().  
    //! NOTE: Does not redraw or update row2RecordId, etc. arrays.
    //!       Calling routine *MUST* do the updates, or things will break!
    //! By requiring caller to do updates (versus letting (un)hideRecordId do
    //! them, it prevents O(n^2) effort for some common cases.
    //! Returns TRUE if all children hidden/unhidden are displayed *strictly*
    //! after parent, FALSE otherwise.  (I.e., if TRUE, redraw only below 
    //! parent, if FALSE, redraw everything.)
    bool hideTreeChildren (RecordIdInfo *parentInfo, bool hide);

    //! Internal routine to register requirement for screen update.  
    //! Will delay update as long as overlapping updates are required 
    //! or until flushUpdate() is called.  Overlapping areas are
    //! combined appropriately.  An update request to a different 
    //! (non-overlapping) portion of the screen will flush the 
    //! current pending update.  
    //! 
    //! The goal of this routine is reduce the multiple updates to
    //! the same cell, when multiple attributes, etc. are being added.
    void requireUpdate(int x, int y, int w, int h);

    //! Internal routine to register requirement for a complete screen update.
    //! Will delay complete update until flushUpdate() is called.  
    //! Combines appropriately with calls to requireUpdate() 
    //! (basically everything will be updated next flushUpdate, 
    //! so combines will all requireUpdate calls).
    //! Very useful when resizing the grid, the screen, or calling
    //! placeRecordId().
    void requireCompleteUpdate();

    //! Internal routine to register requirement for a complete placement 
    //! array reconstruction (that is, the results of placeRecordId()).  
    //! Allows the expensive placement array reconstruction from multiple 
    //! placeRecordId() calls to be combined.
    void requireReconstruction() {reconstructionPending = TRUE;}

    //! Internal reutine to register the requirment for a syncGridSize().
    //! Allows placement array reconstruction to be delayed.
    void requireSync() {syncPending=TRUE;}

    //! Update visible geometries after resize events (and upon initialization)
    void updateGeometries();  

    //! Internal routine to layout cell with current constraints.
    //! Lays out everything in cell.  Called by paintCell() and 
    //! MouseEventHandler()
    //! and they need to use the same layout to get the correct results.
    //! If cellValue != NULL, string version of value will be returned.
    //! Use attrId to determine if indentation is needed.
    void layoutCell (CellLayout &layout, Cell *cell, int x, int y, 
		     int w, int h, QString *returnCellValue = NULL,
		     int recordId = -1, int attrId = -1);

    //! Paints everything in the cell at recordId, attrId.
    //! Handles a pixmap and/or a string at each cell.
    //! Also, allows a grid to be drawn.
    //!
    //! Should probably only be called by drawContents.
    void paintCell( QPainter &p, int recordId, int attrId, int x, int y, 
		    int w, int h, int clipx, int clipy);

    //! Used by sortTreeChildren() in heapsort's heapsort() routine
    static int compareUp( int, int, const void * );

    //! Used by sortTreeChildren() in heapsort's heapsort() routine
    static int compareDown( int, int, const void * );

    //! Used by sortTreeChildren() in heapsort's presort() routine
    static int isNull( int, const void * );

    //! Internal routine to update display of selected text
    void updateSelection (int x, int y, bool allowScroll);

    //! Internal routine that grabs the text in the selection area and 
    //! places it in selectedText. Clears selectedText if nothing is seleted.
    void updateSelectedText();


    IntArrayTable<Cell> cellTable; //! Lookup table containing grid of cells
    int numRecordIds;	//!< Num recordIds in grid, used in address calc, etc.
    int numAttrIds;	//!< Num attrIds in grid, used in address calc, etc.
    int numRowsVisible; //!< Num rows actually displayed, <= numRecordIds
    int rowMapAllocSize;//!< Actual alloced size of recordId2Row/rwo2RecordId 
    RecordIdInfo *recordIdInfo; //!< Info structure for each recordId
    int *row2RecordId; //!< Converts row to recordId ids (allows sorting)
    int *recordIdOrder; //!< Display order for recordIds (allows sorting)

    RecordIdInfo *treeFirstChild;  //!< First child of top level tree
    RecordIdInfo *treeLastChild;   //!< Last child of top level tree
    //! First placed child of top level tree (determines display order)
    RecordIdInfo *treeFirstPlacedChild; 
    RecordIdInfo *treeLastPlacedChild; //!< Last placed child of top level tree
    int treeIndentation;           //!< Pixels to indent each level of tree
    int treeAnnotSize;             //!< Pixels to reserve for the annotation

    QHeader *topHeader; //!< Manages attrId width and number
    bool topHeaderVisible; //!< TRUE if header is visible (default)
    QFont defaultCellFont;  //!< Default font for cells
    int recordIdFixedHeight;  //!< Manually manage recordId width and number
    bool gridOn;        //!< Draw grid if TRUE
    int sortAttrId;     //!< attrId of last sortAttrIdView request (-1, none)
    int sortDirection;  //!< Direction of last sortAttrIdView request

    QPixmap paintPixmap;//!< Used by paintCell to bitblt graphics
    CellLayout cellLayout;//!< Used by paintCell to place slots

    IntArrayTable<CellStyle> styleTable; //!< Maps CellStyleIds to styles
    CellStyleId	maxStyleId; //!< Max id assigned, used to assign new ids

    CellStyleId defaultStyleId;  //!< Id of default style for cell grid
    CellStyle *defaultStyle;     //!< Pointer to default style for quick access

    IntArrayTable<GridPixmap> pixmapIdTable; //!< Maps GridPixmapIds to pixmaps
    StringTable<GridPixmap> pixmapNameTable; //!< Maps names to pixmaps
    GridPixmapId maxPixmapId; //!< Max id assigned, used to assign new ids

    
    //! Caches update requests (requireUpdate(), flushUpdate(), 
    //! requireCompleteUpdate())
    bool updatePending;
    bool completeUpdatePending;
    
    //! Cache placement array reconstruction requests (requireReconstruction())
    bool reconstructionPending;

    //! Cache syncGridSize requests (requireSync())
    bool syncPending;

    int updateX1; //!< Caches update requests (requireUpdate(), flushUpdate())
    int updateX2; //!< Caches update requests (requireUpdate(), flushUpdate())
    int updateY1; //!< Caches update requests (requireUpdate(), flushUpdate())
    int updateY2; //!< Caches update requests (requireUpdate(), flushUpdate())

    //! Get clipboard for ease of use
    QClipboard *clipboard;
    
    bool areaSelected;  //!< Is an area of the grid selected?
    int startSelectX;   //!< Starting point of selection
    int startSelectY;   //!< Starting point of selection
    int currentSelectX;   //!< Current mouse position during selection
    int currentSelectY;   //!< Current mouse position during selection
    int selectedX1; //!< Area of grid selected 
    int selectedX2; //!< Area of grid selected 
    int selectedY1; //!< Area of grid selected 
    int selectedY2; //!< Area of grid selected 


    //! Amount to scroll on next tick due to selecting text off screen
    //! Normally both are 0, when no scrolling
    int scrollDX;
    int scrollDY;
    
    //! Scroll timer for selecting text
    QTimer *scrollTimer;

    MessageBuffer selectedText; //!< Text in selected area

    //! Default closed pixmap for row heirarchies
    GridPixmapId defaultClosedTreePixmap;
    //! Default opened pixmap for row heirarchies
    GridPixmapId defaultOpenedTreePixmap;

    //! The actual closed pixmap for row heirarchies
    QPixmap *closedTreePixmap;
    //! The actual opened pixmap for row heirarchies
    QPixmap *openedTreePixmap;

    //! The attrId to annotate with tree info (make -1 for no tree annot)
    int treeAttrId;

    //! Colors to paint tree hierachry lines with
    //! Making look 3d, so have light, main, dark versions of color
    QColor lightTreeLineColor;
    QColor mainTreeLineColor; //!< Colors to paint tree hierachry lines with
    QColor darkTreeLineColor; //!< Colors to paint tree hierachry lines with

    int watchRecordId;  //!< RecordId search engine watching (-1 not watched)
    int watchAttrId;    //!< AttrId search engine watching (-1 not watched)
    bool watchpointTriggered; //!< Flags when a search watchpoint is triggered

    int minRHeight;
};


#endif
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

