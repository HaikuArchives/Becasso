#ifndef _NAGWINDOW_H
#define _NAGWINDOW_H

#include <Window.h>

class NagWindow : public BWindow
{
  public:
	NagWindow(BRect frame);
	virtual ~NagWindow();

	virtual void Go();

  private:
	typedef BWindow inherited;
};

#endif