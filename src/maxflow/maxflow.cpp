#ifdef GRAPH_H

#include <limits>

// special constants for node->parent
#define TERMINAL (arc*)1 // arc to terminal
#define ORPHAN   (arc*)2 // arc to orphan

/// Mark node as active.
/// i->next points to the next active node (or itself, if last).
/// i->next is 0 iff i should not be considered in the list.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::set_active(node* i)
{
    if (!i->next) { // not yet in the list
        if (queue_last) queue_last->next = i;
        else            queue_first        = i;
        queue_last = i;
        i->next = i;
    }
}

/// Return the next active node and remove it from the list.
template <typename captype, typename tcaptype, typename flowtype> 
typename Graph<captype,tcaptype,flowtype>::node*
Graph<captype,tcaptype,flowtype>::next_active()
{
    while (true) {
        node* i=queue_first;
        if (!i)
            return 0;
        if (i->next == i) queue_first = queue_last = 0;
        else              queue_first = i->next;
        i->next = 0;
        if (i->parent) return i; // active iff it has a parent
    }
}

/// Set node as orphan.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::set_orphan(node* i)
{
    i->parent = ORPHAN;
    nodeptr* np = nodeptr_block->New();
    np->ptr = i;
    np->next = 0;
    if (orphan_last) orphan_last->next = np;
    else             orphan_first      = np;
    orphan_last = np;
}

/// Set active nodes at distance 1 from a terminal node.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::maxflow_init()
{
    queue_first = queue_last = 0;
    orphan_first = 0;

    time = 0;

    typename std::vector<node>::iterator it=nodes.begin();
    for (; it!=nodes.end(); ++it) {
        node* i= &(*it);
        i->next = 0;
        i->ts = time;
        if(i->cap == 0)
            i->parent = 0;
        else {
            i->term = (i->cap>0? SOURCE: SINK);
            i->parent = TERMINAL;
            set_active(i);
            i->dist = 1;
        }
    }
}

/// Find max flow that we can push from source to sink through midarc.
/// midarc must be oriented from source tree to sink tree.
template <typename captype, typename tcaptype, typename flowtype>
captype Graph<captype,tcaptype,flowtype>::find_bottleneck(arc* midarc)
{
    captype cap = midarc->cap;

    // source tree
    node_id i=arcs[midarc->sister].head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        if (cap > arcs[a->sister].cap)
            cap = arcs[a->sister].cap;
        i = a->head;
    } while(true);
    if (cap > nodes[i].cap)
        cap = nodes[i].cap;

    // sink tree
    i=midarc->head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        if (cap > a->cap)
            cap = a->cap;
        i=a->head;
    } while(true);
    if (cap > -nodes[i].cap)
        cap = -nodes[i].cap;

    return cap;
}

/// Push flow f through path from source to sink through midarc.
template <typename captype, typename tcaptype, typename flowtype>
void Graph<captype,tcaptype,flowtype>::push_flow(arc* midarc, captype f)
{
    flow += f;
    // source tree
    arcs[midarc->sister].cap += f;
    midarc->cap -= f;
    node_id i=arcs[midarc->sister].head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        a->cap += f;
        arcs[a->sister].cap -= f;
        if (!arcs[a->sister].cap)
            set_orphan(&nodes[i]);
        i=a->head;
    } while(true);
    nodes[i].cap -= f;
    if (!nodes[i].cap)
        set_orphan(&nodes[i]);

    // sink tree
    i=midarc->head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        arcs[a->sister].cap += f;
        a->cap -= f;
        if (!a->cap)
            set_orphan(&nodes[i]);
        i=a->head;
    } while(true);
    nodes[i].cap += f;
    if (!nodes[i].cap)
        set_orphan(&nodes[i]);
}

/// Push flow through path from source to sink passing through midarc.
template <typename captype, typename tcaptype, typename flowtype>
void Graph<captype,tcaptype,flowtype>::augment(arc* midarc)
{
    // Orient arc from source tree to sink tree
    if(nodes[midarc->head].term==SOURCE)
        midarc = &arcs[midarc->sister];

    captype bottleneck = find_bottleneck(midarc);
    push_flow(midarc, bottleneck);
}

