# qmake input for TGclient.  Process with qmake.
# **************************************************************************
#  Tool Gear (www.llnl.gov/CASC/tool_gear)
#  Version 2.00                                              March 29, 2006
#  Please see COPYRIGHT AND LICENSE information at the end of this file.
# **************************************************************************
SOURCES += gui_main.cpp cellgrid.cpp ../Utils/md.c ../Utils/l_alloc_new.c \
        uimanager.cpp ../Utils/int_symbol.c ../Utils/int_array_symbol.c \
        ../Utils/index_symbol.c \
        ../Utils/string_symbol.c ../Utils/tg_socket.c gui_socket_reader.cpp \
	gui_action_sender.cpp ../Utils/heapsort.c \
	tg_collector.cpp ../Utils/tg_error.c \
	tg_parse_opts.cpp tg_gui_listener.cpp celllayout.cpp \
	treeview.cpp mainview.cpp messageview.cpp \
	tracebackview.cpp tabtracebackview.cpp \
        messageviewer.cpp \
	cellgrid_searcher.cpp filecollection.cpp \
	../Utils/tg_pack.cpp ../Utils/tg_time.c ../Utils/messagebuffer.cpp \
        ../Utils/command_tags.cpp \
	../Utils/tg_swapbytes.c ./Dialogs/inst_dialog.cpp \
	./Dialogs/search_path_dialog.cpp ./Dialogs/drag_list_view.cpp \
	./Dialogs/dir_view_item.cpp  ./Dialogs/path_view_item.cpp \
	./Dialogs/search_dialog.cpp ./Dialogs/search_list_button.cpp

HEADERS += cellannot.h ../Utils/stringtable.h  cellgrid.h \
../Utils/intarraytable.h tg_collector.h ../Utils/index_symbol.h \
celllayout.h ../Utils/intset.h tg_gui_listener.h ../Utils/tabledef.h \
../Utils/inttable.h tg_parse_opts.h datastats.h ../Utils/l_alloc_new.h \
tg_program_state.h ../Utils/l_punt.h ../Utils/tg_types.h \
gui_action_sender.h mainview.h \
messageview.h messageviewer.h tracebackview.h tabtracebackview.h \
gui_socket_reader.h ../Utils/md.h ../Utils/heapsort.h \
treeview.h ../Utils/int_array_symbol.h ../Utils/string_symbol.h uimanager.h \
cellgrid_searcher.h filecollection.h \
../Utils/command_tags.h ../Utils/tg_pack.h ../Utils/tg_time.h \
../Utils/tg_error.h ../Utils/tg_socket.h ../Utils/tg_typetags.h \
../Utils/tg_inst_point.h ../Utils/tg_swapbytes.h \
../Utils/messagebuffer.h \
./Dialogs/inst_dialog.h ./Dialogs/search_path_dialog.h \
./Dialogs/drag_list_view.h ./Dialogs/dir_view_item.h \
./Dialogs/path_view_item.h ./Dialogs/search_dialog.h ../Utils/inttoindex.h  \
./Dialogs/search_list_button.h

FORMS = ./Dialogs/inst_dialog_base.ui ./Dialogs/search_path_dialog_base.ui \
./Dialogs/search_dialog_base.ui

DBFILE = ./Dialogs/dialogs.db

UI_DIR = ./Dialogs

#CONFIG += debug
CONFIG += warn_on
CONFIG += qt

DEFINES += USE_READ_THREAD

INCLUDEPATH += . ../Utils ./Dialogs

DEPENDPATH += . ../Utils ./Dialogs

TARGET = TGclient

# Put executable directly in Tool Gear's bin directory
DESTDIR = ../../bin

# Compile for threads (but we don't want libqt-mt, so we
# can't use the thread config)
!isEmpty(QMAKE_CC_THREAD): QMAKE_CC = $$QMAKE_CC_THREAD
!isEmpty(QMAKE_CXX_THREAD): QMAKE_CXX = $$QMAKE_CXX_THREAD
!isEmpty(QMAKE_LINK_THREAD): QMAKE_LINK = $$QMAKE_LINK_THREAD
QMAKE_CFLAGS += $$QMAKE_CFLAGS_THREAD
QMAKE_CXXFLAGS += $$QMAKE_CXXFLAGS_THREAD
QMAKE_LFLAGS += $$QMAKE_LFLAGS_THREAD
QMAKE_LIBS += $$QMAKE_LIBS_THREAD

#OSNAME = $$(OSTYPE)
OSNAME = $$system( uname -s )

contains( OSNAME, [Ss]olaris ) {
	message( "Building TGclient Makefile for SunOS" )
	DEFINES += TG_SUN
	DEFINES += TG_SSH_COMMAND=\"/usr/local/bin/ssh\"
}
contains( OSNAME, [Dd]arwin ) {
	message( "Building TGclient Makefile for Macintosh OS X" )
	DEFINES += TG_MAC
# Need extra level of backslash quoting when generating Xcode file
# Be sure to change both versions when editing!
	contains( MAKEFILE_GENERATOR, PROJECTBUILDER ) {
		DEFINES += TG_SSH_COMMAND=\\"/usr/bin/ssh\\"
	} else {
		DEFINES += TG_SSH_COMMAND=\"/usr/bin/ssh\"
	}
}
contains( OSNAME, [Aa][Ii][Xx] ) {
	message( "Building TGclient Makefile for AIX" )
	DEFINES += TG_AIX
	QMAKE_CXXFLAGS += -qstaticinline -qcheck
        QMAKE_LFLAGS += -qcheck
	DEFINES += TG_SSH_COMMAND=\"/usr/local/bin/ssh\"
}
contains( OSNAME, [Ll]inux ) {
	message( "Building TGclient Makefile for Linux" )
	DEFINES += TG_LINUX
	DEFINES += TG_SSH_COMMAND=\"/usr/bin/ssh\"
}
contains( OSNAME, [Oo][Ss][Ff]1 ) {
	message( "Building TGclient Makefile for Tru64" )
	DEFINES += TG_TRU64
	DEFINES += TG_SSH_COMMAND=\"/usr/local/bin/ssh\"
	# The following flag is supposed to be added automatically
	# when the QMAKE_CC_THREAD compiler invoked (i.e., cxx -pthread)
	# but apparently it doesn't happen.
	QMAKE_CFLAGS += -D_REENTRANT
}
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

