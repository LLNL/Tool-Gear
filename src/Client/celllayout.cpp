// celllayout.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
 * Support class CellLayout, which manages the layout of text and annotations
 * in a CellGrid Cell.  Used by CellGrid and the various classes derived
 * from CellAnnotation.  Probably only useful to tool developers creating 
 * custom CellGrid annotations.
 *
 * Designed by John May and John Gyllenhaal at LLNL
 * Initial implementation by John Gyllenhaal around April 2001.
 * Split out of cellgrid.cpp 7/13/01.
 *
*****************************************************************************/

#include <celllayout.h>

// Calculates position, x, of the leftmost pixel of an annotation of width 
// 'annotWidth', based on index, slot, and position in the slot (posFlags).  
// For posFlags, only the SlotLeft, SlotHCenter, SlotRight bits are examined
// (see enum PosFlags) and if none are specified, defaults to SlotHCenter.
// Punts if annotWidth > slotWidth specified by slotRequired!
// Does all the heavy lifting for calcCenterX and calcRightX also.
int CellLayout::calcLeftX (int index, int slot, int posFlags, int annotWidth)
{
    // Get leftmost x postion of target slot, needed by all position calcs
    int slotX = getPos (index, slot);

    // Get width of target slot, needed by center and right justification
    // Get here so can check width below for every call to calcX
    // Note: slotWidth may be 0, that is a zero width slot.
    int slotWidth = getWidth (index, slot, FALSE);

    // The layout algorithm current assumes that annotWidth fits within
    // slotWidth (that is annotWidth <= slotWidth).  Punt if not true.
    // Make message generic since checks for calcCenterX and calcRightX
    // also.
    if (annotWidth > slotWidth)
    {
	TG_error ("CellLayout::calc[Left|Center|Right]X):\n"
		  " Error AnnotWidth(%i) > slot width(%i) for "
		  "index %i slot %i\n"
		  " Make sure CellLayout::slotRequired called with "
		  "correct width!\n",
		  annotWidth, slotWidth, index, slot);
    }

    // Calculate position using posFlag settings
    int annotX;
    if (posFlags & SlotLeft)
    {
	// For leftmost positioning, just return slot's leftmost pixel position
	annotX = slotX;
    }

    // Handle zero width slots explicitly, so can handle zero
    // annotWidth requests properly (below).
    else if (slotWidth == 0)
    {
	// Cell zero width, so return start of cell
	annotX = slotX;
    }

    // Handle other position settings where slotWidth > 0
    else 
    {
	// Force annotWidths <= 0 to be 1, so calculations below work
	// properly (with 0 width, can end up one pixel too far to the right)
	if (annotWidth <= 0)
	    annotWidth = 1;

	// Handle right justification
	if (posFlags & SlotRight)
	{
	    // Find pixel just past end and then subtract width
	    // Code above forces annotWidth >= 1 and annotWidth <= slotWidth,
	    // so forces always to land in cell.
	    annotX = slotX + slotWidth - annotWidth;
	}
	// Handle centered justification (or defaulting to center)
	else 
	{
	    // In cases where two center pixels, picks leftmost one
	    annotX = slotX + (slotWidth - annotWidth)/2;
	}
    }

    // Return the leftmost pixel to draw annotation at
    return (annotX);
}

// Calculates position, x, of the centermost pixel of an annotation of width 
// 'annotWidth', based on index, slot, and position in the slot (posFlags).  
// Most of the heavy lifting is done by calcLeftX, which this routine uses,
// so see calcLeftX for more details.
// For even widths (i.e., two center pixels), returns the rightmost center
// pixel, which QPainter::drawLine requires for proper line placement.

int CellLayout::calcCenterX (int index, int slot, int posFlags, int annotWidth)
{
    // Get leftmost pixel of annotation
    int annotX = calcLeftX (index, slot, posFlags, annotWidth);

    // Adjust to get center pixel, if width >= 2 (otherwise already at center)
    if (annotWidth >= 2)
    {
	// Move to center pixel, rightmost center pixel if two center pixels
	// This is what QPainter::drawLine requires for proper placement
	annotX += annotWidth/2;
    }

    // Return the centermost pixel to draw annotation at
    return (annotX);
}

