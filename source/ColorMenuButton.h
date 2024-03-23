#ifndef COLORMENUBUTTON_H
#define COLORMENUBUTTON_H

#include <View.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>
#include <Message.h>
#include <NumberFormat.h>
#include "ColorMenu.h"
#include "Layer.h"
// #include "ColorWindow.h"
#include "HelpView.h"

#define C_V_NUM 8
#define C_H_NUM 32
#define C_SIZE 12

class ColorMenuButton : public BView {
  public:
	ColorMenuButton(const char* ident, BRect frame, const char* name);
	virtual ~ColorMenuButton();
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* msg);
	virtual void MessageReceived(BMessage* msg);
	virtual void Draw(BRect update);
	int32 selected();
	void editorSaysBye();
	bool approx(rgb_color a, rgb_color b);
	float cdiff(rgb_color a);
	void set(int _index, bool via_win = false);
	bool set(rgb_color c, bool anyway = false);
	void setTolerance(rgb_color _t);
	void setTolerance(float _t);
	void setAlpha(uchar a);
	float getTolerance();
	int getIndexOfClosest(rgb_color c);
	void extractPalette(Layer* l, int max_col = C_V_NUM * C_H_NUM, bool clobber = true);
	rgb_color getClosest(rgb_color c);
	rgb_color color();
	rgb_color ColorForIndex(int ind);
	void Save(BEntry entry);
	void Load(BEntry entry);
	void Generate(ulong w);

	int numColors() { return (C_V_NUM * C_H_NUM); };

	rgb_color* palette();

	ColorMenu* getMenu() { return (menu); };

	bool IsEditorShowing() { return (editorshowing); };

	class ColorWindow* Editor() { return (editor); };

	void ShowEditor();
	BLocker* lock;
	bool MenuWinOnScreen;

  private:
	typedef BView inherited;
	class ColorWindow* editor;
	char _name[MAX_HLPNAME];
	BBitmap* button;
	ulong fW, fH;
	ColorMenu* menu;
	int index;
	rgb_color rgb_tolerance;
	uchar alpha;
	float tolerance;
	bool editorshowing;
	int click;
	bigtime_t dcspeed;
	BNumberFormat fNumberFormat;
};

#endif
