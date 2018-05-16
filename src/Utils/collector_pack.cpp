// collector_pack.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 12 October 2000

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "collector_pack.h"
#include "tg_pack.h"
#include "tg_socket.h"
#include "command_tags.h"
#include "tg_error.h"

#define BUFFER_SIZE (1<<13)

void pack_and_send_module( const char * modulePath, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "S", modulePath);

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_INSERT_MODULE], DB_INSERT_MODULE );
#endif
	TG_send( socket, DB_INSERT_MODULE, 0, length, buffer );
}

void pack_and_send_function( const char * function, const char * path,
				int start, int stop, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSII", function, path,
			start, stop );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_INSERT_FUNCTION],
			DB_INSERT_FUNCTION );
#endif
	TG_send( socket, DB_INSERT_FUNCTION, 0, length, buffer );
}

void pack_and_send_module_parsed( const char * modulePath, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "S", modulePath);

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_FILE_PARSE_COMPLETE],
			DB_FILE_PARSE_COMPLETE );
#endif
	TG_send( socket, DB_FILE_PARSE_COMPLETE, 0, length, buffer );
}

void pack_and_send_plain_entry( const char * function, const char * tag,
				int line, const char * description, int socket )
{                       
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSIS", function, tag,
				  line, description );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_INSERT_PLAIN_ENTRY],
			DB_INSERT_PLAIN_ENTRY );
#endif
	TG_send( socket, DB_INSERT_PLAIN_ENTRY, 0, length, buffer );
}       

void pack_and_send_entry( const char * function, const char * tag, int line, 
			  TG_InstPtType type, TG_InstPtLocation location,
			  const char *funcCalled, int callIndex,
			  const char * description, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSIIISIS", function, tag, 
			  line, type, location, funcCalled, callIndex, 
			  description );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_INSERT_ENTRY], DB_INSERT_ENTRY );
#endif
	TG_send( socket, DB_INSERT_ENTRY, 0, length, buffer );
}

void pack_and_send_function_parsed( const char *funcName, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "S", funcName);

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_FUNCTION_PARSE_COMPLETE],
			DB_FUNCTION_PARSE_COMPLETE );
#endif
	TG_send( socket, DB_FUNCTION_PARSE_COMPLETE, 0, length, buffer );
}

void pack_and_send_action_attr( const char * action_name,
				const char * action_label,
				const char * description, int socket)
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSS", action_name, action_label,
		    description );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_DECLARE_ACTION_ATTR],
			DB_DECLARE_ACTION_ATTR );
#endif
	TG_send( socket, DB_DECLARE_ACTION_ATTR, 0, length, buffer );
}

void pack_and_send_enable_action( const char * function, const char * tag,
				const char * action_name, int socket)
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSS", function, tag,
			action_name );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_ENABLE_ACTION], DB_ENABLE_ACTION );
#endif
	TG_send( socket, DB_ENABLE_ACTION, 0, length, buffer );
}

void pack_and_send_data_attr( const char * dataAttrTag,
		const char * dataAttrText, const char * description,
		int dataType, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSSI", dataAttrTag,
			dataAttrText, description, dataType );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_DECLARE_DATA_ATTR],
			DB_DECLARE_DATA_ATTR );
#endif
	TG_send( socket, DB_DECLARE_DATA_ATTR, 0, length, buffer );
}

void pack_and_send_process_thread( int process, int thread, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "II", process, thread, socket );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_INSERT_PROCESS_THREAD],
			DB_INSERT_PROCESS_THREAD );
#endif
	TG_send( socket, DB_INSERT_PROCESS_THREAD, 0, length, buffer );
}

void pack_and_send_int_result( int result, const char * message,
		int tag, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "IS", result, message );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[tag], tag );
#endif
	TG_send( socket, tag, 0, length, buffer );
}

void pack_and_send_double( const char * function, const char * tag,
		const char * dataAttrTag, int process, int thread,
		double data, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSSIID", function, tag,
			dataAttrTag, process, thread, data );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_INSERT_DOUBLE], DB_INSERT_DOUBLE );
#endif
	TG_send( socket, DB_INSERT_DOUBLE, 0, length, buffer );
}

void pack_and_send_add_double( const char * function, const char * tag,
		const char * dataAttrTag, int process, int thread,
		double data, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSSIID", function, tag,
			dataAttrTag, process, thread, data );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_ADD_DOUBLE], DB_ADD_DOUBLE );
#endif
	TG_send( socket, DB_ADD_DOUBLE, 0, length, buffer );
}

void pack_and_send_int( const char * function, const char * tag,
		const char * dataAttrTag, int process, int thread,
		int data, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSSIII", function, tag,
			dataAttrTag, process, thread, data );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_INSERT_INT], DB_INSERT_INT );
