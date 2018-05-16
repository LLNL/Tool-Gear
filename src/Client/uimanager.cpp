// uimanager.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
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

#include "tg_socket.h"
#include "tg_pack.h"
#include "command_tags.h"
#include <uimanager.h>
#include <stdio.h>
#include "tg_error.h"
#include <qapplication.h>
#include <qstring.h>
#include <qsettings.h>
#include <math.h>
#include <string.h>
#include "tg_time.h"
#include <qxml.h>
#include <ctype.h>

// Should put this in a global place
const QString APP_KEY = "/Tool Gear/";

// Define error value for double DataStats class to NULL_DOUBLE.
// Used by getMax(), getMean(), etc.
template <> // Required by xlC
const double DataStats<double>::errorValue = NULL_DOUBLE;

// Also initialize IdArray version.
template <> // Required by xlC
const double DataStatsIdArray<double,2>::errorValue = NULL_DOUBLE;

// Shared source file collection will be initilialize on first use
// (in a call to read a source line or file length; remoteSocket
// must be correctly set by this time).
FileCollection * UIManager::sourceCollection = NULL;

/* Creates MD database to hold UI management structures in a form
// that can be easily written out and read in.  If snapshotName is
// not NULL, will read in data from previous writeSnapshot() into 
// manager.
//
// Cache pointers to key sections to improve performance of various query
// routines. */
UIManager::UIManager(QApplication* app, const char *desc, 
		     const char *snapshotName):

    // Create unknownXML table that frees levelmask on delete
    unknownXMLTable("unknownXML", DeleteData, 0),

    // Create xmlToken table that frees XML_Token structures on delete
    xmlTokenTable("xmlToken", DeleteData, 0),

    a(app),   

    // To facilitate mapping indexes to taskIds, NULL_INT indicates not found
    indexTaskIdMap("indexTaskId", NULL_INT, 0),

    // To facilitate mapping indexes to threadIds, NULL_INT indicates not found
    indexThreadIdMap("indexTreadId", NULL_INT, 0),

    remoteSocket(-1), 


#if 0
    // Initially not currently reading a file
    sourceState(notReadingFile),

    // Create pendingFileInfo table that does not free the struct on deletion
    // (there shouldn't be any items in the table at deletion anyway)
    pendingFileInfoTable("pendingFileInfo", NoDealloc, 0),
#endif

    // Create funcStats table that deletes funcStats structs on deletion
    funcStatsTable("funcStats", DeleteData, 0),

#if 0
    // Create fileInfo table that deletes FileInfo structs on deletion
    fileInfoTable("fileInfo", DeleteData, 0),
#endif

    // Create fileStats table that deletess fileStats structs on deletion
    fileStatsTable("fileStats", DeleteData, 0),

    // Create appStats table that deletes appStats structs on deletion
    appStatsTable("appStats", DeleteData, 0),

    // Create pixmapInfo table that deletes PixmapInfo structs on deletion
    pixmapInfoTable("pixmapInfo", DeleteData, 0),

    // Create ActionInfo table that deletes ActionInfo struct on deletion
    actionInfoTable("actionInfo", DeleteData, 0),
    
    // Cache null QPixmap definition
    nullQPixmap(),

    // Initially, no actions activated
    maxActivationOrderId(0),

    // Create MessageFolderInfo table that deletss MessageFolderInfo 
    // struct on delete
    messageFolderInfoTable("messageFolderInfo", DeleteData, 0)


{
    // Set object name to aid in debugging connection issues
    setName ("UIManager");


    // Warn when running MD in debug mode, don't want to pay for
    // checking cost normally
#ifdef MD_DEBUG_MACROS
    fprintf (stderr,
	     "Warning: Assertions on (MD_DEBUG_MACROS set in uimanager.h)!\n");
#endif

    // Create new empty database to store UI contents
    // Use smallest database size as starting point, everything
    // dynamically resizes to maintain appropriate performance
    //
    // Note: All MD_ routines punt(exit with message) if they run out of
    //       memory!  No need to check return values from 'new' calls.
    md = MD_new_md ("UIManager snapshot", 0);

    //
    // Create "indexed" sections to hold lists of known files, functions,
    // processes, threads, process/thread combos, etc.
    // Hold on to section pointers to eliminate need for future searches.
    //


    // Holds generic info, such as description, date created, version info
    infoSection = newIndexedSection ("_info_", &infoIndexDecl, &infoMap);

    // Make a STRING value field to hold generic string values
    infoValueDecl = MD_new_field_decl (infoSection, "value", 
				       MD_REQUIRED_FIELD);
    MD_require_string (infoValueDecl, 0);


    // Create info field for the passed in description
    // Must be done after initializing infoMap!
    setInfoValue ("description", desc);


    // Lists all file names seen.  Holds additional info such as
    // functions in each file, path to file, etc.
    fileNameSection = newIndexedSection ("_fileName_", &fileNameIndexDecl, 
					 &fileNameMap);

    // Make a STRING* "functionList" field to hold the functions found
    // in this file
    fileFunctionListDecl = MD_new_field_decl (fileNameSection, "functionList",
					      MD_REQUIRED_FIELD);
    MD_require_string (fileFunctionListDecl, 0);
    MD_kleene_star_requirement (fileFunctionListDecl, 0);

    // Make a INT* "functionMap" field to hold the (global) function indexes
    // associated with each line in the file (field indexed by lineNo)
    fileFunctionMapDecl = MD_new_field_decl (fileNameSection, "functionMap",
					      MD_REQUIRED_FIELD);
    MD_require_int (fileFunctionMapDecl, 0);
    MD_kleene_star_requirement (fileFunctionMapDecl, 0);

    
    // Make a INT "parseState" field to hold indicate parse state
    fileParseStateDecl = MD_new_field_decl (fileNameSection, "parseState",
					    MD_REQUIRED_FIELD);
    MD_require_int (fileParseStateDecl, 0);



    // Lists all function names seen.  Holds additional info such as
    // file function is in, line function starts/ends on, etc.
    functionNameSection = newIndexedSection ("_functionName_", 
					     &functionNameIndexDecl, 
					     &functionNameMap);

    // Make a STRING "fileName" to hold the file this function in
    functionFileNameDecl = MD_new_field_decl (functionNameSection, "fileName",
					      MD_REQUIRED_FIELD);
    MD_require_string (functionFileNameDecl, 0);

    // Make a INT "fileIndex" to the file index that can be used to
    // in fileFunctionAt() as the index.
    functionFileIndexDecl = MD_new_field_decl (functionNameSection, 
					       "fileIndex",
					       MD_REQUIRED_FIELD);
    MD_require_int (functionFileIndexDecl, 0);


    // Make a INT "startLine" to hold the starting line no for the function
    // -1 is assumed to be unknown
    functionStartLineDecl = MD_new_field_decl (functionNameSection, 
					       "startLine", MD_REQUIRED_FIELD);
    MD_require_int (functionStartLineDecl, 0);

    // Make a INT "endLine" to the starting line no for the function
    // -1 is assumed to be unknown
    functionEndLineDecl = MD_new_field_decl (functionNameSection, "endLine",
					     MD_REQUIRED_FIELD);
    MD_require_int (functionEndLineDecl, 0);

    // Make a INT "parseState" field to hold indicate parse state
    functionParseStateDecl = MD_new_field_decl (functionNameSection, 
						"parseState",
						MD_REQUIRED_FIELD);
    MD_require_int (functionParseStateDecl, 0);

    

    // Lists all the task ids seen.  Holds task info such as hostname, etc.
    taskIdSection = newIndexedSection ("_taskId_", &taskIdIndexDecl,
				       &taskIdMap);

    // Make a INT* "threadList" to hold the task's thread id list
    taskThreadListDecl = MD_new_field_decl (taskIdSection, "threadList",
					    MD_REQUIRED_FIELD);
    MD_require_int (taskThreadListDecl, 0);
    MD_kleene_star_requirement (taskThreadListDecl, 0);



    // Lists all the thread ids seen.  
    threadIdSection = newIndexedSection ("_threadId_", &threadIdIndexDecl, 
					 &threadIdMap);

    // Make a INT* "taskList" to hold the thread's task id list
    threadTaskListDecl = MD_new_field_decl (threadIdSection, "taskList",
					    MD_REQUIRED_FIELD);
    MD_require_int (threadTaskListDecl, 0);
    MD_kleene_star_requirement (threadTaskListDecl, 0);



    // Lists all the combination of task and thread ids seen.
    // Maps this to an element index to hold data for this combo.
    PTPairSection = newIndexedSection ("_PTPair_", 
					   &PTPairIndexDecl, 
					   &PTPairMap);
    
    // Make a INT "taskId" to hold the task/thread taskId
    PTPairTaskIdDecl = MD_new_field_decl (PTPairSection, "taskId",
				       MD_REQUIRED_FIELD);
    MD_require_int (PTPairTaskIdDecl, 0);

    // Make a INT "threadId" to hold the task/thread threadId
    PTPairThreadIdDecl = MD_new_field_decl (PTPairSection, "threadId",
					 MD_REQUIRED_FIELD);
    MD_require_int (PTPairThreadIdDecl, 0);

    
    // To facilitate mapping a pair of ints (taskId and threadId)
    // to an index, create an extra symbol table that stores indexes
    PTPairIndexMap = INT_ARRAY_new_symbol_table ("PTPairIndex", 0);

    // To facilitate mapping taskId to the corresponding task entry
    taskEntryMap = INT_new_symbol_table ("taskEntry", 0);

    // To facilitate mapping threadId to the corresponding thread entry
    threadEntryMap = INT_new_symbol_table ("threadEntry", 0);


    // Lists all the data attrs seen.  Holds tool tip for dataAttr, etc.
    dataAttrSection = newIndexedSection ("_dataAttr_", 
					   &dataAttrIndexDecl, 
					   &dataAttrMap);

    // Make a STRING "text" to hold the dataAttr name the gui uses
    dataAttrTextDecl = MD_new_field_decl (dataAttrSection, "text",
					MD_REQUIRED_FIELD);
    MD_require_string (dataAttrTextDecl, 0);

    // Make a STRING "toolTip" to hold the dataAttr's tool tip
    dataAttrToolTipDecl = MD_new_field_decl (dataAttrSection, "toolTip",
					MD_REQUIRED_FIELD);
    MD_require_string (dataAttrToolTipDecl, 0);

    // Make a INT "type" to hold the dataAttr's data type (MD_INT, etc.)
    dataAttrTypeDecl = MD_new_field_decl (dataAttrSection, "type",
					MD_REQUIRED_FIELD);
    MD_require_int (dataAttrTypeDecl, 0);

    // Make a INT "initialAttrStat" to hold the dataAttr's initialAttrStat
    dataAttrSuggestedAttrStatDecl = MD_new_field_decl (dataAttrSection, 
						     "initialAttrStat", 
						     MD_REQUIRED_FIELD);
    MD_require_int (dataAttrSuggestedAttrStatDecl, 0);

    // Make a INT "initialEntryStat" to hold the dataAttr's initialEntryStat
    dataAttrSuggestedEntryStatDecl = MD_new_field_decl (dataAttrSection, 
						     "initialEntryStat", 
						     MD_REQUIRED_FIELD);
    MD_require_int (dataAttrSuggestedEntryStatDecl, 0);

    
    // Lists all the action types available.  
    // Holds actionText, toolTip, etc.
    actionSection = newIndexedSection ("_action_", 
					   &actionIndexDecl, 
					   &actionMap);

    // Make a STRING "initialState" to hold the initial state for this action 
    actionInitialStateDecl = MD_new_field_decl (actionSection, 
						"initialState", 
						MD_REQUIRED_FIELD);
    MD_require_string (actionInitialStateDecl, 0);


    
    // List all the pixmaps available to the tools
    pixmapSection = newIndexedSection ("_pixmap_", &pixmapIndexDecl, 
				       &pixmapMap);

    // Make a STRING "xpmArray" to hold the array of xpm strings
    // DEBUG, put name in field name.  Don't know if works
    xpmArrayDecl = MD_new_field_decl (pixmapSection, "xpmArray", 
					MD_REQUIRED_FIELD);
    MD_require_string (xpmArrayDecl, 0);
    MD_kleene_star_requirement (xpmArrayDecl, 0);

    // Create quick-lookup tables on the side for finding the MD_section
    // pointer associated with each raw function name.  In the md 
    // database, these names will have prefixes added to prevent name 
    // collisions and makes looking them up using MD routines painful.
    //
    // Note: All symbol_table functions punt (exit with message) if they run
    //       out of memory.  No need to check return values for 'new' calls.
    functionSectionTable = STRING_new_symbol_table ("functionSection", 0);

    // Holds all messageFolder's declared.  Currently only holds
    // the title of the message folder.
    messageFolderSection = newIndexedSection ("_messageFolder_", 
					     &messageFolderIndexDecl, 
					     &messageFolderMap);

    // Make a STRING 'messageFolderTitle' to hold the title of the message 
    // folder
    messageFolderTitleDecl = MD_new_field_decl (messageFolderSection,
					  "messageFolderTitle", 
					  MD_REQUIRED_FIELD);
    MD_require_string (messageFolderTitleDecl, 0);


    // Make a INT 'messageFolderIfEmpty' to hold the ifEmpty policy of the 
    // message folder
    messageFolderIfEmptyDecl = MD_new_field_decl (messageFolderSection,
						  "messageFolderIfEmpty", 
						  MD_REQUIRED_FIELD);
    MD_require_int (messageFolderIfEmptyDecl, 0);


    // Holds all the site display priority settings.
    sitePrioritySection = newIndexedSection ("_sitePriority_", 
					     &sitePriorityIndexDecl, 
					     &sitePriorityMap);

    // Make STRING 'sitePriorityFile' to hold RegExp to match site 'File'
    sitePriorityFileDecl = MD_new_field_decl (sitePrioritySection,
					      "sitePriorityFile", 
					      MD_OPTIONAL_FIELD);
    MD_require_string (sitePriorityFileDecl, 0);

    // Make STRING 'sitePriorityLine' to hold RegExp to match site 'Line'
    sitePriorityLineDecl = MD_new_field_decl (sitePrioritySection,
					      "sitePriorityLine", 
					      MD_OPTIONAL_FIELD);
    MD_require_string (sitePriorityLineDecl, 0);

    // Make STRING 'sitePriorityDesc' to hold RegExp to match site 'Desc'
    sitePriorityDescDecl = MD_new_field_decl (sitePrioritySection,
					      "sitePriorityDesc", 
					      MD_OPTIONAL_FIELD);
    MD_require_string (sitePriorityDescDecl, 0);

    // Make DOUBLE 'sitePriorityModifier' to hold modifier for display priority
    sitePriorityModifierDecl = MD_new_field_decl (sitePrioritySection,
						  "sitePriorityModifier", 
						  MD_REQUIRED_FIELD);
    MD_require_double (sitePriorityModifierDecl, 0);


    // If snapshotName specified, load in snapshot contents by
    // adding to "empty" database with a 1 multiplier.
    if (snapshotName != NULL)
    {
	addSnapshot(snapshotName, 1);
    }

    // Initially no user specified about text
    // User info can be prepended or appended to the about text
    // calling UIManager::addAboutText().
    aboutText = "";

#if defined(TG_LINUX)
    // Courier doesn't look good on linux
//    mainFont = QFont("console", 12);
    mainFont = QFont("courier", 10, QFont::Bold);
#elif defined(TG_MAC) 
    mainFont = QFont("Courier", 10);
#else
    mainFont = QFont("courier", 8);
#endif
    // See if there's a settings file with a main font to
    // override the one we just set; if not, default to that
    QSettings settings;
    settings.setPath( "llnl.gov", "Tool Gear", QSettings::User );
    mainFont.fromString( settings.readEntry( APP_KEY + "MainFont",
			    mainFont.toString() ) );

#if 0
    fprintf( stderr, "Initializing mainFont to %s\n",
		    mainFont.toString().latin1() );
#endif
    // See if the requested font was matched; if not, get a font
    // that represents what we really got, so we can display it
    // correctly in the dialog.
    QFontInfo mainFontActual( mainFont );
    if( ! mainFontActual.exactMatch() ) {
	    mainFont.setFamily( mainFontActual.family() );
	    mainFont.setPointSize( mainFontActual.pointSize() );
	    mainFont.setWeight( mainFontActual.weight() );
	    mainFont.setItalic( mainFontActual.italic() );
#if 0
    	    fprintf( stderr, "Rematched mainFont to %s\n",
		    mainFont.toString().latin1() );
#endif
    }


    // Set the labelFont to the default label font
    labelFont = QApplication::font();

#if 0
    // Initialize the sequence of keys
    nextPendingFileInfoKey = 0;
#endif
    // Create a souce collection object, if it doesn't already exist.
    // A single collection is shared among all instances of UIManagers.
    if( sourceCollection == NULL ) {
        sourceCollection = new FileCollection();
	TG_checkAlloc(sourceCollection);
    }

    connect( sourceCollection, SIGNAL( cleared() ),
		    this, SIGNAL( clearedSourceCache() ) );


}

/* Create delete routine that can be called from the "C" routine
// STRING_delete_symbol_table.  Used when deleting functionSectionTable
// below.  This is a static routine! */
void UIManager::deleteFuncInfo (FuncInfo *ptr)
{
    delete ptr;
}

// Deletes MD database and frees associated data
UIManager::~UIManager()
{
#if 0
    // DEBUG
    printf ("At deletion:\n");
    printSnapshot(stdout);
    printf ("\n");
#endif

    // COMEBACK, Need to delete all the symbol tables created!!!

    // Delete quick function lookup table.  
    // Contains pointers FuncInfo structures.
    // Pass "static" deleteFuncInfo to it to delete these structures
    STRING_delete_symbol_table (functionSectionTable, 
				(void (*)(void *))UIManager::deleteFuncInfo);
    
    // Deletes entire md database
    // No need to delete function sections, etc. all deleted by this command
    MD_delete_md (md);
}

/* Adds contents of snapshotName (multiplied by integer 'multiplier').
// To diff to snapshots, load one and then subtract a different one
// by using multiplier of '-1'.  May be other useful multipliers.
//
// Messages are currently loaded if the multiplier is 1.  No messages
// are added/removed if multiplier is not 1. -JCG 4/23/04
//
// Punts if snapshotName doesn't exist or file not compatible */
void UIManager::addSnapshot(const char *snapshotName, int multiplier)
{
    FILE *in;

    // Open file for reading
    if ((in = fopen (snapshotName, "r")) == NULL)
	TG_error ("UIManager::addSnapshot: %s not found!", snapshotName);

    // Read in snapshot md file
    MD *sd = MD_read_md (in, snapshotName);

    // Loop through each PTpair in the snapshot, adding it if necessary
    for (MD_Entry *PTPairEntry = SSGetFirstEntry (sd, "_PTPair_");
	 PTPairEntry != NULL; PTPairEntry = MD_next_entry(PTPairEntry))
    {
	// Get the taskId and threadId for this PTPair entry
	int taskId = SSGetInt (PTPairEntry, "taskId", 0);
	int threadId = SSGetInt (PTPairEntry, "threadId", 0);

	// Create (if necessary) this task/thread pair.
	// The routine just ignores duplicate requests
	insertPTPair (taskId, threadId);
    }

    // Loop through each dataAttr in the snapshot, adding it if necessary
    for (MD_Entry *dataAttrEntry = SSGetFirstEntry (sd, "_dataAttr_");
	 dataAttrEntry != NULL;
	 dataAttrEntry = MD_next_entry(dataAttrEntry))
    {
	// Get the dataAttrTag, dataAttrText, toolTip, and dataType
	char *dataAttrTag = dataAttrEntry->name;
	char *dataAttrText = SSGetString (dataAttrEntry, "text", 0);
	char *toolTip = SSGetString (dataAttrEntry, "toolTip", 0);
	int dataType = SSGetInt (dataAttrEntry, "type", 0);

	// If dataAttrTag already exists, make sure data type matches
	if (dataAttrIndex (dataAttrTag) != NULL_INT)
	{
	    // Get existing info
	    int ODataType = dataAttrType (dataAttrTag);

	    // The dataType is the only critical one to check
	    if (dataType != ODataType)
	    {
		TG_error ("UIManager::addSnapshot: %s "
			  "incompatible with current contents!\n"
			  "  Data attr type definition mismatch for '%s'\n"
			  "  Current data type: %i\n"
			  "  Snapshot data type: %i\n"
			  "  Snapshot must be for same tool version!",
			  snapshotName, dataAttrTag, ODataType, dataType);
	    }
	}
	// Otherwise, add data attr
	else
	{
	    declareDataAttr (dataAttrTag, dataAttrText, toolTip, dataType);
	}
    }

#if 0
    // I am thinking this should not be in this function!

    // Loop through each action in the snapshot, adding it if necessary
    for (MD_Entry *actionEntry = SSGetFirstEntry (sd, "_action_");
	 actionEntry != NULL;
	 actionEntry = MD_next_entry(actionEntry))
    {
	// Get the actionTag, actionText, and toolTip
	char *actionTag = actionEntry->name;
	char *actionText = SSGetString (actionEntry, "text", 0);
	char *toolTip = SSGetString (actionEntry, "toolTip", 0);

	// Only add actionTag, if doesn't exist
	// No good way of comparing to existing entries, since
	// toolTips and actionText can change.
	if (actionIndex (actionTag) == NULL_INT)
	{
	    declareAction (actionTag, actionText, toolTip);
	}
    }
#endif

    //
    // Loop through each funcName in the snapshot, adding its contents
    //
    for (MD_Entry *funcEntry = SSGetFirstEntry (sd, "_functionName_");
	 funcEntry != NULL; funcEntry = MD_next_entry (funcEntry))
    {
	// Get the funcName, fileName, startLine, and endLine from this entry
	char *funcName = funcEntry->name;
	char *fileName = SSGetString(funcEntry, "fileName", 0);
	int startLine = SSGetInt (funcEntry, "startLine", 0);
	int endLine = SSGetInt (funcEntry, "endLine", 0);

	// If function already exists, make sure info matches
	if (functionIndex (funcName) != NULL_INT)
	{
	    // Get the existing info
	    QString OFileName = functionFileName (funcName);
	    int OStartLine = functionStartLine (funcName);
	    int OEndLine = functionEndLine (funcName);

	    // Compare and punt on error
	    if ((OFileName.compare(funcName) != 0) ||
		(OStartLine != startLine) ||
		(OEndLine != endLine))
	    {
		TG_error ("UIManager::addSnapshot: %s incompatible "
			  "with current contents!\n"
			  "  Function definition mismatch for '%s'\n"
			  "  Current:\n"
			  "    fileName: %s\n"
			  "    startLine: %i\n"
			  "    endLine: %i\n"
			  "  Snapshot:\n"
			  "    fileName: %s\n"
			  "    startLine: %i\n"
			  "    endLine: %i\n"
			  "  Snapshot must be for same code/code version!",
			  snapshotName, funcName, 
			  OFileName.latin1(), OStartLine, OEndLine,
			  fileName, startLine, endLine);
	    }
	}
	// Otherwise, add function to database
	else
	{
	    insertFunction (funcName, fileName, startLine, endLine);
	}

	// Get the name of the data section for this function name (prefix D_)
	QString tempDataName;
	tempDataName.sprintf ("D_%s", funcName);
	char *funcDataName = (char *)tempDataName.latin1();

	// Go through all the entries for this function, adding entry contents
	for (MD_Entry *entry =  SSGetFirstEntry (sd, funcDataName);
	     entry != NULL; entry = MD_next_entry (entry))
	{
	    // Get entryKey, line, type, and toolTip for this entry
	    char *entryKey = entry->name;
	    int line = SSGetInt (entry, "_line_", 0);
	    TG_InstPtType type = (TG_InstPtType) SSGetInt (entry, "_type_", 0);
	    char *toolTip = SSGetString (entry, "_toolTip_", 0);

	    // If entry exists, make sure on same line
	    if (entryIndex (funcName, entryKey) != NULL_INT)
	    {
		// Get the current line
		int OLine = entryLine (funcName, entryKey);

		// The line is the only critical one to check
		if (line != OLine)
		{
		    TG_error ("UIManager::addSnapshot: %s "
			      "incompatible with current contents!\n"
			      "  Entry '%s' line mismatch for '%s'\n"
			      "  Current line: %i\n"
			      "  Snapshot line: %i\n"
			      "  Snapshot must be for same tool version!",
			      snapshotName, entryKey, funcName, OLine, line);
		}
	    }
	    // Otherwise, add the entry
	    else
	    {
		// Declare default values for location, funcCalled, callIndex
		TG_InstPtLocation location = TG_IPL_invalid;
		char *funcCalled = NULL;
		int callIndex = NULL_INT;

		// For TG_IPT_function_call, read in location, funcCalled,
		// and callIndex (otherwise, use above default values)
		if (type == TG_IPT_function_call)
		{
		    location = (TG_InstPtLocation) SSGetInt (entry, 
							     "_location_", 0);
		    funcCalled = SSGetString (entry, "_funcCalled_", 0);
		    callIndex = SSGetInt (entry, "_callIndex_", 0);
		}

		insertEntry (funcName, entryKey, line, type, location,
			     funcCalled, callIndex, toolTip);
	    }

	    // Loop through each dataAttr, adding its contents
	    for (MD_Entry *dataAttrEntry = 
		     SSGetFirstEntry (sd, "_dataAttr_");
		 dataAttrEntry != NULL;
		 dataAttrEntry = MD_next_entry(dataAttrEntry))
	    {
		// Get the dataAttrTag 
		char *dataAttrTag = dataAttrEntry->name;

		// Get the field declaration for this field
		MD_Field_Decl *decl = MD_find_field_decl (entry->section,
							  dataAttrTag);

		// Declaration better exist!
		if (decl == NULL)
		{
		    TG_error ("UIManager::addSnapshot: %s appears corrupted.\n"
			      "  Expect field '%s'\n"
			      "     for entry '%s'\n"
			      "     in section '%s'!",
			      snapshotName, dataAttrTag, entryKey, 
			      funcDataName);
		}

		// Get the field for this entry, may not exist
		MD_Field *field = MD_find_field (entry, decl);

		// If doesn't exist, goto next data attr
		if (field == NULL)
		    continue;

		// Loop through the PTPairs, adding non-NULL data
		for (MD_Entry *PTPairEntry = SSGetFirstEntry (sd, "_PTPair_");
		     PTPairEntry != NULL; 
		     PTPairEntry = MD_next_entry (PTPairEntry))
		{
		    // Get the index for this pair
		    int index = SSGetInt (PTPairEntry, "_index_", 0);

		    // Skip empty elements
		    if (field->element[index] == NULL)
			continue;

		    // Get the taskId and threadId for this data element
		    int taskId = SSGetInt (PTPairEntry, "taskId", 0);
		    int threadId = SSGetInt (PTPairEntry, "threadId", 0);

		    // Handle the different data types
		    int intValue, intIncrement;
		    double doubleValue, doubleIncrement;
		    switch (field->element[index]->type)
		    {
			// Scale int and add to database value
		      case MD_INT:
			intValue = MD_get_int (field, index);
			intIncrement = intValue * multiplier;
			addInt (funcName, entryKey, dataAttrTag,
				taskId, threadId, intIncrement);
			break;

			// Scale double and add to database value
		      case MD_DOUBLE:
			doubleValue = MD_get_double (field, index);
			doubleIncrement = doubleValue * (double)multiplier;
			addDouble (funcName, entryKey, dataAttrTag,
				   taskId, threadId, doubleIncrement);
			break;

		      default:
			TG_error ("UIManager::addSnapshot: %s "
				  "contains unhandled/unexpected data!\n"
				  "  FuncTag: '%s'\n"
				  "  EntryTag: '%s'\n"
				  "  DataAttrTag: '%s'\n"
				  "  taskID: %i\n"
				  "  threadID: %i\n"
				  "  index: %i\n"
				  "  dataType: %i\n"
				  "Need to expand functionality!",
				  snapshotName, funcName, entryKey, 
				  dataAttrTag, taskId, threadId, index, 
				  field->element[index]->type);
		    }
		}
	    }
	}
    }

    // Handle messages only if multiplier is 1
    if (multiplier == 1)
    {
	// Loop through each message folder in snapshot, adding it
	for (MD_Entry *MFolderEntry = SSGetFirstEntry (sd, "_messageFolder_");
	     MFolderEntry != NULL; MFolderEntry = MD_next_entry (MFolderEntry))
	{
	    // Get messageFolderTag and messageFolderTitle
	    char *messageFolderTag = MFolderEntry->name;
	    char *messageFolderTitle = 
		SSGetString (MFolderEntry, "messageFolderTitle", 0);
	    
	    // Delare message folder with snapshot info
	    declareMessageFolder (messageFolderTag, messageFolderTitle);

	    // Generate name of message folder section (prefix M_)
	    QString tempMSecName;
	    tempMSecName.sprintf ("M_%s", messageFolderTag);
	    char *messageSecName = (char *) tempMSecName.latin1();

	    // Need to build up messageText now from many elements
	    MessageBuffer messageTextBuf;

	    // Loop through each message in message folder, adding it
	    for (MD_Entry *entry = SSGetFirstEntry (sd, messageSecName);
		 entry != NULL; entry = MD_next_entry (entry))
	    {
		// Get messageTextand messageTraceback

		// Need to assemble the messageText from all the elements
		// in the field
		MD_Field *messageTextField = SSGetField (entry, 
							 "_messageText_",
							 0, MD_STRING);

		// Get the number of lines in the message text
		int numLines = MD_num_elements (messageTextField);
		
		// Get the first line
		const char *headerText = MD_get_string (messageTextField, 0);
		messageTextBuf.sprintf ("%s", headerText);

		// Get the rest of the lines
		for (int line = 1; line < numLines; ++line)
		{
		    const char *bodyText = MD_get_string (messageTextField, 
							  line);
		    messageTextBuf.appendSprintf ("\n%s", bodyText);
		    
		}

		// Handle optional field
		char *messageTraceback = "";
		if (SSFieldExists (entry, "_messageTraceback_", 0))
		{
		    messageTraceback = 
			SSGetString (entry, "_messageTraceback_", 0);
		}

		// Add message to message folder
		addMessage (messageFolderTag, messageTextBuf.contents(), 
			    messageTraceback);
	    }
	}
    }

    // Free snapshot md file
    MD_delete_md (sd);
}

// Internal addSnapshot() helper routine to get first entry in a section
// Punts on any error (indicating addSnapshot() had error)
MD_Entry *UIManager::SSGetFirstEntry(MD *sd, const char *sectionName)
{
    // Get the named section
    MD_Section *section = MD_find_section (sd, sectionName);

    // Punt with descriptive error if section not found
    if (section == NULL)
    {
	TG_error ("UIManager::addSnapshot: '%s' is not a valid snapshot file!"
		  "\n   Section '%s' expected in file and it was not found!\n",
		  sd->name, sectionName);
    }
    
    // Get the first entry
    MD_Entry *entry = MD_first_entry(section);
    
    // Return the first entry
    return (entry);
}

// Internal addSnapShot() helper routine to get field from entry's field.
// Punts on any error (indicating addSnapshot() had error),
// including if the value at the index is of the wrong type.
MD_Field *UIManager::SSGetField (MD_Entry *entry, const char *fieldName, int index,
				 int type)
{
    // Get the named field declaration
    MD_Field_Decl *field_decl = MD_find_field_decl (entry->section, fieldName);
    
    // Punt with descriptive error if field declaration not found
    if (field_decl == NULL)
    {
	TG_error ("UIManager::addSnapshot: '%s' is not a valid snapshot file!"
		  "\n   Expect field '%s' in section '%s'!\n",
		  entry->section->md->name, fieldName, entry->section->name);
    }

    // Get the named field from the entry
    MD_Field *field = MD_find_field (entry, field_decl);
    
    // Punt with descriptive error if field not found
    if (field == NULL)
    {
	TG_error ("UIManager::addSnapshot: '%s' is not a valid snapshot file!"
		  "\n   Expect field '%s' for entry '%s' in section '%s'!\n",
		  entry->section->md->name, fieldName, entry->name, 
		  entry->section->name);
    }

    // Punt if named element does not exist in field or is the wrong type
    if ((index < 0) || 
	(index > MD_max_element_index(field)) ||
	(field->element[index] == NULL) || 
	(field->element[index]->type != type))
    {
	char *typeName = "(unknown)";
	switch (type)
	{
	  case MD_INT:
	    typeName = "integer";
	    break;

	  case MD_DOUBLE:
	    typeName = "double";
	    break;

	  case MD_STRING:
	    typeName = "string";
	    break;

	  case MD_LINK:
	    typeName = "link";
	    break;
	}

	TG_error ("UIManager::addSnapshot: '%s' is not a valid snapshot file!"
		  "\n   Expect %s value at index %i in field '%s'"
		  "   for entry '%s' in section '%s'!\n",
		  entry->section->md->name, typeName, index, fieldName, 
		  entry->name, entry->section->name);
    }

    // Return the checked field pointer
    return (field);
}

// Internal addSnapShot() helper routine that returns 1 if the entry's
// field element exists, 0 otherwise. 
// Returns 0 on any errors (doesn't punt).
bool UIManager::SSFieldExists (MD_Entry *entry, const char *fieldName, 
			       int index)
{
    // Get the named field declaration
    MD_Field_Decl *field_decl = MD_find_field_decl (entry->section, fieldName);
    
    // Return FALSE if field declaration not found
    if (field_decl == NULL)
    {
	return (FALSE);
    }

    // Get the named field from the entry
    MD_Field *field = MD_find_field (entry, field_decl);
    
    // Return FALSE if field not found
    if (field == NULL)
    {
	return (FALSE);
    }

    // Return FALSE if named element does not exist in field
    if ((index < 0) || 
	(index > MD_max_element_index(field)) ||
	(field->element[index] == NULL))
    {
	return (FALSE);
    }
    
    // Must exist if got here, return 1
    return (TRUE);
}


// Internal addSnapshot() helper routine to get int value from entry field
// Punts on any error (indicating addSnapshot() had error)
int UIManager::SSGetInt(MD_Entry *entry, const char *fieldName, int index)
{
    // Use helper routine to get and check the int field element
    MD_Field *field = SSGetField (entry, fieldName, index, MD_INT);

    // Get and return the value
    int intValue = MD_get_int (field, index);
    return (intValue);
}


// Internal addSnapshot() helper routine to get double value from field
// Punts on any error (indicating addSnapshot() had error)
double UIManager::SSGetDouble(MD_Entry *entry, const char *fieldName, int index)
{
    // Use helper routine to get and check the double field element
    MD_Field *field = SSGetField (entry, fieldName, index, MD_DOUBLE);

    // Get and return the value
    double doubleValue = MD_get_double (field, index);
    return (doubleValue);
}

