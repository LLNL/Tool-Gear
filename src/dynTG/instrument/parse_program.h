//!\ file parse_program.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
//! These functions use DPCL to gather information about the
//! structure of a target program and send it to the Client.
// John May, 10 October 2000


#ifndef PARSE_PROGRAM_H
#define PARSE_PROGRAM_H

#include <boost/shared_ptr.hpp>
#include <map>
using std::map;
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <regex.h>
#include <sys/types.h>

#include "tg_inst_point.h"

//! Struct for list of modules (files)
struct ModuleInfo {
	ModuleInfo() : file_path(NULL)
	{ }
	~ModuleInfo()
	{
		delete [] file_path;
	}
	int process_id;			//!< DPCL (not Unix) process id number
	int module_id;			//!< DPCL module id number
	char * file_path;		//!< Path to source file
};

//! Struct for list of functions
struct FunctionInfo {
	FunctionInfo() : func_name(NULL)
	{ }
	~FunctionInfo()
	{
		delete [] func_name;
	}
	int function_id;		//!< DPCL function id number
	int start_line;			//!< Starting line number (0 if unknown)
	int end_line;			//!< Ending line number (0 if unknown)
	char * func_name;		//!< Function name (demangled in C++)
};

//! Struct for list of instrumentation points
struct IPInfo {
	IPInfo() : func_called(NULL), ip_tag(NULL), description(NULL)
	{ }

	//! Makes a shallow copy of the inputs
	IPInfo( InstPoint pt, TG_InstPtType t, TG_InstPtLocation lo,
		string infunc, int li, string fc, int ci, string ipt, string d,
		int i, int mid, int fid)
		: ip(pt), type(t), loc(lo), in_function(infunc), line(li),
		func_called(fc), call_index(ci), ip_tag(ipt), description(d),
		index(i), module_id(mid), function_id(fid)
	{ }
	
	InstPoint	ip;		//!< DPCL instrumentation point handle
	TG_InstPtType	type;		//!< Type (entry, exit, func call...)
	TG_InstPtLocation loc;		//!< Location (before, after)
	string		in_function;	//!< Function that contains this point
	int		line;		//!< Line number of inst point
	string		func_called;	//!< Name of function called (if any)
	int		call_index;	//!< Which function on current line
	string		ip_tag;		//!< Unique identifying string
	string		description;	//!< Text summary of instrumentation
					//!< point information
	int		index;		//!< Internal use only
        int             module_id;      //!< DPCL module id
        int 	        function_id;    //!< DPCL function id
};

//! Smart pointer for storing IPInfo in a container
typedef boost::shared_ptr<IPInfo> IPInfoP;
	
//! Parse a parallel program using DPCL and send the information about it
//! to the Client.  Parses only one process of the application (so it
//! assumes the program is SPMD).  An array of ModuleInfo objects is
//! returned in modules, and the number of items in this array is the
//! return value (-1 return value indicates an error).  The
//! must delete this array.
int parse_application( Application& application, ModuleInfo *& modules);

//! Parse a single process using DPCL and send the information about it
//! to the Client.  Parses only to the level of modules (files).  Since
//! processes may contain many linked-in modules that the user doesn't,
//! care about, this function can filter out uninteresting modules using
//! a combination of inclusion and exclustion filters.  The environment 
//! variable TG_EXCLUSION_FILE contains the name of a file with a list of
//! modules to be ignored during parsing.  If this variable isn't set,
//! then we look for a file in the current directory called "parse_exclude".
//! Similary, an environment variable called TG_INCLUSION_FILE specifies
//! a file with a list of files not to be ignored.  (Or "parse_include"
//! is used if the variable isn't set.)  If there is an inclusion list,
//! then only modules appearing on that list will be reported to the Client.
//! Any combination of inclusion and exclusion lists may be used.  If a
//! module appears on both lists, it will be excluded.  An
//! array of ModuleInfo objects is returned in modules, and the
//! number of items in this array is the ! return value (-1 return
//! value indicates an error).  The caller must delete this array.
int parse_process( Process& process, int pnum, ModuleInfo *& modules );

//! Parse a module in an process and send the information about it
//! to the Client.  Parses only to the level of functions.
//! An array of FunctionInfo objects is returned in funcs, and
//! the number of items in this array is the return value (-1
//! return value indicates an error).  The caller must delete
//! this array.
int parse_module( Application&, int process_id, int module_id,
		FunctionInfo *& funcs );

//! Parse a function in a module and send information about the
//! instrumentation points to the Client.  Returns the length of
//! the allocated inst_points array, or -1 if an error occured.
//! The caller must delete this array.
int parse_function( Application&,	//!< DPCL application handle
		int process_id,		//!< DPCL process number (not Unix pid)
		int module_id,		//!< DPCL module number
		int function_id,	//!< DPCL function number
		const vector<regex_t>& reg_exps,
#if 0
		regex_t * rep,		//!< Pointer to compiled regular
					//!< expression to match
					//!< function call instrumentation
					//!< points.  Uses Unix Extended
					//!< Regular Expression syntax.  If
					//!< this pointer is NULL, all function
					//!< calls are returned; if not, only
					//!< matching calls are returned.
#endif
		const vector<TG_InstPtLocation>& match_loc,
					//!< Instrumentation point
					//!< location to match.  If equal to
					//!< TG_IPL_invalid, all locations are
					//!< are returned; if not, only
					//!< matching ones are.
		const vector<TG_InstPtType>& match_type,
					//!< Instrumentation point type
					//!< to match.  Works like match_loc,
					//!< but use TG_IPT_invalid to match
					//!< any type.
		map<string,IPInfoP>& inst_points
					//!< Vector of matching instrumentation
					//!< points returned.
		 );

//! Returns the DPCL instrumentation point corresponding to a text tag.
InstPoint tag_to_inst_point( Process * p, char * tag );

//! Returns the DPCL instrumentation point corresponding to a text tag.
InstPoint tag_to_inst_point( Application * a, char * tag );

#endif // PARSE_PROGRAM_H
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

