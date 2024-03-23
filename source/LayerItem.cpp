#include "LayerItem.h"
#include "LayerView.h"
#include "Becasso.h"
#include "Colors.h"
#include "LayerNameWindow.h"
#include "DModes.h"
#include "Slider.h"
#include <MenuField.h>
#include "Settings.h"

LayerItem::LayerItem(BRect frame, const char* name, int layerIndex, CanvasView* _myView)
	: BView(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS)
{
	index = layerIndex;
	fMyView = _myView;
	fLayer = fMyView->getLayer(index);
	BRect canvasRect = fMyView->canvasFrame();
	BMessage* msg = new BMessage('cbHd');
	msg->AddInt32("index", index);
	fThumbVSize = min_c(THUMBLAYERMAXHEIGHT, canvasRect.Height());
	fThumbHSize = min_c(canvasRect.Width() / canvasRect.Height() * fThumbVSize, THUMBLAYERMAXWIDTH);
	hide = new BCheckBox(
		BRect(fThumbHSize + 8, 2, fThumbHSize + 50, 24), "hide", lstring(164, "Hide"), msg
	);
	AddChild(hide);
	hide->SetTarget(fMyView->Window());
	// hide->SetDrawingMode (B_OP_OVER);
	hide->SetValue(fLayer->IsHidden());
	msg = new BMessage('LIga');
	msg->AddInt32("index", index);
	Slider* ga = new Slider(
		BRect(fThumbHSize + 54, 4, LAYERITEMWIDTH - 4, 20), 0, "", 0, 255, 1, msg, B_HORIZONTAL
	);
	ga->SetValue(fLayer->getGlobalAlpha());
	AddChild(ga);
	SetHighColor(Black);
	if (index == fMyView->currentLayerIndex())
		SetLowColor(DarkGrey);
	else
		SetLowColor(LightGrey);
	SetViewColor(LowColor());
	hide->SetViewColor(LowColor());
	fModePU = new BPopUpMenu("");
	msg = new BMessage('DMch');
	msg->AddInt32("index", index);
	msg->AddInt32("newmode", DM_BLEND);
	BMenuItem* item = new BMenuItem(lstring(165, "Blend"), msg);
	item->SetTarget(fMyView->Window());
	fModePU->AddItem(item);
	msg = new BMessage('DMch');
	msg->AddInt32("index", index);
	msg->AddInt32("newmode", DM_MULTIPLY);
	item = new BMenuItem(lstring(166, "Multiply"), msg);
	item->SetTarget(fMyView->Window());
	fModePU->AddItem(item);
	BMenuField* mode = new BMenuField(
		BRect(fThumbHSize + 8, 22, fThumbHSize + 140, 34), "mode", lstring(167, "Operation:"),
		fModePU
	);
	fModePU->ItemAt(fLayer->getMode())->SetMarked(true);
	mode->SetDivider(58);
	AddChild(mode);
	click = 1;
	get_click_speed(&dcspeed);
}

LayerItem::~LayerItem() {}

void
LayerItem::Refresh(Layer* layer)
{
	fLayer = layer;
	DrawThumbOnly();
}

void
LayerItem::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (message && message->what == 'ldrg') {
		int16 to = int16(point.y / LAYERITEMHEIGHT + 0.5);
		if (transit == B_EXITED_VIEW) {
			SetHighColor(Black);
			StrokeLine(Bounds().LeftTop(), Bounds().RightTop());
			StrokeLine(Bounds().LeftBottom(), Bounds().RightBottom());
		} else if (to == 0) {
			SetHighColor(Blue);
			StrokeLine(Bounds().LeftTop(), Bounds().RightTop());
			SetHighColor(Black);
			StrokeLine(Bounds().LeftBottom(), Bounds().RightBottom());
		} else {
			SetHighColor(Black);
			StrokeLine(Bounds().LeftTop(), Bounds().RightTop());
			SetHighColor(Blue);
			StrokeLine(Bounds().LeftBottom(), Bounds().RightBottom());
		}
		Window()->UpdateIfNeeded();
	}
}

