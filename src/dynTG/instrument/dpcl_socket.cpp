// dpcl_socket.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

// John May, 27 October 2000
// Receive and dispatch control messages.

#include <errno.h>
#include <fcntl.h>
#include <map>
using std::map;
#include <list>
using std::list;
#include <regex.h>
#include <stdlib.h>
#include <string>
using std::string;
#include <string.h>
#include <sys/mode.h>
#include <sys/stat.h>
#include <dpcl.h>
#include <utility>
using std::pair;
using std::make_pair;
#include <vector>
using std::vector;
#include "tg_socket.h"
#include "tg_source_reader.h"
#include "tg_pack.h"
#include "dpcl_socket.h"
#include "collector_pack.h"
#include "command_tags.h"
#include "dpcl_action_type.h"
#include "dpcl_action_instance.h"
#include "dpcl_callbacks.h"
#include "dpcl_instrument_app.h"
#include "dpcl_run_app.h"
#include "parse_program.h"
#include "tg_globals.h"
#include "tg_time.h"
#include "dpcl_error.h"

/* MS/START - dynamic collector */
#include <dlfcn.h>
#include "base.h"
/* MS/END - dynamic collector */

#define USE_SOURCE_READER 1
TGSourceReader * sourceReader;

// Thread-safe version of error string library for IBM
#define USE_STRERROR_R

#define TG_REMOVE_INSTRUMENTATION 1

static int check_continue_search( void );

int dpcl_socket_handler( int fd )
{
	char *buf;
	int tag, id, size;
	int quitting = 0;
    
	// Block further "interrupts" while processing this input;
	// can happen if we make DPCL calls inside this handler.
	AisStatus status = Ais_remove_fd( fd );

	// Use non-blocked receive.  Get all messages waiting and
	// then return.
	while( TG_nb_recv( fd, &tag, &id, &size, (void **)&buf) != 0 ) {

#ifdef DEBUG
		printf("dpcl read: tag=%i id=%i size=%i buf='%s'\n",
			tag, id, size, buf);
#endif
		switch( tag ) {
			case DPCL_ACTIVATE_ACTION:
			case DPCL_ACTIVATE_ACTION_ALL:
				if( TG_app != NULL )
					activate_action( TG_app, buf, tag );

				break;
			case DPCL_DEACTIVATE_ACTION:
			case DPCL_DEACTIVATE_ACTION_ALL:
				if( TG_app != NULL )
					deactivate_action( TG_app, buf, tag ); 
				break;
			case DPCL_INITIALIZE_APP_PARALLEL:
				unpack_parallel_app( buf );
				break;
			case DPCL_INITIALIZE_APP_SEQUENTIAL:
				unpack_sequential_app( buf );
				break;
			case DPCL_CHANGE_DIR:
				unpack_change_dir( buf );
				break;
			case DPCL_PARSE_MODULE:
				if( TG_app != NULL && !dont_parse )
					unpack_parse_module( TG_app, buf );
				break;
			case DPCL_PARSE_FUNCTION:
				if( TG_app != NULL && !dont_parse )
					unpack_parse_function( TG_app, buf );
				break;
			case DPCL_INSTRUMENT_APP:
				if( TG_app != NULL )
					instrument_app( TG_app, buf );
				break;
			case DPCL_START_PROGRAM:
				if( TG_app != NULL )
					start_program( TG_app );
				break;
			case DPCL_STOP_PROGRAM:
				if( TG_app != NULL )
					stop_program( TG_app );
				break;
			case DPCL_READ_FILE:
#ifdef USE_SOURCE_READER
				if( ! sourceReader ) {
					sourceReader = new TGSourceReader( fd,
							check_continue_search );
				}
				sourceReader->read_file( buf, id );
#else
				read_file( buf, id, sock );
#endif
				break;
#ifdef USE_SOURCE_READER
			case COLLECTOR_GET_SUBDIRS:
				if( ! sourceReader ) {
					sourceReader = new TGSourceReader( fd,
							check_continue_search );
				}
				sourceReader->unpack_get_subdirs( buf );
				break;
			case COLLECTOR_REPORT_SEARCH_PATH:
				if( ! sourceReader ) {
					sourceReader = new TGSourceReader( fd,
							check_continue_search );
				}
				sourceReader->report_search_path( buf );
				break;
			case COLLECTOR_SET_SEARCH_PATH:
				sourceReader->unpack_set_source_path( buf );
				break;
			case COLLECTOR_CANCEL_READ_FILE:
				return tag;
#endif
			case DPCL_SET_HEARTBEAT:
				unpack_heartbeat( buf );
				break;
			case GUI_SAYS_QUIT:
//			        TG_timestamp ("DPCL thread: got Quit request "
//					      "from GUI\n");
				shutdown_app();
				Ais_end_main_loop();	// loop exits when this
							// function returns
				quitting = 1;
				break;

			/* MS/START - dynamic collector */

		        case DYNCOLLECT_LOADMODULE:
			  {
			    inittool_t initroutine;
			    int mod,err;
			    
			    TG_unpack(buf,"I",&mod);

			    if (modhandles[mod]!=NULL)
			      {
				/* query routine */
				
				initroutine=(inittool_t) dlsym(modhandles[mod],"InitializeDPCLTool");
				if (initroutine==NULL)
				  {
				      DPCL_error ("Error: unable to find InitializeDPCLTool symbol in Module %i\n", mod);

				  }
		
				/* run the init routine for this module */
				
				err=(*initroutine)(fd, modhandles[mod]);
				if (err!=0)
				  {
				      DPCL_error ("Error: error %i running InitializeDPCLTool for Module %i\n", err, mod);
				  }
			      }
			  break;
			  }

		        case DYNCOLLECT_ALLLOADED:
			  {
			    // Now that all actions are defined, tell the client to create
			    // the viewer.
	
			    pack_and_send_create_viewer( TREE_VIEW, fd );
			    TG_flush( fd );
			    break;
			  }

			  /* MS/END - dynamic collector */

			default:
				fprintf( stderr,
					"Unknown dpcl_socket_handler tag %d\n",
						tag );
		}

		free( buf );
	}


	// Reactivate "interrupts" for this fd, but only if they
	// were successfully disabled earlier
	if( !quitting ) {
		if( status == ASC_success ) {
			status = Ais_add_fd( fd, dpcl_socket_handler );
		}
	}

	// Returning -1 (if we were about to quit) would
	// close the connection entirely, so we couldn't send
	// info back to the Client.
	return 0;
}

