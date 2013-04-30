/* graph.h */
/*
	version 3.02

	This software library implements the maxflow algorithm
	described in

		"An Experimental Comparison of Min-Cut/Max-Flow Algorithms for Energy Minimization in Vision."
		Yuri Boykov and Vladimir Kolmogorov.
		In IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI), 
		September 2004

	This algorithm was developed by Yuri Boykov and Vladimir Kolmogorov
	at Siemens Corporate Research. To make it available for public use,
	it was later reimplemented by Vladimir Kolmogorov based on open publications.

	If you use this software for research purposes, you should cite
	the aforementioned paper in any resulting publication.
*/

/*
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
	typedef enum
	{
		SOURCE	= 0,
		SINK	= 1
	} termtype; // terminals 
	typedef int node_id;
    typedef int arc_id;

	/////////////////////////////////////////////////////////////////////////
	//                     BASIC INTERFACE FUNCTIONS                       //
	//              (should be enough for most applications)               //
	/////////////////////////////////////////////////////////////////////////

	// Constructor. 
	// The argument is the pointer to a function which will be called 
	// if an error occurs; an error message is passed to this function. 
	// If this argument is omitted, exit(1) will be called.
	Graph(void (*err_function)(const char *) = NULL);

	// Destructor
	virtual ~Graph();

	// Adds node to the graph. First call returns 0, second 1, and so on. 
	node_id add_node();

	// Adds two edges between 'i' and 'j' with the weights 'cap' and 'rev_cap'
	void add_edge(node_id i, node_id j, captype cap, captype rev_cap);

	// Adds new edges 'SOURCE->i' and 'i->SINK' with corresponding weights.
	// Can be called multiple times for each node.
	// Weights can be negative.
	// NOTE: the number of such edges is not counted in edge_num_max.
	//       No internal memory is allocated by this call.
	void add_tweights(node_id i, tcaptype cap_source, tcaptype cap_sink);

	// Computes the maxflow. Can be called several times.
	flowtype maxflow();

	// After the maxflow is computed, this function returns to which
	// segment the node 'i' belongs (Graph<captype,tcaptype,flowtype>::SOURCE or Graph<captype,tcaptype,flowtype>::SINK).
	//
	// Occasionally there may be several minimum cuts. If a node can be assigned
	// to both the source and the sink, then default_segm is returned.
	termtype what_segment(node_id i, termtype default_segm = SOURCE);

private:
	struct node;
	struct arc;

	struct node
	{
		arc_id first; // first outgoing arc
		arc* parent;  // initial path to root (a terminal node) if in tree
		node* next;   // pointer to the next active node (itself if last node)
		int			TS;			// timestamp showing when DIST was computed
		int			DIST;		// distance to the terminal
		termtype	term;	// flag showing whether the node is in the source or in the sink tree (if parent!=NULL)

		tcaptype	tr_cap;		// if tr_cap > 0 then tr_cap is residual capacity of the arc SOURCE->node
								// otherwise         -tr_cap is residual capacity of the arc node->SINK 

	};

	struct arc
	{
		node_id	head;  // node the arc points to
		arc_id next;   // next arc with the same originating node
		arc_id sister; // reverse arc

		captype	r_cap; // residual capacity
	};

	struct nodeptr
	{
		node    	*ptr;
		nodeptr		*next;
	};
	static const int NODEPTR_BLOCK_SIZE = 128;

    std::vector<node> nodes;
    std::vector<arc> arcs;

	int					node_num;

	DBlock<nodeptr>		*nodeptr_block;

	void	(*error_function)(const char *);	// this function is called if a error occurs,
										// with a corresponding error message
										// (or exit(1) is called if it's NULL)

	flowtype			flow;		// total flow

	/////////////////////////////////////////////////////////////////////////

	node				*queue_first, *queue_last;	// list of active nodes
	nodeptr				*orphan_first, *orphan_last;		// list of pointers to orphans
	int					TIME;								// monotonically increasing global counter

	/////////////////////////////////////////////////////////////////////////

	// functions for processing active list
	void set_active(node *i);
	node *next_active();

	// functions for processing orphans list
	void set_orphan(node* i);  // add to the end of the list

	void maxflow_init();
	void augment(arc *middle_arc);
	void process_source_orphan(node *i);
	void process_sink_orphan(node *i);

	void test_consistency(node* current_node=NULL); // debug function
};

///////////////////////////////////////
// Implementation - inline functions //
///////////////////////////////////////

template <typename captype, typename tcaptype, typename flowtype> 
	inline typename Graph<captype,tcaptype,flowtype>::node_id Graph<captype,tcaptype,flowtype>::add_node()
{
	node n = {-1, 0, 0, 0, 0, SOURCE, 0};
	node_id i = static_cast<node_id>(nodes.size());
    nodes.push_back(n);
	return i;
}

template <typename captype, typename tcaptype, typename flowtype> 
	inline void Graph<captype,tcaptype,flowtype>::add_tweights(node_id i, tcaptype cap_source, tcaptype cap_sink)
{
	tcaptype delta = nodes[i].tr_cap;
	if (delta > 0) cap_source += delta;
	else           cap_sink   -= delta;
	flow += (cap_source < cap_sink) ? cap_source : cap_sink;
	nodes[i].tr_cap = cap_source - cap_sink;
}

template <typename captype, typename tcaptype, typename flowtype> 
	inline void Graph<captype,tcaptype,flowtype>::add_edge(node_id _i, node_id _j, captype cap, captype rev_cap)
{
	assert(_i != _j);
	assert(cap >= 0);
	assert(rev_cap >= 0);

    arc_id ij=static_cast<arc_id>(arcs.size()), ji=ij+1;

    arc a     = {_j, nodes[_i].first, ji, cap};
    arc a_rev = {_i, nodes[_j].first, ij, rev_cap};

	nodes[_i].first = ij;
	nodes[_j].first = ji;
    arcs.push_back(a);
    arcs.push_back(a_rev);
}

template <typename captype, typename tcaptype, typename flowtype> 
	inline typename Graph<captype,tcaptype,flowtype>::termtype Graph<captype,tcaptype,flowtype>::what_segment(node_id i, termtype default_segm)
{
	return (nodes[i].parent? nodes[i].term: default_segm);
}

#include "graph.cpp"
#include "maxflow.cpp"

#endif
