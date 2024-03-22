#ifndef LAYERWINDOW_H
#define LAYERWINDOW_H

#include <Message.h>
#include <ScrollBar.h>
#include <MenuBar.h>
#include <Window.h>
#include "LayerView.h"
#include "CanvasView.h"

class LayerWindow : public BWindow {
	friend class CanvasView;

  public:
	LayerWindow(BRect frame, char* name, CanvasView* _MyView);
	virtual ~LayerWindow();
	virtual void MessageReceived(BMessage* msg);
	virtual void Quit();
	virtual void WindowActivated(bool active);
	void doChanges(int index = -1);

  private:
	typedef BWindow inherited;
	LayerView* layerView;
	BScrollBar *h, *v;
	BWindow* myWindow;
	CanvasView* myView;
	BMenuBar* menubar;
	BMenu* layerM;
};

#endif