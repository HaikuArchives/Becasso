#include <Screen.h>
#include <TextControl.h>
#include <Clipboard.h>
#include <Beep.h>
#include "CanvasView.h"
#include "CanvasWindow.h"
#include "Becasso.h"
#include "PicMenuButton.h"
#include "ColorMenuButton.h"
#include "PatternMenuButton.h"
#include "AttribDraw.h"
#include "AttribSelect.h"
#include "Modes.h"
#include "DModes.h"
#include "Tools.h"
#include "hsv.h"
#include "Brush.h"
#include "BitmapStuff.h"
#include "BGView.h"
#include <zlib.h>
#include "Tablet.h"
#include "BecassoAddOn.h"
#include "debug.h"
#include "mmx.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <PrintJob.h>
#include <NodeInfo.h>
#include <Mime.h>
#include <BitmapStream.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <WindowScreen.h>
#include <MessageQueue.h>
#include "Properties.h"
#include "Settings.h"
#include "NagWindow.h"
#include "LayerWindow.h"
#include "PosView.h"
#include "MagWindow.h"

#define BLEND_USES_SHIFTS

static BRect PreviewRect()
{
	extern BLocker g_settings_lock;
	extern becasso_settings g_settings;
	g_settings_lock.Lock();
	BRect rect (0, 0, g_settings.preview_size - 1, g_settings.preview_size - 1);
	g_settings_lock.Unlock();
	return rect;
}

status_t GetExportingTranslators (BMessage *inHere);
uint32 TypeCodeForMIME (const char *MIME);

CanvasView::CanvasView (const BRect frame, const char *name, BBitmap *map, rgb_color color)
: SView (frame, name, B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED | B_SUBPIXEL_PRECISE)
{
//	printf ("Map version\n");
	SetViewColor (DarkGrey);
	BRect bitmapRect;
	if (map)
	{
		bitmapRect = map->Bounds();
	}
	else
	{
		bitmapRect.Set (0, 0, frame.Width(), frame.Height());
	}
	fCanvasFrame = bitmapRect;
	layer[0] = new Layer (bitmapRect, lstring (162, "Background"));
	fCurrentLayer = 0;
	fNumLayers = 1;

	drawView = new SView (bitmapRect, name, B_FOLLOW_NONE, uint32 (NULL));
	layer[0]->Lock();
	layer[0]->AddChild (drawView);
	drawView->SetDrawingMode (B_OP_COPY);
	if (map)
	{
		if (map->ColorSpace() == B_COLOR_8_BIT)	// This should *NOT* be necessary!
		{
			const color_map *cmap = system_colors();
			const rgb_color *palette = cmap->color_list;
			rgb_color color;
			uint8 *sourceptrb = (uint8 *) map->Bits();
			uint8 *rowptr = sourceptrb;
			uint8 *destptrb = (uint8 *) layer[0]->Bits() - 1;
			int32 x, y, deltarow = map->BytesPerRow();
			int32 numrows = map->Bounds().IntegerHeight() + 1;
			int32 numcolumns = map->Bounds().IntegerWidth() + 1;
			uint8 pixelb;

			for (y = 0; y < numrows; y++)
			{
				sourceptrb = rowptr - 1;
				for (x = 0; x < numcolumns; x++)
				{
					pixelb = *(++sourceptrb);
					color = palette[pixelb];
					*(++destptrb) = color.blue;
					*(++destptrb) = color.green;
					*(++destptrb) = color.red;
					*(++destptrb) = 255;	// opaque
				}
				rowptr += deltarow;
			}
		}
		else
		{
			drawView->DrawBitmapAsync (map, B_ORIGIN);
			drawView->Sync();
//			uint8 *srcptr = (uint8 *) map->Bits();
//			uint8 *lptr   = (uint8 *) layer[0]->Bits();
//			printf ("b = %i, g = %i, r = %i, a = %i\n", *srcptr++, *srcptr++, *srcptr++, *srcptr);
//			printf ("b = %i, g = %i, r = %i, a = %i\n", *lptr++, *lptr++, *lptr++, *lptr);
		}
	}
	else
	{
		drawView->SetLowColor (color);
		drawView->FillRect (bitmapRect, B_SOLID_LOW);
		drawView->Sync();
	}
	layer[0]->Unlock();

	RestOfCtor ();
}

CanvasView::CanvasView (const BRect frame, const char *name, FILE *fp)
: SView (frame, name, B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED)
{
//	printf ("*fp version\n");
	SetViewColor (B_TRANSPARENT_32_BIT);
	BRect bitmapRect;
	bitmapRect.Set (0, 0, frame.Width(), frame.Height());
	fCanvasFrame = bitmapRect;

	rewind (fp);
	char line[81];
	do
	{
		fgets (line, 80, fp);
	}	while (line[0] == '#');
	char *endp = line;
	float vers = strtod (endp, &endp);
	fNumLayers = strtol (endp, &endp, 10);
	fCurrentLayer = strtol (endp, &endp, 10);
	/*int32 dpi = */strtol (endp, &endp, 10);	// not used
	int watermarked = strtol (endp, &endp, 10);	// watermarked image?
	switch (int (vers))
	{
	case 0:
		for (int i = 0; i < fNumLayers; i++)
		{
			fgets (line, 80, fp);
			line[strlen(line) - 1] = 0;
			endp = line;
			BRect lFrame;
			lFrame.left = strtod (endp, &endp);
			lFrame.top = strtod (endp, &endp);
			lFrame.right = strtod (endp, &endp);
			lFrame.bottom = strtod (endp, &endp);
//			int m = strtol (endp, &endp, 10);
			bool h = strtol (endp, &endp, 10);
			layer[i] = new Layer (lFrame, endp);
			layer[i]->Hide (h);
			layer[i]->setMode (0);	// Note: DM_BLEND has moved to 0, and <1.1 didn't have any other ops anyway.
			fread (layer[i]->Bits(), layer[i]->BitsLength(), 1, fp);
		}
		break;
	case 1:
	{
		uchar *tmpzbuf = NULL;
		z_streamp zstream = new z_stream;
		zstream->zalloc = Z_NULL;
		zstream->zfree = Z_NULL;
		zstream->opaque = Z_NULL;
		if (inflateInit (zstream) != Z_OK)
		{
			fprintf (stderr, "Oops!  Problems with inflateInit()...\n");
		}
		for (int i = 0; i < fNumLayers; i++)
		{
			fgets (line, 80, fp);
			line[strlen(line) - 1] = 0;
			endp = line;
			BRect lFrame;
			lFrame.left = strtod (endp, &endp);
			lFrame.top = strtod (endp, &endp);
			lFrame.right = strtod (endp, &endp);
			lFrame.bottom = strtod (endp, &endp);
			int m = strtol (endp, &endp, 10);
			bool h = strtol (endp, &endp, 10);
			uchar ga = strtol (endp, &endp, 10);
			if (vers > 1.1)
			{
				/* int am = */strtol (endp, &endp, 10);
				// printf ("%salpha map\n", am ? "" : "no ");
			}
			layer[i] = new Layer (lFrame, endp);
			layer[i]->Hide (h);
			if (vers == 1.0)
				layer[i]->setMode (0);	// Note: DM_BLEND has moved to 0, and <1.1 didn't have any other ops anyway.
			else
				layer[i]->setMode (m);
			layer[i]->setGlobalAlpha (ga);
			layer[i]->setAlphaMap (NULL);
		}
		for (int i = 0; i < fNumLayers; i++)
		{
			if (!tmpzbuf)
			{
				tmpzbuf = new uchar [layer[i]->BitsLength()];
				zstream->avail_in = 0;
			}
			zstream->next_in = tmpzbuf;
			zstream->avail_in += fread (tmpzbuf, 1, layer[i]->BitsLength() - zstream->avail_in, fp);
			zstream->next_out = (uchar *) layer[i]->Bits();
			zstream->avail_out = layer[i]->BitsLength();
//			printf ("avail_in = %li, avail_out = %li\n", zstream->avail_in, zstream->avail_out);
			if (inflate (zstream, Z_FINISH) != Z_STREAM_END)
			{
				fprintf (stderr, "Oops!  Layer couldn't be decompressed completely...\n");
			}
			memmove (tmpzbuf, zstream->next_in, zstream->avail_in);
			inflateReset (zstream);
		}
		if (inflateEnd (zstream) != Z_OK)
		{
			fprintf (stderr, "Oops!  Temporary zlib buffer couldn't be deallocated\n");
		}
		delete [] tmpzbuf;
		delete zstream;

		if (watermarked)
			InsertGlobalAlpha (layer, fNumLayers);

		break;
	}
	case 2:
	{
		uchar *tmpzbuf = NULL;
		z_streamp zstream = new z_stream;
		zstream->zalloc = Z_NULL;
		zstream->zfree = Z_NULL;
		zstream->opaque = Z_NULL;
		if (inflateInit (zstream) != Z_OK)
		{
			fprintf (stderr, "Oops!  Problems with inflateInit()...\n");
		}
		for (int i = 0; i < fNumLayers; i++)
		{
			fgets (line, 80, fp);
			line[strlen(line) - 1] = 0;
			endp = line;
			BRect lFrame;
			lFrame.left = strtod (endp, &endp);
			lFrame.top = strtod (endp, &endp);
			lFrame.right = strtod (endp, &endp);
			lFrame.bottom = strtod (endp, &endp);
			int m = strtol (endp, &endp, 10);
			bool h = strtol (endp, &endp, 10);
			uchar ga = strtol (endp, &endp, 10);
			if (vers > 1.1)
			{
				/* int am = */strtol (endp, &endp, 10);
				// printf ("%salpha map\n", am ? "" : "no ");
			}
			layer[i] = new Layer (lFrame, endp + 1);
			layer[i]->Hide (h);
			if (vers == 1.0)
				layer[i]->setMode (0);	// Note: DM_BLEND has moved to 0, and <1.1 didn't have any other ops anyway.
			else
				layer[i]->setMode (m);
			layer[i]->setGlobalAlpha (ga);
			layer[i]->setAlphaMap (NULL);
		}
		for (int i = 0; i < fNumLayers; i++)
		{
			if (!tmpzbuf)
			{
				tmpzbuf = new uchar [layer[i]->BitsLength()];
				zstream->avail_in = 0;
			}
			zstream->next_in = tmpzbuf;
			zstream->avail_in += fread (tmpzbuf, 1, layer[i]->BitsLength() - zstream->avail_in, fp);
			zstream->next_out = (uchar *) layer[i]->Bits();
			zstream->avail_out = layer[i]->BitsLength();
//			printf ("avail_in = %li, avail_out = %li\n", zstream->avail_in, zstream->avail_out);
			if (inflate (zstream, Z_FINISH) != Z_STREAM_END)
			{
				fprintf (stderr, "Oops!  Layer couldn't be decompressed completely...\n");
			}
			memmove (tmpzbuf, zstream->next_in, zstream->avail_in);
			inflateReset (zstream);
		}
		if (inflateEnd (zstream) != Z_OK)
		{
			fprintf (stderr, "Oops!  Temporary zlib buffer couldn't be deallocated\n");
		}
		delete [] tmpzbuf;
		delete zstream;

		if (watermarked)
			InsertGlobalAlpha (layer, fNumLayers);

		break;
	}
	default:	// This should never happen
		fprintf (stderr, "Hmm, somehow a newer version file got through to the actual loading routine.\n");
	}
	fclose (fp);

	drawView = new SView (bitmapRect, name, B_FOLLOW_NONE, B_SUBPIXEL_PRECISE);
	currentLayer()->AddChild (drawView);
	RestOfCtor ();
}

void CanvasView::RestOfCtor ()
{
	selection = new Selection (fCanvasFrame);
	selectionView = new SView (fCanvasFrame, "Selection View", B_FOLLOW_NONE, B_SUBPIXEL_PRECISE);
	selection->AddChild (selectionView);
	SetViewColor (B_TRANSPARENT_32_BIT);
	selchanged = false;
	prevPaste = fCanvasFrame;
	prev_eraser = fCanvasFrame;
	cutbg = NULL;
	changed = false;
	entry = 0;
	prevTool = -1;
	mouse_on_canvas = false;
	polygon = new BPolygon;
	polypoint = BPoint (-1, -1);
	tolerance = 0;
	toleranceRGB.red = 0;
	toleranceRGB.green = 0;
	toleranceRGB.blue = 0;
	text = NULL;
	indexUndo = -1;
	fIndex1 = -1;
	fIndex2 = -1;
	maxUndo = 0;
	for (int i = 0; i < MAX_UNDO; i++)
	{
		undo[i].bitmap  = NULL;
		undo[i].sbitmap = NULL;
	}
	myWindow = NULL;
	windowLock = false;
//	extern bool inPaste, inDrag;
//	if (inPaste)
//		printf ("inPaste\n");
//	if (inDrag)
//		printf ("inDrag\n");
	firstDrag = true;
	first = true;
	sel = false;
	inAddOn = false;
	newWin = true;
	// doFilter = true;
	fScale = 1;
	pradius = 200;
	fBGView = NULL;
	fPreviewRect = PreviewRect();
	fPrevpreviewRect = fPreviewRect;
	cutView = NULL;
	filterOpen = NULL;
	transformerOpen = NULL;
	generatorOpen = NULL;
	previewLayer = new Layer (fPreviewRect, "Preview Layer");
	previewSelection = new Selection (fPreviewRect);
	didPreview = 0;
	transformerSaved = false;
	generatorSaved = false;
//	addonInQueue = NULL;
	fCurrentProperty = 0;
	fLayerSpecifier = -1;
	fTreshold = 0;
	tr2x2Layer = NULL;
	tr2x2Selection = NULL;
	fMouseDown = false;
	fPicking = false;
	fDragScroll = false;
	fTranslating = false;
	fRotating = false;
	fLastCenter = BPoint (fCanvasFrame.Width()/2, fCanvasFrame.Height()/2);
}

CanvasView::~CanvasView ()
{
	extern Becasso *mainapp;
	mainapp->setHand();
	layer[fCurrentLayer]->RemoveChild (drawView);
	delete drawView;

	for (int i = 0; i < fNumLayers; i++)
		delete layer[i];

	if (screenbitmap->ColorSpace() != B_RGBA32)
		delete screenbitmap32;
	delete screenbitmap;
	delete polygon;

	selection->RemoveChild (selectionView);
	delete selectionView;
	delete selection;

	for (int i = 0; i < MAX_UNDO; i++)
	{
		delete undo[i].bitmap;
		delete undo[i].sbitmap;
	}

	delete cutbg;
	delete previewLayer;
	delete previewSelection;
//	delete cutView;
}

void CanvasView::AttachedToWindow ()
{
	extern bool inPaste;
	bool wasInPaste = inPaste;
	// printf ("Attached to window%s.\n", inPaste ? " while in paste" : "");
	inPaste = false;
	extern BBitmap *clip;
	BScreen screen;
	if (screen.ColorSpace() == B_COLOR_8_BIT)
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_COLOR_8_BIT, true);
		screenbitmap32 = new BBitmap (fCanvasFrame, B_RGBA32, true);
	}
	else	// 32 or 16 bit screens
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_RGBA32, true);
		screenbitmap32 = screenbitmap;
	}
//	totaldatalen = (fCanvasFrame.IntegerWidth() + 1)*(fCanvasFrame.IntegerHeight() + 1)*3;
//	totaldata = new uchar [totaldatalen];

	FrameResized (Bounds().Width(), Bounds().Height());
	// This is here to get the scrollbars right.

	myWindow = (CanvasWindow *)Window();
	myWindow->Lock();
	myWindow->setMenuItem (B_UNDO, false);
	myWindow->setMenuItem ('redo', false);
	myWindow->setMenuItem (B_CUT, false);
	myWindow->setMenuItem (B_COPY, false);
	myWindow->setMenuItem ('csel', false);
	myWindow->setPaste (clip != NULL);
	myWindow->setMenuItem ('sund', false);
	myWindow->setMenuItem ('sinv', false);
	if (wasInPaste)
	{
		Paste (true);
	}
	myWindow->Unlock();
	Invalidate();
	MakeFocus();
}

void CanvasView::ScreenChanged (BRect /* frame */, color_space mode)
{
	if (mode == B_COLOR_8_BIT)
	{
		if (screenbitmap->ColorSpace() != B_COLOR_8_BIT)
		{
			delete screenbitmap;
			screenbitmap = new BBitmap (fCanvasFrame, B_COLOR_8_BIT, true);
			screenbitmap32 = new BBitmap (fCanvasFrame, B_RGBA32, true);
		}
	}
	else
	{
		if (screenbitmap->ColorSpace() == B_COLOR_8_BIT)
		{
			delete screenbitmap32;
			delete screenbitmap;
			screenbitmap = new BBitmap (fCanvasFrame, B_RGBA32, true);
			screenbitmap32 = screenbitmap;
		}
	}
	Invalidate();
}

void CanvasView::FrameResized (float width, float height)
{
	SView::FrameResized (width, height);
}

void CanvasView::setScale (float s, bool invalidate)
{
	BRect frame;
//	frame.right = (fCanvasFrame.right + 1)*s - 1;
//	frame.bottom = (fCanvasFrame.bottom + 1)*s - 1;
	frame.right = fCanvasFrame.right*s;
	frame.bottom = fCanvasFrame.bottom*s;
	fScale = s;
	ResizeTo (frame.Width(), frame.Height());
//	frame.PrintToStream();
	SetScale (s);
	fBGView->setScale (s);		// For the scrollbars and background
	inherited::setScale (s);	// For the mouse positions et al.
//	inherited::FrameResized (Bounds().Width()*s, Bounds().Height()*s);
	BMessage *msg = new BMessage ('Crsd');
	msg->AddFloat ("width", frame.Width());
	msg->AddFloat ("height", frame.Height());
	msg->AddFloat ("scale", s);
	Window()->PostMessage (msg);
	delete msg;
	if (invalidate)
		Invalidate();
}

void CanvasView::ZoomIn ()
{
	BPoint c1, c2;
	uint32 dummy;
	GetMouse (&c1, &dummy, false);

	float cs = getScale();
	if (cs < 1)
		setScale (1/(1/cs - 1), false);
	else
		setScale (cs + 1, false);

	if (mouse_on_canvas)
	{
		cs = getScale();
		GetMouse (&c2, &dummy, false);
		fBGView->ScrollBy ((c1.x - c2.x)*cs, (c1.y - c2.y)*cs);
	}
	Invalidate();
}

void CanvasView::ZoomOut ()
{
	BPoint c1, c2;
	uint32 dummy;
	GetMouse (&c1, &dummy, false);

	float cs = getScale();
	if (cs <= 1)
		setScale (1/(1/cs + 1), false);
	else
		setScale (cs - 1, false);

	if (mouse_on_canvas)
	{
		cs = getScale();
		GetMouse (&c2, &dummy, false);
		fBGView->ScrollBy ((c1.x - c2.x)*cs, (c1.y - c2.y)*cs);
	}
	Invalidate();
}

rgb_color CanvasView::getColor (long x, long y)
{
	uint32 *addr = (uint32 *) currentLayer()->Bits() + currentLayer()->BytesPerRow()/4*y + x;
	return (bgra2rgb (*addr));
}

rgb_color CanvasView::getColor (BPoint p)
{
	if (Bounds().Contains (p))
		return (getColor (long (p.x), long (p.y)));
	else
		return (Black);
}

rgb_color CanvasView::getColor (LPoint p)
{
	uint32 *addr = (uint32 *) bbits + bbpr/4*p.y + p.x;
	return (bgra2rgb (*addr));
}

uint32 CanvasView::get_and_plot (BPoint p, rgb_color c)
{
	if (Bounds().Contains (p))
	{
		uint32 *addr = (uint32 *) currentLayer()->Bits() + currentLayer()->BytesPerRow()/4*long (p.y + 0.5) + long (p.x + 0.5);
		uint32 pix = *addr;
		*addr = PIXEL (c.red, c.green, c.blue, c.alpha);
		changed = true;
		return pix;
	}
	else
		return 0;
}

void CanvasView::plot (BPoint p, rgb_color c)
{
	if (Bounds().Contains (p))
	{
		uint32 *addr = (uint32 *) currentLayer()->Bits() + currentLayer()->BytesPerRow()/4*long (p.y + 0.5) + long (p.x + 0.5);
		*addr = PIXEL (c.red, c.green, c.blue, c.alpha);
		changed = true;
	}
}

void CanvasView::plot_alpha (BPoint p, rgb_color c)
{
	// Needs:
	//	 sbptr -> pointer to currentLayer()->Bits();
	//   sblpr -> currentLayer()->BytesPerRow()/4;
	if (Bounds().Contains (p))
	{
		uint32 *addr = sbptr + sblpr*long (p.y + 0.5) + long (p.x + 0.5);
		uint32 pixel = *addr;
		int sa = c.alpha;
		int da = 255 - sa;
		*addr = PIXEL ((RED(pixel)*da + c.red*sa)/255,
					   (GREEN(pixel)*da + c.green*sa)/255,
					   (BLUE(pixel)*da + c.blue*sa)/255,
					   clipchar (int (ALPHA(pixel)) + int (c.alpha)));
		changed = true;
	}
}

void CanvasView::fplot (BPoint p, rgb_color c)
{
	// Needs:
	//	 sbptr -> pointer to currentLayer()->Bits();
	//   sblpr -> currentLayer()->BytesPerRow()/4;
	if (Bounds().Contains (p))
	{
		uint32 *addr = sbptr + sblpr*long (p.y + 0.5) + long (p.x + 0.5);
		*addr = PIXEL (c.red, c.green, c.blue, c.alpha);
		changed = true;
	}
}

void CanvasView::fplot_alpha (BPoint p, rgb_color c)
{
	// Needs:
	//	 sbptr -> pointer to currentLayer()->Bits();
	//   sblpr -> currentLayer()->BytesPerRow()/4;
	if (Bounds().Contains (p))
	{
		uint32 *addr = sbptr + sblpr*long (p.y + 0.5) + long (p.x + 0.5);
//		uint32 *addr = (uint32 *) currentLayer()->Bits() + currentLayer()->BytesPerRow()/4*long (p.y + 0.5) + long (p.x + 0.5);
		uint32 pixel = *addr;
		int sa = c.alpha;
		int da = 255 - sa;
		*addr = PIXEL ((RED(pixel)*da + c.red*sa)/255,
					   (GREEN(pixel)*da + c.green*sa)/255,
					   (BLUE(pixel)*da + c.blue*sa)/255,
					   clipchar (int (ALPHA(pixel)) + int (c.alpha)));
		changed = true;
	}
}

/* This one is for when Be fixes B_TRANSPARENT_32_BIT... */
void CanvasView::tplot32 (BPoint p, rgb_color c)
{
	if (temp->Bounds().Contains (p))
	{
		uint32 *addr = (uint32 *) temp->Bits() + temp->BytesPerRow()/4*long (p.y) + long (p.x);
		*addr = PIXEL (c.red, c.green, c.blue, c.alpha);
	}
}

void CanvasView::tplot8 (LPoint p, uchar c)
{
//	if (temp->Bounds().Contains (p))	// Warning! No check!
//	{
		uchar *addr = tbits + tbpr*p.y + p.x;
		*addr = c;
//	}
}

void CanvasView::AttachCurrentLayer ()
{
	extern bool inPaste;
	extern BBitmap *clip;
	//printf ("Attaching current layer\n");
	currentLayer()->Lock();
	currentLayer()->AddChild (drawView);
	if (inPaste)
	{
		BPoint point;
		uint32 buttons;
		extern long pasteX, pasteY;
		GetMouse (&point, &buttons, true);
		cutbg->Lock();
		cutView->DrawBitmap (currentLayer(), BPoint (-point.x - pasteX, -point.y - pasteY));
		cutbg->Unlock();
		prevPaste = BRect (point.x + pasteX, point.y + pasteY,
			 point.x + pasteX + clip->Bounds().right, point.y + pasteY + clip->Bounds().bottom);
		ePasteM (point);
	}
	currentLayer()->Unlock();
}

void CanvasView::DetachCurrentLayer ()
{
	extern bool inPaste;
	currentLayer()->Lock();
//	printf ("Detaching current layer\n");
	if (inPaste)
	{
//		printf ("In paste!\n");
		drawView->DrawBitmap (cutbg, prevPaste.LeftTop());
	}
	currentLayer()->RemoveChild (drawView);
	currentLayer()->Unlock();
}

void CanvasView::AttachSelection ()
{
	selection->Lock();
	selection->AddChild (selectionView);
	selection->Unlock();
}

void CanvasView::DetachSelection ()
{
	selection->Lock();
	selection->RemoveChild (selectionView);
	selection->Unlock();
}

void CanvasView::addLayer (const char *name)
{
	DetachCurrentLayer();
	layer[fNumLayers] = new Layer (fCanvasFrame, name);
	fCurrentLayer = fNumLayers;
	AttachCurrentLayer();
	fNumLayers++;
	BMessage *msg = new BMessage ('lChg');
	if (myWindow->LockWithTimeout (100000) == B_OK)
	{
		myWindow->PostMessage (msg);
		myWindow->Unlock();
	}
	else
	 	printf ("1. Couldn't get lock\n");
	delete msg;
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
//	UndoSelection();	// This is debatable.
	changed = true;
	checkMetaLayer();
	Invalidate();
}

void CanvasView::insertLayer (int index, const char *name)
{
	DetachCurrentLayer();
	for (int i = fNumLayers; i > index; i--)
	{
		layer[i] = layer[i - 1];
	}
	fNumLayers++;
	layer[index] = new Layer (fCanvasFrame, name);
	fCurrentLayer = index;
	AttachCurrentLayer();
	BMessage *msg = new BMessage ('lChg');
	myWindow->Lock();
	myWindow->PostMessage (msg);
	myWindow->Unlock();
	delete msg;
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
//	UndoSelection();
	checkMetaLayer();
	changed = true;
	Invalidate();
}

void CanvasView::duplicateLayer (int index)
{
	char name[1024];	// Should be a nice #define.
	strcpy (name, layer[index]->getName());
	strcat (name, lstring (163, " copy"));
	DetachCurrentLayer();
	for (int i = fNumLayers; i > index; i--)
		layer[i] = layer[i - 1];
	fNumLayers++;
	layer[index + 1] = new Layer (fCanvasFrame, name);
	fCurrentLayer = index + 1;
	memcpy (currentLayer()->Bits(), layer[index]->Bits(), layer[index]->BitsLength());
	AttachCurrentLayer();
 	BMessage *msg = new BMessage ('lChg');
 	myWindow->Lock();
	myWindow->PostMessage (msg);
	myWindow->Unlock();
	delete msg;
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
//	UndoSelection();
	checkMetaLayer();
	changed = true;
	Invalidate();
}

void CanvasView::translateLayer (int index)
{
	extern Becasso *mainapp;
	mainapp->setMover();
	fTranslating = index + 1;
	fRotating = 0;
}

void CanvasView::rotateLayer (int index)
{
	extern Becasso *mainapp;
	mainapp->setRotator();
	fRotating = index + 1;
	fTranslating = 0;
}

void CanvasView::do_translateLayer (int /*index*/, int32 mode)
{
	extern Becasso *mainapp;
	SetupUndo (mode);
	uint32 buttons;
	BPoint point, origin, prev;
	GetMouse (&point, &buttons);
	prev = point;
	origin = point;
	if (mode == M_DRAW)
	{
		while (buttons)
		{
			if (prev != point)
			{
				currentLayer()->ClearTo (0);
				BPoint p = BPoint (point.x - origin.x, point.y - origin.y);
				// drawView->DrawBitmap (undo[indexUndo].bitmap, p);
				SBitmapToLayer (undo[indexUndo].bitmap, currentLayer(), p);
				mainapp->setMover();
				Invalidate();
				prev = point;
			}
			snooze (20000);
			GetMouse (&point, &buttons);
		}
	}
	else
	{
		while (buttons)
		{
			if (prev != point)
			{
				selection->ClearTo (0);
				BPoint p = BPoint (point.x - origin.x, point.y - origin.y);
				// selectionView->DrawSBitmap (undo[indexUndo].sbitmap, p);
				SBitmapToSelection (undo[indexUndo].sbitmap, selection, p);
				mainapp->setMover();
				Invalidate();
				prev = point;
			}
			snooze (20000);
			GetMouse (&point, &buttons);
		}
	}
}

