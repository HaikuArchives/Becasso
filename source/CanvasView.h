#ifndef CANVASVIEW_H
#define CANVASVIEW_H

#include <View.h>
#include <Rect.h>
#include <ScrollBar.h>
#include <Bitmap.h>
#include <Message.h>
#if defined(DATATYPES)
#include <Datatypes.h>
#endif
#include <Point.h>
#include <Polygon.h>
#include <TextControl.h>
#include <Message.h>
#include <stdio.h>
#include "AttribDraw.h"
#include "PointStack.h"
#include "SView.h"
#include "AddOn.h"
#include "undo_entry.h"
#include "Position.h"
#include "Settings.h"
#include "Colors.h"
#include <NumberFormat.h>

#define STRETCH_TO_FIT 0

class Brush;
class Layer;
class Selection;
class BGView;

typedef struct
{
	float cx, cy;
	float Spacing;
	int pstrength;
	Brush* b;
	BBitmap* brushbm;
	BPoint pos;
} brush_cache;

typedef struct
{
	float cx, cy;
	float Spacing;
	int pstrength;
	Brush* b;
	BPoint pos;
	BPoint offset;
	BPoint prevctr;
} clone_cache;

typedef struct
{
	const color_map* cmap;
	rgb_color hi, lo, hit, lot;
	bool lighten;
	float flowrate;
	float color_ratio;
	float sigma;
	float ssq;
	int strength;
	Position position;
} spraycan_cache;

