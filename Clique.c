#include "Clique.h"
#include "Variable.h"
#include "potential.h"

Clique make_Clique(Variable **variables, int num_of_vars){
  Clique c;
  int *cardinality = (int *) calloc(num_of_vars, sizeof(int));
  int i;
  for(i = 0; i < num_of_vars; i++){
    cardinality[i] = *(variables[i]).cardinality;
  }
  c.p = make_potential(cardinality, num_of_vars);
  c.variables = variables;
  c.sepsets = 0;
  return c;
}

void add_sepset(Clique *c, Sepset *s){
  link new = (link) malloc(sizeof(element));
  new->data = s;
  new->fwd = c->sepsets;
  new->bwd = 0;
  if(c->sepsets != 0)
    c->sepsets->bwd = new;
  c->sepsets = new;
}

Sepset make_Sepset(Variable **variables, int num_of_vars, Clique **cliques){
  Sepset s;
  int *cardinality = (int *) calloc(num_of_vars, sizeof(int));
  int i;
  for(i = 0; i < num_of_vars; i++){
    cardinality[i] = *(variables[i]).cardinality;
  }
  s.old = make_potential(cardinality, num_of_vars);
  s.new = make_potential(cardinality, num_of_vars);
  s.variables = variables;
  s.cliques = cliques;
  return s;
}
