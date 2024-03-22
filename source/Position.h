#ifndef POSITION_H
#define POSITION_H

#include <Point.h>

#define POSITION_QUEUE 256

typedef struct {
	BPoint fPoint;
	BPoint fTilt;
	uint32 fButtons;
	uint8 fPressure;
	bool fProximity;
} Position;

int32
position_tracker(void* data);

#endif