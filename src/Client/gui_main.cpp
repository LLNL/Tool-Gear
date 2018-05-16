// gui_main.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 20 July 2001

#include <iostream>
#include <new>
#include <stdio.h>
#include <stdlib.h>

#include <qapplication.h>
#include <qobject.h>
#include <qsettings.h>
#include <qstring.h>
#include <qptrvector.h>

#include "uimanager.h"
#include "cellgrid_searcher.h"
#include "tg_collector.h"
#include "tg_parse_opts.h"
#include "tg_program_state.h"
#include "gui_socket_reader.h"
#include "gui_action_sender.h"
#include "command_tags.h"
#include "tg_time.h"
#include "treeview.h"
#include "mainview.h"
#include "messageviewer.h"
#include "l_alloc_new.h"
#include <unistd.h>  // for sleep
#include <stdlib.h>  // for atexit
using namespace std;


#define TITLE_SIZE 200
#define MAX_HOST_NAME 257
#define TG_LOCAL_COLLECTOR_PATH "TGdpcl"
#define TG_REMOTE_COLLECTOR_PATH "TGdpcl"

// Used to find QSettings
const QString APP_KEY = "/Tool Gear/";

// Handle out of memory conditions with new (without (nothrow)) somewhat
// gracefully (See Item 7 of Effective C++)
void noMoreMemory()
{
    // First, uninstall this handler in case something below trys
    // to allocate memory
    set_new_handler(NULL);

    // print out error message
    fprintf (stderr, "Tool Gear: Out of memory, exiting...\n");
    
    // I don't want to coredump, just exit with -1 return value
    exit (-1);
}

// Flag that we are terminating normally
static bool normal_termination = FALSE;

// Handle any cleanup at exit we need, intially what is needed
// is a delay at exit so all the ssh-tunnelled messages make it
// to the GUI from the collector and then to flush them out before
// existing, out so users actually see the error message.
void exit_cleanup(void)
{
    // If we are not terminating normally, sleep 1 second before
    // exit to allow ssh to forward the collector stderr output
    // and then flush it to the screen
    if (!normal_termination)
    {
	// Tell user what we are doing
	fflush (stdout);
	fprintf (stderr,
                 "(Tool Gear Client flushing collector output before "
		 "abnormal termination...)\n");
        fflush (stderr);
	
	// Wait for ssh to forward collector output
	sleep (1);

	// Print all pending collector output to screen
	clearCollectorOutput();

	// Make sure it is really printed out
	fflush (stdout);
        fflush (stderr);
    }
}

