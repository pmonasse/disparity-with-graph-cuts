#ifdef GRAPH_H

/// Constructor
/// The argument is the pointer to a function which will be called 
/// if an error occurs; an error message is passed to this function. 
/// If this argument is omitted, exit(1) will be called.
template <typename captype, typename tcaptype, typename flowtype> 
Graph<captype, tcaptype, flowtype>::Graph()
: nodes(), arcs(), flow(0),
  queue_first(0),queue_last(0), nodeptr_block(0), orphan_first(0),orphan_last(0)
{}

/// Destructor
template <typename captype, typename tcaptype, typename flowtype> 
Graph<captype,tcaptype,flowtype>::~Graph()
{
    delete nodeptr_block;
}

/// Add node to the graph. First call returns 0, second 1, and so on. 
template <typename captype, typename tcaptype, typename flowtype> 
typename Graph<captype,tcaptype,flowtype>::node_id
Graph<captype,tcaptype,flowtype>::add_node()
{
    node n = {-1, 0, 0, 0, 0, SOURCE, 0};
    node_id i = static_cast<node_id>(nodes.size());
    nodes.push_back(n);
    return i;
}

/// Add two edges between 'i' and 'j' with the weights 'cap' and 'rev_cap'
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::add_edge(node_id _i, node_id _j,
                                                captype cap, captype rev_cap)
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

/// Adds new edge 'SOURCE->i' and 'i->SINK' with corresponding weights.
/// Can be called multiple times for each node.
/// Weights can be negative.
/// No internal memory is allocated by this call.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::add_tweights(node_id i,
                                                    tcaptype cap_source,
                                                    tcaptype cap_sink)
{
    tcaptype delta = nodes[i].cap;
    if (delta > 0) cap_source += delta;
    else           cap_sink   -= delta;
    flow += (cap_source < cap_sink) ? cap_source : cap_sink;
    nodes[i].cap = cap_source - cap_sink;
}

/// After the maxflow is computed, this function returns to which segment the
/// node 'i' belongs (SOURCE or SINK).
/// Occasionally there may be several minimum cuts. If a node can be assigned
/// to both the source and the sink, then default_segm is returned.
template <typename captype, typename tcaptype, typename flowtype> 
typename Graph<captype,tcaptype,flowtype>::termtype
Graph<captype,tcaptype,flowtype>::what_segment(node_id i, termtype default_segm)
{
    return (nodes[i].parent? nodes[i].term: default_segm);
}

#endif
