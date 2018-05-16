// parse_program.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 9 October 2000

#include <dpcl.h>
#include <stdio.h>
#include <stdlib.h>
#include "collector_pack.h"
#include "parse_program.h"
#include "tg_globals.h"
#include "tg_socket.h"

// #define MIN_FILE_PATH_LENGTH 256
// #define MAX_FILE_PREFIX_LENGTH 64
#define MAX_FUNC_NAME_LENGTH 1000
#define MAX_IP_DESCRIPTION_LENGTH 1000
#define IP_TAG_LENGTH 256
#define KEY_LENGTH 128
#define FILTER_MAX (1<<16)
#define EXCLUSION_FILE "parse_exclude"
#define EXCLUSION_VAR "TG_EXCLUSION_FILE"
#define INCLUSION_FILE "parse_include"
#define INCLUSION_VAR "TG_INCLUSION_FILE"

static void build_ip_description( InstPoint& ip, int line, 
				  TG_InstPtType& type, 
				  TG_InstPtLocation& location, 
				  char * funcCalled, int funcCalledLen,
				  int& callIndex, char * str, int max_str );
static void initialize_filter( const char * env_var, const char * file_name,
		char * list );
static char exclusion_list[FILTER_MAX] = "";
static char inclusion_list[FILTER_MAX] = "";

// Parse a parallel program using DPCL and store the information about it
// in the program database.
int parse_application( Application& application, ModuleInfo *& modules )
{
//	int num_processes = application.get_count();
//	int pnum;

	// Get each process from the application
	//
	// For MPMD program, only need to do this once!
//	for( pnum = 0; pnum < num_processes; pnum++ ) {
//		Process process = application.get_process(pnum);
//		if( parse_process( process, to_client_socket, pnum ) != 0 ) {
//			return -1;
//		}
//	}
	Process process = application.get_process(0);
	return parse_process( process, 0, modules );
}

// Parse a serial program using DPCL and store the information about it
// in the program database.
int parse_process( Process& process, int pnum, ModuleInfo *& modules )
{

	if( inclusion_list[0] == '\0' ) {
		initialize_filter( INCLUSION_VAR, INCLUSION_FILE,
				inclusion_list);
	}

	if( exclusion_list[0] == '\0' ) {
		initialize_filter( EXCLUSION_VAR, EXCLUSION_FILE,
				exclusion_list);
	}

	SourceObj process_source_obj = process.get_program_object();

	// Get each module (file) from the process
	int num_modules = process_source_obj.child_count();

	modules = new ModuleInfo[num_modules];
	int i, mnum = 0;
	for( i = 0; i < num_modules; i++ ) {
		SourceObj module = process_source_obj.child(i);

		// Get length of path
		int module_fp_length = module.module_name_length();

		// Read in file path after prefix
		char * file_path = new char[module_fp_length];
		module.module_name( file_path, module_fp_length );

		// If there is an inclusion list and this module
		// isn't on it, or the module appears on the
		// exclusion list, then we skip it.  (Thus, a
		// module appearing on both lists will be excluded;
		// if there is no inclusion list, all modules will
		// be included except those explicitly excluded.)
		if( (inclusion_list[0] != '\0'
				&& strstr( inclusion_list, file_path) == NULL )
			|| (strstr( exclusion_list, file_path ) != NULL ) ) {
			delete file_path;
			continue;
		}

		modules[mnum].module_id = i;
		modules[mnum].process_id = pnum;
		modules[mnum].file_path = file_path;
		mnum++;

	}

	return mnum;
}

int parse_module( Application& app, int process_id, int module_id, 
			FunctionInfo *& funcs )
{
	Process p = app.get_process( process_id );

	SourceObj process_source_obj = p.get_program_object();
	SourceObj module = process_source_obj.child( module_id );

	if( module.bexpand(p) != ASC_success ) {
		fprintf( stderr, "couldn't expand module %d\n", module_id );
		return -1;
	}

	// Get each function from the module
	int num_functions = module.child_count();

	funcs = new FunctionInfo[num_functions];
	int i, fnum = 0;	// Loop counter is separate from function
				// index because some functions won't
				// end up in the list.
	for( i = 0; i < num_functions; i++ ) {
		SourceObj function = module.child(fnum);

		int function_name_length =
			function.get_demangled_name_length();
		if( function_name_length <= 0 ) continue;

		funcs[fnum].start_line = function.line_start();
		funcs[fnum].end_line = function.line_end();
		funcs[fnum].function_id = i;
		funcs[fnum].func_name = new char[function_name_length];
		function.get_demangled_name( funcs[fnum].func_name,
			function_name_length );
		fnum++;
	}

	return fnum;
}

