/*
 * Jointree.h $Id: Jointree.h,v 1.1 2004-02-02 13:34:31 mvkorpel Exp $
 */

#include "Clique.h"

typedef struct {
  /* No, mitäs tänne tulee??? */
  Clique *cliques;
} jointreetype;
typedef jointreetype *Jointree;

/* Method for creating jointrees
 */
Jointree make_Jointree(int num_of_cliques);

/* Method for removing jointrees and freeing memory: returns an error code */
int free_Jointree(Jointree j);

/* Method for adding a clique to the jointree: returns an error code */
int add_Clique(Jointree j, Clique c);
