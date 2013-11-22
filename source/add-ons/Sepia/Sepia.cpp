// © 2000-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include <RadioButton.h>
#include <Box.h>
#include <string.h>

rgb_color	gFg;
rgb_color	gBg;
int			gMode;

#define		DUO_BW		0
#define		DUO_SEPIA	1
#define		DUO_FB		2

class SepiaView : public BView
{
public:
			 SepiaView (BRect rect);
virtual		~SepiaView () {};

virtual void MessageReceived (BMessage *msg);
};

SepiaView::SepiaView (BRect rect)
: BView (rect, "sepia_view", B_FOLLOW_ALL, B_WILL_DRAW)
{
	ResizeTo (188, 80);
	
	BBox *mode = new BBox (BRect (8, 4, 180, 78), "mode");
	mode->SetLabel ("Colors");
	AddChild (mode);
	
	BRadioButton *rbBW = new BRadioButton (BRect (4, 14, 170, 30), "bw", "Black/White", new BMessage ('clBW'));
	BRadioButton *rbSP = new BRadioButton (BRect (4, 32, 170, 48), "sepia", "Sepia", new BMessage ('clSP'));
	BRadioButton *rbFB = new BRadioButton (BRect (4, 50, 170, 66), "fg", "Foreground/Background", new BMessage ('clFB'));
	mode->AddChild (rbBW);
	mode->AddChild (rbSP);
	mode->AddChild (rbFB);
	
	rbBW->SetValue (true);
	gFg.red = gFg.green = gFg.blue = 0;
	gBg.red = gBg.green = gBg.blue = 255;
	gMode = DUO_BW;
}

void SepiaView::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case 'clBW':
			gFg.red = gFg.green = gFg.blue = 0;
			gBg.red = gBg.green = gBg.blue = 255;
			gMode = DUO_BW;
			break;
		
		case 'clSP':
			gFg.red = 50;
			gFg.green = 5;
			gFg.blue = 8;	// brown
			gBg.red = 255;
			gBg.green = 255;
			gBg.blue = 245;	// yellowish
			gMode = DUO_SEPIA;
			break;
		
		case 'clFB':
			gFg = highcolor();
			gBg = lowcolor();
			gMode = DUO_FB;
			break;
		
		default:
			BView::MessageReceived (msg);
			return;
	}
	addon_preview();
}

#define NEW_WEIGHTS 1

status_t addon_init (uint32 index, becasso_addon_info *info)
{
	strcpy (info->name, "DuoTone");
	strcpy (info->author, "Sander Stoks");
	strcpy (info->copyright, "© 2000-2001 ∑ Sum Software");
	strcpy (info->description, "Maps pixel intensities to a background-foreground color scale");
	info->type				= BECASSO_FILTER;
	info->index				= index;
	info->version			= 0;
	info->release			= 2;
	info->becasso_version	= 2;
	info->becasso_release	= 0;
	info->does_preview		= PREVIEW_FULLSCALE;
	info->flags				= LAYER_ONLY;
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
	*view = new SepiaView (rect);
	return B_OK;
}

void addon_color_changed (void)
{
	if (gMode == DUO_FB)
	{
		gFg = highcolor();
		gBg = lowcolor();
	}
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
		else
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
	
	switch (mode)
	{
	case M_DRAW:
	{
		bgra_pixel *sbits = (bgra_pixel *) inLayer->Bits() - 1;
		bgra_pixel *dbits = (bgra_pixel *) (*outLayer)->Bits() - 1;
		rgb_color fc = gFg;
		rgb_color lc = gBg;

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
					#if defined (NEW_WEIGHTS)
					float g = (0.213*RED(p) + 0.715*GREEN(p) + 0.072*BLUE(p))/255.0;
					#else
					float g = (0.299*RED(p) + 0.587*GREEN(p) + 0.114*BLUE(p))/255.0;
					#endif
					bgra_pixel gp = PIXEL (g*lc.red + (1 - g)*fc.red, 
										   g*lc.green + (1 - g)*fc.green,
										   g*lc.blue + (1 - g)*fc.blue,
										   ALPHA(p));
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
		// does not make sense to do sepia on a selection
		break;
	}
	default:
		fprintf (stderr, "Sepia: Unknown mode\n");
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
