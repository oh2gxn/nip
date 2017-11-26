/**
 * @file
 * @brief Functions for handling potentials: 
 * probability tables for distributions of categorical or discrete 
 * random variables
 *
 * @author Janne Toivola
 * @author Mikko Korpela
 * @copyright &copy; 2007,2012 Janne Toivola <br>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. <br>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. <br>
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NIPPOTENTIAL_H__
#define __NIPPOTENTIAL_H__

#ifndef __STDC_VERSION__
#define __STDC_VERSION__ 199901L
#endif
#ifndef _POSIX_VERSION
#define _POSIX_VERSION 200112L
#endif

#include <math.h> // HUGE_VAL
#include <stdio.h> // FILE
#include "niplists.h" // nip_string_pair_list

#ifndef HUGE_DOUBLE
#ifndef HUGE_VAL
#error "HUGE_VAL not defined!"
#endif
#define HUGE_DOUBLE HUGE_VAL ///< (1.0/0.0) Mac OS X had invalid HUGE_VAL?
#endif /* HUGE_DOUBLE */

#define NIP_DIMENSIONALITY(p) ((p)->dimensionality) ///< get number of dims

/**
 * Structure for storing multidimensional tables of probabilities
 */
typedef struct nip_pot_array {
  int dimensionality; ///< number of dimensions
  int* cardinality; ///< dimensions of the data
  int* temp_index; ///< space for index calculations
  int size_of_data; ///< total number of data elements, prod(cardinality)
  double* data; ///< data array: the probability of each combination
  nip_string_pair_list application_specific_properties; ///< external data
} nip_potential_struct;

typedef nip_potential_struct* nip_potential; ///< potential reference

/**
 * Make a potential array of certain dimensionality. 
 * The potential array \p data can be null, if it is not known, and then 
 * uniform array of ones is used. Otherwise, the data array is assumed to have 
 * prod(cardinality) values. Special case of dimensionality=0 leads to a
 * potential with just one data element ("scalar weight").
 * @param cardinality Size of each dimension (how many states a var has)
 * @param dimensionality Number of dimensions, length of cardinality array
 * @param NULL for "ones", or initial data to be copied
 * @return a reference to a new potential
 * @see nip_free_potential() */
nip_potential nip_new_potential(int cardinality[], int dimensionality, 
				double data[]);

/**
 * Saves a pair of strings as additional data to a potential.
 * @param p The potential to modify
 * @param key A key string
 * @param value A value string corresponding to the key
 * @return An error code if something failed, or 0 */
int nip_set_potential_property(nip_potential p, char* key, char* value);

/**
 * Retrieves a piece of additional data from a potential
 * @param p The potential to read
 * @param key The key string
 * @return Reference to corresponding value string, or NULL
 * @see nip_string_pair_list_search() */
char* nip_get_potential_property(nip_potential p, char* key);

/**
 * Creates a new potential as an exact copy of \p p.
 * @param p A potential to copy
 * @return A new potential with a copy of the same data
 * @see nip_free_potential() */
nip_potential nip_copy_potential(nip_potential p);

/**
 * Copies the content of \p ref potential to \p p, assuming they 
 * have equal dimensionality. Used for reverting back to original model 
 * parameters (\p ref) after messing p with old evidence. 
 * @param p The potential to modify
 * @param ref Potential from which the content is copied
 * @return An error code if something failed, or 0 */
int nip_retract_potential(nip_potential p, nip_potential ref);

/**
 * Free the memory used by potential \p p. */
void nip_free_potential(nip_potential p);

/**
 * Sets all the elements to the specified value (usually 0 or 1)
 * @param p The potential to modify
 * @param value The value to copy */
void nip_uniform_potential(nip_potential p, double value);

/**
 * Randomize potential elements (values between 0 and 1 inclusive) 
 * Remember to set the seed value first (with srand(X))
 * @param p The potential to modify */
void nip_random_potential(nip_potential p);

/**
 * Gets a value from the potential \p p. 
 * Nicer access than fooling with addresses yourself.
 * Does not check if indices are too big, though.
 * @param p The potential to read
 * @param indices Array of index values, one for each dimension 
 * @return The value at indices */
double nip_get_potential_value(nip_potential p, int indices[]);

/**
 * Sets a value in the given potential \p p at given index.
 * Does not check if indices are too big, though.
 * @param p The potential to modify
 * @param indices Array of index values, one for each dimension
 * @param value The value to set
 */
void nip_set_potential_value(nip_potential p, int indices[], double value);

/**
 * Mapping from 1-dimensional / flat index to n-dimensional index, 
 * where n is the number of variables in potential \p p.
 * USUALLY NOT NEEDED outside of potential.c 
 * @param p The potential
 * @param flat_index One dimensional 0-based index in the data
 * @param indices Array where indices are written */
void nip_inverse_mapping(nip_potential p, int flat_index, int indices[]);

