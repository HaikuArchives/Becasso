#ifndef POSVIEW_H
#define POSVIEW_H

#include <View.h>
#include <Rect.h>
#include <Point.h>
#include <NumberFormat.h>
#include <Message.h>
#include "CanvasView.h"
#include "Colors.h"

#define POSWIDTH 130

class PosView : public BView {
  public:
	PosView(const BRect frame, const char* name, CanvasView* _canvas);
	virtual void Draw(BRect updaterect);
	virtual void Pulse();

	void SetPoint(const int x, const int y);
	void DoRadius(const bool r);
	void SetTextLayer(const bool t = true);

  private:
	typedef BView inherited;
	CanvasView* canvas;
	int mouse_x;
	int mouse_y;
	int set_x;
	int set_y;
	int delta_x;
	int delta_y;
	float radius;
	bool do_radius;
	bool is_textlayer;
	BNumberFormat fNumberFormat;
	BPoint prev;
};

#endif
