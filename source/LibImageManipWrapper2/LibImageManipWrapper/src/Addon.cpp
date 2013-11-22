// © 2001 Michael Pfeiffer

/*
#include <be/interface/Menu.h>
#include <be/interface/MenuItem.h>
#include <be/interface/PopUpMenu.h>
#include <be/interface/MenuField.h>
#include <be/interface/CheckBox.h>
#include <be/interface/StringView.h>
#include <be/support/String.h>
#include <be/app/MessageFilter.h>
*/
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <CheckBox.h>
#include <StringView.h>
#include <String.h>
#include <MessageFilter.h>
#include <Button.h>
#include <Window.h>

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#ifdef IMAGEMANIP_WEAK_LINKAGE
#undef _IMPEXP_IMAGEMANIP // for weak linkage
#endif
#include "ImageManip.h"
#include "BBitmapAccessor.h"

#define MSG_FILTER_SELECTED     'fsel'
#define MSG_FILTER_PREVIEW      'prev'
#define MSG_APPLY_TO_SELECTION  'aply'

#ifdef MANIPULATOR
#define WINDOW_TITLE       "Manipulate %s"
#define NAME               "Filter"
#define MENU_FIELD         "Filter:"
#define POPUP_NAME         "Select Filter"
#define WRAPPER_TYPE  	   BECASSO_FILTER
#define PREVIEW            PREVIEW_2x2
#define APPLY_TO_SELECTION "Apply filter to selection too"
#endif

#ifdef CONVERTER
#define WINDOW_TITLE       "Convert %s"
#define NAME               "Generator"
#define MENU_FIELD         "Generator:"
#define POPUP_NAME         "Select Generator"
#define WRAPPER_TYPE  	   BECASSO_TRANSFORMER
#define PREVIEW            PREVIEW_2x2
#define APPLY_TO_SELECTION "Apply transformer to selection too"
#endif

class WrapperView;

static int32             mAddonId = -1;
static bool              mApplyToSelection = false;
static WrapperView      *mView = NULL;

#define STATUS_TEXT_HEIGHT 20

class WrapperView : public BView
{
	void SetupFilters(BMenu* sub_menu);
	BView *mView;
	BMenuField *mMenuField;
	int mTop, mWidth; 
	BStringView *mStatus;

public:
	WrapperView (BRect rect);
	virtual	~WrapperView () {};

	void MessageReceived(BMessage* msg);
	void RemoveView();
	void SetStatus(const char* text);
	void SetupConfigView(int32 addon, const char* name);
};

