/*****************************************************************************

	Projet	: Mandelbrot add-on for Becasso 1.1

	Fichier	: mandelbrot.cpp

	Auteur	: RM
	Date		: 241297
	Format	: tabs==2

	Any redistribution or reuse forbidden (execpt as part of a Becasso
	distribution) without explicit notice from the author.
	Using this code as a base sample code to create even more powerfull add-ons
	for Becasso is fully allowed and encouraged. This file shouldn't be
	distributed without its corresponding BeIDE project.
	The author, <raphael.moll@inforoute.cgs.fr>, cannot be taken as responsible
	for anything bad or good that may happen from the use, misuse or unuse of
	this source or anything compiled from it.

	(12may2001-SRMS adapted to 2.0 API)

*****************************************************************************/

#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include <math.h>
#include <stdlib.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <Screen.h>
#include <Box.h>
#include <RadioButton.h>
#include <StringView.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <string.h>

#define K_MANDEL 1
#define K_JULIA 2

#define K_MANDEL_MSG 'maMd'
#define K_JULIA_MSG 'juMd'
#define K_XCENTER_MSG 'xcMd'
#define K_YCENTER_MSG 'ycMd'
#define K_XZOOM_MSG 'xzMd'
#define K_YZOOM_MSG 'yzMd'
#define K_ANGLE_MSG 'agMd'
#define K_INVERT_MSG 'inMd'
#define K_APPLY_ITER_MSG 'aiMd'
#define K_PREV_ITER_MSG 'piMd'
#define K_BLOCK_4_MSG '04Md'
#define K_BLOCK_8_MSG '08Md'
#define K_BLOCK_16_MSG '16Md'
#define K_ISOMORPH_MSG 'imMd'

//---------------------------------------------------------------------------
// prototypes

void
drawMandel(
	Layer* layer, uint32 width, uint32 height, bool final, BPoint point, Layer* inLayer,
	Selection* selection, bool julia
);

void
selectMandel(
	Selection* outselection, uint32 width, uint32 height, bool final, BPoint point,
	Selection* selection, bool julia
);

//---------------------------------------------------------------------------
// local static pointers

#define EPSILON (1e-6)
#define max(a, b) ((a) > (b) ? (a) : (b))

//---------------------------------------------------------------------------

//***************************************************************************
class MandelView : public BView
//***************************************************************************
{
  public:
	MandelView(BRect rect);
	virtual ~MandelView(void){};
	virtual void MessageReceived(BMessage* msg);

	int type(void) { return mType; }

	uint32 blockSize(void) { return mBlockSize; }

	bool invert(void) { return mInvert; }

	bool isomorphic(void) { return mIsomorphic; }

	uint32 applyIterations(void) { return mApplyIterations; }

	uint32 previewIterations(void) { return mPreviewIterations; }

	void displayCenter(void);
	void displayZoom(void);
	void displayAngle(void);
	void displayIter(void);

	double mXCenter, mYCenter, mXZoom, mYZoom;
	int32 mAngle;

	// interface items
	BRadioButton* mIMandel;
	BRadioButton* mIJulia;
	BTextControl *mIXCenter, *mIYCenter, *mIXZoom, *mIYZoom, *mIAngle;
	BTextControl *mIApplyIter, *mIPreviewIter;
	BCheckBox *mIInvert, *mIIsomorphic;

	BPoint mLastZoom;
	BPoint mLastRotate;
	BPoint mLastSlide;
	uint32 mLastModifier;

	double mCos[360];
	double mSin[360];

  private:
	typedef BView inherited;
	int mType;
	bool mInvert;
	bool mIsomorphic;
	uint32 mBlockSize; // size of blocks in preview mode
	uint32 mApplyIterations;
	uint32 mPreviewIterations;
};

MandelView* view = 0;

