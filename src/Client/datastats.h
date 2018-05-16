//! \file datastats.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** Data Stats class designed to efficiently as possible calculate the sum, 
** sum of squares (for std dev calculations), min, max of possibly 
** thousands of data values, which may be changing one at a time.
** 
** Tries to minimizes number of scans over all the data by using old values
** to update the sums and caching the top two mins/maxes and inferring
** as much about the mins/maxes as possible by using data ids.  
** If mins/maxes become out of date, stops updating them and flags at 
** next read that they are out of date (i.e., with NULL_INT, NULL_DOUBLE).  
** The caller then needs to reset and rescan in the dataset.
** The value to return when out of date must be defined for each data type.
**
** Preliminary target is UIManager (uimanager.h) but perhaps will
** be useful to others.  Look at how UIManager uses this for clues to
** how to use it. :)
**
** Designed by John Gyllenhaal at LLNL
** Initial implementation by John Gyllenhaal 4/22/02
**
*****************************************************************************/

#ifndef DATASTATS_H
#define DATASTATS_H

#include "tg_types.h"
//! Data Stats class designed to efficiently as possible calculate the sum, 
//! sum of squares (for std dev calculations), min, max of possibly 
//! thousands of data values, which may be changing one at a time.
//! 
//! Tries to minimizes number of scans over all the data by using old values
//! to update the sums and caching the top two mins/maxes and inferring
//! as much about the mins/maxes as possible by using data ids.  
//! If mins/maxes become out of date, stops updating them and flags at 
//! next read that they are out of date (i.e., with NULL_INT, NULL_DOUBLE).  
//! The caller then needs to reset and rescan in the dataset.
//! The value to return when out of date must be defined for each data type.
//!
//! NULL_INT is not a valid id.  NULL_INT used is internally as special ids.
//!
//! Warning: The sum of squares calculation overflows rapidly for ints.
//!          I recommend using double type even for ints if planning
//!          on using the "sum of squares" value or if the even the
//!          "sum" is likely to overflow an int.

template<class DataType>
class DataStats 
{
public:
    
    //! Quick class initialization for quick creation
    DataStats() : numData(0) {}
    ~DataStats() {}
    
    //! Resets stat to clean slate.  
    void resetStats() {numData = 0;}
    
    //! Call for adding a new data value or for updating an existing data
    //! value (with id).
    //! 
    //! Each data value must have a unique id (such as the data value's index),
    //! and this id will be returned with min/max queries.  This id will also
    //! be used to break ties for min/max (lowest id wins).  
    //! 
    //! If the 'update' is TRUE, oldValue must be the previous value for
    //! this data point that was passed to updateData (if it is different,
    //! or the id is wrong, or if this data point was not passed to 
    //! updateStats earlier, you will get garbage stats).  If update is FALSE,
    //! the value of oldValue does not matter.
    //!
    //! This class only maintains aggregate data and makes inferences based
    //! on what is passed in.  Garbage in will yield garbage out!
    //!
    //! With certain sequences of data updates, the min and/or max values
    //! may become unknown (a query of min/max will reveal this).  
    //! In this case, a clean rescan (reset() then updateStats() for
    //! all data with update == FALSE) is needed then to rebuild min/max.
    //! If a rescan is necessary, save all the old valid data (such as sums)
    //! and compare them to the new values.  If they differ, there is
    //! a problem!
    void updateStats(int id, DataType value, bool update, DataType oldValue);

    //! Call for invalidating sum, sumOfSquares, and perhaps min/max based
    //! on the id and value.  To be called when the exact value to be
    //! processed is not known (i.e., stats on max when max needs rebuilding
    //! to be known).  Updates count, invalidates sums, and if id or value
    //! is such that max/min needs invalidating, it does that also.
    void invalidateStats (int id, DataType value, bool update);

    //! Returns the number of data values these stats covers (may be 0)
    int count() {return (numData);}
    
    //! Returns the sum, or errorValue if no data has been processed
    DataType sum () 
	{if (numData > 0) return (dataSum); else return (errorValue);}

    //! Returns the sum of squares, or errorValue if no data has been processed
    DataType sumOfSquares () 
	{if (numData > 0) return (dataSumOfSquares); else return (errorValue);}

    //! Returns the max value, or errorValue if no data has been processed
    //! or if the update pattern was such that the max is not known.
    //! In this case, do a reset() and scan thru the data with updateStats()
    //! with update == FALSE, in order to generate a known max value.
    DataType max () 
	{ if ((numData > 0) && (maxId1 != NULL_INT)) 
	    return (max1); 
 	  else 
	    return (errorValue);
	}

    //! Returns the id of the max value, or NULL_INT if no data has been
    //! processed or the max is not currently know.  See max() for details.
    int maxId () { if (numData > 0) return (maxId1); else return (NULL_INT);}

    //! Returns the min value, or errorValue if no data has been processed
    //! or if the update pattern was such that the min is not known.
    //! In this case, do a reset() and scan thru the data with updateStats()
    //! with update == FALSE, in order to generate a known min value.
    DataType min () 
	{ if ((numData > 0) && (minId1 != NULL_INT)) 
	    return (min1); 
 	  else 
	    return (errorValue);
	}

