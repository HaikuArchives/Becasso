// © 2000-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <string.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <Box.h>

int16* gLut = 0;

#define FOREGROUND 0
#define BACKGROUND 1

#define R_BITS 5
#define G_BITS 6
#define B_BITS 5

#define R_PREC (1 << R_BITS)
#define G_PREC (1 << G_BITS)
#define B_PREC (1 << B_BITS)

#define ELEM(array, r, g, b) (array[b * R_PREC * G_PREC + r * G_PREC + g])
// This is because on MWCC, the huge array needed for the lut can't be
// created on the stack. Hence this workaround.

// #define FLOAT_WEIGHTS 1

// Strangely, the new weights give worst results.
// With the integer weights, results are almost identical with the old floats,
// and probably a lot faster.

#if defined(FLOAT_WEIGHTS)
#define R_WEIGHT 0.2125
#define G_WEIGHT 0.7154
#define B_WEIGHT 0.0721
#elif defined(OLD_WEIGHTS)
#define R_WEIGHT 0.299
#define G_WEIGHT 0.587
#define B_WEIGHT 0.114
#else
#define R_WEIGHT 2
#define G_WEIGHT 3
#define B_WEIGHT 1
#endif

#define R_SHIFT 3
#define G_SHIFT 2
#define B_SHIFT 3

/* log2(histogram cells in update box) for each axis; this can be adjusted */
#define BOX_R_LOG (R_BITS - 3)
#define BOX_G_LOG (G_BITS - 3)
#define BOX_B_LOG (B_BITS - 3)

#define BOX_R_ELEMS (1 << BOX_R_LOG) /* # of hist cells in update box */
#define BOX_G_ELEMS (1 << BOX_G_LOG)
#define BOX_B_ELEMS (1 << BOX_B_LOG)

#define BOX_R_SHIFT (R_SHIFT + BOX_R_LOG)
#define BOX_G_SHIFT (G_SHIFT + BOX_G_LOG)
#define BOX_B_SHIFT (B_SHIFT + BOX_B_LOG)

class QView : public BView {
  public:
	QView(BRect rect) : BView(rect, "Quantize_view", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		fNumColors = 256;
		fDitherCB = NULL;
		fPalette = FOREGROUND;
		ResizeTo(188, 118);
		Slider* nSlid =
			new Slider(BRect(8, 8, 180, 24), 50, "# Colors", 1, 256, 1, new BMessage('numC'));
		AddChild(nSlid);
		nSlid->SetValue(256);
		fDitherCB = new BCheckBox(
			BRect(8, 30, 180, 46), "dither", "Floyd-Steinberg Dithering", new BMessage('fsDt')
		);
		fDitherCB->SetValue(false);
		AddChild(fDitherCB);
		BBox* palB = new BBox(BRect(8, 54, 180, 110), "palette");
		palB->SetLabel("Use Colors From");
		AddChild(palB);

		BRadioButton* fgF = new BRadioButton(
			BRect(4, 14, 170, 30), "fg", "Foreground Palette", new BMessage('palF')
		);
		BRadioButton* fgB = new BRadioButton(
			BRect(4, 32, 170, 48), "bg", "Background Palette", new BMessage('palB')
		);
		palB->AddChild(fgF);
		palB->AddChild(fgB);

		fgF->SetValue(true);
	}

	virtual ~QView() {}

	virtual void MessageReceived(BMessage* msg);

	int numColors() { return fNumColors; }

	bool dither() { return (fDitherCB ? fDitherCB->Value() : false); }

	int palette() { return fPalette; }

	BCheckBox* fDitherCB;

  private:
	int fNumColors;
	int fPalette;
};

QView* view = NULL;

void
QView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'numC':
		fNumColors = int(msg->FindFloat("value"));
		break;
	case 'fsDt':
		// We query the button itself...
		break;
	case 'palF':
		fPalette = FOREGROUND;
		break;
	case 'palB':
		fPalette = BACKGROUND;
		break;
	default:
		BView::MessageReceived(msg);
		return;
	}
	for (int32 i = 0; i < R_PREC; i++)
		for (int32 j = 0; j < G_PREC; j++)
			for (int32 k = 0; k < B_PREC; k++)
				ELEM(gLut, i, j, k) = -1; // We build the LUT on the fly

	addon_preview();
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Quantize");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 2000-2001 ∑ Sum Software");
	strcpy(info->description, "Quantizes the colors to a given palette");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 0;
	info->release = 7;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = PREVIEW_FULLSCALE;
	info->flags = LAYER_ONLY;
	return B_OK;
}

