# This is a copy of Makefile on the programming course T2 :

# A skeleton Makefile for the C programming assignment
# Please modify the parts marked with XXX below!
# Variable assignments for make
# XXX Replace "*.c" below with the names of your source files!
POT_SRCS=potential.c potentialtest.c
CLI_SRCS=potential.c Variable.c Clique.c cliquetest.c
PAR_SRCS=potential.c Variable.c Clique.c errorhandler.c fileio.c parser.c parsertest.c
HUG_DEFS=huginnet.y
HUG_SRCS=$(HUG_DEFS:.y=.tab.c)

# XXX Replace "cliquetest" below with the name you want for your program!
POT_TARGET=potentialtest
CLI_TARGET=cliquetest
PAR_TARGET=parsertest
TARGET=POT_TARGET CLI_TARGET PAR_TARGET

# You should not need to modify anything below this line...
# Sets the name and some flags for the C compiler and linker
CC=gcc
CFLAGS=-O2 -g -Wall
#CFLAGS=-Wall
LD=gcc
LDFLAGS=
YY=bison

# Link the math library in with the program, in case you use the
# functions in <math.h>
#LIBS=-lm
LIBS=

# This gives make the names of object files made by the compiler and
# used by the linker.
POT_OBJS=$(POT_SRCS:.c=.o)
CLI_OBJS=$(CLI_SRCS:.c=.o)
PAR_OBJS=$(PAR_SRCS:.c=.o)
HUG_OBJS=$(HUG_DEFS:.c=.o)
OBJS=POT_OBJS CLI_OBJS PAR_OBJS HUG_OBJS

# Rules for make
# The first rule tells make what to do by default: compile the program
# given by the variable TARGET (which was set above).
# NOTE the tab character! Syntax of a rule:
# <target>: <dependencies>
# \t<command>
all: $(POT_TARGET) $(CLI_TARGET) $(PAR_TARGET)

# The program depends on the object files in $(OBJS). Make knows how
# to compile a .c file into an object (.o) file; this rule tells it
# how to link the .o files together into an executable program.
$(POT_TARGET): $(POT_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(POT_OBJS) $(LIBS)

$(CLI_TARGET): $(CLI_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(CLI_OBJS) $(LIBS)

$(PAR_TARGET): $(PAR_OBJS) $(HUG_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(PAR_OBJS) $(LIBS)

$(HUG_SRCS): $(HUG_DEFS)
	$(YY) $(HUG_DEFS)

# With these lines, executing "make clean" removes the .o files that
# are not needed after the program is compiled.
clean:
	rm -f $(OBJS)

# "make realclean" does the same as "make clean", and also removes the
# compiled program and a possible "core" file.
realclean: clean
	rm -f $(TARGET) core

# Tells make that all, clean and realclean are not the names of
# programs to compile.
.PHONY: all clean realclean
