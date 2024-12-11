#include "ColorMenuButton.h"
#include <Application.h>
#include <File.h>
#include <Path.h>
#include <Screen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BecassoAddOn.h"
#include "BitmapStuff.h"
#include "ColorMenu.h"
#include "ColorWindow.h"
#include "Colors.h"
#include "PatternMenuButton.h"	// I'm not too happy about this.
#include "hsv.h"

#define OPEN_RAD 4

ColorMenuButton::ColorMenuButton(const char* ident, BRect frame, const char* name)
	: BView(frame, name, B_FOLLOW_LEFT, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	index = 0;
	strcpy(_name, name);
	menu = new ColorMenu(ident, this, C_H_NUM, C_V_NUM, C_SIZE);

	rgb_tolerance.red = 10;
	rgb_tolerance.green = 10;
	rgb_tolerance.blue = 10;
	rgb_tolerance.alpha = 10;
	tolerance = 18;
	editorshowing = false;
	// Note: No way to react to bit depth changes this way yet...
	frame.OffsetTo(B_ORIGIN);
	{
		BScreen screen;
		button =
			new BBitmap(frame, (screen.ColorSpace() == B_COLOR_8_BIT) ? B_COLOR_8_BIT : B_RGBA32);
		// This is because we can't store 16 bit bitmaps, and 32 bit bitmaps don't look too bad on
		// 16 bit screens.
	}
	fW = frame.IntegerWidth();
	fH = frame.IntegerWidth();
	alpha = 255;  // Note: alpha = opacity!
	get_click_speed(&dcspeed);
	click = 1;
	lock = new BLocker();
	MenuWinOnScreen = false;
	menu->setParent(this);
	//	printf ("CMB: ctor done\n");
}

ColorMenuButton::~ColorMenuButton()
{
	delete menu;
	delete lock;
}

void
ColorMenuButton::editorSaysBye()
{
	editorshowing = false;
}

void
ColorMenuButton::MouseDown(BPoint point)
{
	Window()->Lock();
	uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");
	// uint32 clicks = Window()->CurrentMessage()->FindInt32 ("clicks");
	BMenuItem* mselected;
	point = BPoint(0, 0);
	ConvertToScreen(&point);
	if (click != 2 && buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY)) {
		BPoint bp, pbp;
		uint32 bt;
		GetMouse(&pbp, &bt, true);
		bigtime_t start = system_time();
		while (system_time() - start < dcspeed) {
			snooze(20000);
			GetMouse(&bp, &bt, true);
			if (!bt && click != 2) {
				click = 0;
			}
			if (bt && !click) {
				click = 2;
			}
			if (bp != pbp)
				break;
		}
		if (click != 2) {
			BRect openRect = BRect(
				point.x - OPEN_RAD, point.y - OPEN_RAD, point.x + OPEN_RAD, point.y + OPEN_RAD);
			menu->Show();
			if ((mselected = menu->Track(true, &openRect)) != NULL) {
				index = menu->IndexOf(mselected);
				menu->Mark(index);
				if (MenuWinOnScreen)
					menu->InvalidateWindow();
				extern PatternMenuButton* pat;
				if (pat && pat->Window()) {
					pat->Window()->Lock();
					pat->Invalidate();
					pat->lock->Lock();
					if (pat->MenuWinOnScreen)
						pat->getMenu()->InvalidateWindow();
					pat->lock->Unlock();
					pat->Window()->Unlock();
					be_app->PostMessage('clrX');
				}
			}
			menu->Hide();
			if (editorshowing) {
				rgb_color c = color();
				BMessage* msg = new BMessage('SetC');
				msg->AddInt32("color", c.red);
				msg->AddInt32("color", c.green);
				msg->AddInt32("color", c.blue);
				editor->PostMessage(msg);
				delete msg;
			}
			click = 1;
		}
	}
	if (click == 2 || buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY) {
		click = 1;
		if (editorshowing) {
			rgb_color c = color();
			BMessage* msg = new BMessage('SetC');
			msg->AddInt32("color", c.red);
			msg->AddInt32("color", c.green);
			msg->AddInt32("color", c.blue);
			editor->PostMessage(msg);
			delete msg;
		} else {
			ShowEditor();
		}
	}
	Invalidate();
	Window()->Unlock();
}

