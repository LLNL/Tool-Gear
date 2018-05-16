//! \file memcheck2tgxml.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.02                                               Sept 8, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// Converts Valgrind Memcheck's 3.0.1's XML output to Tool Gear's XML for
// display by memcheckview.
// Written by John Gyllenhaal June 2005

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
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
#include <time.h>

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif
#include "tg_types.h"
#include "stringtable.h"
#include "lineparser.h"
#include "tempcharbuf.h"
#include "messagebuffer.h"
#include "logfile.h"
#include "tg_time.h"
#include "qxml.h"

//! Create table of declared folder tags but DONTautomatically cleaning up 
//! at end (because strdup uses malloc, not new, which causes a malloc/delete
//! mismatch.  Better to have a leak (the program is exiting anyway) than
//! some weird memory problem.
//!
//! I don't like making initialized static variables that are classes (since 
//! there are restrictions on what that class can do, like cannot use I/O) and
//! we may want to do something different if this turns into a problem.
StringTable<char> folderTagTable("folderTag", FreeData);

// Helper routine that prints out Xml to declare folder and 
// registers folderTags that we know about it (so we can handle unknown
// memcheck "kind" types better)
void declareFolder (FILE *out, const char *folderTag, 
		     const char *folderTitle, const char *ifEmptyPolicy)
{
    // If already in folderTag table, do nothing
    if (folderTagTable.entryExists(folderTag))
	return;

    // Emit TG xml to create message folder
    fprintf (out,
	     "<message_folder>\n"
	     "  <tag>%s</tag>\n"
	     "  <title>%s</title>\n"
	     "  <if_empty>%s</if_empty>\n"
	     "</message_folder>\n"
	     "\n",
	     folderTag, folderTitle, ifEmptyPolicy);

    // Add folderTag to table, strduping title as the table entry 
    folderTagTable.addEntry(folderTag, strdup(folderTitle));
}


//! Used to map XML elements to ids.  
struct XML_Token {
    XML_Token(int t, unsigned int l) : token(t), levelMask(l) {}
    int token;
    unsigned int levelMask;
};

//! Used to hold state for sorting messages based on count
class ErrorInfo
{
public:
    ErrorInfo(): sortValue(0.0), count(0), msg(NULL) {}
    double sortValue;     // Use double so can encode tiebreaker
    int count;
    const char *msg;  // Message to print with out
};

//! Used to hold state across XML snippets.   We are doing incremental XML 
//! parsing, so very little state can be saved in parser
class MemcheckInfo
{
public:
    MemcheckInfo() : 
	// Create unknownXML table that frees levelmask on delete
	unknownXMLTable("unknownXML", DeleteData, 0),
	
	// Create xmlToken table that frees XML_Token structures on delete
	xmlTokenTable("xmlToken", DeleteData, 0),

	// Create message table that uses free to free string on delete
	messageTable ("message", FreeData, 0),
	
	numMessages(0), errorArray(NULL), errorArrayCount(0), 

	prologText(""), valgrindInvoke(""), valgrindInvokeLen(0),
	appExe(""), appInvoke(""), appInvokeLen(0),

	suppText(""), 

	xmlEnded (0),  minutesIdle(0), xmlOut(NULL)

	{}
    
    //! Only warn about unknown XML elements once per invocation
    //! Use int to store level mask, so warn for each level
    //! (each snippet parsed separately, so cannot put in XML parser)
    StringTable<int> unknownXMLTable;

    //! Only want to populate mapping table once, so put this here.  
    StringTable<XML_Token> xmlTokenTable;

    //! Want to print out messages annotated with counts at end.
    //! Store XML for message for later annotation, indixed by unique
    StringTable<char> messageTable;

    //! Count the number of messages in xml under 'error'
    int numMessages;

    //! Array that we create to hold messages for sorting
    ErrorInfo *errorArray;

    //! Count the number of messages being sorted by count
    int errorArrayCount;

    //! Holds the concatenation of all the prolog text
    QString prologText;

    //! Holds valgrind arguments
    QString valgrindInvoke;
    int valgrindInvokeLen;

    //! Holds app name and arguments
    QString appExe;
    QString appInvoke;
    int appInvokeLen;

    //! Holds supplimental counts
    QString suppText;

    //! Set to 1 when end of Valgrind XML reached
    int xmlEnded;

    //! Count the minutes since the last Valgrind XML input was read
    int minutesIdle;

    //! Hold pointer to xml output file
    FILE *xmlOut;
};

//! Class for incrementally grabbing coherient Tool Gear XML snippets from
//! a file.   
class XMLSnippetParser
{
public:
    XMLSnippetParser (FILE *file_in) : in (file_in), partialParse(FALSE),
				       lastLT(0), lineNo(1),
				       snippetLineOffset(0)
	{
	}
    const char *getNextSnippet(MemcheckInfo *mcinfo)
	{
#if 0
	    // Believe no longer needed and suppresses error messages
	    // after valgrind end marker
	    // If encountered XML parse error, don't parse anymore because
	    // it will just generate extra random parse errors
	    if (mcinfo->xmlEnded)
	    {
		return (NULL);
	    }
#endif

	    // If not mid parse, clear current snippet contents and state
	    if (!partialParse)
	    {
		// Clear snippet buffer
		sbuf.clear();

		// Last < symbol at start of buffer
		lastLT = 0;

		// Record how many lines skipped before the snippet
		snippetLineOffset = lineNo-1;
	    }
	    
	    // Read characters until run out of characters or hit
	    // the end of a snippet
	    int ch;
	    while ((ch = fgetc(in)) != EOF)
	    {
		// Check for bad XML, either because of Valgrind FATAL error
		// or because a non-xml Valgrind file was passed to this
		// parser.  Need to check when hit '<', '>' or 'EOF' in
		// order to catch the cases before other sanity checks fail.
		// Optimize a little by only doing check when lastLT == 0.
		if ((lastLT == 0) && 
		    ((ch == '<') || (ch == '>')))
		{
		    // Get current partial contents to test
		    const char *scanPtr = sbuf.contents();
		    
		    // Skip leading whitespace on contents
		    while ((*scanPtr != 0) && isspace(*scanPtr))
			scanPtr++;
		    
		    // Expect scanPtr to either now be empty or start 
		    // with '<'.   
		    if ((*scanPtr != 0) && (*scanPtr != '<'))
		    {
			// Have fatal parse error, append rest of text
			// available for reading before output message
			// (because random '<' and '>' will truncate the
			// message otherwise
			sbuf.appendChar(ch);
			while ((ch = fgetc(in)) != EOF)
			{
			    sbuf.appendChar(ch);
			}
			
			// Get pointer to completed buffer now (old scanPtr
			// no longer valid
			scanPtr = sbuf.contents();

			// Skip leading whitespace on contents
			while ((*scanPtr != 0) && isspace(*scanPtr))
			    scanPtr++;

			// Get local copy of unexpected text before
			// using sbuf to write out error message.
			QString unexpectedText = scanPtr;
			
			// Warn users that something weird has happened 
			fprintf (stderr, 
				 "\nError: Unexpected text in Valgrind XML "
				 "file:\n"
				 "%s\n"
				 "Assuming FATAL Valgrind error has occurred "
				 "(since not valid XML).\n",
				 unexpectedText.latin1());
			
			// Create fake XML from error detected
			sbuf.sprintf ("<FATAL>%s</FATAL>\n",
				      unexpectedText.latin1());
			
			// Mark that we have reached the end of the XML
			mcinfo->xmlEnded = 1;
			
			// Indicate memcheck terminated abnormally
			fprintf (mcinfo->xmlOut,
				 "<status> XML parse error (assuming fatal "
				 "Valgrind error has occurred)</status>\n");
			
			fflush (mcinfo->xmlOut);
			
			// Treat as if have valid XML FATAL snippet
			partialParse = FALSE;
			return (sbuf.contents());
		    }
		}

		// If got '<', get index in string we are going to place it
		if (ch == '<')
		{
		    // The last '<' will be placed at the end of the string
		    lastLT = sbuf.strlen();
		}

		// Add character to end of snippet buffer
		sbuf.appendChar(ch);

		// If got '>', see if at end of known snippet terminator
		if (ch == '>')
		{
		    // Get pointer to last element or end of element marker
		    const char *element = sbuf.contents() + lastLT;
		    
		    // DEBUG
//		    fprintf (stderr, "Last element: '%s' (%i-%i)\n", element,
//			     lastLT, sbuf.strlen());


		    // Is it a start valgrindoutput marker?
		    if (isStartElement (element, "valgrindoutput"))
		    {
//			fprintf (stderr, "Deleting valgrind marker '%s'!\n",
//				 element);
			sbuf.truncate(lastLT);
		    }
		    // Is it a start <?xml version="1.0"?> marker?
		    else if (isStartElement (element, "?xml"))
		    {
//			fprintf (stderr, "Deleting xml version marker '%s'!\n",
//				 element);
			sbuf.truncate(lastLT);
		    }

		    // Is it a end of element marker we recognize?
		    else if (isEndElement (element, "protocolversion") ||
			     isEndElement (element, "preamble") ||
			     isEndElement (element, "pid") ||
			     isEndElement (element, "ppid") ||
			     isEndElement (element, "tool") ||
			     isEndElement (element, "usercomment") ||
			     isEndElement (element, "args") ||
			     isEndElement (element, "status") ||
			     isEndElement (element, "error") ||
			     isEndElement (element, "errorcounts") ||
			     isEndElement (element, "suppcounts"))
		    {
//			fprintf (stderr, "End snippet marker %s detected!\n", 
//				 element);
			partialParse = FALSE;
			return (sbuf.contents());
		    }

		    // Is it a end valgrindoutput marker?
		    else if (isEndElement (element, "valgrindoutput"))
		    {

//			fflush (stdout);
//			fprintf (stderr, "Deleting valgrind marker '%s'!\n",
//				 element);

			// Mark that we have reached the end of the XML
			mcinfo->xmlEnded = 1;

			// Indicate memcheck exited normally
			fprintf (mcinfo->xmlOut,
				 "<status>Memcheck exited "
				 "normally</status>\n"
				 "\n");
			fflush (mcinfo->xmlOut);


			sbuf.truncate(lastLT);

			// If has XML in there, return it now
			const char *ptr = sbuf.contents();
			char ch;
			while ((ch=*ptr) != 0)
			{
			    if (ch == '<')
			    {
				fprintf (stderr, 
					 "\nTool Gear Valgrind XML Parser Warning: \n"
					 "   Unrecognized XML at end, sending:\n"
					 "   '%s'\n",
					 sbuf.contents());
				partialParse = FALSE;
				return (sbuf.contents());
			    }
			    ptr++;
			}
		    }
		}

		// Increment line number if newline
		if (ch == '\n')
		    ++lineNo;
	    }

	    // Detect Valgrind printing out error message (not in XML in 
	    // Valgrind 3.0.1, may be in later versions)
	    // Only do test if sbuf is not empty
	    if (sbuf.strlen() > 0)
	    {
		// Get current partial contents to test
		const char *scanPtr = sbuf.contents();
		
		// Skip leading whitespace on contents
		while ((*scanPtr != 0) && isspace(*scanPtr))
		    scanPtr++;
		
		// Expect scanPtr to either now be empty or start with '<'.   
		if ((*scanPtr != 0) && (*scanPtr != '<'))
		{
		    // Get local copy of unexpected text before
		    // using sbuf to write out error message.
		    QString unexpectedText = scanPtr;
		    
		    // Warn users that something weird has happened 
		    fprintf (stderr, 
			     "\nError: Unexpected text in Valgrind XML "
			     "file:\n"
			     "%s\n"
			     "Assuming FATAL Valgrind error has occurred "
			     "(since not valid XML).\n",
			     unexpectedText.latin1());
		    
		    // Create fake XML from error detected
		    sbuf.sprintf ("<FATAL>%s</FATAL>\n",
				  unexpectedText.latin1());
		    
		    // Mark that we have reached the end of the XML
		    mcinfo->xmlEnded = 1;
		    
		    // Indicate memcheck terminated abnormally
		    fprintf (mcinfo->xmlOut,
			     "<status> XML parse error (assuming fatal "
			     "Valgrind error has occurred)</status>\n");
		    
		    fflush (mcinfo->xmlOut);
		    
		    // Treat as if have valid XML FATAL snippet
		    partialParse = FALSE;
		    return (sbuf.contents());
		}
	    }

	    // If got here, must be in partial parse
	    partialParse = TRUE;


	    // Return that there is no complete snippet ready yet
	    return (NULL);
	}

