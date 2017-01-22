#include "TabView.h"
#include "Colors.h"
#include <stdio.h>
#include <string.h>
#include <InterfaceDefs.h>

#define _HTABHEIGHT (TAB_HEIGHT/2)

TabView::TabView (BRect frame, const char *name, uint32 resizingMode)
: BView (frame, name, resizingMode, B_WILL_DRAW | B_NAVIGABLE)
{
	index = 0;
	current = -2;
	focusedview = 0;
	SetViewColor (LightGrey);
}

TabView::~TabView ()
{
	for (int i = 0; i < index; i++)
		delete views[i];
}

void TabView::Draw (BRect update)
{
	update = update;
	if (current < -1) 	// No sheet selected yet
		return;
	float xpos = _HTABHEIGHT + 1;
	for (int i = 0; i < index; i++)
	{
		float width = StringWidth (tabname[i]);
		// Up slope of tab
		SetHighColor (Grey30);
		if (current == i - 1)
			MovePenTo (BPoint (xpos - _HTABHEIGHT/2 + 1, _HTABHEIGHT - 1));
		else
			MovePenTo (BPoint (xpos - _HTABHEIGHT, TAB_HEIGHT));
		StrokeLine (BPoint (xpos, 0));
		// Horizontal line of tab
		StrokeLine (BPoint (xpos + width, 0));
		// Down slope of tab
		SetHighColor (Grey15);
		if (i == current || i == index - 1)
			StrokeLine (BPoint (xpos + width + _HTABHEIGHT, TAB_HEIGHT));
		else
			StrokeLine (BPoint (xpos + width + _HTABHEIGHT/2, _HTABHEIGHT));
		// Tab label
		if (current == i)
			SetHighColor (Black);
		else
			SetHighColor (Grey8);
		SetLowColor (Grey30);
		SetDrawingMode (B_OP_OVER);
		DrawString (tabname[i], BPoint (xpos + 1, TAB_HEIGHT - 2));
		// View top
		if (current == i - 1)
		{
			SetHighColor (Grey30);
			StrokeLine (BPoint (xpos + 1, TAB_HEIGHT), BPoint (xpos + width + _HTABHEIGHT, TAB_HEIGHT));
		}
		else if (current == i + 1)
		{
			SetHighColor (Grey30);
			StrokeLine (BPoint (xpos - _HTABHEIGHT, TAB_HEIGHT), BPoint (xpos + width, TAB_HEIGHT));
		}
		if (IsFocus() && focusedview == i)
		{
			SetHighColor (ui_color (B_KEYBOARD_NAVIGATION_COLOR));
			StrokeLine (BPoint (xpos + 1, TAB_HEIGHT - 1), BPoint (xpos + width - 1, TAB_HEIGHT - 1));
		}
		xpos += width + _HTABHEIGHT;
	}
	// other view borders
	SetHighColor (Grey14);
	StrokeLine (BPoint (Bounds().Width(), TAB_HEIGHT), BPoint (Bounds().right, Bounds().bottom - 1));
	StrokeLine (BPoint (Bounds().left, Bounds().bottom - 1));
	SetHighColor (Grey30);
	StrokeLine (BPoint (0, TAB_HEIGHT));
	StrokeLine (BPoint (xpos + 1, TAB_HEIGHT), BPoint (Bounds().Width(), TAB_HEIGHT));
}

void TabView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes == 1)
	{
		switch (*bytes)
		{
		case B_SPACE:
		case B_ENTER:
		{	
			//printf ("Enter\n");
			RaiseView (focusedview);
			inherited::KeyDown (bytes, numBytes);
			break;
		}
		case B_LEFT_ARROW:
			//printf ("Left\n");
			focusedview--;
			if (focusedview < 0)
				focusedview = index - 1;
			Invalidate();
			break;
		case B_RIGHT_ARROW:
			//printf ("Right\n");
			focusedview++;
			if (focusedview >= index)
				focusedview = 0;
			Invalidate();
			break;
		case B_TAB:
			//MakeFocus (false);
			inherited::KeyDown (bytes, numBytes);
			Invalidate();
			break;
		default:
			inherited::KeyDown (bytes, numBytes);
		}
	}
	else
		inherited::KeyDown (bytes, numBytes);
}

void TabView::MakeFocus (bool focused)
{
	inherited::MakeFocus (focused);
	Invalidate();
}

void TabView::MouseDown (BPoint point)
{
	if (point.y < 0 || point.y > TAB_HEIGHT)
		return;
	float xpos = 10;
	float width;
	for (int i = 0; i < index; i++)
	{
		width = StringWidth (tabname[i]);
		if (point.x > xpos - 5 && point.x < xpos + width + 5)
		{
			RaiseView (i);
			return;
		}
		xpos += width;
	}
}

void TabView::AddView (BView *view, const char *tab)
{
	if (index < MAX_VIEWS - 1)
	{
		strcpy (tabname[index], tab);
		views[index] = view;
		if (index == 0)
		{
			RaiseView (0);
		}
		index++;
	}
	else
		fprintf (stderr, "More than %i Views in a TabView\n", MAX_VIEWS);
}

//BView *TabView::Current ()
//{
//	return (views[current]);
//}

void TabView::RaiseView (int n)
{
	if (current == n)
		return;

	if (current > -2)
		RemoveChild (views[current]);
	AddChild (views[n]);
	current = n;
	Invalidate();
}