/**
 * Method for marginalising over certain variables. 
 * Useful in message passing from clique to sepset.
 * NOTE: Take care of the ORDER of variables! 
 * EXAMPLE: If sepset variables are the second (i.e. 1) and third (i.e. 2) 
 *          variable in a five variable clique, the call is 
 *          marginalise(cliquePotential, newSepsetPotential, {1, 2}) 
 * @param source The potential to be marginalised
 * @param destination The potential to put the answer into
 * @param mapping Placement of the destination variables in the source 
 * potential in the same order they appear in the destination potential 
 * (0-based indices)
 * @return an error code, or 0 on success
 */
int nip_general_marginalise(nip_potential source, nip_potential destination, 
			    int mapping[]);

/**
 * Method for finding out the probability distribution of a single variable 
 * according to a clique potential. This one is a marginalisation too, but 
 * this is not generic. The outcome is not normalized.
 * @param source The potential to be marginalised
 * @param destination The double array for the answer
 *   NOTE: Size of the array MUST be correct (check it from the variable)
 * @param variable The 0-based index of the variable of interest 
 * @return an error code, or 0 on success
 */
int nip_total_marginalise(nip_potential source, double destination[], 
			  int variable);

/**
 * Normalises an array. Divides every member by their sum.
 * The function modifies the given array.
 * NOTE: zero array is left intact
 * @param result The array to modify
 * @param array_size Length of the array
 */
void nip_normalise_array(double result[], int array_size);

/**
 * Normalises a potential so that the sum of all elements is one. 
 * (except when all of the elements are zero)
 * @param p The potential to modify
 * @return an error code, or 0 on success
 */
int nip_normalise_potential(nip_potential p);

/**
 * Normalises a potential so that it is a valid conditional probability 
 * distribution (CPD). This assumes that the first variable (dimension) 
 * is the child. Potentially faster than normalise_dimension(p, 0).
 * @param p The potential to modify
 * @return an error code, or 0 on success
 */
int nip_normalise_cpd(nip_potential p);


/**
 * A more general way of normalising a potential along any single dimension.
 * NOTE: normalise_cpd(p) <==> normalise_dimension(p, 0)
 * @return an error code, or 0 on success 
 * @see nip_normalise_cpd() */
int nip_normalise_dimension(nip_potential p, int dimension);


/**
 * Method for accumulating a sum of potentials: 
 * "sum += increment" for potential tables.
 * @param sum The potential to add into
 * @param increment The potential to add
 * @return an error code, or 0 on success */
int nip_sum_potential(nip_potential sum, nip_potential increment);


/**
 * Method for updating target potential by multiplying with numerator 
 * potential and dividing with denominator potential. Useful in message 
 * passing from sepset to clique.
 * - If denominator is NULL, only the multiplication is done.
 * - If numerator is NULL, only the division is done.
 * - If both are NULL or have different geometry, an error is reported.
 *
 * EXAMPLE: If two sepset variables are the third (indexed by 2) and 
 *          fifth (4) variables in a five variable clique, the call is 
 * update(newSepsetPotential, oldSepsetPotential, cliquePotential, {2, 4})
 *
 * @param target The potential whose values are updated
 * @param numerator Multiplier, usually the newer sepset potential (source)
 * @param denominator Divider, usually the older sepset potential. 
 *   This MUST have similar dimensions to numerator.
 * @param mapping An index array which holds the placement of the variables 
 *   of \p numerator & \p denominator potential in the \p target potential
 * @return an error code, or 0 on success */
int nip_update_potential(nip_potential numerator, nip_potential denominator, 
			 nip_potential target, int mapping[]);

/**
 * Method for updating potential according to new evidence.
 * Precondition: numerator[i] > 0 => denominator[i] > 0, for all i
 * In plain English, event that was previously impossible cannot be switched 
 * to possible. Otherwise a global retraction must be done before calling this!
 * @param numerator New evidence about the variable, length matching \p var
 * @param denominator Old likelihood of the variable
 * @param target The potential to be updated
 * @param var The 0-based index of the dimension that gets new evidence
 * @return an error code, or 0 on success
 */
int nip_update_evidence(double numerator[], double denominator[], 
			nip_potential target, int var);

/**
 * This one implements the initialisation with observations. 
 * See [Huang & Darwiche 1994] the Procedural Guide page 25, step 2.
 * @param probs A probability distribution as a potential
 * @param target The potential to update
 * @param mapping Indices of each \p probs dimension in \p target
 * @return an error code, or 0 on success
 * @see update_potential()
 */
int nip_init_potential(nip_potential probs, nip_potential target, 
		       int mapping[]);

/**
 * Prints a textual representation of the potential \p p to stream.
 * Mostly for debugging.
 * @param stream An open stream to print to
 * @param p The potential values to print */
void nip_fprintf_potential(FILE* stream, nip_potential p);

#endif
