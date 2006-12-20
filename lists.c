/*
 * Functions for using list structures
 * (a C++ implementation would use STL)
 *
 * $Id: lists.c,v 1.3 2006-12-20 15:57:29 jatoivol Exp $
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

variablelist make_variablelist(){
  variablelist vl = (variablelist) malloc(sizeof(variableliststruct));
  /* Q: What if NULL was returned? */
  vl->length = 0;
  vl->first  = NULL;
  vl->last   = NULL;
  return vl;
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

int append_variable(variablelist l, variable v){
  variablelink new = (variablelink) malloc(sizeof(variablelinkstruct));

  if(!l || !v){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = v;
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

int prepend_variable(variablelist l, variable v){
  variablelink new = (variablelink) malloc(sizeof(variablelinkstruct));

  if(!l || !v){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = v;
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
  doublelink ln;
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

  ln = dl->first;
  for(i = 0; i < dl->length; i++){
    new[i] = ln->data; /* the data is copied here */
    ln = ln->fwd;
  }
  return new;
}

char** list_to_string_array(stringlist sl){
  int i;
  stringlink ln;
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

  ln = sl->first;
  for(i = 0; i < sl->length; i++){
    new[i] = ln->data; /* the pointer is copied here */
    ln = ln->fwd;
  }
  return new;
}

variable* list_to_variable_array(variablelist vl){
  int i;
  variablelink ln;
  variable* new;
  
  if(!vl){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(vl->length == 0)
    return NULL;

  new = (variable*) calloc(vl->length, sizeof(variable));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  ln = vl->first;
  for(i = 0; i < vl->length; i++){
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

void empty_variablelist(variablelist vl){
  variablelink ln;

  if(!vl)
    return;
  
  ln = vl->last;

  vl->last = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(vl->first);
  vl->first = NULL;
  vl->length = 0;
  return;
}


/*
 * Iterator
 */
variable next_variable(variable_iterator* it){
  variable v;
  if(*it){
    v = (*it)->data;
    *it = (*it)->fwd;
  }
  else
    v = NULL;
  return v;
}


/*
 * Search method
 */
variable get_parser_variable(variablelist vl, char *symbol){
  variable v; 
  variable_iterator it = vl->first; /* a private copy */
  v = next_variable(&it);

  if(v == NULL)
    return NULL; /* didn't find the variable (possibly normal) */
  
  /* search for the variable reference */
  while(strcmp(symbol, v->symbol) != 0){
    v = next_variable(&it);
    if(v == NULL){
      return NULL; /* didn't find the variable (a normal situation) */
    }
  }
  return v;
}

