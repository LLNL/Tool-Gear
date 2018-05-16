// cellgrid_searcher.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 30 March 2004

#include "cellgrid_searcher.h"
#include <celllayout.h>
#include <iostream>
#include <qmessagebox.h>

CellGridSearcher:: CellGridSearcher()
	: searchDialogBox( 0 ), searchGrids( NULL ),
	currentGrid( NULL ), lastSearchText( QString::null ),
	nextSearchPos( NULL_INT ), nextSearchAttrId( NULL_INT ),
	nextSearchIndex( NULL_INT ), nextSearchCase( FALSE ),
	nextSearchForward( TRUE ), searchHighlightRecordId( NULL_INT ),
	searchHighlightAttrId( NULL_INT ), searchHighlightHandle( -3 )
{

	connect( &searchDialogBox, SIGNAL(
		startSearch( QString, bool, bool, bool, QPtrList<CellGrid> *) ),
		this, SLOT( doSearch( QString, bool, bool, bool,
				QPtrList<CellGrid> *) ) );
}

CellGridSearcher:: ~CellGridSearcher()
{
	// No cleanup needed
}

void CellGridSearcher:: searchForText()
{
	// Set the other search parameters and show the dialog.
	// doSearch will catch the signal when the user triggers
	// the search.
	searchDialogBox.setSearchParameters( lastSearchText,
			nextSearchCase, nextSearchForward );

	searchDialogBox.show();
}

void CellGridSearcher:: searchForTextAgain()
{
	// If no current valid search text, silently ignore
	// However, if there is text but the cell grid last
	// highlighted is gone, we just start with the same
	// text in the next available grid.
	if( lastSearchText.isNull() ) {
		return;
	}

	doSearch( lastSearchText, nextSearchCase, nextSearchForward, FALSE,
	       NULL );
}

void CellGridSearcher:: addCellGrid( CellGrid * c )
{
	// Add the new item to our list
	grids.append( c );

	// Add it to our list of places to search in the dialog box
	searchDialogBox.addCellGrid( c );

	// Also add this grid to our current list of items to be
	// searched.  However, we don't want to change the "current"
	// pointer in this list, so we have to read the current
	// index and restore it.  Since we're adding the new item
	// to the end of the list, it shouldn't affect the index
	if( searchGrids != NULL ) {
		int curIndex = searchGrids->at();
		searchGrids->append( c );
		searchGrids->at( curIndex );	// resets current index
	}

	// When this grid is clicked, see if we need to do anything
	connect( c, SIGNAL( gridClicked( CellGrid * ) ),
			this, SLOT( gridClickedHandler( CellGrid * ) ) );

	// When this item is destroyed, get a signal so we can
	// remove it from the list.
	connect( c, SIGNAL( destroyed( CellGrid * ) ),
			this, SLOT( deleteCellGrid( CellGrid * ) ) );

	// When a grid is cleared, we may need to reset some search info
	connect( c, SIGNAL( gridContentsCleared( CellGrid * ) ),
			this, SLOT( contentsClearedHandler( CellGrid * ) ) );
}

void CellGridSearcher:: deleteCellGrid( CellGrid * c )
{
	// Remove specified item from our list.  Don't
	// remove null item, because that causes the
	// QPtrList class to remove the current item,
	// which is probably not what we want.
	if( c != NULL ) {
		grids.removeRef( c );

		// If current search position was in this
		// grid, reset the stored search parameters
		if( c == currentGrid ) {
			currentGrid = NULL;
			nextSearchPos = NULL_INT;
			nextSearchAttrId = NULL_INT;
			nextSearchIndex = NULL_INT;
			searchHighlightRecordId = NULL_INT;
			searchHighlightAttrId = NULL_INT;
		}

		// If have searchGrids defined, remove this grid from it
		// -JCG 4/12/04
		if (searchGrids != NULL) {
		    // Figure out how we'll set the new current
		    // item once this one is removed.
		    int curIndex = searchGrids->at();
		    int itemIndex = searchGrids->findRef( c );
		    searchGrids->remove( itemIndex );
		    
		    // Now figure out how to reset the current item
		    // If an item was pulled out below, adjust the index
		    // downward to compensate; it's still the same item
		    if( itemIndex < curIndex ) --curIndex;
		    // If current item was removed, the index now points to the
		    // succeeding item.  For forward search this is correct.
		    // For backward search, we go down one.
		    else if( itemIndex == curIndex ) {
			if( !nextSearchForward ) {
			    if( --curIndex < 0 ) {
				// Wrap around
				curIndex = searchGrids->count() - 1;
			    }
			}
		    }
		    // Nothing to do if item removed had higher index
		    
		    // Now reset the current item
		    searchGrids->at( curIndex );	
		}
	}

}