void CanvasView::do_rotateLayer (int /*index*/, int32 mode)
{
	extern Becasso *mainapp;
	SetupUndo (mode);
	uint32 buttons;
	BPoint point, origin, prev;
	GetMouse (&point, &buttons);
	prev = point;
	origin = point;
	float angle = 0;
	float dangle;
	if (mode == M_DRAW)
	{
		while (buttons)
		{
			if (prev != point)
			{
				angle = atan2 (origin.y - point.y, point.x - origin.x);
				dangle = angle*180/M_PI;
				currentLayer()->ClearTo (0);
				Rotate (undo[indexUndo].bitmap, currentLayer(), origin, angle, false);
				// printf ("angle = %f°\n", dangle);
				mainapp->setRotator();
				Invalidate();
				SetDrawingMode (B_OP_INVERT);
				SetPenSize (0);
				StrokeLine (BPoint (Bounds().right, origin.y), origin);
				StrokeLine (point);
				StrokeArc (origin, 10, 10, 0, dangle);
				char angletxt[16];
				sprintf (angletxt, "%.1f°", dangle);
				DrawString (angletxt, BPoint (origin.x + 15, origin.y + (angle > 0 ? 10 : -2)));
				prev = point;
			}
			snooze (20000);
			GetMouse (&point, &buttons);
		}
		mainapp->setBusy();
		Rotate (undo[indexUndo].bitmap, currentLayer(), origin, angle, true);
	}
	else
	{
		while (buttons)
		{
			if (prev != point)
			{
				angle = atan2 (origin.y - point.y, point.x - origin.x);
				dangle = angle*180/M_PI;
				selection->ClearTo (0);
				Rotate (undo[indexUndo].sbitmap, selection, origin, angle, false);
				// printf ("angle = %f°\n", dangle);
				mainapp->setRotator();
				Invalidate();
				SetDrawingMode (B_OP_INVERT);
				SetPenSize (0);
				StrokeLine (BPoint (Bounds().right, origin.y), origin);
				StrokeLine (point);
				StrokeArc (origin, 10, 10, 0, dangle);
				char angletxt[16];
				sprintf (angletxt, "%.1f°", dangle);
				DrawString (angletxt, BPoint (origin.x + 15, origin.y + (angle > 0 ? 10 : -2)));
				prev = point;
			}
			snooze (20000);
			GetMouse (&point, &buttons);
		}
		mainapp->setBusy();
		Rotate (undo[indexUndo].sbitmap, selection, origin, angle, true);
	}
	mainapp->setReady();
	Invalidate();
}

void CanvasView::flipLayer (int hv, int index)
{
	extern PicMenuButton *mode;
	int m = mode->selected();

	SetupUndo (m);

	if (m == M_DRAW)
	{
		if (index == -1)
			index = fCurrentLayer;

		Layer *l = layer[index];
		bgra_pixel *p = (bgra_pixel *) l->Bits();
		uint32 ppr = l->BytesPerRow()/4;
		int32 w = fCanvasFrame.IntegerWidth();
		int32 h = fCanvasFrame.IntegerHeight();
		if (hv == 0)	// horizontal flip
		{
			for (int32 y = 0; y <= h; y++)
			{
				bgra_pixel *lp = p + y*ppr;
				bgra_pixel *ilp = p + y*ppr + w;
				for (int32 x = 0; x <= w/2; x++)
				{
					bgra_pixel tmp = *lp;
					*lp++ = *ilp;
					*ilp-- = tmp;
				}
			}
		}
		else			// vertical flip
		{
			for (int32 x = 0; x <= w; x++)
			{
				bgra_pixel *lp = p + x;
				bgra_pixel *ilp = p + h*ppr + x;
				for (int32 y = 0; y <= h/2; y++)
				{
					bgra_pixel tmp = *lp;
					*lp = *ilp;
					*ilp = tmp;
					lp += ppr;
					ilp -= ppr;
				}
			}
		}
	}
	else
	{
		grey_pixel *p = (grey_pixel *) selection->Bits();
		uint32 ppr = selection->BytesPerRow();
		int32 w = fCanvasFrame.IntegerWidth();
		int32 h = fCanvasFrame.IntegerHeight();
		if (hv == 0)	// horizontal flip
		{
			for (int32 y = 0; y <= h; y++)
			{
				grey_pixel *lp = p + y*ppr;
				grey_pixel *ilp = p + y*ppr + w;
				for (int32 x = 0; x < w/2; x++)
				{
					grey_pixel tmp = *lp;
					*lp++ = *ilp;
					*ilp-- = tmp;
				}
			}
		}
		else			// vertical flip
		{
			for (int32 x = 0; x <= w; x++)
			{
				grey_pixel *lp = p + x;
				grey_pixel *ilp = p + h*ppr + x;
				for (int32 y = 0; y < h/2; y++)
				{
					grey_pixel tmp = *lp;
					*lp = *ilp;
					*ilp = tmp;
					lp += ppr;
					ilp -= ppr;
				}
			}
		}
	}
	Invalidate();
}

void CanvasView::moveLayers (int from, int to)
{
	if (from == to)
	{
		makeCurrentLayer (from);
		return;
	}
	if (to < 0)
		to = 0;
	if (to >= fNumLayers)
		to = fNumLayers - 1;
	DetachCurrentLayer();
	if (from < to)
	{
		Layer *tmp = layer[from];
		for (int i = from; i < to; i++)
			layer[i] = layer[i + 1];
		layer[to] = tmp;
	}
	else
	{
		Layer *tmp = layer[from];
		for (int i = from; i > to; i--)
			layer[i] = layer[i - 1];
		layer[to] = tmp;
	}
	fCurrentLayer = to;
	AttachCurrentLayer();
	BMessage msg ('lChg');
	myWindow->Lock();
	myWindow->PostMessage (&msg);
	myWindow->Unlock();
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
//	UndoSelection();
	checkMetaLayer();
	changed = true;
	Invalidate();
}

void CanvasView::removeLayer (int index)
{
	if (!index)		// Don't remove the background layer!
		return;
	extern int DebugLevel;
	if (DebugLevel)
		printf ("Removing layer %i\n", index);

	snooze (20000);	// ahem.
	fIndex2 = index;
	SetupUndo (M_DELLAYER);
	fIndex2 = -1;
	do_removeLayer (index);
	changed = true;
	Invalidate();
}

void CanvasView::do_removeLayer (int index)
{
///////////////////////////
//
	snooze (100000);
//
//	Ahem.  This means there's some deadlock.  If I remove this,
//  sometimes the LayerWindow would crash (it would probably be redrawing its
//  thumbnail view when CanvasView was already deleting it...)
//
////////////////////////////

	if (index >= fCurrentLayer)
	{
//		printf ("Detaching current layer\n"); snooze (20000);
		DetachCurrentLayer();
	}
//	printf ("Moving layers\n"); snooze (20000);
	delete layer[index];
	for (int i = index; i < fNumLayers - 1; i++)
	{
//		printf ("%i <- %i\n", i, i + 1); snooze (20000);
		layer[i] = layer[i + 1];
	}
	fNumLayers--;
	layer[fNumLayers] = NULL;
	if (index >= fCurrentLayer)
	{
		fCurrentLayer--;
//		printf ("Huu\n"); snooze (20000);
		AttachCurrentLayer();
//		printf ("Current Layer attached.\n");
		snooze (20000);
	}
	else
	{
//		printf ("Boe\n"); snooze (20000);
		//fCurrentLayer--;
		//UndoSelection();
	}
	BMessage *msg = new BMessage ('lChg');
	myWindow->Lock();
	myWindow->PostMessage (msg);
	myWindow->Unlock();
	delete msg;
//	printf ("Almost done...\n"); snooze (20000);
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
	checkMetaLayer();
}

void CanvasView::makeCurrentLayer (int index)
{
	if (index >= 0 && index < fNumLayers)
	{
		DetachCurrentLayer();
		BMessage msg ('lChg');
		msg.AddInt32 ("prev", fCurrentLayer);
		msg.AddInt32 ("current", index);
		fCurrentLayer = index;
		AttachCurrentLayer();
		myWindow->Lock();
		myWindow->PostMessage (&msg);
		myWindow->Unlock();
		if (myWindow->isLayerOpen())
		{
			myWindow->layerWindow->Lock();
			myWindow->layerWindow->doChanges (index);
			myWindow->layerWindow->Unlock();
		}
//		printf ("Hey!\n");
//		UndoSelection();
		checkMetaLayer();
		changed = true;
		Invalidate();
	}
	else
		fprintf (stderr, "Becasso: Layer index %d out of range\n", index);
}

void CanvasView::checkMetaLayer ()
{
	myWindow->posview->SetTextLayer (false);
}

void CanvasView::mergeLayers (int bottom, int top)
{
	fIndex1 = bottom;
	fIndex2 = top;
	SetupUndo (M_MERGE);
	Merge (layer[bottom], layer[top], fCanvasFrame, false);
	do_removeLayer (top);
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
	fIndex1 = -1;
	fIndex2 = -1;
	changed = true;
	checkMetaLayer();
	Invalidate();
}

void CanvasView::setChannelOperation (int index, int newmode)
{
	layer[index]->setMode (newmode);
	changed = true;
	Invalidate();
}

void CanvasView::setGlobalAlpha (int index, int ga)
{
//	printf ("setGlobalAlpha of %d to %d\n", index, ga);
	layer[index]->Lock();
//	printf ("Locked... "); fflush (stdout);
	layer[index]->setGlobalAlpha (ga);
//	printf ("Set... "); fflush (stdout);
	layer[index]->Unlock();
//	printf ("Done\n");
	changed = true;
	Invalidate();
}

void CanvasView::setLayerContentsToBitmap (int index, BBitmap *map, int32 resize)
{
	clock_t start, end;
	start = clock();
	int previousCurrentLayer = fCurrentLayer;
	makeCurrentLayer (index);
	currentLayer()->Lock();
	SetupUndo (M_DRAW);
	if (map->Bounds() != fCanvasFrame && resize == STRETCH_TO_FIT)
	{
		verbose (1, "Stretching bitmap to fit\n");
		::Scale (map, NULL, currentLayer(), NULL);
	}
	else
	{
		drawView->DrawBitmap (map, B_ORIGIN);
	}
	currentLayer()->Unlock();
	makeCurrentLayer (previousCurrentLayer);
	changed = true;
	checkMetaLayer();
	Invalidate();
	end = clock();
	extern int DebugLevel;
	if (DebugLevel > 2)
		printf ("CanvasView::setLayerContentsToBitmap Took %ld ms\n", end - start);
}

void CanvasView::invertAlpha ()
{
//	printf ("Inverting alpha channel...\n");
	currentLayer()->Lock();
	InvertAlpha (currentLayer());
	currentLayer()->Unlock();
	Invalidate();
}

void CanvasView::CenterMouse ()
{
	BPoint sp = ConvertToScreen (BPoint (fLastCenter.x*fScale, fLastCenter.y*fScale));
	set_mouse_position (int (sp.x), int (sp.y));
}

void CanvasView::CropToWindow ()
{
	BRect rect (Parent()->Bounds());
	rect.left = int (rect.left/fScale + 0.5);
	rect.top = int (rect.top/fScale + 0.5);
	rect.right = int (rect.right/fScale + 0.5);
	rect.bottom = int (rect.bottom/fScale + 0.5);
	Crop (rect);
}

void CanvasView::CropToSelection ()
{
	BRect rect = GetSmallestRect (selection);
	Crop (rect);
}

void CanvasView::AutoCrop ()
{
	currentLayer()->Lock();
	rgb_color border = GuessBackgroundColor();
//	printf ("%d, %d, %d, %d\n", border.red, border.green, border.blue, border.alpha);
	uint32 trans = PIXEL (border.red, border.green, border.blue, border.alpha);
	//(border.blue << 24) | (border.green << 16) | (border.red << 8) | border.alpha;
//	printf ("%lx\n", trans);

	uint32 *data = (uint32 *) currentLayer()->Bits();
//	long bpr = currentLayer()->BytesPerRow();
	long h = fCanvasFrame.IntegerHeight();
	long w = fCanvasFrame.IntegerWidth();
	BRect res = BRect (w, h, 0, 0);

	data--;
	for (long i = 0; i <= h; i++)
	{
		for (long j = 0; j <= w; j++)
		{
			if (*(++data) != trans)
			{
				if (i < res.top)
					res.top = i;
				if (i > res.bottom)
					res.bottom = i;
				if (j < res.left)
					res.left = j;
				if (j > res.right)
					res.right = j;
			}
		}
	}
	currentLayer()->Unlock();
	Crop (res);
}

status_t CanvasView::Crop (BRect rect)
{
	CloseOpenAddons();
	if (rect == fCanvasFrame)
		return -1;
	if (rect.bottom < 0)
		rect.bottom += fCanvasFrame.bottom;
	if (rect.right < 0)
		rect.right += fCanvasFrame.right;
	if (rect.left < 0 || rect.top < 0 || rect.bottom > fCanvasFrame.bottom || rect.right > fCanvasFrame.right)
		return -2;
	if (rect.left > fCanvasFrame.right || rect.top > fCanvasFrame.bottom)
		return -3;
	SetupUndo (M_RESIZE);
	fBGView->ScrollTo (B_ORIGIN);
	changed = true;
	BRect newrect = rect;
	newrect.OffsetTo (B_ORIGIN);
	// printf ("Detaching Current Layer\n");
	DetachCurrentLayer();
	// printf ("Resizing DrawView\n");
	drawView->ResizeTo (newrect.Width(), newrect.Height());
	for (int i = 0; i < fNumLayers; i++)
	{
		// printf ("Cropping layer %i\n", i);
		layer[i]->Lock();
		Layer *tmp = new Layer (newrect, layer[i]->getName());
		tmp->setMode (layer[i]->getMode());
		tmp->setGlobalAlpha (layer[i]->getGlobalAlpha());
		tmp->Hide (layer[i]->IsHidden());
		tmp->AddChild (drawView);
		tmp->Lock();
		drawView->SetDrawingMode (B_OP_COPY);
		drawView->DrawBitmap (layer[i], rect, newrect);
		tmp->Unlock();
		tmp->RemoveChild (drawView);
		delete layer[i];
		layer[i] = tmp;
	}
	// printf ("Attaching Current Layer\n");
	AttachCurrentLayer();
	fCanvasFrame = newrect;
	selection->Lock();
	DetachSelection();
	selectionView->ResizeTo (newrect.Width(), newrect.Height());
	Selection *tmp = new Selection (newrect);
	tmp->AddChild (selectionView);
	tmp->Lock();
	selectionView->SetDrawingMode (B_OP_COPY);
	selectionView->DrawBitmap (selection, rect, newrect);
	tmp->Unlock();
	tmp->RemoveChild (selectionView);
	delete selection;
	selection = tmp;
	AttachSelection();
	myWindow->Lock();
	if (screenbitmap->ColorSpace() != B_RGBA32)
		delete screenbitmap32;
	delete screenbitmap;
	BScreen screen;
	if (screen.ColorSpace() == B_COLOR_8_BIT)
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_COLOR_8_BIT, true);
		screenbitmap32 = new BBitmap (fCanvasFrame, B_RGBA32, true);
	}
	else	// 32 or 16 bit screens
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_RGBA32, true);
		screenbitmap32 = screenbitmap;
	}
	ResizeTo ((rect.Width())*fScale, (rect.Height())*fScale);
	myWindow->FrameResized (myWindow->Bounds().Width(), myWindow->Bounds().Height());
	fBGView->setFrame (newrect);
	// hack to kick the BGView into repositioning
	fBGView->FrameResized ((rect.Width())*fScale + 1, (rect.Height())*fScale + 1);
	fBGView->FrameResized ((rect.Width())*fScale, (rect.Height())*fScale);
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
	Invalidate();
	myWindow->UpdateIfNeeded();
	myWindow->Unlock();
	return B_OK;
}

void CanvasView::Pad (uint32 what)
// 'pwlt', 'pwrt', 'pwlb', 'pwrb', 'pwct'
{
	CloseOpenAddons();
	SetupUndo (M_RESIZE);
	BRect rect = Parent()->Bounds();
	rect.left = int (rect.left/fScale + 0.5);
	rect.top = int (rect.top/fScale + 0.5);
	rect.right = int (rect.right/fScale + 0.5);
	rect.bottom = int (rect.bottom/fScale + 0.5);
	changed = true;
	BRect newrect = rect;
	newrect.OffsetTo (B_ORIGIN);
	DetachCurrentLayer();
	drawView->ResizeTo (newrect.Width(), newrect.Height());
	BPoint pos;
	float wd = rect.Width() - fCanvasFrame.Width();
	float hd = rect.Height() - fCanvasFrame.Height();
	switch (what)
	{
	case 'pwlt':
		pos = B_ORIGIN;
		break;
	case 'pwrt':
		pos = BPoint (wd, 0);
		break;
	case 'pwlb':
		pos = BPoint (0, hd);
		break;
	case 'pwrb':
		pos = BPoint (wd, hd);
		break;
	case 'pwct':
		pos = BPoint (wd/2, hd/2);
		break;
	default:
		fprintf (stderr, "CanvasView::Pad: Invalid position\n");
		pos = B_ORIGIN;
	}
	for (int i = 0; i < fNumLayers; i++)
	{
		layer[i]->Lock();
		Layer *tmp = new Layer (newrect, layer[i]->getName());
		tmp->AddChild (drawView);
		tmp->Lock();
		tmp->setMode (layer[i]->getMode());
		tmp->setGlobalAlpha (layer[i]->getGlobalAlpha());
		tmp->Hide (layer[i]->IsHidden());
		drawView->SetDrawingMode (B_OP_COPY);
		drawView->DrawBitmap (layer[i], pos);
		drawView->Sync();
		tmp->Unlock();
		tmp->RemoveChild (drawView);
		delete layer[i];
		layer[i] = tmp;
	}
	AttachCurrentLayer();
	fCanvasFrame = newrect;
	DetachSelection();
	selection->Lock();
	selectionView->ResizeTo (newrect.Width(), newrect.Height());
	Selection *tmp = new Selection (newrect);
	tmp->AddChild (selectionView);
	tmp->Lock();
	selectionView->SetDrawingMode (B_OP_COPY);
	selectionView->DrawBitmap (selection, pos);
	tmp->Unlock();
	tmp->RemoveChild (selectionView);
	delete selection;
	selection = tmp;
	AttachSelection();
	myWindow->Lock();
	if (screenbitmap->ColorSpace() != B_RGBA32)
		delete screenbitmap32;
	delete screenbitmap;
	BScreen screen;
	if (screen.ColorSpace() == B_COLOR_8_BIT)
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_COLOR_8_BIT, true);
		screenbitmap32 = new BBitmap (fCanvasFrame, B_RGBA32, true);
	}
	else	// 32 or 16 bit screens
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_RGBA32, true);
		screenbitmap32 = screenbitmap;
	}
//	screenbitmap = new BBitmap (fCanvasFrame, screen.ColorSpace(), true);
//	totaldatalen = (fCanvasFrame.IntegerWidth() + 1)*(fCanvasFrame.IntegerHeight() + 1)*3;
//	delete [] totaldata;
//	totaldata = new uchar [totaldatalen];
	ResizeTo (rect.Width()*fScale, rect.Height()*fScale);
	fBGView->setFrame (newrect);
	ScrollTo (B_ORIGIN);
	myWindow->FrameResized (myWindow->Bounds().Width(), myWindow->Bounds().Height());
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
	myWindow->Unlock();
	Invalidate();
}

void CanvasView::resizeTo (int32 w, int32 h)
{
	CloseOpenAddons();
	SetupUndo (M_RESIZE);
	BRect newrect = BRect (0, 0, w - 1, h - 1);
	changed = true;
	DetachCurrentLayer();
	drawView->ResizeTo (newrect.Width(), newrect.Height());
	for (int i = 0; i < fNumLayers; i++)
	{
		layer[i]->Lock();
		Layer *tmp = new Layer (newrect, layer[i]->getName());
		tmp->setMode (layer[i]->getMode());
		tmp->setGlobalAlpha (layer[i]->getGlobalAlpha());
		tmp->Hide (layer[i]->IsHidden());
		tmp->AddChild (drawView);
		tmp->Lock();
		drawView->SetDrawingMode (B_OP_COPY);
		::Scale (layer[i], NULL, tmp, NULL);
		// Ugly scaling...
		//drawView->DrawBitmap (layer[i], layer[i]->Bounds(), newrect);
		drawView->Sync();
		tmp->Unlock();
		tmp->RemoveChild (drawView);
		delete layer[i];
		layer[i] = tmp;
	}
	AttachCurrentLayer();
	fCanvasFrame = newrect;
	DetachSelection();
	selection->Lock();
	selectionView->ResizeTo (newrect.Width(), newrect.Height());
	Selection *newselection = new Selection (fCanvasFrame);
	::Scale (NULL, selection, NULL, newselection);
	delete selection;
	selection = newselection;
	AttachSelection();
	myWindow->Lock();
	if (screenbitmap->ColorSpace() != B_RGBA32)
		delete screenbitmap32;
	delete screenbitmap;
	BScreen screen;
	if (screen.ColorSpace() == B_COLOR_8_BIT)
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_COLOR_8_BIT, true);
		screenbitmap32 = new BBitmap (fCanvasFrame, B_RGBA32, true);
	}
	else	// 32 or 16 bit screens
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_RGBA32, true);
		screenbitmap32 = screenbitmap;
	}
	ResizeTo (newrect.Width()*fScale, newrect.Height()*fScale);
	fBGView->setFrame (newrect);
	ScrollTo (B_ORIGIN);
	myWindow->FrameResized (myWindow->Bounds().Width(), myWindow->Bounds().Height());
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
	myWindow->Unlock();
	Invalidate();
}

void CanvasView::ResizeToWindow (uint32 what)
// 'rwkr', 'rwar'
{
	BRect rect = Parent()->Bounds();
	rect.left = int (rect.left/fScale + 0.5);
	rect.top = int (rect.top/fScale + 0.5);
	rect.right = int (rect.right/fScale + 0.5);
	rect.bottom = int (rect.bottom/fScale + 0.5);
	changed = true;
	BRect newrect = rect;
	newrect.OffsetTo (B_ORIGIN);
	switch (what)
	{
	case 'rwkr':
	{
		float ratio = Bounds().Width() / Bounds().Height();
		if (rect.Height()*ratio > newrect.Width())
		{
			newrect.bottom = int (rect.right/ratio);
		}
		else
		{
			newrect.right = int (rect.bottom*ratio);
		}
	}
	case 'rwar':
		break;
	default:
		fprintf (stderr, "CanvasView::ResizeToWindow: Invalid ratio mode\n");
	}
	resizeTo (newrect.IntegerWidth() + 1, newrect.IntegerHeight() + 1);
}

void CanvasView::RotateCanvas (uint32 what)
// 'r90', 'r180', 'r270'
{
	CloseOpenAddons();
	// This is so easy to undo, we shouldn't waste buffers on it...
//	SetupUndo (M_RESIZE);
	changed = true;
	BRect rect = Bounds();
	BRect newrect = Bounds();
	rect.left /= fScale;
	rect.right /= fScale;
	rect.top /= fScale;
	rect.bottom /= fScale;
	newrect.left /= fScale;
	newrect.right /= fScale;
	newrect.top /= fScale;
	newrect.bottom /= fScale;
	DetachCurrentLayer();
	switch (what)
	{
	case 'r90':
	case 'r270':
	{
		float tmp = newrect.right;
		newrect.right = newrect.bottom;
		newrect.bottom = tmp;
		break;
	}
	case 'r180':
		break;
	default:
		fprintf (stderr, "CanvasView::RotateCanvas: Invalid angle\n");
	}
	drawView->ResizeTo (newrect.Width(), newrect.Height());
	for (int i = 0; i < fNumLayers; i++)
	{
		layer[i]->Lock();
		Layer *tmp = new Layer (newrect, layer[i]->getName());
		tmp->setMode (layer[i]->getMode());
		tmp->setGlobalAlpha (layer[i]->getGlobalAlpha());
		tmp->Hide (layer[i]->IsHidden());
		tmp->Lock();
		uint32 *src = (uint32 *) layer[i]->Bits();
		uint32 *dest = (uint32 *) tmp->Bits();
		int w = layer[i]->Bounds().IntegerWidth() + 1;
		int h = layer[i]->Bounds().IntegerHeight() + 1;
		uint32 bpr = layer[i]->BytesPerRow()/4;
		switch (what)
		{
		case 'r90':
			dest--;
			for (int y = 0; y < w; y++)
			{
				src = (uint32 *) layer[i]->Bits() + bpr - y - 1;
				for (int x = 0; x < h; x++)
				{
					*(++dest) = *src;
					src += bpr;
				}
			}
			break;
		case 'r180':
			src--;
			dest += tmp->BitsLength()/4;
			for (int y = 0; y < h; y++)
				for (int x = 0; x < w; x++)
					*(--dest) = *(++src);
			break;
		case 'r270':
			dest--;
			src += bpr - 1;
			for (int y = 0; y < w; y++)
			{
				src = (uint32 *) layer[i]->Bits() + layer[i]->BitsLength()/4 - bpr + y;
				for (int x = 0; x < h; x++)
				{
					*(++dest) = *src;
					src -= bpr;
				}
			}
			break;
		default:
			fprintf (stderr, "CanvasView::RotateCanvas: Invalid angle\n");
		}
		tmp->Unlock();
		delete layer[i];
		layer[i] = tmp;
	}
	AttachCurrentLayer();
	fCanvasFrame = newrect;
	DetachSelection();
	selection->Lock();
	selectionView->ResizeTo (newrect.Width(), newrect.Height());
	Selection *tmp = new Selection (fCanvasFrame);
	tmp->Lock();
	grey_pixel *src = (grey_pixel *) selection->Bits();
	grey_pixel *dest = (grey_pixel *) tmp->Bits();
	int w = selection->Bounds().IntegerWidth() + 1;
	int h = selection->Bounds().IntegerHeight() + 1;
	uint32 sbpr = selection->BytesPerRow();
	uint32 dbpr = tmp->BytesPerRow();
	switch (what)
	{
	case 'r90':
	{
		int ddif = dbpr - h;
		dest--;
		for (int y = 0; y < w; y++)
		{
			src = (grey_pixel *) selection->Bits() + w - y - 1;
			for (int x = 0; x < h; x++)
			{
				*(++dest) = *src;
				src += sbpr;
			}
			dest += ddif;
		}
		break;
	}
	case 'r180':
	{
		int ddif = dbpr - w;
		int sdif = sbpr - w;
		src--;
		dest += tmp->BitsLength() - ddif + 1;
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				*(--dest) = *(++src);
			}
			dest -= ddif;
			src  += sdif;
		}
		break;
	}
	case 'r270':
	{
		int ddif = dbpr - h;
		dest--;
		src += w - 1;
		for (int y = 0; y < w; y++)
		{
			src = (grey_pixel *) selection->Bits() + selection->BitsLength() - sbpr + y;
			for (int x = 0; x < h; x++)
			{
				*(++dest) = *src;
				src -= sbpr;
			}
			dest += ddif;
		}
		break;
	}
	default:
		fprintf (stderr, "CanvasView::RotateCanvas: Invalid angle\n");
	}
	tmp->Unlock();
	delete selection;
	selection = tmp;
	AttachSelection();
	myWindow->Lock();
	if (screenbitmap->ColorSpace() != B_RGBA32)
		delete screenbitmap32;
	delete screenbitmap;
	BScreen screen;
	if (screen.ColorSpace() == B_COLOR_8_BIT)
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_COLOR_8_BIT, true);
		screenbitmap32 = new BBitmap (fCanvasFrame, B_RGBA32, true);
	}
	else	// 32 or 16 bit screens
	{
		screenbitmap = new BBitmap (fCanvasFrame, B_RGBA32, true);
		screenbitmap32 = screenbitmap;
	}
	ResizeTo (newrect.Width()*fScale, newrect.Height()*fScale);
	fBGView->setFrame (newrect);
	ScrollTo (B_ORIGIN);
	myWindow->FrameResized (myWindow->Bounds().Width(), myWindow->Bounds().Height());
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges();
		myWindow->layerWindow->Unlock();
	}
	myWindow->Unlock();
	Invalidate();
}

