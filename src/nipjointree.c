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

/* nipjointree.c
 * Authors: Janne Toivola, Mikko Korpela
 * Version: $Id: nipjointree.c,v 1.13 2011-03-14 14:37:36 jatoivol Exp $
 */

#include "nipjointree.h"


/*
#define NIP_DEBUG_CLIQUE
*/

/*
#define NIP_DEBUG_RETRACTION
*/

static nip_error_code nip_message_pass(nip_clique c1, 
				       nip_sepset s, 
				       nip_clique c2);

/* Tells which variable v is in clique c */
static int nip_clique_var_index(nip_clique c, nip_variable v);

/* Tells if clique c is marked */
static int nip_clique_marked(nip_clique c);

/* Depth-first search in the clique tree starting from clique 'start' */
static nip_error_code nip_join_tree_dfs(nip_clique start, 
					nip_error_code (*cFuncPointer)(nip_clique, double*),
					nip_error_code (*sFuncPointer)(nip_sepset, double*),
					double* ptr);

/* Some callback functions for join_tree_dfs */
static nip_error_code nip_retract_clique(nip_clique c, double* ptr);
static nip_error_code nip_retract_sepset(nip_sepset s, double* ptr);
static nip_error_code nip_clique_mass(nip_clique c, double* ptr);
static nip_error_code nip_neg_sepset_mass(nip_sepset s, double* ptr);

/* Internal function for removing s from c */
static void nip_remove_sepset(nip_clique c, nip_sepset s);


