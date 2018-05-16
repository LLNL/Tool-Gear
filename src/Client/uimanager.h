//! \file uimanager.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.01                                              July 19, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** Widget UIManager, which manages the overall user interface content and
** allows snapshots of current state to be stored and loaded.
**
** This widget doesn't actually display any data, a TBD viewer widget is
** needed for this task.
**
** Designed by John May and John Gyllenhaal at LLNL
** Initial implementation by John Gyllenhaal 10/6/00
**
*****************************************************************************/

#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <qobject.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qfont.h>
#include <limits.h>

// For development and debugging, use error checking of MD code
// For real runs, undef MD_DEBUG_MACROS to enable inline/macro versions
//#define MD_DEBUG_MACROS
#include <md.h>

// For efficiency, use symbol tables to speed up lookup of items.
// These symbol tables are based on the md symbol table algorithms
#include <int_symbol.h>
#include <int_array_symbol.h>
#include <string_symbol.h>

// Migrate over to C++ wrappers for symbol tables, where possible.
// For efficiency, use symbol tables to speed up lookup of items.
// These symbol tables are based on the md symbol table algorithms
#include <inttable.h>
#include <intarraytable.h>
#include <stringtable.h>

// Include instrumentation point location and type enumerations
#include <tg_inst_point.h>

// Include ToolGear types, such as NULL_INT, NULL_DOUBLE, etc.
#include <tg_types.h>

// Include DataStats template definition 
#include <datastats.h>

// Include an object that collects all the source files we read
#include "filecollection.h"

// MessageBuffer for parsing messages into individual lines
#include "messagebuffer.h"

// For int to int mapping support
#include "inttoindex.h"

// Predefine class that are friends of UIManager;
class UIXMLParser; 


//! UI manager, manages overall user interface content and
//! allows snapshots of current state to be stored and loaded.
//! Inherits from QObject, so Qt's signal and slots mechanisms
//! can be used to attach to viewers, etc.
class UIManager : public QObject
{
    Q_OBJECT

public:
    //! Creates MD database to hold UI management structures in a form
    //! that can be easily written out and read in.  If snapshotName is
    //! not NULL, will read in data from previous writeSnapshot() into 
    //! manager.       
    //! 
    //! Cache pointers to key sections to improve performance of various query
    //! routines.
    UIManager (QApplication* app, const char *desc=NULL, 
	       const char *snapshotName=NULL);

    //! Deletes MD database and frees associated data
    ~UIManager ();

    //! Print out snapshot in human readable format with the given page width
    void printSnapshot (FILE *out, int pageWidth = 80);

    //! Write out snapshot in easy to parse, machine independent format
    void writeSnapshot (FILE *out);

    //! Adds contents of snapshotName (multiplied by integer 'multiplier').
    /*! To diff to snapshots, load one and then subtract a different one
    // by using multiplier of '-1'.  May be other useful multipliers.
    // Punts if snapshotName doesn't exist or file not compatible */
    void addSnapshot(const char *snapshotName, int multiplier);

    //! Adds info entry set to 'string' to the info table
    //! These strings currently are for informational purposes only
    void setInfoValue (const char *infoName, const char *string);

    //! Gets info entry string from the info table
    QString getInfoValue (const char *infoName);

    //! Inserts a (presumably valid) socket handle for getting data
    //! from the collector process.
    void setRemoteSocket( int sock )
    {	remoteSocket = sock; sourceCollection->setRemoteSocket( sock ); }

    //! Retrieves the socket handle in use.
    int getRemoteSocket() const { return remoteSocket; };

    //! Creates table for the named function so that entries may be
    //! made in it.  Also indicates where the function source is located.
    //! Returns index function inserted at.  
    int insertFunction (const char *funcName, const char *fileName, 
                        int startLine=-1, int endLine=-1);

    //! Inserts a character buffer containing the full text of
    //! a source file into the database.  This function is
    //! intended as a callback when the UIMananger requests a
    //! file over a socket connection.
    //! This routine modifies fileBuf (changes newlines to string terminators)!
    //! The size_estimate, if specified, is used to process large files
    //! more efficiently (so useful to specify, if known).
    void insertSourceFile(char * fileBuf, int id, int size_estimate=-1 )
    {	sourceCollection->insertSourceFile( fileBuf, id, size_estimate ); }


    //! Associates a full path name (or any other string) with an
    //! outstanding request for a source file.  This is normally
    //! used as a callback when the collector is reporting a the
    //! full name of a requested file.  The id is the integer
    //! assigned to this request when it was issued, and this function
    //! can only be called before insertSourceFile is called, since
    //! that function invalidates the id.
    void setFullPath( char * fullPath, int id )
    {	sourceCollection->setFullPath( fullPath, id); }

    enum functionState  {functionUnparsed=0, functionParseRequested=-1, 
			 functionParsed=1};

    //! Sets function parse state, initially functionUnparsed
    void functionSetState (const char *functionName, functionState state);

    //! Returns function parse state (#functionState enum)
    functionState functionGetState (const char *functionName);

    //! Returns the number of functions currently inserted. 
    int functionCount ();

    //! Returns index of funcName, NULL_INT if funcName not found.
    int functionIndex (const char *funcName);

    //! Returns funcName at index (0 - count-1), NULL_QSTRING if out of bounds.
    QString functionAt (int index);

    //! Returns fileName for funcName, NULL_QSTRING if funcName not found
    QString functionFileName (const char *funcName);
    
    //! Returns fileIndex of funcName, NULL_INT if funcName not found.
    //! Returns function's index from file's point of view, can be used
    //! in fileFunctionAt(fileName, index) call.
    int functionFileIndex (const char *funcName);

    //! Returns startLine for funcName, NULL_INT if funcName not found
    int functionStartLine (const char *funcName);

    //! Returns endLine for funcName, NULL_INT if funcName not found
    int functionEndLine (const char *funcName);

    //! Registers file name and returns index function inserted at.
    /*! Calling insertFunction() will insert the file if it doesn't
    // exist, so it is not strictly necessary to call this.  
    // Ignores duplicate calls but always returns function index. */
    int insertFile (const char *fileName);
    
    enum fileState  {fileUnparsed=0, fileParseRequested=-1, fileParsed=1};

    //! Sets file parse state (#fileState enum above), initially FileUnparsed
    void fileSetState (const char *fileName, fileState state);

    //! Returns file parse state (#fileState enum above)
    fileState fileGetState (const char *fileName);

    //! Returns the number of files currently inserted. 
    int fileCount ();

    //! Returns index of fileName, NULL_INT if fileName not found.
    int fileIndex (const char *fileName);

    //! Returns fileName at index (0 - count-1), NULL_QSTRING if out of bounds.
    QString fileAt (int index);

    //! Returns the number of functions inserted for this file
    int fileFunctionCount(const char *fileName);

    //! Returns funcName at index for this file, NULL_QSTRING if out of bounds.
    QString fileFunctionAt(const char *fileName, int index);

    //! Returns funcName at line in this file, NULL_QSTRING if none mapped
    QString fileFunctionAtLine(const char *fileName, int lineNo);


    //! Statistics that can be performed for an entry and dataAttr 
    //! over all the PTPairs where there is data present.  
    //!
    //! Used by entryDataStat(), functionDataStat(), fileDataStat(), and
    //! applicationDataStat().
    //! 
    //! Performance (all stats tuned for maximum performance using the
    //! stats caching inferencing package in datastats.h) is shown
    //! for each operation.
    enum EntryStat {
	EntryInvalid = 0,
	EntrySum,		//!< constant time, fast as getValue()
	EntrySumOfSquares,	//!< same as EntrySum
	EntryCount,		//!< same as EntrySum
	EntryMean,		//!< constant time, fast as getValue() + divide
	EntryStdDev,		//!< constant time, fast as getValue() +
				//!< divide and sqrt
	EntryMax,		//!< variable time, base case constant time,
				//!< worst case requires linear time rescan
				//!< of all PTPair data
	EntryMin		//!< same as EntryMax
    };

    //! Statistics that can be performed over a range (function, file, or
    //! application) of entry stats.
    //!
    //! Used by functionDataStat(), fileDataStat(), and  
    //! applicationDataStat().
    //! 
    //! Performance is shown below for each operation.
    //! (Any stats over EntrySum, EntryMean, EntryMin, and EntryMax
    //!              are tuned for maximum performance using the stats caching
    //!              and inferencing algorithm in datastats.h.  Any stats over 
    //!              EntryStdDev, EntrySumOfSquares, and EntryCount may be
    //!              recalculated each time it is requested (limited caching), 
    //!              so much more expensive.)
    enum AttrStat {
	AttrInvalid = 0,
	AttrSum,		//!< const time over EntrySum and EntryMean,
				//!< variable time over EntryMax and EntryMin,
				//!< linear time over EntrySumOfSquares,
				//!< EntryStdDev, and EntryCount
	AttrSumOfSquares,	//!< same as AttrSum
	AttrCount,		//!< same as AttrSum, however value independent
				//!< of EntryStat
	AttrMean,		//!< same as AttrSum + divide
	AttrStdDev,		//!< same as AttrSum + divide and sqrt
	AttrMax,		//!< variable time, best case acts like
				//!< AttrSum, worst case linear time scan over
				//!< entryStats required
	AttrMin			//!< same as AttrMax
    };

