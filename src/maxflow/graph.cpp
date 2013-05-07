#ifdef GRAPH_H

/// Constructor
template <typename captype, typename tcaptype, typename flowtype> 
Graph<captype, tcaptype, flowtype>::Graph()
: nodes(), arcs(), flow(0), activeBegin(0),activeEnd(0), orphans(), time(0)
{}

/// Destructor
template <typename captype, typename tcaptype, typename flowtype> 
Graph<captype,tcaptype,flowtype>::~Graph()
{}

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

/// Add two edges between 'i' and 'j' with the weights 'capij' and 'capji'
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::add_edge(node_id i, node_id j,
                                                captype capij, captype capji)
{
    assert(0<=i && i<(int)nodes.size());
    assert(0<=j && j<(int)nodes.size());
    assert(i != j);
    assert(capij >= 0);
    assert(capji >= 0);

    arc_id ij=static_cast<arc_id>(arcs.size()), ji=ij+1;

    arc aij = {j, nodes[i].first, ji, capij};
    arc aji = {i, nodes[j].first, ij, capji};

    nodes[i].first = ij;
    nodes[j].first = ji;
    arcs.push_back(aij);
    arcs.push_back(aji);
}

/// Adds new edge 'SOURCE->i' and 'i->SINK' with corresponding weights.
/// Can be called multiple times for each node.
/// Weights can be negative.
/// No internal memory is allocated by this call.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::add_tweights(node_id i,
                                                    tcaptype capSource,
                                                    tcaptype capSink)
{
    tcaptype delta = nodes[i].cap;
    if (delta > 0) capSource += delta;
    else           capSink   -= delta;
    flow += (capSource < capSink) ? capSource : capSink;
    nodes[i].cap = capSource - capSink;
}

/// After the maxflow is computed, this function returns to which segment the
/// node 'i' belongs (SOURCE or SINK).
/// Occasionally there may be several minimum cuts. If a node can be assigned
/// to both the source and the sink, then default def is returned.
template <typename captype, typename tcaptype, typename flowtype> 
typename Graph<captype,tcaptype,flowtype>::termtype
Graph<captype,tcaptype,flowtype>::what_segment(node_id i, termtype def) const
{
    return (nodes[i].parent? nodes[i].term: def);
}

#endif
