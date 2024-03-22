// © 1998-2001 Sum Software

#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <Box.h>
#include <RadioButton.h>
#include <Window.h>
#include <stdlib.h>
#include <string.h>

#define N_ADDITIVE 1
#define N_MULTIPLICATIVE 2
#define MAXRANDSIZE 256
#define MAXAMOUNT 127

int gSigma;
int gType;

int xvec[MAXRANDSIZE];
void
fillvector(int x);
int
random(int max);

class NoiseView : public BView {
  public:
	NoiseView(BRect rect) : BView(rect, "noise_view", B_FOLLOW_NONE, B_WILL_DRAW)
	{
		gSigma = 5;
		gType = N_ADDITIVE;
		ResizeTo(188, 82);
		Slider* xSlid =
			new Slider(BRect(8, 62, 180, 78), 50, "Sigma", 1, MAXAMOUNT, 1, new BMessage('nSch'));
		AddChild(xSlid);
		BBox* type = new BBox(BRect(4, 4, 184, 58), "type");
		type->SetLabel("Type");
		AddChild(type);
		BRadioButton* tAdd =
			new BRadioButton(BRect(8, 13, 164, 30), "add", "Additive", new BMessage('nTad'));
		BRadioButton* tMul =
			new BRadioButton(BRect(8, 30, 164, 47), "mul", "Multiplicative", new BMessage('nTmp'));
		type->AddChild(tAdd);
		type->AddChild(tMul);
		tAdd->SetValue(true);
		xSlid->SetValue(5);
		fillvector(5);
	}

	virtual ~NoiseView() {}

	virtual void MessageReceived(BMessage* msg);
};

void
NoiseView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'nSch':
		gSigma = int(msg->FindFloat("value") + 0.5);
		fillvector(gSigma);
		break;
	case 'nTad':
		gType = N_ADDITIVE;
		break;
	case 'nTmp':
		gType = N_MULTIPLICATIVE;
		break;
	default:
		BView::MessageReceived(msg);
		return;
	}
	addon_preview();
}

void
fillvector(int x)
{
	for (int i = 0; i < MAXRANDSIZE; i++) {
		float r = float(i) / MAXRANDSIZE * 4;
		xvec[i] = int(x * exp(-r) * ((i & 1) ? 1 : -1));
	}
}

int
random(int m)
{
	return (int((double)(rand()) * m / RAND_MAX));
	// A bit convoluted, but this is necessary on GCC because RAND_MAX is MAX_INT.
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Noise");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1999-2001 ∑ Sum Software");
	strcpy(info->description, "Adds noise to the image");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 1;
	info->release = 2;
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
	*view = new NoiseView(rect);
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
	//	printf ("Bounds: ");
	//	bounds.PrintToStream();
	//	printf ("Frame:  ");
	//	frame->PrintToStream();

	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer(*inLayer);
	if (mode == M_SELECT) {
		if (inSelection)
			*outSelection = new Selection(*inSelection);
		else // No Selection to diffuse!
			return (0);
	}
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
		mapbits = (grey_pixel*)inSelection->Bits();
		mbpr = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (final)
		addon_start();
	float delta = 100.0 / h; // For the Status Bar.

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* sbits = (bgra_pixel*)inLayer->Bits() - 1;
		bgra_pixel* dbits = (bgra_pixel*)(*outLayer)->Bits() - 1;

		switch (gType) {
		case N_ADDITIVE: {
			for (uint32 y = 0; y < h; y++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}
				for (uint32 x = 0; x < w; x++) {
					if (!inSelection) {
						rgb_color p = bgra2rgb(*(++sbits));
						char amount = xvec[random(MAXRANDSIZE)];
						p.red = clipchar(p.red + amount);
						p.green = clipchar(p.green + amount);
						p.blue = clipchar(p.blue + amount);
						*(++dbits) = rgb2bgra(p);
					}
					else if (*(++mapbits)) {
						rgb_color p = bgra2rgb(*(++sbits));
						char amount = xvec[random(MAXRANDSIZE)] * *mapbits / 255;
						p.red = clipchar(p.red + amount);
						p.green = clipchar(p.green + amount);
						p.blue = clipchar(p.blue + amount);
						*(++dbits) = rgb2bgra(p);
					}
					else
						*(++dbits) = *(++sbits);
				}
				mapbits += mdiff;
			}
			break;
		}
		case N_MULTIPLICATIVE: {
			for (uint32 y = 0; y < h; y++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}
				for (uint32 x = 0; x < w; x++) {
					if (!inSelection) {
						rgb_color p = bgra2rgb(*(++sbits));
						char amount = xvec[random(MAXRANDSIZE)];
						int factor = 255 + amount;
						p.red = clipchar(p.red * factor / 255);
						p.green = clipchar(p.green * factor / 255);
						p.blue = clipchar(p.blue * factor / 255);
						*(++dbits) = rgb2bgra(p);
					}
					else if (*(++mapbits)) {
						rgb_color p = bgra2rgb(*(++sbits));
						char amount = xvec[random(MAXRANDSIZE)] * *mapbits / 255;
						int factor = 255 + amount;
						p.red = clipchar(p.red * factor / 255);
						p.green = clipchar(p.green * factor / 255);
						p.blue = clipchar(p.blue * factor / 255);
						*(++dbits) = rgb2bgra(p);
					}
					else
						*(++dbits) = *(++sbits);
				}
				mapbits += mdiff;
			}
			break;
		}
		default:
			fprintf(stderr, "Noise: Unknown type\n");
		}
		break;
	}
	case M_SELECT: {
		if (!inSelection)
			*outSelection = NULL;
		else {
			grey_pixel* sbits = mapbits;
			grey_pixel* dbits = (grey_pixel*)(*outSelection)->Bits();

			switch (gType) {
			case N_ADDITIVE: {
				for (uint32 y = 0; y < h; y++) {
					if (final) {
						addon_update_statusbar(delta);
						if (addon_stop()) {
							error = ADDON_ABORT;
							break;
						}
					}
					grey_pixel* srcline = sbits + y * mbpr - 1;
					grey_pixel* destline = dbits + y * mbpr - 1;
					for (uint32 x = 0; x < w; x++) {
						*(++destline) = clipchar(*(++srcline) + xvec[random(MAXRANDSIZE)]);
					}
				}
				break;
			}
			case N_MULTIPLICATIVE: {
				for (uint32 y = 0; y < h; y++) {
					if (final) {
						addon_update_statusbar(delta);
						if (addon_stop()) {
							error = ADDON_ABORT;
							break;
						}
					}
					grey_pixel* srcline = sbits + y * mbpr - 1;
					grey_pixel* destline = dbits + y * mbpr - 1;
					for (uint32 x = 0; x < w; x++) {
						*(++destline) =
							clipchar(*(++srcline) * (255 + xvec[random(MAXRANDSIZE)]) / 255);
					}
				}
				break;
			}
			default:
				fprintf(stderr, "Noise: Unknown type\n");
			}
			break;
		}
		break;
	}
	default:
		fprintf(stderr, "Noise: Unknown mode\n");
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
