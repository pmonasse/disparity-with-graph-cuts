/*
 * Copyright (c) 2010, Pascal Monasse <monasse@imagine.enpc.fr>
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under, at your option, the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version, or
 * the terms of the simplified BSD license.
 *
 * You should have received a copy of these licenses along this
 * program. If not, see <http://www.gnu.org/licenses/> and
 * <http://www.opensource.org/licenses/bsd-license.html>.
 */

/**
 * @file io_tiff.c
 * @brief TIFF read/write simplified interface
 *
 * This is a front-end to libtiff, with routines to:
 * @li read a TIFF file as a deinterlaced float array
 * @li write a float array to a TIFF file
 *
 * @todo handle multi-channel images and on-the-fly color model conversion
 * @todo add a test suite
 * @todo internally handle RGB/gray conversion in read_tiff_raw()
 * @todo handle deinterlacing as a libtiff transform function
 *
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/* option to use a local version of the libtiff */
#ifdef IO_TIFF_LOCAL_LIBTIFF
#include "tiffio.h"
#else
#include <tiffio.h>
#endif

/* ensure consistency */
#include "io_tiff.h"

/*
 * INFO
 */

/* string tag inserted into the binary */
static char io_tiff_tag[] = "using io_tiff " IO_TIFF_VERSION;
/**
 * @brief helps tracking versions, via the string tag inserted into
 * the library
 *
 * This function is not expected to be used in real-world programs.
 *
 * @return a pointer to a version info string
 */
char *io_tiff_info(void)
{
    return io_tiff_tag;
}

/*
 * READ
 */

/**
 * Read a TIFF float image.
 */
static float *readTIFF(TIFF * tif, size_t * nx, size_t * ny)
{
    uint32 w = 0, h = 0, i;
    uint16 spp = 0, bps = 0, fmt = 0;
    float *data, *line;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &fmt);
    if (spp != 1 || bps != (uint16) sizeof(float) * 8
        || fmt != SAMPLEFORMAT_IEEEFP)
        return NULL;

    assert((size_t) TIFFScanlineSize(tif) == w * sizeof(float));
    data = (float *) malloc(w * h * sizeof(float));
    *nx = (size_t) w;
    *ny = (size_t) h;
    for (i = 0; i < h; i++) {
        line = data + i * w;
        if (TIFFReadScanline(tif, line, i, 0) < 0) {
            fprintf(stderr, "readTIFF: error reading row %u\n", i);
            free(data);
            return NULL;
        }
    }

    return data;
}

/**
 * Load TIFF float image.
 */
float *io_tiff_read_f32_gray(const char *fname, size_t * nx, size_t * ny)
{
    float *data;
    TIFF *tif = TIFFOpen(fname, "r");
    if (!tif) {
        fprintf(stderr, "Unable to read TIFF file %s\n", fname);
        return NULL;
    }
    data = readTIFF(tif, nx, ny);
    TIFFClose(tif);
    return data;
}

/*
 * WRITE
 */

/**
 * Write a TIFF float image.
 */
static int writeTIFF(TIFF * tif, const float *data, size_t w, size_t h,
                     size_t c)
{
    uint32 rowsperstrip;
    int ok;
    size_t k, i;
    float *line;

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32) w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32) h);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_SEPARATE);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16) c);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16) sizeof(float) * 8);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    rowsperstrip = TIFFDefaultStripSize(tif, (uint32) h);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

    ok = 1;
    for (k = 0; ok && k < c; k++)
        for (i = 0; ok && i < h; i++) {
            line = (float *) (data + (i + k * h) * w);
            if (TIFFWriteScanline(tif, line, (uint32) i, (tsample_t) k) < 0) {
                fprintf(stderr, "writeTIFF: error writing row %i\n", (int) i);
                ok = 0;
            }
        }
    return ok;
}

/**
 * Write float image as TIFF 32 bits per sample.
 */
int io_tiff_write_f32(const char *fname, const float *data,
                      size_t nx, size_t ny, size_t nc)
{
    int ok;
    TIFF *tif = TIFFOpen(fname, "w");
    if (!tif) {
        fprintf(stderr, "Unable to write TIFF file %s\n", fname);
        return 0;
    }

    ok = writeTIFF(tif, data, nx, ny, nc);
    TIFFClose(tif);
    return (ok ? 0 : -1);
}
