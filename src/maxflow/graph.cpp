#ifdef GRAPH_H
/* graph.cpp */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

template <typename captype, typename tcaptype, typename flowtype> 
	Graph<captype, tcaptype, flowtype>::Graph(void (*err_function)(const char *))
	: nodeptr_block(NULL),
	  error_function(err_function)
{
	flow = 0;
}

template <typename captype, typename tcaptype, typename flowtype> 
	Graph<captype,tcaptype,flowtype>::~Graph()
{
	if (nodeptr_block) 
	{ 
		delete nodeptr_block; 
		nodeptr_block = NULL; 
	}
}

#endif
