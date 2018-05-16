//! \file gui_socket_reader.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 26 October 2000

#ifndef GUI_SOCKET_SENDER_H
#define GUI_SOCKET_SENDER_H

#define CONTINUE_THREAD 1
#define QUIT_THREAD 0
#include <qobject.h>

#include "uimanager.h"
#include "tg_program_state.h"

//! Reads from the socket for the GUI

//! The GUISocketReader centralizes reception and processing of
//! messages from a Collector.  The GUIActionSender takes care
//! sending messages.  This class is derived from QObject so it
//! can use the Qt signal/slot mechanism.
class GUISocketReader : public QObject {
	Q_OBJECT
public:
	//! Create a GUISocketReader.  We expect there to be one
	//! GUISocketReader per Collector, but we haven't tested
	//! multiple Collectors.  A reader can either read and
	//! process messages as they arrive, or it can wait for an
	//! explicit request (through check_socket()).  The latter is
	//! useful at startup and shutdown, since it gives the
	//! application better control over the handshake.
	//! Therefore, the reader is started with auto-reading
	//! disabled.
	GUISocketReader( int sock_in,	//!< Socket for incoming messages
			 UIManager * m,	//!< UIManager that receives
					//!< data through this object
			 TGProgramState * ps
			 		//!< Information on program state
			 );
	~GUISocketReader();
	//! Turns automatic reading and processing of messages on or off.
	//! GUISocketReaders have auto-reading disabled upon initialization
	void enable_auto_read( bool set_enable );
	//! Returns state of auto-reading: TRUE if enabled, FALSE if not
	//!
	bool check_auto_read() { return auto_reading; }

public slots:
	//! Reads an incoming message (and blocks until one becomes
	//! available), then processes it according the the message tag
        //! If sizeRead != NULL, message size is returned
	int check_socket(int *sizeRead = NULL);
	//! Determines whether a message is pending, and calls check_socket()
	//! if it is
	void get_pending_data();
signals:
	//! Indicates whether the target program is loaded and runnable
	//!
	void programActive( bool );
	//! Indicates that the named module has been inserted in the database
	//!
	void newModule( char * module_name );
	//! Indicates that the named target process has terminated
	//!
	void processTerminated( char * proc_name );
	//! Indicates that the target program is running
	//!
	void programRunning( bool );
	//! Requests that a message be posted in the program status line
	//!
	void setProgramMessage( const char * message );
	//! Notify application that the socket closed unexpectedly
	//!
	void readerSocketClosed();
protected:
	//! Process a request to insert a database entry (corresponding
	//! to a location in the target program.)
	void unpack_and_insert_entry( char * buf );
	//! Process a request to insert a database "plain" database entry
	//! (corresponding to a program location that isn't a DPCL 
	//! instrumentation point)
	void unpack_and_insert_plain_entry( char * buf );
	//! Process a request to insert a target program function in the
	//! database
	void unpack_and_insert_function( char * buf );
	//! Process a request to insert a target program module (file) in the
	//! database
	void unpack_and_insert_module( char * buf );
	//! Process a request to insert a target program action the
	//! database.  Actions are things that DPCL can do to a target
	//! program (such as collecting a specific kind of data) at the
	//! user's request
	void unpack_and_declare_action_attr( char * buf );
	//! Process a request to make an action available for the user
	//! to request
	void unpack_and_enable_action( char * buf );
	//! Process a request to declare a data column in the database
	//! 
	void unpack_and_declare_data_attr( char * buf );
	//! Process a request to declare a process/thread pair in the target
	//! program
	void unpack_and_insert_process_thread( char * buf );
	//! Process a request to insert a double precision data value
	//! in the database for a particular column and entry
	void unpack_and_insert_double( char * buf );
	//! Process a request to insert an integer data value
	//! in the database for a particular column and entry
	void unpack_and_insert_int( char * buf );
	//! Process a request to add a double precision data value
	//! to an exisiting value in the database
	void unpack_and_add_double( char * buf );
	//! Process a request to add an integer data value
	//! to an exisiting value in the database
	void unpack_and_add_int( char * buf );
	//! OBSOLETE
	//!
	void unpack_and_set_target_info( char * buf );
	//! Declare parsing complete for the specified file
	//!
        void unpack_and_set_file_parse_complete ( char * buf);
	//! Declare parsing complete for the specified function
	//!
        void unpack_and_set_function_parse_complete ( char * buf);
	//! Get result from changing the directory, and post a 
	//! warning if it failed
        int unpack_cd_result( char * buf );
	//! Get result from initializing the app, and post a
	//! warning if it failed
        int unpack_init_app_result( char * buf );
	//! Tell the Client that the target halted (because it
	//! reached a breakpoint)
	void unpack_target_halted( char * buf );
	//! Tell the Client that the target terminated
	//!
	void unpack_target_terminated( char * buf );
	//! Store "About" text in the UIManager, for display in
	//! the viewer.
	void unpack_about_text( char * buf );
        //! Declare a new message folder in the UIManager
        //!
        void unpack_and_declare_message_folder (char *buf);
        //! Declare a new message in a messageFolder in the UIManager
        //!
        void unpack_and_add_message (char *buf);
        //! Process XML snippet and do recognized commands
        void unpack_and_process_xml_snippet (char *buf);
	//! Unpack a list of subdirectories and insert them in
	//! a SearchPathDialog
	void unpack_subdir_list( char * buf );
	//! Unpack a search path and send it to a SearchPathDialog
	void unpack_search_path( char * buf );
	//! Inherited virtual function that is executed automatically
	//! whenever the Application has no pending events to process.
	//! It calls get_pending_data().
	void timerEvent( QTimerEvent * );

        /* MS/START - dynamic module loading */
        void unpack_and_countDynamicModules( char * buf );
        void unpack_and_recordDynamicModules( char * buf );
        void unpack_and_queryDynamicModules( char * buf , int fd );
        /* MS/END - dynamic module loading */

	TGProgramState * programState;
private:
	int socket_in;	
	bool auto_reading;
	UIManager * um;
	int timer_id;

        /* MS/START - dynamic module loading */
        int number_of_modules;
        char **module_table;
        char **module_table_s;
       /* MS/END - dynamic module loading */
};

#endif // GUI_SOCKET_SENDER_H
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

