//! \file TGxmlserver.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
//! An XML-based collector that reads in Tool Gear XML incrementally and
//! sends it to the Client.   Works like tail -f, continuously looks for 
//! new XML in input file until Client exits, allowing this XML collector
//! to be used for both post-morten (e.g., MpipView and UmpireView) and 
//! and live (e.g. MemcheckView) message viewers.
//!
//! This collector includes the standard capabilities for
//! serving source code and changing directories.
// Core collector infrastructure and tail functionality by 
// John May,  9 September 2004
// Incremental XML parsing by
// John Gyllenhaal, 21 March 2005

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* TRUE and FALSE not defined on some systems */
#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

#include "command_tags.h"
#include "lookup_function_lines.h"
#include "tg_socket.h"
#include "tg_pack.h"
#include "collector_pack.h"
#include "search_path.h"
#include "tg_source_reader.h"
#include "tg_typetags.h"
#include "lineparser.h"
#include "tempcharbuf.h"
#include "messagebuffer.h"
#include "logfile.h"
#include "tg_time.h"
#include "socketmanager.h"

// Flag that XML input is setting status messages
bool statusSet = FALSE;

//! Class for incrementally grabbing coherient Tool Gear XML snippets from
//! a file without using seek (no rewinding file).   
class XMLSnippetParser
{
public:
    XMLSnippetParser (FILE *file_in) : in (file_in), partialParse(FALSE),
				       lastLT(0), lineNo(1),
				       snippetLineOffset(0)
	{
	}
    //! Returns complete and coherient XML snippet containing at least one
    //! top-level XML <tag></tag> pair.   Uses knowledge of valid Tool Gear
    //! top-level XML tags to grab snippets, not an XML parser (so very
    //! little checking of XML correctness.   
    const char *getNextSnippet()
	{
	    // If not mid parse, clear current snippet contents and state
	    if (!partialParse)
	    {
		// Clear snippet buffer
		sbuf.clear();

		// Last < symbol at start of buffer
		lastLT = 0;

		// Record how many lines skipped before the snippet
		snippetLineOffset = lineNo-1;
	    }
	    
	    // Read characters until run out of characters or hit
	    // the end of a snippet
	    int ch;
	    while ((ch = fgetc(in)) != EOF)
	    {
		// If got '<', get index in string we are going to place it
		if (ch == '<')
		    lastLT = sbuf.strlen();

		// Add character to end of snippet buffer
		sbuf.appendChar(ch);

		// If got '>', see if at end of known snippet terminator
		if (ch == '>')
		{
		    // Get pointer to last element or end of element marker
		    const char *element = sbuf.contents() + lastLT;
		    
		    // DEBUG
//		    fprintf (stderr, "Last element: '%s' (%i-%i)\n", element,
//			     lastLT, sbuf.strlen());

		    // Is it a start tool_gear marker?
		    if (isStartElement (element, "tool_gear"))
		    {
//			fprintf (stderr, "Deleting Tool_Gear marker '%s'!\n",
//				 element);
			sbuf.truncate(lastLT);
		    }

		    // Is it a end of element marker we recognize?
		    else if (isEndElement (element, "message") ||
			     isEndElement (element, "site_data") ||
			     isEndElement (element, "message_folder") ||
			     isEndElement (element, "site_priority") ||
			     isEndElement (element, "tool_title") ||
			     isEndElement (element, "site_column") ||
			     isEndElement (element, "about"))
		    {
//			fprintf (stderr, "End snippet marker %s detected!\n", 
//				 element);
			partialParse = FALSE;
			return (sbuf.contents());
		    }

		    // Is it an end status marker?
		    else if (isEndElement (element, "status"))
		    {
			// Flag that XML setting status
			statusSet = TRUE;
			partialParse = FALSE;
			return (sbuf.contents());
		    }

		    // Is it a end tool_gear marker?
		    else if (isEndElement (element, "tool_gear"))
		    {
//			fprintf (stderr, "Deleting Tool_Gear marker '%s'!\n",
//				 element);
			sbuf.truncate(lastLT);

			// If has XML in there, return it now
			const char *ptr = sbuf.contents();
			char ch;
			while ((ch=*ptr) != 0)
			{
			    if (ch == '<')
			    {
				fprintf (stderr, 
					 "\nTool Gear XML collector Warning: \n"
					 "   Unrecognized XML at end, sending:\n"
					 "   '%s'\n",
					 sbuf.contents());
				partialParse = FALSE;
				return (sbuf.contents());
			    }
			    ptr++;
			}
		    }
		}

		// Increment line number if newline
		if (ch == '\n')
		    ++lineNo;
	    }
	    // If got here, must be in partial parse
	    partialParse = TRUE;

	    // Return that there is no complete snippet ready yet
	    return (NULL);
	}

