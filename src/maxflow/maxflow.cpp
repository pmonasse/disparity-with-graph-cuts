#ifdef __GRAPH_H__
/* maxflow.cpp */

#include <stdio.h>

/*
	special constants for node->parent. Duplicated in graph.cpp, both should match!
*/
#define TERMINAL ( (arc *) 1 )		/* to terminal */
#define ORPHAN   ( (arc *) 2 )		/* orphan */


#define INFINITE_D ((int)(((unsigned)-1)/2))		/* infinite distance to the terminal */

/***********************************************************************/

/*
	Functions for processing active list.
	i->next points to the next node in the list
	(or to i, if i is the last node in the list).
	If i->next is NULL iff i is not in the list.
*/


template <typename captype, typename tcaptype, typename flowtype> 
	inline void Graph<captype,tcaptype,flowtype>::set_active(node *i)
{
	if (!i->next)
	{
		/* it's not in the list yet */
		if (queue_last) queue_last -> next = i;
		else               queue_first        = i;
		queue_last = i;
		i -> next = i;
	}
}

/*
	Returns the next active node.
	If it is connected to the sink, it stays in the list,
	otherwise it is removed from the list
*/
template <typename captype, typename tcaptype, typename flowtype> 
	inline typename Graph<captype,tcaptype,flowtype>::node* Graph<captype,tcaptype,flowtype>::next_active()
{
	node *i;

	while ( 1 )
	{
		if (!(i=queue_first))
            return NULL;

		/* remove it from the active list */
		if (i->next == i) queue_first = queue_last = NULL;
		else              queue_first = i -> next;
		i -> next = NULL;

		/* a node in the list is active iff it has a parent */
		if (i->parent) return i;
	}
}

/***********************************************************************/

template <typename captype, typename tcaptype, typename flowtype> 
	inline void Graph<captype,tcaptype,flowtype>::set_orphan(node *i)
{
	nodeptr *np;
	i -> parent = ORPHAN;
	np = nodeptr_block -> New();
	np -> ptr = i;
	if (orphan_last) orphan_last -> next = np;
	else             orphan_first        = np;
	orphan_last = np;
	np -> next = NULL;
}

/***********************************************************************/

template <typename captype, typename tcaptype, typename flowtype> 
	void Graph<captype,tcaptype,flowtype>::maxflow_init()
{
	node *i;

	queue_first = queue_last = NULL;
	orphan_first = NULL;

	TIME = 0;

	for (i=nodes; i<node_last; i++)
	{
		i -> next = NULL;
		i -> TS = TIME;
		if (i->tr_cap > 0)
		{
			/* i is connected to the source */
			i -> term = SOURCE;
			i -> parent = TERMINAL;
			set_active(i);
			i -> DIST = 1;
		}
		else if (i->tr_cap < 0)
		{
			/* i is connected to the sink */
			i -> term = SINK;
			i -> parent = TERMINAL;
			set_active(i);
			i -> DIST = 1;
		}
		else
		{
			i -> parent = NULL;
		}
	}
}

template <typename captype, typename tcaptype, typename flowtype> 
	void Graph<captype,tcaptype,flowtype>::augment(arc *middle_arc)
{
    node_id i;
	tcaptype bottleneck;

	/* 1. Finding bottleneck capacity */
	/* 1a - the source tree */
	bottleneck = middle_arc->r_cap;
	i=middle_arc->sister->head;
	do {
		arc* a = nodes[i].parent;
		if (a == TERMINAL) break;
		if (bottleneck > a->sister->r_cap) bottleneck = a->sister->r_cap;
        i = a->head;
	} while(true);
	if (bottleneck > nodes[i].tr_cap) bottleneck = nodes[i].tr_cap;
	/* 1b - the sink tree */
	i=middle_arc->head;
	do {
		arc* a = nodes[i].parent;
		if (a == TERMINAL) break;
		if (bottleneck > a->r_cap) bottleneck = a->r_cap;
        i=a->head;
	} while(true);
	if (bottleneck > -nodes[i].tr_cap) bottleneck = -nodes[i].tr_cap;

	/* 2. Augmenting */
	flow += bottleneck;
	/* 2a - the source tree */
	middle_arc->sister->r_cap += bottleneck;
	middle_arc->r_cap -= bottleneck;
	i=middle_arc->sister->head;
	do {
		arc* a = nodes[i].parent;
		if (a == TERMINAL) break;
		a->r_cap += bottleneck;
		a->sister->r_cap -= bottleneck;
		if (!a->sister->r_cap)
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
		a->sister->r_cap += bottleneck;
		a->r_cap -= bottleneck;
		if (!a->r_cap)
			set_orphan(&nodes[i]);
        i=a->head;
	} while(true);
	nodes[i].tr_cap += bottleneck;
	if (!nodes[i].tr_cap)
		set_orphan(&nodes[i]);
}

