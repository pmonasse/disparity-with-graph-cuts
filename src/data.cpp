/**
 * @file data.cpp
 * @brief Data cost for pixel correspondence
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

/*
Functions depending on input images:

data_penalty_X(Coord p, Coord q)
smoothness_penalty_X(Coord p1, Coord p2, Coord disp)

where X describes the appropriate case (gray/color)
*/

#include "match.h"
#include <algorithm>

/************************************************************/
/********************* data penalty *************************/
/************************************************************/
// The data term is computed as described in
//   Stan Birchfield and Carlo Tomasi
//   "A pixel dissimilarity measure that is insensitive to image sampling"
//   IEEE Trans. on PAMI 20(4):401-406, April 98
// with one distinction: intensity intervals for a pixels
// are computed from 4 neighbors rather than 2.

/// Upper bound for intensity level difference when computing data cost
static int CUTOFF=30;

/// Distance from v to interval [min,max]
inline int dist_interval(int v, int min, int max) {
    if(v<min) return (min-v);
    if(v>max) return (v-max);
    return 0;
}

/// Birchfield-Tomasi gray distance between pixels p and q
int Match::data_penalty_gray(Coord p, Coord q) const {
    int Ip    = IMREF(imLeft,    p), Iq    = IMREF(imRight,    q);
    int IpMin = IMREF(imLeftMin, p), IqMin = IMREF(imRightMin, q);
    int IpMax = IMREF(imLeftMax, p), IqMax = IMREF(imRightMax, q);

    int dp = dist_interval(Ip, IqMin, IqMax);
    int dq = dist_interval(Iq, IpMin, IpMax);
    int d = std::min(dp, dq);
    if(d>CUTOFF) d = CUTOFF;
    if(params.dataCost==Parameters::L2) d = d*d;

    return d;
}

/// Birchfield-Tomasi color distance between pixels p and q
int Match::data_penalty_color(Coord p, Coord q) const {
    int dSum=0;
    // Loop over the 3 channels
    for(int i=0; i<3; i++) {
        int Ip    = IMREF(imColorLeft,     p).c[i];
        int Iq    = IMREF(imColorRight,    q).c[i];
        int IpMin = IMREF(imColorLeftMin,  p).c[i];
        int IqMin = IMREF(imColorRightMin, q).c[i];
        int IpMax = IMREF(imColorLeftMax,  p).c[i];
        int IqMax = IMREF(imColorRightMax, q).c[i];

        int dp = dist_interval(Ip, IqMin, IqMax);
        int dq = dist_interval(Iq, IpMin, IpMax);
        int d = std::min(dp, dq);
        if(d>CUTOFF) d = CUTOFF;
        if(params.dataCost==Parameters::L2) d = d*d;
        dSum += d;
    }
    return dSum/3;
}

/************************************************************/
/******************* Preprocessing for Birchfield-Tomasi ****/
/************************************************************/

/// Fill ImMin and ImMax from Im (gray version)
static void SubPixel(GrayImage Im, GrayImage ImMin, GrayImage ImMax) {
    Coord p;
    int I, I1, I2, I3, I4, IMin, IMax;
    int xmax=imGetXSize(ImMin), ymax=imGetYSize(ImMin);

    for (p.y=0; p.y<ymax; p.y++)
    for (p.x=0; p.x<xmax; p.x++) {
        I = IMin = IMax = imRef(Im, p.x, p.y);
        I1 = (p.x>0?      (imRef(Im, p.x-1, p.y) + I) / 2: I);
        I2 = (p.x+1<xmax? (imRef(Im, p.x+1, p.y) + I) / 2: I);
        I3 = (p.y>0?      (imRef(Im, p.x, p.y-1) + I) / 2: I);
        I4 = (p.y+1<ymax? (imRef(Im, p.x, p.y+1) + I) / 2: I);

        if (IMin > I1) IMin = I1;
        if (IMin > I2) IMin = I2;
        if (IMin > I3) IMin = I3;
        if (IMin > I4) IMin = I4;
        if (IMax < I1) IMax = I1;
        if (IMax < I2) IMax = I2;
        if (IMax < I3) IMax = I3;
        if (IMax < I4) IMax = I4;

        imRef(ImMin, p.x, p.y) = IMin;
        imRef(ImMax, p.x, p.y) = IMax;
    }
}

