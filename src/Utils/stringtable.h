//! \file stringtable.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/*!***************************************************************************
**
** Hopefully efficient C++ wrapper around a subset of the C library 
** string_symbol.[ch].
** 
** I got tired of putting Cisms in my C++ code, and this should also
** make it easier if we need to switch to a different hash table
** implementation in the future.  
**
** For now, using templates to get rid of void * stuff.  Add some
** straightforward wrappers that I need now.  May need to think this
** interface thru more later.
**
** Initial implementation by John Gyllenhaal 2/20/01
**
*****************************************************************************/
#ifndef STRINGTABLE_H
#define STRINGTABLE_H

#include "tabledef.h"
#include "string_symbol.h"
#include "tg_error.h"
#include <stdlib.h>  // For free()

template<class T> class StringTable
{
public:
    //! Creates table with the given name and initially optimized
    //! for the expected size (dynamically resizes, so 0 ok).
    //! If use_dealloc_type is not NoDealloc, the type of deallocator
    //! specified (DeleteData, DeleteArrayData, or FreeData)
    //! will be called when the symbol or table is deleted.
    StringTable(const char *name, 
		TableDealloc use_dealloc_type = NoDealloc, 
		int expected_size = 0)
    {
	table = STRING_new_symbol_table (name, expected_size);
	dealloc_type = use_dealloc_type;
    }

    ~StringTable()
    {
	// Free data pointed to, if user requested this
	if (dealloc_type != NoDealloc)
	{
	    // Delete data based on deallocation type
	    STRING_Symbol *symbol;

	    //
	    // Do each deallocation type separately for efficiency
	    //

	    // Handle delete case
	    if (dealloc_type == DeleteData)
	    {
		// Loop though each symbol, delete pointer, and 
		// reset pointer to NULL (for sanity).
		for (symbol = table->head_symbol; symbol != NULL;
		     symbol = symbol->next_symbol)
		{
		    // Get pointer to data
		    T *ptr = (T *)symbol->data;
		    
		    // Null out pointer to data
		    symbol->data = NULL;
		    
		    // Delete the data
		    delete ptr;
		}
	    }

	    // Handle delete[] case
	    else if (dealloc_type == DeleteArrayData)
	    {
		// Loop though each symbol, delete[] pointer, and 
		// reset pointer to NULL (for sanity).
		for (symbol = table->head_symbol; symbol != NULL;
		     symbol = symbol->next_symbol)
		{
		    // Get pointer to data
		    T *ptr = (T *)symbol->data;
		    
		    // Null out pointer to data
		    symbol->data = NULL;

		    // Delete[] the data
		    delete[] ptr;
		}
	    }

	    // Handle free() case
	    else if (dealloc_type == FreeData)
	    {
		// Loop though each symbol, free pointer, and 
		// reset pointer to NULL (for sanity).
		for (symbol = table->head_symbol; symbol != NULL;
		     symbol = symbol->next_symbol)
		{
		    // Get pointer to data
		    void *ptr = symbol->data;
		    
		    // Null out pointer to data
		    symbol->data = NULL;

		    // free() the data
		    free(ptr);
		}
	    }

	    // Handle we got something unexpected case
	    else
	    {
		TG_error ("StringTable::~StringTable:: "
			  "Unknown TableDealloc type '%i'\n", 
			  dealloc_type);
	    }
	}

	// Delete the symbol table, don't use it's free routine!
	STRING_delete_symbol_table (table, NULL);
    }

    //! Add entry indexed with char * key
    //! Will punt if key already in table!
    inline void addEntry (const char *key, T* data)
    {
	STRING_add_symbol (table, key, (void *)data);
    }

    //! Find entry with const char * key
    //! Returns pointer to data associated with key, or NULL if not found.
    inline T *findEntry (const char *key) const
    {
	return ((T *)STRING_find_symbol_data (table, key));
    }


    //! Determine if entry in table using const char * key
    //! Returns TRUE if key found in table, FALSE otherwise
    inline bool entryExists (const char *key) const
    {
	return (STRING_find_symbol (table, key) != NULL);
    }

    //! Removes entry for the given const char * key.
    //! Does nothing if keys not found in table
    inline void deleteEntry (const char *key)
    {
	STRING_Symbol *symbol = STRING_find_symbol (table, key);
							  
	if (symbol != NULL)
	{
	    if (dealloc_type != NoDealloc)
	    {
		T *ptr = (T *)symbol->data;
		if (ptr != NULL)
		{
		    if (dealloc_type == DeleteData)
		    {
			delete ptr;
		    }
		    else if (dealloc_type == DeleteArrayData)
		    {
			delete[] ptr;
		    }
		    else if (dealloc_type == FreeData)
		    {
			free((void *)ptr);
		    }
		    else
		    {
			TG_error ("StringTable::deleteEntry:: "
				  "Unknown TableDealloc type '%i'\n", 
				  dealloc_type);
		    }

		    symbol->data = NULL; // Set to NULL for sanity
		}
	    }
	    STRING_delete_symbol (symbol, NULL);
	}
    }

    //! Deletes all entries in the table 
    inline void deleteAllEntries ()
    {
	STRING_Symbol *symbol;

	while ((symbol = table->head_symbol) != NULL)
	{
	    // Free data pointed to, if user requested this
	    if (dealloc_type != NoDealloc)
	    {
		T *ptr = (T *)symbol->data;
		if (ptr != NULL)
		{
		    if (dealloc_type == DeleteData)
		    {
			delete ptr;
		    }
		    else if (dealloc_type == DeleteArrayData)
		    {
			delete[] ptr;
		    }
		    else if (dealloc_type == FreeData)
		    {
			free((void *)ptr);
		    }
		    else
		    {
			TG_error ("StringTable::deleteAllEntries:: "
				  "Unknown TableDealloc type '%i'\n", 
				  dealloc_type);
		    }
		}
		symbol->data = NULL; // Set to NULL for sanity
	    }
	    STRING_delete_symbol (symbol, NULL);
	}
    }


private:
    STRING_Symbol_Table *table;
    TableDealloc dealloc_type;
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

