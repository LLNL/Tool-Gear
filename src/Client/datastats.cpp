// datastats.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
/****************************************************************************
**
** TEMPLATE IMPLEMENTATION OF datastats.h!
**
** SHOULD NOT NEED TO COMPILE THIS FILE DIRECTLY, INCLUDED IN datastats.h!
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
** Also provide SumDataStats template that only does sums (no min/max).
**
** Designed by John Gyllenhaal at LLNL (with suggestions from John May)
** Initial implementation by John Gyllenhaal 4/22/02
**
*****************************************************************************/

// Just in case someone needs to compile this standalone...
#include "datastats.h"
#include <iostream>
using namespace std;

// Call for adding a new data value or for updating an existing data
// value (with id).
// 
// Each data value must have a unique id (such as the data value's index),
// and this id will be returned with min/max queries.  This id will also
// be used to break ties for min/max (lowest id wins).  
// 
// If the 'update' is TRUE, oldValue must be the previous value for
// this data point that was passed to updateData (if it is different,
// or the id is wrong, or if this data point was not passed to 
// updateStats earlier, you will get garbage stats).  If update is FALSE,
// the value of oldValue does not matter.
//
// This class only maintains aggregate data and makes inferences based
// on what is passed in.  Garbage in will yield garbage out!
//
// With certain sequences of data updates, the min and/or max values
// may become unknown (a query of min/max will reveal this).  
// In this case, a clean rescan (reset() then updateStats() for
// all data with update == FALSE) is needed then to rebuild min/max.
// If a rescan is necessary, save all the old valid data (such as sums)
// and compare them to the new values.  If they differ, there is
// a problem!
template<class DataType>
void DataStats<DataType>::updateStats(int id, DataType value, 
				      bool update, DataType oldValue)
{
    // If only data point (first data point or update of only data
    // point), can set data values directly
    if ((numData == 0) || (update && (numData == 1)))
    {
	// This data alone sets sum, min, max, etc.
	numData = 1;
	dataSum = value;
	dataSumOfSquares = value * value;
	max1 = value;
	maxId1 = id;
	min1 = value;
	minId1 = id;

	// Mark that no 2nd min/max yet
	maxId2 = NULL_INT;
	minId2 = NULL_INT;
    }

    // If new data point (not an update), straightforward to update stats
    else if (!update)
    {
	// New data point, increment count
	numData++;

	// Don't update invalidated sums
	if (dataSum != errorValue)
	{
	    // Add to sums
	    dataSum += value;
	    dataSumOfSquares += value * value;
	}

	// Update max1 and max2, if max1 is valid
	if (maxId1 != NULL_INT)
	{
	    // If have new max1, update both max1 and max2
	    // Use id to break ties (lowest id wins)
	    if ((value > max1) || ((value == max1) && (id < maxId1)))
	    {
		// max2 is now the old max1
		max2 = max1;
		maxId2 = maxId1;

		// max1 is now value
		max1 = value;
		maxId1 = id;
	    }

	    // If second data point, or value bigger than max2, set max2
	    else if ((numData == 2) || 
		     ((maxId2 != NULL_INT) && 
		      ((value > max2) || 
		       ((value == max2) && (id < maxId2)))))
	    {
		max2 = value;
		maxId2 = id;
	    }
	}

	// Update min1 and min2, if min1 is valid
	if (minId1 != NULL_INT)
	{
	    // If have new min1, update both min1 and min2
	    // Use id to break ties (lowest id wins)
	    if ((value < min1) || ((value == min1) && (id < minId1)))
	    {
		// min2 is now the old min1
		min2 = min1;
		minId2 = minId1;

		// min1 is now  value
		min1 = value;
		minId1 = id;
	    }

	    // If second data point, or value smaller than min2, set min2
	    else if ((numData == 2) || 
		     ((minId2 != NULL_INT) && 
		      ((value < min2) || 
		       ((value == min2) && (id < minId2)))))
	    {
		min2 = value;
		minId2 = id;
	    }
	}
    }

    // Otherwise, if not new data and have more than 1 data point,
    // complicated update.  This update can invalidate min/max
    // values (which would require rescan of all data points
    // to make correct again)
    else 
    {
	// Don't update invalidated sums
	if (dataSum != errorValue)
	{
	    // Add difference from old value to sums
	    dataSum += value - oldValue;
	    dataSumOfSquares += (value * value) - (oldValue *oldValue);
	}

	// Update max1 and max2, if max1 is valid
	if (maxId1 != NULL_INT)
	{
	    // If id already set max1, update max1 if possible,
	    // and invalidate max1 if not possible
	    if (maxId1 == id)
	    {
		// If bigger or equal to old max1, or if bigger
		// to old max2 (if exists), just update
		if ((value >= max1) || 
		    ((maxId2 != NULL_INT) && 
		     ((value > max2) || ((value == max2) && (id < maxId2)))))
		{
		    max1 = value;
		}

		// If have max2 (and max2 is bigger than value
		// if got here), make it max1 and invalidate max2
		else if (maxId2 != NULL_INT)
		{
		    max1 = max2;
		    maxId1 = maxId2;
		    maxId2 = NULL_INT;
		}

		// Otherwise, don't know what max1 is anymore
		else
		{
		    maxId1 = NULL_INT;
		}
	    }

	    // If have new max1 (and was not maxId1 before), 
	    // update both max1 and max2
	    else if ((value > max1) || ((value == max1) && (id < maxId1)))
	    {
		// max2 is now the old max1
		max2 = max1;
		maxId2 = maxId1;

		// max1 is now value
		max1 = value;
		maxId1 = id;
	    }

	    // If was old max2 (and since here, smaller than max1),
	    // update max2 if possible, invalidate if not
	    else if (maxId2 == id)
	    {
		// If greater or equal to current max2, 
		// or if this is the only other data point, just update
		if ((value >= max2) || (numData == 2))
		    max2 = value;

		// Otherwise, must invalidate since don't know if max2
		else
		    maxId2 = NULL_INT;
	    }

	    // If second data point, or value bigger than max2, set max2
	    else if ((numData == 2) || 
		     ((maxId2 != NULL_INT) && 
		      ((value > max2) || 
		       ((value == max2) && (id < maxId2)))))
	    {
		max2 = value;
		maxId2 = id;
	    }
	}

	// Update min1 and min2, if min1 is valid
	if (minId1 != NULL_INT)
	{
	    // If id already set min1, update min1 if possible,
	    // and invalidate min1 if not possible
	    if (minId1 == id)
	    {
		// If smaller or equal to old min1, or if smaller
		// or equal to old min2 (if exists), just update
		if ((value <= min1) || 
		    ((minId2 != NULL_INT) && 
		     ((value < min2) || ((value == min2) && (id < minId2)))))
		{
		    min1 = value;
		}

		// If have min2 (and min2 is smaller than value
		// if got here), make it min1 and invalidate min2
		else if (minId2 != NULL_INT)
		{
		    min1 = min2;
		    minId1 = minId2;
		    minId2 = NULL_INT;
		}

		// Otherwise, don't know what min1 is anymore
		else
		{
		    minId1 = NULL_INT;
		}
	    }

	    // If have new min1 (and was not minId1 before), 
	    // update both min1 and min2
	    else if ((value < min1) || ((value == min1) && (id < minId1)))
	    {
		// min2 is now the old min1
		min2 = min1;
		minId2 = minId1;

		// min1 is now value
		min1 = value;
		minId1 = id;
	    }

	    // If was old min2 (and since here, min1 smaller than 
	    // NewValue), update min2 if possible, invalidate if not
	    else if (minId2 == id)
	    {
		// If smaller or equal to current min2, 
		// or if this is the only other data point, just update
		if ((value <= min2) || (numData == 2))
		    min2 = value;

		// Otherwise, must invalidate since don't know if min2
		else
		    minId2 = NULL_INT;
	    }

	    // If second data point, or value smaller than min2, set min2
	    else if ((numData == 2) || 
		     ((minId2 != NULL_INT) && 
		      ((value < min2) || 
		       ((value == min2) && (id < minId2)))))
	    {
		min2 = value;
		minId2 = id;
	    }
	}
    }
}


