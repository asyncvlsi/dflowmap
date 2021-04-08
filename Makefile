#-------------------------------------------------------------------------
#
#  Copyright (c) 2020 Rajit Manohar
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
EXE=dflowmap.$(EXT)

TARGETS=$(EXE)
TARGETINCS=ChpGenerator.h Metrics.h ChpProcGenerator.h
#TARGETINCSUBDIR=act

OBJS=ChpGenerator.o main.o Metrics.o ChpProcGenerator.o

SRCS=$(OBJS:.o=.cc)

include $(VLSI_TOOLS_SRC)/scripts/Makefile.std

CXX += -g

$(EXE): $(OBJS) $(ACTPASSDEPEND)
	$(CXX) $(CFLAGS) $(OBJS) -o $(EXE) $(LIBACTPASS) -ldl -ledit

-include Makefile.deps
