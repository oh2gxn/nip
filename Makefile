# This is a copy of Makefile on the programming course T2 :

# A skeleton Makefile for the C programming assignment
# Please modify the parts marked with XXX below!
# Variable assignments for make
# XXX Replace "*.c" below with the names of your source files!
SRCS=potential.c Variable.c Clique.c cliquetest.c

# XXX Replace "tester" below with the name you want for your program!
TARGET=cliquetest

# You should not need to modify anything below this line...
# Sets the name and some flags for the C compiler and linker
CC=gcc
CFLAGS=-O2 -g -Wall
#CFLAGS=-Wall
LD=gcc
LDFLAGS=

# Link the math library in with the program, in case you use the
# functions in <math.h>
#LIBS=-lm
LIBS=

# This gives make the names of object files made by the compiler and
# used by the linker.
OBJS=$(SRCS:.c=.o)

# Rules for make
# The first rule tells make what to do by default: compile the program
# given by the variable TARGET (which was set above).
# NOTE the tab character! Syntax of a rule:
# <target>: <dependencies>
# \t<command>
all: $(TARGET)

# The program depends on the object files in $(OBJS). Make knows how
# to compile a .c file into an object (.o) file; this rule tells it
# how to link the .o files together into an executable program.
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

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
