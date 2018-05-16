//! \file TGmpip.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.01                                              July 19, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 26 February 2002
//! This program shows how the handshake with the Client is
//! established and how the collector can send data to the
//! Client for display.
//
// mpiP tool enhanced to use the new message interface, to support
// the new search path interface, to support reading in multiple levels
// of callsite traceback, and to implement initial/partial support for
// arbitrarily long file and function names.
// John Gyllenhaal and John May
// October 2003 - March 2004
//
// mpiP tool modified to generate xml for messages instead of sending
// them directly to the Tool Gear GUI.
// John Gyllenhaal
// December 2004

// No Qt needed by this tool, make sure header files don't bring it in
#define NO_QT

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

#include "command_tags.h"
#include "lookup_function_lines.h"
#include "tg_socket.h"
#include "tg_pack.h"
#include "collector_pack.h"
#include "search_path.h"
#include "tg_source_reader.h"
#include "tg_typetags.h"
#include "lineparser.h"
#include "tempcharbuf.h"
#include "messagebuffer.h"
#include "logfile.h"
#include "tg_time.h"
#include "socketmanager.h"
#include "tg_types.h"
#include "xml_convert_dup.h"

// Thread-safe version of error string library for IBM
#ifdef THREAD_SAFE
#define USE_STRERROR_R 
#endif

#define COUNT_COLUMN "count"
#define MAX_TIME_COLUMN "max_time"
#define MIN_TIME_COLUMN "min_time"
#define MEAN_TIME_COLUMN "mean_time"
#define APP_PERCENT_TIME_COLUMN "app_percent_time"
#define MPI_PERCENT_TIME_COLUMN "mpi_percent_time"
#define STRING_HEADER_LEN 12
#define STRING_POINTER_LEN 10


// Holds stats for each CallSite for each mpi rank
struct SiteStats {
    // Times called
    int  count;

    // Timing stats
    double max;
    double mean;
    double min;
    double app_pct;
    double mpi_pct;

    // Data sent stats
    double maxSent;
    double meanSent;
    double minSent;
    double sumSent;

    // IO stats
    double maxIO;
    double meanIO;
    double minIO;
    double sumIO;

    // Initialize to obviously bad values
    SiteStats() : count(-1), max(-1), mean(-1), min (-1), app_pct(-1), 
		  mpi_pct(-1), 
		  maxSent(-1), meanSent(-1),
		  minSent(-1), sumSent(-1),
		  maxIO(-1), meanIO(-1),
		  minIO(-1), sumIO(-1)
	{}
};

// Holds location info for one level in the call stack
struct LocationInfo {
    char *filename;
    int   line_number;
    char *function;
    int   format;  // 1 indicates info available, 2 indicates only address
};

// Holds MpiP info for each call site
struct CallSite {
    int   siteID;	
    char *call;
    char *annot_function;
    char *entry_key;
    char *traceback;          // messageTraceback formatted 
    SiteStats *stats;         // Array of stats, one per rank
    LocationInfo *location;   // Array of locations, one per call stack level
    double sorting_mpi_pct;   // For sorting, aggregate MPI percent
    double sorting_sumSent;   // For sorting, aggregate bytes sent
    double sorting_sumIO;     // For sorting, aggregate IO bytes 
    int timingLineNo;	      // First line of Timing stats
    int sentLineNo;           // First line of Data Sent stats
    int ioLineNo;             // First line of I/O stats
};

// Use class to hold mpiP data and then to print out messages
// in Tool Gear's format
class MpipInfo {
 public:
    //! Initialize internal data structures from contents of the
    //! mpip file (tested on mpiP versions 2.5 and 2.6)
    MpipInfo (char *mpipFileName);

    //! Free all the data inside MpipInfo
    ~MpipInfo();


    //! Sort callsite timing messages by MPI% and then callsiteId, then format
    //! and write in XML the messageViewer the MPIP callsite timing  messages
    void writeCallsiteTimingMessages(FILE *xml_out);

    //! Sort callsite bytes sent messages by MPI% (instead of total bytes sent)
    //! and then  callsiteId, then format and write in XML the MPIP 
    //! callsite bytes sent messages
    void writeCallsiteBytesSentMessages(FILE *xml_out);

    //! Sort callsite IO messages by MPI% (instead of total IO)
    //! and then callsiteId, then format and write in XML the MPIP
    //! callsite IO messages
    void writeCallsiteIOMessages (FILE *xml_out);

    //! Create and write in XML callsite location messages in the order
    //! of the callsites.  Mainly to allow user to see each callsite location.
    void writeCallsiteLocationMessages(FILE *xml_out);

    //! Write in XML the mpiP file itself with messages indicating
    //! section headings
    void writeMpiPFileMessages(FILE *xml_out);

 private:

    //! Helper routine to read in callsite timing stats from mpip file
    void readCallsiteTimingStats(LineParser &parser);

    //! Helper routine to read in callsite data sent stats from mpip file
    void readCallsiteSentStats(LineParser &parser);

    //! Helper routine to read in callsite IO stats from mpip file
    void readCallsiteIOStats(LineParser &parser);


    //! Helper routine that creates and sends index message for the mpip file
    void writeMpiPFileIndex (FILE *xml_out, int lineNo, 
			     const char *fmt, ...);

    float      version;       // MpiP version
    char      *MPIP_settings; // MPIP env var settings (from file)
    int        trace_depth;   // callsite stack walking depth (default = 1)
    int	       num_callsites; // Number of callsites
    int        num_tasks;     // Number of mpi tasks
    CallSite  *sites;         // Array of call site info
    int        siteIdBase;    // Offset for siteIds to support mpiP 1.7

    // Line number of various interesting points in the input mpiP file
    int        commandLineNo; // @ Command 
    int        versionLineNo; // @ Version     
    int        envLineNo;     // @ MPIP env var
    int        nodesLineNo;   // First: @ MPI Task Assignment
    int        mpiTimeLineNo; // @--- MPI Time (seconds)
    int	       sitesLineNo;   // @--- Callsites
    int	       topTimeLineNo; // @--- Aggregate Time (top twenty
    int	       topSentLineNo; // @--- Aggregate Sent Message Size (top twenty
    int        topIOLineNo;   // @--- Aggregate I/O Size (top twenty
    int	       allTimeLineNo; // @--- Callsite Time statistics (...
    int	       allSentLineNo; // @--- Callsite Message Sent statistics (...
    int        allIOLineNo;   // @--- Callsite I/O statistics (all, I/O bytes)
};

char *mpip_file_name = NULL;
char *xml_filename = NULL;
TGSourceReader * sourceReader;

// By default, include references to the raw MpiP data in all messagse.
// For huge (100MB mpiP files), this really slows down the loading of
// the first message (so allow to disable for testing purposes).
int outputMpiPRefs = 1;

int main (int argc, char *argv[])
{
    // Expect exactly two arguments, 
    // the input mpiP filename and the output xml file name
    if( argc != 3 ) {
	fprintf( stderr, "Usage: %s input_mpiP_filename output_xml_filename\n",
		 argv[0] );
	return -1;
    }
    
    mpip_file_name = argv[1];
    xml_filename = argv[2];

    FILE *xml_out = fopen (xml_filename, "w");
    if (xml_out == NULL)
    {
	TG_error ("Failed to open XML output file %s!", xml_filename );

    }

    // Start the xml file 
    fprintf (xml_out, "<tool_gear>\n\n");

    // Describe the tool for the About... box
    fprintf (xml_out, 
	     "<about>\n"
	     "  <prepend>Tool Gear %4.2f's mpiP View GUI:\n"
	     "Reports timing data for MPI calls using data gathered through mpiP\n"
	     "Written (using Tool Gear's XML interface) by\n"
	     "John Gyllenhaal and John May\n"
	     "\n"
	     "The mpiP library was written by\n"
	     "Jeffrey Vetter and Chris Chambreau\n"
	     "www.llnl.gov/CASC/mpip</prepend>\n"
	     "</about>\n"
	     "\n", TG_VERSION);

    // Get file name without path
    int lastSlash = 0;
    for (int index=0; mpip_file_name[index] != 0; index++)
    {
	if (mpip_file_name[index] == '/')
	    lastSlash = index + 1;
    }
   const char *noPathFileName = &mpip_file_name[lastSlash];
    

    // Set the initial main tool title
    fprintf (xml_out,
	     "<tool_title>MpiP View - %s</tool_title>\n"
	     "\n",
	     mpip_file_name);

    // Set the initial tool status message
    fprintf (xml_out,
             "<status>Reading in %s</status>\n"
             "\n",
	     noPathFileName);
    fflush (xml_out);
	     

    // Read in mpip data from 
    MpipInfo mpipData (mpip_file_name);

    // Write sorted callsite timing statistics in Tool Gear's XML format
    mpipData.writeCallsiteTimingMessages(xml_out);
    fflush (xml_out);

    // Write sorted callsite bytes sent statistics in XML in TG's XML format
    mpipData.writeCallsiteBytesSentMessages(xml_out);
    fflush (xml_out);

    // Write sorted callsite I/O statistics in Tool Gear's XML format
    mpipData.writeCallsiteIOMessages(xml_out);
    fflush (xml_out);

    // Write callsite locations in Tool Gear's XML format
    mpipData.writeCallsiteLocationMessages(xml_out);
    fflush (xml_out);

    // Write details about the mpiP file itself with messages indicating
    // section headings (since we don't grab everything in the file)
    mpipData.writeMpiPFileMessages(xml_out);
    
    // Set the Final tool status message
    fprintf (xml_out,
             "<status>Finished reading in %s</status>\n"
             "\n",
	     noPathFileName);
    fflush (xml_out);
	     
    
    // End the xml file
    fprintf (xml_out, "</tool_gear>\n");

    // Close the xml file
    fclose (xml_out);
    
    return 0;
}

