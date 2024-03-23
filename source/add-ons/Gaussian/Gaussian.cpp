// © 1998-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <string.h>
#include <stdlib.h>

#define MAXGBLURSIZE 15
#define GBLURRAD 7 // int (MAXGBLURSIZE/2)

float matrix[MAXGBLURSIZE][MAXGBLURSIZE];
int gX, gY;
void
fillmatrix(int x, int y);

class BlurView : public BView
{
  public:
	BlurView(BRect rect);

	virtual ~BlurView() {}

	virtual void MessageReceived(BMessage* msg);

  private:
	typedef BView inherited;
};

BlurView::BlurView(BRect rect) : BView(rect, "blurview", B_FOLLOW_ALL, B_WILL_DRAW)
{
	gX = 5;
	gY = 5;
	ResizeTo(Bounds().Width(), 50);
	Slider* xSlid =
		new Slider(BRect(8, 8, 180, 24), 60, "x size", 1, MAXGBLURSIZE, 2, new BMessage('gblX'));
	Slider* ySlid =
		new Slider(BRect(8, 28, 180, 44), 60, "y size", 1, MAXGBLURSIZE, 2, new BMessage('gblY'));
	AddChild(xSlid);
	AddChild(ySlid);
	xSlid->SetValue(gX);
	ySlid->SetValue(gY);
}

void
BlurView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'gblX':
		gX = int(msg->FindFloat("value") + 0.5);
		fillmatrix(gX, gY);
		break;
	case 'gblY':
		gY = int(msg->FindFloat("value") + 0.5);
		fillmatrix(gX, gY);
		break;
	default:
		inherited::MessageReceived(msg);
		return;
	}
	addon_preview();
}

