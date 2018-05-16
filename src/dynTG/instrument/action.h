//! \file action.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 26 July 2000

#ifndef _ACTION_H
#define _ACTION_H

#include <string>
using std::string;

class Action;

//! ActionPoints are locations in a a program where Actions can
//! happen.  Zero or more actions can be associated with each
//! ActionPoint.  This class is intended to be a base class for
//! more specific kinds of action points, such as DPCLActionPoints.
class ActionPoint {
public:

	//! Create an ActionPoint, identified by a text tag.  Tags must
	//! be unique, since these objects are stored in STL sets, and
	//! the tags are used as keys.
	ActionPoint(string action_point_tag);

	//! Deletes memory associated with an ActionPoint,
	//! and all Actions associated with this point are deleted.
	virtual ~ActionPoint();

	//! Associates an Action with this point.  The Action
	//! passed in is not duplicated; only the pointer is copied.
	//! Returns -EINVAL if new_action is a NULL pointer.
	int add_action(Action * new_action);

	//! Remove an action from the action point.  The Action 
	//! itself is deleted.  We maintain simple
	//! array and don't attempt to compact it, since we expect that
	//! Actions won't be removed very often.  Returns -EINVAL
	//! if act_id does not refer to a valid action for this point.
	int remove_action(int act_id);

	//! Returns the number of Actions for this point
	int get_num_actions() const;

	//! Returns a pointer to the Action specified by act_id.
	//! This is a pointer to the actual data, not a copy.
	//! Returns -EINVAL if act_id does not refer to a valid
	//! Action.
	Action * get_action(int act_id);

	//! Returns the tag associated with this point.  This is a
	//! pointer to the actual tag, not a copy.
	string get_tag() const;

protected:
	string tag;
	Action ** actions;
	int next_action, last_action;
};

//! Allows sorting of ActionsPoints, so they can be stored in STL sets.
//! Compares action points based on their text strings.
bool operator<( const ActionPoint& lhs, const ActionPoint& rhs );

//! Allows comparison of ActionPoints, so the set find operation works.
//! Compares based on text strings.
bool operator==( const ActionPoint& lhs, const ActionPoint& rhs );

//! An action that is attached to an ActionPoint.  An action is
//! some time of event that can take place, usually under the
//! Collector's control, while the program is running.  The
//! main (and for now, only) type of action is a DPCLAction,
//! which represents some code that DPCL causes to be executed in
//! a program.  The Action class is intended to be a base class 
//! for more specific kinds of actions, such as DPCLActions.
class Action {
public:
	//! Initializes an Action.
	Action(string identifying_text);

	//! Not used in base class, but derived classes may need
	//! virtual destructor.
	virtual ~Action();

	//! Returns the identifying text for the action.  
	string get_text() const;

protected:
	string text;
};

//! Allows comparison of Actions, so they can be stored in STL sets.
//! Compares actions based on their text strings.
bool operator<( const Action& lhs, const Action& rhs );

//! Allows comparison of Actions, so the set find operation works.
//! Compares based on text strings.
bool operator==( const Action& lhs, const Action& rhs );

#endif // _ACTION_H

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

