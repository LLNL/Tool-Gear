//! \file treeview.h
//!
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
#ifndef TREEVIEW_H
#define TREEVIEW_H

#include "cellgrid.h"
#include "cellgrid_searcher.h"
#include <qstring.h>
#include <qwidget.h>
#include "uimanager.h"
#include <qfont.h>
#include <qpopupmenu.h>
#include <qstatusbar.h>
#include <qtimer.h>
#include <qlabel.h>
#include "intset.h"
#include "int_array_symbol.h"

//! An adaptive tree-based data viewer and action interface

//! Builds a tree-based viewer window on top of a CellGrid window.
//! This viewer lets users look at a program hierarchically, starting
//! with just the program name.  They can then dive into the program
//! see source files, dive into source files to see functions, and
//! dive into functions to see source code.  Data from each level is
//! "rolled up" and summarized at the higher levels.
class TreeView : public QWidget
{
    Q_OBJECT

public:

    TreeView(const char *progName, UIManager *m,
	     CellGridSearcher * grid_searcher,
	     QWidget *parent=NULL, 
	     const char *name=NULL, // QStatusBar *statusBar=NULL,
	     bool viewingSnapshot = FALSE);

    ~TreeView();

    //! Internal mapInfo types
    enum MapInfoType 
    {
	invalidType = 0,
	fileType,
	funcType,
	entryType,
	lineType
    };


    //! Compare results to 'compareTo'.  Undo comparison by setting
    //! compareTo to NULL
    void compareTo(UIManager *compareTo);

    //! Set program State message (QSTRING_NULL) for no message
    void setProgramState (QString stateMessage);

    //! Expands tree if necessary and center on fileName and lineNo
    //! Will highlight the line at the specified attrId
    void showFileLine (const char *fileName, int lineNo, int attrId = 0);

protected:

    //! Structure to track how data should be displayed/summarized
    struct DataInfo
    {
	UIManager::AttrStat columnStat;
	UIManager::EntryStat entryStat;

	DataInfo (UIManager::AttrStat columnStat_=UIManager::AttrMean,
		  UIManager::EntryStat entryStat_=UIManager::EntryMean):
	    columnStat(columnStat_), entryStat(entryStat_) {}
    };
    
    //! Internel helper routine that returns column stat name as const char *
    const char *columnStatName (UIManager::AttrStat columnStat);

    //! Internel helper routine that returns entry stat name as const char *
    const char *entryStatName (UIManager::EntryStat entryStat);

    //! Structure for tracking how recordId, fileId, funcId, all
    //! map to each other.  Used in recordIdTable, fileIdTable, and
    //! funcId table.
    struct MapInfo
    {
	MapInfo (MapInfoType myType, int myRecordId, int myFileId,
		 int myFuncId=NULL_INT, int myEntryId=NULL_INT,
		 int myLineNo=NULL_INT) :
	    type(myType),
	    recordId(myRecordId),
	    fileId(myFileId),
	    funcId(myFuncId),
	    entryId(myEntryId),
	    lineNo(myLineNo)
	    {
	    }
	~MapInfo() {}

    private:
	//! Only allow TreeView routines to access data
	friend class TreeView;
	MapInfoType type;
	int recordId;
	int fileId;
	int funcId;
	int entryId;
	int lineNo;
    };

    //! Structure for managing pixmapInfo for popup menus, etc.
    struct PixmapInfo
    {
	PixmapInfo (const QPixmap &pixmap, GridPixmapId id) :
	    pixmapId(id), iconSet (pixmap)
	    {
	    }
	~PixmapInfo() {}

    private:
	//! Only allow TreeView routines to access data
	friend class TreeView;

	GridPixmapId pixmapId;  // grid's id
	QIconSet iconSet;       // Icon set for popup menu for pixmap
    };
    
    void resizeEvent( QResizeEvent *);

    //! Returns pointer to "clean" name, advanced past the prefix delimiter
    //! If prefixDelimiter==0, returns entire name (no prefixes)
    const char *cleanName(const char *name);

    //! Returns pointer to a "stripped" name, where all the leading directories
    //! have been stripped off.  (Advanced past the last '/' or '\'.)
    const char *strippedName(const char *name);

