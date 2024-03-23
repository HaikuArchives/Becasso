// © 1999 Sum Software

#ifndef TABVIEW_H
#define TABVIEW_H

#include <View.h>
#include <Rect.h>
#include <Point.h>
#include "Build.h"

#define MAX_VIEWS 32
#define MAX_TAB 32
#define TAB_HEIGHT 16

class IMPEXP TabView : public BView
{
  public:
	TabView(BRect frame, const char* name, uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual ~TabView();
	virtual void Draw(BRect update);
	virtual void MouseDown(BPoint point);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void MakeFocus(bool focused = true);
	void AddView(BView* view, const char* tab);
	// BView		*Current ();
	void RaiseView(int n);

  private:
	typedef BView inherited;
	BView* views[MAX_VIEWS];
	char tabname[MAX_VIEWS][MAX_TAB];
	int index;
	int current;
	int focusedview;
};

#endif