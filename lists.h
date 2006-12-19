/*
 * Various linked list data structures used e.g. in parser
 *
 * $Id: lists.h,v 1.3 2006-12-19 17:54:43 jatoivol Exp $
 */

#ifndef __LISTS_H__
#define __LISTS_H__

#include "variable.h"
#include "potential.h"
#include <stdio.h>


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


/* List for storing parsed potentials while constructing the graph etc. */
struct initDataList {
  potential data;
  variable child;
  variable* parents;
  struct initDataList *fwd;
  struct initDataList *bwd;
};

typedef struct initDataList initDataElement;
typedef initDataElement *initDataLink;


/* List for storing "NIP_next" relations */
struct time_init_list {
  variable var;
  char* previous;
  struct time_init_list *fwd;
};

typedef struct time_init_list time_init_element;
typedef time_init_element *time_init_link;


/* Creates an empty list of <T> */
doublelist make_doublelist();
stringlist make_stringlist();

/* Adds a <T> to the end of the list 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 */
int append_double(doublelist l, double d);
int append_string(stringlist l, char* s);

/* Adds a <T> to the beginning of the list 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 */
int prepend_double(doublelist l, double d);
int prepend_string(stringlist l, char*  s);

/* Creates a <T> array out of the list of <T>. 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 * Remember to free it (and the list too) */
double* list_to_double_array(doublelist dl);
char**  list_to_string_array(stringlist sl);

/* Makes a list of doubles empty, free it with free(dl) 
 * NOTE: Only the list is deleted, not the content. If you had a list 
 *       of dynamically allocated stuff, you just leaked memory.
 */
void empty_doublelist(doublelist dl);
void empty_stringlist(stringlist sl);


#endif /* __LISTS_H__ */
