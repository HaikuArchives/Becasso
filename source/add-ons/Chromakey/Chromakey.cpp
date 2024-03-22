// © 2002 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <string.h>
#include <stdlib.h>
#include <Box.h>
#include <RadioButton.h>

int gSigma;
int gThreshold;
int gMode;

#define CK_BLUE 0
#define CK_FG 1
#define CK_BG 2

class CKView : public BView {
  public:
	CKView(BRect rect);

	virtual ~CKView() {}

	virtual void MessageReceived(BMessage* msg);

  private:
	typedef BView inherited;
};

CKView::CKView(BRect rect) : BView(rect, "CKView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	gSigma = 5;
	gThreshold = 5;
	gMode = CK_BLUE;
	BBox* mode = new BBox(BRect(8, 4, 180, 78), "mode");
	mode->SetLabel("Key Color");
	AddChild(mode);

	BRadioButton* rbBl =
		new BRadioButton(BRect(4, 14, 170, 30), "blue", "Blue", new BMessage('ckBl'));
	BRadioButton* rbFG =
		new BRadioButton(BRect(4, 32, 170, 48), "fg", "Foreground", new BMessage('ckFG'));
	BRadioButton* rbBG =
		new BRadioButton(BRect(4, 50, 170, 66), "bg", "Background", new BMessage('ckBG'));
	mode->AddChild(rbBl);
	mode->AddChild(rbFG);
	mode->AddChild(rbBG);

	rbBl->SetValue(true);
	Slider* xSlid = new Slider(BRect(8, 82, 180, 98), 60, "Sigma", 0, 100, 1, new BMessage('ckSg'));
	Slider* ySlid =
		new Slider(BRect(8, 102, 180, 118), 60, "Threshold", 0, 100, 1, new BMessage('ckTh'));
	AddChild(xSlid);
	AddChild(ySlid);
	xSlid->SetValue(gSigma);
	ySlid->SetValue(gThreshold);

	ResizeTo(Bounds().Width(), 124);
}

void
CKView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'ckSg':
		gSigma = int(msg->FindFloat("value") + 0.5);
		break;
	case 'ckTh':
		gThreshold = int(msg->FindFloat("value") + 0.5);
		break;
	case 'ckBl':
		gMode = CK_BLUE;
		break;
	case 'ckFG':
		gMode = CK_FG;
		break;
	case 'ckBG':
		gMode = CK_BG;
		break;
	default:
		inherited::MessageReceived(msg);
		return;
	}
	addon_preview();
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "ChromaKey");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 2002 ∑ Sum Software");
	strcpy(info->description, "Keys out a color to transparent");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 0;
	info->release = 1;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = PREVIEW_FULLSCALE;
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
	*view = new CKView(rect);
	return B_OK;
}

status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* /* frame */, bool final, BPoint /* point */, uint32 /* buttons */
)
{
	int error = ADDON_OK;
	BRect bounds = inLayer->Bounds();
	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer(*inLayer);
	if (mode == M_SELECT) {
		if (inSelection)
			*outSelection = new Selection(*inSelection);
		else // No Selection to blur!
			return (error);
	}
	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();
	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;
	uint8* mapbits = NULL;
	uint32 mbpr = 0;
	uint32 mdiff = 0;

	int threshold = gThreshold * 441 / 100;
	int sigma = gSigma;
	if (inSelection) {
		mapbits = (uint8*)inSelection->Bits() - 1;
		mbpr = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (final)
		addon_start();

	float delta = 100.0 / h; // For the Status Bar.

	rgb_color key;

	if (gMode == CK_FG)
		key = highcolor();
	else if (gMode == CK_BG)
		key = lowcolor();
	else {
		key.red = 0;
		key.green = 0;
		key.blue = 255;
	}

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* sbits = (uint32*)inLayer->Bits() - 1;
		bgra_pixel* dbits = (uint32*)(*outLayer)->Bits() - 1;

		for (uint32 y = 0; y < h; y++) {
			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}
			for (uint32 x = 0; x < w; x++) {
				grey_pixel mp = inSelection ? *(++mapbits) : 255;
				if (mp) {
					bgra_pixel p = *(++sbits);
					rgb_color pix = bgra2rgb(p);
					int d = int(diff(pix, key) + 0.5);
					if (d < threshold)
						*(++dbits) = p & COLOR_MASK;
					else if (d < threshold + sigma) {
						int alpha = 255 * (d - threshold) / sigma;
						*(++dbits) = (p & COLOR_MASK) | (pix.alpha * alpha << (ALPHA_BPOS - 8));
					}
					else
						*(++dbits) = p;
				}
				else
					*(++dbits) = *(++sbits);
			}
			mapbits += mdiff;
		}

		break;
	}
	case M_SELECT: {
		break;
	}
	default:
		fprintf(stderr, "ChromaKey: Unknown mode\n");
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
