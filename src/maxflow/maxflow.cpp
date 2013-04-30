#ifdef GRAPH_H

// special constants for node->parent
#define TERMINAL (arc*)1 // arc to terminal
#define ORPHAN   (arc*)2 // arc to orphan

#define INFINITE_D ((int)(((unsigned)-1)/2)) // infinite distance to terminal

/// Mark node as active.
/// i->next points to the next active node (or itself, if last).
/// i->next is NULL iff i is not in the list.
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
            return NULL;
        if (i->next == i) queue_first = queue_last = NULL;
        else              queue_first = i->next;
        i->next = NULL;
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
    np->next = NULL;
    if (orphan_last) orphan_last->next = np;
    else             orphan_first      = np;
    orphan_last = np;
}

/// Set active nodes at distance 1 from a terminal node.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::maxflow_init()
{
    queue_first = queue_last = NULL;
    orphan_first = NULL;

    TIME = 0;

    typename std::vector<node>::iterator it=nodes.begin();
    for (; it!=nodes.end(); ++it) {
        node* i= &(*it);
        i->next = NULL;
        i->TS = TIME;
        if(i->tr_cap == 0)
            i->parent = NULL;
        else {
            i->term = (i->tr_cap>0? SOURCE: SINK);
            i->parent = TERMINAL;
            set_active(i);
            i->DIST = 1;
        }
    }
}

/// Push flow through path from source to sink passing through middle_arc.
template <typename captype, typename tcaptype, typename flowtype>
void Graph<captype,tcaptype,flowtype>::augment(arc* middle_arc)
{
    /* 1. Finding bottleneck capacity */
    /* 1a - the source tree */
    tcaptype bottleneck = middle_arc->r_cap;
    node_id i=arcs[middle_arc->sister].head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        if (bottleneck > arcs[a->sister].r_cap)
            bottleneck = arcs[a->sister].r_cap;
        i = a->head;
    } while(true);
    if (bottleneck > nodes[i].tr_cap)
        bottleneck = nodes[i].tr_cap;
    /* 1b - the sink tree */
    i=middle_arc->head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        if (bottleneck > a->r_cap)
            bottleneck = a->r_cap;
        i=a->head;
    } while(true);
    if (bottleneck > -nodes[i].tr_cap) bottleneck = -nodes[i].tr_cap;

    /* 2. Augmenting */
    flow += bottleneck;
    /* 2a - the source tree */
    arcs[middle_arc->sister].r_cap += bottleneck;
    middle_arc->r_cap -= bottleneck;
    i=arcs[middle_arc->sister].head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        a->r_cap += bottleneck;
        arcs[a->sister].r_cap -= bottleneck;
        if (!arcs[a->sister].r_cap)
            set_orphan(&nodes[i]);
        i=a->head;
    } while(true);
    nodes[i].tr_cap -= bottleneck;
    if (!nodes[i].tr_cap)
        set_orphan(&nodes[i]);
    /* 2b - the sink tree */
    i=middle_arc->head;
    do {
        arc* a = nodes[i].parent;
        if (a == TERMINAL) break;
        arcs[a->sister].r_cap += bottleneck;
        a->r_cap -= bottleneck;
        if (!a->r_cap)
            set_orphan(&nodes[i]);
        i=a->head;
    } while(true);
    nodes[i].tr_cap += bottleneck;
    if (!nodes[i].tr_cap)
        set_orphan(&nodes[i]);
}

/// Try to reconnect orphan i to its original tree.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::process_source_orphan(node* i)
{
    arc* a0_min=NULL;
    int d_min=INFINITE_D;

    /* trying to find a new parent */
    for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next)
        if (arcs[arcs[a0].sister].r_cap) {
            node* j = &nodes[arcs[a0].head];
            if (j->term==SOURCE && j->parent!=0) {
                /* checking the origin of j */
                int d = 0;
                while (true) {
                    if (j->TS == TIME) {
                        d += j->DIST;
                        break;
                    }
                    arc* a = j->parent;
                    d ++;
                    if (a==TERMINAL) {
                        j->TS = TIME;
                        j->DIST = 1;
                        break;
                    }
                    if (a==ORPHAN) { d = INFINITE_D; break; }
                    j = &nodes[a->head];
                }
                if (d<INFINITE_D) { /* j originates from the source - done */
                        
                    if (d<d_min) {
                        a0_min = &arcs[a0];
                        d_min = d;
                    }
                    /* set marks along the path */
                    for (j=&nodes[arcs[a0].head]; j->TS!=TIME; j=&nodes[j->parent->head]) {
                        j->TS = TIME;
                        j->DIST = d --;
                    }
                }
            }
        }

    if ((i->parent = a0_min) != 0) {
        i->TS = TIME;
        i->DIST = d_min + 1;
    } else { // no parent is found, process neighbors
        for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next) {
            node* j = &nodes[arcs[a0].head];
            arc* a;
            if (j->term==SOURCE && (a=j->parent)) {
                if (arcs[arcs[a0].sister].r_cap) set_active(j);
                if (a!=TERMINAL && a!=ORPHAN && &nodes[a->head]==i)
                    set_orphan(j);
            }
        }
    }
}

