/**
 * @file image.h
 * @brief Data structure for images and input/output
 * @author Vladimir Kolmogorov <vnk@cs.cornell.edu>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 * 
 * Copyright (c) 2001, 2012-2014, Vladimir Kolmogorov, Pascal Monasse
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
Routines for loading and saving standard gray (.pgm)
and color (.ppm) images. Also it supports unofficial
formats: float.

Example usage - converting color image in.ppm to gray image out.pgm:
///////////////////////////////////////////////////
#include "image.h"
...

RGBImage rgb = (RGBImage) imLoad(IMAGE_RGB, "in.ppm");
if(!rgb) { fprintf(stderr, "Can't open in.ppm\n"); exit(1); }
int x, y, xsize = imGetXSize(rgb), ysize = imGetYSize(rgb);

GrayImage gray = (GrayImage) imNew(IMAGE_GRAY, xsize, ysize);
if(!gray) { fprintf(stderr, "Not enough memory\n"); exit(1); }

for(y=0; y<ysize; y++)
for(x=0; x<xsize; x++) {
    int r = imRef(rgb, x, y).r;
    int g = imRef(rgb, x, y).g;
    int b = imRef(rgb, x, y).b;
    imRef(gray, x, y) = (r + g + b) / 3;
}

if(imSave(gray, "out.pgm") != 0) { fprintf(stderr, "Can't save out.pgm\n"); }

imFree(rgb);
imFree(gray);
///////////////////////////////////////////////////
*/

#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>

typedef enum
{
    IMAGE_GRAY,
    IMAGE_RGB,
    IMAGE_INT,
    IMAGE_FLOAT
} ImageType;

typedef struct ImageHeader_st
{
    ImageType type;
    int data_size;
    int xsize, ysize;
} ImageHeader;

typedef struct GeneralImage_t {void*data;} *GeneralImage;

typedef struct GrayImage_t  {unsigned char                *data;} *GrayImage;
typedef struct RGBImage_t   {struct {unsigned char c[3];} *data;} *RGBImage;
typedef struct IntImage_t   {int                          *data;} *IntImage;
typedef struct FloatImage_t {float                        *data;} *FloatImage;

#define imHeader(im) ((ImageHeader*) ( ((char*)(im)) - sizeof(ImageHeader) ))

#define imRef(im, x, y) ( ((im)+(y))->data[x] )
#define imGetXSize(im) (imHeader(im)->xsize)
#define imGetYSize(im) (imHeader(im)->ysize)

void * imNew(ImageType type, int xsize, int ysize);
inline void imFree(void *im) {
    if(im) { free(GeneralImage(im)->data); free(imHeader(im)); }
}
void * imLoad(ImageType type, const char *filename);
int imSave(void *im, const char *filename);

/// Pixel coordinates with basic operations.
struct Coord
{
    int x, y;

    Coord() {}
    Coord(int a, int b) { x = a; y = b; }

    Coord operator+ (Coord a) const { return Coord(x + a.x, y + a.y); }
    Coord operator+ (int a)   const { return Coord(x+a,y); }
    Coord operator- (int a)   const { return Coord(x-a,y); }
    bool  operator< (Coord a) const { return (x <  a.x) && (y <  a.y); }
    bool  operator<=(Coord a) const { return (x <= a.x) && (y <= a.y); }
    bool  operator!=(Coord a) const { return (x != a.x) || (y != a.y); }
};
/// Value of image im at pixel p
#define IMREF(im, p) (imRef((im), (p).x, (p).y))

/// Overload with parameter of type Coord
inline void * imNew(ImageType type, Coord size) {
    return imNew(type, size.x, size.y);
}

/// Is p inside rectangle r?
inline bool inRect(Coord p, Coord r) {
    return (Coord(0,0)<=p && p<r);
}

/// Rectangle iterator
class RectIterator {
    Coord p; ///< Current point
    int w; ///< Width of rectangle
public:
    RectIterator(Coord rect): p(0,0), w(rect.x) {}
    const Coord& operator*() const { return p; }
    bool operator!=(const RectIterator& it) const { return (p!=it.p); }
    RectIterator& operator++() { if(++p.x==w){p.x=0;++p.y;} return *this; }

    friend RectIterator rectBegin(Coord rect);
    friend RectIterator rectEnd(Coord rect);
};

inline RectIterator rectBegin(Coord rect) { return RectIterator(rect); }
inline RectIterator rectEnd(Coord rect) {
    RectIterator it(rect);
    it.p.y = rect.y;
    return it;
}

#endif