void
fillmatrix(int x, int y)
{
	float tmpmatrix[MAXGBLURSIZE][MAXGBLURSIZE];
	float sum = 0;

	for (int i = 0; i < MAXGBLURSIZE; i++)
		for (int j = 0; j < MAXGBLURSIZE; j++) {
			float rx = float(j - GBLURRAD) / (x / 2.0);
			float ry = float(i - GBLURRAD) / (y / 2.0);
			float rsq = (rx * rx + ry * ry);
			tmpmatrix[i][j] = exp(-rsq);
		}
	for (int i = -y / 2; i <= y / 2; i++)
		for (int j = -x / 2; j <= x / 2; j++)
			sum += tmpmatrix[i + GBLURRAD][j + GBLURRAD];

	for (int i = 0; i < MAXGBLURSIZE; i++)
		for (int j = 0; j < MAXGBLURSIZE; j++)
			matrix[i][j] = tmpmatrix[i][j] / sum;
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Gaussian Blur");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1998-2001 ∑ Sum Software");
	strcpy(info->description, "Applies a Gaussian blur");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 1;
	info->release = 2;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = PREVIEW_FULLSCALE;
	info->flags = 0;
	fillmatrix(5, 5);
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
	*view = new BlurView(rect);
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

	int sx = gX;
	int sy = gY;
	if (inSelection) {
		mapbits = (uint8*)inSelection->Bits() - 1;
		mbpr = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (final)
		addon_start();

	float delta = 100.0 / h; // For the Status Bar.

	//	for (int i = -sy/2; i <= sy/2; i++)
	//	{
	//		for (int j = -sx/2; j <= sx/2; j++)
	//		{
	//			float v = matrix[i + GBLURRAD][j + GBLURRAD];
	//			printf ("%3f ", w);
	//		}
	//		printf ("\n");
	//	}
	//	printf ("\n");

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* ibits = (bgra_pixel*)inLayer->Bits();
		uint32 ilpr = inLayer->BytesPerRow() / 4;

		// Allocate MAGBLURSIZE pixel rows with room for the "edges" (i.e. sx - 1 extra pixels)
		bgra_pixel* rows[MAXGBLURSIZE];
		for (int i = 0; i < MAXGBLURSIZE; i++)
			rows[i] = new bgra_pixel[w + sx - 1];

		bgra_pixel* sbits = ibits - 1;
		bgra_pixel* dbits = (bgra_pixel*)(*outLayer)->Bits() - 1;

		// Copy the source into the buffer, "folding" the edges around.
		for (int i = -sy / 2; i <= sy / 2; i++) {
			memcpy(rows[GBLURRAD + i] + sx / 2, ibits + abs(i) * ilpr, ilpr * 4);
			for (int j = -sx / 2; j < 0; j++) {
				rows[GBLURRAD + i][sx / 2 + j] = rows[GBLURRAD + i][sx / 2 - j];
				rows[GBLURRAD + i][w + sx / 2 - j - 1] = rows[GBLURRAD + i][w + sx / 2 + j - 1];
			}
		}

		for (uint32 y = 0; y < h; y++) {
			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}
			bgra_pixel* tmp = rows[0];
			for (int i = 1; i < MAXGBLURSIZE; i++)
				rows[i - 1] = rows[i];
			rows[MAXGBLURSIZE - 1] = tmp;
			if (y >= h - sy / 2 - 1 && y < h) {
				memcpy(
					rows[GBLURRAD + sy / 2] + sx / 2, ibits + (2 * (h - 1) - (y + sy / 2)) * ilpr,
					ilpr * 4
				);
				for (int j = -sx / 2; j < 0; j++) {
					rows[GBLURRAD + sy / 2][sx / 2 + j] = rows[GBLURRAD + sy / 2][sx / 2 - j];
					rows[GBLURRAD + sy / 2][w + sx / 2 - j - 1] =
						rows[GBLURRAD + sy / 2][w + sx / 2 + j - 1];
				}
			} else if (y < h) {
				memcpy(rows[GBLURRAD + sy / 2] + sx / 2, ibits + (y + sy / 2) * ilpr, ilpr * 4);
				for (int j = -sx / 2; j < 0; j++) {
					rows[GBLURRAD + sy / 2][sx / 2 + j] = rows[GBLURRAD + sy / 2][sx / 2 - j];
					rows[GBLURRAD + sy / 2][w + sx / 2 - j - 1] =
						rows[GBLURRAD + sy / 2][w + sx / 2 + j - 1];
				}
			}
			for (uint32 x = 0; x < w; x++) {
				if (!inSelection || *(++mapbits)) {
					float wr = 0;
					float wg = 0;
					float wb = 0;
					float wa = 0;
					sbits++;
					for (int i = -sy / 2; i <= sy / 2; i++) {
						bgra_pixel* b = rows[GBLURRAD + i] + x - 1;
						for (int j = -sx / 2; j <= sx / 2; j++) {
							bgra_pixel p = *(++b);
							float v = matrix[i + GBLURRAD][j + GBLURRAD];
							wb += v * BLUE(p);
							wg += v * GREEN(p);
							wr += v * RED(p);
							wa += v * ALPHA(p);
						}
					}
					*(++dbits) = PIXEL(wr, wg, wb, wa);
				} else
					*(++dbits) = *(++sbits);
			}
			mapbits += mdiff;
		}
		for (int i = 0; i < MAXGBLURSIZE; i++)
			delete[] rows[i];
		break;
	}
	case M_SELECT: {
		grey_pixel* ibits = (grey_pixel*)inSelection->Bits();
		uint32 ibpr = inSelection->BytesPerRow();
		uint32 odiff = (*outSelection)->BytesPerRow() - w;

		// Allocate MAGBLURSIZE pixel rows with room for the "edges" (i.e. sx - 1 extra pixels)
		grey_pixel* rows[MAXGBLURSIZE];
		for (int i = 0; i < MAXGBLURSIZE; i++)
			rows[i] = new grey_pixel[ibpr + sx - 1];

		grey_pixel* dbits = (grey_pixel*)(*outSelection)->Bits() - 1;

		// Copy the source into the buffer, "folding" the edges around.
		for (int i = -sy / 2; i <= sy / 2; i++) {
			memcpy(rows[GBLURRAD + i] + sx / 2, ibits + abs(i) * ibpr, ibpr);
			for (int j = -sx / 2; j < 0; j++) {
				rows[GBLURRAD + i][sx / 2 + j] = rows[GBLURRAD + i][sx / 2 - j];
				rows[GBLURRAD + i][w + sx / 2 - j - 1] = rows[GBLURRAD + i][w + sx / 2 + j - 1];
			}
		}

		for (uint32 y = 0; y < h; y++) {
			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}
			grey_pixel* tmp = rows[0];
			for (int i = 1; i < MAXGBLURSIZE; i++)
				rows[i - 1] = rows[i];
			rows[MAXGBLURSIZE - 1] = tmp;
			if (y >= h - sy / 2 - 1 && y < h) {
				memcpy(
					rows[GBLURRAD + sy / 2] + sx / 2, ibits + (2 * (h - 1) - (y + sy / 2)) * ibpr,
					ibpr
				);
				for (int j = -sx / 2; j < 0; j++) {
					rows[GBLURRAD + sy / 2][sx / 2 + j] = rows[GBLURRAD + sy / 2][sx / 2 - j];
					rows[GBLURRAD + sy / 2][w + sx / 2 - j - 1] =
						rows[GBLURRAD + sy / 2][w + sx / 2 + j - 1];
				}
			} else if (y < h) {
				memcpy(rows[GBLURRAD + sy / 2] + sx / 2, ibits + (y + sy / 2) * ibpr, ibpr);
				for (int j = -sx / 2; j < 0; j++) {
					rows[GBLURRAD + sy / 2][sx / 2 + j] = rows[GBLURRAD + sy / 2][sx / 2 - j];
					rows[GBLURRAD + sy / 2][w + sx / 2 - j - 1] =
						rows[GBLURRAD + sy / 2][w + sx / 2 + j - 1];
				}
			}
			for (uint32 x = 0; x < w; x++) {
				float wa = 0;
				for (int i = -sy / 2; i <= sy / 2; i++) {
					grey_pixel* b = rows[GBLURRAD + i] + x - 1;
					for (int j = -sx / 2; j <= sx / 2; j++) {
						grey_pixel p = *(++b);
						float v = matrix[i + GBLURRAD][j + GBLURRAD];
						wa += v * p;
					}
				}
				*(++dbits) = uchar(wa);
			}
			dbits += odiff;
		}
		for (int i = 0; i < MAXGBLURSIZE; i++)
			delete[] rows[i];
		break;
	}
	default:
		fprintf(stderr, "Blur: Unknown mode\n");
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
