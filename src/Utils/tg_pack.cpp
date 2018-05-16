/* tg_pack.c */
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

/* John May, 26 October 2000 */
/* Packs data into a supplied buffer. */
/* MessageBuffer version by John Gyllenhaal, 2 April 2004 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "tg_pack.h"
#include "tg_time.h"
#include "tg_error.h"
#include <memory.h>

#define INT_CHARS 10
#define DOUBLE_CHARS 30
#define DOUBLE_PREC 16

/* Pack data into a self-describing buffer supplied by the
 * user.  No more than length characters will be packed.  The
 * actual number of bytes packed is returned.  The format is
 * a \0 terminated string containing the characters S, I, and D,
 * descrbing the types of the values to be packed.
 */
unsigned TG_pack( char * buf, unsigned length, const char * format, ... )
{
#if 1
    // DEBUG, test out MessageBuffer version by using it for everything!
    MessageBuffer mbuf;
    va_list ap;

    // Use the TG_vpack version to do heavy lifting
    va_start(ap, format);
    unsigned len = TG_vpack (mbuf, format, ap);
    va_end(ap);

    // If fits in length, copy over to buf
    if (len <= length)
	memcpy (buf, (const void *)mbuf.contents(),  len);

    // Otherwise, punt (instead of returning partial message)
    else
    {
	TG_error ("TG_pack(): Buffer length (%i) exceeded by %i byte message!",
		  length, len);
    }

    // Return length
    return (len);
#else

        // Use the original implementation of TG_pack
	const char * pfmt;
	char * pbuf;
	char * plength;
	char * sval;
	int string_start;
	va_list ap;
	unsigned position = 0;

	va_start( ap, format );

	/* Decide whether to traverse the args and figure out the
	 * length, then copy in to buffer; allocate buffer ont
	 * the file, pass in a max buffer size or what??  Using
	 * a static buffer is not thread safe and a bad idea here.
	 */
	pbuf = buf;
	for( pfmt = format; *pfmt; pfmt++ ) {
		switch( toupper( *pfmt ) ) {
		/* String: format is S<length in ascii><string with \0> */
		/* Leave a blank space for the length and then go back
		 * and fill it in; that way we traverse the input string
		 * only once.
		 */
		case 'S':
			/* Be sure there's room at least for 'S', length,
			 * and a \0.
			 */
			if( length - position <
					2 * sizeof(char) + INT_CHARS ) {
				va_end( ap );
				return position;
			}
			*pbuf++ = 'S';
			plength = pbuf;
			pbuf += INT_CHARS;	/* leave some space */
			position += sizeof(char) + INT_CHARS;
			string_start = position;
			sval = va_arg( ap, char *);

			/* Copy string until we've done the \0 or until
			 * we run out of space in the buffer; if so,
			 * terminate with \0 and return.  If string is
			 * NULL, just store the zero length.
			 */
			if( sval != NULL ) {
				do {
					if( ++position == length ) {
						/* out of space; fill in
						 * \0 and stop; length
						 * should still be stored
						 * correctly below.
						 */
						*pbuf = '\0';
						break;
					}
					*pbuf++ = *sval;
				} while( *sval++ != '\0' );
			} 

			/* Fill in the length of the string */
			sprintf( plength, "%*d", INT_CHARS-1,
					position - string_start );
			break;
		/* Integer: format is I<integer in ascii> */
		case 'I':
			/* Be sure there's room for 'I' and value */
			if( length - position < sizeof(char) + INT_CHARS ) {
				va_end( ap );
				return position;
			}
			*pbuf++ = 'I';
			sprintf( pbuf, "%*d", INT_CHARS-1, va_arg( ap, int ) );
			pbuf += INT_CHARS;
			position += INT_CHARS + sizeof(char);
			break;
		/* Double: format is D<double in ascii> */
		case 'D':
			/* Be sure there's room for 'D' and value */
			if( length - position <
					sizeof(char) + DOUBLE_CHARS ) {
				va_end( ap );
				return position;
			}
			*pbuf++ = 'D';
			sprintf( pbuf, "%*.*e", DOUBLE_CHARS-1,
					DOUBLE_PREC, va_arg( ap, double ) );
			pbuf += DOUBLE_CHARS;
			position += DOUBLE_CHARS + sizeof(char);
			break;
		default:
			fprintf( stderr, "Unknown format %c in TG_pack!\n",
					*pfmt );
		}
	}

	va_end( ap );
	return position;
#endif
}

