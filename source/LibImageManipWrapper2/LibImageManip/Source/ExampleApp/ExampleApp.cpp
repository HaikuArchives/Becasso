/*
 *  ExampleApp.cpp
 *  Release 1.0.0 (Oct 24th, 1999)
 *
 *  Example application for using the image manipulation library
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 *  Public domain
 */


// Includes
#include <stdio.h>
#include <string.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Message.h>
#include <Alert.h>
#include <Bitmap.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <Button.h>
#include <Rect.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <File.h>
#include <FilePanel.h>
#include <Screen.h>
#include <TranslationUtils.h>
#include <String.h>
#include "ImageManip.h"
#include "BBitmapAccessor.h"


// Defines
#define WINDOW_CLOSED 'winc'
#define SHOW_FILE_PANEL 'shfp'
#define IMAGE_MANIP 'mnip'
#define IMAGE_CONV 'conv'
#define IMAGE_CONF_MANIP 'cnfm'
#define IMAGE_CONF_CONV 'cnfc'

// MyFilePanel class
class MyFilePanel : public BFilePanel {
	virtual void WasHidden()
	{
		be_app->PostMessage(WINDOW_CLOSED);
		BFilePanel::WasHidden();
	};
};

// ExampleApp class
class ExampleApp : public BApplication {
  public:
	ExampleApp();
	virtual void ReadyToRun(void);
	virtual void AboutRequested();
	virtual bool QuitRequested(void);
	virtual void MessageReceived(BMessage* msg);
	virtual void RefsReceived(BMessage* message);

  private:
	MyFilePanel* mOpenPanel;
};

// BitmapView class
class BitmapView : public BView {
  public:
	BitmapView(BBitmap* bitmap);
	~BitmapView();
	virtual void Draw(BRect update);
	virtual void AttachedToWindow();
	virtual void FrameResized(float width, float height);
	BBitmap* Bitmap() const;

  private:
	BBitmap* mBitmap;
	float mBitmapWidth;
	float mBitmapHeight;
	BScrollBar* mHorBar;
	BScrollBar* mVerBar;
};

// ImageWindow class
class ImageWindow : public BWindow {
  public:
	ImageWindow(const char* title, BBitmap* bitmap);
	virtual bool QuitRequested(void);
	virtual void MessageReceived(BMessage* msg);

	static int sWindowCount;

  private:
	BMenu* CreateFileMenu(BBitmap* bitmap);
	static void ImageManip_entry(ImageWindow* obj);
	void ImageManip();
	static void ImageConvert_entry(ImageWindow* obj);
	void ImageConvert();
	void PrintAddonInfo();

	BitmapView* mBitmapView;
	bool mBitmapInUse;
	image_addon_id mAddonId;
};

// ConfigWindow class
class ConfigWindow : public BWindow {
	BMessenger* mWindow;
	int mAddonId;
	int32 mWhat;

  public:
	ConfigWindow(int addonId, int32 what, const char* title, ImageWindow* win);
	~ConfigWindow();
	void MessageReceived(BMessage* msg);
};

ConfigWindow::ConfigWindow(int addonId, int32 what, const char* title, ImageWindow* win)
	: BWindow(BRect(40, 40, 400, 600), "", B_TITLED_WINDOW, B_NOT_RESIZABLE), mAddonId(addonId),
	  mWhat(what)
{
	BString s("Configure ");
	s += title;
	SetTitle(s.String());
	mWindow = new BMessenger(win);
	BView* view;
	Image_MakeConfigurationView(addonId, NULL, &view);
	AddChild(view);
	BRect r(view->Bounds());
	BButton* apply = new BButton(
		BRect(0, r.bottom + 5, 40, r.bottom + 25), "apply", "Apply", new BMessage('aply')
	);
	AddChild(apply);
	ResizeTo(r.Width(), r.Height() + 40);
	Show();
}

ConfigWindow::~ConfigWindow() { delete mWindow; }

void
ConfigWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'aply': {
		BMessage m(mWhat);
		m.AddInt32("addon_id", mAddonId);
		mWindow->SendMessage(&m);
	} break;
	default:
		BWindow::MessageReceived(msg);
	}
}

