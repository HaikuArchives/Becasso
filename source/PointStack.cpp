#include "PointStack.h"
#include <stdio.h>
#include <stdlib.h>

PointStack::PointStack()
{
	bot = 0;
	top = 0;
	stack = new LPoint[MAXPOINTSTACK];
}

PointStack::PointStack(LPoint p)
{
	top = 1;
	bot = 0;
	stack = new LPoint[MAXPOINTSTACK];
	stack[0] = p;
}

PointStack::~PointStack()
{
	delete[] stack;
}

bool
PointStack::push(LPoint point)
{
	top %= MAXPOINTSTACK;

	stack[top++] = point;
	return (true);
	//  NOTE:  This version _doesn't_ check whether the stack
	//         is full...  This is potentially dangerous for
	//         large area fills.
}

LPoint
PointStack::pop()
{
	bot %= MAXPOINTSTACK;

	if (bot == top) {
		fprintf(stderr, "PointStack:  Popping from empty stack!\n");
		exit(1);
	}
	return stack[bot++];
}

bool
PointStack::isempty()
{
	return (bot == top);
}
