/* match.h */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001-2003. */

#ifndef __MATCH_H__
#define __MATCH_H__

#include "image.h"
class Energy;

/// Pixel coordinates with basic operations.
struct Coord
{
    int x, y;

    Coord() {}
    Coord(int a, int b) { x = a; y = b; }

    Coord operator- ()        { return Coord(-x, -y); }
    Coord operator+ (Coord a) { return Coord(x + a.x, y + a.y); }
    Coord operator- (Coord a) { return Coord(x - a.x, y - a.y); }
    Coord operator+ (int a) { return Coord(x+a,y); }
    Coord operator- (int a) { return Coord(x-a,y); }
    bool  operator< (Coord a) { return (x <  a.x) && (y <  a.y); }
    bool  operator<=(Coord a) { return (x <= a.x) && (y <= a.y); }
    bool  operator> (Coord a) { return (x >  a.x) && (y >  a.y); }
    bool  operator>=(Coord a) { return (x >= a.x) && (y >= a.y); }
    bool  operator==(Coord a) { return (x == a.x) && (y == a.y); }
    bool  operator!=(Coord a) { return (x != a.x) || (y != a.y); }
};
/// Value of image im at pixel p
#define IMREF(im, p) (imRef((im), (p).x, (p).y))

///< (half of) the neighborhood system.
/// The full neighborhood system is edges in NEIGHBORS plus reversed edges
const struct Coord NEIGHBORS[] = { Coord(-1,0), Coord(0,1) };
#define NEIGHBOR_NUM (sizeof(NEIGHBORS) / sizeof(Coord))

/// Main class for Kolmogorov-Zabih algorithm
class Match
{
public:
    Match(GeneralImage left, GeneralImage right, bool color=false);
    ~Match();

    void SetDispRange(int disp_base, int disp_max);

    /// Parameters of algorithm.
    struct Parameters
    {
        enum { L1, L2 } data_cost; ///< Data term
        /// Data term must be multiplied by denominator.
        /// Equivalent to using lambda1/denom, lambda2/denom, K/denom
        int denominator;

        // Smoothness term
        int I_threshold2; ///< Intensity level diff for 'edge'
        int lambda1; ///< Smoothness cost not across edge
        int lambda2; ///< Smoothness cost across edge (should be <=lambda1)
        int K; ///< Penalty for inactive assignment

        int iter_max; ///< Maximum number of iterations
        bool randomize_every_iteration; ///< Random alpha order at each iter

    };
    float GetK();
    void SetParameters(Parameters *params);
    void KZ2();

    void SaveXLeft(const char *file_name); ///< Save disp. map as float TIFF
    void SaveScaledXLeft(const char *file_name, bool flag); ///< Save as PPM

private:
    Coord im_size; ///< Image dimensions
    GrayImage im_left, im_right; ///< Original images (if gray)
    RGBImage im_color_left, im_color_right; ///< Original images (if color)
    GrayImage im_left_min, im_left_max;   ///< Range of gray based on neighbors
    GrayImage im_right_min, im_right_max; ///< Range of gray based on neighbors
    RGBImage im_color_left_min, im_color_left_max; ///< For color images
    RGBImage im_color_right_min, im_color_right_max;
    int dispMin, dispMax; ///< range of disparities

#define OCCLUDED 255 ///< Special value of disparity meaning occlusion
    /// if l - pixel in the left image, r - pixel in the right image, then
    /// r == l + Coord(IMREF(x_left,  l), l.y)
    /// l == r + Coord(IMREF(x_right, r), r.y)
    IntImage  x_left, x_right;
    Parameters  params;

    int E; ///< Current energy
    IntImage vars0; ///< Variables before alpha expansion
    IntImage varsA; ///< Variables after alpha expansion

    void Run();
    void InitSubPixel();

    // Data penalty functions
    int data_penalty_gray (Coord l, Coord r) const;
    int data_penalty_color(Coord l, Coord r) const;

    // Smoothness penalty functions
    int   smoothness_penalty_gray (Coord p, Coord np, int d) const;
    int   smoothness_penalty_color(Coord p, Coord np, int d) const;

    // Kolmogorov-Zabih algorithm
    int   data_occlusion_penalty(Coord l, Coord r) const;
    int   smoothness_penalty(Coord p, Coord np, int d) const;
    int   ComputeEnergy() const;
    void  ExpansionMove(int a);

    // Graph construction
    void build_nodes     (Energy& e, Coord p, int a);
    void build_uniqueness(Energy& e, Coord p, int a);
    void build_smoothness(Energy& e, Coord p, Coord np, int a);
    void update_disparity(const Energy& e, int a);
};

#endif
