/* kz2.cpp */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001-2003. */

#include "match.h"
#include "energy.h"

/// Compute the data+occlusion penalty (D(a)-K)
int Match::data_occlusion_penalty(Coord l, Coord r) const
{
    int D = (im_left? data_penalty_gray(l,r): data_penalty_color(l,r));
    return params.denominator*D - params.K;
}

/// Compute the smoothness penalty of assignments (p,p+d) and (np,np+d)
int Match::smoothness_penalty(Coord p, Coord np, int d) const
{
    return (im_left?
            smoothness_penalty_gray (p,np,d):
            smoothness_penalty_color(p,np,d));
}

/// Compute current energy.
/// We use this function only for sanity check.
int Match::ComputeEnergy() const
{
    int E = 0;

    Coord p;
    for(p.y=0; p.y<im_size.y; p.y++)
        for(p.x=0; p.x<im_size.x; p.x++) {
            int d = IMREF(x_left, p);
            if (d != OCCLUDED)
                E += data_occlusion_penalty(p, p+d);

            for (unsigned int k=0; k<NEIGHBOR_NUM; k++) {
                Coord np = p + NEIGHBORS[k];
                if (np>=Coord(0,0) && np<im_size) {
                    int nd = IMREF(x_left, np);
                    if(d == nd) continue; // smoothness satisfied
                    if( d!=OCCLUDED && np+d>=Coord(0,0) && np+d<im_size)
                        E += smoothness_penalty(p, np, d);
                    if(nd!=OCCLUDED && p+nd>=Coord(0,0) && p+nd<im_size)
                        E += smoothness_penalty(p, np, nd);
                }
            }
        }

    return E;
}

/// VAR_ACTIVE means disparity alpha before expansion move (in vars0 and varsA)
static const Energy::Var VAR_ACTIVE     = ((Energy::Var)-1);
/// VAR_NONPRESENT means occlusion in vars0, and p+alpha outside image in varsA
static const Energy::Var VAR_NONPRESENT = ((Energy::Var)-2);
/// Indicate if the variable has a regular value
inline bool IS_VAR(Energy::Var var) { return (var>=0); }

/// Build nodes in graph representing data+occlusion penalty for pixel p.
///
/// For assignments in A^0:       SOURCE means 1, SINK means 0.
/// For assigments in A^{\alpha}: SOURCE means 0, SINK means 1.
void Match::build_nodes(Energy& e, Coord p, int a) {
    int d = IMREF(x_left, p);
    Coord pd = p+d;
    if(a==d) { // active assignment (p,p+a) in A^a will remain active
        IMREF(vars0, p) = VAR_ACTIVE;
        IMREF(varsA, p) = VAR_ACTIVE;
        e.add_constant(data_occlusion_penalty(p, pd));
        return;
    }

    IMREF(vars0, p) = (d!=OCCLUDED)? // assignment (p,p+d) in A^0
        e.add_variable(data_occlusion_penalty(p, pd), 0): VAR_NONPRESENT;

    Coord pa = p+a;
    IMREF(varsA, p) = (pa>=Coord(0,0)&&pa<im_size)? // assignment (p,p+a) in A^a
        e.add_variable(0, data_occlusion_penalty(p, pa)): VAR_NONPRESENT;
}

