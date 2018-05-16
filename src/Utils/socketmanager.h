//! file socketmanager.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
//  Created by John Gyllenhaal 25 April 2004

#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#include "tg_socket.h"
#include "tg_pack.h"
#include "command_tags.h"
#include "messagebuffer.h"
#include "stdarg.h"
#include "stdio.h"

//! Manages packing and sending data over a socket.

//! This class is a wrapper around TG_pack and TG_send to
//! make sending messages via sockets easier and more
//! foolproof (as far as buffering).
class SocketManager
{
public:
    SocketManager (int socketId, FILE *logFile=NULL) : 
	sock (socketId), log (logFile) {}

    //! Packs and sends message to socket with format 'fmt'
    void packSend (int tag, int id, const char *fmt, ...)
	{
	    va_list ap;

	    // Get start of varargs
	    va_start (ap, fmt);

	    // Use TG_vpack to pack message
	    int len = TG_vpack (mbuf, fmt, ap);

	    // Clean up varargs
	    va_end (ap);

#if 0
	    // DEBUG
	    fprintf (stderr, "packSend: %i bytes\n", len);
#endif

	    // Send message to socket
	    TG_send (sock, tag, id, len, mbuf.contents());
	}

    //! If log file enabled, writes message to log file
    void writeLog (const char *fmt, ...)
	{
	    va_list args;

	    // Do nothing if no log file 
	    if (log == NULL)
		return;

	    /* Print out log message in printf format using vfprintf */
	    va_start (args, fmt);
	    vfprintf (log, fmt, args);
	    va_end(args);

	    /* For now, flush out each log entry */
	    fflush (log);
	}


    void sendInfoAboutTool (const char *toolInfo)
	{packSend (DPCL_TOOL_INFO, 0, "S", toolInfo);}

    void sendCreateViewer (ViewType t)
	{TG_send (sock, GUI_CREATE_VIEWER, (int)t,  0, 0);}

    void sendStaticDataComplete ()
	{TG_send (sock, DB_STATIC_DATA_COMPLETE, 0,  0, 0);}

    void sendDeclareMessageFolder (const char *messageFolderTag,
				 const char *messageFolderTitle)
	{ 
	    packSend (DB_DECLARE_MESSAGE_FOLDER, 0, "SS", messageFolderTag,
		      messageFolderTitle);
	    writeLog ("  <new_list>\n"
		      "    <tag>%s</tag>\n"
		      "    <title>%s</title>\n"
		      "  </new_list>\n\n",
		      messageFolderTag, messageFolderTitle);
	}

    void sendAddMessage (const char *messageFolderTag,
			 const char *messageText,
			 const char *messageTraceback)
	{
	    packSend (DB_ADD_MESSAGE, 0, "SSS", messageFolderTag,
		      messageText, messageTraceback);
	    writeLog ("  <message>\n"
		      "     <list>%s</list>\n"
		      "     <text>%s</text>\n"
		      "     <traceback>\n"
		      "%s\n"
		      "     </traceback>\n"
		      "  </message>\n\n", 
		      messageFolderTag, messageText, messageTraceback);
	}

    void sendDataAttr(const char * dataAttrTag,
		      const char * dataAttrText, const char * description,
		      int dataType)
	{packSend (DB_DECLARE_DATA_ATTR, 0, "SSSI", dataAttrTag,
		   dataAttrText, description, dataType );}

    void sendModuleParsed(const char * modulePath)
	{packSend (DB_FILE_PARSE_COMPLETE, 0, "S", modulePath);}

    void sendModule(const char * modulePath)
	{packSend (DB_INSERT_MODULE, 0, "S", modulePath);}

    void sendFunctionParsed(const char *funcName)
	{packSend (DB_FUNCTION_PARSE_COMPLETE, 0, "S", funcName);}
    
    void sendFunction( const char * function, const char * path,
		       int start, int stop)
	{packSend (DB_INSERT_FUNCTION, 0, "SSII", function, path,
		   start, stop );}

    void sendPlainEntry( const char * function, const char * tag,
			 int line, const char * description)
	{packSend (DB_INSERT_PLAIN_ENTRY, 0, "SSIS", function, tag,
		   line, description );}

    void sendInt( const char * function, const char * tag,
		  const char * dataAttrTag, int process, int thread,
		  int data)
	{packSend (DB_INSERT_INT, 0, "SSSIII", function, tag,
		   dataAttrTag, process, thread, data );}

    void sendDouble( const char * function, const char * tag,
		     const char * dataAttrTag, int process, int thread,
		     double data)
	{packSend (DB_INSERT_DOUBLE, 0, "SSSIID", function, tag,
		   dataAttrTag, process, thread, data );}

    void sendIntResult( int result, const char * message,
			int tag)
	{packSend (tag, 0, "IS", result, message );}

    void sendXMLSnippet (const char *XMLSnippet, int lineOffset)
	{packSend (DB_PROCESS_XML_SNIPPET,0, "IS", lineOffset, XMLSnippet);}


    void flush ()
	{TG_flush (sock);}

private:
    int sock;
    FILE *log;
    MessageBuffer mbuf;
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

