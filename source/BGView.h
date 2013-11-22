#ifndef BGVIEW_H
#define BGVIEW_H

#include <View.h>
#include <ScrollBar.h>

class BGView : public BView
{
public:
			 BGView (BRect frame, const char *name, uint32 resizeMask, uint32 flags);
virtual		~BGView ();
virtual void FrameResized (float width, float height);
virtual void MouseMoved (BPoint point, uint32 transit, const BMessage *msg);
virtual void MouseDown (BPoint point);
virtual void MouseUp (BPoint point);
virtual void Draw (BRect update);
virtual void ScrollTo (BPoint where);
void		 setScale (float s);
float		 getScale () { return fScale; };
void		 setFrame (BRect frame);
BRect		 getFrame () { return fFrame; };

private:
typedef BView inherited;
float		 fScale;
BRect		 fFrame;
};

#endif 