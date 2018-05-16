// filecollection.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 15 April 2004

#include <qapplication.h>
#include <stdio.h>
#include <string.h>

#include "command_tags.h"
#include "filecollection.h"
#include "tg_error.h"
#include "tg_socket.h"
#include "tg_pack.h"
#include "tg_time.h"

FileCollection:: FileCollection()
	: sock( -1 ), sourceState( notReadingFile ),
	// Create pendingFileInfo table that does not free the struct
	// on deletion (there shouldn't be any items in the table at
	// deletion anyway)
	pendingFileInfoTable( "pendingFileInfo", NoDealloc, 0 ),
	nextPendingFileInfoKey( 0 ),
	// Create fileInfo table that frees FileInfo structs on deletion
	fileInfoTable("fileInfo", DeleteData, 0),
	progress( 0 )
{ 
	connect( qApp, SIGNAL( lastWindowClosed() ),
			this, SLOT( reportWindowClosed() ) );
}

void FileCollection:: reportWindowClosed()
{
	// User wants to quit.  Since we have our own event loop
	// in this class, we need to break out of it, if it's
	// running.  At this point, our legs have been ripped 
	// out from under us, because the widget that called no
	// longer exists (it was deleted with the window that closed)
	// Therefore, we'll just notify the collector and exit.
	if( sourceState == readingFile ) {
		TG_send( sock, GUI_SAYS_QUIT, 0, 0, 0 );
		exit(0);
	}
}

FileCollection:: ~FileCollection()
{ 
	// Destructor for fileInfoTable should take care of everything.
}

QString FileCollection:: getSourceLine( const char * fileName, int lineNo )
{
        // Return QString::null if fileName NULL or empty
        if ((fileName == NULL) || (*fileName == 0))
	         return QString::null;

	// Find existing file info, if exists
	FileInfo *fileInfo = fileInfoTable.findEntry(fileName);

	// If doesn't exist, create it
	if (fileInfo == NULL)
		fileInfo = createFileInfo (fileName);
			
	// Return QString::null if file unavailable or lineNo out of bounds
	if (fileInfo->unavailable || (lineNo < 1) ||
		(lineNo > fileInfo->maxLineNo))
		return QString::null;
			
	// Lookup the source line in the table; should always find it!
	QString *line = fileInfo->lineTable.findEntry(lineNo);
			
	// Return QString.  Not checking line for NULL; it never should be!
	return (*line);		 
}		   

// QString version, provided for user convenience
QString FileCollection:: getSourceLine (const QString &fileName, int lineNo)
{		   
	// Get ascii (const char *) version of QString parameters
	const char *fileNameAscii = fileName.latin1(); 
					
	// Call const char * version of routine
	return (getSourceLine (fileNameAscii, lineNo));
}   

// Returns the number of lines in the source file
// returns 0 if file unavailable or empty
int FileCollection:: getSourceSize (const char *fileName)
{
       // Return 0 if fileName NULL or empty
       if ((fileName == NULL) || (*fileName == 0))
	  return (0);

	// Find existing file info, if exists
	FileInfo *fileInfo = fileInfoTable.findEntry(fileName);

	// If doesn't exist, create it
	if (fileInfo == NULL)
		fileInfo = createFileInfo (fileName);

	// Return maxLineNo, which should be 0 if file unavailable or empty
	return (fileInfo->maxLineNo);
}

// QString version, provided for user convenience
int FileCollection:: getSourceSize (const QString &fileName)
{
	// Get ascii (const char *) version of QString parameters
	const char *fileNameAscii = fileName.latin1();

	// Call const char * version of routine
	return (getSourceSize (fileNameAscii));
}

// Returns the full path to the source file where it was found
// (if known), or the file name passed in if this file source
// has not been read.
QString FileCollection:: getSourcePath (const char * fileName)
{
	// Return 0 if fileName NULL or empty
	if ((fileName == NULL) || (*fileName == 0))
	  return (0);

	// Find existing file info, if exists
	FileInfo *fileInfo = fileInfoTable.findEntry(fileName);

	// If doesn't exist, just return the name that was passed in
	if (fileInfo == NULL)
		return QString( fileName );

	return fileInfo->fullPath;
}