//***************************************************************************
MandelView::MandelView(BRect rect) : BView(rect, "mandel_view", B_FOLLOW_ALL, B_WILL_DRAW)
//***************************************************************************
{
	mType = K_MANDEL;

	mBlockSize = 4;
	mApplyIterations = 256;
	mPreviewIterations = 64;
	mInvert = false;
	mIsomorphic = true;
	mXCenter = 0.0;
	mYCenter = 0.0;
	mXZoom = 1.0;
	mYZoom = 1.0;
	mAngle = 0;

	mLastModifier = 0;

	for (int i = 0; i < 360; i++) {
		mCos[i] = cos(i * 2 * 3.141592654 / 360.0);
		mSin[i] = sin(i * 2 * 3.141592654 / 360.0);
	}

#define KH1 16 // size for plain font
#define KH2 16 // size for bold font
#define KH3 25
#define KW 200

	uint32 kh1, kh2, kh3;

	// get fonts height
	font_height fh;
	be_plain_font->GetHeight(&fh);
	kh1 = uint32(fh.ascent + fh.descent + fh.leading);
	be_bold_font->GetHeight(&fh);
	kh2 = uint32(fh.ascent + fh.descent + fh.leading);

	kh1 = max(KH1, kh1 + 5);
	kh2 = max(KH2, kh2 + 5);
	kh3 = kh1 + 10;

	uint32 th = kh2 + 5 * kh3 + 10 + kh2 + 2 * kh1 + 10 + kh2 + 3 * kh1 + 5 * kh3 + 10;
	ResizeTo(KW, th);

	// create window content

	const uint32 x1 = 4;		   // box left
	const uint32 x2 = 8;		   // items left
	const uint32 x3 = KW - 4;	   // box right
	const uint32 x4 = KW - 8 - x2; // item right
	uint32 y1 = 1, y2;

	// -- type box --

	BBox* type = new BBox(BRect(x1, y1, x3, y1 + kh2 + 2 * kh1 + 5), "type");
	y1 += kh2 + 2 * kh1 + 10;
	type->SetLabel("Type");
	AddChild(type);

	y2 = kh2;
	mIMandel = new BRadioButton(
		BRect(x2, y2, x4, y2 + kh1), "mandel", "Mandelbrot", new BMessage(K_MANDEL_MSG)
	);
	y2 += kh1;
	mIJulia =
		new BRadioButton(BRect(x2, y2, x4, y2 + kh1), "julia", "Julia", new BMessage(K_JULIA_MSG));

	type->AddChild(mIMandel);
	type->AddChild(mIJulia);
	mIMandel->SetValue(true);

	// -- interactive settings box --

	BBox* inter = new BBox(BRect(x1, y1, x3, y1 + kh2 + 3 * kh1 + 5 * kh3 + 3), "inter");
	y1 += kh2 + 3 * kh1 + 5 * kh3 + 10;
	inter->SetLabel("Interactive Settings");
	AddChild(inter);

	y2 = kh2;
	inter->AddChild(new BStringView(BRect(x2, y2, x4, y2 + kh1), "label1", "[Center: mouse]"));
	y2 += kh1;
	mIXCenter = new BTextControl(
		BRect(x2, y2, x4, y2 + kh3), "xcenter", "X Center", "0", new BMessage(K_XCENTER_MSG)
	);
	mIXCenter->SetModificationMessage(new BMessage(K_XCENTER_MSG));
	y2 += kh3;
	mIYCenter = new BTextControl(
		BRect(x2, y2, x4, y2 + kh3), "ycenter", "Y Center", "0", new BMessage(K_YCENTER_MSG)
	);
	mIYCenter->SetModificationMessage(new BMessage(K_YCENTER_MSG));
	y2 += kh3;
	inter->AddChild(new BStringView(BRect(x2, y2, x4, y2 + kh1), "label1", "[Zoom: Shift+mouse]"));
	y2 += kh1;
	mIXZoom = new BTextControl(
		BRect(x2, y2, x4, y2 + kh3), "xcenter", "X Zoom", "0", new BMessage(K_XZOOM_MSG)
	);
	mIXZoom->SetModificationMessage(new BMessage(K_XZOOM_MSG));
	y2 += kh3;
	mIYZoom = new BTextControl(
		BRect(x2, y2, x4, y2 + kh3), "ycenter", "Y Zoom", "0", new BMessage(K_YZOOM_MSG)
	);
	mIYZoom->SetModificationMessage(new BMessage(K_YZOOM_MSG));
	y2 += kh3;
	inter->AddChild(new BStringView(BRect(x2, y2, x4, y2 + kh1), "label1", "[Angle: Control+mouse]")
	);
	y2 += kh1;
	mIAngle = new BTextControl(
		BRect(x2, y2, x4, y2 + kh3), "angle", "Angle", "0", new BMessage(K_ANGLE_MSG)
	);
	mIAngle->SetModificationMessage(new BMessage(K_ANGLE_MSG));

	// x5 =
	// max(mXCenter->Divider(),max(mYCenter->Divider(),max(mXZoom->Divider(),max(mYZoom->Divider(),mAngle->Divider()))));
	uint32 x5 = uint32(mIXCenter->StringWidth("X Center") + 10);
	mIXCenter->SetDivider(x5);
	mIYCenter->SetDivider(x5);
	mIXZoom->SetDivider(x5);
	mIYZoom->SetDivider(x5);
	mIAngle->SetDivider(x5);

	inter->AddChild(mIXCenter);
	inter->AddChild(mIYCenter);
	inter->AddChild(mIXZoom);
	inter->AddChild(mIYZoom);
	inter->AddChild(mIAngle);

	// -- fixed settings box --

	BBox* fixed = new BBox(BRect(x1, y1, x3, y1 + kh2 + 5 * kh3 + 5), "fixed");
	fixed->SetLabel("Fixed Settings");
	AddChild(fixed);

	y2 = kh2;
	mIApplyIter = new BTextControl(
		BRect(x2, y2, x4, y2 + kh3), "applyiter", "Apply Iterations", "0",
		new BMessage(K_APPLY_ITER_MSG)
	);
	mIApplyIter->SetModificationMessage(new BMessage(K_APPLY_ITER_MSG));
	y2 += kh3;
	mIPreviewIter = new BTextControl(
		BRect(x2, y2, x4, y2 + kh3), "previter", "Preview Iterations", "0",
		new BMessage(K_PREV_ITER_MSG)
	);
	mIPreviewIter->SetModificationMessage(new BMessage(K_PREV_ITER_MSG));
	y2 += kh3;

	BPopUpMenu* block = new BPopUpMenu("Colors");
	block->AddItem(new BMenuItem("4 pixels", new BMessage(K_BLOCK_4_MSG)));
	block->AddItem(new BMenuItem("8 pixels", new BMessage(K_BLOCK_8_MSG)));
	block->AddItem(new BMenuItem("16 pixels", new BMessage(K_BLOCK_16_MSG)));
	block->ItemAt(0)->SetMarked(true);
	BMenuField* blockfield =
		new BMenuField(BRect(x2, y2, x4, y2 + kh3), "block", "Preview Block Size", block);
	y2 += kh3;

	mIInvert = new BCheckBox(
		BRect(x2, y2, x4, y2 + kh3), "invert", "Invert Colors", new BMessage(K_INVERT_MSG)
	);
	mIInvert->SetValue(invert());
	y2 += kh3;
	mIIsomorphic = new BCheckBox(
		BRect(x2, y2, x4, y2 + kh3), "isomorf", "Isomorphic", new BMessage(K_ISOMORPH_MSG)
	);
	mIIsomorphic->SetValue(isomorphic());

	x5 = uint32(mIPreviewIter->StringWidth("Preview Iterations") + 10);
	mIApplyIter->SetDivider(x5);
	mIPreviewIter->SetDivider(x5);
	blockfield->SetDivider(x5);

	fixed->AddChild(mIApplyIter);
	fixed->AddChild(mIPreviewIter);
	fixed->AddChild(blockfield);
	fixed->AddChild(mIInvert);
	fixed->AddChild(mIIsomorphic);

	displayCenter();
	displayZoom();
	displayAngle();
	displayIter();
}