#define MAX_LEN 1024

#if 0
// This function is used in sorting the list of sites by file and
// line number
static int compareCallSitesByLine( const void * cs1, const void * cs2 )
{
	int fileCompare = strcmp( ((CallSite *)cs1)->location[0].filename,
				((CallSite *)cs2)->location[0].filename );

	if( fileCompare ) return fileCompare;

	return (((CallSite *)cs1)->location[0].line_number - 
		((CallSite *)cs2)->location[0].line_number);
}

// This function is used in sorting the list by call site id
static int compareCallSitesByID( const void * cs1, const void * cs2 )
{
	return ((CallSite *)cs1)->siteID - ((CallSite *)cs2)->siteID;
}
#endif

// Read in mpip info file into internal data structures
MpipInfo::MpipInfo(char *mpipfile)
{
    // Initialize variables before setting/updating them below
    // We will do sanity checks below to verify they have changed 
    // from these initial values
    version = 0.0;
    MPIP_settings = NULL;
    trace_depth = 1; // (defaults to 1, set with -k in MPIP env variable)
    num_callsites = 0;
    num_tasks = 0;  
    sites = NULL;
    siteIdBase = 0; // Normally 0, but for mpiP 1.7 mid dumps, will be non-zero

    // Intialize Line number of various interesting points in the input 
    // mpiP file to -1
    commandLineNo = -1; // @ Command 
    versionLineNo = -1; // @ Version     
    envLineNo = -1;     // @ MPIP env var
    nodesLineNo = -1;   // First: @ MPI Task Assignment
    mpiTimeLineNo = -1; // @--- MPI Time (seconds)
    sitesLineNo = -1;   // @--- Callsites
    topTimeLineNo = -1; // @--- Aggregate Time (top twenty
    topSentLineNo = -1; // @--- Aggregate Sent Message Size (top twenty
    topIOLineNo = -1;   // @--- Aggregate I/O Size (top twenty
    allTimeLineNo = -1; // @--- Callsite Time statistics (all, milliseconds)
    allSentLineNo = -1; // @--- Callsite Message Sent statistics (all, sent...
    allIOLineNo = -1;   // @--- Callsite I/O statistics (all, I/O bytes)


    // Open the mpiP file for reading
    FILE * fp = fopen( mpipfile, "r" );
    if( fp == NULL ) {
	TG_error ("Failed to open mpiP file %s!", mpipfile );
    }

    // Create a line parser to handle reading in arbitrarily long
    // lines and close the fp on deconstruction of parser
    LineParser parser(fp, TRUE);

    // Get pointer to const char * line returned by lineparser
    const char *line = NULL;
    
    // Read the file and look for the list of call sites
    // Also read in pieces of the file header in the process
    while ((line = parser.getNextLine()) != NULL)
    {
	// Record location of @ Command header
	if (strncmp (line, "@ Command", 9) == 0)
	{
	    // Record lineNo for this point in the file
	    commandLineNo = parser.lineNo();
	}

	// Read in mpiP version line
	if (strncmp (line, "@ Version", 9) == 0)
	{
	    char versionString[200];

	    // Record lineNo for this point in the file
	    versionLineNo = parser.lineNo();
	    
	    // Line format is: @ Version                  : 2.6
	    if (sscanf( line, "@ Version : %f", &version) != 1)
		TG_error ("Error reading in Mpip version!");

	    // Warn if haven't tested with this version of mpiP
	    // Tested with 2.8.5 and the upcoming 2.9 was promised to
	    // have the same format - JCG 3/15/06
	    // Upcoming 3.0 release has same default format, so update
	    // version.   New concise format not yet supported -JCG 3/19/06
	    // Currently read 2.8.5 as 2.8, so this is only rough version info
	    if (version > 3.01)
	    {
		// Default to converted from float string
		sprintf (versionString, "%g", version);

		// Parse string out of raw input, so can handle 2.8.5
		sscanf ( line, "@ Version : %s", versionString);
		fprintf (stderr, 
			 "Warning: MpiP version %s has not been tested with "
			 "this reader!\n",  versionString);
	    }

#if 0
	    // 2.7 now fully supported.
	    // Warn about limited support for 2.7 right now
	    if ((version < 2.701) && (version > 2.699))
	    {
		fprintf (stderr, 
			 "Warning: Only subset of MpiP 2.7 currently "
			 "supported by this reader!\n");
	    }
#endif
	}

	// Read in MPIP env var setting and look for -k <n> setting in order
	// to get the expected trace_depth
	if (strncmp (line, "@ MPIP env var", 14) == 0)
	{
	    // Record lineNo for this point in the file
	    envLineNo = parser.lineNo();

	    // Line format is: @ MPIP env var             : -k 4 -n
	    // (Space between k and number is optional)

	    // Easiest to just grab the string after :, so
	    // advance the pointer to that point
	    const char *ptr = line;
	    while (*ptr != 0)
	    {
		// If found ':', advance to second character after it
		if (*ptr == ':')
		{
		    // Don't advance 2 if goes past terminator
		    if (ptr[1] == 0)
			ptr++;
		    else
			ptr+=2;
		    break;
		}
		ptr++;
	    }

	    // Copy the environment variable setting
	    MPIP_settings = strdup (ptr);

	    // Remove newline terminator
	    MPIP_settings[strlen(MPIP_settings)-1] = 0;

	    // If not '[null]', search for -k trace_depth in configuration
	    if (strcmp (MPIP_settings, "[null]") != 0)
	    {
		// Start scanning at start of settings
		ptr = MPIP_settings;

		// Search for -k
		while (*ptr != 0)
		{
		    // Do we have "-k"? (number may follow k directly)
		    if ((ptr[0] == '-') &&
			(ptr[1] == 'k')) 
		    {
			// Yes, convert next argument to number (atoi ignores
			// any leading whitespace)
			int level = atoi (&ptr[2]);
		       
			// If a valid number, atoi returns it
			if (level >= 1)
			    trace_depth = level;
			
			break;
		    }
		    
		    // No, goto next character
		    ptr++;
		}
	    }
	}

	// Read in MPI Task Assignments to file number of tasks
	if (strncmp (line, "@ MPI Task Assignment", 21) == 0)
	{
	    // Record lineNo for this point in the file (first match only)
	    if (nodesLineNo == -1)
		nodesLineNo = parser.lineNo();

	    // Line format is: @ MPI Task Assignment      : 3 berg01.llnl.gov
	    int rank;
	    if (sscanf( line, "@ MPI Task Assignment : %i", &rank) != 1)
		TG_error ("Error reading in MPI Task Assignment!");
	    
	    // Update number of tasks, if necessary
	    if ( num_tasks < (rank + 1))
		num_tasks = rank + 1;
	}

	// Record location of @--- MPI Time (seconds) header
	if (strncmp (line, "@--- MPI Time (seconds)", 23) == 0)
	{
	    // Record lineNo for this point in the file
	    mpiTimeLineNo = parser.lineNo();
	}

	// Stop when find the list of callsites 
	// and get the number of callsites from the header
	if( strncmp( line, "@--- Callsites:", 15) == 0)
	{
	    // Record lineNo for this point in the file
	    sitesLineNo = parser.lineNo();

	    // Line format is: @--- Callsites: 14 -------------------
	    if (sscanf (line, "@--- Callsites: %i", &num_callsites) != 1)
		TG_error ("Error reading in number of callsites!");

	    break;
	}
    } 

    // Sanity check, should not reach end of file!
    if( line == NULL ) 
	TG_error ("Error reading mpiP file: no callsite list");

    // Sanity check, version must be set by now
    if (version < 0.1)
	TG_error ("mpiP version not found!");

    // Sanity check, MPIP_settings must be set by now
    if (MPIP_settings == NULL)
	TG_error ("MPIP env var not found!");
    
    // Sanity check, number of tasks should be set by now
    if (num_tasks < 1)
	TG_error ("MPI Task Assignment (and thus number of tasks) not found!");

    // Sanity check, num_callsites should be > 0
    if (num_callsites < 1)
	TG_error ("Expect number of callsites (%i) > 0!", num_callsites);


    // First we'll build a list of callsite data objects, then
    // sort them so that they appear in file/line order.  Only
    // then can we report everything to the Client, since we
    // want everything from a given function to be sent together.
    sites = new CallSite[num_callsites];

    // For each callsite, create a stats array
    for(int i = 0; i < num_callsites; ++i ) 
    {
	// Sanity check, set this site's structure to obviously bad values
	sites[i].siteID = -1;
	sites[i].call = NULL;
	sites[i].annot_function = NULL;
	sites[i].entry_key = NULL;
	sites[i].traceback = NULL;
	sites[i].sorting_mpi_pct = -1.0;
	sites[i].sorting_sumSent = -1.0;
	sites[i].sorting_sumIO = -1.0;
	sites[i].timingLineNo = -1;
	sites[i].sentLineNo = -1;
	sites[i].ioLineNo = -1;

	// For each site, now need an array of stats, one for each task
	// plus 1 more for the "aggregate" stats
	sites[i].stats = new SiteStats[num_tasks + 1];

	// For each level in the call stack traced (minimum one), 
	// create a LocationInfo structure
	sites[i].location = new LocationInfo[trace_depth];

	// Sanity check, initialize LocationInfo to obviously bad values
	for (int d = 0; d < trace_depth; ++d)
	{
	    sites[i].location[d].filename = NULL;
	    sites[i].location[d].line_number = -1;
	    sites[i].location[d].function = NULL;
	    sites[i].location[d].format = -1;
	}
    }

    
    // Throw out the next two lines
    parser.getNextLine();
    parser.getNextLine();
    
    int i;

    // Create resizable temporary buffers for the function name, call site,
    // and filename
    TempCharBuf functionBuf, callBuf, filenameBuf;
    
    // Create an automatically expandable character buffer to hold the
    // traceback info
    MessageBuffer tbuf;

    // Get the first line
    for( i = 0; i < num_callsites; ++i ) {
	int line_number, siteID, level;
	char *function, *call, *filename;
	int levels_processed;
	
	// Scan in all the lines to do with this site id, should process
	// at least one level
	levels_processed = 0;
	while (1)
	{
	    // Peek at the next line to see if it belongs to this site
	    line = parser.peekNextLine();
	    if (line == NULL)
	    {
		TG_error("Error reading mpiP file: "
			 "callsite list ends unexpectedly at site %i!",  i);
	    }

	    // Resize temporary buffers to be big enough to handle entire
	    // line.  To reduce overhead, just use parser's max length
	    // (instead of counting each strings length).
	    int maxSize = parser.getMaxLen();
	    function = functionBuf.resize(maxSize);
	    call = callBuf.resize(maxSize);
	    filename = filenameBuf.resize(maxSize);


	    // Set all variables to be read in to invalid values
	    siteID = -1;
	    level = -1;
	    line_number = -1;
	    filename[0] = 0;
	    function[0] = 0;
	    call[0] = 0;
	    int format = -1; 

	    // Detect end of call sites
	    if (line[0] == '-')
	    {
		// Sanity check, should have processed at least on level
		if (levels_processed < 1)
		{
		    TG_error("Error reading mpiP file: "
			     "Expected callsite %i info, not end marker!\n",
			    i+1);
		}

		// The siteID = -1 will cause loop to exit below
		siteID = -1;
	    }

	    // Otherwise process line, should be valid
	    else
	    {
		// Two line formats, either: 
		//
		//   site-id level file-name line func mpi-call
		//
		// or when callsite location info is not available:
		//
		//   site-id level Address [unknown] mpi-call
		// 
		// Try to parse the first format first:
		format = 1;  // Mark trying format 1
		int parse_count = 
		    sscanf( line, "%d %d %s %d %s %s", &siteID, &level, 
			    filename, &line_number, function, call );

		// If in format one, expect 6 items read for level 0 or
		// 5 items read in for level 1.  If didn't get either of
		// these, try format two.
		if (((level == 0) && (parse_count != 6)) ||
		    ((level != 0) && (parse_count != 5)))
		{
		    // reset all variables (except call) to be read in to 
		    // invalid values
		    siteID = -1;
		    level = -1;
		    line_number = -1;
		    filename[0] = 0;
		    function[0] = 0;

		    // Try parsing format two, address goes into filename.
		    int parse_count2 = 
			sscanf( line, "%d %d %s %s %s", 
				&siteID, &level, filename, 
				function, call );

		    // If at level 0, expect 5 items read
		    // If at level 1, expect 4 items read
		    if (((level == 0) && (parse_count2 == 5)) ||
			((level != 0) && (parse_count2 == 4)))
		    {
			// Sanity check, make sure not new format
			// that we haven't seen before.
			// Expect function to be '[unknown]' for format 2
			if (strcmp (function, "[unknown]") != 0)
			{
			    TG_error ("Error reading callsite %i info "
				      "(format two)!\n"
				      "Expected '[unknown]' not '%s' "
				      "for Parent_Funct!\n"
				      "'%s'\n", i, function, line);
			}
			
			// Mark that this is format 2
			format = 2;
		    }
		    // Otherwise, don't know how to parse this line
		    else
		    {
			TG_error ("Error reading callsite %i info for "
				  "level %i!\n"
				  "Format 1 parsed %i items!\n"
				  "Format 2 parsed %i items!\n"
				  "'%s'\n", i, level, 
				  parse_count, parse_count2, line);
		    }
		}

		// For levels past 0, call will be still set correctly,
		// since sscanf will not read it in.
	    }

	    // Set siteIdBase using the first callsite read in
	    // to deal with mpiP 1.7 mid-run dump callsites not starting
	    // with 1
	    if ((i == 0) && (levels_processed == 0) && (siteID != 1))
	    {
		// Set siteIdBase to the first siteID -1, so the normallized
		// siteID will be 1 below and where siteId is converted
		// into siteIndexes.
		siteIdBase = siteID -1;
	    }
	    

	    // If not for this siteID, exit loop
	    if ((siteID - siteIdBase) != (i+1))
	    {
		// Sanity check, should have processed at least on level
		if (levels_processed < 1)
		{
		    TG_error ("Error reading mpiP file: "
			      "Expected callsite %i info, not %i!\n",  i,
			      siteID);
		}

		// Exit this siteID's processing loop
		break;
	    }

	    // Store some level 0 info in top level sites structure
	    if (level == 0)
	    {
		sites[i].siteID = siteID;
		sites[i].call = strdup( call );
	    }

	    // Sanity check, level must be < trace_depth!
	    // Otherwise, will corrupt memory
	    if ((level <0) || (level >= trace_depth))
	    {
		TG_error ("Error reading mpiP file: level (%i) not in"
			  "expected range (0-%i)!\n",
			  level, trace_depth-1);
	    }

	    // Sanity check, this level's info better not already be set
	    if (sites[i].location[level].filename != NULL)
	    {
		TG_error ("Error reading mpiP file: callsite's %i level %i's "
			  "information already read in!!\n", i, level);
	    }
	    
	    // Copy this level's info into location array
	    sites[i].location[level].filename = XML_convert_dup (filename);
	    sites[i].location[level].line_number = line_number;
	    sites[i].location[level].function = XML_convert_dup (function);
	    sites[i].location[level].format = format;

	    // If for this siteID, consume the line by getting it
	    parser.getNextLine();

	    // Indicate that we have processed a level
	    levels_processed ++;
	}

	// Create traceback structure from call stack info read in
	// for callsite, using an automatically resizing buffer, tbuf

	// Sanity check, better have been read in
	if (sites[i].location[0].filename == NULL)
	{
	    TG_error ("Error reading mpiP file: callsite's %i level 0's "
		      "information not found!\n", sites[i].siteID);
	}

	// Handle format 1, have all info
	if (sites[i].location[0].format == 1)
	{
	    tbuf.sprintf ("  <annot>\n"
			  "    <title>%s[%d] Source</title>\n"
			  "    <site>\n"
			  "      <file>%s</file>\n"
			  "      <line>%d</line>\n"
			  "      <desc>%s</desc>\n"
			  "    </site>\n", 
			  sites[i].call,
			  sites[i].siteID,
			  sites[i].location[0].filename, 
			  sites[i].location[0].line_number,
			  sites[i].location[0].function);
	}

	// Handle format 2, only have address in code
	else if (sites[i].location[0].format == 2)
	{
	    tbuf.sprintf ("  <annot>\n"
			  "    <title>%s[%d] Source</title>\n"
			  "    <site>\n"
			  "      <desc>[Addr: %s] (unknown location, source mapping failed for this callsite address)</desc>\n"
			  "    </site>\n",
			  sites[i].call,
			  sites[i].siteID,
			  sites[i].location[0].filename);
	}
	
	// No other formats yet
	else
	{
	    TG_error ("Unexpected level 0 callsite format %i\n", 
		      sites[i].location[0].format);
	}
	    
	// Append all levels below 0 (if any)
	for (int d = 1; d < trace_depth; ++d)
	{
	    // If read in less levels than specified, stop now
	    if (sites[i].location[d].filename == NULL)
		break;

	    // Handle format 1, have all info
	    if (sites[i].location[d].format == 1)
	    {
		// Append this level's info into traceback info
		tbuf.appendSprintf ("    <site>\n"
				    "      <file>%s</file>\n"
				    "      <line>%d</line>\n"
				    "      <desc>%s</desc>\n"
				    "    </site>\n", 
				    sites[i].location[d].filename, 
				    sites[i].location[d].line_number, 
				    sites[i].location[d].function);
	    }

	    // Handle format 2, only have address in code
	    else if (sites[i].location[d].format == 2)
	    {
		tbuf.appendSprintf ("    <site>\n"
				    "      <desc>[Addr: %s] (unknown location, source mapping failed for this callsite address)</desc>\n"
				    "    </site>\n", 
				    sites[i].location[d].filename);
	    }

	    // No other formats yet
	    else
	    {
		TG_error ("Unexpected level %i callsite format %i\n", 
			  d, sites[i].location[d].format);
	    }

	}

	// Append end annot XML marker
	tbuf.appendSprintf ("  </annot>\n");

	// Store duplicate of the formated "trackback" string for ease of
	// use in generating messages
	sites[i].traceback = tbuf.strdup();
    }

    
    // Look for the beginning of the stats sections and read in
    // those sections we care about
    while ((line = parser.getNextLine()) != NULL)
    {
	// Record location of @--- Aggregate Time (top twenty header
	if (strncmp (line, "@--- Aggregate Time (top twenty", 31) == 0)
	{
	    // Record lineNo for this point in the file
	    topTimeLineNo = parser.lineNo();
	}

	// Record location of @--- Aggregate Sent Message Size (top twenty
	if (strncmp (line, 
		     "@--- Aggregate Sent Message Size (top twenty", 44) == 0)
	{
	    // Record lineNo for this point in the file
	    topSentLineNo = parser.lineNo();
	}

	// Record location of @--- Aggregate I/O Size (top twenty header
	if (strncmp (line, "@--- Aggregate I/O Size (top twenty", 35) == 0)
	{
	    // Record lineNo for this point in the file
	    topIOLineNo = parser.lineNo();
	}


	// Read in (and record location) of callsite timing stats
	if( (strncmp( line, "@--- Callsite statistics (all, milliseconds", 43) == 0 ) ||
	    (strncmp( line, "@--- Callsite Time statistics", 29 ) == 0 ))
	    
	{
	    // Use helper routine to read callsite timing stats
	    readCallsiteTimingStats(parser);
	}

	// Read in (and record location) of callsite bytes sent stats
	if ((strncmp (line, 
		      "@--- Callsite statistics (all, sent bytes", 41) == 0) ||
	    (strncmp (line,
		      "@--- Callsite Message Sent statistics", 37) == 0))
	{
	    // Use helper routine to read callsite Sent bytes stats
	    readCallsiteSentStats(parser);
	}

	// Read in (and record location) of callsite I/O stats
	if (strncmp (line, "@--- Callsite I/O statistics", 28) == 0)
	{
	    // Use helper routine to read callsite IO stats
	    readCallsiteIOStats(parser);
	}

    } 
    
    // Destructor in parser automatically closes fp
    // fclose( fp );
}


