// timer_actions.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 8 Feb 2002

/*--- Standard stuff ---*/

#include <dpcl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <utility>
using std::make_pair;
#include <list>
using std::list;
#include <map>
using std::map;
#include <string>
using std::string;

#include <base.h>
#include "timermodule.h"

/*----------------------------------------------------------------------*/
/* Constants - defining target probe */

#define PMAPI_PROBE   "mod_timer"
#define PMAPI_TAG     "timer"

#define START_TIMER_TAG "startTimer"
#define STOP_TIMER_TAG "stopTimer"
#define TIME_COLUMN "time"


/*---------------------------------------------------------------------*/
/* Prototype for callback */

extern "C"
{
// Declaration of the callback function
  void StopTime_cb( GCBSysType sys, GCBTagType tag,
		    GCBObjType obj, GCBMsgType msg );
}


/*---------------------------------------------------------------------*/
/* report the tool's needs */

int GetToolNeeds(toolCapabilities_t *cap)
{
  cap->useDPCL=1;
  strcpy(cap->shortname,"TIMER");
  strcpy(cap->name,"Timing routines");
  return 0;
}


/*---------------------------------------------------------------------*/
/* init the action -> called during startup */

int InitializeDPCLTool( int sock, void *handle)
{
  char *module_name;
  int  i,numcount,err;
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

  sprintf(fullname,"%s/%s",module_name,PMAPI_PROBE);

  // Check that the file exists and is readable.
  if (access(module_name, R_OK | X_OK )!=0)
    {
      fprintf(stderr,
	      "Error accessing probe module %s: %d\n",
	      module_name, errno);
      return -1;
    }

  // Define the start and stop timer action types and tell the Client
	
  AddActionNoCB(START_TIMER_TAG, fullname, "StartTimer", sock );

  pack_and_send_action_attr( START_TIMER_TAG, "Start timer",
			     "Start execution timer", sock );

  AddAction(STOP_TIMER_TAG, fullname, "StopTimer", sock,
	    "StopTime_cb", handle);

  pack_and_send_action_attr( STOP_TIMER_TAG, "Stop timer",
			     "Stop execution timer and report result", sock );


 #if NOT_YET_DONE
  // needs to fixed to dynamically link using C++ name and
  // to support multiple info entries
  pack_and_send_info_about_tool(
			"Timer Tool: Allows timing of user-selected\n"
			"regions of code.  Written by John May and\n"
			"John Gyllenhaal.\n\n", sock );
#endif

  // Declare columns in the client's display where data
  // will be shown.  Parameters are: tag for the column,
  // column label, description of data, data type, and
  // socket.
  pack_and_send_data_attr( TIME_COLUMN, "Time (sec)",
			   "Execution time",
			   TG_DOUBLE, sock );

  err=AddInit("initialize", fullname, "InitTimer", sock);
  if (err<0)
    {
      fprintf(stderr,
	      "Error setting an init callback %s: %d\n",
	      module_name, errno);
      return -1;
    }

  // Data isn't sent to the Client until this function
  // is called.
  TG_flush( sock );
  
  return 0;
}

/*---------------------------------------------------------------------*/
/* callback for probe */

void StopTime_cb( GCBSysType sys, GCBTagType tag,
                        GCBObjType obj, GCBMsgType msg )
{
	// msg contains the result data
	struct timerresult * result = (struct timerresult *)msg;
        Process * p = (Process *) obj;  // DPCL Process
        DPCLPointAction * action_instance =
                (DPCLPointAction *) tag;
	DPCLActionPointP owner = action_instance->get_owner();
        int task = p->get_task();       // parallel process id
        int sock = action_instance->get_type()->get_socket();

        // Send the timer data to the Client.  Parameters are:
        // target program function name, identifier for the
        // instrumentation point (so the Client knows where in
        // the source code to display the data), name of the
        // column where data will appear, task id, thread id
        // (unused here), data value, and socket.
        pack_and_send_add_double( owner->get_inst_point()->in_function.c_str(),
                owner->get_tag().c_str(), TIME_COLUMN, task, result->thread,
                result->interval, sock );

        TG_flush( sock );

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

