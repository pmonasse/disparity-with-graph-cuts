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

data_penalty_X(Coord l, Coord r)
smoothness_penalty_X(Coord p, Coord np, Coord disp)

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

int Match::data_penalty_gray(Coord l, Coord r) const
{
    int Il, Il_min, Il_max, Ir, Ir_min, Ir_max;
    Il     = IMREF(im_left,     l); Ir     = IMREF(im_right,     r);
    Il_min = IMREF(im_left_min, l); Ir_min = IMREF(im_right_min, r);
    Il_max = IMREF(im_left_max, l); Ir_max = IMREF(im_right_max, r);

    int dl = dist_interval(Il, Ir_min, Ir_max);
    int dr = dist_interval(Ir, Il_min, Il_max);
    int d = std::min(dl, dr);
    if(d>CUTOFF) d = CUTOFF;
    if(params.dataCost==Parameters::L2) d = d*d;

    return d;
}

int Match::data_penalty_color(Coord l, Coord r) const
{
    int dl, dr, d, d_sum = 0;
    int Il, Il_min, Il_max, Ir, Ir_min, Ir_max;

    // red component
    Il     = IMREF(im_color_left,      l).r;
    Ir     = IMREF(im_color_right,     r).r;
    Il_min = IMREF(im_color_left_min,  l).r;
    Ir_min = IMREF(im_color_right_min, r).r;
    Il_max = IMREF(im_color_left_max,  l).r;
    Ir_max = IMREF(im_color_right_max, r).r;

    dl = dist_interval(Il, Ir_min, Ir_max);
    dr = dist_interval(Ir, Il_min, Il_max);
    d = std::min(dl, dr);
    if (d>CUTOFF) d = CUTOFF;
    if (params.dataCost==Parameters::L2) d = d*d;
    d_sum += d;

    // green component
    Il     = IMREF(im_color_left,      l).g;
    Ir     = IMREF(im_color_right,     r).g;
    Il_min = IMREF(im_color_left_min,  l).g;
    Ir_min = IMREF(im_color_right_min, r).g;
    Il_max = IMREF(im_color_left_max,  l).g;
    Ir_max = IMREF(im_color_right_max, r).g;

    dl = dist_interval(Il, Ir_min, Ir_max);
    dr = dist_interval(Ir, Il_min, Il_max);
    d = std::min(dl, dr);
    if (d>CUTOFF) d = CUTOFF;
    if (params.dataCost==Parameters::L2) d = d*d;
    d_sum += d;

    // blue component
    Il     = IMREF(im_color_left,      l).b;
    Ir     = IMREF(im_color_right,     r).b;
    Il_min = IMREF(im_color_left_min,  l).b;
    Ir_min = IMREF(im_color_right_min, r).b;
    Il_max = IMREF(im_color_left_max,  l).b;
    Ir_max = IMREF(im_color_right_max, r).b;

    dl = dist_interval(Il, Ir_min, Ir_max);
    dr = dist_interval(Ir, Il_min, Il_max);
    d = std::min(dl, dr);
    if (d>CUTOFF) d = CUTOFF;
    if (params.dataCost==Parameters::L2) d = d*d;
    d_sum += d;

    return d_sum/3;
}

/************************************************************/
/******************* sub_pixel preprocessing ****************/
/************************************************************/

static void SubPixel(GrayImage Im, GrayImage ImMin, GrayImage ImMax)
{
    Coord p;
    int I, I1, I2, I3, I4, I_min, I_max;
    int xmax=imGetXSize(Im), ymax=imGetYSize(Im);

    for (p.y=0; p.y<ymax; p.y++)
    for (p.x=0; p.x<xmax; p.x++)
    {
        I = I_min = I_max = imRef(Im, p.x, p.y);
        I1 = (p.x>0?      (imRef(Im, p.x-1, p.y) + I) / 2: I);
        I2 = (p.x+1<xmax? (imRef(Im, p.x+1, p.y) + I) / 2: I);
        I3 = (p.y>0?      (imRef(Im, p.x, p.y-1) + I) / 2: I);
        I4 = (p.y+1<ymax? (imRef(Im, p.x, p.y+1) + I) / 2: I);

        if (I_min > I1) I_min = I1;
        if (I_min > I2) I_min = I2;
        if (I_min > I3) I_min = I3;
        if (I_min > I4) I_min = I4;
        if (I_max < I1) I_max = I1;
        if (I_max < I2) I_max = I2;
        if (I_max < I3) I_max = I3;
        if (I_max < I4) I_max = I4;

        imRef(ImMin, p.x, p.y) = I_min;
        imRef(ImMax, p.x, p.y) = I_max;
    }
}