//***************************************************************************
void
MandelView::MessageReceived(BMessage* msg)
//***************************************************************************
{
	switch (msg->what) {
	case K_MANDEL_MSG:
		mType = K_MANDEL;
		break;
	case K_JULIA_MSG:
		mType = K_JULIA;
		break;
	case K_BLOCK_4_MSG:
		mBlockSize = 4;
		break;
	case K_BLOCK_8_MSG:
		mBlockSize = 8;
		break;
	case K_BLOCK_16_MSG:
		mBlockSize = 16;
		break;
	case K_INVERT_MSG:
		if (mIInvert)
			mInvert = mIInvert->Value();
		printf("mIInvert %p, value %" B_PRId32 "\n", mIInvert, mIInvert->Value());
		break;
	case K_ISOMORPH_MSG:
		if (mIIsomorphic)
			mIsomorphic = mIIsomorphic->Value();
		break;

#define M_XY_MSG(msg, obj, var)                                                                    \
	case msg:                                                                                      \
		if (obj) {                                                                                 \
			char* p = (char*)obj->Text();                                                          \
			if (p)                                                                                 \
				var = atof(p);                                                                     \
		}                                                                                          \
		break
		M_XY_MSG(K_XCENTER_MSG, mIXCenter, mXCenter);
		M_XY_MSG(K_YCENTER_MSG, mIYCenter, mYCenter);
		M_XY_MSG(K_XZOOM_MSG, mIXZoom, mXZoom);
		M_XY_MSG(K_YZOOM_MSG, mIYZoom, mYZoom);

#define M_INT_MSG(msg, obj, var)                                                                   \
	case msg:                                                                                      \
		if (obj) {                                                                                 \
			char* p = (char*)obj->Text();                                                          \
			if (p)                                                                                 \
				var = atol(p);                                                                     \
		}                                                                                          \
		break
		M_INT_MSG(K_APPLY_ITER_MSG, mIApplyIter, mApplyIterations);
		M_INT_MSG(K_PREV_ITER_MSG, mIPreviewIter, mPreviewIterations);
		M_INT_MSG(K_ANGLE_MSG, mIAngle, mAngle);

	default:
		inherited::MessageReceived(msg);
	}
}