void
ColorMenuButton::ShowEditor()
{
	editor = new ColorWindow(BRect(100, 180, 300, 300), _name, this);
	editorshowing = true;
	editor->Show();
}

void
ColorMenuButton::MouseMoved(BPoint point, uint32 transit, const BMessage* msg)
{
	point = point;
	msg = msg;
	if (transit == B_ENTERED_VIEW) {
		BMessage* hlp = new BMessage('chlp');
		hlp->AddString("View", _name);
		Window()->PostMessage(hlp);
		delete hlp;
	} else if (transit == B_EXITED_VIEW) {
		BMessage* hlp = new BMessage('chlp');
		hlp->AddString("View", "");
		Window()->PostMessage(hlp);
		delete hlp;
	}
}

void
ColorMenuButton::Draw(BRect update)
{
	// printf ("CMB::Draw\n");
	BBitmap* b32 = new BBitmap(button->Bounds(), B_RGBA32, true);
	BView* v = new BView(button->Bounds(), "tmp colorbutton view", 0, 0);
	b32->Lock();
	b32->AddChild(v);
	v->SetHighColor(color());
	update = Bounds();
	v->FillRect(update);
	b32->RemoveChild(v);
	delete v;

	switch (button->ColorSpace()) {
		case B_COLOR_8_BIT:
		{
			FSDither(b32, button, update);
			delete b32;
			break;
		}
		default:
		{
			button->Lock();
			delete button;
			button = b32;
			break;
		}
	}

	update.right--;
	DrawBitmap(button, B_ORIGIN);
	SetHighColor(Grey8);
	SetDrawingMode(B_OP_ADD);
	StrokeLine(update.LeftBottom(), update.LeftTop());
	StrokeLine(update.RightTop());
	SetDrawingMode(B_OP_SUBTRACT);
	StrokeLine(update.RightBottom());
	StrokeLine(update.LeftBottom());
}

bool
ColorMenuButton::approx(rgb_color a, rgb_color b)
// Note: Doesn't check for alpha.
{
	return ((abs(int(a.red) - b.red) <= rgb_tolerance.red) &&
			(abs(int(a.green) - b.green) <= rgb_tolerance.green) &&
			(abs(int(a.blue) - b.blue) <= rgb_tolerance.blue));
}

float
ColorMenuButton::cdiff(rgb_color a)
{
	return (diff(a, color()));
}

rgb_color*
ColorMenuButton::palette()
// Note: Caller should free the palette!
{
	rgb_color* p = new rgb_color[numColors()];
	for (int i = 0; i < numColors(); i++) {
		p[i] = ColorForIndex(i);
	}
	return p;
}

#define CMB_R 32
#define CMB_G 64
#define CMB_B 32

#define NEW_WEIGHTS 1

#if defined(NEW_WEIGHTS)
#define RED_WEIGHT 0.2125
#define GREEN_WEIGHT 0.7154
#define BLUE_WEIGHT 0.0721
#elif defined(USE_WEIGHTS)
#define RED_WEIGHT 0.299
#define GREEN_WEIGHT 0.587
#define BLUE_WEIGHT 0.114
#else
#define RED_WEIGHT 2
#define GREEN_WEIGHT 3
#define BLUE_WEIGHT 1
#endif

typedef struct {
	uint16 rmin, rmax;
	uint16 gmin, gmax;
	uint16 bmin, bmax;
	uint32 volume;
	uint32 num_colors;
} CMB_box;

void trim_box(CMB_box* b, uint32* histogram);
CMB_box* biggest_population(CMB_box* b, int boxindex);
CMB_box* biggest_volume(CMB_box* b, int boxindex);