// Calculates position, x, of the rightmost pixel of an annotation of width 
// 'annotWidth', based on index, slot, and position in the slot (posFlags).  
// Most of the heavy lifting is done by calcLeftX, which this routine uses,
// so see calcLeftX for more details.
int CellLayout::calcRightX (int index, int slot, int posFlags, int annotWidth)
{
    // Get leftmost pixel of annotation
    int annotX = calcLeftX (index, slot, posFlags, annotWidth);

    // Adjust to get rightmost pixel, if width >= 2 (otherwise already at 
    // the rightmost pixel)
    if (annotWidth >= 2)
    {
	// Move to rightmost pixel, need to subtract 1 to get correct pixel
	annotX += annotWidth - 1;
    }

    // Return the rightmost most pixel to draw annotation at
    return (annotX);
}


// Calculates position, y, of the topmost pixel of an annotation of 
// width 'annotWidth', based on the vertical position in the cell (posFlags).
// For posFlags, only the SlotTop, SlotVCenter, SlotBottom bits are examined
// (see enum PosFlags) and if none are specified, defaults to SlotVCenter.
// If annotHeight > cell height, can return location above cell.  The
// cell bounding box will limit what is drawn properly in this case.
// Does all the heavy lifting for calcCenterY and calcBottomX also.
int CellLayout::calcTopY (int posFlags, int annotHeight)
{
    // Get topmost y postion of cell 
    int cellY = getY();

    // Get height of cell 
    int cellHeight = getH();

    // To make calculations below work properly, force at least a height
    // of 1.  Otherwise, end up one pixel too low in calcs below.
    if (annotHeight < 1)
	annotHeight = 1;

    // Calculate based on veritical position flags
    int annotY;
    if (posFlags & SlotTop)
	annotY = cellY;
    else if (posFlags & SlotBottom)
	annotY = cellY + cellHeight - annotHeight;
    else // if centered or defaulting to centered, +1 give bottom pixel on tie
	annotY = cellY + (cellHeight + 1 - annotHeight)/2;

    // Return the topmost pixel to draw annotation at
    return (annotY);
}

// Calculates position, y, of the centermost pixel of an annotation of 
// width 'annotWidth', based on the vertical position in the cell (posFlags).
// All the heavy lifting done by calcTopY, so see calcTopY for details.
// For even widths (i.e., two center pixels), returns the bottommost center
// pixel, which QPainter::drawLine requires for proper line placement.
int CellLayout::calcCenterY (int posFlags, int annotHeight)
{
    // Get topmost pixel of annotation
    int annotY = calcTopY (posFlags, annotHeight);

    // Adjust to get center pixel, if width >= 2 (otherwise already at center)
    if (annotHeight >= 2)
    {
	// Move to center pixel, bottommost center pixel if two center pixels
	annotY += annotHeight/2;
    }

    // Return the centermost pixel to draw annotation at
    return (annotY);
}

// Calculates position, y, of the bottommost pixel of an annotation of 
// width 'annotHeight', based on the vertical position in the cell (posFlags).
// All the heavy lifting done by calcTopY, so see calcTopY for details.
int CellLayout::calcBottomY (int posFlags, int annotHeight)
{
    // Get topmost pixel of annotation
    int annotY = calcTopY (posFlags, annotHeight);

    // Adjust to get bottom pixel, if width >= 2 (otherwise already at bottom)
    if (annotHeight >= 2)
    {
	// Move to bottom pixel, need to subtract 1 to get correct pixel
	annotY += annotHeight - 1;
    }

    // Return the bottommost pixel to draw annotation at
    return (annotY);
}


// Start over, clear all existing info and set to new cell's stats
// Should only be called by layoutCell()
void CellLayout::resetInfo(int nx, int ny, int nw, int nh, int nbaseLine) 
{
    // Delete table and reset linked list of slots
    slotTable.deleteAllEntries(); 
    firstSlot=NULL; 
    lastSlot=NULL;
    x = nx;
    y = ny;
    w = nw;
    h = nh;

    // Assumed fixed baseline for now
    baseLine = nbaseLine;
    placementCalculated = FALSE;

    // No indentation specified by default
    indentLevel = -1;
    firstIndent = 0;
    restIndent = 0;
    totalIndent = 0;
}

