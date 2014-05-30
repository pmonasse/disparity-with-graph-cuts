/**
 * @file test_energy.cpp
 * @brief Example of usage of energy.h
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

#include <iostream>
#include "energy.h"

/// Minimize the following function of 3 binary variables:
/// E(x, y, z) = x - 2*y + 3*(1-z) - 4*x*y + 5*|y-z|
int main()
{
    Energy::Var x, y, z;
    Energy e;

    x = e.add_variable();
    y = e.add_variable();
    z = e.add_variable();

    e.add_term1(x, 0, 1);           // add term x
    e.add_term1(y, 0, -2);          // add term -2*y
    e.add_term1(z, 3, 0);           // add term 3*(1-z)

    e.add_term2(x, y, 0, 0, 0, -4); // add term -4*x*y
    e.add_term2(y, z, 0, 5, 5, 0);  // add term 5*|y-z|

    Energy::TotalValue Emin = e.minimize();

    std::cout << "Minimum = " << Emin   << std::endl;
    std::cout << "Optimal solution:"    << std::endl;
    std::cout << "x = " << e.get_var(x) << std::endl;
    std::cout << "y = " << e.get_var(y) << std::endl;
    std::cout << "z = " << e.get_var(z) << std::endl;
    return 0;
}
