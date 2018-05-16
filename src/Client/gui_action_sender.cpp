// gui_action_sender.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 26 October 2000
// Catches notifications about actions from the GUI and forwards
// then to DPCL
#include <qapplication.h>
#include "gui_action_sender.h"
#include "tg_pack.h"
#include "tg_program_state.h"
#include "tg_socket.h"
#include "command_tags.h"
#include "messagebuffer.h"
#include "tg_time.h"

#define BUFFER_SIZE (1<<14)
#define HEARTBEAT_INTERVAL 5000	// 5000 milliseconds between heartbeats
#define HEARTBEAT_TIMEOUT 60 // 60 second timeout for collector to quit

GUIActionSender:: GUIActionSender( int sock_out, UIManager *m,
		TGProgramState * ps, bool heartbeat )
	:  programState(ps), socket_out( sock_out ), um( m )
{
    // Set object name to aid in debugging connection issues
    setName ("GUIActionSender");

    // Connect our file parse request handler
    connect (um, SIGNAL(fileStateChanged (const char *, UIManager::fileState)),
	     this, SLOT(fileStateChanged (const char *, 
					  UIManager::fileState)));

    // Connect our function parse request handler
    connect (um, SIGNAL(functionStateChanged (const char *, 
					      UIManager::functionState)),
	     this, SLOT(functionStateChanged (const char *, 
					      UIManager::functionState)));

    // Connect our activation signal handler
    connect(um, SIGNAL(actionActivated (const char *, const char *, 
					const char *, int , int)),
	    this, SLOT(actionActivated (const char *, const char *, 
					const char *, int ,int)));
    connect(um, SIGNAL(actionActivated (const char *, const char *, 
					const char *)),
	    this, SLOT(actionActivated (const char *, const char *, 
					const char *)));
    
    // Connect our deactivation signal handler
    connect(um,
	    SIGNAL(actionDeactivated (const char *, const char *, 
				      const char *, int , int)),
	    this, SLOT(actionDeactivated (const char *, const char *, 
					  const char *, int , int)));
    connect(um, SIGNAL(actionDeactivated (const char *, const char *, 
					  const char *)),
	    this, SLOT(actionDeactivated (const char *, const char *, 
					  const char *)));

    if( heartbeat ) {
	// Create a heartbeat timer to tell the collector that we're
	// still alive.
	startTimer( HEARTBEAT_INTERVAL );
	// Send a heartbeat right away in case we crash soon!
	timerEvent(NULL);
    }
}