// Indicates specified slot is needed with at least minWidth
void CellLayout::slotRequired (int index, int slot, int minWidth)
{
    // Sanity check, cannot change requirements after placement
    if (placementCalculated)
    {
	TG_error ("CellLayout::slotRequired(%i,%i,%i): "
		  "Slots already placed!", index, slot, minWidth);
    }
    
    // Force negative widths to be 0
    if (minWidth < 0)
	minWidth = 0;
    
    // Get existing entry, if any
    SlotInfo *info = slotTable.findEntry (index, slot);
    
    // If exists, update width (if necessary)
    if (info != NULL)
    {
	if (info->width < minWidth)
	    info->width = minWidth;
    }
    // Otherwise, create slotInfo and place in slot table and list
    else
    {
	info = new SlotInfo (index, slot, minWidth);
	TG_checkAlloc(info);
	slotTable.addEntry (index, slot, info);
	
	// Find slot infos this info should go between 
	// Start at end, since this is usually where it will be.
	SlotInfo *afterSlot = lastSlot;
	SlotInfo *beforeSlot = NULL;
	while ((afterSlot != NULL) && ((afterSlot->index > index) ||
				       ((afterSlot->index == index) && 
					(afterSlot->slot > slot))))
	{
	    beforeSlot = afterSlot;
	    afterSlot = afterSlot->prevSlot;
	}
	
	// Set up the doublely linked list of slot infos.
	if (afterSlot != NULL)
	    afterSlot->nextSlot = info;
	else
	    firstSlot = info;
	
	if (beforeSlot != NULL)
	    beforeSlot->prevSlot = info;
	else
	    lastSlot = info;
	
	info->prevSlot = afterSlot;
	info->nextSlot = beforeSlot;
    }
}

// Indicates tree hierarchy indentation required in layout
void CellLayout::indentRequired (int level, int firstSize, int restSize)
{
    // Sanity check, cannot change requirements after placement
    if (placementCalculated)
    {
	TG_error ("CellLayout::indentRequired(%i,%i,%i): "
		  "Slots already placed!", level, firstSize, restSize);
    }

    // Sanity check, indentRequired better not be called twice
    if (indentLevel != -1)
    {
	TG_error ("CellLayout::indentRequired(%i,%i,%i): "
		  "Indent already required!", level, firstSize, restSize);
    }

    // Algorithm requires level >= 0, so force it to be so
    if (level < 0)
	level = 0;

    // Algorithm requires sizes > 0, so force it to be so
    if (firstSize < 1)
	firstSize = 1;
    if (restSize < 1)
	restSize = 1;

    // Initialize indentation parameters
    indentLevel = level;
    firstIndent = firstSize;
    restIndent = restSize;

    // Calculate total indentation needed based on indent level
    totalIndent = firstSize + (level * restSize);
}

// Calculates placement of all slots with left or right justification
/*
 * To get centered placement, use something like:
 * calcPlacement (((cellWidth - totalIndent) - cellLayout.getTotalWidth())/2, 
 *                TRUE); 
 *
 * Note: modification to support indentation may require rethinking 
 * how center calculated.
*/
void CellLayout::calcPlacement (int justifyToPos, bool leftJustify)
{
    // Sanity check, currently don't allow placement twice
    // This is more for efficiency that a true requirement
    if (placementCalculated)
	TG_error ("CellLayout::calcPlacement: Slots already placed!");
    
    
    // Calculate leftmost slot start position based on 
    // justification algorithm
    int pos; 
    
    // For leftJustify, just put leftmost slot at justifyToPos
    // Now take into account indent, for leftJustify only
    if (leftJustify)
    {
	pos = justifyToPos + totalIndent;
    }
    
    // For rightJustify, subtract width (with some tweaking) to
    // get last pixel to fall on justifyToPos
    else
    {
	int width = getTotalWidth();
	// For widths > 0, need to subtract 1 
	if (width > 0)
	    pos = justifyToPos - (width - 1);
	
	// Otherwise, if width is 0, set pos to justifyToPos
	// to get reasonable positions for the zero width slots
	else
	    pos = justifyToPos;
    }
    
    // Start at firstSlot and layout from left to right
    for (SlotInfo *slot = firstSlot; slot != NULL;
	 slot = slot->nextSlot)
    {
	slot->pos = pos;
	pos += slot->width;
    }
    
    // Mark that we have done placement
    placementCalculated = TRUE;
}

// Returns the total width of all slots (in pixel)
int CellLayout::getTotalWidth()
{
    int width = 0;
    
    // Sum up all widths in all cells
    for (SlotInfo *slot = firstSlot; slot != NULL;
	 slot = slot->nextSlot)
    {
	width += slot->width;
    }
    
    return (width);
}

