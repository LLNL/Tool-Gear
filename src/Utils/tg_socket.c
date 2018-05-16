/* tg_socket.c */
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

/* Simple socket library for passing messages between two
 * threads or processes (i.e., DCPL and QT).  Both sockets
 * are put into non-blocking mode and the read/write
 * routines are designed to handle this.
 * 
 * Created by John C. Gyllenhaal, 4/21/00 
 * John May--added write queuing in separate thread 3/22/01
 * John May--added read queuing in separate thread 12/5/01
 */
#if defined(USE_WRITE_THREAD) || defined(USE_READ_THREAD)
#include <pthread.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include "tg_socket.h"
#include "tg_swapbytes.h"
#include "tg_error.h"
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>  /* defines select() on tru64 */


#if defined(USE_WRITE_THREAD) || defined(USE_READ_THREAD)
static int forever = 0;	/* avoids compiler warning for infinite loop */
typedef struct _queue_entry {
	int tag;
	int id;
	int size;
	void * buf;
	struct _queue_entry * next;
} Queue_entry;

typedef struct {
	int		fd;
	pthread_t	thread;
	pthread_cond_t	cond;
	pthread_mutex_t	lock;
	Queue_entry * head;
	Queue_entry * tail;
} Socket_thread;
int socket_closed = 0;	/* set to nonzero if socket is seen to be closed */
/* Create threads with a 1 MB stack */
#define THREAD_STACK_SIZE (1<<20)
#endif

#ifdef USE_WRITE_THREAD
#define NUM_WRITE_THREADS 2
static Socket_thread write_socket_thread[NUM_WRITE_THREADS];
static void * TG_write_queue_handler(void * thread_data);
static int TG_init_write_thread( int fd )
#endif

#ifdef USE_READ_THREAD
#define NUM_READ_THREADS 2
static Socket_thread read_socket_thread[NUM_READ_THREADS];
static void * TG_read_queue_handler(void * thread_data);
static int TG_init_read_thread( int fd );
#endif

#ifdef USE_LOG
#include <ctype.h>
FILE * logfile = NULL;
#endif

static int TG_internal_nb_recv (int fd, int *tag, int *id, int *size,
		void **buf);

#ifdef USE_WRITE_THREAD
/*! Works for a maximum of NUM_THREADS threads (probably only one is
 * needed for a process that isn't talking to itself.)
 */
int TG_init_write_thread( int fd )
{
	int i;
	int rc;
	pthread_attr_t attr;

	rc = pthread_attr_init( &attr );
	if( rc != 0 ) {
		TG_error( "TG_init_write_thread: "
				"pthread_attr_init returned %d", rc );
	}
	rc = pthread_attr_setstacksize( &attr, THREAD_STACK_SIZE );
	if( rc != 0 ) {
		TG_error( "TG_init_write_thread: "
				"pthread_attr_setstacksize returned %d", rc );
	}

	/* Find an empty slot in our list of threads and initialize it */
	for( i = 0; i < NUM_WRITE_THREADS; i++ ) {
		if( write_socket_thread[i].fd == 0 ) { /* default initial val */
			write_socket_thread[i].fd = fd;
			write_socket_thread[i].head
				= write_socket_thread[i].tail = NULL;

			rc = pthread_cond_init( &(write_socket_thread[i].cond),
					NULL );
			if( rc != 0 )
				    TG_error( "TG_init_write_thread: "
					    "pthread_cond_init returned %d",
					    rc );
			rc = pthread_mutex_init( &(write_socket_thread[i].lock),
					NULL );
			if( rc != 0 )
				    TG_error( "TG_init_write_thread: "
					    "pthread_mutex_init returned %d",
					    rc );
			rc = pthread_create( &(write_socket_thread[i].thread),
					&attr, TG_write_queue_handler,
					(void *)(&(write_socket_thread[i])) );
			if( rc != 0 )
				    TG_error( "TG_init_write_thread: "
					    "pthread_create returned %d", rc );
			break;
		}
	}
	
	pthread_attr_destroy( &attr );

	if( i == NUM_WRITE_THREADS )
		TG_error( "TG_init_write_thread: ran out of thread slots\n" );
}
#endif