// Internal addSnapshot() helper routine to get string value from field
// Punts on any error (indicating addSnapshot() had error)
char *UIManager::SSGetString(MD_Entry *entry, const char *fieldName, int index)
{
    // Use helper routine to get and check the string field element
    MD_Field *field = SSGetField (entry, fieldName, index, MD_STRING);

    // Get and return the value
    char *stringValue = MD_get_string (field, index);
    return (stringValue);
}



// Internal routine to create an "indexed" section.
// Creates the index field declaration and index map which is updated
// by newIndexedEntry() and used by 'QString xxxAt(int)' and 
// 'int xxxIndex(const char *name)'.
MD_Section *UIManager::newIndexedSection (const char *sectionName, 
					  MD_Field_Decl **indexDecl,
					  INT_Symbol_Table **indexMap)
{
    // Create the new section
    MD_Section *section = MD_new_section (md, sectionName, 0, 0);

    // Declare an INT '_index_' field in this section.  Return thru parameter.
    *indexDecl = MD_new_field_decl (section, "_index_", MD_REQUIRED_FIELD);
    MD_require_int (*indexDecl, 0);

    // Create the index map for this section and return thru parameter
    *indexMap = INT_new_symbol_table (sectionName, 0);

    // Return the new indexed section
    return (section);
}

// Internal routine to create an "indexed" entry for the specified section.
// Stashes extra info into the indexDecl field and into indexMap in order 
// to facilitate 'QString xxxAt(int)' and 'int xxxIndex(const char *name)'.
// If return_index not NULL, returns new index in address passed
MD_Entry *UIManager::newIndexedEntry (MD_Section *section, const char *entryName, 
				      MD_Field_Decl *indexDecl,
				      INT_Symbol_Table *indexMap,
				      int *return_index)
{
    // Create new entry in the passed section
    MD_Entry *entry = MD_new_entry (section, entryName);

    // Calculate the 0-based index for this entry in the above section
    // Must call *after* MD_new_entry above.
    int index = MD_num_entries (section) - 1;

    // Create index field in this entry to hold the index, and set it
    MD_Field *field = MD_new_field (entry, indexDecl, 0);
    MD_set_int (field, 0, index);

    // Allocate QString version of entryName
    QString *entryNameQString = new QString (entryName);
    TG_checkAlloc(entryNameQString);

    // Store pointer to this QString version of entryName in map at index
    INT_add_symbol (indexMap, index, (void *)entryNameQString);

    // If return_index not NULL, return index in that variable
    if (return_index != NULL)
	*return_index = index;

    // Return new indexed entry
    return (entry);
}

// Print out snapshot in human readable format with the given page width
void UIManager::printSnapshot (FILE *out, int pageWidth)
{
#if 0
    // Print out declarations, so can be recompiled if desired
    // This makes the printout less readable, may not want to do
    MD_print_md_declarations (out, md, pageWidth);
#endif

    // Print out the actual content 
    MD_print_md (out, md, pageWidth);
}

// Write out snapshot in easy to parse, machine independent format
// Not easily read by humans!
void UIManager::writeSnapshot (FILE *out) 
{
    MD_write_md (out, md);
}

// Adds info entry set to 'string' to the info table
// These strings currently are for informational purposes only
void UIManager::setInfoValue (const char *infoName, const char *string)
{
    // Get existing entry, if exists
    MD_Entry *infoEntry = MD_find_entry (infoSection, infoName);

    // If it doesn't exist, create indexed entry and value field
    MD_Field *valueField;
    if (infoEntry == NULL)
    {
	infoEntry = newIndexedEntry (infoSection, infoName, 
				     infoIndexDecl, infoMap);
	valueField = MD_new_field (infoEntry, infoValueDecl, 0);
    }
    // Otherwise, get existing field
    else
    {
	valueField = MD_find_field (infoEntry, infoValueDecl);
    }

    // If string NULL, set to ""
    if (string == NULL)
	string = "";

    // Set value to the passed string
    MD_set_string (valueField, 0, string);
}

// Get QString value of infoName saved in the InfoValue section
// Returns NULL_QSTRING if infoName not found
QString UIManager::getInfoValue (const char *infoName)
{
    // Get existing entry, if exists
    MD_Entry *infoEntry = MD_find_entry (infoSection, infoName);

    // If it doesn't exist, return NULL_QSTRING
    if (infoEntry == NULL)
    {
	return (NULL_QSTRING);
    }

    // Otherwise, get existing field
    MD_Field *valueField;
    valueField = MD_find_field (infoEntry, infoValueDecl);

    // Get value and return
    return (MD_get_string (valueField, 0));
}

// Register's file name and returns index function inserted at.
// Calling insertFunction will insert the file if it doesn't
// exist, so it is not strictly necessary to call this.  
// Ignores duplicate calls but always returns function index.
int UIManager::insertFile (const char *fileName)
{
    // Get file index using internal helper routine (may be NULL_INT)
    int index = getFieldInt(fileNameSection, fileName, fileNameIndexDecl, 0);

    // If doesn't exist, create it and the functionList & functionMap fields
    // Also set parseState field to FileUnparsed
    if (index == NULL_INT)
    {
	MD_Entry *fileNameEntry = newIndexedEntry (fileNameSection, fileName,
						   fileNameIndexDecl, 
						   fileNameMap, &index);

	// Create FunctionList field, initial size 1, will dynamically resize!
	MD_new_field (fileNameEntry, fileFunctionListDecl, 1);

	// Create FunctionMap field, initial size 1, will dynamically resize!
	MD_new_field (fileNameEntry, fileFunctionMapDecl, 1);

	// Create parseState field, size 1 and set it to FileUnparsed
	MD_Field *parseStateField = MD_new_field (fileNameEntry, 
						  fileParseStateDecl, 1);
	MD_set_int (parseStateField, 0, fileUnparsed);

#if 0
	// DEBUG
	printf ("UIManager::insertFile: emiting inserted %s\n", fileName);
#endif

	// Emit signal to notify any listerners that a file has been inserted
	emit fileInserted (fileName);
    }

    // Return file index
    return (index);
}


// Creates table for the named function so that entries may be
// made in it.  Also indicates where the function source is located.
// Returns index function inserted at.  
int UIManager::insertFunction (const char *funcName, const char *fileName, 
			       int startLine, int endLine)
{
    // If anything is weird about startLine or endLine, set
    // both to -1 so that it won't screw up algorithms that
    // expect reasonableness.
    if ((startLine < 1) || (endLine < 1) || (endLine < startLine))
    {
	startLine = -1;
	endLine = -1;
    }

    //
    // Add function info to function name section
    //

    // Make sure function doesn't already exist 
    // JMM -- changed punt to just return (FIX?)
    if (MD_find_entry (functionNameSection, funcName) != NULL)
	    return -1;
//	TG_error ("UIManager::insertFunction: %s already inserted!", funcName);

    // Add function name to indexed function name list and get index
    int index;
    MD_Entry *functionNameEntry = newIndexedEntry (functionNameSection, 
						   funcName, 
						   functionNameIndexDecl,
						   functionNameMap,
						   &index);

    // Set file name for this function
    MD_Field *fileNameField = 
	MD_new_field (functionNameEntry, functionFileNameDecl, 1);
    MD_set_string (fileNameField, 0, fileName);

    // Set the start line for this function
    MD_Field *startLineField = 
	MD_new_field (functionNameEntry, functionStartLineDecl, 1);
    MD_set_int (startLineField, 0, startLine);

    // Set the end line for this function
    MD_Field *endLineField = 
	MD_new_field (functionNameEntry, functionEndLineDecl, 1);
    MD_set_int (endLineField, 0, endLine);

    // Set function to unparsed state
    MD_Field *parseStateField = 
	MD_new_field (functionNameEntry, functionParseStateDecl, 1);
    MD_set_int (parseStateField, 0, functionUnparsed);

    // FileIndex field set below!

    //
    // Add file info to file name section
    //
    
    // Insert file name, if not already inserted (ignores duplicates)
    insertFile (fileName);

    // Get file name entry (should not be NULL since inserted above)
    MD_Entry *fileNameEntry = MD_find_entry (fileNameSection, fileName);

    // Get function list and map fields
    MD_Field *functionListField = MD_find_field (fileNameEntry, 
						 fileFunctionListDecl);
    MD_Field *functionMapField = MD_find_field (fileNameEntry, 
						fileFunctionMapDecl);
	
    // Get the existing number of elements in the function list,
    // this is the new fileIndex for this function
    int fileIndex = MD_num_elements (functionListField);
    
    // Insert new element at 'fileIndex' element, works because
    // list is zero based (so fileIndex is just after last element)
    MD_set_string (functionListField, fileIndex, funcName);

    // Set the _fileIndex_ for this function
    MD_Field *fileIndexField = 
	MD_new_field (functionNameEntry, functionFileIndexDecl, 1);
    MD_set_int (fileIndexField, 0, fileIndex);


    // Handle function line mapping if startLine and endLine positive
    // They default to -1 if not explicitly specified.
    if ((startLine > 0) && (endLine > 0))
    {
	// Sanity check, make sure nothing is already mapped betwen
	// startLine and endLine.  Do in separate loop to optimize
	// check (hopefully the map is not already allocated for this range)
	int maxMapped = MD_max_element_index (functionMapField);
	for (int lineNo = startLine; 
	     (lineNo <= maxMapped) && (lineNo <= endLine); lineNo++)
	{
	    // For those map position allocated, expect element to be NULL
	    if (functionMapField->element[lineNo] != NULL)
	    {
		// Print out warning that mapping conflict occurred
		int mappedIndex = MD_get_int(functionMapField, lineNo);
		QString mappedFuncName = functionAt(mappedIndex);
// JMM: reinstate this code!  Just took it out to avoid distraction
// while debugging something else
#if 0
		fprintf (stderr, 
			 "Warning: %s line %i: mapped to two functions:\n"
			 "  %s\n"
			 "  %s\n",
			 fileName, lineNo, mappedFuncName.latin1(),
			 funcName);
#endif
	    }
	}

	// Insert function index into functionMap for all the lines
	// between startLine and endLine.  Start from endLine to
	// allow the MD routines to allocate all the space needed at once
	// (since could have a lot of lines covered by this function)
	for (int lineNo = endLine; lineNo >= startLine; lineNo--)
	{
	    // Set the entry at the lineNo to the function index
	    MD_set_int (functionMapField, lineNo, index);
	}
    }

    //
    // Add new 'data' and 'action' sections for this function name
    //
    
    // Create 'data' section for this function.  Prefix function
    // name for this section with 'D_'.  Also prevents name conflicts
    // with internal section names as _taskId_.
    QString dataName_buf;
    dataName_buf.sprintf ("D_%s", funcName);
    MD_Field_Decl *indexDecl;
    INT_Symbol_Table *indexMap;
    MD_Section *funcDataSection = 
	newIndexedSection (dataName_buf.latin1(), &indexDecl, 
			   &indexMap);

    // Add INT _lineIndex_ field, to hold the entry index relative to this line
    MD_Field_Decl *lineIndexDecl = MD_new_field_decl (funcDataSection, 
						 "_lineIndex_",
						 MD_REQUIRED_FIELD);
    MD_require_int (lineIndexDecl, 0);


    // Add INT _line_ field, to hold the line this entry corresponds to 
    MD_Field_Decl *lineDecl = MD_new_field_decl (funcDataSection, "_line_",
						 MD_REQUIRED_FIELD);
    MD_require_int (lineDecl, 0);

    // Add INT _type_ field, to hold the instrumentation point type 
    // this entry corresponds to
    MD_Field_Decl *typeDecl = MD_new_field_decl (funcDataSection, 
						 "_type_",
						 MD_REQUIRED_FIELD);
    MD_require_int (typeDecl, 0);


    // Add INT _location_ field, to hold the instrumentation point location 
    // this entry corresponds to
    MD_Field_Decl *locationDecl = MD_new_field_decl (funcDataSection, 
						     "_location_",
						     MD_OPTIONAL_FIELD);
    MD_require_int (locationDecl, 0);


    // Add STRING _funcCalled_ field, to hold the tool tip for this line
    MD_Field_Decl *funcCalledDecl = MD_new_field_decl (funcDataSection, 
						       "_funcCalled_",
						       MD_OPTIONAL_FIELD);
    MD_require_string (funcCalledDecl, 0);


    // Add INT _callIndex_ field, to hold the instrumentation point callIndex 
    // this entry corresponds to
    MD_Field_Decl *callIndexDecl = MD_new_field_decl (funcDataSection, 
						      "_callIndex_",
						      MD_OPTIONAL_FIELD);
    MD_require_int (callIndexDecl, 0);


    // Add STRING _toolTip_ field, to hold the tool tip for this line
    MD_Field_Decl *toolTipDecl = MD_new_field_decl (funcDataSection, 
						    "_toolTip_",
						    MD_REQUIRED_FIELD);
    MD_require_string (toolTipDecl, 0);


    // Create 'action' section for this function.  Prefix function
    // name for this section with 'A_'.  Also prevents name conflicts
    // with internal section names as _taskId_.
    QString actionName_buf;
    actionName_buf.sprintf ("A_%s", funcName);
    MD_Section *funcActionSection = 
	MD_new_section (md, actionName_buf.latin1(), 0, 0);
    
    // Add LINK(_action_)* _actions_ field to data section, to hold 
    // the actions that are enabled for this entry
    MD_Field_Decl *actionsDecl = MD_new_field_decl (funcActionSection, 
						    "_actions_",
						    MD_OPTIONAL_FIELD);
    MD_require_link (actionsDecl, 0, actionSection);
    MD_kleene_star_requirement (actionsDecl, 0);

    // To speed up lookup of function sections (don't want to have
    // to prefix it every time), store FuncInfo structure with the
    // data and action section pointers, and the field decl pointers
    // in table that can be looked up with  the original name
    STRING_add_symbol (functionSectionTable, funcName, 
		       (void *)new FuncInfo(funcDataSection, 
					    funcActionSection, 
					    indexDecl, indexMap, 
					    lineIndexDecl, lineDecl,
					    typeDecl, locationDecl, 
					    funcCalledDecl, callIndexDecl,
					    toolTipDecl, actionsDecl));


    // Add to this function the data attrs already declared
    for (MD_Entry *dataAttrEntry = MD_first_entry (dataAttrSection);
	 dataAttrEntry != NULL; dataAttrEntry = MD_next_entry (dataAttrEntry))
    {
	// Use private function to add dataAttr, so added the same way 
	// by this routine and declareDataAttr();
	declareDataAttr (funcDataSection, dataAttrEntry);
    }

    // Add to this function the action type fields already declared
    for (MD_Entry *actionEntry = MD_first_entry (actionSection);
	 actionEntry != NULL; actionEntry = MD_next_entry (actionEntry))
    {
	// Use private function to add action field, so added the same way 
	// by this routine and declareAction();
	declareAction (funcActionSection, actionEntry);
    }

    // Emit signal to notify any listeners that a function has been inserted
    emit functionInserted (funcName, fileName, startLine, endLine);

    // Return index this function inserted at
    return (index);
}

// Sets function parse state, initially functionUnparsed
void UIManager::functionSetState (const char *functionName, functionState state)
{
    // Do nothing if already in desired state (or function doesn't 
    // exist, indicated by NULL_INT)
    functionState currentState = functionGetState (functionName);
    if ((currentState == state) || (((int)currentState == NULL_INT)))
	return;

    // Know entry and field exists (because didn't get NULL_INT above)
    // So grab the field.
    MD_Entry *functionEntry = MD_find_entry (functionNameSection, 
					     functionName);
    MD_Field *stateField = MD_find_field (functionEntry, 
					  functionParseStateDecl);

    // Set the state to the new setting
    MD_set_int (stateField, 0, (int)state);

    emit functionStateChanged (functionName, state);
}

// Returns function parse state 
UIManager::functionState UIManager::functionGetState (const char *functionName)
{
    // Get state using internal helper routine and return it
    functionState state = 
	(functionState) getFieldInt(functionNameSection, functionName, 
				    functionParseStateDecl, 0);
    return (state);
}

// Returns the number of functions currently inserted. 
int UIManager::functionCount ()
{
    // Get count from MD table that holds inserted function names
    int count = MD_num_entries (functionNameSection);

    // Return this count
    return (count);
}

// Returns index of funcName, NULL_INT if funcName not found.
int UIManager::functionIndex (const char *funcName)
{
    // Get index using internal helper routine and return it
    int index = getFieldInt(functionNameSection, funcName, 
			    functionNameIndexDecl, 0);
    return (index);
}

// Returns funcName at index (0 - count-1), NULL_QSTRING if out of bounds.
// Returns QString which makes shallow copies and protects internal copy
QString UIManager::functionAt (int index)
{
    QString *funcNamePtr = 
	(QString *)INT_find_symbol_data(functionNameMap, index);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (funcNamePtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*funcNamePtr);
}

// Returns fileName for funcName, NULL_QSTRING if funcName not found
QString UIManager::functionFileName (const char *funcName)
{
    // Get fileName using internal helper routine and return it
    QString fileName = getFieldQString(functionNameSection, funcName, 
				       functionFileNameDecl, 0);
    return (fileName);
}

// Returns fileIndex of funcName, NULL_INT if funcName not found.
// Returns function's index from file's point of view, can be used
// in fileFunctionAt(fileName, index) call.
int UIManager::functionFileIndex (const char *funcName)
{
    // Get fileIndex using internal helper routine and return it
    int fileIndex = getFieldInt(functionNameSection, funcName, 
				functionFileIndexDecl, 0);
    return (fileIndex);
}

// Returns startLine for funcName, NULL_INT if funcName not found
int UIManager::functionStartLine (const char *funcName)
{
    // Get startLine using internal helper routine and return it
    int startLine = getFieldInt(functionNameSection, funcName, 
				functionStartLineDecl, 0);
    return (startLine);
}

// Returns endLine for funcName, NULL_INT if funcName not found
int UIManager::functionEndLine (const char *funcName)
{
    // Get endLine using internal helper routine and return it
    int endLine = getFieldInt(functionNameSection, funcName, 
			      functionEndLineDecl, 0);
    return (endLine);
}

// Sets file parse state, initially FileUnparsed
void UIManager::fileSetState (const char *fileName, fileState state)
{
    // Do nothing if already in desired state (or file doesn't exist NULL_INT)
    fileState currentState = fileGetState (fileName);
    if ((currentState == state) || (((int)currentState == NULL_INT)))
	return;

    // Know entry and field exists (because didn't get NULL_INT above)
    // So grab the field.
    MD_Entry *fileEntry = MD_find_entry (fileNameSection, fileName);
    MD_Field *stateField = MD_find_field (fileEntry, fileParseStateDecl);

    // Set the state to the new setting
    MD_set_int (stateField, 0, (int)state);

    emit fileStateChanged (fileName, state);
}

// Returns file parse state 
UIManager::fileState UIManager::fileGetState (const char *fileName)
{
    // Get state using internal helper routine and return it
    fileState state = (fileState) getFieldInt(fileNameSection, fileName, 
					      fileParseStateDecl, 0);
    return (state);
}

// Returns the number of files currently inserted. 
int UIManager::fileCount ()
{
    // Get count from MD table that holds inserted file names
    int count = MD_num_entries (fileNameSection);

    // Return this count
    return (count);
}

// Returns index of fileName, NULL_INT if fileName not found.
int UIManager::fileIndex (const char *fileName)
{
    // Get index using internal helper routine and return it
    int index = getFieldInt(fileNameSection, fileName, fileNameIndexDecl, 0);
    return (index);
}

// Returns fileName at index (0 - count-1), NULL_QSTRING if out of bounds.
// Returns QString which makes shallow copies and protects internal copy
QString UIManager::fileAt (int index)
{
    QString *fileNamePtr = 
	(QString *)INT_find_symbol_data(fileNameMap, index);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (fileNamePtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*fileNamePtr);
}

// Returns the number of functions inserted for this file
int UIManager::fileFunctionCount(const char *fileName)
{
    // Get entry for this file
    MD_Entry *fileEntry = MD_find_entry (fileNameSection, fileName);

    // Return NULL_INT if file not found
    if (fileEntry == NULL)
	return (NULL_INT);

    // Get function field for this entry
    MD_Field *funcField = MD_find_field (fileEntry, fileFunctionListDecl);

    // Get function name count from this field and return it
    int count = MD_num_elements(funcField);
    return (count);
}

// Returns funcName at index for this file, NULL_QSTRING if out of bounds.
QString UIManager::fileFunctionAt(const char *fileName, int index)
{
    // Get funcName using internal helper routine and return it
    QString funcName = getFieldQString(fileNameSection, fileName, 
				       fileFunctionListDecl, index);
    return (funcName);
}

// Returns funcName at line in this file, NULL_QSTRING if none mapped
QString UIManager::fileFunctionAtLine(const char *fileName, int lineNo)
{
    // Get func index using internal helper routine
    int funcIndex = getFieldInt(fileNameSection, fileName, 
				fileFunctionMapDecl, lineNo);
    
    // Return NULL_QSTRING now if funcIndex not set (NULL_INT)
    if (funcIndex == NULL_INT)
	return (NULL_QSTRING);

    // Return the funcName at that index
    return (functionAt(funcIndex));

    // COME BACK!!!  
    // May want to adjust bounds for functions as entries are
    // added.
}





// Creates dataAttr for every function table that data may be written
// into.  dataType must be MD_INT, MD_DOUBLE, or MD_STRING for now.
// The dataAttrTag is used internally and the dataAttrText is the dataAttr
// header shown in the GUI, and the toolTip is the dataAttr's tool tip
void UIManager::declareDataAttr (const char *dataAttrTag, const char *dataAttrText, 
				 const char *toolTip, int dataType,
				 AttrStat suggestedAttrStat,
				 EntryStat suggestedEntryStat)
{
    // Don't allow tags starting with '_'.  Internal fields begin with _.
    if (dataAttrTag[0] == '_')
    {
	TG_error ("UIManager::declareDataAttr: Invalid tag '%s', may not "
		  "begin with '_'!");
    }
    
    // Make sure tag not already in use
    if (MD_find_entry (dataAttrSection, dataAttrTag) != NULL)
    {
	TG_error ("UIManager::declareDataAttr: dataAttr '%s' already "
		  "defined!");

    }
    
    // Add dataAttr tag to section
    MD_Entry *dataAttrEntry = newIndexedEntry (dataAttrSection, dataAttrTag,
					     dataAttrIndexDecl,
					     dataAttrMap);

    // Add name field and fill it with dataAttrText
    MD_Field *nameField = MD_new_field (dataAttrEntry, dataAttrTextDecl, 1);
    MD_set_string (nameField, 0, dataAttrText);

    // Add toolTip field and fill it with toolTip
    MD_Field *toolTipField = MD_new_field (dataAttrEntry, 
					   dataAttrToolTipDecl, 1);
    MD_set_string (toolTipField, 0, toolTip);

    // Add type field and fill it with dataType
    MD_Field *typeField = MD_new_field (dataAttrEntry, dataAttrTypeDecl, 1);
    MD_set_int (typeField, 0, dataType);


    // Add suggestedAttrStat field and fill it with suggestedAttrStat
    MD_Field *suggestedAttrStatField = 
	MD_new_field (dataAttrEntry, dataAttrSuggestedAttrStatDecl, 1);
    MD_set_int (suggestedAttrStatField, 0, (int)suggestedAttrStat);

    // Add suggestedEntryStat field and fill it with suggestedEntryStat
    MD_Field *suggestedEntryStatField = 
	MD_new_field (dataAttrEntry, dataAttrSuggestedEntryStatDecl, 1);
    MD_set_int (suggestedEntryStatField, 0, (int)suggestedEntryStat);

    // Add this data attr to all existing function data sections (those
    // beginning with D_
    for (MD_Section *funcSection = MD_first_section (md); 
	 funcSection != NULL; funcSection = MD_next_section (funcSection))
    {
	// Skip non-function sections (those that don't begin with D_)
	if ((funcSection->name[0] != 'D') || (funcSection->name[1] != '_'))
	    continue;

	// Use private function to add dataAttr, so added the same way 
	// by this routine and insertFunction();
	declareDataAttr (funcSection, dataAttrEntry);
    }
    
    // Emit signal to notify any listeners that a dataAttr has been declared
    emit dataAttrDeclared (dataAttrTag, dataAttrText, toolTip, dataType);
}


// Returns the number of dataAttrs currently inserted.
int UIManager::dataAttrCount()
{
    // Get count from MD table that holds inserted dataAttr names
    int count = MD_num_entries (dataAttrSection);

    // Return this count
    return (count);
}

// Returns index of dataAttrTag, NULL_INT if not found.
int UIManager::dataAttrIndex (const char *dataAttrTag)
{
    // Get index using internal helper routine and return it
    int index = getFieldInt(dataAttrSection, dataAttrTag, 
			    dataAttrIndexDecl, 0);
    return (index);
}


// Returns dataAttrTag at index, NULL_QSTRING if out of bounds
QString UIManager::dataAttrAt (int index)
{
    QString *dataAttrPtr = 
	(QString *)INT_find_symbol_data(dataAttrMap, index);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (dataAttrPtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*dataAttrPtr);
}

// Returns dataAttrText for dataAttrTag, NULL_QSTRING if dataAttrTag not found
QString UIManager::dataAttrText (const char *dataAttrTag)
{
    // Get dataAttrText using internal helper routine and return it
    QString dataAttrText = getFieldQString(dataAttrSection, dataAttrTag, 
					 dataAttrTextDecl, 0);
    return (dataAttrText);
}

// Returns dataAttrToolTip for dataAttrTag, 
// NULL_QSTRING if dataAttrTag not found
QString UIManager::dataAttrToolTip (const char *dataAttrTag)
{
    // Get toolTip using internal helper routine and return it
    QString toolTip = getFieldQString(dataAttrSection, dataAttrTag, 
				      dataAttrToolTipDecl, 0);
    return (toolTip);
}


// Returns dataType for dataAttrTag, NULL_INT if dataAttrTag not found
int UIManager::dataAttrType (const char *dataAttrTag)
{
    // Get dataType using internal helper routine and return it
    int dataType = getFieldInt(dataAttrSection, dataAttrTag, 
			       dataAttrTypeDecl, 0);
    return (dataType);
}

// Returns suggestedAttrStat for dataAttrTag, AttrInvalid if dataAttrTag
// not found
UIManager::AttrStat 
UIManager::dataAttrSuggestedAttrStat (const char *dataAttrTag)
{
    // Get dataSuggestedAttrStat using internal helper routine and return it
    int dataSuggestedAttrStat = getFieldInt(dataAttrSection, dataAttrTag, 
					    dataAttrSuggestedAttrStatDecl, 0);

    if (dataSuggestedAttrStat == NULL_INT)
	return (UIManager::AttrInvalid);
    else
	return ((UIManager::AttrStat) dataSuggestedAttrStat);
}

// Returns suggestedEntryStat for dataAttrTag, EntryInvalid if dataAttrTag
// not found
UIManager::EntryStat 
UIManager::dataAttrSuggestedEntryStat (const char *dataAttrTag)
{
    // Get dataSuggestedEntryStat using internal helper routine and return it
    int dataSuggestedEntryStat = getFieldInt(dataAttrSection, dataAttrTag, 
					    dataAttrSuggestedEntryStatDecl, 0);

    if (dataSuggestedEntryStat == NULL_INT)
	return (UIManager::EntryInvalid);
    else
	return ((UIManager::EntryStat) dataSuggestedEntryStat);
}




// Internal routine to declare a data attr for a specific function
// Used by insertFunction() and declareDataAttr()
void UIManager::declareDataAttr (MD_Section *dataSection, 
				   MD_Entry *dataAttrEntry)
{
    // Get the dataAttrTag and dataType from the dataAttrEntry
    const char *dataAttrTag = dataAttrEntry->name;
    MD_Field *typeField = MD_find_field(dataAttrEntry, dataAttrTypeDecl);
    int dataType = MD_get_int (typeField, 0);

    // Create the data attr (field) declaration for this function
    MD_Field_Decl *fieldDecl = 
	MD_new_field_decl (dataSection, dataAttrTag, MD_OPTIONAL_FIELD);
    
    // Set data type for the field declaration
    switch (dataType)
    {
      case MD_INT:
	MD_require_int (fieldDecl, 0);
	break;
	
      case MD_DOUBLE:
	MD_require_double (fieldDecl, 0);
	break;
	
      case MD_STRING:
	MD_require_string (fieldDecl, 0);
	break;
	
	// Punt if we don't know what this data type means
      default:
	// Purposely point at declareDataAttr instead of this internal routine.
	TG_error ("UIManager::declareDataAttr: unsupported dataType %i!",
		  dataType);
    }
    
    // Make kleene star, since may have data point for each task/thread combo
    MD_kleene_star_requirement (fieldDecl, 0);
}


// Internal routine to declare a action attr for a specific function
// Used by insertFunction() and declareAction()
void UIManager::declareAction (MD_Section *actionSection, 
			       MD_Entry *actionEntry)
{
    // Get the actionTag and action from the actionEntry
    char *actionTag = actionEntry->name;

    // For now, declare action attr as holding INT*.
    // Will set to 1 when action activated, empty otherwise
    int dataType = MD_INT;

    // Create the action attr (field) declaration for this function
    MD_Field_Decl *fieldDecl = 
	MD_new_field_decl (actionSection, actionTag, MD_OPTIONAL_FIELD);
    
    // Set data type for the field declaration
    switch (dataType)
    {
      case MD_INT:
	MD_require_int (fieldDecl, 0);
	break;
	
      case MD_DOUBLE:
	MD_require_double (fieldDecl, 0);
	break;
	
      case MD_STRING:
	MD_require_string (fieldDecl, 0);
	break;
	
	// Punt if we don't know what this data type means
      default:
	// Purposely point at declareAction instead of this routine.
	TG_error ("UIManager::declareAction: unsupported dataType %i!",
		  dataType);
    }
    
    // Make kleene star, since may have data point for each task/thread combo
    MD_kleene_star_requirement (fieldDecl, 0);
}

// Internal routine to increment the line count for the given line and
// IntTable<int> *.  Returns the new count for line (1 if first time seen).
int UIManager::incrementLineCount (IntTable<int> &lineTable, int line)
{
    int *count_ptr;

    // Get existing pointer to int, create if doesn't exist
    if ((count_ptr = lineTable.findEntry(line)) == NULL)
    {
	// Create int initialized to 0, so get 1 after increment
	count_ptr = new int(0);
	TG_checkAlloc(count_ptr);
	lineTable.addEntry(line, count_ptr);
    }
    
    // Increment count
    *count_ptr = *count_ptr + 1;

    // Return count 
    return (*count_ptr);
}

// Creates an entry in the function's table.  Many entries may
// be associated with the same line.  For non-DPCL tools, the
// entryKey can just be the string version of the line number.
void UIManager::insertEntry (const char *funcName, const char *entryKey, int line, 
			      const char *toolTip)
{
    // Call detailed routine with missing parameters filled in
    return (insertEntry (funcName, entryKey, line, 
			 TG_IPT_invalid, TG_IPL_invalid, NULL, 
			 NULL_INT, toolTip));
}


// More detailed insertEntry that has extra information relevant 
// to instrumentation points in the code and how to visualize them.
// See tg_inst_point.h for instrumentation point location and type values.
// callIndex denotes the expected function call order, starting at 1.
void UIManager::insertEntry (const char *funcName, const char *entryKey, int line,
			     TG_InstPtType type, TG_InstPtLocation location, 
			     const char *funcCalled, int callIndex, const char *toolTip)
{
    // Make sure the function name has already been inserted
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable,
					      funcName);
    if (fi == NULL)
    {
	TG_error ("UIManager::insertEntry: function '%s' not found!",
		  funcName);
    }

    // Make sure the entryKey has not already been inserted
    if (MD_find_entry (fi->dataSection, entryKey) != NULL)
    {
//	TG_error ("UIManager::insertEntry: entry '%s' already in '%s'!",
//		  entryKey, funcName);
	// Don't complain; just return
	return;
    }

    // Create entry in this function's data table, get index
    int index;
    MD_Entry *dataEntry = newIndexedEntry (fi->dataSection, entryKey,
					   fi->indexDecl, fi->indexMap,
					   &index);

    // Create line field in entry and fill it with the line number
    MD_Field *lineField = MD_new_field (dataEntry, fi->lineDecl, 1);
    MD_set_int (lineField, 0, line);

    // Get the new entry count for this line in the file
    int lineCount = incrementLineCount (fi->lineCountTable, line);
    
    // Set entry lineIndex to count - 1, so 0 indexed
    int lineIndex = lineCount - 1;
    MD_Field *lineIndexField = MD_new_field (dataEntry, fi->lineIndexDecl, 1);
    MD_set_int (lineIndexField, 0, lineIndex);
    
    // Lookup QString pointer used indexMap so can use same pointer
    // in lineIndexMap (for efficiency)
    QString *entryNameQString = 
	(QString *) INT_find_symbol_data(fi->indexMap, index);

    // Add same QString pointer to lineIndexMap for this line and lineIndex
    fi->lineIndexMap.addEntry(line, lineIndex, entryNameQString);


    // Create type field in entry and fill it with the type 
    MD_Field *typeField = MD_new_field (dataEntry, fi->typeDecl, 1);
    MD_set_int (typeField, 0, type);

    // For now, only function call instrumentation points use the
    // location, funcCalled, and callIndex info
    if (type == TG_IPT_function_call)
    {
	// Create location field in entry and fill it with the location 
	MD_Field *locationField = MD_new_field (dataEntry, fi->locationDecl, 
						1);
	MD_set_int (locationField, 0, location);

	// Create funcCalled field in entry and fill it with the funcCalled 
	MD_Field *funcCalledField = MD_new_field (dataEntry, 
						  fi->funcCalledDecl, 1);
	MD_set_string (funcCalledField, 0, funcCalled);

	// Create callIndex field in entry and fill it with the callIndex 
	MD_Field *callIndexField = MD_new_field (dataEntry, fi->callIndexDecl, 
						1);
	MD_set_int (callIndexField, 0, callIndex);
    }


    // Create toolTip field in entry and fill it the the entry tool tip
    MD_Field *toolTipField = MD_new_field (dataEntry, fi->toolTipDecl, 1);
    MD_set_string (toolTipField, 0, toolTip);


    // Also create entry in this function's action table
    /* MD_Entry *actionEntry = */ 
    MD_new_entry (fi->actionSection, entryKey);


    // Emit signal to notify any listeners that an entry has been inserted
    emit entryInserted (funcName, entryKey, line, type, location, 
			funcCalled, callIndex, toolTip);
}




