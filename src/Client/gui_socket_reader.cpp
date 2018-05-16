// gui_socket_reader.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 26 October 2000
// Check socket and handle what arrives on it.

#include <stdlib.h>

#include <qapplication.h>	// for qWarning
#include <qdatetime.h>
#include <qmessagebox.h>

#include "tg_socket.h"
#include "tg_pack.h"
#include "tg_program_state.h"
#include "gui_socket_reader.h"
#include "uimanager.h"
#include "command_tags.h"
#include "tg_error.h"
#include "tg_time.h"
#include "tg_typetags.h"
#include "md.h"
#include "search_path_dialog.h"

// DEBUG ,for sleep call
#include <unistd.h>

/* MS/START - ADDED SUPPORT FOR PMAPI MODULE */
#include "gui_query_modules.h"
#define MAXNUMCOUNTERS 8
/* MS/END - ADDED SUPPORT FOR PMAPI MODULE */


#define MIN_BUF_LEN 8192

GUISocketReader:: GUISocketReader( int sock_in, UIManager *m,
		TGProgramState * ps )
	: programState(ps), socket_in(sock_in), auto_reading(FALSE), um(m)
{
    // Set object name to aid in debugging connection issues
    setName ("GUISocketReader");
}

void GUISocketReader:: enable_auto_read( bool set_enable )
{
	// Set a timer to read the input queue periodically.
	// The interval must be chosen as a compromise between
	// keeping up with incoming data and avoiding excessive
	// spinning (and therefore CPU usage) when the input
	// is idle.
	if( set_enable && !auto_reading) {
		timer_id = startTimer(10);
		auto_reading = TRUE;
	} else if ( !set_enable && auto_reading ) {
		killTimer( timer_id );
		timer_id = 0;
		auto_reading = FALSE;
	}
}

void GUISocketReader:: timerEvent( QTimerEvent * )
{
	get_pending_data();
}
	
// Read data for no more than approximately 100 ms (up to 200ms delay, which is
// fine), but only check the time after every 50 reads or 75,000 bytes of 
// data read to minimize the time checking overhead.  
// We want to allow GUI to respond even when being swamped with a lot 
// of messages or huge messages.
//
// These numbers calibrated on 1.4Ghz Power4 processing TGui mpip_example.tgui
// and a 145MB BG/L mpiP file (which caused byte counts to be added). 
// -JCG 3/13/06
#define GSR_TIME_CHECK_COUNT 50
#define GSR_TIME_CHECK_BYTES 75000
#define GSR_READ_TIME 100 

void GUISocketReader:: get_pending_data()
{
    // Since we poll relatively infrequently, get a bunch
    // of data each time we check, but only read for a
    // limited amount of time.  This is a balance between
    // keeping up with incoming data and making the GUI responsive.
    
    // Before we spend time starting the timer,
    // see if there's something to read.
    if( TG_read_data_queued( socket_in ) == 0 ) return;
    
    int retval;
    
    // Check elapsed time after either GSR_TIME_CHECK_COUNT messages
    // or after GSR_TIME_CHECK_BYTES byts of data read to minimize
    // time checking overhead (which can be significant). 
    int next_time_check_count = GSR_TIME_CHECK_COUNT;
    int next_time_check_bytes = GSR_TIME_CHECK_BYTES; 
    int check_count = 0;
    int bytes_count = 0;
    QTime t;
    t.start();
    
    while(( retval = TG_read_data_queued( socket_in ) )> 0)
    {
	int sizeRead = 0;
	
	check_socket(&sizeRead);
	
	// Count number of checks
	check_count++;
	bytes_count+= sizeRead;
	
	// Occationally check clock to see if too much time has gone by
	next_time_check_count--;
	next_time_check_bytes -= sizeRead;
	if ((next_time_check_count < 0) || 
	    (next_time_check_bytes < 0))
	{
	    int elapsed;
	    // It too much time has gone by, stop checking sockets
	    // now so GUI can respond to events
	    if ((elapsed = t.elapsed()) > GSR_READ_TIME)
	    {
#if 0
		// DEBUG
		TG_timestamp ("Socket check exited early after %i "
			      "checks, %i bytes and %i ms\n", 
			      check_count, bytes_count, elapsed);
#endif
		
		break;
	    }
	    
	    // Don't check time for many more interations
	    next_time_check_count = GSR_TIME_CHECK_COUNT;
	    next_time_check_bytes = GSR_TIME_CHECK_BYTES;
	}
    }
    
    // retval < 0 indicates the socket closed unexpectedly
    if( retval < 0 ) 
	emit readerSocketClosed();
}

GUISocketReader:: ~GUISocketReader()
{
	if( auto_reading )
		killTimer( timer_id );
}

int GUISocketReader:: check_socket(int *sizeRead)
{
	int tag, id, size;
	char * buf;

	int retval = CONTINUE_THREAD;

	// Sanity check, set size read to 0 initially
	size = 0;
	if (sizeRead != NULL)
	    *sizeRead = 0;

	if( TG_recv( socket_in, &tag, &id, &size, (void **)&buf ) < 0 ) {
		emit readerSocketClosed();
		return DPCL_SAYS_QUIT;
	}

	// If sizeRead pointer is not NULL, set to size read in
	if (sizeRead != NULL)
	    *sizeRead = size;

	switch( tag ) {
		case DB_INSERT_ENTRY:
			unpack_and_insert_entry( buf );
			break;
		case DB_INSERT_PLAIN_ENTRY:
			unpack_and_insert_plain_entry( buf );
			break;
		case DB_INSERT_FUNCTION:
			unpack_and_insert_function( buf );
			break;
		case DB_INSERT_MODULE:
			unpack_and_insert_module( buf );
			break;
		case DB_DECLARE_ACTION_ATTR:
			unpack_and_declare_action_attr( buf );
			break;
		case DB_ENABLE_ACTION:
			unpack_and_enable_action( buf );
			break;
		case DB_DECLARE_DATA_ATTR:
			unpack_and_declare_data_attr( buf );
			break;
		case DB_INSERT_PROCESS_THREAD:
			unpack_and_insert_process_thread( buf );
			break;
		case DB_INSERT_DOUBLE:
			unpack_and_insert_double( buf );
			break;
		case DB_INSERT_INT:
			unpack_and_insert_int( buf );
			break;
		case DB_ADD_DOUBLE:
			unpack_and_add_double( buf );
			break;
		case DB_ADD_INT:
			unpack_and_add_int( buf );
			break;
		case GUI_SET_TARGET_INFO:
			unpack_and_set_target_info( buf );
			break;
  	        case DB_FILE_PARSE_COMPLETE:
		        unpack_and_set_file_parse_complete (buf);
			break;
  	        case DB_FUNCTION_PARSE_COMPLETE:
		        unpack_and_set_function_parse_complete (buf);
			break;
		case DPCL_PARSE_COMPLETE:
			retval = tag;
			break;
		case DB_FILE_FULL_PATH:
			um->setFullPath( buf, id );
			break;
		case DB_FILE_READ_COMPLETE:
		  um->insertSourceFile( buf, id, size );
			break;
		case DB_STATIC_DATA_COMPLETE:
			emit setProgramMessage( "Data read complete" );
			break;
 	        case DB_DECLARE_MESSAGE_FOLDER:
		        unpack_and_declare_message_folder (buf);
		        break;
	        case DB_ADD_MESSAGE:
		        unpack_and_add_message (buf);
			break;
	        case DB_PROCESS_XML_SNIPPET:
		        unpack_and_process_xml_snippet (buf);
			break;
		case DPCL_CHANGE_DIR_RESULT:
			retval = unpack_cd_result( buf );
			break;
		case DPCL_INITIALIZE_APP_RESULT:
			retval = unpack_init_app_result( buf );
			break;
	        case DPCL_TARGET_HALTED:
			unpack_target_halted( buf );
			break;
		case GUI_CREATE_VIEWER:
			retval = id;
			break;
//		case GUI_CREATE_STATIC_VIEWER:
//			retval = GUI_CREATE_STATIC_VIEWER;
//			break;
		case GUI_ADD_SUBDIRS:
			unpack_subdir_list( buf );
			break;
		case GUI_SEARCH_PATH:
			unpack_search_path( buf );
			break;
		case DPCL_TOOL_INFO:
			unpack_about_text( buf );
			break;
		case DPCL_TARGET_TERMINATED:
			unpack_target_terminated( buf );
			break;
		case DPCL_SAYS_QUIT:
			retval = tag;
			break;
		/* Allow collector to print out error message at client*/
        	case COLLECTOR_ERROR_MESSAGE:
		        char *error_message;
			TG_unpack( buf, "S", &error_message );
			fprintf (stderr, "\n%s\n",error_message);
		        break;
		/* MS/START - dynamic module loading */
   	        case DYNCOLLECT_NUMMODS:
	                unpack_and_countDynamicModules(buf);
	                break;
	        case DYNCOLLECT_NAME:
		        unpack_and_recordDynamicModules(buf);
	                break;
	        case DYNCOLLECT_ASK:
	                unpack_and_queryDynamicModules(buf,socket_in);
	                break;
		/* MS/END - dynamic module loading */
		default:
			fprintf( stderr, "Unknown tag from DPCL: %d\n", tag );
			if ((tag >= 0) && (tag < LAST_COMMAND_TAG))
			{
			    fprintf (stderr, "Unknown tag %d maps to '%s'\n",
				     tag, command_strings[tag]);
			}
			fprintf (stderr, "FYI: GUI_CREATE_VIEWER really is %d\n",
				 GUI_CREATE_VIEWER);
			break;
	}

	free( buf );

	return retval;
}

