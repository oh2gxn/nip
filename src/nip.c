/*
 * nip.c $Id: nip.c,v 1.23 2004-10-26 15:51:16 jatoivol Exp $
 */

#include "nip.h"
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "errorhandler.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
#define DEBUG_NIP
*/

/***** 
 * TODO: 
 * - some sort of a function for the forward-backward algorithm
 *   - a data type that can be returned by the algorithm (how to index?)
 *     (how about the size?  T*N*M, 
 *      where T=time, N=number of hidden variables and M is mean cardinality)
 *   + some abstraction for a time series (structure and access to data..?)
 *
 * - Viterbi algorithm for the ML-estimate of the latent variables
 *
 * - EM algorithm for estimating parameters of the model
 *****/

extern int yyparse();

static int increment_indices(int indices[], Variable vars[], int num_of_vars);
static double get_data(double data[], int indices[],
		       int num_of_vars, int cardinality[]);


void reset_model(Nip model){
  int i, retval;

  for(i = 0; i < model->num_of_vars; i++)
    reset_likelihood(model->variables[i]);
  retval = global_retraction(model->variables, model->num_of_vars, 
			     model->cliques, model->num_of_cliques);
  if(retval != NO_ERROR)
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
}


Nip parse_model(char* file){
  int i, j, retval;
  Variable temp;
  Variable_iterator it;
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

  new->variables = (Variable*) calloc(new->num_of_vars, sizeof(Variable));
  if(!(new->variables)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(new);
    return NULL;
  }
    
  it = get_first_variable();
  temp = next_Variable(&it);
  i = 0;
  while(temp != NULL){
    new->variables[i++] = temp;
    temp = next_Variable(&it);
  }
  
  for(i = 0; i < new->num_of_vars; i++)
    if(new->variables[i]->next)
      new->num_of_nexts++;

  new->next = (Variable*) calloc(new->num_of_nexts, sizeof(Variable));
  new->previous = (Variable*) calloc(new->num_of_nexts, sizeof(Variable));
  if(!(new->next && new->previous)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(new->variables);
    free(new);
    return NULL;
  }
  
  j = 0;
  for(i = 0; i < new->num_of_vars; i++)  
    if(new->variables[i]->next){
      new->next[j] = new->variables[i];
      new->previous[j] = new->variables[i]->next;
      j++;
    }

  /* Check one little detail :) 
   * NOTE: needed because of an implementation detail. (FIX IT?) 
   * NOTE2: Not anymore... this should be carefully removed. */
  j = 1;
  for(i = 1; i < new->num_of_nexts; i++)
    if(get_id(new->previous[i-1]) > get_id(new->previous[i]))
      j = 0;
  assert(j); 


  /* 3. Reset parser globals */
  it = get_last_variable(); /* free the list structure */
  while(it->bwd){
    it = it->bwd;
    free(it->fwd);
  }
  free(it);
  reset_Variable_list();
  reset_Clique_array();

  return new;
}


void free_model(Nip model){
  int i;

  /* 1. Free Cliques and adjacent Sepsets */
  for(i = 0; i < model->num_of_cliques; i++)
    free_Clique(model->cliques[i]);
  free(model->cliques);

  /* 2. Free the Variables */
  for(i = 0; i < model->num_of_vars; i++)
    free_variable(model->variables[i]);
  free(model->variables);

  free(model->next);
  free(model->previous);
  free(model);
}


