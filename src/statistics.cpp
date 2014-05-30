/**
 * @file statistics.cpp
 * @brief Automatic computation of K in Kolmogorov-Zabih algorithm
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

#include <algorithm>
#include <iostream>
#include "match.h"

/// Heuristic for selecting parameter 'K'
/// Details are described in Kolmogorov's thesis
float Match::GetK()
{
    int i = dispMax-dispMin+1;
    int k = (i+2)/4; // around 0.25 times the number of disparities
    if(k<3) k=3;

    int* array = new int[k];
    std::fill_n(array, k, 0);
    int sum=0, num=0;

    int xmin = std::max(0,-dispMin); // 0<=x,x+dispMin
    int xmax = std::min(imSizeL.x,imSizeR.x-dispMax); // x<wl,x+dispMax<wr
    Coord p;
    for(p.y=0; p.y<imSizeL.y && p.y<imSizeR.y; p.y++)
    for(p.x=xmin; p.x<xmax; p.x++) {
        // compute k'th smallest value among data_penalty(p, p+d) for all d
        for(int i=0, d=dispMin; d<=dispMax; d++) {
            int delta = (imLeft?
                         data_penalty_gray (p,p+d):
                         data_penalty_color(p,p+d));
            if(i<k) array[i++] = delta;
            else for(i=0; i<k; i++)
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
    std::cout <<"Computing statistics: K(data_penalty noise) ="<< K <<std::endl;
    return K;
}
