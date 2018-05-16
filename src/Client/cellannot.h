//! \file cellannot.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/*!
 * Interface class CellAnnot, which allows generic cell annotations
 * to be created for use by CellGrid.  CellGrid provides the interface
 * to the "standard" annotations created in this file, so this class
 * is useful to tool developers only if they want to create new custom
 * annotations.
 *
 * NOTE: These annotations are restricted to within *one* cell, 
 * possibly between multiple slots within the cell.  
 */
/****************************************************************************
**
** Designed by John May and John Gyllenhaal at LLNL
** Initial implementation by John Gyllenhaal around April 2001.
** Split out of cellgrid.h 7/13/01.
**
*****************************************************************************/

#ifndef CELLLANNOT_H
#define CELLLANNOT_H

#include <qpainter.h>
#include <qpixmap.h>
#include <qcolor.h>
#include <celllayout.h>
#include <l_punt.h>

// DEBUG
#include <stdio.h>

//! Interface class for cell annotations used by CellGrid
/*! These are annotations within *one* cell, possibly between
 *  multiple slots inside the cell. */
class CellAnnot {
public:
    //! All annotation must have a handle and a layer drawn at.
    //! Handle and layer may be any value
    CellAnnot (int handle, int layer): 
	myHandle(handle), myLevel(layer) {}

    //! Virtual destructor, so derived class can redefine
    virtual ~CellAnnot (){}

    //! Returns annotation handle (must not change after creation)
    inline int handle() {return myHandle;}

    //! Returns layer that annotation should be drawn at (must not
    //! change after creation of annotation)
    inline int layer() {return myLevel;}

    //! When called, must specify layout requirments to layout
    //! by making layout.requireSlot() calls. 
    virtual void requirements (CellLayout &layout) = 0;

    //! When called, must paint cell annotation in p, using layout
    //! to determine x and y positions 
    virtual void draw (QPainter &p, CellLayout &layout) = 0;

    //! When called, return TRUE if annotation clickable at index/slot
    //! Return FALSE if annotation not clickable, or not at location
    virtual int clickableAt (int index, int slot) = 0;

private:
    // Make private since should not be changed after creation
    int myHandle;
    int myLevel;
};

//! Pixmap annotation.  Used by CellGrid and not for direct user use.
class CellPixmapAnnot : public CellAnnot {
public:
    //! Create pixmap annotation, only holds pointer to pixmap
    CellPixmapAnnot (int handle, int layer, bool clickable, 
		     int index, int slot, int posFlags, QPixmap *pixmap) :
	CellAnnot(handle, layer), myIndex(index), mySlot(slot), 
	myPosFlags(posFlags), isClickable (clickable), myPixmap(pixmap)
    {};

    //! Require a slot at the position specified as least as width as pixmap
    void requirements (CellLayout &layout)
    {
	layout.slotRequired (myIndex, mySlot, myPixmap->width());
    }

    void draw (QPainter &p, CellLayout &layout)
    {
	// Calculate leftmost (startX) and topmost(startY) pixel location
	int startX = layout.calcLeftX (myIndex, mySlot, myPosFlags, 
				   myPixmap->width());
	int startY = layout.calcTopY (myPosFlags, myPixmap->height());

	// Draw pixmap at calculated start position
	p.drawPixmap (startX, startY, *myPixmap);
    }

    //! When called, return TRUE if annotation clickable at index/slot
    //! Return FALSE if annotation not clickable, or not at location
    int clickableAt (int index, int slot)
    {
	// If is clickable and location match, return true
	if (isClickable && (myIndex == index) && (mySlot == slot))
	    return (TRUE);
	else
	    return (FALSE);
    }


private:
    int myIndex;
    int mySlot;
    int myPosFlags;
    bool isClickable;
    QPixmap *myPixmap;
};

//! Annotation for a line
class CellLineAnnot : public CellAnnot {
public:
    //! Create line annotation, only holds pointer to pixmap
    CellLineAnnot (int handle, int layer, bool clickable, 
		   int startIndex, int startSlot, int startPosFlags, 
		   int stopIndex, int stopSlot, int stopPosFlags,
		   const QColor &color, int width) :
	CellAnnot(handle, layer), 
	index1(startIndex), slot1(startSlot), posFlags1(startPosFlags), 
	index2(stopIndex), slot2(stopSlot), posFlags2(stopPosFlags), 
	isClickable (clickable), lineColor(color), lineWidth(width)
    {};