void activate_action( Application * app, char * buf, int tag )
{
	char * funcName;
	char * entryKey;
	char * actionAttrTag;
	int taskId;
	int threadId;

	// If we're activating on just one process (not all), unpack the ids
	// The threadId is unused for now.
	if( tag == DPCL_ACTIVATE_ACTION ) {
		TG_unpack( buf, "SSSII", &funcName, &entryKey, &actionAttrTag,
				&taskId, &threadId );
	} else {
		TG_unpack( buf, "SSS", &funcName, &entryKey, &actionAttrTag );
	}

	// InstPoint ip = tag_to_inst_point( app, entryKey );
	map<string,IPInfoP>::iterator p_info = ipinfo.find( entryKey );
	if( p_info == ipinfo.end() ) {
		fprintf( stderr, "Unknown Instrumentation Point tag %s\n",
				entryKey );
		return;
	}

	vector<DPCLActionTypeP> action_types;	// Need a vector to work

	action_types.push_back( actionList[actionAttrTag] );
	if( !action_types[0] ) {
		fprintf( stderr, "unknown action request %s\n", actionAttrTag );
		// Remove the action type we created by accident
		actionList.erase( actionAttrTag );
		return;
	}

#ifdef DEBUG
	fprintf( stderr, "%s in %s on %s (%d %d)\n",
			actionAttrTag, funcName, entryKey,
			taskId, threadId );
#endif
	// Intitialize the "function object" that applies the instrumentation
	ApplyInstrumentation apply( action_types, *app, ap_list,
				(tag == DPCL_ACTIVATE_ACTION) ?
				taskId : ALL_PROCESSES );

	// Now apply the instrumentation (this is an overloaded function op
	// call; we designed it this way to make automatic instrumentation
	// easier in another part of the code.)
	apply( *p_info );
}