// Delete all internal data structures used
MpipInfo::~MpipInfo()
{
    // Free all memory allocated for each callsite
    // Note mixture of free and delete [] is needed
    for( int i = 0; i < num_callsites; ++i ) 
    {
	// Delete all the strdups, if exist
	if (sites[i].call != NULL)
	    free( sites[i].call );
	if (sites[i].entry_key != NULL)
	    free( sites[i].entry_key );
	if (sites[i].annot_function != NULL)
	    free( sites[i].annot_function );
	if (sites[i].traceback != NULL)
	    free( sites[i].traceback );

	// Delete the array of stats for each callsite
	delete [] sites[i].stats;

	// Delete the strings strduped in location
	for (int d = 0; d < trace_depth; ++d)
	{
	    // May not exists at all levels, so free only if exists
	    if (sites[i].location[d].filename != NULL)
		free (sites[i].location[d].filename);

	    // May not exists at all levels, so free only if exists
	    if (sites[i].location[d].function != NULL)
		free (sites[i].location[d].function);
	}

	// Delete the array of locations for each callsite
	delete [] sites[i].location;
    }

    // Free copy of MPIP environment variable settings
    if (MPIP_settings != NULL)
    {
	free (MPIP_settings);
    }

    // Delete array of call sites
    delete [] sites;
}

