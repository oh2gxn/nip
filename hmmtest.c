#include <stdlib.h>
#include <assert.h>
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"
#include "nip.h"

/*
#define PRINT_CLIQUES
*/


int main(int argc, char *argv[]){

  char** tokens;
  int** data;
  int i, j, k, l, retval, t = 0;
  int num_of_hidden = 0;
  double** quotient;
  double*** result; /* probs of the hidden variables */

  Nip model;
  Clique clique_of_interest;

  Variable *hidden;
  Variable temp;
  Variable interesting;
  Variable_iterator it;

  datafile* timeseries;

  /*************************************/
  /* Some experimental timeslice stuff */
  /*************************************/
  
  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/
  /* -- Start parsing the network definition file */
  if(argc < 3){
    printf("Give the names of the net-file and data file, please!\n");
    return 0;
  }
  else
    model = parse_model(argv[1]);

  if(model == NULL)
    return -1;
  /* The input file has been parsed. -- */

#ifdef PRINT_CLIQUES
  print_Cliques();
#endif

  /*****************************/
  /* read the data from a file */
  /*****************************/
  timeseries = open_datafile(argv[2], ',', 0, 1); /* 1. Open */
  if(timeseries == NULL){
    report_error(__FILE__, __LINE__, ERROR_FILENOTFOUND, 1);
    fprintf(stderr, "%s\n", argv[2]);
    return -1;
  }



  /* Figure out the number of hidden variables and variables that substitute
   * some other variable in the next timeslice. */
  it = model->first_var;
  temp = next_Variable(&it);
  while(temp != NULL){
    j = 1;
    for(i = 0; i < timeseries->num_of_nodes; i++)
      if(equal_variables(temp, get_Variable(model, 
					    timeseries->node_symbols[i])))
	j = 0;
    if(j)
      num_of_hidden++;
    temp = next_Variable(&it);
  }

  assert(num_of_hidden == (model->num_of_vars - timeseries->num_of_nodes));

  /* Allocate arrays for hidden variables. */
  hidden = (Variable *) calloc(num_of_hidden, sizeof(Variable));
  if(!hidden){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }

  /* Fill the arrays */
  it = model->first_var;
  temp = next_Variable(&it);
  i = 0;
  l = 0;
  while(temp != NULL){
    j = 1;
    for(k = 0; k < timeseries->num_of_nodes; k++)
      if(equal_variables(temp, get_Variable(model, 
					    timeseries->node_symbols[k])))
	j = 0;
    if(j)
      hidden[l++] = temp;
    temp = next_Variable(&it);
  }
  

  /* Allocate some space for data */
  data = (int**) calloc(timeseries->datarows, sizeof(int*));
  if(!data){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }
  for(t = 0; t < timeseries->datarows; t++){
    data[t] = (int*) calloc(timeseries->num_of_nodes, sizeof(int));
    if(!(data[t])){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }
  }


  /* Allocate some space for filtering */
  result = (double***) calloc(timeseries->datarows + 1, sizeof(double**));
  quotient = (double**) calloc(num_of_hidden, sizeof(double*));
  if(!(result && quotient)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }
  for(t = 0; t < timeseries->datarows + 1; t++){
    result[t] = (double**) calloc(num_of_hidden, sizeof(double*));
    if(!result[t]){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }

    for(i = 0; i < num_of_hidden; i++){
      result[t][i] = (double*) calloc(number_of_values(hidden[i]), 
				      sizeof(double));
      if(!result[t][i]){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
    }
  }
  for(i = 0; i < num_of_hidden; i++){
    quotient[i] = (double*) calloc(number_of_values(hidden[i]), 
				   sizeof(double));
    if(!quotient[i]){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }
  }

  /* Fill the data array */
  for(t = 0; t < timeseries->datarows; t++){
    retval = nextline_tokens(timeseries, ',', &tokens); /* 2. Read */
    assert(retval == timeseries->num_of_nodes);

    /* 3. Put into the data array */
    for(i = 0; i < retval; i++){
      data[t][i] = 
	get_stateindex(get_Variable(model, timeseries->node_symbols[i]), 
		       tokens[i]);

      /* Q: Should missing data be allowed?   A: Yes. */
      /* assert(data[t][i] >= 0); */
    }

    for(i = 0; i < retval; i++) /* 4. Dump away */
      free(tokens[i]);
  }



  /*****************/
  /* Forward phase */
  /*****************/
  printf("## Forward phase ##\n");  

  for(t = 0; t <= timeseries->datarows; t++){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t);
        
    /********************/
    /* Do the inference */
    /********************/
    
    make_consistent(model);
    
    
    /* an experimental forward phase (a.k.a. filtering)... */
    /* Calculates the result values */
    for(i = 0; i < num_of_hidden; i++){
      
      /*********************************/
      /* Check the result of inference */
      /*********************************/
      
      /* 1. Decide which Variable you are interested in */
      interesting = hidden[i];
      
      /* 2. Find the Clique that contains the family of 
       *    the interesting Variable */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       &interesting, 1);
      if(!clique_of_interest){
	printf("In hmmtest.c : No clique found! Sorry.\n");
	return 1;
      }  
      
      /* 3. Marginalisation (memory for the result must have been allocated) */
      marginalise(clique_of_interest, interesting, result[t][i]);
      
      /* 4. Normalisation */
      normalise(result[t][i], number_of_values(interesting));    
      
      /* 5. Print the result */
      for(j = 0; j < number_of_values(interesting); j++)
	printf("P(%s=%s) = %f\n", get_symbol(interesting),
	       (interesting->statenames)[j], result[t][i][j]);
      printf("\n");
    }

    if(t < timeseries->datarows){    
      /* forget old evidence */
      reset_model(model);
      
      for(i = 0; i < num_of_hidden; i++){
	/* old posteriors become new priors */
	temp = hidden[i];
	if(temp->next != NULL)
	  update_likelihood(temp->next, result[t][i]);
      }
      
      global_retraction(model->first_var, model->cliques,
			model->num_of_cliques);
      
      /* 0. Put some data in */
      for(i = 0; i < timeseries->num_of_nodes; i++)
	if(data[t][i] >= 0)
	  enter_i_observation(model->first_var, model->cliques, 
			      model->num_of_cliques,
			      get_Variable(model, 
					   timeseries->node_symbols[i]), 
			      data[t][i]);
    }
  }
  
  
  
  
  /******************/
  /* Backward phase */
  /******************/

  printf("## Backward phase ##\n");  
  
  /* forget old evidence */
  reset_model(model);
  
  
  for(t = timeseries->datarows; t >= 0; t--){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t);
    
    
    if(t > 0){
      for(i = 0; i < timeseries->num_of_nodes; i++){
	temp = get_variable((timeseries->node_symbols)[i]);
	assert(temp);
	if(data[t - 1][i] >= 0)
	  enter_i_observation(model->first_var, model->cliques, 
			      model->num_of_cliques, 
			      temp, 
			      data[t - 1][i]);
      }
      
      for(i = 0; i < num_of_hidden; i++){
	temp = hidden[i];
	if(temp->next != NULL)
	  enter_evidence(model->first_var, model->cliques, 
			 model->num_of_cliques, temp->next, result[t-1][i]);
      }
    }      
    
    if(t < timeseries->datarows){
      
      for(i = 0; i < num_of_hidden; i++){
	temp = hidden[i];
	if(temp->previous != NULL){
	  /* search for the other index */
	  for(k = 0; k < num_of_hidden; k++)
	    if(equal_variables(temp->previous, hidden[k]))
	      break;
	  
	  /* FIXME: Get rid of the quotient array */
	  
	  for(j = 0; j < number_of_values(temp); j++)
	    quotient[i][j] = result[t + 1][i][j] / result[t][k][j]; 
	  
	  enter_evidence(model->first_var, model->cliques, 
			 model->num_of_cliques, 
			 temp->previous, quotient[i]);
	}
      }
    }
    
    /********************/
    /* Do the inference */
    /********************/
    
    make_consistent(model);
    
    
    
    /*********************************/
    /* Check the result of inference */
    /*********************************/
    for(i = 0; i < num_of_hidden; i++){
      
      /* 1. Decide which Variable you are interested in */
      interesting = hidden[i];
      
      /* 2. Find the Clique that contains the family of 
       *    the interesting Variable */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       &interesting, 1);
      if(!clique_of_interest){
	printf("In hmmtest.c : No clique found! Sorry.\n");
	return 1;
      }  
      
      /* 3. Marginalisation (the memory must have been allocated) */
      marginalise(clique_of_interest, interesting, result[t][i]);
      
      /* 4. Normalisation */
      normalise(result[t][i], number_of_values(interesting));
      
      /* 5. Print the result */
      for(j = 0; j < number_of_values(interesting); j++)
	printf("P(%s=%s) = %f\n", get_symbol(interesting),
	       (interesting->statenames)[j], result[t][i][j]);
      printf("\n");
    }
    
    /* forget old evidence */
    reset_model(model);
    
  }
  
  return 0;
}
