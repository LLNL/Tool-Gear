//! \file tgchartdata.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

/*! Implements the model for holding chart data
 * Not currently used.
 *
 * In the way of the ancients, this is a model-view-controler situation. The
 * only thing is that there is no real controler, so its more of a model-view.
 * But thats okay. This class is the model part, holding data for the chart.
 *
 * There are several things that we need to keep track of. First and foremost
 * are the data points. We will do this by having a "set" represent a list of
 * points, and then keep a list of these sets. Every set has an identifier (ID)
 * which is just a number. The numbers are important, however, since they are
 * graphed in order from least to greatest.
 *
 * Every set also has a list of attributes associated with it. These include
 * things like color, max, min, and style. Some of these attributes, such as
 * color and style, are manually assigned, while others, like max and min, are
 * actually calculated depending on the values present in the set.
 *
 * On Second thought, some of those attributes should be kept by the view, not
 * by the model. I'll have to contemplate this some more.
 *
 * Finally there are some attributes which are important on a global scale.
 * This includes the total max/min of the all the sets.
 *
 * HISTORY / NOTES
 *  - 2002.02.28.21.57
 *    - Created this section
 *    - Re-working and commenting. Now creating model-view architecture
 */


#ifndef TGCHARTDATA_H
#define TGCHARTDATA_H

#include <stdio.h>
#include <qvaluevector.h>
#include <qmap.h>
#include <qcolor.h>

//! Provides dataset management for TgChart
class TgChartData
{
  public:
    TgChartData()
    {
      lastID = 0;
    }

    //! Vector contains individual points
    typedef QValueVector<double> Vector;

    //! Not used at this point
    typedef struct _dataset
    {
      Vector points;
      char *label;
      double min;
      double max;
      QColor *color;
    } Dataset;

    //! map from ID -> Dataset (not used yet)
    typedef QMap<int,Dataset> Map_id_dataset;

    //! map from ID -> data set
    typedef QMap<int,Vector> Map_id_vector;

    //! map from ID -> double (for things like max/min)
    typedef QMap<int,double> Map_id_double;

    //! map from ID -> string (for labels)
    typedef QMap<int,char*> Map_id_string;

    //! Get the ID of the nth set
    int getID(int set);

    //! Add a set, given a Vector
    void addSet(int id, Vector s);

    //! Get a set
    Vector getSet(int id)
    { return data[id]; }

    //! Add a set, given an array of doubles and a length
    void addSet(int id, double *set, int size);

    //! Append an array of doubles as the last dataset
    void appendSet(double *set, int size);

    //! Append a singe data point
    void appendPoint(int id, double point);

    //! Remove a dataset, given an ID
    void removeSet(int id);

    //! Get the max of a set
    double getSetMax(int id);
    //! Get the min of a set
    double getSetMin(int id);

    //! Get the ovarall min
    double min();
    //! Get the overall max
    double max();

    //! Clear all the data
    void clear();

    //! Get the number of data sets
    int numSets();

    //! Access the nth set (sorted by ID)
    /*!
     * Access the vector by offset
     *
     * \todo It really annoys me that this must be put in the .h file!
     */
    Vector & operator[] (int offset)
    {
      Map_id_vector::Iterator it = data.begin();
      int num_sets = data.count();
      int count = 0;
      if (offset > num_sets)
      {
        // I haven't decided what kind of punishment to give for
        // out-of-bounds access yet.
        printf("ERROR: cannot access [%i], there are only %i sets.\n",
          offset,num_sets);
        return it.data();
      } else {
        while ((it != data.end()) && (count < offset))
        {
          ++it;
          ++count;
        }
        return it.data();
      }
    }
    
    void print();

  private:

    // Map_id_dataset data;
    
    
    Map_id_vector data; // Hold the actual data sets
    Map_id_double set_max; // Hold the max of each set
    Map_id_double set_min; // Hold the min of each set
    Map_id_string labels; // Hold the labels
    

    int lastID; // what is the last (highest) ID in use?
};

#endif

// End of tgchartdata.h

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