    //! Declares dataAttr that data may be written into.  
    /*! dataType must be MD_INT, MD_DOUBLE, or MD_STRING for now.
    // The dataAttrTag is used internally and the dataAttrText is the dataAttr
    // header shown in the GUI, and the toolTip is the dataAttr tip 
    // suggestedAttrStat is a hint to the GUI as to the AttrStat
    // to use for function/file/application stats.
    // suggestedEntryStat is a hint to the GUI as to the EntryStat
    // to use for entry stats
    */
    void declareDataAttr (const char *dataAttrTag, const char *dataAttrText, 
			  const char *toolTip, int dataType,
			  AttrStat suggestedAttrStat = UIManager::AttrMean,
			  EntryStat suggestedEntryStat = UIManager::EntryMean);

    //! Returns the number of dataAttrs currently inserted.
    int dataAttrCount();

    //! Returns index of dataAttrTag, NULL_INT if not found.
    int dataAttrIndex (const char *dataAttrTag);

    //! Returns dataAttrTag at index, NULL_QSTRING if out of bounds
    QString dataAttrAt (int index);

    //! Returns dataAttrText for dataAttrTag, NULL_QSTRING if not found
    QString dataAttrText (const char *dataAttrTag);

    //! Returns dataAttrToolTip for dataAttrTag, NULL_QSTRING if not found
    QString dataAttrToolTip (const char *dataAttrTag);

    //! Returns dataType for dataAttrTag, NULL_INT if dataAttrTag not found
    int dataAttrType (const char *dataAttrTag);

    //! Returns suggestedAttrStat for dataAttrTag, AttrInvalid if dataAttrTag
    //! not found
    UIManager::AttrStat dataAttrSuggestedAttrStat (const char *dataAttrTag);

    //! Returns suggestedEntryStat for dataAttrTag, EntryInvalid if dataAttrTag
    //! not found
    UIManager::EntryStat dataAttrSuggestedEntryStat (const char *dataAttrTag);

    //! Creates an entry in the function's table.  Many entries may
    //! be associated with the same line.  For non-DPCL tools, the
    //! entryKey can just be the string version of the line number.
    void insertEntry (const char *funcName, const char *entryKey, int line, 
		      const char *toolTip);

    //! More detailed insertEntry that has extra information relevant 
    //! to instrumentation points in the code and how to visualize them.
    //! See tg_inst_point.h for instrumentation point location and type values.
    //! callIndex denotes the expected function call order, starting at 1.
    void insertEntry (const char *funcName, const char *entryKey, int line,
		      TG_InstPtType type, TG_InstPtLocation location, 
		      const char *funcCalled, int callIndex, 
		      const char *toolTip);

    //! Returns the number of entries for funcName, NULL_INT if not found.
    int entryCount(const char *funcName);

    //! Returns the number of entries for funcName:line, NULL_INT if not found.
    int entryCount(const char *funcName, int line);

    //! Returns index of entryKey in funcName, NULL_INT if not found.
    int entryIndex (const char *funcName, const char *entryKey);

    //! Return lineIndex of entryKey at funcName:line, NULL_INT if not found
    int entryLineIndex (const char *funcName, const char *entryKey);

    //! Returns entryKey at index for funcName, NULL_QSTRING if out of bounds
    QString entryKeyAt (const char *funcName, int index);

    //! Returns entryKey at line and lineIndex, NULL_QSTRING if out of bounds
    QString entryKeyAt (const char *funcName, int line, int lineIndex);

    //! Returns line for entryKey in funcName, NULL_INT if out of bounds
    int entryLine (const char *funcName, const char *entryKey);

    //! Returns inst point type for entryKey, TG_IPT_invalid if out of bounds
    TG_InstPtType entryType (const char *funcName, const char *entryKey);

    //!Returns location for type TG_IPT_function_call, otherwise TG_IPL_invalid
    TG_InstPtLocation entryLocation (const char *funcName, 
				     const char *entryKey);

    //!Returns funcCalled for type TG_IPT_function_call, otherwise NULL_QSTRING
    QString entryFuncCalled (const char *funcName, const char *entryKey);

    //! Returns callIndex for type TG_IPT_function_call, otherwise NULL_INT
    int entryCallIndex (const char *funcName, const char *entryKey);

    //! Returns toolTip for entryKey in funcName, NULL_QSTRING if out of bounds
    QString entryToolTip (const char *funcName, const char *entryKey);

    //! Creates (if necessary) the task/thread pair and returns its index.
    /*! Optional since set_int(), etc. will also create task/thread pairs.
    // This is the only way to declare taskIds or threadIds. 
    // Use -1 for unknown taskIds or threadIds. */
    int insertPTPair (int taskId, int threadId);

    //! Returns number of taskId/threadId pairs inserted
    int PTPairCount();

    //! Returns index (>= 0) if taskId/threadId pair exists, NULL_INT otherwise
    int PTPairIndex (int taskId, int threadId);

    //! Returns task portion of PTPair at index 
    int PTPairTaskAt(int index);

    //! Returns thread portion of PTPair at index
    int PTPairThreadAt(int index);

    //! Returns number of taskIds inserted by insertPTPair().
    int taskCount();

    //! Returns index of taskId, NULL_INT if not found
    int taskIndex (int taskId);
    
    //! Returns taskId at index in task list, NULL_INT if index out of bounds
    int taskAt(int index);
    
    //! Returns number of threadIds paired with this task, NULL_INT if id bad
    int taskThreadCount(int taskId);
    
    //! Returns the threadId paired with this task at index
    //! NULL_INT if out of bounds
    int taskThreadAt(int taskId, int index);
    
    
    //! Returns number of threadIds inserted by insertPTPair().
    int threadCount();
    
    //! Returns index of threadId, NULL_INT if not found
    int threadIndex (int threadId);
    
    //! Returns threadId at index in thread list, NULL_INT if out of bounds
    int threadAt(int index);
    
    //! Returns number of taskIds paired with this thread
    int threadTaskCount(int threadId);

    //! Returns the taskId paired with this thread at index
    int threadTaskAt(int threadId, int index);


    //! Declares a pixmap of name pixmapName from XPM 3 definition, which
    //! must be a valid XPM 3 image (for now, only minimal checking done but
    //! invalid formats could cause memory read errors).  
    //! See http://koala.ilog.fr/lehors/xpm.html for the format details, 
    //! tools for converting from earlier versions, etc.
    void declarePixmap (const char *pixmapName, const char *xpm[]);

    //! Returns number of pixmaps currently declared
    int pixmapCount();

    //! Returns the index of pixmapName, NULL_INT if pixmapName not found
    int pixmapIndex (const char *pixmapName);

    //! Returns pixmapName at index, NULL_QSTRING if not found
    QString pixmapAt (int index);

    //! Returns the *xpm[] (**xpm) for pixmapName, or NULL if not found
    const char **pixmapXpm (const char *pixmapName);

    //! Returns the QPixmap constructed from the *xpm[] for pixmapName.
    //! Returns NULL QPixmap (see isNULL() call) if not found.
    //! Since QPixmap uses lazy copying, using these QPixmaps may
    //! save memory, since all windows can share data.
    const QPixmap &pixmapQPixmap (const char *pixmapName);


    //! Declares a type of action, and its initial state, that may be 
    //! performed on a function entry.  Has same arguments and
    //! effect of declareActionState() with two extra features:
    //! 1) It declares the actionTag, so must happen before declareActionState
    //! calls for this actionTag.  2) It defines the initialState
    //! for this actionTag.  
    void declareAction (const char *actionTag, 
			const char *initialStateTag,
			const char *toStatePixmapTag, 
			const char *toStateMenuText, 
			const char *toStateToolTip,
			const char *inStatePixmapTag, 
			const char *inStateToolTip,
			bool enableTransitionToStateByDefault);

    //! Returns number of action attrs currently declared
    int actionCount();

    //! Returns the index of actionTag, NULL_INT if actionTag not found
    int actionIndex(const char *actionTag);

    //! Returns actionTag at index, NULL_QSTRING if not found
    QString actionAt(int index);

    //! Returns the initialState for actionTag, NULL_QSTRING if not found
    QString actionInitialState(const char *actionTag);


    //! Declares and describes a state an action may be in and the pixmap,
    //! Menu text, and ToolTip text used to indicate when a user can 
    //! transition to this state, and the pixmap and toolTip to present
    //! to the user when in the state.  If 'enableTransitionToStateByDefault'
    //! is TRUE, all states (other than itself) will allow user transition
    //! to this state, by default (unless disableActionStateTransition is 
    //! called). 
    //! If pixmap tags are NULL or "", no pixmap will be displayed.
    //! Will punt if action not declared or if state already declared.
    //! Note: the tool may transition from any state to any state 
    //! it wants to, this only specifies what the user may do from the UI.
    void declareActionState (const char *actionTag, 
			     const char *stateTag,
			     const char *toStatePixmapTag, 
			     const char *toStateMenuText, 
			     const char *toStateToolTip,
			     const char *inStatePixmapTag, 
			     const char *inStateToolTip,
			     bool enableTransitionToStateByDefault);


    //! Returns number of states currently declared for actionTag,
    //! NULL_INT if not found.
    int actionStateCount(const char *actionTag);

