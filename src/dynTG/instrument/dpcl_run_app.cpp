// dpcl_run_app.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 24 August 2001

#include <sys/wait.h>
#include <dpcl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "command_tags.h"
#include "collector_pack.h"
#include "dpcl_run_app.h"
#include "parse_program.h"
#include "tg_globals.h"
#include "dpcl_callbacks.h"
#include "tg_time.h"
#include "dpcl_error.h"


static char ** build_args( char * poe_path, char * program,
		                int argc, char * argv );
static void destroy_args( char ** args );
static int check_parallel( char * program );

#define MAX_HOST_NAME 257
#define MAX_PROGRAM_NAME 1048
#define MAX_CWD 1048
// Works on baby, with overrun (program doesn't stop immediately when created)
#define BABY_POE_PATH "/usr/bin/poe"
// Works on blue, with overrun
// As of 9/28/2001, both /usr/bin/poe and /usr/local/PE/misc/poe seem to work
#define BLUE_POE_PATH "/usr/bin/poe"
// #define BLUE_POE_PATH "/usr/local/PE/misc/poe"
// #define BLUE_POE_PATH "/g/g0/johnmay/baby/Infra/DB/poe_dpcl"
// Works on snow
// /usr/bin/poe doesn't yet work on snow 12/20/2001 - "remove target prelink"
// #define SNOW_POE_PATH "/usr/local/PE/misc/poe"
#define SNOW_POE_PATH "/usr/bin/poe"
// Default guess
#define DEFAULT_POE_PATH "/usr/bin/poe"


int run_app_sequential( char * program, int argc, char * arg_string )
{
	// See if it should be running parallel instead
	if( check_parallel( program ) ) {
		return run_app_parallel( program, argc, arg_string );
	}

//	TG_timestamp( "DPCL thread: Creating serial application\n");

	TG_process = new Process;
	TG_app = new Application;

        // Make sure first character in program name is '/'
	if( strlen( program ) >= MAX_PROGRAM_NAME ) {
		fprintf( stderr, "Program name %d too long (%d bytes max)\n",
				program, MAX_PROGRAM_NAME + 1 );
		return -1;
	}
	
	char program_path[MAX_PROGRAM_NAME + MAX_CWD];
	if( *program != '/' ) {
		char cwd[MAX_CWD];
		if( getcwd( cwd, MAX_CWD ) == NULL ) {
			perror(program);
			return -1;
		}
		sprintf( program_path, "%s/%s", cwd, program );
	} else {
		sprintf( program_path, "%s", program );
	}
	
	char ** argvp = build_args( NULL, program_path, argc, arg_string );
	if( argvp == NULL ) return -1;

	char host_name[MAX_HOST_NAME];
	gethostname( host_name, MAX_HOST_NAME );
#ifdef TG_USE_BCREATE 
	AisStatus result = TG_process->bcreate( host_name, program_path,
				argvp, TG_envp, stdout_cb, NULL,
				stderr_cb, NULL );
#endif
	AisStatus result = TG_process->create( host_name, program_path,
					     argvp, TG_envp, stdout_cb, 
					     NULL, stderr_cb, NULL,
					     startup_cb, NULL );

	// Tell user and exit cannot create application -JCG 3/17/06
	if (result.status() != ASC_success)
	{
	    DPCL_error (
		"Error: DPCL failed while starting serial application:\n"
		"       %s\n"
		"       DPCL status code %i\n", 
		program_path, result.status());
	}

#ifdef TG_DEBUG_REDIRECT_OUTPUT
	AisStatus result = TG_process->bcreate( host_name, program_path,
				argvp, TG_envp, NULL,
			"/g/g0/johnmay/baby/Toolgear/DPCLcollector/dpclout", 
			"/g/g0/johnmay/baby/Toolgear/DPCLcollector/dpclerr",
				NULL, NULL,
				NULL, NULL );
#endif

	destroy_args( argvp );
#ifdef TG_USE_BCREATE
	pack_and_send_int_result( result.status(), result.status_name(),
		DPCL_INITIALIZE_APP_RESULT, TG_dpcl_socket_out );
	if( result.status() != ASC_success ) {
		return -1;
	}

	TG_process->battach();	// Attaching lets us control execution

	//TG_timestamp( "DPCL thread: Created serial application\n");
#endif
	TG_app->add_process( TG_process );
	TG_program_path = strdup( program_path );

	// Assume one thread for now.  Declare one proc and thread.
	// Use pointer to process for proc id.
//	pack_and_send_process_thread( (int)TG_p, 1, sock );

	return 0;
}

