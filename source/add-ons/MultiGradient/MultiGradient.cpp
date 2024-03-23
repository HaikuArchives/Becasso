// © 1999 Sum Software

#include "BecassoAddOn.h"
#include "AddOnWindow.h"
#include "AddOnSupport.h"
#include <Box.h>
#include <RadioButton.h>

#define G_LINEAR 1
#define G_CIRCULAR 2
#define G_COSINE 3

class GradientWindow : public AddOnWindow
{
  public:
	GradientWindow(BRect rect, becasso_addon_info* info) : AddOnWindow(rect, info)
	{
		fOrientation = G_LINEAR;
		fSlope = G_LINEAR;
	};

	virtual ~GradientWindow(){};
	virtual void MessageReceived(BMessage* msg);

	int orientation() { return fOrientation; };

	int slope() { return fSlope; };

  private:
	typedef AddOnWindow inherited;
	int fOrientation;
	int fSlope;
};

void
GradientWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'groL':
		fOrientation = G_LINEAR;
		break;
	case 'groC':
		fOrientation = G_CIRCULAR;
		break;
	case 'grsL':
		fSlope = G_LINEAR;
		break;
	case 'grsS':
		fSlope = G_COSINE;
		break;
	case 'grsC':
		fSlope = G_CIRCULAR;
		break;
	default:
		inherited::MessageReceived(msg);
		return;
	}
	aPreview();
}

GradientWindow* window;

int
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "MultiGradient");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1999 ∑ Sum Software");
	strcpy(info->description, "Generates a multicolor gradient");
	info->type = BECASSO_GENERATOR;
	info->index = index;
	info->version = 0;
	info->release = 1;
	info->becasso_version = 1;
	info->becasso_release = 0;
	info->does_preview = PREVIEW_FULLSCALE | PREVIEW_MOUSE;
	window = new GradientWindow(BRect(100, 180, 100 + 188, 180 + 204), info);
	BBox* orientation = new BBox(BRect(4, 4, 184, 58), "orientation");
	orientation->SetLabel("Orientation");
	window->Background()->AddChild(orientation);
	BRadioButton* oLinear =
		new BRadioButton(BRect(8, 13, 164, 30), "lin", "Linear", new BMessage('groL'));
	BRadioButton* oCircular =
		new BRadioButton(BRect(8, 30, 164, 47), "circ", "Circular", new BMessage('groC'));
	orientation->AddChild(oLinear);
	orientation->AddChild(oCircular);
	oLinear->SetValue(true);
	BBox* slope = new BBox(BRect(4, 64, 184, 134), "slope");
	slope->SetLabel("Slope");
	window->Background()->AddChild(slope);
	BRadioButton* sLinear =
		new BRadioButton(BRect(8, 13, 164, 30), "slin", "Linear", new BMessage('grsL'));
	BRadioButton* sCosine =
		new BRadioButton(BRect(8, 30, 164, 47), "scos", "Half Cosine", new BMessage('grsS'));
	BRadioButton* sCircular =
		new BRadioButton(BRect(8, 47, 164, 64), "scirc", "Circle Arc", new BMessage('grsC'));
	slope->AddChild(sLinear);
	slope->AddChild(sCosine);
	slope->AddChild(sCircular);
	sLinear->SetValue(true);
	window->Run();
	return (0);
}

int
addon_exit(void)
{
	return (0);
}

int
addon_open(BWindow* client, const char* name)
{
	char title[B_FILE_NAME_LENGTH];
	sprintf(title, "Gradient %s", name);
	window->Lock();
	window->SetTitle(title);
	if (window->IsHidden())
		window->aShow(client);
	else
		window->aActivate(client);
	window->Unlock();
	return (0);
}

int
addon_close()
{
	window->aClose();
	return (0);
}

