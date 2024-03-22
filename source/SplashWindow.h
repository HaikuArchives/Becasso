#ifndef SPLASHWINDOW_H
#define SPLASHWINDOW_H

#include <Window.h>

class SAboutView;

class SplashWindow : public BWindow {
  public:
	SplashWindow(BRect rect, BBitmap* becasso, BBitmap* sum);
	virtual ~SplashWindow();
	virtual void MessageReceived(BMessage* msg);

  private:
	typedef BWindow inherited;
	SAboutView* abV;
};

#endif