    //! Require a slot at the position specified as least as width of line
    void requirements (CellLayout &layout)
    {
	// Assume line width sets required width for now.
	// That is, assume horizonal line end points require line width
	// in the start and stop slot.
	layout.slotRequired (index1, slot1, lineWidth);
	layout.slotRequired (index2, slot2, lineWidth);
    }

    //! Draw line (unless color invalid, which doesn't draw anything but
    //! still is clickable and does reserve space)
    void draw (QPainter &p, CellLayout &layout)
    {
	// Don't actually draw line if color invalid.  Allows 'transparent'
	// clickable lines to be put on display.
	if (!lineColor.isValid())
	    return;

	// Calculate x,y location of the 'centered' starting point for line 
	// The Qt::SquareCap setting for the line drawer below extends
	// the line 1/2 lineWidth around the starting point, so using
	// the 'center' point gives exactly the point we want.
	int sx1 = layout.calcCenterX (index1, slot1, posFlags1, lineWidth);
	int sy1 = layout.calcCenterY (posFlags1, lineWidth);

	// Calculate x,y location of the 'centered' ending point for line.
	int sx2 = layout.calcCenterX (index2, slot2, posFlags2, lineWidth);
	int sy2 = layout.calcCenterY (posFlags2, lineWidth);

	// Draw line at calculated start and end points
	p.setPen(QPen(lineColor, lineWidth, Qt::SolidLine, 
		      Qt::SquareCap, Qt::MiterJoin));
	p.drawLine (sx1, sy1, sx2, sy2);
    }

    //! When called, return TRUE if annotation clickable at index/slot
    //! Return FALSE if annotation not clickable, or not at location
    int clickableAt (int index, int slot)
    {
	// If is clickable and location match, return true
	if (isClickable && 
	    ((index > index1) || ((index == index1) && (slot >= slot1))) &&
	    ((index < index2) || ((index == index2) && (slot <= slot2))))
	    return (TRUE);
	else
	    return (FALSE);
    }


private:
    int index1;
    int slot1;
    int posFlags1;
    int index2;
    int slot2;
    int posFlags2;
    bool isClickable;
    QColor lineColor;
    int lineWidth;
};


//! Annotation for a clickable region (nothing drawn)
class CellClickableAnnot : public CellAnnot {
public:
    //! Create clickable annotation
    CellClickableAnnot (int handle, int layer, 
			int startIndex, int startSlot,
			int stopIndex, int stopSlot) :
	CellAnnot(handle, layer), 
	index1(startIndex), slot1(startSlot), 
	index2(stopIndex), slot2(stopSlot)
    {};

    //! Require endpoints to exist (but can be 0 width)
    void requirements (CellLayout &layout)
    {
	layout.slotRequired (index1, slot1, 0);
	layout.slotRequired (index2, slot2, 0);
    }

    //! Draw nothing (this annotation is invisible)
    void draw (QPainter &, CellLayout &)
    {
	return;
    }

    //! When called, return TRUE if annotation clickable at index/slot
    //! Return FALSE if annotation  not at location
    int clickableAt (int index, int slot)
    {
	// If location matches, return true
	if (((index > index1) || ((index == index1) && (slot >= slot1))) &&
	    ((index < index2) || ((index == index2) && (slot <= slot2))))
	    return (TRUE);
	else
	    return (FALSE);
    }


private:
    int index1;
    int slot1;
    int index2;
    int slot2;
};

//! Annotation for a Rectangle inside a cell
class CellRectAnnot : public CellAnnot {
public:
    //! Create rectangle annotation
    CellRectAnnot (int handle, int layer, bool clickable, 
		   int startIndex, int startSlot, int startPosFlags, 
		   int stopIndex, int stopSlot, int stopPosFlags,
		   const QColor &boxColor, int width, 
		   const QColor &insideColor) :
	CellAnnot(handle, layer), 
	index1(startIndex), slot1(startSlot), posFlags1(startPosFlags), 
	index2(stopIndex), slot2(stopSlot), posFlags2(stopPosFlags), 
	isClickable (clickable), lineColor(boxColor), 
	lineWidth(width), fillColor(insideColor)
    {};

