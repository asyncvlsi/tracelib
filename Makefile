#-------------------------------------------------------------------------
#
#  Copyright (c) 2022 Rajit Manohar
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA  02110-1301, USA.
#
#-------------------------------------------------------------------------

LIB=libtracelib_$(EXT).a
SHLIB1=libtrace_vcd_$(EXT).so
SHLIB2=libtrace_lxt2_$(EXT).so
SHLIB3=libtrace_atr_$(EXT).so

TARGETLIBS=$(LIB) $(SHLIB1) $(SHLIB2) $(SHLIB3)

OBJS=trace.o
SHOBJS1=vcd.os
SHOBJS2=lxt2.os ext/lxt2_write.os
SHOBJS3=atr.os

SRCS=$(SHOBJS1:.os=.cc) $(OBJS:.o=.c) lxt2.c atr.c

SUBDIRS=ext

include $(ACT_HOME)/scripts/Makefile.std

$(LIB): $(OBJS)
	ar ruv $(LIB) $(OBJS)
	$(RANLIB) $(LIB)

$(SHLIB1): $(SHOBJS1)
	$(ACT_HOME)/scripts/linkso $(SHLIB1) $(SHOBJS1) $(SHLIBCOMMON)

$(SHLIB2): $(SHOBJS2)
	$(ACT_HOME)/scripts/linkso $(SHLIB2) $(SHOBJS2) -lz

$(SHLIB3): $(SHOBJS3)
	$(ACT_HOME)/scripts/linkso $(SHLIB3) $(SHOBJS3) $(SHLIBCOMMON)

-include Makefile.deps

