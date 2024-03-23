#include "BitmapStuff.h"
#include "Brush.h"
#include "hsv.h"
#include <zlib.h>
#include "ProgressiveBitmapStream.h"
#include "BecassoAddOn.h"
#include <stdio.h>
#include <Path.h>
#include <stdlib.h>
#include <string.h>
#include <TranslationUtils.h>
#include <Alert.h>
#include <Entry.h>
#include <Screen.h>
#include "AttribDraw.h" // for MAX_LAYERS
#include "SView.h"
#include "SBitmap.h"
#include <Debug.h>
#include <Node.h>

#if defined(DATATYPES)
#include "Datatypes.h"
#endif

#if defined(__POWERPC__)
#pragma optimization_level 4
#pragma peephole on
#endif

// Note: This is a design mistake, discovered when 1.3 was already being released.
// The "Merge" function should have been in here from day one; this is a copy of
// the same method in CanvasView.

void
Merge(BBitmap* a, Layer* b, BRect update, bool doselect, bool preserve_alpha);

void
Merge(BBitmap* a, Layer* b, BRect update, bool doselect, bool preserve_alpha)
{
	//	clock_t start, end;
	//	start = clock();
	doselect = doselect;
	preserve_alpha = preserve_alpha;
	//	ulong h = update.IntegerHeight() + 1;	// Was: fCanvasFrame!
	ulong w = update.IntegerWidth() + 1;
	//	ulong sbbpr = a->BytesPerRow();
	ulong sbpr = b->BytesPerRow() / 4;
	ulong rt = ulong(update.top);
	ulong rb = ulong(update.bottom);
	ulong rl = ulong(update.left);
	ulong rr = ulong(update.right);
	ulong dw = rr - rl + 1;
	ulong ddiff = (w - dw);
	ulong sdiff = sbpr - dw;
	//	ulong brl = rl;
	uint32* src = (uint32*)b->Bits() + rt * sbpr + rl - 1;
	uint32* dest = (uint32*)a->Bits() + (rt * w + rl) - 1;
	int ga = b->getGlobalAlpha();
	switch (b->getMode()) {
	case 0:
		for (ulong y = rt; y <= rb; y++) {
			for (ulong x = rl; x <= rr; x++) {
#if defined(__POWERPC__)
				register uint32 srcpixel = *(++src);
				register uint32 destpixel = *(++dest);
				register int sa = (srcpixel & 0xFF) * ga / 255;
				register int da = 255 - sa;
				if (da == 0) // Fully opaque pixel
				{
					*dest = srcpixel;
				} else if (da == 255) // Fully transparent pixel
				{
				} else {
#if !defined(BLEND_USES_SHIFTS)
					*dest =
						((((destpixel & 0xFF000000) / 255 * da + (srcpixel & 0xFF000000) / 255 * sa)
						 ) &
						 0xFF000000) |
						((((destpixel & 0x00FF0000) * da + (srcpixel & 0x00FF0000) * sa) / 255) &
						 0x00FF0000) |
						((((destpixel & 0x0000FF00) * da + (srcpixel & 0x0000FF00) * sa) / 255) &
						 0x0000FF00) | // (max_c (sa, destpixel & 0xFF));
						(clipchar(sa + int(destpixel & 0xFF)));
#else
					*dest = (((((destpixel & 0xFF000000) >> 8) * da +
							   ((srcpixel & 0xFF000000) >> 8) * sa)) &
							 0xFF000000) |
							((((destpixel & 0x00FF0000) * da + (srcpixel & 0x00FF0000) * sa) >> 8) &
							 0x00FF0000) |
							((((destpixel & 0x0000FF00) * da + (srcpixel & 0x0000FF00) * sa) >> 8) &
							 0x0000FF00) |
							(clipchar(sa + int(destpixel & 0xFF)));
#endif
				}
#else
				register uint32 srcpixel = *(++src);
				register uint32 destpixel = *(++dest);
				register int sa = (srcpixel >> 24) * ga / 255;
				register int da = 255 - sa;
				if (da == 0) // Fully opaque pixel
				{
					*dest = srcpixel;
				} else if (da == 255) // Fully transparent pixel
				{
				} else {
#if !defined(BLEND_USES_SHIFTS)
					*dest =
						((((destpixel & 0x00FF0000) * da + (srcpixel & 0x00FF0000) * sa) / 255) &
						 0x00FF0000) |
						((((destpixel & 0x0000FF00) * da + (srcpixel & 0x0000FF00) * sa) / 255) &
						 0x0000FF00) |
						((((destpixel & 0x000000FF) * da + (srcpixel & 0x000000FF) * sa) / 255) &
						 0x000000FF) |
						(clipchar(sa + int(destpixel >> 24)) << 24);
#else
					*dest = ((((destpixel & 0x00FF0000) * da + (srcpixel & 0x00FF0000) * sa) >> 8) &
							 0x00FF0000) |
							((((destpixel & 0x0000FF00) * da + (srcpixel & 0x0000FF00) * sa) >> 8) &
							 0x0000FF00) |
							((((destpixel & 0x000000FF) * da + (srcpixel & 0x000000FF) * sa) >> 8) &
							 0x000000FF) |
							(clipchar(sa + int(destpixel >> 24)) << 24);
#endif
				}
#endif
			}
			src += sdiff;
			dest += ddiff;
		}
		break;
	case 1:
		for (ulong y = rt; y <= rb; y++) {
			for (ulong x = rl; x <= rr; x++) {
#if defined(__POWERPC__)
				register uint32 srcpixel = *(++src);
				register uint32 destpixel = *(++dest);
				register int sa = (srcpixel & 0xFF) * ga / 255;
				register int da = destpixel & 0xFF;
				register int tsa = 65025 - sa * 255;
				register int tda = 65025 - da * 255;
				if (sa == 0) // Fully transparent pixel
				{
				} else if (sa == 255) // Fully opaque pixel
				{
					*dest =
						(((((srcpixel >> 24)) * (tda + da * ((destpixel >> 24))) / 65025) << 24) &
						 0xFF000000) |
						(((((srcpixel >> 16) & 0xFF) * (tda + da * ((destpixel >> 16) & 0xFF)) /
						   65025)
						  << 16) &
						 0x00FF0000) |
						(((((srcpixel >> 8) & 0xFF) * (tda + da * ((destpixel >> 8) & 0xFF)) / 65025
						  )
						  << 8) &
						 0x0000FF00) |
						(clipchar(sa + int(destpixel & 0xFF)));
				} else {
					*dest = (((tsa + sa * ((srcpixel >> 24))) * (tda + da * ((destpixel >> 24))) /
								  16581375
							  << 24) &
							 0xFF000000) |
							(((tsa + sa * ((srcpixel >> 16) & 0xFF)) *
								  (tda + da * ((destpixel >> 16) & 0xFF)) / 16581375
							  << 16) &
							 0x00FF0000) |
							(((tsa + sa * ((srcpixel >> 8) & 0xFF)) *
								  (tda + da * ((destpixel >> 8) & 0xFF)) / 16581375
							  << 8) &
							 0x0000FF00) |
							(clipchar(sa + int(destpixel & 0xFF)));
				}
#else
				register uint32 srcpixel = *(++src);
				register uint32 destpixel = *(++dest);
				register int sa = (srcpixel >> 24) * ga / 255;
				register int da = destpixel >> 24;
				register int tsa = 65025 - sa * 255;
				register int tda = 65025 - da * 255;
				if (sa == 0) // Fully transparent pixel
				{
				} else if (sa == 255) // Fully opaque pixel
				{
					*dest = (((((srcpixel >> 16) & 0xFF) * (tda + da * ((destpixel >> 16) & 0xFF)) /
							   65025)
							  << 16) &
							 0x00FF0000) |
							(((((srcpixel >> 8) & 0xFF) * (tda + da * ((destpixel >> 8) & 0xFF)) /
							   65025)
							  << 8) &
							 0x0000FF00) |
							(((((srcpixel) & 0xFF) * (tda + da * ((destpixel) & 0xFF)) / 65025)) &
							 0x000000FF) |
							(clipchar(sa + int(destpixel >> 24)) << 24);
				} else {
					*dest = (((tsa + sa * ((srcpixel >> 16) & 0xFF)) *
								  (tda + da * ((destpixel >> 16) & 0xFF)) / 16581375
							  << 16) &
							 0x00FF0000) |
							(((tsa + sa * ((srcpixel >> 8) & 0xFF)) *
								  (tda + da * ((destpixel >> 8) & 0xFF)) / 16581375
							  << 8) &
							 0x0000FF00) |
							(((tsa + sa * ((srcpixel) & 0xFF)) * (tda + da * ((destpixel) & 0xFF)) /
							  16581375) &
							 0x000000FF) |
							(clipchar(sa + int(destpixel >> 24)) << 24);
				}
#endif
			}
			src += sdiff;
			dest += ddiff;
		}
		break;
	default:
		fprintf(stderr, "Merge:  Unknown Drawing Mode\n");
		break;
	}
	//	end = clock();
	//	printf ("Merge took %d ms\n", end - start);
}

void
InsertGlobalAlpha(Layer* layer[], int numLayers) // Watermark all layers by inverting
{
	SView* demoView = new SView(layer[0]->Bounds(), "demoview", B_FOLLOW_NONE, uint32(NULL));
	const char demotext[] = "Becasso";
	float h = layer[0]->Bounds().Height();
	float w = layer[0]->Bounds().Width();
	BFont font(be_bold_font);
	font_height fh;
	font.GetHeight(&fh);
	float ratio = font.StringWidth(demotext) / (fh.ascent + fh.descent + fh.leading);
	float size = sqrt(h * h / (ratio * ratio) + w * w) / font.StringWidth(demotext) * 11;
	float angle = 360 * atan2(h - (fh.ascent + fh.leading), w) / 2 / M_PI;
	font.SetSize(size);
	// printf ("Size = %f, ratio = %f, sqrt = %f\n", size, ratio, sqrt (h*h/(ratio*ratio) + w*w));
	font.SetRotation(angle);
	font.SetFlags(B_DISABLE_ANTIALIASING);
	BBitmap* tmp = new BBitmap(layer[0]->Bounds(), B_RGBA32, true);
	tmp->Lock();
	tmp->AddChild(demoView);
	demoView->SetFont(&font);
	demoView->SetFont(&font);
	demoView->SetHighColor(0xBE, 0xBE, 0xBE, 0xBE);
	demoView->SetDrawingMode(B_OP_COPY);
	demoView->DrawString(demotext, BPoint(fh.ascent + fh.descent + fh.leading, h - fh.descent));
	demoView->Sync();
	tmp->RemoveChild(demoView);
	delete demoView;

	for (int i = 0; i < numLayers; i++) {
		layer[i]->Lock();
		uint32* dst = (uint32*)layer[i]->Bits();
		uint32* src = (uint32*)tmp->Bits();
		uint32 size = layer[i]->BitsLength() / 4;
		for (uint32 j = 0; j < size; j++) {
			if (*src++ == 0xBEBEBEBE)
				*dst++ ^= COLOR_MASK;
			else
				dst++;
		}
		layer[i]->Unlock();
	}
	delete tmp;
}