void CanvasView::ReplaceCurrentLayer (BBitmap *newlayer)
{
	if (newlayer->Bounds() != fCanvasFrame)
	{
		CloseOpenAddons();
		SetupUndo (M_RESIZE);
		BRect rect = newlayer->Bounds();
		rect.left = int (rect.left/fScale + 0.5);
		rect.top = int (rect.top/fScale + 0.5);
		rect.right = int (rect.right/fScale + 0.5);
		rect.bottom = int (rect.bottom/fScale + 0.5);
		changed = true;
		BRect newrect = rect;
		newrect.OffsetTo (B_ORIGIN);
		DetachCurrentLayer();
		drawView->ResizeTo (newrect.Width(), newrect.Height());
		for (int i = 0; i < fNumLayers; i++)
		{
			layer[i]->Lock();
			delete layer[i];
			layer[i] = new Layer (newrect, layer[i]->getName());
		}
		AttachCurrentLayer();
		fCanvasFrame = newrect;
		DetachSelection();
		selection->Lock();
		selectionView->ResizeTo (newrect.Width(), newrect.Height());
		delete selection;
		selection = new Selection (fCanvasFrame);
		AttachSelection();
		myWindow->Lock();
		if (screenbitmap->ColorSpace() != B_RGBA32)
			delete screenbitmap32;
		delete screenbitmap;
		BScreen screen;
		if (screen.ColorSpace() == B_COLOR_8_BIT)
		{
			screenbitmap = new BBitmap (fCanvasFrame, B_COLOR_8_BIT, true);
			screenbitmap32 = new BBitmap (fCanvasFrame, B_RGBA32, true);
		}
		else	// 32 or 16 bit screens
		{
			screenbitmap = new BBitmap (fCanvasFrame, B_RGBA32, true);
			screenbitmap32 = screenbitmap;
		}
		ResizeTo (newrect.Width()*fScale, newrect.Height()*fScale);
		fBGView->setFrame (newrect);
		ScrollTo (B_ORIGIN);
		myWindow->FrameResized (myWindow->Bounds().Width(), myWindow->Bounds().Height());
		if (myWindow->isLayerOpen())
		{
			myWindow->layerWindow->Lock();
			myWindow->layerWindow->doChanges();
			myWindow->layerWindow->Unlock();
		}
		myWindow->Unlock();
		Invalidate();
	}
	else
	{
		extern PicMenuButton *mode;
		SetupUndo (mode->selected());
	}
	currentLayer()->Lock();
	drawView->DrawBitmapAsync (newlayer, B_ORIGIN);
	drawView->Sync();
	currentLayer()->Unlock();
	Invalidate();
}

void CanvasView::OpenAddon (AddOn *addon, const char *name)
{
	printf ("OpenAddon\n");
//	extern PicMenuButton *mode;
	Window()->Unlock();
	switch (addon->Type())
	{
	case BECASSO_FILTER:
//		if (filterOpen && filterOpen != addon)
//		{
//			printf ("Closing filter...\n");
//			addonInQueue = addon;
//			filterOpen->Close();
//		}
//		else
			filterOpen = addon;
		break;
	case BECASSO_TRANSFORMER:
//		if (transformerOpen && transformerOpen != addon)
//		{
//			addonInQueue = addon;
//			transformerOpen->Close();
//		}
//		else
			transformerOpen = addon;
		transformerSaved = false;
		break;
	case BECASSO_GENERATOR:
//		if (generatorOpen && transformerOpen != addon)
//		{
//			addonInQueue = addon;
//			generatorOpen->Close();
//		}
//		else
			generatorOpen = addon;
		generatorSaved = false;
		break;
	default:
		fprintf (stderr, "OpenAddon: Unknown Add-on type!\n");
	}
	Window()->Lock();
	myWindow->setMenuItem ('redo', false);
	myWindow->setMenuItem (B_UNDO, false);
	addon->Open (myWindow, name);

	if (generatorOpen && !(generatorOpen->DoesPreview() & PREVIEW_MOUSE))
	{
		Generate (generatorOpen, 1);	// For stuff like the Tile add-on - immediately call
										// it one time once selected.
	}
	if (transformerOpen && !(transformerOpen->DoesPreview() & PREVIEW_MOUSE))
	{
		Transform (transformerOpen, 1);
	}

	if (addon->DoesPreview() && !(addon->DoesPreview() & PREVIEW_MOUSE))
	{
		Invalidate (fPreviewRect);
	}
}

void CanvasView::CloseAddon (AddOn *addon)
{
//	printf ("Closing Addon.\n");
//	if (addonInQueue)
//		printf ("   Addon in Queue\n");
	switch (addon->Type())
	{
	case BECASSO_FILTER:
//		if (addonInQueue)
//			filterOpen = addonInQueue;
//		else
			filterOpen = NULL;
		break;
	case BECASSO_TRANSFORMER:
//		if (addonInQueue)
//			transformerOpen = addonInQueue;
//		else
			transformerOpen = NULL;
		if (transformerSaved)
			Undo();
		transformerSaved = false;
		break;
	case BECASSO_GENERATOR:
//		if (addonInQueue)
//			generatorOpen = addonInQueue;
//		else
			generatorOpen = NULL;
		if (generatorSaved)
			Undo();
		generatorSaved = false;
		break;
	default:
		fprintf (stderr, "CloseAddon: Unknown Add-on type!\n");
	}
//	addonInQueue = NULL;
	Invalidate();
	addon->Close();
	myWindow->Lock();
	myWindow->setMenuItem (B_UNDO, indexUndo > 0);
	myWindow->setMenuItem ('redo', indexUndo + 1 < maxIndexUndo);
	myWindow->Unlock();
}

void CanvasView::CloseOpenAddons ()
{
	if (filterOpen)
		filterOpen->Close();
	if (transformerOpen)
		transformerOpen->Close();
	if (generatorOpen)
		generatorOpen->Close();
	myWindow->Lock();
	myWindow->setMenuItem (B_UNDO, indexUndo > 0);
	myWindow->setMenuItem ('redo', indexUndo + 1 < maxIndexUndo);
	myWindow->Unlock();
//	printf ("All Open Addons closed\n");
}

void CanvasView::Filter (AddOn *addon, uint8 preview)
// preview is true when the add-on signals a change in parameters
// preview is false when this is called for "the real thing".
{
//	extern PicMenuButton *mode;
	//printf ("Calling from Filter.  sel = %s, preview = %s\n", sel ? "true" : "false", preview ? "true" : "false");
	if (preview)
	{
		Invalidate (fPreviewRect);
	}
	else	// User clicked "Apply", so Do The Real Thing.
	{
		extern Becasso *mainapp;
		extern PicMenuButton *mode;
		SetupUndo (mode->selected());
		if (!preview) mainapp->setBusy();
		currentLayer()->Lock();
		selection->Lock();
		Layer *newlayer = NULL;
		Selection *newselection = NULL;
		DetachCurrentLayer();
		DetachSelection();
		BRect pRect = sel ? GetSmallestRect (selection) : fCanvasFrame;
		if (addon->Process (currentLayer(), sel ? selection : NULL,
							&newlayer, &newselection, mode->selected(), &pRect, !preview)
			!= ADDON_OK)
		{
			if (newlayer != currentLayer())
				delete newlayer;
			if (newselection != selection)
				delete newselection;
			AttachSelection();
			AttachCurrentLayer();
			selection->Unlock();
			currentLayer()->Unlock();
			Undo();
			mainapp->setHand();
			return;
		}
		selection->Unlock();
		currentLayer()->Unlock();
		if (newlayer != currentLayer() && newlayer)
		{
			currentLayer()->Lock();
			delete currentLayer();
			layer[fCurrentLayer] = newlayer;
		}
		if (newselection != selection && newselection)
		{
			selection->Lock();
			delete selection;
			selection = newselection;
		}
		AttachSelection();
		AttachCurrentLayer();
		myWindow->Lock();
		if (myWindow->isLayerOpen())
		{
			myWindow->layerWindow->Lock();
			myWindow->layerWindow->doChanges (currentLayerIndex());
			myWindow->layerWindow->Unlock();
		}
		myWindow->Unlock();
		Invalidate();
		mainapp->setHand();
		checkMetaLayer();
	}
}

void CanvasView::Transform (AddOn *addon, uint8 preview)
{
//	printf ("Transform\n");
	extern Becasso *mainapp;
	extern PicMenuButton *mode;
	static int32 prevmode = mode->selected();
	if (prevmode != mode->selected())
	{
		Undo (true);
		Invalidate();
		transformerSaved = false;
		prevmode = mode->selected();
	}
	if (!transformerSaved)	// There's no undo buffer set up yet.
	{
		SetupUndo (mode->selected());	// Save now.
		// But if we're `just' previewing, there's no need to flush the undo
		// buffer, so we shouldn't save next time we come here.
	}
	else
	{
		// We're here for a second (...) time, so restore the previous state
		// of the canvas.
		// I'm not too happy with this because it causes quite an overhead for
		// large pictures, but otherwise translators got their own previous result
		// back, which wasn't good.
		Undo (false);
	}
	transformerSaved = preview;

	if (!preview) mainapp->setBusy();
	currentLayer()->Lock();
	selection->Lock();
	Layer *newlayer = NULL;
	Selection *newselection = NULL;
	DetachCurrentLayer();
	DetachSelection();
	BRect pRect = sel ? GetSmallestRect (selection) : fCanvasFrame;
	if (addon->Process (currentLayer(), sel ? selection : NULL,
						 &newlayer, &newselection, mode->selected(), &pRect, !preview)
		!= ADDON_OK)
	{
		if (newlayer != currentLayer())
			delete newlayer;
		if (newselection != selection)
			delete newselection;
		AttachSelection();
		AttachCurrentLayer();
		selection->Unlock();
		currentLayer()->Unlock();
		Undo();
		mainapp->setHand();
		return;
	}
	selection->Unlock();
	currentLayer()->Unlock();
	if (newlayer != currentLayer() && newlayer)
	{
		currentLayer()->Lock();
		delete currentLayer();
		layer[fCurrentLayer] = newlayer;
	}
	if (newselection != selection && newselection)
	{
		selection->Lock();
		delete selection;
		selection = newselection;
		sel = true;
	}
	AttachSelection();
	AttachCurrentLayer();
	myWindow->Lock();
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges (currentLayerIndex());
		myWindow->layerWindow->Unlock();
	}
	myWindow->Unlock();
	Invalidate();
	mainapp->setHand();
}

void CanvasView::Generate (AddOn *addon, uint8 preview)
{
	extern Becasso *mainapp;
	extern PicMenuButton *mode;
	if (!generatorSaved)	// There's no undo buffer setup yet.
	{
		SetupUndo (mode->selected());	// Save now.
		// But if we're `just' previewing, there's no need to flush the undo
		// buffer, so we shouldn't save next time we come here.
	}
	else
	{
		// We're here for a second (...) time, so restore the previous state
		// of the canvas.
		// I'm not too happy with this because it causes quite an overhead for
		// large pictures, but otherwise generators that "blended" with the old
		// bitmap got their own previous result back, which wasn't good (especially
		// using the Tile Generator with a non-255 selection map).
		Undo (false);
	}
	generatorSaved = preview;

	if (!preview) mainapp->setBusy();
	currentLayer()->Lock();
	selection->Lock();
	Layer *newlayer = NULL;
	Selection *newselection = NULL;
//	Layer *newlayer = new Layer (*currentLayer());
//	Selection *newselection = new Selection (selection->Bounds());
//	newlayer->Lock();
//	newselection->Lock();
//	memcpy (newlayer->Bits(), currentLayer()->Bits(), newlayer->BitsLength());
//	memcpy (newselection->Bits(), selection->Bits(), newselection->BitsLength());
//	selection->Unlock();
//	currentLayer()->Unlock();
	DetachCurrentLayer();
	DetachSelection();
	BRect pRect = sel ? GetSmallestRect (selection) : fCanvasFrame;
	if (addon->Process (currentLayer(), sel ? selection : NULL,
						 &newlayer, &newselection, mode->selected(), &pRect, !preview)
		!= ADDON_OK)
	{
		if (newlayer != currentLayer())
			delete newlayer;
		if (newselection != selection)
			delete newselection;
		AttachSelection();
		AttachCurrentLayer();
		// selection->Unlock();
		// currentLayer()->Unlock();
		Undo();
		mainapp->setHand();
		return;
	}
	selection->Unlock();	// WHOOPEE!!  global alpha channel bug is fixed with this.
							// Don't ask me why.  It just works now.
	// currentLayer()->Unlock();
	if (newlayer != currentLayer() && newlayer)
	{
		// printf ("Replacing layer\n");
		// currentLayer()->Lock();
		delete currentLayer();
		layer[fCurrentLayer] = newlayer;
	}
	if (newselection != selection && newselection)
	{
		// printf ("Replacing selection\n");
		// selection->Lock();
		delete selection;
		selection = newselection;
		sel = true;
		selchanged = true;
	}
	AttachSelection();
	AttachCurrentLayer();
	myWindow->Lock();
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges (currentLayerIndex());
		myWindow->layerWindow->Unlock();
	}
	myWindow->Unlock();
	Invalidate();
	mainapp->setHand();
}

void CanvasView::eFilter (BPoint point, uint32 buttons)
{
	while (buttons)
	{
		if (prev != point)
		{
			BRect prevpreviewRect = fPreviewRect;
			fPreviewRect.OffsetTo (point);
			fPreviewRect.left = int (fPreviewRect.left);
			fPreviewRect.top = int (fPreviewRect.top);
			fPreviewRect.right = int (fPreviewRect.right);
			fPreviewRect.bottom = int (fPreviewRect.bottom);
			Invalidate (prevpreviewRect | fPreviewRect);
		}
		prev = point;
		snooze (100000);
		ScrollIfNeeded (point);
		myWindow->Lock();
		myWindow->posview->Pulse();
		myWindow->Unlock();
		GetMouse (&point, &buttons, true);
	}
}

//#define OLD_ETRANSFORM 1

#if defined (OLD_ETRANSFORM)

void CanvasView::eTransform (BPoint point, uint32 buttons)
{
	// Note: inefficient.  If !sel, don't `new' newselection!
	extern PicMenuButton *mode;
	if (!transformerSaved)	// There's no undo buffer setup yet.
	{
		SetupUndo (mode->selected());	// Save now.
		// But we're `just' previewing (interactively here), there's no need to
		// flush the undo buffer, so we shouldn't save next time we come here.
		transformerSaved = true;
	}
	else
	{
		Undo (false);
	}

	currentLayer()->Lock();
	selection->Lock();
	Layer *newlayer = new Layer (*currentLayer());
	Selection *newselection = new Selection (*selection);
	memcpy (newlayer->Bits(), currentLayer()->Bits(), currentLayer()->BitsLength());
	memcpy (newselection->Bits(), selection->Bits(), selection->BitsLength());
	selection->Unlock();
	currentLayer()->Unlock();
//	DetachCurrentLayer();
//	DetachSelection();
	BRect pRect = GetSmallestRect (selection);
	BPoint prev = BPoint (0, 0);
	Layer *cLayer = currentLayer();
//		printf ("currentLayer() = %p, ->Bits() = %p\n", cLayer, cLayer->Bits());
	BRect prevRect = pRect;
	BRect dirty = prevRect;
	inAddOn = (mode->selected() == M_DRAW);	// ConstructCanvas wants to know about this.
	while (buttons)
	{
		if (prev != point)
		{

			dirty = prevRect | pRect;
			//dirty.PrintToStream();
			drawView->DrawBitmap (newlayer, B_ORIGIN /*, dirty, dirty */);
			selectionView->DrawBitmap (newselection, B_ORIGIN /*, dirty, dirty*/);
			drawView->Sync();
			selectionView->Sync();
			transformerOpen->Process (newlayer, sel ? newselection : NULL,
							&cLayer, &selection,
							mode->selected(), &pRect, false, point, buttons);
			Invalidate (pRect | dirty);
			prevRect = pRect;
		}
		prev = point;
		snooze (100000);
		ScrollIfNeeded (point);
		myWindow->Lock();
		myWindow->posview->Pulse();
		myWindow->Unlock();
		GetMouse (&point, &buttons, true);
	}
	drawView->DrawBitmap (newlayer, B_ORIGIN /* dirty, dirty */);
	selectionView->DrawBitmap (newselection, B_ORIGIN /* dirty, dirty */);
	drawView->Sync();
	selectionView->Sync();
	inAddOn = false;
	transformerOpen->Process (newlayer, sel ? newselection : NULL,
					&cLayer, &selection,
					mode->selected(), &pRect, false, point, buttons);
//	AttachSelection();
//	AttachCurrentLayer();
	newlayer->Lock();
	delete newlayer;
	newselection->Lock();
	delete newselection;
	myWindow->Lock();
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges (currentLayerIndex());
		myWindow->layerWindow->Unlock();
	}
	myWindow->Unlock();
	Invalidate();
	if (mode->selected() == M_SELECT)
		sel = true;
}

#else

void CanvasView::eTransform (BPoint point, uint32 buttons)
// This is where we arrive when a Transformer add-on tells us it's interested in mouse
// movements (for instance, Scale and Ripple).
{
//	printf ("eTransform\n");
	extern PicMenuButton *mode;
	uint8 previewmode = transformerOpen->DoesPreview();
	BRect halfsize = fCanvasFrame;
	static int32 prevmode = mode->selected();
	if (prevmode != mode->selected())
	{
		Undo (true);
		Invalidate();
		transformerSaved = false;
		prevmode = mode->selected();
	}
	if (!transformerSaved)	// There's no undo buffer setup yet.
	{
		if (mode->selected() == M_DRAW && (previewmode & LAYER_AND_SELECTION))
			SetupUndo (M_DRAW_SELECT);
		else
			SetupUndo (mode->selected());	// Save now.
		// But we're `just' previewing (interactively here), there's no need to
		// flush the undo buffer, so we shouldn't save next time we come here.
		transformerSaved = true;
	}
	else
		Undo (false);

	if (previewmode & PREVIEW_2x2)
	{
		// Set up a 2x2 scaled version of the current layer and the selection.
		halfsize.left   /= 2;
		halfsize.top    /= 2;	// just to be sure...
		halfsize.right  /= 2;
		halfsize.bottom /= 2;
		tr2x2Layer = new Layer (halfsize, "temp 2x2 preview layer");
		tr2x2Layer->Lock();
		SView *tmpView = new SView (halfsize, "temp 2x2 preview view", B_FOLLOW_NONE, uint32 (NULL));
		if (sel)
		{
			tr2x2Selection = new Selection (halfsize);
			tr2x2Selection->Lock();
			tr2x2Selection->AddChild (tmpView);
			tmpView->DrawBitmap (selection, selection->Bounds(), halfsize);
			tr2x2Selection->RemoveChild (tmpView);
		}
		tr2x2Layer->AddChild (tmpView);
		tmpView->DrawBitmap (currentLayer(), fCanvasFrame, halfsize);
		tr2x2Layer->RemoveChild (tmpView);
		delete tmpView;
	}

//	currentLayer()->Lock();
//	selection->Lock();
	Layer *newlayer = NULL;
	Selection *newselection = NULL;

	BRect pRect = sel ? GetSmallestRect (selection) : fCanvasFrame;
	BRect savePRect = pRect;
	BPoint prev = BPoint (0, 0);
	if (mode->selected() == M_SELECT)
		selchanged = true;
	// if (sel) printf ("Sel!\n");
	// else printf ("No Sel!\n");
	inAddOn = (mode->selected() == M_DRAW);	// ConstructCanvas wants to know about this.
//	printf ("Detaching\n");
	DetachCurrentLayer();
	DetachSelection();
	bool hascreatednewlayer = true;
	bool hascreatednewselection = true;
	bool hascreatednewlayeronce = false;
	bool hascreatednewselectiononce = false;
	bool first = true;
//	printf ("Entering loop\n");
	if (previewmode & PREVIEW_2x2)
	// in this case the loop looks slightly different...
	{
		pRect.left   /= 2;
		pRect.top    /= 2;
		pRect.right  /= 2;
		pRect.bottom /= 2;
		point.x /= 2;
		point.y /= 2;
		while (buttons)
		{
			if (prev != point)
			{
				if (!first)
				{
					if (!hascreatednewlayer && !hascreatednewselection)
					{
//						printf ("Restoring previous result\n");
						Undo (false);
					}
					if (hascreatednewlayer)	// Cheap way of restoring old result...
					{
//						printf ("Restoring previous 2x2 result (cheaply)\n");
						Layer *tmp = newlayer;
						newlayer = tr2x2Layer;
						tr2x2Layer = tmp;
					}
					if (hascreatednewselection)
					{
//						printf ("Restoring previous 2x2 result (s) (cheaply)\n");
						Selection *tmp = newselection;
						newselection = tr2x2Selection;
						tr2x2Selection = tmp;
					}
				}
				first = false;
//				printf ("Calling Process (%f, %f)\n", point.x, point.y);
				transformerOpen->Process (tr2x2Layer, sel ? tr2x2Selection : NULL,
								&newlayer, &newselection,
								mode->selected(), &pRect, false, point, buttons);
				if (newlayer && newlayer != tr2x2Layer)
				{
//					printf ("newlayer\n");
					hascreatednewlayer = true;
					hascreatednewlayeronce = true;
					Layer *tmp = tr2x2Layer;
					tr2x2Layer = newlayer;
					newlayer = tmp;
				}
				else
					hascreatednewlayer = false;

				if (newselection && newselection != tr2x2Selection)
				{
//					printf ("newselection\n");
					hascreatednewselection = true;
					hascreatednewselectiononce = true;
					Selection *tmp = tr2x2Selection;
					tr2x2Selection = newselection;
					newselection = tmp;
				}
				else
					hascreatednewselection = false;

//				printf ("Rescaling bitmap\n");
				AttachCurrentLayer();
				drawView->DrawBitmap (tr2x2Layer, halfsize, fCanvasFrame);
				DetachCurrentLayer();

//				printf ("Invalidating...\n");
				Invalidate ();
			}
			prev = point;
			snooze (100000);
			ScrollIfNeeded (point);
			myWindow->Lock();
			myWindow->posview->Pulse();
			myWindow->Unlock();
			GetMouse (&point, &buttons, true);
			point.x /= 2;
			point.y /= 2;
		}
		// We'll take care of this ourselves, because when we fall through
		// the loop, we want the original 1x1 scale image restored.
		if (hascreatednewlayeronce)
			delete newlayer;
		if (hascreatednewselectiononce)
			delete newselection;
		hascreatednewlayeronce = false;
		hascreatednewselectiononce = false;
		newlayer = NULL;
		newselection = NULL;
		pRect = savePRect;
		point.x *= 2;
		point.y *= 2;
	}
	else
	{
		while (buttons)
		{
			if (prev != point)
			{
				if (!first)
				{
					if (!hascreatednewlayer && !hascreatednewselection)
					{
//						printf ("Restoring previous result\n");
						Undo (false);
					}
					if (hascreatednewlayer)	// Cheap way of restoring old result...
					{
//						printf ("Restoring previous result (cheaply)\n");
						Layer *tmp = newlayer;
						newlayer = currentLayer();
						layer[fCurrentLayer] = tmp;
					}
					if (hascreatednewselection)
					{
//						printf ("Restoring previous result (cheaply)\n");
						Selection *tmp = newselection;
						newselection = selection;
						selection = tmp;
					}
				}
				first = false;
//				printf ("Calling Process\n");
				transformerOpen->Process (currentLayer(), sel ? selection : NULL,
								&newlayer, &newselection,
								mode->selected(), &pRect, false, point, buttons);
				if (newlayer && newlayer != currentLayer())
				{
//					printf ("newlayer\n");
					hascreatednewlayer = true;
					hascreatednewlayeronce = true;
					Layer *tmp = currentLayer();
					layer[fCurrentLayer] = newlayer;
					newlayer = tmp;
				}
				else
					hascreatednewlayer = false;

				if (newselection && newselection != selection)
				{
//					printf ("newselection\n");
					hascreatednewselection = true;
					hascreatednewselectiononce = true;
					Selection *tmp = selection;
					selection = newselection;
					newselection = tmp;
				}
				else
					hascreatednewselection = false;

//				printf ("Invalidating...\n");
				Invalidate ();
			}
			prev = point;
			snooze (100000);
			ScrollIfNeeded (point);
			myWindow->Lock();
			myWindow->posview->Pulse();
			myWindow->Unlock();
			GetMouse (&point, &buttons, true);
		}
	}
//	printf ("Exited loop\n");
	inAddOn = false;
	if (!hascreatednewlayeronce && !hascreatednewselectiononce)
	{
//		printf ("Restoring previous result\n");
		Undo (false);
	}
	else
	{
		if (hascreatednewlayer)	// Cheap way of restoring old result...
		{
//			printf ("Restoring previous result (cheaply)\n");
			Layer *tmp = newlayer;
			newlayer = currentLayer();
			layer[fCurrentLayer] = tmp;
		}
		if (hascreatednewselection)
		{
//			printf ("Restoring previous result (cheaply)\n");
			Selection *tmp = newselection;
			newselection = selection;
			selection = tmp;
		}
	}
//	printf ("Calling Process (%f, %f) again for final preview\n", point.x, point.y);
	transformerOpen->Process (currentLayer(), sel ? selection : NULL,
					&newlayer, &newselection,
					mode->selected(), &pRect, false, point, buttons);
//	printf ("Done.\n");
	if (newlayer && newlayer != currentLayer())
	{
//		printf ("newlayer\n");
		delete currentLayer();
		layer[fCurrentLayer] = newlayer;
	}

	if (newselection && newselection != selection)
	{
//		printf ("newselection\n");
		delete selection;
		selection = newselection;
	}

	delete tr2x2Selection;
	delete tr2x2Layer;
	tr2x2Selection = NULL;
	tr2x2Layer = NULL;

//	printf ("Attaching\n");
	AttachSelection();
	AttachCurrentLayer();

	myWindow->Lock();
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges (currentLayerIndex());
		myWindow->layerWindow->Unlock();
	}
	myWindow->Unlock();
//	printf ("Finishing up...\n");
	currentLayer()->Lock();
	selection->Lock();
	Invalidate();
	if (mode->selected() == M_SELECT)
		sel = true;
}

#endif

void CanvasView::eGenerate (BPoint point, uint32 buttons)
{
//	printf ("CanvasView::eGenerate()\n");
	// Note: inefficient.  If !sel, don't `new' newselection!
	extern PicMenuButton *mode;
	if (!generatorSaved)	// There's no undo buffer setup yet.
	{
		SetupUndo (mode->selected());	// Save now.
		// But we're `just' previewing (interactively here), there's no need to
		// flush the undo buffer, so we shouldn't save next time we come here.
		generatorSaved = true;
	}

	currentLayer()->Lock();
	selection->Lock();
	Layer *newlayer = new Layer (*currentLayer());
	Selection *newselection = new Selection (*selection);
	memcpy (newlayer->Bits(), currentLayer()->Bits(), currentLayer()->BitsLength());
	memcpy (newselection->Bits(), selection->Bits(), selection->BitsLength());
	selection->Unlock();
	currentLayer()->Unlock();
	BRect pRect = sel ? GetSmallestRect (selection) : fCanvasFrame;
	BPoint prev = BPoint (0, 0);
	Layer *cLayer = currentLayer();
	DetachCurrentLayer();
	DetachSelection();
//		printf ("currentLayer() = %p, ->Bits() = %p\n", cLayer, cLayer->Bits());
	if (mode->selected() == M_SELECT)
		selchanged = true;
	// if (sel) printf ("Sel!\n");
	// else printf ("No Sel!\n");
	inAddOn = (mode->selected() == M_DRAW);	// ConstructCanvas wants to know about this.
	while (buttons)
	{
		if (prev != point)
		{
			generatorOpen->Process (newlayer, sel ? newselection : NULL,
							&cLayer, &selection,
							mode->selected(), &pRect, false, point, buttons);
			Invalidate ();
		}
		prev = point;
		snooze (100000);
		ScrollIfNeeded (point);
		myWindow->Lock();
		myWindow->posview->Pulse();
		myWindow->Unlock();
		GetMouse (&point, &buttons, true);
	}
	inAddOn = false;
	generatorOpen->Process (newlayer, sel ? newselection : NULL,
					&cLayer, &selection,
					mode->selected(), &pRect, false, point, buttons);
	AttachSelection();
	AttachCurrentLayer();
	newlayer->Lock();
	delete newlayer;
	newselection->Lock();
	delete newselection;
	myWindow->Lock();
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->doChanges (currentLayerIndex());
		myWindow->layerWindow->Unlock();
	}
	myWindow->Unlock();
	Invalidate();
	if (mode->selected() == M_SELECT)
		sel = true;
}

BBitmap *CanvasView::canvas (bool do_export)
{
	BBitmap *cvBitmap = new BBitmap (fCanvasFrame, B_RGBA32, true);
	ConstructCanvas (cvBitmap, fCanvasFrame, !do_export, do_export);
	return cvBitmap;
}

