/* This is a small test program for potentials... 
 * 
 * Author: Janne Toivola, Mikko Korpela
 * Version: $Id: potentialtest.c,v 1.7 2010-12-03 17:21:29 jatoivol Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include "nippotential.h" 

/* Main function for testing */
int main(){

  /* Use big numbers here and see that 640K isn't enough for everyone... */
  int cardinality[] = {2, 3, 4, 5, 6};
  int card2[] = {3, 5, 4, 2};
  int num_of_vars = 5;
  int margin_mapping[] = {1, 3, 2, 0}; /* maps variables p -> q */
  int indices[5], i, j, k, l, m, x = 0;
  double value;
  nip_potential p, q;
  p = nip_new_potential(cardinality, num_of_vars, NULL);
  q = nip_new_potential(card2, num_of_vars - 1, NULL);

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
	    nip_set_potential_value(p, indices, x);
	    x++;
	  }
	}
      }
    }
  }

  /* Print values of p */
  printf("Mapping:\n");
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
	    value = nip_get_potential_value(p, indices);
	    printf("(%d, %d, %d, %d, %d) --> %g\n", indices[0],
		   indices[1], indices[2], indices[3], indices[4], value);
	  }
	}
      }
    }
  }

  /* testing inverse mapping */
  printf("Inverse mapping:\n");
  for(i = 0; i < p->size_of_data; i++){
    nip_inverse_mapping(p, i, indices);
    printf("%d --> (%d, %d, %d, %d, %d)\n", i, indices[0],
	   indices[1], indices[2], indices[3], indices[4]);
  }

  /* marginalise over variable 4 (fifth variable of p) */
  printf("Marginalized:\n");
  nip_general_marginalise(p, q, margin_mapping);

  /* Print values of q */
  for(i = 0; i < card2[0]; i++){
    indices[0] = i;
    for(j = 0; j < card2[1]; j++){
      indices[1] = j;
      for(k = 0; k < card2[2]; k++){
	indices[2] = k;
	for(l = 0; l < card2[3]; l++){
	  indices[3] = l;
	  value = nip_get_potential_value(q, indices);
	  printf("(%d, %d, %d, %d) --> %g\n", indices[0],
		 indices[1], indices[2], indices[3], value);
	}
      }
    }
  }

  return 0;
}