// Where it all starts
int
main()
{
	// Initialize the image manipulation library
	if (Image_Init(NULL) != B_OK) {
		puts("Cannot initialize libimagemanip.so");
		return B_ERROR;
	}

	// Check the library's version
	int32 curVersion;
	int32 minVersion;
	puts(Image_Version(&curVersion, &minVersion));
	if (curVersion < IMAGE_LIB_MIN_VERSION) {
		puts("Need a newer version of libimagemanip.so");
		return B_ERROR;
	}
	if (minVersion > IMAGE_LIB_CUR_VERSION) {
		puts("Need an older version of libimagemanip.so");
		return B_ERROR;
	}

	// Create application object and start it
	ExampleApp theApp;
	theApp.Run();

	// Shutdown the image manipulation library
	Image_Shutdown();

	return B_OK;
}

// Application constructor
ExampleApp::ExampleApp() : BApplication("application/x-vnd.imagemanip-exampleapp"), mOpenPanel(NULL)
{
}

// Ready to run
void
ExampleApp::ReadyToRun(void)
{
	// Open file panel if there are no windows open
	if (ImageWindow::sWindowCount == 0)
		PostMessage(SHOW_FILE_PANEL);
}

// About requested
void
ExampleApp::AboutRequested()
{
	(new BAlert(
		 "About",
		 "ImageManipExample 1.0.0 (Oct 24th, 1999)\n\n"
		 "Example application for using the image manipulation library "
		 "'libimagemanip.so'\n\n"
		 "Public domain\n\n"
		 "Written by Edmund Vermeulen\n"
		 "E-mail: edmundv@xs4all.nl\n"
		 "Home: http://www.xs4all.nl/~edmundv/",
		 "OK"
	 ))
		->Go(NULL);
}

// Quit requested
bool
ExampleApp::QuitRequested(void)
{
	// Make sure that the windows close first
	if (BApplication::QuitRequested()) {
		delete mOpenPanel;
		return true;
	}

	return false;
}

// Handle messages
void
ExampleApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case SHOW_FILE_PANEL:
		if (!mOpenPanel)
			mOpenPanel = new MyFilePanel();
		mOpenPanel->Show();
		break;

	case B_SIMPLE_DATA: // drag and drop
		if (msg->HasRef("refs"))
			RefsReceived(msg);
		break;

	case WINDOW_CLOSED:
		// Last window closed?
		if (ImageWindow::sWindowCount == 0 && (!mOpenPanel || !mOpenPanel->IsShowing())) {
			PostMessage(B_QUIT_REQUESTED);
		}
		break;

	default:
		BApplication::MessageReceived(msg);
		break;
	}
}

// Handle opened files
void
ExampleApp::RefsReceived(BMessage* msg)
{
	entry_ref the_ref;
	for (int32 i = 0; msg->FindRef("refs", i, &the_ref) == B_OK; ++i) {
		char file_name[B_FILE_NAME_LENGTH];
		strncpy(file_name, the_ref.name, B_FILE_NAME_LENGTH);

		BFile* the_file = new BFile(&the_ref, B_READ_ONLY);
		BBitmap* mBitmap = BTranslationUtils::GetBitmap(the_file);
		if (mBitmap) {
			// Open new window with bitmap
			new ImageWindow(file_name, mBitmap);
		}
		else {
			char alert_string[256];
			sprintf(alert_string, "Couldn't get bitmap for '%s'.", file_name);
			(new BAlert("Error", alert_string, "OK"))->Go();
		}
	}
}

