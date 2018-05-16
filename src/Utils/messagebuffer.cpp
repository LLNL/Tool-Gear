//! \file messagebuffer.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/*
 * Simple message output buffer library that facilitates generating arbitrarily
 * long messages with sprintf and also allows a sprintf to to end of
 * an existing message (at for this to be quick.)
 * Users may get access to a const char * pointer to the buffer for 
 * copying or outputing.
 *
 * Design goals were to make buffer overflow impossible and make appending
 * a lot of lines (say 1024 lines) one at a time approximately linear time
 * and not a O(n^2) operation (that using strcat yields, if called 1024 times).
 * The resizes can be O(nlog(n)), so it is recommended that buffers are
 * reused (using clear()) instead of creating a new one each time.
 *
 * Created by John C. Gyllenhaal, 3/29/04 
 */

#include <stdio.h>
#include "tg_error.h"
#include <stdlib.h>
#include "tempcharbuf.h"
#include <stdarg.h>
#include "messagebuffer.h"
#include "tg_time.h"

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

// Works like sprintf except buffer automatically resized to make it fit.
int MessageBuffer::sprintf (const char *fmt, ...)
{
    va_list args;
    
    // Clear existing message
    clear();
    
    // Use vappend version of routine to "append" this message to the 
    // empty buffer
    va_start (args, fmt);
    int len = vappendSprintf (fmt, args);
    va_end(args);
    
    // Return length sprintf wrote
    return (len);
}

// Works like vsprintf except buffer automatically resized to make it fit.
int MessageBuffer::vsprintf (const char *fmt, va_list args)
{
    // Clear existing message
    clear();
    
    // Use vappend version of routine to "append" this message to the 
    // empty buffer
    int len = vappendSprintf (fmt, args);
    
    // Return length sprintf wrote
    return (len);
}

// Works like sprintf except appends to the existing message (if any)
// and automatically resizes the message buffer to make it fit.
int MessageBuffer::appendSprintf (const char *fmt, ...)
{
    va_list args;
    
    // Use vappend version of routine to "append" this message to the 
    // empty buffer
    va_start (args, fmt);
    int len = vappendSprintf (fmt, args);
    va_end(args);
    
    // Return length sprintf wrote
    return (len);
}

