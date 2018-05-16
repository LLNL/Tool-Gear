// chartview.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
#include "chartview.h"

// Static number to uniquely identify viewer windows
// \TODO Should this be different from MainView?
int ChartView::windowId = 1;

// Creates interface for viewing for the passed UIManager in a chart
ChartView::ChartView(UIManager *m, GUIActionSender *send,
  GUISocketReader * receive, QWidget *parent,
  const char *name, bool beVerbose)
{
  // Disable all window updates until viewer is created
  setUpdatesEnabled( FALSE );

  // Get the id for this window, and increment it
  winId = windowId;
  windowId++;

  // Initialize pointer at UIManager that viewer will interact with
  um = m;

  // Initially, don't compare to another snapshot
//  diffUm = NULL;


  // Create caption from name, if provided
  QString caption;
  if (name != NULL)
  {
    winName = strdup (name);
  }
  // Otherwise, use default name
  else
  {
    winName = strdup ("Data Viewer");
  }
  caption.sprintf ("%i: %s", winId, winName);
  setCaption(caption);

  // Make menu tool bar
  mainMenu = new QMenuBar (this);
  fileMenu = new QPopupMenu;
  fileMenu->insertItem ("New Chart View", this, SLOT(menuCloneViewer()),
    CTRL+Key_N);
  fileMenu->insertSeparator();
  fileMenu->insertItem ("Quit", qApp, SLOT(closeAllWindows()), CTRL+Key_Q);

  mainMenu->insertItem ("&File", fileMenu);

  // Save job-control object (may be NULL)
  sender = send;
  receiver = receive;
  
  // Create ChartView window, must use 'this' as parent (thanks John May!)
  chart = new TgChart_dev(this, name);

  // Make central widget for main window
  setCentralWidget(chart);

  // For now, by default create a 1000x500 window
  resize(1000, 500);

  // Install our data update handlers
  connect(um, SIGNAL(intSet (char *, char *, char *, int , int, int)),
    this, SLOT(intSet (char *, char *, char *, int , int, int)));
  connect(um, SIGNAL(doubleSet (char *, char *, char *, int , int, double)),
    this, SLOT(doubleSet (char *, char *, char *, int , int, double)));

  // Make main window visible
  show();
}


// Destory the ChartView
ChartView::~ChartView()
{
  // Delete the chart widget
  delete chart;
}


// Catch menu request to spawn a new viewer window of the same contents
void ChartView::menuCloneViewer()
{
    // Create new viewer for same um
    // Use same job control object 'sender' and same name
    // We may also want to make this use the same TgChartData in the future.
    /*ChartView *newViewer = */new ChartView (um, sender, receiver, NULL, winName);
}


// Catch data changes and update the chart
void ChartView::intSet (char *funcName, char *entryKey, char *dataAttrTag, 
			int taskId, int threadId, int value)
{
  printf("IntSet\n  funcName: %s\n  entryKey: %s\n  dataAttrTag: %s\n",
    funcName, entryKey, dataAttrTag);
  printf("  taskID: %i\n  threadID: %i\n  value: %i\n",
    taskId, threadId, value);
}

// Catch data changes and update the chart
void ChartView::doubleSet (char *funcName, char *entryKey, char *dataAttrTag, 
			   int taskId, int threadId, double value)
{
  printf("DoubleSet\n  funcName: %s\n  entryKey: %s\n  dataAttrTag: %s\n",
    funcName, entryKey, dataAttrTag);
  printf("  taskID: %i\n  threadID: %i\n  value: %f\n",
    taskId, threadId, value);

  if(strcmp(dataAttrTag,"l1util")==0)
  {
    printf("Adding point...\n");
    chart->appendPoint(0,value);
  }

}

// Include the QT specific code that is automatically
// generated from chartview.h
//#include "chartview.moc"

// End of chartview.cpp

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