// Returns the number of entries for funcName, NULL_INT if not found.
int UIManager::entryCount(const char *funcName)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_INT if function doesn't exist
    if (fi == NULL)
	return (NULL_INT);

    // Get count from MD table that hold this function's entries
    int count = MD_num_entries (fi->dataSection);

    // Return this count
    return (count);
}

// Returns the number of entries for funcName:line, NULL_INT if not found.
int UIManager::entryCount(const char *funcName, int line)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_INT if function doesn't exist
    if (fi == NULL)
	return (NULL_INT);

    // Get count_ptr from lineCountTable, may be NULL
    int *count_ptr = fi->lineCountTable.findEntry(line);

    // If NULL, return NULL_INT
    if (count_ptr == NULL)
	return (NULL_INT);

    // Otherwise return count stored in pointer
    else
	return (*count_ptr);
}




// Returns index of entryKey in funcName, NULL_INT if not found.
int UIManager::entryIndex (const char *funcName, const char *entryKey)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_INT if function doesn't exist
    if (fi == NULL)
	return (NULL_INT);

    // Get index using internal helper routine and return it
    int index = getFieldInt(fi->dataSection, entryKey, fi->indexDecl, 0);
    return (index);
}


// Return lineIndex of entryKey at funcName:line, NULL_INT if not found
int UIManager::entryLineIndex (const char *funcName, const char *entryKey)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_INT if function doesn't exist
    if (fi == NULL)
	return (NULL_INT);

    // Get lineIndex using internal helper routine and return it
    int lineIndex = getFieldInt(fi->dataSection, entryKey, 
				fi->lineIndexDecl, 0);
    return (lineIndex);
}

// Returns entryKey at index for funcName, NULL_QSTRING if out of bounds
QString UIManager::entryKeyAt (const char *funcName, int index)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_QSTRING if function doesn't exist
    if (fi == NULL)
	return (NULL_QSTRING);
    
    QString *entryKeyPtr = 
	(QString *)INT_find_symbol_data(fi->indexMap, index);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (entryKeyPtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*entryKeyPtr);
}


// Returns entryKey at line and lineIndex, NULL_QSTRING if out of bounds
QString UIManager::entryKeyAt (const char *funcName, int line, int lineIndex)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_QSTRING if function doesn't exist
    if (fi == NULL)
	return (NULL_QSTRING);
    
    QString *entryKeyPtr = fi->lineIndexMap.findEntry(line, lineIndex);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (entryKeyPtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*entryKeyPtr);
}


// Returns line for entryKey in funcName, NULL_INT if out of bounds
int UIManager::entryLine (const char *funcName, const char *entryKey)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_INT if function doesn't exist
    if (fi == NULL)
	return (NULL_INT);

    // Get line using internal helper routine and return it
    int line = getFieldInt(fi->dataSection, entryKey, fi->lineDecl, 0);
    return (line);
}


// Returns inst point type for entryKey, TG_IPT_invalid if out of bounds
TG_InstPtType UIManager::entryType (const char *funcName, const char *entryKey)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return TG_IPT_invalid if function doesn't exist
    if (fi == NULL)
	return (TG_IPT_invalid);

    // Get type using internal helper routine 
    int type = getFieldInt(fi->dataSection, entryKey, fi->typeDecl, 0);

    // If NULL_INT, return TG_IPT_invalid
    if (type == NULL_INT)
	return (TG_IPT_invalid);

    // Otherwise, cast to appropriate type and return it
    else
	return ((TG_InstPtType) type);
}

// Returns location for type TG_IPT_function_call, otherwise TG_IPL_invalid
TG_InstPtLocation UIManager::entryLocation (const char *funcName, const char *entryKey)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return TG_IPLinvalid if function doesn't exist
    if (fi == NULL)
	return (TG_IPL_invalid);

    // Get type using internal helper routine 
    // Assume insertEntry stores location on for type TG_IPT_function_call
    int location = getFieldInt(fi->dataSection, entryKey, fi->locationDecl, 0);

    // If NULL_INT, return TG_IPLinvalid
    if (location == NULL_INT)
	return (TG_IPL_invalid);

    // Otherwise, cast to appropriate type and return it
    else
	return ((TG_InstPtLocation) location);
}



// Returns funcCalled for type TG_IPT_function_call, otherwise NULL_QSTRING
QString UIManager::entryFuncCalled (const char *funcName, const char *entryKey)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_QSTRING if function doesn't exist
    if (fi == NULL)
	return (NULL_QSTRING);

    // Get toolTip using internal helper routine and return it
    // Assume insertEntry only sets field for type TG_IPT_function_call
    QString funcCalled = getFieldQString(fi->dataSection, entryKey, 
					 fi->funcCalledDecl, 0);
    return (funcCalled);
}


// Returns callIndex for type TG_IPT_function_call, otherwise NULL_INT
int UIManager::entryCallIndex (const char *funcName, const char *entryKey)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_INT if function doesn't exist
    if (fi == NULL)
	return (NULL_INT);

    // Get line using internal helper routine and return it
    // Assume insertEntry only sets this field for type TG_IPT_function_call
    int callIndex = getFieldInt(fi->dataSection, entryKey, 
				fi->callIndexDecl, 0);
    return (callIndex);
}




// Returns toolTip for entryKey in funcName, NULL_QSTRING if out of bounds
QString UIManager::entryToolTip (const char *funcName, const char *entryKey)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable, funcName);

    // Return NULL_QSTRING if function doesn't exist
    if (fi == NULL)
	return (NULL_QSTRING);

    // Get toolTip using internal helper routine and return it
    QString toolTip = getFieldQString(fi->dataSection, entryKey, 
				      fi->toolTipDecl, 0);
    return (toolTip);
}


// Internal helper routine that returns a pointer to the first character and
// to the space or terminator after the last character of a XPM token that 
// starts *at* or just after the initial value of startToken (if startToken 
// points at whitespace).
// Returns TRUE if token found or FALSE if string ends before XPM token found.
// Modifies both startToken and endToken to point at start/end of token.
// Set startToken = previously returned endToken to grab the 
// next XPM token in the string.
bool UIManager::findXPMToken (const char *&startToken, const char *&endToken)
{
    // Advance startToken past any leading whitespace.  XPM 3 manual
    // states that only valid whitespace is 'space' or 'tab', so that
    // is all we will handle here
    while ((*startToken != 0) && 
	   ((*startToken == ' ') || (*startToken == '\t')))
    {
	startToken+=1;
    }
    
    // Start out with endToken at startToken's place
    endToken = startToken;

    // If endToken points at string terminator, return FALSE now, no
    // more to do
    if (*startToken == 0)
	return (FALSE);

    // Otherwise, have token to find, find end of it by advancing endToken
    // until hit whitespace or terminator
    while ((*endToken != 0) && (*endToken != ' ') && (*endToken != '\t'))
    {
	endToken += 1;
    }
    
    // endToken should now be on terminator or white space after token
    // Return TRUE, found token
    return (TRUE);
}

// Internal routine to convert XPM token to an integer.
// Emits UIManager::declarePixmap errors on format problems
int UIManager::XPMconvertToInt (const char *pixmapName, const char *fieldName,
				const char *startToken, const char *endToken)
{
    // Make sure something to read
    if (endToken <= startToken)
    {
	TG_error ("UIManager::declarePixmap: XPM format error for '%s', "
		  "expected '%s'", pixmapName, fieldName);
    }

    char *endPtr;
    int val = strtol (startToken, &endPtr, 0);
    
    // Make sure converted everything
    if (endPtr != endToken)
    {
	TG_error ("UIManager::declarePixmap: XPM format error for "
		  "'%s' field '%s'\n"
		  "  Unable to convert '%c' to int in '%s'",
		  pixmapName, fieldName, *endPtr, startToken);
    }
    
    // Return the value read
    return (val);
}

// Internal routine that returns the size of the XPM array of strings, 
// based on my interpretation of the XPM 3 parsing rules.
// Returns NULL_INT if doesn't appear to be valid XPM array of strings.
// Pass in pixmapName for error messages use only.
int UIManager::XPMArraySize (const char *pixmapName, const char *xpm[])
{
    // From XPM 3 manual, first line is of the format
    // <width> <height> <ncolors> <cpp> [<x_hotspot> <y_hotspot>] [XPMEXT]
    // XPMEXT, if exists, specifies that a potentially unlimited number
    // of lines occur at the end, until XPMENDEXT is encountered.
    // Otherwise, lines in *xpm[] should be 1 + <ncolors> + <height>
    const char *startToken, *endToken;

    // Get the first line
    startToken = xpm[0];

    // Get the widht, height, ncolors, and cpp from the first line
    findXPMToken (startToken, endToken);
    /*int width =*/ XPMconvertToInt (pixmapName, "width", startToken, 
				     endToken);

    startToken = endToken;
    findXPMToken (startToken, endToken);
    int height = XPMconvertToInt (pixmapName, "height", startToken, endToken);

    startToken = endToken;
    findXPMToken (startToken, endToken);
    int ncolors = XPMconvertToInt (pixmapName, "ncolors", startToken, 
				   endToken);

    startToken = endToken;
    findXPMToken (startToken, endToken);
    /*int cpp =*/ XPMconvertToInt (pixmapName, "cpp", startToken, endToken);

    // Calculate the size without extensions
    int stdSize = 1 + height + ncolors;

    // Do we have the optional x_hotspot or XPMEXT (or at least someting)?
    startToken = endToken;
    bool haveXPMEXT = FALSE;
    if (findXPMToken (startToken, endToken))
    {
	// Do we have the XMPEXT token?
	if ((startToken[0] == 'X') && (startToken[1] == 'P') &&
	    (startToken[2] == 'M') && (startToken[3] == 'E') &&
	    (startToken[4] == 'X') && (startToken[5] == 'T') &&
	    ((endToken - startToken) == 6))
	{
	    // Indicate that we do have XPMEXT
	    haveXPMEXT = TRUE;
	}
	// Otherwise, read and throw away x_hotspot and y_hotspot
	else
	{
	    /*int x_hotspot =*/ XPMconvertToInt (pixmapName, "x_hotspot", 
						 startToken, endToken);
	    
	    startToken = endToken;
	    findXPMToken (startToken, endToken);
	    /*int y_hotspot = */ XPMconvertToInt (pixmapName, "y_hotspot", 
						  startToken, endToken);

	    // Do we now have XPMEXT?
	    startToken = endToken;
	    if (findXPMToken (startToken, endToken))
	    {
		// Do we have the XMPEXT token?
		if ((startToken[0] == 'X') && (startToken[1] == 'P') &&
		    (startToken[2] == 'M') && (startToken[3] == 'E') &&
		    (startToken[4] == 'X') && (startToken[5] == 'T') &&
		    ((endToken - startToken) == 6))
		{
		    // Indicate that we do have XPMEXT
		    haveXPMEXT = TRUE;
		}
		// Otherwise, format error
		else
		{
		    TG_error ("UIManager::declarePixmap: "
			      "XPM format error in '%s'.\n"
			      "Expected optional XPMEXT not '%s'",
			      startToken);
		}
	    }
	}

	// Better not have anything else!
	startToken = endToken;
	if (findXPMToken (startToken, endToken))
	{
	    TG_error ("UIManager::declarePixmap: "
		      "XPM format error in '%s'.\n"
		      "Unexpected token '%s'", startToken);
	}
    }
    
    // Handle the case where have XPMEXT
    int extSize = 0;
    if (haveXPMEXT)
    {
#if 0
	// DEBUG
	printf ("XPM Pixmap '%s' has XPMEXT flag!\n", pixmapName);
#endif

	// Goto the first line after the std XPM
	startToken = xpm[stdSize];
	findXPMToken (startToken, endToken);

	// Count the lines int the extension until hit XPMENDEXT
	while ((startToken[0] != 'X') && (startToken[1] != 'P') && 
	       (startToken [2] != 'M') && (startToken[3] != 'X') && 
	       (startToken[4] != 'P') && (startToken [5] != 'M') &&
	       (startToken[6] != 'X') && (startToken[7] != 'P') && 
	       (startToken [8] != 'M'))
	{
	    // Increment line count and test the next line
	    extSize++;
	    startToken = xpm[stdSize+extSize];
	    findXPMToken (startToken, endToken);
	}
	// Count the last line with XPMENDEXT
	extSize++;
    }

    // Return stdSize plus the extension size (if any)
    return (stdSize + extSize);
}

// Declares a pixmap of name pixmapName from XPM 3 definition, which
// must be a valid XPM 3 image (for now, only minimal checking done but
// invalid formats could cause memory read errors).  
// See http://koala.ilog.fr/lehors/xpm.html for the format details, 
// tools for converting from earlier versions, etc.
void UIManager::declarePixmap (const char *pixmapName, const char *xpm[])
{
    // Get xpm array size, will punt on header format errors
    // with the error message indicating that it is from declarePixmap.
    int xpmArraySize = XPMArraySize(pixmapName, xpm);

#if 0    
    // Print out the xpm array to see if we got it all
    printf ("declarePixmap: '%s' xpm pixmap %i lines long\n", 
	    pixmapName, xpmArraySize);
    
    for (int i = 0; i < xpmArraySize; i++)
    {
	printf ("%i: '%s'\n", i, xpm[i]);
    }
    printf ("\n");
#endif

    // Make sure pixmapName not already in use
    if (MD_find_entry (pixmapSection, pixmapName) != NULL)
    {
	TG_error ("UIManager::declarePixmap: pixmap '%s' already "
		  "declared!", pixmapName);
    }
    
    // Add pixmap to section
    MD_Entry *pixmapEntry = newIndexedEntry (pixmapSection, pixmapName,
					  pixmapIndexDecl, pixmapMap);

    // Add xpmArray field and fill it with the xpm array contents
    MD_Field *xpmArrayField = MD_new_field (pixmapEntry, xpmArrayDecl, 1);

    // Also create internal copy of xmp array with pointers to strings
    // in the xpmArrayField.  NULL terminate, just for sanity
    char **internalXpm = new char *[xpmArraySize+1];
    TG_checkAlloc(internalXpm);

    // Go thru the array, adding the string there to to xmpArrayField
    for (int i = 0; i < xpmArraySize; i++)
    {
	MD_set_string (xpmArrayField, i, (char *) xpm[i]);
	internalXpm[i] = MD_get_string (xpmArrayField, i);
    }

    // NULL terminate for sanity sake (not necessary in xpm format)
    internalXpm[xpmArraySize] = NULL;

    // Create pixmapInfo structure with this internalXpm and a new
    // QPixmap created from it
    PixmapInfo *pixmapInfo = new PixmapInfo((const char **)internalXpm, 
					  QPixmap((const char **)internalXpm));
    TG_checkAlloc(pixmapInfo);

    // Add it to pixmap table under pixmap name, it will take care
    // of deleting everything created above
    pixmapInfoTable.addEntry (pixmapName, pixmapInfo);
    
    // Emit signal to notify any listeners that a pixmap has been declared
    emit pixmapDeclared ((char *)pixmapName, xpm);
}


// Returns number of pixmaps currently declared
int UIManager::pixmapCount()
{
    // Get count from MD table that holds inserted pixmaps
    int count = MD_num_entries (pixmapSection);

    // Return this count
    return (count);
}

// Returns the index of pixmapName, NULL_INT if pixmapName not found
int UIManager::pixmapIndex (const char *pixmapName)
{
    // Get index using internal helper routine and return it
    int index = getFieldInt(pixmapSection, pixmapName, 
			    pixmapIndexDecl, 0);
    return (index);
}

// Returns pixmapName at index, NULL_QSTRING if not found
QString UIManager::pixmapAt (int index)
{
    QString *pixmapNamePtr = 
	(QString *)INT_find_symbol_data(pixmapMap, index);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (pixmapNamePtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*pixmapNamePtr);
}

// Returns the *xpm[] (**xpm) for pixmapName, or NULL if not found
const char **UIManager::pixmapXpm (const char *pixmapName)
{
    // Get pixmapInfo for this pixmapName,  if exists
    PixmapInfo *pixmapInfo = pixmapInfoTable.findEntry (pixmapName);
    
    // Return NULL if pixmapInfo not found
    if (pixmapInfo == NULL)
	return (NULL);

    // Otherwise, return pointer to internal copy of xpm
    else
	return ((const char **)pixmapInfo->xpm);
}


// Returns the QPixmap constructed from the *xpm[] for pixmapName.
// Returns NULL QPixmap (see isNULL() call) if not found.
// Since QPixmap uses lazy copying, using these QPixmaps may
// save memory, since all windows can share data.
const QPixmap &UIManager::pixmapQPixmap (const char *pixmapName)
{
    // Get pixmapInfo for this pixmapName,  if exists
    PixmapInfo *pixmapInfo = pixmapInfoTable.findEntry (pixmapName);
    
    // Return NULL QPixmap if pixmapInfo not found
    if (pixmapInfo == NULL)
	return (nullQPixmap);

    // Otherwise, return internal qpixmap version of pixmap
    else
	return (pixmapInfo->qpixmap);
}


// Declares a type of action, and its initial state, that may be 
// performed on a function entry.  Has same arguments and
// effect of declareActionState() with two extra features:
// 1) It declares the actionTag, so must happen before declareActionState
// calls for this actionTag.  2) It defines the initialState
// for this actionTag.  
void UIManager::declareAction (const char *actionTag, const char *initialStateTag,
			       const char *toStatePixmapTag, 
			       const char *toStateMenuText, const char *toStateToolTip,
			       const char *inStatePixmapTag, const char *inStateToolTip,
			       bool enableTransitionToStateByDefault)
{
    // Make sure actionTag not already in use
    if (MD_find_entry (actionSection, actionTag) != NULL)
    {
	TG_error ("UIManager::declareAction: action '%s' already "
		  "declared!", actionTag);
    }
    
    // Add action tag to section
    // Will also use the automatic _index_ field value as the index
    // into the _actions_ field to place the action link. 
    MD_Entry *actionEntry = newIndexedEntry (actionSection, actionTag,
					     actionIndexDecl,
					     actionMap);

    // Add initialState field and fill it with initialState
    MD_Field *initialStateField = MD_new_field (actionEntry, 
						actionInitialStateDecl, 1);
    MD_set_string (initialStateField, 0, initialStateTag);

    // Add this action field to all existing function action sections (those
    // beginning with A_
    for (MD_Section *funcSection = MD_first_section (md); 
	 funcSection != NULL; funcSection = MD_next_section (funcSection))
    {
	// Skip non-function sections (those that don't begin with A_)
	if ((funcSection->name[0] != 'A') || (funcSection->name[1] != '_'))
	    continue;

	// Use private function to add action field decl, so added the same 
	// way by this routine and insertFunction();
	declareAction (funcSection, actionEntry);
    }

    // Create 'action state' section for this action.  Prefix 
    // actionTag name for this section with 'S_' to prevent name conflicts.
    QString actionStateSectionName;
    actionStateSectionName.sprintf ("S_%s", actionTag);
    MD_Field_Decl *indexDecl;
    INT_Symbol_Table *indexMap;
    MD_Section *actionStateSection =
        newIndexedSection (actionStateSectionName.latin1(), &indexDecl,
                           &indexMap);

    // Add optional LINK(_pixmap_) field to hold toStatePixmap
    MD_Field_Decl *toStatePixmapDecl = 
	MD_new_field_decl (actionStateSection, "toStatePixmap",
			   MD_OPTIONAL_FIELD);
    MD_require_link (toStatePixmapDecl, 0, pixmapSection);

    // Add STRING field to hold toStateMenuText
    MD_Field_Decl *toStateMenuTextDecl = 
	MD_new_field_decl (actionStateSection, "toStateMenuText",
			   MD_REQUIRED_FIELD);
    MD_require_string (toStateMenuTextDecl, 0);

    // Add STRING field to hold toStateToolTip
    MD_Field_Decl *toStateToolTipDecl = 
	MD_new_field_decl (actionStateSection, "toStateToolTip",
			   MD_REQUIRED_FIELD);
    MD_require_string (toStateToolTipDecl, 0);

    // Add optional LINK(_pixmap_) field to hold inStatePixmap
    MD_Field_Decl *inStatePixmapDecl = 
	MD_new_field_decl (actionStateSection, "inStatePixmap",
			   MD_OPTIONAL_FIELD);
    MD_require_link (inStatePixmapDecl, 0, pixmapSection);

    // Add STRING field to hold inStateToolTip
    MD_Field_Decl *inStateToolTipDecl = 
	MD_new_field_decl (actionStateSection, "inStateToolTip",
			   MD_REQUIRED_FIELD);
    MD_require_string (inStateToolTipDecl, 0);


    // Add INT field to hold enableTransitionToStateByDefault
    MD_Field_Decl *connectNewStatesToDecl = 
	MD_new_field_decl (actionStateSection, "connectNewStatesTo",
			   MD_REQUIRED_FIELD);
    MD_require_int (connectNewStatesToDecl, 0);

    // Add LINK (to this section) for states that can transition to
    MD_Field_Decl *canTransitionToDecl = 
	MD_new_field_decl (actionStateSection, "canTransitionTo",
			   MD_REQUIRED_FIELD);
    MD_require_link (canTransitionToDecl, 0, actionStateSection);
    MD_kleene_star_requirement (canTransitionToDecl, 0);

    // To Speed up lookup of action state sections (don't want to have
    // to prefix it every time), store ActionInfo structure with the
    // actionStateSection pointer, the entry for this action in the
    // action's section, and all the field decl pointers for state entries
    // in actionStateSection in a table that can be looked up with 
    // the actionTag
    actionInfoTable.addEntry(actionTag,
			     new ActionInfo (actionStateSection,
					     indexDecl,
					     indexMap,
					     actionEntry,
					     toStatePixmapDecl,
					     toStateMenuTextDecl,
					     toStateToolTipDecl,
					     inStatePixmapDecl,
					     inStateToolTipDecl,
					     connectNewStatesToDecl,
					     canTransitionToDecl));

    // Declare the initial state before emitting signal
    // declareAction detects that it is defining the initialState and
    // it doesn't emit a signal for actionStateDeclared so that it
    // doesn't confuse listeners.
    declareActionState (actionTag, initialStateTag, toStatePixmapTag, 
			toStateMenuText, toStateToolTip, inStatePixmapTag, 
			inStateToolTip, enableTransitionToStateByDefault);
    
    // Emit signal to notify any listeners that a action type,
    // and its default state, has been declared
    emit actionDeclared (actionTag, initialStateTag,
			 toStatePixmapTag, toStateMenuText, 
			 toStateToolTip, inStatePixmapTag, 
			 inStateToolTip, 
			 enableTransitionToStateByDefault);
}


// Returns number of action attrs currently declared
int UIManager::actionCount()
{
    // Get count from MD table that holds inserted action types
    int count = MD_num_entries (actionSection);

    // Return this count
    return (count);
}

// Returns the index of actionTag, NULL_INT if not found
int UIManager::actionIndex(const char *actionTag)
{
    // Get index using internal helper routine and return it
    int index = getFieldInt(actionSection, actionTag, 
			    actionIndexDecl, 0);
    return (index);
}


// Returns actionTag at index in the action list, NULL_QSTRING if not found
QString UIManager::actionAt(int index)
{
    QString *actionTagPtr = 
	(QString *)INT_find_symbol_data(actionMap, index);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (actionTagPtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*actionTagPtr);
}

// Returns the initialState for actionTag,  NULL_QSTRING if not found
QString UIManager::actionInitialState(const char *actionTag)
{
    // Get initialStateTag using internal helper routine and return it
    QString initialStateTag = getFieldQString(actionSection, 
					      actionTag, 
					      actionInitialStateDecl, 
					      0);
    return (initialStateTag);
}

// Declares and describes a state an action may be in and the pixmap,
// Menu text, and ToolTip text used to indicate when a user can 
// transition to this state, and the pixmap and toolTip to present
// to the user when in the state.  If 'enableTransitionToStateByDefault'
// is TRUE, all states (other than itself) will allow user transition
// to this state, by default (unless disableActionStateTransition is 
// called). 
// If pixmap tags are NULL or "", no pixmap will be displayed.
// Will punt if action not declared or if state already declared.
// Note: the tool may transition from any state to any state 
// it wants to, this only specifies what the user may do from the UI.
void UIManager::declareActionState (const char *actionTag, const char *stateTag,
				    const char *toStatePixmapTag, 
				    const char *toStateMenuText, 
				    const char *toStateToolTip,
				    const char *inStatePixmapTag, 
				    const char *inStateToolTip,
				    bool enableTransitionToStateByDefault)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Punt if action is not declared
    if (actionInfo == NULL)
    {
        TG_error ("UIManager::declareActionState: actionTag '%s' not "
		  "declared!  Declare first!", actionTag);
    }

    // Make sure state is not already declared, punt if it is (for now)
    if (MD_find_entry (actionInfo->actionStateSection, stateTag))
    {
        TG_error ("UIManager::declareActionState: state '%s' for action '%s'"
		  "already declared!", stateTag, actionTag);
    }
    
    // Create new state entry
    MD_Entry *stateEntry = newIndexedEntry(actionInfo->actionStateSection,
					   stateTag, actionInfo->indexDecl,
					   actionInfo->indexMap, NULL);

    // If toStatePixmapTag not NULL or "", find and link to pixmap in field
    if ((toStatePixmapTag != NULL) && (toStatePixmapTag[0] != 0))
    {
	// Get entry field for pixmap
	MD_Entry *toPixmapEntry = 
	    MD_find_entry (pixmapSection, toStatePixmapTag);

	// Punt if not defined
	if (toPixmapEntry == NULL)
	{
	    TG_error ("UIManager::declareActionState: toStatePixmapTag '%s' "
		      "not defined for state '%s' and action '%s'",
		      toStatePixmapTag, stateTag, actionTag);
	}
	
	// Create toStatePixmap field
	MD_Field *toStatePixmapField = 
	    MD_new_field (stateEntry, actionInfo->toStatePixmapDecl, 0);

	// Set pixmap link
	MD_set_link (toStatePixmapField, 0, toPixmapEntry);
    }
    
    // Create field for and set 'toState' Menu text
    MD_Field *toStateMenuTextField = 
	MD_new_field (stateEntry, actionInfo->toStateMenuTextDecl, 0);
    MD_set_string (toStateMenuTextField, 0, toStateMenuText);

    // Create field for and set 'toState' ToolTip text
    MD_Field *toStateToolTipField = 
	MD_new_field (stateEntry, actionInfo->toStateToolTipDecl, 0);
    MD_set_string (toStateToolTipField, 0, toStateToolTip);

    // If inStatePixmapTag not NULL or "", find and link to pixmap in field
    if ((inStatePixmapTag != NULL) && (inStatePixmapTag[0] != 0))
    {
	// Get entry field for pixmap
	MD_Entry *inPixmapEntry = 
	    MD_find_entry (pixmapSection, inStatePixmapTag);

	// Punt if not defined
	if (inPixmapEntry == NULL)
	{
	    TG_error ("UIManager::declareActionState: inStatePixmapTag '%s' "
		      "not defined for state '%s' and action '%s'",
		      inStatePixmapTag, stateTag, actionTag);
	}
	
	// Create inStatePixmap field
	MD_Field *inStatePixmapField = 
	    MD_new_field (stateEntry, actionInfo->inStatePixmapDecl, 0);

	// Set pixmap link
	MD_set_link (inStatePixmapField, 0, inPixmapEntry);
    }
    
    // Create field for and set 'inState' ToolTip text
    MD_Field *inStateToolTipField = 
	MD_new_field (stateEntry, actionInfo->inStateToolTipDecl, 0);
    MD_set_string (inStateToolTipField, 0, inStateToolTip);

    // Create connectNewStates field
    MD_Field *connectNewStatesToField = 
	MD_new_field (stateEntry, actionInfo->connectNewStatesToDecl, 0);

    // Set connectNewStatesToDecl to 0 or 1
    if (enableTransitionToStateByDefault)
	MD_set_int (connectNewStatesToField, 0, 1);
    else
	MD_set_int (connectNewStatesToField, 0, 0);

    // Create canTransitionTo field (empty for now)
    /*MD_Field *canTransitionToField = */
	MD_new_field (stateEntry, actionInfo->canTransitionToDecl, 0);

    // Scan all existing states, may need to connect to this state
    // and may need to connect this state to it
    for (MD_Entry *scanEntry = MD_first_entry(actionInfo->actionStateSection);
	 scanEntry != NULL; scanEntry = MD_next_entry(scanEntry))
    {
	// Skip entry just added, don't want to connect to self
	if (scanEntry == stateEntry)
	    continue;

	// Is scanEntry is connected to by default?
	MD_Field *scanDefaultToField = 
	    MD_find_field (scanEntry, actionInfo->connectNewStatesToDecl);
	int scanDefaultTo = MD_get_int (scanDefaultToField, 0);

	// If so, enable transition from the new entry (stateEntry) to it
	if (scanDefaultTo)
	{
	    enableActionStateTransition (actionTag, stateTag, 
					 scanEntry->name);
	}

	
	// If stateTag is connected to default, enable transition from
	// the scan entry to the new entry
	if (enableTransitionToStateByDefault)
	{
	    enableActionStateTransition (actionTag, scanEntry->name,
					 stateTag);
	}
    }

    // Only emit signal that action has been declared states after
    // the first (default) state.  declareAction declares this first
    // state using this routine and emiting a signal here would cause 
    // listener confusion (that is a state being declared before the
    // action is declared)!
    if (MD_num_entries(actionInfo->actionStateSection) > 1)
    {
	// Emit signal that action state has been declared
	emit actionStateDeclared (actionTag, stateTag,
				  toStatePixmapTag, toStateMenuText, 
				  toStateToolTip, inStatePixmapTag, 
				  inStateToolTip, 
				  enableTransitionToStateByDefault);
    }
}


// Returns number of states currently declared for actionTag,
// NULL_INT if not found.
int UIManager::actionStateCount(const char *actionTag)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Punt if action is not declared
    if (actionInfo == NULL)
	return (NULL_INT);

    // One entry per state in actionStateSection
    int stateCount = MD_num_entries (actionInfo->actionStateSection);

    return (stateCount);
}


// Returns the index of stateTag for actionTag, NULL_INT if not found
int UIManager::actionStateIndex(const char *actionTag, const char *stateTag)
{

    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Return NULL_INT if action is not declared
    if (actionInfo == NULL)
	return (NULL_INT);

    // Get index using internal helper routine and return it
    int index = getFieldInt(actionInfo->actionStateSection, stateTag,
                            actionInfo->indexDecl, 0);
    return (index);
}


// Returns stateTag for actionTag at index, NULL_QSTRING if not found
QString UIManager::actionStateAt (const char *actionTag, int index)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Return NULL_QSTRING if action is not declared
    if (actionInfo == NULL)
	return (NULL_QSTRING);
    
    // Get QString version of state name (for efficiency) from indexMap
    QString *stateTagPtr = 
	(QString *)INT_find_symbol_data (actionInfo->indexMap, index);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (stateTagPtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*stateTagPtr);
}


// Returns pixmapTag for 'to state' transition, NULL_QSTRING if not found
QString UIManager::actionStateToPixmapTag(const char *actionTag, const char *stateTag)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Return NULL_QSTRING if action is not declared
    if (actionInfo == NULL)
	return (NULL_QSTRING);

    // Get entry for stateTag
    MD_Entry *stateEntry = MD_find_entry (actionInfo->actionStateSection,
					  stateTag);

    // Return NULL_QSTRING if stateTag is not declared
    if (stateEntry == NULL)
	return (NULL_QSTRING);

    // Get field for toPixmap, may not be there!
    MD_Field *toPixmapField = MD_find_field (stateEntry, 
					     actionInfo->toStatePixmapDecl);

    // If no pixmap specified, return NULL_QSTRING
    if (toPixmapField == NULL)
	return (NULL_QSTRING);
	
    // Get the link to the pixmap 
    MD_Entry *pixmapEntry = MD_get_link (toPixmapField, 0);

    // Return pixmapEntry name 
    return (pixmapEntry->name);
}


// Returns menuText for 'to state' transition, NULL_QSTRING if not found
QString UIManager::actionStateToMenuText(const char *actionTag, const char *stateTag)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Return NULL_QSTRING if action is not declared
    if (actionInfo == NULL)
	return (NULL_QSTRING);

    // Get entry for stateTag
    MD_Entry *stateEntry = MD_find_entry (actionInfo->actionStateSection,
					  stateTag);

    // Return NULL_QSTRING if stateTag is not declared
    if (stateEntry == NULL)
	return (NULL_QSTRING);

    // Get field for toStateMenuText, must be there
    MD_Field *toStateMenuTextField = MD_find_field (stateEntry, 
				          actionInfo->toStateMenuTextDecl);
    // Return toStateMenuText string in that field
    return (MD_get_string (toStateMenuTextField, 0));
}


// Returns toolTip for 'to state' transition, NULL_QSTRING if not found
QString UIManager::actionStateToToolTip(const char *actionTag, const char *stateTag)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Return NULL_QSTRING if action is not declared
    if (actionInfo == NULL)
	return (NULL_QSTRING);

    // Get entry for stateTag
    MD_Entry *stateEntry = MD_find_entry (actionInfo->actionStateSection,
					  stateTag);

    // Return NULL_QSTRING if stateTag is not declared
    if (stateEntry == NULL)
	return (NULL_QSTRING);

    // Get field for toStateToolTip, must be there
    MD_Field *toStateToolTipField = MD_find_field (stateEntry, 
				          actionInfo->toStateToolTipDecl);
    // Return toStateToolTip string in that field
    return (MD_get_string (toStateToolTipField, 0));
}


// Returns pixmapTag for when action 'in the state', 
// NULL_QSTRING if not found
QString UIManager::actionStateInPixmapTag(const char *actionTag, const char *stateTag)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Return NULL_QSTRING if action is not declared
    if (actionInfo == NULL)
	return (NULL_QSTRING);

    // Get entry for stateTag
    MD_Entry *stateEntry = MD_find_entry (actionInfo->actionStateSection,
					  stateTag);

    // Return NULL_QSTRING if stateTag is not declared
    if (stateEntry == NULL)
	return (NULL_QSTRING);

    // Get field for inPixmap, may not be there!
    MD_Field *inPixmapField = MD_find_field (stateEntry, 
					     actionInfo->inStatePixmapDecl);

    // If no pixmap specified, return NULL_QSTRING
    if (inPixmapField == NULL)
	return (NULL_QSTRING);
	
    // Get the link to the pixmap 
    MD_Entry *pixmapEntry = MD_get_link (inPixmapField, 0);

    // Return pixmapEntry name 
    return (pixmapEntry->name);
}

