// tgchart_dev.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
#include "tgchart_dev.h"

TgChart_dev::TgChart_dev(
        QWidget* parent,
        const char* name,
        WFlags f,
        TgChartData *iData):
      QFrame( parent, name , f)
{
  // Check to see if they gave us some data to work with
  if(iData == NULL)
  {
    // Nope - Allocate a new data object
    data = new TgChartData;
  } else {
    // Use the data they gave us
    data = iData;
  }
  
  // Allocate variables
  p = new QPainter();
  buffer = new QPixmap();

  // Initialize default settings
  border_width = 15;
}

TgChart_dev::~TgChart_dev()
{
  // Nothing to see here yet
  delete buffer;
  delete p;
}


/*
 * When we resize, re-plot the graph
 */
void TgChart_dev::resizeEvent(QResizeEvent *e)
{
  qDebug("Resize event.");
  refreshPixmap(e->size().width(),e->size().height());
}


/*
 * Slot for drawing the contents of the frame, for now we will just
 * ignore the fact that we are in a frame and draw our pixmap.
 */
void TgChart_dev::drawContents(QPainter * paint)
{
  qDebug("DrawContents event.");
  paint->drawPixmap(0,0,*buffer);
}

int transform_y(float point, float scale, float offset, int reverse)
{
  return (int)(reverse - ((point - offset) * scale));
}

void TgChart_dev::drawAxis(QPainter *p, double min, double max)
{
  // Get the width and height of this viewport
  int width = p->viewport().width();
  int height = p->viewport().height();

  // Figure out the max width and max height of text
  int maxWidth = 0;
  // Max height will always be the same.
  int maxHeight = p->boundingRect(0,0,AlignAuto,width,height,"123").height();
  int tmp; 
  double yscale = height / (max - min);
  // TODO: change from const 5 to changeable 5. This is minimum spacing.
  double spacing = (maxHeight + 5)  / yscale;
  char str[32];
  for(float i = min + spacing ; i < max - spacing; i += spacing)
  {
    //sprintf(str,"%.1f",i);
    sprintf(str,"%g",i);
    tmp = p->boundingRect(0,0,AlignAuto,width,height,str).width();
    p->drawText(0, transform_y(i, yscale, min, height) + maxHeight / 2, str);
    printf("Num: %f\t width: %i\n",i,tmp);
    if(tmp > maxWidth) maxWidth = tmp;
  }

  // Draw the y-axis
  p->drawLine(maxWidth + 4,0,maxWidth + 4,p->viewport().height());

  // Draw the x-axis
  p->drawLine(maxWidth + 4,height,width,height);
  
  p->save();
  p->setPen(QColor("lightgrey"));
  // Draw the horizontal gridlines
  for(float i = min + spacing; i < max - spacing; i += spacing)
  {
    p->drawLine(
      maxWidth + 4,
      transform_y(i, yscale, min, height),
      width ,
      transform_y(i, yscale, min, height));
  }
  p->restore();
  
  printf("Max width: %i\n", maxWidth);

  // Set up the smaller viewport for future operations
  p->setViewport(
     p->viewport().left() + maxWidth + 4,
     p->viewport().top(),
     p->viewport().width() - maxWidth - 4,
     p->viewport().height());
}

void TgChart_dev::drawSet(QPainter *p, int ID, double min, double max)
{
  int width = p->viewport().width();
  int height = p->viewport().height();
  int n = data->getSet(ID).size();
  int xspacing = width / (n - 1);
  double yscale = height / (max - min);
  QPen *pen =  new QPen();
  pen->setColor(lineColor[ID]);
  if (lineStyle[ID])
    pen->setStyle(lineStyle[ID]);
  p->setPen( *pen );
  delete pen;
  for(int i = 1; i < n; ++i)
  {
    p->drawLine(
      (i-1) * xspacing,
      transform_y(data->getSet(ID)[i-1], yscale, min, height),
      i * xspacing,
      transform_y(data->getSet(ID)[i], yscale, min, height));
  }

}

void TgChart_dev::drawBorder(QPainter *p)
{
  int width = p->viewport().width();
  int height = p->viewport().height();
  p->drawRect(0, 0, width, height);
  //p->setWindow(5, 5, width-5, height - 5);
  p->setViewport(
     p->viewport().left() + 5,
     p->viewport().top() + 5,
     width - 5,
     height - 5);
  p->setPen(Qt::blue);
  p->drawRect(0, 0, p->viewport().width(), p->viewport().height());
}

void TgChart_dev::refreshPixmap(short imagewidth, short imageheight)
{
  qDebug("refreshPixmap");

  // First things first, we need to set up the buffer.
  // Set the imagesize of the buffer and fill it with white
  buffer->resize(imagewidth,imageheight);
  buffer->fill(Qt::white);

  // Reset the pixmap, attach it to buffer
  if(p->isActive()) p->end(); // Make sure the pixmap isn't active (should't be)
  p->begin(buffer);


  // Roughly we'll want to do it like this:
  //   1. Draw the axises and labels
  //   2. Draw the sets
  // intuitive, eh?
  
  // Save the state of the painter.
  p->save();

  // Subtract the border from our current drawing area
  p->setViewport(border_width,
    border_width,
    imagewidth - border_width,
    imageheight - border_width);

  if(data->min() < data->max())
  {
    drawAxis(p, data->min(), data->max());

    // Draw every data set on the current viewport
    for(int i = 0; i < data->numSets(); ++i)
    {
      int curID = data->getID(i);
      drawSet(p, curID, data->min(), data->max());
    }
  }
  
  // Restore the saved state
  p->restore();

  // Now we are done we need to reset the pixmap
  p->end();

  // Update changes
  update();
}


// Include the QT-extension of tgchart.h
//#include "tgchart_dev.moc"


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