void GUISocketReader:: unpack_and_insert_entry( char * buf )
{
	char * func_name;
	char * ip_tag;
	int line;
	TG_InstPtType type;
	TG_InstPtLocation location;
	char * funcCalled;
	int callIndex;
	char * description;
	    

	TG_unpack( buf, "SSIIISIS", &func_name, &ip_tag, &line, &type,
		   &location, &funcCalled, &callIndex, &description );
#if 0
	TG_timestamp("inserting entry %s for %s\n", ip_tag, func_name);
	printf("line %d type %d location %d func %s index %d\n",
			line, type, location, funcCalled, callIndex );
#endif
	um->insertEntry( func_name, ip_tag, line, type, location,
			 funcCalled, callIndex, description );
}

void GUISocketReader:: unpack_and_insert_plain_entry( char * buf )
{
	char * func_name;
	char * ip_tag;
	int line;
	char * description;
	    

	TG_unpack( buf, "SSIS", &func_name, &ip_tag, &line, &description );
#if 0
	TG_timestamp("inserting entry %s for %s\n", ip_tag, func_name);
#endif
	um->insertEntry( func_name, ip_tag, line, description );
}

// Get function info from message and store in the database
void GUISocketReader:: unpack_and_insert_function( char * buf )
{
	char * func_name;
	char * path;
	int start_line, stop_line;

	TG_unpack( buf, "SSII", &func_name, &path, &start_line, &stop_line );
#if 0
	TG_timestamp("inserting function %s\n", func_name);
#endif
	um->insertFunction( func_name, path, start_line, stop_line );
}

// Get module info from message and -->>>FOR NOW<<<--, just ask
// the DPCL side for all the function information on the module.
// LATER we'll store the module info in the DB and only ask for
// function info when the user requests it.
void GUISocketReader:: unpack_and_insert_module( char * buf )
{
	char * module_name;

	// Remember, module_name is just set to point into buf,
	// so it will disappear sometime after we return.
	TG_unpack( buf, "S", &module_name);
#if 0
	TG_timestamp("inserting module %s\n", module_name);
#endif

	// Insert module (file) in DB
	um->insertFile ( module_name);
	emit newModule( module_name);
}

// Declare the pixmaps (in xpm form)
// See http://koala.ilog.fr/lehors/xpm.html for the format details,
// tools for converting from earlier versions, etc.
static const char * startCounters_xpm[] = {
"15 15 6 1",
"       c None",
".      c #000000000000",
"G      c #0000FFFF0000",
"R      c #FFFF00000000",
":      c #E500E500E500",
"W      c #FFFFFFFFFFFF",
"WWWWWWWWWWWWWWW",
"W:::GGGGGGG:::.",
"W::GG.G.G.GG::.",
"W:GGG.G.G.GGG:.",
"WGGG.......GGG.",
"WG...........G.",
"WGGG.......GGG.",
"WG...........G.",
"WGGG.......GGG.",
"WG...........G.",
"WGGG.......GGG.",
"W:GGG.G.G.GGG:.",
"W::GG.G.G.GG::.",
"W:::GGGGGGG:::.",
"W.............."};

static const char * stopCounters_xpm[] = {
"15 15 6 1",
"       c None",
".      c #000000000000",
"G      c #0000FFFF0000",
"R      c #FFFF00000000",
":      c #E500E500E500",
"W      c #FFFFFFFFFFFF",
"WWWWWWWWWWWWWWW",
"W:::RRRRRRR:::.",
"W::RR.R.R.RR::.",
"W:RRR.R.R.RRR:.",
"WRRR.......RRR.",
"WR...........R.",
"WRRR.......RRR.",
"WR...........R.",
"WRRR.......RRR.",
"WR...........R.",
"WRRR.......RRR.",
"W:RRR.R.R.RRR:.",
"W::RR.R.R.RR::.",
"W:::RRRRRRR:::.",
"W.............."};


static const char * haltTarget_xpm[] = {
"15 15 6 1",
"       c None",
".      c #000000000000",
"G      c #0000FFFF0000",
"R      c #FFFF00000000",
":      c #E500E500E500",
"W      c #FFFFFFFFFFFF",
"WWWWWWWWWWWWWWW",
"W:::::::::::::.",
"W:::::::::::::.",
"W::...:::...::.",
"W::...:::...::.",
"W::...:::...::.",
"W::...:::...::.",
"W::...:::...::.",
"W::...:::...::.",
"W::...:::...::.",
"W::...:::...::.",
"W::...:::...::.",
"W:::::::::::::.",
"W:::::::::::::.",
"W.............."};

static const char * startTimer_xpm[] = {
"15 15 6 1",
"       c None",
".      c #000000000000",
"G      c #0000FFFF0000",
"R      c #FFFF00000000",
":      c #E500E500E500",
"W      c #FFFFFFFFFFFF",
"WWWWWWWWWWWWWWW",
"W:::::...:::::.",
"W:::.GG.GG.:::.",
"W::.GGG.GGG.::.",
"W:.GGGG.GGGG.:.",
"W:.GGGG.GGGG.:.",
"W.GGGGG.GGGGG..",
"W.GGGGG........",
"W.GGGGGGGGGGG..",
"W:.GGGGGGGGG.:.",
"W:.GGGGGGGGG.:.",
"W::.GGGGGGG.::.",
"W:::.GGGGG.:::.",
"W:::::...:::::.",
"W.............."};

static const char * stopTimer_xpm[] = {
"15 15 6 1",
"       c None",
".      c #000000000000",
"G      c #0000FFFF0000",
"R      c #FFFF00000000",
":      c #E500E500E500",
"W      c #FFFFFFFFFFFF",
"WWWWWWWWWWWWWWW",
"W:::::...:::::.",
"W:::.RR.RR.:::.",
"W::.RRR.RRR.::.",
"W:.RRRR.RRRR.:.",
"W:.RRRR.RRRR.:.",
"W.RRRRR.RRRRR..",
"W.RRRRR........",
"W.RRRRRRRRRRR..",
"W:.RRRRRRRRR.:.",
"W:.RRRRRRRRR.:.",
"W::.RRRRRRR.::.",
"W:::.RRRRR.:::.",
"W:::::...:::::.",
"W.............."};

