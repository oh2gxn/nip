#include <stdlib.h>
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

Clique make_Clique(Variable vars[], int num_of_vars){
  Clique c = (Clique) malloc(sizeof(cliquetype));
  int *cardinality = (int *) calloc(num_of_vars, sizeof(int));
  int i;
  c->variables = (Variable *) calloc(num_of_vars, sizeof(Variable));
  for(i = 0; i < num_of_vars; i++){
    cardinality[i] = vars[i]->cardinality;
    c->variables[i] = vars[i];
  }
  c->p = make_potential(cardinality, num_of_vars, NULL);
  free(cardinality);
  c->sepsets = 0;
  c->mark = 0;
  return c;
}


int free_Clique(Clique c){
  if(c == NULL)
    return ERROR_NULLPOINTER;
  /* clean the list of sepsets */
  link l1 = c->sepsets;
  link l2 = c->sepsets;
  while(l1 != 0){
    l2 = l1->fwd;
    free(l1);
    l1=l2;
  }
  /* clean the rest */
  free_potential(c->p);
  free(c->variables);
  free(c);
  return 0;
}


int add_Sepset(Clique c, Sepset s){
  link new = (link) malloc(sizeof(element));
  new->data = s;
  new->fwd = c->sepsets;
  new->bwd = 0;
  if(c->sepsets != 0)
    c->sepsets->bwd = new;
  c->sepsets = new;
  return 0;
}


Sepset make_Sepset(Variable vars[], int num_of_vars, Clique cliques[]){
  Sepset s = (Sepset) malloc(sizeof(sepsettype));
  s->cliques = (Clique *) calloc(2, sizeof(Clique));
  int *cardinality = (int *) calloc(num_of_vars, sizeof(int));
  int i;
  s->variables = (Variable *) calloc(num_of_vars, sizeof(Variable));
  for(i = 0; i < num_of_vars; i++){
    cardinality[i] = vars[i]->cardinality;
    s->variables[i] = vars[i];
  }
  s->old = make_potential(cardinality, num_of_vars, NULL);
  s->new = make_potential(cardinality, num_of_vars, NULL);
  free(cardinality); /* the array was copied ? */
  s->cliques[0] = cliques[0]; /* Hopefully the space was allocated! */
  s->cliques[1] = cliques[1];
  return s;
}


int free_Sepset(Sepset s){
  free_potential(s->old);
  free_potential(s->new);
  free(s->variables);
  free(s->cliques);
  free(s);
  return 0;
}


potential create_Potential(Variable variables[], int num_of_vars, 
			   double data[]){
  // THIS IS TRICKY: we have to reorder the array and stuff...
  int i, j, card_temp, index, size_of_data = 1;
  int cardinality[num_of_vars];
  int indices[num_of_vars];
  int reorder[num_of_vars];
  unsigned long temp;
  potential p;

  /* reorder[i] is the place of i:th variable (in the sense of this program) 
   * in the array variables[] */

  /* init (needed?) */
  for(i = 0; i < num_of_vars; i++)
    indices[i] = 0;

  /* Create the reordering table: O(num_of_vars^2) i.e. stupid but working.
   * Note the temporary use of indices array. */
  for(i = 0; i < num_of_vars; i++){
    temp = get_id(variables[i]);
    for(j = 0; j < num_of_vars; j++){
      if(get_id(variables[j]) > temp)
	indices[j]++; // counts how many greater variables there are
    }
  }
  
  /* Figure out some stuff */
  for(i = 0; i < num_of_vars; i++){
    reorder[indices[i]] = i; // fill the reordering
    size_of_data *= variables[i]->cardinality; // optimal?
    cardinality[i] = variables[reorder[i]]->cardinality;
  }

  /* Create a potential */
  p = make_potential(cardinality, num_of_vars, NULL);
  
  /* Copy the contents to their correct places */
  for(i = 0; i < size_of_data; i++){
    /* Now this is the trickiest part */
    // find out indices
    inverse_mapping(p, i, indices); 

    // calculate the address in the original array
    index = indices[reorder[0]];
    card_temp = 1;
    /* THE mapping */
    for(i = 1; i < num_of_vars; i++){
      card_temp *= cardinality[i-1];
      index += indices[reorder[i]] * card_temp;
    }

    // set the value (in a little ugly way)
    p->data[i] = data[index];
  }

  return p;
}


int free_Potential(potential p){
  return free_potential(p);
}


int unmark_Clique(Clique c){
  c->mark = 0;
  return 0;
}


int distribute_evidence(Clique c){
  /* mark */
  c->mark = 1;

  /* pass the messages */
  link l = c->sepsets;
  Sepset s;
  while (l != 0){
    s = l->data;
    if(s->cliques[0]->mark == 0)
      message_pass(c, s, s->cliques[0]); /* pass a message */
    else if(s->cliques[1]->mark == 0)
      message_pass(c, s, s->cliques[1]); /* pass a message */
    l = l->fwd;
  }

  /* call neighboring cliques */
  l = c->sepsets;
  while (l != 0){
    s = l->data;
    if(s->cliques[0]->mark == 0)
      distribute_evidence(s->cliques[0]);
    else if(s->cliques[1]->mark == 0)
      distribute_evidence(s->cliques[1]);
    l = l->fwd;
  }
  return 0;
}


