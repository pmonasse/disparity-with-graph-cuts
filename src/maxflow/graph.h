/*
  version 3.02

  This software library implements the maxflow algorithm described in

  "An Experimental Comparison of Min-Cut/Max-Flow Algorithms for
   Energy Minimization in Vision."
  Yuri Boykov and Vladimir Kolmogorov.
  In IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI), 
  September 2004

  This algorithm was developed by Yuri Boykov and Vladimir Kolmogorov
  at Siemens Corporate Research. To make it available for public use,
  it was later reimplemented by Vladimir Kolmogorov based on open publications.

  If you use this software for research purposes, you should cite
  the aforementioned paper in any resulting publication.

  For description, license, example usage see README.TXT.
*/

#ifndef GRAPH_H
#define GRAPH_H

#include <string.h>
#include <assert.h>
#include <vector>
#include "block.h"

// captype: type of edge capacities (excluding t-links)
// tcaptype: type of t-links (edges between nodes and terminals)
// flowtype: type of total flow
template <typename captype, typename tcaptype, typename flowtype> class Graph
{
public:
    typedef enum { SOURCE=0, SINK=1} termtype; // terminals 
    typedef int node_id;
    typedef int arc_id;

    Graph();
    virtual ~Graph();

    node_id add_node();
    void add_edge(node_id i, node_id j, captype cap, captype rev_cap);
    void add_tweights(node_id i, tcaptype cap_source, tcaptype cap_sink);

    flowtype maxflow();
    termtype what_segment(node_id i, termtype default_segm=SOURCE);

private:
    struct node;
    struct arc;

    struct node {
        arc_id first;    // first outgoing arc
        arc* parent;     // initial path to root (a terminal node) if in tree
        node* next;      // pointer to next active node (itself if last node)
        int TS;          // timestamp showing when DIST was computed
        int DIST;        // distance to the terminal
        termtype term;   // source or sink tree? (only if parent!=NULL)
        tcaptype tr_cap; // capacity of arc SOURCE->node if >0, or node->SINK
    };

    struct arc {
        node_id head;  // node the arc points to
        arc_id next;   // next arc with the same originating node
        arc_id sister; // reverse arc
        captype r_cap; // residual capacity
    };

    struct nodeptr {
        node* ptr;
        nodeptr* next;
    };

    std::vector<node> nodes;
    std::vector<arc> arcs;

    flowtype flow; // total flow

    node *queue_first, *queue_last; // list of active nodes
    static const int NODEPTR_BLOCK_SIZE = 128;
    DBlock<nodeptr>* nodeptr_block;
    nodeptr *orphan_first, *orphan_last; // list of pointers to orphans
    int TIME;// monotonically increasing global counter

    // functions for processing active list
    void set_active(node* i);
    node* next_active();

    // functions for processing orphans
    void set_orphan(node* i);  // add to the end of the list
    void process_source_orphan(node* i);
    void process_sink_orphan(node* i);

    void maxflow_init();
    void augment(arc* middle_arc);
};

#include "graph.cpp"
#include "maxflow.cpp"

#endif
