// © 2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include <string.h>
#include <View.h>
#include "Slider.h"

float gBrightness = 0;
float gContrast = 0;

class BCView : public BView
{
  public:
	BCView(BRect rect);

	virtual ~BCView() {}

	virtual void MessageReceived(BMessage* msg);

  private:
	typedef BView inherited;
};

BCView::BCView(BRect rect) : BView(rect, "brightness/contrast", B_FOLLOW_ALL, B_WILL_DRAW)
{
	gBrightness = 0;
	gContrast = 0;
	ResizeTo(Bounds().Width(), 50);
	Slider* bSlid =
		new Slider(BRect(8, 8, 180, 24), 70, "Brightness", -100, 100, 1, new BMessage('gblB'));
	Slider* cSlid =
		new Slider(BRect(8, 28, 180, 44), 70, "Contrast", -100, 100, 1, new BMessage('gblC'));
	AddChild(bSlid);
	AddChild(cSlid);
	bSlid->SetValue(gBrightness);
	cSlid->SetValue(gContrast);
}

void
BCView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'gblB':
		gBrightness = msg->FindFloat("value");
		break;
	case 'gblC':
		gContrast = msg->FindFloat("value");
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
	strcpy(info->name, "Brightness/Contrast");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 2001 ∑ Sum Software");
	strcpy(info->description, "Change brightness and contrast of an image");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 0;
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
	*view = new BCView(rect);
	return B_OK;
}

status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* /*frame*/, bool final, BPoint /* point */, uint32 /* buttons */
)
{
	status_t error = ADDON_OK;
	BRect bounds = inLayer->Bounds();
	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer(*inLayer);
	if (mode == M_SELECT) {
		if (inSelection)
			*outSelection = new Selection(*inSelection);
		else // No Selection!
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
		mapbits = (grey_pixel*)inSelection->Bits() - 1;
		mbpr = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (final)
		addon_start();

	float delta = 100.0 / h; // For the Status Bar.

	int brightness = int(gBrightness * 255 / 100 + 0.5);
	float contrast = (gContrast + 100.0) / 100;

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* sbits = (bgra_pixel*)inLayer->Bits() - 1;
		bgra_pixel* dbits = (bgra_pixel*)(*outLayer)->Bits() - 1;

		for (uint32 y = 0; y < h; y++) {
			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}
			for (uint32 x = 0; x < w; x++) {
				int map = 255;
				if (!inSelection || (map = *(++mapbits))) {
					bgra_pixel pixel = *(++sbits);
					bgra_pixel newpix = PIXEL(
						clipchar(brightness + contrast * RED(pixel)),
						clipchar(brightness + contrast * GREEN(pixel)),
						clipchar(brightness + contrast * BLUE(pixel)), ALPHA(pixel)
					);
					*(++dbits) = weighted_average(newpix, map, pixel, 255 - map);
				} else
					*(++dbits) = *(++sbits);
			}
			mapbits += mdiff;
		}
		break;
	}
	case M_SELECT: {
		if (!inSelection)
			*outSelection = NULL;
		else {
			grey_pixel* sbits = mapbits;
			grey_pixel* dbits = (grey_pixel*)(*outSelection)->Bits() - 1;
			for (uint32 y = 0; y < h; y++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}
				for (uint32 x = 0; x < w; x++) {
					grey_pixel pixel = *(++sbits);
					*(++dbits) = clipchar(brightness + contrast * pixel);
				}
				sbits += mdiff;
				dbits += mdiff;
			}
		}
		break;
	}
	default:
		fprintf(stderr, "Brightness: Unknown mode\n");
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