//***************************************************************************
void
MandelView::displayCenter(void)
//***************************************************************************
{
	char s[64];
	if (LockLooper()) {
		sprintf(s, "%f", mXCenter);
		if (mIXCenter)
			mIXCenter->SetText(s);
		sprintf(s, "%f", mYCenter);
		if (mIYCenter)
			mIYCenter->SetText(s);
		UnlockLooper();
	}
}


//***************************************************************************
void
MandelView::displayZoom(void)
//***************************************************************************
{
	char s[64];
	if (LockLooper()) {
		sprintf(s, "%f", mXZoom);
		if (mIXZoom)
			mIXZoom->SetText(s);
		sprintf(s, "%f", mYZoom);
		if (mIYZoom)
			mIYZoom->SetText(s);
		UnlockLooper();
	}
}


//***************************************************************************
void
MandelView::displayAngle(void)
//***************************************************************************
{
	char s[64];
	if (LockLooper()) {
		sprintf(s, "%" B_PRId32 " deg", mAngle);
		if (mIAngle)
			mIAngle->SetText(s);
		UnlockLooper();
	}
}


//***************************************************************************
void
MandelView::displayIter(void)
//***************************************************************************
{
	char s[64];
	if (LockLooper()) {
		sprintf(s, "%" B_PRId32, mApplyIterations);
		if (mIApplyIter)
			mIApplyIter->SetText(s);
		sprintf(s, "%" B_PRId32, mPreviewIterations);
		if (mIPreviewIter)
			mIPreviewIter->SetText(s);
		UnlockLooper();
	}
}


//---------------------------------------------------------------------------

//***************************************************************************
status_t
addon_init(uint32 index, becasso_addon_info* info)
//***************************************************************************
{
	strcpy(info->name, "Mandelbrot");
	strcpy(info->author, "by R'alf <raphael.moll@capway.com>");
	strcpy(info->copyright, "The PowerPulsar Guy.");
	strcpy(
		info->description, "\nGenerates a color Mandelbrot or Julia\nKeys :\n"
						   "Cmd+mouse : move the center,\n"
						   "Cmd+Shift+Mouse : change the zoom"
	);

	info->type = BECASSO_GENERATOR;
	info->index = index;
	info->version = 0;
	info->release = 2;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = PREVIEW_FULLSCALE | PREVIEW_MOUSE;
	info->flags = 0;

	return B_OK;
}

//***************************************************************************
status_t
addon_exit(void)
//***************************************************************************
{
	return B_OK;
}

status_t
addon_close(void)
{
	return B_OK;
}

status_t
addon_make_config(BView** vw, BRect rect)
{
	view = new MandelView(rect);
	*vw = view;
	return B_OK;
}


// ----------