    // Expands the grid and returns the new recordId created by expansion
    int createRecordId (void);


    //! Internal helper function to add menu items for the state transitions
    //! allowed from the current action state.
    //! Returns TRUE if items added and FALSE if no menu items added
    bool addStateTransToMenu (QPopupMenu *popupMenu, const QString &funcName, 
			      const QString &entryKey,
			      const QString &actionTag);

    //! Internal routine that moves popup menu just below cell clicked on
    //! aligned with index/slot passed
    void placeMenuBelowCell (QPopupMenu *popupMenu, int recordId, int attrId,
			     int index, int slot);


    //! Used as parameter to updateStatusMessage
    enum StatusType
    {
	mainMessage = 0,
	typeMessage = 1,
	lineMessage = 2
    };

    //! Used as parameter to updateStatusMessage
    enum StatusSource
    {
	dataDisplay = 0,
	usageHelp = 1,
	locationDisplay = 2
    };

    //! Internal routine to allow prioritized access to the limited 
    //! status bar space.  The status bar is not actually updated 
    //! until updateStatusMessage() is called.
    void setStatusMessage (TreeView::StatusType statusType, 
			   TreeView::StatusSource statusSource,
			   const QString &message);

    //! Internal routine to update the status Bar to reflect the messages
    //! set by setStatusMessage().  Updates only those messages that change
    //! and give priority to the statusSource with the lowest number
    void updateStatusMessage();

    //! Internal routine to update status bar to reflect current mouse position
    //! If handle == NULL_INT, assumes not over annotation
    //! If index == NULL_INT, assumes over tree hierarchy control
    void updateStatusBar (int recordId, int attrId,
			  ButtonState buttonState,
			  int index = NULL_INT, int slot = NULL_INT,
			  int handle = NULL_INT, int layer = NULL_INT);

    //! Internal routine to update stats display portion of status bar
    void updateStatusBarStats (int recordId, int attrId);

    //! Flushes all new File, Function, Entry additions to the UIManager
    //! to the screen.
    void flushFileFunctionEntryUpdates ();

    //! Flushes all data cell updates (including rollups) recorded by
    //! recordDataCellUpdate(). 
    void flushDataCellUpdates ();



private slots:

    //! Handle mouse over clickable cell annotation events
    void annotMovedOverHandler(int recordId, int attrId,
                               ButtonState buttonState,
                               int handle, int layer, int index, int slot,
                               int x, int y, int w, int h);
    
    //! Handle mouse over cell slot events (not over clickable annotations)
    void slotMovedOverHandler(int recordId, int attrId,
                              ButtonState buttonState,
                              int index, int slot,
                              int x, int y, int w, int h);

    //! Handles mouse over the tree hierachy control area events
    void treeMovedOverHandler (int recordId, int attrId, 
			       ButtonState buttonState,
			       int indentLevel, int parentRecordId, 
			       bool treeOpen,
			       int x, int y, int w, int h);

    //! Handles mouse over empty area events
    void emptyAreaMovedOverHandler (int recordId, int attrId,
				    ButtonState buttonState, int gridx, 
				    int gridy);

    // Handles clicks on data columns (or for now, data values)
    void dataClickHandler (int recordId, int attrId,
			   ButtonState buttonState,
			   int handle, int layer, int index, int slot,
			   int x, int y, int w, int h);

    //! Handles clicks on data popUp menus
    void dataMenuClickHandler(int id);


    //! Handles clicks on cell annotations
    void annotClickHandler(int recordId, int attrId, ButtonState buttonState,
                          int handle, int layer, int index, int slot,
                          int x, int y, int w, int h);

    //! Handles highlighting options on action popUp menus
    void actionMenuHighlightHandler(int);

    //! Handles signals that clears the actionMenu help
    void actionMenuClearStatus();


    //! Handles clicks on action popUp menus
    void actionMenuClickHandler(int);

    //! Handles clicks on collapse tree popUp menus
    void collapseTreeHandler(int treeRecordId);

    //! Handles clicks on the tree hierachy control area
    void treeClickHandler (int recordId, int attrId, ButtonState buttonState,
                           int indentLevel, int parentRecordId, bool treeOpen,
                           int x, int y, int w, int h);

