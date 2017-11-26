/**
 * @file
 * @brief Functions for using list structures
 *
 * This code is a prime example why object oriented programming 
 * and generics were invented.
 * (a C++ implementation would use STL, this could use macros or C11)
 * @author Janne Toivola
 * @copyright &copy; 2007,2012 Janne Toivola <br>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. <br>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. <br>
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/*  NIP - Dynamic Bayesian Network library
    Copyright (C) 2012  Janne Toivola

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include "niplists.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "niperrorhandler.h"


nip_int_array_list nip_new_int_array_list() {
  nip_int_array_list ial = (nip_int_array_list) 
    malloc(sizeof(nip_int_array_list_struct));
  if(!ial){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }
  ial->length = 0;
  ial->first = NULL;
  ial->last = NULL;
  return ial;
}


nip_int_list nip_new_int_list(){
  nip_int_list il = 
    (nip_int_list) malloc(sizeof(nip_int_list_struct));
  if(!il){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }
  il->length = 0;
  il->first  = NULL;
  il->last   = NULL;
  return il;
}


nip_double_list nip_new_double_list(){
  nip_double_list dl = 
    (nip_double_list) malloc(sizeof(nip_double_list_struct));
  if(!dl){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }
  dl->length = 0;
  dl->first  = NULL;
  dl->last   = NULL;
  return dl;
}


nip_string_list nip_new_string_list(){
  nip_string_list sl = 
    (nip_string_list) malloc(sizeof(nip_string_list_struct));
  if(!sl){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }
  sl->length = 0;
  sl->first  = NULL;
  sl->last   = NULL;
  return sl;
}


nip_string_pair_list nip_new_string_pair_list(){
  nip_string_pair_list sl = 
    (nip_string_pair_list) malloc(sizeof(nip_string_pair_list_struct));
  if(!sl){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }
  sl->length = 0;
  sl->first  = NULL;
  sl->last   = NULL;
  return sl;
}


int nip_append_int_array(nip_int_array_list l, int* i, int ni) {
  if(!l)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_int_array_link new = (nip_int_array_link) 
    malloc(sizeof(nip_int_array_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->data = i;
  new->size = ni;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;
  l->last = new;
  l->length++;
  return 0;
}


int nip_append_int(nip_int_list l, int i){
  if(!l)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_int_link new = 
    (nip_int_link) malloc(sizeof(nip_int_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->data = i;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;
  l->last = new;
  l->length++;
  return 0;
}


int nip_append_double(nip_double_list l, double d){
  if(!l)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_double_link new = 
    (nip_double_link) malloc(sizeof(nip_double_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->data = d;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;
  l->last = new;
  l->length++;
  return 0;
}


int nip_append_string(nip_string_list l, char* s){
  if(!l || !s)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_string_link new = 
    (nip_string_link) malloc(sizeof(nip_string_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->data = s;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;
  l->last = new;
  l->length++;
  return 0;
}


int nip_append_string_pair(nip_string_pair_list l, char* key, char* value){
  if(!l || !key || !value)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_string_pair_link new = 
    (nip_string_pair_link) malloc(sizeof(nip_string_pair_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->key = key;
  new->value = value;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;
  l->last = new;
  l->length++;
  return 0;
}


int nip_prepend_int_array(nip_int_array_list l, int* i, int ni) {
  if(!l)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_int_array_link new = 
    (nip_int_array_link) malloc(sizeof(nip_int_array_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->data = i;
  new->size = ni;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;
  l->first = new;
  l->length++;
  return 0;
}


int nip_prepend_int(nip_int_list l, int i){
  if(!l)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_int_link new = 
    (nip_int_link) malloc(sizeof(nip_int_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->data = i;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return 0;
}


int nip_prepend_double(nip_double_list l, double d){
  if(!l)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_double_link new = 
    (nip_double_link) malloc(sizeof(nip_double_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->data = d;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return 0;
}


int nip_prepend_string(nip_string_list l, char* s){
  if(!l || !s)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_string_link new = 
    (nip_string_link) malloc(sizeof(nip_string_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->data = s;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return 0;
}


int nip_prepend_string_pair(nip_string_pair_list l, 
			    char* key, char* value){
  if(!l || !key | !value)
    return nip_report_error(__FILE__, __LINE__, EFAULT, 1);

  nip_string_pair_link new = 
    (nip_string_pair_link) malloc(sizeof(nip_string_pair_link_struct));
  if(!new)
    return nip_report_error(__FILE__, __LINE__, ENOMEM, 1);

  new->key = key;
  new->value = value;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return 0;
}


int* nip_int_list_to_array(nip_int_list l){
  int i;
  nip_int_link ln;
  int* new;
  
  if(!l){
    nip_report_error(__FILE__, __LINE__, EFAULT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (int*) calloc(l->length, sizeof(int));
  if(!new){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the data is copied here */
    ln = ln->fwd;
  }
  return new;
}


