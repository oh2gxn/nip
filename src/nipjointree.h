/**
 * @file
 * @brief Functions for handling cliques and sepsets.
 *
 * Includes evidence handling and propagation of information
 * in the join tree.
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

#ifndef __NIPJOINTREE_H__
#define __NIPJOINTREE_H__

#include <stdio.h> // FILE
#include "nipvariable.h"
#include "nippotential.h"

/**
 * References to sepsets neighbouring a clique, implemented as a linked list
 */
typedef struct nip_sepsetlink {
  void* data; ///< a nip_sepset, void because of the order of definitions
  struct nip_sepsetlink *fwd; ///< the next element in the list, or NULL
  struct nip_sepsetlink *bwd; ///< the previous element in the list, or NULL
} nip_sepsetlink_struct;
typedef nip_sepsetlink_struct* nip_sepset_link; ///< sepset list reference

/**
 * Cliques: nodes of the join tree, containing a belief potential of related random variables
 */
typedef struct {
  nip_potential p; ///< current belief potential with evidence etc.
  nip_potential original_p; ///< the original model distribution, not messed with evidence
  nip_variable* variables; ///< related random variables, array size == p->dimensionality
  nip_sepset_link sepsets; ///< list of neighboring sepsets (and other cliques behind each)
  int num_of_sepsets; ///< number of sepsets, TODO: coupled with the list, but efficient?
  char mark; ///< the way to prevent endless loops, either MARK_ON or MARK_OFF
} nip_clique_struct;
typedef nip_clique_struct* nip_clique; ///< clique reference

/**
 * Sepsets: "intersections" of belief potentials between two nodes / cliques in a join tree.
 */
typedef struct {
  nip_potential old; ///< previous potential (before latest evidence)
  nip_potential new; ///< current potential (after latest evidence)
  nip_variable* variables; ///< related variables, size == old->num_of_vars
  nip_clique first_neighbour; ///< one of the two (neighbour) cliques
  nip_clique second_neighbour; ///< another of the two (neighbour) cliques
} nip_sepset_struct;
typedef nip_sepset_struct* nip_sepset; ///< sepset reference

/**
 * List item for storing parsed potentials while constructing the graph etc.
 * (when the cliques don't exist yet) */
typedef struct nip_potentiallink {
  nip_potential data; ///< conditional distribution of child given parents
  nip_variable child; ///< the variable whose dependency this is about
  nip_variable* parents; ///< the variables child depends on
  struct nip_potentiallink* fwd; ///< the next item in the list, or NULL
  struct nip_potentiallink* bwd; ///< the previous item in the list, or NULL
} nip_potential_link_struct;
typedef nip_potential_link_struct* nip_potential_link; ///< potential list item reference

/**
 * The list structure for linked lists of potentials, yet to be turned into a clique tree
 */
typedef struct {
  int length; ///< number of items in the list
  nip_potential_link first; ///< the beginning of the list
  nip_potential_link last; ///< the end of the list
} nip_potential_list_struct;
typedef nip_potential_list_struct* nip_potential_list; ///< potential list reference

/**
 * Method for creating cliques (without pointers to sepsets).
 * The array \p vars can be in any order.
 * @param vars Array of related random variables
 * @param nvars Size of \p vars
 * @return reference to a new clique, remember to free later
 * @see nip_free_clique()
 */
nip_clique nip_new_clique(nip_variable vars[], int nvars);

/**
 * Method for removing cliques and freeing memory.
 * @param c Reference to the clique to be freed */
void nip_free_clique(nip_clique c);

/**
 * Method for adding a sepset next to a clique. The sepset knows which cliques, but cliques don't
 * reference the sepset yet.
 * @return an error code, or 0 if successful */
int nip_confirm_sepset(nip_sepset s);

/**
 * Method for creating sepsets.
 * NOTE: the cliques don't reference the sepset until nip_confirm_sepset is called.
 * @param neighbour_a The first neighbour clique
 * @param neighbour_b The second neighbour clique (order not relevant, but there are 2)
 * @return reference to a new sepset, free it somewhere later
 * @see nip_confirm_sepset()
 * @see nip_free_sepset() */
