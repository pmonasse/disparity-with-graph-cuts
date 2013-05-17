/* image.h */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001. */

/*
Routines for loading and saving standard gray (.pgm)
and color (.ppm) images. Also it supports unofficial
formats: short, long, ptr, float, double.

Tested under windows, Visual C++ 6.0 compiler
and unix (SunOS 5.8 and RedHat Linux 7.0, gcc and c++ compilers).

Example usage - converting color image in.ppm to gray image out.pgm:
///////////////////////////////////////////////////
#include "image.h"
...

RGBImage rgb = (RGBImage) imLoad(IMAGE_RGB, "in.ppm");
if (!rgb) { fprintf(stderr, "Can't open in.ppm\n"); exit(1); }
int x, y, xsize = imGetXSize(rgb), ysize = imGetYSize(rgb);

GrayImage gray = (GrayImage) imNew(IMAGE_GRAY, xsize, ysize);
if (!gray) { fprintf(stderr, "Not enough memory\n"); exit(1); }

for (y=0; y<ysize; y++)
for (x=0; x<xsize; x++)
{
    int r = imRef(rgb, x, y).r;
    int g = imRef(rgb, x, y).g;
    int b = imRef(rgb, x, y).b;
    imRef(gray, x, y) = (r + g + b) / 3;
}

if (imSave(gray, "out.pgm") != 0) { fprintf(stderr, "Can't save out.ppm\n"); exit(1); }

imFree(rgb);
imFree(gray);
///////////////////////////////////////////////////
*/

#ifndef __IMAGE_H__
#define __IMAGE_H__

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

typedef struct GrayImage_t  {unsigned char                 *data;} *GrayImage;
typedef struct RGBImage_t   {struct {unsigned char r,g,b;} *data;} *RGBImage;
typedef struct IntImage_t   {int                           *data;} *IntImage;
typedef struct FloatImage_t {float                         *data;} *FloatImage;

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

#endif