// QString version, provided for user convenience
QString FileCollection:: getSourcePath (const QString &fileName)
{
	// Get ascii (const char *) version of QString parameters
	const char *fileNameAscii = fileName.latin1();

	// Call const char * version of routine
	return (getSourcePath (fileNameAscii));
}

// Internal routine that creates FileInfo structure for file, loads source 
// if possible, and puts structure into fileInfoTable.  
// Returns pointer to FileInfo structure created.
// Will punt (in fileInfoTable routines) if file created more than once!
#define BUFFER_SIZE (1<<14)
FileCollection::FileInfo *
FileCollection::createFileInfo (const char *fileName)
{
#if 0
    // DEBUG
    TG_timestamp ("createFileInfo (%s) start \n", fileName);
#endif
	// Request the file from the collector.  Even if this
	// process is running on the same computer as the collector,
	// only the collector has the search path that the user
	// has set up for finding source files.

	// Make sure there are no other pending requests for files
	// active.  This can happen even in a single threaded
	// program because while we're processing events here
	// or below, the user could issue another request that
	// causes us to load a file (the same one or a different one).
	while( sourceState == readingFile ) {
		qApp->processEvents();
	}

	// DEBUG
//	printf( "FileCollection::createFileInfo: need %s\n", fileName );

	// See if the the preceding event loop has caused our
	// requested file to be loaded.  If so, just return the
	// record.
	FileInfo * existingFileInfo = fileInfoTable.findEntry( fileName );
	if( existingFileInfo != NULL ) {
		// DEBUG
//		printf ("FileCollection::createFileInfo: found %s "
//				"after waiting\n", fileName);
		return existingFileInfo;
	}

	// DEBUG
//	printf ("FileCollection::createFileInfo: requesting %s\n", fileName);

	// This should now be the first (and only) request for this file,
	// so allocate a record for it.
	FileInfo *fileInfo = new FileInfo( fileName) ;
	TG_checkAlloc(fileInfo);

	// Don't bother looking up empty filenames
	if( fileName == NULL || fileName[0] == '\0' ) {
		return fileInfo;
	}

	// Note that there is now a pending request
	sourceState = readingFile;

	// Send a request to read the source file.  Store the pointer
	// in an internal table and send the key so we can look up
	// this FileInfo object when the data arrives.
	pendingFileInfoTable.addEntry( nextPendingFileInfoKey,
				fileInfo );
	char buf[BUFFER_SIZE]; 
	int length = TG_pack( buf, BUFFER_SIZE, "S", fileName );
	TG_send( sock, DPCL_READ_FILE, nextPendingFileInfoKey++,
				length, buf );
	TG_flush( sock );

	// Wait until some time has passed before displaying
	// the progress dialog
	updateTimer = new QTimer();
	updateTimer->start( 2000, TRUE );	// 2 sec, single shot
	
	// Process events (including incoming file data) until
	// the file has arrived.  Once that happens, insertSourceFile
	// will take care of  everything else, and we're done.
	int counter = 0;	// Need to keep incrementing for progress bar
	bool canceled = FALSE;
	while( sourceState != notReadingFile ) {
		// If the timer has expired, update the progress bar and
		// reset the timer.
		if( updateTimer && !updateTimer->isActive() && !canceled ) {
			if( !progress ) {
				// Display a progress bar if the operation
				// is going to take a long time.  This will
				// also give the user an opportunity to
				// cancel.  We set 0 total steps so
				// that the progress bar doesn't try to show
				// how much longer we think we'll need.
				progress = new QProgressDialog(
						"Searching for source file...",
						"Stop", 0, 0, 0, TRUE );
			} else {
				progress->setProgress( counter++ ); 
			}

			// Reset the timer
			updateTimer->start( 50, TRUE );
		}

		// Keep the GUI responsive
		qApp->processEvents();

		// See if the user wants to bail out
		if( progress && progress->wasCanceled() ) {
			TG_send( sock, COLLECTOR_CANCEL_READ_FILE, 0, 0, 0 );
			// We still need to wait in this loop until
			// the collector responds with file not found,
			// but we don't want to see the dialog or keep
			// sending this request, so we'll get rid of the
			// timer and the progress dialog and tell the
			// first part of this loop not to recreate them
			canceled = TRUE;
			delete updateTimer;
			delete progress;
			updateTimer = 0;
			progress = 0;
		}
	}

	delete updateTimer;
	delete progress;
	progress = 0;
	updateTimer = 0;
#if 0
	// DEBUG
	TG_timestamp ("createFileInfo (%s) end \n", fileName);
#endif
	// DEBUG
//	printf ("FileCollection::createFileInfo: should have %s\n", fileName);
	return fileInfo;
}

