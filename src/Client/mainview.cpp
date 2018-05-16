// mainview.cpp
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

#include "command_tags.h"
#include "mainview.h"
#include "treeview.h"
#include "messageviewer.h"
#include "search_dialog.h"
#include "search_path_dialog.h"
#include "inst_dialog.h"
#include "qapplication.h"
#include "qmessagebox.h"
#include "qfiledialog.h"
#include "qfont.h"
#include "qfontdialog.h"
#include "qtoolbar.h"
#include "qlabel.h"
#include "qpushbutton.h"
#include "qsettings.h"
#include "qstatusbar.h"
#include "qstringlist.h"
#include "qwhatsthis.h"
#include "qtoolbutton.h"
#include "qlineedit.h"
#include "tg_time.h"
#include "qcursor.h"
#include <stdio.h>
#include <stdlib.h>

// Should put this in a global place, or maybe uimanager.h
const QString APP_KEY = "/Tool Gear/";

// Static number to uniquely identify viewer windows
int MainView::windowId = 1;

// Static data keeps track of run button and program message
MainView::buttonState MainView::runButtonState = DISABLED;
QString MainView::programMessage("Starting program; please wait");

// Creates main interface for viewing for the passed UIManager
MainView::MainView (UIManager *m, CellGridSearcher * grid_searcher,
	       	    GUIActionSender *send, GUISocketReader * receive,
		    QWidget */*parent*/, const char *name, bool /*beVerbose*/,
		    bool /*snapshotView*/, bool staticdata)
	: QMainWindow( 0, "MainView", WDestructiveClose )
{
    // Disable all window updates until viewer is created
    setUpdatesEnabled( FALSE );

    // Get the id for this window, and increment it
    winId = windowId;
    windowId++;

    // Initialize pointer at UIManager that viewer will interact with
    um = m;

    // Initialize pointer at CellGridSearcher so any contained here
    // can register themselves and this view can initiate a search
    cgs = grid_searcher;

    // Initially, don't compare to another snapshot
    diffUm = NULL;

    // Record whether we are viewing static data
    dataIsStatic = staticdata;

    // Create caption from UIManager caption, if set
    
    // Create caption from name, if provided
    QString caption;
    
    QString uicaption = um->getWindowCaption();
    if (uicaption != NULL_QSTRING)
    {
	winName = strdup (uicaption.latin1());
    }
    // Otherwise, use filename pased in
    else if (name != NULL)
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

    // Use apps status bar for treeView's status for now
    // -- now just set this up in treeView ctor JMM
    //QStatusBar *status = statusBar();

    // Create treeView window, must use 'this' as parent (thanks John May!)
    // If snapshotView == TRUE, will not allow/track state modifications
//    view = new TreeView (winName, m, this, name, status, snapshotView);
//    TG_checkAlloc(view);

    // Make central widget for main window
//    setCentralWidget (view);

    // Make menu tool bar
    mainMenu = new QMenuBar (this);
    TG_checkAlloc(mainMenu);
    fileMenu = new QPopupMenu;
    TG_checkAlloc(fileMenu);
    int item_no;

    fileMenu->insertItem ("New Window", this, SLOT(menuCloneViewer()),
			  CTRL+Key_N);

    // For now, snapshots doesn't make sense for message viewers
    if (!staticdata)
    {
	fileMenu->insertItem ("Write Snapshot", this, SLOT(menuWriteSnapshot()));
	fileMenu->insertItem ("View Snapshot", this, SLOT(menuViewSnapshot()));
	fileMenu->insertItem ("Compare to Snapshot", this, 
			      SLOT(menuCompareSnapshot()));
	fileMenu->insertItem ("Write Text Snapshot", this, 
			      SLOT(menuWriteTextSnapshot()));
    }
    fileMenu->insertSeparator();
    fileMenu->insertItem ("Close Window", this, SLOT(close()), CTRL+Key_W);
    fileMenu->insertItem ("Quit", qApp, SLOT(closeAllWindows()), CTRL+Key_Q);

    editMenu = new QPopupMenu;
    TG_checkAlloc(editMenu);
    editMenu->insertItem ("Find...", cgs, SLOT(searchForText()), CTRL+Key_F);
    editMenu->insertItem ("Find again", cgs, SLOT(searchForTextAgain()),
			  CTRL+Key_G);
    editMenu->insertItem ("Set search path...", this, SLOT(setSearchPath()),
		    	  CTRL+Key_P);

    fontMenu = new QPopupMenu;
    TG_checkAlloc(fontMenu);
    item_no = fontMenu->insertItem ("Change display font...", this,
		    SLOT( changeFont() ), CTRL+Key_T);
    fontMenu->setWhatsThis( item_no, "Sets the font for program data and "
		    "source code displays." );
    item_no = fontMenu->insertItem ("Set current display font as default", this,
		    	  SLOT( setDefaultFont() ));	// No KB shortcut here
    fontMenu->setWhatsThis( item_no, "Makes the current font used for program "
		    "data and source code displays the default when you use "
		    "any Tool Gear tool." );
    item_no = fontMenu->insertItem ("Change label font...", this,
		    SLOT( changeLabelFont() ), CTRL+SHIFT+Key_T );
    fontMenu->setWhatsThis( item_no, "Sets the font for all text except "
		    "program data and source code displays." );
    item_no = fontMenu->insertItem ("Set current label font as default", this,
		    	  SLOT( setDefaultLabelFont() ));// No KB shortcut here
    fontMenu->setWhatsThis( item_no, "Makes the current font used for labels "
		    "the default when you use any Tool Gear tool." );

    if( !dataIsStatic ) {
	    actionsMenu = new QPopupMenu;
	    TG_checkAlloc(actionsMenu);
	    actionsMenu->insertItem ("Automatic Instrumentation...", this,
		    SLOT(menuCreateInstrumentationRequest()), CTRL+Key_I);
    }

    helpMenu = new QPopupMenu;
    TG_checkAlloc(helpMenu);
//    helpMenu->insertItem ("About " + um->getProgramName(),
    helpMenu->insertItem ("About This Tool" /*+ um->getProgramName()*/,
		    this, SLOT(menuAbout()));

    mainMenu->insertItem ("&File", fileMenu);
    mainMenu->insertItem ("&Edit", editMenu);
    mainMenu->insertItem ("Fon&t", fontMenu);
    if( ! dataIsStatic ) mainMenu->insertItem ("&Actions", actionsMenu);
    mainMenu->insertSeparator();
    mainMenu->insertItem ("&Help", helpMenu);

    // Save job-control object (may be NULL)
    sender = send;
    receiver = receive;


    // If have job-control object, install start/stop button
    if (sender != NULL )
    {
	if( !dataIsStatic ) {
	    // Create dpclTools only if runing dynTG -JCG 05/03/06
	    QToolBar * dpclTools = new QToolBar( "DPCL communications",
						 this, QMainWindow::Top );
	    TG_checkAlloc(dpclTools);

		// Use QToolButton to look more like What's this button -JCG 
		startStopButton = new QToolButton( dpclTools, "&Run");
		startStopButton->setText("&Run");
		TG_checkAlloc(startStopButton);
		startStopButton->setToggleButton( TRUE );

		// Set button state to match the current program state
		if( runButtonState == DISABLED )
			startStopButton->setEnabled( FALSE );
		else 
			startStopButton->setOn( runButtonState == RUNNING );
		
		// Use dpclTools only if runing dynTG -JCG 05/03/06
		// Create what's this button on tool bar
		QToolButton *whatsThisButton = QWhatsThis::whatsThisButton(dpclTools);
		// Create status message line on tool bar
		QLineEdit *statusMessage = new QLineEdit (dpclTools);
		statusMessage->setText ("Hit \"Run\" to start application");
		statusMessage->setReadOnly(TRUE);
		statusMessage->setMaxLength(50);

		const char * whatsThisText = "<p> Click the <b>Whats This> button "
		    "and then click "
		    "on a button, menu item, or clickable portion of the display "
		    "in order to get on-line help (if available) for the item "
		    "clicked on </p>";
		
		QWhatsThis::add(whatsThisButton, whatsThisText);
	}

	QToolBar * statusToolBar  = new QToolBar( "Status",
						  this, QMainWindow::Top );
	statusMessage = new QLabel (statusToolBar);
	statusMessage->setIndent(4);  // Put space before message

	// Try to get status bar to work with Qt 3.3.6 on IA64 -JCG 05/03/06
	statusToolBar->setStretchableWidget(statusMessage);

	const char *statusToolBarText = "<p> Status Bar.  Displays overall "
	    "state, typically whether the program is running or halted.</p>";
	QWhatsThis::add(statusToolBar, statusToolBarText);

	if( !dataIsStatic ) {
		const char *startStopText =
		    "<p> Clicking the <b>Run</b> button "
		    "either continues the execution of stopped applications or "
		    "halts the execution of running applications.  "
		    " The <b>Run</b> button is unavailable (greyed out) "
		    "during application creation and "
		    "after program execution has terminated.</p>";

		QWhatsThis::add(startStopButton, startStopText);

		// Connect the button to the program control and to
		// update the stored button state
		connect( startStopButton, SIGNAL(toggled(bool)),
			 sender, SLOT(startStopProgram(bool)) );
		//connect( startStopButton, SIGNAL(toggled(bool)),
		//	 this, SLOT(handleRunToggled(bool)) );

		// When the program becomes inactive, this signal is
		// emitted with a value of false; dim out the button
		//connect( receiver, SIGNAL(programActive(bool)),
		//		startStopButton, SLOT(setEnabled(bool)) );
		connect( receiver, SIGNAL(programActive(bool)),
				this, SLOT(handleProgramActive(bool)) );

		// When a breakpoint is hit, the programRunning signal
		// is emitted with a value of false; set the button so it
		// can be restarted.
		//connect( receiver, SIGNAL(programRunning(bool)),
		//		startStopButton, SLOT(setOn(bool)) );
		connect( receiver, SIGNAL(programRunning(bool)),
				this, SLOT(handleProgramRunning(bool)) );
		connect( sender, SIGNAL(programRunning(bool)),
				this, SLOT(handleProgramRunning(bool)) );
	}
	

	// Accept messages for the status line
	connect( receiver, SIGNAL(setProgramMessage(const char *)),
			this, SLOT(handleProgramMessage(const char *)));
	connect( sender, SIGNAL(setProgramMessage(const char *)),
			this, SLOT(handleProgramMessage(const char *)));

    }
    else
    {
	// Sanity, initialize sender only class variables to NULL
	startStopButton = NULL;
	statusMessage = NULL;
    }

    // Change window title if tool changes caption
    connect (um, SIGNAL (windowCaptionSet(const char *)),
	     this, SLOT (changeWindowCaption( const char *)));

    // Change window status if tool changes status
    connect (um, SIGNAL (toolStatusSet(const char *)),
	     this, SLOT(handleProgramMessage(const char *)));

    // Initialize the status message
    if( statusMessage ) {
	    statusMessage->setText( programMessage );
    }

    // For now, by default create a 1000x500 window
    resize(1000, 500);

    // Enable window updates and repaint it (required)
    setUpdatesEnabled( TRUE );
 
    // Don't make main window visible yet; wait for the contained
    // widget to insert itself
}

