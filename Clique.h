#ifndef __CLIQUE_H__
#define __CLIQUE_H__

struct sepsetlist {
  Sepset *data;
  struct sepsetlist *fwd;
  struct sepsetlist *bwd;
};

typedef struct sepsetlist element;
typedef element *link;

typedef struct {
  potential p;
  Variable **variables;
  link sepsets;
  int num_of_sepsets;
} Clique;

typedef struct {
  potential old;   /* previous potential */
  potential new;   /* current potential */
  Variable **variables;
  Clique* cliques[2];
} Sepset;

Clique make_Clique(Variable **variables, int num_of_vars);

void add_sepset(Clique *c, Sepset *s);

Sepset make_Sepset(Variable **variables, int num_of_vars, Clique **cliques);

/* This will change or be removed */
void insert_evidence(Clique c, double *data);

void distribute_evidence(Clique c); /* ??? */

void collect_evidence(Clique c); /* ??? */

#endif /* __CLIQUE_H__ */
