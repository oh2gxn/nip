# Makefile for the "nip" project.
# $Id: Makefile,v 1.24 2004-06-29 13:15:33 jatoivol Exp $

# Variable assignments for make
# XXX Replace "*.c" below with the names of your source files!
POT_SRCS=potential.c errorhandler.c
CLI_SRCS=$(POT_SRCS) Variable.c Clique.c
GRPH_SRCS=$(CLI_SRCS) grphmnp/cls2clq.c grphmnp/Heap.c Graph.c
PAR_SRCS=$(GRPH_SRCS) fileio.c parser.c
HUG_DEFS=huginnet.y
HUG_SRCS=$(HUG_DEFS:.y=.tab.c)
BIS_SRCS=$(HUG_SRCS) $(PAR_SRCS)
IO_SRCS=fileio.c errorhandler.c
DF_SRCS=$(PAR_SRCS)
HMM_SRCS=$(BIS_SRCS)

# XXX Replace "cliquetest" below with the name you want for your program!
POT_TARGET=potentialtest
CLI_TARGET=cliquetest
PAR_TARGET=parsertest
GRPH_TARGET=graph_test
IO_TARGET=iotest
BIS_TARGET=bisontest
DF_TARGET=datafiletest
HMM_TARGET=hmmtest
TARGET=$(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) $(BIS_TARGET) $(IO_TARGET) $(DF_TARGET) $(HMM_TARGET)

# You should not need to modify anything below this line...
# Sets the name and some flags for the C compiler and linker
CC=gcc
CFLAGS=-O2 -g -Wall
#CFLAGS=-g -Wall --save-temps
#CFLAGS=-Wall
LD=gcc
LDFLAGS=
#LDFLAGS=-v
YY=bison
YYFLAGS=-v

# Link the math library in with the program, in case you use the
# functions in <math.h>
#LIBS=-lm
LIBS=

# This gives make the names of object files made by the compiler and
# used by the linker.
POT_OBJS=$(POT_SRCS:.c=.o) potentialtest.o
CLI_OBJS=$(GRPH_SRCS:.c=.o) cliquetest.o # had to put the Graph srcs
PAR_OBJS=$(PAR_SRCS:.c=.o) parsertest.o
GRPH_OBJS=$(GRPH_SRCS:.c=.o) graph_test.o
HUG_OBJS=$(HUG_SRCS:.c=.o) 
BIS_OBJS=$(BIS_SRCS:.c=.o) bisontest.o
IO_OBJS=$(IO_SRCS:.c=.o) iotest.o
DF_OBJS=$(DF_SRCS:.c=.o) datafiletest.o
HMM_OBJS=$(HMM_SRCS:.c=.o) hmmtest.o
OBJS=$(BIS_OBJS) potentialtest.o cliquetest.o parsertest.o graph_test.o iotest.o datafiletest.o hmmtest.o

# Rules for make
# The first rule tells make what to do by default: compile the program
# given by the variable TARGET (which was set above).
# NOTE the tab character! Syntax of a rule:
# <target>: <dependencies>
# \t<command>
all: $(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) $(BIS_TARGET) $(IO_TARGET) $(DF_TARGET) $(HMM_TARGET)

# The program depends on the object files in $(OBJS). Make knows how
# to compile a .c file into an object (.o) file; this rule tells it
# how to link the .o files together into an executable program.
$(POT_TARGET): $(POT_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(POT_OBJS) $(LIBS)

$(CLI_TARGET): $(CLI_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(CLI_OBJS) $(LIBS)

$(PAR_TARGET): $(PAR_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(PAR_OBJS) $(LIBS)

$(GRPH_TARGET): $(GRPH_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(GRPH_OBJS) $(LIBS)

$(BIS_TARGET): $(BIS_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(BIS_OBJS) $(LIBS)

$(IO_TARGET): $(IO_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(IO_OBJS) $(LIBS)

$(DF_TARGET): $(DF_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(DF_OBJS) $(LIBS)

$(HMM_TARGET): $(HMM_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(HMM_OBJS) $(LIBS)

$(HUG_SRCS): $(HUG_DEFS)
	$(YY) $(YYFLAGS) $(HUG_DEFS)

# With these lines, executing "make clean" removes the .o files that
# are not needed after the program is compiled.
clean:
	rm -f $(OBJS) $(HUG_SRCS) *.i *.s

# "make realclean" does the same as "make clean", and also removes the
# compiled program and a possible "core" file.
realclean: clean
	rm -f $(TARGET) core

# Tells make that all, clean and realclean are not the names of
# programs to compile.
.PHONY: all clean realclean
