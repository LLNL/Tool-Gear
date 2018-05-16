/* IMPACT Public Release (www.crhc.uiuc.edu/IMPACT)            Version 2.31  */
/* IMPACT Trimaran Release (www.trimaran.org)                  June 21, 1999 */
/*****************************************************************************\
 * LICENSE AGREEMENT NOTICE
 * 
 * IT IS A BREACH OF THIS LICENSE AGREEMENT TO REMOVE THIS NOTICE FROM
 * THE FILE OR SOFTWARE, OR ANY MODIFIED VERSIONS OF THIS FILE OR
 * SOFTWARE OR DERIVATIVE WORKS.
 * 
 * ------------------------------
 * 
 * Copyright Notices/Identification of Licensor(s) of 
 * Original Software in the File
 * 
 * Copyright 1990-1999 The Board of Trustees of the University of Illinois
 * For commercial license rights, contact: Research and Technology
 * Management Office, University of Illinois at Urbana-Champaign; 
 * FAX: 217-244-3716, or email: rtmo@uiuc.edu
 * 
 * All rights reserved by the foregoing, respectively.
 * 
 * ------------------------------
 * 	
 * Copyright Notices/Identification of Subsequent Licensor(s)/Contributors 
 * of Derivative Works
 * 
 * Copyright  <Year> <Owner>
 * <Optional:  For commercial license rights, contact:_____________________>
 * 
 * 
 * All rights reserved by the foregoing, respectively.
 * 
 * ------------------------------
 * 
 * The code contained in this file, including both binary and source 
 * [if released by the owner(s)] (hereafter, Software) is subject to
 * copyright by the respective Licensor(s) and ownership remains with
 * such Licensor(s).  The Licensor(s) of the original Software remain
 * free to license their respective proprietary Software for other
 * purposes that are independent and separate from this file, without
 * obligation to any party.
 * 
 * Licensor(s) grant(s) you (hereafter, Licensee) a license to use the
 * Software for academic, research and internal business purposes only,
 * without a fee.  "Internal business purposes" means that Licensee may
 * install, use and execute the Software for the purpose of designing and
 * evaluating products.  Licensee may submit proposals for research
 * support, and receive funding from private and Government sponsors for
 * continued development, support and maintenance of the Software for the
 * purposes permitted herein.
 * 
 * Licensee may also disclose results obtained by executing the Software,
 * as well as algorithms embodied therein.  Licensee may redistribute the
 * Software to third parties provided that the copyright notices and this
 * License Agreement Notice statement are reproduced on all copies and
 * that no charge is associated with such copies.  No patent or other
 * intellectual property license is granted or implied by this Agreement,
 * and this Agreement does not license any acts except those expressly
 * recited.
 * 
 * Licensee may modify the Software to make derivative works (as defined
 * in Section 101 of Title 17, U.S. Code) (hereafter, Derivative Works),
 * as necessary for its own academic, research and internal business
 * purposes.  Title to copyrights and other proprietary rights in
 * Derivative Works created by Licensee shall be owned by Licensee
 * subject, however, to the underlying ownership interest(s) of the
 * Licensor(s) in the copyrights and other proprietary rights in the
 * original Software.  All the same rights and licenses granted herein
 * and all other terms and conditions contained in this Agreement
 * pertaining to the Software shall continue to apply to any parts of the
 * Software included in Derivative Works.  Licensee's Derivative Work
 * should clearly notify users that it is a modified version and not the
 * original Software distributed by the Licensor(s).
 * 
 * If Licensee wants to make its Derivative Works available to other
 * parties, such distribution will be governed by the terms and
 * conditions of this License Agreement.  Licensee shall not modify this
 * License Agreement, except that Licensee shall clearly identify the
 * contribution of its Derivative Work to this file by adding an
 * additional copyright notice to the other copyright notices listed
 * above, to be added below the line "Copyright Notices/Identification of
 * Subsequent Licensor(s)/Contributors of Derivative Works."  A party who
 * is not an owner of such Derivative Work within the meaning of
 * U.S. Copyright Law (i.e., the original author, or the employer of the
 * author if "work of hire") shall not modify this License Agreement or
 * add such party's name to the copyright notices above.
 * 
 * Each party who contributes Software or makes a Derivative Work to this
 * file (hereafter, Contributed Code) represents to each Licensor and to
 * other Licensees for its own Contributed Code that:
 * 
 * (a) Such Contributed Code does not violate (or cause the Software to
 * violate) the laws of the United States, including the export control
 * laws of the United States, or the laws of any other jurisdiction.
 * 
 * (b) The contributing party has all legal right and authority to make
 * such Contributed Code available and to grant the rights and licenses
 * contained in this License Agreement without violation or conflict with
 * any law.
 * 
 * (c) To the best of the contributing party's knowledge and belief,
 * the Contributed Code does not infringe upon any proprietary rights or
 * intellectual property rights of any third party.
 * 
 * LICENSOR(S) MAKE(S) NO REPRESENTATIONS ABOUT THE SUITABILITY OF THE
 * SOFTWARE OR DERIVATIVE WORKS FOR ANY PURPOSE.  IT IS PROVIDED "AS IS"
 * WITHOUT EXPRESS OR IMPLIED WARRANTY, INCLUDING BUT NOT LIMITED TO THE
 * MERCHANTABILITY, USE OR FITNESS FOR ANY PARTICULAR PURPOSE AND ANY
 * WARRANTY AGAINST INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * LICENSOR(S) SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY THE USERS
 * OF THE SOFTWARE OR DERIVATIVE WORKS.
 * 
 * Any Licensee wishing to make commercial use of the Software or
 * Derivative Works should contact each and every Licensor to negotiate
 * an appropriate license for such commercial use, and written permission
 * of all Licensors will be required for such a commercial license.
 * Commercial use includes (1) integration of all or part of the source
 * code into a product for sale by or on behalf of Licensee to third
 * parties, or (2) distribution of the Software or Derivative Works to
 * third parties that need it to utilize a commercial product sold or
 * licensed by or on behalf of Licensee.
 * 
 * By using or copying this Contributed Code, Licensee agrees to abide by
 * the copyright law and all other applicable laws of the U.S., and the
 * terms of this License Agreement.  Any individual Licensor shall have
 * the right to terminate this license immediately by written notice upon
 * Licensee's breach of, or non-compliance with, any of its terms.
 * Licensee may be held legally responsible for any copyright
 * infringement that is caused or encouraged by Licensee's failure to
 * abide by the terms of this License Agreement.  
\*****************************************************************************/
/*****************************************************************************\
 *      File:   md.c
 *
 *      Description: The interface to the internal representation of the
 *                   IMPACT Meta-Description Language
 *
 *      Creation Date:  March 1995
 *
 *      Authors: John C. Gyllenhaal and Wen-mei Hwu
 *
 *      Revisions:
 *           Shail Aditya (HP Labs)  February 1996
 *           Changed MD_get_xxx() arguments to be consistent with MD_set_xxx()
 *           Added MD_num_xxx() which returns number of sections, entries, etc.
 *
 *           John C. Gyllenhaal  May 1996
 *           Folded Shail's changes back into IMPACT's version
 *           Changed low-level representation to use hex for integers
 *           Added MD_rename_entry(), MD_delete_element(), MD_max_field_index()
 *                 MD_max_element_index, and MD_print_md_declarations()
 *           Changed all error messages to uniformly use MD_punt()
 *
 * 	     John C. Gyllenhaal July 1996
 *	     Implemented Shail Aditya's suggestion of adding void pointers to
 *           MD_Section and MD_Entry for the user's use.  Added functions
 *           MD_set_section_ext(), MD_get_section_ext(), MD_set_entry_ext(), 
 *           and MD_get_entry_ext().
 *
 *           John C. Gyllenhaal January 1998
 *           Implemented Marie Conte's and Andy Trick's suggestion of adding
 *           a new data type 'BLOCK' to allow an user to specify a block
 *           of binary data the same way ints, doubles, strings, and links
 *           are specified in fields.  (User readable input/output 
 *           respresents this data as a hex string.  Note that this data type
 *           may not make sense to programs running on different platforms or
 *           compiled with different compilers!)
 * 	     Also, switched to internal version of strdup (MD_strdup) in order
 *           to handle 'out of memory' errors more gracefully.
 *
 *           John C. Gyllenhaal August 2002
 *           Changed most 'char *' to 'const char *' in function parameters
 *           to make more C++ const friendly.  Did not change anything else
 *           in order to maintain backward-compatibility with code that 
 *           doesn't use const everywhere.
 *
 *           John C. Gyllenhaal March 2004
 *           Modified MD_read_string and added a new field string marker 'N'
 *           that allows newlines in a string element.   To maintain backward
 *           compatibility, the new marker 'N' is only used when newlines are
 *           present and the older string marker 'S' is used in all other
 *           cases.
 *
 * *
\*****************************************************************************/


/* Turn on debugging macros */
#define MD_DEBUG_MACROS 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md.h"
#include <stdarg.h>
#include <ctype.h>
#include "l_alloc_new.h"


/* Internal buffer structure used by MD_read_md(),
 *  MD_read_double(), MD_read_string().
 */
typedef struct MD_Buf
{
    char	*buf;
    int		buf_size;
} MD_Buf;

/* Internal pool used by read routines only */
static L_Alloc_Pool *MD_Buf_pool = NULL;

/* L_Alloc pool declarations */
static L_Alloc_Pool *MD_pool = NULL;
static L_Alloc_Pool *MD_Symbol_Table_pool = NULL;
static L_Alloc_Pool *MD_Symbol_pool = NULL;
static L_Alloc_Pool *MD_Section_pool = NULL;
static L_Alloc_Pool *MD_Entry_pool = NULL;
static L_Alloc_Pool *MD_Field_Decl_pool = NULL;
static L_Alloc_Pool *MD_Element_Req_pool = NULL;
static L_Alloc_Pool *MD_Field_pool = NULL;
static L_Alloc_Pool *MD_Element_pool = NULL;

/* Create a type array for error checking */
static char *MD_type_name[] = {"(UNDEFINED)", "INT", "DOUBLE", "STRING", 
			       "LINK", "BLOCK", "(UNDEFINED)"};

/* Prototypes of interal static routines */
static void MD_resize_field_arrays (MD_Section *section, int max_index);
static void MD_resize_element_array (MD_Field *field, int max_index);
static int MD_legal_ident (const char *ident);


/*
 * Internal md library routines.  
 * Should not be used by non-library routines.
 */

/* Error routine, fmt and trailing args are the same as printf.  
 * Adds header message identifying error as coming from MD_library and
 * which md caused the error.
 * Exits with value 1.
 */
static void MD_punt (MD *md, const char *fmt, ...)
{
    va_list args;

    /* Print error message header.
     * Allow NULL to be passed in for md 
     */
    fprintf (stderr, "\nMD library error");
    if (md == NULL)
	fprintf (stderr, ":\n");
    else
	fprintf (stderr, " (in \"%s\"):\n", md->name);

    /* Print out error message */
    va_start (args, fmt);
    vfprintf (stderr, fmt, args);
    va_end(args);
    fprintf (stderr,"\n");

    exit (1);
}

/* MD warning routine, args are the same as fprintf.  
 * Prints message to outand returns to caller.
 *
 * Provided as a useful place to place a breakpoint when debugging warnings.
 */
static void MD_warn (FILE *out, const char *fmt, ...)
{
    va_list args;

    /* Make sure warning goes on new line */
    fprintf (out, "\n");

    /* Print out warning message */
    va_start (args, fmt);
    vfprintf (out, fmt, args);
    va_end(args);

    /* Automatically add newline to warning */
    fprintf (out,"\n");
}

/* Returns a pointer to a new string which is a duplicate of the
 * string to which 'string' points.  The space for the new string is 
 * obtained using the malloc() function.  If out of memory,
 * an error message using MD_punt will occur.  
 *
 * The library strdup() routine was originally used, but it 
 * didn't handle out of memory errors gracefully.  -JCG 1/15/98
 */
static char *MD_strdup(const char *string)
{
    char *new_string;
    unsigned int len, i;

    /* Get length of string */ 
    len = 0;
    while (string[len] != 0)
    {
	len++;
    }

    /* Malloc buffer (len + 1) to hold string */
    if ((new_string = (char *) malloc (len + 1)) == NULL)
	MD_punt (NULL, "MD_strdup: Out of memory (string length %i).", len);
    
    /* Copy string to new_string, including terminator */
    for (i=0; i <= len; i++)
	new_string[i] = string[i];

    /* Return new copy of string */
    return (new_string);
}

/* MD Symbol table code for md section, entry and field names.
 * Internal MD use only!  Not designed to be used by non-MD functions!
 */
/* Malloc's a string buffer and concatinates the two strings into it.
 * This newly malloced bufffer is returned.
 * 
 * Used by MD to build descriptive names for symbol tables.
 */
static char *MD_concat_strings (const char *string1, const char *string2)
{
    char *buf;
    int len;

    /* The buffer length needed is the length of both strings plus 1 for
     * the terminator.
     */
    len = strlen (string1) + strlen (string2) + 1;
    if ((buf = (char *) malloc (len * sizeof(char))) == NULL)
    {
	MD_punt (NULL,
		 "MD_concat_strings: Out of memory mallocing buffer (size %i)",
		 len);
    }

    /* Concat strings */
    sprintf (buf, "%s%s", string1, string2);

    /* Return concatinated strings */
    return (buf);
}

/* Create and initialize MD_Symbol_Table.
 * Creates a hash table of initial size 2 * expected_size rounded up
 * to the closest power of two.  (Min hash size 32)
 */
static MD_Symbol_Table *MD_new_symbol_table (MD *md, const char *name, 
					     int expected_size)
{
    MD_Symbol_Table *table;
    MD_Symbol **hash;
    unsigned int min_size, hash_size;
    unsigned int i;


    /* If expected size negative, force to be 0 */
    if (expected_size < 0)
	expected_size = 0;

    /* To prevent infinite loop by sizing algorithm (and running out of
     * memory :) ), expected_size must be <= a billion.
     */
    if (expected_size > 1000000000)
	MD_punt (md, "MD_Symbol_Table: unreasonable expected_size (%u)",
		 expected_size);

    /* Want a minumum size of at least twice the expected size */
    min_size = expected_size * 2;

    /* Start with a minumum hash size of 32.  
     * (Smaller sizes don't work as well with the hashing algorithm)
     */
    hash_size = 32;

    /* Double hash_size until min_size is reached or exceeded */
    while (hash_size < min_size)
	hash_size = hash_size << 1;

    
    /* Sanity check (debugging purposes) */
    if (MD_Symbol_Table_pool == NULL)
    {
	MD_punt (NULL, 
		 "MD routines not initialized (call MD_new_md() first)!");
	
    }

    /* Allocate symbol table */
    table = (MD_Symbol_Table *) L_alloc (MD_Symbol_Table_pool);

    /* Allocate array for hash */
    hash = (MD_Symbol **) malloc (hash_size * sizeof(MD_Symbol *));
    if (hash == NULL)
    {
	MD_punt (md, "MD_new_symbol_table: Out of memory, hash array size %i.",
		 hash_size);
    }

    /* Initialize hash table */
    for (i=0; i < hash_size; i++)
        hash[i] = NULL;

    /* Initialize fields */
    table->md = md;
    table->name = MD_strdup (name);
    table->hash = hash;
    table->hash_size = hash_size;
    table->hash_mask = hash_size -1; /* AND mask, works only for power of 2 */
    /* Resize when count at 75% of hash_size */
    table->resize_size = hash_size - (hash_size >> 2);
    table->head_symbol = NULL;
    table->tail_symbol = NULL;
    table->symbol_count = 0;

    return (table);
}

/* Frees the symbol table, and optionally frees the data pointed to */
void MD_delete_symbol_table (MD_Symbol_Table *table, 
			     void (*free_routine)(void *))
{
    MD_Symbol *symbol, *next_symbol;

    /* For all the symbols in the table, free each one */
    for (symbol = table->head_symbol; symbol != NULL;
	 symbol = next_symbol)
    {
	/* Get the next symbol before deleting this one */
	next_symbol = symbol->next_symbol;

	/* If free routine specified, free data */
	if (free_routine != NULL)
	    free_routine (symbol->data);

	/* Free symbol structure */
	L_free (MD_Symbol_pool, symbol);
    }

    /* Free the hash array and table name*/
    free (table->hash);
    free (table->name);

    /* Free the table structure */
    L_free (MD_Symbol_Table_pool, table);
}

/* Doubles the symbol table hash array size */
static void MD_resize_symbol_table (MD_Symbol_Table *table)
{
    MD_Symbol **new_hash, *symbol, *hash_head;
    int new_hash_size;
    unsigned int new_hash_mask, new_hash_index;
    int i;

    /* Double the size of the hash array */
    new_hash_size = table->hash_size * 2;

    /* Allocate new hash array */
    new_hash = (MD_Symbol **) malloc (new_hash_size * sizeof (MD_Symbol *));
    if (new_hash == NULL)
    {
	MD_punt (table->md, 
		 "MD_resize_symbol_table: Out of memory, new size %i.",
		 new_hash_size);
    }

    /* Initialize new hash table */
    for (i=0; i < new_hash_size; i++)
	new_hash[i] = NULL;

    /* Get the hash mask for the new hash table */
    new_hash_mask = new_hash_size -1; /* AND mask, works only for power of 2 */
    
    /* Go though all the symbol and add to new hash table.
     * Can totally disreguard old hash links.
     */
    for (symbol = table->head_symbol; symbol != NULL; 
	 symbol = symbol->next_symbol)
    {
	/* Get index into hash table to use for this name */
	new_hash_index = symbol->hash_val & new_hash_mask;
	
	/* Add symbol to head of linked list */
	hash_head = new_hash[new_hash_index];
	symbol->next_hash = hash_head;
	symbol->prev_hash = NULL;
	if (hash_head != NULL)
	    hash_head->prev_hash = symbol;
	new_hash[new_hash_index] = symbol;
    }

    /* Free old hash table */
    free (table->hash);
   
    /* Initialize table fields for new hash table */
    table->hash = new_hash;
    table->hash_size = new_hash_size;
    table->hash_mask = new_hash_mask;
    /* Resize when count at 75% of new_hash_size */
    table->resize_size = new_hash_size - (new_hash_size >> 2); 
}

/* Hashes a string, returning an unsigned 32 bit number. */
static unsigned int MD_hash_string (const char *string)
{
    unsigned int hash_val;
    const unsigned char *ptr;
    unsigned int ch;
    
    hash_val = 0;

    /* Scan through all the characters, adding the characters to the
     * hash value.  Multiply the hash value by 17 (using shifts) before
     * adding each character in.
     * 
     * This very quick hash algorithm was tuned to work well with
     * strings ending with numbers.
     */
    ptr = (const unsigned char *) string;
    while ((ch = *ptr) != 0)
    {
	/* Multiply hash_val by 17 by adding 16*hash_val to it.
	 * (Use a shift, integer multiply is usually very expensive)
	 */
	hash_val += (hash_val << 4);

	/* Add in character value */
	hash_val += ch;

	/* Goto next character */
	ptr ++;
    }

    /* Return the hash value */
    return (hash_val);
}

/* Adds MD structure to symbol table, name and data are not copied!!! 
 * Dynamically increases symbol table's hash array.
 * Returns pointer to added symbol.
 */