    //! Require a slot at the position specified as least as width of line
    void requirements (CellLayout &layout)
    {
	// Assume line width sets required width for now.
	// That is, assume horizonal line end points require line width
	// in the start and stop slot.
	layout.slotRequired (index1, slot1, lineWidth);
	layout.slotRequired (index2, slot2, lineWidth);
    }

    //! Draw Rectangle and/or fill rectangle with fill color 
    //! (unless colors are invalid, which doesn't draw anything for that 
    //! portionbut still is clickable and does reserve space)
    void draw (QPainter &p, CellLayout &layout)
    {
	// Don't actually draw box if both line and fill colors are invalid.  
	// Allows 'transparent' clickable boxes to be put on display.
	// Treat lineWidth < 1 as invalid line color.
	if ((!lineColor.isValid() || lineWidth < 1) && 
	    !fillColor.isValid())
	    return;

	// Calculate x,y location of the 'centered' starting point for line 
	// The Qt::SquareCap setting for the line drawer below extends
	// the line 1/2 lineWidth around the starting point, so using
	// the 'center' point gives exactly the point we want.
	int sx1 = layout.calcCenterX (index1, slot1, posFlags1, lineWidth);
	int sy1 = layout.calcCenterY (posFlags1, lineWidth);

	// Calculate x,y location of the 'centered' ending point for line.
	int sx2 = layout.calcCenterX (index2, slot2, posFlags2, lineWidth);
	int sy2 = layout.calcCenterY (posFlags2, lineWidth);

	// Draw lines to create box at calculated start and end points
	// If line color is valid and lineWidth > 0.
	if (lineColor.isValid() && (lineWidth > 0))
	{
	    p.setPen(QPen(lineColor, lineWidth, Qt::SolidLine, 
			  Qt::SquareCap, Qt::MiterJoin));
	    p.drawLine (sx1, sy1, sx1, sy2);
	    p.drawLine (sx1, sy1, sx2, sy1);
	    p.drawLine (sx2, sy1, sx2, sy2);
	    p.drawLine (sx1, sy2, sx2, sy2);
	}

	// Fill in interior of box, if fill color is valid
	// To verify filling works right, do after drawing lines...
	if (fillColor.isValid())
	{
	    // Get actual lineWidth
	    int lw = lineWidth;

	    // Force lineWidth for this calculation to be at least 0
	    if (lw < 0)
		lw = 0;

	    // Find the top, left corner of the portion of the box to fill.
	    // Also calculate the bottom, right corner so can calculate
	    // the height and width.  Use layout routines to figure
	    // this out for clarity and accuracy (if slightly slower).
	    int x1, x2, y1, y2;

	    // Pick points so x1 is always to the left (< x2)
	    // Use lw (lineWidth) to find start/end points, so that
	    // 0 linewidth rectangles are filled properly.
	    if (sx1 < sx2)
	    {
		// Start rectangle a line width to the right of the 
		// leftmost pixel of the leftmost line
		x1 = layout.calcLeftX (index1, slot1, posFlags1, lineWidth) 
		    + lw;

		// End rectangle a line width to the left of the 
		// rightmost pixel of the rightmost line
		x2 = layout.calcRightX (index2, slot2, posFlags2, lineWidth) 
		    - lw;
	    }
	    else
	    {
		// Start rectangle a line width to the right of the 
		// leftmost pixel of the leftmost line
		x1 = layout.calcLeftX (index2, slot2, posFlags2, lineWidth) 
		    + lw;

		// End rectangle a line width to the left of the 
		// rightmost pixel of the rightmost line
		x2 = layout.calcRightX (index1, slot1, posFlags1, lineWidth) 
		    - lw;
	    }


	    // Pick points so y1 is always on top (< y2)
	    // Use lw (lineWidth) to find start/end points, so that
	    // 0 linewidth rectangles are filled properly.
	    if (sy1 < sy2)
	    {
		// Start rectangle a line width below the topmost pixel
		// of the topmost line
		y1 = layout.calcTopY (posFlags1, lineWidth) + lw;

		// Stop rectangle a line width above the bottommost pixel
		// of the bottommost line
		y2 = layout.calcBottomY (posFlags2, lineWidth) - lw;
	    }
	    else
	    {
		// Start rectangle a line width below the topmost pixel
		// of the topmost line
		y1 = layout.calcTopY (posFlags2, lineWidth) + lw;

		// Stop rectangle a line width above the bottommost pixel
		// of the bottommost line
		y2 = layout.calcBottomY (posFlags1, lineWidth) - lw;
	    }

	    // Use corner coordinates to calc width and height, 
	    // Need to add 1 for the 0th pixel
	    int w = (x2 - x1) + 1;
	    int h = (y2 - y1) + 1;

#if 0
	    // DEBUG
	    printf ("%p: x1=%i x2=%i w=%i y1=%i y2=%i h=%i linewidth=%i, "
		    "cell x=%i y=%i w=%i h=%i, sy1=%i sx1=%i\n",
		    this, x1, x2, w, y1, y2, h, lineWidth, 
		    layout.getX(), layout.getY(), layout.getW(), 
		    layout.getH(), sy1, sx1);
#endif

	    // Only fill if w > 0 and h > 0
	    if ((w > 0) && (h > 0))
		p.fillRect(x1, y1, w, h, fillColor);
	}



    }

