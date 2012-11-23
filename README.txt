##################################################################
#                                                                #
#       MATCH - software for computing dense correspondence      #
#       (disparity map) between two images using graph cuts      #
#                        Version 3.4                             #
#       http://www.cs.cornell.edu/People/vnk/recon.html          #
#                                                                #
#       Vladimir N. Kolmogorov                                   #
#       v.kolmogorov@cs.ucl.ac.uk               2001-2003        #
#                                                                #
##################################################################



##################################################################

1. Introduction.

This software implements stereo algorithms described in the following papers:

	Vladimir Kolmogorov and Ramin Zabih
	"Multi-Camera scene Reconstruction via Graph Cuts"
	In: European Conference on Computer Vision, May 2002.

	Vladimir Kolmogorov and Ramin Zabih
	"Computing Visual Correspondence with Occlusions using Graph Cuts"
	In: International Conference on Computer Vision, July 2001.
and
	Yuri Boykov, Olga Veksler and Ramin Zabih
	"Markov Random Fields with Efficient Approximations"
	In: IEEE Computer Vision and Pattern Recognition Conference, June 1998.	

The first algorithm is referred to as 'KZ1', the second as 'KZ2' and the third as 'BVZ'.

The input are two images in .pgm or .ppm formats, and the output
is a disparity map registered with respect to the left image. KZ1 and BVZ
compute disparities for every pixel in the image, while KZ2 marks explicitly
pixels that are not visible in the right image as 'OCCLUDED'.

Note that KZ1 and KZ2 algorithms can be viewed as special cases of a more
general algorithm desribed in

	Vladimir Kolmogorov, Ramin Zabih and Steven Gortler
	"Generalized Multi-Camera scene Reconstruction Using Graph Cuts"
	In: Fourth International Workshop on Energy Minimization Methods in
        Computer Vision and Pattern Recognition, July 2003. 

More details can also be found in my PhD thesis (to appear).

Enclosed is also the 'head' dataset which is a courtesy
of Y. Ohta and Y. Nakamura from the University of Tsukuba, Japan.

This software can be used only for research purposes.

Please send any feedback to vnk@cs.cornell.edu.

##################################################################

2. Maxflow algorithm.

This software includes a new implementation of the maxflow algorithm
described in

	Yuri Boykov and Vladimir Kolmogorov
	"An Experimental Comparison of Min-Cut/Max-Flow Algorithms
	for Energy Minimization in Computer Vision"
	In: Third International Workshop on Energy Minimization Methods
	in Computer Vision and Pattern Recognition, September 2001.

This algorithm was originally developed at Siemens.

There are two versions of the algorithm using different
graph representations (adjacency list and forward star).
The former one (which is the default) uses more than twice as
much memory as the latter one but is 10-20% faster.
To switch to the latter version, copy all files from the
directory 'maxflow/forward_star' to the directory 'maxflow'.
See maxflow/README.TXT for more details.

##################################################################

3. Compiling the program.

The directory 'win' contains the project file 'match.dsw' for Visual C++ 6.0.
The directory 'unix' contains a makefile for compiling it under unix;
to compile it, run
	cd unix
	make

(tested under SunOS 5.8 and RedHat Linux 7.0).

##################################################################

4. Running the program.

Usage:
   Windows       bin\match.exe <config_file>
   Unix          bin/match <config_file>

The configuration file contains a sequence of commands
that the program executes. The file 'match.txt' contains
a description of all possible commands. Files 'kz1.txt', 'kz2.txt'
and 'bvz.txt' contain simple sequences of commands for running
corresponding algorithms on the head image pair from Tsukuba University.
All methods require a regularization parameter 'lambda' which
should be set differently for different images. An automatic method
for choosing this parameter for algorithms KZ1 and KZ2 is also supported.
It is based on some heuristic for estimating noise in the images.

While the default number of iterations is INFINITY (i.e., algorithms
run until convergence), it is reasonable to set it to 2 or 3;
results after the first two iterations are very close to final
results. 

##################################################################

5. Functions.

The program maintains two disparity maps - one for the left 
image and one for the right image. They are called 'unique'
if they correspond to each other: if a pixel 'p' in the left
image corresponds to a pixel 'q' in the right image according
to the left disparity map, then 'q' corresponds to 'p'
according to the right disparity map, and vice versa. This
also means that each pixel corresponds to at most one pixel
in the other image. There are also pixels that do not correspond
to any pixel; they are labeled as 'OCCLUDED'.

Besides KZ1, KZ2 and BVZ there are other functions for manipulating
disparity maps. They are:

CLEAR            clears disparity maps (labels all pixels as 'OCCLUDED')
SWAP_IMAGES      swaps left and right images
MAKE_UNIQUE      converts left disparity map into unique left and right
                 disparity maps using the following techique: if several
                 pixels in the left image are mapped to the same pixel
                 in the right image only one of them is kept
                 (for stereo, the leftmost pixel)
CROSS_CHECK      uses both left and right disparity maps for computing
                 unique disparity maps. A disparity for a pixel 'p' in
                 the left image is kept only if 'p' corresponds to 'q'
                 and 'q' corresponds to 'p'
FILL_OCCLUSIONS  assigns to occluded pixels in the left image the disparity
                 of the closest non-occluded left neighbor lying on the same                  scanline
CORR             computes the left disparity map using correlation algorithm
                 with L1 or L2 distance (useful if you want to determine
                 the range of disparities)


As an example, correlation cross-checking is implemented by the following
sequence of commands:

CORR
SWAP_IMAGES
CORR
SWAP_IMAGES
CROSS_CHECK

The initial disparity map for KZ method must be unique. If a previous
function did not compute a unique disparity map, then MAKE_UNIQUE is
automatically called before running KZ.

##################################################################

6. Energy for KZ1, KZ2 and BVZ.

There are several parameters describing energy functions for KZ1, KZ2 and
BVZ algorithms.
Data term for all algorithms depends on two parameters: boolean variable
'sub_pixel' and variable 'data_cost' which can be either 'L1' or 'L2'.
If variable 'sub_pixel' is set, then data term is computed as described in

	Stan Birchfield and Carlo Tomasi
	"A pixel dissimilarity measure that is insensitive to image sampling"
	PAMI 20(4):401-406, April 98

with one distinction: intensity intervals for a pixels are computed
from 4 neighbors rather than 2.
For color images the data term is an average over 3 components.

By default 'sub_pixel' is set and 'data_cost' is 'L2'.

Other parameters are described in the file 'match.txt'.

##################################################################

7. BVZ method.

BVZ method implemented here allows for pixels to have the special
label 'OCCLUDED'. There is a fixed penalty for having this label.
If this penalty is INFINITY then the algorithm is equivalent
to the method described in the original paper.

A graph construction also differs from the original publication.
The paper uses auxilary nodes for implementing certain
binary interactions. This implementation uses a different graph
construction given in

	Vladimir Kolmogorov and Ramin Zabih
	"What Energy Functions can be Minimized via Graph Cuts?"
	In: European Conference on Computer Vision, May 2002.

This construction is about 20% faster on Tsukuba pair than the
original one.