void CanvasView::Print ()
{
	status_t res = B_OK;
	extern BMessage *printSetup;
	char name[1024];
	sprintf (name, "Becasso: %s", myWindow->Name());
	BPrintJob job (name);
	if (!printSetup)
	{
		verbose (2, "No existing print setup\n");
		if ((res = job.ConfigPage()) == B_OK)
		{
			printSetup = job.Settings();
			verbose (2, "Got new settings\n");
		}
	}

	if (res == B_OK)
	{
		job.SetSettings (new BMessage (*printSetup));
		if ((res = job.ConfigJob()) == B_OK)
		{
			verbose (2, "Print job configured\n");
			if (!printSetup)
				printf ("HUH?!\n");

			delete printSetup;
			printSetup = job.Settings();
			job.BeginJob();
			verbose (2, "BeginJob() done\n");
			job.DrawView (this, fCanvasFrame, B_ORIGIN);
			verbose (2, "DrawView() done\n");
			job.SpoolPage();
			verbose (2, "SpoolPage() done\n");
			job.CommitJob();
			verbose (2, "CommitJob() done\n");
		}
	}
}

void CanvasView::ConstructCanvas (BBitmap *bitmap, BRect update, bool doselect, bool do_export)
{
//	clock_t start, end;
//	start = clock();
//	extern PicMenuButton *mode;
	bitmap->Lock();
	if (doselect)
		selection->Lock();
	update = update & bitmap->Bounds();		// This used not to be necessary, but somehow R3 can crash if you
											// go (too far) outside the bitmap.  Strange that PR2 didn't!

#if 0
	BPoint po = fPreviewRect.LeftTop();
	fPreviewRect = PreviewRect();
	if (fPreviewRect != fPrevpreviewRect)
	{
		// Preview Rect size has changed in prefs...
		delete previewLayer;
		delete previewSelection;
		previewLayer = new Layer (fPreviewRect, "Preview Layer");
		previewSelection = new Selection (fPreviewRect);
		fPrevpreviewRect = fPreviewRect;
	}
	fPreviewRect.OffsetTo (po);
#endif

//	ulong h = fCanvasFrame.IntegerHeight() + 1;	// Note the + 1 ! Really!
	ulong w = fCanvasFrame.IntegerWidth() + 1;
//	ulong sbbpr = bitmap->BytesPerRow();
	ulong sbpr = layer[0]->BytesPerRow();
	ulong rt = ulong (update.top);
	ulong rb = ulong (update.bottom);
	ulong rl = ulong (update.left);
	ulong rr = ulong (update.right);
	ulong dw = rr - rl + 1;
	ulong ddiff = (w - dw);
//	ulong sdiff = sbpr - dw;
	uint32 *src = (uint32 *) layer[0]->Bits();
	src += rt*sbpr + rl;
	uint32 *dest =  (uint32 *) screenbitmap32->Bits() + (rt*w + rl) - 1;
//	ulong brl = rl;
//	long sel_bpr = selection->BytesPerRow();
//	uchar *sel_data = (uchar *) selection->Bits() + rt*sel_bpr + rl - 1;
//	long seldiff = sel_bpr - dw;

//	printf ("sdiff = %li, ddiff = %li\n", sdiff, ddiff);
//	printf ("Background layer\n");
	#if defined (__POWERPC__)
		uint32 check1 = do_export ? 0xFFFFFF00 : 0xC8C8C800;
		uint32 check2 = do_export ? 0xFFFFFF00 : 0xEBEBEB00;
	#else
		uint32 check1 = do_export ? 0x00FFFFFF : 0x00C8C8C8;
		uint32 check2 = do_export ? 0x00FFFFFF : 0x00EBEBEB;
	#endif
	for (ulong y = rt; y <= rb; y++)	// First layer: Background.
	{
		for (ulong x = rl; x <= rr; x++)
		{
			if (((x & 8) || (y & 8)) && !((x & 8) && (y & 8)))
			{
				*(++dest) = check1;
			}
			else
			{
				*(++dest) = check2;
			}
		}
		dest += ddiff;
	}
	for (int i = 0; i < fNumLayers; i++)	// Next the layers: Add.
	{
//		printf ("Layer %i\n", i);
		// printf ("[%d]", i); fflush (stdout);
		if (!layer[i]->IsHidden())
		{
			Layer *pLayer = NULL;
			Selection *pSelection = NULL;
			bool preview_done = false;
			if ((currentLayerIndex() == i)
				&& (doselect)
				&& (filterOpen)
				&& (filterOpen->DoesPreview())& (update.Intersects (fPreviewRect)))
			{
				pLayer = new Layer (*previewLayer);
				pSelection = new Selection (*previewSelection);
				BView *preView = new SView (pLayer->Bounds(), "preView", B_FOLLOW_ALL, uint32 (NULL));
				pLayer->Lock();
				pLayer->AddChild (preView);
				preView->DrawBitmap (currentLayer(), BPoint (-fPreviewRect.left, -fPreviewRect.top));
				pLayer->RemoveChild (preView);
				pSelection->Lock();
				pSelection->AddChild (preView);
				preView->DrawBitmap (selection, BPoint (-fPreviewRect.left, -fPreviewRect.top));
				pSelection->RemoveChild (preView);
				delete preView;
				currentLayer()->Lock();
				drawView->DrawBitmap (previewLayer, fPreviewRect.LeftTop());
				currentLayer()->Unlock();
				selection->Lock();
				selectionView->DrawBitmap (previewSelection, fPreviewRect.LeftTop());
				selection->Unlock();
				preview_done = true;
			}
			Merge (screenbitmap32, layer[i], update, doselect & (i == fCurrentLayer), do_export);
			if (preview_done)
			{
				currentLayer()->Lock();
				drawView->DrawBitmap (pLayer, fPreviewRect.LeftTop());
				currentLayer()->Unlock();
				selection->Lock();
				selectionView->DrawBitmap (pSelection, fPreviewRect.LeftTop());
				selection->Unlock();
				delete pLayer;
				delete pSelection;
				preview_done = false;
			}
		}
	}
	if (bitmap->ColorSpace() == B_COLOR_8_BIT)
	{
		FSDither (screenbitmap32, bitmap, update);
	}
	else if (bitmap != screenbitmap)
	{
		///////////////////
		// printf ("Here?!\n");
		if (bitmap->ColorSpace() == B_RGBA32)
			memcpy (bitmap->Bits(), screenbitmap32->Bits(), bitmap->BitsLength());
		else
			printf ("Whoa, big trouble.\n");
	}

	extern int gGlobalAlpha;	// == registered
	if (do_export && !gGlobalAlpha)
	{
		SView *demoView = new SView (fCanvasFrame, "demoview", B_FOLLOW_NONE, uint32 (NULL));
		const char demotext[] = "Becasso";
		bitmap->AddChild (demoView);
		float h = fCanvasFrame.Height();
		float w = fCanvasFrame.Width();
		BFont font (be_bold_font);
		font_height fh;
		font.GetHeight (&fh);
		float ratio = font.StringWidth (demotext) / (fh.ascent + fh.descent + fh.leading);
		float size = sqrt (h*h/(ratio*ratio) + w*w)/font.StringWidth (demotext) * 11;
		float angle = 360*atan2 (h - (fh.ascent + fh.leading), w)/2/M_PI;
		font.SetSize (size);
		// printf ("Size = %f, ratio = %f, sqrt = %f\n", size, ratio, sqrt (h*h/(ratio*ratio) + w*w));
		font.SetRotation (angle);
		demoView->SetFont (&font);
		demoView->SetDrawingMode (B_OP_OVER);
		demoView->SetHighColor (Red);
		demoView->DrawString (demotext, BPoint (fh.ascent + fh.descent + fh.leading, h - fh.descent));
		demoView->Sync();
		bitmap->RemoveChild (demoView);
		delete demoView;
	}
	if (doselect)
		selection->Unlock();
	bitmap->Unlock();
//	end = clock();
//	printf ("ConstructCanvas took %d ms\n", end - start);
}

