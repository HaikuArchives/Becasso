#ifndef PICMENUBUTTON_H
#define PICMENUBUTTON_H

#include <Rect.h>
#include <Picture.h>
#include <PictureButton.h>
#include "PicItem.h"
#include "PicMenu.h"
#include "HelpView.h"

class PicMenuButton : public BPictureButton
{
  public:
	PicMenuButton(BRect frame, const char* name, BPicture* p);
	virtual ~PicMenuButton();
	virtual void AddItem(PicMenu* _menu);
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* msg);
	virtual void MessageReceived(BMessage* msg);
	virtual void Draw(BRect update);
	int32 selected();
	void set(int32 ind);

	PicMenu* getMenu() { return (menu); };

	BLocker* lock;
	bool MenuWinOnScreen;

  private:
	typedef BPictureButton inherited;
	PicMenu* menu;
	int32 index;
	char _name[MAX_HLPNAME];
	int click;
	bigtime_t dcspeed;
};

#endif