void
trim_box(CMB_box* box, uint32* histogram)
{
	uint32 r, g, b;
	if (box->rmax > box->rmin)
		for (r = box->rmin; r <= box->rmax; r++)
			for (g = box->gmin; g <= box->gmax; g++)
				for (b = box->bmin; b <= box->bmax; b++)
					if (histogram[r * (CMB_G * CMB_B) + g * CMB_B + b] != 0) {
						box->rmin = r;
						goto have_rmin;
					}
have_rmin:
	if (box->rmax > box->rmin)
		for (r = box->rmax; r >= box->rmin; r--)
			for (g = box->gmin; g <= box->gmax; g++)
				for (b = box->bmin; b <= box->bmax; b++)
					if (histogram[r * (CMB_G * CMB_B) + g * CMB_B + b] != 0) {
						box->rmax = r;
						goto have_rmax;
					}
have_rmax:
	if (box->gmax > box->gmin)
		for (g = box->gmin; g <= box->gmax; g++)
			for (r = box->rmin; r <= box->rmax; r++)
				for (b = box->bmin; b <= box->bmax; b++)
					if (histogram[r * (CMB_G * CMB_B) + g * CMB_B + b] != 0) {
						box->gmin = g;
						goto have_gmin;
					}
have_gmin:
	if (box->gmax > box->gmin)
		for (g = box->gmax; g >= box->gmin; g--)
			for (r = box->rmin; r <= box->rmax; r++)
				for (b = box->bmin; b <= box->bmax; b++)
					if (histogram[r * (CMB_G * CMB_B) + g * CMB_B + b] != 0) {
						box->gmax = g;
						goto have_gmax;
					}
have_gmax:
	if (box->bmax > box->bmin)
		for (b = box->bmin; b <= box->bmax; b++)
			for (g = box->gmin; g <= box->gmax; g++)
				for (r = box->rmin; r <= box->rmax; r++)
					if (histogram[r * (CMB_G * CMB_B) + g * CMB_B + b] != 0) {
						box->bmin = b;
						goto have_bmin;
					}
have_bmin:
	if (box->bmax > box->bmin)
		for (b = box->bmax; b >= box->bmin; b--)
			for (g = box->gmin; g <= box->gmax; g++)
				for (r = box->rmin; r <= box->rmax; r++)
					if (histogram[r * (CMB_G * CMB_B) + g * CMB_B + b] != 0) {
						box->bmax = b;
						goto have_bmax;
					}
have_bmax:
	// Count colors
	int32 count = 0;
	for (b = box->bmin; b <= box->bmax; b++)
		for (g = box->gmin; g <= box->gmax; g++)
			for (r = box->rmin; r <= box->rmax; r++)
				if (histogram[r * (CMB_G * CMB_B) + g * CMB_B + b] != 0)
					count++;

	box->num_colors = count;
	// Use 2-norm; scaled distances (i.e. green weighs in more)
	// Note: Maybe the factors should be squared too, but IMHO this puts
	//       too much emphasis on green.
	box->volume = (uint32)(RED_WEIGHT * (box->rmax - box->rmin) * (box->rmax - box->rmin) +
						   GREEN_WEIGHT * (box->gmax - box->gmin) * (box->gmax - box->gmin) +
						   BLUE_WEIGHT * (box->bmax - box->bmin) * (box->bmax - box->bmin));
}

CMB_box*
biggest_population(CMB_box* b, int boxindex)
{
	int32 maxv = 0;
	int i;

	CMB_box* thebiggest = NULL;
	for (i = 0; i < boxindex; i++)
		if (b[i].volume > maxv) {
			thebiggest = &b[i];
			maxv = b[i].volume;
		}
	return thebiggest;
}

CMB_box*
biggest_volume(CMB_box* b, int boxindex)
{
	int32 maxv = 0;
	int i;

	CMB_box* thebiggest = NULL;
	for (i = 0; i < boxindex; i++)
		if (b[i].volume > maxv) {
			thebiggest = &b[i];
			maxv = b[i].volume;
		}
	return thebiggest;
}