// Helper routine to read in callsite timing stats from mpip file
void MpipInfo::readCallsiteTimingStats(LineParser &parser)
{
    // Record lineNo for this point in the file
    allTimeLineNo = parser.lineNo();

    // Get pointer to const char * line returned by lineparser
    const char *line = NULL;

    // Throw out the next line
    parser.getNextLine();
    
    // Get the header for the callsite stats
    const char *header_line;
    header_line = parser.getNextLine();
    
    while(1) 
    {
	int site;
	char call[MAX_LEN];
	char rank[100];	// string b/c it might be '*'
	int count;
	double max, mean, min, app_pct, mpi_pct;
	
	line = parser.getNextLine();
	
	if( line[0] == '-' ) break;	// end of data
	
	// Skip blank lines
	if (strlen (line) < 5)
	    continue;
	
	// Parse all the data values in the line
	if (sscanf( line, "%s %d %s %d %lf %lf %lf %lf %lf",
		    call, &site, rank, &count,
		    &max, &mean, &min, &app_pct, &mpi_pct ) != 9)
	{
	    TG_error ("Error parsing callsite statistics!  Could not parse\n"
		      "  '%s'!\n", line);
	}
	
	// Determine task id from 'rank' string (may be *).  Set
	// task to max_task + 1 if '*'.
	int task;
	if (rank[0] == '*')
	    task = num_tasks;
	else
	    task = atoi (rank);

	// Sanity check, make sure siteId within expected bounds
	// Now have to subtract siteIdBase to get 1 to num_callsites range
	// due to mpiP 1.7 output with mid-run dumps.
	if (((site - siteIdBase) < 1) || ((site - siteIdBase) > num_callsites))
	{
	    TG_error ("SiteId (%i) out of bounds (%i-%i) in line:\n"
		      "  '%s'!", site, 1+siteIdBase, num_callsites+siteIdBase,
		      line);
	}

	// To fit within C array, subtract 1 from siteId to get siteindex
	int siteindex = (site - siteIdBase) - 1;
	
	// Sanity check, make sure taskId within expected bounds
	// (Yes > than num_tasks, not >= num_tasks. '*' id is num_tasks)
	if ((task < 0) || (task > num_tasks))
	{
	    TG_error ("TaskId (%i) out of bounds (0-%i) in line:\n"
		      "  '%s'!", task, num_tasks, line);
	}

	// Sanity check, make sure count is positive (don't think legal
	// to be 0, but we will just make sure not negative for now
	// so doesn't conflict with -1 markings this reader uses)
	if (count < 0)
	{
	    TG_error ("Count (%i) < 0 in line:\n",
		      "  '%s'!", count);
	}

	// Save statistics in the callsite stats array, indexed by taskId
	// Note, must subtract 1 fro site to get sites index!
	sites[siteindex].stats[task].count = count;
	sites[siteindex].stats[task].max = max;
	sites[siteindex].stats[task].mean = mean;
	sites[siteindex].stats[task].min = min;
	sites[siteindex].stats[task].app_pct = app_pct;
	sites[siteindex].stats[task].mpi_pct = mpi_pct;

	// If have not set timingLineNo for this siteindex yet, set it
	if (sites[siteindex].timingLineNo == -1)
	    sites[siteindex].timingLineNo = parser.lineNo();

	// If have aggregate mpi_pct, set site's sorting_mpi_pct with
	// it for use by compareCallSitePtrsByMpiPct()
	if (task == num_tasks)
	    sites[siteindex].sorting_mpi_pct = mpi_pct;
	
    }

    // DEBUG
#if 0
    // Sanity check, make sure count filled in for each callsite and task
    // DISABLED by JMM 5/24/04 -- it's possible for callsites to be
    // reached by only some of the tasks, so a legal mpiP file might
    // fail this test.
    for(int i = 0; i < num_callsites; ++i ) 
    {
	for (int r = 0; r < (num_tasks + 1); r++)
	{
	    if (sites[i].stats[r].count == -1)
		fprintf (stderr, "Not all stats filled in!  Missing site %i stat %i (min %lf)!\n",
		       i, r, sites[i].stats[r].min);
	}
    }
#endif
}

