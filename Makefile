#-------------------------------------------------------------------------
#
#  Copyright (c) 2023 Rajit Manohar
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
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
TARGETINCS=tracelib.h
TARGETINCSUBDIR=act

OBJS1=tracelib.o
SHOBJS1=vcd.os
SHOBJS2=lxt2.os ext/lxt2_write.os
SHOBJS3=atr.os

OBJS=$(OBJS1) $(SHOBJS1) $(SHOBJS2) $(SHOBJS3)

SRCS=$(SHOBJS1:.os=.cc) $(OBJS1:.o=.c) lxt2.c atr.c

SUBDIRS=ext

include $(ACT_HOME)/scripts/Makefile.std

CFLAGS+=-DACT_MODE -DTRACELIB_ENV=\"ACT_HOME\"

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