    //! Returns the id of the min value, or NULL_INT if no data has been
    //! processed or the min is not currently know.  See min() for details.
    int minId () { if (numData > 0) return (minId1); else return (NULL_INT);}
    

private:
    DataType dataSum;          	// Sum of all the data elements
    DataType dataSumOfSquares; 	// Sum of the square of the data elements
    DataType max1;	      	// Max of all the data elements
    DataType max2;		// 2nd highest value of all data elements
    DataType min1;		// Min of all the data elements
    DataType min2;		// 2nd lowest value of all the data elements
    int maxId1;                 // Id of max1, NULL_INT if not valid
    int maxId2;                 // Id of max2, NULL_INT if not valid
    int minId1;                 // Id of min1, NULL_INT if not valid
    int minId2;                 // Id of min2, NULL_INT if not valid
    int numData;          	// Number of unique data elements processed

    //! This value is statically defined once per DataType.
    //! The first instantiation for a datatype needs to define
    //! something like this (i.e., if DataType is double) in their code 
    //! to make the program link:
    //!
    //! const double DataStats<double>::errorValue = NULL_DOUBLE;
    //!
    static const DataType errorValue;
};

//! Enhancement of DataStats with an id array (of size numIds) per entry
//! See DataStats for functionality details.
template<class DataType, int numIds>
class DataStatsIdArray
{
public:
    
    //! Quick class initialization for quick creation
    DataStatsIdArray() : numData(0) {}
    ~DataStatsIdArray() {}
    
    //! Resets stat to clean slate.  
    void resetStats() {numData = 0;}
    
    //! Call for adding a new data value or for updating an existing data
    //! value (with ids).
    //! 
    //! Each data value must have a unique set of id (such as the data 
    //! value's index plus table number), and these ids will be returned 
    //! with min/max queries.  These ids will also be used to break ties 
    //! for min/max (lowest id1 wins, id2 used by break ties (lowest wins)).  
    //! 
    //! If the 'update' is TRUE, oldValue must be the previous value for
    //! this data point that was passed to updateData (if it is different,
    //! or the ids are wrong, or if this data point was not passed to 
    //! updateStats earlier, you will get garbage stats).  If update is FALSE,
    //! the value of oldValue does not matter.
    //!
    //! This class only maintains aggregate data and makes inferences based
    //! on what is passed in.  Garbage in will yield garbage out!
    //!
    //! With certain sequences of data updates, the min and/or max values
    //! may become unknown (a query of min/max will reveal this).  
    //! In this case, a clean rescan (reset() then updateStats() for
    //! all data with update == FALSE) is needed then to rebuild min/max.
    //! If a rescan is necessary, save all the old valid data (such as sums)
    //! and compare them to the new values.  If they differ, there is
    //! a problem!
    void updateStats(int id[], DataType value, bool update, 
		     DataType oldValue);

    //! Call for invalidating sum, sumOfSquares, and perhaps min/max based
    //! on the id and value.  To be called when the exact value to be
    //! processed is not known (i.e., stats on max when max needs rebuilding
    //! to be known).  Updates count, invalidates sums, and if id or value
    //! is such that max/min needs invalidating, it does that also.
    void invalidateStats (int id[], DataType value, bool update);

    //! Returns the number of data values these stats covers (may be 0)
    int count() {return (numData);}
    
    //! Returns the sum, or errorValue if no data has been processed
    DataType sum () 
	{if (numData > 0) return (dataSum); else return (errorValue);}

    //! Returns the sum of squares, or errorValue if no data has been processed
    DataType sumOfSquares () 
	{if (numData > 0) return (dataSumOfSquares); else return (errorValue);}

    //! Returns the max value, or errorValue if no data has been processed
    //! or if the update pattern was such that the max is not known.
    //! In this case, do a reset() and scan thru the data with updateStats()
    //! with update == FALSE, in order to generate a known max value.
    DataType max () 
	{ if ((numData > 0) && (maxId1[0] != NULL_INT)) 
	    return (max1); 
 	  else 
	    return (errorValue);
	}

    //! Returns the ids of the max value, or NULL_INT if no data has been
    //! processed or the max is not currently know.  See max() for details.
    int maxId (unsigned int idIndex) 
	{ 
	    if ((numData > 0) && (idIndex < numIds)) 
		return (maxId1[idIndex]);
	    else
		return (NULL_INT);
	}


    //! Returns the min value, or errorValue if no data has been processed
    //! or if the update pattern was such that the min is not known.
    //! In this case, do a reset() and scan thru the data with updateStats()
    //! with update == FALSE, in order to generate a known min value.
    DataType min () 
	{ if ((numData > 0) && (minId1[0] != NULL_INT)) 
	    return (min1); 
 	  else 
	    return (errorValue);
	}

    //! Returns the ids of the min value, or NULL_INT if no data has been
    //! processed or the min is not currently know.  See min() for details.
    int minId (unsigned int idIndex) 
	{ 
	    if ((numData > 0) && (idIndex < numIds)) 
		return (minId1[idIndex]); 
	    else 
		return (NULL_INT);
	}

private:
    //! Internal helper routine to set destId to srcId
    void setId (int destId[], int srcId[])
	{
	    for (int i=0; i < numIds; i++)
		destId[i] = srcId[i];
	}