void
ColorMenuButton::extractPalette(Layer* l, int max_col, bool clobber)
{
	if (max_col > numColors())
		max_col = numColors();

	// uint32 histogram[CMB_R][CMB_G][CMB_B];
	uint32* histogram = new uint32[CMB_R * CMB_G * CMB_B];
	memset(histogram, 0, CMB_R * CMB_G * CMB_B * 4);  // otherwise strange errors with MALLOC_DEBUG!

	uint32 dtable[C_V_NUM * C_H_NUM];
	int cindex = 0;
	int b16count = 0;

	bool needtoquantize = false;
	bgra_pixel* s = (bgra_pixel*)l->Bits() - 1;
	// Iterate over all the pixels
	for (uint32 p = (l->Bounds().IntegerWidth() + 1) * (l->Bounds().IntegerHeight() + 1); p > 0;
		 p--) {
		bgra_pixel pixel = *(++s);
		if (!needtoquantize)  // Maybe we can get away with the existing colors...
		{
			int j = 0;
			while (pixel != dtable[j] && j < cindex)
				j++;
			if (j == cindex)  // New color
			{
				dtable[cindex++] = pixel;
				if (cindex >= max_col)
					needtoquantize = true;	// Too many colors.
			}
		}

		uint8 r, g, b;
		r = ((RED(pixel)) >> 3) & 0x1F;
		g = ((GREEN(pixel)) >> 2) & 0x3F;
		b = ((BLUE(pixel)) >> 3) & 0x1F;
		if (!histogram[r * (CMB_G * CMB_B) + g * CMB_B + b])
			b16count++;
		histogram[r * (CMB_G * CMB_B) + g * CMB_B + b]++;
	}

	extern int DebugLevel;
	if (DebugLevel)
		fprintf(stderr, "Found at least %d colors\n", cindex);

	if (needtoquantize && b16count <= max_col) {
		// Special case: There are too many colors, but during
		// quantization to 16bit there were too few again.
		// This can happen in images with very subtle color differences
		// (e.g. with shallow gradients).
		if (DebugLevel)
			fprintf(stderr, "Shallow palette: %d entries\n", b16count);

		// What we'll do is replace b16count colors of the original palette,
		// and leave the rest in place.
		uint8 r, g, b;
		for (g = 0; g < CMB_G; g++)
			for (r = 0; r < CMB_R; r++)
				for (b = 0; b < CMB_B; b++)
					if (histogram[r * (CMB_G * CMB_B) + g * CMB_B + b] && b16count)
						dtable[--b16count] = PIXEL((r << 3), (g << 2), (b << 4), 255);

		needtoquantize = false;
	}

	if (needtoquantize) {
		if (DebugLevel)
			fprintf(stderr, "Quantizing: %d unique 16 bit colors\n", b16count);
		CMB_box* boxes = new CMB_box[max_col];
		int32 boxindex = 1;

		// Set up the first box
		boxes[0].rmin = boxes[0].gmin = boxes[0].bmin = 0;
		boxes[0].rmax = CMB_R - 1;
		boxes[0].gmax = CMB_G - 1;
		boxes[0].bmax = CMB_B - 1;
		trim_box(&boxes[0], histogram);

		for (int j = 1; j < max_col; j++) {
			// Find the largest box and split it
			CMB_box* biggest;
			if (j * 2 <= max_col)
				biggest = biggest_population(boxes, boxindex);
			else
				biggest = biggest_volume(boxes, boxindex);

			if (!biggest)
				break;	// No more splittable boxes left...
						// (This shouldn't happen, because we test for #colors <= max_col first)

			if (DebugLevel > 2)
				printf("Biggest: %ld colors R[%d - %d] G[%d - %d] B[%d - %d]\n",
					biggest->num_colors, biggest->rmin, biggest->rmax, biggest->gmin, biggest->gmax,
					biggest->bmin, biggest->bmax);

			CMB_box* n = &boxes[boxindex++];
			n->rmin = biggest->rmin;
			n->rmax = biggest->rmax;
			n->gmin = biggest->gmin;
			n->gmax = biggest->gmax;
			n->bmin = biggest->bmin;
			n->bmax = biggest->bmax;

			// Decide how to split the box
			int rd = n->rmax - n->rmin;
			int gd = n->gmax - n->gmin;
			int bd = n->bmax - n->bmin;
			float sr = rd * RED_WEIGHT;
			float sg = gd * GREEN_WEIGHT;
			float sb = gd * BLUE_WEIGHT;

			if (sr > sg) {
				if (sr > sb)  // red wins
				{
					if (DebugLevel > 2)
						printf("Split on red\n");
					n->rmin = n->rmax - rd / 2;
					biggest->rmax = n->rmin - 1;
				} else	// blue wins
				{
					if (DebugLevel > 2)
						printf("Split on blue\n");
					n->bmin = n->bmax - bd / 2;
					biggest->bmax = n->bmin - 1;
				}
			} else {
				if (sg > sb)  // green wins
				{
					if (DebugLevel > 2)
						printf("Split on green\n");
					n->gmin = n->gmax - gd / 2;
					biggest->gmax = n->gmin - 1;
				} else	// blue wins
				{
					if (DebugLevel > 2)
						printf("Split on blue\n");
					n->bmin = n->bmax - bd / 2;
					biggest->bmax = n->bmin - 1;
				}
			}
			trim_box(n, histogram);
			trim_box(biggest, histogram);
			if (DebugLevel > 2)
				printf("  After split: %ld colors R[%d - %d] G[%d - %d] B[%d - %d]\n",
					biggest->num_colors, biggest->rmin, biggest->rmax, biggest->gmin, biggest->gmax,
					biggest->bmin, biggest->bmax);
			if (DebugLevel > 2)
				printf("  And:         %ld colors R[%d - %d] G[%d - %d] B[%d - %d]\n",
					n->num_colors, n->rmin, n->rmax, n->gmin, n->gmax, n->bmin, n->bmax);
		}

		if (DebugLevel)
			fprintf(stderr, "Found %ld boxes.  Calculating averages...\n", boxindex);

		// Next, iterate over all the boxes and fill the color table with
		// reasonable averages for the box colors.
		for (int j = 0; j < max_col; j++) {
			if (DebugLevel > 2)
				printf("Box %i: %ld colors R[%d - %d] G[%d - %d] B[%d - %d]\n", j,
					boxes[j].num_colors, boxes[j].rmin, boxes[j].rmax, boxes[j].gmin, boxes[j].gmax,
					boxes[j].bmin, boxes[j].bmax);
			uint32 fr, fg, fb;
			fr = fg = fb = 0;
			uint32 tot = 0;
			uint8 r, g, b;
			for (r = boxes[j].rmin; r <= boxes[j].rmax; r++)
				for (b = boxes[j].bmin; b <= boxes[j].bmax; b++)
					for (g = boxes[j].gmin; g <= boxes[j].gmax; g++) {
						uint32 w = histogram[r * (CMB_G * CMB_B) + g * CMB_B + b];
						tot += w;
						fr += w * (r << 3);
						fg += w * (g << 2);
						fb += w * (b << 3);
					}
			r = g = b = 0;
			if (tot != 0)  // This shouldn't fail, actually.
			{
				r = fr / tot;
				g = fg / tot;
				b = fb / tot;
			}

			dtable[j] = PIXEL(r, g, b, 255);
		}

		if (DebugLevel)
			fprintf(stderr, "Done calculating averages.\n");
		delete[] boxes;
	}
	delete[] histogram;

	for (int i = 0; i < cindex; i++) {
		menu->ItemAt(i)->setColor(bgra2rgb(dtable[i]));
	}
	if (clobber) {
		for (int i = cindex; i < numColors(); i++) {
			menu->ItemAt(i)->setColor(menu->ItemAt(cindex - 1)->getColor());
		}
	}

	Window()->Lock();
	Invalidate();
	extern PatternMenuButton* pat;
	pat->Invalidate();
	Window()->Unlock();
	if (MenuWinOnScreen)
		menu->InvalidateWindow();
}

