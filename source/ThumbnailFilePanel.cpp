// ThumbnailFilePanel

#include "ThumbnailFilePanel.h"
#include "Colors.h"
#include <stdio.h>
#include "BitmapStuff.h"
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Entry.h>
#include <Mime.h>
#include <Window.h>

#define THUMBNAIL_VIEW_HEIGHT 120
#define THUMBNAIL_VIEW_WIDTH 160

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

ThumbnailFilePanel::ThumbnailFilePanel (file_panel_mode mode, 
								         BMessenger* target, 
								         entry_ref *panel_directory, 
								         uint32 node_flavors, 
								         bool allow_multiple_selection, 
								         BMessage *message, 
								         BRefFilter *filter, 
								         bool modal, 
								         bool hide_when_done)
: BFilePanel (mode, target, panel_directory, node_flavors, allow_multiple_selection,
	message, filter, modal, hide_when_done)
{
	float resizeby = THUMBNAIL_VIEW_HEIGHT - 40;
	float minWidth, minHeight, maxWidth, maxHeight;
	Window()->Lock();
	Window()->GetSizeLimits (&minWidth, &maxWidth, &minHeight, &maxHeight);
	BView *background = Window()->ChildAt (0);
	BView *poseView = background->FindView ("PoseView");
//	BView *cancelButton = background->FindView ("cancel button");
	BView *countVW = background->FindView ("CountVw");
	BView *vScrollBar = background->FindView ("VScrollBar");
	
	BView *hScrollBar = background->FindView ("HScrollBar");
	//printf ("Looking for Horizontal ScrollBar...\n");
	// Horizontal ScrollBar workaround
	// Note: Be was so kind as to change the name from "" to "HScrollBar" in 4.1
	if (!hScrollBar)	// >= R4.1 I suppose.
	{
		for (int i = background->CountChildren(); i > 0; i--)
		{
			BView *view = background->ChildAt (i - 1);
			// printf ("View %d, '%s'\n", i - 1, view->Name());
			if (!strcmp ("", view->Name()))
			{
				hScrollBar = view;
				break;
			}
		}
	}
	BRect tFrame = poseView->Frame();
	Window()->ResizeBy (0, resizeby);
	Window()->SetSizeLimits (minWidth, maxWidth, minHeight + resizeby, maxWidth);
	// The poseView will resize along, so undo that.
	poseView->ResizeBy (0, -resizeby);
	vScrollBar->ResizeBy (0, -resizeby);
	hScrollBar->MoveBy (0, -resizeby);
	countVW->MoveBy (0, -resizeby);
	BRect viewRect= BRect (tFrame.left - 1, tFrame.bottom + 24,tFrame.left + THUMBNAIL_VIEW_WIDTH, tFrame.bottom + 8 + THUMBNAIL_VIEW_HEIGHT);
	fView = new ThumbnailView (viewRect,
							   "thumbnail view",
							   B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	background->AddChild (fView);
	
	infoView1 = new BStringView (BRect (viewRect.right + 8, viewRect.top, background->Frame().right - 4, viewRect.top + 16), "infoView1", "File Type", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	infoView2 = new BStringView (BRect (viewRect.right + 8, viewRect.top + 18, background->Frame().right - 4, viewRect.top + 34), "infoView2", "Dimensions", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	background->AddChild (infoView1);
	background->AddChild (infoView2);
	SelectionChanged();
	Window()->Unlock();
}

ThumbnailFilePanel::~ThumbnailFilePanel ()
{
}

void ThumbnailFilePanel::SelectionChanged ()
{
	Rewind();
	entry_ref ref;
	if (GetNextSelectedRef (&ref) != B_OK)
	{
		infoView1->SetText ("");
		infoView2->SetText ("");
		fView->update (NULL);
		return;
	}
	BEntry entry (&ref);
	BBitmap *map = entry2bitmap (entry, true);
	if (!map)
	{
		infoView1->SetText ("");
		infoView2->SetText ("");
		fView->update (NULL);
		return;
	}

	//filetype [node.ReadAttr ("BEOS:TYPE", 0, 0, filetype, 80)] = 0;
	//printf ("Filetype: '%s'\n", filetype);
	BNode node (&ref);
	BNodeInfo ninfo (&node);
	char filetype[256];
	if (ninfo.GetType (filetype) < B_OK)
	{
		BEntry entry (&ref);
		BPath path (&entry);
		update_mime_info (path.Path(), 0, 1, 0);
		if (ninfo.GetType (filetype) < B_OK)
		{
			fprintf (stderr, "Can't find the type of '%s'\n", path.Path());
		}
	}
	
	BMimeType mime (filetype);
	BRect viewRect = fView->Frame();
	infoView1->MoveTo (viewRect.right + 8, viewRect.top);
	infoView2->MoveTo (viewRect.right + 8, viewRect.top + 18);
	if (mime.IsValid())
		infoView1->SetText (filetype);
	else
		infoView1->SetText ("?");

	char sizestr[80];
	sprintf (sizestr, "%ld x %ld", 
		map->Bounds().IntegerWidth() + 1,
		map->Bounds().IntegerHeight() + 1);
	infoView2->SetText (sizestr);
	
	fView->update (map);
	delete map;
}

///////////////////
//
//	ThumbnailView
//

ThumbnailView::ThumbnailView (BRect frame, const char *name, uint32 resizingMode, uint32 flags)
: BView (frame, name, resizingMode, flags)
{
	fBitmap = NULL;
}

void ThumbnailView::AttachedToWindow ()
{
	SetViewColor (Parent()->ViewColor());
}

ThumbnailView::~ThumbnailView ()
{
	delete fBitmap;
}

void ThumbnailView::Draw (BRect update)
{
	if (!fBitmap)
	{
		SetHighColor (Parent()->ViewColor());
		FillRect (update);
		return;
	}
	DrawBitmap (fBitmap, BPoint (1, 1));
	BRect bb = fBitmap->Bounds();
	bb.right += 2;
	bb.bottom += 2;
	SetHighColor (Grey13);
	StrokeLine (bb.RightTop(), bb.LeftTop());
	StrokeLine (bb.LeftBottom());
	SetHighColor (Grey29);
	StrokeLine (bb.RightBottom());
	StrokeLine (bb.RightTop());
}

void ThumbnailView::update (BBitmap *map)
{
	delete fBitmap;
	fBitmap = NULL;
	if (map)
	{
		BRect mbounds = map->Bounds();
		BRect bounds = Bounds();
		bounds.InsetBy (1, 1);
		float ratio = MIN (bounds.Width()/mbounds.Width(), bounds.Height()/mbounds.Height());
		bounds.right = bounds.left + ratio*mbounds.Width();
		bounds.bottom = bounds.top + ratio*mbounds.Height();
		bounds.OffsetTo (B_ORIGIN);
		BView *tmpView = new BView (bounds, "tmp View for scaling", 0, 0);
		fBitmap = new BBitmap (bounds, B_RGBA32, true);
		fBitmap->Lock();
		fBitmap->AddChild (tmpView);
		tmpView->DrawBitmap (map, mbounds, bounds);
		fBitmap->RemoveChild (tmpView);
		delete tmpView;
	}
	Invalidate();
}