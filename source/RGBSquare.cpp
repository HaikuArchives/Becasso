#include "RGBSquare.h"
#include <Screen.h>
#include <stdio.h>
#include "Colors.h"
#include "hsv.h"

RGBSquare::RGBSquare(BRect frame, int nc, ColorWindow* ed)
	: BView(frame, "RGBSquare", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	color_space cs = B_RGB32;
	{
		BScreen screen;
		if (screen.ColorSpace() == B_COLOR_8_BIT)
			cs = B_COLOR_8_BIT;
	}
	colorsquare = new BBitmap(BRect(0, 0, 255, 255), cs);
	colorcol = new BBitmap(BRect(0, 0, 31, 255), cs);
	notcolor = nc;
	editor = ed;
	current = editor->rgb();
	first = true;
	clicks = 0;
	prev = B_ORIGIN;
}

RGBSquare::~RGBSquare()
{
	delete colorsquare;
	delete colorcol;
}

void
RGBSquare::AttachedToWindow()
{
	current = editor->rgb();
	SetColor(current);
	inherited::AttachedToWindow();
}

void
RGBSquare::DrawLines()
{
	SetDrawingMode(B_OP_INVERT);
	SetHighColor(Black);
	switch (notcolor) {
		case 0:	 // Red in the column
			StrokeLine(BPoint(0, current.red), BPoint(31, current.red));
			StrokeLine(BPoint(current.blue + 40, 0), BPoint(current.blue + 40, 255));
			StrokeLine(BPoint(40, current.green), BPoint(295, current.green));
			break;
		case 1:	 // Green in the column
			StrokeLine(BPoint(0, current.green), BPoint(31, current.green));
			StrokeLine(BPoint(current.blue + 40, 0), BPoint(current.blue + 40, 255));
			StrokeLine(BPoint(40, current.red), BPoint(295, current.red));
			break;
		default:  // Blue in the column
			StrokeLine(BPoint(0, current.blue), BPoint(31, current.blue));
			StrokeLine(BPoint(current.green + 40, 0), BPoint(current.green + 40, 255));
			StrokeLine(BPoint(40, current.red), BPoint(295, current.red));
			break;
	}
}

void
RGBSquare::ScreenChanged(BRect /* frame */, color_space mode)
{
	// Note: Only changes from 8 <-> {16, 32} are interesting.
	// 16 bit screens won't use dithering.
	if (colorsquare->ColorSpace() == B_COLOR_8_BIT && mode != B_COLOR_8_BIT) {
		delete colorsquare;
		delete colorcol;
		colorsquare = new BBitmap(BRect(0, 0, 255, 255), B_RGB32);
		colorcol = new BBitmap(BRect(0, 0, 31, 255), B_RGB32);
		first = true;
		AttachedToWindow();
		Invalidate();
	} else if (colorsquare->ColorSpace() != B_COLOR_8_BIT && mode == B_COLOR_8_BIT) {
		delete colorsquare;
		delete colorcol;
		colorsquare = new BBitmap(BRect(0, 0, 255, 255), B_COLOR_8_BIT);
		colorcol = new BBitmap(BRect(0, 0, 31, 255), B_COLOR_8_BIT);
		first = true;
		AttachedToWindow();
		Invalidate();
	}
}

void
RGBSquare::Draw(BRect /* update */)
{
	BRect middle, right;
	middle.Set(32, 0, 39, 256);
	right.Set(295, 0, Bounds().Width(), 256);
	SetDrawingMode(B_OP_COPY);
	DrawBitmapAsync(colorsquare, BPoint(40, 0));
	DrawBitmap(colorcol, BPoint(0, 0));
	SetHighColor(LightGrey);
	FillRect(middle);
	FillRect(right);
	DrawLines();
}

void
RGBSquare::MouseDown(BPoint point)
{
	if (point == prev)
		clicks++;
	else
		clicks = 0;
	prev = point;

	if (clicks > 1) {
		uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");
		if (buttons & B_PRIMARY_MOUSE_BUTTON) {
			BMessage set('CXSm');
			Window()->PostMessage(&set);
		}
		if (buttons & B_SECONDARY_MOUSE_BUTTON) {
			BMessage set('CXSc');
			Window()->PostMessage(&set);
		}
		inherited::MouseDown(point);
	} else {
		thread_id id = spawn_thread(RGB_track_mouse, "RGB tracker", B_DISPLAY_PRIORITY, this);
		resume_thread(id);
	}
}

