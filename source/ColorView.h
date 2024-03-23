#ifndef COLORVIEW_H
#define COLORVIEW_H

#include <View.h>
#include <Rect.h>
#include <Bitmap.h>

class ColorView : public BView
{
  public:
	ColorView(BRect frame, const char* name, rgb_color c);
	virtual ~ColorView();
	virtual void Draw(BRect update);
	virtual void MouseDown(BPoint point);
	virtual void ScreenChanged(BRect frame, color_space cs);
	void SetColor(rgb_color c);
	rgb_color GetColor();

  private:
	typedef BView inherited;
	BBitmap* bitmap;
	rgb_color color;
};

#endif