#include "MagView.h"
#include "MagWindow.h"
#include "Colors.h"
#include "ColorMenuButton.h"
#include "Becasso.h"
#include <SupportDefs.h>

MagView::MagView (BRect frame, const char *name, CanvasView *_myView)
	: BView (frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED)
{
	grid_visible = 1;
	pos = BPoint (0, 0);
	myView = _myView;
	undoIndex = -1;
	redoIndex = 0;
	mouse_on_canvas = false;
	fPicking = false;
}

MagView::~MagView ()
{
}

bool MagView::contains (BPoint p) const
{
	BPoint r = BPoint (p.x*zoom, p.y*zoom);
	return (Bounds().Contains (r));
}

void MagView::Draw (BRect update)
{
	// You won't believe how simple this is.
	// I know about the one-pixel-off bug.
	SetDrawingMode (B_OP_COPY);
	BRect source, ibounds;
	BBitmap *canvas = new BBitmap (myView->canvasFrame(), B_RGBA32);
	source.Set (int (update.left/zoom), int (update.top/zoom), 
				int (update.right/zoom) + 1, int (update.bottom/zoom) + 1);	
	ibounds.Set (int (source.left)*zoom, int (source.top)*zoom, 
				int (source.right + 1)*zoom, int (source.bottom + 1)*zoom);
	myView->ConstructCanvas (canvas, source, false);
	DrawBitmapAsync (canvas, source, ibounds);

	if (grid_visible)
	{
		rgb_color Hi;
		if (grid_visible == 1)
			Hi = Black;
		else
			Hi = White;
		BeginLineArray ((update.right - update.left
			+ update.bottom - update.top)/zoom + 8);
		for (long x = long (update.left/zoom - 1); x <= long (update.right/zoom + 1); x++)
			AddLine (BPoint (x*zoom, update.top), 
				BPoint (x*zoom, update.bottom), Hi);
		for (long y = long (update.top/zoom - 1); y <= long (update.bottom/zoom + 1); y++)
			AddLine (BPoint (update.left, y*zoom),
				BPoint (update.right, y*zoom), Hi);
		EndLineArray();
	}
	Sync();
	delete canvas;
}

void MagView::MouseDown (BPoint point)
{
	extern ColorMenuButton *locolor, *hicolor;
	BPoint p = BPoint (int (point.x/zoom), int (point.y/zoom));
	BRect pix;
	uint32 buttons = Window()->CurrentMessage()->FindInt32 ("buttons");
	rgb_color hi;
	if (modifiers() & B_OPTION_KEY)
	{
		if (buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY))
			hicolor->set (myView->getColor (p.x, p.y));
		if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
			locolor->set (myView->getColor (p.x, p.y));
		fPicking = true;
	}
	else if (!fPicking)
	{
		if (buttons & B_PRIMARY_MOUSE_BUTTON)
			hi = hicolor->color ();
		if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
			hi = locolor->color ();
		BPoint prevpoint = B_ORIGIN;
		while (buttons)
		{
			if (prevpoint != p)
			{
				SetHighColor (hi);
				if (myView->LockLooper())
				{
					SetupUndo (p, myView->get_and_plot (p, hi));
					myView->Invalidate (BRect (p.x - 1, p.y - 1, p.x + 1, p.y + 1));
					myView->UnlockLooper();
				}
				snooze (30*1000);
				pix.Set (p.x*zoom, p.y*zoom, p.x*zoom + zoom - 1, p.y*zoom + zoom - 1);
				FillRect (pix);
				if (grid_visible)
				{
					rgb_color Hi;
					if (grid_visible == 1)
						Hi = Black;
					else
						Hi = White;
					SetHighColor (Hi);
					pix.right++;
					pix.bottom++;
					StrokeRect (pix);
				}			
				prevpoint = p;
				GetMouse (&point, &buttons);
				BRect bounds = Bounds();
				if (point.x > bounds.right)		// Still flickers...
					ScrollBy (zoom*(point.x - bounds.right), 0);
				if (point.x < bounds.left && bounds.left > 0)
					ScrollBy (zoom*(point.x - bounds.left), 0);
				if (point.y > bounds.bottom)	// Ditto.  Think of a nice test.  (I know what you're thinking, but I tried that...)
					ScrollBy (0, zoom*(point.y - bounds.bottom));
				if (point.y < bounds.top && bounds.top > 0)
					ScrollBy (0, zoom*(point.y - bounds.top));
			}
			else
				GetMouse (&point, &buttons);

			p = BPoint (int (point.x/zoom), int (point.y/zoom));
	
			snooze (20000);
		}
	}
