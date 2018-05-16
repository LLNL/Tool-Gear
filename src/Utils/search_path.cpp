//! \file search_path.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 26 February 2004

#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace std;

#include "search_path.h"

#define SEPARATOR ':'
#define NEWLINE '\n'
#define RECUR_SYMBOL '*'
#define CANCEL_SEARCH (-2)
#define NOT_FOUND (-1)


SearchPath:: SearchPath()
	: head(NULL)
{ }

SearchPath:: SearchPath( const char * initial_path )
	: head(NULL)
{
	setPath( initial_path );
}

SearchPath:: ~SearchPath()
{
	cleanUp();
}

char * SearchPath:: getPath( char sep ) const
{

	// Allocate an array and copy in the path.  Figure out
	// the length needed by calling the buffer-using version
	// of this function to count the length of the path
	// with a dummy buffer.
	int length_needed = getPath( NULL, 0, 0 );

	char * out = new char[length_needed];

	if( out != NULL ) getPath( out, length_needed, sep );
	else cerr << "Not enough memory to copy path of length " <<
		length_needed << endl;

	return out;
}

int SearchPath:: getPath( char * buffer, int len, char sep ) const
{
	const PathEntry * node;
	int sofar = 0;

	// Path is empty; store the terminating \0, if possible.
	// Return value indicates a one-char string is needed.
	if( head == 0 ) {
		if( buffer != 0 && len > 0 ) {
			buffer[0] = '\0';
		}
		return 1;
	}

	for( node = head; node != 0; node = node->next ) {
		char * p = buffer + sofar;
		// Only copy if there's room, but run through the
		// whole list regardless so we know how much room
		// it would need.
		if( sofar + node->len + node->recursive <= len ) {
			// If this isn't the first item, put a separator
			// ahead of this item (where there was a \0)
			if( node != head ) *(p - 1) = sep;

			if( node->recursive ) *p++ = RECUR_SYMBOL;

			strcpy( p, node->dir );
		}
		sofar += node->len + node->recursive;
	}

	return sofar;
}

void SearchPath:: setPath( const char * new_path )
{

	// Get rid of any exisiting path
	cleanUp();

	// Be sure there's a path to set
	if( !new_path ) return;

	const char * p;
	const char * last_start;
	PathEntry * last = 0;	// avoid compiler warning
	PathEntry * new_entry;

	int finished = 0;
	for( p = last_start = new_path; !finished; ++p ) {
		// Look for separators; when we find one,
		// copy the current string into a new list node
		// Be sure to get the last item in the string,
		// which is terminated by \0
		if( *p == SEPARATOR || *p == NEWLINE || *p == '\0' ) {
			int recur;
			int length;

			if( *p == '\0' ) {
				finished = 1;
			}

			// Figure out recursiveness and length
			if( *last_start == RECUR_SYMBOL ) {
				++last_start;
				recur = 1;
			} else {
				recur = 0;
			}
			length = ( p - last_start ) + 1;

			// Skip empty path entries
			if( length <=  1 ) {
				last_start = p + 1;
				continue;
			}

			// Create a new list node
			new_entry = new PathEntry;
			if( !new_entry ) break;

			// Copy in the directory name and other data
			new_entry->dir = new char[length];
			if( !new_entry->dir ) {
				delete new_entry;
				break;
			}
			memcpy( new_entry->dir, last_start, length - 1 );
			new_entry->dir[length - 1] = '\0';
			new_entry->len = length;
			new_entry->recursive = recur;
			new_entry->subdirs = 0;	// Don't set this yet
			// Haven't looked for subdirs yet
			new_entry->subdirs_checked = FALSE;
			new_entry->next = 0;

			// Put the entry in the list
			if( head == 0 ) {
				head = new_entry;
			} else {
				last->next = new_entry;
			}
			last = new_entry;

			// Advance pointer to get ready for next item
			last_start = p + 1;
		}
	}
}