// Returns toolTip for when action 'in the state', 
// NULL_QSTRING if not found
QString UIManager::actionStateInToolTip(const char *actionTag, const char *stateTag)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Return NULL_QSTRING if action is not declared
    if (actionInfo == NULL)
	return (NULL_QSTRING);

    // Get entry for stateTag
    MD_Entry *stateEntry = MD_find_entry (actionInfo->actionStateSection,
					  stateTag);

    // Return NULL_QSTRING if stateTag is not declared
    if (stateEntry == NULL)
	return (NULL_QSTRING);

    // Get field for inStateToolTip, must be there
    MD_Field *inStateToolTipField = MD_find_field (stateEntry, 
				          actionInfo->inStateToolTipDecl);
    // Return inStateToolTip string in that field
    return (MD_get_string (inStateToolTipField, 0));
}


// Returns TRUE if all state transitions are enabled by default, FALSE
// if not.  Punts on invalid args.
// Note: the tool may transition from any state to any state 
// it wants to, this only specifies what the user may do from the UI.
bool UIManager::actionStateToTransitionDefault(const char *actionTag, 
						  const char *stateTag)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Punt if action is not declared
    if (actionInfo == NULL)
    {
        TG_error ("UIManager::actionStateToTransitionDefault: actionTag '%s'"
		  " not declared!", actionTag);
    }

    // Get stateEntry for stateTag
    MD_Entry *stateEntry = MD_find_entry (actionInfo->actionStateSection, 
					  stateTag);
    if (stateEntry == NULL)
    {
        TG_error ("UIManager::actionStateToTransitionDefault: state '%s' for "
		  "action '%s' not declared!", stateTag, actionTag);
    }

    // Get connectNewStatesToDecl field for stateEntry
    MD_Field *connectNewStatesToField = MD_find_field (stateEntry, 
				      actionInfo->connectNewStatesToDecl);

    // Get 1 or 0 from field
    int connectNewStates = MD_get_int (connectNewStatesToField, 0);

    // Return TRUE if 1, 0 otherwise
    if (connectNewStates)
	return (TRUE);
    else
	return (FALSE);
}


// Enables user to transition from 'fromStateTag' to 'toStateTag' for
// the specified actionTag.
// Ignores call if transition already enabled, punts on invalid args.
// Note: the tool may transition from any state to any state 
// it wants to, this only specifies what the user may do from the UI.
void UIManager::enableActionStateTransition (const char *actionTag, 
					     const char *fromStateTag,
					     const char *toStateTag)
{
#if 0
    // DEBUG
    printf ("enableActionStateTransition (%s, %s, %s)\n", actionTag, 
	    fromStateTag, toStateTag);
#endif

    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Punt if action is not declared
    if (actionInfo == NULL)
    {
        TG_error ("UIManager::enableActionStateTransition: actionTag '%s' "
		  "not declared!  Declare first!", actionTag);
    }

    // Get fromStateEntry for fromStateTag
    MD_Entry *fromStateEntry = MD_find_entry (actionInfo->actionStateSection, 
					      fromStateTag);
    if (fromStateEntry == NULL)
    {
        TG_error ("UIManager::enableActionStateTransition: fromState '%s' for "
		  "action '%s' not declared!", fromStateTag, actionTag);
    }

    // Get toStateEntry for toStateTag
    MD_Entry *toStateEntry = MD_find_entry (actionInfo->actionStateSection, 
					    toStateTag);
    if (toStateEntry == NULL)
    {
        TG_error ("UIManager::enableActionStateTransition: toState '%s' for "
		  "action '%s' not declared!", toStateTag, actionTag);
    }

    // Get canTransitionTo field for fromStateEntry
    MD_Field *canTransitionToField = MD_find_field (fromStateEntry, 
					     actionInfo->canTransitionToDecl);
    
    // How many transitions are already allowed?
    int numTransitions = MD_num_elements (canTransitionToField);

    // Scan existing transitions to make sure not already there
    bool alreadyExists = FALSE;
    for (int index = 0; index < numTransitions; index++)
    {
	MD_Entry *scanEntry = MD_get_link (canTransitionToField, index);

	// Do we already have the toStateEntry?
	if (scanEntry == toStateEntry)
	{
	    // Yes, stop search
	    alreadyExists = TRUE;
	    break;
	}
    }

    // If already in there, ignore this request, otherwise
    // add to end of this and emit signal.
    if (!alreadyExists)
    {
	// Add entry to end of list
	MD_set_link (canTransitionToField, numTransitions, toStateEntry);

	// Signal that enabled this transition
	emit actionStateTransitionEnabled (actionTag, fromStateTag,
					   toStateTag);
    }
}

// Prevents user to transition from 'fromStateTag' to 'toStateTag' for
// the specifed actionTag.
// Ignores call if transition already disabled, punts on invalid args.
// Note: the tool may transition from any state to any state 
// it wants to, this only specifies what the user may do from the UI.
void UIManager::disableActionStateTransition (const char *actionTag, 
					      const char *fromStateTag,
					      const char *toStateTag)
{
#if 0
    // DEBUG
    printf ("disableActionStateTransition (%s, %s, %s)\n", actionTag, 
	    fromStateTag, toStateTag);
#endif

    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Punt if action is not declared
    if (actionInfo == NULL)
    {
        TG_error ("UIManager::disableActionStateTransition: actionTag '%s' "
		  "not declared!  Declare first!", actionTag);
    }

    // Get fromStateEntry for fromStateTag
    MD_Entry *fromStateEntry = MD_find_entry (actionInfo->actionStateSection, 
					      fromStateTag);
    if (fromStateEntry == NULL)
    {
        TG_error ("UIManager::disableActionStateTransition: fromState '%s' "
		  "for action '%s' not declared!", fromStateTag, actionTag);
    }

    // Get toStateEntry for toStateTag
    MD_Entry *toStateEntry = MD_find_entry (actionInfo->actionStateSection, 
					    toStateTag);
    if (toStateEntry == NULL)
    {
        TG_error ("UIManager::disableActionStateTransition: toState '%s' for "
		  "action '%s' not declared!", toStateTag, actionTag);
    }

    // Get canTransitionTo field for fromStateEntry
    MD_Field *canTransitionToField = MD_find_field (fromStateEntry, 
					     actionInfo->canTransitionToDecl);
    
    // How many transitions are already allowed?
    int numTransitions = MD_num_elements (canTransitionToField);

    // Scan existing transitions to make sure there
    bool currentlyEnabled = FALSE;
    int targetIndex = -1;
    for (int index = 0; index < numTransitions; index++)
    {
	MD_Entry *scanEntry = MD_get_link (canTransitionToField, index);

	// Do we already have the toStateEntry?
	if (scanEntry == toStateEntry)
	{
	    // Yes, stop search
	    currentlyEnabled = TRUE;

	    // Indicate the target to delete
	    targetIndex = index;
	    break;
	}
    }

    // If not enabled, ignore this request, otherwise
    // add to end of this and emit signal.
    if (currentlyEnabled)
    {
	// Move current links down to fill space
	for (int index = targetIndex + 1; index < numTransitions;
	     index++)
	{
	    MD_Entry *moveEntry = MD_get_link (canTransitionToField, index);
	    MD_set_link (canTransitionToField, index-1, moveEntry);

	}

	// Delete the last one, now a duplicate
	MD_delete_element (canTransitionToField, numTransitions-1);

	// Signal that disabled this transistion
	emit actionStateTransitionDisabled (actionTag, fromStateTag,
					    toStateTag);
    }
}


// Returns number of states that may be transitioned to 
// from the state 'fromStateTag'.  Note, may return 0, user cannot
// change out of this state.  Returns NULL_INT on invalid args.
int UIManager::actionStateTransitionCount (const char *actionTag, 
					   const char *fromStateTag)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Punt if action is not declared
    if (actionInfo == NULL)
	return (NULL_INT);

    // Get fromStateEntry for fromStateTag
    MD_Entry *fromStateEntry = MD_find_entry (actionInfo->actionStateSection, 
					      fromStateTag);
    if (fromStateEntry == NULL)
	return (NULL_INT);

    // Get canTransitionTo field for fromStateEntry
    MD_Field *canTransitionToField = MD_find_field (fromStateEntry, 
					     actionInfo->canTransitionToDecl);
    
    // How many transitions are currently allowed?
    int numTransitions = MD_num_elements (canTransitionToField);

    // Return this count
    return (numTransitions);
}


// Returns the state at 'index' that the user may transition to, from
// the state 'fromStateTag'.  Returns in the order the transitions
// were enabled, either thru the declareActionState(..., TRUE) or
// thru enableActionStateTransition.  This should give the tool writer
// some control of the order state transitions are presented to the user.
// Punts on invalid actionTag and fromStateTag.  Returns NULL_QSTRING
// for indexes out of bounds.
QString UIManager::actionStateTransitionAt (const char *actionTag, 
					    const char *fromStateTag,
					    int index)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Punt if action is not declared
    if (actionInfo == NULL)
    {
        TG_error ("UIManager::actionStateTransitionAt: actionTag '%s' "
		  "not declared!  Declare first!", actionTag);
    }

    // Get fromStateEntry for fromStateTag
    MD_Entry *fromStateEntry = MD_find_entry (actionInfo->actionStateSection, 
					      fromStateTag);
    if (fromStateEntry == NULL)
    {
        TG_error ("UIManager::actionStateTransitionAt: fromState '%s' for "
		  "action '%s' not declared!", fromStateTag, actionTag);
    }

    // Get canTransitionTo field for fromStateEntry
    MD_Field *canTransitionToField = MD_find_field (fromStateEntry, 
					     actionInfo->canTransitionToDecl);
    
    // How many transitions are currently allowed?
    int numTransitions = MD_num_elements (canTransitionToField);

    // Return NULL_QSTRING if out of bounds
    if ((index < 0) || (index >= numTransitions))
	return (NULL_QSTRING);

    // Get the toStateTag from field at this index, convert to QString
    MD_Entry *toEntry = MD_get_link (canTransitionToField, index);

    // Return name (toStateTag) of toEntry
    return (toEntry->name);
}


// Returns TRUE, if user may transition from 'fromStateTag' to 
// 'toStateTag'.  Returns FALSE otherwise.  Punts on invalid args.
bool UIManager::isActionStateTransitionEnabled (const char *actionTag, 
						const char *fromStateTag,
						const char *toStateTag)
{
    // Get actionInfo for action
    ActionInfo *actionInfo = actionInfoTable.findEntry(actionTag);

    // Punt if action is not declared
    if (actionInfo == NULL)
    {
        TG_error ("UIManager::isActionStateTransitionEnabled: actionTag '%s' "
		  "not declared!  Declare first!", actionTag);
    }

    // Get fromStateEntry for fromStateTag
    MD_Entry *fromStateEntry = MD_find_entry (actionInfo->actionStateSection, 
					      fromStateTag);
    if (fromStateEntry == NULL)
    {
        TG_error ("UIManager::isActionStateTransitionEnabled: fromState '%s' "
		  "for action '%s' not declared!", fromStateTag, actionTag);
    }

    // Get toStateEntry for toStateTag
    MD_Entry *toStateEntry = MD_find_entry (actionInfo->actionStateSection, 
					    toStateTag);
    if (toStateEntry == NULL)
    {
        TG_error ("UIManager::isActionStateTransitionEnabled: toState '%s' "
		  "for action '%s' not declared!", toStateTag, actionTag);
    }

    // Get canTransitionTo field for fromStateEntry
    MD_Field *canTransitionToField = MD_find_field (fromStateEntry, 
					     actionInfo->canTransitionToDecl);
    
    // How many transitions are already allowed?
    int numTransitions = MD_num_elements (canTransitionToField);

    // Scan existing transitions to see if in list
    bool transitionEnabled = FALSE;
    for (int index = 0; index < numTransitions; index++)
    {
	MD_Entry *scanEntry = MD_get_link (canTransitionToField, index);

	// Do we have toStateEntry?
	if (scanEntry == toStateEntry)
	{
	    // Yes, stop search
	    transitionEnabled = TRUE;
	    break;
	}
    }

    // Return TRUE if transition enabled, false otherwise
    return (transitionEnabled);
}


// Enables action (allows user to click on it which causes it to be
// activated/deactivated) on the entry for the specified function.
// Actions can be activated without enabling them and vice-versa!
// Actions must first be declared with declareAction().
// If action already enabled, this routine siliently does nothing.
//
// For now, actions must be enabled/disabled for all tasks/threads.
void UIManager::enableAction (const char *funcName, const char *entryKey, 
			       const char *actionTag)
{
    // Get the action type entry for the actionTag 
    MD_Entry *actionEntry = MD_find_entry (actionSection, actionTag);

    // Punt if action is not declared
    if (actionEntry == NULL)
    {
	TG_error ("UIManager::enableAction: actionTag '%s' not declared!  "
		  "Declare first!", actionTag);
    }

    // Get the index field for this entry and get the index from it
    MD_Field *indexField = MD_find_field (actionEntry, actionIndexDecl);
    int index = MD_get_int (indexField, 0);

    // Get the '_actions_' field (create if necessary) in the entry
    MD_Field *actionsField = getActionField ("UIManager::enableAction", 
					     funcName, entryKey, 
					     "_actions_", MD_LINK, TRUE);

    // If not already enabled, enable it.
    // Go through the effort of checking if already enabled so
    // that signals will only be emitted on state changes...
    if ((index > MD_max_element_index(actionsField)) ||
	(actionsField->element[index] == NULL))
    {
	MD_set_link (actionsField, index, actionEntry);

	// Emit signal to notify any listeners that an action has been enabled
	emit actionEnabled (funcName, entryKey, actionTag);
    }
}




// Disables action (no longer allow user to click on it) on the entry 
// for the specified function.
// This doesn't deactivate activated actions.
// Actions must first be declared with declareAction().
// If action already disabled, this routine siliently does nothing.
// For now, actions must be enabled/disabled for all tasks/threads.
void UIManager::disableAction (const char *funcName, const char *entryKey, 
			       const char *actionTag)
{
    // Get the action type entry for the actionTag 
    MD_Entry *actionEntry = MD_find_entry (actionSection, actionTag);

    // Punt if action is not declared
    if (actionEntry == NULL)
    {
	TG_error ("UIManager::disableAction: actionTag '%s' not declared!  "
		  "Declare first!", actionTag);
    }

    // Get the index field for this entry and get the index from it
    MD_Field *indexField = MD_find_field (actionEntry, actionIndexDecl);
    int index = MD_get_int (indexField, 0);

    // Get the '_actions_' field (don't create if absent) in the entry
    MD_Field *actionsField = getActionField ("UIManager::disableAction", 
					     funcName, entryKey, 
					     "_actions_", MD_LINK, FALSE);

    // Only if already enabled, disable it.
    // Go through the effort of checking if enabled so
    // that signals will only be emitted on state changes...
    if ((actionsField != NULL) &&
	(index <= MD_max_element_index(actionsField)) &&
	(actionsField->element[index] != NULL))
    {
	// Totally delete element holding link to action type
	MD_delete_element (actionsField, index);

	// Emit signal to notify any listeners that an action has been disabled
	emit actionDisabled (funcName, entryKey, actionTag);
    }
}



// Returns TRUE, if the specified action is enabled for the entry.
bool UIManager::isActionEnabled (const char *funcName, const char *entryKey, 
				 const char *actionTag)
{
    // Get the action type entry for the actionTag 
    MD_Entry *actionEntry = MD_find_entry (actionSection, actionTag);

    // Punt if action is not declared
    if (actionEntry == NULL)
    {
	TG_error ("UIManager::actionEnabled: actionTag '%s' not declared!  "
		  "Declare first!", actionTag);
    }

    // Get the index field for this entry and get the index from it
    // Probably should upgrade to ActionIndex() when it exists
    MD_Field *indexField = MD_find_field (actionEntry, actionIndexDecl);
    int index = MD_get_int (indexField, 0);

    // Get the '_actions_' field (don't create if absent) in the entry
    MD_Field *actionsField = getActionField ("UIManager::isActionEnabled", 
					     funcName, entryKey, 
					     "_actions_", MD_LINK, FALSE);
    
    // Return true if enabled (exists, index in range, and element not NULL)
    if ((actionsField != NULL) &&
	(index <= MD_max_element_index(actionsField)) &&
	(actionsField->element[index] != NULL))
    {
	return (TRUE);
    }

    // Otherwise, return FALSE, not enabled
    else
    {
	return (FALSE);
    }
}


// Internal routine that returns the next non-zero action activation order id
// to use.  Will return numbers between 1 and 1 billion.  Punts
// if increments past 1 billion.  I chose to punt early so that 
// the return value could be safely multiplied by 4 by the viewer and
// still work properly (see treeview.cpp, 1 billion is about the biggest
// number that will work in a 32 bit system).  
// A new strategy is needed if hit 1 billion action activations.
unsigned int UIManager::nextActivationOrderId ()
{
    // Increment the max id and make sure it hasn't passed 1 billion
    maxActivationOrderId++;
    if (maxActivationOrderId > 1000000000)
    {
	TG_error ("UIManager::nextActivationOrderId: Error more than "
		  "1 billion activations!\n"
		  "Need a new strategy for this routine!");
    }

    // Return the new order id
    return (maxActivationOrderId);
}

// Activates (triggers) action on the entry for the specified function.
// May activate actions that are not enabled!
// If action already activated, ignores call
void UIManager::activateAction (const char *funcName, const char *entryKey, 
				 const char *actionTag, int taskId, int threadId)
{
    // Get the actionTag field (create if absent) in the entry
    MD_Field *actionField = getActionField ("UIManager::activateAction", 
					    funcName, entryKey, 
					    actionTag, MD_INT, TRUE);
    
    // Insert (if necessary) this taskId/threadId pair and get the index 
    // to write the value at for this taskId and threadId
    int index = insertPTPair (taskId, threadId);

    // If not currently activated, activate it
    if ((index > MD_max_element_index (actionField)) ||
	(actionField->element[index] == NULL) ||
	(MD_get_int (actionField, index) == 0))
    {
	// Get the next activation order id and use it to active it
	int orderId = nextActivationOrderId();

	// Write the orderId at the task/thread-determined index in the field
	MD_set_int (actionField, index, orderId);

	// Emit signal to notify any listeners that an action has been 
	// activated
	emit actionActivated (funcName, entryKey, actionTag, taskId, 
			      threadId);
    }
}


// Activates (triggers) action on the all tasks and threads in the entry
// for the specified function.
// May activate actions that are not enabled!
// If action currently deactivated for only a subset of tasks/threads,
// will emit individual signals for those actually activated.
// If action currently deactivated for all tasks/threads, one summary
// signal will be emitted.  
// If action not currently deactivated for any task/thread, nothing is done
// and no signal is emitted.
void UIManager::activateAction (const char *funcName, const char *entryKey, 
				 const char *actionTag)
{
    // Get the actionTag field (create if absent) in the entry
    MD_Field *actionField = getActionField ("UIManager::activateAction", 
					    funcName, entryKey, 
					    actionTag, MD_INT, TRUE);
    
    // Activate all the PTPairs; FOR NOW, ASSUME THAT EVERY ACTION USES
    // PTPAIRS.  That may not be true in a future version?
    int count = PTPairCount();

    // Is this action currently deactivated for all PTPairs?
    // Assume true, look for false case among PTPairs
    bool allWereDeactivated = TRUE;
    for( int index = 0; index < count; index++ ) 
    {
	// If not currently deactivated, all are not currently deactivated
	if ((index <= MD_max_element_index (actionField)) &&
	    (actionField->element[index] != NULL) &&
	    (MD_get_int (actionField, index) != 0))
	{
	    allWereDeactivated = FALSE;
	    break;
	}
    }

    // Delay getting orderId until know activating something
    unsigned int orderId = 0;

    // Now activate any deactivated action
    for( int index = 0; index < count; index++ ) 
    {
	// If not currently activated, activate it
	if ((index > MD_max_element_index (actionField)) ||
	    (actionField->element[index] == NULL) ||
	    (MD_get_int (actionField, index) == 0))
	{
	    // On first activation, get orderId and use for rest
	    if (orderId == 0)
		orderId = nextActivationOrderId();

	    // Write orderId at the task/thread-determined index in the field
	    MD_set_int (actionField, index, orderId);

	    // If not all deactivated, we need to signal each activation
	    // separately
	    if (!allWereDeactivated)
	    {
		// Get taskId and threadId for this PTpair
		int taskId = taskAt (index);
		int threadId = threadAt (index);

		// Emit signal to notify any listeners that an action has been 
		// activated
		emit actionActivated (funcName, entryKey, actionTag, 
				      taskId, threadId);
	    }
	}
    }

    // If *ALL* PTtasks were deactivated before this action, emit signal 
    // indicating all were activated (instead of doing individual signals
    // when only a subset is activated).
    if (allWereDeactivated)
    {
	emit actionActivated (funcName, entryKey, actionTag);
    }
}


// Deactivates (untriggers) action on the entry for the specified function.
// May deactivate actions that are not enabled!
// If action already deactivated, ignore call (no signal emitted).
void UIManager::deactivateAction (const char *funcName, const char *entryKey, 
				  const char *actionTag, int taskId, 
				  int threadId)
{
    // Get the actionTag field (create if absent) in the entry
    MD_Field *actionField = getActionField ("UIManager::deactivateAction", 
					    funcName, entryKey, 
					    actionTag, MD_INT, TRUE);
    
    // Insert (if necessary) this taskId/threadId pair and get the index 
    // to write the value at for this taskId and threadId
    int index = insertPTPair (taskId, threadId);

    // If not currently deactivated, deactivate it
    if ((index <= MD_max_element_index (actionField)) &&
	(actionField->element[index] != NULL) &&
	(MD_get_int (actionField, index) != 0))
    {
	// Write 0 at the task/thread-determined index in the field
	MD_set_int (actionField, index, 0);

	// Emit signal to notify any listeners that an action has been 
	// deactivated
	emit actionDeactivated (funcName, entryKey, actionTag, 
				taskId, threadId);
    }
}


// Deactivates (untriggers) action on all tasks and threads in the entry for
// the specified function.
// May deactivate actions that are not enabled!
// If action currently activated for only a subset of tasks/threads,
// will emit individual signals for those actually deactivated.
// If action currently activated for all tasks/threads, one summary
// signal will be emitted.  
// If action not currently activated for any task/thread, nothing is done
// and no signal is emitted.
void UIManager::deactivateAction (const char *funcName, const char *entryKey, 
				  const char *actionTag)
{
    // Get the actionTag field (create if absent) in the entry
    MD_Field *actionField = getActionField ("UIManager::deactivateAction", 
					    funcName, entryKey, 
					    actionTag, MD_INT, TRUE);
    
    // Activate all the PTPairs; FOR NOW, ASSUME THAT EVERY ACTION USES
    // PTPAIRS.  That may not be true in a future version?
    int count = PTPairCount();

    // Is this action currently activated for all PTPairs?
    // Assume true, look for false case among PTPairs
    bool allWereActivated = TRUE;
    for( int index = 0; index < count; index++ ) 
    {
	// If not currently activated, all are not activated
	if ((index > MD_max_element_index (actionField)) ||
	    (actionField->element[index] == NULL) ||
	    (MD_get_int (actionField, index) == 0))
	{
	    allWereActivated = FALSE;
	    break;
	}
    }

    // Now deactivate any activated action
    for( int index = 0; index < count; index++ ) 
    {
	// If not currently deactivated, deactivate it
	if ((index <= MD_max_element_index (actionField)) &&
	    (actionField->element[index] != NULL) &&
	    (MD_get_int (actionField, index) != 0))
	{
	    // Write 0 at the task/thread-determined index in the field
	    MD_set_int (actionField, index, 0);

	    // If not all activated, we need to signal each deactivation
	    // separately
	    if (!allWereActivated)
	    {
		// Get taskId and threadId for this PTpair
		int taskId = taskAt (index);
		int threadId = threadAt (index);

		// Emit signal to notify any listeners that an action has been 
		// deactivated
		emit actionDeactivated (funcName, entryKey, actionTag, 
					taskId, threadId);
	    }
	}
    }

    // If *ALL* PTtasks were activated before this action, emit signal 
    // indicating all were deactivated (instead of doing individual signals
    // when only a subset is deactivated).
    if (allWereActivated)
    {
	emit actionDeactivated (funcName, entryKey, actionTag);
    }
}


// Returns a non-zero value, if the specified action has been activated 
// for the entry.  This non-zero value specifies the order the actions
// have been activated.  This allows actions at the same action point
// to be represented in the correct order (are executed in the
// order activated).
unsigned int UIManager::isActionActivated (const char *funcName, 
					   const char *entryKey, 
					   const char *actionTag, int taskId, 
					   int threadId)
{
    // Get the actionTag field (don't create if absent) in the entry
    MD_Field *actionField = getActionField ("UIManager::isActionActivated", 
					     funcName, entryKey, 
					     actionTag, MD_INT, FALSE);
    
    // Insert (if necessary) this taskId/threadId pair and get the index 
    // to write the value at for this taskId and threadId
    int index = insertPTPair (taskId, threadId);

    // Return true if activated (exists, index in range, element not NULL, 
    // and element set to 1).
    if ((actionField != NULL) &&
	(index <= MD_max_element_index(actionField)) &&
	(actionField->element[index] != NULL))
    {
	// Get the order value for this action, if 0, not activated.  
	// Otherwise, orderValue is the order Id for the activation of 
	// this action.
	int orderValue = MD_get_int(actionField, index);
	return (orderValue);
    }

    // Otherwise, return 0, not activated
    else
    {
	return (0);
    }
}


// Internal routine to get AppStats structure for the specified 
// data field.  If doesn't exist, and create TRUE, creates it.
// Otherwise returns NULL.  Error messsages use callerDesc as routine name.
// Punts if  dataAttrTag not found.
UIManager::AppStats *UIManager::getAppStats (const char *callerDesc,
					     const char *dataAttrTag,
					     bool create)
{
    // Get the dataAttr's index using internal helper routine
    int dataIndex = getFieldInt(dataAttrSection, dataAttrTag, 
				dataAttrIndexDecl, 0);
    
    // Punt if dataAttr does not exist
    if (dataIndex == NULL_INT)
	TG_error ("%s: attr '%s' not found!", callerDesc, dataAttrTag);


    // Does the App Stats exist?
    AppStats *appStats = appStatsTable.findEntry (dataIndex);

    // If no, and should create, create it
    if ((appStats == NULL) && create)
    {
	// Create appStats
	appStats = new AppStats();
	TG_checkAlloc(appStats);
	
	// Put in appStatsTable
	appStatsTable.addEntry (dataIndex, appStats);

#if 0
	// DEBUG
	printf ("%s: app creating stats for '%s'\n", callerDesc,
		dataAttrTag);
#endif

    }

    // Return appStats (may be NULL)
    return (appStats);
}

// Internal routine to get FileStats structure for the specified file
// and data field pair.  If doesn't exist, and create TRUE, creates it.
// Otherwise returns NULL.  Error messsages use callerDesc as routine name.
// Punts if fileName or dataAttrTag not found.
UIManager::FileStats *UIManager::getFileStats (const char *callerDesc,
					       const char *fileName,
					       const char *dataAttrTag,
					       bool create)
{
    
    // Get fileName's index using internal helper routine 
    int fileIndex = getFieldInt(fileNameSection, fileName,
				fileNameIndexDecl, 0);
    
    // Punt if fileName does not exist
    if (fileIndex == NULL_INT)
	TG_error ("%s: file '%s' not found!", callerDesc, fileName);
    

    // Get the dataAttr's index using internal helper routine
    int dataIndex = getFieldInt(dataAttrSection, dataAttrTag, 
				dataAttrIndexDecl, 0);

    // Punt if dataAttr does not exist
    if (dataIndex == NULL_INT)
	TG_error ("%s: attr '%s' not found!", callerDesc, dataAttrTag);


    // Does the File Stats exist?
    FileStats *fileStats = fileStatsTable.findEntry (fileIndex, dataIndex);

    // If no, and should create, create it
    if ((fileStats == NULL) && create)
    {
	// Get (and possibly create) appStats for this data type
	AppStats *appStats = getAppStats (callerDesc, dataAttrTag, TRUE);

	// Create fileStats
	fileStats = new FileStats(appStats);
	TG_checkAlloc(fileStats);

	// Put in fileStatsTable
	fileStatsTable.addEntry (fileIndex, dataIndex, fileStats);

	// Add to appStats linked list (better call only once per creation)
	appStats->addFileStats(fileStats);

#if 0
	// DEBUG
	printf ("%s: '%s' creating stats for '%s'\n", callerDesc,
		fileName, dataAttrTag);
#endif

    }

    // Return fileStats (may be NULL)
    return (fileStats);
}

// Internal routine to get FuncStats structure for the specified function
// and data field pair.  If doesn't exist, and create TRUE, creates it.
// Otherwise returns NULL.  Error messsages use callerDesc as routine name.
// Punts if funcName or dataAttrTag not found.
UIManager::FuncStats *UIManager::getFuncStats (const char *callerDesc,
					       const char *funcName,
					       const char *dataAttrTag,
					       bool create)
{
    
    // Get funcName's index using internal helper routine 
    int funcIndex = getFieldInt(functionNameSection, funcName,
				functionNameIndexDecl, 0);
    
    // Punt if funcName does not exist
    if (funcIndex == NULL_INT)
	TG_error ("%s: function '%s' not found!", callerDesc, funcName);
    

    // Get the dataAttr's index using internal helper routine
    int dataIndex = getFieldInt(dataAttrSection, dataAttrTag, 
			    dataAttrIndexDecl, 0);

    // Punt if dataAttr does not exist
    if (dataIndex == NULL_INT)
	TG_error ("%s: attr '%s' not found!", callerDesc, dataAttrTag);


    // Does the Func Stats exist?
    FuncStats *funcStats = funcStatsTable.findEntry (funcIndex, dataIndex);

    // If no, and should create, create it
    if ((funcStats == NULL) && create)
    {
	// Get fileName using a internal helper routine
	QString fileName = getFieldQString(functionNameSection, funcName,
					   functionFileNameDecl, 0);

	// Get (and possibly create) fileStats for this data type
	FileStats *fileStats = getFileStats (callerDesc, 
					     fileName.latin1(),
					     dataAttrTag, TRUE);

	// Create funcStats
	funcStats = new FuncStats(funcIndex, fileStats);
	TG_checkAlloc(funcStats);
	
	// Put in funcStatsTable
	funcStatsTable.addEntry (funcIndex, dataIndex, funcStats);

	// Add to fileStats linked list (better call only once per creation)
	fileStats->addFuncStats(funcStats);
	
#if 0
	// DEBUG
	printf ("%s: '%s' creating stats for '%s'\n", callerDesc,
		funcName, dataAttrTag);
#endif
    }

    // Return funcStats (may be NULL)
    return (funcStats);
}
					       
					       

// Internal routine to get EntryStats structure for the specified 
// data field for the entry (includes pointer to field).
// If entryStats doesn't exist and create true, it creates it, 
// otherwise returns NULL.  Will punt with error message if funcName, 
// entryKey, or dataAttr tag doesn't exist or expectedType mismatches.  
// If create false, will not create any new fields (or anything else).
// Error messages use 'callerDesc' as routine name.
// Used by setInt(), getMax(), etc.  
UIManager::EntryStats *UIManager::getEntryStats (const char *callerDesc, 
						 const char *funcName,
						 const char *entryKey, 
						 const char *dataAttrTag,
						 int expectedType, 
						 bool create)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable,
					      funcName);

    // Punt if function does not exist
    if (fi == NULL)
	TG_error ("%s: function '%s' not found!",	callerDesc, funcName);


    // Get the dataAttr's index using internal helper routine
    int dataIndex = getFieldInt(dataAttrSection, dataAttrTag, 
			    dataAttrIndexDecl, 0);

    // Punt if dataAttr does not exist
    if (dataIndex == NULL_INT)
	TG_error ("%s: attr '%s' not found!", callerDesc, dataAttrTag);

    // Get the entry's index using internal helper routine
    int entryIndex = getFieldInt(fi->dataSection, entryKey, fi->indexDecl, 0);

    if (entryIndex == NULL_INT)
	TG_error ("%s: entry '%s' not found in '%s'!", callerDesc,
		  entryKey, funcName);

    // Does the Entry Stats exist?
    EntryStats *stats = fi->entryStatsTable.findEntry (entryIndex, dataIndex);

    // If exists, check type
    if (stats != NULL)
    {
	// Error if attr is incorrect data type.  Assume type 0 requirement
	// set to MD_INT, MD_DOUBLE only!
	MD_Element_Req *requirement = stats->field->decl->require[0];

	// If NULL_INT passed, don't check type (for getValue(), getMax(), 
	// etc., when INT or DOUBLE allowed).
	if ((expectedType != NULL_INT) && 
	    (requirement->type != expectedType))
	{
	    char *typeDesc = "(unexpected type)";
	    // Get understandable type name for this attr
	    switch (requirement->type)
	    {
	      case MD_INT:
		typeDesc = "integer";
		break;
		
	      case MD_DOUBLE:
		typeDesc = "double";
		break;
	    }
	    TG_error ("%s (%s, %s, %s,...): \n"
		    "   DataAttr '%s' declared as holding type '%s'!\n",
		    callerDesc, funcName, entryKey, dataAttrTag, 
		    typeDesc);
	}
    }

    // If not, create (if desired)
    else if (create)
    {
	// Use getField with the function's dataSection to create a data
	// field for this entry.
	MD_Field *field = getField (callerDesc, 
				    funcName, fi->dataSection,
				    entryKey, dataAttrTag, 
				    expectedType, TRUE);

	// Get (and possibly create) funcStats for this data type
	FuncStats *funcStats = getFuncStats (callerDesc, funcName,
					     dataAttrTag, TRUE);

	// Create new entry stats 
	stats = new EntryStats (entryIndex, funcStats, field);
	TG_checkAlloc(stats);

	// Add to entryStats table
	fi->entryStatsTable.addEntry (entryIndex, dataIndex, stats);

	// Add to funcStats linked list (better call only once per creation
	funcStats->addEntryStats(stats);

#if 0
	// DEBUG
	printf ("%s: '%s' entry '%s' creating stats for '%s'\n", callerDesc,
		funcName, entryKey, dataAttrTag);
#endif
    }

    // Return the entryStats (may be NULL)
    return (stats);
}

