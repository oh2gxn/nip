#ifndef __CLIQUE_H__
#define __CLIQUE_H__

struct sepsetlist {
  Sepset data;
  struct sepsetlist *fwd;
  struct sepsetlist *bwd;
};

typedef struct sepsetlist element;
typedef element *link;

typedef struct {
  potential p;
  Variable **variables; /* p contains num_of_vars */
  link sepsets;
  int num_of_sepsets;
  int mark; /* the way to prevent endless loops */
} cliquetype;

typedef cliquetype *Clique;

typedef struct {
  potential old;   /* previous potential */
  potential new;   /* current potential */
  Variable **variables; /* p contains num_of_vars */
  Clique cliques[2]; /* always between two cliques */
} sepsettype;

typedef sepsettype *Sepset;

/* Method for creating cliques (without pointers to sepsets) 
   - **variables is an array of pointers to variables */
Clique make_Clique(Variable **variables, int num_of_vars);

/* Method for adding a sepset next to a clique */
void add_sepset(Clique c, Sepset s);

/* Method for removing cliques and freeing memory */
void free_Clique(Clique c);

/* Method for creating sepsets: 
   - cliques[] is an array which contains references to BOTH 
     neighbouring cliques */
Sepset make_Sepset(Variable **variables, int num_of_vars, Clique cliques[]);

/* Method for removing sepsets and freeing memory */
void free_Sepset(Sepset s);

/* Method for unmarking a clique: call this to every clique before 
   collecting or distributing evidence. */
void unmark_Clique(Clique c);

/* Call Distribute-Evidence for c. */
void distribute_evidence(Clique c);

/* Call Collect-Evidence for Clique c2 from Clique c1 (or nullpointer). */
void collect_evidence(Clique c1, Clique c2);

/* Method for passing messages between cliques. */
void message_pass(Clique c1, Sepset s, Clique c2);

/* This will change or be removed */
void insert_evidence(Clique c, double *data);

#endif /* __CLIQUE_H__ */