int SearchPath:: readPath( const char * pathfile )
{
	// Figure out how much data is in the file (and if it's
	// readable), then read the file and use the data to
	// set the path.

	struct stat fileinfo;
	int retval = stat( pathfile, &fileinfo );

	// Silently fail if file not found (user may want to retry)
	if( retval < 0 ) return NOT_FOUND;

	// Be sure it's a regular file
	if( !S_ISREG( fileinfo.st_mode ) ) {
		cerr << pathfile << " is not a regular file\n";
		return NOT_FOUND;
	}

	char * path_data = new char[fileinfo.st_size + 1];
	if( !path_data ) {
		cerr << "Path file too large to read\n";
		return NOT_FOUND;
	}

	int fd = open( pathfile, O_RDONLY );
	if( fd < 0 ) {
		cerr << "Failed to open path file " << pathfile << endl;
		return NOT_FOUND;
	}

	read( fd, path_data, fileinfo.st_size );
	path_data[fileinfo.st_size] = '\0';
	close( fd );

	setPath( path_data );

	delete path_data;

	return 0;
}

int SearchPath:: findFile( const char * filename, char * buf, int len,
	       int (*continueSearch)()	)
{
	struct stat fileinfo;
	int retval;

	// If the filename begins with /, just copy it to the buffer
	// and verify that it exists.
	if( filename[0] == '/' ) {
		strncpy( buf, filename, len );
		buf[len - 1] = '\0';
		retval = stat( buf, &fileinfo );
		if( retval != 0 ) return NOT_FOUND;
		else return strlen( buf );
	}

	// File name was relative, so we need to look for it in
	// our search path.
	// To avoid infinite loops in the recursive search, we'll
	// set up a temporary list of directories seen.  If we
	// see any directory a second time, we won't search it
	// again.  This also saves time when one directory is
	// linked to more than one place in the search tree.
	STRING_Symbol_Table * dirs_seen
		= STRING_new_symbol_table( "dirs_seen", 100 );

	// Do the search in the path; don't assume recursive search to start
	retval = findFileInPath( filename, buf, len, dirs_seen, 0,
			continueSearch );

	// Toss out the list of paths seen so we can start fresh next time
	STRING_delete_symbol_table( dirs_seen, NULL );
	
	return retval;
}

int SearchPath:: findFileInPath( const char * filename, char * buf, int len, 
		STRING_Symbol_Table * dirs_seen, int allrecur,
		int (*continueSearch)() )
{
	PathEntry * node;
	struct stat fileinfo;
	int actual_len = 0;	// Avoid compiler warning
	int retval = NOT_FOUND;	// Ensure that empty path searches fail

	// This function should only be called when filename is
	// relative to the search path (i.e., doesn't start with /)

	for( node = head; node != NULL; node = node->next ) {
		// See if this directory has been seen before in
		// this search; we rely on the realpath call
		// in getSubdirs to provide directory names in
		// the path that are in canonical form (no . or ..
		// or symbolic links).  If we've seen this
		// directory before, just skip it and keep going.
		if( STRING_find_symbol( dirs_seen, node->dir ) != NULL ) {
//			fprintf( stderr, "Warning: %s appears more than "
//					"once in search path\n", node->dir );
			continue;
		}

		actual_len = snprintf( buf, len, "%s/%s",
				node->dir, filename );
		if( actual_len > len ) continue;

		retval = stat( buf, &fileinfo );
		if( retval == 0 ) break;	// Found it!

		// Add this directory to the list.  Even if we aren't
		// doing a recursive search from this directory, we
		// don't want to look here again if we arrive at this
		// directory through another path.  We pass NULL data,
		// so we need to call STRING_find_symbol, not
		// STRING_find_symbol_data to detect whether the string
		// is in the table.
		STRING_add_symbol( dirs_seen, node->dir, NULL );

		// If we're doing a recursive search, see if this
		// directory has any subdirectories, and search them
		// if they exist.
		if( node->recursive || allrecur ) {
			
			// Before each recursive search, see if the
			// user has cancelled the search
			if( !(*continueSearch)() ) return CANCEL_SEARCH;

			// If we haven't already looked for subdirectories
			// for this node, look now.
			if( !node->subdirs_checked ) {
				char * subdirlist = getSubdirs( node->dir,
						SEPARATOR );
				if( subdirlist ) {
					node->subdirs
						= new SearchPath( subdirlist );
					delete subdirlist;
					if( !node->subdirs ) {
						cerr << "Ran out of memory "
							"searching for file "
							<< buf << endl;
						return NOT_FOUND;
					}
				}
				// Not an error if subdirlist is null; just
				// means there weren't any subdirectories.
				// Now that we've checked, we don't need to
				// look again. (Assumes user isn't adding
				// any at runtime!)
				node->subdirs_checked = TRUE;
			}

			// Recursively search subdirectories, if this directory
			// has any.
			if( node->subdirs
				&& ( actual_len
					= node->subdirs->findFileInPath(
						filename, buf, len, dirs_seen,
						1, continueSearch ) ) >= 0 ) {
				retval = 0;
				break;		// Found recursively
			} else if( actual_len == CANCEL_SEARCH ) {
				return CANCEL_SEARCH;
			}
		}
	}

	if( retval != 0 ) return NOT_FOUND;
	else return actual_len;
}