void CanvasView::Merge (BBitmap *a, Layer *b, BRect update, bool doselect, bool preserve_alpha)
{
//	clock_t start, end;
//	start = clock();
	extern bool UseMMX;
	extern BLocker g_settings_lock;
	extern becasso_settings g_settings;
	bool invertselect = mouse_on_canvas;
	g_settings_lock.Lock();
	if (g_settings.selection_type != SELECTION_IN_OUT)
		invertselect = true;
	g_settings_lock.Unlock();
//	ulong h = fCanvasFrame.IntegerHeight() + 1;	// Note the + 1 ! Really!
	ulong w = fCanvasFrame.IntegerWidth() + 1;
//	ulong sbbpr = a->BytesPerRow();
	ulong sbpr = b->BytesPerRow()/4;
	ulong rt = ulong (update.top);
	ulong rb = ulong (update.bottom);
	ulong rl = ulong (update.left);
	ulong rr = ulong (update.right);
	ulong dw = rr - rl + 1;
	ulong ddiff = (w - dw);
	ulong sdiff = sbpr - dw;
//	ulong brl = rl;
	long sel_bpr = selection->BytesPerRow();
	uchar *sel_data = (uchar *) selection->Bits() + rt*sel_bpr + rl - 1;
	long seldiff = sel_bpr - dw;
	uint32 *src = (uint32 *) b->Bits() + rt*sbpr + rl - 1;
	uint32 *dest = (uint32 *) a->Bits() + (rt*w + rl) - 1;
	int ga = b->getGlobalAlpha();
	switch (b->getMode())
	{
	case DM_BLEND:
		if (/* mode->selected() == M_SELECT && */ invertselect && selchanged && doselect && !inAddOn)
		{
//					printf ("Selection in layer %i\n", i);
			for (ulong y = rt; y <= rb; y++)
			{
				for (ulong x = rl; x <= rr; x++)
				{
#if defined (__POWERPC__)
					register uint32 srcpixel = *(++src);
					register uint32 destpixel = *(++dest);
					register int sa = (srcpixel & 0xFF)*ga/255;
					register int da = 255 - sa;
					register unsigned int sel = *(++sel_data);
					if (!sel)
					{
//						*dest = ((((destpixel & 0xFF00FF00) >> 8)*da + ((srcpixel & 0xFF00FF00) >> 8)*sa) & 0xFF00FF00) |
//								((((destpixel & 0x00FF0000) >> 8)*da + ((srcpixel & 0x00FF0000) >> 8)*sa) * 0x00FF0000) |
//								(sa);
//						*dest = (((((destpixel>>24)        *da +  (srcpixel>>24)        *sa)<<16)) & 0xFF000000) |
//								(((((destpixel>>16) & 0xFF)*da + ((srcpixel>>16) & 0xFF)*sa)<< 8)  & 0x00FF0000) |
//								 ((((destpixel>> 8) & 0xFF)*da + ((srcpixel>> 8) & 0xFF)*sa)       & 0x0000FF00) |
//								(sa & 0xFF);//(max_c (sa, da) & 0xFF);
#	if !defined (BLEND_USES_SHIFTS)
						*dest = ((((destpixel & 0xFF000000)/255*da + (srcpixel & 0xFF000000)/255*sa)) & 0xFF000000) |
								((((destpixel & 0x00FF0000)*da + (srcpixel & 0x00FF0000)*sa)/255) & 0x00FF0000) |
								((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |\
								  // (max_c (sa, destpixel & 0xFF));
								    (clipchar (sa + int (destpixel & 0xFF)));
#	else
						*dest = (((((destpixel & 0xFF000000)>>8)*da + ((srcpixel & 0xFF000000)>>8)*sa)) & 0xFF000000) |
								 ((((destpixel & 0x00FF0000)*da + (srcpixel & 0x00FF0000)*sa)>>8) & 0x00FF0000) |
								 ((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)>>8) & 0x0000FF00) |\
								    (clipchar (sa + int (destpixel & 0xFF)));
#	endif
					}
					else
					{
//						*dest = (0xFFFFFF00 - (srcpixel & 0xFFFFFF00)) | sa;
						register uint32 ipixel = pixelblend (srcpixel, (0xFFFFFF00 - (srcpixel & 0xFFFFFF00)) | sel);
#	if !defined (BLEND_USES_SHIFTS)
						*dest = ((((destpixel & 0xFF000000)/255*da + (ipixel & 0xFF000000)/255*sa)) & 0xFF000000) |
								((((destpixel & 0x00FF0000)*da + (ipixel & 0x00FF0000)*sa)/255) & 0x00FF0000) |
								((((destpixel & 0x0000FF00)*da + (ipixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |
								sa;
#	else
						*dest = (((((destpixel & 0xFF000000)>>8)*da + ((ipixel & 0xFF000000)>>8)*sa)) & 0xFF000000) |
								 ((((destpixel & 0x00FF0000)*da + (ipixel & 0x00FF0000)*sa)>>8) & 0x00FF0000) |
								 ((((destpixel & 0x0000FF00)*da + (ipixel & 0x0000FF00)*sa)>>8) & 0x0000FF00) |
								sa;
#	endif
					}
#else	// Intel
					register uint32 srcpixel = *(++src);
					register uint32 destpixel = *(++dest);
					register int sa = (srcpixel >> 24)*ga/255;
					register int da = 255 - sa;
					register unsigned int sel = *(++sel_data);
					if (!sel)
					{
//						*dest = ((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |
//								((((destpixel & 0x00FF00FF)*da + (srcpixel & 0x00FF00FF)*sa)/255) & 0x00FF00FF) |
//								(sa);
#	if !defined (BLEND_USES_SHIFTS)
						*dest = ((((destpixel & 0x00FF0000)*da + (srcpixel & 0x00FF0000)*sa)/255) & 0x00FF0000) |
								((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |
								((((destpixel & 0x000000FF)*da + (srcpixel & 0x000000FF)*sa)/255) & 0x000000FF) |
								    (clipchar (sa + int (destpixel >> 24)) << 24);
								    //((max_c (sa, destpixel >> 24)) << 24);
#	else
						*dest = (((((destpixel & 0x00FF0000) - (srcpixel & 0x00FF0000))*da + ((srcpixel & 0x00FF0000)<<8))>>8) & 0x00FF0000) |
								(((((destpixel & 0x0000FF00) - (srcpixel & 0x0000FF00))*da + ((srcpixel & 0x0000FF00)<<8))>>8) & 0x0000FF00) |
								(((((destpixel & 0x000000FF) - (srcpixel & 0x000000FF))*da + ((srcpixel & 0x000000FF)<<8))>>8) & 0x000000FF) |
								    (clipchar (sa + int (destpixel >> 24)) << 24);
#	endif
					}
					else
					{
//						*dest = (0x00FFFFFF - (srcpixel & 0x00FFFFFF)) | (sa << 24);
						register uint32 ipixel = pixelblend (srcpixel, (0x00FFFFFF - (srcpixel & 0x00FFFFFF)) | (sel << 24));
#	if !defined (BLEND_USES_SHIFTS)
						*dest = ((((destpixel & 0x00FF0000)*da + (ipixel & 0x00FF0000)*sa)/255) & 0x00FF0000) |
								((((destpixel & 0x0000FF00)*da + (ipixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |
								((((destpixel & 0x000000FF)*da + (ipixel & 0x000000FF)*sa)/255) & 0x000000FF) |
								(sa << 24);
#	else
						*dest = ((((destpixel & 0x00FF0000)*da + (ipixel & 0x00FF0000)*sa)>>8) & 0x00FF0000) |
								((((destpixel & 0x0000FF00)*da + (ipixel & 0x0000FF00)*sa)>>8) & 0x0000FF00) |
								((((destpixel & 0x000000FF)*da + (ipixel & 0x000000FF)*sa)>>8) & 0x000000FF) |
								(sa << 24);
#	endif
					}
#endif
				}
				src += sdiff;
				dest += ddiff;
				sel_data += seldiff;
			}
		}
		else	// doselect et al.
		{
			if (UseMMX && !preserve_alpha)
				mmx_alpha_blend ((uint32 *) b->Bits(), (uint32 *) a->Bits(), ga, w, rl, rt, rr, rb);
			else
			{
				for (ulong y = rt; y <= rb; y++)
				{
					for (ulong x = rl; x <= rr; x++)
					{
#if defined (__POWERPC__)
						register uint32 srcpixel = *(++src);
						register uint32 destpixel = *(++dest);
						register int sa = (srcpixel & 0xFF)*ga/255;
						register int da = 255 - sa;
						if (da == 0)		// Fully opaque pixel
						{
							*dest = srcpixel;
						}
						else if (da == 255)	// Fully transparent pixel
						{
						}
						else
						{
//							*dest = ((((destpixel & 0xFF00FF00)/255)*da + ((srcpixel & 0xFF00FF00)/255)*sa) & 0xFF00FF00) |
//									((((destpixel & 0x00FF0000)/255)*da + ((srcpixel & 0x00FF0000)/255)*sa) & 0x00FF0000) |
//									(sa);
//							*dest = (((((destpixel>>24)        *da +  (srcpixel>>24)        *sa)<<16)) & 0xFF000000) |
//									(((((destpixel>>16) & 0xFF)*da + ((srcpixel>>16) & 0xFF)*sa)<< 8)  & 0x00FF0000) |
//									 ((((destpixel>> 8) & 0xFF)*da + ((srcpixel>> 8) & 0xFF)*sa)       & 0x0000FF00) |
//									(sa & 0xFF);//(max_c (sa, da) & 0xFF);
#	if !defined (BLEND_USES_SHIFTS)
							*dest = ((((destpixel & 0xFF000000)/255*da + (srcpixel & 0xFF000000)/255*sa)) & 0xFF000000) |
									((((destpixel & 0x00FF0000)*da + (srcpixel & 0x00FF0000)*sa)/255) & 0x00FF0000) |
									((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |\
									  // (max_c (sa, destpixel & 0xFF));
									    (clipchar (sa + int (destpixel & 0xFF)));
#	else
							*dest = (((((destpixel & 0xFF000000)>>8)*da + ((srcpixel & 0xFF000000)>>8)*sa)) & 0xFF000000) |
									 ((((destpixel & 0x00FF0000)*da + (srcpixel & 0x00FF0000)*sa)>>8) & 0x00FF0000) |
									 ((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)>>8) & 0x0000FF00) |\
									    (clipchar (sa + int (destpixel & 0xFF)));
#	endif
						}
#else	// Intel
						register uint32 srcpixel = *(++src);
						register uint32 destpixel = *(++dest);
						register int sa = (srcpixel >> 24)*ga/255;
						register int da = 255 - sa;
						if (da == 0)		// Fully opaque pixel
						{
							*dest = srcpixel;
						}
						else if (da == 255)	// Fully transparent pixel
						{
						}
						else
						{
//							*dest = ((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa) >> 8) & 0x0000FF00) |
//									((((destpixel & 0x00FF00FF)*da + (srcpixel & 0x00FF00FF)*sa) & 0xFF00FF00) >> 8) |
//									(clipchar (sa + int (destpixel >> 24)) << 24);
//							*dest = ((((((destpixel>>16) & 0xFF)*da + ((srcpixel>>16) & 0xFF)*sa) << 16)/255) & 0x00FF0000) |
//									((((((destpixel>> 8) & 0xFF)*da + ((srcpixel>> 8) & 0xFF)*sa) <<  8)/255) & 0x0000FF00) |
//									((((((destpixel    ) & 0xFF)*da + ((srcpixel    ) & 0xFF)*sa)      )/255) & 0x000000FF) |
#	if !defined (BLEND_USES_SHIFTS)
							*dest = ((((destpixel & 0x00FF0000)*da + (srcpixel & 0x00FF0000)*sa)/255) & 0x00FF0000) |
									((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |
									((((destpixel & 0x000000FF)*da + (srcpixel & 0x000000FF)*sa)/255) & 0x000000FF) |
									    (clipchar (sa + int (destpixel >> 24)) << 24);
									    //((max_c (sa, destpixel >> 24)) << 24);
#	else
							*dest = ((((destpixel & 0x00FF0000)*da + (srcpixel & 0x00FF0000)*sa)>>8) & 0x00FF0000) |
									((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)>>8) & 0x0000FF00) |
									((((destpixel & 0x000000FF)*da + (srcpixel & 0x000000FF)*sa)>>8) & 0x000000FF) |
									    (clipchar (sa + int (destpixel >> 24)) << 24);
#	endif
						}
#endif
					}
					src += sdiff;
					dest += ddiff;
				}
			}
		}
		break;
	case DM_MULTIPLY:
		if (/* mode->selected() == M_SELECT && */ invertselect && selchanged && doselect && !inAddOn)
		{
//					printf ("Selection in layer %i\n", i);
			for (ulong y = rt; y <= rb; y++)
			{
				for (ulong x = rl; x <= rr; x++)
				{
#if defined (__POWERPC__)
					register uint32 srcpixel = *(++src);
					register uint32 destpixel = *(++dest);
					register int sa = (srcpixel & 0xFF)*ga/255;
					register int da = destpixel & 0xFF;
					register unsigned int sel = *(++sel_data);
					register int tsa = 65025 - sa*255;
					register int tda = 65025 - da*255;
					if (!sel)
					{
						*dest = (((tsa + sa*((srcpixel >> 24)       ))*(tda + da*((destpixel >> 24)       ))/16581375 << 24) & 0xFF000000) |
								(((tsa + sa*((srcpixel >> 16) & 0xFF))*(tda + da*((destpixel >> 16) & 0xFF))/16581375 << 16) & 0x00FF0000) |
								(((tsa + sa*((srcpixel >>  8) & 0xFF))*(tda + da*((destpixel >>  8) & 0xFF))/16581375 <<  8) & 0x0000FF00) |
								(max_c (sa, da) & 0xFF);
					}
					else
					{
//						*dest = (0xFFFFFF00 - (srcpixel & 0xFFFFFF00)) | sa;
						register uint32 ipixel = pixelblend (srcpixel, (0xFFFFFF00 - (srcpixel & 0xFFFFFF00)) | sel);
//						register uint32 ipixel = 0xFFFFFF00 - (srcpixel & 0xFFFFFF00);
						*dest = ((((destpixel & 0xFF000000)/255*da + (ipixel & 0xFF000000)/255*sa)) & 0xFF000000) |
								((((destpixel & 0x00FF0000)*da + (ipixel & 0x00FF0000)*sa)/255) & 0x00FF0000) |
								((((destpixel & 0x0000FF00)*da + (ipixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |
								sa;
					}
#else
					register uint32 srcpixel = *(++src);
					register uint32 destpixel = *(++dest);
					register int sa = (srcpixel >> 24)*ga/255;
					register int da = destpixel >> 24;
					register unsigned int sel = *(++sel_data);
					register int tsa = 65025 - sa*255;
					register int tda = 65025 - da*255;
					if (!sel)
					{
						*dest = (((tsa + sa*((srcpixel >> 16) & 0xFF))*(tda + da*((destpixel >> 16) & 0xFF))/16581375 << 16) & 0x00FF0000) |
								(((tsa + sa*((srcpixel >>  8) & 0xFF))*(tda + da*((destpixel >>  8) & 0xFF))/16581375 <<  8) & 0x0000FF00) |
								(((tsa + sa*((srcpixel      ) & 0xFF))*(tda + da*((destpixel      ) & 0xFF))/16581375      ) & 0x000000FF) |
								  (clipchar (sa + int (destpixel >> 24)) << 24);
								//((max_c (sa, da) & 0xFF) << 24);
					}
					else
					{
//						*dest = (0x00FFFFFF - (srcpixel & 0x00FFFFFF)) | (sa << 24);
//						register uint32 ipixel = 0x00FFFFFF - (srcpixel & 0x00FFFFFF);
						register uint32 ipixel = pixelblend (srcpixel, (0x00FFFFFF - (srcpixel & 0x00FFFFFF)) | (sel << 24));
						*dest = ((((destpixel & 0x00FF0000)*da + (ipixel & 0x00FF0000)*sa)/255) & 0x00FF0000) |
								((((destpixel & 0x0000FF00)*da + (ipixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |
								((((destpixel & 0x000000FF)*da + (ipixel & 0x000000FF)*sa)/255) & 0x000000FF) |
								(sa << 24);
					}
#endif
				}
				src += sdiff;
				dest += ddiff;
				sel_data += seldiff;
			}
		}
		else
		{
			for (ulong y = rt; y <= rb; y++)
			{
				for (ulong x = rl; x <= rr; x++)
				{
#if defined (__POWERPC__)
					register uint32 srcpixel = *(++src);
					register uint32 destpixel = *(++dest);
					register int sa = (srcpixel & 0xFF)*ga/255;
					register int da = destpixel & 0xFF;
					register int tsa = 65025 - sa*255;
					register int tda = 65025 - da*255;
					if (sa == 0)		// Fully transparent pixel
					{
					}
					else if (sa == 255)	// Fully opaque pixel
					{
						*dest = (((((srcpixel >> 24)       )*(tda + da*((destpixel >> 24)       ))/65025) << 24) & 0xFF000000) |
								(((((srcpixel >> 16) & 0xFF)*(tda + da*((destpixel >> 16) & 0xFF))/65025) << 16) & 0x00FF0000) |
								(((((srcpixel >>  8) & 0xFF)*(tda + da*((destpixel >>  8) & 0xFF))/65025) <<  8) & 0x0000FF00) |
								    (clipchar (sa + int (destpixel & 0xFF)));
									//(max_c (sa, da) & 0xFF);
					}
					else
					{
						*dest = (((tsa + sa*((srcpixel >> 24)       ))*(tda + da*((destpixel >> 24)       ))/16581375 << 24) & 0xFF000000) |
								(((tsa + sa*((srcpixel >> 16) & 0xFF))*(tda + da*((destpixel >> 16) & 0xFF))/16581375 << 16) & 0x00FF0000) |
								(((tsa + sa*((srcpixel >>  8) & 0xFF))*(tda + da*((destpixel >>  8) & 0xFF))/16581375 <<  8) & 0x0000FF00) |
							      (clipchar (sa + int (destpixel & 0xFF)));
								//(max_c (sa, da) & 0xFF);
					}
#else
					register uint32 srcpixel = *(++src);
					register uint32 destpixel = *(++dest);
					register int sa = (srcpixel >> 24)*ga/255;
					register int da = destpixel >> 24;
					register int tsa = 65025 - sa*255;
					register int tda = 65025 - da*255;
					if (sa == 0)		// Fully transparent pixel
					{
					}
					else if (sa == 255)	// Fully opaque pixel
					{
						*dest = (((((srcpixel >> 16) & 0xFF)*(tda + da*((destpixel >> 16) & 0xFF))/65025) << 16) & 0x00FF0000) |
								(((((srcpixel >>  8) & 0xFF)*(tda + da*((destpixel >>  8) & 0xFF))/65025) <<  8) & 0x0000FF00) |
								(((((srcpixel      ) & 0xFF)*(tda + da*((destpixel      ) & 0xFF))/65025)      ) & 0x000000FF) |
								    (clipchar (sa + int (destpixel >> 24)) << 24);
								//((max_c (sa, da) & 0xFF) << 24);
					}
					else
					{
						*dest = (((tsa + sa*((srcpixel >> 16) & 0xFF))*(tda + da*((destpixel >> 16) & 0xFF))/16581375 << 16) & 0x00FF0000) |
								(((tsa + sa*((srcpixel >>  8) & 0xFF))*(tda + da*((destpixel >>  8) & 0xFF))/16581375 <<  8) & 0x0000FF00) |
								(((tsa + sa*((srcpixel      ) & 0xFF))*(tda + da*((destpixel      ) & 0xFF))/16581375      ) & 0x000000FF) |
								  (clipchar (sa + int (destpixel >> 24)) << 24);
								//((max_c (sa, da) & 0xFF) << 24);
					}
#endif
				}
				src += sdiff;
				dest += ddiff;
			}
		}
		break;
	case DM_DIFFERENCE:
	default:
		fprintf (stderr, "Merge:  Unknown Drawing Mode\n");
		break;
	}
//	end = clock();
//	printf ("Merge took %d ms\n", end - start);
}

// Used when a color change is made in the palette - some add-ons may need to recalculate.
void CanvasView::InvalidateAddons ()
{
	if (filterOpen && filterOpen->DoesPreview())
		filterOpen->ColorChanged();
	if (transformerOpen && transformerOpen->DoesPreview())
		transformerOpen->ColorChanged();
	if (generatorOpen && generatorOpen->DoesPreview())
		generatorOpen->ColorChanged();
}

void CanvasView::Invalidate (const BRect rect)
{
	clock_t start, end;
	start = clock();
	// fprintf (stderr, "CanvasView::Invalidate (BRect) ... "); fflush (stderr);
	extern PicMenuButton *mode;
//	inherited::Invalidate (rect);
//	rect.PrintToStream();

	BPoint po = fPreviewRect.LeftTop();
	fPreviewRect = PreviewRect();
	if (fPreviewRect != fPrevpreviewRect)
	{
		// Preview Rect size has changed in prefs...
		delete previewLayer;
		delete previewSelection;
		previewLayer = new Layer (fPreviewRect, "Preview Layer");
		previewSelection = new Selection (fPreviewRect);
		fPrevpreviewRect = fPreviewRect;
	}
	fPreviewRect.OffsetTo (po);

	BRect update = makePositive (rect);
	update.left = min_c (max_c (int (update.left), fCanvasFrame.left), fCanvasFrame.right);
	update.top = min_c (max_c (int (update.top), fCanvasFrame.top), fCanvasFrame.bottom);
	update.right = max_c (min_c (int (update.right), fCanvasFrame.right), fCanvasFrame.left);
	update.bottom = max_c (min_c (int (update.bottom), fCanvasFrame.bottom), fCanvasFrame.top);
//	update.PrintToStream();
	if (filterOpen && rect.Intersects (fPreviewRect) && filterOpen->DoesPreview())
	{
		Layer *pLayer = previewLayer;
		Selection *pSelection = previewSelection;
		if (mode->selected() == M_DRAW)
			pLayer = new Layer (*previewLayer);
		else
			pSelection = new Selection (*previewSelection);
		BView *preView = new SView (pLayer->Bounds(), "preView", B_FOLLOW_ALL, uint32 (NULL));
		pLayer->Lock();
		pLayer->AddChild (preView);
		preView->DrawBitmap (currentLayer(), BPoint (-fPreviewRect.left, -fPreviewRect.top));
		preView->Sync();
		pLayer->RemoveChild (preView);
		pSelection->Lock();
		pSelection->AddChild (preView);
		preView->DrawBitmap (selection, BPoint (-fPreviewRect.left, -fPreviewRect.top));
		preView->Sync();
		pSelection->RemoveChild (preView);
		BRect pRect = sel ? (GetSmallestRect (selection) & fPreviewRect) : fPreviewRect;
		pRect.OffsetBy (-fPreviewRect.left, -fPreviewRect.top);
//		printf ("Calling from Invalidate.  sel = %s\n", sel ? "true" : "false");
		filterOpen->Process (pLayer, sel ? pSelection : NULL, &previewLayer, &previewSelection,
							  mode->selected(), &pRect, false);
		pRect.OffsetBy (fPreviewRect.left, fPreviewRect.top);
		delete preView;
		if (mode->selected() == M_DRAW)
			delete pLayer;
		else
			delete pSelection;
	}
	extern bool inPaste;
	bool doselect = !inPaste;
	ConstructCanvas (screenbitmap, update, doselect);
	BRect drawRect = BRect (update.left*fScale, update.top*fScale, update.right*fScale, update.bottom*fScale);
	Draw (drawRect);
	if (myWindow->isLayerOpen())
	{
		myWindow->layerWindow->Lock();
		myWindow->layerWindow->layerView->getLayerItem(fCurrentLayer)->DrawThumbOnly();
		myWindow->layerWindow->Unlock();
	}
	if (myWindow->TheMagWindow())
	{
		if (!(myWindow->TheMagWindow()->IsActive()))
		{
			// printf ("Sending 'draw' to MagWindow\n");
			myWindow->TheMagWindow()->PostMessage ('draw');
		}
		// (otherwise, the redraw probably comes from the mag window itself...)
	}
	// fprintf (stderr, "Done.\n");
	end = clock();
	extern int DebugLevel;
	if (DebugLevel > 7)
		printf ("Invalidate (BRect) took %ld ms\n", end - start);
}

void CanvasView::Invalidate ()
{
	BRect update;
//	if (fBGView)
//	{
//		update = fBGView->Bounds();
//		update.left   /= fScale;
//		update.right  /= fScale;
//		update.top    /= fScale;
//		update.bottom /= fScale;
//	}
//	else
		update = fCanvasFrame;
	Invalidate (update);
	if (!text)
		MakeFocus();
}

void CanvasView::Draw (BRect updateRect)
{
//	printf ("Draw: "); fflush (stdout);
	if (IsPrinting())
	{
		verbose (2, "IsPrinting()\n");
		BBitmap *b = canvas();
		DrawBitmap (b);
		Sync();
		verbose (2, "DrawBitmap(), Sync()\n");
		delete b;
	}
	else
	{
		updateRect.left   = int (updateRect.left/fScale);
		updateRect.top    = int (updateRect.top/fScale);
		updateRect.right  = int (updateRect.right/fScale) + 1;
		updateRect.bottom = int (updateRect.bottom/fScale) + 1;
//		updateRect.PrintToStream();
//		BRegion clip = BRegion();
//		clip.Include (updateRect);
		// This might break in R3...
//		SetScale (fScale);
//		ConstrainClippingRegion (&clip);
//		printf ("Drawing... "); fflush (stdout);
		screenbitmap->Lock();
		SetDrawingMode (B_OP_COPY);
		if (fScale == 1)
			DrawBitmapAsync (screenbitmap, updateRect, updateRect);
		else
			DrawBitmapAsync (screenbitmap, B_ORIGIN);
//		printf ("Syncing... "); fflush (stdout);
		//DrawBitmapAsync (screenbitmap, B_ORIGIN);
		Sync();
		screenbitmap->Unlock();
//		printf ("Unlocked... "); fflush (stdout);
		if (filterOpen)
		{
			SetDrawingMode (B_OP_INVERT);
			SetPenSize (0);
			StrokeRect (fPreviewRect);
		}
//		SetScale (1);
	}
//	printf ("Done\n");
}

void CanvasView::setScrollBars (BScrollBar *_h, BScrollBar *_v)
{
	hSB = _h;
	vSB = _v;
	hSB->SetProportion (min_c (1, Bounds().Width()/fScale/fCanvasFrame.Width()));
	vSB->SetProportion (min_c (1, Bounds().Height()/fScale/fCanvasFrame.Height()));
}

void CanvasView::Save (BEntry entry)
{
	extern int gGlobalAlpha;
	if (!gGlobalAlpha)
	{
#if SAVE_DISABLED
		int32 result;
		BAlert *alert = new BAlert ("Disabled",
		 lstring (158, "Saving is disabled in this demo version of Becasso.\nOn http://www.sumware.demon.nl you can find information on obtaining the full version.\n\nThanks for your interest in Becasso!"),
		 lstring (131, "Cancel"), lstring (159, "Visit URL"), NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		result = alert->Go();
		if (result == 1)
			system ("NetPositive http://www.sumware.demon.nl &");
		return;
#elif SAVE_NAG
		LockLooper();
		BRect rect = Window()->Frame();
		UnlockLooper();
		rect.left += 16;
		rect.top += 16;
		rect.right = rect.left + 340;
		rect.bottom = rect.top + 80;
		Window()->Hide();
		(new NagWindow (rect))->Go();
		Window()->Show();
		Invalidate();
#endif
	}
	extern const char *Version;
	extern Becasso *mainapp;
	FILE *fp;
	char line[81];
	mainapp->setBusy();
	BFile file = BFile (&entry, B_CREATE_FILE | B_READ_WRITE | B_ERASE_FILE);
	BPath path;
	entry.GetPath (&path);
	BNode node = BNode (&entry);
	BNodeInfo nodeinfo;
	nodeinfo.SetTo (&node);
	// node.WriteAttr ("BEOS:TYPE", B_STRING_TYPE, 0, "image/x-becasso\0", 16);
	nodeinfo.SetType ("image/x-becasso");
	BMimeType mime ("image/x-becasso");
	if (!mime.IsInstalled())
	{
		mime.Install();
	}
	fp = fopen (path.Path(), "w");
	fputs ("# Becasso image file\n", fp);
	if (!gGlobalAlpha)
		fputs ("# (Unregistered Version)\n", fp);
	sprintf (line, "%s %i %i 72 %i\n", Version, fNumLayers, fCurrentLayer, gGlobalAlpha ? 0: 1);
	fputs (line, fp);
	uchar *tmpzbuf = new uchar [uint32 (currentLayer()->BitsLength()*1.002) + 12L];
	z_streamp zstream = new z_stream;
	zstream->zalloc = Z_NULL;
	zstream->zfree = Z_NULL;
	zstream->opaque = Z_NULL;
	if (deflateInit (zstream, Z_DEFAULT_COMPRESSION) != Z_OK)
	{
		fprintf (stderr, "zlib error!\n");
	}
	for (int i = 0; i < fNumLayers; i++)
	{
		sprintf (line, "%.0f %.0f %.0f %.0f %i %i %i %i %s\n",
			layer[i]->Bounds().left,
			layer[i]->Bounds().top,
			layer[i]->Bounds().right,
			layer[i]->Bounds().bottom,
			layer[i]->getMode(),
			layer[i]->IsHidden() ? 1 : 0,
			layer[i]->getGlobalAlpha(),
			layer[i]->getAlphaMap() ? 1 : 0,
			layer[i]->getName());
		fputs (line, fp);
	}
	if (!gGlobalAlpha)
		InsertGlobalAlpha (layer, fNumLayers);
	for (int i = 0; i < fNumLayers; i++)
	{
//		printf ("Compressing layer %i\n", i);
		zstream->next_in = (uchar *) layer[i]->Bits();
		zstream->avail_in = layer[i]->BitsLength();
		zstream->next_out = tmpzbuf;
		zstream->avail_out = zstream->avail_in;
		if (deflate (zstream, Z_FINISH) != Z_STREAM_END)
		{
			fprintf (stderr, "Argh, zlib couldn't compress the whole layer!\n");
		}
		fwrite (tmpzbuf, zstream->next_out - tmpzbuf, 1, fp);
		deflateReset (zstream);
	}
	if (!gGlobalAlpha)
		InsertGlobalAlpha(layer, fNumLayers);
	deflateEnd (zstream);
	delete zstream;
	delete [] tmpzbuf;
	fclose (fp);
	mainapp->setReady();
}

void CanvasView::Pulse ()
{
	extern BLocker *clipLock;

	if (clipLock->LockWithTimeout (500000) != B_OK)
	{
		printf ("Still switching windows..? %s\n", myWindow->Name());
		return;
	}

	extern bool inPaste, inDrag;
	extern Tablet *wacom;
	if (wacom && wacom->IsValid() && Window()->IsActive())
	{
		//printf ("Updating tablet... "); fflush (stdout);
		wacom->Update();
		//printf ("Done.\n");
		if (wacom->Proximity())
		{
			// printf ("Proximity detected!\n");
//			thread_id position_thread;
//			if (find_thread ("Position Tracker") == B_NAME_NOT_FOUND)
//			{
//				posision_thread = spawn_thread (position_tracker, "Position Tracker", B_DISPLAY_PRIORITY, this);
//			}

			extern bool BuiltInTablet;
			if (BuiltInTablet)
			{
				Position position;
				wacom->SetScale (fCanvasFrame.Width(), fCanvasFrame.Height());
				fTreshold = wacom->Threshold();
				GetPosition (&position);
				if (position.fButtons)
					MouseOrTablet (&position);
			}
			// printf ("Back home.\n");
			clipLock->Unlock();
			return;
		}
	}
	BPoint point;
	uint32 buttons;
	GetMouse (&point, &buttons, true);

	extern Becasso *mainapp;
	if (modifiers() & B_OPTION_KEY && mouse_on_canvas)	// Color Picker / DragScroll
	{
		if (modifiers() & B_SHIFT_KEY)
		{
			if (fPicking)
				mainapp->setPicker (false);

			mainapp->setGrab (true, buttons);
			if (!fDragScroll & buttons)
			{
				BPoint lt = Frame().LeftTop();
				BPoint p = ConvertToScreen (BPoint (point.x*fScale, point.y*fScale));
				fPoint = BPoint (p.x - lt.x, p.y - lt.y);
//				printf ("New drag: ");
//				fPoint.PrintToStream();
			}

			fDragScroll = true;
			fPicking = false;
		}
		else
		{
			if (fDragScroll)
				mainapp->setGrab (false);

			mainapp->setPicker (true);
			fPicking = true;
			fDragScroll = false;
		}
	}
	else if (fPicking || fDragScroll)
	{
		if (fPicking)
		{
			mainapp->setPicker (false);
			fPicking = false;
		}
		if (fDragScroll)
		{
			mainapp->setGrab (false);
			fDragScroll = false;
		}

		if (mouse_on_canvas && !buttons)
			mainapp->FixCursor();	// Otherwise: Few pixels off!!  (BeOS bug w.r.t. hot spot of cursors ...)
	}

	extern PicMenuButton *tool, *mode;
	int32 currentMode = mode->selected();
	int32 currentTool = tool->selected();
	static int32 lastTool = tool->selected();
	static int32 lastMode = mode->selected();
	if (lastMode != currentMode)
	{
		lastMode = currentMode;
		mainapp->setCross();
	}
	if (lastTool != currentTool)
	{
		lastTool = currentTool;
		myWindow->posview->SetPoint (-1, -1);
	}
	if (point != prev && Window()->IsActive() && !(modifiers() & B_OPTION_KEY))
	{
		if (currentTool != prevTool)
		{
			entry = 0;
			prevTool = currentTool;
		}
		currentLayer()->Lock();
		if (inPaste && !inDrag)
			ePasteM (point);
		else if (fTranslating && mouse_on_canvas)
			mainapp->setMover();
		else if (fRotating && mouse_on_canvas)
			mainapp->setRotator();
		else
		{
			switch (currentTool)	// Only some tools are interested in this.
			{
			case T_ERASER:
				tEraserM (currentMode, point);
				break;
			case T_TEXT:
				tTextM (currentMode, point);
				break;
			case T_LINES:
				tLinesM (currentMode, point);
				break;
			case T_POLYGON:
				tPolygonM (currentMode, point);
				break;
			case T_RECT:
				tRectM (currentMode, point);
				break;
			case T_RRECT:
				tRoundRectM (currentMode, point);
				break;
			case T_CIRCLE:
				tCircleM (currentMode, point);
				break;
			case T_ELLIPSE:
				tEllipseM (currentMode, point);
				break;
			default:
				break;
			}
		}
		currentLayer()->Unlock();
		prev = point;
	}
	clipLock->Unlock();
}


BHandler *CanvasView::ResolveSpecifier (BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property)
{
//	printf ("CanvasView::ResolveSpecifier!!  message:\n");
//	message->PrintToStream();
//	printf ("\nspecifier:\n");
//	specifier->PrintToStream();
//	printf ("command: %ld, property: ""%s""\n", command, property);

// This is a bit nonstandard, but we want the same BHandler to first find out
// which Layer is specified, and then again be called for the actual property.

	if (!strcasecmp (property, "Layer"))
	{
		fLayerSpecifier = -1;
		const char *namedspecifier;
		int32 indexspecifier;
		int32 i;
		if (specifier->FindString ("name", &namedspecifier) == B_OK)
		{
			// printf ("name = %s\n", namedspecifier);
			for (i = 0; i < fNumLayers; i++)
			{
				Layer *l = layer[i];
				if (!strcmp (namedspecifier, l->getName()))
				{
					fLayerSpecifier = i;
				}
			}
		}
		else if (specifier->FindInt32 ("index", &indexspecifier) == B_OK)
		{
			if (indexspecifier >= 0 && indexspecifier < fNumLayers)
			{
				fLayerSpecifier = indexspecifier;
			}
		}
		message->PopSpecifier();
		message->GetCurrentSpecifier (&index, specifier, &command, &property);
		fCurrentProperty = PROP_LAYER;
		if (message->what == B_DELETE_PROPERTY)
			return this;
		else
			return ResolveSpecifier (message, index, specifier, command, property);
	}
	if (!strcasecmp (property, "GlobalAlpha") && fLayerSpecifier != -1)
	{
		fCurrentProperty = PROP_GLOBALALPHA;
		return this;
	}
	if (!strcasecmp (property, "Contents") && fLayerSpecifier != -1)
	{
		fCurrentProperty = PROP_CONTENTS;
		return this;
	}
	if (!strcasecmp (property, "Name") && fLayerSpecifier != -1)
	{
		fCurrentProperty = PROP_NAME;
		return this;
	}

	return inherited::ResolveSpecifier (message, index, specifier, command, property);
}

void CanvasView::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
	case B_GET_PROPERTY:
	{
		switch (fCurrentProperty)
		{
		case PROP_GLOBALALPHA:
		{
			if (fLayerSpecifier != -1)
			{
				if (msg->IsSourceWaiting())
				{
					BMessage error (B_REPLY);
					error.AddInt32 ("result", layer[fLayerSpecifier]->getGlobalAlpha());
					msg->SendReply (&error);
				}
				else
					fprintf (stderr, "Alpha of Layer %s = %d\n", layer[fLayerSpecifier]->getName(), layer[fLayerSpecifier]->getGlobalAlpha());
			}
			fLayerSpecifier = -1;
			break;
		}
		case PROP_NAME:
		{
			if (fLayerSpecifier != -1)
			{
				if (msg->IsSourceWaiting())
				{
					BMessage error (B_REPLY);
					error.AddString ("result", layer[fLayerSpecifier]->getName());
					msg->SendReply (&error);
				}
				else
					fprintf (stderr, "Layer %ld = %s\n", fLayerSpecifier, layer[fLayerSpecifier]->getName());
			}
			fLayerSpecifier = -1;
			break;
		}
		default:
			break;
		}
		fCurrentProperty = 0;
		break;
	}
	case B_SET_PROPERTY:
	{
		switch (fCurrentProperty)
		{
		case PROP_NAME:
		{
			const char *name;
			if (msg->FindString ("data", &name) == B_OK && fLayerSpecifier != -1)
			{

			}
			fLayerSpecifier = -1;
			break;
		}
		case PROP_GLOBALALPHA:
		{
			int32 value;
			if (msg->FindInt32 ("data", &value) == B_OK && fLayerSpecifier != -1)
			{
				if (value >= 0 && value <= 255)
				{
					setGlobalAlpha (fLayerSpecifier, value);
					if (myWindow->isLayerOpen())
					{
						myWindow->layerWindow->Lock();
						myWindow->layerWindow->doChanges();
						myWindow->layerWindow->Unlock();
					}
					if (msg->IsSourceWaiting())
					{
						BMessage error (B_REPLY);
						error.AddInt32 ("error", B_NO_ERROR);
						msg->SendReply (&error);
					}
				}
			}
			fLayerSpecifier = -1;
			break;
		}
		case PROP_CONTENTS:
		{
			clock_t start, end;
			start = clock();
			verbose (3, "Changing Layer Contents: Entered MessageReceived case\n");
			entry_ref eref;
			const char *fname;
			if (msg->FindRef ("data", &eref) == B_OK && fLayerSpecifier != -1)
			{
				BBitmap *map;
				BEntry entry = BEntry (&eref);
				BPath path;
				entry.GetPath (&path);
				if ((map = BTranslationUtils::GetBitmapFile (path.Path())), map)
				{
					setLayerContentsToBitmap (fLayerSpecifier, map);
					delete map;
					if (msg->IsSourceWaiting())
					{
						BMessage error (B_REPLY);
						error.AddInt32 ("error", B_NO_ERROR);
						msg->SendReply (&error);
					}
				}
				else
				{
					if (msg->IsSourceWaiting())
					{
						BMessage error (B_ERROR);
						error.AddInt32 ("error", B_NO_TRANSLATOR);
						msg->SendReply (&error);
					}
				}
			}
			else if (msg->FindString ("data", &fname) == B_OK)
			{
				char errstring[256];
				sprintf (errstring, "File added by name (%s) not supported - use ref", fname);
				if (msg->IsSourceWaiting())
				{
					BMessage error (B_ERROR);
					error.AddInt32 ("error", B_BAD_SCRIPT_SYNTAX);
					error.AddString ("message", errstring);
					msg->SendReply (&error);
				}
				else
					fprintf (stderr, "%s\n", errstring);
			}
			fLayerSpecifier = -1;
			end = clock();
			extern int DebugLevel;
			if (DebugLevel > 2)
				printf ("MessageReceived case Took %ld ms\n", end - start);
			break;
		}
		default:
			break;
		}
		fCurrentProperty = 0;
		break;
	}
	case B_DELETE_PROPERTY:
	{
		switch (fCurrentProperty)
		{
			case PROP_LAYER:
				if (fLayerSpecifier != -1)
					removeLayer (fLayerSpecifier);
				fLayerSpecifier = -1;
				if (myWindow->isLayerOpen())
				{
					myWindow->layerWindow->Lock();
					myWindow->layerWindow->doChanges();
					myWindow->layerWindow->Unlock();
				}
				if (msg->IsSourceWaiting())
				{
					BMessage error (B_REPLY);
					error.AddInt32 ("error", B_NO_ERROR);
					msg->SendReply (&error);
				}
				break;
			default:
				inherited::MessageReceived (msg);
				break;
		}
		fCurrentProperty = 0;
		break;
	}
	case 'drag':
	{
		extern bool inPaste, inDrag;
		printf ("%s gets dragmessage, inPaste = %s\n", Window()->Title(), inPaste ? "true" : "false");
//		SetupUndo (M_DRAG);
//		inPaste = false;
		inDrag = true;
//		firstDrag = true;
//		changed = true;
		//if (inPaste)
			Paste (inPaste);
		inDrag = false;
		break;
	}
	case 'lNch':
	{
		// Layer name has changed...
		int32 index;
		msg->FindInt32 ("index", &index);
		changed = true;
		break;
	}
	case B_SIMPLE_DATA:
	case B_MIME_DATA:
	case B_ARCHIVED_OBJECT:
	{
		if (msg->HasBool ("not_top"))
		{
			extern bool inPaste, inDrag;
			printf ("inDrag = %s, inPaste = %s\n", (inDrag ? "true" : "false"), (inPaste ? "true" : "false"));
			inDrag = true;
			Paste (inPaste);
			inDrag = false;
		}
		else
			SimpleData (msg);

		break;
	}
	case B_COPY_TARGET:
	{
		CopyTarget (msg);
		break;
	}
	case B_TRASH_TARGET:
	{
		TrashTarget (msg);
		break;
	}
	default:
	{
		extern int DebugLevel;
		if (DebugLevel > 2)
			msg->PrintToStream();
		inherited::MessageReceived (msg);
		break;
	}
	}
}

void CanvasView::SimpleData (BMessage *message)
{
	verbose (1, "SimpleData dropped\n");
	BBitmap *bitmap_ptr = NULL;
	BBitmap *drop_map = NULL;
	BRect fr;
	BPoint pt = B_ORIGIN;
	int w, h;
	BPoint target;
	bool fromBecasso = false;

	extern int DebugLevel;
	if (message->FindBool ("becasso", &fromBecasso) != B_OK && DebugLevel >= 2)
	{
		printf ("Not Becasso-originated.\n");
		message->PrintToStream ();
	}

	const void *dataPtr;
	ssize_t numBytes;
	if (message->FindData ("image/x-becasso", B_RAW_TYPE, &dataPtr, &numBytes) == B_OK
	 || message->FindData ("image/x-becasso", B_MIME_TYPE, &dataPtr, &numBytes) == B_OK
	 || message->FindData ("image/png", B_RAW_TYPE, &dataPtr, &numBytes) == B_OK
	 || message->FindData ("image/png", B_MIME_TYPE, &dataPtr, &numBytes) == B_OK
	 || message->FindData ("image/x-png", B_RAW_TYPE, &dataPtr, &numBytes) == B_OK
	 || message->FindData ("image/x-png", B_MIME_TYPE, &dataPtr, &numBytes) == B_OK
	 || message->FindData ("image/targa", B_RAW_TYPE, &dataPtr, &numBytes) == B_OK
	 || message->FindData ("image/targa", B_MIME_TYPE, &dataPtr, &numBytes) == B_OK
	 || message->FindData ("image/x-targa", B_RAW_TYPE, &dataPtr, &numBytes) == B_OK
	 || message->FindData ("image/x-targa", B_MIME_TYPE, &dataPtr, &numBytes) == B_OK)
	{
		// Looks like we got a data message!
		verbose (2, "Data message\n");
//		message->PrintToStream();
		BBitmapStream bstr;
		BMemoryIO *mstr = new BMemoryIO (dataPtr, numBytes);
		if (BTranslatorRoster::Default()->Translate (mstr, NULL, NULL, &bstr, B_TRANSLATOR_BITMAP) != B_OK)
		{
			fprintf (stderr, "CanvasView: Data Message translation error...\n");
			delete mstr;
			return;
		}
		bstr.DetachBitmap (&drop_map);
		delete mstr;
		BMessage *prev = (BMessage *) message->Previous()->Previous();
		// (the cast is to get rid of a const compiler warning).
		if (!prev)
		{
			SetupUndo (M_DRAW);
			currentLayer()->Lock();
			drawView->SetDrawingMode (B_OP_ALPHA);
			drawView->DrawBitmap (drop_map, fr.LeftTop());
			currentLayer()->Unlock();
			Invalidate();
			return;
		}
//		printf ("\nPrevious:\n");
//		prev->PrintToStream();

		// DON'T !!   delete message;
		message = prev;
	}

	if (message->FindRect ("be:_frame", &fr) != B_OK)
	{
		fr.Set (0, 0, 0, 0);
	}
	if (message->WasDropped())
	{
		verbose (2, "Dropped...\n");
		BPoint offset = B_ORIGIN;
		if (fromBecasso && !message->FindPoint ("_drop_offset_", &offset))
		{
//			printf ("Offset by (%f, %f)\n", offset.x, offset.y);
		}
		//target = message->DropPoint (&pt);	<= doesn't seem to work OK in R4
		target = message->DropPoint();
		ConvertFromScreen (&target);
		target.x -= offset.x;
		target.y -= offset.y;
		if (!message->FindPoint ("be:_source_point", &pt))
		{
			pt.x -= fr.left;
			pt.y -= fr.top;
		}
	}
	else if (!message->FindPoint ("be:destination_point", &target))
	{
		/* target is OK */
	}
	else
	{
		target = B_ORIGIN;
	}
	target.x -= pt.x;
	target.y -= pt.y;
	if (target.x < 0)
	{
		fr.left -= target.x;
		target.x = 0;
	}
	if (target.y < 0)
	{
		fr.top -= target.y;
		target.y = 0;
	}
	//fr.PrintToStream();
	if ((fr.left > fr.right) || (fr.top > fr.bottom))
		goto outside;

	if (!drop_map && !message->IsSourceRemote() && !message->FindPointer ("be:_bitmap_ptr", (void**) &bitmap_ptr))
	{
//		printf ("SourceRemote && FindPointer\n");
		/* this is one big race condition -- what if the source window went away? */
		/* make sure we have bitmap in our color space */
		if (message->FindRect ("be:_frame", &fr))
		{
			fr = bitmap_ptr->Bounds();
		}
		BRect temp(fr);
		fr.OffsetTo (B_ORIGIN);
		drop_map = new BBitmap (fr, B_RGBA32, true);
		BView *v = new BView (fr, "_", B_FOLLOW_NONE, B_WILL_DRAW);
		drop_map->Lock();
		drop_map->AddChild (v);
		v->DrawBitmap (bitmap_ptr, temp, fr);
		v->Sync();
		drop_map->RemoveChild (v);
		drop_map->Unlock();
//		printf ("Filled in drop_map from local ptr\n");
		delete v;
	}
	else if (!drop_map)
	{
		if (message->what == B_ARCHIVED_OBJECT)
		{
			verbose (2, "B_ARCHIVED_OBJECT\n");
			if (!validate_instantiation (message, "BBitmap"))
				goto err;
			drop_map = dynamic_cast<BBitmap *> (instantiate_object (message));
			if (!drop_map)
				goto err;
		}
		else if (message->HasRef ("refs"))
		{
			verbose (2, "HasRef\n");
			BBitmapStream output;
			entry_ref ref;
			if (message->FindRef ("refs", &ref))
				goto err;
			BFile input;
			if (input.SetTo (&ref, O_RDONLY))
				goto err;
			if (BTranslatorRoster::Default()->Translate (&input, NULL, NULL, &output, B_TRANSLATOR_BITMAP))
				goto err;
			if (output.DetachBitmap (&drop_map))
				goto err;
			myWindow->Lock();
			myWindow->setPaste (true);
			myWindow->Unlock();
			extern BBitmap *clip;
			if (clip)
			{
				clip->Lock();
				delete clip;
			}
			clip = drop_map;
			SetupUndo (M_DRAW);
			Paste (true);
			return;
		}
		else
		{
			// Extended DnD protocol.  The message probably contains "promises" only.
			verbose (1, "Extended DnD\n");
			extern int DebugLevel;
			if (DebugLevel > 2)
				message->PrintToStream();

			BMessage reply (B_COPY_TARGET);
			int32 numTypes;
			type_code typeFound;
			message->GetInfo ("be:types", &typeFound, &numTypes);
			const char **types = new const char *[numTypes];
			int32 i;
			for (i = 0; i < numTypes; i++)
				message->FindString ("be:types", i, &(types[i]));

			// Look for types we like, in the order Becasso, PNG, Targa.
			int32 iBecasso = -1;
			int32 iPNG = -1;
			int32 iTarga = -1;
			for (i = 0; i < numTypes; i++)
			{
				if (!strcasecmp (types[i], "image/x-becasso"))
					iBecasso = i;
				if (!strcasecmp (types[i], "image/x-png")
				 || !strcasecmp (types[i], "image/png"))
				 	iPNG = i;
				if (!strcasecmp (types[i], "image/targa")
				 || !strcasecmp (types[i], "image/x-targa"))
				 	iTarga = i;
			}
			if (iBecasso != -1)
				reply.AddString ("be:types", types[iBecasso]);
			else if (iPNG != -1)
				reply.AddString ("be:types", types[iPNG]);
			else if (iTarga != -1)
				reply.AddString ("be:types", types[iTarga]);
			else
			{
				fprintf (stderr, "Neither image/x-becasso, image/(x-)png nor image/(x-)targa supported!\n");
				delete [] types;
				return;
			}
			if (DebugLevel >= 2)
			{
				printf ("Sending reply:\n");
				reply.PrintToStream();
			}
			message->SendReply (&reply, this);
			delete [] types;
			return;
		}
		if (message->FindRect ("be:_frame", &fr))
		{
			fr = drop_map->Bounds();
		}
	}
	w = (int) fr.Width();
	if (w + target.x > fCanvasFrame.right)
	{
		w = (int)(fCanvasFrame.right - target.x);
	}
	h = (int) fr.Height();
	if (h + target.y > fCanvasFrame.bottom)
	{
		h = (int)(fCanvasFrame.bottom - target.y);
	}
	if (drop_map)
	// We finally have our dropped bitmap
	{
		//printf ("We're here!\n");
		fr.Set (target.x, target.y, target.x + w, target.y + h);
//		SetSelection (fr, drop_map);
//		delete fDithered;
//		fDithered = NULL;
		SetupUndo (M_DRAW);
		currentLayer()->Lock();
//		drawView->SetDrawingMode (B_OP_ALPHA);
//		drawView->DrawBitmap (drop_map, fr.LeftTop());
		AddWithAlpha (drop_map, currentLayer(), int (target.x), int (target.y));
		currentLayer()->Unlock();
		Invalidate (/*fr*/);
	}
outside:
		;

err:
	delete drop_map;
}

void CanvasView::CopyTarget (BMessage *message)
{
	extern int DebugLevel;
	if (DebugLevel)
	{
		printf ("B_COPY_TARGET\n");
		message->PrintToStream();
	}

	extern bool gGlobalAlpha;
	if (!gGlobalAlpha)
	{
#if SAVE_DISABLED
		int32 result;
		BAlert *alert = new BAlert ("Disabled",
		 lstring (158, "Saving is disabled in this demo version of Becasso.\nOn http://www.sumware.demon.nl you can find information on obtaining the full version.\n\nThanks for your interest in Becasso!"),
		 lstring (131, "Cancel"), lstring (159, "Visit URL"), NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		result = alert->Go();
		if (result == 1)
			system ("NetPositive http://www.sumware.demon.nl &");
		return;
#else
		LockLooper();
		BRect rect = Window()->Frame();
		UnlockLooper();
		rect.left += 16;
		rect.top += 16;
		rect.right = rect.left + 340;
		rect.bottom = rect.top + 80;
		Window()->Hide();
		(new NagWindow (rect))->Go();
		Window()->Show();
		Invalidate();
#endif
	}

	extern BBitmap *clip;
	const char *type = NULL;
	if (!message->FindString ("be:types", &type) && clip)
	{
		BBitmapStream strm (clip);
		if (!strcasecmp (type, B_FILE_MIME_TYPE))
		{
			const char *name;
			entry_ref dir;
			if (!message->FindString ("be:filetypes", &type) &&
				!message->FindString ("name", &name) &&
				!message->FindRef ("directory", &dir))
			{
				//	write file
				uint32 type_code = TypeCodeForMIME (type);
				BDirectory d (&dir);
				BFile f (&d, name, O_RDWR | O_TRUNC);
				BTranslatorRoster::Default()->Translate (&strm, NULL, NULL, &f, type_code);
				BNodeInfo ni (&f);
				ni.SetType (type);
			}
		}
		else if (!message->FindString ("be:types", &type))
		{
			//	put in message
			uint32 type_code = TypeCodeForMIME (type);
			BMessage msg (B_MIME_DATA);
			BMallocIO f;
			BTranslatorRoster::Default()->Translate (&strm, NULL, NULL, &f, type_code);
			msg.AddData (type, B_MIME_TYPE, f.Buffer(), f.BufferLength());
			message->SendReply (&msg);
		}
		else
		{
			BMessage msg (B_MESSAGE_NOT_UNDERSTOOD);
			message->SendReply (&msg);
		}
		strm.DetachBitmap (&clip);
	}
}

void CanvasView::TrashTarget (BMessage *message)
{
	printf ("B_TRASH_TARGET\n");
	message->PrintToStream();
}

void CanvasView::MouseDown (BPoint point)
{
//	printf ("MouseDown\n");
	if (!fTranslating && !fRotating)
		SetMouseEventMask (B_POINTER_EVENTS, B_SUSPEND_VIEW_FOCUS | B_NO_POINTER_HISTORY);
		// Hmmm, ik krijg toch een pointer history zo!

	uint32 buttons = Window()->CurrentMessage()->FindInt32 ("buttons");
//	printf ("buttons = %lx\n", buttons);
	Position position;
	position.fPoint     = point;
	position.fButtons   = buttons;
	position.fTilt      = B_ORIGIN;
	position.fProximity = false;
	position.fPressure  = 255;
	MouseOrTablet (&position);

	// Remove all waiting MouseMoved messages
	if (myWindow->LockWithTimeout (50000) == B_OK)
	{
		BMessageQueue *queue = myWindow->MessageQueue();
		BMessage *msg = NULL;
		int count = 0;
		do {
			msg = queue->FindMessage (B_MOUSE_MOVED, 0);
			if (msg)
			{
				count++;
				queue->RemoveMessage (msg);
			}
		} while (msg);
//		printf ("Removed %d MouseMoved messages\n", count);
		myWindow->Unlock();
	}
	else
		printf ("Hah!  Couldn't get lock!\n");
}

void CanvasView::MouseUp (BPoint /*point*/)
{
//	printf ("MouseUp\n");
	fMouseDown = false;
	if (!entry)
		myWindow->posview->SetPoint (-1, -1);

	extern PicMenuButton *tool;
	extern Becasso *mainapp;
	int32 currentTool = tool->selected();
	if (currentTool == T_CLONE)
	{
		mainapp->setCross();
		BPoint sp = ConvertToScreen (BPoint (fCC.pos.x*fScale, fCC.pos.y*fScale));
		set_mouse_position (int (sp.x), int (sp.y));
		Invalidate();
	}
}

void CanvasView::MouseMoved (BPoint point, uint32 transit, const BMessage *msg)
{
	// clock_t start, end;
	// start = clock();
	point.x /= fScale;
	point.y /= fScale;
	extern Becasso *mainapp;
//	extern bool inDrag, inPaste;

	int32 buttons;
	Window()->CurrentMessage()->FindInt32 ("buttons", &buttons);
	if (transit == B_EXITED_VIEW)
	{
		mainapp->setHand();
		mouse_on_canvas = false;
		Invalidate();
	}
	else if (transit == B_ENTERED_VIEW && Window()->IsActive() && !msg)
	{
		// Fix for the fact that we don't get MouseUp()s outside our view
//		BPoint point;
//		uint32 buttons;
//		GetMouse (&point, &buttons, false);

//		fMouseDown = false;

//		printf ("%sinDrag, %sinPaste\n", inDrag ? "" : "!", inPaste ? "" : "!");
		extern PicMenuButton *tool;
		int32 currentTool = tool->selected();
		if ((transformerOpen && !(transformerOpen->DoesPreview() & PREVIEW_MOUSE))
		 || (generatorOpen && !(generatorOpen->DoesPreview() & PREVIEW_MOUSE)))
			mainapp->setCCross();
		else if (fTranslating)
			mainapp->setMover();
		else if (fRotating)
			mainapp->setRotator();
		else
			mainapp->setCross();
		mouse_on_canvas = true;
		if (currentTool != prevTool)
		{
			entry = 0;
			prevTool = currentTool;
		}
		if (buttons && !fMouseDown)
		{
//			printf ("Drawing started outside...\n");
//			MouseDown (point);
		}

		Invalidate();
	}
	else
	{
		if (buttons && !fMouseDown)	// MouseDown missed!  (This happens a LOT!)
		{
			return;
		}
//		MouseDown (point);

		fMouseDown = buttons;
		if (fMouseDown && Window()->IsActive()
			 && !msg // Don't draw inside a drag!
			 // The following disables spraycan and brush to be used when
			 // a previewing add-on is open (to fix a bug - this is _not_ ideal!!)
			 && (!( //(filterOpen && filterOpen->DoesPreview())
			 	   (generatorOpen)// && (generatorOpen->DoesPreview() & PREVIEW_MOUSE))
			 	|| (transformerOpen))))// && (transformerOpen->DoesPreview() & PREVIEW_MOUSE)))))
		{
			currentLayer()->Lock();
			selection->Lock();
			if (modifiers() & B_OPTION_KEY)	// Color Picker / DragScroll
			{
				if (modifiers() & B_SHIFT_KEY)	// DragScroll
				{
					mainapp->setGrab (true, true);
					fDragScroll = true;
					fPicking = false;
					BPoint p = ConvertToScreen (BPoint (point.x*fScale, point.y*fScale));
					// printf ("new pos = %f, %f\n", fPoint.x - p.x, fPoint.y - p.y);
					fBGView->ScrollTo (BPoint (fPoint.x - p.x, fPoint.y - p.y));
				}
				else	// Color Picker
				{
					extern ColorMenuButton *hicolor, *locolor;
					if (buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY))
						hicolor->set (getColor (point));
					if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
						locolor->set (getColor (point));
					fPicking = true;
					fDragScroll = false;
				}
			}
			else if (!fPicking)
			{
				extern PicMenuButton *tool;
				int32 currentTool = tool->selected();
				if (currentTool == T_BRUSH)
				{
					extern PicMenuButton *mode;
					Position position;
					GetPosition (&position);
					if (DebugLevel > 6)
						printf ("P = %d\n", position.fPressure);
					tBrushM (mode->selected(), point, position.fButtons, position.fPressure, position.fTilt);
					fMouseDown = position.fButtons; // Sometimes MouseUp() is missed!
				}
				else if (currentTool == T_CLONE)
				{
					extern PicMenuButton *mode;
					Position position;
					GetPosition (&position);
					if (DebugLevel > 6)
						printf ("P = %d\n", position.fPressure);
					tCloneM (mode->selected(), point, position.fButtons, position.fPressure, position.fTilt);
					fMouseDown = position.fButtons; // Sometimes MouseUp() is missed!
				}
				else if (currentTool == T_SPRAYCAN)
				{
					extern PicMenuButton *mode;
					Position position;
					GetPosition (&position);
					tSpraycanM (mode->selected(), point, position.fButtons, position.fPressure, position.fTilt);
					fMouseDown = position.fButtons; // Sometimes MouseUp() is missed!
				}
			}
			selection->Unlock();
			currentLayer()->Unlock();
		}
	}
	// end = clock();
	// printf ("MouseMoved took %d ms\n", end - start);
}


void CanvasView::MouseOrTablet (Position *position)
{
	fMouseDown = true;
	BPoint point   = position->fPoint;
	uint32 buttons = position->fButtons;
	point.x /= fScale;
	point.y /= fScale;
	extern bool inPaste;
	extern PicMenuButton *tool, *mode;
	extern ColorMenuButton *locolor, *hicolor;
	extern Becasso *mainapp;
	int32 currentMode = mode->selected();
	int32 currentTool = tool->selected();
	currentLayer()->Lock();
	selection->Lock();
	if (fTranslating)
	{
		do_translateLayer (fTranslating - 1, currentMode);
		fTranslating = 0;
		mainapp->setCross();
	}
	else if (fRotating)
	{
		do_rotateLayer (fRotating - 1, currentMode);
		fRotating = 0;
		mainapp->setCross();
	}
	else if (modifiers() & B_OPTION_KEY)	// Color Picker	/ DragScroll
	{
		if (modifiers() & B_SHIFT_KEY)	// DragScroll
		{
			fDragScroll = true;
			BPoint lt = Frame().LeftTop();
			BPoint p = ConvertToScreen (BPoint (point.x*fScale, point.y*fScale));
			fPoint = BPoint (p.x - lt.x, p.y - lt.y);
//			printf ("New drag: ");
//			fPoint.PrintToStream();
		}
		else
		{
			if (buttons & B_PRIMARY_MOUSE_BUTTON)
				hicolor->set (getColor (point));
			if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
				locolor->set (getColor (point));
		}
	}
	else if ((modifiers() & B_SHIFT_KEY) & selchanged)	// Cut/Copy/Paste
	{
		if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
		{
			Cut();
			eDrag (point, buttons);
		}
		else if (buttons & B_PRIMARY_MOUSE_BUTTON)
		{
			Copy();
			eDrag (point, buttons);
		}
	}
	else if ((modifiers() & B_COMMAND_KEY) && filterOpen && filterOpen->DoesPreview())	// AddOn Preview
	{
		eFilter (point, buttons);
	}
	else if (generatorOpen && (generatorOpen->DoesPreview() & PREVIEW_MOUSE))
	{
		eGenerate (point, buttons);
	}
	else if (transformerOpen && (transformerOpen->DoesPreview() & PREVIEW_MOUSE))
	{
		eTransform (point, buttons);
	}
	else if (inPaste)
		ePaste (point, buttons);
	else if (!transformerOpen && !generatorOpen)
	{
		switch (currentTool)
		{
		case T_BRUSH:
			tBrush (currentMode, point, buttons);
			break;
		case T_ERASER:
			tEraser (currentMode, point, buttons);
			break;
		case T_FILL:
			tFill (currentMode, point, buttons);
			break;
		case T_TEXT:
			tText (currentMode, point, buttons);
			break;
		case T_SPRAYCAN:
			tSpraycan (currentMode, point, buttons);
			break;
		case T_CLONE:
			tClone (currentMode, point, buttons);
			break;
		case T_FREEHAND:
			tFreehand (currentMode, point, buttons);
			break;
		case T_LINES:
			myWindow->posview->SetPoint (point.x, point.y);
			myWindow->posview->DoRadius (true);
			tLines (currentMode, point, buttons);
			break;
		case T_POLYBLOB:
			tPolyblob (currentMode, point, buttons);
			break;
		case T_POLYGON:
			myWindow->posview->SetPoint (point.x, point.y);
			tPolygon (currentMode, point, buttons);
			break;
		case T_RECT:
			myWindow->posview->SetPoint (point.x, point.y);
			tRect (currentMode, point, buttons);
			break;
		case T_RRECT:
			myWindow->posview->SetPoint (point.x, point.y);
			tRoundRect (currentMode, point, buttons);
			break;
		case T_CIRCLE:
			myWindow->posview->SetPoint (point.x, point.y);
			myWindow->posview->DoRadius (true);
			tCircle (currentMode, point, buttons);
			break;
		case T_ELLIPSE:
			myWindow->posview->SetPoint (point.x, point.y);
			tEllipse (currentMode, point, buttons);
			break;
		default:
			fprintf (stderr, "Unknown Tool selected...\n");
			break;
		}
	}
	selectionView->Sync();
	drawView->Sync();
	selection->Unlock();
	currentLayer()->Unlock();
//	printf ("MoT done\n");
}

void CanvasView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes == 1 && !myWindow->MenuOnScreen())
	{
		switch (*bytes)
		{
		case B_TAB:		// Switch mode
		{
			extern PicMenuButton *mode;
			if (mode->selected() == M_DRAW)
				mode->set (M_SELECT);
			else
				mode->set (M_DRAW);
			Invalidate();
			break;
		}
		case B_ENTER:	// "Attach" clip to mouse cursor or keep initial distance
		{
			extern bool inPaste;
			if (inPaste)
			{
				extern long pasteX, pasteY, fPasteX, fPasteY;
				if (pasteX || pasteY)
				{
					pasteX = 0;
					pasteY = 0;
				}
				else
				{
					pasteX = fPasteX;
					pasteY = fPasteY;
				}
				BPoint point;
				uint32 buttons;
				GetMouse (&point, &buttons);
				ePasteM (point);
			}
			break;
		}
		case B_ESCAPE:	// Bail out of whatever you're doing
		{
			extern bool inPaste;
			if (inPaste)
			{
				currentLayer()->Lock();
				drawView->DrawBitmap (cutbg, prevPaste.LeftTop());
				currentLayer()->Unlock();
				cutbg->RemoveChild (cutView);
				delete cutbg;
				delete cutView;
				cutbg = NULL;
				inPaste = false;
				Invalidate();
			}
			else if (entry)
			{
				entry = 0;
				Invalidate();
			}
			fTranslating = 0;
			fRotating = 0;
			extern Becasso *mainapp;
			mainapp->setCross();
			mainapp->setReady();
			break;
		}
		case B_LEFT_ARROW:
		{
			uint32 buttons;
			BPoint sp;
			GetMouse (&sp, &buttons);
			sp = ConvertToScreen (BPoint (sp.x*fScale, sp.y*fScale));
			float add = modifiers() & B_CONTROL_KEY ? 10 : 1;
			set_mouse_position (int (sp.x - fScale*add), int (sp.y));
			break;
		}
		case B_RIGHT_ARROW:
		{
			uint32 buttons;
			BPoint sp;
			GetMouse (&sp, &buttons);
			sp = ConvertToScreen (BPoint (sp.x*fScale, sp.y*fScale));
			float add = modifiers() & B_CONTROL_KEY ? 10 : 1;
			set_mouse_position (int (sp.x + (fScale >= 1 ? fScale : 1)*add), int (sp.y));
			break;
		}
		case B_UP_ARROW:
		{
			uint32 buttons;
			BPoint sp;
			GetMouse (&sp, &buttons);
			sp = ConvertToScreen (BPoint (sp.x*fScale, sp.y*fScale));
			float add = modifiers() & B_CONTROL_KEY ? 10 : 1;
			set_mouse_position (int (sp.x), int (sp.y - fScale*add));
			break;
		}
		case B_DOWN_ARROW:
		{
			uint32 buttons;
			BPoint sp;
			GetMouse (&sp, &buttons);
			sp = ConvertToScreen (BPoint (sp.x*fScale, sp.y*fScale));
			float add = modifiers() & B_CONTROL_KEY ? 10 : 1;
			set_mouse_position (int (sp.x), int (sp.y + (fScale >= 1 ? fScale : 1)*add));
			break;
		}
		case B_SPACE:
		{
			uint32 buttons;
			BPoint point;
			GetMouse (&point, &buttons);

			Position position;
			position.fPoint     = BPoint (point.x*fScale, point.y*fScale);
			position.fButtons   = (modifiers() & B_SHIFT_KEY) ? B_SECONDARY_MOUSE_BUTTON : B_PRIMARY_MOUSE_BUTTON;
			position.fTilt      = B_ORIGIN;
			position.fProximity = false;
			position.fPressure  = 255;
			MouseOrTablet (&position);
			MouseUp (point);
			break;
		}
		default:
			inherited::KeyDown (bytes, numBytes);
		}
	}
	else
		inherited::KeyDown (bytes, numBytes);
}


void CanvasView::SetupUndo (int32 mode)
{
//	if (didPreview == 2)
//	{
//		// This means we're in a preview-train (the user is sliding a slider on an
//		// add-on window, for instance).  We don't want to flood the undo buffer...
//		return;
//	}
//	else if (didPreview == 2)
//	{
//		// This means "something else" has happened since the last preview.
//		// We'd better save now, and next time the add-on does something too!
//		didPreview = 0;
//	}
	Sync();
	currentLayer()->Lock();
	drawView->Sync();
	currentLayer()->Unlock();
	extern Becasso *mainapp;
	int32 type;
	switch (mode)
	{
	case M_SELECT:
		type = UNDO_SELECT;
		break;
	case M_DRAW:
		type = UNDO_DRAW;
		break;
	case M_MERGE:
		type = UNDO_MERGE;
		break;
	case M_DELLAYER:
		type = UNDO_LDEL;
		break;
	case M_RESIZE:		// For the time being, just flush the undo array.
						// Should I warn the user about this?
		indexUndo = -1;
		maxUndo = 0;
		for (int i = 0; i < MAX_UNDO; i++)
		{
			delete undo[i].bitmap;
			delete undo[i].sbitmap;
			undo[i].bitmap  = NULL;
			undo[i].sbitmap = NULL;
		}
		myWindow->Lock();
		myWindow->setMenuItem (B_UNDO, false);
		myWindow->setMenuItem ('redo', false);
		myWindow->Unlock();
		return;
	case M_DRAW_SELECT:
		type = UNDO_BOTH;
		break;
	default:
		fprintf (stderr, "SetupUndo: Unknown mode [%li]...\n", mode);
		return;
	}
	if (type == UNDO_SELECT || type == UNDO_BOTH)
	{
		selchanged = true;
		myWindow->Lock();
		myWindow->setMenuItem (B_CUT, true);
		myWindow->setMenuItem (B_COPY, true);
		myWindow->setMenuItem ('csel', true);
		myWindow->setMenuItem ('sund', true);
		myWindow->setMenuItem ('sinv', true);
		myWindow->Unlock();
		sel = true;
	}
	mainapp->setBusy();
	changed = true;
//	extern AttribDraw *mAttribDraw;
	maxUndo = max_undo();
	int32 prevtype = undo[indexUndo].type;
	if ((prevtype == UNDO_DRAW && type == UNDO_SELECT) || (prevtype == UNDO_SELECT && type == UNDO_DRAW))
	{
		type = UNDO_SWITCH;
	}
	if (indexUndo + 1 == maxUndo - 1)
	{
//		printf ("Deleting undo[0]: %p, %p (indexUndo = %d)\n", undo[0].bitmap, undo[0].sbitmap, indexUndo);
		delete undo[0].bitmap;
		delete undo[0].sbitmap;
		for (int i = 1; i < maxUndo; i++)
		{
			undo[i - 1] = undo[i];
		}
		undo[maxUndo - 1].bitmap = 0;
		undo[maxUndo - 1].sbitmap = 0;
	}
	else
		indexUndo++;

//	printf ("Killing the redo buffer...\n");
	for (int i = indexUndo; i < MAX_UNDO; i++)
	{
		delete undo[i].bitmap;
		delete undo[i].sbitmap;
		undo[i].bitmap  = NULL;
		undo[i].sbitmap = NULL;
	}
	maxIndexUndo = indexUndo;

	undo_entry &u = undo[indexUndo];	// alias to save typing.
//	printf ("Copying the data\n");
	if (type == UNDO_SELECT)
	{
		selection->Lock();
		u.sbitmap = new SBitmap (selection);
		u.bitmap = NULL;
//		printf ("Setup undo: selection -> undo[%i]\n", indexUndo);
		selection->Unlock();
		u.layer = -1;
	}
	else if (type == UNDO_DRAW)
	{
		currentLayer()->Lock();
		u.bitmap = new SBitmap (currentLayer());
		u.sbitmap = NULL;
//		printf ("Setup undo: layer[%i] -> undo[%i]\n", fCurrentLayer, indexUndo);
		currentLayer()->Unlock();
		u.layer = fCurrentLayer;
	}
	else if (type == UNDO_BOTH)
	{
		selection->Lock();
		u.sbitmap = new SBitmap (selection);
		u.bitmap = new SBitmap (currentLayer());
//		printf ("Setup undo: selection -> undo[%i]\n", indexUndo);
		selection->Unlock();
		currentLayer()->Lock();
//		printf ("Setup undo: layer[%i] -> undo[%i]\n", fCurrentLayer, indexUndo);
		currentLayer()->Unlock();
		u.layer = fCurrentLayer;
	}
	else if (type == UNDO_SWITCH)
	{
		currentLayer()->Lock();
		u.bitmap = new SBitmap (currentLayer());
//		printf ("Setup undo: layer[%i] & selection -> undo[%i]\n", fCurrentLayer, indexUndo);
		currentLayer()->Unlock();
		u.layer = fCurrentLayer;
		u.sbitmap = new SBitmap (selection);
		selection->Lock();
		selection->Unlock();
	}
	else if (type == UNDO_MERGE)
	{
		layer[fIndex1]->Lock();
		u.bitmap = new SBitmap (layer[fIndex1]);
		layer[fIndex1]->Unlock();
		layer[fIndex2]->Lock();
		u.sbitmap = new SBitmap (layer[fIndex2]);
		layer[fIndex2]->Unlock();
		u.layer        = fIndex1;
		u.next         = fIndex2;
		u.fMode        = layer[fIndex2]->getMode();
		u.fGlobalAlpha = layer[fIndex2]->getGlobalAlpha();
		u.fHide        = layer[fIndex2]->IsHidden();
		strcpy (u.fName, layer[fIndex2]->getName());
	}
	else if (type == UNDO_LDEL)
	{
		layer[fIndex2]->Lock();
		u.bitmap = new SBitmap (layer[fIndex2]);
		layer[fIndex2]->Unlock();
		u.layer        = fIndex2;
		u.fMode        = layer[fIndex2]->getMode();
		u.fGlobalAlpha = layer[fIndex2]->getGlobalAlpha();
		u.fHide        = layer[fIndex2]->IsHidden();
		strcpy (u.fName, layer[fIndex2]->getName());
	}
	u.type = type;
	myWindow->Lock();
	myWindow->setMenuItem (B_UNDO, true);
	myWindow->setMenuItem ('redo', false);
	myWindow->Unlock();
	mainapp->setReady();
}

void CanvasView::Undo (bool advance, bool menu)
{
	if (menu)
	{
		if (filterOpen || transformerOpen || generatorOpen)
		{
			beep();
			return;
		}
	}
	extern PicMenuButton *mode;
	int32 currentmode = mode->selected();
	if (entry)
	{
		entry = 0;
		Draw (fCanvasFrame);
		return;
	}
	changed = true;
	extern Becasso *mainapp;
//	extern AttribDraw *mAttribDraw;
	mainapp->setBusy();
	maxUndo = max_undo();

	if (indexUndo < 0)
	{
		fprintf (stderr, "Empty undo buffer!\n");
		indexUndo = -1;
		return;	// Sorry, undo buffer is empty...
	}

	if (indexUndo == maxIndexUndo && advance)	// First undo
	{
//		printf ("First undo - saving current state\n");
		int32 prevtype = undo[indexUndo].type;
		int32 type = 0;
		switch (currentmode)
		{
		case M_DRAW:
			type = UNDO_DRAW;
			break;
		case M_SELECT:
			type = UNDO_SELECT;
			break;
		default:
			fprintf (stderr, "Undo: Unknown mode [%li]...\n", currentmode);
		}
		if ((prevtype == UNDO_DRAW && type == UNDO_SELECT) || (prevtype == UNDO_SELECT && type == UNDO_DRAW))
		{
			type = UNDO_SWITCH;
		}
		indexUndo++;
		maxIndexUndo++;
		if (indexUndo == maxUndo)
		{
//			printf ("Shifting the undo buffer\n");
			delete undo[0].bitmap;
			delete undo[0].sbitmap;
			for (int i = 1; i < maxUndo; i++)
			{
				undo[i - 1] = undo[i];
			}
			indexUndo--;
			maxIndexUndo--;
		}

		// Copy the bitmap into the undo buffer
		undo_entry &u = undo[indexUndo]; // Alias to save typing
		if (type == UNDO_SELECT)
		{
//			printf ("Current selection -> undo[%i]\n", indexUndo);
			u.sbitmap = new SBitmap (selection);
			u.bitmap = NULL;
			u.layer = -1;
		}
		else if (type == UNDO_DRAW)
		{
//			printf ("Current layer[%i] -> undo[%i]\n", fCurrentLayer, indexUndo);
			u.bitmap = new SBitmap (currentLayer());
			u.sbitmap = NULL;
			u.layer = fCurrentLayer;
		}
		else if (type == UNDO_SWITCH)
		{
//			printf ("Current layer[%i] & selection -> undo[%i]\n", fCurrentLayer, indexUndo);
			u.bitmap = new SBitmap (currentLayer());
			u.layer = fCurrentLayer;
			u.sbitmap = new SBitmap (selection);
		}
		// N.B. UNDO_MERGE can't happen here...
		u.type = type;
		indexUndo--;
	}

	// Copy the undo buffer into the bitmap
//	printf ("Restoring from undo[%i];\n", indexUndo);
	undo_entry &u = undo[indexUndo];	// Alias to save typing
	if (u.type == UNDO_DRAW)
	{
//		printf ("undo[%i] -> layer[%i]\n", indexUndo, u.layer);
		layer[u.layer]->Lock();
		memcpy (layer[u.layer]->Bits(), u.bitmap->Bits(), layer[u.layer]->BitsLength());
		layer[u.layer]->Unlock();
	}
	else if (u.type == UNDO_SELECT)
	{
//		printf ("undo[%i] -> selection\n", indexUndo);
		selection->Lock();
		memcpy (selection->Bits(), u.sbitmap->Bits(), selection->BitsLength());
		selection->Unlock();
		selchanged = true;
		sel = true;
	}
	else if (u.type == UNDO_BOTH)
	{
		layer[u.layer]->Lock();
		memcpy (layer[u.layer]->Bits(), u.bitmap->Bits(), layer[u.layer]->BitsLength());
		layer[u.layer]->Unlock();
		selection->Lock();
		memcpy (selection->Bits(), u.sbitmap->Bits(), selection->BitsLength());
		selection->Unlock();
		selchanged = true;
		sel = true;
	}
	else if (u.type == UNDO_SWITCH)
	{
//		printf ("undo[%i] -> layer[%i] & selection\n", indexUndo, u.layer);
		selection->Lock();
		memcpy (selection->Bits(), u.sbitmap->Bits(), selection->BitsLength());
		selection->Unlock();
		layer[u.layer]->Lock();
		memcpy (layer[u.layer]->Bits(), u.bitmap->Bits(), layer[u.layer]->BitsLength());
		layer[u.layer]->Unlock();
		selchanged = true;
		sel = true;
	}
	else if (u.type == UNDO_MERGE)
	{
		layer[u.layer]->Lock();
		memcpy (layer[u.layer]->Bits(), u.bitmap->Bits(), layer[u.layer]->BitsLength());
		layer[u.layer]->Unlock();
		insertLayer (u.next, "Restored");
		layer[u.next]->Lock();
		memcpy (layer[u.next]->Bits(), u.sbitmap->Bits(), layer[u.next]->BitsLength());
		layer[u.next]->setName (u.fName);
		layer[u.next]->Hide (u.fHide);
		layer[u.next]->setGlobalAlpha (u.fGlobalAlpha);
		layer[u.next]->setMode (u.fMode);
		layer[u.next]->Unlock();
	}
	else if (u.type == UNDO_LDEL)
	{
		insertLayer (u.layer, "Restored");
		layer[u.layer]->Lock();
		memcpy (layer[u.layer]->Bits(), u.bitmap->Bits(), layer[u.layer]->BitsLength());
		layer[u.layer]->setName (u.fName);
		layer[u.layer]->Hide (u.fHide);
		layer[u.layer]->setGlobalAlpha (u.fGlobalAlpha);
		layer[u.layer]->setMode (u.fMode);
		layer[u.layer]->Unlock();
	}
	if (advance)
	{
		myWindow->Lock();
		myWindow->setMenuItem (B_UNDO, indexUndo > 0);
		myWindow->setMenuItem ('redo', true);
		myWindow->Unlock();
		Invalidate();
		indexUndo--;
	}
	mainapp->setReady();
}

void CanvasView::Redo (bool menu)
{
	if (menu)
	{
		if (filterOpen || transformerOpen || generatorOpen)
		{
			beep();
			return;
		}
	}
	if (indexUndo + 1 >= maxIndexUndo)
	{
		fprintf (stderr, "Nothing to redo!\n");
		return;	// Nothing to redo!
	}
	indexUndo++;
	changed = true;
	extern Becasso *mainapp;
	mainapp->setBusy();

	// Copy the redo buffer into the bitmap
//	printf ("Restoring from undo[%i];\n", indexUndo);
	undo_entry &u = undo[indexUndo + 1];	// Alias to save typing
	if (u.type == UNDO_DRAW)
	{
		layer[u.layer]->Lock();
		memcpy (layer[u.layer]->Bits(), u.bitmap->Bits(), layer[u.layer]->BitsLength());
//		printf ("undo[%i] -> layer[%i]\n", indexUndo + 1, u.layer);
		layer[u.layer]->Unlock();
	}
	else if (u.type == UNDO_SELECT)
	{
		selection->Lock();
		memcpy (selection->Bits(), u.sbitmap->Bits(), selection->BitsLength());
//		printf ("undo[%i] -> selection\n", indexUndo + 1);
		selection->Unlock();
		selchanged = true;
		sel = true;
	}
	else if (u.type == UNDO_BOTH)
	{
		layer[u.layer]->Lock();
		memcpy (layer[u.layer]->Bits(), u.bitmap->Bits(), layer[u.layer]->BitsLength());
//		printf ("undo[%i] -> layer[%i]\n", indexUndo + 1, u.layer);
		layer[u.layer]->Unlock();
		selection->Lock();
		memcpy (selection->Bits(), u.sbitmap->Bits(), selection->BitsLength());
//		printf ("undo[%i] -> selection\n", indexUndo + 1);
		selection->Unlock();
		selchanged = true;
		sel = true;
	}
	else if (u.type == UNDO_SWITCH)
	{
		layer[u.layer]->Lock();
		memcpy (layer[u.layer]->Bits(), u.bitmap->Bits(), layer[u.layer]->BitsLength());
//		printf ("undo[%i] -> layer[%i] & selection\n", indexUndo + 1, u.layer);
		layer[u.layer]->Unlock();
		selection->Lock();
		memcpy (selection->Bits(), u.sbitmap->Bits(), selection->BitsLength());
		selection->Unlock();
	}
	else if (u.type == UNDO_MERGE)
	{
		Merge (layer[u.layer], layer[u.next], fCanvasFrame, false);
		do_removeLayer (u.next);
	}
	else if (u.type == UNDO_LDEL)
	{
		do_removeLayer (u.layer);
	}
	myWindow->Lock();
	myWindow->setMenuItem (B_UNDO, true);
	myWindow->setMenuItem ('redo', indexUndo + 1 < maxIndexUndo);
	myWindow->Unlock();
	Invalidate();
	mainapp->setReady();
}


void CanvasView::Cut ()
{
	extern long pasteX, pasteY, fPasteX, fPasteY;
	extern Becasso *mainapp;
	extern BBitmap *clip;
	mainapp->setBusy();
	SetupUndo (M_DRAW);
	if (clip)
	{
		clip->Lock();
		delete clip;
	}

	uint32 buttons;
	BPoint point;
	GetMouse (&point, &buttons, true);
	BRect selectrect = GetSmallestRect (selection);
	fPasteX = long (selectrect.left - point.x);
	fPasteY = long (selectrect.top - point.y);
	pasteX = 0;
	pasteY = 0;
	BRect cutrect = selectrect;
	cutrect.OffsetTo (B_ORIGIN);
	clip = new BBitmap (cutrect, B_RGBA32);

	CutOrCopy (currentLayer(), clip, selection, int (selectrect.left), int (selectrect.top), true);

	myWindow->Lock();
	myWindow->setPaste (true);
	myWindow->Unlock();
	BMessage *clipmsg = new BMessage ('bits');
	clip->Archive (clipmsg, false);
	if (be_clipboard->Lock())
	{
		be_clipboard->Clear();
		BMessage *clipper = be_clipboard->Data();
		clipper->AddMessage ("image/x-be-bitmap", clipmsg);
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
	delete clipmsg;
	//UndoSelection();
	Invalidate();
	mainapp->setReady();
}

void CanvasView::Copy ()
{
	extern long pasteX, pasteY, fPasteX, fPasteY;
	extern Becasso *mainapp;
	extern BBitmap *clip;
	mainapp->setBusy();
	//SetupUndo (M_DRAW);
	if (clip)
	{
		clip->Lock();
		delete clip;
	}

	uint32 buttons;
	BPoint point;
	GetMouse (&point, &buttons, true);
	BRect selectrect = GetSmallestRect (selection);
	fPasteX = long (selectrect.left - point.x);
	fPasteY = long (selectrect.top - point.y);
	pasteX = 0;
	pasteY = 0;
	BRect cutrect = selectrect;
	cutrect.OffsetTo (B_ORIGIN);
	clip = new BBitmap (cutrect, B_RGBA32);

	CutOrCopy (currentLayer(), clip, selection, int (selectrect.left), int (selectrect.top), false);

	myWindow->Lock();
	myWindow->setPaste (true);
	myWindow->Unlock();
	BMessage *clipmsg = new BMessage ('bits');
	clip->Archive (clipmsg, false);
	if (be_clipboard->Lock())
	{
		be_clipboard->Clear();
		BMessage *clipper = be_clipboard->Data();
		clipper->AddMessage ("image/x-be-bitmap", clipmsg);
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
	delete clipmsg;
	//UndoSelection();
	mainapp->setReady();
}

void CanvasView::CopyToNewLayer ()
{
//	Copy();
	char name[256];
	strcpy (name, currentLayer()->getName());
	strcat (name, lstring (405, " (detail)"));
	addLayer (name);
	extern long pasteX, pasteY, fPasteX, fPasteY;
	pasteX = fPasteX;
	pasteY = fPasteY;
	Paste();
}

void CanvasView::Paste (bool winAct)
{
	extern bool inPaste, inDrag;
	extern int DebugLevel;
	if (DebugLevel)
	{
		printf ("%s: Paste (%s); inPaste = %s, inDrag = %s\n",
				Window()->Title(),
				winAct ? "true" : "false",
				inPaste ? "true" : "false",
				inDrag ? "true" : "false");
	}
	BPoint point;
	ulong buttons;
	GetMouse (&point, &buttons, true);
	if (inPaste)
	{
		if (winAct)
		{
			verbose (1, "Window Activated while in Paste()\n");
		}
		else
		{
			ePaste (point, buttons);		// Note: These are dummies!
			return;
		}
	}
	extern BBitmap *clip;
	extern long pasteX, pasteY;
	if (!clip)
	{
		if (be_clipboard->Lock())
		{
			BMessage *clipper = be_clipboard->Data();
			BMessage clipmsg;
			if (clipper->FindMessage ("image/x-be-bitmap", &clipmsg) == B_OK)
			{
				clip = (BBitmap *) BBitmap::Instantiate (&clipmsg);
				if (!clip)
				{
					be_clipboard->Unlock();
					return;
				}
			}
			else
			{
				be_clipboard->Unlock();
				return;
			}
		}
	}

//	if (!winAct)		// dunno why I did this. Hmmm.  It breaks cross-window pastes
						// and drops from Tracker - 1dec2000
		SetupUndo (M_DRAW);

	currentLayer()->Lock();
	clip->Lock();
	prevPaste = clip->Bounds();
	delete cutbg;
	cutbg = new BBitmap (prevPaste, B_RGBA32, true);
	cutView = new SView (prevPaste, "CutView", uint32 (NULL), uint32 (NULL));
	cutbg->Lock();
	cutbg->AddChild (cutView);
	cutView->DrawBitmap (currentLayer(), BPoint (-point.x - pasteX, -point.y - pasteY));
	cutbg->Unlock();
	clip->Unlock();
	inPaste = true;
	prevPaste.OffsetTo (point.x + pasteX, point.y + pasteY);
	Invalidate();
	Window()->UpdateIfNeeded();
	if (inDrag && !winAct)
	{
		verbose (1, "Drag in window\n");
		currentLayer()->Lock();
		AddWithAlpha (clip, currentLayer(), int (point.x + pasteX), int (point.y + pasteY));
		inPaste = false;
		//ePaste (point, buttons);
	}
	else if (inDrag)
	{
		extern int DebugLevel;
		if (DebugLevel >= 1)
			printf ("%s... Drag to ext. window\n", Window()->Title());

		currentLayer()->Lock();
		AddWithAlpha (clip, currentLayer(), int (point.x + pasteX), int (point.y + pasteY));
		ePaste (point, buttons);
		inPaste = false;
	}
	else if (winAct)
	{
		verbose (2, "WinAct\n");
	}
	else
	{
		// printf ("Copy/Paste\n");
		ePasteM (point);
	}
	inDrag = false;
	currentLayer()->Unlock();
}

void CanvasView::ePaste (BPoint /* point */, uint32 buttons)
{
	verbose (1, "ePaste\n");
	extern bool inPaste;
	inPaste = false;
	fMouseDown = false;
//	SetupUndo (M_DRAW);
	if (buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)	// Bail out!
	{
		//FastCopy (cutbg, currentLayer());
		//memcpy (currentLayer()->Bits(), cutbg->Bits(), currentLayer()->BitsLength());
		currentLayer()->Lock();
		drawView->DrawBitmap (cutbg, prevPaste.LeftTop());
		currentLayer()->Unlock();
	}
	cutbg->RemoveChild (cutView);
	delete cutbg;
	cutbg = NULL;
	delete cutView;
	Invalidate();
}

void CanvasView::ePasteM (BPoint point)
{
	extern long pasteX, pasteY;
	extern BBitmap *clip;
	if (!cutbg)
		return;
	cutbg->Lock();
	clip->Lock();
	currentLayer()->Lock();
	// printf ("cutbg = %p, clip = %p\n", cutbg, clip);
	// printf ("point = (%.3f, %.3f), paste = (%li, %li)\n", point.x, point.y, pasteX, pasteY);
	//FastCopy (cutbg, currentLayer());
	//memcpy (currentLayer()->Bits(), cutbg->Bits(), currentLayer()->BitsLength());
	drawView->DrawBitmap (cutbg, prevPaste.LeftTop());
	cutView->DrawBitmap (currentLayer(), BPoint (-point.x - pasteX, -point.y - pasteY));
	AddWithAlpha (clip, currentLayer(), int (point.x + pasteX), int (point.y + pasteY));
	BRect pasteRect = BRect (int (point.x + pasteX), int (point.y + pasteY),
		 int (point.x + pasteX + clip->Bounds().right), int (point.y + pasteY + clip->Bounds().bottom));
	Invalidate (prevPaste | pasteRect);
	prevPaste = pasteRect;
	currentLayer()->Unlock();
	clip->Unlock();
	cutbg->Unlock();
}

status_t GetExportingTranslators (BMessage *inHere)
{
	BTranslatorRoster *r = BTranslatorRoster::Default();
	translator_id *trans;
	int32 count;
	if (r->GetAllTranslators (&trans, &count))
	{
		return B_ERROR;
	}

	/* find out which of them can export for us */
	int32 actuals = 0;
	for (int ix = 0; ix < count; ix++)
	{
		/* they have to have B_TRANSLATOR_BITMAP among inputs */
		const translation_format *formats;
		int32 nInput;
		if (r->GetInputFormats (trans[ix], &formats, &nInput))
		{
			continue;
		}
		for (int iy = 0; iy < nInput; iy++)
		{
			if (formats[iy].type == B_TRANSLATOR_BITMAP)
			{	/* using goto avoids pesky flag variables! */
				goto do_this_translator;
			}
		}
		continue; /* didn't have translator bitmap */

	do_this_translator:
		/* figure out what the Translator can write */
		int32 nOutput;
		if (r->GetOutputFormats (trans[ix], &formats, &nOutput))
		{
			continue;
		}
		/* add everything besides B_TRANSLATOR_BITMAP to outputs */

		inHere->AddInt32 ("be:_translator", trans[ix]);
		for (int iy = 0; iy < nOutput; iy++)
		{
			if (formats[iy].type != B_TRANSLATOR_BITMAP)
			{
				inHere->AddString ("be:types", formats[iy].MIME);
				inHere->AddString ("be:filetypes", formats[iy].MIME);
				inHere->AddString ("be:type_descriptions", formats[iy].name);
				inHere->AddInt32 ("be:_format", formats[iy].type);
				actuals++;
			}
		}
	}

	/* done with list of installed translators */
	delete[] trans;

	return B_OK;
}

uint32 TypeCodeForMIME (const char *MIME)
{
	BTranslatorRoster *r = BTranslatorRoster::Default();
	translator_id *trans;
	int32 count;
	if (r->GetAllTranslators (&trans, &count))
	{
		return B_ERROR;
	}

	for (int ix = 0; ix < count; ix++)
	{
		const translation_format *format_list = NULL;
		int32 fmt_count;
		if (B_OK <= BTranslatorRoster::Default()->GetInputFormats (trans[ix], &format_list, &fmt_count))
		{
			for (int iy = 0; iy < fmt_count; iy++)
			{
				if (!strcasecmp (format_list[iy].MIME, MIME))
				{
					uint32 type_code = format_list[iy].type;
					// delete [] format_list;
					delete [] trans;
					return type_code;
				}
			}
		}
		// delete [] format_list;
	}

	delete [] trans;
	return 0;
}

#if defined (OLD_DRAG)

void CanvasView::eDrag (BPoint point, uint32 buttons)
{
	extern BBitmap *clip;
	extern long pasteX, pasteY;
	extern bool inPaste, inDrag;
	if (!cutbg)
		cutbg = new BBitmap (currentLayer()->Bounds(), B_RGBA32);
	inPaste = true;
	cutbg->Lock();
	clip->Lock();
	BRect clipBounds = clip->Bounds();
	bbits = (uchar *) currentLayer()->Bits();
	bbpr = currentLayer()->BytesPerRow();
	tbits = (uchar *) clip->Bits();
	tbpr = clip->BytesPerRow();
	bwi = fCanvasFrame.Width();
	bhi = fCanvasFrame.Height();
	twi = clipBounds.Width();
	thi = clipBounds.Height();
	uchar *bgbits = (uchar *) cutbg->Bits();
	ulong len = currentLayer()->BitsLength();
	memcpy (bgbits, bbits, len);
	BPoint prevpoint = point;
	BRect frame = fCanvasFrame;
	BRect dragRect = clipBounds;
	dragRect.OffsetTo (point.x + pasteX + 8/fScale, point.y + pasteY + 8/fScale);	// + 8 ?!
	dragRect.left *= fScale;
	dragRect.top *= fScale;
	dragRect.right *= fScale;
	dragRect.bottom *= fScale;
	inDrag = true;
/*	BBitmap *dragmap = new BBitmap (clipBounds, B_RGBA32);
	uint32 *dbits = ((uint32 *) dragmap->Bits()) - 1;
	uint32 *cbits = ((uint32 *) clip->Bits()) - 1;
	uint32 transparent = (B_TRANSPARENT_32_BIT.red << 24) + (B_TRANSPARENT_32_BIT.green << 16)
		+ (B_TRANSPARENT_32_BIT.blue << 8) + (B_TRANSPARENT_32_BIT.alpha);
	for (uint32 i = 0; i < clip->BitsLength()/4; i++)
	{
		uint32 pixel = *(++cbits);
		if (pixel & 0xFF)
			*(++dbits) = pixel;
		else
			*(++dbits) = transparent;
	}
	SetDrawingMode (B_OP_OVER);
	DragMessage (new BMessage ('drag'), dragmap, BPoint (pasteX, pasteY));
*/	DragMessage (new BMessage ('drag'), dragRect);
	while (buttons)
	{
		if (point != prevpoint)
		{
			memcpy (bbits, bgbits, len);
			Invalidate (prevPaste);
			FastAddWithAlpha (point.x + pasteX, point.y + pasteY);
			prevPaste = BRect (point.x + pasteX, point.y + pasteY,
				 point.x + pasteX + clipBounds.right, point.y + pasteY + clipBounds.bottom);
			drawView->Sync();
			Invalidate (prevPaste);
			myWindow->Lock();
			myWindow->posview->Pulse();
			myWindow->Unlock();
		}
		BRect bounds = Bounds();
		if (point.x > bounds.right && bounds.right < frame.right)
			ScrollBy (point.x - bounds.right, 0);
		if (point.x < bounds.left && bounds.left > 0)
			ScrollBy (point.x - bounds.left, 0);
		if (point.y > bounds.bottom && bounds.bottom < frame.bottom)
			ScrollBy (0, point.y - bounds.bottom);
		if (point.y < bounds.top && bounds.top > 0)
			ScrollBy (0, point.y - bounds.top);
		snooze (20000);
		prevpoint = point;
		GetMouse (&point, &buttons, true);
	}
	clip->Unlock();
	cutbg->Unlock();
	inPaste = false;
	Invalidate();
}

#else

void CanvasView::eDrag (BPoint point, uint32 buttons)
{
	extern BBitmap *clip;
	clip->Lock();
	extern long pasteX, pasteY;
	BRect dragRect = clip->Bounds();
	dragRect.OffsetTo (point.x + pasteX + 8/fScale, point.y + pasteY + 8/fScale);	// + 8 ?!
	dragRect.left *= fScale;
	dragRect.top *= fScale;
	dragRect.right *= fScale;
	dragRect.bottom *= fScale;
	BRect clipBounds = clip->Bounds();

	{
		// Otherwise: Few pixels off!!  (BeOS bug...)
		extern Becasso *mainapp;
		mainapp->setHand();
		BPoint sp = ConvertToScreen (BPoint (point.x*fScale, point.y*fScale));
		set_mouse_position (int (sp.x + 1), int (sp.y));
		set_mouse_position (int (sp.x), int (sp.y));
	}

	BMessage *dragMessage = new BMessage (B_SIMPLE_DATA);
	bool tmp = true;
	dragMessage->AddBool ("becasso", tmp);
	BBitmap *dragmap = new BBitmap (clipBounds, B_RGBA32, false);
	memcpy (dragmap->Bits(), clip->Bits(), clip->BitsLength());
	//dragMessage->AddRect ("be:_frame", clip->Bounds());
	dragMessage->AddPointer ("be:_bitmap_ptr", clip);
	dragMessage->AddInt32 ("be:actions", B_COPY_TARGET);
	dragMessage->AddInt32 ("be:actions", B_TRASH_TARGET);
	char clipname[B_FILE_NAME_LENGTH];
	CanvasWindow *myWindow = dynamic_cast<CanvasWindow *> (Window());
	strcpy (clipname, myWindow->FileName());
	strcat (clipname, " Clip");
	dragMessage->AddString ("be:clip_name", clipname);
	dragMessage->AddString ("be:types", B_FILE_MIME_TYPE);
	GetExportingTranslators (dragMessage);

	extern long fPasteX, fPasteY;		// These names suck, I know.
//	printf ("(%ld, %ld), (%ld, %ld)\n", pasteX, pasteY, fPasteX, fPasteY);

	extern int DebugLevel;
	if (DebugLevel > 2)
	{
		printf ("Dragging Message:\n");
		dragMessage->PrintToStream();
	}

	if ((fCurrentLayer == fNumLayers - 1)	// i.e. Top layer is active
	 && (fScale == 1))
	{
		// printf ("Top layer active...\n");
		DragMessage (dragMessage, dragmap, B_OP_ALPHA, BPoint (-fPasteX, -fPasteY));
	}
	else
	{
		SetupUndo (M_DRAW);
		extern bool inPaste, inDrag;
		delete cutbg;
		cutbg = new BBitmap (clipBounds, B_RGBA32, true);
		BView *cutView = new BView (clipBounds, "CutView", uint32 (NULL), uint32 (NULL));
		cutbg->AddChild (cutView);
		cutbg->Lock();
		// Setup stuff for FastAddWithAlpha
		bbits = (uchar *) currentLayer()->Bits();
		bbpr = currentLayer()->BytesPerRow();
		tbits = (uchar *) clip->Bits();
		tbpr = clip->BytesPerRow();
		bwi = fCanvasFrame.IntegerWidth();
		bhi = fCanvasFrame.IntegerHeight();
		twi = clipBounds.IntegerWidth();
		thi = clipBounds.IntegerHeight();

		dragMessage->AddBool ("not_top", tmp);
		DragMessage (dragMessage, dragRect);

		inPaste = true;
		inDrag = true;

		BBitmap *cl = currentLayer();
		cutView->DrawBitmap (cl, BPoint (-point.x - pasteX, -point.y - pasteY));
		drawView->SetDrawingMode (B_OP_COPY);
	//	drawView->DrawBitmap (cutbg, BPoint (point.x + pasteX, point.y + pasteY));
	//	Invalidate();
		BRect pasteRect;
		prevPaste = fCanvasFrame;
		BPoint prevpoint = point;
		bool invalidate = false;
		while (buttons)
		{
			if (point != prevpoint)
			{
				drawView->DrawBitmap (cutbg, BPoint (prevpoint.x + pasteX, prevpoint.y + pasteY));
				drawView->Sync();
				cutView->DrawBitmap (cl, BPoint (-point.x - pasteX, -point.y - pasteY));
				cutView->Sync();
				FastAddWithAlpha (int (point.x + pasteX), int (point.y + pasteY));
				pasteRect = BRect (int (point.x + pasteX), int (point.y + pasteY),
					 int (point.x + pasteX + clipBounds.right), int (point.y + pasteY + clipBounds.bottom));
				Invalidate (prevPaste | prevPaste);
				prevPaste = pasteRect;
				myWindow->Lock();
				myWindow->posview->Pulse();
				myWindow->Unlock();
				invalidate = true;
			}
			else if (invalidate)
			{
				Invalidate();
				invalidate = false;
			}
			ScrollIfNeeded (point);
			snooze (20000);
			prevpoint = point;
			GetMouse (&point, &buttons, true);
		}
		cutbg->RemoveChild (cutView);
		delete cutView;
		delete cutbg;
		cutbg = NULL;
		clip->Unlock();
		inPaste = false;
	//	inDrag = false;
		Invalidate();
	}
}

#endif

void CanvasView::SelectAll (bool menu)
{
	if (menu)
	{
		if (filterOpen || transformerOpen || generatorOpen)
		{
			beep();
			return;
		}
	}
	SetupUndo (M_SELECT);
	selection->Lock();
	selectionView->SetDrawingMode (B_OP_COPY);
	selectionView->SetHighColor (SELECT_FULL);
	selectionView->FillRect (selection->Bounds());
	selectionView->Sync();
	selection->Unlock();
	Invalidate();
	Window()->UpdateIfNeeded();
}

void CanvasView::UndoSelection (bool menu)
// Note: This is `Deselect All'.
{
	if (menu)
	{
		if (filterOpen || transformerOpen || generatorOpen)
		{
			beep();
			return;
		}
	}
	SetupUndo (M_SELECT);
	selection->Lock();
	selectionView->SetDrawingMode (B_OP_COPY);
	selectionView->SetLowColor (SELECT_NONE);
	selectionView->FillRect (selection->Bounds(), B_SOLID_LOW);
	selectionView->Sync();
	selection->Unlock();
	selchanged = false;
	sel = false;
	myWindow->Lock();
	myWindow->setMenuItem (B_CUT, false);
	myWindow->setMenuItem (B_COPY, false);
	myWindow->setMenuItem ('csel', false);
	myWindow->setMenuItem ('sund', false);
	myWindow->setMenuItem ('sinv', false);
	myWindow->Unlock();
	Invalidate();
	Window()->UpdateIfNeeded();
}

//////////////////////////////
#if defined (__POWERPC__)
#	pragma optimization_level 4
#endif

void CanvasView::ChannelToSelection (uint32 what, bool menu)
{
	if (menu)
	{
		if (filterOpen || transformerOpen || generatorOpen)
		{
			beep();
			return;
		}
	}
	SetupUndo (M_SELECT);
	selection->Lock();
	currentLayer()->Lock();
	grey_pixel *s_data = (grey_pixel *) selection->Bits() - 1;
	bgra_pixel *c_data = (bgra_pixel *) currentLayer()->Bits() - 1;
	uint32 h = fCanvasFrame.IntegerHeight() + 1;
	uint32 w = fCanvasFrame.IntegerWidth() + 1;
	uint32 sbpr = selection->BytesPerRow();
	uint32 sdif = sbpr - w;
	switch (what)
	{
	case 'ctsA':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(++s_data) = ALPHA(*(++c_data));
			}
			s_data += sdif;
		}
		break;
	case 'ctsR':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(++s_data) = RED(*(++c_data));
			}
			s_data += sdif;
		}
		break;
	case 'ctsG':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(++s_data) = GREEN(*(++c_data));
			}
			s_data += sdif;
		}
		break;
	case 'ctsB':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(++s_data) = BLUE(*(++c_data));
			}
			s_data += sdif;
		}
		break;
	case 'ctsC':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(++s_data) = CYAN(bgra2cmyk (*(++c_data)));
			}
			s_data += sdif;
		}
		break;
	case 'ctsM':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(++s_data) = MAGENTA(bgra2cmyk (*(++c_data)));
			}
			s_data += sdif;
		}
		break;
	case 'ctsY':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(++s_data) = YELLOW(bgra2cmyk (*(++c_data)));
			}
			s_data += sdif;
		}
		break;
	case 'ctsK':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(++s_data) = BLACK(bgra2cmyk (*(++c_data)));
			}
			s_data += sdif;
		}
		break;
	case 'ctsH':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				hsv_color h = bgra2hsv (*(++c_data));
				*(++s_data) = uchar (h.hue*255/360);
			}
			s_data += sdif;
		}
		break;
	case 'ctsS':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				hsv_color h = bgra2hsv (*(++c_data));
				if (h.hue != HUE_UNDEF)
					*(++s_data) = uchar (h.saturation*255);
				else
					*(++s_data) = uchar (h.value);
			}
			s_data += sdif;
		}
		break;
	case 'ctsV':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				hsv_color h = bgra2hsv (*(++c_data));
				*(++s_data) = uchar (h.value*255);
			}
			s_data += sdif;
		}
		break;
	default:
		fprintf (stderr, "ChannelToSelection: Invalid channel\n");
	}
	currentLayer()->Unlock();
	selection->Unlock();
	Invalidate();
	Window()->UpdateIfNeeded();
}

