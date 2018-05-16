//! \file filecollection.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 15 April 2004

#ifndef FILECOLLECTION_H
#define FILECOLLECTION_H

// The symbol table headers below need this.
#include <stdio.h>

// For efficiency, use symbol tables to speed up lookup of items.
// These symbol tables are based on the md symbol table algorithms
#include <int_symbol.h>
#include <string_symbol.h>

// Migrate over to C++ wrappers for symbol tables, where possible.
// For efficiency, use symbol tables to speed up lookup of items.
// These symbol tables are based on the md symbol table algorithms
#include <inttable.h>
#include <stringtable.h>

#include <qobject.h>
#include <qstring.h>
#include <qprogressdialog.h>
#include <qtimer.h>

//! Repository for source file contents.

//! Responds to requests for specific lines from specific source
//! files, and reads these files (through the collector) as needed.
class FileCollection : public QObject {
	Q_OBJECT

public:
	//! Constructor needs the name of the socket to the
	//! collector so it can request source file information.
	//! No source data is stored initially.
	FileCollection();

	//! Destructor removes all stored source files.
	~FileCollection();
	
	//! Clears the source cache so that any requests for file source or
	//! size will reload the file (usually used after changing
	//! search path).
	void clear()
	{ 
		// Delete all existing source file info
		fileInfoTable.deleteAllEntries();

		// Let source viewers know they should reload
		emit cleared();
	}
  
	//! Returns the specified source line from the specified file.
	//! Returns NULL_QSTRING if file unavailable or lineNo out of
	//! bounds.  For files not previously loaded, this function
	//! issues a request to get the file and does not return
	//! until the file has been loaded (or determined to be
	//! unavailable).  However, events will be processed while
	//! the function is waiting for the file.
	QString getSourceLine (const char *fileName, int lineNo);

	//! QString version of getSourceLine(const char *, int)
	QString getSourceLine (const QString &fileName, int lineNo);

	//! Returns the number of lines in the source file
	//! returns 0 if file is unavailable or empty.
	//! For files not previously loaded, this function
	//! issues a request to get the file and does not return
	//! until the file has been loaded (or determined to be
	//! unavailable).  However, events will be processed while
	//! the function is waiting for the file.
	int getSourceSize (const char *fileName);
 
	//! QString version of getSourceSize(const char *)
	int getSourceSize (const QString &fileName);

	//! Returns the full path to the source file where it was found
	//! (if known), or the file name passed in if this file source
	//! has not been read.
	QString getSourcePath (const char *fileName);

	//! QString version of getSourcePath(const char *)
	QString getSourcePath (const QString &fileName);

	//! Inserts a character buffer containing the full text of
	//! a source file into the database.  This function is
	//! intended as a callback when the FileCollection requests a
	//! file over a socket connection.
	//! This routine modifies fileBuf (changes newlines to string
	//! terminators)!
        //! The size_estimate, if specified, is used to process large files
        //! more efficiently (so useful to specify, if known).
	void insertSourceFile(char * fileBuf, int id, int size_estimate=-1 );

	//! Associates a full path name (or any other string) with an
	//! outstanding request for a source file.  This is normally
	//! used as a callback when the collector is reporting a the
	//! full name of a requested file.  The id is the integer
	//! assigned to this request when it was issued, and this function
	//! can only be called before insertSourceFile is called, since
	//! that function invalidates the id.
	void setFullPath( char * fullPath, int id );

	//! Sets the socket to be used for requesting files.
	void setRemoteSocket( int remoteSocket )
	{	sock = remoteSocket; }

	//! Retrieves the remote socket.  -1 means it hasn't been set yet.
	int getRemoteSocket( void ) const
	{	return sock; }
	
public slots:
	void reportWindowClosed();
signals:
	//! Emitted when all the files in the collection have been removed.
	void cleared();

protected:
	//! Internal structure to cache one source file.
	struct FileInfo 
	{
		//! Set to true if file source could not be read
		bool unavailable;

		//! Max line number in file (first line is line 1)
		int maxLineNo;

		//! Store the name so it's available even
		//! when this object is indexed by something else
		QString fileName;

		//! Store the full path name separately, since the
		//! fileName above (which is used to uniquely identify
		//! this file in tables) may or may not be the full path.
		//! However, the user may want to know the full 
		//! path where the file was found.
		QString fullPath;

		//! Store lines in table, look up with line number
		IntTable<QString> lineTable;


		FileInfo( const char * name ) :
			unavailable( TRUE ), maxLineNo( 0 ), fileName( name ),
			// Create line table that deletes QStrings on deletion
			lineTable ("Line", DeleteData, 0)
		{ }
		~FileInfo() {}
	};  
	
	//! Internal routine that creates FileInfo structure for fileName
	//! and fills it in with the text of the file.  Does not return
	//! until the file has been read, but it executes an event loop
	//! while waiting for the data.  Will punt (in fileInfoTable
	//! routines) if file created more than once!
	FileInfo *createFileInfo (const char *fileName);

	//! Socket used to communicated with the collector.
	int sock;

	//! Keeps track of whether we are in the middle of reading a
	//! source file over the socket connection.  If so, other
	//! activities should be disabled.
	enum sourceFileState {
		notReadingFile, readingFile, readyToParse
	} sourceState; 

	//! Stores pointers to FileInfo objects that are waiting to be
	//! filled in.  Keys to this table are passed along with the
	//! file request and are used to look up the corresponding object
	//! when the FileInfo arrives.
	IntTable<FileInfo> pendingFileInfoTable;
	int nextPendingFileInfoKey;

	//! The list of FileInfo objects in our collection.
	StringTable<FileInfo> fileInfoTable;

	//! Give the user something to look at while waiting for
	//! a file
	QProgressDialog * progress;

	//! Used for animating the busy signal when we're waiting for
	//! a file to come back.
	QTimer * updateTimer;
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

