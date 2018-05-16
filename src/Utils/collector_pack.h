//! \file collector_pack.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
//! These functions pack data into the appropriate format for
//! given type of message and then send the message to the Client.
// John May, 12 October 2000

#ifndef COLLECTOR_PACK_H
#define COLLECTOR_PACK_H
#include "tg_inst_point.h"
#include "command_tags.h"

//! Sends a file name from the target program.
extern void pack_and_send_module( const char * modulePath, int socket );

//! Sends a function name from the target program.
extern void pack_and_send_function( const char * function,
				    const char * path, int start, int stop,
				    int socket );

//! Indicates that all function names from the specified module have been
//! sent.
extern void pack_and_send_module_parsed( const char * modulePath, int socket );

//! Sends a program location not associated with a DPCL instrumentation
//! point.
extern void pack_and_send_plain_entry( const char * function,
				       const char * tag, int line,
				       const char * description, int socket );

//! Sends a program location and its corresponding DPCL instrumentation
//! point.
extern void pack_and_send_entry( const char * function, const char * tag,
				 int line, 
				 TG_InstPtType type, 
				 TG_InstPtLocation location,
				 const char *funcCalled, int callIndex,
				 const char * description, int socket );

//! Indicates that all instrumentation points names from the
//! specified function have been sent.
extern void pack_and_send_function_parsed( const char *funcName, int socket );

//! Sends information for declaring an action to the Client.
extern void pack_and_send_action_attr( const char * action_name,
				       const char * action_label,
				       const char * description, int socket);

//! Indicates that the specifie action has been enabled in the target program.
extern void pack_and_send_enable_action( const char * function,
					 const char * tag,
					 const char * action_name, int socket);

//! Sends information for declaring a data column in the Client database.
extern void pack_and_send_data_attr( const char * dataAttrTag,
				     const char * dataAttrText, 
				     const char * description, int dataType, 
				     int socket );

//! Sends information for declaring a process/thread pair.
extern void pack_and_send_process_thread( int process, int thread, 
					  int socket );

//! Sends an integer value representing the result of a requested
//! action (such as changing directories or starting a program).
extern void pack_and_send_int_result( int result, const char * message,
				      int tag,
				      int socket );

//! Sends a double to be inserted in the database.
extern void pack_and_send_double( const char * function, const char * tag, 
				  const char * dataAttrTag, int process,
				  int thread, double data, int socket );

//! Sends an int to be inserted in the database.
extern void pack_and_send_int( const char * function, const char * tag, 
			       const char * dataAttrTag, int process,
			       int thread, int data, int socket );

//! Sends a double to be added to an exisiting value in the database.
extern void pack_and_send_add_double( const char * function, const char * tag, 
				      const char * dataAttrTag, int process, 
				      int thread, double data, int socket );

//! Sends an int to be added to an exisiting value in the database.
extern void pack_and_send_add_int( const char * function, const char * tag, 
				   const char * dataAttrTag, int process, 
				   int thread, int data, int socket );

//! No longer used.
extern void pack_and_send_target_info( const char * program, const char * host, 
				       int socket );

//! Indicates that the target program has reached a breakpoint.
extern void pack_and_send_target_halted( const char * function,
					 const char * tag, int process,
					 int socket );

//! Indicates that a process in the target program has terminated.
extern void pack_and_send_termination( const char * pid, int socket );

//! Requests Client to create a display window to present data.
extern void pack_and_send_create_viewer( ViewType t, int socket );

//! Requests Client to create a display window to present data with no
//! program control on instrumentation options.
extern void pack_and_send_create_static_viewer( int socket );

//! Inserts text in the About... box for the GUI
extern void pack_and_send_info_about_tool( const char * tool_info, int socket );

//! Notify Client that all data from post-mortem file has been sent
extern void pack_and_send_static_data_complete( int socket );

//! Declares a new messageFolder that messages can be added to
extern void pack_and_send_declare_message_folder (const char *messageFolderTag,
					        const char *messageFolderTitle,
					        int socket);

//! Adds a new message to messageList
extern void pack_and_send_add_message (const char *messageFolderTag, 
				       const char *messageText,
				       const char *messageTraceback, 
				       const char *supplementalTraceback,
				       int socket);

/*--------------------------------------------------------------------------*/
/* MS/START - dynamic module loading */

void pack_and_sendModuleCount(int socket, int count);
void pack_and_sendModuleName(int socket, int slot, char *name, char *shortname);
void pack_and_sendModuleQuery(int socket);

/* MS/END - dynamic module loading */
/*--------------------------------------------------------------------------*/

#endif // COLLECTOR_PACK_H

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

