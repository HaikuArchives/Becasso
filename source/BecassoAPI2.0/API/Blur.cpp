// © 1999-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include <string.h>

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Blur");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1999-2001 ∑ Sum Software");
	strcpy(info->description, "Applies a 3x3 blur kernel convolution");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 1;
	info->release = 3;
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
	// Blur hasn't got any UI
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
	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;
	grey_pixel* mapbits = NULL;
	uint32 mbpr = 0;
	uint32 mdiff = 0;
	if (inSelection) {
		mapbits = (grey_pixel*)inSelection->Bits();
		mbpr = inSelection->BytesPerRow();
		mdiff = mbpr - w + 2;
	}

	if (final)
		addon_start();

	float delta = 100.0 / h; // For the Status Bar.

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* sbits = (bgra_pixel*)inLayer->Bits();
		bgra_pixel* dbits = (bgra_pixel*)(*outLayer)->Bits();

		// Note:  We don't update the status bar during the corners and edges.
		//        The time this takes is insignificant anyway.

		// First do the corners:
		// Left top
		if (!inSelection || *mapbits)
			*dbits = average4(*sbits, *(sbits + 1), *(sbits + w), *(sbits + w + 1));
		else
			*dbits = *sbits;
		// Right top
		if (!inSelection || *(mapbits + w - 1))
			*(dbits + w - 1) = average4(
				*(sbits + w - 2), *(sbits + w - 1), *(sbits + 2 * w - 2), *(sbits + 2 * w - 1)
			);
		else
			*(dbits + w - 1) = *(sbits + w - 1);
		// Left bottom
		if (!inSelection || *(mapbits + (h - 1) * mbpr))
			*(dbits + (h - 1) * w) = average4(
				*(sbits + (h - 2) * w), *(sbits + (h - 2) * w + 1), *(sbits + (h - 1) * w),
				*(sbits + (h - 1) * w)
			);
		else
			*(dbits + (h - 1) * w) = *(sbits + (h - 1) * w);
		// Right bottom
		if (!inSelection || *(mapbits + (h - 1) * mbpr + w - 1))
			*(dbits + h * w - 1) = average4(
				*(sbits + (h - 1) * w - 2), *(sbits + (h - 1) * w - 1), *(sbits + h * w - 2),
				*(sbits + h * w - 1)
			);
		else
			*(dbits + h * w - 1) = *(sbits + h * w - 1);

		// Now do the edges:
		// Top/bottom edges
		for (uint32 x = 1; x < w - 1; x++) {
			if (!inSelection || *(mapbits + x)) {
				bgra_pixel* sbitsx = sbits + x;
				bgra_pixel* sbitsxl = sbitsx + w;
				*(dbits + x) = average6(
					*(sbitsx - 1), *sbitsx, *(sbitsx + 1), *(sbitsxl - 1), *sbitsxl, *(sbitsxl + 1)
				);
			}
			else
				*(dbits + x) = *(sbits + x);

			if (!inSelection || *(mapbits + (h - 1) * mbpr + x)) {
				bgra_pixel* sbitsx = sbits + (h - 1) * w + x;
				bgra_pixel* sbitsxu = sbitsx - w;
				*(dbits + (h - 1) * w + x) = average6(
					*(sbitsx - 1), *sbitsx, *(sbitsx + 1), *(sbitsxu - 1), *sbitsxu, *(sbitsxu + 1)
				);
			}
			else
				*(dbits + (h - 1) * w + x) = *(sbits + (h - 1) * w + x);
		}

		// Left/right edges:
		for (uint32 y = 1; y < h - 1; y++) {
			if (!inSelection || *(mapbits + y * mbpr)) {
				bgra_pixel* sbitsy = sbits + y * w;
				bgra_pixel* sbitsyr = sbitsy + 1;
				*(dbits + y * w) = average6(
					*(sbitsy - w), *sbitsy, *(sbitsy + w), *(sbitsyr - w), *sbitsyr, *(sbitsyr + w)
				);
			}
			else
				*(dbits + y * w) = *(sbits + y * w);

			if (!inSelection || *(mapbits + y * mbpr + w - 1)) {
				bgra_pixel* sbitsy = sbits + (y + 1) * w - 1;
				bgra_pixel* sbitsyl = sbitsy - 1;
				*(dbits + (y + 1) * w - 1) = average6(
					*(sbitsy - w), *sbitsy, *(sbitsy + w), *(sbitsyl - w), *sbitsyl, *(sbitsyl + w)
				);
			}
			else
				*(dbits + (y + 1) * w - 1) = *(sbits + (y + 1) * w - 1);
		}

		// Finally, the bulk of the canvas.
		dbits += w;
		sbits += w;
		mapbits += mbpr;
		for (uint32 y = 1; y < h - 1; y++) {
			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}
			for (uint32 x = 1; x < w - 1; x++) {
				if (!inSelection || *(++mapbits)) {
					bgra_pixel* sbitsu = ++sbits - w;
					bgra_pixel* sbitsl = sbits + w;
					*(++dbits) = average9(
						*(sbitsu - 1), *sbitsu, *(sbitsu + 1), *(sbits - 1), *sbits, *(sbits + 1),
						*(sbitsl - 1), *sbitsl, *(sbitsl + 1)
					);
				}
				else
					*(++dbits) = *(++sbits);
			}
			dbits += 2;
			sbits += 2;
			mapbits += mdiff;
		}
		break;
	}
	case M_SELECT: {
		if (!inSelection)
			*outSelection = NULL;
		else {
			grey_pixel* sbits = mapbits;
			grey_pixel* dbits = (grey_pixel*)(*outSelection)->Bits();
			// First do the corners:
			// Left top
			*dbits = (*sbits + *(sbits + 1) + *(sbits + mbpr) + *(sbits + mbpr + 1)) / 4;
			// Right top
			*(dbits + mbpr - 1) = (*(sbits + mbpr - 2) + *(sbits + mbpr - 1) +
								   *(sbits + 2 * mbpr - 2) + *(sbits + 2 * mbpr - 1)) /
								  4;
			// Left bottom
			*(dbits + (h - 1) * mbpr) = (*(sbits + (h - 2) * mbpr) + *(sbits + (h - 2) * mbpr + 1) +
										 *(sbits + (h - 1) * mbpr) + *(sbits + (h - 1) * mbpr)) /
										4;
			// Right bottom
			*(dbits + h * mbpr - 1) =
				(*(sbits + (h - 1) * mbpr - 2) + *(sbits + (h - 1) * mbpr - 1) +
				 *(sbits + h * mbpr - 2) + *(sbits + h * mbpr - 1)) /
				4;

			// Now do the edges:
			// Top/bottom edges
			for (uint32 x = 1; x < w - 1; x++) {
				grey_pixel* sbitsx = sbits + x;
				grey_pixel* sbitsxl = sbitsx + mbpr;
				*(dbits + x) = (*(sbitsx - 1) + *sbitsx + *(sbitsx + 1) + *(sbitsxl - 1) +
								*sbitsxl + *(sbitsxl + 1)) /
							   6;

				sbitsx = sbits + (h - 1) * mbpr + x;
				grey_pixel* sbitsxu = sbitsx - mbpr;
				*(dbits + (h - 1) * mbpr + x) = (*(sbitsx - 1) + *sbitsx + *(sbitsx + 1) +
												 *(sbitsxu - 1) + *sbitsxu + *(sbitsxu + 1)) /
												6;
			}

			// Left/right edges:
			for (uint32 y = 1; y < h - 1; y++) {
				grey_pixel* sbitsy = sbits + y * mbpr;
				grey_pixel* sbitsyr = sbitsy + 1;
				*(dbits + y * mbpr) = (*(sbitsy - mbpr) + *sbitsy + *(sbitsy + mbpr) +
									   *(sbitsyr - mbpr) + *sbitsyr + *(sbitsyr + mbpr)) /
									  6;

				sbitsy = sbits + (y + 1) * mbpr - 1;
				grey_pixel* sbitsyl = sbitsy - 1;
				*(dbits + (y + 1) * mbpr - 1) = (*(sbitsy - mbpr) + *sbitsy + *(sbitsy + mbpr) +
												 *(sbitsyl - mbpr) + *sbitsyl + *(sbitsyl + mbpr)) /
												6;
			}

			// Finally, the bulk of the canvas.
			dbits += mbpr; // We've already done the top row
			sbits += mbpr;
			for (uint32 y = 1; y < h - 1; y++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}
				sbits = (grey_pixel*)inSelection->Bits() + y * mbpr;
				dbits = (grey_pixel*)(*outSelection)->Bits() + y * mbpr;
				for (uint32 x = 1; x < w - 1; x++) {
					grey_pixel* sbitsu = ++sbits - mbpr;
					grey_pixel* sbitsl = sbits + mbpr;
					*(++dbits) = (*(sbitsu - 1) + *sbitsu + *(sbitsu + 1) + *(sbits - 1) + *sbits +
								  *(sbits + 1) + *(sbitsl - 1) + *sbitsl + *(sbitsl + 1)) /
								 9;
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