nip_sepset nip_new_sepset(nip_clique neighbour_a, nip_clique neighbour_b);

/**
 * Method for removing sepsets and freeing memory.
 * @param s The sepset to be freed */
void nip_free_sepset(nip_sepset s);

/**
 * Method for creating belief potentials with correct structure.
 * @param variables Array of the variables in a suitable order
 * @param nvars Number of variables (potential dimensionality)
 * @param data A flat data array in the order according to variables
 * @return reference to a new potential
 * @see nip_free_potential() */
nip_potential nip_create_potential(nip_variable variables[], int nvars,
                                   double data[]);

/**
 * Method for unmarking a clique:
 * call this to every clique before collecting or distributing evidence.
 * @param c Reference to a clique to be unmarked (available for search) */
void nip_unmark_clique(nip_clique c);

/**
 * Tells how many variables a clique contains.
 * @param c Reference to a clique
 * @return number of variables, i.e. potential dimensionality */
int nip_clique_size(nip_clique c);

/**
 * Tells how many variables a sepset contains.
 * @param s Reference to a sepset
 * @return number of related variables */
int nip_sepset_size(nip_sepset s);

/**
 * Tells if the cliques are connected (in the same join tree).
 * Remember to UNMARK cliques before calling this.
 * @param one Reference to a clique
 * @param two Reference to another clique
 * @return 1 if connected, 0 if not
 * @see nip_unmark_clique() */
int nip_cliques_connected(nip_clique one, nip_clique two);

/**
 * Start distributing evidence from clique \p c outwards.
 * Remember to UNMARK cliques before calling this.
 * @param c Reference to a clique to start from
 * @return an error code, or 0 if successful
 * @see nip_unmark_clique() */
int nip_distribute_evidence(nip_clique c);

/**
 * Call Collect-Evidence from clique c1 (or nullpointer) for clique c2.
 * Sepset s12 is the sepset between c1 and c2.
 * Remember to UNMARK cliques before calling this.
 * @param c1 Reference to earlier clique, or NULL when starting recursion
 * @param s12 Reference to the sepset between c1 and c2, or NULL
 * @param c2 Reference to clique where evidence collected from (never NULL)
 * @return an error code, or 0 if successful
 * @see nip_unmark_clique() */
int nip_collect_evidence(nip_clique c1, nip_sepset s12, nip_clique c2);

/**
 * Method for finding out the joint probability distribution of arbitrary
 * variables by making a DFS in the join tree.
 * NOTE: \p isect MAY NOT contain any of the variables in \p vars
 * NOTE: Remember to unmark all cliques before calling this.
 * @param start The starting point for the recursion
 * @param vars Specifies the variables of interest and the order of variables in
 * the resulting potential.
 * @param n_vars Size of the \p vars array
 * @param isect An intersection used during recursion, initially NULL (empty)
 * @param n_isect Size of the intersection during recursion, initially 0
 * @return reference to the resulting belief potential (free afterwards)
 * @see nip_unmark_clique()
 * @see nip_free_potential() */
nip_potential nip_gather_joint_probability(nip_clique start,
                                           nip_variable *vars, int n_vars,
                                           nip_variable *isect, int n_isect);

/**
 * Initialises a clique with a belief potential, which describes the
 * conditional probability of the child variable given its parents.
 * This is basically a potential multiplication so that the clique keeps
 * the parameters even when retraction is used. If transient==1, the
 * potential can be "wiped" with retraction.
 *
 * NOTE: p->dimensionality equals "number of parents + 1"!
 * Sum of the elements in the potential is assumed to be 1.
 * The "ownership" of the potential changes, so do not free it.
 * @param c The clique to initialize with model parameters
 * @param child The variable whose dependency is being modeled
 * @param p Reference to potential describing CPD
 * @param transient 0 for permanent model parameters, 1 for temporary stuff
 * @return error code, or 0 if successful
 */
