/*
 * nip.c $Id: nip.c,v 1.18 2004-09-07 10:16:38 jatoivol Exp $
 */

#include "nip.h"
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "errorhandler.h"
#include <stdlib.h>
#include <string.h>

/*
#define DEBUG_NIP
*/

extern int yyparse();

static int increment_indices(int indices[], Variable vars[], int num_of_vars);
static double get_data(double data[], int indices[],
		       int num_of_vars, int cardinality[]);


void reset_model(Nip model){
  int retval;
  Variable temp;
  Variable_iterator it = model->first_var;

  while((temp = next_Variable(&it)) != NULL)
    reset_likelihood(temp);
  retval = global_retraction(model->first_var, model->cliques,
			     model->num_of_cliques);
  if(retval != NO_ERROR)
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
}


Nip parse_model(char* file){
  int retval;
  Nip new = (Nip) malloc(sizeof(nip_type));

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }
  
  /* 1. Parse */
  if(open_yyparse_infile(file) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return NULL;
  }

  retval = yyparse();

  close_yyparse_infile();

  if(retval != 0)
    return NULL;

  /* 2. Get the parsed stuff and make a model out of them */
  new->num_of_cliques = get_num_of_cliques();
  new->num_of_vars = total_num_of_vars();
  new->cliques = *get_cliques_pointer();
  new->first_var = get_first_variable();
  new->last_var = get_last_variable();

  /* 3. Reset parser globals */
  reset_Variable_list();
  reset_Clique_array();

  return new;
}


void free_model(Nip model){
  int i;
  Variable_iterator it = model->first_var;
  Variable v = next_Variable(&it);

  /* 1. Free Cliques and adjacent Sepsets */
  for(i = 0; i < model->num_of_cliques; i++){
    free_Clique(model->cliques[i]);
  }
  free(model->cliques);

  /* 2. Free the Variables and the list */
  while(v != NULL){
    free_variable(v);
    if(it != NULL)
      free(it->bwd);
    v = next_Variable(&it);
  }

  free(model->last_var);
  free(model);
}


int insert_hard_evidence(Nip model, char* variable, char* observation){
  int ret;
  Variable v = get_Variable(model, variable);
  if(v == NULL)
    return ERROR_INVALID_ARGUMENT;
  ret = enter_observation(model->first_var, model->cliques,
			  model->num_of_vars, v, observation);
  if(ret != NO_ERROR)
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);

  make_consistent(model);
  return ret;
}


int insert_soft_evidence(Nip model, char* variable, double* distribution){
  int ret;
  Variable v = get_Variable(model, variable);
  if(v == NULL)
    return ERROR_INVALID_ARGUMENT;
  ret =  enter_evidence(model->first_var, model->cliques,
			model->num_of_cliques, v, distribution);
  make_consistent(model);
  return ret;
}


Variable get_Variable(Nip model, char* symbol){

  if(!model){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }

  return get_variable(model->first_var, symbol);

}


void make_consistent(Nip model){
  int i;
  for (i = 0; i < model->num_of_cliques; i++)
    unmark_Clique(model->cliques[i]);

  if(collect_evidence(NULL, NULL, model->cliques[0]) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return;
  }

  for (i = 0; i < model->num_of_cliques; i++)
    unmark_Clique(model->cliques[i]);

  if(distribute_evidence(model->cliques[0]) != NO_ERROR)
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);

  return;
}


double *get_probability(Nip model, Variable v, int print){

  Clique clique_of_interest;
  double *result;
  int cardinality;
  int i;
  char *symbol;

  if(!model){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }

  if(!v){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }

  cardinality = number_of_values(v);
  result = (double *) calloc(cardinality, sizeof(double));
  if(!result){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* 1. Find the Clique that contains the interesting Variable */
  clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				   &v, 1);
  if(!clique_of_interest){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    free(result);
    return NULL;
  }

  /* 2. Marginalisation (the memory must have been allocated) */
  marginalise(clique_of_interest, v, result);

  /* 3. Normalisation */
  normalise(result, cardinality);

  /* 4. Print result if desired */
  if(print){
    symbol = get_symbol(v);
    for(i = 0; i < cardinality; i++)
      printf("P(%s=%s) = %f\n", symbol, (v->statenames)[i], result[i]);
    printf("\n");
  }

  /* 5. Return the result */
  return result;

}


static int increment_indices(int indices[], Variable vars[], int num_of_vars){

  int finger = 0;
  int i;

  while(finger <= num_of_vars - 1){

    if(indices[finger] >= vars[finger]->cardinality - 1)
      finger++;
    else{
      indices[finger]++;
      for(i = 0; i < finger; i++)
	indices[i] = 0;
      return 1; /* TRUE */
    }
  }

  return 0; /* FALSE */
}


static double get_data(double data[], int indices[],
		       int num_of_vars, int cardinality[]){

  int index = 0;
  int i;
  int card_temp = 1;

  for(i = 0; i < num_of_vars; i++){
    index += indices[i] * card_temp;
    card_temp *= cardinality[i];
  }

#ifdef DEBUG_NIP
  printf("in get_data(): index = %d\n", index);
#endif

  return data[index];

}