//***************************************************************************
void
drawMandel(
	Layer* layer, uint32 width, uint32 height, bool final, BPoint point, Layer* inLayer,
	Selection* selection, bool julia
)
//***************************************************************************
/*
	Note for the curious reader :
	This function has grown into a complex mess all over the time.
	Don't be afraid if it looks messy at first -- in fact it is :-)
*/
{
	uint32* dmap = (uint32*)layer->Bits() - 1;
	uint32 dbpr = layer->BytesPerRow() / sizeof(uint32);
	uint32 *smap = 0, sbpr = 0, selbpr = 0;
	uint8* selmap = 0;
	uint32 block;
	double a, b, ar, br, cr, sr, x, xp, x2, y, y2;
	int k;
	uint32 ki = 255;
	int i, j, step, angle;

	if (!height || !width || !view)
		return;

	if (final)
		addon_start();
	float delta = 100.0 / height; // For the Status Bar.

	double xcenter = view->mXCenter;
	double ycenter = view->mYCenter;
	double xzoom = view->mXZoom;
	double yzoom = view->mYZoom;
	if (xzoom < EPSILON)
		xzoom = EPSILON;
	if (yzoom < EPSILON)
		yzoom = EPSILON;

	uint32 mods = modifiers();
	if (mods & B_SHIFT_KEY) {
		// change zoom value log
		if (view->mLastModifier != B_SHIFT_KEY)
			view->mLastModifier = B_SHIFT_KEY;
		else {
			int fudge = (xzoom > 1 ? int(pow(10, log10(xzoom) + 1)) : 1);
			xzoom += fudge * (point.x - view->mLastZoom.x) / width;
			if (xzoom < EPSILON)
				xzoom = EPSILON;
			if (view->isomorphic())
				yzoom = xzoom;
			else {
				fudge = (yzoom > 1 ? int(pow(10, log10(yzoom) + 1)) : 1);
				yzoom += fudge * (point.y - view->mLastZoom.y) / height;
			}
			if (yzoom < EPSILON)
				yzoom = EPSILON;
			view->mXZoom = xzoom;
			view->mYZoom = yzoom;
			view->displayZoom();
		}
		view->mLastZoom = point;
	} else if (mods & B_CONTROL_KEY) {
		// change rotation
		if (view->mLastModifier != B_CONTROL_KEY) {
			view->mLastModifier = B_CONTROL_KEY;
			view->mLastRotate = point;
		} else {
			double a, x, y;
			// x = (point.x - window->mLastRotate.x);
			// y = (point.y - window->mLastRotate.y);
			x = (view->mLastRotate.x - point.x);
			y = (view->mLastRotate.y - point.y);
			a = atan2(y, x);
			a *= 180 / 3.141592654; // convert in degrees
			angle = int(a);
			while (angle < 0)
				angle += 360;
			while (angle >= 360)
				angle -= 360;
			view->mAngle = angle;
			view->displayAngle();
		}
	} else {
		// move center
		if (view->mLastModifier != B_COMMAND_KEY)
			view->mLastModifier = B_COMMAND_KEY;
		else {
			if (julia) {
				xcenter = ((point.x - (width / 2)) / (width / 2)) * 2.0;
				ycenter = ((point.y - (height / 2)) / (height / 2)) * 2.0;
			} else {
				xcenter = view->mXCenter + (view->mLastSlide.x - point.x) / (width / 2 * xzoom);
				ycenter = view->mYCenter + (view->mLastSlide.y - point.y) / (height / 2 * yzoom);
			}
			view->mXCenter = xcenter;
			view->mYCenter = ycenter;
			view->displayCenter();
		}
		view->mLastSlide = point;
	}

	angle = 359 - view->mAngle;
	cr = view->mCos[angle];
	sr = view->mSin[angle];

	block = view->blockSize();
	ki = (final ? view->applyIterations() : view->previewIterations()) - 1;
	// ki = view->applyIterations();
	step = (final ? 1 : block);
	rgb_color hi = highcolor();
	rgb_color lo = lowcolor();
	if (view->invert()) {
		rgb_color t = lo;
		lo = hi;
		hi = t;
	}

	if (selection) {
		smap = (uint32*)inLayer->Bits() - 1;
		sbpr = inLayer->BytesPerRow() / sizeof(uint32);
		selmap = (uint8*)selection->Bits() - 1;
		selbpr = selection->BytesPerRow() / sizeof(uint8);
	}

	xzoom = 4.0 / xzoom;
	yzoom = 4.0 / yzoom;
	if (!julia) {
		xcenter += xzoom / 2;
		ycenter += yzoom / 2;
	}
	xzoom /= width;
	yzoom /= height;

	for (j = height; j > 0; j -= step) {
		uint32* dbits = dmap;
		uint32* sbits = smap;
		uint8* selbits = selmap;
		dmap += dbpr * step;

		if (final) {
			addon_update_statusbar(delta);
			if (addon_stop()) {
				// error = ADDON_ABORT;
				break;
			}
		}

		if (selection) {
			smap += sbpr * step;
			selmap += selbpr * step;
		}

		int yr = step;
		if (j < yr)
			yr = j;

		if (julia)
			br = ((int)(height / 2) - j) * yzoom; // julia
		else
			br = ycenter - yzoom * j; // mandelbrot

		for (i = width; i > 0; i -= step) {
			if (julia) {
				// julia
				ar = ((int)(width / 2) - i) * xzoom;
				a = cr * ar - sr * br;
				b = sr * ar + cr * br;
				x = a;
				y = b;
				x2 = x * x;
				y2 = y * y;
				for (k = ki; k > 0 && (x2 + y2) < 4.0; k--) {
					// julia : like a mandelbrot (z=z2+c) except that :
					// z=(a,b) moving on the canvas for each pixel
					// c=(rx,ry) coordinates selected on the mandelbrot set that doesn't move across
					// iterations z = z2+c

					xp = x2 - y2 + xcenter;
					y = 2 * x * y + ycenter;
					x = xp;
					x2 = x * x;
					y2 = y * y;
				}
			} else {
				// mandelbrot
				ar = xcenter - xzoom * i;
				a = cr * ar - sr * br;
				b = sr * ar + cr * br;

				x = 0.0;
				y = 0.0;
				x2 = 0.0;
				y2 = 0.0;
				for (k = ki; k > 0 && (x2 + y2) < 4.0; k--) {
					// z = z2+c
					// z = (x+iy)
					// c=(a+ib)
					// z2 = x2-y2+i2xy
					// x' = x2-y2+a
					// y' = xy + b

					xp = x2 - y2 + a;
					y = 2 * x * y + b;
					x = xp;
					x2 = x * x;
					y2 = y * y;
				}
			}

			if (k < 0)
				k = 0;
			uint32 pixel = weighted_average_rgb(lo, k, hi, ki - k) & COLOR_MASK;

			int xr = step;
			if (xr > 1) {
				if (i < xr)
					xr = i;
				uint32* dbits2 = dbits;
				int offset = dbpr - xr;

				// this hint has a side effect that can be considered a bug :
				// --> the alpha value gets modified by blocks
				if (selection) {
					pixel = pixelblend(*(sbits + 1), (pixel | (*(selbits + 1)) << ALPHA_BPOS));
					sbits += step;
					selbits += step;
				} else
					pixel |= ALPHA_MASK;
				dbits += step;

				for (int yr2 = yr; yr2 > 0; yr2--) {
					switch (xr) {
					case 16:
						*(++dbits2) = pixel;
					case 15:
						*(++dbits2) = pixel;
					case 14:
						*(++dbits2) = pixel;
					case 13:
						*(++dbits2) = pixel;
					case 12:
						*(++dbits2) = pixel;
					case 11:
						*(++dbits2) = pixel;
					case 10:
						*(++dbits2) = pixel;
					case 9:
						*(++dbits2) = pixel;
					case 8:
						*(++dbits2) = pixel;
					case 7:
						*(++dbits2) = pixel;
					case 6:
						*(++dbits2) = pixel;
					case 5:
						*(++dbits2) = pixel;
					case 4:
						*(++dbits2) = pixel;
					case 3:
						*(++dbits2) = pixel;
					case 2:
						*(++dbits2) = pixel;
					case 1:
						*(++dbits2) = pixel;
					}
					dbits2 += offset;
				}
			} else {
				// step is 1
				if (selection)
					*(++dbits) = pixelblend(*(++sbits), (pixel | (*(++selbits)) << ALPHA_BPOS));
				else
					*(++dbits) = pixel | ALPHA_MASK;
			}
		}
	}
	if (final)
		addon_done();
}