    //! Returns the index of stateTag for actionTag, NULL_INT if not found
    int actionStateIndex(const char *actionTag, const char *stateTag);

    //! Returns stateTag for actionTag at index, NULL_QSTRING if not found
    QString actionStateAt (const char *actionTag, int index); 

    //! Returns pixmapTag for 'to state' transition, NULL_QSTRING if not found
    QString actionStateToPixmapTag(const char *actionTag, 
				   const char *stateTag);

    //! Returns menuText for 'to state' transition, NULL_QSTRING if not found
    QString actionStateToMenuText(const char *actionTag, const char *stateTag);

    //! Returns toolTip for 'to state' transition, NULL_QSTRING if not found
    QString actionStateToToolTip(const char *actionTag, const char *stateTag);

    //! Returns pixmapTag for when action 'in the state', 
    //! NULL_QSTRING if not found
    QString actionStateInPixmapTag(const char *actionTag, 
				   const char *stateTag);

    //! Returns toolTip for when action 'in the state', 
    //! NULL_QSTRING if not found
    QString actionStateInToolTip(const char *actionTag, const char *stateTag);


    //! Returns TRUE if all state transitions are enabled by default, FALSE
    //! if not.  Punts on invalid args.
    //! Note: the tool may transition from any state to any state 
    //! it wants to, this only specifies what the user may do from the UI.
    bool actionStateToTransitionDefault(const char *actionTag, 
					const char *stateTag);

    //! Enables user to transition from 'fromStateTag' to 'toStateTag' for
    //! the specified actionTag.
    //! Ignores call if transition already enabled, punts on invalid args.
    //! Note: the tool may transition from any state to any state 
    //! it wants to, this only specifies what the user may do from the UI.
    void enableActionStateTransition (const char *actionTag, 
				      const char *fromStateTag,
				      const char *toStateTag);


    //! Prevents user to transition from 'fromStateTag' to 'toStateTag' for
    //! the specifed actionTag.
    //! Ignores call if transition already disabled, punts on invalid args.
    //! Note: the tool may transition from any state to any state 
    //! it wants to, this only specifies what the user may do from the UI.
    void disableActionStateTransition (const char *actionTag, 
				       const char *fromStateTag,
				       const char *toStateTag);

    //! Returns number of states that may be transitioned to 
    //! from the state 'fromStateTag'.  Note, may return 0, user cannot
    //! change out of this state.  Returns NULL_INT on invalid args.
    int actionStateTransitionCount (const char *actionTag, 
				    const char *fromStateTag);

    //! Returns the state at 'index' that the user may transition to, from
    //! the state 'fromStateTag'.  Returns in the order the transitions
    //! were enabled, either thru the declareActionState(..., TRUE) or
    //! thru enableActionStateTransition.  This should give the tool writer
    //! some control of the order state transitions are presented to the user.
    //! Punts on invalid actionTag and fromStateTag.  Returns NULL_QSTRING
    //! for indexes out of bounds.
    QString actionStateTransitionAt (const char *actionTag, 
				     const char *fromStateTag,
				     int index);

    //! Returns TRUE, if user may transition from 'fromStateTag' to 
    //! 'toStateTag'.  Returns FALSE otherwise.  Punts on invalid args.
    bool isActionStateTransitionEnabled (const char *actionTag, 
					 const char *fromStateTag,
					 const char *toStateTag);

    //! Enables action (allows user to click on it which causes it to be
    //! activated/deactivated) on the entry for the specified function.
    //! Actions can be activated without enabling them and vice-versa.
    //! Actions must first be declared with declareAction().
    //! If action already enabled, this routine siliently does nothing.
    void enableAction (const char *funcName, const char *entryKey, 
		       const char *actionTag);

    //! Disables action (no longer allow user to click on it) on the entry 
    //! for the specified function.
    //! This doesn't deactivate activated actions.
    //! Disables action on the entry for the specified function.
    //! Actions must first be declared with declareAction().
    //! If action already disabled, this routine siliently does nothing.
    void disableAction (const char *funcName, const char *entryKey, 
			const char *actionTag);

    //! Returns TRUE, if the specified action is enabled for the entry.
    bool isActionEnabled (const char *funcName, const char *entryKey, 
			  const char *actionTag);

    //! Activates (triggers) action on the entry for the specified function.
    //! May activate actions that are not enabled!
    //! If action already activated, ignore call (no signal emitted).
    //! Overloaded versions activate action for just one task/thread or
    //! for all tasks/threads in an entry.
    void activateAction (const char *funcName, const char *entryKey, 
			 const char *actionTag, int taskId, int threadId);

    //! All tasks/threads versions:
    //! If action currently deactivated for only a subset of tasks/threads,
    //! will emit individual signals for those actually activated.
    //! If action currently deactivated for all tasks/threads, one summary
    //! signal will be emitted.  
    //! If action not currently deactivated for any task/thread, nothing is
    //! done and no signal is emitted.
    void activateAction (const char *funcName, const char *entryKey, 
			 const char *actionTag);

    //! Deactivates (untriggers) action on the entry for the specified 
    //! function.
    //! May deactivate actions that are not enabled!
    //! If action already deactivated, ignores call (doesn't emit signal).
    //! Overloaded versions deactivate action for just one task/thread or
    //! for all tasks/threads in an entry.
    void deactivateAction (const char *funcName, const char *entryKey, 
	   	           const char *actionTag, int taskId, int threadId);

    //! All task/threads versions:
    //! If action currently activated for only a subset of tasks/threads,
    //! will emit individual signals for those actually deactivated.
    //! If action currently activated for all tasks/threads, one summary
    //! signal will be emitted.  
    //! If action not currently activated for any task/thread, nothing is done
    //! and no signal is emitted.
    void deactivateAction (const char *funcName, const char *entryKey, 
			   const char *actionTag);

    //! Returns a non-zero value, if the specified action has been activated 
    //! for the entry.  This non-zero value specifies the order the actions
    //! have been activated.  This allows actions at the same action point
    //! to be represented in the correct order (are executed in the
    //! order activated).
    unsigned int isActionActivated (const char *funcName, 
				    const char *entryKey, 
				    const char *actionTag, int taskId, 
				    int threadId);

    //! Returns TRUE if value is set, FALSE otherwise.  Independent
    //! of datatype (may be called for INT, DOUBLE, etc.) value.
    bool isValueSet (const char *funcName, const char *entryKey, 
		     const char *dataAttrTag, 
		     int taskId, int threadId);

    //! Writes an int to the specified location
    //! Punts if funcName, entryKey, or dataAttrTag undefined or not int col
    void setInt (const char *funcName, const char *entryKey, 
		 const char *dataAttrTag, int taskId, int threadId, int value);

    //! Returns the new, incremented int value at the specified location. 
    //! Assumes unset values are 0.  Much faster than getInt() + setInt()!
    int addInt (const char *funcName, const char *entryKey, 
		const char *dataAttrTag, int taskId, int threadId, 
		int increment);

    //! Returns the int at the specified location, NULL_INT if not set
    //! See also the generic getValue() routine.
    //! Punts if funcName, entryKey, or dataAttrTag undefined or not int col
    int getInt (const char *funcName, const char *entryKey, 
		const char *dataAttrTag, int taskId, int threadId);

    //! Writes a double to the specified location
    //! Punts if funcName, entryKey, or dataAttrTag undefined or not double col
    void setDouble (const char *funcName, const char *entryKey, 
		    const char *dataAttrTag, 
		    int taskId, int threadId, double value);

    //! Returns the new, incremented double value at the specified location. 
    //! Assumes unset values are 0.  Much faster than getDouble()+setDouble()!
    double addDouble (const char *funcName, const char *entryKey, 
		      const char *dataAttrTag, int taskId, int threadId, 
		      double increment);

    //! Returns the double at the specified location, NULL_DOUBLE if not set
    //! See also the generic getValue() routine.
    //! Punts if funcName, entryKey, or dataAttrTag undefined or not double col
    double getDouble (const char *funcName, const char *entryKey, 
		      const char *dataAttrTag, int taskId, int threadId);

    //! Returns the data value (of type int or double) at the specificied 
    //! location as a double.  Returns NULL_DOUBLE if value not set.
    //! Generic version of getInt and getDouble that works for either
    //! data type (a double can represent all the values of a 32 bit int).
    double getValue (const char *funcName, const char *entryKey, 
		     const char *dataAttrTag, int taskId, int threadId);

    //! Returns the EntryStat requested for the specified entryKey and 
    //! dataAttrTag across all the PTPairs that have set values.  
    //! If taskId/threadId is NULL, these parameter are ignored.
    //! If taskId/threadId is not NULL, for entryMin and entryMax, the 
    //! taskId/threadId responsible will be returned (all other stats return
    //! NULL_INT).
    //! See enum #EntryStat for performance information.
    double entryDataStat (const char *funcName, const char *entryKey, 
			  const char *dataAttrTag, EntryStat entryStat, 
			  int *taskId = NULL, int *threadId = NULL);
    
    //! Returns the AttrStat performed on the EntryStat specified across
    //! the function's entries (that have data).
    //! If entryKey is not NULL, returns entry that set min/max for
    //! AttrMin and AttrMax (NULL_QSTRING otherwise).
    //! See enums #AttrStat and #EntryStat for performance information.
    double functionDataStat (const char *funcName, const char *dataAttrTag,
			     AttrStat functionStat, EntryStat ofEntryStat,
			     QString *entryKey = NULL);

