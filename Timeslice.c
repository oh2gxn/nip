/*
 * Timeslice.c $Id: Timeslice.c,v 1.5 2004-06-21 06:12:12 mvkorpel Exp $
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Graph.h"
#include "Variable.h"
#include "Timeslice.h"

/* All time slice operations assume G is completely specified!
   That is, every variable must be added before using any of these,
   and every child relation specified before unrolling! */
Timeslice new_timeslice(Graph *G)
{
    Timeslice ts;
    int n = get_size(G);

    ts = (Timeslice) malloc(sizeof(Timeslice));
    ts->G = G;

    /* Startup variables */
    ts->n_SV = 0; ts->n_SV_max = n;
    ts->SV = (Variable*) calloc(ts->n_SV_max, sizeof(Variable));
    ts->s_adjm = (int*) calloc(n*ts->n_SV_max, sizeof(int));

    ts->n_FC = 0;
    ts->f_adjm = (int*) calloc(n*n, sizeof(int));
    memset(ts->f_adjm, 0, n*n*sizeof(int));
    
    return ts;
}

void ts_add_start(Timeslice ts, Variable v)
{
    int n;

    n = get_size(ts->G);

    /* Memory management */ /* Aleksis Kiveäkin kiinnostaa varata muistia */
    if (ts->n_SV == ts->n_SV_max)
    {
        ts->n_SV_max *= 2;
        ts->SV = (Variable*) realloc(ts->SV, ts->n_SV_max*sizeof(Variable));
        ts->s_adjm = (int*) realloc(ts->s_adjm, n*ts->n_SV_max*sizeof(int));
    }
    
    /* Set new portion of the adjancency matrix to 0 and
       add new variable to the list */
    memset(ts->SV + n*ts->n_SV, 0, n*sizeof(int));
    ts->SV[ts->n_SV++] = v; /* No checks. Dangerous. */
}

void ts_connect_start(Timeslice ts, Variable parent, Variable child)
{
    int i, parent_i = ts->n_SV;
    
    for (i = 0; i < ts->n_SV; i++) /* Super-efficient linear search */
        if (equal_variables(parent, ts->SV[i]))
        {
            parent_i = i; break;
        }
        
    if (parent_i == ts->n_SV)
        return;
        
    S_ADJM(ts, parent_i, get_graph_index(ts->G, child)) = 1;
}

void ts_connect_future(Timeslice ts, Variable parent, Variable child)
{
    int parent_i = get_graph_index(ts->G, parent);
    int child_i = get_graph_index(ts->G, child);
    F_ADJM(ts, parent_i, child_i) = 1; /* XX No checks. Dangerous. */
}

Graph* ts_unroll(Timeslice ts, unsigned T)
{
    int n, n_orig, i, j, iter, offset;
    Graph* G;
    Variable newvar;
    Variable* vars, *oldvars;
    char *symbol, itertag[VAR_SYMBOL_LENGTH+1];
    int tag_len, s_len;
    
    n_orig = get_size(ts->G);
    n = ts->n_SV + T*(n_orig - ts->n_FC) /*+ ts->n_FC*/;

    G = new_graph(n);
    vars = (Variable*) calloc(n_orig*T, sizeof(Variable));
    oldvars = get_variables(ts->G);
    
    /* First add every variable */
    for (i = 0; i < ts->n_SV; i++)
        add_variable(G, ts->    SV[i]);
    for (iter = 0; iter < T; iter++)
        for (i = 0; i < n_orig; i++)
        {
            newvar = copy_variable(oldvars[i]); /* Cro-Fra 2-1!!!!! */
            /*symbol = get_symbol(newvar);
            s_len = strnlen(symbol, VAR_SYMBOL_LENGTH);
            snprintf(itertag, VAR_SYMBOL_LENGTH, "-%i", iter);
            strcpy(symbol+s_len-tag_len, itertag, VAR_SYMBOL_LENGTH);*/
            
            vars[iter*n_orig + i] = newvar;
            add_variable(G, newvar);    
        }
    /*for (i = 0; i < ts->n_FC; i++) XX add last future children? */

    /* Then add_children. */
    for (i = 0; i < ts->n_SV; i++)        /* Starting vars */
        for (j = 0; j < n_orig; j++)
            if S_ADJM(ts, i, j)
                add_child(G, ts->SV[i], vars[j]);
    for (iter = 0; iter < T; iter++)
    {
        offset = iter*n_orig;
        for (i = 0; i < n_orig; i++)
        {
            for (j = 0; j < n_orig; j++)
            {
                if (is_child(ts->G, oldvars[i], oldvars[j]))
                    add_child(G, vars[offset+i], vars[offset+j]);
                if (F_ADJM(ts, i, j) && iter!=T-1)
                    add_child(G, vars[offset+i], vars[n_orig+offset+j]);
            }
        }
    }

    /* It's a wrap! */
    free(vars);    
    return G;
}

