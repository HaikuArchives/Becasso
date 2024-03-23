// © 1999 - 2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include <Box.h>
#include <View.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <RadioButton.h>
#include <Button.h>
#include <StringView.h>
#include "Slider.h"
#include "TabView.h"
#include "Colors.h"
#include <string.h>

#define MAC_GAMMA 1.8

#define CS_RGBA 0
#define CS_CMYK 1
#define CS_HSV 2
#define CS_RGBL 3

#define GT_STRAIGHT 0
#define GT_SMOOTH 1
#define GT_GAMMA 2

#define NUM_CHANNELS 11 // RGBA + CMYK + HSV

#ifdef __INTEL__
extern "C" void
gamma_x86(uint32* s, uint32* d, uint32* table, uint32 numPix);
#endif

#define MAX_POINTS 64

typedef struct
{
	unsigned char x;
	unsigned char y;
} point;

class CurveView : public BView
{
  public:
	CurveView(BRect rect, const char* name) : BView(rect, name, 0, B_WILL_DRAW)
	{
		type = GT_STRAIGHT;
		SetViewColor(B_TRANSPARENT_32_BIT);
		for (int i = 0; i < NUM_CHANNELS; i++) {
			fGamma[i] = 1.0;
			npoints[i] = 2;
			points[i][0].x = 0;
			points[i][0].y = 0;
			points[i][1].x = 255;
			points[i][1].y = 255;
			FillTable(i);
		}
		fCurrent = 99;
	}

	virtual ~CurveView() {}

	virtual void Draw(BRect update);
	virtual void MouseDown(BPoint point);

	void AddPoint();
	void do_AddPoint(int channel);
	void RemovePoint();
	void do_RemovePoint(int channel);
	void SetType(int t);

	void SetGamma(float gamma)
	{
		if (fCurrent == 99) {
			fGamma[0] = gamma;
			fGamma[1] = gamma;
			fGamma[2] = gamma;
		} else
			fGamma[fCurrent] = gamma;
	}

	float Gamma(int c = -1)
	{
		if (c == -1)
			c = fCurrent;
		if (c == 99)
			return fGamma[0];
		else
			return fGamma[c];
	}

	void FillTable(int channel = -1);
	void do_FillTable(int channel);

	int NPoints()
	{
		if (fCurrent == 99)
			return npoints[0];
		else
			return npoints[fCurrent];
	}

	int Current() { return fCurrent; }

	void SetCurrent(int current);

	unsigned char wct[NUM_CHANNELS][256];

  private:
	int type;
	int npoints[NUM_CHANNELS];
	point points[NUM_CHANNELS][MAX_POINTS];
	int added[NUM_CHANNELS][MAX_POINTS];
	int fCurrent;
	float fGamma[NUM_CHANNELS];
};

CurveView* gCurve = 0;