static const char * checkMemory_xpm[] = {
"15 15 5 1",
"       c None",
".      c #000000000000",
"B      c #00000000FFFF",
":      c #E500E500E500",
"W      c #FFFFFFFFFFFF",
"WWWWWWWWWWWWWWW",
"W:::::::::::::.",
"W::.:.....:.::.",
"W::..:::::..::.",
"W::.:.....:.::.",
"W::.:::::::.::.",
"W::.B:::::B.::.",
"W::.BBBBBBB.::.",
"W::.BBBBBBB.::.",
"W::.BBBBBBB.::.",
"W::.BBBBBBB.::.",
"W::..BBBBB..::.",
"W::::.....::::.",
"W:::::::::::::.",
"W.............."};

/* MS/START - ADDED SUPPORT FOR PMAPI AND COUNTER MODULE */

static const char * showCount_xpm[] = {
"15 15 7 1",
"       c None",
".      c #000000000000",
"B      c #00000000FFFF",
"R      c #FFFF00000000",
":      c #E500E500E500",
"W      c #FFFFFFFFFFFF",
"w      c #FF00FF00FF00",
"WWWWWWWWWWWWWWW",
"W:::BBBBBBB:::.",
"W::BBwBwBwBB::.",
"W:BBBwBwBwBBB:.",
"WBBBwwwwwwwBBB.",
"WBwwwwwwwwwwwB.",
"WBBBwwwwwwwBBB.",
"WBwwwwwwwwwwwB.",
"WBBBwwwwwwwBBB.",
"WBwwwwwwwwwwwB.",
"WBBBwwwwwwwBBB.",
"W:BBBwBwBwBBB:.",
"W::BBwBwBwBB::.",
"W:::BBBBBBB:::.",
"W.............."};

static const char * stopNumCounterOn_xpm[][22] = 
{
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::RRRRRRR:::B",
    "W::RR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR:::BBB:::RRB",
    "WR:::BBBB::::RB",
    "WR::BB:BB::::RB",
    "WR:::::BB::::RB",
    "WR:::::BB::::RB",
    "WR::::BBBB:::RB",
    "WRR:::::::::RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RR::B",
    "W:::RRRRRRR:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::RRRRRRR:::B",
    "W::RR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR::BBBB:::RRB",
    "WR::BBBBBB:::RB",
    "WR:BB::::BB::RB",
    "WR::::::BBB::RB",
    "WR::::BBB::::RB",
    "WR:::BB::::::RB",
    "WRR::BBBBB::RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RR::B",
    "W:::RRRRRRR:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::RRRRRRR:::B",
    "W::RR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR::BBBB:::RRB",
    "WR::BBBBBB:::RB",
    "WR:::::::BB::RB",
    "WR:::::BBB:::RB",
    "WR:::::::BB::RB",
    "WR::BBBBBB:::RB",
    "WRR::BBBB:::RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RR::B",
    "W:::RRRRRRR:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::RRRRRRR:::B",
    "W::RR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR::BB:::::RRB",
    "WR:::BB::::::RB",
    "WR:::BB:BB:::RB",
    "WR:::BBBBBB::RB",
    "WR::::::BB:::RB",
    "WR::::::BB:::RB",
    "WRR:::::::::RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RR::B",
    "W:::RRRRRRR:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::RRRRRRR:::B",
    "W::RR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR::BBBBB::RRB",
    "WR:::BBBBB:::RB",
    "WR:::BB::::::RB",
    "WR:::BBBB::::RB",
    "WR::::::BB:::RB",
    "WR::BB::BB:::RB",
    "WRR::BBBB:::RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RR::B",
    "W:::RRRRRRR:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::RRRRRRR:::B",
    "W::RR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR::BBBBB::RRB",
    "WR::BB:::BB::RB",
    "WR::BB:::::::RB",
    "WR::BBBBBB:::RB",
    "WR::BB:::BB::RB",
    "WR::BB:::BB::RB",
    "WRR::BBBBB::RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RR::B",
    "W:::RRRRRRR:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::RRRRRRR:::B",
    "W::RR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR::BBBBB::RRB",
    "WR:::BBBBB:::RB",
    "WR::::::BB:::RB",
    "WR:::::BB::::RB",
    "WR::::BB:::::RB",
    "WR:::BB::::::RB",
    "WRR::BB:::::RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RR::B",
    "W:::RRRRRRR:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::RRRRRRR:::B",
    "W::RR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR::BBBBB::RRB",
    "WR::BB:::BB::RB",
    "WR::BB:::BB::RB",
    "WR:::BBBBB:::RB",
    "WR::BB:::BB::RB",
    "WR::BB:::BB::RB",
    "WRR::BBBBB::RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RR::B",
    "W:::RRRRRRR:::B",
    "WBBBBBBBBBBBBBB"
  }
};

static const char * startNumCounterOn_xpm[][22] = 
{
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::GGGGGGG:::B",
    "W::GG:::::GG::B",
    "W:GG:::::::GG:B",
    "WGG:::BBB:::GGB",
    "WG:::BBBB::::GB",
    "WG::BB:BB::::GB",
    "WG:::::BB::::GB",
    "WG:::::BB::::GB",
    "WG::::BBBB:::GB",
    "WGG:::::::::GGB",
    "W:GG:::::::GG:B",
    "W::GG:::::GG::B",
    "W:::GGGGGGG:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::GGGGGGG:::B",
    "W::GG:::::GG::B",
    "W:GG:::::::GG:B",
    "WGG::BBBB:::GGB",
    "WG::BBBBBB:::GB",
    "WG:BB::::BB::GB",
    "WG::::::BBB::GB",
    "WG::::BBB::::GB",
    "WG:::BB::::::GB",
    "WGG::BBBBB::GGB",
    "W:GG:::::::GG:B",
    "W::GG:::::GG::B",
    "W:::GGGGGGG:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::GGGGGGG:::B",
    "W::GG:::::GG::B",
    "W:GG:::::::GG:B",
    "WGG::BBBB:::GGB",
    "WG::BBBBBB:::GB",
    "WG:::::::BB::GB",
    "WG:::::BBB:::GB",
    "WG:::::::BB::GB",
    "WG::BBBBBB:::GB",
    "WGG::BBBB:::GGB",
    "W:GG:::::::GG:B",
    "W::GG:::::GG::B",
    "W:::GGGGGGG:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::GGGGGGG:::B",
    "W::GG:::::GG::B",
    "W:GG:::::::GG:B",
    "WGG::BB:::::GGB",
    "WG:::BB::::::GB",
    "WG:::BB:BB:::GB",
    "WG:::BBBBBB::GB",
    "WG::::::BB:::GB",
    "WG::::::BB:::GB",
    "WGG:::::::::GGB",
    "W:GG:::::::GG:B",
    "W::GG:::::GG::B",
    "W:::GGGGGGG:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::GGGGGGG:::B",
    "W::GG:::::GG::B",
    "W:GG:::::::GG:B",
    "WGG::BBBBB::GGB",
    "WG:::BBBBB:::GB",
    "WG:::BB::::::GB",
    "WG:::BBBB::::GB",
    "WG::::::BB:::GB",
    "WG::BB::BB:::GB",
    "WGG::BBBB:::GGB",
    "W:GG:::::::GG:B",
    "W::GG:::::GG::B",
    "W:::GGGGGGG:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::GGGGGGG:::B",
    "W::GG:::::GG::B",
    "W:GG:::::::GG:B",
    "WGG::BBBBB::GGB",
    "WG::BB:::BB::GB",
    "WG::BB:::::::GB",
    "WG::BBBBBB:::GB",
    "WG::BB:::BB::GB",
    "WG::BB:::BB::GB",
    "WGG::BBBBB::GGB",
    "W:GG:::::::GG:B",
    "W::GG:::::GG::B",
    "W:::GGGGGGG:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::GGGGGGG:::B",
    "W::GG:::::GG::B",
    "W:GG:::::::GG:B",
    "WGG::BBBBB::GGB",
    "WG:::BBBBB:::GB",
    "WG::::::BB:::GB",
    "WG:::::BB::::GB",
    "WG::::BB:::::GB",
    "WG:::BB::::::GB",
    "WGG::BB:::::GGB",
    "W:GG:::::::GG:B",
    "W::GG:::::GG::B",
    "W:::GGGGGGG:::B",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "W:::GGGGGGG:::B",
    "W::GG:::::GG::B",
    "W:GG:::::::GG:B",
    "WGG::BBBBB::GGB",
    "WG::BB:::BB::GB",
    "WG::BB:::BB::GB",
    "WG:::BBBBB:::GB",
    "WG::BB:::BB::GB",
    "WG::BB:::BB::GB",
    "WGG::BBBBB::GGB",
    "W:GG:::::::GG:B",
    "W::GG:::::GG::B",
    "W:::GGGGGGG:::B",
    "WBBBBBBBBBBBBBB"
  }
};