nip_clique nip_new_clique(nip_variable vars[], int nvars){
  nip_clique c;
  int *cardinality;
  int *reorder;
  int *indices;
  int i, j;
  unsigned long temp;

  c = (nip_clique) malloc(sizeof(nip_clique_struct));
  if(!c){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }    

  cardinality = (int *) calloc(nvars, sizeof(int));
  if(!cardinality){
    free(c);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }    

  reorder = (int *) calloc(nvars, sizeof(int));
  if(!reorder){
    free(c);
    free(cardinality);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }    

  indices = (int *) calloc(nvars, sizeof(int));
  if(!indices){
    free(c);
    free(cardinality);
    free(reorder);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }    

  /* reorder[i] is the place of i:th variable (in the sense of this program) 
   * in the array variables[].
   * Because of this, vars[] can be given in any order.
   */

  /* init (needed?) */
  for(i = 0; i < nvars; i++)
    indices[i] = 0;

  /* Create the reordering table: O(nvars^2) i.e. stupid but working.
   * Note the temporary use of indices array. */
  for(i = 0; i < nvars; i++){
    temp = nip_variable_id(vars[i]);
    for(j = 0; j < nvars; j++){
      if(nip_variable_id(vars[j]) > temp)
	indices[j]++; /* counts how many greater variables there are */
    }
  }

  for(i = 0; i < nvars; i++)
    reorder[indices[i]] = i; /* fill the reordering */

  c->variables = (nip_variable *) calloc(nvars, sizeof(nip_variable));
  if(!(c->variables)){
    free(c);
    free(cardinality);
    free(reorder);
    free(indices);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* JJ_NOTE: reordering probably not required anymore... */
  for(i = 0; i < nvars; i++){
    cardinality[i] = NIP_CARDINALITY(vars[reorder[i]]);
    c->variables[i] = vars[reorder[i]];
  }
  
  c->p = nip_new_potential(cardinality, nvars, NULL);
  c->original_p = nip_new_potential(cardinality, nvars, NULL);

  /* Propagation of error */
  if(c->p == NULL || c->original_p == NULL){
    free(cardinality);
    free(indices);
    free(reorder);
    free(c->variables);
    nip_free_potential(c->p);
    nip_free_potential(c->original_p);
    free(c);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NULL;
  }

  free(cardinality);
  free(indices);
  free(reorder);
  c->sepsets = NULL;
  c->mark = NIP_MARK_OFF;

  return c;
}


void nip_free_clique(nip_clique c){
  nip_sepset_link l1, l2;
  nip_clique cln;
  nip_sepset s;

  if(c == NULL)
    return;
  
  /* clean the list of sepsets */
  l1 = c->sepsets;
  while(l1 != NULL){
    l2 = l1->fwd;
    s = (nip_sepset)l1->data;

    /* Remove sepsets from the cliques. */
    cln = s->first_neighbour;
    nip_remove_sepset(cln, s);
    cln = s->second_neighbour;
    nip_remove_sepset(cln, s);

    /* Free the sepset. */
    nip_free_sepset(s);

    l1=l2;
  }
  /* clean the rest */
  nip_free_potential(c->p);
  nip_free_potential(c->original_p);
  free(c->variables);
  free(c);
  return;
}


nip_error_code nip_confirm_sepset(nip_sepset s){
  int i;
  nip_clique c;
  nip_sepset_link new, new2;

  new = (nip_sepset_link) malloc(sizeof(nip_sepsetlink_struct));
  new2 = (nip_sepset_link) malloc(sizeof(nip_sepsetlink_struct));
  if(!new || !new2){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }
  
  c = s->first_neighbour;
  for (i=0; i<2; i++){
    new->data = s;
    new->fwd = c->sepsets;
    new->bwd = NULL;
    if(c->sepsets != NULL)
      c->sepsets->bwd = new;
    c->sepsets = new;
    new = new2; /* init second loop */
    c = s->second_neighbour;
  }

  return NIP_NO_ERROR;
}


static void nip_remove_sepset(nip_clique c, nip_sepset s){
  nip_sepset_link l;

  if(!(c && s)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return;
  }
  l = c->sepsets;
  while(l != NULL){
    if(((nip_sepset)l->data) == s){

      if(l->bwd == NULL)
	c->sepsets = l->fwd;
      else
	l->bwd->fwd = l->fwd;

      if(l->fwd != NULL)
	l->fwd->bwd = l->bwd;

      free(l);
      return;
    }      
    l = l->fwd;
  }
  return;
}


nip_sepset nip_new_sepset(nip_clique neighbour_a, nip_clique neighbour_b){
  nip_sepset s;
  int isect_size;
  int *cardinality = NULL;
  /*int *reorder = NULL;
    int *indices = NULL;*/
  int i;

  if (neighbour_a == NULL || neighbour_b == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NULL;
  }    

  s = (nip_sepset) malloc(sizeof(nip_sepset_struct));
  if(!s){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* Take the intersection of two cliques. */
  s->first_neighbour = neighbour_a;
  s->second_neighbour = neighbour_b;
  s->variables = nip_variable_isect(neighbour_a->variables, 
				    neighbour_b->variables, 
				    NIP_DIMENSIONALITY(neighbour_a->p),
				    NIP_DIMENSIONALITY(neighbour_b->p),
				    &isect_size);

  if(isect_size < 0){
    free(s);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NULL;
  }

  if(isect_size > 0){
    cardinality = (int *) calloc(isect_size, sizeof(int));
    if(!cardinality){
      nip_free_sepset(s);
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
  }
  else{
    cardinality = NULL;
  }

  /* JJ_NOTE: reordering was not required anymore..? */
  for(i = 0; i < isect_size; i++)
    cardinality[i] = NIP_CARDINALITY(s->variables[i]);

  s->old = nip_new_potential(cardinality, isect_size, NULL);
  s->new = nip_new_potential(cardinality, isect_size, NULL);
  free(cardinality); /* was copied by new_potential */
  if(s->old == NULL || s->new == NULL){
    nip_free_sepset(s);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NULL;
  }

  return s;
}


void nip_free_sepset(nip_sepset s){
  if(s){
    nip_free_potential(s->old);
    nip_free_potential(s->new);
    free(s->variables);
    free(s);
  }
  return;
}


/* FIXME: seems to be copy-paste from new_sepset... 
 * use mapper and functions provided by nippotential.h ! */
nip_potential nip_create_potential(nip_variable variables[], 
				   int nvars, 
				   double data[]){
  /*
   * Suppose we get an array of variables with IDs {5, 2, 3, 4, 0, 1}.
   * In this case, temp_array will be              {5, 2, 3, 4, 0, 1},
   * and reorder will be                           {4, 5, 1, 2, 3, 0}.
   *
   * If we get variables with IDs                  {6, 9, 3, 1, 5, 8},
   * temp_array will be                            {3, 5, 1, 0, 2, 4},
   * and reorder will be                           {3, 2, 4, 0, 5, 1}.
   *
   * temp_array is an array {x_0, x_1, ..., x_N-1}, where x_i
   * is a number indicating how many smaller IDs there are in
   * the variables[] array than the ID of variables[i]
   *
   * reorder is an array {x_0, x_1, ..., x_N-1}, where x_i is
   * the index in variables[] of the variable with the (i+1) -smallest ID.
   * For example, x_0 tells us where in the original array we can find
   * the variable with the smallest ID.
   */

  int i, j, card_temp, index, size_of_data = 1;

  int *cardinality;
  int *indices;
  int *temp_array;
  int *reorder;

  unsigned long temp;
  nip_potential p;

  if((cardinality = (int *) calloc(nvars, sizeof(int))) == NULL) {
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  if((indices = (int *) calloc(nvars, sizeof(int))) == NULL) {
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(cardinality);
    return NULL;
  }

  if((temp_array = (int *) calloc(nvars, sizeof(int))) == NULL) {
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(cardinality);
    free(indices);
    return NULL;
  }

  if((reorder = (int *) calloc(nvars, sizeof(int))) == NULL) {
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(cardinality);
    free(indices);
    free(temp_array);
    return NULL;
  }

  /* reorder[i] is the place of i:th variable (in the sense of this program) 
   * in the array variables[] */

  /* init (needed?) */
  for(i = 0; i < nvars; i++)
    temp_array[i] = 0;

  /* Create the reordering table: O(nvars^2) i.e. stupid but working.
   * Note the temporary use of indices array. */
  for(i = 0; i < nvars; i++){
    temp = nip_variable_id(variables[i]);
    for(j = 0; j < nvars; j++){
      if(nip_variable_id(variables[j]) > temp)
	temp_array[j]++; /* counts how many greater variables there are */
    }
  }

  for(i = 0; i < nvars; i++)
    reorder[temp_array[i]] = i; /* fill the reordering */

  /* Figure out some stuff */
  for(i = 0; i < nvars; i++){
    size_of_data *= NIP_CARDINALITY(variables[i]); /* optimal? */
    cardinality[i] = NIP_CARDINALITY(variables[reorder[i]]);
  }

  /* Create a potential */
  p = nip_new_potential(cardinality, nvars, NULL);

  /* Propagation of error */
  if(p == NULL){
    free(cardinality);
    free(indices);
    free(temp_array);
    free(reorder);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NULL;
  }
  
  /*
   * NULL data is accepted.
   * In that case, potential will be uniformly distributed.
   */
  if(data != NULL)

    /* Copy the contents to their correct places */
    /******************************************************* 
	  JJT: In principle it is a bit ugly to do this 
	  at this level. If potential.c changes, this has to 
	  be revised too!!! 
    ********************************************************/
    for(i = 0; i < size_of_data; i++){

      /*
       * Now this is the trickiest part.
       * Find out indices (in the internal order of the program,
       * determined by the variable IDs).
       */
      nip_inverse_mapping(p, i, indices); 

      /* calculate the address in the original array */
      index = 0;
      card_temp = 1;

      /* THE mapping */
      for(j = 0; j < nvars; j++){
	index += indices[temp_array[j]] * card_temp;
	card_temp *= cardinality[temp_array[j]];
      }

      /* set the value (in a little ugly way) */
      /* data is being copied => free(data) somewhere */
      p->data[i] = data[index];
    }

  free(cardinality);
  free(indices);
  free(temp_array);
  free(reorder);

  return p;
}


/* NOTE: don't use this. This is just a bad idea we had... */
nip_potential nip_reorder_potential(nip_variable vars[], nip_potential p){

  /* Simple (stupid) checks */
  if(!p){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NULL;
  }
  if(!vars){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NULL;
  }

  /* Old crappy code removed, this does nothing. */
  /* The original idea could now be implemented with:
   * - creating a suitable mapping array
   * - nip_new_potential
   * - nip_update_potential(p, NULL, newp, mapping) */

  return nip_copy_potential(p);
}


void nip_unmark_clique(nip_clique c){
  if(c != NULL)
    c->mark = NIP_MARK_OFF;
  return;
}


/*
 * Returns 0 if clique is not marked, 1 if it is. (This could be a macro...)
 */
static int nip_clique_marked(nip_clique c){
  if (c)
    return (c->mark == NIP_MARK_ON);
  return 0;
}


int nip_clique_size(nip_clique c){
  if (c)
    return NIP_DIMENSIONALITY(c->p); /* macro? */
  return 0;
}


int nip_sepset_size(nip_sepset s){
  if (s)
    return NIP_DIMENSIONALITY(s->old); /* macro? */
  return 0;
}


/* Finds out if two cliques are in the same tree.
 * Returns 1 if they are, 0 if not.
 * cliques must be unmarked before calling this.
 */
int nip_cliques_connected(nip_clique one, nip_clique two){
  nip_sepset_link l;
  nip_sepset s;

  if(one == NULL || two == NULL) /* normal situation or an error? */
    return 0; /* as in FALSE */

  l = one->sepsets;

  /* mark */
  one->mark = NIP_MARK_ON;

  /* NOTE: this defines the equality of cliques. */
  if(one == two)
    return 1; /* TRUE (end of recursion) */

  /* call neighbouring cliques */
  while (l != NULL){
    s = (nip_sepset)(l->data);
    if(!nip_clique_marked(s->first_neighbour)){
      if(nip_cliques_connected(s->first_neighbour, two))
	return 1; /* TRUE */
    }
    else if(!nip_clique_marked(s->second_neighbour)){
      if(nip_cliques_connected(s->second_neighbour, two))
	return 1; /* TRUE */
    }
    l = l->fwd;
  }

  return 0; /* FALSE */
}


nip_error_code nip_distribute_evidence(nip_clique c){

  int retval;
  nip_sepset_link l;
  nip_sepset s;

#ifdef NIP_DEBUG_CLIQUE
  printf("Distributing evidence in ");
  print_clique(c);
#endif

  /* mark */
  c->mark = NIP_MARK_ON;

  /* pass the messages */
  l = c->sepsets;
  while (l != 0){
    s = l->data;
    if(!nip_clique_marked(s->first_neighbour)){
      retval = nip_message_pass(c, s, s->first_neighbour); /* pass a message */
      if(retval != NIP_NO_ERROR){
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }
    }
    else if(!nip_clique_marked(s->second_neighbour)){
      retval = nip_message_pass(c, s, s->second_neighbour); /* pass a message */
      if(retval != NIP_NO_ERROR){
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }
    }
    l = l->fwd;
  }

  /* call neighboring cliques */
  l = c->sepsets;
  while (l != 0){
    s = l->data;
    if(!nip_clique_marked(s->first_neighbour)){
      if(nip_distribute_evidence(s->first_neighbour) != NIP_NO_ERROR){
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }
    }
    else if(!nip_clique_marked(s->second_neighbour)){
      if(nip_distribute_evidence(s->second_neighbour) != NIP_NO_ERROR){
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }
    }
    l = l->fwd;
  }
  return NIP_NO_ERROR;
}


nip_error_code nip_collect_evidence(nip_clique c1, 
				    nip_sepset s12, 
				    nip_clique c2){
  nip_error_code retval;
  nip_sepset_link l;
  nip_sepset s;

  /* mark */
  c2->mark = NIP_MARK_ON;

  /* call neighboring cliques */
  l = c2->sepsets;
  while (l != NULL){
    s = l->data;
    if(!nip_clique_marked(s->first_neighbour)){
      retval = nip_collect_evidence(c2, s, s->first_neighbour);
      if(retval != NIP_NO_ERROR){
	nip_report_error(__FILE__, __LINE__, retval, 1);
	return retval;
      }
    }
    else if(!nip_clique_marked(s->second_neighbour))
      retval = nip_collect_evidence(c2, s, s->second_neighbour);
      if(retval != NIP_NO_ERROR){
	nip_report_error(__FILE__, __LINE__, retval, 1);
	return retval;
      }
    l = l->fwd;
  }

  /* pass the message to c1 */
  if((c1 != NULL) && (s12 != NULL)){
    retval = nip_message_pass(c2, s12, c1);
    if(retval != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, retval, 1);
      return retval;
    }
  }

#ifdef NIP_DEBUG_CLIQUE
  if(c1 != NULL && c2 != NULL){
    printf("Collecting evidence from ");
    print_clique(c2);
    printf(" to ");
    print_clique(c1);
  }
#endif

  return NIP_NO_ERROR;
}


/*
 * Method for passing messages between cliques.
 * The message goes from clique c1 through sepset s to clique c2.
 * Returns an error code.
 */
static nip_error_code nip_message_pass(nip_clique c1, 
				       nip_sepset s, 
				       nip_clique c2){
  nip_error_code retval;
  int *mapping;

  /* save the newer potential as old by switching the pointers */
  nip_potential temp;
  temp = s->old;
  s->old = s->new;
  s->new = temp;

  /*
   * Marginalise (projection). Information flows from clique c1 to sepset s.
   */
  mapping = nip_mapper(c1->variables, s->variables, 
		       NIP_DIMENSIONALITY(c1->p), 
		       NIP_DIMENSIONALITY(s->new));
  retval = nip_general_marginalise(c1->p, s->new, mapping);
  free(mapping);
  if(retval != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NIP_ERROR_GENERAL;
  }

  /*
   * Update (absorption). Information flows from sepset s to clique c2.
   */
  mapping = nip_mapper(c2->variables, s->variables, 
		       NIP_DIMENSIONALITY(c2->p), 
		       NIP_DIMENSIONALITY(s->new));
  retval = nip_update_potential(s->new, s->old, c2->p, mapping);
  free(mapping);
  if(retval != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NIP_ERROR_GENERAL;
  }

  return NIP_NO_ERROR;
}


/* TODO: check that this has a correct mapping between p and c! */
int nip_init_clique(nip_clique c, nip_variable child, 
		    nip_potential p, int transient){
  int i, j, k;
  int* mapping = NULL;
  nip_error_code err;
  nip_variable var = NULL;
  nip_variable* parents = nip_get_parents(child);

  if(NIP_DIMENSIONALITY(p) < NIP_DIMENSIONALITY(c->p)){

    mapping = (int *) calloc(NIP_DIMENSIONALITY(p), sizeof(int));
    if(!mapping){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NIP_ERROR_OUTOFMEMORY;
    }    
    
    /***************************************************************/
    /* HEY! parents[] NOT assumed to be in any particular order!   */
    /* But the variables of p are assumed to be in the same order  */
    /* as in the clique c!                                         */
    /***************************************************************/
    
    /* initialisation with conditional distributions 
       first: select the variables (in a stupid but working way) */
    /* FIXME: use mapper? */
    k = 0;
    for(i=0; i < NIP_DIMENSIONALITY(c->p); i++){
      if(k == NIP_DIMENSIONALITY(p))
	break; /* all found */
      var = (c->variables)[i];
      
      for(j=0; j < NIP_DIMENSIONALITY(p) - 1; j++)
	if(nip_equal_variables(var, parents[j]))
	  mapping[k++] = i;
      
      if(nip_equal_variables(var, child))
	mapping[k++] = i;
    }
  }

  /* rest the case */
  err = nip_init_potential(p, c->p, mapping);
  if(err != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, err, 1);
    free(mapping);
    return err;
  }

  /* Some extra work is done here,
   * because only the last initialisation counts. */
  if(!transient){
    err = nip_init_potential(p, c->original_p, mapping);
    if(err != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, err, 1);
      free(mapping);
      return err;
    }
  }
  /* This should be an optional phase: in timeslice models 
   * some clique potentials need to be initialised but still 
   * be retractable... */

  free(mapping); /* free(NULL) is O.K. */
  return NIP_NO_ERROR;
}


int nip_marginalise_clique(nip_clique c, nip_variable v, double r[]){
  int index = nip_clique_var_index(c, v);
  int retval;

  /* variable not in this clique => NIP_ERROR */
  if(index == -1){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }
  
  retval = nip_total_marginalise(c->p, r, index);
  if(retval != NIP_NO_ERROR)
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);

  return(retval);
}


