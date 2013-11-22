// © 2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include <string.h>

status_t addon_init (uint32 index, becasso_addon_info *info)
{
	strcpy (info->name, "AutoContrast");
	strcpy (info->author, "Sander Stoks");
	strcpy (info->copyright, "© 2001 ∑ Sum Software");
	strcpy (info->description, "Optimizes image contrast");
	info->type				= BECASSO_FILTER;
	info->index				= index;
	info->version			= 0;
	info->release			= 1;
	info->becasso_version	= 2;
	info->becasso_release	= 0;
	info->does_preview		= PREVIEW_FULLSCALE;
	info->flags				= 0;
	return B_OK;
}

status_t addon_close (void)
{
	return B_OK;
}

status_t addon_exit (void)
{
	return B_OK;
}

status_t addon_make_config (BView **view, BRect rect)
{
	return B_OK;
}

status_t process (Layer *inLayer, Selection *inSelection, 
				  Layer **outLayer, Selection **outSelection, int32 mode,
				  BRect * /*frame*/, bool final, BPoint /* point */, uint32 /* buttons */)
{
	status_t error = ADDON_OK;
	BRect bounds = inLayer->Bounds();
	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer (*inLayer);
	if (mode == M_SELECT)
	{
		if (inSelection)
			*outSelection = new Selection (*inSelection);
		else	// No Selection!
			return (0);
	}
	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();
	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;
	grey_pixel  *mapbits = NULL;
	uint32  mbpr    = 0;
	uint32  mdiff   = 0;
	if (inSelection)
	{
		mapbits = (grey_pixel *) inSelection->Bits() - 1;
		mbpr  = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (final)
		addon_start();

	float delta = 50.0/h;	// For the Status Bar.  Two passes -> 50 instead of 100.
	
	switch (mode)
	{
	case M_DRAW:
	{
		bgra_pixel *sbits = (bgra_pixel *) inLayer->Bits() - 1;

		// first, find the current max and min values
		float minC = 255.0;
		float maxC = 0.0;
		for (uint32 y = 0; y < h; y++)
		{
			if (final)
			{
				addon_update_statusbar (delta);
				if (addon_stop())
				{
					error = ADDON_ABORT;
					break;
				}
			}
			for (uint32 x = 0; x < w; x++)
			{
				if (!inSelection || *(++mapbits))
				{
					bgra_pixel pixel = *(++sbits);
					int cval = int (0.213*RED(pixel) + 0.715*GREEN(pixel) + 0.072*BLUE(pixel) + .5);
					if (cval < minC) minC = cval;
					if (cval > maxC) maxC = cval;
				}
				else
					++sbits;
			}
			mapbits += mdiff;
		}
//		printf ("min = %f, max = %f\n", minC, maxC);

		// now, adjust contrast accordingly
		if (maxC > minC)
		{
			float factor = 255.0/(maxC - minC);
			sbits = (bgra_pixel *) inLayer->Bits() - 1;
			mapbits = inSelection ? (grey_pixel *) inSelection->Bits() - 1 : 0;
			bgra_pixel *dbits = (bgra_pixel *) (*outLayer)->Bits() - 1;
	
			for (uint32 y = 0; y < h; y++)
			{
				if (final)
				{
					addon_update_statusbar (delta);
					if (addon_stop())
					{
						error = ADDON_ABORT;
						break;
					}
				}
				for (uint32 x = 0; x < w; x++)
				{
					int mapx = 255;
					if (!inSelection || (mapx = *(++mapbits)))
					{
						bgra_pixel pixel = *(++sbits);
						bgra_pixel newpix = PIXEL (clipchar (factor*(RED(pixel) - minC)),
												   clipchar (factor*(GREEN(pixel) - minC)),
												   clipchar (factor*(BLUE(pixel) - minC)),
												   ALPHA(pixel));
						*(++dbits) = weighted_average (newpix, mapx, pixel, 255 - mapx);
					}
					else
						*(++dbits) = *(++sbits);
				}
				mapbits += mdiff;
			}
		}
		else
		{
			sbits = (bgra_pixel *) inLayer->Bits();
			bgra_pixel *dbits = (bgra_pixel *) (*outLayer)->Bits();
			memcpy (dbits, sbits, w*h*4);
		}
		break;
	}
	case M_SELECT:
	{
		if (!inSelection)
			*outSelection = NULL;
		else
		{
			// first, find the current max and min values
			float minC = 255.0;
			float maxC = 0.0;
			grey_pixel *sbits = mapbits;
			for (uint32 y = 0; y < h; y++)
			{
				if (final)
				{
					addon_update_statusbar (delta);
					if (addon_stop())
					{
						error = ADDON_ABORT;
						break;
					}
				}
				for (uint32 x = 0; x < w; x++)
				{
					grey_pixel pixel = *(++sbits);
					if (pixel > maxC) maxC = pixel;
					if (pixel < minC) minC = pixel;
				}
				sbits += mdiff;
			}
			
			// now, adjust contrast accordingly
			if (maxC > minC)
			{
				float factor = 255.0/(maxC - minC);
	
				sbits = mapbits;
				grey_pixel *dbits = (grey_pixel *) (*outSelection)->Bits() - 1;

				for (uint32 y = 0; y < h; y++)
				{
					if (final)
					{
						addon_update_statusbar (delta);
						if (addon_stop())
						{
							error = ADDON_ABORT;
							break;
						}
					}
					for (uint32 x = 0; x < w; x++)
					{
						grey_pixel pixel = *(++sbits);
						*(++dbits) = clipchar (factor*(pixel - minC));
					}
					sbits += mdiff;
					dbits += mdiff;
				}
			}
			else
			{
				sbits = mapbits;
				grey_pixel *dbits = (grey_pixel *) (*outSelection)->Bits();
				memcpy (dbits, sbits, w*h);
			}
		}
		break;
	}
	default:
		fprintf (stderr, "AutoContrast: Unknown mode\n");
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
