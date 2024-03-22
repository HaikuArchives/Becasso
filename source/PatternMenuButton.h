#ifndef PATTERNMENUBUTTON_H
#define PATTERNMENUBUTTON_H

#include <View.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>
#include <Message.h>
#include "PatternMenu.h"
#include "HelpView.h"

#define P_V_NUM 2
#define P_H_NUM 10
#define P_SIZE 24

class PatternMenuButton : public BView {
  public:
	PatternMenuButton(BRect frame, const char* name);
	virtual ~PatternMenuButton();
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* msg);
	virtual void Draw(BRect update);
	virtual void MessageReceived(BMessage* msg);
	int32 selected();
	void set(int _index);
	void set(pattern _pat);
	pattern pat();

	PatternMenu* getMenu() { return (menu); };

	bool IsEditorShowing() { return (editorshowing); };

	// class PatternWindow *Editor () { return (editor); };
	void ShowEditor();
	void editorSaysBye();
	BLocker* lock;
	bool MenuWinOnScreen;

  private:
	typedef BView inherited;
	// class PatternWindow *editor;
	char _name[MAX_HLPNAME];
	PatternMenu* menu;
	int index;
	bool editorshowing;
	int click;
	bigtime_t dcspeed;
};

#endif