#ifndef __CLIQUE_H__
#define __CLIQUE_H__

#include "potential.h"
#include "Variable.h"

struct sepsetlist {
  void *data; /* void is needed because of the order of definitions */
  struct sepsetlist *fwd;
  struct sepsetlist *bwd;
};

typedef struct sepsetlist element;
typedef element *link;

typedef struct {
  potential p;
  Variable *variables; /* p contains num_of_vars */
  link sepsets;
  int num_of_sepsets;
  int mark; /* the way to prevent endless loops */
} cliquetype;
typedef cliquetype *Clique;

typedef struct {
  potential old;   /* previous potential */
  potential new;   /* current potential */
  Variable *variables; /* p contains num_of_vars */
  Clique *cliques; /* always between two cliques */
} sepsettype;

typedef sepsettype *Sepset;

/* Method for creating cliques (without pointers to sepsets) 
   - MVK: vars[] is an array of Variables (which are pointers) */
Clique make_Clique(Variable vars[], int num_of_vars);

/* Method for removing cliques and freeing memory: returns an error code */
int free_Clique(Clique c);

/* Method for adding a sepset next to a clique: returns an error code */
int add_Sepset(Clique c, Sepset s);

/* Method for creating sepsets: 
   - cliques[] is an array which contains references to BOTH 
     neighbouring cliques */
Sepset make_Sepset(Variable variables[], int num_of_vars, Clique cliques[]);

/* Method for removing sepsets and freeing memory: returns an error code */
int free_Sepset(Sepset s);

/* Method for creating potentials with correct structure.
 * - variables[] is an array of the Variables in a suitable order
 * - num_of_vars tells how many variables there are
 * - data[] is the data array in the order according to variables */
potential create_Potential(Variable variables[], int num_of_vars, 
			   double data[]);

/* Method for cleaning potentials and releasing the memory. 
 * Returns an error code. */
int free_Potential(potential p);

/* Method for unmarking a clique: call this to every clique before 
   collecting or distributing evidence. Returns an error code. */
int unmark_Clique(Clique c);

/* Call Distribute-Evidence for c. Returns an error code. */
int distribute_evidence(Clique c);

/* Call Collect-Evidence from Clique c1 (or nullpointer) for Clique c2. 
   Sepset s12 is the sepset between c1 and c2 or nullpointer to get
   started. Returns an error code. */
int collect_evidence(Clique c1, Sepset s12, Clique c2);

/* Method for passing messages between cliques. Returns an error code. */
int message_pass(Clique c1, Sepset s, Clique c2);

/* Make up a better name for this */
/* !!! p->num_of_vars equals "length of parents + 1" !!! 
 * Sum of the elements in the potential is assumed to be 1. */
int initialise(Clique c, Variable v, Variable parents[], potential p);

/* This one calculates the probability distribution for a variable v
   according to the clique c. To make sense, the join tree should be 
   made consistent before this. 
- The result is placed in the array r
- The returned value is an error code. */
int marginalise(Clique c, Variable v, double r[]);

/* Normalises the array. Divides every member by their sum.
 * The function modifies the given array.
 */
int normalise(double result[], int array_size);

/* Method for entering evidence to a clique. 
   sizeof(evidence) must equal variable->cardinality.
   Returns an error code. *** NOTE: If this returns GLOBAL_RETRACTION, 
   it failed and an initialisation should be made before calling this 
   again.*/
int enter_evidence(Clique c, Variable v, double evidence[]);

/* Method for checking if Variable v is part of Clique c.
   Returns -1 if not, else the index of v among the Variables in c. */
int var_index(Clique c, Variable v);

#endif /* __CLIQUE_H__ */