int
ColorMenuButton::getIndexOfClosest(rgb_color c)
{
	float minc = 255;  // Worst case...
	float cd;
	int i = 0;
	int ind = 0;
	while (i < MAX_COLORS - 1) {
		if ((cd = diff(c, menu->ItemAt(i)->getColor())) < minc) {
			minc = cd;
			ind = i;
		}
		i++;
	}
	return (ind);
}

rgb_color
ColorMenuButton::getClosest(rgb_color c)
{
	return (menu->ItemAt(getIndexOfClosest(c))->getColor());
}

void
ColorMenuButton::set(int _index, bool via_win)
{
	index = _index;
	menu->Mark(index);
	if (Window()) {
		Window()->Lock();
		Invalidate();
		Window()->Unlock();
	}
	if (MenuWinOnScreen && !via_win)
		menu->InvalidateWindow();

	extern PatternMenuButton* pat;
	if (pat && pat->Window()) {
		pat->Window()->Lock();
		pat->Invalidate();
		pat->lock->Lock();
		if (pat->MenuWinOnScreen)
			pat->getMenu()->InvalidateWindow();
		pat->lock->Unlock();
		pat->Window()->Unlock();
	}
	if (editorshowing) {
		rgb_color c = color();
		BMessage* msg = new BMessage('SetC');
		msg->AddInt32("color", c.red);
		msg->AddInt32("color", c.green);
		msg->AddInt32("color", c.blue);
		msg->AddInt32("alpha", c.alpha);
		editor->PostMessage(msg);
		delete msg;
	}
	be_app->PostMessage('clrX');
}