    //! When called, return TRUE if annotation clickable at index/slot
    //! Return FALSE if annotation not clickable, or not at location
    int clickableAt (int index, int slot)
    {
	// If is clickable and location match, return true
	if (isClickable && 
	    ((index > index1) || ((index == index1) && (slot >= slot1))) &&
	    ((index < index2) || ((index == index2) && (slot <= slot2))))
	    return (TRUE);
	else
	    return (FALSE);
    }


private:
    int index1;
    int slot1;
    int posFlags1;
    int index2;
    int slot2;
    int posFlags2;
    bool isClickable;
    QColor lineColor;
    int lineWidth;
    QColor fillColor;
};

//! Annotation for a Ellipse inside a cell
class CellEllipseAnnot : public CellAnnot {
public:
    //! Create ellipse annotation
    CellEllipseAnnot (int handle, int layer, bool clickable, 
		   int startIndex, int startSlot, int startPosFlags, 
		   int stopIndex, int stopSlot, int stopPosFlags,
		   const QColor &boxColor, int width, 
		   const QColor &insideColor) :
	CellAnnot(handle, layer), 
	index1(startIndex), slot1(startSlot), posFlags1(startPosFlags), 
	index2(stopIndex), slot2(stopSlot), posFlags2(stopPosFlags), 
	isClickable (clickable), 
	lineColor(boxColor), lineWidth(width), fillColor(insideColor)
    {};

    //! Require a slot at the position specified as least as width of line
    void requirements (CellLayout &layout)
    {
	// Assume line width sets required width for now.
	// That is, assume horizonal line end points require line width
	// in the start and stop slot.
	layout.slotRequired (index1, slot1, lineWidth);
	layout.slotRequired (index2, slot2, lineWidth);
    }

