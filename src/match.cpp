/* match.cpp */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001-2003. */

#include "match.h"
#include "nan.h"
#include <iostream>
#include <iomanip>
#include <cassert>

/// Constructor
Match::Match(GeneralImage left, GeneralImage right, bool color)
{
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

        im_left_min = im_left_max = im_right_min = im_right_max = 0;
    } else {
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

            im_color_left_min = im_color_left_max = 0;
            im_color_right_min = im_color_right_max = 0;
        }

    dispMin = dispMax = 0;

    x_left  = (IntImage)imNew(IMAGE_INT, im_size.x, im_size.y);
    x_right = (IntImage)imNew(IMAGE_INT, im_size.x, im_size.y);
    if (!x_left || !x_right)
        { std::cerr << "Not enough memory!" << std::endl; exit(1); }

    Coord p;
    for (p.y=0; p.y<im_size.y; p.y++)
        for (p.x=0; p.x<im_size.x; p.x++)
            IMREF(x_left, p) = IMREF(x_right, p) = OCCLUDED;

    vars0 = (IntImage)imNew(IMAGE_INT, im_size.x, im_size.y);
    varsA = (IntImage)imNew(IMAGE_INT, im_size.x, im_size.y);
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
    Coord p;
    FloatImage out = (FloatImage)imNew(IMAGE_FLOAT, im_size.x,im_size.y);

    for (p.y=0; p.y<im_size.y; p.y++)
        for (p.x=0; p.x<im_size.x; p.x++) {
            int d=IMREF(x_left,p);
            IMREF(out,p) = (d==OCCLUDED? NaN: static_cast<float>(d));
        }

    imSave(out, file_name);
    imFree(out);
}

/// Save scaled disparity map as 8-bit color image (gray between 64 and 255).
/// flag: lowest disparity should appear darkest (true) or brightest (false).
void Match::SaveScaledXLeft(const char *file_name, bool flag)
{
    RGBImage im = (RGBImage)imNew(IMAGE_RGB, im_size.x, im_size.y);
    const int dispSize = dispMax-dispMin+1;

    Coord p;
    for (p.y=0; p.y<im_size.y; p.y++)
        for (p.x=0; p.x<im_size.x; p.x++) {
            int d = IMREF(x_left, p), c;
            if (d==OCCLUDED) {
                IMREF(im, p).r=0; IMREF(im, p).g=IMREF(im, p).b=255;
            } else {
                if (dispSize == 0) c = 255;
                else if (flag) c = 255 - (255-64)*(dispMax - d)/dispSize;
                else           c = 255 - (255-64)*(d - dispMin)/dispSize;
                IMREF(im,p).r=IMREF(im,p).g=IMREF(im,p).b = c;
            }
        }

    imSave(im, file_name);
    imFree(im);
}

/// Specify disparity range
void Match::SetDispRange(int _disp_base, int _disp_max)
{
    dispMin = _disp_base;
    dispMax = _disp_max;
    if (! (dispMin<=dispMax) ) {
        std::cerr << "Error: wrong disparity range!\n" << std::endl;
        exit(1);
    }
}

/// Main algorithm
void Match::KZ2()
{
    if (params.K<0 || params.I_threshold2<0 ||
        params.lambda1<0 || params.lambda2<0 || params.denominator<1) {
        std::cerr << "Error in KZ2: wrong parameter!" << std::endl;
        exit(1);
    }

    if (params.denominator == 1)
        std::cout << "KZ2:  K=" << params.K << std::endl
                  << "      I_threshold2=" << params.I_threshold2
                  << ", lambda1=" << params.lambda1
                  << ", lambda2=" << params.lambda2
                  << std::endl;
    else
        std::cout << "KZ2:  K=" << params.K<<'/'<<params.denominator <<std::endl
                  << "      I_threshold2=" << params.I_threshold2
                  << ", lambda1=" << params.lambda1<<'/'<<params.denominator
                  << ", lambda2=" << params.lambda1<<'/'<<params.denominator
                  << std::endl;
    std::cout << "      data_cost = L" <<
        ((params.data_cost==Parameters::L1)? '1': '2') << std::endl;

    Run();
}

/// Generate a random permutation of the array elements
void generate_permutation(int *buf, int n)
{
    for(int i=0; i<n; i++) buf[i] = i;
    for(int i=0; i<n-1; i++) {
        int j = i + (int) (((double)rand()/(RAND_MAX+1.0))*(n - i));
        std::swap(buf[i],buf[j]);
    }
}

/// Main algorithm: a series of alpha-expansions.
void Match::Run()
{
    // Display 1 number after decimal separator for number of iterations
    std::cout << std::fixed << std::setprecision(1);

    const int dispSize = dispMax-dispMin+1;
    int* permutation = new int[dispSize]; // random permutation
    bool* buf = new bool[dispSize]; // Can expansion of label decrease energy?
    int nBuf = dispSize; // number of 'false' entries in buf

    E = ComputeEnergy();
    std::cout << "E=" << E << std::endl;

    std::fill_n(buf, dispSize, false);
    int step=0;
    for(int iter=0; iter<params.iter_max && nBuf>0; iter++) {
        if (iter==0 || params.randomize_every_iteration)
            generate_permutation(permutation, dispSize);

        for(int index=0; index<dispSize; index++) {
            int label = permutation[index];
            if (buf[label]) continue;

            int oldE=E;
            ExpansionMove(dispMin+label);
            assert(ComputeEnergy()==E);

            ++step;
            std::cout << (oldE==E? '-': '*') << std::flush;

            if(oldE == E) {
                buf[label] = true; --nBuf;
            } else {
                std::fill_n(buf, dispSize, false);
                buf[label] = true;
                nBuf = dispSize-1;
            }
        }
        std::cout << " E=" << E << std::endl;
    }

    std::cout << (float)step/dispSize << " iterations" << std::endl;

    delete [] permutation;
    delete [] buf;
}
