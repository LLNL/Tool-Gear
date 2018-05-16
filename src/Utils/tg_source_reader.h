//! \file tg_source_reader.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 30 August 2004

#ifndef TG_SOURCE_READER_H
#define TG_SOURCE_READER_H

#include "tg_socket.h"
#include "tg_pack.h"
#include "search_path.h"

//! Functions for collectors to respond to requests for source files
//! These functions are designed specifically to work in the Tool
//! gear Client-Collector protocol, so they use other TG 
//! functions extensively, and they expect to read and send
//! requests over the TG socket connection in standard ways.

class TGSourceReader {
public:
	//! Set up the source file reader with the socket to
	//! be used for returning requests and the callback
	//! to be polled periodically during file searches
	//! to check whether the search should continue (or
	//! has been cancelled).  The function should return
	//! nonzero if the search should continue, or be
	//! NULL if no check should be made.
	TGSourceReader( int sock, int (*continue_search_cb)(void) );

	//! Cleans up internal state; does not communicate with Client
	~TGSourceReader();

	//! Looks a source file and returns its contents, using either an
	//! absolute path name or a search path specified by the Client or
	//! in a SourcePath file
	void read_file( char * buf, int id );

	//! Handles a request to get the subdirectories of a named 
	//! directory and returns the result to the Client.
	void unpack_get_subdirs( char * buf );

	//! Reports the current search path for source files to the Client.
	void report_search_path( char * buf );

	//! Unpack a request from the client to set the search path for
	//! source files.
	void unpack_set_source_path( char * buf );

	//! Return the socket passed in during construction
	int getSocket() const
	{	return sock;	}

protected:
	//! Internal routine to sets the path to be used in searches for
	//! source files.
	void set_source_path(void);

	//! Save the current search path in a SourcePath file in the named
	//! directory.
	void save_source_path( char * directory );

	//! Internal routine to read the search path to be used for finding
	//! source files from a SourcePath file.
	void set_source_path_from_file(void);

private:
	//! Called by findFile (without a socket name); we relay
	//! the call to the callback passed to the ctor, and add
	//! the socket.  Also, handle the case where the user gave
	//! us no callback.
	SearchPath * sourcePath;
	int sock;
	int (*continue_cb)(void);
};

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

