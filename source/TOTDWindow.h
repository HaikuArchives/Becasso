#ifndef _TOTDWindow_H
#define _TOTDWindow_H

#include <Window.h>

class BTextView;

class TOTDWindow : public BWindow {
  public:
	TOTDWindow(const BRect frame, const int num);
	virtual ~TOTDWindow();
	virtual void MessageReceived(BMessage* msg);
	virtual bool QuitRequested();

  private:
	int32 fTotd;
	BTextView* fTextView;
};

#endif