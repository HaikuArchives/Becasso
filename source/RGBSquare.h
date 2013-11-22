#ifndef RGBSQUARE_H
#define RGBSQUARE_H

#include <View.h>
#include <Bitmap.h>
#include <Rect.h>
#include "ColorWindow.h"

class RGBSquare : public BView
{
public:
			 RGBSquare (BRect frame, int nc, ColorWindow *ed);
virtual		~RGBSquare ();
virtual void Draw (BRect update);
virtual void MouseDown (BPoint point);
virtual void mouseDown (BPoint point, int ob);
virtual void AttachedToWindow ();
virtual void ScreenChanged (BRect frame, color_space cs);
void		 SetColor (rgb_color c);
void		 SetNotColor (int nc);
rgb_color	 GetColor ();

private:
typedef BView inherited;
void		 DrawLines();
BBitmap		*colorsquare;
BBitmap		*colorcol;
uchar		 square[256][256][3];
uchar		 col[256][32][3];
rgb_color	 current;
int			 notcolor;
class ColorWindow	*editor;
bool		 first;
int			 clicks;
BPoint		 prev;
};

int32 RGB_track_mouse (void *data);

#endif 