void
LayerItem::select(bool sel)
{
	SetLowColor(sel ? DarkGrey : LightGrey);
	SetViewColor(sel ? DarkGrey : LightGrey);
	Draw(Bounds());
	for (int i = 0; i < CountChildren(); i++) {
		BView* v = ChildAt(i);
		v->SetLowColor(sel ? DarkGrey : LightGrey);
		v->SetViewColor(sel ? DarkGrey : LightGrey);
		v->FillRect(Bounds(), B_SOLID_LOW);
		v->Draw(Bounds());
		for (int j = 0; j < v->CountChildren(); j++) {
			v->ChildAt(j)->SetLowColor(sel ? DarkGrey : LightGrey);
			v->ChildAt(j)->Draw(Bounds());
		}
	}
	Sync();
}

void
LayerItem::MouseDown(BPoint point)
{
	//	extern Becasso *mainapp;
	uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");
	//	BPoint prevpoint = point;
	//	uint32 clicks = Window()->CurrentMessage()->FindInt32 ("clicks");
	if (click != 2 && buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY)) {
		// Immediate feedback: Draw selected state
		select(true);
		if (LayerView* lv = dynamic_cast<LayerView*>(Parent())) {
			LayerItem* sibling = lv->getLayerItem(fMyView->currentLayerIndex());
			sibling->select(false);
		}
		BMessage msg('lSel');
		msg.AddInt32("index", index);
		fMyView->Window()->PostMessage(&msg);

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
		if (!bt && click != 2) {
			click = 1;
		} else if (click != 2 && buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY)) {
			BMessage* dragmessage = new BMessage('ldrg');
			dragmessage->AddInt32("startingindex", index);
			BBitmap* dragbitmap = new BBitmap(Bounds(), B_RGBA32, true);
			dragbitmap->Lock();
			BPoint origin = Frame().LeftTop();
			LockLooper();
			BView* parent = Parent();
			parent->RemoveChild(this);
			dragbitmap->AddChild(this);
			MoveTo(B_ORIGIN);
			select(true);
			dragbitmap->RemoveChild(this);
			MoveTo(origin);
			parent->AddChild(this);
			UnlockLooper();

			uint32* bits = (uint32*)dragbitmap->Bits();
			for (uint32 i = 0; i < dragbitmap->BitsLength() / 4; i++) {
				uint32 pixel = *bits;
				*bits++ = (pixel & COLOR_MASK) | (127 << ALPHA_BPOS);
			}
			DragMessage(dragmessage, dragbitmap, B_OP_ALPHA, point);
			delete dragmessage;
			click = 1;
		}
		if (click == 2 || buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY) {
			click = 1;
			BPoint lnwpos = ConvertToScreen(Bounds().LeftTop());
			Window()->Unlock();
			LayerNameWindow* lnw = new LayerNameWindow(fLayer->getName());
			lnw->MoveTo(lnwpos);
			if (lnw->Go()) {
				fLayer->setName(lnw->name());
				BMessage* msg = new BMessage('lNch');
				msg->AddInt32("index", index);
				fMyView->Window()->PostMessage(msg);
				delete msg;
			}
			lnw->Lock();
			lnw->Quit();
			Window()->Lock();
			Draw(Bounds());
			return;
		}
	}
}

void
LayerItem::Draw(BRect /* update */)
//  Note: I'm not happy with this implementation.  LayerItems should definitaly
//        cache the thumbnail bitmaps.  However, this works and updates in realtime.
{
	BRect canvasRect = fMyView->canvasFrame();
	BRect thumbRect = BRect(0, 0, fThumbHSize, fThumbVSize);
	BBitmap* thumbnail = new BBitmap(thumbRect, B_RGBA32, true);
	BView* thumbView = new BView(thumbRect, "Layer Thumbnail", uint32(NULL), uint32(NULL));
	thumbnail->Lock();
	thumbnail->AddChild(thumbView);
	thumbView->DrawBitmap(fLayer, canvasRect, thumbRect);
	thumbView->Sync();
	uint32* bits = ((uint32*)thumbnail->Bits()) - 1;
	for (int i = 0; i < thumbnail->BitsLength() / 4; i++) {
		register uint32 pixel = *(++bits);
#if defined(__POWERPC__)
		register uint32 a = pixel & 0x000000FF;
		register uint32 b =
			(((pixel & 0xFF000000) >> 8) * a - (a << 24) + (255 << 24)) & 0xFF000000;
		register uint32 g =
			(((pixel & 0x00FF0000) >> 8) * a - (a << 16) + (255 << 16)) & 0x00FF0000;
		register uint32 r = (((pixel & 0x0000FF00) >> 8) * a - (a << 8) + (255 << 8)) & 0x0000FF00;
		*bits = (b | g | r | a);
#else
		register uint32 a = pixel >> 24;
		register uint32 b = ((((pixel & 0x000000FF) * a) >> 8) - (a) + (255)) & 0x000000FF;
		register uint32 g =
			((((pixel & 0x0000FF00) * a) >> 8) - (a << 8) + (255 << 8)) & 0x0000FF00;
		register uint32 r =
			((((pixel & 0x00FF0000) * a) >> 8) - (a << 16) + (255 << 16)) & 0x00FF0000;
		*bits = (b | g | r | (a << 24));
#endif
	}
	thumbRect.OffsetTo(4, 4);
	FillRect(Bounds(), B_SOLID_LOW);
	DrawBitmapAsync(thumbnail, thumbRect.LeftTop());
	DrawString(
		fLayer->getName(), BPoint(thumbRect.right + 8, thumbRect.bottom - 4)
	); // Should be dynamic...
	StrokeRect(Bounds());
	Sync();
	thumbnail->RemoveChild(thumbView);
	delete thumbView;
	delete thumbnail;
}