// Removes all memory used by MainViewer
MainView::~MainView ()
{
	// view should be deleted automatically when parent is deleted
//    delete view;
}



// Catch menu request to spawn a new viewer window of the same contents
void MainView::menuCloneViewer()
{
    // Create new viewer for same um
    // Use same job control object 'sender' and same name
    MainView *newViewer = new MainView (um, cgs, sender, receiver, NULL,
		    winName, FALSE, FALSE, dataIsStatic);
    TG_checkAlloc(newViewer);

    QWidget * w = cloneCentralWidget( winName, um, newViewer, winName, FALSE );
    newViewer->setCentralWidget( w );

    newViewer->show();
}

QWidget * MainView::cloneCentralWidget( const char * progName, UIManager * m,
		QWidget * parent, const char * name,
		bool viewingSnapshot ) const
{
	QWidget * w = centralWidget();
	const char * widgetType = w->className();
	QWidget * copy;

	// Run through the known possibilities and make
	// a copy if we recognize the type.  It would be
	// natural to do this with a virtual function
	// that was common to all the known types, but
	// that would require declaring them with a common
	// base class, and that, in turn, would require
	// multiple inheritance, which we want to avoid.
	if( strcmp( widgetType, "MessageViewer" ) == 0 ) {
		copy = new MessageViewer( progName, m, cgs, parent, name,
				viewingSnapshot );
		TG_checkAlloc( copy );
	} else if( strcmp( widgetType, "TreeView" ) == 0 ) {
		copy = new TreeView( progName, m, cgs, parent, name,
				viewingSnapshot );
		TG_checkAlloc( copy );
	} else {
		qWarning( "Internal error: MainView doesn't know how "
				"to copy a %s", widgetType );
		copy = NULL;
	}

	return copy;
}

