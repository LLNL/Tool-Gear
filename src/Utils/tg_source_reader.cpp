//! \file tg_source_reader.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/

// John May, 30 August 2004

#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "command_tags.h"
#include "tg_source_reader.h"

#define SOURCE_PATH_FILE "SourcePath"
#define DEFAULT_SOURCE_PATH "."
#define STRING_HEADER_LEN 12
#define STRING_POINTER_LEN 10

TGSourceReader:: TGSourceReader( int s, int (*continue_search_cb)(void) )
	: sourcePath( NULL ), sock( s ), continue_cb( continue_search_cb )
{ }

TGSourceReader:: ~TGSourceReader()
{
	if( sourcePath )
		delete sourcePath;
}

void TGSourceReader:: read_file( char * buf, int id )
{

	char * fileName;
	TG_unpack( buf, "S", &fileName );
	char fullPath[PATH_MAX];
	int retval;
	struct stat fileinfo;

	if( !sourcePath ) set_source_path_from_file();
	if( sourcePath->findFile( fileName, fullPath, PATH_MAX + 1,
			       continue_cb ) < 0 ) {
		TG_send( sock, DB_FILE_READ_COMPLETE, id,
				1, "" );
		TG_flush( sock );
		return;
	} 

	// Send the full name of the file first
	TG_send( sock, DB_FILE_FULL_PATH, id, strlen( fullPath ) + 1,
			fullPath );

	// Should be successful, since filename has be validated
	stat( fullPath, &fileinfo );
	
	if( fileinfo.st_size <= 0 ) {
		TG_send( sock, DB_FILE_READ_COMPLETE, id,
				1, "" );
		TG_flush( sock );
		fprintf( stderr, "%s is an empty file\n", fullPath );
	} else {
		char * wholeFile = new char[fileinfo.st_size + 1];
		if( wholeFile == NULL ) {
			TG_send( sock, DB_FILE_READ_COMPLETE, id,
					1, "" );
			TG_flush( sock );
			fprintf( stderr, "%s is too big to read: %u bytes\n",
					fullPath, (unsigned)fileinfo.st_size );
		} else {
			int fd = open( fullPath, O_RDONLY );
			if( fd < 0 ) {
				// send error message
				fprintf( stderr, "Failed to open %s\n",
						fullPath );
			} else {
				retval = read( fd, wholeFile,
						fileinfo.st_size );
				// Client expects \0 termination
				wholeFile[fileinfo.st_size] = 0;
				close( fd );

				TG_send( sock, DB_FILE_READ_COMPLETE, id,
						fileinfo.st_size + 1,
						wholeFile );
				TG_flush( sock );

			}
			delete [] wholeFile;
		}
	}
}

void TGSourceReader:: report_search_path( char * buf )
{
	char * path_string;
	char * p1str;

	TG_unpack( buf, "S", &p1str );

	// Be sure that some path has been set
	if( !sourcePath ) {
		set_source_path_from_file();
	}

	path_string = sourcePath->getPath( ':' );

	// STRING_HEADER_LEN includes the terminating \0 for the
	// for the string header and for the actual string (no need
	// to add it to strlen).  STRING_POINTER_LEN does not
	// include the terminating \0
	int outbuflen = 2 * STRING_HEADER_LEN + STRING_POINTER_LEN
		+ (strlen( path_string ) );
	char * outbuf = new char[outbuflen];

	if( outbuf == NULL ) {
		exit( -1 );
	}

	int length = TG_pack( outbuf, outbuflen, "SS", p1str, path_string );

	TG_send( sock, GUI_SEARCH_PATH, 0, length, outbuf );
	TG_flush( sock );

	delete outbuf;
	delete path_string;
}

void TGSourceReader:: unpack_set_source_path( char * buf )
{
	char * path;
	int save_path;

	TG_unpack( buf, "IS", &save_path, &path );

	if( !sourcePath ) {
		sourcePath = new SearchPath( path );
	} else {
		sourcePath->setPath( path );
	}

	// Save path, if requested
	if( save_path == COLLECTOR_SAVE_PATH_LOCAL ) {
		save_source_path( "." );
	} else if( save_path == COLLECTOR_SAVE_PATH_HOME ) {
		save_source_path( getenv( "HOME" ) );
	}
}

void TGSourceReader:: save_source_path( char * directory )
{
	char path[PATH_MAX];
	int len;

	if( directory == NULL ) {
		fprintf( stderr, "Can't save source path in null directory\n" );
		return;
	}

	len = snprintf( path, PATH_MAX, "%s/%s", directory, SOURCE_PATH_FILE );
	if( len > PATH_MAX ) {
		fprintf( stderr, "File name for source path too long "
				"(%d bytes)\n", len );
		return;
	}

	sourcePath->storePath( path, '\n' );
}

void TGSourceReader:: unpack_get_subdirs( char * buf )
{
	char * directory;
	char * p1str;
	char * p2str;

	TG_unpack( buf, "SSS", &p1str, &p2str, &directory );

	char fullpath[PATH_MAX+1];
	char * retstring = SearchPath::getSubdirs( directory, ':', fullpath );
	int path_length = ( retstring ? strlen( retstring ) : 0 );

	// Make a buffer with room for the ptr tags and the output
	// There are 3 string headers and 2 string pointers,
	// and the path needs cur_strlen+1 bytes
	int outbuflen = (4 * STRING_HEADER_LEN) * (2 * STRING_POINTER_LEN)
		+ path_length + 1 + strlen(fullpath) + 1;
	char * outbuf = new char[outbuflen];
	if( !outbuf ) {
		delete retstring;
		path_length = 0;
	}

	int length = TG_pack( outbuf, outbuflen, "SSSS", p1str, p2str,
			fullpath, ( ( path_length > 0 ) ? retstring : "" ) );

	TG_send( sock, GUI_ADD_SUBDIRS, 0, length, outbuf );
	TG_flush( sock );

	delete outbuf;
	delete retstring;
}

void TGSourceReader:: set_source_path_from_file( void )
{

	if( !sourcePath ) {
		sourcePath = new SearchPath();
	}

	// Try to initialize from current directory; return if we succeed
	if( sourcePath->readPath( SOURCE_PATH_FILE ) >= 0 ) return;
	
	// Try to initialize from home directory
	char home_source_path_file[PATH_MAX];
	char * homedir = getenv( "HOME" );
	if( homedir ) {
		int namel = snprintf( home_source_path_file, PATH_MAX,
				"%s/%s", homedir, SOURCE_PATH_FILE );
		if( namel < PATH_MAX ) {
			// If we found a path file in the home directory, return
			if( sourcePath->readPath( home_source_path_file ) >= 0 )
				return;
		}
	}

	// No initialization file found so far; set the default path.
	sourcePath->setPath( DEFAULT_SOURCE_PATH );
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