// Helper routine to read in callsite data sent stats from mpip file
void MpipInfo::readCallsiteSentStats(LineParser &parser)
{
    // Record lineNo for this point in the file
    allSentLineNo = parser.lineNo();
    
    // Get pointer to const char * line returned by lineparser
    const char *line = NULL;

    // Throw out "-----------------------------------------------"
    parser.getNextLine();

    // Throw out "Name Site Rank Count Max Mean Min Sum"
    parser.getNextLine();

    // Parse all the data lines
    while(1)
    {
        int site;
        char call[MAX_LEN];
        char rank[100]; // string b/c it might be '*'
        int count;
        double maxSent, meanSent, minSent, sumSent;

	// Get the next line
        line = parser.getNextLine();

	// Detect end of data
        if( (line == NULL) || (line[0] == '-') ) 
	    break;     

        // Skip blank lines
        if (strlen (line) < 5)
            continue;

	// Parse all the data values in the line
        if (sscanf( line, "%s %d %s %d %lf %lf %lf %lf",
                    call, &site, rank, &count,
                    &maxSent, &meanSent, &minSent, &sumSent) != 8)
        {
            TG_error ("Error parsing callsite bytes sent statistics!  "
		      "Could not parse:\n"
                      "  '%s'!\n", line);
        }

        // Determine task id from 'rank' string (may be *).  Set
        // task to max_task + 1 if '*'.
        int task;
        if (rank[0] == '*')
            task = num_tasks;
        else
            task = atoi (rank);

        // Sanity check, make sure siteId within expected bounds
	// Now have to subtract siteIdBase to get 1 to num_callsites range
	// due to mpiP 1.7 output with mid-run dumps.
        if (((site - siteIdBase) < 1) || ((site - siteIdBase) > num_callsites))
        {
            TG_error ("SiteId (%i) out of bounds (%i-%i) in line:\n"
                      "  '%s'!", site, 1+siteIdBase, num_callsites+siteIdBase,
		      line);
        }

        // To fit within C array, subtract 1 from siteId to get siteindex
        int siteindex = (site -siteIdBase) - 1;

        // Sanity check, make sure taskId within expected bounds
        // (Yes > than num_tasks, not >= num_tasks. '*' id is num_tasks)
        if ((task < 0) || (task > num_tasks))
        {
            TG_error ("TaskId (%i) out of bounds (0-%i) in line:\n"
                      "  '%s'!", task, num_tasks, line);
        }

        // Save statistics in the callsite stats array, indexed by taskId
        // Note, must subtract 1 from site to get sites index!
        sites[siteindex].stats[task].maxSent = maxSent;
        sites[siteindex].stats[task].meanSent = meanSent;
        sites[siteindex].stats[task].minSent = minSent;
        sites[siteindex].stats[task].sumSent = sumSent;

	// If have not set sentLineNo for this siteindex yet, set it
	if (sites[siteindex].sentLineNo == -1)
	    sites[siteindex].sentLineNo = parser.lineNo();


#if 0
	// Sanity check, expect count to be the same as earlier data
	// DISABLED by JMM 5/24/04.  mpiP's bytes-sent stats do not
	// currently do not currently include calls that send
	// zero bytes, although these calls are included in timing
	// stats.  This discrepancy can cause this test to fail for
	// valid mpiP files.
        if(sites[siteindex].stats[task].count != count)
	{
            TG_error ("Site %i Task %i: data sent count mismatch "
		      "(%i != %i) in line:\n  '%s'!", site, task, 
		      sites[siteindex].stats[task].count, count, line);
	}
#endif

	// If have aggregate sumSent, set site's sorting_sumSent with
        // it for use by compareCallSitePtrsBysumSent()
        if (task == num_tasks)
            sites[siteindex].sorting_sumSent = sumSent;
    }
}

// Helper routine to read in callsite IO stats from mpip file
void MpipInfo::readCallsiteIOStats(LineParser &parser)
{
    // Record lineNo for this point in the file
    allIOLineNo = parser.lineNo();
    
    // Get pointer to const char * line returned by lineparser
    const char *line = NULL;

    // Throw out "-----------------------------------------------"
    parser.getNextLine();

    // Throw out "Name Site Rank Count Max Mean Min Sum"
    parser.getNextLine();

    // Parse all the data lines
    while(1)
    {
        int site;
        char call[MAX_LEN];
        char rank[100]; // string b/c it might be '*'
        int count;
        double maxIO, meanIO, minIO, sumIO;

	// Get the next line
        line = parser.getNextLine();

	// Detect end of data
        if( (line == NULL) || (line[0] == '-') ) 
	    break;     

        // Skip blank lines
        if (strlen (line) < 5)
            continue;

	// Parse all the data values in the line
        if (sscanf( line, "%s %d %s %d %lf %lf %lf %lf",
                    call, &site, rank, &count,
                    &maxIO, &meanIO, &minIO, &sumIO) != 8)
        {
            TG_error ("Error parsing callsite bytes sent statistics!  "
		      "Could not parse:\n"
                      "  '%s'!\n", line);
        }

        // Determine task id from 'rank' string (may be *).  Set
        // task to max_task + 1 if '*'.
        int task;
        if (rank[0] == '*')
            task = num_tasks;
        else
            task = atoi (rank);

        // Sanity check, make sure siteId within expected bounds
	// Now have to subtract siteIdBase to get 1 to num_callsites range
	// due to mpiP 1.7 output with mid-run dumps.
        if (((site - siteIdBase) < 1) || ((site - siteIdBase) > num_callsites))
        {
            TG_error ("SiteId (%i) out of bounds (%i-%i) in line:\n"
                      "  '%s'!", site, 1+siteIdBase, num_callsites+siteIdBase,
		      line);
        }

        // To fit within C array, subtract 1 from siteId to get siteindex
        int siteindex = (site -siteIdBase) - 1;

        // Sanity check, make sure taskId within expected bounds
        // (Yes > than num_tasks, not >= num_tasks. '*' id is num_tasks)
        if ((task < 0) || (task > num_tasks))
        {
            TG_error ("TaskId (%i) out of bounds (0-%i) in line:\n"
                      "  '%s'!", task, num_tasks, line);
        }

        // Save statistics in the callsite stats array, indexed by taskId
        // Note, must subtract 1 from site to get sites index!
        sites[siteindex].stats[task].maxIO = maxIO;
        sites[siteindex].stats[task].meanIO = meanIO;
        sites[siteindex].stats[task].minIO = minIO;
        sites[siteindex].stats[task].sumIO = sumIO;

	// If have not set ioLineNo for this siteindex yet, set it
	if (sites[siteindex].ioLineNo == -1)
	    sites[siteindex].ioLineNo = parser.lineNo();

	// If have aggregate sumIO, set site's sorting_sumIO with
        // it for use by compareCallSitePtrsBysumIO()
        if (task == num_tasks)
            sites[siteindex].sorting_sumIO = sumIO;
    }
}


// This function is used in sorting a list of CallSite pointers by
// the percent MPI time spents in aggregate breaking ties with callsiteId
static int compareCallSitePtrsByMpiPct(const void * cs1ptr, 
				       const void * cs2ptr)
{
    const CallSite *cs1 = *((const CallSite **)cs1ptr);
    const CallSite *cs2 = *((const CallSite **)cs2ptr);
    
    // Return 1 if cs2 spends more time in MPI in aggregate
    if (cs2->sorting_mpi_pct > cs1->sorting_mpi_pct)
	return (1);

    // Return -1 if cs2 spends less time in MPI in aggregate
    if (cs2->sorting_mpi_pct < cs1->sorting_mpi_pct)
	return (-1);

    // Break ties using siteId
    return (cs1->siteID - cs2->siteID);
}

