/*
 * Functions for using list structures
 * (a C++ implementation would use STL)
 *
 * $Id: lists.c,v 1.8 2007-01-12 16:56:42 jatoivol Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lists.h"
#include "errorhandler.h"


/* 
 * make_<T>list operations 
 */

doublelist make_doublelist(){
  doublelist dl = (doublelist) malloc(sizeof(doubleliststruct));
  /* Q: What if NULL was returned? */
  dl->length = 0;
  dl->first  = NULL;
  dl->last   = NULL;
  return dl;
}

stringlist make_stringlist(){
  stringlist sl = (stringlist) malloc(sizeof(stringliststruct));
  /* Q: What if NULL was returned? */
  sl->length = 0;
  sl->first  = NULL;
  sl->last   = NULL;
  return sl;
}

stringpairlist make_stringpairlist(){
  stringpairlist sl = (stringpairlist) malloc(sizeof(stringpairliststruct));
  /* Q: What if NULL was returned? */
  sl->length = 0;
  sl->first  = NULL;
  sl->last   = NULL;
  return sl;
}


/*
 * append_<T> operations
 */

int append_double(doublelist l, double d){
  doublelink new = (doublelink) malloc(sizeof(doublelinkstruct));

  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = d;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}

int append_string(stringlist l, char* s){
  stringlink new = (stringlink) malloc(sizeof(stringlinkstruct));

  if(!l || !s){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = s;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}

int append_stringpair(stringpairlist l, char* name, char* value){
  stringpairlink new = (stringpairlink) malloc(sizeof(stringpairlinkstruct));

  if(!l || !name || !value){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->name = name;
  new->value = value;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}


/*
 * prepend_<T> operations
 */

int prepend_double(doublelist l, double d){
  doublelink new = (doublelink) malloc(sizeof(doublelinkstruct));

  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = d;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}

int prepend_string(stringlist l, char* s){
  stringlink new = (stringlink) malloc(sizeof(stringlinkstruct));

  if(!l || !s){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = s;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}

int prepend_stringpair(stringpairlist l, char* name, char* value){
  stringpairlink new = (stringpairlink) malloc(sizeof(stringpairlinkstruct));

  if(!l || !name | !value){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->name = name;
  new->value = value;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}



/*
 * list_to_<T>_array conversions
 */

double* list_to_double_array(doublelist l){
  int i;
  doublelink ln;
  double* new;
  
  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (double*) calloc(l->length, sizeof(double));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the data is copied here */
    ln = ln->fwd;
  }
  return new;
}

char** list_to_string_array(stringlist l){
  int i;
  stringlink ln;
  char** new;
  
  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (char**) calloc(l->length, sizeof(char*));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the pointer is copied here */
    ln = ln->fwd;
  }
  return new;
}


/*
 * empty_<T>list operations... don't free the dynamically allocated contents
 */

void empty_doublelist(doublelist l){
  doublelink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(l->first);
  l->first = NULL;
  l->length = 0;
  return;
}

void empty_stringlist(stringlist l){
  stringlink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(l->first);
  l->first = NULL;
  l->length = 0;
  return;
}


void free_stringlist(stringlist l){
  stringlink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    if(ln->fwd != NULL){
      free(ln->fwd->data);
      free(ln->fwd);
    }
    ln = ln->bwd;
  }
  if(l->first != NULL){
    free(l->first->data);
    free(l->first);
    l->first = NULL;
  }
  l->length = 0;

  free(l);
  l = NULL;
  return;
}

void free_stringpairlist(stringpairlist l){
  stringpairlink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    if(ln->fwd != NULL){
      free(ln->fwd->name); /* both strings freed */
      free(ln->fwd->value);
      free(ln->fwd);
    }
    ln = ln->bwd;
  }
  if(l->first != NULL){
    free(l->first->name);
    free(l->first->value);
    free(l->first);
    l->first = NULL;
  }
  l->length = 0;

  free(l);
  l = NULL;
  return;
}


/* Checks if the given string is in the list */
int stringlist_contains(stringlist l, char* string){
  stringlink s = NULL;

  if(!l || !string)
    return 0;

  s = l->first;
  while(s != NULL){
    if(strcmp(string, s->data) == 0){
      return 1; /* found */
    }
    s = s->fwd;
  }
  return 0; /* not found */
}

/* Searches the list for a certain field name and returns the corresponding
 * value string (or NULL, if not found) */
char* stringpair_value(stringpairlist l, char* name){
  stringpairlink s = NULL;

  if(!l || !name)
    return NULL;

  s = l->first;
  while(s != NULL){
    if(strcmp(name, s->name) == 0){
      return s->value; /* found */
    }
    s = s->fwd;
  }
  return NULL; /* not found */
}