    // Returns the number of lines skipped/processed before the snippet
    int getSnippetOffset () {return (snippetLineOffset);}

private:
    bool isEndElement (const char *element, const char *name)
	{
	    // Expect element[0] == '<'
	    if (element[0] != '<')
	    {
		fprintf (stderr, "Error in XMLSnippetParser::isEndElement: \n"
			 "  Expect < not '%c' in '%s'!\n", element[0],
			 element);
		exit (1);
	    }

	    // For now, treat as end element only if starts with </
	    // (Not expecting <foo/> right now as select ending element)
	    if (element [1] != '/')
		return (FALSE);
	    
	    // Get length of name to limit search
	    int nameLen = strlen (name);
	    
	    // Expect name to follow / if end element of this name
	    if (strncmp (&element[2], name, nameLen) != 0)
		return (FALSE);

	    // Have </name but need to make sure 'name' not part of
	    // longer name by testing character after name in element
	    // (that is, distinguish </message> from </message_folder>
	    int nextChar = element[2+nameLen];
	    if ((nextChar != '>') && !isspace(nextChar))
		return (FALSE);

	    // If got here, must be end element for 'name'
	    return (TRUE);
	}

    bool isStartElement (const char *element, const char *name)
	{
	    // Expect element[0] == '<'
	    if (element[0] != '<')
	    {
		fprintf (stderr, "Error in XMLSnippetParser::isStartElement: "
			 "Expect < not '%c' in '%s'!\n", element[0], element);
		exit (1);
	    }

	    // Get length of name to limit search
	    int nameLen = strlen (name);
	    
	    // Expect name to follow < if start element of this name
	    if (strncmp (&element[1], name, nameLen) != 0)
		return (FALSE);

	    // Have <name but need to make sure 'name' not part of
	    // longer name by testing character after name in element
	    // (that is, distinguish <message> from <message_folder>
	    int nextChar = element[1+nameLen];
	    if ((nextChar != '>') && !isspace(nextChar))
		return (FALSE);

	    // If got here, must be end element for 'name'
	    return (TRUE);
	}


    MessageBuffer sbuf;
    FILE *in;
    bool partialParse;
    int lastLT;
    int lineNo;
    int snippetLineOffset;
};

class MCXMLParser : public QXmlDefaultHandler
{
    
// Convert element names into enums for quick checking of state
enum XMLElementToken
{
    XML_NULL,
    XML_unknown,
    XML_protocolversion, 
    XML_preamble, 
    XML_line, 
    XML_pid, 
    XML_ppid, 
    XML_tool,
    XML_usercomment,
    XML_date,
    XML_hostname,
    XML_rank,
    XML_args,
    XML_vargv, 
    XML_argv, 
    XML_exe, 
    XML_arg, 
    XML_error, 
    XML_unique, 
    XML_tid, 
    XML_kind, 
    XML_what, 
    XML_leakedbytes, 
    XML_leakedblocks, 
    XML_stack, 
    XML_frame, 
    XML_ip, 
    XML_obj, 
    XML_fn, 
    XML_dir, 
    XML_file, 
    XML_auxwhat, 
    XML_errorcounts, 
    XML_pair, 
    XML_count, 
    XML_status, 
    XML_state,
    XML_time,
    XML_suppcounts, 
    XML_name,
    XML_FATAL
};

public:
    MCXMLParser(FILE *out, MemcheckInfo *mci, QString &xml, int lineNoOffset) :
        mcinfo(mci), xml_out(out), xml_text(xml), lineOffset(lineNoOffset),
	lineNoGuess(1) 
	{
	    // Populate XML id table, if not already populated
	    if (!mcinfo->xmlTokenTable.entryExists("error"))
	    {

		// Use C preprocessor tricks to prevent mistakes and
		// make it easier.   The #name expands to "(name)"
		// and XML_##name expands to the value defined for
		// XML_(name).
#define declareToken(name,level) declareToken_(#name,XML_##name,level)

//		fprintf (stderr, "Initializing xmlTokenTable!\n");

		// Declare all the valid tokens and the levels they
		// are expected to occur on

		// protocolversion token
		declareToken(protocolversion, 0);

		// preamble tokens
		declareToken(preamble, 0);
		declareToken(line, 1);
		
		// pid token
		declareToken(pid, 0);

		// ppid token
		declareToken(ppid, 0);

		// tool token
		declareToken(tool, 0);

		// usercomment tokens (inserted by memcheck scripts)
		declareToken(usercomment, 0);
		declareToken(date, 1);
		declareToken(hostname, 1);
		declareToken(rank, 1);

		// argv tokens
		declareToken(args, 0);
		declareToken(vargv, 1);
		declareToken(argv, 1);
		declareToken(exe, 2);
		declareToken(arg, 2);
		
		// error tokens
		declareToken(error, 0);
		declareToken(unique, 1);
		declareToken(tid, 1);
		declareToken(kind, 1);
		declareToken(what, 1);
		declareToken(leakedbytes, 1);
		declareToken(leakedblocks, 1);
		declareToken(stack, 1);
		declareToken(frame, 2);
		declareToken(ip, 3);
		declareToken(obj, 3);
		declareToken(fn, 3);
		declareToken(dir, 3);
		declareToken(file, 3);
		declareToken(line, 3);
		declareToken(auxwhat, 1);

		// errorcounts tokens
		declareToken(errorcounts, 0);
		declareToken(pair, 1);
		declareToken(count, 2);
		declareToken(unique, 2);

		// status tokens
		declareToken(status, 0);
		declareToken(state, 1);
		declareToken(time, 1);

		// suppcounts tokens
		declareToken(suppcounts, 0);
		declareToken(pair, 1);
		declareToken(count, 2);
		declareToken(name, 2);

		// FATAL tokens (created by get_snippet when weirdnes happens)
		declareToken(FATAL, 0);
	    }
	}

    bool startDocument() 
	{
	    nestLevel = -1;
	    for (int i=0; i < 10; i++)
	    {
		elementTokenAt[i] = XML_NULL;
		elementNameAt[i] = "";
		valueAt[i] = "";
	    }
	    // No error, continue parsing
	    return (TRUE);
	}

    bool startElement( const QString&, const QString&, 
		       const QString& elementName,
                       const QXmlAttributes& );


    bool endElement( const QString&, const QString&, 
		     const QString& elementName);

    bool characters ( const QString & incoming_text )
	{
	    // Use existing MessageBuffer since has fast append and
	    // can handle any text size (but clear contents first)
	    filtered_text.clear();

	    // Since we are emitting XML based on text we don't control,
	    // we need to replace '<' and '&' in the incoming text with
	    // the XML equivalents.   The XML documentation also recommends
	    // replacing '>' ''' and '"' with XML replacements also,
	    // although it is not illegal to have them in there.
	    const char *ptr = (const char *)incoming_text;
	    char ch;
	    while ((ch = *ptr) != 0)
	    {
		// Increment line number guess based on newlines incoming_text
		if (ch == '\n')
		    lineNoGuess++;

		// Replace '<' with '&lt;'
		if (ch == '<')
		    filtered_text.appendSprintf("&lt;");
		else if (ch == '&')
		    filtered_text.appendSprintf("&amp;");
		else if (ch == '>')
		    filtered_text.appendSprintf("&gt;");
		else if (ch == '\'')
		    filtered_text.appendSprintf("&apos;");
		else if (ch == '"')
		    filtered_text.appendSprintf("&quot;");
		else
		    filtered_text.appendChar(ch);

		// Goto next character in incoming_text
		++ptr;
	    }

	    
	    // Append filtered text to the current value at this level
	    valueAt[nestLevel].append(filtered_text.contents());
	    
	    // No error, continue parsing
	    return (TRUE);
	}

    bool fatalError ( const QXmlParseException & exception )
	{
	    error_message = "Fatal XML parse error";

	    // Print out location and description of error
	    fprintf (stderr, 
		     "\nXML parse error at line %i column %i: %s\n",
		     exception.lineNumber() + lineOffset,
		     exception.columnNumber(),
		     (const char *)exception.message());

	    // Print out line that caused error
	    printErrorContext (exception.lineNumber(),
			       exception.columnNumber());

	    // Print out XML stack when error occurred
	    QString indent="";
	    fprintf (stderr, "XML stack at time of parse error:\n");
	    for (int level=0; (level < nestLevel); ++level)
	    {
		// Currently only store the first 10 levels of XML stack
		if (level < 10)
		{
		    fprintf (stderr, "%s<%s>\n", 
			     (const char *)indent,
			     (const char *)elementNameAt[level]);
		}
		else
		{
		    fprintf (stderr, "%s<(Element name not stored for level %i)>\n", 
			     (const char *) indent, 
			     level);
		}
		indent.append("  ");
	    }
	    fprintf (stderr, 
		     "\nWarning: Discarding rest of XML snippet due to "
		     "parse error.\n\n");


	    // Stop parsing this snippet of XML
	    return FALSE;
	}
    bool error ( const QXmlParseException & exception )
	{
	    // Handle same way as fatal error for now
	    fatalError (exception);

	    // Stop parsing this snippet of XML
	    return FALSE;
	}
    bool warning ( const QXmlParseException & exception )
	{
	    fprintf (stderr, 
		     "\nXML Parse warning: Line %i, column %i: %s\n",
		     exception.lineNumber()+lineOffset, 
		     exception.columnNumber(),
		     exception.message().latin1());

	    // Print out line that caused warning
	    printErrorContext (exception.lineNumber(),
			       exception.columnNumber());

	    fprintf (stderr, "\n");

	    // Continue parsing XML
	    return TRUE;
 	}

