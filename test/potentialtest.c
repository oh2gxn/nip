/* Tämä on pieni testi potentiaalille. */

#include <stdio.h>
#include <stdlib.h>
#include "potential.h" 

/* Main function for testing */
int main(){

  /* Use big numbers here and see that 640K isn't enough for everyone... */
  int cardinality[] = { 2, 3, 4, 5, 6};
  int card2[] = { 2, 3, 4, 5};
  int num_of_vars = 5;
  int margin_var = 4; /* usually an array */
  int indices[5], i, j, k, l, m, x = 0;
  double value;
  potential p, q;
  p = make_potential(cardinality, num_of_vars);
  q = make_potential(card2, num_of_vars - 1);

  /* Set values of p */
  for(i = 0; i < cardinality[0]; i++){
    indices[0] = i;
    for(j = 0; j < cardinality[1]; j++){
      indices[1] = j;
      for(k = 0; k < cardinality[2]; k++){
	indices[2] = k;
	for(l = 0; l < cardinality[3]; l++){
	  indices[3] = l;
	  for(m = 0; m < cardinality[4]; m++){
	    indices[4] = m;
	    x++;
	    set_pvalue(p, indices, x);
	  }
	}
      }
    }
  }

  /* Print values of p */
  for(i = 0; i < cardinality[0]; i++){
    indices[0] = i;
    for(j = 0; j < cardinality[1]; j++){
      indices[1] = j;
      for(k = 0; k < cardinality[2]; k++){
	indices[2] = k;
	for(l = 0; l < cardinality[3]; l++){
	  indices[3] = l;
	  for(m = 0; m < cardinality[4]; m++){
	    indices[4] = m;
	    value = get_pvalue(p, indices);
	    printf("Potential (%d, %d, %d, %d, %d) is: %f\n", indices[0],
		   indices[1], indices[2], indices[3], indices[4], value);
	  }
	}
      }
    }
  }

  /* testing inverse mapping */
  for(i = 0; i < p->size_of_data; i++){
    inverse_mapping(p, i, indices);
    printf("Inverse mapping: %d --> (%d, %d, %d, %d, %d)\n", i, indices[0],
	   indices[1], indices[2], indices[3], indices[4]);
  }

  /* marginalise over variable 4 (fifth variable of p) */
  general_marginalise(p, q, &margin_var);

  /* Print values of q */
  for(i = 0; i < card2[0]; i++){
    indices[0] = i;
    for(j = 0; j < card2[1]; j++){
      indices[1] = j;
      for(k = 0; k < card2[2]; k++){
	indices[2] = k;
	for(l = 0; l < card2[3]; l++){
	  indices[3] = l;
	  value = get_pvalue(q, indices);
	  printf("Potential (%d, %d, %d, %d) is: %f\n", indices[0],
		 indices[1], indices[2], indices[3], value);
	}
      }
    }
  }

  return 0;
}
