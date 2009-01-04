# Makefile for the "nip" project.
#
# TODO
# + make a library out of the basic source files
# + make test programs separately
# + make utility programs separately
#
# $Id: Makefile,v 1.61 2009-01-04 20:13:43 jatoivol Exp $


# Sets the name and some flags for the C compiler and linker
CC = gcc
CCFLAGS = -c
CFLAGS = -g -pedantic-errors -Wall
#CFLAGS = -g -pedantic-errors -Wall
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
NIPLIBS = -L./lib -lnip -lm
INC = -Isrc/

# NOTE: for some reason, this does not work
#%.o: %.c %.h
#	$(CC) $(CFLAGS) $(CCFLAGS) $< -o $@

all: test util

# Rules for static object files
src/fileio.o: src/fileio.c src/fileio.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

src/errorhandler.o: src/errorhandler.c src/errorhandler.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

src/potential.o: src/potential.c src/potential.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

src/variable.o: src/variable.c src/variable.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

src/clique.o: src/clique.c src/clique.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

src/Heap.o: src/Heap.c src/Heap.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

src/cls2clq.o: src/cls2clq.c src/cls2clq.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

src/Graph.o: src/Graph.c src/Graph.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

# use Bison parser generator
HUG_DEF = src/huginnet.y
HUG_SRC = $(HUG_DEF:.y=.tab.c)
HUG_HDR = $(HUG_DEF:.y=.tab.h)
HUG_OBJ = $(HUG_DEF:.y=.o)
$(HUG_SRC) $(HUG_HDR): $(HUG_DEF)
	$(YY) $(YYFLAGS) $< -o $(HUG_SRC)

$(HUG_OBJ): $(HUG_SRC) $(HUG_HDR)
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

# ...the rest of the library objects
src/lists.o: src/lists.c src/lists.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

src/parser.o: src/parser.c src/parser.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@

src/nip.o: src/nip.c src/nip.h
	$(CC) -fPIC $(CFLAGS) $(CCFLAGS) $< -o $@


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
DLIBSO = $(DLIB).1
DLIBRN = $(DLIBSO).0.0
lib: $(SLIB) $(DLIBRN)

# package the object files as a static library
$(SLIB): $(LIB_OBJS)
	ar rcs $(SLIB) $(LIB_OBJS)

# compile a shared library
$(DLIBRN): $(LIB_OBJS)
	$(CC) -shared -Wl,-soname,$(DLIBSO) -o $(DLIBRN)  $(LIB_OBJS)
# About sonames and realnames:
# http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html


# Rules for making each program

### FIXME the source code assumes fileio.h, but it's src/fileio.h

IO_SRC = test/iotest.c
IO_TARGET = test/iotest
$(IO_TARGET): $(IO_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@
# previously: $(LD) $(LDFLAGS) -o $@ $(IO_OBJS) $(LIBS)

POT_SRC = test/potentialtest.c
POT_TARGET = test/potentialtest
$(POT_TARGET): $(POT_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


CLI_SRC = test/cliquetest.c
CLI_TARGET = test/cliquetest
$(CLI_TARGET): $(CLI_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


GRPH_SRC = test/graph_test.c
GRPH_TARGET = test/graph_test
$(GRPH_TARGET): $(GRPH_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


PAR_SRC = test/parsertest.c
PAR_TARGET = test/parsertest
$(PAR_TARGET): $(PAR_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


BIS_SRC = test/bisontest.c
BIS_TARGET = test/bisontest
$(BIS_TARGET): $(BIS_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


DF_SRC = test/datafiletest.c
DF_TARGET = test/datafiletest
$(DF_TARGET): $(DF_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


HMM_SRC = test/hmmtest.c
HMM_TARGET = test/hmmtest
$(HMM_TARGET): $(HMM_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@

HTM_SRC = test/htmtest.c
HTM_TARGET = test/htmtest
$(HTM_TARGET): $(HTM_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


MLT_SRC = test/memleaktest.c
MLT_TARGET = test/memleaktest
$(MLT_TARGET): $(MLT_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@





JNT_SRC = util/joint_test.c
JNT_TARGET = util/joint_test
$(JNT_TARGET): $(JNT_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


EM_SRC = util/em_test.c
EM_TARGET = util/em_test
$(EM_TARGET): $(EM_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


GEN_SRC = util/gen_test.c
GEN_TARGET = util/gen_test
$(GEN_TARGET): $(GEN_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


MAP_SRC = util/map.c
MAP_TARGET = util/map
$(MAP_TARGET): $(MAP_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


INF_SRC = util/inftest.c
INF_TARGET = util/inftest
$(INF_TARGET): $(INF_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


CONV_SRC = util/convert.c
CONV_TARGET = util/convert
$(CONV_TARGET): $(CONV_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


LIKE_SRC = util/likelihood.c
LIKE_TARGET = util/likelihood
$(LIKE_TARGET): $(LIKE_SRC) $(SLIB)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@


LOO_SRC = util/loo_prediction_test.c
LOO_TARGET = util/loo_prediction_test
$(LOO_TARGET): $(LOO_SRC) $(HUG_OBJS)
	$(LD) $(LDFLAGS) $< $(INC) $(NIPLIBS) -o $@
# The program depends on the object files in $(OBJS). Make knows how
# to compile a .c file into an object (.o) file; this rule tells it
# how to link the .o files together into an executable program.




# All targets
TARGET=$(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) \
$(BIS_TARGET) $(IO_TARGET) $(DF_TARGET) $(HMM_TARGET) $(HTM_TARGET) \
$(MLT_TARGET) $(JNT_TARGET) $(EM_TARGET) $(GEN_TARGET) $(MAP_TARGET) \
$(INF_TARGET) $(CONV_TARGET) $(LIKE_TARGET) $(LOO_TARGET)


# More rules for make
# The first rule tells make what to do by default.
# NOTE the tab character! The syntax goes:
# <target>: <dependencies>
# \t<command>

#all: test util

test: $(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET) $(GRPH_TARGET) \
$(BIS_TARGET) $(IO_TARGET) $(DF_TARGET) $(HMM_TARGET) $(HTM_TARGET) \
$(MLT_TARGET)

util: $(JNT_TARGET) $(EM_TARGET) $(GEN_TARGET) $(MAP_TARGET) $(INF_TARGET) \
$(CONV_TARGET) $(LIKE_TARGET) $(LOO_TARGET)



# With these lines, executing "make clean" removes the .o files that
# are not needed after the program is compiled.
IFILES = $(LIB_OBJS:.o=.i)
SFILES = $(LIB_OBJS:.o=.s)
clean:
	rm -f $(HUG_SRC) $(HUG_HDR) $(LIB_OBJS) $(IFILES) $(SFILES)

# "make realclean" does the same as "make clean", and also removes the
# compiled program and a possible "core" file.
realclean: clean
	rm -f $(TARGET) $(SLIB) $(DLIBRN) core

# Tells make that all, clean, realclean etc. are not the names of
# programs to compile.
.PHONY: all clean realclean test util lib
