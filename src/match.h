/* match.h */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001-2003. */

#ifndef __MATCH_H__
#define __MATCH_H__

#include "image.h"
class Energy;

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
#define IMREF(im, p) (imRef((im), (p).x, (p).y))

/* (half of) the neighborhood system
   the full neighborhood system is edges in NEIGHBORS
   plus reversed edges in NEIGHBORS */
//const struct Coord NEIGHBORS[] = { Coord(1, 0), Coord(0, -1) };
const struct Coord NEIGHBORS[] = { Coord(-1, 0), Coord(0, 1) };
#define NEIGHBOR_NUM (sizeof(NEIGHBORS) / sizeof(Coord))

class Match
{
public:
	Match(GeneralImage left, GeneralImage right, bool color=false);
	~Match();

	void SaveXLeft(const char *file_name); ///< Save disp. map as float TIFF

	/* save disparity maps as scaled .ppm images */
	void SaveScaledXLeft(const char *file_name, bool flag); /* if flag is TRUE then larger */
	/* load disparity maps as .pgm images */
	void LoadXLeft(const char *file_name, bool flag); /* if flag is TRUE then larger */
	
	void SetDispRange(int disp_base, int disp_max);

	float GetK(); /* compute statistics of data_penalty */

	/* Parameters of KZ2 algorithm. */
	/* Description is in the config file */
	struct Parameters
	{
		/********** data term for KZ2 **********/
		enum { L1, L2 } data_cost;
		int				denominator; /* data term is multiplied by denominator.  */
									 /* Equivalent to using lambda1/denominator, */
									 /* lambda2/denominator, K/denominator       */

		/********** smoothness term for KZ2 **********/
		int				I_threshold2; /* intensity threshold for KZ2 */
		int				lambda1, lambda2;

		/********** penalty for an assignment being inactive for KZ2 **********/
		int				K;

		/********** iteration parameters for KZ2 **********/
		int				iter_max;
		bool			randomize_every_iteration;

	};
	void SetParameters(Parameters *params);

	/* algorithms */
	void KZ2();

private:
	/************** BASIC DATA *****************/
	Coord			im_size;					/* image dimensions */
	GrayImage		im_left, im_right;			/* original images */
	RGBImage		im_color_left, im_color_right;	/* original color images */
	GrayImage		im_left_min, im_left_max,	/* contain range of intensities */
					im_right_min, im_right_max; /* based on intensities of neighbors */
	RGBImage		im_color_left_min, im_color_left_max,
					im_color_right_min, im_color_right_max;
	int			disp_base, disp_max, disp_size;	/* range of disparities */
#define OCCLUDED 255
	IntImage		x_left, x_right;
	/*
		disparity map
		IMREF(x_..., p)==OCCLUDED means that 'p' is occluded
		if l - pixel in the left image, r - pixel in the right image, then
		r == l + Coord(IMREF(x_left,  l), l.y)
		l == r + Coord(IMREF(x_right, r), r.y)
	*/
	Parameters		params;

	/********* INTERNAL VARIABLES **************/
	int				E;					/* current energy */
	IntImage vars0, varsA; /* Variables corresponding to nodes */

	/********* INTERNAL FUNCTIONS **************/

	void		Run();
	void		InitSubPixel();

	/* data penalty functions */
	int			data_penalty_gray (Coord l, Coord r);
	int			data_penalty_color(Coord l, Coord r);

	/* smoothness penalty functions */
	int			smoothness_penalty_gray (Coord p, Coord np, int d);
	int			smoothness_penalty_color(Coord p, Coord np, int d);

	/**************** KZ2 ALGORITHM *************/
	int			data_occlusion_penalty(Coord l, Coord r);
	int			smoothness_penalty(Coord p, Coord np, int d);
	int			ComputeEnergy();			/* computes current energy */
	void		ExpansionMove(int a); /* computes min a-expansion */

    /**************** Graph construction ********/
    void build_nodes     (Energy* e, Coord p, int a);
    void build_uniqueness(Energy* e, Coord p, int a);
    void build_smoothness(Energy* e, Coord p, Coord np, int a);
};

#define INFINITY 10000		/* infinite capacity */

#endif
