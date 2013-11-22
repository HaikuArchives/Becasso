#include "LayerView.h"
#include "LayerWindow.h"
#include "LayerItem.h"

LayerView::LayerView (BRect frame, const char *name, CanvasView *_myView)
: BView (frame, name, B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS | B_WILL_DRAW)
{
	fMyView = _myView;
	fFrame = frame;
	for (int i = 0; i < MAX_LAYERS; i++)
		layerItem[i] = NULL;
}

LayerView::~LayerView ()
{
//	printf ("~LayerView\n");
}

void LayerView::setScrollBars (BScrollBar *_h, BScrollBar *_v)
{
	mh = _h;
	mv = _v;
	mh->SetTarget (this);
	mv->SetTarget (this);
}

void LayerView::Draw (BRect updateRect)
{
	inherited::Draw (updateRect);
}

void LayerView::AttachedToWindow ()
{
	// Yes I know this is rather blunt.  Let's call this RAD and all is well.
	for (int i = 0; i < MAX_LAYERS; i++)
	{
		if (layerItem[i])
		{
			RemoveChild (layerItem[i]);
			delete layerItem[i];
			layerItem[i] = NULL;
		}
	}
	FrameResized (Bounds().Width(), Bounds().Height());
	SetViewColor (B_TRANSPARENT_32_BIT);
	BRect layerItemRect = BRect (0, 0, LAYERITEMWIDTH, LAYERITEMHEIGHT);
	for (int i = fMyView->numLayers() - 1; i >= 0; i--)
	{
		layerItem[i] = new LayerItem (layerItemRect, "Layer Item", i, fMyView);
		AddChild (layerItem[i]);
		layerItemRect.OffsetBy (0, LAYERITEMHEIGHT);
	}
}

void LayerView::FrameResized (float width, float height)
{
	float hmin, hmax, vmin, vmax;
	mh->GetRange (&hmin, &hmax);
	mv->GetRange (&vmin, &vmax);
	mh->SetRange (hmin, fFrame.Width() - width);
	mv->SetRange (vmin, fFrame.Height() - height);
	mh->SetProportion (width/fFrame.Width());
	mv->SetProportion (height/fFrame.Height());
}