status_t
addon_close(void)
{
	delete[] gLut;
	gLut = 0;
	return B_OK;
}

status_t
addon_exit(void)
{
	return B_OK;
}

void
fill_lut(int16* lut, rgb_color palette[], int max_colors, int r, int g, int b);
int
find_nearby_colors(
	rgb_color palette[], int numcolors, uint8 color_list[], int min_r, int min_g, int min_b
);
void
find_best_colors(
	rgb_color palette[], int numcolors, int minr, int ming, int minb, uint8 colorlist[],
	uint8 bestcolors[]
);

int
find_nearby_colors(
	rgb_color palette[], int numcolors, uint8 color_list[], int minr, int ming, int minb
)
{
	int maxr, maxg, maxb;
	int cr, cg, cb;
	int i, x, ncolors;
	float minmaxdist, min_dist, max_dist, tdist;
	float mindist[256]; // 256 = the maximum palette size, actually.

	maxr = minr + ((1 << BOX_R_SHIFT) - (1 << R_SHIFT));
	cr = (minr + maxr) / 2;
	maxg = ming + ((1 << BOX_G_SHIFT) - (1 << G_SHIFT));
	cg = (ming + maxg) / 2;
	maxb = minb + ((1 << BOX_B_SHIFT) - (1 << B_SHIFT));
	cb = (minb + maxb) / 2;

	/* For each color in colormap, find:
	 *  1. its minimum squared-distance to any point in the update box
	 *     (zero if color is within update box);
	 *  2. its maximum squared-distance to any point in the update box.
	 * Both of these can be found by considering only the corners of the box.
	 * We save the minimum distance for each color in mindist[];
	 * only the smallest maximum distance is of interest.
	 */

	minmaxdist = 0x7FFFFFFFL;

	for (i = 0; i < numcolors; i++) {
		/* We compute the squared-r-distance term, then add in the other two. */
		x = palette[i].red;
		if (x < minr) {
			tdist = (x - minr) * R_WEIGHT;
			min_dist = tdist * tdist;
			tdist = (x - maxr) * R_WEIGHT;
			max_dist = tdist * tdist;
		}
		else if (x > maxr) {
			tdist = (x - maxr) * R_WEIGHT;
			min_dist = tdist * tdist;
			tdist = (x - minr) * R_WEIGHT;
			max_dist = tdist * tdist;
		}
		else // within cell range so no contribution to min_dist
		{
			min_dist = 0;
			if (x <= cr) {
				tdist = (x - maxr) * R_WEIGHT;
				max_dist = tdist * tdist;
			}
			else {
				tdist = (x - minr) * R_WEIGHT;
				max_dist = tdist * tdist;
			}
		}

		x = palette[i].green;
		if (x < ming) {
			tdist = (x - ming) * G_WEIGHT;
			min_dist += tdist * tdist;
			tdist = (x - maxg) * G_WEIGHT;
			max_dist += tdist * tdist;
		}
		else if (x > maxg) {
			tdist = (x - maxg) * G_WEIGHT;
			min_dist += tdist * tdist;
			tdist = (x - ming) * G_WEIGHT;
			max_dist += tdist * tdist;
		}
		else {
			if (x <= cg) {
				tdist = (x - maxg) * G_WEIGHT;
				max_dist += tdist * tdist;
			}
			else {
				tdist = (x - ming) * G_WEIGHT;
				max_dist += tdist * tdist;
			}
		}

		x = palette[i].blue;
		if (x < minb) {
			tdist = (x - minb) * B_WEIGHT;
			min_dist += tdist * tdist;
			tdist = (x - maxb) * B_WEIGHT;
			max_dist += tdist * tdist;
		}
		else if (x > maxb) {
			tdist = (x - maxb) * B_WEIGHT;
			min_dist += tdist * tdist;
			tdist = (x - minb) * B_WEIGHT;
			max_dist += tdist * tdist;
		}
		else {
			if (x <= cb) {
				tdist = (x - maxb) * B_WEIGHT;
				max_dist += tdist * tdist;
			}
			else {
				tdist = (x - minb) * B_WEIGHT;
				max_dist += tdist * tdist;
			}
		}

		mindist[i] = min_dist; /* save away the results */
		if (max_dist < minmaxdist)
			minmaxdist = max_dist;
	}

	/* Now we know that no cell in the update box is more than minmaxdist
	 * away from some colormap entry.  Therefore, only colors that are
	 * within minmaxdist of some part of the box need be considered.
	 */

	ncolors = 0;
	for (i = 0; i < numcolors; i++) {
		if (mindist[i] <= minmaxdist)
			color_list[ncolors++] = i;
	}

	return ncolors;
}

