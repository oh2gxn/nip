/* nippotential.h
 * Functions for handling potentials: 
 * probability tables for distributions of categorical variables
 * Author: Janne Toivola
 * Version: $Id: nippotential.h,v 1.1 2010-11-23 17:25:52 jatoivol Exp $
 */

#ifndef __NIPPOTENTIAL_H__
#define __NIPPOTENTIAL_H__

#include <stdio.h>
#include <math.h>

/* Mac OS X had invalid HUGE_VAL ? */
#ifndef HUGE_DOUBLE
  /*#error "HUGE_VAL not defined!"*/
#define HUGE_DOUBLE (1.0/0.0)
#endif /* HUGE_DOUBLE */

typedef struct nip_pot_array_t {
  int size_of_data;
  int *cardinality;
  int num_of_vars;
  double *data;

  /* TODO: stringpairlist application_specific_properties; ? */
} nip_potential_struct;

/* typedef struct pot_array_t potential_struct; */
typedef nip_potential_struct* nip_potential;

/* Make a num_of_vars -dimension potential array. 
 * The potential array data[] can be null, if it is not known. */
nip_potential nip_new_potential(int cardinality[], int num_of_vars, 
				double data[]);

/* Free the memory used by potential p. */
void nip_free_potential(nip_potential p);

/* Sets all the elements to the specified value (usually 0 or 1) */
void nip_uniform_potential(nip_potential p, double value);

/* Randomize potential elements (values between 0 and 1 inclusive) 
 * Remember to set the seed value first (with srand(X)) */
void nip_random_potential(nip_potential p);

/* Gets a value from the potential p. Syntactic sugar. */
double nip_get_potential_value(nip_potential p, int indices[]);

/* Sets a value in the potential p and returns an error code.
 * Syntactic sugar.
 */
void nip_set_potential_value(nip_potential p, int indices[], double value);

/* Mapping from flat index to n-dimensional index, where n is the number of
 * variables in potential p.
 * USUALLY NOT NEEDED outside of potential.c */
void nip_inverse_mapping(nip_potential p, int flat_index, int indices[]);

/* Method for marginalising over certain variables. Useful in message passing
 * from clique to sepset.
 * TAKE CARE OF THE ORDER OF VARIABLES! 
 * -source: the potential to be marginalised
 * -destination: the potential to put the answer into
 * -mapping: placement of the destination variables in the source potential 
 *           in the same order they appear in the destination potential
 * EXAMPLE: If sepset variables are the second (i.e. 1) and third (i.e. 2) 
 *          variable in a five variable clique, the call is 
 *          marginalise(cliquePotential, newSepsetPotential, {1, 2}) 
 * -Returns an error code.
 */
int nip_general_marginalise(nip_potential source, nip_potential destination, 
			    int mapping[]);

/* Method for finding out the probability distribution of a single variable 
 * according to a clique potential. This one is a marginalisation too, but 
 * this one is not generic. The outcome is not normalized. 
 * -source: the potential to be marginalised
 * -destination: the double array for the answer
 *               SIZE OF THE ARRAY MUST BE CORRECT (check it from the variable)
 * -variable: the index of the variable of interest 
 */
int nip_total_marginalise(nip_potential source, double destination[], 
			  int variable);


/* Normalises an array. Divides every member by their sum.
 * The function modifies the given array.
 * NOTE: zero array is left intact
 */
void nip_normalise_array(double result[], int array_size);


/* Normalises a potential so that the sum of all elements is one. 
 * (except when all of the elements are zero)
 * Returns an error code.
 */
int nip_normalise_potential(nip_potential p);


/* Normalises a potential so that it is a valid conditional probability 
 * distribution (CPD). This assumes that the first variable (dimension) 
 * is the child. (faster than normalise_dimension(p, 0) ) 
 * Returns an error code. 
 */
int nip_normalise_cpd(nip_potential p);


/* A more general way of normalising a potential along any single dimension. 
 * ( normalise_cpd(p) <==> normalise_dimension(p, 0) )
 */
int nip_normalise_dimension(nip_potential p, int dimension);


/* Method for counting a sum of potentials: 
 * "sum += increment" for potential tables. */
int nip_sum_potential(nip_potential sum, nip_potential increment);


/* Method for updating target potential by multiplying with numerator 
 * potential and dividing with denominator potential. Useful in message 
 * passing from sepset to clique. 
 * -target: the potential whose values are updated
 * -numerator: multiplier, usually the newer sepset potential (source)
 * -denominator: divider, usually the older sepset potential. This MUST have 
 *  similar geometry to numerator.
 * -mapping: an integer array which holds the placement of the variables 
 *           of numerator/denominator potential in the target potential
 * EXAMPLE: If two sepset variables are the third (indexed by 2) and 
 *          fifth (4) variables in a five variable clique, the call is 
 *  update(newSepsetPotential, oldSepsetPotential, cliquePotential, {2, 4}) 
 * -Returns an error code.
 * 
 * JJT: If denominator is NULL, only the multiplication is done.
 *      If numerator is NULL, only the division is done.
 *      If both are NULL or have different geometry, an error is reported.
 */
int nip_update_potential(nip_potential numerator, nip_potential denominator, 
			 nip_potential target, int mapping[]);

/* Method for updating potential according to new evidence.
 * MUST BE: evidence[i] > 0 => v->likelihood[i] > 0
 * Otherwise a global retraction must be done before calling this !!!!!!
 * -numerator[]: new evidence about the variable
 * -denominator[]: old likelihood of the variable
 * -target: the potential to be updated
 * -var: the index (in the potential) of the variable that gets new evidence
 */
int nip_update_evidence(double numerator[], double denominator[], 
			nip_potential target, int var);

/* This one implements the initialisation with observations. 
 * See the Procedural Guide page 25, step 2. 
 * Similar to update_potential() so look at the info about 
 * arguments there.
 */
int nip_init_potential(nip_potential probs, nip_potential target, 
		       int mapping[]);

/* TODO: make "fprintf_potential" ! */
void nip_fprintf_potential(FILE* stream, nip_potential p);

#endif