#endif
	TG_send( socket, DB_INSERT_INT, 0, length, buffer );
}

void pack_and_send_add_int( const char * function, const char * tag,
		const char * dataAttrTag, int process, int thread,
		int data, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSSIII", function, tag,
			dataAttrTag, process, thread, data );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_ADD_INT], DB_ADD_INT );
#endif
	TG_send( socket, DB_ADD_INT, 0, length, buffer );
}

void pack_and_send_target_info( const char * program, const char * host,
		int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SS", program, host );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[GUI_SET_TARGET_INFO],
			GUI_SET_TARGET_INFO );
#endif
	TG_send( socket, GUI_SET_TARGET_INFO, 0, length, buffer );
}

void pack_and_send_target_halted( const char * function, const char * tag,
		int process, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "SSI", function, tag,
			process );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DPCL_TARGET_HALTED],
			DPCL_TARGET_HALTED );
#endif
	TG_send( socket, DPCL_TARGET_HALTED, 0, length, buffer );
}

void pack_and_send_termination( const char * proc_info, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "S", proc_info );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DPCL_TARGET_TERMINATED],
			DPCL_TARGET_TERMINATED );
#endif
	TG_send( socket, DPCL_TARGET_TERMINATED, 0, length, buffer );

	// Presumably, we don't want to wait around for termination, so
	// always flush it right away.
	TG_flush( socket );
}

void pack_and_send_create_viewer( ViewType t, int socket )
{
#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[GUI_CREATE_VIEWER], GUI_CREATE_VIEWER );
#endif
	TG_send( socket, GUI_CREATE_VIEWER, (int)t, 0, 0 );
}

void pack_and_send_create_static_viewer( int socket )
{
#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[GUI_CREATE_STATIC_VIEWER],
			GUI_CREATE_STATIC_VIEWER );
#endif
	TG_send( socket, GUI_CREATE_STATIC_VIEWER, 0, 0, 0 );
}

void pack_and_send_info_about_tool( const char * tool_info, int socket )
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack( buffer, BUFFER_SIZE, "S", tool_info );

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DPCL_TOOL_INFO], DPCL_TOOL_INFO );
#endif
	TG_send( socket, DPCL_TOOL_INFO, 0, length, buffer );

}
	
void pack_and_send_static_data_complete( int socket )
{
#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_STATIC_DATA_COMPLETE],
			DB_STATIC_DATA_COMPLETE );
#endif
	TG_send( socket, DB_STATIC_DATA_COMPLETE, 0, 0, 0 );
}

void pack_and_send_declare_message_folder (const char *messageFolderTag, 
					 const char *messageFolderTitle,
					 int socket)
{
	char buffer[BUFFER_SIZE];
	int length;

	length = TG_pack (buffer, BUFFER_SIZE, "SS", messageFolderTag,
			messageFolderTitle);

#ifdef DEBUG_TAGS
	fprintf( stderr, "%s %d Sending %s, code %d\n",
			__FILE__, __LINE__,
			command_strings[DB_DECLARE_MESSAGE_FOLDER],
			DB_DECLARE_MESSAGE_FOLDER );
#endif
	TG_send (socket, DB_DECLARE_MESSAGE_FOLDER, 0, length, buffer);

}

void pack_and_send_add_message (const char *messageFolderTag, 
				const char *messageText,
				const char *messageTraceback, 
				const char *supplementalTraceback,
				int socket)
{
    MessageBuffer mbuf;
    int length;

    // Use MessageBuffer version to prevent buffer overflow
    length = TG_pack (mbuf, "SSSS", messageFolderTag,
		      messageText, messageTraceback, supplementalTraceback);

    TG_send (socket, DB_ADD_MESSAGE, 0, length, mbuf.contents());

}

/*--------------------------------------------------------------------------*/
/* MS/START - dynamic module loading */

void pack_and_sendModuleCount(int socket, int count)
{
  char buffer[BUFFER_SIZE];
  int length;
  
  length = TG_pack( buffer, BUFFER_SIZE, "I", count);
  TG_send( socket, DYNCOLLECT_NUMMODS, 0, length, buffer );
}

void pack_and_sendModuleName(int socket, int slot, char *name, char *shortname)
{
  char buffer[BUFFER_SIZE];
  int length;
  
  length = TG_pack( buffer, BUFFER_SIZE, "ISS", slot,name,shortname);
  TG_send( socket, DYNCOLLECT_NAME, 0, length, buffer );
}

void pack_and_sendModuleQuery(int socket)
{
  TG_send(socket, DYNCOLLECT_ASK, 0, 0, 0);
}
/* MS/END - dynamic module loading */
/*--------------------------------------------------------------------------*/
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