#ifdef USE_READ_THREAD
int TG_init_read_thread( int fd )
{
	int i;
	int rc;
	pthread_attr_t attr;

	rc = pthread_attr_init( &attr );
	if( rc != 0 ) {
		TG_error( "TG_init_read_thread: "
				"pthread_attr_init returned %d", rc );
	}
	rc = pthread_attr_setstacksize( &attr, THREAD_STACK_SIZE );
	if( rc != 0 ) {
		TG_error( "TG_init_read_thread: "
				"pthread_attr_setstacksize returned %d", rc );
	}


	/* Find an empty slot in our list of threads and initialize it */
	for( i = 0; i < NUM_READ_THREADS; i++ ) {
		if( read_socket_thread[i].fd == 0 ) { /* default initial val */
			read_socket_thread[i].fd = fd;
			read_socket_thread[i].head
				= read_socket_thread[i].tail = NULL;

			rc = pthread_cond_init( &(read_socket_thread[i].cond),
					NULL );
			if( rc != 0 )
				    TG_error( "TG_init_read_thread: "
					    "pthread_cond_init returned %d"
						    "errno = %d",
						    rc, errno );
			rc = pthread_mutex_init( &(read_socket_thread[i].lock),
					NULL );
			if( rc != 0 )
				    TG_error( "TG_init_read_thread: "
					    "pthread_mutex_init returned %d"
						    "errno = %d",
						    rc, errno );
			rc = pthread_create( &(read_socket_thread[i].thread),
					&attr, TG_read_queue_handler,
					(void *)(&(read_socket_thread[i])) );
			if( rc != 0 ) {
				perror("pthread_create failed");
				TG_error( "TG_init_read_thread: "
				    "pthread_create returned %d"
				    "errno = %d", rc, errno );
			}

			break;
		}
	}

	pthread_attr_destroy( &attr );
	
	if( i == NUM_READ_THREADS )
		TG_error( "TG_init_read_thread: ran out of thread slots\n" );
	return i;
}
#endif


/* Internal routine for writing to a non-blocking socket.
 * Handles all the retries necessary and punts on error.
 */
static void TG_internal_write (int fd, char *buf, int size)
{
    int size_left, size_written;
    char *buf_left;

    /* Handle non-atomic writes */
    buf_left = buf;
    size_left = size;

    /* Loop until everything has been written out */
    while (size_left > 0)
    {
	/* Write out as much as can */
	if ((size_written = write (fd, (void *)buf_left, size_left)) <= 0)
	{
	    /* For now, only loop if errno is EAGAIN or EINTR */
	    if ((errno == EAGAIN) || (errno == EINTR))
	    {
#ifdef USE_LOG
		fprintf( logfile, "write interrupted; sent %d bytes\n",
				size_written );
		fflush( logfile );
#endif
		continue;
	    }
	    else
	    {
		/* Punt otherwise */
		TG_errno ("TG_internal_write: error during write(%i, x, %i)!",
			  fd, size_left);
	    }
	}
	/* If sucessfully write something, updates pointer and counter */
	else
	{
	    buf_left += size_written;
	    size_left -= size_written;
	}
    }
}


/* Internal routine for reading from a non-blocking socket.
 * If no data is available for the first read, it returns 0.
 * Otherwise, it will read all the data (with retries) and return 1.
 * If the read returns 0 (socket closed, but no error), return -1
 * Punts on error.
 */