// Internal routine to update the entry stats and then the rest of the 
// rollup stats (func, file, and app stats).  Do not call when rescanning
// in entry data!
void UIManager::updateEntryStats (EntryStats *stats, int PTPairIndex, 
				  double newEntryValue, bool entryUpdate, 
				  double origEntryValue)
{

#if 0
    // DEBUG, use MD info to get names
    printf ("*** UpdateEntryStats for (%s, %s, %s): (%i, %g, %i, %g)\n",
	    stats->field->entry->section->name, stats->field->entry->name, 
	    stats->field->decl->name, PTPairIndex, newEntryValue, 
	    entryUpdate, origEntryValue);
#endif

    // Get old value for stats before update
    int oldCount = stats->count();
    double oldSum = stats->sum();
    double oldMax = stats->max();
    double oldMin = stats->min();

    // Unless oldCount == 0, this is an update of the funcStats, fileStats,
    // and appStats.   Also calculate old mean differently if no old data
    bool rollupUpdate;
    double oldMean;
    if (oldCount == 0)
    {
	oldMean = 0.0;
	rollupUpdate = FALSE;
    }
    else
    {
	oldMean = oldSum/(double)oldCount;
	rollupUpdate = TRUE;
    }

    // Update the entry stats (always as double, int sums overflow too easily)
    stats->updateStats (PTPairIndex, newEntryValue, entryUpdate, 
			origEntryValue);

    // Get the new entry stats after update
    int newCount = stats->count();
    double newSum = stats->sum();
    double newMean = newSum/(double)newCount; // count cannot be 0
    double newMax = stats->max();
    double newMin = stats->min();
    
    // Get the entryIndex, funcStats, fileStats, and appStats for ease of use
    int entryIndex = stats->entryIndex;
    FuncStats *funcStats = stats->funcStats;
    FileStats *fileStats = funcStats->fileStats;
    AppStats *appStats = fileStats->appStats;

    // For file, application stats, use statIndex array where
    // statIndex[0] is funcIndex
    // statIndex[1] is entryIndex
    int statIndex[2];
    statIndex[0] = funcStats->funcIndex;
    statIndex[1] = entryIndex;

    // Update SumStats for the function, file, and app
    funcStats->sumStats.updateStats(entryIndex, newSum, rollupUpdate, oldSum);
    fileStats->sumStats.updateStats(statIndex, newSum, rollupUpdate, oldSum);
    appStats->sumStats.updateStats(statIndex, newSum, rollupUpdate, oldSum);

    // Update MeanStats for the function, file, and app
    funcStats->meanStats.updateStats(entryIndex, newMean, rollupUpdate, 
				     oldMean);
    fileStats->meanStats.updateStats(statIndex, newMean, rollupUpdate, 
				     oldMean);
    appStats->meanStats.updateStats(statIndex, newMean, rollupUpdate, 
				    oldMean);
    
    // If have valid newMax, update maxStats for the function, file, and app
    if (newMax != NULL_DOUBLE)
    {
	funcStats->maxStats.updateStats (entryIndex, newMax, rollupUpdate, 
					 oldMax);
	fileStats->maxStats.updateStats (statIndex, newMax, rollupUpdate, 
					 oldMax);
	appStats->maxStats.updateStats (statIndex, newMax, rollupUpdate, 
					oldMax);
    }
    // Otherwise, invalidate stats that require newMax to be calculated
    // precisely (i.e., could be newEntryValue)
    else
    {
	funcStats->maxStats.invalidateStats (entryIndex, newEntryValue, 
					     rollupUpdate);
	fileStats->maxStats.invalidateStats (statIndex, newEntryValue, 
					     rollupUpdate);
	appStats->maxStats.invalidateStats (statIndex, newEntryValue, 
					    rollupUpdate);
    }

#if 0
    // DEBUG
    double maxMax = funcStats->maxStats.max();
    double maxMin = funcStats->maxStats.min();
    double maxSum = funcStats->maxStats.sum();
    cout << "Post maxMax " << maxMax << " maxMin " << maxMin << " maxSum " <<
	maxSum << endl;
#endif

    // If have valid newMin, update minStats for the function, file, and app
    if (newMin != NULL_DOUBLE)
    {
	funcStats->minStats.updateStats (entryIndex, newMin, rollupUpdate, 
					 oldMin);
	fileStats->minStats.updateStats (statIndex, newMin, rollupUpdate, 
					 oldMin);
	appStats->minStats.updateStats (statIndex, newMin, rollupUpdate, 
					oldMin);
    }
    // Otherwise, invalidate stats that require newMax to be calculated
    // precisely (i.e., could be newEntryValue)
    else
    {
	funcStats->minStats.invalidateStats (entryIndex, newEntryValue, 
					     rollupUpdate);
	fileStats->minStats.invalidateStats (statIndex, newEntryValue, 
					     rollupUpdate);
	appStats->minStats.invalidateStats (statIndex, newEntryValue, 
					    rollupUpdate);
    }

    // Invalidata the miscStats cache for this function, file, and app
    funcStats->miscStatsType = UIManager::EntryInvalid;
    fileStats->miscStatsType = UIManager::EntryInvalid;
    appStats->miscStatsType = UIManager::EntryInvalid;

#if 0
    // DEBUG
    double minMax = funcStats->minStats.max();
    double minMin = funcStats->minStats.min();
    double minSum = funcStats->minStats.sum();
    cout << "Post minMax " << minMax << " minMin " << minMin << " minSum " <<
	minSum << endl;
#endif
}


// Internal routine to get the specified data field for the function.
// If field doesn't exist and create true, it creates the field, 
// otherwise returns NULL.  Will punt with error meesage if funcName, 
// entryKey, or dataAttr tag doesn't exist or expectedType mismatches.  
// If create false, will not create any new fields (or anything else).
// Error messages use 'callerDesc' as routine name.
// Used by setInt(), getInt(), etc.  
MD_Field *UIManager::getDataField (const char *callerDesc, const char *funcName,
				   const char *entryKey, const char *dataAttrTag,
				   int expectedType, 
				   bool create)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable,
					      funcName);

    // Punt if function does not exist
    if (fi == NULL)
	TG_error ("%s: function '%s' not found!", callerDesc, funcName);

    // Use getField with the function's dataSection to do the rest 
    MD_Field *field = getField (callerDesc, 
				funcName, fi->dataSection,
				entryKey, dataAttrTag, 
				expectedType, create);

    // Return the field (may be NULL)
    return (field);
}

// Internal routine to get the specified action field for the function.
// If field doesn't exist and create true, it creates the field, 
// otherwise returns NULL.  Will punt with error meesage if funcName, 
// entryKey, or action tag doesn't exist or expectedType mismatches.  
// If create false, will not create any new fields (or anything else).
// Error messages use 'callerDesc' as routine name.
// Used by activateAction(), deactivateAction(), etc.  
MD_Field *UIManager::getActionField (const char *callerDesc, 
				     const char *funcName,
				     const char *entryKey, 
				     const char *actionTag,
				     int expectedType, 
				     bool create)
{
    // Get the function info for this funcName
    FuncInfo *fi = 
	(FuncInfo *) STRING_find_symbol_data (functionSectionTable,
					      funcName);

    // Punt if function does not exist
    if (fi == NULL)
	TG_error ("%s: function '%s' not found!", callerDesc, funcName);

    // Use getField with the function's actionSection to do the rest 
    MD_Field *field = getField (callerDesc, 
				funcName, fi->actionSection,
				entryKey, actionTag, 
				expectedType, create);

    // Return the field (may be NULL)
    return (field);
}


// Internal routine to get the specified field from the specified section).
// If field doesn't exist and create true, it creates the field, 
// otherwise returns NULL.  Will punt with error meesage if funcName, 
// entryKey, or attr tag doesn't exist or expectedType mismatches.  
// If create false, will not create any new fields (or anything else).
// Error messages use 'callerDesc' as routine name
// Used directly by getDataField() and getActionField() and indirectly
// by setInt(), getInt(), enableAction(), etc.  
MD_Field *UIManager::getField (const char *callerDesc, 
			       const char *funcName, MD_Section *section,
			       const char *entryKey, const char *attrTag,
			       int expectedType, 
			       bool create)
{
    // Get the entry for the entryKey in this function's passed section
    MD_Entry *entry = MD_find_entry (section, entryKey);

    // Punt if entry does not exist in this function
    if (entry == NULL)
    {
	TG_error ("%s: entry '%s' not found in '%s'!", callerDesc,
		  entryKey, funcName);
    }

    // Get the field declaration for this attrTag
    MD_Field_Decl *fieldDecl = MD_find_field_decl (section, attrTag);
    
    // Punt if attr does not exist in this function
    if (fieldDecl == NULL)
	TG_error ("%s: attr '%s' not found!", callerDesc, attrTag);

    // Error if attr is incorrect data type.  Assume type 0 requirement
    // set to MD_INT, MD_STRING, MD_DOUBLE, or MD_LINK (for action types) only!
    MD_Element_Req *requirement = fieldDecl->require[0];
    if (requirement->type != expectedType)
    {
	char *typeDesc = "(unknown type)";
	// Get understandable type name for this attr
	switch (requirement->type)
	{
	  case MD_INT:
	    typeDesc = "integer";
	    break;

	  case MD_DOUBLE:
	    typeDesc = "double";
	    break;

	  case MD_STRING:
	    typeDesc = "string";
	    break;

	  case MD_LINK:
	    typeDesc = "link";
	    break;
	}
	TG_error ("%s (%s, %s, %s,...): \n"
		  "   DataAttr '%s' declared as holding type '%s'!",
		  callerDesc, funcName, entryKey, attrTag, attrTag,
		  typeDesc);
    }

    // Get the desired field, if it exists
    MD_Field *field = MD_find_field (entry, fieldDecl);

    // If field doesn't exist and create set, create it
    if ((field == NULL) && (create != 0))
	field = MD_new_field (entry, fieldDecl, 0);

    // Return the field (or NULL if doesn't exist and asked not to create it)
    return (field);
}

// Internal routine to get int from entry.  Returns NULL_INT on any error
int UIManager::getFieldInt (MD_Section *section, const char *entryName, 
			    MD_Field_Decl *fieldDecl, int index)
{
    // Get entry, if exists
    MD_Entry *entry = MD_find_entry (section, entryName);

    // Return NULL_INT if entry not found
    if (entry == NULL)
	return (NULL_INT);

    // Get field for entry from fieldDecl
    MD_Field *field = MD_find_field (entry, fieldDecl);

    // Return NULL_INT if field not found 
    if (field == NULL)
	return (NULL_INT);

    // Return NULL_INT if index out of bounds or value has not been set
    if ((index < 0) || (index > MD_max_element_index(field)) ||
	(field->element[index] == NULL))
	return (NULL_INT);
    
    // Get value from field
    int value = MD_get_int (field, index);

    // Return this value
    return (value);
}

// Internal routine to get double from entry.  Returns NULL_DOUBLE on error
double UIManager::getFieldDouble (MD_Section *section, const char *entryName, 
				  MD_Field_Decl *fieldDecl, int index)
{
    // Get entry, if exists
    MD_Entry *entry = MD_find_entry (section, entryName);

    // Return NULL_DOUBLE if entry not found
    if (entry == NULL)
	return (NULL_DOUBLE);

    // Get field for entry from fieldDecl
    MD_Field *field = MD_find_field (entry, fieldDecl);

    // Return NULL_DOUBLE if field not found 
    if (field == NULL)
	return (NULL_DOUBLE);

    // Return NULL_DOUBLE if index out of bounds or value has not been set
    if ((index < 0) || (index > MD_max_element_index(field)) ||
	(field->element[index] == NULL))
	return (NULL_DOUBLE);
    
    // Get value from field
    double value = MD_get_double (field, index);

    // Return this value
    return (value);
}

// Internal routine to get QString from entry.  On error, returns NULL_QSTRING
QString UIManager::getFieldQString (MD_Section *section, const char *entryName, 
				    MD_Field_Decl *fieldDecl, int index)
{
    // Get entry, if exists
    MD_Entry *entry = MD_find_entry (section, entryName);

    // Return NULL_QSTRING if entry not found
    if (entry == NULL)
	return (NULL_QSTRING);

    // Get field for entry from fieldDecl
    MD_Field *field = MD_find_field (entry, fieldDecl);

    // Return NULL_QSTRING if field not found 
    if (field == NULL)
	return (NULL_QSTRING);

    // Return NULL_QSTRING if index out of bounds or value has not been set
    if ((index < 0) || (index > MD_max_element_index(field)) ||
	(field->element[index] == NULL))
	return (NULL_QSTRING);
    
    // Get char * value from field
    char *asciiValue = MD_get_string (field, index);

    // Create QString version of string
    QString value (asciiValue);
    
    // Return QString version of this char * value
    // QString makes shallow copies, so expect this to be efficient
    return (value);
}


// Creates (if necessary) the task/thread pair and returns its index.
// Optional since set_int(), etc. will also create task/thread pairs.
// This is the only way to declare taskIds or threadIds. 
// Use -1 for unknown taskIds or threadIds.
int UIManager::insertPTPair (int taskId, int threadId)
{
    // See if index already assigned to this taskId and threadId pair
    int index = PTPairIndex (taskId, threadId);

    // Return index now if exists
    if (index != NULL_INT)
	return (index);

    // 
    // Otherwise, add task/thread combo to PTPairSection, 
    // task to taskSection, and thread to threadSection (as necessary)
    //

    // Create tag string from taskId and threadId
    char PTPairTag[100];
    sprintf (PTPairTag, "%i_%i", taskId, threadId);

    // Create indexed entry for this task/thread combo
    MD_Entry *PTPairEntry = newIndexedEntry (PTPairSection, 
						 PTPairTag,
						 PTPairIndexDecl, 
						 PTPairMap, &index);

    // Create threadId field and assign threadId to it
    MD_Field *threadIdField = MD_new_field (PTPairEntry,
					    PTPairThreadIdDecl, 0);
    MD_set_int (threadIdField, 0, threadId);

    // Create taskId field and assign taskId to it
    MD_Field *taskIdField = MD_new_field (PTPairEntry, 
					  PTPairTaskIdDecl, 0);
    MD_set_int (taskIdField, 0, taskId);

    // Stash index in symbol table keyed with both taskId and threadId
    // Use (void *)(long) to suppress warnings but this is potentially
    // dangerous to do (although it appears valid on all systems so far)!
    int key_array[2] = {taskId, threadId};
    INT_ARRAY_add_symbol (PTPairIndexMap, key_array, 2, (void *)(long)index);
    

    // 
    // Add/update the taskId section with this task/thread combo
    //

    // Create tag string from taskId
    char taskIdTag[100];
    sprintf (taskIdTag, "%i", taskId);

    // Get the task entry for this task, if exists
    MD_Entry *taskIdEntry = MD_find_entry (taskIdSection, taskIdTag);


    // If exists, get the 'threadList' field
    MD_Field *threadListField;
    if (taskIdEntry != NULL)
    {
	threadListField = MD_find_field (taskIdEntry, taskThreadListDecl);
    }

    // Otherwise, create entry and the 'threadList' field for it
    else
    {
	int taskIndex;
	taskIdEntry = newIndexedEntry (taskIdSection, taskIdTag,
				       taskIdIndexDecl, taskIdMap,
				       &taskIndex);
	threadListField = MD_new_field (taskIdEntry, taskThreadListDecl, 0);
	// To facilitate mapping taskId to the corresponding task entry
	INT_add_symbol (taskEntryMap, taskId, (void *) taskIdEntry);
	
	// To facilitate mapping indexes to taskIds
	indexTaskIdMap.addEntry(taskIndex, taskId);

    }

    // Add thread id at end of thread list field
    // (MD_max_element_index returns -1 if field is empty)
    int threadListIndex = MD_max_element_index(threadListField) + 1;
    MD_set_int (threadListField, threadListIndex, threadId);
    
    // 
    // Add/update the threadId section with this task/thread combo
    //

    // Create tag string from threadId
    char threadIdTag[100];
    sprintf (threadIdTag, "%i", threadId);
    
    // Get the thread entry for this thread, if exists
    MD_Entry *threadIdEntry = MD_find_entry (threadIdSection, threadIdTag);


    // If exists, get the 'taskList' field
    MD_Field *taskListField;
    if (threadIdEntry != NULL)
    {
	taskListField = MD_find_field (threadIdEntry, threadTaskListDecl);
    }

    // Otherwise, create entry and the 'taskList' field for it
    else
    {
	int threadIndex;
	threadIdEntry = newIndexedEntry (threadIdSection, threadIdTag,
					 threadIdIndexDecl, threadIdMap,
					 &threadIndex);
	taskListField = MD_new_field (threadIdEntry, threadTaskListDecl, 0);

	// To facilitate mapping threadId to the corresponding thread entry
	INT_add_symbol (threadEntryMap, threadId, (void *) threadIdEntry);

	// To facilitate mapping indexes to threadIds
	indexThreadIdMap.addEntry(threadIndex, threadId);
    }

    // Add thread id at end of thread list field
    // (MD_max_element_index returns -1 if field is empty)
    int taskListIndex = MD_max_element_index(taskListField) + 1;
    MD_set_int (taskListField, taskListIndex, taskId);
    
    // Notify listeners that a new PTPair has been inserted
    emit PTPairInserted (taskId, threadId);

    // Return the new index for this task/thread combo
    return (index);
}

// Returns number of taskId/threadId pairs inserted
int UIManager::PTPairCount()
{
    // Get count from MD table that holds inserted task/thread pairs
    int count = MD_num_entries (PTPairSection);

    // Return this count
    return (count);
}


// Returns index (>= 0) if taskId/threadId pair exists, NULL_INT otherwise
int UIManager::PTPairIndex (int taskId, int threadId)
{
    // Create key array to look up taskId/threadId pair
    int key_array[2] = {taskId, threadId};

    // Get symbol for this key
    INT_ARRAY_Symbol *symbol = INT_ARRAY_find_symbol (PTPairIndexMap, 
						      key_array, 2);

    // If doesn't exist, return NULL_INT (pair not declared)
    if (symbol == NULL)
	return (NULL_INT);

    // Otherwise, return index which is stashed int the data field
    else
    {
	// Get rid of ptr to int warning when on tru64 by using long tmp
	long tmp = (long)symbol->data;
	return ((int)tmp);
    }
}

// Returns task portion of taskId/threadId at index 
int UIManager::PTPairTaskAt(int index)
{
    QString *PTPairPtr = 
	(QString *)INT_find_symbol_data(PTPairMap, index);

    // If pointer NULL, index out of bounds, return NULL_INT
    if (PTPairPtr == NULL)
	return (NULL_INT);

    // Otherwise, get char * version of PTPairTag
    char *PTPairTag = (char *)PTPairPtr->latin1();

    // Get task using internal helper routine and return it
    int task = getFieldInt(PTPairSection, PTPairTag, 
			   PTPairTaskIdDecl, 0);
    return (task);
}

// Returns thread portion of taskId/threadId at index
int UIManager::PTPairThreadAt(int index)
{
    QString *PTPairPtr = 
	(QString *)INT_find_symbol_data(PTPairMap, index);

    // If pointer NULL, index out of bounds, return NULL_INT
    if (PTPairPtr == NULL)
	return (NULL_INT);

    // Otherwise, get char * version of PTPairTag
    char *PTPairTag = (char *)PTPairPtr->latin1();

    // Get thread using internal helper routine and return it
    int thread = getFieldInt(PTPairSection, PTPairTag, 
			   PTPairThreadIdDecl, 0);
    return (thread);
}


// Returns number of taskIds inserted by insertPTPair().
int UIManager::taskCount()
{
    // Get count from MD table that holds inserted tasks
    int count = MD_num_entries (taskIdSection);

    // Return this count
    return (count);
}

// Returns index of taskId, NULL_INT if not found
int UIManager::taskIndex (int taskId)
{
    // Use quick lookup table to get taskIdEntry from taskId
    MD_Entry *taskIdEntry = 
	(MD_Entry *) INT_find_symbol_data (taskEntryMap, taskId);

    // If not found, return NULL_INT
    if (taskIdEntry == NULL)
	return (NULL_INT);

    // Get index field for this entry, its value, and return the value
    MD_Field *indexField = MD_find_field (taskIdEntry, taskIdIndexDecl);
    int index = MD_get_int (indexField, 0);
    return (index);
}

// Returns taskId at index in task list, NULL_INT if index out of bounds
int UIManager::taskAt(int index)
{
    // Use quick lookup map to get task id at this index
    int taskId = indexTaskIdMap.findEntry(index);

    // If mapping not found, above calls returns NULL_INT, otherwise taskId
    return (taskId);
}

// Returns number of threadIds paired with this task
int UIManager::taskThreadCount(int taskId)
{
    // Use quick lookup table to get taskIdEntry from taskId
    MD_Entry *taskIdEntry = 
	(MD_Entry *) INT_find_symbol_data (taskEntryMap, taskId);

    // If not found, return NULL_INT
    if (taskIdEntry == NULL)
	return (NULL_INT);

    // Get threadList field for this entry
    MD_Field *threadListField = MD_find_field (taskIdEntry, 
					       taskThreadListDecl);

    // Get count from this field and return it
    int count = MD_num_elements(threadListField);
    return (count);    
}

// Returns the threadId paired with this task at index
int UIManager::taskThreadAt(int taskId, int index)
{
    // Use quick lookup table to get taskIdEntry from taskId
    MD_Entry *taskIdEntry = 
	(MD_Entry *) INT_find_symbol_data (taskEntryMap, taskId);

    // If not found, return NULL_INT
    if (taskIdEntry == NULL)
	return (NULL_INT);

    // Get threadList field for this entry
    MD_Field *threadListField = MD_find_field (taskIdEntry, 
					       taskThreadListDecl);

    // Return NULL_INT, if index out of bounds or element not set
    if ((index < 0) || 
	(index > MD_max_element_index(threadListField)) ||
	(threadListField->element[index] == NULL))
	return (NULL_INT);

    // Get threadId from field and return it
    int threadId = MD_get_int (threadListField, index);
    return (threadId);
}


// Returns number of threadIds inserted by insertPTPair().
int UIManager::threadCount()
{
    // Get count from MD table that holds inserted threads
    int count = MD_num_entries (threadIdSection);

    // Return this count
    return (count);
}

// Returns index of threadId, NULL_INT if not found
int UIManager::threadIndex (int threadId)
{
    // Use quick lookup table to get taskIdEntry from taskId
    MD_Entry *threadIdEntry = 
	(MD_Entry *) INT_find_symbol_data (threadEntryMap, threadId);

    // If not found, return NULL_INT
    if (threadIdEntry == NULL)
	return (NULL_INT);

    // Get index field for this entry, its value, and return the value
    MD_Field *indexField = MD_find_field (threadIdEntry, threadIdIndexDecl);
    int index = MD_get_int (indexField, 0);
    return (index);
}

// Returns threadId at index in thread list, NULL_INT if out of bounds
int UIManager::threadAt(int index)
{
    // Use quick lookup map to get thread id at this index
    int threadId = indexThreadIdMap.findEntry(index);

    // If mapping not found, return NULL_INT, otherwise threadId
    return (threadId);
}

// Returns number of taskIds paired with this thread
int UIManager::threadTaskCount(int threadId)
{
    // Use quick lookup table to get threadIdEntry from threadId
    MD_Entry *threadIdEntry = 
	(MD_Entry *) INT_find_symbol_data (threadEntryMap, threadId);

    // If not found, return NULL_INT
    if (threadIdEntry == NULL)
	return (NULL_INT);

    // Get taskList field for this entry
    MD_Field *taskListField = MD_find_field (threadIdEntry, 
					     threadTaskListDecl);

    // Get count from this field and return it
    int count = MD_num_elements(taskListField);
    return (count);    
}

// Returns the taskId paired with this thread at index
int UIManager::threadTaskAt(int threadId, int index)
{
    // Use quick lookup table to get threadIdEntry from threadId
    MD_Entry *threadIdEntry = 
	(MD_Entry *) INT_find_symbol_data (threadEntryMap, threadId);

    // If not found, return NULL_INT
    if (threadIdEntry == NULL)
	return (NULL_INT);

    // Get taskList field for this entry
    MD_Field *taskListField = MD_find_field (threadIdEntry, 
					     threadTaskListDecl);

    // Return NULL_INT, if index out of bounds or element not set
    if ((index < 0) || 
	(index > MD_max_element_index(taskListField)) ||
	(taskListField->element[index] == NULL))
	return (NULL_INT);

    // Get taskId from field and return it
    int taskId = MD_get_int (taskListField, index);
    return (taskId);
}

// Returns TRUE if value is set, FALSE otherwise.  Independent
// of datatype (may be called for INT, DOUBLE, etc.) value.
bool UIManager::isValueSet (const char *funcName, const char *entryKey, const char *dataAttrTag, 
			    int taskId, int threadId)
{
    // Get type of data attribute, so that getDataField won't punt
    int dataType = dataAttrType (dataAttrTag);

    // If dataAttrTag doesn't exist, return FALSE
    if (dataType == NULL_INT)
	return (FALSE);

    // Get the specified data field in the entry (don't create if not there)
    MD_Field *field = getDataField ("UIManager::isValueSet", funcName,
				    entryKey, dataAttrTag, dataType, 0);

    // If doesn't exist, return FALSE
    if (field == NULL)
	return (FALSE);
    
    // Get the index to read the value at for this task and thread
    // Doesn't add taskId/threadId combo if it doesn't exist already
    int index = PTPairIndex (taskId, threadId);

    // If taskId/threadId combo doesn't exist (got NULL_INT), or out
    // of bounds for the field, return FALSE
    if ((index == NULL_INT) || (index > MD_max_element_index(field)))
	return (FALSE);
    
    // If this particular value has not been set, return FALSE
    if (field->element[index] == NULL)
	return (FALSE);

    // If got here, value must be set.  Return TRUE
    return (TRUE);
}

// Writes an int to the specified location for the specified task/thread id.
// Punts if funcName, entryKey, or dataAttrTag undefined or not int dataAttr
void UIManager::setInt (const char *funcName, const char *entryKey, const char *dataAttrTag, 
			int taskId, int threadId, int value)
{
    // Get the entryStats for this entry and data field (create if not there)
    EntryStats *stats = getEntryStats ("UIManager::setInt", funcName,
				       entryKey, dataAttrTag, MD_INT, 1);

    // Get the specified data field from the entryStats for ease of use
    MD_Field *field = stats->field;

    // Insert (if necessary) this taskId/threadId pair and get the index 
    // to write the value at for this taskId and threadId
    int index = insertPTPair (taskId, threadId);

    // Get original value (as double), if actual set (in bounds and not NULL)
    int origValue;
    bool update;
    if ((index <= MD_max_element_index(field)) &&
	(field->element[index] != NULL))
    {
	origValue = MD_get_int (field, index);
	update = TRUE;  // Updating existing value
    }

    // Otherwise, default to 0
    else
    {
	origValue = 0;
	update = FALSE; // Setting new value
    }

    // Write integer at the task/thread-determined index in the field
    MD_set_int (field, index, value);

    // Update stats for this data field (as doubles, int overflow too quickly)
    updateEntryStats (stats, index, (double)value, update, (double)origValue);

    // Emit signal to notify any listeners that a value has been set
    emit intSet(funcName, entryKey, dataAttrTag, taskId, threadId, value);
}


// Returns the new, incremented int value at the specified location for 
// the specified task/thread id.  Assumes unset values are 0.
// Much more efficient than getInt() followed by setInt()!
// Punts if funcName, entryKey, or dataAttrTag undefined or not int dataAttr
int UIManager::addInt (const char *funcName, const char *entryKey, const char *dataAttrTag, 
		       int taskId, int threadId, int increment)
{
    // Get the entryStats for this entry and data field (create if not there)
    EntryStats *stats = getEntryStats ("UIManager::addInt", funcName,
				       entryKey, dataAttrTag, MD_INT, 1);

    // Get the specified data field from the entryStats for ease of use
    MD_Field *field = stats->field;

    // Insert (if necessary) this taskId/threadId pair and get the index 
    // to write the value at for this taskId and threadId
    int index = insertPTPair (taskId, threadId);

    // Get original value (as double), if actual set (in bounds and not NULL)
    int origValue;
    bool update;
    if ((index <= MD_max_element_index(field)) &&
	(field->element[index] != NULL))
    {
	origValue = MD_get_int (field, index);
	update = TRUE;  // Updating existing value
    }

    // Otherwise, default to 0
    else
    {
	origValue = 0;
	update = FALSE; // Setting new value
    }

    // Calculate new value
    int newValue = origValue + increment;

    // Write integer at the task/thread-determined index in the field
    MD_set_int (field, index, newValue);

    // Update stats for this data field (as doubles, int overflow too quickly)
    updateEntryStats (stats, index, (double)newValue, update, 
		      (double)origValue);

    // Emit signal to notify any listeners that a value has been set
    emit intSet(funcName, entryKey, dataAttrTag, taskId, threadId, newValue);

    // Return the new value for this data location
    return (newValue);
}

// Returns the int at the specified location for the specified task/thread id.
// Returns NULL_INT if value not previously set.
// Punts if funcName, entryKey, or dataAttrTag undefined or not int dataAttr
int UIManager::getInt (const char *funcName, const char *entryKey, const char *dataAttrTag, 
		       int taskId, int threadId)
{
    // Get the specified data field in the entry (don't create if not there)
    MD_Field *field = getDataField ("UIManager::getInt", funcName,
				    entryKey, dataAttrTag, MD_INT, 0);

    // If doesn't exist, return NULL_INT
    if (field == NULL)
	return (NULL_INT);
    
    // Get the index to read the value at for this task and thread
    // Doesn't add taskId/threadId combo if it doesn't exist already
    int index = PTPairIndex (taskId, threadId);

    // If taskId/threadId combo doesn't exist (got NULL_INT), or out
    // of bounds for the field, return NULL_INT
    if ((index == NULL_INT) || (index > MD_max_element_index(field)))
	return (NULL_INT);
    
    // If this particular value has not been set, return NULL_INT
    if (field->element[index] == NULL)
	return (NULL_INT);

    // Read integer at the task/thread-determined index in the field
    int value = MD_get_int (field, index);

    // Return the value read for this data location
    return (value);
}


// Writes an double to the specified location for the specified task/thread id.
// Punts if funcName, entryKey, or dataAttrTag undefined or not double dataAttr
void UIManager::setDouble (const char *funcName, const char *entryKey, 
			   const char *dataAttrTag, int taskId, 
			   int threadId, double value)
{
    // Ignore inf/NAN values, screws up statistics and display
    if (!finite(value))
    {
	// Print warning that ignoring value
	fprintf (stderr, 
		 "\n"
		 "Warning: Tool Gear ignoring bad value '%g' for:\n"
		 "  UIManager::setDouble (%s, %s, %s, %i, %i, %g):\n",
		 value, funcName, entryKey, dataAttrTag, taskId, threadId, 
		 value);

	return;
    }

    // Get the entryStats for this entry and data field (create if not there)
    EntryStats *stats = getEntryStats ("UIManager::setDouble", funcName,
				       entryKey, dataAttrTag, MD_DOUBLE, 1);
	
    // Get the specified data field from the entryStats for ease of use
    MD_Field *field = stats->field;

    // Insert (if necessary) this taskId/threadId pair and get the index 
    // to write the value at for this taskId and threadId
    int index = insertPTPair (taskId, threadId);

    // Use real original value, if actual set (in bounds and not NULL)
    double origValue;
    bool update;
    if ((index <= MD_max_element_index(field)) &&
	(field->element[index] != NULL))
    {
	origValue = MD_get_double (field, index);
	update = TRUE;  // Updating existing value
    }

    // Otherwise, default to 0
    else
    {
	origValue = 0.0;
	update = FALSE; // Setting new value
    }

    // Write double at the task/thread-determined index in the field
    MD_set_double (field, index, value);

    // Update stats for this data field
    updateEntryStats (stats, index, value, update, origValue);

    // Emit signal to notify any listeners that a value has been set
    emit doubleSet(funcName, entryKey, dataAttrTag, taskId, threadId, value);
}


// Returns the new, incremented double value at the specified location for 
// the specified task/thread id.  Assumes unset values are 0.0.
// Much more efficient than getDouble() followed by setDouble()!
// Punts if funcName, entryKey, or dataAttrTag undefined or not double dataAttr
double UIManager::addDouble (const char *funcName, const char *entryKey, 
			     const char *dataAttrTag, int taskId, 
			     int threadId, double increment)
{
    // Ignore inf/NAN values, screws up statistics and display
    if (!finite(increment))
    {
	// Print warning that ignoring increment
	fprintf (stderr, 
		 "\n"
		 "Warning: Tool Gear ignoring bad increment '%g' for:\n"
		 "  UIManager::addDouble (%s, %s, %s, %i, %i, %g):\n",
		 increment, funcName, entryKey, dataAttrTag, taskId, threadId, 
		 increment);

	// Return there bad value to them
	return (increment);
    }


    // Get the entryStats for this entry and data field (create if not there)
    EntryStats *stats = getEntryStats ("UIManager::addDouble", funcName,
				       entryKey, dataAttrTag, MD_DOUBLE, 1);


    // Get the specified data field from the entryStats for ease of use
    MD_Field *field = stats->field;

    // Insert (if necessary) this taskId/threadId pair and get the index 
    // to write the value at for this taskId and threadId
    int index = insertPTPair (taskId, threadId);

    // Use real original value, if actual set (in bounds and not NULL)
    double origValue;
    bool update;
    if ((index <= MD_max_element_index(field)) &&
	(field->element[index] != NULL))
    {
	origValue = MD_get_double (field, index);
	update = TRUE;  // Updating existing value
    }

    // Otherwise, default to 0
    else
    {
	origValue = 0.0;
	update = FALSE; // Setting new value
    }

    // Calculate new value
    double newValue = origValue + increment;

    // Write double at the task/thread-determined index in the field
    MD_set_double (field, index, newValue);

    // Update stats for this data field
    updateEntryStats (stats, index, newValue, update, origValue);

    // Emit signal to notify any listeners that a value has been set
    emit doubleSet(funcName, entryKey, dataAttrTag, 
		   taskId, threadId, newValue);

    // Return the new value for this data location
    return (newValue);
}



// Returns the double at the specified location for the task/thread id.
// Returns DOUBLE_NULL if value not previously set.
// Punts if funcName, entryKey, or dataAttrTag undefined or not double dataAttr
double UIManager::getDouble (const char *funcName, const char *entryKey, 
			     const char *dataAttrTag, int taskId, int threadId)
{
    // Get the specified data field in the entry (don't create if not there)
    MD_Field *field = getDataField ("UIManager::getDouble", funcName,
				    entryKey, dataAttrTag, MD_DOUBLE, 0);

    // If doesn't exist, return NULL_DOUBLE
    if (field == NULL)
	return (NULL_DOUBLE);
    
    // Get the index to read the value at for this task and thread
    // Doesn't add taskId/threadId combo if it doesn't exist already
    int index = PTPairIndex (taskId, threadId);

    // If taskId/threadId combo doesn't exist (got NULL_INT), or out
    // of bounds for the field, return NULL_DOUBLE
    if ((index == NULL_INT) || (index > MD_max_element_index(field)))
	return (NULL_DOUBLE);

    // If this particular value has not been set, return NULL_DOUBLE
    if (field->element[index] == NULL)
	return (NULL_DOUBLE);

    // Read double at the task/thread-determined index in the field
    double value = MD_get_double (field, index);

    // Return the value read for this data location
    return (value);
}


