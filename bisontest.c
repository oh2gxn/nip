#include <stdlib.h>
#include "parser.h"
#include "Clique.h"

int yyparse();

int main(int argc, char *argv[]){

  int i, retval;
  double* result;
  double probB[] = {0.25, 0.25, 0.40, 0.10};
  double probD[] = {0.2, 0.3, 0.5};
  Variable observed[2];
  Variable interesting;
  Clique clique_of_interest;

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

  /* propagation: some action */
  /*
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);
  distribute_evidence(nip_cliques[0]);
  */

  /* add some evidence */
  observed[0] = get_variable("B");
  observed[1] = get_variable("D");

  clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
				   observed, 2);
  enter_evidence(clique_of_interest, observed[0], probB);
  printf("Entered evidence into the clique of ");
  for(i = 0; i < clique_of_interest->p->num_of_vars - 1; i++)
    printf("%s ", clique_of_interest->variables[i]->symbol);
  printf("and %s.\n", clique_of_interest->variables[i]->symbol);


  /*  clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
      observed+1, 1);*/
  enter_evidence(clique_of_interest, observed[1], probD);
  printf("Entered evidence into the clique of ");
  for(i = 0; i < clique_of_interest->p->num_of_vars - 1; i++)
    printf("%s ", clique_of_interest->variables[i]->symbol);
  printf("and %s.\n", clique_of_interest->variables[i]->symbol);


  /* another propagation */
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);
  distribute_evidence(nip_cliques[0]);


  /* marginalisation */
  interesting = get_variable("A"); /* THE variable */

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

  printf("Marginalisation returned %d. (0 is OK)\n", 
	 marginalise(clique_of_interest, interesting, result));


  /* normalization */
  normalise(result, interesting->cardinality);

  printf("Normalised probability of %s:\n", interesting->symbol);
  for(i = 0; i < interesting->cardinality; i++)
    printf("result[%d] = %f\n", i, result[i]);
  /* To be continued... */

  free(result);
  return 0;
}
