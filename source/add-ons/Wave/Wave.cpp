// © 1998-2001 Sum Software

#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <Box.h>
#include <RadioButton.h>
#include <string.h>

#define HORIZONTAL 0
#define VERTICAL 1

#define FOREGROUND 0
#define BACKGROUND 1
#define TRANSPARENT 2
#define IMAGECOLOR 3

float gPeriods;
float gAmplitude;
float gPhase;
int gDirection;
int gColor;

class WaveView : public BView
{
  public:
	WaveView(BRect rect) : BView(rect, "wave view", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		gPeriods = 1;
		gAmplitude = 0.05;
		gPhase = 0;
		gDirection = 0;
		gColor = BACKGROUND;
		ResizeTo(188, 232);
		Slider* aSlid = new Slider(
			BRect(8, 8, 180, 24), 60, "Amplitude", 0, 1, 0.01, new BMessage('wavA'), B_HORIZONTAL,
			30, "%3.2f"
		);
		Slider* pSlid = new Slider(
			BRect(8, 28, 180, 44), 60, "Periods", 0.25, 20, 0.25, new BMessage('wavP'),
			B_HORIZONTAL, 30, "%4.2f"
		);
		Slider* fSlid =
			new Slider(BRect(8, 48, 180, 64), 60, "Phase", 0, 360, 1, new BMessage('wavF'));
		AddChild(aSlid);
		AddChild(pSlid);
		AddChild(fSlid);
		aSlid->SetValue(0.05);
		pSlid->SetValue(1);
		fSlid->SetValue(0);
		BBox* dir = new BBox(BRect(4, 70, 184, 126), "direction");
		dir->SetLabel("Direction");
		AddChild(dir);
		BRadioButton* dH =
			new BRadioButton(BRect(8, 13, 164, 30), "hor", "Horizontal", new BMessage('dirH'));
		BRadioButton* dV =
			new BRadioButton(BRect(8, 30, 164, 47), "ver", "Vertical", new BMessage('dirV'));
		dir->AddChild(dH);
		dir->AddChild(dV);
		dH->SetValue(true);
		BBox* col = new BBox(BRect(4, 136, 184, 228), "color");
		col->SetLabel("Exposed Pixels");
		AddChild(col);
		BRadioButton* cF =
			new BRadioButton(BRect(8, 13, 164, 30), "hi", "Foreground Color", new BMessage('colF'));
		BRadioButton* cB =
			new BRadioButton(BRect(8, 30, 164, 47), "lo", "Background Color", new BMessage('colB'));
		BRadioButton* cT =
			new BRadioButton(BRect(8, 47, 164, 64), "tr", "Transparent", new BMessage('colT'));
		BRadioButton* cI =
			new BRadioButton(BRect(8, 64, 164, 88), "im", "Original Image", new BMessage('colI'));
		col->AddChild(cF);
		col->AddChild(cB);
		col->AddChild(cT);
		col->AddChild(cI);
		cB->SetValue(true);
	}

	virtual ~WaveView() {}

	virtual void MessageReceived(BMessage* msg);
};

void
WaveView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'wavP':
		gPeriods = msg->FindFloat("value");
		break;
	case 'wavA':
		gAmplitude = msg->FindFloat("value");
		break;
	case 'wavF':
		gPhase = msg->FindFloat("value");
		break;
	case 'dirH':
		gDirection = HORIZONTAL;
		break;
	case 'dirV':
		gDirection = VERTICAL;
		break;
	case 'colF':
		gColor = FOREGROUND;
		break;
	case 'colB':
		gColor = BACKGROUND;
		break;
	case 'colI':
		gColor = IMAGECOLOR;
		break;
	case 'colT':
		gColor = TRANSPARENT;
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
	strcpy(info->name, "Wave");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1998-2001 ∑ Sum Software");
	strcpy(info->description, "Applies a sinusoid wave effect");
	info->type = BECASSO_TRANSFORMER;
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
	*view = new WaveView(rect);
	return B_OK;
}

