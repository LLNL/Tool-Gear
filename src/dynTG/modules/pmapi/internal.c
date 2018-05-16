/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pmapi.h>


/*---------------------------------------------------------------------*/
/* Constants */

#define MAX_STRING  100
#define MAXCOUNTER  100
#define CONFIG_FILENAME ".pmapiconf"

#define DEFAULT_GROUP 2

#define ACCESS_CNT(c,i) cnt_table[c*MAXCOUNTER+i]

/*---------------------------------------------------------------------*/
/* global variables */

pm_info_t         pminfo;
pm_groups_info_t  pmgroup;
pm_prog_t         pmprog;

int               usegroups;
int               at_least_one_group;
int               *cnt_table,*rec_cntnum,*rec_line,*config,*rec_maxline;
int               rec_max;


/*---------------------------------------------------------------------*/
/* open PMAPI and get the number of counters */

int pmapi_int_open()
{
  int err,i;

  err=pm_init(PM_VERIFIED|PM_UNVERIFIED|PM_CAVEAT|PM_GET_GROUPS,&pminfo,&pmgroup);
  if (err!=0)
    return -1;

  if (pmgroup.maxgroups==0)
    usegroups=0;
  else
    usegroups=1;


  config=(int*)malloc(sizeof(int)*pminfo.maxpmcs);
  if (config==NULL)
    return -1;

  if (usegroups)
    {
      cnt_table=(int*)malloc(sizeof(int)*pmgroup.maxgroups);
      if (cnt_table==NULL)
	return -1;

      for (i=0; i<pmgroup.maxgroups; i++)
	cnt_table[i]=1;

      at_least_one_group=0;
    }
  else
    {
      cnt_table=(int*)malloc(sizeof(int)*pminfo.maxpmcs*MAXCOUNTER);
      if (cnt_table==NULL)
	return -1;
      rec_line=(int*)malloc(sizeof(int)*pminfo.maxpmcs);
      if (rec_line==NULL)
	return -1;
      rec_cntnum=(int*)malloc(sizeof(int)*pminfo.maxpmcs);
      if (rec_cntnum==NULL)
	return -1;
      rec_maxline=(int*)malloc(sizeof(int)*pminfo.maxpmcs);
      if (rec_maxline==NULL)
	return -1;

      for (i=0; i<pminfo.maxpmcs; i++)
	rec_cntnum[i]=-1;
      for (i=0; i<pminfo.maxpmcs*MAXCOUNTER; i++)
	cnt_table[i]=-1;
    }

  return pminfo.maxpmcs;
}


/*---------------------------------------------------------------------*/
/* Try to add a counter to the counter array */

int add_to_count(int num, char *name)
{
  int i,j,ret;

  if (num>=MAXCOUNTER)
    {
      fprintf(stderr,"WARNING: Ignoring counter %s (too many counters)\n",name);
      return 0;
    }

  /* now see if we can find this counter */

  ret=0;
  for (i=0; i<pminfo.maxpmcs; i++)
    {
      for (j=0; j<pminfo.maxevents[i]; j++)
	{
	  if (strcmp(pminfo.list_events[i][j].short_name,name)==0)
	    {
	      ACCESS_CNT(i,num)=j;
	      ret=1;
	    }
	}
    }
  return ret;
}


/*---------------------------------------------------------------------*/
/* Try to add a counter to a group */

int add_to_group(int num,char *name)
{
  int found,i,j,k;

  found=0;

  /* how many groups have this event */

  for (i=0; i<pmgroup.maxgroups; i++)
    {
      if (cnt_table[i])
	{
	  for (j=0; j<pminfo.maxpmcs; j++)
	    {
	      if (pmgroup.event_groups[i].events[j]!=COUNT_NOTHING)
		{
		  for (k=0; k<pminfo.maxevents[j];k++)
		    {
		      if (pmgroup.event_groups[i].events[j]==pminfo.list_events[j][k].event_id)
			{
			  if (strcmp(pminfo.list_events[j][k].short_name,name)==0)
			    {
			      /* found the counter */
			      found++;
			      cnt_table[i]=2;
			      at_least_one_group=1;
			    }
			}
		    }
		}
	    }
	}
    }

  /* delete all other ones */

  if (found>0)
    {
      for (i=0; i<pmgroup.maxgroups; i++)
	{
	  if (cnt_table[i]==2)
	    {
	      cnt_table[i]=1;
	    }
	  else
	    {
	      cnt_table[i]=0;
	    }
	}
    }
}


/*---------------------------------------------------------------------*/
/* Recursive routine to find counter setup */

void find_counter_dist(int level,int rec)
{
  int i,found,iter;

  /* check if the new condition is better */

  if (rec<rec_max)
    iter=rec;
  else
    iter=rec_max;

  for (i=0; i<iter; i++)
    {
      if (rec_line[i]>rec_maxline[i])
	{
	  /* we have sth. better already */
	  return;
	}
    }

  if (rec_max<rec)
    {
      /* we found a new longest sequence */

      for (i=0; i<rec; i++)
	{
	  rec_maxline[i]=rec_line[i];
	}
      for (i=0; i<pminfo.maxpmcs; i++)
	{
	  config[i]=rec_cntnum[i];
	}
      rec_max=rec;
    }

  /* recursion end condition */

  if ((level>=MAXCOUNTER) || (rec>=pminfo.maxpmcs))
    return;

  /* test the current config */

  /* loop over all options */

  found=0;
  for (i=0; i<pminfo.maxpmcs; i++)
    {
      if (ACCESS_CNT(i,level)>=0)
	{
	  /* The counter at level is available as counter i */
	  
	  if (rec_cntnum[i]==-1)
	    {
	      /* This counter is still free */
	      
	      found=1;
	      rec_cntnum[i]=ACCESS_CNT(i,level);
	      rec_line[rec]=level;
	      find_counter_dist(level+1,rec+1);
	      rec_cntnum[i]=-1;
	    }
	}
    }

  if (found==0)
    {
      /* we can't place this counter, skip it */
      find_counter_dist(level+1,rec);
    }
}

