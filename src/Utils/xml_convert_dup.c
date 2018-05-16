/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/* A XML utility function that converts a C string that may contain forbidden 
 * XML characters '<', '&', '* '>', '\'', and '"' into their XML 
 * representations '&lt;', etc.  Should be run on a string exactly once that
 * is being written out to XML.   If you do it twice, you will end up with
 * hard to read gibberish as the replacements will happen twice.
 *
 * Behaves like strdup(), must free() pointer returned to prevent leaks!
 * Returnes NULL if out of memory.
 *
 * Created by John Gyllenhaal 04/06.
 */
#include "xml_convert_dup.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *XML_convert_dup (const char *string_to_convert)
{
    const char *scan_ptr = string_to_convert;
    char *converted_string, *write_ptr;
    int ch;
    int convert_len = 0, diff_len;
    
    /* Calculate length of converted string after replacement of
     * the special characters with XML escape sequences
     */
    while ((ch = *scan_ptr) != 0)
    {
	/* Replace '<' with "&lt;" */
	if (ch == '<')
	    convert_len+=4;

	/* Replace '&' with "&amp;" */
	else if (ch == '&')
	    convert_len+=5;

	/* Replace '>' with "&gt;" */
	else if (ch == '>')
	    convert_len+=4;

	/* Replace "'" with "&apos;" */
	else if (ch == '\'')
	    convert_len+=6;

	/* Replace '"' with "&quot;" */
	else if (ch == '"')
	    convert_len+=6;
	
	/* Otherwise, just one character needed in new string */
	else
	    convert_len+=1;

	/* Goto next character */
	scan_ptr++;
    }

    /* Allocate string big enough to hold converted length + 1 for terminator*/
    converted_string = (char *)malloc (convert_len + 1);
    
    /* Return NULL if malloc failed */
    if (converted_string == NULL)
	return (NULL);

    /* Get pointers to original and converted string */
    scan_ptr = string_to_convert;
    write_ptr = converted_string;

    /* Now scan over original string and do copy and convert */
    while ((ch = *scan_ptr) != 0)
    {
	/* Replace '<' with "&lt;" */
	if (ch == '<')
	{
	    write_ptr[0] = '&';
	    write_ptr[1] = 'l';
	    write_ptr[2] = 't';
	    write_ptr[3] = ';';
	    write_ptr+=4;
	}

	/* Replace '&' with "&amp;" */
	else if (ch == '&')
	{
	    write_ptr[0] = '&';
	    write_ptr[1] = 'a';
	    write_ptr[2] = 'm';
	    write_ptr[3] = 'p';
	    write_ptr[4] = ';';
	    write_ptr+=5;
	}

	/* Replace '>' with "&gt;" */
	else if (ch == '>')
	{
	    write_ptr[0] = '&';
	    write_ptr[1] = 'g';
	    write_ptr[2] = 't';
	    write_ptr[3] = ';';
	    write_ptr+=4;
	}

	/* Replace "'" with "&apos;" */
	else if (ch == '\'')
	{
	    write_ptr[0] = '&';
	    write_ptr[1] = 'a';
	    write_ptr[2] = 'p';
	    write_ptr[3] = 'o';
	    write_ptr[4] = 's';
	    write_ptr[5] = ';';
	    write_ptr+=6;
	}

	/* Replace '"' with "&quot;" */
	else if (ch == '"')
	{
	    write_ptr[0] = '&';
	    write_ptr[1] = 'q';
	    write_ptr[2] = 'u';
	    write_ptr[3] = 'o';
	    write_ptr[4] = 't';
	    write_ptr[5] = ';';
	    write_ptr+=6;
	}
	
	/* Otherwise, just one character needed in new string */
	else
	{
	    write_ptr[0] = ch;
	    write_ptr+=1;
	}
	
	/* Goto next character to read*/
	scan_ptr++;
    }

    /* Place terminator on end of converted string */
    write_ptr[0] = 0;

    /* Sanity check, make sure calculated length equals written length */
    diff_len = write_ptr - converted_string;
    if (diff_len != convert_len)
    {
	fprintf (stderr, "Error in XML_convert_dup (): "
		 "Calculated len %i != written len %i!\n"
		 " Source: '%s'\n"
		 " Dest:   '%s'\n",
		 convert_len, diff_len, 
		 string_to_convert, 
		 converted_string);
	exit (1);
    }

#if 0
    /* DEBUG */
    if ((((int)strlen(string_to_convert)) != diff_len) ||
	(strcmp (string_to_convert, converted_string) != 0))
    {
	printf ("Converted: %4i: '%s'\n"
		"       To: %4i: '%s'\n",
		(int)strlen(string_to_convert), 
		string_to_convert, 
		diff_len, 
		converted_string);
    }
#endif
	    
    /* Return the malloced(), converted string */
    return (converted_string);
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