void CanvasView::SelectionToChannel (uint32 what)
{
	SetupUndo (M_DRAW);
	selection->Lock();
	currentLayer()->Lock();
	grey_pixel *s_data = (grey_pixel *) selection->Bits() - 1;
	bgra_pixel *c_data = (bgra_pixel *) currentLayer()->Bits() - 1;
	uint32 h = fCanvasFrame.IntegerHeight() + 1;
	uint32 w = fCanvasFrame.IntegerWidth() + 1;
	uint32 sbpr = selection->BytesPerRow();
	uint32 sdif = sbpr - w;
	switch (what)
	{
	case 'stcA':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(c_data) = (*(++c_data) & COLOR_MASK) | (*(++s_data) << ALPHA_BPOS);
			}
			s_data += sdif;
		}
		break;
	case 'stcR':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(c_data) = (*(++c_data) & IRED_MASK) | (*(++s_data) << RED_BPOS);
			}
			s_data += sdif;
		}
		break;
	case 'stcG':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(c_data) = (*(++c_data) & IGREEN_MASK) | (*(++s_data) << GREEN_BPOS);
			}
			s_data += sdif;
		}
		break;
	case 'stcB':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				*(c_data) = (*(++c_data) & IBLUE_MASK) | (*(++s_data) << BLUE_BPOS);
			}
			s_data += sdif;
		}
		break;
	case 'stcC':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				bgra_pixel p = *(++c_data);
				*(c_data) = ((cmyk2bgra ((bgra2cmyk (p) & ICYAN_MASK) | (*(++s_data) << CYAN_BPOS))) & COLOR_MASK) | (p & ALPHA_MASK);
			}
			s_data += sdif;
		}
		break;
	case 'stcM':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				bgra_pixel p = *(++c_data);
				*(c_data) = ((cmyk2bgra ((bgra2cmyk (p) & IMAGENTA_MASK) | (*(++s_data) << MAGENTA_BPOS))) & COLOR_MASK) | (p & ALPHA_MASK);
			}
			s_data += sdif;
		}
		break;
	case 'stcY':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				bgra_pixel p = *(++c_data);
				*(c_data) = ((cmyk2bgra ((bgra2cmyk (p) & IYELLOW_MASK) | (*(++s_data) << YELLOW_BPOS))) & COLOR_MASK) | (p & ALPHA_MASK);
			}
			s_data += sdif;
		}
		break;
	case 'stcK':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				bgra_pixel p = *(++c_data);
				*(c_data) = ((cmyk2bgra ((bgra2cmyk (p) & IBLACK_MASK) | (*(++s_data) << BLACK_BPOS))) & COLOR_MASK) | (p & ALPHA_MASK);
		}
			s_data += sdif;
		}
		break;
	case 'stcH':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				hsv_color h = bgra2hsv (*(++c_data));
				h.hue = 360.0*(*(++s_data))/255;
				*(c_data) = hsv2bgra (h);
			}
			s_data += sdif;
		}
		break;
	case 'stcS':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				hsv_color h = bgra2hsv (*(++c_data));
				if (h.hue != HUE_UNDEF)
					h.saturation = *(++s_data)/255.0;
				else
					++s_data;
				*(c_data) = hsv2bgra (h);
			}
			s_data += sdif;
		}
		break;
	case 'stcV':
		for (uint32 i = 0; i < h; i++)
		{
			for (uint32 j = 0; j < w; j++)
			{
				hsv_color h = bgra2hsv (*(++c_data));
				h.value = *(++s_data)/255.0;
				*(c_data) = hsv2bgra (h);
			}
			s_data += sdif;
		}
		break;
	default:
		fprintf (stderr, "SelectionToChannel: Invalid channel\n");
	}
	currentLayer()->Unlock();
	selection->Unlock();
	Invalidate();
	Window()->UpdateIfNeeded();
}

