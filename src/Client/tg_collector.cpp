// tg_collector.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 18 July 2001

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <qapplication.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qobject.h>
#include <qregexp.h>
#include <qsocket.h>
#include <qsocketnotifier.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qdatetime.h>
#include <qptrvector.h>

#include "tg_collector.h"
#include "tg_gui_listener.h"
#include "tg_socket.h"
#include "tg_swapbytes.h"
#include "command_tags.h"
using namespace std;

// The following strings trigger a message to the user to type
// some data on the command line.  This happens only when text
// containing one of these strings appears on the stderr stream
// of the collector process when the status is Launching.  The
// idea is that these are prompts for passwords or other
// interaction with the user.  We avoid putting up the dialog
// for all stderr output while launching because other messages
// may appear during that time that don't require a user response.
// Comparisons are case-insensitive, and regular expressions are
// permitted; see the QRegExp documentation.
char * TGCollector:: promptStrings[] = {
	"password", "passphrase"
};

// Maintain a list of all TGCollectors so that we can run through
// them at shutdown time and clear any remaining output.  Keeping
// the list as a static variable hides it from other uses and
// lets us create a simple, nonmember function call to do the
// whole job.
static QPtrList<TGCollector> collectorList;

// Launch and connect to a process that will collect data for
// Tool Gear collectorPath is the path to the collector program
// (either remote or local), targetArgs is an array of strings
// giving the target program name and its arguments.  remoteHost
// is the (optional) name of a remote system to run the collector
// on, loginName is the (optional) user name on that system, and
// connectPort is the (optional) port number on the remote machine
// where ssh will connect; the latter two are ignored if
// remoteHost is null.
TGCollector:: TGCollector( QApplication * app, QString&
		collectorPath, QPtrVector<QString>& targetArgs,
		QString& remoteHost, QString& loginName, QString&
		connectPort ) : a(app), remoteHostName(remoteHost),
	       	stdinStream(0), stdoutStream(0),
		stderrStream(0), stdoutSN(0), stderrSN(0),
		sock(-1), mb(0)
{
    // Set object name to aid in debugging connection issues
    setName ("TGCollector");

	listener = new TGGuiListener;
	if( ! listener->ok() ) {
		qWarning( "TGCollector couldn't initialize its listen port" );
		status = Failed;
		return;
	}

	// Once we launch the collector program, we will have to wait
	// for it to hook up with us.
	// Until that happens, this object is in an intermediate state.
	// When the connection happens, a signal will be emitted and the
	// rest of the setup will be completed.
	status = Launching;
	
	connect( listener, SIGNAL( isConnected(int) ),
			this, SLOT( listenerConnected(int) ) );

	// Launch the collector, either on this host or remotely.
	collectorProcess = TGLaunchCollector( listener->port(), collectorPath,
			remoteHost, loginName, connectPort, targetArgs );

	if( collectorProcess == -1 ) {
		qWarning( "TGcollector failed to launch collector program %s",
				collectorPath.latin1() );
		status = Failed;
		return;
	}

	stdinStream = new QTextStream( &input );
	stdoutStream = new QTextStream( &output );
	stderrStream = new QTextStream( &error );

	// Add this new object to the list of Collector connections
	collectorList.append( this );

}

