// dpcl_collector.cpp
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 23 August 2001
//! This program is invoked by the Tool Gear client to execute
//! and gather data from a target program using DPCL.

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <dpcl.h>

#include "tg_socket.h"
#include "parse_program.h"
#include "command_tags.h"
#include "dpcl_callbacks.h"
#include "collector_pack.h"
#include "dpcl_socket.h"
#include "dpcl_action_type.h"
#include "mpx_actions.h"
#include "timer_actions.h"
#include "tg_globals.h"
#include "tg_time.h"
#include "dpcl_error.h"
#include "messagebuffer.h"

/* MS/START - DYNAMIC MODULE LOAD */
#include <dirent.h>
#include "base.h"
#include <dlfcn.h>
#include <ctype.h>

 void               **modhandles;




/* select routine for scandir */
/* Make case independent -JCG 3/17/06 */
int check_for_name(char *namelist, char *name)
{
  char *run;

  while (*namelist!=((char)0))
    {
      run=name;
      while ((toupper(*namelist)==toupper(*run))&&
	     (*namelist!=((char)0))&&(*name!=((char)0)))
	{
	  namelist++;
	  run++;
	}
      if ((run!=name) && ((*namelist==',')||(*namelist==*run)))
	{
	  return 1;
	}
      while ((*namelist!=',')&&(*namelist!=((char)0)))
	namelist++;
      if (*namelist==',')
	namelist++;
    }
  return 0;
}


int select_module(struct dirent *ent)
{
  if (strstr(ent->d_name,"col_")==ent->d_name)
  /*if (strstr(ent->d_name,local_chkpt_name)==ent->d_name)*/
    {
      /* Yes, the instance was the prefix of the name */
      return 1;
    }

  return 0;
}

/* MS/END - DYNAMIC MODULE LOAD */


int connectToClient( int port );