void deactivate_action( Application * app, char * buf, int tag )
{
	char * funcName;
	char * entryKey;
	char * actionAttrTag;
	int taskId;
	int threadId;

	// taskId and threadId are unused for now
	if ( tag == DPCL_DEACTIVATE_ACTION ) {
	    TG_unpack( buf, "SSSII", &funcName, &entryKey, &actionAttrTag,
		       &taskId, &threadId );
	} else {
	    TG_unpack ( buf, "SSS", &funcName, &entryKey, &actionAttrTag );
	}
	
	DPCLActionPointP action_point = ap_list[entryKey];
	// Failed to find the corresponding action!
	if( !action_point ) {
		fprintf( stderr, "deactivate action couldn't find an "
			"active action point with the tag %s!\n", entryKey );
		// Get rid of the entry we accidentally created
		ap_list.erase( entryKey );
		return;
	}

	// Find an action attached to this point with the correct tag
	int num_actions = action_point->get_num_actions();
	int i;
	Action * a;
	for( i = 0; i < num_actions; i++ ) {
		a = action_point->get_action( i );
		if( a != NULL && a->get_text() == actionAttrTag )
			break;
	}

	// Couldn't find a corresponding action to delete
	if( i == num_actions ) {
		fprintf( stderr, "deactivate action couldn't find an "
			"action %s for entry %s!\n", actionAttrTag, entryKey );
		return;
	}

	// Delete the action from the action point; this automatically 
	// causes DPCL to remove it from the program and deallocates
	// the object.
	action_point->remove_action( i );

	// If there are no actions left on this action point, remove it.
	if( action_point->get_num_actions() == 0 ) {
		ap_list.erase( entryKey );
	}
}

void unpack_parallel_app( char * buf )
{
	char * arg_string;
	int argc;

	TG_unpack( buf, "A", &argc, &arg_string );
	
	// String contains all the args, each terminated by \0.
	// The first arg should be the program name, and since
	// it's terminated, we can just pass the arg_string.
	int retval = run_app_parallel( arg_string, argc, arg_string );

	// run_app sends result back, since it knows more about failure modes
}

void unpack_sequential_app( char * buf )
{
	char * arg_string;
	int argc;

	TG_unpack( buf, "A", &argc, &arg_string );
	
	// String contains all the args, each terminated by \0.
	// The first arg should be the program name, and since
	// it's terminated, we can just pass the arg_string.
	int retval = run_app_sequential( arg_string, argc, arg_string );

	static count = 0;
	count++;
	// run_app sends result back, since it knows more about failure modes
}

void unpack_change_dir( char * buf )
{
	char * directory;
	char * errstring;
	char result_string[500];

	TG_unpack( buf, "S", &directory );
	int retval = chdir( directory );
	if( retval ) {
		perror( "cd" );
#ifdef USE_STRERROR_R
		strerror_r( errno, result_string,
				sizeof(result_string) );
		errstring = result_string;
#else
		errstring = strerror(errno);
#endif
	} else {
		errstring = NULL;
	}

	
	pack_and_send_int_result( retval, errstring,
			DPCL_CHANGE_DIR_RESULT, TG_dpcl_socket_out );
}

