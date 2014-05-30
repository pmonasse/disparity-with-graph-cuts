/**
 * @file graph.cpp
 * @brief Graph structure
 * @author Vladimir Kolmogorov <vnk@cs.cornell.edu>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2001-2003, 2012-2014, Vladimir Kolmogorov, Pascal Monasse
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Pulic License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// Do not compile when not included from graph.h
#ifdef GRAPH_H

/// Constructor.
/// For efficiency, it is advised to give appropriate hint sizes.
template <typename captype, typename tcaptype, typename flowtype>
Graph<captype, tcaptype, flowtype>::Graph(int hintNbNodes, int hintNbArcs)
: nodes(), arcs(), flow(0), activeBegin(0),activeEnd(0), orphans(), time(0),
  TERMINAL(0), ORPHAN(0)
{
    nodes.reserve(hintNbNodes);
    arcs.reserve(hintNbArcs);
}

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

/// Add edge with infinite capacity from node 'i' to 'j'
template <typename captype, typename tcaptype, typename flowtype>
void Graph<captype,tcaptype,flowtype>::add_edge_infty(node_id i, node_id j)
{
    add_edge(i, j, std::numeric_limits<captype>::max(), 0);
}

/// Adds new edges 'SOURCE(s)->i' and 'i->SINK(t)' with corresponding weights.
/// Can be called multiple times for each node.
/// Weights can be negative.
/// No internal memory is allocated by this call.
template <typename captype, typename tcaptype, typename flowtype>
void Graph<captype,tcaptype,flowtype>::add_tweights(node_id i,
                                                    tcaptype capS,
                                                    tcaptype capT)
{
    assert(0<=i && i<(int)nodes.size());
    tcaptype delta = nodes[i].cap;
    if(delta > 0) capS += delta;
    else          capT -= delta;
    flow += (capS<capT)? capS: capT;
    nodes[i].cap = capS - capT;
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
