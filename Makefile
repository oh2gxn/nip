# Makefile for the "nip" project.
#
# FIXME & TODO: all kinds of stuff !!!
# - make a library out of the basic source files
# - make test programs separately
# - make utility programs separately
#
# $Id: Makefile,v 1.57 2008-12-21 11:20:19 jatoivol Exp $


# Sets the name and some flags for the C compiler and linker
CC = gcc
CCFLAGS = -c
#CFLAGS=-O2 -Wall
CFLAGS = -g -pedantic-errors -Wall
#CFLAGS = -Os -g -Wall -ansi -pedantic-errors
#CFLAGS = -O2 -g -Wall
#CFLAGS = -g -Wall --save-temps
#CFLAGS = -Wall
LD = gcc
LDFLAGS =
#LDFLAGS = -v
YY = bison
YYFLAGS = -d
AR = ar

# Link the math library in with the program, in case you use the
# functions in <math.h>
LIBS = -lm
NIPLIBS = -lm -L./lib -lnip


# Object files depend on headers also, FIXME !
#%.o: %.c %.h
#	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@
%.o: %.c %.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

# How to use Bison parser generator
HUG_DEFS = src/huginnet.y
HUG_SRCS = $(HUG_DEFS:.y=.tab.c)
HUG_HDRS = $(HUG_DEFS:.y=.tab.h)
$(HUG_SRCS) $(HUG_HDRS): $(HUG_DEFS)
	$(YY) $(YYFLAGS) $<

# How to create the static and shared libraries

LIB_SRCS = src/fileio.c \
src/errorhandler.c \
src/potential.c \
src/variable.c \
src/clique.c \
src/Heap.c \
src/cls2clq.c \
src/Graph.c \
$(HUG_SRCS) \
src/lists.c \
src/fileio.c \
src/parser.c \
src/nip.c
LIB_HDRS=$(LIB_SRCS:.c=.h)
LIB_OBJS=$(LIB_SRCS:.c=.o)
lib: lib/libnip.a lib/libnip.so

lib/libnip.a: $(LIB_OBJS)
	$(AR) rcs lib/libnip.a nip.o

lib/libnip.so: $(LIB_OBJS)
	$(CC) -shared -Wl,-soname,libnip.so.1 -o libnip.so.1.0.1  nip.o



