/* timermodule.c */
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

#include <pthread.h>
#include <assert.h>
#include <dpclExt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "timermodule.h"

extern pthread_key_t timer_stack_key;
pthread_key_t timer_stack_key;

/* We want a separate stack and stack pointer for each thread.
 * To get this, we use the pthread key facility, which manages
 * separate data values for each thread.  In this case, the
 * value is a block of memory that we'll allocate to hold the
 * stack and its pointer.  When the thread exits, we'll deallocate
 * the memory for that stack through the destructor function, 
 * which is called automatically.  To keep the pointer an the
 * timer data in the same block of memory, we'll do some ugly
 * type casts and assume a pointer will fit in the space 
 * needed for one timeval struct.
 */
#define MAX_DEPTH 200
#define TIMER_STACK_MEMORY (int)( sizeof(struct timeval *) \
		+ MAX_DEPTH * sizeof(struct timeval) )
static void StackDestructor( void * stack )
{
	/* This function shouldn't be called if stack is NULL,
	 * but just in case it is...
	 */
	if( stack != NULL ) {
		struct timeval * sp = *((struct timeval **)stack);
		/* The number of items on the stack is the
		 * difference between the stack pointer and
		 * the stack base, which is just past the memory
		 * where the stack pointer itself is stored.
		 */
		struct timeval * stackbase = (struct timeval *)((char *)stack
				+ sizeof(struct timeval *));
		int items = sp - stackbase;
		if( items != 0 ) {
			fprintf( stderr, "Thread %x exiting with "
					"%d pending timer %s\n",
					pthread_self(), items,
					(items == 1 ? "call" : "calls") );
		}

		free( stack );
		stack = NULL;
		pthread_setspecific( timer_stack_key, stack );
	}
}

void InitTimer( void )
{
	int retval;

	retval = pthread_key_create( &timer_stack_key, StackDestructor );
	assert( retval == 0 );
}

void * InitTimerThread( void )
{
	int retval;
	void * stack;

	stack = malloc( TIMER_STACK_MEMORY );
	assert( stack != NULL );

	/* The first location in this block will hold the stack pointer.
	 * It will initially point to the next location, where the first
	 * data value will be stored.  The char * cast ensures that the
	 * pointer arithmetic will be byte-wise.
	 */
	*((struct timeval **)stack) = (void *)((char *)stack
			+ sizeof(struct timeval *));

	retval = pthread_setspecific( timer_stack_key, stack );
	assert( retval == 0 );

	return stack;
}

void StartTimer()
{
	void * stack;
	struct timeval * handle;

	/* Get the stack for this thread, or create it if it doesn't
	 * exist yet.
	 */
	if( (stack = pthread_getspecific( timer_stack_key )) == NULL ) {
		stack = InitTimerThread();
	}

	/* Dereference stack pointer to get next location. */
	handle = *((struct timeval **)stack);

	/* Bump up the stack pointer and make sure it hasn't overflowed. */
	*((struct timeval **)stack) += 1;	/* 1 means one timeval unit */
	if( (char *)handle - (char *)stack >= TIMER_STACK_MEMORY ) {
		fprintf( stderr, "Timer stack overflow!\n" );
		/* Don't mess with the pointer if there's an overflow; let
		 * it come back in range with corresponding StopTimer calls.
		 * But don't write any data outside the stack bounds.
		 */
		return;	
	}

	gettimeofday( handle, NULL );
}

void StopTimer( AisPointer ais_send_handle )
{
	struct timeval stop;
	long sec;
	int usec;
	void * stack;
	struct timeval * handle;
	int stackused;
	struct timerresult result;

	gettimeofday( &stop , NULL );

	/* Get the corresponding start value from this thread's stack. */
	stack = pthread_getspecific( timer_stack_key );
	if( stack == NULL ) {
		fprintf( stderr, "Stop Timer called before any timer "
				"started for this thread!\n" );
		return;
	}

	/* Look at the last-used stack position; if stack was full,
	 * just decrement pointer and return.
	 */
	*((struct timeval **)stack) -= 1;
	handle = *((struct timeval **)stack);
	stackused = (char *)handle - (char *)stack;
	if( stackused >= TIMER_STACK_MEMORY ) {
		fprintf( stderr, "Stack full (%d bytes used) in thread %x\n", 
				stackused, pthread_self() );
		return;
	}
	
	/* See if we've underflowed the stack. */
	if( stackused <= 0 ) {
		fprintf( stderr, "Stop timer call doesn't match any start!\n" );

		/* Reset the stack pointer to the first entry. */
		*((struct timeval **)stack) =
			(void *) ((char *)stack + sizeof(struct timeval *));
		return;
	}

	/* No errors seen; just calculate elapsed time and send it off. */
	sec = stop.tv_sec - handle->tv_sec;
	usec = stop.tv_usec - handle->tv_usec;
	result.interval = sec + usec / 1e6;
	result.thread = pthread_self();

	Ais_send( ais_send_handle, &result, sizeof( result ) );
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

