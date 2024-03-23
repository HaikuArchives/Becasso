// © 1998-2001 Sum Software

#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <Box.h>
#include <RadioButton.h>
#include <CheckBox.h>
#include <string.h>

#define RANGE(lb, x, ub) (((x) < (lb)) ? (lb) : (((x) > (ub)) ? (ub) : (x)))

#define RADIAL 0
#define ANGULAR 1

bgra_pixel
interpolate(float x, float y, bgra_pixel p0, bgra_pixel p1, bgra_pixel p2, bgra_pixel p3);

float gAmplitude;
float gWavelength;
float gPhase;
float gDecay;
float gXscale;
float gYscale;
int gRippleType;
bool gInvDecay;

class RippleView : public BView
{
  public:
	RippleView(BRect rect) : BView(rect, "ripple view", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		gAmplitude = 10;
		gWavelength = 25;
		gPhase = 0;
		gDecay = 0;
		gXscale = 1;
		gYscale = 1;
		gRippleType = RADIAL;
		gInvDecay = false;
		ResizeTo(188, 216);
		Slider* aSlid =
			new Slider(BRect(8, 8, 180, 24), 68, "Amplitude", 0, 100, 1, new BMessage('rplA'));
		Slider* wSlid =
			new Slider(BRect(8, 28, 180, 44), 68, "Wavelength", 1, 100, 1, new BMessage('rplW'));
		Slider* pSlid =
			new Slider(BRect(8, 48, 180, 64), 68, "Phase", 0, 360, 1, new BMessage('rplP'));
		Slider* dSlid = new Slider(
			BRect(8, 68, 180, 84), 68, "Decay", 0, 1, 0.01, new BMessage('rplD'), B_HORIZONTAL, 0,
			"%.2f"
		);
		Slider* xSlid = new Slider(
			BRect(8, 88, 180, 104), 68, "X Scale", 0, 2, 0.01, new BMessage('rplX'), B_HORIZONTAL,
			0, "%.2f"
		);
		Slider* ySlid = new Slider(
			BRect(8, 108, 180, 124), 68, "Y Scale", 0, 2, 0.01, new BMessage('rplY'), B_HORIZONTAL,
			0, "%.2f"
		);
		AddChild(aSlid);
		AddChild(wSlid);
		AddChild(pSlid);
		AddChild(dSlid);
		AddChild(xSlid);
		AddChild(ySlid);
		aSlid->SetValue(10);
		wSlid->SetValue(25);
		pSlid->SetValue(0);
		dSlid->SetValue(0);
		xSlid->SetValue(1);
		ySlid->SetValue(1);
		iD = new BCheckBox(
			BRect(8, 130, 180, 154), "invdecay", "Inverse Decay", new BMessage('invD')
		);
		AddChild(iD);
		iD->SetValue(false);
		BBox* type = new BBox(BRect(4, 158, 180, 212), "type");
		type->SetLabel("Type");
		AddChild(type);
		BRadioButton* tA =
			new BRadioButton(BRect(8, 13, 164, 30), "ang", "Angular", new BMessage('rptA'));
		BRadioButton* tR =
			new BRadioButton(BRect(8, 30, 164, 47), "rad", "Radial", new BMessage('rptR'));
		type->AddChild(tA);
		type->AddChild(tR);
		tR->SetValue(true);
	}

	virtual ~RippleView() {}

	virtual void MessageReceived(BMessage* msg);

	BCheckBox* iD;
};