    //! Returns the number of lines skipped/processed before the snippet
    //! Used to correlate XML snippet back to original file lines.
    int getSnippetOffset () {return (snippetLineOffset);}

private:
    bool isEndElement (const char *element, const char *name)
	{
	    // Expect element[0] == '<'
	    if (element[0] != '<')
	    {
		fprintf (stderr, "Error in XMLSnippetParser::isEndElement: \n"
			 "  Expect < not '%c' in '%s'!\n", element[0],
			 element);
		exit (1);
	    }

	    // For now, treat as end element only if starts with </
	    // (Not expecting <foo/> right now as select ending element)
	    if (element [1] != '/')
		return (FALSE);
	    
	    // Get length of name to limit search
	    int nameLen = strlen (name);
	    
	    // Expect name to follow / if end element of this name
	    if (strncmp (&element[2], name, nameLen) != 0)
		return (FALSE);

	    // Have </name but need to make sure 'name' not part of
	    // longer name by testing character after name in element
	    // (that is, distinguish </message> from </message_folder>
	    int nextChar = element[2+nameLen];
	    if ((nextChar != '>') && !isspace(nextChar))
		return (FALSE);

	    // If got here, must be end element for 'name'
	    return (TRUE);
	}

    bool isStartElement (const char *element, const char *name)
	{
	    // Expect element[0] == '<'
	    if (element[0] != '<')
	    {
		fprintf (stderr, "Error in XMLSnippetParser::isStartElement!"
			 "Expect < not '%c'!\n", element[0]);
		exit (1);
	    }

	    // Get length of name to limit search
	    int nameLen = strlen (name);
	    
	    // Expect name to follow < if start element of this name
	    if (strncmp (&element[1], name, nameLen) != 0)
		return (FALSE);

	    // Have <name but need to make sure 'name' not part of
	    // longer name by testing character after name in element
	    // (that is, distinguish <message> from <message_folder>
	    int nextChar = element[1+nameLen];
	    if ((nextChar != '>') && !isspace(nextChar))
		return (FALSE);

	    // If got here, must be end element for 'name'
	    return (TRUE);
	}


    MessageBuffer sbuf;
    FILE *in;
    bool partialParse;
    int lastLT;
    int lineNo;
    int snippetLineOffset;
};

// Thread-safe version of error string library for IBM
#ifdef THREAD_SAFE
#define USE_STRERROR_R 
#endif

// Function prototypes
int connectToClient( int port );
int check_socket( int sock );
void unpack_change_dir( char * buf, int sock );
void unpack_input_params( char * buf );
int check_continue_search(void);
void parse_input (XMLSnippetParser &XMLParser, SocketManager &sm);

TGSourceReader * sourceReader;
bool wait_for_input = TRUE;
char input_file_name[PATH_MAX + 1];
bool unlink_input_file = FALSE;


// Flag that we are terminating normally
static bool normal_termination = FALSE;


// Make output socket global (but static) so
// exit_cleanup() can send nice disconnect message
int sock = -1;

// Handle any cleanup at exit we need, intially what is needed
// is a delay at exit so all the ssh-tunnelled messages make it
// out so users actually see the error message.
void exit_cleanup(void)
{

    // If we are not terminating normally, sleep a few seconds before
    // exit to allow ssh to forward our stderr output
    if (!normal_termination)
    {
	fflush (stdout);
	fprintf (stderr, 
		 "(Tool Gear XML server flushing output before "
		 "abnormal termination...)\n");
	fflush (stderr);
	
	// Tell GUI that we are exiting, if possible
	if (sock != -1)
	{
	    TG_flush(sock);
	    TG_send( sock, DPCL_SAYS_QUIT, 0, 0, NULL );
	    TG_flush(sock);
	}

	// OK, 1 second is long enough if X is not through SSH
	// However, with X thru ssh, the client hasn't come up
	// before this dies.  Times that do not work in this case: 1, 2, 
	// For me, 3 seconds seems sufficient, so make 5 seconds to be safe
	sleep (5);
    }
}

