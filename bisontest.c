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
  Clique intressant;

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
  /*  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);
  distribute_evidence(nip_cliques[0]);*/

  /* add some evidence */
  observed[0] = get_variable("B");
  observed[1] = get_variable("D");
  enter_evidence(find_family(nip_cliques, nip_num_of_cliques, observed, 1), 
		 observed[0], probB);
  enter_evidence(find_family(nip_cliques, nip_num_of_cliques, observed+1, 1),
		 observed[1], probD);

  /* another propagation */
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);
  distribute_evidence(nip_cliques[0]);

  /* marginalisation */
  interesting = get_variable("A"); /* THE variable */

  if(!interesting){
    printf("This is not interesting!\n");
    return 1;
  }

  intressant = find_family(nip_cliques, nip_num_of_cliques, &interesting, 1);
  if(!intressant){
    printf("No clique found! Sorry.\n");
    return 1;
  }  
    
  result = (double *) calloc(interesting->cardinality,
			     sizeof(double));
  if(!result){
    printf("No room for results! Sorry.\n");
    return 1;
  }

  printf("Marginalisation returned %d. (0 is OK)\n", 
	 marginalise(intressant, interesting, result));

  /* normalization */
  normalise(result, interesting->cardinality);

  printf("Normalised probability of %s:\n", interesting->symbol);
  for(i = 0; i < interesting->cardinality; i++)
    printf("result[%d] = %f\n", i, result[i]);
  /* To be continued... */

  return 0;
}