class ColorCurvesView : public BView
{
  public:
	ColorCurvesView(BRect rect) : BView(rect, "color_curves", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		ResizeTo(430, 308);
		controlsBox = new BBox(BRect(280, 8, 422, 298), "controls box");
		graphBox = new BBox(BRect(8, 8, 272, 298), "graph tabs");
		AddChild(graphBox);
		AddChild(controlsBox);

		cspacePU = new BPopUpMenu("ColorSpace");
		cspacePU->AddItem(new BMenuItem("RGBA", new BMessage('cspR')));
		cspacePU->AddItem(new BMenuItem("CMYK", new BMessage('cspC')));
		cspacePU->AddItem(new BMenuItem("HSV", new BMessage('cspH')));
		cspacePU->AddItem(new BMenuItem("RGB Locked", new BMessage('cspL')));
		cspacePU->ItemAt(CS_RGBL)->SetMarked(true);
		cspaceMF = new BMenuField(BRect(4, 264, 104, 278), "cspaceMF", "", cspacePU);
		cspaceMF->SetDivider(0);
		graphBox->AddChild(cspaceMF);

#if 0
				graphPU = new BPopUpMenu ("Graphed");
				graphPU->AddItem (new BMenuItem ("Red", new BMessage ('RspR')));
				graphPU->AddItem (new BMenuItem ("Green", new BMessage ('RspG')));
				graphPU->AddItem (new BMenuItem ("Blue", new BMessage ('RspB')));
				graphPU->AddItem (new BMenuItem ("Alpha", new BMessage ('RspA')));
				graphPU->ItemAt(0)->SetMarked (true);
				graphMF = new BMenuField (BRect (108, 264, 188, 278), "graphMF", "", graphPU);
				graphMF->SetDivider (0);
				graphBox->AddChild (graphMF);
#else
		graphMF = NULL;
#endif

		typeBox = new BBox(BRect(8, 8, 134, 66), "typeBox");
		typeBox->SetLabel("Correction");
		controlsBox->AddChild(typeBox);

		freeRB = new BRadioButton(
			BRect(8, 16, 118, 32), "freeRB", "Free Function", new BMessage('Tfre')
		);
		gammaRB = new BRadioButton(BRect(8, 34, 118, 50), "gammaRB", "Gamma", new BMessage('Tgam'));
		typeBox->AddChild(freeRB);
		typeBox->AddChild(gammaRB);
		freeRB->SetValue(B_CONTROL_ON);

		addB = new BButton(BRect(8, 72, 132, 88), "addB", "Add Point", new BMessage('addP'));
		removeB =
			new BButton(BRect(8, 100, 132, 128), "removeB", "Remove Point", new BMessage('delP'));
		controlsBox->AddChild(addB);
		controlsBox->AddChild(removeB);
		removeB->SetEnabled(false);

		graphTypeBox = new BBox(BRect(8, 138, 134, 198), "graphTypeBox");
		graphTypeBox->SetLabel("Curve Type");
		controlsBox->AddChild(graphTypeBox);

		linesRB = new BRadioButton(
			BRect(8, 16, 118, 32), "linesRB", "Straight Lines", new BMessage('Glin')
		);
		smoothRB =
			new BRadioButton(BRect(8, 34, 118, 50), "smoothRB", "Smooth Fit", new BMessage('Gfit'));
		graphTypeBox->AddChild(linesRB);
		graphTypeBox->AddChild(smoothRB);
		linesRB->SetValue(B_CONTROL_ON);

		gCurve = new CurveView(BRect(4, 4, 259, 259), "curve");
		graphBox->AddChild(gCurve);

		gammaS = new Slider(
			BRect(8, 206, 134, 224), 50, "Gamma", 0, 3, 0.01, new BMessage('gamC'), B_HORIZONTAL, 0,
			"%1.2f"
		);
		gammaS->SetValue(gCurve->Gamma());
		controlsBox->AddChild(gammaS);

		BButton* pc2mac =
			new BButton(BRect(8, 230, 68, 254), "pc2mac", "PC » Mac", new BMessage('gp2m'));
		BButton* mac2pc =
			new BButton(BRect(74, 230, 134, 254), "mac2pc", "Mac » PC", new BMessage('gm2p'));
		controlsBox->AddChild(pc2mac);
		controlsBox->AddChild(mac2pc);
	}

	virtual ~ColorCurvesView() {}

	virtual void MessageReceived(BMessage* msg);

	int Channel()
	{
		return (
			cspacePU->IndexOf(cspacePU->FindMarked()) * 4 + graphPU->IndexOf(graphPU->FindMarked())
		);
	}

	BBox* controlsBox;
	BBox* graphBox;
	BBox* typeBox;
	BBox* graphTypeBox;
	BMenuField* cspaceMF;
	BPopUpMenu* cspacePU;
	BMenuField* graphMF;
	BPopUpMenu* graphPU;
	BStringView* curcoords;
	BRadioButton *freeRB, *gammaRB;
	BRadioButton *linesRB, *smoothRB;
	Slider* gammaS;
	BButton *addB, *removeB;
};

ColorCurvesView* gView = 0;

void
CurveView::Draw(BRect update)
{
	int chan;
	SetLowColor(240, 240, 240, 255);
	FillRect(update, B_SOLID_LOW);
	SetHighColor(255, 0, 0, 255);
	if (fCurrent == 99)
		chan = 0;
	else
		chan = fCurrent;

	MovePenTo(BPoint(0, 255 - wct[chan][0]));
	for (int i = 1; i < 256; i++)
		StrokeLine(BPoint(i, 255 - wct[chan][i]));
	if (type != GT_GAMMA) {
		SetHighColor(Black);
		for (int i = 0; i < npoints[chan]; i++)
			StrokeRect(BRect(
				points[chan][i].x - 3, 252 - points[chan][i].y, points[chan][i].x + 3,
				258 - points[chan][i].y
			));
	}
}