// Catch menu request to save snapshot to file
void MainView::menuWriteSnapshot()
{
    // Scan for available file names of the form snapshot#.ss
    QString availName;
    bool isAvail=FALSE;
    FILE *in;
    
    // Count up, until find unused name
    for (int i = 1; i < 100; i++)
    {
	availName.sprintf ("snapshot%03i.ss", i);
	if ((in = fopen ((char *)availName.latin1(), "r")) == NULL)
	{
	    isAvail=TRUE;
	    break;
	}
	// Otherwise, close file and try again
	else
	{
	    fclose(in);
	}
    }
    // If no other name available, use default name
    if (!isAvail)
    {
	availName.sprintf ("snapshot.ss");
    }

    // Let user select file name
    QString fileName = QFileDialog::getSaveFileName (availName, 
						     "SnapShots (*.ss)",
						     this);

    // Save file if user actually selected name
    if (!fileName.isNull() )
    {
	FILE *out;
	// Open file, and put up message if cannot open it
	if ((out = fopen ((char *)fileName.latin1(), "w")) == NULL)
	{
	    QString message;
	    message.sprintf ("Unable to open '%s' for writing!",
			     (char *)fileName.latin1());
	    QMessageBox::information(this, "Save Snapshot",  message);
	}
	// Otherwise, write out snapshot
	else
	{
	    um->writeSnapshot(out);
	    fclose (out);
	}
    }
}