static MD_Symbol *MD_add_symbol (MD_Symbol_Table *table, const char *name, 
				 void *data)
{
    MD_Symbol *symbol, *hash_head, *check_symbol, *tail_symbol;
    unsigned int hash_val, hash_index;
    int symbol_count;

    /* Increase symbol table size if necessary before adding new symbol.  
     * This will change the hash_mask if the table is resized!
     */
    symbol_count = table->symbol_count;
    if (symbol_count >= table->resize_size)
    {
	MD_resize_symbol_table (table);
    }

    /* Allocate a symbol (pool initialized in create table routine)*/
    symbol = (MD_Symbol *) L_alloc (MD_Symbol_pool);
    
    /* Initialize fields */
    symbol->name = (char *) name;
    symbol->data = data;
    symbol->table = table;
    symbol->symbol_id = -1;	/* Used by MD_write_md only */

    /* Get tail symbol for ease of use */
    tail_symbol = table->tail_symbol;

    /* Add to linked list of symbols */
    symbol->next_symbol = NULL;
    symbol->prev_symbol = tail_symbol;

    if (tail_symbol == NULL)
	table->head_symbol = symbol;
    else
	tail_symbol->next_symbol = symbol;
    table->tail_symbol = symbol;

    /* Get hash value of name and put in symbol structure for quick compare
     * and table resize.
     */
    hash_val = MD_hash_string (name);
    symbol->hash_val = hash_val;

    /* Get index into hash table to use for this name */
    hash_index = hash_val & table->hash_mask;
    
    /* Get head symbol in current linked list for ease of use */
    hash_head = table->hash[hash_index];

    
    /* Sanity check (may want to ifdef out later).
     *
     * Check that this symbol's name is not already in the symbol table.
     * Punt if it is, since can cause a major debugging nightmare.
     */
    for (check_symbol = hash_head; check_symbol != NULL;
	 check_symbol = check_symbol->next_hash)
    {
	/* Check hash value before doing strcmp to avoid function
	 * call when possible.
	 */
	if ((check_symbol->hash_val == hash_val) &&
	    (strcmp(check_symbol->name, name) == 0))
	{
	    MD_punt (table->md, 
		     "%s: cannot add '%s', already in table!",
		     table->name, name);
	}
    }
    

    /* Add symbol to head of linked list */
    symbol->next_hash = hash_head;
    symbol->prev_hash = NULL;
    if (hash_head != NULL)
	hash_head->prev_hash = symbol;
    table->hash[hash_index] = symbol;

    /* Update table's symbol count */
    table->symbol_count = symbol_count + 1;

    /* Return symbol added */
    return (symbol);
}

/* Renames a symbol (name is still not copied and old name is not freed!)
 * Punts if new name is already in symbol table.
 */
static void MD_rename_symbol (MD_Symbol *symbol, const char *new_name)
{
    MD_Symbol_Table *table;
    MD_Symbol *hash_head, *check_symbol;
    unsigned int hash_val, hash_index;

    /* Get symbol table for ease of use */
    table = symbol->table;


    /* Get index into hash table for old name */
    hash_index = symbol->hash_val & table->hash_mask;

    /* Remove symbol hash linked list */
    if (symbol->prev_hash == NULL)
	table->hash[hash_index] = symbol->next_hash;
    else
	symbol->prev_hash->next_hash = symbol->next_hash;

    if (symbol->next_hash != NULL)
	symbol->next_hash->prev_hash = symbol->prev_hash;
    

    /* Get hash value of new name and put in symbol structure for quick compare
     * and table resize.
     */
    symbol->name = (char *)new_name;
    hash_val = MD_hash_string (new_name);
    symbol->hash_val = hash_val;

    /* Get index into hash table to use for this name */
    hash_index = hash_val & table->hash_mask;
    
    /* Get head symbol in current linked list for ease of use */
    hash_head = table->hash[hash_index];

    /* 
     * Check that this symbol's name is not already in the symbol table.
     * Punt if it is, since can cause a major debugging nightmare.
     */
    for (check_symbol = hash_head; check_symbol != NULL;
	 check_symbol = check_symbol->next_hash)
    {
	/* Check hash value before doing strcmp to avoid function
	 * call when possible.
	 */
	if ((check_symbol->hash_val == hash_val) &&
	    (strcmp(check_symbol->name, new_name) == 0))
	{
	    MD_punt (table->md, 
		     "%s: cannot rename to '%s', already in table!",
		     table->name, new_name);
	}
    }
    

    /* Add symbol to head of linked list */
    symbol->next_hash = hash_head;
    symbol->prev_hash = NULL;
    if (hash_head != NULL)
	hash_head->prev_hash = symbol;
    table->hash[hash_index] = symbol;
}

#if 0
/* -Wall complains that this routine not used -JCG 9/20/02 */
/* Returns a MD_Symbol structure with the desired name, or NULL
 * if the name is not in the symbol table.
 */
static MD_Symbol *MD_find_symbol (MD_Symbol_Table *table, const char *name)
{
    MD_Symbol *symbol;
    unsigned int hash_val, hash_index;

    /* Get the hash value for the name */
    hash_val = MD_hash_string (name);

    /* Get the index into the hash table */
    hash_index = hash_val & table->hash_mask;

    /* Search the linked list for matching name */
    for (symbol = table->hash[hash_index]; symbol != NULL; 
	 symbol = symbol->next_hash)
    {
	/* Compare hash_vals first before using string compare */
	if ((symbol->hash_val == hash_val) &&
	    (strcmp(symbol->name, name) == 0))
	{
	    return (symbol);
	}
    }
    return (NULL);
}
#endif

/* 
 * DO NOT CALL DIRECTLY!  FOR USE ONLY BY MD MACROS AND ROUTINES!
 * Returns the data for desired name, or NULL
 * if the name is not in the symbol table.
 */
void *_MD_find_symbol_data (MD_Symbol_Table *table, const char *name)
{
    MD_Symbol *symbol;
    unsigned int hash_val, hash_index;

    /* Get the hash value for the name */
    hash_val = MD_hash_string (name);

    /* Get the index into the hash table */
    hash_index = hash_val & table->hash_mask;

    /* Search the linked list for matching name */
    for (symbol = table->hash[hash_index]; symbol != NULL; 
	 symbol = symbol->next_hash)
    {
	/* Compare hash_vals first before using string compare */
	if ((symbol->hash_val == hash_val) &&
	    (strcmp(symbol->name, name) == 0))
	{
	    return (symbol->data);
	}
    }
    return (NULL);
}

/* Deletes symbol and optionally deletes the data using the free routine */
void MD_delete_symbol (MD_Symbol *symbol, void (*free_routine)(void *))
{
    MD_Symbol_Table *table;
    MD_Symbol *next_hash, *prev_hash, *next_symbol, *prev_symbol;
    unsigned int hash_index;

    /* Get the table the symbol is from */
    table = symbol->table;

    /* Get the hash index from the symbol's hash_val */
    hash_index = symbol->hash_val & table->hash_mask;

    /* Remove symbol from hash table */
    prev_hash = symbol->prev_hash;
    next_hash = symbol->next_hash;
    if (prev_hash == NULL)
	table->hash[hash_index] = next_hash;
    else
	prev_hash->next_hash = next_hash;

    if (next_hash != NULL)
	next_hash->prev_hash = prev_hash;

    /* Remove symbol from symbol list */
    prev_symbol = symbol->prev_symbol;
    next_symbol = symbol->next_symbol;
    if (prev_symbol == NULL)
	table->head_symbol = next_symbol;
    else
	prev_symbol->next_symbol = next_symbol;

    if (next_symbol == NULL)
	table->tail_symbol = prev_symbol;
    else
	next_symbol->prev_symbol = prev_symbol;


    /* If free routine specified, free symbol data */
    if (free_routine != NULL)
	free_routine (symbol->data);

    /* Free symbol structure */
    L_free (MD_Symbol_pool, symbol);

    /* Decrement table symbol count */
    table->symbol_count --;
}


#if 0
/* -Wall complains that this routine not used -JCG 9/20/02 */
/* Prints out the symbol table's hash table (debug routine) */
static void MD_print_symbol_table_hash (FILE *out, MD_Symbol_Table *table)
{
    int hash_index, lines;
#if 0
    MD_Symbol *symbol;
#endif

    /* Count lines used in table */
    lines = 0;
    for (hash_index = 0; hash_index < table->hash_size; hash_index++)
    {
	if (table->hash[hash_index] != NULL)
	    lines++;
    }
    fprintf (out, "%s has %i entries (hash size %i, used %i):\n", 
	     table->name, table->symbol_count, table->hash_size, lines);

#if 0
    /* For each hash_index in hash table */
    for (hash_index = 0; hash_index < table->hash_size; hash_index++)
    {
	/* Skip empty lines */
	if (table->hash[hash_index] == NULL)
	    continue;

	fprintf (out, "%4i:", hash_index);
	for (symbol = table->hash[hash_index]; symbol != NULL; 
	     symbol = symbol->next_hash)
	{
	    fprintf (out, " %s(%i)", symbol->name, symbol->hash_val);
	}
	fprintf (out, "\n");
    }
#endif
}
#endif

/* Creates a new MD with an initial size suitable for the specified
 * number of sections.  The MD automatically resizes (upward) when
 * appropriate, so 0 sections may be specified.
 */
MD *MD_new_md (const char *name, int num_sections)
{
    MD *md;

    /* If the expected number of sections is negative, force to be 0 */
    if (num_sections < 0)
	num_sections = 0;

    /* To catch improperly specified num_sections, ensure that the
     * expected size is less than a billion.  (Also prevents symbol
     * table routines from punting.)
     */
    if (num_sections > 1000000000)
	MD_punt (NULL, "MD_new_md: unreasonable num_sections (%u)",
		 num_sections);

    /* Initialize all the pools used by the MD routines on first call
     * to MD_new_md().
     */
    if (MD_pool == NULL)
    {
	MD_pool = L_create_alloc_pool ("MD", sizeof (MD), 1);
        MD_Symbol_Table_pool = L_create_alloc_pool ("MD_Symbol_Table",
						    sizeof (MD_Symbol_Table), 
						    16);
	MD_Symbol_pool = L_create_alloc_pool ("MD_Symbol", sizeof (MD_Symbol),
					      64);
	MD_Section_pool = L_create_alloc_pool ("MD_Section", 
					       sizeof(MD_Section), 16);
	MD_Field_Decl_pool = L_create_alloc_pool ("MD_Field_Decl",
						  sizeof(MD_Field_Decl), 16);
	MD_Element_Req_pool = L_create_alloc_pool ("MD_Element_Req",
						  sizeof(MD_Element_Req), 64);
	MD_Entry_pool = L_create_alloc_pool ("MD_Entry", sizeof(MD_Entry), 64);
	MD_Field_pool = L_create_alloc_pool ("MD_Field", sizeof(MD_Field), 64);
	MD_Element_pool = L_create_alloc_pool ("MD_Element", 
					       sizeof(MD_Element), 64);
    }

    /* Allocate new md */
    md = (MD *) L_alloc (MD_pool);

    /* Initialize fields */
    md->name = MD_strdup (name);
    md->section_table = MD_new_symbol_table (md, "Section table", 
					     num_sections);

    /* Return new md */
    return (md);
}

/* Frees up all memory associated with the md.
 * If no other mds are allocated, free up the memory used by
 * the L_alloc memory manager.
 */
void MD_delete_md (MD *md)
{
    char *name;

    /* Free name last (for error messages) */
    name = md->name;

    /* Free section symbol table and all the information contained in it. */
    MD_delete_symbol_table (md->section_table, 
			    (void(*)(void *))_MD_free_section);

    /* Free the md structure */
    L_free (MD_pool, md);

    /* If no other mds are allocated, attempt to free the memory
     * used by the L_alloc memory manager.
     */
    if (MD_pool->allocated == MD_pool->free)
    {
	/* The following free routines will give warnings if some
	 * of the allocated memory is still in use.
	 * We will through away the memory unfreed if this
	 * occurs (it shouldn't!)
	 */
	L_free_alloc_pool (MD_pool);
	L_free_alloc_pool (MD_Symbol_Table_pool);
	L_free_alloc_pool (MD_Symbol_pool);
	L_free_alloc_pool (MD_Section_pool);
	L_free_alloc_pool (MD_Entry_pool);
	L_free_alloc_pool (MD_Field_Decl_pool);
	L_free_alloc_pool (MD_Element_Req_pool);
	L_free_alloc_pool (MD_Field_pool);
	L_free_alloc_pool (MD_Element_pool);

	/* Set all pool pointer's to NULL to prevent use of freed memory
	 * (and to signal that pools need to be initialized if used)
	 */
	MD_pool = NULL;
	MD_Symbol_Table_pool = NULL;
	MD_Symbol_pool = NULL;
	MD_Section_pool = NULL;
	MD_Entry_pool = NULL;
	MD_Field_Decl_pool = NULL;
	MD_Element_Req_pool = NULL;
	MD_Field_pool = NULL;
	MD_Element_pool = NULL;
    }

    /* Free the md's name */
    free (name);
}

/*
 * This function checks the elements in the specified md.
 * 
 * Returns the number of required fields not specified + the 
 * number of elements with incorrect type in the entire md (0 if all ok).
 * 
 * If out is non-NULL, warning messages for non-specified required field
 * and for each element with incorrect type will be printed to out.
 */
int MD_check_md (FILE *out, MD *md)
{
    MD_Symbol *symbol;
    MD_Section *section;
    int error_count;

    /* Initialize error count */
    error_count = 0;

    /* Scan every section in the md for errors */
    for (symbol = md->section_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get the section pointed to by this symbol */
	section = (MD_Section *) symbol->data;

	/* Sum up all the errors found by check section */
	error_count += _MD_check_section (out, section, "MD_check_md");
    }

    /* Return the number of errors found in the md */
    return (error_count);
}


/* Creates a md buf for the MD_read routines */
MD_Buf *MD_new_buf ()
{
    MD_Buf *buf;
    int buf_size;

    /* Allocate new buf */
    buf = (MD_Buf *) L_alloc (MD_Buf_pool);

    /* Create a initial buffer of size 128 */
    buf_size = 128;
    buf->buf = (char *) malloc (buf_size);
    if (buf->buf == NULL)
	MD_punt (NULL, "MD_new_buf: out of memory (size %i)", buf_size);
    buf->buf_size = buf_size;

    /* Return the new buffer */
    return (buf);
}

/* Doubles the size of the buffer in buf */
void MD_resize_buf (MD_Buf *buf)
{
    char *old_buf, *new_buf;
    int old_size, new_size, i;

    /* Get fields into local variables for ease of use */
    old_buf = buf->buf;
    old_size = buf->buf_size;

    /* Double buffer size */
    new_size = old_size << 1;
    
    new_buf = (char *) malloc (new_size);
    if (new_buf == NULL)
	MD_punt (NULL, "MD_resize_buf: out of memory (size %i)", new_size);
    
    /* Copy over contents of old buffer */
    for (i=0; i < old_size; i++)
	new_buf[i] = old_buf[i];
    
    /* Free old buf and reset structure variables */
    free (old_buf);
    buf->buf = new_buf;
    buf->buf_size = new_size;
}

/* Frees md buf used by MD_read routines */
void MD_delete_buf (MD_Buf *buf)
{
    /* Free buffer */
    free (buf->buf);

    /* Free buffer structure */
    L_free (MD_Buf_pool, buf);
}

/* Reads an hex integer and returns the value. 
 * Uses fast conversion algorithm that assumes ascii and that all the
 * integers written in MD_write_md are in lower-case hex.
 */
int MD_read_int (FILE *in)
{
    int value, ch, negate;
    unsigned int nibble;
    
    /* Skip leading whitespace */
    while ((ch = getc(in)) != EOF)
    {
	if (ch != ' ')
	    break;
    }
   
    /* Punt if hit EOF */
    if (ch == EOF)
	MD_punt (NULL, "MD_read_int: unexpected EOF");

    /* Is value negative? */
    negate = 0;
    if (ch == '-')
    {
	negate = 1;
	if ((ch = getc(in)) == EOF)
	MD_punt (NULL, "MD_read_int: unexpected EOF");
    }

    /* Get the first nibble, assume ascii and using 'a' thru 'f'*/
    if (ch >= 'a')
	nibble = 10 + ch - 'a';
    else
	nibble = ch - '0';

    /* Make sure in range */
    if (nibble > 15)
	MD_punt (NULL, "MD_read_int: int expected not '%c'", ch);
	
    /* Initially set value to this */
    value = nibble;
    
    /* Read in rest of int */
    while ((ch = getc(in)) != EOF)
    {
	/* Stop at space or newline */
	if ((ch == ' ') || (ch == '\n'))
	    break;

	/* Get the next nibble, assume ascii and using 'a' thru 'f'*/
	if (ch >= 'a')
	    nibble = 10 + ch - 'a';
	else
	    nibble = ch - '0';

	/* Make sure in range */
	if (nibble > 15)
	    MD_punt (NULL, "MD_read_int: int expected not '%c'", ch);

	/* Shift value and add in nibble */
	value = (value << 4) + nibble;
    }

    /* If not at EOF, put back character just read */
    if (ch != EOF)
	ungetc (ch, in);

    /* Negate value if necessary */
    if (negate)
	value = -value;

    /* Return value read in */
    return (value);
}

/* Reads an double into buf and returns its value (contents of buf destroyed)
 */
double MD_read_double (FILE *in, MD_Buf *value_buf)
{
    char *buf, *end_ptr;
    int i, ch, resize_size;
    double value;
    
    /* Get values in value_buf into local variables for ease of access */
    buf = value_buf->buf;
    resize_size = value_buf->buf_size -2; /* Want space for terminator */

    /* Skip leading whitespace */
    while ((ch = getc(in)) != EOF)
    {
	if (ch != ' ')
	    break;
    }
   
    /* Punt if hit EOF */
    if (ch == EOF)
	MD_punt (NULL, "MD_read_double: unexpected EOF");

    /* Place first character of in buffer */
    buf[0] = ch;
    i = 1;

    /* Read in rest of double */
    while ((ch = getc(in)) != EOF)
    {
	/* Stop at space or newline */
	if ((ch == ' ') || (ch == '\n'))
	    break;
	
	/* Resize buffer if necessary */
	if (i >= resize_size)
	{
	    MD_resize_buf (value_buf);

	    /* Get new values into local variables */
	    buf = value_buf->buf;
	    resize_size = value_buf->buf_size -2;
	}

	buf[i] = ch;
	i++;
    }

    /* If not at EOF, put back character just read */
    if (ch != EOF)
	ungetc (ch, in);

    /* Terminate buffer */
    buf[i] = 0;

    /* Get the double value */
    value = strtod (buf, &end_ptr);

    /* Make sure read in something */
    if (*end_ptr != 0)
	MD_punt (NULL, "MD_read_double: double expected not '%s'!", buf);

    /* Return value read in */
    return (value);
}

/* Reads an string into buf (value returned in buf) 
 * If 'newline_count' is > 0, newline_count newlines will be considered
 * part of the string.  This allows strings in fields that have newlines
 * in them.  Passing in 0 for newline count gives the old behavior. 
 */
void MD_read_string (FILE *in, MD_Buf *value_buf, int newline_count)
{
    char *buf;
    int  i, ch, resize_size;
    
    /* Get values in value_buf into local variables for ease of access */
    buf = value_buf->buf;
    resize_size = value_buf->buf_size -2; /* Want space for terminator */

    /* Dont skip leading whitespace */
   
    i = 0;

    /* Read in string */
    while ((ch = getc(in)) != EOF)
    {
	/* Stop at newline only (spaces can be part of the string)
	 * but now include 'newline_count' newlines in the string  -JCG 3/24/04
	 */
	if (ch == '\n')
	{
	    /* Decrement the included newline count for the string */
	    newline_count--;

	    /* If this newline was not part of the string, terminate string */
	    if (newline_count < 0)
		break;
	}
	
	/* Resize buffer if necessary */
	if (i >= resize_size)
	{
	    MD_resize_buf (value_buf);

	    /* Get new values into local variables */
	    buf = value_buf->buf;
	    resize_size = value_buf->buf_size -2;
	}

	buf[i] = ch;
	i++;
    }

    /* If not at EOF, put back character just read */
    if (ch != EOF)
	ungetc (ch, in);
    /* Make sure not at EOF with empty buffer */
    else if (i == 0)
	MD_punt (NULL, "MD_read_string: unexpected EOF");

    /* Terminate buffer, value returned in value_buffer (buf)*/
    buf[i] = 0;
}