static const char * stopNumCounterOff_xpm[][22] = 
{
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::RRRRRRR:::B",
    "W:RRR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR:R:BBB:::RRB",
    "WR:::RBBB::::RB",
    "WR::BBRBB::::RB",
    "WR:::::RB::::RB",
    "WR:::::BR::::RB",
    "WR::::BBBR:::RB",
    "WRR:::::::R:RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RRR:B",
    "W:::RRRRRRR::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::RRRRRRR:::B",
    "W:RRR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR:RBBBB:::RRB",
    "WR::BRBBBB:::RB",
    "WR:BB:R::BB::RB",
    "WR:::::RBBB::RB",
    "WR::::BBR::::RB",
    "WR:::BB::R:::RB",
    "WRR::BBBBBR:RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RRR:B",
    "W:::RRRRRRR::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::RRRRRRR:::B",
    "W:RRR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR:RBBBB:::RRB",
    "WR::BRBBBB:::RB",
    "WR::::R::BB::RB",
    "WR:::::RBB:::RB",
    "WR::::::RBB::RB",
    "WR::BBBBBR:::RB",
    "WRR::BBBB:R:RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RRR:B",
    "W:::RRRRRRR::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::RRRRRRR:::B",
    "W:RRR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR:RBB:::::RRB",
    "WR:::RB::::::RB",
    "WR:::BR:BB:::RB",
    "WR:::BBRBBB::RB",
    "WR::::::RB:::RB",
    "WR::::::BR:::RB",
    "WRR:::::::R:RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RRR:B",
    "W:::RRRRRRR::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::RRRRRRR:::B",
    "W:RRR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR:RBBBBB::RRB",
    "WR:::RBBBB:::RB",
    "WR:::BR::::::RB",
    "WR:::BBRB::::RB",
    "WR::::::RB:::RB",
    "WR::BB::BR:::RB",
    "WRR::BBBB:R:RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RRR:B",
    "W:::RRRRRRR::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::RRRRRRR:::B",
    "W:RRR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR:RBBBBB::RRB",
    "WR::BR:::BB::RB",
    "WR::BBR::::::RB",
    "WR::BBBRBB:::RB",
    "WR::BB::RBB::RB",
    "WR::BB:::RB::RB",
    "WRR::BBBBBR:RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RRR:B",
    "W:::RRRRRRR::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::RRRRRRR:::B",
    "W:RRR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR:RBBBBB::RRB",
    "WR:::RBBBB:::RB",
    "WR::::R:BB:::RB",
    "WR:::::RB::::RB",
    "WR::::BBR::::RB",
    "WR:::BB::R:::RB",
    "WRR::BB:::R:RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RRR:B",
    "W:::RRRRRRR::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::RRRRRRR:::B",
    "W:RRR:::::RR::B",
    "W:RR:::::::RR:B",
    "WRR:RBBBBB::RRB",
    "WR::BR:::BB::RB",
    "WR::BBR::BB::RB",
    "WR:::BBRBB:::RB",
    "WR::BB::RBB::RB",
    "WR::BB:::RB::RB",
    "WRR::BBBBBR:RRB",
    "W:RR:::::::RR:B",
    "W::RR:::::RRR:B",
    "W:::RRRRRRR::RB",
    "WBBBBBBBBBBBBBB"
  }
};

static const char * startNumCounterOff_xpm[][22] = 
{
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::GGGGGGG:::B",
    "W:RGG:::::GG::B",
    "W:GR:::::::GG:B",
    "WGG:R:BBB:::GGB",
    "WG:::RBBB::::GB",
    "WG::BBRBB::::GB",
    "WG:::::RB::::GB",
    "WG:::::BR::::GB",
    "WG::::BBBR:::GB",
    "WGG:::::::R:GGB",
    "W:GG:::::::RG:B",
    "W::GG:::::GGR:B",
    "W:::GGGGGGG::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::GGGGGGG:::B",
    "W:RGG:::::GG::B",
    "W:GR:::::::GG:B",
    "WGG:RBBBB:::GGB",
    "WG::BRBBBB:::GB",
    "WG:BB:R::BB::GB",
    "WG:::::RBBB::GB",
    "WG::::BBR::::GB",
    "WG:::BB::R:::GB",
    "WGG::BBBBBR:GGB",
    "W:GG:::::::RG:B",
    "W::GG:::::GGR:B",
    "W:::GGGGGGG::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::GGGGGGG:::B",
    "W:RGG:::::GG::B",
    "W:GR:::::::GG:B",
    "WGG:RBBBB:::GGB",
    "WG::BRBBBB:::GB",
    "WG::::R::BB::GB",
    "WG:::::RBB:::GB",
    "WG::::::RBB::GB",
    "WG::BBBBBR:::GB",
    "WGG::BBBB:R:GGB",
    "W:GG:::::::RG:B",
    "W::GG:::::GGR:B",
    "W:::GGGGGGG::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::GGGGGGG:::B",
    "W:RGG:::::GG::B",
    "W:GR:::::::GG:B",
    "WGG:RBB:::::GGB",
    "WG:::RB::::::GB",
    "WG:::BR:BB:::GB",
    "WG:::BBRBBB::GB",
    "WG::::::RB:::GB",
    "WG::::::BR:::GB",
    "WGG:::::::R:GGB",
    "W:GG:::::::RG:B",
    "W::GG:::::GGR:B",
    "W:::GGGGGGG::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::GGGGGGG:::B",
    "W:RGG:::::GG::B",
    "W:GR:::::::GG:B",
    "WGG:RBBBBB::GGB",
    "WG:::RBBBB:::GB",
    "WG:::BR::::::GB",
    "WG:::BBRB::::GB",
    "WG::::::RB:::GB",
    "WG::BB::BR:::GB",
    "WGG::BBBB:R:GGB",
    "W:GG:::::::RG:B",
    "W::GG:::::GGR:B",
    "W:::GGGGGGG::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::GGGGGGG:::B",
    "W:RGG:::::GG::B",
    "W:GR:::::::GG:B",
    "WGG:RBBBBB::GGB",
    "WG::BR:::BB::GB",
    "WG::BBR::::::GB",
    "WG::BBBRBB:::GB",
    "WG::BB::RBB::GB",
    "WG::BB:::RB::GB",
    "WGG::BBBBBR:GGB",
    "W:GG:::::::RG:B",
    "W::GG:::::GGR:B",
    "W:::GGGGGGG::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::GGGGGGG:::B",
    "W:RGG:::::GG::B",
    "W:GR:::::::GG:B",
    "WGG:RBBBBB::GGB",
    "WG:::RBBBB:::GB",
    "WG::::R:BB:::GB",
    "WG:::::RB::::GB",
    "WG::::BBR::::GB",
    "WG:::BB::R:::GB",
    "WGG::BB:::R:GGB",
    "W:GG:::::::RG:B",
    "W::GG:::::GGR:B",
    "W:::GGGGGGG::RB",
    "WBBBBBBBBBBBBBB"
  },
  {
    "15 15 6 1",
    "       c None",
    "B      c #000000000000",
    "G      c #0000FFFF0000",
    "R      c #FFFF00000000",
    ":      c #E500E500E500",
    "W      c #FFFFFFFFFFFF",
    "WWWWWWWWWWWWWWW",
    "WR::GGGGGGG:::B",
    "W:RGG:::::GG::B",
    "W:GR:::::::GG:B",
    "WGG:RBBBBB::GGB",
    "WG::BR:::BB::GB",
    "WG::BBR::BB::GB",
    "WG:::BBRBB:::GB",
    "WG::BB::RBB::GB",
    "WG::BB:::RB::GB",
    "WGG::BBBBBR:GGB",
    "W:GG:::::::RG:B",
    "W::GG:::::GGR:B",
    "W:::GGGGGGG::RB",
    "WBBBBBBBBBBBBBB"
  }
};

