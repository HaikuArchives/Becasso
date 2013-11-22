#include "BGView.h"
#include "Colors.h"
#include <Region.h>
#include <stdio.h>

BGView::BGView (BRect frame, const char *name, uint32 resizeMask, uint32 flags)
: BView (frame, name, resizeMask, flags)
{
	fFrame = frame;
	fFrame.OffsetTo (0, 0);
	SetViewColor (B_TRANSPARENT_32_BIT);
	fScale = 1;
}

BGView::~BGView ()
{
}

void BGView::Draw (BRect update)
{
	BView *canvas = FindView ("Canvas");
	if (canvas)
	{
		BRect cFrame = canvas->Frame();
		BRegion upd (update);
		upd.Exclude (cFrame);
		SetHighColor (DarkGrey);
		FillRegion (&upd);
	}
	BView::Draw (update);
}

void BGView::MouseMoved (BPoint point, uint32 transit, const BMessage *msg)
{
	BView *canvas = FindView ("Canvas");
	if (canvas)
	{
		BPoint mp = BPoint (point.x - canvas->Frame().left,
							point.y - canvas->Frame().top);
		canvas->MouseMoved (mp, B_OUTSIDE_VIEW, msg);
	}
	BView::MouseMoved (point, transit, msg);
}

void BGView::MouseDown (BPoint point)
{
	BView *canvas = FindView ("Canvas");
	if (canvas)
	{
		BPoint mp = BPoint (point.x - canvas->Frame().left,
							point.y - canvas->Frame().top);
		canvas->MouseDown (mp);
	}
	BView::MouseDown (point);
}

void BGView::MouseUp (BPoint point)
{
	BView *canvas = FindView ("Canvas");
	if (canvas)
	{
		BPoint mp = BPoint (point.x - canvas->Frame().left,
							point.y - canvas->Frame().top);
		canvas->MouseUp (mp);
	}
	BView::MouseUp (point);
}

void BGView::ScrollTo (BPoint where)
{
	float x = where.x;
	float y = where.y;
	float hmin, hmax, vmin, vmax;
	BScrollBar *h = ScrollBar (B_HORIZONTAL);
	BScrollBar *v = ScrollBar (B_VERTICAL);
	if (!h || !v)
	{
		fprintf (stderr, "BGView::ScrollTo (x, y): no BScrollBars\n");
		return;
	}
	h->GetRange (&hmin, &hmax);
	v->GetRange (&vmin, &vmax);
	if (x < hmin)
		x = hmin;
	if (x > hmax)
		x = hmax;
	if (y < vmin)
		y = vmin;
	if (y > vmax)
		y = vmax;
	BView::ScrollTo (BPoint (x, y));
}

void BGView::FrameResized (float width, float height)
{
	BView *canvas = FindView ("Canvas");
	BRect cFrame;
	if (canvas)
	{
		cFrame = canvas->Frame();
		float cw = canvas->Bounds().Width();
		float ch = canvas->Bounds().Height();
		if (cw < Bounds().Width() && ch < Bounds().Height())
			canvas->MoveTo ((width - cw)/2, (height - ch)/2);
		else if (cw < Bounds().Width())
			canvas->MoveTo ((width - cw)/2, min_c (0, cFrame.top));
		else if (ch < Bounds().Height())
			canvas->MoveTo (min_c (0, cFrame.left), (height - ch)/2);
		else
			canvas->MoveTo (min_c (0, cFrame.left), min_c (0, cFrame.top));
		cFrame = canvas->Frame();
	}
	float hmin, hmax, vmin, vmax;
	BScrollBar *h = ScrollBar (B_HORIZONTAL);
	BScrollBar *v = ScrollBar (B_VERTICAL);
	h->GetRange (&hmin, &hmax);
	v->GetRange (&vmin, &vmax);
//	printf ("vrange = %f\n", fFrame.Height()*fScale - height);
	h->SetRange (hmin, max_c (0, fFrame.Width()*fScale - width));
	v->SetRange (vmin, max_c (0, fFrame.Height()*fScale - height));
	h->SetProportion (min_c (1, width/fScale/fFrame.Width()));
	v->SetProportion (min_c (1, height/fScale/fFrame.Height()));
	h->SetSteps (1, Bounds().Width());
	v->SetSteps (1, Bounds().Height());
	h->SetValue (fFrame.left*fScale - cFrame.left);
	v->SetValue (fFrame.top*fScale - cFrame.top);
}

void BGView::setScale (float s)
{
	fScale = s;
	FrameResized (Bounds().Width(), Bounds().Height());
	Invalidate();
}

void BGView::setFrame (BRect frame)
{
	fFrame = frame;
	FrameResized (frame.Width(), frame.Height());
}