rgb_color CanvasView::GuessBackgroundColor ()
{
	uint32 r, g, b, a, n;
	r = g = b = a = 0;
	currentLayer()->Lock();
	bgra_pixel *c_data = (bgra_pixel *) currentLayer()->Bits() - 1;
	uint32 h = fCanvasFrame.IntegerHeight() + 1;
	uint32 w = fCanvasFrame.IntegerWidth() + 1;
	bgra_pixel p;
	for (uint32 j = 0; j < w; j++)
	{
		p = *(++c_data);
		b += BLUE(p);
		g += GREEN(p);
		r += RED(p);
		a += ALPHA(p);
	}
	for (uint32 i = 1; i < h - 1; i++)
	{
		p = *(++c_data);
		b += BLUE(p);
		g += GREEN(p);
		r += RED(p);
		a += ALPHA(p);
		c_data += w;
		p = *(--c_data);
		b += BLUE(p);
		g += GREEN(p);
		r += RED(p);
		a += ALPHA(p);
	}
	for (uint32 j = 0; j < w; j++)
	{
		p = *(++c_data);
		b += BLUE(p);
		g += GREEN(p);
		r += RED(p);
		a += ALPHA(p);
	}
	currentLayer()->Unlock();
	rgb_color c;
	n = 2*w + 2*h - 4;
	c.red = r/n;
	c.green = g/n;
	c.blue = b/n;
	c.alpha = a/n;
	return (c);
}