// ----------


//***************************************************************************
void
selectMandel(
	Selection* outselection, uint32 width, uint32 height, bool final, BPoint point,
	Selection* /*selection*/, bool julia
)
//***************************************************************************
/*
	Note for the curious reader :
	This function is an ugly copy-paste from drawMandel.
	Don't focus on this mess, better have a look a drawMandel directly.

	Note : for the moment, the inSelection is ignored. It doesn't make sense here, indeed.
*/
{
	uint8* dmap = (uint8*)outselection->Bits() - 1;
	uint32 dbpr = outselection->BytesPerRow() / sizeof(uint8);
	// uint32 selbpr;
	// uint8 *selmap;
	uint32 block;
	double a, b, ar, br, cr, sr, x, xp, x2, y, y2;
	int k;
	uint32 ki = 255;
	int i, j, step, angle;

	if (!height || !width || !view)
		return;

	if (final)
		addon_start();

	float delta = 100.0 / height; // For the Status Bar.

	double xcenter = view->mXCenter;
	double ycenter = view->mYCenter;
	double xzoom = view->mXZoom;
	double yzoom = view->mYZoom;
	if (xzoom < EPSILON)
		xzoom = EPSILON;
	if (yzoom < EPSILON)
		yzoom = EPSILON;

	uint32 mods = modifiers();
	if (mods & B_SHIFT_KEY) {
		// change zoom value log
		if (view->mLastModifier != B_SHIFT_KEY)
			view->mLastModifier = B_SHIFT_KEY;
		else {
			int fudge = (xzoom > 1 ? int(pow(10, log10(xzoom) + 1)) : 1);
			xzoom += fudge * (point.x - view->mLastZoom.x) / width;
			if (xzoom < EPSILON)
				xzoom = EPSILON;
			if (view->isomorphic())
				yzoom = xzoom;
			else {
				fudge = (yzoom > 1 ? int(pow(10, log10(yzoom) + 1)) : 1);
				yzoom += fudge * (point.y - view->mLastZoom.y) / height;
			}
			if (yzoom < EPSILON)
				yzoom = EPSILON;
			view->mXZoom = xzoom;
			view->mYZoom = yzoom;
			view->displayZoom();
		}
		view->mLastZoom = point;
	} else if (mods & B_CONTROL_KEY) {
		// change rotation
		if (view->mLastModifier != B_CONTROL_KEY) {
			view->mLastModifier = B_CONTROL_KEY;
			view->mLastRotate = point;
		} else {
			double a, x, y;
			// x = (point.x - view->mLastRotate.x);
			// y = (point.y - view->mLastRotate.y);
			x = (view->mLastRotate.x - point.x);
			y = (view->mLastRotate.y - point.y);
			a = atan2(y, x);
			a *= 180 / 3.141592654; // convert in degrees
			angle = int(a);
			while (angle < 0)
				angle += 360;
			while (angle >= 360)
				angle -= 360;
			view->mAngle = angle;
			view->displayAngle();
		}
	} else {
		// move center
		if (view->mLastModifier != B_COMMAND_KEY)
			view->mLastModifier = B_COMMAND_KEY;
		else {
			if (julia) {
				xcenter = ((point.x - (width / 2)) / (width / 2)) * 2.0;
				ycenter = ((point.y - (height / 2)) / (height / 2)) * 2.0;
			} else {
				xcenter = view->mXCenter + (view->mLastSlide.x - point.x) / (width / 2 * xzoom);
				ycenter = view->mYCenter + (view->mLastSlide.y - point.y) / (height / 2 * yzoom);
			}
			view->mXCenter = xcenter;
			view->mYCenter = ycenter;
			view->displayCenter();
		}
		view->mLastSlide = point;
	}

	angle = 359 - view->mAngle;
	cr = view->mCos[angle];
	sr = view->mSin[angle];

	block = view->blockSize();
	ki = (final ? view->applyIterations() : view->previewIterations()) - 1;
	step = (final ? 1 : block);
	/*
		rgb_color hi = highcolor();
		rgb_color lo = lowcolor();
		if (window->invert())
		{
			rgb_color t = lo;
			lo = hi;
			hi = t;
		}
	*/

	/*
		if (selection)
		{
			selmap = (uint8 *)selection->Bits() - 1;
			selbpr = selection->BytesPerRow()/sizeof(uint8);
		}
	*/
	xzoom = 4.0 / xzoom;
	yzoom = 4.0 / yzoom;
	if (!julia) {
		xcenter += xzoom / 2;
		ycenter += yzoom / 2;
	}
	xzoom /= width;
	yzoom /= height;

	for (j = height; j > 0; j -= step) {
		uint8* dbits = dmap;
		//		uint8 *selbits = selmap;
		dmap += dbpr * step;

		if (final) {
			addon_update_statusbar(delta);
			if (addon_stop()) {
				// error = ADDON_ABORT;
				break;
			}
		}

		//		if (selection) selmap += selbpr*step;

		int yr = step;
		if (j < yr)
			yr = j;

		if (julia)
			br = ((int)(height / 2) - j) * yzoom; // julia
		else
			br = ycenter - yzoom * j; // mandelbrot

		for (i = width; i > 0; i -= step) {
			if (julia) {
				// julia
				ar = ((int)(width / 2) - i) * xzoom;
				a = cr * ar - sr * br;
				b = sr * ar + cr * br;
				x = a;
				y = b;
				x2 = x * x;
				y2 = y * y;
				for (k = ki; k > 0 && (x2 + y2) < 4.0; k--) {
					// julia : like a mandelbrot (z=z2+c) except that :
					// z=(a,b) moving on the canvas for each pixel
					// c=(rx,ry) coordinates selected on the mandelbrot set that doesn't move across
					// iterations z = z2+c

					xp = x2 - y2 + xcenter;
					y = 2 * x * y + ycenter;
					x = xp;
					x2 = x * x;
					y2 = y * y;
				}
			} else {
				// mandelbrot
				ar = xcenter - xzoom * i;
				a = cr * ar - sr * br;
				b = sr * ar + cr * br;

				x = 0.0;
				y = 0.0;
				x2 = 0.0;
				y2 = 0.0;
				for (k = ki; k > 0 && (x2 + y2) < 4.0; k--) {
					// z = z2+c
					// z = (x+iy)
					// c=(a+ib)
					// z2 = x2-y2+i2xy
					// x' = x2-y2+a
					// y' = xy + b

					xp = x2 - y2 + a;
					y = 2 * x * y + b;
					x = xp;
					x2 = x * x;
					y2 = y * y;
				}
			}

			if (k < 0)
				k = 0;
			// selection colors :
			// 0 = use layer pattern (selection)
			// 255 = nothing (transparent, no selection)
			// default : make mandelbrot zone to 0, outside to 255, invert if needed
			uint32 pixel = uint32((double)(ki - k) / (double)ki * 255.0);
			if (view->invert())
				pixel = 255 - pixel;

			int xr = step;
			if (xr > 1) {
				if (i < xr)
					xr = i;
				uint8* dbits2 = dbits;
				int offset = dbpr - xr;

				// this hint has a side effect that can be considered a bug :
				// --> the alpha value gets modified by blocks
				//				if (selection)
				//				{
				//					pixel = pixelblend (*(sbits+1), (pixel | *(selbits+1)));
				//					sbits += step;
				//					selbits += step;
				//				}
				//				else
				//					pixel |= 0x0FF;
				dbits += step;

				for (int yr2 = yr; yr2 > 0; yr2--) {
					switch (xr) {
					case 16:
						*(++dbits2) = pixel;
					case 15:
						*(++dbits2) = pixel;
					case 14:
						*(++dbits2) = pixel;
					case 13:
						*(++dbits2) = pixel;
					case 12:
						*(++dbits2) = pixel;
					case 11:
						*(++dbits2) = pixel;
					case 10:
						*(++dbits2) = pixel;
					case 9:
						*(++dbits2) = pixel;
					case 8:
						*(++dbits2) = pixel;
					case 7:
						*(++dbits2) = pixel;
					case 6:
						*(++dbits2) = pixel;
					case 5:
						*(++dbits2) = pixel;
					case 4:
						*(++dbits2) = pixel;
					case 3:
						*(++dbits2) = pixel;
					case 2:
						*(++dbits2) = pixel;
					case 1:
						*(++dbits2) = pixel;
					}
					dbits2 += offset;
				}
			} else {
				// step is 1
				// if (selection) *(++dbits) = pixelblend (*(++sbits), (pixel | *(++selbits)));
				// else *(++dbits) = pixel | 0x0FF;
				*(++dbits) = pixel;
			}
		}
	}
	if (final)
		addon_done();
}