static int TG_internal_read (int fd, char *buf, int size)
{
    int size_left, size_read;
    char *buf_left;

    /* Handle non-atomic writes/reads */
    buf_left = buf;
    size_left = size;

    /* Loop until everything has been read in */
    while (size_left > 0)
    {
	/* Read in as much as can */
	if ((size_read = read (fd, (void *)buf_left, size_left)) < 0)
	{
	    /* If errno == EAGAIN and no data has been read, return 0 now */
	    if ((errno == EAGAIN) && (size_left == size))
		return (0);

	    /* For now, only loop if errno is EAGAIN or EINTR */
	    if ((errno == EAGAIN) || (errno == EINTR))
	    {
#ifdef USE_LOG
		    int sockerr;
		    size_t size = sizeof(int);
		    getsockopt( fd, SOL_SOCKET, SO_ERROR, &sockerr, &size );
		fprintf( logfile, "read interrupted; got %d bytes, error %d\n",
				size_read, sockerr );

		fflush( logfile );
#endif
		continue;
	    }
	    else
	    {
		/* Punt otherwise */
		TG_errno ("TG_internal_read: error during read(%i, x, %i)!",
			  fd, size_left);
	    }
	}
	else if ( size_read == 0 ) 
	{
		/* Socket is probably closed; let caller deal with it */
		return -1;
	}
	/* If sucessfully read something, updates pointer and counter */
	else
	{
#ifdef USE_LOG
	    int i; /* debug only */

	    if( logfile == NULL ) {
		char name[20];
		sprintf( name, "log%d", getpid() );
		logfile = fopen( name, "w" );
	    }
	    fprintf( logfile, "got %d bytes: ", size_read );
	    for( i = 0; i < size_read; i++ ) {
		    if(isprint(buf_left[i]))
			    putc( buf_left[i], logfile );
		    else
			    fprintf(logfile, "<%x>", buf_left[i]);
            }

	    putc('\n', logfile );
	    fflush(logfile);
#endif

	    buf_left += size_read;
	    size_left -= size_read;
	}
    }
    return (1);
}

/* Writes tag, id, size, and then size bytes of buf to
 * the non-blocking socket fd (opened with IT_open_sockets).  
 * Punts on error.  
 */
void TG_send (int fd, int tag, int id, int size, const void *buf)
{
#ifdef USE_WRITE_THREAD
    int i;
    Queue_entry * entry;
#else
    int header[3];
#endif

    /* Sanity check, size must be >= 0 */
    if (size < 0)
    {
	TG_error ("TG_send: size (%i) < 0!", size);
    }

#ifndef USE_WRITE_THREAD

	/* Fill header with tag, id, and size */
    	/* Swapping is only done for the header; we assume the
	 * message body is text.
	 */
        if( tg_need_swap ) {
		header[0] = SWAP_BYTES(tag);
		header[1] = SWAP_BYTES(id);
		header[2] = SWAP_BYTES(size);
	} else {
		header[0] = tag;
		header[1] = id;
		header[2] = size;
	}

#ifdef USE_LOG
	if( logfile == NULL ) {
		char name[20];
		sprintf( name, "log%d", getpid() );
		logfile = fopen( name, "w" );
	}
	fprintf(logfile, "writing tag %d id %d size %d\n", tag, id, size );
	if( buf ) {
		fwrite( buf, sizeof(char), size, logfile );
		putc('\n', logfile);
	}
	fflush(logfile);
#endif
	
	/* Write header out */
	TG_internal_write (fd, (char *)header, sizeof(header));

	/* Only write buffer, if size is > 0 */
	if (size > 0) {
	    /* Write buffer out */
	    TG_internal_write (fd, (char *)buf, size);
	}
#else
    /* Determine which thread data corresponds to the requested fd */
    for( i = 0; i < NUM_WRITE_THREADS; i++ ) {
	    if( fd == write_socket_thread[i].fd ) break;

	    /* If not found yet, initialize this thread */
	    if( write_socket_thread[i].fd == 0 ) {
		    TG_init_write_thread( fd );
		    break;
	    }
    }

    /* Didn't find the requested fd in our list of known fds;
     * we could just send the data directly on the socket without
     * putting it in the queue, but to help us find bugs, we'll
     * just call this an error.
     */
    if( i == NUM_WRITE_THREADS ) {
	    TG_error( "TG_send: unknown socket (%d)", fd );
    }

    /* Create a new queue entry and put it on the end of the queue */
    entry = (Queue_entry *)malloc( sizeof(Queue_entry) );
    if( entry == NULL ) {
	    TG_error( "TG_send: malloc failed for new queue entry" );
    }

    if( size > 0 ) {
	    entry->buf = (void *)malloc(size);
	    if( entry->buf == NULL ) {
		    TG_error( "TG_send: malloc failed for buffer of size %d",
				    size );
	    }
	    memcpy( entry->buf, buf, size );
    } else {
	    entry->buf = NULL;
    }

    entry->tag = tag;
    entry->id = id;
    entry->size = size;
    entry->next = NULL;

    /* Put this new entry in the queue of messages to send. */
    pthread_mutex_lock( &(write_socket_thread[i].lock) );
    if( write_socket_thread[i].tail != NULL )
	    write_socket_thread[i].tail->next = entry;
    write_socket_thread[i].tail = entry;
    if( write_socket_thread[i].head == NULL )
	    write_socket_thread[i].head = entry;
    pthread_mutex_unlock( &(write_socket_thread[i].lock) );
#endif

}

