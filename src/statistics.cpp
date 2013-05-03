/* statistics.cpp */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001-2003. */

#include <algorithm>
#include <iostream>
#include "match.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/// Heuristic for selecting parameter 'K'
/// Details are described in Kolmogorov's thesis
float Match::GetK()
{
    int i = dispMax-dispMin+1;
    int k = (i+2)/4; // around 0.25 times the number of disparities
    if(k<3) k=3;
    if(k>i) k=i;

    int* array = new int[k];
    int sum=0, num=0;

    int xmin = 0-std::min(dispMin, 0), xmax=im_size.x-std::max(dispMax,0);
    Coord p;
    for(p.y=0; p.y<im_size.y; p.y++)
    for(p.x=xmin; p.x<xmax; p.x++) {
        // compute k'th smallest value among data_penalty(p, p+d) for all d
        for(int i=0, d=dispMin; d<=dispMax; d++) {
            int delta = (im_left?
                         data_penalty_gray (p,p+d):
                         data_penalty_color(p,p+d));
            if(i<k) array[i++] = delta;
            else for (i=0; i<k; i++)
                     if(delta<array[i])
                         std::swap(delta, array[i]);
        }
        sum += *std::max_element(array, array+k);
        num ++;
    }

    delete [] array;
    if(num==0) { std::cerr<<"GetK: Not enough samples!"<<std::endl; exit(1); }
    if(sum==0) { std::cerr<<"GetK failed: K is 0!"<<std::endl; exit(1); }

    float K = ((float)sum)/num;
    std::cout << "Computing statistics: data_penalty noise K=" << K <<std::endl;
    return K;
}