void
find_best_colors(
	rgb_color palette[], int numcolors, int minr, int ming, int minb, uint8 colorlist[],
	uint8 bestcolors[]
)
{
	int ir, ig, ib;
	int i, icolor;
	register float* bptr; // pointer into bestdist[] array
	uint8* cptr;		  // pointer into bestcolor[] array
	float dist0, dist1;	  // initial distance values
	register float dist2; // current distance in inner loop
	float xx0, xx1;		  // distance increments
	register float xx2;
	float inc0, inc1, inc2; // initial values for increments
	// This array holds the distance to the nearest-so-far color for each cell
	float bestdist[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS];

	/* Initialize best-distance for each cell of the update box */
	bptr = bestdist - 1;
	for (i = BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS - 1; i >= 0; i--)
		*(++bptr) = 0x7FFFFFFFL;

		/* For each color selected by find_nearby_colors,
		 * compute its distance to the center of each cell in the box.
		 * If that's less than best-so-far, update best distance and color number.
		 */

		/* Nominal steps between cell centers ("x" in Thomas article) */
#define STEP_R ((1 << R_SHIFT) * R_WEIGHT)
#define STEP_G ((1 << G_SHIFT) * G_WEIGHT)
#define STEP_B ((1 << B_SHIFT) * B_WEIGHT)

	for (i = 0; i < numcolors; i++) {
		icolor = colorlist[i];
		/* Compute (square of) distance from minr/g/b to this color */
		inc0 = (minr - palette[icolor].red) * R_WEIGHT;
		dist0 = inc0 * inc0;
		inc1 = (ming - palette[icolor].green) * G_WEIGHT;
		dist0 += inc1 * inc1;
		inc2 = (minb - palette[icolor].blue) * B_WEIGHT;
		dist0 += inc2 * inc2;
		/* Form the initial difference increments */
		inc0 = inc0 * (2 * STEP_R) + STEP_R * STEP_R;
		inc1 = inc1 * (2 * STEP_G) + STEP_G * STEP_G;
		inc2 = inc2 * (2 * STEP_B) + STEP_B * STEP_B;
		/* Now loop over all cells in box, updating distance per Thomas method */
		bptr = bestdist;
		cptr = bestcolors;
		xx0 = inc0;
		for (ir = BOX_R_ELEMS - 1; ir >= 0; ir--) {
			dist1 = dist0;
			xx1 = inc1;
			for (ig = BOX_G_ELEMS - 1; ig >= 0; ig--) {
				dist2 = dist1;
				xx2 = inc2;
				for (ib = BOX_B_ELEMS - 1; ib >= 0; ib--) {
					if (dist2 < *bptr) {
						*bptr = dist2;
						*cptr = icolor;
					}
					dist2 += xx2;
					xx2 += 2 * STEP_B * STEP_B;
					bptr++;
					cptr++;
				}
				dist1 += xx1;
				xx1 += 2 * STEP_G * STEP_G;
			}
			dist0 += xx0;
			xx0 += 2 * STEP_R * STEP_R;
		}
	}
}