WrapperView::WrapperView (BRect rect) : BView(rect, "", B_FOLLOW_NONE, 0) {
	mView = NULL;
	BMenu *sub_menu = new BPopUpMenu(POPUP_NAME);
	// mHeight = rect.IntegerHeight(); 
	mWidth = rect.IntegerWidth();
	mWidth = 270;

	mTop = 0;
	mMenuField = new BMenuField(BRect(5, mTop, 200, mTop+30), NAME, MENU_FIELD, sub_menu);
	mMenuField->SetDivider(70);
	AddChild(mMenuField);
	AddChild(new BButton(BRect(205, mTop, 265, mTop + 20), "", "Preview", new BMessage(MSG_FILTER_PREVIEW)));
	mTop += 32; 
	AddChild(new BCheckBox(BRect(5, mTop, 265, mTop + 20), "", APPLY_TO_SELECTION, new BMessage(MSG_APPLY_TO_SELECTION))); 
	mTop += 22;
	ResizeTo(mWidth, mTop + STATUS_TEXT_HEIGHT);

	mStatus = new BStringView(BRect(5, mTop, 265, mTop + 16), "", "Status", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(mStatus);
	
	SetupFilters(sub_menu);	
};


void WrapperView::SetupFilters(BMenu* sub_menu) {
	// Create bitmap accessor object to pass along, so that we
	// only get the add-ons that support this bitmap in the menu
	BBitmap bitmap(BRect(0, 0, 100, 20), B_RGB32);
	BBitmapAccessor* accessor = Image_CreateBBitmapAccessor(&bitmap);
	if (accessor == NULL) {
		sub_menu->SetEnabled(false); return;
	}
	accessor->SetDispose(false);

	// Create sub menu with image manipulators
	const char *addonName;
	const char *addonInfo;
	const char *addonCategory;
	int32 addonVersion;
	image_addon_id *outList;
	int32 outCount = 0;
	BMenuItem *item;
#ifdef MANIPULATOR
	if (Image_GetManipulators(accessor, NULL, &outList, &outCount) == B_OK)
#endif
#ifdef CONVERTER
	if (Image_GetConverters(accessor, NULL, &outList, &outCount) == B_OK)
#endif
	{
		mAddonId = outList[0];
		
		for (int i = outCount-1; i >= 0; --i)
		{
			if (Image_GetAddonInfo(outList[i], &addonName, &addonInfo,
					&addonCategory, &addonVersion) == B_OK)
			{
				BMessage *msg = new BMessage(MSG_FILTER_SELECTED);
				msg->AddInt32("addon_id", outList[i]);
				msg->AddString("addon_name", addonName);
				item = new BMenuItem(addonName, msg);
				sub_menu->AddItem(item);
				if (i == outCount-1) SetupConfigView(outList[i], addonName);
			}
		}
		delete[] outList;
	}
	if (outCount == 0)
		sub_menu->SetEnabled(false);
	else 
	{
		sub_menu->ItemAt((int32)0)->SetMarked(true);
	}
	delete accessor;
}


void WrapperView::RemoveView() {
	if (mView) {
		RemoveChild(mView);
		delete mView; mView = NULL;
	}
}


void WrapperView::SetupConfigView(int32 addon, const char* name) {
	mAddonId = addon;
	RemoveView();
	int width;
	BMessage ioExt, cap;
	status_t s = Image_GetConfigurationMessage(mAddonId, &ioExt, &cap);
	if (Image_MakeConfigurationView(mAddonId, &ioExt, &mView) == 0 && mView) {
		AddChild(mView); 
		int width = max_c(mWidth, mView->Bounds().Width()) + 10;
		mView->MoveTo((width-mView->Bounds().Width()) / 2, mTop);
		ResizeTo(width, mTop + STATUS_TEXT_HEIGHT + mView->Bounds().Height());
	} else {
		ResizeTo(mWidth, mTop + STATUS_TEXT_HEIGHT);
		width = mWidth;
	}
}


void WrapperView::MessageReceived(BMessage* msg) {
	switch(msg->what) {
		case MSG_FILTER_SELECTED: {
			BString name;
			SetStatus("Status");
			msg->FindString("addon_name", &name);
			SetupConfigView(msg->FindInt32("addon_id"), name.String());
			BWindow* window = Window();
			window->Lock();
			window->PostMessage(ADDON_RESIZED);
			window->Unlock();
			break; }
		case MSG_FILTER_PREVIEW:
			addon_preview();
			break;
		case MSG_APPLY_TO_SELECTION: {
			int32 s;
				if (B_OK == msg->FindInt32("be:value", &s)) {
					mApplyToSelection = (bool)s;
				}
			}
			break;
		default:
			BView::MessageReceived(msg);
	}
}


void WrapperView::SetStatus(const char* text) {
	mStatus->SetText(text);
}


status_t addon_make_config (BView **view, BRect rect)
{
	*view = mView = new WrapperView(rect); 
	return B_OK;
}

status_t addon_init (uint32 index, becasso_addon_info *info)
{
	strcpy (info->name, "LibImageManip Wrapper");
	strcpy (info->author, "Michael Pfeiffer");
	strcpy (info->copyright, "© 2001 under GPL");
	strcpy (info->description, "Wrapper to LibImageManip");
	info->type              = WRAPPER_TYPE;
	info->index				= index;
	info->version			= 0;
	info->release			= 4;
	info->becasso_version	= 2;
	info->becasso_release	= 0;
	info->does_preview		= PREVIEW;
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
	if (Image_Init(NULL) != B_OK)
	{
		puts("Cannot initialize libimagemanip.so");
		return B_ERROR;
	}

	// Check the library's version
	puts(Image_Version(&curVersion, &minVersion));
	if (curVersion < IMAGE_LIB_MIN_VERSION)
	{
		puts("Need a newer version of libimagemanip.so");
		return B_ERROR;
	}
	if (minVersion > IMAGE_LIB_CUR_VERSION)
	{
		puts("Need an older version of libimagemanip.so");
		return B_ERROR;
	}
#endif
	return (0);
}

status_t addon_exit (void)
{
	// We have to remove the config view from Imagelibmanip otherwise we
	// crash when the destructor of this window is called!
    // When IMAGE_WEAK_LINKAGE is defined this is no issue and the 
    // Imagelibmanip is always shutdown!

	// Shutdown the image manipulation library
#if defined(MANIPULATOR) || defined(IMAGEMANIP_WEAK_LINKAGE)
	// Note: We assume that the addon_exit of the transformer is called
	//       after the filter add-on.
	Image_Shutdown();
#endif	
	return (0);
}


status_t addon_close ()
{
	BWindow* window = mView->Window();
	window->Lock();
	mView->RemoveView();
	window->Unlock();
	return (0);
}

static void SetStatus(const char* text) {
	BWindow* window = mView->Window();
	window->Lock();
	mView->SetStatus(text);
	window->Unlock();
}


static void CopySelection(BBitmap* src, BBitmap* dst, int x, int y) {
	dst->Lock();
	BView* view = new BView(dst->Bounds(), "tmp", 0, 0);
	dst->AddChild(view);
	view->DrawBitmap(src, BPoint(-x, -y));
	view->Sync();
	dst->RemoveChild(view);
	delete view;
	dst->Unlock();
}


status_t process (Layer *inLayer, Selection *inSelection, 
			 Layer **outLayer, Selection **outSelection, int32 mode,
			 BRect * frame, bool final, BPoint /* point */, uint32 /* buttons */)
{
	int error = ADDON_OK;

	BRect bounds = inLayer->Bounds();

//	printf ("Bounds: ");
//	bounds.PrintToStream();
//	printf ("Frame:  ");
//	frame->PrintToStream();
//	fprintf(stderr, "mode = %d final = %d\n", mode, final);
//	fprintf(stderr, "in %x out %x in %x out %x\n", inLayer, *outLayer, inSelection, *outSelection);

// #ifdef MANIPULATOR
	if (*outLayer == NULL && mode == M_DRAW) {
		*outLayer = new Layer (*inLayer);
	}
	if (*outSelection == NULL && mode == M_DRAW) {
		*outSelection = inSelection;
	}
	
	if (mode == M_SELECT)
	{
		if (inSelection)
		{
		 	if (*outSelection == NULL)
				*outSelection = new Selection (*inSelection);
		}
		else	// No Selection to filter!
			return (0);
	}
	if (*outLayer)
		(*outLayer)->Lock();
		
	if (*outSelection)
		(*outSelection)->Lock();
// #endif

//	fprintf(stderr, "in %x out %x in %x out %x\n", inLayer, *outLayer, inSelection, *outSelection);
		
	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;

	if (final)
	{
//		addon_start();
	} 
	else
	{
//		return error;
	} 
	
	BMessage ioExt, m;
	Image_GetConfigurationMessage(mAddonId, &ioExt, &m);
//	ioExt.PrintToStream();
//	m.PrintToStream();
	if (frame->IntegerWidth() == 0 || frame->IntegerHeight() == 0) {
		ioExt.AddRect("selection_rect", bounds);
	} else {
		// frame->PrintToStream();
		ioExt.AddRect("selection_rect", *frame);
	}
	switch (mode)
	{
	#ifdef MANIPULATOR // Becasso Filter
		case M_DRAW:
		{
			// Create bitmap accessor object
			memcpy((*outLayer)->Bits(), inLayer->Bits(), inLayer->BitsLength());
			Layer* layer;
			if (inSelection) {
				layer = new Layer(*inLayer);
				memcpy(layer->Bits(), inLayer->Bits(), inLayer->BitsLength());
			} else {
				layer = *outLayer;
			}
			BBitmapAccessor* src_accessor = Image_CreateBBitmapAccessor(layer);
			if (src_accessor == NULL) {
				error = ADDON_ABORT;;
				if (inSelection) delete layer;
				break;
			}
			src_accessor->SetDispose(false);
			
			// manipulate layer
			if (Image_Manipulate(mAddonId, src_accessor, &ioExt, false) != B_OK) {
				SetStatus("Error: Could not manipulate layer!");
				error = ADDON_UNKNOWN;
			}
			delete src_accessor;
			
			// manipulate selection			
			if (error == ADDON_OK && mApplyToSelection && inSelection) {
				memcpy((*outSelection)->Bits(), inSelection->Bits(), inSelection->BitsLength());
				src_accessor = Image_CreateBBitmapAccessor(*outSelection);
				src_accessor->SetDispose(false);
				if (Image_Manipulate(mAddonId, src_accessor, &ioExt, false) != B_OK) {
					SetStatus("Error: Could not manipulate selection!");
				}
				delete src_accessor;
			}

			// return selection of layer only
			if (error == ADDON_OK && inSelection && *outSelection) {
				CutOrCopy(layer, layer, *outSelection, 0, 0, false);
				AddWithAlpha(layer, *outLayer, 0, 0);
			}

			if (inSelection) {
				delete layer;
			}			
		break;
		}
		case M_SELECT:
		{
			// Create bitmap accessor object
			memcpy((*outSelection)->Bits(), inSelection->Bits(), inSelection->BitsLength());
			BBitmapAccessor* src_accessor = Image_CreateBBitmapAccessor(*outSelection);
			if (src_accessor == NULL) {
				error = ADDON_ABORT;;
				break;
			}
			src_accessor->SetDispose(false);

			if (Image_Manipulate(mAddonId, src_accessor, &ioExt, false) < 0) {
				//window->Lock();
				//window->SetStatus("Error: Could not manipulate selection!");
				//window->Unlock();
			}
			delete src_accessor;
		break;
		}
	#endif
	
	
	#ifdef CONVERTER // Becasso Transformer
		case M_DRAW:
		{
			// Create bitmap accessor object
			Layer* layer;
			if (inSelection) {
				layer = new Layer(*inLayer);
				memcpy((*outLayer)->Bits(), inLayer->Bits(), inLayer->BitsLength());
			} else {
				layer = *outLayer;
			}


			BBitmapAccessor* src_accessor = Image_CreateBBitmapAccessor(inLayer);
			if (src_accessor == NULL) {
				error = ADDON_ABORT;;
				break;
			}
			src_accessor->SetDispose(false);

			BBitmapAccessor* dst_accessor = Image_CreateBBitmapAccessor();
			dst_accessor->SetDispose(true);
			
			// convert layer
			if (Image_Convert(mAddonId, src_accessor, dst_accessor, &ioExt, false) != B_OK) {
				SetStatus("Error: Could not convert layer!");
				error = ADDON_UNKNOWN;
			}
			delete src_accessor;
			
			int dx = -(bounds.IntegerWidth() - dst_accessor->Bounds().IntegerWidth()) / 2; 
			int dy = -(bounds.IntegerHeight() - dst_accessor->Bounds().IntegerHeight()) / 2; 
			CutOrCopy(dst_accessor->Bitmap(), layer, NULL, dx, dy, false);
			delete dst_accessor;
						
			// manipulate selection			
			if (error == ADDON_OK && mApplyToSelection && inSelection) {
				src_accessor = Image_CreateBBitmapAccessor(inSelection);
				src_accessor->SetDispose(false);
				dst_accessor = Image_CreateBBitmapAccessor();
				dst_accessor->SetDispose(true);
				if (Image_Convert(mAddonId, src_accessor, dst_accessor, &ioExt, false) != B_OK) {
					SetStatus("Error: Could not manipulate selection!");
				}
				delete src_accessor;
				CopySelection(dst_accessor->Bitmap(), *outSelection,dx, dy);
				delete dst_accessor;
			}

			// return selection of layer only
			if (error == ADDON_OK && inSelection && *outSelection) {
				CutOrCopy(layer, layer, *outSelection, 0, 0, false);
				AddWithAlpha(layer, *outLayer, 0, 0);
			}

			if (inSelection) {
				delete layer;
			}

		break;
		}
		case M_SELECT:
		{
			// Create bitmap accessor object
			BBitmapAccessor* src_accessor = Image_CreateBBitmapAccessor(inSelection);
			if (src_accessor == NULL) {
				error = ADDON_ABORT;;
				break;
			}
			src_accessor->SetDispose(false);
			BBitmapAccessor* dst_accessor = Image_CreateBBitmapAccessor();
			dst_accessor->SetDispose(true);
			
			if (Image_Convert(mAddonId, src_accessor, dst_accessor, &ioExt, false) != B_OK) {
				SetStatus("Error: Could not convert selection!");
				error = ADDON_ABORT;;
			} else {
				int dx = -(bounds.IntegerWidth() - dst_accessor->Bounds().IntegerWidth()) / 2; 
				int dy = -(bounds.IntegerHeight() - dst_accessor->Bounds().IntegerHeight()) / 2; 
				CopySelection(dst_accessor->Bitmap(), *outSelection, dx, dy);
			}
			delete src_accessor; delete dst_accessor;
		break;
		}
#endif 
	
	default:
		fprintf (stderr, "Wrapper: Unknown mode\n");
		error = ADDON_UNKNOWN;
	}

// #ifdef MANIPULATOR
	if (*outSelection)
		(*outSelection)->Unlock();
	if (*outLayer)
		(*outLayer)->Unlock();
// #endif
	
	if (final)
	{
//		addon_done();
	}
	return (error);
}
