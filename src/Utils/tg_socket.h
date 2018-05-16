/*! \file tg_socket.h
 */
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/*
 * Simple socket library for passing messages between two
 * threads or processes (i.e., Client and Collector).
 * Can be compiled to use a background thread for sending,
 * receiving or both.  In threaded mode, data that arrives on
 * the socket is read right away and put into a buffer
 * (so that the socket buffer doesn't back up and overflow),
 * and data transmissions can take place in the background.
 * Normally, we have been using threaded reading for the Client
 * and no threading for the Collector.
 *
 * Created by John C. Gyllenhaal, 4/21/00 
 * Modified by John May
 */

#ifndef TG_SOCKET_H
#define TG_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/* See TG_socket.c for description of each function */
#if 0
extern void TG_error (char *fmt, ...);
extern void TG_errno (char *fmt, ...);
extern void TG_open_sockets (int *to_server, int *from_server, 
			int *to_client, int *from_client);
extern void TG_close_sockets  (int to_server, int from_server, 
			int to_client, int from_client);
#endif
/*! Writes tag, id, size, and then size bytes of buf to
 * the non-blocking socket fd (which must already be opened
 * and set appropriately).  
 * Punts on error.  
 */
extern void TG_send (int fd, int tag, int id, int size, const void *buf);

/*! Blocking version of TG_nb_recv; will only return when data is ready.
 * Nonthreaded version does select on fd followed by a nonblocking read.
 * Threaded version blocks on a condition variable until it is notified
 * that data is available in the input queue.
 */
extern int TG_recv (int fd, int *tag, int *id, int *size, void **buf);

/*! For threaded writing, this function tells the write thread to
 * wake up and send the queued data.  For nonthreaded writing, this
 * function does nothing.
 */
extern void TG_flush(int fd);

/*! Wrapper for TG_internal_nb_recv -- allows it to work the same
 * for both threaded and nonthreaded reading.
 */
extern int TG_nb_recv (int fd, int *tag, int *id, int *size, void **buf);

/*! For threaded reads, this function returns a non-zero value if data 
 * data is available to be read (i.e., TG_recv won't block).  For
 * nonthreaded reads, this function is UNDEFINED, and attempts to use
 * it will produce a linker error.
 */
extern int TG_read_data_queued( int fd );

/*! Determine whether data is wating to be read, without blocking or
 * reading any data.  Returns nonzero if data is available.
 */
extern int TG_poll_socket( int fd );
#ifdef __cplusplus
           }
#endif
#endif


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

