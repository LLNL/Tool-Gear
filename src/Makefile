# **************************************************************************
#  Tool Gear (www.llnl.gov/CASC/tool_gear)
#  Version 2.00                                              March 29, 2006
#  Please see COPYRIGHT AND LICENSE information at the end of this file.
# **************************************************************************
# Builds Tool Gear infrastructure and any Tool-Gear-based tool subdirectories
# present in the directory (mpipview, memcheckview, umpireview, dynTG, etc.).
# 
# The makefile designed to allow individual tools to be released separately
# or bundled all together.
#
# Most of the tools uses Qt's qmake program to build its Makefile and
# the compiler configuration used by Qt.   
# On AIX/xlC, we recommend configuring Qt to build everything with 
# -qstaticinline.  This will remove most duplicate symbol warnings.
#
# For Tool Gear 2.00, you must have Qt 3.3.1 or later in your path and its 
# environment variables set up properly to build the Client and Mpipview.  
# (Qt 3.3.3 recommended with Mac OS.)
# (Tested and works for "free" and "licensed" Qt 3.3.1, 3.3.2, and 3.3.3.  
#  Qt 3.3.0 is flaky with TGclient.  Not compatible with earlier Qt versions.)
# Not compatible with Qt 4.
#
# dynTG will currently only attempt to compile on IBM platform's with DPCL
# installed.
#
# The default 'all' target places all the binaries and scripts in the ../bin
# directory.   The scripts (mpipview, memcheckview, etc.) are modified during
# the install process to set TGROOT to the root directory of where the tools
# are built (so will have to be edited if you are moving the binaries).
# We recommend linking the scripts (not the exectuables) into /usr/local/bin
# for ease of use.  

all: checkQtVersion TGclient TGxmlserver TGmpip2xml TGmemcheck2xml \
	 umpireview_script dynTGBinaries
	@echo "----"
	@echo "DONE"
	@echo "----"

# All the targets in this file are phony, so declare them that way
# so that bad file name choices does not disable the make file
.PHONY: all checkQtVersion mpipview memcheckview umpireview dynTG \
	clean TGclient TGxmlserver TGmpip2xml TGmemcheck2xml \
	umpireview_script dynTGBinaries

# Verify Qt as much as we can
checkQtVersion:
	@./Utils/checkQtVersion

mpipview: checkQtVersion TGclient TGxmlserver TGmpip2xml
	@echo "----"
	@echo "DONE"
	@echo "----"

memcheckview: checkQtVersion TGclient TGxmlserver TGmemcheck2xml
	@echo "----"
	@echo "DONE"
	@echo "----"

umpireview: checkQtVersion TGclient TGxmlserver umpireview_script
	@echo "----"
	@echo "DONE"
	@echo "----"

dynTG: checkQtVersion TGclient dynTGBinaries
	@echo "----"
	@echo "DONE"
	@echo "----"


clean:
	@if [ -f Client/Makefile ]; then \
		cd Client ; \
		${MAKE} clean; \
	fi ;
	@if [ -f Xmlserver/Makefile ]; then \
		cd Xmlserver ; \
		${MAKE} clean; \
	fi ;
	@if [ -f Mpipview/Makefile ]; then \
		cd Mpipview ; \
		${MAKE} clean; \
	fi ;
	@if [ -f Memcheckview/Makefile ]; then \
		cd Memcheckview ; \
		${MAKE} clean; \
	fi ;
	@if [ -f dynTG/Makefile ]; then \
		cd dynTG ; \
		${MAKE} clean; \
	fi ;


TGclient:
	@if [ "${QTDIR}" = "" ]; then \
		echo "*************************************************************"; \
		echo "ERROR: QTDIR NOT SET, Qt 3.3.1 or later 3.X version REQUIRED!"; \
		echo "Qt's website: www.trolltech.com"; \
		echo "*************************************************************"; \
		exit 1; \
	fi ;
	@if [ -f Client/tgclient.pro ]; then \
		echo "----------------------------------------"; \
		echo "BUILDING Client binaries (Tool Gear GUI)"; \
		echo "----------------------------------------"; \
		cd Client ; \
		qmake tgclient.pro; \
		${MAKE} ; \
	fi ;

TGxmlserver:
	@if [ -f Xmlserver/tgxmlserver.pro ]; then \
		echo "-------------------------------------------"; \
		echo "BUILDING Tool Gear's Xml interface binaries"; \
		echo "-------------------------------------------"; \
		cd ..; src/Utils/patchTGROOT bin/TGui "`pwd`" ; cd src; \
		cd Xmlserver ; \
		qmake tgxmlserver.pro; \
		${MAKE} ; \
	fi ;

