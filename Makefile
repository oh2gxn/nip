# Makefile for the "nip" project.
#
# FIXME & TODO: all kinds of stuff !!!
# - make a library out of the basic source files
# - make test programs separately
# - make utility programs separately
#
# $Id: Makefile,v 1.55 2008-12-20 13:28:39 jatoivol Exp $

# Variable assignments for make
# XXX Replace "*.c" below with the names of your source files!
POT_SRCS=src/potential.c src/errorhandler.c
CLI_SRCS=$(POT_SRCS) src/variable.c src/clique.c src/Heap.c
GRPH_SRCS=$(CLI_SRCS) src/cls2clq.c src/Graph.c

HUG_DEFS=src/huginnet.y
HUG_SRCS=$(HUG_DEFS:.y=.tab.c)

PAR_SRCS=$(GRPH_SRCS) $(HUG_SRCS) src/lists.c src/fileio.c src/parser.c
BIS_SRCS=$(PAR_SRCS)
IO_SRCS=src/fileio.c src/errorhandler.c
DF_SRCS=$(PAR_SRCS)
HMM_SRCS=nip.c $(PAR_SRCS)
HTM_SRCS=$(HMM_SRCS)
JNT_SRCS=$(HMM_SRCS)
EM_SRCS=$(HMM_SRCS)
MLT_SRCS=$(HMM_SRCS)
GEN_SRCS=$(HMM_SRCS)
MAP_SRCS=$(HMM_SRCS)
INF_SRCS=$(HMM_SRCS)
CONV_SRCS=$(HMM_SRCS)
LIKE_SRCS=$(HMM_SRCS)
LOO_SRCS=$(HMM_SRCS)

# XXX Replace "cliquetest" below with the name you want for your program!
POT_TARGET=test/potentialtest
CLI_TARGET=test/cliquetest
PAR_TARGET=test/parsertest
GRPH_TARGET=test/graph_test
IO_TARGET=test/iotest
BIS_TARGET=test/bisontest
DF_TARGET=test/datafiletest
HMM_TARGET=test/hmmtest
HTM_TARGET=test/htmtest
MLT_TARGET=test/memleaktest

JNT_TARGET=util/joint_test
EM_TARGET=util/em_test
GEN_TARGET=util/gen_test
MAP_TARGET=util/map
INF_TARGET=util/inftest
CONV_TARGET=util/convert
LIKE_TARGET=util/likelihood
LOO_TARGET=util/loo_prediction_test
TARGET=$(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) \
$(BIS_TARGET) $(IO_TARGET) $(DF_TARGET) $(HMM_TARGET) $(HTM_TARGET) \
$(MLT_TARGET) $(JNT_TARGET) $(EM_TARGET) $(GEN_TARGET) $(MAP_TARGET) \
$(INF_TARGET) $(CONV_TARGET) $(LIKE_TARGET) $(LOO_TARGET)

# Sets the name and some flags for the C compiler and linker
CC=gcc
CCFLAGS=-c
#CFLAGS=-O2 -Wall
CFLAGS=-g -pedantic-errors -Wall
#CFLAGS=-Os -g -Wall -ansi -pedantic-errors
#CFLAGS=-O2 -g -Wall
#CFLAGS=-g -Wall --save-temps
#CFLAGS=-Wall
LD=gcc
LDFLAGS=
#LDFLAGS=-v
YY=bison
YYFLAGS=-d

# Link the math library in with the program, in case you use the
# functions in <math.h>
LIBS=-lm
NIPLIBS=-lm -lnip

