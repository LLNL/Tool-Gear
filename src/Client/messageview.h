//! \file messageview.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** Widget MessageView, which builds a Message viewer window on
** top of a CellGrid window.
**
** Created by John Gyllenhaal 11/04/03
**
*****************************************************************************/
#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include "qvbox.h"
#include "qgroupbox.h"
#include "qcombobox.h"
#include "cellgrid.h"
#include "cellgrid_searcher.h"
#include <qstring.h>
#include <qwidget.h>
#include "uimanager.h"
#include <qpopupmenu.h>
#include <qstatusbar.h>
#include <qtimer.h>
#include <qlabel.h>
#include <qfont.h>
#include "inttoindex.h"
#include "intset.h"
#include "inttable.h"

//! A message viewer 

//! Builds a message viewer window on top of a CellGrid window and
//! notifies listener of messages that are clicked on
class MessageView : public QVBox
{
    Q_OBJECT

public:

    MessageView(const char *progName, UIManager *m,
	     CellGridSearcher * grid_searcher, QWidget *parent=NULL, 
	     const char *name=NULL, // QStatusBar *statusBar=NULL,
	     bool viewingSnapshot = FALSE);

    ~MessageView();

    //! Selects a default message, if one not already selected
    //! Used by messageViewer->customEvent() to selected the
    //! first message if the user hasn't already selected one
    void selectDefaultMessage();

signals:

    //! Signals when a messageFolder is selected by the user
    void messageFolderSelected (const char *messageFolderTag,
				const char *messageFolderTitle);


    //! Signals when a message is selected by the user.
    //! Note: messageText and messageTraceback may be NULL if user
    //! switched to a folder with no messages (so 'none' are now selected)
    void messageSelected (const char *messageFolderTag,
			  const char *messageText,
			  const char *messageTraceback);
			  
   
private slots:
    
    //! Handle declarations of new message Folders
    void messageFolderDeclaredHandler (const char *messageFolderTag,
				       const char *messageFolderTitle,
				       UIManager::PolicyIfEmpty ifEmpty);

    //! Handle selection of a new message folder to display
    void messageFolderSelectedHandler (int folderIndex);
    
    //! Handle new messages added to a folder
    void messageAddedHandler (const char *messageFolderTag, 
			      const char *messageText,
			      const char *messageTraceback);

    //! Handle clicks on messages
    void messageClickedHandler (int recordId, int attrId, ButtonState,
				int, int, int, int, int, int);

    //! Handle double clicks on messages
    void messageDoubleClickedHandler (int recordId, int attrId, ButtonState,
				      int, int, int, int, int, int);

    //! Handles clicks on the tree hierachy control area
    void treeClickHandler (int recordId, int attrId, ButtonState buttonState,
                           int indentLevel, int parentRecordId, bool treeOpen,
                           int x, int y, int w, int h);

    //! Handles double clicks on the tree hierachy control area
    void treeDoubleClickHandler (int recordId, int attrId,
				 ButtonState buttonState,
				 int indentLevel, int parentRecordId,
				 bool treeOpen, int x, int y, 
				 int w, int h);

    //! Flushes all message updates pending (added by messageAddedHandler())
    void flushMessageUpdates ();

    //! Update the display width so all text is visible
    void updateMessageDisplayWidth();

    //! Catch changes to the messageGrid display window size
    void messageGridResized( QResizeEvent *e );

    //! Catch message window movements so we can record possitions
    void contentsMovingHandler (int x, int y);

    //! Catch requests to change the cellgrid font
    void resetFont( QFont & f);

    //! Catch requests to change label font
    void labelFontChanged( QFont & f);

private:

    //! Update message folder title (either create it or update it)
    void updateMessageFolderTitle (const char *messageFolderTag);

    //! Updates the message display for a message folder
    void updateMessageDisplay (const QString &folderTag);

    //! Returns selected line out of a multiline string (using newlines)
    //! where lineNo == 0 returns the first line.  Returns TRUE if lineNo
    //! exists in string, FALSE otherwise
    bool getIndexedLine (const QString &src, int lineNo, QString &dest);
    QStatusBar *status;
    UIManager *um;
    QString programName;
    CellGrid *messageGrid;
    QGroupBox *messageGroupBox;
    QComboBox *messageFolder;

    //! Currently selected message folder, -1 if none selected
    int selectedMessageFolder;

    //! Style to display selected message with
    CellStyleId selectedMessageStyle;

    //! Style to display message headers with
    CellStyleId headerStyle;

    //! Maps recordIds to messageIndexes
    IntToIndex recordId2messageIndex;

    //! Maps messageIndexes to recordIds (of message header)
    IntToIndex messageIndex2recordId;

    //! Maps comboBoxIndexes to folderIndexes 
    IntToIndex comboBoxIndex2folderIndex;

    //! Maps folderIndexes to comboBoxIndexes
    IntToIndex folderIndex2comboBoxIndex;

    //! Which messageFolder is currently displayed
    QString displayedMessageFolder;

    //! How many messages for displayedMessageFolder are currently displayed
    int displayedMessageCount;

    //! How many lines does the above messages take to display
    int displayedLineCount;

    //! RecordId of the longest message header
    int maxHeaderId;

    //! Strlen of the longest message header
    int maxHeaderLen;

    //! RecordId of the longest message body line
    int maxBodyId;

    //! Strlen of the longest message body line
    int maxBodyLen;

    //! Max width of the current displayed message lines
    int displayedMaxWidth;

    //! Indicate messages have been added since last flush
    bool messagesAdded;

    //! Initially assume don't need to popup messageFolder list
    //! (Set to TRUE when user selects disabled title, so they can try again)
    bool popupFolderSelection;

    //! Holds the last displayed count for each message folder
    IntToIndex displayedCount;

    //! Update screen timer (combine screen updates for scalability);
    //! use one timer for all instances of this class
    static QTimer *updateTimer;

    //! Holds the messageFolder-specific state
    struct MessageFolderState
    {
	//! Current top,left pixel location for this messageFolder
	int 	curX;
	int	curY;

	//! Current set of open messages for this messageFolder
	IntSet 	openMessages;

	//! Currently selected message in message folder, -1 if none selected
	int selectedMessage;  

	//! Currently selected recordId of message, -1 if none selected
	int selectedRecordId;

	//! Flags that the messageFolder just displayed and the view should
	//! be refreshed (move contents, set message, etc.
	bool refreshView;

	MessageFolderState() : curX(0), curY(0), openMessages("Open Messages"),
			       selectedMessage(-1), selectedRecordId(-1),
			       refreshView(FALSE)
	    {}
    };

    //! Holds each message folder's state
    IntTable<MessageFolderState> messageFolderState;

    //! Holds the displayed folder's state
    MessageFolderState *displayedFolderState;
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