void
RippleView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'rplW':
		gWavelength = msg->FindFloat("value");
		//		printf ("Wavelength %f\n", fWavelength);
		break;
	case 'rplA':
		gAmplitude = msg->FindFloat("value");
		//		printf ("Amplitude %f\n", fAmplitude);
		break;
	case 'rplP':
		gPhase = msg->FindFloat("value");
		//		printf ("Phase %f\n", fPhase);
		break;
	case 'rplD':
		gDecay = msg->FindFloat("value");
		//		printf ("Decay %f\n", fDecay);
		break;
	case 'rplX':
		gXscale = msg->FindFloat("value");
		break;
	case 'rplY':
		gYscale = msg->FindFloat("value");
		break;
	case 'rptA':
		gRippleType = ANGULAR;
		break;
	case 'rptR':
		gRippleType = RADIAL;
		break;
	case 'invD':
		gInvDecay = iD->Value();
		break;
	default:
		BView::MessageReceived(msg);
		return;
	}
	addon_preview();
}

bgra_pixel
interpolate(float x, float y, bgra_pixel p0, bgra_pixel p1, bgra_pixel p2, bgra_pixel p3)
{
	float au, ru, gu, bu, ad, rd, gd, bd;
	ru = x * RED(p0) + (1 - x) * RED(p1);
	rd = x * RED(p3) + (1 - x) * RED(p2);
	gu = x * GREEN(p0) + (1 - x) * GREEN(p1);
	gd = x * GREEN(p3) + (1 - x) * GREEN(p2);
	bu = x * BLUE(p0) + (1 - x) * BLUE(p1);
	bd = x * BLUE(p3) + (1 - x) * BLUE(p2);
	au = x * ALPHA(p0) + (1 - x) * ALPHA(p1);
	ad = x * ALPHA(p3) + (1 - x) * ALPHA(p2);
	return (PIXEL(
		y * ru + (1 - y) * rd, y * gu + (1 - y) * gd, y * bu + (1 - y) * bd, y * au + (1 - y) * ad
	));
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Ripple");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1998-2001 ∑ Sum Software");
	strcpy(info->description, "Maps the image onto rippling fluid");
	info->type = BECASSO_TRANSFORMER;
	info->index = index;
	info->version = 0;
	info->release = 7;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = PREVIEW_2x2 | PREVIEW_MOUSE;
	info->flags = 0;
	return B_OK;
}

status_t
addon_close(void)
{
	return B_OK;
}

status_t
addon_exit(void)
{
	return B_OK;
}

status_t
addon_make_config(BView** view, BRect rect)
{
	*view = new RippleView(rect);
	return B_OK;
}