// Catch menu request to save text-form snapshot to file
void MainView::menuWriteTextSnapshot()
{
    // Scan for available file names of the form snapshot#.tss
    QString availName;
    bool isAvail=FALSE;
    FILE *in;
    
    // Count up, until find unused name
    for (int i = 1; i < 100; i++)
    {
	availName.sprintf ("snapshot%03i.tss", i);
	if ((in = fopen ((char *)availName.latin1(), "r")) == NULL)
	{
	    isAvail=TRUE;
	    break;
	}
	// Otherwise, close file and try again
	else
	{
	    fclose(in);
	}
    }
    // If no other name available, use default name
    if (!isAvail)
    {
	availName.sprintf ("snapshot.tss");
    }

    // Let user select file name
    QString fileName = QFileDialog::getSaveFileName (availName, 
						     "Text SnapShots (*.tss)",
						     this);

    // Save file if user actually selected name
    if (!fileName.isNull() )
    {
	FILE *out;
	// Open file, and put up message if cannot open it
	if ((out = fopen ((char *)fileName.latin1(), "w")) == NULL)
	{
	    QString message;
	    message.sprintf ("Unable to open '%s' for writing!",
			     (char *)fileName.latin1());
	    QMessageBox::information(this, "Save Text Snapshot",  message);
	}
	// Otherwise, write out text snapshot
	else
	{
	    um->printSnapshot(out);
	    fclose (out);
	}
    }
}

// Catch menu request to read in a snapshot and display window
void MainView::menuViewSnapshot()
{
    // Scan for used file names of the form snapshot#.ss
    QString usedName;
    bool isUsed=FALSE;
    FILE *in;
    
    // Scan down until find used name
    for (int i = 99; i > 0; i--)
    {
	usedName.sprintf ("./snapshot%03i.ss", i);
	char * fname = (char *)usedName.latin1();
	if ((in = fopen (fname, "r")) != NULL)
	{
	    isUsed=TRUE;
	    fclose(in);
	    break;
	}
    }

    // If no names used, use default name
    if (!isUsed)
    {
	usedName.sprintf ("./snapshot.ss");
    }

    // Let user select file name
    QString fileName = QFileDialog::getOpenFileName (usedName, 
						     "SnapShots (*.ss)",
						     this);
    // Open file if user actually selected name
    if (!fileName.isNull() )
    {
	// Make sure file is readable
	if ((in = fopen ((char *)fileName.latin1(), "r")) == NULL)
	{
	    QString message;
	    message.sprintf ("Unable to open '%s' for reading!",
			     (char *)fileName.latin1());
	    QMessageBox::information(this, "View Snapshot",  message);
	}
	// Otherwise, create new UIManager from file and spawn viewer
	else
	{
	    // Clean up read test
	    fclose (in);

	    char * newTitle = (char *)fileName.latin1();

	    // Read in file to new UIManager
	    UIManager *newUM = new UIManager(qApp, newTitle, newTitle );
	    TG_checkAlloc(newUM);

	    newUM->setRemoteSocket( um->getRemoteSocket() );

	    // Spawn new viewer for this newUM
	    // No job-control available on snapsnots, set snapshotView=TRUE
	    // Still pass sender to allow source fetching
	    MainView *newViewer = new MainView (newUM, cgs, sender, receiver,
			    NULL, newTitle, FALSE, TRUE, TRUE);
	    TG_checkAlloc(newViewer);

	    QWidget * w = cloneCentralWidget( newTitle, newUM, newViewer,
			    newTitle, TRUE );
	    newViewer->setCentralWidget( w );

	    newViewer->show();
	}
    }
}