// ----------

//***************************************************************************
status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* /* frame */, bool final, BPoint point, uint32 buttons
)
//***************************************************************************
{
	static BPoint lastpoint = BPoint(0, 0);

	int error = 0;

	if (!final && buttons)
		lastpoint = point;
	if (!buttons && view)
		view->mLastModifier = 0;

	BRect bounds = inLayer->Bounds();
	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer(*inLayer);

	if (*outSelection == NULL && mode == M_SELECT)
		*outSelection = new Selection(inLayer->Bounds());

	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();
	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;

	rgb_color hi = highcolor();
	rgb_color lo = lowcolor();

	switch (mode) {
	case M_DRAW: {
		if (view->type() == K_MANDEL)
			drawMandel(*outLayer, w, h, final, lastpoint, inLayer, inSelection, false);
		else {
			drawMandel(*outLayer, w, h, final, lastpoint, inLayer, inSelection, true);
			if (*outLayer && buttons) {
				// circle for mandelbrot : c=<40,50> percent, radius=<40-20=20> percent
				BView* view = new BView(inLayer->Bounds(), "tmp view", 0, 0);
				(*outLayer)->AddChild(view);
				view->SetHighColor(contrastingcolor(hi, lo));
				float x = lastpoint.x;
				float y = lastpoint.y;
				view->StrokeLine(BPoint(x - 5, y), BPoint(x + 5, y));
				view->StrokeLine(BPoint(x, y - 5), BPoint(x, y + 5));
				float w1 = (float)w * 0.4;
				float h1 = (float)h * 0.5;
				float r = (float)w * 0.2;
				view->StrokeEllipse(BPoint(w1, h1), r, r);
				view->Sync();
				(*outLayer)->RemoveChild(view);
				delete view;
			}
		}

		break;
	}
	case M_SELECT: {
		if (view->type() == K_MANDEL)
			selectMandel(*outSelection, w, h, final, lastpoint, inSelection, false);
		else {
			selectMandel(*outSelection, w, h, final, lastpoint, inSelection, true);
			if (*outLayer && buttons) {
				// circle for mandelbrot : c=<40,50> percent, radius=<40-20=20> percent
				BView* view = new BView(inLayer->Bounds(), "tmp view", 0, 0);
				(*outSelection)->AddChild(view);
				view->SetHighColor(contrastingcolor(hi, lo));
				float x = lastpoint.x;
				float y = lastpoint.y;
				view->StrokeLine(BPoint(x - 5, y), BPoint(x + 5, y));
				view->StrokeLine(BPoint(x, y - 5), BPoint(x, y + 5));
				float w1 = (float)w * 0.4;
				float h1 = (float)h * 0.5;
				float r = (float)w * 0.2;
				view->StrokeEllipse(BPoint(w1, h1), r, r);
				view->Sync();
				(*outSelection)->RemoveChild(view);
				delete view;
			}
		}
		break;
	}
	default:
		fprintf(stderr, "Mandel: Unknown mode\n");
		error = 1;
	}

	if (!buttons && view)
		view->mLastModifier = 0;

	if (*outSelection)
		(*outSelection)->Unlock();
	if (*outLayer)
		(*outLayer)->Unlock();
	return (error);
}

//---------------------------------------------------------------------------
// eoc