status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* frame, bool final, BPoint point, uint32 buttons
)
{
	static BPoint center;

	int error = ADDON_OK;
	BRect bounds = inLayer->Bounds();
	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer(*inLayer);
	if (mode == M_SELECT) {
		if (inSelection)
			*outSelection = new Selection(*inSelection);
		else // No Selection to ripple!
			return (error);
	}
	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();

	//	frame->PrintToStream();

	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;
	grey_pixel* mapbits = NULL;
	uint32 mbpr = 0;
	uint32 mdiff = 0;

	float amplitude = gAmplitude / (buttons ? 2 : 1);
	float wavelength = gWavelength / (buttons ? 2 : 1);
	float phase = gPhase * M_PI / 180.0;
	float decay = gDecay;
	int type = gRippleType;
	int invdecay = gInvDecay;

	if (inSelection) {
		mapbits = (grey_pixel*)inSelection->Bits();
		mbpr = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (point != BPoint(-1, -1)) {
		center = point;
		//		printf ("Center set to (%.0f, %.0f\n)", center.x, center.y);
	}

	if (final)
		addon_start();
	float delta = 100.0 / h; // For the Status Bar.

	switch (mode) {
	case M_DRAW: {
		int32 row, col;
		uint32 pprs, pprd;
		bgra_pixel *s, *d;
		int32 firstRow, lastRow, firstCol, lastCol;
		int32 cx, cy;
		float scale_x, scale_y, redwavelength, scaleparam;

		pprs = inLayer->BytesPerRow() / 4;
		pprd = (*outLayer)->BytesPerRow() / 4;

		s = (bgra_pixel*)inLayer->Bits();
		d = (bgra_pixel*)(*outLayer)->Bits();

		firstRow = int32(frame->top);
		lastRow = int32(frame->bottom);
		firstCol = int32(frame->left);
		lastCol = int32(frame->right);

		cx = int32(center.x); //(lastCol - firstCol)/2;
		cy = int32(center.y); //(lastRow - firstRow)/2;

		scale_x = gXscale;
		scale_y = gYscale;

		/* This here is for non-square images, to keep the ripples circular */
		//		scale_x = scale_y = 1.0;
		//		if (w < h)
		//			scale_x = (float) h/w;
		//		else if (w > h)
		//			scale_y = (float) w/h;

		redwavelength = wavelength / (2.0 * M_PI);
		scaleparam = hypot(h, w);

		for (row = 0; row < firstRow; row++)
			memcpy(d + row * pprd, s + row * pprs, inLayer->BytesPerRow());

		for (row = firstRow; row <= lastRow; row++) {
			//			printf ("%d ", row); fflush (stdout);
			float r, dx, dy;
			bgra_pixel* dl = d + row * pprd - 1;
			dy = (row - cy) * scale_y;

			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}

			bgra_pixel* sl = s + row * pprs - 1;
			for (col = 0; col < firstCol; col++)
				*(++dl) = *(++sl);

			grey_pixel* ml = mapbits + row * mbpr + firstCol - 1;

			for (col = firstCol; col <= lastCol; col++) {
				if (!inSelection || *(++ml)) {
					float delta, nx, ny, mx, my;
					uint32 p0, p1, p2, p3;
					uint32 inx, iny;
					dx = (col - cx) * scale_x;
					r = hypot(dx, dy); /* Distance to center of ripples */
					delta =
						amplitude *
						((decay != 0.0) ? (invdecay ? (r / scaleparam) / decay
													: (1 - decay) * (scaleparam - r) / scaleparam)
										: 1.0) *
						sin(r / redwavelength + phase);
					if (type == RADIAL) {
						nx = (delta + dx) / scale_x + cx;
						ny = (delta + dy) / scale_y + cy;
					} else // ANGULAR
					{
						float nr = r + delta;
						float ang = atan2(dx, dy);
						nx = nr * sin(ang) + cx;
						ny = nr * cos(ang) + cy;
					}
					nx = RANGE(firstCol, nx, lastCol);
					ny = RANGE(firstRow, ny, lastRow);

					/* Use simple (2D) interpolation here to get rid of ugly aliasing */
					mx = 1 - fmod(nx, 1.0);
					my = 1 - fmod(ny, 1.0);

					iny = (uint32)ny;
					inx = (uint32)nx;
					if (final) {
						p0 = (s + iny * pprs)[inx];
						p1 = (s + iny * pprs)[inx + (inx < w ? 1 : 0)];
						p2 = (s + (iny + (iny < h ? 1 : 0)) * pprs)[inx + (inx < w ? 1 : 0)];
						p3 = (s + (iny + (iny < h ? 1 : 0)) * pprs)[inx];

						*(++dl) = interpolate(mx, my, p0, p1, p2, p3);
					} else {
						p0 = (s + iny * pprs)[inx];
						*(++dl) = p0;
					}

					++sl;
				} else
					*(++dl) = *(++sl);
			}
			sl = s + row * pprs + lastCol;
			for (col = lastCol + 1; col < w; col++)
				*(++dl) = *(++sl);
		}
		for (row = lastRow + 1; row < h; row++)
			memcpy(d + row * pprd, s + row * pprs, inLayer->BytesPerRow());

		break;
	}
	case M_SELECT: {
		break;
	}
	default:
		fprintf(stderr, "Ripple: Unknown mode\n");
		error = ADDON_UNKNOWN;
	}

	if (*outSelection)
		(*outSelection)->Unlock();
	if (*outLayer)
		(*outLayer)->Unlock();

	if (final)
		addon_done();

	return (error);
}