    QString errorString() {return (error_message);};

    void printErrorContext (int errorLine, int errorColumn)
	{
	    // Get the XML we are parsing as characters
	    const char *xml = (const char *)xml_text;
	    const char *startPtr, *printPtr;
	    int lineNo = 1;

	    // Scan through to find the begining of the problem line
	    for (startPtr = xml; (*startPtr != 0); ++startPtr)
	    {
		// Stop when hit beginning of problem line
		if (lineNo >= errorLine)
		    break;

		// Increment lineNo after each newline
		if (*startPtr == '\n')
		    ++lineNo;
	    }
	    
	    // Print out the next line
	    for (printPtr = startPtr; (*printPtr != 0) && (*printPtr != '\n');
		 ++printPtr)
	    {
		fputc (*printPtr, stderr);
	    }
	    fputc ('\n', stderr);

	    if (errorColumn >= 0)
	    {
		// Print out arrow to column where error occurred
		for (int col = 1; col < errorColumn; ++col)
		    fputc (' ', stderr);
		
		fprintf (stderr, "^\n");
	    }
	}

private:
    // Returns int value at nestLevel (implicit) or NULL_INT if it is
    // not a valid int or not between minInt and maxInt (which may
    // both be NULL_INT (and default to that)
    int xmlConvertToInt(int minInt = NULL_INT, int maxInt = NULL_INT)
	{
	    if (nestLevel >= 10)
	    {
		fprintf (stderr, "Error MCXMLParser::xmlConvertToInt: "
			 "nestLevel(%i) >= 10 unsupported!\n", nestLevel);

		// Return Invalid value
		return (NULL_INT);
	    }

	    // Get string to convert
	    const char *startPtr = (const char *)valueAt[nestLevel];

	    // Strip off leading whitespace
	    char ch;
	    while ((ch = *startPtr) != 0)
	    {
		if (isspace (ch))
		    startPtr++;
		else
		    break;
	    }

	    // Warn and ignore empty values
	    if (ch == 0)
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring empty XML int value for"
			 " '%s' on line %i:\n"
			 "  ",
			 (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_INT);
	    }
	    
	    // Convert to int
	    char *endPtr = NULL;
	    int val = strtol(startPtr, &endPtr, 0);

	    // Strip off trailing whitespace
	    while ((ch = *endPtr) != 0)
	    {
		if (isspace (ch))
		    endPtr++;
		else
		    break;
	    }

	    // Make sure converted everything that is not whitespace in string
	    if (*endPtr != 0)
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring invalid XML int value '%s' for\n"
			 "  '%s' on line %i:\n"
			 "  ",
			 startPtr, (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_INT);
	    }

	    // If bounded, test bounds
	    if ((minInt != NULL_INT) && (val < minInt))
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring too small XML int value %i (< %i) for\n"
			 "  '%s' on line %i:\n"
			 "  ",
			  val, minInt,
			 (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_INT);
		
	    }
	    if ((maxInt != NULL_INT) && (val > maxInt))
	    {
		fprintf (stderr, 
			 "Warning: Tool Gear ignoring too large XML int value %i (> %i) for\n"
			 "  '%s' on line %i:\n"
			 "  ",
			  val, maxInt,
			 (const char *)elementNameAt[nestLevel],
			 lineNoGuess+lineOffset);
		
		// Print out line that caused error
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, "\n");

		// Return Invalid value
		return (NULL_INT);
	    }

	    // Value must be ok if got here, return it
	    return (val);
	}

    XMLElementToken getToken (const char *name, int nestLevel)
	{

	    // Set the appropriate bit to test/set for this nestLevel
	    int levelBit;
		
	    // Use a bit for each level below 31.  Treat all levels
	    // above 31 the same (don't expect that deep but who knows)
	    if (nestLevel < 31)
		levelBit= (1 << nestLevel);
	    else
		levelBit=(1 << 31);

	    // Default to unknown token;
	    XMLElementToken token = XML_unknown;

	    // Is token in table?
	    XML_Token *entry = mcinfo->xmlTokenTable.findEntry(name);
	    
	    // Yes, token in table
	    if (entry != NULL)
	    {
		// Set token if levelBit is in levelMask
		if ((entry->levelMask & levelBit) != 0)
		    token = (XMLElementToken)entry->token;
	    }

//	    fprintf (stderr, "getToken(%s, %i) returns %i\n", name, nestLevel,
//		     (int)token);

	    // Return token found
	    return (token);
	}

    // Utility funciton for constructor
    void declareToken_(const char *name, XMLElementToken token, int nestLevel)
	{
//	    fprintf (stderr, "declareToken_(%s, %i, %i)\n",
//		     name, token, nestLevel);

	    // Set the appropriate bit to test/set for this nestLevel
	    int levelBit;
		
	    // Use a bit for each level below 31.  Treat all levels
	    // above 31 the same (don't expect that deep but who knows)
	    if (nestLevel < 31)
		levelBit= (1 << nestLevel);
	    else
		levelBit=(1 << 31);

	    // Is it already in table?
	    XML_Token *entry = mcinfo->xmlTokenTable.findEntry(name);
	    
	    // Yes, add this levelBit to it
	    if (entry != NULL)
	    {
		entry->levelMask |= levelBit;
	    }

	    // No, create entry and add to table
	    else
	    {
		entry = new XML_Token((int) token, levelBit);
		mcinfo->xmlTokenTable.addEntry(name, entry);
	    }
	}

    MemcheckInfo *mcinfo;
    FILE *xml_out;
    int nestLevel;
    XMLElementToken elementTokenAt[10];
    QString elementNameAt[10];
    QString valueAt[10];

    // For filtering '<', '&', etc. out of incoming text
    MessageBuffer filtered_text;

    // Values for error parameters
    QString error_unique;
    int error_tid;
    QString error_kind;
    QString error_what;
    int error_what_use_count;    // Must use at least once or lose info
    QString error_auxwhat;
    int error_auxwhat_use_count; // Must use at least once or lose info
    QString error_location;     // Root location of message
    MessageBuffer error_xml_traceback; // MessageBuffer has fast appendSprintf
    MessageBuffer error_text_traceback; // MessageBuffer has fast appendSprintf
    MessageBuffer xml_tbbuf;      // MessageBuffer has fast appendSprintf
    MessageBuffer text_tbbuf;     // MessageBuffer has fast appendSprintf
    MessageBuffer path_buf;     // MessageBuffer has fast appendSprintf
    QString error_frame_ip;
    QString error_frame_obj;
    QString error_frame_fn;
    QString error_frame_dir;
    QString error_frame_file;
    QString error_frame_line;
    int error_frame_count;
    int error_stack_count;
    QString pair_unique;
    QString pair_name;
    int pair_count;
    
    // Valgrind XML protocol used (expect it to be 1 or 2)
    int protocolVersion;

    // Preamble text which we add to the about text
    QString preamble;

    // path to valgrind executable being used
    QString valgrind_exe;
    
    // Valgrind arguments invoked (one per line)
    QString valgrind_args;

    // Executable being checked
    QString app_exe;

    // Arguments to executable being checked
    QString app_args;

    // Status fields
    QString status_state;
    QString status_time;

    // Usercomment fields (passed in my memcheck scripts)
    QString comment_hostname;
    QString comment_date;
    QString comment_rank;

    // QXmlDefaultHandler expects errorString() to return something,
    // we are returning error_message;
    QString error_message;

    // For printing out parse error messages, the XML text being parsed
    QString xml_text;

    // For printing out parse error message, the lineNumberOffset
    int lineOffset;

    // For printing out warning messages, guess at lineNo
    int lineNoGuess;
};

