//! \file lineparser.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// Created by John C. Gyllenhaal, 1/05/04 

#ifndef TG_LINEPARSER_H
#define TG_LINEPARSER_H

#include <stdio.h>
#include "tg_error.h"
#include <stdlib.h>

//! Simple line parsing library that can read in arbitrarily long
//! lines from files and returns const char * pointers to them. 
class LineParser
{

public:

    /*! Initialize with the file that the lines are to be read from 
     * Flag if want parser to fclose the file on exit.
     */
    LineParser (FILE *file, bool fcloseFileOnExit = FALSE)
	{
	    in = file;
	    closeOnExit = fcloseFileOnExit;

	    // Intially at line 0 "before read something"
	    lineNumber = 0;

	    // Always create a buffer upfront to simplify control logic
	    maxLen = 128;
	    if ((buf = (char *) malloc (maxLen)) == NULL)
		TG_error ("Out of memory allocating %i bytes\n", maxLen);

	    // Need to read the next line before returning anything to user
	    peekedAt = FALSE;
	}
    ~LineParser ()
	{
	    // Free the line buffer 
	    if (buf != NULL)
		free (buf);

	    // If specified to do so, close the file
	    if (closeOnExit)
		fclose (in);
	}

    /*! Returns pointer to internal buffer holding the arbitrarily long 
     * next line in the file (DO NOT MODIFY) or NULL if at end of file.  
     *
     * Works with the peakNextLine() below to return the correct line 
     * even if it has been peeked at earlier.  
     */
    const char *getNextLine ()
	{
	    // Peak at the next line 
	    peekNextLine();

	    // Mark it as not peeked at (since reading line for real) 
	    peekedAt = FALSE;

	    // Increment lineNumber, so now "points" at this line just read in
	    if (buf != NULL)
		++lineNumber;

	    // Return buffer holding line (may be NULL if at EOF)
	    return (buf);
	}
    
    /*! Returns pointer to internal buffer holding the value of the
     * arbitrarily long next line (DO NOT MODIFY) or NULL if at end of file.
     *
     * Acts like a peek, so can peak the same line multiple times and
     * the next getNextLine() will get the same line you just peeked at.
     */
    const char *peekNextLine ()
	{
	    /* If already peeked at, or if buf NULL (EOF), return buf) */
	    if (peekedAt || (buf == NULL))
		return (buf);

	    /* Mark that we are peeking at the next line */
	    peekedAt = TRUE;
	    
	    /* Read from file until hit end of line */
	    int index = 0;
	    int ch;
	    
	    /* Loop and read until end of line or file */
	    while ((ch = fgetc(in)) != EOF)
	    {
		/* Store next character (guarenteed to have space) */
		buf[index] = ch;

		/* Advance pointer to next empty space */
		index++;

		/* Do we need to expand the buffer for the next character or
		 * terminator? 
		 */
		if (index >= maxLen)
		{
		    /* If so, double the current size using realloc */
		    maxLen = maxLen * 2;
		    buf = (char *) realloc (buf, maxLen);
		}

		// Exit loop if have hit end of line 
		if (ch == '\n')
		    break;
	    }
	    
	    /* If read something, terminate it */
	    if (index > 0)
	    {
		/* Terminate string, guarenteed to have space */
		buf[index] = 0;
	    }
	    /* Otherwise, at end of file, free buffer and set to NULL */
	    else
	    {
		free (buf);
		buf = NULL;
	    }
		
	    /* Return the peeked line (may be null) */
	    return (buf);
	}

    /*! Returns the maximum line length that can be processed without a 
     * reallocation (terminator must also fit in length to avoid reallocation)
     */
    int getMaxLen () {return (maxLen);}

    /*! Returns the line number of the line just read (not just peeked at).
     *  Returns 0 if had not read any lines yet.
     */
    int lineNo () {return (lineNumber);}

private:

    FILE *in;
    char *buf;
    int maxLen;
    bool peekedAt;
    bool closeOnExit;
    int lineNumber;
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