void
CurveView::SetType(int t)
{
	type = t;
	FillTable(fCurrent);
	addon_preview();
	Invalidate();
}

void
CurveView::SetCurrent(int current)
{
	if (current == 99) // Lock RGB
	{
		// copy from R
		npoints[1] = npoints[0];
		npoints[2] = npoints[0];
		for (int i = 0; i < npoints[0]; i++) {
			points[1][i] = points[0][i];
			points[2][i] = points[0][i];
		}
		fGamma[1] = fGamma[0];
		fGamma[2] = fGamma[0];
	}
	fCurrent = current;
	FillTable(current);
	addon_preview();
	Invalidate();
}

void
CurveView::do_FillTable(int channel)
{
	//	printf ("Filling %d\n", channel);
	switch (type) {
	case GT_STRAIGHT: {
		for (int i = 0; i < npoints[channel] - 1; i++) {
			for (int j = points[channel][i].x; j <= points[channel][i + 1].x; j++)
				wct[channel][j] =
					(unsigned char)(float(points[channel][i + 1].y - points[channel][i].y) *
										(j - points[channel][i].x) /
										(points[channel][i + 1].x - points[channel][i].x) +
									points[channel][i].y);
		}
		break;
	}
	case GT_SMOOTH: {
		if (npoints[channel] == 2) {
			for (int j = 0; j <= 255; j++)
				wct[channel][j] =
					(unsigned char)(float(points[channel][1].y - points[channel][0].y) * j / 255 +
									points[channel][0].y);
		} else {
			float a, b, c, d;
			float yprime, ynprime;

			// First section: quadratic function
			//			ynprime = (float (points[1].y - points[0].y)/(points[1].x -
			// points[0].x)*(points[2].x - points[1].x)
			//					+ float (points[2].y - points[1].y)/(points[2].x -
			// points[1].x)*(points[1].x - points[0].x)) 					/ (points[2].x -
			// points[0].x);
			ynprime = float(points[channel][2].y - points[channel][0].y) /
					  (points[channel][2].x - points[channel][0].x);
			a = ynprime / points[channel][1].x -
				float(points[channel][1].y - points[channel][0].y) /
					(points[channel][1].x * points[channel][1].x);
			b = 2 * float(points[channel][1].y - points[channel][0].y) / points[channel][1].x -
				ynprime;
			c = points[channel][0].y;
			for (int j = 0; j <= points[channel][1].x; j++)
				wct[channel][j] = clipchar(a * j * j + b * j + c);

			// Middle sections: cubic
			for (int i = 1; i < npoints[channel] - 2; i++) {
				float xl = points[channel][i].x;
				float yl = points[channel][i].y;
				float xr = points[channel][i + 1].x;
				float yr = points[channel][i + 1].y;
				float xt = xr - xl;
				yprime = ynprime;
				//				ynprime = (float (yr - yl)/(xr - xl)*(points[i + 2].x - points[i +
				// 1].x)
				//						 + float (points[i + 2].y - yr)/(points[i + 2].x - points[i
				//+ 1].x)*xt) 						 / (xr - points[i - 1].x);
				ynprime = float(points[channel][i + 2].y - points[channel][i].y) /
						  (points[channel][i + 2].x - points[channel][i].x);
				a = (ynprime - yprime - 2 * (yr - yprime * xt - yl) / xt) / (xt * xt);
				b = (ynprime - yprime - 3 * a * xt * xt) / (2 * xt);
				c = yprime;
				d = points[channel][i].y;
				for (int j = points[channel][i].x; j <= points[channel][i + 1].x; j++) {
					float x = j - points[channel][i].x;
					wct[channel][j] = clipchar(a * x * x * x + b * x * x + c * x + d);
				}
			}

			// Last section: quadratic again
			float xt =
				points[channel][npoints[channel] - 1].x - points[channel][npoints[channel] - 2].x;
			a = float(
					points[channel][npoints[channel] - 1].y - ynprime * xt -
					points[channel][npoints[channel] - 2].y
				) /
				(xt * xt);
			b = ynprime;
			c = points[channel][npoints[channel] - 2].y;
			for (int j = points[channel][npoints[channel] - 2].x;
				 j <= points[channel][npoints[channel] - 1].x; j++) {
				float x = j - points[channel][npoints[channel] - 2].x;
				wct[channel][j] = clipchar(a * x * x + b * x + c);
			}
		}
		break;
	}
	case GT_GAMMA: {
		float gamma = Gamma();
		for (int i = 0; i < 256; i++) {
			float v = float(i) / 255.0;
			wct[channel][i] = uchar(255 * pow(v, gamma));
		}
		break;
	}
	default:
		fprintf(stderr, "Color Curves: Unknown graph type\n");
		break;
	}
}

