// © 2001 Michael Pfeiffer

#include <be/interface/Menu.h>
#include <be/interface/MenuItem.h>
#include <be/interface/PopUpMenu.h>
#include <be/interface/MenuField.h>
#include <be/interface/CheckBox.h>
#include <be/interface/StringView.h>
#include <be/support/String.h>
#include <be/app/MessageFilter.h>

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnWindow.h"
#include "AddOnSupport.h"
#ifdef IMAGEMANIP_WEAK_LINKAGE
#undef _IMPEXP_IMAGEMANIP // for weak linkage
#endif
#include "ImageManip.h"
#include "BBitmapAccessor.h"

#define MSG_FILTER_SELECTED 'fsel'
#define MSG_FILTER_PREVIEW 'prev'
#define MSG_APPLY_TO_SELECTION 'aply'

#ifdef MANIPULATOR
#define WINDOW_TITLE "Manipulate %s"
#define NAME "Filter"
#define MENU_FIELD "Filter:"
#define POPUP_NAME "Select Filter"
#define WRAPPER_TYPE BECASSO_FILTER
#define PREVIEW PREVIEW_2x2
#define APPLY_TO_SELECTION "Apply filter to selection too"
#endif

#ifdef CONVERTER
#define WINDOW_TITLE "Convert %s"
#define NAME "Generator"
#define MENU_FIELD "Generator:"
#define POPUP_NAME "Select Generator"
#define WRAPPER_TYPE BECASSO_TRANSFORMER
#define PREVIEW PREVIEW_2x2
#define APPLY_TO_SELECTION "Apply transformer to selection too"
#endif

class WrapperWindow;

static WrapperWindow* window = NULL;
static int32 mAddonId = -1;
static bool mApplyToSelection = false;

class WrapperWindow : public AddOnWindow {
	void SetupFilters(BMenu* sub_menu);
	BView* mView;
	BMenuField* mMenuField;
	int mHeight, mWidth;
	void SetupConfigView(int32 addon, const char* name);
	BStringView* mStatus;

  public:
	WrapperWindow(BRect rect, becasso_addon_info* info);
	virtual ~WrapperWindow();

	void MessageReceived(BMessage* msg);
	void RemoveView();
	void SetStatus(const char* text);
};

WrapperWindow::WrapperWindow(BRect rect, becasso_addon_info* info) : AddOnWindow(rect, info)
{
	mView = NULL;
	BMenu* sub_menu = new BPopUpMenu(POPUP_NAME);
	mHeight = rect.IntegerHeight();
	mWidth = rect.IntegerWidth();
	SetupFilters(sub_menu);
	mMenuField = new BMenuField(BRect(5, mHeight, 200, mHeight + 30), NAME, MENU_FIELD, sub_menu);
	mMenuField->SetDivider(70);
	Background()->AddChild(mMenuField);
	Background()->AddChild(new BButton(
		BRect(205, mHeight, 265, mHeight + 20), "", "Preview", new BMessage(MSG_FILTER_PREVIEW)
	));
	mHeight += 32;
	mWidth = 270;
	Background()->AddChild(new BCheckBox(
		BRect(5, mHeight, 265, mHeight + 20), "", APPLY_TO_SELECTION,
		new BMessage(MSG_APPLY_TO_SELECTION)
	));
	mHeight += 20;
	mStatus = new BStringView(BRect(5, mHeight, 265, mHeight + 20), "", "Status");
	Background()->AddChild(mStatus);
	mHeight += 22;
	ResizeTo(mWidth, mHeight);
};

WrapperWindow::~WrapperWindow() { window = NULL; }

