#include <stdlib.h>
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "potential.h"

/*
#define PRINT_POTENTIALS
*/

/*
#define DEBUG_BISONTEST
*/

/*
#define EVIDENCE
*/

/*#define EVIDENCE1*/


int yyparse();

int main(int argc, char *argv[]){

  int i, retval;
  int nip_num_of_cliques;
  Clique *nip_cliques;

#ifdef PRINT_POTENTIALS
  int j;
#endif

#ifdef DEBUG_BISONTEST
  int temp;
  int evidence_retval;
#endif

  double* result;

#ifdef EVIDENCE

#ifdef EVIDENCE1
  double probB[] = {0.25, 0.25, 0.40, 0.10};
  double probD[] = {0.2, 0.3, 0.5};
#endif /* EVIDENCE1 */

#ifndef EVIDENCE1
  double probB[] = {0.75, 0.21, 0.03, 0.01};
  double probD[] = {0.6, 0.1, 0.3};
#endif /* !EVIDENCE1 */

  Variable observed[2];
#endif /* EVIDENCE */

  Variable interesting;
  Clique clique_of_interest;

  /* -- Start parsing the network definition file */
  if(argc < 2){
    if(open_infile("infile") != 0)
      return -1;
  }
  else if(open_infile(argv[1]) != 0)
    return -1;

  retval = yyparse();

  close_infile();

  if(retval != 0)
    return retval;
  /* The input file has been parsed. -- */

  nip_cliques = get_nip_cliques();
  nip_num_of_cliques = get_nip_num_of_cliques();

  /* propagation: some action */
  /*
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);

  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  distribute_evidence(nip_cliques[0]);
  */



  /* *********************************************************** */
#ifdef PRINT_POTENTIALS
  for(i = 0; i < nip_num_of_cliques; i++){
    printf("In bisontest.c : Clique of ");
    for(j = 0; j < clique_num_of_vars(nip_cliques[i]) - 1; j++)
      printf("%s ", get_symbol(clique_get_Variable(nip_cliques[i], j)));
    printf("and %s.\n", get_symbol(clique_get_Variable(nip_cliques[i], j)));
    
    print_potential(nip_cliques[i]->p);
  }
#endif /* PRINT_POTENTIALS */
  /* *********************************************************** */




  /* *********************************************************** */
#ifdef EVIDENCE
  /* add some evidence */
  observed[0] = get_variable("B");
  observed[1] = get_variable("D");

  clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
				   observed, 1);

#ifndef DEBUG_BISONTEST
  enter_evidence(clique_of_interest, observed[0], probB);
#endif

#ifdef DEBUG_BISONTEST
  evidence_retval = enter_evidence(clique_of_interest, observed[0], probB);
  printf("\n\nEntered evidence into the clique of ");
  for(i = 0; i < clique_num_of_vars(clique_of_interest) - 1; i++)
    printf("%s ", get_symbol(clique_get_Variable(clique_of_interest, i)));
  printf("and %s.\n", get_symbol(clique_get_Variable(clique_of_interest, i)));

  printf("enter_evidence returned %d.\n", evidence_retval);
#endif /* DEBUG_BISONTEST */

  /* propagation: some action */

  /* Procedural guide: UNMARK all clusters. */
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[1]);

  /* Procedural guide: UNMARK all clusters. */
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  distribute_evidence(nip_cliques[1]);

  clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
				   observed, 2);

#ifndef DEBUG_BISONTEST
  enter_evidence(clique_of_interest, observed[1], probD);
#endif

#ifdef DEBUG_BISONTEST
  evidence_retval = enter_evidence(clique_of_interest, observed[1], probD);
  printf("Entered evidence into the clique of ");
  for(i = 0; i < clique_num_of_vars(clique_of_interest) - 1; i++)
    printf("%s ", get_symbol(clique_get_Variable(clique_of_interest, i)));
  printf("and %s.\n", get_symbol(clique_get_Variable(clique_of_interest, i)));

  printf("enter_evidence returned %d.\n", evidence_retval);
#endif /* DEBUG_BISONTEST*/
#endif /* EVIDENCE */
  /* *********************************************************** */




  printf("\n\n");

  /* another propagation */
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);

  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  distribute_evidence(nip_cliques[0]);


  /* marginalisation */
  if(argc > 2)
    interesting = get_variable(argv[2]); /* THE variable */
  else
    interesting = get_variable("B"); /* THE variable */

  if(!interesting){
    printf("In bisontest.c : Variable of interest not found.\n");
    return 1;
  }

  clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
				   &interesting, 1);
  if(!clique_of_interest){
    printf("In bisontest.c : No clique found! Sorry.\n");
    return 1;
  }  

  result = (double *) calloc(number_of_values(interesting),
			     sizeof(double));
  if(!result){
    printf("In bisontest.c : Calloc failed.\n");
    return 1;
  }

#ifdef DEBUG_BISONTEST
  printf("\n\nMarginalising in a Clique with ");
  for(i = 0; i < clique_num_of_vars(clique_of_interest); i++)
    printf("%s", get_symbol(clique_get_Variable(clique_of_interest, i)));
  printf("\n");
#endif /* DEBUG_BISONTEST */
    
#ifndef DEBUG_BISONTEST
  marginalise(clique_of_interest, interesting, result);
#endif

#ifdef DEBUG_BISONTEST
  temp = marginalise(clique_of_interest, interesting, result);
  printf("Marginalisation returned %d. (0 is OK)\n", temp);

  printf("After marginalisation, probability of %s:\n", 
	 get_symbol(interesting));
  for(i = 0; i < number_of_values(interesting); i++)
    printf("result[%d] = %f\n", i, result[i]);
#endif /* DEBUG_BISONTEST */

  /* normalization */
  normalise(result, number_of_values(interesting));

  printf("Normalised probability of %s:\n", get_symbol(interesting));
  for(i = 0; i < number_of_values(interesting); i++)
    printf("result[%d] = %f\n", i, result[i]);
  /* To be continued... */

  free(result);
  return 0;
}
