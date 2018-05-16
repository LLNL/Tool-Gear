/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
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

/*----------------------------------------------------------------------*/
/* Constants - defining target probe */

#define COUNTER_PROBE "mod_counter"
#define COUNTER_MODULE "counter_mod"
#define COUNTER_TAG "counter"
#define COUNTER_COLUMN_LOCAL "loc_count"
#define COUNTER_COLUMN_GLOBAL "glob_count"


/*---------------------------------------------------------------------*/
/* Prototype for callback */

extern "C"
{
  void CounterProbe_cb(GCBSysType sys, GCBTagType tag,
		       GCBObjType obj, GCBMsgType msg);
}


/*---------------------------------------------------------------------*/
/* Hash mechansim for counter buckets */

std::map <void*,int> counter_table;


/*---------------------------------------------------------------------*/
/* report the tool's needs */

int GetToolNeeds(toolCapabilities_t *cap)
{
  cap->useDPCL=1;
  strcpy(cap->shortname,"COUNTER");
  strcpy(cap->name,"Global counter");
  return 0;
}


/*---------------------------------------------------------------------*/
/* init the action -> called during startup */

int InitializeDPCLTool( int sock, void *handle)
{
  char * module_name;
  char fullname[500];
  int i,err;

  // Find the probe module; look in the environment

  module_name=getenv( "TG_MODULELIBRARY" );
  if (module_name==NULL)
    {
      fprintf(stderr,
	      "Error accessing probe module library %d\n",
	       errno);
      return -1;
    }

  sprintf(fullname,"%s/%s",module_name,COUNTER_PROBE);

  // Check that the file exists and is readable.
  if (access(module_name, R_OK | X_OK )!=0)
    {
      fprintf(stderr,
	      "Error accessing probe module %s: %d\n",
	      module_name, errno);
      return -1;
    }

  err=AddAction(COUNTER_TAG,fullname,"counter_inc",
		sock,"CounterProbe_cb",handle);
  if (err<0)
    {
      fprintf(stderr,
	      "Error setting the callback %s: %d\n",
	      module_name, errno);
      return -1;
    }


  // Tell the client about this action type.  Parameters
  // are: tag, short name, description (used for help
  // text), and socket name.

  pack_and_send_action_attr(COUNTER_TAG, "Get count",
			    "Global and Local Counter", sock);

  // Declare columns in the client's display where data
  // will be shown.  Parameters are: tag for the column,
  // column label, description of data, data type, and
  // socket.

  pack_and_send_data_attr (COUNTER_COLUMN_LOCAL,"Local Count",
			   "Number of time point has been reached",
			   TG_INT, sock);
  pack_and_send_data_attr (COUNTER_COLUMN_GLOBAL,"Global Count",
			   "Number of time point has been reached",
			   TG_INT, sock);

  // Data isn't sent to the Client until this function
  // is called.

  TG_flush(sock);
  
  return 0;
}


/*---------------------------------------------------------------------*/
/* callback for probe */

void CounterProbe_cb(GCBSysType sys, GCBTagType tag,
		     GCBObjType obj, GCBMsgType msg)
{
  int global_count;
  int local_count;
  Process *p;
  int task;
  DPCLPointAction  *action;
  DPCLActionPointP location;
  int sock;

  global_count=*((int*) msg);       // msg contains the global count
  p=(Process*) obj;       // DPCL Process
  task=p->get_task();     // parallel process id
  action=(DPCLPointAction*) tag;

  // Retrieve the location in the program of this point probe
  location=action->get_owner();

  // Increment counter

  if (counter_table.find((void*) (action)) == counter_table.end())
    {
      // new entry

      local_count=1;
    }
  else
    {
      // existing entry

      local_count=counter_table[(void*) (action)]+1;
    }

  counter_table[(void*) (action)]=local_count;

  // Where to send the data
  sock=action->get_type()->get_socket();

  // Send the count data to the Client.  Parameters are:
  // target program function name, identifier for the
  // instrumentation point (so the Client knows where in
  // the source code to display the data), name of the
  // column where data will appear, task id, thread id
  // (unused here), data value, and socket.
  pack_and_send_int(location->get_inst_point()->in_function.c_str(),
		    location->get_tag().c_str(), COUNTER_COLUMN_LOCAL, 
		    task, 0, local_count, sock);
  pack_and_send_int(location->get_inst_point()->in_function.c_str(),
		    location->get_tag().c_str(), COUNTER_COLUMN_GLOBAL, 
		    task, 0, global_count, sock);

  TG_flush(sock);
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

