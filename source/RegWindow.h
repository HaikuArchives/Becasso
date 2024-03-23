// Registration window

#ifndef _REGWINDOW_H
#define _REGWINDOW_H

#include <Window.h>

#define REG_NONE 0
#define REG_CD 1
#define REG_ESD 2
#define REG_PREINST 3
#define REG_PPC 4
#define REG_HAIKU 5
#define REG_HAS_14 14
#define REG_HAS_15 15

class BTextControl;

class RegWindow : public BWindow
{
  public:
	RegWindow(const BRect frame, const char* title, char type);
	virtual ~RegWindow();

	int32 Go();
	virtual void MessageReceived(BMessage* msg);

  private:
	char fType;
	int32 fStatus;
	BTextControl* fName;
};

#endif
