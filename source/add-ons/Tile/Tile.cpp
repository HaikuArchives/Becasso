// © 1998-2001 Sum Software

#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include <StringView.h>
#include <Application.h>
#include <Path.h>
#include <Directory.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <string.h>
#include <Entry.h>
#include <Window.h>
#include <Button.h>
#include <PopUpMenu.h>
#include <View.h>
#include <Roster.h>

class TileView : public BView {
  public:
	TileView(BRect frame, BBitmap* bitmap);

	virtual ~TileView() { delete fBitmap; };

	virtual void SetBitmap(BBitmap* bitmap)
	{
		delete fBitmap;
		fBitmap = bitmap;
		Invalidate();
	};

	virtual void Draw(BRect update);
	virtual void KeyDown(const char* bytes, int32 numbytes);

	BBitmap* Bitmap() { return fBitmap; };

  private:
	typedef BView inherited;
	BBitmap* fBitmap;
};

TileView::TileView(BRect frame, BBitmap* bitmap)
	: BView(frame, "tileview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fBitmap = bitmap;
}

void
TileView::Draw(BRect update)
{
	SetLowColor(DarkGrey);

	if (fBitmap) {
		int xs = fBitmap->Bounds().IntegerWidth();
		int ys = fBitmap->Bounds().IntegerHeight();
		for (int x = 0; x < Bounds().Width() / xs; x++)
			for (int y = 0; y < Bounds().Height() / ys; y++)
				DrawBitmapAsync(fBitmap, BPoint(x * xs, y * ys));
	}
	else
		FillRect(update, B_SOLID_LOW);

	Sync();
}

void
TileView::KeyDown(const char* bytes, int32 numbytes)
{
	if (numbytes == 1) {
		if (bytes[0] == B_LEFT_ARROW) {
			BMessage msg('Til-');
			LockLooper();
			Window()->PostMessage(&msg);
			UnlockLooper();
		}
		else if (bytes[0] == B_RIGHT_ARROW) {
			BMessage msg('Til+');
			LockLooper();
			Window()->PostMessage(&msg);
			UnlockLooper();
		}
		else
			inherited::KeyDown(bytes, numbytes);
	}
	else
		inherited::KeyDown(bytes, numbytes);
}

class TileMainView : public BView {
  public:
	TileMainView(BRect rect);

	virtual ~TileMainView() {}

	virtual void MessageReceived(BMessage* msg);
};

class TilesList;

TileView* view;
node_ref tilesRef;
BList* tilesDirList;
TilesList* CurrentTilesEntries;
int32 current_index;
BButton *next, *prev;
BStringView* text;
BPopUpMenu* catalogPU;

TileMainView::TileMainView(BRect rect) : BView(rect, "tile main view", B_FOLLOW_ALL, B_WILL_DRAW)
{
	ResizeTo(188, 164);
	app_info ainfo;
	be_app->GetAppInfo(&ainfo);
	BEntry appEntry = BEntry(&ainfo.ref);
	BEntry appDir;
	BPath appPath;
	appEntry.GetParent(&appDir);
	appDir.GetPath(&appPath);
	char dirname[B_FILE_NAME_LENGTH];
	strcpy(dirname, appPath.Path());
	BDirectory tilesDir;
	tilesDir.SetTo(strcat(dirname, "/Tiles"));
	tilesDir.GetNodeRef(&tilesRef);
	tilesDirList = new BList();
	catalogPU = new BPopUpMenu("Catalog");
	BMenuField* catalogMenu =
		new BMenuField(BRect(8, 4, 180, 30), "catalog", "Catalog:", catalogPU);
	catalogMenu->SetDivider(50);
	AddChild(catalogMenu);
	view = new TileView(BRect(8, 32, 120, 144), NULL);
	AddChild(view);
	prev = new BButton(BRect(128, 32, 150, 52), "prev", "<", new BMessage('Til-'));
	next = new BButton(BRect(158, 32, 180, 52), "next", ">", new BMessage('Til+'));
	AddChild(prev);
	AddChild(next);
	text = new BStringView(BRect(8, 146, 180, 160), "title", "(no tile)");
	AddChild(text);
}

class TilesList {
  public:
	TilesList() { fIndex = 0; }

	virtual ~TilesList() {}

	virtual void AddItem(BEntry entry)
	{
		if (fIndex < 4096) {
			entry.GetRef(&(entries[fIndex++]));
		}
		else
			fprintf(stderr, "TilesList: Too many tiles\n");
	}

	virtual BEntry ItemAt(int32 index) // NB No error checking here...
	{
		BEntry entry;
		entry.SetTo(&entries[index], true);
		return entry;
	}

