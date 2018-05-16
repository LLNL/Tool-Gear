/* IMPACT Public Release (www.crhc.uiuc.edu/IMPACT)            Version 2.00  */
/* IMPACT Trimaran Release (www.trimaran.org)                  Feb. 22, 1999 */
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
/*===========================================================================
 *
 *      File :          l_alloc_new.c
 *      Description :   New and improved data structure allocation/mgmt
 *      Creation Date : May 1993
 *      Authors :       John C. Gyllenhaal and Wen-mei Hwu
 *      Contributors:   Roger A. Bringmann and Scott Mahlke
 *
 *==========================================================================*/


#include <stdio.h>
#if 0
/* This doesn't exist everywhere, and malloc is usually
 * declared in stdlib.h now.  We may have to use preproc
 * variables if some platforms need this.
 */
#include <malloc.h>
#endif
#include <string.h>
#include <stdlib.h>
#include "l_alloc_new.h"

/* 
 * Set this to 1 before creating an alloc pool to bypass alloc routines
 * so that malloc and free are called each time L_alloc and L_free are called
 * with that alloc pool. 
 * This allows efficient use of the debug malloc routines.
 */

int bypass_alloc_routines = 0;

L_Alloc_Pool *L_create_alloc_pool (char *name, int size, int num_in_block)
{
    L_Alloc_Pool *pool;
    int pool_size, name_size;

    /* Make sure valid values are passed */
    if (name == NULL)
    {
	fprintf (stderr, "L_create_alloc_pool: name NULL\n");
	exit (1);
    }

    /* Force size to be at least big enough to hold header,
     * (so can make linked list of free elements, even
     * if element size is < 4)
     */
    if (size < (int) sizeof (L_Alloc_Pool_Header))
	size = (int) sizeof (L_Alloc_Pool_Header);

    /*
     * Force size to be a multiple of 4.
     * Otherwise get alignment problems with the L_Alloc_Pool_Header.
     * We may need to force to multiples of 8 (if double alignment 
     * becomes a problem), but I think sizeof() forces the size 
     * to be a mutliple of 8 if any doubles are in the structure. -JCG
     */
    size = (((size + 3) >> 2) << 2);

    /* Must allocate at least one elment in the block */
    if (num_in_block < 1)
    {
	fprintf (stderr, "L_create_alloc_pool: num_in_block (%i) < 1\n",
		 num_in_block);
	exit (1);
    }

    /* Get size to allocate */
    pool_size = sizeof (L_Alloc_Pool);
    name_size = strlen (name) + 1;
    
    /* Allocate pool and buffer space for name */
    if (((pool = (L_Alloc_Pool *) malloc (pool_size)) == NULL) ||
	((pool->name = (char *) malloc (name_size)) == NULL))
    {
	fprintf (stderr, "L_create_alloc_pool: Out of memory\n");
	exit (1);
    }

    /* Initialize structure for nothing allocated */
    strcpy (pool->name, name);
    pool->element_size = size;
    pool->num_in_block = num_in_block;
    /*
     * Get the size of block to allocate, include space for
     * block header (to keep track of blocks allocated.
     *
     * Note:  We must double align the header and the input size to
     * prevent access violations for doubles.
     */
    pool->block_size = (size * num_in_block) + 
       (((sizeof (L_Alloc_Pool_Header)+7)>>3)<<3);

    pool->head = NULL;
    pool->allocated = 0;
    pool->free = 0;
    pool->blocks_allocated = 0;
    pool->block_list = NULL;

    /* Read flag about whether the routines should be bypassed and
     * malloc and free used every time.  See message at top of file.
     */
    pool->bypass_routines = bypass_alloc_routines;

    /* Return the pool */
    return (pool);
}

/*
 * Frees all the memory allocated for a memory pool.
 * All the individual elements in the pool must be
 * free or this command will print warning messages
 * and not free the memory.
 */