/* Reads an binary block of size block_size into buf (value returned in buf) */
void MD_read_block (FILE *in, unsigned int block_size, MD_Buf *value_buf)
{
    unsigned char *buf;
    unsigned int  i;
    int ch;
 
    /* Resize buffer until big enough for binary block */
    while (block_size > (unsigned) value_buf->buf_size)
	MD_resize_buf (value_buf);

    /* Get values in value_buf into local variables for ease of access */
    buf = (unsigned char *)value_buf->buf;

    /* Read in binary block into buffer, use getc to guarentee compatibility
     * with ungetc (although fread seems to work with ungetc on HPUX).
     */
    for (i = 0; i < block_size; i++)
    {
	/* Get next character, punt if EOF */
	if ((ch = getc(in)) == EOF)
	    MD_punt (NULL, "MD_read_block: unexpected EOF");

	/* Write character into binary block buffer */
	buf[i] = (unsigned char) ch;
    }
}

void MD_read_nl (FILE *in)
{
    int ch;

    ch = getc (in);
    if (ch != '\n')
    {
	if (ch == EOF)
	    MD_punt (NULL, "MD_read_nl: unexpected EOF");

	MD_punt (NULL, "MD_read_nl: newline expected not '%c'!", ch);
    }
}

void MD_read_sp (FILE *in)
{
    int ch;

    ch = getc (in);
    if (ch != ' ')
    {
	if (ch == EOF)
	    MD_punt (NULL, "MD_read_sp: unexpected EOF");

	MD_punt (NULL, "MD_read_sp: space expected not '%c'!", ch);
    }
}

/* Creates a md from the low-level format file created by MD_write_md */
MD *MD_read_md (FILE *in, const char *name)
{
    MD *md;
    MD_Section *section, **section_array, **target_array;
    MD_Entry *entry, **entry_array, *link;
    MD_Field_Decl *field_decl, **field_decl_array;
    MD_Buf *name_buf, *value_buf;
    MD_Field *field;
    int section_array_size, entry_array_size;
    int section_index, entry_index, field_decl_index;
    int entry_count, field_decl_count;
    char type_marker, kleene_marker, require_marker;
    MD_FIELD_TYPE field_type;
    int kleene_starred, require_count, require_index, target_index;
    int target_count, local_entry_index, element_index, element_count;
    int field_count, field_index, int_value, link_index;
    double double_value;
    unsigned int block_size;
    int nl_count;
    int i;
    
    /* Create buf alloc pool used by read routines */
    MD_Buf_pool = L_create_alloc_pool ("MD_Buf", sizeof(MD_Buf), 4);

    /* Create the buffers we will need to read in the md */
    name_buf = MD_new_buf();
    value_buf = MD_new_buf();

    /* Get the number of sections and entries */
    section_array_size = MD_read_int (in);
    entry_array_size = MD_read_int (in);
    MD_read_nl (in);
    MD_read_nl (in);

    /* Create md */
    md = MD_new_md (name, section_array_size);
    
/*    printf ("section_array_size = %i   entry_array_size = %i\n",
	    section_array_size, entry_array_size); */

    /* Allocate section and entry arrays */
    section_array = (MD_Section **) malloc (sizeof(MD_Section *) * 
					    section_array_size);
    if (section_array == NULL)
    {
	MD_punt (NULL, "MD_read_md: out of memory (size %i).",
		 section_array_size);
    }

    entry_array = (MD_Entry **) malloc (sizeof(MD_Entry *) * 
					    entry_array_size);
    if (entry_array == NULL)
    {
	MD_punt (NULL, "MD_read_md: out of memory (size %i).",
		 entry_array_size);
    }

    /* Initialize entry index */
    entry_index = 0;

    /* Read in each section and the section's entry names */
    for (section_index = 0; section_index < section_array_size; 
	 section_index++)
    {
	/* Read in section name, entry count and field decl count */
	MD_read_string (in, name_buf, 0);
	MD_read_nl(in);

	entry_count = MD_read_int (in);
	field_decl_count = MD_read_int (in);
	MD_read_nl(in);

/*	printf ("Read %s %i %i\n", name_buf->buf, entry_count, 
		field_decl_count);*/

	/* Create section and put in section array */
	section = MD_new_section (md, name_buf->buf, entry_count, 
				  field_decl_count);
	section_array[section_index] = section;

	/* Read in section's entry names */
	for (i = 0; i < entry_count; i++)
	{
	    MD_read_string (in, name_buf, 0);
	    MD_read_nl (in);

	    /* Create entry and put in entry array */
	    entry = MD_new_entry (section, name_buf->buf);
	    entry_array[entry_index++] = entry;
	}
	MD_read_nl(in);
    }
    MD_read_nl(in);

    /* Restart entry index for reading in all the entry's fields */
    entry_index = 0;
    
    /* Read in each section's field declarations and the
     * field values for all entries in the section.
     */
    for (section_index = 0; section_index < section_array_size; 
	 section_index++)
    {
	/* Read in section name, entry count and field decl count */
	MD_read_string (in, name_buf, 0);
/*	printf ("Reading section %s\n", name_buf->buf); */
	MD_read_nl(in);

	entry_count = MD_read_int (in);
	field_decl_count = MD_read_int (in);
	MD_read_nl(in);

	/* Get the section we expect to be in */
	section = section_array[section_index];

	/* Make sure the name matches the name read in */
	if (strcmp (name_buf->buf, section->name) != 0)
	{
	    MD_punt (NULL, "Section '%s' expected, not '%s'!",
		     section->name, name_buf->buf);
	}

	/* Create array only if there are field declarations */
	if (field_decl_count > 0)
	{
	    /* Create field declaration array */
	    field_decl_array = 
		(MD_Field_Decl **) malloc (sizeof(MD_Field_Decl *) *
					   field_decl_count);
	    if (field_decl_array == NULL)
		MD_punt (NULL, "MD_read_md: Out of memory (size %i)", 
			 field_decl_count);
	}
	else
	{
	    /* Should not be used if there are no field declarations */
	    field_decl_array = NULL;
	}

	/* Read in the field declarations */
	for (field_decl_index = 0; field_decl_index < field_decl_count;
	     field_decl_index++)
	{
	    /* Get field name */
	    MD_read_string (in, name_buf, 0);
/*	    printf ("Declaring field '%s'\n", name_buf->buf); */
	    MD_read_nl (in);

	    /* Get type of field declaration */
	    type_marker = getc (in);

	    switch (type_marker)
	    {
	      case 'R':
		field_type = MD_REQUIRED_FIELD;
		break;

	      case 'O':
		field_type = MD_OPTIONAL_FIELD;
		break;

	      default:
		MD_punt (NULL, "MD_read_md: Unknown field decl type '%c'",
			 type_marker);
		field_type = 0; /* Avoid compiler warning */
	    }

	    require_count = MD_read_int(in);
	    MD_read_sp (in);
	    kleene_marker = getc (in);

	    switch (kleene_marker)
	    {
	      case '*':
		kleene_starred = 1;
		break;

	      case '-':
		kleene_starred = 0;
		break;

	      default:
		MD_punt (NULL, "MD_read_md: Unknown kleene marker '%c'",
			 kleene_marker);
		kleene_starred = 0;     /* Avoid compiler warning */
	    }
	    MD_read_nl (in);

	    /* Create field declaration and place into array */
	    field_decl = MD_new_field_decl (section, name_buf->buf,
					    field_type);
	    field_decl_array[field_decl_index] = field_decl;

/*	    printf ("Read field_decl %s\n", name_buf->buf); */
	    
	    /* Read in requirements */
	    for (require_index = 0; require_index < require_count;
		 require_index++)
	    {
		/* Get requirement type */
		require_marker = getc (in);

		/* Define requirment */
		switch (require_marker)
		{
		  case 'I':
		    MD_require_int (field_decl, require_index);
		    break;

		  case 'D':
		    MD_require_double (field_decl, require_index);
		    break;

		  case 'S':
		    MD_require_string (field_decl, require_index);
		    break;

		  case 'B':
		    MD_require_block (field_decl, require_index);
		    break;

		  case 'L':
		    /* Get the number of targets */
		    target_count = MD_read_int (in);

		    /* Create target array */
		    target_array = 
			(MD_Section **) malloc (sizeof(MD_Section *) *
						target_count);
		    
		    if (target_array == NULL)
		    {
			MD_punt (NULL, "MD_read_md: Out of memory (size %i)",
				 target_count);
		    }

		    /* Read in targets */
		    for (i=0; i < target_count; i++)
		    {
			target_index = MD_read_int (in);
			target_array[i] = section_array[target_index];
		    }

		    MD_require_multi_target_link (field_decl, require_index,
						  target_count, target_array);

		    /* Free target array */
		    free (target_array);

		    break;
		    
		  default:
		    MD_punt (NULL, 
			     "MD_read_md: Unknown requirement type '%c'", 
			    require_marker);

		}
		MD_read_nl(in);

	    }
	    MD_read_nl(in);

	    /* Mark last requirement as kleene starred (if required) */
	    if (kleene_starred)
	    {
		MD_kleene_star_requirement (field_decl, require_count -1);
	    }
	}
	/* Read in the fields for each entry */
	for (local_entry_index = 0; local_entry_index < entry_count;
	     local_entry_index++)
	{	  
	    /* Get the entry we are processing, use entry_index
	     * to get entry id from start of md (not from start
	     * of section)
	     */
	    entry = entry_array[entry_index++];
	    
	    /* Read the number of fields defined */
	    field_count = MD_read_int (in);
	    MD_read_nl (in);
	    MD_read_nl (in);
	    
	    /* Read in the fields defined */
	    for (i = 0; i < field_count; i++)
	    {
		/* Get the field index and field decl for this index */
		field_index = MD_read_int (in);
		element_count = MD_read_int (in);
		MD_read_nl (in);

		field_decl = field_decl_array[field_index];
		
/*		printf ("Reading field %s\n", field_decl->name); */

		/* Create field */
		field = MD_new_field (entry, field_decl, element_count);

		/* Read in the elements */
		for (element_index = 0; element_index < element_count;
		     element_index++)
		{
		    /* Get the type of element */
		    type_marker = getc(in);

		    /* Read in depending on type */
		    switch (type_marker)
		    {
		      case 'I':
			int_value = MD_read_int (in);
			MD_set_int (field, element_index, int_value);
			break;

		      case 'D':
			double_value = MD_read_double (in, value_buf);
			MD_set_double (field, element_index, double_value);
			break;

		      case 'S':
			/* The older string format does allow any newlines
			 * in the string.  Used when possible for backward
			 * compatibility. -JCG 3/24/04
			 */
			MD_read_string (in, value_buf, 0);
			MD_set_string (field, element_index, value_buf->buf);
			break;

		      case 'N':
			/* In order to support newlines in strings, 
			 * now have new type of string marker 'N' that 
			 * includes a newline count (and a space) right
			 * before the string. -JCG 3/24/04
			 */
			nl_count = MD_read_int (in);
			MD_read_sp (in);
			MD_read_string (in, value_buf, nl_count);
			MD_set_string (field, element_index, value_buf->buf);
			break;

		      case 'B':
			block_size = MD_read_int (in);
			MD_read_sp (in);
			MD_read_block (in, block_size, value_buf);
			MD_set_block (field, element_index, block_size,
				      (void *)value_buf->buf);
			break;

		      case 'L':
			link_index = MD_read_int (in);
			link = entry_array[link_index];
			MD_set_link (field, element_index, link);
			break;

		      case '-':
			/* Do nothing, value not set for this element */
			break;

		      default:
			MD_punt (NULL, 
				 "MD_read_md: Unknown element type '%c'.",
				 type_marker);
		    }
		    MD_read_nl(in);
		}
		MD_read_nl(in);
	    }
	}
	MD_read_nl(in);

	/* Free field_decl_array */
	if (field_decl_array != NULL)
	    free (field_decl_array);
    }

    /* Free arrays we created for reading in md */
    free (section_array);
    free (entry_array);


    /* Free the buffers we created */
    MD_delete_buf (name_buf);
    MD_delete_buf (value_buf);

    /* Free the buf alloc pool */
    L_free_alloc_pool (MD_Buf_pool);

    /* Return the md created */
    return (md);
}



/* Writes the md's contents to the file in the low-level format
 * that MD_read_md() expects.
 */
void MD_write_md (FILE *out, MD *md)
{
    MD_Symbol *section_symbol, *entry_symbol, *field_decl_symbol;
    MD_Section *section, **link_array;
    MD_Entry *entry;
    MD_Field_Decl *field_decl;
    MD_Element_Req *element_req, **require_array;
    MD_Field **field_array, *field;
    MD_Element *element, **element_array;
    int total_entry_count, max_require_index, i;
    unsigned int block_size;
    int section_id, entry_id, field_decl_id, link_array_size;
    int max_field_index, field_count, max_assigned_index, require_index;
    char type_marker, kleene_marker;
    char *ptr, ch;
    int nl_count;

    /* Initialize the total number of entries in the md */
    total_entry_count = 0;

    /* Initialize the seciton and entry id counters.  */
    section_id = 0;
    entry_id = 0;

    /* Scan entire md, assigning ids to all the symbols */
    for (section_symbol = md->section_table->head_symbol; 
	 section_symbol != NULL; section_symbol = section_symbol->next_symbol)
    {
	/* Set the section symbol id */
	section_symbol->symbol_id = section_id++;

	/* Get the section for ease of use */
	section = (MD_Section *) section_symbol->data;

	/* Set the field_ids for all fields declared for this section.
	 * Starts from 0 at the beginning of each section.
	 */
	field_decl_id = 0;
	for (field_decl_symbol = section->field_decl_table->head_symbol;
	     field_decl_symbol != NULL; 
	     field_decl_symbol = field_decl_symbol->next_symbol)
	{
	    field_decl_symbol->symbol_id = field_decl_id++;
	}


	/* Set the symbol_ids for all entries in this table.
	 * Starts from 0 at beginning of md to give each entry
	 * a unique id.
	 */
	for (entry_symbol = section->entry_table->head_symbol;
	     entry_symbol != NULL; entry_symbol = entry_symbol->next_symbol)
	{
	    entry_symbol->symbol_id = entry_id++;
	}

	/* Update the total entry count */
	total_entry_count += section->entry_table->symbol_count;
    }

    /* Write out how many sections and entries in the md */
    fprintf (out, "%x %x\n\n", md->section_table->symbol_count,
	     total_entry_count);

    /* Write out the header for each section and list out the
     * entries in each section.
     */
    for (section_symbol = md->section_table->head_symbol; 
	 section_symbol != NULL; section_symbol = section_symbol->next_symbol)
    {
	/* Get the section for ease of use */
	section = (MD_Section *) section_symbol->data;

	/* Write out each section name and how many entries the section
	 * has and how many fields are declared for the section.
	 */
#if 0
	fprintf (out, "%i %s %i %i\n", section_symbol->symbol_id, 
		 section->name,
		 section->entry_table->symbol_count, 
		 section->field_decl_table->symbol_count);
#endif
	fprintf (out, "%s\n%x %x\n", 
		 section->name,
		 section->entry_table->symbol_count, 
		 section->field_decl_table->symbol_count);

	for (entry_symbol = section->entry_table->head_symbol;
	     entry_symbol != NULL; entry_symbol = entry_symbol->next_symbol)
	{
	    /* Get the entry for ease of use */
	    entry = (MD_Entry *) entry_symbol->data;

	    /* Write out each entries name */
#if 0
	    fprintf (out, "%i %s\n", entry_symbol->symbol_id,
		     entry->name);
#endif
	    fprintf (out, "%s\n", entry->name);

	}

	putc ('\n', out);
    }
    putc ('\n', out);

    /*
     * Write out the section's contents
     */
    for (section_symbol = md->section_table->head_symbol; 
	 section_symbol != NULL; section_symbol = section_symbol->next_symbol)
    {
	/* Get the section for ease of use */
	section = (MD_Section *) section_symbol->data;

	/* Again write how many entries and field declarations there
	 * are in the section 
	 */
#if 0
	fprintf (out, "%i %s %i %i\n", section_symbol->symbol_id,
		 section->name, 
		 section->entry_table->symbol_count,
		 section->field_decl_table->symbol_count);
#endif
	fprintf (out, "%s\n%x %x\n",
		 section->name, 
		 section->entry_table->symbol_count,
		 section->field_decl_table->symbol_count);

	for (field_decl_symbol = section->field_decl_table->head_symbol;
	     field_decl_symbol != NULL; 
	     field_decl_symbol = field_decl_symbol->next_symbol)
	{
	    /* Get the field declaration from the symbol */
	    field_decl = (MD_Field_Decl *) field_decl_symbol->data;

	    switch (field_decl->type)
	    {
	      case MD_REQUIRED_FIELD:
		type_marker = 'R';
		break;

	      case MD_OPTIONAL_FIELD:
		type_marker = 'O';
		break;

	      default:
		MD_punt (NULL, "MD_write_md: Unknown field decl type '%i'.",
			 field_decl->type);
		type_marker = 0;	/* Avoid compiler warning */
	    }

	    if (field_decl->kleene_starred_req != NULL)
		kleene_marker = '*';
	    else
		kleene_marker = '-';


	    /* Get values into local variables for ease of use */
	    max_require_index = field_decl->max_require_index;
	    require_array = field_decl->require;
	    
	    /* Print out the field name and the number of
	     * required fields
	     */
#if 0
	    fprintf (out, "%i %s %i %c\n", field_decl_symbol->symbol_id,
		     field_decl->name, 
		     max_require_index + 1,
		     kleene_marker);
#endif
	    fprintf (out, "%s\n%c %x %c\n", 
		     field_decl->name,
		     type_marker,
		     max_require_index + 1,
		     kleene_marker);

	    for (require_index=0; require_index <= max_require_index; 
		 require_index++)
	    {
		element_req = require_array[require_index];
		
		switch (element_req->type)
		{
		  case MD_INT:
		    putc ('I', out);
		    putc ('\n', out);
		    break;

		  case MD_DOUBLE:
		    putc ('D', out);
		    putc ('\n', out);
		    break;

		  case MD_STRING:
		    putc ('S', out);
		    putc ('\n', out);
		    break;

		  case MD_BLOCK:
		    putc ('B', out);
		    putc ('\n', out);
		    break;

		  case MD_LINK:
		    link_array = element_req->link;
		    link_array_size = element_req->link_array_size;
		    fprintf (out, "L %x", link_array_size);
		    for (i=0; i < link_array_size; i++)
			fprintf (out, " %x", link_array[i]->symbol->symbol_id);
		    putc ('\n', out);
		    break;
		    
		  default:
		    MD_punt (NULL, "MD_write_md: Unknown req type %i",  
			     element_req->type);
		}

	    }
	    putc ('\n', out);
	}

	/* Get values into local variables for ease of use */
	max_field_index = section->max_field_index;
	
	/* Print out the field contents for each field */
	for (entry_symbol = section->entry_table->head_symbol;
	     entry_symbol != NULL; 
	     entry_symbol = entry_symbol->next_symbol)
	{
	    /* Get the entry for ease of use */
	    entry = (MD_Entry *) entry_symbol->data;
	    
	    /* Get the field array for ease of use */
	    field_array = entry->field;
	    
	    /* Count the number of fields defined for this entry */
	    field_count = 0;
	    for (i=0; i <= max_field_index; i++)
	    {
		if (field_array[i] != NULL)
		    field_count ++;
	    }
	    
	    /* Write out header for each entry, specifying the
	     * number of fields defined for each entry
	     */
#if 0
	    fprintf (out, "%i %i\n", entry_symbol->symbol_id,
		     field_count);
#endif

	    fprintf (out, "%x\n\n", field_count);
	    
	    for (field_decl_symbol=section->field_decl_table->head_symbol;
		 field_decl_symbol != NULL; 
		 field_decl_symbol = field_decl_symbol->next_symbol)
	    {
		/* Get the field declaration from the symbol */
		field_decl = (MD_Field_Decl *) field_decl_symbol->data;
		
		field = field_array[field_decl->field_index];
		
		if (field == NULL)
		    continue;

		/* Get element array for ease of use */
		element_array = field->element;

		/* Find the max assigned element index */
		for (max_assigned_index = field->max_element_index;
		     max_assigned_index >= 0; max_assigned_index--)
		{
		    if (element_array[max_assigned_index] != NULL)
			break;
		}
		
		/* Print out field header, specifying field symbol id,
		 * and the number of elements specified
		 */
		fprintf (out, "%x %x\n", 
			 field_decl_symbol->symbol_id,
			 max_assigned_index + 1);

		for (i=0; i <= max_assigned_index; i++)
		{
		    /* Get element for ease of use */
		    element = element_array[i];

		    if (element == NULL)
		    {
			fprintf (out, "-\n");
		    }
		    else
		    {
			switch (element->type)
			{
			    /* Print int elements as signed hex numbers */
			  case MD_INT:
			    if (element->value.i < 0)
				fprintf (out, "I-%x\n", -element->value.i);
			    else
				fprintf (out, "I%x\n", element->value.i);

			    break;

			  case MD_DOUBLE:
			    fprintf (out, "D%0.16g\n", element->value.d);
			    break;

			  case MD_STRING:
			    /* Now count the number of newlines in the
			     * string and included the count before the
			     * string. -JCG 3/24/04
			     */
			    nl_count = 0;
			    ptr = element->value.s;
			    while ((ch = *ptr) != 0)
			    {
				/* Count the newlines in the string */
				if (ch == '\n')
				    nl_count++;
				
				/* Goto next character in string */
				ptr++;
			    }

			    /* Use new string marker 'N' if newlines are
			     * present (to allow backward compatibility
			     * with old md files). -JCG 3/24/04
			     */
			     if (nl_count > 0)
			     {
				 /* Print out string marker and the number of
				  * newlines (and a space) before the actual 
				  * string.  Use new marker 'N' instead of 'S'!
				  */
				 fprintf (out, "N%x %s\n", nl_count, 
					  element->value.s);
			     }

			     /* Otherwise, print the normal string with no 
			      * newlines with the old 'S' marker. -JCG 3/24/04
			      */
			     else
			     {
				 fprintf (out, "S%s\n", element->value.s);
			     }
			    break;

			  case MD_BLOCK:
			    /* Get block size for ease of use */
			    block_size = element->value.b.size;

			    /* Print out contents of binary block in
			     * 'binary', prefixed by the size of the block
			     * in hex (i.e., 'Bsize binary_data\n')
			     * 
			     * This data is not platform independent anyway,
			     * so I didn't see the benefit from converting
			     * to ascii like I did with the other data 
			     * types. -JCG 1/15/98
			     */
			    fprintf (out, "B%x ", block_size);

			    /* Write out binary data */
			    fwrite (element->value.b.ptr, block_size,
				    1, out);

			    /* End with newline like normal */
			    putc ('\n', out);
			    break;

			  case MD_LINK:
			    fprintf (out, "L%x\n",
				     element->value.l->symbol->symbol_id);
			    break;

			  default:
			    MD_punt (NULL, 
				     "MD_write_md: Unknown element type %i.",
				     element->type);
			}
		    }
		}
		
		putc ('\n', out);
	    }
	}
	
	putc ('\n', out);
	
    }
    putc('\n', out);

    /* Flush output to make result available right away -JCG 7/29/97 */
    fflush (out);
}

