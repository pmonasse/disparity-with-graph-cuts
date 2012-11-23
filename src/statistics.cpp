/* statistics.cpp */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001-2003. */

#include <algorithm>
#include <stdio.h>
#include "match.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/************************************************************/
/************************************************************/
/************************************************************/

/*
	Heuristic for selecting parameter 'K'
	Details are described in Kolmogorov's thesis
*/
   

float Match::GetK()
{
	int i = disp_size;
	int k = (i+2)/4; /* 0.25 times the number of disparities */
	if(k<3) k=3;
    if(k>i) k=i;

	int* array = new int[k];
	if (!array) { fprintf(stderr, "GetK: Not enough memory!\n"); exit(1); }

	int sum=0, num=0;

	int xmin = 0-MIN(disp_base, 0), xmax=im_size.x-MAX(disp_max,0);
	Coord p;
	for(p.y=0; p.y<im_size.y; p.y++)
	for(p.x=xmin; p.x<xmax; p.x++) {
		/* compute k'th smallest value among data_penalty(p, p+d) for all d */
		for(int i=0, d=disp_base; d<=disp_max; d++) {
			int delta = (im_left?
                         data_penalty_gray (p,p+d):
                         data_penalty_color(p,p+d));
			if(i<k) array[i++] = delta;
			else for (i=0; i<k; i++)
				if(delta<array[i])
                    std::swap(delta, array[i]);
		}
		sum += *std::max_element(array, array+k);
		num ++;
	}

	delete [] array;
	if(num==0) { fprintf(stderr, "GetK: Not enough samples!\n"); exit(1); }
	if(sum==0) { fprintf(stderr, "GetK failed: K is 0!\n"); exit(1); }

	float K = ((float)sum)/num;
	printf("Computing statistics: data_penalty noise K=%f\n", K);
	return K;
}