void CanvasView::SelectByColor (rgb_color c, bool menu)
{
	if (menu)
	{
		if (filterOpen || transformerOpen || generatorOpen)
		{
			beep();
			return;
		}
	}
	SetupUndo (M_SELECT);
	selection->Lock();
	currentLayer()->Lock();
	grey_pixel *s_data = (grey_pixel *) selection->Bits() - 1;
	bgra_pixel *c_data = (bgra_pixel *) currentLayer()->Bits() - 1;
	uint32 h = fCanvasFrame.IntegerHeight() + 1;
	uint32 w = fCanvasFrame.IntegerWidth() + 1;
	uint32 sbpr = selection->BytesPerRow();
	uint32 sdif = sbpr - w;
	for (uint32 i = 0; i < h; i++)
	{
		for (uint32 j = 0; j < w; j++)
		{
			bgra_pixel b = *(++c_data);
			*(++s_data) = uchar ((255 - diff (c, bgra2rgb (b)))*(ALPHA(b))/255);
		}
		s_data += sdif;
	}
	currentLayer()->Unlock();
	selection->Unlock();
	Invalidate();
	Window()->UpdateIfNeeded();
}

void CanvasView::ColorizeSelection (rgb_color c)
{
	SetupUndo (M_DRAW);
	selection->Lock();
	currentLayer()->Lock();
	grey_pixel *s_data = (grey_pixel *) selection->Bits() - 1;
	bgra_pixel *c_data = (bgra_pixel *) currentLayer()->Bits() - 1;
	uint32 h = fCanvasFrame.IntegerHeight() + 1;
	uint32 w = fCanvasFrame.IntegerWidth() + 1;
	uint32 sbpr = selection->BytesPerRow();
	uint32 sdif = sbpr - w;
	bgra_pixel b = rgb2bgra (c);
	for (uint32 i = 0; i < h; i++)
	{
		for (uint32 j = 0; j < w; j++)
		{
			register bgra_pixel d = *(++c_data);
			register int sa = *(++s_data);
//			register int da = 255 - sa;
			b &= COLOR_MASK;
			b |= (sa << ALPHA_BPOS);
//			if (da == 0)		// Fully opaque pixel
//			{
//				*c_data = b;
//			}
//			else if (da == 255)	// Fully transparent pixel
//			{
//			}
//			else
//			{
//				#if defined (__POWERPC__)
//					*c_data = (((((d>>24)        *da +  (b>>24)        *sa)<<16)) & 0xFF000000) |
//							  (((((d>>16) & 0xFF)*da + ((b>>16) & 0xFF)*sa)<< 8)  & 0x00FF0000) |
//							   ((((d>> 8) & 0xFF)*da + ((b>> 8) & 0xFF)*sa)       & 0x0000FF00) |
//								//(sa & 0xFF);//(max_c (sa, da) & 0xFF);
//								clipchar (sa + int (d & 0xFF));
//				#else
//					*c_data = (((((d>>16) & 0xFF)*da + ((b>>16) & 0xFF)*sa)<<16) & 0x00FF0000) |
//							  (((((d>> 8) & 0xFF)*da + ((b>> 8) & 0xFF)*sa)<< 8) & 0x0000FF00) |
//							  (((((d    ) & 0xFF)*da + ((b    ) & 0xFF)*sa)    ) & 0x000000FF) |
//							      (clipchar (sa + int (d >> 24)) << 24);
//				#endif
//			}
			*c_data = pixelblend (d, b);
		}
		s_data += sdif;
	}
	currentLayer()->Unlock();
	selection->Unlock();
	Invalidate();
	Window()->UpdateIfNeeded();
}

void CanvasView::InvertSelection (bool menu)
{
	if (menu)
	{
		if (filterOpen || transformerOpen || generatorOpen)
		{
			beep();
			return;
		}
	}
	// SetupUndo (M_SELECT);
	// Note: This is _so_ easily undone, it's a waste of a valuable undo buffer...
	selection->Lock();
	uchar *sbits = (uchar *) selection->Bits();
	for (int i = 0; i < selection->BitsLength(); i++)
		*sbits++ = 255 - *sbits;
	sel = true;
	selection->Unlock();
	Invalidate();
	Window()->UpdateIfNeeded();
}

void CanvasView::FastAddWithAlpha (long x, long y, int strength)
// Note: Setup tbits, tbpr, bbits, bbpr, twi, thi, bwi and bhi before you enter here!
// tbits = source->Bits(); // (32bit brush/clip bitmap)
// tbpr  = source->BytesPerRow();
// twi   = source->Bounds().Width();
// thi   = source->Bounds().Height();
// bbits = (uint32 *) destination->Bits(); // (32 bit canvas bitmap)
// bbpr  = destination->BytesPerRow();
// bwi   = destination->Bounds().Width();
// bhi   = destination->Bounds().Height();
//
// Alpha = opacity!
{
	// fprintf (stderr, "CanvasView::FastAddWithAlpha... "); fflush (stderr);
	clock_t	start, end;
	start = clock();
	uint32 *src_data = (uint32 *) tbits;
	uint32 *dest_data = (uint32 *) bbits;
	uint32 tlpr = tbpr/4;
	uint32 blpr = bbpr/4;
	if ((x + twi <= 0) || (x > bwi) || (y + thi <= 0) || (y > bhi) || !strength)
	{
		// printf ("Totally out of bounds - returning\n");
		return;
	}

	// Clipping
	int minx = max_c (x, 0);
	int maxx = min_c (x + twi + 1, bwi + 1);
	int miny = max_c (y, 0);
	int maxy = min_c (y + thi + 1, bhi + 1);

	ulong src_bprdiff = tlpr - (maxx - minx);
	ulong dest_bprdiff = blpr - (maxx - minx);

	if (y < 0)
		src_data -= y*tlpr;
	if (x < 0)
		src_data -= x;

	dest_data += miny*blpr + minx;

//	printf ("Alpha_src = %i, Alpha_dest = %i\n", src_data[3], dest_data[3]); // ON PPC!
	dest_data--;	// Note: On PPC, pre-increment is faster.
	src_data--;
	if (strength == 255)
	{
		for (int j = miny; j < maxy; j++)
		{
			for (int i = minx; i < maxx; i++)
			{
#if defined (__POWERPC__)
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = srcpixel & 0xFF;
				register int da = /*destpixel & 0xFF;*/ 255 - sa;
				register int ta = /*sa + da;*/ 255;
				if (sa == 255 || /*da == 0*/ !(destpixel & 0xFF))	// Fully opaque
				{
					*dest_data = srcpixel;
				}
				else if (sa == 0)	// Fully transparent
				{
				}
				else
				{
//					*dest_data	= ((((((destpixel>>24)       )*da + ((srcpixel>>24)       )*sa)/ta)<<24) & 0xFF000000) |
//								  ((((((destpixel>>16) & 0xFF)*da + ((srcpixel>>16) & 0xFF)*sa)/ta)<<16) & 0x00FF0000) |
//								  ((((((destpixel>> 8) & 0xFF)*da + ((srcpixel>> 8) & 0xFF)*sa)/ta)<< 8) & 0x0000FF00) |
//										clipchar (sa + int (destpixel & 0xFF));
					*dest_data =  ((((destpixel & 0xFF000000)/ta)*da + ((srcpixel & 0xFF000000)/ta)*sa) & 0xFF000000) |
								  ((((destpixel & 0x00FF0000)/ta)*da + ((srcpixel & 0x00FF0000)/ta)*sa) & 0x00FF0000) |
								 (((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa))/ta) & 0x0000FF00) |
									  clipchar (sa + int (destpixel & 0xFF));
									  //clipchar (ta);
				}
#else
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = srcpixel >> 24;
				register int da = destpixel >> 24; // 255 - sa;
				register int ta = sa + da; // ta = 255;
				if (sa == 255 || da == 0  /* !(destpixel & 0xFF000000) */ )	// Fully opaque
				{
					*dest_data = srcpixel;
				}
				else if (sa == 0)	// Fully transparent
				{
				}
				else
				{

//					*dest_data = ((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)/255) & 0x0000FF00) |
//								 ((((destpixel & 0x00FF00FF)*da + (srcpixel & 0x00FF00FF)*sa)/255) & 0x00FF00FF) |
//								    (clipchar (sa + int (destpixel >> 24)) << 24);

					*dest_data = ((((((destpixel & 0x00FF0000) >> 1)*da + ((srcpixel & 0x00FF0000) >> 1)*sa)/ta) << 1) & 0x00FF0000) |
								   ((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)/ta) & 0x0000FF00) |
								   ((((destpixel & 0x000000FF)*da + (srcpixel & 0x000000FF)*sa)/ta) & 0x000000FF) |
									 // (clipchar (ta) << 24);
									  (clipchar (sa + int (destpixel >> 24)) << 24);
//								    ((max_c (sa, destpixel >> 24)) << 24);
				}
#endif
			}
			src_data += src_bprdiff;
			dest_data += dest_bprdiff;
		}
	}
	else if (strength > 0)
	{
		for (int j = miny; j < maxy; j++)
		{
			for (int i = minx; i < maxx; i++)
			{
#if defined (__POWERPC__)
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = (srcpixel & 0xFF)*strength/255;
				register int da = /*destpixel & 0xFF;*/ 255 - sa;
				register int ta = /*sa + da;*/ 255;
				if (sa == 255 || /*da == 0*/ !(destpixel & 0xFF) )	// Fully opaque
				{
					*dest_data = srcpixel;
				}
				else if (sa == 0)	// Fully transparent
				{
				}
				else
				{
//					*dest_data	= ((((((destpixel>>24)       )*da + ((srcpixel>>24)       )*sa)/ta)<<24) & 0xFF000000) |
//								  ((((((destpixel>>16) & 0xFF)*da + ((srcpixel>>16) & 0xFF)*sa)/ta)<<16) & 0x00FF0000) |
//								  ((((((destpixel>> 8) & 0xFF)*da + ((srcpixel>> 8) & 0xFF)*sa)/ta)<< 8) & 0x0000FF00) |
//										clipchar (sa + int (destpixel & 0xFF));
					*dest_data =  ((((destpixel & 0xFF000000)/ta)*da + ((srcpixel & 0xFF000000)/ta)*sa) & 0xFF000000) |
								  ((((destpixel & 0x00FF0000)/ta)*da + ((srcpixel & 0x00FF0000)/ta)*sa) & 0x00FF0000) |
								  ((((destpixel & 0x0000FF00)    *da +  (srcpixel & 0x0000FF00)*sa)/ta) & 0x0000FF00) |
									  clipchar (sa + int (destpixel & 0xFF));
									//clipchar (ta);
				}
#else
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = (srcpixel >> 24)*strength/255;
				register int da = /*destpixel >> 24; */ 255 - sa;
				register int ta = /*sa + da;*/ 255;
				if (sa == 255 || /*da == 0*/ !(destpixel & 0xFF000000))	// Fully opaque
				{
					*dest_data = srcpixel;
				}
				else if (sa == 0)	// Fully transparent
				{
				}
				else
				{
					*dest_data = ((((((destpixel & 0x00FF0000) >> 1)*da + ((srcpixel & 0x00FF0000) >> 1)*sa)/ta) << 1) & 0x00FF0000) |
								   ((((destpixel & 0x0000FF00)*da + (srcpixel & 0x0000FF00)*sa)/ta) & 0x0000FF00) |
								   ((((destpixel & 0x000000FF)*da + (srcpixel & 0x000000FF)*sa)/ta) & 0x000000FF) |
//									  (clipchar (ta) << 24);
//								    ((max_c (sa, destpixel >> 24)) << 24);
									  (clipchar (sa + int (destpixel >> 24)) << 24);
				}
#endif
			}
			src_data += src_bprdiff;
			dest_data += dest_bprdiff;
		}
	}
	else	// Very special case:  Only "erase" transparency.
	{
		for (int j = miny; j < maxy; j++)
		{
			for (int i = minx; i < maxx; i++)
			{
#if defined (__POWERPC__)
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = (srcpixel & 0xFF)*(-strength)/255;
				register int da = (destpixel & 0xFF);
				register int ta = 255;
				if (sa == 255 || !(destpixel & 0xFF))	// Fully opaque
				{
					*dest_data = destpixel & 0xFFFFFF00;
				}
				else if (sa == 0)	// Fully transparent
				{
				}
				else
				{
					*dest_data	= (destpixel & 0xFFFFFF00) | clipchar (da - sa);
				}
#else
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = (srcpixel >> 24)*(-strength)/255;
				register int da = (destpixel >> 24);
//				register int ta = 255;
				if (sa == 255 || !(destpixel & 0xFF000000))	// Fully opaque
				{
					*dest_data = destpixel & 0x00FFFFFF;
				}
				else if (sa == 0)	// Fully transparent
				{
				}
				else
				{
					*dest_data	= (destpixel & 0x00FFFFFF) | ((clipchar (da - sa)) << 24);
				}
#endif
			}
			src_data += src_bprdiff;
			dest_data += dest_bprdiff;
		}
	}
	// printf ("Strength: %i   \r", strength);
	// fprintf (stderr, "Done.\n");
	end = clock();
	extern int DebugLevel;
	if (DebugLevel > 7)
		printf ("FastAddWithAlpha took %ld ms\n", end - start);
}


// The following function isn't used anymore, and hence isn't made IA ready!
//
void CanvasView::FastBlendWithAlpha (long x, long y, int /* strength */)
// Note: Setup tbits, tbpr, bbits, bbpr, twi, thi, bwi and bhi before you enter here!
// tbits = (uchar *) source->Bits(); // (32bit brush/clip bitmap)
// tbpr  = source->BytesPerRow();
// twi   = source->Bounds().Width();
// thi   = source->Bounds().Hright();
// bbits = (uchar *) destination->Bits(); // (32 bit canvas bitmap)
// bbpr  = destination->BytesPerRow();
// bwi   = destination->Bounds().Width();
// bhi   = destination->Bounds().Height();
//
// Alpha = opacity!
{
	uchar *src_data = tbits;
	uchar *dest_data = bbits;
	if ((x + twi <= 0) || (x > bwi) || (y + thi <= 0) || (y > bhi))
	{
		return;
	}

	// Clipping
	int minx = max_c (x, 0);
	int maxx = min_c (x + twi + 1, bwi + 1);
	int miny = max_c (y, 0);
	int maxy = min_c (y + thi + 1, bhi + 1);

	ulong src_bprdiff = tbpr - 4*(maxx - minx);
	ulong dest_bprdiff = bbpr - 4*(maxx - minx);

	if (y < 0)
		src_data -= y*tbpr;
	if (x < 0)
		src_data -= x*4;

	dest_data += miny*bbpr + 4*minx;

//	printf ("Alpha_src = %i, Alpha_dest = %i\n", src_data[3], dest_data[3]);
	dest_data--;	// Note: On PPC, pre-increment is faster.
	src_data--;
	for (int j = miny; j < maxy; j++)
	{
		for (int i = minx; i < maxx; i++)
		{
			register int sa = src_data[4];	// Yes, 4.  *_data are pre-decremented.
			register int da = dest_data[4];
			register int ta = sa + da;
			if (da == 0)
			{
				*(++dest_data) = *(++src_data);
				*(++dest_data) = *(++src_data);
				*(++dest_data) = *(++src_data);
				*(++dest_data) = *(++src_data);	// new alpha channel
			}
			else if (sa == 0)
			{
				dest_data += 4;
				src_data += 4;
			}
			else
			{
				*dest_data = (*(++dest_data)*da + *(++src_data)*sa)/ta;
				*dest_data = (*(++dest_data)*da + *(++src_data)*sa)/ta;
				*dest_data = (*(++dest_data)*da + *(++src_data)*sa)/ta;
				*dest_data = clipchar (int (*(++dest_data) + *(++src_data)));	// new alpha channel
			}
		}
		src_data += src_bprdiff;
		dest_data += dest_bprdiff;
	}
}

void CanvasView::WindowActivated (bool active)
{
	// Apparently, views get multiple activate- and deactivate-calls in a row
	// This sounds like a bug to me, but this will work around it by treating it
	// as "switch ringing".

	extern bool inPaste;
	extern BLocker *clipLock;

	if (clipLock->LockWithTimeout (500000) != B_OK)
	{
		printf ("Couldn't get clipLock to %sactivate %s\n", active ? "" : "in", myWindow->Name());
		return;
	}

//	printf ("WindowActivated - %s ", myWindow->Name());
	inherited::WindowActivated (active);
	if (active && !windowLock)
	{
//		printf ("Activating.");
		windowLock = true;
		BPoint point;
		uint32 buttons;
		GetMouse (&point, &buttons, true);
		if (inPaste && !newWin)
		{
//			extern long pasteX, pasteY;
//			extern BBitmap *clip;
//			BRect clipRect = clip->Bounds();
//			printf ("Deleting old cutbg\n");
//			delete cutbg;
//			printf ("Creating new one\n");
//			cutbg = new BBitmap (clipRect, B_RGBA32, true);
//			cutbg->Lock();
//			cutView->DrawBitmap (currentLayer(), BPoint (-point.x - pasteX, -point.y - pasteY));
//			cutbg->Unlock();
//			prevPaste = BRect (point.x + pasteX, point.y + pasteY,
//				 point.x + pasteX + clip->Bounds().right, point.y + pasteY + clip->Bounds().bottom);
//			printf ("prevPaste set\n");
//			ePasteM (point);
			//printf ("Calling Paste from WindowActivated()\n");
			Paste (true);
		}
		if (Bounds().Contains (point))
		{
			extern Becasso *mainapp;
			if ((transformerOpen && !(transformerOpen->DoesPreview() & PREVIEW_MOUSE))
			 || (generatorOpen && !(generatorOpen->DoesPreview() & PREVIEW_MOUSE)))
				mainapp->setCCross();
			else
				mainapp->setCross();
			mouse_on_canvas = true;
			Invalidate();
		}
		newWin = false;
	}
	else if (!active && windowLock)
	{
//		printf ("Inactivating.");
		if (inPaste)
		{
//			printf ("(inPaste)");
//			extern long pasteX, pasteY;
			currentLayer()->Lock();
			cutbg->Lock();
			drawView->DrawBitmap (cutbg, prevPaste.LeftTop());
			delete cutbg;
			cutbg = NULL;
			currentLayer()->Unlock();
			Invalidate();
		}
		Window()->UpdateIfNeeded();
		mouse_on_canvas = false;
		windowLock = false;
	}
//	printf ("\n");
//	else
//		printf ("spurious %s call.\n", active ? "activate" : "inactivate");
//	printf ("End of WindowActivated - %s\n", myWindow->Name());
	clipLock->Unlock();
}

void CanvasView::ScrollIfNeeded (BPoint point)
{
	// fprintf (stderr, "CanvasView::ScrollIfNeeded... "); fflush (stderr);
	BRect bounds = fBGView->Bounds();
	bounds.left /= fScale;
	bounds.top /= fScale;
	bounds.right /= fScale;
	bounds.bottom /= fScale;
//	printf ("%f %f %f\n", point.x, bounds.right, fCanvasFrame.right);
	if (point.x > bounds.right && bounds.right < fCanvasFrame.right)
		fBGView->ScrollBy ((point.x - bounds.right)*fScale, 0);
	if (point.x < bounds.left && bounds.left > 0)
		fBGView->ScrollBy ((point.x - bounds.left)*fScale, 0);
	if (point.y > bounds.bottom && bounds.bottom < fCanvasFrame.bottom)
		fBGView->ScrollBy (0, (point.y - bounds.bottom)*fScale);
	if (point.y < bounds.top && bounds.top > 0)
		fBGView->ScrollBy (0, (point.y - bounds.top)*fScale);
	// fprintf (stderr, "Done.\n");
}

BRect CanvasView::makePositive (BRect r)
{
	BRect res;
	res.left = min_c (r.left, r.right);
	res.right = max_c (r.left, r.right);
	res.top = min_c (r.top, r.bottom);
	res.bottom = max_c (r.top, r.bottom);
	return res;
}

BRect CanvasView::PRect (float l, float t, float r, float b)
{
	BRect res;
	res.left = min_c (l, r);
	res.right = max_c (l, r);
	res.top = min_c (t, b);
	res.bottom = max_c (t, b);
	return res;
}

void CanvasView::WriteAsHex (const char *fname)
{
	BScreen screen;
	FILE *f = fopen (fname, "wb");
	for (int i = 0; i <= fCanvasFrame.Height(); i++)
	{
		int j;
		for (j = 0; j <= fCanvasFrame.Width(); j++)
		{
			rgb_color c = getColor (BPoint (j, i));
			fprintf (f, "%c%c%c", c.red, c.green, c.blue);
		}
	}
	fclose (f);
}

void CanvasView::ExportCstruct (const char *fname)
{
	BScreen screen;
	bool last = false;
	FILE *f = fopen (fname, "wb");
	printf ("Dumping to %s\n", fname);
	fprintf (f, "/* C array for %s */\n", fname);
	fprintf (f, "uchar data[%ld*%ld] = {\n	", fCanvasFrame.IntegerHeight() + 1, fCanvasFrame.IntegerWidth() + 1);
	for (int i = 0; i <= fCanvasFrame.IntegerHeight(); i++)
	{
		for (int j = 0; j <= fCanvasFrame.IntegerWidth(); j++)
		{
			last = (i == fCanvasFrame.IntegerHeight() && j == fCanvasFrame.IntegerWidth());
			rgb_color c = getColor (BPoint (j, i));
			if (c.alpha == 0)
				c = B_TRANSPARENT_32_BIT;
			uint8 val = screen.IndexForColor (c);
			fprintf (f, "0x%02x%s", val, last ? "" : ", ");
		}
		fprintf (f, "\n%s", last ? "" : "	");
	}
	fprintf (f, "};\n");
	fclose (f);

}