double *get_joint_probability(Nip model, Variable *vars, int num_of_vars,
			      int print){

  Clique clique_of_interest;
  potential source, destination;
  int *cardinality;
  int *source_vars;
  int *indices = NULL;
  int i, j = 0, k = 0;
  int retval;
  int extra_vars;
  double *result;
  double prob;

  Variable *vars_sorted;

#ifdef DEBUG_NIP
  printf("In get_joint_probability()\n");
#endif

  /* Find the Clique that contains the family of the interesting Variables */
  clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				   vars, num_of_vars);
  if(!clique_of_interest){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return NULL;
  }

  /* Sort the Variables to program order (ascending ID) */
  vars_sorted = sort_variables(vars, num_of_vars);
  if(!vars_sorted){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return NULL;
  }

#ifdef DEBUG_NIP
  printf("Variables in given order:\n");
  for(i = 0; i < num_of_vars; i++)
    printf("Symbol: %s\tId: %ld\n", vars[i]->symbol, vars[i]->id);

  printf("\n");

  printf("Variables in sorted order:\n");
  for(i = 0; i < num_of_vars; i++)
    printf("Symbol: %s\tId: %ld\n", vars_sorted[i]->symbol, vars_sorted[i]->id);
#endif

  cardinality = (int *) calloc(num_of_vars, sizeof(int));
  if(!cardinality){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(vars_sorted);
    return NULL;
  }

  for(i = 0; i < num_of_vars; i++)
    cardinality[i] = vars_sorted[i]->cardinality;

  source = clique_of_interest->p;

  destination = make_potential(cardinality, num_of_vars, NULL);
  if(!destination){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    free(vars_sorted);
    free(cardinality);
    return NULL;
  }

  extra_vars = clique_of_interest->p->num_of_vars - num_of_vars;

  source_vars = (int *) calloc(extra_vars, sizeof(int));
  if(!source_vars){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(vars_sorted);
    free(cardinality);
    free_potential(destination);
    return NULL;
  }

  /* Select the variables that are in the Clique but not in the set of
   * Variables given to this function.
   * This relies on the order of variables. */
  for(i=0; i < clique_of_interest->p->num_of_vars; i++){
    if(j < num_of_vars &&
       equal_variables((clique_of_interest->variables)[i], vars_sorted[j]))
      j++;
    else {
      source_vars[k] = i;
      k++;
    }
  }

  retval = general_marginalise(source, destination, source_vars);
  if(retval != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    free(vars_sorted);
    free(cardinality);
    free(source_vars);
    return NULL;
  }

#ifdef DEBUG_NIP
  for(i = 0; i < destination->size_of_data; i++)
    printf("in get_joint_probability(): destination->data[%d] = %f\n",
	   i, destination->data[i]);
#endif

  result = reorder_potential(vars, destination);

#ifdef DEBUG_NIP
  for(i = 0; i < destination->size_of_data; i++)
    printf("in get_joint_probability(): result[%d] = %f\n",
	   i, result[i]);
#endif

  /* NORMALISE? */

  /* Print the result if print != 0 */
  if(print){

    /* Create and initialise the array of indices */
    indices = (int *) calloc(num_of_vars, sizeof(int));
    if(!indices){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free_potential(destination);
      free(vars_sorted);
      free(cardinality);
      free(source_vars);
      return NULL;
    }
  
    for(i = 0; i < num_of_vars; i++)
      indices[i] = 0;

    /* The loop goes through every combination of values */
    do{
      prob = get_data(result, indices, num_of_vars, cardinality);
      printf("P(");
      for(i = 0; i < num_of_vars - 1; i++)
	printf("%s = %s, ", vars[i]->symbol,
	       vars[i]->statenames[indices[i]]);
      printf("%s = %s) = ", vars[num_of_vars - 1]->symbol,
	     vars[num_of_vars-1]->statenames[indices[num_of_vars-1]]);
      printf("%f\n", prob);
    } while(increment_indices(indices, vars, num_of_vars));
  }

  /* FREE MEMORY!!! */
  free_potential(destination);
  free(vars_sorted);
  free(cardinality);
  free(source_vars);
  free(indices);

  return result;

}


void print_Cliques(Nip model){

  int i;
  int num_of_cliques;
  Clique clique_of_interest;
  Sepset_link sepsetlist;
  Clique *cliques;

  if(!model){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return;
  }

  cliques = model->cliques;
  num_of_cliques = model->num_of_cliques;

  printf("Found Cliques:\n");
  for(i = 0; i < num_of_cliques; i++){
    clique_of_interest = cliques[i];
    print_Clique(clique_of_interest);
    sepsetlist = clique_of_interest->sepsets;
    while(sepsetlist){
      print_Sepset((Sepset)sepsetlist->data);
      sepsetlist = sepsetlist->fwd;
    }
    printf("\n");
  }

}