// Bitmap view constructor
BitmapView::BitmapView(BBitmap* bitmap)
	: BView(bitmap->Bounds(), "bitmap", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
	  mBitmap(bitmap)
{
	// No stinkin' background colour fills!
	SetViewColor(B_TRANSPARENT_32_BIT);

	mBitmapWidth = mBitmap->Bounds().Width();
	mBitmapHeight = mBitmap->Bounds().Height();
}

// Bitmap view destructor
BitmapView::~BitmapView()
{
	// Deallocate bitmap
	delete mBitmap;
}

// Blit bitmap
void
BitmapView::Draw(BRect update)
{
	DrawBitmap(mBitmap, update, update);
}

// Find scroll bars
void
BitmapView::AttachedToWindow()
{
	mHorBar = ScrollBar(B_HORIZONTAL);
	mVerBar = ScrollBar(B_VERTICAL);

	BView::AttachedToWindow();
}

// Adjust scroll bars
void
BitmapView::FrameResized(float width, float height)
{
	mHorBar->SetProportion(width / mBitmapWidth);
	mVerBar->SetProportion(height / mBitmapHeight);

	mHorBar->SetRange(0.0, mBitmapWidth - width);
	mVerBar->SetRange(0.0, mBitmapHeight - height);

	mHorBar->SetSteps(8.0, width);
	mVerBar->SetSteps(8.0, height);

	BView::FrameResized(width, height);
}

// Get pointer to bitmap
BBitmap*
BitmapView::Bitmap() const
{
	return mBitmap;
}

// Window constructor
ImageWindow::ImageWindow(const char* title, BBitmap* bitmap)
	: BWindow(BRect(80.0, 26.0, 80.0, 26.0), title, B_DOCUMENT_WINDOW, 0), mBitmapInUse(false)
{
	++sWindowCount;

	// Create menu bar
	BMenuBar* menu_bar = new BMenuBar(BRect(0.0, 0.0, 0.0, 0.0), "menu");
	menu_bar->AddItem(CreateFileMenu(bitmap));

	// Add menu bar to window
	AddChild(menu_bar);

	// Create bitmap view
	mBitmapView = new BitmapView(bitmap);

	// Create scrolling view
	BScrollView* scroll_view = new BScrollView(
		"scroll", mBitmapView, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, true, true, B_NO_BORDER
	);
	scroll_view->MoveTo(0.0, menu_bar->Frame().bottom + 1.0);

	// Set initial window size
	BRect win_rect = menu_bar->Frame() | scroll_view->Frame();
	float width = win_rect.Width() - 1.0;
	float height = win_rect.Height() - 1.0;

	// Set window size limits
	float min_width = 100.0;
	if (width < min_width)
		min_width = width;

	float min_height = 65.0;
	if (height < min_height)
		min_height = height;

	SetSizeLimits(min_width, width, min_height, height);
	ResizeTo(width, height);

	// Set scroll bars to full size
	scroll_view->ScrollBar(B_HORIZONTAL)->SetRange(0, 0);
	scroll_view->ScrollBar(B_VERTICAL)->SetRange(0, 0);

	// Add scroll view to the window
	AddChild(scroll_view);

	// Show window
	Show();

	// Get screen size
	BScreen screen;
	float screen_width = screen.Frame().Width();
	float screen_height = screen.Frame().Height();

	// Resize window if it doesn't fit on screen
	if (width + Frame().left > screen_width)
		width = screen_width - Frame().left - 7.0;
	if (height + Frame().top > screen_height)
		height = screen_height - Frame().top - 7.0;
	ResizeTo(width, height);
}

// Quit requested
bool
ImageWindow::QuitRequested(void)
{
	// Bitmap still in use?
	if (mBitmapInUse)
		return false;

	// Close window
	--sWindowCount;
	be_app->PostMessage(WINDOW_CLOSED);
	return true;
}

// Handle messages to the window
void
ImageWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case B_SIMPLE_DATA: // drag and drop
		if (msg->HasRef("refs"))
			be_app->RefsReceived(msg);
		break;

	case IMAGE_CONF_MANIP:
	case IMAGE_CONF_CONV:
		if (!mBitmapInUse) {
			BString name;
			msg->FindString("addon_name", &name);
			mAddonId = msg->FindInt32("addon_id");
			new ConfigWindow(
				mAddonId, msg->what == IMAGE_CONF_MANIP ? IMAGE_MANIP : IMAGE_CONV, name.String(),
				this
			);
		}
		break;

	case IMAGE_MANIP:
		if (!mBitmapInUse) {
			mAddonId = msg->FindInt32("addon_id");

			resume_thread(
				spawn_thread((thread_entry)ImageManip_entry, "ImageManip", B_NORMAL_PRIORITY, this)
			);
		}
		break;

	case IMAGE_CONV:
		if (!mBitmapInUse) {
			mAddonId = msg->FindInt32("addon_id");

			resume_thread(spawn_thread(
				(thread_entry)ImageConvert_entry, "ImageConvert", B_NORMAL_PRIORITY, this
			));
		}
		break;

	default:
		BWindow::MessageReceived(msg);
		break;
	}
}

// Initialise instance counter
int ImageWindow::sWindowCount = 0;

