#include <Screen.h>
#include <TextControl.h>
#include <Clipboard.h>
#include "CanvasView.h"
#include "CanvasWindow.h"
#include "Becasso.h"
#include "PicMenuButton.h"
#include "ColorMenuButton.h"
#include "PatternMenuButton.h"
#include "AttribDraw.h"
#include "AttribSelect.h"
#include "AttribBrush.h"
#include "AttribClone.h"
#include "AttribEraser.h"
#include "AttribFill.h"
#include "AttribText.h"
#include "AttribSpraycan.h"
#include "AttribFreehand.h"
#include "AttribLines.h"
#include "AttribPolyblob.h"
#include "AttribPolygon.h"
#include "AttribRect.h"
#include "AttribRoundRect.h"
#include "AttribCircle.h"
#include "AttribEllipse.h"
#include "Modes.h"
#include "Tools.h"
#include "hsv.h"
#include "Brush.h"
#include "BitmapStuff.h"
#include "Tablet.h"
#include "Position.h"
#include "BecassoAddOn.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "PosView.h"

// #define USE_THREAD_FOR_POSITION
#define FIX_STROKELINE_BUG
#define USE_MOUSEMOVED	// instead of polling - works since 4.5

void CanvasView::tBrush (int32 mode, BPoint point, uint32 buttons)
{
//	printf ("Top of tBrush\n");
	Position position;
	GetPosition (&position);
	extern ColorMenuButton *hicolor, *locolor;
	extern AttribBrush *tAttribBrush;
	float Spacing = tAttribBrush->getSpacing();
	int _w = tAttribBrush->getBrush()->Width();
	int _h = tAttribBrush->getBrush()->Height();
	float cx = _w/2;
	float cy = _h/2;
	extern AttribDraw *mAttribDraw;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	drawView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	SetupUndo (mode);
	drawView->MovePenTo (point);
	MovePenTo (point);
	if (buttons & B_PRIMARY_MOUSE_BUTTON)
	{
		drawView->SetHighColor (hicolor->color());
		drawView->SetLowColor (locolor->color());
		SetHighColor (hicolor->color());
		SetLowColor (locolor->color());
	}
	if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
	{
		drawView->SetHighColor (locolor->color());
		drawView->SetLowColor (hicolor->color());
		SetHighColor (locolor->color());
		SetLowColor (hicolor->color());
	}
	Brush *b = tAttribBrush->getBrush();
	BBitmap *brushbm = b->ToBitmap (HighColor());
	SView::setSnoozeTime (max_c (20 * b->Height() * b->Width(), 20000));
	int strength = position.fPressure;

	twi = brushbm->Bounds().IntegerWidth();
	thi = brushbm->Bounds().IntegerHeight();
	bwi = fCanvasFrame.IntegerWidth();
	bhi = fCanvasFrame.IntegerHeight();
	
	fBC.cx = cx;
	fBC.cy = cy;
	fBC.Spacing = Spacing;
	fBC.b = b;
	fBC.brushbm = brushbm;
	fBC.pstrength = strength;
	
//	uchar *bits = (uchar *) currentLayer()->Bits();
//	long bytesperrow = currentLayer()->BytesPerRow();
	tbits = (uchar *) brushbm->Bits();
	tbpr = brushbm->BytesPerRow();
	bbits = (uchar *) currentLayer()->Bits();
	bbpr = currentLayer()->BytesPerRow();
	SetHighColor (Black);
	if (abs (strength) > fTreshold)
	{
		if (mode == M_SELECT)
		{
			AddToSelection (b, selection, int (point.x - cx), int (point.y - cy), buttons & B_SECONDARY_MOUSE_BUTTON ? -strength : strength);
		}
		else if (dm == B_OP_COPY || dm == B_OP_OVER)
		{
			FastAddWithAlpha (point.x - cx, point.y - cy, strength);
		}
		else
		{
			DrawBitmapAsync (brushbm, BPoint (point.x - cx, point.y - cy));
			drawView->DrawBitmap (brushbm, BPoint (point.x - cx, point.y - cy));
		}
	}
	// For immediate feedback at the first click:
	Invalidate (BRect (point.x - cx - 1, point.y - cy - 1, point.x + cx + 1, point.y + cy + 1));
	BPoint pos = point;
	fBC.pos = pos;
#if !defined (USE_MOUSEMOVED)
	BPoint nextPos;
//	BRect frame = fCanvasFrame;
	int pstrength = strength;
	while (buttons)
	{
		register float dx = pos.x - point.x;
		register float dy = pos.y - point.y;
		register float ds = dx*dx + dy*dy;
		register float distance = sqrt (ds);
		BRect aR = BRect (point.x - cx - 1, point.y - cy - 1, point.x + cx + 1, point.y + cy + 1);
		BRect bR = BRect (pos.x - cx - 1, pos.y - cy - 1, pos.x + cx + 1, pos.y + cy + 1);
		if (distance > Spacing)
		{
			clock_t start, end;
			start = clock();
			register float num = distance/Spacing;
			register float facx = (point.x - pos.x)/num;
			register float facy = (point.y - pos.y)/num;
			register float facs = (strength - pstrength)/num;
			for (float i = 1; i < num; i++)
			{
				nextPos = BPoint (pos.x + i*facx, pos.y + i*facy);
				if (mode == M_SELECT)
				{
					AddToSelection (b, selection, int (nextPos.x - cx), int (nextPos.y - cy), int (pstrength + i*facs));
				}
				else // if (dm == B_OP_COPY || dm == B_OP_OVER)
				{
					FastAddWithAlpha (int (nextPos.x - cx), int (nextPos.y - cy), int (pstrength + i*facs));
				}
//				else
//				{
//					DrawBitmapAsync (brushbm, BPoint (nextPos.x - cx, nextPos.y - cy));
//					drawView->DrawBitmap (brushbm, BPoint (nextPos.x - cx, nextPos.y - cy));
//					drawView->Sync();
//					selectionView->Sync();
//				}
			}
			pos = nextPos;
			Invalidate (aR | bR);
//			DrawBitmap (currentLayer(), dirty, dirty);
			end = clock();
			extern int DebugLevel;
			if (DebugLevel > 7)
				printf ("Brush Stroke took %ld ms\n", end - start);
		}
		snooze (10000);
		// We're not being good citizens, but Beatware's brush is faster...
		myWindow->Lock();
		myWindow->posview->Pulse();
		myWindow->Unlock();
		ScrollIfNeeded (point);
		// GetMouse (&point, &buttons, true);
		GetPosition (&position);
		point = position.fPoint;
		buttons = position.fButtons;
		pstrength = strength;
		strength = position.fPressure;
		if (buttons & B_TERTIARY_MOUSE_BUTTON)
			strength = -strength;
	}
	Invalidate();
#endif
}


void CanvasView::tBrushM (int32 mode, BPoint point, uint32 buttons, int strength, BPoint /*tilt*/)
{
//	printf ("Top of tBrushM: (%.0f, %.0f), %i (%.3f, %.3f)\n", point.x, point.y, strength, tilt.x, tilt.y);
	BPoint pos = fBC.pos;
	BPoint nextPos;
	float cx = fBC.cx;
	float cy = fBC.cy;
	float Spacing = fBC.Spacing;
	Brush *b = fBC.b;
	int pstrength = fBC.pstrength;
//	BBitmap *brushbm = fBC.brushbm;
	
	register float dx = pos.x - point.x;
	register float dy = pos.y - point.y;
	register float ds = dx*dx + dy*dy;
	register float distance = sqrt (ds);
	BRect aR = BRect (point.x - cx - 1, point.y - cy - 1, point.x + cx + 1, point.y + cy + 1);
	BRect bR = BRect (pos.x - cx - 1, pos.y - cy - 1, pos.x + cx + 1, pos.y + cy + 1);
	
//	extern AttribDraw *mAttribDraw;
//	drawing_mode dm = mAttribDraw->getDrawingMode();
//	SetDrawingMode (dm);
	if (distance > Spacing)
	{
		clock_t start, end;
		start = clock();
		register float num = distance/Spacing;
		register float facx = (point.x - pos.x)/num;
		register float facy = (point.y - pos.y)/num;
		register float facs = (strength - pstrength)/num;
		for (float i = 1; i < num; i++)
		{
			nextPos = BPoint (pos.x + i*facx, pos.y + i*facy);
			if (mode == M_SELECT)
			{
				AddToSelection (b, selection, int (nextPos.x - cx), int (nextPos.y - cy), buttons & B_SECONDARY_MOUSE_BUTTON ? -int (pstrength + i*facs) : int (pstrength + i*facs));
			}
			else // if (dm == B_OP_COPY || dm == B_OP_OVER)
			{
				FastAddWithAlpha (int (nextPos.x - cx), int (nextPos.y - cy), int (pstrength + i*facs));
			}
//			else
//			{
//				drawView->SetDrawingMode (dm);
//				DrawBitmapAsync (brushbm, BPoint (nextPos.x - cx, nextPos.y - cy));
//				drawView->DrawBitmap (brushbm, BPoint (nextPos.x - cx, nextPos.y - cy));
//				drawView->Sync();
//			}
		}
		pos = nextPos;
		Invalidate (aR | bR);
//			DrawBitmap (currentLayer(), dirty, dirty);
		end = clock();
		extern int DebugLevel;
		if (DebugLevel > 7)
			printf ("Brush Stroke took %ld ms\n", end - start);
		fBC.pos = pos;
	}
//	snooze (10000);
//	// We're not being good citizens, but Beatware's brush is faster...
//	myWindow->Lock();
//	myWindow->posview->Pulse();
//	myWindow->Unlock();
	ScrollIfNeeded (point);
	// GetMouse (&point, &buttons, true);
//	GetPosition (&position);
//	point = position.fPoint;
//	buttons = position.fButtons;
	fBC.pstrength = strength;
//	strength = position.fPressure;
//	if (buttons & B_TERTIARY_MOUSE_BUTTON)
//		strength = -strength;
}


void CanvasView::tClone (int32 mode, BPoint point, uint32 buttons)
{
//	printf ("Top of tClone\n");
	Position position;
	GetPosition (&position);
	extern AttribClone *tAttribClone;
	float Spacing = tAttribClone->getSpacing();
	int _w = tAttribClone->getBrush()->Width();
	int _h = tAttribClone->getBrush()->Height();
	float cx = _w/2;
	float cy = _h/2;
	extern AttribDraw *mAttribDraw;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	drawView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	SetupUndo (mode);
	drawView->MovePenTo (point);
	MovePenTo (point);
	bbits = (uchar *) currentLayer()->Bits();
	bbpr = currentLayer()->BytesPerRow();
	bbprl = bbpr/4;
	bwi = currentLayer()->Bounds().IntegerWidth();
	bhi = currentLayer()->Bounds().IntegerHeight();
	extern Becasso *mainapp;
	if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
	{
		mainapp->setCCross();
		fCC.offset.x = point.x - fCC.pos.x;
		fCC.offset.y = point.y - fCC.pos.y;
	}
	if (buttons & B_PRIMARY_MOUSE_BUTTON)
	{
		if (mode == M_SELECT)
			mainapp->setSelect();
		else
			mainapp->setCross();

		Brush *b = tAttribClone->getBrush();
		SView::setSnoozeTime (max_c (20 * b->Height() * b->Width(), 20000));
		int strength = position.fPressure;
	
		fCC.cx = cx;
		fCC.cy = cy;
		fCC.Spacing = Spacing;
		fCC.b = b;
		fCC.pstrength = strength;
		
		if (abs (strength) > fTreshold)
		{
			int32 h = b->Height();	// note: brush width/height is num pixels!
			int32 w = b->Width();
			uint8 *bd = b->Data();
			int32 ch = currentLayer()->Bounds().IntegerHeight();
			int32 cw = currentLayer()->Bounds().IntegerWidth();
			BBitmap *copy = new BBitmap (BRect (0, 0, w, h), B_RGB32);
			tbits = (uint8 *) copy->Bits();
			tbpr = copy->BytesPerRow();
			twi = w - 1;
			thi = h - 1;

			if (mode == M_SELECT)
			{
			}
			else
			{
				// first, extract the cloned area into "copy" with alpha replaced by the brush values
				uint32 *cbits = (uint32*) copy->Bits();
				uint32 *dbits = (uint32*) bbits;
				uint32 cbprl = copy->BytesPerRow()/4;
				int32 off_x = int32 (point.x + fCC.offset.x - w/2);
				int32 off_y = int32 (point.y + fCC.offset.y - h/2);
				for (int y = 0; y < h; y++)
				{
					for (int x = 0; x < w; x++)
					{
						cbits[y*cbprl + x] = (y + off_y <= ch && x + off_x <= cw && y + off_y >= 0 && x + off_x >= 0) ? 
							((dbits[(y + off_y)*bbprl + x + off_x] & COLOR_MASK) | bd[y*w + x] << ALPHA_BPOS) : 0;
//								(0xFF00FF00 & COLOR_MASK | bd[y*w + x] << ALPHA_BPOS) : 0;
					}
				}
				
				// next, add the copy back to the cursor position
				FastAddWithAlpha (point.x - w/2, point.y - h/2, strength);
			}
			delete copy;
		}
		// Draw a little cursor at the clone source spot
		BPoint ctr;
		ctr.x = point.x + fCC.offset.x;
		ctr.y = point.y + fCC.offset.y;
		SetLowColor (White);
		SetHighColor (Black);
		StrokeLine (BPoint (ctr.x - 1, ctr.y - 3), BPoint (ctr.x - 1, ctr.y + 3), B_SOLID_LOW);
		StrokeLine (BPoint (ctr.x + 1, ctr.y - 3), BPoint (ctr.x + 1, ctr.y + 3), B_SOLID_LOW);
		StrokeLine (BPoint (ctr.x - 3, ctr.y - 1), BPoint (ctr.x + 3, ctr.y - 1), B_SOLID_LOW);
		StrokeLine (BPoint (ctr.x - 3, ctr.y + 1), BPoint (ctr.x + 3, ctr.y + 1), B_SOLID_LOW);
		StrokeLine (BPoint (ctr.x - 3, ctr.y), BPoint (ctr.x + 3, ctr.y));
		StrokeLine (BPoint (ctr.x, ctr.y - 3), BPoint (ctr.x, ctr.y + 3));
		fCC.prevctr = ctr;
	}
	
	// For immediate feedback at the first click:
	Invalidate (BRect (point.x - cx - 1, point.y - cy - 1, point.x + cx + 1, point.y + cy + 1));
	fCC.pos = point;
}