int nip_global_retraction(nip_variable* vars, int nvars, 
			  nip_clique* cliques, int ncliques){
  int i, index;
  int retval;
  nip_variable v;
  nip_clique c;

  for(index = 0; index < ncliques; index++)
    nip_unmark_clique(cliques[index]);

  /* Reset all the potentials back to the original.
   * NOTE: this excludes the priors. */
  nip_join_tree_dfs(cliques[0], nip_retract_clique, nip_retract_sepset, NULL);

  /* Enter evidence back to the join tree.
   * Does not enter the priors... */
  for(i = 0; i < nvars; i++){
    v = vars[i];
    c = nip_find_family(cliques, ncliques, v);
    index = nip_clique_var_index(c, v);

    retval = nip_update_evidence(v->likelihood, NULL, c->p, index);
    if(retval != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      return NIP_ERROR_GENERAL;
    }
  }
  return NIP_NO_ERROR;
}


int nip_enter_observation(nip_variable* vars, int nvars, 
			  nip_clique* cliques, int ncliques, 
			  nip_variable v, char *state){
  int index = nip_variable_state_index(v, state);
  if(index < 0)
    return NIP_NO_ERROR;
  return nip_enter_index_observation(vars, nvars, 
				     cliques, ncliques, 
				     v, index);
}


