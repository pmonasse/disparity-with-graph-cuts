/**
 * @file match.cpp
 * @brief Disparity estimation by Kolomogorov-Zabih algorithm
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
#include "nan.h"
#include <limits>
#include <iostream>

const int Match::OCCLUDED = std::numeric_limits<int>::max();

/// Constructor
Match::Match(GeneralImage left, GeneralImage right, bool color)
{
    imSizeL = Coord(imGetXSize(left),imGetYSize(left));
    imSizeR = Coord(imGetXSize(right),imGetYSize(right));

    if (!color) {
        imColorLeft = imColorRight = 0;
        imColorLeftMin = imColorRightMin = 0;
        imColorLeftMax = imColorRightMax = 0;

        imLeft  = (GrayImage)left; imRight = (GrayImage)right;
        imLeftMin = imLeftMax = imRightMin = imRightMax = 0;
    } else {
        imLeft = imRight = 0;
        imLeftMin = imRightMin = 0;
        imLeftMax = imRightMax = 0;

        imColorLeft = (RGBImage)left;
        imColorRight = (RGBImage)right;

        imColorLeftMin = imColorLeftMax = 0;
        imColorRightMin = imColorRightMax = 0;
    }

    dispMin = dispMax = 0;

    d_left  = (IntImage)imNew(IMAGE_INT, imSizeL);
    d_right = (IntImage)imNew(IMAGE_INT, imSizeR);

    vars0 = (IntImage)imNew(IMAGE_INT, imSizeL);
    varsA = (IntImage)imNew(IMAGE_INT, imSizeL);
    if (!d_left || !d_right || !vars0 || !varsA)
        { std::cerr << "Not enough memory!" << std::endl; exit(1); }
}

/// Destructor
Match::~Match()
{
    imFree(imLeftMin);
    imFree(imLeftMax);
    imFree(imRightMin);
    imFree(imRightMax);
    imFree(imColorLeftMin);
    imFree(imColorLeftMax);
    imFree(imColorRightMin);
    imFree(imColorRightMax);

    imFree(d_left);
    imFree(d_right);

    imFree(vars0);
    imFree(varsA);
}

/// Save disparity map as float TIFF image
void Match::SaveXLeft(const char *fileName)
{
    FloatImage out = (FloatImage)imNew(IMAGE_FLOAT, imSizeL);

    RectIterator end=rectEnd(imSizeL);
    for(RectIterator p=rectBegin(imSizeL); p!=end; ++p) {
        int d=IMREF(d_left,*p);
        IMREF(out,*p) = (d==OCCLUDED? NaN: static_cast<float>(d));
    }

    imSave(out, fileName);
    imFree(out);
}

/// Save scaled disparity map as 8-bit color image (gray between 64 and 255).
/// flag: lowest disparity should appear darkest (true) or brightest (false).
void Match::SaveScaledXLeft(const char *fileName, bool flag)
{
    RGBImage im = (RGBImage)imNew(IMAGE_RGB, imSizeL);
    const int dispSize = dispMax-dispMin+1;

    RectIterator end=rectEnd(imSizeL);
    for(RectIterator p=rectBegin(imSizeL); p!=end; ++p) {
        int d = IMREF(d_left,*p), c;
        if (d==OCCLUDED) {
            IMREF(im,*p).r=0; IMREF(im,*p).g=IMREF(im,*p).b=255;
        } else {
            if (dispSize == 0) c = 255;
            else if (flag) c = 255 - (255-64)*(dispMax - d)/dispSize;
            else           c = 255 - (255-64)*(d - dispMin)/dispSize;
            IMREF(im,*p).r=IMREF(im,*p).g=IMREF(im,*p).b = c;
        }
    }

    imSave(im, fileName);
    imFree(im);
}

/// Specify disparity range
void Match::SetDispRange(int dMin, int dMax)
{
    dispMin = dMin;
    dispMax = dMax;
    if (! (dispMin<=dispMax) ) {
        std::cerr << "Error: wrong disparity range!\n" << std::endl;
        exit(1);
    }
    RectIterator end=rectEnd(imSizeL);
    for(RectIterator p=rectBegin(imSizeL); p!=end; ++p)
        IMREF(d_left, *p) = OCCLUDED;
    end = rectEnd(imSizeR);
    for(RectIterator q=rectBegin(imSizeR); q!=end; ++q)
        IMREF(d_right,*q) = OCCLUDED;
}