// Menu id for uncompare menu item
#define MENU_UNCOMPARE_ID 100

// Catch menu request to compare snapshot contents to current contents
void MainView::menuCompareSnapshot()
{
    // First see if the central widget supports comparison
    QWidget * w = centralWidget();

    // Run through the known possibilities and punt if
    // the type is not known to support comparison.
    // As with the clone function above, it would be
    // natural to do this with a virtual function
    // that was common to all the known types, but
    // that would require declaring them with a common
    // base class, and that, in turn, would require
    // multiple inheritance, which we want to avoid.
    if( ! w->inherits( "TreeView" ) ) {	// Only possibility so far
        QString message;
        message.sprintf ("%s does not support comparison with snapshots",
			w->className() );
        QMessageBox::information(this, "Compare Snapshot",  message);
    	return;
    }

    // Set the type of the widget here so the rest of
    // the code will compile
    TreeView * view = (TreeView *)w;

    // Scan for used file names of the form snapshot#.ss
    QString usedName;
    bool isUsed=FALSE;
    FILE *in;
    
    // Scan down until find used name
    for (int i = 99; i > 0; i--)
    {
	usedName.sprintf ("snapshot%03i.ss", i);
	if ((in = fopen ((char *)usedName.latin1(), "r")) != NULL)
	{
	    isUsed=TRUE;
	    fclose(in);
	    break;
	}
    }

    // If no names used, use default name
    if (!isUsed)
    {
	usedName.sprintf ("snapshot.ss");
    }

    // Let user select file name
    QString fileName = QFileDialog::getOpenFileName (usedName, 
						     "SnapShots (*.ss)",
						     this);
    // Open file if user actually selected name
    if (!fileName.isNull() )
    {
	// Make sure file is readable
	if ((in = fopen ((char *)fileName.latin1(), "r")) == NULL)
	{
	    QString message;
	    message.sprintf ("Unable to open '%s' for reading!",
			     (char *)fileName.latin1());
	    QMessageBox::information(this, "Compare Snapshot",  message);
	}
	// Otherwise, create new UIManager from file and spawn viewer
	else
	{
	    // Clean up read test
	    fclose (in);

	    // If already comparing to a UIManager, delete it first
	    if (diffUm != NULL)
	    {
		delete diffUm;
		diffUm = NULL;
	    }
	    // Otherwise, add menu item to undo comparison
	    else
	    {
		// Add menu item for undoing comparison
		fileMenu->insertItem ("Undo Comparison", this, 
				      SLOT(menuUncompareSnapshot()), 
				      0, MENU_UNCOMPARE_ID, 4);
	    }
	    
	    // Read in file to new UIManager to compare to
	    diffUm = new UIManager(NULL, (char *)fileName.latin1(),
				   (char *)fileName.latin1());
	    TG_checkAlloc(diffUm);

	    // Tell the view to compare to this UIManager's value
	    view->compareTo (diffUm);

	    // Update caption, so show comparison
	    QString caption;
	    caption.sprintf ("%i: %s compared to %s", winId, winName, 
			    (char *)fileName.latin1());
	    setCaption(caption);
	}
    }


}

// Catch menu request to uncompare snapshot contents to current contents
void MainView::menuUncompareSnapshot()
{
    // Do nothing, if not comparing to snapshot
    if (diffUm == NULL)
	return;

    // Remove menu item to undo comparison
    fileMenu->removeItem (MENU_UNCOMPARE_ID);

    // Assume that if we got here, the view supports comparison.
    // For now, that means only TreeViews.  If other such views
    // are added, need to figure out here which one we have (using
    // the inherits() function, probably).
    // Tell the tree to not compare to anything
    TreeView * view = (TreeView *)(centralWidget());
    view->compareTo (NULL);

    // Delete the snapshot (must do after view->compareTo (NULL),
    // since it could still access diffUm)
    delete diffUm;
    diffUm = NULL;

    // Update caption, so show no comparison
    QString caption;
    caption.sprintf ("%i: %s", winId, winName);
    setCaption(caption);
}


// Catch menu request 'about' program
void MainView::menuAbout()
{
//    QString caption = "About " + um->getProgramName();
    // Pick something more generic for now -JCG
    QString caption = "About This Tool";
    QMessageBox::about( this, caption, um->getAboutText() );

}


