// ThumbnailFilePanel

#ifndef _THUMBNAILFILEPANEL_H
#define _THUMBNAILFILEPANEL_H

#include <FilePanel.h>
#include <View.h>
#include <Bitmap.h>
#include <StringView.h>

class ThumbnailView : public BView {
  public:
	ThumbnailView(BRect frame, const char* name, uint32 resizingMode, uint32 flags);
	virtual ~ThumbnailView();
	virtual void AttachedToWindow();
	virtual void Draw(BRect update);

	void update(BBitmap* map);

  private:
	BBitmap* fBitmap;
};

class ThumbnailFilePanel : public BFilePanel {
  public:
	ThumbnailFilePanel(
		file_panel_mode mode = B_OPEN_PANEL, BMessenger* target = NULL,
		entry_ref* panel_directory = NULL, uint32 node_flavors = 0,
		bool allow_multiple_selection = true, BMessage* message = NULL, BRefFilter* filter = NULL,
		bool modal = false, bool hide_when_done = true
	);
	virtual ~ThumbnailFilePanel();
	virtual void SelectionChanged();

  private:
	ThumbnailView* fView;
	BStringView* infoView1;
	BStringView* infoView2;
};

#endif