    //! Internal helper routine to set all id values to NULL_INT
    void invalidateId (int id[])
	{
	    for (int i=0; i < numIds; i++)
		id[i] = NULL_INT;
	}

    //! Returns 1 if id is valid, 0 otherwise
    int validId (int id[])
	{
	    return (id[0] != NULL_INT);
	}

    //! Returns 0 if two ids equal, -1 if id1 smaller, 
    //! +1 if id1 bigger
    int compareId (int id1[], int id2[])
	{
	    for (int i=0; i < numIds; i++)
	    {
		int v1 = id1[i];
		int v2 = id2[i];
		if (v1 != v2)
		{
		    if (v1 < v2)
			return (-1);
		    else
			return (+1);
		}
	    }
	    // Must be equal if got here
	    return (0);
	}

    //! Returns 1 if two ids are equal, 0 otherwise
    int sameId (int id1[], int id2[])
	{
	    return (compareId(id1, id2) == 0);
	}

    //! Returns 1 if id1 < id2, 0 otherwise
    int smallerId (int id1[], int id2[])
	{
	    return (compareId(id1, id2) < 0);
	}

    

    DataType dataSum;          	// Sum of all the data elements
    DataType dataSumOfSquares; 	// Sum of the square of the data elements
    DataType max1;	      	// Max of all the data elements
    DataType max2;		// 2nd highest value of all data elements
    DataType min1;		// Min of all the data elements
    DataType min2;		// 2nd lowest value of all the data elements
    int maxId1[numIds];       	// Ids of max1, NULL_INT if not valid
    int maxId2[numIds];         // Ids of max2, NULL_INT if not valid
    int minId1[numIds];         // Ids of min1, NULL_INT if not valid
    int minId2[numIds];         // Ids of min2,  NULL_INT if not valid
    int numData;          	// Number of unique data elements processed

    //! This value is statically defined once per DataType.
    //! The first instantiation for a datatype needs to define
    //! something like this (i.e., if DataType is double) in their code 
    //! to make the program link:
    //!
    //! const double DataStatsIdArray<double,2>::errorValue = NULL_DOUBLE;
    //!
    static const DataType errorValue;
};


//! SumDataStats class designed to efficiently as possible calculate the 
//! sum, sum of squares (for std dev calculations) of possibly 
//! thousands of data values, which may be changing one at a time.
//! This is a stripped down version of DataStats that does not calc
//! min/max (only sums).
//! 
//! NULL_INT is not a valid id.  NULL_INT used is internally as special ids.
//!
//! Warning: The sum of squares calculation overflows rapidly for ints.
//!          I recommend using double type even for ints if planning
//!          on using the "sum of squares" value or if the even the
//!          "sum" is likely to overflow an int.

template<class DataType>
class SumDataStats 
{
public:
    
    //! Quick class initialization for quick creation
    SumDataStats() : numData(0) {}
    ~SumDataStats() {}
    
    //! Resets stat to clean slate.  
    void resetStats() {numData = 0;}
    
    //! Call for adding a new data value or for updating an existing data
    //! value (with id).
    //! 
    //! Each data value must have a unique id (such as the data value's index).
    //! 
    //! If the 'update' is TRUE, oldValue must be the previous value for
    //! this data point that was passed to updateData (if it is different,
    //! or the id is wrong, or if this data point was not passed to 
    //! updateStats earlier, you will get garbage stats).  If update is FALSE,
    //! the value of oldValue does not matter.
    //!
    //! This class only maintains aggregate data and makes inferences based
    //! on what is passed in.  Garbage in will yield garbage out!
    //!
    void updateStats(int id, DataType value, bool update, DataType oldValue);

    //! Call for invalidating sum, sumOfSquares.
    //! To be called when the exact value to be processed is not known 
    //! (i.e., stats on max when max needs rebuilding to be known). 
    void invalidateStats (int id, DataType value, bool update);

    //! Returns the number of data values these stats covers (may be 0)
    int count() {return (numData);}
    
    //! Returns the sum, or errorValue if no data has been processed
    DataType sum () 
	{if (numData > 0) return (dataSum); else return (errorValue);}

    //! Returns the sum of squares, or errorValue if no data has been processed
    DataType sumOfSquares () 
	{if (numData > 0) return (dataSumOfSquares); else return (errorValue);}

private:
    DataType dataSum;          	// Sum of all the data elements
    DataType dataSumOfSquares; 	// Sum of the square of the data elements
    int numData;          	// Number of unique data elements processed

    //! This value is statically defined once per DataType.
    //! The first instantiation for a datatype needs to define
    //! something like this (i.e., if DataType is double) in their code 
    //! to make the program link:
    //!
    //! const double SumDataStats<double>::errorValue = NULL_DOUBLE;
    //!
    static const DataType errorValue;
};

// Include the implementation of DataStat functions so can instantiate
// properly.
#include "datastats.cpp"
    

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

