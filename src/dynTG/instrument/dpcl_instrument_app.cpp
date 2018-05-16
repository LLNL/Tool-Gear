// dpcl_instrument_app.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 3 September 2002
// Installs DPCL probes (represented as DPCLPointAction objects)
// at a set of instrumentation points.

#include <dpcl.h>
#include <regex.h>
#include <vector>
using std::vector;
#include <map>
using std::map;
#include <string>
using std::string;
#include <stdio.h>
#include <utility>
using std::pair;
#include <iostream>
using std::cout;
#include "dpcl_instrument_app.h"
#include "dpcl_action_type.h"
#include "dpcl_action_point.h"
#include "dpcl_action_instance.h"
#include "collector_pack.h"
#include "parse_program.h"
#include "tg_inst_point.h"
#include "tg_globals.h"

#define COUNT_INTERVAL (10)

#include "tg_time.h"

//void ApplyInstrumentation::operator() ( pair<string,IPInfoP> p_info,
//		map<string,DPCLActionPointP>& action_points )
void ApplyInstrumentation:: operator() ( pair<string,IPInfoP> ippair )
{
	// Index refers to which pattern was matched when this
	// instrumentation point was found.  It identifies the
	// corresponding action type to apply.
	int at_index = ippair.second->index;

	// Get the action point that matches the inst point
	// Finds the action point, or the location where the action
	// point should go in this collection.
	map<string,DPCLActionPointP>::iterator ap_iter 
		= action_points.lower_bound( ippair.second->ip_tag );
	DPCLActionPointP ap;
	// If we didn't find a match, create & insert a new action point
	if( ap_iter == action_points.end()
			|| ap_iter->first != ippair.second->ip_tag ) {
		ap = DPCLActionPointP(
			new DPCLActionPoint( ippair.second->ip_tag,
				app, ippair.second ) );
		action_points.insert( ap_iter,
				make_pair( ippair.second->ip_tag, ap ) );
	} else {
		// Found a match; get the action point
		ap = ap_iter->second;

		// See if this action point already has the same action
		// attached to it.  This could happen, for example, if 
		// we use both automatic and manual instrumentation, and
		// it would be hard for the user to detect or undo.
		for( int i = 0; i < ap->get_num_actions(); ++i ) {
			Action * act = ap->get_action( i );
			if( act != NULL && act->get_text()
					== action_types[at_index]->get_tag() ) {
				cout << "Skipping duplicate action "
					<< action_types[at_index]->get_tag()
					<< " in "  <<
					ippair.second->description << std::endl;
				return;
			}
		}
	}

//	cout << "Applying " << action_types[at_index]->get_tag() << " to " << 
		//ippair.second->description << std::endl;

	ap->add_action( new DPCLPointAction( ap, action_types[at_index],
				process_id ) );
	static int counter;
	if( (++counter % COUNT_INTERVAL) == 0 ) {
		cout << counter << "...";
	}
//	TG_timestamp("Done applying %s to %s\n",
			//action_types[at_index]->get_tag(),
			//ippair.second->description );

}

void ReportInstPoint:: operator() ( pair<string,IPInfoP> ippair ) const
{

	IPInfoP p_info = ippair.second;
	//TG_timestamp("Reporting %s to %s\n", p_info->ip_tag.c_str() );
	pack_and_send_entry( p_info->in_function.c_str(),
			p_info->ip_tag.c_str(),
			p_info->line, p_info->type, p_info->loc,
			p_info->func_called.c_str(), p_info->call_index,
			p_info->description.c_str(), socket );
	//TG_timestamp("Done reporting %s to %s\n", p_info->ip_tag.c_str() );
}

