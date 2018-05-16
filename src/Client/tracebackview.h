//! \file tracebackview.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** Widget TracebackView, which builds a source traceback viewer window on
** top of a TreeView window.
**
** Created by John Gyllenhaal 11/06/03
**
*****************************************************************************/
#ifndef TRACEBACKVIEW_H
#define TRACEBACKVIEW_H

#include "qvbox.h"
#include "qgroupbox.h"
#include "qcombobox.h"
#include <qfont.h>
//#include "treeview.h"
#include "cellgrid.h"
#include "cellgrid_searcher.h"
#include <qstring.h>
#include <qwidget.h>
#include "uimanager.h"
#include <qstatusbar.h>
#include <qtabbar.h>
#include <qtabwidget.h>

//! Source code traceback viewer 

//! Builds a source traceback viewer window on top of a TreeVeiw window to
//! support click back to source for messages
class TracebackView : public QVBox
{
    Q_OBJECT

public:

    TracebackView(UIManager *m,
		  CellGridSearcher * grid_searcher,
		  bool showTracebackTitle,
		  QWidget *parent=NULL, 
		  const char *name=NULL);

    ~TracebackView();

    //! Set the traceback to display and level in the traceback to show
    //! (level -1, the default, picks the highest priority level, 
    //!  and in the case of ties, picks the lowest level number (i.e., the 
    //!  top level).
    //! traceback should be of the form 
    //! "<level0Func:line_no0>func_name0\n<level1Func:line_no1>func_name1\n..."
    void setTraceback (const QString &traceback, int level = -1);

private slots:

    //! Handle selection of a new traceback source level to display
    void tracebackIndexSelectedHandler (int levelIndex);

    //! Update the display width so all text is visible
    void updateTracebackDisplayWidth();

    //! Catch changes to the trackbackView display window size
    void tracebackViewResized( QResizeEvent *e );

    //! Catch changes to the search path and redisplay the traceback
    void reloadSource();

    //! Catch requests to change the cellgrid font
    void resetFont( QFont & f);


private:

    // Calculate the priority of this site location
    double calcLevelPriority (double basePriority, const QString &file,
			      int line, const QString &desc);

    //! Returns the number of levels specified by the traceback string
    int getLevelCount ();

    //! Returns the location info for 'index' in the traceback string 
    //! (via parameters) and TRUE if info for 'index' exists, FALSE otherwise.
    //! Note: With tracebackTitle specifiers and multiple tracebacks in a 
    //! string that 'index' is typically not the same as 'tracebackLevel',
    //! so made 'tracebackLevel' a return parameter and renamed variables.
    //! To support informational messages, now return maxTracebackLevel also.
    bool getTracebackInfo (int index, QString &fileName, int &lineNo, 
			   QString &funcName, int &tracebackLevel,
			   int &tracebackId, int &maxTracebackId,
			   int &maxTracebackLevel, QString &tracebackTitle,
			   bool &titleSpecifier);

    //! Display the indicated file and lineNo, highlighting the source line
    void showFileLine (const char *fileName, int lineNo);

    QStatusBar *status;
    UIManager *um;

    // Should we show the traceback title (TRUE), 
    // or will the parent do this (FALSE), typically for tabbed view
    bool showTitle;

    // Holds the unparsed traceback string specified by setTraceback
    QString tracebackString;
    QString programName;
//    TreeView *tracebackView;
    CellGrid *tracebackView;
    CellGridSearcher * cgs;
    QGroupBox *tracebackGroupBox;
    QComboBox *tracebackList;
    QTabBar *tracebackTabBar;
    QTabWidget *tracebackTabWidget;

    //! Selected index in the traceback tree, -1 if none selected
    int tracebackIndex;

    //! TracebackBase is either 0 or 1.  If 1, the title of the traceback
    //! has been hidden and tracebackIndex needs to be adjusted.
    int tracebackBase;

    //! Style to display selected line with
    CellStyleId selectedLineStyle;
    
    //! File and line currently being displayed
    QString displayedFileName;
    int displayedLineNo;

    //! Maximum width of all the source currently displayed
    int maxSourceWidth;

    //! recordId of the longest source line
    int maxSourceId;

    //! Strlen of the longest source line
    int maxSourceLen;
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