/* MS/END - ADDED SUPPORT FOR PMAPI MODULE */

// Get action type info from message and store in the database
void GUISocketReader:: unpack_and_declare_action_attr( char * buf )
{
	char * actionName;
	char * label;
	char * description;

	TG_unpack( buf, "SSS", &actionName, &label, &description );

	// Declare action when declare intial state below.  These
	// arguments need to be updated since label and description
	// are no longer used.
	// COME BACK
//	um->declareActionAttr( actionName, label, description );

	// Set up GUI for MPX action 'startCounters'
	if (strcmp (actionName, "startCounters") == 0)
	{
	    // Declare start counter pixmap
	    um->declarePixmap ("startCountersPixmap", startCounters_xpm);
	    
	    // Declare action state startCounters and
	    // declare pixmaps, menu text, and tool tip for the 
	    // initial uninstrumented state 'Out'
	    um->declareAction ("startCounters", 
			       "Out",
			       // Pixmap for removing
			       "startCountersPixmap", 
			       "Remove Start Counters",
			       "Remove mpx probe to start cache and FLOP measurements from here",
			       "", // No pixmap if removed
			       "Uninstrumented potential start counters location",
			       TRUE);
	    
	    // Declare pixmaps, menu text, and tool tip for the 
	    // instrumented state 'In'
	    um->declareActionState ("startCounters", 
				    "In",
				    "startCountersPixmap", 
				    "Start Counters",
				    "Insert mpx probe to start cache and FLOP measurements here",
				    "startCountersPixmap",
				    "Starts cache and FLOP measurements here (starts new mpx region)",
				    TRUE);
	}
	// Set up GUI for MPX action 'stopCounters'
	else if (strcmp (actionName, "stopCounters") == 0)
	{
	    um->declarePixmap ("stopCountersPixmap", stopCounters_xpm);

	    // Declare action stopCounters and
	    // declare pixmaps, menu text, and tool tip for the 
	    // initial uninstrumented state 'Out'
	    um->declareAction ("stopCounters", 
			       "Out",
			       // Pixmap for removing
			       "stopCountersPixmap", 
			       "Remove Stop Counters",
			       "Remove mpx probe to stop cache and FLOP measurements from here",
			       "", // No pixmap if removed
			       "Uninstrumented potential stop counters location",
			       TRUE);
	    
	    // Declare pixmaps, menu text, and tool tip for the 
	    // instrumented state 'In'
	    um->declareActionState ("stopCounters", 
				    "In",
				    "stopCountersPixmap", 
				    "Stop Counters",
				    "Insert mpx probe to stop cache and FLOP measurements here",
				    "stopCountersPixmap",
				    "Stops cache and FLOP measurements here (stops new mpx region)",
				    TRUE);
	}
	// Set up GUI for MPX action 'haltTarget'
	else if (strcmp (actionName, "haltTarget") == 0)
	{
	    um->declarePixmap ("haltTargetPixmap", haltTarget_xpm);
	    
	    // Declare action haltTarget and 
	    // declare pixmaps, menu text, and tool tip for the 
	    // initial uninstrumented state 'Out'
	    um->declareAction ("haltTarget", 
			       "Out",
			       // Pixmap for removing
			       "haltTargetPixmap", 
			       "Remove Breakpoint",
			       "Remove Breakpoint from here",
			       "", // No pixmap if removed
			       "Uninstrumented potential Breakpoint location",
			       TRUE);
	    
	    // Declare pixmaps, menu text, and tool tip for the 
	    // instrumented state 'In'
	    um->declareActionState ("haltTarget", 
				    "In",
				    "haltTargetPixmap", 
				    "set Breakpoint",
				    "Insert breakpoint here",
				    "haltTargetPixmap",
				    "Application stops here and pops out 'run' button",
				    TRUE);
	}
	// Set up GUI for Timer action 'startTimer'
	else if (strcmp (actionName, "startTimer") == 0)
	{
	    // Declare start timer pixmap
	    um->declarePixmap ("startTimerPixmap", startTimer_xpm);
	    
	    // Declare action state startTimer and
	    // declare pixmaps, menu text, and tool tip for the 
	    // initial uninstrumented state 'Out'
	    um->declareAction ("startTimer", 
			       "Out",
			       // Pixmap for removing
			       "startTimerPixmap", 
			       "Remove Start Timer",
			       "Remove probe to start timer from here",
			       "", // No pixmap if removed
			       "Uninstrumented potential start timer location",
			       TRUE);
	    
	    // Declare pixmaps, menu text, and tool tip for the 
	    // instrumented state 'In'
	    um->declareActionState ("startTimer", 
				    "In",
				    "startTimerPixmap", 
				    "Start Timer",
				    "Insert probe to start timer here",
				    "startTimerPixmap",
				    "Starts timer here (starts new timer interval)",
				    TRUE);
	}
	// Set up GUI for timer action 'stopTimer'
	else if (strcmp (actionName, "stopTimer") == 0)
	{
	    um->declarePixmap ("stopTimerPixmap", stopTimer_xpm);

	    // Declare action stopTimer and
	    // declare pixmaps, menu text, and tool tip for the 
	    // initial uninstrumented state 'Out'
	    um->declareAction ("stopTimer", 
			       "Out",
			       // Pixmap for removing
			       "stopTimerPixmap", 
			       "Remove Stop Timer",
			       "Remove probe to stop timer from here",
			       "", // No pixmap if removed
			       "Uninstrumented potential stop timer location",
			       TRUE);
	    
	    // Declare pixmaps, menu text, and tool tip for the 
	    // instrumented state 'In'
	    um->declareActionState ("stopTimer", 
				    "In",
				    "stopTimerPixmap", 
				    "Stop Timer",
				    "Insert probe to stop timer here",
				    "stopTimerPixmap",
				    "Stops timer measurements here (stops timer interval)",
				    TRUE);
	}
	// Set up GUI for memory action 'checkMemory'
	else if (strcmp (actionName, "checkMemory") == 0)
	{
	    um->declarePixmap ("checkMemoryPixmap", checkMemory_xpm);

	    // Declare action checkMemory and
	    // declare pixmaps, menu text, and tool tip for the 
	    // initial uninstrumented state 'Out'
	    um->declareAction ("checkMemory", 
			       "Out",
			       // Pixmap for removing
			       "checkMemoryPixmap", 
			       "Remove Check Memory",
			       "Remove probe to check memoryfrom here",
			       "", // No pixmap if removed
			       "Uninstrumented potential check memory location",
			       TRUE);
	    
	    // Declare pixmaps, menu text, and tool tip for the 
	    // instrumented state 'In'
	    um->declareActionState ("checkMemory", 
				    "In",
				    "checkMemoryPixmap", 
				    "Check Memory",
				    "Insert probe to check memory here",
				    "checkMemoryPixmap",
				    "Checks memory allocation status here",
				    TRUE);
	}
	/* MS/START - ADDED SUPPORT FOR PMAPI AND COUNTER MODULE */

	else if (strcmp (actionName, "counter") == 0)
	{
	    // Declare counter pixmap
	    um->declarePixmap ("countPixmap", showCount_xpm);
	    
	    // Declare action state startCounters and
	    // declare pixmaps, menu text, and tool tip for the 
	    // initial uninstrumented state 'Out'
	    um->declareAction ("counter", 
			       "Out",
			       // Pixmap for removing
			       "countPixmap", 
			       "Show Counters",
			       "Show the current Counter",
			       "", // No pixmap if removed
			       "Uninstrumented potential counting location",
			       TRUE);
	    
	    // Declare pixmaps, menu text, and tool tip for the 
	    // instrumented state 'In'
	    um->declareActionState ("counter", 
				    "In",
				    "countPixmap", 
				    "Show Counters",
				    "Show the current Counter",
				    "countPixmap",
				    "Show the counter here",
				    TRUE);
	}
	else if (strstr(actionName,"NUMCOUNTER_"))
	{
	  // requesting an action with numbered counters
	  // as used in the PMAPI tool
	  // further decoding necessary

	  int   eventtype=-1, cntnum=-1;
	  char  name[40];

	  if (strstr(actionName,"START_")==&(actionName[11]))
	    {
	      eventtype=0;
	      cntnum=atoi(&(actionName[17]));
	    }
	  else if (strstr(actionName,"STOP_")==&(actionName[11]))
	    {
	      eventtype=1;
	      cntnum=atoi(&(actionName[16]));
	    }
	  else
	    {
	      eventtype=-1;
	    }

	  // check if we really have a valid event */

	  if ((eventtype<0) || (cntnum<0) || (cntnum>=MAXNUMCOUNTERS))
	    {
	      // Let tool writer know that this number is too high
	      printf ("%s line %i:\n"
		      "GUISocketReader:: unpack_and_declare_action_attr:\n"
		      "Client doesn't know how to display action '%s'!\n",
		      __FILE__, __LINE__, actionName);
	    }
	  else
	    {
	      // declare ON actions
	      
	      if (eventtype==0)
		{
		  // start event
		  
		  sprintf(name,"numcounter_s%02i_pxm",cntnum);
		  um->declarePixmap (name,startNumCounterOn_xpm[cntnum]);
		}
	      else
		{
		  // stop event
		  
		  sprintf(name,"numcounter_k%02i_pxm",cntnum);
		  um->declarePixmap (name,stopNumCounterOn_xpm[cntnum]);
		}
	      
	      um->declareAction(actionName, 
				"In",
				name, 
				label,
				label,
				name,
				"IN",
				TRUE);
	      
	      
	      // declare OFF actions
	      
	      if (eventtype==0)
		{
		  // start event
		  
		  sprintf(name,"numcounter_so%02i_pxm",cntnum);
		  um->declarePixmap (name,startNumCounterOff_xpm[cntnum]);
		}
	      else
		{
		  // stop event
		  
		  sprintf(name,"numcounter_ko%02i_pxm",cntnum);
		  um->declarePixmap (name,stopNumCounterOff_xpm[cntnum]);
		}

	      um->declareActionState(actionName,
				     "Out",
				     name, 
				     label,
				     label,
				     "", // No pixmap if removed
				     "OUT",
				     TRUE);
	    }
	}

	/* MS/END - ADDED SUPPORT FOR PMAPI MODULE */

	else
	{
	    // Let user know that this function needs updating
	    fprintf (stderr, "Error: "
		     "GUISocketReader:: unpack_and_declare_action_attr:\n"
		     "Client doesn't know how to display action '%s'!\n"
		     "Need to update gui_socket_reader.cpp\n",
		     actionName);
	    exit (1);
	}
}