TGmpip2xml:
	@if [ -f Mpipview/tgmpip2xml.pro ]; then \
		echo "------------------------------"; \
		echo "BUILDING MpiPView binaries"; \
		echo "------------------------------"; \
		cd ..; src/Utils/patchTGROOT bin/mpipview "`pwd`" ; cd src; \
		cd Mpipview ; \
		qmake tgmpip2xml.pro; \
		${MAKE} ; \
	fi ;


TGmemcheck2xml:
	@if [ -f Memcheckview/tgmemcheck2xml.pro ]; then \
		echo "------------------------------"; \
		echo "BUILDING MemcheckView binaries"; \
		echo "------------------------------"; \
		cd ..; src/Utils/patchTGROOT bin/memcheckview "`pwd`" ; \
		src/Utils/patchTGROOT bin/memcheck "`pwd`" ; \
		src/Utils/patchTGROOT bin/memcheck_all "`pwd`" ; cd src; \
		cd Memcheckview ; \
		qmake tgmemcheck2xml.pro; \
		${MAKE} ; \
	fi ;

umpireview_script:
	@if [ -f ../bin/umpireview ]; then \
		echo "------------------------------"; \
		echo "Patching Umpire View Script"; \
		echo "------------------------------"; \
		cd ..; src/Utils/patchTGROOT bin/umpireview "`pwd`" ; cd src; \
	fi ;

dynTGBinaries:
	@if [ -f dynTG/Makefile -a "`uname`" = "AIX" ]; then \
             if [ -d /usr/lpp/ppe.dpcl/include ]; then \
		echo "---------------------------------------------------------"; \
		echo "BUILDING dynTG binaries (currently requires AIX and DPCL)"; \
		echo "---------------------------------------------------------"; \
		cd ..; src/Utils/patchTGROOT bin/dyntg "`pwd`" ; cd src; \
		cd dynTG; \
		${MAKE};  \
             else \
		echo "-----------------------------------------------------------------"; \
		echo "NOT BUILDING dynTG binaries (DPCL not found in /usr/lpp/ppe.dpcl)"; \
		echo "-----------------------------------------------------------------"; \
	     fi ; \
	fi ;

install: all
	@echo "-----------------------------------------------------------------"; \
	echo "Recursively changing permissions to make world readable/executable:"; \
	cd ..; echo "  " chmod -R go+rX `pwd` ; \
	chmod -R go+rX .; echo " "; \
	echo "After chmod -R go+rX:"; echo " "; \
	ls -ld `pwd`; echo " "; ls -ld `pwd`/*; echo " "; ls -ld `pwd`/bin/*; \
	echo " "; \
	cd bin; \
	./install_links; \
	echo "-----------------------------------------------------------------"; \


################################################################################
# COPYRIGHT AND LICENSE
# 
# Copyright (c) 2006, The Regents of the University of California.
# Produced at the Lawrence Livermore National Laboratory
# Written by John Gyllenhaal (gyllen@llnl.gov), John May (johnmay@llnl.gov),
# and Martin Schulz (schulz6@llnl.gov).
# UCRL-CODE-220834.
# All rights reserved.
# 
# This file is part of Tool Gear.  For details, see www.llnl.gov/CASC/tool_gear.
# 
# Redistribution and use in source and binary forms, with or
# without modification, are permitted provided that the following
# conditions are met:
# 
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the disclaimer below.
# 
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the disclaimer (as noted below) in
#   the documentation and/or other materials provided with the distribution.
# 
# * Neither the name of the UC/LLNL nor the names of its contributors may
#   be used to endorse or promote products derived from this software without
#   specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OF THE UNIVERSITY 
# OF CALIFORNIA, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE 
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# ADDITIONAL BSD NOTICE
# 
# 1. This notice is required to be provided under our contract with the 
#    U.S. Department of Energy (DOE). This work was produced at the 
#    University of California, Lawrence Livermore National Laboratory 
#    under Contract No. W-7405-ENG-48 with the DOE.
# 
# 2. Neither the United States Government nor the University of California 
#    nor any of their employees, makes any warranty, express or implied, 
#    or assumes any liability or responsibility for the accuracy, completeness,
#    or usefulness of any information, apparatus, product, or process disclosed,
#    or represents that its use would not infringe privately-owned rights.
# 
# 3. Also, reference herein to any specific commercial products, process,
#    or services by trade name, trademark, manufacturer or otherwise does not
#    necessarily constitute or imply its endorsement, recommendation, or
#    favoring by the United States Government or the University of California.
#    The views and opinions of authors expressed herein do not necessarily
#    state or reflect those of the United States Government or the University
#    of California, and shall not be used for advertising or product
#    endorsement purposes.
################################################################################