	virtual int32 CountItems() { return fIndex; };

	virtual void Reset() { fIndex = 0; };

  private:
	entry_ref entries[4096];
	int32 fIndex;
};

class TilesDir {
  public:
	TilesDir() { fTilesList = new TilesList(); };

	~TilesDir() { delete fTilesList; };

	entry_ref fDir;
	TilesList* fTilesList;
};

void
TileMainView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'Til-':
		if (current_index > 0) {
			char title[B_FILE_NAME_LENGTH];
			char name[B_FILE_NAME_LENGTH];
			BEntry entry = CurrentTilesEntries->ItemAt(--current_index);
			view->SetBitmap(entry2bitmap(entry));
			entry.GetName(name);
			if (view->Bitmap())
				sprintf(
					title, "%s (%i x %i)", name, int(view->Bitmap()->Bounds().Height() + 1),
					int(view->Bitmap()->Bounds().Width() + 1)
				);
			else
				sprintf(title, "No handler for %s", name);
			text->SetText(title);
			next->SetEnabled(true);
		}
		if (current_index)
			prev->SetEnabled(true);
		else
			prev->SetEnabled(false);
		break;
	case 'Til+':
		if (current_index < CurrentTilesEntries->CountItems() - 1) {
			char title[B_FILE_NAME_LENGTH];
			char name[B_FILE_NAME_LENGTH];
			BEntry entry = CurrentTilesEntries->ItemAt(++current_index);
			view->SetBitmap(entry2bitmap(entry));
			entry.GetName(name);
			if (view->Bitmap())
				sprintf(
					title, "%s (%i x %i)", name, int(view->Bitmap()->Bounds().Height() + 1),
					int(view->Bitmap()->Bounds().Width() + 1)
				);
			else
				sprintf(title, "No handler for %s", name);
			text->SetText(title);
			prev->SetEnabled(true);
		}
		if (current_index < CurrentTilesEntries->CountItems() - 1)
			next->SetEnabled(true);
		else
			next->SetEnabled(false);
		break;
	case 'TilS': {
		int index = catalogPU->IndexOf(catalogPU->FindMarked());
		//		printf ("New catalog: %d\n", index);
		CurrentTilesEntries = (TilesList*)((TilesDir*)tilesDirList->ItemAt(index))->fTilesList;
		current_index = 0;
		LockLooper();
		view->MakeFocus();
		if (CurrentTilesEntries->CountItems()) {
			view->SetBitmap(entry2bitmap(CurrentTilesEntries->ItemAt(0)));
			char title[B_FILE_NAME_LENGTH];
			char name[B_FILE_NAME_LENGTH];
			CurrentTilesEntries->ItemAt(0).GetName(name);
			if (view->Bitmap())
				sprintf(
					title, "%s (%i x %i)", name, int(view->Bitmap()->Bounds().Height() + 1),
					int(view->Bitmap()->Bounds().Width() + 1)
				);
			else
				sprintf(title, "No handler for %s", name);
			text->SetText(title);
			prev->SetEnabled(true);
			next->SetEnabled(true);
		}
		if (current_index == 0)
			prev->SetEnabled(false);
		if (current_index == CurrentTilesEntries->CountItems() - 1)
			next->SetEnabled(false);
		UnlockLooper();
		break;
	}
	default:
		BView::MessageReceived(msg);
		return;
	}
	addon_preview();
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Tile");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1998-2001 ∑ Sum Software");
	strcpy(info->description, "Tile the selection with a texture");
	info->type = BECASSO_GENERATOR;
	info->index = index;
	info->version = 1;
	info->release = 3;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = 0;
	info->flags = LAYER_ONLY;
	return B_OK;
}

status_t
addon_close(void)
{
	TilesDir* adir;
	for (uint32 i = 0; (adir = (TilesDir*)tilesDirList->ItemAt(i), adir); i++)
		delete adir;
	delete tilesDirList;
	return B_OK;
}

status_t
addon_exit(void)
{
	return B_OK;
}

