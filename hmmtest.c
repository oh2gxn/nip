#include <stdlib.h>
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

extern int yyparse();

int main(int argc, char *argv[]){

  char **data;
  int i, retval, t = 0;
  int nip_num_of_cliques;
  double* result = NULL;

  Clique *nip_cliques;
  Clique clique_of_interest;

  Variable interesting;

  datafile* timeseries;


  /*************************************/
  /* Some experimental timeslice stuff */
  /*************************************/
  
  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/
  /* -- Start parsing the network definition file */
  if(argc < 4){
    printf("Give the names of the net-file, ");
    printf("data file and variable, please!\n");
    return 0;
  }
  else if(open_yyparse_infile(argv[1]) != NO_ERROR)
    return -1;

  retval = yyparse();

  close_yyparse_infile();

  if(retval != 0)
    return retval;
  /* The input file has been parsed. -- */


  /* Get references to the results of parsing */
  nip_cliques = *get_cliques_pointer();
  nip_num_of_cliques = get_num_of_cliques();



  /*****************************/
  /* read the data from a file */
  /*****************************/
  timeseries = open_datafile(argv[2], ',', 0, 1); /* 1. Open */
  if(timeseries == NULL){
    report_error(__FILE__, __LINE__, ERROR_FILENOTFOUND, 1);
    fprintf(stderr, "%s\n", argv[2]);
    return -1;
  }



  /* FIXME: Allocate some space for filtering and smoothing */


  retval = nextline_tokens(timeseries, ',', &data); /* 2. Read */


  for(t = 0; t < timeseries->datarows; t++){ /* FOR EVERY TIMESLICE */
    
    for(i = 0; i < retval; i++) /* 3. Insert into model */
      enter_observation(get_variable((timeseries->node_symbols)[i]), data[i]);
    
    for(i = 0; i < retval; i++) /* 4. Dump away */
      free(data[i]);
    
    
    
    /********************/
    /* Do the inference */
    /********************/

    /* 1. Unmark all Cliques */
    for(i = 0; i < nip_num_of_cliques; i++)
      unmark_Clique(nip_cliques[i]);
    
    /* 2. Collect evidence */
    collect_evidence(NULL, NULL, nip_cliques[0]);
    
    /* 3. Unmark all Cliques */
    for(i = 0; i < nip_num_of_cliques; i++)
      unmark_Clique(nip_cliques[i]);

    /* 4. Distribute evidence */
    distribute_evidence(nip_cliques[0]);
    


    /*********************************/
    /* Check the result of inference */
    /*********************************/

    /* 1. Decide which Variable you are interested in */
    interesting = get_variable(argv[3]); /* THE variable */
    
    if(!interesting){
      printf("In hmmtest.c : Variable of interest not found.\n");
      return 1;
    }
    
    /* 2. Find the Clique that contains the family of 
     *    the interesting Variable */
    clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
				     &interesting, 1);
    if(!clique_of_interest){
      printf("In hmmtest.c : No clique found! Sorry.\n");
      return 1;
    }  
    
    /* 3. Allocate memory for the result */
    result = (double *) calloc(number_of_values(interesting), sizeof(double));
    if(!result){
      printf("In hmmtest.c : Calloc failed.\n");
      return 1;
    }
    
    /* 4. Marginalisation */
    marginalise(clique_of_interest, interesting, result);
    
    /* 5. Normalisation */
    normalise(result, number_of_values(interesting));
    
    
    /* 6. Print the result */
    printf("Normalised probability of %s:\n", get_symbol(interesting));
    for(i = 0; i < number_of_values(interesting); i++)
      printf("P(%s=%s) = %f\n", get_symbol(interesting), 
	     (interesting->statenames)[i], result[i]);
    
    printf("\n");
    
    retval = nextline_tokens(timeseries, ',', &data); /* 2. Read */


    /* an experimental forward phase (a.k.a. filtering) */
    if(retval > 0){
      /* Reset the join tree and new priors from the posteriors by entering
       * evidence. */
      ;
    }
  }
  
  
  free(result);
  return 0;
}

