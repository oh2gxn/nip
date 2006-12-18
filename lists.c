/*
 * Functions for using list structures
 * (a C++ implementation would use STL)
 *
 * $Id: lists.c,v 1.2 2006-12-18 17:08:45 jatoivol Exp $
 */

#include <stdio.h>
#include <stdlib.h>
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


/*
 * list_to_<T>_array conversions
 */

double* list_to_double_array(doublelist dl){
  int i;
  doublelink ln = dl->first;
  double* new;
  
  if(!dl){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(dl->length == 0)
    return NULL;

  new = (double*) calloc(dl->length, sizeof(double));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  for(i = 0; i < dl->length; i++){
    new[i] = ln->data; /* the data is copied here */
    ln = ln->fwd;
  }
  return new;
}

char** list_to_string_array(stringlist sl){
  int i;
  stringlink ln = sl->first;
  char** new;
  
  if(!sl){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(sl->length == 0)
    return NULL;

  new = (char**) calloc(sl->length, sizeof(char*));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  for(i = 0; i < sl->length; i++){
    new[i] = ln->data; /* the pointer is copied here */
    ln = ln->fwd;
  }
  return new;
}


/*
 * empty_<T>list operations... don't free the dynamically allocated contents
 */

void empty_doublelist(doublelist dl){
  doublelink ln;

  if(!dl)
    return;
  
  ln = dl->last;

  dl->last = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(dl->first);
  dl->first = NULL;
  dl->length = 0;
  return;
}

void empty_stringlist(stringlist sl){
  stringlink ln;

  if(!sl)
    return;
  
  ln = sl->last;

  sl->last = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(sl->first);
  sl->first = NULL;
  sl->length = 0;
  return;
}
