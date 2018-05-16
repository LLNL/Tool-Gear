// messsageviewer.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** Widget MessageViewer, which builds a Message and Source viewer window on
** top of a CellGrid window with a TreeView window to view source.
**
** Created by John Gyllenhaal 11/04/03
**
*****************************************************************************/

#include "messageviewer.h"
#include <stdio.h>
#include <stdlib.h>
#include <qmainwindow.h>
#include "tg_time.h"

MessageViewer::MessageViewer(const char *progName, UIManager *m, 
			 CellGridSearcher * grid_searcher,
			 QWidget *parent,  const char *name, 
			 // QStatusBar *statusBar,
			 bool viewingSnapshot) :
    QSplitter (parent, name),
    
    // If not NULL, display extra information about contents (for now)
//    status (statusBar),
      // Assume top-level parent is a QMainWindow
// No longer use status bar in message viewer -JCG
//      status( ((QMainWindow *)(topLevelWidget()))->statusBar() ),

    // Get the UIManager pointer
    um (m),

    // Get the program name
    programName (progName)
    
{
    // Create smallest grid, with all the attrIds we are going to get
    // Set min row height to 15, to make room for internal pixmaps
    messageView = new MessageView (progName, m, grid_searcher, this,
		    "Message window", viewingSnapshot);
    TG_checkAlloc(messageView);


    // Create source traceback viewer below message window
    // Have TracebackView display the traceback title (e.g., self contained)
    tracebackView = new TabTracebackView (m, grid_searcher, 
					  this, "Source Traceback");
    TG_checkAlloc(tracebackView);


    // Try a vertical split between the windows
    setOrientation(Qt::Vertical);

    // For now, by default create a 1000x500 window
    resize(1000,500);

    messageView->resize(1000, 250);
    tracebackView->resize(1000,250);

    // Listen for message selected events and update traceback veiw as 
    // appropriate
    connect (messageView, 
	     SIGNAL(messageSelected(const char *, const char *, const char *)),
	     this,
	     SLOT(messageSelectedHandler(const char *, const char *, const char *)));

    // Set size policy so that the treeview will try to expand
    // to fill all available space (versus grabbing half of it)
    setSizePolicy (QSizePolicy(QSizePolicy::Expanding, 
			       QSizePolicy::Expanding));
}

// Removes all memory used by MessageViewer
MessageViewer::~MessageViewer ()
{
	// No need to delete child widgets
}

// Handle custom events, mainly select default message event
void MessageViewer::customEvent (QCustomEvent *e)
{

    switch (e->type())
    {
	// Handle new message folder event
      case 1101:
#if 0
      {
	  // DEBUG
	  const char *winName = name();
	  if (winName != NULL)
	  {
	      TG_timestamp ("MessageViewer::customEvent(%s): selectDefaultMessage()\n", 
			    winName, e->type());
	  }
	  else
	  {
	      TG_timestamp ("MessageViewer::customEvent: selectDefaultMessage()\n", e->type());
	  }
      }
#endif

      // Select the default message, if user has not already selected one
      messageView->selectDefaultMessage();
      break;
      
      default:
	fprintf (stderr, 
		 "MessageViewer::customEvent: Warning unknown event type %i\n",
		 e->type());
	break;
    }
}


// Handle message selected events by displaying source
void MessageViewer::messageSelectedHandler(const char * /*messageFolderTag*/,
					   const char * /*messageTag*/,
					   const char *messageTraceback)
{
    // Point the traceback window at the messageTraceback location, if exists
    if (messageTraceback != NULL)
	tracebackView->setTraceback (messageTraceback);

    // Otherwise, if traceback doesn't exist, put up message
    else
	tracebackView->setTraceback ("(No traceback location specified)");
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

