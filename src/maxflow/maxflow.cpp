#ifdef GRAPH_H

// special constants for node->parent
#define TERMINAL (arc*)1 // arc to terminal
#define ORPHAN   (arc*)2 // arc to orphan

#define INFINITE_D ((int)(((unsigned)-1)/2)) // infinite distance to terminal

/// Mark node as active.
/// i->next points to the next active node (or itself, if last).
/// i->next is 0 iff i is not in the list.
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

/// Return the next active node and remove from the list.
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

/// Push flow through path from source to sink passing through middle_arc.
template <typename captype, typename tcaptype, typename flowtype>
void Graph<captype,tcaptype,flowtype>::augment(arc* middle_arc)
{
    /* 1. Finding bottleneck capacity */
    /* 1a - the source tree */
    tcaptype bottleneck = middle_arc->cap;
    node_id i=arcs[middle_arc->sister].head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        if (bottleneck > arcs[a->sister].cap)
            bottleneck = arcs[a->sister].cap;
        i = a->head;
    } while(true);
    if (bottleneck > nodes[i].cap)
        bottleneck = nodes[i].cap;
    /* 1b - the sink tree */
    i=middle_arc->head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        if (bottleneck > a->cap)
            bottleneck = a->cap;
        i=a->head;
    } while(true);
    if (bottleneck > -nodes[i].cap)
        bottleneck = -nodes[i].cap;

    /* 2. Augmenting */
    flow += bottleneck;
    /* 2a - the source tree */
    arcs[middle_arc->sister].cap += bottleneck;
    middle_arc->cap -= bottleneck;
    i=arcs[middle_arc->sister].head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        a->cap += bottleneck;
        arcs[a->sister].cap -= bottleneck;
        if (!arcs[a->sister].cap)
            set_orphan(&nodes[i]);
        i=a->head;
    } while(true);
    nodes[i].cap -= bottleneck;
    if (!nodes[i].cap)
        set_orphan(&nodes[i]);
    /* 2b - the sink tree */
    i=middle_arc->head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        arcs[a->sister].cap += bottleneck;
        a->cap -= bottleneck;
        if (!a->cap)
            set_orphan(&nodes[i]);
        i=a->head;
    } while(true);
    nodes[i].cap += bottleneck;
    if (!nodes[i].cap)
        set_orphan(&nodes[i]);
}

/// Try to reconnect orphan i to its original tree.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::process_source_orphan(node* i)
{
    int d_min=INFINITE_D;

    /* trying to find a new parent */
    i->parent = 0;
    for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next)
        if (arcs[arcs[a0].sister].cap) {
            node* j = &nodes[arcs[a0].head];
            if (j->term==SOURCE && j->parent!=0) {
                /* checking the origin of j */
                int d = 0;
                while (true) {
                    if (j->ts == time) {
                        d += j->dist;
                        break;
                    }
                    arc* a = j->parent;
                    d ++;
                    if (a==TERMINAL) {
                        j->ts = time;
                        j->dist = 1;
                        break;
                    }
                    if (a==ORPHAN || a==0) { d = INFINITE_D; break; }
                    j = &nodes[a->head];
                }
                if (d<INFINITE_D) { /* j originates from the source - done */
                    if (d<d_min) {
                        i->parent = &arcs[a0];
                        d_min = d;
                    }
                    /* set marks along the path */
                    for (j=&nodes[arcs[a0].head]; j->ts!=time;
                         j=&nodes[j->parent->head]) {
                        j->ts = time;
                        j->dist = d--;
                    }
                }
            }
        }

    if (i->parent) {
        i->ts = time;
        i->dist = d_min + 1;
    } else { // no parent is found, process neighbors
        for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next) {
            node* j = &nodes[arcs[a0].head];
            arc* a;
            if (j->term==SOURCE && (a=j->parent)) {
                if (a!=TERMINAL && a!=ORPHAN && &nodes[a->head]==i)
                    set_orphan(j);
                if (arcs[arcs[a0].sister].cap)
                    set_active(j);
            }
        }
    }
}

/// Try to reconnect orphan i to its original tree.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::process_sink_orphan(node *i)
{
    int d_min=INFINITE_D;

    /* trying to find a new parent */
    i->parent = 0;
    for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next)
        if (arcs[a0].cap) {
            node* j = &nodes[arcs[a0].head];
            if (j->term==SINK && j->parent!=0) {
                /* checking the origin of j */
                int d = 0;
                while (true) {
                    if (j->ts == time) {
                        d += j->dist;
                        break;
                    }
                    arc* a = j->parent;
                    d ++;
                    if (a==TERMINAL) {
                        j->ts = time;
                        j->dist = 1;
                        break;
                    }
                    if (a==ORPHAN || a==0) { d = INFINITE_D; break; }
                    j = &nodes[a->head];
                }
                if (d<INFINITE_D) { /* j originates from the sink - done */
                    if (d<d_min) {
                        i->parent = &arcs[a0];
                        d_min = d;
                    }
                    /* set marks along the path */
                    for (j=&nodes[arcs[a0].head]; j->ts!=time;
                         j=&nodes[j->parent->head]) {
                        j->ts = time;
                        j->dist = d --;
                    }
                }
            }
        }

    if (i->parent) {
        i->ts = time;
        i->dist = d_min + 1;
    } else { // no parent is found, process neighbors
        for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next) {
            node* j = &nodes[arcs[a0].head];
            arc* a;
            if (j->term==SINK && (a=j->parent)) {
                if (a!=TERMINAL && a!=ORPHAN && &nodes[a->head]==i)
                    set_orphan(j);
                if (arcs[a0].cap)
                    set_active(j);
            }
        }
    }
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
                if(i->term==SINK)
                    a = arcs[a].sister;
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
            if (i->term==SINK) process_sink_orphan(i);
            else               process_source_orphan(i);
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
        if (!i->parent) // i has become orphan
            i=0;
    }

    delete nodeptr_block;
    nodeptr_block = 0;

    return flow;
}

#undef TERMINAL
#undef ORPHAN
#endif
