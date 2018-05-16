//! \file inttoindex.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
#ifndef INTTOINDEX_H
#define INTTOINDEX_H

#include <index_symbol.h>

/****************************************************************************
**
** A quick and dirty C++ wrapper that maps an int to another int (typically
** used as an index, thus the name).
** Modified to use a hacked int_symbol.[ch], named index_symbol.[ch] that
** stores ints instead of pointers (so I didn't have to do (void *)(long)
** cast hack).
**
** Based on Initial intsymbol.h implementation by John Gyllenhaal 2/20/01
** Inttoindex version created by John Gyllenhaal 2/3/04
** Removed pointer hack (by using index_symbol.[ch]) 4/06 by John Gyllenhaal
**
*****************************************************************************/
class IntToIndex
{
public:
    //! Creates table with the given name and initially optimized
    //! for the expected size (dynamically resizes, so 0 ok).
    //! Set not_found_value to what you want returned when int not found,
    //! defaults to -1.
    IntToIndex(const char *name, int not_found_value = -1, 
	       int expected_size = 0)
    {
	table = INDEX_new_symbol_table (name, expected_size);
	notFoundValue = not_found_value;
    }
    ~IntToIndex()
    {
	// Delete the symbol table, don't use it's free routine!
	// The pointers are not valid (random ints)
	INDEX_delete_symbol_table (table);
    }
    //! Add entry indexed with int key
    //! Will punt if key already in table!
    inline void addEntry (int key, int data)
    {
	INDEX_add_symbol (table, key, data);
    }

    //! Find entry with int key
    //! Returns pointer to data associated with key, or NULL if not found.
    inline int findEntry (int key) const
    {
	// Find entry, if exists
	INDEX_Symbol *symbol = INDEX_find_symbol (table, key);
				
	// If exists, return int value
	if (symbol != NULL)
	{
	    return (symbol->data);
	}
	// Otherwise, return not found value
	return (notFoundValue);
    }

    //! Determine if entry in table using int key
    //! Returns TRUE if key found in table, FALSE otherwise
    inline bool entryExists (int key) const
    {
	return (INDEX_find_symbol (table, key) != NULL);
    }

    //! Removes entry for the given int key.
    //! Does nothing if keys not found in table
    inline void deleteEntry (int key)
    {
	INDEX_Symbol *symbol = INDEX_find_symbol (table, key);
							  
	if (symbol != NULL)
	{
	    INDEX_delete_symbol (symbol);
	}
    }

    //! Removes all entries from the table
    inline void deleteAllEntries()
    {
	INDEX_Symbol *symbol, *next_symbol;
	
	/* For all the symbols in the table, free each one */
	for (symbol = table->head_symbol; symbol != NULL;
	     symbol = next_symbol)
	{
	    /* Get the next symbol before deleting this one */
	    next_symbol = symbol->next_symbol;
	    
	    /* Delete the symbol */
	    INDEX_delete_symbol (symbol);
	}
    }

    //! Updates table's estimated size so on the next table resize, the
    //! resized table will be big enough after just one resize (instead of
    //! several).
    inline void tweakTableResize (int new_estimated_size)
    {
	INDEX_tweak_table_resize (table, new_estimated_size);
    }

private:
    INDEX_Symbol_Table *table;
    int notFoundValue;
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