int main( int argc, char * argv[], char * envp[] )
{
  /* MS/START - DYNAMIC TOOL LOAD */
  
  char               *libdir;
  int                nument,err,i;
  struct dirent      **namelist;
  int                *index;
  char               fullname[300];
  getneeds_t         needroutine;
  toolCapabilities_t *needs;
  int                modcount;

  /* MS/END - DYNAMIC TOOL LOAD */

        /* check arguments */

	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s <port> [module]\n", argv[0]);
		return -1;
	}
	// TG_timestamp ("DPCL thread: main() started\n");
	TG_argc = argc;
	TG_argv = argv;
	TG_envp = envp;


	/* initialize TG */

	int port = atoi( argv[1] );
	int sock = connectToClient( port );

	if( sock < 0 ) return -1;

	// Enable DPCL_error to sent DPCL_SAYS_QUIT before exit  -JCG 3/16/06
	DPCL_error_sock = sock;
	TG_dpcl_socket_out = sock;

	    // Describe the tool for the About... box
	pack_and_send_info_about_tool(
	    "Tool Gear 2.00's DynTG Tool:\n"
	    "A tool for interactive, dynamic instrumentation of either serial or parallel applications.\n"
	    "Written by Martin Schulz, John May, and John Gyllenhaal\n",
	    sock );



	/* MS/START - DYNAMIC TOOL LOAD */

	/* get library directory */
	libdir=getenv("TG_MODULELIBRARY");
	if (libdir==NULL)
	{
	    DPCL_error ("Error: Environment variable TG_MODULELIBRARY not set!"
			"  Required by dynTG!\n");
	}
	
	/* scan the directory */
	
	nument=scandir(libdir,&namelist,select_module,alphasort);
	if (nument<0)
	{
	    DPCL_error ("Error: Unable to read directory specified by "
			"TG_MODULELIBRARY:\n  %s\n", libdir);
	}
	if (nument==0)
	{
	    DPCL_error ("Error: No dynTG modules found in directory specified "
			"by TG_MODULELIBRARY:\n  %s\n", libdir);
	}

	/* create handle array */
	
	modhandles = (void**) malloc (nument*sizeof(void*));
	if (modhandles==NULL)
	{
	    DPCL_error ("Out of memory");
	}
	
	index = (int*) malloc (nument*sizeof(int));
	if (index==NULL)
	{
	    DPCL_error ("Out of memory");
	}
	
	/* loop over all modules and load them */
	
	needs=(toolCapabilities_t*) malloc(sizeof(toolCapabilities_t)*nument);
	if (needs==NULL)
	{
	    DPCL_error ("Out of memory");
	}

	modcount=0;

	for (i=0; i<nument; i++)
	  {
	    /* open module */
	    
	    sprintf(fullname,"%s/%s",libdir,namelist[i]->d_name);
	    modhandles[modcount]=dlopen(fullname,RTLD_NOW);
	    if (modhandles[modcount]==NULL)
	    {
		DPCL_error ("Error: Unable to dlopen %s",
			    fullname);
	    }
	    
	    /* query routine */
	    
	    needroutine=(getneeds_t) dlsym(modhandles[modcount],"GetToolNeeds");
	    if (needroutine==NULL)
	    {
		DPCL_error ("Error: Unable to find symbol GetToolNeeds in %s",
			    fullname);
	    }
	    	    
	    /* get the needs for this module */

	    err=(*needroutine)(&(needs[i]));
	    if (err!=0)
	    {
		DPCL_error ("Error %i calling GetToolNeeds in %s",
			    err, fullname);
	    }
	    
	    /* check if we are supposed to load this */

	    if (argc==3)
	    {
		err=check_for_name(argv[2],needs[i].shortname);
	    }
	    else
	    {
		err=1;
	    }
	    
	    if (err)
	    {
		/* evaluate needs and try accomodate them */
		
		if (needs[i].useDPCL==0)
		{
		    DPCL_error ("Error module %s doesn't need DPCL, "
				"currently unsupported!",
				fullname);
		}
		
		index[modcount]=i;
		modcount++;

		err=0;
	      }
	    else
	      {
		/* ignore this module */

		dlclose(modhandles[modcount]);
		modhandles[modcount]=NULL;
		index[i]=-1;
	      }
	  }

	/* check if we have any modules left */

	if (modcount==0)
	{
	    // If error due to mispelled -m module_name, tell user -JCG 3/17/06
	    if (argc==3)
	    {
		MessageBuffer errorMessage;
		errorMessage.sprintf (
		    "Error: Unabled to find module(s) '%s' specified on "
		    "command line with -m.\n", 
		    argv[2]);
		errorMessage.appendSprintf (
		    "\nAvailable module names (case insensitive) are:\n");
		for (int i=0; i<nument; i++)
		{
		    errorMessage.appendSprintf("  %s\n", needs[i].shortname);
		}
		DPCL_error ("%s", errorMessage.contents());
	    }
	    else
	    {
		DPCL_error ("Error: Ignored all modules found in directory specified "
			    "by TG_MODULELIBRARY:\n  %s\n", libdir);
	    }
	}

	/* now that we have accepted the modules, send its name to TG */

	if (argc!=3)
	  {
	    pack_and_sendModuleCount(sock,modcount);
	
	    for (i=0; i<modcount; i++)
	      {
		if (modhandles[i]==NULL)
		  {
		    pack_and_sendModuleName(sock,i,"n/a",
					    "NONE");
		  }
		else
		  {
		    pack_and_sendModuleName(sock,i,needs[index[i]].name,
					    needs[index[i]].shortname);
		  }
	      }
	  }

	/* start DPCL */

	Ais_initialize();

	// Set up handler to receive messages from GUI
	Ais_add_fd( sock, dpcl_socket_handler );

	/* print the modules in TG ask TG to activate some modules */

	if (argc!=3)
	  {
	    pack_and_sendModuleQuery(sock);
	  }
	else
	  {
	    inittool_t initroutine;

	    /* The has defined the module, so let's load it (or them?) */

	    for (i=0; i<modcount; i++)
	      {
		if (modhandles[i]!=NULL)
		  {
		    initroutine=(inittool_t) dlsym(modhandles[i],"InitializeDPCLTool");
		    if (initroutine==NULL)
		      {
			  DPCL_error ("InitializeDPCLTool not found for module %i\n",
				      i);
		      }
		    
		    /* run the init routine for this module */
		    
		    err=(*initroutine)(sock, modhandles[i]);
		    if (err!=0)
		      {
			  DPCL_error ("Error running InitializeDPCLTool "
				      "for module %i\n", i);
		      }
		  }
	      }
	    pack_and_send_create_viewer( TREE_VIEW, sock );
	    TG_flush( sock );
	  }

	/* MS/END - DYNAMIC TOOL LOAD */


	// What to do when the target program exits
	Ais_override_default_callback( AIS_PROC_TERMINATE_MSG,
			termination_cb, (GCBTagType)&sock,
			NULL, NULL );


	// Begin the DPCL event loop
	Ais_main_loop();
	// Not listening to the Client socket after this point
	// (if we do, bdestroy seems to get bogged down for parallel jobs)

