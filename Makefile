# Makefile for the "nip" project.
#
# FIXME & TODO: all kinds of stuff !!!
# - make a library out of the basic source files
# - make test programs separately
# - make utility programs separately
#
# $Id: Makefile,v 1.60 2009-01-04 14:53:53 jatoivol Exp $


# Sets the name and some flags for the C compiler and linker
CC = gcc
CCFLAGS = -c
CFLAGS = -g -pedantic-errors -Wall
#CFLAGS=-O2 -Wall
#CFLAGS = -Os -g -Wall -ansi -pedantic-errors
#CFLAGS = -g -Wall --save-temps

LD = gcc
LDFLAGS = -static
#LDFLAGS = -v

YY = bison
YYFLAGS = -d

# Link the math library in with the program, in case you use the
# functions in <math.h>
LIBS = -lm
NIPLIBS = -lm -L. -lnip


# NOTE: for some reason, this does not work
#%.o: %.c %.h
#	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

# Rules for static object files
src/fileio.o: src/fileio.c src/fileio.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

src/errorhandler.o: src/errorhandler.c src/errorhandler.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

src/potential.o: src/potential.c src/potential.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

src/variable.o: src/variable.c src/variable.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

src/clique.o: src/clique.c src/clique.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

src/Heap.o: src/Heap.c src/Heap.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

src/cls2clq.o: src/cls2clq.c src/cls2clq.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

src/Graph.o: src/Graph.c src/Graph.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

# use Bison parser generator
HUG_DEF = src/huginnet.y
HUG_SRC = $(HUG_DEF:.y=.tab.c)
HUG_HDR = $(HUG_DEF:.y=.tab.h)
HUG_OBJ = $(HUG_DEF:.y=.o)
$(HUG_SRC) $(HUG_HDR): $(HUG_DEF)
	$(YY) $(YYFLAGS) $< -o $(HUG_SRC)

$(HUG_OBJ): $(HUG_SRC) $(HUG_HDR)
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

# ...the rest of the library objects
src/lists.o: src/lists.c src/lists.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

src/parser.o: src/parser.c src/parser.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

src/nip.o: src/nip.c src/nip.h
	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@


# Rules to create the static and shared libraries
LIB_SRCS = src/fileio.c \
src/errorhandler.c \
src/potential.c \
src/variable.c \
src/clique.c \
src/Heap.c \
src/cls2clq.c \
src/Graph.c \
$(HUG_SRC) \
src/lists.c \
src/parser.c \
src/nip.c
LIB_HDRS = $(LIB_SRCS:.c=.h)
LIB_OBJS = $(LIB_SRCS:.c=.o)
SLIB = lib/libnip.a
DLIB = lib/libnip.so
lib: $(SLIB) $(DLIB)

# package the object files as a static library
$(SLIB): $(LIB_OBJS)
	ar rcs $(SLIB) $(LIB_OBJS)

# compile a shared library
lib/libnip.so: $(LIB_OBJS)
	$(CC) -shared -Wl,-soname,libnip.so.1 -o libnip.so.1.0.1  $(LIB_OBJS)
# FIXME: compile the objects separately with -fPIC ?
# TODO: why .1 and .1.0.1 ???


# Rules for making each program

### FIXME the source code assumes fileio.h, but it's src/fileio.h

IO_SRC=test/iotest.c
IO_OBJ=$(IO_SRC:.c=.o)
IO_TARGET=test/iotest
$(IO_TARGET): $(IO_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(NIPLIBS) -o $@

POT_SRC=test/potentialtest.c
POT_HDR=$(POT_SRC:.c=.h)
POT_OBJ=$(POT_SRC:.c=.o)
POT_TARGET=test/potentialtest
$(POT_TARGET): $(POT_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(NIPLIBS) -o $@


CLI_SRC=test/cliquetest.c
CLI_HDR=$(CLI_SRCS:.c=.h)
CLI_OBJ=$(CLI_SRCS:.c=.o)
CLI_TARGET=test/cliquetest
$(CLI_TARGET): $(CLI_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(NIPLIBS) -o $@
#	$(LD) $(LDFLAGS) -o $@ $(CLI_OBJS) $(LIBS)


# TODO: all the ones below

GRPH_SRCS=$(CLI_SRCS) src/cls2clq.c src/Graph.c
GRPH_HDRS=$(GRPH_SRCS:.c=.h)
GRPH_OBJS=$(GRPH_SRCS:.c=.o) src/graph_test.o
GRPH_TARGET=test/graph_test
$(GRPH_TARGET): $(GRPH_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(GRPH_OBJS) $(LIBS)



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
