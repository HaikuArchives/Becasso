#ifndef PATTERNMENUWINDOW_H
#define PATTERNMENUWINDOW_H

#include "View.h"
#include "PatternMenuButton.h"

class PatternMenuView : public BView
{
public:
			 PatternMenuView (BRect rect, const char *name, PatternMenuButton *pmb);
virtual		~PatternMenuView ();
virtual void Draw (BRect updateRect);
virtual void MouseDown (BPoint point);
virtual void MouseMoved (BPoint point, uint32 transit, const BMessage *msg);
BRect		 RectForIndex (int ind);

private:
typedef BView inherited;
PatternMenuButton *fPMB;
int			 index;
int			 click;
bigtime_t	 dcspeed;
};

#endif 