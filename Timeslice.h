#ifndef __TIMESLICE_H__
#define __TIMESLICE_H__

#include "Graph.h"
#include "Variable.h"

#define F_ADJM(ts, i, j) ( (ts)->f_adjm[(i)*get_size((ts)->G) + (j)] )
#define S_ADJM(ts, i, j) ( (ts)->s_adjm[(i)*get_size((ts)->G) + (j)] ) 


typedef struct {
	Graph* G;
	int n_FC; /* Number of children in next timeslice, ie. Future Children */
	int n_SV; /* Number of initial variables, dynamic memory allocation */
	int n_SV_max; /* Memory management */
	Variable* SV; /* Start variables */
	int* f_adjm; /* Two dimensional, contains future children */
	int* s_adjm;	     /* Two dimensional, contains start vars (dynamic) */
} ts_type;
typedef ts_type* Timeslice;

/* XX AR: Oikeampi tapa tehd‰ t‰m‰ olisi rakentaa menetelm‰t geneeristen
   verkkojen yhdist‰miselle, jolloin alkumuuttujat voisi liimata
   valmiin auki rullatun verkon alkuun.
   
   Kysymys tosin lienee, onko t‰m‰ tarpeellista. Nyth‰n alkumuuttujat voivat
   viitata vain ensimm‰isen osaverkon muuttujiin. Saattaa riitt‰‰ moneen.
*/

Timeslice new_timeslice(Graph* G);
void ts_add_start(Timeslice ts, Variable v);
void ts_connect_start(Timeslice ts, Variable start_parent, Variable child);
void ts_connect_future(Timeslice ts, Variable parent, Variable future_child);
Graph* ts_unroll(Timeslice ts, unsigned T);

#ENDIF /*__TIMESLICE_H__*/