/***********************************************************************/

template <typename captype, typename tcaptype, typename flowtype> 
	void Graph<captype,tcaptype,flowtype>::process_source_orphan(node *i)
{
	node *j;
	arc *a0, *a0_min = NULL, *a;
	int d, d_min = INFINITE_D;

	/* trying to find a new parent */
	for (a0=i->first; a0; a0=a0->next)
	if (a0->sister->r_cap)
	{
		j = &nodes[a0->head];
		if (j->term==SOURCE && (a=j->parent))
		{
			/* checking the origin of j */
			d = 0;
			while ( 1 )
			{
				if (j->TS == TIME)
				{
					d += j -> DIST;
					break;
				}
				a = j -> parent;
				d ++;
				if (a==TERMINAL)
				{
					j -> TS = TIME;
					j -> DIST = 1;
					break;
				}
				if (a==ORPHAN) { d = INFINITE_D; break; }
				j = &nodes[a->head];
			}
			if (d<INFINITE_D) /* j originates from the source - done */
			{
				if (d<d_min)
				{
					a0_min = a0;
					d_min = d;
				}
				/* set marks along the path */
				for (j=&nodes[a0->head]; j->TS!=TIME; j=&nodes[j->parent->head])
				{
					j -> TS = TIME;
					j -> DIST = d --;
				}
			}
		}
	}

	if ((i->parent = a0_min) != 0)
	{
		i -> TS = TIME;
		i -> DIST = d_min + 1;
	}
	else
	{
		/* no parent is found */
		/* process neighbors */
		for (a0=i->first; a0; a0=a0->next)
		{
			j = &nodes[a0->head];
			if (j->term==SOURCE && (a=j->parent))
			{
				if (a0->sister->r_cap) set_active(j);
				if (a!=TERMINAL && a!=ORPHAN && &nodes[a->head]==i)
				{
					set_orphan(j); // add j to the end of the adoption list
				}
			}
		}
	}
}

template <typename captype, typename tcaptype, typename flowtype> 
	void Graph<captype,tcaptype,flowtype>::process_sink_orphan(node *i)
{
	node *j;
	arc *a0, *a0_min = NULL, *a;
	int d, d_min = INFINITE_D;

	/* trying to find a new parent */
	for (a0=i->first; a0; a0=a0->next)
	if (a0->r_cap)
	{
		j = &nodes[a0->head];
		if (j->term==SINK && (a=j->parent))
		{
			/* checking the origin of j */
			d = 0;
			while ( 1 )
			{
				if (j->TS == TIME)
				{
					d += j -> DIST;
					break;
				}
				a = j -> parent;
				d ++;
				if (a==TERMINAL)
				{
					j -> TS = TIME;
					j -> DIST = 1;
					break;
				}
				if (a==ORPHAN) { d = INFINITE_D; break; }
				j = &nodes[a->head];
			}
			if (d<INFINITE_D) /* j originates from the sink - done */
			{
				if (d<d_min)
				{
					a0_min = a0;
					d_min = d;
				}
				/* set marks along the path */
				for (j=&nodes[a0->head]; j->TS!=TIME; j=&nodes[j->parent->head])
				{
					j -> TS = TIME;
					j -> DIST = d --;
				}
			}
		}
	}

	if ((i->parent = a0_min) != 0)
	{
		i -> TS = TIME;
		i -> DIST = d_min + 1;
	}
	else
	{
		/* no parent is found */
		/* process neighbors */
		for (a0=i->first; a0; a0=a0->next)
		{
			j = &nodes[a0->head];
			if (j->term==SINK && (a=j->parent))
			{
				if (a0->r_cap) set_active(j);
				if (a!=TERMINAL && a!=ORPHAN && &nodes[a->head]==i)
				{
					set_orphan(j); // add j to the end of the adoption list
				}
			}
		}
	}
}

/***********************************************************************/

