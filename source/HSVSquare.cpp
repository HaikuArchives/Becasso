#include "HSVSquare.h"
#include <Screen.h>
#include "Colors.h"
#include <math.h>
#include "BecassoAddOn.h"

HSVSquare::HSVSquare(BRect frame, ColorWindow* ed)
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
	editor = ed;
	prevy = -1;
	for (int x = -127; x <= 128; x++)
		for (int y = -127; y <= 128; y++) {
			if (x * x + y * y < 16129 && (x || y)) {
				float hue = atan2(x, y) / M_PI * 180 - 90;
				if (hue < 0)
					hue += 360;
				hs[y + 127][x + 127][0] = hue;
				hs[y + 127][x + 127][1] = sqrt(x * x + y * y) / 127;
			}
			else if (!x && !y)
				hs[y + 127][x + 127][0] = -1;
			else
				hs[y + 127][x + 127][0] = -2;
		}

	prev = B_ORIGIN;
	clicks = 0;
	fFirst = true;
}

HSVSquare::~HSVSquare()
{
	delete colorsquare;
	delete colorcol;
}

void
HSVSquare::AttachedToWindow()
{
	for (int s = 0; s < 256; s++)
		for (int x = 0; x < 32; x++) {
			col[s][x][0] = s;
			col[s][x][1] = s;
			col[s][x][2] = s;
		}
	colorcol->SetBits(col, 256 * 32 * 3, 0, B_RGB32);
	currentHSV.value = 0.5;
	SetColor(rgb2hsv(editor->rgb()));
	inherited::AttachedToWindow();
}

void
HSVSquare::ScreenChanged(BRect /* frame */, color_space cs)
{
	// Note: Only changes from 8 <-> {16, 32} are interesting.
	// 16 bit screens won't use dithering.
	if (colorsquare->ColorSpace() == B_COLOR_8_BIT && cs != B_COLOR_8_BIT) {
		delete colorsquare;
		delete colorcol;
		colorsquare = new BBitmap(BRect(0, 0, 255, 255), B_RGB32);
		colorcol = new BBitmap(BRect(0, 0, 31, 255), B_RGB32);
		AttachedToWindow();
		Invalidate();
	}
	else if (colorsquare->ColorSpace() != B_COLOR_8_BIT && cs == B_COLOR_8_BIT) {
		delete colorsquare;
		delete colorcol;
		colorsquare = new BBitmap(BRect(0, 0, 255, 255), B_COLOR_8_BIT);
		colorcol = new BBitmap(BRect(0, 0, 31, 255), B_COLOR_8_BIT);
		AttachedToWindow();
		Invalidate();
	}
}

void
HSVSquare::DrawLines()
{
	SetDrawingMode(B_OP_COPY);
	SetHighColor(Red);
	StrokeLine(BPoint(0, currentHSV.value * 255), BPoint(31, currentHSV.value * 255));
	SetDrawingMode(B_OP_INVERT);
	float x = 40 + 127 + 127 * currentHSV.saturation * cos(currentHSV.hue * M_PI / 180);
	float y = 127 - 127 * currentHSV.saturation * sin(currentHSV.hue * M_PI / 180);
	StrokeEllipse(BPoint(x, y), 3, 3);
}

void
HSVSquare::Draw(BRect /* update */)
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
	Sync();
	DrawLines();
}

void
HSVSquare::MouseDown(BPoint point)
{
	if (point == prev)
		clicks++;
	else
		clicks = 0;
	prev = point;

	if (clicks > 1) {
		uint32 buttons;
		GetMouse(&point, &buttons); // flush the buffer
		if (buttons & B_PRIMARY_MOUSE_BUTTON) {
			BMessage set('CXSm');
			Window()->PostMessage(&set);
		}
		if (buttons & B_SECONDARY_MOUSE_BUTTON) {
			BMessage set('CXSc');
			Window()->PostMessage(&set);
		}
	}
	else {
		thread_id id = spawn_thread(HSV_track_mouse, "HSV tracker", B_DISPLAY_PRIORITY, this);
		resume_thread(id);
	}
}