/* A MessageBuffer version of the above routine that cannot overflow
 * the buffer (thus no maxLength).  Returns length of message.
 */
unsigned TG_pack( MessageBuffer &mbuf, const char * format, ... )
{
    va_list ap;

    // Use the TG_vpack version to do heavy lifting
    va_start(ap, format);
    unsigned len = TG_vpack (mbuf, format, ap);
    va_end(ap);

    // Return length
    return (len);
}

/* A MessageBuffer va_list version to provide a single implementation
 * of the MessageBuffer version to be used by several calls
 */
unsigned TG_vpack( MessageBuffer &mbuf, const char * format, va_list ap )
{
    char *sval;
    int slen, ival;
    double dval;
    
    // Clear current contents of mbuf, so can just append everything
    mbuf.clear();
    
    /* Traverse arguments, appending appropriate contents to mbuf */
    for( const char *pfmt = format; *pfmt; pfmt++ ) 
    {
	switch( toupper( *pfmt ) ) 
	{
	    /* String: format is S<length in ascii>\0<string with \0> */
	  case 'S':
	    // Get the next argument, a char *
	    sval = va_arg( ap, char *);

	    // If string is null, just store the zero length.
	    if( sval == NULL ) {
		    mbuf.appendSprintf ("S%*d%c", INT_CHARS-1, 0, '\0' );
		    break;
	     }
	    
	    // Get the length
	    slen = strlen (sval) + 1;
	    
	    // Append packed argument to mbuf
	    // Note: had to use %c to get '\0' into the mbuf properly
	    mbuf.appendSprintf ("S%*d%c%s%c", INT_CHARS-1, slen, '\0',
				sval, '\0');
	    break;
	    
	    /* Integer: format is I<integer in ascii>\0 */
	  case 'I':
	    // Get the next argument, a int
	    ival = va_arg( ap, int);

	    // Append packed argument to mbuf
	    // Note: had to use %c to get '\0' into the mbuf properly
	    mbuf.appendSprintf ("I%*d%c", INT_CHARS-1, ival, '\0');
	    break;

	    /* Double: format is D<double in ascii>\0 */
	  case 'D':
	    // Get the next argument, a double
	    dval = va_arg( ap, double);

	    // Append packed argument to mbuf
	    // Note: had to use %c to get '\0' into the mbuf properly
	    mbuf.appendSprintf ("D%*.*e%c", DOUBLE_CHARS-1, DOUBLE_PREC, dval,
				'\0');
	    break;

	    /* Array of strings: This format expects two values, a nonnegative
	     * integer, N, and a char *.  The char * points to a sequence
	     * of N  \0 terminated strings.  These are packed as
	     * A<N>\0<N strings>.  Each string is stored without any
	     * additional formatting (i.e., no 'S' or length info).
	     */
	  case 'A':
	    // Get the number of args
	    ival = va_arg( ap, int );
	    if( ival < 0 ) {
		    TG_error("Invalid number of strings (%d) for packing with "
			   "A format.  Must be nonnegative!\n", ival);
		    break;
	    }

	    // Now get the argument string
	    sval = va_arg( ap, char *);

	    // If string is null, then we have should zero args; otherwise
	    // it's an error.
	    if( sval == NULL ) {
		    if( ival == 0 ) {
			    mbuf.appendSprintf ("A%*d%c", INT_CHARS-1, 0,
					    '\0' );
	 	    } else {
			    TG_error("Trying to pack %d string in A format "
				"but got a null pointer to string list!\n",
				ival );
		    }
	    	    break;
	    }
	    
	    // Store the preamble listing the number of strings
	    mbuf.appendSprintf ("A%*d%c", INT_CHARS-1, ival, '\0');

	    // Store each of the strings
	    int i;
	    char * p;
	    for( i = 0, p = sval; i < ival; p += slen, ++i ) {
		    mbuf.appendSprintf ("%s%c", p, '\0' );
		    slen = strlen (p) + 1;
	    }

	    // No extra termination follows the strings for an arg list
	    break;


	  default:
	    TG_error("Unknown format %c in TG_pack(MessageBuffer)!\n", *pfmt);
	}
    }
    
    // Return the length of the message buffer
    return (mbuf.strlen());
}