/*---------------------------------------------------------------------*/
/* Finish up counter arrays */

void add_to_count_complete()
{
  int i;

  /* start recursion to find optimal table */

  rec_max=0;
  find_counter_dist(0,0);

  /* user mode counting only */

  pmprog.mode.w=0;
  pmprog.mode.b.user=1;

  /* set events */

  for (i=0; i<pminfo.maxpmcs; i++)
    {
      if (config[i]>=0)
	pmprog.events[i]=config[i];
      else
	pmprog.events[i]=COUNT_NOTHING;
    }
}


/*---------------------------------------------------------------------*/
/* Finish up counter sets */

void add_to_group_complete()
{
  int i,global_grp,k,j;

  global_grp=DEFAULT_GROUP;

  /* find the group */

  if (at_least_one_group)
    {
      for (i=0; i<pmgroup.maxgroups; i++)
	{
	  if (cnt_table[i])
	    {
	      global_grp=i;
	      break;
	    }
	}
    }

  /* find the counters in that group */

  for (i=0; i<pminfo.maxpmcs; i++)
    {
      if (pmgroup.event_groups[global_grp].events[i]==COUNT_NOTHING)
	{
	  config[i]=-1;
	}
      else
	{
	  for (k=0; k<pminfo.maxevents[i];k++)
	    {
	      if (pmgroup.event_groups[global_grp].events[i]==pminfo.list_events[i][k].event_id)
		config[i]=k;
	    }
	}
    }

  /* check for doubles */

  for (i=0; i<pminfo.maxpmcs; i++)
    for (j=0; j<i; j++)
      {
	if (strcmp(pminfo.list_events[i][config[i]].short_name,
		   pminfo.list_events[j][config[j]].short_name)==0)
	  {
	    config[i]=-1;
	  }
      }

  /* user mode counting only */

  pmprog.mode.w=0;
  pmprog.mode.b.user=1;

  pmprog.events[0] = global_grp;
  pmprog.mode.b.is_group = 1;

}


/*---------------------------------------------------------------------*/
/* read config file and select counters */

int pmapi_int_readconfig()
{
  FILE  *conffile;
  char  name[MAX_STRING];
  char  *envptr;
  int   counter,res,i,j;

  envptr=getenv("HOME");
  sprintf(name,"%s/%s",envptr,CONFIG_FILENAME);
  conffile=fopen(name,"r");

  counter=0;

  if (conffile==NULL)
    {
      if (usegroups==0)
	{
	  res=add_to_count(counter,"PM_CYC");
	  if (res) counter++;
	  res=add_to_count(counter,"PM_INSTCMPL");
	  if (res) counter++;
	  res=add_to_count(counter,"PM_LD_MISS_L1");
	  if (res) counter++;
	  res=add_to_count(counter,"PM_DC_INV_L2");
	  if (res) counter++;
	  res=add_to_count(counter,"PM_INST_DISP");
	  if (res) counter++;
	  res=add_to_count(counter,"PM_INST_CMPL");
	  if (res) counter++;
	  res=add_to_count(counter,"PM_ST_REF_L1");
	  if (res) counter++;
	  res=add_to_count(counter,"PM_LD_REF_L1");
	  if (res) counter++;
	}
    }
  else
    {
      while (!feof(conffile))
	{
	  fscanf(conffile,"%s\n",name);
	  
	  if (usegroups)
	    {
	      res=add_to_group(counter,name);
	    }
	  else
	    {
	      res=add_to_count(counter,name);
	    }
	  
	  if (res) counter++;
	}
    }

  if (usegroups)
    add_to_group_complete();
  else
    add_to_count_complete();

  return 0;
}


/*---------------------------------------------------------------------*/
/* return name for menu and column */

int pmapi_int_getname(int i, char *name, char *desc)
{
  if (config[i]>=0)
    {
      strcpy(desc,pminfo.list_events[i][config[i]].short_name);
      sprintf(name,"CNT %i: %s (%x)",i,desc,pminfo.list_events[i][config[i]].status);

      return 0;
    }
  else
    return -1;
}


/*---------------------------------------------------------------------*/
/* Program all counters as selected by the config file */
/* assumes pmapi_int_readconfig has already been called */

int pmapi_program_counters()
{
  int i,res;

  /* set program */

  res=pm_set_program_mythread(&pmprog);
  if (res<0)
    return res;
  
  /* reset counting */

  res=pm_reset_data_mythread();
  if (res<0)
    return res;

  /* start counting */

  res=pm_start_mythread();
  if (res<0)
    return res;

  return 0;
}


/*---------------------------------------------------------------------*/
/* Read one counter */

double pmapi_int_getcounter(int cnt)
{
  pm_data_t data;
  int       res;
  double    value;

  res=pm_get_data_mythread(&data);
  if (res<0)
    return res;

  value=data.accu[cnt];

  /*  printf("This is counter %i: %f\n",cnt,value); */

  return value;
}


/*---------------------------------------------------------------------*/
/* The End. */
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