void
WrapperWindow::SetupFilters(BMenu* sub_menu)
{
	// Create bitmap accessor object to pass along, so that we
	// only get the add-ons that support this bitmap in the menu
	BBitmap bitmap(BRect(0, 0, 100, 20), B_RGB32);
	BBitmapAccessor* accessor = Image_CreateBBitmapAccessor(&bitmap);
	if (accessor == NULL) {
		sub_menu->SetEnabled(false);
		return;
	}
	accessor->SetDispose(false);

	// Create sub menu with image manipulators
	const char* addonName;
	const char* addonInfo;
	const char* addonCategory;
	int32 addonVersion;
	image_addon_id* outList;
	int32 outCount = 0;
	BMenuItem* item;
#ifdef MANIPULATOR
	if (Image_GetManipulators(accessor, NULL, &outList, &outCount) == B_OK)
#endif
#ifdef CONVERTER
		if (Image_GetConverters(accessor, NULL, &outList, &outCount) == B_OK)
#endif
		{
			mAddonId = outList[0];

			for (int i = outCount - 1; i >= 0; --i) {
				if (Image_GetAddonInfo(
						outList[i], &addonName, &addonInfo, &addonCategory, &addonVersion
					) == B_OK) {
					BMessage* msg = new BMessage(MSG_FILTER_SELECTED);
					msg->AddInt32("addon_id", outList[i]);
					msg->AddString("addon_name", addonName);
					item = new BMenuItem(addonName, msg);
					sub_menu->AddItem(item);
					if (i == outCount - 1)
						PostMessage(msg);
				}
			}
			delete[] outList;
		}
	if (outCount == 0)
		sub_menu->SetEnabled(false);
	else {
		sub_menu->ItemAt((int32)0)->SetMarked(true);
	}
	delete accessor;
}

void
WrapperWindow::RemoveView()
{
	if (mView) {
		Background()->RemoveChild(mView);
		delete mView;
		mView = NULL;
	}
}

void
WrapperWindow::SetupConfigView(int32 addon, const char* name)
{
	mAddonId = addon;
	RemoveView();
	int width;
	BMessage dummy;
	if (Image_MakeConfigurationView(mAddonId, &dummy, &mView) == 0 && mView) {
		Background()->AddChild(mView);
		int width = max_c(mWidth, mView->Bounds().Width()) + 10;
		mView->MoveTo((width - mView->Bounds().Width()) / 2, mHeight);
		ResizeTo(width, mHeight + mView->Bounds().Height());
	}
	else {
		ResizeTo(mWidth, mHeight);
		width = mWidth;
	}
}

void
WrapperWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case MSG_FILTER_SELECTED: {
		BString name;
		SetStatus("Status");
		msg->FindString("addon_name", &name);
		SetupConfigView(msg->FindInt32("addon_id"), name.String());
		break;
	}
	case MSG_FILTER_PREVIEW:
		aPreview();
		break;
	case MSG_APPLY_TO_SELECTION: {
		int32 s;
		if (B_OK == msg->FindInt32("be:value", &s)) {
			mApplyToSelection = (bool)s;
		}
	} break;
	default:
		AddOnWindow::MessageReceived(msg);
	}
}

void
WrapperWindow::SetStatus(const char* text)
{
	mStatus->SetText(text);
}

int
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "LibImageManip Wrapper");
	strcpy(info->author, "Michael Pfeiffer");
	strcpy(info->copyright, "© 2001 under GPL");
	strcpy(info->description, "Wrapper to LibImageManip");
	info->type = WRAPPER_TYPE;
	info->index = index;
	info->version = 0;
	info->release = 1;
	info->becasso_version = 1;
	info->becasso_release = 0;
	info->does_preview = PREVIEW;
#if defined(MANIPULATOR) || defined(IMAGEMANIP_WEAK_LINKAGE)
	// Note: We assume that the filter is loaded first by Becasso,
	//       then the transformer is loaded and both are loaded
	//       or none.
	//       Imagelibmanip is initialized once here.
	//       When we exit we assume that the transformer is called
	//       last and we close Imagelibmanip there.
	//       We must also assume that there is no other filter that
	//       uses Imagelibmanip otherwise it is likely that Becasso
	//       will crash when the add-ons are exited (addon_exit).
	//
	// When IMAGEMANIP_WEAK_LINKAGE is defined we solve this problem
	// as Imagelibmanip is loaded in each wrapper add-on.

	// Check the library's version
	int32 curVersion;
	int32 minVersion;

	// Initialize the image manipulation library
	if (Image_Init(NULL) != B_OK) {
		puts("Cannot initialize libimagemanip.so");
		return B_ERROR;
	}

	// Check the library's version
	puts(Image_Version(&curVersion, &minVersion));
	if (curVersion < IMAGE_LIB_MIN_VERSION) {
		puts("Need a newer version of libimagemanip.so");
		return B_ERROR;
	}
	if (minVersion > IMAGE_LIB_CUR_VERSION) {
		puts("Need an older version of libimagemanip.so");
		return B_ERROR;
	}