// Launch and connect to a process that will collect data for Tool Gear
// collectorPath is the path to the collector program (either remote or
// local). remoteHost is the (optional) name of a remote system to run
// the collector on, loginName is the (optional) user name on that
// system, and connectPort is the (option) port number on the remote
// machine where ssh will connect; the latter two are ignored if
// remoteHost is null.
TGCollector:: TGCollector( QApplication * app, QString& collectorPath,
		QString& remoteHost, QString& loginName, QString& connectPort )
	:  a(app), remoteHostName( remoteHost ), stdinStream(0),
	stdoutStream(0), stderrStream(0), // dataSocket(0),
	stdoutSN(0), stderrSN(0), sock(-1), mb(0)
{

	listener = new TGGuiListener;
	if( ! listener->ok() ) {
		qWarning( "TGCollector couldn't initialize its listen port" );
		status = Failed;
		return;
	}

	// Once we launch the collector program, we will have to wait
	// for it to hook up with us.
	// Until that happens, this object is in an intermediate state.
	// When the connection happens, a signal will be emitted and the
	// rest of the setup will be completed.
	status = Launching;
	
	connect( listener, SIGNAL( isConnected(int) ),
			this, SLOT( listenerConnected(int) ) );

	// Launch the collector, either on this host or remotely.
	QPtrVector<QString> dummyArgs;	// empty list of target args
	collectorProcess = TGLaunchCollector( listener->port(), collectorPath,
			remoteHost, loginName, connectPort, dummyArgs );

	if( collectorProcess == -1 ) {
		qWarning( "TGcollector failed to launch collector program %s",
				collectorPath.latin1() );
		status = Failed;
		return;
	}

	stdinStream = new QTextStream( &input );
	stdoutStream = new QTextStream( &output );
	stderrStream = new QTextStream( &error );

	// Add this new object to the list of Collector connections
	collectorList.append( this );

}

// Shut down the collector program and clean up the connections to it.
#define QUIT_TIMEOUT_MILLISEC (10000)
TGCollector:: ~TGCollector()
{
	// The kind way to this is to send it a message.  We could also
	// kill it, if necessary.

	if( status == Connected ) {
		TG_send( sock, GUI_SAYS_QUIT, 0, 0, NULL );
		TG_flush( sock );


                // Give a more generic message -JCG 4/28/04
		//cerr << "Waiting for remote process to exit... ";
		//cerr << "Waiting for data collection process to exit... ";
		cerr << "Waiting for the Tool Gear data server process to exit... ";

		int tag = LAST_COMMAND_TAG, id, size;
		void * buf;

		QTime t;
		t.start();
		int retval;
		do {
			// Handle any pending events, but turn off
			// the socket notifiers for stdout/stderr
			// because when the collector exits, these
			// sockets will close, and we don't want
			// the event loop to interpret that as an error
			stdoutSN->setEnabled(FALSE);
			stderrSN->setEnabled(FALSE);
			a->processEvents();
			// Get all pending messages; return value of
			// -1 means socket was closed, so remote
			// process is presumably gone.
			while( ( retval = TG_nb_recv( sock, &tag, &id,
							&size, &buf ) ) > 0) {
				if( tag == DPCL_SAYS_QUIT ) break;
			}
		} while( tag != DPCL_SAYS_QUIT && retval != -1
				&& t.elapsed() < QUIT_TIMEOUT_MILLISEC );

		if( tag == DPCL_SAYS_QUIT || retval == -1 ) {
			cerr << "Done." << endl;
		} else {
			cerr << endl <<
			"Giving up!  Wait a minute or two, then" << endl <<
			"check for orphan processes on remote machine." << endl;
		}
	}

	// Closing a QFile opened with an existing handle (as these were)
	// does nothing, so we have to close the sockets explicitly
	if( status == Connected || status == Launching ) {
		close( input.handle() );
		close( output.handle() );
		close( error.handle() );
	}

	/* MS/START ASSUMED MOVED LOCATION BY JCG */
	// One last chance to look for output from the collector
	clearProcessOutput();
	/* MS/END ASSUMED MOVED LOCATION BY JCG */

	delete listener;
	delete stdinStream;
	delete stdoutStream;
	delete stderrStream;
	if( mb ) delete mb;
	if( collectorProcess != -1 )
		kill( collectorProcess, SIGKILL );

	// Remove this collector connection from the master list
	collectorList.removeRef( this );
}


// TG_SSH_COMMAND is defined in the qmake specification file tgclient.pro
#define TG_SSH_REMOTEPORT_OPT "-R"
#define TG_SSH_LOGNAME_OPT "-l"
#define TG_SSH_CONNECTPORT_OPT "-p"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