void
CurveView::FillTable(int channel)
{
	if (channel == -1)
		channel = fCurrent;
	if (channel == 99) {
		do_FillTable(0);
		do_FillTable(1);
		do_FillTable(2);
	} else
		do_FillTable(channel);
}

void
CurveView::do_AddPoint(int channel)
{
	if (npoints[channel] == MAX_POINTS) {
		fprintf(stderr, "Color Curves: Too many control points\n");
		return;
	}
	if (npoints[channel] == 2) {
		added[channel][npoints[channel]++] = 1;
		points[channel][2] = points[channel][1];
		points[channel][1].x = (points[channel][2].x - points[channel][0].x) / 2;
		points[channel][1].y = (points[channel][2].y - points[channel][0].y) / 2;
	} else {
		// Find largest section and add the new point there
		int maxdist = 0;
		int ind = 0;
		for (int i = 0; i < npoints[channel] - 1; i++) {
			int dist = points[channel][i + 1].x - points[channel][i].x;
			if (dist > maxdist) {
				ind = i + 1;
				maxdist = dist;
			}
		}
		//		printf ("Max dist = %d at %d\n", maxdist, ind);
		added[channel][npoints[channel]++] = ind;
		for (int i = npoints[channel] - 1; i > ind; i--)
			points[channel][i] = points[channel][i - 1];

		points[channel][ind].x = (points[channel][ind + 1].x + points[channel][ind - 1].x) / 2;
		points[channel][ind].y = wct[channel][points[channel][ind].x];
		//		printf ("New point at (%d, %d)\n", points[ind].x, points[ind].y);
	}
}

void
CurveView::AddPoint()
{
	if (fCurrent == 99) {
		do_AddPoint(0);
		do_AddPoint(1);
		do_AddPoint(2);
	} else
		do_AddPoint(fCurrent);
	FillTable(fCurrent);
	addon_preview();
	Invalidate();
}

void
CurveView::do_RemovePoint(int channel)
{
	if (npoints[channel] == 2) {
		fprintf(stderr, "Color Curves: Trying to remove one of the last two control points\n");
		return;
	}
	int ind = added[channel][--npoints[channel]];
	for (int i = ind; i < npoints[channel]; i++)
		points[channel][i] = points[channel][i + 1];
}

void
CurveView::RemovePoint()
{
	if (fCurrent == 99) {
		do_RemovePoint(0);
		do_RemovePoint(1);
		do_RemovePoint(2);
	} else
		do_RemovePoint(fCurrent);
	FillTable(fCurrent);
	addon_preview();
	Invalidate();
}