// Slot to set the message text; keep track of the text in a
// static variable so we can use the same text in new windows.
void MainView::handleProgramMessage(const char * text)
{
    programMessage = text;
    statusMessage->setText( programMessage );
}

// Program became active or inactive (commenced or terminated)
void MainView::handleProgramActive( bool isActive )
{
	// When a program becomes active, it is initially STOPPED
	runButtonState = (isActive ? STOPPED : DISABLED );

	if( runButtonState == DISABLED )
		startStopButton->setEnabled( FALSE );
	else {
		startStopButton->setEnabled( TRUE );
		startStopButton->setOn( FALSE );
	}
}

// Program has stopped (because of a breakpoint) or started (why??)
// so we need to update the button accordingly
void MainView::handleProgramRunning( bool isRunning )
{
	runButtonState = (isRunning ? RUNNING : STOPPED );
	startStopButton->setOn( isRunning );
}

//! Allow changes to the main window title (called caption in qt)
void MainView::changeWindowCaption(const char *newCaption)
{
    QString updatedCaption;

    // Update window name
    if (winName != NULL)
    {
	free (winName);
    }
    winName = strdup (newCaption);
    updatedCaption.sprintf("%i: %s", winId, winName);
    setCaption(updatedCaption);
}

void MainView:: menuCreateInstrumentationRequest()
{
	InstrumentationDialog * id = new InstrumentationDialog( um );

	if( id->exec() == QDialog::Accepted ) {
		QPtrList<TGInstSpec> * isl = id->instrumentationList();

		sender->instrumentApp( *isl );
//		TGInstSpec * it;
//		for(it = isl->first(); it; it = isl->next() ) {
//			sender->instrumentApp( *it );
//		}
		delete isl;		// Frees all the entries
	}

	delete id;
}

void MainView:: setSearchPath()
{
	SearchPathDialog * sd = new SearchPathDialog(this);

	// Respond to requests for directory information
	connect( sd, SIGNAL( dirViewNeedsChildren(QString,
					SearchPathDialog *, void *) ),
			sender, SLOT( getDirList(QString, SearchPathDialog *,
					void *) ) );

	// Get the initial directory information
	sender->getDirList( ".", sd, sd );

	// Get the current search path
	sender->getSearchPath( sd );
	
	if( sd->exec() == QDialog::Accepted ) {
		// Get the final search path list and whether it
		// should be saved
		SearchPathDialog::SaveState save_request = sd->saveState();
		// Translate to global request name
		int save_path = 0;      // Avoid compiler warning
		switch( save_request ) {
		case SearchPathDialog::NoSelection:
		case SearchPathDialog::NoSave:
			save_path = COLLECTOR_NO_SAVE_PATH;
			break;
		case SearchPathDialog::SaveCurrentDir:
			save_path = COLLECTOR_SAVE_PATH_LOCAL;
			break;
		case SearchPathDialog::SaveHomeDir:
			save_path = COLLECTOR_SAVE_PATH_HOME;
			break;
		}

		sender->setSearchPath( sd->searchPath(), save_path );

		// Flush the file cache so new search path will be
		// used next time a file is loaded
		um->clearSourceCache();
	}
	
	delete sd;
}

void MainView:: changeFont()
{
	bool ok;

	// Need to work out a global default font, probably
	// using QSettings.
	QFont currentFont = um->getMainFont();
#if 0
	fprintf( stderr, "main font is now %s\n",
			currentFont.toString().latin1() );
#endif

	QFont newFont = QFontDialog::getFont( &ok, currentFont, this );

	if( ok ) {
		um->setMainFont( newFont );
	}
}

void MainView:: changeLabelFont()
{
	bool ok;

	QFont currentFont = qApp->font();

	QFont newFont = QFontDialog::getFont( &ok, currentFont, this );

	if( ok ) {
		qApp->setFont( newFont, TRUE );

		// Also notify any Tool Gear widgets that need to update
		// their label font manually
		um->setLabelFont (newFont);
	}
}

void MainView:: setDefaultFont()
{
	QSettings settings;
	settings.setPath( "llnl.gov", "Tool Gear",
			QSettings::User );
	settings.writeEntry( APP_KEY + "MainFont",
			um->getMainFont().toString() );
}

void MainView:: setDefaultLabelFont()
{
	QSettings settings;
	settings.setPath( "llnl.gov", "Tool Gear",
			QSettings::User );
	settings.writeEntry( APP_KEY + "LabelFont",
			qApp->font().toString() );
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

