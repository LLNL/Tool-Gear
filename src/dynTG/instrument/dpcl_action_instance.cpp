// dpcl_action_instance.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

// John May, 30 January 2002

#include <dpcl.h>
#include <iostream>
#include <stdlib.h>
#include <string>
using namespace std;

#include "dpcl_action_instance.h"
#include "dpcl_action_point.h"
#include "dpcl_action_type.h"





#define BUFFER_LENGTH (256)

// FIX Use an STL container (map indexed by name?) instead of this
struct ModuleListItem {
	string name;
	ProbeModule * module;
	ModuleListItem * next;
};

static ProbeExp get_probe_function( Application * app, string func_name,
		string module_name )
{
	static ModuleListItem * modules;
	static ProbeModule * probe_module;

	// Keep track of modules that have already been loaded 
	// for this app, so we don't load twice.
	ModuleListItem * thisItem;
	for( thisItem = modules; thisItem != 0; thisItem = thisItem->next ) {
		if( module_name == thisItem->name ) {
			probe_module = thisItem->module;
			break;
		}
	}


	// Not already loaded; try to get it
	if( thisItem == 0 ) {
#ifdef DEBUG_PROBE_MODULE
		cerr << "Looking for function " << func_name << " in module "
			<< module_name << endl;
#endif

		try {
			probe_module = new ProbeModule( module_name.c_str() );
		}
		catch(...) {
			cerr << "Failed to open module " << module_name << endl;
			exit(-1);
		}

		AisStatus status = app->bload_module(probe_module);
		if( status != ASC_success ) {
			cerr << "Error trying to load module " << module_name
				<< ": " << status.status_name() << endl;
			exit(-1);
		}

		// Put the module in our list so we don't load it again
		ModuleListItem * newitem = new ModuleListItem;
		newitem->name = module_name;
		newitem->module = probe_module;
		newitem->next = modules;
		modules = newitem;
	}

	// Now look for the function name in the module
	char buffer[BUFFER_LENGTH];
	ProbeExp probe_exp;
	int i;
	for( i = 0; i < probe_module->get_count(); i++ ) {
		probe_module->get_name( i, buffer, BUFFER_LENGTH );
		if( func_name == buffer ) {
			probe_exp = probe_module->get_reference( i );
			break;
		}
	}

	// Failed to find the probe function; give up!
	if( i == probe_module->get_count() ) {
		cerr << "Failed to find the function " << func_name <<
			" in the module " << module_name << endl;

		exit(-1);
	}

	return probe_exp;
}

/*! This version creates a point probe */
DPCLPointAction:: DPCLPointAction( DPCLActionPointP own,
		DPCLActionTypeP action_type, int pid )
	: Action( action_type->get_tag() ),
	type(action_type), owner( own ), process_id( pid )
{
	if( !owner || !action_type ) {
		cerr << "Invalid input data for DPCLPointAction" << endl;
		return;
	}

	app = own->get_application();

	// Get the handle to the probe function in the target
	ProbeExp func;
	if( !(type->probe_function_is_set()) ) {
		func = get_probe_function( app, type->get_func_name(),
					type->get_module_name() );
		type->set_probe_function( func );
	} else {
		func = type->get_probe_function();
	}

	// Create the expression that represents a call to the probe
	// function, using a callback if necessary
	GCBFuncType callback = type->get_callback();
	ProbeExp func_call;
	GCBTagType tag;
	ProbeExp function_arg = Ais_msg_handle;
	if( callback != NULL ) {
		func_call = func.call( 1, &function_arg );
		tag = this;

	} else {
		func_call = func.call( 0, NULL );
		tag = 0;
	}

	InstPoint ip = own->get_inst_point()->ip;

	AisStatus result;

	// Working on a single process?
	if( process_id >= 0 && process_id < app->get_count() ) {
		Process p = app->get_process( process_id );

		if( ( result = p.binstall_probe(1, &func_call, &ip, &callback,
						&tag, &probe_handle ) )
					!= ASC_success ) {
			cerr << "Failed to install probe for "
				<< type->get_tag() << ": "
				<< result.status_name() << endl;
			return;
		}

		if( ( result = p.bactivate_probe( 1, &probe_handle ) )
				!= ASC_success ) {
			cerr << "Failed to activate probe for "
				<< type->get_tag() << ": "
				<< result.status_name() << endl;
			return;
		}
	} else {	// Whole application
		if( ( result = app->binstall_probe(1, &func_call, &ip,
						&callback, &tag,
						&probe_handle ) )
					!= ASC_success ) {
			cerr << "Failed to install probe for "
				<< type->get_tag() << ": "
				<< result.status_name() << endl;
			return;
		}

		if( ( result = app->bactivate_probe( 1, &probe_handle ) )
				!= ASC_success ) {
			cerr << "Failed to activate probe for "
				<< type->get_tag() << ": "
				<< result.status_name() << endl;
			return;
		}
	}
}

// Removes the probe from the target program
DPCLPointAction:: ~DPCLPointAction()
{
	// Remove probe for a single process
	AisStatus result;
	if( process_id >=0 && process_id < app->get_count() ) {
		Process p = app->get_process( process_id );
		if( (result = p.bremove_probe( 1, &probe_handle ))
			       	!= ASC_success ) {
			cerr << "Failed to removed probe "
				<< type->get_tag() << ": "
				<< result.status_name() << endl;
		}
	} else {	// Remove probe for whole app
		if( (result = app->bremove_probe( 1, &probe_handle ))
			       	!= ASC_success ) {
			cerr << "Failed to removed probe "
				<< type->get_tag() << ": "
				<< result.status_name() << endl;
		}
	}
}

/*! Causes a probe to execute once, and immediately, in the
// target application.  If the probe uses a callback, a
// pointer to this object will be passed to it.  The user
// must ensure either that this pointer is still valid
// (i.e., this object still exists) or that the pointer
// isn't used */
DPCLOneShotAction:: DPCLOneShotAction( Application * app,
		DPCLActionTypeP type, int process_id )
{
	if( app == NULL || !type ) {
		cerr << "Invalid input data for DPCLOneShotAction" << endl;
		return;
	}

	// Get the handle to the probe function in the target
	ProbeExp func;
	if( !(type->probe_function_is_set()) ) {
		func = get_probe_function( app, type->get_func_name(),
					type->get_module_name() );
		type->set_probe_function( func );
	} else {
		func = type->get_probe_function();
	}

	// Create the expression that represents a call to the probe
	// function, using a callback if necessary
	GCBFuncType callback = type->get_callback();
	ProbeExp func_call;
	GCBTagType tag;
	ProbeExp function_arg = Ais_msg_handle;
	if( callback != NULL ) {
		func_call = func.call( 1, &function_arg );
		tag = this;

	} else {
		func_call = func.call( 0, NULL );
		tag = 0;
	}

	AisStatus result;
	// One process
	if( process_id >= 0 && process_id < app->get_count() ) {
		
		Process p = app->get_process( process_id );
		if( ( result = p.bexecute(func_call, callback,
					tag ) ) != ASC_success ) {
			cerr << "Failed to execute probe for "
				<< type->get_tag() << ": "
				<< result.status_name() << endl;
			return;
		}
	} else {	// Whole application
		if( ( result = app->bexecute(func_call, callback,
					tag ) ) != ASC_success ) {
			cerr << "Failed to execute probe for "
				<< type->get_tag() << ": "
				<< result.status_name() << endl;
			return;
		}
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