bool MCXMLParser::startElement( const QString&, const QString&, 
				const QString& elementName,
				const QXmlAttributes& )
{
    // Ignore "valgrindoutput" and "valgrindoutput_snippet" at level -1
    // (before any real token) in order to simplify parsing logic 
    // (i.e., error always happens at level 0, 
    // indepent of the presence of "<valgrindoutput>" or 
    // "<valgrindoutput_snippet>".
    if ((nestLevel == -1) &&
	((elementName == "valgrindoutput") ||
	 (elementName == "valgrindoutput_snippet")))
    {
	// No error, continue parsing
	return (TRUE);
    }
    
    // Increment level before processing startElement
    ++nestLevel;
    
    // Sanity check, nestLevel better be bounded
    if ((nestLevel < 0) || (nestLevel > 100))
    {
	fprintf (stderr, "Error parsing XML: "
		 "nestLevel (%i) out of bounds (0-100)!\n", nestLevel);
	exit (1);
    }
    
    // Get token based on elementName and nestLevel
    // If name known but not for nestLevel, XML_known will be returned
    XMLElementToken elementToken = getToken (elementName, nestLevel);
    
    // Do any required special handling of level 0 tokens
    if (nestLevel == 0)
    {
	if (elementToken == XML_error)
	{
	    // Clear error level 0 parameters
	    error_unique = "";
	    error_tid = NULL_INT;
	    error_kind = "";
	    error_what = "";
	    error_what_use_count = 0;
	    error_auxwhat = "";
	    error_auxwhat_use_count = 0;
	    error_xml_traceback.clear();

	    // Clear the stack count
	    error_stack_count = 0;

	    // Clear the string holding the "root" location for the error
	    error_location = "";
	}

	else if (elementToken == XML_protocolversion)
	{
	    // Clear protocol specifier
	    protocolVersion = NULL_INT;
	}

	else if (elementToken == XML_preamble)
	{
	    // Clear preamble text
	    preamble = "";
	}
	
	else if (elementToken == XML_args)
	{
	    // Clear valgrind name/args and application name/args
	    valgrind_exe = "";
	    valgrind_args = "";
	    app_exe = "";
	    app_args = "";
	}
	else if (elementToken == XML_status)
	{
	    status_state = "";
	    status_time = "";
	}
	else if (elementToken == XML_errorcounts)
	{
	    // Using numMessages, create array for holding message count
	    // info.   At /errorcounts, we will sort this array and list them
	    // Don't do anything if no messages
	    if (mcinfo->numMessages > 0)
	    {
		// Allocate array of ErrorInfo structures
		mcinfo->errorArray = new ErrorInfo[mcinfo->numMessages];
		if (mcinfo->errorArray == NULL)
		{
		    fprintf (stderr, 
			     "Error: out of memory allocating error array "
			     "of size %i\n",
			     mcinfo->numMessages);
		    exit (1);
		}
	    }

	    // Have nothing in the array so far
	    mcinfo->errorArrayCount = 0;
	}
	else if (elementToken == XML_usercomment)
	{
	    comment_hostname="";
	    comment_date="";
	    comment_rank="";
	}

    }
    
    // Do any required special handling of level 1 tokens
    else if (nestLevel == 1)
    {
	if (elementTokenAt[0] == XML_error)
	{
	    if (elementToken == XML_stack)
	    {
		// Clear the message buffer holding the TG XML 
		// traceback for this stack (and the text version)
		xml_tbbuf.clear();
		text_tbbuf.clear();

		// Clear the frame count
		error_frame_count = 0;

		// Increase the error stack count
		error_stack_count++;
	    }
	}

	else if (elementTokenAt[0] == XML_errorcounts)
	{
	    if (elementToken == XML_pair)
	    {
		// Clear errorcount pair parameters
		pair_count = NULL_INT;
		pair_unique = "";
	    }
	}

	else if (elementTokenAt[0] == XML_suppcounts)
	{
	    if (elementToken == XML_pair)
	    {
		// Clear suppcount pair parameters
		pair_count = NULL_INT;
		pair_name = "";
	    }
	}
    }
    
    
    // Do any required special handling of level 2 tokens
    else if (nestLevel == 2)
    {
	if (elementTokenAt[0] == XML_error)
	{
	    if (elementTokenAt[1] == XML_stack)
	    {
		if (elementToken == XML_frame)
		{
		    // Clear error level 2 parameters
		    error_frame_ip = "";
		    error_frame_obj = "";
		    error_frame_fn = "";
		    error_frame_dir = "";
		    error_frame_file = "";
		    error_frame_line = "";
		}
	    }
	}
    }
    
    // For now (aganst XML standard) warn about unknown element names
    if (elementToken == XML_unknown)
    {
	// Want to warn only once per unknown element name per level
	bool printWarning = TRUE;
	
	// Set the appropriate bit to test/set for this nestLevel
	int levelBit;
	
	// Use a bit for each level below 31.  Treat all levels
	// above 31 the same (don't expect that deep but who knows)
	if (nestLevel < 31)
	    levelBit= (1 << nestLevel);
	else
	    levelBit=(1 << 31);
	
	// Have we seen this name before?
	if (mcinfo->unknownXMLTable.entryExists(elementName))
	{
	    // Yes, get level mask for this entry
	    int *levelMask =
		mcinfo->unknownXMLTable.findEntry(elementName);
	    
	    // Is the bit already set?
	    if ((*levelMask) & levelBit)
	    {
		// Yes, suppress warning
		printWarning=FALSE;
	    }
	    else
	    {
		// No, set bit and print warning
		*levelMask |= levelBit;
	    }
	}
	else
	{
	    // No, add to unknownXMLTable
	    int *levelMask = new int (levelBit);
	    mcinfo->unknownXMLTable.addEntry (elementName, levelMask);
	}
	
	// Print warning if haven't printed warning before for this
	// particular elementName and nestLevel
	if (printWarning)
	{
	    fprintf (stderr, 
		     "Warning: Tool Gear Valgrind parser ignoring "
		     "unknown XML elements"
		     " '%s' at level %i\n"
		     "  '%s' first encountered on line %i:\n"
		     "  ",
		     (const char *)elementName, nestLevel,
		     (const char *)elementName,
		     lineNoGuess+lineOffset);
	    
	    // Print out line that caused error
	    printErrorContext (lineNoGuess, -1);
	    fprintf (stderr, "\n");
	}
    }	    
    
    
    // For now, do minimal processing above level 10
    // This prevents overflowing our parsing arrays 
    // A true XML parser should just ignore unexpected data but
    // I want warnings for now (which is done above).
    if (nestLevel >= 10)
    {
	// No error, continue parsing
	return (TRUE);
    }
    
    // Save element token at this level
    elementTokenAt[nestLevel] = elementToken;
    
    // Save element name in case we need it
    elementNameAt[nestLevel] = elementName;
    
    // Clear current value saved at this level
    valueAt[nestLevel] = "";
    
#if 0
    // DEBUG
    fprintf (stderr, "Level %i for start '%s'\n", nestLevel,
	     (const char *) elementName);
#endif
    
    // No error, continue parsing
    return (TRUE);
}

// This function is used by qsort to sort error messages by error count
// with the tiebreaker already factored in (in sortValue)
static int compareErrorsByCount (const void *err1ptr, const void *err2ptr)
{
    const ErrorInfo *ei1 = (const ErrorInfo *)err1ptr;
    const ErrorInfo *ei2 = (const ErrorInfo *)err2ptr;
    if (ei2->sortValue > ei1->sortValue)
	return (1);
    else
	return (-1);
}

