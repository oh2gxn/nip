#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "Graph.h"
#include "Variable.h"


#define STATE_SHIFT (0.66)
#define EMIT_P (0.8)
#define WAYNEGRETZKY 99

/* Random number between 0 and 1 */
double urnd()
{
    return rand()/((double)RAND_MAX)
}

char** gen_obs(int n)
{
    int i, curr_state = 0;
    char** observations;
    
    observations = (char**) calloc(n, sizeof(char*));
    
    for (i = 0; i < n; i++)
    {
        /* State flip */
        if (urnd() > STATE_SHIFT)
            curr_state ^= 1;
        /* Emit symbol */
        if (urnd() < EMIT_P)
            observations[i] = curr_state? "A":"B";
        else
            observations[i] = curr_state? "B":"A";
    }
    return observations;
}

void make_consistent(Clique* cliques, int n_cliques)
{
    int i;
    for (i = 0; i < n_cliques; i++)
        unmark_Clique(cliques[i]);
    collect_evidence(NULL, NULL, cliques[0]);
    for (i = 0; i < n_cliques; i++)
        unmark_Clique(cliques[i]);
    distribute_evidence(cliques[0]);
}

int get_distribution_1(Variable v, Clique* cliques, int n, double** data)
{
    int n_states;
    Clique c;
    
    n_states = number_of_values(v);
    *data = (double*) calloc(n_states, sizeof(double));
    c = find_family(cliques, n, &v, 1);
    marginalise(c, H1, *data);
    normalise(*data, n_states);
    
    return n_states;
}

int main() {

	Clique* cliques;
    int i;
	int n_cliques, n_states;
	Variable H0, H1, O1;
	Graph *G;
	char** observations;
	double* prob;
	
	char*[2] Hstates = ["False", "True"];
	char*[2] Ostates = ["A", "B"];

	/* T->T, ja F->F = 0.66; T yleensä (.8) emittoi A:n, F B:n */

	H0 = new_variable("H0", "Hidden", Hstates, 2);
	H1 = new_variable("H1", "Hidden", Hstates, 2);
	O1 = new_variable("O1", "Observed", Ostates, 2);

	G = new_graph(3);
	add_variable(H0); add_variable(H1); add_variable(O1);
	add_child(H0, H1); add_child(H1, O1);
	
	n_cliques = find_cliques(G, &cliques);
    find_sepsets(cliques, n_cliques);
	assert(n_cliques == 2);

    observations = gen_obs(100);
    
    /* Initialize potentials XX */
    make_consistent(cliques, n_cliques);
    for (i = 0; i < 100; i++)
    {
        n_states = get_distribution_1(H1, cliques, n_cliques, &prob);
        enter_evidence(H0, prob);
        assert(n_states == 2);
               /*XX Poista H1:n evidenssi? Aseta yhtä suureksi?
                * Pitäisikö näitä tallentaa paluumatkalle? */
               /* Vai jotain muuta? A_ij ihan irralleen?*/
        enter_observation(O1, observations[i]);
        make_consistent(cliques, n_cliques);
    } /* [H0H1] -(H1)- [H1O1] */
    for (i = WAYNEGRETZKY; i >= 0; i--)
    {
        /* crab canon */
    }

}