    //! Returns the AttrStat performed on the EntryStat specified across
    //! the file's entries (that have data).
    //! If funcName and/or entryKey is not NULL, returns function/entry 
    //! that set min/max for AttrMin and AttrMax (NULL_QSTRING otherwise).
    //! See enums AttrStat and EntryStat for performance information.
    double fileDataStat (const char *fileName, const char *dataAttrTag,
			 AttrStat fileStat, EntryStat ofEntryStat,
			 QString *funcName = NULL, QString *entryKey = NULL);

    //! Returns the AttrStat performed on the EntryStat specified across
    //! the entire application (that have data).
    //! If funcName and/or entryKey is not NULL, returns function/entry 
    //! that set min/max for AttrMin and AttrMax (NULL_QSTRING otherwise).
    //! See enums #AttrStat and #EntryStat for performance information.
    double applicationDataStat ( const char *dataAttrTag, 
				 AttrStat applicationStat, 
				 EntryStat ofEntryStat,
				 QString *funcName = NULL, 
				 QString *entryKey = NULL);

    //! Clears the source cache so that any requests for file source or size
    //! will reload the file (usually used after changing search path)
    void clearSourceCache () 
    {	sourceCollection->clear(); }

    //! Returns the specified source line from the specified file.
    //! Returns NULL_QSTRING if file unloadable or lineNo out of bounds
    //! Use these routines for efficiency and for coherence between viewers
    QString getSourceLine (const char *fileName, int lineNo)
    {	return sourceCollection->getSourceLine( fileName, lineNo ); }

    //! Returns the number of lines in the source file
    //! returns 0 if file unloadable or empty
    int getSourceSize (const char *fileName)
    {	return sourceCollection->getSourceSize( fileName ); }

    //! Returns full path at which source file was found, if
    //! known, or the file name passed in if the source file has
    //! not been loaded.
    QString getSourcePath (const char * fileName)
    { 	return sourceCollection->getSourcePath( fileName ); }

    //! Returns double value for Tool Gear version
    double getToolGearVersion ()
    {
	return (TG_VERSION);
    }

    //! Puts new text at the beginning of the current About text,
    //! unless prepend is set to FALSE (then appends).
    //! Automatically puts newline between current and next About text.
    void addAboutText (const char * text, bool prepend = TRUE)
    {
	if (prepend)
	{
	    // Prepend newline if aboutText not empty
	    if (!aboutText.isEmpty())
	    {
		aboutText.prepend("\n");	
	    }
	    
	    // Then prepend about text
	    aboutText.prepend(text);
	}
	else
	{
	    // Append blank line if aboutText not empty
	    if (!aboutText.isEmpty())
	    {
		// If earlier aboutText doesn't end in newline, add one
		if (aboutText[aboutText.length()-1] != '\n')
		    aboutText.append("\n");
		
		// Append blank line
		aboutText.append("\n");	
	    }
	    
	    // Then append about text
	    aboutText.append(text);
	}
    }

    //! Returns the About text string
    QString getAboutText()
    {
	QString patchedAboutText (aboutText);
	QString TGAboutText;
	
	TGAboutText.sprintf (
	    "Tool Gear Infrastructure Version %3.2f\n"
	    "Designed and implemented by\n"
	    "John Gyllenhaal, John May, and Martin Schulz\n"
	    "Lawrence Livermore National Laboratory\n"
	    "www.llnl.gov/CASC/tool_gear",
	    getToolGearVersion());

	// If exiting about Text not empty, append blank line
	if (!patchedAboutText.isEmpty())
	{
	    // Get the last character in the about text
	    QChar lastChar = patchedAboutText[patchedAboutText.length()-1];

	    // Terminate last line if not already and add blank line
	    if (lastChar != '\n')
		patchedAboutText.append("\n\n");
	    else
		patchedAboutText.append("\n");
	}

	// Append Tool Gear about info to about Text
	patchedAboutText.append(TGAboutText);

	return patchedAboutText;
    }

    //! Set the window caption (title)
    void setWindowCaption(const char *caption)
    {
	setInfoValue ("caption", caption);
	emit windowCaptionSet(caption);
    }

    //! Get the window caption (title), returns NULL_QSTRING if not set
    QString getWindowCaption()
    {
	return (getInfoValue("caption"));
    }

 
    //! Returns the QApplication pointer
    QApplication * getApp()
    {
	    return a;
    }

    //! Returns the name of the Client program
    QString getProgramName();

    //! Return the font used for main text areas
    QFont getMainFont()
    {	return mainFont;	}

    //! Set the font to be used for main text areas.  Emits
    //! mainFontChanged signal when the font is changed.
    void setMainFont( QFont & f );

    //! Set the font to be used for labels.  Emits
    //! labelFontChanged signal when the font is changed.
    void setLabelFont( QFont & f );

    //! Specify the message folder display policy when empty
    enum PolicyIfEmpty {
	InvalidIfEmpty = 0,  // Not a valid choice, used as an error ret value
	ShowIfEmpty,
	DisableIfEmpty,
	HideIfEmpty
    };

    //! Creates a message folder referenced by messageFolderTag and
    //! displayed with messageFolderTitle.
    //! If message folder already exists, does nothing and returns -1.
    //! Otherwise, returns index of messageFolder (>= 0)
    int declareMessageFolder (const char *messageFolderTag, 
			      const char *messageFolderTitle,
			      PolicyIfEmpty ifEmpty = UIManager::ShowIfEmpty);

    //! Returns the number of message folders currently declared
    int messageFolderCount();

    //! Returns the messageFolderTag at index (0 - count-1), NULL_QSTRING if
    //! out of bounds.
    QString messageFolderAt (int index);

    //! Returns the index of messageFolderTag, NULL_INT if not found
    int messageFolderIndex (const char *messageFolderTag);

    //! Returns title of messageFolder, NULL_QSTRING if 
    //! messageFolderTag not found
    QString messageFolderTitle(const char *messageFolderTag);

    //! Returns ifEmpty policy of messageFolder.
    //! Returns InvalidIfEmpty if messageFolderTag not found
    UIManager::PolicyIfEmpty
    messageFolderIfEmpty(const char *messageFolderTag);

    //! Creates and adds a message to the messageFolder.  Messages are added 
    //! to an specific messageFolder (messageFolderTag) and consist of one or
    //! more lines of messageText and zero or more lines of messageTraceback
    //! information (several independent tracebacks can be specified and each
    //! traceback may be several stack level deep, if desired).
    //! Messages currently cannot be modified or deleted after being added. 
    //! Currently, automatically creates message tags (M0, M1, etc.) to
    //! allow same interface as other UIManager calls.  
    //!
    //! MessageText Format:
    //! Use newlines (\n) to get multiline messages (only first line shown
    //! unless expanded).  
    //!
    //! MessageTraceback Format:
    //! It may be NULL or "" to indicate no tracback.
    //! Each traceback consists of a title and then one or more location
    //! specifiers separated by newlines.  Here is the basic traceback
    //! format for one traceback and one level of traceback:
    //! "^Title\n<file_name:line_no1>function_name"
    //!
    //! If function_name or line_no is not known, it can be left out, 
    //! but the display may not do well initially if don't have both file 
    //! and line number.  Also, function_name may be used as a comment
    //! field (although it will be currently labeled function).
    //!
    //! Additional levels of traceback may be specified after the
    //! first level, separated by newlines(\n), like this:
    //! "^Title\n<file1.c:line_no1>func1\n<file2.c:line_no2>func2"
    //!
    //! Multiple different tracebacks can be specified by
    //! specifying another title (i.e.,Title2) and traceback after 
    //! the first one.
    //! That is:
    //! "^Title1\n<file1.c:line_no1>func1\n^Title2\n<file2.c:line_no2>func2"
    //! 
    //! In theory, any number of tracebacks may be specified 
    //! (up to three tested)
    //!
    //! It is important to not put spaces before '^' and '<'.  Unexpected
    //! parsing results may occur otherwise (now or in the future).
    //! 
    //! Return's index of message inserted.
    int addMessage (const char *messageFolderTag, const char *messageText,
		    const char *messageTraceback);


    //! Returns the number of messages currently in messageFolder
    int messageCount(const char *messageFolderTag);

    //! Returns the messageTag in messageFolder at index (0 - count-1), 
    //! NULL_QSTRING if out of bounds.
    QString messageAt (const char *messageFolderTag, int index);

    //! Returns the index of messageTag in messageFolder, NULL_INT if not found
    int messageIndex (const char *messageFolderTag, const char *messageTag);

    //! Returns text for message, NULL_QSTRING if messageTag not found
    QString messageText(const char *messageFolderTag, const char *messageTag);

    //! Returns the number of lines in the message
    int messageTextLineCount (const char *messageFolderTag, 
			      const char *messageTag);

    //! Returns actual ptr to a line of the message text
    //! Note: lineNo starts at 0!
    const char *messageTextLineRef (const char *messageFolderTag, 
				    const char *messageTag,
				    int lineNo);

    //! Returns traceback for the  message, 
    //! NULL_QSTRING if messageTag not found
    QString messageTraceback(const char *messageFolderTag, 
			     const char *messageTag);


