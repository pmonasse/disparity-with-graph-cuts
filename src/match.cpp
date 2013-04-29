/* match.cpp */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001-2003. */

#include "match.h"
#include "io_tiff.h"
#include "nan.h"
#include <iostream>
#include <ctime>
#include <cstdio>

/************************************************************/
/************************************************************/
/************************************************************/

Match::Match(GeneralImage left, GeneralImage right, bool color)
{
	Coord p;

	if (!color) {
		im_color_left = im_color_right = NULL;
		im_color_left_min = im_color_right_min = NULL;
		im_color_left_max = im_color_right_max = NULL;

		im_left  = (GrayImage)left;
		im_right = (GrayImage)right;

		im_size.x = imGetXSize(im_left); im_size.y = imGetYSize(im_left);

		if(im_size.x!=imGetXSize(im_right) ||
           im_size.y!=imGetYSize(im_right) ) {
            std::cerr << "Image sizes are different!" << std::endl;
			exit(1);
		}

		im_left_min = im_left_max = im_right_min = im_right_max = NULL;
	}
	else
	{
		im_left = im_right = NULL;
		im_left_min = im_right_min = NULL;
		im_left_max = im_right_max = NULL;

		im_color_left = (RGBImage)left;
		im_color_right = (RGBImage)right;

		im_size.x = imGetXSize(im_color_left);
        im_size.y = imGetYSize(im_color_left);

		if(im_size.x!=imGetXSize(im_color_right) ||
           im_size.y!=imGetYSize(im_color_right)) {
            std::cerr << "Image sizes are different!" << std::endl;
			exit(1);
		}

		im_color_left_min = im_color_left_max = im_color_right_min = im_color_right_max = NULL;
	}

	disp_base = 0; disp_max = 0; disp_size = 1;

	x_left  = (IntImage) imNew(IMAGE_INT, im_size.x, im_size.y);
	x_right = (IntImage) imNew(IMAGE_INT, im_size.x, im_size.y);
	if (!x_left || !x_right)
        { std::cerr << "Not enough memory!" << std::endl; exit(1); }
	
	for (p.y=0; p.y<im_size.y; p.y++)
	for (p.x=0; p.x<im_size.x; p.x++)
        IMREF(x_left, p) = IMREF(x_right, p) = OCCLUDED;

	vars0 = (IntImage) imNew(IMAGE_INT, im_size.x, im_size.y);
	varsA = (IntImage) imNew(IMAGE_INT, im_size.x, im_size.y);
}

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

/************************************************************/
/************************************************************/
/************************************************************/

void Match::SaveXLeft(const char *file_name)
{
	Coord p;
    float* out=new float[im_size.x*im_size.y], *c=out;

	for (p.y=0; p.y<im_size.y; p.y++)
    for (p.x=0; p.x<im_size.x; p.x++, c++)
	{
		int d=IMREF(x_left,p);
		*c = (d==OCCLUDED? NaN: static_cast<float>(d));
	}

	io_tiff_write_f32(file_name, out, im_size.x, im_size.y, 1);
    delete [] out;
}

void Match::SaveScaledXLeft(const char *file_name, bool flag)
{
	Coord p;
	RGBImage im = (RGBImage) imNew(IMAGE_RGB, im_size.x, im_size.y);

	for (p.y=0; p.y<im_size.y; p.y++)
	for (p.x=0; p.x<im_size.x; p.x++)
	{
		int d = IMREF(x_left, p), c;
		if (d==OCCLUDED) { IMREF(im, p).r = 0; IMREF(im, p).g = IMREF(im, p).b = 255; }
		else
		{
			if (disp_size == 0) c = 255;
			else if (flag) c = 255 - (255-64)*(disp_max - d)/disp_size;
			else           c = 255 - (255-64)*(d - disp_base)/disp_size;
			IMREF(im, p).r = IMREF(im, p).g = IMREF(im, p).b = c;
		}
	}

	imSave(im, file_name);
	imFree(im);
}