void CanvasView::tCloneM (int32 mode, BPoint point, uint32 buttons, int strength, BPoint /*tilt*/)
{
//	printf ("Top of tCloneM: (%.0f, %.0f), %i (%.3f, %.3f)\n", point.x, point.y, strength, tilt.x, tilt.y);
	BPoint pos = fCC.pos;
	BPoint nextPos;
	float cx = fCC.cx;
	float cy = fCC.cy;
	float Spacing = fCC.Spacing;
	Brush *b = fCC.b;
	int pstrength = fCC.pstrength;
	
	register float dx = pos.x - point.x;
	register float dy = pos.y - point.y;
	register float ds = dx*dx + dy*dy;
	register float distance = sqrt (ds);
	BRect aR = BRect (point.x - cx - 1, point.y - cy - 1, point.x + cx + 1, point.y + cy + 1);
	BRect bR = BRect (pos.x - cx - 1, pos.y - cy - 1, pos.x + cx + 1, pos.y + cy + 1);
	extern Becasso *mainapp;
	if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
	{
		mainapp->setCCross();
		fCC.offset.x = point.x - fCC.pos.x;
		fCC.offset.y = point.y - fCC.pos.y;
		BPoint pos = PenLocation();
		BRect frame = PRect (pos.x, pos.y, point.x, point.y);
		BRect pframe = PRect (pos.x, pos.y, fCC.pos.x, fCC.pos.y);
		BRect tframe = frame | pframe;
		Invalidate (tframe);
		SetHighColor (Red);
		StrokeLine (fCC.pos, point);
	}
	if (buttons & B_PRIMARY_MOUSE_BUTTON)
	{
		if (mode == M_SELECT)
			mainapp->setSelect();
		else
			mainapp->setCross();
		if (distance > Spacing)
		{
			clock_t start, end;
			start = clock();
			int32 h = b->Height();	// note: brush width/height is num pixels!
			int32 w = b->Width();
			uint8 *bd = b->Data();
			int32 ch = currentLayer()->Bounds().IntegerHeight();
			int32 cw = currentLayer()->Bounds().IntegerWidth();
			BBitmap *copy = new BBitmap (BRect (0, 0, w, h), B_RGB32);
			tbits = (uint8 *) copy->Bits();
			tbpr = copy->BytesPerRow();
			twi = w - 1;
			thi = h - 1;
			float num = distance/Spacing;
			float facx = (point.x - pos.x)/num;
			float facy = (point.y - pos.y)/num;
			float facs = (strength - pstrength)/num;
			for (float i = 1; i < num; i++)
			{
				nextPos = BPoint (pos.x + i*facx, pos.y + i*facy);
				if (mode == M_SELECT)
				{
				}
				else
				{
					// first, extract the cloned area into "copy" with alpha replaced by the brush values
					uint32 *cbits = (uint32*) copy->Bits();
					uint32 *dbits = (uint32*) bbits;
					uint32 cbprl = copy->BytesPerRow()/4;
					int32 off_x = int32 (nextPos.x + fCC.offset.x - w/2);
					int32 off_y = int32 (nextPos.y + fCC.offset.y - h/2);
					for (int y = 0; y < h; y++)
					{
						for (int x = 0; x < w; x++)
						{
							cbits[y*cbprl + x] = (y + off_y <= ch && x + off_x <= cw && y + off_y >= 0 && x + off_x >= 0) ? 
								((dbits[(y + off_y)*bbprl + x + off_x] & COLOR_MASK) | bd[y*w + x] << ALPHA_BPOS) : 0;
//								(0xFF00FF00 & COLOR_MASK | bd[y*w + x] << ALPHA_BPOS) : 0;
						}
					}
					
					// next, add the copy back to the cursor position
					FastAddWithAlpha (nextPos.x - w/2, nextPos.y - h/2, int (pstrength + i*facs));
				}
			}
			pos = nextPos;
			delete copy;
			
			// Draw a little cursor at the clone source spot
			BPoint ctr;
			ctr.x = point.x + fCC.offset.x;
			ctr.y = point.y + fCC.offset.y;
			SetLowColor (White);
			SetHighColor (Black);
			Invalidate (BRect (fCC.prevctr.x - 3, fCC.prevctr.y - 3, fCC.prevctr.x + 3, fCC.prevctr.y + 3));
			StrokeLine (BPoint (ctr.x - 1, ctr.y - 3), BPoint (ctr.x - 1, ctr.y + 3), B_SOLID_LOW);
			StrokeLine (BPoint (ctr.x + 1, ctr.y - 3), BPoint (ctr.x + 1, ctr.y + 3), B_SOLID_LOW);
			StrokeLine (BPoint (ctr.x - 3, ctr.y - 1), BPoint (ctr.x + 3, ctr.y - 1), B_SOLID_LOW);
			StrokeLine (BPoint (ctr.x - 3, ctr.y + 1), BPoint (ctr.x + 3, ctr.y + 1), B_SOLID_LOW);
			StrokeLine (BPoint (ctr.x - 3, ctr.y), BPoint (ctr.x + 3, ctr.y));
			StrokeLine (BPoint (ctr.x, ctr.y - 3), BPoint (ctr.x, ctr.y + 3));
			fCC.prevctr = ctr;
			
			Invalidate (aR | bR);
//			DrawBitmap (currentLayer(), dirty, dirty);
			end = clock();
			extern int DebugLevel;
			if (DebugLevel > 7)
				printf ("Clone Stroke took %ld ms\n", end - start);
			fCC.pos = pos;
		}
	}
	ScrollIfNeeded (point);
	fCC.pstrength = strength;
}


void CanvasView::tTablet (int32 mode)
{
//	printf ("tTablet\n");
	extern Becasso *mainapp;
	extern ColorMenuButton *hicolor, *locolor;
	extern AttribBrush *tAttribBrush;
	extern Tablet *wacom;
	#if defined (USE_THREAD_FOR_POSITION)
		extern port_id position_port;
		thread_id position_thread = spawn_thread (position_tracker, "Position Tracker", B_DISPLAY_PRIORITY, this);
	#endif
	wacom->SetScale (fCanvasFrame.Width(), fCanvasFrame.Height());
	#if defined (USE_THREAD_FOR_POSITION)
		resume_thread (position_thread);
	#endif
	float Spacing = tAttribBrush->getSpacing();
	int _w = tAttribBrush->getBrush()->Width();
	int _h = tAttribBrush->getBrush()->Height();
	float cx = _w/2;
	float cy = _h/2;
	float borderx = max_c (cx + 1, 7);
	float bordery = max_c (cy + 1, 7);
//	Window()->Lock();
	currentLayer()->Lock();
	selection->Lock();
	extern AttribDraw *mAttribDraw;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	drawView->SetDrawingMode (dm);
	//printf ("Setting up Undo\n");
	SetupUndo (mode);
	Position position;
	#if defined (USE_THREAD_FOR_POSITION)
		int32 msg_code;
		read_port (position_port, &msg_code, (void *) &position, sizeof (Position));
	#else
		GetPosition (&position);
	#endif
	BPoint point = position.fPoint;
	drawView->MovePenTo (point);
	uint32 buttons = position.fButtons;
	int strength = position.fPressure;
	MovePenTo (point);
	mainapp->HideCursor();
	if (buttons & B_PRIMARY_MOUSE_BUTTON)	// Drawing
	{
		drawView->SetHighColor (hicolor->color());
		drawView->SetLowColor (locolor->color());
		SetHighColor (hicolor->color());
		SetLowColor (locolor->color());
	}
	if (buttons & B_SECONDARY_MOUSE_BUTTON)	// Erasing
	{
		drawView->SetHighColor (locolor->color());
		drawView->SetLowColor (hicolor->color());
		SetHighColor (locolor->color());
		SetLowColor (hicolor->color());
		strength = -strength;
	}
	//printf ("Preparing brush\n");
	Brush *b = tAttribBrush->getBrush();
	BBitmap *brushbm = b->ToBitmap (HighColor());
	SView::setSnoozeTime (max_c (20 * b->Height() * b->Width(), 20000));
	twi = brushbm->Bounds().IntegerWidth();
	thi = brushbm->Bounds().IntegerHeight();
	bwi = fCanvasFrame.IntegerWidth();
	bhi = fCanvasFrame.IntegerHeight();
//	uchar *bits = (uchar *) currentLayer()->Bits();
//	long bytesperrow = currentLayer()->BytesPerRow();
	tbits = (uchar *) brushbm->Bits();
	tbpr = brushbm->BytesPerRow();
	bbits = (uchar *) currentLayer()->Bits();
	bbpr = currentLayer()->BytesPerRow();
	SetHighColor (Black);
	SetDrawingMode (B_OP_INVERT);
	SetScale (fScale);
	StrokeLine (BPoint (point.x - 5, point.y), BPoint (point.x + 5, point.y));
	StrokeLine (BPoint (point.x, point.y - 5), BPoint (point.x, point.y + 5));
	SetScale (1);
	SetDrawingMode (dm);
	if (abs (strength) > 255*wacom->Threshold())
	{
		if (mode == M_SELECT)
		{
			AddToSelection (b, selection, int (point.x - cx), int (point.y - cy), int (strength));
		}
		else if (dm == B_OP_COPY || dm == B_OP_OVER)
		{
			FastAddWithAlpha (point.x - cx, point.y - cy, strength);
		}
		else
		{
			DrawBitmapAsync (brushbm, BPoint (point.x - cx, point.y - cy));
			drawView->DrawBitmap (brushbm, BPoint (point.x - cx, point.y - cy));
		}
	}
	BPoint pos = point;
	BPoint nextPos;
//	BRect frame = fCanvasFrame;
	int pstrength = strength;
	bool proximity = true;
	// printf ("Entering loop\n");
	while (proximity && Window()->IsActive())
	{
		register float dx = pos.x - point.x;
		register float dy = pos.y - point.y;
		register float ds = dx*dx + dy*dy;
		register float distance = sqrt (ds);
		BRect aR = BRect (point.x - borderx, point.y - bordery, point.x + borderx, point.y + bordery);
		BRect bR = BRect (pos.x - borderx, pos.y - bordery, pos.x + borderx, pos.y + bordery);
		if (distance > Spacing)
		{
			register float num = distance/Spacing;
			register float facx = (point.x - pos.x)/num;
			register float facy = (point.y - pos.y)/num;
			register float facs = (strength - pstrength)/num;
			for (float i = 1; i < num; i++)
			{
				nextPos = BPoint (pos.x + i*facx, pos.y + i*facy);
				int str = int (strength + i*facs);
				if (abs (str) > 255*wacom->Threshold())
				{
					if (mode == M_SELECT)
					{
						AddToSelection (b, selection, int (nextPos.x - cx), int (nextPos.y - cy), int (strength));
					}
					else if (dm == B_OP_COPY || dm == B_OP_OVER)
					{
						FastAddWithAlpha (int (nextPos.x - cx), int (nextPos.y - cy), int (pstrength + i*facs));
					}
					else
					{
						DrawBitmapAsync (brushbm, BPoint (nextPos.x - cx, nextPos.y - cy));
						drawView->DrawBitmap (brushbm, BPoint (nextPos.x - cx, nextPos.y - cy));
						drawView->Sync();
						selectionView->Sync();
					}
				}
			}
			pos = nextPos;
			Invalidate (aR | bR);
			SetDrawingMode (B_OP_INVERT);
			SetScale (fScale);
			StrokeLine (BPoint (point.x - 5, point.y), BPoint (point.x + 5, point.y));
			StrokeLine (BPoint (point.x, point.y - 5), BPoint (point.x, point.y + 5));
			SetScale (1);
			SetDrawingMode (dm);
//			DrawBitmap (currentLayer(), dirty, dirty);
		}
		Window()->Unlock();
		snooze (10000);
		Window()->Lock();
		// We're not being good citizens, but Beatware's brush is faster...
		myWindow->Lock();
		myWindow->posview->Pulse();
		myWindow->Unlock();
		ScrollIfNeeded (point);
		#if defined (USE_THREAD_FOR_POSITION)
			read_port (position_port, &msg_code, (void *) &position, sizeof (Position));
		#else
			GetPosition (&position);
		#endif
		point = position.fPoint;
		buttons = position.fButtons;
		pstrength = strength;
		strength = position.fPressure;
		proximity = position.fProximity;
		if (buttons & B_SECONDARY_MOUSE_BUTTON)
			strength = -strength;
		// printf ("(%4.0f, %4.0f) %4i\r", point.x, point.y, strength); fflush (stdout);
	}
	// printf ("Exit loop\n");
	selection->Unlock();
	currentLayer()->Unlock();
	mainapp->ShowCursor();
	Invalidate();
}

////////////////////////////
#if defined (__POWERPC__)
#	pragma optimization_level 1
#endif

void CanvasView::tEraser (int32 mode, BPoint point, uint32 buttons)
{
//	clock_t start, end;
	extern AttribEraser *tAttribEraser;
	Position position;
	int type = tAttribEraser->getType();
	SetupUndo (mode);
	float h = tAttribEraser->getYSize();
	float w = tAttribEraser->getXSize();
	BRect eraser = BRect (0, 0, w, h);
	SetHighColor (Black);
	SetLowColor (White);
	drawView->SetLowColor (Transparent);
	selectionView->SetLowColor (SELECT_NONE);
	SetDrawingMode (B_OP_COPY);
	drawView->SetDrawingMode (B_OP_COPY);
	selectionView->SetDrawingMode (B_OP_COPY);
	SetPenSize (1);
	drawView->SetPenSize (0);
	selectionView->SetPenSize (0);
//	BRect frame = fCanvasFrame;
	while (buttons)
	{
//		start = clock();
		eraser.OffsetTo (point.x - w/2, point.y - h/2);
		switch (type)
		{
		case ERASER_RECT:
			if (mode == M_DRAW)
			{
				drawView->FillRect (eraser, B_SOLID_LOW);
//				FillRect (eraser, B_SOLID_LOW);
			}
			else
				selectionView->FillRect (eraser, B_SOLID_LOW);
			/*S*/StrokeRect (eraser);
			break;
		case ERASER_ELLIPSE:
			if (mode == M_DRAW)
			{
				drawView->FillEllipse (eraser, B_SOLID_LOW);
//				FillEllipse (eraser, B_SOLID_LOW);
			}
			else
				selectionView->FillEllipse (eraser, B_SOLID_LOW);
			/*S*/StrokeEllipse (eraser);
			break;
		default:
			fprintf (stderr, "Unknown Eraser type...\n");
		}
		drawView->Sync();
		selectionView->Sync();
		Sync();
		Invalidate (eraser);
		snooze (20000);
		ScrollIfNeeded (point);
		myWindow->Lock();
		myWindow->posview->Pulse();
		myWindow->Unlock();

//		GetMouse (&point, &buttons, true);
		GetPosition (&position);
		point = position.fPoint;
		buttons = position.fButtons;
//		end = clock();
//		printf ("Took %d ms\n", end - start);
	}
	Invalidate();
	Window()->UpdateIfNeeded ();
}