/* Unpack data from a self-describing message.  The format string
 * is not strictly necessary, but it verifies that the message
 * contains what the caller expects.  The user must pass in the
 * correct pointer types to contain the values.  Strings are NOT
 * copied; a pointer into the buffer is returned instead.
 */
char * TG_unpack( char * buf, const char * format, ... )
{
	va_list ap;
	unsigned length;
	double * dval;
	int * ival;
	char ** sval;
	char * pbuf;
	const char * pfmt;

	va_start( ap, format );

	pbuf = buf;
	for( pfmt = format; *pfmt; pfmt++ ) {
		switch( toupper( *pfmt ) ) {
		case 'S':
			if( *pbuf++ != 'S' ) {
			    TG_error("TG_unpack format mismatch: "
				     "expected 'S' (format '%s') but message "
				     "contained '%c' (%i)\n"
				     "First portion of message '%s'", 
				     format, *--pbuf,
				     *pbuf, buf);
			    va_end( ap );
			    return pbuf;
			}
			length = atoi( pbuf );
			pbuf += INT_CHARS;
			sval = va_arg( ap, char **);
			if( length == 0 ) {
				*sval = NULL;
			} else {
				*sval = pbuf;
			}
			pbuf += length;
			break;
		case 'I':
			if( *pbuf++ != 'I' ) {
			    TG_error("TG_unpack format mismatch: "
				     "expected 'I' but message "
				     "contained '%c'\n", *--pbuf );
			    va_end( ap );
			    return pbuf;
			}
			ival = va_arg( ap, int *);
			*ival = atoi( pbuf );
			pbuf += INT_CHARS;
			break;
		case 'D':
			if( *pbuf++ != 'D' ) {
			    TG_error( "TG_unpack format mismatch: "
				      "expected 'I' but message "
				      "contained '%c'\n", *--pbuf );
			    va_end( ap );
			    return pbuf;
			}
			dval = va_arg( ap, double *);
			*dval = atof( pbuf );
			pbuf += DOUBLE_CHARS;
			break;
		case 'A':
			if( *pbuf++ != 'A' ) {
			    TG_error( "TG_unpack format mismatch: "
				      "expected 'A' but message "
				      "contained '%c'\n", *--pbuf );
			    va_end( ap );
			    return pbuf;
			}
			/* First get the number of strings in the array */
			ival = va_arg( ap, int * );
			*ival = atoi( pbuf );
			pbuf += INT_CHARS;

			/* Now store the pointer to the first string */
			sval = va_arg( ap, char ** );
			if( *ival == 0 ) {
			    *sval = NULL;
			} else {
			    *sval = pbuf;
			}

			/* Advance the buffer pointer past the last string */
			int i;
			for( i = 0; i < *ival; ++i ) {
			    pbuf += strlen( pbuf) + 1;
			}
			break;

		default:
		  TG_error("Unknown format %c in TG_unpack!\n",
			   *pfmt );
		}
	}

	va_end( ap );

	return pbuf;
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

