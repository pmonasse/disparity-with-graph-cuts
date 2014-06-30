/**
 * @file main.cpp
 * @brief Main program for Kolmogorov-Zabih disparity estimation with graph cuts
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2012-2014, Pascal Monasse
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
#include "cmdLine.h"
#include <limits>
#include <cmath>
#include <ctime>

/// Max denominator for fractions. We need to approximate float values as
/// fractions since the max-flow is implemented using short integers. The
/// denominator multiplies the data term in Match::data_occlusion_penalty. To
/// avoid overflow, we have to make sure this stays below 2^15 (max short).
/// The data term can reach (CUTOFF=30<2^5)^2<2^10 if using L2 norm, so a
/// denominator up to 2^4 will reach 2^14 and not provoke overflow.
static const int MAX_DENOM=1<<4;

/// Is the color image actually gray?
bool isGray(RGBImage im) {
    const int xsize=imGetXSize(im), ysize=imGetYSize(im);
    for(int y=0; y<ysize; y++)
        for(int x=0; x<xsize; x++)
            if(imRef(im,x,y).c[0] != imRef(im,x,y).c[1] ||
               imRef(im,x,y).c[0] != imRef(im,x,y).c[2])
                return false;
    return true;
}

/// Convert to gray level a color image (extract red channel)
void convert_gray(GeneralImage& im) {
    const int xsize=imGetXSize(im), ysize=imGetYSize(im);
    GrayImage g = (GrayImage)imNew(IMAGE_GRAY, xsize, ysize);
    for(int y=0; y<ysize; y++)
        for(int x=0; x<xsize; x++)
            imRef(g,x,y) = imRef((RGBImage)im,x,y).c[0];
    imFree(im);
    im = (GeneralImage)g;
}

/// Store in \a params fractions approximating the last 3 parameters.
///
/// They have the same denominator (up to \c MAX_DENOM), chosen so that the sum
/// of relative errors is minimized.
void set_fractions(Match::Parameters& params,
                   float K, float lambda1, float lambda2) {
    float minError = std::numeric_limits<float>::max();
    for(int i=1; i<=MAX_DENOM; i++) {
        float e = 0;
        int numK=0, num1=0, num2=0;
        if(K>0)
            e += std::abs((numK=int(i*K+.5f))/(i*K) - 1.0f);
        if(lambda1>0)
            e += std::abs((num1=int(i*lambda1+.5f))/(i*lambda1) - 1.0f);
        if(lambda2>0)
            e += std::abs((num2=int(i*lambda2+.5f))/(i*lambda2) - 1.0f);
        if(e<minError) {
            minError = e;
            params.denominator = i;
            params.K = numK;
            params.lambda1 = num1;
            params.lambda2 = num2;
        }
    }
}

/// Make sure parameters K, lambda1 and lambda2 are non-negative.
///
/// - K may be computed automatically and lambda set to K/5.
/// - lambda1=3*lambda, lambda2=lambda
/// As the graph requires integer weights, use fractions and common denominator.
void fix_parameters(Match& m, Match::Parameters& params,
                    float& K, float& lambda, float& lambda1, float& lambda2) {
    if(K<0) { // Automatic computation of K
        m.SetParameters(&params);
        K = m.GetK();
    }
    if(lambda<0) // Set lambda to K/5
        lambda = K/5;
    if(lambda1<0) lambda1 = 3*lambda;
    if(lambda2<0) lambda2 = lambda;
    set_fractions(params, K, lambda1, lambda2);
    m.SetParameters(&params);
}

/// Main program
int main(int argc, char *argv[]) {
    Match::Parameters params = { // Default parameters
        Match::Parameters::L2, 1, // dataCost, denominator
        8, -1, -1, // edgeThresh, lambda1, lambda2 (smoothness cost)
        -1,        // K (occlusion cost)
        4, false   // maxIter, bRandomizeEveryIteration
    };

    CmdLine cmd;
    std::string cost, sDisp;
    float K=-1, lambda=-1, lambda1=-1, lambda2=-1;
    cmd.add( make_option('i', params.maxIter, "max_iter") );
    cmd.add( make_option('o', sDisp, "output") );
    cmd.add( make_switch('r', "random") );
    cmd.add( make_option('c', cost, "data_cost") );
    cmd.add( make_option('k', K) );
    cmd.add( make_option('l', lambda, "lambda") );
    cmd.add( make_option(0, lambda1, "lambda1") );
    cmd.add( make_option(0, lambda2, "lambda2") );
    cmd.add( make_option('t', params.edgeThresh, "threshold") );

    cmd.process(argc, argv);
    if(argc != 5 && argc != 6) {
        std::cerr << "Usage: " << argv[0] << " [options] "
                  << "im1.png im2.png dMin dMax [dispMap.tif]" << std::endl;
        std::cerr << "General options:" << '\n'
                  << " -i,--max_iter iter: max number of iterations" <<'\n'
                  << " -o,--output disp.png: scaled disparity map" <<'\n'
                  << " -r,--random: random alpha order at each iteration" <<'\n'
                  << "Options for cost:" <<'\n'
                  << " -c,--data_cost dist: L1 or L2" <<'\n'
                  << " -l,--lambda lambda: value of lambda (smoothness)" <<'\n'
                  << " --lambda1 l1: smoothness cost not across edge" <<'\n'
                  << " --lambda2 l2: smoothness cost across edge" <<'\n'
                  << " -t,--threshold thres: intensity diff for 'edge'" <<'\n'
                  << " -k k: cost for occlusion" <<std::endl;
        return 1;
    }

    if( cmd.used('r') ) params.bRandomizeEveryIteration=true;
    if( cmd.used('c') ) {
        if(cost == "L1")
            params.dataCost = Match::Parameters::L1;
        else if(cost == "L2")
            params.dataCost = Match::Parameters::L2;
        else {
            std::cerr << "The cost parameter must be 'L1' or 'L2'" << std::endl;
            return 1;
        }
    }

    GeneralImage im1 = (GeneralImage)imLoad(IMAGE_RGB, argv[1]);
    GeneralImage im2 = (GeneralImage)imLoad(IMAGE_RGB, argv[2]);
    if(!im1 || !im2) {
        std::cerr << "Unable to read image " << argv[im1?2:1] << std::endl;
        return 1;
    }
    bool color=true;
    if(isGray((RGBImage)im1) && isGray((RGBImage)im2)) {
        color=false;
        convert_gray(im1);
        convert_gray(im2);
    }
    Match m(im1, im2, color);

    // Disparity
    int dMin=0, dMax=0;
    std::istringstream f(argv[3]), g(argv[4]);
    if(! ((f>>dMin).eof() && (g>>dMax).eof())) {
        std::cerr << "Error reading dMin or dMax" << std::endl;
        return 1;
    }
    m.SetDispRange(dMin, dMax);

    time_t seed = time(NULL);
    srand((unsigned int)seed);

    fix_parameters(m, params, K, lambda, lambda1, lambda2);
    if(argc>5 || !sDisp.empty()) {
        m.KZ2();
        if(argc>5)
            m.SaveXLeft(argv[5]);
        if(! sDisp.empty())
            m.SaveScaledXLeft(sDisp.c_str(), false);
    } else {
        std::cout << "K=" << K << std::endl;
        std::cout << "lambda=" << lambda << std::endl;
    }

    imFree(im1);
    imFree(im2);
    return 0;
}
