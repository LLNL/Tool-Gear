// inst_dialog.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 11 October 2002

#include <assert.h>
#include <qbutton.h>
#include <qbuttongroup.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qstringlist.h>

#include "inst_dialog.h"

// Lookup arrays to translate button locations to IP locations and
// types.  These arrays assume that the buttons are created in the
// order shown here.
const TG_InstPtLocation InstrumentationDialog:: iploc[]
	= { TG_IPL_before, TG_IPL_after, TG_IPL_invalid, TG_IPL_invalid };
const TG_InstPtType InstrumentationDialog:: iptype[]
	= { TG_IPT_function_call, TG_IPT_function_call, TG_IPT_function_entry,
		TG_IPT_function_exit };

InstrumentationDialog:: InstrumentationDialog( UIManager * um,
		QWidget * parent, const char * name,
		bool modal, WFlags fl )
	: InstrumentationDialogBase( parent, name, modal, fl ),
	inst_requests( new QPtrList<TGInstSpec> )
{
	// Set auto-deletion on the request list
	inst_requests->setAutoDelete( TRUE );

	// Populate the Action List Combo box by getting the
	// list of actions from the UIManger.  Also populate
	// the corresponding list of tags.  This will let us
	// refer to the action and state tags when the user
	// selects an action from the list.
	int i, actions = um->actionCount();
	for( i = 0; i < actions; ++i ) {
		QString actionTag = um->actionAt( i );
		assert( !actionTag.isNull() );

		int j, states = um->actionStateCount( actionTag );
		for( j = 0; j < states; ++j ) {
			QString stateTag = um->actionStateAt( actionTag, j );
			assert( !stateTag.isNull() );
			QString menuText = um->actionStateToMenuText(
					actionTag, stateTag);
			QString pixmapTag = um->actionStateToPixmapTag(
					actionTag, stateTag );
			QPixmap pixmap = um->pixmapQPixmap( pixmapTag );

			// Put the pixmap and text in the combo box.
			// We rely on items to appear in the box in
			// the same order as they appear in the actionList
			actionComboBox->insertItem( pixmap, menuText );
			actionList.push_back(
					qMakePair( actionTag, stateTag ) );
		}
	}
}

InstrumentationDialog:: ~InstrumentationDialog()
{ }

void InstrumentationDialog:: instrumentationDefined()
{
	// New entry for our list of inst requests
	TGInstSpec * instSpec = new TGInstSpec;

	// We are making both a text version of the request to
	// display to the user and a TGInstSpec object, which
	// is what we'll eventually return to the program.
	
	// Get the action
	int item = actionComboBox->currentItem();
	const QPixmap * pixmap = actionComboBox->pixmap( item );
	QString instSpecStr = actionComboBox->text( item );
	QPair<QString, QString> actionInfo = actionList[item];
	instSpec->actionTag = actionInfo.first;
	instSpec->stateTag = actionInfo.second;

	// Get the type and location.  The IPT_before and IPT_after
	// flags are only meaningful if the location is IPL_call
	QButton * locButton = locationButtons->selected();
	if( locButton != NULL ) {
		instSpecStr += " ";
		instSpecStr += locButton->text();
		int buttonId = locationButtons->id( locButton );
		instSpec->location = iploc[ buttonId ];
		instSpec->type = iptype[ buttonId ];
	} else {
		instSpec->location = TG_IPL_invalid;
		instSpec->type = TG_IPT_invalid;
	}

	// Get the function regular expression
	instSpecStr += " ";
	instSpecStr += functionRELine->text();
	instSpec->functionRE = functionRELine->text();
	
	// Put the new spec string in the list box
	instrumentationListBox->insertItem( *pixmap, instSpecStr );

	// Add the spec to our list
	inst_requests->append( instSpec );
}

void InstrumentationDialog:: deleteRequested()
{
	uint count = instrumentationListBox->count();
	int i;
	// Need to run through the list of selected items backward while 
	// deleting, since deletion can cause some selected items to get
	// lower index numbers, and we could miss them if we started 
	// with low indices and went up.  We need to delete both from
	// the list box and from the internal list.
	for( i = count - 1; i >= 0; --i ) {
		if( instrumentationListBox->isSelected( i ) ) {
			instrumentationListBox->removeItem( i );
			inst_requests->remove( i );
		}
	}
}
QPtrList<TGInstSpec> * InstrumentationDialog:: instrumentationList()
{
	return inst_requests;
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