    //! Handles clicks not on annotations
    void cellSlotClickHandler (int recordId, int attrId, 
			       ButtonState buttonState, 
			       int index, int slot, int x, int y, 
			       int w, int h);

    //! Handles clicks on empty area events
    void emptyAreaClickHandler (int recordId, int attrId,
				ButtonState buttonState, int gridx, 
				int gridy);

    //! Listens for file state changes (parsing, done parsing)
    void fileStateChanged (const char *fileName, UIManager::fileState state);

    //! Lists file name in tree, if not already there
    void listFileName(const char * fileName);


    //! Listens for function state changes (parsing, done parsing)
    void functionStateChanged (const char *funcName, 
			       UIManager::functionState state);

    //! Listens for functions being inserted, calls listFunctionName
    void functionInserted(const char *funcName, const char *fileName, 
			  int startLine, int endLine);

    //! Lists function name in tree, if not already there
    void listFunctionName(const char *funcName);

    //! Updates the rollup display for function and dataAttrTag
    void updateFunctionRollup (const char *funcName, const char *dataAttrTag);

    //! Updates the rollup display for file and dataAttrTag
    void updateFileRollup (const char *fileName, const char *dataAttrTag);

    //! Updates the application rollup display for dataAttrTag
    void updateAppRollup (const char *dataAttrTag);

    //! Adds source to display for this function, if possible
    void listFunctionSource (const char *funcName);

    //! Paints the contents of the source line with the current state
    void paintSourceLine (MapInfo *lineInfo);

    //! Calculate index into line that this entry should be placed
    //! to the left of.  That is, all slots should be negative.
    int calcEntryIndex (const QString &funcName, const QString &entryKey, 
			const QString &line, int lineNo);
    
    // Returns function called at callIndex on the given line
    // Punts if callIndex not found!  Used by calcEntryIndex.
    QString getFuncCalledAt (const QString &funcName,
			     int lineNo, int callIndex);


    //! Listens for entries being inserted, calls listEntry
    void entryInserted (const char *funcName, const char *entryKey, int line,
                        TG_InstPtType type, TG_InstPtLocation location,
                        const char *funcCalled, int callIndex, 
			const char *toolTip);

    //! Displays entry (if no source) and calculates menu point placements
    //! for actions on this entry
    void listEntry (int funcId, int entryId);


    //! Listens for state changes for actionTag.
    //! Handles enabled/disabled/activated/deactivated signals
    void actionStateChanged (const char *funcName, const char *entryKey,
			     const char *actionTag);

    //! Listens for action state changes for a particular task/thread.
    //! Mixed task/thread support not designed yet, so whine when called.
    //! Handles enabled/disabled/activated/deactivated signals
    void actionStateChanged (const char *funcName, const char *entryKey,
			     const char *actionTag,
			     int taskId, int threadId);

    
    //! Catch data changes and update cell grid
    void intSet(const char *funcName, const char *entryKey, 
		const char *dataAttrTag, int taskId, int threadId, int value);
    void doubleSet (const char *funcName, const char *entryKey, 
		    const char *dataAttrTag, 
		    int taskId, int threadId, double value);

    //! Records that a data cell needs updating.  The cell's value is
    //! not actually updated until flushDataCellUpdates() is called.
    //! This allows parallel updates to a cell to be combined and updated
    //! on a regular interval (to prevent swamping the redraw routines).
    void recordDataCellUpdate (const QString &funcName, 
			       const QString &entryKey,  
			       const QString &dataTag);


    //! Flushes all pending updates that are driven by the timer
    void updateTimerHandler();


    //! Update contents for specified data cell
    void updateDataCell (const QString &funcName, 
			 const QString &entryKey, 
			 const QString &dataTag);

    //! Removes all contents from dataCell (after comparison).  
    void resetDataCell (const QString &funcName, 
			const QString &changedEntryKey,  
			const QString &dataTag);

    //! Updates internal tables for each pixmap declared in the UIManager
    void pixmapDeclaredHandler (const char *pixmapName, const char **xpm);