// Lanches the collector, either locally or on a remote machine using ssh.
pid_t TGCollector:: TGLaunchCollector( Q_UINT16 port, QString& collectorPath,
		QString& remoteHost, QString& loginName,
		QString& connectPort,
		QPtrVector<QString>& targetArgs )
{
	// How many items in our arg list when we fork?
	int execArgc = targetArgs.size() + 3;	// At least 3 extra args
	int execInd = 0;

#ifdef TG_REDIRECT_COLLECTOR
	//DEBUG
	execArgc += 2;	// For output redirection
#endif
	
	const char ** args;
	QString portRedir;	// must be declared in same scope as args

	// First decide whether we need to launch a remote process
	// or a local one; this is determined by whether a remote
	// host was specified.
	if( remoteHost.isNull() ) {
		// Build the command line by putting the command
		// name first in a list, followed by the port number
		args = new const char*[execArgc];
	} else {
		// Use ssh to remotely execute the collector process.
		// Tunnel the port from the remote process back to
		// this host, and tell the remote process which port
		// to use.  It's possible that the port number chosen
		// here could be unavailable on the remote side.
		// Will ssh detect that when it tries to set up its
		// tunnel?

		
		// Adjust count of args to exec and allocate the array
		execArgc += 4;	// extra args for ssh <host> -R <ports>

#ifdef DEBUG_SSH
		execArgc++;
#endif

		if( ! loginName.isNull() ) execArgc += 2; // -l <name>
		args = new const char*[execArgc];

		// syntax is: ssh [options] host [command]
		args[execInd++] = TG_SSH_COMMAND;
		

#ifdef DEBUG_SSH
		args[execInd++] = "-vvv";
#endif

		// login name, if given
		if( ! loginName.isNull() ) {
			args[execInd++] = TG_SSH_LOGNAME_OPT;
			args[execInd++] = loginName.latin1();
		}

		// connection port, if given
		if( ! connectPort.isNull() ) {
			args[execInd++] = TG_SSH_CONNECTPORT_OPT;
			args[execInd++] = connectPort.latin1();
		}

		// port redirection command
		args[execInd++] = TG_SSH_REMOTEPORT_OPT; 

		// The ssh redirection command causes connections
		// from the Collector to be forwarded to TGGuiListener
		// on this host.  It's important to specify the local
		// host as an IPv4 address, rather than "localhost".
		// Otherwise, some versions of ssh will try to make
		// an IPv6 connection to the localhost.  This will
		// fail, because we currently listen on an IPv4
		// port, and ssh won't retry on the IPv4 port.  We
		// could also force ssh to use only IPv4 by passing
		// the -4 option, but not all ssh versions support it.
		portRedir.sprintf( "%d:127.0.0.1:%d", port, port );
		args[execInd++] = portRedir.latin1();

		// remote host
		args[execInd++] = remoteHost.latin1();
	}

	// collector command
	args[execInd++] = collectorPath.latin1();

	// Port name
	QString portString;
	portString.setNum( port );
	args[execInd++] = portString.latin1();

	// Finish adding the remote args to the list that exec will get
	int i;
	for( i = 0; i < (int)targetArgs.size(); i++ ) {
		args[execInd++] = targetArgs[i]->latin1();
	}

#ifdef TG_REDIRECT_COLLECTOR
	// DEBUG
	args[execInd++] = ">&";
	args[execInd++] = "output";
#endif

	args[execInd] = NULL;

	// Set up pipes for redirecting stdin/stdout
	int to_parent[2], to_child[2], to_parent_error[2];
	pipe( to_parent );
	pipe( to_child );
	pipe( to_parent_error );

	// Fork and exec the collector, directly or through ssh
	if( (collectorProcess = fork()) == 0 ) {	// Child; run collector
		// Do the redirection by closing stdin, stdout & stderr
		// and then duping the ends of the pipe.
		close( 0 );
		dup( to_child[0] );
		close( 1 );
		dup( to_parent[1] );
		close( 2 );
		dup( to_parent_error[1] );

		// Done with these
		close( to_child[0] );
		close( to_child[1] );
		close( to_parent[0] );
		close( to_parent[1] );
		close( to_parent_error[0] );
		close( to_parent_error[1] );

		// Now exec the collector
		if( execv( args[0], (char * const *) args ) == -1 ) {
			perror( args[0] );
			exit(-1);
		}
	}

	// Parent; establish the pipe
	close( to_child[0] );
	close( to_parent[1] );
	close( to_parent_error[1] );

	if( collectorProcess != -1 ) {
		input.open( IO_WriteOnly, to_child[1] );

		if( fcntl( to_parent[0], F_SETFL, O_NONBLOCK ) == -1 ) {
			qWarning("Couldn't make collector stdout nonblocking");
		}
		output.open( IO_ReadOnly, to_parent[0] );
		stdoutSN = new QSocketNotifier( to_parent[0],
				QSocketNotifier::Read, this );
		connect( stdoutSN, SIGNAL( activated(int) ),
				this, SLOT( hasStdoutData(int) ) );

		if( fcntl( to_parent_error[0], F_SETFL, O_NONBLOCK ) == -1 ) {
			qWarning("Couldn't make collector stdout nonblocking");
		}
		error.open( IO_ReadOnly, to_parent_error[0] );
		stderrSN = new QSocketNotifier( to_parent_error[0],
				QSocketNotifier::Read, this );
		connect( stderrSN, SIGNAL( activated(int) ),
				this, SLOT( hasStderrData(int) ) );
	} else {
		close( to_child[1] );
		close( to_parent[0] );
		close( to_parent_error[0] );
	}

	delete [] args;

	return collectorProcess;
}

