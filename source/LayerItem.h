#ifndef LAYERITEM_H
#define LAYERITEM_H

#include <View.h>
#include <Rect.h>
#include <Point.h>
#include <CheckBox.h>
#include <Message.h>
#include "Layer.h"
#include "CanvasView.h"
#include <MenuItem.h>

#define LAYERITEMHEIGHT     64
#define LAYERITEMWIDTH     256
#define THUMBLAYERMAXHEIGHT 56
#define THUMBLAYERMAXWIDTH  90

class LayerItem : public BView
{
public:
			 LayerItem (BRect frame, const char *name, int layerIndex, CanvasView *_myView);
virtual		~LayerItem ();
virtual void Refresh (Layer *layer);
virtual void Draw (BRect updateRect);
virtual void MouseDown (BPoint point);
virtual void MouseMoved (BPoint point, uint32 transit, const BMessage *message);
virtual void MessageReceived (BMessage *message);
void		 DrawThumbOnly ();
CanvasView	*getCanvasView () { return (fMyView); };
void		 setCanvasView (CanvasView *_myView) { fMyView = _myView; };
void		 select (bool sel = true);

private:
typedef BView inherited;
int			 index;
Layer		*fLayer;
CanvasView	*fMyView;
float		 fThumbHSize, fThumbVSize;
BCheckBox	*hide;
BPopUpMenu	*fModePU;
int			 click;
bigtime_t	 dcspeed;
};

#endif 