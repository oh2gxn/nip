/* Various linked list data structures used e.g. in parser
 * Author: Janne Toivola
 * Version: $Id: niplists.h,v 1.1 2010-11-22 15:45:21 jatoivol Exp $
 */

#ifndef __NIPLISTS_H__
#define __NIPLISTS_H__

#include <stdio.h>

/* FIXME: these are dangerous, if the field names don't match for all lists! */
#define NIP_LIST_LENGTH(l)   ( (l)->length )
#define NIP_LIST_ITERATOR(l) ( (l)->first  )
#define NIP_LIST_HAS_NEXT(l) ( (l)->fwd != NULL )
#define NIP_LIST_NEXT(l)     ( (l)->fwd  )
#define NIP_LIST_ELEMENT(l)  ( (l)->data  )


/* List of doubles for parsing potential tables etc. */
struct nip_double_link_type {
  double data;
  struct nip_double_link_type *fwd;
  struct nip_double_link_type *bwd;
};
typedef struct nip_double_link_type nip_double_link_struct;
typedef nip_double_link_struct *nip_double_link;

struct nip_double_list_type {
  int length;
  nip_double_link first;
  nip_double_link last;
};
typedef struct nip_double_list_type nip_double_list_struct;
typedef nip_double_list_struct *nip_double_list;


/* List of strings for parsing state names etc. */
struct nip_string_link_type {
  char* data;
  struct nip_string_link_type *fwd;
  struct nip_string_link_type *bwd;
};
typedef struct nip_string_link_type nip_string_link_struct;
typedef nip_string_link_struct *nip_string_link;

struct nip_string_list_type {
  int length;
  nip_string_link first;
  nip_string_link last;
};
typedef struct nip_string_list_type nip_string_list_struct;
typedef nip_string_list_struct *nip_string_list;


/* List of strings for parsing <fieldname> = "<value>" pairs. 
 * (NIP_next = "<variable name>" gets handled differently) */
struct nip_string_pair_link_type {
  char* name;
  char* value;
  struct nip_string_pair_link_type *fwd;
  struct nip_string_pair_link_type *bwd;
};
typedef struct nip_string_pair_link_type nip_string_pair_link_struct;
typedef nip_string_pair_link_struct *nip_string_pair_link;

struct nip_string_pair_list_type {
  int length;
  nip_string_pair_link first;
  nip_string_pair_link last;
};
typedef struct nip_string_pair_list_type nip_string_pair_list_struct;
typedef nip_string_pair_list_struct *nip_string_pair_list;



/* Creates an empty list of <T> */
nip_double_list nip_new_double_list();
nip_string_list nip_new_string_list();
nip_string_pair_list nip_new_string_pair_list();


/* Adds a <T> to the end of the list 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 */
int nip_append_double(nip_double_list l, double d);
int nip_append_string(nip_string_list l, char* s);
int nip_append_string_pair(nip_string_pair_list l, char* name, char* value);


/* Adds a <T> to the beginning of the list 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 */
int nip_prepend_double(nip_double_list l, double d);
int nip_prepend_string(nip_string_list l, char*  s);
int nip_prepend_string_pair(nip_string_pair_list l, char* name, char* value);


/* Creates a <T> array out of the list of <T>. 
 * NOTE: If <T> is pointer, only the pointer is copied, not the content 
 * Remember to free it (and the list too) */
double* nip_double_list_to_array(nip_double_list l);
char**  nip_string_list_to_array(nip_string_list l);


/* Makes a list empty, free it with free(x) 
 * NOTE: Only the list is deleted, not the content. If you had a list 
 *       of dynamically allocated stuff, you just leaked memory.
 */
void nip_empty_double_list(nip_double_list l);
void nip_empty_string_list(nip_string_list l);


/* Frees the list AND its contents */
void nip_free_string_list(nip_string_list l);
void nip_free_string_pair_list(nip_string_pair_list l);


/* Some helper functions */
int nip_string_list_contains(nip_string_list l, char* string);
char* nip_string_pair_value(nip_string_pair_list l, char* name);

#endif /* __NIPLISTS_H__ */
