/*
 * Jointree.h $Id: Jointree.h,v 1.2 2004-02-06 15:49:21 mvkorpel Exp $
 */

#ifndef __JOINTREE_H__
#define __JOINTREE_H__

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

#endif /* __JOINTREE_H__ */