/* Prints the md's contents to out in text format */
void MD_print_md (FILE *out, MD *md, int page_width)
{
    MD_Section *section;
    MD_Symbol *symbol;

    /* Print header for this md (as comment) */
    fprintf (out, "/*\n * %s:\n", md->name);
    fprintf (out, 
	     " *    Field declarations and entry contents for each section\n");
    fprintf (out, " */");

    /* Print out each section in the md */
    for (symbol = md->section_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get the section for ease of use */
	section = (MD_Section *) symbol->data;

	putc ('\n', out);

	/* Print out the section */
	MD_print_section (out, section, page_width);
    }

    /* Flush output to make result available right away -JCG 7/29/97 */
    fflush (out);
}

/*
 * Prints the section and entry declarations required to make the output of 
 * MD_print_md a valid hmdes file.  These declarations resolve the 
 * problem of circular LINKs that can occur between entries in md files.   
 * Print out all sections and entries (verses the minimum subset required)
 * to preserve section and entry order in md.
 * 
 * Print in a compact format since these declarationcs are not very 
 * informative.
 */
void MD_print_md_declarations (FILE *out, MD *md, int page_width)
{
    MD_Section *section;
    MD_Entry *entry, *next_entry;
    char *entry_name;
    int col, legal_ident, name_len;
    int comment_indent, cur_indent;


    /* Print header for this md (as comment) */
    fprintf (out, 
	     "/*\n * %s:\n",
	     md->name);
    fprintf (out, 
	     " *   Declaration of all section and entry names\n */");

    /* Determine where to print the '// Declaration' comment. 
     * Put it near column 39 unless page_width is too narrow.
     */
    comment_indent = 39;
    if (comment_indent > (page_width - 16))
	comment_indent = page_width - 16;

    /* Create each section in order */
    for (section = MD_first_section (md); section != NULL;
	 section = MD_next_section (section))
    {
	fprintf (out, "\nCREATE SECTION %s",
		 section->name);

	/* Calculate current indentation */
	cur_indent = 15 + strlen (section->name);

	/* Pad with spaces */
	while (cur_indent < comment_indent)
	{
	    putc (' ', out);
	    cur_indent++;
	}

	/* Print comment */
	fprintf (out, " // Declaration\n");
	fprintf (out, "{\n");

	/* Start out in column 0 */
	col = 0;

	/* Declare all the entries in each section */
	for (entry = MD_first_entry (section); entry != NULL;
	     entry = next_entry)
	{
	    /* Get next entry now for newline processing later */
	    next_entry = MD_next_entry (entry);

	    /* Get entry name for ease of use */
	    entry_name = entry->name;

	    /* Get length of name */
	    name_len = strlen (entry_name);

	    /* Is it a legal identifier? */
	    legal_ident = MD_legal_ident (entry_name);

	    /* If not a legal identifier, name will be two characters longer */
	    if (!legal_ident)
		name_len += 2;

	    /* Do we need to go to a newline before we print this name?
	     * (yes if not at beginning of line and will go past page_width)
	     */
	    if ((col != 0) && (col + name_len + 4) >= page_width)
	    {
		putc ('\n', out);
		col = 0;
	    }

	    /* Put extra space at beginning of each line */
	    if (col == 0)
	    {
		putc (' ', out);
		col = 1;
	    }

	    /* Print out the entry name (with quotes if not a legal ident) */
	    if (!MD_legal_ident(entry_name))
	    {
		fprintf (out, " '%s'();", entry_name);
	    }
	    else
	    {
		fprintf (out, " %s();", entry_name);
	    }
	    col += name_len + 4;

	}
	
	putc ('\n', out);
	putc ('}', out);
	putc ('\n', out);
    }

    

}

/* Prints the section and entry names in the md */
void MD_print_md_template (FILE *out, MD *md)
{
    MD_Section *section;
    MD_Symbol *symbol;
    int first_section;

    /* Print header for this md (as comment) */
    fprintf (out, "/*\n * %s template\n */\n", md->name);

    /* Mark that we are about to print out the first section */
    first_section = 1;

    /* Print out each section in the md */
    for (symbol = md->section_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get the section for ease of use */
	section = (MD_Section *) symbol->data;

	/* Print a newline before the section if not the first section */
	if (!first_section)
	    putc ('\n', out);

	/* Otherwise, mark that next section is not the first section */
	else
	    first_section = 0;

	/* Print out the section name and section entry's names*/
	MD_print_section_template (out, section);
    }
}


/* Creates a new section in the md with an initial size suitable for the 
 * specified number of entries and number of fields.  The section 
 * automatically resizes (upward) when appropriate, so 0 entries and/or
 * 0 may be specified.
 */