#define MAX_PREFIX_FUNC_NAME (1000)
void unpack_parse_module( Application * app, char * buf )
{
        char * prefixed_file_name;
	int process_id, module_id;

	TG_unpack( buf, "S", &prefixed_file_name);

	// Pull the process_id and module_id out of the file name
	if (sscanf ( prefixed_file_name, "%d_%d ", 
		     &process_id, &module_id ) != 2)
	{
	    fprintf (stderr, 
		     "unpack_parse_module: Unexpected file prefix '%s'\n",
		     prefixed_file_name);
	    exit (1);
	}

	FunctionInfo * funcs;
	int length = parse_module( *app, process_id, module_id, funcs );
	if( length < 0 ) return;

	// Get the process and source module objects based on their IDs
        Process p = app->get_process( process_id );
        SourceObj process_source_obj = p.get_program_object();
        SourceObj module = process_source_obj.child( module_id );

	int i;
	char prefixed_func_name[MAX_PREFIX_FUNC_NAME];
	for( i = 0; i < length; i++ ) {
		snprintf( prefixed_func_name, MAX_PREFIX_FUNC_NAME,
			"%d_%d_%d %s", process_id, module_id, i,
			funcs[i].func_name );
		pack_and_send_function( prefixed_func_name, prefixed_file_name,
				funcs[i].start_line, funcs[i].end_line,
				TG_dpcl_socket_out );
	}
	pack_and_send_module_parsed (prefixed_file_name, TG_dpcl_socket_out);

	// Send all the messages to the Client
	TG_flush( TG_dpcl_socket_out );

	delete [] funcs;
}

void unpack_parse_function( Application * app, char * buf )
{
	char * prefixed_func_name;
	int process_id, module_id, function_id;

	TG_unpack( buf, "S", &prefixed_func_name );

	// Pull the process, module, and function ids out of the function name
	if (sscanf( prefixed_func_name, "%d_%d_%d ",
		    &process_id, &module_id, &function_id ) != 3)
	{
	    fprintf (stderr, 
		     "unpack_parse_function: Unexpected func prefix '%s'\n",
		     prefixed_func_name);
	    exit (1);
	}

	vector<regex_t> exps(1);
	vector<TG_InstPtLocation> locs;
	vector<TG_InstPtType> types;

	// Use a regular expression that can match anything.  Some day,
	// we may want to accept a more specific RE from the client and
	// parse the function for only matching items (without actually
	// instrumenting them).
	regcomp( &exps[0], ".*", REG_NOSUB | REG_EXTENDED );

	locs.push_back( TG_IPL_invalid );
	types.push_back( TG_IPT_invalid );

	int length = parse_function( *app, process_id, module_id, function_id,
			exps, locs, types, ipinfo );

	if( length < 0 ) return;

	map<string,IPInfoP>::iterator i;
	for( i = ipinfo.begin(); i != ipinfo.end(); ++i ) {
		IPInfoP p_info = i->second;

		// Send only info for this module and function id -JCG 9/1/04
                if ((p_info->function_id != function_id) ||
		    (p_info->module_id != module_id))
		    continue;

		pack_and_send_entry( prefixed_func_name, p_info->ip_tag.c_str(),
			p_info->line, p_info->type, p_info->loc,
			p_info->func_called.c_str(), p_info->call_index,
			p_info->description.c_str(), TG_dpcl_socket_out );

		map<string,DPCLActionTypeP>::iterator j;
		for( j = actionList.begin(); j != actionList.end(); ++j ) {
			pack_and_send_enable_action(
				prefixed_func_name,
				p_info->ip_tag.c_str(),
				j->first.c_str(),
				TG_dpcl_socket_out );
		}
	}

	// Tell Client that all entries for this function has been sent
	pack_and_send_function_parsed (prefixed_func_name, TG_dpcl_socket_out);
	TG_flush( TG_dpcl_socket_out );

	regfree( &exps[0] );
}

void start_program( Application * app )
{
	static bool hasrun;

	AisStatus status;
	if( hasrun ) {
//		status = app->bresume();
		status = app->bdetach();
	} else {
		status = app->bstart();
		hasrun = TRUE;
	}

	if( status != ASC_success ) {
		fprintf( stderr, "Couldn't start app: %s\n",
				status.status_name() );
	}
}

void stop_program( Application * app )
{

#ifdef TG_DEBUG_STOP
	Process p = app->get_process(0);
	ConnectState state;
	AisStatus retval = p.query_state(&state);
	if( retval != ASC_success ) {
		TG_timestamp("DPCL collector: Didn't get thread state\n");
	} else {
		printf("in stop: process state is %d\n", (int)(state) );
	}
#endif

//	AisStatus status = app->bsuspend();
	AisStatus status = app->battach();
	if( status != ASC_success ) {
		fprintf( stderr, "Couldn't stop app: %s\n",
				status.status_name() );
	}
}