void GUIActionSender:: actionActivated (const char *funcName, 
					const char *entryKey, 
					const char *actionAttrTag, int taskId, 
					int threadId)
{
	char buf[BUFFER_SIZE];
	int length;

	length = TG_pack( buf, BUFFER_SIZE, "SSSII", funcName, entryKey,
			actionAttrTag, taskId, threadId );

	TG_send( socket_out, DPCL_ACTIVATE_ACTION, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender::fileStateChanged (const char *fileName, 
					UIManager::fileState state)
{
    // If signaled that request file be parsed, request parse
    if (state == UIManager::fileParseRequested)
    {
	parseModule (fileName);
    }
}

void GUIActionSender::functionStateChanged (const char *funcName,
					    UIManager::functionState state)
{
    // If signaled that request file be parsed, request parse
    if (state == UIManager::functionParseRequested)
    {
	parseFunction (funcName);
    }
}



void GUIActionSender:: actionActivated (const char *funcName, 
					const char *entryKey, 
					const char *actionAttrTag)
{
	char buf[BUFFER_SIZE];
	int length;

	length = TG_pack( buf, BUFFER_SIZE, "SSS", funcName, entryKey,
			actionAttrTag );

	TG_send( socket_out, DPCL_ACTIVATE_ACTION_ALL, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: actionDeactivated (const char *funcName, 
					  const char *entryKey, 
					  const char *actionAttrTag, 
					  int taskId, 
					  int threadId)
{
	char buf[BUFFER_SIZE];
	int length;

	length = TG_pack( buf, BUFFER_SIZE, "SSSII", funcName, entryKey,
			actionAttrTag, taskId, threadId );

	TG_send( socket_out, DPCL_DEACTIVATE_ACTION, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: actionDeactivated (const char *funcName, 
					  const char *entryKey, 
					  const char *actionAttrTag)
{
	char buf[BUFFER_SIZE];
	int length;

	length = TG_pack( buf, BUFFER_SIZE, "SSS", funcName, entryKey,
			actionAttrTag );

	TG_send( socket_out, DPCL_DEACTIVATE_ACTION_ALL, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: parseModule (const char *moduleName)
{
	char buf[BUFFER_SIZE];
	int length;

#if 0
	fprintf (stderr, "parseModule sending '%s'\n", moduleName);
#endif
	length = TG_pack( buf, BUFFER_SIZE, "S", moduleName);

	TG_send( socket_out, DPCL_PARSE_MODULE, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: parseFunction (const char *funcName)
{
	char buf[BUFFER_SIZE];
	int length;

	length = TG_pack( buf, BUFFER_SIZE, "S", funcName );

	TG_send( socket_out, DPCL_PARSE_FUNCTION, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: changeDirectory( const char *dirName )
{
	char buf[BUFFER_SIZE];
	int length;

	length = TG_pack( buf, BUFFER_SIZE, "S", dirName );

	TG_send( socket_out, DPCL_CHANGE_DIR, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: initializeApp( int argCount, char **args,
					bool isParallel )
{
	char buf[BUFFER_SIZE];
	int length;

	// Need to create a single array containing the sequence of
	// args for TG_pack.
	MessageBuffer argString;
	int i;
	for( i = 0; i < argCount; ++i ) {
		argString.appendSprintf( "%s%c", args[i], '\0' );
	}

	length = TG_pack( buf, BUFFER_SIZE, "A", argCount,
			argString.contents() );

	TG_send( socket_out, (isParallel ? DPCL_INITIALIZE_APP_PARALLEL
		 : DPCL_INITIALIZE_APP_SEQUENTIAL), 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: getDirList( QString dirName, SearchPathDialog * sd,
		void * parent )
{
	char buf[BUFFER_SIZE];
	char p1str[20], p2str[20];
	int length;

	// Pack the two pointers and send them off as strings;
	// the other side will just copy them and send them back
	// with the response.
	sprintf( p1str, "%p", sd );
	sprintf( p2str, "%p", parent );

	length = TG_pack( buf, BUFFER_SIZE, "SSS", p1str, p2str, 
			(const char *)dirName );

	TG_send( socket_out, COLLECTOR_GET_SUBDIRS, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: getSearchPath( SearchPathDialog * sd )
{
	char buf[BUFFER_SIZE];
	char p1str[20];
	int length;

	// Pack the pointer and send it off as a string;
	// the other side will just copy it and send it back
	// with the response.
	sprintf( p1str, "%p", sd );

	length = TG_pack( buf, BUFFER_SIZE, "S", p1str );

	TG_send( socket_out, COLLECTOR_REPORT_SEARCH_PATH, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: setSearchPath( QString path, int save_state )
{
	int length;
	char * buf;

	// Estimated length is string length + \n + integer (in string form)
	// + 2 headers @ 12 chars each
	int est_length = path.length() + 1 + 10 + 2 * 12;
	buf = new char[ est_length ];

	length = TG_pack( buf, est_length, "IS", save_state, path.latin1() );
	TG_send( socket_out, COLLECTOR_SET_SEARCH_PATH, 0, length, buf );
	TG_flush( socket_out );
}

void GUIActionSender:: instrumentApp( QPtrList<TGInstSpec>& inst_list )
{
	// We need a buffer big enough to hold all the items in
	// this list.  We'll pack the buffer incrementally.
	int length, count = inst_list.count(), buf_size = count * BUFFER_SIZE;
	char * buf = new char[ buf_size ];

	// First, pack the number of list items.
	length = TG_pack( buf,  buf_size, "I", count );

	int size_left = buf_size - length;

	// Now run through the list and pack each entry in sequence
	// into the buffer.
	TGInstSpec * it;
	for( it = inst_list.first(); it; it = inst_list.next() ) {
		// Determine whether to insert or remove instrumentation
		// We may want to make this more flexible in the future FIX
		int insert = (it->stateTag == "In");

		length += TG_pack( buf + length, size_left, "SIIIS",
			it->actionTag.latin1(), insert,
			it->location, it->type,
			it->functionRE.latin1() );
		size_left = buf_size - length;

		// Don't overrun the buffer; send what we have now
		// and start over on this buffer
		if( size_left < BUFFER_SIZE ) {

			// Repack the count (depends on the fact
			// that integers are packed in a fixed amount
			// of space)
			TG_pack( buf, buf_size, "I", inst_list.at() );
			TG_send( socket_out, DPCL_INSTRUMENT_APP, 0,
					length, buf );
			TG_flush( socket_out );

			// Now start over on the buffer
			length = TG_pack( buf,  buf_size, "I",
					count - inst_list.at() - 1 );
			size_left = buf_size - length;
		}
	}

	TG_send( socket_out, DPCL_INSTRUMENT_APP, 0, length, buf );
	TG_flush( socket_out );

	delete [] buf;
}

void GUIActionSender:: startStopProgram( bool start )
{
	int action;
	if( start && programState->getActivity() == TGProgramState::Stopped ) {
		action = DPCL_START_PROGRAM;
		programState->setActivity(TGProgramState::Running);
		emit programRunning(TRUE);
		emit setProgramMessage("Program running; press Run to pause");
	} else if ( !start && programState->getActivity()
			== TGProgramState::Running ) {
		action = DPCL_STOP_PROGRAM;
		programState->setActivity(TGProgramState::Stopped);
		emit programRunning(FALSE);
		emit setProgramMessage("Program stopped; press Run to start");
	} else {
		return;
	}

	TG_send( socket_out, action, 0, 0, NULL );
	TG_flush( socket_out );
}

void GUIActionSender:: quitProgram( void )
{
	QApplication::exit();
}

void GUIActionSender:: timerEvent( QTimerEvent * )
{
	char buf[BUFFER_SIZE];
	int length;

	length = TG_pack( buf, BUFFER_SIZE, "I", HEARTBEAT_TIMEOUT );

	TG_send( socket_out, DPCL_SET_HEARTBEAT, 0, length, buf );
	TG_flush( socket_out );
}

//#include "gui_action_sender.moc"

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