// Call for invalidating sum, sumOfSquares, and perhaps min/max based
// on the id and value.  To be called when the exact value to be
// processed is not known (i.e., stats on max when max needs rebuilding
// to be known).  Updates count, invalidates sums, and if id or value
// is such that max/min needs invalidating, it does that also.
template<class DataType>
void DataStats<DataType>::invalidateStats (int id, DataType value, 
					   bool update)
{
    // DEBUG
//    cout << "Invaliding id " << id << " for value " << value << " update " 
//	 << update << endl;

    // If not update, increment count
    if (!update)
	numData++;

    // Invalidate sums by setting to errorValue
    dataSum = errorValue;
    dataSumOfSquares = errorValue;

    // If max1 is still valid, determine if need to invalidate
    if (maxId1 != NULL_INT)
    {
	// If maxId1 is id, or would cause update of max1, invalidate max1
	if ((maxId1 == id) || (value > max1) ||
	    ((value == max1) && (id < maxId1)))
	{
	    // DEBUG
//	    cout << "*Invaliding maxId1 " << maxId1 << " max1 " << max1 << endl;
	    maxId1 = NULL_INT;
	}
	
	// Otherwise if maxId2 is id, or would cause update of max2,
	// invalidate max2
	else if ((maxId2 != NULL_INT) && 
		 ((maxId2 == id) || (value > max2) ||
		  ((value == max2) && (id < maxId2))))
	{
	    // DEBUG
//	    cout << "*Invaliding maxId2 " << maxId2 << " max2 " << max2 << endl;
	    maxId2 = NULL_INT;
	}
    }

    // If min1 is still valid, determine if need to invalidate
    if (minId1 != NULL_INT)
    {
	// If minId1 is id, or would cause update of min1, invalidate min1
	if ((minId1 == id) || (value < min1) ||
	    ((value == min1) && (id < minId1)))
	{
	    // DEBUG
//	    cout << "*Invaliding minId1 " << minId1 << " min1 " << min1 << endl;
	    minId1 = NULL_INT;
	}
	
	// Otherwise if minId2 is id, or would cause update of min2,
	// invalidate min2
	else if ((minId2 != NULL_INT) && 
		 ((minId2 == id) || (value < min2) ||
		  ((value == min2) && (id < minId2))))
	{
	    // DEBUG
//	    cout << "*Invaliding minId2 " << minId2 << " min2 " << min2 << endl;
	    minId2 = NULL_INT;
	}
    }
}



