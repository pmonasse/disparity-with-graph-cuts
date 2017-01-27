/**
 * @file kz2.cpp
 * @brief Graph representation of alpha-expansion
 * @author Vladimir Kolmogorov <vnk@cs.cornell.edu>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2001-2003, 2012-2014, Vladimir Kolmogorov, Pascal Monasse
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

#include "match.h"
#include "energy.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cassert>

/// (half of) the neighborhood system.
/// The full neighborhood system is edges in NEIGHBORS plus reversed edges.
const struct Coord NEIGHBORS[] = { Coord(-1,0), Coord(0,1) };
#define NEIGHBOR_NUM (sizeof(NEIGHBORS) / sizeof(Coord))

/// Compute the data+occlusion penalty (D(a)-K)
int Match::data_occlusion_penalty(Coord p, Coord q) const {
    int D = (imLeft? data_penalty_gray(p,q): data_penalty_color(p,q));
    return params.denominator*D - params.K;
}

/// Compute the smoothness penalty of assignments (p1,p1+d) and (p2,p2+d)
int Match::smoothness_penalty(Coord p1, Coord p2, int d) const {
    return (imLeft?
            smoothness_penalty_gray (p1,p2,d):
            smoothness_penalty_color(p1,p2,d));
}

/// Compute current energy.
/// We use this function only for sanity check.
int Match::ComputeEnergy() const {
    int E = 0;

    RectIterator end=rectEnd(imSizeL);
    for(RectIterator p1=rectBegin(imSizeL); p1!=end; ++p1) {
        int d1 = IMREF(d_left,*p1);
        if(d1!=OCCLUDED)
            E += data_occlusion_penalty(*p1, *p1+d1);

        for(unsigned int k=0; k<NEIGHBOR_NUM; k++) {
            Coord p2 = *p1 + NEIGHBORS[k];
            if(inRect(p2,imSizeL)) {
                int d2 = IMREF(d_left, p2);
                if(d1==d2) continue; // smoothness satisfied
                if(d1!=OCCLUDED && inRect( p2+d1,imSizeR))
                    E += smoothness_penalty(*p1, p2, d1);
                if(d2!=OCCLUDED && inRect(*p1+d2,imSizeR))
                    E += smoothness_penalty(*p1, p2, d2);
            }
        }
    }

    return E;
}

/// VAR_ALPHA means disparity alpha before expansion move (in vars0 and varsA)
static const Energy::Var VAR_ALPHA     = ((Energy::Var)-1);
/// VAR_ABSENT means occlusion in vars0, and p+alpha outside image in varsA
static const Energy::Var VAR_ABSENT = ((Energy::Var)-2);
/// Indicate if the variable has a regular value
inline bool IS_VAR(Energy::Var var) { return (var>=0); }

/// Build nodes in graph representing data+occlusion penalty for pixel p.
///
/// For assignments in A^0:       SOURCE means active, SINK means inactive.
/// For assigments in A^{\alpha}: SOURCE means inactive, SINK means active.
void Match::build_nodes(Energy& e, Coord p, int a) {
    int d = IMREF(d_left, p);
    Coord q = p+d;
    if(a==d) { // active assignment (p,p+a) in A^a will remain active
        IMREF(vars0, p) = VAR_ALPHA;
        IMREF(varsA, p) = VAR_ALPHA;
        e.add_constant(data_occlusion_penalty(p,q));
        return;
    }

    IMREF(vars0, p) = (d!=OCCLUDED)? // (p,p+d) in A^0 can remain active
        e.add_variable(data_occlusion_penalty(p,q), 0): VAR_ABSENT;

    q = p+a;
    IMREF(varsA, p) = inRect(q,imSizeR)? // (p,p+a) in A^a can become active
        e.add_variable(0, data_occlusion_penalty(p,q)): VAR_ABSENT;
}

/// Build smoothness term for neighbor pixels p1 and p2 with disparity a.
void Match::build_smoothness(Energy& e, Coord p1, Coord p2, int a) {
    int d1 = IMREF(d_left, p1);
    Energy::Var o1 = (Energy::Var) IMREF(vars0, p1);
    Energy::Var a1 = (Energy::Var) IMREF(varsA, p1);

    int d2 = IMREF(d_left, p2);
    Energy::Var o2 = (Energy::Var) IMREF(vars0, p2);
    Energy::Var a2 = (Energy::Var) IMREF(varsA, p2);

    // disparity a
    if(a1!=VAR_ABSENT && a2!=VAR_ABSENT) {
        int delta = smoothness_penalty(p1, p2, a);
        if(a1 != VAR_ALPHA) { // (p1,p1+a) is variable
            if(a2 != VAR_ALPHA) // Penalize different activity
                e.add_term2(a1, a2, 0, delta, delta, 0);
            else // Penalize (p1,p1+a) inactive
                e.add_term1(a1, delta, 0);
        } else if(a2 != VAR_ALPHA) // (p1,p1+a) active, (p2,p2+a) variable
            e.add_term1(a2, delta, 0); // Penalize (p2,p2+a) inactive
    }

    // disparity d==nd!=a
    if(d1==d2 && IS_VAR(o1) && IS_VAR(o2)) {
        assert(d1!=a && d1!=OCCLUDED);
        int delta = smoothness_penalty(p1,p2,d1);
        e.add_term2(o1, o2, 0, delta, delta, 0); // Penalize different activity
    }

    // disparity d1, a!=d1!=d2, (p2,p2+d1) inactive neighbor assignment
    if(d1!=d2 && IS_VAR(o1) && inRect(p2+d1,imSizeR))
        e.add_term1(o1, smoothness_penalty(p1,p2,d1), 0);

    // disparity d2, a!=d2!=d1, (p1,p1+d2) inactive neighbor assignment
    if(d2!=d1 && IS_VAR(o2) && inRect(p1+d2,imSizeR))
        e.add_term1(o2, smoothness_penalty(p1,p2,d2), 0);
}

/// Build edges in graph enforcing uniqueness at pixels p and p+d:
/// - Prevent (p,p+d) and (p,p+a) from being both active.
/// - Prevent (p,p+d) and (p+d-alpha,p+d) from being both active.
void Match::build_uniqueness(Energy& e, Coord p, int alpha) {
    Energy::Var o = (Energy::Var) IMREF(vars0, p);
    if(! IS_VAR(o))
        return;

    // Enfore unique image of p
    Energy::Var a = (Energy::Var) IMREF(varsA, p);
    if(a!=VAR_ABSENT)
        e.forbid01(o,a);

    // Enforce unique antecedent at p+d
    int d = IMREF(d_left, p);
    assert(d!=OCCLUDED);
    p = p+(d-alpha);
    if(inRect(p,imSizeL)) {
        a = (Energy::Var) IMREF(varsA, p);
        assert(IS_VAR(a)); // not active because of current uniqueness
        e.forbid01(o, a);
    }
}

/// Update the disparity map according to min cut of energy.
/// We need to set d_right for smoothness term in next expansion move.
void Match::update_disparity(const Energy& e, int alpha) {
    RectIterator end=rectEnd(imSizeL);
    for(RectIterator p=rectBegin(imSizeL); p!=end; ++p) {
        Energy::Var o = (Energy::Var) IMREF(vars0,*p);
        if(IS_VAR(o) && e.get_var(o)==1)
            IMREF(d_left,*p) = IMREF(d_right,*p+IMREF(d_left,*p)) = OCCLUDED;
    }
    for(RectIterator p=rectBegin(imSizeL); p!=end; ++p) {
        Energy::Var a = (Energy::Var) IMREF(varsA,*p);
        if(IS_VAR(a) && e.get_var(a)==1) // New disparity
            IMREF(d_right,*p+alpha) = -(IMREF(d_left,*p) = alpha);
    }
}

/// Compute the minimum a-expansion configuration.
///
/// Return whether the move is different from identity.
bool Match::ExpansionMove(int a) {
    // Factors 2 and 12 are minimal ensuring no reallocation
    Energy e(2*imSizeL.x*imSizeL.y, 12*imSizeL.x*imSizeL.y);

    // Build graph
    RectIterator endL=rectEnd(imSizeL);
    for(RectIterator p=rectBegin(imSizeL); p!=endL; ++p)
        build_nodes(e, *p, a);

    for(RectIterator p1=rectBegin(imSizeL); p1!=endL; ++p1)
        for(unsigned int k=0; k<NEIGHBOR_NUM; k++) {
            Coord p2 = *p1+NEIGHBORS[k];
            if(inRect(p2,imSizeL))
                build_smoothness(e, *p1, p2, a);
        }

    for(RectIterator p=rectBegin(imSizeL); p!=endL; ++p)
        build_uniqueness(e, *p, a);

    int oldE=E;
    E = e.minimize(); // Max-flow, give the lowest-energy expansion move

    if(E<oldE) { // lower energy, accept the expansion move
        update_disparity(e, a);
        assert(ComputeEnergy()==E);
        return true;
    }
    return false;
}

/// Generate a random permutation of the array elements.
///
/// Fisher-Yates shuffle: http://en.wikipedia.org/wiki/Fisher–Yates_shuffle
static void generate_permutation(int *buf, int n) {
    for(int i=0; i<n; i++) buf[i] = i;
    for(int i=0; i<n-1; i++) {
        int j = i + (int) (((double)rand()/RAND_MAX)*(n - i));
        if(j >= n) // Very unlikely, but still possible
            continue;
        std::swap(buf[i],buf[j]);
    }
}

/// Main algorithm: a series of alpha-expansions.
void Match::run() {
    // Display 1 number after decimal separator for number of iterations
    std::cout << std::fixed << std::setprecision(1);

    const int dispSize = dispMax-dispMin+1;
    int* permutation = new int[dispSize]; // random permutation

    E = ComputeEnergy();
    std::cout << "E=" << E << std::endl;

    bool* done = new bool[dispSize]; // Can expansion of label decrease energy?
    std::fill_n(done, dispSize, false);
    int nDone = dispSize; // number of 'false' entries in 'done'

    int step=0;
    for(int iter=0; iter<params.maxIter && nDone>0; iter++) {
        if(iter==0 || params.bRandomizeEveryIteration)
            generate_permutation(permutation, dispSize);

        for(int index=0; index<dispSize; index++) {
            int label = permutation[index];
            if(done[label]) continue;
            ++step;

            if( ExpansionMove(dispMin+label) ) {
                std::fill_n(done, dispSize, false);
                nDone = dispSize;
                std::cout << '*';
            } else
                std::cout << '-';
            std::cout << std::flush;
            done[label] = true;
            --nDone;
        }
        std::cout << " E=" << E << std::endl;
    }

    std::cout << (float)step/dispSize << " iterations" << std::endl;

    delete [] permutation;
    delete [] done;
}

/// Main algorithm
void Match::KZ2() {
    if(params.K<0 || params.edgeThresh<0 ||
        params.lambda1<0 || params.lambda2<0 || params.denominator<1) {
        std::cerr << "Error in KZ2: wrong parameter!" << std::endl;
        exit(1);
    }

    std::string strDenom; // Denominator as output string
    if(params.denominator!=1) {
        std::ostringstream s;
        s << params.denominator;
        strDenom = "/" + s.str();
    }
    std::cout << "KZ2:  K=" << params.K << strDenom <<std::endl
              << "      edgeThreshold=" << params.edgeThresh
              << ", lambda1=" << params.lambda1 << strDenom
              << ", lambda2=" << params.lambda2 << strDenom
              << ", dataCost = L" <<
        ((params.dataCost==Parameters::L1)? '1': '2') << std::endl;

    run();
}
