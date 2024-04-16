#
#   Copyright (C) 2021-2023 Free Software Foundation, Inc.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.

CC       = gcc
WARNINGS = -Wall -Werror=undef -Wstrict-prototypes
OPT      = -g -O
CFLAGS   = $(OPT) $(WARNINGS)
LDFLAGS  =
LIBS     = -lm -lpthread
OBJDIR   = ../objects
BINDIR   = ../bindir
EXPDIR   = ../experiments

EXE     = mxv-pthreads
OBJECTS = $(OBJDIR)/main.o $(OBJDIR)/manage_data.o $(OBJDIR)/workload.o $(OBJDIR)/mxv.o

default: $(BINDIR)/$(EXE)

$(BINDIR)/$(EXE): $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CC) -o $(BINDIR)/$(EXE) $(LDFLAGS) $(OBJECTS) $(LIBS)
	ldd $(BINDIR)/$(EXE)

$(OBJDIR)/main.o: main.c
	@mkdir -p $(OBJDIR)
	$(CC) -o $(OBJDIR)/main.o -c $(CFLAGS) main.c
$(OBJDIR)/manage_data.o: manage_data.c
	@mkdir -p $(OBJDIR)
	$(CC) -o $(OBJDIR)/manage_data.o -c $(CFLAGS) manage_data.c
$(OBJDIR)/workload.o: workload.c
	@mkdir -p $(OBJDIR)
	$(CC) -o $(OBJDIR)/workload.o -c $(CFLAGS) workload.c
$(OBJDIR)/mxv.o: mxv.c
	@mkdir -p $(OBJDIR)
	$(CC) -o $(OBJDIR)/mxv.o -c $(CFLAGS) mxv.c

$(OBJECTS): mydefs.h

.c.o:
	$(CC) -c -o $@ $(CFLAGS) $<

check:
	@echo "Running $(EXE) in $(EXPDIR)"
	@./$(EXPDIR)/$(EXE) -m 1000 -n 1500 -t 2

install: $(BINDIR)/$(EXE)
	@/bin/cp $(BINDIR)/$(EXE) $(EXPDIR)
	@echo "Installed $(EXE) in $(EXPDIR)"

clean:
	@/bin/rm -f $(BINDIR)/$(EXE)
	@/bin/rm -f $(OBJECTS)

veryclean:
	@make clean
	@/bin/rm -f $(EXPDIR)/$(EXE)
