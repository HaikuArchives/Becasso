#ifndef _MAGVIEW_H
#define _MAGVIEW_H

#include <View.h>
#include <Rect.h>
#include <Point.h>
#include <ScrollBar.h>
#include "CanvasView.h"

#define MAX_MAG_UNDO 512

struct MagUndoEntry {
	uint16 x;
	uint16 y;
	uint32 c;
};

class MagView : public BView {
  public:
	MagView(BRect frame, const char* name, CanvasView* _myView);
	virtual ~MagView();
	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* msg);
	virtual void FrameResized(float width, float height);
	virtual void AttachedToWindow();
	virtual void Pulse();

	void setScrollBars(BScrollBar* _h, BScrollBar* _v);
	void setzoom(int z);
	void setgrid(int g);
	bool contains(BPoint p) const;
	float currentzoom();

	bool CanUndo();
	bool CanRedo();
	status_t Undo();
	status_t Redo();

  private:
	void SetupUndo(BPoint p, uint32 c);

	float zoom;
	int grid_visible;
	BPoint pos;
	BScrollBar *mh, *mv;
	CanvasView* myView;
	MagUndoEntry undo[MAX_MAG_UNDO];
	int32 undoIndex;
	int32 redoIndex;
	bool mouse_on_canvas;
	bool fPicking;
};

#endif