    //! Registers a site priority modifier.  If non-NULL regular expressions
    //! are specified and they match the site file,desc, or line, then the 
    //! priorityModifier will be added to the site display priority.
    //! The highest site display priority will be displayed by default
    //! in the message traceback.  Negative priorityModifiers are allowed 
    //! and useful for filtering out known unuseful content.
    //! If all regular expressions are NULL, addSitePriority do nothing.
    int addSitePriority (double priorityModifier, 
			 const char *fileRegExp = NULL,
			 const char *descRegExp = NULL, 
			 const char *lineRegExp = NULL);


    //! Returns the number of sitePriority entries
    int sitePriorityCount();

    //! Returns the sitePriorityTag at index (0 - count-1), 
    //! NULL_QSTRING if out of bounds.
    QString sitePriorityAt (int index);

    //! Returns the index of sitePriorityTag, NULL_INT if not found
    int sitePriorityIndex (const char *sitePriorityTag);

    //! Returns modifier for sitePriority, NULL_DOUBLE if not found
    double sitePriorityModifier(const char *sitePriorityTag);

    //! Returns fileRegExp for sitePriority, NULL_QSTRING if not set
    QString sitePriorityFile(const char *sitePriorityTag);

    //! Returns descRegExp for sitePriority, NULL_QSTRING if not set
    QString sitePriorityDesc(const char *sitePriorityTag);

    //! Returns lineRegExp for sitePriority, NULL_QSTRING if not set
    QString sitePriorityLine(const char *sitePriorityTag);


    //! Processes the XMLSnippet (first wrapping it with <tool_gear_snippet>
    //! ... </tool_gear_snippet> to make the XML parser happy with multiple
    //! snippets) and process the recognized commands (warning about those
    //! not recognized.   Initially XML can be used to execute
    //! declareMessageFolder(), addMessage(), and addAboutText() commands.
    //! lineNoOffset is used in error messages to relate line in snippet to
    //! line in source file (defaults to 0).
    void processXMLSnippet (const char *XMLSnippet, int lineNoOffset = 0);


    enum ColumnAlign {
	AlignInvalid = 0,
	AlignAuto,		//!< Right align numbers, left everything else
	AlignLeft,	        //!< Align column contents on the left
	AlignRight		//!< Align column contents on the right
    };

    
    //! Declare a data column for sites (typically source files) referenced 
    //! by message annotations.  
    //! Must provide siteColumnTag, title, and toolTip for the column, 
    //! the rest have semi-reasonable defaults.   
    //! Position specifies the location of the column relative to the source 
    //! file (negative positions are before the source, positive after, ties
    //! broken by declaration order). 
    //! The Column's siteMask is ANDed with the message annnotation's site mask
    //! and if the value is non-zero, the column is shown.
    //! If hideIfEmpty is TRUE, the column will be hidden if would be empty 
    //! for the file being displayed, otherwise it is always shown (default).
    //! The current valid values for align (the ColumnAlign enum) is AlignAuto,
    //! AlignRight, and AlignLeft.
    //! The minWidth (default 0), provides a minimum width hint (in 
    //! characters) to the GUI (there may be other minimum width constraints).
    //! The maxWidth (default 1000), provides a minimum width hint (in
    //! characters) to the GUI (there may be other maximum width constraints).
    //! Returns index of the new site column (-1 if already exists)
    int declareSiteColumn (const char *siteColumnTag, 
			   const char *title, 
			   const char *toolTip,
			   int position = 1, 
			   unsigned int siteMask = 0xFFFFFFFF,
			   bool hideIfEmpty = FALSE,
			   ColumnAlign align = UIManager::AlignAuto,
			   int minWidth = 0,
			   int maxWidth = 1000);
    
signals:
    //! Called just after new file inserted (by insertFile()
    //! or insertFunction())
    void fileInserted (const char *fileName);

    //! Called just before insertFunction() returns
    void functionInserted (const char *funcName, const char *fileName, 
			   int startLine, int endLine);

    //! Called just before declareDataAttr() returns
    void dataAttrDeclared (const char *dataAttrTag, const char *dataAttrText, 
			   const char *toolTip, int dataType);

    //! Called just before insertEntry() returns
    void entryInserted (const char *funcName, const char *entryKey, int line, 
			TG_InstPtType type, TG_InstPtLocation location, 
			const char *funcCalled, int callIndex, 
			const char *toolTip);

    //! Called just before insertPTPair() returns (on state changes only)
    void PTPairInserted (int taskId, int threadId);


    //! Called just before declareAction() returns
    void actionDeclared (const char *actionTag, 
			 const char *initialStateTag,
			 const char *toStatePixmapTag, 
			 const char *toStateMenuText, 
			 const char *toStateToolTip, 
			 const char *inStatePixmapTag, 
			 const char *inStateToolTip,
			 bool enableTransitionToStateByDefault);

    //! Called just before declareActionState() returns
    void actionStateDeclared (const char *actionTag, 
			      const char *stateTag,
			      const char *toStatePixmapTag, 
			      const char *toStateMenuText, 
			      const char *toStateToolTip, 
			      const char *inStatePixmapTag, 
			      const char *inStateToolTip,
			      bool enableTransitionToStateByDefault);

    //! Called just before enableActionStateTransision() 
    //! (on state changes only)
    void actionStateTransitionEnabled (const char *actionTag, 
				       const char *fromStateTag,
				       const char *toStateTag);

    //! Called just before disableActionStateTransision()
    //! (on state changes only)
    void actionStateTransitionDisabled (const char *actionTag, 
					const char *fromStateTag, 
					const char *toStateTag);

    //! Called just before enableAction() returns (on state changes only)
    void actionEnabled (const char *funcName, const char *entryKey, 
			const char *actionTag);

    //! Called just before disableAction() returns (on state changes only)
    void actionDisabled (const char *funcName, const char *entryKey, 
			 const char *actionTag);

    //! Called just before activateAction() returns (on every call)
    void actionActivated (const char *funcName, const char *entryKey, 
			  const char *actionTag, int taskId, int threadId);

    //! Called just before activateAction() returns (on every call)
    void actionActivated (const char *funcName, const char *entryKey, 
			  const char *actionTag);

    //! Called just before deactivateAction() returns (on every call)
    void actionDeactivated (const char *funcName, const char *entryKey, 
			    const char *actionTag, int taskId, int threadId);

    //! Called just before deactivateAction() returns (on every call)
    void actionDeactivated (const char *funcName, const char *entryKey, 
			    const char *actionTag);

    //! Called just before setInt() (or addInt()) returns (on every call)
    void intSet(const char *funcName, const char *entryKey, 
		const char *dataAttrTag, int taskId, int threadId, int value);

    //! Called just before setDouble() (or addDouble()) returns (on every call)
    void doubleSet (const char *funcName, const char *entryKey, 
		    const char *dataAttrTag, int taskId, int threadId, 
		    double value);

    //! Called after changing state with fileSetState()
    void fileStateChanged (const char *fileName, UIManager::fileState state);

    //! Called after changing state with functionSetState()
    void functionStateChanged (const char *funcName, UIManager::functionState state);

    //! Called after adding pixmap with declarePixmap(), had to use **xpm
    //! instead of const char *xpm[], since Qt could not parse this correctly.
    void pixmapDeclared (const char *pixmapName, const char **xpm);

    //! Called after new messageFolder is declared
    void messageFolderDeclared (const char *messageFolderTag, 
				const char *messageFolderTitle,
				UIManager::PolicyIfEmpty ifEmpty);

    //! Called after new message is added to MessageFolder
    void messageAdded (const char *messageFolderTag, const char *messageText,
		       const char *messageTraceback);

    //! Called after clearSourceCache is called
    void clearedSourceCache();

    //! Called when requested main font changes, so widgets that use this
    //! font can update themselves.
    void mainFontChanged( QFont& );

    //! Called when requested label font changes, so widgets that use this
    //! label font can update themselves.
    void labelFontChanged( QFont& );

    //! Called when new caption (window title) is specified by the tool
    void windowCaptionSet(const char *caption);

    //! Called when new tool status is specified by the tool
    void toolStatusSet (const char *status);

protected:

    //! Internal addSnapshot() helper routine to get first entry in a section
    //! Punts on any error (indicating addSnapshot() had error)
    MD_Entry *SSGetFirstEntry(MD *sd, const char *sectionName);

    //! Internal addSnapShot() helper routine to get field from entry's field.
    //! Punts on any error (indicating addSnapshot() had error),
    //! including if the value at the index is of the wrong type.
    MD_Field *SSGetField (MD_Entry *entry, const char *fieldName, int index,
			  int type);

    //! Internal addSnapShot() helper routine that returns 1 if the entry's
    //! field element exists, 0 otherwise. 
    //! Returns 0 on any errors (doesn't punt).
    bool SSFieldExists (MD_Entry *entry, const char *fieldName, int index); 

    //! Internal addSnapshot() helper routine to get int value from entry field
    //! Punts on any error (indicating addSnapshot() had error)
    int SSGetInt(MD_Entry *entry, const char *fieldName, int index);

    //! Internal addSnapshot() helper routine to get double value from field
    //! Punts on any error (indicating addSnapshot() had error)
    double SSGetDouble(MD_Entry *entry, const char *fieldName, int index);