#ifndef USE_READ_SOURCE_MODULE
void read_file( char * buf, int id )
{
	char * fileName;

	TG_unpack( buf, "S", &fileName );

	struct stat fileinfo;
	int retval = stat( fileName, &fileinfo );
	if( retval != 0 ) {
		TG_send( TG_dpcl_socket_out, DB_FILE_READ_COMPLETE, id,
				1, "" );
		TG_flush( TG_dpcl_socket_out );
//		fprintf( stderr, "Couldn't get info on file %s\n", fileName );
	} else if( (fileinfo.st_mode & S_IFREG) != S_IFREG ) {
		TG_send( TG_dpcl_socket_out, DB_FILE_READ_COMPLETE, id,
				1, "" );
		TG_flush( TG_dpcl_socket_out );
		fprintf( stderr, "%s is not a regular file\n", fileName );
	} else if( fileinfo.st_size <= 0 ) {
		TG_send( TG_dpcl_socket_out, DB_FILE_READ_COMPLETE, id,
				1, "" );
		TG_flush( TG_dpcl_socket_out );
		fprintf( stderr, "%s is an empty file\n", fileName );
	} else {
		char * wholeFile = new char[fileinfo.st_size + 1];
		if( wholeFile == NULL ) {
			TG_send( TG_dpcl_socket_out, DB_FILE_READ_COMPLETE, id,
					1, "" );
			TG_flush( TG_dpcl_socket_out );
			fprintf( stderr, "%s is too big to read: %d bytes\n",
					fileinfo.st_size );
		} else {
			int fd = open( fileName, O_RDONLY );
			if( fd < 0 ) {
				// send error message
				fprintf( stderr, "Failed to open %s\n",
						fileName );
			} else {
				retval = read( fd, wholeFile,
						fileinfo.st_size );
				// Client expects \0 termination
				wholeFile[fileinfo.st_size] = 0;
				close( fd );

				TG_send( TG_dpcl_socket_out,
						DB_FILE_READ_COMPLETE, id,
						fileinfo.st_size + 1,
						wholeFile );
				TG_flush( TG_dpcl_socket_out );

			}
			delete [] wholeFile;
		}
	}
}
#endif // USE_READ_SOURCE_MODULE

// What to do when the heartbeat timer expires
int heartbeat_handler( int signal )
{
	// Quit as gracefully as possible
	TG_timestamp("DPCL collector: Heartbeat timer expired\n");
	Ais_end_main_loop();
	return 0;		// return value is ignored
}

// Tells the collector to quit unless another heartbeat message is
// received within the specified interval (in seconds).  An interval
// of zero cancels the previous heartbeat request.
void unpack_heartbeat( char * buf )
{
	int interval;
	static int heartbeat_activated;

	TG_unpack( buf, "I", &interval );

	if( interval <= 0 ) {
		if( heartbeat_activated ) {
			// Disable the timer before removing the callback
			alarm( 0 );
			Ais_remove_signal( SIGALRM );
			heartbeat_activated = 0;
		}
	} else {
		// Create a callback before activating the timer
		if( !heartbeat_activated ) {
			Ais_add_signal( SIGALRM, heartbeat_handler );
		}
		alarm( interval );
		heartbeat_activated = 1;
	}
}

// Removes all activated actions, in preparation to shut down the program
void remove_all_actions( Application * app )
{
	// Stop the program, in case it's currently running
	// (Shouldn't be needed if we use attach/detach instead of
	// suspend/resume)
//	AisStatus status = app->bsuspend();

	// Find all finalization operations and execute them.
	list<DPCLActionTypeP>::iterator i;
	for( i = finalizationList.begin();
			i != finalizationList.end(); ++i ) {
		DPCLOneShotAction( TG_app, *i );
	}
}