// Two id version of DataStats updateStats call.  See DataStats for details.
template<class DataType, int numIds>
void DataStatsIdArray<DataType, numIds>::updateStats(int id[],
						     DataType value, 
						     bool update, 
						     DataType oldValue)
{
//    cout << "UpdateStats " << id[0] << " " << id[1] << " value " << value 
//	 << " update " << update << " oldValue " << oldValue << endl;

    // If only data point (first data point or update of only data
    // point), can set data values directly
    if ((numData == 0) || (update && (numData == 1)))
    {
	// This data alone sets sum, min, max, etc.
	numData = 1;
	dataSum = value;
	dataSumOfSquares = value * value;
	max1 = value;
	setId (maxId1, id);
	min1 = value;
	setId (minId1, id);

	// Mark that no 2nd min/max yet
	invalidateId (maxId2);
	invalidateId (minId2);
    }

    // If new data point (not an update), straightforward to update stats
    else if (!update)
    {
	// New data point, increment count
	numData++;

	// Don't update invalidated sums
	if (dataSum != errorValue)
	{
	    // Add to sums
	    dataSum += value;
	    dataSumOfSquares += value * value;
	}

	// Update max1 and max2, if max1 is valid
	if (validId(maxId1))
	{
	    // If have new max1, update both max1 and max2
	    // Use id to break ties (lowest id wins)
	    if ((value > max1) || 
		((value == max1) && smallerId(id, maxId1)))
	    {
		// max2 is now the old max1
		max2 = max1;
		setId (maxId2, maxId1);

		// max1 is now value
		max1 = value;
		setId (maxId1, id);
	    }

	    // If second data point, or value bigger than max2, set max2
	    else if ((numData == 2) || 
		     (validId(maxId2) && 
		      ((value > max2) || 
		       ((value == max2) && smallerId(id, maxId2)))))
	    {
		max2 = value;
		setId (maxId2, id);
	    }
	}

	// Update min1 and min2, if min1 is valid
	if (validId(minId1))
	{
	    // If have new min1, update both min1 and min2
	    // Use id to break ties (lowest id wins)
	    if ((value < min1) || 
		((value == min1) && smallerId(id, minId1)))
	    {
		// min2 is now the old min1
		min2 = min1;
		setId (minId2, minId1);

		// min1 is now  value
		min1 = value;
		setId (minId1, id);
	    }

	    // If second data point, or value smaller than min2, set min2
	    else if ((numData == 2) || 
		     (validId(minId2) && 
		      ((value < min2) || 
		       ((value == min2) && smallerId(id, minId2)))))
	    {
		min2 = value;
		setId (minId2, id);
	    }
	}
    }

    // Otherwise, if not new data and have more than 1 data point,
    // complicated update.  This update can invalidate min/max
    // values (which would require rescan of all data points
    // to make correct again)
    else 
    {
	// Don't update invalidated sums
	if (dataSum != errorValue)
	{
	    // Add difference from old value to sums
	    dataSum += value - oldValue;
	    dataSumOfSquares += (value * value) - (oldValue *oldValue);
	}

	// Update max1 and max2, if max1 is valid
	if (validId(maxId1))
	{
	    // If id already set max1, update max1 if possible,
	    // and invalidate max1 if not possible
	    if (sameId(id, maxId1))
	    {
		// If bigger or equal to old max1, or if bigger
		// to old max2 (if exists), just update
		if ((value >= max1) || 
		    (validId(maxId2) && 
		     ((value > max2) || 
		      ((value == max2) && smallerId(id, maxId2)))))
		{
//		    cout << "Updating max1 to " << value << endl;
		    max1 = value;
		}

		// If have max2 (and max2 is bigger than value
		// if got here), make it max1 and invalidate max2
		else if (validId(maxId2))
		{
//		    cout << "Updating max1 to max2 " << max2 << endl;
		    max1 = max2;
		    setId (maxId1, maxId2);
		    invalidateId (maxId2);
		}

		// Otherwise, don't know what max1 is anymore
		else
		{
//		    cout << "Invalidating max1" << endl;
		    invalidateId (maxId1);
		}
	    }

	    // If have new max1 (and was not maxId1 before), 
	    // update both max1 and max2
	    else if ((value > max1) || 
		     ((value == max1) && smallerId(id, maxId1)))
	    {
		// max2 is now the old max1
		max2 = max1;
		setId (maxId2, maxId1);

		// max1 is now value
		max1 = value;
		setId (maxId1, id);
	    }

	    // If was old max2 (and since here, smaller than max1),
	    // update max2 if possible, invalidate if not
	    else if (sameId(id, maxId2))
	    {
		// If greater or equal to current max2, 
		// or if this is the only other data point, just update
		if ((value >= max2) || (numData == 2))
		    max2 = value;

		// Otherwise, must invalidate since don't know if max2
		else
		{
		    invalidateId (maxId2);
		}
	    }

	    // If second data point, or value bigger than max2, set max2
	    else if ((numData == 2) || 
		     (validId(maxId2) && 
		      ((value > max2) || 
		       ((value == max2) && smallerId(id, maxId2)))))
	    {
		max2 = value;
		setId (maxId2, id);
	    }
	}

	// Update min1 and min2, if min1 is valid
	if (validId(minId1))
	{
	    // If id already set min1, update min1 if possible,
	    // and invalidate min1 if not possible
	    if (sameId(id, minId1))
	    {
		// If smaller or equal to old min1, or if smaller
		// or equal to old min2 (if exists), just update
		if ((value <= min1) || 
		    (validId(minId2) && 
		     ((value < min2) || 
		      ((value == min2) && smallerId(id, minId2)))))
		{
		    min1 = value;
		}

		// If have min2 (and min2 is smaller than value
		// if got here), make it min1 and invalidate min2
		else if (validId(minId2))
		{
		    min1 = min2;
		    setId (minId1, minId2);
		    invalidateId (minId2);
		}

		// Otherwise, don't know what min1 is anymore
		else
		{
		    invalidateId (minId1);
		}
	    }

	    // If have new min1 (and was not minId1 before), 
	    // update both min1 and min2
	    else if ((value < min1) || 
		     ((value == min1) && smallerId(id, minId1)))
	    {
		// min2 is now the old min1
		min2 = min1;
		setId (minId2, minId1);

		// min1 is now value
		min1 = value;
		setId (minId1, id);
	    }

	    // If was old min2 (and since here, min1 smaller than 
	    // NewValue), update min2 if possible, invalidate if not
	    else if (sameId(id, minId2))
	    {
		// If smaller or equal to current min2, 
		// or if this is the only other data point, just update
		if ((value <= min2) || (numData == 2))
		    min2 = value;

		// Otherwise, must invalidate since don't know if min2
		else
		{
		    invalidateId(minId2);
		}
	    }

	    // If second data point, or value smaller than min2, set min2
	    else if ((numData == 2) || 
		     (validId(minId2) && 
		      ((value < min2) || 
		       ((value == min2) && smallerId(id, minId2)))))
	    {
		min2 = value;
		setId (minId2, id);
	    }
	}
    }
}


