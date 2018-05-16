//! \file tgchart_dev.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/*! Implements the view portion of a chart. See also TgChartData
 *  Not currently used.
 * 
 * HISTORY / NOTES
 *  - 2002.03.07.08.12
 *    - Changed data into a pointer. Begin integration with the rest of ToolGear
 *    - I'm trying to decide where data goes. The raw points obviously should
 *      be stored in TgChartData, but what about colors and set labels?
 *
 *  - 2002.02.28.22.13
 *    - Complete re-write has begun
 *
 *  - 2001.12.02.23.23
 *    - After having some trouble with the below idea, the data is now
 *      split off into its own class, TgChartData. This will also allow for a
 *      cleaner re-implementation if need be
 *    - Now all data sets have an ID they can be referenced by. See the
 *      TgChartData documentation for details
 *    - Fixed alignment problem on labels (and made them work at all).
 *    - Beginning of the end for the monstorous refreshPixmap. Started to
 *      break it up into its appropriate pieces
 *    - Tweaked documentation a bit. Note the "more..." after every method
 *      is long gone
 *    - Discovered that there is a problem instantiating multiple charts.
 *      Fixing this I'm sure as always will involve fixing something in
 *      refreshPixmap or in the variable allocation
 *
 *  - 2001.11.03.23.05
 *    - Now using QMap<int,DoubleVector> to hold the vectors
 *
 *  - 2001.08.08.14.11
 *    - Even more documentation added
 *    - Thumbnail = TRUE is equivelent to [grid = FALSE,
 *      xaxis = FALSE, yaxis = FALSE]
 *    - Now uses ValueVectors internally
 *    - Now uses Doubles instead of Floats
 *    - Inherits from QFrame
 */

// We only want this to be included once.
#ifndef TGCHART_H
#define TGCHART_H

#include <qframe.h>
#include <qpainter.h>
#include <qpixmap.h>
#include "tgchartdata.h"

class TgChart_dev : public QFrame
{
  // This is a QT object
  Q_OBJECT
  
  public:
    
    //! Create a new TgChart_dev widget, just like other widgets
    TgChart_dev(
        QWidget* parent = 0,
        const char* name = 0,
        WFlags f = 0,
        TgChartData *iData = NULL);

/*TgChart_dev::TgChart_dev(
        QWidget* parent = 0,
        const char* name = 0,
        WFlags f = 0):
      QFrame( parent, name , f) {} */

    //! Destory the TgChart_dev
    ~TgChart_dev();

    //! The hidious refreshPixmap updates the graph
    void refreshPixmap(
        short imagewidth,
        short imageheight);
    
    // -- The following few are just wrappers for accessing the data sets --
    // -- See TgChartData class                                           --
    
    //! Add a set of data from an array of double, given an ID (overwrites)
    void addSet(int id, double *set, int size)
      { data->addSet(id, set, size); }

    void setLineColor(int id, QColor color)
      { lineColor[id] = color; }

    void setLineStyle(int id, PenStyle style)
      { lineStyle[id] = style; }
      
    //! Clear all data (might change name to clearData)
    void emptyData()
      { data->clear(); }

    //! For debugging - print out data
    void printData()
      { data->print(); }
    
    //! Get the ID of a set
    int getSetID(int set)
      { return data->getID(set); }

    //! Append an array of doubles as the last dataset
    void appendSet(double *set, int size)
      { data->appendSet(set,size); }

    //! Remove a dataset, given an ID
    void removeSet(int id)
      { data->removeSet(id); }

    //! Append a singe data point
    void appendPoint(int id, double point)
      { data->appendPoint(id, point);
       /* printf("  width: %i\n  height: %i\n",
          frameRect().width(),
          frameRect().height()); */
        refreshPixmap(
          frameRect().width(),
          frameRect().height());
    }

    //! Get the number of data sets
    int numSets()
      { return data->numSets(); }

    //! Get a pointer to the data set (a TgChartData object)
    // This isn't needed by the world, is it?
    TgChartData * getData()
      { return data; }

    //! Set the whole data set (given a TgChartData object)
    void setData(TgChartData  * _data)
      { data = _data; }

    // -- End of wrappers --

  private:
  
    //! map from ID -> double (for things like max/min)
    typedef QMap<int,QColor> Map_id_color;
    
    //! map from ID -> double (for things like max/min)
    typedef QMap<int,PenStyle> Map_id_style;

    //! Hold the data points
    TgChartData *data;
    Map_id_color lineColor;
    Map_id_style lineStyle;

    //! This is used to paint on the buffer
    QPainter *p;

    //! This holds the currently displayed chart
    QPixmap *buffer;

    //! Border between the edge of the frame and things
    //! that are actually on the chart
    int border_width;
    
    void drawSet(QPainter *p, int ID, double min, double max);
    void drawBorder(QPainter *p);
    void drawAxis(QPainter *p, double min, double max);

  protected:
  
    //! Resize the window!
    virtual void resizeEvent(QResizeEvent *);

    //! Draw the contents of the buffer
    virtual void drawContents(QPainter * p);

};

#endif

// End of tgchart_dev.h

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

