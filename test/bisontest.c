#include <stdlib.h>
#include "parser.h"
#include "Clique.h"
#include "potential.h"

/*#define PRINT_POTENTIALS*/

#define DEBUG_BISONTEST

#define EVIDENCE

/*
#define EVIDENCE1
*/

int yyparse();

int main(int argc, char *argv[]){

  int i, retval, temp;
  int nip_num_of_cliques;
  Clique *nip_cliques;

#ifdef PRINT_POTENTIALS
  int j;
#endif

  double* result;

#ifdef EVIDENCE
  int evidence_retval;

#ifdef EVIDENCE1
  double probB[] = {0.25, 0.25, 0.40, 0.10};
  double probD[] = {0.2, 0.3, 0.5};
#endif

#ifndef EVIDENCE1
  double probB[] = {0.75, 0.21, 0.03, 0.01};
  double probD[] = {0.6, 0.1, 0.3};
#endif

  Variable observed[2];
#endif

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
  distribute_evidence(nip_cliques[0]);
  */

#ifdef PRINT_POTENTIALS
  for(i = 0; i < nip_num_of_cliques; i++){
    printf("In bisontest.c : Clique of ");
    for(j = 0; j < nip_cliques[i]->p->num_of_vars - 1; j++)
      printf("%s ", nip_cliques[i]->variables[j]->symbol);
    printf("and %s.\n", nip_cliques[i]->variables[j]->symbol);
    
    print_potential(nip_cliques[i]->p);
  }
#endif

#ifdef EVIDENCE
  /* add some evidence */
  observed[0] = get_variable("B");
  observed[1] = get_variable("D");

  clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
				   observed, 2);
  evidence_retval = enter_evidence(clique_of_interest, observed[0], probB);

#ifdef DEBUG_BISONTEST
  printf("Entered evidence into the clique of ");
  for(i = 0; i < clique_of_interest->p->num_of_vars - 1; i++)
    printf("%s ", clique_of_interest->variables[i]->symbol);
  printf("and %s.\n", clique_of_interest->variables[i]->symbol);

  printf("enter_evidence returned %d.\n", evidence_retval);
#endif
#endif

  /* propagation: some action */
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);
  distribute_evidence(nip_cliques[0]);

  clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
				   observed+1, 1);

#ifdef EVIDENCE
  evidence_retval = enter_evidence(clique_of_interest, observed[1], probD);

#ifdef DEBUG_BISONTEST
  printf("Entered evidence into the clique of ");
  for(i = 0; i < clique_of_interest->p->num_of_vars - 1; i++)
    printf("%s ", clique_of_interest->variables[i]->symbol);
  printf("and %s.\n", clique_of_interest->variables[i]->symbol);

  printf("enter_evidence returned %d.\n", evidence_retval);
#endif
#endif

  /* another propagation */
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);
  distribute_evidence(nip_cliques[0]);


  /* marginalisation */
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

  result = (double *) calloc(interesting->cardinality,
			     sizeof(double));
  if(!result){
    printf("In bisontest.c : Calloc failed.\n");
    return 1;
  }

#ifdef DEBUG_BISONTEST
  printf("Marginalising in a Clique with ");
  for(i = 0; i < clique_of_interest->p->num_of_vars; i++)
    printf("%s", clique_of_interest->variables[i]->symbol);
  printf("\n");
#endif
    
  temp = marginalise(clique_of_interest, interesting, result);

#ifdef DEBUG_BISONTEST
  printf("Marginalisation returned %d. (0 is OK)\n", temp);

  printf("After marginalisation, probability of %s:\n", interesting->symbol);
  for(i = 0; i < interesting->cardinality; i++)
    printf("result[%d] = %f\n", i, result[i]);
#endif

  /* normalization */
  normalise(result, interesting->cardinality);

  printf("Normalised probability of %s:\n", interesting->symbol);
  for(i = 0; i < interesting->cardinality; i++)
    printf("result[%d] = %f\n", i, result[i]);
  /* To be continued... */

  free(result);
  return 0;
}