// Two id version of DataStats invalidateStats.  See DataStats function 
// for details
template<class DataType, int numIds>
void DataStatsIdArray<DataType, numIds>::invalidateStats (int id[],
							  DataType value,  
							  bool update)
{
    // DEBUG
//    cout << "Invaliding id " << id[0] << " " << id[1] << " for value " 
//	 << value << " update " << update << endl;

    // If not update, increment count
    if (!update)
	numData++;

    // Invalidate sums by setting to errorValue
    dataSum = errorValue;
    dataSumOfSquares = errorValue;

    // If max1 is still valid, determine if need to invalidate
    if (validId(maxId1))
    {
	// If maxId1 is id, or would cause update of max1, invalidate max1
	if (sameId(id, maxId1) || (value > max1) ||
	    ((value == max1) && smallerId(id, maxId1)))
	{
	    // DEBUG
//	    cout << "*Invaliding maxId1 " << maxId1 << " max1 " << max1 << endl;
	    invalidateId(maxId1);
	}
	
	// Otherwise if maxId2 is id, or would cause update of max2,
	// invalidate max2
	else if (validId(maxId2) && 
		 (sameId(id, maxId2) || (value > max2) ||
		  ((value == max2) && smallerId(id, maxId2))))
	{
	    // DEBUG
//	    cout << "*Invaliding maxId2 " << maxId2 << " max2 " << max2 << endl;
	    invalidateId(maxId2);
	}
    }

    // If min1 is still valid, determine if need to invalidate
    if (validId(minId1))
    {
	// If minId1 is id, or would cause update of min1, invalidate min1
	if (sameId(id, minId1) || (value < min1) ||
	    ((value == min1) && smallerId(id, minId1)))
	{
	    // DEBUG
//	    cout << "*Invaliding minId1 " << minId1 << " min1 " << min1 << endl;
	    invalidateId(minId1);
	}
	
	// Otherwise if minId2 is id, or would cause update of min2,
	// invalidate min2
	else if (validId(minId2) && 
		 (sameId(id, minId2) || (value < min2) ||
		  ((value == min2) && smallerId(id, minId2))))
	{
	    // DEBUG
//	    cout << "*Invaliding minId2 " << minId2 << " min2 " << min2 << endl;
	    invalidateId(minId2);
	}
    }
}


