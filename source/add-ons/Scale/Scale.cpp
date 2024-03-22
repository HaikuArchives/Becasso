// © 1998-2001 Sum Software

#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include <View.h>
#include <string.h>

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Scale");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1998-2001 ∑ Sum Software");
	strcpy(info->description, "Resize a selection of the canvas");
	info->type = BECASSO_TRANSFORMER;
	info->index = index;
	info->version = 1;
	info->release = 1;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = PREVIEW_FULLSCALE | PREVIEW_MOUSE | LAYER_AND_SELECTION;
	info->flags = 0;
	return B_OK;
}

status_t
addon_close(void)
{
	return B_OK;
}

status_t
addon_exit(void)
{
	return B_OK;
}

status_t
addon_make_config(BView** view, BRect rect)
{
	*view = NULL; // Scale has no GUI
	return B_OK;
}

status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* frame, bool final, BPoint point, uint32 buttons
)
{
	// We don't use the StatusBar in the present incarnation of the Scale add-on,
	// because everything happens interactively and the final anti-aliased drawing
	// upon release of the mouse button is one Becasso function call.
	// Perhaps in a future version this function will internally send messages to
	// a given BStatusBar object.

	static bool entry = false;
	static BPoint firstpoint = BPoint(-1, -1);
	static BPoint lastpoint = BPoint(-1, -1);
	static Layer* oLayer = NULL;
	static Selection* oSelection = NULL;
	static BRect oBounds = EmptyRect;
	static int varcorner = -1;
	static BRect newrect;

	int error = ADDON_OK;

	if (/*!inLayer || */ !inSelection) {
		printf("Scale only works with a valid selection ATM.\n");
		return (error);
	}

	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = inLayer; // new Layer (*inLayer);
	if (*outSelection == NULL)
		*outSelection = inSelection; // new Selection (*inSelection);

	//	if (*outLayer)
	//		(*outLayer)->Lock();
	//	if (*outSelection)
	//		(*outSelection)->Lock();

	if (final) {
		// This means the user has clicked "Apply".
		// Set up our local cutout copies of the original layer and selection.
		// We still do this (although Becasso (as of 1.3) passes add-ons a "clean"
		// layer each time) because it saves us the work of cutting out the selected part.

		//		firstpoint = point;
		oBounds = *frame;
		BRect bounds = oBounds;
		// oBounds.PrintToStream();
		bounds.OffsetTo(B_ORIGIN);
		oLayer = new Layer(bounds, "tmpLayer");
		oSelection = new Selection(bounds);
		BView* view = new BView(bounds, "tmp view", 0, 0);
		oSelection->Lock();
		oSelection->AddChild(view);
		view->DrawBitmap(inSelection, BPoint(-oBounds.left, -oBounds.top));
		view->Sync();
		oSelection->RemoveChild(view);
		oSelection->Unlock();
		entry = true;
		//		float dlt = ((frame->left - firstpoint.x)*(frame->left - firstpoint.x) + (frame->top
		//- firstpoint.y)*(frame->top - firstpoint.y)); 		float drt = ((frame->right -
		//firstpoint.x)*(frame->right - firstpoint.x) + (frame->top - firstpoint.y)*(frame->top -
		//firstpoint.y)); 		float dlb = ((frame->left - firstpoint.x)*(frame->left - firstpoint.x) +
		//(frame->bottom - firstpoint.y)*(frame->bottom - firstpoint.y)); 		float drb = ((frame->right
		//- firstpoint.x)*(frame->right - firstpoint.x) + (frame->bottom -
		//firstpoint.y)*(frame->bottom - firstpoint.y)); 		varcorner = 0; // LeftTop 		if (drt < dlt)
		//varcorner = 1;	// RightTop 		if (dlb < drt && dlb < dlt) varcorner = 2;	// LeftBottom 		if
		//(drb < dlb && drb < drt && drb < dlt) varcorner = 3;	// RightBottom
		printf("Scale: final - setup done\n");
	}

	if (!final && buttons && !entry) // First entry of a realtime drag
	{
		// Set up our local cutout copies of the original layer and selection.
		// We still do this (although Becasso (as of 1.3) passes add-ons a "clean"
		// layer each time) because it saves us the work of cutting out the selected part.

		firstpoint = point;
		oBounds = *frame;
		BRect bounds = oBounds;
		// oBounds.PrintToStream();
		bounds.OffsetTo(B_ORIGIN);
		oLayer = new Layer(bounds, "tmpLayer");
		oSelection = new Selection(bounds);
		BView* view = new BView(bounds, "tmp view", 0, 0);
		oSelection->Lock();
		oSelection->AddChild(view);
		view->DrawBitmap(inSelection, BPoint(-oBounds.left, -oBounds.top));
		view->Sync();
		oSelection->RemoveChild(view);
		oSelection->Unlock();

		entry = true;
		float dlt =
			((frame->left - firstpoint.x) * (frame->left - firstpoint.x) +
			 (frame->top - firstpoint.y) * (frame->top - firstpoint.y));
		float drt =
			((frame->right - firstpoint.x) * (frame->right - firstpoint.x) +
			 (frame->top - firstpoint.y) * (frame->top - firstpoint.y));
		float dlb =
			((frame->left - firstpoint.x) * (frame->left - firstpoint.x) +
			 (frame->bottom - firstpoint.y) * (frame->bottom - firstpoint.y));
		float drb =
			((frame->right - firstpoint.x) * (frame->right - firstpoint.x) +
			 (frame->bottom - firstpoint.y) * (frame->bottom - firstpoint.y));
		varcorner = 0; // LeftTop
		if (drt < dlt)
			varcorner = 1; // RightTop
		if (dlb < drt && dlb < dlt)
			varcorner = 2; // LeftBottom
		if (drb < dlb && drb < drt && drb < dlt)
			varcorner = 3; // RightBottom
	}
	if (buttons)
		lastpoint = point;
	if (mode == M_DRAW) {
		oLayer->Lock();
		inLayer->Lock();
		inSelection->Lock();
		CutOrCopy(
			*outLayer ? *outLayer : inLayer, oLayer, inSelection, int(oBounds.left),
			int(oBounds.top), true
		);
		inSelection->Unlock();
		inLayer->Unlock();
		oLayer->Unlock();
	}

	BRect lframe = *frame;
	newrect = lframe;
	BRect bounds = lframe;
	bounds.OffsetTo(B_ORIGIN);
	//	lastpoint = point;
	switch (varcorner) {
	case 0:
		newrect.left = lastpoint.x;
		newrect.top = lastpoint.y;
		break;
	case 1:
		newrect.right = lastpoint.x;
		newrect.top = lastpoint.y;
		break;
	case 2:
		newrect.left = lastpoint.x;
		newrect.bottom = lastpoint.y;
		break;
	case 3:
		newrect.right = lastpoint.x;
		newrect.bottom = lastpoint.y;
	}
	if (newrect.right <= newrect.left || newrect.bottom <= newrect.top)
		return (0);

	rgb_color hi = highcolor();
	rgb_color lo = lowcolor();

	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();

	switch (mode) {
	case M_DRAW: {
		if (*outLayer && buttons) // The user is dragging.
		{
			printf("Dragging\n");
			BRect newbounds = newrect;
			newbounds.OffsetTo(B_ORIGIN);
			Layer* sLayer = new Layer(newbounds, "Scaled Layer");
			BView* view = new BView(newbounds, "tmp view", 0, 0);
			view->SetHighColor(contrastingcolor(lo, hi));
			sLayer->Lock();
			sLayer->AddChild(view);
			view->DrawBitmap(oLayer, oLayer->Bounds(), newbounds);
			view->StrokeRect(newbounds);
			view->Sync();
			sLayer->RemoveChild(view);
			delete view;
			AddWithAlpha(sLayer, (*outLayer), int(newrect.left), int(newrect.top));
			delete sLayer;
		}
		else if (entry) // The user has released the mouse.  Clean up.
		// N.B. We also get here when "Apply" is clicked.
		{
			printf("Clean up\n");
			firstpoint = BPoint(-1, -1);
			BRect newbounds = newrect;
			newbounds.OffsetTo(B_ORIGIN);
			// newrect.PrintToStream();
			BView* view = new BView(inLayer->Bounds(), "tmp view", 0, 0);
			Layer* sLayer = new Layer(newbounds, "Scaled Layer");
			Selection* sSelection = new Selection(newbounds);
			Scale(oLayer, oSelection, sLayer, sSelection);
			AddWithAlpha(
				sLayer, (*outLayer) ? (*outLayer) : inLayer, int(newrect.left), int(newrect.top)
			);
			if (*outSelection)
				(*outSelection)->AddChild(view);
			else
				inSelection->AddChild(view);

			view->SetLowColor(SELECT_NONE);
			view->FillRect(inLayer->Bounds(), B_SOLID_LOW);
			view->DrawBitmap(sSelection, newrect.LeftTop());
			view->Sync();
			if (*outSelection)
				(*outSelection)->RemoveChild(view);
			else
				inSelection->RemoveChild(view);
			delete view;
			delete sLayer;
			delete sSelection;
			entry = false;
			delete oLayer;
			oLayer = NULL;
			delete oSelection;
			oSelection = NULL;
		}
		break;
	}
	case M_SELECT: {
		if (*outSelection && buttons) // The user is dragging.
		{
			BRect newbounds = newrect;
			newbounds.OffsetTo(B_ORIGIN);
			BView* view = new BView(inSelection->Bounds(), "tmp view", 0, 0);
			(*outSelection)->AddChild(view);
			view->SetLowColor(SELECT_NONE);
			view->FillRect(inLayer->Bounds(), B_SOLID_LOW);
			view->DrawBitmapAsync(oSelection, oSelection->Bounds(), newrect);
			view->Sync();
			(*outSelection)->RemoveChild(view);
			delete view;
		}
		else if (entry) // The user has released the mouse.  Clean up.
		// N.B. We also get here when "Apply" is clicked.
		{
			firstpoint = BPoint(-1, -1);
			BRect newbounds = newrect;
			newbounds.OffsetTo(B_ORIGIN);
			// newrect.PrintToStream();
			BView* view = new BView(inLayer->Bounds(), "tmp view", 0, 0);
			Selection* sSelection = new Selection(newbounds);
			Scale(NULL, oSelection, NULL, sSelection);
			if (*outSelection)
				(*outSelection)->AddChild(view);
			else
				inSelection->AddChild(view);
			view->SetLowColor(SELECT_NONE);
			view->FillRect(inLayer->Bounds(), B_SOLID_LOW);
			view->DrawBitmap(sSelection, newrect.LeftTop());
			view->Sync();
			if (*outSelection)
				(*outSelection)->RemoveChild(view);
			else
				inSelection->RemoveChild(view);
			delete view;
			delete sSelection;
			entry = false;
			delete oLayer;
			oLayer = NULL;
			delete oSelection;
			oSelection = NULL;
		}
		break;
	}
	default:
		fprintf(stderr, "Scale: Unknown mode\n");
		error = 1;
	}

	if (*outSelection)
		(*outSelection)->Unlock();
	if (*outLayer)
		(*outLayer)->Unlock();
	*frame = newrect;
	return (error);
}
