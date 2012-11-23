#######################################################################
#                                                                     #
#    ENERGY - software for exact minimization of certain functions    #
#    of binary variables via computing mincut/maxflow in a graph      #
#                        Version 1.0                                  #
#    http://www.cs.cornell.edu/People/vnk/software.html               #
#                                                                     #
#    Vladimir N. Kolmogorov                                           #
#    vnk@cs.cornell.edu                          2003                 #
#                                                                     #
#######################################################################

1. Introduction.

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

The method works only if each term is "regular". The definition of regularity
is given below (and is also contained in the paper).

This software can be used only for research purposes. IF YOU USE THIS SOFTWARE,
YOU SHOULD CITE THE AFOREMENTIONED PAPER IN ANY RESULTING PUBLICATION.

Tested under windows, Visual C++ 6.0 compiler and unix (SunOS 5.8
and RedHat Linux 7.0, GNU c++ compiler).

##################################################################

2. Installation.

The distribution consists of two files: energy.h and README.TXT.
File energy.h contains functions that construct a graph corresponding to a
given energy function. In order to minimize the function (i.e. compute the
minimum cut on the graph), you will need a software implementing a mincut/maxflow
algrorithm. It can be obtained from

http://www.cs.cornell.edu/People/vnk/software.html ,
"MAXFLOW" software package (version 2.02 or higher).

##################################################################

3. Regularity.

The method implemented in this software works only if each term in
the energy function is "regulare". Here is the definition of regularity:

1. Any term E^{i}(x_i) of one binary variable is regular.
2. Term E^{i,j}(x_i, x_j) of two binary variables is regular if
	E^{i,j}(0,0) + E^{i,j}(1,1) <= E^{i,j}(0,1) + E^{i,j}(1,0)
3. Term E^{i,j,k}(x_i,x_j,x_k) of three binary variables is regular if
the following condition holds: if one of the variables is fixed
(for example, x_j=1), then the resulting function of two variables must
be regular. Since there are 6 ways to fix one variable (3 variables times
2 binary values - 0 and 1), this is equivalent to 6 inequalities.

##################################################################

4. Example usage.

This section shows how to use the library to minimize the following
function of 3 binary variables:
	E(x, y, z) = x - 2*y + 3*(1-z) - 4*x*y + 5*|y-z|
It has 5 terms: 3 terms depending on one variable and 2 terms depending
on two variables. Note that each term is regular.

///////////////////////////////////////////////////

#include <stdio.h>
#include "energy.h"

void main()
{
	/* Minimize the following function of 3 binary variables:
	   E(x, y, z) = x - 2*y + 3*(1-z) - 4*x*y + 5*|y-z| */
	   
	Energy::Var varx, vary, varz;
	Energy *e = new Energy();

	varx = e -> add_variable();
	vary = e -> add_variable();
	varz = e -> add_variable();

	e -> add_term1(varx, 0, 1);  /* add term x */
	e -> add_term1(vary, 0, -2); /* add term -2*y */
	e -> add_term1(varz, 3, 0);  /* add term 3*(1-z) */

	e -> add_term2(x, y, 0, 0, 0, -4); /* add term -4*x*y */
	e -> add_term2(y, z, 0, 5, 5, 0); /* add term 5*|y-z| */

	Energy::TotalValue Emin = e -> minimize();
	
	printf("Minimum = %d\n", Emin);
	printf("Optimal solution:\n");
	printf("x = %d\n", e->get_var(varx));
	printf("y = %d\n", e->get_var(vary));
	printf("z = %d\n", e->get_var(varz));

	delete e;
}

///////////////////////////////////////////////////