// Get function and action point info from message and enable the action 
void GUISocketReader:: unpack_and_enable_action( char * buf )
{
	char * func_name;
	char * ip_tag;
	char * action;

	TG_unpack( buf, "SSS", &func_name, &ip_tag, &action );
	um->enableAction( func_name, ip_tag, action );
}

// Get dataAttr name and type and declare it in the database
void GUISocketReader:: unpack_and_declare_data_attr( char * buf )
{
	char * dataAttrTag;
	char * attrName;
	char * description;
	int dataType;
	int mdType;

	TG_unpack( buf, "SSSI", &dataAttrTag, &attrName, &description,
		&dataType );

	// Translate from the TG type to the MD type
	switch( dataType ) {
	case TG_INT:
		mdType = MD_INT;
		break;
	case TG_DOUBLE:
		mdType = MD_DOUBLE;
		break;
	default:
		// Unknown type -- complain and then guess double
		printf( "unknown TG datatype declaration (%d) for %s,"
		       "using double\n", dataType, dataAttrTag );
		mdType = MD_DOUBLE;
	}

	// Pick some default suggestions for AttrStat and EntryStat
	UIManager::AttrStat suggestedAttrStat = UIManager::AttrMean;
	UIManager::EntryStat suggestedEntryStat = UIManager::EntryMean;

	// Refine suggestions based on dataAttrTag
	// MPX: average % utilization across both tasks/threads and entries
	if (strcmp(dataAttrTag, "l1util")==0)
	{ 
	    suggestedAttrStat = UIManager::AttrMean;
	    suggestedEntryStat = UIManager::EntryMean;
	}

	// MPX: average flop data over tasks/threads and sum over entries
	else if ((strcmp(dataAttrTag, "flops")==0) ||
		 (strcmp(dataAttrTag, "fp_ins")==0) ||
		 (strcmp(dataAttrTag, "fma")==0) ||
		 (strcmp(dataAttrTag, "FLOP/sec")==0))
	{
	    suggestedAttrStat = UIManager::AttrSum;
	    suggestedEntryStat = UIManager::EntryMean;
	}

	// mpiP: All data should be averages over tasks and summed over entries
	else if (strcmp(dataAttrTag, "count")==0) {
	    suggestedAttrStat = UIManager::AttrSum;
	    suggestedEntryStat = UIManager::EntrySum;
	}
	/* MS/START: Local and Global COUNTERS */
	else if (strcmp(dataAttrTag, "loc_count")==0) {
	    suggestedAttrStat = UIManager::AttrSum;
	    suggestedEntryStat = UIManager::EntrySum;
	}
	else if (strcmp(dataAttrTag, "glob_count")==0) {
	    suggestedAttrStat = UIManager::AttrMax;
	    suggestedEntryStat = UIManager::EntryMax;
	}
	/* MS/END: Local and Global COUNTERS */
	else if (strcmp(dataAttrTag, "max_time")==0) {
	    suggestedAttrStat = UIManager::AttrMax;
	    suggestedEntryStat = UIManager::EntryMax;
	}
	else if (strcmp(dataAttrTag, "min_time")==0) {
	    suggestedAttrStat = UIManager::AttrMin;
	    suggestedEntryStat = UIManager::EntryMin;
	}
	else if (strcmp(dataAttrTag, "mean_time")==0) {
	    suggestedAttrStat = UIManager::AttrMean;
	    suggestedEntryStat = UIManager::EntryMean;
	}
	else if ( (strcmp(dataAttrTag, "app_percent_time")==0) ||
		 (strcmp(dataAttrTag, "mpi_percent_time")==0)) {
	    suggestedAttrStat = UIManager::AttrSum;
	    suggestedEntryStat = UIManager::EntryMean;
	}
	// timer: sum over task/thread and over entries
	else if ( strcmp(dataAttrTag, "time") == 0 ) {
		suggestedAttrStat = UIManager::AttrSum;
		suggestedEntryStat = UIManager::EntrySum;
	}
	/* MS/START: PMAPI COUNTERS */
	else if (strstr(dataAttrTag,"PM_")==dataAttrTag) {
	    suggestedAttrStat = UIManager::AttrSum;
	    suggestedEntryStat = UIManager::EntrySum;
	}
	/* MS/END: PMAPI COUNTERS */
	// memory usage: max over tasks, sum over entries
	else if ((strcmp(dataAttrTag, "allocSize")==0) ||
		(strcmp(dataAttrTag, "allocItems")==0) ||
		(strcmp(dataAttrTag, "reservedMem")==0) ||
		(strcmp(dataAttrTag, "unusedReseved")==0))
	{
	    suggestedAttrStat = UIManager::AttrMax;
	    suggestedEntryStat = UIManager::EntrySum;
	}
	else
	{
	    // Let tool writer know that default is being used
	    printf ("%s line %i:\n"
		    "  unpack_and_declare_data_attr: defaults being used for '%s' dataAttr\n", __FILE__, __LINE__, dataAttrTag);
	}

	um->declareDataAttr( dataAttrTag, attrName, description, mdType,
			     suggestedAttrStat, suggestedEntryStat);
}