int nip_enter_index_observation(nip_variable* vars, int nvars, 
				nip_clique* cliques, int ncliques, 
				nip_variable v, int index){
  int i, retval;
  double *evidence;

  if(index < 0)
    return NIP_NO_ERROR;

  evidence = (double *) calloc(NIP_CARDINALITY(v), sizeof(double));
  if(!evidence){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }

  for(i = 0; i < NIP_CARDINALITY(v); i++)
    if(i == index)
      evidence[i] = 1;
    else
      evidence[i] = 0;

  retval = nip_enter_evidence(vars, nvars, cliques, ncliques, v, evidence);
  free(evidence);
  return retval;
}


int nip_enter_evidence(nip_variable* vars, int nvars, 
		       nip_clique* cliques, int ncliques, 
		       nip_variable v, double evidence[]){
  int index, i;
  int retraction = 0;
  int retval;
  nip_clique c;

  if(v == NULL || evidence == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NIP_ERROR_NULLPOINTER;
  }

  c = nip_find_family(cliques, ncliques, v);    
  if(!c){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 0);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  index = nip_clique_var_index(c, v);

  for(i = 0; i < NIP_CARDINALITY(v); i++)
    if((v->likelihood)[i] == 0 && evidence[i] != 0)
      retraction = 1; /* Must do global retraction! */
  
  /*
   * Here is the update of clique potential.
   * MUST be done before update_likelihood.
   */
  if(!retraction){
    retval = nip_update_evidence(evidence, v->likelihood, c->p, index);
    if(retval != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      return NIP_ERROR_GENERAL;
    }
  }

  /* Update likelihood. Check the return value. */
  retval = nip_update_likelihood(v, evidence);
  if(retval != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NIP_ERROR_GENERAL;
  }

  if(retraction){
    retval = nip_global_retraction(vars, nvars, cliques, ncliques);
    if(retval != NIP_NO_ERROR)
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return(retval);
  }

  return NIP_NO_ERROR;
}