/// Try to reconnect orphan i to its original tree.
template <typename captype, typename tcaptype, typename flowtype> 
void Graph<captype,tcaptype,flowtype>::process_sink_orphan(node *i)
{
    arc* a0_min=NULL;
    int d_min=INFINITE_D;

    /* trying to find a new parent */
    for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next)
        if (arcs[a0].r_cap) {
            node* j = &nodes[arcs[a0].head];
            if (j->term==SINK && j->parent!=0) {
                /* checking the origin of j */
                int d = 0;
                while (true) {
                    if (j->TS == TIME) {
                        d += j->DIST;
                        break;
                    }
                    arc* a = j->parent;
                    d ++;
                    if (a==TERMINAL) {
                        j->TS = TIME;
                        j->DIST = 1;
                        break;
                    }
                    if (a==ORPHAN) { d = INFINITE_D; break; }
                    j = &nodes[a->head];
                }
                if (d<INFINITE_D) { /* j originates from the sink - done */
                    if (d<d_min) {
                        a0_min = &arcs[a0];
                        d_min = d;
                    }
                    /* set marks along the path */
                    for (j=&nodes[arcs[a0].head]; j->TS!=TIME; j=&nodes[j->parent->head]) {
                        j->TS = TIME;
                        j->DIST = d --;
                    }
                }
            }
        }

    if ((i->parent = a0_min) != 0) {
        i->TS = TIME;
        i->DIST = d_min + 1;
    } else { // no parent is found, process neighbors
        for (arc_id a0=i->first; a0>=0; a0=arcs[a0].next) {
            node* j = &nodes[arcs[a0].head];
            arc* a;
            if (j->term==SINK && (a=j->parent)) {
                if (arcs[a0].r_cap) set_active(j);
                if (a!=TERMINAL && a!=ORPHAN && &nodes[a->head]==i)
                    set_orphan(j);
            }
        }
    }
}

// Compute the maxflow.
template <typename captype, typename tcaptype, typename flowtype> 
flowtype Graph<captype,tcaptype,flowtype>::maxflow()
{
    node *i, *j, *current_node = NULL;

    nodeptr_block = new DBlock<nodeptr>(NODEPTR_BLOCK_SIZE);

    maxflow_init();

    // main loop
    while (true)
        {
            if ((i=current_node)) {
                i->next = NULL; /* remove active flag */
                if (!i->parent) i = NULL;
            }
            if (!i && !(i=next_active()))
                break;

            arc_id a=-1;
            /* grow tree */
            for (a=i->first; a>=0; a=arcs[a].next)
            if (i->term==SOURCE? arcs[a].r_cap: arcs[arcs[a].sister].r_cap) {
                    j = &nodes[arcs[a].head];
                    if (!j->parent) {
                        j->term = i->term;
                        j->parent = &arcs[arcs[a].sister];
                        j->TS = i->TS;
                        j->DIST = i->DIST + 1;
                        set_active(j);
                    }
                    else if (j->term!=i->term) break;
                    else if (j->TS <= i->TS &&
                             j->DIST > i->DIST) {
                        // heuristic: try shortening distance from j to terminal
                        j->parent = &arcs[arcs[a].sister];
                        j->TS = i->TS;
                        j->DIST = i->DIST + 1;
                    }
                }

            TIME ++;

            if (a>=0) {
                i->next = i; /* set active flag */
                current_node = i;

                /* augmentation */
                if(i->term==SINK)
                    a = arcs[a].sister;
                augment(&arcs[a]);
                /* augmentation end */

                /* adoption */
                nodeptr* np;
                while ((np=orphan_first)!=0) {
                    nodeptr* np_next = np->next;
                    np->next = NULL;

                    while ((np=orphan_first)) {
                        orphan_first = np->next;
                        i = np->ptr;
                        nodeptr_block->Delete(np);
                        if (!orphan_first) orphan_last = NULL;
                        if (i->term==SINK) process_sink_orphan(i);
                        else            process_source_orphan(i);
                    }

                    orphan_first = np_next;
                }
                /* adoption end */
            }
            else current_node = NULL;
        }

    delete nodeptr_block; 
    nodeptr_block = NULL; 

    return flow;
}

#undef TERMINAL
#undef ORPHAN
#endif
