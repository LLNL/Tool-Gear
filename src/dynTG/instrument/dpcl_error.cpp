/* dpcl_error.c */
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

/****************************************************************************
**
** Tool Gear error message printing (and exiting) functions. 
** 
** DPCL_error() prints source file and line number, the user message (same 
** format as printf), a new line, sends DPCL_SAYS_QUIT, and then exits(2).
**
** DPCL_errno() also prints out the errno and the errno message using perror.
**
** All routines use DPCL_error() and DPCL_errorContext (added by macro).
** This design allows a single breakpoint in DPCL_error() to catch all
** exits due to errors.
**
** This is somewhat of a hack since some compilers (such as xlC) does
** not support the c99 preprocessor standard that allows:
** 
** #define DPCL_error(...) DPCL_error (__FILE__, __LINE__, __VA_ARGS__)
**
** Hopefully this version is more portable (and doesn't cause too many 
** problems)
**
** Initial version by John Gyllenhaal at LLNL 5/6/02
** DPCL_error version (based on TG_error) by John Gyllenhaal at LLNL 3/16/06
*****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "tg_error.h"
#include <errno.h>
#include <string.h>
#include "messagebuffer.h"
#include "tg_pack.h"
#include "command_tags.h"
#include "tg_socket.h"
#include "unistd.h"

/* Prevent tricky redefs of DPCL_error from messing up this definition */
#undef DPCL_error

static char *errorFileName = NULL;
static int errorLineNo = -1;
static int errorPrintErrno = 0;

/* Need socket to sent DPCL_SAYS_QUIT to TGClient */
int DPCL_error_sock = -1;

/* Set up context (file and line number) for when DPCL_error called */
void DPCL_errorContext (const char *fileName, int lineNo)
{
    errorFileName = (char *)fileName;
    errorLineNo = lineNo;
}

/* Flag that should print error number when DPCL_error called */
void DPCL_printErrno()
{
    errorPrintErrno = 1;
}

/* Macros call this function DPCL_error() to handle all error functions */
void DPCL_error (const char *fmt, ...)
{
    va_list     args;
    MessageBuffer message;

    /* Flush stdout, so error message comes last */
    fflush (stdout);

    /* Print header with file and line number of DPCL_error (if exists)*/
    if (errorFileName != NULL)
    {
	/* Handle has context but no errno case */
	if (errorPrintErrno == 0)
	{
	    message.sprintf("dynTG error, %s line %i:\n", errorFileName, 
			    errorLineNo);
	}

	/* Handle has context and errno case */
	else
	{
	    message.sprintf(
		"dynTG error (with errno %i info), %s line %i:\n", 
		errno, errorFileName, errorLineNo);
	}
    }

    /* Don't have file or line info, so print out generic header */
    else
    {
	/* Handle no context and no errno case */
	if (errorPrintErrno == 0)
	{	
	    message.sprintf("dynTG error:\n");
	}

	/* Handle no context but errno case */
	else
	{
	    message.sprintf("dynTG error (with errno %i info):\n", errno);
	}
    }
    
    /* Print out error message in printf format using vfprintf */
    va_start (args, fmt);
    message.vappendSprintf( fmt, args);
    va_end(args);
    
    /* Print trailing newline so caller doesn't need to */
    message.appendSprintf("\n");

    /* Print errno info if called with DPCL_errno() macro */
    if (errorPrintErrno != 0)
    {
	message.appendSprintf ("Errno(%i): %s", errno, strerror(errno));
    }

    /* If socket is not -1, send error message to socket */
    if (DPCL_error_sock != -1)
    {

	/* Allocate buffer big enough to hold packed message*/
	int max_size = message.strlen() + 100;
	char *buf_ptr = (char *)malloc (max_size);
	if (buf_ptr != NULL)
	{
	    int len = TG_pack (buf_ptr, max_size, "S", message.contents());
	    TG_send (DPCL_error_sock, COLLECTOR_ERROR_MESSAGE, 0, len,
		     buf_ptr);
	    
	}
	else
	{
	    fprintf (stderr, 
		     "Warning: Out of memory so cannot send "
		     "this message:\n%s\n",
		     message.contents());
	}

	// Send QUIT to TGclient twice, needed by TGclient to exit cleanly
	TG_send( DPCL_error_sock, DPCL_SAYS_QUIT, 0, 0, NULL );
	TG_send( DPCL_error_sock, DPCL_SAYS_QUIT, 0, 0, NULL );
	TG_flush( DPCL_error_sock );

	// The flush above doesn't actually send the data, so delay
	// a little to give the message a chance to get out.
	sleep (2);
    }
    else
    {
	fprintf (stderr, 
		 "Warning: No DPCL_error_sock specified so cannot send "
		 "this message:\n%s\n",
		 message.contents());
    }

    /* Exit with non-zero value, 2 chosen by random */
    exit(2);
}

/* Macros call this function DPCL_error() to handle all warning functions */
void DPCL_warn (const char *fmt, ...)
{
    va_list     args;
    MessageBuffer message;

    /* Flush stdout, so error message comes last */
    fflush (stdout);

    /* Print header with file and line number of DPCL_error (if exists)*/
    if (errorFileName != NULL)
    {
	message.sprintf("dynTG warning, %s line %i:\n", errorFileName, 
			    errorLineNo);
    }

    /* Don't have file or line info, so print out generic header */
    else
    {
	message.sprintf("dynTG warning:\n");
    }
    
    /* Print out error message in printf format using vfprintf */
    va_start (args, fmt);
    message.vappendSprintf( fmt, args);
    va_end(args);
    
    /* Print trailing newline so caller doesn't need to */
    message.appendSprintf("\n");

    /* If socket is not -1, send error message to socket */
    if (DPCL_error_sock != -1)
    {

	/* Allocate buffer big enough to hold packed message*/
	int max_size = message.strlen() + 100;
	char *buf_ptr = (char *)malloc (max_size);
	if (buf_ptr != NULL)
	{
	    int len = TG_pack (buf_ptr, max_size, "S", message.contents());
	    TG_send (DPCL_error_sock, COLLECTOR_ERROR_MESSAGE, 0, len,
		     buf_ptr);
	    
	}
	else
	{
	    fprintf (stderr, 
		     "Warning: Out of memory so cannot send "
		     "this message:\n%s\n",
		     message.contents());
	}

	// Flush out the data
	TG_flush( DPCL_error_sock );

    }
    else
    {
	fprintf (stderr, 
		 "Warning: No DPCL_error_sock specified so cannot send "
		 "this message:\n%s\n",
		 message.contents());
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