void usage( char * program )
{
	fprintf( stderr, "Usage: %s <port>\n", program );
	fprintf( stderr, "   Expects to be called by TGclient to parse an XML file and serve source.\n" );
	fprintf( stderr, "   Expects to read data from file passed by TGclient\n" );
}


int main (int argc, char *argv[])
{
    // Register our at_exit cleanup routine 
    atexit (exit_cleanup);
    
    // Expect at least one argument, the port
    if( argc < 2 ) {
	usage( argv[0] );
	return -1;
    }
    
    // The port to which we should connect to talk to the Client
    int port = atoi( argv[1] );

    // Make the connection and get a socket id back
    sock = connectToClient( port );

    // Create socket manager to provide faster and more robust message
    // packing and sending.  The raw socket is still used in some places
    // in this release.
    SocketManager sm (sock);

    // Create static message viewer to let user know something
    // is happening.
    sm.sendCreateViewer (STATIC_MESSAGE_VIEW);
    
    // Data isn't sent to the Client until this function
    // is called.
    sm.flush();

    // Get the name of the file to read; it will be placed
    // in input_file_name (see check_socket below for details)
    int last_tag;
    fd_set readfds;
    do {
	// block until some data is ready (don't want to
	// waste cycles spinning)
	FD_ZERO( &readfds );
	FD_SET( sock, &readfds );
	// This program must not use the threaded version of
	// the tg_socket functions, because this select will
	// compete with the read thread to be notified of
	// incoming data.
	int ready = select( sock + 1, &readfds, NULL, NULL, NULL );
	if( ready < 1 && errno != EINTR ) {
		fprintf( stderr, "select returned %d; errno is %d\n",
				ready, errno );
	} else if (ready == 0 ) {
		fprintf( stderr, "select returned 0\n");
	}
    } while( (last_tag = check_socket( sock ) )
			!= DPCL_INITIALIZE_APP_SEQUENTIAL &&
		last_tag != DPCL_INITIALIZE_APP_PARALLEL &&
		last_tag != SOCKET_ERROR );

    if( last_tag == SOCKET_ERROR ) {
	    fprintf( stderr, "(1) Collector detected error reading socket\n" );
	    exit( -1 );
    }
    

#if 0
    // Try openning directly with fopen now -JCG 11/07/05

    // Read and process input data
    int fd = open( input_file_name, O_RDONLY );
    if( fd < 0 ) {
	    fprintf( stderr, "Failed to open input file %s\n",
			    input_file_name );
	    exit( -1 );
    }
    // Create FILE version of fd
    FILE *in = fdopen (fd, "r");
    if ( in == NULL)
    {
	    fprintf( stderr, "Failed to convert input file %s handle to FILE\n",
			    input_file_name );
	    exit( -1 );
    }
#endif
    FILE *in = NULL;

    // If input file is -, use stdin
    if (strcmp(input_file_name, "-") == 0)
	in = stdin;
    else
	in = fopen (input_file_name, "r");
    if (in == NULL)
    {
	    fprintf( stderr, "Failed to open input file %s\n",
			    input_file_name );
	    exit( -1 );
    }
    
    // If -unlink option specified, unlink the input file now.
    // This should not inhibit reading it but should make sure the
    // file is cleaned up even if the user hits ^C
    if (unlink_input_file)
    {
//	fprintf (stderr, "Warning: Unlinking '%s'\n", input_file_name);
	unlink (input_file_name);
    }

    // DEBUG, create XML parser
    XMLSnippetParser  XMLParser (in);

    parse_input (XMLParser, sm);

    // Let the user that we have sent all the data from the file
    // Only do this if XML doesn't set status message itself
    // Memcheck controls it's status messages
    if (!statusSet)
    {
	sm.sendStaticDataComplete ();
    }
    sm.flush();
    
    struct timeval timeout, *tvp;
    // Process requests (usually for source code) until we're done
    do {
	// block until some data is ready (don't want to
	// waste cycles spinning)
	FD_ZERO( &readfds );
	FD_SET( sock, &readfds );

	// Also watch input file, if reqested.  Break out of the
	// select call every second to check the file.  (doing
	// select directly on any open fd always returns ready,
	// even if it's at eof, so we don't want to add fd to readfds!)
	if( wait_for_input ) {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		tvp = &timeout;
	} else {
		// No timeout if we're not watching a file
		tvp = NULL;
	}

	// This program must not use the threaded version of
	// the tg_socket functions, because this select will
	// compete with the read thread to be notified of
	// incoming data.
	int ready = select( sock + 1, &readfds, NULL, NULL, tvp );
	if( ready < 0 && errno != EINTR ) {
		last_tag = SOCKET_ERROR;
		break;
	}

	// Check for input from the client
	if( ready > 0 &&  FD_ISSET( sock, &readfds ) ) {
		last_tag = check_socket( sock );
	}

	// Check for file input (even if select returned
	// because there was socket data)
	if( wait_for_input) {
	    parse_input(XMLParser, sm);
	}

    } while( last_tag != GUI_SAYS_QUIT
		    && last_tag != SOCKET_ERROR ) ;

    if( last_tag == SOCKET_ERROR ) {
	    fprintf( stderr, "(2) Collector detected error reading socket\n" );
	    sock = -1;
	    exit( -1 );
    }
    
    // Flag that we are terminating normally
    normal_termination = TRUE;

    // Tell the GUI we are exiting
    TG_send( sock, DPCL_SAYS_QUIT, 0, 0, NULL );
    TG_flush(sock);
    
    return 0;
}

