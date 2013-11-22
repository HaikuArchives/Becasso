// © 1999-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include <string.h>

status_t addon_init (uint32 index, becasso_addon_info *info)
{
	strcpy (info->name, "Negate");
	strcpy (info->author, "Sander Stoks");
	strcpy (info->copyright, "© 1999-2001 ∑ Sum Software");
	strcpy (info->description, "Inverts the image");
	info->type				= BECASSO_FILTER;
	info->index				= index;
	info->version			= 0;
	info->release			= 3;
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
	*view = NULL;	// Negate doesn't have a GUI
	return B_OK;
}

status_t process (Layer *inLayer, Selection *inSelection, 
				  Layer **outLayer, Selection **outSelection, int32 mode,
				  BRect * /*frame*/, bool final, BPoint /* point */, uint32 /* buttons */)
{
	int error = ADDON_OK;
	BRect bounds = inLayer->Bounds();
//	printf ("Bounds: ");
//	bounds.PrintToStream();
//	printf ("Frame:  ");
//	frame->PrintToStream();
	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer (*inLayer);
	if (mode == M_SELECT)
	{
		if (inSelection)
			*outSelection = new Selection (*inSelection);
		else
			return (0);
	}
	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();
	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;
	grey_pixel *mapbits = NULL;
	uint32  mbpr    = 0;
	uint32  mdiff   = 0;
	if (inSelection)
	{
		// printf ("inSelection\n");
		mapbits = (grey_pixel *) inSelection->Bits();
		mbpr  = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (final)
		addon_start();
	
	float delta = 100.0/h;	// For the Status Bar.
	
	switch (mode)
	{
	case M_DRAW:
	{
		bgra_pixel *sbits = (uint32 *) inLayer->Bits() - 1;
		bgra_pixel *dbits = (uint32 *) (*outLayer)->Bits() - 1;

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
				grey_pixel mp = inSelection ? *(++mapbits) : 255;
				if (mp)
				{
					bgra_pixel p = *(++sbits);
					bgra_pixel gp = PIXEL (255 - RED(p), 255 - GREEN(p), 255 - BLUE(p), ALPHA(p));
					if (mp == 255)
						*(++dbits) = gp;
					else
						*(++dbits) = weighted_average (gp, mp, p, 255 - mp);
				}
				else
					*(++dbits) = *(++sbits);
			}
			mapbits += mdiff;
		}
		break;
	}
	case M_SELECT:
	{
		grey_pixel *sbits = (grey_pixel *) inSelection->Bits() - 1;
		grey_pixel *dbits = (grey_pixel *) (*outSelection)->Bits() - 1;
		int32 sdiff = inSelection->BytesPerRow() - w;
		int32 ddiff = (*outSelection)->BytesPerRow() - w;

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
				*(++dbits) = 255 - *(++sbits);
			}
			sbits += sdiff;
			dbits += ddiff;
		}
		break;
	}
	default:
		fprintf (stderr, "Negate: Unknown mode\n");
		error = ADDON_UNKNOWN;
	}

	if (*outSelection)
		(*outSelection)->Unlock();
	if (*outLayer)
		(*outLayer)->Unlock();
	
	if (final)
		addon_done();
	
	return error;
}
