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
        im_color_left = im_color_right = 0;
        im_color_left_min = im_color_right_min = 0;
        im_color_left_max = im_color_right_max = 0;

        im_left  = (GrayImage)left; im_right = (GrayImage)right;
        im_left_min = im_left_max = im_right_min = im_right_max = 0;
    } else {
        im_left = im_right = 0;
        im_left_min = im_right_min = 0;
        im_left_max = im_right_max = 0;

        im_color_left = (RGBImage)left;
        im_color_right = (RGBImage)right;

        im_color_left_min = im_color_left_max = 0;
        im_color_right_min = im_color_right_max = 0;
    }

    dispMin = dispMax = 0;

    x_left  = (IntImage)imNew(IMAGE_INT, imSizeL);
    x_right = (IntImage)imNew(IMAGE_INT, imSizeR);

    vars0 = (IntImage)imNew(IMAGE_INT, imSizeL);
    varsA = (IntImage)imNew(IMAGE_INT, imSizeL);
    if (!x_left || !x_right || !vars0 || !varsA)
        { std::cerr << "Not enough memory!" << std::endl; exit(1); }
}

/// Destructor
Match::~Match()
{
    imFree(im_left_min);
    imFree(im_left_max);
    imFree(im_right_min);
    imFree(im_right_max);
    imFree(im_color_left_min);
    imFree(im_color_left_max);
    imFree(im_color_right_min);
    imFree(im_color_right_max);

    imFree(x_left);
    imFree(x_right);

    imFree(vars0);
    imFree(varsA);
}

/// Save disparity map as float TIFF image
void Match::SaveXLeft(const char *file_name)
{
    FloatImage out = (FloatImage)imNew(IMAGE_FLOAT, imSizeL);

    RectIterator end=rectEnd(imSizeL);
    for(RectIterator p=rectBegin(imSizeL); p!=end; ++p) {
        int d=IMREF(x_left,*p);
        IMREF(out,*p) = (d==OCCLUDED? NaN: static_cast<float>(d));
    }

    imSave(out, file_name);
    imFree(out);
}

/// Save scaled disparity map as 8-bit color image (gray between 64 and 255).
/// flag: lowest disparity should appear darkest (true) or brightest (false).
void Match::SaveScaledXLeft(const char *file_name, bool flag)
{
    RGBImage im = (RGBImage)imNew(IMAGE_RGB, imSizeL);
    const int dispSize = dispMax-dispMin+1;

    RectIterator end=rectEnd(imSizeL);
    for(RectIterator p=rectBegin(imSizeL); p!=end; ++p) {
        int d = IMREF(x_left,*p), c;
        if (d==OCCLUDED) {
            IMREF(im,*p).r=0; IMREF(im,*p).g=IMREF(im,*p).b=255;
        } else {
            if (dispSize == 0) c = 255;
            else if (flag) c = 255 - (255-64)*(dispMax - d)/dispSize;
            else           c = 255 - (255-64)*(d - dispMin)/dispSize;
            IMREF(im,*p).r=IMREF(im,*p).g=IMREF(im,*p).b = c;
        }
    }

    imSave(im, file_name);
    imFree(im);
}

/// Specify disparity range
void Match::SetDispRange(int disp_min, int disp_max)
{
    dispMin = disp_min;
    dispMax = disp_max;
    if (! (dispMin<=dispMax) ) {
        std::cerr << "Error: wrong disparity range!\n" << std::endl;
        exit(1);
    }
    RectIterator end=rectEnd(imSizeL);
    for(RectIterator p=rectBegin(imSizeL); p!=end; ++p)
        IMREF(x_left, *p) = OCCLUDED;
    end = rectEnd(imSizeR);
    for(RectIterator p=rectBegin(imSizeR); p!=end; ++p)
        IMREF(x_right,*p) = OCCLUDED;
}
