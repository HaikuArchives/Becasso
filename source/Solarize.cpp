// © 1999-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <string.h>

float		gThreshold;

class SolarizeView : public BView
{
public:
			 SolarizeView (BRect rect) : BView (rect, "solarize view", B_FOLLOW_ALL, B_WILL_DRAW)
			 {
			 	gThreshold = 0;
				ResizeTo (188, 26);
				Slider *thresholdS = new Slider (BRect (4, 4, 184, 22), 64, "Threshold", 0, 100, 1, new BMessage ('thrS'));
				AddChild (thresholdS);
			 }
virtual		~SolarizeView () {}

virtual void MessageReceived (BMessage *msg);
};

void SolarizeView::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
	case 'thrS':
		gThreshold = msg->FindFloat ("value");
		addon_preview();
		break;
	default:
		BView::MessageReceived (msg);
	}
}

status_t addon_init (uint32 index, becasso_addon_info *info)
{
	strcpy (info->name, "Solarize");
	strcpy (info->author, "Sander Stoks");
	strcpy (info->copyright, "© 1999-2001 ∑ Sum Software");
	strcpy (info->description, "Simulates over-exposure");
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
	*view = new SolarizeView (rect);
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
	if (*outLayer == NULL && mode== M_DRAW)
		*outLayer = new Layer (*inLayer);
	if (mode == M_SELECT)
	{
		if (inSelection)
			*outSelection = new Selection (*inSelection);
		else	// No Selection to blur!
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
		mapbits = (grey_pixel *) inSelection->Bits();
		mbpr  = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (final)
		addon_start();

	float delta = 100.0/h;	// For the Status Bar.
	float threshold = gThreshold/25500;
	
	switch (mode)
	{
	case M_DRAW:
	{
		bgra_pixel *sbits = (bgra_pixel *) inLayer->Bits();
		bgra_pixel *dbits = (bgra_pixel *) (*outLayer)->Bits();

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
					uchar red = RED(p);
					uchar green = GREEN(p);
					uchar blue = BLUE(p);
					uchar alpha = ALPHA(p);
					float intensity = 0.0008333*red + 0.0028055*green + 0.00028275*blue;
					if (intensity >= threshold*mp)
						*(++dbits) = PIXEL (255 - red, 255 - green, 255 - blue, alpha);
					else
						*(++dbits) = p;
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
		// no sense to solarize the selection map
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

	return (error);
}