/// Fill ImMin and ImMax from Im (color version)
static void SubPixelColor(RGBImage Im, RGBImage ImMin, RGBImage ImMax) {
    int I, I1, I2, I3, I4, IMin, IMax;

    Coord p;
    int xmax=imGetXSize(ImMin), ymax=imGetYSize(ImMin);
    for(p.y=0; p.y<ymax; p.y++)
    for(p.x=0; p.x<xmax; p.x++)
        for(int i=0; i<3; i++) { // Loop over channels
            I = IMin = IMax = imRef(Im, p.x, p.y).c[i];
            I1 = (p.x>0?      (imRef(Im, p.x-1, p.y).c[i] + I) / 2: I);
            I2 = (p.x+1<xmax? (imRef(Im, p.x+1, p.y).c[i] + I) / 2: I);
            I3 = (p.y>0?      (imRef(Im, p.x, p.y-1).c[i] + I) / 2: I);
            I4 = (p.y+1<ymax? (imRef(Im, p.x, p.y+1).c[i] + I) / 2: I);

            if (IMin > I1) IMin = I1;
            if (IMin > I2) IMin = I2;
            if (IMin > I3) IMin = I3;
            if (IMin > I4) IMin = I4;
            if (IMax < I1) IMax = I1;
            if (IMax < I2) IMax = I2;
            if (IMax < I3) IMax = I3;
            if (IMax < I4) IMax = I4;

            imRef(ImMin, p.x, p.y).c[i] = IMin;
            imRef(ImMax, p.x, p.y).c[i] = IMax;
        }
}

/// Preprocessing for faster Birchfield-Tomasi distance computation.
void Match::InitSubPixel() {
    if(imLeft && !imLeftMin) {
        imLeftMin  = (GrayImage) imNew(IMAGE_GRAY, imSizeL);
        imLeftMax  = (GrayImage) imNew(IMAGE_GRAY, imSizeL);
        imRightMin = (GrayImage) imNew(IMAGE_GRAY, imSizeR);
        imRightMax = (GrayImage) imNew(IMAGE_GRAY, imSizeR);

        SubPixel(imLeft,  imLeftMin,  imLeftMax);
        SubPixel(imRight, imRightMin, imRightMax);
    }
    if(imColorLeft && !imColorLeftMin) {
        imColorLeftMin  = (RGBImage) imNew(IMAGE_RGB, imSizeL);
        imColorLeftMax  = (RGBImage) imNew(IMAGE_RGB, imSizeL);
        imColorRightMin = (RGBImage) imNew(IMAGE_RGB, imSizeR);
        imColorRightMax = (RGBImage) imNew(IMAGE_RGB, imSizeR);

        SubPixelColor(imColorLeft,  imColorLeftMin,  imColorLeftMax);
        SubPixelColor(imColorRight, imColorRightMin, imColorRightMax);
    }
}

/************************************************************/
/****************** smoothness penalty **********************/
/************************************************************/

/// Smoothness penalty between assignments (p1,p1+disp) and (p2,p2+disp).
int Match::smoothness_penalty_gray(Coord p1, Coord p2, int disp) const {
    // |I1(p1)-I1(p2)| and |I2(p1+disp)-I2(p2+disp)|
    int dl = IMREF(imLeft,  p1     ) - IMREF(imLeft,  p2     );
    int dr = IMREF(imRight, p1+disp) - IMREF(imRight, p2+disp);
    if (dl<0) dl = -dl; if (dr<0) dr = -dr;
    return (dl<params.edgeThresh && dr<params.edgeThresh)?
        params.lambda1: params.lambda2;
}

/// Smoothness penalty between assignments (p1,p1+disp) and (p2,p2+disp).
int Match::smoothness_penalty_color(Coord p1, Coord p2, int disp) const {
    int d, dMax=0; // Max inf norm in RGB space of (p1,p2) and (p1+disp,p2+disp)
    for(int i=0; i<3; i++) {
        d = IMREF(imColorLeft,  p1     ).c[i]-IMREF(imColorLeft,  p2     ).c[i];
        if(d<0) d = -d; if (dMax<d) dMax = d;
        d = IMREF(imColorRight, p1+disp).c[i]-IMREF(imColorRight, p2+disp).c[i];
        if(d<0) d = -d; if (dMax<d) dMax = d;
    }
    return (dMax<params.edgeThresh)? params.lambda1: params.lambda2;
}

/// Set parameters for algorithm
void Match::SetParameters(Parameters *_params) {
    params = *_params;
    InitSubPixel();
}