void
CurveView::MouseDown(BPoint point)
{
	if (fCurrent == 99) {
		for (int i = 0; i < npoints[0]; i++) {
			BRect handle(
				points[0][i].x - 3, 252 - points[0][i].y, points[0][i].x + 3, 257 - points[0][i].y
			);
			if (handle.Contains(point)) {
				// Grabbed a handle, start tracking!
				uint32 buttons = 1;
				BPoint prev = point;
				while (buttons) {
					if (point != prev) {
						points[0][i].x = clipchar(point.x);
						points[0][i].y = clipchar(255 - point.y);
						points[1][i].x = clipchar(point.x);
						points[1][i].y = clipchar(255 - point.y);
						points[2][i].x = clipchar(point.x);
						points[2][i].y = clipchar(255 - point.y);

						if (i == 0) {
							points[0][i].x = 0;
							points[1][i].x = 0;
							points[2][i].x = 0;
						} else {
							if (points[0][i].x <= points[0][i - 1].x)
								points[0][i].x = points[0][i - 1].x + 1;
							if (points[1][i].x <= points[1][i - 1].x)
								points[1][i].x = points[1][i - 1].x + 1;
							if (points[2][i].x <= points[2][i - 1].x)
								points[2][i].x = points[2][i - 1].x + 1;
						}

						if (i == npoints[0] - 1) {
							points[0][i].x = 255;
							points[1][i].x = 255;
							points[2][i].x = 255;
						} else {
							if (points[0][i].x >= points[0][i + 1].x)
								points[0][i].x = points[0][i + 1].x - 1;
							if (points[1][i].x >= points[1][i + 1].x)
								points[1][i].x = points[1][i + 1].x - 1;
							if (points[2][i].x >= points[2][i + 1].x)
								points[2][i].x = points[2][i + 1].x - 1;
						}

						FillTable(fCurrent);
						addon_preview();
						Draw(Bounds());
						prev = point;
					}
					snooze(20000);
					GetMouse(&point, &buttons);
				}
			}
		}
	} else {
		for (int i = 0; i < npoints[fCurrent]; i++) {
			BRect handle(
				points[fCurrent][i].x - 3, 252 - points[fCurrent][i].y, points[fCurrent][i].x + 3,
				257 - points[fCurrent][i].y
			);
			if (handle.Contains(point)) {
				// Grabbed a handle, start tracking!
				uint32 buttons = 1;
				BPoint prev = point;
				while (buttons) {
					if (point != prev) {
						points[fCurrent][i].x = clipchar(point.x);
						points[fCurrent][i].y = clipchar(255 - point.y);

						if (i == 0)
							points[fCurrent][i].x = 0;
						else if (points[fCurrent][i].x <= points[fCurrent][i - 1].x)
							points[fCurrent][i].x = points[fCurrent][i - 1].x + 1;

						if (i == npoints[fCurrent] - 1)
							points[fCurrent][i].x = 255;
						else if (points[fCurrent][i].x >= points[fCurrent][i + 1].x)
							points[fCurrent][i].x = points[fCurrent][i + 1].x - 1;

						FillTable(fCurrent);
						addon_preview();
						Draw(Bounds());
						prev = point;
					}
					snooze(20000);
					GetMouse(&point, &buttons);
				}
			}
		}
	}
}

