//! \file search_path_dialog.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 10 February 2004

#ifndef SEARCH_PATH_DIALOG_H
#define SEARCH_PATH_DIALOG_H

#include <qlistview.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qstringlist.h>
#include "drag_list_view.h"
#include "search_path_dialog_base.h"

class DirViewItem;

//! Dialog for setting a search path for application source files.

//! This is not an automatically generated class.  It uses the
//! SearchPathDialogBase class that's generated by Qt designer
//! and extends it to add the necessary functionality.
//! This dialog lets the user select a search path on the
//! Collector file system (which may be different from the
//! local file system).
class SearchPathDialog : public SearchPathDialogBase {
	Q_OBJECT
public:
	//! Create a dialog to let the user select from a list
	//! of directories which ones should be in a search
	//! path, what order they should be in, and whether
	//! subdirecrtories should be searched as well.
	SearchPathDialog( QWidget * parent = 0, const char * name = 0,
			bool modal = FALSE, WFlags fl = 0 );
	~SearchPathDialog();

	//! Returns a single string containing the chosen search
	//! path, with directory names separated by colons in
	//! the standard Unix form, except that directory names
	//! preceded with an asterisk (*) should have all their
	//! subdirectories searched recursively.
	QString searchPath();
	
	//! Enumerates possible choices for saving the search path
	enum SaveState {
		NoSelection = -1, NoSave, SaveCurrentDir, SaveHomeDir
	};

	//! Returns the user's choice of whether and where to save
	//! the search path.
	SaveState saveState();

	//! Populate a the full directory list.  parent is the full
	//! path to the directory being populated.  dirs is a 
	//! colon-separated list of full directory path names. 
	//! If the void * object is this, then we put the directory
	//! list at the top level; otherwise, we treat the object as
	//! a DirViewItem entry and the list as its children.
	void populateDirectoryList( QString parent, QString dirs, void * );
	
	//! Populate the search path list.  path is a colon-separated
	//! list of full path names.  The current path in the view
	//! is wiped out.
	void populateSearchPath( QString path );

	//! Get subdirectory names for the DirViewItem dir.  This is not
	//! normally called by user code; instead, it's here to relay
	//! the request from the item to the outside world.
	void getDirChildren( QString fullname, DirViewItem * dir );
	
public slots:
	//! Determine whether a list view can handle an drop
	//! (depends on the source, target, and type of drop.)
	virtual void checkProposedDrop( QDragEnterEvent * e,
			DragListView * target );

	//! Accept an item in the search directory window
	virtual void paste();
	
signals:
	//! A request to get a list of directories in the specified path
	void dirViewNeedsChildren( QString parentDir, SearchPathDialog *,
			void * );

protected slots:

	//! Called when the user wants to move the selected directories
	//! to the search path list.  Items are placed after the
	//! specified entry (if this is called as a regular function)
	//! or at then end (if after == 0).
	virtual void addNames();

	//! Called when the user wants to remove selected items from
	//! the search path list.
	virtual void removeNames();

	//! Display a help message.
	virtual void showHelp();

	//! Handle an incoming drop event; this is done differently
	//! for the two list views.
	virtual void handleDrop( QDropEvent * e, DragListView * target );
	
	//! Handle drops specifcially on a list view item (in the
	//! search directory list)
	virtual void handleSearchDirDrop( QDropEvent * e );
	
	//! Replace the list of directories in the full list with a
	//! new list based at the path given in the string.  This slot
	//! will issue a request to get the list of directories and
	//! then return.  Another function will handle populating the
	//! list once the data arrives.
	virtual void resetDirectory();

protected:
	//! Get the initial list of directory names and put them
	//! in the display.
	QStringList getNameList();

	//! Add names after a specific entry in the list (called
	//! by the slot of the same name)
	virtual void addNames( QListViewItem * after );
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
