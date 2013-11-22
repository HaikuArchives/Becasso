#include "DragView.h"
#include <stdio.h>

const ulong _GO_AWAY = 0;
const ulong _DRAG = 1;

long MouseThread(void* data);

long MouseThread(void* data) {
	DragView* This = (DragView*)data;
	// Curly braces there due to a bug in mwcc (??)
	BRect goAwayRect;
	if (This->orientation == 0) goAwayRect.Set(3,3,16,14);
	else goAwayRect.Set(3,3,16,14);
	ulong mouseDown;
	BPoint where, old_where;

	while(1) {
		// printf("Thread Resumed...\n");
		This->Window()->Lock();
		This->GetMouse(&where,&mouseDown);
	
		// If going away
		switch (This->mouse_status) {

			case _GO_AWAY:
			
				while (mouseDown) {
					This->GetMouse(&where,&mouseDown);
					if (goAwayRect.Contains(where)) {
						if (This->goAwayState == 1) {
							This->goAwayState = 0;
							This->Draw(goAwayRect);
						}
					}
					else if (This->goAwayState != 1) {
						This->goAwayState = 1;
						This->Draw(goAwayRect);
					}
					snooze(20000);
				}
				break;

			case _DRAG:

				old_where = where;
				while (mouseDown) {
					This->GetMouse(&where, &mouseDown);
					if (old_where != where) {
						This->Window()->MoveBy(where.x - old_where.x, where.y - old_where.y);
						This->Draw(BRect(0,0,15,13));
						BView *contents = This->NextSibling();
						if (contents)
						{
							contents->Draw (contents->Bounds());
						}
					}
					snooze(20000);
				}
				break;

			default:
			
				break;
		}
		This->Window()->Unlock();
		if (This->goAwayState == 0)
			This->Window()->PostMessage(B_QUIT_REQUESTED);
		suspend_thread(find_thread(NULL));
	}
}

DragView::DragView(BRect frame, const char* Title, int Orientation = DV_HORZ) 
	: BView(frame, Title, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	thMouseID = -1;
	active = 1;
	
	BRect goAwayRect(0,0,10,10);
	goAwayUp = new BBitmap(goAwayRect, B_COLOR_8_BIT);
	goAwayDown = new BBitmap(goAwayRect, B_COLOR_8_BIT);
	
	goAwayUp->SetBits(goAway_raw,144,0,B_COLOR_8_BIT);
	goAwayDown->SetBits(goAway_raw2,144,0,B_COLOR_8_BIT);

	orientation = Orientation;
	goAwayState = 1;
}

DragView::~DragView()
{
	suspend_thread (thMouseID);
}

void DragView::Draw(BRect updateRect)
{
	#pragma unused (updateRect)
	if (orientation == DV_HORZ) DrawHorz();
	else DrawVert();
}

void DragView::DrawHorz() {
	Window()->Lock();
	
	rgb_color Gold = {255,203,0,255};
	rgb_color Yellow = {255,255,0,255};
	rgb_color DkGrey  = {176,176,176,255};
	rgb_color LtGrey = {230,230,230,255};

	BRect bounds;
	bounds = Bounds();
	
	SetHighColor(Gold);
	FillRect(BRect(1,1,bounds.right,bounds.bottom-4));

	if (goAwayState == 1) DrawBitmap(goAwayUp, BPoint(0,1));
	else DrawBitmap(goAwayDown, BPoint(0,1));

	SetHighColor(Yellow);
	StrokeLine(BPoint(0,0), BPoint(bounds.right-1,0));
	StrokeLine(BPoint(0,1), BPoint(0,bounds.bottom-3));

	SetHighColor(LtGrey);
	FillRect(BRect(0,bounds.bottom-3,bounds.right,bounds.bottom-1));

	SetHighColor(DkGrey);
	StrokeLine(BPoint(0,bounds.bottom),BPoint(bounds.right,bounds.bottom));

	Window()->Unlock();
}

void DragView::DrawVert() {
	Window()->Lock();

	rgb_color Gold = {255,203,0};
	rgb_color Yellow = {255,255,0};
	rgb_color DkGrey = {176,176,176};
	rgb_color LtGrey = {230,230,230,0};

	BRect bounds = Bounds();

	SetHighColor(Gold);
	FillRect(BRect(1,1,bounds.right-4,bounds.bottom));

	if (goAwayState == 1) DrawBitmap(goAwayUp, BPoint(0,1));
	else DrawBitmap(goAwayDown, BPoint(0,1));

	SetHighColor(Yellow);
	StrokeLine(BPoint(0,0), BPoint(bounds.right-3,0));
	StrokeLine(BPoint(0,1), BPoint(0,bounds.bottom-1));

	SetHighColor(LtGrey);
	FillRect(BRect(bounds.right-3,0,bounds.right-1,bounds.bottom));

	SetHighColor(DkGrey);
	StrokeLine(BPoint(bounds.right,0),BPoint(bounds.right,bounds.bottom));

	Window()->Unlock();
}

void DragView::MouseDown(BPoint where)
{
	BRect goAwayBounds;	

	if (orientation == 0) goAwayBounds.Set(3,3,16,14);
	else goAwayBounds.Set(3,3,16,14);

	if (thMouseID < 0) thMouseID = spawn_thread(MouseThread,"BecassoDVMC",B_DISPLAY_PRIORITY,(void*)this);	

	// Set a flag for clicking in the goAway or the menuBar
	if (goAwayBounds.Contains(where)) mouse_status = _GO_AWAY;
	else mouse_status = _DRAG;
	
	resume_thread(thMouseID);
}