void
fill_lut(int16* lut, rgb_color palette[], int max_colors, int r, int g, int b)
{
	int minr, ming, minb; /* lower left corner of update box */
	int ir, ig, ib;
	register uint8* cptr; /* pointer into bestcolor[] array */

	/* This array lists the candidate colormap indexes. */
	uint8 colorlist[256];
	int numcolors; /* number of candidate colors */
	/* This array holds the actually closest colormap index for each cell. */
	uint8 bestcolor[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS];

	/* Convert cell coordinates to update box ID */
	r >>= BOX_R_LOG;
	g >>= BOX_G_LOG;
	b >>= BOX_B_LOG;

	/* Compute true coordinates of update box's origin corner.
	 * Actually we compute the coordinates of the center of the corner
	 * histogram cell, which are the lower bounds of the volume we care about.
	 */
	minr = (r << BOX_R_SHIFT) + ((1 << R_SHIFT) >> 1);
	ming = (g << BOX_G_SHIFT) + ((1 << G_SHIFT) >> 1);
	minb = (b << BOX_B_SHIFT) + ((1 << B_SHIFT) >> 1);

	/* Determine which colormap entries are close enough to be candidates
	 * for the nearest entry to some cell in the update box.
	 */
	numcolors = find_nearby_colors(palette, max_colors, colorlist, minr, ming, minb);

	/* Determine the actually nearest colors. */
	find_best_colors(palette, numcolors, minr, ming, minb, colorlist, bestcolor);

	/* Save the best color numbers (plus 1) in the main cache array */
	r <<= BOX_R_LOG; /* convert ID back to base cell indexes */
	g <<= BOX_G_LOG;
	b <<= BOX_B_LOG;

	cptr = bestcolor - 1;

	for (ir = 0; ir < BOX_R_ELEMS; ir++)
		for (ig = 0; ig < BOX_G_ELEMS; ig++)
			for (ib = 0; ib < BOX_B_ELEMS; ib++)
				ELEM(lut, (r + ir), (g + ig), (b + ib)) = *(++cptr);
}

