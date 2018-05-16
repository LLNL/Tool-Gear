//! \file tg_globals.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
//! All the nasty stuff that we haven't figured out how to encapsulate!
// John May, 3 November 2000

#ifndef TG_GLOBALS_H
#define TG_GLOBALS_H

#include <map>
using std::map;
#include <list>
using std::list;
#include <string>
using std::string;
#include <dpcl.h>
#include "dpcl_action_type.h"
#include "parse_program.h"	// Needed for IPInfoP type
#include "dpcl_action_instance.h"	// Needed for DCPLPointActionP

//! Pointer to DPCL representation of the target application (for
//! parallel jobs.)
extern Application * TG_app;

//! Pointer to DPCL representation of the target process (for
//! serial jobs.)
extern Process * TG_process;

//! Socket for communicating with Client
extern int TG_dpcl_socket_out;

//! List of environment variables.
extern char ** TG_envp;

//! List of command line arguments.
extern char ** TG_argv;

//! Number of command line arguments.
extern int TG_argc;

//! Indicates whether application is running as a poe job (nonzero means yes.)
extern int TG_is_poe_app;

//! List of DPCLActionType objects that have been registered with Tool Gear.
extern map<string,DPCLActionTypeP> actionList;

//! List of initialization actions
extern list<DPCLActionTypeP> initializationList;

//! List of initialization actions
extern list<DPCLActionTypeP> finalizationList;

//! Full path for target program.
extern char * TG_program_path;

//! Associative array that maps instrumentation point tags (four digits
//! separated by spaces) to full details about the instrumenation point
//! (IPInfoP objects)
extern map<string,IPInfoP> ipinfo;

//! Keeps track of all the action points that the user has activated.
extern map<string,DPCLActionPointP> ap_list;

//! Whether the tool wants DPCL to get structure information on
//! the target program when the program is loaded.  A nonzero
//! value means parsing won't be done.  This variable should be
//! set in the tool's initialization function.  As a global
//! variable, it defaults to zero, which means the program is
//! parsed unless the tool requests otherwise.
extern bool dont_parse;

#endif // TG_GLOBALS_H
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

