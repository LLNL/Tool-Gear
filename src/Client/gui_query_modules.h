//! \file gui_query_modules.h
//!
/***************************************************************************/
/* Tool Gear (www.llnl.gov/CASC/tool_gear)                                 */
/* Version 2.00                                             March 29, 2006 */
/* Please see COPYRIGHT AND LICENSE information at the end of this file.   */
/***************************************************************************/
// John May, 26 October 2000

#ifndef GUI_QUERY_MODULES_H
#define GUI_QUERY_MODULES_H

#include <qobject.h>
#include <qdialog.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

//! Displays and Querys List of DPCL Modules

#define BUFFER_SIZE (1<<14)

class GUIQueryModuleDialog : public QDialog {

public:
	GUIQueryModuleDialog(int nom, char **mt, char **mt_s, int sock); 
	~GUIQueryModuleDialog();

protected:
	virtual void accept();
	virtual void reject();

private:
	QPushButton  *dlg_but_ok;
	QPushButton  *dlg_but_cancel;
	QCheckBox   **dlg_chkbox;

	int number_of_modules;
	int avail_mod;
	char **module_table;
	char **module_table_s;
	int fd;

};



GUIQueryModuleDialog::GUIQueryModuleDialog(int nom,
					   char **mt,
					   char **mt_s,
					   int sock) : QDialog(0,
							       "DynTG: Which probe/collector modules would you like to use?",
							       TRUE,
							       0)
{
  int i;
  char name[200];
#if 0
  char sendbuf[BUFFER_SIZE];
  int length;
#endif
  
  number_of_modules=nom;
  module_table=mt;
  module_table_s=mt_s;
  fd=sock;

  setCaption("DynTG: Which probe/collector modules would you like to use?");
  
  dlg_but_ok= new QPushButton("Start DynTG",this,"Start DynTG");
  connect(dlg_but_ok,SIGNAL(clicked()),this,SLOT(accept()));
  
  dlg_but_cancel = new QPushButton("Abort DynTG",this,"Abort DynTG");
  connect(dlg_but_cancel,SIGNAL(clicked()),this,SLOT(reject()));
  
  dlg_chkbox = (QCheckBox**) malloc(sizeof(QCheckBox*)*number_of_modules);
  if (dlg_chkbox==NULL)
    {
      TG_send( fd, GUI_SAYS_QUIT, 0, 0, 0 );
      TG_flush( fd );	 
      printf("Memory Error\n");
      
      exit(0);
    }

  avail_mod=0;
  
  for (i=0; i<number_of_modules; i++)
    {
      if (strcmp(module_table_s[i],"NONE")!=0)
	{
	  sprintf(name,"%s: %s\n",module_table_s[i],module_table[i]);
	  
	  dlg_chkbox[i] = new QCheckBox(name,this,"Available Modules");
	  dlg_chkbox[i]->setGeometry(10,10+20*i,480,15);
	  dlg_chkbox[i]->setChecked(TRUE);
	  avail_mod++;
	}
      else
	dlg_chkbox[i]=NULL;
    }
  
  dlg_but_ok->setGeometry(80,avail_mod*20+20,150,30);
  dlg_but_cancel->setGeometry(280,avail_mod*20+20,150,30);
  
  setGeometry(100,100,500,avail_mod*20+60);
}


GUIQueryModuleDialog::~GUIQueryModuleDialog()
{
  int i;
  
  if (dlg_but_ok!=NULL) delete dlg_but_ok;
  if (dlg_but_cancel!=NULL) delete dlg_but_cancel;
  if (dlg_chkbox!=NULL) 
    {
      for (i=0; i<number_of_modules; i++)
	delete dlg_chkbox[i];
    }
}


void GUIQueryModuleDialog::accept()
{
  int i;
  char sendbuf[BUFFER_SIZE];
  int length;
  
  QDialog::accept();

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
  
  delete this;
}

void GUIQueryModuleDialog::reject()
{
  QDialog::reject();

  TG_send( fd, GUI_SAYS_QUIT, 0, 0, 0 );
  TG_flush( fd );	 
  printf("User Cancel\n");

  delete this;
}

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