void TGCollector::clearProcessOutput()
{
	bool hasStdout, hasStderr;

	// Look for unread data from the collector and display it
	// Check both at once (not two separate loops) so we have
	// a better chance of reading everything that appears
	// before we quit.
	hasStdout = !stdoutStream->atEnd();
        hasStderr = !stderrStream->atEnd();
	while( hasStdout  || hasStderr)
        {
		// Call the slots that would be invoked if the
		// event loop detected data.  0 is dummy value.
		if( hasStdout ) hasStdoutData(0);
		if( hasStderr ) hasStderrData(0);
  
                // Is there more to process?
		hasStdout = !stdoutStream->atEnd();
        	hasStderr = !stderrStream->atEnd();
	}
}

void TGCollector:: hasStdoutData( int )
{
	// This slot can be called if the stream has been disconnected
	if( stdoutStream->atEnd() ) {
		cout << "Collector has terminated" << endl;
		cleanup();
		return;
	}

	// For testing:
	QString data = stdoutStream->readLine();
	cout << "collector stdout: " << data << endl;

	// For real, when ready:
	// emit stdoutReady( stdoutStream );
}

void TGCollector:: hasStderrData( int )
{
	// This slot can be called if the stream has been disconnected
	if( stderrStream->atEnd() ) {
		cout << "Collector has terminated" << endl;
		cleanup();
		return;
	}

	// Some interaction (such as a password request) might
	// be needed while we're launching the collector.
	QString data = stderrStream->readLine();
	
	// If there's an old message box up, delete it
	if( mb != 0 ) {
		delete mb;
		mb = 0;
	}

	bool doPrompt = FALSE;
	if( status == Launching ) {
		int i;
		int count = sizeof(promptStrings) / sizeof(char *);
		for( i = 0; i < count; i++ ) {
			QRegExp r = QRegExp( promptStrings[i], FALSE );
			if( data.contains( r ) ) {
				doPrompt = TRUE;
				break;
			}
		}
	}
	if( doPrompt ) {
		cerr << data;
		mb = new QMessageBox( remoteHostName + " login",
				"Please enter requested information on "
				"the command line.",
				QMessageBox::Information,
				QMessageBox::Ok, QMessageBox::NoButton,
				QMessageBox::NoButton, 0, 0, FALSE );
		mb->show();
	} else {
		// For testing:
		cout << "collector stderr: " << data << endl;

		// For real, when ready:
		// DEBUG JCG for stderr issue
//		emit stdoutReady( stderrStream );
	}
}

