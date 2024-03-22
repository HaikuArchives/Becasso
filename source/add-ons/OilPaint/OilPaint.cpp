// © 1998-2001 Sum Software

#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <string.h>

// Simulate an oil painting by replacing each pixel with the most frequently
// appearing color in an n x n grid around it.

// NB: This is a very straightforward implementation, and NOT very efficient!

class cn_list {
  public:
	cn_list(int _max) : fMax(_max), fIndex(0), fMf(0)
	{
		clist = new bgra_pixel[fMax];
		nlist = new int[fMax];
	};

	virtual ~cn_list()
	{
		delete[] clist;
		delete[] nlist;
	};

	int add(bgra_pixel c);

	void reset()
	{
		fIndex = 0;
		fMf = 0;
	}

	bgra_pixel most_frequent();

  private:
	bgra_pixel* clist;
	int* nlist;
	int fMax;
	int fIndex;
	int fMf;
};

int
cn_list::add(bgra_pixel c)
{
	for (int i = 0; i < fIndex; i++) {
		if (clist[i] == c) {
			if (++nlist[i] > nlist[fMf])
				fMf = i;
			return (i);
		}
	}
	if (fIndex == fMax) {
		fprintf(stderr, "cn_list:  Too many items\n");
		return (-1);
	}
	clist[fIndex] = c;
	nlist[fIndex++] = 1;
	return (fIndex - 1);
}

bgra_pixel
cn_list::most_frequent()
{
	if (nlist[fMf] == 1) // Apparently, only different colors.  Return the original middle pixel.
		return (clist[fIndex / 2]);

	return clist[fMf];
}

int gSize;

class OilView : public BView {
  public:
	OilView(BRect rect) : BView(rect, "oil_view", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		gSize = 5;
		ResizeTo(188, 28);
		Slider* sSlid =
			new Slider(BRect(8, 8, 180, 24), 64, "Mask Size", 3, 15, 2, new BMessage('oilS'));
		sSlid->SetValue(5);
		AddChild(sSlid);
	}

	virtual ~OilView(){};
	virtual void MessageReceived(BMessage* msg);
};

void
OilView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'oilS':
		gSize = int(msg->FindFloat("value") + 0.5);
		break;
	default:
		BView::MessageReceived(msg);
		return;
	}
	addon_preview();
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "OilPaint");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1998-2001 ∑ Sum Software");
	strcpy(info->description, "Simulates an oil painting effect");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 0;
	info->release = 7;
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
	*view = new OilView(rect);
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
	if (*outLayer == NULL && mode == M_DRAW) {
		*outLayer = new Layer(*inLayer);
		// printf ("Allocated new outLayer\n");
	}
	if (mode == M_SELECT) {
		if (inSelection)
			*outSelection = new Selection(*inSelection);
		else // No Selection to filter!
			return (0);
	}
	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();
	int32 h = bounds.IntegerHeight() + 1;
	int32 w = bounds.IntegerWidth() + 1;
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

	switch (mode) {
	case M_DRAW: {
		uint32 slpr = inLayer->BytesPerRow() / 4;
		uint32 dlpr = (*outLayer)->BytesPerRow() / 4;
		bgra_pixel* sbits = (bgra_pixel*)inLayer->Bits() - 1;
		int size = gSize;
		int hsize = size / 2; // Rounded down.  Size is always odd.
		cn_list colors(size * size);

		//		printf ("size = %i, hsize = %i\n", size, hsize);
		//		printf ("slpr = %i, dlpr = %i, h = %i, w = %i\n", slpr, (*outLayer)->BytesPerRow(),
		//h, w);
		for (int32 y = 0; y < h; y++) {
			bgra_pixel* dbits = (bgra_pixel*)(*outLayer)->Bits() + y * dlpr - 1;
			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}
			for (int32 x = 0; x < w; x++) {
				if (!inSelection || *(++mapbits)) {
					int f = 255;
					if (inSelection)
						f = *mapbits;

					int32 cl = x - hsize * f / 255;
					if (cl < 0)
						cl = 0;
					int32 cr = x + hsize * f / 255;
					if (cr > w - 1)
						cr = w - 1;
					int32 cu = y - hsize * f / 255;
					if (cu < 0)
						cu = 0;
					int32 cb = y + hsize * f / 255;
					if (cb > h - 1)
						cb = h - 1;

					// printf ("(%i, %i) ", y, x); fflush (stdout);
					// printf ("LT (%i, %i), RB: (%i, %i)\n", cu, cl, cb, cr);
					colors.reset();
					for (int32 i = cu; i <= cb; i++) {
						bgra_pixel* s = sbits + i * slpr + cl;
						for (int32 j = cl; j <= cr; j++)
							colors.add(*(++s));
					}
					// printf ("(%ld, %ld) -> %p\n", x, y, colors.most_frequent());
					*(++dbits) = colors.most_frequent();
				}
				else
					*(++dbits) = *(sbits + y * slpr + x);
			}
			if (inSelection)
				mapbits += mdiff;
		}
		break;
	}
	case M_SELECT: {
		if (!inSelection)
			*outSelection = NULL;
		else {
		}
		break;
	}
	default:
		fprintf(stderr, "OilPaint: Unknown mode\n");
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