/* Wrapper for TG_internal_nb_recv -- allows it to work the same
 * for both threaded and nonthreaded reading.  Returns -1 if
 * the socket has closed; returns 0 if no data available or > 0
 * if data was read successfully.
 */
int TG_nb_recv (int fd, int *tag, int *id, int *size, void **buf)
{
#ifdef USE_READ_THREAD
	int retval = TG_read_data_queued( fd );
	if( retval > 0 ) {
		TG_recv( fd, tag, id, size, buf );
	}
	return retval;
#else
	return TG_internal_nb_recv( fd, tag, id, size, buf );
#endif
}

/* Reads tag, id, size, and then size bytes of buf from
 * the non-blocking socket fd.
 * If no data is available, returns 0 .
 * If socket has closed (with no error), returns -1.
 * Otherwise, mallocs buf, fills in entries, and returns 1.
 * Punts on error.  
 */
int TG_internal_nb_recv (int fd, int *tag, int *id, int *size, void **buf)
{
    int header[3];
    int retval;

    /* Read header, return 0 is no message waiting */
    if ( (retval = TG_internal_read (fd, (char *)header, sizeof(header))) == 0)
    {
	/* Sanity check, clear all fields */
	*tag = 0;
	*id = 0;
	*size = 0;
	*buf = NULL;
	return (0);
    } else if( retval == -1 ) {
	*tag = 0;
	*id = 0;
	*size = 0;
	*buf = NULL;
	return -1;
    }

    /* Pull tag, id, and size out of header */
    if( tg_need_swap ) {
	    *tag = SWAP_BYTES(header[0]);
	    *id = SWAP_BYTES(header[1]);
	    *size = SWAP_BYTES(header[2]);
    } else {
	    *tag = header[0];
	    *id = header[1];
	    *size = header[2];
    }

#ifdef USE_LOG
    fprintf( logfile, "received tag %d, id %d, size %d\n",
		    *tag, *id, *size );
    fflush( logfile );
#endif

    /* Sanity check */
    if (*size < 0)
    {
	TG_error ("TG_internal_nb_recv: size recieved (%i) < 0!", *size);
    }
    
    /* If size is 0, no buffer sent (set to NULL)*/
    if (*size == 0)
    {
	*buf = NULL;
    }
    /* Otherwise, malloc buffer and read data */
    else
    {
	/* Malloc buffer */
	if ((*buf = (void *)malloc (*size)) == NULL)
	{
	    TG_error ("TG_internal_nb_recv: Out of memory allocating %i bytes!",
			   *size);
	}

	/* Loop until data recieved (since non-blocking pipe) */
	while ((retval = TG_internal_read (fd, (char *)*buf, *size)) == 0)
	    ;
	if( retval == -1 ) return -1;
    }
    return (1);
}

/* Blocking version of TG_nb_recv, will only return when data is ready.
 * Returns -1 socket closed or an error occurred.  Otherwise returns > 0.
 */