#if 0
	// Shut down the application before exiting
	if( TG_app != NULL ) {
		// App should be in attached or created state
		// before it's destroyed
		Process p = TG_app->get_process(0);
		ConnectState state;
		AisStatus retval = p.query_state(&state);
		if( retval != ASC_success ) {
			TG_timestamp("DPCL collector: "
					"Didn't get thread state\n");
		}

		if( state != PRC_attached && state != PRC_created ) {
			TG_app->battach();
		}

		// Remove instrumentation that might interfere with quitting
		remove_all_actions( TG_app );

		// bdestroy isn't a virtual function for some reason, so
		// we have to call it with a type cast to get the app
		// destroyed correctly.
		if( TG_is_poe_app ) {
			((PoeAppl *)TG_app)->bdestroy();
		} else {
			TG_app->bdestroy();
		}
		delete TG_app;
	}

	if( TG_process != NULL ) {
		// Process should be in attached or created state
		// before it's destroyed
		ConnectState state;
		TG_process->query_state(&state);
		if( state != PRC_attached && state != PRC_created ) {
			TG_process->battach();
		}
		TG_process->bdestroy();
		delete TG_process;
	}

	// Acknowledge quitting
	TG_send( sock, DPCL_SAYS_QUIT, 0, 0, NULL );
	TG_flush( sock );
	exit(2);
#endif

//	TG_timestamp ("DPCL thread: exiting\n");


	return 0;
}


int connectToClient( int port )
{
        int sock = socket( PF_INET, SOCK_STREAM, 0 );
        if( sock == -1 ) {
                perror( "error creating socket" );
                return -1;
        }

        // Socket must be nonblocking for TG_send and TG_recv
        if( fcntl( sock, F_SETFL, O_NONBLOCK ) == -1 ) {
                fprintf( stderr, "Error making collector socket nonblocking" );
        }

	// Setting TCP_NODELAY avoids pauses for short messages
	int ndelay = 1;
	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, &ndelay, sizeof(ndelay) );

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        struct hostent * hp = gethostbyname( "localhost" );
        if( hp == NULL ) {
                perror( "error looking up localhost address" );
                return -1;
        }

        bcopy( hp->h_addr, &address.sin_addr, hp->h_length );
        address.sin_port = htons( port );

        int err = connect( sock, (struct sockaddr *)&address,
                        sizeof(address) );
        if( err != 0 ) {
                perror( "connect failed" );
                return -1;
        }

#if 0
	// Set the socket buffer size
        int outsz, insz;
	size_t length = sizeof(int);

	getsockopt( sock, SOL_SOCKET, SO_SNDBUF, &outsz, &length );
	getsockopt( sock, SOL_SOCKET, SO_RCVBUF, &insz, &length );
	printf("Socket outbuf: %d inbuf: %d\n", outsz, insz );

        outsz = SOCK_BUF_SIZE, insz = SOCK_BUF_SIZE;
	setsockopt( sock, SOL_SOCKET, SO_SNDBUF, &outsz, length );
	setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &insz, length );

	// DEBUG -- look at buffer sizes
	getsockopt( sock, SOL_SOCKET, SO_SNDBUF, &outsz, &length );
	getsockopt( sock, SOL_SOCKET, SO_RCVBUF, &insz, &length );
	printf("Socket outbuf: %d inbuf: %d\n", outsz, insz );
#endif


	// Shake hands with the spawning process; this also
	// verfies to the client that the process talking to
	// it over the socket is the one it launched.
//	fprintf(stderr, "collector is alive using socket %d!\n", sock);
	char checkValue[20];
	char * f = fgets( checkValue, 20, stdin );
	if( f == NULL ) {
		if( errno ) {
			perror( "reading in collector");
			exit(-1);
		}
	}
	char *end_ptr = NULL;
	unsigned int outval = strtoul(checkValue, &end_ptr, 0);
	if ((*end_ptr != 0) && (*end_ptr != '\n'))
	{
	    perror ("Error parsing authentication value 'outval'");
	    exit (-1);
	}
	write( sock, (void *) &outval, sizeof(outval) );

        return sock;
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