int main( int argc, char * argv[] )
{
    // Register our at_exit cleanup routine
    atexit (exit_cleanup);

    // Set up a out of memory handler for the GUI client (see above)
    set_new_handler(noMoreMemory);

    // Test of memory handler
//    int *big = new int[1000000000];

    // To facilate memory debugging tools, turn off internal alloc
    // routines for symbol tables
#ifdef TG_BYPASS_ALLOC_ROUTINES
    bypass_alloc_routines = 1;
#endif
    
#ifdef TG_TIMINGS    
    TG_timestamp ("GUI thread: Thread started\n");
#endif
    QApplication a( argc, argv );
    
#ifndef LOCAL_ONLY
    QString verString;
    verString.sprintf ("Tool Gear %4.2f", TG_VERSION);
    UIManager um( &a, verString );	// shared by all windows
#else
    UIManager um( &a, "snapshot001.ss", "snapshot001.ss" );	// shared by all windows
#endif
    
    QString remoteHost, loginName;  // initialized to null
    QString connectPort;
    QString remoteDir;
    QString collectorProg;

    QPtrVector<QString> collargs;

    char ** remoteArgs;             // will point into argv
    int remoteCount;                // number of values in remoteArgs
    bool setParallel = FALSE;
    bool heartbeat = TRUE;		// use periodic heartbeat and timeout
    // to shut down collector if client
    // becomes quiet
    
#ifndef LOCAL_ONLY
    // Get the options; leaves remoteHost and loginName null if user
    // didn't specify them
    TGParseOpts( argc, argv, remoteHost, loginName, connectPort, remoteDir,
		 collectorProg, remoteCount, remoteArgs, 
		 setParallel, heartbeat, collargs);
    
    QString p;
    if( collectorProg.isNull() ) {
	p = ( remoteHost.isNull() ) ? TG_LOCAL_COLLECTOR_PATH :
	    TG_REMOTE_COLLECTOR_PATH;
    } else {
	p = collectorProg;
    }
    
    // Start the collector (with an empty list of target args, since
    // we aren't starting the target program just yet)
    TGCollector * collector
	    = new TGCollector( &a, p, collargs, remoteHost, loginName, 
			       connectPort );
    TG_checkAlloc(collector);
    
    
    // Lets the collector object process the connection request from
    // the collector program
    while( collector->getStatus() == TGCollector::Launching ) {
	a.processOneEvent();
    }
    
    if( collector->getStatus() != TGCollector::Connected ) {
	cout << "Connection failed; state is " <<
	    (int)(collector->getStatus()) << endl;
	return 0;
    }
    
    // When neeed for debugging, gives user time to start the
    // debugger on the remote side and attach to the collector
    // printf("Hit a key to continue\n");
    // getchar();
    
    // Socket reader and sender (if it exists) share access to 
    // programState
    TGProgramState programState;	// initially Stopped & 0 procs
    GUISocketReader gsr( collector->getSocket(), &um, &programState );

    QObject::connect( &gsr, SIGNAL( readerSocketClosed() ), 
		    collector, SLOT( handleClosedSocket() ) );
    
    um.setRemoteSocket( collector->getSocket() );
    GUIActionSender * p_sender =
	new GUIActionSender( collector->getSocket(), &um,
			     &programState, heartbeat );
    
    // Change remote directory and run the remote application,
    // then request parse
    if( !remoteDir.isNull() ) {
	p_sender->changeDirectory( remoteDir.latin1() );
	int result = gsr.check_socket();	// check the response
	if( result == DPCL_SAYS_QUIT ) {
		delete collector;
		return 0;
	}
    }
    
#if 0
    // Now get the program name and build its arg list
    // remoteCount doesn't include the last NULL in argv
    QString argList = remoteArgs[1]; // Assumes last arg is NULL
    int i;
    for( i = 2; i < remoteCount; i++ ) {
	argList += ' ';
	argList += remoteArgs[i];
    }
    p_sender->initializeApp( targetProgram.latin1(),
			     remoteCount - 1, argList.latin1(),
			     setParallel );
#endif
    // Create a sequence of \0 terminated strings
//    QString argList;
//    int i;
//    for( i = 0; i < remoteCount; ++i ) {
//        argList += remoteArgs[i];
//	argList += '\0';
//    }
    
    /* MS/START ASSUMED ADDED BY JCG */
#if 0
    p_sender->initializeApp( remoteCount, remoteArgs,
			     setParallel );
#endif
    /* MS/START ASSUMED ADDED BY JCG */
    
#ifdef TG_TIMINGS    
    TG_timestamp ("GUI thread: Starting database population\n");
#endif
    
    // Populate the um database before starting the viewer
    // FIX -- this is a quasi-event-loop.  We should really
    // use Qt's own loop.  The problem is that we want to
    // delay creating the viewer until we know that the we've
    // received all the information about it from the Collector
    // (such as columns).  This occurs (for now) when
    // GUI_CREATE_VIEWER is sent.  It would be better not
    // to wait here but to proceed to the main loop and
    // create the viewer whenever the Collector is ready.
    // Then the code to create the viewer would need access to
    // all the data that's used below, such as title, sender,
    // and socket reader.  Also, since we're not processing
    // the heartbeat timer here, the Collector will not get
    // a message until it sends a create viewer request.
    int result;
    while( (result = gsr.check_socket()) != DPCL_SAYS_QUIT
	   && !(IS_A_VIEW_TYPE( result ) ) ) {
    }
    
    if( result == DPCL_SAYS_QUIT ) {
	delete collector;
	return 0;
    }

    /* MS/START ASSUMED ADDED BY JCG */
    p_sender->initializeApp( remoteCount, remoteArgs,
			     setParallel );
    /* MS/END ASSUMED ADDED BY JCG */

#ifdef TG_TIMINGS    
    TG_timestamp ("GUI thread: Ending database population\n");
#endif
    
    // Put the socket reader on automatic
    gsr.enable_auto_read( TRUE );
    
    // Create the first window
//    char title[TITLE_SIZE];
//    const char * host;
    QString title;
    // Figure out a good title for the window.  First try the first
    // arg passed to the collector (usually a program name or a file name);
    // if that's empty, use the name of the collector.
//    const char * target;
    if( remoteArgs[0] == NULL ) {
	title = collectorProg;
    } else {
	title = remoteArgs[0];
    }

//    if( targetProgram.isNull() ) {
//	target = collectorProg.latin1();
 //   } else {
//	target = targetProgram.latin1();
 //   }
    title += " on ";

    if( remoteHost.isNull() ) {
        char hostname[MAX_HOST_NAME];
	gethostname( hostname, MAX_HOST_NAME );
	title += hostname;
    } else {
	title += remoteHost;
    }
    
    if( IS_A_DYNAMIC_VIEW_TYPE(result) ) {
	title += " (Live)";
    }
    
//    snprintf( title, TITLE_SIZE, "%s on %s%s",
//	      target, host,
//	      ( IS_A_DYNAMIC_VIEW_TYPE(result) ? " (Live)" : "") );
    
    // See if there is a preferred label font; if not, Qt should
    // set a reasonable default
    QSettings settings;
    settings.setPath( "llnl.gov", "Tool Gear", QSettings::User );
    QString labelFontString = settings.readEntry( APP_KEY + "LabelFont" );
    if( labelFontString != QString::null ) {
	    QFont labelFont;
	    labelFont.fromString( labelFontString );
	    a.setFont( labelFont, TRUE );
    }

    // Create a CellGridSearcher to keep track of all cell
    // grids and coordinate searches on them.
    CellGridSearcher * cgs = new CellGridSearcher;

// Define TWO_MAINS to get two MainViewers.  Exposes some
// reordering issues with events (no longer seen)
#undef TWO_MAINS
    
    
#endif
#ifndef LOCAL_ONLY
    MainView * view2;
#if 0
    MessageViewer *mview = NULL;
    // Default is dynamic collector that support program control
    if( result == GUI_CREATE_VIEWER ) {
	    view2 = new MainView (&um, p_sender, &gsr, NULL, title);
    } else {	// Static (post-mortem) collector with no program control
	    view2 = new MainView (&um, p_sender, &gsr, NULL, title,
			    			FALSE, FALSE, TRUE);
	    view2->handleProgramMessage( "Reading data" );

	    mview = new MessageViewer ("Message Viewer", &um, NULL, "Message Viewer name");
    }
#endif
    // Create the outer shell of the viewer
    if( IS_A_DYNAMIC_VIEW_TYPE( result ) ) {
	    view2 = new MainView( &um, cgs, p_sender, &gsr, NULL,
			    title.latin1() );
    } else {
	    view2 = new MainView( &um, cgs, p_sender, &gsr, NULL,
			    title.latin1(), FALSE, FALSE, TRUE );
    }
    TG_checkAlloc(view2);


    // Now add the requested view as the central widget
    QWidget * w;
    if( IS_A_TREE_VIEW( result ) ) {
	    w = new TreeView( title.latin1(), &um, cgs, view2, title.latin1(),
			    FALSE );
	    TG_checkAlloc( w );
    } else if( IS_A_MESSAGE_VIEW( result ) ) {
	    w = new MessageViewer( title.latin1(), &um, cgs, view2,
			    title.latin1(), FALSE );
	    TG_checkAlloc( w );
    } else {
	    cerr << "Internal error: request for unknown view type "
		    << result << endl;
	    return -1;
    }

    if( w != NULL )
	    view2->setCentralWidget( w );

    
#else
    TreeView *tree = new TreeView ("snapshot", 
				   &um, NULL, "snapshot");
    TG_checkAlloc(tree);
    
#endif
#ifdef TWO_MAINS
    // DEBUG, testing placement in closed trees
    MainView *view3 = new MainView (&um, cgs, p_sender, &gsr, NULL,
		    title.latin1());
    TG_checkAlloc(view3);
#endif
    
    
#ifndef LOCAL_ONLY
#ifdef TG_TIMINGS    
    TG_timestamp ("GUI thread: Finished creation of TreeView\n");
#endif
    
    // Exit main loop when last window closes
    QObject::connect (&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
    
    // No need for this; quitting on lastWindowClosed does the
    // same thing
//    a.setMainWidget (view2);

    // Show message viewer if it exists
    // Superceded by putting message view in a main viewer
//    if (mview != NULL)
//	mview->show();

#endif
    view2->show();
#ifdef  TWO_MAINS
    view3->show();
#endif
#ifdef TG_TIMINGS    
    TG_timestamp ("GUI thread: Entering GUI event loop\n");
#endif
    
    a.exec();
    
#ifndef LOCAL_ONLY
    
#ifdef TG_TIMINGS    
    TG_timestamp ("GUI thread: Exiting GUI event loop\n");
#endif
    
    // Lets the collector manage further socket messages directly
    // during the delete collector statement below (needs sole control)
    gsr.enable_auto_read(FALSE);
    
    // tells the collector process to shut down
    delete collector;
    delete p_sender;
    
    // DEBUG, delete viewer.  Really don't want to free
    // things when exiting, since just wastes time and exposes
    // bugs unnecessarily.
#if 0
    delete view;
#endif
#endif

    // Flag that we are terminating normally
    normal_termination = TRUE;

    // Print all pending collector output to screen
    clearCollectorOutput();
    
    // Make sure it is really printed out
    fflush (stdout);
    fflush (stderr);
    
    return 0;
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