    //! Internal addSnapshot() helper routine to get string value from field
    //! Punts on any error (indicating addSnapshot() had error)
    char *SSGetString(MD_Entry *entry, const char *fieldName, int index);



    //! Internal routine to create an "indexed" section.
    //! Creates the index field declaration and index map which is updated
    //! by newIndexedEntry() and used by 'QString xxxAt(int)' and 
    //! 'int xxxIndex(const char *name)'.
    MD_Section *newIndexedSection (const char *sectionName, 
				   MD_Field_Decl **indexDecl,
				   INT_Symbol_Table **indexMap);

    //! Internal routine to create an "indexed" entry for the 
    //! specified section.
    //! Stashes extra info into the indexDecl field and into indexMap in order
    //! to facilitate 'QString xxxAt(int)' and 
    //! 'int xxxIndex(const char *name)'.
    //! If pointer to index not NULL, returns new index in that variable
    MD_Entry *newIndexedEntry (MD_Section *section, const char *entryName, 
			       MD_Field_Decl *indexDecl,
			       INT_Symbol_Table *indexMap,
			       int *return_index=NULL);

    //! Internal routine to declare a dataAttr for a specific function
    void declareDataAttr (MD_Section *dataSection, MD_Entry *dataAttrEntry);

    //! Internal routine to declare a action Attr for a specific function
    void declareAction (MD_Section *actionSection, 
			      MD_Entry *actionEntry);

    //! Internal routine to create (if necessary) and return a data field
    MD_Field *getDataField (const char *callerDesc, const char *funcName,
			    const char *entryKey, const char *dataAttrTag,
			    int expectedType, bool create);

    // Predeclare various Stats structures so can create links to each other
    struct EntryStats;
    struct FuncStats;
    struct FileStats;
    struct AppStats;
    friend struct UIManager::EntryStats;
    friend struct UIManager::FuncStats;
    friend struct UIManager::FileStats;
    friend struct UIManager::AppStats;
    friend class ::UIXMLParser;


    //! Internal info structure allocated for each entry/data pair where
    //! at least one value has been set.  In addition to the DataStats
    //! functionality, it points to the specific field where the data
    //! is stored.  Uses 'double' precision for both double and ints!
    //! Found that 'int' type overflowed quickly and became useless.
    struct EntryStats : public DataStats <double>
    {
	MD_Field *field;
	int entryIndex;			//! Index for entry

	UIManager::FuncStats *funcStats;//! funcStats rolling up this entryStats
	EntryStats *nextEntryStats;	//! For funcStats' linked list 

	EntryStats (int _entryIndex, FuncStats *_funcStats, MD_Field *_field): 
	    field(_field), entryIndex(_entryIndex), funcStats(_funcStats),
	    nextEntryStats(NULL) {};
	~EntryStats () {}
    };

    //! Internal routine to create (if necessary) and return entry stats
    EntryStats *getEntryStats (const char *callerDesc, const char *funcName, 
			       const char *entryKey, const char *dataAttrTag,
			       int expectedType,  bool create);

    //! Internal routine to update the entry stats and then the rest of the 
    //! rollup stats (func, file, and app stats).  Do not call when 
    //! rescanning in entry data!
    void updateEntryStats (EntryStats *stats, int PTPairIndex,
			   double newEntryValue, bool entryUpdate, 
			   double origEntryValue);

    //! Internal helper routine to quickly rebuild the entry stats when
    //! the min(), minId(), max(), or maxId() routines say these stats
    //! cannot be calculated until the stats are rebuilt.
    void rebuildEntryStats (EntryStats *stats);

    //! Internal helper routine that returns the specified entryStat from 
    //! stats. Used by entryDataStat(), functionDataStat(), etc.
    double getEntryDataStat (EntryStats *stats, EntryStat entryStat,
			     int *taskId = NULL, int *threadId = NULL);


    //! Internal info structure allocated for each function/data pair where
    //! at least one value has been set.  Individual stats taken for
    //! the min, max, sum, and mean values of the entries under it.
    //! If an entries min/max becomes invalid, all the stats for maxStats 
    //! and minStats will be invalidated (and a rescan will be needed to 
    //! rebuild).  Less commonly used stats (in theory) are cached in
    //! miscStats (which will be allocated if needed) and rebuilt every
    //! time something changes (or the miscStatsType changes).
    //! For efficiency, contains linked list of EntryStats for rescans.
    struct FuncStats 
    {
	DataStats <double>    maxStats;  //! Updated with max values of entries
	DataStats <double>    meanStats; //! Updated with mean values of entries
	DataStats <double>    minStats;  //! Updated with min values of entries
	DataStats <double>    sumStats;  //! Updated with sum values of entries

	DataStats <double>    *miscStats; //! Caches other types of entry stats
	EntryStat	      miscStatsType; //! EntryStat currently cached

	int        funcIndex;           //! Index for function
	EntryStats *firstEntryStats;    //! Linked list of entries with data

	FileStats *fileStats;		//! fileStats rolling up this function
	FuncStats *nextFuncStats;	//! For FileStats' linked list 

	FuncStats (int _funcIndex, FileStats *_fileStats) : 
	     miscStats(NULL), miscStatsType(EntryInvalid),
	     funcIndex(_funcIndex), firstEntryStats(NULL),
	     fileStats(_fileStats) {};
	~FuncStats () { if (miscStats != NULL) delete miscStats; }

	//! Add entry stats to head of entryStats linked list
	void addEntryStats (EntryStats *entryStats)
	    {
		entryStats->nextEntryStats = firstEntryStats;
		firstEntryStats = entryStats;
	    }
    };

    //! Internal routine to get FuncStats structure for the specified function
    //! and data field pair.  If doesn't exist, and create TRUE, creates it.
    //! Otherwise returns NULL.  Error messsages use callerDesc as routine 
    //! name.  Punts if funcName or dataAttrTag not found.
    FuncStats *getFuncStats (const char *callerDesc, const char *funcName,
			     const char *dataAttrTag, bool create);


    //! Internal info structure allocated for each file/data pair where
    //! at least one value has been set.  Individual stats taken for
    //! the min, max, sum, and mean values of the entries under it.
    //! If an entries min/max becomes invalid, all the stats for maxStats 
    //! and minStats will be invalidated (and a rescan will be needed to 
    //! rebuild).  Less commonly used stats (in theory) are cached in
    //! miscStats (which will be allocated if needed) and rebuilt every
    //! time something changes (or the miscStatsType changes).
    //! Use DataStatsIdArray with id array of size 2, so can tag data with
    //! both the functionIndex and EntryIndex.  For min/max, uses 
    //! lowest functionIndex (ties broken with EntryIndex).
    //! For efficiency, contains linked list of FuncStats for rescans.
    struct FileStats 
    {
	//! Updated with max values of entries
	DataStatsIdArray <double,2> maxStats; 

	//! Updated with mean values of entries
	DataStatsIdArray <double,2> meanStats; 

	//! Updated with min values of entries
	DataStatsIdArray <double,2> minStats;  

	//! Updated with sum values of entries
	DataStatsIdArray <double,2> sumStats;  

	//! Caches other types of entry stats
	DataStatsIdArray <double,2> *miscStats; 

	//! EntryStat currently cached
	EntryStat	            miscStatsType; 

	//! appStats rolling up this fileStats
	AppStats *appStats;		

	//! Linked list of functions with data, used for rescanning entries
	FuncStats                   *firstFuncStats;      

	//! For AppStats' linked list 
	FileStats *nextFileStats;	

	FileStats (AppStats *_appStats) : 
	    miscStats(NULL), miscStatsType(EntryInvalid),
	    appStats(_appStats), firstFuncStats(NULL) {};
	~FileStats () { if (miscStats != NULL) delete miscStats; }

	//! Add func stats to head of funcStats linked list
	void addFuncStats (FuncStats *funcStats)
	    {
		funcStats->nextFuncStats = firstFuncStats;
		firstFuncStats = funcStats;
	    }
    };

    //! Internal routine to get FileStats structure for the specified file
    //! and data field pair.  If doesn't exist, and create TRUE, creates it.
    //! Otherwise returns NULL.  Error messsages use callerDesc as routine 
    //! name.  Punts if fileName or dataAttrTag not found.
    FileStats *getFileStats (const char *callerDesc, const char *fileName,
			     const char *dataAttrTag, bool create);


    //! Internal application info structure allocated for each dataAttr
    //! at least one value has been set.  Individual stats taken for
    //! the min, max, sum, and mean values of the entries under it.
    //! If an entries min/max becomes invalid, all the stats for maxStats 
    //! and minStats will be invalidated (and a rescan will be needed to 
    //! rebuild).  Less commonly used stats (in theory) are cached in
    //! miscStats (which will be allocated if needed) and rebuilt every
    //! time something changes (or the miscStatsType changes).
    //! Use DataStatsIdArray with id array of size 2, so can tag data with
    //! both the functionIndex and EntryIndex.  For min/max, uses 
    //! lowest functionIndex (ties broken with EntryIndex).
    //! For efficiency, contains linked list of FileStats for rescans.
    struct AppStats 
    {
	//! Updated with max values of entries
	DataStatsIdArray <double,2> maxStats; 

	//! Updated with mean values of entries
	DataStatsIdArray <double,2> meanStats; 

