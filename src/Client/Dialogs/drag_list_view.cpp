//! \file drag_list_view.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 11 February 2004

#include "drag_list_view.h"
#include "dir_view_item.h"
#include "path_view_item.h"
#include <stdio.h>

DragListView:: DragListView( QWidget * parent, const char * name )
	: QListView( parent, name ), selfDropPermitted( FALSE )
{ 
	// Need to accept drops both in the list view itself
	// and its viewport!
	setAcceptDrops( TRUE );
	viewport()->setAcceptDrops( TRUE );
	setSelectionMode( Multi );
}

QDragObject * DragListView:: dragObject()
{
	// Get a list of the selected items and put them in a
	// drag object.
	
	QListViewItemIterator it( this, QListViewItemIterator::Selected );

	QString names;
	QListViewItem * item;

	while( (item = it.current()) != 0 ) {
		++it;
		// Use special symbol SELECT_TEXT and virtual text
		// function that we've defined for the subclasses
		// to get the "important" text, regardless of what
		// column it's in
		names += item->text(SELECT_TEXT);
		// Separate the names with a newline
		if( it.current() != 0 ) {
			names.append( '\n' );
		}
	}

	return new QTextDrag( names, this );
}

void DragListView:: contentsDragEnterEvent( QDragEnterEvent * event )
{
	// If source of drag is local see if self-drops are OK.
	// If not local, emit a signal to find out if the drop is OK.
	if( event->source() == this ) {
		if( selfDropPermitted ) event->accept();
	}
	else emit dragListEntered( event, this );
}

void DragListView:: contentsDropEvent( QDropEvent * event )
{
	// First check to see if this is a self-drop (i.e.,
	// reordering items in the list) and if that's
	// permitted for this instance.  If so, handle that
	// within this class; otherwise (it's not a local drop)
	// emit a signal to whomever cares about those.
	if( event->source() == this ) {
		if( selfDropPermitted ) handleSelfDrop( event );
		else event->ignore();
	} else {
		emit dragListReceivedDrop( event, this );
	}
}

void DragListView:: handleSelfDrop( QDropEvent * e )
{
	QString text;
	if( !QTextDrag::decode( e, text ) ) {
		e->ignore();
		return;
	}

	QListViewItem * targetItem = itemAt( contentsToViewport( e->pos() ) );

	QStringList names = QStringList::split( QChar('\n'), text );
	QPtrList<QListViewItem> moveList;

	// Make a list of the items to move
	for( QStringList::Iterator it = names.begin(); it != names.end();
			++it ) {
		QListViewItem * item = findItem( *it, mainTextColumn() );
		
		// Make sure none of the items being moved is the
		// target item; if it is, cancel the drop
		if( item == targetItem ) {
			e->ignore();
			return;
		}
		moveList.append( item );
	}

	// Now move each item to its new location.  If no specific
	// item was chosen (drop happened in a blank area), move
	// items to end of list.
	if( targetItem == 0 ) {
		targetItem = lastItem();
	}
	QPtrListIterator<QListViewItem> mli( moveList );
	QListViewItem * mover;
	while( (mover = mli.current() ) != 0 ) {
		++mli;
		mover->moveItem( targetItem );
		setSelected( mover, FALSE );
		targetItem = mover;	// Next item goes after this one
	}

	e->accept();
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