void
ColorCurvesView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'cspL':
		if (graphMF)
			graphBox->RemoveChild(graphMF);
		delete graphMF;
		graphMF = NULL;
		gCurve->SetCurrent(99);
		break;
	case 'cspR': {
		if (graphMF)
			graphBox->RemoveChild(graphMF);
		delete graphMF;
		graphPU = new BPopUpMenu("Graphed");
		BMenuItem* item;
		graphPU->AddItem(item = new BMenuItem("Red", new BMessage('RspR')));
		item->SetTarget(this);
		graphPU->AddItem(item = new BMenuItem("Green", new BMessage('RspG')));
		item->SetTarget(this);
		graphPU->AddItem(item = new BMenuItem("Blue", new BMessage('RspB')));
		item->SetTarget(this);
		graphPU->AddItem(item = new BMenuItem("Alpha", new BMessage('RspA')));
		item->SetTarget(this);
		graphPU->ItemAt(0)->SetMarked(true);
		graphMF = new BMenuField(BRect(68, 264, 148, 278), "graphMF", "", graphPU);
		graphMF->SetDivider(0);
		graphBox->AddChild(graphMF);
		gCurve->SetCurrent(0);
		break;
	}
	case 'cspC': {
		if (graphMF)
			graphBox->RemoveChild(graphMF);
		delete graphMF;
		graphPU = new BPopUpMenu("Graphed");
		BMenuItem* item;
		graphPU->AddItem(item = new BMenuItem("Cyan", new BMessage('CspC')));
		item->SetTarget(this);
		graphPU->AddItem(item = new BMenuItem("Magenta", new BMessage('CspM')));
		item->SetTarget(this);
		graphPU->AddItem(item = new BMenuItem("Yellow", new BMessage('CspY')));
		item->SetTarget(this);
		graphPU->AddItem(item = new BMenuItem("Black", new BMessage('CspK')));
		item->SetTarget(this);
		graphPU->ItemAt(0)->SetMarked(true);
		graphMF = new BMenuField(BRect(68, 264, 148, 278), "graphMF", "", graphPU);
		graphMF->SetDivider(0);
		graphBox->AddChild(graphMF);
		gCurve->SetCurrent(4);
		break;
	}
	case 'cspH': {
		if (graphMF)
			graphBox->RemoveChild(graphMF);
		delete graphMF;
		graphPU = new BPopUpMenu("Graphed");
		BMenuItem* item;
		graphPU->AddItem(
			item = new BMenuItem("Hue", new BMessage('HspH'))
		); // doesn't make sense but looks funny
		item->SetTarget(this);
		graphPU->AddItem(item = new BMenuItem("Saturation", new BMessage('HspS')));
		item->SetTarget(this);
		graphPU->AddItem(item = new BMenuItem("Value", new BMessage('HspV')));
		item->SetTarget(this);
		graphPU->ItemAt(0)->SetMarked(true);
		item->SetTarget(this);
		graphMF = new BMenuField(BRect(68, 264, 148, 278), "graphMF", "", graphPU);
		graphMF->SetDivider(0);
		graphBox->AddChild(graphMF);
		gCurve->SetCurrent(8);
		break;
	}
	case 'RspR':
		gCurve->SetCurrent(0);
		gammaS->SetValue(gCurve->Gamma(0));
		break;
	case 'RspG':
		gCurve->SetCurrent(1);
		gammaS->SetValue(gCurve->Gamma(1));
		break;
	case 'RspB':
		gCurve->SetCurrent(2);
		gammaS->SetValue(gCurve->Gamma(2));
		break;
	case 'RspA':
		gCurve->SetCurrent(3);
		gammaS->SetValue(gCurve->Gamma(3));
		break;
	case 'CspC':
		gCurve->SetCurrent(4);
		gammaS->SetValue(gCurve->Gamma(4));
		break;
	case 'CspM':
		gCurve->SetCurrent(5);
		gammaS->SetValue(gCurve->Gamma(5));
		break;
	case 'CspY':
		gCurve->SetCurrent(6);
		gammaS->SetValue(gCurve->Gamma(6));
		break;
	case 'CspK':
		gCurve->SetCurrent(7);
		gammaS->SetValue(gCurve->Gamma(7));
		break;
	case 'HspH':
		gCurve->SetCurrent(8);
		gammaS->SetValue(gCurve->Gamma(8));
		break;
	case 'HspS':
		gCurve->SetCurrent(9);
		gammaS->SetValue(gCurve->Gamma(9));
		break;
	case 'HspV':
		gCurve->SetCurrent(10);
		gammaS->SetValue(gCurve->Gamma(10));
		break;
	case 'Tfre':
		linesRB->SetEnabled(true);
		smoothRB->SetEnabled(true);
		addB->SetEnabled(gCurve && gCurve->NPoints() < MAX_POINTS - 1);
		removeB->SetEnabled(gCurve && gCurve->NPoints() > 2);
		if (linesRB->Value())
			gCurve->SetType(GT_STRAIGHT);
		else
			gCurve->SetType(GT_SMOOTH);
		break;
	case 'Tgam':
		linesRB->SetEnabled(false);
		smoothRB->SetEnabled(false);
		addB->SetEnabled(false);
		removeB->SetEnabled(false);
		gCurve->SetType(GT_GAMMA);
		break;
	case 'addP':
		if (gCurve)
			gCurve->AddPoint();
		addB->SetEnabled(gCurve && gCurve->NPoints() < MAX_POINTS - 1);
		removeB->SetEnabled(gCurve && gCurve->NPoints() > 2);
		break;
	case 'delP':
		if (gCurve)
			gCurve->RemovePoint();
		addB->SetEnabled(gCurve && gCurve->NPoints() < MAX_POINTS - 1);
		removeB->SetEnabled(gCurve && gCurve->NPoints() > 2);
		break;
	case 'Glin':
		if (gCurve)
			gCurve->SetType(GT_STRAIGHT);
		break;
	case 'Gfit':
		if (gCurve)
			gCurve->SetType(GT_SMOOTH);
		break;
	case 'gamC': {
		float gamma = msg->FindFloat("value");
		gCurve->SetGamma(gamma);
		gCurve->FillTable();
		addon_preview();
		gCurve->Invalidate();
		break;
	}
	case 'gp2m':
		gammaS->SetValue(MAC_GAMMA);
		gCurve->SetGamma(MAC_GAMMA);
		gCurve->FillTable();
		addon_preview();
		gCurve->Invalidate();
		break;
	case 'gm2p':
		gammaS->SetValue(1.0 / MAC_GAMMA);
		gCurve->SetGamma(1.0 / MAC_GAMMA);
		gCurve->FillTable();
		addon_preview();
		gCurve->Invalidate();
		break;
	default:
		BView::MessageReceived(msg);
	}
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Color Curves");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1999-2001 ∑ Sum Software");
	strcpy(info->description, "Adjust Colors (gamma correction)");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 0;
	info->release = 5;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = true;
	return B_OK;
}