bool MCXMLParser::endElement( const QString&, const QString&, 
 			      const QString& elementName)
{
    // Ignore "valgrindoutput" and "valgrindoutput_snippet" at level -1
    // (after any real token) in order to simplify parsing logic 
    // (i.e., message_folder always happens at level 0, 
    // indepent of the presence of "<valgrindoutput>" or 
    // "<valgrindoutput_XML_snippet>".
    if ((nestLevel == -1) && (elementName == "valgrindoutput_snippet"))
    {
	// No error, continue parsing
	return (TRUE);
    }

    if ((elementName == "valgrindoutput"))
    {
	// DEBUG
	fprintf (stderr, "Saw end valgrindoutput!\n");
	exit (1);
	// No error, continue parsing
	return (TRUE);
    }
    
    // DEBUG
//    fprintf (stderr, "Level %i %s: '%s'\n", nestLevel,
//	     (const char *) elementName, (const char *)valueAt[nestLevel]);
    
    // For sanity check, record if endElement actually processed
    bool elementHandled = FALSE;
    
    // Process XML at level 0 
    if (nestLevel == 0)
    {
	// After processing all error contents, emit message in TG XML
	if (elementTokenAt[0] == XML_error)
	{
	    // Handle case where auxwhat doesn't have any traceback,
	    // print out info without traceback
	    if ((error_auxwhat != "") &&
		(error_auxwhat_use_count < 1))
	    {
		// Append auxwhat text to message
		error_text_traceback.appendSprintf("%s\n",
						   (const char *)error_auxwhat);

		// Create a title with not traceback with auxwhat info
		error_xml_traceback.appendSprintf("  <annot>\n"
						  "    <title>%s</title>\n"
						  "  </annot>\n",
						  (const char *)error_auxwhat);
	    }
	    

	    // If we encounter an error_kind that we haven't see before,
	    // create a generic folder for it.
	    if (!folderTagTable.entryExists(error_kind))
	    {
		// Create generic title from error_kind label
		QString genericTitle;
		genericTitle.sprintf ("%s messages", error_kind.latin1());

		// Emit xml to create folder
		declareFolder (xml_out, error_kind.latin1(), 
			       genericTitle.latin1(),
			       "disable");

		// Warn user that we are seeing error_kind we haven't
		// seen before
		fprintf (stderr, 
			 "Warning: Unknown '%s' error type, using generic "
			 "message folder title.\n", 
			 error_kind.latin1());
	    }

	    // Increment message count
	    mcinfo->numMessages++;

	    // Emit the TG xml message targetting two folders, the main 
	    // folder and the one specific to the error_kind.
	    fprintf (xml_out, 
		     "\n"
		     "<message>\n"
		     "  <folder>all_in_order</folder>\n"
		     "  <folder>%s</folder>\n"
		     "  <heading>%-67s  %s</heading>\n"
		     "  <body>%s</body>\n"
		     "%s" // Implict newline at end of xml traceback
		     "</message>\n",
		     (const char *)error_kind,
		     (const char *)error_what,
		     (const char *)error_location,
		     error_text_traceback.contents(),
		     error_xml_traceback.contents());
	    fflush (xml_out);


	    // Create template for outputing message with count annotations
	    // at end when message counts are known
	    QString countXML;
	    countXML.sprintf ("\n"
		     "<message>\n"
		     "  <folder>all_in_order</folder>\n"
		     "  <folder>all_by_count</folder>\n"
		     "  <heading>%%5i - %-67s  %s</heading>\n"
		     "  <body>%s</body>\n"
		     "%s" // Implict newline at end of xml traceback
		     "</message>\n",
		     (const char *)error_what,
		     (const char *)error_location,
		     error_text_traceback.contents(),
		     error_xml_traceback.contents());


	    // Add message XML to table, indexed by error_unique value
	    mcinfo->messageTable.addEntry(error_unique.latin1(),
					  strdup(countXML.latin1()));
	    
	    elementHandled = TRUE; // Mark element handled
	}

	else if (elementTokenAt[0] == XML_protocolversion)
	{
	    // Get protocal version (an INT)
	    protocolVersion = xmlConvertToInt();

	    // Have tested with protocols 1 and 2
	    if ((protocolVersion < 1) || (protocolVersion > 2))
	    {
		fprintf (stderr,
			 "\n"
			 "Warning: Valgrind protocal version %i has not been "
			 "tested with this reader!\n\n", 
			 protocolVersion);
	    }

	    // Make sure we know
	    elementHandled = TRUE; // Mark element handled
	}
	else if (elementTokenAt[0] == XML_preamble)
	{
	    // Append preamble to about text (put after GUI about info)
	    fprintf (xml_out,
		     "<about>\n"
		     "  <append>%s</append>\n"
		     "</about>\n"
		     "\n", preamble.latin1());

	    // Also add copy right notice to Memcheck status messages
	    fprintf (xml_out,
		     "<message>\n"
		     "  <folder>memcheck_invocation</folder>\n"
		     "  <heading>Memcheck Copyright Notice</heading>\n"
		     "  <body>%s</body>\n"
		     "</message>\n"
		     "\n", preamble.latin1());
	    fflush (xml_out);

	    // Add copywrite text to prolog message
	    mcinfo->prologText.append(preamble);

	    elementHandled = TRUE; // Mark element handled
	}
	else if (elementTokenAt[0] == XML_pid)
	{
	    // Get the pid in string form
	    // Print out info about the ppid
	    fprintf (xml_out,
		     "<message>\n"
		     "  <folder>memcheck_invocation</folder>\n"
		     "  <heading>pid %s</heading>\n"
		     "</message>\n"
		     "\n", valueAt[0].latin1());
	    fflush(xml_out);

	    // Add pid text to prolog message
	    QString pidText;
	    pidText.sprintf ("\npid %s\n", valueAt[0].latin1());
	    mcinfo->prologText.append(pidText);

	    elementHandled = TRUE; // Mark element handled
	}
	else if (elementTokenAt[0] == XML_ppid)
	{
	    // Print out info about the ppid
	    fprintf (xml_out,
		     "<message>\n"
		     "  <folder>memcheck_invocation</folder>\n"
		     "  <heading>ppid %s</heading>\n"
		     "</message>\n"
		     "\n", valueAt[0].latin1());
	    fflush(xml_out);
	    
	    // Add ppid text to prolog message
	    QString ppidText;
	    ppidText.sprintf ("ppid %s\n", valueAt[0].latin1());
	    mcinfo->prologText.append(ppidText);

	    elementHandled = TRUE; // Mark element handled
	}
	else if (elementTokenAt[0] == XML_tool)
	{
	    // Make sure value expected
	    if (valueAt[0] != "memcheck")
	    {
		// Print out context
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, 
			 "Warning: expected tool 'memcheck' not '%s', "
			 "may not work...\n", valueAt[0].latin1());
	    }

	    elementHandled = TRUE; // Mark element handled
	}
	else if (elementTokenAt[0] == XML_args)
	{
	    // Print out prolog message now, at end of args.
	    fprintf (xml_out,
		     "<message>\n"
		     "  <folder>all_in_order</folder>\n"
//		     "  <heading>Valgrind's Memcheck Tool Preamble Text</heading>\n"
		     "  <heading>Valgrind's Memcheck Tool Copyright and Invocation Information</heading>\n"
		     "  <body>%s</body>\n"
		     "  <annot>\n"
		     "    <title>Application's Invocation Info</title>\n"
		     "    <site>\n"
		     "      <desc>%s</desc>\n"
		     "    </site>\n"
		     "  </annot>\n"
		     "  <annot>\n"
		     "    <title>Valgrind's Invocation Info</title>\n"
		     "    <site>\n"
		     "      <desc>%s</desc>\n"
		     "    </site>\n"
		     "  </annot>\n"
		     "</message>\n"
		     "\n", mcinfo->prologText.latin1(),
		     mcinfo->appInvoke.latin1(),
		     mcinfo->valgrindInvoke.latin1());
	    fflush (xml_out);

	    // Clear the prologText, so can detect if more added
	    mcinfo->prologText="";

	    // All the rest done in vargv and argv handler
	    elementHandled = TRUE; // Mark element handled
	}
	else if (elementTokenAt[0] == XML_status)
	{
	    // Print out status info
	    fprintf (xml_out,
		     "<message>\n"
		     "  <folder>all_in_order</folder>\n"
		     "  <folder>memcheck_status</folder>\n"
		     "  <heading>  --- Status: %-9s %s ---</heading>\n"
		     "  <annot>\n"
		     "    <title>Status</title>\n"
		     "    <site><desc>%-9s %s</desc></site>\n"
		     "  </annot>\n"
		     "</message>\n"
		     "\n", 
		     status_state.latin1(), 
		     status_time.latin1(),
		     status_state.latin1(), 
		     status_time.latin1());
	    fflush (xml_out);
	    
	    elementHandled = TRUE; // Mark element handled
	}
	else if (elementTokenAt[0] == XML_errorcounts)
	{
	    // Sort the error array by sortValue (based on count, 
	    // with a tie-breaker that attempts to break ties by occurance
	    // order.
	    qsort ( (void *)mcinfo->errorArray, mcinfo->errorArrayCount,
		    sizeof(ErrorInfo), compareErrorsByCount );

	    // If have error counts, print out header to combined output
	    if (mcinfo->errorArrayCount > 0)
	    {
		// Count the total number of messages
		int totalErrors = 0;
		for (int index=0; index < mcinfo->errorArrayCount; ++index)
		{
		    totalErrors += mcinfo->errorArray[index].count;
		}
		
		fprintf (xml_out,
			 "<message>\n"
			 "  <folder>all_in_order</folder>\n"
			 "  <folder>memcheck_status</folder>\n"
			 "  <heading>  --- ERROR SUMMARY: %i errors from %i contexts ---</heading>\n"
			 "  <annot>\n"
			 "    <title>Error Summary</title>\n"
			 "    <site><desc>%i errors from %i contexts</desc></site>\n"
			 "  </annot>\n"
			 "</message>\n"
			 "\n", 
			 totalErrors, mcinfo->errorArrayCount,
			 totalErrors, mcinfo->errorArrayCount);
		fflush (xml_out);
	    }

	    // Print out error array to XML file in sorted order
	    for (int index=0; index < mcinfo->errorArrayCount; ++index)
	    {
//		printf ("Writing %i: sortValue %g\n",
//			index, mcinfo->errorArray[index].sortValue);
		fprintf (xml_out, mcinfo->errorArray[index].msg, 
			 mcinfo->errorArray[index].count);
	    }
	    fflush (xml_out);

	    // Free it, if it exists
	    if (mcinfo->errorArray != NULL)
	    {
		delete[] mcinfo->errorArray;
		mcinfo->errorArray = NULL;
	    }


	    // Everything done in pair handler
	    elementHandled = TRUE; // Mark element handled
	}
	else if (elementTokenAt[0] == XML_suppcounts)
	{
	    if (!mcinfo->suppText.isEmpty())
	    {
		fprintf (xml_out,
			 "<message>\n"
			 "  <folder>all_in_order</folder>\n"
			 "  <folder>all_by_count</folder>\n"
			 "  <heading>Suppressed error message statistics (messages typically safely ignored)</heading>\n"
			 "  <body>%s</body>\n"
			 "</message>\n"
			 "\n", 
			 mcinfo->suppText.latin1());
		fflush(xml_out);
	    }

	    // Everything done in pair handler
	    elementHandled = TRUE; // Mark element handled
	}

	else if (elementTokenAt[0] == XML_usercomment)
	{
	    // Print out status info, if user comment specified
	    if ((!comment_date.isEmpty()) ||
		(!comment_hostname.isEmpty()) ||
		(!comment_rank.isEmpty()))
	    {
		fprintf (xml_out,
			 "<message>\n"
			 "  <folder>all_in_order</folder>\n"
			 "  <folder>memcheck_status</folder>\n"
			 "  <heading>  --- Started: %s%s%s ---</heading>\n"
			 "  <annot>\n"
			 "    <title>Started</title>\n"
			 "    <site><desc>%s%s%s</desc></site>\n"
			 "  </annot>\n"
			 "</message>\n"
			 "\n", 
			 comment_date.latin1(),
			 comment_hostname.latin1(),
			 comment_rank.latin1(),
			 comment_date.latin1(),
			 comment_hostname.latin1(),
			 comment_rank.latin1());
		fflush (xml_out);
	    }

	    elementHandled = TRUE; // Mark element handled
	}


	else if (elementTokenAt[0] == XML_FATAL)
	{
	    // Emit the TG xml message targetting two folders, the main 
	    // folder and the one specific to the error_kind.
	    fprintf (xml_out, 
		     "\n"
		     "<message>\n"
		     "  <folder>all_in_order</folder>\n"
		     "  <folder>memcheck_status</folder>\n"
		     "  <heading>   --- Fatal XML parse error.  Valgrind Memcheck output parsing aborted ---   </heading>\n"
		     "  <body>Unexpected Valgrind non-XML output as follows (typically Valgrind error message):\n"
		     "%s</body>\n"
		     "</message>\n",
		     valueAt[0].latin1());
	    fflush (xml_out);
	    elementHandled = TRUE; // Mark element handled
	}
    }
    
    // Process XML at level 1
    else if (nestLevel == 1)
    {
	// Handle error level 1 values
	if (elementTokenAt[0] == XML_error)
	{
	    // Store error->unique value in variable
	    if (elementTokenAt[1] == XML_unique)
	    {
		error_unique = valueAt[1];
		elementHandled = TRUE; // Mark element handled
	    }

	    // Store error->tid value in variable
	    else if (elementTokenAt[1] == XML_tid)
	    {
		error_tid = xmlConvertToInt();
		elementHandled = TRUE; // Mark element handled
	    }
	    // Store error->kind value in variable
	    else if (elementTokenAt[1] == XML_kind)
	    {
		error_kind = valueAt[1];
		elementHandled = TRUE; // Mark element handled
	    }
	    // Store error->what value in variable
	    else if (elementTokenAt[1] == XML_what)
	    {
		error_what = valueAt[1];
		error_what_use_count = 0;  // Info lost if not used once
		elementHandled = TRUE; // Mark element handled
	    }
	    
	    // Store error->auxwhat value in variable
	    else if (elementTokenAt[1] == XML_auxwhat)
	    {
		// Handle case where auxwhat doesn't have any traceback,
		// print out info without traceback
		if ((error_auxwhat != "") &&
		    (error_auxwhat_use_count < 1))
		{
		    // Append auxwhat text to message
		    error_text_traceback.appendSprintf("%s\n",
						       (const char *)error_auxwhat);

		    // Create a title with not traceback with auxwhat info
		    error_xml_traceback.appendSprintf("  <annot>\n"
						      "    <title>%s</title>\n"
						      "  </annot>\n",
						      (const char *)error_auxwhat);
		}

		error_auxwhat = valueAt[1];
		error_auxwhat_use_count = 0; // Info lost if not used once
		elementHandled = TRUE; // Mark element handled
	    }

	    // Currently don't use leakedbytes, so ignore
	    else if (elementTokenAt[1] == XML_leakedbytes)
	    {
		elementHandled = TRUE; // Mark element handled
	    }

	    // Currently don't use leakedblocks, so ignore
	    else if (elementTokenAt[1] == XML_leakedblocks)
	    {
		elementHandled = TRUE; // Mark element handled
	    }
	    
	    // Handle error->stack closing by appending TG XML annot snippet 
	    // from the built up stack traceback in xml_tbbuf
	    else if (elementTokenAt[1] == XML_stack)
	    {
		// Pick title based on what and auxwhat contents
		QString title;

		// Use most recent auxwhat if present (specifies what the stack
		// just processed is for)
		if (error_auxwhat != "")
		{
		    title = error_auxwhat;
		    error_auxwhat_use_count++; // Mark used auxwhat
		    error_text_traceback.appendSprintf("%s\n"
						       "%s",
						       error_auxwhat.latin1(),
						       text_tbbuf.contents());
		}

		// Otherwise, heading will title contents, so don't put title
		else if (error_what != "")
		{
		    title=error_what;
		    error_what_use_count++;  // Mark used what
		    error_text_traceback.appendSprintf("%s",
						       text_tbbuf.contents());
		}
		// Valgrind standard requires <what> before <stack>,
		// so punt here if that doesn't happen.
		else
		{
		    // Print out line that caused error
		    printErrorContext (lineNoGuess, -1);
		    fprintf (stderr, "\n");
		    
		    TG_error ("Invalid Memcheck XML: "
			      "<what> not specified before <stack>!");
		}

		   
		error_xml_traceback.appendSprintf("  <annot>\n"
						  "    <title>%s</title>\n"
						  "%s"
						  "  </annot>\n",
						  (const char *)title,
						  xml_tbbuf.contents());


#if 0						   
		// DEBUG
		printf ("\nXML traceback now (stacks %i, frames %i):\n%s\n", 
			error_stack_count, error_frame_count,
			error_xml_traceback.contents());
#endif
#if 0
		printf ("\nTEXT traceback now (root %s):\n%s\n", 
			(const char *)error_location,
			error_text_traceback.contents());
#endif
		
		elementHandled = TRUE; // Mark element handled
	    }
	}      
	else if ((elementTokenAt[0] == XML_preamble) &&
		 (elementTokenAt[1] == XML_line))
	{
	    // Append line to preamble
	    preamble.append(valueAt[1]);

	    // Append newline after line
	    preamble.append('\n');
	    
	    elementHandled = TRUE; // Mark element handled
	}
	else if ((elementTokenAt[0] == XML_args) &&
		 (elementTokenAt[1] == XML_vargv))
	{
	    // Make sure have valgrind executable and app executable set
	    if (valgrind_exe == "")
	    {
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, 
			 "Warning: expected valgrind executable to be "
			 "specified in Valgrind's XML");
	    }

	    // Print out how valgrind was started
	    fprintf (xml_out,
		     "<message>\n"
		     "  <folder>memcheck_invocation</folder>\n"
		     "  <heading>Valgrind's Invocation Info</heading>\n"
		     "  <body>%s %s</body>\n"
		     "  <annot>\n"
		     "    <title>Valgrind's Invocation Info</title>\n"
		     "    <site>\n"
		     "      <desc>%s</desc>\n"
		     "    </site>\n"
		     "  </annot>\n"
		     "</message>\n"
		     "\n", 
		     valgrind_exe.latin1(), valgrind_args.latin1(),
		     mcinfo->valgrindInvoke.latin1());

	    fflush(xml_out);

	    // Add invocation info to prolog text
	    mcinfo->prologText.append("\nValgrind's Invocation Info:\n");
	    mcinfo->prologText.append(valgrind_exe);
	    mcinfo->prologText.append(valgrind_args);

	    elementHandled = TRUE; // Mark element handled
	}
	else if ((elementTokenAt[0] == XML_args) &&
		 (elementTokenAt[1] == XML_argv))
	{
	    if (app_exe == "")
	    {
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, 
			 "Warning: expected application executable name to be "
			 "specified in Valgrind's XML");
	    }

	    // Handle no argument case
	    if (app_args == "")
		app_args = "\n  (no arguments)";

	    // Print out how the application was started
	    fprintf (xml_out,
		     "<message>\n"
		     "  <folder>memcheck_invocation</folder>\n"
		     "  <heading>Application's Invocation Info</heading>\n"
		     "  <body>%s %s</body>\n"
		     "  <annot>\n"
		     "    <title>Application's Invocation Info</title>\n"
		     "    <site>\n"
		     "      <desc>%s</desc>\n"
		     "    </site>\n"
		     "  </annot>\n"
		     "</message>\n"
		     "\n", app_exe.latin1(), app_args.latin1(),
		     mcinfo->appInvoke.latin1());
	    fflush(xml_out);


	    // Add invocation info to prolog text
	    mcinfo->prologText.append("\n\nApplication's Invocation Info:\n");
	    mcinfo->prologText.append(app_exe);
	    mcinfo->prologText.append(app_args);

	    elementHandled = TRUE; // Mark element handled
	}
	else if ((elementTokenAt[0] == XML_status) &&
		 (elementTokenAt[1] == XML_state))
	{
	    status_state = valueAt[1];
	    elementHandled = TRUE; // Mark element handled
	}
	else if ((elementTokenAt[0] == XML_status) &&
		 (elementTokenAt[1] == XML_time))
	{
	    status_time = valueAt[1];
	    elementHandled = TRUE; // Mark element handled
	}
	else if ((elementTokenAt[0] == XML_errorcounts) &&
		 (elementTokenAt[1] == XML_pair))
	{
	    const char *xml_message;

	    // Get message based on unique key
	    xml_message = mcinfo->messageTable.findEntry(pair_unique.latin1());

	    // Print warning if not found
	    if (xml_message == NULL)
	    {
		// Print out context
		printErrorContext (lineNoGuess, -1);
		fprintf (stderr, 
			 "Warning: Could not find message for unique "
			 "value '%s'! Ignoring count info!\n",
			 pair_unique.latin1());
	    }

	    // Otherwise, write message to errorArray for sorting 
	    // using pair_count
	    else
	    {
		int index = mcinfo->errorArrayCount;

		// Sanity check
		if (index > mcinfo->numMessages)
		{
		    fprintf (stderr, "Error: errorArrayCount (%i) > numMessages (%i)!\n",
			     mcinfo->errorArrayCount, mcinfo->numMessages);
		    exit (1);
		}
		// Sanity check
		if (mcinfo->errorArray == NULL)
		{
		    fprintf (stderr, "Error: errorArray is NULL!\n");
		    exit (1);
		}
		

		// For now, assume errorcounts are printed in reverse order,
		// so want to give the biggest fractional number to the last
		// pair
		mcinfo->errorArray[index].sortValue = 
		    ((double)pair_count) + 
		    ((double)index/((double)(mcinfo->numMessages+1)));
		mcinfo->errorArray[index].count = pair_count;
		mcinfo->errorArray[index].msg = xml_message;

		// Update errorArrayCount
		mcinfo->errorArrayCount++;
	    }

	    elementHandled = TRUE; // Mark element handled
	}
	else if ((elementTokenAt[0] == XML_suppcounts) &&
		 (elementTokenAt[1] == XML_pair))
	{
	    // Print out supplimental count line to temp buffer
	    QString suppCountText;
	    suppCountText.sprintf("%-9i %s\n",
				  pair_count, pair_name.latin1());

	    // Append to suppText, printed out at end
	    mcinfo->suppText.append(suppCountText);
	    
	    elementHandled = TRUE; // Mark element handled
	}
	else if (elementTokenAt[0] == XML_usercomment)
	{
	    if (elementTokenAt[1] == XML_date)
	    {
		comment_date=valueAt[1];
		elementHandled = TRUE; // Mark element handled
	    }
	    else if (elementTokenAt[1] == XML_hostname)
	    {
		comment_hostname.sprintf(" on %s",valueAt[1].latin1()); 
		elementHandled = TRUE; // Mark element handled
	    }
	    else if (elementTokenAt[1] == XML_rank)
	    {
		comment_rank.sprintf(" (MPI Task %s)",valueAt[1].latin1()); 
		elementHandled = TRUE; // Mark element handled
	    }
	}
    }
    
    // Process XML at level 2
    else if (nestLevel == 2)
    {
	// Handle error->stack->frame closing by appending TG XML traceback
	// snippet based on filled in frame data
	if ((elementTokenAt[0] == XML_error) &&
	    (elementTokenAt[1] == XML_stack) &&
	    (elementTokenAt[2] == XML_frame))
	{
	    // Create full path to file name (but don't add ./)
	    QString full_file_path = error_frame_file;
	    if ((error_frame_file != "") && (error_frame_dir != "") &&
		(error_frame_dir != "."))
	    {
		full_file_path.sprintf ("%s/%s", 
				  (const char *)error_frame_dir,
				  (const char *)error_frame_file);
	    }

	    // Increment frame count
	    error_frame_count++;


	    //
	    // Create path message for both text traceback and for the
	    // message title
	    //
	    path_buf.clear();

	    if (error_frame_fn != "")
	    {
		path_buf.appendSprintf (" %s",
					  (const char *)error_frame_fn);
	    }

	    // Description first choice, file:line
	    if ((error_frame_file != "") && (error_frame_line != ""))
	    {
		path_buf.appendSprintf (" (%s:%s)",
					  (const char *)error_frame_file,
					  (const char *)error_frame_line);
	    }

	    // Second choice, file only (with full path)
	    else if (full_file_path != "")
	    {
		path_buf.appendSprintf (" (in %s)",
					  (const char *)full_file_path);
	    }

	    // Third choice, object lives in
	    else if (error_frame_obj != "")
	    {
		path_buf.appendSprintf (" (in %s)",
					  (const char *)error_frame_obj);
	    }

	    // Print warning if don't have good info
	    else
	    {
		path_buf.appendSprintf (" (no location info available)");
	    }


	    //
	    // Create text traceback for message body
	    //

	    // Start Text version of frame info with 'at' for 1st frame and 
	    // 'by' for all other frames (and the ip addr)
	    if (error_frame_count == 1)
	    {
		text_tbbuf.appendSprintf ("  at %s:%s\n", 
					  (const char *)error_frame_ip,
					  path_buf.contents());
	    }
	    else
	    {
		text_tbbuf.appendSprintf ("  by %s:%s\n",
					  (const char *)error_frame_ip,
					  path_buf.contents());
	    }


	    //
	    // Create "root" location for error for message title
	    //
	    if ((error_location == "") &&
		(error_stack_count == 1) &&
		(error_frame_file != "vg_replace_malloc.c") &&
		(error_frame_file != "libmpiwrap.c"))
	    {
		// Remove leading space, if exists (easier to remove at end
		// than to put logic in not to have space)
		const char *buf_ptr = path_buf.contents();
		if (*buf_ptr == ' ')
		    ++buf_ptr;
		error_location = buf_ptr;
	    }

	    	    
	    //
	    // Create XML section for traceback info
	    //
	    xml_tbbuf.appendSprintf("    <site>\n");

	    // Only add file info, if present
	    if (full_file_path != "")
	    {
		// Print out the full file name path and end tag
		xml_tbbuf.appendSprintf("      <file>%s</file>\n",
					(const char *)full_file_path);
	    }
	    
	    // Add line info, if present
	    if (error_frame_line != "")
	    {
		xml_tbbuf.appendSprintf("      <line>%s</line>\n",
					(const char *)error_frame_line);
	    }
	    
	    // Create description from available info
	    if (error_frame_fn != "")
	    {
		xml_tbbuf.appendSprintf("      <desc>%s</desc>\n",
					(const char *)error_frame_fn);
	    }
	    
	    // Otherwise, do our best the with remaining info
	    else
	    {
		xml_tbbuf.appendSprintf("      <desc>@%s in %s</desc>\n",
					(const char *)error_frame_ip,
					(const char *)error_frame_obj);
	    }
	    
	    xml_tbbuf.appendSprintf("    </site>\n");

	    elementHandled = TRUE; // Mark element handled
	}

	else if ((elementTokenAt[0] == XML_args) &&
		 (elementTokenAt[1] == XML_vargv))
	{
	    if (elementTokenAt[2] == XML_exe)
	    {
		valgrind_exe = valueAt[2];

		// Put at beginning of valgrindInvoke also
		mcinfo->valgrindInvoke = valgrind_exe;
		mcinfo->valgrindInvokeLen = valgrind_exe.length();

		elementHandled = TRUE; // Mark element handled
	    }
	    else if(elementTokenAt[2] == XML_arg)
	    {
		valgrind_args.append("\n  ");
		valgrind_args.append(valueAt[2]);

		// Also save for prolog annotations, without newlines
		int argLen = valueAt[2].length();
		if (mcinfo->valgrindInvokeLen > 0)
		{
		    if ((mcinfo->valgrindInvokeLen + argLen + 1) > 120)
		    {
			// Append the XML to finish current site and '\'
			// and start another site
			mcinfo->valgrindInvoke.append(" \\</desc>\n"
						"   </site>\n"
						"   <site>\n"
						"     <desc>");

			// Start a new line, arg length added below
			mcinfo->valgrindInvokeLen = 0;
		    }
		    else
		    {
			// Add space after previous argument on line
			mcinfo->valgrindInvoke.append(" ");

			// Increase the current line by this arg + space
			mcinfo->valgrindInvokeLen += 1;
		    }
		}

		mcinfo->valgrindInvoke.append(valueAt[2]);

		// Increase app length by length of valuAt[2]
		mcinfo->valgrindInvokeLen += argLen;

		elementHandled = TRUE; // Mark element handled	}
	    }
	}

	else if ((elementTokenAt[0] == XML_args) &&
		 (elementTokenAt[1] == XML_argv))
	{
	    if (elementTokenAt[2] == XML_exe)
	    {
		app_exe = valueAt[2];

		// Also save for prolog annotations
		mcinfo->appExe = valueAt[2];

		// Put at beginning of appInvoke also
		mcinfo->appInvoke =mcinfo->appExe;
		mcinfo->appInvokeLen = mcinfo->appExe.length();

		elementHandled = TRUE; // Mark element handled
	    }
	    else if (elementTokenAt[2] == XML_arg)
	    {
		app_args.append("\n  ");
		app_args.append(valueAt[2]);


		// Also save for prolog annotations, without newlines
		int argLen = valueAt[2].length();
		if (mcinfo->appInvokeLen > 0)
		{
		    if ((mcinfo->appInvokeLen + argLen + 1) > 120)
		    {
			// Append the XML to finish current site and '\'
			// and start another site
			mcinfo->appInvoke.append(" \\</desc>\n"
						"   </site>\n"
						"   <site>\n"
						"     <desc>");

			// Start a new line, arg length added below
			mcinfo->appInvokeLen = 0;
		    }
		    else
		    {
			// Add space after previous argument on line
			mcinfo->appInvoke.append(" ");

			// Increase the current line by this arg + space
			mcinfo->appInvokeLen += 1;
		    }
		}

		mcinfo->appInvoke.append(valueAt[2]);

		// Increase app length by length of valuAt[2]
		mcinfo->appInvokeLen += argLen;

		elementHandled = TRUE; // Mark element handled
	    }
	}

	else if ((elementTokenAt[0] == XML_errorcounts) &&
		 (elementTokenAt[1] == XML_pair))
	{
	    if (elementTokenAt[2] == XML_count)
	    {
		pair_count = xmlConvertToInt(1, NULL_INT);
		elementHandled = TRUE; // Mark element handled
	    }
	    if (elementTokenAt[2] == XML_unique)
	    {
		pair_unique = valueAt[2];
		elementHandled = TRUE; // Mark element handled
	    }
	}

	else if ((elementTokenAt[0] == XML_suppcounts) &&
		 (elementTokenAt[1] == XML_pair))
	{
	    if (elementTokenAt[2] == XML_count)
	    {
		pair_count = xmlConvertToInt(1, NULL_INT);
		elementHandled = TRUE; // Mark element handled
	    }
	    if (elementTokenAt[2] == XML_name)
	    {
		pair_name = valueAt[2];
		elementHandled = TRUE; // Mark element handled
	    }
	}
    }
    
    // Process XML at level 3
    else if (nestLevel == 3)
    {
	// Handle error->stack->frame->* value specifications
	if ((elementTokenAt[0] == XML_error) &&
	    (elementTokenAt[1] == XML_stack) &&
	    (elementTokenAt[2] == XML_frame))
	{
	    if (elementTokenAt[3] == XML_ip)
	    {
		error_frame_ip = valueAt[3];
		elementHandled = TRUE; // Mark element handled
	    }
	    
	    else if (elementTokenAt[3] == XML_obj)
	    {
		error_frame_obj = valueAt[3];
		elementHandled = TRUE; // Mark element handled
	    }
	    
	    else if (elementTokenAt[3] == XML_fn)
	    {
		error_frame_fn = valueAt[3];
		elementHandled = TRUE; // Mark element handled
	    }
	    
	    else if (elementTokenAt[3] == XML_dir)
	    {
		error_frame_dir = valueAt[3];
		elementHandled = TRUE; // Mark element handled
	    }
	    
	    else if (elementTokenAt[3] == XML_file)
	    {
		error_frame_file = valueAt[3];
		elementHandled = TRUE; // Mark element handled
	    }
	    
	    else if (elementTokenAt[3] == XML_line)
	    {
		error_frame_line = valueAt[3];
		elementHandled = TRUE; // Mark element handled
	    }
	}
    }
    
    // Print warning once for each unhandled known element
    // Use same suppression table for unknown XML tokens since
    // they should not intersect (since things not in the table
    // will be all XML_unknown).
    // To simplify, only print warnings for first 10 levels of XML,
    // since don't expect to have info deeper than level 3 for a while 
    if ((!elementHandled) && 
	(nestLevel >= 0) && (nestLevel < 10) &&
	(elementTokenAt[nestLevel] != XML_unknown))
    {
	// Get elementName for ease of use
	const char *elementName = elementNameAt[nestLevel];
	
	// Want to warn only once per unknown element name per level
	bool printWarning = TRUE;
	
	// Set the appropriate bit to test/set for this nestLevel
	int levelBit;
	
	// Use a bit for each level below 31.  Treat all levels
	// above 31 the same (don't expect that deep but who knows)
	if (nestLevel < 31)
	    levelBit= (1 << nestLevel);
	else
	    levelBit=(1 << 31);
	
	// Have we seen this name before?
	if (mcinfo->unknownXMLTable.entryExists(elementName))
	{
	    // Yes, get level mask for this entry
	    int *levelMask =
		mcinfo->unknownXMLTable.findEntry(elementName);
	    
	    // Is the bit already set?
	    if ((*levelMask) & levelBit)
	    {
		// Yes, suppress warning
		printWarning=FALSE;
	    }
	    else
	    {
		// No, set bit and print warning
		*levelMask |= levelBit;
	    }
	}
	else
	{
	    // No, add to unknownXMLTable
	    int *levelMask = new int (levelBit);
	    mcinfo->unknownXMLTable.addEntry (elementName, levelMask);
	}
	
	// Print warning if haven't printed warning before for this
	// particular elementName and nestLevel
	if (printWarning)
	{
	    fprintf (stderr, 
		     "Warning: Tool Gear Valgrind parser not "
		     "handling XML elements"
		     " '%s' at level %i\n"
		     "  '%s' first encountered on line %i:\n"
		     "  ",
		     (const char *)elementName, nestLevel,
		     (const char *)elementName,
		     lineNoGuess+lineOffset);
	    
	    // Print out line that caused error
	    printErrorContext (lineNoGuess, -1);
	    fprintf (stderr, "\n");
	}
	
    }
    
    // Finished with element, decrease nestLevel
    --nestLevel;
    
    // No error, continue parsing
    return (TRUE);
}