int parse_function( Application& app, int process_id, int module_id,
		int function_id, const vector<regex_t>& reg_exps,
		const vector<TG_InstPtLocation>& match_loc,
		const vector<TG_InstPtType>& match_type,
		map<string,IPInfoP>& inst_points )
{
	char ip_description[MAX_IP_DESCRIPTION_LENGTH];
	char funcCalled[MAX_FUNC_NAME_LENGTH];
	TG_InstPtType type = TG_IPT_invalid; // Initialized to debug code
	TG_InstPtLocation location = TG_IPL_invalid;
	int callIndex = -1; 
	char buf[IP_TAG_LENGTH];
	char this_func[MAX_FUNC_NAME_LENGTH];

	Process p = app.get_process( process_id );
	SourceObj process_source_obj = p.get_program_object();
	SourceObj module = process_source_obj.child( module_id );
	SourceObj function = module.child( function_id );

	// Create the prefixed function name and put it in the string
	// containing_func so we can store it with each IPInfo struct
	sprintf( this_func, "%d_%d_%d ",
			process_id, module_id, function_id );
	int prefix_len = strlen( this_func );
	function.get_demangled_name( this_func + prefix_len,
			MAX_FUNC_NAME_LENGTH - prefix_len );
	string containing_func( this_func );

	// Allocate the array of instrumentation points
	int num_points = function.inclusive_point_count();
	
	// Get each inst point in the function
	int ipfound, ipused = 0;
	for( ipfound = 0; ipfound < num_points; ipfound++ ) {
		InstPoint inst_point = function.inclusive_point(ipfound);
		int line = inst_point.get_line();

		build_ip_description( inst_point, line,
				type, location, 
				funcCalled, MAX_FUNC_NAME_LENGTH,
				callIndex, ip_description,
				MAX_IP_DESCRIPTION_LENGTH );
		sprintf( buf, "%d %d %d %d",
				process_id, module_id, function_id, ipfound );

		// For each pattern (reg exp, inst point type and location),
		// see if we have a match.  For function call inst points,
		// we match the name of the called function and the location
		// (before or after); for entry/exit inst points, we match
		// the containing function.  If the inst point type given
		// in the pattern is TG_IPT_invalid, then only the name
		// of the function (being called, or being entered/exited,
		// as the case may be) needs to match.
		vector<TG_InstPtLocation>::const_iterator ipl_it;
		vector<TG_InstPtType>::const_iterator ipt_it;
		vector<regex_t>::const_iterator re_it;
		int i;
		for( ipl_it = match_loc.begin(), ipt_it = match_type.begin(),
				re_it = reg_exps.begin(), i = 0;
				re_it != reg_exps.end();
				++ipl_it, ++ipt_it, ++re_it, ++i ) {
			if(  ( type == TG_IPT_function_call &&
			       ( ( *ipt_it == type && *ipl_it == location )
				 || *ipt_it == TG_IPT_invalid ) &&
			       regexec( &(*re_it), funcCalled, 0, NULL, 0 )
			      			 == 0 )
				||
			     ( ( type == TG_IPT_function_entry ||
				 type == TG_IPT_function_exit ) &&
			       ( *ipt_it == type ||
					 *ipt_it == TG_IPT_invalid ) &&
			       regexec( &(*re_it), this_func + prefix_len,
						 0, NULL, 0 ) == 0 ) ) {
				string ip_tag( buf );
				inst_points.insert( make_pair( ip_tag,
						       IPInfoP(
							new IPInfo(inst_point,
							type, location,
							containing_func, line,
							funcCalled, callIndex,
							ip_tag,
  						        ip_description, i,
							module_id, 
							function_id
							) ) ) );
				++ipused;
			} 
		}
	}

	return ipused;
}

// Following arrays rely on specific values of DPCL enumerated types.
static char * ipt_str[] = { "Invalid",
        "Entry to ", "Exit from ", "call to " };
static int ipt_str_len[] = { 7, 9, 10, 8 };
static char * ipl_str[] = { "", "Before ", "After " };
static int ipl_str_len[] = {0, 7, 6};
static TG_InstPtType tg_type[] = { TG_IPT_invalid, TG_IPT_function_entry,
				   TG_IPT_function_exit, 
				   TG_IPT_function_call };
static TG_InstPtLocation tg_location[] = {TG_IPL_invalid, TG_IPL_before,
					  TG_IPL_after};
#define LABEL_PREAMBLE_LENGTH 15