bool
ColorMenuButton::set(rgb_color c, bool anyway)
// Returns true if no match was found within the tolerance.
// If anyway == true (false by default), the best match is set to c,
// regardless of the tolerance.
{
	rgb_color now = color();
	if (c.red == now.red && c.green == now.green && c.blue == now.blue && c.alpha == alpha)
		return true;

	alpha = c.alpha;
	bool ret = false;
	float minc = 255;
	float cd;
	int i = 0;
	int ind = 0;
	while (i < MAX_COLORS - 1) {
		if ((cd = diff(c, menu->ItemAt(i)->getColor())) < minc) {
			minc = cd;
			ind = i;
		}
		i++;
	}
	if (minc > tolerance)
		ret = true;
	if (!anyway)
		ind = selected();
	menu->ItemAt(ind)->setColor(c);
	menu->Mark(ind);
	Window()->Lock();
	Invalidate();
	Window()->Unlock();
	extern PatternMenuButton* pat;
	if (pat && pat->Window()) {
		pat->Window()->Lock();
		pat->Invalidate();
		pat->lock->Lock();
		if (pat->MenuWinOnScreen)
			pat->getMenu()->InvalidateWindow();
		pat->lock->Unlock();
		pat->Window()->Unlock();
	}
	if (MenuWinOnScreen)
		menu->InvalidateWindow();
	if (editorshowing) {
		BMessage* msg = new BMessage('SetC');
		msg->AddInt32("color", c.red);
		msg->AddInt32("color", c.green);
		msg->AddInt32("color", c.blue);
		msg->AddInt32("alpha", c.alpha);
		editor->PostMessage(msg);
		delete msg;
	}
	be_app->PostMessage('clrX');
	return (ret);
}

void
ColorMenuButton::setTolerance(rgb_color _t)
{
	rgb_tolerance = _t;
}

void
ColorMenuButton::setTolerance(float _t)
{
	tolerance = _t;
}

void
ColorMenuButton::setAlpha(uchar a)
{
	alpha = a;	// Note: The Color Editor gives us opacity (of course!)
}

float
ColorMenuButton::getTolerance()
{
	return tolerance;
}

int32
ColorMenuButton::selected()
{
	return (index);
}

rgb_color
ColorMenuButton::color()
{
	rgb_color c = menu->FindMarked()->getColor();
	c.alpha = alpha;
	return (c);
}

rgb_color
ColorMenuButton::ColorForIndex(int ind)
{
	rgb_color c = menu->ItemAt(ind)->getColor();
	c.alpha = alpha;
	return (c);
}

void
ColorMenuButton::Save(BEntry entry)
{
	FILE* fp;
	char line[81];
	BFile file = BFile(&entry, B_CREATE_FILE | B_READ_WRITE | B_ERASE_FILE);
	BPath path;
	entry.GetPath(&path);
	BNode node = BNode(&entry);
	node.WriteAttr("BEOS:TYPE", B_STRING_TYPE, 0, "application/x-sum-becasso.palette\0", 34);
	fp = fopen(path.Path(), "w");
	fputs("# Becasso Palette file\n", fp);
	fputs("# R G B triplets, separated by whitespace, ranging from 0 - 255.\n", fp);
	fputs("# Lines starting with # are comments.\n", fp);
	BString redS, greenS, blueS, lineS;
	for (int i = 0; i < C_V_NUM * C_H_NUM; i++) {
		rgb_color c = menu->ItemAt(i)->getColor();
		fNumberFormat.Format(redS, (int32)int(c.red));
		fNumberFormat.Format(greenS, (int32)int(c.green));
		fNumberFormat.Format(blueS, (int32)int(c.blue));
		lineS.SetToFormat("%s %s %s\n", redS.String(), greenS.String(), blueS.String());
		fputs(lineS.String(), fp);
	}
	fclose(fp);
}