// Get the name of a process and thread and declare them in the database
void GUISocketReader:: unpack_and_insert_process_thread( char * buf )
{
	int process;
	int thread;

	TG_unpack( buf, "II", &process, &thread );
	um->insertPTPair( process, thread );
//	printf("gsr: insert task %d thread %d\n", process, thread );
#if 0
	// Generates extra work for debugging slow updates
	int i;
	for( i = 0; i < 64; i++ ) {
		um->insertPTPair( i, thread );
	}
#endif

}

// Get a double and insert it in the appropriate location in the database
void GUISocketReader:: unpack_and_insert_double( char * buf )
{
	char * function;
	char * ip_tag;
	char * dataAttr_tag;
	int process;
	int thread;
	double value;

	TG_unpack( buf, "SSSIID", &function, &ip_tag, &dataAttr_tag,
			&process, &thread, &value );
	um->setDouble( function, ip_tag, dataAttr_tag, process, thread, value );
//	printf("gsr: set double %d thread %d\n", process, thread );
#if 0
	// Generates extra work for debugging slow updates
	int i;
	for( i = 0; i < 64; i++ ) {
		um->setDouble( function, ip_tag, dataAttr_tag, i,
				thread, value );
	}
#endif

}

// Get a int and insert it in the appropriate location in the database
void GUISocketReader:: unpack_and_insert_int( char * buf )
{
	char * function;
	char * ip_tag;
	char * dataAttr_tag;
	int process;
	int thread;
	int value;

	TG_unpack( buf, "SSSIII", &function, &ip_tag, &dataAttr_tag,
			&process, &thread, &value );
	um->setInt( function, ip_tag, dataAttr_tag, process, thread, value );
//	printf("gsr: set int %d thread %d\n", process, thread );
}

// Get a double and add it in the appropriate location in the database
void GUISocketReader:: unpack_and_add_double( char * buf )
{
	char * function;
	char * ip_tag;
	char * dataAttr_tag;
	int process;
	int thread;
	double value;

	TG_unpack( buf, "SSSIID", &function, &ip_tag, &dataAttr_tag,
			&process, &thread, &value );
	um->addDouble( function, ip_tag, dataAttr_tag, process, thread, value );

//	printf("gsr: add double %d thread %d\n", process, thread );
}

// Get an int and add it in the appropriate location in the database
void GUISocketReader:: unpack_and_add_int( char * buf )
{
	char * function;
	char * ip_tag;
	char * dataAttr_tag;
	int process;
	int thread;
	int value;

	TG_unpack( buf, "SSSIII", &function, &ip_tag, &dataAttr_tag,
			&process, &thread, &value );
	um->addInt( function, ip_tag, dataAttr_tag, process, thread, value );
//	printf("gsr: add int %d thread %d\n", process, thread );
#if 0
	// Generates extra work for debugging slow updates
	int i;
	for( i = 0; i < 64; i++ ) {
		um->addInt( function, ip_tag, dataAttr_tag, i,
				thread, value );
	}
#endif
}

// Obsolete
void GUISocketReader:: unpack_and_set_target_info( char * buf )
{
	char * program;
	char * host;

	TG_unpack( buf, "SS", &program, &host );
}

// Get file name and mark its parsing as complete
void GUISocketReader::unpack_and_set_file_parse_complete ( char * buf)
{
    char *fileName;

    TG_unpack (buf, "S", &fileName);
    um->fileSetState(fileName, UIManager::fileParsed);
#if 0
    // DEBUG
    TG_timestamp ("Marked '%s' file parse complete\n", fileName);
#endif
}

// Get function name and mark its parsing as complete
void GUISocketReader::unpack_and_set_function_parse_complete ( char * buf)
{
    char *funcName;

    TG_unpack (buf, "S", &funcName);
    um->functionSetState(funcName, UIManager::functionParsed);
#if 0
    // DEBUG
    TG_timestamp ("Marked '%s' function parse complete\n", funcName);
#endif
}

// Get result from changing the directory, and post a warning if it failed
// FIX: we probably need to cleanup and fail gracefully in something's wrong
int GUISocketReader::unpack_cd_result( char * buf )
{
	int result;
	char * message;
	
	char out[1000];
	TG_unpack( buf, "IS", &result, &message );

	if( result != 0 ) {
		snprintf( out, 1000, "Remote directory change failed: %s",
				message );
		QMessageBox::warning( 0, um->getProgramName(), out );
		return DPCL_SAYS_QUIT;
	}

	return CONTINUE_THREAD;
}

// Get result from initializing the app, and post a warning if it failed
int GUISocketReader::unpack_init_app_result( char * buf )
{
	int result;
	char * message;
	
	char out[1000];
	TG_unpack( buf, "IS", &result, &message );

	if( result != 0 ) {
		snprintf( out, 1000,
				"Failed to initialize target application: %s",
				message );
		emit setProgramMessage( out );
		QMessageBox::warning( 0, um->getProgramName(), out );
		return DPCL_SAYS_QUIT;
	}

	emit programActive(TRUE);
	emit setProgramMessage( "Ready to run; press Run to start" );


	return DPCL_PARSE_COMPLETE;
}

// Target halted (because it hit a breakpoint)
void GUISocketReader::unpack_target_halted( char * buf )
{
	char * function;
	char * ip_tag;
	int process_id;
	char msg[1000];

	TG_unpack( buf, "SSI", &function, &ip_tag, &process_id );

	programState->setActivity(TGProgramState::Stopped);

	// FIX need to highlight the breakpoint somehow, but for now
	// we'll just print a message saying where we stopped.
	sprintf( msg, "Process %d stopped in %s; press Run to resume",
			process_id, function );
	emit programRunning(FALSE);
	emit setProgramMessage( msg );
}

void GUISocketReader::unpack_target_terminated( char * buf )
{
	char * process_name;

	TG_unpack( buf, "S", &process_name );

	programState->setActivity(TGProgramState::Terminated);

	emit programActive(FALSE);
	emit setProgramMessage( "Program has terminated" );
	emit processTerminated( process_name );
}

void GUISocketReader:: unpack_about_text( char * buf )
{
	char * about_string;
	TG_unpack( buf, "S", &about_string );

	um->addAboutText( about_string );

}

// Declare a new messageFolder in the UIManager
void GUISocketReader::unpack_and_declare_message_folder (char * buf)
{
    char * messageFolderTag;
    char * messageFolderTitle;

    TG_unpack( buf, "SS", &messageFolderTag, &messageFolderTitle);
    um->declareMessageFolder (messageFolderTag, messageFolderTitle);
}