void CanvasView::tEraserM (int32 /* mode */, BPoint point)
{
	extern AttribEraser *tAttribEraser;
	int type = tAttribEraser->getType();
	float h = tAttribEraser->getYSize();
	float w = tAttribEraser->getXSize();
	BRect eraser = BRect (0, 0, w, h);
	eraser.OffsetTo (point.x - w/2, point.y - h/2);
	SetPenSize (1);
	SetDrawingMode (B_OP_INVERT);
	SetHighColor (Black);
	Invalidate (prev_eraser | eraser);
	prev_eraser = eraser;
	Window()->UpdateIfNeeded();
	switch (type)
	{
	case ERASER_RECT:
		/*S*/StrokeRect (eraser);
		break;
	case ERASER_ELLIPSE:
		/*S*/StrokeEllipse (eraser);	// THIS DOESN'T DO ANYTHING?!
		break;
	default:
		fprintf (stderr, "Unknown Eraser type...\n");
	}
}

void CanvasView::tText (int32 mode, BPoint point, uint32 buttons)
{
#if defined (USE_OLD_TEXT_TOOL)
	if (text)
		return;		// The old BTextControl is still there...
	text = new BTextControl (BRect (point.x*fScale, point.y*fScale, point.x*fScale, point.y*fScale), "tTextControl", NULL, NULL, new BMessage ('tTed'));
	extern AttribText *tAttribText;
	extern ColorMenuButton *locolor, *hicolor;
	SetupUndo (mode);
	if (mode == M_DRAW)
	{
		if (fButtons & B_PRIMARY_MOUSE_BUTTON)
		{
			SetHighColor (hicolor->color());
			SetLowColor (locolor->color());
		}
		if (fButtons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
		{
			SetHighColor (locolor->color());
			SetLowColor (hicolor->color());
		}
	}
	else
	{
		SetHighColor (Black);
		SetLowColor (White);
	}
	BFont font = tAttribText->getFont();
	font.SetSpacing (B_BITMAP_SPACING);
	fMode = mode;
	fButtons = buttons;
	fPoint = point;
//	text->TextView()->SetScale (fScale);	// Fix this once.
	rgb_color hi = HighColor();
	text->TextView()->SetFontAndColor (&font, B_FONT_ALL, &hi);
	text->TextView()->MakeResizable (true, text);
	text->ResizeToPreferred();
	if (mode == M_DRAW)
	{
//		text->TextView()->SetHighColor (HighColor());
//		text->TextView()->SetLowColor (LowColor());
	}
	else
	{
		text->TextView()->SetHighColor (Black);
		text->TextView()->SetLowColor (White);
	}
	float dx = -text->TextView()->TextRect().left - 3;
	float dy = text->TextView()->Bounds().top - text->TextView()->TextRect().bottom + 4;
	text->MoveBy (dx, dy);
	text->TextView()->SetTextRect (text->Bounds());
	AddChild (text);
	text->MakeFocus();
#else

	// New 2.0 text tool
	extern AttribText *tAttribText;
	extern ColorMenuButton *locolor, *hicolor;
	SetupUndo (mode);

//	drawing_mode dm = mAttribDraw->getDrawingMode();
	BFont font = tAttribText->getFont();
	const char *text = tAttribText->getText();
	font.SetSpacing (B_BITMAP_SPACING);

	// all of this is for multiline texts
	font_height fh;
	font.GetHeight (&fh);
	float height = fh.ascent + fh.descent + fh.leading;
	float rot = font.Rotation()*M_PI/180.0;
	float fx = height*sin(rot);
	float fy = height*cos(rot);
	
	int len = strlen (text);
	char *stext = strdup (text);	// we poke zeroes in this string
	int i = 0, j = 0;
	BPoint nextpoint = point;

	if (mode == M_DRAW)
	{
		if (buttons & B_PRIMARY_MOUSE_BUTTON)
		{
			drawView->SetHighColor (hicolor->color());
			drawView->SetLowColor (locolor->color());
		}
		if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
		{
			drawView->SetHighColor (locolor->color());
			drawView->SetLowColor (hicolor->color());
		}
		drawView->SetDrawingMode (B_OP_OVER);
		drawView->SetFont (&font);

		while (i <= len)
		{
			if (stext[i] == '\n' || !stext[i])
			{
				stext[i] = 0;
				drawView->DrawString (stext + j, nextpoint);
				j = i + 1;
				nextpoint.x += fx;
				nextpoint.y += fy;
			}
			i++;
		}
	}
	else
	{
		// Yuk this is silly - but I'm afraid it's necessary.  Sigh.
		// The reason is that selection maps are 8 bit, but you can't
		// set the HighColor by index directly.
		rgb_color c = hicolor->color();
		int value = 255 - int (0.5*c.green + 0.3*c.red + 0.2*c.blue + 0.5);
		BBitmap *tempsel = new BBitmap (fCanvasFrame, B_RGBA32, true);
		bzero (tempsel->Bits(), tempsel->BitsLength());
		BView *tempselView = new BView (fCanvasFrame, "Temp Selection View", uint32 (NULL), uint32 (NULL));
		tempsel->Lock();
		tempsel->AddChild (tempselView);
		tempselView->SetHighColor (value, value, value);
		tempselView->SetLowColor (Black);
		tempselView->SetFont (&font);
		tempselView->SetDrawingMode (B_OP_OVER);
		while (i <= len)
		{
			if (stext[i] == '\n' || !stext[i])
			{
				stext[i] = 0;
				tempselView->DrawString (stext + j, nextpoint);
				j = i + 1;
				nextpoint.x += fx;
				nextpoint.y += fy;
			}
			i++;
		}
		tempselView->Sync();
		uint32 *tdata = ((uint32 *) tempsel->Bits()) - 1;
		uchar *sdata = (uchar *) selection->Bits() - 1;
		ulong h = fCanvasFrame.IntegerHeight() + 1;
		ulong w = fCanvasFrame.IntegerWidth() + 1;
		int32 diff = selection->BytesPerRow() - w;
		for (ulong i = 0; i < h; i++)
		{
			for (ulong j = 0; j < w; j++)
				*sdata = clipchar (int (*(++sdata) + ((*(++tdata) >> 8) & 0xFF)));
			sdata += diff;
		}
		tempsel->RemoveChild (tempselView);
		delete tempselView;
		delete tempsel;		
	}

	free (stext);
	drawView->Sync();
	selectionView->Sync();
	Invalidate();

#endif
}

void CanvasView::tTextM (int32 mode, BPoint point)
{
	extern AttribText *tAttribText;
	
	extern ColorMenuButton *locolor, *hicolor;
	
	if (mode == M_DRAW)
	{
		SetHighColor (hicolor->color());
		SetLowColor (locolor->color());
	}
	else
	{
		SetHighColor (Black);
		SetLowColor (White);
	}
	
	BFont font = tAttribText->getFont();
	const char *text = tAttribText->getText();
	
	font.SetSpacing (B_BITMAP_SPACING);
	
	if (mode == M_DRAW)
		SetDrawingMode (B_OP_OVER);
	else
		SetDrawingMode (B_OP_INVERT);
	
	SetFont (&font);

	// all of this is for multiline texts
	font_height fh;
	font.GetHeight (&fh);
	float height = fh.ascent + fh.descent + fh.leading;
	float rot = font.Rotation()*M_PI/180.0;
	float fx = height*sin(rot);
	float fy = height*cos(rot);
	
	int len = strlen (text);
	char *stext = strdup (text);	// we poke zeroes in this string
	int i = 0, j = 0;
	BPoint nextpoint = point;
	Invalidate();
	while (i <= len)
	{
		if (stext[i] == '\n' || !stext[i])
		{
			stext[i] = 0;
			DrawString (stext + j, nextpoint);
			j = i + 1;
			nextpoint.x += fx;
			nextpoint.y += fy;
		}
		i++;
	}
	free (stext);
}

void CanvasView::tTextD ()
{
#if defined (USE_OLD_TEXT_TOOL)

	// Note:  This function gets called by CanvasWindow - hence all the locking.
	myWindow->Lock();
	currentLayer()->Lock();
	selection->Lock();
	extern ColorMenuButton *locolor, *hicolor;
	extern PicMenuButton *mode;
	extern AttribText *tAttribText;
	extern AttribDraw *mAttribDraw;
	myWindow->Lock();
	int32 currentMode = mode->selected();
	myWindow->Unlock();
	BFont font = tAttribText->getFont();
	font.SetSpacing (B_BITMAP_SPACING);
	drawing_mode dm = mAttribDraw->getDrawingMode();
	SetDrawingMode (dm);
	drawView->SetDrawingMode (dm);
	drawView->SetFont (&font);
	if (currentMode == M_DRAW)
	{
		if (fButtons & B_PRIMARY_MOUSE_BUTTON)
		{
			drawView->SetHighColor (hicolor->color());
			drawView->SetLowColor (locolor->color());
		}
		if (fButtons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
		{
			drawView->SetHighColor (locolor->color());
			drawView->SetLowColor (hicolor->color());
		}
	}
	BPoint place = ConvertFromScreen (text->TextView()->ConvertToScreen ((text->TextView()->TextRect().LeftBottom())));
	place.x = (place.x + 7)/fScale;
	place.y = (place.y - 7)/fScale;
	// Oh, the joy of not having access to the anti-aliasing alpha data...
	if (currentMode == M_DRAW)
	{
		if (dm == B_OP_COPY || dm == B_OP_BLEND)
		{
			BBitmap *tempsel = new BBitmap (fCanvasFrame, B_RGBA32, true);
			bzero (tempsel->Bits(), tempsel->BitsLength());
			BView *tempselView = new BView (fCanvasFrame, "Temp Text View", uint32 (NULL), uint32 (NULL));
			tempsel->Lock();
			tempsel->AddChild (tempselView);
			tempselView->SetHighColor (White);
			tempselView->SetLowColor (Black);
			tempselView->SetFont (&font);
			tempselView->SetDrawingMode (B_OP_OVER);
			tempselView->DrawString (text->Text(), place);
			tempselView->Sync();
			uint32 *tdata = ((uint32 *) tempsel->Bits()) - 1;
			ulong h = fCanvasFrame.IntegerHeight() + 1;
			ulong w = fCanvasFrame.IntegerWidth() + 1;
			rgb_color c = drawView->HighColor();
#if defined (__POWERPC__)
			uint32 pixel = (c.blue << 24) + (c.green << 16) + (c.red << 8);
			for (int i = 0; i < h; i++)
				for (int j = 0; j < w; j++)
					*(tdata) = pixel | ((*(++tdata) >> 8) & 0xFF);
#else
			uint32 pixel = (c.blue) + (c.green << 8) + (c.red << 16);
			for (ulong i = 0; i < h; i++)
				for (ulong j = 0; j < w; j++)
					*(tdata) = pixel | ((*(++tdata) << 8) & 0xFF000000);
#endif
			if (dm == B_OP_COPY)
				AddWithAlpha (tempsel, currentLayer(), 0, 0);
			else
				BlendWithAlpha (tempsel, currentLayer(), 0, 0);
			tempsel->RemoveChild (tempselView);
			delete tempselView;
			delete tempsel;	
		}
		else
		{
			drawView->DrawString (text->Text(), place);
		}
	}
	else
	{
		BBitmap *tempsel = new BBitmap (fCanvasFrame, B_RGBA32, true);
		bzero (tempsel->Bits(), tempsel->BitsLength());
		BView *tempselView = new BView (fCanvasFrame, "Temp Selection View", uint32 (NULL), uint32 (NULL));
		tempsel->Lock();
		tempsel->AddChild (tempselView);
		tempselView->SetHighColor (White);
		tempselView->SetLowColor (Black);
		tempselView->SetFont (&font);
		tempselView->SetDrawingMode (B_OP_OVER);
		tempselView->DrawString (text->Text(), place);
		tempselView->Sync();
		uint32 *tdata = ((uint32 *) tempsel->Bits()) - 1;
		uchar *sdata = (uchar *) selection->Bits() - 1;
		ulong h = fCanvasFrame.IntegerHeight() + 1;
		ulong w = fCanvasFrame.IntegerWidth() + 1;
		int32 diff = selection->BytesPerRow() - w;
		for (ulong i = 0; i < h; i++)
		{
			for (ulong j = 0; j < w; j++)
				*sdata = clipchar (int (*(++sdata) + ((*(++tdata) >> 8) & 0xFF)));
			sdata += diff;
		}
		tempsel->RemoveChild (tempselView);
		delete tempselView;
		delete tempsel;		
	}
	Sync();
	drawView->Sync();
	selectionView->Sync();
	RemoveChild (text);
	delete text;
	text = NULL;
	selection->Unlock();
	currentLayer()->Unlock();
	myWindow->Unlock();
	Invalidate();
	Window()->UpdateIfNeeded();
#endif
}

void CanvasView::tSpraycan (int32 mode, BPoint point, uint32 buttons)
{
	extern ColorMenuButton *locolor, *hicolor;
	extern AttribSpraycan *tAttribSpraycan;
//	Position position;
	int strength = 0;
	SetupUndo (mode);
	float sigma = tAttribSpraycan->getSigma();
	float ssq = sigma*sigma;
	float color_ratio = tAttribSpraycan->getColorRatio();
	float flowrate = tAttribSpraycan->getFlowRate();
	bool lighten = tAttribSpraycan->getLighten();
	extern AttribDraw *mAttribDraw;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	drawView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	SetPenSize (0);
	drawView->SetPenSize (0);
	selectionView->SetPenSize (0);
	MovePenTo (point);
	drawView->MovePenTo (point);
	if (mode == M_DRAW)
		StrokeLine (point);		// This fixes a _very_ strange bug...  After drawing with another
	//	tool in a large pen size, it seemed no drawing took place to the canvas BView... (R3)
	
	rgb_color hi, lo, hit = Black, lot = Black;
	if (buttons & B_PRIMARY_MOUSE_BUTTON)
	{
		BScreen screen;
		hi = hicolor->color ();
		lo = locolor->color ();
//		selectionView->SetLowColor (SELECT_NONE);
//		selectionView->SetHighColor (SELECT_FULL);
		selectionView->SetLowColor (screen.ColorMap()->color_list[lo.alpha]);
		selectionView->SetHighColor (screen.ColorMap()->color_list[hi.alpha]);
	}
	if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
	{
		BScreen screen;
		hi = locolor->color ();
		lo = hicolor->color ();
		selectionView->SetLowColor (screen.ColorMap()->color_list[hi.alpha]);
		selectionView->SetHighColor (screen.ColorMap()->color_list[lo.alpha]);
	}
	if (mode == M_SELECT)
		drawView->SetDrawingMode (B_OP_INVERT);
	else
	{
		// Setup parameters for fplot[_alpha]
		sbptr = (uint32 *) currentLayer()->Bits();
		sblpr = currentLayer()->BytesPerRow()/4;
	}
	const color_map *cmap;
	{
		BScreen screen;
		cmap = screen.ColorMap();
	}
	
	fSC.cmap = cmap;
	fSC.flowrate = flowrate;
	fSC.color_ratio = color_ratio;
	fSC.lighten = lighten;
	fSC.ssq = ssq;
	fSC.sigma = sigma;
	fSC.hi = hi; fSC.lo = lo;
	fSC.hit = hit; fSC.lot = lot;
	fSC.strength = strength;
	
#if !defined (USE_MOUSEMOVED)
	while (buttons)
	{
		for (int i = 0; i < (buttons & B_TERTIARY_MOUSE_BUTTON ? flowrate : flowrate*strength/255); i++)
		{
			float msigma = (buttons & B_TERTIARY_MOUSE_BUTTON ? sigma*strength/255 : sigma);
			float r = frand()*msigma;
			float phi = frand()*2*M_PI;
			float rsq = r*r;
			float gauss = exp (-rsq/ssq);
			if (frand() < gauss || lighten)
			{
				float x = r*cos(phi);
				float y = r*sin(phi);
				hit = hi;
				lot = lo;
				if (lighten)
				{
//					float factor = 1 - gauss;
//					hit.red   += (255 - hit.red)*factor;
//					hit.green += (255 - hit.green)*factor;
//					hit.blue  += (255 - hit.blue)*factor;
//					lot.red   += (255 - lot.red)*factor;
//					lot.green += (255 - lot.green)*factor;
//					lot.blue  += (255 - lot.blue)*factor;
					hit.alpha = uint8 (255*gauss);
					lot.alpha = uint8 (255*gauss);
				}
				BPoint p = BPoint (point.x + x, point.y + y);
				//SetHighColor (hit);
				//SetLowColor (lot);
				if (mode == M_SELECT)
				{
					selectionView->SetHighColor (cmap->color_list[hit.alpha]);
					selectionView->MovePenTo (p);
					selectionView->StrokeLine (p);
//					MovePenTo (p);
//					StrokeLine (p);
				}
				else if (frand() > color_ratio)
				{
					if (lighten)
						fplot_alpha (p, hit);
					else
						fplot (p, hit);
//					MovePenTo (p);
//					StrokeLine (p);
				}
				else
				{
					if (lighten)
						fplot_alpha (p, lot);
					else
						fplot (p, lot);
//					MovePenTo (p);
//					StrokeLine (p, B_SOLID_LOW);
				}
			}
		}
		Sync();
		drawView->Sync();
		selectionView->Sync();
		Invalidate (BRect (point.x - 2*sigma, point.y - 2*sigma, point.x + 2*sigma, point.y + 2*sigma));
		Window()->UpdateIfNeeded ();
		myWindow->Lock();
		myWindow->posview->Pulse();
		myWindow->Unlock();
		ScrollIfNeeded (point);
		snooze (20000);
//		GetMouse (&point, &buttons, true);
		GetPosition (&position);
		point = position.fPoint;
		buttons = position.fButtons;
		strength = abs (position.fPressure);
	}
	drawView->SetDrawingMode (dm);
	Invalidate ();
#endif
}

void CanvasView::tSpraycanM (int32 mode, BPoint point, uint32 buttons, int pressure, BPoint /*tilt*/)
{
	const color_map *cmap = fSC.cmap;
	float flowrate = fSC.flowrate;
	float color_ratio = fSC.color_ratio;
	bool lighten = fSC.lighten;
	float sigma = fSC.sigma;
	float ssq = fSC.ssq;
	rgb_color hi, lo, hit, lot;
	hi = fSC.hi; lo = fSC.lo;
	hit = fSC.hit; lot = fSC.lot;
	int strength = pressure; //fSC.strength;
	Position position;
	
	// make preference!!
	for (int i = 0; i < (buttons & B_TERTIARY_MOUSE_BUTTON ? flowrate : flowrate*strength/255); i++)
	{
		float msigma = (buttons & B_TERTIARY_MOUSE_BUTTON ? sigma*strength/255 : sigma);
		float r = frand()*msigma;
		float phi = frand()*2*M_PI;
		float rsq = r*r;
		float gauss = exp (-rsq/ssq);
		if (frand() < gauss || lighten)
		{
			float x = r*cos(phi);
			float y = r*sin(phi);
			hit = hi;
			lot = lo;
			if (lighten)
			{
//					float factor = 1 - gauss;
//					hit.red   += (255 - hit.red)*factor;
//					hit.green += (255 - hit.green)*factor;
//					hit.blue  += (255 - hit.blue)*factor;
//					lot.red   += (255 - lot.red)*factor;
//					lot.green += (255 - lot.green)*factor;
//					lot.blue  += (255 - lot.blue)*factor;
				hit.alpha = uint8 (255*gauss);
				lot.alpha = uint8 (255*gauss);
			}
			BPoint p = BPoint (point.x + x, point.y + y);
			//SetHighColor (hit);
			//SetLowColor (lot);
			if (mode == M_SELECT)
			{
				selectionView->SetHighColor (cmap->color_list[hit.alpha]);
				selectionView->MovePenTo (p);
				selectionView->StrokeLine (p);
//					MovePenTo (p);
//					StrokeLine (p);
			}
			else if (frand() > color_ratio)
			{
				if (lighten)
					fplot_alpha (p, hit);
				else
					fplot (p, hit);
//					MovePenTo (p);
//					StrokeLine (p);
			}
			else
			{
				if (lighten)
					fplot_alpha (p, lot);
				else
					fplot (p, lot);
//					MovePenTo (p);
//					StrokeLine (p, B_SOLID_LOW);
			}
		}
	}
	Sync();
	drawView->Sync();
	selectionView->Sync();
	Invalidate (BRect (point.x - 2*sigma, point.y - 2*sigma, point.x + 2*sigma, point.y + 2*sigma));
	Window()->UpdateIfNeeded ();
	myWindow->Lock();
	myWindow->posview->Pulse();
	myWindow->Unlock();
	ScrollIfNeeded (point);
//	snooze (20000);
//		GetMouse (&point, &buttons, true);
	GetPosition (&position);
	fSC.position = position;
	fSC.strength = abs (position.fPressure);
}

void CanvasView::tFreehand (int32 mode, BPoint point, uint32 buttons)
// Note: Support for singe-pixel editing is still not perfect!
{
	extern PatternMenuButton *pat;
	extern AttribFreehand *tAttribFreehand;
	Position position;
	SetupUndo (mode);
	float pensize = tAttribFreehand->getPenSize();
	extern AttribDraw *mAttribDraw;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	drawView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	point.x = int (point.x + 0.5);
	point.y = int (point.y + 0.5);
	drawView->MovePenTo (point);
	selectionView->MovePenTo (point);
	MovePenTo (point);
	float halfpensize = int (pensize/2 - 0.5);
	drawView->SetPenSize (pensize);
	selectionView->SetPenSize (pensize);
	drawView->SetLineMode (B_ROUND_CAP, B_ROUND_JOIN);
	selectionView->SetLineMode (B_ROUND_CAP, B_ROUND_JOIN);
	SetPenSize (pensize);
	pattern curpat = pat->pat();
	tSetColors (buttons);
//	if (mode == M_SELECT)
//		drawView->SetDrawingMode (B_OP_INVERT);
//	BRect frame = fCanvasFrame;
	BPoint prev = point;
	while (buttons)
	{
		BPoint p;
		if (mode == M_DRAW)
		{
			p = drawView->PenLocation();
			#if defined (FIX_STROKELINE_BUG)
			if (halfpensize > 0.5 && point == prev) 
				drawView->FillEllipse (point, halfpensize, halfpensize, curpat);
			else
			#endif
			drawView->StrokeLine (point, curpat);
		}
		else
		{
			p = selectionView->PenLocation();
			#if defined (FIX_STROKELINE_BUG)
			if (halfpensize > 0.5 && point == prev)
				selectionView->FillEllipse (point, halfpensize, halfpensize, curpat);
			else
			#endif
			selectionView->StrokeLine (point, curpat);
		}
		#if defined (FIX_STROKELINE_BUG)
		if (halfpensize > 0.5 && point == prev)
			FillEllipse (point, halfpensize, halfpensize, curpat);
		else
		#endif
		StrokeLine (point, curpat);
		Sync();
		drawView->Sync();
		selectionView->Sync();
		BRect r = PRect (p.x, p.y, point.x, point.y);
		r.InsetBy (-halfpensize - 1, -halfpensize - 1);
		Invalidate (r);
		snooze (20000);
		ScrollIfNeeded (point);
		myWindow->Lock();
		myWindow->posview->Pulse();
		myWindow->Unlock();
//		GetMouse (&point, &buttons, true);
		GetPosition (&position);
		prev = point;
		point = position.fPoint;
		buttons = position.fButtons;
	}
/*
#if defined (FIX_STROKELINE_BUG)
	if (prev == point && halfpensize > 0.5)	// Fixes R4 bug?
	{
		point.x += 0.1;
		point.y += 0.1;
		if (mode == M_DRAW)
		{
			drawView->FillEllipse (point, halfpensize, halfpensize, curpat);
		}
		else
		{
			selectionView->FillEllipse (point, halfpensize, halfpensize, curpat);
		}
	}
#endif
*/
	Invalidate();
}

void CanvasView::tLines (int32 mode, BPoint point, uint32 buttons)
{
	extern PatternMenuButton *pat;
	extern AttribLines *tAttribLines;
	extern AttribDraw *mAttribDraw;
	Position position;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	drawView->SetDrawingMode (dm);
	selectionView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	float pensize = tAttribLines->getPenSize();
	float halfpensize = int (pensize/2 - 0.5);
	bool fillcorners = tAttribLines->fillCorners();
	if (fillcorners)
	{
		drawView->SetLineMode (B_ROUND_CAP, B_ROUND_JOIN);	
		selectionView->SetLineMode (B_ROUND_CAP, B_ROUND_JOIN);
	}
	else
	{
		drawView->SetLineMode (B_BUTT_CAP, B_MITER_JOIN);
		selectionView->SetLineMode (B_BUTT_CAP, B_MITER_JOIN);
	}
	drawView->SetPenSize (pensize);
	selectionView->SetPenSize (pensize);
	SetPenSize (pensize);
	pattern curpat = pat->pat();
	tSetColors (buttons);
	if (!entry)
	{
		BPoint pos = point;
		BPoint prev = point;
		fLastCenter = point;
//		BRect frame = fCanvasFrame;
		SetupUndo (mode);
		while (buttons)
		{
			BRect frame, pframe, tframe;
			if (point != prev)
			{
				frame.Set (pos.x, pos.y, point.x, point.y);
				pframe.Set (pos.x, pos.y, prev.x, prev.y);
				frame = makePositive (frame);
				pframe = makePositive (pframe);
				tframe = frame | pframe;
				tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
				currentLayer()->Lock();
				selection->Lock();
				if (mode == M_DRAW)
				{
					//drawView->SetDrawingMode (B_OP_COPY);
					//drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
					//drawView->SetDrawingMode (dm);
					//drawView->Sync();
					SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
//					if (fillcorners)
//					{
//						drawView->FillEllipse (pos, halfpensize, halfpensize, curpat);
//						drawView->FillEllipse (point, halfpensize, halfpensize, curpat);
//					}
					drawView->StrokeLine (pos, point, curpat);
				}
				else
				{
//					selectionView->SetDrawingMode (B_OP_COPY);
//					selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
//					drawView->SetDrawingMode (dm);
//					selectionView->Sync();
					SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
//					if (fillcorners)
//					{
//						selectionView->FillEllipse (pos, halfpensize, halfpensize, curpat);
//						selectionView->FillEllipse (point, halfpensize, halfpensize, curpat);
//					}
					selectionView->StrokeLine (pos, point, curpat);
				}
				drawView->Sync();
				selectionView->Sync();
				selection->Unlock();
				currentLayer()->Unlock();
				Invalidate (tframe);
			}
			snooze (20000);
			ScrollIfNeeded (point);
			myWindow->Lock();
			myWindow->posview->Pulse();
			myWindow->Unlock();
			prev = point;
//			GetMouse (&point, &buttons, true);
			GetPosition (&position);
			point = position.fPoint;
			buttons = position.fButtons;
		}
		if (point == pos)
		{
			MovePenTo (pos);
			Sync();
			entry++;
		}
		else
		{
			if (mode == M_DRAW)
			{
				memcpy (currentLayer()->Bits(), undo[indexUndo].bitmap->Bits(), currentLayer()->BitsLength());
//				if (fillcorners)
//				{
//					drawView->FillEllipse (pos, halfpensize, halfpensize, curpat);
//					drawView->FillEllipse (point, halfpensize, halfpensize, curpat);
//				}
				drawView->StrokeLine (pos, point, curpat);
			}
			else
			{
				memcpy (selection->Bits(), undo[indexUndo].sbitmap->Bits(), selection->BitsLength());
//				if (fillcorners)
//				{
//					selectionView->FillEllipse (pos, halfpensize, halfpensize, curpat);
//					selectionView->FillEllipse (point, halfpensize, halfpensize, curpat);
//				}
				selectionView->StrokeLine (pos, point, curpat);
			}
		}
	}
	else if (!(buttons & B_TERTIARY_MOUSE_BUTTON))
	{
		if (mode == M_DRAW)
		{
			memcpy (currentLayer()->Bits(), undo[indexUndo].bitmap->Bits(), currentLayer()->BitsLength());
//			if (fillcorners)
//			{
//				drawView->FillEllipse (PenLocation(), halfpensize, halfpensize, curpat);
//				drawView->FillEllipse (point, halfpensize, halfpensize, curpat);
//			}
			drawView->StrokeLine (PenLocation(), point, curpat);
		}
		else
		{
			memcpy (selection->Bits(), undo[indexUndo].sbitmap->Bits(), selection->BitsLength());
//			if (fillcorners)
//			{
//				selectionView->FillEllipse (PenLocation(), halfpensize, halfpensize, curpat);
//				selectionView->FillEllipse (point, halfpensize, halfpensize, curpat);
//			}
			selectionView->StrokeLine (PenLocation(), point, curpat);
		}
		entry++;
		MovePenTo (point);
		drawView->Sync();
		selectionView->Sync();
		SetupUndo (mode);
	}
	else
	{
		entry = 0;
		myWindow->posview->SetPoint (-1, -1);
	}
	selectionView->Sync();
	drawView->Sync();
	Sync();
	Invalidate();
	Window()->UpdateIfNeeded();
}

void CanvasView::tLinesM (int32 mode, BPoint point)
{
	if (entry && mouse_on_canvas)
	{
		extern AttribLines *tAttribLines;
		extern PatternMenuButton *pat;
		pattern curpat = pat->pat();
		float pensize = tAttribLines->getPenSize();
		float halfpensize = int (pensize/2 - 0.5);
		currentLayer()->Lock();
		selection->Lock();
		SetPenSize (pensize);
		drawView->SetPenSize (pensize);
		selectionView->SetPenSize (pensize);
		BPoint pos = PenLocation();
		BRect frame = PRect (pos.x, pos.y, point.x, point.y);
		BRect pframe = PRect (pos.x, pos.y, prev.x, prev.y);
		BRect tframe = frame | pframe;
		tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
		if (mode == M_DRAW)
		{
			if (tAttribLines->fillCorners() && entry == 1)
				drawView->SetLineMode (B_ROUND_CAP, B_ROUND_JOIN);
			else
				drawView->SetLineMode (B_BUTT_CAP, B_MITER_JOIN);
//				drawView->FillEllipse (pos, halfpensize, halfpensize, curpat);
			drawView->StrokeLine (pos, point, curpat);
		}
		else
		{
			if (tAttribLines->fillCorners() && entry == 1)
				selectionView->SetLineMode (B_ROUND_CAP, B_ROUND_JOIN);
			else
				selectionView->SetLineMode (B_BUTT_CAP, B_MITER_JOIN);
//				selectionView->FillEllipse (pos, halfpensize, halfpensize, curpat);
			selectionView->StrokeLine (pos, point, curpat);
		}
//		drawView->MovePenTo (pos);
//		selectionView->MovePenTo (pos);
		drawView->Sync();
		selectionView->Sync();
		Invalidate (tframe);
		if (mode == M_DRAW)
			// drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
			SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
		else
			// selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
			SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
		currentLayer()->Unlock();
		selection->Unlock();
		prev = point;
	}
}

void CanvasView::tPolyblob (int32 mode, BPoint point, uint32 buttons)
{
	extern PatternMenuButton *pat;
	extern AttribPolyblob *tAttribPolyblob;
	float pensize = tAttribPolyblob->getPenSize();
	extern AttribDraw *mAttribDraw;
	Position position;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	drawView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	drawView->MovePenTo (point);
//	MovePenTo (BPoint (point.x*fScale, point.y*fScale));
	MovePenTo (BPoint (point.x, point.y));
//	float halfpensize = int (pensize/2 - 0.5);
	drawView->SetPenSize (pensize);
	selectionView->SetPenSize (pensize);
	SetPenSize (pensize);
	pattern curpat = pat->pat();
	tSetColors (buttons);
	SetupUndo (mode);
	delete polygon;		// Pity there isn't a BPolygon::Reset or ::Clear or whatever...
	polygon = new BPolygon ();
	if (mode == M_SELECT)
		SetDrawingMode (B_OP_INVERT);
//	BRect frame = fCanvasFrame;
	while (buttons)
	{
		polygon->AddPoints (&point, 1);
//		StrokeLine (BPoint (point.x*fScale, point.y*fScale));
		StrokeLine (BPoint (point.x, point.y));
//		Sync();		// This doesn't help.  You will see dots when scrolling.  Strange.
// Another strange thing, by the way, happens when the lot gets invalidated when there
// was some scrolling.  This doesn't redraw correctly, apparently.
		snooze (20000);
		ScrollIfNeeded (point);
		myWindow->Lock();
		myWindow->posview->Pulse();
		myWindow->Unlock();
//		GetMouse (&point, &buttons, true);
		GetPosition (&position);
		point = position.fPoint;
		buttons = position.fButtons;
	}
	switch (tAttribPolyblob->getType())
	{
	case POLYBLOB_OUTLINE:
		if (mode == M_DRAW)
			drawView->StrokePolygon (polygon, true, curpat);
		else
			selectionView->StrokePolygon (polygon, true, curpat);
		break;
	case POLYBLOB_FILL:
		if (mode == M_DRAW)
			drawView->FillPolygon (polygon, curpat);
		else
			selectionView->FillPolygon (polygon, curpat);
		break;
	case POLYBLOB_OUTFILL:
		if (mode == M_DRAW)
		{
			drawView->FillPolygon (polygon, curpat);
			drawView->StrokePolygon (polygon);
		}
		else
		{
			selectionView->FillPolygon (polygon, curpat);
			selectionView->StrokePolygon (polygon);
		}
		break;
	default:
		fprintf (stderr, "Unknown Free Shape type...\n");
	}
	selectionView->Sync();
	drawView->Sync();
	Sync();
	Invalidate();
}

void CanvasView::tPolygon (int32 mode, BPoint point, uint32 buttons)
// Add points with one mouse button, close with the other.
// Yes, I know this isn't consistent with the rest.  Got any better ideas?
// Note: Added 13 Oct 98: Closing with tertiary mouse button.
{
	extern PatternMenuButton *pat;
	extern AttribPolygon *tAttribPolygon;
	extern AttribDraw *mAttribDraw;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	drawView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	float pensize = tAttribPolygon->getPenSize();
	drawView->MovePenTo (point);
	MovePenTo (BPoint (point.x*fScale, point.y*fScale));
//	float halfpensize = int (pensize/2 - 0.5);
	drawView->SetPenSize (pensize);
	selectionView->SetPenSize (pensize);
	SetPenSize (pensize);
	pattern curpat = pat->pat();
	if (leftfirst)
		tSetColors (B_PRIMARY_MOUSE_BUTTON);
	else
		tSetColors (B_SECONDARY_MOUSE_BUTTON);
	if (!entry)
	{
		if (buttons & B_PRIMARY_MOUSE_BUTTON)
			leftfirst = true;
		if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
			leftfirst = false;
	}
	if (mode == M_SELECT)
		SetDrawingMode (B_OP_INVERT);
	if (!entry)
	{
		delete polygon;		// There should be a more elegant way of clearing an old BPolygon.
		polygon = new BPolygon (&point, 1);
		polypoint = point;	// This is a hack.  Apparently, a 2-point BPolygon can't be 
							// drawn, so this is for tPolygonM().
		entry = 1;
	}
	else
	{
		if ((leftfirst && (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY))
		|| (!leftfirst && (buttons & B_PRIMARY_MOUSE_BUTTON))
		|| (buttons & B_TERTIARY_MOUSE_BUTTON))		// Polygon closed
		{
			SetupUndo (mode);
			switch (tAttribPolygon->getType())
			{
			case POLYGON_OUTLINE:
				if (mode == M_DRAW)
					drawView->StrokePolygon (polygon, true, curpat);
				else
					selectionView->StrokePolygon (polygon, true, curpat);
				break;
			case POLYGON_FILL:
				if (mode == M_DRAW)
					drawView->FillPolygon (polygon, curpat);
				else
					selectionView->StrokePolygon (polygon, true, curpat);
				break;
			case POLYGON_OUTFILL:
				if (mode == M_DRAW)
				{
					drawView->FillPolygon (polygon, curpat);
					drawView->StrokePolygon (polygon);
				}
				else
				{
					selectionView->FillPolygon (polygon, curpat);
					selectionView->StrokePolygon (polygon);
				}
				break;
			default:
				fprintf (stderr, "Unknown Polygon type...\n");
			}
			Sync();
			selectionView->Sync();
			drawView->Sync();
			Invalidate();
			entry = 0;
			myWindow->posview->SetPoint (-1, -1);
		}
		else
		{
			polygon->AddPoints (&point, 1);
			Sync();
			myWindow->Lock();
			Draw (fCanvasFrame);
			myWindow->Unlock();
//			SetScale (fScale);
			StrokePolygon (polygon, false, curpat);
			if (entry == 1)
				StrokeLine (polypoint, point, curpat);
			snooze (20000);
			Sync();
//			SetScale (1);
			myWindow->Lock();
			myWindow->posview->Pulse();
			myWindow->Unlock();
			entry++;
		}
	}
}

void CanvasView::tPolygonM (int32 mode, BPoint point)
{
	if (entry && mouse_on_canvas)
	{
		extern AttribPolygon *tAttribPolygon;
		extern AttribDraw *mAttribDraw;
		drawing_mode dm = mAttribDraw->getDrawingMode();
		drawView->SetDrawingMode (dm);
		float pensize = tAttribPolygon->getPenSize();
//		float halfpensize = int (pensize/2 - 0.5);
		SetPenSize (pensize);
		myWindow->Lock();
		Draw (fCanvasFrame);
		myWindow->Unlock();
//		SetScale (fScale);
		BPoint pos = PenLocation();
		if (mode == M_SELECT)
			SetDrawingMode (B_OP_INVERT);
		if (entry == 2)		// Apparently, a 2-point polygon can't be drawn...?
		{
			StrokeLine (polypoint, pos);
			StrokeLine (point);
		}
		else
		{
			StrokePolygon (polygon, false);
			StrokeLine (BPoint (pos.x/fScale, pos.y/fScale), point);
		}
		MovePenTo (pos);
//		SetScale (1);
	}
}

void CanvasView::tRect (int32 mode, BPoint point, uint32 buttons)
{
	extern PatternMenuButton *pat;
	extern AttribRect *tAttribRect;
	extern AttribDraw *mAttribDraw;
	Position position;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	currentLayer()->Lock();
	selection->Lock();
	selectionView->SetDrawingMode (dm);
	drawView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	float pensize = tAttribRect->getPenSize();
	float halfpensize = int (pensize/2 - 0.5);
	drawView->SetPenSize (pensize);
	selectionView->SetPenSize (pensize);
	SetPenSize (pensize);
	pattern curpat = pat->pat();
	tSetColors (buttons);
	rgb_color c = HighColor();
	if (!entry)
	{
		BPoint pos = point;
		BPoint prev = point;
		fLastCenter = point;
//		BRect frame = fCanvasFrame;
		SetupUndo (mode);
		while (buttons)
		{
			BRect frame, pframe, tframe;
			if (point != prev)
			{
				frame.Set (pos.x, pos.y, point.x, point.y);
				pframe.Set (pos.x, pos.y, prev.x, prev.y);
				frame = makePositive (frame);
				pframe = makePositive (pframe);
				tframe = frame | pframe;
				tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
				if (mode == M_DRAW)
				{
					//drawView->SetDrawingMode (B_OP_COPY);
					//drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
					//drawView->SetDrawingMode (dm);
					SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
					switch (tAttribRect->getType())
					{
					case RECT_OUTLINE:
						drawView->StrokeRect (BRect (pos, point), curpat);
						break;
					case RECT_FILL:
						drawView->FillRect (BRect (pos, point), curpat);
						break;
					case RECT_OUTFILL:
						drawView->FillRect (BRect (pos, point), curpat);
						drawView->StrokeRect (BRect (pos, point));
						break;
					default:
						fprintf (stderr, "Unknown Rect type...\n");
					}
				}
				else
				{
					//selectionView->SetDrawingMode (B_OP_COPY);
					//selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
					//selectionView->SetDrawingMode (dm);
					SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
					switch (tAttribRect->getType())
					{
					case RECT_OUTLINE:
						selectionView->StrokeRect (BRect (pos, point), curpat);
						break;
					case RECT_FILL:
						selectionView->FillRect (BRect (pos, point), curpat);
						break;
					case RECT_OUTFILL:
						selectionView->FillRect (BRect (pos, point), curpat);
						selectionView->StrokeRect (BRect (pos, point));
						break;
					default:
						fprintf (stderr, "Unknown Rect type...\n");
					}
				}
				drawView->Sync();
				selectionView->Sync();
				Invalidate (tframe);
			}
			snooze (20000);
			ScrollIfNeeded (point);
			myWindow->Lock();
			myWindow->posview->Pulse();
			myWindow->Unlock();
			prev = point;
//			GetMouse (&point, &buttons, true);
			GetPosition (&position);
			point = position.fPoint;
			buttons = position.fButtons;
		}
		if (point == pos)
		{
			MovePenTo (pos);
			Sync();
			entry = 1;
		}
		else
		{
			if (mode == M_DRAW)
			{
				memcpy (currentLayer()->Bits(), undo[indexUndo].bitmap->Bits(), currentLayer()->BitsLength());
				switch (tAttribRect->getType())
				{
				case RECT_OUTLINE:
					drawView->StrokeRect (BRect (pos, point), curpat);
					break;
				case RECT_FILL:
					drawView->FillRect (BRect (pos, point), curpat);
					break;
				case RECT_OUTFILL:
					drawView->FillRect (BRect (pos, point), curpat);
					drawView->StrokeRect (BRect (pos, point));
					break;
				default:
					fprintf (stderr, "Unknown Rect type...\n");
				}
				drawView->Sync();
			}
			else
			{
				memcpy (selection->Bits(), undo[indexUndo].sbitmap->Bits(), selection->BitsLength());
				switch (tAttribRect->getType())
				{
				case RECT_OUTLINE:
					selectionView->StrokeRect (BRect (pos, point), curpat);
					break;
				case RECT_FILL:
					selectionView->FillRect (BRect (pos, point), curpat);
					break;
				case RECT_OUTFILL:
					selectionView->FillRect (BRect (pos, point), curpat);
					selectionView->StrokeRect (BRect (pos, point));
					break;
				default:
					fprintf (stderr, "Unknown Rect type...\n");
				}
				selectionView->Sync();
			}
			entry = 0;
			myWindow->posview->SetPoint (-1, -1);
		}
	}
	else
	{
		if (mode == M_DRAW)
		{
			switch (tAttribRect->getType())
			{
			case RECT_OUTLINE:
				drawView->StrokeRect (BRect (PenLocation(), point), curpat);
				break;
			case RECT_FILL:
				drawView->FillRect (BRect (PenLocation(), point), curpat);
				break;
			case RECT_OUTFILL:
				drawView->FillRect (BRect (PenLocation(), point), curpat);
				drawView->StrokeRect (BRect (PenLocation(), point));
				break;
			default:
				fprintf (stderr, "Unknown Rect type...\n");
			}
			drawView->Sync();
		}
		else
		{
			switch (tAttribRect->getType())
			{
			case RECT_OUTLINE:
				selectionView->StrokeRect (BRect (PenLocation(), point), curpat);
				break;
			case RECT_FILL:
				selectionView->FillRect (BRect (PenLocation(), point), curpat);
				break;
			case RECT_OUTFILL:
				selectionView->FillRect (BRect (PenLocation(), point), curpat);
				selectionView->StrokeRect (BRect (PenLocation(), point));
				break;
			default:
				fprintf (stderr, "Unknown Rect type...\n");
			}
			selectionView->Sync();
		}
		entry = 0;
		myWindow->posview->SetPoint (-1, -1);
	}
	Invalidate();
	selection->Unlock();
	currentLayer()->Unlock();
}

void CanvasView::tRectM (int32 mode, BPoint point)
{
	if (entry && mouse_on_canvas)
	{
		extern AttribRect *tAttribRect;
		extern PatternMenuButton *pat;
		extern AttribDraw *mAttribDraw;
		drawing_mode dm = mAttribDraw->getDrawingMode();
		currentLayer()->Lock();
		selection->Lock();
		drawView->SetDrawingMode (dm);
		selectionView->SetDrawingMode (dm);
		SetDrawingMode (dm);
//		tSetColors (B_PRIMARY_MOUSE_BUTTON);
//		selectionView->SetHighColor (SELECT_FULL);
//		selectionView->SetLowColor (SELECT_NONE);
		pattern curpat = pat->pat();
		float pensize = tAttribRect->getPenSize();
		float halfpensize = int (pensize/2 - 0.5);
		BPoint pos = PenLocation();
		BRect frame = PRect (pos.x, pos.y, point.x, point.y);
		BRect pframe = PRect (pos.x, pos.y, prev.x, prev.y);
		BRect tframe = frame | pframe;
		tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
		if (mode == M_DRAW)
		{
			switch (tAttribRect->getType())
			{
			case RECT_OUTLINE:
				drawView->StrokeRect (BRect (PenLocation(), point), curpat);
				break;
			case RECT_FILL:
				drawView->FillRect (BRect (PenLocation(), point), curpat);
				break;
			case RECT_OUTFILL:
				drawView->FillRect (BRect (PenLocation(), point), curpat);
				drawView->StrokeRect (BRect (PenLocation(), point));
				break;
			default:
				fprintf (stderr, "Unknown Rect type...\n");
			}
			drawView->MovePenTo (pos);
			drawView->Sync();
		}
		else
		{
			switch (tAttribRect->getType())
			{
			case RECT_OUTLINE:
				selectionView->StrokeRect (BRect (PenLocation(), point), curpat);
				break;
			case RECT_FILL:
				selectionView->FillRect (BRect (PenLocation(), point), curpat);
				break;
			case RECT_OUTFILL:
				selectionView->FillRect (BRect (PenLocation(), point), curpat);
				selectionView->StrokeRect (BRect (PenLocation(), point));
				break;
			default:
				fprintf (stderr, "Unknown Rect type...\n");
			}
			selectionView->MovePenTo (pos);
			selectionView->Sync();
		}
		Invalidate (tframe);
		drawView->SetDrawingMode (B_OP_COPY);
		selectionView->SetDrawingMode (B_OP_COPY);
		if (mode == M_DRAW)
			//drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
			SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
		else
			//selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
			SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
		selection->Unlock();
		currentLayer()->Unlock();
		prev = point;
	}
}

void CanvasView::tRoundRect (int32 mode, BPoint point, uint32 buttons)
{
	extern PatternMenuButton *pat;
	extern AttribRoundRect *tAttribRoundRect;
	extern AttribDraw *mAttribDraw;
	Position position;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	currentLayer()->Lock();
	selection->Lock();
	drawView->SetDrawingMode (dm);
	selectionView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	float pensize = tAttribRoundRect->getPenSize();
	float halfpensize = int (pensize/2 - 0.5);
	drawView->SetPenSize (pensize);
	selectionView->SetPenSize (pensize);
	SetPenSize (pensize);
	pattern curpat = pat->pat();
	tSetColors (buttons);
	float xRad;
	float yRad;
	if (!entry)
	{
		BPoint pos = point;
		BPoint prev = point;
		fLastCenter = point;
//		BRect frame = fCanvasFrame;
		SetupUndo (mode);
		while (buttons)
		{
			BRect frame, pframe, tframe;
			if (point != prev)
			{
				frame.Set (pos.x, pos.y, point.x, point.y);
				pframe.Set (pos.x, pos.y, prev.x, prev.y);
				frame = makePositive (frame);
				pframe = makePositive (pframe);
				tframe = frame | pframe;
				tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
				switch (tAttribRoundRect->getRadType())
				{
				case RRECT_RELATIVE:
					xRad = abs (int (pos.x - point.x))*tAttribRoundRect->getRadXrel();
					yRad = abs (int (pos.y - point.y))*tAttribRoundRect->getRadYrel();
					break;
				case RRECT_ABSOLUTE:
					xRad = tAttribRoundRect->getRadXabs();
					yRad = tAttribRoundRect->getRadYabs();
					break;
				default:
					fprintf (stderr, "RoundRect: Unknown radius type\n");
					xRad = 8;
					yRad = 8;
				}
				if (mode == M_DRAW)
				{
					//drawView->SetDrawingMode (B_OP_COPY);
					//drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
					//drawView->SetDrawingMode (dm);
					SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
					switch (tAttribRoundRect->getType())
					{
					case RRECT_OUTLINE:
						drawView->StrokeRoundRect (BRect (pos, point), xRad, yRad, curpat);
						break;
					case RRECT_FILL:
						drawView->FillRoundRect (BRect (pos, point), xRad, yRad, curpat);
						break;
					case RRECT_OUTFILL:
						drawView->FillRoundRect (BRect (pos, point), xRad, yRad, curpat);
						drawView->StrokeRoundRect (BRect (pos, point), xRad, yRad);
						break;
					default:
						fprintf (stderr, "Unknown RoundRect type...\n");
					}
					drawView->Sync();
				}
				else
				{
					//selectionView->SetDrawingMode (B_OP_COPY);
					//selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
					//selectionView->SetDrawingMode (dm);
					SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
					switch (tAttribRoundRect->getType())
					{
					case RRECT_OUTLINE:
						selectionView->StrokeRoundRect (BRect (pos, point), xRad, yRad, curpat);
						break;
					case RRECT_FILL:
						selectionView->FillRoundRect (BRect (pos, point), xRad, yRad, curpat);
						break;
					case RRECT_OUTFILL:
						selectionView->FillRoundRect (BRect (pos, point), xRad, yRad, curpat);
						selectionView->StrokeRoundRect (BRect (pos, point), xRad, yRad);
						break;
					default:
						fprintf (stderr, "Unknown RoundRect type...\n");
					}
					selectionView->Sync();
				}
				Invalidate (tframe);
			}
			snooze (20000);
			ScrollIfNeeded (point);
			myWindow->Lock();
			myWindow->posview->Pulse();
			myWindow->Unlock();
			prev = point;
//			GetMouse (&point, &buttons, true);
			GetPosition (&position);
			point = position.fPoint;
			buttons = position.fButtons;
		}
		if (point == pos)
		{
			MovePenTo (pos);
			Sync();
			entry = 1;		
		}
		else
		{
			switch (tAttribRoundRect->getRadType())
			{
			case RRECT_RELATIVE:
				xRad = abs (int (pos.x - point.x))*tAttribRoundRect->getRadXrel();
				yRad = abs (int (pos.y - point.y))*tAttribRoundRect->getRadYrel();
				break;
			case RRECT_ABSOLUTE:
				xRad = tAttribRoundRect->getRadXabs();
				yRad = tAttribRoundRect->getRadYabs();
				break;
			default:
				fprintf (stderr, "RoundRect: Unknown radius type\n");
				xRad = 8;
				yRad = 8;
			}
			if (mode == M_DRAW)
			{
				memcpy (currentLayer()->Bits(), undo[indexUndo].bitmap->Bits(), currentLayer()->BitsLength());
				switch (tAttribRoundRect->getType())
				{
				case RRECT_OUTLINE:
					drawView->StrokeRoundRect (BRect (pos, point), xRad, yRad, curpat);
					break;
				case RRECT_FILL:
					drawView->FillRoundRect (BRect (pos, point), xRad, yRad, curpat);
					break;
				case RRECT_OUTFILL:
					drawView->FillRoundRect (BRect (pos, point), xRad, yRad, curpat);
					drawView->StrokeRoundRect (BRect (pos, point), xRad, yRad);
					break;
				default:
					fprintf (stderr, "Unknown RoundRect type...\n");
				}
				drawView->Sync();
			}
			else
			{
				memcpy (undo[indexUndo].sbitmap->Bits(), selection->Bits(), selection->BitsLength());
				switch (tAttribRoundRect->getType())
				{
				case RRECT_OUTLINE:
					selectionView->StrokeRoundRect (BRect (pos, point), xRad, yRad, curpat);
					break;
				case RRECT_FILL:
					selectionView->FillRoundRect (BRect (pos, point), xRad, yRad, curpat);
					break;
				case RRECT_OUTFILL:
					selectionView->FillRoundRect (BRect (pos, point), xRad, yRad, curpat);
					selectionView->StrokeRoundRect (BRect (pos, point), xRad, yRad);
					break;
				default:
					fprintf (stderr, "Unknown RoundRect type...\n");
				}
				selectionView->Sync();
			}
			entry = 0;
			myWindow->posview->SetPoint (-1, -1);
		}
	}
	else
	{
		BPoint pos = PenLocation();
		float xRad;
		float yRad;
		switch (tAttribRoundRect->getRadType())
		{
		case RRECT_RELATIVE:
			xRad = abs (int (pos.x - point.x))*tAttribRoundRect->getRadXrel();
			yRad = abs (int (pos.y - point.y))*tAttribRoundRect->getRadYrel();
			break;
		case RRECT_ABSOLUTE:
			xRad = tAttribRoundRect->getRadXabs();
			yRad = tAttribRoundRect->getRadYabs();
			break;
		default:
			fprintf (stderr, "RoundRect: Unknown radius type\n");
			xRad = 8;
			yRad = 8;
		}
		if (mode == M_DRAW)
		{
			switch (tAttribRoundRect->getType())
			{
			case RRECT_OUTLINE:
				drawView->StrokeRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				break;
			case RRECT_FILL:
				drawView->FillRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				break;
			case RRECT_OUTFILL:
				drawView->FillRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				drawView->StrokeRoundRect (BRect (PenLocation(), point), xRad, yRad);
				break;
			default:
				fprintf (stderr, "Unknown RoundRect type...\n");
			}
			drawView->Sync();
		}
		else
		{
			switch (tAttribRoundRect->getType())
			{
			case RRECT_OUTLINE:
				selectionView->StrokeRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				break;
			case RRECT_FILL:
				selectionView->FillRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				break;
			case RRECT_OUTFILL:
				selectionView->FillRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				selectionView->StrokeRoundRect (BRect (PenLocation(), point), xRad, yRad);
				break;
			default:
				fprintf (stderr, "Unknown RoundRect type...\n");
			}
			selectionView->Sync();
		}
		entry = 0;
		myWindow->posview->SetPoint (-1, -1);
	}
	Invalidate();
	selection->Unlock();
	currentLayer()->Unlock();
}

void CanvasView::tRoundRectM (int32 mode, BPoint point)
{
	if (entry && mouse_on_canvas)
	{
		extern AttribRoundRect *tAttribRoundRect;
		extern PatternMenuButton *pat;
		extern AttribDraw *mAttribDraw;
//		Position position;
		drawing_mode dm = mAttribDraw->getDrawingMode();
		currentLayer()->Lock();
		selection->Lock();
		drawView->SetDrawingMode (dm);
		selectionView->SetDrawingMode (dm);
		SetDrawingMode (dm);
		pattern curpat = pat->pat();
		float pensize = tAttribRoundRect->getPenSize();
		float halfpensize = int (pensize/2 - 0.5);
		SetPenSize (pensize);
		BPoint pos = PenLocation();
		BRect frame = PRect (pos.x, pos.y, point.x, point.y);
		BRect pframe = PRect (pos.x, pos.y, prev.x, prev.y);
		BRect tframe = frame | pframe;
		tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
		float xRad;
		float yRad;
		switch (tAttribRoundRect->getRadType())
		{
		case RRECT_RELATIVE:
			xRad = abs (int (pos.x - point.x))*tAttribRoundRect->getRadXrel();
			yRad = abs (int (pos.y - point.y))*tAttribRoundRect->getRadYrel();
			break;
		case RRECT_ABSOLUTE:
			xRad = tAttribRoundRect->getRadXabs();
			yRad = tAttribRoundRect->getRadYabs();
			break;
		default:
			fprintf (stderr, "RoundRect: Unknown radius type\n");
			xRad = 8;
			yRad = 8;
		}
		if (mode == M_DRAW)
		{
			switch (tAttribRoundRect->getType())
			{
			case RRECT_OUTLINE:
				drawView->StrokeRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				break;
			case RRECT_FILL:
				drawView->FillRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				break;
			case RRECT_OUTFILL:
				drawView->FillRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				drawView->StrokeRoundRect (BRect (PenLocation(), point), xRad, yRad);
				break;
			default:
				fprintf (stderr, "Unknown RoundRect type...\n");
			}
			drawView->MovePenTo (pos);
			drawView->Sync();
		}
		else
		{
			switch (tAttribRoundRect->getType())
			{
			case RRECT_OUTLINE:
				selectionView->StrokeRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				break;
			case RRECT_FILL:
				selectionView->FillRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				break;
			case RRECT_OUTFILL:
				selectionView->FillRoundRect (BRect (PenLocation(), point), xRad, yRad, curpat);
				selectionView->StrokeRoundRect (BRect (PenLocation(), point), xRad, yRad);
				break;
			default:
				fprintf (stderr, "Unknown RoundRect type...\n");
			}
			selectionView->MovePenTo (pos);
			selectionView->Sync();
		}
		Invalidate (tframe);
		drawView->SetDrawingMode (B_OP_COPY);
		selectionView->SetDrawingMode (B_OP_COPY);
		if (mode == M_DRAW)
			//drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
			SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
		else
			//selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
			SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
		selection->Unlock();
		currentLayer()->Unlock();
		prev = point;
	}
}

void CanvasView::tCircle (int32 mode, BPoint point, uint32 buttons)
{
	extern PatternMenuButton *pat;
	extern AttribCircle *tAttribCircle;
	extern AttribDraw *mAttribDraw;
	Position position;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	currentLayer()->Lock();
	selection->Lock();
	drawView->SetDrawingMode (dm);
	selectionView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	int cent = tAttribCircle->getFirst();
	float pensize = tAttribCircle->getPenSize();
	float halfpensize = int (pensize/2 - 0.5);
	drawView->SetPenSize (pensize);
	selectionView->SetPenSize (pensize);
	SetPenSize (pensize);
	pattern curpat = pat->pat();
	tSetColors (buttons);
	if (!entry)
	{
		BPoint prev = point;
		BPoint center = point;
		fLastCenter = point;
//		BRect frame = fCanvasFrame;
		float radius = 0;
		SetupUndo (mode);
		while (buttons)
		{
			BRect frame, pframe, tframe;
			if (point != prev)
			{
				radius = sqrt ((center.x - point.x)*(center.x - point.x) + (center.y - point.y)*(center.y - point.y));
				if (cent == FIXES_CENTER)
				{
					frame.Set (center.x - radius, center.y - radius, center.x + radius, center.y + radius);
					pframe.Set (center.x - pradius, center.y - pradius, center.x + pradius, center.y + pradius);
				}
				else
				{
					frame.Set (point.x - radius, point.y - radius, point.x + radius, point.y + radius);
					pframe.Set (prev.x - pradius, prev.y - pradius, prev.x + pradius, prev.y + pradius);
				}
				frame = makePositive (frame);
				pframe = makePositive (pframe);
				tframe = frame | pframe;
				tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
				if (mode == M_DRAW)
				{
					//drawView->SetDrawingMode (B_OP_COPY);
					//drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
					//drawView->SetDrawingMode (dm);
					SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
					switch (tAttribCircle->getType())
					{
					case CIRCLE_OUTLINE:
						drawView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
						break;
					case CIRCLE_FILL:
						drawView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
						break;
					case CIRCLE_OUTFILL:
						drawView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
						drawView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius);
						break;
					default:
						fprintf (stderr, "Unknown Circle type...\n");
					}
					drawView->Sync();
				}
				else
				{
					//selectionView->SetDrawingMode (B_OP_COPY);
					//selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
					//selectionView->SetDrawingMode (dm);
					SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
					switch (tAttribCircle->getType())
					{
					case CIRCLE_OUTLINE:
						selectionView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
						break;
					case CIRCLE_FILL:
						selectionView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
						break;
					case CIRCLE_OUTFILL:
						selectionView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
						selectionView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius);
						break;
					default:
						fprintf (stderr, "Unknown Circle type...\n");
					}
					selectionView->Sync();
				}
				Invalidate (tframe);
			}
			snooze (20000);
			ScrollIfNeeded (point);
			myWindow->Lock();
			myWindow->posview->Pulse();
			myWindow->Unlock();
			prev = point;
			pradius = radius;
//			GetMouse (&point, &buttons, true);
			GetPosition (&position);
			point = position.fPoint;
			buttons = position.fButtons;
		}
		if (point == center)
		{
			MovePenTo (center);
			Sync();
			entry = 1;
		}
		else
		{
			if (mode == M_DRAW)
			{
				memcpy (currentLayer()->Bits(), undo[indexUndo].bitmap->Bits(), currentLayer()->BitsLength());
				radius = sqrt ((center.x - point.x)*(center.x - point.x) + (center.y - point.y)*(center.y - point.y));
				switch (tAttribCircle->getType())
				{
				case CIRCLE_OUTLINE:
					drawView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
					break;
				case CIRCLE_FILL:
					drawView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
					break;
				case CIRCLE_OUTFILL:
					drawView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
					drawView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius);
					break;
				default:
					fprintf (stderr, "Unknown Circle type...\n");
				}
				drawView->Sync();
			}
			else
			{
				memcpy (selection->Bits(), undo[indexUndo].sbitmap->Bits(), selection->BitsLength());
				radius = sqrt ((center.x - point.x)*(center.x - point.x) + (center.y - point.y)*(center.y - point.y));
				switch (tAttribCircle->getType())
				{
				case CIRCLE_OUTLINE:
					selectionView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
					break;
				case CIRCLE_FILL:
					selectionView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
					break;
				case CIRCLE_OUTFILL:
					selectionView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
					selectionView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius);
					break;
				default:
					fprintf (stderr, "Unknown Circle type...\n");
				}
				selectionView->Sync();
			}
			entry = 0;
			myWindow->posview->SetPoint (-1, -1);
		}
	}
	else
	{
		BPoint center = PenLocation();
//		float radius = sqrt ((center.x - point.x)*(center.x - point.x) + (center.y - point.y)*(center.y - point.y));
		if (mode == M_DRAW)
		{
			memcpy (currentLayer()->Bits(), undo[indexUndo].bitmap->Bits(), currentLayer()->BitsLength());
			float radius = sqrt ((center.x - point.x)*(center.x - point.x) + (center.y - point.y)*(center.y - point.y));
			switch (tAttribCircle->getType())
			{
			case CIRCLE_OUTLINE:
				drawView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				break;
			case CIRCLE_FILL:
				drawView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				break;
			case CIRCLE_OUTFILL:
				drawView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				drawView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius);
				break;
			default:
				fprintf (stderr, "Unknown Circle type...\n");
			}
			drawView->Sync();
		}
		else
		{
			memcpy (selection->Bits(), undo[indexUndo].sbitmap->Bits(), selection->BitsLength());
			float radius = sqrt ((center.x - point.x)*(center.x - point.x) + (center.y - point.y)*(center.y - point.y));
			switch (tAttribCircle->getType())
			{
			case CIRCLE_OUTLINE:
				selectionView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				break;
			case CIRCLE_FILL:
				selectionView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				break;
			case CIRCLE_OUTFILL:
				selectionView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				selectionView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius);
				break;
			default:
				fprintf (stderr, "Unknown Circle type...\n");
			}
			selectionView->Sync();
		}
		entry = 0;
		myWindow->posview->SetPoint (-1, -1);
	}
	selection->Unlock();
	currentLayer()->Unlock();
	Invalidate();
}