// Sort callsite timing statistic messages by MPI% and then callsiteId, then 
// format and write in XML the MPIP callsite timing messages
void MpipInfo::writeCallsiteTimingMessages(FILE *xml_out)
{
    // Create message folder for callsite statistic messages in XML format
    fprintf (xml_out, 
	     "<message_folder>\n"
	     "  <tag>timings</tag>\n"
	     "  <title>MpiP Callsite Timing Statistics (all, milliseconds)</title>\n"
	     "</message_folder>\n"
	     "\n");

    // Flush out folder, so something will be visible in GUI early for
    // huge (145MB) mpiP files
    fflush (xml_out);


    // Create pointer array for sorting callsites 
    CallSite **sortedSites = new CallSite *[num_callsites];

    // Initialize sortedSites with pointers to the unsorted array
    for (int i = 0; i < num_callsites; ++i)
    {
	sortedSites[i] = &sites[i];
    }
    
    // Sort pointers to the callsites by percent MPI and to break ties,
    // callsite id
    qsort( (void *)sortedSites, num_callsites, sizeof( CallSite *),
	   compareCallSitePtrsByMpiPct );

    // Create an automatically resized message buffer that we can create 
    // message and traceback in without fear of overflowing the buffer
    MessageBuffer mbuf, tbuf;

    // Small buffer so can append callsite to call type (i.e., Bcast[12])
    char typebuf[200];

    // Figure out how many digits required to print num_tasks
    sprintf (typebuf, "%i", num_tasks);
    int taskDigits = strlen (typebuf);

    // Print out call statistics with largest MPI % first
    for (int i = 0; i < num_callsites; ++i)
    {
	// Create callsite type and siteId identifier
	sprintf (typebuf, "%s[%i]", sortedSites[i]->call, 
		 sortedSites[i]->siteID);

	// Determine how many tasks have data filled in (count != -1)
	int taskCount = 0;
	for (int rank=0; rank < num_tasks; ++rank)
	{
	    SiteStats *stats = &sortedSites[i]->stats[rank];
	    
	    // Count tasks that are filled in by mpiP (count != -1)
	    if (stats->count != -1)
		++taskCount;
	}
    
	// Create the heading, a one-line description line for this callsite,
	// depends on callsite format (whether location info exists)
	if (sortedSites[i]->location[0].format == 1)
	{
	    mbuf.sprintf ( 
		"  <heading>%-18s %6.2f%% of MPI %6.2f%% of App   "
		"%*i/%i Tasks   %s:%i  (%s)</heading>\n",
		typebuf, 
		sortedSites[i]->stats[num_tasks].mpi_pct,
		sortedSites[i]->stats[num_tasks].app_pct,
		taskDigits, taskCount, num_tasks,
		sortedSites[i]->location[0].function, 
		sortedSites[i]->location[0].line_number,
		sortedSites[i]->location[0].filename);
	}

	// Create the heading where location info does not exist
	else if (sortedSites[i]->location[0].format == 2)
	{
	    mbuf.sprintf ( 
		"  <heading>%-18s %6.2f%% of MPI %6.2f%% of App   "
		"%*i/%i Tasks   [Addr: %s] (unknown location)</heading>\n",
		typebuf, 
		sortedSites[i]->stats[num_tasks].mpi_pct,
		sortedSites[i]->stats[num_tasks].app_pct,
		taskDigits, taskCount, num_tasks,
		sortedSites[i]->location[0].filename);
	}

	// No other formats right now
	else
	{
	    TG_error ("Creating Timing Heading: "
		      "Unexpected callsite format %i\n", 
		      sortedSites[i]->location[0].format);
	}

	// Append header for stats (first line of the message body)
	mbuf.appendSprintf ("  <body>   Task      Count     Max(ms)   Mean(ms)    "
			    "Min(ms)     MPI%%    App%%\n");

	// The stats at 'num_tasks' are the aggregate stats
	SiteStats *astats = &sortedSites[i]->stats[num_tasks];

	// Append out aggregate stats after header
	mbuf.appendSprintf ("   ALL: %10i %11.4f %10.4f %10.4f   %6.2f  %6.2f",
			    astats->count, astats->max, astats->mean, 
			    astats->min, astats->mpi_pct, astats->app_pct);

	// Append to the message a stats line for each task
	for (int rank=0; rank < num_tasks; ++rank)
	{
	    SiteStats *stats = &sortedSites[i]->stats[rank];

	    // Only print stats for tasks that reached callsite (count != -1)
	    if (stats->count != -1)
	    {
		mbuf.appendSprintf (
		    "\n%6i: %10i %11.4f %10.4f %10.4f   %6.2f  %6.2f",
		    rank, stats->count, stats->max, stats->mean, 
		    stats->min, stats->mpi_pct, stats->app_pct);
	    }
	}

	// Don't end message with '\n' unless you want a blank line
	// after the message

	// Put end body tag after message
	mbuf.appendSprintf("</body>\n");

	// Start traceback with callsite info
	tbuf.sprintf ("%s", sortedSites[i]->traceback);

	// If have timingLineNo info (not -1), append traceback to
	// the MpiP output that generated this message
	if ((sortedSites[i]->timingLineNo > -1) && (outputMpiPRefs))
	{
	    tbuf.appendSprintf ("  <annot>\n"
				"    <title>Raw MpiP Data</title>\n"
				"    <site>\n"
				"      <file>%s</file>\n"
				"      <line>%i</line>\n"
				"    </site>\n"
				"  </annot>\n", 
				mpip_file_name, sortedSites[i]->timingLineNo);
	}

	// DEBUG, stress out GUI by sending multiple of same message!
	for (int dupl = 0; dupl < 1; ++dupl)
	{

	    // Write out stats message with callsite traceback in XML
	    fprintf (xml_out, 
		     "<message>\n"
		     "  <folder>timings</folder>\n"
		     "%s"
		     "%s"
		     "</message>\n"
		     "\n", 
		     mbuf.contents(),
		     tbuf.contents());
	}
    }

    // Delete the sorted callsite info pointers (only)
    delete [] sortedSites;
}

#if 0
// Not currently used; call is commented out
// This function is used in sorting a list of CallSite pointers by
// the sum of bytes sent, in aggregate, breaking ties with callsiteId
static int compareCallSitePtrsByBytesSent(const void * cs1ptr, 
					  const void * cs2ptr)
{
    const CallSite *cs1 = *((const CallSite **)cs1ptr);
    const CallSite *cs2 = *((const CallSite **)cs2ptr);
    
    // Return 1 if cs2 sent more bytes
    if (cs2->sorting_sumSent > cs1->sorting_sumSent)
	return (1);

    // Return -1 if cs2 sent list bytes
    if (cs2->sorting_sumSent < cs1->sorting_sumSent)
	return (-1);

    // Break ties using siteId
    return (cs1->siteID - cs2->siteID);
}
#endif

