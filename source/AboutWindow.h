#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <Window.h>

class AboutWindow : public BWindow {
public:
	AboutWindow(BRect rect);
	virtual ~AboutWindow();
	virtual void MessageReceived(BMessage* msg);

private:
	typedef BWindow inherited;
};

#endif