void CanvasView::tCircleM (int32 mode, BPoint point)
{
	if (entry && mouse_on_canvas)
	{
		extern AttribCircle *tAttribCircle;
		extern PatternMenuButton *pat;
		extern AttribDraw *mAttribDraw;
		drawing_mode dm = mAttribDraw->getDrawingMode();
		currentLayer()->Lock();
		selection->Lock();
		drawView->SetDrawingMode (dm);
		selectionView->SetDrawingMode (dm);
		SetDrawingMode (dm);
		pattern curpat = pat->pat();
		float pensize = tAttribCircle->getPenSize();
		float halfpensize = int (pensize/2 - 0.5);
		int cent = tAttribCircle->getFirst();
		BPoint center = PenLocation();
		float radius = sqrt ((center.x - point.x)*(center.x - point.x) + (center.y - point.y)*(center.y - point.y));
		SetPenSize (pensize);
		BPoint pos = PenLocation();
		BRect frame, pframe;
		if (cent == FIXES_CENTER)
		{
			frame = PRect (center.x - radius, center.y - radius, center.x + radius, center.y + radius);
			pframe =  PRect (center.x - pradius, center.y - pradius, center.x + pradius, center.y + pradius);
		}
		else
		{
			frame = PRect (point.x - radius, point.y - radius, point.x + radius, point.y + radius);
			pframe =  PRect (prev.x - pradius, prev.y - pradius, prev.x + pradius, prev.y + pradius);
		}
		BRect tframe = frame | pframe;
		tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
		if (mode == M_DRAW)
		{
			switch (tAttribCircle->getType())
			{
			case CIRCLE_OUTLINE:
				drawView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				break;
			case CIRCLE_FILL:
				drawView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				break;
			case CIRCLE_OUTFILL:
				drawView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				drawView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius);
				break;
			default:
				fprintf (stderr, "Unknown Circle type...\n");
			}
			drawView->MovePenTo (pos);
			drawView->Sync();
		}
		else
		{
			switch (tAttribCircle->getType())
			{
			case CIRCLE_OUTLINE:
				selectionView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				break;
			case CIRCLE_FILL:
				selectionView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				break;
			case CIRCLE_OUTFILL:
				selectionView->FillEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius, curpat);
				selectionView->StrokeEllipse ((cent == FIXES_CENTER) ? center : point, radius, radius);
				break;
			default:
				fprintf (stderr, "Unknown Circle type...\n");
			}
			selectionView->MovePenTo (pos);
			selectionView->Sync();
		}
		Invalidate (tframe);
		drawView->SetDrawingMode (B_OP_COPY);
		selectionView->SetDrawingMode (B_OP_COPY);
		if (mode ==  M_DRAW)
			//drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
			SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
		else
			//selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
			SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
		selection->Unlock();
		currentLayer()->Unlock();
		prev = point;
		pradius = radius;
	}
}