status_t
addon_exit(void)
{
	return B_OK;
}

status_t
addon_close(void)
{
	return B_OK;
}

status_t
addon_make_config(BView** view, BRect rect)
{
	gView = new ColorCurvesView(rect);
	*view = gView;
	return B_OK;
}

#define wct gCurve->wct

status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* /* frame */, bool final, BPoint /* point */, uint32 /* buttons */
)
{
	int error = ADDON_OK;
	BRect bounds = inLayer->Bounds();

	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer(*inLayer);
	if (mode == M_SELECT) {
		if (inSelection)
			*outSelection = new Selection(*inSelection);
		else
			return (0);
	}
	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();

	int cspace = gView->cspacePU->IndexOf(gView->cspacePU->FindMarked());
	//	printf ("cspace = %d\n", cspace);

	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;
	grey_pixel* mapbits = NULL;
	uint32 mbpr = 0;
	uint32 mdiff = 0;
	if (inSelection) {
		mapbits = (grey_pixel*)inSelection->Bits();
		mbpr = inSelection->BytesPerRow();
		mdiff = mbpr - w;
	}

	if (final)
		addon_start();
	float delta = 100.0 / h; // For the Status Bar.

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* sbits = (bgra_pixel*)inLayer->Bits();
		bgra_pixel* dbits = (bgra_pixel*)(*outLayer)->Bits();

		for (uint32 y = 0; y < h; y++) {
			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}
#if __INTEL__

			// use faster asm version
			if (!inSelection && (cspace == 0 || cspace == 3)) {
				uint32 table[4 * 256];
				// fill the table
				for (uint32 t = 0; t < 256; t++) {
					table[t] = PIXEL(0, 0, 0, wct[3][t]);
					table[t + 256] = PIXEL(wct[0][t], 0, 0, 0);
					table[t + 512] = PIXEL(0, wct[1][t], 0, 0);
					table[t + 768] = PIXEL(0, 0, wct[2][t], 0);
				}
				// printf ("("); fflush (stdout);
				gamma_x86(sbits, dbits, table, w * h);
				// printf (")"); fflush (stdout);
			} else {
#endif
				for (uint32 x = 0; x < w; x++) {
					if (!inSelection || *(++mapbits)) {
						if (cspace == 0 || cspace == 3) // RGBA
						{
							bgra_pixel pixel = *(++sbits);
							*(++dbits) = PIXEL(
								wct[0][RED(pixel)], wct[1][GREEN(pixel)], wct[2][BLUE(pixel)],
								wct[3][ALPHA(pixel)]
							);
						} else if (cspace == 1) // CMYK
						{
							uint32 p = *(++sbits);
							cmyk_pixel pixel = bgra2cmyk(p);
							bgra_pixel bgra = cmyk2bgra(PIXEL(
								wct[6][YELLOW(pixel)], wct[5][MAGENTA(pixel)], wct[4][CYAN(pixel)],
								wct[7][BLACK(pixel)]
							));
							*(++dbits) = (bgra & COLOR_MASK) | (p & ALPHA_MASK);
						} else // HSV
						{
							uint32 p = *(++sbits);
							hsv_color c = bgra2hsv(p);
							c.hue = float(wct[8][int(c.hue / 360.0 * 255)]) * 360 / 255;
							c.saturation = float(wct[9][int(c.saturation * 255)]) / 255.0;
							c.value = float(wct[10][int(c.value * 255)]) / 255.0;
							*(++dbits) = (hsv2bgra(c) & COLOR_MASK) | (p & ALPHA_MASK);
						}
					} else
						*(++dbits) = *(++sbits);
				}
				mapbits += mdiff;
			}
#if __INTEL__
		}
#endif
		break;
	}
	case M_SELECT: {
		if (!inSelection)
			*outSelection = NULL;
		else {
			grey_pixel* sbits = mapbits;
			grey_pixel* dbits = (grey_pixel*)(*outSelection)->Bits();

			for (uint32 y = 0; y < h; y++) {
				if (final) {
					addon_update_statusbar(delta);
					if (addon_stop()) {
						error = ADDON_ABORT;
						break;
					}
				}
				sbits = (grey_pixel*)inSelection->Bits() + y * mbpr;
				dbits = (grey_pixel*)(*outSelection)->Bits() + y * mbpr;
				for (uint32 x = 0; x < w; x++) {
					*(++dbits) = *(++sbits);
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