/// Build smoothness term for pixels p and neighbor np with disparity a.
void Match::build_smoothness(Energy& e, Coord p, Coord np, int a) {
    int d = IMREF(x_left, p);
    Energy::Var var0 = (Energy::Var) IMREF(vars0, p);
    Energy::Var varA = (Energy::Var) IMREF(varsA, p);

    int nd = IMREF(x_left, np);
    Energy::Var nvar0 = (Energy::Var) IMREF(vars0, np);
    Energy::Var nvarA = (Energy::Var) IMREF(varsA, np);

    // disparity a
    if(varA!=VAR_NONPRESENT && nvarA!=VAR_NONPRESENT) {
        int delta = smoothness_penalty(p, np, a);
        if(varA != VAR_ACTIVE) { // (p,p+a) is variable
            if(nvarA != VAR_ACTIVE) // Penalize different activity
                e.add_term2(varA, nvarA, 0, delta, delta, 0);
            else // Penalize (p,p+a) inactive
                e.add_term1(varA, delta, 0);
        } else if(nvarA != VAR_ACTIVE) // (p,p+a) active, (np,np+a) variable
            e.add_term1(nvarA, delta, 0); // Penalize (np,np+a) inactive
    }

    // disparity d (if not a)
    if(IS_VAR(var0) && np+d>=Coord(0,0) && np+d<im_size) {
        int delta = smoothness_penalty(p, np, d);
        if(d == nd) // Penalize different activity
            e.add_term2(var0, nvar0, 0, delta, delta, 0);
        else // (np,np+d) inactive, so penalize (p,p+d) active
            e.add_term1(var0, delta, 0);
    }

    // disparity nd (not a or d): (p,p+nd) inactive, penalize (np,p+nd) active
    if(IS_VAR(nvar0) && d!=nd && p+nd>=Coord(0,0) && p+nd<im_size) {
        int delta = smoothness_penalty(p, np, nd);
        e.add_term1(nvar0, delta, 0);
    }
}

/// Build edges in graph enforcing uniqueness at pixel p
void Match::build_uniqueness(Energy& e, Coord p, int a) {
    const int INF = 10000; /// Infinite capacity

    Energy::Var var0 = (Energy::Var) IMREF(vars0, p);
    Energy::Var varA = (Energy::Var) IMREF(varsA, p);
    
    // Prevent (p,p+d) and (p,p+a) from being both active
    if(IS_VAR(var0) && varA!=VAR_NONPRESENT)
        e.add_term2(var0, varA, 0, INF, 0, 0);

    // Prevent (q-d,q) and (q-a,q) from being both active
    int d = IMREF(x_right, p);
    if(d==OCCLUDED) return;
    var0 = (Energy::Var) IMREF(vars0, p+d);
    if(var0!=VAR_ACTIVE) {
        Coord pa = p-a;
        if (pa>=Coord(0,0) && pa<im_size) {
            varA = (Energy::Var) IMREF(varsA, pa);
            e.add_term2(var0, varA, 0, INF, 0, 0);
        }
    }
}

/// Update the disparity map according to min cut of energy.
/// We need to set x_right for smoothness term in next expansion move.
void Match::update_disparity(const Energy& e, int a) {
    Coord p;
    for (p.y=0; p.y<im_size.y; p.y++)
        for (p.x=0; p.x<im_size.x; p.x++) {
            Energy::Var var0 = (Energy::Var) IMREF(vars0, p);
            if(IS_VAR(var0) && e.get_var(var0)==1)
                IMREF(x_left,p) = IMREF(x_right, p+IMREF(x_left,p)) = OCCLUDED;
        }
    for (p.y=0; p.y<im_size.y; p.y++)
        for (p.x=0; p.x<im_size.x; p.x++) {
            Energy::Var varA = (Energy::Var) IMREF(varsA, p);
            if(IS_VAR(varA) && e.get_var(varA)==1) // New disparity
                IMREF(x_right, p+a) = -(IMREF(x_left, p) = a);
        }
}

/// Compute the minimum a-expansion configuration
void Match::ExpansionMove(int a)
{ 
    Energy e;

    // Build graph
    Coord p;
    for(p.y=0; p.y<im_size.y; p.y++)
        for(p.x=0; p.x<im_size.x; p.x++)
            build_nodes(e, p, a);

    for(p.y=0; p.y<im_size.y; p.y++)
        for(p.x=0; p.x<im_size.x; p.x++) {
            for(unsigned int k=0; k<NEIGHBOR_NUM; k++) {
                Coord np = p+NEIGHBORS[k];
                if(np>=Coord(0,0) && np<im_size)
                    build_smoothness(e, p, np, a);
            }
            build_uniqueness(e, p, a);
        }

    int oldE=E;
    E = e.minimize(); // Max-flow, give the lowest-energy expansion move

    if(E<oldE) // lower energy, accept the expansion move
        update_disparity(e, a);
}