status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* frame, bool final, BPoint /* point */, uint32 /* buttons */
)
{
	int error = ADDON_OK;
	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer(*inLayer);
	if (mode == M_SELECT) {
		if (inSelection)
			*outSelection = new Selection(*inSelection);
		else // No Selection to wave!
			return (error);
	}
	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();

	if (mode == M_SELECT) {
		*frame = inSelection->Bounds();
	}

	int firstRow = int(frame->top);
	int lastRow = int(frame->bottom);
	int firstCol = int(frame->left);
	int lastCol = int(frame->right);

	int w = inLayer->Bounds().IntegerWidth() + 1;
	int h = inLayer->Bounds().IntegerHeight() + 1;

	if (final)
		addon_start();

	int direction = gDirection;
	float amplitude = gAmplitude;
	float phase = gPhase * M_PI / 180.0;
	float periods = gPeriods;

	int row, col;
	rgb_color c;
	if (gColor == BACKGROUND)
		c = lowcolor();
	else if (gColor == FOREGROUND)
		c = highcolor();
	else
		c.red = c.green = c.blue = c.alpha = 0;

	bool orig = (gColor == IMAGECOLOR);

	bgra_pixel bg = PIXEL(c.red, c.green, c.blue, c.alpha);

	switch (mode) {
	case M_DRAW: {
		int pprs = inLayer->BytesPerRow() / 4;
		int pprd = (*outLayer)->BytesPerRow() / 4;
		bgra_pixel* s = (bgra_pixel*)inLayer->Bits() - 1;
		bgra_pixel* d = (bgra_pixel*)(*outLayer)->Bits() - 1;

		if (direction == 0) /* Horizontal */
		{
			float delta = 100.0 / (lastRow - firstRow); // For the Status Bar.
			float iamp = amplitude * (lastCol - firstCol);
			float pfac = periods / (lastRow - firstRow) * M_PI * 2;

			for (row = 0; row < firstRow; row++)
				memcpy(d + 1 + row * pprd, s + 1 + row * pprs, pprs * 4);

			for (row = firstRow; row <= lastRow; row++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
					float foffset = iamp * sin((row - firstRow) * pfac + phase);
					bgra_pixel* sl = s + row * pprs + 1;
					bgra_pixel* dl = d + row * pprd + 1;
					int offset = int(foffset);
					uint8 ofac = uint8(255 * fabs(foffset - offset));
					if (foffset >= 0) // Shift right
					{
						for (col = 0; col < firstCol; col++)
							*dl++ = *sl++;

						if (orig) {
							for (col = firstCol; col < firstCol + offset; col++)
								*dl++ = *sl++;
						} else {
							for (col = firstCol; col < firstCol + offset; col++)
								*dl++ = bg;
						}
						bgra_pixel a, b;
						if (orig) {
							a = *sl;
							sl -= offset;
						} else
							a = bg;
						b = *sl;
						for (col = firstCol + offset; col <= lastCol; col++) {
							*dl++ = weighted_average(a, ofac, b, 255 - ofac);
							a = b;
							b = *sl++;
						}
						sl += offset;
						for (col = lastCol + 1; col < w; col++)
							*dl++ = *sl++;
					} else // if (0)	// Shift left
					{
						for (col = 0; col < firstCol; col++)
							*dl++ = *sl++;

						sl -= offset;
						bgra_pixel a, b;
						a = *sl++;
						b = a;
						for (col = firstCol; col < lastCol + offset; col++) {
							*dl++ = weighted_average(b, ofac, a, 255 - ofac);
							a = b;
							b = *sl++;
						}
						if (orig) {
							sl += offset;
							*dl++ = weighted_average(*sl, ofac, b, 255 - ofac);
							for (col = lastCol + offset; col < lastCol; col++)
								*dl++ = *sl++;
							//							sl += offset;
						} else {
							*dl++ = weighted_average(bg, ofac, a, 255 - ofac);
							for (col = lastCol + offset; col < lastCol; col++) {
								*dl++ = bg;
							}
						}
						for (col = lastCol; col < w; col++)
							*dl++ = *sl++;
					}
					//					else	// zero crossing
					//					{
					//						for (col = 0; col < w; col++)
					//							*(++dl) = *(++sl);
					//					}
				} else // !final
				{
					int32 offset = int32(iamp * sin((row - firstRow) * pfac + phase));
					bgra_pixel* sl = s + row * pprs;
					bgra_pixel* dl = d + row * pprd;
					if (offset > 0) // Shift right
					{
						for (col = 0; col < firstCol; col++)
							*(++dl) = *(++sl);
						if (orig) {
							for (col = firstCol; col < firstCol + offset; col++) {
								*(++dl) = *(++sl);
							}
							sl -= offset;
						} else {
							for (col = firstCol; col < firstCol + offset; col++) {
								*(++dl) = bg;
							}
						}
						/* Following breaks when firstCol > offset, actually... */
						/* If this can happen, do something smart with max_c. */
						for (col = firstCol + offset; col < lastCol; col++) {
							*(++dl) = *(++sl);
						}
						sl += offset;
						for (col = lastCol; col < w; col++) {
							*(++dl) = *(++sl);
						}
					} else // Shift left
					{
						for (col = 0; col < firstCol; col++)
							*(++dl) = *(++sl);

						sl -= offset;

						for (col = firstCol; col < lastCol + offset; col++) {
							*(++dl) = *(++sl);
						}
						if (orig) {
							sl += offset;
							for (col = lastCol + offset; col <= lastCol; col++) {
								*(++dl) = *(++sl);
							}
							// sl += offset;
						} else {
							for (col = lastCol + offset; col < lastCol; col++) {
								*(++dl) = bg;
							}
						}
						for (col = lastCol + offset; col < w; col++) {
							*(++dl) = *(++sl);
						}
					}
				}
			}
			for (row = lastRow + 1; row < h; row++)
				memcpy(d + 1 + row * pprd, s + 1 + row * pprs, pprs * 4);
		} else /* Vertical */
		{
			float delta = 100.0 / (lastCol - firstCol); // For the Status Bar.
			float iamp = amplitude * (lastRow - firstRow);
			float pfac = periods / (lastCol - firstCol) * M_PI * 2;

			for (col = 0; col < firstCol; col++) {
				bgra_pixel* sl = s + col + 1;
				bgra_pixel* dl = d + col + 1;
				for (row = 0; row < h; row++) {
					*dl = *sl;
					dl += pprd;
					sl += pprs;
				}
			}

			for (col = firstCol; col <= lastCol + 1; col++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
					float foffset = -iamp * sin((col - firstCol) * pfac + phase);
					bgra_pixel* sl = s + col + 1;
					bgra_pixel* dl = d + col + 1;
					int offset = int(foffset);
					uint8 ofac = uint8(255 * (foffset - offset));
					if (foffset > 0) // Shift down
					{
						for (row = 0; row < firstRow; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
						if (orig) {
							for (row = firstRow; row < firstRow + offset; row++) {
								*dl = *sl;
								dl += pprd;
								sl += pprs;
							}
							sl -= offset * pprs;
						} else {
							for (row = firstRow; row < firstRow + offset; row++) {
								*dl = bg;
								dl += pprd;
							}
						}
						bgra_pixel a, b;
						if (orig) {
							a = *sl;
							// sl -= pprs;
						} else
							a = bg;
						b = *sl;
						for (row = firstRow + offset; row <= lastRow; row++) {
							*dl = weighted_average(a, ofac, b, 255 - ofac);
							a = b;
							b = *sl;
							dl += pprd;
							sl += pprs;
						}
						sl += offset * pprs;
						for (row = lastRow + 1; row < h; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
					} else // if (foffset < 0)	// Shift up
					{
						for (row = 0; row < firstRow; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
						sl -= offset * pprs;
						bgra_pixel a, b;
						a = *sl;
						sl += pprs;
						b = a;
						for (row = firstRow; row < lastRow + offset; row++) {
							*dl = weighted_average(a, ofac, b, 255 - ofac);
							a = b;
							b = *sl;
							dl += pprd;
							sl += pprs;
						}
						if (orig) {
							sl += offset * pprs;
							*dl = weighted_average(*sl, ofac, b, 255 - ofac);
							dl += pprd;
							for (row = lastRow + offset + 1; row <= lastRow; row++) {
								*dl = *sl;
								dl += pprd;
								sl += pprs;
							}
						} else {
							*dl = weighted_average(a, ofac, bg, 255 - ofac);
							dl += pprd;
							for (row = lastRow + offset + 1; row <= lastRow; row++) {
								*dl = bg;
								dl += pprd;
							}
						}
						for (row = lastRow + 1; row < h; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
					}
				} else // !final
				{
					int32 offset = int32(-iamp * sin((col - firstCol) * pfac + phase));
					bgra_pixel* sl = s + col + 1;
					bgra_pixel* dl = d + col + 1;
					if (offset > 0) // Shift down
					{
						for (row = 0; row < firstRow; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
						if (orig) {
							for (row = firstRow; row < firstRow + offset; row++) {
								*dl = *sl;
								dl += pprd;
								sl += pprs;
							}
							sl -= offset * pprs;
						} else {
							for (row = firstRow; row < firstRow + offset; row++) {
								*dl = bg;
								dl += pprd;
							}
						}
						for (row = firstRow + offset; row <= lastRow; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
						sl += offset * pprs;
						for (row = lastRow + 1; row < h; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
					} else // Shift up
					{
						for (row = 0; row < firstRow; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
						sl -= offset * pprs;
						for (row = firstRow; row <= lastRow + offset; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
						if (orig) {
							sl += offset * pprs;
							for (row = lastRow + offset + 1; row <= lastRow; row++) {
								*dl = *sl;
								dl += pprd;
								sl += pprs;
							}
						} else {
							for (row = lastRow + offset + 1; row <= lastRow; row++) {
								*dl = bg;
								dl += pprd;
							}
						}
						for (row = lastRow + 1; row < h; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
					}
				}
			}

			for (col = lastCol + 1; col < w; col++) {
				bgra_pixel* sl = s + col + 1;
				bgra_pixel* dl = d + col + 1;
				for (row = 0; row < h; row++) {
					*dl = *sl;
					dl += pprd;
					sl += pprs;
				}
			}
		}
		break;
	}
	case M_SELECT: {
		int pprs = inSelection->BytesPerRow();
		int pprd = (*outSelection)->BytesPerRow();
		grey_pixel* s = (grey_pixel*)inSelection->Bits() - 1;
		grey_pixel* d = (grey_pixel*)(*outSelection)->Bits() - 1;

		if (direction == 0) /* Horizontal */
		{
			float delta = 100.0 / (lastRow - firstRow); // For the Status Bar.
			float iamp = amplitude * (lastCol - firstCol);
			float pfac = periods / (lastRow - firstRow) * M_PI * 2;

			for (row = 0; row < firstRow; row++)
				memcpy(d + 1 + row * pprd, s + 1 + row * pprs, pprs);

			for (row = firstRow; row <= lastRow; row++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}

				int32 offset = int32(iamp * sin((row - firstRow) * pfac + phase));
				grey_pixel* sl = s + row * pprs;
				grey_pixel* dl = d + row * pprd;
				if (offset > 0) // Shift right
				{
					for (col = 0; col < firstCol; col++)
						*(++dl) = *(++sl);
					if (orig) {
						for (col = firstCol; col < firstCol + offset; col++) {
							*(++dl) = *(++sl);
						}
						sl -= offset;
					} else {
						for (col = firstCol; col < firstCol + offset; col++) {
							*(++dl) = bg;
						}
					}
					/* Following breaks when firstCol > offset, actually... */
					/* If this can happen, do something smart with max_c. */
					for (col = firstCol + offset; col < lastCol; col++) {
						*(++dl) = *(++sl);
					}
					sl += offset;
					for (col = lastCol; col < w; col++) {
						*(++dl) = *(++sl);
					}
				} else // Shift left
				{
					for (col = 0; col < firstCol; col++)
						*(++dl) = *(++sl);

					sl -= offset;

					for (col = firstCol; col < lastCol + offset; col++) {
						*(++dl) = *(++sl);
					}
					if (orig) {
						sl += offset;
						for (col = lastCol + offset; col <= lastCol; col++) {
							*(++dl) = *(++sl);
						}
						// sl += offset;
					} else {
						for (col = lastCol + offset; col < lastCol; col++) {
							*(++dl) = bg;
						}
					}
					for (col = lastCol + offset; col < w; col++) {
						*(++dl) = *(++sl);
					}
				}
			}
			for (row = lastRow + 1; row < h; row++)
				memcpy(d + 1 + row * pprd, s + 1 + row * pprs, pprs * 4);
		} else /* Vertical */
		{
			float delta = 100.0 / (lastCol - firstCol); // For the Status Bar.
			float iamp = amplitude * (lastRow - firstRow);
			float pfac = periods / (lastCol - firstCol) * M_PI * 2;

			for (col = 0; col < firstCol; col++) {
				grey_pixel* sl = s + col + 1;
				grey_pixel* dl = d + col + 1;
				for (row = 0; row < h; row++) {
					*dl = *sl;
					dl += pprd;
					sl += pprs;
				}
			}

			for (col = firstCol; col <= lastCol + 1; col++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}
				int32 offset = int32(-iamp * sin((col - firstCol) * pfac + phase));
				grey_pixel* sl = s + col + 1;
				grey_pixel* dl = d + col + 1;
				if (offset > 0) // Shift down
				{
					for (row = 0; row < firstRow; row++) {
						*dl = *sl;
						dl += pprd;
						sl += pprs;
					}
					if (orig) {
						for (row = firstRow; row < firstRow + offset; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
						sl -= offset * pprs;
					} else {
						for (row = firstRow; row < firstRow + offset; row++) {
							*dl = bg;
							dl += pprd;
						}
					}
					for (row = firstRow + offset; row <= lastRow; row++) {
						*dl = *sl;
						dl += pprd;
						sl += pprs;
					}
					sl += offset * pprs;
					for (row = lastRow + 1; row < h; row++) {
						*dl = *sl;
						dl += pprd;
						sl += pprs;
					}
				} else // Shift up
				{
					for (row = 0; row < firstRow; row++) {
						*dl = *sl;
						dl += pprd;
						sl += pprs;
					}
					sl -= offset * pprs;
					for (row = firstRow; row <= lastRow + offset; row++) {
						*dl = *sl;
						dl += pprd;
						sl += pprs;
					}
					if (orig) {
						sl += offset * pprs;
						for (row = lastRow + offset + 1; row <= lastRow; row++) {
							*dl = *sl;
							dl += pprd;
							sl += pprs;
						}
					} else {
						for (row = lastRow + offset + 1; row <= lastRow; row++) {
							*dl = bg;
							dl += pprd;
						}
					}
					for (row = lastRow + 1; row < h; row++) {
						*dl = *sl;
						dl += pprd;
						sl += pprs;
					}
				}
			}

			for (col = lastCol + 1; col < w; col++) {
				grey_pixel* sl = s + col + 1;
				grey_pixel* dl = d + col + 1;
				for (row = 0; row < h; row++) {
					*dl = *sl;
					dl += pprd;
					sl += pprs;
				}
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