// Processes the XMLSnippet (first wrapping it with <valgrindoutput_snippet>
// ... </valgrindoutput_snippet> to make the XML parser happy with multiple
// snippets) and process the recognized commands (warning about those
// not recognized.   Initially XML can be used to execute
// declareMessageFolder(), addMessage(), and addAboutText() commands.
// lineNoOffset is used in error messages to relate line in snippet to
// line in source file.
void processXMLSnippet (FILE *xml_out, MemcheckInfo *mcinfo, 
			const char *XMLSnippet, int lineNoOffset)
{
    QString wrappedXMLSnippet;

    wrappedXMLSnippet.sprintf ("<valgrindoutput_snippet>\n%s\n</valgrindoutput_snippet>",
			       XMLSnippet);
#if 0
    // DEBUG
    fprintf (stderr, 
	     "Processing XML Snippet:\n"
	     "%s\n"
	     "-----------------------\n", (const char *) wrappedXMLSnippet);
#endif

    // Subtract 1 from offset since adding one line
    MCXMLParser handler (xml_out, mcinfo, wrappedXMLSnippet, lineNoOffset-1);
    QXmlInputSource source;
    source.setData(wrappedXMLSnippet);
    QXmlSimpleReader reader;
    reader.setContentHandler (&handler);
    reader.setErrorHandler (&handler);
    reader.parse(source);

}