#endif

	window = new WrapperWindow(BRect(100, 180, 100 + 188, 180 + 72), info);
	window->Run();
	return (0);
}

int
addon_exit(void)
{
	// We have to remove the config view from Imagelibmanip otherwise we
	// crash when the destructor of this window is called!
	window->Lock();
	window->RemoveView();
	window->Unlock();
	// Shutdown the image manipulation library
#if defined(MANIPULATOR) || defined(IMAGEMANIP_WEAK_LINKAGE)
	// Note: We assume that the addon_exit of the transformer is called
	//       after the filter add-on.
	Image_Shutdown();
#endif
	return (0);
}

int
addon_open(BWindow* client, const char* name)
{
	char title[B_FILE_NAME_LENGTH];
	sprintf(title, WINDOW_TITLE, name);
	window->Lock();
	window->SetTitle(title);
	if (window->IsHidden())
		window->aShow(client);
	else
		window->aActivate(client);
	window->SetStatus("Status");
	window->Unlock();
	return (0);
}

int
addon_close()
{
	window->aClose();
	return (0);
}

int
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* frame, bool final, BPoint /* point */, uint32 /* buttons */
)
{
	int error = ADDON_OK;

	BRect bounds = inLayer->Bounds();

	//	printf ("Bounds: ");
	//	bounds.PrintToStream();
	//	printf ("Frame:  ");
	//	frame->PrintToStream();
	//	fprintf(stderr, "mode = %d final = %d\n", mode, final);
	//	fprintf(stderr, "in %x out %x in %x out %x\n", inLayer, *outLayer, inSelection,
	//*outSelection);

#ifdef MANIPULATOR
	if (*outLayer == NULL && mode == M_DRAW) {
		*outLayer = new Layer(*inLayer);
	}
	if (*outSelection == NULL && mode == M_DRAW) {
		*outSelection = inSelection;
	}

	if (mode == M_SELECT) {
		if (inSelection) {
			if (*outSelection == NULL)
				*outSelection = new Selection(*inSelection);
		}
		else // No Selection to filter!
			return (0);
	}
	if (*outLayer)
		(*outLayer)->Lock();

	if (*outSelection)
		(*outSelection)->Lock();
#endif

	//	fprintf(stderr, "in %x out %x in %x out %x\n", inLayer, *outLayer, inSelection,
	//*outSelection);

	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;

	if (final) {
		window->Lock();
		window->Start();
		window->Unlock();
	}
	else {
		//		return error;
	}


	switch (mode) {
#ifdef MANIPULATOR // Becasso Filter
	case M_DRAW: {
		// Create bitmap accessor object
		memcpy((*outLayer)->Bits(), inLayer->Bits(), inLayer->BitsLength());
		Layer* layer;
		if (inSelection) {
			layer = new Layer(*inLayer);
			memcpy(layer->Bits(), inLayer->Bits(), inLayer->BitsLength());
		}
		else {
			layer = *outLayer;
		}
		BBitmapAccessor* src_accessor = Image_CreateBBitmapAccessor(layer);
		if (src_accessor == NULL) {
			error = ADDON_ABORT;
			;
			if (inSelection)
				delete layer;
			break;
		}
		src_accessor->SetDispose(false);

		if (Image_Manipulate(mAddonId, src_accessor, 0) < 0) {
			window->Lock();
			window->SetStatus("Error");
			window->Unlock();
		}
		delete src_accessor;

		if (mApplyToSelection && inSelection) {
			memcpy((*outSelection)->Bits(), inSelection->Bits(), inSelection->BitsLength());
			src_accessor = Image_CreateBBitmapAccessor(*outSelection);
			src_accessor->SetDispose(false);
			if (Image_Manipulate(mAddonId, src_accessor, 0) < 0) {
				window->Lock();
				window->SetStatus("Selection not supported");
				window->Unlock();
			}
			delete src_accessor;
		}

		if (inSelection && *outSelection) {
			CutOrCopy(layer, layer, *outSelection, 0, 0, false);
			AddWithAlpha(layer, *outLayer, 0, 0);
		}

		if (inSelection) {
			delete layer;
		}
		break;
	}
	case M_SELECT: {
		// Create bitmap accessor object
		memcpy((*outSelection)->Bits(), inSelection->Bits(), inSelection->BitsLength());
		BBitmapAccessor* src_accessor = Image_CreateBBitmapAccessor(*outSelection);
		if (src_accessor == NULL) {
			error = ADDON_ABORT;
			;
			break;
		}
		src_accessor->SetDispose(false);

		if (Image_Manipulate(mAddonId, src_accessor, 0) < 0) {
			window->Lock();
			window->SetStatus("Error");
			window->Unlock();
		}
		delete src_accessor;
		break;
	}
#endif


#ifdef CONVERTER // Becasso Transformer
	case M_SELECT:
	case M_DRAW: {
		if (mode == M_DRAW && *outLayer == NULL || mode == M_SELECT && *outSelection == NULL) {
			// Create bitmap accessor object
			BBitmapAccessor* src_accessor =
				Image_CreateBBitmapAccessor(mode == M_DRAW ? inLayer : inSelection);
			if (src_accessor == NULL) {
				error = ADDON_ABORT;
				;
				break;
			}
			src_accessor->SetDispose(false);

			// Create empty bitmap accessor object for the destination
			BBitmapAccessor* dest_accessor = Image_CreateBBitmapAccessor();

			// Do image conversion
			if (Image_Convert(mAddonId, src_accessor, dest_accessor, NULL) < 0) {
				window->Lock();
				window->SetStatus("Error");
				window->Unlock();
			}

			// Got valid new bitmap?
			if (dest_accessor->IsValid()) {
				BBitmap* dest = dest_accessor->Bitmap();
				dest->Lock();
				fprintf(stderr, "ColorSpace %d\n", inLayer->ColorSpace());
				fprintf(stderr, "ColorSpace %d\n", dest->ColorSpace());
				if (mode == M_DRAW && dest->ColorSpace() == B_RGBA32) {
					fprintf(stderr, "new Layer\n");
					*frame = dest->Bounds();
					frame->PrintToStream();
					*outLayer = new Layer(*frame, "Transformed");
					(*outLayer)->Bounds().PrintToStream();
					memcpy((*outLayer)->Bits(), dest->Bits(), dest->BitsLength());
				}
				if (mode == M_SELECT && dest->ColorSpace() == B_GRAY8) {
					fprintf(stderr, "new Selection\n");
					*frame = dest->Bounds();
					frame->PrintToStream();
					*outSelection = new Selection(*frame);
					(*outSelection)->Bounds().PrintToStream();
					memcpy((*outSelection)->Bits(), dest->Bits(), dest->BitsLength());
				}
				dest->Unlock();
			}
			delete dest_accessor;
			delete src_accessor;
		}
		else {
			fprintf(stderr, "Wrapper: outLayer must be null\n");
		}
		break;
	}
#endif

	default:
		fprintf(stderr, "Wrapper: Unknown mode\n");
		error = ADDON_UNKNOWN;
	}

#ifdef MANIPULATOR
	if (*outSelection)
		(*outSelection)->Unlock();
	if (*outLayer)
		(*outLayer)->Unlock();
#endif

	if (final) {
		window->Lock();
		window->Stopped();
		window->Unlock();
	}
	return (error);
}
