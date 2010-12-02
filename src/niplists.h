/* Various linked list data structures used e.g. in parser
 * Author: Janne Toivola
 * Version: $Id: niplists.h,v 1.6 2010-12-02 18:15:21 jatoivol Exp $
 */

#ifndef __NIPLISTS_H__
#define __NIPLISTS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "niperrorhandler.h"

/* FIXME: these are dangerous, if the field names don't match for all lists! */
#define NIP_LIST_LENGTH(l)   ( (l)->length )
#define NIP_LIST_ITERATOR(l) ( (l)->first  )
#define NIP_LIST_HAS_NEXT(l) ( (l)->fwd != NULL )
#define NIP_LIST_NEXT(l)     ( (l)->fwd  )
#define NIP_LIST_ELEMENT(l)  ( (l)->data  )


/* Element for linked list of integer arrays */
typedef struct nip_int_array_link_type {
  int* data;
  int size;
  struct nip_int_array_link_type* fwd;
  struct nip_int_array_link_type* bwd;
} nip_int_array_link_struct;
typedef nip_int_array_link_struct* nip_int_array_link;

/* Linked list of int arrays */
typedef struct nip_int_array_list_type {
  int length;            /* length of the list */
  nip_int_array_link first; /* first element */
  nip_int_array_link last;  /* last element */
} nip_int_array_list_struct;
typedef nip_int_array_list_struct* nip_int_array_list;


/* Element for linked list of doubles */
typedef struct nip_double_link_type {
  double data;                      /* the data */
  struct nip_double_link_type* fwd; /* next element */
  struct nip_double_link_type* bwd; /* previous element */
} nip_double_link_struct;
typedef nip_double_link_struct *nip_double_link;

/* Linked list of doubles */
typedef struct nip_double_list_type {
  int length;            /* length of the list */
  nip_double_link first; /* first element */
  nip_double_link last;  /* last element */
} nip_double_list_struct;
typedef nip_double_list_struct *nip_double_list;


/* Element for linked list of strings */
typedef struct nip_string_link_type {
  char* data;                       /* The string */
  struct nip_string_link_type *fwd; /* next element */
  struct nip_string_link_type *bwd; /* previous element */
} nip_string_link_struct;
typedef nip_string_link_struct *nip_string_link;

/* Linked list of strings */
typedef struct nip_string_list_type {
  int length;
  nip_string_link first;
  nip_string_link last;
} nip_string_list_struct;
typedef nip_string_list_struct *nip_string_list;


/* Element for linked list of <key> = "<value>" string pairs. 
 * (NIP_next = "<variable name>" gets handled differently) */
typedef struct nip_string_pair_link_type {
  char* key;
  char* value;
  struct nip_string_pair_link_type *fwd;
  struct nip_string_pair_link_type *bwd;
} nip_string_pair_link_struct;
typedef nip_string_pair_link_struct *nip_string_pair_link;

typedef struct nip_string_pair_list_type {
  int length;
  nip_string_pair_link first;
  nip_string_pair_link last;
} nip_string_pair_list_struct;
typedef nip_string_pair_list_struct *nip_string_pair_list;



/* Creates an empty list */
nip_int_array_list nip_new_int_array_list();
nip_double_list nip_new_double_list();
nip_string_list nip_new_string_list();
nip_string_pair_list nip_new_string_pair_list();


/* Adds data to the end of the list 
 * NOTE: In case of a pointer, only the pointer is copied, not the content 
 */
int nip_append_int_array(nip_int_array_list l, int* i, int ni);
int nip_append_double(nip_double_list l, double d);
int nip_append_string(nip_string_list l, char* s);
int nip_append_string_pair(nip_string_pair_list l, char* key, char* value);


/* Adds data to the beginning of the list 
 * NOTE: In case of a pointer, only the pointer is copied, not the content 
 */
int nip_prepend_int_array(nip_int_array_list l, int* i, int ni);
int nip_prepend_double(nip_double_list l, double d);
int nip_prepend_string(nip_string_list l, char*  s);
int nip_prepend_string_pair(nip_string_pair_list l, char* key, char* value);


/* Creates an array out of the given list. 
 * NOTE: In case of pointer contents (strings etc.), only the pointer is 
 * copied, not the actual content. 
 * Remember to free it (and the list too). */
double* nip_double_list_to_array(nip_double_list l);
char**  nip_string_list_to_array(nip_string_list l);


/* Makes a list empty, free it with free(x) 
 * NOTE: Only the list is deleted, not the content. If you had a list 
 *       of dynamically allocated stuff (pointers), you just leaked memory.
 */
void nip_empty_int_array_list(nip_int_array_list l);
void nip_empty_double_list(nip_double_list l);
void nip_empty_string_list(nip_string_list l);


/* Frees the list AND its contents */
void nip_free_int_array_list(nip_int_array_list l);
void nip_free_string_list(nip_string_list l);
void nip_free_string_pair_list(nip_string_pair_list l);


/* Returns 0 if the given int array (of flags) is not a subset of any of 
 * the arrays in the list. NOTE: the subset identification assumes 
 * all arrays are sorted and have ni elements! 
 * This piece of code was originally written for finding cliques in graphs. 
 */
int nip_int_array_list_contains_subset(nip_int_array_list l,
				       int* i, int ni);

/* Returns 0 if the given string is not found in the list. */
int nip_string_list_contains(nip_string_list l, char* string);


/* Makes a linear search through the list of (key,value) string pairs. 
 * If a matching key is found, a pointer to the value is returned. 
 * Otherwise, returns NULL. */
char* nip_string_pair_list_search(nip_string_pair_list l, char* key);

#endif /* __NIPLISTS_H__ */
