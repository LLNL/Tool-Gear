/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include <dpclExt.h>
#include "internal.h"
#include <assert.h>


/*---------------------------------------------------------------------*/
/* Globals */

#define MAXSTACK 100

double *stack[8];
int stack_ptr[8];


/*---------------------------------------------------------------------*/
/* get the actual counter and return it */

void start_counter(int cnt)
{
  assert(stack_ptr[cnt]<MAXSTACK);

  stack[cnt][stack_ptr[cnt]]=pmapi_int_getcounter(cnt);

  stack_ptr[cnt]++;
}


/*---------------------------------------------------------------------*/
/* get the actual counter and return it */

void stop_counter(int cnt, AisPointer ais_send_handle)
{
  int counter[2];
  counter_res_t result;

  assert(stack_ptr[cnt]>0);

  stack_ptr[cnt]--;

  result.value=pmapi_int_getcounter(cnt)-stack[cnt][stack_ptr[cnt]];
  result.counter=cnt;

  Ais_send(ais_send_handle, &result, sizeof(result));
}


/*---------------------------------------------------------------------*/
/* Switchboard for counter routines */

void start_counter_00() { start_counter(0); }
void start_counter_01() { start_counter(1); }
void start_counter_02() { start_counter(2); }
void start_counter_03() { start_counter(3); }
void start_counter_04() { start_counter(4); }
void start_counter_05() { start_counter(5); }
void start_counter_06() { start_counter(6); }
void start_counter_07() { start_counter(7); }

void stop_counter_00(AisPointer ais_send_handle) { stop_counter(0,ais_send_handle); }
void stop_counter_01(AisPointer ais_send_handle) { stop_counter(1,ais_send_handle); }
void stop_counter_02(AisPointer ais_send_handle) { stop_counter(2,ais_send_handle); }
void stop_counter_03(AisPointer ais_send_handle) { stop_counter(3,ais_send_handle); }
void stop_counter_04(AisPointer ais_send_handle) { stop_counter(4,ais_send_handle); }
void stop_counter_05(AisPointer ais_send_handle) { stop_counter(5,ais_send_handle); }
void stop_counter_06(AisPointer ais_send_handle) { stop_counter(6,ais_send_handle); }
void stop_counter_07(AisPointer ais_send_handle) { stop_counter(7,ais_send_handle); }


/*---------------------------------------------------------------------*/
/* Initialize the library and read configuration */

void InitPMAPI()
{
  int numcount,res,i;

  numcount=pmapi_int_open();
  assert(numcount>0);
  assert(numcount<=8);
    
  res=pmapi_int_readconfig();
  assert(res>=0);

  res=pmapi_program_counters();
  assert(res>=0);

  for (i=0; i<numcount; i++)
    {
      stack[i]=(double*) malloc(sizeof(double)*MAXSTACK);
      assert(stack[i]!=NULL);
      stack_ptr[i]=0;
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