// Thread-safe version of error string library for IBM
#ifdef THREAD_SAFE
#define USE_STRERROR_R 
#endif

// Function prototypes
void parse_input(MemcheckInfo *mcinfo, XMLSnippetParser &XMLParser, 
                 FILE *xml_out);

char *memcheck_file_name = NULL;
char *xml_filename = NULL;

int main (int argc, char *argv[])
{

    // Expect exactly two arguments,
    // the input mpiP filename and the output xml file name
    if( argc != 3 ) {
        fprintf( stderr, "Usage: %s input_memcheck_xml_filename output_tg_xml_filename\n",
                 argv[0] );
	fprintf (stderr, "       Setting output_tg_xml_filename to '-' specifies output to stdout\n");
        return -1;
    }
    
    memcheck_file_name = argv[1];
    xml_filename = argv[2];

    FILE *xml_out = NULL;

    // Get initial parent ppid so can tell if changed
    long initial_ppid = getppid();

    // If file_name is -, write to stdout
    if (strcmp(xml_filename, "-") == 0)
	xml_out = stdout;

    // Otherwise, open the file normally
    else
	xml_out = fopen (xml_filename, "w");

    if (xml_out == NULL)
    {
        TG_error ("Failed to open XML output file %s!", xml_filename );
	
    }

    // Create class to hold memcheck info during parsing
    MemcheckInfo  mcinfo;

    // To allow parser to write status messages, put xml_out into mcinfo
    mcinfo.xmlOut = xml_out;


    // Start the xml file, specify XML format and minimum Tool Gear
    // version that supports all the XML features used
    fprintf (xml_out, 
	     "<tool_gear><format>1</format><version>%4.2f</version>\n\n",
	     TG_VERSION);


    // Set the initial main tool title
    fprintf (xml_out,
	     "<tool_title>MemcheckView - %s</tool_title>\n"
	     "\n",
	     memcheck_file_name);

    // Set the initial tool status message
    fprintf (xml_out,
	     "<status>Reading in %s</status>\n"
	     "\n",
	     memcheck_file_name);
    fflush (xml_out);

    // Add about text for the GUI (memcheck's preamble added during parsing
    // to about text)
    fprintf (xml_out,
	     "<about>\n"
	     "  <prepend>Tool Gear %4.2f's Valgrind Memcheck View GUI\n"
	     "Written (using Tool Gear's XML interface) by John Gyllenhaal</prepend>\n"
	     "</about>\n"
	     "\n",
	     TG_VERSION);
    

    // Declare the folders in the order that they should be displayed
    // Most folders (marked hide) are not shown unless populated

    // Declare the main message folder
    declareFolder (xml_out, 
		   "all_in_order", 
		   "Combined memcheck output (in output order)", 
		   "show");

    // Declare all the possible message 'kinds', used to categorize messages
    declareFolder (xml_out, 
		   "InvalidFree", 
		   "  free/delete/delete[] on an invalid pointer", 
		   "hide");

    declareFolder (xml_out, 
		   "MismatchedFree", 
		   "  free/delete/delete[] does not match allocation function", 
		    "hide");

    declareFolder (xml_out, 
		   "InvalidWrite", 
		   "  Write of an invalid address", 
		   "hide");

    declareFolder (xml_out, 
		   "InvalidRead", 
		   "  Read of an invalid address", 
		   "hide");

    declareFolder (xml_out, 
		   "InvalidJump", 
		   "  Jump to an invalid address", 
		   "hide");

    declareFolder (xml_out, 
		   "ClientCheck", 
		   "  Application/library-specific messages (Client checks, usually from MPI wrappers)", 
		   "hide");

    declareFolder (xml_out, 
		   "UninitCondition", 
		   "  Conditional jump/move depends on undefined value", 
		   "hide");

    declareFolder (xml_out, 
		   "UninitValue", 
		   "  Other use of undefined value (primarily memory addresses)",
		   "hide");

    declareFolder (xml_out, 
		   "Overlap", 
		   "  Args overlap each other or are otherwise bogus", 
		   "hide");

    declareFolder (xml_out, 
		   "SyscallParam", 
		   "  System call params are undefined or point to undefed/unaddressible memory", 
		   "hide");

    declareFolder (xml_out, 
		   "InvalidMemPool", 
		   "  Invalid mem pool specified in client request", 
		   "hide");

    declareFolder (xml_out, 
		   "Leak_DefinitelyLost", 
		   "  Memory leak (the referenced blocks are definitely lost)", 
		   "hide");

    declareFolder (xml_out, 
		   "Leak_IndirectlyLost", 
		   "  Indirect memory Leak (all pointers to the referenced blocks are only in leaked blocks)", 
		   "hide");

    declareFolder (xml_out, 
		   "Leak_PossiblyLost", 
		   "  Possible memory leak (only interior pointers to referenced blocks were found)", 
		   "hide");

    declareFolder (xml_out, 
		   "Leak_StillReachable", 
		   "  Unfreed memory leak (pointers to un-freed blocks are still available)", 
		   "hide");

    // This folder is not populated unless valgrind ends successfully.
    declareFolder (xml_out, 
		   "all_by_count", 
		   "  Error Message counts (all error categories)", 
		   "hide");

    // Declare folder for information about how memcheck was run
    declareFolder (xml_out, 
		   "memcheck_status", 
		   "  Memcheck status", 
		   "show");

    declareFolder (xml_out, 
		   "memcheck_invocation", 
		   "  Valgrind's memcheck tool copyright and invocation information", 
		   "show");


    // Declare priority modifiers for valgrind files that we think are 
    // less important to show users by default

    // Deemphasize source of valgrind's replacement malloc
    fprintf (xml_out,
	     "<site_priority>\n"
	     "  <file>vg_replace_malloc.c$</file>"
	     "  <modifier>-1.0</modifier>"
	     "</site_priority>\n"
	     "\n");

    // Deemphasize source of valgrind's mpi wrapper library
    fprintf (xml_out,
	     "<site_priority>\n"
	     "  <file>libmpiwrap.c$</file>"
	     "  <modifier>-1.0</modifier>"
	     "</site_priority>\n"
	     "\n");

    // Deemphasize sites that don't have files specified
    fprintf (xml_out,
	     "<site_priority>\n"
	     "  <file>^$</file>"
	     "  <modifier>-10.0</modifier>"
	     "</site_priority>\n"
	     "\n");

    fflush(xml_out);
    
    // Read and process input data
    int fd = open( memcheck_file_name, O_RDONLY );
    if( fd < 0 ) 
    {
	fprintf( stderr, "Failed to open input file %s\n",
		 memcheck_file_name );
	exit( -1 );
    }

    // Create FILE version of fd
    FILE *in = fdopen (fd, "r");
    if ( in == NULL)
    {
	fprintf( stderr, "Failed to convert input file %s handle to FILE\n",
		 memcheck_file_name );
	exit( -1 );
    }

    // DEBUG, create XML parser
    XMLSnippetParser  XMLParser (in);

    parse_input (&mcinfo, XMLParser, xml_out);


    // If file not ended, indicate we are monitoring it for changes
    if (!mcinfo.xmlEnded)
    {
	fprintf (xml_out,
		 "<status>Monitoring %s for new messages</status>\n"
		 "\n",
		 memcheck_file_name);
	fflush (xml_out);
    }

    // For now, allow parser to go approximately 10 hours without 
    // seeing XML before giving up and exiting.   I say approximate
    // since nanosleep calls are not delaying as long as expected.
    while ((!mcinfo.xmlEnded) && (mcinfo.minutesIdle < 600))
    {
	// DEBUG 
//	fprintf (stderr, "Minutes idle %i\n", mcinfo.minutesIdle);

	// Spin for a minute before updating idle minutes
	for (int seconds = 0; (!mcinfo.xmlEnded) && (seconds < 60); seconds++)
	{
	    // Check for new messages 10 times a second
	    for (int tenths = 0; (!mcinfo.xmlEnded) && (tenths < 10); tenths++)
	    {
		// Sleep for .1 seconds (I hope)
		struct timespec mytimespec;
		mytimespec.tv_sec = 0;
		mytimespec.tv_nsec = 100000000;
		nanosleep (&mytimespec, NULL);
		parse_input (&mcinfo, XMLParser, xml_out);	

		// Make sure parent hasn't gone away
		long cur_ppid = getppid();
		if (cur_ppid != initial_ppid)
		{
		    // Assume parent killed, die now in graceful way
		    fprintf (stderr, "\nWarning: MemcheckView termination "
			     "detected. "
			     "Exiting Valgrind XML output monitor.\n");

		    fprintf (xml_out,
			     "<status> Memcheck XML to Tool Gear XML "
			     "converter terminated abnormally</status>\n");

		    // End the xml file
		    fprintf (xml_out, "</tool_gear>\n");
		    fflush (xml_out);
		    
		    // Close the xml file (if not stdout)
		    if (xml_out != stdout)
			fclose (xml_out);

		    exit (1);
		}
	    }
	}

	// Update idle minute count (if not exiting)
	if (!mcinfo.xmlEnded)
	    mcinfo.minutesIdle++;
    }


    // Warn that we timed out, should have no idle minutes
    if (mcinfo.minutesIdle > 60)
    {
	fprintf (stderr, "\nMemcheck XML to Tool Gear XML "
		 "timedout after %i minutes\n",
		 mcinfo.minutesIdle);

	fprintf (xml_out,
		 "<status> Memcheck XML to Tool Gear XML "
		 "timedout after %i minutes</status>\n",
		 mcinfo.minutesIdle);
    }
    
    // End the xml file
    fprintf (xml_out, "</tool_gear>\n");

    fflush (xml_out);

    // Close the xml file (if not stdout)
    if (xml_out != stdout)
	fclose (xml_out);

// For debugging 'hanging' XML parser case only
#if 0
    // DEBUG
    fprintf (stderr, "End of Valgrind XML detected\n");


    // DEBUG, how do we handle if never seen end XML token
    while (1)
    {
	// Sleep for 2.1 seconds (I hope)
	struct timespec mytimespec;
	mytimespec.tv_sec = 2;
	mytimespec.tv_nsec = 100000000;
	nanosleep (&mytimespec, NULL);
	fprintf (stderr, "DEBUG: In infinit loop still\n");
    }
#endif

    return 0;
}


// Parses input, return control when run out input (there may be parial
// line left when this returns
void parse_input(MemcheckInfo *mcinfo, XMLSnippetParser &XMLParser, 
		 FILE *xml_out)
{
    // Get pointer to const char * snippet returned by XMLParser
    const char *snippet = NULL;
    
    // Read the file and look for the list of call sites
    // Also read in pieces of the file header in the process
    while ((snippet = XMLParser.getNextSnippet(mcinfo)) != NULL)
    {
#if 0
	fprintf( stderr, 
		 "--------------\n"
		 "%s\n"
		 "--------------\n",
		 snippet);
#endif
	processXMLSnippet (xml_out, mcinfo, snippet, 
			   XMLParser.getSnippetOffset());

	// Reset the idle count
	mcinfo->minutesIdle = 0;
	fflush (xml_out);
    }
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

