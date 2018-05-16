//! \file cellgrid_searcher.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 30 March 2004

#ifndef CELLGRID_SEARCHER_H
#define CELLGRID_SEARCHER_H

#include "cellgrid.h"
#include "search_dialog.h"

#include <qcolor.h>
#include <qptrlist.h>

//! Provides an interface for searching all CellGrid objects in use

//! This class keeps track of the current search parameters so it can
//! resume a search upon request.  It manages the highlighting in
//! the CellGrid of the text that has been found.  It tries to
//! respond intelligently when a grid that has the current search
//! position is removed, or when the currently-highlighted text changes.
class CellGridSearcher : public QObject {
	Q_OBJECT
public:
	CellGridSearcher();
	~CellGridSearcher();
public slots:

	//! Call this to pop up a dialog box for searching the displayed text.
	//! Designed to allow menu/hot-key use.
	void searchForText();

	//! Call this to repeat the last search; designed for menu/hot-key use.
	void searchForTextAgain();

	//! Called to add a grid to our list
	void addCellGrid( CellGrid * c );

	//! Called when a CellGrid is destroyed
	void deleteCellGrid( CellGrid * c );

	//! Called when the contents of a grid are reset to reset the
	//! stored search parameters, if needed
	void contentsClearedHandler( CellGrid * c );

	//! Called by CellGrid when a highlighted cell's contents have
	//! been changed (so we can remove the highlighting)
	void cellContentsChangedHandler( CellGrid * c, int recordId,
			int attrId );
	
	//! Called when the user initiates a search
	void doSearch( QString searchText, bool caseSensitive,
			bool forwardSearch, bool newSearch,
			QPtrList<CellGrid> * searchGrids );

protected:
	//! Highlight found item
	void addSearchHighlighting( CellGrid * grid, int recordId, int attrId,
			int startIndex, int textLength );

	//! Remove colored highlight on found item
	void clearSearchHighlighting();

protected slots:
	//! Respond to clicks in a grid
	void gridClickedHandler( CellGrid * );

private:
        //! Create a single search dialog box for this window
	SearchDialog searchDialogBox;

	QPtrList<CellGrid> grids;  //!< All searchable grids
	QPtrList<CellGrid> * searchGrids; //!< Grids user has chosen to search
	CellGrid * currentGrid;	   //!< Location of most recently found item
	QString lastSearchText;    //!< Text last searched
	int nextSearchPos;         //!< Next search recordId
	int nextSearchAttrId;      //!< Next search attrId
	int nextSearchIndex;       //!< Next search textIndex
	bool nextSearchCase;       //!< Next search caseSensitivity
	bool nextSearchForward;    //!< Next search forwardDirection
	int searchHighlightRecordId; //!< Current search highlight recordId
	int searchHighlightAttrId;   //!< Current search highlight attrId
	int searchHighlightHandle; //!< Identifies highlight to grid
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