int collect_evidence(Clique c1, Sepset s12, Clique c2){
  /* mark */
  c2->mark = 1;

  /* call neighboring cliques */
  link l = c2->sepsets;
  Sepset s;
  while (l != 0){
    s = l->data;
    if(s->cliques[0]->mark == 0)
      collect_evidence(c2, s, s->cliques[0]);
    else if(s->cliques[1]->mark == 0)
      collect_evidence(c2, s, s->cliques[1]);
    l = l->fwd;
  }

  /* pass the message to c1 */
  if((c1 != 0) && (s12 != 0))
    message_pass(c2, s12, c1);

  return 0;
}


int message_pass(Clique c1, Sepset s, Clique c2){
  int i, j = 0, k = 0;
  int source_vars[c1->p->num_of_vars - s->new->num_of_vars];
  int extra_vars[c2->p->num_of_vars - s->new->num_of_vars];
  /* save the newer potential as old by switching the pointers */
  potential temp;
  temp = s->old;
  s->old = s->new;
  s->new = temp;

  /* marginalise (projection)
     first: select the variables */
  for(i=0; i < c1->p->num_of_vars; i++){
    if(j < s->new->num_of_vars &&
       equal_variables((c1->variables)[i], (s->variables)[j]))
      j++;
    else {
      source_vars[k] = i;
      k++;
    }
  } /* then: do da job */
  general_marginalise(c1->p, s->new, source_vars);

  j = 0; k = 0;
  /* update (absorption) 
     first: select the variables */
  for(i=0; i < c2->p->num_of_vars; i++){
    if(j < s->new->num_of_vars &&
       equal_variables((c2->variables)[i], (s->variables)[j]))
      j++;
    else {
      extra_vars[k] = i;
      k++;
    }
  } /* rest the case */
  update_potential(s->new, s->old, c2->p, extra_vars);
  return 0;
}


int initialise(Clique c, Variable v, Variable parents[], potential p){
  int i, j = 0, k = 0;
  int diff = c->p->num_of_vars - p->num_of_vars;
  int *extra_vars = NULL;

  if(diff > 0)
    extra_vars = (int *) calloc(diff, sizeof(int));

  /* UNFINISHED! Who can make such a potential and how??? */

  /* JJ_NOTE: I'm convinced that there is a bug somewhere!
   * What happens for example when diff==0 ?!?!? 
   * - Fixed 18.5.2004 */

  /* initialisation with conditional distributions 
     first: select the variables */
  for(i=0; i < c->p->num_of_vars; i++){
    if(j < (p->num_of_vars - 1) &&
       equal_variables((c->variables)[i], parents[j])) /* or v? */
      j++;                                             /* MVK: Look down */
    else if(!equal_variables((c->variables)[i], v)){
      if(diff > 0)
	extra_vars[k++] = i;
      // else there is an ERROR!!! (Namely invalid arguments)
    }
    /* Else, if equal_variables((c->variables)[i], v), do nothing. */
  }
  /* rest the case */
  i = init_potential(p, c->p, extra_vars);
  free(extra_vars);
  return (i);
}


int marginalise(Clique c, Variable v, double r[]){
  int index = var_index(c, v);

  /* Variable not in this Clique => ERROR */
  if(index == -1)
    return ERROR_INVALID_ARGUMENT;
  
  return(total_marginalise(c->p, r, index));
}


int normalise(double result[], int array_size){
  int i;
  double sum = 0;
  for(i = 0; i < array_size; i++)
    sum += result[i];
  if(sum == 0)
    return 0;
  for(i = 0; i < array_size; i++)
    result[i] /= sum;
  return 0;
}


int enter_evidence(Clique c, Variable v, double evidence[]){
  int index, i;

  if(c == NULL || v == NULL || evidence == NULL)
    return ERROR_NULLPOINTER;
    
  index = var_index(c, v);

  /* Variable not in this Clique => ERROR */
  if(index == -1)
    return ERROR_INVALID_ARGUMENT;

  for(i = 0; i < v->cardinality; i++)
    if((v->likelihood)[i] == 0 && evidence[i] != 0)
      return GLOBAL_RETRACTION;
  
  /* Here is the update of clique potential. */
  update_evidence(evidence, v->likelihood, c->p, index);

  /* update likelihood */
  update_likelihood(v, evidence);

  /* GLOBAL UPDATE or GLOBAL RETRACTION probably needed !!! */
  
  /* JJ NOTE: update_evidence or enter_evidence should be 
   *          'fail fast' and there could be global retraction 
   *	      upon failure. 
   *	      Zeros are a menace! */

  return 0;
}


int var_index(Clique c, Variable v){
  int var = 0;
  while(!equal_variables(v, c->variables[var])){
    var++;
    if(var == c->p->num_of_vars)
      /* Variable not in this Clique => -1 */
      return -1;
  }
  return var;
}
