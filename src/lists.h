/* Various linked list data structures used e.g. in parser
 *
 * $Id: lists.h,v 1.10 2008-12-20 12:59:53 jatoivol Exp $
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



/* Creates an empty list of <T> */
doublelist make_doublelist();
stringlist make_stringlist();
stringpairlist make_stringpairlist();


/* Adds a <T> to the end of the list 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 */
int append_double(doublelist l, double d);
int append_string(stringlist l, char* s);
int append_stringpair(stringpairlist l, char* name, char* value);


/* Adds a <T> to the beginning of the list 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 */
int prepend_double(doublelist l, double d);
int prepend_string(stringlist l, char*  s);
int prepend_stringpair(stringpairlist l, char* name, char* value);


/* Creates a <T> array out of the list of <T>. 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 * Remember to free it (and the list too) */
double* list_to_double_array(doublelist l);
char**  list_to_string_array(stringlist l);


/* Makes a list empty, free it with free(x) 
 * NOTE: Only the list is deleted, not the content. If you had a list 
 *       of dynamically allocated stuff, you just leaked memory.
 */
void empty_doublelist(doublelist l);
void empty_stringlist(stringlist l);


/* Frees the list AND its contents */
void free_stringlist(stringlist l);
void free_stringpairlist(stringpairlist l);


/* Some helper functions */
int stringlist_contains(stringlist l, char* string);
char* stringpair_value(stringpairlist l, char* name);

#endif /* __LISTS_H__ */
