//! \file mainview.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** MainView is a primary GUI interface to the UIManager, providing menus
** and high-level window management for TG UIManager viewers.
**
** Currently uses TreeView to dynamically parse and instrument executables.
** Designed by John May and John Gyllenhaal at LLNL
** Based on RawViewer & DynViewer implemented by John Gyllenhaal and John May.
** Initial version created from older viewers by John Gyllenhaal 12/19/01
**
*****************************************************************************/
#ifndef MAINVIEW_H
#define MAINVIEW_H

#include "qmainwindow.h"
#include "qmenubar.h"
#include "qpopupmenu.h"
#include "qpushbutton.h"
#include "qtoolbutton.h"
#include "uimanager.h"
#include "treeview.h"
#include "qstring.h"
#include "l_punt.h"
#include "gui_action_sender.h"
#include "gui_socket_reader.h"
#include "cellgrid_searcher.h"

//! Main GUI interface for a UI Manager.
/*! Shows raw state of UI Manager when the viewer is created, allows user 
// to dynamically parse and instrument serial/parallel executables and
// get back results from instrumentation.
//
// Inherits from QMainWindow so we can wrap some nice things around TreeView
// and connect to UI manager, etc. */
class MainView : public QMainWindow
{
    Q_OBJECT

public:
    //! Creates main interface for viewing for the passed UIManager
    MainView (UIManager *m, CellGridSearcher * grid_searcher,
	      GUIActionSender *send=NULL, GUISocketReader * receive=NULL,
	      QWidget *parent=NULL,
	      const char *name=NULL, bool beVerbose=FALSE,
    	      bool snapshotView=FALSE, bool staticdata=FALSE);

    //! Removes all memory used by MainViewer
    ~MainView ();

public slots:

    //! Catch requests to update the program status message
    void handleProgramMessage(const char *);

    //! Catch changes in whether the program is loaded or not
    void handleProgramActive(bool);

    //! Catch changes in whether the program is executing
    void handleProgramRunning(bool);

    //! Allow changes to the main window title (called caption in qt)
    void changeWindowCaption(const char *newCaption);

private slots:
    //! Catch menu request to spawn a new viewer window of the same contents
    void menuCloneViewer();

    //! Catch menu request to save snapshot to file
    void menuWriteSnapshot();

    //! Catch menu request to save text of snapshot to file
    void menuWriteTextSnapshot();

    //! Catch menu request to read in a snapshot and display window
    void menuViewSnapshot();

    //! Catch menu request to compare snapshot contents to current contents
    void menuCompareSnapshot();

    //! Catch menu request to uncompare snapshot contents to current contents
    void menuUncompareSnapshot();

    //! Catch menu request to instrument code automatically
    void menuCreateInstrumentationRequest();

    //! Catch menu request to define a search path
    void setSearchPath();

    //! Display a dialog to change the font used in the main displays
    void changeFont();

    //! Display a dialog to change the font used in the labels and menus
    void changeLabelFont();

    //! Set the current font as the default
    void setDefaultFont();

    //! Set the label current font as the default
    void setDefaultLabelFont();

    //! Catch menu request 'about' program
    void menuAbout();

protected:
    //! Create a copy of the central widget, for use when cloning this view
    QWidget * cloneCentralWidget( const char * progName, UIManager * m, 
		    QWidget * parent, const char * name,
		    bool viewingSnapshot) const;

private:

    // UIManager to view
    UIManager *um;

    // UIManager to compare results against
    UIManager *diffUm;


    // Single TreeView window for now
    TreeView *view;

    //! Menu bar object
    QMenuBar *mainMenu;
    //! Popup for the file menu
    QPopupMenu *fileMenu;
    //! Popup for the edit menu
    QPopupMenu *editMenu;
    //! Popup menu for font operations
    QPopupMenu *fontMenu;
    //! Popup for the actions menu
    QPopupMenu *actionsMenu;

    //! Add menu for Brock (may cause coredump)
    QPopupMenu *chartMenu;
    
    //! Popup for the help menu
    QPopupMenu *helpMenu;

    //! Window name, may be NULL
    char *winName;
    int winId;

    //! Keeps track of all CellGrids in the app
    CellGridSearcher * cgs;

    //! Job control object, if not NULL
    GUIActionSender *sender;
    GUISocketReader *receiver;

    //! Start stop button for sender (NULL otherwise)
    QToolButton *startStopButton;

    //! Status message for sender (NULL otherwise)
    QLabel *statusMessage;

    //! Whether data is static (read from a file by collector)
    //! or dynanmic (gathered from a live program)
    bool dataIsStatic;

    //! Static number to uniquely identify viewer windows
    static int windowId;

    //! Keep track of Run button and program message so
    //! we can propagate info to new windows
    static enum buttonState { DISABLED, STOPPED, RUNNING } runButtonState;
    static QString programMessage;
};

#endif











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

