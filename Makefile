# Makefile for the "nip" project.
# $Id: Makefile,v 1.50 2006-11-10 18:04:42 jatoivol Exp $

# Variable assignments for make
# XXX Replace "*.c" below with the names of your source files!
POT_SRCS=potential.c errorhandler.c
CLI_SRCS=$(POT_SRCS) variable.c clique.c Heap.c
GRPH_SRCS=$(CLI_SRCS) cls2clq.c Graph.c
PAR_SRCS=$(GRPH_SRCS) fileio.c parser.c

HUG_DEFS=huginnet.y
HUG_SRCS=$(HUG_DEFS:.y=.tab.c)

BIS_SRCS=$(HUG_SRCS) $(PAR_SRCS)
IO_SRCS=fileio.c errorhandler.c
DF_SRCS=$(PAR_SRCS)
HMM_SRCS=nip.c $(HUG_SRCS) $(PAR_SRCS)
HTM_SRCS=$(HMM_SRCS)
JNT_SRCS=$(HMM_SRCS)
EM_SRCS=$(HMM_SRCS)
MLT_SRCS=$(HMM_SRCS)
GEN_SRCS=$(HMM_SRCS)
MAP_SRCS=$(HMM_SRCS)
INF_SRCS=$(HMM_SRCS)
CONV_SRCS=$(HMM_SRCS)

# XXX Replace "cliquetest" below with the name you want for your program!
POT_TARGET=potentialtest
CLI_TARGET=cliquetest
PAR_TARGET=parsertest
GRPH_TARGET=graph_test
IO_TARGET=iotest
BIS_TARGET=bisontest
DF_TARGET=datafiletest
HMM_TARGET=hmmtest
HTM_TARGET=htmtest
JNT_TARGET=joint_test
EM_TARGET=em_test
MLT_TARGET=memleaktest
GEN_TARGET=gen_test
MAP_TARGET=maptest
INF_TARGET=inftest
CONV_TARGET=convert
TARGET=$(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) \
$(BIS_TARGET) $(IO_TARGET) $(DF_TARGET) $(HMM_TARGET) $(HTM_TARGET) \
$(MLT_TARGET) $(JNT_TARGET) $(EM_TARGET) $(GEN_TARGET) $(MAP_TARGET) \
$(INF_TARGET) $(CONV_TARGET)

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
HTM_OBJS=$(HTM_SRCS:.c=.o) htmtest.o
MLT_OBJS=$(MLT_SRCS:.c=.o) memleaktest.o
JNT_OBJS=$(JNT_SRCS:.c=.o) joint_test.o
EM_OBJS=$(EM_SRCS:.c=.o) em_test.o
GEN_OBJS=$(GEN_SRCS:.c=.o) gen_test.o
MAP_OBJS=$(MAP_SRCS:.c=.o) maptest.o
INF_OBJS=$(INF_SRCS:.c=.o) inftest.o
CONV_OBJS=$(CONV_SRCS:.c=.o) convert.o
OBJS=$(HTM_OBJS) potentialtest.o cliquetest.o parsertest.o graph_test.o \
bisontest.o iotest.o datafiletest.o hmmtest.o memleaktest.o joint_test.o \
em_test.o gen_test.o maptest.o inftest.o convert.o

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


# Rules for make
# The first rule tells make what to do by default: compile the program
# given by the variable TARGET (which was set above).
# NOTE the tab character! Syntax of a rule:
# <target>: <dependencies>
# \t<command>
all: $(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) $(BIS_TARGET) \
$(IO_TARGET) $(DF_TARGET) $(HMM_TARGET) $(HTM_TARGET) $(MLT_TARGET) \
$(JNT_TARGET) $(EM_TARGET) $(GEN_TARGET) $(MAP_TARGET) $(INF_TARGET) \
$(CONV_TARGET)

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


# With these lines, executing "make clean" removes the .o files that
# are not needed after the program is compiled.
clean:
	rm -f huginnet.tab.c huginnet.tab.h *.o *.i *.s

# "make realclean" does the same as "make clean", and also removes the
# compiled program and a possible "core" file.
realclean: clean
	rm -f $(TARGET) core

# Tells make that all, clean and realclean are not the names of
# programs to compile.
.PHONY: all clean realclean
