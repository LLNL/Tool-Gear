# Makefile for Tool Gear collectors
# **************************************************************************
#  Tool Gear (www.llnl.gov/CASC/tool_gear)
#  Version 2.00                                              March 29, 2006
#  Please see COPYRIGHT AND LICENSE information at the end of this file.
# **************************************************************************

all: dynTG

CCX =		xlC_r -O2 -qstaticinline
CC =		xlc_r -O2
SRC_DIR = ../..
BIN_DIR = ../../../bin
CUR_DIR := `pwd`
DPCL =		/usr/lpp/ppe.dpcl
DPCL_INC =	$(DPCL)/include
DPCL_LIB =	$(DPCL)/lib
BOOST_INC =	$(SRC_DIR)
EXT_DIR := ${shell ./printExtDir}


VERBOSE = # -bloadmap:PARM -bnoquiet

# Since the rest of Tool Gear is compiled with the compiler QT is compiled
# with, we don't want to use those .o files.   Use vpath to allow the source
# to be in different directories but the .o files to be build in this directory
# Use lowercase vpath to limit other directories for source files only,
# not object files.
vpath %.c .:$(SRC_DIR)/Utils
vpath %.cpp .:$(SRC_DIR)/Utils

	 
TGC_OBJECTS = dpcl_run_app.o dpcl_callbacks.o \
	      parse_program.o \
              collector_pack.o tg_socket.o dpcl_socket.o \
	      dpcl_instrument_app.o \
              tg_pack.o action.o dpcl_action_point.o \
              dpcl_action_type.o dpcl_action_instance.o \
              tg_globals.o tg_time.o \
              tg_swapbytes.o tg_error.o \
              messagebuffer.o \
              tg_source_reader.o search_path.o \
              string_symbol.o l_alloc_new.o dpcl_error.o

# Union of all the headers used by the dpcl collector files
COLLECTOR_HEADERS = 	action.h base.h breakpoint_actions.h cacheinst.h \
			dpcl_action_instance.h dpcl_action_point.h \
			dpcl_action_type.h dpcl_callbacks.h \
			dpcl_instrument_app.h dpcl_run_app.h dpcl_socket.h \
			memory_actions.h memorymodule.h mpip_actions.h \
			mpx_actions.h parse_program.h tg_globals.h \
			timer_actions.h timermodule.h \
			$(SRC_DIR)/Utils/tg_typetags.h


# Union of all the headers used by Utils files
UTILS_HEADERS =  $(SRC_DIR)/Utils/collector_pack.h \
		 $(SRC_DIR)/Utils/command_tags.h \
		 $(SRC_DIR)/Utils/tg_inst_point.h \
		 $(SRC_DIR)/Utils/tg_pack.h \
		 $(SRC_DIR)/Utils/tg_socket.h \
		 $(SRC_DIR)/Utils/tg_error.h \
		 $(SRC_DIR)/Utils/tg_swapbytes.h \
		 $(SRC_DIR)/Utils/tg_time.h \
		 $(SRC_DIR)/Utils/tg_source_reader.h \
		 $(SRC_DIR)/Utils/messagebuffer.h \
		 $(SRC_DIR)/Utils/tempcharbuf.h \
		 $(SRC_DIR)/Utils/search_path.h \
		 $(SRC_DIR)/Utils/string_symbol.h \
		 $(SRC_DIR)/Utils/l_alloc_new.h 


.SUFFIXES: .cpp

.cpp.o: 
	$(CCX) -q32 -c -I. -I$(SRC_DIR)/Utils -I$(DPCL_INC) -I$(BOOST_INC) -qnofullpath -o $@ $<
.c.o:
	$(CC) -c -q32 -I. -I$(SRC_DIR)/Utils -qnofullpath -o $@ $<

# Set dependences to rebuild all .o files if any header changes.
$(TGC_OBJECTS): $(UTILS_HEADERS) $(COLLECTOR_HEADERS)


${EXT_DIR}/dpclcol.so: $(TGC_OBJECTS)
	$(CCX) -bM:SRE -G -q32 -qnofullpath -bexpall -bnoentry -o $@ $(TGC_OBJECTS) $(VERBOSE) -L$(EXT_DIR)

$(BIN_DIR)/dpclcol: ${EXT_DIR}/dpclcol.so dpcl_collector.o 
	$(CCX) -brtl -q32 -o $@ -L$(EXT_DIR) $(EXT_DIR)/dpclcol.so dpcl_collector.o $(DPCL_LIB)/libdpcl.a $(VERBOSE)

dynTG: $(BIN_DIR)/dpclcol 

clean:
	rm -f *.o *.a 

clobber: clean
	rm *~

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