status_t
addon_make_config(BView** vw, BRect rect)
{
	view = new QView(rect);
	*vw = view;
	gLut = new int16[R_PREC * G_PREC * B_PREC];
	for (int32 i = 0; i < R_PREC; i++)
		for (int32 j = 0; j < G_PREC; j++)
			for (int32 k = 0; k < B_PREC; k++)
				ELEM(gLut, i, j, k) = -1; // We build the LUT on the fly
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
		else // No Selection to Quantize
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

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* sbits = (bgra_pixel*)inLayer->Bits() - 1;
		bgra_pixel* dbits = (bgra_pixel*)(*outLayer)->Bits() - 1;
		rgb_color* palette;
		if (view->palette() == BACKGROUND)
			palette = lowpalette();
		else
			palette = highpalette();

		int numcolors = view->numColors();

		if (!view->dither()) // Simple quantizer
		{
			for (uint32 y = 0; y < h; y++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}
				for (uint32 x = 0; x < w; x++) {
					bgra_pixel pixel = *(++sbits);
					if (!inSelection || *(++mapbits)) {
						uint8 r = RED(pixel);
						uint8 g = GREEN(pixel);
						uint8 b = BLUE(pixel);
						//						uint16 appr = ((r << 8) & 0xF700)|((g << 3) &
						//0x07E0)|((b >> 3) & 0x001F);
						int rs = r >> R_SHIFT;
						int gs = g >> G_SHIFT;
						int bs = b >> B_SHIFT;
						if (ELEM(gLut, rs, gs, bs) < 0) // Not filled in yet
						{
							fill_lut(
								gLut, palette, numcolors, r >> R_SHIFT, g >> G_SHIFT, b >> B_SHIFT
							);
						}
						*(++dbits) = rgb2bgra(palette[ELEM(gLut, rs, gs, bs)]);
					}
					else
						*(++dbits) = *(++sbits);
				}
				mapbits += mdiff;
			}
		}
		else // FS Dither
		{

			// Foley & Van Dam, pp 572.
			// Own note:  Probably the errors in the different color channels should be weighted
			//            according to visual sensibility.  But this version is primarily meant to
			//            be quick.

			uint32 width = bounds.IntegerWidth() + 1;
			uint32 slpr = inLayer->BytesPerRow() / 4;
			bgra_pixel* src = sbits; //(bgra_pixel *) inLayer->Bits() + int (bounds.top)*slpr + int
									 //(bounds.left) - 1;
			int32 sdif = slpr - width;
			uint32 dbpr = (*outLayer)->BytesPerRow() / 4;
			bgra_pixel* dest = dbits; //(bgra_pixel *) (*outLayer)->Bits() + int (bounds.top)*dbpr +
									  //int (bounds.left) - 1;
			int32 ddif = dbpr - width;

			int* nera = new int[width];
			int* nega = new int[width];
			int* neba = new int[width];
			int* cera = new int[width];
			int* cega = new int[width];
			int* ceba = new int[width];

			bzero(nera, width * sizeof(int));
			bzero(nega, width * sizeof(int));
			bzero(neba, width * sizeof(int));
			bzero(cera, width * sizeof(int));
			bzero(cega, width * sizeof(int));
			bzero(ceba, width * sizeof(int));

			int r, g, b, er, eg, eb, per, peg, peb;
			uint8 apix;
			uint32 x, y;
			rgb_color a;

			for (y = uint32(bounds.top); y < uint32(bounds.bottom); y++) {
				//				printf ("%ld", y); fflush (stdout);
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}

				x = 0;

				// Special case: First pixel in a row
				bgra_pixel s = *(++src);
				r = clip8(RED(s) + cera[0]);
				g = clip8(GREEN(s) + cega[0]);
				b = clip8(BLUE(s) + ceba[0]);

				cera[0] = 0;
				cega[0] = 0;
				ceba[0] = 0;

				// Find the nearest match in the palette and write it out
				int rs = r >> R_SHIFT;
				int gs = g >> G_SHIFT;
				int bs = b >> B_SHIFT;
				if (ELEM(gLut, rs, gs, bs) < 0) // Not filled in yet
					fill_lut(gLut, palette, numcolors, rs, gs, bs);
				apix = ELEM(gLut, rs, gs, bs);
				// And the corresponding RGB color
				a = palette[apix];

				if (!inSelection || *(++mapbits))
					*(++dest) = rgb2bgra(a);
				else
					*(++dest) = s;

				// Calculate the error terms
				er = r - a.red;
				eg = g - a.green;
				eb = b - a.blue;

				per = 7 * er / 16;
				peg = 7 * eg / 16;
				peb = 7 * eb / 16;

				// Put all the remaining error in the pixels down and down-right
				// (since there is no pixel down-left...)
				nera[x] += er / 2;
				nega[x] += eg / 2;
				neba[x] += eb / 2;
				nera[x + 1] += er / 16;
				nega[x + 1] += eg / 16;
				neba[x + 1] += eb / 16;

				for (x = 1; x < width - 1; x++) {
					//					printf (","); fflush (stdout);
					// Get one source pixel
					s = *(++src);

					// Get color components and add errors from previous pixel
					r = clip8(RED(s) + per + cera[x]);
					g = clip8(GREEN(s) + peg + cega[x]);
					b = clip8(BLUE(s) + peb + ceba[x]);

					cera[x] = 0;
					cega[x] = 0;
					ceba[x] = 0;

					// Find the nearest match in the palette and write it out
					int rs = r >> R_SHIFT;
					int gs = g >> G_SHIFT;
					int bs = b >> B_SHIFT;
					if (ELEM(gLut, rs, gs, bs) < 0) // Not filled in yet
						fill_lut(gLut, palette, numcolors, rs, gs, bs);
					apix = ELEM(gLut, rs, gs, bs);
					// And the corresponding RGB color
					a = palette[apix];

					//					printf ("%c.", 8); fflush (stdout);
					if (!inSelection || *(++mapbits))
						*(++dest) = rgb2bgra(a);
					else
						*(++dest) = s;
					//					printf ("%c:", 8); fflush (stdout);


					// Calculate the error terms
					er = r - a.red;
					eg = g - a.green;
					eb = b - a.blue;

					per = 7 * er / 16;
					peg = 7 * eg / 16;
					peb = 7 * eb / 16;

					nera[x - 1] += 3 * er / 16;
					nega[x - 1] += 3 * eg / 16;
					neba[x - 1] += 3 * eb / 16;
					nera[x] += 5 * er / 16;
					nega[x] += 5 * eg / 16;
					neba[x] += 5 * eb / 16;
					nera[x + 1] += er / 16;
					nega[x + 1] += eg / 16;
					neba[x + 1] += eb / 16;
				}
				// Special case: Last pixel
				//				printf ("Writing last pixel - "); fflush (stdout);
				s = *(++src);
				//				printf ("1"); fflush (stdout);

				// Get color components and add errors from previous pixel
				r = clip8(RED(s) + per + cera[x]);
				g = clip8(GREEN(s) + peg + cega[x]);
				b = clip8(BLUE(s) + peb + ceba[x]);

				cera[x] = 0;
				cega[x] = 0;
				ceba[x] = 0;

				// Find the nearest match in the palette and write it out
				rs = r >> R_SHIFT;
				gs = g >> G_SHIFT;
				bs = b >> B_SHIFT;
				if (ELEM(gLut, rs, gs, bs) < 0) // Not filled in yet
					fill_lut(gLut, palette, numcolors, rs, gs, bs);
				apix = ELEM(gLut, rs, gs, bs);
				//				printf ("@"); fflush (stdout);
				// And the corresponding RGB color
				a = palette[apix];

				//				printf ("2"); fflush (stdout);
				if (!inSelection || *(++mapbits))
					*(++dest) = rgb2bgra(a);
				else
					*(++dest) = s;
				//				printf ("Still alive.\n");

				// Calculate the error terms
				er = r - a.red;
				eg = g - a.green;
				eb = b - a.blue;

				// Put all the error in the pixels down and down-left
				nera[x - 1] += er / 2;
				nega[x - 1] += eg / 2;
				neba[x - 1] += eb / 2;
				nera[x] += er / 2;
				nega[x] += eg / 2;
				neba[x] += eb / 2;

				// Switch the scratch data
				int* tmp;
				tmp = cera;
				cera = nera;
				nera = tmp;
				tmp = cega;
				cega = nega;
				nega = tmp;
				tmp = ceba;
				ceba = neba;
				neba = tmp;

				dest += ddif;
				src += sdif;
				mapbits += mdiff;
			}
			// Special case: Last line
			// All the error goes into the right pixel.
			er = 0;
			eg = 0;
			eb = 0;

			//			printf ("Entering last line...\n");
			for (x = 0; x < width - 1; x++) {
				// Get one source pixel
				bgra_pixel s = *(++src);

				// Get color components and add errors from previous pixel
				r = clip8(RED(s) + er + cera[x]);
				g = clip8(GREEN(s) + eg + cega[x]);
				b = clip8(BLUE(s) + eb + ceba[x]);

				// Find the nearest match in the palette and write it out
				int rs = r >> R_SHIFT;
				int gs = g >> G_SHIFT;
				int bs = b >> B_SHIFT;
				if (ELEM(gLut, rs, gs, bs) < 0) // Not filled in yet
					fill_lut(gLut, palette, numcolors, rs, gs, bs);
				apix = ELEM(gLut, rs, gs, bs);
				// And the corresponding RGB color
				a = palette[apix];

				if (!inSelection || *(++mapbits))
					*(++dest) = rgb2bgra(a);
				else
					*(++dest) = s;

				// Calculate the error terms
				er = r - a.red;
				eg = g - a.green;
				eb = b - a.blue;
			}
			// Last but not least, the bottom right pixel.
			bgra_pixel s = *(++src);

			r = clip8(RED(s) + er + cera[x]);
			g = clip8(GREEN(s) + eg + cega[x]);
			b = clip8(BLUE(s) + eb + ceba[x]);

			int rs = r >> R_SHIFT;
			int gs = g >> G_SHIFT;
			int bs = b >> B_SHIFT;
			if (ELEM(gLut, rs, gs, bs) < 0) // Not filled in yet
				fill_lut(gLut, palette, numcolors, rs, gs, bs);
			apix = ELEM(gLut, rs, gs, bs);

			if (!inSelection || *(++mapbits))
				*(++dest) = rgb2bgra(palette[apix]);
			else
				*(++dest) = s;

			delete[] nera;
			delete[] nega;
			delete[] neba;
			delete[] cera;
			delete[] cega;
			delete[] ceba;
		}
		delete[] palette;
		break;
	}
	case M_SELECT: {
		break;
	}
	default:
		fprintf(stderr, "Quantize: Unknown mode\n");
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
