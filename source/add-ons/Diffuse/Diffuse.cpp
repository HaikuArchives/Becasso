// © 1998-2001 Sum Software

#include "BecassoAddOn.h"
#include "AddOnWindow.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <stdlib.h>
#include <string.h>

#define MAXGBLURSIZE 15
#define MAXRANDSIZE 256

// static uint32 rndval = 0;

int gX;
int gY;
int xvec[MAXRANDSIZE], yvec[MAXRANDSIZE];

void
fillvectors(int x, int y)
{
	for (int i = 0; i < MAXRANDSIZE; i++) {
		float r = float(i) / MAXRANDSIZE * 4;
		xvec[i] = int(x * exp(-r) * ((i & 1) ? 1 : -1));
		yvec[i] = int(y * exp(-r) * ((i & 1) ? 1 : -1));
	}
}

int
random(int max)
{
	//	rndval = 1664525*rndval + 1013904223;
	return (int)((((double)rand()) / RAND_MAX) * max);
	//	return (int)(((double) rndval)/RAND_MAX)*max/2;
	// A bit convoluted, but this is necessary on GCC because RAND_MAX is MAX_INT.
}

class DiffuseView : public BView {
  public:
	DiffuseView(BRect rect) : BView(rect, "diffuse_view", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		gX = 5;
		gY = 5;
		ResizeTo(188, 48);
		Slider* xSlid = new Slider(
			BRect(8, 8, 180, 24), 60, "x size", 1, MAXGBLURSIZE, 2, new BMessage('gblX')
		);
		Slider* ySlid = new Slider(
			BRect(8, 28, 180, 44), 60, "y size", 1, MAXGBLURSIZE, 2, new BMessage('gblY')
		);
		AddChild(xSlid);
		AddChild(ySlid);
		xSlid->SetValue(5);
		ySlid->SetValue(5);
		fillvectors(5, 5);
	}

	virtual ~DiffuseView() {}

	virtual void MessageReceived(BMessage* msg)
	{
		switch (msg->what) {
		case 'gblX':
			gX = int(msg->FindFloat("value") + 0.5);
			fillvectors(gX, gY);
			break;
		case 'gblY':
			gY = int(msg->FindFloat("value") + 0.5);
			fillvectors(gX, gY);
			break;
		default:
			BView::MessageReceived(msg);
			return;
		}
		addon_preview();
	}
};

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Diffuse");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1998-2001 ∑ Sum Software");
	strcpy(info->description, "Spreads the pixels randomly");
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
	*view = new DiffuseView(rect);
	return B_OK;
}

status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* /* frame */, bool final, BPoint /* point */, uint32 /* buttons */
)
{
	srand(0);
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
		// printf ("inSelection\n");
		mapbits = (grey_pixel*)inSelection->Bits();
		mbpr = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (final)
		addon_start();

	float delta = 100.0 / h; // For the Status Bar.

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
				if (!inSelection || *(++mapbits)) {
					int xspread = xvec[random(MAXRANDSIZE)];
					int yspread = yvec[random(MAXRANDSIZE)];
					if (x + xspread < 0 || x + xspread >= w)
						xspread = -xspread;

					if (y + yspread < 0 || y + yspread >= h)
						yspread = -yspread;

					// printf ("(%d, %d): (%d, %d)\n", x, y, xspread, yspread);
					bgra_pixel* p = ++sbits + yspread * w + xspread;
					*(++dbits) = *p;
				}
				else
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
			grey_pixel* dbits = (grey_pixel*)(*outSelection)->Bits() - 1;
			int ddiff = (*outSelection)->BytesPerRow() - w;
			grey_pixel* sbits = mapbits - 1;
			// printf ("ddiff = %d, mdiff = %d\n", ddiff, mdiff);
			for (uint32 y = 0; y < h; y++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}
				for (uint32 x = 0; x < w; x++) {
					int xspread = xvec[random(MAXRANDSIZE)];
					int yspread = yvec[random(MAXRANDSIZE)];
					if (x + xspread < 0 || x + xspread >= w)
						xspread = -xspread;

					if (y + yspread < 0 || y + yspread >= h)
						yspread = -yspread;

					// printf ("(%d, %d): (%d, %d)\n", x, y, xspread, yspread);
					grey_pixel* p = ++sbits + yspread * mbpr + xspread;
					*(++dbits) = *p;
				}
				sbits += mdiff;
				dbits += ddiff;
			}
		}
		break;
	}
	default:
		fprintf(stderr, "Diffuse: Unknown mode\n");
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
