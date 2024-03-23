#ifndef PICMENUVIEW_H
#define PICMENUVIEW_H

#include "View.h"
#include "PicMenuButton.h"

class PicMenuView : public BView
{
  public:
	PicMenuView(BRect rect, int _hnum, const char* name, PicMenuButton* pmb);
	virtual ~PicMenuView();
	virtual void Draw(BRect updateRect);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* msg);
	virtual void MouseDown(BPoint point);

  private:
	BRect RectForIndex(int i);

	typedef BView inherited;
	PicMenuButton* fPMB;
	int index;
	int click;
	int hnum;
	bigtime_t dcspeed;
};

#endif