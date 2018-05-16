//! \file tg_collector.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 18 July 2001

#ifndef TG_COLLECTOR_H
#define TG_COLLECTOR_H

#include <unistd.h>

#include <qfile.h>
#include <qmessagebox.h>
#include <qptrlist.h>
#include <qobject.h>
#include <qsocket.h>
#include <qsocketnotifier.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qptrvector.h>

#include "tg_gui_listener.h"

//! Represents a process that collects data for Tool Gear,
//! on either a remote or a local machine.  Launches the
//! collector, establishes a connection to it, and provides
//! links to its stdin, stdout, stderr, and data sockets.
class TGCollector : public QObject {
	Q_OBJECT
public:
	// TGCollector(); // Need this for delayed launch?
	//! Launch and connect to a process that will collect data for Tool Gear
	//! collectorPath is the path to the collector program (either remote or
	//! local), targetArgs is an array of strings giving the target program
	//! name and its arguments.  remoteHost is the (optional) name of a
	//! remote system to run the collector on, loginName is the 
	//! (optional) user name on that system, and
	//! connectPort is the (optional) port number on the remote machine
	//! where ssh will connect; the latter two are ignored if
	//! remoteHost is null.
	TGCollector( QApplication * app, QString& collectorPath,
			QPtrVector<QString>& targetArgs,
			QString& remoteHost, QString& loginName,
		  	QString& connectPort );

	//! Launch and connect to a process that will collect data for Tool Gear
	//! collectorPath is the path to the collector program (either remote or
	//! local). remoteHost is the (optional) name of a remote system to run
	//! the collector on, loginName is the (optional) user name on that
	//! system, and connectPort is the (optional) port number on the
	//! remote machine where ssh will connect; the latter two are 
	//! ignored if remoteHost is null.
	TGCollector( QApplication * app, QString& collectorPath,
			QString& remoteHost, QString& loginName,
		  	QString& connectPort );
	//! Shut down the collector program and clean up the connections to it.
	~TGCollector();

	// QSocket * getQSocket() { return dataSocket; }
	//! Returns the socket id used for communicating with the Collector.
	int getSocket() { return sock; }

	//! Returns a pointer to the Qt stream object connected to stdin.
	QTextStream * getStdin() { return stdinStream; }
	//! Returns a pointer to the Qt stream object connected to stdout.
	QTextStream * getStdout() { return stdoutStream; }
	//! Returns a pointer to the Qt stream object connected to stderr.
	QTextStream * getStderr() { return stderrStream; }

	//! Read and display any pending data from the stdout or stderr
	//! pipes from the collector.  This function is normally called
	//! automatically when the Client exits and not during regular
	//! operation.
	void clearProcessOutput();
	
	enum State { Connected, Launching, Failed, Terminated };

	//! Returns the current status of the Collector process.
	State getStatus() const { return status; }
public slots:
	//! Should be called when another part of the application
	//! detects that the socket connection has closed.
	void handleClosedSocket();

signals:
	//! Emitted when data is ready on Collector's stdout.
	void stdoutReady( QTextStream * );

	//! Emitted when data is ready on Collector's stderr.
	void stderrReady( QTextStream * );

	//! Emitted when Collector process terminates.
	void terminated();

protected slots:
	//! Slot called when data arrives from the target process's stdout.
	//! It may also be called if connection is closed.
	void hasStdoutData( int );

	//! Slot called when data arrives from the target process's stderr.
	//! It may also be called if connection is closed.
	void hasStderrData( int );

	//! Slot used internally to establish the connection with 
	//! the collector process.
	void listenerConnected( int );

	//! Cleans up socket connections when they close unexpectedly
	void cleanup();

protected:
	//! Fork a process to run the collector (or talk to it
	//! through ssh).
	pid_t TGLaunchCollector( Q_UINT16 port,
			QString& collectorPath, QString& remoteHost,
			QString& loginName, QString& connectPort,
			QPtrVector<QString>& targetArgs );

	QApplication * a;
	QString remoteHostName;
	// Names of streams and files are from the collector process's
	// point of view; i.e., we write to the input stream to send
	// data to its stdin.
	QTextStream * stdinStream;
	QTextStream * stdoutStream;
	QTextStream * stderrStream;
	QFile input;
	QFile output;
	QFile error;
//	QSocket * dataSocket;
	QSocketNotifier * stdoutSN;
	QSocketNotifier * stderrSN;
	TGGuiListener * listener;
	State status;
	pid_t collectorProcess;
	int sock;

	//! The following strings trigger a message to the user to type
	//! some data on the command line.  This happens only when text
	//! containing one of these strings appears on the stderr stream
	//! of the collector process when the status is Launching.  The
	//! idea is that these are prompts for passwords or other
	//! interaction with the user.  We avoid putting up the dialog
	//! for all stderr output while launching because other messages
	//! may appear during that time that don't require a user response.
	//! Comparisons are case-insensitive, and regular expressions are
	//! permitted; see the QRegExp documentation.
	static char * promptStrings[];
	QMessageBox * mb;
};

//! Gathers any data from the Collector's stdout and stderr and
//! displays it to the user.  This function is intended to be called
//! just before the Client shuts down.
extern void clearCollectorOutput( void );

#endif // TG_COLLECTOR_H
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

