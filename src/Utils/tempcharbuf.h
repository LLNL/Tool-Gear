// \file tempcharbuf.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/*
 * Simple resizable temporary character buffer that can be used to hold
 * strings (while parsing a file) and resize along the way, if more
 * buffer space is needed.   Returns char * pointers that are guarenteed
 * to hold minSize + 1 (for terminator) characters.  You always get the
 * same buffer (pointer may move) everytime you ask for the buffer, 
 * so treat like it is exactly one buffer.
 *
 * Created by John C. Gyllenhaal, 1/16/04 
 */

#ifndef TG_TEMPCHARBUF_H
#define TG_TEMPCHARBUF_H

#include <stdio.h>
#include "tg_error.h"
#include <stdlib.h>

class TempCharBuf
{

public:
    
    /*! Creates a temporary temp buffer manager */
    TempCharBuf ()
	{
	    // Always create a buffer upfront to simplify control logic
	    maxLen = 128;
	    if ((buf = (char *) malloc (maxLen)) == NULL)
		TG_error ("Out of memory allocating %i bytes\n", maxLen);
	}
    ~TempCharBuf ()
	{
	    // Free the line buffer 
	    if (buf != NULL)
		free (buf);
	}
    
    /*! Returns a pointer to character buffer that can hold at least
     * minSize +1 characters (+1 for terminator).  Will resize buffer
     * (preserving contents) if necessary to fit size.  Always returns
     * the same "contents" but pointer may change due to resizes.
     */
    char *resize(int minSize)
	{
	    // Do we need to expand the buffer to fit minSize plus terminator?
	    if (minSize >= maxLen) 
	    {
		// Yes, resize it
		maxLen = minSize + 1;
		buf = (char *) realloc (buf, maxLen);
	    }
	    
	    // Return pointer to modifyable buffer of least length minSize+1
	    return (buf);
	}

    /*! Returns the current buffer with no resizing done */
    char *contents() {return (buf);}

    /*! Returns the maximum length that can be written (not counting 
     * terminator) without a reallocation
     */
    int getMaxLen () {return (maxLen-1);}

private:

    char *buf;
    int maxLen;
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