/// Extend the tree to neighbor nodes of tree leaf i. If doing so reaches the
/// other tree, return the arc oriented from source tree to sink tree.
template <typename captype, typename tcaptype, typename flowtype>
typename Graph<captype,tcaptype,flowtype>::arc*
Graph<captype,tcaptype,flowtype>::grow_tree(node* i)
{
    for (arc_id a=i->first; a>=0; a=arcs[a].next)
        if (i->term==SOURCE? arcs[a].cap: arcs[arcs[a].sister].cap) {
            node* j = &nodes[arcs[a].head];
            if (!j->parent) {
                j->term = i->term;
                j->parent = &arcs[arcs[a].sister];
                j->ts = i->ts;
                j->dist = i->dist + 1;
                set_active(j);
            } else if (j->term!=i->term) {
                return &arcs[a];
            } else if (j->ts<=i->ts && j->dist>i->dist) {
                // heuristic: try shortening distance from j to terminal
                j->parent = &arcs[arcs[a].sister];
                j->ts = i->ts;
                j->dist = i->dist + 1;
            }
        }
    return 0;
}

/// Number of nodes of path from the root of the tree to node j.
/// Return max integer in case there is no path.
template <typename captype, typename tcaptype, typename flowtype> 
int Graph<captype,tcaptype,flowtype>::dist_to_root(node* j)
{
    int d = 2; // count nodes j and root
    for(arc* a; (a=j->parent)!=TERMINAL; d++, j=&nodes[a->head]) {
        if (a==ORPHAN || a==0)
            return std::numeric_limits<int>::max();
        if (j->ts == time)
            return d+j->dist-1; // -1: do not count root twice
    }
    j->ts = time;
    j->dist = 1;
    return d;
}

/// Try to reconnect orphan to its original tree.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::process_orphan(node* i)
{
    int dmin=std::numeric_limits<int>::max();

    i->parent = 0;
    for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next)
        if (i->term==SOURCE? arcs[arcs[a0].sister].cap: arcs[a0].cap) {
            node* j = &nodes[arcs[a0].head];
            if (j->term==i->term && j->parent) { // check origin of j
                int d = dist_to_root(j);
                if (d<std::numeric_limits<int>::max()) { // found root
                    if (d<dmin) {
                        i->parent = &arcs[a0];
                        i->ts = time;
                        i->dist = dmin = d;
                    }
                    for (j=&nodes[arcs[a0].head]; j->ts!=time;
                         j=&nodes[j->parent->head]) { // set marks along path
                        j->ts = time;
                        j->dist = d--;
                    }
                }
            }
        }

    if (!i->parent) { // no parent is found, process neighbors
        for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next) {
            node* j = &nodes[arcs[a0].head];
            arc* a;
            if (j->term==i->term && (a=j->parent)) {
                if (a!=TERMINAL && a!=ORPHAN && &nodes[a->head]==i)
                    set_orphan(j); // j child of i, becomes orphan
                if (j->term==SOURCE? arcs[arcs[a0].sister].cap: arcs[a0].cap)
                    set_active(j); // j in tree is neighbor, becomes active
            }
        }
    }
}

/// Try reconnecting orphans to their tree
template <typename captype, typename tcaptype, typename flowtype>
void Graph<captype,tcaptype,flowtype>::adopt_orphans()
{
    nodeptr* np;
    while ((np=orphan_first)!=0) {
        nodeptr* np_next = np->next;
        np->next = 0;

        while ((np=orphan_first)) {
            node* i = np->ptr;
            orphan_first = np->next;
            nodeptr_block->Delete(np);
            if (!orphan_first) orphan_last = 0;
            process_orphan(i);
        }

        orphan_first = np_next;
    }
}

/// Compute the maxflow.
template <typename captype, typename tcaptype, typename flowtype> 
flowtype Graph<captype,tcaptype,flowtype>::maxflow()
{
    nodeptr_block = new DBlock<nodeptr>(NODEPTR_BLOCK_SIZE);

    maxflow_init();

    // main loop
    for(node *i=0; i || (i=next_active());) {
        arc* a = grow_tree(i);
        ++time;
        if(!a) {
            i = 0;
            continue;
        }
        i->next = i; // set active: prevent adding again to active list
        augment(a);
        adopt_orphans();
        i->next = 0; // remove active flag
        if (!i->parent) // i could not be adopted
            i=0;
    }

    delete nodeptr_block;
    nodeptr_block = 0;

    return flow;
}

#undef TERMINAL
#undef ORPHAN
#endif
