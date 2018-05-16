/*! \file dpcl_error.h
 */
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/*!***************************************************************************
**
** dynTG (DPCL) error message printing (and exiting) functions. 
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

#ifndef DPCL_ERROR_H
#define DPCL_ERROR_H


/* Define prototypes before macros below which redefine DPCL_error and DPCL_errno!
 */
/*! Macros call this function DPCL_error() to handle all error functions */
extern void DPCL_error (const char *fmt, ...);
/*! Set up context (file and line number) for when DPCL_error called */
extern void DPCL_errorContext (const char *fileName, int lineNo);
/*! Flag that should print error number when DPCL_error called */
extern void DPCL_printErrno();

/*! Macro call this to print warning, don't exit */
extern void DPCL_warn (const char *fmt, ...);

/*! c99 preprocessor standard compliance becomes common (use of __VA_ARGS__ in
 * macros, use the following hack to set error context before calling DPCL_error
 * and DPCL_errno.  All calls use DPCL_error, context specifies if errno printed.
 * (The comma syntax is used to make "if (error) DPCL_error()" work properly.)
 */
#define DPCL_checkAlloc(ptr) if (!ptr) DPCL_error ("Out of memory, exiting...")
#define DPCL_errno DPCL_printErrno(), DPCL_error 
#define DPCL_error DPCL_errorContext(__FILE__, __LINE__), DPCL_error 
#define DPCL_warn DPCL_errorContext(__FILE__, __LINE__), DPCL_warn

/* Need socket to sent DPCL_SAYS_QUIT to TGClient, must be set by dynTG */
extern int DPCL_error_sock;



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