    //! Draw ellipse and/or fill ellipse with fill color 
    //! (unless colors are invalid, which doesn't draw anything for that 
    //! portion but still is clickable and does reserve space)
    void draw (QPainter &p, CellLayout &layout)
    {
	// This routine is more complicated than originally expected
	// due to the following:
	//
	// 1) QPainter's drawEllipse filler sometimes misses pixels!
        //    This looks to be a X11 issue and it is mainly with small
	//    or narrow ellipses and when lineWidth == 1.  So there is 
	//    complexity trying to overcome these problems, since being able
	//    to draw small/narrow ellipses with lineWidth == 1 is desired.
	//    The algorithm essentally fills the ellipse twice, in two 
	//    different ways, which appears to mostly fix the problem.  
	//    I also sometimes fill with lines, for really narrow ellipses.
	// 
	// 2) I want to be able to do a 'fill' only (invalid lineColor) that
	//    hits (approximately) only the pixels that would be filled
	//    if the ellipse outline was drawn (without drawing it).  
	//    Telling drawEllipse to not draw the outline doesn't yield 
	//    the desired results (ignores lineWidth when filling).
	//    
        // 3) If the box coordinates specify a line, I want to draw a 
	//    line of width lineWidth (with rounded ends, if possible). 
	//    QPainter's drawEllipse draws nothing if h < 2 or w < 2.  
	//    (If I don't do this, ellipses can disappear if the lineWidth
	//     gets too big or the ellipse gets too relatively small!)

	// Don't actually draw ellipse if both line and fill colors are 
	// invalid.  Allows 'transparent' clickable boxes to be put on display.
	// Treat lineWidth < 1 as invalid line color.
	if ((!lineColor.isValid() || lineWidth < 1) && 
	    !fillColor.isValid())
	    return;

	// Calculate x,y location of the 'centered' starting point for 
	// the elipse.  QPainter's drawEllipse appears to want the center 
	// points of the ellipse's line, so the 'center' point is what we want
	int sx1 = layout.calcCenterX (index1, slot1, posFlags1, lineWidth);
	int sy1 = layout.calcCenterY (posFlags1, lineWidth);

	// Calculate x,y location of the 'centered' ending point for line.
	int sx2 = layout.calcCenterX (index2, slot2, posFlags2, lineWidth);
	int sy2 = layout.calcCenterY (posFlags2, lineWidth);

	
	// Fill in interior of ellipse, if fill color is valid
	// Need to do before outline due to the rounding errors 
	// involved in drawing ellipses.
	if (fillColor.isValid())
	{
	    // Get actual lineWidth
	    int lw = lineWidth;

	    // Force lineWidth for this calculation to be at least 0
	    if (lw < 0)
		lw = 0;

	    // Find the top, left corner of the portion of the ellipse to fill.
	    // Also calculate the bottom, right corner so can calculate
	    // the height and width.  Use layout routines to figure
	    // this out for clarity and accuracy (if slightly slower).
	    int x1, x2, y1, y2;

	    // Pick points so x1 is always to the left (< x2)
	    // Use lw (lineWidth) to find start/end points, so that
	    // 0 linewidth ellipses are filled properly.
	    if (sx1 < sx2)
	    {
		// Start ellipse a line width to the right of the 
		// leftmost pixel of the leftmost line
		x1 = layout.calcLeftX (index1, slot1, posFlags1, lineWidth) 
		    + lw;

		// End ellipse a line width to the left of the 
		// rightmost pixel of the rightmost line
		x2 = layout.calcRightX (index2, slot2, posFlags2, lineWidth) 
		    - lw;
	    }
	    else
	    {
		// Start ellipse a line width to the right of the 
		// leftmost pixel of the leftmost line
		x1 = layout.calcLeftX (index2, slot2, posFlags2, lineWidth) 
		    + lw;

		// End ellipse a line width to the left of the 
		// rightmost pixel of the rightmost line
		x2 = layout.calcRightX (index1, slot1, posFlags1, lineWidth) 
		    - lw;
	    }


	    // Pick points so y1 is always on top (< y2)
	    // Use lw (lineWidth) to find start/end points, so that
	    // 0 linewidth ellipses are filled properly.
	    if (sy1 < sy2)
	    {
		// Start ellipse a line width below the topmost pixel
		// of the topmost line
		y1 = layout.calcTopY (posFlags1, lineWidth) + lw;

		// Stop ellipse a line width above the bottommost pixel
		// of the bottommost line
		y2 = layout.calcBottomY (posFlags2, lineWidth) - lw;
	    }
	    else
	    {
		// Start ellipse a line width below the topmost pixel
		// of the topmost line
		y1 = layout.calcTopY (posFlags2, lineWidth) + lw;

		// Stop ellipse a line width above the bottommost pixel
		// of the bottommost line
		y2 = layout.calcBottomY (posFlags1, lineWidth) - lw;
	    }

	    // Use corner coordinates to calc width and height, 
	    // Need to add 1 for the 0th pixel
	    int w = (x2 - x1) + 1;
	    int h = (y2 - y1) + 1;

	    // Only fill if w > 0 and h > 0 (only if something to fill)
	    if ((w > 0) && (h > 0))
	    {
		// Set both the fill and line color to the fillColor
		// So actually drawning a smaller filled ellipse inside
		// the outline to get the fill.  As mentioned above,
		// this lets the fill be done separately from the outline
		// and still get the correct image.
		p.setBrush(QBrush(fillColor));
		p.setPen(QPen(fillColor, 1, Qt::SolidLine, 
			      Qt::SquareCap, Qt::MiterJoin));
#if 0
		//DEBUG
		printf ("Filling ellipse (x %i, y %i, w %i, h %i)\n",
			x1, y1, w, h);
#endif
		p.drawEllipse(x1, y1, w, h);

		// For narrow ellipses, connect mid points to fill in missing
		// pixels that occur with width 1 lines
		// Note: drawEllipse will do nothing if w == 1 or h == 1,
		//       and I want something to appear for this case!
		if ((w < 4) || (h < 4))
		{
		    // Draw 1 pixel lines to fill in the pixels normal 
		    // filling misses
		    p.drawLine(x1 + (w/2), y1, x1 + (w/2), y1 + h - 1);
		    p.drawLine(x1, y1+(h/2), x1 +w - 1, y1 + (h/2));
		}

		// If not drawing outline, need to refill filled zone
		// to handle missed pixels.  The outline ellipse fill
		// normally catches these pixels, so don't need to
		// do if going to do a fill below.
		if (!lineColor.isValid() || (lineWidth <= 0))
		{
		    p.drawEllipse(x1+1, y1+1, w-2, h-2);
		}
	    }
	}

	// Draw ellipse outline at calculated start and end points
	// If line color is valid and lineWidth > 0.
	// 
	// Note:  If fillColor is valid, I will also fill the ellipse below
	//        (even though the above algorithm should do it).  This is
	//        to catch the pixels the above fill misses.  The above fill
	//        is done to catch the pixels the fill below misses.  So,
	//        yes, it is necessary to fill twice.
	//    
	// Do after filling above, due to the rounding errors involved in
	// drawing ellipses, the fill may hit some of the pixels of the outline
	if (lineColor.isValid() && (lineWidth > 0))
	{
	    // Find the top, left corner of the box the ellipse will
	    // be drawn in, and the height/width of this box.
	    int x1, y1, w, h;

	    if (sx1 < sx2)
	    {
		x1 = sx1;
		w = (sx2 - sx1) + 1;
	    }
	    else
	    {
		x1 = sx2;
		w = (sx1 - sx2) + 1;
	    }

	    if (sy1 < sy2)
	    {
		y1 = sy1;
		h = (sy2 - sy1) + 1;
	    }
	    else
	    {
		y1 = sy2;
		h = (sy1 - sy2) + 1;
	    }

	    // If have valid fill color, fill ellipse with it
	    if (fillColor.isValid())
		p.setBrush(QBrush(fillColor));

	    // Otherwise, draw outline only
	    else
		p.setBrush(QBrush());


	    // Use RoundCap in case drawing line instead of ellipse
	    p.setPen(QPen(lineColor, lineWidth, Qt::SolidLine, 
			  Qt::RoundCap, Qt::MiterJoin));
#if 0
	    // DEBUG
	    printf ("Drawing ellipse outline (x %i, y %i, w %i, h %i)\n",
		    x1, y1, w, h);
#endif

	    // Handle degenerate cases where h <= 1 or w <= 1, just
	    // draw line with round endcaps between endpoints (drawEllipse 
	    // does nothing in this case and I want *something* to appear)
	    if ((h <= 1) || (w <= 1))
	    {
		p.drawLine (sx1, sy1, sx2, sy2);
	    }
	    // Otherwise, draw actual ellipse
	    else
	    {
		p.drawEllipse (x1, y1, w, h);
	    }
	}
    }

    //! When called, return TRUE if annotation clickable at index/slot
    //! Return FALSE if annotation not clickable, or not at location
    int clickableAt (int index, int slot)
    {
	// If is clickable and location match, return true
	if (isClickable && 
	    ((index > index1) || ((index == index1) && (slot >= slot1))) &&
	    ((index < index2) || ((index == index2) && (slot <= slot2))))
	    return (TRUE);
	else
	    return (FALSE);
    }


private:
    int index1;
    int slot1;
    int posFlags1;
    int index2;
    int slot2;
    int posFlags2;
    bool isClickable;
    QColor lineColor;
    int lineWidth;
    QColor fillColor;
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