BBitmap*
entry2bitmap(BEntry entry, bool silent)
{
	extern const char* Version;
	//	extern bool DataTypes;
	BBitmap* map = NULL;
	char type[80];
	BNode node = BNode(&entry);
	ssize_t s;
	FILE* fp = NULL;
	if ((s = node.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, type, 79)) > 0) {
		if (!strcmp(type, "image/becasso") || !strcmp(type, "image/x-becasso")) {
			Layer* layer[MAX_LAYERS];
			BPath path;
			entry.GetPath(&path);
			fp = fopen(path.Path(), "r");
			char line[81];
			do {
				fgets(line, 80, fp);
			} while (line[0] == '#');
			char* endp = line;
			float tvers = atof(Version);
			float cvers = strtod(endp, &endp);
			int numLayers = strtol(endp, &endp, 10);
			/* int currentlayer = */ strtol(endp, &endp, 10); // not used
			/* int dpi = */ strtol(endp, &endp, 10);		  // not used
			int watermarked = strtol(endp, &endp, 10);		  // watermarked image?
			// printf ("#Layers = %d\n", numLayers);
			/*int currentLayer = */ strtol(endp, &endp, 10);
			BRect lFrame;
			if (int(cvers) > int(tvers)) // Check major version
			{
				return NULL;
			}
			if (int(cvers) == int(tvers) &&
				cvers - int(cvers) > tvers - int(tvers)) // Check release
			{
				return NULL;
			}
			switch (int(cvers)) {
			case 0:
				for (int i = 0; i < numLayers; i++) {
					fgets(line, 80, fp);
					line[strlen(line) - 1] = 0;
					endp = line;
					lFrame.left = strtod(endp, &endp);
					lFrame.top = strtod(endp, &endp);
					lFrame.right = strtod(endp, &endp);
					lFrame.bottom = strtod(endp, &endp);
					//			int m = strtol (endp, &endp, 10);
					bool h = strtol(endp, &endp, 10);
					layer[i] = new Layer(lFrame, endp);
					layer[i]->Hide(h);
					layer[i]->setMode(0
					); // Note: DM_BLEND has moved to 0, and <1.1 didn't have any other ops anyway.
					fread(layer[i]->Bits(), layer[i]->BitsLength(), 1, fp);
				}
				break;
			case 1: {
				uchar* tmpzbuf = NULL;
				z_streamp zstream = new z_stream;
				zstream->zalloc = Z_NULL;
				zstream->zfree = Z_NULL;
				zstream->opaque = Z_NULL;
				if (inflateInit(zstream) != Z_OK) {
					fprintf(stderr, "Oops!  Problems with inflateInit()...\n");
				}
				for (int i = 0; i < numLayers; i++) {
					fgets(line, 80, fp);
					line[strlen(line) - 1] = 0;
					// printf ("%s\n", line);
					endp = line;
					lFrame.left = strtod(endp, &endp);
					lFrame.top = strtod(endp, &endp);
					lFrame.right = strtod(endp, &endp);
					lFrame.bottom = strtod(endp, &endp);
					int m = strtol(endp, &endp, 10);
					bool h = strtol(endp, &endp, 10);
					uchar ga = strtol(endp, &endp, 10);
					if (cvers > 1.1) {
						/* int am = */ strtol(endp, &endp, 10);
						// printf ("%salpha map\n", am ? "" : "no ");
					}
					layer[i] = new Layer(lFrame, endp);
					layer[i]->Hide(h);
					if (cvers == 1.0)
						layer[i]->setMode(0); // Note: DM_BLEND has moved to 0, and <1.1 didn't have
											  // any other ops anyway.
					else
						layer[i]->setMode(m);
					layer[i]->setGlobalAlpha(ga);
					layer[i]->setAlphaMap(NULL);
				}
				for (int i = 0; i < numLayers; i++) {
					if (!tmpzbuf) {
						tmpzbuf = new uchar[layer[i]->BitsLength()];
						zstream->avail_in = 0;
					}
					zstream->next_in = tmpzbuf;
					zstream->avail_in +=
						fread(tmpzbuf, 1, layer[i]->BitsLength() - zstream->avail_in, fp);
					zstream->next_out = (uchar*)layer[i]->Bits();
					zstream->avail_out = layer[i]->BitsLength();
					//			printf ("avail_in = %li, avail_out = %li\n", zstream->avail_in,
					// zstream->avail_out);
					if (inflate(zstream, Z_FINISH) != Z_STREAM_END) {
						fprintf(stderr, "Oops!  Layer couldn't be decompressed completely...\n");
					}
					memmove(tmpzbuf, zstream->next_in, zstream->avail_in);
					inflateReset(zstream);
				}
				if (inflateEnd(zstream) != Z_OK) {
					fprintf(stderr, "Oops!  Temporary zlib buffer couldn't be deallocated\n");
				}
				delete[] tmpzbuf;
				delete zstream;
				break;

				if (watermarked)
					InsertGlobalAlpha(layer, numLayers);
			}
			case 2: {
				uchar* tmpzbuf = NULL;
				z_streamp zstream = new z_stream;
				zstream->zalloc = Z_NULL;
				zstream->zfree = Z_NULL;
				zstream->opaque = Z_NULL;
				if (inflateInit(zstream) != Z_OK) {
					fprintf(stderr, "Oops!  Problems with inflateInit()...\n");
				}
				for (int i = 0; i < numLayers; i++) {
					fgets(line, 80, fp);
					line[strlen(line) - 1] = 0;
					// printf ("%s\n", line);
					endp = line;
					lFrame.left = strtod(endp, &endp);
					lFrame.top = strtod(endp, &endp);
					lFrame.right = strtod(endp, &endp);
					lFrame.bottom = strtod(endp, &endp);
					int m = strtol(endp, &endp, 10);
					bool h = strtol(endp, &endp, 10);
					uchar ga = strtol(endp, &endp, 10);
					if (cvers > 1.1) {
						/* int am = */ strtol(endp, &endp, 10);
						// printf ("%salpha map\n", am ? "" : "no ");
					}
					layer[i] = new Layer(lFrame, endp);
					layer[i]->Hide(h);
					if (cvers == 1.0)
						layer[i]->setMode(0); // Note: DM_BLEND has moved to 0, and <1.1 didn't have
											  // any other ops anyway.
					else
						layer[i]->setMode(m);
					layer[i]->setGlobalAlpha(ga);
					layer[i]->setAlphaMap(NULL);
				}
				for (int i = 0; i < numLayers; i++) {
					if (!tmpzbuf) {
						tmpzbuf = new uchar[layer[i]->BitsLength()];
						zstream->avail_in = 0;
					}
					zstream->next_in = tmpzbuf;
					zstream->avail_in +=
						fread(tmpzbuf, 1, layer[i]->BitsLength() - zstream->avail_in, fp);
					zstream->next_out = (uchar*)layer[i]->Bits();
					zstream->avail_out = layer[i]->BitsLength();
					//			printf ("avail_in = %li, avail_out = %li\n", zstream->avail_in,
					// zstream->avail_out);
					if (inflate(zstream, Z_FINISH) != Z_STREAM_END) {
						fprintf(stderr, "Oops!  Layer couldn't be decompressed completely...\n");
					}
					memmove(tmpzbuf, zstream->next_in, zstream->avail_in);
					inflateReset(zstream);
				}
				if (inflateEnd(zstream) != Z_OK) {
					fprintf(stderr, "Oops!  Temporary zlib buffer couldn't be deallocated\n");
				}
				delete[] tmpzbuf;
				delete zstream;
				break;

				if (watermarked)
					InsertGlobalAlpha(layer, numLayers);
			}
			default: // This should never happen
				fprintf(
					stderr,
					"Hmm, somehow a newer version file got through to the actual loading routine.\n"
				);
			}
			fclose(fp);
			// OK, we've got the layers.  Now blend them into a BBitmap.

			//			lFrame.PrintToStream();
			map = new BBitmap(lFrame, B_RGBA32, false);
			//			ulong h = height;
			ulong w = lFrame.IntegerWidth() + 1;
			//			ulong sbbpr = bitmap->BytesPerRow();
			ulong sbpr = layer[0]->BytesPerRow();
			ulong rt = ulong(lFrame.top);
			ulong rb = ulong(lFrame.bottom);
			ulong rl = ulong(lFrame.left);
			ulong rr = ulong(lFrame.right);
			ulong dw = rr - rl + 1;
			ulong ddiff = (w - dw);
			//				ulong sdiff = sbpr - dw;
			uint32* src = (uint32*)layer[0]->Bits();
			src += rt * sbpr + rl;
			uint32* dest = (uint32*)map->Bits() + (rt * w + rl) - 1;
			//				ulong brl = rl;

// printf ("Background layer: [%ld, %ld, %ld, %ld]\n", rl, rt, rr, rb);
// printf ("3"); fflush (stdout);
#if defined(__POWERPC__)
			uint32 check1 = 0xFFFFFF00;
			uint32 check2 = 0xFFFFFF00;
#else
			uint32 check1 = 0x00FFFFFF;
			uint32 check2 = 0x00FFFFFF;
#endif
			for (ulong y = rt; y <= rb; y++) // First layer: Background.
			{
				for (ulong x = rl; x <= rr; x++) {
					if (((x & 8) || (y & 8)) && !((x & 8) && (y & 8))) {
						*(++dest) = check1;
					} else {
						*(++dest) = check2;
					}
				}
				dest += ddiff;
			}
			for (int i = 0; i < numLayers; i++) // Next the layers: Add.
			{
				// printf ("Layer %i\n", i);
				if (!layer[i]->IsHidden()) {
					Merge(map, layer[i], lFrame, false, true);
				}
			}

			return map;
		}
	}

