/* tg_time.c */
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

/*
 * Routines for getting wall-clock time and printing messages
 * to stderr that are timestamped with wall-clock time.  
 *
 * Created by John Gyllenhaal 12/21/00.
 * Modified to use more standard gettimeofday, John May 7/20/2001
 */
 
#include "tg_time.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
/* #include <sys/timers.h> */
#include <sys/time.h>

/* Returns time in seconds (double) since the first call to TG_time
 * (i.e., the first call returns 0.0).  
 */
double TG_time( void )
{
	static double base_time = -1;
/*        struct timespec ts; */
	struct timeval ts;
        int status;
	double new_time;

	/* Get wall-clock time */
        /* status = getclock( CLOCK_REALTIME, &ts ); */
	status = gettimeofday( &ts, NULL );

	/* Return 0.0 on error */
        if( status != 0 ) return 0.0;

	/* Converst structure to double (in seconds ) (a large number) */
/*        new_time = (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9; */
        new_time = (double)ts.tv_sec + (double)ts.tv_usec * 1e-6;

	/* If first time called, set base_time 
	 * Note: Lock shouldn't be needed, since even if multiple
	 *       threads initialize this, it will be to basically
	 *       the same value.
	 */
	if (base_time < 0)
	    base_time = new_time;

	/* Returned offset from first time called */
	return (new_time - base_time);
}

/* Works like printf, except prefixes wall-clock time (using TG_time)
 * and writes to stderr.  Also flushes stdout, so messages stay
 * in reasonable order.
 */
void TG_timestamp (char * fmt, ...)
{
    va_list args;

    /* Get wall-clock time since first call to TG_time */
    double sec = TG_time();
    
    /* Flush stdout, so message appear in reasonable order */
    fflush (stdout);

    /* Print out timestamp */
    fprintf (stderr, "%7.3f: ", sec);
    
    /* Print out passed message */
    va_start (args, fmt);
    vfprintf (stderr, fmt, args);

    va_end (args);
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