void CanvasView::tEllipse (int32 mode, BPoint point, uint32 buttons)
{
	extern PatternMenuButton *pat;
	extern AttribEllipse *tAttribEllipse;
	extern AttribDraw *mAttribDraw;
	Position position;
	drawing_mode dm = mAttribDraw->getDrawingMode();
	currentLayer()->Lock();
	selection->Lock();
	selectionView->SetDrawingMode (dm);
	drawView->SetDrawingMode (dm);
	SetDrawingMode (dm);
	pattern curpat = pat->pat();
	float pensize = tAttribEllipse->getPenSize();
	float halfpensize = int (pensize/2 - 0.5);
	drawView->SetPenSize (pensize);
	selectionView->SetPenSize (pensize);
	SetPenSize (pensize);
	tSetColors (buttons);
	if (!entry)
	{
		BPoint pos = point;
		BPoint prev = point;
		fLastCenter = point;
//		BRect frame = fCanvasFrame;
		SetupUndo (mode);
		while (buttons)
		{
			BRect frame, pframe, tframe;
			if (point != prev)
			{
				frame.Set (pos.x, pos.y, point.x, point.y);
				pframe.Set (pos.x, pos.y, prev.x, prev.y);
				frame = makePositive (frame);
				pframe = makePositive (pframe);
				tframe = frame | pframe;
				tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
				if (mode == M_DRAW)
				{
					//drawView->SetDrawingMode (B_OP_COPY);
					//drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
					//drawView->SetDrawingMode (dm);
					SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
					switch (tAttribEllipse->getType())
					{
					case ELLIPSE_OUTLINE:
						drawView->StrokeEllipse (BRect (pos, point), curpat);
						break;
					case ELLIPSE_FILL:
						drawView->FillEllipse (BRect (pos, point), curpat);
						break;
					case ELLIPSE_OUTFILL:
						drawView->FillEllipse (BRect (pos, point), curpat);
						drawView->StrokeEllipse (BRect (pos, point));
						break;
					default:
						fprintf (stderr, "Unknown Ellipse type...\n");
					}
					drawView->Sync();
				}
				else
				{
					//selectionView->SetDrawingMode (B_OP_COPY);
					//selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
					//selectionView->SetDrawingMode (dm);
					SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
					switch (tAttribEllipse->getType())
					{
					case ELLIPSE_OUTLINE:
						selectionView->StrokeEllipse (BRect (pos, point), curpat);
						break;
					case ELLIPSE_FILL:
						selectionView->FillEllipse (BRect (pos, point), curpat);
						break;
					case ELLIPSE_OUTFILL:
						selectionView->FillEllipse (BRect (pos, point), curpat);
						selectionView->StrokeEllipse (BRect (pos, point));
						break;
					default:
						fprintf (stderr, "Unknown Ellipse type...\n");
					}
					selectionView->Sync();
				}
				Invalidate (tframe);
			}
			snooze (20000);
			ScrollIfNeeded (point);
			myWindow->Lock();
			myWindow->posview->Pulse();
			myWindow->Unlock();
			prev = point;
//			GetMouse (&point, &buttons, true);
			GetPosition (&position);
			point = position.fPoint;
			buttons = position.fButtons;
		}
		if (point == pos)
		{
			MovePenTo (pos);
			Sync();
			entry = 1;
		}
		else
		{
			if (mode == M_DRAW)
			{
				memcpy (currentLayer()->Bits(), undo[indexUndo].bitmap->Bits(), currentLayer()->BitsLength());
				switch (tAttribEllipse->getType())
				{
				case ELLIPSE_OUTLINE:
					drawView->StrokeEllipse (BRect (pos, point), curpat);
					break;
				case ELLIPSE_FILL:
					drawView->FillEllipse (BRect (pos, point), curpat);
					break;
				case ELLIPSE_OUTFILL:
					drawView->FillEllipse (BRect (pos, point), curpat);
					drawView->StrokeEllipse (BRect (pos, point));
					break;
				default:
					fprintf (stderr, "Unknown Ellipse type...\n");
				}
				drawView->Sync();
			}
			else
			{
				memcpy (selection->Bits(), undo[indexUndo].sbitmap->Bits(), selection->BitsLength());
				switch (tAttribEllipse->getType())
				{
				case ELLIPSE_OUTLINE:
					selectionView->StrokeEllipse (BRect (pos, point), curpat);
					break;
				case ELLIPSE_FILL:
					selectionView->FillEllipse (BRect (pos, point), curpat);
					break;
				case ELLIPSE_OUTFILL:
					selectionView->FillEllipse (BRect (pos, point), curpat);
					selectionView->StrokeEllipse (BRect (pos, point));
					break;
				default:
					fprintf (stderr, "Unknown Ellipse type...\n");
				}
				selectionView->Sync();
			}
			entry = 0;
			myWindow->posview->SetPoint (-1, -1);
		}
	}
	else
	{
		if (mode == M_DRAW)
		{
			switch (tAttribEllipse->getType())
			{
			case ELLIPSE_OUTLINE:
				drawView->StrokeEllipse (BRect (PenLocation(), point), curpat);
				break;
			case ELLIPSE_FILL:
				drawView->FillEllipse (BRect (PenLocation(), point), curpat);
				break;
			case ELLIPSE_OUTFILL:
				drawView->FillEllipse (BRect (PenLocation(), point), curpat);
				drawView->StrokeEllipse (BRect (PenLocation(), point));
				break;
			default:
				fprintf (stderr, "Unknown Ellipse type...\n");
			}
			drawView->Sync();
		}
		else
		{
			switch (tAttribEllipse->getType())
			{
			case ELLIPSE_OUTLINE:
				selectionView->StrokeEllipse (BRect (PenLocation(), point), curpat);
				break;
			case ELLIPSE_FILL:
				selectionView->FillEllipse (BRect (PenLocation(), point), curpat);
				break;
			case ELLIPSE_OUTFILL:
				selectionView->FillEllipse (BRect (PenLocation(), point), curpat);
				selectionView->StrokeEllipse (BRect (PenLocation(), point));
				break;
			default:
				fprintf (stderr, "Unknown Ellipse type...\n");
			}
			selectionView->Sync();
		}
		entry = 0;
		myWindow->posview->SetPoint (-1, -1);
	}
	selection->Unlock();
	currentLayer()->Unlock();
	Invalidate ();
}