void
LayerItem::DrawThumbOnly()
{
	BRect canvasRect = fMyView->canvasFrame();
	fThumbVSize = min_c(THUMBLAYERMAXHEIGHT, canvasRect.Height());
	fThumbHSize = min_c(canvasRect.Width() / canvasRect.Height() * fThumbVSize, THUMBLAYERMAXWIDTH);
	BRect thumbRect = BRect(0, 0, fThumbHSize, fThumbVSize);
	BBitmap* thumbnail = new BBitmap(thumbRect, B_RGBA32, true);
	BView* thumbView = new BView(thumbRect, "Layer Thumbnail", uint32(NULL), uint32(NULL));
	thumbnail->Lock();
	thumbnail->AddChild(thumbView);
	thumbView->DrawBitmap(fLayer, canvasRect, thumbRect);
	thumbView->Sync();
	uint32* bits = ((uint32*)thumbnail->Bits()) - 1;
	for (int i = 0; i < thumbnail->BitsLength() / 4; i++) {
		register uint32 pixel = *(++bits);
#if defined(__POWERPC__)
		register uint32 a = pixel & 0x000000FF;
		register uint32 b =
			(((pixel & 0xFF000000) >> 8) * a - (a << 24) + (255 << 24)) & 0xFF000000;
		register uint32 g =
			(((pixel & 0x00FF0000) >> 8) * a - (a << 16) + (255 << 16)) & 0x00FF0000;
		register uint32 r = (((pixel & 0x0000FF00) >> 8) * a - (a << 8) + (255 << 8)) & 0x0000FF00;
		*bits = (b | g | r | a);
#else
		register uint32 a = pixel >> 24;
		register uint32 b = (((pixel & 0x000000FF) * a >> 8) - (a) + (255)) & 0x000000FF;
		register uint32 g = (((pixel & 0x0000FF00) * a >> 8) - (a << 8) + (255 << 8)) & 0x0000FF00;
		register uint32 r =
			(((pixel & 0x00FF0000) * a >> 8) - (a << 16) + (255 << 16)) & 0x00FF0000;
		*bits = (b | g | r | (a << 24));
#endif
	}
	thumbRect.OffsetTo(4, 4);
	DrawBitmap(thumbnail, thumbRect.LeftTop());
	Sync();
	thumbnail->RemoveChild(thumbView);
	delete thumbView;
	delete thumbnail;
}

void
LayerItem::MessageReceived(BMessage* message)
{
	if (message->WasDropped() && message->what == 'ldrg') {
		BPoint point;
		point = ConvertFromScreen(message->DropPoint());
		int32 sindex;
		message->FindInt32("startingindex", &sindex);
		if (sindex == index) {
			BMessage* msg = new BMessage('lSel');
			msg->AddInt32("index", index);
			fMyView->Window()->PostMessage(msg);
			delete msg;
		} else {
			BMessage* msg = new BMessage('movL');
			int16 to = int16(floor(point.y / LAYERITEMHEIGHT - 0.5) + (sindex > index ? 0 : 1));
			msg->AddInt32("from", sindex);
			msg->AddInt32("to", index - to);
			fMyView->Window()->PostMessage(msg);
			delete msg;
		}
	} else
		inherited::MessageReceived(message);
}