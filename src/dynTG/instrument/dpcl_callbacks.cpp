// dpcl_callbacks.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 24 August 2001

#include <dpcl.h>
#include <stdio.h>

#include "command_tags.h"
#include "tg_globals.h"
#include "dpcl_callbacks.h"
#include "collector_pack.h"
#include "parse_program.h"
#include "tg_time.h"
#include "tg_socket.h"
#include "dpcl_action_instance.h"

// For testing only
#include <string>
using std::string;
#include <vector>
using std::vector;
#include "tg_inst_point.h"
#include "dpcl_instrument_app.h"

void stdout_cb(GCBSysType sys, GCBTagType tag, GCBObjType obj, GCBMsgType msg)
{							 
#ifdef TG_STDOUT_TO_FILE
	char name[100];
	sprintf( name, "stdout%d", getpid() );
	FILE * fp = fopen( name, "a" );
#endif
	char *p = (char *) msg;
	for (int i=0; i<sys.msg_size; i++) {
#ifdef TG_STDOUT_TO_FILE
		fputc( *p, fp );
#else
		printf("%c",*p);
#endif
#if 0
		if ((*p == '\n') && (i < (sys.msg_size -1)))
		{
			printf("stdout: ");
		}
#endif
		p++;
	}   
	p = (char *) msg;
	if (p[sys.msg_size-1] != '\n')
	{
#ifdef TG_STDOUT_TO_FILE
		fputc('\n', fp);
#else
		printf("\n");
#endif
	} 

#ifdef TG_STDOUT_TO_FILE
	fclose( fp );
#else
	fflush(stdout);
#endif


}

void stderr_cb(GCBSysType sys, GCBTagType tag, GCBObjType obj, GCBMsgType msg)
{						 
#ifndef TG_STDERR_TO_FILE
	printf("stderr: ");   
#else
	char name[100];
	sprintf( name, "stderr%d", getpid() );
	FILE * fp = fopen( name, "a" );
#endif
	char *p = (char *) msg;
	for (int i=0; i<sys.msg_size; i++) {
#ifdef TG_STDERR_TO_FILE
		fputc(*p, fp);
#else
		printf("%c", *p);
#endif
	
#if 0
		if ((*p == '\n') && (i < (sys.msg_size -1)))
		{
			printf("stderr: ");
		}
#endif
		p++;
	}
	p = (char *) msg;
	if (p[sys.msg_size-1] != '\n')
	{
#ifdef TG_STDERR_TO_FILE
		fputc('\n', fp);
#else
		printf("\n");
#endif
	} 

#ifdef TG_STDERR_TO_FILE
	fclose( fp );
#else
	fflush(stdout);
#endif
}		 

void termination_cb(GCBSysType sys, GCBTagType tag, GCBObjType obj,
		GCBMsgType msg)
{
	// msg is a string containing the pid that terminated 
	// (including the host name), and tag is a pointer to
	// an int identifying the socket for communication with 
	// the client (but for poe jobs, the tag doesn't seem to
	// get passed correctly)
	pack_and_send_termination( (char *)msg, TG_dpcl_socket_out );
}

// This function is called when an application has been
// started successfully.
#define MAX_ANNOT_PATH_LENGTH (10000)
void startup_cb(GCBSysType sys, GCBTagType tag, GCBObjType obj,
		GCBMsgType msg)
{
	AisStatus * p_result = (AisStatus *)msg;

	if( p_result->status() != ASC_success ) {
		TG_app = NULL;
		TG_process = NULL;
	} else {

#ifdef TG_USE_SUSPEND
		// Not needed if we start/stop using attach/detach
		TG_app->battach();
#endif
	//	TG_timestamp( "DPCL thread: Created application\n" );

                // Right now, the Client really wants to know
		// about all task/thread pairs up front.  So, we
		// are forced to make up thread '1' for now for
		// all the tasks we know about now
		for( int i = 0; i < TG_app->get_count(); i++ ) {
			pack_and_send_process_thread( i , 1,
					TG_dpcl_socket_out );
		}

		if( ! dont_parse ) {
			ModuleInfo * modules;
			int num_modules = parse_application( *TG_app, modules );

			// Send the list of modules found to the Client
			if( num_modules > 0 ) {
				char annot_file_name[MAX_ANNOT_PATH_LENGTH];
				for( int i = 0; i < num_modules; i++ ) {
					snprintf( annot_file_name,
						MAX_ANNOT_PATH_LENGTH,
						"%d_%d %s",
						modules[i].process_id,
						modules[i].module_id,
						modules[i].file_path );
					pack_and_send_module( annot_file_name, 
						TG_dpcl_socket_out );
				}
				TG_flush( TG_dpcl_socket_out );
			}

			delete [] modules;
		}

		// Find all initialization operations and execute them.
		list<DPCLActionTypeP>::iterator i;
		for( i = initializationList.begin();
				i != initializationList.end(); ++i ) {
			DPCLOneShotAction( TG_app, *i );
		}
	}

	// Send this last so the user can't try to start the program
	// before parsing is done.
	pack_and_send_int_result( p_result->status(), p_result->status_name(),
			DPCL_INITIALIZE_APP_RESULT, TG_dpcl_socket_out );

}

void shutdown_cb(GCBSysType sys, GCBTagType tag, GCBObjType obj,
		GCBMsgType msg)
{

	AisStatus * p_result = (AisStatus *)msg;
//	TG_timestamp("DPCL collector: Shutdown complete: got %s\n",
//			p_result->status_name() );
	if( TG_app != NULL ) 
		delete TG_app;

	if( TG_process != NULL )
		delete TG_process;

#if 0
	// Acknowledge quitting to client
	TG_error ("DPCL collector: Acknowledging quit\n");
#endif
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