	//! Updated with min values of entries
	DataStatsIdArray <double,2> minStats;  

	//! Updated with sum values of entries
	DataStatsIdArray <double,2> sumStats;  

	//! Caches other types of entry stats
	DataStatsIdArray <double,2> *miscStats; 

	//! EntryStat currently cached
	EntryStat	            miscStatsType; 

	//! Linked list of files with data, used for rescanning entries
	FileStats                   *firstFileStats;      

	AppStats () : miscStats(NULL), miscStatsType(EntryInvalid), 
	    firstFileStats(NULL) {};
	~AppStats () { if (miscStats != NULL) delete miscStats; }

	//! Add file stats to head of fileStats linked list
	void addFileStats (FileStats *fileStats)
	    {
		fileStats->nextFileStats = firstFileStats;
		firstFileStats = fileStats;
	    }
    };

    //! Internal routine to get AppStats structure for the specified 
    //! data field.  If doesn't exist, and create TRUE, creates it.
    //! Otherwise returns NULL.  Error messsages use callerDesc as 
    //! routine name. Punts if  dataAttrTag not found.
    AppStats *getAppStats (const char *callerDesc, const char *dataAttrTag,
			   bool create);

    struct ActionInfo 
    {
	MD_Section *actionStateSection;
        MD_Field_Decl *indexDecl;
        INT_Symbol_Table *indexMap;
	MD_Entry *actionEntry;
	MD_Field_Decl *toStatePixmapDecl;
	MD_Field_Decl *toStateMenuTextDecl;
	MD_Field_Decl *toStateToolTipDecl;
	MD_Field_Decl *inStatePixmapDecl;
	MD_Field_Decl *inStateToolTipDecl;
	MD_Field_Decl *connectNewStatesToDecl;
	MD_Field_Decl *canTransitionToDecl;
	ActionInfo (MD_Section *_actionStateSection, 
		    MD_Field_Decl *_indexDecl,
		    INT_Symbol_Table *_indexMap,
		    MD_Entry *_actionEntry,
		    MD_Field_Decl *_toStatePixmapDecl,
		    MD_Field_Decl *_toStateMenuTextDecl,
		    MD_Field_Decl *_toStateToolTipDecl,
		    MD_Field_Decl *_inStatePixmapDecl,
		    MD_Field_Decl *_inStateToolTipDecl,
		    MD_Field_Decl *_connectNewStatesToDecl,
		    MD_Field_Decl *_canTransitionToDecl) :
	    
	    actionStateSection(_actionStateSection),
	    indexDecl(_indexDecl),
	    indexMap(_indexMap),
	    actionEntry(_actionEntry),
	    toStatePixmapDecl(_toStatePixmapDecl),
	    toStateMenuTextDecl(_toStateMenuTextDecl),
	    toStateToolTipDecl(_toStateToolTipDecl),
	    inStatePixmapDecl(_inStatePixmapDecl),
	    inStateToolTipDecl(_inStateToolTipDecl),
	    connectNewStatesToDecl(_connectNewStatesToDecl),
	    canTransitionToDecl(_canTransitionToDecl)
	    {}
	~ActionInfo (){}
    };


    //! Internal routine to create (if necessary) and return an action field
    MD_Field *getActionField (const char *callerDesc, const char *funcName,
			      const char *entryKey, const char *dataAttrTag,
			      int expectedType, bool create);

    //! Internal routine to create (if necessary) and return the 
    //! specified field
    //! Used by getDataField() and getActionField() to do most of the work
    MD_Field *getField (const char *callerDesc, const char *funcName, 
			MD_Section *section, const char *entryKey, 
			const char *dataAttrTag, int expectedType, 
			bool create);

    //! Internal routine to get int from entry.  Returns NULL_INT on error
    int getFieldInt (MD_Section *section, const char *entryName, 
		     MD_Field_Decl *fieldDecl, int index);

    //! Internal routine to get double from entry.
    //! Returns NULL_DOUBLE on error
    double getFieldDouble (MD_Section *section, const char *entryName, 
			   MD_Field_Decl *fieldDecl, int index);

    //! Internal routine to get QString from entry.  NULL_QSTRING on error
    QString getFieldQString (MD_Section *section, const char *entryName, 
			     MD_Field_Decl *fieldDecl, int index);

#if 0
    //! Internal structure to cache file source.  Cache source in UIManager
    //! for efficiency and coherence.
    //
    //! It should be efficient since the file will be loaded only once and with
    //! the QString shallow copies, it may actually only be in memory once.  
    //
    //! It will hopefully enhance coherence, it that we would like multiple
    //! viewers to show the same source version for each file (which requires
    //! the same algorithm to find each file, or a central manager).  
    struct FileInfo 
    {
	//! Name of the file represented
	QString fileName;

	//! Set to true if file source cannot be loaded
	bool unloadable;

	//! Max line number in file (first line is line 1)
	int maxLineNo;

	//! Store lines in table, look up with line number
	IntTable<QString> lineTable;


        FileInfo ( const char * name) :
	    // Store the name of the file
	    fileName( name ),
	    // Create line table that frees QStrings on deletion
	    lineTable ("Line", TRUE, 0)
                 { maxLineNo = 0; unloadable = TRUE;}
        ~FileInfo() {}
    };

    //! Internal routine that creates FileInfo structure for fileName.
    //! Will punt (in fileInfoTable routines) if file created more than once!
    FileInfo *createFileInfo (const char *fileName);
#endif

    struct PixmapInfo
    {
	// internal copy of xmp format of pixmap, will take care of deleting
	const char **xpm;

	// internal copy of QPixmap version of xpm. 
	const QPixmap qpixmap;

	PixmapInfo (const char **myxpm, const QPixmap &myQPixmap) :
	    xpm(myxpm), qpixmap(myQPixmap)  {}
	~PixmapInfo() {delete[]xpm;}
    };


    //! Internal routine to increment the line count for the given line and
    //! IntTable<int> *. Returns the new count for line (1 if first time seen).
    int incrementLineCount (IntTable<int> &lineTable, int line);


    //! Internal routine that returns the next non-zero action activation 
    //! order id to use.  Will return numbers between 1 and 1 billion.  Punts
    //! if increments past 1 billion.  I chose to punt early so that 
    //! the return value could be safely multiplied by 4 by the viewer and
    //! still work properly.  A new strategy is needed if hit 1 billion
    //! action activations.
    unsigned int nextActivationOrderId ();


    //! Internal helper routine that returns a pointer to the 
    //! first character and to the space or terminator after the 
    //! last character of a XPM token that starts *at* or just 
    //! after the initial value of startToken (if startToken points 
    //! at whitespace).
    //! Returns TRUE if token found or FALSE if string ends before 
    //! XPM token found. Modifies both startToken and endToken to 
    //! point at start/end of token.
    //! Set startToken = previously returned endToken to grab the 
    //! next XPM token in the string.
    bool findXPMToken (const char *&startToken, const char *&endToken);

    //! Internal routine that returns the size of the XPM array of strings, 
    //! based on my interpretation of the XPM 3 parsing rules.
    //! Returns NULL_INT if doesn't appear to be valid XPM array of strings.
    //! Pass in pixmapName for error messages use only.
    int XPMArraySize (const char *pixmapName, const char *xpm[]);

    //! Internal routine to convert XPM token to an integer.
    //! Emits UIManager::declarePixmap errors on format problems
    int XPMconvertToInt (const char *pixmapName, const char *fieldName,
			 const char *startToken, const char *endToken);

    //! Only warn about unknown XML elements once per UIManager
    //! Use int to store level mask, so warn for each level
    //! (each snippet parsed separately, so cannot put in XML parser)
    StringTable<int> unknownXMLTable;

    //! Used to map XML elements to ids.   Only want to populate mapping
    //! table once, so put this in UIManager.  However, since all
    //! the enums are in a subclass, we are forced to hold a void pointer
    //! for XML parser map.  We have to call a explicit delete routine
    //! on deletion of UIManager to free memory
    struct XML_Token {
	XML_Token(int t, unsigned int l) : token(t), levelMask(l) {}
	int token;
	unsigned int levelMask;
    };
    StringTable<XML_Token> xmlTokenTable;

private:
    //! Event loop with which this object is associated
    QApplication * a;

    //! Main md database, where all the savable data is stored
    MD *md;	

    // Cache all the high-level MD sections for quick usage.
    MD_Section *infoSection;
    MD_Section *fileNameSection;
    MD_Section *functionNameSection;
    MD_Section *taskIdSection;
    MD_Section *threadIdSection;
    MD_Section *PTPairSection;
    MD_Section *dataAttrSection;
    MD_Section *actionSection;
    MD_Section *pixmapSection;
    MD_Section *messageFolderSection;
    MD_Section *sitePrioritySection;

