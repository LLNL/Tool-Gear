//! \file search_path.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 26 February 2004

#ifndef SEARCH_PATH_H
#define SEARCH_PATH_H

#include <string_symbol.h>

// Handle platforms that do not define TRUE and FALSE
#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

//! Encapsulates creating, maintaining, and storing a search path
class SearchPath {
public:
	//! Constructor builds a path from a string.  Individual
	//! path entires are separated by : or \\n characters.
	//! Entries that begin with * indicate directories that will
	//! be searched recursively.
	SearchPath( const char * initial_path );

	//! Null constructor; creates an empty search path.  No
	//! files will be found until the path is set using
	//! setPath (even in the current directory).
	SearchPath();

	//! Destructor; frees internal resources.
	~SearchPath();

	//! Returns a string containing the search path, with 'sep' as the
	//! separator and the * prefix as described for the constructor.
	//! The returned string should be deleted when no longer needed.
	char * getPath( char sep ) const;

	//! Overloaded version of above function; stores path in
	//! user-supplied buffer, but no more than len characters
	//! (including terminating \\0).  If full path doesn't fit,
	//! the buffer is filled until space runs out.  The return
	//! value indicates the length of the full path, so if it's
	//! greater than len, the path was truncated.
	int getPath( char * buffer, int len, char sep ) const;

	//! Reset the path to the path given in new_path.  Individual
	//! directories should be separated by : or \\n characters.
	//! Entries that begin with * indicate directories that will
	//! be searched recursively.  Passing a null pointer or an
	//! empty string sets the path to nothing; not even the
	//! current directory will be searched if findFile is called.
	void setPath( const char * new_path );

	//! Resets the path by reading a new path from the named file.
	//! The file should be plain text and formatted as described
	//! for setPath().  Returns 0 on success or -1 if the file
	//! could not be read.
	int readPath( const char * pathfile );

	//! Find a file using the search path and return its
	//! fully qualified name in the supplied buffer.  No more
	//! than len characters will be written to the buffer,
	//! including the the terminating \\0.  Since fully
	//! qualified names that don't fit into buf will not be
	//! tested, there is no way to distinguish file-not-found
	//! from the case where the file exists, but the buffer
	//! was too small to hold its name.  However, a buffer size
	//! of PATH_MAX + 1 (defined in limits.h) should be large
	//! enough to hold any valid file name.  The buffer may be
	//! modified even if the search fails.  The return value
	//! indicates the length of the complete file name 
	//! (if found), or -1 if no matching file was found.
	//! \a continueSearch is a pointer to an option callback.
	//! This function is called periodically during the search;
	//! if it returns 0, the search is aborted and no file
	//! is returned, otherwise the search continues.  This
	//! gives the function a way to check for user cancellation
	//! of long running searches.
	int findFile( const char * filename, char * buf, int len,
		   int (*continueSearch)() = NULL );

	//! Store the path in the named pathfile.  The individual 
	//! directory entries are separated by 'sep', and the
	//! * prefix indicates directories that are searched
	//! recursively.  Returns 0 on success or -1 if the file
	//! could not be written.
	int storePath( const char * pathfile, char sep ) const;

	//! Static member that gets a list of all the subdirectories
	//! of the specifified parent (excecpt . and ..).  Used
	//! internally, but also useful to external users.
	//! Subdirectories are separated by 'sep'.  If a nonnull 
	//! pointer is passed in path_return, the fully resolved
	//! path name of the parent directory will be stored there.
	//! The caller must ensure that this buffer is large
	//! enough (i.e., at least PATH_MAX + 1 bytes).  Also, if
	//! the string in parent isn't a valid directory name, this
	//! function will return NULL, and path_return (if defined)
	//! set to an empty string.
	static char * getSubdirs( const char * parent, char sep,
		       char * path_return = 0 );

protected:
	//! This does most of the work for finding a file in a path
	//! (except checking the case where a fully-qualified name
	//! is passed in).  It is separate from the public version
	//! because the public version is the top level of any
	//! recursive seach, so it can set up and tear down that
	//! table directories seen.  This table is passed in
	//! \a dirs_seen.
	int findFileInPath( const char * filename, char * buf, int len,
		       STRING_Symbol_Table * dirs_seen, int allrecur,
		       int (*continueSearch)() = NULL );
	void cleanUp();

	struct PathEntry {
		PathEntry()
		{ }
		~PathEntry()
		{	delete subdirs;
			delete dir;
		}
		char * dir;
		int len;
		int recursive;
		SearchPath * subdirs;
		bool subdirs_checked;
		struct PathEntry * next;
	};

	PathEntry * head;
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

