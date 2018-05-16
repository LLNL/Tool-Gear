// lookup_function_lines.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 10 April 2002
// Finds the starting and ending line number for a function within
// a given file name.

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "lookup_function_lines.h"

// The basic strategy is to use the ctags program to
// create an index, then parse and sort the index.  Since it's
// likely that there will be several successive requests for the
// same file, we will cache the info for the last file read (we
// could also cache all files read, but since we don't currently
// expect to jump around between files, this seems unnecessary.)

enum TagsFormat {
	UNKNOWN_FORMAT, CTAGS_NOT_FOUND, REGULAR_ETAGS, EXUBERANT_CTAGS
};

static int find_tags_program( char * tagsprogram, unsigned length, int& format );
static int get_command_output( char * command, char * target,
		char * outbuf, int length );

#define MAX_NAME_LEN (256)
#define MAX_LINE_LEN (10000)
#define MIN_FUNCS (1000)
struct FuncLine {
	char function[MAX_NAME_LEN];
	int first, last;
};
static int maxfuncs = 0;
static int numfuncs = 0;

static FuncLine ** funclines;
static char lastfile[MAX_NAME_LEN];
static char tagsprogram[MAX_NAME_LEN];
static int tags_format = UNKNOWN_FORMAT;

static int comp_func_lines( const void * e1, const void * e2 )
{
	return ( (*((const FuncLine **)e1))->first
			- (*((const FuncLine **)e2))->first );
}

