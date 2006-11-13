/*
 * Functions for using list structures
 *
 * $Id: lists.c,v 1.1 2006-11-13 18:00:29 jatoivol Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include "lists.h"
#include "errorhandler.h"


doublelist make_doublelist(){
  doublelist dl = (doublelist) malloc(sizeof(doubleliststruct));
  /* Q: What if NULL was returned? */
  dl->length = 0;
  dl->first  = NULL;
  dl->last   = NULL;
  return dl;
}


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
