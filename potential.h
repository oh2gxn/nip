#ifndef __POTENTIAL_H__
#define __POTENTIAL_H__

/* definition of potentials */
typedef double* Potential;

/* method for initializing a potential */
Potential makePotential(int cardinality /*+other stuff*/);

/* method for copying a potential */
Potential copyPotential(Potential p);

/* method for marginalising over certain variables */
Potential marginalise(Potential pot, Variable* vars); /* WILL CHANGE */

/* method for multiplying potentials */
void mulPotential(Potential factor, Potential target);

#endif __POTENTIAL_H__