Timeseries read_timeseries(Nip model, char* filename){
  int i, j, k, m;
  char** tokens = NULL;
  Timeseries ts = NULL;
  datafile* df = NULL;
  
  ts = (Timeseries) malloc(sizeof(timeseries_type));
  if(!ts){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  df = open_datafile(filename, ',', 0, 1);
  if(df == NULL){
    report_error(__FILE__, __LINE__, ERROR_FILENOTFOUND, 1);
    fprintf(stderr, "%s\n", filename);
    free(ts);
    return NULL;
  }  
  ts->length = df->datarows;

  /* Find out how many (totally) latent variables there are. */
  ts->num_of_hidden = model->num_of_vars - df->num_of_nodes;

  /* Allocate the array for the hidden variables. */
  ts->hidden = (Variable *) calloc(ts->num_of_hidden, sizeof(Variable));
  ts->observed = (Variable *) calloc(df->num_of_nodes, sizeof(Variable));
  if(!(ts->hidden && ts->observed)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(ts->hidden);
    free(ts);
    close_datafile(df);
    return NULL;
  }  

  /* Set the pointers to the hidden variables. */
  m = 0;
  for(k = 0; k < model->num_of_vars; k++){
    j = 1;
    for(i = 0; i < df->num_of_nodes; i++)
      if(equal_variables(model->variables[k], 
			 get_Variable(model, df->node_symbols[i])))
	j = 0;
    if(j)
      ts->hidden[m++] = model->variables[k];
  }

  for(i = 0; i < df->num_of_nodes; i++)
    ts->observed[i] = get_Variable(model, df->node_symbols[i]);

  /* Allocate some space for data */
  ts->data = (int**) calloc(ts->length, sizeof(int*));
  if(!(ts->data)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(ts->hidden);
    free(ts->observed);
    free(ts);
    close_datafile(df);
    return NULL;
  }
  for(i = 0; i < ts->length; i++){
    ts->data[i] = (int*) calloc(df->num_of_nodes, sizeof(int));
    if(!(ts->data[i])){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      for(j = 0; j < i; j++)
	free(ts->data[j]);
      free(ts->data);
      free(ts->hidden);
      free(ts->observed);
      free(ts);
      close_datafile(df);
      return NULL;
    }
  }

  /* Get the data */
  for(j = 0; j < ts->length; j++){
    m = nextline_tokens(df, ',', &tokens); /* 2. Read */
    assert(m == df->num_of_nodes);

    /* 3. Put into the data array */
    for(i = 0; i < m; i++){
      ts->data[j][i] = get_stateindex(get_Variable(model, 
						   df->node_symbols[i]), 
				      tokens[i]);

      /* Q: Should missing data be allowed?   A: Yes. */
      /* assert(data[j][i] >= 0); */
    }

    for(i = 0; i < m; i++) /* 4. Dump away */
      free(tokens[i]);
    free(tokens);
  }

  close_datafile(df);
  return ts;
}


void free_timeseries(Timeseries ts){
  int t;
  if(ts){
    for(t = 0; t < ts->length; t++)
      free(ts->data[t]);
    free(ts->data);
    free(ts->hidden);
    free(ts->observed);
    free(ts);
  }
}


int timeseries_length(Timeseries ts){
  return ts->length;
}


char* get_observation(Timeseries ts, Variable v, int time){
  int i, j = -1;
  for(i = 0; i < ts->model->num_of_vars - ts->num_of_hidden; i++)
    if(equal_variables(v, ts->observed[i]))
      j = i;

  if(j < 0)
    return NULL;

  return v->statenames[ts->data[time][j]];
}


int set_observation(Timeseries ts, Variable v, int time, char* observation){
  int i, j = -1;
  for(i = 0; i < ts->model->num_of_vars - ts->num_of_hidden; i++)
    if(equal_variables(v, ts->observed[i]))
      j = i;

  i = get_stateindex(v, observation);
  /* a valid variable? a valid observation for that variable? */
  if((j < 0) || (i < 0))
    return ERROR_INVALID_ARGUMENT;

  ts->data[time][j] = i;
  return 0;
}


int insert_hard_evidence(Nip model, char* variable, char* observation){
  int ret;
  Variable v = get_Variable(model, variable);
  if(v == NULL)
    return ERROR_INVALID_ARGUMENT;
  ret = enter_observation(model->variables, model->num_of_vars, 
			  model->cliques, model->num_of_cliques, 
			  v, observation);
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
  ret =  enter_evidence(model->variables, model->num_of_vars, 
			model->cliques, model->num_of_cliques, 
			v, distribution);
  make_consistent(model);
  return ret;
}


Variable get_Variable(Nip model, char* symbol){
  int i;

  if(!model){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }

  for(i = 0; i < model->num_of_vars; i++)
    if(strcmp(symbol, model->variables[i]->symbol) == 0)
      return model->variables[i];

  return NULL;
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
