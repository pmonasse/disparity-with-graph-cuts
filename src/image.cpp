/* image.cpp */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001. */

#include <stdio.h>
#include "image.h"
#include "io_png.h"

const int ONE = 1;
const int SWAP_BYTES = (((char *)(&ONE))[0] == 0) ? 1 : 0;

/************************************************************/
/************************************************************/
/************************************************************/

void * imNew(ImageType type, int xsize, int ysize)
{
	void *ptr;
	GeneralImage im;
	int data_size;
	int y;

	if (xsize<=0 || ysize<=0) return NULL;

	switch (type)
	{
		case IMAGE_BINARY: data_size = sizeof(unsigned char);    break;
		case IMAGE_GRAY:   data_size = sizeof(unsigned char);    break;
		case IMAGE_SHORT:  data_size = sizeof(short);            break;
		case IMAGE_RGB:    data_size = sizeof(unsigned char[3]); break;
		case IMAGE_INT:    data_size = sizeof(int);              break;
		case IMAGE_PTR:    data_size = sizeof(void *);           break;
		case IMAGE_FLOAT:  data_size = sizeof(float);            break;
		case IMAGE_DOUBLE: data_size = sizeof(double);           break;
		default: return NULL;
	}

	ptr = malloc(sizeof(ImageHeader) + ysize*sizeof(void*));
	if (!ptr) return NULL;
	im = (GeneralImage) ((char*)ptr + sizeof(ImageHeader));

	imHeader(im) -> type      = type;
	imHeader(im) -> data_size = data_size;
	imHeader(im) -> xsize     = xsize;
	imHeader(im) -> ysize     = ysize;

	im->data = (void *) malloc(xsize*ysize*data_size);
	for (y=1; y<ysize; y++) (im+y)->data = ((char*)(im->data)) + xsize*y*data_size;

	return im;
}

/************************************************************/
/************************************************************/
/************************************************************/

inline int is_digit(unsigned char c) { return (c>='0' && c<='9'); }

void SwapBytes(GeneralImage im)
{
	if (SWAP_BYTES)
	{
		ImageType type = imHeader(im) -> type;

		if (type==IMAGE_SHORT || type==IMAGE_INT || type==IMAGE_FLOAT || type==IMAGE_DOUBLE)
		{
			char *ptr, c;
			int i, k;

			int size = (imHeader(im) -> xsize) * (imHeader(im) -> ysize);
			int data_size = imHeader(im) -> data_size;

			ptr = (char *) (im->data);
			for (i=0; i<size; i++)
			{
				for (k=0; k<data_size/2; k++)
				{
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
    unsigned char* data=0;
    size_t xsize, ysize;
    if(type == IMAGE_GRAY)
        data = io_png_read_u8_gray(filename, &xsize, &ysize);
    if(type == IMAGE_RGB)
        data = io_png_read_u8_rgb(filename, &xsize, &ysize);
    if(! data)
        return data;

	GeneralImage im = (GeneralImage) imNew(type, xsize, ysize);
    const size_t size = xsize*ysize;
    if(type == IMAGE_GRAY)
        for(size_t i=0; i<size; i++)
            imRef((GrayImage)im,i,0) = data[i];
    if(type == IMAGE_RGB) {
        const size_t r=0*size, g=1*size, b=2*size;
        for(size_t i=0; i<size; i++) {
            imRef((RGBImage)im,i,0).r = data[i+r];
            imRef((RGBImage)im,i,0).g = data[i+g];
            imRef((RGBImage)im,i,0).b = data[i+b];
        }
    }
    free(data);
    return im;
}

/************************************************************/
/************************************************************/
/************************************************************/

int imSave(void *im, const char *filename)
{
	int i;
	FILE *fp;
	int im_max = 0;
	ImageType type = imHeader(im) -> type;
	int xsize = imHeader(im) -> xsize, ysize = imHeader(im) -> ysize;
	int data_size = imHeader(im) -> data_size;

	fp = fopen(filename, "wb");
	if (!fp) return -1;

	switch (type)
	{
		case IMAGE_BINARY:
			fprintf(fp, "P4\n%d %d\n", xsize, ysize);
			break;
		case IMAGE_GRAY:
			for (i=0; i<xsize*ysize; i++)
			if (im_max < ((GrayImage)im)->data[i]) im_max = ((GrayImage)im)->data[i];
			fprintf(fp, "P5\n%d %d\n%d\n", xsize, ysize, im_max);
			break;
		case IMAGE_RGB:
			for (i=0; i<xsize*ysize; i++)
			{
				if (im_max < ((RGBImage)im)->data[i].r) im_max = ((RGBImage)im)->data[i].r;
				if (im_max < ((RGBImage)im)->data[i].g) im_max = ((RGBImage)im)->data[i].g;
				if (im_max < ((RGBImage)im)->data[i].b) im_max = ((RGBImage)im)->data[i].b;
			}
			fprintf(fp, "P6\n%d %d\n%d\n", xsize, ysize, im_max);
			break;
		case IMAGE_SHORT:
			fprintf(fp, "Q4\n%d %d\n", xsize, ysize);
			break;
		case IMAGE_INT:
			fprintf(fp, "Q3\n%d %d\n", xsize, ysize);
			break;
		case IMAGE_FLOAT:
			fprintf(fp, "Q1\n%d %d\n", xsize, ysize);
			break;
		case IMAGE_DOUBLE:
			fprintf(fp, "Q2\n%d %d\n", xsize, ysize);
			break;
		default:
			fclose(fp);
			return -1;
	}

	if (type == IMAGE_BINARY)
	{
		int s = (xsize*ysize+7)/8;
		unsigned char *b = (unsigned char *) malloc(s);
		if (!b) { fclose(fp); return -1; }
		for (i=0; i<s; i++) b[i] = 0;
		for (i=0; i<xsize*ysize; i++)
		{
			if (((BinaryImage)im)->data[i])
				b[i/8] ^= (1<<(7-(i%8)));
		}
		i = fwrite(b, 1, s, fp);
		free(b);
		if (i != s) { fclose(fp); return -1; }
	}
	else
	{
		SwapBytes((GeneralImage)im);
		i = fwrite(((GeneralImage)im)->data, data_size, xsize*ysize, fp);
		SwapBytes((GeneralImage)im);
		if (i != xsize*ysize) { fclose(fp); return -1; }
	}

	fclose(fp);
	return 0;
}