// Works like vsprintf except appends to the existing message (if any) 
// and automatically resizes the message buffer 
int MessageBuffer::vappendSprintf (const char *fmt, va_list passed_args)
{
    // Need to make copy of passed_args before each use because we
    // may to parse the args multiple times and parsing the args
    // on some platforms destroys them (i.e., for 64-bit opteron builds).
    // If don't copy, will get segfault in second vsnprintf call on opteron.
    va_list args;

    // Get the current length of message buffer 
    // (buffer is actually 1 character bigger, reserved for terminator)
    int bufLen = buf.getMaxLen();
    
    // Calculate the remaining length in buffer for this sprintf
    int availLen = bufLen - curLen;
    
    // Get pointer to buffer for use in sprintf
    char *bufPtr = buf.contents();

    // Make copy of passed args for use in vsnprintf
    // vsnprintf apparently doesn't destroy args on 32bit x86, AIX, or
    // tru64 but does on 64-bit opteron.   Figured out that va_copy 
    // was the way to make local copies for uses like this. -JCG
    // Of course, not supported on tru64 (sigh) -JCG
#ifndef TG_TRU64
    va_copy (args, passed_args);

    // Print out message using vsnprintf (which may not work
    // if there is not enough buffer) at end of current buffer
    // contents.
    int appendLen = vsnprintf (&bufPtr[curLen], availLen, fmt, args);

    // Delete copy of passed args
    va_end (args);

    // Otherwise, do the no va_copy hack if not supported 
#else

    // Print out message using vsnprintf (which may not work
    // if there is not enough buffer) at end of current buffer
    // contents.
    int appendLen = vsnprintf (&bufPtr[curLen], availLen, fmt, passed_args);

#endif

    // If sprintf didn't work, keep trying, resizing the buffer each 
    // time.  On newer implementations, availLen will actually specify 
    // how much we need to increase the buffer, older implmentations 
    // return -1 (so we have to iterate our buffer resizes), and
    // apparently for the IBM 6.0 compiler, returns appendLen == availLen,
    // when run out of buffer, so make >=!
    // And on Tru64, cxx returns appendLen = availLen -1! (sigh!), when
    // it runs out of buffer, so add -1 to test (and use >=).
    while ((appendLen >= (availLen -1)) || (appendLen < 0))
    {
	// Double the buffer size
	int newLen = (bufLen + 1) * 2;
	
	// Sanity check, check for to integer overflow
	if ((newLen > 1000000000) || (newLen < 0))
	{
	    TG_error ("MessageBuffer::appended_sprintf: "
		      "buf too large (size %i)!", newLen);
	}
	
	// If appendLen positive, this is the size we need.
	// Make sure doubling provides all the space needed
	if ((appendLen > 0) && ((newLen-curLen) <= appendLen))
	{
	    
	    // Keep doubling, until have more than appendLen characters
	    // free (<=, not <, so will have room for terminator)
	    while ((newLen-curLen) <= appendLen)
	    {
		// Double the size of the buffer again
		newLen *= 2;
		
		// Sanity check, don't want to infinite loop due
		// to integer overflow (if have 1GB buffer, in trouble)
		if ((newLen > 1000000000) || (newLen < 0))
		{
		    TG_error ("MessageBuffer::appended_sprintf: "
			      "buf unexpectedly large (size %i)!",
			      newLen);
		}
	    }
	}
	
	// Resize the buffer to this new size
	// (buf adds 1 for terminator, so subtract one to 
	//  keep buf size a power of 2 (which I like :) ))
	buf.resize (newLen-1);
	
	// Get new values for bufPtr, bufLen, and availLen, since the
	// above manipulations invalidated all the old values
	bufPtr = buf.contents();		
	bufLen = buf.getMaxLen();
	availLen = bufLen - curLen;

	// va_copy not supported on tru64 (sigh) -JCG
#ifndef TG_TRU64
	// Make copy of passed args for use in vsnprintf
	va_copy (args, passed_args);
	
	// Try the sprintf again with the buffer buffer
	appendLen = vsnprintf (&bufPtr[curLen], availLen, fmt, args);

	// Delete copy of passed args
	va_end (args);

#else
	// Do the no va_copy hack which is not portable 
	// Try the sprintf again with the buffer buffer
	appendLen = vsnprintf (&bufPtr[curLen], availLen, fmt, passed_args);

#endif
    }
    
    // Update curLen with appended length
    curLen += appendLen;
    
    // Return length of appended string
    return (appendLen);
}


// Copies the next line of text into the message buffer (not copying
// the newline) and advances parsePtr to either the next character
// after the newline or the termintor.  Returns TRUE if a next line
// is found (may be empty if have \n\n) or FALSE if at end of the
// string being parsed.
bool MessageBuffer::getNextLine (const char *&parsePtr)
{
    // Get pointer to buffer for use in sprintf
    char *bufPtr = buf.contents();
    
    // Get local copy of parsePtr for optimization reasons
    const char *scanPtr = parsePtr;
    
    // If at end of string, set to empty string and return FALSE
    if (*scanPtr == 0)
    {
	bufPtr[0] = 0;
	curLen = 0;
	return (FALSE);
    }
    
    // Get the current length of message buffer
    // (buffer is actually 1 character bigger, reserved for terminator)
    int bufLen = buf.getMaxLen();
    
    // Copy over buffer until hit newline or end of string
    int len = 0;
    char ch;
    while (((ch = scanPtr[len]) != 0) && (ch != '\n'))
    {
	bufPtr[len] = ch;
	++len;
	
	// Detect the need to resize the buffer
	if (len >= bufLen)
	{
	    // resize the buffer to be twice as big (subtract 1 to
	    // keep size a power of 2, which I like)
	    buf.resize ((bufLen *2)-1);
	    
	    // Get new values for bufPtr and bufLen since the
	    // above manipulations invalidated all the old values
	    bufPtr = buf.contents();
	    bufLen = buf.getMaxLen();
	}
    }
    
    // Terminate buffer
    bufPtr[len] = 0;
    
    // Update current length 
    curLen = len;
    
    // Update the passed in pointer possition to point to either
    // the character past the newline or the terminator
    if (scanPtr[len] == '\n')
	parsePtr = &scanPtr[len+1];
    else
	parsePtr = &scanPtr[len];
    
    // Return TRUE, we grabbed another line
    return (TRUE);
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

