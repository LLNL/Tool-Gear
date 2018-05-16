/* IMPACT Public Release (www.crhc.uiuc.edu/IMPACT)            Version 2.20  */
/* IMPACT Trimaran Release (www.trimaran.org)                  May 17, 1999  */
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
 *      File:   index_symbol.c
 *      Author: John C. Gyllenhaal, Wen-mei Hwu
 *      Creation Date:  1995 (split into separate file Sept 1998 -JCG)
 *      Changed 'char *' function parameters to 'const char *' -JCG 08/02
 *      Copied int_symbol.h to index_symbol.h and made 'data' type 'int'
 *      instead of 'void *' for int index uses.  -JCG 04/06
\*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index_symbol.h"
#include "l_alloc_new.h"
#include "l_punt.h"

L_Alloc_Pool *INDEX_Symbol_Table_pool = NULL;
L_Alloc_Pool *INDEX_Symbol_pool = NULL;


/***
 *** 
 *** INDEX routines
 ***
 ***/

/* Create and initialize INDEX_Symbol_Table.
 * Creates a hash table of initial size 2 * expected_size rounded up
 * to the closest power of two.  (Min hash size 32)
 */
INDEX_Symbol_Table *INDEX_new_symbol_table (const char *name, int expected_size)
{
    INDEX_Symbol_Table *table;
    INDEX_Symbol **hash;
    unsigned int min_size, hash_size;
    unsigned int i;

    /* If expected size negative, force to be 0 */
    if (expected_size < 0)
	expected_size = 0;

    /* To prevent infinite loop by sizing algorithm (and running out of
     * memory :) ), expected_size must be <= a billion.
     */
    if (expected_size > 1000000000)
	L_punt ("INDEX_Symbol_Table: unreasonable expected_size (%u)",
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

    
    /* Create new symbol table pool (and symbol pool if necessary) */
    if (INDEX_Symbol_Table_pool == NULL)
    {
	INDEX_Symbol_Table_pool = L_create_alloc_pool ("INDEX_Symbol_Table",
						     sizeof (INDEX_Symbol_Table),
						     16);
	INDEX_Symbol_pool = L_create_alloc_pool ("INDEX_Symbol",
					       sizeof (INDEX_Symbol), 64);
    }

    /* Allocate symbol table */
    table = (INDEX_Symbol_Table *) L_alloc (INDEX_Symbol_Table_pool);

    /* Allocate array for hash */
    hash = (INDEX_Symbol **) malloc (hash_size * sizeof(INDEX_Symbol *));
    if (hash == NULL)
    {
	L_punt ( "INDEX_new_symbol_table: Out of memory, hash array size %i.",
		 hash_size);
    }

    /* Initialize hash table */
    for (i=0; i < hash_size; i++)
        hash[i] = NULL;

    /* Initialize fields */
    table->name = strdup (name);
    table->hash = hash;
    table->hash_size = hash_size;
    table->hash_mask = hash_size -1; /* AND mask, works only for power of 2 */
    /* Resize when count at 75% of hash_size */
    table->resize_size = hash_size - (hash_size >> 2);
    table->head_symbol = NULL;
    table->tail_symbol = NULL;
    table->symbol_count = 0;
    table->resize_target = 0; /* For later updates to estimated size */

    return (table);
}

/* Frees the symbol table */
void INDEX_delete_symbol_table (INDEX_Symbol_Table *table)
{
    INDEX_Symbol *symbol, *next_symbol;

    /* For all the symbols in the table, free each one */
    for (symbol = table->head_symbol; symbol != NULL;
	 symbol = next_symbol)
    {
	/* Get the next symbol before deleting this one */
	next_symbol = symbol->next_symbol;

	/* Free symbol structure */
	L_free (INDEX_Symbol_pool, symbol);
    }

    /* Free the hash array and table name*/
    free (table->hash);
    free (table->name);

    /* Free the table structure */
    L_free (INDEX_Symbol_Table_pool, table);
}

/* Doubles the symbol table hash array size */
void INDEX_resize_symbol_table (INDEX_Symbol_Table *table)
{
    INDEX_Symbol **new_hash, *symbol, *hash_head;
    int new_hash_size;
    unsigned int new_hash_mask, new_hash_index;
    int i;

    /* Double the size of the hash array */
    new_hash_size = table->hash_size * 2;

    /* If user has specified larger resize target, use that instead (also
     * power of two).   Set by INDEX_tweak_table_resize ().
     */
    if (new_hash_size < table->resize_target)
    {
#if 0
	/* DEBUG */
	fprintf (stderr, "INDEX_resize_symbol_table: Using resize target %i instead of %i!\n",
		 table->resize_target, new_hash_size);
#endif

	new_hash_size = table->resize_target;
    }

#if 0
    /* DEBUG */
    TG_timestamp("INDEX_resize_symbol_table (%s): new size %i\n",
		 table->name, new_hash_size);
#endif

    /* Allocate new hash array */
    new_hash = (INDEX_Symbol **) malloc (new_hash_size * sizeof (INDEX_Symbol *));
    if (new_hash == NULL)
    {
	L_punt ("INDEX_resize_symbol_table: Out of memory, new size %i.",
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
	new_hash_index = symbol->value & new_hash_mask;
	
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


/* Adds structure to symbol table.
 * Dynamically increases symbol table's hash array.
 * Returns pointer to added symbol.
 */
INDEX_Symbol *INDEX_add_symbol (INDEX_Symbol_Table *table, int value, int data)
{
    INDEX_Symbol *symbol, *hash_head, *check_symbol, *tail_symbol;
    unsigned int hash_index;
    int symbol_count;

    /* Increase symbol table size if necessary before adding new symbol.  
     * This will change the hash_mask if the table is resized!
     */
    symbol_count = table->symbol_count;
    if (symbol_count >= table->resize_size)
    {
	INDEX_resize_symbol_table (table);
    }

    /* Allocate a symbol (pool initialized in create table routine)*/
    symbol = (INDEX_Symbol *) L_alloc (INDEX_Symbol_pool);
    
    /* Initialize fields */
    symbol->value = value;
    symbol->data = data;
    symbol->table = table;

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

    /* Get index into hash table to use for this value */
    hash_index = value & table->hash_mask;
    
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
	/* If value the same, punt */
	if (check_symbol->value == value)
	{
	    L_punt ("INDEX_add_symbol, %s: cannot add '%i', already in table!",
		     table->name, value);
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

/* Returns a INDEX_Symbol structure with the desired value, or NULL
 * if the value is not in the symbol table.
 */
INDEX_Symbol *INDEX_find_symbol (INDEX_Symbol_Table *table, int value)
{
    INDEX_Symbol *symbol;
    unsigned int hash_index;

    /* Get the index into the hash table */
    hash_index = value & table->hash_mask;

    /* Search the linked list for matching name */
    for (symbol = table->hash[hash_index]; symbol != NULL; 
	 symbol = symbol->next_hash)
    {
	/* Compare values to find match */
	if (symbol->value == value)
	{
	    return (symbol);
	}
    }
    return (NULL);
}

/* Deletes symbol and optionally deletes the data using the free routine */
void INDEX_delete_symbol (INDEX_Symbol *symbol)
{
    INDEX_Symbol_Table *table;
    INDEX_Symbol *next_hash, *prev_hash, *next_symbol, *prev_symbol;
    unsigned int hash_index;

    /* Get the table the symbol is from */
    table = symbol->table;

    /* Get the hash index from the symbol's value */
    hash_index = symbol->value & table->hash_mask;

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


    /* Free symbol structure */
    L_free (INDEX_Symbol_pool, symbol);

    /* Decrement table symbol count */
    table->symbol_count --;
}

/* Sets target size for next resize.  Use if after creation of the table, 
 * the expected size changes radically (i.e., expected size goes from 32 to 
 * 2,000,000) to avoid intermediate symbol table resize costs. 
 */
void INDEX_tweak_table_resize (INDEX_Symbol_Table *table, 
			     int updated_expected_size)
{
    int min_size, target_size;

    /* Do nothing if the updated expected size is smaller than
     * the current table (since it will not change resize size)
     */
    if (updated_expected_size < table->hash_size)
    {
	/* Clear any previous resize targets before returning */
	table->resize_target = 0;
	return;
    }

    /* To prevent infinite loop by sizing algorithm (and running out of
     * memory :) ), updated_expected_size must be <= a billion.
     */
    if (updated_expected_size > 1000000000)
	L_punt ("INDEX_tweak_table_resize: "
		"unreasonable updated_expected_size (%u)",
		updated_expected_size);

    /* Want a minumum size of at least twice the expected size */
    min_size = updated_expected_size * 2;

    /* Start with 2 * current hash_size and grow as necessary */
    target_size = table->hash_size << 1;

    /* Double target_size until min_size is reached or exceeded */
    while (target_size < min_size)
	target_size = target_size << 1;


    /* Set this as the new user-specified resize target that will
     * take effect at the next table resize.
     */
    table->resize_target = target_size;    
}


/* Prints out the symbol table's hash table (debug routine) */
void INDEX_print_symbol_table_hash (FILE *out, INDEX_Symbol_Table *table)
{
    INDEX_Symbol *symbol;
    int hash_index, lines;

    /* Count lines used in table */
    lines = 0;
    for (hash_index = 0; hash_index < table->hash_size; hash_index++)
    {
	if (table->hash[hash_index] != NULL)
	    lines++;
    }
    fprintf (out, "%s has %i entries (hash size %i, used %i):\n", 
	     table->name, table->symbol_count, table->hash_size, lines);

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
	    fprintf (out, " %i", symbol->value);
	}
	fprintf (out, "\n");
    }
}