status_t
addon_make_config(BView** vw, BRect rect)
{
	*vw = new TileMainView(rect);
	tilesDirList->MakeEmpty();
	for (int i = catalogPU->CountItems(); i >= 0; i--)
		delete (catalogPU->RemoveItem(i));
	// tilesEntries.Reset();
	BEntry tilefile;
	BDirectory tilesDir;
	tilesDir.SetTo(&tilesRef);
	tilesDir.Rewind();
	while (tilesDir.GetNextEntry(&tilefile) != ENOENT) {
		BDirectory subdir;
		if (subdir.SetTo(&tilefile) == B_OK) {
			BPath path;
			tilefile.GetPath(&path);
			catalogPU->AddItem(new BMenuItem(path.Leaf(), new BMessage('TilS')));
			//			printf ("Dir: %s\n", path.Leaf());
			subdir.Rewind();
			TilesDir* tilesdir = new TilesDir();
			BEntry dirent;
			subdir.GetEntry(&dirent);
			dirent.GetRef(&(tilesdir->fDir));
			BEntry tile;
			while (subdir.GetNextEntry(&tile) != ENOENT) {
				//				BPath tilepath;
				//				tile.GetPath (&tilepath);
				//				printf ("     %s\n", tilepath.Leaf());
				tilesdir->fTilesList->AddItem(tile);
			}
			tilesDirList->AddItem(tilesdir);
		}
		// It wasn't a directory.  Continue.
	}
	BMenuItem* firstitem = catalogPU->ItemAt(0);
	if (firstitem)
		firstitem->SetMarked(true);
	CurrentTilesEntries = (TilesList*)((TilesDir*)tilesDirList->ItemAt(0))->fTilesList;
	if (CurrentTilesEntries->CountItems()) {
		view->SetBitmap(entry2bitmap(CurrentTilesEntries->ItemAt(0)));
		char title[B_FILE_NAME_LENGTH];
		char name[B_FILE_NAME_LENGTH];
		CurrentTilesEntries->ItemAt(0).GetName(name);
		if (view->Bitmap())
			sprintf(
				title, "%s (%i x %i)", name, int(view->Bitmap()->Bounds().Height() + 1),
				int(view->Bitmap()->Bounds().Width() + 1)
			);
		else
			sprintf(title, "No handler for %s", name);
		text->SetText(title);
		prev->SetEnabled(true);
	}
	if (current_index == 0)
		prev->SetEnabled(false);
	if (current_index == CurrentTilesEntries->CountItems())
		next->SetEnabled(false);

	return B_OK;
}

status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* frame, bool final, BPoint /* point*/, uint32 /* buttons */
)
{
	int error = ADDON_OK;
	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = inLayer;
	//		*outLayer = new Layer (*inLayer);

	if (*outSelection == NULL && mode == M_SELECT)
		*outSelection = new Selection(inLayer->Bounds());

	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();
	uint32 h = frame->IntegerHeight() + 1;
	uint32 w = frame->IntegerWidth() + 1;
	uint8* mapbits = NULL;
	uint32 mbpr = 0;
	uint32 mdiff = 0;

	switch (mode) {
	case M_DRAW: {
		BRect zeroframe = *frame;
		zeroframe.OffsetTo(B_ORIGIN);
		BBitmap* b = new BBitmap(zeroframe, B_RGB_32_BIT, true);
		b->Lock();
		BView* v = new BView(zeroframe, "tmp", 0, 0);
		b->AddChild(v);
		v->SetLowColor(DarkGrey);
		if (view->Bitmap()) {
			int xs = view->Bitmap()->Bounds().IntegerWidth();
			int ys = view->Bitmap()->Bounds().IntegerHeight();
			for (int x = 0; x < zeroframe.Width() / xs; x++)
				for (int y = 0; y < zeroframe.Height() / ys; y++) {
					v->DrawBitmapAsync(view->Bitmap(), BPoint(x * xs, y * ys));
				}
		}
		else
			v->FillRect(zeroframe, B_SOLID_LOW);
		v->Sync();
		b->RemoveChild(v);
		delete v;
		uint32* bd = (uint32*)b->Bits() - 1;
		if (inSelection) {
			mbpr = inSelection->BytesPerRow();
			mdiff = mbpr - w;
			mapbits = (uint8*)inSelection->Bits() - 1 + int(frame->top) * mbpr + int(frame->left);
			for (uint32 y = 0; y < h; y++) {
				for (uint32 x = 0; x < w; x++) {
					bgra_pixel pixel = *(++bd) & COLOR_MASK;
					uint8 alpha = *(++mapbits);
					*(bd) = pixel | (alpha << ALPHA_BPOS);
				}
				mapbits += mdiff;
			}
			// memcpy ((*outLayer)->Bits(), inLayer->Bits(), inLayer->BitsLength());
			*outLayer = inLayer;
		}
		else // No selection: tile the entire canvas/
		{
			for (int i = 0; i < b->BitsLength() / 4; i++)
				*(bd) = *(++bd) | ALPHA_MASK;
		}
		AddWithAlpha(b, *outLayer, int(frame->left), int(frame->top));
		delete b;
		break;
	}
	case M_SELECT: {
		break;
	}
	default:
		fprintf(stderr, "Tile: Unknown mode\n");
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