// Returns the data value (of type int or double) at the specificied 
// location as a double.  Returns NULL_DOUBLE if value not set.
// Generic version of getInt and getDouble that works for either
// data type (a double can represent all the values of a 32 bit int).
double UIManager::getValue (const char *funcName, const char *entryKey, const char *dataAttrTag, 
			    int taskId, int threadId)
{
    // Get the entryStats for this entry and data field (don't create if 
    // not there)
    EntryStats *stats = getEntryStats ("UIManager::addDouble", funcName,
				       entryKey, dataAttrTag, NULL_INT, 0);

    // If doesn't exist, return NULL_DOUBLE
    if (stats == NULL)
	return (NULL_DOUBLE);

    // Get the specified data field from the entryStats for ease of use
    MD_Field *field = stats->field;
    
    // Get the index to read the value at for this task and thread
    // Doesn't add taskId/threadId combo if it doesn't exist already
    int index = PTPairIndex (taskId, threadId);

    // If taskId/threadId combo doesn't exist (got NULL_INT), or out
    // of bounds for the field, return NULL_DOUBLE
    if ((index == NULL_INT) || (index > MD_max_element_index(field)))
	return (NULL_DOUBLE);

    // If this particular value has not been set, return NULL_DOUBLE
    if (field->element[index] == NULL)
	return (NULL_DOUBLE);

    // Get attribute type
    MD_Element_Req *requirement = field->decl->require[0];

    // Read value based on type
    double value;
    if (requirement->type == MD_DOUBLE)
    {
	// Read double at the task/thread-determined index in the field
	value = MD_get_double (field, index);
    }
    else if (requirement->type == MD_INT)
    {
	// Read int at the task/thread-determined index in the field
	// and convert to double
	value = (double) MD_get_int (field, index);
    }
    else
    {
	TG_error ("UIManager::getValue: unexpected type %i!", 
		requirement->type);
	value = 0;      // avoid compiler warning about uninitialized value
    }

    // Return the value read for this data location
    return (value);
}


// Returns the EntryStat requested for the specified entryKey and 
// dataAttrTag across all the PTPairs that have set values.  
// If taskId/threadId is NULL, these parameter are ignored.
// If taskId/threadId is not NULL, for entryMin and entryMax, the 
// taskId/threadId responsible will be returned (all other stats return
// NULL_INT).
// See enum EntryStat for performance information.
double UIManager::entryDataStat (const char *funcName, const char *entryKey, 
				 const char *dataAttrTag, EntryStat entryStat,
				 int *taskId, int *threadId)
{
    // Get the entryStats for this entry and data field (don't create if 
    // not there)
    EntryStats *stats = getEntryStats ("UIManager::addDouble", funcName,
				       entryKey, dataAttrTag, NULL_INT, 0);

    // If doesn't exist, return NULL_DOUBLE
    if (stats == NULL)
    {
	// If taskId not NULL, initialize to NULL_INT
	if (taskId != NULL)
	    *taskId = NULL_INT;

	// If threadId not NULL, initialize to NULL_INT
	if (threadId != NULL)
	    *threadId = NULL_INT;

	return (NULL_DOUBLE);
    }


    // Get and return stat value using internal helper routine
    return (getEntryDataStat (stats, entryStat, taskId, threadId));
}



// Internal helper routine that returns the specified entryStat from stats.
// Used by entryDataStat, functionDataStat, etc.
double UIManager::getEntryDataStat (EntryStats *stats, EntryStat entryStat,
				    int *taskId, int *threadId)
{
    // If taskId not NULL, initialize to NULL_INT
    if (taskId != NULL)
	*taskId = NULL_INT;
    
    // If threadId not NULL, initialize to NULL_INT
    if (threadId != NULL)
	*threadId = NULL_INT;
    
    // Get data count, must be >= 1 if got here (no data deletes)
    int count = stats->count();  

    // Sanity check, make sure count not < 1
    if (count < 1)
    {
	TG_error ("UIManager::getEntryDataStat: expect count (%i) > 0!", 
		  count);
    }


    // Switch on entryStat to determine what stat to set statVal to
    double statVal;
    int PTPairIndex;
    switch (entryStat)
    {
      case EntrySum:
	statVal = stats->sum();
	break;
	
      case EntrySumOfSquares:
	statVal = stats->sumOfSquares();
	break;

      case EntryCount:
	statVal = (double) count;
	break;

      case EntryMean:
	statVal = stats->sum() / (double)count;
	break;

      case EntryStdDev:
	// If only one data point, make the stdDev exactly 0.0 (skip a bunch
	// of math that may give roundoff error)
	if (count == 1)
	{
	    statVal = 0.0;
	    break;
	}
	
	// Get the sum and sum of squares of all the values
	double sum, sumOfSquares, mean;
	sum = stats->sum();
	sumOfSquares = stats->sumOfSquares();
	
	// Calculate the mean
	mean = sum / (double)count;
	
	// The population standard deviation is the square root of the 
	// "mean of the squares minus the mean square"
	statVal = sqrt ((sumOfSquares/(double)count) - (mean * mean));
	break;

      case EntryMax:
	// Get the max of all the values
	statVal = stats->max();

	// If max needs rebuilding, rebuild stats and do query again
	if (statVal == NULL_DOUBLE)
	{
	    // Rebuild the stats so we can get the max
	    rebuildEntryStats (stats);
	    
	    // Get the max of all the values again (should be set now)
	    statVal = stats->max();
	}
	
	// Get the PTPair index of the max value
	PTPairIndex = stats->maxId();

	// If taskId not NULL, set it to the taskId of the max
	if (taskId != NULL)
	    *taskId = PTPairTaskAt(PTPairIndex);
	
	// If threadId not NULL, set it to the threadId of the max
	if (threadId != NULL)
	    *threadId = PTPairThreadAt (PTPairIndex);
	break;

      case EntryMin:
	// Get the min of all the values
	statVal = stats->min();

	// If min needs rebuilding, rebuild stats and do query again
	if (statVal == NULL_DOUBLE)
	{
	    // Rebuild the stats so we can get the min
	    rebuildEntryStats (stats);
	
	    // Get the min of all the values again (should be set now)
	    statVal = stats->min();
	}
	// Get the PTPair index of the min value
	PTPairIndex = stats->minId();

	// If taskId not NULL, set it to the taskId of the min
	if (taskId != NULL)
	    *taskId = PTPairTaskAt(PTPairIndex);

	
	// If threadId not NULL, set it to the threadId of the min
	if (threadId != NULL)
	    *threadId = PTPairThreadAt (PTPairIndex);
	break;


      default:
	TG_error ("UIManager:: entryDataStat: Unknown entryStat (%i)!",
		  entryStat);
	statVal = 0;    // Avoid compiler warning about uninitialized statVal
    }

    // Return the statVal calculated
    return (statVal);
}


// Returns the AttrStat performed on the EntryStat specified across
// the function's entries (that have data).
// If entryKey is not NULL, returns entry that set min/max for
// AttrMin and AttrMax (NULL_QSTRING otherwise).
// See enums AttrStat and EntryStat for performance information.
double UIManager::functionDataStat (const char *funcName, const char *dataAttrTag,
				    AttrStat functionStat, 
				    EntryStat ofEntryStat,
				    QString *entryKey)
{
    // If entryKey != NULL, initialize to NULL_QSTRING
    if (entryKey != NULL)
	*entryKey = NULL_QSTRING;

    // Get the funcStats for this function and data field (don't create)
    FuncStats *funcStats = getFuncStats ("UIManager::functionDataStat",
					 funcName, dataAttrTag, FALSE);

    // If no funcStats created, there is no data so return NULL_DOUBLE
    if (funcStats == NULL)
	return (NULL_DOUBLE);
    
    // Determine which stats structure to query
    DataStats<double> *queryStats;
    bool needRebuild = FALSE;  // Initially assume don't need to rebuild 
    switch (ofEntryStat)
    {
      case EntrySum:
	queryStats = &funcStats->sumStats;
	break;

      case EntryMean:
	queryStats = &funcStats->meanStats;
        break;

      case EntryMax:
	queryStats = &funcStats->maxStats;
        break;

      case EntryMin:
	queryStats = &funcStats->minStats;
        break;
	
      case EntrySumOfSquares:
      case EntryStdDev:
      case EntryCount:
	// Create miscStats if not already created
	if (funcStats->miscStats == NULL)
	{
	    funcStats->miscStats = new DataStats<double>;
	    TG_checkAlloc(funcStats->miscStats);
	}
	
	// Use miscStats to cache stats calculation
	queryStats = funcStats->miscStats;

	// If have not already cached the desired stats (and have
	// not been cleared by an update, which invalidates miscStatsType)
	// rebuild.  Otherwise, use cached stats "as is".
	if (funcStats->miscStatsType != ofEntryStat)
	{
	    needRebuild = TRUE;
	    funcStats->miscStatsType = ofEntryStat;
	}
	break;

      default:
	TG_error ("UIManager::functionDataStats: Unknown ofEntryStat (%i)!",
		  ofEntryStat);
	queryStats = 0; // Avoid compiler warning
    }
    
    // Determine if need to rebuild stats first (if don't already need rebuild)
    if (!needRebuild)
    {
	switch (functionStat)
	{
	  case AttrSum:
	  case AttrSumOfSquares:
	  case AttrMean:
	  case AttrStdDev:
	    // Rebuild if sum is NULL_DOUBLE
	    if (queryStats->sum() == NULL_DOUBLE)
		needRebuild = TRUE;
	    break;
	    
	    // AttrCount should never need rebuilding!
	  case AttrCount:
	    break;
	    
	  case AttrMax:
	    // Rebuild if max is NULL_DOUBLE
	    if (queryStats->max() == NULL_DOUBLE)
		needRebuild = TRUE;
	    break;
	    
	  case AttrMin:
	    // Rebuild if min is NULL_DOUBLE
	    if (queryStats->min() == NULL_DOUBLE)
		needRebuild = TRUE;
	    break;
	    
	  default:
	    TG_error ("UIManager::functionDataStats: "
		      "Unknown functionStat (%i)!", functionStat);
	}
    }

    // Rebuild queryStats, if necessary
    if (needRebuild)
    {
#if 0
	// DEBUG
	printf ("Rebuilding function Stat %i of entry stat %i\n",
		functionStat, ofEntryStat);
#endif
	
	// Reset stats
	queryStats->resetStats();

	double scanVal;
	// Scan all entries in function that has data set
	for (EntryStats *entryStats = funcStats->firstEntryStats;
	     entryStats != NULL; entryStats = entryStats->nextEntryStats)
	{
	    // Get the ofEntryStat value for this entry
	    scanVal = getEntryDataStat (entryStats, ofEntryStat);

	    // Add scanVal to queryStats (as new value, since rescanning)
	    queryStats->updateStats (entryStats->entryIndex, scanVal,
				     FALSE, 0.0);
	}
    }

    // Get data count, must be >= 1 if got here (no data deletes)
    int count = queryStats->count();  

    // Sanity check, make sure count not < 1
    if (count < 1)
    {
	TG_error ("UIManager::functionDataStat: expect count (%i) > 0!", 
		  count);
    }

    // Get the requested stat from queryStats
    double statVal;
    int entryIndex;
    switch (functionStat)
    {
      case AttrSum:
	statVal = queryStats->sum();
	break;

      case AttrSumOfSquares:
	statVal = queryStats->sumOfSquares();
	break;

      case AttrCount:
	statVal = (double) count;
	break;

      case AttrMean:
	statVal = queryStats->sum() / (double)count;
	break;

      case AttrStdDev:
	// If only one data point, make the stdDev exactly 0.0 (skip a bunch
        // of math that may give roundoff error)
        if (count == 1)
        {
            statVal = 0.0;
            break;
        }

        // Get the sum and sum of squares of all the values
        double sum, sumOfSquares, mean;
        sum = queryStats->sum();
        sumOfSquares = queryStats->sumOfSquares();

        // Calculate the mean
        mean = sum / (double)count;

        // The population standard deviation is the square root of the
        // "mean of the squares minus the mean square"
        statVal = sqrt ((sumOfSquares/(double)count) - (mean * mean));
        break;

      case AttrMax:
	// Get max value
	statVal = queryStats->max();

	// If entryKey not NULL, get entryKey for max
	if (entryKey != NULL)
	{
	    entryIndex = queryStats->maxId();
	    *entryKey = entryKeyAt(funcName, entryIndex);
	}

	break;

      case AttrMin:
	// Get min value
	statVal = queryStats->min();

	// If entryKey not NULL, get entryKey for min
	if (entryKey != NULL)
	{
	    entryIndex = queryStats->minId();
	    *entryKey = entryKeyAt(funcName, entryIndex);
	}
	break;

      default:
	TG_error ("UIManager::functionDataStats: Unknown functionStat (%i)!",
		  functionStat);
	statVal = 0;    // Avoid compiler warning
    }
    
    // Return the functionStat of ofEntryStat calculated above
    return (statVal);
}

// Returns the AttrStat performed on the EntryStat specified across
// the file's entries (that have data).
// If funcName and/or entryKey is not NULL, returns function/entry 
// that set min/max for AttrMin and AttrMax (NULL_QSTRING otherwise).
// See enums AttrStat and EntryStat for performance information.
double UIManager::fileDataStat (const char *fileName, const char *dataAttrTag,
		     AttrStat fileStat, EntryStat ofEntryStat,
		     QString *funcName, QString *entryKey)
{
    // If funcName != NULL, initialize to NULL_QSTRING
    if (funcName != NULL)
	*funcName = NULL_QSTRING;

    // If entryKey != NULL, initialize to NULL_QSTRING
    if (entryKey != NULL)
	*entryKey = NULL_QSTRING;

    // Get the fileStats for this file and data field (don't create)
    FileStats *fileStats = getFileStats ("UIManager::fileDataStat",
					 fileName, dataAttrTag, FALSE);

    // If no fileStats created, there is no data so return NULL_DOUBLE
    if (fileStats == NULL)
	return (NULL_DOUBLE);
    
    // Determine which stats structure to query
    DataStatsIdArray<double,2> *queryStats;
    bool needRebuild = FALSE;  // Initially assume don't need to rebuild 
    switch (ofEntryStat)
    {
      case EntrySum:
	queryStats = &fileStats->sumStats;
	break;

      case EntryMean:
	queryStats = &fileStats->meanStats;
        break;

      case EntryMax:
	queryStats = &fileStats->maxStats;
        break;

      case EntryMin:
	queryStats = &fileStats->minStats;
        break;
	
      case EntrySumOfSquares:
      case EntryStdDev:
      case EntryCount:
	// Create miscStats if not already created
	if (fileStats->miscStats == NULL)
	{
	    fileStats->miscStats = new DataStatsIdArray<double,2>;
	    TG_checkAlloc(fileStats->miscStats);
	}
	
	// Use miscStats to cache stats calculation
	queryStats = fileStats->miscStats;

	// If have not already cached the desired stats (and have
	// not been cleared by an update, which invalidates miscStatsType)
	// rebuild.  Otherwise, use cached stats "as is".
	if (fileStats->miscStatsType != ofEntryStat)
	{
	    needRebuild = TRUE;
	    fileStats->miscStatsType = ofEntryStat;
	}
	break;

      default:
	TG_error ("UIManager::fileDataStats: Unknown ofEntryStat (%i)!",
		  ofEntryStat);
	queryStats = 0; // Avoid compiler warning
    }
    
    // Determine if need to rebuild stats first (if don't already need rebuild)
    if (!needRebuild)
    {
	switch (fileStat)
	{
	  case AttrSum:
	  case AttrSumOfSquares:
	  case AttrMean:
	  case AttrStdDev:
	    // Rebuild if sum is NULL_DOUBLE
	    if (queryStats->sum() == NULL_DOUBLE)
		needRebuild = TRUE;
	    break;
	    
	    // AttrCount should never need rebuilding!
	  case AttrCount:
	    break;
	    
	  case AttrMax:
	    // Rebuild if max is NULL_DOUBLE
	    if (queryStats->max() == NULL_DOUBLE)
		needRebuild = TRUE;
	    break;
	    
	  case AttrMin:
	    // Rebuild if min is NULL_DOUBLE
	    if (queryStats->min() == NULL_DOUBLE)
		needRebuild = TRUE;
	    break;
	    
	  default:
	    TG_error ("UIManager::fileDataStats: "
		      "Unknown fileStat (%i)!", fileStat);
	}
    }

    // Rebuild queryStats, if necessary
    if (needRebuild)
    {
#if 0
	// DEBUG
	printf ("Rebuilding file Stat %i of entry stat %i\n",
		fileStat, ofEntryStat);
#endif
	
	// Reset stats
	queryStats->resetStats();

	double scanVal;

	// Use a two element statIndex array for stats
	// statIndex[0] is funcIndex
	// statIndex[1] is entryIndex in func
	int statIndex[2];

	// Scan all functions in file that have data set
	for (FuncStats *funcStats = fileStats->firstFuncStats;
	     funcStats != NULL; funcStats = funcStats->nextFuncStats)
	{
	    // Set statIndex[0] to funcIndex
	    statIndex[0] = funcStats->funcIndex;

	    // Scan all entries in function that have data set
	    for (EntryStats *entryStats = funcStats->firstEntryStats;
		 entryStats != NULL; entryStats = entryStats->nextEntryStats)
	    {
		// Set statIndex[0] to entryIndex
		statIndex[1] = entryStats->entryIndex;

		// Get the ofEntryStat value for this entry
		scanVal = getEntryDataStat (entryStats, ofEntryStat);
		
		// Add scanVal to queryStats (as new value, since rescanning)
		queryStats->updateStats (statIndex, scanVal, FALSE, 0.0);
	    }
	}
    }

    // Get data count, must be >= 1 if got here (no data deletes)
    int count = queryStats->count();  

    // Sanity check, make sure count not < 1
    if (count < 1)
    {
	TG_error ("UIManager::fileDataStat: expect count (%i) > 0!", 
		  count);
    }

    // Get the requested stat from queryStats
    double statVal;
    int entryIndex, funcIndex;
    switch (fileStat)
    {
      case AttrSum:
	statVal = queryStats->sum();
	break;

      case AttrSumOfSquares:
	statVal = queryStats->sumOfSquares();
	break;

      case AttrCount:
	statVal = (double) count;
	break;

      case AttrMean:
	statVal = queryStats->sum() / (double)count;
	break;

      case AttrStdDev:
	// If only one data point, make the stdDev exactly 0.0 (skip a bunch
        // of math that may give roundoff error)
        if (count == 1)
        {
            statVal = 0.0;
            break;
        }

        // Get the sum and sum of squares of all the values
        double sum, sumOfSquares, mean;
        sum = queryStats->sum();
        sumOfSquares = queryStats->sumOfSquares();

        // Calculate the mean
        mean = sum / (double)count;

        // The population standard deviation is the square root of the
        // "mean of the squares minus the mean square"
        statVal = sqrt ((sumOfSquares/(double)count) - (mean * mean));
        break;

      case AttrMax:
	// Get max value
	statVal = queryStats->max();

	// Only figure out max func/entry if needed
	if ((funcName != NULL) || (entryKey != NULL))
	{
	    // statIndex[0] is the funcIndex
	    funcIndex = queryStats->maxId(0);
	    QString maxFuncName = functionAt(funcIndex);

	    // If funcName not NULL, set funcName for max
	    if (funcName != NULL)
	    {
		*funcName = maxFuncName;
	    }
	    
	    // If entryKey not NULL, set entryKey for max
	    if (entryKey != NULL)
	    {
		// statIndex[1] is the entryIndex
		entryIndex = queryStats->maxId(1);
		*entryKey = entryKeyAt(maxFuncName, entryIndex);
	    }
	}
	break;

      case AttrMin:
	// Get min value
	statVal = queryStats->min();

	// Only figure out min func/entry if needed
	if ((funcName != NULL) || (entryKey != NULL))
	{
	    // statIndex[0] is the funcIndex
	    funcIndex = queryStats->minId(0);
	    QString minFuncName = functionAt(funcIndex);

	    // If funcName not NULL, set funcName for min
	    if (funcName != NULL)
	    {
		*funcName = minFuncName;
	    }
	    
	    // If entryKey not NULL, set entryKey for min
	    if (entryKey != NULL)
	    {
		// statIndex[1] is the entryIndex
		entryIndex = queryStats->minId(1);
		*entryKey = entryKeyAt(minFuncName, entryIndex);
	    }
	}
	break;

      default:
	TG_error ("UIManager::fileDataStats: Unknown fileStat (%i)!",
		  fileStat);
	statVal = 0;    // Avoid compiler warning
    }
    
    // Return the fileStat of ofEntryStat calculated above
    return (statVal);
}

// Returns the AttrStat performed on the EntryStat specified across
// the entire application (that have data).
// If funcName and/or entryKey is not NULL, returns function/entry 
// that set min/max for AttrMin and AttrMax (NULL_QSTRING otherwise).
// See enums AttrStat and EntryStat for performance information.
double UIManager::applicationDataStat ( const char *dataAttrTag, 
					AttrStat applicationStat, 
					EntryStat ofEntryStat,
					QString *funcName, 
					QString *entryKey)
{
    // If funcName != NULL, initialize to NULL_QSTRING
    if (funcName != NULL)
	*funcName = NULL_QSTRING;

    // If entryKey != NULL, initialize to NULL_QSTRING
    if (entryKey != NULL)
	*entryKey = NULL_QSTRING;

    // Get the appStats for this data field (don't create)
    AppStats *appStats = getAppStats ("UIManager::applicationDataStat",
				      dataAttrTag, FALSE);
    
    // If no appStats created, there is no data so return NULL_DOUBLE
    if (appStats == NULL)
	return (NULL_DOUBLE);
    
    // Determine which stats structure to query
    DataStatsIdArray<double,2> *queryStats;
    bool needRebuild = FALSE;  // Initially assume don't need to rebuild 
    switch (ofEntryStat)
    {
      case EntrySum:
	queryStats = &appStats->sumStats;
	break;

      case EntryMean:
	queryStats = &appStats->meanStats;
        break;

      case EntryMax:
	queryStats = &appStats->maxStats;
        break;

      case EntryMin:
	queryStats = &appStats->minStats;
        break;
	
      case EntrySumOfSquares:
      case EntryStdDev:
      case EntryCount:
	// Create miscStats if not already created
	if (appStats->miscStats == NULL)
	{
	    appStats->miscStats = new DataStatsIdArray<double,2>;
	    TG_checkAlloc(appStats->miscStats);
	}
	
	// Use miscStats to cache stats calculation
	queryStats = appStats->miscStats;

	// If have not already cached the desired stats (and have
	// not been cleared by an update, which invalidates miscStatsType)
	// rebuild.  Otherwise, use cached stats "as is".
	if (appStats->miscStatsType != ofEntryStat)
	{
	    needRebuild = TRUE;
	    appStats->miscStatsType = ofEntryStat;
	}
	break;

      default:
	TG_error ("UIManager::applicationDataStats: Unknown ofEntryStat (%i)!",
		  ofEntryStat);
	queryStats = 0; // Avoid compiler warning
    }
    
    // Determine if need to rebuild stats first (if don't already need rebuild)
    if (!needRebuild)
    {
	switch (applicationStat)
	{
	  case AttrSum:
	  case AttrSumOfSquares:
	  case AttrMean:
	  case AttrStdDev:
	    // Rebuild if sum is NULL_DOUBLE
	    if (queryStats->sum() == NULL_DOUBLE)
		needRebuild = TRUE;
	    break;
	    
	    // AttrCount should never need rebuilding!
	  case AttrCount:
	    break;
	    
	  case AttrMax:
	    // Rebuild if max is NULL_DOUBLE
	    if (queryStats->max() == NULL_DOUBLE)
		needRebuild = TRUE;
	    break;
	    
	  case AttrMin:
	    // Rebuild if min is NULL_DOUBLE
	    if (queryStats->min() == NULL_DOUBLE)
		needRebuild = TRUE;
	    break;
	    
	  default:
	    TG_error ("UIManager::applicationDataStats: "
		      "Unknown applicationStat (%i)!", applicationStat);
	}
    }

    // Rebuild queryStats, if necessary
    if (needRebuild)
    {
#if 0
	// DEBUG
	printf ("Rebuilding app Stat %i of entry stat %i\n",
		applicationStat, ofEntryStat);
#endif
	
	// Reset stats
	queryStats->resetStats();

	double scanVal;

	// Use a two element statIndex array for stats
	// statIndex[0] is funcIndex
	// statIndex[1] is entryIndex in func
	int statIndex[2];

	// Scan all files in application that have data set
	for (FileStats *fileStats = appStats->firstFileStats;
	     fileStats != NULL; fileStats = fileStats->nextFileStats)
	{
	    // Scan all functions in file that have data set
	    for (FuncStats *funcStats = fileStats->firstFuncStats;
		 funcStats != NULL; funcStats = funcStats->nextFuncStats)
	    {
		// Set statIndex[0] to funcIndex
		statIndex[0] = funcStats->funcIndex;
		
		// Scan all entries in function that have data set
		for (EntryStats *entryStats = funcStats->firstEntryStats;
		     entryStats != NULL; 
		     entryStats = entryStats->nextEntryStats)
		{
		    // Set statIndex[0] to entryIndex
		    statIndex[1] = entryStats->entryIndex;
		    
		    // Get the ofEntryStat value for this entry
		    scanVal = getEntryDataStat (entryStats, ofEntryStat);
		    
		    // Add scanVal to queryStats 
		    // (as new value, since rescanning)
		    queryStats->updateStats (statIndex, scanVal, FALSE, 0.0);
		}
	    }
	}
    }

    // Get data count, must be >= 1 if got here (no data deletes)
    int count = queryStats->count();  

    // Sanity check, make sure count not < 1
    if (count < 1)
    {
	TG_error ("UIManager::applicationDataStat: expect count (%i) > 0!", 
		  count);
    }

    // Get the requested stat from queryStats
    double statVal;
    int entryIndex, funcIndex;
    switch (applicationStat)
    {
      case AttrSum:
	statVal = queryStats->sum();
	break;

      case AttrSumOfSquares:
	statVal = queryStats->sumOfSquares();
	break;

      case AttrCount:
	statVal = (double) count;
	break;

      case AttrMean:
	statVal = queryStats->sum() / (double)count;
	break;

      case AttrStdDev:
	// If only one data point, make the stdDev exactly 0.0 (skip a bunch
        // of math that may give roundoff error)
        if (count == 1)
        {
            statVal = 0.0;
            break;
        }

        // Get the sum and sum of squares of all the values
        double sum, sumOfSquares, mean;
        sum = queryStats->sum();
        sumOfSquares = queryStats->sumOfSquares();

        // Calculate the mean
        mean = sum / (double)count;

        // The population standard deviation is the square root of the
        // "mean of the squares minus the mean square"
        statVal = sqrt ((sumOfSquares/(double)count) - (mean * mean));
        break;

      case AttrMax:
	// Get max value
	statVal = queryStats->max();

	// Only figure out max func/entry if needed
	if ((funcName != NULL) || (entryKey != NULL))
	{
	    // statIndex[0] is the funcIndex
	    funcIndex = queryStats->maxId(0);
	    QString maxFuncName = functionAt(funcIndex);

	    // If funcName not NULL, set funcName for max
	    if (funcName != NULL)
	    {
		*funcName = maxFuncName;
	    }
	    
	    // If entryKey not NULL, set entryKey for max
	    if (entryKey != NULL)
	    {
		// statIndex[1] is the entryIndex
		entryIndex = queryStats->maxId(1);
		*entryKey = entryKeyAt(maxFuncName, entryIndex);
	    }
	}
	break;

      case AttrMin:
	// Get min value
	statVal = queryStats->min();

	// Only figure out min func/entry if needed
	if ((funcName != NULL) || (entryKey != NULL))
	{
	    // statIndex[0] is the funcIndex
	    funcIndex = queryStats->minId(0);
	    QString minFuncName = functionAt(funcIndex);

	    // If funcName not NULL, set funcName for min
	    if (funcName != NULL)
	    {
		*funcName = minFuncName;
	    }
	    
	    // If entryKey not NULL, set entryKey for min
	    if (entryKey != NULL)
	    {
		// statIndex[1] is the entryIndex
		entryIndex = queryStats->minId(1);
		*entryKey = entryKeyAt(minFuncName, entryIndex);
	    }
	}
	break;

      default:
	TG_error ("UIManager::applicationDataStats: Unknown applicationStat (%i)!",
		  applicationStat);
	statVal = 0;    // Avoid compiler warning
    }
    
    // Return the applicationStat of ofEntryStat calculated above
    return (statVal);
}

// Internal helper routine to quickly rebuild the entry stats when
// the min(), minId(), max(), or maxId() routines say these stats
// cannot be calculated until the stats are rebuilt.
void UIManager::rebuildEntryStats (EntryStats *stats)
{
    // Get the data field the rebuild stats from for ease of use
    MD_Field *field = stats->field;
    
#if 0
    // DEBUG, use MD info to get names
    printf ("*** In rebuildEntryStats for (%s, %s, %s)!\n",
	    field->entry->section->name, field->entry->name, 
	    field->decl->name);
#endif

    // Get attribute type that we are rebuilding
    MD_Element_Req *requirement = field->decl->require[0];
    
    // Get the maxIndex to scan
    int maxIndex = MD_max_element_index (field);
    
    // Sanity check
    // Grab current state and compare after rescan
    // All valid (NON NULL_DOUBLE) values should be the same
    int oldCount = stats->count();

    // With more confidence in results, now just check count()
    // Also, NAN's are messing up these checks 
#if 0
    double oldSum = stats->sum();
    double oldMax = stats->max();
    int oldMaxId = stats->maxId();
    double oldMin = stats->min();
    int oldMinId = stats->minId();
#endif

    // Reset the stats before rescan
    stats->resetStats();
    
    // All values will be cast to double for ease of stat taking
    double value;

    // Do special case loop for double type in order to remove branch from
    // inner loop
    if (requirement->type == MD_DOUBLE)
    {
	// Scan over index's that could have value set
	for (int index = 0; index <= maxIndex; index++)
	{
	    // Only process elements that actually have data in them
	    if (field->element[index] != NULL)
	    {
		// Get the double value
		value = MD_get_double (field, index);

		// Add this value to stats
		stats->updateStats (index, value, FALSE, 0.0);
	    }
	}
    }

    // Do special case loop for int type in order to remove branch from
    // inner loop
    else if (requirement->type == MD_INT)
    {
	// Scan over index's that could have value set
	for (int index = 0; index <= maxIndex; index++)
	{
	    // Only process elements that actually have data in them
	    if (field->element[index] != NULL)
	    {
		// Get the int value and convert to double
		value = (double)MD_get_int (field, index);

		// Add this value to stats
		stats->updateStats (index, value, FALSE, 0.0);
	    }
	}
    }

    // Sanity check, should never get here
    else
    {
	TG_error ("UIManager::rebuildEntryStats: unexpected type %i!", 
		  requirement->type);
    }
    
    // Sanity check, make sure new stats match old stats, in those 
    // cases where the old stats were valid!
    int newCount = stats->count();

    // Counts must always match
    if (newCount != oldCount)
    {
	TG_error ("UIManager::rebuildEntryStats: Algorithm error: "
		  "newCount (%i) != oldCount (%i)", newCount, oldCount);
    }

    // With more confidence in results, now just check count()
    // Also, NAN's are messing up these checks 
#if 0
    double newSum = stats->sum();
    double newSumOfSquares = stats->sumOfSquares();
    double newMax = stats->max();
    int newMaxId = stats->maxId();
    double newMin = stats->min();
    int newMinId = stats->minId();

    // Sums should match, but need to handle rounding errors
    // (Assume if Sum correct, SumOfSquares will also be correct)
    if (newSum != oldSum)
    {
	// Make sure not to close to zero
	if ((newSum > .0000001) || (newSum < -.0000001))
	{
	    // Calculate percent error, make positive
	    double error = (oldSum - newSum) / newSum;
	    if (error < 0.0)
		error = -error;

	    // If percent error to big to be rounding error, punt
	    // otherwise assume rounding error and continue
	    if (error > .0000001)
	    {
		TG_error ("UIManager::rebuildEntryStats: Algorithm error: "
			  "newSum (%g) != oldSum (%g), error %g%%", 
			  newSum, oldSum, (100.0 * error));
	    }
	}
    }

    // If had a non-NULL MaxId before, max and MaxId should match
    if (oldMaxId != NULL_INT)
    {
	if (newMax != oldMax)
	{
	    TG_error ("UIManager::rebuildEntryStats: Algorithm error: "
		      "newMax (%g) != oldMax (%g)", newMax, oldMax);
	}
	
	if (newMaxId != oldMaxId)
	{
	    TG_error ("UIManager::rebuildEntryStats: Algorithm error: "
		      "newMaxId (%g) != oldMaxId (%g)", newMaxId, oldMaxId);
	}
    }

    // If had a non-NULL MinId before, min and MinId should match
    if (oldMinId != NULL_INT)
    {
	if (newMin != oldMin)
	{
	    TG_error ("UIManager::rebuildEntryStats: Algorithm error: "
		      "newMin (%g) != oldMin (%g)", newMin, oldMin);
	}
	
	if (newMinId != oldMinId)
	{
	    TG_error ("UIManager::rebuildEntryStats: Algorithm error: "
		      "newMinId (%g) != oldMinId (%g)", newMinId, oldMinId);
	}
    }

    // If have non-zero count, better have min and max!
    if (newCount > 0)
    {
	if ((newMax == NULL_DOUBLE) || (newMin == NULL_DOUBLE))
	{
	    TG_error ("UIManager::rebuildEntryStats: Algorithm error: "
		      "newMax (%g) or newMin (%g) NULL_DOUBLE", newMax, 
		      newMin);
	}
    }
#endif
}

// Return the name of the program in a QString
QString UIManager::getProgramName()
{
    char * path = a->argv()[0];
    char * pname;

    // Strip off the path prefix, if it exists
    if( (pname = strrchr( path, '/' )) == NULL ) {      // No prefix
        pname = path;
    } else { 
        pname += 1;                 // skip the '/'
    }

    return QString(pname);
}

