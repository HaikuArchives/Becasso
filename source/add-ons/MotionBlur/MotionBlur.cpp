// © 1998-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <string.h>

#define MAXSIZE 50

float gSize;
float gAngle;

class MotionBlurView : public BView
{
  public:
	MotionBlurView(BRect rect) : BView(rect, "motion_blur_view", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		gSize = 0;
		gAngle = 0;
		ResizeTo(188, 48);
		Slider* sSlid =
			new Slider(BRect(8, 8, 180, 24), 60, "Distance", 2, MAXSIZE, 1, new BMessage('mblS'));
		Slider* aSlid =
			new Slider(BRect(8, 28, 180, 44), 60, "Angle", 0, 360, 1, new BMessage('mblA'));
		AddChild(sSlid);
		AddChild(aSlid);
		sSlid->SetValue(5);
		aSlid->SetValue(0);
	}

	virtual ~MotionBlurView(){};
	virtual void MessageReceived(BMessage* msg);
};

void
MotionBlurView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'mblS':
		gSize = msg->FindFloat("value");
		break;
	case 'mblA':
		gAngle = msg->FindFloat("value");
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
	strcpy(info->name, "Motion Blur");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1998-2001 ∑ Sum Software");
	strcpy(info->description, "Mimics the effect of moving the camera while still open.");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 1;
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
	*view = new MotionBlurView(rect);
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
		else // No Selection to blur!
			return (0);
	}
	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();
	int h = bounds.IntegerHeight() + 1;
	int w = bounds.IntegerWidth() + 1;
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
	float distance = gSize;
	float angle = gAngle * M_PI / 180.0;
	float cangle = cos(angle);
	float sangle = sin(angle);

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* sbits = (bgra_pixel*)inLayer->Bits();
		bgra_pixel* dbits = (bgra_pixel*)(*outLayer)->Bits() - 1;
		uint32 pprs = inLayer->BytesPerRow() / 4;

		/* Algorithm: Use the Bresenham line algorithm to `walk' along a line
		   with the given angle and length, summing pixel values, and replace
		   the original pixel with the average along that line. */

		int32 sum[4];
		bgra_pixel pixel;
		int x, y, i, xx, yy, n;
		int dx, dy, px, py, swapdir, e, s1, s2, err = 0;

		/* The actual work happens here */

		for (y = 0; y < h; y++) {
			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}
			for (x = 0; x < w; x++) {
				xx = x;
				yy = y;
				e = err;
				sum[0] = sum[1] = sum[2] = sum[3] = 0;
				if (!inSelection || *(++mapbits)) {
					n =
						int(distance * (inSelection ? *mapbits : 255) / 255
						); /* This effectively rounds it down */
					if (n > 1) {
						px = int(n * cangle);
						py = int(n * sangle);

						/* For the Bresenham algorithm (see Foley & Van Dam)
						   dx = |x2 - x1|, s1 = sgn (x2 - x1)
						   dy = |y2 - y1|, s2 = sgn (y2 - y1)  */

						if ((dx = px) != 0) {
							if (dx < 0) {
								dx = -dx;
								s1 = -1;
							} else
								s1 = 1;
						} else
							s1 = 0;

						if ((dy = py) != 0) {
							if (dy < 0) {
								dy = -dy;
								s2 = -1;
							} else
								s2 = 1;
						} else
							s2 = 0;

						if (dy > dx) {
							swapdir = dx;
							dx = dy;
							dy = swapdir;
							swapdir = 1;
						} else
							swapdir = 0;

						dy *= 2;
						err = dy - dx;
						dx *= 2;

						for (i = 0; i < n;) {
							pixel = (sbits + yy * pprs)[xx];
							sum[0] += RED(pixel);
							sum[1] += GREEN(pixel);
							sum[2] += BLUE(pixel);
							sum[3] += ALPHA(pixel);
							i++;

							while (e >= 0) {
								if (swapdir)
									xx += s1;
								else
									yy += s2;
								e -= dx;
							}
							if (swapdir)
								yy += s2;
							else
								xx += s1;
							e += dy;
							if ((xx < 0) || (xx >= w) || (yy < 0) || (yy >= h))
								break;
						}
					} else
						i = 0;
				} else
					i = 0;

				if (i == 0)
					*(++dbits) = (sbits + y * pprs)[x];
				else {
					sum[0] /= i;
					sum[1] /= i;
					sum[2] /= i;
					sum[3] /= i;
					*(++dbits) = PIXEL(sum[0], sum[1], sum[2], sum[3]);
				}
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

			/* Algorithm: Use the Bresenham line algorithm to `walk' along a line
			   with the given angle and length, summing pixel values, and replace
			   the original pixel with the average along that line. */

			int32 sum;
			grey_pixel pixel;
			int x, y, i, xx, yy, n;
			int dx, dy, px, py, swapdir, err, e, s1, s2;

			/* The actual work happens here */

			n = int(distance); /* This effectively rounds it down */
			px = int(n * cangle);
			py = int(n * sangle);

			/* For the Bresenham algorithm (see Foley & Van Dam)
			   dx = |x2 - x1|, s1 = sgn (x2 - x1)
			   dy = |y2 - y1|, s2 = sgn (y2 - y1)  */

			if ((dx = px) != 0) {
				if (dx < 0) {
					dx = -dx;
					s1 = -1;
				} else
					s1 = 1;
			} else
				s1 = 0;

			if ((dy = py) != 0) {
				if (dy < 0) {
					dy = -dy;
					s2 = -1;
				} else
					s2 = 1;
			} else
				s2 = 0;

			if (dy > dx) {
				swapdir = dx;
				dx = dy;
				dy = swapdir;
				swapdir = 1;
			} else
				swapdir = 0;

			dy *= 2;
			err = dy - dx;
			dx *= 2;

			for (y = 0; y < h; y++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}
				for (x = 0; x < w; x++) {
					xx = x;
					yy = y;
					e = err;
					sum = 0;
					if (n > 1) {

						for (i = 0; i < n;) {
							pixel = (sbits + yy * mbpr)[xx];
							sum += pixel;
							i++;

							while (e >= 0) {
								if (swapdir)
									xx += s1;
								else
									yy += s2;
								e -= dx;
							}
							if (swapdir)
								yy += s2;
							else
								xx += s1;
							e += dy;
							if ((xx < 0) || (xx >= w) || (yy < 0) || (yy >= h))
								break;
						}
					} else
						i = 0;

					if (i == 0)
						*(++dbits) = (sbits + y * mbpr)[x];
					else {
						sum /= i;
						*(++dbits) = sum;
					}
				}
				dbits += mdiff;
			}
		}
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
