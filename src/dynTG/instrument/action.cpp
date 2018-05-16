// action.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 26 July 2000

#include <errno.h>
#include <stdlib.h>

#include "action.h"

#define ALLOC_INCREMENT 10

/* Constructor */
Action:: Action(string identifying_text)
	: text(identifying_text)
{ }

Action:: ~Action()
{ }

string Action:: get_text() const
{
	return text;
}

bool operator<( const Action& lhs, Action& rhs )
{
	return lhs.get_text() < rhs.get_text();
}

bool operator==( const Action& lhs, Action& rhs )
{
	return lhs.get_text() == rhs.get_text();
}

ActionPoint:: ActionPoint(string action_point_tag)
	: tag(action_point_tag), actions(NULL),
	next_action(0), last_action(0)
{ }

ActionPoint:: ~ActionPoint()
{
	for( int i = 0; i < next_action; i++ ) {
		if( actions[i] != NULL )
			delete actions[i];
	}

	if( actions != NULL ) delete [] actions;
}

int ActionPoint:: add_action(Action * new_action)
{
	if( new_action == NULL ) {
		return -EINVAL;
	}

	if( next_action >= last_action ) {
		actions = (Action **) realloc(actions, sizeof(Action *)
				* (last_action + ALLOC_INCREMENT));
		if( actions == NULL ) return -ENOMEM;
		last_action += ALLOC_INCREMENT;
	}

	actions[next_action] = new_action;

	return next_action++;
}

Action * ActionPoint:: get_action( int act_id )
{
	if( act_id < 0 || act_id >= next_action ||
			actions[act_id] == NULL ) {
		return NULL;
	}

	return actions[act_id];
}

// Remove an action from the action point.  We maintain simple
// array and don't attempt to compact it, since we expect that
// actions won't be removed very often.
int ActionPoint:: remove_action(int act_id)
{
	if( act_id < 0 || act_id >= next_action ||
			actions[act_id] == NULL ) {
		return -EINVAL;
	}

	delete actions[act_id];
	actions[act_id] = NULL;

	return 0;
}

int ActionPoint:: get_num_actions() const
{
	// We're really returning the "high water mark", not the
	// current number of actions, since this will be used to
	// find out the maximum action tag id to query, and if a
	// high-numbered tag is kept and a low-numbered tag is
	// removed, the high-numbered item would not be visible.
	return next_action;
}

string ActionPoint:: get_tag() const
{
	return tag;
}

bool operator<( const ActionPoint& lhs, ActionPoint& rhs )
{
	return lhs.get_tag() < rhs.get_tag();
}

bool operator==( const ActionPoint& lhs, ActionPoint& rhs )
{
	return lhs.get_tag() == rhs.get_tag();
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

