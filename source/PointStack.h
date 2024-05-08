#ifndef POINTSTACK_H
#define POINTSTACK_H

#include <Point.h>

class LPoint {
public:
	LPoint(){};

	LPoint(long _x, long _y)
	{
		x = _x;
		y = _y;
	};

	LPoint(BPoint p)
	{
		x = long(p.x);
		y = long(p.y);
	};

	~LPoint(){};
	long x;
	long y;
};

#define MAXPOINTSTACK 1024

// Note: This is really a FIFO, not a LIFO stack.

class PointStack {
public:
	PointStack();
	PointStack(LPoint p);
	~PointStack();
	bool push(LPoint point);
	LPoint pop();
	bool isempty();

private:
	LPoint* stack;
	int bot;
	int top;
};

#endif