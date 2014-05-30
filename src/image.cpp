/**
 * @file image.cpp
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

#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include "image.h"
#ifdef HAS_PNG
#include "io_png.h"
#endif
#ifdef HAS_TIFF
#include "io_tiff.h"
#endif

static const int ONE = 1;
static const int SWAP_BYTES = (((char *)(&ONE))[0] == 0) ? 1 : 0;

void* imNew(ImageType type, int xsize, int ysize)
{
    void *ptr;
    GeneralImage im;
    int data_size;
    int y;

    if (xsize<=0 || ysize<=0) return NULL;

    switch (type) {
    case IMAGE_GRAY:   data_size = sizeof(unsigned char);    break;
    case IMAGE_RGB:    data_size = sizeof(unsigned char[3]); break;
    case IMAGE_INT:    data_size = sizeof(int);              break;
    case IMAGE_FLOAT:  data_size = sizeof(float);            break;
    default: return NULL;
    }

    ptr = malloc(sizeof(ImageHeader) + ysize*sizeof(void*));
    if (!ptr) return NULL;
    im = (GeneralImage) ((char*)ptr + sizeof(ImageHeader));

    imHeader(im)->type      = type;
    imHeader(im)->data_size = data_size;
    imHeader(im)->xsize     = xsize;
    imHeader(im)->ysize     = ysize;

    im->data = (void *) malloc(xsize*ysize*data_size);
    for (y=1; y<ysize; y++)
        (im+y)->data = ((char*)(im->data)) + xsize*y*data_size;

    return im;
}

void SwapBytes(GeneralImage im)
{
    if (SWAP_BYTES) {
        ImageType type = imHeader(im)->type;

        if (type==IMAGE_INT ||
            type==IMAGE_FLOAT) {
            char *ptr, c;
            int i, k;

            int size = (imHeader(im)->xsize) * (imHeader(im)->ysize);
            int data_size = imHeader(im)->data_size;

            ptr = (char *) (im->data);
            for (i=0; i<size; i++) {
                for (k=0; k<data_size/2; k++) {
                    c = ptr[k];
                    ptr[k] = ptr[data_size-k-1];
                    ptr[data_size-k-1] = c;
                }
                ptr += data_size;
            }
        }
    }
}

/// Load image
void* imLoad(ImageType type, const char *filename)
{
    assert(type==IMAGE_GRAY || type==IMAGE_RGB);
    unsigned char* data=0;
    size_t xsize, ysize;
    int stepColor=1; // Distance between color planes of same pixel
    int stepPixel=3; // Distance between consecutive pixels

    const char* ext = strrchr(filename,'.');
    if(ext && (strcmp(ext,".png")==0)) {
#ifdef HAS_PNG
        if(type == IMAGE_GRAY)
            data = io_png_read_u8_gray(filename, &xsize, &ysize);
        if(type == IMAGE_RGB) {
            data = io_png_read_u8_rgb(filename, &xsize, &ysize);
            stepColor=xsize*ysize; // Planar color format
            stepPixel=1;
        }
        if(! data) return 0;
#else
        std::cerr << "Unable to read file " << filename << " as PNG since the "
                  << "program was built without PNG support" << std::endl;
        return 0;
#endif
    }

    if(! data) { // Read PGM or PPM
        std::ifstream file(filename, std::ifstream::binary);
        if(! file)
            return 0;
        char c;
        bool text=false;
        while(file >> c)
            if(c=='#') { // Skip comments
                std::string s;
                getline(file,s);
                continue;
            }
            else if(c=='P') {
                file >> c;
                text = (c=='2'||c=='3');
                c -= 3;
                if(c=='2' && type==IMAGE_GRAY) break;
                if(c=='3' && type==IMAGE_RGB) break;
                return 0;
            }
            else return 0;
        int max=0;
        if(! (file >> xsize >> ysize >> max)) return 0;
        assert(max<256);
        int size = ((type==IMAGE_GRAY? 1: 3) *xsize*ysize);
        data = (unsigned char*) malloc(size);
        if(! data) return 0;
        if(text) { // Read ASCII
            int read=0;
            std::string str;
            while(file >> str) {
                if(str[0] == '#') {
                    std::string s;
                    std::getline(file,s);
                    continue;
                }
                int nb;
                if(read==size ||
                   !(std::istringstream(str)>>nb)) {
                    free(data); return 0;
                }
                data[read++] = static_cast<unsigned char>(nb);
            }
        } else { // Read binary
            std::string s;
            std::getline(file,s);
            if(! file.read((char*)(data), size)) {
                free(data); return 0;
            }
        }
    }

    GeneralImage im = (GeneralImage) imNew(type, xsize, ysize);
    const size_t size = xsize*ysize;
    if(type == IMAGE_GRAY)
        for(size_t i=0; i<size; i++)
            imRef((GrayImage)im,i,0) = data[i];
    if(type == IMAGE_RGB) {
        const size_t r=0*stepColor, g=1*stepColor, b=2*stepColor;
        for(size_t i=0, j=0; i<size; i++, j+=stepPixel) {
            imRef((RGBImage)im,i,0).c[0] = data[j+r];
            imRef((RGBImage)im,i,0).c[1] = data[j+g];
            imRef((RGBImage)im,i,0).c[2] = data[j+b];
        }
    }
    free(data);
    return im;
}

int imSave(void *im, const char *filename)
{
    int i;
    int im_max = 0;
    ImageType type = imHeader(im)->type;
    int xsize = imHeader(im)->xsize, ysize = imHeader(im)->ysize;
    int data_size = imHeader(im)->data_size;

    const char* ext = strrchr(filename,'.');
    if(ext && (strcmp(ext,".tif")==0||strcmp(ext,".tiff")==0)) {
#ifdef HAS_TIFF
        assert(type==IMAGE_FLOAT);
        return io_tiff_write_f32(filename,((FloatImage)im)->data,xsize,ysize,1);
#else
        std::cerr << "Unable to save file " << filename << " as TIFF since the "
                  << "program was built without TIFF support. Trying PGM..."
                  << std::endl;
#endif
    }

    if(ext && strcmp(ext,".png")==0) {
#ifdef HAS_PNG
        if(type==IMAGE_GRAY)
            return io_png_write_u8 (filename, ((GrayImage)im)->data,
                                    xsize, ysize, 1);
        if(type==IMAGE_FLOAT)
            return io_png_write_f32(filename, ((FloatImage)im)->data,
                                    xsize, ysize, 1);
        assert(type==IMAGE_RGB);
        const size_t size = xsize*ysize;
        unsigned char* data = new unsigned char[3*size];
        unsigned char *r=data+0*size;
        unsigned char *g=data+1*size;
        unsigned char *b=data+2*size;
        for(size_t i=0; i<size; i++) {
            *r++ = imRef((RGBImage)im,i,0).c[0];
            *g++ = imRef((RGBImage)im,i,0).c[1];
            *b++ = imRef((RGBImage)im,i,0).c[2];
        }
        int res = io_png_write_u8(filename, data, xsize, ysize, 3);
        delete [] data;
        return res;
#else
        std::cerr << "Unable to save file " << filename << " as PNG since the "
                  << "program was built without PNG support. Trying PGM..."
                  << std::endl;
#endif
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) return -1;

    switch (type) {
    case IMAGE_GRAY: {
        GrayImage g = (GrayImage)im;
        for (i=0; i<xsize*ysize; i++)
            if (im_max < g->data[i]) im_max = g->data[i]; }
        fprintf(fp, "P5\n%d %d\n%d\n", xsize, ysize, im_max);
        break;
    case IMAGE_RGB: {
        RGBImage rgb = (RGBImage)im;
        for (i=0; i<xsize*ysize; i++) {
            if (im_max < rgb->data[i].c[0]) im_max = rgb->data[i].c[0];
            if (im_max < rgb->data[i].c[1]) im_max = rgb->data[i].c[1];
            if (im_max < rgb->data[i].c[2]) im_max = rgb->data[i].c[2];
        }}
        fprintf(fp, "P6\n%d %d\n%d\n", xsize, ysize, im_max);
        break;
    case IMAGE_FLOAT:
        fprintf(fp, "Q1\n%d %d\n", xsize, ysize);
        break;
    default:
        fclose(fp);
        return -1;
    }

    SwapBytes((GeneralImage)im);
    i = fwrite(((GeneralImage)im)->data, data_size, xsize*ysize, fp);
    SwapBytes((GeneralImage)im);
    if (i != xsize*ysize) { fclose(fp); return -1; }

    fclose(fp);
    return 0;
}