#if defined(DATATYPES)
	if (DataTypes) {
		ProgressiveBitmapStream* bms;
		BFile inStream(&entry, B_READ_ONLY);
		bms = new ProgressiveBitmapStream();
		//		bms->DisplayProgressBar (BPoint (200, 200), "Reading Image…");
		if (DATATranslate(inStream, NULL, NULL, *bms, DATA_BITMAP)) {
			if (!silent) {
				// Error translating...
				char errstring[B_FILE_NAME_LENGTH + 64];
				sprintf(errstring, "Datatypes error:\nCouldn't translate `%s'", fName);
				BAlert* alert =
					new BAlert("", errstring, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go();
			}
		} else {
			map = bms->GetBitmap();
			bms->SetDispose(false);
		}
		delete bms;
	} else {
		printf("No DataTypes (?!)\n");
		// No DataTypes...
	}
#else
	BPath path;
	entry.GetPath(&path);
	if (!(map = BTranslationUtils::GetBitmapFile(path.Path())) && !silent) {
		char errstring[B_FILE_NAME_LENGTH + 64];
		sprintf(errstring, "Translation Kit error:\nCouldn't translate '%s'", path.Path());
		BAlert* alert =
			new BAlert("", errstring, "Help", "OK", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		uint32 button = alert->Go();
		if (button == 0) {
			BNode node(&entry);
			char filetype[80];
			filetype[node.ReadAttr("BEOS:TYPE", 0, 0, filetype, 80)] = 0;
			if (!strncmp(filetype, "image/", 6)) {
				sprintf(
					errstring,
					"The file '%s' is of type '%s'.\nThis does look like an image format to "
					"me.\nMaybe you haven't installed the corresponding Translator?",
					path.Path(), filetype
				);
			} else {
				sprintf(
					errstring,
					"The file '%s' is of type '%s'.\nI don't think this is an image at all.",
					path.Path(), filetype
				);
			}
			alert = new BAlert("", errstring, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
			alert->Go();
		}
	}
#endif
	return map;
}

float
AverageAlpha(BBitmap* src)
{
	float res = 0;
	uint32* src_data = (uint32*)src->Bits();
	int w = src->Bounds().IntegerWidth();
	int h = src->Bounds().IntegerHeight();
	src_data--;
	for (int i = 0; i <= h; i++)
		for (int j = 0; j <= w; j++)
#if defined(__POWERPC__)
			res += *(++src_data) & 0xFF;
#else
			res += (*(++src_data) >> 24);
#endif
	res /= (w + 1) * (h + 1);
	return res;
}

void
InvertAlpha(BBitmap* src)
{
	//	printf ("AverageAlpha before = %f\n", AverageAlpha (src));
	uint32* src_data = (uint32*)src->Bits() - 1;
	int w = src->Bounds().IntegerWidth();
	int h = src->Bounds().IntegerHeight();
	for (int i = 0; i <= h; i++)
		for (int j = 0; j <= w; j++) {
			uint32 pixel = *(++src_data);
			*src_data = (pixel & COLOR_MASK) | ((255 - ALPHA(pixel)) << ALPHA_BPOS);
		}
	//	printf ("AverageAlpha after  = %f\n", AverageAlpha (src));
}

void
BlendWithAlpha(BBitmap* src, BBitmap* dest, long x, long y, int /* strength */)
{
	// src: 32bit BBitmap
	// dest: 32bit BBitmap
	// Note: Alpha = opacity!
	uint32* src_data = (uint32*)src->Bits();
	ulong src_bpr = src->BytesPerRow() / 4;
	uint32* dest_data = (uint32*)dest->Bits();
	ulong dest_bpr = dest->BytesPerRow() / 4;
	int src_w = src->Bounds().IntegerWidth();
	int src_h = src->Bounds().IntegerHeight();
	int dest_w = dest->Bounds().IntegerWidth();
	int dest_h = dest->Bounds().IntegerHeight();
	if ((x + src_w <= 0) || (x > dest_w) || (y + src_h <= 0) || (y > dest_h)) {
		return;
	}

	// Clipping
	int minx = max_c(x, 0);
	int maxx = min_c(x + src_w + 1, dest_w + 1);
	int miny = max_c(y, 0);
	int maxy = min_c(y + src_h + 1, dest_h + 1);

	ulong src_bprdiff = src_bpr - (maxx - minx);
	ulong dest_bprdiff = dest_bpr - (maxx - minx);

	if (y < 0)
		src_data -= y * src_bpr;
	if (x < 0)
		src_data -= x;

	dest_data += miny * dest_bpr + minx;

	dest_data--; // Note: On PPC, pre-increment is faster.
	src_data--;
	for (int j = miny; j < maxy; j++) {
		for (int i = minx; i < maxx; i++) {
#if defined(__POWERPC__)
			register uint32 srcpixel = *(++src_data);
			register uint32 destpixel = *(++dest_data);
			register int sa = srcpixel & 0xFF;
			register int da = 255 - sa; // destpixel & 0xFF;
			register int ta = 255;		// sa + da;
			if (sa == 255)				// Fully Opaque
			{
				*dest_data = srcpixel;
			} else if (sa == 0) // Fully transparent
			{
			} else {
				*dest_data =
					((((((destpixel >> 24)) * da + ((srcpixel >> 24)) * sa) / ta) << 24) &
					 0xFF000000) |
					((((((destpixel >> 16) & 0xFF) * da + ((srcpixel >> 16) & 0xFF) * sa) / ta)
					  << 16) &
					 0x00FF0000) |
					((((((destpixel >> 8) & 0xFF) * da + ((srcpixel >> 8) & 0xFF) * sa) / ta)
					  << 8) &
					 0x0000FF00) |
					clipchar(sa + int(destpixel & 0xFF));
			}
#else
			register uint32 srcpixel = *(++src_data);
			register uint32 destpixel = *(++dest_data);
			register int sa = srcpixel >> 24;
			register int da = 255 - sa; // destpixel >> 24;
			register int ta = 255;		// sa + da;
			if (sa == 255)				// Fully Opaque
			{
				*dest_data = srcpixel;
			} else if (sa == 0) // Fully transparent
			{
			} else {
				*dest_data =
					((((((destpixel >> 16) & 0xFF) * da + ((srcpixel >> 16) & 0xFF) * sa) / ta)
					  << 16) &
					 0x00FF0000) |
					((((((destpixel >> 8) & 0xFF) * da + ((srcpixel >> 8) & 0xFF) * sa) / ta)
					  << 8) &
					 0x0000FF00) |
					((((((destpixel) & 0xFF) * da + ((srcpixel) & 0xFF) * sa) / ta)) & 0x000000FF) |
					((clipchar(sa + int(destpixel >> 24))) << 24);
			}
#endif
		}
		src_data += src_bprdiff;
		dest_data += dest_bprdiff;
	}
}

void
AddWithAlpha(BBitmap* src, BBitmap* dest, long x, long y, int strength)
{
	// src: 32bit BBitmap
	// dest: 32bit BBitmap
	// Note: Alpha = opacity!
	uint32* src_data = (uint32*)src->Bits();
	ulong src_bpr = src->BytesPerRow() / 4;
	uint32* dest_data = (uint32*)dest->Bits();
	ulong dest_bpr = dest->BytesPerRow() / 4;
	int src_w = src->Bounds().IntegerWidth();
	int src_h = src->Bounds().IntegerHeight();
	int dest_w = dest->Bounds().IntegerWidth();
	int dest_h = dest->Bounds().IntegerHeight();
	if ((x + src_w <= 0) || (x > dest_w) || (y + src_h <= 0) || (y > dest_h) || !strength) {
		return;
	}

	// Clipping
	int minx = max_c(x, 0);
	int maxx = min_c(x + src_w + 1, dest_w + 1);
	int miny = max_c(y, 0);
	int maxy = min_c(y + src_h + 1, dest_h + 1);

	ulong src_bprdiff = src_bpr - (maxx - minx);
	ulong dest_bprdiff = dest_bpr - (maxx - minx);

	if (y < 0)
		src_data -= y * src_bpr;
	if (x < 0)
		src_data -= x;

	dest_data += miny * dest_bpr + minx;

	dest_data--; // Note: On PPC, pre-increment is faster.
	src_data--;
	if (strength == 255) {
		for (int j = miny; j < maxy; j++) {
			for (int i = minx; i < maxx; i++) {
#if defined(__POWERPC__)
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = srcpixel & 0xFF;
				register int da = /* destpixel & 0xFF; */ 255 - sa;
				register int ta = /* sa + da;*/ 255;
				if (sa == 255 || /* da == 0*/ !(destpixel & 0xFF)) // Fully opaque
				{
					*dest_data = srcpixel;
				} else if (sa == 0) // Fully transparent
				{
				} else {
					//					*dest_data	= ((((((destpixel>>24)       )*da +
					//((srcpixel>>24)       )*sa)/ta)<<24) & 0xFF000000) |
					//								  ((((((destpixel>>16) & 0xFF)*da +
					//((srcpixel>>16) & 0xFF)*sa)/ta)<<16) & 0x00FF0000) |
					//								  ((((((destpixel>> 8) & 0xFF)*da + ((srcpixel>>
					// 8) & 0xFF)*sa)/ta)<< 8) & 0x0000FF00) |
					// clipchar (sa + int (destpixel & 0xFF));
					*dest_data =
						((((destpixel & 0xFF000000) / ta) * da + ((srcpixel & 0xFF000000) / ta) * sa
						 ) &
						 0xFF000000) |
						((((destpixel & 0x00FF0000) / ta) * da + ((srcpixel & 0x00FF0000) / ta) * sa
						 ) &
						 0x00FF0000) |
						((((destpixel & 0x0000FF00) * da + (srcpixel & 0x0000FF00) * sa)) / ta &
						 0x0000FF00) |
						clipchar(sa + int(destpixel & 0xFF));
					// clipchar (ta);
				}
#else
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = srcpixel >> 24;
				register int da = /*destpixel >> 24;*/ 255 - sa;
				register int ta = /*sa + da; */ ta = 255;
				if (sa == 255 || /*da == 0*/ !(destpixel & 0xFF000000)) // Fully opaque
				{
					*dest_data = srcpixel;
				} else if (sa == 0) // Fully transparent
				{
				} else {
					*dest_data =
						((((((destpixel & 0x00FF0000) >> 1) * da +
							((srcpixel & 0x00FF0000) >> 1) * sa) /
						   ta)
						  << 1) &
						 0x00FF0000) |
						((((destpixel & 0x0000FF00) * da + (srcpixel & 0x0000FF00) * sa) / ta) &
						 0x0000FF00) |
						((((destpixel & 0x000000FF) * da + (srcpixel & 0x000000FF) * sa) / ta) &
						 0x000000FF) |
						(clipchar(sa + int(destpixel >> 24)) << 24);
				}
#endif
			}
			src_data += src_bprdiff;
			dest_data += dest_bprdiff;
		}
	} else if (strength > 0) {
		for (int j = miny; j < maxy; j++) {
			for (int i = minx; i < maxx; i++) {
#if defined(__POWERPC__)
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = (srcpixel & 0xFF) * strength / 255;
				register int da = /* destpixel & 0xFF;*/ 255 - sa;
				register int ta = /* sa + da;*/ 255;
				if (sa == 255 || /* da == 0*/ !(destpixel & 0xFF)) // Fully opaque
				{
					*dest_data = srcpixel;
				} else if (sa == 0) // Fully transparent
				{
				} else {
					//					*dest_data	= ((((((destpixel>>24)       )*da +
					//((srcpixel>>24)       )*sa)/ta)<<24) & 0xFF000000) |
					//								  ((((((destpixel>>16) & 0xFF)*da +
					//((srcpixel>>16) & 0xFF)*sa)/ta)<<16) & 0x00FF0000) |
					//								  ((((((destpixel>> 8) & 0xFF)*da + ((srcpixel>>
					// 8) & 0xFF)*sa)/ta)<< 8) & 0x0000FF00) |
					// clipchar (sa + int (destpixel & 0xFF));
					*dest_data =
						((((destpixel & 0xFF000000) / ta) * da + ((srcpixel & 0xFF000000) / ta) * sa
						 ) &
						 0xFF000000) |
						((((destpixel & 0x00FF0000) / ta) * da + ((srcpixel & 0x00FF0000) / ta) * sa
						 ) &
						 0x00FF0000) |
						((((destpixel & 0x0000FF00) * da + (srcpixel & 0x0000FF00) * sa)) / ta &
						 0x0000FF00) |
						clipchar(sa + int(destpixel & 0xFF));
					// clipchar (ta);
				}
#else
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = (srcpixel >> 24) * strength / 255;
				register int da = /*destpixel >> 24;*/ 255 - sa;
				register int ta = sa + da;								  // 255
				if (sa == 255 || /* da == 0 */ !(destpixel & 0xFF000000)) // Fully opaque
				{
					*dest_data = srcpixel;
				} else if (sa == 0) // Fully transparent
				{
				} else {
					*dest_data =
						((((((destpixel & 0x00FF0000) >> 1) * da +
							((srcpixel & 0x00FF0000) >> 1) * sa) /
						   ta)
						  << 1) &
						 0x00FF0000) |
						((((destpixel & 0x0000FF00) * da + (srcpixel & 0x0000FF00) * sa) / ta) &
						 0x0000FF00) |
						((((destpixel & 0x000000FF) * da + (srcpixel & 0x000000FF) * sa) / ta) &
						 0x000000FF) |
						(clipchar(sa + int(destpixel >> 24)) << 24);
				}
#endif
			}
			src_data += src_bprdiff;
			dest_data += dest_bprdiff;
		}
	} else // Very special case:  Only "erase" transparency.
	{
		for (int j = miny; j < maxy; j++) {
			for (int i = minx; i < maxx; i++) {
#if defined(__POWERPC__)
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = (srcpixel & 0xFF) * (-strength) / 255;
				register int da = (destpixel & 0xFF);
				register int ta = 255;
				if (sa == 255 || !(destpixel & 0xFF)) // Fully opaque
				{
					*dest_data = destpixel & 0xFFFFFF00;
				} else if (sa == 0) // Fully transparent
				{
				} else {
					*dest_data = (destpixel & 0xFFFFFF00) | clipchar(da - sa);
				}
#else
				register uint32 srcpixel = *(++src_data);
				register uint32 destpixel = *(++dest_data);
				register int sa = (srcpixel >> 24) * (-strength) / 255;
				register int da = (destpixel >> 24);
				//				register int ta = 255;
				if (sa == 255 || !(destpixel & 0xFF000000)) // Fully opaque
				{
					*dest_data = destpixel & 0x00FFFFFF;
				} else if (sa == 0) // Fully transparent
				{
				} else {
					*dest_data = (destpixel & 0x00FFFFFF) | (clipchar(da - sa) << 24);
				}
#endif
			}
			src_data += src_bprdiff;
			dest_data += dest_bprdiff;
		}
	}
}

void
InvertWithInverseAlpha(BBitmap* src, BBitmap* selection)
// Note: Inverse alpha because the selection map is an inverse alpha map.
//		  i.e. alpha = transparency
{
	long w = src->Bounds().IntegerWidth();
	long h = src->Bounds().IntegerHeight();
	uchar* src_data = (uchar*)src->Bits();
	uchar* sel_data = (uchar*)selection->Bits();
	long src_bpr = src->BytesPerRow();
	long sel_bpr = selection->BytesPerRow();
	long sel_bprdif = sel_bpr - w - 1;

	src_data--;
	sel_data--;
	if (src->ColorSpace() == B_RGBA32) {
		for (int j = 0; j <= h; j++) {
			for (int i = 0; i <= w; i++) {
				register unsigned int ialpha = *(++sel_data);
				if (ialpha == 255)
					src_data += 4;
				else {
					*src_data = 255 - *(++src_data);
					*src_data = 255 - *(++src_data);
					*src_data = 255 - *(++src_data);
					++src_data;
				}
			}
			sel_data += sel_bprdif;
		}
	} else if (src->ColorSpace() == B_COLOR_8_BIT) {
		const color_map* map = system_colors();
		long src_bprdif = src_bpr - w - 1;
		for (int j = 0; j <= h; j++) {
			for (int i = 0; i <= w; i++) {
				register unsigned int ialpha = *(++sel_data);
				if (ialpha == 255)
					++src_data;
				else
					*src_data = map->inversion_map[*(++src_data)];
			}
			sel_data += sel_bprdif;
			src_data += src_bprdif;
		}
	}
}

void
AddToSelection(Brush* src, BBitmap* dest, long x, long y, int strength)
{
	// dest:  Transparency alpha map, 8bit (selection)

	long src_w = src->Width();
	long src_h = src->Height();
	long dest_w = dest->Bounds().IntegerWidth();
	long dest_h = dest->Bounds().IntegerHeight();
	uchar* src_data = src->Data();
	ulong src_bpr = src_w;
	uchar* dest_data = (uchar*)dest->Bits();
	ulong dest_bpr = dest->BytesPerRow();
	if ((x + src_w <= 0) || (x > dest_w) || (y + src_h <= 0) || (y > dest_h) || !strength) {
		return;
	}

	// Clipping
	long minx = max_c(x, 0);
	long maxx = min_c(x + src_w + 1, dest_w + 1);
	long miny = max_c(y, 0);
	long maxy = min_c(y + src_h + 1, dest_h + 1);

	ulong src_bprdiff = src_bpr - (maxx - minx);
	ulong dest_bprdiff = dest_bpr - (maxx - minx);

	if (y < 0)
		src_data -= y * src_bpr;
	if (x < 0)
		src_data -= x;

	dest_data += miny * dest_bpr + minx;

	dest_data--;
	src_data--;
	if (strength == 255) {
		for (long j = miny; j < maxy; j++) {
			for (long i = minx; i < maxx; i++) {
				int sa = *(++src_data);
				//				int da = 255 - sa;
				*dest_data = clipchar(int(*(++dest_data) + sa));
			}
			src_data += src_bprdiff;
			dest_data += dest_bprdiff;
		}
	} else {
		for (long j = miny; j < maxy; j++) {
			for (long i = minx; i < maxx; i++) {
				int sa = *(++src_data) * strength / 255;
				//				int da = 255 - sa;
				*dest_data = clipchar(int(*(++dest_data) + sa));
			}
			src_data += src_bprdiff;
			dest_data += dest_bprdiff;
		}
	}
}

void
CopyWithTransparency(BBitmap* src, BBitmap* dest, long x, long y)
// This is for bitmaps with B_TRANSPARENT_32_BIT as transparency values
// (not `real' alpha-aware bitmaps)
{
#if defined(__POWERPC__)
	uint32 transparent = (B_TRANSPARENT_32_BIT.blue << 24) + (B_TRANSPARENT_32_BIT.green << 16) +
						 (B_TRANSPARENT_32_BIT.red << 8) + (B_TRANSPARENT_32_BIT.alpha);
#else
	uint32 transparent = (B_TRANSPARENT_32_BIT.alpha << 24) + (B_TRANSPARENT_32_BIT.red << 16) +
						 (B_TRANSPARENT_32_BIT.green << 8) + (B_TRANSPARENT_32_BIT.blue);
#endif
	uint32* src_data = (uint32*)src->Bits();
	ulong src_bprl = src->BytesPerRow() / 4;
	uint32* dest_data = (uint32*)dest->Bits();
	ulong dest_bprl = dest->BytesPerRow() / 4;
	long src_w = src->Bounds().IntegerWidth();
	long src_h = src->Bounds().IntegerHeight();
	long dest_w = dest->Bounds().IntegerWidth();
	long dest_h = dest->Bounds().IntegerHeight();
	if ((x + src_w <= 0) || (x > dest_w) || (y + src_h <= 0) || (y > dest_h)) {
		return;
	}

	// Clipping
	long minx = max_c(x, 0);
	long maxx = min_c(x + src_w + 1, dest_w + 1);
	long miny = max_c(y, 0);
	long maxy = min_c(y + src_h + 1, dest_h + 1);

	ulong src_bprldiff = src_bprl - (maxx - minx);
	ulong dest_bprldiff = dest_bprl - (maxx - minx);

	if (y < 0)
		src_data -= y * src_bprl;
	if (x < 0)
		src_data -= x;

	dest_data += miny * dest_bprl + minx;

	src_data--;
	for (long j = miny; j < maxy; j++) {
		for (long i = minx; i < maxx; i++) {
			if (*(++src_data) != transparent)
				*dest_data = *src_data;
			dest_data++;
		}
		src_data += src_bprldiff;
		dest_data += dest_bprldiff;
	}
}

BRect
GetSmallestRect(BBitmap* b)
{
	b->Lock();

	uchar* data = (uchar*)b->Bits();
	long bpr = b->BytesPerRow();
	long h = b->Bounds().IntegerHeight();
	long w = b->Bounds().IntegerWidth();
	long bprdif = bpr - w - 1;
	BRect res = BRect(w, h, 0, 0);

	data--;
	for (long i = 0; i <= h; i++) {
		for (long j = 0; j <= w; j++) {
			if (*(++data)) {
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
		data += bprdif;
	}

	b->Unlock();

	if (res.right < res.left || res.bottom < res.top)
		res = BRect(0, 0, w, h);

	return (res);
}

void
CutOrCopy(BBitmap* src, BBitmap* dest, BBitmap* sel, long offx, long offy, bool cut)
{
	// The following code copies a rect of dest->Bounds() out of `src' at (offx, offy)
	// into `dest' selectively via `sel'.
	// `sel' contains an opacity alpha map.
	// If sel == NULL, assume a full selection (Thomas Fletcher).
	// It also cuts out the selection from `src' if cut = true;
	// The resulting alpha channel is opacity.
	src->Lock();
	if (sel)
		sel->Lock();
	dest->Lock();
	uchar trans8;
#if defined(__POWERPC__)
	uint32 transparent = (B_TRANSPARENT_32_BIT.blue << 24) + (B_TRANSPARENT_32_BIT.green << 16) +
						 (B_TRANSPARENT_32_BIT.red << 8) + (B_TRANSPARENT_32_BIT.alpha);
#else
//	uint32 transparent = (B_TRANSPARENT_32_BIT.alpha << 24) + (B_TRANSPARENT_32_BIT.red << 16)
//		+ (B_TRANSPARENT_32_BIT.green << 8) + (B_TRANSPARENT_32_BIT.blue);
#endif
	{
		BScreen screen;
		trans8 = screen.IndexForColor(B_TRANSPARENT_32_BIT);
	}

	uchar* selbits = 0;
	long selbpr = 0;
	if (sel) {
		selbits = (uchar*)sel->Bits();
		selbpr = sel->BytesPerRow();
	}
	uint32* srcbits = (uint32*)src->Bits();
	long srcbprl = src->BytesPerRow() / 4;
	uint32* destbits = (uint32*)dest->Bits();
	//	long destbprl = dest->BytesPerRow()/4;
	ulong selbprdif = selbpr - long(dest->Bounds().right) - 1;
	ulong srcbprldif = srcbprl - long(dest->Bounds().right) - 1;

	selbits += offy * selbpr + offx;
	srcbits += offy * srcbprl + offx;

	selbits--;
	for (long i = 0; i <= dest->Bounds().bottom; i++) {
		for (long j = 0; j <= dest->Bounds().right; j++) {
			register unsigned int opacity = sel ? *(++selbits) : 255;
			//			register unsigned int transparency = 255 - opacity;
			//			uchar *dpixel = (uchar *)destbits;
			uchar* spixel = (uchar*)srcbits;
#if defined(__POWERPC__)
			*destbits = (*srcbits & 0xFFFFFF00) | ((opacity * spixel[3] / 255) & 0xFF);
#else
			*destbits = (*srcbits & 0x00FFFFFF) | ((opacity * spixel[3] / 255) << 24);
#endif
			if (cut)
				spixel[3] = clipchar(int(spixel[3] - opacity));
			destbits++;
			srcbits++;
		}
		selbits += selbprdif;
		srcbits += srcbprldif;
	}
	dest->Unlock();
	if (sel)
		sel->Unlock();
	src->Unlock();
}

void
FastCopy(BBitmap* src, BBitmap* dest)
{
	// Lock them yourself first!
	double* destdata = (double*)dest->Bits();
	double* srcdata = (double*)src->Bits();
	destdata--;
	srcdata--;
	for (long i = 0; i < dest->BitsLength() / 8; i++)
		*(++destdata) = *(++srcdata);
	if (dest->BitsLength() % 8) // Odd number of pixels...
		*((long*)++destdata) = *((long*)++srcdata);
}

#ifndef QUICK_DITHER

// check HSV!

void
FSDither(BBitmap* b32, BBitmap* b8, BRect place)
// Foley & Van Dam, pp 572.
// Own note:  Probably the errors in the different color channels should be weighted
//            according to visual sensibility.  But this version is primarily meant to
//            be quick.
{
	uint32 width = place.IntegerWidth() + 1;
	//	uint32 height	 = place.IntegerHeight() + 1;
	uint32 slpr = b32->BytesPerRow() / 4;
	bgra_pixel* src = (bgra_pixel*)b32->Bits() + int(place.top) * slpr + int(place.left) - 1;
	int32 sdif = slpr - width;
	uint32 dbpr = b8->BytesPerRow();
	uint8* dest = (uint8*)b8->Bits() + int(place.top) * dbpr + int(place.left) - 1;
	int32 ddif = dbpr - width;
	uint8* xpal = (uint8*)system_colors()->index_map;
	const rgb_color* sysc = system_colors()->color_list;

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
	uint8 appr;
	uint32 x, y;
	rgb_color a;

	for (y = uint32(place.top); y < uint32(place.bottom); y++) {
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
		appr = xpal[((r << 7) & 0x7C00) | ((g << 2) & 0x03E0) | ((b >> 3) & 0x001F)];
		*(++dest) = appr;

		// And the corresponding RGB color
		a = sysc[appr];

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
			appr = xpal[((r << 7) & 0x7C00) | ((g << 2) & 0x03E0) | ((b >> 3) & 0x001F)];
			*(++dest) = appr;

			// And the corresponding RGB color
			a = sysc[appr];

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
		s = *(++src);

		// Get color components and add errors from previous pixel
		r = clip8(RED(s) + per + cera[x]);
		g = clip8(GREEN(s) + peg + cega[x]);
		b = clip8(BLUE(s) + peb + ceba[x]);

		cera[x] = 0;
		cega[x] = 0;
		ceba[x] = 0;

		// Find the nearest match in the palette and write it out
		appr = xpal[((r << 7) & 0x7C00) | ((g << 2) & 0x03E0) | ((b >> 3) & 0x001F)];
		*(++dest) = appr;

		// And the corresponding RGB color
		a = sysc[appr];

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
	}
	// Special case: Last line
	// All the error goes into the right pixel.
	er = 0;
	eg = 0;
	eb = 0;

	for (x = 0; x < width - 1; x++) {
		// Get one source pixel
		bgra_pixel s = *(++src);

		// Get color components and add errors from previous pixel
		r = clip8(RED(s) + er + cera[x]);
		g = clip8(GREEN(s) + eg + cega[x]);
		b = clip8(BLUE(s) + eb + ceba[x]);

		// Find the nearest match in the palette and write it out
		appr = xpal[((r << 7) & 0x7C00) | ((g << 2) & 0x03E0) | ((b >> 3) & 0x001F)];
		*(++dest) = appr;

		// And the corresponding RGB color
		a = sysc[appr];

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

	*(++dest) = xpal[((r << 7) & 0x7C00) | ((g << 2) & 0x03E0) | ((b >> 3) & 0x001F)];

	delete[] nera;
	delete[] nega;
	delete[] neba;
	delete[] cera;
	delete[] cega;
	delete[] ceba;
}

#else

void
FSDither(BBitmap* b32, BBitmap* b8, BRect place)
{
	// b32   32bit bitmap
	// b8    8bit  bitmap
	// place The rect of the b8 bitmap into which b32 must be dithered.

	/*
	 *	Does a floyd alike dithering to a BBitmap
	 *
	 *	in_data		= input data in BGRA format
	 *	in_width	= width of the picture (== bytes per row!)
	 *	in_height	= height of the picture
	 *	dest_bitmap	= pointer to a BBitmap with COLOR_8_BIT colorspace
	 *  			  Make sure that the BBitmap is big enough...
	 *
	 *  corrections are done this way:
	 *
	 *	+-----+-----+- -
	 *  |     | 0.5 |
	 *  +-----+-----+- -
	 *  | 0.5 | 0.0 |
	 *  +-----+-----+- -
	 *  |     |     |
	 *
	 *  This allows to use only shifting and avoiding real divisions.
	 *  You will note a slight diagonal dithering, but this is no
	 *  real disadvantage.
	 *
	 */
	ulong in_width = place.Width() + 1;
	ulong in_height = place.Height() + 1;
	uint32* in_data =
		(uint32*)b32->Bits() + ulong(place.top) * b32->BytesPerRow() / 4 + ulong(place.left) - 1;
	char* delta = new char[(in_width) * 2 * 4];
	uchar* dstb = (uchar*)b8->Bits() + ulong(place.left) - 1;
	uchar* xpal = (uchar*)system_colors()->index_map;
	ulong* cpal = (ulong*)system_colors()->color_list;
	char* fcwrite = delta + in_width;
	char* cread = delta - 1;
	char* cwrite = fcwrite - 1;
	ulong xmod = b8->BytesPerRow();
	ulong smod = b32->BytesPerRow() / 4 - in_width;
	long switcher = 0;

	bzero(delta, in_width * 2 * 4);

	for (int y = place.top; y <= int(place.bottom); y++) {
		long rdlast = 0;
		long gdlast = 0;
		long bdlast = 0;
		uchar* dst = dstb + y * xmod;

		for (int x = place.left; x <= int(place.right); x++) {
			// get 1 pixel from the input buffer

			ulong p = *(++in_data);

			// add corrections from previous pixels.

			ulong cread_data = *(++cread);

#if defined(__POWERPC__)
			long b = (p >> 24) + ((*(++cread) + bdlast) / 2);
			long g = ((p >> 16) & 0xFF) + ((*(++cread) + gdlast) / 2);
			long r = ((p >> 8) & 0xFF) + ((*(++cread) + rdlast) / 2);
#else
			long b = (p & 0xFF) + ((*(++cread) + bdlast) / 2);
			long g = ((p >> 8) & 0xFF) + ((*(++cread) + gdlast) / 2);
			long r = ((p >> 16) & 0xFF) + ((*(++cread) + rdlast) / 2);
#endif

			// fix high and low values

			r = r > 255 ? 255 : (r < 0 ? 0 : r);
			g = g > 255 ? 255 : (g < 0 ? 0 : g);
			b = b > 255 ? 255 : (b < 0 ? 0 : b);

			// lets see which color we get from the BeOS palette.
			uchar q = xpal[((r << 7) & 0x7C00) | ((g << 2) & 0x03E0) | ((b >> 3) & 0x001F)];
			ulong s = cpal[q];

			// output the pixel

			*(++dst) = q;

			// save the differences to the requested color

#if defined(__POWERPC__)
			*(++cwrite) = rdlast = r - (s >> 24);
			*(++cwrite) = gdlast = g - ((s >> 16) & 0xFF);
			*(++cwrite) = bdlast = b - ((s >> 8) & 0xFF);
#else
			*(++cwrite) = rdlast = r - ((s >> 16) & 0xFF);
			*(++cwrite) = gdlast = g - ((s >> 8) & 0xFF);
			*(++cwrite) = bdlast = b - (s & 0xFF);
#endif
		}

		if (switcher) {
			cwrite = delta - 1;
			cread = fcwrite - 1;
			switcher = 0;
		} else {
			cread = delta - 1;
			cwrite = fcwrite - 1;
			switcher = 1;
		}
		in_data += smod;
	}
	delete[] delta;
}

#endif

int32
gcd(int32 p, int32 q)
{
	int32 r;
	while (r = p % q, r) {
		p = q;
		q = r;
	}
	return q;
}

extern "C" void
mmx_scale_32_h(bgra_pixel* s, bgra_pixel* d, int32 h, int32 sw, int32 dw, int32 dbpr);
extern "C" void
mmx_scale_32_v(bgra_pixel* s, bgra_pixel* d, int32 w, int32 sh, int32 dh, int32 dbpr);

int
Scale(BBitmap* src, BBitmap* srcmap, BBitmap* dest, BBitmap* destmap)
// Note:  Scale doesn't treat the alpha channel any different from R, G, and B.
//        Therefore, it doesn't need to be modified for LE vs BE issues, only the
//        nomenclature of the channels is correct on PPC
{
	BRect srcRect;
	if (src)
		srcRect = src->Bounds();
	else
		srcRect = srcmap->Bounds();
	BRect destRect;
	if (dest)
		destRect = dest->Bounds();
	else
		destRect = destmap->Bounds();

	int sw = srcRect.IntegerWidth() + 1;
	int sh = srcRect.IntegerHeight() + 1;
	int dw = destRect.IntegerWidth() + 1;
	int dh = destRect.IntegerHeight() + 1;
	int nbx = gcd(dw, sw);
	int nby = gcd(dh, sh);
	int mdx = dw / nbx;
	int mdy = dh / nby;
	int msx = sw / nbx;
	int msy = sh / nby;

	BRect horiRect = srcRect;
	double xscale = (destRect.Width() + 1) / (srcRect.Width() + 1);
	double yscale = (destRect.Height() + 1) / (srcRect.Height() + 1);
	horiRect.right = destRect.right;
	BBitmap* stmp = new BBitmap(horiRect, B_RGBA32);
	BBitmap* mtmp = new BBitmap(horiRect, B_COLOR_8_BIT);
	// printf ("xscale = %f\n", xscale);
	float f = float(mdx) / msx;
	extern bool UseMMX;
	extern int DebugLevel;
	if (xscale < 1) // Shrink horizontally
	{
		// printf ("Shrink horizontally:\n");
		if (src) // Code for the Layer
		{
			if (DebugLevel)
				printf("Scaling %d -> %d; gcd = %d\n", sw, dw, nbx);
			time_t start = clock();
			if (UseMMX &&
				sw <= 1024) // otherwise, fixed point arithmetic in MMX code will overflow.
			{
#if defined(__INTEL__)
				mmx_scale_32_h(
					(bgra_pixel*)src->Bits(), (bgra_pixel*)stmp->Bits(), sh, sw, dw,
					stmp->BytesPerRow()
				);
#endif
			} else {
				// printf ("Layer...\n");
				bgra_pixel* srcdata = (bgra_pixel*)src->Bits();
				bgra_pixel* destdata = (bgra_pixel*)stmp->Bits();
				int32 slpr = src->BytesPerRow() / 4;
				int32 dlpr = stmp->BytesPerRow() / 4;
				for (int y = 0; y < sh; y++) {
					bgra_pixel* srcline = srcdata + y * slpr;
					bgra_pixel* destline = destdata + y * dlpr;
					for (int nb = 0; nb < nbx; nb++) {
						int s = 1;
						int d = 1;
						bool stillinblock = true;
						float r = 0, g = 0, b = 0, a = 0;
						while (stillinblock) {
							while (s * mdx < d * msx) {
								bgra_pixel pixel = *srcline++;
								s++;
								r += ((pixel & 0x0000FF00) >> 8) * f;
								g += ((pixel & 0x00FF0000) >> 16) * f;
								b += (pixel >> 24) * f;
								a += (pixel & 0x000000FF) * f;
							}
							if (s * mdx != d * msx) {
								float beta = s * f - d;
								float alpha = f - beta;
								d++;
								s++;
								bgra_pixel pixel = *srcline;
								r += ((pixel & 0x0000FF00) >> 8) * alpha;
								g += ((pixel & 0x00FF0000) >> 16) * alpha;
								b += (pixel >> 24) * alpha;
								a += (pixel & 0x000000FF) * alpha;
								*(destline++) = (uint8(b) << 24) + (uint8(g) << 16) +
												(uint8(r) << 8) + (uint8(a));
								r = ((pixel & 0x0000FF00) >> 8) * beta;
								g = ((pixel & 0x00FF0000) >> 16) * beta;
								b = (pixel >> 24) * beta;
								a = (pixel & 0x000000FF) * beta;
								srcline++;
							} else {
								bgra_pixel pixel = *(srcline++);
								r += ((pixel & 0x0000FF00) >> 8) * f;
								g += ((pixel & 0x00FF0000) >> 16) * f;
								b += (pixel >> 24) * f;
								a += (pixel & 0x000000FF) * f;
								*(destline++) = (uint8(b) << 24) + (uint8(g) << 16) +
												(uint8(r) << 8) + (uint8(a));
								stillinblock = false;
							}
						}
					}
				}
			}
			time_t end = clock();
			extern int DebugLevel;
			if (DebugLevel > 1)
				printf("H Scale took %ld ms\n", end - start);
		}
		if (srcmap) // Code for Selection
		{
			// printf ("Selection...\n");
			grey_pixel* srcdata = (grey_pixel*)srcmap->Bits();
			grey_pixel* destdata = (grey_pixel*)mtmp->Bits();
			int32 sbpr = srcmap->BytesPerRow();
			int32 dbpr = mtmp->BytesPerRow();
			for (int y = 0; y < sh; y++) {
				grey_pixel* srcline = srcdata + y * sbpr;
				grey_pixel* destline = destdata + y * dbpr;
				for (int nb = 0; nb < nbx; nb++) {
					int s = 1;
					int d = 1;
					bool stillinblock = true;
					float r = 0;
					while (stillinblock) {
						while (s * mdx < d * msx) {
							r += (*srcline++) * f;
							s++;
						}
						if (s * mdx != d * msx) {
							float beta = s * f - d;
							float alpha = f - beta;
							r += alpha * *(srcline);
							*(destline++) = grey_pixel(r);
							d++;
							r = beta * *(srcline++);
							s++;
						} else {
							r += *(srcline++) * f;
							*(destline++) = uint8(r);
							stillinblock = false;
						}
					}
				}
			}
		}
	} else if (xscale > 1) // Stretch horizontally
	{
		// printf ("Stretch horizontally\n");
		if (src) // Code for Layer
		{
			if (DebugLevel)
				printf("Scaling %d -> %d; gcd = %d\n", sw, dw, nbx);
			time_t start = clock();
			if (UseMMX) {
#if defined(__INTEL__)
				mmx_scale_32_h(
					(bgra_pixel*)src->Bits(), (bgra_pixel*)stmp->Bits(), sh, sw, dw,
					stmp->BytesPerRow()
				);
#endif
			} else {
				// printf ("Layer...\n");
				bgra_pixel* srcdata = (bgra_pixel*)src->Bits();
				bgra_pixel* destdata = (bgra_pixel*)stmp->Bits();
				int32 slpr = src->BytesPerRow() / 4;
				int32 dlpr = stmp->BytesPerRow() / 4;
				for (int y = 0; y < sh; y++) {
					bgra_pixel* srcline = srcdata + y * slpr;
					bgra_pixel* destline = destdata + y * dlpr;
					for (int nb = 0; nb < nbx; nb++) {
						float s = 1;
						float d = 1;
						bool stillinblock = true;
						while (stillinblock) {
							while (d * msx < s * mdx) {
								*(destline++) = *srcline;
								d++;
							}
							if (d * msx != s * mdx) {
								float beta = d / f - s;
								float alpha = 1 - beta;
								bgra_pixel pixela = *srcline;
								bgra_pixel pixelb = *(++srcline);
								s++;
								float r = ((pixela & 0x0000FF00) >> 8) * alpha;
								float g = ((pixela & 0x00FF0000) >> 16) * alpha;
								float b = (pixela >> 24) * alpha;
								float a = (pixela & 0x000000FF) * alpha;
								r += ((pixelb & 0x0000FF00) >> 8) * beta;
								g += ((pixelb & 0x00FF0000) >> 16) * beta;
								b += (pixelb >> 24) * beta;
								a += (pixelb & 0x000000FF) * beta;
								*(destline++) = (uint8(b) << 24) + (uint8(g) << 16) +
												(uint8(r) << 8) + (uint8(a));
								d++;
							} else {
								*(destline++) = *(srcline++);
								stillinblock = false;
							}
						}
					}
				}
			}
			time_t end = clock();
			extern int DebugLevel;
			if (DebugLevel > 1)
				printf("H Scale took %ld ms\n", end - start);
		}
		if (srcmap) // Code for selection
		{
			// printf ("Selection\n");
			grey_pixel* srcdata = (grey_pixel*)srcmap->Bits();
			grey_pixel* destdata = (grey_pixel*)mtmp->Bits();
			int32 sbpr = srcmap->BytesPerRow();
			int32 dbpr = mtmp->BytesPerRow();
			for (int y = 0; y < sh; y++) {
				grey_pixel* srcline = srcdata + y * sbpr;
				grey_pixel* destline = destdata + y * dbpr;
				for (int nb = 0; nb < nbx; nb++) {
					int s = 1;
					int d = 1;
					bool stillinblock = true;
					while (stillinblock) {
						while (d * msx < s * mdx) {
							*(destline++) = *srcline;
							d++;
						}
						if (d * msx != s * mdx) {
							float beta = d / f - s;
							float alpha = 1 - beta;
							*(destline++) = uchar(alpha * *srcline + beta * *(srcline + 1));
							srcline++;
							s++;
							d++;
						} else {
							*(destline++) = *(srcline++);
							stillinblock = false;
						}
					}
				}
			}
		}
	} else // No scaling in x-dimension -> copy to temp bitmap.
	{
		// printf ("copying\n");
		if (src)
			memcpy(stmp->Bits(), src->Bits(), src->BitsLength());
		if (srcmap)
			memcpy(mtmp->Bits(), srcmap->Bits(), srcmap->BitsLength());
	}
	// printf ("yscale = %f\n", yscale);
	f = float(mdy) / msy;
	if (yscale < 1) // Shrink vertically
	{
		// printf ("Shrink vertically:\n");
		if (src) // Code for Layer
		{
			if (DebugLevel)
				printf("Scaling %d -> %d; gcd = %d\n", sh, dh, nby);
			time_t start = clock();
			if (UseMMX && sh <= 1024) {
#if defined(__INTEL__)
				mmx_scale_32_v(
					(bgra_pixel*)stmp->Bits(), (bgra_pixel*)dest->Bits(), dw, sh, dh,
					dest->BytesPerRow()
				);
#endif
			} else {
				// printf ("Layer...\n");
				bgra_pixel* srcdata = (bgra_pixel*)stmp->Bits();
				bgra_pixel* destdata = (bgra_pixel*)dest->Bits();
				int32 slpr = stmp->BytesPerRow() / 4;
				int32 dlpr = dest->BytesPerRow() / 4;
				for (int x = 0; x < dw; x++) {
					bgra_pixel* srccol = srcdata + x;
					bgra_pixel* destcol = destdata + x;
					for (int nb = 0; nb < nby; nb++) {
						int s = 1;
						int d = 1;
						bool stillinblock = true;
						float r = 0, g = 0, b = 0, a = 0;
						while (stillinblock) {
							while (s * mdy < d * msy) {
								bgra_pixel pixel = *srccol;
								srccol += slpr;
								s++;
								r += ((pixel & 0x0000FF00) >> 8) * f;
								g += ((pixel & 0x00FF0000) >> 16) * f;
								b += (pixel >> 24) * f;
								a += (pixel & 0x000000FF) * f;
							}
							if (s * mdy != d * msy) {
								float beta = s * f - d;
								float alpha = f - beta;
								bgra_pixel pixel = *srccol;
								r += ((pixel & 0x0000FF00) >> 8) * alpha;
								g += ((pixel & 0x00FF0000) >> 16) * alpha;
								b += (pixel >> 24) * alpha;
								a += (pixel & 0x000000FF) * alpha;
								*destcol = (uint8(b) << 24) + (uint8(g) << 16) + (uint8(r) << 8) +
										   (uint8(a));
								destcol += dlpr;
								d++;
								r = ((pixel & 0x0000FF00) >> 8) * beta;
								g = ((pixel & 0x00FF0000) >> 16) * beta;
								b = (pixel >> 24) * beta;
								a = (pixel & 0x000000FF) * beta;
								srccol += slpr;
								s++;
							} else {
								bgra_pixel pixel = *srccol;
								srccol += slpr;
								r += ((pixel & 0x0000FF00) >> 8) * f;
								g += ((pixel & 0x00FF0000) >> 16) * f;
								b += (pixel >> 24) * f;
								a += (pixel & 0x000000FF) * f;
								*destcol = (uint8(b) << 24) + (uint8(g) << 16) + (uint8(r) << 8) +
										   (uint8(a));
								destcol += dlpr;
								stillinblock = false;
							}
						}
					}
				}
			}
			time_t end = clock();
			extern int DebugLevel;
			if (DebugLevel > 1)
				printf("V Scale took %ld ms\n", end - start);
		}
		if (srcmap) // Code for Selection
		{
			// printf ("Selection...\n");
			grey_pixel* srcdata = (grey_pixel*)mtmp->Bits();
			grey_pixel* destdata = (grey_pixel*)destmap->Bits();
			int32 sbpr = mtmp->BytesPerRow();
			int32 dbpr = destmap->BytesPerRow();
			for (int x = 0; x < dw; x++) {
				grey_pixel* srccol = srcdata + x;
				grey_pixel* destcol = destdata + x;
				for (int nb = 0; nb < nby; nb++) {
					int s = 1;
					int d = 1;
					bool stillinblock = true;
					float r = 0;
					while (stillinblock) {
						while (s * mdy < d * msy) {
							r += *srccol * f;
							srccol += sbpr;
							s++;
						}
						if (s * mdy != d * msy) {
							float beta = s * f - d;
							float alpha = f - beta;
							r += alpha * *srccol;
							*destcol = grey_pixel(r);
							destcol += dbpr;
							d++;
							r = beta * *srccol;
							srccol += sbpr;
							s++;
						} else {
							r += (*srccol) * f;
							srccol += sbpr;
							*destcol = uchar(r);
							destcol += dbpr;
							stillinblock = false;
						}
					}
				}
			}
		}
	} else if (yscale > 1) // Stretch vertically
	{
		// printf ("Stretch vertically:\n");
		if (src) // Code for Layer
		{
			if (DebugLevel)
				printf("Scaling %d -> %d; gcd = %d\n", sh, dh, nby);
			time_t start = clock();
			if (UseMMX) {
#if defined(__INTEL__)
				mmx_scale_32_v(
					(bgra_pixel*)stmp->Bits(), (bgra_pixel*)dest->Bits(), dw, sh, dh,
					dest->BytesPerRow()
				);
#endif
			} else {
				// printf ("Layer...\n");
				bgra_pixel* srcdata = (bgra_pixel*)stmp->Bits();
				bgra_pixel* destdata = (bgra_pixel*)dest->Bits();
				int32 slpr = stmp->BytesPerRow() / 4;
				int32 dlpr = dest->BytesPerRow() / 4;
				for (int x = 0; x < dw; x++) {
					bgra_pixel* srccol = srcdata + x;
					bgra_pixel* destcol = destdata + x;
					for (int nb = 0; nb < nby; nb++) {
						int s = 1;
						int d = 1;
						bool stillinblock = true;
						while (stillinblock) {
							while (d * msy < s * mdy) {
								*destcol = *srccol;
								destcol += dlpr;
								d++;
							}
							if (d * msy != s * mdy) {
								float beta = d / f - s;
								float alpha = 1 - beta;
								bgra_pixel pixela = *srccol;
								bgra_pixel pixelb = *(srccol + slpr);
								srccol += slpr;
								s++;
								float r = ((pixela & 0x0000FF00) >> 8) * alpha;
								float g = ((pixela & 0x00FF0000) >> 16) * alpha;
								float b = (pixela >> 24) * alpha;
								float a = (pixela & 0x000000FF) * alpha;
								r += ((pixelb & 0x0000FF00) >> 8) * beta;
								g += ((pixelb & 0x00FF0000) >> 16) * beta;
								b += (pixelb >> 24) * beta;
								a += (pixelb & 0x000000FF) * beta;
								*destcol = (uint8(b) << 24) + (uint8(g) << 16) + (uint8(r) << 8) +
										   (uint8(a));
								destcol += dlpr;
								d++;
							} else {
								*destcol = *srccol;
								destcol += dlpr;
								srccol += slpr;
								stillinblock = false;
							}
						}
					}
				}
			}
			time_t end = clock();
			extern int DebugLevel;
			if (DebugLevel > 1)
				printf("V Scale took %ld ms\n", end - start);
		}
		if (srcmap) // Code for Selection
		{
			// printf ("Selection...\n");
			grey_pixel* srcdata = (grey_pixel*)mtmp->Bits();
			grey_pixel* destdata = (grey_pixel*)destmap->Bits();
			int32 sbpr = mtmp->BytesPerRow();
			int32 dbpr = destmap->BytesPerRow();
			for (int x = 0; x < dw; x++) {
				grey_pixel* srccol = srcdata + x;
				grey_pixel* destcol = destdata + x;
				for (int nb = 0; nb < nby; nb++) {
					int s = 1;
					int d = 1;
					bool stillinblock = true;
					while (stillinblock) {
						while (d * msy < s * mdy) {
							*destcol = *srccol;
							destcol += dbpr;
							d++;
						}
						if (d * msy != s * mdy) {
							float beta = d / f - s;
							float alpha = 1 - beta;
							*destcol = uchar(alpha * *srccol + beta * *(srccol + sbpr));
							destcol += dbpr;
							d++;
							srccol += sbpr;
							s++;
						} else {
							*destcol = *srccol;
							destcol += dbpr;
							srccol += sbpr;
							stillinblock = false;
						}
					}
				}
			}
		}
	} else // No scaling in y-dimension -> Just copy the bitmap to dest.
	{
		// printf ("Copying...\n");
		if (src)
			memcpy(dest->Bits(), stmp->Bits(), stmp->BitsLength());
		if (srcmap)
			memcpy(destmap->Bits(), mtmp->Bits(), mtmp->BitsLength());
	}
	delete mtmp;
	delete stmp;
	return 0;
}

void
Rotate(SBitmap* s, Layer* d, BPoint o, float rad, bool hiq)
{
	float sr = sin(rad);
	float cr = cos(rad);
	float ix, iy;
	float bw, bh;
	uint32 bpr = s->BytesPerRow() / 4;
	bw = s->Bounds().right;
	bh = s->Bounds().bottom;
	bgra_pixel* dp = (bgra_pixel*)d->Bits() - 1;
	bgra_pixel* sp = (bgra_pixel*)s->Bits();
	float ox, oy;
	ox = o.y * sr + o.x * (1 - cr);
	oy = o.y * (1 - cr) - o.x * sr;
	if (hiq && rad != 0) // do anti-aliasing
	{
		for (int i = 0; i <= bh; i++) {
			ix = ox - i * sr;
			iy = oy + i * cr;
			bgra_pixel* dl = dp + i * bpr;
			for (int j = 0; j <= bw; j++) {
				int iix = int(ix);
				int iiy = int(iy);
				float fix = (float)iix;
				float fiy = (float)iiy;

				bgra_pixel a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0, a6 = 0, a7 = 0, a8 = 0, a9 = 0;
				if (iix >= 0 && iix <= bw && iiy >= 0 && iiy <= bh)
					a5 = *(sp + iiy * bpr + iix);

				float A1 = 0, A2 = 0, A3 = 0, A4 = 0, A5 = 0, A6 = 0, A7 = 0, A8 = 0,
					  A9 = 0; // Areas
				float w, h;

				w = ix - (fix + .5);
				h = iy - (fiy + .5);
				//				float r = sqrt (w*w + h*h);

				if (w < 0 && h < 0) // TLC
				{
					if (iix - 1 >= 0 && iix - 1 <= bw && iiy - 1 >= 0 && iiy - 1 <= bh)
						a1 = *(sp + (iiy - 1) * bpr + iix - 1);
					if (iix >= 0 && iix <= bw && iiy - 1 >= 0 && iiy - 1 <= bh)
						a2 = *(sp + (iiy - 1) * bpr + iix);
					if (iix - 1 >= 0 && iix - 1 <= bw && iiy >= 0 && iiy <= bh)
						a4 = *(sp + iiy * bpr + iix - 1);
					A1 = w * h * ALPHA(a1);
					A2 = (-h - A1 / 256) * ALPHA(a2);
					A4 = (-w - A1 / 256) * ALPHA(a3);
					A5 = ALPHA(a5) - A1 - A2 - A4;
				} else if (w > 0 && h < 0) // TRC
				{
					if (iix >= 0 && iix <= bw && iiy - 1 >= 0 && iiy - 1 <= bh)
						a2 = *(sp + (iiy - 1) * bpr + iix);
					if (iix + 1 >= 0 && iix + 1 <= bw && iiy - 1 >= 0 && iiy - 1 <= bh)
						a3 = *(sp + (iiy - 1) * bpr + iix + 1);
					if (iix + 1 >= 0 && iix + 1 <= bw && iiy >= 0 && iiy <= bh)
						a6 = *(sp + iiy * bpr + iix + 1);
					A3 = -w * h * ALPHA(a3);
					A2 = (-h - A3 / 256) * ALPHA(a2);
					A6 = (w - A3 / 256) * ALPHA(a6);
					A5 = ALPHA(a5) - A3 - A2 - A6;
				} else if (w < 0 && h > 0) // BLC
				{
					if (iix - 1 >= 0 && iix - 1 <= bw && iiy >= 0 && iiy <= bh)
						a4 = *(sp + iiy * bpr + iix - 1);
					if (iix - 1 >= 0 && iix - 1 <= bw && iiy + 1 >= 0 && iiy + 1 <= bh)
						a7 = *(sp + (iiy + 1) * bpr + iix - 1);
					if (iix >= 0 && iix <= bw && iiy + 1 >= 0 && iiy + 1 <= bh)
						a8 = *(sp + (iiy + 1) * bpr + iix);
					A7 = -w * h * ALPHA(a7);
					A4 = (-w - A7 / 256) * ALPHA(a4);
					A8 = (h - A7 / 256) * ALPHA(a8);
					A5 = ALPHA(a5) - A7 - A4 - A8;
				} else if (w > 0 && h > 0) // BRC
				{
					if (iix + 1 >= 0 && iix + 1 <= bw && iiy >= 0 && iiy <= bh)
						a6 = *(sp + iiy * bpr + iix + 1);
					if (iix >= 0 && iix <= bw && iiy + 1 >= 0 && iiy + 1 <= bh)
						a8 = *(sp + (iiy + 1) * bpr + iix);
					if (iix + 1 >= 0 && iix + 1 <= bw && iiy + 1 >= 0 && iiy + 1 <= bh)
						a9 = *(sp + (iiy + 1) * bpr + iix + 1);
					A9 = w * h * ALPHA(a9);
					A6 = (w - A9 / 256) * ALPHA(a6);
					A8 = (h - A9 / 256) * ALPHA(a8);
					A5 = ALPHA(a5) - A9 - A6 - A8;
				} else // do 9-aliasing
				{
					A5 = ALPHA(a5);
				}

				uint8 red, green, blue, alpha;
				red = clipchar(float(
					(RED(a1) * A1 + RED(a2) * A2 + RED(a3) * A3 + RED(a4) * A4 + RED(a5) * A5 +
					 RED(a6) * A6 + RED(a7) * A7 + RED(a8) * A8 + RED(a9) * A9) /
					256
				));
				green = clipchar(float(
					(GREEN(a1) * A1 + GREEN(a2) * A2 + GREEN(a3) * A3 + GREEN(a4) * A4 +
					 GREEN(a5) * A5 + GREEN(a6) * A6 + GREEN(a7) * A7 + GREEN(a8) * A8 +
					 GREEN(a9) * A9) /
					256
				));
				blue = clipchar(float(
					(BLUE(a1) * A1 + BLUE(a2) * A2 + BLUE(a3) * A3 + BLUE(a4) * A4 + BLUE(a5) * A5 +
					 BLUE(a6) * A6 + BLUE(a7) * A7 + BLUE(a8) * A8 + BLUE(a9) * A9) /
					256
				));
				alpha = clipchar(float(A1 + A2 + A3 + A4 + A5 + A6 + A7 + A8 + A9));
				*(++dl) = PIXEL(red, green, blue, alpha);
				ix += cr;
				iy += sr;
			}
		}
	} else {
		for (int i = 0; i <= bh; i++) {
			ix = ox - i * sr;
			iy = oy + i * cr;
			bgra_pixel* dl = dp + i * bpr;
			for (int j = 0; j <= bw; j++) {
				int iix = int(ix + .5);
				int iiy = int(iy + .5);
				if (iix >= 0 && iix <= bw && iiy >= 0 && iiy <= bh)
					*(++dl) = *(sp + iiy * bpr + iix);
				else
					*(++dl) = 0;
				ix += cr;
				iy += sr;
			}
		}
	}
}

void
Rotate(SBitmap* s, Selection* d, BPoint o, float rad, bool hiq)
{
	float sr = sin(rad);
	float cr = cos(rad);
	float ix, iy;
	float bw, bh;
	uint32 bpr = s->BytesPerRow();
	bw = s->Bounds().right;
	bh = s->Bounds().bottom;
	grey_pixel* dp = (grey_pixel*)d->Bits() - 1;
	grey_pixel* sp = (grey_pixel*)s->Bits();
	float ox, oy;
	ox = o.y * sr + o.x * (1 - cr);
	oy = o.y * (1 - cr) - o.x * sr;
	if (hiq && rad != 0) // do anti-aliasing
	{
		for (int i = 0; i <= bh; i++) {
			ix = ox - i * sr;
			iy = oy + i * cr;
			grey_pixel* dl = dp + i * bpr;
			for (int j = 0; j <= bw; j++) {
				int iix = int(ix);
				int iiy = int(iy);
				float fix = (float)iix;
				float fiy = (float)iiy;

				grey_pixel a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0, a6 = 0, a7 = 0, a8 = 0, a9 = 0;
				if (iix >= 0 && iix <= bw && iiy >= 0 && iiy <= bh)
					a5 = *(sp + iiy * bpr + iix);

				float A1 = 0, A2 = 0, A3 = 0, A4 = 0, A5 = 0, A6 = 0, A7 = 0, A8 = 0,
					  A9 = 0; // Areas
				float w, h;

				w = ix - (fix + .5);
				h = iy - (fiy + .5);
				//				float r = sqrt (w*w + h*h);

				if (w < 0 && h < 0) // TLC
				{
					if (iix - 1 >= 0 && iix - 1 <= bw && iiy - 1 >= 0 && iiy - 1 <= bh)
						a1 = *(sp + (iiy - 1) * bpr + iix - 1);
					if (iix >= 0 && iix <= bw && iiy - 1 >= 0 && iiy - 1 <= bh)
						a2 = *(sp + (iiy - 1) * bpr + iix);
					if (iix - 1 >= 0 && iix - 1 <= bw && iiy >= 0 && iiy <= bh)
						a4 = *(sp + iiy * bpr + iix - 1);
					A1 = w * h * a1;
					A2 = (-h - A1 / 256) * a2;
					A4 = (-w - A1 / 256) * a4;
					A5 = a5 - A1 - A2 - A4;
				} else if (w > 0 && h < 0) // TRC
				{
					if (iix >= 0 && iix <= bw && iiy - 1 >= 0 && iiy - 1 <= bh)
						a2 = *(sp + (iiy - 1) * bpr + iix);
					if (iix + 1 >= 0 && iix + 1 <= bw && iiy - 1 >= 0 && iiy - 1 <= bh)
						a3 = *(sp + (iiy - 1) * bpr + iix + 1);
					if (iix + 1 >= 0 && iix + 1 <= bw && iiy >= 0 && iiy <= bh)
						a6 = *(sp + iiy * bpr + iix + 1);
					A3 = -w * h * a3;
					A2 = (-h - A3 / 256) * a2;
					A6 = (w - A3 / 256) * a6;
					A5 = a5 - A3 - A2 - A6;
				} else if (w < 0 && h > 0) // BLC
				{
					if (iix - 1 >= 0 && iix - 1 <= bw && iiy >= 0 && iiy <= bh)
						a4 = *(sp + iiy * bpr + iix - 1);
					if (iix - 1 >= 0 && iix - 1 <= bw && iiy + 1 >= 0 && iiy + 1 <= bh)
						a7 = *(sp + (iiy + 1) * bpr + iix - 1);
					if (iix >= 0 && iix <= bw && iiy + 1 >= 0 && iiy + 1 <= bh)
						a8 = *(sp + (iiy + 1) * bpr + iix);
					A7 = -w * h * a7;
					A4 = (-w - A7 / 256) * a4;
					A8 = (h - A7 / 256) * a8;
					A5 = a5 - A7 - A4 - A8;
				} else if (w > 0 && h > 0) // BRC
				{
					if (iix + 1 >= 0 && iix + 1 <= bw && iiy >= 0 && iiy <= bh)
						a6 = *(sp + iiy * bpr + iix + 1);
					if (iix >= 0 && iix <= bw && iiy + 1 >= 0 && iiy + 1 <= bh)
						a8 = *(sp + (iiy + 1) * bpr + iix);
					if (iix + 1 >= 0 && iix + 1 <= bw && iiy + 1 >= 0 && iiy + 1 <= bh)
						a9 = *(sp + (iiy + 1) * bpr + iix + 1);
					A9 = w * h * a9;
					A6 = (w - A9 / 256) * a6;
					A8 = (h - A9 / 256) * a8;
					A5 = a5 - A9 - A6 - A8;
				} else // do 9-aliasing
				{
					A5 = a5;
				}

				uint8 alpha = clipchar(float(A1 + A2 + A3 + A4 + A5 + A6 + A7 + A8 + A9));
				*(++dl) = alpha;
				ix += cr;
				iy += sr;
			}
		}
	} else {
		for (int i = 0; i <= bh; i++) {
			ix = ox - i * sr;
			iy = oy + i * cr;
			grey_pixel* dl = dp + i * bpr;
			for (int j = 0; j <= bw; j++) {
				int iix = int(ix + .5);
				int iiy = int(iy + .5);
				if (iix >= 0 && iix <= bw && iiy >= 0 && iiy <= bh)
					*(++dl) = *(sp + iiy * bpr + iix);
				else
					*(++dl) = 0;
				ix += cr;
				iy += sr;
			}
		}
	}
}

void
SBitmapToLayer(SBitmap* s, Layer* d, BPoint& p)
{
	ASSERT(s->BytesPerPixel() == 4);

	if (p == B_ORIGIN) {
		memcpy(d->Bits(), s->Bits(), s->BitsLength());
		return;
	}

	if (p.x > s->Bounds().right || p.x < -s->Bounds().right)
		return;
	if (p.y > s->Bounds().bottom || p.y < -s->Bounds().bottom)
		return;

	if (p.x >= 0) {
		if (p.y >= 0) {
			int32 rt = int32(p.y);
			int32 rb = int32(s->Bounds().bottom);
			int32 rl = int32(p.x);
			int32 rr = int32(s->Bounds().right);

			int32 bpr = 4 * (rr - rl + 1);
			for (int32 y = rt; y <= rb; y++) {
				uint32* sp = (uint32*)s->Bits() + (y - rt) * s->BytesPerRow() / 4;
				uint32* dp = (uint32*)d->Bits() + y * d->BytesPerRow() / 4 + rl;
				memcpy(dp, sp, bpr);
			}
		} else {
			int32 rt = -int32(p.y);
			int32 rb = int32(s->Bounds().bottom);
			int32 rl = int32(p.x);
			int32 rr = int32(s->Bounds().right);

			int32 bpr = 4 * (rr - rl + 1);
			for (int32 y = rt; y <= rb; y++) {
				uint32* sp = (uint32*)s->Bits() + y * s->BytesPerRow() / 4;
				uint32* dp = (uint32*)d->Bits() + (y - rt) * d->BytesPerRow() / 4 + rl;
				memcpy(dp, sp, bpr);
			}
		}
	} else {
		if (p.y >= 0) {
			int32 rt = int32(p.y);
			int32 rb = int32(s->Bounds().bottom);
			int32 rl = -int32(p.x);
			int32 rr = int32(s->Bounds().right);

			int32 bpr = 4 * (rr - rl + 1);
			for (int32 y = rt; y <= rb; y++) {
				uint32* sp = (uint32*)s->Bits() + (y - rt) * s->BytesPerRow() / 4 + rl;
				uint32* dp = (uint32*)d->Bits() + y * d->BytesPerRow() / 4;
				memcpy(dp, sp, bpr);
			}
		} else {
			int32 rt = -int32(p.y);
			int32 rb = int32(s->Bounds().bottom);
			int32 rl = -int32(p.x);
			int32 rr = int32(s->Bounds().right);

			int32 bpr = 4 * (rr - rl + 1);
			for (int32 y = rt; y <= rb; y++) {
				uint32* sp = (uint32*)s->Bits() + y * s->BytesPerRow() / 4 + rl;
				uint32* dp = (uint32*)d->Bits() + (y - rt) * d->BytesPerRow() / 4;
				memcpy(dp, sp, bpr);
			}
		}
	}
}

void
SBitmapToLayer(SBitmap* s, Layer* d, BRect& r)
{
	ASSERT(s->BytesPerPixel() == 4);

	if (s->Bounds() == d->Bounds() && d->Bounds() == r) {
		memcpy(d->Bits(), s->Bits(), s->BitsLength());
		return;
	}

	int32 rt = max_c(0, int32(r.top));
	int32 rb = min_c(int32(d->Bounds().bottom), max_c(int32(s->Bounds().bottom), int32(r.bottom)));
	int32 rl = max_c(0, int32(r.left));
	int32 rr = min_c(int32(d->Bounds().right), max_c(int32(s->Bounds().right), int32(r.right)));

	int32 bpr = 4 * (rr - rl + 1);
	if (bpr <= 0)
		return;

	for (int32 y = rt; y <= rb; y++) {
		uint32* sp = (uint32*)s->Bits() + y * s->BytesPerRow() / 4 + rl;
		uint32* dp = (uint32*)d->Bits() + y * d->BytesPerRow() / 4 + rl;
		memcpy(dp, sp, bpr);
	}
}

void
SBitmapToSelection(SBitmap* s, Selection* d, BPoint& p)
{
	ASSERT(s->BytesPerPixel() == 1);

	if (p == B_ORIGIN) {
		memcpy(d->Bits(), s->Bits(), s->BitsLength());
		return;
	}

	if (p.x > s->Bounds().right || p.x < -s->Bounds().right)
		return;
	if (p.y > s->Bounds().bottom || p.y < -s->Bounds().bottom)
		return;

	if (p.x >= 0) {
		if (p.y >= 0) {
			int32 rt = int32(p.y);
			int32 rb = int32(s->Bounds().bottom);
			int32 rl = int32(p.x);
			int32 rr = int32(s->Bounds().right);

			int32 bpr = rr - rl + 1;
			for (int32 y = rt; y <= rb; y++) {
				uint8* sp = (uint8*)s->Bits() + (y - rt) * s->BytesPerRow();
				uint8* dp = (uint8*)d->Bits() + y * d->BytesPerRow() + rl;
				memcpy(dp, sp, bpr);
			}
		} else {
			int32 rt = -int32(p.y);
			int32 rb = int32(s->Bounds().bottom);
			int32 rl = int32(p.x);
			int32 rr = int32(s->Bounds().right);

			int32 bpr = rr - rl + 1;
			for (int32 y = rt; y <= rb; y++) {
				uint8* sp = (uint8*)s->Bits() + y * s->BytesPerRow();
				uint8* dp = (uint8*)d->Bits() + (y - rt) * d->BytesPerRow() + rl;
				memcpy(dp, sp, bpr);
			}
		}
	} else {
		if (p.y >= 0) {
			int32 rt = int32(p.y);
			int32 rb = int32(s->Bounds().bottom);
			int32 rl = -int32(p.x);
			int32 rr = int32(s->Bounds().right);

			int32 bpr = rr - rl + 1;
			for (int32 y = rt; y <= rb; y++) {
				uint8* sp = (uint8*)s->Bits() + (y - rt) * s->BytesPerRow() + rl;
				uint8* dp = (uint8*)d->Bits() + y * d->BytesPerRow();
				memcpy(dp, sp, bpr);
			}
		} else {
			int32 rt = -int32(p.y);
			int32 rb = int32(s->Bounds().bottom);
			int32 rl = -int32(p.x);
			int32 rr = int32(s->Bounds().right);

			int32 bpr = rr - rl + 1;
			for (int32 y = rt; y <= rb; y++) {
				uint8* sp = (uint8*)s->Bits() + y * s->BytesPerRow() + rl;
				uint8* dp = (uint8*)d->Bits() + (y - rt) * d->BytesPerRow();
				memcpy(dp, sp, bpr);
			}
		}
	}
}

void
SBitmapToSelection(SBitmap* s, Selection* d, BRect& r)
{
	ASSERT(s->BytesPerPixel() == 1);

	if (s->Bounds() == d->Bounds() && d->Bounds() == r) {
		memcpy(d->Bits(), s->Bits(), s->BitsLength());
		return;
	}

	int32 rt = max_c(0, int32(r.top));
	int32 rb = min_c(int32(d->Bounds().bottom), max_c(int32(s->Bounds().bottom), int32(r.bottom)));
	int32 rl = max_c(0, int32(r.left));
	int32 rr = min_c(int32(d->Bounds().right), max_c(int32(s->Bounds().right), int32(r.right)));

	int32 bpr = (rr - rl + 1);
	if (bpr <= 0)
		return;

	for (int32 y = rt; y <= rb; y++) {
		uint8* sp = (uint8*)s->Bits() + y * s->BytesPerRow() + rl;
		uint8* dp = (uint8*)d->Bits() + y * d->BytesPerRow() + rl;
		memcpy(dp, sp, bpr);
	}
}


#if defined(__MWCC__)
#pragma peephole reset
#endif
