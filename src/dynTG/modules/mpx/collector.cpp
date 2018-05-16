// mpx_actions.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 1 Feburary 2002

// This file initializes a set of DPCL actions and defines
// the necessary callbacks for the MPX tool.  It is an
// example of what a tool builder needs to define (at
// the collector end) to create a new tool using Tool
// Gear's DPCL collector.

#include <dpcl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <utility>
using std::make_pair;

#include <base.h>
#include "cacheinst.h"


// This function knows about all the action types that the MPX
// tool will use, and it initializes a DPCLActionType object for
// each one.  It sets an array of these objects and a count
// of them.  The return value is zero if the initialization
// succeeds, nonzero otherwise.  Eventually, this kind of
// initialization could be put in a dynamic library that the
// the DPCL collector would load automatically and search for
// an appropriate initialization function.  Then it wouldn't
// be necessary to recompile the collector for new tools. 
// However, for tool developers, recompiling might be easier
// to do (portably) than creating dynamic libraries.
// Enough philosophy... on with the code!


/*----------------------------------------------------------------------*/
/* Constants - defining target probe */

#define MPX_PROBE "mod_mpx"

#define START_COUNTERS_TAG "startCounters"
#define STOP_COUNTERS_TAG "stopCounters"
#define HALT_TARGET_TAG "haltTarget"

#define FLOPS_COLUMN "flops"
#define FMA_COLUMN "fma"
#define FP_INS_COLUMN "fp_ins"
#define L1_UTIL_COLUMN "l1util"
#define FLOPRATE_COLUMN "FLOP/sec"

/*---------------------------------------------------------------------*/
/* Prototypes for callback */

extern "C"
{
  void StopCount_cb( GCBSysType sys, GCBTagType tag, GCBObjType obj,
		     GCBMsgType msg );
  void HaltTarget_cb( GCBSysType sys, GCBTagType tag, GCBObjType obj,
		      GCBMsgType msg );
}

/*---------------------------------------------------------------------*/
/* report the tool's needs */

int GetToolNeeds(toolCapabilities_t *cap)
{
  cap->useDPCL=1;
  strcpy(cap->shortname,"MPX");
  strcpy(cap->name,"PAPI-based L1 counters");
  return 0;
}


/*---------------------------------------------------------------------*/
/* init the action -> called during startup */

int InitializeDPCLTool( int sock, void *handle)
{

  char *module_name;
  int  i,err;
  char fullname[500];

  // Find the probe module; look in the environment
  // variable, then try a hard-coded location.

  module_name=getenv( "TG_MODULELIBRARY" );
  if (module_name==NULL)
    {
      fprintf(stderr,
	      "Error accessing probe module library %d\n",
	       errno);
      return -1;
    }

  sprintf(fullname,"%s/%s",module_name,MPX_PROBE);

  // Check that the file exists and is readable.
  if (access(module_name, R_OK | X_OK )!=0)
    {
      fprintf(stderr,
	      "Error accessing probe module %s: %d\n",
	      module_name, errno);
      return -1;
    }


  // Define each of the action types and tell the Client about them

  AddActionNoCB(START_COUNTERS_TAG, fullname, "StartCachePerf", sock);

  pack_and_send_action_attr( START_COUNTERS_TAG, "Start count",
			     "Start cache and FLOP measurements", sock );

  AddAction(STOP_COUNTERS_TAG, fullname, "StopCachePerf", sock,	"StopCount_cb", handle);

  pack_and_send_action_attr( STOP_COUNTERS_TAG, "Stop count",
			     "Stop cache and FLOP measurements", sock );

  // Declare attrs where data will be stored.
  pack_and_send_data_attr( L1_UTIL_COLUMN, "L1 util",
			   "Percentage of data load requests satified from "
			   "level 1 cache", TG_DOUBLE, sock );
  pack_and_send_data_attr( FP_INS_COLUMN, "FP Ins",
			   "Floating point instructions completed", TG_INT, sock );
  pack_and_send_data_attr( FMA_COLUMN, "FMAs",
			   "Floating point multiply-adds completed",
			   TG_INT, sock );
  pack_and_send_data_attr( FLOPS_COLUMN, "Total flop",
			   "Total floating point operations completed",
			   TG_INT, sock );
  
  pack_and_send_data_attr( FLOPRATE_COLUMN, "FLOP/sec",
			   "Floating point operations per second", TG_DOUBLE,
			   sock );

  AddAction(HALT_TARGET_TAG, fullname, "HaltTarget", sock, "HaltTarget_cb", handle);

  pack_and_send_action_attr( HALT_TARGET_TAG, "Set breakpoint",
			     "Set a breakpoint", sock );

#if NOT_YET_DONE
  // needs to fixed to dynamically link using C++ name and
  // to support multiple info entries
  pack_and_send_info_about_tool(
				"MPX Tool: Gathers cache utilization and FLOP rate\n"
				"data for a target program using the hardware \n"
				"performance counters.  Written by John May and\n"
				"John Gyllenhaal.\n\n", sock );
#endif

	// Tell the Client to display the viewer
	
//	pack_and_send_create_viewer( sock );

	TG_flush(sock);

	// This one isn't accessible from the Client.  The special tag
	// "finalize" tells the DPCL collector to instantatiate this
	// action as a one-shot probe when shutting down.  (If initialization
	// is necessary, it could be done here in this function, or maybe
	// we could add similar functionality to the DPCL collector.)
//	actionList[actionCount++] = new DPCLActionType( "finalize",
//				cpmodule, "FinalizeCachePerf", sock );

	// Add initialization and finalization actions to their special
	// lists.  Each list will be traversed in order.  We the
	// initializations at the end of the list and finalization at
	// the beginning (so they will be executed in the reverse order
	// of initialization).
	
	// This one will be executed after the application starts up
	// Initializing PAPI takes about half a second, and we want that
	// to get that done right away.

	AddInit("initialize", fullname, "InitializeCachePerf", sock );

	// This tells the application to exit.  It's a brute force
	// solution to a problem that occurs in parallel programs run
	// by DPCL.  Some process don't always exit cleanly.  This
	// ensures that they do.
	
	AddFinalize("finalize", fullname, "FinalizeCachePerf", sock );

	return 0;
}


