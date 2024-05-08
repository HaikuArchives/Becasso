#ifndef COLORMENUVIEW_H
#define COLORMENUVIEW_H

#include "ColorMenuButton.h"
#include "View.h"

class ColorMenuView : public BView {
public:
	ColorMenuView(BRect rect, const char* name, ColorMenuButton* pmb);
	virtual ~ColorMenuView();
	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* msg);
	BRect RectForIndex(int ind);

private:
	typedef BView inherited;
	ColorMenuButton* fCMB;
	int index;
	int click;
	bigtime_t dcspeed;
};

#endif