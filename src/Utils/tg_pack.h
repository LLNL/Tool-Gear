//! \file tg_pack.h
//! Functions to pack and unpack data in text-only messages.
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

/* John May, 27 October 2000 */
/* MessageBuffer version by John Gyllenhaal, 2 April 2004 */


#ifndef TG_PACK_H
#define TG_PACK_H

#include <stdarg.h>

#include "messagebuffer.h"

	/*! Pack data into a self-describing buffer supplied by the
	 * user.  No more than length characters will be packed.  The
	 * actual number of bytes packed is returned.  The format parameter 
	 * should contain a \\0 terminated string consisting of a sequence
	 * of the characters S, I, D, and A, descrbing the types of the
	 * values to be packed.  The remaining parameters are interpreted
	 * and packed according to the sequence of letters in the format.
	 * The layout of the data in the buffer is as follows (angle
	 * brackets do not appear in the actual output):
	 * For string data, S<length in ascii><string with \\0>
	 * For integer data, I<decimal integer in ascii>
	 * For double precision data, D<ascii value in scientific notation>
	 * For arrays of strings, A<number of strings><strings, each followed
	 * by \\0>.  For these arrays, the caller passes an nonnegative
	 * integer specifying the number of strings, followed by a char
	 * pointer to a sequence of that number of strings, each terminated
	 * by a \\0.
	 * All integer data is given in a field 10 characters wide, padded
	 * with spaces.  Double data is given in a space-padded field
	 * of 30 characters with 16 decimal digits of precision.
	 *
	 * Since the string field is terminated with a \\0, the buffer
	 * can have one or more internal \\0 characters.
	 * 
	 */
	unsigned TG_pack( char * buf, unsigned length,
			const char * format, ... );

        /*! A MessageBuffer version of the above routine that cannot overflow
         * the buffer (thus no maxLength).  Returns length of message.
         */
	unsigned TG_pack( MessageBuffer &mbuf, const char * format, ... );

        /*! A MessageBuffer va_list version to provide a single implementation
	 * of the MessageBuffer version to be used by several calls
 	 */
         unsigned TG_vpack( MessageBuffer &mbuf, const char * format, 
			    va_list ap );

	/*! Unpack data from a self-describing message.  The format string
	 * is not strictly necessary, but it verifies that the message
	 * contains what the caller expects.  If the message doesn't match
	 * the expected format, a message is printed on stderr, and the
	 * function returns without processing the message any further.
	 * The user must pass the correct pointer types to contain the
	 * values.  Strings are NOT copied; instead, a pointer is set to
	 * a location in buf.  The return value is a pointer to the next
	 * unprocessed character in the buffer.  (If a format character
	 * mismatch is found, the pointer points at the mismatched
	 * character in the buffer.)
	 */
	char * TG_unpack( char * buffer, const char * format, ... );

#endif /* TG_PACK_H */
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