    // Cache all the high-level MD fields for quick usage
    MD_Field_Decl *infoValueDecl;
    MD_Field_Decl *fileFunctionListDecl;
    MD_Field_Decl *fileFunctionMapDecl;
    MD_Field_Decl *fileParseStateDecl;
    MD_Field_Decl *functionFileNameDecl;
    MD_Field_Decl *functionFileIndexDecl;
    MD_Field_Decl *functionStartLineDecl;
    MD_Field_Decl *functionEndLineDecl;
    MD_Field_Decl *functionParseStateDecl;
    MD_Field_Decl *dataAttrTextDecl;
    MD_Field_Decl *dataAttrToolTipDecl;
    MD_Field_Decl *dataAttrTypeDecl;
    MD_Field_Decl *dataAttrSuggestedAttrStatDecl;
    MD_Field_Decl *dataAttrSuggestedEntryStatDecl;
    MD_Field_Decl *PTPairTaskIdDecl;
    MD_Field_Decl *PTPairThreadIdDecl;
    MD_Field_Decl *taskThreadListDecl;
    MD_Field_Decl *threadTaskListDecl;
    MD_Field_Decl *actionInitialStateDecl;
    MD_Field_Decl *xpmArrayDecl;
    MD_Field_Decl *messageFolderTitleDecl;
    MD_Field_Decl *messageFolderIfEmptyDecl;
    MD_Field_Decl *sitePriorityFileDecl;
    MD_Field_Decl *sitePriorityLineDecl;
    MD_Field_Decl *sitePriorityDescDecl;
    MD_Field_Decl *sitePriorityModifierDecl;

    
    // Create index maps and index fields for all the cached MD sections
    // Initialized/updated by newIndexSection() and newIndexedEntry().
    // Used by 'QString xxxAt(int)' and 'int xxxIndex(char *name)'.
    INT_Symbol_Table *infoMap;
    MD_Field_Decl *infoIndexDecl;

    INT_Symbol_Table *fileNameMap;
    MD_Field_Decl *fileNameIndexDecl;

    INT_Symbol_Table *functionNameMap;
    MD_Field_Decl *functionNameIndexDecl;

    INT_Symbol_Table *taskIdMap;
    MD_Field_Decl *taskIdIndexDecl;

    INT_Symbol_Table *threadIdMap;
    MD_Field_Decl *threadIdIndexDecl;

    INT_Symbol_Table *PTPairMap;
    MD_Field_Decl *PTPairIndexDecl;

    INT_Symbol_Table *dataAttrMap;
    MD_Field_Decl *dataAttrIndexDecl;

    INT_Symbol_Table *actionMap;
    MD_Field_Decl *actionIndexDecl;

    INT_Symbol_Table *pixmapMap;
    MD_Field_Decl *pixmapIndexDecl;

    INT_Symbol_Table *messageFolderMap;
    MD_Field_Decl *messageFolderIndexDecl;

    INT_Symbol_Table *sitePriorityMap;
    MD_Field_Decl *sitePriorityIndexDecl;

    //! To facilitate mapping a pair of ints (taskId and threadId)
    //! to an index, create an extra symbol table that stores indexes
    INT_ARRAY_Symbol_Table *PTPairIndexMap;

    //! To facilitate mapping taskId to the corresponding task entry
    INT_Symbol_Table *taskEntryMap;

    //! To facilitate mapping indexes to taskIds
    IntToIndex indexTaskIdMap;

    //! To facilitate mapping threadId to the corresponding thread entry
    INT_Symbol_Table *threadEntryMap;

    //! To facilitate mapping indexes to threadIds
    IntToIndex indexThreadIdMap;

    //! Internal structure to cache MD info about each function's section,
    //! such as the section pointer and pointers to the field declarations
    //! for line and description
    struct FuncInfo;
    friend struct UIManager::FuncInfo;

    struct FuncInfo 
    {
        MD_Section *dataSection;
        MD_Section *actionSection;
        MD_Field_Decl *indexDecl;
        INT_Symbol_Table *indexMap;
	MD_Field_Decl *lineIndexDecl;
        MD_Field_Decl *lineDecl;
        MD_Field_Decl *typeDecl;
        MD_Field_Decl *locationDecl;
        MD_Field_Decl *funcCalledDecl;
        MD_Field_Decl *callIndexDecl;
        MD_Field_Decl *toolTipDecl;
        MD_Field_Decl *actionsDecl;
	IntTable<int> lineCountTable;
	IntArrayTable<QString> lineIndexMap;
	IntArrayTable<EntryStats> entryStatsTable;
        FuncInfo (MD_Section *dataSec, MD_Section *actionSec,
                  MD_Field_Decl *funcIndexDecl, 
                  INT_Symbol_Table *funcIndexMap,
		  MD_Field_Decl *lineIndex,
                  MD_Field_Decl *line, MD_Field_Decl *type, 
		  MD_Field_Decl *location, MD_Field_Decl *funcCalled, 
		  MD_Field_Decl *callIndex, MD_Field_Decl *toolTip,
                  MD_Field_Decl *actions) :
	    dataSection(dataSec), actionSection(actionSec), 
	    indexDecl(funcIndexDecl), indexMap(funcIndexMap),
	    lineIndexDecl(lineIndex), lineDecl(line), typeDecl(type),
	    locationDecl(location), funcCalledDecl(funcCalled),  
	    callIndexDecl(callIndex),  toolTipDecl(toolTip),  
	    actionsDecl(actions),

	    // Create line Count table, delete allocated ints on delete
	    lineCountTable("lineCount", DeleteData),

	    // Create line index map, don't free.  indexMap will free QStrings
	    lineIndexMap("lineIndexMap", NoDealloc),

	    // Create EntryStats table, delete all pointers on delete
	    entryStatsTable("EntryStats", DeleteData)

	    {}

        ~FuncInfo() {}
    };

    //! Socket connection to data collector process; used to get
    //! source file information that may reside on the remote system.
    int remoteSocket;

    //! Font to be used for drawing in main text areas
    QFont mainFont;

    //! Font to be used for drawing labels
    QFont labelFont;

#if 0
    //! Keeps track of whether we are in the middle of reading a
    //! source file over the socket connection.  If so, other
    //! activities should be disabled.
    enum sourceFileState {
	    notReadingFile, readingFile, readyToParse
    } sourceState; 

    //! Stores pointers to FileInfo objects that are waiting to be
    //! filled in.  Keys to this table are passed along with the
    //! file request and are used to look up the corresponding object
    //! when the FileInfo arrives.
    IntTable<FileInfo> pendingFileInfoTable;
    int nextPendingFileInfoKey;
#endif

    //! Create delete routine that can be called from the "C" routine
    //! STRING_delete_symbol_table.  Used when deleting functionSectionTable
    //! below
    static void deleteFuncInfo (FuncInfo *ptr);


    //! Cache FuncInfo structure for each function in a symbol table for 
    //! quick access.  In the MD database, a prefix (F_) will be 
    //! used to prevent name space collisions between functions
    //! and internal names which make using MD_find_section routines 
    //! more expensive/painful.
    STRING_Symbol_Table *functionSectionTable;


    //! Quick lookup of FuncStats structure, allocated for each
    //! functionIndex and dataAttrIndex pair where there is data.
    //! Indexed by functionIndex, dataAttrIndex.
    IntArrayTable<FuncStats> funcStatsTable;

    //! Quick lookup of FileStats structure, allocated for each
    //! fileIndex and dataAttrIndex pair where there is data.
    //! Indexed by fileIndex, dataAttrIndex.
    IntArrayTable<FileStats> fileStatsTable;

    //! Quick lookup of FileStats structure, allocated for each
    //! dataAttrIndex where there is data.
    //! Indexed by dataAttrIndex.
    IntTable<AppStats> appStatsTable;

#if 0
    //! Cache file source code, on demand.  Do in UIManager for efficiency 
    //! and coherence (would like muliple viewers to show the same source 
    //! version).
    StringTable<FileInfo> fileInfoTable;
#endif

    //! Share the collection of source files among all instances.
    static FileCollection * sourceCollection;

    //! Cache pixmap definitions for efficiency.
    StringTable<PixmapInfo> pixmapInfoTable;

    //! Cache Action definitions for efficiency.
    StringTable<ActionInfo> actionInfoTable;

    //! Cache null QPixmap definition
    QPixmap nullQPixmap;

    //! Max order Id used for determining action activate order
    unsigned int maxActivationOrderId;

    //! Text that will appear in the About box
    QString aboutText;

    //! Internal structure to cache MD info about each messageFolder's section,
    //! such as the section pointer and pointers to the field declarations
    //! for the messageText and messageTraceback
    struct MessageFolderInfo;
    friend struct UIManager::MessageFolderInfo;

    struct MessageFolderInfo 
    {
        MD_Section *messageSection;
        MD_Field_Decl *indexDecl;
        INT_Symbol_Table *indexMap;
	MD_Field_Decl *messageTextDecl;
        MD_Field_Decl *messageTracebackDecl;
        MessageFolderInfo (MD_Section *_messageSection,
		  MD_Field_Decl *_indexDecl, 
                  INT_Symbol_Table *_indexMap,
                  MD_Field_Decl *_messageTextDecl, 
 		  MD_Field_Decl *_messageTracebackDecl) :
	    messageSection(_messageSection),
	    indexDecl(_indexDecl),
	    indexMap(_indexMap),
	    messageTextDecl(_messageTextDecl),
	    messageTracebackDecl(_messageTracebackDecl)
	    {}

        ~MessageFolderInfo() {}
    };

    //! Cache message folder info for efficiency.
    StringTable<MessageFolderInfo> messageFolderInfoTable;

    //! MessageBuffer used for parsing message text and traceback locations
    //! into individual lines (automatically resizes to hold any length)
    MessageBuffer lineBuf;

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

