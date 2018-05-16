/* tg_error.c */
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

/****************************************************************************
**
** Tool Gear error message printing (and exiting) functions. 
** 
** TG_error() prints source file and line number, the user message (same 
** format as printf), a new line, and then exits(-1).
**
** TG_errno() also prints out the errno and the errno message using perror.
**
** All routines use TG_error() and TG_errorContext (added by macro).
** This design allows a single breakpoint in TG_error() to catch all
** exits due to errors.
**
** This is somewhat of a hack since some compilers (such as xlC) does
** not support the c99 preprocessor standard that allows:
** 
** #define TG_error(...) TG_error (__FILE__, __LINE__, __VA_ARGS__)
**
** Hopefully this version is more portable (and doesn't cause too many 
** problems)
**
** Initial version by John Gyllenhaal at LLNL 5/6/02
*****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "tg_error.h"
#include <errno.h>

/* Prevent tricky redefs of TG_error from messing up this definition */
#undef TG_error

static char *errorFileName = NULL;
static int errorLineNo = -1;
static int errorPrintErrno = 0;

/* Set up context (file and line number) for when TG_error called */
void TG_errorContext (const char *fileName, int lineNo)
{
    errorFileName = (char *)fileName;
    errorLineNo = lineNo;
}

/* Flag that should print error number when TG_error called */
void TG_printErrno()
{
    errorPrintErrno = 1;
}

/* Macros call this function TG_error() to handle all error functions */
void TG_error (const char *fmt, ...)
{
    va_list     args;

    /* Flush stdout, so error message comes last */
    fflush (stdout);

    /* Print header with file and line number of TG_error (if exists)*/
    if (errorFileName != NULL)
    {
	/* Handle has context but no errno case */
	if (errorPrintErrno == 0)
	{
	    fprintf (stderr, "Tool Gear error, %s line %i:\n", errorFileName, 
		     errorLineNo);
	}

	/* Handle has context and errno case */
	else
	{
	    fprintf (stderr, 
		     "Tool Gear error (with errno %i info), %s line %i:\n", 
		     errno, errorFileName, errorLineNo);
	}
    }

    /* Don't have file or line info, so print out generic header */
    else
    {
	/* Handle no context and no errno case */
	if (errorPrintErrno == 0)
	{	
	    fprintf (stderr, "Tool Gear error:\n");
	}

	/* Handle no context but errno case */
	else
	{
	    fprintf (stderr, "Tool Gear error (with errno %i info):\n", errno);
	}
    }
    
    /* Print out error message in printf format using vfprintf */
    va_start (args, fmt);
    vfprintf (stderr, fmt, args);
    va_end(args);
    
    /* Print trailing newline so caller doesn't need to */
    fprintf (stderr,"\n");

    /* Print errno info if called with TG_errno() macro */
    if (errorPrintErrno != 0)
    {
	char buf[100];
	sprintf (buf, "Errno(%i)", errno);
	perror(buf);
    }

    /* Exit with non-zero value, -1 chosen by random */
    exit(-1);
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