int TG_recv (int fd, int *tag, int *id, int *size, void **buf)
{

#ifndef USE_READ_THREAD
	fd_set readfds;
	int ready;
	do {
		FD_ZERO( &readfds );
		FD_SET( fd, &readfds );
		ready = select( fd + 1, &readfds, NULL, NULL, NULL );
		/* restart if select was interrupted */
	} while( ready < 0 && errno == EINTR );

	return TG_nb_recv( fd, tag, id, size, buf );

#else /* USE_READ_THREAD */

    /* Figure out which thread is handling this file descriptor */
    int i;
    Socket_thread * queue_info;
    Queue_entry * entry;

    for( i = 0; i < NUM_READ_THREADS; i++ ) {
	    if( fd == read_socket_thread[i].fd ) break;

	    /* If not found yet, initialize this location 
	     * (assumes slots in array are always filled in
	     * order, so threads should never be removed)
	     */
	    if( read_socket_thread[i].fd == 0 ) {
		    TG_init_read_thread( fd );
		    break;
	    }
    }

    if( i == NUM_READ_THREADS ) {
	    TG_error( "Couldn't find or allocate a thread for fd %d\n", fd );
    }
    
    queue_info = &(read_socket_thread[i]);
    
    /* Acquire the lock and look at the queue for data */
    pthread_mutex_lock( &(queue_info->lock) );
    if( socket_closed ) {
	    	/* If the socket is already closed, give up and return */
		pthread_mutex_unlock( &(queue_info->lock) );
		return -1;
    }

    /* Block until the reader thread signals that data is ready */
    while( queue_info->head == NULL ) {
	    pthread_cond_wait( &(queue_info->cond), &(queue_info->lock) );
    }

    /* Get the data off the queue */
    entry = queue_info->head;
    *tag = entry->tag;
    *id = entry->id;
    *size = entry->size;
    *buf = entry->buf;
#ifdef DEBUG_QUEUE
    fprintf( stderr, "Reading tag %d id %d size %d\n\n", *tag, *id, *size );
#endif

    /* Remove the queue entry */
    queue_info->head = entry->next;
    if( entry->next == NULL ) queue_info->tail = NULL;
    free( entry );

    pthread_mutex_unlock( &(queue_info->lock) );

    return 1;
#endif /* USE_READ_THREAD */

}

void TG_flush( int fd )
{
#ifdef USE_WRITE_THREAD
    int i;

    for( i = 0; i < NUM_THREADS; i++ ) {
	    if( fd == write_socket_thread[i].fd ) break;
    }

    if( i == NUM_THREADS ) {
	    TG_error( "TG_send: unknown socket (%d)", fd );
    }

    /* Wake up the writer thread and tell it to clear the queue */
    pthread_cond_signal( &(write_socket_thread[i].cond) );
#else
    fd = fd;	/* avoid compiler warnings */
#endif
}

#ifdef USE_WRITE_THREAD
void * TG_write_queue_handler(void * thread_data)
{
    Socket_thread * socket_handler = thread_data;
    Queue_entry * local_head, * item, * next;
    int header[3];

    while( !forever ) {
	/* Get the lock that controls the queue */
	pthread_mutex_lock( &(socket_handler->lock) );

	/* Wait for some data to appear in the queue; we must hold
	 * the lock whenever we examine the queue.  The first time
	 * through, the lock statement above guarantees this.  On
	 * later iterations, pthread_cond_wait acquires the lock.
	 */
	while( socket_handler->head == NULL ) {
		pthread_cond_wait( &(socket_handler->cond),
				&(socket_handler->lock) );
	}

	/* Transfer the queue to local control and release the
	 * lock so that the other thread can continue adding
	 * data to it.
	 */
	local_head = socket_handler->head;
	socket_handler->head = socket_handler->tail = NULL;

	pthread_mutex_unlock( &(socket_handler->lock) );

	for( item = local_head; item != NULL; item = next ) {

		/* Fill header with tag, id, and size */
		if( tg_need_swap ) {
			header[0] = SWAP_BYTES(item->tag);
			header[1] = SWAP_BYTES(item->id);
			header[2] = SWAP_BYTES(item->size);
		} else {
			header[0] = item->tag;
			header[1] = item->id;
			header[2] = item->size;
		}

		/* Write header out */
		TG_internal_write (socket_handler->fd, (char *)header,
				sizeof(header));

		/* Only write buffer, if size is > 0 */
		if (item->size > 0) {
		    /* Write buffer out */
		    TG_internal_write (socket_handler->fd, (char *)(item->buf),
				    item->size);
		}

		if( item->buf != NULL )
			free( item->buf );
		next = item->next;
		free( item );
	}
    }

    return NULL;
}
#endif