void
ColorMenuButton::Load(BEntry entry)
{
	FILE* fp;
	char line[81];
	BFile file = BFile(&entry, B_READ_ONLY);
	BPath path;
	entry.GetPath(&path);
	fp = fopen(path.Path(), "r");
	for (int i = 0; i < C_V_NUM * C_H_NUM; i++) {
		rgb_color c;
		if (fgets(line, 80, fp)) {
			if (line[0] != '#')	 // Comment line
			{
				char* endp = line;
				c.red = clipchar(int(strtol(endp, &endp, 0)));
				c.green = clipchar(int(strtol(endp, &endp, 0)));
				c.blue = clipchar(int(strtol(endp, &endp, 0)));
				c.alpha = 255;
				menu->ItemAt(i)->setColor(c);
			} else
				i--;
		} else {
			c.red = 0;
			c.green = 0;
			c.blue = 0;
			c.alpha = 255;
			menu->ItemAt(i)->setColor(c);
		}
	}
	fclose(fp);
	Window()->Lock();
	Invalidate();
	extern PatternMenuButton* pat;
	pat->Invalidate();
	Window()->Unlock();
	if (MenuWinOnScreen)
		menu->InvalidateWindow();
}

void
ColorMenuButton::Generate(ulong w)
{
	if (w == 'PGdf') {
		for (int i = 0; i < 256; i++)
			menu->ItemAt(i)->setColor(system_colors()->color_list[i]);
	} else if (w == 'PGht') {
		rgb_color c;
		for (int i = 0; i <= 43; i++) {
			c.red = 0;
			c.green = 0;
			c.blue = 255 * i / 43;
			menu->ItemAt(i)->setColor(c);

			c.green = 255 * i / 51;
			c.blue = 255;
			menu->ItemAt(i + 42)->setColor(c);

			c.red = 0;
			c.green = 255;
			c.blue = 255 - 255 * i / 43;
			menu->ItemAt(i + 85)->setColor(c);

			c.red = 255 * i / 43;
			c.green = 255;
			c.blue = 0;
			menu->ItemAt(i + 128)->setColor(c);

			c.red = 255;
			c.green = 255 - 255 * i / 43;
			c.blue = 0;
			menu->ItemAt(i + 170)->setColor(c);

			c.green = 255 * i / 43;
			c.blue = 255 * i / 43;
			menu->ItemAt(i + 212)->setColor(c);
		}
	} else if (w == 'PGsp') {
		for (int i = 0; i < 256; i++) {
			hsv_color h;
			h.hue = 315 * (255 - i) / 255;
			h.saturation = 1.0;
			h.value = 1.0;
			menu->ItemAt(i)->setColor(hsv2rgb(h));
		}
	} else {
		for (int i = 0; i < 256; i++) {
			rgb_color c;
			c.red = (w == 'PGG' || w == 'PGr' || w == 'PGrg' || w == 'PGrb') ? i : 0;
			c.green = (w == 'PGG' || w == 'PGg' || w == 'PGrg' || w == 'PGgb') ? i : 0;
			c.blue = (w == 'PGG' || w == 'PGb' || w == 'PGrb' || w == 'PGgb') ? i : 0;
			menu->ItemAt(i)->setColor(c);
		}
	}
	Window()->Lock();
	Invalidate();
	extern PatternMenuButton* pat;
	pat->Invalidate();
	Window()->Unlock();
	if (MenuWinOnScreen)
		menu->InvalidateWindow();
}

void
ColorMenuButton::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_PASTE:
		{
			if (msg->WasDropped()) {
				rgb_color* dropped;
				long dummy;
				msg->FindData("RGBColor", B_RGB_COLOR_TYPE, (const void**)&dropped, &dummy);
				set(*dropped, false);
			} else
				fprintf(stderr, "ColorMenuButton just received a non-dropped B_PASTE?!\n");
			break;
		}
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			if (msg->FindRef("refs", &ref) == B_NO_ERROR) {
				BNode node = BNode(&ref);
				ssize_t s;
				char buffer[1024];
				if ((s = node.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, buffer, 1022)) > 0) {
					buffer[s] = 0;
					if (!strcmp(buffer, "application/x-sum-becasso.palette")) {
						BEntry entry = BEntry(&ref);
						Load(entry);
						break;
					}
				}
			}
			// Note: Fall through to default!
		}
		default:
			inherited::MessageReceived(msg);
			break;
	}
}
