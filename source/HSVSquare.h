#ifndef HSVSQUARE_H
#define HSVSQUARE_H

#include <View.h>
#include <Bitmap.h>
#include <Rect.h>
#include "ColorWindow.h"
#include "hsv.h"

class HSVSquare : public BView {
  public:
	HSVSquare(BRect frame, ColorWindow* ed);
	virtual ~HSVSquare();
	virtual void Draw(BRect update);
	virtual void MouseDown(BPoint point);
	virtual void mouseDown(BPoint point, int ob);
	virtual void AttachedToWindow();
	virtual void ScreenChanged(BRect frame, color_space cs);
	void SetColor(rgb_color c);
	void SetColor(hsv_color c);
	rgb_color GetColor();
	hsv_color GetColorHSV();

  private:
	typedef BView inherited;
	void DrawLines();
	BBitmap* colorsquare;
	BBitmap* colorcol;
	uchar square[256][256][3];
	float hs[256][256][2];
	uchar col[256][32][3];
	rgb_color current;
	hsv_color currentHSV;
	int notcolor;
	class ColorWindow* editor;
	float prevy;
	int clicks;
	BPoint prev;
	bool fFirst;
};

int32
HSV_track_mouse(void* data);

#endif