class CanvasView : public SView
{
  public:
	CanvasView(const BRect frame, const char* name, BBitmap* map = NULL, rgb_color color = White);
	CanvasView(const BRect frame, const char* name, FILE* fp);
	void RestOfCtor();
	virtual ~CanvasView();
	virtual void FrameResized(float width, float height);
	virtual void Draw(BRect updateRect);
	virtual void Invalidate(const BRect rect);
	virtual void Invalidate();
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* msg);
	virtual void MouseDown(BPoint point);
	virtual void MouseUp(BPoint point);
	virtual void MouseOrTablet(Position* position);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void Pulse();
	virtual void WindowActivated(bool active);
	virtual void AttachedToWindow();
	virtual void ScreenChanged(BRect rect, color_space mode);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);
	virtual void MessageReceived(BMessage* msg);
	void SetupUndo(int32 mode);
	void Undo(bool advance = true, bool menu = false);
	void Redo(bool menu = false);
	void Cut();
	void Copy();
	void CopyToNewLayer();
	void Paste(bool winAct = false);
	void SelectAll(bool menu = false);
	void UndoSelection(bool menu = false);
	void ChannelToSelection(uint32 what, bool menu = false);
	void SelectionToChannel(uint32 what);
	rgb_color GuessBackgroundColor();
	void SelectByColor(rgb_color c, bool menu = false);
	void ColorizeSelection(rgb_color c);
	void InvertSelection(bool menu = false);

	rgb_color getColor(long x, long y);
	rgb_color getColor(BPoint p);
	rgb_color getColor(LPoint p);
	uint32 get_and_plot(BPoint p, rgb_color c = Black);
	void plot(BPoint p, rgb_color c = Black);
	void plot_alpha(BPoint p, rgb_color c = Black);
	void fplot(BPoint p, rgb_color c = Black);
	void fplot_alpha(BPoint p, rgb_color c = Black);
	void tplot32(BPoint p, rgb_color c = Black);
	void tplot8(LPoint p, uchar c = 255);
	void setScrollBars(BScrollBar* _h, BScrollBar* _v);

	BRect canvasFrame() { return fCanvasFrame; };

	BBitmap* canvas(bool do_export = false);

	Selection* getSelection() { return selection; };

	void
	ConstructCanvas(BBitmap* cvCanvas, BRect rect, bool doselect = true, bool do_export = false);
	void Print();
	void
	Merge(BBitmap* a, Layer* b, BRect update, bool doselect = false, bool preserve_alpha = true);

	Layer* currentLayer() { return layer[fCurrentLayer]; };

	Layer* getLayer(int index) { return layer[index]; };

	int numLayers() { return fNumLayers; };

	int currentLayerIndex() { return fCurrentLayer; };

	void addLayer(const char* name);
	void insertLayer(int index, const char* name);
	void duplicateLayer(int index);
	void moveLayers(int a, int b);
	void removeLayer(int index);
	void makeCurrentLayer(int index);
	void mergeLayers(int bottom, int top);
	void translateLayer(int index);
	void rotateLayer(int index);
	void flipLayer(int hv, int index);
	void checkMetaLayer();

	void setChannelOperation(int index, int newmode);
	void setGlobalAlpha(int index, int ga);
	void setLayerContentsToBitmap(int index, BBitmap* map, int32 resize = STRETCH_TO_FIT);
	void invertAlpha();
	bool changed;
	void tTextD();
	void Save(BEntry entry);
	void setScale(float s, bool invalidate = true);

	float getScale() { return fScale; };

	void ZoomIn();
	void ZoomOut();

	void setBGView(BGView* bg) { fBGView = bg; };

	BGView* getBGView() { return fBGView; };

	status_t Crop(BRect rect);
	void CenterMouse();
	void CropToWindow();
	void CropToSelection();
	void AutoCrop();
	void Pad(uint32 what);
	void resizeTo(int32 w, int32 h);
	void ResizeToWindow(uint32 what);
	void RotateCanvas(uint32 what);
	void ReplaceCurrentLayer(BBitmap* newlayer);
	void InvalidateAddons();
	void OpenAddon(AddOn* addon, const char* name);
	void CloseAddon(AddOn* addon);
	void Filter(AddOn* addon, uint8 preview = 0);
	void Transform(AddOn* addon, uint8 preview = 0);
	void Generate(AddOn* addon, uint8 preview = 0);
	void WriteAsHex(const char* fname);

	AddOn* FilterOpen() { return filterOpen; };

	AddOn* TransformerOpen() { return transformerOpen; };

	AddOn* GeneratorOpen() { return generatorOpen; };

	void Fill(int32 mode, BPoint point, rgb_color* c);

	void SimpleData(BMessage* msg);
	void CopyTarget(BMessage* msg);
	void TrashTarget(BMessage* msg);

	void ExportCstruct(const char* fname);

	////////
  private:
	typedef SView inherited;
	void FastAddWithAlpha(long x, long y, int strength = 255);
	void FastBlendWithAlpha(long x, long y, int strength = 255);
	void ePaste(BPoint point, uint32 buttons);
	void ePasteM(BPoint point);
	void eDrag(BPoint point, uint32 buttons);
	void eFilter(BPoint point, uint32 buttons);
	void eGenerate(BPoint point, uint32 buttons);
	void eTransform(BPoint point, uint32 buttons);
	void tBrush(int32 mode, BPoint point, uint32 buttons);
	void tBrushM(int32 mode, BPoint point, uint32 buttons, int pressure, BPoint tilt);
	void tClone(int32 mode, BPoint point, uint32 buttons);
	void tCloneM(int32 mode, BPoint point, uint32 buttons, int pressure, BPoint tilt);
	void tTablet(int32 mode);
	void tEraser(int32 mode, BPoint point, uint32 buttons);
	void tEraserM(int32 mode, BPoint point);
	void tText(int32 mode, BPoint point, uint32 buttons);
	void tTextM(int32 mode, BPoint point);
	void tSpraycan(int32 mode, BPoint point, uint32 buttons);
	void tSpraycanM(int32 mode, BPoint point, uint32 buttons, int pressure, BPoint tilt);
	void tFreehand(int32 mode, BPoint point, uint32 buttons);
	void tLines(int32 mode, BPoint point, uint32 buttons);
	void tLinesM(int32 mode, BPoint point);
	void tPolyblob(int32 mode, BPoint point, uint32 buttons);
	void tPolygon(int32 mode, BPoint point, uint32 buttons);
	void tPolygonM(int32 mode, BPoint point);
	void tRect(int32 mode, BPoint point, uint32 buttons);
	void tRectM(int32 mode, BPoint point);
	void tRoundRect(int32 mode, BPoint point, uint32 buttons);
	void tRoundRectM(int32 mode, BPoint point);
	void tCircle(int32 mode, BPoint point, uint32 buttons);
	void tCircleM(int32 mode, BPoint point);
	void tEllipse(int32 mode, BPoint point, uint32 buttons);
	void tEllipseM(int32 mode, BPoint point);
	void tFill(int32 mode, BPoint point, uint32 buttons, rgb_color* c = NULL);
	void tSetColors(uint32 buttons);

	bool isfillcolor0(LPoint point);
	bool isfillcolorrgb(LPoint point, uchar* t);
	bool isfillcolort(LPoint point, uchar* t);
	bool inbounds(LPoint point);
	bool istransparent8(LPoint point);

	void ScrollIfNeeded(BPoint point);
	BRect makePositive(BRect r);
	BRect PRect(float l, float t, float r, float b);

	void DetachCurrentLayer();
	void AttachCurrentLayer();
	void DetachSelection();
	void AttachSelection();
	void CloseOpenAddons();

	void do_removeLayer(int index);
	void do_translateLayer(int index, int32 mode);
	void do_rotateLayer(int index, int32 mode);

	class CanvasWindow* myWindow;
	BGView* fBGView;
	int32 entry;
	int32 prevTool;
	bool mouse_on_canvas;
	BScrollBar* hSB;
	BScrollBar* vSB;
	BRect fCanvasFrame;
	BBitmap* screenbitmap;
	BBitmap* screenbitmap32;
	int bwi, bhi;
	ulong* bbitsl;
	uchar* bbits;
	ulong bbprl, bbpr;
	uint32* sbptr;
	uint32 sblpr;
	undo_entry undo[MAX_UNDO];
	int fIndex1, fIndex2;
	int indexUndo, maxIndexUndo, maxUndo;
	Layer* layer[MAX_LAYERS];
	int fCurrentLayer, fNumLayers;
	Layer* previewLayer;
	Selection* previewSelection;
	Layer* tr2x2Layer;
	Selection* tr2x2Selection;
	int didPreview;
	bool transformerSaved, generatorSaved;
	Selection* selection;
	BBitmap* cutbg;
	SView* cutView;
	BRect prevPaste, prev_eraser;
	SView* selectionView;
	bool sel, selchanged, firstDrag, first, inAddOn;
	BBitmap* temp;
	uchar* tbits;
	ulong tbprl, tbpr;
	float tw, th;
	int twi, thi;
	SView* drawView;
	BPolygon* polygon;
	BPoint polypoint;
	rgb_color fillcolor;
	uint32 fill32;
	rgb_color toleranceRGB;
	float tolerance;
	BPoint prev;
	float pradius;
	BTextControl* text;
	uint32 fButtons;
	int32 fMode;
	BPoint fPoint;
	float fScale;
	bool leftfirst;
	bool windowLock; // I don't believe this either!
	bool newWin;
	BRect fPreviewRect;
	BRect fPrevpreviewRect;
	AddOn* filterOpen;
	AddOn* transformerOpen;
	AddOn* generatorOpen;
	// AddOn		*addonInQueue;
	int32 fCurrentProperty;
	int32 fLayerSpecifier;
	float fTreshold;
	bool fMouseDown;
	bool fPicking;
	bool fDragScroll;
	int fTranslating;
	int fRotating;
	brush_cache fBC;
	spraycan_cache fSC;
	clone_cache fCC;
	BPoint fLastCenter;
	BNumberFormat fNumberFormat;
};

#endif
