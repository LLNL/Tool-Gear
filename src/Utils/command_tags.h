//! \file command_tags.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

// John May, 12 October 2000

#ifndef COMMAND_TAGS_H
#define COMMAND_TAGS_H
//! Lists actions that can be requested between Client and Collector.
//! The command_tags enum lists all the requests and messages
//! that can be sent between the Client and the Collector.  The
//! prefix letters (DB, DPCL, and GUI) are somewhat descriptive
//! of either the source or destination of the message,
//! but they haven't been used consistently.
typedef enum {
	DB_INSERT_ENTRY,		//!< Enter a target program location
					//!< in the database
	DB_INSERT_PLAIN_ENTRY,		//!< Used for entries that don't
					//!< correspond to a DPCL action point
	DB_INSERT_FUNCTION,		//!< Enter a target program function
					//!< in the database
	DB_DECLARE_ACTION_ATTR,		//!< Declare actions that user can
					//!< take in target program (such as
					//!< setting instrumentation points)
	DB_ENABLE_ACTION,		//!< Tell GUI to make an action
					//!< available to the user
	DB_DECLARE_DATA_ATTR,		//!< Declare a data column
					//!<
	DB_INSERT_PROCESS_THREAD,	//!< Declare process/thread pair
					//!<
	DB_INSERT_DOUBLE,		//!< Insert double precision data
					//!< value in database
	DB_INSERT_INT,			//!< Insert integer value into database
					//!<
	DB_ADD_DOUBLE,			//!< Add value to an exisiting double
					//!< in the database
	DB_ADD_INT,			//!< Add value to an exisiting integer
					//!< in the database
	DB_INSERT_MODULE,		//!< Declare a module (usually a file)
					//!< from the target program
	DB_PROCESS_PARSE_COMPLETE,	//!< All modules from the target
					//!< process have been declared
	DB_FILE_PARSE_COMPLETE,		//!< All functions from the current
					//!< file (module) have been declared
	DB_FUNCTION_PARSE_COMPLETE,	//!< All instrumentation points from
					//!< the current function have been
					//!< declared.
	DB_FILE_FULL_PATH,		//!< Reports the full path location
					//!< where source file was found
	DB_FILE_READ_COMPLETE,		//!< Transmission of the requested
					//!< source file is complete
	DB_STATIC_DATA_COMPLETE,	//!< All data from post-mortem file
					//!< has been sent
	DB_DECLARE_MESSAGE_FOLDER,      //!< Declare a new message folder
	                                //!< 
	DB_ADD_MESSAGE,                 //!< Add a new message to messageFolder
	                                //!<
	DB_PROCESS_XML_SNIPPET,         //!< Process XML snippet's command(s)
	                                //!<
	DPCL_PARSE_COMPLETE,		//!< OBSOLETE
					//!<
	DPCL_START_PROGRAM,		//!< Start the target program 
					//!<
	DPCL_STOP_PROGRAM,		//!< Stop the target program
					//!<
	DPCL_ACTIVATE_ACTION,		//!< Activate a declared action at a
					//!< particular action point for a
					//!< specified process
	DPCL_DEACTIVATE_ACTION,		//!< Deactivate an action for a 
					//!< specified process
	DPCL_ACTIVATE_ACTION_ALL,	//!< Activate a declared action at a
					//!< particular action point for ll
					//!< processes
	DPCL_DEACTIVATE_ACTION_ALL,	//!< Deactivate an action for all
					//!< processes
	DPCL_INITIALIZE_APP_PARALLEL,	//!< Load target application as a
					//!< parallel job
	DPCL_INITIALIZE_APP_SEQUENTIAL,	//!< Load target application, and
					//!< attempt to determine whether it
					//!< should be parallel or serial
	DPCL_INITIALIZE_APP_RESULT,	//!< Message indicates result of
					//!< application initializaion
	DPCL_CHANGE_DIR,		//!< Set directory where target
					//!< program will run
	DPCL_CHANGE_DIR_RESULT,		//!< Message indicates result of
					//!< directory request
	DPCL_PARSE_MODULE,		//!< Get function information from
					//!< module using DPCL
	DPCL_PARSE_FUNCTION,		//!< Get instumentation points from
					//!< function using DPCL
	DPCL_TARGET_TERMINATED,		//!< Target program has terminated
					//!<
	DPCL_READ_FILE,			//!< Request to Collector to return
					//!< contents of specified source file
	COLLECTOR_CANCEL_READ_FILE,     //!< Tells collector to halt search
	DPCL_SET_HEARTBEAT,		//!< Set timeout timer on Collector
					//!<
	DPCL_TARGET_HALTED,		//!< Target program has reached a
					//!< breakpoint
	GUI_CREATE_VIEWER,		//!< Request by Collector to create
					//!< GUI display
	GUI_CREATE_STATIC_VIEWER,	//!< Request a GUI display that has 
					//!< no program control or
					//!< instrumentation options
	GUI_SET_TARGET_INFO,		//!< OBSOLETE
					//!<
	DPCL_TOOL_INFO,			//!< Place enclosed info about a tool
					//!< in the "About..." box
	DPCL_INSTRUMENT_APP,		//!< Request to find locations in the
					//!< app that match a specfication and
					//!< apply or a remove a given action
	COLLECTOR_GET_SUBDIRS,		//!< Request to read subdirectories
					//!< of specified directory
	GUI_ADD_SUBDIRS,		//!< Response to request for subdirs
	COLLECTOR_REPORT_SEARCH_PATH,	//!< Request collector's current path
	GUI_SEARCH_PATH,		//!< Response to request for search path
	COLLECTOR_SET_SEARCH_PATH,	//!< New search path set by client
	GUI_SAYS_QUIT,			//!< User has exited the application
					//!<
	DPCL_SAYS_QUIT,			//!< Collector cannot continue running
					//!<
	/* dynamic collector error message facility */
	COLLECTOR_ERROR_MESSAGE,        //!< Includes error string to print out

	SOCKET_ERROR,			//!< Indicates communcation error

	/* MS/START - dynamic collector loading / COLL -> CLIENT */

	DYNCOLLECT_NUMMODS,             //!< Send number of dynamic modules
	DYNCOLLECT_NAME,                //!< Send one name of dynamic module and its rand
	DYNCOLLECT_ASK,                 //!< Display list and ask which we should use
	
	/* dynamic collector loading / CLIENT -> COLL */

	DYNCOLLECT_LOADMODULE,          //!< Tell collector to load a module
	DYNCOLLECT_ALLLOADED,           //!< Signal that all modules are loaded


	/* MS/END - dynamic collector loading */

	LAST_COMMAND_TAG		//!< Indicated number of items in
					//!< this enum
} command_tags;