// Not needed?
// Request 256K socket buffer to handle large data volume (256K is
// max allowed by solaris; we may have to interactively choose a size
// for portability)
//#define SOCK_BUF_SIZE (1<<18)

// Slot used internally to establish the connection with the collector process
void TGCollector:: listenerConnected( int socket )
{
	// Get the data socket established
	//dataSocket = new QSocket( this );
	//dataSocket->setSocket( socket );
	sock = socket;

	// Avoid delays for short messages
	int ndelay = 1;
	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, &ndelay, sizeof(ndelay) );

	// Verify that the connection is from the collector we launched
	// by sending it a random number on its standard input and
	// looking for the same number back on the socket connection.
	
	// Get the random number, using the current time of day as a seed
	struct timeval t;
	gettimeofday( &t, NULL );
	srand( t.tv_usec );
	unsigned int randval = rand();

	// Send the number out and read it back.  We should pick a
	// reasonable timeout so we don't hang.
	char outbuf[20];
	sprintf( outbuf, "0x%08x\n", randval );
	*stdinStream << outbuf;
	//int err = dataSocket->waitForMore( -1 );	// CHANGE THIS
	unsigned int inval = 0;
	char * p = (char *)&inval;
	int size_left = sizeof(inval);
	bool failed = FALSE;
	// Although we're reading a small amount of data, there's no
	// guarantee that we'll get it all at once, so we have to
	// keep reading until all the data has arrived.
	while( size_left > 0 ) {
		int size_read = read( sock, (void *)p, size_left );
		if( size_read <= 0 ) {
			if( errno == EAGAIN || errno == EINTR ) {
				continue;
			} else {
				failed = TRUE;
				qWarning( "Couldn't read data from "
						"collector: %s",
				       strerror(errno) );
				break;
			}
		} else {
			p += size_read;
			size_left -= size_read;
		}
	}
	
	// Check for a match; if at first we don't succeed, see if
	// swapping the byte order helps and try again.
	if( !failed ) {
		if( inval == randval ) {
			tg_need_swap = 0;
		} else if( inval == SWAP_BYTES(randval) ) {
			tg_need_swap = 1;
		} else {
			failed = TRUE;
			qWarning( "Couldn't authenticate collector process; "
				"disconnecting (%i(0x%x) != %i(0x%x)",
				 randval, randval, inval, inval);
		}
	}
	
	if( failed ) {
		kill( collectorProcess, SIGKILL );
		close( input.handle() );
		close( output.handle() );
		close( error.handle() );
		status = Terminated;

		return;
	}

	// Socket should be nonblocking
	if( fcntl( sock, F_SETFL, O_NONBLOCK ) == -1 ) {
		qWarning( "Couldn't make client's socket nonblocking" );
	}

	status = Connected;
}

// Called when a closed socket is detected externally
void TGCollector:: handleClosedSocket()
{
	cleanup();
}

// Cleans up socket connections when they close unexpectedly
void TGCollector:: cleanup()
{
	qWarning( "Collector has disconnected" );

	disconnect( stdoutSN, SIGNAL( activated(int) ),
			this, SLOT( hasStdoutData(int) ) );
	disconnect( stderrSN, SIGNAL( activated(int) ),
			this, SLOT( hasStderrData(int) ) );

	close( input.handle() );
	close( output.handle() );
	close( error.handle() );

	status = Terminated;
	emit terminated();
}

void clearCollectorOutput( void )
{
	QPtrListIterator<TGCollector> it( collectorList );
	TGCollector * tgc;

	while( (tgc = it.current()) != 0 ) {
		++it;
		tgc->clearProcessOutput();
	}
}

//#include "tg_collector.moc"
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