void CanvasView::tEllipseM (int32 mode, BPoint point)
{
	if (entry && mouse_on_canvas)
	{
		extern AttribEllipse *tAttribEllipse;
		extern PatternMenuButton *pat;
		extern AttribDraw *mAttribDraw;
		drawing_mode dm = mAttribDraw->getDrawingMode();
		currentLayer()->Lock();
		selection->Lock();
		drawView->SetDrawingMode (dm);
		SetDrawingMode (dm);
		pattern curpat = pat->pat();
		float pensize = tAttribEllipse->getPenSize();
		float halfpensize = int (pensize/2 - 0.5);
		SetPenSize (pensize);
		BPoint pos = PenLocation();
		BRect frame = PRect (pos.x, pos.y, point.x, point.y);
		BRect pframe = PRect (pos.x, pos.y, prev.x, prev.y);
		BRect tframe = frame | pframe;
		tframe.InsetBy (-halfpensize - 1, -halfpensize - 1);
		if (mode == M_DRAW)
		{
			switch (tAttribEllipse->getType())
			{
			case ELLIPSE_OUTLINE:
				drawView->StrokeEllipse (BRect (PenLocation(), point), curpat);
				break;
			case ELLIPSE_FILL:
				drawView->FillEllipse (BRect (PenLocation(), point), curpat);
				break;
			case ELLIPSE_OUTFILL:
				drawView->FillEllipse (BRect (PenLocation(), point), curpat);
				drawView->StrokeEllipse (BRect (PenLocation(), point));
				break;
			default:
				fprintf (stderr, "Unknown Ellipse type...\n");
			}
			drawView->MovePenTo (pos);
			drawView->Sync();
		}
		else
		{
			switch (tAttribEllipse->getType())
			{
			case ELLIPSE_OUTLINE:
				selectionView->StrokeEllipse (BRect (PenLocation(), point), curpat);
				break;
			case ELLIPSE_FILL:
				selectionView->FillEllipse (BRect (PenLocation(), point), curpat);
				break;
			case ELLIPSE_OUTFILL:
				selectionView->FillEllipse (BRect (PenLocation(), point), curpat);
				selectionView->StrokeEllipse (BRect (PenLocation(), point));
				break;
			default:
				fprintf (stderr, "Unknown Ellipse type...\n");
			}
			selectionView->MovePenTo (pos);
			selectionView->Sync();
		}
		Invalidate (tframe);
		drawView->SetDrawingMode (B_OP_COPY);
		selectionView->SetDrawingMode (B_OP_COPY);
		if (mode == M_DRAW)
			//drawView->DrawSBitmap (undo[indexUndo].bitmap, tframe, tframe);
			SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), tframe);
		else
			//selectionView->DrawSBitmap (undo[indexUndo].sbitmap, tframe, tframe);
			SBitmapToSelection (undo[indexUndo].sbitmap, selection, tframe);
		selection->Unlock();
		currentLayer()->Unlock();
		prev = point;
	}
}

