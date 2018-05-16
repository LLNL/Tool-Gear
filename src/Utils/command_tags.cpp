// command_tags.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 15 August 2003 

//! List of strings corresponding to Tool Gear command tags.
//! Needed only for debugging.  Must be updated whenever 
//! command_tags.h is changed.

char * command_strings[] = {
	"DB_INSERT_ENTRY",
	"DB_INSERT_PLAIN_ENTRY",
	"DB_INSERT_FUNCTION",
	"DB_DECLARE_ACTION_ATTR",
	"DB_ENABLE_ACTION",
	"DB_DECLARE_DATA_ATTR",
	"DB_INSERT_PROCESS_THREAD",
	"DB_INSERT_DOUBLE",
	"DB_INSERT_INT",
	"DB_ADD_DOUBLE",
	"DB_ADD_INT",
	"DB_INSERT_MODULE",
	"DB_PROCESS_PARSE_COMPLETE",
	"DB_FILE_PARSE_COMPLETE",
	"DB_FUNCTION_PARSE_COMPLETE",
	"DB_FILE_FULL_PATH",
	"DB_FILE_READ_COMPLETE",
	"DB_STATIC_DATA_COMPLETE",
	"DB_DECLARE_MESSAGE_FOLDER",
	"DB_ADD_MESSAGE",
	"DB_PROCESS_XML_SNIPPET",
	"DPCL_PARSE_COMPLETE",
	"DPCL_START_PROGRAM",
	"DPCL_STOP_PROGRAM",
	"DPCL_ACTIVATE_ACTION",
	"DPCL_DEACTIVATE_ACTION",
	"DPCL_ACTIVATE_ACTION_ALL",
	"DPCL_DEACTIVATE_ACTION_ALL",
	"DPCL_INITIALIZE_APP_PARALLEL",
	"DPCL_INITIALIZE_APP_SEQUENTIAL",
	"DPCL_INITIALIZE_APP_RESULT",
	"DPCL_CHANGE_DIR",
	"DPCL_CHANGE_DIR_RESULT",
	"DPCL_PARSE_MODULE",
	"DPCL_PARSE_FUNCTION",
	"DPCL_TARGET_TERMINATED",
	"DPCL_READ_FILE",
	"COLLECTOR_CANCEL_READ_FILE",
	"DPCL_SET_HEARTBEAT",
	"DPCL_TARGET_HALTED",
	"GUI_CREATE_VIEWER",
	"GUI_CREATE_STATIC_VIEWER",
	"GUI_SET_TARGET_INFO",
	"DPCL_TOOL_INFO",
	"DPCL_INSTRUMENT_APP",
	"COLLECTOR_GET_SUBDIRS",
	"GUI_ADD_SUBDIRS",
	"COLLECTOR_REPORT_SEARCH_PATH",
	"GUI_SEARCH_PATH",
	"COLLECTOR_SET_SEARCH_PATH",
	"GUI_SAYS_QUIT",
	"DPCL_SAYS_QUIT",
	"SOCKET_ERROR",

	/* MS/START - dynamic collector loading */
	"DYNCOLLECT_NUMMODS",
	"DYNCOLLECT_NAME",
	"DYNCOLLECT_ASK",
	"DYNCOLLECT_LOADMODULE",
	"DYNCOLLECT_ALLLOADED", 

	/* MS/END - dynamic collector loading */

	"LAST_COMMAND_TAG"
};
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