# This gives make the names of object files made by the compiler and
# used by the linker.
POT_OBJS=$(POT_SRCS:.c=.o) src/potentialtest.o
CLI_OBJS=$(GRPH_SRCS:.c=.o) src/cliquetest.o # had to put the Graph srcs
PAR_OBJS=$(PAR_SRCS:.c=.o) src/parsertest.o
GRPH_OBJS=$(GRPH_SRCS:.c=.o) src/graph_test.o
HUG_OBJS=$(HUG_SRCS:.c=.o) 
BIS_OBJS=$(BIS_SRCS:.c=.o) src/bisontest.o
IO_OBJS=$(IO_SRCS:.c=.o) src/iotest.o
DF_OBJS=$(DF_SRCS:.c=.o) src/datafiletest.o
HMM_OBJS=$(HMM_SRCS:.c=.o) src/hmmtest.o
HTM_OBJS=$(HTM_SRCS:.c=.o) src/htmtest.o
MLT_OBJS=$(MLT_SRCS:.c=.o) src/memleaktest.o
JNT_OBJS=$(JNT_SRCS:.c=.o) src/joint_test.o
EM_OBJS=$(EM_SRCS:.c=.o) src/em_test.o
GEN_OBJS=$(GEN_SRCS:.c=.o) src/gen_test.o
MAP_OBJS=$(MAP_SRCS:.c=.o) src/maptest.o
INF_OBJS=$(INF_SRCS:.c=.o) src/inftest.o
CONV_OBJS=$(CONV_SRCS:.c=.o) src/convert.o
LIKE_OBJS=$(LIKE_SRCS:.c=.o) src/likelihood.o
LOO_OBJS=$(LOO_SRCS:.c=.o) src/loo_prediction_test.o
OBJS=$(HTM_OBJS) src/potentialtest.o src/cliquetest.o src/parsertest.o \
src/graph_test.o src/bisontest.o src/iotest.o src/datafiletest.o \
src/hmmtest.o src/memleaktest.o src/joint_test.o src/em_test.o \
src/gen_test.o src/maptest.o src/inftest.o src/convert.o src/likelihood.o \
src/loo_prediction_test.o

# Headers
POT_HDRS=$(POT_SRCS:.c=.h)
CLI_HDRS=$(GRPH_SRCS:.c=.h) # had to put the Graph srcs
PAR_HDRS=$(PAR_SRCS:.c=.h)
GRPH_HDRS=$(GRPH_SRCS:.c=.h)
#HUG_HDRS - no such thing
BIS_HDRS=$(BIS_SRCS:.c=.h)
IO_HDRS=$(IO_SRCS:.c=.h)
DF_HDRS=$(DF_SRCS:.c=.h)
HMM_HDRS=$(HMM_SRCS:.c=.h)
HTM_HDRS=$(HTM_SRCS:.c=.h)
MLT_HDRS=$(MLT_SRCS:.c=.h)
JNT_HDRS=$(JNT_SRCS:.c=.h)
EM_HDRS=$(EM_SRCS:.c=.h)
GEN_HDRS=$(GEN_SRCS:.c=.h)
MAP_HDRS=$(MAP_SRCS:.c=.h)
INF_HDRS=$(INF_SRCS:.c=.h)
CONV_HDRS=$(CONV_SRCS:.c=.h)
LIKE_HDRS=$(LIKE_SRCS:.c=.h)
LOO_HDRS=$(LOO_SRCS:.c=.h)


# Rules for make
# The first rule tells make what to do by default: compile the program
# given by the variable TARGET (which was set above).
# NOTE the tab character! Syntax of a rule:
# <target>: <dependencies>
# \t<command>
all: $(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) $(BIS_TARGET) \
$(IO_TARGET) $(DF_TARGET) $(HMM_TARGET) $(HTM_TARGET) $(MLT_TARGET) \
$(JNT_TARGET) $(EM_TARGET) $(GEN_TARGET) $(MAP_TARGET) $(INF_TARGET) \
$(CONV_TARGET) $(LIKE_TARGET) $(LOO_TARGET)

# Object files depend on headers also
%.o: %.c %.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

# How to use Bison parser generator
%.tab.c %.tab.h: %.y
	$(YY) $(YYFLAGS) $<

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

$(HTM_TARGET): $(HTM_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(HTM_OBJS) $(LIBS)

$(MLT_TARGET): $(MLT_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(MLT_OBJS) $(LIBS)

$(JNT_TARGET): $(JNT_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(JNT_OBJS) $(LIBS)

$(EM_TARGET): $(EM_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(EM_OBJS) $(LIBS)

$(GEN_TARGET): $(GEN_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(GEN_OBJS) $(LIBS)

$(MAP_TARGET): $(MAP_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(MAP_OBJS) $(LIBS)

$(INF_TARGET): $(INF_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(INF_OBJS) $(LIBS)

$(CONV_TARGET): $(CONV_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(CONV_OBJS) $(LIBS)

$(LIKE_TARGET): $(LIKE_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(LIKE_OBJS) $(LIBS)

$(LOO_TARGET): $(LOO_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(LOO_OBJS) $(LIBS)


# With these lines, executing "make clean" removes the .o files that
# are not needed after the program is compiled.
clean:
	rm -f src/huginnet.tab.c src/huginnet.tab.h src/*.o src/*.i src/*.s

# "make realclean" does the same as "make clean", and also removes the
# compiled program and a possible "core" file.
realclean: clean
	rm -f $(TARGET) core

# Tells make that all, clean and realclean are not the names of
# programs to compile.
.PHONY: all clean realclean
