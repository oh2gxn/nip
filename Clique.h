#ifndef __CLIQUE_H__
#define __CLIQUE_H__

typedef struct {
  potential p;
  Variable **variables;
  Sepset **sepsets;
  int num_of_sepsets;
} Clique;

typedef struct {
  potential old;   /* previous potential */
  potential new;   /* current potential */
  Variable **variables;
  Clique* cliques[2];
} Sepset;

/* TODO */
Clique make_Clique();

/* TODO */
Sepset make_Sepset();

#endif /* __CLIQUE_H__ */
