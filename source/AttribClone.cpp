#include "AttribClone.h"
#include "Colors.h"
#include <Box.h>
#include <Picture.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Settings.h"

static property_info prop_list[] = {
	{"Spacing", SET, DIRECT, "float: 1 .. 64"},
	{"Strength", SET, DIRECT, "int: 0 .. 255"},
	{"Angle", SET, DIRECT, "float: 0 .. 45"},
	{"Width|XSize", SET, DIRECT, "int: 0 .. 40"},
	{"Height|YSize", SET, DIRECT, "int: 0 .. 40"},
	{"Hardness", SET, DIRECT, "int: 0 .. 100"},
	0
};

AttribClone::AttribClone() : AttribView(BRect(0, 0, 160, 280), lstring(35, "Clone"))
{
	SetViewColor(LightGrey);
	fSpacing = 1;
	fAngle = 0;
	fX = 5;
	fY = 5;
	fHardness = 1;
	fStrength = 128;
	fBrush = NULL;
	bv = new BitmapView(BRect(4, 4, 154, 154), "BrushBitmapView");
	AddChild(bv);
	ConstructBrush();
	xSlid = new Slider(
		BRect(4, 160, 154, 176), 60, lstring(300, "X-size"), 1, 40, 1, new BMessage('ABxc'),
		B_HORIZONTAL, 26
	);
	ySlid = new Slider(
		BRect(4, 180, 154, 196), 60, lstring(301, "Y-size"), 1, 40, 1, new BMessage('AByc'),
		B_HORIZONTAL, 26
	);
	aSlid = new Slider(
		BRect(4, 200, 154, 216), 60, lstring(302, "Angle"), 0, 45, 1, new BMessage('ABac'),
		B_HORIZONTAL, 26
	);
	sSlid = new Slider(
		BRect(4, 220, 154, 236), 60, lstring(303, "Strength"), 0, 255, 1, new BMessage('ABsc'),
		B_HORIZONTAL, 26
	);
	hSlid = new Slider(
		BRect(4, 240, 154, 256), 60, lstring(305, "Hardness"), 1, 100, 1, new BMessage('ABph'),
		B_HORIZONTAL, 26
	);
	pSlid = new Slider(
		BRect(4, 260, 154, 276), 60, lstring(304, "Spacing"), 1, 64, 1, new BMessage('ABpc'),
		B_HORIZONTAL, 26
	);
	AddChild(xSlid);
	AddChild(ySlid);
	AddChild(aSlid);
	AddChild(sSlid);
	AddChild(hSlid);
	AddChild(pSlid);
	xSlid->SetValue(fX);
	ySlid->SetValue(fY);
	sSlid->SetValue(fStrength);
	hSlid->SetValue(fHardness);
	fCurrentProperty = 0;
}

AttribClone::~AttribClone()
{
	//	RemoveChild (bv);
	//	delete bv;
	delete fBrush;
}

void
AttribClone::ConstructBrush()
{
	delete fBrush;
	float rangle = -fAngle / 180 * M_PI;
	int ssx = 2 * fX + 1;
	int ssy = 2 * fY + 1;
	int sx = max_c(
		abs(int(ssx * cos(rangle))), abs(int((ssy * sin(rangle)) * (1.0 + sin(-rangle) / 4)))
	);
	int sy = max_c(
		abs(int(ssy * cos(rangle))), abs(int((ssx * sin(rangle)) * (1.0 + sin(-rangle) / 4)))
	);
	// printf ("Width: %d, Height: %d\n", sx, sy);
	fBrush = new Brush(sy * 1.1, sx * 1.1, fSpacing);
	mkGaussianBrush(fBrush, fX, fY, fAngle, fStrength, fHardness / 10 + 1);
	bv->SetBitmap(fBrush->ToBitmap(Black, White));
	//	printf ("bv = %p, fBrush = %p\n", bv, fBrush);
}

inline float
sgn(float f)
{
	return (f < 0 ? -1 : (f == 0 ? 0 : 1));
}

inline float
hfunc(float r, float s)
{
	return (pow(r, s));
}

void
AttribClone::mkGaussianBrush(
	Brush* b, float sigmaxsq, float sigmaysq, float angle, int cval, float hardness
)
{
	int hw = b->Width() / 2;
	int hh = b->Height() / 2;
	float rangle = -angle / 180 * M_PI;
	for (float x = -hw; x <= hw; x++) {
		for (float y = -hh; y <= hh; y++) {
			register float rx = (x * cos(rangle) + y * sin(rangle)) / sigmaxsq * 2;
			register float ry = (x * sin(rangle) - y * cos(rangle)) / sigmaysq * 2;
			register float r = rx * rx + ry * ry;
			b->Set(hw + x, hh + y, cval * exp(-(hfunc(r, hardness))));
		}
	}
}

