/*
 * Clique.c $Id: Clique.c,v 1.107 2005-02-22 15:18:47 jatoivol Exp $
 * Functions for handling cliques and sepsets.
 * Includes evidence handling and propagation of information
 * in the join tree.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"
#include "grphmnp/Heap.h"

/*
#define DEBUG_CLIQUE
*/

/*
#define DEBUG_RETRACTION
*/

static int message_pass(Clique c1, Sepset s, Clique c2);

static int var_index(Clique c, Variable v);

static int clique_search(Clique one, Clique two);

static void jtree_dfs(Clique start, void (*cFuncPointer)(Clique),
		      void (*sFuncPointer)(Sepset));

static int clique_marked(Clique c);

static void retract_Clique(Clique c);

static void retract_Sepset(Sepset s);

static void remove_Sepset(Clique c, Sepset s);

Clique make_Clique(Variable vars[], int num_of_vars){
  Clique c;
  int *cardinality;
  int *reorder;
  int *indices;
  int i, j;
  unsigned long temp;

  c = (Clique) malloc(sizeof(cliquetype));
  if(!c){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }    

  cardinality = (int *) calloc(num_of_vars, sizeof(int));
  if(!cardinality){
    free(c);
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }    

  reorder = (int *) calloc(num_of_vars, sizeof(int));
  if(!reorder){
    free(c);
    free(cardinality);
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }    

  indices = (int *) calloc(num_of_vars, sizeof(int));
  if(!indices){
    free(c);
    free(cardinality);
    free(reorder);
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }    

  /* reorder[i] is the place of i:th variable (in the sense of this program) 
   * in the array variables[].
   * Because of this, vars[] can be given in any order.
   */

  /* init (needed?) */
  for(i = 0; i < num_of_vars; i++)
    indices[i] = 0;

  /* Create the reordering table: O(num_of_vars^2) i.e. stupid but working.
   * Note the temporary use of indices array. */
  for(i = 0; i < num_of_vars; i++){
    temp = get_id(vars[i]);
    for(j = 0; j < num_of_vars; j++){
      if(get_id(vars[j]) > temp)
	indices[j]++; /* counts how many greater variables there are */
    }
  }

  for(i = 0; i < num_of_vars; i++)
    reorder[indices[i]] = i; /* fill the reordering */

  c->variables = (Variable *) calloc(num_of_vars, sizeof(Variable));
  if(!(c->variables)){
    free(c);
    free(cardinality);
    free(reorder);
    free(indices);
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* JJ_NOTE: reordering probably not required anymore... */
  for(i = 0; i < num_of_vars; i++){
    cardinality[i] = vars[reorder[i]]->cardinality;
    c->variables[i] = vars[reorder[i]];
  }
  
  c->p = make_potential(cardinality, num_of_vars, NULL);
  c->original_p = make_potential(cardinality, num_of_vars, NULL);

  /* Propagation of error */
  if(c->p == NULL || c->original_p == NULL){
    free(cardinality);
    free(indices);
    free(reorder);
    free(c->variables);
    free_potential(c->p);
    free_potential(c->original_p);
    free(c);
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return NULL;
  }

  free(cardinality);
  free(indices);
  free(reorder);
  c->sepsets = NULL;
  c->mark = 0;

  return c;
}


void free_Clique(Clique c){
  Sepset_link l1, l2;
  Clique cl1, cl2;
  Sepset s;

  if(c == NULL)
    return;
  
  /* clean the list of sepsets */
  l1 = c->sepsets;
  while(l1 != NULL){
    l2 = l1->fwd;
    cl1 = ((Sepset)l1->data)->cliques[0];
    cl2 = ((Sepset)l1->data)->cliques[1];
    s = (Sepset)l1->data;

    /* Removes Sepsets from the Cliques. */
    remove_Sepset(cl1, s);
    remove_Sepset(cl2, s);

    /* Destroy the Sepset. */
    free_Sepset(s);

    l1=l2;
  }
  /* clean the rest */
  free_potential(c->p);
  free_potential(c->original_p);
  free(c->variables);
  free(c);
  return;
}


int add_Sepset(Clique c, Sepset s){

  Sepset_link new = (Sepset_link) malloc(sizeof(element));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = s;
  new->fwd = c->sepsets;
  new->bwd = NULL;
  if(c->sepsets != NULL)
    c->sepsets->bwd = new;
  c->sepsets = new;

  return 0;
}


static void remove_Sepset(Clique c, Sepset s){
  Sepset_link l;

  if(!(c && s)){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return;
  }
  l = c->sepsets;
  while(l != NULL){
    if(((Sepset)l->data) == s){

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


/*
 * ATTENTION! Check what this does when num_of_vars == 0.
 */
Sepset make_Sepset(Variable vars[], int num_of_vars, Clique cliques[]){

  Sepset s;
  int *cardinality = NULL;
  int *reorder = NULL;
  int *indices = NULL;
  int i, j;
  unsigned long temp;

  s = (Sepset) malloc(sizeof(sepsettype));

  if(!s){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  if(num_of_vars){
    cardinality = (int *) calloc(num_of_vars, sizeof(int));
    if(!cardinality){
      free(s);
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return NULL;
    }

    reorder = (int *) calloc(num_of_vars, sizeof(int));
    if(!reorder){
      free(s);
      free(cardinality);
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return NULL;
    }

    indices = (int *) calloc(num_of_vars, sizeof(int));
    if(!indices){
      free(s);
      free(cardinality);
      free(reorder);
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
  }
  else{
    cardinality = NULL;
    reorder = NULL;
    indices = NULL;
  }

  s->cliques = (Clique *) calloc(2, sizeof(Clique));

  if(!s->cliques){
    free(s);
    free(cardinality);
    free(reorder);
    free(indices);
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  if(num_of_vars){
    s->variables = (Variable *) calloc(num_of_vars, sizeof(Variable));
    if(!s->variables){
      free(cardinality);
      free(reorder);
      free(indices);
      free(s->cliques);
      free(s);
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
  }

  /* reorder[i] is the place of i:th variable (in the sense of this program) 
   * in the array variables[].
   * Because of this, vars[] can be given in any order.
   */

  /* init (needed?) */
  for(i = 0; i < num_of_vars; i++)
    indices[i] = 0;

  /* Create the reordering table: O(num_of_vars^2) i.e. stupid but working.
   * Note the temporary use of indices array. */
  for(i = 0; i < num_of_vars; i++){
    temp = get_id(vars[i]);
    for(j = 0; j < num_of_vars; j++){
      if(get_id(vars[j]) > temp)
	indices[j]++; /* counts how many greater variables there are */
    }
  }

  for(i = 0; i < num_of_vars; i++)
    reorder[indices[i]] = i; /* fill the reordering */


  /* JJ_NOTE: reordering probably not required anymore... */
  for(i = 0; i < num_of_vars; i++){
    cardinality[i] = vars[reorder[i]]->cardinality;
    s->variables[i] = vars[reorder[i]];
  }

  s->old = make_potential(cardinality, num_of_vars, NULL);
  s->new = make_potential(cardinality, num_of_vars, NULL);

  /* Propagation of error */
  if(s->old == NULL || s->new == NULL){
    free(cardinality);
    free(indices);
    free(reorder);
    free(s->cliques);
    free(s->variables);
    free_potential(s->old);
    free_potential(s->new);
    free(s);
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return NULL;
  }

  free(cardinality); /* the array was copied ? */
  free(reorder);
  free(indices);
  s->cliques[0] = cliques[0];
  s->cliques[1] = cliques[1];

  return s;
}


void free_Sepset(Sepset s){

  if(s){
    if(s->old->num_of_vars)
      free(s->variables);
    
    free_potential(s->old);
    free_potential(s->new);
    free(s->cliques);
    free(s);
  }

  return;
}


potential create_Potential(Variable variables[], int num_of_vars, 
			   double data[]){
  /*
   * Suppose we get an array of Variables with IDs {5, 2, 3, 4, 0, 1}.
   * In this case, temp_array will be              {5, 2, 3, 4, 0, 1},
   * and reorder will be                           {4, 5, 1, 2, 3, 0}.
   *
   * If we get Variables with IDs                  {6, 9, 3, 1, 5, 8},
   * temp_array will be                            {3, 5, 1, 0, 2, 4},
   * and reorder will be                           {3, 2, 4, 0, 5, 1}.
   *
   * temp_array is an array {x_0, x_1, ..., x_N-1}, where x_i
   * is a number indicating how many smaller IDs there are in
   * the variables[] array than the ID of variables[i]
   *
   * reorder is an array {x_0, x_1, ..., x_N-1}, where x_i is
   * the index in variables[] of the Variable with the (i+1) -smallest ID.
   * For example, x_0 tells us where in the original array we can find
   * the Variable with the smallest ID.
   */

  int i, j, card_temp, index, size_of_data = 1;

  int *cardinality;
  int *indices;
  int *temp_array;
  int *reorder;

  unsigned long temp;
  potential p;

  if((cardinality = (int *) malloc(num_of_vars * sizeof(int))) == NULL) {
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  if((indices = (int *) malloc(num_of_vars * sizeof(int))) == NULL) {
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(cardinality);
    return NULL;
  }

  if((temp_array = (int *) malloc(num_of_vars * sizeof(int))) == NULL) {
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(cardinality);
    free(indices);
    return NULL;
  }

  if((reorder = (int *) malloc(num_of_vars * sizeof(int))) == NULL) {
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(cardinality);
    free(indices);
    free(temp_array);
    return NULL;
  }

  /* reorder[i] is the place of i:th variable (in the sense of this program) 
   * in the array variables[] */

  /* init (needed?) */
  for(i = 0; i < num_of_vars; i++)
    temp_array[i] = 0;

  /* Create the reordering table: O(num_of_vars^2) i.e. stupid but working.
   * Note the temporary use of indices array. */
  for(i = 0; i < num_of_vars; i++){
    temp = get_id(variables[i]);
    for(j = 0; j < num_of_vars; j++){
      if(get_id(variables[j]) > temp)
	temp_array[j]++; /* counts how many greater variables there are */
    }
  }

  for(i = 0; i < num_of_vars; i++)
    reorder[temp_array[i]] = i; /* fill the reordering */

  /* Figure out some stuff */
  for(i = 0; i < num_of_vars; i++){
    size_of_data *= variables[i]->cardinality; /* optimal? */
    cardinality[i] = variables[reorder[i]]->cardinality;
  }

  /* Create a potential */
  p = make_potential(cardinality, num_of_vars, NULL);

  /* Propagation of error */
  if(p == NULL){
    free(cardinality);
    free(indices);
    free(temp_array);
    free(reorder);
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return NULL;
  }
  
  /*
   * NULL data is accepted.
   * In that case, potential will be uniformly distributed.
   */
  if(data != NULL)

    /* Copy the contents to their correct places 
     * JJT: In principle it is a bit ugly to do this 
     * at this level. If potential.c changes, this has to 
     * be revised too!!! */
    for(i = 0; i < size_of_data; i++){

      /*
       * Now this is the trickiest part.
       * Find out indices (in the internal order of the program,
       * determined by the Variable IDs).
       */
      inverse_mapping(p, i, indices); 

      /* calculate the address in the original array */
      index = 0;
      card_temp = 1;

      /* THE mapping */
      for(j = 0; j < num_of_vars; j++){
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


double *reorder_potential(Variable vars[], potential p){

  int old_flat_index, new_flat_index;
  int i, j;
  int *old_indices, *new_indices;
  int *new_card;
  unsigned long smallest_id;
  unsigned long this_id;
  unsigned long biggest_taken;
  int smallest_index = 0;
  double *new_data;
  int card_temp;

  /* Simple (stupid) checks */
  if(!p){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }
  if(!vars){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }

  old_indices = (int *) calloc(p->num_of_vars, sizeof(int));
  if(!old_indices){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  new_indices = (int *) calloc(p->num_of_vars, sizeof(int));
  if(!new_indices){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(old_indices);
    return NULL;
  }

  new_card = (int *) calloc(p->num_of_vars, sizeof(int));
  if(!new_card){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(old_indices);
    free(new_indices);
    return NULL;
  }

  new_data = (double *) calloc(p->size_of_data, sizeof(double));
  if(!new_data){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(old_indices);
    free(new_indices);
    free(new_card);
    return NULL;
  }


  for(old_flat_index = 0; old_flat_index < p->size_of_data; old_flat_index++){

    /* 1. old_flat_index -> old_indices (inverse_mapping) */
    inverse_mapping(p, old_flat_index, old_indices);


    /* FIXME: This is totally wrong. */
    /* 2. "old_indices" is re-ordered according to "variables"
     *    -> new_indices */
    for(i = 0; i < p->num_of_vars; i++){

      for(j = 0; j < p->num_of_vars; j++){
	this_id = vars[j]->id;
	if(j != 0 && i != 0){
	  if(this_id < smallest_id &&
	     this_id > biggest_taken){
	    smallest_index = j;
	    smallest_id = this_id;
	  }
	}
	else if(j != 0 && i == 0){
	  if(this_id < smallest_id){
	    smallest_index = j;
	    smallest_id = this_id;
	  }
	}
	else if(j == 0 && i != 0){
	  if(this_id > biggest_taken){
	    smallest_index = j;
	    smallest_id = this_id;
	  }
	}
	else{
	  smallest_index = j;
	  smallest_id = this_id;
	}
      }

      new_indices[smallest_index] = old_indices[i];
      new_card[smallest_index] = p->cardinality[i];
      biggest_taken = vars[smallest_index]->id;
    }
    /* <\totally wrong> */




    /* 3. new_indices -> new_flat_index (look at get_ppointer()) */
    new_flat_index = 0;
    card_temp = 1;

    for(i = 0; i < p->num_of_vars; i++){
      new_flat_index += new_indices[i] * card_temp;
      card_temp *= new_card[i];
    }

    printf("New flat index == %d\n", new_flat_index);

    /* 4. */
    new_data[new_flat_index] = p->data[old_flat_index];

  }

  free(old_indices);
  free(new_indices);
  free(new_card);

  /* Pointer to allocated memory. Potential p remains alive also. */
  return new_data;

}


void unmark_Clique(Clique c){
  if(c != NULL)
    c->mark = 0;
  return;
}


/*
 * Returns 0 if clique is not marked, 1 if it is. (This could be a macro...)
 */
static int clique_marked(Clique c){
  return c->mark;
}


int clique_num_of_vars(Clique c){
  return c->p->num_of_vars; /* macro? */
}


int sepset_num_of_vars(Sepset s){
  return s->old->num_of_vars; /* macro? */
}


int distribute_evidence(Clique c){

  int retval;
  Sepset_link l;
  Sepset s;

#ifdef DEBUG_CLIQUE
  printf("Distributing evidence in ");
  print_Clique(c);
#endif

  /* mark */
  c->mark = 1;

  /* pass the messages */
  l = c->sepsets;
  while (l != 0){
    s = l->data;
    if(!clique_marked(s->cliques[0])){
      retval = message_pass(c, s, s->cliques[0]); /* pass a message */
      if(retval != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }
    }
    else if(!clique_marked(s->cliques[1])){
      retval = message_pass(c, s, s->cliques[1]); /* pass a message */
      if(retval != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }
    }
    l = l->fwd;
  }

  /* call neighboring cliques */
  l = c->sepsets;
  while (l != 0){
    s = l->data;
    if(!clique_marked(s->cliques[0])){
      if(distribute_evidence(s->cliques[0]) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }
    }
    else if(!clique_marked(s->cliques[1])){
      if(distribute_evidence(s->cliques[1]) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }
    }
    l = l->fwd;
  }
  return NO_ERROR;
}


int collect_evidence(Clique c1, Sepset s12, Clique c2){

  int retval;
  Sepset_link l;
  Sepset s;

  /* mark */
  c2->mark = 1;

  /* call neighboring cliques */
  l = c2->sepsets;
  while (l != NULL){
    s = l->data;
    if(!clique_marked(s->cliques[0])){
      if(collect_evidence(c2, s, s->cliques[0]) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }
    }
    else if(!clique_marked(s->cliques[1]))
      if(collect_evidence(c2, s, s->cliques[1]) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }
    l = l->fwd;
  }

  /* pass the message to c1 */
  if((c1 != NULL) && (s12 != NULL)){
    retval = message_pass(c2, s12, c1);
    if(retval != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      return ERROR_GENERAL;
    }
  }

#ifdef DEBUG_CLIQUE
  if(c1 != NULL && c2 != NULL){
    printf("Collecting evidence from ");
    print_Clique(c2);
    printf(" to ");
    print_Clique(c1);
  }
#endif

  return NO_ERROR;
}


/*
 * Method for passing messages between cliques.
 * The message goes from Clique c1 through Sepset s to Clique c2.
 * Returns an error code.
 */
static int message_pass(Clique c1, Sepset s, Clique c2){
  int i, j = 0, k = 0;
  int retval;
  int *mapping;

  /* save the newer potential as old by switching the pointers */
  potential temp;
  temp = s->old;
  s->old = s->new;
  s->new = temp;

  mapping = (int *) calloc(s->new->num_of_vars, sizeof(int));
  if(!mapping){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  /*
   * Marginalise (projection).
   * First: select the variables. This takes O(n^2)
   */
  for(i=0; i < c1->p->num_of_vars; i++){
    if(k == s->new->num_of_vars)
      break; /* all found */
    for(j=0; j < s->new->num_of_vars; j++)
      if(equal_variables((c1->variables)[i], (s->variables)[j])){
	mapping[j] = i;
	k++;
      }
  }

  /* Information flows from Clique c1 to Sepset s. */
  retval = general_marginalise(c1->p, s->new, mapping);
  if(retval != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    free(mapping);
    return ERROR_GENERAL;
  }

  j = 0; k = 0;

  /*
   * Update (absorption).
   * First: select the variables. This relies on the order of variables.
   */
  for(i=0; i < c2->p->num_of_vars; i++){
    if(k == s->new->num_of_vars)
      break; /* all found */
    for(j=0; j < s->new->num_of_vars; j++)
      if(equal_variables((c2->variables)[i], (s->variables)[j])){
	mapping[j] = i;
	k++;
      }
  }

  /* Information flows from Sepset s to Clique c2. */
  retval = update_potential(s->new, s->old, c2->p, mapping);
  if(retval != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    free(mapping);
    return ERROR_GENERAL;
  }

  free(mapping);

  return NO_ERROR;
}


int initialise(Clique c, Variable child, Variable parents[], potential p,
	       int transient){
  int i, j = 0, k = 0;
  int *mapping = NULL;
  int retval;
  Variable var = NULL;

  if(p->num_of_vars < c->p->num_of_vars){
    mapping = (int *) calloc(p->num_of_vars, sizeof(int));
    if(!mapping){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return ERROR_OUTOFMEMORY;
    }    
    
    /***************************************************************/
    /* HEY! parents[] NOT assumed to be in any particular order!   */
    /* But the variables of p are assumed to be in the same order  */
    /* as in the clique c!                                         */
    /***************************************************************/
    
    /* initialisation with conditional distributions 
       first: select the variables (in a stupid but working way) */
    for(i=0; i < c->p->num_of_vars; i++){
      if(k == p->num_of_vars)
	break; /* all found */
      var = (c->variables)[i];
      
      for(j=0; j < p->num_of_vars - 1; j++)
	if(equal_variables(var, parents[j]))
	  mapping[k++] = i;
      
      if(equal_variables(var, child))
	mapping[k++] = i;
    }
  }

  /* rest the case */
  retval = init_potential(p, c->p, mapping);
  if(retval != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return ERROR_GENERAL;
  }

  /* Some extra work is done here,
   * because only the last initialisation counts. */
  if(!transient){
    retval = init_potential(p, c->original_p, mapping);
    if(retval != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      return ERROR_GENERAL;
    }
  }
  /* This should be an optional phase: in timeslice models 
   * some clique potentials need to be initialised but still 
   * be retractable... */

  free(mapping); /* free(NULL) is O.K. */
  return NO_ERROR;
}


int marginalise(Clique c, Variable v, double r[]){
  int index = var_index(c, v);
  int retval;

  /* Variable not in this Clique => ERROR */
  if(index == -1){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }
  
  retval = total_marginalise(c->p, r, index);
  if(retval != NO_ERROR)
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);

  return(retval);
}


void normalise(double result[], int array_size){
  int i;
  double sum = 0;
  for(i = 0; i < array_size; i++)
    sum += result[i];
  if(sum == 0)
    return;
  for(i = 0; i < array_size; i++)
    result[i] /= sum;
  return;
}


int global_retraction(Variable* vars, int nvars, Clique* cliques, 
		      int ncliques){

  int i, index;
  int retval;
  Variable v;
  Clique c;

  for(index = 0; index < ncliques; index++)
    unmark_Clique(cliques[index]);

  /* Reset all the potentials back to original. */
  jtree_dfs(cliques[0], retract_Clique, retract_Sepset);

  /* Enter evidence back to the join tree. */
  for(i = 0; i < nvars; i++){
    v = vars[i];
    c = find_family(cliques, ncliques, v);
    index = var_index(c, v);

    retval = update_evidence(v->likelihood, NULL, c->p, index);
    if(retval != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      return ERROR_GENERAL;
    }
  }

  return NO_ERROR;
}


int enter_observation(Variable* vars, int nvars, Clique* cliques, 
		      int ncliques, Variable v, char *state){
  int index = get_stateindex(v, state);

  return enter_i_observation(vars, nvars, cliques, ncliques, v, index);
}


int enter_i_observation(Variable* vars, int nvars, Clique* cliques, 
			int ncliques, Variable v, int index){
  int i, retval;
  double *evidence = (double *) calloc(v->cardinality, sizeof(double));

  if(!evidence){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  for(i = 0; i < v->cardinality; i++)
    if(i == index)
      evidence[i] = 1;
    else
      evidence[i] = 0;

  retval = enter_evidence(vars, nvars, cliques, ncliques, v, evidence);
  free(evidence);

  return retval;
}


int enter_evidence(Variable* vars, int nvars, Clique* cliques, 
		   int ncliques, Variable v, double evidence[]){
  int index, i;
  int retraction = 0;
  int retval;
  Clique c;

  if(v == NULL || evidence == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return ERROR_NULLPOINTER;
  }

  c = find_family(cliques, ncliques, v);
    
  if(!c){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 0);
    return ERROR_INVALID_ARGUMENT;
  }

  index = var_index(c, v);

  for(i = 0; i < v->cardinality; i++)
    if((v->likelihood)[i] == 0 && evidence[i] != 0)
      retraction = 1; /* Must do global retraction! */
  
  /*
   * Here is the update of clique potential.
   * MUST be done before update_likelihood.
   */
  if(!retraction){
    retval = update_evidence(evidence, v->likelihood, c->p, index);
    if(retval != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      return ERROR_GENERAL;
    }
  }

  /* Update likelihood. Check the return value. */
  retval = update_likelihood(v, evidence);
  if(retval != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return ERROR_GENERAL;
  }

  if(retraction){
    retval = global_retraction(vars, nvars, cliques, ncliques);
    if(retval != NO_ERROR)
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return(retval);
  }

  return NO_ERROR;
}


/*
 * Method for checking if Variable v is part of Clique c.
 * Returns -1 if not, else the index of v among the Variables in c.
 */
static int var_index(Clique c, Variable v){
  int var = 0;

  if(!(c && v)){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return -1;
  }

  while(!equal_variables(v, c->variables[var])){
    var++;
    if(var == c->p->num_of_vars)
      /* Variable not in this Clique => -1 */
      return -1;
  }
  return var;
}


Clique find_family(Clique *cliques, int num_of_cliques, Variable var){
  int i, n;
  Variable *family = NULL;
  Clique found;

  /* NOTE: uses memoization for finding families */
  if(var->family_clique != NULL)
    return (Clique)(var->family_clique);

  n = number_of_parents(var);
  family = (Variable*) calloc(n + 1, sizeof(Variable));
  if(!family){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  for(i = 0; i < n; i++)
    family[i] = var->parents[i];
  family[n] = var;

  found = find_clique(cliques, num_of_cliques, family, n+1);
  var->family_clique = found; /* MEMOIZE! */
  return found;
}


Clique find_clique(Clique *cliques, int num_of_cliques, 
		   Variable *variables, int num_of_vars){
  int i, j, k;
  int ok;

  for(i = 0; i < num_of_cliques; i++){
    ok = 0;

    for(j = 0; j < num_of_vars; j++){

      for(k = 0; k < cliques[i]->p->num_of_vars; k++){
	if(equal_variables(variables[j], cliques[i]->variables[k])){
	  ok++;
	  break; /* found the variable in the clique */
	}
      }

    }

    /* All variables found in the clique */
    if(ok == num_of_vars)
      return cliques[i];
  }

  return NULL;

}


int find_sepsets(Clique *cliques, int num_of_cliques){

  int inserted = 0;
  int i;
  Sepset s;
  Clique one, two;

#ifdef DEBUG_CLIQUE
  int j, k;
  int ok = 1;
#endif

  Heap *H = build_sepset_heap(cliques, num_of_cliques);

  if(!H){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return ERROR_GENERAL;
  }

  while(inserted < num_of_cliques - 1){

    /* Extract the "best" candidate sepset from the heap. */
    if(extract_min_sepset(H, &s)){

      /* In case of error */
      free_heap(H);
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      return ERROR_GENERAL;
    }

    one = s->cliques[0];
    two = s->cliques[1];

    /* Unmark MUST be done before searching (clique_search). */
    for(i = 0; i < num_of_cliques; i++)
      unmark_Clique(cliques[i]);

    /* Prevent loops by checking if the Cliques
     * are already in the same tree. */
    if(!clique_search(one, two)){

      mark_useful_Sepset(H, s); /* MVK */

#ifdef DEBUG_CLIQUE
      printf("In Clique.c: Trying to add ");
      print_Sepset(s);

      printf(" to ");
      print_Clique(one);

      printf(" and ");
      print_Clique(two);
#endif

      if(add_Sepset(one, s) != NO_ERROR){
	free_heap(H);
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }
	
      if(add_Sepset(two, s) != NO_ERROR){
	free_heap(H);
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }

      inserted++;
    }

  }

  free_heap(H);

#ifdef DEBUG_CLIQUE
  for(i = 0; i < num_of_cliques - 1; i++)
    for(j = i + 1; j < num_of_cliques; j++){

      /* Unmark MUST be done before searching (clique_search). */
      for(k = 0; k < num_of_cliques; k++)
	unmark_Clique(cliques[k]);

      if(!clique_search(cliques[i], cliques[j])){
	ok = 0;
	printf("No connection between ");
	print_Clique(cliques[i]);

	printf(" and ");
	print_Clique(cliques[j]);
      }
    }
  if(ok)
    printf("All connections between Cliques are OK!\n");
#endif

  return NO_ERROR;

}


/*
 * Finds out if two Cliques are in the same tree.
 * Returns 1 if they are, 0 if not.
 * Cliques must be unmarked before calling this.
 */
static int clique_search(Clique one, Clique two){
  Sepset_link l = one->sepsets;
  Sepset s;

#ifdef DEBUG_CLIQUE
  printf("clique_search in ");
  print_Clique(one);
#endif

  if(one == NULL || two == NULL) /* normal situation or an error? */
    return 0; /* as in FALSE */

  /* mark */
  one->mark = 1;

  /* NOTE: this defines the equality of cliques. */
  if(one == two)
    return 1; /* TRUE (end of recursion) */

#ifdef DEBUG_CLIQUE
  if(l == NULL){
    printf("In Clique.c: No Sepsets attached to ");
    print_Clique(one);
  }
#endif

  /* call neighboring cliques */
  while (l != NULL){
    s = (Sepset)(l->data);
    if(!clique_marked(s->cliques[0])){
#ifdef DEBUG_CLIQUE
      printf("In clique_search: s->cliques[0] not marked.\n");
#endif
      if(clique_search(s->cliques[0], two))
	return 1; /* TRUE */
    }
    else if(!clique_marked(s->cliques[1])){
#ifdef DEBUG_CLIQUE
      printf("In clique_search: s->cliques[1] not marked.\n");
#endif
      if(clique_search(s->cliques[1], two))
	return 1; /* TRUE */
    }
    l = l->fwd;
  }

  return 0; /* FALSE */
}


void print_Clique(Clique c){

  int i;

  printf("Clique ");
  for(i = 0; i < c->p->num_of_vars; i++)
    printf("%s ", c->variables[i]->symbol);
  printf("\n");

}


void print_Sepset(Sepset s){

  int i;

  printf("Sepset ");
  for(i = 0; i < s->old->num_of_vars; i++)
    printf("%s ", s->variables[i]->symbol);
  printf("\n");

}


static void retract_Clique(Clique c){

  copy_potential(c->original_p, c->p);

}


static void retract_Sepset(Sepset s){

  int i;

  for(i = 0; i < s->old->size_of_data; i++){
    s->old->data[i] = 1;
    s->new->data[i] = 1;
  }

}


/*
 * A generic function for traversing the join tree. 
 * Cliques must be unmarked before calling this.
 * Parameters:
 * - a clique where the DFS starts
 * - a function pointer to the function to be used for every Clique on the way
 * - a function pointer to the function to be used for every Sepset on the way
 */
static void jtree_dfs(Clique start, void (*cFuncPointer)(Clique),
		      void (*sFuncPointer)(Sepset)){
  /* a lot of copy-paste from collect/distribute_evidence and clique_search */

  Sepset_link l = start->sepsets;
  Sepset s;
  
  if(start == NULL) /* error? */
    return;

  /* mark */
  start->mark = 1;

  if(cFuncPointer)
    cFuncPointer(start); /* do it now or after the children ??? */

  /* call neighboring cliques */
  while (l != NULL){
    s = (Sepset)(l->data);
    if(!clique_marked(s->cliques[0])){
      if(sFuncPointer)
	sFuncPointer(s);
      jtree_dfs(s->cliques[0], cFuncPointer, sFuncPointer);
    }
    else if(!clique_marked(s->cliques[1])){
      if(sFuncPointer)
	sFuncPointer(s);
      jtree_dfs(s->cliques[1], cFuncPointer, sFuncPointer);
    }
    l = l->fwd;
  }
}


int clique_intersection(Clique cl1, Clique cl2, Variable **vars, int *n){

  int i, j;
  int max_vars = cl2->p->num_of_vars;
  int realsize = 0;
  Variable *isect;
  Variable *shaved_isect;

  if(cl1->p->num_of_vars < max_vars)
    max_vars = cl1->p->num_of_vars;

  isect = (Variable *) calloc(max_vars, sizeof(Variable));

  if(!isect){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  for(i = 0; i < cl1->p->num_of_vars; i++)

    for(j = 0; j < cl2->p->num_of_vars; j++){

      if(equal_variables(cl1->variables[i], cl2->variables[j]))
	isect[realsize++] = cl1->variables[i];
    }

  if(realsize == 0){
    free(isect);
    *vars = NULL;
    *n = 0;
    return NO_ERROR;
  }

  /* Intersection is non-empty, realsize > 0 */

  shaved_isect = (Variable *) calloc(realsize, sizeof(Variable));

  if(!shaved_isect){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(isect);
    return ERROR_OUTOFMEMORY;
  }

  for(i = 0; i < realsize; i++)
    shaved_isect[i] = isect[i];
  
  free(isect);
  *vars = shaved_isect;
  *n = realsize;

  return NO_ERROR;

}