//	Sync ();
}

void MagView::MouseMoved (BPoint point, uint32 transit, const BMessage *msg)
{
	extern Becasso *mainapp;
	if (transit == B_EXITED_VIEW)
	{
		mainapp->setHand();
		mouse_on_canvas = false;
		// Invalidate();
	}
	else if (transit == B_ENTERED_VIEW && Window()->IsActive() && !msg)
	{
		mouse_on_canvas = true;
	}

	int32 buttons;
	Window()->CurrentMessage()->FindInt32 ("buttons", &buttons);
	if (modifiers() & B_OPTION_KEY)	// Color Picker
	{
		extern ColorMenuButton *hicolor, *locolor;
		BPoint p = BPoint (int (point.x/zoom), int (point.y/zoom));
		if (buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY))
			hicolor->set (myView->getColor (p.x, p.y));
		if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
			locolor->set (myView->getColor (p.x, p.y));
		fPicking = true;
	}
}

void MagView::Pulse ()
{
	BPoint point;
	uint32 buttons;
	GetMouse (&point, &buttons, true);

	extern Becasso *mainapp;
	if (modifiers() & B_OPTION_KEY && mouse_on_canvas)	// Color Picker
	{
		mainapp->setPicker (true);
		fPicking = true;
	}
	else if (fPicking)
	{
		mainapp->setPicker (false);
		if (mouse_on_canvas && !buttons)
			mainapp->FixCursor();	// Otherwise: Few pixels off!!  (BeOS bug w.r.t. hot spot of cursors ...)

		fPicking = false;
	}
}

void MagView::setzoom (int z)
{
	zoom = z;
	int maxw = int (zoom*myView->canvasFrame().IntegerWidth() + B_V_SCROLL_BAR_WIDTH);
	int maxh = int (zoom*myView->canvasFrame().IntegerHeight() + B_H_SCROLL_BAR_HEIGHT +
		((MagWindow *)Window())->menubarHeight() + 1);
	Window()->SetSizeLimits (64, maxw, 64, maxh);
	Window()->ResizeTo (min_c (maxw, Window()->Frame().Width()),
		min_c (maxh, Window()->Frame().Height()));
	FrameResized (Bounds().Width(), Bounds().Height());
	Draw (Bounds ());
}

float MagView::currentzoom ()
{
	return (zoom);
}

void MagView::setgrid (int g)
{
	grid_visible = g;
	Draw (Bounds ());
}

void MagView::setScrollBars (BScrollBar *_h, BScrollBar *_v)
{
	mh = _h;
	mv = _v;
}

void MagView::AttachedToWindow ()
{
	setzoom (8);
	FrameResized (Bounds().Width(), Bounds().Height());
	SetViewColor (B_TRANSPARENT_32_BIT);
}

void MagView::FrameResized (float width, float height)
{
	float hmin, hmax, vmin, vmax;
	mh->GetRange (&hmin, &hmax);
	mv->GetRange (&vmin, &vmax);
	mh->SetRange (hmin, myView->canvasFrame().Width()*zoom - width + zoom - 1);
	mv->SetRange (vmin, myView->canvasFrame().Height()*zoom - height + zoom - 1);
	mh->SetProportion (Bounds().IntegerWidth()/zoom/myView->canvasFrame().Width());
	mv->SetProportion (Bounds().IntegerHeight()/zoom/myView->canvasFrame().Height());
	mh->SetSteps (zoom, zoom*int (Bounds().IntegerWidth()/zoom));
	mv->SetSteps (zoom, zoom*int (Bounds().IntegerHeight()/zoom));
}