static int create_func_list( char * file )
{
	// See if we've figured out the tags program and format yet
	// If we haven't tried yet, the format will be unknown, and
	// tagsprogram[0] will still be 0.  If we tried and failed,
	// tagsprogram[0] will be -1.  If we tried and succeeded
	// earlier, tags_format will be something other than UNKNOWN_FORMAT
	if( tags_format == UNKNOWN_FORMAT ) {
		int result =
			find_tags_program( tagsprogram, MAX_NAME_LEN,
					tags_format );
		if( result != 0 ) {
			tags_format = CTAGS_NOT_FOUND;
			return -1;
		}
	} else if( tags_format == CTAGS_NOT_FOUND ) {
		return -1;
	}
	
	// Keep track of the file we're looking at
	strncpy( lastfile, file, MAX_NAME_LEN - 1 );

	char outfile[MAX_NAME_LEN];
	strcpy( outfile, "/tmp/tagsXXXXXX" );
	char * args[7];
	int argIndex = 0;

	args[argIndex++] = tagsprogram;
	if( tags_format == EXUBERANT_CTAGS ) {
		args[argIndex++] = "-x";	// generate index
		// get only the symbols that look like functions
		args[argIndex++] = "--fortran-kinds=psf";
		args[argIndex++] = "--c-kinds=f";
		args[argIndex++] = "--c++-kinds=f";
	}
	args[argIndex++] = file;
	args[argIndex] = NULL;

	// Create the index file
	// Be sure that only the owner of this process can access it.
	umask(077);
	int fd = mkstemp( outfile );
	if( fd < 0 ) {
		fprintf( stderr, "couldn't create index file %s (%d)\n",
				outfile, errno );
		return -1;
	}

	pid_t tags_proc;

	if( (tags_proc = fork() ) == 0 ) {	// child
		// Redirect output to index file
		close( 1 );
		dup( fd );

		// Run the tags job
		if( execv( args[0], (char * const *) args ) == -1 ) {
			perror( args[0] );
			exit(-1);
		}
	}

	// parent; wait for the ctags process to finish
	int status;
	waitpid( tags_proc, &status, 0 );
	if( status != 0 ) {
		fprintf( stderr, "%s run failed for %s\n", args[0], file );
		return -1;
	}

	FILE * fp = fopen( outfile, "r" );
	if( fp == NULL ) {
		fprintf( stderr, "couldn't open index file %s\n", outfile );
		return -1;
	}

	// Read the index file
	char line[MAX_LINE_LEN];
	char * p;

	// Get rid of the old list, if it exists
	int i;
	for( i = 0; i < numfuncs; i++ )
		free( funclines[i] );
	if( funclines != NULL ) free( funclines );
	funclines = NULL;
	numfuncs = maxfuncs = 0;

	do {
		// Make sure the array is big enough
		if( numfuncs >= maxfuncs ) {
			maxfuncs += MIN_FUNCS;
			funclines = (FuncLine **)realloc( funclines,
				sizeof(FuncLine *) * maxfuncs );
			if( funclines == NULL ) {
				fprintf( stderr,
			"Allocation error in lookup_function_lines!\n" );
				return -1;
			}
		}

		p = fgets( line, MAX_LINE_LEN, fp );
		if( p == NULL ) break;

		funclines[numfuncs] = (FuncLine *)
			malloc( sizeof( FuncLine ) );
		if( funclines[numfuncs] == NULL ) {
			fprintf( stderr,
			"Allocation error in lookup_function_lines!\n" );
			return -1;
		}

		// Line format is: <name> <kind> <line-no> <path> <signature>
		// We just need the name and the line
		if( tags_format == EXUBERANT_CTAGS ) {
			sscanf( line, "%s %*s %d",
					funclines[numfuncs]->function,
					&(funclines[numfuncs]->first) );
		}

		numfuncs++;
	} while( p != NULL );

	fclose( fp );
	unlink( outfile );

	// We didn't find any functions in the index file
	if( numfuncs == 0 ) {
		fprintf( stderr, "no functions found in %s\n", file );
		return -1;
	}

	// At this point, we have an unsorted list of functions,
	// and the last line fields aren't filled in.  So we'll
	// now sort the list by starting line number.

	qsort( funclines, numfuncs, sizeof(FuncLine *),
			comp_func_lines );

	// With the sorted list, we can now set the end line for
	// each function by making it one less than the start line
	// for the following function.  The last function ends on
	// the last line.
	for( i = 0; i < (numfuncs - 1); i++ ) {
		funclines[i]->last = (funclines[i+1]->first) - 1;
	}

	// Now we need to fill in the last line number; best way
	// I can think of to do that is open the source file and
	// count the lines.
	
	fp = fopen( file, "r" );
	int lines = 0;
	do {
		p = fgets( line, MAX_LINE_LEN, fp );
		if( p != NULL ) ++lines;
	} while( p != NULL );
	fclose( fp );

	funclines[numfuncs - 1]->last = lines;

#if 0
	// DEBUG
	FILE *out = fopen ("funcparse.out", "w");
	for( i = 0; i < numfuncs; i++ ) {
	    fprintf (out, "%i of %i: %s start %i end %i\n", i, numfuncs,
		    funclines[i]->function, funclines[i]->first,
		    funclines[i]->last);
	}
	fclose (out);
#endif


	return 0;
}

int lookup_function_lines( char * file, int line_loc,
		int& first, int& last, char * realfunction, int max_func_len )
{

	// If we haven't cached this file, read it and create the index
	if( strncmp( file, lastfile, MAX_NAME_LEN ) != 0 ) {
		if( create_func_list( file ) != 0 ) {
			return -1;
		}
	}

	// Search through the function list for a matching function
	// We match by comparing the line number passed in to the
	// intervals found for each funciton.  This is more reliable
	// than comparing function names between source code and the
	// symbol table because names in the symbol table often have
	// platform-specific modifications (e.g. trailing _ or
	// leading . ).  In fact, we return the name of the fuction
	// as found in the source code for a given interval so the
	// caller knows the name that the user gave the funciton.
	int i;
	for( i = 0; i < numfuncs; i++ ) {
		if( line_loc >= funclines[i]->first
					&& line_loc <= funclines[i]->last ) {
			first = funclines[i]->first;
			last = funclines[i]->last;
//			fprintf( stderr, "found %s at %d to %d\n",
//					function, first, last );
			strncpy( realfunction, funclines[i]->function,
					max_func_len );
			realfunction[max_func_len-1] = 0;
			return 0;
		}
	}

	fprintf( stderr, "no function found for line %d\n", line_loc );
	return -1;

}

