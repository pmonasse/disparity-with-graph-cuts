KZ2 - Kolmogorov and Zabih’s graph cuts stereo matching algorithm

This software is linked to the IPOL article
"Kolmogorov and Zabih’s Graph Cuts Stereo Matching Algorithm"
by Vladimir Kolmogorov, Pascal Monasse, Pauline Tan
    http://www.ipol.im/pub/pre/97/
It implements the algorithm of the article:
	Vladimir Kolmogorov and Ramin Zabih
	"Computing Visual Correspondence with Occlusions using Graph Cuts"
	In: International Conference on Computer Vision, July 2001.


This program is written by Pascal Monasse <monasse@imagine.enpc.fr> and
distributed under the terms of the LGPLv3 license.
Based on original code "Match 3.4" by Vladimir Kolmogorov <vnk@cs.cornell.edu>
http://pub.ist.ac.at/~vnk/software.html

Version 1.0-rc1 released on 2013/09/17

Future releases and updates:

Build
-----
Prerequisites: CMake version 2.6 or later

- Unix, MacOS, Windows with MinGW:
$ cd /path_to_KZ2/
$ mkdir Build && cd Build && cmake -DCMAKE_BUILD_TYPE:bool=Release ..
$ make

CMake tries to find libPNG and libTIFF on your system, though neither is mandatory. They need to come with header files, which are provided by "...-dev" packages under Linux Debian or Ubuntu.

- Windows with Microsoft Visual Studio (MSVC):
1. Launch CMake, input as source code location the KZ2 folder and use a new folder for binaries.
2. Press 'Configure' then 'Generate' buttons. Choose your MSVC version. When done, you can quit CMake.
3. In the binary folder, open the solution file KZ2.sln, this launches MSVC. Select 'Release' mode, then 'Build solution' (F7) will build your program.

Test:
$ bin/KZ2 ../images/scene_l.png ../images/scene_r.png -15 0 disp.tif -o disp.png
Input images must be epipolar rectified, the disparity range depends on the pair. The output images are here:
- disp.tif (float TIFF image, able to contain negative values and with occluded pixels as NaN, Not A Number) and
- disp.png, representing the same image directly viewable, with gray levels for disparity and cyan color for occluded pixels.
The latter is useful as many image viewers do not understand float TIFF.

Usage
-----
bin/KZ2 [options] im1.png im2.png dMin dMax [dispMap.tif]
General options:
 -i,--iter_max iter: max number of iterations
 -o,--output disp.png: scaled disparity map
 -r,--random: random alpha order at each iteration
Options for cost:
 -c,--data_cost dist: L1 or L2
 -l,--lambda lambda: value of lambda (smoothness)
 --lambda1 l1: smoothness cost not across edge
 --lambda2 l2: smoothness cost across edge
 -t,--threshold2 thres: intensity diff for 'edge'
 -k k: cost for occlusion
If no output is given (neither dispMap.tif nor -o option), the program just displays the recommended computed values for K and lambda.

Files
-----
Only files with (*) are reviewed.
README.txt
CMakeLists.txt
images/scene_l.png
images/scene_r.png
src/CMakeLists.txt
src/cmdLine.h
src/io_tiff.h
src/io_tiff.c
src/io_png.h
src/io_png.c
src/nan.h
src/image.h (*)
src/image.cpp (*)
src/kz2.cpp (*)
src/match.h (*)
src/match.cpp (*)
src/data.cpp (*)
src/statistics.cpp (*)
src/main.cpp (*)
src/energy/README.TXT
src/energy/energy.h (*)
src/maxflow/README.TXT
src/maxflow/graph.h (*)
src/maxflow/graph.cpp (*)
src/maxflow/maxflow.cpp (*)

LIMITATIONS
-----------
The software is a bit slower (10-20%) than the original code Match of V. Kolmogorov due to memory management of the graph. Match allocates sets of nodes and edges with malloc/realloc, and stores directly pointers to link nodes and edges. It has thus to adjust the pointers when realloc changes array address. This results in ugly code with offsets to pointers but:
- Match has faster max-flow computation since it uses pointers to follow paths while KZ2 uses index in std::vector.
- It was noticed that with the same allocation policy, using C's alloc/realloc is faster than standard allocator of std::vector using C++'s new. The reason is a mistery since elements have no constructor/destructor.
To alleviate the latter defect, a preset amount of memory is pre-allocated for node and edge arrays: 2n edges and 11n edges, with n the number of pixels (see Match::ExpansionMove in kz2.cpp). These were determined experimentally to be adapted for a usual graph, but is less elegant and less efficient than on-demand allocation.
