#include "Tablet.h"
#include "CanvasView.h"
#include "SView.h"

#if 1

int32
position_tracker(void* data)
{
	SView* view = (SView*)data;
	Position pos;
	uint32 time = view->getSnoozeTime();
	extern port_id position_port;
	view->GetPosition(&pos);
	while (pos.fProximity) {
		write_port(position_port, 'Spos', (void*)&pos, sizeof(Position));
		snooze(time);
		view->GetPosition(&pos);
	}
	write_port(position_port, 'Lpos', (void*)&pos, sizeof(Position));
	// The last position written has its fButtons entry empty.
	return 0;
}

#else

int32
position_tracker(void* data)
{
	CanvasView* view = (CanvasView*)data;
	Position pos;
	uint32 time = view->getSnoozeTime();
	extern port_id position_port;
	view->GetPosition(&pos);
	BMessage* pm = new BMessage();
	while (pos.fProximity) {
		write_port(position_port, 'Spos', (void*)&pos, sizeof(Position));
		snooze(time);
		view->GetPosition(&pos);
	}
	view->PostMessage(pm);
	delete pm;
	// The last position written has its fButtons entry empty.
	return 0;
}

#endif