int run_app_parallel( char * program, int argc, char * arg_string )
{
	TG_app = new PoeAppl;
	TG_is_poe_app = 1;

        // Make sure first character in program name is '/'
	if( strlen( program ) >= MAX_PROGRAM_NAME ) {
		fprintf( stderr, "Program name %d too long (%d bytes max)\n",
				program, MAX_PROGRAM_NAME + 1 );
		return -1;
	}
	
	char program_path[MAX_PROGRAM_NAME + MAX_CWD];
	if( *program != '/' ) {
		char cwd[MAX_CWD];
		if( getcwd( cwd, MAX_CWD ) == NULL ) {
			perror(program);
			return -1;
		}
		sprintf( program_path, "%s/%s", cwd, program );
	} else {
		sprintf( program_path, "%s", program );
	}

	char host_name[MAX_HOST_NAME];
	gethostname( host_name, MAX_HOST_NAME );
	char * ppath;
#if 0
	// Figure out which verion of poe to use (highly LLNL
	// specific!!)
	if( strncmp(host_name, "baby", 4) == 0 ) {
		ppath = BABY_POE_PATH;
	} else if( strncmp( host_name, "snow", 4) == 0 ) {
		ppath = SNOW_POE_PATH;
	} else if( strncmp( host_name, "blue", 4) == 0 ) {
		ppath = BLUE_POE_PATH;
	} else {
		ppath = DEFAULT_POE_PATH;
		printf("Don't know what poe version to use\n"
				"trying %s\n", ppath );
	}
#endif
	ppath = DEFAULT_POE_PATH;
	char ** argvp = build_args( ppath, program_path, argc, arg_string );
	if( argvp == NULL ) return -1;

	//TG_timestamp( "DPCL thread: Creating POE application\n");
#ifdef TG_USE_BCREATE
	AisStatus result = ((PoeAppl *)TG_app)->bcreate( host_name, ppath,
					     argvp, TG_envp, stdout_cb, 
					     NULL, stderr_cb, NULL );

#endif
	AisStatus result = ((PoeAppl *)TG_app)->create( host_name, ppath,
					     argvp, TG_envp, stdout_cb, 
					     NULL, stderr_cb, NULL,
					     startup_cb, NULL );

	// Tell user and exit cannot create application -JCG 3/17/06
	if (result.status() != ASC_success)
	{
	    DPCL_error (
		"Error: DPCL failed while starting parallel application:\n"
		"       %s\n"
		"       DPCL status code %i\n", 
		program_path, result.status());
	}

	//TG_timestamp( "DPCL thread: Returned from create call\n");
	destroy_args( argvp );

#ifdef TG_USE_BCREATE 
	pack_and_send_int_result( result.status(), result.status_name(),
		DPCL_INITIALIZE_APP_RESULT, TG_dpcl_socket_out );

	if( result.status() != ASC_success ) {
		return -1;
	}

	TG_app->battach();	// Attaching lets us control execution

	//TG_timestamp( "DPCL thread: Created POE application\n");
#endif

	TG_program_path = strdup( program_path );

#if 0
	// Declare each process in the application.  Assume one
	// thread per process for now.  Number the processes
	// sequentially from 0.  Does this correspond to MPI rank??
	for( int i = 0; i < TG_app->get_count(); i++ ) {
		pack_and_send_process_thread( i, 1, TG_dpcl_socket_out );
	}

	if( ! dont_parse ) parse_application( *TG_app, TG_dpcl_socket_out );
#endif

	return 0;
}

