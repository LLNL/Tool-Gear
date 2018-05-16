//! \file messagebuffer.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// Created by John C. Gyllenhaal, 3/29/04 


#ifndef TG_MESSAGEBUFFER_H
#define TG_MESSAGEBUFFER_H

#include <stdio.h>
#include "tg_error.h"
#include <stdlib.h>
#include "tempcharbuf.h"
#include <stdarg.h>
#include <string.h>

// Handle platforms that do not define TRUE and FALSE
#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

//! Arbritrary-length message buffer output library.

//! Simple message output buffer library that facilitates generating arbitrarily
//! long messages with sprintf and also allows a sprintf to to end of
//! an existing message (at for this to be quick.)
//! Users may get access to a const char * pointer to the buffer for 
//! copying or outputing.
//!
//! Design goals were to make buffer overflow impossible and make appending
//! a lot of lines (say 1024 lines) one at a time approximately linear time
//! and not a O(n^2) operation (that using strcat yields, if called 1024 times).
//! The resizes can be O(nlog(n)), so it is recommended that buffers are
//! reused (using clear()) instead of creating a new one each time.
class MessageBuffer
{
public:
    //! Create a messageBuffer that automatically resizes to prevent overflow
    MessageBuffer () : curLen(0) 
	{
	}
    
    //! Sets buffer to the empty string ""
    void clear() 
	{
	    curLen = 0;
	    char *ptr = buf.contents();
	    ptr[0] = 0;
	}

    //! Works like sprintf except buffer automatically resized to make it fit.
    int sprintf (const char *fmt, ...);

    //! Works like vsprintf except buffer automatically resized to make it fit.
    int vsprintf (const char *fmt, va_list args);

    //! Works like sprintf except appends to the existing message (if any)
    //! and automatically resizes the message buffer to make it fit.
    int appendSprintf (const char *fmt, ...);

    //! Works like vsprintf except appends to the existing message (if any) 
    //! and automatically resizes the message buffer 
    int vappendSprintf (const char *fmt, va_list args);

    //! Appends character to the existing message (if any) 
    //! and automatically resizes the message buffer
    void appendChar (char ch)
	{
	    // Get the current length of message buffer 
	    // (buffer is actually 1 character bigger, reserved for terminator)
	    int bufLen = buf.getMaxLen();

	    // Resize buffer, if necessary
	    if (curLen >= bufLen)
	    {
		// Double the buffer size, add 1 to keep power of 2
		// See vappendSprintf for details
		int newLen = (bufLen * 2) + 1;
		buf.resize (newLen);
		bufLen = buf.getMaxLen();
	    }
	    
	    // Get pointer to buffer 
	    char *bufPtr = buf.contents();
	    
	    // Set character and terminator
	    bufPtr[curLen] = ch;
	    bufPtr[curLen+1] = 0;
		
	    // Update current length
	    curLen++;
	}

    //! If newLen is less than the length of the string, then the string
    //! is truncated at position newLen.  Otherwise nothing happens
    void truncate (int newLen)
	{
	    // Only do something if newLen is reasonable
	    if ((newLen < curLen) && (newLen >= 0))
	    {
		// Get pointer to buffer 
		char *bufPtr = buf.contents();

		// Put terminator to set new length
		bufPtr[newLen] = 0;

		// Update length to new shorter length
		curLen = newLen;
	    }
	}


    //! Copies the next line of text into the message buffer (not copying
    //! the newline) and advances parsePtr to either the next character
    //! after the newline or the termintor.  Returns TRUE if a next line
    //! is found (may be empty if have \n\n) or FALSE if at end of the
    //! string being parsed.
    bool getNextLine (const char *&parsePtr);

    // DEBUG
    void debug (FILE *out)
	{
	    fprintf (out, "curLen = %i, maxLen = %i, buf:\n'%s'\n",
		     curLen, buf.getMaxLen(), buf.contents());
	}

    //! Returns const char * pointer to the message for copying or
    //! outputting.  Contents must not be modified and pointer may
    //! become invalid after any other MessageBuffer operation.
    const char *contents () {return (buf.contents());}

    //! Return strdup() generated copy of the current buffer contents
    char *strdup() {return (::strdup(contents()));}

    //! Return the strlen of the current bffer contents
    int strlen () {return(curLen);}

private:
    TempCharBuf buf;  // Resizable buffer 
    int curLen;       // Length of current buffer contents
};

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