int SearchPath:: storePath( const char * pathfile, char sep ) const
{
	char * pathdata = getPath( sep );
	FILE * fp = fopen( pathfile, "w" );

	if( !fp ) {
		delete pathdata;
		return -1;
	}

	int retval = fputs( pathdata, fp  );
	fclose( fp );
	delete pathdata;

	return( retval < 0 ? -1 : 0 );
}

char * SearchPath:: getSubdirs( const char * parent, char sep,
		char * path_return )
{
	// Get a list of all the subdirectories of the specified
	// directory.  Begin by recording where we are and then
	// changing to the directory specified by the request.
	char fullpathbuf[PATH_MAX+1];
	char * fullpath;
	char * retstring;
	int cur_strlen = 0;
	int max_strlen = 2 * (PATH_MAX + 1);// A place to start that guarantees
					// at least two dirs fit
					
	retstring = new char[max_strlen];
	if( !retstring ) {
		return NULL;
	}

	// Caller can request return of the full path that we get here
	// by passing a nonzero pointer in path_return; otherwise, we
	// use our own buffer.
	fullpath = ( path_return != NULL ) ? path_return : fullpathbuf;

	if( realpath( parent, fullpath ) == NULL ) {
		fullpath[0] = '\0';
		delete retstring;
		return NULL;
	} 

	DIR * dirhandle = opendir( fullpath );
	if( dirhandle == NULL ) {
		fullpath[0] = '\0';
		delete retstring;
		return NULL;
	} else {
#if 0
		// From here on, we assume at least partial
		// success, so we'll start the path string.
		// First, put the full path to this directory in the
		// first position.
		strcpy( retstring, fullpath );
		cur_strlen += strlen( fullpath );	// excludes \0
#endif
		char dir_entry_path[PATH_MAX];

		struct stat si;
		struct dirent * di;
		while( (di = readdir( dirhandle )) != 0 ) {
			// Skip . and ..
			if( strcmp( di->d_name, "." ) == 0 ||
					strcmp( di->d_name, ".." ) == 0 )
				continue;
			snprintf( dir_entry_path, PATH_MAX,
					"%s/%s", fullpath, di->d_name );
			int r = stat( dir_entry_path, &si );
			// Is this file a directory?
			if( !r && S_ISDIR( si.st_mode ) ) {
				int namelen = strlen( dir_entry_path );

				// Be sure there's room (account for both
				// the separator and the \0)
				if( cur_strlen + namelen + 1 >= max_strlen ) {
					char * newstring
						= new char[max_strlen * 2];
					if( newstring == NULL ) {
						return 0;
					}
					strcpy( newstring, retstring );
					delete retstring;
					retstring = newstring;
					max_strlen *= 2;
				}

				if( cur_strlen > 0 ) {
					// Replace \0 with separator if this
					// isn't the first entry
					retstring[cur_strlen - 1] = sep;
				}

				// Now add the new directory
				strcpy( retstring + cur_strlen,
						dir_entry_path );
				cur_strlen += namelen + 1;
			}
		}
		closedir( dirhandle );
	}

	if( cur_strlen == 0 ) {	// Found no subdirs
		delete retstring;
		retstring = 0;
	}
	return retstring;
}

void SearchPath:: cleanUp()
{
	PathEntry * node;
	PathEntry * next;

	// Free up everything
	for( node = head; node != NULL; node = next ) {
		next = node->next;
		delete node;
	}

	head = 0;
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