static void SubPixelColor(RGBImage Im, RGBImage ImMin, RGBImage ImMax)
{
    Coord p;
    int I, I1, I2, I3, I4, I_min, I_max;
    int xmax=imGetXSize(Im), ymax=imGetYSize(Im);

    for (p.y=0; p.y<ymax; p.y++)
    for (p.x=0; p.x<xmax; p.x++)
    {
        // red component
        I = I_min = I_max = imRef(Im, p.x, p.y).r;
        I1 = (p.x>0?      (imRef(Im, p.x-1, p.y).r + I) / 2: I);
        I2 = (p.x+1<xmax? (imRef(Im, p.x+1, p.y).r + I) / 2: I);
        I3 = (p.y>0?      (imRef(Im, p.x, p.y-1).r + I) / 2: I);
        I4 = (p.y+1<ymax? (imRef(Im, p.x, p.y+1).r + I) / 2: I);

        if (I_min > I1) I_min = I1;
        if (I_min > I2) I_min = I2;
        if (I_min > I3) I_min = I3;
        if (I_min > I4) I_min = I4;
        if (I_max < I1) I_max = I1;
        if (I_max < I2) I_max = I2;
        if (I_max < I3) I_max = I3;
        if (I_max < I4) I_max = I4;

        imRef(ImMin, p.x, p.y).r = I_min;
        imRef(ImMax, p.x, p.y).r = I_max;

        // green component
        I = I_min = I_max = imRef(Im, p.x, p.y).g;
        I1 = (p.x>0?      (imRef(Im, p.x-1, p.y).g + I) / 2: I);
        I2 = (p.x+1<xmax? (imRef(Im, p.x+1, p.y).g + I) / 2: I);
        I3 = (p.y>0?      (imRef(Im, p.x, p.y-1).g + I) / 2: I);
        I4 = (p.y+1<ymax? (imRef(Im, p.x, p.y+1).g + I) / 2: I);

        if (I_min > I1) I_min = I1;
        if (I_min > I2) I_min = I2;
        if (I_min > I3) I_min = I3;
        if (I_min > I4) I_min = I4;
        if (I_max < I1) I_max = I1;
        if (I_max < I2) I_max = I2;
        if (I_max < I3) I_max = I3;
        if (I_max < I4) I_max = I4;

        imRef(ImMin, p.x, p.y).g = I_min;
        imRef(ImMax, p.x, p.y).g = I_max;

        // blue component
        I = I_min = I_max = imRef(Im, p.x, p.y).b;
        I1 = (p.x>0?      (imRef(Im, p.x-1, p.y).b + I) / 2: I);
        I2 = (p.x+1<xmax? (imRef(Im, p.x+1, p.y).b + I) / 2: I);
        I3 = (p.y>0?      (imRef(Im, p.x, p.y-1).b + I) / 2: I);
        I4 = (p.y+1<ymax? (imRef(Im, p.x, p.y+1).b + I) / 2: I);

        if (I_min > I1) I_min = I1;
        if (I_min > I2) I_min = I2;
        if (I_min > I3) I_min = I3;
        if (I_min > I4) I_min = I4;
        if (I_max < I1) I_max = I1;
        if (I_max < I2) I_max = I2;
        if (I_max < I3) I_max = I3;
        if (I_max < I4) I_max = I4;

        imRef(ImMin, p.x, p.y).b = I_min;
        imRef(ImMax, p.x, p.y).b = I_max;
    }
}

void Match::InitSubPixel()
{
    if(im_left && !im_left_min)
    {
        im_left_min  = (GrayImage) imNew(IMAGE_GRAY, imSizeL);
        im_left_max  = (GrayImage) imNew(IMAGE_GRAY, imSizeL);
        im_right_min = (GrayImage) imNew(IMAGE_GRAY, imSizeR);
        im_right_max = (GrayImage) imNew(IMAGE_GRAY, imSizeR);

        SubPixel(im_left,  im_left_min,  im_left_max);
        SubPixel(im_right, im_right_min, im_right_max);
    }
    if(im_color_left && !im_color_left_min)
    {
        im_color_left_min  = (RGBImage) imNew(IMAGE_RGB, imSizeL);
        im_color_left_max  = (RGBImage) imNew(IMAGE_RGB, imSizeL);
        im_color_right_min = (RGBImage) imNew(IMAGE_RGB, imSizeR);
        im_color_right_max = (RGBImage) imNew(IMAGE_RGB, imSizeR);

        SubPixelColor(im_color_left,  im_color_left_min,  im_color_left_max);
        SubPixelColor(im_color_right, im_color_right_min, im_color_right_max);
    }
}

/************************************************************/
/****************** smoothness penalty **********************/
/******************** (static clues) ************************/
/************************************************************/

int Match::smoothness_penalty_gray(Coord p, Coord np, int disp) const
{
    int dl = IMREF(im_left, p) - IMREF(im_left, np);
    int dr = IMREF(im_right, p+disp) - IMREF(im_right, np+disp);

    if (dl<0) dl = -dl; if (dr<0) dr = -dr;

    return (dl<params.edgeThresh && dr<params.edgeThresh)?
        params.lambda1: params.lambda2;
}

int Match::smoothness_penalty_color(Coord p, Coord np, int disp) const
{
    int d, d_max=0;

    // red component
    d = IMREF(im_color_left, p).r - IMREF(im_color_left, np).r;
    if (d<0) d = -d; if (d_max<d) d_max = d;
    d = IMREF(im_color_right, p+disp).r - IMREF(im_color_right, np+disp).r;
    if (d<0) d = -d; if (d_max<d) d_max = d;

    // green component
    d = IMREF(im_color_left, p).g - IMREF(im_color_left, np).g;
    if (d<0) d = -d; if (d_max<d) d_max = d;
    d = IMREF(im_color_right, p+disp).g - IMREF(im_color_right, np+disp).g;
    if (d<0) d = -d; if (d_max<d) d_max = d;

    // blue component
    d = IMREF(im_color_left, p).b - IMREF(im_color_left, np).b;
    if (d<0) d = -d; if (d_max<d) d_max = d;
    d = IMREF(im_color_right, p+disp).b - IMREF(im_color_right, np+disp).b;
    if (d<0) d = -d; if (d_max<d) d_max = d;

    return (d_max<params.edgeThresh)? params.lambda1: params.lambda2;
}

void Match::SetParameters(Parameters *_params)
{
    params = *_params;
    InitSubPixel();
}
