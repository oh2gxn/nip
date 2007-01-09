/*
 * Various linked list data structures used e.g. in parser
 *
 * $Id: lists.h,v 1.8 2007-01-09 16:49:26 jatoivol Exp $
 */

#ifndef __LISTS_H__
#define __LISTS_H__

#include "variable.h"
#include "potential.h"
#include <stdio.h>


#define LIST_LENGTH(l)   ( (l)->length )
#define LIST_ITERATOR(l) ( (l)->first  )


/* List of doubles for parsing potential tables etc. */
struct doublelinktype {
  double data;
  struct doublelinktype *fwd;
  struct doublelinktype *bwd;
};

typedef struct doublelinktype doublelinkstruct;
typedef doublelinkstruct *doublelink;

struct doublelisttype {
  int length;
  doublelink first;
  doublelink last;
};

typedef struct doublelisttype doubleliststruct;
typedef doubleliststruct *doublelist;


/* List of strings for parsing state names etc. */
struct stringlinktype {
  char* data;
  struct stringlinktype *fwd;
  struct stringlinktype *bwd;
};

typedef struct stringlinktype stringlinkstruct;
typedef stringlinkstruct *stringlink;

struct stringlisttype {
  int length;
  stringlink first;
  stringlink last;
};

typedef struct stringlisttype stringliststruct;
typedef stringliststruct *stringlist;


/* List of strings for parsing <fieldname> = "<value>" pairs. 
 * (NIP_next = "<variable name>" gets handled differently) */
struct stringpairlinktype {
  char* name;
  char* value;
  struct stringpairlinktype *fwd;
  struct stringpairlinktype *bwd;
};

typedef struct stringpairlinktype stringpairlinkstruct;
typedef stringpairlinkstruct *stringpairlink;

struct stringpairlisttype {
  int length;
  stringpairlink first;
  stringpairlink last;
};

typedef struct stringpairlisttype stringpairliststruct;
typedef stringpairliststruct *stringpairlist;


/* List for storing variables */
struct variablelinktype {
  variable data;
  struct variablelinktype *fwd;
  struct variablelinktype *bwd;
};

typedef struct variablelinktype variablelinkstruct;
typedef variablelinkstruct *variablelink;
typedef variablelink variable_iterator;

struct variablelisttype {
  int length;
  variablelink first;
  variablelink last;
};

typedef struct variablelisttype variableliststruct;
typedef variableliststruct *variablelist;



/* List for storing parsed potentials while constructing the graph etc. */
struct potentialLinkType {
  potential data;
  variable child;
  variable* parents;
  struct potentialLinkType *fwd;
  struct potentialLinkType *bwd;
};

typedef struct potentialLinkType potentialLinkStruct;
typedef potentialLinkStruct *potentialLink;

struct potentialListType {
  int length;
  potentialLink first;
  potentialLink last;
};

typedef struct potentialListType potentialListStruct;
typedef potentialListStruct *potentialList;



/* List for storing "NIP_next" relations */
struct interfaceLinkType {
  variable var;
  char* next;
  struct interfaceLinkType *fwd;
  struct interfaceLinkType *bwd;
};

typedef struct interfaceLinkType interfaceLinkStruct;
typedef interfaceLinkStruct *interfaceLink;

struct interfaceListType {
  int length;
  interfaceLink first;
  interfaceLink last;
};

typedef struct interfaceListType interfaceListStruct;
typedef interfaceListStruct *interfaceList;



/* Creates an empty list of <T> */
doublelist make_doublelist();
stringlist make_stringlist();
stringpairlist make_stringpairlist();
variablelist make_variablelist();
potentialList make_potentialList();
interfaceList make_interfaceList();


/* Adds a <T> to the end of the list 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 */
int append_double(doublelist l, double d);
int append_string(stringlist l, char* s);
int append_stringpair(stringpairlist l, char* name, char* value);
int append_variable(variablelist l, variable v);
int append_potential(potentialList l, potential p, 
		     variable child, variable* parents);
int append_interface(interfaceList l, variable var, char* next);


/* Adds a <T> to the beginning of the list 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 */
int prepend_double(doublelist l, double d);
int prepend_string(stringlist l, char*  s);
int prepend_stringpair(stringpairlist l, char* name, char* value);
int prepend_variable(variablelist l, variable v);
int prepend_potential(potentialList l, potential p, 
		      variable child, variable* parents);
int prepend_interface(interfaceList l, variable var, char* next);


/* Creates a <T> array out of the list of <T>. 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 * Remember to free it (and the list too) */
double* list_to_double_array(doublelist l);
char**  list_to_string_array(stringlist l);
variable* list_to_variable_array(variablelist l);


/* Makes a list empty, free it with free(x) 
 * NOTE: Only the list is deleted, not the content. If you had a list 
 *       of dynamically allocated stuff, you just leaked memory.
 */
void empty_doublelist(doublelist l);
void empty_stringlist(stringlist l);
void empty_variablelist(variablelist l);

/* Frees the list AND its contents */
void free_stringlist(stringlist l);
void free_stringpairlist(stringpairlist l);

/* Frees the memory allocated to a potentialList.
 * NOTE: this frees also the actual potentials and parent variable arrays! 
 * (variables themselves are kept intact) */
void free_potentialList(potentialList l);

/* Frees the list structure and the strings stored into it. */
void free_interfaceList(interfaceList l);

/* Some helper functions */
int stringlist_contains(stringlist l, char* string);
char* stringpair_value(stringpairlist l, char* name);
variable next_variable(variable_iterator* it);
variable get_parser_variable(variablelist l, char *symbol);

#endif /* __LISTS_H__ */