void CellGridSearcher:: doSearch( QString searchText, bool caseSensitive,
		bool forwardSearch, bool newSearch,
		QPtrList<CellGrid> * sg )
{
	// Nothing to do if no search text given
	if( searchText.isEmpty() ) return;

	// Replace the old list of grids to search with the newly-created one
	if( sg != NULL ) {
		delete searchGrids;
		searchGrids = sg;
	}

	// Remove highlight on any previously found item
	clearSearchHighlighting();

	int startPos = (newSearch ? NULL_INT : nextSearchPos);
	int startAttrId = (newSearch ? NULL_INT : nextSearchAttrId);
	int startIndex = (newSearch ? NULL_INT : nextSearchIndex);
	
	// Search for text successively in each grid; if found,
	// nextSearchPos and nextSearchAttrId are automatically
	// updated so the next search can start where this one
	// left off.
	// For now, don't search closed trees and always wrap.
	
	CellGrid * grid = ( newSearch ? 
			( forwardSearch ? searchGrids->first()
			 	: searchGrids->last() )
		       	: searchGrids->current() );

	unsigned grids_searched;
	bool found = FALSE;
	CellGrid * firstGrid = searchGrids->current();
	CellGrid * tmp;
	// Loop over the list of grids to be searched.  The messy
	// increment-expression gets the next grid for forward
	// or backward searches and wraps around if necessary,
	// then increments the count of grids searched.
	for( grids_searched = 0; grids_searched < searchGrids->count();
		grid = ( forwardSearch ?
		(((tmp = searchGrids->next()) == 0) ? searchGrids->first()
							 : tmp )
		: (((tmp = searchGrids->prev()) == 0) ? searchGrids->last()
		 					: tmp ) ),
	       	++grids_searched ) {

		// Don't search in hidden grids
		if( ! grid->isVisible() ) continue;

		found = grid->findText(startPos, startAttrId, startIndex,
			       searchText, forwardSearch,
			       caseSensitive, TRUE, FALSE);
		if( found ) break;
	}

	// If we're continuing a search, we need to wrap back to
	// search the first part of the first grid if nothing
	// found so far.
	// When we pass nextSearchIndex - 1 (for forward search)
	// we're counting on the fact that it was set on the
	// previous search without changing nextSearchPos or
	// nextSearchAttr b/c findText adjusts these automatically
	// if we've fallen off then end of a cell.  In other words,
	// nextSearchIndex should never be 0 at this point.
	if( !found && !newSearch && firstGrid != NULL ) {
		grid = firstGrid;
		found = grid->findText( startPos, startAttrId,
				startIndex, searchText, forwardSearch,
				caseSensitive, TRUE, FALSE,
				nextSearchPos, nextSearchAttrId,
				nextSearchIndex + (forwardSearch ? -1 : 1) );
	}

	if (found) {
#if 0
	    printf ("Found '%s' at recordId %i attrId %i index %i!\n",
		    searchText.latin1(), startPos, startAttrId, startIndex);
#endif
	    addSearchHighlighting( grid, startPos, startAttrId, startIndex,
			    searchText.length() );

	    // Record that has annotation
	    searchHighlightRecordId = startPos;
	    searchHighlightAttrId = startAttrId;

	    // Make sure highlighted text is visible
	    grid->scrollCellIntoView (startPos, startAttrId);

	    // Ask the cellgrid to notify us if the text in the cell
	    // changes, so we can remove the highlighting.
	    grid->setSearchEngineWatchpoint( startPos, startAttrId );

	    connect( grid, SIGNAL( searchEngineWatchpointTriggered(
					    CellGrid *, int, int) ),
				    this, SLOT( cellContentsChangedHandler(
						    CellGrid *, int, int ) ) );
	}

	// Store info for Find again command
	lastSearchText = searchText;
	nextSearchPos = startPos;
	nextSearchAttrId = startAttrId;
	if (forwardSearch)
	    nextSearchIndex = startIndex + 1;
	else
	    nextSearchIndex = startIndex - 1;
	nextSearchCase = caseSensitive;
	nextSearchForward = forwardSearch;
	currentGrid = grid;
	

	if( !found ) 
	{
	    // Mark that should not allow search again until find again
	    nextSearchPos = NULL_INT;
	    // No grid has found data
	    currentGrid = NULL;

	    // Message dialog returns 0 if user wants to
	    // try again; 1 otherwise.
	    if( QMessageBox::information( &searchDialogBox,
					  "Find",
					  QString("Text \"") + searchText
					  + QString("\" was not found.\n"
						    "Try a new search?"),
					  "&Search", "&Cancel", QString::null,
					  0, 1 ) == 0 ) 
	    {
		searchForText();
	    }
	}
}

