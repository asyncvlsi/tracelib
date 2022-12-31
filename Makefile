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
SHLIB1=libtracevcd_$(EXT).so

TARGETLIBS=$(LIB) $(SHLIB1)

OBJS=trace.o
SHOBJS1=vcd.os

SRCS=$(SHOBJS1:.os=.cc) $(OBJS:.o=.c)

include $(VLSI_TOOLS_SRC)/scripts/Makefile.std

$(LIB): $(OBJS)
	ar ruv $(LIB) $(OBJS)
	$(RANLIB) $(LIB)

$(SHLIB1): $(SHOBJS1)
	$(VLSI_TOOLS_SRC)/scripts/linkso $(SHLIB1) $(SHOBJS1) $(SHLIBCOMMON)

-include Makefile.deps

