/*
 * nip.c $Id: nip.c,v 1.11 2004-08-26 12:59:25 mvkorpel Exp $
 */

#include "nip.h"
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "errorhandler.h"
#include <stdlib.h>
#include <string.h>

extern int yyparse();

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

  /* 1. Find the Clique that contains the family of 
   *    the interesting Variable */
  clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				   &v, 1);
  if(!clique_of_interest){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
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

