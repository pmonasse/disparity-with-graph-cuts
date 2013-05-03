/* energy.h */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2003. */

/* This software implements an energy minimization technique described in

   What Energy Functions can be Minimized via Graph Cuts?
   Vladimir Kolmogorov and Ramin Zabih. 
   IEEE Trans. on PAMI, 26(2), 2004 and  Proc. of ECCV, 2002. 

   It computes the global minimum of a function E of binary
   variables x_1, ..., x_n which can be written as a sum of terms involving
   at most three variables at a time:

       E(x_1, ..., x_n) = \sum_{i}     E^{i}    (x_i)
                        + \sum_{i,j}   E^{i,j}  (x_i, x_j)

   The method works only if each term is "regular". Definitions of regularity
   for terms E^{i}, E^{i,j} are given below as comments to functions
   add_term1(), add_term2(). 

   This software can be used only for research purposes.
   IF YOU USE THIS SOFTWARE,
   YOU SHOULD CITE THE AFOREMENTIONED PAPER IN ANY RESULTING PUBLICATION.

   In order to use it, you will also need a MAXFLOW software which can be
   obtained from http://www.cs.cornell.edu/People/vnk/software.html

   Example usage
   (Minimizes the following function of 3 binary variables:
       E(x, y, z) = x - 2*y + 3*(1-z) - 4*x*y + 5*|y-z|):

///////////////////////////////////////////////////
#include <stdio.h>
#include "energy.h"

// Minimize the following function of 3 binary variables:
// E(x, y, z) = x - 2*y + 3*(1-z) - 4*x*y + 5*|y-z|     
void main()
{
Energy::Var varx, vary, varz;
Energy e;

varx = e.add_variable();
vary = e.add_variable();
varz = e.add_variable();

e.add_term1(varx, 0, 1);  // add term x 
e.add_term1(vary, 0, -2); // add term -2*y
e.add_term1(varz, 3, 0);  // add term 3*(1-z)

e.add_term2(x, y, 0, 0, 0, -4); // add term -4*x*y
e.add_term2(y, z, 0, 5, 5, 0); // add term 5*|y-z|

Energy::TotalValue Emin = e.minimize();
  
printf("Minimum = %d\n", Emin);
printf("Optimal solution:\n");
printf("x = %d\n", e.get_var(varx));
printf("y = %d\n", e.get_var(vary));
printf("z = %d\n", e.get_var(varz));
}
///////////////////////////////////////////////////
*/

#ifndef ENERGY_H
#define ENERGY_H

#include <assert.h>
#include "graph.h"

class Energy : Graph<short,short,int>
{
public:
    typedef node_id Var;

    // Types of energy values.
    typedef short Value; ///< Type of a value in a single term
    typedef int TotalValue; ///< Type of a value of the total energy

    Energy();
    ~Energy();

    Var add_variable();
    void add_constant(Value E);
    void add_term1(Var x, Value E0, Value E1);
    void add_term2(Var x, Var y, Value E00, Value E01, Value E10, Value E11);

    TotalValue minimize();
    int get_var(Var x);

private:
    TotalValue Econst; ///< Constant added to the energy
};

/// Constructor
inline Energy::Energy()
: Graph(), Econst(0)
{}

/// Destructor
inline Energy::~Energy() {}

/// Add a new binary variable
inline Energy::Var Energy::add_variable() { return add_node(); }

/// Adds a constant to the energy function
inline void Energy::add_constant(Value A) { Econst += A; }

/// Adds a term E(x) of one binary variable to the energy function, where
/// E(0)=E0, E(1)=E1. E0 and E1 can be arbitrary.
inline void Energy::add_term1(Var x, Value E0, Value E1) {
    add_tweights(x, E1, E0);
}

/// Adds a term E(x,y) of two binary variables to the energy function, where
/// E(0,0)=A, E(0,1)=B, E(1,0)=C, E(1,1)=D.
/// The term must be regular, i.e. E00+E11 <= E01+E10
inline void Energy::add_term2(Var x, Var y, Value A, Value B, Value C, Value D){
    // E = A B = B B + A-B 0 +    0    0
    //     C D   D D   A-B 0   B+C-A-D 0
    add_tweights(x, D, B);
    add_tweights(y, 0, A-B);
    add_edge(x, y, 0, B+C-A-D);
}

/// After construction of the energy function, call this to minimize it.
/// Return the minimum of the function
inline Energy::TotalValue Energy::minimize() { return Econst + maxflow(); }

/// After 'minimize' has been called, determine the value of variable 'x'
/// in the optimal solution. Can be 0 or 1.
inline int Energy::get_var(Var x) { return (int)what_segment(x); }

#endif
