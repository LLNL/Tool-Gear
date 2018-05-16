/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 10 February 2004
//! \file search_path_dialog.cpp

#include <iostream>
#include <stdio.h>
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qclipboard.h>
#include <qheader.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qstring.h>
#include "search_path_dialog.h"
#include "dir_view_item.h"
#include "path_view_item.h"
using namespace std;

SearchPathDialog:: SearchPathDialog( QWidget * parent, const char * name,
		bool modal, WFlags fl )
	: SearchPathDialogBase( parent, name, modal, fl )
{

	// Create the columns in the list views
	allDirList->addColumn( "All directories" );
	allDirList->setShowSortIndicator( TRUE);
	allDirList->setMainTextColumn( 0 );
	
	searchDirList->addColumn( "Recursive search?" );
	searchDirList->addColumn( "Search directories" );

	searchDirList->setAllColumnsShowFocus( TRUE );
	searchDirList->setMainTextColumn( 1 );

	// Listen for signals from the list views
	connect( allDirList, SIGNAL( dragListEntered(
					QDragEnterEvent *, DragListView *) ),
		this, SLOT( checkProposedDrop(
					QDragEnterEvent *, DragListView *) ) );
	connect( searchDirList, SIGNAL( dragListEntered(
					QDragEnterEvent *, DragListView *) ),
		this, SLOT( checkProposedDrop(
					QDragEnterEvent *, DragListView *) ) );
	connect( allDirList, SIGNAL( dragListReceivedDrop(
					QDropEvent *, DragListView *) ),
		this, SLOT( handleDrop( QDropEvent *, DragListView *) ) );
	connect( searchDirList, SIGNAL( dragListReceivedDrop(
					QDropEvent *, DragListView *) ),
		this, SLOT( handleDrop( QDropEvent *, DragListView *) ) );

	// Show hierarchical views
	allDirList->setRootIsDecorated( TRUE );

	// Disable sorting on the search path list so we can
	// choose the order we want.
	searchDirList->setSorting(-1);

	// Allow self-drop (i.e., reordering) only on search path
	searchDirList->setSelfDropOK( TRUE );
}

SearchPathDialog:: ~SearchPathDialog()
{ }

void SearchPathDialog:: populateDirectoryList( QString parent_path,
		QString dirs, void * parent_node )
{
	// We handle all incoming lists of directories here; see
	// if it's a top-level list or a lower-level list.
	
	if( parent_node == this ) {	// top level
		// Clear out any existing names
		allDirList->clear();

		// Put the full parent path in the text box
		editBasePath->setText( parent_path );

		DirViewItem * item = new DirViewItem( this, allDirList,
				parent_path );
		item->addChildren( dirs );
		item->setOpen( TRUE );
		item->setDragEnabled( TRUE );
	} else {	// Adding subdirectories to a child
		((DirViewItem *)parent_node)->addChildren( dirs );
	}
}
	
void SearchPathDialog:: populateSearchPath( QString path )
{
	searchDirList->clear();

	QListViewItem * after = 0;

	QStringList dirNames = QStringList::split( QChar(':'), path );
	QStringList::Iterator it;
       	for( it = dirNames.begin(); it != dirNames.end(); ++it ) {
		// See if the * recursion symbol is at the start
		// of this string; if so, note it and move to the
		// next character.  We'll do this C-style, since
		// it's quicker than doing all the QString operations.
		const char * p = ((QString)(*it)).latin1();
		bool recur;
		if( *p == '*' ) {
			recur = TRUE;
			++p;
		} else {
			recur = FALSE;
		}

		PathViewItem * item = new PathViewItem( searchDirList,
				after, p );
		item->setDragEnabled( TRUE );
		item->setOn( recur );
		after = item;
	}
}

void SearchPathDialog:: addNames()
{
	addNames( searchDirList->lastItem() );
}
	
void SearchPathDialog:: addNames( QListViewItem * after )
{
	// Get the list of names that the user selected,
	// add them to the directory path list, and disable
	// them (but don't remove them) in the full path
	// list.

	QListViewItemIterator it( allDirList, QListViewItemIterator::Selected );
	QListViewItem * selItem;

	while( (selItem = it.current()) != 0 ) {
		++it;
		PathViewItem * item = new PathViewItem( searchDirList,
				after, selItem->text(
					allDirList->mainTextColumn() ) );
		item->setDragEnabled( TRUE );
		selItem->setSelectable( FALSE );
		selItem->setDragEnabled( FALSE );
		selItem->repaint();	// changing Selectability changes color

		after = item;	// Next item goes after this new one
	}

	// Reset all the selection
	allDirList->setSelectionMode( QListView::NoSelection );
	allDirList->setSelectionMode( QListView::Multi );
}

void SearchPathDialog:: removeNames()
{
	// Get the list of names that the user selected,
	// reenable them in the full path list (if they
	// exist; if not there's something wrong), and
	// then remove them from the directory path list.
	
	QListViewItemIterator it( searchDirList,
			QListViewItemIterator::Selected );
	QListViewItem * selItem;

	int allDirListCol = allDirList->mainTextColumn();
	int searchDirListCol = searchDirList->mainTextColumn();
	while( (selItem = it.current()) != 0 ) {
		++it;
		QListViewItem * matchItem
			= allDirList->findItem( selItem->text(
					searchDirListCol ), allDirListCol );
		if( matchItem != 0 ) {
			// If this item was in the list, reenable it
			// for dragging and selecting
			matchItem->setSelectable( TRUE );
			matchItem->setDragEnabled( TRUE );
			matchItem->repaint();
		} else {
			// Not an error because name may have
			// been pasted in from outside
		}
		delete selItem;	// also removes item from its list view
	}
}

