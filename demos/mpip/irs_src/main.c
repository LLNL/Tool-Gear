#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "irs.h"
#include "irscom.h"
#include "irsdmp.h"
#include "irsedits.h"
#include "irseos.h"
#include "irsgen.h"
#include "irsgenrd.h"
#include "irsparm.h"
#include "irsrgst.h"
#include "irssys.h"
#include "irsctl.hh"
#include "Conditional.h"
#include "Hash.h"
#include "FunctionTimer.h"
#include "ProblemArray.h"
#include "VersionData.h"
#include "Gparm.hh"
int main( int argc, char **argv ) {
   char *me = "main";
   char **myargv;
   char hspname[MAXLINE];
   char rgdmpname[MAXLINE];
   char cname[MAXLINE];
   char msg[MAXLINE], tmpstr[MAXLINE];
   int  termfg = TERMFLG_READY;
   int  i, j, ierr;
   int  runflag, cycgflag, cycxflag, mdmpflag, lastflag;
   int  sewflag;
   int  cycval;
   double cpu;
   double runtime; 
   double debug_mpi_timer_start;
   HardwareInit() ;
   FunctionTimer_maxclock(&ft_maxclock, &ft_minclock);
   gv_perm_ht = hash_tbl_create(5, NULL);
   FunctionTimer_initialize(me, gv_perm_ht);
   cpu = getcpu();
   debug_mpi_timer_start = 0.0;
   setvbuf(stdout,0,_IONBF,0);
   userexitfn(exit_cleanup);
   getversion();
#ifdef UNIX
   signal(SIGINT,SIG_DFL);
#endif
   if (rgst_init() != 0) {
      ctlerror(me,"Registry initialization failed");  
   }
   ifdofq = -1 ;
   ifdocy = -1 ;
   ifparm = 1 ;
   ifplusmin = 1 ;
   parminit(); 
   expressions_init();
   inst_com();
   gparm = ALLOT(Gparm_t, ngparmx + 1);
   strcpy(meshlink_codesys,codename); 
   initget();
   setup_attrs();
   for (i = 1; i < argc; i++ ) {
      if (argv[i][0] == '-') {
         if (!strcmp(argv[i],"-sp2")) {
            strcpy(tmpstr, argv[i+1]);
            i++;
            if (strcmp(tmpstr,"seqrun") == 0) {
               sp2_seq_run_flag = TRUE;
            } else if (strcmp(tmpstr,"dontrun") == 0) {
               sp2_dont_run_flag = TRUE;
            } else {
               sprintf(msg,"\n\t-sp2 option '%s' is invalid"
                       "\n\tValid options are 'seqrun' and 'dontrun'",tmpstr);
               ctlerror(me,msg);
            }
	    argc = argc - 2;
	    for (j=i-1; j<argc; j++) {
               argv[j] = argv[j + 2];
            }
	    i = i - 1;
         }
      }
   }
   myargv = (char **)malloc(argc * sizeof(char *));
   ifparallel = checkpara(&argc, argv, &myargv);
   for (i=0; i<argc; i++) {
      argv[i] = myargv[i];
   }

   /* fprintf(stdout,"TERRY TRACE INFO - MPI ID : PID %10d:%10d\n",myid,getpid()); */

   rgdmpname[0] = '\0' ;
   outpath[0] = '\0' ;
   memset(hspname,'\0',MAXLINE);
   strcpy(hspname, codename );
   cycgflag = 0;
   cycxflag = 0;
   mdmpflag = 0;
   runflag  = 0;
   sewflag  = 0;
   lastflag = 0;
   warntime = 0;
   iffamily = 0;
   for (i = 1; i < argc; i++ ) {
      if (argv[i][0] == '-') {
         if (!strcmp(argv[i],"-c")) {
            cycval = atoi(argv[i+1]);
            cycflag=TRUE;
            i++;
         } else if (!strcmp(argv[i],"-cg")) {
            cycval = atoi(argv[i+1]);
            cycgflag=1;
            i++;
         } else if (!strcmp(argv[i],"-cx")) {
            cycval = atoi(argv[i+1]);
            cycxflag=1;
            i++;
         } else if (!strcmp(argv[i],"-nbaton")) {
            dmp_nbaton = atoi(argv[i+1]);
            i++;
         } else if (!strcmp(argv[i],"-d")) {
            if (argc <= i+1) {
               ctlerror(me,"-d [DUMP] must be followed by a file name");
            }
            strcpy(rgdmpname, argv[i+1]);
            i++;
	    if (!strcmp(rgdmpname,"lastwith")) {
               strcpy(rgdmpname, argv[i+1]);
               lastflag = 1;
               i++;
	    }
	    if (!strcmp(rgdmpname,"last")) {
               lastflag = 2;
	    }
         } else if (!strcmp(argv[i],"-dm")) {
            strcpy(rgdmpname, argv[i+1]);
            mdmpflag = 1;
            i++;
	    if (!strcmp(rgdmpname,"lastwith")) {
               strcpy(rgdmpname, argv[i+1]);
               lastflag = 1;
               i++;
	    }
	    if (!strcmp(rgdmpname,"last")) {
               lastflag = 2;
	    }
         } else if (!strcmp(argv[i],"-noerr")) {
            noerrflag = 1;
         } else if (!strcmp(argv[i],"-debug")) {
            debugflag = 1;
         } else if (!strcmp(argv[i],"-def")) {
            char defstring[MAXLINE];
            char *cptr1;
            char *cptr2;
            strcpy(defstring,argv[i+1]); 
            i++;
            cptr1 = strtok(defstring,"=");
            cptr2 = strtok(NULL,"=");
            gpdef0(0,cptr1, cptr2);
         } else if (!strcmp(argv[i],"-notimers")) {
            ft_timersflag = FALSE;
         } else if (!strcmp(argv[i],"-hack")) {
            hackblocking = 1;
         } else if ((!strcmp(argv[i],"-h")) || (!strcmp(argv[i],"-help"))) {
             printhelp();
	     exit_cleanup();
             exit(0);
         } else if (!strcmp(argv[i],"-fam")) {
            iffamily = 1;
         } else if (!strcmp(argv[i],"-k")) {
            strncpy(hspname, argv[i+1], MAXWORD);
            i++;
         } else if (!strcmp(argv[i],"-nd")) {
            multi_dump_write = atoi(argv[i+1]);
            i++;
         } else if (!strcmp(argv[i],"-o")) {
            strcpy(outpath, argv[i+1]);
            i++;
         } else if (!strcmp(argv[i],"-p")) {
            pccsflag = 1;
         } else if (!strcmp(argv[i],"-r")) {
            runflag = 1;
         } else if (!strcmp(argv[i],"-runtime")) {
            runtime_max = (double) atof(argv[i+1]);
            runtime_max = 60. * runtime_max;
            i++;
         } else if (!strcmp(argv[i],"-threads")) {
            ifthreads = 1;
	    ft_running_with_threads = TRUE;
         } else if (!strcmp(argv[i],"-sew")) {
            sewflag = 1;
         } else if (!strcmp(argv[i],"-merge")) {
            sewflag = 1;
         } else if (!strcmp(argv[i],"-barrier")) {
	   ifbarrier = 1;
         } else if (!strcmp(argv[i],"-v")) {
            pversion();
	    exit_cleanup();
            exit(0);
         } else if (!strcmp(argv[i],"-warn")) {
            warntime = atoi(argv[i+1]);
            warntime = warntime * 60;
            i++;
         } else {
            sprintf(msg,"Unknown option '%s', see usage guide below",argv[i]);
            ctlnotice(me,msg);
            printhelp();
	    exit_cleanup();
            exit(0);
         }
      } else {
         strcpy(inputDeck,argv[i]);
         newfile(argv[i],1);
      }
   }
   if (myid == 0 && warntime > 0) {
      irspcs_register();
   }


   if (outpath[0] != '\0') {
      i = strlen(outpath) - 1;
      if (outpath[i] != '/') {
         strcat(outpath,"/");
      }
   }
   strcpy(pbnm, hspname);

#ifndef TERRY_TRACE
   if (ifparallel) {
      setstdout(0);
   }
#endif

   strcat(hspname, "hsp");
   outfile(hspname);
   ctlmsg("(c) Copyright 1996-2001 Regents of University of California");

   sprintf(msg,"MPI ID:PID MAPPING %10d:%10d\n",myid,getpid());
   fprintf(stdout,"%s",msg);
   ctlmsg(msg);

   memset(cname,'\0',MAXLINE);
   strcpy(cname,codename);
   sprintf(msg,"%s version %s",cname,gv_ver.date_compiled);
   ctlmsg(msg);
   if (sp2_dont_run_flag == TRUE) {   
      goto cleanup_and_exit;
   }
   if (sewflag) {
      sewmeshes(mdmpflag);
      exit(0);
   }
   if (cycgflag) {
      condgdmp(cycval);
      goto cleanup_and_exit;
   }
   if (cycxflag) {
      goto cleanup_and_exit;
   }
   if (cycflag) {
      conddmp(cycval);
      goto cleanup_and_exit;
   }
   if (lastflag) {
      if (lastflag == 2)  {
         strcpy(rgdmpname,pbnm);
      }
      if (mdmpflag) {
         getdmplst_last(rgdmpname,"root",-1,mdmpflag);	 
      } else {
         getdmplst_last(rgdmpname,"silo",-1,mdmpflag);	 
      } 
   }
   if (rgdmpname[0] != '\0') {
      if (ifparallel) {
         initcom(rgdmpname,mdmpflag);
      } else {
         Restart_read_driver(rgdmpname,mdmpflag,0);
         if (lastfile == -1) lastfile = 0;
      } 
   }
   if ((debugflag == TRUE) && (ifmpi == TRUE)) {
      debug_mpi_timer_start = MPI_Wtime_Wrapper();
   }

#ifdef TERRY_TRACE
  trace_init();
#endif

   if (runflag == 1) {
      if (rgdmpname[0] != '\0') {
         termfg = run();
         if ((termfg == TERMFLG_USERSTOP) || (lastfile != 0)) {
            termfg = TERMFLG_READY;
         }
      }
   }
   while (termfg == TERMFLG_READY) {
      termfg = docmd();
      if ((termfg == TERMFLG_READY) && (runflag == 1) && (lastfile == 0))
         termfg = TERMFLG_RUN;
      while (termfg == TERMFLG_RUN) {
         termfg = run();
         if (termfg == TERMFLG_USERSTOP) {
            ps = NULL ;
            fpin = stdin ;
            fpold[++lastfile] = fpin ;
            termfg = docmd();
            fpin = fpold[--lastfile] ;
         }
      }
      if (runflag != 1) {
         if ((termfg == TERMFLG_CSTOP) || (termfg == TERMFLG_TSTOP))
            termfg = TERMFLG_READY;
      }
   }
   

   conditionals_atexit();
   runtime = getruntime();
   FunctionTimer_finalize(me, gv_perm_ht, 1.0);
   cpu = getcpu() - cpu ;
   sprintf(msg,"wall        time used: %e seconds\n"
               "total   cpu time used: %e seconds\n"
               "physics cpu time used: %e seconds", 
               runtime,cpu,runcpu);
   ctlmsg(msg);
   if ((debugflag == TRUE) && (ifmpi == TRUE)) {
      printf("mpi         time used: %e seconds\n",
             MPI_Wtime_Wrapper() - debug_mpi_timer_start);
   }
   /*
   for (i=0; i<nblk; i++) {
      if (myid == 0) {
         hash_analyze(domains[i].hash, TRUE, fpout, NULL, NULL);
      } else {
         hash_analyze(domains[i].hash, TRUE, NULL,  NULL, NULL);
      }
   }
   */
   if (ifparallel) {
      comcleanup();
   }   
   cleanup_and_exit:

#ifdef TERRY_TRACE
  trace_finalize();
#endif

   if (ifmpi == TRUE) {
      ierr = MPI_Finalize_Wrapper();
   } 	
   endout();
   memclr(); 
   ProblemArray_del_all(NULL);
   rgst_cleanup();
   exit(0);
}  