//! Process a single GUI request (usually for source code or the name of the
//! the XML file to parse)
int check_socket( int sock )
{
	int tag, id, size;
	char * buf;

	if( TG_recv( sock, &tag, &id, &size, (void **)&buf ) < 1 ) {
		return SOCKET_ERROR;
	}

	//fprintf( stderr, "Collector got %d (%s) tag %d size %d buf %s\n",
	//		tag, command_strings[tag], id, size, buf );

	switch( tag ) {
	case DPCL_READ_FILE:
		if( ! sourceReader ) {
			sourceReader = new TGSourceReader( sock,
					check_continue_search );
		}
		sourceReader->read_file( buf, id );
		break;
	case DPCL_CHANGE_DIR:
		unpack_change_dir( buf, sock );
		break;
	case DPCL_INITIALIZE_APP_PARALLEL:
	case DPCL_INITIALIZE_APP_SEQUENTIAL:
		unpack_input_params( buf );
		break;
	case COLLECTOR_GET_SUBDIRS:
		if( ! sourceReader ) {
			sourceReader = new TGSourceReader( sock,
					check_continue_search );
		}
		sourceReader->unpack_get_subdirs( buf );
		break;
	case COLLECTOR_REPORT_SEARCH_PATH:
		if( ! sourceReader ) {
			sourceReader = new TGSourceReader( sock,
					check_continue_search );
		}
		sourceReader->report_search_path( buf );
		break;
	case COLLECTOR_SET_SEARCH_PATH:
		sourceReader->unpack_set_source_path( buf );
		break;
	};

	free( buf );
		

	return tag;
}


// Establishes a connection with the Tool Gear Client on a 
// specified port and performs the hand shake.  This involves
// reading a random number generated by the Client and sending
// it back.  This provides rudimentary assurance to the Client
// that the program talking to it is the one it launched.
// However, this technique is not secure against a real
// adversary, only an accidental connection.
int connectToClient( int port )
{
        int sock = socket( PF_INET, SOCK_STREAM, 0 );
        if( sock == -1 ) {
                perror( "error creating socket" );
                return -1;
        }

#if 0
        // Socket must be nonblocking for TG_send and TG_recv
        if( fcntl( sock, F_SETFL, O_NONBLOCK ) == -1 ) {
                fprintf( stderr, "Error making collector socket nonblocking" );
        }
#endif

	// Setting TCP_NODELAY avoids pauses for short messages
	int ndelay = 1;
	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, &ndelay, sizeof(ndelay) );


        struct sockaddr_in address;
	bzero( &address, sizeof(address) );
        address.sin_family = AF_INET;
        struct hostent * hp = gethostbyname( "localhost" );
        if( hp == NULL ) {
                perror( "error looking up localhost address" );
                return -1;
        }

        bcopy( hp->h_addr, &address.sin_addr, hp->h_length );
        address.sin_port = htons( port );

        int err = connect( sock, (struct sockaddr *)&address,
                        sizeof(address) );
        if( err != 0 ) {
                perror( "connect failed" );
                return -1;
        }

        // Socket must be nonblocking for TG_send and TG_recv
        if( fcntl( sock, F_SETFL, O_NONBLOCK ) == -1 ) {
                fprintf( stderr, "Error making collector socket nonblocking" );
        }

	// Shake hands with the spawning process; this also
	// verfies to the client that the process talking to
	// it over the socket is the one it launched.