void
RGBSquare::mouseDown(BPoint point, int ob)
{
	if (ob == 1) {
		float prevy = -1;
		if (prevy != point.y) {
			rgb_color c = current;
			switch (notcolor) {
				case 0:	 // Red in the column
					c.red = clipchar(point.y);
					break;
				case 1:	 // Green in the column
					c.green = clipchar(point.y);
					break;
				default:  // Blue in the column
					c.blue = clipchar(point.y);
					break;
			}
			SetColor(c);
			Window()->UpdateIfNeeded();
		}
		BMessage* msg = new BMessage('CSQc');
		msg->AddInt32("color", current.red);
		msg->AddInt32("color", current.green);
		msg->AddInt32("color", current.blue);
		Window()->PostMessage(msg);
		delete msg;
		prevy = point.y;
	} else if (ob == 2) {
		DrawLines();
		switch (notcolor) {
			case 0:	 // Red in the column
				current.green = clipchar(point.y);
				current.blue = clipchar(point.x - 40);
				break;
			case 1:	 // Green in the column
				current.red = clipchar(point.y);
				current.blue = clipchar(point.x - 40);
				break;
			default:  // Blue in the column
				current.red = clipchar(point.y);
				current.green = clipchar(point.x - 40);
				break;
		}
		DrawLines();
		Window()->UpdateIfNeeded();
		BMessage* msg = new BMessage('CSQc');
		msg->AddInt32("color", current.red);
		msg->AddInt32("color", current.green);
		msg->AddInt32("color", current.blue);
		Window()->PostMessage(msg);
		delete msg;
	}
}

int32
RGB_track_mouse(void* data)
{
	BRect colRect = BRect(0, 0, 31, 255);
	BRect squareRect = BRect(40, 0, 295, 255);
	BMessage* msg;
	RGBSquare* obj = (RGBSquare*)data;
	uint32 buttons;
	BPoint point;
	BPoint prev = BPoint(-1, -1);
	obj->Window()->Lock();
	obj->GetMouse(&point, &buttons, true);
	if (colRect.Contains(point)) {
		msg = new BMessage('RGBc');
		obj->Window()->PostMessage(msg);
		delete msg;
	} else if (squareRect.Contains(point)) {
		msg = new BMessage('RGBs');
		obj->Window()->PostMessage(msg);
		delete msg;
	}
	while (buttons) {
		if (prev != point) {
			msg = new BMessage('RGBm');
			msg->AddFloat("x", point.x);
			msg->AddFloat("y", point.y);
			obj->Window()->PostMessage(msg);
			delete msg;
			prev = point;
		}
		obj->Window()->Unlock();
		snooze(50000);
		obj->Window()->Lock();
		obj->GetMouse(&point, &buttons, true);
	}
	msg = new BMessage('RGBu');
	obj->Window()->PostMessage(msg);
	delete msg;
	obj->Window()->Unlock();
	return 0;
}

void
RGBSquare::SetColor(rgb_color c)
// There is a one pixel black border which I don't understand...
{
	// This could be faster... In the meantime, just do it.
	if (true || first || ((notcolor == 0) && (c.red != current.red))
		|| ((notcolor == 1) && (c.green != current.green))
		|| ((notcolor == 2) && (c.blue != current.blue))) {
		if (notcolor == 0)	// Red in the column
		{
			uchar r = c.red;
			for (int g = 0; g < 256; g++) {
				for (int b = 0; b < 256; b++) {
					square[g][b][0] = r;
					square[g][b][1] = g;
					square[g][b][2] = b;
				}
				for (int x = 0; x < 32; x++) {
					col[g][x][0] = g;
					col[g][x][1] = 0;
					col[g][x][2] = 0;
				}
			}
		} else if (notcolor == 1)  // Green in the column
		{
			uchar g = c.green;
			for (int r = 0; r < 256; r++) {
				for (int b = 0; b < 256; b++) {
					square[r][b][0] = r;
					square[r][b][1] = g;
					square[r][b][2] = b;
				}
				for (int x = 0; x < 32; x++) {
					col[r][x][0] = 0;
					col[r][x][1] = r;
					col[r][x][2] = 0;
				}
			}
		} else	// Blue in the column
		{
			uchar b = c.blue;
			for (int r = 0; r < 256; r++) {
				for (int g = 0; g < 256; g++) {
					square[r][g][0] = r;
					square[r][g][1] = g;
					square[r][g][2] = b;
				}
				for (int x = 0; x < 32; x++) {
					col[r][x][0] = 0;
					col[r][x][1] = 0;
					col[r][x][2] = r;
				}
			}
		}
		current = c;
		//		uchar *test = (uchar *) colorsquare->Bits();
		//		printf ("Alpha = %i\n", int (test[3]));
		colorsquare->SetBits(square, 256 * 256 * 3, 0, B_RGB32);
		colorcol->SetBits(col, 256 * 32 * 3, 0, B_RGB32);
		//		printf ("Alpha = %i\n", int (test[3]));
		first = false;
	}
	Invalidate();
}

void
RGBSquare::SetNotColor(int nc)
{
	notcolor = nc;
	SetColor(current);
}

rgb_color
RGBSquare::GetColor()
{
	return current;
}