void
HSVSquare::mouseDown(BPoint point, int ob)
{
	float x = point.x - 127 - 40;
	float y = point.y - 127;

	if (ob == 1) {
		if (prevy != point.y) {
			DrawLines();
			hsv_color newc;
			newc.hue = currentHSV.hue;
			newc.saturation = currentHSV.saturation;
			newc.value = clipone(point.y / 255);
			BMessage* msg = new BMessage('CSQh');
			msg->AddFloat("color", newc.hue);
			msg->AddFloat("color", newc.saturation);
			msg->AddFloat("color", newc.value);
			Window()->PostMessage(msg);
			delete msg;
			SetColor(newc);
			DrawLines();
		}
		prevy = point.y;
	}
	else if (ob == 2) {
		if (x == 0 && y == 0) {
			DrawLines();
			currentHSV.hue = HUE_UNDEF;
			currentHSV.saturation = 0;
			DrawLines();
			BMessage* msg = new BMessage('CSQh');
			msg->AddFloat("color", currentHSV.hue);
			msg->AddFloat("color", currentHSV.saturation);
			msg->AddFloat("color", currentHSV.value);
			Window()->PostMessage(msg);
			delete msg;
		}
		else {
			DrawLines();
			currentHSV.hue = clipdegr(atan2(x, y) / M_PI * 180 - 90);
			currentHSV.saturation = clipone(sqrt(x * x + y * y) / 127);
			DrawLines();
			BMessage* msg = new BMessage('CSQh');
			msg->AddFloat("color", currentHSV.hue);
			msg->AddFloat("color", currentHSV.saturation);
			msg->AddFloat("color", currentHSV.value);
			Window()->PostMessage(msg);
			delete msg;
		}
	}
}

int32
HSV_track_mouse(void* data)
{
	BRect colRect = BRect(0, 0, 31, 255);
	BRect squareRect = BRect(40, 0, 295, 255);
	BMessage* msg;
	HSVSquare* obj = (HSVSquare*)data;
	uint32 buttons;
	BPoint point;
	BPoint prev = BPoint(-1, -1);
	obj->Window()->Lock();
	obj->GetMouse(&point, &buttons, true);
	if (colRect.Contains(point)) {
		msg = new BMessage('HSVc');
		obj->Window()->PostMessage(msg);
		delete msg;
	}
	else if (squareRect.Contains(point)) {
		float x = point.x - 127 - 40;
		float y = point.y - 127;
		if (x * x + y * y < 16129) {
			msg = new BMessage('HSVs');
			obj->Window()->PostMessage(msg);
			delete msg;
		}
	}
	while (buttons) {
		if (prev != point) {
			msg = new BMessage('HSVm');
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
	msg = new BMessage('HSVu');
	obj->Window()->PostMessage(msg);
	delete msg;
	obj->Window()->Unlock();
	return 0;
}

void
HSVSquare::SetColor(rgb_color c)
{
	SetColor(rgb2hsv(c));
}

void
HSVSquare::SetColor(hsv_color h)
{
	float value = currentHSV.value;
	currentHSV = h;
	current = hsv2rgb(h);
	if (value != currentHSV.value) {
		for (int x = -127; x <= 128; x++)
			for (int y = -127; y <= 128; y++) {
				if (hs[y + 127][x + 127][0] >= 0) {
					hsv_color c;
					c.hue = hs[y + 127][x + 127][0];
					c.saturation = hs[y + 127][x + 127][1];
					c.value = currentHSV.value;
					rgb_color d = hsv2rgb(c);
					square[y + 127][x + 127][0] = d.red;
					square[y + 127][x + 127][1] = d.green;
					square[y + 127][x + 127][2] = d.blue;
				}
				else {
					square[y + 127][x + 127][0] = LightGrey.red;
					square[y + 127][x + 127][1] = LightGrey.green;
					square[y + 127][x + 127][2] = LightGrey.blue;
				}
				square[127][127][0] = uint8(currentHSV.value * 255);
				square[127][127][1] = uint8(currentHSV.value * 255);
				square[127][127][2] = uint8(currentHSV.value * 255);
			}
		colorsquare->SetBits(square, 256 * 256 * 3, 0, B_RGB32);
	}
	Invalidate();
}

rgb_color
HSVSquare::GetColor()
{
	return current;
}

hsv_color
HSVSquare::GetColorHSV()
{
	return currentHSV;
}