int nip_init_clique(nip_clique c, nip_variable child,
                    nip_potential p, int transient);

/**
 * This one calculates the probability distribution for a variable
 * according to the given clique. NOTE: the join tree should be
 * made consistent before this (enter, distribute, and collect evidence).
 * @param c Reference to a clique containing \p v
 * @param v Reference to the variable of interest
 * @param r Pointer to an array of size v->cardinality,
 * where the result gets written
 * @return error code, or 0 if successful
 * @see nip_enter_evidence()
 * @see nip_distribute_evidence()
 * @see nip_collect_evidence()
 */
int nip_marginalise_clique(nip_clique c, nip_variable v, double r[]);

/**
 * Method for backing away from impossibilities in observations.
 *
 * Typically, one would enter only non-contradicting evidence, but 0 probabilities cannot be updated
 * or "re-used" by means of multiplication, so this provides means to reset the whole model to
 * original model parameters without evidence messing things.
 * @param vars Array of all the variables in the model
 * @param nvars Size of the array \p vars
 * @param cliques Array of all the cliques in the join tree
 * @param ncliques Size of the array \p cliques
 * @return error code, or 0 if successful */
int nip_global_retraction(nip_variable* vars, int nvars,
                          nip_clique* cliques, int ncliques);

/**
 * Computes the so called probability mass of a clique tree.
 * Suitable for evaluating conditional probability of new evidence,
 * given the old weight. Hence, call this before and after entering
 * evidence of interest (and making the tree consistent).
 * @param cliques Array of all the cliques in the join tree
 * @param ncliques Size of the array \p cliques
 * @return remaining potential weight consistent with evidence so far */
double nip_probability_mass(nip_clique* cliques, int ncliques);

/**
 * Function for entering an observation to a clique tree.
 * The observed state of the variable is given as a string, and this is assumed to be hard / crisp
 * evidence without uncertainty (all other states are impossible after this, unless retracted).
 * Remember to make the join tree consistent after all evidence entered and before querying for
 * results of inference.
 * @param vars Array of all variables in the model
 * @param nvars Size of the array \p vars
 * @param cliques Array of all nodes in the join tree
 * @param ncliques Size of the array \p cliques
 * @param v The variable of interest
 * @param state Observed state of \p v as a string
 * @return error code, or 0 if successful
 * @see nip_enter_evidence() */
int nip_enter_observation(nip_variable* vars, int nvars,
                          nip_clique* cliques, int ncliques,
                          nip_variable v, char *state);

/**
 * Function for entering an observation to a clique tree.
 * The observed state of the variable is given as an index.
 * @param vars Array of all variables in the model
 * @param nvars Size of the array \p vars
 * @param cliques Array of all nodes in the join tree
 * @param ncliques Size of the array \p cliques
 * @param v The variable of interest
 * @param index The index of observed state of \p v: 0 being the first
 * @return error code, or 0 if successful
 * @see nip_variable_state_index() */
int nip_enter_index_observation(nip_variable* vars, int nvars,
                                nip_clique* cliques, int ncliques,
                                nip_variable v, int index);

/**
 * Function for entering (hard or soft) evidence to a clique tree.
 * Retracting the evidence or entering some new evidence about the
 * variable cancels the effects of this operation.
 * Size of the evidence array must equal \p v->cardinality.
 * This function might do a global retraction, if required.
 * @param vars Array of all variables in the model
 * @param nvars Size of the array \p vars
 * @param cliques Array of all nodes in the join tree
 * @param ncliques Size of the array \p cliques
 * @param v The variable of interest
 * @param evidence The observed probability of each state of \p v
 * @return error code, or 0 if successful */
int nip_enter_evidence(nip_variable* vars, int nvars,
                       nip_clique* cliques, int ncliques,
                       nip_variable v, double evidence[]);