template <typename captype, typename tcaptype, typename flowtype> 
	flowtype Graph<captype,tcaptype,flowtype>::maxflow()
{
	node *i, *j, *current_node = NULL;
	arc *a;
	nodeptr *np, *np_next;

    nodeptr_block = new DBlock<nodeptr>(NODEPTR_BLOCK_SIZE, error_function);

    maxflow_init();

	// main loop
	while ( 1 )
	{
		// test_consistency(current_node);

		if ((i=current_node))
		{
			i -> next = NULL; /* remove active flag */
			if (!i->parent) i = NULL;
		}
		if (!i)
		{
			if (!(i = next_active())) break;
		}

		/* growth */
		if (i->term==SOURCE)
		{
			/* grow source tree */
			for (a=i->first; a; a=a->next)
			if (a->r_cap)
			{
				j = &nodes[a->head];
				if (!j->parent)
				{
					j -> term = SOURCE;
					j -> parent = a -> sister;
					j -> TS = i -> TS;
					j -> DIST = i -> DIST + 1;
					set_active(j);
				}
				else if (j->term==SINK) break;
				else if (j->TS <= i->TS &&
				         j->DIST > i->DIST)
				{
					/* heuristic - trying to make the distance from j to the source shorter */
					j -> parent = a -> sister;
					j -> TS = i -> TS;
					j -> DIST = i -> DIST + 1;
				}
			}
		}
		else
		{
			/* grow sink tree */
			for (a=i->first; a; a=a->next)
			if (a->sister->r_cap)
			{
				j = &nodes[a->head];
				if (!j->parent)
				{
					j -> term = SINK;
					j -> parent = a -> sister;
					j -> TS = i -> TS;
					j -> DIST = i -> DIST + 1;
					set_active(j);
				}
				else if (j->term==SOURCE) { a = a -> sister; break; }
				else if (j->TS <= i->TS &&
				         j->DIST > i->DIST)
				{
					/* heuristic - trying to make the distance from j to the sink shorter */
					j -> parent = a -> sister;
					j -> TS = i -> TS;
					j -> DIST = i -> DIST + 1;
				}
			}
		}

		TIME ++;

		if (a)
		{
			i -> next = i; /* set active flag */
			current_node = i;

			/* augmentation */
			augment(a);
			/* augmentation end */

			/* adoption */
			while ((np=orphan_first))
			{
				np_next = np -> next;
				np -> next = NULL;

				while ((np=orphan_first))
				{
					orphan_first = np -> next;
					i = np -> ptr;
					nodeptr_block -> Delete(np);
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
	// test_consistency();

    delete nodeptr_block; 
    nodeptr_block = NULL; 

	return flow;
}

/***********************************************************************/


template <typename captype, typename tcaptype, typename flowtype> 
	void Graph<captype,tcaptype,flowtype>::test_consistency(node* current_node)
{
	node *i;
	arc *a;
	int r;
	int num1 = 0, num2 = 0;

	// test whether all nodes i with i->next!=NULL are indeed in the queue
	for (i=nodes; i<node_last; i++)
	{
		if (i->next || i==current_node) num1 ++;
	}
	for (r=0; r<2; r++)
	{
		i = (r == 1) ? current_node : queue_first;
		if (i)
		for ( ; ; i=i->next)
		{
			num2 ++;
			if (i->next == i)
			{
				if (r<1) assert(i == queue_last);
				else     assert(i == current_node);
				break;
			}
		}
	}
	assert(num1 == num2);

	for (i=nodes; i<node_last; i++)
	{
		// test whether all edges in seach trees are non-saturated
		if (i->parent == NULL) {}
		else if (i->parent == ORPHAN) {}
		else if (i->parent == TERMINAL)
		{
			if (i->term==SOURCE) assert(i->tr_cap > 0);
			else             assert(i->tr_cap < 0);
		}
		else
		{
			if (i->term==SOURCE) assert (i->parent->sister->r_cap > 0);
			else             assert (i->parent->r_cap > 0);
		}
		// test whether passive nodes in search trees have neighbors in
		// a different tree through non-saturated edges
		if (i->parent && !i->next)
		{
			if (i->term==SOURCE)
			{
				assert(i->tr_cap >= 0);
				for (a=i->first; a; a=a->next)
				{
					if (a->r_cap > 0) assert(nodes[a->head]->parent && nodes[a->head]->term==SOURCE);
				}
			}
			else
			{
				assert(i->tr_cap <= 0);
				for (a=i->first; a; a=a->next)
				{
					if (a->sister->r_cap > 0) assert(nodes[a->head]->parent && nodes[a->head]->term==SINK);
				}
			}
		}
		// test marking invariants
		if (i->parent && i->parent!=ORPHAN && i->parent!=TERMINAL)
		{
			assert(i->TS <= nodes[i->parent->head]->TS);
			if (i->TS == nodes[i->parent->head]->TS) assert(i->DIST > nodes[i->parent->head]->DIST);
		}
	}
}

#endif