double* nip_double_list_to_array(nip_double_list l){
  int i;
  nip_double_link ln;
  double* new;
  
  if(!l){
    nip_report_error(__FILE__, __LINE__, EFAULT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (double*) calloc(l->length, sizeof(double));
  if(!new){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the data is copied here */
    ln = ln->fwd;
  }
  return new;
}


char** nip_string_list_to_array(nip_string_list l){
  int i;
  nip_string_link ln;
  char** new;
  
  if(!l){
    nip_report_error(__FILE__, __LINE__, EFAULT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (char**) calloc(l->length, sizeof(char*));
  if(!new){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the pointer is copied here! */
    ln = ln->fwd;
  }
  return new;
}


void nip_empty_int_array_list(nip_int_array_list l) {
  nip_int_array_link ln;

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


void nip_empty_int_list(nip_int_list l){
  nip_int_link ln;

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


void nip_empty_double_list(nip_double_list l){
  nip_double_link ln;

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


void nip_empty_string_list(nip_string_list l){
  nip_string_link ln;

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


void nip_free_int_array_list(nip_int_array_list l) {
  nip_int_array_link ln;

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


void nip_free_string_list(nip_string_list l){
  nip_string_link ln;

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


void nip_free_string_pair_list(nip_string_pair_list l){
  nip_string_pair_link ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    if(ln->fwd != NULL){
      free(ln->fwd->key); /* both strings freed */
      free(ln->fwd->value);
      free(ln->fwd);
    }
    ln = ln->bwd;
  }
  if(l->first != NULL){
    free(l->first->key);
    free(l->first->value);
    free(l->first);
    l->first = NULL;
  }
  l->length = 0;

  free(l);
  l = NULL;
  return;
}


int nip_int_array_list_contains_subset(nip_int_array_list l,
				       int* i, int ni) {
  nip_int_array_link lnk;
  int v, flag;
  
  /* Iterate the list of known clusters */
  /* Later additions cannot be supersets of earlier ones */
  /* One of the variables in an earlier var_set is removed */
  for (lnk = l->first; lnk != NULL; lnk = lnk->fwd) {
    flag = 0;
    for (v = 0; v < ni; v++) {
      if (i[v] && !lnk->data[v]) {
	flag = 1; /* i not a subset of l */
	break;
      }
    }
    if (!flag) /* We have a subset */
      return 1;
  }
  
  return 0;
}


int nip_string_list_contains(nip_string_list l, char* string){
  nip_string_link s = NULL;

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


/* Searches the list for a certain key and returns the corresponding
 * value string (or NULL, if not found) */
char* nip_string_pair_list_search(nip_string_pair_list l, char* key){
  nip_string_pair_link s = NULL;

  if(!l || !key)
    return NULL;

  s = l->first;
  while(s != NULL){
    if(strcmp(key, s->key) == 0){
      return s->value; /* found */
    }
    s = s->fwd;
  }
  return NULL; /* not found */
}