// There are several versions of ctags out there, some using different
// output formats and some that are not usable because they fail for
// C++ source files.  Exuberant Ctags works best and is easy to
// identify.  It might be possible to use other versions of ctags,
// but we'd need to figure out what options to use and in what situtions
// they worked incorrectly.  That's best done at configuration time, not
// at runtime, so for now we'll just look for Exuberant Ctags and accept
// nothing else.

#define WHICH_COMMAND "/usr/bin/which"
#define CTAGS_ENV_VAR "EXUBERANT_CTAGS_PATH"
// Pick a tags progam to use, and return its name and the format
// it uses.  Returns non-zero and leaves tagsprogram and format
// untouched if a program can't be found or its name would 
// overrun the buffer.
static int find_tags_program( char * tagsprogram, unsigned length, int& format )
{
	char ctags[1000];
	char version_info[1000];
	int status;

	// First look for a path in the environment variable
	char * ctags_env_path = getenv( CTAGS_ENV_VAR );
	char * ctags_path;

	// If none found, see if the default version will work
	if( ctags_env_path == NULL ) {
		status = get_command_output( WHICH_COMMAND,
				"ctags", ctags, 1000 );
		if( status ) return -1;
		char * nl = strchr( ctags, '\n' );
		if( nl ) *nl = '\0';
		ctags_path = ctags;
	} else {
		ctags_path = ctags_env_path;
	}

	// See if the version we found will work
	status = get_command_output( ctags_path, "--version",
			version_info, 1000 );

	if( status == 0 && strstr( version_info, "Exuberant" ) != NULL ) {
		if( strlen( ctags_path ) < length ) {
			strcpy( tagsprogram, ctags_path );
			format = EXUBERANT_CTAGS;
			return 0;
		} else {
			fprintf( stderr, "Path to ctags program too long!\n"
					"(%s won't fit in %d bytes)\n",
					ctags_path, length );
			return -1;
		}
	}

	// There are several other versions of ctags,
	// and it's hard to tell which will work without a complicated
	// test.  So at this point, if we didn't get Exuberant ctags,
	// we'll punt.
	
	fprintf( stderr, "This tool requires the Exuberant Ctags program.\n"
			"Please set the environment variable %s\n"
			"to the full path for the program.\n",
			CTAGS_ENV_VAR );
	if( ctags_env_path != NULL ) {
		fprintf( stderr, "%s is set to %s\n", CTAGS_ENV_VAR,
				ctags_env_path );
	} else {
		fprintf( stderr, "%s is not currently set\n", CTAGS_ENV_VAR );
	}
	return -1;
}

// Used for finding Ctags and testing it.
static int get_command_output( char * command, char * target,
		char * outbuf, int length )
{
	// First, find the path to ctags
	char * args[3];
	args[0] = command;
	args[1] = target;
	args[2] = NULL;

	pid_t which_proc;

	int to_parent[2];
	pipe( to_parent );

	if( (which_proc = fork() ) == 0 ) {	// child
		// Redirect child output to parent
		close( 1 );
		dup( to_parent[1] );

		// Exec "which" to find tags program
		if( execv( args[0], (char * const *) args ) == -1 ) {
			perror( args[0] );
			exit(-1);
		}
	}

	// Parent: wait for child, then read result
	int status;
	close( to_parent[1] );

	// read should not return until the child has closed the pipe
	ssize_t bytes_read = read(  to_parent[0], outbuf, length );
	outbuf[bytes_read] = '\0';
	close( to_parent[0] );
	waitpid( which_proc, &status, 0 );
	if( status != 0 ) {
		//fprintf( stderr, "%s exited with status %d\n",
		//		args[0], status );
		return -1;
	}

	// printf( "%s %s returns %s\n", command, target, outbuf );

	return 0;
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