bool MagView::CanUndo ()
{
	return (undoIndex >= 0);
}

bool MagView::CanRedo ()
{
	return (redoIndex > undoIndex);
}

void MagView::SetupUndo (BPoint p, uint32 c)
{
	if (undoIndex < MAX_MAG_UNDO - 1)
	{
		++undoIndex;
		redoIndex = undoIndex;
		undo[undoIndex].x = uint16 (p.x);
		undo[undoIndex].y = uint16 (p.y);
		undo[undoIndex].c = c;
//		printf ("Setup undo in %ld\n", undoIndex);
	}
	else
	{
		for (uint32 i = 0; i < MAX_MAG_UNDO - 1; i++)
			undo[i] = undo[i + 1];
		undo[MAX_MAG_UNDO - 1].x = uint16 (p.x);
		undo[MAX_MAG_UNDO - 1].y = uint16 (p.y);
		undo[MAX_MAG_UNDO - 1].c = c;
	}
}

status_t MagView::Undo ()
{
	if (undoIndex >= 0)
	{
		BPoint p = BPoint (undo[undoIndex].x, undo[undoIndex].y);
		if (myView->LockLooper())
		{
			uint32 c = undo[undoIndex].c;
			rgb_color col = {RED(c), GREEN(c), BLUE(c), ALPHA(c)};
			undo[undoIndex].c = myView->get_and_plot (p, col);
			myView->Invalidate (BRect (p.x - 1, p.y - 1, p.x + 1, p.y + 1));
			myView->UnlockLooper();
//			printf ("Undid %ld, index now %ld\n", undoIndex, undoIndex - 1);
			--undoIndex;
			BRect pix;
			pix.Set (p.x*zoom, p.y*zoom,
				p.x*zoom + zoom - 1, p.y*zoom + zoom - 1);
			SetHighColor (col);
			FillRect (pix);
			if (LockLooper())
			{
				if (grid_visible)
				{
					rgb_color Hi;
					if (grid_visible == 1)
						Hi = Black;
					else
						Hi = White;
					SetHighColor (Hi);
					pix.right++;
					pix.bottom++;
					StrokeRect (pix);
				}
				UnlockLooper();
			}
		}
		return B_OK;
	}
	else
		return B_ERROR;	// Nothing to undo...
}

status_t MagView::Redo ()
{
	if (1 || redoIndex < MAX_MAG_UNDO && redoIndex > 0)
	{
		if (myView->LockLooper())
		{
			++undoIndex;
			BPoint p = BPoint (undo[undoIndex].x, undo[undoIndex].y);
			uint32 c = undo[undoIndex].c;
			rgb_color col = {RED(c), GREEN(c), BLUE(c), ALPHA(c)};
			undo[undoIndex].c = myView->get_and_plot (p, col);
			myView->Invalidate (BRect (p.x - 1, p.y - 1, p.x + 1, p.y + 1));
			myView->UnlockLooper();
			BRect pix;
			pix.Set (p.x*zoom, p.y*zoom,
				p.x*zoom + zoom - 1, p.y*zoom + zoom - 1);
			SetHighColor (col);
			FillRect (pix);
			if (LockLooper())
			{
				if (grid_visible)
				{
					rgb_color Hi;
					if (grid_visible == 1)
						Hi = Black;
					else
						Hi = White;
					SetHighColor (Hi);
					pix.right++;
					pix.bottom++;
					StrokeRect (pix);
				}
				UnlockLooper();
			}
		}
		
		return B_OK;
	}
	else
		return B_ERROR;
}