/**
 * Function for entering a prior distribution to a clique tree.
 * Retracting evidence from the cliques cancels the effect of this operation,
 * but entering evidence about the variable does not.
 * Size of the prior array must equal \p v->cardinality.
 * @param vars Array of all variables in the model
 * @param nvars Size of the array \p vars
 * @param cliques Array of all nodes in the join tree
 * @param ncliques Size of the array \p cliques
 * @param v The variable of interest
 * @param prior Array of prior probabilities P(v)
 * @return error code, or 0 if successful
 */
int nip_enter_prior(nip_variable* vars, int nvars,
                    nip_clique* cliques, int ncliques,
                    nip_variable v, double prior[]);

/**
 * Finds a clique containing the family of the given variable.
 * @param cliques Array of all nodes in the join tree
 * @param ncliques Size of the array \p cliques
 * @param var The variable whose family is to be found
 * @return reference to the family clique, or NULL if not found
 * @see nip_find_clique() */
nip_clique nip_find_family(nip_clique* cliques, int ncliques, nip_variable var);

/**
 * Computes or finds a mapping from the family members to the clique
 * variables, so that you can use general_marginalise to compute
 * P(child | pa(child)) from the family clique.
 * @param family The node which models the probability distribution of the
 * child and its parents
 * @param child The dependent variable of interest
 * @return Array indicating which variables of the family clique are the child
 * and each of its parents. NOTE: Do not free the array (owned by \p child). */
int* nip_find_family_mapping(nip_clique family, nip_variable child);

/**
 * Finds a clique containing the specified set of variables.
 * Note that this is usually more expensive than nip_find_family().
 * @param cliques Array of all nodes in the join tree
 * @param ncliques Size of the array \p cliques
 * @param variables Array of the variables expected to be included in the clique
 * @param nvars Size of the array \p variables
 * @return reference to the clique of interest, or NULL if not found */
nip_clique nip_find_clique(nip_clique* cliques, int ncliques,
                           nip_variable* variables, int nvars);

/**
 * Prints the variables of the given clique.
 * @param stream An open output stream for writing
 * @param c The clique of interest */
void nip_fprintf_clique(FILE* stream, nip_clique c);

/**
 * Prints the variables of the given sepset.
 * @param stream An open output stream for writing
 * @param s The sepset of interest */
void nip_fprintf_sepset(FILE* stream, nip_sepset s);

/**
 * Finds the intersection of two cliques. Creates an array of those
 * variables that can be found in both cliques. Remember to free the
 * memory when the array is not needed anymore.
 * @param cl1 Clique of interest
 * @param cl2 Another clique of interest
 * @param vars Pointer where array reference to the result is written
 * @param n Pointer where size of the array \p *vars is written
 * @return error code, or 0 if successful */
int nip_clique_intersection(nip_clique cl1, nip_clique cl2,
                            nip_variable **vars, int *n);

/**
 * Creates an empty list for storing potentials and corresponding variables
 * @return Reference to an empty list
 * @see nip_free_potential_list() */
nip_potential_list nip_new_potential_list();

/**
 * Adds potentials to the end of a list
 * @param l The list to modify
 * @param p The potential to append
 * @param child The related child variable
 * @param parents Array of parent variables of \p child, to be freed by the list
 * @return an error code, or 0 if successful */
int nip_append_potential(nip_potential_list l, nip_potential p,
                         nip_variable child, nip_variable* parents);

/**
 * Adds potentials to the beginning of a list
 * @param l The list to modify
 * @param p The potential to prepend
 * @param child The related child variable
 * @param parents Array of parent variables of \p child, to be freed by the list
 * @return an error code, or 0 if successful */
int nip_prepend_potential(nip_potential_list l, nip_potential p,
                          nip_variable child, nip_variable* parents);

/**
 * Frees the memory allocated to a potential list.
 * NOTE: This frees also the actual potentials and parent variable arrays!
 * Variables themselves are kept intact.
 * @param l The list to be freed */
void nip_free_potential_list(nip_potential_list l);

#endif /* __NIPJOINTREE_H__ */