// Get current width of cell (max of all min widths for slot)
// Set allowInvalidSlots to TRUE if want to be able to ask about anything
int CellLayout::getWidth (int index, int slot, bool allowInvalidSlots)
{
    // Return current width, if found, otherwise punt
    SlotInfo *info = slotTable.findEntry (index, slot);
    // Sanity check
    if (info == NULL)
    {
	// If allowing querying of index/slots not specified, return 0
	if (allowInvalidSlots)
	    return (0);
	// Otherwise punt!
	else
	    TG_error ("CellLayout::getWidth (%i, %i): Slot not found!\n",
		      index, slot);
    }
    return (info->width);
}
    
// Returns leftmost pixel of slot
int CellLayout::getPos (int index, int slot)
{
    // Sanity check, must place first
    if (!placementCalculated)
    {
	TG_error ("CellLayout::getPos(%i,%i): Slots not placed!", 
		  index, slot);
    }
    
    // Return current width, if found, otherwise punt
    SlotInfo *info = slotTable.findEntry (index, slot);
    
    // Sanity check
    if (info == NULL) 
    {
	TG_error ("CellLayout::getPos (%i, %i): Slot not found!\n",
		  index, slot);
    }
    
    // Return starting (leftmost) position of slot
    return (info->pos);
}	
// Returns leftmost pixel of indentation slot
int CellLayout::getIndentPos (int level)
{
    // Sanity check, must place first
    if (!placementCalculated)
    {
	TG_error ("CellLayout::getIndentPos(%i): Slots not placed!", level);
    }

    // Sanity check, must be indented cell!
    if (indentLevel < 0)
	TG_error ("CellLayout::getIndentPos(%i): Cell not indented!", level);

    // Sanity check, level must be in bounds
    if ((level < 0) || (level > indentLevel))
    {
	TG_error ("CellLayout::getIndentPos(%i): level must be >= 0 and "
		  "<= %i!", level, indentLevel);
    }
    
    // Returns leftmost pixel of indentation slot
    return (x + totalIndent - firstIndent - (level * restIndent));
}


// Returns total width of indentation (in pixels)
// Returns 0 if no indentation for this level
int CellLayout::getTotalIndent()
{
    if (indentLevel < 0)
	return (0);
    else
	return (totalIndent);
}

// Returns level of indentation.  Returns -1 if no indentation
int CellLayout::getIndentLevel()
{
    return (indentLevel);
}

// Maps x to indent level (0 is rightmost/closest to text, n is
// leftmost (closest to left side of cell).  For example,
// Level 0 is the arrow (or blank space) for closing row's children.
// Level 1 is for closing parent's children.
// Returns TRUE if could do mapping (in indent area), FALSE otherwise
bool CellLayout::pixelToIndentLevel (int px, int &level)
{
    // Substract off 'x' to make px 0 based
    px = px - x;

    // Handle case where we in the indent area of cell?
    if ((px >= 0) && (px < totalIndent))
    {
	// Calculate level we are at
	level = 0;

	// Loop until find level that puts us in range.
	// Set algorithm above guarentees restIndent > 0!
	while ( px < (totalIndent - firstIndent - (level * restIndent)))
	    level++;

	return (TRUE);
    }
    // Otherwise, return FALSE.  Not in indent area
    else
    {
	level = NULL_INT;
	return (FALSE);
    }
}


// Maps px to index/slot.  Returns TRUE if could do mapping, FALSE otherwise
bool CellLayout::pixelToLocation (int px, int &index, int &slot)
{
    // Sanity check, must place first
    if (!placementCalculated)
	TG_error ("CellLayout::pixelToLocation(%i): Slots not placed!", px);

    // If in indent zone, return FALSE (not mapped to index/slot)
    if (px < totalIndent)
    {
	index = NULL_INT;
	slot = NULL_INT;
	return (FALSE);
    }
    
    // Scan slots looking for the first one that covers the pixel,
    // can never return 0 width slots (by algorithm design)
    for (SlotInfo *slotInfo = firstSlot; slotInfo != NULL;
	 slotInfo = slotInfo->nextSlot)
    {
	// If found slot, set index/slot and return true
	if ((slotInfo->pos <= px) && 
	    (px < (slotInfo->pos + slotInfo->width)))
	{
	    index = slotInfo->index;
	    slot = slotInfo->slot;
	    return (TRUE);
	}
    }
    
    // If got here, didn't find slot, set index/slot to NULL_INT
    // and return FALSE
    index = NULL_INT;
    slot = NULL_INT;
    return (FALSE);
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

