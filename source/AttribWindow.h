#ifndef ATTRIBWINDOW_H
#define ATTRIBWINDOW_H

#include <Message.h>
#include <Window.h>
#include "AttribView.h"
#include "BitmapView.h"

#define MAX_VIEWS 64

typedef struct {
	AttribView* view;
	BitmapView* icon;
} view_n_icon;

class AttribWindow : public BWindow {
public:
	AttribWindow(BRect frame, const char* title);
	virtual ~AttribWindow();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property);
	virtual bool QuitRequested();
	virtual int AddView(AttribView* view, BBitmap* icon = 0);

	virtual int Current() { return (current); };

	virtual void RaiseView(int index);
	virtual void Show();
	virtual void Hide();
	virtual void Quit();

private:
	typedef BWindow inherited;
	view_n_icon views[MAX_VIEWS];
	int current;
	int numviews;
	char orig_title[16];
};

#endif