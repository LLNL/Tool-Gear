//! \file celllayout.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/*!
 * Support class CellLayout, which manages the layout of text and annotations
 * in a CellGrid Cell.  Used by CellGrid and the various classes derived
 * from CellAnnotation.  Probably only useful to tool developers creating 
 * custom CellGrid annotations.
 */
/****************************************************************************
**
** Designed by John May and John Gyllenhaal at LLNL
** Initial implementation by John Gyllenhaal around April 2001.
** Split out of cellgrid.h 7/13/01.
**
*****************************************************************************/


#ifndef CELLLAYOUT_H
#define CELLLAYOUT_H

#include <qwidget.h>
#include <intarraytable.h>
#include <l_punt.h>

// Include ToolGear types, such as NULL_INT, NULL_DOUBLE, etc.
#include <tg_types.h>


//! For now, place position enum here (move into ToolGear namespace later)
enum PosFlag {
    SlotLeft    = 0x00000001,
    SlotHCenter = 0x00000002,
    SlotRight   = 0x00000004,
    SlotTop     = 0x00000010,
    SlotVCenter = 0x00000020,
    SlotBottom  = 0x00000040
};

//! CellGrid support class that determine the position and width of each
//! slot in a cell. 
class CellLayout {
public:
    CellLayout () : 
	// Have slot table delete all SlotInfo allocated on exit
	slotTable ("slotTable", DeleteData), x(-1), y(-1), w(-1), h(-1),
	firstSlot(NULL), lastSlot(NULL), baseLine (-1), 
	placementCalculated(FALSE),
	indentLevel(-1), firstIndent(0), restIndent(0), totalIndent(0){}
    
    //! Automatic destructor deletes everything correctly
    ~CellLayout () {}

    //! Internal Structure used by CellLayout
    struct SlotInfo {
	SlotInfo (int nindex, int nslot, int nwidth=0):
	    index(nindex), slot (nslot), width(nwidth), pos (NULL_INT) {}
	~SlotInfo() {}
    private:
	// Allow CellLayout to access data SlotInfo directly
	friend class CellLayout;

	//! Index of slot
	int index;
        //! slot id
	int slot;
        //! Width of slot
	int width;
        //! Leftmost pixel of slot
	int pos;
        //! For linked list of slots in order
	SlotInfo *nextSlot;
        //! in CellLayout
	SlotInfo *prevSlot;
    };

    //! Calculates position, x, of the leftmost pixel of an annotation of 
    //! width 'annotWidth', based on index, slot, and position in the 
    //! slot (posFlags).  
    //! For posFlags, only the SlotLeft, SlotHCenter, SlotRight bits are 
    //! examined (see enum PosFlags) and if none are specified, defaults 
    //! to SlotHCenter.
    //! Punts if annotWidth > slotWidth specified by slotRequired!
    //! Does all the heavy lifting for calcCenterX and calcRightX also.
    int calcLeftX (int index, int slot, int posFlags, int annotWidth);

    //! Calculates position, x, of the centermost pixel of an annotation of 
    //! width 'annotWidth', based on index, slot, and position in the 
    //! slot (posFlags).  Most of the heavy lifting is done by calcLeftX, 
    //! which this routine uses, so see calcLeftX for more details.
    //! For even widths (i.e., two center pixels), returns the rightmost center
    //! pixel, which QPainter::drawLine requires for proper line placement.
    int calcCenterX (int index, int slot, int posFlags, int annotWidth);

    //! Calculates position, x, of the rightmost pixel of an annotation of 
    //! width 'annotWidth', based on index, slot, and position in the 
    //! slot (posFlags).  Most of the heavy lifting is done by calcLeftX, 
    //! which this routine uses, so see calcLeftX for more details.
    int calcRightX (int index, int slot, int posFlags, int annotWidth);

    //! Calculates position, y, of the topmost pixel of an annotation of 
    //! width 'annotWidth', based on the vert position in the cell (posFlags).
    //! For posFlags, only the SlotTop, SlotVCenter, SlotBottom bits are 
    //! examined (see enum PosFlags) and if none are specified, defaults to 
    //! SlotVCenter.
    //!
    //! If annotHeight > cell height, can return location above cell.  The
    //! cell bounding box will limit what is drawn properly in this case.
    //! Does all the heavy lifting for calcCenterY and calcBottomX also.
    int calcTopY (int posFlags, int annotHeight);

    //! Calculates position, y, of the centermost pixel of an annotation of 
    //! width 'annotWidth', based on the vert position in the cell (posFlags).
    //! All the heavy lifting done by calcTopY, so see calcTopY for details.
    //! For even widths (i.e., two center pixels), returns the bottommost center
    //! pixel, which QPainter::drawLine requires for proper line placement.
    int calcCenterY (int posFlags, int annotHeight);

    //! Calculates position, y, of the bottommost pixel of an annotation of 
    //! width 'annotWidth', based on the vert position in the cell (posFlags).
    //! All the heavy lifting done by calcTopY, so see calcTopY for details.
    int calcBottomY (int posFlags, int annotHeight);
    
    //! Start over, clear all existing info and set to new cell's stats
    //! Should only be called by layoutCell()
    void resetInfo(int nx, int ny, int nw, int nh, int nbaseLine);
    
    //! Indicates specified slot is needed with at least minWidth
    void slotRequired (int index, int slot, int minWidth);

    //! Indicates tree hierarchy indentation required in layout
    void indentRequired (int level, int firstSize, int restSize);

    //! Calculates placement of all slots with left or right justification
    //! To get centered placement, use something like:
    //! calcPlacement ((cellWidth - cellLayout.getTotalWidth())/2, TRUE);
    void calcPlacement (int justifyToPos, bool leftJustify);

    //! Returns the total width of all slots (in pixels)
    int getTotalWidth();

    //! Get current width of cell (max of all min widths for slot)
    //! Set allowInvalidSlots to TRUE if want to be able to ask about anything
    int getWidth (int index, int slot, bool allowInvalidSlots);

    //! Returns leftmost pixel of slot
    int getPos (int index, int slot);

    //! Returns leftmost pixel of indentation slot
    int getIndentPos (int level);

    //! Returns total width of indentation (in pixels)
    int getTotalIndent();

    //! Returns level of indentation
    int getIndentLevel();
    
    //! Maps x to indent level (0 is rightmost/closest to text).
    //! Returns TRUE if could do mapping (in indent area), FALSE otherwise
    bool pixelToIndentLevel (int x, int &level);

    //! Maps x to index/slot.  Returns TRUE if could do mapping, FALSE otherwise
    bool pixelToLocation (int x, int &index, int &slot);

    //! Return dimensions and baseline point for cell
    inline int getX () {return x;}
    //! Return dimensions and baseline point for cell
    inline int getY () {return y;}
    //! Return dimensions and baseline point for cell
    inline int getW () {return w;}
    //! Return dimensions and baseline point for cell
    inline int getH () {return h;}
    //! Return dimensions and baseline point for cell
    inline int getBaseLine () {return baseLine;}

    
private:
    //! Holds all specified slots info
    IntArrayTable<SlotInfo> slotTable;
    int x;
    int y;
    int w;
    int h;
    SlotInfo *firstSlot;
    SlotInfo *lastSlot;
    int baseLine;
    bool      placementCalculated;

    // To handle tree hierarchy indentation
    int indentLevel;  // Levels to indent, -1 means no indentation
    int firstIndent;  // Size of first (rightmost) indentation
    int restIndent;   // Size of remaining indentations
    int totalIndent;  // Pixels indented
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