void shutdown_app( void )
{
	if( TG_app != NULL ) {
		// App should be in attached or created state
		// before it's destroyed
		Process p = TG_app->get_process(0);
		ConnectState state;
		AisStatus retval = p.query_state(&state);
		if( retval != ASC_success ) {
			TG_timestamp("DPCL collector: "
					"Didn't get thread state\n");
		}

		// Nothing to do; the app is already shut down
		if( retval == ASC_terminated_pid ) {
			return;
		}

//		if( state != PRC_attached && state != PRC_created ) {
		// Need to be in attached state to terminate the program
		if( state != PRC_attached ) {
			TG_timestamp("DPCL collector: Attaching to app\n");
			retval = TG_app->battach();
			TG_timestamp("DPCL collector: Attached %s\n",
				retval.status_name());
		}

#ifdef TG_REMOVE_INSTRUMENTATION
		// Remove instrumentation that might interfere with quitting
		TG_timestamp("DPCL collector: Removing actions\n");
		remove_all_actions( TG_app );
		TG_timestamp("DPCL collector: Removed\n");
#endif

		// bdestroy isn't a virtual function for some reason, so
		// we have to call it with a type cast to get the app
		// destroyed correctly.
		TG_timestamp("DPCL collector: Destroying app\n");
		if( TG_is_poe_app ) {
//			retval = ((PoeAppl *)TG_app)->destroy( shutdown_cb,
//					NULL );
			((PoeAppl *)TG_app)->bdestroy();
			
		} else {
//			retval = TG_app->destroy( shutdown_cb, NULL );
			TG_app->bdestroy();
		}
		TG_timestamp("DPCL collector: Returned from destroy\n");
	}

	if( TG_process != NULL ) {
		// Process should be in attached state
		// before it's destroyed
		ConnectState state;
		AisStatus retval;
		TG_process->query_state(&state);
		if( state != PRC_attached ) {
			TG_process->battach();
		}
//		retval = TG_process->destroy( shutdown_cb, NULL );
		retval = TG_process->bdestroy();
	}

	// Acknowledge quitting to client
//	TG_timestamp("DPCL collector: Acknowledging quit\n");
	TG_send( TG_dpcl_socket_out, DPCL_SAYS_QUIT, 0, 0, NULL );
	TG_send( TG_dpcl_socket_out, DPCL_SAYS_QUIT, 0, 0, NULL );
	TG_flush( TG_dpcl_socket_out );
}

void instrument_app( Application * app, char * buf )
{
	char * actionTag;
	char * functionRE;
	TG_InstPtLocation ipl;
	TG_InstPtType ipt;
	int insert;
	int count;

	// First find out how many items we'll be getting
	char * p = TG_unpack( buf, "I", &count );

	vector<string> expressions;
	vector<TG_InstPtType> iptypes;
	vector<TG_InstPtLocation> iplocations;
	vector<DPCLActionTypeP> action_types;

	// Reserve space in each vector
	expressions.reserve( count );
	iptypes.reserve( count );
	iplocations.reserve( count );
	action_types.reserve( count );

	// Now read the data into the vectors
	int i;
	for( i = 0; i < count; i++ ) {
		p = TG_unpack( p, "SIIIS", &actionTag, &insert, &ipl, &ipt,
				&functionRE );

		expressions.push_back( functionRE );
		iptypes.push_back( ipt );
		iplocations.push_back( ipl );
		action_types.push_back( actionList[ actionTag ] );
	}

	// See if we are inserting or deleting instrumentation
	if( insert ) {
		InstrumentLocations( *app, expressions, iptypes,
				iplocations, action_types, ap_list );
	} else {
		// Nothing yet; will need to run through the list of
		// action points and remove items.
		fprintf( stderr, "Automaic removal of instrumentation "
				"not yet supported!\n" );
	}
}

// Callback sent to sourceReader so it can periodically see if the
// user has asked to cancel the search.  Returns nonzero if the
// search should continue.
int check_continue_search( void )
{
	int sock = sourceReader->getSocket();
	if( TG_poll_socket(sock) ) {
		int tag;
		if( (tag = dpcl_socket_handler( sock ))
				== COLLECTOR_CANCEL_READ_FILE ) {
			return FALSE;
		}
	}
	return TRUE;
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