// Sort callsite bytes sent messages by MPI% (instead of total bytes sent)
// and then  callsiteId, then format and write in XML the MPIP 
// callsite bytes sent messages
void MpipInfo::writeCallsiteBytesSentMessages(FILE *xml_out)
{
    // Create message folder for callsite data sent messages in XML format
    fprintf (xml_out, 
	     "<message_folder>\n"
	     "  <tag>bytesSent</tag>\n"
	     "  <title>MpiP Callsite Bytes Sent Statistics (all, bytes)</title>\n"
	     "</message_folder>\n"
	     "\n");

    // If no stats provided, send message to folder to indicate
    if (allSentLineNo == -1)
    {
	// write just one message, indicating no data
	fprintf (xml_out,
		 "<message>\n"
		 "  <folder>bytesSent</folder>\n"
		 "  <heading>(No bytes sent statistics in mpiP file)</heading>\n"
		 "</message>\n"
		 "\n");

	// Return now
	return;
    }



    // Create pointer array for sorting callsites 
    CallSite **sortedSites = new CallSite *[num_callsites];

    // Initialize sortedSites with pointers to the unsorted array
    for (int i = 0; i < num_callsites; ++i)
    {
	sortedSites[i] = &sites[i];
    }

#if 0
    // REALLY WANT SORTED BY %MPI, NOT MESSAGE SIZE
    // Sort pointers to the callsites by percent MPI and to break ties,
    // callsite id
    qsort( (void *)sortedSites, num_callsites, sizeof( CallSite *),
	   compareCallSitePtrsByBytesSent );
#endif

    // Sort pointers to the callsites by percent MPI and to break ties,
    // callsite id
    qsort( (void *)sortedSites, num_callsites, sizeof( CallSite *),
	   compareCallSitePtrsByMpiPct );

    // Create an automatically resized message buffer that we can create 
    // message and traceback in without fear of overflowing the buffer
    MessageBuffer mbuf, tbuf;


    // Small buffer so can append callsite to call type (i.e., Bcast[12])
    char typebuf[200];

    // Figure out how many digits required to print num_tasks
    sprintf (typebuf, "%i", num_tasks);
    int taskDigits = strlen (typebuf);

    // Print out call statistics with largest total bytes sent first
    for (int i = 0; i < num_callsites; ++i)
    {
	// Skip messages that mpiP didn't report about
	if (sortedSites[i]->stats[num_tasks].sumSent < 0)
	    continue;
	
	// Create callsite type and siteId identifier
	sprintf (typebuf, "%s[%i]", sortedSites[i]->call, 
		 sortedSites[i]->siteID);
    
	// Create the one-line description line for this callsite

	// Determine how many tasks have data filled in (count != -1)
	int taskCount = 0;
	for (int rank=0; rank < num_tasks; ++rank)
	{
	    SiteStats *stats = &sortedSites[i]->stats[rank];
	    
	    // Count tasks that are filled in by mpiP (count != -1)
	    if (stats->count != -1)
		++taskCount;
	}

	// Create heading, the one-line description line for this callsite,
	// depends on callsite format (whether location info exists)
	if (sortedSites[i]->location[0].format == 1)
	{
	    mbuf.sprintf ( 
		"  <heading>%-18s %6.2f%% of MPI  %13.8g Total  %13.8g Mean   "
		"%*i/%i Tasks   %s:%i  (%s)</heading>\n",
		typebuf, 
		sortedSites[i]->stats[num_tasks].mpi_pct,
		sortedSites[i]->stats[num_tasks].sumSent,
		sortedSites[i]->stats[num_tasks].meanSent,
		taskDigits, taskCount, num_tasks,
		sortedSites[i]->location[0].function, 
		sortedSites[i]->location[0].line_number,
		sortedSites[i]->location[0].filename);
	}
	// Create heading where location info does not exist
	else if (sortedSites[i]->location[0].format == 2)
	{
	    mbuf.sprintf ( 
		"  <heading>%-18s %6.2f%% of MPI  %13.8g Total  %13.8g Mean   "
		"%*i/%i Tasks   [Addr: %s] (unknown location)</heading>\n",
		typebuf, 
		sortedSites[i]->stats[num_tasks].mpi_pct,
		sortedSites[i]->stats[num_tasks].sumSent,
		sortedSites[i]->stats[num_tasks].meanSent,
		taskDigits, taskCount, num_tasks,
		sortedSites[i]->location[0].filename);
	}
	// No other formats right now
	else
	{
	    TG_error ("Creating Bytes Sent Header: "
		      "Unexpected callsite format %i\n", 
		      sortedSites[i]->location[0].format);
	}

	// Append header for stats (first line of message body)
	mbuf.appendSprintf ("  <body>   Task      Count     Max(bytes)    Mean(bytes)     Min(bytes)     Sum(bytes)\n");

	// The stats at 'num_tasks' are the aggregate stats
	SiteStats *astats = &sortedSites[i]->stats[num_tasks];

	// Append out aggregate stats after header
	mbuf.appendSprintf ("   ALL: %10i  %13.8g  %13.8g  %13.8g  %13.8g",
			    astats->count, astats->maxSent, astats->meanSent, 
			    astats->minSent, astats->sumSent);

	// Append to the message a stats line for each task
	for (int rank=0; rank < num_tasks; ++rank)
	{
	    SiteStats *stats = &sortedSites[i]->stats[rank];

	    // Only print stats for tasks that reached callsite (count != -1)
	    if (stats->count != -1)
	    {
		mbuf.appendSprintf (
		    "\n%6i: %10i  %13.8g  %13.8g  %13.8g  %13.8g",
		    rank, stats->count, 
		    stats->maxSent, stats->meanSent, 
		    stats->minSent, stats->sumSent);
	    }
	}

	// Don't end message with '\n' unless you want a blank line
	// after the message

	// Put end body tag after message
        mbuf.appendSprintf("</body>\n");


	// Start traceback with callsite info
	tbuf.sprintf ("%s", sortedSites[i]->traceback);

	// If have sentLineNo info (not -1), append traceback to
	// the MpiP output that generated this message
	if ((sortedSites[i]->sentLineNo > -1) && (outputMpiPRefs))
	{
	    tbuf.appendSprintf ("  <annot>\n"
				"    <title>Raw MpiP Data</title>\n"
				"    <site>\n"
				"      <file>%s</file>\n"
				"      <line>%i</line>\n"
				"    </site>\n"
				"  </annot>\n", 
				mpip_file_name, sortedSites[i]->sentLineNo);
	}

	// Write out stats message with callsite traceback
	fprintf (xml_out,
		 "<message>\n"
		 "  <folder>bytesSent</folder>\n"
		 "%s"
		 "%s"
		 "</message>\n"
		 "\n",
		 mbuf.contents(),
		 tbuf.contents());
    }

    // Delete the sorted callsite info pointers (only)
    delete [] sortedSites;
}

// Sort callsite IO messages by MPI% (instead of total IO)
// and then callsiteId, then format and write in XML the MPIP 
// callsite IO messages
void MpipInfo::writeCallsiteIOMessages(FILE *xml_out)
{
    // Create message folder for callsite IO messages in XML format
    fprintf (xml_out,
             "<message_folder>\n"
             "  <tag>IO</tag>\n"
             "  <title>MpiP Callsite I/O Statistics (all, I/O bytes)</title>\n"
             "</message_folder>\n"
             "\n");


    // If no stats provided, send message to folder to indicate
    if (allIOLineNo == -1)
    {
        // write just one message, indicating no data
        fprintf (xml_out,
                 "<message>\n"
                 "  <folder>IO</folder>\n"
                 "  <heading>(No I/O statistics in mpiP file)</heading>\n"
                 "</message>\n"
                 "\n");

	// Return now
	return;
    }


    // Create pointer array for sorting callsites 
    CallSite **sortedSites = new CallSite *[num_callsites];

    // Initialize sortedSites with pointers to the unsorted array
    for (int i = 0; i < num_callsites; ++i)
    {
	sortedSites[i] = &sites[i];
    }

    // Sort pointers to the callsites by percent MPI and to break ties,
    // callsite id
    qsort( (void *)sortedSites, num_callsites, sizeof( CallSite *),
	   compareCallSitePtrsByMpiPct );

    // Create an automatically resized message buffer that we can create 
    // message and traceback in without fear of overflowing the buffer
    MessageBuffer mbuf, tbuf;

    // Small buffer so can append callsite to call type (i.e., Bcast[12])
    char typebuf[200];

    // Figure out how many digits required to print num_tasks
    sprintf (typebuf, "%i", num_tasks);
    int taskDigits = strlen (typebuf);

    // Print out call I/O statistics in sorted order
    for (int i = 0; i < num_callsites; ++i)
    {
	// Skip messages that mpiP didn't report about
	if (sortedSites[i]->stats[num_tasks].sumIO < 0)
	    continue;
	
	// Create callsite type and siteId identifier
	sprintf (typebuf, "%s[%i]", sortedSites[i]->call, 
		 sortedSites[i]->siteID);
    
	// Create the one-line description line for this callsite

	// Determine how many tasks have data filled in (count != -1)
	int taskCount = 0;
	for (int rank=0; rank < num_tasks; ++rank)
	{
	    SiteStats *stats = &sortedSites[i]->stats[rank];
	    
	    // Count tasks that are filled in by mpiP (count != -1)
	    if (stats->count != -1)
		++taskCount;
	}

	// Create heading, the one-line description line for this callsite,
	// depends on callsite format (whether location info exists)
	if (sortedSites[i]->location[0].format == 1)
	{
	    mbuf.sprintf ( 
		"  <heading>%-18s %6.2f%% of MPI  %13.8g Total I/O  %13.8g Mean I/O  "
		"%*i/%i Tasks   %s:%i  (%s)</heading>\n",
		typebuf, 
		sortedSites[i]->stats[num_tasks].mpi_pct,
		sortedSites[i]->stats[num_tasks].sumIO,
		sortedSites[i]->stats[num_tasks].meanIO,
		taskDigits, taskCount, num_tasks,
		sortedSites[i]->location[0].function, 
		sortedSites[i]->location[0].line_number,
		sortedSites[i]->location[0].filename);
	}
	// Create heading where location info does not exist
	else if (sortedSites[i]->location[0].format == 2)
	{
	    mbuf.sprintf ( 
		"  <heading>%-18s %6.2f%% of MPI  %13.8g Total I/O  %13.8g Mean I/O   "
		"%*i/%i Tasks   [Addr: %s] (unknown location)</heading>\n",
		typebuf, 
		sortedSites[i]->stats[num_tasks].mpi_pct,
		sortedSites[i]->stats[num_tasks].sumIO,
		sortedSites[i]->stats[num_tasks].meanIO,
		taskDigits, taskCount, num_tasks,
		sortedSites[i]->location[0].filename);
	}
	// No other formats right now
	else
	{
	    TG_error ("Creating I/O Header: "
		      "Unexpected callsite format %i\n", 
		      sortedSites[i]->location[0].format);
	}

	// Append header for stats (first line of message body)
	mbuf.appendSprintf ("  <body>   Task      Count     Max(bytes)    Mean(bytes)     Min(bytes)     Sum(bytes)\n");

	// The stats at 'num_tasks' are the aggregate stats
	SiteStats *astats = &sortedSites[i]->stats[num_tasks];

	// Append out aggregate stats after header
	mbuf.appendSprintf ("   ALL: %10i  %13.8g  %13.8g  %13.8g  %13.8g",
			    astats->count, astats->maxIO, astats->meanIO, 
			    astats->minIO, astats->sumIO);

	// Append to the message a stats line for each task
	for (int rank=0; rank < num_tasks; ++rank)
	{
	    SiteStats *stats = &sortedSites[i]->stats[rank];

	    // Only print stats for tasks that reached callsite (count != -1)
	    if (stats->count != -1)
	    {
		mbuf.appendSprintf (
		    "\n%6i: %10i  %13.8g  %13.8g  %13.8g  %13.8g",
		    rank, stats->count, 
		    stats->maxIO, stats->meanIO, 
		    stats->minIO, stats->sumIO);
	    }
	}

	// Don't end message with '\n' unless you want a blank line
	// after the message

	// Put end body tag after message
        mbuf.appendSprintf("</body>\n");

	// Start traceback with callsite info
	tbuf.sprintf ("%s", sortedSites[i]->traceback);

	// If have ioLineNo info (not -1), append traceback to
	// the MpiP output that generated this message
	if ((sortedSites[i]->ioLineNo > -1) && (outputMpiPRefs))
	{
	    tbuf.appendSprintf ("  <annot>\n"
				"    <title>Raw MpiP Data</title>\n"
				"    <site>\n"
				"      <file>%s</file>\n"
				"      <line>%i</line>\n"
				"    </site>\n"
				"  </annot>\n", 
				mpip_file_name, sortedSites[i]->ioLineNo);
	}

	// Write out stats message with callsite traceback
	fprintf (xml_out,
                 "<message>\n"
                 "  <folder>IO</folder>\n"
                 "%s"
                 "%s"
                 "</message>\n"
                 "\n",
                 mbuf.contents(),
                 tbuf.contents());
    }


    // Delete the sorted callsite info pointers (only)
    delete [] sortedSites;
}