// (SumDataStats version, stripped down DataStats code.)
// Call for adding a new data value or for updating an existing data
// value (with id).
// 
// Each data value must have a unique id (such as the data value's index).

// If the 'update' is TRUE, oldValue must be the previous value for
// this data point that was passed to updateData (if it is different,
// or the id is wrong, or if this data point was not passed to 
// updateStats earlier, you will get garbage stats).  If update is FALSE,
// the value of oldValue does not matter.
//
// This class only maintains aggregate data and makes inferences based
// on what is passed in.  Garbage in will yield garbage out!
template<class DataType>
void SumDataStats<DataType>::updateStats(int id, DataType value, 
					 bool update, DataType oldValue)
{
    // If only data point (first data point or update of only data
    // point), can set data values directly
    if ((numData == 0) || (update && (numData == 1)))
    {
	// This data alone sets sum, min, max, etc.
	numData = 1;
	dataSum = value;
	dataSumOfSquares = value * value;
    }

    // If new data point (not an update), straightforward to update stats
    else if (!update)
    {
	// New data point, increment count
	numData++;

	// Don't update invalidated sums
	if (dataSum != errorValue)
	{
	    // Add to sums
	    dataSum += value;
	    dataSumOfSquares += value * value;
	}
    }

    // Otherwise, if not new data and have more than 1 data point,
    // slightly more complicated update.  
    else 
    {
	// Don't update invalidated sums
	if (dataSum != errorValue)
	{
	    // Add difference from old value to sums
	    dataSum += value - oldValue;
	    dataSumOfSquares += (value * value) - (oldValue *oldValue);
	}
    }
}


// Call for invalidating sum, sumOfSquares.
// To be called when the exact value to be processed is not known 
// (i.e., stats on max when max needs rebuilding to be known). 
template<class DataType>
void SumDataStats<DataType>::invalidateStats (int id, DataType value, 
					      bool update)
{
    // DEBUG
//    cout << "Invaliding id " << id << " for value " << value << " update " 
//	 << update << endl;

    // If not update, increment count
    if (!update)
	numData++;

    // Invalidate sums by setting to errorValue
    dataSum = errorValue;
    dataSumOfSquares = errorValue;
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