void SearchPathDialog:: checkProposedDrop( QDragEnterEvent * e,
		DragListView * target )
{
	// For the full directory list, only accept drops from
	// the search directory list (not from the outside world);
	// for the search directory list, we can accept drops from
	// the outside world.  In any case, we only accept text.
	
	if( QTextDrag::canDecode( e ) &&
			( target == searchDirList || 
			  ( target == allDirList
			    && e->source() == searchDirList )) ) {
		e->accept();
	}
}

void SearchPathDialog:: handleDrop( QDropEvent * e, DragListView * target )
{
	// Drops in the list views are handled differently depending on
	// their source and target.  But in any event, we have to
	// decode the text and make a list of strings.
	
	if( target == searchDirList ) {
		handleSearchDirDrop( e );
	} else if( target == allDirList ) {
		// Drops can only come from the search dir list,
		// which means this is just like clicking the
		// Remove button; otherwise, something's messed up.
		if( e->source() == searchDirList ) {
			removeNames();
			e->accept();
		} else {
			e->ignore();	
		}
	} else {
		qWarning( "We shouldn't be able to drop on that widget!" );
		e->ignore();
	}
}

void SearchPathDialog:: handleSearchDirDrop( QDropEvent * e )
{
	// This function is designed only for drops on the search
	// directory list.  Drops can come from the full list or the
	// outside world.  Drops from elsewhere in this list should
	// be handled by the class itself. 
	
	QString text;
	if( !QTextDrag::decode( e, text ) ) {
		qWarning( "Got a drop we couldn't decode!" );
		e->ignore();
		return;
	}

	QStringList names = QStringList::split( QChar('\n'), text );
	
	// Figure out where the drop should go; after will be 0
	// if there's no specific item, and will put everything at
	// the end of the list.
	PathViewItem * after = (PathViewItem *)searchDirList->itemAt( 
				searchDirList->contentsToViewport( e->pos() ) );

	if( e->source() == 0 || e->source() == editBasePath ) {
		// From outside or from edit box; add items
		for( QStringList::Iterator it = names.begin();
				it != names.end(); ++it ) {
			// Add after item dropped on, or at end
			PathViewItem * item;
			if( after != 0 ) {
				item = new PathViewItem( searchDirList,
						after, *it );
			} else {
				item = new PathViewItem( searchDirList, *it );
			}
			item->setDragEnabled( TRUE );
		}
		e->accept();
	} else if( e->source() == allDirList ) {
		// Easy! Just like clicking add.
		if( after ) addNames(after);
		else addNames();
		e->accept();
	} else if ( e->source() == searchDirList ) {
		qWarning( "Self-move should be handled elsewhere!");
		e->ignore();
	} else {
		qWarning("Huh?  Where did this drop come from?");
		e->ignore();
	}
}

QString SearchPathDialog:: searchPath()
{
	QListViewItemIterator it( searchDirList );
	PathViewItem * item;
	QString path;

	while( ( item = (PathViewItem *)(it.current()) ) != 0 ) {
		++it;

		// Add * for checked items (indicates that they
		// should be searched recursively)
		if( item->isOn() ) {
			path.append('*');
		}

		QString dirname = item->text( SELECT_TEXT );

		// Filter out . and .. annotations
		if( dirname.contains( "(current directory)" ) ) {
			path.append( "." );
		} else if( dirname.contains( "(parent directory)" ) ) {
			path.append( ".." );
		} else {
			// Add the directory name as it appears in the list
			path.append( dirname );
		}

		// Add separator, unless we're at the end
		if( it.current() != 0 ) {
			path.append(':');
		}
	}

	return path;
}

SearchPathDialog::SaveState SearchPathDialog::saveState()
{
	// Enumerate values correspond to button position,
	// including -1 = no selection, so just return the
	// selected id from the button group
	return (SaveState)( groupApply->selectedId() );
}

void SearchPathDialog:: resetDirectory()
{
	dirViewNeedsChildren( editBasePath->text(), this, this );
}

void SearchPathDialog:: getDirChildren( QString fullname, DirViewItem * dir )
{
	emit dirViewNeedsChildren( fullname, this, dir );
}

void SearchPathDialog:: showHelp()
{
	// We're borrowing this slot for now to display the
	// final search path; later this should pop up a
	// help message.
	
	QMessageBox::information( this, "Search path help",
			"Setting the search path:\n\n"
			"Click items in left box to select for path.\n"
			"Click \"Add to path\" to add them to search path\n"
			"in right box.  Items can also be dragged across,\n"
			"dragged from other windows on screen, or cut and\n"
			"pasted.\n\n"
			"Click to left of items left box to see\n"
			"subdirectories.\n\n"
			"Items in right box can be dragged to change order.\n"
			"Remove items from search path by selecting them\n"
			"and clicking \"Remove from path\" or by dragging\n"
			"them back to left box.\n\n"
			"Checking the box next to a directory in the\n"
			"search path will cause all its subdirectories\n"
			"to be searched as well.\n\n"
			"To save the selected path (on the remote machine\n"
			"if applicable) click \"...and save in current\n"
			"directory\" to use this path only when browsing\n"
			"in the current directory or \"...and save in\n"
			"home directory\" to use whenever no path data\n"
			"is saved in the current directory.  Path data\n"
			"is stored in a file called \"SourcePath\".\n\n"
			"Click \"OK\" when finished or \"Cancel\"." );

}

void SearchPathDialog:: paste()
{
	// See if there's valid text in the clipboard, and create a
	// new item in the search path if there is.
	QString text;
	if( QTextDrag::decode( QApplication::clipboard()->data(), text )
			&& !text.isEmpty() ) {
		PathViewItem * item = new PathViewItem( searchDirList, text );
			item->setDragEnabled( TRUE );
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