void UIManager::setMainFont( QFont & f )
{
	if( f != mainFont ) {
		mainFont = f;
#if 0
		fprintf( stderr, "Resetting mainFont to %s\n",
			    mainFont.toString().latin1() );
#endif
		emit mainFontChanged( f );
	}
}

void UIManager::setLabelFont( QFont & f )
{
	if( f != labelFont ) {
		labelFont = f;
#if 0
		fprintf( stderr, "Resetting labelFont to %s\n",
			    labelFont.toString().latin1() );
#endif
		emit labelFontChanged( f );
	}
}

// Creates a message folder referenced by messageFolderTag and
// displayed with messageFolderTitle.
// If message folder already exists, does nothing and returns -1.
// Otherwise, returns index of messageFolder (>= 0)
int UIManager::declareMessageFolder (const char *messageFolderTag, 
				     const char *messageFolderTitle,
				     PolicyIfEmpty ifEmpty)
{
    // Sanity check on ifEmpty values
    if ((ifEmpty != UIManager::ShowIfEmpty) &&
	(ifEmpty != UIManager::HideIfEmpty) &&
	(ifEmpty != UIManager::DisableIfEmpty))
    {
	TG_error("UIManager::declareMessageFolder: Invalid ifEmpty value %i\n",
		 (int)ifEmpty);
    }
	
    // If message folder already exists, do nothing and return -1
    if (MD_find_entry (messageFolderSection, messageFolderTag) != NULL)
    {
	return (-1);
    }
    
    // Add messageFolder tag to indexed messageFolder folder and get index
    int index;
    MD_Entry *messageFolderEntry = newIndexedEntry (messageFolderSection,
						  messageFolderTag,
						  messageFolderIndexDecl,
						  messageFolderMap,
						  &index);

    // Set title for this messageFolder
    MD_Field *titleField = MD_new_field (messageFolderEntry, 
					 messageFolderTitleDecl, 1);
    MD_set_string (titleField, 0, messageFolderTitle);


    // Set if_empty policy for this messageFolder
    MD_Field *ifEmptyField = MD_new_field (messageFolderEntry, 
					   messageFolderIfEmptyDecl, 1);
    MD_set_int (ifEmptyField, 0, (int) ifEmpty);


    //
    // Add new 'message' sections for this message folder.
    // Prefix messageFolderTag with M_ to prevent name conflicts
    // with internal section names such as _taskId_.
    //
    
    QString messageSectionName;
    messageSectionName.sprintf ("M_%s", messageFolderTag);
    MD_Field_Decl *indexDecl;
    INT_Symbol_Table *indexMap;
    MD_Section *messageSection = 
	newIndexedSection (messageSectionName.latin1(), &indexDecl, &indexMap);
    

    // Add STRING* _messageText_ to hold the message text
    MD_Field_Decl *messageTextDecl = MD_new_field_decl (messageSection,
							"_messageText_",
							MD_REQUIRED_FIELD);
    MD_require_string (messageTextDecl, 0);
    MD_kleene_star_requirement (messageTextDecl, 0);

    
    // Add STRING _messageTraceback_ to hold message traceback (source)
    MD_Field_Decl *messageTracebackDecl = 
	MD_new_field_decl (messageSection,
			   "_messageTraceback_",
			   MD_OPTIONAL_FIELD);
    MD_require_string (messageTracebackDecl, 0);

    // To speed up look of message sections (I don't want to have to
    // to prefix it every time), source MessageFolderInfo struction with
    // the messageSection pointer and the field decl pointers in this
    // table that can be looked up with messageFolderTag
    messageFolderInfoTable.addEntry (messageFolderTag,
	   new MessageFolderInfo (messageSection, 
				indexDecl,
				indexMap,
				messageTextDecl,
				messageTracebackDecl));

    // Emit signal to notify any listeners that a messageFolder has been declared
    emit messageFolderDeclared (messageFolderTag, messageFolderTitle,
				ifEmpty);

#if 0
    // DEBUG
    printf ("UIManager::messageFolderDeclared (\"%s\", \"%s\");\n",
	    messageFolderTag, messageFolderTitle);
#endif

    // Return index this messageFolder inserted at
    return (index);
}

// Returns the number of message folders currently declared
int UIManager::messageFolderCount()
{
    // Get count from MD table that holds messageFolder tags
    int count = MD_num_entries (messageFolderSection);

    // Return this count
    return (count);
}

// Returns the messageFolderTag at index (0 - count-1), NULL_QSTRING if
// out of bounds.
QString UIManager::messageFolderAt (int index)
{
    QString *messageFolderPtr = 
	(QString *)INT_find_symbol_data(messageFolderMap, index);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (messageFolderPtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*messageFolderPtr);    
}


// Returns the index of messageFolderTag, NULL_INT if not found
int UIManager::messageFolderIndex (const char *messageFolderTag)
{
    // Get index using internal helper routine and return it
    int index = getFieldInt(messageFolderSection, messageFolderTag, 
			    messageFolderIndexDecl, 0);

    return (index);
}


// Returns title of messageFolder, NULL_QSTRING if messageFolderTag not found
QString UIManager::messageFolderTitle(const char *messageFolderTag)
{
    // Get title using internal helper routine and return it
    QString title = getFieldQString(messageFolderSection, 
				    messageFolderTag,
				    messageFolderTitleDecl, 0);
    return (title);
}

// Returns ifEmpty policy of messageFolder.
// Returns InvalidIfEmpty if messageFolderTag not found
UIManager::PolicyIfEmpty
UIManager::messageFolderIfEmpty(const char *messageFolderTag)
{
    // Get ifEmpty value using internal helper routine and return it
    int ifEmptyInt = getFieldInt(messageFolderSection, 
				 messageFolderTag,
				 messageFolderIfEmptyDecl, 0);

    if (ifEmptyInt == NULL_INT)
	return (UIManager::InvalidIfEmpty);
    else
	return ((UIManager::PolicyIfEmpty)ifEmptyInt);
}


// Creates and adds a message to the messageFolder.  Messages are added 
// to an specific messageFolder (messageFolderTag) and consist of one or
// more lines of messageText and zero or more lines of messageTraceback
// information (several independent tracebacks can be specified and each
// traceback may be several stack level deep, if desired).
// Messages currently cannot be modified or deleted after being added. 
// Currently, automatically creates message tags (M0, M1, etc.) to
// allow same interface as other UIManager calls.  
//
// MessageText Format:
// Use newlines (\n) to get multiline messages (only first line shown
// unless expanded).  
//
// MessageTraceback Format:
// It may be NULL or "" to indicate no tracback.
// Each traceback consists of a title and then one or more location
// specifiers separated by newlines.  Here is the basic traceback
// format for one traceback and one level of traceback:
// "^Title\n<file_name:line_no1>function_name"
//
// If function_name or line_no is not known, it can be left out, 
// but the display may not do well initially if don't have both file 
// and line number.  Also, function_name may be used as a comment
// field (although it will be currently labeled function).
//
// Additional levels of traceback may be specified after the
// first level, separated by newlines(\n), like this:
// "^Title\n<file1.c:line_no1>func1\n<file2.c:line_no2>func2"
//
// Multiple different tracebacks can be specified by
// specifying another title (i.e.,Title2) and traceback after 
// the first one.
// That is:
// "^Title1\n<file1.c:line_no1>func1\n^Title2\n<file2.c:line_no2>func2"
// 
// In theory, any number of tracebacks may be specified 
// (up to three tested)
//
// It is important to not put spaces before '^' and '<'.  Unexpected
// parsing results may occur otherwise (now or in the future).
// 
// Return's index of message inserted.
int UIManager::addMessage (const char *messageFolderTag, 
			   const char *messageText,
			   const char *messageTraceback)
{
    // Make sure the messageFolderTag has been declared
    MessageFolderInfo *mi = 
	messageFolderInfoTable.findEntry (messageFolderTag);

    if (mi == NULL)
    {
	TG_error ("UIManager::addMessage: messageFolderTag '%s' not declared!",
		  messageFolderTag);
    }

    // For now, I don't think there is any reason to force the user
    // to generate a messageTag, so just generate one for them.
    // If it is needed later, we can change this routine.
    QString messageTag;
    messageTag.sprintf("M%i", MD_num_entries(mi->messageSection));

    // Create message in this messageFolder, get index
    int index;
    MD_Entry *messageEntry = newIndexedEntry (mi->messageSection, 
					      messageTag.latin1(),
					      mi->indexDecl, mi->indexMap,
					      &index);
    
    // Create and set text field
    MD_Field *messageTextField = 
	MD_new_field (messageEntry, mi->messageTextDecl, 1);
#if 0
    MD_set_string (messageTextField, 0, messageText);

#else

    // If message is totally empty, create empty message
    if (messageText[0] == 0)
    {
	// Set first line to 'empty' contents 
	MD_set_string (messageTextField, 0, "");
    }

    // Otherwise, parse message into lines and put each line
    // into the field at index (lineNo -1)
    else
    {
	// First count how many lines are in message text, so we can handle
	// the huge 65k line BGL messages in linear time instead of nlog(n).
	int lineCount = 0;
	
	// Get pointer to message text for parsing
	const char *scanPtr = messageText;

	// Every newline increments lineCount, special processing at 
	// the end handles the last line not terminated with a newline
	char ch;
	while ((ch = *scanPtr) != 0)
	{
	    if (ch == '\n')
		lineCount++;
	    scanPtr++;
	}

	// If buffer not empty and the character before the terminator is 
	// not '\n', increment line count.  
	if ((messageText[0] != 0) && (scanPtr[-1] != '\n'))
	    lineCount++;

	// If lineCount > 1, preallocate all the database entries by
	// writing "" into the last line (will be filled in for real below)
	if (lineCount > 1)
	{
	    // Allocate all the lines needed up front (0 based, so subtract 1)
	    MD_set_string (messageTextField, lineCount-1, "");
	}

	// Parse and write each line into the MD database
	int lineNo = 0;
	const char *parsePtr = messageText;
	while (lineBuf.getNextLine(parsePtr))
	{
	    // Set line contents 
	    MD_set_string (messageTextField, lineNo, lineBuf.contents());
	    
	    // Update line count
	    lineNo ++;

#if 0	    
	    // DEBUG
	    TG_timestamp ("%s %s line %i: '%s'\n", messageFolderTag, 
			  messageTag.latin1(), lineNo, lineBuf.contents());
#endif
	}

#if 0
	// DEBUG
	TG_timestamp ("%s %s lines %i linecount %i max_field_index %i\n", messageFolderTag, 
		      messageTag.latin1(), lineNo, lineCount, 
		      MD_num_elements(messageTextField));
#endif
    }

#endif

    // Create and set messageTraceback field, if not empty
    if ((messageTraceback != NULL) && 
	(messageTraceback[0] != 0))
    {
	MD_Field *messageTracebackField =
	    MD_new_field (messageEntry, mi->messageTracebackDecl, 1);
	MD_set_string (messageTracebackField, 0, messageTraceback);
    }

    // Emit signal to notify any foldereners that a message has been added
    emit messageAdded (messageFolderTag, messageText, messageTraceback);

#if 0
// DEBUG
    printf ("UIManager::addMessage (\"%s\", \"%s\", \"%s\");\n",
	    messageFolderTag, messageText, messageTraceback);
#endif

    // Return the index of the new message
    return (index);
}

// Returns the number of messages currently in messageFolder
int UIManager::messageCount(const char *messageFolderTag)
{
    // Get the messageFolderInfo for this messageFolderTag 
    MessageFolderInfo *mi = 
	messageFolderInfoTable.findEntry (messageFolderTag);

    // Return NULL_INT of messageFolderTag doesn't exist
    if (mi == NULL)
	return (NULL_INT);

    // Get count from MD table that hold this message folder's entries
    int count = MD_num_entries (mi->messageSection);

    // Return this count
    return (count);
}

// Returns the messageTag in messageFolder at index (0 - count-1), 
// NULL_QSTRING if out of bounds.
QString UIManager::messageAt (const char *messageFolderTag, int index)
{
    // Get the messageFolderInfo for this messageFolderTag 
    MessageFolderInfo *mi = 
	messageFolderInfoTable.findEntry (messageFolderTag);

    // Return NULL_QSTRING of messageFolderTag doesn't exist
    if (mi == NULL)
	return (NULL_QSTRING);

    // Look up tag at this index
    QString *messageTagPtr = 
	(QString *) INT_find_symbol_data (mi->indexMap, index);

    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (messageTagPtr == NULL)
	return (NULL_QSTRING);

    // Otherwise, return QString pointed at
    else
	return (*messageTagPtr);
    
}

// Returns the index of messageTag in messageFolder, NULL_INT if not found
int UIManager::messageIndex (const char *messageFolderTag, 
			     const char *messageTag)
{
    // Get the messageFolderInfo for this messageFolderTag 
    MessageFolderInfo *mi = 
	messageFolderInfoTable.findEntry (messageFolderTag);

    // Return NULL_INT of messageFolderTag doesn't exist
    if (mi == NULL)
	return (NULL_INT);

    // Get messageIndex using internal helper routine and return it
    int index = getFieldInt (mi->messageSection, messageTag, 
			     mi->indexDecl, 0);
    return (index);
}

// Returns text for message, NULL_QSTRING if messageTag not found
QString UIManager::messageText(const char *messageFolderTag, 
			       const char *messageTag)
{
    // Get the messageFolderInfo for this messageFolderTag 
    MessageFolderInfo *mi = 
	messageFolderInfoTable.findEntry (messageFolderTag);

    // Return NULL_QSTRING of messageFolderTag doesn't exist
    if (mi == NULL)
	return (NULL_QSTRING);

#if 0
    // Get text using internal helper routine and return it
    QString text = getFieldQString(mi->messageSection, 
				   messageTag,
				   mi->messageTextDecl, 0);
    return (text);
#else
    // Get the entry for the entryKey in this function's passed section
    MD_Entry *entry = MD_find_entry (mi->messageSection, messageTag);

    // Return NULL_QSTRING if messageTag doesn't exist
    if (entry == NULL)
	return (NULL_QSTRING);

    // Get the field declaration for the messageText
    MD_Field_Decl *messageTextDecl = mi->messageTextDecl;

    // Get the field in the entry
    MD_Field *messageTextField = MD_find_field(entry, messageTextDecl);

    // COME BACK AND MAKE strcpy and appendStrcpy

    // Put the first line of the message 
    const char *firstLine = MD_get_string (messageTextField, 0);
    lineBuf.sprintf ("%s", firstLine);

    // Append all the remaininglines
    int lineCount = MD_num_elements(messageTextField);
    for (int index = 1; index < lineCount; ++index)
    {
	const char *nextLine = MD_get_string (messageTextField, index);
	lineBuf.appendSprintf ("\n%s", nextLine);
    }

    // Return the lineBuf contents, automatically converted to QString
    return (lineBuf.contents());
#endif
}

// Returns the number of lines in the message
int UIManager::messageTextLineCount (const char *messageFolderTag, 
				     const char *messageTag)
{
    // Get the messageFolderInfo for this messageFolderTag 
    MessageFolderInfo *mi = 
	messageFolderInfoTable.findEntry (messageFolderTag);

    // Return 0 of messageFolderTag doesn't exist
    if (mi == NULL)
	return (0);

    // Get the entry for the entryKey in this function's passed section
    MD_Entry *entry = MD_find_entry (mi->messageSection, messageTag);

    // Return NULL_INT if messageTag doesn't exist
    if (entry == NULL)
	return (0);

    // Get the field declaration for the messageText
    MD_Field_Decl *messageTextDecl = mi->messageTextDecl;

    // Get the field in the entry
    MD_Field *messageTextField = MD_find_field(entry, messageTextDecl);

    // Get the number of lines in message
    int lineCount = MD_num_elements(messageTextField);
    
    // Return this
    return (lineCount);
}

// Returns actual ptr to a line of the message text
// Note: lineNo starts at 1!
const char *UIManager::messageTextLineRef (const char *messageFolderTag, 
					   const char *messageTag,
					   int lineNo)
{
    // Get the messageFolderInfo for this messageFolderTag 
    MessageFolderInfo *mi = 
	messageFolderInfoTable.findEntry (messageFolderTag);

    // Return NULL of messageFolderTag doesn't exist
    if (mi == NULL)
	return (NULL);

    // Get the entry for the entryKey in this function's passed section
    MD_Entry *entry = MD_find_entry (mi->messageSection, messageTag);

    // Return NULL if messageTag doesn't exist
    if (entry == NULL)
	return (NULL);

    // Get the field declaration for the messageText
    MD_Field_Decl *messageTextDecl = mi->messageTextDecl;

    // Get the field in the entry
    MD_Field *messageTextField = MD_find_field(entry, messageTextDecl);

    // Get the number of lines
    int lineCount = MD_num_elements(messageTextField);
    
    // If out of bounds, return NULL
    if ((lineNo < 0) || (lineNo >= lineCount))
	return (NULL);

    // Get the actual pointer to the message line
    const char *linePtr = MD_get_string (messageTextField, lineNo);

    // Return the actual pointer to calling routine
    return (linePtr);
}

// Returns traceback for the  message, 
// NULL_QSTRING if messageTag not found
QString UIManager::messageTraceback(const char *messageFolderTag, 
				    const char *messageTag)
{
    // Get the messageFolderInfo for this messageFolderTag 
    MessageFolderInfo *mi = 
	messageFolderInfoTable.findEntry (messageFolderTag);

    // Return NULL_QSTRING of messageFolderTag doesn't exist
    if (mi == NULL)
	return (NULL_QSTRING);

    // Get traceback using internal helper routine and return it
    QString traceback = getFieldQString(mi->messageSection, 
					messageTag,
					mi->messageTracebackDecl, 0);
    return (traceback);
}

// Registers a site priority modifier.  If non-empty regular expressions
// are specified and they match the site file,desc, or line, then the 
// priorityModifier will be added to the site display priority.
// The highest site display priority will be displayed by default
// in the message traceback.  Negative priorityModifiers are allowed 
// and useful for filtering out known unuseful content.
int UIManager::addSitePriority (double priorityModifier, 
				const char *fileRegExp,
				const char *descRegExp, 
				const char *lineRegExp)
{
    // Sanity check, don't expect all regular expressions to be NULL
    if ((fileRegExp == NULL) && (descRegExp == NULL) &&
	(lineRegExp == NULL))
    {
	fprintf (stderr, 
		 "Warning ignoring "
		 "UIManager::addSitePriority (%g, NULL, NULL, NULL)!\n",
		 priorityModifier);

	return NULL_INT;
    }

    // For now, I don't think there is any reason to force the user
    // to generate a sitePriorityTag, so just generate one for them.
    // If it is needed later, we can change this routine.
    QString sitePriorityTag;
    sitePriorityTag.sprintf("P%i", MD_num_entries(sitePrioritySection));

    // Create sitePriority in this sitePriorityFolder, get index
    int index;
    MD_Entry *sitePriorityEntry = newIndexedEntry (sitePrioritySection, 
						   sitePriorityTag.latin1(),
						   sitePriorityIndexDecl, 
						   sitePriorityMap,
						   &index);
    
    // Create and set priority modifier field
    MD_Field *sitePriorityModifierField = 
	MD_new_field (sitePriorityEntry, sitePriorityModifierDecl, 1);
    MD_set_double (sitePriorityModifierField, 0, priorityModifier);

    // If fileRegExp set, add it to entry
    if (fileRegExp != NULL)
    {
	// Create and set fileRegExp field
	MD_Field *sitePriorityFileField = 
	    MD_new_field (sitePriorityEntry, sitePriorityFileDecl, 1);
	MD_set_string (sitePriorityFileField, 0, fileRegExp);
    }

    // If descRegExp set, add it to entry
    if (descRegExp != NULL)
    {
	// Create and set descRegExp field
	MD_Field *sitePriorityDescField = 
	    MD_new_field (sitePriorityEntry, sitePriorityDescDecl, 1);
	MD_set_string (sitePriorityDescField, 0, descRegExp);
    }

    // If lineRegExp set, add it to entry
    if (lineRegExp != NULL)
    {
	// Create and set lineRegExp field
	MD_Field *sitePriorityLineField = 
	    MD_new_field (sitePriorityEntry, sitePriorityLineDecl, 1);
	MD_set_string (sitePriorityLineField, 0, lineRegExp);
    }

    // Return index of created site priority entry
    return (index);
}

// Returns the number of sitePriority entries
int UIManager::sitePriorityCount()
{
    // Get count from MD table that holds priority entries
    int count = MD_num_entries (sitePrioritySection);

    // Return this count
    return (count);
}

// Returns the sitePriorityTag at index (0 - count-1), 
// NULL_QSTRING if out of bounds.
QString UIManager::sitePriorityAt (int index)
{
    QString *priorityTagPtr =
        (QString *)INT_find_symbol_data(sitePriorityMap, index);
    
    // If pointer NULL, index out of bounds, return NULL_QSTRING
    if (priorityTagPtr == NULL)
        return (NULL_QSTRING);
    
    // Otherwise, return QString pointed at
    else
        return (*priorityTagPtr);

}

// Returns the index of sitePriorityTag, NULL_INT if not found
int UIManager::sitePriorityIndex (const char *sitePriorityTag)
{
    // Get index using internal helper routine and return it
    int index = getFieldInt(sitePrioritySection, sitePriorityTag,
                            sitePriorityIndexDecl, 0);
    return (index);
}

// Returns modifier for sitePriority, NULL_DOUBLE if not found
double UIManager::sitePriorityModifier(const char *sitePriorityTag)
{
    // Get modifier using internal helper routine and return it
    double modifier = getFieldDouble(sitePrioritySection, sitePriorityTag,
				     sitePriorityModifierDecl, 0);

    return (modifier);

}

// Returns fileRegExp for sitePriority, NULL_QSTRING if not set
QString UIManager::sitePriorityFile(const char *sitePriorityTag)
{
    // Get fileRegExp using internal helper routine and return it
    QString fileRegExp = getFieldQString(sitePrioritySection, sitePriorityTag,
					 sitePriorityFileDecl, 0);
    return (fileRegExp);
}

// Returns descRegExp for sitePriority, NULL_QSTRING if not set
QString UIManager::sitePriorityDesc(const char *sitePriorityTag)
{
    // Get descRegExp using internal helper routine and return it
    QString descRegExp = getFieldQString(sitePrioritySection, sitePriorityTag,
                                       sitePriorityDescDecl, 0);
    return (descRegExp);
}

// Returns lineRegExp for sitePriority, NULL_QSTRING if not set
QString UIManager::sitePriorityLine(const char *sitePriorityTag)
{
    // Get lineRegExp using internal helper routine and return it
    QString lineRegExp = getFieldQString(sitePrioritySection, sitePriorityTag,
                                       sitePriorityLineDecl, 0);
    return (lineRegExp);
}


#if 0
// Declare a data column for sites (typically source files) referenced 
// by message annotations.  
// Must provide siteColumnTag, title, and toolTip for the column, 
// the rest have semi-reasonable defaults.   
// Position specifies the location of the column relative to the source 
// file (negative positions are before the source, positive after, ties
// broken by declaration order). 
// The Column's siteMask is ANDed with the message annnotation's site mask
// and if the value is non-zero, the column is shown.
// If hideIfEmpty is TRUE, the column will be hidden if would be empty 
// for the file being displayed, otherwise it is always shown (default).
// The current valid values for align (the ColumnAlign enum) is AlignAuto,
// AlignRight, and AlignLeft.
// The minWidth (default 0), provides a minimum width hint (in 
// characters) to the GUI (there may be other minimum width constraints).
// The maxWidth (default 1000), provides a minimum width hint (in
// characters) to the GUI (there may be other maximum width constraints).
// Returns index of the new site column (-1 if already exists)
int UIManager::declareSiteColumn (const char *siteColumnTag, 
				  const char *title, 
				  const char *toolTip,
				  int position,
				  unsigned int siteMask,
				  bool hideIfEmpty,
				  ColumnAlign align,
				  int minWidth,
				  int maxWidth)
{
    // If siteColumn already exists, do nothing
    if (MD_find_entry (siteColumnSection, siteColumnTag) != NULL)
    {
	fprintf (stderr, "Warning: UIManager::declareSiteColumn: "
		 "Ignoring redeclaration of tag '%s'!\n", siteColumnTag);
	return (-1);
    }
    
    // Add siteColumn tag to indexed siteColumn folder and get index
    int index;
    MD_Entry *siteColumnEntry = newIndexedEntry (siteColumnSection,
						 siteColumnTag,
						 siteColumnIndexDecl,
						 siteColumnMap,
						 &index);
    
    // Set title for this siteColumn
    MD_Field *titleField = MD_new_field (siteColumnEntry, 
					 siteColumnTitleDecl, 1);
    MD_set_string (titleField, 0, title);

    // COME BACK, what MD structure are we creating for columns?
    
    //
    // Add new 'SiteColumn' sections for this site column folder.
    // Prefix siteColumnTag with SC_ to prevent name conflicts
    // with internal section names such as _taskId_.
    //
    
    QString messageSectionName;
    messageSectionName.sprintf ("M_%s", siteColumnTag);
    MD_Field_Decl *indexDecl;
    INT_Symbol_Table *indexMap;
    MD_Section *messageSection = 
	newIndexedSection (messageSectionName.latin1(), &indexDecl, &indexMap);
    

    // Add STRING* _messageText_ to hold the message text
    MD_Field_Decl *messageTextDecl = MD_new_field_decl (messageSection,
							"_messageText_",
							MD_REQUIRED_FIELD);
    MD_require_string (messageTextDecl, 0);
    MD_kleene_star_requirement (messageTextDecl, 0);

    
    // Add STRING _messageTraceback_ to hold message traceback (source)
    MD_Field_Decl *messageTracebackDecl = 
	MD_new_field_decl (messageSection,
			   "_messageTraceback_",
			   MD_OPTIONAL_FIELD);
    MD_require_string (messageTracebackDecl, 0);

    // To speed up look of message sections (I don't want to have to
    // to prefix it every time), source MessageFolderInfo struction with
    // the messageSection pointer and the field decl pointers in this
    // table that can be looked up with siteColumnTag
    siteColumnInfoTable.addEntry (siteColumnTag,
	   new MessageFolderInfo (messageSection, 
				indexDecl,
				indexMap,
				messageTextDecl,
				messageTracebackDecl));

    // Emit signal to notify any listeners that a siteColumn has been declared
    emit siteColumnDeclared (siteColumnTag, siteColumnTitle);

#if 0
    // DEBUG
    printf ("UIManager::siteColumnDeclared (\"%s\", \"%s\");\n",
	    siteColumnTag, siteColumnTitle);
#endif

    // Return index this siteColumn inserted at
    return (index);
    
}

#endif

// Maximum number of folders that may be specified in the body of a message
// Need 2 for Valgrind, put 5 for now since appears more than enough
#define MAX_MESSAGE_FOLDERS 5

class UIXMLParser : public QXmlDefaultHandler
{
    
// Convert element names into enums for quick checking of state
enum XMLElementToken
{
    XML_NULL,
    XML_unknown,
    XML_format,
    XML_version,
    XML_about,
    XML_tool_title,
    XML_prepend,
    XML_append,
    XML_message_folder,
    XML_message,
    XML_tag,
    XML_title,
    XML_if_empty,
    XML_folder,
    XML_heading,
    XML_body,
    XML_annot,
    XML_site,
    XML_file,
    XML_line,
    XML_desc,
    XML_site_column,
    XML_site_mask,
    XML_hide_if_empty,
    XML_position,
    XML_tooltip,
    XML_align,
    XML_min_width,
    XML_max_width,
    XML_site_data,
    XML_col,
    XML_set,
    XML_l,
    XML_v,
    XML_site_priority,
    XML_modifier,
    XML_status
};

public:
    UIXMLParser(UIManager *uimanager, QString &xml, int lineNoOffset) : 
	um(uimanager), xml_text(xml), lineOffset(lineNoOffset),
	lineNoGuess(1) 
	{
	    // Assume using the latest format and version
	    xml_format = 1;
	    xml_version = um->getToolGearVersion();
	    
	    
	    // Populate XML id table, if not already populated
	    if (!um->xmlTokenTable.entryExists("about"))
	    {

		// Use C preprocessor tricks to prevent mistakes and
		// make it easier.   The #name expands to "(name)"
		// and XML_##name expands to the value defined for
		// XML_(name).
#define declareToken(name,level) declareToken_(#name,XML_##name,level)

//		fprintf (stderr, "Initializing xmlTokenTable!\n");

		// Declare all the valid tokens and the levels they
		// are expected to occur on

		// XML format tokens
		declareToken(format, 0);
		declareToken(version, 0);

		// about tokens
		declareToken(about, 0);
		declareToken(prepend, 1);
		declareToken(append, 1);

		// Tool Title tokens
		declareToken(tool_title, 0);

		// message_folder tokens
		declareToken(message_folder, 0);
		declareToken(tag, 1);
		declareToken(title, 1);
		declareToken(if_empty, 1);

		// message tokens
		declareToken(message, 0);
		declareToken(title, 2);
		declareToken(folder, 1);
		declareToken(heading, 1);
		declareToken(body, 1);
		declareToken(annot, 1);

		// message->annot->site tokens
		declareToken(site, 2);
		declareToken(file, 3);
		declareToken(line, 3);
		declareToken(desc, 3);

		// site_column tokens
		declareToken(site_column, 0);
		declareToken(site_mask, 1);
		declareToken(hide_if_empty, 1);
		declareToken(position, 1);
		declareToken(tooltip, 1);
		declareToken(align, 1);
		declareToken(min_width, 1);
		declareToken(max_width, 1);

		// site_data tokens
		declareToken(site_data, 0);
		declareToken(col, 1);
		declareToken(file, 1);
		declareToken(set, 1);
		declareToken(l, 2);
		declareToken(v, 2);

		// site_priority tokens
		declareToken(site_priority, 0);
		declareToken(file, 1);
		declareToken(line, 1);
		declareToken(desc, 1);
		declareToken(modifier, 1);

		// Tool status tokens
		declareToken(status, 0);
	    }
	}

    bool startDocument() 
	{
	    nestLevel = -1;
	    for (int i=0; i < 10; i++)
	    {
		elementTokenAt[i] = XML_NULL;
		elementNameAt[i] = "";
		valueAt[i] = "";
	    }
	    // No error, continue parsing
	    return (TRUE);
	}

    bool startElement( const QString&, const QString&, 
		       const QString& elementName,
                       const QXmlAttributes& )
	{
	    // Ignore "tool_gear" and "tool_gear_XML_snippet" at level -1
	    // (before any real token) in order to simplify parsing logic 
	    // (i.e., message_folder always happens at level 0, 
	    // indepent of the presence of "<tool_gear>" or 
	    // "<tool_gear_XML_snippet>".
	    if ((nestLevel == -1) &&
		((elementName == "tool_gear") ||
		 (elementName == "tool_gear_XML_snippet")))
	    {
		// No error, continue parsing
		return (TRUE);
	    }

	    // Increment level before processing startElement
	    ++nestLevel;

	    // Sanity check, nestLevel better be bounded
	    if ((nestLevel < 0) || (nestLevel > 100))
	    {
		fprintf (stderr, "Error parsing XML: "
			 "nestLevel (%i) out of bounds (0-100)!\n", nestLevel);
		exit (1);
	    }

	    // Get token based on elementName and nestLevel
	    // If name known but not for nestLevel, XML_known will be returned
	    XMLElementToken elementToken = getToken (elementName, nestLevel);

	    // Do any required special handling of level 0 tokens
	    if (nestLevel == 0)
	    {
		if (elementToken == XML_message_folder)
		{
		    // Clear declare message folder parameter, expect to be set
		    // by XML inside message_folder
		    message_folder_tag = "";
		    message_folder_title = "";
		    message_folder_if_empty = UIManager::InvalidIfEmpty;
		}
		else if (elementToken == XML_message)
		{
		    // Clear add message parameters, expect at least folder
		    // and heading to be set by XML inside message_folder
		    for (int i = 0; i < MAX_MESSAGE_FOLDERS; i++)
			message_folder[i] = "";
		    message_folder_count = 0;
		    message_heading = "";
		    message_body = "";
		    message_traceback = "";
		}
		
		else if (elementToken == XML_site_priority)
		{
		    // Clear add site priority parameters
		    site_priority_file = NULL_QSTRING;
		    site_priority_desc = NULL_QSTRING;
		    site_priority_line = NULL_QSTRING;
		    site_priority_modifier = NULL_DOUBLE;
		}
		
		else if (elementToken == XML_site_column)
		{
		    // Clear add site_column required parameters
		    site_column_tag = "";
		    site_column_title = "";

		    // Clear add site_column optional parameters
		    site_column_site_mask = NULL_INT;
		    site_column_hide_if_empty = NULL_INT;
		    site_column_position = NULL_INT;
		    site_column_tooltip = "";
		    site_column_align = "";
		    site_column_min_width = NULL_INT;
		    site_column_max_width = NULL_INT;
		}

		else if (elementToken == XML_site_data)
		{
		    // Clear add site_column required parameters
		    site_data_col = "";
		    site_data_file = "";

		    // The optional 'set' commands must be after the above
		    // are set
		}
	    }

	    // Do any required special handling of level 1 tokens
	    else if (nestLevel == 1)
	    {
		if (elementToken == XML_annot)
		{
		    // Clear annotation parameters
		    annot_title = "";
		    annot_traceback = "";
		}
	    }


	    // Do any required special handling of level 2 tokens
	    else if (nestLevel == 2)
	    {
		if (elementToken == XML_site)
		{
		    // Clear site parameters
		    site_file = "";
		    site_line = "";
		    site_desc = "";
		}
	    }

	    // For now (aganst XML standard) warn about unknown element names
	    if (elementToken == XML_unknown)
	    {
		// Want to warn only once per unknown element name per level
		bool printWarning = TRUE;

		// Set the appropriate bit to test/set for this nestLevel
		int levelBit;
		
		// Use a bit for each level below 31.  Treat all levels
		// above 31 the same (don't expect that deep but who knows)
		if (nestLevel < 31)
		    levelBit= (1 << nestLevel);
		else
		    levelBit=(1 << 31);

		// Have we seen this name before?
		if (um->unknownXMLTable.entryExists(elementName))
		{
		    // Yes, get level mask for this entry
		    int *levelMask =um->unknownXMLTable.findEntry(elementName);

		    // Is the bit already set?
		    if ((*levelMask) & levelBit)
		    {
			// Yes, suppress warning
			printWarning=FALSE;
		    }
		    else
		    {
			// No, set bit and print warning
			*levelMask |= levelBit;
		    }
		}
		else
		{
		    // No, add to unknownXMLTable
		    int *levelMask = new int (levelBit);
		    um->unknownXMLTable.addEntry (elementName, levelMask);
		}

		// Print warning if haven't printed warning before for this
		// particular elementName and nestLevel
		if (printWarning)
		{
		    fprintf (stderr, 
			     "Warning: Tool Gear ignoring unknown XML elements"
			     " '%s' at level %i\n"
			     "  '%s' first encountered on line %i:\n"
			     "  ",
			     (const char *)elementName, nestLevel,
			     (const char *)elementName,
			     lineNoGuess+lineOffset);

		    // Print out line that caused error
		    printErrorContext (lineNoGuess, -1);
		    fprintf (stderr, "\n");
		}
	    }	    

	    
	    // For now, do minimal processing above level 10
	    // This prevents overflowing our parsing arrays 
	    // A true XML parser should just ignore unexpected data but
	    // I want warnings for now (which is done above).
	    if (nestLevel >= 10)
	    {
		// No error, continue parsing
		return (TRUE);
	    }

	    // Save element token at this level
	    elementTokenAt[nestLevel] = elementToken;

	    // Save element name in case we need it
	    elementNameAt[nestLevel] = elementName;

	    // Clear current value saved at this level
	    valueAt[nestLevel] = "";

#if 0
	    // DEBUG
	    fprintf (stderr, "Level %i for start '%s'\n", nestLevel,
		     (const char *) elementName);
#endif

	    // No error, continue parsing
	    return (TRUE);
	}