    //! Catch changes to the search path and redisplay the source
    void reloadSource();

    //! Catch requests to change the cellgrid font
    void resetFont( QFont & f);


    
private:
    
    UIManager *um;
    UIManager *diffUm;
    CellGrid *grid;
    QStatusBar *status;

    // String discription of program state (may be NULL_QSTRING)
    QString programStateMessage;

    // Status bar labels (NULL if not info should be presented)
    QLabel *statusLineNo;
    QLabel *statusStatType;
    
    // What is the last status displayed, to minimize redraws
    int statusLastRecordId;
    int statusLastAttrId;
    int statusLastHandle;

    // StatusMessage queue for use by updateStatusMessage
    QString statusMessageQueue[3][3];

    // Current status messages for use by updateStatusMessage
    QString statusMessages[3];

    // Buffers used for composing status meetings
    QString statusBuf;
    QString statusTypeBuf;

    int lineAttrId;
    int treeAttrId;
    int sourceAttrId;
    int dataStartAttrId;

    QString programName;
    int programRecordId; // RecordId for program summary info

    GridPixmapId hourglassPixmap;
    int hourglassHandle;
    int dataMenuHandle;

    GridPixmapId actionMenuPixmap;

    QIconSet headerSpacer;

    // Action menu handling
    int actionMenuRecordId;
    int actionMenuAttrId;
    int actionMenuEntryId;
    int actionMenuFuncId;

    // Data menu handling
    int dataMenuDataIndex;

    // Common/generic popup menu for use by treeview
    QPopupMenu commonPopupMenu;

    //Specific actionMenu so can attach listener to give usage info
    QPopupMenu actionPopupMenu;

    // Cache some colors used in treeView
    QColor blueColor;
    QColor redColor;

    // Styles used in treeView
    CellStyleId programStyle;
    CellStyleId fileStyle;
    CellStyleId funcStyle;
    CellStyleId diffedStyle;

    //! The prefix delimiter is used to separate internal tags from external
    //! names.  (I.e., ' ' is the delimiter for "0_2_1 main").
    //! For simplicity, assume same delimiter for files and functions.
    //! Evenually this will be specified in the UIManager
    //! but will default to ' ' for now.
    char prefixDelimiter;


    //! Tables to correlate recordId, fileId, funcId, entryId's, and
    //! lineNumbers together
    IntTable<MapInfo> recordIdTable;
    IntTable<MapInfo> fileIdTable;
    IntTable<MapInfo> funcIdTable; 
    IntArrayTable<MapInfo> entryIdTable;
    IntArrayTable<MapInfo> lineNoTable;

    //! Maps dataAttrIndex to DataInfo structure
    IntTable<DataInfo> dataInfoTable;

    //! Records pending dataCell updates (combine for scalability)
    INT_ARRAY_Symbol_Table *cellUpdateTable;

    //! Records pending function rollup updates (combine for scalability)
    INT_ARRAY_Symbol_Table *funcUpdateTable;

    //! Records pending file rollup updates (combine for scalability)
    INT_ARRAY_Symbol_Table *fileUpdateTable;

    //! Records pending app rollup updates (combine for scalability)
    INT_ARRAY_Symbol_Table *appUpdateTable;

    //! Records pending entry insertions (combine for scalability)
    INT_ARRAY_Symbol_Table *entryInsertTable;

    //! IntSet used to track which tree's (indexed by recordId) have been 
    //! populated/expanded (user has clicked on them to open them).  
    //! Expensive (and defeats the CellGrid tree design) to repopulate each 
    //! tree every time the tree is opened. 
    IntSet expandedRecordId;

    //! Keep track of tool declared pixmaps (thru UIManager) so that
    //! treeview can efficiently access and use them.  Allow lookup of 
    //! PixmapInfo by pixmapName.
    StringTable<PixmapInfo> pixmapInfoTable;

    //! Update screen timer (combine screen updates for scalability)
    QTimer *updateTimer;

    //! True if viewing a snapshot, not a live program
    bool snapshotView;

    int lastFileIndexDisplayed; //!< Last file index displayed from UIManager
    int lastFuncIndexDisplayed; //!< Last func index displayed from UIManager
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