// In order to calculate callIndex properly, this routine assumes
// that this routine is called function call order, and that
// all the inst points for a function are called together.
// If this is not true, probably will get the wrong callIndex!
void build_ip_description( InstPoint& ip, int line, TG_InstPtType& type, 
			   TG_InstPtLocation& location, 
			   char * funcCalled, int funcCalledLen,
			   int &callIndex, char * str, int max_str )
{
        // Keep local state needed to calculate callIndex
        static int prevLine = -1;
	static int prevIndex = 0;
	InstPtType ipt = ip.get_type();

	// Convert DPCL type into TG_InstPtType with array lookup
	type = tg_type[ipt];

	// Initialize location, funcCalled, callIndex to default values
	// that IPT_function_call overrides
	location = TG_IPL_invalid;
	funcCalled[0] = 0;
	callIndex = -1;

	switch( ipt ) {
	case IPT_function_entry:
	case IPT_function_exit:
	{
		SourceObj container = ip.get_container();
		int container_name_length =
			container.get_demangled_name_length();

#if 0
		if( LABEL_PREAMBLE_LENGTH + container_name_length > max_str ) {
			max_str = LABEL_PREAMBLE_LENGTH + container_name_length;
			delete [] str;
			str = new char[max_str];
		}
#endif

		strcpy( str, ipt_str[ipt]);

		// Tack on the function name (avoids an extra copy and
		// possible allocation compared to getting string separately)
		container.get_demangled_name(str + ipt_str_len[ipt],
				max_str - ipt_str_len[ipt]);

		// Reset prevLine and prevIndex since at beginning/end of
		// function.  
		prevLine = -1;
		prevIndex = 0;
		break;
	}
	case IPT_function_call:
	{
		int func_name_length = ip.get_demangled_name_length();

#if 0
		if( LABEL_PREAMBLE_LENGTH + func_name_length > max_str ) {
			max_str = LABEL_PREAMBLE_LENGTH + func_name_length;
			delete [] str;
			str = new char[max_str];
		}
#endif

		InstPtLocation ipl = ip.get_location();
		sprintf( str, "%s%s", ipl_str[ipl], ipt_str[ipt]);

		// Tack on function name
		ip.get_demangled_name(str + ipt_str_len[ipt] + ipl_str_len[ipl],
				max_str - ipt_str_len[ipt] - ipl_str_len[ipl]);

		// Convert location to tool gear location thru array
		location = tg_location[ipl];

		// Get function call name instrumenting around
		ip.get_demangled_name (funcCalled, funcCalledLen);

		// If function call on new line, the callIndex must be 1
		if (line != prevLine)
		{
		    callIndex = 1;
		    prevLine = line;
		}
		// If before a new function call (on same line)
		// increment call index
		else if (ipl == IPL_before)
		{
		    callIndex = prevIndex + 1;
		}
		// If after a function call, use the index before
		else
		{
		    callIndex = prevIndex;
		}
		// Update prevIndex;
		prevIndex = callIndex;
		break;
	}
	default:
		sprintf( str, "Unknown Instrumenation Point type (%d)", ipt );
		break;
	}
}

InstPoint tag_to_inst_point( Process * p, char * tag )
{
	int module_num;
	int function_num;
	int inst_point_num;

	if (sscanf( tag, "%*d %d %d %d", &module_num, &function_num,
		    &inst_point_num ) != 4)
	{
	    fprintf (stderr, 
		     "tag_to_inst_point(process): Unexpected tag '%s'\n", tag);
	    exit (1);
	}

	SourceObj process_source_obj = p->get_program_object();
	SourceObj module = process_source_obj.child( module_num );
	SourceObj function = module.child( function_num );
	InstPoint ip = function.inclusive_point( inst_point_num );

	return ip;
}

InstPoint tag_to_inst_point( Application * a, char * tag )
{
	int proc_num;
	int module_num;
	int function_num;
	int inst_point_num;

	if (sscanf( tag, "%d %d %d %d", &proc_num, &module_num, &function_num,
			&inst_point_num ) != 4)
	{
	    fprintf (stderr, 
		     "tag_to_inst_point (application): Unexpected tag '%s'\n", 
		     tag);
	    exit (1);
	}

	Process p = a->get_process( proc_num );
	SourceObj process_source_obj = p.get_program_object();
	SourceObj module = process_source_obj.child( module_num );
	SourceObj function = module.child( function_num );
	InstPoint ip = function.inclusive_point( inst_point_num );

	return ip;
}

// Filter is a list of files to include or exclude.  The
// application can call this function repeatedly to create
// different kinds of filters as needed.
// This function looks first for an environment variable
// and then for a specific file name.
void initialize_filter( const char * env_var, const char * file_name,
		char * list )
{
	const char * filter_file = getenv( env_var );
	if( filter_file == NULL )
		filter_file = file_name;

	FILE * fp = fopen( filter_file, "r" );

	if( fp == NULL ) {
		// Not an error; there's just no filter file.
		// We don't want to keep checking, though, so
		// set the first character of the list to
		// \n to indicate there's nothing there.
		list[0] = '\0';
	} else {
		int bytes_read =
			fread( list, sizeof(char), FILTER_MAX - 1, fp );
		list[bytes_read] = 0;
		if( bytes_read == FILTER_MAX - 1 ) {
			fprintf( stderr,
				"Filter file %s contained too much data; "
				"only %d characters read.\n",
				filter_file, FILTER_MAX - 1 );
		}
		fclose( fp );
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