int
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* /* frame */, bool final, BPoint point, uint32 buttons
)
{
	static BPoint firstpoint = BPoint(-1, -1);
	static BPoint lastpoint = BPoint(-1, -1);
	static bool entry = false;

	if (!final && buttons && !entry) // First entry of a realtime drag
	{
		firstpoint = point;
		entry = true;
		return (ADDON_OK);
	}
	if (!final && !buttons) // Exit a realtime drag (buttons released)
	{
		entry = false;
	}
	if (buttons)
		lastpoint = point;
	int error = ADDON_OK;
	BRect bounds = inLayer->Bounds();
	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer(*inLayer);

	if (*outSelection == NULL && mode == M_SELECT)
		*outSelection = new Selection(inLayer->Bounds());

	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();
	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;
	grey_pixel* mapbits = NULL;
	uint32 mbpr = 0;
	uint32 mdiff = 0;
	if (inSelection) {
		mapbits = (grey_pixel*)inSelection->Bits() - 1;
		mbpr = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}
	if (final) {
		window->Lock();
		window->Start();
		window->Unlock();
	}
	float delta = 100.0 / h; // For the Status Bar.

	rgb_color hi = highcolor();
	rgb_color lo = lowcolor();

	float vx = lastpoint.x - firstpoint.x;
	float vy = lastpoint.y - firstpoint.y;
	float d = hypot(vx, vy);
	if (d < 1)
		d = 1;
	float nx = vy / d;
	float ny = -vx / d;

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* sbits = (uint32*)inLayer->Bits() - 1;
		bgra_pixel* dbits = (uint32*)(*outLayer)->Bits() - 1;
		for (uint32 y = 0; y < h; y++) {
			if (final) {
				window->Lock();
				window->UpdateStatusBar(delta);
				if (window->Stop()) {
					error = ADDON_ABORT;
					window->Unlock();
					break;
				}
				window->Unlock();
			}
			for (uint32 x = 0; x < w; x++) {
				uint8 val = 0;
				float dist = 0;
				switch (window->orientation()) {
				case G_LINEAR: {
					float f = ((x - firstpoint.x) * nx + (y - firstpoint.y) * ny);
					float cx = firstpoint.x + f * nx;
					float cy = firstpoint.y + f * ny;
					dist = hypot(x - cx, y - cy);
					if ((x - cx) * vx < 0 || (y - cy) * vy < 0)
						dist = 0;
					break;
				}
				case G_CIRCULAR:
					dist = hypot(firstpoint.x - x, firstpoint.y - y);
					break;
				default:
					printf("Huu!\n");
					break;
				}
				if (dist > d)
					dist = d;
				if (dist < 0)
					dist = 0;
				switch (window->slope()) {
				case G_LINEAR:
					val = grey_pixel(255 * dist / d);
					break;
				case G_CIRCULAR:
					val = grey_pixel(255 - 255 * sqrt(1 - dist * dist / (d * d)));
					break;
				case G_COSINE:
					val = grey_pixel(255 - 127.5 * (cos(M_PI * dist / d) + 1));
					break;
				default:
					printf("Huu!\n");
					break;
				}
				bgra_pixel pixel = weighted_average_rgb(lo, val, hi, 255 - val) & COLOR_MASK;
				if (inSelection) {
					*(++dbits) = pixelblend(*(++sbits), (pixel | (*(++mapbits) << ALPHA_BPOS)));
				} else {
					*(++dbits) = pixel | ALPHA_MASK;
					sbits++;
				}
			}
			mapbits += mdiff;
		}
		if (*outLayer && buttons) {
			BView* view = new BView(inLayer->Bounds(), "tmp view", 0, 0);
			(*outLayer)->AddChild(view);
			view->SetHighColor(contrastingcolor(hi, lo));
			view->StrokeLine(firstpoint, lastpoint);
			view->Sync();
			(*outLayer)->RemoveChild(view);
			delete view;
		}
		break;
	}
	case M_SELECT: {
		for (uint32 y = 0; y < h; y++) {
			if (final) {
				window->Lock();
				window->UpdateStatusBar(delta);
				if (window->Stop()) {
					error = ADDON_ABORT;
					window->Unlock();
					break;
				}
				window->Unlock();
			}
			grey_pixel* sbits = 0;
			if (inSelection)
				sbits = (grey_pixel*)inSelection->Bits() + y * mbpr - 1;
			grey_pixel* dbits = (grey_pixel*)(*outSelection)->Bits() + y * mbpr - 1;
			for (uint32 x = 0; x < w; x++) {
				uint8 val = 0;
				float dist = 0;
				switch (window->orientation()) {
				case G_LINEAR: {
					float f = ((x - firstpoint.x) * nx + (y - firstpoint.y) * ny);
					float cx = firstpoint.x + f * nx;
					float cy = firstpoint.y + f * ny;
					dist = hypot(x - cx, y - cy);
					if ((x - cx) * vx < 0 || (y - cy) * vy < 0)
						dist = 0;
					break;
				}
				case G_CIRCULAR:
					dist = hypot(firstpoint.x - x, firstpoint.y - y);
					break;
				default:
					printf("Huu!\n");
					break;
				}
				if (dist > d)
					dist = d;
				if (dist < 0)
					dist = 0;
				switch (window->slope()) {
				case G_LINEAR:
					val = grey_pixel(255 - 255 * dist / d);
					break;
				case G_CIRCULAR:
					val = grey_pixel(255 * sqrt(1 - dist * dist / (d * d)));
					break;
				case G_COSINE:
					val = grey_pixel(127.5 * (cos(M_PI * dist / d) + 1));
					break;
				default:
					printf("Huu!\n");
					break;
				}
				if (inSelection)
					*(++dbits) = val * (*(++sbits)) / 255;
				else
					*(++dbits) = val;
			}
		}
		if (*outSelection && buttons) {
			BView* view = new BView(inLayer->Bounds(), "tmp view", 0, 0);
			(*outSelection)->AddChild(view);
			view->SetHighColor(Black);
			view->StrokeLine(firstpoint, lastpoint);
			view->Sync();
			(*outSelection)->RemoveChild(view);
			delete view;
		}
		break;
	}
	default:
		fprintf(stderr, "MultiGradient: Unknown mode\n");
		error = ADDON_UNKNOWN;
	}

	if (*outSelection)
		(*outSelection)->Unlock();
	if (*outLayer)
		(*outLayer)->Unlock();

	if (final) {
		window->Lock();
		window->Stopped();
		window->Unlock();
	}

	return (error);
}
