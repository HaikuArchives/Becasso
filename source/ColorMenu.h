#ifndef COLORMENU_H
#define COLORMENU_H

#include <Menu.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>
#include "ColorItem.h"
#include "DragWindow.h"
#include "sfx.h"

#define MAX_COLORS 256

class ColorMenuButton;

class ColorMenu : public BMenu {
	friend class ColorMenuButton;

public:
	ColorMenu(const char* name, BView* _view, int h, int v, float s);
	virtual ~ColorMenu();
	virtual void AddItem(ColorItem* item, BRect frame);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* msg);
	ColorItem* FindMarked();

	ColorItem* ItemAt(int ind) { return items[ind]; };

	virtual void Mark(int _index);
	void setParent(ColorMenuButton* cmb);

	ColorMenuButton* getParent() { return parent; };

	void TearDone(BRect place, bool newwin);
	void InvalidateWindow();

protected:
	virtual BPoint ScreenLocation();

private:
	typedef BMenu inherited;
	DragWindow* fWindow;
	ColorMenuButton* parent;
	ColorItem* items[MAX_COLORS];
	int index;
	BView* view;
	float hs, vs;
};

#endif