void Match::LoadXLeft(const char *file_name, bool flag)
{
	Coord p;
	GrayImage im = (GrayImage) imLoad(IMAGE_GRAY, file_name);

	if (!im) { fprintf(stderr, "Can't load %s\n", file_name); exit(1); }
	if ( im_size.x != imGetXSize(im) || im_size.y != imGetYSize(im) )
	{
		fprintf(stderr, "Size of the disparity map in %s is different!\n", file_name);
		exit(1);
	}

	for (p.y=0; p.y<im_size.y; p.y++)
	for (p.x=0; p.x<im_size.x; p.x++)
	{
		int d, c = IMREF(im, p);

		if (c>=0 && c<disp_size)
		{
			if (flag) d = c + disp_base;
			else      d = disp_max - c;
			IMREF(x_left, p) = d;
		}
		else IMREF(x_left, p) = OCCLUDED;
	}

	printf("Left x-disparity map from %s loaded\n\n", file_name);
	imFree(im);
}

/************************************************************/
/************************************************************/
/************************************************************/

void Match::SetDispRange(int _disp_base, int _disp_max)
{
	disp_base = _disp_base;
	disp_max = _disp_max;
	disp_size = disp_max - disp_base + 1;
	if (! (disp_base <= disp_max) ) { fprintf(stderr, "Error: wrong disparity range!\n"); exit(1); }
}

/************************************************************/
/************************************************************/
/************************************************************/

void Match::KZ2()
{
	if ( params.K < 0 ||
	     params.I_threshold2 < 0 ||
	     params.lambda1 < 0 ||
	     params.lambda2 < 0 ||
	     params.denominator < 1 )
	{
		fprintf(stderr, "Error in KZ2: wrong parameter!\n");
		exit(1);
	}

	/* printing parameters */
	if (params.denominator == 1)
	{
		printf("KZ2:  K = %d\n", params.K);
		printf("      I_threshold2 = %d, lambda1 = %d, lambda2 = %d\n",
			params.I_threshold2, params.lambda1, params.lambda2);
	}
	else
	{
		printf("KZ2:  K = %d/%d\n", params.K, params.denominator);
		printf("      I_threshold2 = %d, lambda1 = %d/%d, lambda2 = %d/%d\n",
			params.I_threshold2, params.lambda1, params.denominator,
			                    params.lambda2, params.denominator);
	}
	printf("      data_cost = L%d\n",
		params.data_cost==Parameters::L1 ? 1 : 2);

	Run();
}


/************************************************************/
/************************************************************/
/************************************************************/

void generate_permutation(int *buf, int n)
{
	for(int i=0; i<n; i++) buf[i] = i;
	for(int i=0; i<n-1; i++) {
		int j = i + (int) (((double)rand()/(RAND_MAX+1.0))*(n - i));
        std::swap(buf[i],buf[j]);
	}
}

void Match::Run()
{
	unsigned int seed = time(NULL);
    seed=0; // ------------- DEBUG ----------------
	printf("Random seed = %d\n", seed);
	srand(seed);

	int* permutation = new int[disp_size]; // random permutation
	bool* buf = new bool[disp_size]; // Can expansion of label decrease energy?
	int nBuf = disp_size; // number of 'false' entries in buf

	ComputeEnergy();
	printf("E = %d\n", E);

    std::fill_n(buf, disp_size, false);
	int step=0;
	for(int iter=0; iter<params.iter_max && nBuf>0; iter++)
	{
		if (iter==0 || params.randomize_every_iteration)
			generate_permutation(permutation, disp_size);

		for(int index=0; index<disp_size; index++)
		{
			int label = permutation[index];
			if (buf[label]) continue;

			int a = disp_base + label;
			if (a > disp_max) a = OCCLUDED;

			int oldE=E;

			ExpansionMove(a);

#ifndef NDEBUG
			{
				int tmpE=E;
				ComputeEnergy();
				if(tmpE!=E)	{
					fprintf(stderr, "E and tmpE are different! (E = %d, tmpE = %d)\n", E, tmpE);
					exit(1);
				}
			}
#endif

			step ++;
			if(oldE == E) printf("-");
			else printf("*");
            printf("E=%d\n", E);
			fflush(stdout);

			if(oldE == E) {
				buf[label] = true; --nBuf;
			} else {
                std::fill_n(buf, disp_size, false);
				buf[label] = true;
				nBuf = disp_size-1;
			}
		}
		printf(" E = %d\n", E); fflush(stdout);
	}

	printf("%.1f iterations\n", ((float)step)/disp_size);

	delete [] permutation;
	delete [] buf;
}

/************************************************************/
/************************************************************/
/************************************************************/