////////////////////////////
#if defined (__POWERPC__)
#	pragma optimization_level 4
#endif

inline bool CanvasView::inbounds (LPoint p)
{
	return (p.x >= 0 && p.x <= tw && p.y >= 0 && p.y <= th);
}

inline bool CanvasView::isfillcolor0 (LPoint point)
{
	ulong *addr = bbitsl + bbprl*point.y + point.x;
	return (*(uint32*)addr == fill32 || (!(fill32 & ALPHA_MASK) && !(*addr & ALPHA_MASK)));
}

inline bool CanvasView::isfillcolorrgb (LPoint point, uchar *t)
{
	rgb_color c = getColor (point);
	
	if (!fillcolor.alpha && !c.alpha)
		return true;
	
	if (c.alpha 
	  && abs (c.red   - fillcolor.red)   <= toleranceRGB.red
	  && abs (c.green - fillcolor.green) <= toleranceRGB.green
	  && abs (c.blue  - fillcolor.blue)  <= toleranceRGB.blue)
	{
		if (c.alpha == 255)		// Special case
			*t = clipchar (255 - diff (c, fillcolor));
		else
			*t = c.alpha;
		return true;
	}
	else
	{
		*t = 0;
		return false;
	}
}

inline bool CanvasView::isfillcolort (LPoint point, uchar *t)
{
	rgb_color c = getColor (point);
	register uchar d;
	if (!c.alpha)		// Special cases
	{
		if (!fillcolor.alpha)
		{
			*t = 255;
			return (true);
		}
		else
		{
			*t = 0;
			return (false);
		}
	}
	if (c.alpha == 255)
	{
		d = uchar (diff (c, fillcolor));
		*t = uchar (255 - d*254/tolerance);
	}
	else
	{
		d = c.alpha;
		*t = c.alpha;
	}
	return (d <= tolerance);
}

