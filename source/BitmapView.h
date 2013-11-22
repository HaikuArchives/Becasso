#ifndef BITMAPVIEW_H
#define BITMAPVIEW_H

#include <View.h>
#include <Rect.h>
#include <Bitmap.h>

class BitmapView : public BView
{
public:
			 BitmapView (BRect bounds, const char *name);
			 BitmapView (BRect bounds, const char *name, BBitmap *map, drawing_mode mode = B_OP_COPY, bool center = true, bool own = true);
virtual		~BitmapView ();
virtual void Draw (BRect update);
virtual void SetBitmap (BBitmap *bitmap);
virtual void SetPosition (const BPoint leftTop);

private:
typedef BView inherited;
BBitmap		*fBitmap;
BPoint		fPosition;
bool		fCenter;
};

#endif 