MD_Section *MD_new_section (MD *md, const char *name, int num_entries, 
			    int num_fields)
{
    MD_Section *section;
    char *field_decl_desc, *entry_desc;

    /* Allocate a new section structure */
    section = (MD_Section *) L_alloc (MD_Section_pool);

    /* Initialize the section's fields */
    section->md = md;
    section->name = MD_strdup (name);

    /* Initialize user's extension pointer to NULL */
    section->user_ext = NULL;

    /* Create a symbol table for this section's entries.
     * Create a good description, so symbol table error messages
     * (such as two symbol's with same name) are intelligible.
     */
    entry_desc = MD_concat_strings (name, "'s entry table");
    section->entry_table = MD_new_symbol_table (md, entry_desc, num_entries);
    free (entry_desc);


    /* Create a symbol table for this section's field declarations.
     * Create a good description, so symbol table error messages
     * (such as two symbol's with same name) are intelligible.
     */
    field_decl_desc = MD_concat_strings (name, "'s field declaration table");
    section->field_decl_table = MD_new_symbol_table (md, field_decl_desc, 
							num_fields);
    free (field_decl_desc);

    /* Initialize field decl fields to 0 fields then resize using
     * num_fields to get intial size specified in num_fields.
     */
    section->max_field_index = -1;
    section->field_decl = NULL;
    section->field_array_size = 0;

    /* Resize the array based on num_fields unless num_fields is specified
     * as 0.  (The entry_table must be built before this is called).
     */
    if (num_fields != 0)
    {
	MD_resize_field_arrays (section, num_fields);
    }


    /* Add section to the md's section table, place pointer of added
     * symbol into section structure for easy access.
     */
    section->symbol =  MD_add_symbol (md->section_table, section->name, 
				      (void *) section);

    /* Return the new section */
    return (section);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_num_sections()!
 * This is the function version of the macro MD_num_sections().
 * 
 * Used to allow arguments to be checked for macros.
 */
int _MD_num_sections (MD *md)
{
    /* Return number of sections found in the md's section table */
    return (md->section_table->symbol_count);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_find_section()!
 * This is the function version of the macro MD_find_section().
 * 
 * Used to allow arguments to be checked for macros.
 */
MD_Section *_MD_find_section (MD *md, const char *name)
{
    /* Find section in md table, return data cast as MD_Section */
    return ((MD_Section *) _MD_find_symbol_data(md->section_table, name));
}

/*
 * Returns a pointer to the first section in the MD or NULL if MD empty.
 */
MD_Section *MD_first_section (MD *md)
{
    MD_Symbol *symbol;
    
    symbol = md->section_table->head_symbol;

    if (symbol != NULL)
	return ((MD_Section *) symbol->data);
    else
	return (NULL);
}

/*
 * Returns a pointer to the last section in the MD or NULL if MD empty.
 */
MD_Section *MD_last_section (MD *md)
{
    MD_Symbol *symbol;
    
    symbol = md->section_table->tail_symbol;

    if (symbol != NULL)
	return ((MD_Section *) symbol->data);
    else
	return (NULL);
}

/* 
 * Returns a pointer to the next section in the MD or NULL if passed 
 * a pointer to the last section in the MD.
 */
MD_Section *MD_next_section (MD_Section *section)
{
    MD_Symbol *symbol;

    symbol = section->symbol->next_symbol;

    if (symbol != NULL)
	return ((MD_Section *) symbol->data);
    else
	return (NULL);
}

/* 
 * Returns a pointer to the previous section in the MD or NULL if passed 
 * a pointer to the first section in the MD.
 */
MD_Section *MD_prev_section (MD_Section *section)
{
    MD_Symbol *symbol;

    symbol = section->symbol->prev_symbol;

    if (symbol != NULL)
	return ((MD_Section *) symbol->data);
    else
	return (NULL);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_set_section_ext()! 
 * This is the function version of the macro MD_set_section_ext().
 * 
 * Used to allow arguments to be checked for macro.
 */
void *_MD_set_section_ext(MD_Section *section, void *ext) 
{
    section->user_ext = ext;

    /* Return ext to be equivalent to macro version */
    return (ext);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_get_section_ext()!
 * This is the function version of the macro MD_get_section_ext().
 * 
 * Used to allow arguments to be checked for macro.
 */
void *_MD_get_section_ext(MD_Section *section)
{
    return (section->user_ext);   
}

/* 
 * DO NOT CALL DIRECTLY!  Use macro MD_delete_section()!
 * Frees the memory associated with a section.
 * Used by MD_delete_section macro and MD_delete_md.
 * 
 * Assumes section has already been removed from the 
 * md's section_table.
 */
void _MD_free_section (MD_Section *section)
{
    char *name;

    /* Free name last (for error messages) */
    name = section->name;

    /* Free entry symbol table and all the information contained in it. */
    MD_delete_symbol_table (section->entry_table, 
			    (void (*)(void *))_MD_free_entry);

    /* Free field declaration table and all the information contained in it.
     * Must be called after freeing entries because the free entry
     * routine uses the field declaration
     */
    MD_delete_symbol_table (section->field_decl_table, 
			    (void (*)(void *))_MD_free_field_decl);

    /* Free the field_decl array (if it exists) */
    if (section->field_decl != NULL)
	free (section->field_decl);
    
    /* Free the section structure */
    L_free (MD_Section_pool, section);

    /* Free the section's name */
    free (name);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_delete_section()!
 * This is the function version of the macro MD_delete_section().
 * 
 * Used to allow arguments to be checked for macro.
 */
void _MD_delete_section (MD_Section *section)
{
    /* Delete symbol and structure memory */
    MD_delete_symbol (section->symbol, (void(*)(void *))_MD_free_section);
}


/*
 * DO NOT CALL DIRECTLY!  Use macro MD_check_section()!
 * This function checks the elements in the specified section.
 * Used by the MD_check_section() macro and MD_check_md().
 * 
 * Returns the number of required fields not specified + the 
 * number of elements with incorrect type in the section (0 if all ok).
 * 
 * If out is non-NULL, warning messages for non-specified required field
 * and for each element with incorrect type will be printed to out.
 *
 * The caller_name is used in the warning messages to identify the
 * routine/macro that started the checking process (MD_check_md or
 * MD_check_section).
 */
int _MD_check_section (FILE *out, MD_Section *section, const char *caller_name)
{
    MD_Symbol *symbol;
    MD_Entry *entry;
    int error_count;

    /* Initialize error count */
    error_count = 0;

    /* Scan every entry in the section for errors */
    for (symbol = section->entry_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get the entry pointed to by this symbol */
	entry = (MD_Entry *) symbol->data;

	/* Sum up all the errors found by check entry */
	error_count += _MD_check_entry (out, entry, caller_name);
    }


    /* Return the number of errors found in the section */
    return (error_count);
}

/* Prints the section's contents to out in text format */
void MD_print_section (FILE *out, MD_Section *section, int page_width)
{
    MD_Field_Decl *field_decl;
    MD_Symbol *symbol;
    MD_Entry *entry;

    fprintf (out, "SECTION %s\n", section->name);
    /* Print every field declaration in section */
    for (symbol = section->field_decl_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get the field_decl pointed to by this symbol */
	field_decl = (MD_Field_Decl *) symbol->data;

	/* Print the field declaration */
	MD_print_field_decl (out, field_decl, page_width);
    }
    fprintf (out, "{\n");

    /* Print every entry in section */
    for (symbol = section->entry_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get the entry pointed to by this symbol */
	entry = (MD_Entry *) symbol->data;

	/* Print the entry */
	MD_print_entry (out, entry, page_width);
    }

    fprintf (out, "}\n");
}

/* Prints the section's entry names (only) in text format */
void MD_print_section_template (FILE *out, MD_Section *section)
{
    MD_Symbol *symbol;
    MD_Entry *entry;

    fprintf (out, "SECTION %s\n{\n", section->name);

    /* Print every entry in section */
    for (symbol = section->entry_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get the entry pointed to by this symbol */
	entry = (MD_Entry *) symbol->data;

	/* Print the entry */
	MD_print_entry_template (out, entry);
    }

    fprintf (out, "}\n");
}

/* 
 * Creates a new entry in a section. 
 */
MD_Entry *MD_new_entry (MD_Section *section, const char *name)
{
    MD_Entry *entry;
    int field_array_size;
    MD_Field **field_array;
    int i;

    /* Allocate a new entry structure */
    entry = (MD_Entry *) L_alloc (MD_Entry_pool);

    /* Initialize the entry's fields */
    entry->section = section;
    entry->name = MD_strdup (name);

    /* Initialize user's extension pointer to NULL */
    entry->user_ext = NULL;

    /* Get field array size from section for ease of use */
    field_array_size = section->field_array_size;

    /* Allocate field array if at least one field has been declared */
    if (field_array_size > 0)
    {
	field_array = (MD_Field **) malloc (sizeof(MD_Field *) * 
					    field_array_size);
	if (field_array == NULL)
	{
	    MD_punt (section->md, "MD_new_entry: Out of memory (size %i)",
		     field_array_size);
	}

	/* Initialize field array with NULL pointers */
	for (i=0; i < field_array_size; i++)
	    field_array[i] = NULL;

	/* Point field at new field array */
	entry->field = field_array;
    }

    /* Otherwise, point field array at NULL */
    else
    {
	entry->field = NULL;
    }

    /* Add entry to the section's entry table, place pointer of added
     * symbol into entry structure for easy access.
     */
    entry->symbol =  MD_add_symbol (section->entry_table, entry->name, 
				    (void *) entry);

    /* Return the new entry */
    return (entry);
}

/* Renames the entry but otherwise the entry is unchanged */
void MD_rename_entry (MD_Entry *entry, const char *new_name)
{
    char *old_name;

    /* Rename entry but don't free old_name yet.  The symbol is still
     * pointing to the old name, and it cannot be freed until after
     * MD_rename_symbol!
     */
    old_name = entry->name;
    entry->name = MD_strdup (new_name);

    /* Rename in the symbol table (will point at entry->name, not a copy) */
    MD_rename_symbol (entry->symbol, entry->name);

    /* Free old name now that it is safe to do so */
    free (old_name);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_num_entries()!
 * This is the function version of the macro MD_num_entries().
 * 
 * Used to allow arguments to be checked for macros.
 */
int _MD_num_entries (MD_Section *section)
{
    /* Return number of entries found in the section's entry table */
    return (section->entry_table->symbol_count);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_find_entry()!
 * This is the function version of the macro MD_find_entry().
 * 
 * Used to allow arguments to be checked for macros.
 */
MD_Entry *_MD_find_entry (MD_Section *section, const char *name)
{
    /* Find entry in section table, return data cast as MD_Entry */
    return ((MD_Entry *) _MD_find_symbol_data(section->entry_table, name));
}

/*
 * Returns a pointer to the first entry in the section or NULL if the section
 * is empty.
 */
MD_Entry *MD_first_entry (MD_Section *section)
{
    MD_Symbol *symbol;
    
    symbol = section->entry_table->head_symbol;

    if (symbol != NULL)
	return ((MD_Entry *) symbol->data);
    else
	return (NULL);
}

/*
 * Returns a pointer to the last entry in the section or NULL if the section
 * is empty.
 */
MD_Entry *MD_last_entry (MD_Section *section)
{
    MD_Symbol *symbol;
    
    symbol = section->entry_table->tail_symbol;

    if (symbol != NULL)
	return ((MD_Entry *) symbol->data);
    else
	return (NULL);
}

/* 
 * Returns a pointer to the next entry in the section or NULL if passed 
 * a pointer to the last entry in the section.
 */
MD_Entry *MD_next_entry (MD_Entry *entry)
{
    MD_Symbol *symbol;

    symbol = entry->symbol->next_symbol;

    if (symbol != NULL)
	return ((MD_Entry *) symbol->data);
    else
	return (NULL);
}

/* 
 * Returns a pointer to the previous entry in the section or NULL if passed 
 * a pointer to the first entry in the section.
 */
MD_Entry *MD_prev_entry (MD_Entry *entry)
{
    MD_Symbol *symbol;

    symbol = entry->symbol->prev_symbol;

    if (symbol != NULL)
	return ((MD_Entry *) symbol->data);
    else
	return (NULL);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_set_entry_ext()! 
 * This is the function version of the macro MD_set_entry_ext().
 * 
 * Used to allow arguments to be checked for macro.
 */
void *_MD_set_entry_ext(MD_Entry *entry, void *ext) 
{
    entry->user_ext = ext;

    /* Return ext to be equivalent to macro version */
    return (ext);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_get_entry_ext()!
 * This is the function version of the macro MD_get_entry_ext().
 * 
 * Used to allow arguments to be checked for macro.
 */
void *_MD_get_entry_ext(MD_Entry *entry)
{
    return (entry->user_ext);   
}
 
/* 
 * DO NOT CALL DIRECTLY!  Use macro MD_delete_entry()!
 * Frees the memory associated with a entry.
 * Used by MD_delete_entry macro and MD_delete_section.
 * 
 * Assumes entry has already been removed from the 
 * section's entry_table.
 */
void _MD_free_entry (MD_Entry *entry)
{
    char *name;
    int field_index, max_field_index;
    MD_Field **field_array, *field;

    /* Free name last (for error messages) */
    name = entry->name;

    /* Get the max_field_index and field array for ease of use */
    max_field_index = entry->section->max_field_index;
    field_array = entry->field;

    /* Free all the fields specified in field_array, 
     * Assumes max_field_index == -1 if field_array is NULL
     */
    for (field_index=0; field_index <= max_field_index; field_index++)
    {
	field = field_array[field_index];
	if (field != NULL)
	    MD_delete_field (field);
    }

    /* Free the field array (if exists)*/
    if (field_array != NULL)
	free (field_array);
 
    /* Free the entry structure */
    L_free (MD_Entry_pool, entry);

    /* Free the entry's name */
    free (name);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_delete_entry()!
 * This is the function version of the macro MD_delete_entry().
 * 
 * Used to allow arguments to be checked for macro.
 */
void _MD_delete_entry (MD_Entry *entry)
{
    /* Delete symbol and structure memory */
    MD_delete_symbol (entry->symbol, (void(*)(void *))_MD_free_entry);
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_check_entry()!
 * This function checks the elements in the specified entry.
 * Used by the MD_check_entry() macro and _MD_check_section().
 * 
 * Returns the number of required fields not specified + the 
 * number of elements with incorrect type (0 if all ok).
 * 
 * If out is non-NULL, warning messages for non-specified required field
 * and for each element with incorrect type will be printed to out.
 *
 * The caller_name is used in the warning messages to identify the
 * routine/macro that started the checking process (MD_check_md, 
 * MD_check_section, or MD_check_entry).
 */
int _MD_check_entry (FILE *out, MD_Entry *entry, const char *caller_name)
{
    MD_Section *section;
    MD_Field_Decl *field_decl, **field_decl_array;
    MD_Field *field;
    int max_field_index, field_index;
    int error_count;

    /* Initialize the error count */
    error_count = 0;

    /* Get the various pointers into local variables for ease of use */
    section = entry->section;
    field_decl_array = section->field_decl;
    max_field_index = section->max_field_index;

    /* Check all the fields in the array */
    for (field_index = 0; field_index <= max_field_index; field_index ++)
    {
	/* Get the field declaration for this field */
	field_decl = field_decl_array[field_index];

	/* If there is no field declared for this index, goto next index */
	if (field_decl == NULL)
	    continue;

	/* Get the entry's field at this index */
	field = entry->field[field_index];

	/* Check the elements in the field if it exists */
	if (field != NULL)
	{
	    /* Add in any error found by check_field */
	    error_count += _MD_check_field (out, field, caller_name);
	}
	
	/* Otherwise warn if this field is required to be present */
	else if (field_decl->type == MD_REQUIRED_FIELD)
	{
	    error_count++;
	    if (out != NULL)
	    {
		MD_warn (out,
			 "%s(%s->%s->%s):\n  Warning, required field '%s' unspecified for entry '%s'.",
			 caller_name, section->name, entry->name, 
			 field_decl->name, field_decl->name, entry->name);
	    }
	}
    }

    /* Return the number of errors detected */
    return (error_count);
}

/* Returns 1 if a legal identifier (C identifier), 0 otherwise */
static int MD_legal_ident (const char *ident)
{

    const char *ptr;

    /* Must start with letter of '_' (may not be empty string)*/
    if ((ident[0] != '_') && !isalpha (ident[0]))
        return (0);

    /* Rest of string must be alphnumeric or '_' */
    for (ptr = &ident[1]; *ptr != 0; ptr++)
    {
        if ((*ptr != '_') && !isalnum (*ptr))
            return (0);
    }

    /* Return legal */
    return (1);
}

/* Prints out a textual representation of the entry to out 
 * Called by MD_print_section().
 */
void MD_print_entry (FILE *out, MD_Entry *entry, int page_width)
{
    MD_Field **field_array, *field;
    MD_Field_Decl *field_decl;
    MD_Symbol *symbol;
    MD_Section *section;
    MD_Element **element_array, *element;
    char *entry_name, *field_name, *link_name;
    int effective_name_len, field_name_indent, first_field;
    int element_value_indent, value_len, print_column;
    int element_index, first_element;
    int max_assigned_index, link_valid_ident = 0; /* Avoid compiler warning */
    unsigned int block_size, block_index, block_byte, nibble, nibble_char;
    unsigned char *block_ptr;
    char value_buf[500];
    int i;

    /* Set the indentation for the field names.
     * A setting of 17 allows names of length 14 on the same line
     * as first field.  Also puts openning '(' at 2 tabs in.
     */
    field_name_indent = 17;
    
    /* Get pointers for ease of use */
    entry_name = entry->name;
    field_array = entry->field;
    section = entry->section;

    /* Print out the entry name (with quotes if not a legal identifier) */
    if (!MD_legal_ident(entry_name))
    {
	fprintf (out, "  '%s'", entry_name);
	
	/* Calculate the effective entry name length 
	 * (name_len + 2 for indent, 2 for quotes)
	 */
	effective_name_len = strlen (entry_name) + 4;
    }
    else
    {
	fprintf (out, "  %s", entry_name);

	/* Calculate the effective entry name length 
	 * (name_len + 2 for indent)
	 */
	effective_name_len = strlen (entry_name) + 2;
    }
    
    /* Determine if can place first field on same line as entry name 
     * (Want to place a '(' after the entry name)
     */
    if ((effective_name_len + 1) > field_name_indent)
    {
	/* No, force newline and indent */
	putc ('\n', out);

	/* Indent, leaving space for '(' */
	for (i=1; i < field_name_indent; i++)
	    putc (' ', out);
    }
    else
    {
	/* Yes, pad to place to place '(' */
	for (i=effective_name_len + 1; i < field_name_indent; i++)
	    putc (' ', out);
    }
    
    /* Put the opening '(' */
    putc ('(', out);

    /* Mark that we are about to process the first field */
    first_field = 1;

    /* Print out fields in the order declared 
     * (verses the order they are in the field table)
     */
    for (symbol = section->field_decl_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get the field declaration for this symbol */
	field_decl = (MD_Field_Decl *) symbol->data;

	/* Get the entry's field for this declaration */
	field = field_array[field_decl->field_index];

	/* If field has not been specified for this entry, goto next field */
	if (field == NULL)
	    continue;

	/* Need to indent field name if this is not the first field */
	if (!first_field)
	{
	    /* Goto next line */
	    putc ('\n', out);

	    /* Indent field name */
	    for (i=0; i < field_name_indent; i++)
		putc (' ', out);
	}
	/* Otherwise mark that next field will not be the first field */
	else
	{
	    first_field = 0;
	}
	
	/* Get the field name for ease of use */
	field_name = field_decl->name;

	/* Get where all the element values should be indented (if needed) */
	element_value_indent = field_name_indent + strlen(field_name) + 1;

	/* Print the field's name and opening '('*/
	fprintf (out, "%s(", field_name);

	/* Set our print column to the current position */
	print_column = element_value_indent;

	/* Get the element array for ease of use */
	element_array = field->element;

	/* Make max_assigned_index point at the last element set to a value*/
	max_assigned_index = field->max_element_index;
	while ((max_assigned_index >= 0) &&
	       (element_array[max_assigned_index] == NULL))
	    max_assigned_index--;
	
	/* Mark that we are about to print the first element value 
	 * on this line
	 */
	first_element = 1;

	/* Print out all the element values for this field */
	for (element_index = 0; element_index <= max_assigned_index; 
	     element_index ++)
	{
	    /* Get the element for ease of use */
	    element = element_array[element_index];

	    /* Calculate the length of the value for this element.
	     * Place the string for INT and DOUBLE in value_buf.
	     */
	    if (element == NULL)
		value_len = 2;
	    else
	    {
		switch (element->type)
		{
		  case MD_INT:
		    /* Print int to buffer */
		    sprintf (value_buf, "%i", element->value.i);
		    value_len = strlen (value_buf);
		    break;

		  case MD_DOUBLE:
		    /* Print double to buffer with 10 trailing digits
		     * of precision and strip off trailing zeros 
		     * (min 1 trailing zero)
		     */
		    sprintf (value_buf, "%.10f", element->value.d);
		    value_len = strlen (value_buf);
		    while (value_len >= 2)
		    {
			/* Stop at first non-zero value hit or
			 * at one zero before decimal
			 */
			if ((value_buf[value_len-1] != '0') ||
			    (value_buf[value_len-2] == '.'))
			    break;
			
			/* Remove trailing zero */
			value_len--;
			value_buf[value_len] = 0;
		    }
		    break;

		  case MD_STRING:
		    /* Will need quotes around string */
		    value_len = strlen(element->value.s) + 2;
		    break;

		  case MD_BLOCK:
		    /* Will need single quotes around binary data */
		    value_len = element->value.b.size + 2;
		    break;

		  case MD_LINK:
		    /* Quotes are needed around name if not valid ident */
		    link_name = element->value.l->name;
		    link_valid_ident = MD_legal_ident (link_name);
		    if (link_valid_ident)
			value_len = strlen(link_name);
		    else
			value_len = strlen(link_name) + 2;
		    break;

		  default:
		    MD_punt (entry->section->md,
			     "MD_print_entry: Unknown element type '%i'",
			     element->type);
		    value_len = 0;      /* Avoid compiler warning */
		}
	    }
	    
	    /*
	     * If not first element, see if can fit on this line or if need
	     * to goto the next line.  
	     * (Always print first element on same line)
	     */
	    if (!first_element)
	    {
		/* See if we will exceed page width if this value is printed
		 * out, leaving 4 spaces for trailing ')); '
		 */
		if ((print_column + value_len) >= (page_width - 4))
		{
		    /* Goto next line */
		    putc ('\n', out);

		    /* Indent to element_value_indent */
		    for (i=0; i < element_value_indent; i++)
			putc (' ', out);

		    /* Reset print_column */
		    print_column = element_value_indent;
		}

		/* Otherwise, just print space after last value */
		else
		{
		    putc (' ', out);

		    /* Update print_column */
		    print_column++;
		}
	    }
	    /* Otherwise mark that next element will not be the first element
	     * on this line
	     */
	    else
	    {
		first_element = 0;
	    }

	    /* Update print column */
	    print_column += value_len;


	    /* 
	     * Print out the element's value.
	     * INT, DOUBLE, and LINK use info from the value_len calculation.
	     */
	    if (element == NULL)
	    {
		fprintf (out, "()");
		continue;
	    }

	    switch (element->type)
	    {
		/* For INT and DOUBLE, value placed in value buffer */
	      case MD_INT:
	      case MD_DOUBLE:
		fprintf (out, "%s", value_buf);
		break;

	      case MD_STRING:
		/* Print out string with '"' to identify it as a string */
		fprintf (out, "\"%s\"", element->value.s);
		break;

	      case MD_BLOCK:
		/* Print out binary data as a string of hex characters,
		 * two hex characters per byte.  Put "'" around binary
		 * data to provide hint that not string data.  However
		 * it might look like a link (sigh).
		 */
		putc ('\'', out);

		/* Get block size and pointer for ease of use */
		block_size = element->value.b.size;
		block_ptr = (unsigned char *)element->value.b.ptr;

		/* Print out each byte as two hex characters */
		for (block_index = 0; block_index < block_size; block_index++)
		{
		    /* Get the byte to print out */
		    block_byte = block_ptr[block_index];

		    /* Print out the high nibble */
		    nibble = block_byte >> 4;

		    if (nibble < 10)
			nibble_char = '0' + nibble;
		    else
			nibble_char = 'a' + (nibble - 10);

		    putc (nibble_char, out);

		    /* Print out the low nibble */
		    nibble = block_byte & 0x0f;

		    if (nibble < 10)
			nibble_char = '0' + nibble;
		    else
			nibble_char = 'a' + (nibble - 10);

		    putc (nibble_char, out);
		}
		putc ('\'', out);
		break;
		 
	      case MD_LINK:
		/* Print out name of entry linked to.  Put "'" around
		 * name if name not a valid identifier.
		 */
		link_name = element->value.l->name;
		if (link_valid_ident)
		    fprintf (out, "%s", link_name);
		else
		    fprintf (out, "'%s'", link_name);
		break;

	      default:
		MD_punt (entry->section->md,
			 "MD_print_entry: Unknown element type '%i'",
			 element->type);
		
	    }
	}
	/* Closing out field */
	fprintf (out, ")");
    }

    /* Closing out entry */
    fprintf (out, ");\n");
}

/* Prints out a textual template for the entry (Entry name and (); only).
 *
 * Useful for listing the entries in a section/md.
 *
 * Called by MD_print_section_template().
 */
void MD_print_entry_template (FILE *out, MD_Entry *entry)
{
    char *entry_name;
    int effective_name_len, field_name_indent;
    int i;

    /* Set the indentation for the field names.
     * A setting of 17 allows names of length 14 on the same line
     * as first field.  Also puts openning '(' at 2 tabs in.
     */
    field_name_indent = 17;
    
    /* Get pointers for ease of use */
    entry_name = entry->name;

    /* Print out the entry name (with quotes if not a legal identifier) */
    if (!MD_legal_ident(entry_name))
    {
	fprintf (out, "  '%s'", entry_name);
	
	/* Calculate the effective entry name length 
	 * (name_len + 2 for indent, 2 for quotes)
	 */
	effective_name_len = strlen (entry_name) + 4;
    }
    else
    {
	fprintf (out, "  %s", entry_name);

	/* Calculate the effective entry name length 
	 * (name_len + 2 for indent)
	 */
	effective_name_len = strlen (entry_name) + 2;
    }
    
    /* Always (); on same line as entry name */
    /* Pad if necessary*/
    for (i=effective_name_len + 1; i < field_name_indent; i++)
	putc (' ', out);

    
    /* Put the opening and closing '();' */
    putc ('(', out);
    putc (')', out);
    putc (';', out);
    putc ('\n', out);
}

/* Creates a new field declaration in the specified section. */
MD_Field_Decl *MD_new_field_decl (MD_Section *section, const char *name, 
				  MD_FIELD_TYPE field_type)
{
    MD_Field_Decl *field_decl, **decl_array;
    int new_index, max_index;

    /* Allocate a new field declaration structure */
    field_decl = (MD_Field_Decl *) L_alloc (MD_Field_Decl_pool);

    /* Initialize the field declaration's fields */
    field_decl->section = section;
    field_decl->name = MD_strdup (name);
    field_decl->type = field_type;

    field_decl->max_require_index = -1;
    field_decl->require = NULL;           /* Initially no requirements */
    field_decl->require_array_size = 0;
    field_decl->kleene_starred_req= NULL; /* Initially no starred requirement*/

    /* Search for next available field index in section */
    max_index = section->max_field_index;
    decl_array = section->field_decl;
    for (new_index=0; new_index <= max_index; new_index ++)
    {
	/* If slot in field_decl is empty, take it */
	if (decl_array[new_index] == NULL)
	    break;
    }

    /* If new index exceeds current array size, resize field_decl array */
    if (new_index >= section->field_array_size)
	MD_resize_field_arrays (section, new_index);

    /* Place field declaration in new slot
     * Must use section->field_decl since the above resize call will change
     * the array!
     */
    section->field_decl[new_index] = field_decl;
    field_decl->field_index = new_index;

    /* Update max_field_index if necessary */
    if (section->max_field_index < new_index)
	section->max_field_index = new_index;

    /* Add to section's field declaration table, place pointer of added
     * symbol into section structure for easy access.
     */
    field_decl->symbol = MD_add_symbol (section->field_decl_table, 
					field_decl->name, (void *) field_decl);

    /* Return the new field declaration */
    return (field_decl);
}

/* Resizes the field_decl array in the section structure
 * and the field arrays in all the entries in the section.
 * 
 * Will allocate an array at least two bigger than what is needed for
 * max_index and will always round up to the next even number
 * (malloc has to align to a 8 byte boundary anyway, might as well
 *  use the memory).
 */
static void MD_resize_field_arrays (MD_Section *section, int max_index)
{
    MD_Field_Decl **new_decl_array, **old_decl_array;
    MD_Symbol *symbol;
    MD_Entry *entry;
    MD_Field **new_field_array, **old_field_array;
    int max_field_index, new_array_size, i;

    /* Increase size by first rounding up to the nearest odd number,
     * then adding 3 to it so that the new_array_size is always even
     * and has space for another 2 or 3 field declarations.
     */
    new_array_size = (max_index | 1) + 3;

    /* Sanity check */
    if (new_array_size <= section->field_array_size)
    {
	MD_punt (section->md,
		 "MD_resize_field_arrays: new_array_size (%i) must be > current size (%i)",
		 new_array_size, section->field_array_size);
    }

    /* Get the max_field_index for ease of use */
    max_field_index = section->max_field_index;

    /* 
     * Resize the field declaration array 
     */

    /* Create new field decl array */
    new_decl_array = (MD_Field_Decl **) malloc (sizeof (MD_Field_Decl *) * 
						new_array_size);
    if (new_decl_array == NULL)
    {
	MD_punt (section->md, 
		 "MD_resize_field_arrays: Out of memory (size %i)",
		 new_array_size);
    }

    /* Get pointer to old array */
    old_decl_array = section->field_decl;

    /* Copy over pointers from existing array 
     * (Assumes max_field_index is -1 if old array is NULL).
     */
    for (i=0; i <= max_field_index; i++)
	new_decl_array[i] = old_decl_array[i];

    /* Initialize remaining pointers to NULL */
    for (i=max_field_index + 1; i < new_array_size; i++)
	new_decl_array[i] = NULL;

    /* Free old array, if not NULL */
    if (old_decl_array != NULL)
	free (old_decl_array);
    
    /* Install new field declaration array */
    section->field_decl = new_decl_array;
    section->field_array_size = new_array_size;

    /*
     * Resize all the field array in all the section's entries
     */

    /* Process each entry in the section */
    for (symbol = section->entry_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get a pointer to this entry */
	entry = (MD_Entry *) symbol->data;

	/* Create new field array for this entry */
	new_field_array = (MD_Field **) malloc (sizeof (MD_Field *) * 
						new_array_size);
	if (new_field_array == NULL)
	{
	    MD_punt (section->md, 
		     "MD_resize_field_arrays: Out of memory (size %i)",
		     new_array_size);
	}
	
	/* Get pointer to old array for ease of use */
	old_field_array = entry->field;
	
	/* Copy over pointers from existing array 
	 * (Assumes max_field_index is -1 if old array is NULL).
	 */
	for (i=0; i <= max_field_index; i++)
	    new_field_array[i] = old_field_array[i];
	
	/* Initialize remaining pointers to NULL */
	for (i=max_field_index + 1; i < new_array_size; i++)
	    new_field_array[i] = NULL;
	
	/* Free old array, if not NULL */
	if (old_field_array != NULL)
	    free (old_field_array);
	
	/* Install new field array into entry */
	entry->field = new_field_array;
    }
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_num_field_decls()!
 * This is the function version of the macro MD_num_field_decls().
 * 
 * Used to allow arguments to be checked for macros.
 */
int _MD_num_field_decls (MD_Section *section)
{
    /* Return number of declarations found in the section's field decl table */
    return (section->field_decl_table->symbol_count);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_max_field_index()!
 * This is the function version of the macro MD_max_field_index().
 * 
 * Used to allow arguments to be checked for macros.
 */
int _MD_max_field_index (MD_Section *section)
{
    /* Return the index of the max declared field */
    return (section->max_field_index);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_find_field_decl()!
 * This is the function version of the macro MD_find_field_decl().
 * 
 * Used to allow arguments to be checked for macros.
 */
MD_Field_Decl *_MD_find_field_decl (MD_Section *section, const char *name)
{
    /* Find field_decl in section table, return data cast as MD_Field_Decl */
    return ((MD_Field_Decl *) _MD_find_symbol_data(section->field_decl_table,
					      name));
}

/*
 * Returns a pointer to the first field declaration in the section or
 * NULL if no fields are declared for the section.
 */
MD_Field_Decl *MD_first_field_decl (MD_Section *section)
{
    MD_Symbol *symbol;
    
    symbol = section->field_decl_table->head_symbol;

    if (symbol != NULL)
	return ((MD_Field_Decl *) symbol->data);
    else
	return (NULL);
}

/*
 * Returns a pointer to the last field declaration in the section or
 * NULL if no fields are declared for the section.
 */
MD_Field_Decl *MD_last_field_decl (MD_Section *section)
{
    MD_Symbol *symbol;
    
    symbol = section->field_decl_table->tail_symbol;

    if (symbol != NULL)
	return ((MD_Field_Decl *) symbol->data);
    else
	return (NULL);
}

/* 
 * Returns a pointer to the next field declaration in the section or
 * NULL if passed a pointer to the last field declaration in the section
 */
MD_Field_Decl *MD_next_field_decl (MD_Field_Decl *field_decl)
{
    MD_Symbol *symbol;

    symbol = field_decl->symbol->next_symbol;

    if (symbol != NULL)
	return ((MD_Field_Decl *) symbol->data);
    else
	return (NULL);
}

/* 
 * Returns a pointer to the previous field declaration in the section or
 * NULL if passed a pointer to the first field declaration in the section
 */
MD_Field_Decl *MD_prev_field_decl (MD_Field_Decl *field_decl)
{
    MD_Symbol *symbol;

    symbol = field_decl->symbol->prev_symbol;

    if (symbol != NULL)
	return ((MD_Field_Decl *) symbol->data);
    else
	return (NULL);
}



/* 
 * DO NOT CALL DIRECTLY!  Use macro MD_delete_field_decl()!
 * Frees the memory associated with a field declaration.
 * Used by MD_delete_field_decl macro and MD_delete_section.
 * 
 * Assumes field declaration has already been removed from the 
 * section's field_decl_table.
 * 
 * Does not free this field's data in all the section's entries.
 * This is done externally by _MD_free_section() and MD_delete_field_decl().
 */
void _MD_free_field_decl (MD_Field_Decl *field_decl)
{
    char *name;
    MD_Element_Req **require, *element_req;
    int i, max_require_index;
    
    /* Free name last (for error messages) */
    name = field_decl->name;

    /* Remove field_decl from section's field_decl array */
    field_decl->section->field_decl[field_decl->field_index] = NULL;

    /* Get the require array and max index for ease of use */
    require = field_decl->require;
    max_require_index = field_decl->max_require_index;

    /* Delete element requirements in the require array
     * (Assumes max_require_index is -1 if require array is NULL) */
    for (i=0; i <= max_require_index; i++)
    {
	/* Get element to free */
	element_req = require[i];

	/* Free element_req description */
	free (element_req->desc);

	/* If a link, free link array */
	if (element_req->type == MD_LINK)
	    free (element_req->link);

	/* Free element structure using L_free */
	L_free (MD_Element_Req_pool, element_req);
    }

    /* Free the require array (if exists) */
    if (require != NULL)
	free (require);
 
    /* Free the field declaration structure */
    L_free (MD_Field_Decl_pool, field_decl);

    /* Free the field_decl's name */
    free (name);
}

/* Deletes a field declaration and deletes all the data for this
 * field from all the entries in the section.
 */
void MD_delete_field_decl (MD_Field_Decl *field_decl)
{
    MD_Section *section;
    MD_Symbol *symbol;
    MD_Entry *entry;
    MD_Field *field;
    int field_index;

    /* Get the section for ease of use */
    section = field_decl->section;
    
    /* The the index of this field for ease of use */
    field_index = field_decl->field_index;

    /* Scan every entry in section and delete any info associated
     * with this name.  Must delete before freeing field decl
     * becuase MD_delete_field using field_decl info.
     */
    for (symbol = section->entry_table->head_symbol; symbol != NULL;
	 symbol = symbol->next_symbol)
    {
	/* Get the entry for ease of use */
	entry = (MD_Entry *) symbol->data;

	/* Get the field pointer from the field array */
	field = entry->field[field_index];

	if (field != NULL)
	{
	    MD_delete_field (field);
	}
    }

    /* Delete symbol and structure memory */
    MD_delete_symbol (field_decl->symbol, 
		      (void(*)(void *))_MD_free_field_decl);
}

/* Prints out a textual representation of a field declaration.
 * Called by MD_print_section().
 */
void MD_print_field_decl (FILE *out, MD_Field_Decl *field_decl, int page_width)
{
    MD_Element_Req **require_array, *require;
    MD_FIELD_TYPE type;
    char *field_name;
    int decl_len;
    int max_require_index, require_index, first_requirement;
    int require_indent,  print_column, i;

    /* Get various values into local variables for ease of use */
    type = field_decl->type;
    require_array = field_decl->require;
    max_require_index = field_decl->max_require_index;
    field_name = field_decl->name;
    
    /* Print out the type of field this is */
    if (type == MD_REQUIRED_FIELD)
    {
	fprintf (out, "  REQUIRED ");
    }
    else if (type == MD_OPTIONAL_FIELD)
    {
	fprintf (out, "  OPTIONAL ");
    }
    else
    {
	MD_punt (field_decl->section->md,
		 "MD_print_field_decl: Unknown field decl type '%i'",type);
    }

    /* Print out field name and openning '(' */
    fprintf (out, "%s(", field_name);

    /* Get the indentation to line up after openning '(' */
    require_indent = strlen (field_name) + 12;

    /* Get the current column position */
    print_column = require_indent;
    
    /* Mark that we are about to process the first requirement */
    first_requirement = 1;

    /* Print out the requirements for the field */
    for (require_index = 0; require_index <= max_require_index; 
	 require_index++)
    {
	/* Get this requirment */
	require = require_array[require_index];

	/* Get the length of the requirement declaration */
	decl_len = strlen (require->desc);

	
	/* 
	 * If not first requirement, see if can fit this requirement on
	 * this line or if need to goto next line.
	 * (Always print the first requirement on each line)
	 */
	if (!first_requirement)
	{
	    /* See if we will exceed page width if this decl is printed out,
	     * leaving 3 spaces for trailing '); '
	     */
	    if ((print_column + decl_len) >= (page_width - 3))
	    {
		/* Goto next line */
		putc ('\n', out);

		/* Indent to line of with the declaration above */
		for (i=0; i < require_indent; i++)
		    putc (' ', out);

		/* Reset print_column */
		print_column = require_indent;
	    }

	    /* Otherwise, just print space after last requirment */
	    else
	    {
		putc (' ', out);
		print_column ++;
	    }
	}

	/* Otherwise, mark that the next requirement will not be first 
	 * requirement on this line.
	 */
	else
	{
	    first_requirement = 0;
	}

	/* Update print_column */
	print_column += decl_len;

	/* Print out description of requirement */
	fprintf (out, "%s", require->desc);
    }
    
    /* Closing out field declaration */
    fprintf (out, ");\n");
}

/* Creates a new element requirement in the field declaration at
 * at the specified element index.
 * 
 * These element requirements must in order, starting from index 0.
 * For example, a requirment at index 1 cannot be created until
 * the requirement for index 0 is created.
 * 
 * These element requirements specify what type of data must go at
 * that index and that there must be data at that index.
 * These two properties are designed to reduce the error checking the 
 * md user must do.
 *
 * The name of the calling routine is passed in for error messages.
 * 
 * Returns a pointer to the new element_req structure 
 */
static MD_Element_Req *MD_new_element_req (MD_Field_Decl *field_decl, 
					   int index, const char *caller_name)
{
    MD_Element_Req *element_req, **old_require_array, **new_require_array;
    int i, new_array_size, max_require_index;

    /* Get max_require_index for ease of use */
    max_require_index = field_decl->max_require_index;

    /* Check that the index is valid, must be defining
     * the requirement just past the current requirements.
     */
    if (index != (max_require_index + 1))
    {
	/* Give error message for redefinition */
	if (index <= max_require_index)
	{
	    MD_punt (field_decl->section->md,
		     "%s(%s->*->%s[%i]):\n  Element %i already required to be '%s'!",
		     caller_name, field_decl->section->name, field_decl->name, 
		     index, index, field_decl->require[index]->desc);
	}
	/* Otherwise, give error message for non-sequential definition */
	else
	{
	    MD_punt (field_decl->section->md,
		     "%s(%s->*->%s[%i]):\n  Specify requirements for elements before %i first!",
		     caller_name, field_decl->section->name, field_decl->name, 
		     index, index);
	}
    }
    
    /* Check if trying to set a requirement past a kleene_starred
     * requirement.
     */
    if (field_decl->kleene_starred_req != NULL)
    {
	MD_punt (field_decl->section->md,
		 "%s(%s->*->%s[%i]):\n  Cannot specify requirements after a kleene starred requirement!",
		 caller_name, field_decl->section->name, field_decl->name, 
		 index);
    }

    /* Resize require array if necessary */
    if (index >= field_decl->require_array_size)
    {
	/* Increase size by first rounding up to the nearest odd number,
	 * then adding 3 to it so that the new_require_array_size is always 
	 * even and has space for another 2 or 3 requirements.
	 */
	new_array_size = (index | 1) + 3;

	/* Allocate new array */
	new_require_array =(MD_Element_Req **)malloc(sizeof(MD_Element_Req *) *
						     new_array_size);
	if (new_require_array == NULL)
	{
	    MD_punt (field_decl->section->md,
		     "Out of memory resizing require array (size %i).",
		     new_array_size);
	}

	/* Get old array ease of use */
	old_require_array = field_decl->require;

	/* Copy over pointers from existing array
	 * (Assumes max_require_index is -1 if old array is NULL).
	 */
	for (i=0; i <= max_require_index; i++)
	    new_require_array[i] = old_require_array[i];

	/* Initialize remaining pointers to NULL */
	for (i=max_require_index + 1; i < new_array_size; i++)
	    new_require_array[i] = NULL;

	/* Free old array, if not NULL */
	if (old_require_array != NULL)
	    free (old_require_array);

	/* Install new require array */
	field_decl->require = new_require_array;
	field_decl->require_array_size = new_array_size;
    }

    /* Allocate element_req structure */
    element_req = (MD_Element_Req *) L_alloc (MD_Element_Req_pool);

    /* Initialize fields other than type and desc which should
     * always set by caller.
     */
    element_req->field_decl = field_decl;
    element_req->require_index = index;
    element_req->kleene_starred = 0; /* Initially not kleene_starred */
    element_req->link = NULL;
    element_req->link_array_size = 0;

    /* Place in require structure at correct index */
    field_decl->require[index] = element_req;

    /* Update max index (checking at top of function requires it to
     * be the new max)
     */
    field_decl->max_require_index = index;

    /* Return new structure */
    return (element_req);
}

/* Specifies that an int element is required at the index specified.
 * 
 * See description of MD_new_element_req() for restrictions on index
 * and the properties of element requirements.
 */
void MD_require_int (MD_Field_Decl *field_decl, int element_index)
{
    MD_Element_Req *element_req;

    /* Check element_index for validity, allocates element_req structure,
     * resize require array (if necessary) and places element_req in
     * the require array. 
     * All fields except type and desc initialized to correct values.
     *
     * Pass this function's name for error messages.
     */
    element_req = MD_new_element_req (field_decl, element_index, 
				      "MD_require_int");

    /* Set element requirement to INT and set the description for error 
     * messages.
     */
    element_req->type = MD_INT;
    element_req->desc = MD_strdup ("INT");
}

/* Specifies that an double element is required at the index specified.
 * 
 * See description of MD_new_element_req() for restrictions on index
 * and the properties of element requirements.
 */
void MD_require_double (MD_Field_Decl *field_decl, int element_index)
{
    MD_Element_Req *element_req;

    /* Check element_index for validity, allocates element_req structure,
     * resize require array (if necessary) and places element_req in
     * the require array. 
     * All fields except type and desc initialized to correct values.
     *
     * Pass this function's name for error messages.
     */
    element_req = MD_new_element_req (field_decl, element_index, 
				      "MD_require_double");

    /* Set element requirement to DOUBLE and set the description for error 
     * messages.
     */
    element_req->type = MD_DOUBLE;
    element_req->desc = MD_strdup ("DOUBLE");
}

/* Specifies that an string element is required at the index specified.
 * 
 * See description of MD_new_element_req() for restrictions on index
 * and the properties of element requirements.
 */
void MD_require_string (MD_Field_Decl *field_decl, int element_index)
{
    MD_Element_Req *element_req;

    /* Check element_index for validity, allocates element_req structure,
     * resize require array (if necessary) and places element_req in
     * the require array. 
     * All fields except type and desc initialized to correct values.
     *
     * Pass this function's name for error messages.
     */
    element_req = MD_new_element_req (field_decl, element_index, 
				      "MD_require_string");

    /* Set element requirement to STRING and set the description for error 
     * messages.
     */
    element_req->type = MD_STRING;
    element_req->desc = MD_strdup ("STRING");
}

/* Specifies that an binary block element is required at the index specified.
 * 
 * See description of MD_new_element_req() for restrictions on index
 * and the properties of element requirements.
 */
void MD_require_block (MD_Field_Decl *field_decl, int element_index)
{
    MD_Element_Req *element_req;

    /* Check element_index for validity, allocates element_req structure,
     * resize require array (if necessary) and places element_req in
     * the require array. 
     * All fields except type and desc initialized to correct values.
     *
     * Pass this function's name for error messages.
     */
    element_req = MD_new_element_req (field_decl, element_index, 
				      "MD_require_block");

    /* Set element requirement to BLOCK and set the description for error 
     * messages.
     */
    element_req->type = MD_BLOCK;
    element_req->desc = MD_strdup ("BLOCK");
}

/* Specifies that a link (to a single target) element is required at the index
 * specified.  Use MD_require_multi_target_link() if multiple targets
 * are desired.
 * 
 * See description of MD_new_element_req() for restrictions on index
 * and the properties of element requirements.
 */
void MD_require_link (MD_Field_Decl *field_decl, int element_index, 
		      MD_Section *section)
{
    MD_Element_Req *element_req;
    MD_Section **link;
    int desc_length;
    char *desc;
    

    /* Check element_index for validity, allocates element_req structure,
     * resize require array (if necessary) and places element_req in
     * the require array. 
     * All fields except type and desc initialized to correct values.
     *
     * Pass this function's name for error messages.
     */
    element_req = MD_new_element_req (field_decl, element_index, 
				      "MD_require_link");


    /* Allocate link array for section array */
    link = (MD_Section **)malloc (sizeof(MD_Section *));
    if (link == NULL)
    {
	MD_punt (field_decl->section->md,
		 "Out of memory allocating link array (size 1)");
    }

    /* Initialize link array */
    link[0] = section;
    
    /* Calculate the length of the description field */
    desc_length = strlen(section->name) + strlen("LINK()") + 1;

    /* Malloc description buffer */
    desc = (char *) malloc (sizeof(char) * desc_length);
    if (desc == NULL)
    {
	MD_punt (field_decl->section->md,
		 "Out of memory allocating link desc (size %i)",
		 desc_length);
    }

    /* Sanity check, mark where we expect terminator */
    desc[desc_length-1] = 1;

    /* Create description */
    sprintf (desc,"LINK(%s)", section->name);

    /* Sanity check, make sure terminator is where we expect it */
    if (desc[desc_length-1] != 0)
    {
	MD_punt (field_decl->section->md,
		 "link->desc incorrect length (%i)", desc_length);
    }
	
    /* Set element requirement to LINK and set the description for error 
     * messages.
     */
    element_req->type = MD_LINK;
    element_req->desc = desc;
    element_req->link = link;
    element_req->link_array_size = 1;
}

/* Specifies that an multi-target link element is required at the index
 * specified.  
 * 
 * See description of MD_new_element_req() for restrictions on index
 * and the properties of element requirements.
 */
void MD_require_multi_target_link (MD_Field_Decl *field_decl, 
				   int element_index, int section_array_size,
				   MD_Section **section_array)
{
    MD_Element_Req *element_req;
    MD_Section **link, *section;
    int desc_length;
    char *desc, *desc_ptr, *name_ptr;
    int i;
    

    /* Check element_index for validity, allocates element_req structure,
     * resize require array (if necessary) and places element_req in
     * the require array. 
     * All fields except type and desc initialized to correct values.
     *
     * Pass this function's name for error messages.
     */
    element_req = MD_new_element_req (field_decl, element_index, 
				      "MD_require_multi_target_link");

    /* Check array size passed */
    if (section_array_size <= 0)
    {
	MD_punt (field_decl->section->md,
		 "MD_require_multi_target_link: invalid array size (%i).",
		 section_array_size);
    }

    /* Allocate link array for section array */
    link = (MD_Section **)malloc (sizeof(MD_Section *) * section_array_size);
    if (link == NULL)
    {
	MD_punt (field_decl->section->md,
		 "Out of memory allocating link array (size %i)",
		 section_array_size);
    }

    /* Initialize link array */
    desc_length = 0;
    for (i=0; i < section_array_size; i++)
    {
	section = section_array[i];
	link[i] = section;

	/* Sum the length of all the section names */
	desc_length += strlen (section->name);
    }

    /* Add in the length of the wrapping string */
    desc_length += strlen ("LINK()");

    /* Add in the space for '|' and for terminator */
    desc_length += section_array_size;

    /* Malloc description buffer */
    desc = (char *) malloc (sizeof(char) * desc_length);
    if (desc == NULL)
	MD_punt (field_decl->section->md,
		 "Out of memory allocating link desc (size %i)",
		 desc_length);

    /* Sanity check, mark where we expect terminator */
    desc[desc_length-1] = 1;

    /* Create description */
    strcpy (desc, "LINK(");
    desc_ptr = &desc[5];

    for (i=0; i < section_array_size; i++)
    {
	/* Copy name to description */
	for (name_ptr = link[i]->name; *name_ptr != 0; name_ptr++)
	{
	    *desc_ptr = *name_ptr;
	    desc_ptr++;
	}
	/* Add '|' between names */
	if ((i+1) < section_array_size)
	{
	    *desc_ptr = '|';
	    desc_ptr++;
	}
    }
    
    /* Add ending ')' and terminator */
    *desc_ptr = ')';
    desc_ptr++;
    *desc_ptr = 0;

    /* Sanity check, make sure terminator is where we expect it */
    if (desc[desc_length-1] != 0)
    {
	MD_punt (field_decl->section->md,
		 "link->desc incorrect length (%i)", desc_length);
    }
	
    /* Set element requirement to LINK and set the description for error 
     * messages.
     */
    element_req->type = MD_LINK;
    element_req->desc = desc;
    element_req->link = link;
    element_req->link_array_size = section_array_size;
}


/* Specifies that the requirement at the index speicifed should be treated
 * as a kleene starred requirement (0 or more required).
 *
 * Only the last requirement may be kleene starred.
 */
void MD_kleene_star_requirement (MD_Field_Decl *field_decl, int element_index)
{
    MD_Element_Req *element_req;
    char *new_desc;
    int max_require_index;

    max_require_index = field_decl->max_require_index;

    /* Make sure that the element index is in range */
    if ((element_index < 0) || (element_index > max_require_index))
    {
	MD_punt (field_decl->section->md,
		 "MD_kleene_star_requirement(%s->*->%s[%i]:\n  Invalid element index %i (the max valid index is %i).",
		 field_decl->section->name, field_decl->name,
		 element_index, element_index, max_require_index);
    }

    /* Must be specifying the last requirement */
    if (element_index != max_require_index)
    {
	MD_punt (field_decl->section->md,
		 "MD_kleene_star_requirement(%s->*->%s[%i]:\n  Only the last requirment (%i) may be kleene starred.",
		 field_decl->section->name, field_decl->name,
		 element_index, max_require_index);
    }
    
    /* Get the requirement that we want to kleene star */
    element_req = field_decl->require[element_index];

    /* Prevent from being called twice (to aid debugging) */
    if (element_req->kleene_starred)
    {
	MD_punt (field_decl->section->md,
		 "MD_kleene_star_requirement(%s->*->%s[%i]:\n  Requirement already kleene starred.",
		 field_decl->section->name, field_decl->name,
		 element_index);
	
    }
    
    /* Sanity check, something else better not be marked kleene_starred */
    if (field_decl->kleene_starred_req != NULL)
    {
	MD_punt (field_decl->section->md,
		 "MD_kleene_star_requirement(%s->*->%s[%i]:\n  Algorithm error, %i marked kleene starred... why?.",
		 field_decl->section->name, field_decl->name,
		 element_index, field_decl->kleene_starred_req->require_index);
    }

    /* Mark element as kleene starred and point field decl at it */
    element_req->kleene_starred = 1;
    field_decl->kleene_starred_req = element_req;

    /* Change requirement description to have extra star added */
    new_desc = MD_concat_strings (element_req->desc, "*");

    /* Free old desc */
    free (element_req->desc);

    /* Point to new description */
    element_req->desc = new_desc;
}

/* Creates a new field for the entry of type decl with an initial
 * size suitable for the specified number of elements.  The field
 * automatically resizes (upward) when appropriate, so 0 elements
 * may be specified.
 */
MD_Field *MD_new_field (MD_Entry *entry, MD_Field_Decl *decl, 
			       int num_elements)
{
    MD_Field *field, **field_slot;

    /* Sanity check, make sure everything is initialized properly */
    if (MD_Field_pool == NULL)
    {
	MD_punt (NULL, 
		 "MD routines not initialized (call MD_new_md() first)!");
    }
    field = (MD_Field *) L_alloc (MD_Field_pool);

    /* Initialize fields */
    field->entry = entry;
    field->decl = decl;
    field->max_element_index = -1;	/* No elements specified */

    /* Get place in entry's field array to place field */
    field_slot = &entry->field[decl->field_index];
    
    /* Make sure this field has not already been created for this entry */
    if (*field_slot != NULL)
    {
	MD_punt (entry->section->md,
		 "%s, entry %s: Cannot create field '%s', already exists!", 
		 entry->section->name, entry->name, decl->name);
    }

    /* Place in field in the proper slot in the entry's field array */
    *field_slot = field;

    /* Punt if invalid num_elements passed */
    if (num_elements < 0)
    {
	MD_punt (entry->section->md,
		 "%s, entry %s, creating field %i: num_elements (%i) must be >= 0",
		 entry->section->name, entry->name, decl->name, num_elements);
    }

    /* Initialize to no array allocated */
    field->element = NULL;
    field->element_array_size = 0;

    /* Create array of specified size, unless 0 */
    if (num_elements > 0)
    {
	MD_resize_element_array (field, num_elements - 1);
    }

    return (field);
}

/* Increases the size of a field's element array . */
static void MD_resize_element_array (MD_Field *field, int max_index)
{
    MD_Element **old_array, **new_array;
    int new_size;
    int i;
    
    /* Increase size so that it is increased to an even multiple of four.
     * Helps reduce overhead of adding multiple elements.
     * Max element is 0 based, so need to add one to size anyways!
     */
    new_size = (max_index | 3) + 1;

    /* Sanity check */
    if (new_size <= field->element_array_size)
    {
	MD_punt (field->entry->section->md,
		 "MD_resize_element_array: new_size (%i) must be > current size (%i)",
		 new_size, field->element_array_size);
    }

    /* Create new element array */
    new_array = (MD_Element **) malloc (sizeof(MD_Element *) * new_size);
    if (new_array == NULL)
    {
	MD_punt (field->entry->section->md, 
		 "MD_resize_element_array: Out of memory");
    }

    /* Get pointer to old array */
    old_array = field->element;

    /* Copy over pointers from existing array 
     * (Assumes max_element_index is -1 if old array is NULL).
     */
    for (i=field->max_element_index; i >= 0; i--)
	new_array[i] = old_array[i];

    /* Initialize remaining pointers to NULL */
    for (i=field->max_element_index + 1; i < new_size; i++)
	new_array[i] = NULL;

    /* Free old array, if not NULL */
    if (old_array != NULL)
	free (old_array);

    /* Install new array */
    field->element = new_array;
    field->element_array_size = new_size;
}

/* DO NOT CALL DIRECTLY! Use macro MD_find_field()!
 * This is the function version of MD_find_field()
 * 
 * Does a little more error check than the macro.
 */
MD_Field *_MD_find_field (MD_Entry *entry, MD_Field_Decl *field_decl)
{
    MD_Field *field;

    /* Make sure the entry and field_decl are for the same section */
    if (entry->section != field_decl->section)
    {
	MD_punt (entry->section->md,
		 "MD_find_field: entry (%s) from section %s and field decl (%s) from %s!",
		 entry->name, entry->section->name, field_decl->name,
		 field_decl->section->name);
    }

    /* Get the field from the entry (may be NULL) */
    field = entry->field[field_decl->field_index];

    return (field);
}

/* Delete a field from an entry */
void MD_delete_field (MD_Field *field)
{
    MD_Element *element, **element_array;
    int index, max_element_index;

    /* Get max_element_index and the element array for ease of use */
    max_element_index = field->max_element_index;
    element_array = field->element;

    /* Free all the element's allocated. 
     * (Assumes max_element_index == -1 if element_array is NULL
     */
    for (index = 0; index <= max_element_index; index++)
    {
	/* Get the element (may be NULL) */
	element = element_array[index];

	/* Free element if non-NULL */
	if (element != NULL)
	{
	    /* If element is a string, free string */
	    if (element->type == MD_STRING)
		free (element->value.s);

	    /* if element is a block, free block */
	    else if (element->type == MD_BLOCK)
	    {
		if (element->value.b.ptr != NULL)
		    free (element->value.b.ptr);
	    }

	    L_free (MD_Element_pool, element);
	}
    }
    
    /* Free element array (if exists)*/
    if (element_array != NULL)
	free (element_array);

    /* Remove field pointer from entry field array */
    field->entry->field[field->decl->field_index] = NULL;

    /* Free field structure */
    L_free (MD_Field_pool, field);
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_check_field()!
 * This function checks the elements in the specified field.
 * Used by the MD_check_field() macro and _MD_check_entry().
 * 
 * Returns the number of elements with incorrect type (0 if all ok).
 * 
 * If out is non-NULL, warning messages for each incorrect type will
 * be printed to out.
 *
 * The caller_name is used in the warning messages to identify the
 * routine/macro that started the checking process (MD_check_md, 
 * MD_check_section, MD_check_entry, or MD_check_field).
 */
int _MD_check_field (FILE *out, MD_Field *field, const char *caller_name)
{
    MD_Field_Decl *field_decl;
    MD_Element_Req **require_array, *requirement, *kleene_starred_req;
    MD_Element **element_array, *element;
    MD_Section *section, **link_array, *target_section;
    MD_Entry *entry;
    int max_element_index, max_require_index;
    int max_non_kleene_index, max_assigned_index, max_check_index;
    int link_array_size, valid_link;
    int i, j;
    int error_count;
    
    /* Initialize the error count */
    error_count = 0;

    /* Get the various pointers into local variables for ease of use */
    field_decl = field->decl;
    element_array = field->element;
    max_element_index = field->max_element_index;
    require_array = field_decl->require;
    max_require_index = field_decl->max_require_index;
    kleene_starred_req = field_decl->kleene_starred_req;
    entry = field->entry;
    section = entry->section;

    /* Make max_assigned_index point at the last element set to a value*/
    max_assigned_index = max_element_index;
    while ((max_assigned_index >= 0) && 
	   (element_array[max_assigned_index] == NULL))
	max_assigned_index--;

    /* Get the index of the last non-kleene starred requirement */
    if (kleene_starred_req != NULL)
	max_non_kleene_index = max_require_index -1;
    else
	max_non_kleene_index = max_require_index;

    /* Get the last index we need to check 
     * (check to maximum non-kleene starred requirement or to last
     * assigned index.)
     */
    if (max_assigned_index > max_non_kleene_index)
	max_check_index = max_assigned_index;
    else
	max_check_index = max_non_kleene_index;
     

    /* Scan all the elements that require checking.*/
    for (i=0; (i <= max_check_index); i++)
    {
	if (i <= max_assigned_index)
	    element = element_array[i];
	else
	    element = NULL;

	if (i <= max_non_kleene_index)
	    requirement = require_array[i];
	else
	    requirement = kleene_starred_req;

	/* Don't print out error message if both element and requirement
	 * are NULL.
	 */
	if ((element == NULL) && (requirement == NULL))
	    continue;
	
	/* Handle NULL elements */
	if (element == NULL)
	{
	    error_count++;
	    if (out != NULL)
	    {
		MD_warn (out,
			 "%s(%s->%s->%s[%i]):\n  Warning, %s required not NULL.",
			 caller_name, section->name, entry->name, 
			 field_decl->name, i, requirement->desc);
	    }
	}

	/* Handle NULL requirement */
	else if (requirement == NULL)
	{
	    error_count++;
	    if (out != NULL)
	    {
		MD_warn (out,
			 "%s(%s->%s->%s[%i]):\n  Warning, NULL required not %s.",
			 caller_name, section->name, entry->name, 
			 field_decl->name, i, MD_type_name[element->type]);
	    }
	}

	else if (element->type != requirement->type)
	{
	    error_count++;
	    if (out != NULL)
	    {
		MD_warn (out,
			 "%s(%s->%s->%s[%i]):\n  Warning, %s required not %s.",
			 caller_name, section->name, entry->name, 
			 field_decl->name, i, requirement->desc, 
			 MD_type_name[element->type]);
	    }
	}
	else if (element->type == MD_LINK)
	{
	    link_array = requirement->link;
	    link_array_size = requirement->link_array_size;
	    target_section = element->value.l->section;
	    valid_link = 0;
	    
	    for (j=0; j < link_array_size; j++)
	    {
		if (link_array[j] == target_section)
		{
		    valid_link = 1;
		    break;
		}
	    }

	    if (!valid_link)
	    {
		error_count++;
		if (out != NULL)
		{
		    MD_warn (out,
			   "%s(%s->%s->%s[%i]):\n  Warning, %s required not link to %s->%s.",
			     caller_name, section->name, entry->name, 
			     field_decl->name, i, requirement->desc, 
			     target_section->name, element->value.l->name);
		}
	    }
	}
    }

    /* Return count of error encountered */
    return (error_count);
}


/* Used by the MD_set_xxx routines to do type checking */
static int MD_check_setting (FILE *out, MD_Field *field, int index, 
			     const char *caller_name)
{
    MD_Field_Decl *field_decl;
    MD_Element_Req *requirement;
    MD_Element *element;
    MD_Section **link_array, *target_section;
    int link_array_size, valid_link;
    int j;

    /* Get the various pointers into local variables for ease of use */
    field_decl = field->decl;

    if (index <= field->max_element_index)
	element = field->element[index];
    else
	element = NULL;

    if (index <= field_decl->max_require_index)
	requirement = field_decl->require[index];
    else
	requirement = field_decl->kleene_starred_req;

    /* Don't print out error message if both element and requirement
     * are NULL.
     */
    if ((element == NULL) && (requirement == NULL))
    {
	/* Return 0 to signal no errors found */
	return (0);
    }
	
    /* Handle NULL elements */
    if (element == NULL)
    {
	if (out != NULL)
	{
	    MD_warn (out,
		     "%s(%s->%s->%s[%i]):\n  Warning, %s required not NULL.",
		     caller_name, field->entry->section->name, 
		     field->entry->name, field_decl->name, index, 
		     requirement->desc);
	}
	/* Return 1 to signal 1 error found */
	return (1);
    }

    /* Handle NULL requirement */
    else if (requirement == NULL)
    {
	if (out != NULL)
	{
	    MD_warn (out,
		     "%s(%s->%s->%s[%i]):\n  Warning, NULL required not %s.",
		     caller_name, field->entry->section->name, 
		     field->entry->name, field_decl->name, index, 
		     MD_type_name[element->type]);
	}
	/* Return 1 to signal 1 error found */
	return (1);
    }
    else if (element->type != requirement->type)
    {
	if (out != NULL)
	{
	    MD_warn (out,
		     "%s(%s->%s->%s[%i]):\n  Warning, %s required not %s.",
		     caller_name, field->entry->section->name, 
		     field->entry->name, field_decl->name, index, 
		     requirement->desc, MD_type_name[element->type]);
	}
	/* Return 1 to signal 1 error found */
	return (1);
    }
    else if (element->type == MD_LINK)
    {
	link_array = requirement->link;
	link_array_size = requirement->link_array_size;
	target_section = element->value.l->section;
	valid_link = 0;
	    
	for (j=0; j < link_array_size; j++)
	{
	    if (link_array[j] == target_section)
	    {
		valid_link = 1;
		break;
	    }
	}
	
	if (!valid_link)
	{
	    if (out != NULL)
	    {
		MD_warn (out,
		        "%s(%s->%s->%s[%i]):\n  Warning, %s required not link to %s->%s.",
			 caller_name, field->entry->section->name, 
			 field->entry->name, field_decl->name, index, 
			 requirement->desc, target_section->name, 
			 element->value.l->name);
	    }
	    /* Return 1 to signal 1 error found */
	    return (1);
	}
    }

    /* Return 0 to signal 0 errors found */
    return (0);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_num_elements()!
 * This is the function version of the macro MD_num_elements().
 * 
 * Used to allow arguments to be checked for macros.
 */
int _MD_num_elements (MD_Field *field)
{
    /* Return number of elements in a field (DOES count NULL elements before
     * the last defined element).
     */
    return (field->max_element_index + 1);
}

/*
 * DO NOT CALL DIRECTLY! Use macro MD_max_element_index()!
 * This is the function version of the macro MD_max_element_index().
 * 
 * Used to allow arguments to be checked for macros.
 */
int _MD_max_element_index (MD_Field *field)
{
    /* Return the index of the last defined element in the field. */
    return (field->max_element_index);
}


/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_int()!
 * This is the function version of the macro MD_set_int() without
 * type checking.
 *
 * Sets the field element at index to the int value. 
 */
void _MD_set_int (MD_Field *field, int index, int value)
{
    MD_Element *element;

    /* Detect errors, make sure index is not negative! */
    if (index < 0)
    {
	MD_punt (field->entry->section->md,
		 "MD_set_int (%s->%s->%s[%d]): Invalid index (%d)\n",
		 field->entry->section->name, field->entry->name,
		 field->decl->name, index, index);
    }

    /* Detect need to increase max_element_index. */
    if (index > field->max_element_index)
    {
	/* Detect need to increase element array size */
	if (index >= field->element_array_size)
	{
	    /* Increase element array size so index can be handled */
	    MD_resize_element_array (field, index);
	}

	/* Set max_element_index to new value*/
	field->max_element_index = index;
    }

    /* Get element */
    element = field->element[index];

    /* Create element if doesn't exist */
    if (element == NULL)
    {
	element = (MD_Element *) L_alloc (MD_Element_pool);
	field->element[index] = element;
    }

    /* Otherwise, free string memory if the existing element is a string */
    else if (element->type == MD_STRING)
    {
	free (element->value.s);
    }
    /* Otherwise, free block memory if the existing element is a block */
    else if (element->type == MD_BLOCK)
    {
        if (element->value.b.ptr != NULL)
	    free (element->value.b.ptr);
    }

    /* Set content of element */
    element->field = field;
    element->element_index = (unsigned short) index;
    element->type = MD_INT;
    element->value.i = value;
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_int()!
 * This is the function version of the macro MD_set_int() with
 * type checking.
 *
 * Sets the field element at index to the int value. 
 */
void _MD_set_int_type_checking (MD_Field *field, int index, int value)
{
    /* Call the normal set_int routine first */
    _MD_set_int (field, index, value);

    /* Call the check routine for this element */
    MD_check_setting (stderr, field, index, "MD_set_int");
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_get_int()!
 * This is the function version of the macro MD_get_int().
 *
 * Gets the int value of the element at the specified index.
 * The user must prevent this routine from being called for a NULL element.
 * 
 * Unlike the macro, this verifies element type is correct.
 *
 */
int _MD_get_int (MD_Field *field, int index)
{
    MD_Element *element;

    /* Get the element */
    element = field->element[index];

    /* Make sure this is an int element */
    if (element->type != MD_INT)
    {
	MD_punt (field->entry->section->md, 
		 "MD_get_int(%s->%s->%s[%i]):\n  Accessing %s element as an INT!",
		 field->entry->section->name,
		 field->entry->name, field->decl->name,
		 index, MD_type_name[(int) element->type]);
    }

    /* Return element's value */
    return (element->value.i);
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_double()!
 * This is the function version of the macro MD_set_double() without
 * type checking.
 *
 * Sets the field element at index to the double value 
 */
void _MD_set_double (MD_Field *field, int index, double value)
{
    MD_Element *element;

    /* Detect errors, make sure index is not negative! */
    if (index < 0)
    {
	MD_punt (field->entry->section->md,
		 "MD_set_double (%s->%s->%s[%d]): Invalid index (%d)\n",
		 field->entry->section->name, field->entry->name,
		 field->decl->name, index, index);
    }

    /* Detect need to increase max_element_index. */
    if (index > field->max_element_index)
    {
	/* Detect need to increase element array size */
	if (index >= field->element_array_size)
	{
	    /* Increase element array size so index can be handled */
	    MD_resize_element_array (field, index);
	}

	/* Set max_element_index to new value*/
	field->max_element_index = index;
    }

    /* Get element */
    element = field->element[index];

    /* Create element if doesn't exist */
    if (element == NULL)
    {
	element = (MD_Element *) L_alloc (MD_Element_pool);
	field->element[index] = element;
    }
    /* Otherwise, free string memory if the existing element is a string */
    else if (element->type == MD_STRING)
    {
	free (element->value.s);
    }
    /* Otherwise, free block memory if the existing element is a block */
    else if (element->type == MD_BLOCK)
    {
        if (element->value.b.ptr != NULL)
	    free (element->value.b.ptr);
    }

    /* Set content of element */
    element->field = field;
    element->element_index = (unsigned short) index;
    element->type = MD_DOUBLE;
    element->value.d = value;
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_double()!
 * This is the function version of the macro MD_set_double() with
 * type checking.
 *
 * Sets the field element at index to the double value. 
 */
void _MD_set_double_type_checking (MD_Field *field, int index, double value)
{
    /* Call the normal set_double routine first */
    _MD_set_double (field, index, value);

    /* Call the check routine for this element */
    MD_check_setting (stderr, field, index, "MD_set_double");
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_get_double()!
 * This is the function version of the macro MD_get_double().
 *
 * Gets the double value of the element at the specified index.
 * The user must prevent this routine from being called for a NULL element.
 * 
 * Unlike the macro, this verifies element type is correct.
 *
 */
double _MD_get_double (MD_Field *field, int index)
{
    MD_Element *element;

    /* Get the element */
    element = field->element[index];

    /* Make sure this is an double element */
    if (element->type != MD_DOUBLE)
    {
	MD_punt (field->entry->section->md, 
		 "MD_get_double(%s->%s->%s[%i]):\n  Accessing %s element as a DOUBLE!",
		 field->entry->section->name,
		 field->entry->name, field->decl->name,
		 index, MD_type_name[(int) element->type]);
    }

    /* Return element's value */
    return (element->value.d);
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_string()!
 * This is the function version of the macro MD_set_string() without
 * type checking.
 * 
 * Sets the field element at index to the string value 
 */
void _MD_set_string (MD_Field *field, int index, const char *value)
{
    MD_Element *element;

    /* Detect errors, make sure index is not negative! */
    if (index < 0)
    {
	MD_punt (field->entry->section->md,
		 "MD_set_string (%s->%s->%s[%d]): Invalid index (%d)\n",
		 field->entry->section->name, field->entry->name,
		 field->decl->name, index, index);
    }

    /* Detect need to increase max_element_index. */
    if (index > field->max_element_index)
    {
	/* Detect need to increase element array size */
	if (index >= field->element_array_size)
	{
	    /* Increase element array size so index can be handled */
	    MD_resize_element_array (field, index);
	}

	/* Set max_element_index to new value*/
	field->max_element_index = index;
    }

    /* Get element */
    element = field->element[index];

    /* Create element if doesn't exist */
    if (element == NULL)
    {
	element = (MD_Element *) L_alloc (MD_Element_pool);
	field->element[index] = element;
    }
    /* Otherwise, free string memory if the existing element is a string */
    else if (element->type == MD_STRING)
    {
	free (element->value.s);
    }
    /* Otherwise, free block memory if the existing element is a block */
    else if (element->type == MD_BLOCK)
    {
        if (element->value.b.ptr != NULL)
	    free (element->value.b.ptr);
    }

    /* Set content of element */
    element->field = field;
    element->element_index = (unsigned short) index;
    element->type = MD_STRING;
    element->value.s = MD_strdup(value);
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_string()!
 * This is the function version of the macro MD_set_string() with
 * type checking.
 *
 * Sets the field element at index to the string value. 
 */
void _MD_set_string_type_checking (MD_Field *field, int index, 
				   const char *value)
{
    /* Call the normal set_string routine first */
    _MD_set_string (field, index, value);

    /* Call the check routine for this element */
    MD_check_setting (stderr, field, index, "MD_set_string");
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_get_string()!
 * This is the function version of the macro MD_get_string().
 *
 * Gets the string value of the element at the specified index.
 * The user must prevent this routine from being called for a NULL element.
 * 
 * Unlike the macro, this verifies element type is correct.
 *
 */
char *_MD_get_string (MD_Field *field, int index)
{
    MD_Element *element;

    /* Get the element */
    element = field->element[index];

    /* Make sure this is an string element */
    if (element->type != MD_STRING)
    {
	MD_punt (field->entry->section->md, 
		 "MD_get_string(%s->%s->%s[%i]):\n  Accessing %s element as a STRING!",
		 field->entry->section->name,
		 field->entry->name, field->decl->name,
		 index, MD_type_name[(int) element->type]);
    }

    /* Return element's value */
    return (element->value.s);
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_block()!
 * This is the function version of the macro MD_set_block() without
 * type checking.
 * 
 * Sets the field element at index to the block value 
 */
void _MD_set_block (MD_Field *field, int index, unsigned int size, void *ptr)
{
    MD_Element *element;
    void *copied_block;

    /* Detect errors, make sure index is not negative! */
    if (index < 0)
    {
	MD_punt (field->entry->section->md,
		 "MD_set_block (%s->%s->%s[%d]): Invalid index (%d)\n",
		 field->entry->section->name, field->entry->name,
		 field->decl->name, index, index);
    }

    /* Detect need to increase max_element_index. */
    if (index > field->max_element_index)
    {
	/* Detect need to increase element array size */
	if (index >= field->element_array_size)
	{
	    /* Increase element array size so index can be handled */
	    MD_resize_element_array (field, index);
	}

	/* Set max_element_index to new value*/
	field->max_element_index = index;
    }

    /* Get element */
    element = field->element[index];

    /* Create element if doesn't exist */
    if (element == NULL)
    {
	element = (MD_Element *) L_alloc (MD_Element_pool);
	field->element[index] = element;
    }
    /* Otherwise, free string memory if the existing element is a string */
    else if (element->type == MD_STRING)
    {
	free (element->value.s);
    }
    /* Otherwise, free block memory if the existing element is a block */
    else if (element->type == MD_BLOCK)
    {
        if (element->value.b.ptr != NULL)
	    free (element->value.b.ptr);
    }

    /* Set content of element */
    element->field = field;
    element->element_index = (unsigned short) index;
    element->type = MD_BLOCK;
    element->value.b.size = size;
    /* If size is zero, place NULL in ptr */
    if (size == 0)
    {
	element->value.b.ptr = NULL;
    }
    /* Otherwise, malloc block and memcpy data into it */
    else
    {
	if ((copied_block = (void *) malloc (size)) == NULL)
	{
	    MD_punt (field->entry->section->md,
		     "_MD_set_block: Out of memory, block size %i.",
		     size);
	}
	memcpy (copied_block, ptr, size);
	element->value.b.ptr = copied_block;
    }
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_block()!
 * This is the function version of the macro MD_set_block() with
 * type checking.
 *
 * Sets the field element at index to the block value. 
 */
void _MD_set_block_type_checking (MD_Field *field, int index, 
				  unsigned int size, void *ptr)
{
    /* Call the normal set_block routine first */
    _MD_set_block (field, index, size, ptr);

    /* Call the check routine for this element */
    MD_check_setting (stderr, field, index, "MD_set_block");
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_get_block_size()!
 * This is the function version of the macro MD_get_block_size().
 *
 * Gets the block size of the element at the specified index.
 * The user must prevent this routine from being called for a NULL element.
 * 
 * Unlike the macro, this verifies element type is correct.
 *
 */
int _MD_get_block_size (MD_Field *field, int index)
{
    MD_Element *element;

    /* Get the element */
    element = field->element[index];

    /* Make sure this is an block element */
    if (element->type != MD_BLOCK)
    {
	MD_punt (field->entry->section->md, 
		 "MD_get_block_size(%s->%s->%s[%i]):\n  Accessing %s element as a BLOCK!",
		 field->entry->section->name,
		 field->entry->name, field->decl->name,
		 index, MD_type_name[(int) element->type]);
    }

    /* Return element's block size */
    return (element->value.b.size);
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_get_block_ptr()!
 * This is the function version of the macro MD_get_block_ptr().
 *
 * Gets the block pointer of the element at the specified index.
 * The user must prevent this routine from being called for a NULL element.
 * 
 * Unlike the macro, this verifies element type is correct.
 *
 */
void *_MD_get_block_ptr (MD_Field *field, int index)
{
    MD_Element *element;

    /* Get the element */
    element = field->element[index];

    /* Make sure this is an block element */
    if (element->type != MD_BLOCK)
    {
	MD_punt (field->entry->section->md, 
		 "MD_get_block_ptr(%s->%s->%s[%i]):\n  Accessing %s element as a BLOCK!",
		 field->entry->section->name,
		 field->entry->name, field->decl->name,
		 index, MD_type_name[(int) element->type]);
    }

    /* Return element's block pointer */
    return (element->value.b.ptr);
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_link()!
 * This is the function version of the macro MD_set_link() without
 * type checking.
 * 
 * Sets the field element at index to be a link to the specified entry.
 */
void _MD_set_link (MD_Field *field, int index, MD_Entry *value)
{
    MD_Element *element;

    /* Detect errors, make sure index is not negative! */
    if (index < 0)
    {
	MD_punt (field->entry->section->md,
		 "MD_set_link (%s->%s->%s[%d]):\n  Invalid index (%d)\n",
		 field->entry->section->name, field->entry->name,
		 field->decl->name, index, index);
    }

    /* Value may not be a NULL pointer */
    if (value == NULL)
    {
	MD_punt (field->entry->section->md,
		 "MD_set_link (%s->%s->%s[%d]):\n  Invalid value (pointer to NULL entry)\n",
		 field->entry->section->name, field->entry->name,
		 field->decl->name, index);
    }

    /* Detect need to increase max_element_index. */
    if (index > field->max_element_index)
    {
	/* Detect need to increase element array size */
	if (index >= field->element_array_size)
	{
	    /* Increase element array size so index can be handled */
	    MD_resize_element_array (field, index);
	}

	/* Set max_element_index to new value*/
	field->max_element_index = index;
    }

    /* Get element */
    element = field->element[index];

    /* Create element if doesn't exist */
    if (element == NULL)
    {
	element = (MD_Element *) L_alloc (MD_Element_pool);
	field->element[index] = element;
    }
    /* Otherwise, free string memory if the existing element is a string */
    else if (element->type == MD_STRING)
    {
	free (element->value.s);
    }
    /* Otherwise, free block memory if the existing element is a block */
    else if (element->type == MD_BLOCK)
    {
        if (element->value.b.ptr != NULL)
	    free (element->value.b.ptr);
    }

    /* Set content of element */
    element->field = field;
    element->element_index = (unsigned short) index;
    element->type = MD_LINK;
    element->value.l = value;
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_set_link()!
 * This is the function version of the macro MD_set_link() with
 * type checking.
 *
 * Sets the field element at index to be a link to the specified entry.
 */
void _MD_set_link_type_checking (MD_Field *field, int index, MD_Entry *value)
{
    /* Call the normal set_link routine first */
    _MD_set_link (field, index, value);

    /* Call the check routine for this element */
    MD_check_setting (stderr, field, index, "MD_set_link");
}

/*
 * DO NOT CALL DIRECTLY!  Use macro MD_get_link()!
 * This is the function version of the macro MD_get_link().
 *
 * Gets the entry pointed to by the link element at the specified index.
 * The user must prevent this routine from being called for a NULL element.
 * 
 * Unlike the macro, this verifies element type is correct.
 *
 */
MD_Entry *_MD_get_link (MD_Field *field, int index)
{
    MD_Element *element;

    /* Get the element */
    element = field->element[index];

    /* Make sure this is an link element */
    if (element->type != MD_LINK)
    {
	MD_punt (field->entry->section->md, 
		 "MD_get_link(%s->%s->%s[%i]):\n  Accessing %s element as a LINK!",
		 field->entry->section->name,
		 field->entry->name, field->decl->name,
		 index, MD_type_name[(int) element->type]);
    }

    /* Return element's value */
    return (element->value.l);
}

/*
 * Deletes the field element at the index specified.
 */
void MD_delete_element (MD_Field *field, int index)
{
    MD_Element *element, **element_array;
    int max_index;

    /* Get the element array for ease of use */
    element_array = field->element;

    /* Get the element we are deleting */
    element = element_array[index];
    
    /* Do nothing if already NULL */
    if (element == NULL)
	return;

    /* Set the element at the index to NULL in the field's element array */
    element_array[index] = NULL;

    /* Free string if element is a string */
    if (element->type == MD_STRING)
    {
	free (element->value.s);
    }

    /* Free block memory if element is a block */
    else if (element->type == MD_BLOCK)
    {
        if (element->value.b.ptr != NULL)
	    free (element->value.b.ptr);
    }

    /* Free element */
    L_free (MD_Element_pool, element);

    /* Adjust the max_element_index for the field, may need to
     * scan over a range of NULL elements.  If field is now
     * empty, max_index should be set to -1
     */
    max_index = field->max_element_index;
    while ((max_index > -1) && (element_array[max_index] == NULL))
	max_index--;
    field->max_element_index = max_index;
}


#if 0
/* Test out md routines */
int main()
{
    MD *md;
    MD_Section *section, *section2, *section_array[10];
    MD_Entry *entry;
    MD_Symbol_Table *table;
    MD_Symbol *symbol;
    MD_Field *field1, *field2;
    MD_Field_Decl *field_decl1, *field_decl2, *field_decl3, *field_decl4;
    MD_Field_Decl *decl[10];
    MD_Element *element;
    int i, found, j, error_count;
    char buf[100], *string;
    

    printf ("Testing md routines.\n");

    /* Create a new md */
    md = MD_new_md ("Test_md", 1);

    /* Create a new section */
    section = MD_new_section (md, "Test_section", 1, 0);
    printf ("%s max_field_index %i field_array_size %i\n",
	    section->name, section->max_field_index,
	    section->field_array_size);

    section2 = MD_new_section (md, "Section2", 0, 0);

    section_array[0] = section;
    section_array[1] = section2;
    
    /* Declare two fields */
    field_decl1 = MD_new_field_decl (section, "field1", MD_REQUIRED_FIELD);
    printf ("%s max_field_index %i field_array_size %i  %s index %i\n",
	    section->name, section->max_field_index, section->field_array_size,
	    field_decl1->name, field_decl1->field_index);
    field_decl2 = MD_new_field_decl (section, "field2", MD_REQUIRED_FIELD);
    printf ("%s max_field_index %i field_array_size %i  %s index %i\n",
	    section->name, section->max_field_index, section->field_array_size,
	    field_decl2->name, field_decl2->field_index);

    /* Require field1 to have exactly 1 int */
    MD_require_double (field_decl1, 0);
    MD_require_int (field_decl1, 1);
    MD_require_string (field_decl1, 2);
    MD_require_link (field_decl1, 3, section2);
    MD_require_multi_target_link (field_decl1, 4, 2, section_array);
    MD_require_double (field_decl1, 5);
    MD_require_int (field_decl1, 6);
    MD_require_string (field_decl1, 7);
    MD_require_link (field_decl1, 8, section2);
    MD_require_multi_target_link (field_decl1, 9, 2, section_array);

    MD_require_double (field_decl1, 10);
    MD_require_int (field_decl1, 11);
    MD_require_string (field_decl1, 12);
    MD_require_link (field_decl1, 13, section2);
    MD_require_multi_target_link (field_decl1, 14, 2, section_array);
    
    /* Test symbol table routines */
    table = section->entry_table;
    MD_print_symbol_table_hash (stdout, table);

    for (i=0; i < 10; i+= 3)
    {
	if (i == 3)
	    sprintf (buf, "long_name_%i", i);
	else
	    sprintf (buf, "d%i", i);
	entry = MD_new_entry (section, buf);

	/* Create some fields in each entry */
	field1 = MD_new_field (entry, field_decl1, 0);
	MD_set_double (field1, 0, ((double)i) * ((double) 1.001));
	MD_set_int (field1, 1, i); 
	sprintf (buf, "string%i", i);
	MD_set_string (field1, 2, buf);
	MD_set_link (field1, 4, entry);

	MD_set_double (field1, 5, (double)i);
	MD_set_int (field1, 6, i); 
	sprintf (buf, "string%i", i);
	MD_set_string (field1, 7, buf);
	MD_set_link (field1, 9, entry);

	MD_set_double (field1, 10, (double)i);
	MD_set_int (field1, 11, i); 
	sprintf (buf, "string%i", i);
	MD_set_string (field1, 12, buf);
	MD_set_link (field1, 14, entry);

	MD_get_int (field1, 1);
	field2 = MD_new_field (entry, field_decl2, 0);
	if (table->symbol_count == table->resize_size)
	{
	    MD_print_symbol_table_hash (stdout, table);
	}
    }

    fprintf (stderr, "Sizeof MD_Element = %i\n", sizeof(MD_Element));

    field_decl3 = MD_new_field_decl (section, "field3", MD_REQUIRED_FIELD);

/*
    fprintf (stderr, "\nBEGIN MD CHECK\n");
    error_count = MD_check_md (stderr, md);
    error_count = MD_check_section (stderr, section);
    
    entry = MD_find_entry(section, "d6");
    error_count = MD_check_entry (stderr, entry);

   field1 = MD_find_field (entry, field_decl1);
   error_count = MD_check_field (stderr, field1);
    fprintf (stderr, "END MD CHECK (%i errors found)\n\n", error_count);
*/

    MD_print_md (stdout, md, 80);
/*
    MD_print_section (stdout, section, 80);
    entry = MD_find_entry(section, "d6");
    MD_print_entry (stdout, entry);
*/
    MD_print_symbol_table_hash (stdout, table);
    printf ("\n");
    printf ("%s max_field_index %i field_array_size %i  %s index %i\n",
	    section->name, section->max_field_index, section->field_array_size,
	    field_decl3->name, field_decl3->field_index);
    MD_delete_field_decl (field_decl2);
    field_decl4 = MD_new_field_decl (section, "field4", MD_OPTIONAL_FIELD);
    printf ("%s max_field_index %i field_array_size %i  %s index %i\n",
	    section->name, section->max_field_index, section->field_array_size,
	    field_decl4->name, field_decl4->field_index);

    for (j=5; j < 10; j++)
    {
	sprintf (buf, "field%i", j);
	decl[j] = MD_new_field_decl (section, buf, MD_OPTIONAL_FIELD);
	printf ("%s max_field_index %i field_array_size %i  %s index %i\n",
		section->name, section->max_field_index, 
		section->field_array_size, decl[j]->name, 
		decl[j]->field_index);
	
    }
    found = 0;
    for (i=0; i < 1000; i+= 1)
    {
	sprintf (buf, "d%i", i);
	if ((entry = MD_find_entry (section, buf)) != NULL)
	{
	    found ++;
	    MD_delete_entry (entry); 
	}
	if (i == 500)
	    MD_print_symbol_table_hash (stdout, table);
    }
    printf ("\n Found = %i\n", found);

/*
    MD_resize_symbol_table(table);
*/
    MD_print_symbol_table_hash (stdout, table);

    section = MD_find_section(md, "Test_section");
    MD_delete_section(section);

    /* Delete the md */
    MD_delete_md (md);
    md = NULL;
#if 0
    field = MD_new_field (NULL, NULL, 0);
    printf ("size %i max %i\n", field->array_size, field->max_element_index);

    for (i=0; i < 20; i ++)
    {
	MD_set_int (field, i, i);
	printf ("size %i max %i\n", field->array_size, 
		field->max_element_index);
    }
    for (i=0; i < 20; i ++)
    {
	element = field->element[i];
	if (element != NULL)
	{
	    printf ("%2i: %i type %i value\n", i, element->type, element->value.i);
	}
    }    

#endif
    printf ("End md test.\n");
    return (0);
}

#endif
