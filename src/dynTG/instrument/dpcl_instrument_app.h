//!\file dpcl_instrument_app.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
//! These functions help automate the process of selecting instrumentation
//! points from an application and applying instrumentation to them.
// John May, 10 September 2002

#ifndef DPCL_INSTRUMENT_APP_H
#define DPCL_INSTRUMENT_APP_H

#include <boost/shared_ptr.hpp>
#include <dpcl.h>
#include <map>
using std::map;
#include <utility>
using std::pair;
#include <vector>
using std::vector;
#include <string>
using std::string;

#include "dpcl_action_instance.h"
#include "dpcl_action_point.h"
#include "dpcl_action_type.h"
#include "parse_program.h"
#include "tg_inst_point.h"

//! Function object class for instantiating a DPCL action type at a 
//! specified Instrumentation Point.  If a DPCLActionPoint for this
//! instrumentation point does not already exist, one is created at
//! added to the map.
class ApplyInstrumentation :
public std::unary_function<pair<string,IPInfoP>,void> {
public:
	ApplyInstrumentation(
			vector<DPCLActionTypeP>& at,
						//!< Types of actions
						//!< (instrumentation) to be
						//!< applied
			Application& a,		//!< DPCL application
			map<string,DPCLActionPointP>& ap,
			  			//!< Existing DPCLActionPoints;
						//!< new points may be added to
						//!< to this map
			int pid = ALL_PROCESSES //!< Which process(es) get
						//!< the instrumentation
			)
		: action_types(at), app(a), action_points(ap),
		process_id(pid)
	{ }

	void operator() ( pair<string,IPInfoP> ippair
						//!< Description of inst point.
						//!< The first element of the
						//!< pair is unused, but this
						//!< form makes it easy to 
						//!< apply this object to
						//!< a map containing IPInfo
						//!< indexed by strings.
			  );
private:
	vector<DPCLActionTypeP>& action_types;
	Application& app;
	map<string,DPCLActionPointP>& action_points;
	int process_id;
};

//! Function object class for reporting instrumentation points to
//! the Client program.
class ReportInstPoint :
public std::unary_function<pair<string,IPInfoP>,void> {
public:
	explicit ReportInstPoint( int s		//!< Socket to which info will
						//!< be sent
			)
		: socket(s)
	{ }

	void operator() ( pair<string,IPInfoP> ippair
						//!< Description of inst point.
						//!< The first element of the
						//!< pair is unused, but this
						//!< form makes it easy to 
						//!< apply this object to
						//!< a map containing IPInfo
						//!< indexed by strings.
			  ) const;
private:
	int socket;
};


//! Apply instrumentation to a set of instrumentation points specified by
//! regular expressions, types, and locations.  Each set of instrumentation
//! points that matches the criteria will be instrumented with a corresponding
//! instrumentation type.  The set of points specified by the first element
//! in the expressions, iptype, and iploc vectors will be instrumented with
//! the action type given in the first element of the action_types vector.
//! As locations are instrumented, the corresponding DPCLActionPoints will
//! be created and populated with DPCLPointActions.
void InstrumentLocations( Application& app,	//!< DPCL app to which
						//!< instrumentation is applied
			 const vector<string>& expressions,
			 			//!< Vector of regular
						//!< expressions (using the
						//!< the extended notation
						//!< defined for regcomp())
			 const vector<TG_InstPtType>& iptype,
			 			//!< Vector of instrumentation
						//!< point types; see
						//!< tg_inst_point.h.  An
						//!< invalid type acts as a
						//!< wildcard.
			 const vector<TG_InstPtLocation>& iploc,
			 			//!< Vector of instrumentation
						//!< point locations; see
						//!< tg_inst_point.h.  An
						//!< invalid location acts as a
						//!< wildcard.
			 vector<DPCLActionTypeP>& action_types,
			 			//!< ActionTypes to apply to
						//!< the set of inst points
						//!< that match the criteria
						//!< specified in the three
						//!< parameters above.  The
						//!< set of points matching
						//!< the i'th criteria in
						//!< the vectors above is
						//!< instrumented with the
						//!< i'th action type.
			 map<string, DPCLActionPointP>& action_points
			 			//!< The associative array
						//!< is filled in with
						//!< the action points and
						//!< actions created by this
						//!< function.
			 );
#endif // DPCL_INSTRUMENT_APP_H
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