void L_free_alloc_pool (L_Alloc_Pool *pool)
{
    L_Alloc_Pool_Header *block, *next_block;

    /* Make sure NULL pointer not passed */
    if (pool == NULL)
    {
	fprintf (stderr, "L_free_alloc_pool: pool is NULL\n");
	exit (1);
    }

    /* Cannot free pool if elements are still in use */
    if (pool->allocated != pool->free)
    {
	fprintf (stderr,
	      "Warning: Cannot free pool '%s'.  %d elements still in use!\n",
		 pool->name, (pool->allocated - pool->free));
	return;
    }
    
    /* Free each block allocated for the pool */
    for (block = pool->block_list; block != NULL; block = next_block)
    {
	/* Get next block before we free block */
	next_block = block->next;
	
	/* Free the block */
	free (block);
    }

    /* Free the alloc pool name */
    free (pool->name);

    /* Free the pool itself */
    free (pool);
}


/* Allocates one element from the pool specified */
void *L_alloc(L_Alloc_Pool *pool)
{
    int  i;
    L_Alloc_Pool_Header *header, *block_header;
    char *block;

    /* Make sure NULL pointer is not passed */
    if (pool == NULL)
    {
	fprintf (stderr, "L_alloc: NULL pool pointer\n");
	exit (1);
    }

    /*
     * If routine should be bypassed, call malloc directly.
     * allows debug malloc library to be used effectively
     */
    if (pool->bypass_routines)
    {
	/* Update alloc stats */
	pool->allocated++;

	return ((void *)malloc (pool->element_size));
    }

    /* If there are no more free elements, allocate a block of them */
    if (pool->head == NULL)
    {
        /* Allocate the block of memory */
        if ((block = (char *) malloc (pool->block_size)) == NULL)
	{
            fprintf (stderr,
                     "L_alloc (%s): Out of memory (request size %i)\n",
                     pool->name,  pool->block_size);
            exit(1);
        }

	/* Get the head of block for block list */
	block_header = (L_Alloc_Pool_Header *) block;
	block += (((sizeof (L_Alloc_Pool_Header)+7)>>3)<<3);

	/* Add to head of block list */
	block_header->next = pool->block_list;
	pool->block_list = block_header;

        /* Break rest of block into pieces and put into free list */
        for (i=0; i < pool->num_in_block; i++)
	{
            /* typecast current element so can put into free list */
            header = (L_Alloc_Pool_Header *) block;
            header->next = pool->head;
            pool->head = header;

            /* Goto next element */
            block += pool->element_size;
        }

        /* Update stats */
        pool->allocated += pool->num_in_block;
        pool->free += pool->num_in_block;
	pool->blocks_allocated += 1;
    }

    /* Get element from head of list */
    header = pool->head;
    pool->head = header->next;

    /* Update stats */
    pool->free--;

    /* Return element */
    return ((void *) header);
}

/* Puts the element pointed to in ptr into the pool specified */
void L_free (L_Alloc_Pool *pool, void *ptr)
{
    L_Alloc_Pool_Header *header;

    /* Make sure don't try to free NULL pointer */
    if (ptr == NULL)
    {
        fprintf (stderr, "L_free (%s): NULL pointer passed\n", pool->name);
        exit(-1);
    }

    /*
     * If routine should be bypassed, call free directly.
     * Allows debug malloc library to be used effectively.
     */
    if (pool->bypass_routines)
    {
	/* Update free stats */
	pool->free++;
	free (ptr);
	return;
    }

    /* Typecast so can put info free list */
    header = (L_Alloc_Pool_Header *)ptr;

    /* Put into free list */
    header->next = pool->head;
    pool->head = header;

    /* Update stats */
    pool->free++;

    /* If we have freed more than we have allocated, something is wrong */
    if (pool->free > pool->allocated)
    {
        fprintf (stderr, "L_free (%s): More freed than allocated\n",
                 pool->name);
        exit (1);
    }
}

/*
 * Prints how many elements were allocated, how many blocks of memory
 * were allocated and how many free elements in the list there are.
 */
void L_print_alloc_info(FILE *F, L_Alloc_Pool *pool,  int verbose)
{
    if ((verbose) ||
        (pool->allocated != pool->free))
    {
	if (pool->bypass_routines)
	{
	    fprintf(F, 
		    "    %-11s (BYPASSED): allocated %-5d  free %-5d  blocks %-5d\n",
		    pool->name, pool->allocated, pool->free,
		    pool->blocks_allocated);

	}
	else
	{
	    fprintf(F, 
		    "    %-11s: allocated %-5d  free %-5d  blocks %-5d\n",
		    pool->name, pool->allocated, pool->free,
		    pool->blocks_allocated);
	}
    }
}


