/* energy.h */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2003. */

/*
	This software implements an energy minimization technique described in

	What Energy Functions can be Minimized via Graph Cuts?
	Vladimir Kolmogorov and Ramin Zabih. 
	To appear in IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI). 
	Earlier version appeared in European Conference on Computer Vision (ECCV), May 2002. 

	More specifically, it computes the global minimum of a function E of binary
	variables x_1, ..., x_n which can be written as a sum of terms involving
	at most three variables at a time:

		E(x_1, ..., x_n) = \sum_{i}     E^{i}    (x_i)
		                 + \sum_{i,j}   E^{i,j}  (x_i, x_j)
		                 + \sum_{i,j,k} E^{i,j,k}(x_i, x_j, x_k)

	The method works only if each term is "regular". Definitions of regularity
	for terms E^{i}, E^{i,j}, E^{i,j,k} are given below as comments to functions
	add_term1(), add_term2(), add_term3(). 

	This software can be used only for research purposes. IF YOU USE THIS SOFTWARE,
	YOU SHOULD CITE THE AFOREMENTIONED PAPER IN ANY RESULTING PUBLICATION.

	In order to use it, you will also need a MAXFLOW software which can be
	obtained from http://www.cs.cornell.edu/People/vnk/software.html


	Example usage
	(Minimizes the following function of 3 binary variables:
	E(x, y, z) = x - 2*y + 3*(1-z) - 4*x*y + 5*|y-z|):

	///////////////////////////////////////////////////

	#include <stdio.h>
	#include "energy.h"

	void main()
	{
		// Minimize the following function of 3 binary variables:
		// E(x, y, z) = x - 2*y + 3*(1-z) - 4*x*y + 5*|y-z|
		   
		Energy::Var varx, vary, varz;
		Energy *e = new Energy();

		varx = e -> add_variable();
		vary = e -> add_variable();
		varz = e -> add_variable();

		e -> add_term1(varx, 0, 1);  // add term x 
		e -> add_term1(vary, 0, -2); // add term -2*y
		e -> add_term1(varz, 3, 0);  // add term 3*(1-z)

		e -> add_term2(x, y, 0, 0, 0, -4); // add term -4*x*y
		e -> add_term2(y, z, 0, 5, 5, 0); // add term 5*|y-z|

		Energy::TotalValue Emin = e -> minimize();
		
		printf("Minimum = %d\n", Emin);
		printf("Optimal solution:\n");
		printf("x = %d\n", e->get_var(varx));
		printf("y = %d\n", e->get_var(vary));
		printf("z = %d\n", e->get_var(varz));

		delete e;
	}

	///////////////////////////////////////////////////
*/

#ifndef __ENERGY_H__
#define __ENERGY_H__

#include <assert.h>
#include "graph.h"

class Energy : Graph<short,short,int>
{
public:
	typedef node_id Var;

	/* Types of energy values.
	   Value is a type of a value in a single term
	   TotalValue is a type of a value of the total energy.
	   By default Value = short, TotalValue = int.
	   To change it, change the corresponding types in graph.h */
	typedef short Value;
	typedef int TotalValue;

	/* interface functions */

	/* Constructor. */
	Energy();

	/* Destructor */
	~Energy();

	/* Adds a new binary variable */
	Var add_variable();

	/* Adds a constant E to the energy function */
	void add_constant(Value E);

	/* Adds a new term E(x) of one binary variable
	   to the energy function, where
	       E(0) = E0, E(1) = E1
	   E0 and E1 can be arbitrary */
	void add_term1(Var x,
	               Value E0, Value E1);

	/* Adds a new term E(x,y) of two binary variables
	   to the energy function, where
	       E(0,0) = E00, E(0,1) = E01
	       E(1,0) = E10, E(1,1) = E11
	   The term must be regular, i.e. E00 + E11 <= E01 + E10 */
	void add_term2(Var x, Var y,
	               Value E00, Value E01,
	               Value E10, Value E11);

	/* After the energy function has been constructed,
	   call this function to minimize it.
	   Returns the minimum of the function */
	TotalValue minimize();

	/* After 'minimize' has been called, this function
	   can be used to determine the value of variable 'x'
	   in the optimal solution.
	   Returns either 0 or 1 */
	int get_var(Var x);

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

private:
	/* internal variables and functions */

	TotalValue	Econst;
};

/***********************************************************************/
/************************  Implementation ******************************/
/***********************************************************************/

inline Energy::Energy() : Graph()
{
	Econst = 0;
}

inline Energy::~Energy() {}

inline Energy::Var Energy::add_variable() {	return add_node(); }

inline void Energy::add_constant(Value A) { Econst += A; }

inline void Energy::add_term1(Var x,
                              Value A, Value B)
{
	add_tweights(x, B, A);
}

inline void Energy::add_term2(Var x, Var y,
                              Value A, Value B,
                              Value C, Value D)
{
	/* 
	   E = A A  +  0   B-A
	       D D     C-D 0
	   Add edges for the first term
	*/
	add_tweights(x, D, A);
	B -= A; C -= D;

	/* now need to represent
	   0 B
	   C 0
	*/

	assert(B + C >= 0); /* check regularity */
	if (B < 0)
	{
		/* Write it as
		   B B  +  -B 0  +  0   0
		   0 0     -B 0     B+C 0
		*/
		add_tweights(x, 0, B); /* first term */
		add_tweights(y, 0, -B); /* second term */
		add_edge(x, y, 0, B+C); /* third term */
	}
	else if (C < 0)
	{
		/* Write it as
		   -C -C  +  C 0  +  0 B+C
		    0  0     C 0     0 0
		*/
		add_tweights(x, 0, -C); /* first term */
		add_tweights(y, 0, C); /* second term */
		add_edge(x, y, B+C, 0); /* third term */
	}
	else /* B >= 0, C >= 0 */
	{
		add_edge(x, y, B, C);
	}
}

inline Energy::TotalValue Energy::minimize() { return Econst + maxflow(); }

inline int Energy::get_var(Var x) { return (int)what_segment(x); }

#endif