// Create and write in XML the callsite location messages in the order
// of the callsites.  Mainly to allow user to see each callsite location.
void MpipInfo::writeCallsiteLocationMessages(FILE *xml_out)
{
    // Create callsite message forlder in XML format
    fprintf (xml_out,
             "<message_folder>\n"
             "  <tag>sites</tag>\n"
             "  <title>MpiP Call Sites</title>\n"
             "</message_folder>\n"
             "\n");
    
    // Create an automatically resized message buffer that we can create 
    // message in without fear of overflowing the buffer
    MessageBuffer mbuf;

    // Print out each call site location, in siteID order
    for (int i = 0; i < num_callsites; ++i)
    {
	// As heading, print out first level trackback info for callsite
	// depends on callsite format (whether location info exists)
	if (sites[i].location[0].format == 1)
	{
	    mbuf.sprintf ("  <heading>%4.0i %s:%i (%s)  %s</heading>\n", 
			  sites[i].siteID, sites[i].location[0].function, 
			  sites[i].location[0].line_number, 
			  sites[i].location[0].filename, sites[i].call);
	}
	// Create heading where location info does not exist
	else if (sites[i].location[0].format == 2)
	{
	    mbuf.sprintf ("  <heading>%4.0i [Addr: %s] (unknown location)  %s</heading>\n", 
			  sites[i].siteID,
			  sites[i].location[0].filename, sites[i].call);
	}
	// No other formats right now
	else
	{
	    TG_error ("Creating Callsite Header: "
		      "Unexpected callsite format %i\n", 
		      sites[i].location[0].format);
	}

	// If have body (more than one level traceback), start body
	if ((trace_depth > 1) && (sites[i].location[1].function != NULL))
	    mbuf.appendSprintf ("  <body>");

	// Print out any remaining levels that are actually specified
	// (if hit main, doesn't go lower, so may have fewer levels)
	for (int level = 1; level < trace_depth; ++level)
	{
	    // Stop if info not specified for this level
	    if (sites[i].location[level].function == NULL)
		break;

	    // If printing second or later line, append newline 
	    if (level > 1)
		mbuf.appendSprintf("\n");

	    // Append this level's info onto end of message
	    // Handle case where location is known
	    if (sites[i].location[level].format == 1)
	    {
		mbuf.appendSprintf ("   %s:%i (%s)", 
				    sites[i].location[level].function, 
				    sites[i].location[level].line_number, 
				    sites[i].location[level].filename);
	    }
	    // Handle case where location not known, only address is known
	    else if (sites[i].location[level].format == 2)
	    {
		mbuf.appendSprintf ("   [Addr: %s] (unknown location)", 
				    sites[i].location[level].filename);
	    }

	    // No other formats yet
	    else
	    {
		TG_error ("Callsite Body: Unexpected level %i callsite "
			  "format %i\n", 
			  level, sites[i].location[level].format);
	    }
	}

	// If have body (more than one level traceback), end body
	if ((trace_depth > 1) && (sites[i].location[1].function != NULL))
	    mbuf.appendSprintf ("</body>\n");

        // Write out call sites in siteID order to call site message folder
        fprintf (xml_out,
                 "<message>\n"
                 "  <folder>sites</folder>\n"
                 "%s"
                 "%s"
                 "</message>\n"
                 "\n",
                 mbuf.contents(),
                 sites[i].traceback);
    }
}

// Write out messages about the mpiP file itself to indicate
// section headings
void MpipInfo::writeMpiPFileMessages(FILE *xml_out)
{
    // Create mpiP message folder
    fprintf (xml_out,
             "<message_folder>\n"
             "  <tag>mpiP</tag>\n"
             "  <title>Indexed MpiP Output Text</title>\n"
             "</message_folder>\n"
             "\n");

    // Create indexes for various interesting points in the mpiP file
    writeMpiPFileIndex (xml_out, commandLineNo, "Invocation Command");
    writeMpiPFileIndex (xml_out, versionLineNo, "mpiP Version");
    writeMpiPFileIndex (xml_out, envLineNo, 
			"MPIP Environment Variable Setting");
    writeMpiPFileIndex (xml_out, nodesLineNo, "MPI Task Assignment");
    writeMpiPFileIndex (xml_out, mpiTimeLineNo, 
			"MPI Time Breakdown By MPI Task");
    writeMpiPFileIndex (xml_out, sitesLineNo, "Callsites Measured By mpiP");
    writeMpiPFileIndex (xml_out, topTimeLineNo, 
		       "Aggregate Time Statistics for Top Twenty Callsites");
    writeMpiPFileIndex (xml_out, topSentLineNo, 
       "Aggregate Bytes Sent Statistics for Top Twenty Callsites");
    writeMpiPFileIndex (xml_out, topIOLineNo, 
       "Aggregate I/O Size Statistics for Top Twenty Callsites");
    writeMpiPFileIndex (xml_out, allTimeLineNo, 
		       "Detailed Time Statistics for All Callsites");
    writeMpiPFileIndex (xml_out, allSentLineNo, 
		       "Detailed Bytes Sent Statistics for All Callsites");
    writeMpiPFileIndex (xml_out, allIOLineNo, 
		       "Detailed I/O Statistics for All Callsites");
}

// Helper routine that creates and writes index message for the mpip file
// Automatically prepends line number into message
// Automatically skips message if lineNo is -1
void MpipInfo::writeMpiPFileIndex (FILE *xml_out, int lineNo, 
				  const char *fmt, ...)
{
    va_list args;

    // If didn't find a section, don't put out message
    if (lineNo == -1)
	return;

    // Create automatically resized message and traceback buffers to 
    // prevent overflow
    MessageBuffer mbuf, tbuf;

    // Create traceback to lineNo in mpiP file
    tbuf.sprintf ("  <annot>\n"
		  "    <site>\n"
		  "      <file>%s</file>\n"
		  "      <line>%i</line>\n"
		  "    </site>\n"
		  "  </annot>\n", 				
		  mpip_file_name, lineNo);

    // Prepend message with lineNo
    mbuf.sprintf ("%5i: ", lineNo);

    // Append message passed in with fmt
    va_start (args, fmt);
    mbuf.vappendSprintf (fmt, args);
    va_end(args);

    // write message to "mpiP" message section
    fprintf (xml_out,
	     "<message>\n"
	     "  <folder>mpiP</folder>\n"
                 "  <heading>%s</heading>\n"
                 "%s"
                 "</message>\n"
                 "\n",
                 mbuf.contents(),
                 tbuf.contents());
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

