//! \file tabtracebackview.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** Widget TabTracebackView, which is a Tabbed wrapper around TracebackView that
** creates a separate Tab for each traceback specified
**
** Created by John Gyllenhaal 8/20/04
**
*****************************************************************************/
#include "tabtracebackview.h"
#include <ctype.h> // For isspace() on tru64

TabTracebackView::TabTracebackView(UIManager *m, 
				   CellGridSearcher * grid_searcher,
				   QWidget *parent,  const char *name) :

    // For now, child of vertical geometry management widget
    QVBox (parent, name),

    // Save values for creating TracebackViews
    um (m),
    gridSearcher(grid_searcher),

    // Table of tracebackView widgets indexed by ints.  
    // Don't automatically delete widgets, since we make widgets child
    // of this widget, they will be automatically deleted.
    tracebackViewTable ("tracebackViewTable", NoDealloc, 0)
{

    tabWidget = new QTabWidget (this, "Tab widget");

    // Create traceback view that doesn't show the title
    TracebackView *tracebackView = 
	new TracebackView (um, gridSearcher, FALSE, this, "Traceback");

    // Put initial traceback at index 1
    tracebackViewTable.addEntry (1, tracebackView);
    
    // Add a default tab, for now
    tabWidget->addTab(tracebackView, "Traceback");
}

TabTracebackView::~TabTracebackView()
{
    // No need to delete children of this widget
    // Do not need to clean up tracebackViewTab
}

// Returns the max traceback Id (0 if only one traceback in string)
int TabTracebackView::maxTracebackId (const QString &tracebackString)
{
    // If the tracebackString is Empty or NULL, return false now
    if (tracebackString.isEmpty())
	return (-1);

    // Get the start of the traceback string
    const char *startPtr = tracebackString.latin1();

    // Skip all leading whitespace except '\n'
    while ((*startPtr != 0) && (*startPtr != '\n') &&
	   isspace (*startPtr))
	startPtr++;
    
    // If at the end of the string, return FALSE now
    if (*startPtr == 0)
	return (-1);

    // Initially assume 1 traceback in string
    int maxTracebackId = 0;

    // Increment max id for each title specifier (^) encountered after
    // a newline (white space allowed) in the rest of the traceback string.   
    while (*startPtr != 0)
    {
	// Skip all characters until '\n' or end of string
	while ((*startPtr != 0) && (*startPtr != '\n'))
		startPtr++;

	// If at newline, goto next character
	if (*startPtr == '\n')
	    startPtr++;

	// Skip all leading whitespace except '\n'
	while ((*startPtr != 0) && (*startPtr != '\n') &&
	       isspace (*startPtr))
	    startPtr++;

	// If have title specifier, increment maxTracebackId
	if (*startPtr == '^')
	    maxTracebackId++;
    }

    // Return the number of title specifiers
    return (maxTracebackId);
}


// Return the single traceback at 'traceId' (traceId's start at 1)
QString TabTracebackView::getSingleTraceback (const QString &origTraceback,
					      int traceId)
{
    // If the origTraceback is Empty or NULL or traceId clearly invalid,
    // return " " now
    if (origTraceback.isEmpty() || (traceId < 1))
	return (" ");

    // Get the start of the traceback string
    const char *startPtr = origTraceback.latin1();

    // Skip all leading whitespace except '\n' to be consistent
    // with the normal traceback parser
    while ((*startPtr != 0) && (*startPtr != '\n') &&
	   isspace (*startPtr))
	startPtr++;

    // Currently at id 1 (beginning of traceback)
    int atId = 1;

    // Skip tracebacks until atId == traceId
    while ((atId < traceId) && (*startPtr != 0))
    {
	// Skip the current traceback and then increment atId
	
	
	// Skip all characters until '\n' or end of string
	while ((*startPtr != 0) && (*startPtr != '\n'))
		startPtr++;

	// If at newline, goto next character
	if (*startPtr == '\n')
	    startPtr++;

	// Skip all leading whitespace except '\n'
	while ((*startPtr != 0) && (*startPtr != '\n') &&
	       isspace (*startPtr))
	    startPtr++;

	// If have title specifier, increment atId
	if (*startPtr == '^')
	    atId++;
    }

    // If nothing left of traceback, return " "
    if (*startPtr == 0)
	return (" ");

    // Clear tbuf to hold the single traceback
    tbuf.clear();

    // Copy over buffer until hit newline followed by '^'
    while((*startPtr != 0))
    {
	// Stop when hit newline followed by '^'
	if ((startPtr[0] == '\n') && (startPtr[1] == '^'))
	    break;

	// Append this character to tbuf
	tbuf.appendChar(*startPtr);
	startPtr++;
    }

    // Return the single traceback
    return (tbuf.contents());
}

