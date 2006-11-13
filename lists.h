/*
 * Various linked list data structures used e.g. in parser
 *
 * $Id: lists.h,v 1.1 2006-11-13 18:00:29 jatoivol Exp $
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
struct stringlist {
  char* data;
  struct stringlist *fwd;
  struct stringlist *bwd;
};

typedef struct stringlist stringelement;
typedef stringelement *stringlink;


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


/* Creates an empty list of doubles */
doublelist make_doublelist();

/* Adds a double to the end of the list */
int append_double(doublelist l, double d);

/* Adds a double to the beginning of the list */
int prepend_double(doublelist l, double d);

/* Creates a double array out of the list of doubles. 
 * Remember to free it (and the list too) */
double* list_to_double_array(doublelist dl);

/* Makes a list of doubles empty, free it with free(dl) */
void empty_doublelist(doublelist dl);


#endif /* __LISTS_H__ */