int nip_enter_prior(nip_variable* vars, int nvars, 
		    nip_clique* cliques, int ncliques, 
		    nip_variable v, double prior[]){
  int index, i;
  int e;
  nip_clique c;

  if(v == NULL || prior == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NIP_ERROR_NULLPOINTER;
  }

  /* printf("Entering prior for variable %s:\n", v->symbol); DEBUG */

  c = nip_find_family(cliques, ncliques, v);    

  e = 1;
  for(i = 0; i < NIP_CARDINALITY(v); i++){
    /* printf("%g ", prior[i]); DEBUG */
    if(prior[i] > 0)
      e = 0; /* Not a zero vector... */
  }
  /* printf("\n"); DEBUG */

  if(!c || e){ /* v not in any clique or prior is a zero vector */
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 0);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  index = nip_clique_var_index(c, v);

  /*
   * Here is the update of clique potential: simply a multiplication, 
   * effects of which may not disappear if it gets multiplied again
   * when the variable is observed and actual evidence entered.
   */
  e = nip_update_evidence(prior, NULL, c->p, index);
  if(e != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, e, 1);
    return e;
  }

  /* Don't update the likelihood... */

  return NIP_NO_ERROR;
}


/*
 * Method for checking if variable v is part of clique c.
 * Returns -1 if not, else the index of v among the variables in c.
 */
static int nip_clique_var_index(nip_clique c, nip_variable v){
  int var = 0;

  if(!(c && v)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return -1;
  }

  while(!nip_equal_variables(v, c->variables[var])){
    var++;
    if(var == NIP_DIMENSIONALITY(c->p))
      return -1; /* variable not in this clique => -1 */
  }
  return var;
}