//	fprintf(stderr, "collector is alive using socket %d!\n", sock);
	char checkValue[20];
	char * f = fgets( checkValue, 20, stdin );
	if( f == NULL ) {
		if( errno ) {
			perror( "reading in collector");
			exit(-1);
		}
	}
	char *end_ptr = NULL;
	unsigned int outval = strtoul(checkValue, &end_ptr, 0);
	if ((*end_ptr != 0) && (*end_ptr != '\n'))
	{
	    perror ("Error parsing authentication value 'outval'");
	    exit (-1);
	}
	write( sock, (void *) &outval, sizeof(outval) );

        return sock;
}

// Callback sent to findFind so it can periodically see if the
// user has asked to cancel the search.  Returns nonzero if the
// search should continue.
int check_continue_search( void )
{
    if( TG_poll_socket(sock) ) {
        int tag;
        if( (tag = check_socket( sock )) == COLLECTOR_CANCEL_READ_FILE ) {
	    return FALSE;
        } else if( tag == DPCL_SET_HEARTBEAT ) {
	    return TRUE;	// Ignore heartbeat messages
        } else if( tag == GUI_SAYS_QUIT ) {
            // Flag that we are terminating normally
            normal_termination = TRUE;

            // Tell the GUI we are exiting
            TG_send( sock, DPCL_SAYS_QUIT, 0, 0, NULL );
            TG_flush( sock );
	} else {
            fprintf( stderr, "Unexpected message while reading file: %s\n",
                command_strings[tag] );
	    return FALSE;
	}
    }
    return TRUE;
}

void unpack_change_dir( char * buf, int sock )
{
	char * directory;
	char * errstring;
#ifdef USE_STRERROR_R
	char result_string[500];
#endif

	TG_unpack( buf, "S", &directory );
	int retval = chdir( directory );
	if( retval ) {
		perror( "cd" );
#ifdef USE_STRERROR_R
		strerror_r( errno, result_string,
			sizeof(result_string) );
		errstring = result_string;
#else
		errstring = strerror(errno);
#endif
	} else {
		errstring = NULL;
	}


	pack_and_send_int_result( retval, errstring,
		DPCL_CHANGE_DIR_RESULT, sock );
}

void unpack_input_params( char * buf )
{
	int num_args;
	char * arg_strings;

	TG_unpack( buf, "A", &num_args, &arg_strings );

	// arg_strings is a sequence of num_args strings, each terminated
	// by a \0.  The XML file name should be the first of these.
	if( num_args < 1 ) {
		fprintf( stderr, "No Tool Gear XML file sent to data collector\n" );
		return;
	}
	
	// Get the file name, and avoid overrunning the buffer
	strncpy( input_file_name, arg_strings, sizeof(input_file_name) );
	input_file_name[sizeof(input_file_name) - 1] = 0;

	// For now, only -unlink expected after log file, which tells
	// viewer to unlink (delete) the file after openning it
	if (num_args > 1)
	{
	    // Skip past \0 to next string
	    arg_strings += strlen( arg_strings ) + 1;
	    
	    // Is it '-unlink'?
	    if (strcmp (arg_strings, "-unlink") == 0)
		unlink_input_file = TRUE;
	    else
		fprintf (stderr, "Warning option '%s' ignored!\n", 
			 arg_strings);
	}

#if 0
	// Here's how to look at any additional arguments that were sent
	if( num_args > 1 ) {
		int i;
		for(  i = 1; i < num_args; ++i ) {
			// Skip past \0 to next string
			arg_strings += strlen( arg_strings ) + 1;
			fprintf( stderr, "\"%s\" ", arg_strings );
		}
	}
#endif
}

// Parses input, return control when run out input (there may be parial
// line left when this returns
void parse_input(XMLSnippetParser &XMLParser, SocketManager &sm)
{
    // Get pointer to const char * snippet returned by XMLParser
    const char *snippet = NULL;
    
    // Read the file and look for the list of call sites
    // Also read in pieces of the file header in the process
    while ((snippet = XMLParser.getNextSnippet()) != NULL)
    {
#if 0
	fprintf( stderr, 
		 "--------------\n"
		 "%s\n"
		 "--------------\n",
		 snippet);
#endif

	// Send XML Snippet to be parsed and processed
	// Snippet does not need to be wrapped with <tool_gear></tool_gear>
	// (although it can be).   It will automatically be wrapped
	// with <tool_gear_snippet></tool_gear_snippet> before processing
	// so the XML parser will accept multiple command snippets.
	sm.sendXMLSnippet (snippet, XMLParser.getSnippetOffset());
    }
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

