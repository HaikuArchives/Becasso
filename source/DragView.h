#ifndef _DRAGVIEW
#define _DRAGVIEW

#ifndef _VIEW_H
#define _VIEW_H
#include <View.h>
#endif

#ifndef _BITS
#define _BITS
#include "Bits/GoAway.h"
#include "Bits/GoAway2.h"
#endif


const int DV_HORZ = 0;
const int DV_VERT = 1;

class DragView : public BView
{
  public:
	DragView(BRect frame, const char* name, int Orientation);
	virtual ~DragView();
	virtual void Draw(BRect updateRect);
	virtual void DrawHorz();
	virtual void DrawVert();

	BBitmap* goAwayUp;
	BBitmap* goAwayDown;
	ulong orientation;
	int mouse_status;
	int goAwayState;
	int active;
	thread_id thMouseID;

	virtual void MouseDown(BPoint where);
	/*
	virtual	void			WindowActivated(bool state);
	virtual	void			AttachedToWindow();
	virtual	void			AllAttached();
	virtual	void			DetachedFromWindow();
	virtual	void			AllDetached();

	virtual	void			AddChild(BView *aView);
	virtual	bool			RemoveChild(BView *childView);


	virtual	void			MouseMoved(	BPoint where,
										ulong code,
										BMessage *a_message);
	virtual	void			KeyDown(ulong aKey);
	virtual	void			Pulse();
	virtual	void			FrameMoved(BPoint new_position);
	virtual	void			FrameResized(float new_width, float new_height);
	virtual	void			Show();
	virtual	void			Hide();
	*/
};

#endif