# Rules for making each program
IO_SRCS=src/iotest.c
IO_HDRS=$(IO_SRCS:.c=.h)
IO_OBJS=$(IO_SRCS:.c=.o)
IO_TARGET=test/iotest
$(IO_TARGET): $(IO_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(IO_OBJS) $(NIPLIBS)


POT_SRCS=src/potential.c src/errorhandler.c
POT_HDRS=$(POT_SRCS:.c=.h)
POT_OBJS=$(POT_SRCS:.c=.o) src/potentialtest.o
POT_TARGET=test/potentialtest
$(POT_TARGET): $(POT_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(POT_OBJS) $(LIBS)


CLI_SRCS=$(POT_SRCS) src/variable.c src/clique.c src/Heap.c
CLI_HDRS=$(GRPH_SRCS:.c=.h) # had to put the Graph srcs
CLI_OBJS=$(GRPH_SRCS:.c=.o) src/cliquetest.o # had to put the Graph srcs
CLI_TARGET=test/cliquetest
$(CLI_TARGET): $(CLI_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(CLI_OBJS) $(LIBS)


GRPH_SRCS=$(CLI_SRCS) src/cls2clq.c src/Graph.c
GRPH_HDRS=$(GRPH_SRCS:.c=.h)
GRPH_OBJS=$(GRPH_SRCS:.c=.o) src/graph_test.o
GRPH_TARGET=test/graph_test
$(GRPH_TARGET): $(GRPH_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(GRPH_OBJS) $(LIBS)



HUG_DEFS=src/huginnet.y
HUG_SRCS=$(HUG_DEFS:.y=.tab.c)
#HUG_HDRS - no such thing?
HUG_OBJS=$(HUG_SRCS:.c=.o) 
PAR_SRCS=$(GRPH_SRCS) $(HUG_SRCS) src/lists.c src/fileio.c src/parser.c
PAR_HDRS=$(PAR_SRCS:.c=.h)
PAR_OBJS=$(PAR_SRCS:.c=.o) src/parsertest.o
PAR_TARGET=test/parsertest
$(PAR_TARGET): $(PAR_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(PAR_OBJS) $(LIBS)


BIS_SRCS=$(PAR_SRCS)
BIS_HDRS=$(BIS_SRCS:.c=.h)
BIS_OBJS=$(BIS_SRCS:.c=.o) src/bisontest.o
BIS_TARGET=test/bisontest
$(BIS_TARGET): $(BIS_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(BIS_OBJS) $(LIBS)


DF_SRCS=$(PAR_SRCS)
DF_HDRS=$(DF_SRCS:.c=.h)
DF_OBJS=$(DF_SRCS:.c=.o) src/datafiletest.o
DF_TARGET=test/datafiletest
$(DF_TARGET): $(DF_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(DF_OBJS) $(LIBS)


HMM_SRCS=nip.c $(PAR_SRCS)
HMM_HDRS=$(HMM_SRCS:.c=.h)
HMM_OBJS=$(HMM_SRCS:.c=.o) src/hmmtest.o
HMM_TARGET=test/hmmtest
$(HMM_TARGET): $(HMM_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(HMM_OBJS) $(LIBS)

HTM_SRCS=$(HMM_SRCS)
HTM_HDRS=$(HTM_SRCS:.c=.h)
HTM_OBJS=$(HTM_SRCS:.c=.o) src/htmtest.o
HTM_TARGET=test/htmtest
$(HTM_TARGET): $(HTM_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(HTM_OBJS) $(LIBS)


MLT_SRCS=$(HMM_SRCS)
MLT_HDRS=$(MLT_SRCS:.c=.h)
MLT_OBJS=$(MLT_SRCS:.c=.o) src/memleaktest.o
MLT_TARGET=test/memleaktest
$(MLT_TARGET): $(MLT_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(MLT_OBJS) $(LIBS)




JNT_SRCS=$(HMM_SRCS)
JNT_HDRS=$(JNT_SRCS:.c=.h)
JNT_OBJS=$(JNT_SRCS:.c=.o) src/joint_test.o
JNT_TARGET=util/joint_test
$(JNT_TARGET): $(JNT_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(JNT_OBJS) $(LIBS)


EM_SRCS=$(HMM_SRCS)
EM_HDRS=$(EM_SRCS:.c=.h)
EM_OBJS=$(EM_SRCS:.c=.o) src/em_test.o
EM_TARGET=util/em_test
$(EM_TARGET): $(EM_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(EM_OBJS) $(LIBS)


GEN_SRCS=$(HMM_SRCS)
GEN_HDRS=$(GEN_SRCS:.c=.h)
GEN_OBJS=$(GEN_SRCS:.c=.o) src/gen_test.o
GEN_TARGET=util/gen_test
$(GEN_TARGET): $(GEN_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(GEN_OBJS) $(LIBS)


MAP_SRCS=$(HMM_SRCS)
MAP_HDRS=$(MAP_SRCS:.c=.h)
MAP_OBJS=$(MAP_SRCS:.c=.o) src/maptest.o
MAP_TARGET=util/map
$(MAP_TARGET): $(MAP_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(MAP_OBJS) $(LIBS)


INF_SRCS=$(HMM_SRCS)
INF_HDRS=$(INF_SRCS:.c=.h)
INF_OBJS=$(INF_SRCS:.c=.o) src/inftest.o
INF_TARGET=util/inftest
$(INF_TARGET): $(INF_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(INF_OBJS) $(LIBS)


CONV_SRCS=$(HMM_SRCS)
CONV_HDRS=$(CONV_SRCS:.c=.h)
CONV_OBJS=$(CONV_SRCS:.c=.o) src/convert.o
CONV_TARGET=util/convert
$(CONV_TARGET): $(CONV_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(CONV_OBJS) $(LIBS)


LIKE_SRCS=$(HMM_SRCS)
LIKE_HDRS=$(LIKE_SRCS:.c=.h)
LIKE_OBJS=$(LIKE_SRCS:.c=.o) src/likelihood.o
LIKE_TARGET=util/likelihood
$(LIKE_TARGET): $(LIKE_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(LIKE_OBJS) $(LIBS)


LOO_SRCS=$(HMM_SRCS)
LOO_HDRS=$(LOO_SRCS:.c=.h)
LOO_OBJS=$(LOO_SRCS:.c=.o) src/loo_prediction_test.o
LOO_TARGET=util/loo_prediction_test
$(LOO_TARGET): $(LOO_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(LOO_OBJS) $(LIBS)
# The program depends on the object files in $(OBJS). Make knows how
# to compile a .c file into an object (.o) file; this rule tells it
# how to link the .o files together into an executable program.




# All targets
TARGET=$(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) \
$(BIS_TARGET) $(IO_TARGET) $(DF_TARGET) $(HMM_TARGET) $(HTM_TARGET) \
$(MLT_TARGET) $(JNT_TARGET) $(EM_TARGET) $(GEN_TARGET) $(MAP_TARGET) \
$(INF_TARGET) $(CONV_TARGET) $(LIKE_TARGET) $(LOO_TARGET)

# All objects
OBJS=$(HTM_OBJS) src/potentialtest.o src/cliquetest.o src/parsertest.o \
src/graph_test.o src/bisontest.o src/iotest.o src/datafiletest.o \
src/hmmtest.o src/memleaktest.o src/joint_test.o src/em_test.o \
src/gen_test.o src/maptest.o src/inftest.o src/convert.o src/likelihood.o \
src/loo_prediction_test.o

# More rules for make
# The first rule tells make what to do by default.
# NOTE the tab character! Syntax of a rule:
# <target>: <dependencies>
# \t<command>
all: test util

test: $(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) \
$(BIS_TARGET) $(IO_TARGET) $(DF_TARGET) $(HMM_TARGET) $(HTM_TARGET) \
$(MLT_TARGET)

util: $(JNT_TARGET) $(EM_TARGET) $(GEN_TARGET) $(MAP_TARGET) $(INF_TARGET) \
$(CONV_TARGET) $(LIKE_TARGET) $(LOO_TARGET)



# With these lines, executing "make clean" removes the .o files that
# are not needed after the program is compiled.
clean:
	rm -f src/huginnet.tab.c src/huginnet.tab.h src/*.o src/*.i src/*.s

# "make realclean" does the same as "make clean", and also removes the
# compiled program and a possible "core" file.
realclean: clean
	rm -f $(TARGET) core

# Tells make that all, clean, realclean etc. are not the names of
# programs to compile.
.PHONY: all clean realclean test util lib
