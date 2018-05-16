//! \file intset.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** A quick and dirty int set implementation that only can currently
** can tell if an int is in the set or not (and add/remove ints).  
** Relatively high initial space consumption (~256 bytes) but should be better 
** than inttable.h for large number of closely packed ints.  
**
** Currently an inttable that holds 64-bit char arrays that is indexed
** by int value >> 6.
** 
** Initial implementation by John Gyllenhaal 2/25/02
**
*****************************************************************************/
#ifndef INTSET_H
#define INTSET_H

#include <inttable.h>


//! Allows integers to be added/removed to set and queries about
//! the presence of an integer in the set.  Quick and dirty implementation
//! based on IntArray that is more effient than using IntArray directly
//! to get this functionality.
//!
//! All queries add(), remove(), and in(), return the state of the
//! set before the call.  That is, TRUE, if value in set before call.
class IntSet
{
public:
    IntSet(const char *name = "IntSet") : 
	table(name, DeleteData, 0), numInts(0) {}
    
    //! Types of requests (must be public to be usable) 
    enum IntSetRequest { AddToIntSet, RemoveFromIntSet, InIntSet};

    //! Adds int to set, returns TRUE if was already in set
    bool add (int value) {return (processRequest(value, AddToIntSet));}
    
    //! Removes int from set, return TRUE if was in set before clearing
    bool remove (int value) {return (processRequest(value, RemoveFromIntSet));}

    //! Returns TRUE if value in set, FALSE otherwise
    bool in (int value) {return (processRequest(value, InIntSet));}

    //! Returns number of integers in set
    int count () {return (numInts);}

protected:

    //! Private Bit array for IntSet.  Initially set to all zeros
    struct IntSetBitArray
    {
	IntSetBitArray() {for (int i=0; i < 8;i++) {bits[i]=0;}}
	unsigned char bits[8];
    };

    
    //! The routine that actually does all the work
    bool processRequest (int value, IntSetRequest type)
	{
	    // 8 byte entry for each table entry (divide by 64 == >> 6)
	    int tableIndex = value >> 6;
	    
	    // Mask off all but lower bits (so value 0-63)
	    int maskedIndex = value & 0x3F;

	    // 8 bits per char ( divide by 8 == >> 3)
	    int byteIndex = maskedIndex >> 3;

	    // Mask off all but lower bits (so value 0-7)
	    int bitOffset = maskedIndex & 0x7;

	    // Get 1 in proper bit position for bit to test
	    int bitMask = 1 << bitOffset;
	    
	    // Get bit array for tableIndex, if exists
	    IntSetBitArray *array = table.findEntry(tableIndex);

	    // Handle various cases if entry doesn't exist
	    if (array == NULL)
	    {
		// If adding int, add bit array for tableIndex 
		if (type == AddToIntSet)
		{
		    array = new IntSetBitArray;
		    table.addEntry(tableIndex, array);
		}
		// Otherwise, for remove and query, return FALSE
		// but was not in set before request processed
		else
		{
		    return (0);
		}
	    }
	    
	    // Is bit set for this value?
	    if ((array->bits[byteIndex] & bitMask) == 0)
	    {
		// No, if adding, set the bit, etc.
		if (type == AddToIntSet)
		{
		    // Set bit (with or) and increment count
		    array->bits[byteIndex] |= bitMask;
		    numInts++;
		}

		// Return false for all requests (was not set before)
		return (0);
	    }
	    else
	    {
		// Yes, if clearing, clear the bit, etc.
		if (type == RemoveFromIntSet)
		{
		    // Clear bit (with xor), decrement count
		    array->bits[byteIndex] ^= bitMask;
		    numInts--;
		    
		}

		// Return true for all request (was set before)
		return (1);
	    }
	}

private:
    IntTable<IntSetBitArray> table;
    int numInts;
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