void CellGridSearcher:: addSearchHighlighting( CellGrid * grid,
		int recordId, int attrId,
		int startIndex, int textLength )
{
	// Define once and cache for better performance
	static QColor searchHighlightColor("yellow");

	// Use CellGrid's annotation feature to highlight text
	grid->addCellRectAnnot( recordId,
			attrId, 
			searchHighlightHandle,
			TRUE, // Clear any annotation with this handle
			-1,    // Layer, put behind text with -1
			FALSE, // Not clickable 
			 // Start at top/left side of first char
			startIndex,
			0, 
			SlotLeft|SlotTop, 
			// Stop at bottom/right side of last char
			startIndex + textLength -1,
			 0,
			SlotRight|SlotBottom,
			// No line color
			QColor(),
			0,
			// Only fill color for rectangle, don't put
			// named color here for efficiency reasons
			searchHighlightColor );

	// Since not all of cell may be visible, also underline entire text
	QString cellText = grid->cellScreenText( recordId, attrId );
	grid->addCellRectAnnot( recordId,
			attrId, 
			searchHighlightHandle,
			FALSE, // Don't delete rectangle above
			-1,    // Layer, put behind text with -1
			FALSE, // Not clickable 
			// Start at bottom, beginning of cell
			0, 
			0, 
			SlotLeft|SlotBottom, 
			// Stop at bottom/right side of last char
			cellText.length()-1,
			0,
			SlotRight|SlotBottom,
			searchHighlightColor,
			2 /* Line width*/);
}

void CellGridSearcher:: clearSearchHighlighting()
{
	// Remove old search highlighting, if it exists
	if( searchHighlightRecordId != NULL_INT && currentGrid != NULL ) {
		currentGrid->removeCellAnnot( searchHighlightRecordId,
						searchHighlightAttrId,
						searchHighlightHandle );

//		currentGrid->flushUpdate();

		// Disable notification when the text changes
		currentGrid->clearSearchEngineWatchpoint();
	    	disconnect( currentGrid,
				SIGNAL( searchEngineWatchpointTriggered(
					CellGrid *, int, int) ),
				this, SLOT( cellContentsChangedHandler(
					CellGrid *, int, int ) ) );

		// Mark that we have cleared it
		searchHighlightRecordId = NULL_INT;
		searchHighlightAttrId = NULL_INT;
	}
}

void CellGridSearcher:: cellContentsChangedHandler( CellGrid * grid,
		int recordId, int attrId )
{
	// Sanity check: is the changed cell the one that we think
	// is highlighted?
	if( grid != currentGrid || recordId != searchHighlightRecordId
			|| attrId != searchHighlightAttrId ) {
		qWarning( "Internal error: grid %p says watched cell %d,%d "
				"has changed, but cellgrid searcher thinks "
				"grid %p cell %d,%d is highlighted!\n",
				grid, recordId, attrId, currentGrid,
				searchHighlightRecordId,
				searchHighlightAttrId );
		return;
	}

	// Now see if text in cell has really changed compared to
	// search text.  It might not have if, for example, an
	// annotation changed but not the text, or some other text
	// in the cell changed.
	// We need to compare the appropriate subset of the cell
	// contents, starting at the position where the current item
	// was found (offset one from nextSearchIndex) and running
	// the length of the search text
	
	int pos = nextSearchIndex + (nextSearchForward ? -1 : 1 );
	int textlen = lastSearchText.length();
	QString cellText( grid->cellScreenText( recordId, attrId )
			.mid( pos, textlen ) );

	// Do comparison using current case-sensitivity setting.  Since
	// strings are the same length, they are equal if one contains
	// the other.
	if( cellText.contains( lastSearchText, nextSearchCase ) == 0 ) {
		// If there's a change, clear the highlighting
		clearSearchHighlighting();
	} else {
		// Text hasn't changed.  It's possible, though, that
		// the highlighting was removed (if an annotation
		// was changed), so we need to redraw it.
		addSearchHighlighting( grid, recordId, attrId, pos, textlen );
	}
}

void CellGridSearcher:: gridClickedHandler( CellGrid * g )
{

	// If a grid with highlighting in it gets clicked, remove
	// the highlighting, otherwise leave it alone.  The only
	// grid that should have highlighting is the current one.
	if( g == currentGrid  ) {
		clearSearchHighlighting();
	}
}

void CellGridSearcher:: contentsClearedHandler( CellGrid * g )
{
	// If the current grid's contents have been cleared, it
	// doesn't make sense to resume a search from the last
	// location.  But we still want to resume in this grid,
	// so just set the position to the start (or the end)
	// of the grid.  
	if( g == currentGrid ) {
		nextSearchPos = NULL_INT;
		nextSearchAttrId = NULL_INT;
		nextSearchIndex = NULL_INT;
		searchHighlightRecordId = NULL_INT;
		searchHighlightAttrId = NULL_INT;

		// Since the highlighted text is going away, we need
		// to remove the watchpoint on it that tells us when
		// it changes.
		currentGrid->clearSearchEngineWatchpoint();
	    	disconnect( currentGrid,
				SIGNAL( searchEngineWatchpointTriggered(
					CellGrid *, int, int) ),
				this, SLOT( cellContentsChangedHandler(
					CellGrid *, int, int ) ) );
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