    bool endElement( const QString&, const QString&, 
		     const QString& elementName)
	{
	    // Ignore "tool_gear" and "tool_gear_XML_snippet" at level -1
	    // (after any real token) in order to simplify parsing logic 
	    // (i.e., message_folder always happens at level 0, 
	    // indepent of the presence of "<tool_gear>" or 
	    // "<tool_gear_XML_snippet>".
	    if ((nestLevel == -1) &&
		((elementName == "tool_gear") ||
		 (elementName == "tool_gear_XML_snippet")))
	    {
		// No error, continue parsing
		return (TRUE);
	    }

	    // DEBUG
//	    fprintf (stderr, "Level %i %s: '%s'\n", nestLevel,
//		     (const char *) elementName, (const char *)valueAt[nestLevel]);

	    // For sanity check, record if endElement actually processed
	    bool elementHandled = FALSE;

	    // Process XML at level 0 (TG commands only are at level 0)
	    if (nestLevel == 0)
	    {
		// Process addMessage 
	        if (elementTokenAt[0] == XML_message)
		{
		    // Sanity check, empty folders are not allowed
		    for (int i=0; i < message_folder_count; i++)
		    {
			if (message_folder[i].isEmpty())
			{
			    TG_error ("Error parsing xml: "
				      "No folder specified for message!");
			}
		    }

		    // Sanity check, empty headings are not allowed
		    if (message_heading.isEmpty())
		    {
			TG_error ("Error parsing xml: "
				  "No heading specified for message!");
		    }

		    // Ok to have empty body or traceback

		    // Create messageText from heading and body
		    QString messageText;
		    messageText.sprintf ("%s\n%s", 
					 (const char *) message_heading,
					 (const char *) message_body);

		    // add message to each message folder specified
		    for (int i = 0; i < message_folder_count; i++)
		    {
			um->addMessage((const char *)message_folder[i], 
				       (const char *)messageText,
				       (const char *)message_traceback);
		    }

		    elementHandled = TRUE; // Mark element handled
		}

		// Handle XML format specifiers
		else if (elementTokenAt[0] == XML_format)
		{
		    // Convert format value to int
		    int user_xml_format = xmlConvertToInt(NULL_INT, NULL_INT);

		    // Only process if valid value
		    if (user_xml_format != NULL_INT)
		    {
			// Check for tested formats
			if (user_xml_format != 1)
			{
			    fprintf (stderr, 
				     "\n"
				     "Warning: Tool Gear Version %4.2f"
				     " untested with XML format '%i'"
				     " (expected '1')\n"
				     "         May not parse XML properly!\n",
				     um->getToolGearVersion(),
				     user_xml_format);
			}
			
			// Save the specified format to do format particular
			// processing (if any)
			xml_format = user_xml_format;
		    }

		    elementHandled = TRUE; // Mark element handled
		}

		// Handle XML minimum version required
		else if (elementTokenAt[0] == XML_version)
		{
		    // Convert verison value to double
		    double user_xml_version = 
			xmlConvertToDouble(NULL_DOUBLE, NULL_DOUBLE);

		    // Only process if valid value
		    if (user_xml_version != NULL_DOUBLE)
		    {
			// Check for tested required min versions
			if ((user_xml_version < 1.399) || 
			    (user_xml_version > 
			     (um->getToolGearVersion()+.001)))
			{
			    fprintf (stderr, 
				     "\n"
				     "Warning: Tool Gear Version %4.2f"
				     " earlier than XML version"
				     " requirement of %4.2f\n"
				     "         May not parse XML properly!\n",
				     um->getToolGearVersion(),
				     user_xml_version);
			}

			// Save version request for version-specific processing
			// (if any)
			xml_version = user_xml_version;
		    }
		    
		    elementHandled = TRUE; // Mark element handled
		}



		// Process declare message folder (declareMessageFolder)
		else if (elementTokenAt[0] == XML_message_folder)
		{
		    // Sanity check, empty tags are not allowed
		    if (message_folder_tag.isEmpty())
		    {
			TG_error ("Error parsing xml: "
				  "No tag specified for message_folder!");
		    }

		    // Sanity check, empty titles are not allowed
		    if (message_folder_title.isEmpty())
		    {
			TG_error ("Error parsing xml: "
				  "No title specified for message_folder!");
		    }

		    // If if_empty not specified, default to show
		    if (message_folder_if_empty == UIManager::InvalidIfEmpty)
			message_folder_if_empty = UIManager::ShowIfEmpty;

		    // Declare message folder, get index to see if duplicate
		    int index = 
			um->declareMessageFolder(message_folder_tag, 
						 message_folder_title,
						 message_folder_if_empty);

		    // If tag already declared, make sure title is the same
		    // or else print warning.
		    if (index == -1)
		    {
			// Get existing title
			QString oldTitle = 
			    um->messageFolderTitle (message_folder_tag);

			if (message_folder_title != oldTitle)
			{
			    fprintf (stderr,
				     "Warning: Tool Gear ignored invalid XML"
				     " ending on line %i:\n"
				     "  Redeclaration of message_folder tag "
				     "'%s' has different title:\n"
				     "   Orig: '%s'\n"
				     "    New: '%s'\n"
				     "  Existing message folder declarations "
				     "cannot currently be changed!\n\n",
				     lineNoGuess+lineOffset,
				     (const char *)message_folder_tag,
				     (const char *)oldTitle,
				     (const char *)message_folder_title);
			}

		    }

		    elementHandled = TRUE; // Mark element handled
		}

		// Process add site priority commands
		else if (elementTokenAt[0] == XML_site_priority)
		{
		    // Get regular expresses as char * if set, NULL otherwise
		    const char *fileRegExp = NULL;
		    const char *descRegExp = NULL;
		    const char *lineRegExp = NULL;

		    if (site_priority_file != NULL_QSTRING)
			fileRegExp = site_priority_file.latin1();

		    if (site_priority_desc != NULL_QSTRING)
			descRegExp = site_priority_desc.latin1();

		    if (site_priority_line != NULL_QSTRING)
			lineRegExp = site_priority_line.latin1();


		    // Sanity check, empty modifier are not allowed
		    if (site_priority_modifier == NULL_DOUBLE)
		    {
			fprintf (stderr,
				 "Warning: Tool Gear ignored invalid XML"
				 " ending on line %i:\n"
				 "  No priority modifier specified!\n\n",
				 lineNoGuess+lineOffset);
		    }
		
		    // Sanity check, t least one of file, line, or desc
		    // must be specified
		    else if ((fileRegExp == NULL) && (descRegExp == NULL) &&
			     (lineRegExp == NULL))
		    {

			fprintf (stderr,
				 "Warning: Tool Gear ignored invalid XML"
				 " ending on line %i:\n"
				 "At least one RegExp must be specified for "
				 "file, desc, or line for site_priority!\n\n",
				 lineNoGuess+lineOffset);
		    }

		    // If got here, believe valid XML
		    else
		    {
			// Add site priority modifier
			um->addSitePriority (site_priority_modifier, 
					     fileRegExp, descRegExp, 
					     lineRegExp);
		    }

		    elementHandled = TRUE; // Mark element handled
		}

		// Process addAbout command 
		else if (elementTokenAt[0] == XML_about)
		{
		    // Get any text not in <prepend> or <append>
		    // removing any whitespace
		    QString tempAbout = valueAt[0].stripWhiteSpace();

		    // If not empty, print out warning
		    if (!tempAbout.isEmpty())
		    {
			fprintf (stderr, 
				 "Warning: Tool Gear ignoring XML about text "
				 "not between <append> or <prepend>!\n"
				 "  Ignoring '%s'\n",
				 tempAbout.latin1());
		    }

		    // uses value at about level
//		    um->addAboutText ((const char *)valueAt[0]);

		    elementHandled = TRUE; // Mark element handled
		}

		// Process set tool window title command 
		else if (elementTokenAt[0] == XML_tool_title)
		{
		    // Do we need to strip whitespace?
//		    QString toolTitle = valueAt[0].stripWhiteSpace();

		    // uses value at tool_title level
		    um->setWindowCaption ((const char *)valueAt[0]);

		    elementHandled = TRUE; // Mark element handled
		}

		// Set status message for tool
		else if (elementTokenAt[0] == XML_status)
		{
		    // For now, signal new status set
		    // May want to actually save status somewhere
		    emit um->toolStatusSet (valueAt[0].latin1());

		    elementHandled = TRUE; // Mark element handled
		}


	    }
	    // Process XML at level 1
	    else if (nestLevel == 1)
	    {
		// Handle message values
		if (elementTokenAt[0] == XML_message)
		{
		    if (elementTokenAt[1] == XML_folder)
		    {
			// Sanity check, currently allow a max of
			// MAX_MESSAGE_FOLDERS folders to be specfied 
			// for one message
			if (message_folder_count >= MAX_MESSAGE_FOLDERS)
			{
			    fprintf (stderr, 
				     "Warning: Ignoring folder specifier.  "
				     "Only %i are allowed per message:\n", 
				     MAX_MESSAGE_FOLDERS);

			    // Print out line that caused error
			    printErrorContext (lineNoGuess, -1);
			    fprintf (stderr, "\n");

			}
			// Otherwise, add folder to message queue
			else
			{
			    message_folder[message_folder_count] = valueAt[1];
			    message_folder_count++;
			}
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_heading)
		    {
			setIfEmpty(message_heading);
			elementHandled = TRUE; // Mark element handled
		    }

		    else if (elementTokenAt[1] == XML_body)
		    {
			setIfEmpty(message_body);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_annot)
		    {
			// Construct traceback snippet
			QString traceback;
			
			if (!annot_title.isEmpty())
			{
			    traceback.sprintf ("^%s\n%s", 
					       (const char *)annot_title,
					       (const char *)annot_traceback);
			}
			else
			{
			    traceback = annot_traceback;
			}

			// If message traceback not empty, append newline
			// before appending traceback snippet
			if (!message_traceback.isEmpty())
			    message_traceback.append("\n");

			// Append traceback snippet to message traceback
			message_traceback.append(traceback);

			elementHandled = TRUE; // Mark element handled
		    }
		}

		// Handle about values
		else if (elementTokenAt[0] == XML_about)
		{
		    if (elementTokenAt[1] == XML_prepend)
		    {
			um->addAboutText ((const char *)valueAt[1], TRUE);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_append)
		    {
			um->addAboutText ((const char *)valueAt[1], FALSE);
			elementHandled = TRUE; // Mark element handled
		    }
		}

		// Handle site_priority values
		else if (elementTokenAt[0] == XML_site_priority)
		{
		    if (elementTokenAt[1] == XML_file)
		    {
			setIfEmpty(site_priority_file);
			elementHandled = TRUE; // Mark element handled
		    }

		    else if (elementTokenAt[1] == XML_desc)
		    {
			setIfEmpty(site_priority_desc);
			elementHandled = TRUE; // Mark element handled
		    }

		    else if (elementTokenAt[1] == XML_line)
		    {
			setIfEmpty(site_priority_line);
			elementHandled = TRUE; // Mark element handled
		    }

		    else if (elementTokenAt[1] == XML_modifier)
		    {
			site_priority_modifier = 
			    xmlConvertToDouble(NULL_DOUBLE, NULL_DOUBLE);
			elementHandled = TRUE; // Mark element handled
		    }
		}

		// Handle site_data values
		else if (elementTokenAt[0] == XML_site_data)
		{
		    if (elementTokenAt[1] == XML_col)
		    {
			setIfEmpty(site_data_col);
			elementHandled = TRUE; // Mark element handled
		    }

		    else if (elementTokenAt[1] == XML_file)
		    {
			setIfEmpty(site_data_file);
			elementHandled = TRUE; // Mark element handled
		    }

		}

		// Handle message folder values
		else if (elementTokenAt[0] == XML_message_folder)
		{
		    if (elementTokenAt[1] == XML_tag)
		    {
			setIfEmpty(message_folder_tag);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_title)
		    {
			setIfEmpty(message_folder_title);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_if_empty)
		    {
			// Get all lowercase version of input with whitespace
			// removed from the end
			QString if_empty = 
			    valueAt[1].stripWhiteSpace().lower();
			// Look for currently accepted keywords
			if (if_empty == "show")
			{
			    message_folder_if_empty = UIManager::ShowIfEmpty;
			}
			else if (if_empty == "hide")
			{
			    message_folder_if_empty = UIManager::HideIfEmpty;
			}
			else if (if_empty == "disable")
			{
			    message_folder_if_empty = 
				UIManager::DisableIfEmpty;
			}
			else
			{
			    fprintf (stderr, 
				     "Warning: Tool Gear ignoring invalid XML "
				     "if_empty value '%s' for\n"
				     "  '%s' on line %i:\n"
				     "  ",
				     (const char *)if_empty, 
				     (const char *)elementNameAt[nestLevel],
				     lineNoGuess+lineOffset);
		
			    // Print out line that caused error
			    printErrorContext (lineNoGuess, -1);
			    fprintf (stderr, 
				     "  Valid values are 'show', 'hide', "
				     "or 'disable'.\n\n");
			}
			
			elementHandled = TRUE; // Mark element handled
		    }
		}

		// Handle site column info values
		else if (elementTokenAt[0] == XML_site_column)
		{
		    if (elementTokenAt[1] == XML_tag)
		    {
			setIfEmpty(site_column_tag);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_title)
		    {
			setIfEmpty(site_column_title);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_site_mask)
		    {
			// Allow any non-negative signed 32-bit ints 
			// (Although mask of 0 will prevent the column from
			//  showing, which perhaps is desired).
			site_column_site_mask = 
			    xmlConvertToInt(0, 2147483647);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_hide_if_empty)
		    {
			// Only allow 0 or 1 for value
			site_column_hide_if_empty = xmlConvertToInt(0, 1);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_position)
		    {
			// Allow any int value
			site_column_position = xmlConvertToInt(NULL_INT, 
								    NULL_INT);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_tooltip)
		    {
			setIfEmpty(site_column_tooltip);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_align)
		    {
			// Get all lowercase version of input with whitespace
			// removed from the end
			QString align = valueAt[1].stripWhiteSpace().lower();

			// Look for currently accepted keywords
			if (align == "auto")
			{
			}
			else if (align == "right")
			{
			}
			else if (align == "left")
			{
			}
			else if (align == "center")
			{
			}
			else
			{
			    fprintf (stderr, 
				     "Warning: Tool Gear ignoring invalid XML "
				     "align value '%s' for\n"
				     "  '%s' on line %i:\n"
				     "  ",
				     (const char *)align, 
				     (const char *)elementNameAt[nestLevel],
				     lineNoGuess+lineOffset);
		
			    // Print out line that caused error
			    printErrorContext (lineNoGuess, -1);
			    fprintf (stderr, 
				     "  Valid values are 'auto', 'right', 'left', or 'center'.\n\n");
			}
			
			site_column_align =   align;
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_min_width)
		    {
			// Allow any positive int value
			site_column_min_width = xmlConvertToInt(0, 
								     NULL_INT);
			elementHandled = TRUE; // Mark element handled
		    }
		    else if (elementTokenAt[1] == XML_max_width)
		    {
			// Allow any non-zero positive int value
			site_column_max_width = xmlConvertToInt(1, 
								     NULL_INT);
			elementHandled = TRUE; // Mark element handled
		    }
		}
	    }

	    // Process XML at level 2
	    else if (nestLevel == 2)
	    {
		// Handle message values
		if (elementTokenAt[0] == XML_message)
		{
		    // Handle message annotation values
		    if (elementTokenAt[1] == XML_annot)
		    {
			if (elementTokenAt[2] == XML_title)
			{
			    // Record annot title
			    setIfEmpty(annot_title);

			    elementHandled = TRUE; // Mark element handled
			}
			else if (elementTokenAt[2] == XML_site)
			{
			    // Construct traceback line
			    QString traceback_line;

			    // Do we have file or line specified?
			    if ((!site_file.isEmpty()) ||
				(!site_line.isEmpty()))
			    {
				// Yes, do full format
				traceback_line.sprintf ("<%s:%s>%s",
						(const char *) site_file,
						(const char *) site_line,
						(const char *) site_desc);
			    }

			    // Otherwise, do only description (if exists)
			    else if (!site_desc.isEmpty())
			    {
				traceback_line.sprintf("%s",
					       (const char *)site_desc);
			    }

			    // Otherwise, warn about empty site specifier
			    else
			    {
				fprintf (stderr, 
					 "Warning: Empty site specifier for "
					 "message annotation!\n");

				// Put empty traceback line for now
				traceback_line = "";
				
			    }

			    // If existing annot_traceback not empty,
			    // first append newline
			    if (!annot_traceback.isEmpty())
				annot_traceback.append("\n");
			    
			    // Append traceback_line to annot_traceback
			    annot_traceback.append(traceback_line);

			    elementHandled = TRUE; // Mark element handled
			}
		    }

		}
	    }

	    // Process XML at level 3
	    else if (nestLevel == 3)
	    {
		// Handle message values
		if (elementTokenAt[0] == XML_message)
		{
		    // Handle message annot(ation) values
		    if (elementTokenAt[1] == XML_annot)
		    {
			// Handle message annot site values
			if (elementTokenAt[2] == XML_site)
			{
			    if (elementTokenAt[3] == XML_file)
			    {
				// Record site file
				setIfEmpty(site_file);
				
				elementHandled = TRUE; // Mark element handled
			    }
			    else if (elementTokenAt[3] == XML_line)
			    {
				// Record site line
				setIfEmpty(site_line);
				
				elementHandled = TRUE; // Mark element handled
			    }
			    else if (elementTokenAt[3] == XML_desc)
			    {
				// Record description line
				setIfEmpty(site_desc);
				
				elementHandled = TRUE; // Mark element handled
			    }
			}
		    }
		}
	    }

	    // Print warning once for each unhandled known element
	    // Use same suppression table for unknown XML tokens since
	    // they should not intersect (since things not in the table
	    // will be all XML_unknown).
	    // To simplify, only print warnings for first 10 levels of XML,
	    // since don't expect to have info deeper than level 3 for a while 
	    if ((!elementHandled) && 
		(nestLevel >= 0) && (nestLevel < 10) &&
		(elementTokenAt[nestLevel] != XML_unknown))
	    {
		// Get elementName for ease of use
		const char *elementName = elementNameAt[nestLevel];

		// Want to warn only once per unknown element name per level
		bool printWarning = TRUE;

		// Set the appropriate bit to test/set for this nestLevel
		int levelBit;
		
		// Use a bit for each level below 31.  Treat all levels
		// above 31 the same (don't expect that deep but who knows)
		if (nestLevel < 31)
		    levelBit= (1 << nestLevel);
		else
		    levelBit=(1 << 31);

		// Have we seen this name before?
		if (um->unknownXMLTable.entryExists(elementName))
		{
		    // Yes, get level mask for this entry
		    int *levelMask =um->unknownXMLTable.findEntry(elementName);

		    // Is the bit already set?
		    if ((*levelMask) & levelBit)
		    {
			// Yes, suppress warning
			printWarning=FALSE;
		    }
		    else
		    {
			// No, set bit and print warning
			*levelMask |= levelBit;
		    }
		}
		else
		{
		    // No, add to unknownXMLTable
		    int *levelMask = new int (levelBit);
		    um->unknownXMLTable.addEntry (elementName, levelMask);
		}

		// Print warning if haven't printed warning before for this
		// particular elementName and nestLevel
		if (printWarning)
		{
		    fprintf (stderr, 
			     "Warning: Tool Gear not handling XML elements"
			     " '%s' at level %i\n"
			     "  '%s' first encountered on line %i:\n"
			     "  ",
			     (const char *)elementName, nestLevel,
			     (const char *)elementName,
			     lineNoGuess+lineOffset);

		    // Print out line that caused error
		    printErrorContext (lineNoGuess, -1);
		    fprintf (stderr, "\n");
		}
		
	    }

	    // Finished with element, decrease nestLevel
	    --nestLevel;

	    // No error, continue parsing
	    return (TRUE);
	}
    
    bool characters ( const QString & ch )
	{
	    // Append ch to the current value at this level
	    valueAt[nestLevel].append(ch);

	    // Increment line number guess based on newlines in ch
	    const char *ptr = (const char *)ch;
	    while (*ptr != 0)
	    {
		if (*ptr == '\n')
		    lineNoGuess++;

		++ptr;
	    }
	    
	    // No error, continue parsing
	    return (TRUE);
	}

    bool fatalError ( const QXmlParseException & exception )
	{
	    error_message = "Fatal XML parse error";

	    // Print out location and description of error
	    fprintf (stderr, 
		     "\nXML parse error at line %i column %i: %s\n",
		     exception.lineNumber() + lineOffset,
		     exception.columnNumber(),
		     (const char *)exception.message());

	    // Print out line that caused error
	    printErrorContext (exception.lineNumber(),
			       exception.columnNumber());

	    // Print out XML stack when error occurred
	    QString indent="";
	    fprintf (stderr, "XML stack at time of parse error:\n");
	    for (int level=0; (level < nestLevel); ++level)
	    {
		// Currently only store the first 10 levels of XML stack
		if (level < 10)
		{
		    fprintf (stderr, "%s<%s>\n", 
			     (const char *)indent,
			     (const char *)elementNameAt[level]);
		}
		else
		{
		    fprintf (stderr, "%s<(Element name not stored for level %i)>\n", 
			     (const char *) indent, 
			     level);
		}
		indent.append("  ");
	    }
	    fprintf (stderr, 
		     "\nWarning: Discarding rest of XML snippet due to "
		     "parse error.\n\n");


	    // Stop parsing this snippet of XML
	    return FALSE;
	}
    bool error ( const QXmlParseException & exception )
	{
	    // Handle same way as fatal error for now
	    fatalError (exception);

	    // Stop parsing this snippet of XML
	    return FALSE;
	}
    bool warning ( const QXmlParseException & exception )
	{
	    fprintf (stderr, 
		     "\nXML Parse warning: Line %i, column %i: %s\n",
		     exception.lineNumber()+lineOffset, 
		     exception.columnNumber(),
		     (const char *)exception.message());

	    // Print out line that caused warning
	    printErrorContext (exception.lineNumber(),
			       exception.columnNumber());

	    fprintf (stderr, "\n");

	    // Continue parsing XML
	    return TRUE;
 	}

    QString errorString() {return (error_message);};

    void printErrorContext (int errorLine, int errorColumn)
	{
	    // Get the XML we are parsing as characters
	    const char *xml = (const char *)xml_text;
	    const char *startPtr, *printPtr;
	    int lineNo = 1;

	    // print out line number
	    fprintf (stderr, "%i: ", errorLine);

	    // Scan through to find the begining of the problem line
	    for (startPtr = xml; (*startPtr != 0); ++startPtr)
	    {
		// Stop when hit beginning of problem line
		if (lineNo >= errorLine)
		    break;

		// Increment lineNo after each newline
		if (*startPtr == '\n')
		    ++lineNo;
	    }
	    
	    // Print out the next line
	    for (printPtr = startPtr; (*printPtr != 0) && (*printPtr != '\n');
		 ++printPtr)
	    {
		fputc (*printPtr, stderr);
	    }
	    fputc ('\n', stderr);

	    if (errorColumn >= 0)
	    {
		// Print out arrow to column where error occurred
		for (int col = 1; col < errorColumn; ++col)
		    fputc (' ', stderr);
		
		fprintf (stderr, "^\n");
	    }
	}

private:
    // Sets QString if it is empty, otherwise prints warning that
    // the duplicate is being ignored.
    // Returns TRUE if var is empty, otherwise returns FALSE.
    bool setIfEmpty(QString &var)
	{
	    // If empty, everything ok, can fill in new definition
	    if (var.isEmpty())
	    {
		var = valueAt[nestLevel];
		return (TRUE);
	    }

	    // Otherwise, print warning
	    fprintf (stderr, "Warning: Ignoring duplicate '%s' specifier:\n",
		     elementNameAt[nestLevel].latin1());
	    printErrorContext (lineNoGuess, -1);
	    fprintf (stderr, "\n");

	    // Return FALSE, this is a duplicate
	    return (FALSE);
	}

    // Returns int value at nestLevel (implicit) or NULL_INT if it is
    // not a valid int or not between minInt and maxInt (which may
    // both be NULL_INT (and default to that)
    int xmlConvertToInt(int minInt = NULL_INT, int maxInt = NULL_INT)
	{
	    if (nestLevel >= 10)
	    {
		fprintf (stderr, "Error UIXMLParser::xmlConvertToInt: "
			 "nestLevel(%i) >= 10 unsupported!\n", nestLevel);

		// Return Invalid value
		return (NULL_INT);
	    }

	    // Get string to convert
	    const char *startPtr = (const char *)valueAt[nestLevel];

	    // Strip off leading whitespace
	    char ch;
	    while ((ch = *startPtr) != 0)
	    {
		if (isspace (ch))
		    startPtr++;
		else
		    break;
	    }

	    // Warn and ignore empty values
	    if (ch == 0)
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring empty XML int value for"
			 " '%s' on line %i:\n"
			 "  ",
			 (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_INT);
	    }
	    
	    // Convert to int
	    char *endPtr;
	    int val = strtol(startPtr, &endPtr, 0);

	    // Strip off trailing whitespace
	    while ((ch = *endPtr) != 0)
	    {
		if (isspace (ch))
		    endPtr++;
		else
		    break;
	    }

	    // Make sure converted everything that is not whitespace in string
	    if (*endPtr != 0)
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring invalid XML int value '%s' for\n"
			 "  '%s' on line %i:\n"
			 "  ",
			 startPtr, (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_INT);
	    }

	    // If bounded, test bounds
	    if ((minInt != NULL_INT) && (val < minInt))
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring too small XML int value %i (< %i) for\n"
			 "  '%s' on line %i:\n"
			 "  ",
			  val, minInt,
			 (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_INT);
		
	    }
	    if ((maxInt != NULL_INT) && (val > maxInt))
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring too large XML int value %i (> %i) for\n"
			 "  '%s' on line %i:\n"
			 "  ",
			  val, maxInt,
			 (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_INT);
	    }

	    // Value must be ok if got here, return it
	    return (val);
	}

    // Returns double value at nestLevel (implicit) or NULL_DOUBLE if it is
    // not a valid double or not between minDouble and maxDouble (which may
    // both be NULL_DOUBLE (and default to that)
    double xmlConvertToDouble(double minDouble = NULL_DOUBLE, 
			      double maxDouble = NULL_DOUBLE)
	{
	    if (nestLevel >= 10)
	    {
		fprintf (stderr, "Error UIXMLParser::xmlConvertToDouble: "
			 "nestLevel(%i) >= 10 unsupported!\n", nestLevel);

		// Return Invalid value
		return (NULL_DOUBLE);
	    }

	    // Get string to convert
	    const char *startPtr = (const char *)valueAt[nestLevel];

	    // Strip off leading whitespace
	    char ch;
	    while ((ch = *startPtr) != 0)
	    {
		if (isspace (ch))
		    startPtr++;
		else
		    break;
	    }

	    // Warn and ignore empty values
	    if (ch == 0)
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring empty XML double value for"
			 " '%s' on line %i:\n"
			 "  ",
			 (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_DOUBLE);
	    }
	    
	    // Convert to double
	    char *endPtr;
	    double val = strtod(startPtr, &endPtr);

	    // Strip off trailing whitespace
	    while ((ch = *endPtr) != 0)
	    {
		if (isspace (ch))
		    endPtr++;
		else
		    break;
	    }

	    // Make sure converted everything that is not whitespace in string
	    if (*endPtr != 0)
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring invalid XML double value '%s' for\n"
			 "  '%s' on line %i:\n"
			 "  ",
			 startPtr, (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_DOUBLE);
	    }

	    // If bounded, test bounds
	    if ((minDouble != NULL_DOUBLE) && (val < minDouble))
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring too small XML double value %g (< %g) for\n"
			 "  '%s' on line %i:\n"
			 "  ",
			  val, minDouble,
			 (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_DOUBLE);
		
	    }
	    if ((maxDouble != NULL_DOUBLE) && (val > maxDouble))
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring too large XML double value %g (> %g) for\n"
			 "  '%s' on line %i:\n"
			 "  ",
			  val, maxDouble,
			 (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_DOUBLE);
	    }

	    // Value must be ok if got here, return it
	    return (val);
	}

    XMLElementToken getToken (const char *name, int nestLevel)
	{

	    // Set the appropriate bit to test/set for this nestLevel
	    int levelBit;
		
	    // Use a bit for each level below 31.  Treat all levels
	    // above 31 the same (don't expect that deep but who knows)
	    if (nestLevel < 31)
		levelBit= (1 << nestLevel);
	    else
		levelBit=(1 << 31);

	    // Default to unknown token;
	    XMLElementToken token = XML_unknown;

	    // Is token in table?
	    UIManager::XML_Token *entry = um->xmlTokenTable.findEntry(name);
	    
	    // Yes, token in table
	    if (entry != NULL)
	    {
		// Set token if levelBit is in levelMask
		if ((entry->levelMask & levelBit) != 0)
		    token = (XMLElementToken)entry->token;
	    }

//	    fprintf (stderr, "getToken(%s, %i) returns %i\n", name, nestLevel,
//		     (int)token);

	    // Return token found
	    return (token);
	}

    // Utility funciton for constructor
    void declareToken_(const char *name, XMLElementToken token, int nestLevel)
	{
//	    fprintf (stderr, "declareToken_(%s, %i, %i)\n",
//		     name, token, nestLevel);

	    // Set the appropriate bit to test/set for this nestLevel
	    int levelBit;
		
	    // Use a bit for each level below 31.  Treat all levels
	    // above 31 the same (don't expect that deep but who knows)
	    if (nestLevel < 31)
		levelBit= (1 << nestLevel);
	    else
		levelBit=(1 << 31);

	    // Is it already in table?
	    UIManager::XML_Token *entry = um->xmlTokenTable.findEntry(name);
	    
	    // Yes, add this levelBit to it
	    if (entry != NULL)
	    {
		entry->levelMask |= levelBit;
	    }

	    // No, create entry and add to table
	    else
	    {
		entry = new UIManager::XML_Token((int) token, levelBit);
		um->xmlTokenTable.addEntry(name, entry);
	    }
	}

    UIManager *um;
    int nestLevel;
    XMLElementToken elementTokenAt[10];
    QString elementNameAt[10];
    QString valueAt[10];


    // XML version information
    int xml_format;
    double xml_version;

    // Strings for declare message_folder parameters
    QString message_folder_tag;
    QString message_folder_title;
    UIManager::PolicyIfEmpty message_folder_if_empty;

    // Strings for add message parameters
    QString message_folder[MAX_MESSAGE_FOLDERS];    // Allow muliple folders
    int message_folder_count; // Number of message folders
    QString message_heading;
    QString message_body;
    QString message_traceback;

    // Strings for message annot(ation) parameters
    QString annot_title;
    QString annot_traceback;

    // Strings for site priority parameters
    QString site_priority_file;
    QString site_priority_desc;
    QString site_priority_line;
    double site_priority_modifier;


    // Strings for message annot(ation) site parameters
    QString site_file;
    QString site_line;
    QString site_desc;

    // Values for site_column parameters
    QString site_column_tag;
    QString site_column_title;
    int site_column_site_mask;
    int site_column_hide_if_empty;
    int site_column_position;
    QString site_column_tooltip;
    QString site_column_align;
    int site_column_min_width;
    int site_column_max_width;

    // Strings for site_data paramters
    QString site_data_col;
    QString site_data_file;

    
    // QXmlDefaultHandler expects errorString() to return something,
    // we are returning error_message;
    QString error_message;

    // For printing out parse error messages, the XML text being parsed
    QString xml_text;

    // For printing out parse error message, the lineNumberOffset
    int lineOffset;

    // For printing out warning messages, guess at lineNo
    int lineNoGuess;
};

// Processes the XMLSnippet (first wrapping it with <tool_gear_snippet>
// ... </tool_gear_snippet> to make the XML parser happy with multiple
// snippets) and process the recognized commands (warning about those
// not recognized.   Initially XML can be used to execute
// declareMessageFolder(), addMessage(), and addAboutText() commands.
// lineNoOffset is used in error messages to relate line in snippet to
// line in source file (defaults to 0).
void UIManager::processXMLSnippet (const char *XMLSnippet,
				   int lineNoOffset)
{
    QString wrappedXMLSnippet;

    wrappedXMLSnippet.sprintf ("<tool_gear_XML_snippet>\n%s\n</tool_gear_XML_snippet>",
			       XMLSnippet);
#if 0
    // DEBUG
    fprintf (stderr, 
	     "Processing XML Snippet:\n"
	     "%s\n"
	     "-----------------------\n", (const char *) wrappedXMLSnippet);
#endif

    // Subtract 1 from offset since adding one line
    UIXMLParser handler (this, wrappedXMLSnippet, lineNoOffset-1);
    QXmlInputSource source;
    source.setData(wrappedXMLSnippet);
    QXmlSimpleReader reader;
    reader.setContentHandler (&handler);
    reader.setErrorHandler (&handler);
    reader.parse(source);

}

// Include the QT specific code that is automatically
// generated from the uimanager.h
//#include "uimanager.moc"
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

