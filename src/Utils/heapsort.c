/* heapsort.c */
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/* John May, 16 November 2000
 * Implements heapsort based on Baase, _Computer Algorithms_,
 * 2nd Edition, p. 77.
 * Heapsort is an n log n (worst and average case), in place algorithm.
 */

/* User can pass in an anonymous pointer, which is passed on to
 * the comparison function.  This lets the comparison use
 * additional data in a separate data structure, so an array
 * of indices can be sorted according to the actual data items
 * they index.  Elements in the array are copied bitwise to their
 * new locations as they are sorted.
 */

#include <stdlib.h>
#include <assert.h>

#include "heapsort.h"

static void fixheap( int * base, int first, int key, int last, 
		int (*comp)(int, int, const void *),
		const void * extra_data );

/* This optional step moves all elements of the array identified as "null"
 * to the end.  The idea is to speed up sorting of sparse arrays.  The
 * presort takes linear time and operates in place.  Null elements are
 * kept in order as they are moved to the end, but nonnull elements
 * may be shuffled.
 */
void presort( int * base, int nelem, int * nfilled,
		int (*isnull)(int, const void *), const void * extra_data )
{
	int i, lastfull, temp;

	lastfull = -1;
	for( i = nelem - 1; i >= 0; i-- ) {
		if( isnull( base[i], extra_data ) ) {
			/* If there's a non-empty we've seen, swap these */
			if( lastfull > i ) {
				temp = base[i];
				base[i] = base[lastfull];
				base[lastfull] = temp;
				lastfull--;	/* should be no empty spaces
					   between lastfull and i, inclusive */
			}
		} else if( lastfull == -1 ) {	/* first nonempty from end */
			lastfull = i;
		}
	}
	*nfilled = lastfull + 1;
}

void tg_heapsort( int * base, int nelem, 
		int (*comp)(int, int, const void *),
		const void * extra_data )
{
	/* The heap is built in place.  It has the property that
	 * each root node is >= either of its children.  Element
	 * 0 is at the top of the heap.  Each level follows in
	 * order, so the children of any node i are at 2i + 1 and
	 * 2i + 2 (in a 0-based array).
	 */

	int i;
	int max;

	/* First construct the heap.  Each subheap (lowest-level
	 * node plus any leaf nodes) is sent fo fixheap for
	 * arranging, and then we work backward through the array
	 * to the top levels, fixing each higher-level heap.
	 */
	for( i = (nelem / 2) - 1; i >= 0; --i ) {
		fixheap( base, i, base[i], nelem - 1,
				comp, extra_data );
	}

	/* Now sort the array by removing the root node (which is
	 * the largest element in the heap) from index 0 and
	 * placing it at the last unused location in the array.
	 * Then call fixheap to redo the heap structure.
	 */
	for( i = (nelem - 1); i > 0; i-- ) {
		max = base[0];
		fixheap( base, 0, base[i], i - 1, comp, extra_data );
		base[i] = max;
	}
}

#define LEFTCHILD(x) (2 * x + 1)
#define RIGHTCHILD(x) (2 * x + 2)
/* Pass in a heap with a vacant node at the top and a key to insert.
 * Function will bubble the vacant node down to the location where the
 * key belongs in the heap, and the store the key in that node.
 */
void fixheap( int * base, int first, int key, int last, 
		int (*comp)(int, int, const void *),
		const void * extra_data )
{
	int vacant, larger;
	int left, right;

	vacant = first;
	/* While the vacant node has at least one child */
	while( (left = LEFTCHILD( vacant )) <= last ) {
		/* Find the larger of the two children */
		larger = left;
		right = RIGHTCHILD( vacant );
		if( left < last &&
			comp( base[right], base[left], extra_data ) > 0 ) {
			larger = right;
		}

		/* If the new key is smaller than the larger child... */
		if( comp( key, base[larger], extra_data ) < 0 ) {
			/* Move the larger child data into the vacant node
			 * and make the larger child the new vacant node
			 */
			base[vacant] = base[larger];
			vacant = larger;
		} else {
			/* Node is larger than either child, so this 
			 * is where the new key belongs.
			 */
			break;
		}
	}

	/* Vacant should now point the node where the key belongs */
	base[vacant] = key;
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

