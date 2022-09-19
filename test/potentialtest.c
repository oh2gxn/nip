/*  NIP - Dynamic Bayesian Network library
    Copyright (C) 2012  Janne Toivola

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/* This is a small test program for potentials...
 *
 * Author: Janne Toivola, Mikko Korpela
 * Version: $Id: potentialtest.c,v 1.8 2010-12-09 16:52:50 jatoivol Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include "nippotential.h"

/* Main function for testing */
int main(){

  /* Use big numbers here and see that 640K isn't enough for everyone... */
  int cardinality[] = {2, 3, 4};
  int card2[] = {4, 2};
  int num_of_vars = 3;
  int margin_mapping[] = {2, 0}; /* maps variables p -> q */
  int indices[3], i, j, k, x = 0;
  double value;
  double sum[4], update[4];
  nip_potential p, q, r;
  p = nip_new_potential(cardinality, num_of_vars, NULL);
  q = nip_new_potential(card2, num_of_vars - 1, NULL);

  /* Set values of p */
  for(i = 0; i < cardinality[0]; i++){
    indices[0] = i;
    for(j = 0; j < cardinality[1]; j++){
      indices[1] = j;
      for(k = 0; k < cardinality[2]; k++){
	indices[2] = k;
	nip_set_potential_value(p, indices, x);
	x++;
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
	value = nip_get_potential_value(p, indices);
	printf("(%d, %d, %d) --> %g\n",
	       indices[0], indices[1], indices[2], value);
      }
    }
  }

  /* testing inverse mapping */
  printf("Inverse mapping:\n");
  for(i = 0; i < p->size_of_data; i++){
    nip_inverse_mapping(p, i, indices);
    printf("%d --> (%d, %d, %d)\n",
	   i, indices[0], indices[1], indices[2]);
  }

  /* marginalise over variable 4 (second variable of p) */
  printf("Marginalized:\n");
  nip_general_marginalise(p, q, margin_mapping);

  /* Print values of q */
  for(i = 0; i < card2[0]; i++){
    indices[0] = i;
    for(k = 0; k < card2[1]; k++){
      indices[1] = k;
      value = nip_get_potential_value(q, indices);
      printf("(%d, %d) --> %g\n",
	     indices[0], indices[1], value);
    }
  }

  // copy
  printf("Copy:\n");
  r = nip_copy_potential(p);
  for(i = 0; i < cardinality[0]; i++){
    indices[0] = i;
    for(j = 0; j < cardinality[1]; j++){
      indices[1] = j;
      for(k = 0; k < cardinality[2]; k++){
	indices[2] = k;
	value = nip_get_potential_value(r, indices);
	printf("(%d, %d, %d) --> %g\n",
	       indices[0], indices[1], indices[2], value);
      }
    }
  }

  // sum p = p+r
  printf("Sum:\n");
  nip_sum_potential(p, r);
  for(i = 0; i < cardinality[0]; i++){
    indices[0] = i;
    for(j = 0; j < cardinality[1]; j++){
      indices[1] = j;
      for(k = 0; k < cardinality[2]; k++){
	indices[2] = k;
	value = nip_get_potential_value(p, indices);
	printf("(%d, %d, %d) --> %g\n",
	       indices[0], indices[1], indices[2], value);
      }
    }
  }

  // sum across all but one dimension
  printf("Margin:\n");
  nip_total_marginalise(p, sum, 1);
  for(j = 0; j < cardinality[1]; j++)
    printf("(+, %d, +) -> %g\n", j, sum[j]);

  // normalization across the first dimension
  printf("CPD:\n");
  nip_normalise_cpd(p);
  nip_fprintf_potential(stdout, p);

  // updating q = p/r
  printf("Belief update from another potential:\n");
  nip_update_potential(p, r, q, margin_mapping); // FIXME
  nip_fprintf_potential(stdout, q);

  // updating evidence r = update*r/sum
  printf("Belief update from single margin:\n");
  for(j = 0; j < cardinality[1]; j++)
    update[cardinality[1] - j - 1] = sum[j]; // reverse
  nip_update_evidence(update, sum, r, 1);
  nip_fprintf_potential(stdout, r);

  // TODO: normalize across some dimension
  printf("Normalize:\n");
  nip_normalise_dimension(r, 1);
  nip_total_marginalise(r, sum, 1);
  for(j = 0; j < cardinality[1]; j++)
    printf("(+, %d, +) -> %g\n", j, sum[j]);

  nip_free_potential(p);
  nip_free_potential(q);
  nip_free_potential(r);

  return 0;
}
