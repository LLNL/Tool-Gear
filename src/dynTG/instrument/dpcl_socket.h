//! \file dpcl_socket.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

//! Functions to handle input from the Client and process it.
// John May, 27 October 2000


#ifndef DPCL_SOCKET_H
#define DPCL_SOCKET_H

//! Called by DPCL when data arrives on a monitored socket.  This
//! function reads one message from the socket and dispatches it
//! according to the message tag.
int dpcl_socket_handler( int fd );

//! Instantiates a DPCLPointAction at a specified location in the
//! target program.  The message can request that the action be
//! placed in one process or all the processes in a program.
void activate_action( Application * app, char * buf, int tag );

//! Remove the specified action from the action point.
void deactivate_action( Application * app, char * buf, int tag );

//! Handle a request to run a program as a parallel application.
void unpack_parallel_app( char * buf );

//! Handle a request to run a program as a sequential application
//! (calls run_app_sequential(), which may determine that the
//! program should run as a parallel job.)
void unpack_sequential_app( char * buf );

//! Change the current working directory.
void unpack_change_dir( char * buf );

//! Read the functions in a specified module.
void unpack_parse_module( Application * app, char * buf );

//! Read the instrumentation points in a specified function.
void unpack_parse_function( Application * app, char * buf );

//! Insert or remove instrumentation from all locations matching
//! a specification.
void instrument_app( Application * app, char * buf );

//! Start the target program executing.
void start_program( Application * app );

//! Halt the target program.
void stop_program( Application * app );

//! Read a local file and send it to the Client.
void read_file( char * buf, int id );

//! Process a message from the Client that it's still alive.
void unpack_heartbeat( char * buf );

//! Remove all actions from the target program.  This is an
//! internal function, not available as a request from the Client.
void remove_all_actions( Application * app );

//! Clean up and shut down the target program.  This function
//! looks in the ::finalizationList and calls each action there
//! as a one-shot probe to deinitialize the target program and
//! the probes.  When and if these actions return, this function
//! tells DPCL to shut the program down.
void shutdown_app( void );
#endif // DPCL_SOCKET_H
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