// Returns the title of the first traceback or "Traceback" if none specified
QString TabTracebackView::getTitle (const QString &traceback)
{
    // If the traceback is Empty or NULL or traceId clearly invalid,
    // return "Traceback" now
    if (traceback.isEmpty())
	return ("Traceback");

    // Get the start of the traceback string
    const char *startPtr = traceback.latin1();

    // Skip all leading whitespace except '\n' to be consistent
    // with the normal traceback parser
    while ((*startPtr != 0) && (*startPtr != '\n') &&
	   isspace (*startPtr))
	startPtr++;

    // If next character is '^', have title specification
    if (*startPtr == '^')
    {
	// Skip ^ and copy rest of line for title
	startPtr++;

	// Clear tbuf to hold title
	tbuf.clear();

	// Copy over title
	while((*startPtr != 0) && (*startPtr != '\n'))
	{
	    tbuf.appendChar(*startPtr);
	    startPtr++;
	}

	// Return title (in tbuf)
	return (tbuf.contents());
    }

    // Otherwise, return "Traceback" as default title
    else
    {
	// Return default title
	return ("Traceback");
    }
}

void TabTracebackView::setTraceback (const QString &traceback, int  /*index*/)
{
    // Get number of tracebacks to display (add 1 since max traceback id
    // is 0 based);
    int tracebackCount = maxTracebackId (traceback) + 1;

    // If there is no traceback at all, make one tab to can put up notice
    if (tracebackCount < 1)
	tracebackCount = 1;

    // Get the current number of tabs displayed
    int tabCount = tabWidget->count();

    // If necessary, remove traceback tabs
    for (int id = tabCount; id > tracebackCount; id--)
    {
	// Get existing tracebackView, it must exist
	TracebackView *tracebackView = tracebackViewTable.findEntry (id);
	if (tracebackView == NULL)
	    TG_error ("Error: expected tracebackView to exist for id %i!", id);

	// Remove the tab for this widget
	tabWidget->removePage (tracebackView);

	// Update tabCount
	tabCount--;
    }

    // Set title existing tabs to new title
    // Don't actually set traceback yet, makes tab update appear slow
    for (int id = 1; id <= tabCount; id++)
    {
	// Get the traceback widget for tab id
	TracebackView *tracebackView = tracebackViewTable.findEntry (id);

	// Get traceback for just this id
	QString singleTraceback = getSingleTraceback (traceback, id);

	// Get the title of the traceback
	QString title = getTitle (singleTraceback);

	// Update tab title, if necessary
	if (title != tabWidget->tabLabel(tracebackView))
	    tabWidget->changeTab(tracebackView, title);
    }

    // If necessary, add traceback tabs and fill in titles
    // Don't actually set traceback yet, makes tab update appear slow
    for (int id = tabCount+1; id <= tracebackCount; id++)
    {
	// Get existing tracebackView, if exists
	TracebackView *tracebackView = tracebackViewTable.findEntry (id);

	// Create dummy name for traceback

	// Create new tracebackView if it doesn't exist
	if (tracebackView == NULL)
	{
	    QString name;
	    name.sprintf ("Traceback %i", id);

	    tracebackView = 
		new TracebackView (um, gridSearcher, FALSE, this, 
				   name.latin1());

	    // Store in talbe for later use
	    tracebackViewTable.addEntry(id, tracebackView);
	}

	// Get traceback for just this id
	QString singleTraceback = getSingleTraceback (traceback, id);

	// Get the title of the traceback
	QString title = getTitle (singleTraceback);

	// Add tab for traceback with traceback title
	tabWidget->addTab(tracebackView, title);
    }

    // Now actually set tracebacks for each tab
    // Hopefully this will make the tab update appear much snapper even
    // though this actually causes a little more work to be done here
    for (int id = 1; id <= tracebackCount; id++)
    {
	// Get the traceback widget for tab id
	TracebackView *tracebackView = tracebackViewTable.findEntry (id);

	// Get traceback for just this id
	QString singleTraceback = getSingleTraceback (traceback, id);

	// Set the traceback for it to the highest priority one
	tracebackView->setTraceback (singleTraceback);
    }
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