extern char * command_strings[LAST_COMMAND_TAG+1];

typedef enum {
	COLLECTOR_NO_SAVE_PATH,		//!< Apply the path but don't save it
	COLLECTOR_SAVE_PATH_LOCAL,	//!< Apply and save in current dir
	COLLECTOR_SAVE_PATH_HOME	//!< Apply and save in home dir
} save_path_state;


typedef enum {
					// Be sure that no values here
					// overlap with command tags
	FIRST_VIEW_TYPE = LAST_COMMAND_TAG + 1,
	TREE_VIEW = FIRST_VIEW_TYPE,
	MESSAGE_VIEW,
	FIRST_STATIC_VIEW_TYPE,
	STATIC_TREE_VIEW = FIRST_STATIC_VIEW_TYPE,
	STATIC_MESSAGE_VIEW,
	N_VIEW_TYPES
} ViewType;				//!< Types of data viewers

#define IS_A_VIEW_TYPE(t) (t >= FIRST_VIEW_TYPE && t < N_VIEW_TYPES )
#define IS_A_DYNAMIC_VIEW_TYPE(t) \
	( t >= FIRST_VIEW_TYPE && t < FIRST_STATIC_VIEW_TYPE )
#define IS_A_STATIC_VIEW_TYPE(t) \
	( t >= FIRST_STATIC_VIEW_TYPE && t < N_VIEW_TYPES )
#define IS_A_TREE_VIEW(t) (t == TREE_VIEW || t == STATIC_TREE_VIEW )
#define IS_A_MESSAGE_VIEW(t) (t == MESSAGE_VIEW || t == STATIC_MESSAGE_VIEW )
#endif // COMMAND_TAGS_H
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