// Create file menu
BMenu*
ImageWindow::CreateFileMenu(BBitmap* bitmap)
{
	BMenu* menu = new BMenu("File");

	BMenuItem* item = new BMenuItem("About", new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	menu->AddItem(item);
	menu->AddSeparatorItem();

	item = new BMenuItem("Open…", new BMessage(SHOW_FILE_PANEL), 'O');
	item->SetTarget(be_app);
	menu->AddItem(item);
	menu->AddSeparatorItem();

	// Create bitmap accessor object to pass along, so that we
	// only get the add-ons that support this bitmap in the menu
	BBitmapAccessor accessor(bitmap);
	accessor.SetDispose(false);

	// Create sub menu with image manipulators
	BMenu* sub_menu = new BMenu("Manipulate Image");
	const char* addonName;
	const char* addonInfo;
	const char* addonCategory;
	int32 addonVersion;
	image_addon_id* outList;
	int32 outCount = 0;
	if (Image_GetManipulators(&accessor, NULL, &outList, &outCount) == B_OK) {
		for (int i = 0; i < outCount; ++i) {
			if (Image_GetAddonInfo(
					outList[i], &addonName, &addonInfo, &addonCategory, &addonVersion
				) == B_OK) {
				BMessage* msg = new BMessage(IMAGE_CONF_MANIP);
				msg->AddInt32("addon_id", outList[i]);
				msg->AddString("addon_name", addonName);
				item = new BMenuItem(addonName, msg);
				sub_menu->AddItem(item);
			}
		}
		delete[] outList;
	}
	if (outCount == 0)
		sub_menu->SetEnabled(false);
	item = new BMenuItem(sub_menu);
	menu->AddItem(item);

	// Create sub menu with image converters
	sub_menu = new BMenu("Convert Image");
	outCount = 0;
	if (Image_GetConverters(&accessor, NULL, &outList, &outCount) == B_OK) {
		for (int i = 0; i < outCount; ++i) {
			if (Image_GetAddonInfo(
					outList[i], &addonName, &addonInfo, &addonCategory, &addonVersion
				) == B_OK) {
				BMessage* msg = new BMessage(IMAGE_CONF_CONV);
				msg->AddInt32("addon_id", outList[i]);
				msg->AddString("addon_name", addonName);
				item = new BMenuItem(addonName, msg);
				sub_menu->AddItem(item);
			}
		}
		delete[] outList;
	}
	if (outCount == 0)
		sub_menu->SetEnabled(false);
	item = new BMenuItem(sub_menu);
	menu->AddItem(item);
	menu->AddSeparatorItem();

	item = new BMenuItem("Close Window", new BMessage(B_CLOSE_REQUESTED), 'W');
	menu->AddItem(item);
	item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	item->SetTarget(be_app);
	menu->AddItem(item);

	return menu;
}

// Image manipulate
void
ImageWindow::ImageManip_entry(ImageWindow* obj)
{
	obj->ImageManip();
}

void
ImageWindow::ImageManip()
{
	mBitmapInUse = true;

	PrintAddonInfo();

	// Create bitmap accessor object
	BBitmapAccessor src_accessor(mBitmapView->Bitmap());
	src_accessor.SetDispose(false);

	// Flippin' do it!
	Image_Manipulate(mAddonId, &src_accessor, 0);

	// Invalidate bitmap view, so that it will redraw itself
	Lock();
	mBitmapView->Invalidate();
	Unlock();

	mBitmapInUse = false;
}

// Image convert
void
ImageWindow::ImageConvert_entry(ImageWindow* obj)
{
	obj->ImageConvert();
}

void
ImageWindow::ImageConvert()
{
	mBitmapInUse = true;

	PrintAddonInfo();

	// Create bitmap accessor object
	BBitmapAccessor src_accessor(mBitmapView->Bitmap());
	src_accessor.SetDispose(false);

	// Create empty bitmap accessor object for the destination
	BBitmapAccessor dest_accessor;

	// Do image conversion
	Image_Convert(mAddonId, &src_accessor, &dest_accessor, NULL);

	// Got valid new bitmap?
	if (dest_accessor.IsValid()) {
		// Open it in a new window
		new ImageWindow(Title(), dest_accessor.Bitmap());

		// Don't dispose it
		dest_accessor.SetDispose(false);
	}

	mBitmapInUse = false;
}

// Print add-on info
void
ImageWindow::PrintAddonInfo()
{
	const char* addonName;
	const char* addonInfo;
	const char* addonCategory;
	int32 addonVersion;
	if (Image_GetAddonInfo(mAddonId, &addonName, &addonInfo, &addonCategory, &addonVersion) ==
		B_OK) {
		int32 ver = addonVersion / 100;
		int32 rev1 = (addonVersion % 100) / 10;
		int32 rev2 = addonVersion % 10;

		printf(
			"Using '%s' version %ld.%ld.%ld\nCategory: %s\n%s\n", addonName, ver, rev1, rev2,
			addonCategory, addonInfo
		);
	}
}
