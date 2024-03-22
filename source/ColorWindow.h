#ifndef COLORWINDOW_H
#define COLORWINDOW_H

#include <Window.h>
#include <Rect.h>
#include <MenuBar.h>
#include <TextControl.h>
#include <StringView.h>
#include <FilePanel.h>
#include "ColorMenuButton.h"
#include "Slider.h"
#include "RGBSquare.h"
#include "HSVSquare.h"
#include "hsv.h"
#include "ColorView.h"

#define CE_WIDTH 320
#define CE_HEIGHT 502

class ColorWindow : public BWindow {
  public:
	ColorWindow(BRect frame, const char* name, ColorMenuButton* but);
	virtual ~ColorWindow();
	virtual void MessageReceived(BMessage* msg);
	virtual void Quit();
	virtual void ScreenChanged(BRect rect, color_space cs);

	rgb_color rgb() { return c; };

	hsv_color hsv() { return rgb2hsv(c); };

  private:
	typedef BWindow inherited;
	void SetColor(rgb_color _c);
	ColorMenuButton* button;
	BMenuBar* menubar;
	class RGBSquare* rgbSquare;
	class HSVSquare* hsvSquare;
	BTextControl *rTC, *gTC, *bTC, *hTC, *sTC, *vTC;
	ColorView *cur, *match;
	BStringView* dist;
	rgb_color c;
	int ob;
	BFilePanel *savePanel, *openPanel;
	Slider* aSlid;
};

#endif