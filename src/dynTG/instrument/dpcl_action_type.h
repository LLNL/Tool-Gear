//! \file dpcl_action_type.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

// John May, 30 January 2002

#ifndef DPCL_ACTION_TYPE_H
#define DPCL_ACTION_TYPE_H
#include <boost/shared_ptr.hpp>
#include <dpcl.h>
#include <string>
using std::string;

//! Defines Action Types that can be installed and activated
//! through DPCL.  Unlike Actions, Action Types have not
//! been instatiated in the target code; Actions are Action
//! Types that have been instantiated at a point in the code
//! (an Action Point) or as a one-shot or phase probe.
class DPCLActionType {
public:
	//! Define an action type and optionally supply a callback
	//! function in the Collector to handle information received
	//! from the target program.  Copies are made of all input
	//! strings.

	DPCLActionType( string tag,		//!< Action type name
			string module_name,	//!< File containing executable
						//!< code that will be loaded
						//!< dynamically into the target
			string func_name,	//!< Name of the function in
						//!< the module that will
						//!< execute in the target when
						//!< this action executes
			int sock,		//!< Used by callback to send
						//!< data to the Client (this
						//!< is an ugly interface!)
			GCBFuncType callback = NULL
						//!< Optional callback to be
						//!< instantiated when data
						//!< arrives from the target
						//!< program.
			);

	//! Returns name of module containing executable code for actions
	string get_module_name() { return module_name; }

	//! Returns name of the function corresponding to this action
	string get_func_name() { return func_name; }

	//! Returns text name of this action type
	string get_tag() { return tag; }

	//! Returns socket to be used for communicating with the Client
	int get_socket() { return sock; }

	//! Returns pointer to callback function, or NULL if none registered
	GCBFuncType get_callback() { return callback; }

	//! Returns the DPCL probe expression corresponding to this action;
	//! result is undefined if probe_function_is_set() returns FALSE.
	ProbeExp get_probe_function() { return probe_exp; }

	//! Returns true if a DPCL probe expression has been created for
	//! this action.
	bool probe_function_is_set() { return probe_exp_set; }

	//! Assigns a DPCL probe expression for this action
	void set_probe_function( ProbeExp& pe )
	{	probe_exp = pe;
		probe_exp_set = TRUE;
	}

protected:

	string module_name;		// probe module file name
	string func_name;		// name of the probe function to call
	string tag;
	GCBFuncType callback;
	ProbeExp probe_exp;		// reference to the function in the
					// probe module; will be set after
					// the application is loaded
	bool probe_exp_set;		// states whether probe_exp contains
					// a valid value; there isn't a
	 				// way to initialize probe_exp with
	 				// a legal null value, that I know of
	int sock;			// socket on which results are reported
};

//! Smart pointer that allows these objects to be used in STL containers.
typedef boost::shared_ptr<DPCLActionType> DPCLActionTypeP;


#endif // DPCL_ACTION_TYPE_H


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