void FileCollection:: setFullPath( char * fullPathName, int id )
{

	// Look up the pointer to the pending FileInfo object
	FileInfo * fileInfo = pendingFileInfoTable.findEntry( id );
	if( fileInfo == NULL ) {
		TG_error( "Failed to find matching request for file path "
				"arrived with key %d", id );
		return;
	}
	// DO NOT delete the pending entry; it will be needed when
	// the actual file text arrives.
	
	fileInfo->fullPath = fullPathName;
}

void FileCollection:: insertSourceFile(char * buf, int id, int size_estimate )
{
#if 0
    // DEBUG
    TG_timestamp ("FileCollection::insertSourceFile: start id %i\n", id);
#endif
	// Look up the pointer to FileInfo object
	FileInfo * fileInfo = pendingFileInfoTable.findEntry( id );
	if( fileInfo == NULL ) {
		TG_error( "Failed to find matching request for file info "
				"arrived with key %d", id );
		return;
	}
	pendingFileInfoTable.deleteEntry( id );

	int lineNo = 0;

	// If have size_estimate, tweak the line table so on next resize
	// that it is approximately the right size.   Since I don't want
	// to run through the entire buffer to count newlines, I am just
	// going to guess that each line is 80 characters to estimate 
	// final line count.  This is to better handle the 2,000,000 line 
	// files generated on BG/L with mpiP.
	if (size_estimate > 0)
	{
	    int line_guess = size_estimate/80;
	    fileInfo->lineTable.tweakTableResize(line_guess);
	}
	
	// Run though buffer, adding lines to lineTable at the line's number
	// This assumes that the buffer is terminated with a \0
	char * p = buf;
	char * next = buf;
	while( next != NULL && *p != '\0' ) {
		// Find the next newline and replace it with a \0
		// to delimit the line
		next = strchr( p, '\n' );
		if( next != NULL ) {
		    *next = '\0';

		    // If next after p (i.e., string not empty), 
		    // check to see if last character in string is \m and 
		    // remove it if present (source transfered from windows 
		    // may have '^M/n' at the end of each line)
		    // (To put in real ^M in emacs, hit C-q C-m, linux didn't
		    // like, so put in ASCII 13 for ^M)
		    if ((next > p) && (next[-1] == (char)13))
			next[-1] = '\0';
		}

		// Update the line number first, so line numbers start at 1
		lineNo ++;

		// Create QString out of ASCII linebuf
		QString *line = new QString (p);
		TG_checkAlloc(line);

		// Add to lineTable under lineNo
		fileInfo->lineTable.addEntry(lineNo, line);

		// Point past the current line to the start of the next one
		if( next != NULL ) {
			p = next + 1;
		}
	}

	// Update maxLineNo for file
	fileInfo->maxLineNo = lineNo;

	// Mark file as available
	fileInfo->unavailable = FALSE;

	// Put the new file data in our list of files.  It should be
	// the only one if createSourceFile did its job correctly!
	fileInfoTable.addEntry( fileInfo->fileName.latin1(), fileInfo );

	// Indicate that file has been read
	sourceState = notReadingFile;

#if 0
	// DEBUG
	TG_timestamp ("FileCollection::insertSourceFile: end id %i\n", id);
#endif
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