/*---------------------------------------------------------------------*/
/* callbacks for probes */

// Receive counter data from the target application (through DPCL)
// and forward it to the Client
void StopCount_cb( GCBSysType sys, GCBTagType tag, GCBObjType obj,
		GCBMsgType msg )
{
	CacheData * data = (CacheData *)msg;
	Process * p = (Process *)obj;
	DPCLPointAction * action_instance = (DPCLPointAction *)tag;
	DPCLActionPointP owner = action_instance->get_owner();
	int task = p->get_task();
	int sock = action_instance->get_type()->get_socket();

	pack_and_send_double( owner->get_inst_point()->in_function.c_str(),
			owner->get_tag().c_str(),
			L1_UTIL_COLUMN, task, data->thread,
			data->l1utilization, sock );

	pack_and_send_add_int( owner->get_inst_point()->in_function.c_str(),
			owner->get_tag().c_str(),
			FP_INS_COLUMN, task, data->thread,
			data->flops, sock );

	pack_and_send_add_int( owner->get_inst_point()->in_function.c_str(),
			owner->get_tag().c_str(),
			FMA_COLUMN, task, data->thread,
			data->fma, sock );

	pack_and_send_add_int( owner->get_inst_point()->in_function.c_str(),
			owner->get_tag().c_str(),
			FLOPS_COLUMN, task, data->thread,
			data->flops + data->fma, sock );

	pack_and_send_double( owner->get_inst_point()->in_function.c_str(),
			owner->get_tag().c_str(),
			FLOPRATE_COLUMN, task, data->thread,
			(double)data->floprate, sock );

	TG_flush( sock );
}

// Implement a barrier-style breakpoint by letting the program run
// until all processes in the parallel program have checked in.
// This assumes that the breakpoint is reachable by all processes.
// We stop each target process as it hits the breakpoint.  If the
// user tries to stop the application directly before all processes
// have checked in, then the ones that haven't reached the breakpoint
// will hit it when the program is restarted.  This could create
// a deadlock, since some of the processes will have already reached
// breakpoint.  Also, there is no guarantee that all the
// processes hit the same breakpoint, although we could probably
// implement that if necessary by keeping the process count in the
// action point instead of making it a static variable.
void HaltTarget_cb( GCBSysType sys, GCBTagType tag, GCBObjType obj,
		GCBMsgType msg )
{
	DPCLPointAction * action_instance = (DPCLPointAction *)tag;
	DPCLActionPointP owner = action_instance->get_owner();
	Process * p = (Process *)obj;

	// p->bsuspend();
	p->battach();
	
	int sock = action_instance->get_type()->get_socket();
	int task = p->get_task();
	static int process_count;
	process_count += 1;
	Application * app = owner->get_application();

	if( process_count == app->get_count() ) {
		pack_and_send_target_halted(
				owner->get_inst_point()->in_function.c_str(),
				owner->get_tag().c_str(), task, sock );
		TG_flush( sock );
		process_count = 0;
	}
}


/*---------------------------------------------------------------------*/
/* The End. */


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