nip_clique nip_find_family(nip_clique *cliques, int ncliques, 
			   nip_variable var){
  int i, n;
  nip_variable* family = NULL;
  nip_clique found;

  /* NOTE: uses memoization for finding families */
  if(var->family_clique != NULL)
    return (nip_clique)(var->family_clique);

  n = nip_number_of_parents(var);
  family = (nip_variable*) calloc(n + 1, sizeof(nip_variable));
  if(!family){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  if(n > 0) /* A useful check or not? */
    for(i = 0; i < n; i++)
      family[i] = var->parents[i];
  family[n] = var;

  found = nip_find_clique(cliques, ncliques, family, n+1);
  var->family_clique = found; /* MEMOIZE! */
  free(family);
  return found;
}


int* nip_find_family_mapping(nip_clique family, nip_variable child){
  int i, j, n, p;
  int* result = NULL;
  nip_variable* parents;

  if(!family || !child){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* FIXME: accesses nip_variable_struct, bad... */
  if(child->family_mapping == NULL){ /* if not found yet */
    parents = nip_get_parents(child);
    n = nip_number_of_parents(child) + 1;
    result = (int *) calloc(n, sizeof(int));
    if(!result){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NULL;
    }

    /* NOTE: Child must always be the first in the target potential! */
    for(i=0; i < NIP_DIMENSIONALITY(family->p); i++)
      if(nip_equal_variables((family->variables)[i], child)){
	result[0] = i;
	break;
      }

    if(n > 1){ /* if child actually has any parents */
      p = 0;
      n--; /* from #parents+1 down to #parents */
      for(i=0; i < NIP_DIMENSIONALITY(family->p); i++){
	if(p == n)
	  break; /* all pointers found */
	for(j=0; j < n; j++)
	  if(nip_equal_variables((family->variables)[i], parents[j])){
	    result[j+1] = i;
	    p++;
	    break;
	  }
      }
    }
    child->family_mapping = result; /* MEMOIZE */
  }
  return child->family_mapping;
}


nip_clique nip_find_clique(nip_clique *cliques, int ncliques, 
			   nip_variable *variables, int nvars){
  int i, j, k;
  int ok;

  for(i = 0; i < ncliques; i++){
    ok = 0;
    for(j = 0; j < nvars; j++){
      for(k = 0; k < NIP_DIMENSIONALITY(cliques[i]->p); k++){
	if(nip_equal_variables(variables[j], cliques[i]->variables[k])){
	  ok++;
	  break; /* found the variable in the clique */
	}
      }
    }
    /* All variables found in the clique */
    if(ok == nvars)
      return cliques[i];
  }

  return NULL;
}


void nip_fprintf_clique(FILE* stream, nip_clique c){
  int i;
  fprintf(stream, "clique ");
  /*fprintf(stream, "( ");*/
  for(i = 0; i < NIP_DIMENSIONALITY(c->p); i++)
    fprintf(stream, "%s ", nip_variable_symbol(c->variables[i]));
  /*fprintf(stream, ")");*/
  fprintf(stream, "\n");
}


void nip_fprintf_sepset(FILE* stream, nip_sepset s){
  int i;
  fprintf(stream, "sepset ");
  /*fprintf(stream, "[ ");*/
  for(i = 0; i < NIP_DIMENSIONALITY(s->old); i++)
    fprintf(stream, "%s ", nip_variable_symbol(s->variables[i]));
  /*fprintf(stream, "]");*/
  fprintf(stream, "\n");
}


static nip_error_code nip_retract_clique(nip_clique c, double* ptr){
  if(!c)
    return NIP_ERROR_NULLPOINTER;
  /* another option: 
   * nip_uniform_potential(c->p, 1.0);
   * nip_init_potential(c->original_p, c->p); */
  return nip_retract_potential(c->p, c->original_p);
}


static nip_error_code nip_retract_sepset(nip_sepset s, double* ptr){
  if(!s)
    return NIP_ERROR_NULLPOINTER;  
  nip_uniform_potential(s->old, 1);
  nip_uniform_potential(s->new, 1);
  return NIP_NO_ERROR;
}


/*
 * A generic function for traversing the join tree. 
 * cliques must be unmarked before calling this.
 * Parameters:
 * - a clique where the DFS starts
 * - a function pointer to the function to be used for every clique on the way
 * - a function pointer to the function to be used for every sepset on the way
 * - a double pointer where a return value can be written
 */
static nip_error_code nip_join_tree_dfs(nip_clique start, 
					nip_error_code (*cFuncPointer)(nip_clique, double*),
					nip_error_code (*sFuncPointer)(nip_sepset, double*),
					double* ptr) {

  /* a lot of copy-paste from collect/distribute_evidence and clique_search */
  nip_sepset_link l;
  nip_sepset s;
  nip_error_code err;
  
  if(start == NULL){ /* error? */
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NIP_ERROR_NULLPOINTER;
  }

  /* mark */
  start->mark = NIP_MARK_ON;

  if(cFuncPointer){
    err = cFuncPointer(start, ptr); /* do it now or after the children ??? */
    if(err != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, err, 1);
      return err;
    }
  }

  /* call neighboring cliques */
  l = start->sepsets;
  while (l != NULL){
    s = (nip_sepset)(l->data);
    if(!nip_clique_marked(s->first_neighbour)){
      if(sFuncPointer){
	err = sFuncPointer(s, ptr);
	if(err != NIP_NO_ERROR){
	  nip_report_error(__FILE__, __LINE__, err, 1);
	  return err;
	}
      }
      nip_join_tree_dfs(s->first_neighbour, cFuncPointer, sFuncPointer, ptr);
    }
    else if(!nip_clique_marked(s->second_neighbour)){
      if(sFuncPointer){
	err = sFuncPointer(s, ptr);
	if(err != NIP_NO_ERROR){
	  nip_report_error(__FILE__, __LINE__, err, 1);
	  return err;
	}
      }
      nip_join_tree_dfs(s->second_neighbour, cFuncPointer, sFuncPointer, ptr);
    }
    l = l->fwd;
  }
  return NIP_NO_ERROR;
}


static nip_error_code nip_clique_mass(nip_clique c, double* ptr){
  int i;
  double m = 0;
  for(i = 0; i < c->p->size_of_data; i++)
    m += c->p->data[i]; /* TODO: nip_potential_mass? */
  *ptr += m;
  return NIP_NO_ERROR;
}

static nip_error_code nip_neg_sepset_mass(nip_sepset s, double* ptr){
  int i;
  double m = 0;
  for(i = 0; i < s->new->size_of_data; i++)
    m += s->new->data[i]; /* TODO: nip_potential_mass? */
  *ptr -= m;
  return NIP_NO_ERROR;
}

double nip_probability_mass(nip_clique* cliques, int ncliques){
  int i;
  double ret;

  /* unmark all cliques */
  for(i = 0; i < ncliques; i++)
    nip_unmark_clique(cliques[i]);

  /* init */
  ret = 0;

  /* sum of the probability mass in cliques minus the mass in sepsets */
  nip_join_tree_dfs(cliques[0], nip_clique_mass, nip_neg_sepset_mass, &ret);
  return ret;
}


/** TODO: Currently this computes potentials in EVERY node of join tree 
 ** even if the answer can be found in a single node. It could try to 
 ** remember the found variables and prune the DFS after all necessary 
 ** variables have been encountered.
 **/
/* TODO: a lot of copy-paste, what if a dummy clique was used */
/* FIXME: this is probably a big mess... */
nip_potential nip_gather_joint_probability(nip_clique start, 
					   nip_variable *vars, int n_vars,
					   nip_variable *isect, int n_isect){  
  int i;
  int* mapping = NULL;
  int* cardinality = NULL;

  nip_potential prod = NULL; /* product of all messages and start */
  nip_variable* prod_vars = NULL; /* vars + clique */
  int nprod;                      /* size of prod */

  nip_potential msg = NULL;  /* message received from a branch */
  nip_variable* msg_vars = NULL;  /* vars + msg_isect */
  int nmsg;                       /* size of msg */

  nip_variable* msg_isect = NULL; /* isect in a branch */
  int nmsgi;                      /* size of msg_isect */

  nip_potential sum = NULL;  /* the result marginalized from prod */
  nip_sepset_link l;
  nip_sepset s; /* a neighbour sepset */
  nip_clique c; /* a neighbour clique */
  nip_error_code err;
  
  /* error? */
  if(start == NULL || n_vars < 0){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }
  if(n_vars == 0)
    return nip_new_potential(NULL, 0, NULL); /* potential of an empty set */
  if(vars == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  /* List of neigboring sepsets */
  l = start->sepsets;

  /* Mark the clique */
  start->mark = NIP_MARK_ON;

  /*** 1. Reserve space ***/
  /* 1.1 Form the union of given variables and clique variables */
  prod_vars = nip_variable_union(vars, start->variables, 
				 n_vars, NIP_DIMENSIONALITY(start->p),
				 &nprod);
  if(!prod_vars){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* 1.2 Potential for the union of variables */
  cardinality = (int*) calloc(nprod, sizeof(int));
  if(!cardinality){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(prod_vars);
    return NULL;
  }
  for(i = 0; i < nprod; i++)
    cardinality[i] = NIP_CARDINALITY(prod_vars[i]);

  /* ### possibly HUGE potential array ! ### */
  prod = nip_new_potential(cardinality, nprod, NULL); 

  /* free(cardinality);
   * reuse the larger cardinality array: nprod >= n_vars */

  /*** 2. Multiply (<start> clique) ***/
  
  /* 2.1 Form the mapping between potentials */
  /* NOTE: a slightly bigger array allocated for future purposes also. 
   * TODO: use nip_mapper() ??? */
  mapping = nip_mapper(prod_vars, start->variables, 
		       nprod, NIP_DIMENSIONALITY(start->p));
  if(!mapping){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(cardinality);
    free(prod_vars);
    nip_free_potential(prod);
    return NULL;
  }

  /* 2.2 Do the multiplication of potentials */
  err = nip_update_potential(start->p, NULL, prod, mapping);
  free(mapping);
  if(err != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, err, 1);
    free(cardinality);
    free(prod_vars);
    nip_free_potential(prod);
    return NULL;
  }

  /** Traverse to the neighboring cliques in the tree **/
  while (l != NULL){
    s = (nip_sepset)(l->data);
    /* try both directions in a neighboring sepset */
    for(i = 0; i < 2; i++){ 
      if (i==0)
	c = s->first_neighbour;
      else
	c = s->second_neighbour;

      if(!nip_clique_marked(c)){
	
	/*** 3. Operations on potentials ***/

	/* 3.1 Mapping between sepset and prod potentials */
	mapping = nip_mapper(prod_vars, s->variables, 
			     nprod, NIP_DIMENSIONALITY(s->new));

	/* 3.2 Division with sepset potential */
	err = nip_update_potential(NULL, s->new, prod, mapping);
	free(mapping);
	if(err != NIP_NO_ERROR){
	  nip_report_error(__FILE__, __LINE__, err, 1);
	  free(cardinality);
	  free(prod_vars);
	  nip_free_potential(prod);
	  return NULL;
	}

	/* 3.3 Decide what kind of potential you need 
	 *     from the rest of the tree */

	/* original <vars> and intersection of cliques */
	msg_isect = nip_variable_isect(s->variables, vars, 
				       NIP_DIMENSIONALITY(s->new), n_vars, 
				       &nmsgi);
	if(!msg_isect){
	  nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
	  free(cardinality);
	  free(prod_vars);
	  nip_free_potential(prod);
	  return NULL;
	}

	/* 3.4 Continue DFS */
	msg = nip_gather_joint_probability(c, vars, n_vars, msg_isect, nmsgi);
	

	/* 3.5 Mapping between prod potential and recursive result */
	msg_vars = nip_variable_union(vars, msg_isect, n_vars, nmsgi, &nmsg);
	mapping = nip_mapper(prod_vars, msg_vars, nprod, nmsg);
	free(msg_vars);

	/* 3.6 Multiplication with the recursive results */
	err = nip_update_potential(msg, NULL, prod, mapping);
	free(mapping);
	nip_free_potential(msg);
	if(err != NIP_NO_ERROR){
	  nip_report_error(__FILE__, __LINE__, err, 1);
	  free(cardinality);
	  nip_free_potential(prod);
	  return NULL;
	}
      }
    }

    l = l->fwd; /* next neigboring sepset */
  }
  free(prod_vars);

  /*** 4. Marginalisation (if any?) ***/

  /* If we already have what we need, no marginalisation needed... */
  if(nprod == n_vars + n_isect){
    /* FIXME: is the above condition enough? */
    free(cardinality);
    sum = prod;
  }
  else{
    
    /* 4.1 Reserve space for the result */
    for(i = 0; i < n_vars; i++)
      cardinality[i] = NIP_CARDINALITY(vars[i]);
    for(i = 0; i < n_isect; i++)
      cardinality[n_vars + i] = NIP_CARDINALITY(isect[i]);
    /* possibly LARGE potential array ! */
    sum = nip_new_potential(cardinality, n_vars + n_isect, NULL); 
    free(cardinality);
    
    /* 4.2 Form the mapping between prod and sum potentials */
    mapping = (int*) calloc(n_vars + n_isect, sizeof(int));
    for(i = 0; i < n_vars + n_isect; i++)
      mapping[i] = i; /* Trivial because of the union operation above */
    
    /* 4.3 Marginalise */
    err = nip_general_marginalise(prod, sum, mapping);
    free(mapping);
    /* The gain of having a join tree in the first place: */
    nip_free_potential(prod);
    if(err != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, err, 1);
      nip_free_potential(sum);
      return NULL;
    }
  }
  
  /* 4.4 Normalise (?) */
  /* Q: is this a good idea at this point? */
  /*normalise_potential(sum);*/

  return sum;
}


nip_potential_list nip_new_potential_list(){
  nip_potential_list pl = (nip_potential_list) 
    malloc(sizeof(nip_potential_list_struct));
  /* Q: What if NULL was returned? */
  pl->length = 0;
  pl->first  = NULL;
  pl->last   = NULL;
  return pl;
}


nip_error_code nip_append_potential(nip_potential_list l, 
				    nip_potential p, 
				    nip_variable child, 
				    nip_variable* parents){
  nip_potential_link new = (nip_potential_link) 
    malloc(sizeof(nip_potential_link_struct));

  if(!l || !p){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }

  new->data = p;
  new->child = child;
  /* The "ownership" of the parents[] array changes! */
  new->parents = parents;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NIP_NO_ERROR;
}


nip_error_code nip_prepend_potential(nip_potential_list l, 
				     nip_potential p, 
				     nip_variable child, 
				     nip_variable* parents){
  nip_potential_link new = (nip_potential_link) 
    malloc(sizeof(nip_potential_link_struct));

  if(!l || !p){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }

  new->data = p;
  new->child = child;
  new->parents = parents;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NIP_NO_ERROR;
}


void nip_free_potential_list(nip_potential_list l){
  nip_potential_link ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    if(ln->fwd != NULL){
      nip_free_potential(ln->fwd->data);
      free(ln->fwd->parents);
      free(ln->fwd);
    }
    ln = ln->bwd;
  }
  if(l->first != NULL){
    nip_free_potential(l->first->data);
    free(l->first->parents);
    free(l->first);
    l->first = NULL;
  }
  l->length = 0;

  free(l);
  l = NULL;
  return;
}

/* More operations for nip_potential_list? */