#ifdef USE_READ_THREAD
void * TG_read_queue_handler(void * thread_data)
{
    Socket_thread * socket_handler = thread_data;
    Queue_entry * item;
    int tag, id, size;
    void * buf;
#ifdef DEBUG_QUEUE
    int i;
#endif

    while( !forever ) {
	    /* Read data from the incoming socket until a complete
	     * message arrives.
	     */ 
	    fd_set readfd;
	    do {
		    int ready;
		    FD_ZERO( &readfd );
		    FD_SET( socket_handler->fd, &readfd );
		    ready = select( socket_handler->fd + 1,
				    &readfd, NULL, NULL, NULL );
		    if( ready < 1 ) {
			    fprintf( stderr, "select returned %d; "
					    "errno is %d\n", ready, errno );
		    }
	    } while( TG_internal_nb_recv( socket_handler->fd,
				    &tag, &id, &size, &buf ) != 1 );

	    /* Put the new message in the incoming message queue */
	    item = (Queue_entry *)malloc( sizeof(Queue_entry) );
	    item->tag = tag;
	    item->id = id;
	    item->size = size;
	    item->buf = buf;
	    item->next = NULL;
#ifdef DEBUG_QUEUE
	    fprintf( stderr, "Queue adding tag %d id %d size %d\n",
			    tag, id, size);
	    for( i = 0; i < size; i++ ) {
		    if(isprint(((char*)buf)[i]))
			    putc( ((char*)buf)[i], stderr );
		    else
			    fprintf(stderr, "<%x>", ((char*)buf)[i]);
            }
	    putc( '\n', stderr );
#endif

	    pthread_mutex_lock( &(socket_handler->lock) );
	    if( socket_handler->tail != NULL )
		    socket_handler->tail->next = item;
	    socket_handler->tail = item;
	    if( socket_handler->head == NULL ) {
		    socket_handler->head = item;
	    }
#ifdef DEBUG_QUEUE
	    for( item = socket_handler->head; item != NULL;
			    item = item->next ) {
		    fprintf( stderr, "tag %d id %d size %d\n",
				    item->tag, item->id, item->size );
	    }
	    putc( '\n', stderr );
#endif
	    pthread_mutex_unlock( &(socket_handler->lock) );
	    pthread_cond_signal( &(socket_handler->cond ) );
	    
    }

    return NULL;
}

/* See if there's any data waiting on the queue.  Returns
 * -1 if the socket has closed and no data remains in the queue.
 */
int TG_read_data_queued( int fd )
{
    int i;
    int data_ready;

    for( i = 0; i < NUM_READ_THREADS; i++ ) {
	    if( fd == read_socket_thread[i].fd ) break;
    }

    if( i == NUM_READ_THREADS ) return 0;

    pthread_mutex_lock( &(read_socket_thread[i].lock) );
    data_ready = (read_socket_thread[i].head != NULL);
    if( !data_ready && socket_closed ) data_ready = -1;
    pthread_mutex_unlock( &(read_socket_thread[i].lock) );

    return data_ready;
}
#endif /* USE_READ_THREAD */

/* See if there is anything available to read, without blocking */
int TG_poll_socket( int fd )
{
    int ready;
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO( &readfds );
    FD_SET( fd, &readfds );
    timeout.tv_sec = timeout.tv_usec = 0;
    ready = select( fd + 1, &readfds, NULL, NULL, &timeout );
    return (ready > 0 );
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