void
AttribClone::mkDiagonalBrush(Brush* b, int dir)
{
	int d = min_c(b->Height(), b->Width()) - 1;
	if (dir > 0) // forward slash
	{
		for (int i = 1; i < d; i++) {
			b->Set(i, d - i, 255);
			b->Set(i - 1, d - i, 128);
			b->Set(i, d - i + 1, 128);
		}
		b->Set(0, d, 64);
		b->Set(d, 0, 64);
		b->Set(d, 1, 128);
		b->Set(d - 1, 0, 128);
	} else // backward slash
	{
		for (int i = 1; i < d; i++) {
			b->Set(i, i, 255);
			b->Set(i, i - 1, 128);
			b->Set(i - 1, i, 128);
		}
		b->Set(0, 0, 64);
		b->Set(d, d, 64);
		b->Set(d, d - 1, 128);
		b->Set(d - 1, d, 128);
	}
}

status_t
AttribClone::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Clone");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribClone::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
)
{
	//	printf ("\nmessage:\n");
	//	message->PrintToStream();
	//	printf ("specifier:\n");
	//	specifier->PrintToStream();
	//	printf ("Property: %s\n", property);

	if (!strcasecmp(property, "Spacing")) {
		fCurrentProperty = PROP_SPACING;
		return this;
	}
	if (!strcasecmp(property, "Strength")) {
		fCurrentProperty = PROP_STRENGTH;
		return this;
	}
	if (!strcasecmp(property, "Angle")) {
		fCurrentProperty = PROP_ANGLE;
		return this;
	}
	if (!strcasecmp(property, "Width") || !strcasecmp(property, "XSize")) {
		fCurrentProperty = PROP_X;
		return this;
	}
	if (!strcasecmp(property, "Height") || !strcasecmp(property, "YSize")) {
		fCurrentProperty = PROP_Y;
		return this;
	}
	if (!strcasecmp(property, "Hardness")) {
		fCurrentProperty = PROP_HARDNESS;
		return this;
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribClone::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case B_GET_PROPERTY: {
		switch (fCurrentProperty) {
		}
		fCurrentProperty = 0;
		inherited::MessageReceived(msg);
		break;
	}
	case B_SET_PROPERTY: {
		switch (fCurrentProperty) {
		case PROP_SPACING: // float, 1 .. 64
		{
			float value;
			int32 ivalue;
			bool floatvalid = false;
			if (msg->FindInt32("data", &ivalue) == B_OK) // OK, we'll take int32's too.
			{
				value = ivalue;
				floatvalid = true;
			}
			if (floatvalid || msg->FindFloat("data", &value) == B_OK) {
				if (value >= 1 && value <= 40) {
					fSpacing = value;
					pSlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
					ConstructBrush();
				}
			}
			break;
		}
		case PROP_STRENGTH: // int, 0 .. 255
		{
			int32 value;
			if (msg->FindInt32("data", &value) == B_OK) {
				if (value >= 0 && value <= 255) {
					fStrength = value;
					sSlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
					ConstructBrush();
				}
			}
			break;
		}
		case PROP_HARDNESS: // int, 0 .. 100
		{
			int32 value;
			if (msg->FindInt32("data", &value) == B_OK) {
				if (value >= 1 && value <= 100) {
					fHardness = value;
					hSlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
					ConstructBrush();
				}
			}
			break;
		}
		case PROP_ANGLE: // float, 0 .. 45
		{
			float value;
			int32 ivalue;
			bool floatvalid = false;
			if (msg->FindInt32("data", &ivalue) == B_OK) // OK, we'll take int32's too.
			{
				value = ivalue;
				floatvalid = true;
			}
			if (floatvalid || msg->FindFloat("data", &value) == B_OK) {
				if (value >= 0 && value <= 45) {
					fAngle = value;
					aSlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
					ConstructBrush();
				}
			}
			break;
		}
		case PROP_X: // int, 1 .. 40
		{
			int32 value;
			if (msg->FindInt32("data", &value) == B_OK) {
				if (value >= 1 && value <= 40) {
					fX = value;
					xSlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
					ConstructBrush();
				}
			}
			break;
		}
		case PROP_Y: // int, 1 .. 40
		{
			int32 value;
			if (msg->FindInt32("data", &value) == B_OK) {
				if (value >= 1 && value <= 40) {
					fY = value;
					ySlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
					ConstructBrush();
				}
			}
			break;
		}
		}
		fCurrentProperty = 0;
		inherited::MessageReceived(msg);
		break;
	}
	case 'ABxc':
		fX = int(msg->FindFloat("value"));
		ConstructBrush();
		fSpacing = sqrt(fX + fY);
		pSlid->SetValue(fSpacing);
		break;
	case 'AByc':
		fY = int(msg->FindFloat("value"));
		fSpacing = sqrt(fX + fY);
		pSlid->SetValue(fSpacing);
		ConstructBrush();
		break;
	case 'ABsc':
		fStrength = int(msg->FindFloat("value"));
		ConstructBrush();
		break;
	case 'ABac':
		fAngle = msg->FindFloat("value");
		ConstructBrush();
		break;
	case 'ABph':
		fHardness = msg->FindFloat("value");
		ConstructBrush();
		break;
	case 'ABpc':
		fSpacing = msg->FindFloat("value");
		ConstructBrush();
		break;
	default:
		// msg->PrintToStream();
		inherited::MessageReceived(msg);
	}
	bv->Invalidate();
}
