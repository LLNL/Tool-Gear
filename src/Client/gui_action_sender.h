//! \file gui_action_sender.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 27 October 2000
// Based on a prototype by John Gyllenhaal

#ifndef GUI_ACTION_SENDER_H
#define GUI_ACTION_SENDER_H

#include <qobject.h>
#include <qptrlist.h>
#include "search_path_dialog.h"
#include "uimanager.h"
#include "tg_inst_spec.h"
#include "tg_program_state.h"

//! Sends messages from the Client to the Collector

//! The GUIActionSender centralizes transmission of data and
//! requests. The GUISocketReader does the same for incoming
//! data from the Collector.  This class
//! inherits from QObject so Qt's signal and slots mechanisms
//! can be used to receive requests from the the UIManager.
class GUIActionSender : public QObject
{
	Q_OBJECT

public:
	//! Initializes the sender.  We would expect to use one
	//! of these objects for each Collector, but we haven't
	//! tested communication with multiple collectors.
	GUIActionSender( int sock_out,	//!< Socket for outgoing messages
			 UIManager *m,	//!< UIManager that sends requests
			 TGProgramState *ps,
			 		//!< Information about target program
			 bool heartbeat	//!< TRUE causes heatbeat
					//!< messages to sent periodically
			 		//!< to the Collector
			 );
	~GUIActionSender() {}

signals:
	//! Emitted to indicate that a request to start or stop
	//! the target program has been sent to the Collector
	void programRunning(bool);
	//! Emitted to put a message in the GUI's program status
	//! line
	void setProgramMessage(const char *);

public slots:
	//! Sends a request to the Collector to parse the specified
	//! program module
	void parseModule( const char *moduleName);
	//! Sends a request to the Collector to parse the specified
	//! function
	void parseFunction ( const char *funcName);
	//! Sends a request to change to the specifie directory on
	//! the target computer
	void changeDirectory( const char *dirName );
	//! Sends a request to load (but not run) the target application
	//! or file.  Args is an array of strings, whose number is given
	//! by argCount.  Unlike the standard argv, it doesn't need to
	//! end with a pointer to NULL.
	//! If the isParallel parameter is FALSE, the Collector will
	//! try to determine automatically whether to run the application
	//! as a parallel job
	void initializeApp( int argCount, char **args, bool isParallel );
	//! Sends a request to instrument an application according to
	//! with a particular action (or to remove that action), at
	//! a specified set of locations.
	void instrumentApp( QPtrList<TGInstSpec> & inst_list );
	//! Sends a request to get all the names of the directories
	//! that are within the named directory.  The result is
	//! sent to the search path dialog sd, and the void *
	//! parent object tells this dialog where to insert the results.
	void getDirList( QString dirName, SearchPathDialog * sd,
			void * parent );
	//! Sends a request for the current search path on the collector.
	//! The result is sent to the search path dialog sd.
	void getSearchPath( SearchPathDialog * sd );
	//! Sends a new search path for the collector to use and 
	//! whether the path should be saved to a file
	void setSearchPath( QString path, int save_state );

private slots:
	//! Pass UIManger requests for files to be parsed to DPCL
	//!
        void fileStateChanged (const char *fileName, 
			       UIManager::fileState state);
	//! Pass UIManger requests for functions to be parsed to DPCL 
	//!
        void functionStateChanged (const char *funcName, 
				   UIManager::functionState state);
	//! Foward the information to DPCL over the socket; this version for
	//! one task/thread pair
	void actionActivated (const char *funcName, const char *entryKey, 
			      const char *actionAttrTag, int taskId, 
			      int threadId);
	//! Forward the information to DPCL over the socket; this version for
	//! one task/thread pair
	void actionDeactivated (const char *funcName, const char *entryKey, 
				const char *actionAttrTag, int taskId, 
				int threadId);
	//! Forward the information to DPCL over the socket; this version for
	//! all tasks/threads in a job
	void actionActivated (const char *funcName, const char *entryKey, 
			      const char *actionAttrTag);
	//! Forward the information to DPCL over the socket; this version for
	//! all tasks/threads in a job
	void actionDeactivated (const char *funcName, const char *entryKey,
				const char *actionAttrTag);
	//! Send start/stop message to Collector
	//!
	void startStopProgram( bool start );
	//! Shut down the GUI (sends no message; the TGCollector sends
	//! a quit message in its destructor)
	void quitProgram( void );

protected:
	//! Send a heartbeat message to the collector to keep it from timing out
	// !(and set the next timeout interval)
	void timerEvent( QTimerEvent * );

	TGProgramState * programState;
private:
	int socket_out;
	UIManager *um;
};

#endif // GUI_ACTION_SENDER_H
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

