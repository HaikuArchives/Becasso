#ifndef LAYERVIEW_H
#define LAYERVIEW_H

#include <Point.h>
#include <Rect.h>
#include <ScrollBar.h>
#include <View.h>
#include "AttribDraw.h"	 // For MAX_LAYERS
#include "CanvasView.h"
#include "LayerItem.h"

class LayerView : public BView {
public:
	LayerView(BRect frame, const char* name, CanvasView* _myView);
	virtual ~LayerView();
	virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);
	virtual void AttachedToWindow();

	void setScrollBars(BScrollBar* _h, BScrollBar* _v);

	LayerItem* getLayerItem(int index) { return layerItem[index]; };

	void setCanvasView(CanvasView* _myView) { fMyView = _myView; };

private:
	typedef BView inherited;
	BScrollBar *mh, *mv;
	CanvasView* fMyView;
	LayerItem* layerItem[MAX_LAYERS + 1];
	BRect fFrame;
};

#endif