// tgchartdata.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
#include "tgchartdata.h"

/*
 * Adds a set using Vector.
 */
void TgChartData::addSet(int id, Vector s)
{
  
  // Insert the data
  data.insert(id,s);
  // Is this now the biggest ID?
  if (id > lastID) lastID = id;
}


/*
 * Add a set using an array of Type. This array will be converted to a
 * Vector and is here mostly for convenience to users of TgChartData.
 */
void TgChartData::addSet(int id, double *set, int size)
{
  //First we make a value-vectory by copying this double[]
  Vector v(size);
  for(int i=0; i<size; i++)
  {
    v[i] = set[i];
  }
  // Then we add this the other way
  data[id] = v; // or addSet(id,v);
  if (id > lastID) lastID = id;
}

int TgChartData::getID(int set)
{
  Map_id_vector::Iterator it = data.begin();
  int num_sets = data.count();
  int count = 0;
  if (set > num_sets)
  {
    // I haven't decided what kind of punishment to give for
    // out-of-bounds access yet.
    printf("ERROR: cannot access [%i], there are only %i sets.\n",
      set,num_sets);
    return it.key();
  } else {
    while ((it != data.end()) && (count < set))
    {
      ++it;
      ++count;
    }
    return it.key();
  }
}
    

void TgChartData::appendSet(double *set, int size)
{
  // int setnum = getID(num_sets - 1) + 1;
  // Add a set, using the pre-incremented lastID as the ID
  addSet(++lastID, set, size);
}


/* Remove a data set.
 */
void TgChartData::removeSet(int id)
{
  data.remove(id);
}

void TgChartData::appendPoint(int id, double point)
{
  data[id].push_back(point);
}

double TgChartData::getSetMax(int id)
{
  double max = 0;
  Vector v = data[id];
  Vector::iterator it = v.begin();
  if (it != v.end()) max = (*it);

  for (; it != v.end() ; ++it)
    if ((*it) > max) max = (*it);

  return max;
}

double TgChartData::getSetMin(int id)
{
  double min = 0;
  Vector v = data[id];
  Vector::iterator it = v.begin();
  if (it != v.end()) min = (*it);
  for (; it != v.end() ; ++it)
    if ((*it) < min) min = (*it);
  return min;
}

double TgChartData::max()
{
  double max = 0;
  Map_id_vector::iterator it;
  it = data.begin();
  if (it != data.end()) max = getSetMax(it.key());
  for(; it != data.end(); ++it)
    if (max < getSetMax(it.key())) max = getSetMax(it.key());
  return max;
}
    

double TgChartData::min()
{
  double min = 0;
  Map_id_vector::iterator it;
  it = data.begin();
  if (it != data.end()) min = getSetMin(it.key());
  for(; it != data.end(); ++it)
    if (min > getSetMin(it.key())) min = getSetMin(it.key());
  return min;
}
    
void TgChartData::clear()
{
  data.clear();
  lastID = 0;
}

int TgChartData::numSets()
{
  return data.count();
}


void TgChartData::print()
{
  Map_id_vector::iterator it;
  printf("num_sets: %i\n",data.count());
  printf("max: %f\tmin: %f\n", max(), min());
  printf("Current data:\n");
  for(it = data.begin(); it != data.end(); ++it)
  {
    printf("ID: %i\tMax: %f\tMin: %f\n",
      it.key(),
      getSetMax(it.key()),
      getSetMin(it.key()));
    for(unsigned int j = 0; j < it.data().size(); ++j)
      printf("%i,%i:\t%f\n",it.key(),j,it.data()[j]);
  }
  printf("\n");
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