#define MAX_PREFIX_NAME (1000)
void InstrumentLocations( Application& app,
			 const vector<string>& expressions,
			 const vector<TG_InstPtType>& iptype,
			 const vector<TG_InstPtLocation>& iploc,
			 vector<DPCLActionTypeP>& action_types,
			 map<string, DPCLActionPointP>& action_points )
{
	
	// Make sure all the vectors are at least as long as the
	// first one.
	if( ( expressions.size() > iptype.size() ) 
			|| ( expressions.size() > iploc.size() )
			|| ( expressions.size() > action_types.size() )
			|| ( expressions.size() == 0 ) ) {
		fprintf( stderr,
			"Instrument Locations: mismatched vector sizes" );
		return;
	}


	// All matching IPs will be accumulated here
	map<string,IPInfoP> ips;

	//TG_timestamp("compiling Reg exps\n");
	// Compile the regular expressions
	vector<regex_t> reg_exps( expressions.size() );
	vector<string>::const_iterator exp_it;
	int i;
	for( exp_it = expressions.begin(), i = 0;
			exp_it != expressions.end();
			++exp_it, ++i ) {

		int status = regcomp( &reg_exps[i], exp_it->c_str(),
				REG_NOSUB | REG_EXTENDED );
		if( status != 0 ) {
			fprintf( stderr, "Instrument Locations: failed to "
					"compile regular expression %s\n",
					exp_it->c_str() );
			return;
		}
	}

	//TG_timestamp("calling parse_application\n");
	// Run through all the instrumentation points in the
	// application and look for matches.
	ModuleInfo * modules;
	int mod_count = parse_application( app, modules );
	if( mod_count < 0 ) {
		fprintf( stderr, "Instrument Locations: failed "
				"to parse the application\n" );
				return;
	}

	//TG_timestamp("starting to parse modules\n");
	cout << "Looking for matching instrumentation points..." << std::endl;
	vector<string> parsed_funcs;
	for( int mod_num = 0; mod_num < mod_count; mod_num++ ) {

		FunctionInfo * functions;

	//TG_timestamp("parsing module %d\n", mod_num);
		int func_count = parse_module( app,
				modules[mod_num].process_id,
				modules[mod_num].module_id, functions );

		if( func_count < 0 ) {
			fprintf( stderr, "Instrument Locations: failed "
					"to parse the module\n" );
			continue;
		}

		char prefixed_file_name[MAX_PREFIX_NAME];
		snprintf( prefixed_file_name, MAX_PREFIX_NAME,
					"%d_%d %s", 
					modules[mod_num].process_id,
					modules[mod_num].module_id,
					modules[mod_num].file_path );

		for( int func_num = 0; func_num < func_count;
				func_num++ ) {
	//TG_timestamp("parsing function %d in module %d\n", func_num, mod_num);
			int ip_count = parse_function( app,
					modules[mod_num].process_id,
					modules[mod_num].module_id,
					functions[func_num].function_id,
					reg_exps, iploc, iptype,
					ips );
			if( ip_count < 0 ) {
				fprintf( stderr,
					"Instrument Locations: failed "
					"to parse the function\n" );
				continue;
			}
			
			// Report the functions we see as we parse
			// the whole progam
			char prefixed_func_name[MAX_PREFIX_NAME];
			snprintf( prefixed_func_name, MAX_PREFIX_NAME,
					"%d_%d_%d %s",
					modules[mod_num].process_id,
					modules[mod_num].module_id,
					functions[func_num].function_id,
					functions[func_num].func_name );

	//TG_timestamp("sending %s\n", prefixed_func_name);
			pack_and_send_function( prefixed_func_name,
					prefixed_file_name,
					functions[func_num].start_line,
					functions[func_num].end_line,
					TG_dpcl_socket_out );
			//TG_timestamp("sent\n");

			// Keep track of the functions declared
			parsed_funcs.push_back( prefixed_func_name );
			//TG_timestamp("adding function to list\n");
		}

		//TG_timestamp("done parsing function\n");
		delete [] functions;
	}
	//TG_timestamp("done parsing modules\n");

	delete [] modules;

	// At this point, the map ips should contain a list of all
	// the Instrumentation Points that matched the pattern and type.
	// So we create a function object that will apply the
	// appropriate instrumentation (action type) to each of the
	// instrumentation points.
	cout << "Applying instrumentation..." << std::endl;
	ApplyInstrumentation apply( action_types, app,
			action_points );

	for_each( ips.begin(), ips.end(), apply );

	if( ips.size() % COUNT_INTERVAL ) cout << ips.size();
	
	// Report all the instrumentation points to the client
	for_each( ips.begin(), ips.end(),
			ReportInstPoint( TG_dpcl_socket_out ) );

	cout << std::endl << "Finished with automatic instrumentation"
		<< std::endl;
	vector<regex_t>::iterator re_it;
	for( re_it = reg_exps.begin(); re_it != reg_exps.end();
			++re_it ) {
		regfree( &(*re_it) );
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

