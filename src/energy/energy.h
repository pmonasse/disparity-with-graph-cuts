/**
 * @file energy.h
 * @brief Submodular binary variable energy minimization
 * @author Vladimir Kolmogorov <vnk@cs.cornell.edu>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 * 
 * Copyright (c) 2001-2003, 2012-2013, Vladimir Kolmogorov, Pascal Monasse
 * All rights reserved.
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Pulic License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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

   Example usage:
///////////////////////////////////////////////////
#include <iostream>
#include "energy.h"

// Minimize the following function of 3 binary variables:
// E(x, y, z) = x - 2*y + 3*(1-z) - 4*x*y + 5*|y-z|     
int main()
{
    Energy::Var x, y, z;
    Energy e;

    x = e.add_variable();
    y = e.add_variable();
    z = e.add_variable();

    e.add_term1(x, 0, 1);  // add term x 
    e.add_term1(y, 0, -2); // add term -2*y
    e.add_term1(z, 3, 0);  // add term 3*(1-z)

    e.add_term2(x, y, 0, 0, 0, -4); // add term -4*x*y
    e.add_term2(y, z, 0, 5, 5, 0); // add term 5*|y-z|

    Energy::TotalValue Emin = e.minimize();

    std::cout << "Minimum = " << Emin   << std::endl;
    std::cout << "Optimal solution:"    << std::endl;
    std::cout << "x = " << e.get_var(x) << std::endl;
    std::cout << "y = " << e.get_var(y) << std::endl;
    std::cout << "z = " << e.get_var(z) << std::endl;
    return 0;
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
    typedef short Value; ///< Type of a value in a single term
    typedef int TotalValue; ///< Type of a value of the total energy

    Energy();
    ~Energy();

    Var add_variable(Value E0=0, Value E1=0);
    void add_constant(Value E);
    void add_term1(Var x, Value E0, Value E1);
    void add_term2(Var x, Var y, Value E00, Value E01, Value E10, Value E11);

    TotalValue minimize();
    int get_var(Var x) const;

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
inline Energy::Var Energy::add_variable(Value E0, Value E1) {
    Energy::Var var = add_node();
    add_term1(var, E0, E1);
    return var;
}

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
inline int Energy::get_var(Var x) const { return (int)what_segment(x, SINK); }

#endif
