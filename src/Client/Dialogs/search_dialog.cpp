// search_dialog.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 31 March 2004

#include "search_dialog.h"

#include <qbuttongroup.h>
#include <qlineedit.h>
#include <qobjectlist.h>
#include <qradiobutton.h>
#include <qwidget.h>

SearchDialog:: SearchDialog( QWidget * parent, const char * name,
		bool modal, WFlags fl )
	: SearchDialogBase( parent, name, modal, fl )
{
	// Disable sorting on the list of cellgrids
	locationList->setSorting( -1 );
}

SearchDialog:: ~SearchDialog()
{
}

void SearchDialog:: setSearchParameters( QString text, bool match,
		bool direction )
{
	searchtext->setText( text );
	if( match ) matchcase->setChecked( TRUE );
	else ignorecase->setChecked( TRUE );

	if( direction ) forward->setChecked( TRUE );
	else backward->setChecked( TRUE );
}

void SearchDialog:: addCellGrid( CellGrid * grid )
{
	// Create a checkbox and make the locationList its parent.
	// This automatically puts it in the right place.
	SearchListButton * sb = new SearchListButton( grid, locationList );
	sb->setOn( TRUE );

	// If the grid's name changes, reset its button
	connect( grid, SIGNAL( nameChanged( CellGrid * ) ),
			this, SLOT( resetButtonText( CellGrid * ) ) );
	// When this grid is deleted (because its window is closed)
	// remove it from the list.  This could happen even when
	// the window is open (and new grids could be added then too!)
	connect( grid, SIGNAL( destroyed( CellGrid * ) ),
			this, SLOT( removeCellGrid( CellGrid * ) ) );

	// When a grid is hidden, remove it from the list
	connect( grid, SIGNAL( cellgridHidden( CellGrid * ) ),
			this, SLOT( hideButton( CellGrid * ) ) );

	// When a grid is show, restore it to the list
	connect( grid, SIGNAL( cellgridShown( CellGrid * ) ),
			this, SLOT( showButton( CellGrid * ) ) );
}

void SearchDialog:: hideButton( CellGrid * g )
{
	SearchListButton * sb = gridToButton( g );
	if( sb ) sb->setVisible( FALSE );
}

void SearchDialog:: showButton( CellGrid * g )
{
	SearchListButton * sb = gridToButton( g );
	if( sb ) sb->setVisible( TRUE );
}

void SearchDialog:: resetButtonText( CellGrid * g )
{
	SearchListButton * sb = gridToButton( g );
	if( sb ) sb->resetName();
}

void SearchDialog:: removeCellGrid( CellGrid * g )
{
	SearchListButton * sb = gridToButton( g );
	if( sb ) delete sb;
}

// Called as a slot from the dialog box
void SearchDialog:: triggerSearch()
{
	QString text = searchtext->text();
	bool match = matchcase->isChecked();
	bool direction = forward->isChecked();

	// Make a list of the checked grids
	// Run through all the children of the locationList and
	// see which ones are checked.  Include grids that are
	// not visible, because the list we generate here may
	// be used later after a grid becomes visible.
	QPtrList<CellGrid> * search_grids  = new QPtrList<CellGrid>;
	QListViewItemIterator it( locationList );
	SearchListButton * sb;
	while(  (sb = (SearchListButton *)(it.current())) != NULL ) {
		++it;
		if( sb->isOn() ) {
			search_grids->append( sb->getGridReference() );
		}
	}

	accept();

	// Should be exactly one receiver of this signal, and it
	// should delete the search_grid when it's done
	emit startSearch( text, match, direction, TRUE, search_grids );
}

SearchListButton * SearchDialog:: gridToButton( CellGrid * g )
{
	// Find the corresponding button (by matching the
	// grid pointer)
	QListViewItemIterator it( locationList );
	SearchListButton * sb;
	while(  (sb = (SearchListButton *)(it.current())) != NULL ) {
		++it;
		if( sb->getGridReference() == g ) {
			return sb;
		}
	}

	return NULL;
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

