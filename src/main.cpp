#include "match.h"
#include "cmdLine.h"
#include <cstdio>

/// Is the color image actually gray?
bool isGray(RGBImage im) {
    const int xsize=imGetXSize(im), ysize=imGetYSize(im);
    for(int y=0; y<ysize; y++)
        for(int x=0; x<xsize; x++)
            if(imRef(im,x,y).r != imRef(im,x,y).g ||
               imRef(im,x,y).r != imRef(im,x,y).b)
                return false;
    return true;
}

/// Convert to gray level a color image (extract red channel)
void convert_gray(GeneralImage& im) {
    const int xsize=imGetXSize(im), ysize=imGetYSize(im);
    GrayImage g = (GrayImage)imNew(IMAGE_GRAY, xsize, ysize);
    for(int y=0; y<ysize; y++)
        for(int x=0; x<xsize; x++)
            imRef(g,x,y) = imRef((RGBImage)im,x,y).r;
    imFree(im);
    im = (GeneralImage)g;
}

/// Decode a string as a fraction. Accept also values AUTO (=-1) and INFINITY.
bool GetFraction(const std::string& s, int& numerator, int& denominator) {
    if(s=="INFINITY") { numerator = INFINITY; denominator = 1; return true; }
    if(s=="AUTO")     { numerator = -1;       denominator = 1; return true; }
    if(std::sscanf(s.c_str(), "%d/%d", &numerator, &denominator) != 2) {
        if(std::sscanf(s.c_str(), "%d", &numerator) == 1)
            denominator = 1;
    }
    bool ok = (numerator>=0 && denominator>=1);
    if(! ok)
        std::cerr << "Unable to decode " << s << " as fraction" << std::endl;
    return ok;
}

/// Set lambda to value lambda/denom. As we have to keep as int, we need to
/// modify the overall params.denominator in consequence.
void setLambda(int& lambda, int denom, Match::Parameters& params) {
    lambda             *= params.denominator;
    params.lambda1     *= denom;
    params.lambda2     *= denom;
    params.K           *= denom;
    params.denominator *= denom;
}

/// Set params.K to value params.K/denom. See setLambda for explanation.
void setK(int& lambda, int denom, Match::Parameters& params) {
    lambda             *= denom;
    params.lambda1     *= denom;
    params.lambda2     *= denom;
    params.K           *= params.denominator;
    params.denominator *= denom;
}

/// GCD of integers
int gcd(int a, int b) {
    if(b == 0) return a;
    int r = a % b;
    return gcd(b, r);
}

void fix_parameters(Match& m, Match::Parameters params, int lambda) {
    if(lambda<0) { // Set lambda to K/5
        float K = params.K/(float)params.denominator;
        if(params.K<0) { // Automatic computation of K
            m.SetParameters(&params);
            K = m.GetK();
        }
        K /= 5; int denom = 1;
        while(K < 3) { K *= 2; denom *= 2; }
        lambda = int(K+0.5f);
        setLambda(lambda, denom, params);
    }
    if(params.K<0) params.K = 5*lambda;
    if(params.lambda1<0) params.lambda1 = 3*lambda;
    if(params.lambda2<0) params.lambda2 = lambda;
    int denom = gcd(params.K,
                    gcd(params.lambda1,
                        gcd(params.lambda2,
                            params.denominator)));
    if(denom) {
        params.K /= denom;
        params.lambda1 /= denom;
        params.lambda2 /= denom;
        params.denominator /= denom;
    }
    m.SetParameters(&params);
}

int main(int argc, char *argv[]) {
    // Default parameters
    Match::Parameters params = {
        Match::Parameters::L2, 1, /* data_cost, denominator */
        8, -1, -1,       /* I_threshold2, lambda1, lambda2 */
        -1,                 /* K */
        INFINITY, false,    /* iter_max, randomize_every_iteration */
    };

    CmdLine cmd;
    std::string cost, slambda, slambda1, slambda2, sK, sDisp;
    cmd.add( make_option('c', cost, "data_cost") );
    cmd.add( make_option('t', params.I_threshold2, "threshold2") );
    cmd.add( make_option('i', params.iter_max, "iter_max") );
    cmd.add( make_switch('r', "random") );
    cmd.add( make_option('l', slambda, "lambda") );
    cmd.add( make_option(0, slambda1, "lambda1") );
    cmd.add( make_option(0, slambda2, "lambda2") );
    cmd.add( make_option('k', sK) );
    cmd.add( make_option('o', sDisp, "output") );

    cmd.process(argc, argv);
    if(argc != 5 && argc != 6) {
        std::cerr << "Usage: " << argv[0] << " [options] "
                  << "im1.png im2.png dMin dMax [dispMap.tif]" << std::endl;
        return 1;
    }

    if( cmd.used('r') ) params.randomize_every_iteration=true;
    if( cmd.used('c') ) {
        if(cost == "L1")
            params.data_cost = Match::Parameters::L1;
        else if(cost == "L2")
            params.data_cost = Match::Parameters::L2;
        else {
            std::cerr << "The cost parameter must be 'L1' or 'L2'" << std::endl;
            return 1;
        }
    }

    int lambda=-1;
    if(! slambda.empty()) {
        int denom;
        if(! GetFraction(slambda, lambda, denom)) return 1;
        setLambda(lambda, denom, params);
    }
    if(! sK.empty()) {
        int denom;
        if(! GetFraction(sK, params.K, denom)) return 1;
        setK(lambda, denom, params);
    }

    GeneralImage im1 = (GeneralImage)imLoad(IMAGE_RGB, argv[1]);
    GeneralImage im2 = (GeneralImage)imLoad(IMAGE_RGB, argv[2]);
    if(! im1) {
        std::cerr << "Unable to read image " << argv[1] << std::endl;
        return 1;
    }
    if(! im2) {
        std::cerr << "Unable to read image " << argv[2] << std::endl;
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
    int disp_base=0, disp_max=0;
    std::istringstream f(argv[3]), g(argv[4]);
    if(! ((f>>disp_base).eof() && (g>>disp_max).eof())) {
        std::cerr << "Error reading dMin or dMax" << std::endl;
        return 1;
    }
    m.SetDispRange(disp_base, disp_max);

    fix_parameters(m, params, lambda);
    if(argc>5 || !sDisp.empty()) {
        m.KZ2();
        if(argc>5)
            m.SaveXLeft(argv[5]);
        if(! sDisp.empty())
            m.SaveScaledXLeft(sDisp.c_str(), false);
    }

    imFree(im1);
    imFree(im2);
    return 0;
}