// Declare a new message in a messageFolder in the UIManager
void GUISocketReader::unpack_and_add_message (char * buf)
{
	char * messageFolderTag;
	char * messageText;
	char * messageTraceback;

	TG_unpack( buf, "SSS", &messageFolderTag, &messageText,
		   &messageTraceback);

	um->addMessage (messageFolderTag, messageText,
			messageTraceback);
}

// Process XML snippet and do recognized commands
void GUISocketReader::unpack_and_process_xml_snippet (char *buf)
{
        char * XMLSnippet;
	int lineOffset;
  
        TG_unpack( buf, "IS", &lineOffset, &XMLSnippet);

        um->processXMLSnippet (XMLSnippet, lineOffset);
}

void GUISocketReader:: unpack_subdir_list( char * buf )
{
	char * p1str;
	char * p2str;
	char * parent_path;
	char * dirpath;

	TG_unpack( buf, "SSSS", &p1str, &p2str, &parent_path, &dirpath );


	SearchPathDialog * sd;
	void * parent_node;

	sscanf( p1str, "%p", &sd );
	sscanf( p2str, "%p", &parent_node );

	if( *parent_path == '\0' ) {
		QMessageBox::warning( 0, um->getProgramName(), 
			       "That directory was not found" );
	} else {
		sd->populateDirectoryList( QString(parent_path),
				QString(dirpath), parent_node );
	}
}

void GUISocketReader:: unpack_search_path( char * buf )
{
	char * p1str;
	char * searchpath;

	TG_unpack( buf, "SS", &p1str, &searchpath );

	SearchPathDialog * sd;

	sscanf( p1str, "%p", &sd );

	sd->populateSearchPath( QString(searchpath) );
}
/*--------------------------------------------------------------------------*/
/* MS/START - dynamic module loading */

void GUISocketReader:: unpack_and_countDynamicModules( char * buf )
{
  TG_unpack( buf, "I", &number_of_modules );

  module_table  =(char**)malloc(sizeof(char*)*number_of_modules);
  module_table_s=(char**)malloc(sizeof(char*)*number_of_modules);
}

void GUISocketReader:: unpack_and_recordDynamicModules( char * buf )
{
  char *tmp1,*tmp2;
  int slot;

  TG_unpack( buf, "ISS", &slot, &tmp1,&tmp2);
  if ((slot>=0) && (slot<number_of_modules))
    {
      module_table[slot]=(char*) malloc(sizeof(char)*strlen(tmp1)+1);
      strcpy(module_table[slot],tmp1);
      module_table_s[slot]=(char*) malloc(sizeof(char)*strlen(tmp2)+1);
      strcpy(module_table_s[slot],tmp2);
    }
}

#ifndef BUFFER_SIZE
#define BUFFER_SIZE (1<<14)
#endif

#ifdef SIMPLE_MODE
void GUISocketReader:: unpack_and_queryDynamicModules( char * buf, int fd )
{
  int i;
  char sendbuf[BUFFER_SIZE];
  int length;

  printf("\nLIST OF AVAILABLE DPCL MODULES\n");
  for (i=0; i<number_of_modules; i++)
    {
      if (strcmp(module_table_s[i],"NONE")!=0)
	{
	  printf("\t MODULE %i / %8s: %s\n",i,module_table_s[i],module_table[i]);
	  
	  length = TG_pack( sendbuf, BUFFER_SIZE, "I", i);
	  
	  TG_send( fd, DYNCOLLECT_LOADMODULE, 0, length, sendbuf );
	  TG_flush( fd );	
	}
    }
  printf("\n");

  length = TG_pack( sendbuf, BUFFER_SIZE, "I", number_of_modules);
  
  TG_send( fd, DYNCOLLECT_ALLLOADED, 0, length, sendbuf );
  TG_flush( fd );	  
}
#else 
#ifdef USE_MODAL
void GUISocketReader:: unpack_and_queryDynamicModules( char * buf, int fd )
{
  int i,avail_mod;
  char sendbuf[BUFFER_SIZE];
  char name[200];
  int length;

  QDialog *dlg;
  QPushButton *dlg_but_ok, *dlg_but_cancel;
  QCheckBox **dlg_chkbox;
  int res;

  dlg = new QDialog(0,"Which DPCL would you like to use?",TRUE,0);

  dlg_but_ok= new QPushButton("OK",dlg,"OK");
  connect(dlg_but_ok,SIGNAL(clicked()),dlg,SLOT(accept()));

  dlg_but_cancel = new QPushButton("Cancel",dlg,"Cancel");
  connect(dlg_but_cancel,SIGNAL(clicked()),dlg,SLOT(reject()));

  dlg_chkbox = (QCheckBox**) malloc(sizeof(QCheckBox*)*number_of_modules);
  if (dlg_chkbox==NULL)
    {
      TG_send( fd, GUI_SAYS_QUIT, 0, 0, 0 );
      TG_flush( fd );	 
      printf("Memory Error\n");

      delete dlg_but_ok;
      delete dlg_but_cancel;
      delete dlg;
      exit(0);
    }

  avail_mod=0;

  for (i=0; i<number_of_modules; i++)
    {
      if (strcmp(module_table_s[i],"NONE")!=0)
	{
	  sprintf(name,"%s: %s\n",module_table_s[i],module_table[i]);
	  
	  dlg_chkbox[i] = new QCheckBox(name,dlg,"Available Modules");
	  dlg_chkbox[i]->setGeometry(10,10+20*i,380,15);
	  dlg_chkbox[i]->setChecked(TRUE);
	  avail_mod++;
	}
      else
	dlg_chkbox[i]=NULL;
    }

  dlg_but_ok->setGeometry(80,avail_mod*20+20,100,30);
  dlg_but_cancel->setGeometry(220,avail_mod*20+20,100,30);

  dlg->setGeometry(100,100,400,avail_mod*20+60);
  
  res=dlg->exec();

  if (res==QDialog::Accepted)
    {
      printf("\nLIST OF THE ACTIVATED DPCL MODULES\n");
      printf("\n");

      for (i=0; i<number_of_modules; i++)
	{
	  if (dlg_chkbox[i]!=NULL)
	    {
	      if (dlg_chkbox[i]->isChecked())
		{
		  printf("\t MODULE %i / %8s: %s\n",i,module_table_s[i],module_table[i]);
	  
		  length = TG_pack( sendbuf, BUFFER_SIZE, "I", i);
	  
		  TG_send( fd, DYNCOLLECT_LOADMODULE, 0, length, sendbuf );
		  TG_flush( fd );	
		}
	    }
	}

      TG_send( fd, DYNCOLLECT_ALLLOADED, 0, 0, 0 );
      TG_flush( fd );	  

      for (i=0; i<number_of_modules; i++)
	{
	  delete dlg_chkbox[i];
	}
      free(dlg_chkbox);
      delete dlg_but_ok;
      delete dlg_but_cancel;
      delete dlg;

      check_socket();
    }
  else
    {
      TG_send( fd, GUI_SAYS_QUIT, 0, 0, 0 );
      TG_flush( fd );	 
      printf("User Cancel\n");

      for (i=0; i<number_of_modules; i++)
	{
	  delete dlg_chkbox[i];
	}
      free(dlg_chkbox);
      delete dlg_but_ok;
      delete dlg_but_cancel;
      delete dlg;
      exit(0);
    }

  
}
#else
void GUISocketReader:: unpack_and_queryDynamicModules( char * /*buf*/, int fd )
{
  GUIQueryModuleDialog *moddlg;

  moddlg = new GUIQueryModuleDialog(number_of_modules, module_table,
				    module_table_s,fd);
  moddlg->exec();
}
#endif
#endif

/* MS/END - dynamic module loading */
/*--------------------------------------------------------------------------*/


//#include "gui_socket_reader.moc"
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