inline bool CanvasView::istransparent8 (LPoint point)
{
	uchar *addr = tbits + tbpr*point.y + point.x;
	return (*addr == 0);
}

void CanvasView::Fill (int32 mode, BPoint point, rgb_color *c)
{
	tFill (mode, point, B_PRIMARY_MOUSE_BUTTON, c);
}

void CanvasView::tFill (int32 mode, BPoint point, uint32 buttons, rgb_color *c)
{
	extern Becasso *mainapp;
	extern ColorMenuButton *locolor, *hicolor;
	extern PatternMenuButton *pat;
	extern AttribFill *tAttribFill;
	extern AttribDraw *mAttribDraw;
	SetupUndo (mode);
	mainapp->setBusy();
	currentLayer()->Lock();
	temp = new BBitmap (fCanvasFrame, B_BITMAP_ACCEPTS_VIEWS, B_COLOR_8_BIT);
	temp->Lock();
	SView *tempView = new SView (fCanvasFrame, "TempView for Fill", B_FOLLOW_NONE, uint32 (NULL));
	temp->AddChild (tempView);
	tbits = (uchar *) temp->Bits();
	tbpr = temp->BytesPerRow();
	tbprl = tbpr/4;
	bbitsl = (ulong *) currentLayer()->Bits();
	bbits = (uchar *) bbitsl;
	bbpr = currentLayer()->BytesPerRow();
	bbprl = bbpr/4;
	tw = fCanvasFrame.Width();
	th = fCanvasFrame.Height();
	bzero (temp->Bits(), temp->BitsLength());
	tolerance = tAttribFill->getTolerance();
	toleranceRGB = tAttribFill->getToleranceRGB();
	fillcolor = getColor (point);
	fill32 = PIXEL (fillcolor.red, fillcolor.green, fillcolor.blue, fillcolor.alpha);
	PointStack ps (point);
	tplot8 (point);
	tempView->Sync ();

	// printf ("Everything set up - entering loop\n");
	if (((tAttribFill->getTolMode() == FILLTOL_TOL) && (tolerance == 0))
		|| (tAttribFill->getTolMode() == FILLTOL_RGB) && (toleranceRGB.red == 0)
		&& (toleranceRGB.green == 0) && (toleranceRGB.blue == 0))
	{	// Use the special case functions for zero tolerance
		while (!(ps.isempty ()))
		{
			LPoint p = ps.pop ();
			LPoint next;
			next = LPoint (p.x + 1, p.y);
			if (inbounds (next) && istransparent8 (next) && isfillcolor0 (next))
			{
//				if (next.x > frame.right)
//					frame.right++;
				tplot8 (next);
				ps.push (next);
			}
			next = LPoint (p.x - 1, p.y);
			if (inbounds (next) && istransparent8 (next) && isfillcolor0 (next))
			{
//				if (next.x < frame.left)
//					frame.left--;
				tplot8 (next);
				ps.push (next);
			}
			next = LPoint (p.x, p.y + 1);
			if (inbounds (next) && istransparent8 (next) && isfillcolor0 (next))
			{
//				if (next.y > frame.bottom)
//					frame.bottom++;
				tplot8 (next);
				ps.push (next);
			}
			next = LPoint (p.x, p.y - 1);
			if (inbounds (next) && istransparent8 (next) && isfillcolor0 (next))
			{
//				if (next.y < frame.top)
//					frame.top--;
				tplot8 (next);
				ps.push (next);
			}
//			snooze (0);
//			tempView->Sync ();
		}
	}
	else if (tAttribFill->getTolMode() == FILLTOL_TOL)
	{
		while (!(ps.isempty ()))
		{
			uchar t;
			LPoint p = ps.pop ();
			LPoint next;
			next = BPoint (p.x + 1, p.y);
			if (inbounds (next) && istransparent8 (next) && isfillcolort (next, &t))
			{
//				if (next.x > frame.right)
//					frame.right++;
				tplot8 (next, t);
				ps.push (next);
			}
			next = LPoint (p.x - 1, p.y);
			if (inbounds (next) && istransparent8 (next) && isfillcolort (next, &t))
			{
//				if (next.x < frame.left)
//					frame.left--;
				tplot8 (next, t);
				ps.push (next);
			}
			next = LPoint (p.x, p.y + 1);
			if (inbounds (next) && istransparent8 (next) && isfillcolort (next, &t))
			{
//				if (next.y > frame.bottom)
//					frame.bottom++;
				tplot8 (next, t);
				ps.push (next);
			}
			next = LPoint (p.x, p.y - 1);
			if (inbounds (next) && istransparent8 (next) && isfillcolort (next, &t))
			{
//				if (next.y < frame.top)
//					frame.top--;
				tplot8 (next, t);
				ps.push (next);
			}
//			snooze (0);
//			tempView->Sync ();
		}
	}
	else
	{
		while (!(ps.isempty ()))
		{
			uchar t;
			LPoint p = ps.pop ();
			LPoint next;
			next = LPoint (p.x + 1, p.y);
			if (inbounds (next) && istransparent8 (next) && isfillcolorrgb (next, &t))
			{
//				if (next.x > frame.right)
//					frame.right++;
				tplot8 (next, t);
				ps.push (next);
			}
			next = LPoint (p.x - 1, p.y);
			if (inbounds (next) && istransparent8 (next) && isfillcolorrgb (next, &t))
			{
//				if (next.x < frame.left)
//					frame.left--;
				tplot8 (next, t);
				ps.push (next);
			}
			next = LPoint (p.x, p.y + 1);
			if (inbounds (next) && istransparent8 (next) && isfillcolorrgb (next, &t))
			{
//				if (next.y > frame.bottom)
//					frame.bottom++;
				tplot8 (next, t);
				ps.push (next);
			}
			next = LPoint (p.x, p.y - 1);
			if (inbounds (next) && istransparent8 (next) && isfillcolorrgb (next, &t))
			{
//				if (next.y < frame.top)
//					frame.top--;
				tplot8 (next, t);
				ps.push (next);
			}
//			snooze (0);
//			tempView->Sync ();
		}
	}
	
	// printf ("Exited loop.\n");
	pattern curpat = pat->pat();
	BBitmap *temp2 = new BBitmap (fCanvasFrame, B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
	SView *temp2View = new SView (fCanvasFrame, "Temp2View for Fill", B_FOLLOW_NONE, uint32 (NULL));
	temp2->Lock();
	temp2->AddChild (temp2View);
	temp2View->SetDrawingMode (B_OP_COPY);
	if (buttons & B_PRIMARY_MOUSE_BUTTON)
	{
		temp2View->SetHighColor (hicolor->color());
		temp2View->SetLowColor (locolor->color());
	}
	if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
	{
		temp2View->SetHighColor (locolor->color());
		temp2View->SetLowColor (hicolor->color());
	}
	if (c)
	{
		temp2View->SetHighColor (*c);
		temp2View->FillRect (temp2->Bounds());
	}
	else
	{
		temp2View->FillRect (temp2->Bounds(), curpat);
	}
	temp2View->Sync();

	// Change the following code some day to only iterate over BRect frame, since we have that anyway.
	// printf ("Adding alpha channel to temp bitmap\n");
	selection->Lock();
	uint32 *t2bits = (uint32 *) temp2->Bits();
	tbits = (uchar *) temp->Bits() - 1;
	long tbprdif = long (tbpr - tw - 1);
	for (long i = 0; i <= th; i++)
	{
		for (long j = 0; j <= tw; j++)
		{
			uchar *pixel = (uchar *) t2bits++;
			pixel[3] = (pixel[3]* *(++tbits))/255;
		}
		tbits += tbprdif;
	}

	// printf ("Do the actual drawing\n");
	if (mode == M_DRAW)
	{
		if (mAttribDraw->getDrawingMode() == B_OP_BLEND)
			BlendWithAlpha (temp2, currentLayer(), 0, 0);
		else
			AddWithAlpha (temp2, currentLayer(), 0, 0);
	}
	else
	{
		uchar *sbits = (uchar *) selection->Bits();
		long sbprdif = selection->BytesPerRow() - long (tw) - 1;
		t2bits = (uint32 *) temp2->Bits();
		uchar hir = temp2View->HighColor().red;
		uchar hig = temp2View->HighColor().green;
		uchar hib = temp2View->HighColor().blue;
		for (long i = 0; i <= th; i++)
		{
			for (long j = 0; j <= tw; j++)
			{
				uchar *pixel = (uchar *) t2bits++;
				if (pixel[0] == hib && pixel[1] == hig && pixel[2] == hir && pixel[3])
					*sbits = pixel[3];
				sbits++;
			}
			sbits += sbprdif;
		}
	}
	// printf ("Done with the drawing\n");
	selectionView->Sync();
	Sync();
	selection->Unlock();
	delete temp;
	delete temp2;
	currentLayer()->Unlock();
	Invalidate (/*frame*/);
	mainapp->setReady();
}

void CanvasView::tSetColors (uint32 buttons)
{
	extern ColorMenuButton *hicolor, *locolor;
	rgb_color lo = locolor->color();
	rgb_color hi = hicolor->color();

	if (buttons & B_PRIMARY_MOUSE_BUTTON)
	{
		BScreen screen;
		drawView->SetHighColor (hi);
		drawView->SetLowColor (lo);
		SetHighColor (hi);
		SetLowColor (lo);
//		selectionView->SetLowColor (screen.ColorMap()->color_list[(lo.red + lo.green + lo.blue)/3]);
//		selectionView->SetHighColor (screen.ColorMap()->color_list[(hi.red + hi.green + hi.blue)/3]);
		selectionView->SetLowColor (SELECT_NONE);
		selectionView->SetHighColor (SELECT_FULL);
	}
	if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
	{
		BScreen screen;
		drawView->SetHighColor (lo);
		drawView->SetLowColor (hi);
		SetHighColor (lo);
		SetLowColor (hi);
		selectionView->SetLowColor (screen.ColorMap()->color_list[hi.alpha]);
		selectionView->SetHighColor (screen.ColorMap()->color_list[lo.alpha]);
	}
}