// Build a list of arguments for the App initialization.  The
// assumed form of arg_string is:
//  <prog-name>\0[<option>\0+]
//  The calling program has already put the program name in the
//  form of a full path and passed it here separately, so we can
//  ignore the first arg in the list.
// We want an arg list of string pointers like this:
//   [<poe_path>] <program_path> [options...] NULL
// Returns NULL and cleans up in case of allocation errors.
char ** build_args( char * poe_path, char * program,
		int argc, char * arg_string )
{
	int arg_index = 0;
	// Argc arguments + 1 for NULL + maybe 3 for poe
	char ** args = new char*[argc + 1 + (poe_path != NULL ? 3 : 0)];

	if( args == NULL ) return NULL;


	// Store the poe path name, if one was given
	if( poe_path != NULL ) {
		args[arg_index] = new char[strlen( poe_path ) + 1];
		if( args[arg_index] == NULL ) {
			delete args;
			return NULL;
		}
		strcpy( args[arg_index++], poe_path );
	}

	// Store the program name
	args[arg_index] = new char[strlen( program ) + 1];
	if( args[arg_index] == NULL ) {
		if( poe_path != NULL ) delete [] args[0];
		delete args;
		return NULL;
	}
	strcpy( args[arg_index++], program );

	// Copy the program args from the string; skip the first item
	// in the arg_string because that's the program name, and we've
	// already copied it.
	char * p = arg_string + strlen( arg_string ) + 1;
	int i;
	for( i = 1; i < argc; i++, arg_index++ ) {
		int length = strlen( p ) + 1;
		args[arg_index] = new char[length];
		if( args[arg_index] == NULL ) {
			for( int j = arg_index - 1; j <= 0; j-- ) {
				delete [] args[j];
			}
			delete args;
			return NULL;
		}
		strcpy( args[arg_index], p );
		p += length;			// Next opt string
	}

	// Store a flag that will disable attempted storage of core
	// files when the program gets a sigterm (which is how DPCL
	// shuts down an application).  This works for poe 3.1.10
	// and later.
	if( poe_path != NULL ) {
		args[arg_index++] = "-corefile_sigterm";
		args[arg_index++] = "no";
	}

	args[arg_index] = NULL;

	return args;
}

// Clean up memory allocated by build_args
void destroy_args( char ** args )
{
	if( args == NULL ) return;

	char ** p;
	for( p = args; *p != NULL; p++ ) {
		delete [] *p;
	}

	delete args;
}

// Calls a shell script that attempts to determine if a program is
// going to run as a parallel job.
#define CHECK_PARALLEL "AIXisParallel"
int check_parallel( char * program )
{
	char * args[3];

	// The CHECK_PARALLEL program should be in the same directory as
	// this program, so get the path and substitute in the name of
	// the CHECK_PARALLEL program.
	char * check_path = new char[strlen(TG_argv[0])
					+ strlen(CHECK_PARALLEL)];
	strcpy( check_path, TG_argv[0] );
	char * p_last;
	if( (p_last = strrchr( check_path, '/' )) == NULL ) { // No prefix
		strcpy( check_path, CHECK_PARALLEL );
	} else {
		strcpy( p_last + 1, CHECK_PARALLEL );
	}

	args[0] = check_path;
	args[1] = program;
	args[2] = NULL;

	pid_t child = fork();

	if( child == 0 ) {
		// Run the check program; if it fails, exit the
		// child process with status -1 to warn the parent
		if( execv( args[0], (char * const *) args ) == -1 )
			exit( -1 );
	}

	// Wait for the result from the check program
	int retval = -1;
	while( waitpid( child, &retval, 0 ) == -1 ) {
		if( errno != EINTR ) {
			retval = -1;
			break;
		}
	}


	// Enhanced error checking -JCG 3/17/06
	// Got a good result
	if( (retval != -1) &&
	    ((WEXITSTATUS(retval) == 0) || (WEXITSTATUS(retval) == 1) ))
	{
	    delete [] check_path;
	    return retval;
	} else 
	{
	    DPCL_error ("Error: Unable to run '%s'.\n"
			"       Used to determine if target program is parallel or serial.\n"
			"       waitpid() return code %i\n",
			check_path, WEXITSTATUS(retval));

	    // Failed somehow; assume not parallel
	    return 0;
	}
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

