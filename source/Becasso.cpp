#include <Rect.h>
#if defined(DATATYPES)
#include <Datatypes.h>
#endif
#include <Entry.h>
#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <FilePanel.h>
#include "ThumbnailFilePanel.h"
#include <Alert.h>
#include <ClassInfo.h>
#include <List.h>
#include <device/SerialPort.h>
#include "PrintJob.h"
#include "Tablet.h"
#include "AddOn.h"
#include "AddOnWindow.h"
#include "AttribWindow.h"
#include "CanvasWindow.h"
#include "MainWindow.h"
#include "SizeWindow.h"
#include "PrefsWindow.h"
#include "LayerWindow.h"
#include "MagWindow.h"
#include "Becasso.h"
#include "AboutWindow.h"
#include "AboutView.h"
#include "SplashWindow.h"
#include "DragWindow.h"
#include "BBP.h"
#include "Position.h"
#if defined(EASTER_EGG_SFX)
#include "sfx.h"
#endif
#include "PicMenuButton.h" // for the scripting
#include "ColorMenuButton.h"
#include "Properties.h"
#include "debug.h"
#include <Resources.h>
#include <string.h>
#include <Screen.h>
#include <TranslatorRoster.h>
#include <Roster.h>
#include <PropertyInfo.h>
#include "TOTDWindow.h"
#include "Settings.h"

#define MAXTITLE 128
#define SHOW_EXTRA_COLOR_INFO 0

bool DataTypes;
MainWindow* mainWindow;
ThumbnailFilePanel* openPanel;
entry_ref gSaveRef;
BMessage* printSetup;
int newnum;
BBitmap* clip;
long pasteX, pasteY;
long fPasteX, fPasteY;
bool inPaste, inDrag;
const char* Version = "2.0";
unsigned char** RGBColors;
int32 NumColors;
BList* AddOns;
Tablet* wacom;
port_id position_port;
BLocker* clipLock;
#if defined(EASTER_EGG_SFX)
EffectsPlayer* easterEgg;
SoundEffect8_11* fxTear;
#endif
int32 def_out_type;
translator_id def_out_translator;

//__declspec (dllexport) const int32 NumTools = 13;
//__declspec (dllexport) const int32 NumModes = 2;
const char* ModeSpecifiers[NumModes] = {"Draw", "Select"};
const char* ToolSpecifiers[NumTools] = {"Brush",	  "Eraser",	  "Fill",		"Text",
										"Spray Can",  "Clone",	  "Freehand",	"Lines",
										"Free Shape", "Polygons", "Rectangles", "Ovals",
										"Circles",	  "Ellipses"};

uchar cross[3][68] = {
	{16,											 // Size
	 1,												 // Bit depth
	 7,	   7,										 // hot spot
	 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, // Image
	 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7C, 0x7C, 0x00, 0x00, 0x01, 0x00,
	 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, // Mask
	 0x03, 0x80, 0x03, 0x80, 0xFC, 0x7E, 0xFC, 0x7E, 0xFC, 0x7E, 0x03, 0x80,
	 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x00, 0x00},
	{16,   1,	 7,	   7,	 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x40, 0x20, 0x40,
	 0x18, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x30, 0x04, 0x08, 0x04, 0x00,
	 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0xF0,
	 0x70, 0xF0, 0x7D, 0xE0, 0x7F, 0xE0, 0x3F, 0xC0, 0x0E, 0x70, 0x07, 0xF8, 0x0F, 0xFc,
	 0x0F, 0x7C, 0x1E, 0x1C, 0x1E, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00},
	{16,   1,	 7,	   7,	 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x04, 0x08,
	 0x02, 0x30, 0x00, 0x40, 0x00, 0x00, 0x04, 0x00, 0x18, 0x00, 0x40, 0x80, 0x00, 0x40,
	 0x00, 0x40, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x1E, 0x00,
	 0x1E, 0x1C, 0x0F, 0x7C, 0x0F, 0xFC, 0x07, 0xF8, 0x0E, 0xE0, 0x3F, 0xC0, 0x7F, 0xE0,
	 0x7D, 0xE0, 0x70, 0xF0, 0x00, 0xE0, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00}
};

uchar ccross[68] = {16,												// Size,
					1,												// Bit depth,
					7,	  7,										// hot spot
					0x00, 0x00, 0x01, 0x00, 0x03, 0x80, 0x09, 0x20, // Image
					0x11, 0x10, 0x01, 0x00, 0x20, 0x08, 0x7C, 0x7C, 0x20, 0x08, 0x01, 0x00,
					0x11, 0x10, 0x09, 0x20, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x03, 0x80, 0x07, 0xC0, 0x0F, 0xE0, 0x1F, 0xF0, // Mask
					0x3B, 0xB8, 0x73, 0x9C, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0x73, 0x9C,
					0x3B, 0xB8, 0x1F, 0xF0, 0x0F, 0xE0, 0x07, 0xC0, 0x03, 0x80, 0x00, 0x00};

uchar scross[68] = {16,												// Size
					1,												// Bit depth
					7,	  7,										// hot spot
					0x00, 0x00, 0x03, 0x80, 0x02, 0x80, 0x02, 0x80, // Image
					0x02, 0x80, 0x01, 0x00, 0x78, 0x3C, 0x44, 0x44, 0x78, 0x3C, 0x01, 0x00,
					0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00,
					0x07, 0xC0, 0x07, 0xC0, 0x07, 0xC0, 0x07, 0xC0, // Mask
					0x07, 0xC0, 0xF8, 0x3E, 0xF8, 0x3E, 0xF8, 0x3E, 0xF8, 0x3E, 0xF8, 0x3E,
					0x07, 0xC0, 0x07, 0xC0, 0x07, 0xC0, 0x07, 0xC0, 0x07, 0xC0, 0x00, 0x00};

uchar hand[68] = {16,											  // Size
				  1,											  // Bit depth
				  7,	7,										  // hot spot
				  0x01, 0x80, 0x1A, 0x70, 0x26, 0x48, 0x26, 0x4A, // Image
				  0x12, 0x4D, 0x12, 0x49, 0x68, 0x09, 0x98, 0x01, 0x88, 0x02, 0x40, 0x02,
				  0x20, 0x02, 0x20, 0x04, 0x10, 0x04, 0x08, 0x08, 0x04, 0x08, 0x04, 0x08,
				  0x01, 0x80, 0x1B, 0xF0, 0x3F, 0xF8, 0x3F, 0xFA, // Mask
				  0x1F, 0xFF, 0x1F, 0xFF, 0x6F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x7F, 0xFE,
				  0x3F, 0xFE, 0x3F, 0xFC, 0x1F, 0xFC, 0x0F, 0xF8, 0x07, 0xF8, 0x07, 0xF8};

uchar grab[68] = {16,											  // Size
				  1,											  // Bit depth
				  7,	7,										  // hot spot
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Image
				  0x0D, 0xB0, 0x12, 0x4C, 0x10, 0x0A, 0x08, 0x02, 0x18, 0x02, 0x20, 0x02,
				  0x20, 0x02, 0x20, 0x04, 0x10, 0x04, 0x08, 0x08, 0x04, 0x08, 0x04, 0x08,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Mask
				  0x0D, 0xB0, 0x1F, 0xFC, 0x1F, 0xFE, 0x0F, 0xFE, 0x1F, 0xFE, 0x3F, 0xFE,
				  0x3F, 0xFE, 0x3F, 0xFC, 0x1F, 0xFC, 0x0F, 0xF8, 0x07, 0xF8, 0x07, 0xF8};

uchar picker[68] = {16,												// Size,
					1,												// Bit depth,
					14,	  1,										// hot spot
					0x00, 0x00, 0x00, 0x0C, 0x00, 0x1E, 0x00, 0x2E, // Image
					0x01, 0xBC, 0x01, 0x38, 0x00, 0xE0, 0x01, 0x70, 0x02, 0xB0, 0x05, 0x00,
					0x0A, 0x00, 0x14, 0x00, 0x28, 0x00, 0x50, 0x00, 0x60, 0x00, 0x00, 0x00,
					0x00, 0x1E, 0x00, 0x3F, 0x00, 0x7F, 0x03, 0xFF, // Mask
					0x03, 0xFF, 0x03, 0xFE, 0x03, 0xFC, 0x07, 0xF8, 0x0F, 0xF8, 0x1F, 0xF8,
					0x3F, 0x80, 0x7F, 0x00, 0xFE, 0x00, 0xFC, 0x00, 0xF8, 0x00, 0xF0, 0x00};

uchar mover[68] = {16,	 1,	   7,	 7,	   0x00, 0x00, 0x01, 0x00, 0x03, 0x80, 0x07, 0xC0,
				   0x01, 0x00, 0x11, 0x10, 0x31, 0x18, 0x7F, 0xFC, 0x31, 0x18, 0x11, 0x10,
				   0x01, 0x00, 0x07, 0xC0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
				   0x03, 0x80, 0x07, 0xC0, 0x07, 0xE0, 0x0F, 0xF0, 0x3F, 0xF8, 0x7B, 0xBC,
				   0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0x7B, 0xBC, 0x3F, 0xF8, 0x0F, 0xE0,
				   0x0F, 0xE0, 0x07, 0xC0, 0x03, 0x80, 0x00, 0x00};

uchar rotator[68] = {16,   1,	 7,	   7,	 0x00, 0x00, 0x03, 0x80, 0x0C, 0x60, 0x10, 0x10,
					 0x20, 0x08, 0x20, 0x08, 0x41, 0x04, 0x42, 0x84, 0x41, 0x04, 0x20, 0x00,
					 0x20, 0x00, 0x10, 0xF0, 0x0C, 0x38, 0x03, 0xD0, 0x00, 0x10, 0x00, 0x00,
					 0x07, 0xC0, 0x1F, 0xF0, 0x3F, 0xF8, 0x7E, 0xFC, 0x78, 0x3C, 0xF3, 0x9E,
					 0xF7, 0xDE, 0xE7, 0xCE, 0xF7, 0xCE, 0xF3, 0x8E, 0x79, 0xF8, 0x7F, 0xF8,
					 0x3F, 0xF8, 0x1F, 0xF8, 0x07, 0xF8, 0x00, 0x38};

#define GET_AND_SET                                                                                \
	{                                                                                              \
		B_GET_PROPERTY, B_SET_PROPERTY, 0                                                          \
	}
#define GET_SET_AND_PROP                                                                           \
	{                                                                                              \
		B_GET_PROPERTY, B_SET_PROPERTY, B_GET_SUPPORTED_SUITES, 0                                  \
	}
#define SET                                                                                        \
	{                                                                                              \
		B_SET_PROPERTY, 0                                                                          \
	}
#define DIRECT_AND_INDEX                                                                           \
	{                                                                                              \
		B_DIRECT_SPECIFIER, B_INDEX_SPECIFIER, 0                                                   \
	}
#define DIRECT                                                                                     \
	{                                                                                              \
		B_DIRECT_SPECIFIER, 0                                                                      \
	}

static property_info prop_list[] = {
	{"Tool", GET_SET_AND_PROP, DIRECT_AND_INDEX, "Get or set current tool"},
	{"Mode", GET_SET_AND_PROP, DIRECT_AND_INDEX, "Get or set current mode"},
	{"Foreground", SET, DIRECT, "By name or rgb_color"},
	{"Background", SET, DIRECT, "By name or rgb_color"},
	{"TabletArea", GET_AND_SET, DIRECT, "BRect (only useful with -t switch)"},
	{"ExportFormat", GET_AND_SET, DIRECT, "by MIME type or type code"},
	{"Scriptee", GET_AND_SET, DIRECT_AND_INDEX, "Get or set current scripting target (canvas)"},
	{"Canvas", {B_CREATE_PROPERTY, 0}, DIRECT, "Name (string), Size (BRect)"},
	0
};

static value_info value_list[] = {
	{"Export", 'expt', B_COMMAND_KIND, "Export the current canvas. Name|Filename (string)"},
	{"Crop", 'Crop', B_COMMAND_KIND, "Crop the current canvas to the given BRect"},
	0
};

static property_info prop_list_BBP[] = {{"", {0}, {0}, ""}, 0};

static value_info value_list_BBP[] = {{"BBP_*_BBITMAP", 'BPxx', B_COMMAND_KIND, "See BBP.h"}, 0};

Becasso::Becasso() : BApplication("application/x-sum-becasso")
{
	fBusy = 0;
	SetPulseRate(100000);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		BDirectory dir(path.Path());
		BDirectory dummy;
		BEntry entry;
		//		dir.GetEntry (&entry);
		//		entry.GetPath (&path);
		//		printf ("Path now: %s\n", path.Path());
		dir.CreateDirectory("Becasso", &dummy); // this won't clobber anyway
												//		dir.GetEntry (&entry);
												//		entry.GetPath (&path);
												//		printf ("Path now: %s\n", path.Path());
		path.Append("Becasso");
		dir.SetTo(path.Path());

		// Do prefs loading here

		//		dir.CreateDirectory ("Recent", &dummy);
		//		dir.GetEntry (&entry);
		//		entry.GetPath (&path);
		//		printf ("Path now: %s\n", path.Path());
	}

	app_info info;
	GetAppInfo(&info);
	BEntry appEntry = BEntry(&info.ref);
	BEntry appDir;
	appEntry.GetParent(&appDir);
	BPath appDirPath(&appDir);
	char dataFile[B_FILE_NAME_LENGTH];
	strcpy(dataFile, appDirPath.Path());
	strcat(dataFile, "/data/strings/");
	extern BLocker g_settings_lock;
	extern becasso_settings g_settings;
	g_settings_lock.Lock();
	strcat(dataFile, g_settings.language);
	g_settings_lock.Unlock();
	if (init_strings(dataFile) != B_OK)
		fprintf(stderr, "Error initializing strings from %s\n", dataFile);

#if defined(DATATYPES)
	if (DATAVersion)
		DataTypes = true;
	else
		DataTypes = false;
	if (DataTypes)
		if (DATAInit("application/x-sum-becasso"))
			DataTypes = false;
#else
	DataTypes = true; // Called the Translation Kit here, but it's basically the same thing.
#endif
	def_out_type = 0;
	def_out_translator = 0;

	size_t bsize;
	size_t fxsize;
	void* colordata = NULL;
	void* teardata = NULL;
	NumColors = 0;
	GetAppInfo(&info);
	BFile file(&info.ref, O_RDONLY);
	if (file.InitCheck()) {
		fprintf(stderr, "InitCheck() failed\n");
	}
	else {
		BResources res(&file);
		colordata = res.FindResource('rgbx', 131, &bsize);
		teardata = res.FindResource('sfx1', 130, &fxsize);
		// printf ("bsize = %li\n", bsize);
	}
	if (colordata) {
		// RGB Color data resource is in the format:
		// offset 	data
		//	0		Max length of color name
		//	1,2		Number of entries (Big Endian)
		//	3..		[R][G][B]Name\0
		unsigned char* cdata = (unsigned char*)colordata;
		extern bool ShowColors;
		int clen = cdata[0];
		NumColors = cdata[1] + 256 * cdata[2];
		if (ShowColors && SHOW_EXTRA_COLOR_INFO) {
			printf("Number of Color Names: %ld\n", NumColors);
			printf("Max length of color name: %d\n", clen);
		}
		cdata += 3;
		clen += 4;
		RGBColors = new unsigned char*[NumColors];
		for (int i = 0; i < NumColors; i++) {
			RGBColors[i] = new unsigned char[clen];
			RGBColors[i][0] = *cdata++;
			RGBColors[i][1] = *cdata++;
			RGBColors[i][2] = *cdata++;
			unsigned char* ti = &(RGBColors[i][3]);
			while (*cdata)
				*ti++ = *cdata++;
			*ti++ = *cdata++;
			if (ShowColors)
				printf(
					"%3d %3d %3d %s\n", (int)RGBColors[i][0], (int)RGBColors[i][1],
					(int)RGBColors[i][2], &RGBColors[i][3]
				);
		}
	}
	else
		fprintf(stderr, "Warning: Couldn't find RGB color resource\n");

#if defined(EASTER_EGG_SFX)
	if (teardata) {
		extern SoundEffect8_11* fxTear;
		fxTear = new SoundEffect8_11(teardata, fxsize);
	}
	else
		fprintf(stderr, "Warning: Couldn't find easter egg resource\n");
#endif

	newnum = 1;
	refvalid = false;
	openPanel = new ThumbnailFilePanel(
		B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false, new BMessage('opnf'), NULL
	);
	openPanel->GetPanelDirectory(&gSaveRef);
	clip = NULL;
	inDrag = false;
	inPaste = false;
	AddOns = new BList();
	launchMessage = NULL;
	fCurrentScriptee = NULL;
	launching = true;
	position_port = create_port(POSITION_QUEUE, "Position Port");
	extern bool BuiltInTablet;
	if (BuiltInTablet)
		wacom = new Tablet("serial3");
	else
		wacom = new Tablet("");
	wacom->Init();
	clipLock = new BLocker("Clip Locker");

#if defined(EASTER_EGG_SFX)
	extern bool EasterEgg;
	if (EasterEgg && fxTear)
		easterEgg = new EffectsPlayer(fxTear);
	else
		easterEgg = NULL;
#endif
	printSetup = NULL;
	fCurrentProperty = 0;
	canvases.MakeEmpty();
	canvas_index = 0;
	// printf ("Becasso::ctor done\n");
}

Becasso::~Becasso()
{
#if defined(EASTER_EGG_SFX)
	delete easterEgg;
#endif
	delete openPanel;
	if (clip)
		delete clip;
#if defined(DATATYPES)
	if (DataTypes)
		DATAShutdown();
#endif
	AddOn* addon;
	for (long i = 0; addon = (AddOn*)AddOns->ItemAt(i), addon; i++)
		delete addon;
	delete AddOns;
}

void
Becasso::LoadAddOns()
{
	extern bool VerbAddOns, NoAddOns;
	if (NoAddOns) {
		if (VerbAddOns)
			fprintf(stderr, "Add-on loading skipped\n");
		return;
	}
	app_info info;
	GetAppInfo(&info);
	BEntry appEntry = BEntry(&info.ref);
	BEntry appDir;
	BPath appPath;
	BDirectory addonDir;
	appEntry.GetParent(&appDir);
	appDir.GetPath(&appPath);
	char dirname[B_FILE_NAME_LENGTH];
	strcpy(dirname, appPath.Path());
	if (addonDir.SetTo(strcat(dirname, "/add-ons")) == B_OK) {
		BEntry addon;
		while (addonDir.GetNextEntry(&addon) != ENOENT) {
			AddOn* newaddon;
			try {
				newaddon = new AddOn(addon);
			}
			catch (...) {
				BPath path;
				addon.GetPath(&path);
				if (VerbAddOns)
					fprintf(stderr, "Skipped %s\n", path.Path());
				// Don't delete newaddon here!
				// Apparently it still points to the previously loaded addon...
				newaddon = NULL;
			}
			if (newaddon) {
				if (!newaddon->Init(AddOns->CountItems())) {
					BMessage* spl = new BMessage('Iaos');
					spl->AddString("InitString", newaddon->Name());
					// printf ("Splash: %s\n", newaddon->Name());
					extern SplashWindow* splash;
					splash->Lock();
					splash->PostMessage(spl);
					splash->Unlock();
					delete spl;
					AddOns->AddItem(newaddon);
				}
				else
					fprintf(stderr, "Warning: Add-On returned error from init\n");
			}
		}
	}
	else {
		fprintf(stderr, "Hmmm - I couldn't find the add-ons/ directory...\n");
	}
}

bool
Becasso::QuitRequested()
{
	extern bool VerbQuit;
	BWindow** kill_list = new BWindow*[CountWindows()];
	int windex = 0;
	bool canQuit = true;
	for (int i = 0; (i < CountWindows()) && canQuit; i++) {
		if (VerbQuit)
			printf("Checking window %i...\n", i);
		BWindow* win = WindowAt(i);
		if (VerbQuit)
			printf("Class name: %s\n", class_name(win));
		if (is_kind_of(win, AttribWindow)) {
			if (VerbQuit)
				printf("   AttribWindow\n");
			kill_list[windex++] = win;
			continue; // These will have to be killed the hard way or they'll hide.
		}
		if (is_kind_of(win, LayerWindow)) {
			if (VerbQuit)
				printf("   LayerWindow\n");
			continue; // These will go down with their associated CanvasWindow.
		}
		if (is_kind_of(win, AddOnWindow)) {
			if (VerbQuit)
				printf("   AddOnWindow\n");
			// These will be closed by the associated CanvasWindow.
			continue;
		}
		if (is_instance_of(win, MagWindow)) {
			if (VerbQuit)
				printf("   MagWindow\n");
			// These will be closed by the associated CanvasWindow.
			continue;
		}
		if (is_instance_of(win, CanvasWindow)) {
			if (VerbQuit)
				printf("   CanvasWindow - QuitRequested...\n");
			canQuit = win->QuitRequested();
			if (canQuit)
				kill_list[windex++] = win;
			if (VerbQuit) {
				if (canQuit)
					printf("   Acknowledged.\n");
				else
					printf("   Refused!\n");
			}
		}
		if (DragWindow* dw = dynamic_cast<DragWindow*>(win)) {
			// This is a MenuWindow (dragged off PicMenuButton)
			if (VerbQuit)
				printf("   DragWindow\n");
			dw->SavePos();
			kill_list[windex++] = win; // Before the Main Window exits, killing the PicMenuButtons.
		}
		// The rest are probably File Panels and whatnot.
		// Just kill them.
	}
	if (canQuit) {
		//		// Kill the MouseChecking threads of the DragView.  They might be
		//		// lingering around, and access their "parent" window after it's gone.
		//		// No harm done, but it generates an exception on closing.  Better be
		//		// sure and kill them ourselves.
		//
		//		NOTE:  This is no longer needed because R3 offers floating windows by itself.
		//
		//		thread_id mousethread;
		//		status_t exitvalue;
		//		while ((mousethread = find_thread ("BecassoDVMC")) != B_NAME_NOT_FOUND)
		//		{
		//			kill_thread (mousethread);
		//			wait_for_thread (mousethread, &exitvalue);
		//		}
		for (int i = 0; i < windex; i++) {
			if (VerbQuit)
				printf("Closing window %i: %s\n", i, class_name(kill_list[i]));
			kill_list[i]->Lock();
			kill_list[i]->Quit();
			// snooze (20000);
		}
		if (VerbQuit)
			printf("Closing Main Window\n");
		mainWindow->Lock();
		mainWindow->Quit(); // It'll take the openPanel with it.
	}
	delete[] kill_list;
	return (canQuit);
}

void
Becasso::AboutRequested()
{
	//	char line[81];
	//	sprintf (line, "Becasso v%s\n© 1997 ∑ Sum Software\nDemo version, expires 1 dec
	//1997\nhttp://www.sumware.demon.nl", Version); 	BAlert *about = new BAlert ("", line, "OK");
	//	about->Go();
	if (about && about->Lock()) {
		about->Activate();
		about->Unlock();
		return;
	}
	size_t bsize, ssize;
	void *becassodata, *sumdata;
	app_info info;
	GetAppInfo(&info);
	BFile file(&info.ref, O_RDONLY);
	if (file.InitCheck()) {
		fprintf(stderr, "InitCheck() failed\n");
		inherited::AboutRequested();
		return;
	}
	BResources res(&file);
	becassodata = res.FindResource('blog', 128, &bsize);
	sumdata = res.FindResource('slog', 129, &ssize);
	if (!becassodata || !sumdata) {
		fprintf(stderr, "Bitmap data not found in resource\n");
		inherited::AboutRequested();
		return;
	}
	about = new AboutWindow(BRect(200, 200, 480, 410));
	color_space cs = B_RGB32;
	{
		BScreen screen;
		if (screen.ColorSpace() == B_COLOR_8_BIT)
			cs = B_COLOR_8_BIT;
	}
	BRect becassorect = BRect(0, 0, 231, 93);
	BRect sumrect = BRect(0, 0, 63, 83);
	BBitmap* becasso = new BBitmap(becassorect, cs);
	BBitmap* sum = new BBitmap(sumrect, B_RGB32);
	becasso->SetBits(becassodata, bsize, 0, B_RGB32);
	sum->SetBits(sumdata, ssize, 0, B_RGB32);
	SAboutView* aboutView = new SAboutView(about->Bounds(), becasso, sum);
	about->AddChild(aboutView);
	about->Show();
}

void
Becasso::ReadyToRun()
{
	// printf ("ReadyToRun()...\n");
	BRect mainWindowFrame;
	mainWindowFrame.Set(0, 0, 160, 50);
	BPoint origin = get_window_origin(0);
	mainWindow = new MainWindow(mainWindowFrame, "Becasso");
	mainWindow->MoveTo(origin);

	extern int SilentOperation;
	mainWindow->Minimize(SilentOperation >= 2);
	mainWindow->Show();
	extern SplashWindow* splash;
	splash->Lock();
	splash->Quit();
	launching = false;
	if (launchMessage) {
		PostMessage(launchMessage);
		delete launchMessage;
		launchMessage = NULL; // Just to make sure.  It won't be used anymore, though.
	}
	// printf ("ReadyToRun() Done\n");

	extern becasso_settings g_settings;
	extern BLocker g_settings_lock;
	g_settings_lock.Lock();
	int32 totd = g_settings.totd;
	g_settings_lock.Unlock();

	if (totd) {
		BRect totdRect(100, 200, 420, 320);
		BPoint totdPoint = get_window_origin(numTOTDWindow);
		if (totdPoint != InvalidPoint)
			totdRect.OffsetTo(totdPoint);
		TOTDWindow* totdwin = new TOTDWindow(totdRect, totd);
		totdwin->Show();
	}
}

void
Becasso::RefsReceived(BMessage* message)
{
	// printf ("Becasso::RefsReceived\n");
	uint32 type;
	int32 count;
	message->GetInfo("refs", &type, &count);
	for (long i = --count; i >= 0; i--) {
		if (message->FindRef("refs", i, &fRef) == B_OK) {
			refvalid = true;
			BMessage* refmsg = new BMessage('opnf');
			refmsg->AddRef("refs", &fRef);
			bool AskForAlpha = true;
			const char* AskForAlphaString;
			// message->PrintToStream();
			if (message->FindBool("AskForAlpha", &AskForAlpha) == B_OK)
				refmsg->AddBool("AskForAlpha", AskForAlpha);
			else if ((message->FindString("AskForAlpha", &AskForAlphaString) == B_OK) && (!strcasecmp(AskForAlphaString, "false")))
				refmsg->AddBool("AskForAlpha", false);
			// refmsg->PrintToStream();
			PostMessage(refmsg);
			delete refmsg;
			snooze(20000);
		}
	}
}

void
Becasso::ArgvReceived(int32 argc, char** argv)
{
	// printf ("Becasso::ArgvReceived (argc = %ld)\n", argc);
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			// Command line option - these are handled in main().
		}
		else {
			// printf ("%s\n", argv[i]);
			entry_ref ref;
			get_ref_for_path(argv[i], &ref);
			refvalid = true;
			BMessage* refmsg = new BMessage('opnf');
			refmsg->AddRef("refs", &ref);
			PostMessage(refmsg);
			delete refmsg;
			snooze(20000);
		}
	}
}

status_t
Becasso::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso");
	BPropertyInfo info(prop_list, value_list);
	message->AddFlat("messages", &info);
	message->AddString("suites", "suite/x-sum-BBP");
	BPropertyInfo infoBBP(prop_list_BBP, value_list_BBP);
	message->AddFlat("messages", &infoBBP);
	return BApplication::GetSupportedSuites(message);
}

BHandler*
Becasso::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
)
{
	//	printf ("message:\n");
	//	message->PrintToStream();
	//	printf ("\nspecifier:\n");
	//	specifier->PrintToStream();
	//	printf ("command: %ld, property: ""%s""\n", command, property);

	if (!strcasecmp(property, "Tool")) {
		if (specifier->HasString("name") ||
			specifier->HasInt32("index")) // Part of a specifier for Tool settings.
		{
			// Note: Don't PopSpecifier() !!
			// We'll let the AttribWindow resolve the exact tool specifier first...
			extern AttribWindow* tAttribWindow;
			tAttribWindow->PostMessage(message);
			return NULL;
		}
		else {
			fCurrentProperty = PROP_TOOL;
			return this;
		}
	}
	if (!strcasecmp(property, "Mode")) {
		if (specifier->HasString("name") ||
			specifier->HasInt32("index")) // Part of a specifier for Mode settings.
		{
			// Note: Don't PopSpecifier() !!
			// We'll let the AttribWindow resolve the exact mode specifier first...
			extern AttribWindow* mAttribWindow;
			mAttribWindow->PostMessage(message);
			return NULL;
		}
		else {
			fCurrentProperty = PROP_MODE;
			return this;
		}
	}
	if (!strcasecmp(property, "Foreground")) {
		fCurrentProperty = PROP_FGCOLOR;
		return this;
	}
	if (!strcasecmp(property, "Background")) {
		fCurrentProperty = PROP_BGCOLOR;
		return this;
	}
	if (!strcasecmp(property, "TabletArea")) {
		fCurrentProperty = PROP_TABLET;
		return this;
	}
	if (!strcasecmp(property, "Size")) {
		fCurrentProperty = PROP_SIZE;
		return this;
	}
	if (!strcasecmp(property, "Name")) {
		fCurrentProperty = PROP_NAME;
		return this;
	}
	if (!strcasecmp(property, "ExportFormat")) {
		fCurrentProperty = PROP_EXPFMT;
		return this;
	}
	if (!strcasecmp(property, "Scriptee")) {
		fCurrentProperty = PROP_SCRIPTEE;
		return this;
	}
	if (!strcasecmp(property, "Canvas")) {
		fCurrentProperty = PROP_CANVAS;
		switch (specifier->what) {
		case B_DIRECT_SPECIFIER:
			return this;
			break;
		case B_INDEX_SPECIFIER: {
			int32 index;
			if (specifier->FindInt32("index", &index) == B_OK) {
				canvas* current = (canvas*)canvases.ItemAt(index);
				if (current) {
					message->PopSpecifier();
					if (current->its_looper != Looper()) {
						current->its_looper->PostMessage(message);
						return NULL;
					}
				}
				else {
					BString indexData, errorString;
					fNumberFormat.Format(indexData, index);
					errorString.SetToFormat("Index out of range: %s", indexData.String());
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errorString);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errorString);
				}
			}
			break;
		}
		case B_NAME_SPECIFIER: {
			const char* name;
			if (specifier->FindString("name", &name) == B_OK) {
				int32 m = canvases.CountItems();
				int32 i;
				for (i = 0; i < m; i++) {
					canvas* current = (canvas*)canvases.ItemAt(i);
					if (!strcmp(name, current->name)) {
						message->PopSpecifier();
						current->its_looper->PostMessage(message);
						return NULL;
					}
				}
				if (i == m) {
					char errstring[256];
					sprintf(errstring, "Unknown Canvas: '%s'", name);
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
				}
			}
			break;
		}
		default:
			return inherited::ResolveSpecifier(message, index, specifier, command, property);
		}
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
Becasso::PrintSetup()
{
	extern BMessage* printSetup;
	BPrintJob job("Becasso");

	if (printSetup)
		job.SetSettings(new BMessage(*printSetup));
	if (job.ConfigPage() == B_OK) {
		delete printSetup;
		printSetup = job.Settings();
	}
}

#if defined(__MWCC__)
#pragma optimization_level 1
#endif

void
Becasso::MessageReceived(BMessage* message)
{
	//	printf ("Becasso::MessageReceived\n");
	//	message->PrintToStream();
	BString title, windowNumber;

	switch (message->what) {
	case 'creg': {
		// printf ("Registering canvas...\n");
		canvas* newcanvas = new canvas();
		const char* name;
		if (message->FindString("name", &name) == B_OK)
			strcpy(newcanvas->name, name);
		else
			strcpy(newcanvas->name, "<untitled>");
		newcanvas->index = canvas_index++;
		BLooper* its_looper;
		if (message->FindPointer("looper", (void**)&its_looper) == B_OK)
			newcanvas->its_looper = its_looper;
		else
			fprintf(stderr, "BIG error!!\n");

		canvases.AddItem(newcanvas);
		verbose(1, "Canvas '%s' registered...\n", newcanvas->name);
		break;
	}
	case 'cdel': {
		BLooper* its_looper;
		if (message->FindPointer("looper", (void**)&its_looper) == B_OK) {
			int32 m = canvases.CountItems();
			int32 i;
			for (i = 0; i < m; i++) {
				canvas* current = (canvas*)canvases.ItemAt(i);
				if (its_looper == current->its_looper) {
					delete ((canvas*)canvases.RemoveItem(i));
					break;
				}
			}
			if (i == m)
				fprintf(stderr, "Unknown Canvas %p (?!)\n", its_looper);
		}
		else
			fprintf(stderr, "BIG error!!\n");
		break;
	}
	case 'aURL': {
		char* argv[1] = {"https://github.com/HaikuArchives/Becasso"};
		be_roster->Launch("text/html", 1, argv);
		break;
	}
	case 'aDOC': {
		app_info info;
		GetAppInfo(&info);
		BEntry appEntry = BEntry(&info.ref);
		BEntry appDir;
		BPath appPath;
		BDirectory addonDir;
		appEntry.GetParent(&appDir);
		appDir.GetPath(&appPath);
		char dirname[B_FILE_NAME_LENGTH];
		strcpy(dirname, appPath.Path());
		char* argv[1];
		argv[0] = new char[B_FILE_NAME_LENGTH];
		sprintf(argv[0], "file://%s/Documentation/Becasso.html", dirname);
		be_roster->Launch("text/html", 1, argv);
		break;
	}
	case B_SIMPLE_DATA: {
		uint32 type;
		int32 count;
		message->GetInfo("refs", &type, &count);
		for (long i = --count; i >= 0; i--) {
			if (message->FindRef("refs", i, &fRef) == B_OK) {
				refvalid = true;
				BMessage* refmsg = new BMessage('opnf');
				refmsg->AddRef("refs", &fRef);
				PostMessage(refmsg);
				delete refmsg;
				snooze(20000);
			}
		}
		break;
	}
	case 'expt': {
		if (!fCurrentScriptee)
		// There's no current "scriptee" window yet!
		{
			char errstring[256];
			sprintf(
				errstring, "There is no current "
						   "scriptee"
						   " window yet"
			);
			if (message->IsSourceWaiting()) {
				BMessage error(B_ERROR);
				error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
				error.AddString("message", errstring);
				message->SendReply(&error);
			}
			else
				fprintf(stderr, "%s\n", errstring);
			break;
		}

		BMessage exportMessage('expt');
		const char* namestring;
		if (message->FindString("name", &namestring) == B_OK ||
			message->FindString("Name", &namestring) == B_OK ||
			message->FindString("filename", &namestring) == B_OK ||
			message->FindString("Filename", &namestring) == B_OK) {
			exportMessage.AddString("filename", namestring);
			// printf ("File name: %s\n", namestring);
		}

		BMessage reply;
		fCurrentScriptee->SendMessage(&exportMessage, &reply);
		if (message->IsSourceWaiting()) {
			message->SendReply(&reply);
		}
		break;
	}
	case 'Crop': {
		if (!fCurrentScriptee)
		// There's no current "scriptee" window yet!
		{
			char errstring[256];
			sprintf(
				errstring, "There is no current "
						   "scriptee"
						   " window yet"
			);
			if (message->IsSourceWaiting()) {
				BMessage error(B_ERROR);
				error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
				error.AddString("message", errstring);
				message->SendReply(&error);
			}
			else
				fprintf(stderr, "%s\n", errstring);
			break;
		}

		BMessage cropMessage('Crop');
		BRect rect;
		if (message->FindRect("data", &rect) == B_OK || message->FindRect("rect", &rect) == B_OK ||
			message->FindRect("croprect", &rect) == B_OK ||
			message->FindRect("CropRect", &rect) == B_OK) {
			cropMessage.AddRect("data", rect);
		}

		BMessage reply;
		fCurrentScriptee->SendMessage(&cropMessage, &reply);
		if (message->IsSourceWaiting()) {
			message->SendReply(&reply);
		}
		break;
	}
	case 'newc': {
		Unlock();
		static int32 newWidth = 256, newHeight = 256;
		static int32 newColor = COLOR_BG;
		SizeWindow* sizeWindow = new SizeWindow(newHeight, newWidth, newColor);
		if (sizeWindow->Go()) {
			fNumberFormat.Format(windowNumber, newnum++);
			title.SetToFormat("%s %s", "Untitled", windowNumber.String());
			newWidth = sizeWindow->w();
			newHeight = sizeWindow->h();
			newColor = sizeWindow->color();
			BRect canvasWindowFrame;
			canvasWindowFrame.Set(
				128 + newnum * 16, 128 + newnum * 16, 128 + newnum * 16 + newWidth - 1,
				128 + newnum * 16 + newHeight - 1
			);
			rgb_color color = White;
			extern ColorMenuButton *hicolor, *locolor;
			switch (newColor) {
			case COLOR_FG:
				color = hicolor->color();
				break;
			case COLOR_BG:
				color = locolor->color();
				break;
			case COLOR_TR:
				color.red = 255;
				color.green = 255;
				color.blue = 255;
				color.alpha = 0;
				break;
			}
			CanvasWindow* canvasWindow =
				new CanvasWindow(canvasWindowFrame, title, NULL, NULL, false, color);
			delete fCurrentScriptee;
			fCurrentScriptee = new BMessenger(canvasWindow);
			canvasWindow->Show();
		}
		Lock();
		sizeWindow->Lock();
		sizeWindow->Quit();
		break;
	}
	case 'cnew': // Deprecated!!!
	{
		// message->PrintToStream();
		fNumberFormat.Format(windowNumber, newnum++);
		title.SetToFormat("Untitled %s", windowNumber.String());
		BRect canvasWindowFrame;
		canvasWindowFrame.Set(
			128 + newnum * 16, 128 + newnum * 16, 128 + newnum * 16 + 255, 128 + newnum * 16 + 255
		);

		BMessage specifier;
		int32 index;
		while (message->GetCurrentSpecifier(&index, &specifier) == B_OK) {
			// printf ("Found a specifier:\n");
			// specifier.PrintToStream();
			const char* property;
			if (specifier.FindString("property", &property) == B_OK) {
				// printf ("Found property: %s\n", property);
				if (!strcasecmp(property, "Size")) {
					char* sizestring;
					if (specifier.FindString("name", (const char**)&sizestring) == B_OK) {
						if (!strncmp(sizestring, "BRect", 5))
							sizestring += 6;

						canvasWindowFrame.left = atof(strtok(sizestring, " ,"));
						canvasWindowFrame.top = atof(strtok(NULL, " ,"));
						canvasWindowFrame.right = atof(strtok(NULL, " ,"));
						canvasWindowFrame.bottom = atof(strtok(NULL, " ,"));
					}
				}
				else if (!strcasecmp(property, "Name")) {
					const char* namestring;
					if (specifier.FindString("name", &namestring) == B_OK) {
						title.SetTo(namestring);
					}
				}
			}
			message->PopSpecifier();
		}
		// canvasWindowFrame.PrintToStream();
		CanvasWindow* canvasWindow = new CanvasWindow(canvasWindowFrame, title);
		delete fCurrentScriptee;
		fCurrentScriptee = new BMessenger(canvasWindow);
		canvasWindow->Show();
		if (message->IsSourceWaiting()) {
			BMessage error(B_REPLY);
			error.AddInt32("error", B_NO_ERROR);
			message->SendReply(&error);
		}
		break;
	}
	case 'open':
		openPanel->Show(); // Don't ask.
		openPanel->Show();
		break;
	case 'opnR': // open Recent
	{
		entry_ref ref;
		if (message->FindRef("ref", &ref) == B_NO_ERROR) {
			BEntry entry;
			if ((entry.SetTo(&ref, true) == B_NO_ERROR) && entry.IsFile()) {
				bool AskForAlpha = true;
				char buffer[1024];
				BNode node(&ref);
				ssize_t s;
				if ((s = node.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, buffer, 1022)) > 0) {
					buffer[s] = 0;
					if (!strcmp(buffer, "application/x-sum-becasso.palette")) {
						BAlert* alert = new BAlert(
							"",
							lstring(
								6, "Please drop Becasso Palette files on the appropriate Color "
								   "button or on the associated Color Editor window."
							),
							lstring(136, "OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT
						);
						alert->Go();
						break;
					}
				}
				try {
					// message->PrintToStream();
					if (message->HasBool("AskForAlpha"))
						message->FindBool("AskForAlpha", &AskForAlpha);
					CanvasWindow* canvasWindow;
					BRect canvasWindowFrame;
					canvasWindowFrame.Set(100, 180, 500, 500);
					canvasWindow = new CanvasWindow(canvasWindowFrame, ref, AskForAlpha);
					delete fCurrentScriptee;
					fCurrentScriptee = new BMessenger(canvasWindow);
					BScreen screen;
					canvasWindowFrame = canvasWindow->Frame() & screen.Frame();
					canvasWindow->ResizeTo(canvasWindowFrame.Width(), canvasWindowFrame.Height());
					canvasWindow->Show();
				}
				catch (...) {
					printf("Caught in the act!\n");
				}
			}
			else {
				char name[B_FILE_NAME_LENGTH];
				if (entry.GetName(name) == B_OK) {
					printf("Couldn't find %s\n", name);
				}
				else {
					printf("Not a file?\n");
				}
				// Not a file?
			}
		}
		else {
			printf("No ref?\n");
			// No refs?
		}
		break;
	}
	case 'opnf': {
		entry_ref ref;
		openPanel->Hide();
		if (message->FindRef("refs", &ref) == B_NO_ERROR) {
			BEntry entry;
			if ((entry.SetTo(&ref, true) == B_NO_ERROR) && entry.IsFile()) {
				bool AskForAlpha = true;
				char buffer[1024];
				BNode node = BNode(&ref);
				ssize_t s;
				if ((s = node.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, buffer, 1022)) > 0) {
					buffer[s] = 0;
					if (!strcmp(buffer, "application/x-sum-becasso.palette")) {
						BAlert* alert = new BAlert(
							"",
							lstring(
								6, "Please drop Becasso Palette files on the appropriate Color "
								   "button or on the associated Color Editor window."
							),
							lstring(136, "OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT
						);
						alert->Go();
						break;
					}
				}
				try {
					// message->PrintToStream();
					if (message->HasBool("AskForAlpha"))
						message->FindBool("AskForAlpha", &AskForAlpha);
					CanvasWindow* canvasWindow;
					BRect canvasWindowFrame;
					canvasWindowFrame.Set(100, 180, 500, 500);
					canvasWindow = new CanvasWindow(canvasWindowFrame, ref, AskForAlpha);
					delete fCurrentScriptee;
					fCurrentScriptee = new BMessenger(canvasWindow);
					BScreen screen;
					canvasWindowFrame = canvasWindow->Frame() & screen.Frame();
					canvasWindow->ResizeTo(canvasWindowFrame.Width(), canvasWindowFrame.Height());
					extern int SilentOperation;
					canvasWindow->Minimize(SilentOperation >= 1);
					canvasWindow->Show();
				}
				catch (...) {
					printf("Caught in the act!\n");
					break;
				}
			}
			else {
				char name[B_FILE_NAME_LENGTH];
				if (entry.GetName(name) == B_OK) {
					printf("Couldn't find %s\n", name);
				}
				else {
					printf("Not a file?\n");
				}
				// Not a file?
			}
		}
		else {
			printf("No refs?\n");
			// No refs?
		}
		break;
	}
	case 'aliv': // Sent by CanvasWindow itself, due to scripting AskForAlpha switch
	{
		BMessenger msgr;
		message->FindMessenger("canvas", &msgr);
		message->AddInt32("which", 1);
		msgr.SendMessage(message);
		break;
	}
	case 'SPst': {
		BWindow* win;
		for (int i = 1; i < CountWindows(); i++) {
			win = WindowAt(i);
			if (is_instance_of(win, CanvasWindow))
				win->PostMessage(message);
		}
		break;
	}
	case 'clrX': // color changed - warn all the windows
	{
		BWindow* win;
		for (int i = 1; i < CountWindows(); i++) {
			win = WindowAt(i);
			if (is_instance_of(win, CanvasWindow))
				win->PostMessage(message);
		}
		break;
	}
	case 0: // Icon dropped from FileTypes (yes, 0!!)
	{
		BMessage msg;
		if (message->FindMessage("icon/large", &msg) == B_OK) {
			BBitmap* map = new BBitmap(&msg);
			if (map->IsValid()) {
				fNumberFormat.Format(windowNumber, newnum++);
				title.SetToFormat("Untitled (Icon) %s", windowNumber.String());
				float newWidth = map->Bounds().Width();
				float newHeight = map->Bounds().Height();
				BRect canvasWindowFrame;
				canvasWindowFrame.Set(
					128 + newnum * 16, 128 + newnum * 16, 128 + newnum * 16 + newWidth - 1,
					128 + newnum * 16 + newHeight - 1
				);
				CanvasWindow* canvasWindow = new CanvasWindow(canvasWindowFrame, title, map);
				delete fCurrentScriptee;
				fCurrentScriptee = new BMessenger(canvasWindow);
				canvasWindow->Show();
				BMessage* m = new BMessage('M4');
				canvasWindow->PostMessage(m);
				delete m;
				m = new BMessage('rstf');
				canvasWindow->PostMessage(m);
				delete m;
			}
			else {
				fprintf(stderr, "Invalid BBitmap\n");
				// Error
			}
		}
		if (message->FindMessage("icon/mini", &msg) == B_OK) {
			BBitmap* map = new BBitmap(&msg);
			if (map->IsValid()) {
				fNumberFormat.Format(windowNumber, newnum++);
				title.SetToFormat("Untitled (MIcon) %s", windowNumber.String());
				float newWidth = map->Bounds().Width();
				float newHeight = map->Bounds().Height();
				BRect canvasWindowFrame;
				canvasWindowFrame.Set(
					128 + newnum * 16, 128 + newnum * 16, 128 + newnum * 16 + newWidth - 1,
					128 + newnum * 16 + newHeight - 1
				);
				CanvasWindow* canvasWindow = new CanvasWindow(canvasWindowFrame, title, map);
				delete fCurrentScriptee;
				fCurrentScriptee = new BMessenger(canvasWindow);
				canvasWindow->Show();
				BMessage* m = new BMessage('M4');
				canvasWindow->PostMessage(m);
				delete m;
				m = new BMessage('rstf');
				canvasWindow->PostMessage(m);
				delete m;
			}
			else {
				fprintf(stderr, "Invalid BBitmap\n");
				// Error
			}
		}
		break;
	}
	case 'PPNT': // RRaster drop
	{
		if (launching) {
			launchMessage = new BMessage(*message);
		}
		else {
			BRect bounds;
			color_space colorspace;
			ssize_t bits_length;
			message->FindRect("bounds", &bounds);
			message->FindInt32("bits_length", (int32*)&bits_length);
			message->FindInt32("color_space", (int32*)&colorspace);
			fNumberFormat.Format(windowNumber, newnum++);
			title.SetToFormat("Untitled (RRaster) %s", windowNumber.String());
			BBitmap* map = new BBitmap(bounds, colorspace);
			char* bits;
			ssize_t numbytes;
			message->FindData("bits", B_RAW_TYPE, (const void**)&bits, &numbytes);
			if (numbytes != bits_length) {
				fprintf(stderr, "Hey, bits_length != num_bytes!!\n");
			}
			memcpy(map->Bits(), bits, numbytes);
			float newWidth = bounds.Width();
			float newHeight = bounds.Height();
			BRect canvasWindowFrame;
			canvasWindowFrame.Set(
				128 + newnum * 16, 128 + newnum * 16, 128 + newnum * 16 + newWidth - 1,
				128 + newnum * 16 + newHeight - 1
			);
			CanvasWindow* canvasWindow = new CanvasWindow(canvasWindowFrame, title, map);
			delete fCurrentScriptee;
			fCurrentScriptee = new BMessenger(canvasWindow);
			canvasWindow->Show();
		}
		break;
	}
	case BBP_REQUEST_BBITMAP: // BeDC Hack for interaction with ImageElements
	{
		// Find the active Canvas Window
		CanvasWindow* activeWin = NULL;
		CanvasWindow* firstWin = NULL;
		for (int i = 0; i < CountWindows(); i++) {
			BWindow* win = WindowAt(i);
			if (is_instance_of(win, CanvasWindow)) {
				if (!firstWin)
					firstWin = dynamic_cast<CanvasWindow*>(win);
				if (win->IsActive())
					activeWin = dynamic_cast<CanvasWindow*>(win);
			}
		}
		if (!activeWin) // No active window?  Hmmm.
		{
			if (firstWin) {
				activeWin = firstWin; // Just take the first Canvas Window you found.
			}
			else // no canvas windows at all!
			{
				fprintf(stderr, "No canvas windows open...\n");
				if (message->IsSourceWaiting()) // Disappoint our friend.
				{
					BMessage* reply = new BMessage(BBP_NO_WINDOWS);
					message->SendReply(reply);
					delete reply;
				}
				break;
			}
		}
		message->AddString("hack", "");
		activeWin->MessageReceived(message);
		break;
	}
	case BBP_REPLACE_BBITMAP: // This is a _copy_!!!
	{
		// printf ("Got to the main app...\n");
		BMessage realmessage;
		if (message->FindMessage("message", &realmessage) == B_OK) {
			const char* name;
			if (message->FindString("name", &name) == B_OK) {
				title.SetTo(name);
			} else {
				fNumberFormat.Format(windowNumber, newnum++);
				title.SetToFormat("Untitled %s", windowNumber.String());
			}
			BMessage archive;
			if (realmessage.FindMessage("BBitmap", &archive) == B_OK) {
				BBitmap* map = new BBitmap(&archive);
				if (map->IsValid()) {
					BMessenger ie;
					message->FindMessenger("target", &ie);
					float newWidth = map->Bounds().Width();
					float newHeight = map->Bounds().Height();
					BRect canvasWindowFrame;
					canvasWindowFrame.Set(
						128 + newnum * 16, 128 + newnum * 16, 128 + newnum * 16 + newWidth - 1,
						128 + newnum * 16 + newHeight - 1
					);
					CanvasWindow* canvasWindow =
						new CanvasWindow(canvasWindowFrame, title, map, &ie);
					delete fCurrentScriptee;
					fCurrentScriptee = new BMessenger(canvasWindow);
					extern int SilentOperation;
					canvasWindow->Minimize(SilentOperation >= 1);
					canvasWindow->Show();
					BMessage* ok = new BMessage(BBP_BBITMAP_OPENED);
					ok->AddMessenger("target", BMessenger(canvasWindow, NULL));
					if (realmessage.IsSourceWaiting()) {
						realmessage.SendReply(ok);
					}
					else {
						ie.SendMessage(ok);
					}
					delete ok;
					float zoom;
					if (realmessage.FindFloat("zoom", &zoom) == B_OK) {
						BMessage* m = new BMessage('Mnnn');
						m->AddFloat("zoom", zoom);
						canvasWindow->PostMessage(m);
						delete m;
					}
				}
				else {
					fprintf(stderr, "Invalid BBitmap\n");
					// Error
				}
			}
			else {
				fprintf(stderr, "Message didn't contain a valid BBitmap\n");
				// Error
			}
			break;
		}
		else {
			// Fall through to BBP_OPEN_BBITMAP.
			// Explanation:  Becasso received a BBP_REPLACE_BBITMAP message itself,
			// which doesn't make sense (which bitmap should be replaced?).
			// Apart from simply erroring out, the most sensible thing to do would be
			// opening the bitmap in a new window.
		}
	}
	case BBP_OPEN_BBITMAP: {
		// printf ("BBP_OPEN_BBITMAP\n");
		if (launching) {
			launchMessage = new BMessage(*message);
		}
		else {
			const char* name;
			if (message->FindString("name", &name) == B_OK) {
				title.SetTo(name);
			} else {
				fNumberFormat.Format(windowNumber, newnum++);
				title.SetToFormat("Untitled %s", windowNumber.String());
			}
			BMessage archive;
			entry_ref ref;
			if (message->FindMessage("BBitmap", &archive) == B_OK) {
				BBitmap* map = new BBitmap(&archive);
				if (map->IsValid()) {
					BMessenger ie;
					message->FindMessenger("target", &ie);
					float newWidth = map->Bounds().Width();
					float newHeight = map->Bounds().Height();
					BRect canvasWindowFrame;
					canvasWindowFrame.Set(
						128 + newnum * 16, 128 + newnum * 16, 128 + newnum * 16 + newWidth - 1,
						128 + newnum * 16 + newHeight - 1
					);
					CanvasWindow* canvasWindow =
						new CanvasWindow(canvasWindowFrame, title, map, &ie);
					delete fCurrentScriptee;
					fCurrentScriptee = new BMessenger(canvasWindow);
					extern int SilentOperation;
					canvasWindow->Minimize(SilentOperation >= 1);
					canvasWindow->Show();
					BMessage* ok = new BMessage(BBP_BBITMAP_OPENED);
					ok->AddMessenger("target", BMessenger(canvasWindow, NULL));
					if (message->IsSourceWaiting()) {
						message->SendReply(ok);
					}
					else {
						ie.SendMessage(ok);
					}
					delete ok;
					float zoom;
					if (message->FindFloat("zoom", &zoom) == B_OK) {
						BMessage* m = new BMessage('Mnnn');
						m->AddFloat("zoom", zoom);
						canvasWindow->PostMessage(m);
						delete m;
					}
				}
				else {
					fprintf(stderr, "Invalid BBitmap\n");
					// Error
				}
			}
			else if (message->FindRef("ref", &ref) == B_OK) {
				BEntry entry;
				if ((entry.SetTo(&ref, true) == B_NO_ERROR) && entry.IsFile()) {
					bool AskForAlpha = true;
					char buffer[1024];
					BNode node = BNode(&ref);
					ssize_t s;
					if ((s = node.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, buffer, 1022)) > 0) {
						buffer[s] = 0;
						if (!strcmp(buffer, "application/x-sum-becasso.palette")) {
							BAlert* alert = new BAlert(
								"",
								lstring(
									6, "Please drop Becasso Palette files on the appropriate Color "
									   "button or on the associated Color Editor window."
								),
								lstring(136, "OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT
							);
							alert->Go();
							break;
						}
					}
					try {
						BMessenger ie;
						message->FindMessenger("target", &ie);
						// message->PrintToStream();
						if (message->HasBool("AskForAlpha"))
							message->FindBool("AskForAlpha", &AskForAlpha);
						CanvasWindow* canvasWindow;
						BRect canvasWindowFrame;
						canvasWindowFrame.Set(100, 180, 500, 500);
						canvasWindow = new CanvasWindow(canvasWindowFrame, ref, AskForAlpha, &ie);
						delete fCurrentScriptee;
						fCurrentScriptee = new BMessenger(canvasWindow);
						BScreen screen;
						canvasWindowFrame = canvasWindow->Frame() & screen.Frame();
						canvasWindow->ResizeTo(
							canvasWindowFrame.Width(), canvasWindowFrame.Height()
						);
						extern int SilentOperation;
						canvasWindow->Minimize(SilentOperation >= 1);
						canvasWindow->Show();
						BMessage* ok = new BMessage(BBP_BBITMAP_OPENED);
						ok->AddMessenger("target", BMessenger(canvasWindow, NULL));
						if (message->IsSourceWaiting()) {
							message->SendReply(ok);
						}
						else {
							ie.SendMessage(ok);
						}
						delete ok;
						float zoom;
						if (message->FindFloat("zoom", &zoom) == B_OK) {
							BMessage* m = new BMessage('Mnnn');
							m->AddFloat("zoom", zoom);
							canvasWindow->PostMessage(m);
							delete m;
						}
					}
					catch (...) {
						printf("Caught in the act!\n");
						break;
					}
				}
				else {
					char name[B_FILE_NAME_LENGTH];
					if (entry.GetName(name) == B_OK) {
						printf("Couldn't find %s\n", name);
					}
					else {
						printf("Not a file?\n");
					}
					// Not a file?
				}
			}
			else {
				fprintf(stderr, "Message didn't contain a valid BBitmap or ref\n");
				// Error
			}
		}
		break;
	}
	case 'capt': {
		extern BList* AddOns;
		AddOn* addon;
		int32 index;
		message->FindInt32("index", &index);
		addon = (AddOn*)AddOns->ItemAt(index);
		addon->Open(NULL, NULL);
		break;
	}
	case CAPTURE_READY: {
		printf("Capturing...\n");
		extern BList* AddOns;
		AddOn* addon;
		int32 index;
		if (message->FindInt32("index", &index) == B_OK) {
			addon = (AddOn*)AddOns->ItemAt(index);
			title = '0';
			// printf ("Calling Bitmap...\n");
			BString titleCopy = title;
			char* mutableTitle = titleCopy.LockBuffer(titleCopy.Length() + 1);
			strncpy(mutableTitle, titleCopy.String(), titleCopy.Length() + 1);
			titleCopy.UnlockBuffer();
			BBitmap* map = addon->Bitmap(mutableTitle);

			// printf ("Got it!\n");
			BString windowNumber;
			if (map) {
				if (map->IsValid()) {
					if (!title[0]) {
						fNumberFormat.Format(windowNumber, newnum++);
						title.SetToFormat("Untitled %s", windowNumber.String());
					}
					BRect canvasWindowFrame;
					canvasWindowFrame.Set(
						128 + newnum * 16, 128 + newnum * 16,
						128 + newnum * 16 + map->Bounds().Width(),
						128 + newnum * 16 + map->Bounds().Height()
					);
					CanvasWindow* canvasWindow = new CanvasWindow(canvasWindowFrame, title, map);
					delete fCurrentScriptee;
					fCurrentScriptee = new BMessenger(canvasWindow);
					extern int SilentOperation;
					canvasWindow->Minimize(SilentOperation >= 1);
					canvasWindow->Show();
				} else {
					fprintf(stderr, "Capture Error: Invalid BBitmap\n");
					// Error
				}
			}
			else // map == NULL  => Canceled
			{
			}
		}
		else {
			fprintf(
				stderr, "Error: Your capture add-on asked for a grab, but didn't identify itself.\n"
			);
			fprintf(
				stderr,
				"       You should AddInt32(\"index\", &index) to the CAPTURE_READY message.\n"
			);
		}
		break;
	}
	case 'PrSU': {
		PrintSetup();
		break;
	}
	case 'prfs': {
		PrefsWindow* prefsWindow = new PrefsWindow();
		prefsWindow->Show();
		break;
	}
	case B_CREATE_PROPERTY: {
		// message->PrintToStream();
		switch (fCurrentProperty) {
		case PROP_CANVAS: {
			// message->PrintToStream();
			fNumberFormat.Format(windowNumber, newnum++);
			title.SetToFormat("Untitled %s", windowNumber.String());
			BRect canvasWindowFrame;
			canvasWindowFrame.Set(
				128 + newnum * 16, 128 + newnum * 16, 128 + newnum * 16 + 255,
				128 + newnum * 16 + 255
			);

			const char* namestring;
			if (message->FindString("Name", &namestring) == B_OK ||
				message->FindString("name", &namestring) == B_OK) {
				title.SetTo(namestring);
			}

			BRect rect;
			if (message->FindRect("Size", &rect) == B_OK ||
				message->FindRect("size", &rect) == B_OK) {
				canvasWindowFrame = rect;
			}

			// canvasWindowFrame.PrintToStream();
			CanvasWindow* canvasWindow = new CanvasWindow(canvasWindowFrame, title);
			delete fCurrentScriptee;
			fCurrentScriptee = new BMessenger(canvasWindow);
			extern int SilentOperation;
			canvasWindow->Minimize(SilentOperation >= 1);
			canvasWindow->Show();
			if (message->IsSourceWaiting()) {
				BMessage error(B_REPLY);
				error.AddInt32("error", B_NO_ERROR);
				message->SendReply(&error);
			}
			break;
		}
		default:
			inherited::MessageReceived(message);
			break;
		}
		break;
	}
	case B_COUNT_PROPERTIES: {
		switch (fCurrentProperty) {
		case PROP_TOOL: {
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				reply.AddInt32("result", NumTools);
				message->SendReply(&reply);
			}
			else
				fprintf(stderr, "Number of Tools: %ld\n", NumTools);
			break;
		}
		case PROP_MODE: {
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				reply.AddInt32("result", NumModes);
				message->SendReply(&reply);
			}
			else
				fprintf(stderr, "Number of Modes: %ld\n", NumModes);
			break;
		}
		default:
			inherited::MessageReceived(message);
		}
		fCurrentProperty = 0;
		break;
	}
	case B_GET_SUPPORTED_SUITES: {
		switch (fCurrentProperty) {
		case PROP_TOOL: {
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				char str[256];
				for (int i = 0; i < NumTools; i++) {
					sprintf(str, "suite/x-sum-becasso-%s", ToolSpecifiers[i]);
					reply.AddString("suites", str);
				}
				message->SendReply(&reply);
			}
			else {
				fprintf(stderr, "Available Tools:\n");
				for (int i = 0; i < NumTools; i++)
					fprintf(stderr, "%s\n", ToolSpecifiers[i]);
			}
			break;
		}
		case PROP_MODE: {
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				char str[256];
				for (int i = 0; i < NumModes; i++) {
					sprintf(str, "suite/x-sum-becasso-%s", ModeSpecifiers[i]);
					reply.AddString("suites", str);
				}
				message->SendReply(&reply);
			}
			else {
				fprintf(stderr, "Available Modes:\n");
				for (int i = 0; i < NumModes; i++)
					fprintf(stderr, "%s\n", ModeSpecifiers[i]);
			}
			break;
		}
		default:
			inherited::MessageReceived(message);
		}
		fCurrentProperty = 0;
		break;
	}
	case B_SET_PROPERTY: {
		switch (fCurrentProperty) {
		case PROP_TOOL: {
			const char* namespecifier;
			int32 numberspecifier;
			if (message->FindString("data", &namespecifier) == B_OK) {
				int index;
				for (index = 0; index < NumTools; index++) {
					if (!strcasecmp(ToolSpecifiers[index], namespecifier)) {
						extern PicMenuButton* tool;
						tool->set(index);
						if (message->IsSourceWaiting()) {
							BMessage error(B_REPLY);
							error.AddInt32("error", B_NO_ERROR);
							message->SendReply(&error);
						}
						break;
					}
				}
				if (index == NumTools) {
					char errstring[256];
					sprintf(
						errstring,
						"Invalid Tool Name: "
						"%s"
						"",
						namespecifier
					);
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
				}
				break;
			}
			else if (message->FindInt32("data", &numberspecifier) == B_OK) {
				if (0 <= numberspecifier && numberspecifier < NumTools) {
					extern PicMenuButton* tool;
					tool->set(numberspecifier);
					if (message->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						message->SendReply(&error);
					}
					break;
				}
				else {
					BString numberToolsData, numberSpecifierData, errorString;
					fNumberFormat.Format(numberToolsData, NumTools - 1);
					fNumberFormat.Format(numberSpecifierData, numberspecifier);
					errorString.SetToFormat("Tool index out of range [0..%s]: %s", numberToolsData.String(), numberSpecifierData.String());
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errorString.String());
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errorString.String());
				}
			}
			else {
				char errstring[256];
				sprintf(errstring, "Invalid Tool Type (either string or int32 please)");
				if (message->IsSourceWaiting()) {
					BMessage error(B_ERROR);
					error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
					error.AddString("message", errstring);
					message->SendReply(&error);
				}
				else
					fprintf(stderr, "%s\n", errstring);
			}
			break;
		}
		case PROP_MODE: {
			const char* namespecifier;
			int32 numberspecifier;
			if (message->FindString("data", &namespecifier) == B_OK) {
				int index;
				for (index = 0; index < NumModes; index++) {
					if (!strcasecmp(ModeSpecifiers[index], namespecifier)) {
						extern PicMenuButton* mode;
						mode->set(index);
						if (message->IsSourceWaiting()) {
							BMessage error(B_REPLY);
							error.AddInt32("error", B_NO_ERROR);
							message->SendReply(&error);
						}
						break;
					}
				}
				if (index == NumModes) {
					char errstring[256];
					sprintf(
						errstring,
						"Invalid Mode Name: "
						"%s"
						"",
						namespecifier
					);
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
				}
				break;
			}
			else if (message->FindInt32("data", &numberspecifier) == B_OK) {
				if (0 <= numberspecifier && numberspecifier < NumModes) {
					extern PicMenuButton* mode;
					mode->set(numberspecifier);
					if (message->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						message->SendReply(&error);
					}
					break;
				}
				else {
					char errstring[256];
					sprintf(
						errstring, "Mode Index Out of Range [0..%ld]: %ld", NumTools - 1,
						numberspecifier
					);
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
				}
			}
			else {
				char errstring[256];
				sprintf(errstring, "Invalid Mode Type (either string or int32 please)");
				if (message->IsSourceWaiting()) {
					BMessage error(B_ERROR);
					error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
					error.AddString("message", errstring);
					message->SendReply(&error);
				}
				else
					fprintf(stderr, "%s\n", errstring);
			}
			break;
		}
		case PROP_FGCOLOR: {
			// printf ("Setting FG Color\n");
			const char* namespecifier;
			rgb_color* rgbspecifier;
			long dummy;
			if (message->FindString("data", &namespecifier) == B_OK) {
				rgb_color rgb;
				int i;
				for (i = 0; i < NumColors; i++) {
					if (!strcasecmp(namespecifier, (const char*)&(RGBColors[i][3]))) {
						rgb.red = RGBColors[i][0];
						rgb.green = RGBColors[i][1];
						rgb.blue = RGBColors[i][2];
						break;
					}
				}
				if (i == NumColors) {
					char errstring[256];
					sprintf(
						errstring,
						"Unknown Named Color: "
						"%s"
						"",
						namespecifier
					);
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
					break;
				}
				else {
					extern ColorMenuButton* hicolor;
					hicolor->set(rgb);
					if (message->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						message->SendReply(&error);
					}
				}
				break;
			}
			else if (message->FindData("data", B_RGB_COLOR_TYPE, (const void**)&rgbspecifier, &dummy) == B_OK) {
				extern ColorMenuButton* hicolor;
				hicolor->set(*rgbspecifier);
				if (message->IsSourceWaiting()) {
					BMessage error(B_REPLY);
					error.AddInt32("error", B_NO_ERROR);
					message->SendReply(&error);
				}
				break;
			}
			else {
				char errstring[256];
				sprintf(errstring, "Invalid Color Type (either string or rgb_color please)");
				if (message->IsSourceWaiting()) {
					BMessage error(B_ERROR);
					error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
					error.AddString("message", errstring);
					message->SendReply(&error);
				}
				else
					fprintf(stderr, "%s\n", errstring);
			}
			break;
		}
		case PROP_BGCOLOR: {
			// printf ("Setting BG Color\n");
			const char* namespecifier;
			rgb_color* rgbspecifier;
			long dummy;
			if (message->FindString("data", &namespecifier) == B_OK) {
				rgb_color rgb;
				int i;
				for (i = 0; i < NumColors; i++) {
					if (!strcasecmp(namespecifier, (const char*)&(RGBColors[i][3]))) {
						rgb.red = RGBColors[i][0];
						rgb.green = RGBColors[i][1];
						rgb.blue = RGBColors[i][2];
						break;
					}
				}
				if (i == NumColors) {
					char errstring[256];
					sprintf(
						errstring,
						"Unknown Named Color: "
						"%s"
						"",
						namespecifier
					);
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
					break;
				}
				else {
					extern ColorMenuButton* locolor;
					locolor->set(rgb);
					if (message->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						message->SendReply(&error);
					}
				}
				break;
			}
			else if (message->FindData("data", B_RGB_COLOR_TYPE, (const void**)&rgbspecifier, &dummy) == B_OK) {
				extern ColorMenuButton* locolor;
				locolor->set(*rgbspecifier);
				if (message->IsSourceWaiting()) {
					BMessage error(B_REPLY);
					error.AddInt32("error", B_NO_ERROR);
					message->SendReply(&error);
				}
				break;
			}
			else {
				char errstring[256];
				sprintf(errstring, "Invalid Color Type (either string or rgb_color please)");
				if (message->IsSourceWaiting()) {
					BMessage error(B_ERROR);
					error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
					error.AddString("message", errstring);
					message->SendReply(&error);
				}
				else
					fprintf(stderr, "%s\n", errstring);
			}
			break;
		}
		case PROP_EXPFMT: {
			// printf ("Setting Export Format...\n");
			// message->PrintToStream();
			int32 out_type;
			const char* namedspecifier;
			if (message->FindInt32("data", &out_type) == B_OK) {
				// char *s = (char *) &def_out_type;
				// fprintf (stderr, "Current Default Export Type: '%c%c%c%c'\n", s[0], s[1], s[2],
				// s[3]);
			}
			else if (message->FindString("data", &namedspecifier) == B_OK) {
				const char* s = namedspecifier;
				uint32 type;

				// #if defined (__POWERPC__)
				type = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];
				// #else
				//	type = (s[3] << 24) | (s[2] << 16) | (s[1] << 8) | s[0];
				// #endif

				out_type = 0;
				translator_id* translators;
				int32 num_translators;

				BTranslatorRoster::Default()->GetAllTranslators(&translators, &num_translators);

				for (int32 i = 0; i < num_translators; i++) {
					const translation_format* fmts;
					int32 num_fmts;

					BTranslatorRoster::Default()->GetOutputFormats(
						translators[i], &fmts, &num_fmts
					);

					for (int32 j = 0; j < num_fmts; j++) {
						if (!strcasecmp(fmts[j].MIME, s))
						// Type was specified as MIME string
						{
							out_type = fmts[j].type;
							def_out_translator = translators[i];
						}
						else if (fmts[j].type == type)
						// Type was specified as type code
						{
							out_type = type;
							def_out_translator = translators[i];
						}
					}
				}

				if (!out_type) {
					char errstring[256];
					sprintf(errstring, "No Translator available for '%s'", s);
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
					break;
				}
				else {
					def_out_type = out_type;
					if (message->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						message->SendReply(&error);
					}
					break;
				}
			}
			else {
				char errstring[256];
				sprintf(errstring, "Invalid Export Format Type (either string or int32 please)");
				if (message->IsSourceWaiting()) {
					BMessage error(B_ERROR);
					error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
					error.AddString("message", errstring);
					message->SendReply(&error);
				}
				else
					fprintf(stderr, "%s\n", errstring);
				break;
			}
		}
		case PROP_SCRIPTEE: {
			const char* namespecifier;
			int32 indexspecifier;
			if (message->FindString("data", &namespecifier) == B_OK) {
				int32 i;
				for (i = 0; i < CountWindows(); i++) {
					BWindow* win = WindowAt(i);
					if (!strcmp(namespecifier, win->Title())) {
						delete fCurrentScriptee;
						fCurrentScriptee = new BMessenger(win);
						if (message->IsSourceWaiting()) {
							BMessage error(B_REPLY);
							error.AddInt32("error", B_NO_ERROR);
							message->SendReply(&error);
						}
						break;
					}
				}
				if (i == CountWindows()) {
					char errstring[256];
					sprintf(
						errstring, "Invalid Scriptee: Couldn't find window named '%s'",
						namespecifier
					);
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
				}
				break;
			}
			else if (message->FindInt32("data", &indexspecifier) == B_OK) {
				if ((indexspecifier >= 0) && (indexspecifier < CountWindows())) {
					delete fCurrentScriptee;
					fCurrentScriptee = new BMessenger(WindowAt(indexspecifier));

					if (message->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						message->SendReply(&error);
					}
				}
				else {
					char errstring[256];
					sprintf(
						errstring, "Invalid Scriptee: Index %ld out of range [0..%ld]",
						indexspecifier, CountWindows() - 1
					);
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
				}
				break;
			}
			else {
				char errstring[256];
				sprintf(
					errstring,
					"Invalid Scriptee Type (either named (string) or indexed (int32) please)"
				);
				if (message->IsSourceWaiting()) {
					BMessage error(B_ERROR);
					error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
					error.AddString("message", errstring);
					message->SendReply(&error);
				}
				else
					fprintf(stderr, "%s\n", errstring);
			}
			break;
		}
		case PROP_TABLET: {
			BRect rect;
			if (message->FindRect("data", &rect) == B_OK) {
				extern Tablet* wacom;
				if (wacom && wacom->IsValid()) {
					wacom->SetRect(rect);
					if (message->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						message->SendReply(&error);
					}
				}
				else {
					char errstring[256];
					sprintf(errstring, "No tablet initialized (at the moment)");
					if (message->IsSourceWaiting()) {
						BMessage error(B_ERROR);
						error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						error.AddString("message", errstring);
						message->SendReply(&error);
					}
					else
						fprintf(stderr, "%s\n", errstring);
				}
			}
			else {
				char errstring[256];
				sprintf(errstring, "Invalid TabletArea type (BRect please)");
				if (message->IsSourceWaiting()) {
					BMessage error(B_ERROR);
					error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
					error.AddString("message", errstring);
					message->SendReply(&error);
				}
				else
					fprintf(stderr, "%s\n", errstring);
			}
			break;
		}
		default:
			inherited::MessageReceived(message);
		}
		fCurrentProperty = 0;
		break;
	}
	case B_GET_PROPERTY: {
		switch (fCurrentProperty) {
		case PROP_TOOL: {
			extern PicMenuButton* tool;
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				reply.AddSpecifier("result", ToolSpecifiers[tool->selected()]);
				message->SendReply(&reply);
			}
			else
				fprintf(
					stderr,
					"Current Tool: "
					"%s"
					"\n",
					ToolSpecifiers[tool->selected()]
				);
			break;
		}
		case PROP_MODE: {
			extern PicMenuButton* mode;
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				reply.AddSpecifier("result", ModeSpecifiers[mode->selected()]);
				message->SendReply(&reply);
			}
			else
				fprintf(
					stderr,
					"Current Mode: "
					"%s"
					"\n",
					ModeSpecifiers[mode->selected()]
				);
			break;
		}
		case PROP_FGCOLOR: {
			extern ColorMenuButton* hicolor;
			rgb_color rgb = hicolor->color();
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				reply.AddData("result", B_RGB_COLOR_TYPE, &rgb, sizeof(rgb_color));
				message->SendReply(&reply);
			}
			else
				fprintf(
					stderr,
					"Current Foreground Color: [Red: %3d Green: %3d Blue: %3d Alpha: %3d]\n",
					rgb.red, rgb.green, rgb.blue, rgb.alpha
				);
			break;
		}
		case PROP_BGCOLOR: {
			extern ColorMenuButton* locolor;
			rgb_color rgb = locolor->color();
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				reply.AddData("result", B_RGB_COLOR_TYPE, &rgb, sizeof(rgb_color));
				message->SendReply(&reply);
			}
			else
				fprintf(
					stderr,
					"Current Background Color: [Red: %3d Green: %3d Blue: %3d Alpha: %3d]\n",
					rgb.red, rgb.green, rgb.blue, rgb.alpha
				);
			break;
		}
		case PROP_TABLET: {
			extern Tablet* wacom;
			if (wacom && wacom->IsValid()) {
				BRect rect = wacom->GetRect();
				if (message->IsSourceWaiting()) {
					BMessage reply(B_REPLY);
					reply.AddInt32("error", B_NO_ERROR);
					reply.AddRect("result", rect);
					message->SendReply(&reply);
				}
				else
					fprintf(
						stderr,
						"Current Tablet Area: [left: %5.0f top: %5.0f right: %5.0f bottom: "
						"%5.0f]\n",
						rect.left, rect.top, rect.right, rect.bottom
					);
			}
			else {
				char errstring[256];
				sprintf(errstring, "No tablet initialized (at the moment)");
				if (message->IsSourceWaiting()) {
					BMessage error(B_ERROR);
					error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
					error.AddString("message", errstring);
					message->SendReply(&error);
				}
				else
					fprintf(stderr, "%s\n", errstring);
			}
		}
		case PROP_EXPFMT: {
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				reply.AddData("result", B_INT32_TYPE, &def_out_type, sizeof(int32));
				message->SendReply(&reply);
			}
			else {
				char* s = (char*)&def_out_type;
#if defined(__POWERPC__)
				fprintf(
					stderr, "Current Default Export Type: '%c%c%c%c'\n", s[0], s[1], s[2], s[3]
				);
#else
				fprintf(
					stderr, "Current Default Export Type: '%c%c%c%c'\n", s[3], s[2], s[1], s[0]
				);
#endif
			}
			break;
		}
		case PROP_SCRIPTEE: {
			if (message->IsSourceWaiting()) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_NO_ERROR);
				reply.AddData("result", B_MESSENGER_TYPE, &fCurrentScriptee, sizeof(BMessenger));
				message->SendReply(&reply);
			}
			else {
				fprintf(
					stderr, "Scriptee %s.  I'd return a BMessenger * as reply.\n",
					fCurrentScriptee ? "set" : "not set"
				);
			}
			break;
		}
		default:
			// message->PrintToStream();
			inherited::MessageReceived(message);
		}
		fCurrentProperty = 0;
		break;
	}
	default: {
		// message->PrintToStream();
		inherited::MessageReceived(message);
		break;
	}
	}
}

#if defined(__MWCC__)
#pragma optimization_level reset
#endif

void
Becasso::Pulse()
{
	if (fBusy) {
		SetCursor(cross[fBusy - 1]);
		fBusy = fBusy % 3 + 1;
	}
	BApplication::Pulse();
	if (!CountWindows())
		PostMessage(B_QUIT_REQUESTED);
}

void
Becasso::setHand()
{
	fBusy = 0;
	SetCursor(B_HAND_CURSOR);
	fCurrentCursor = CURSOR_HAND;
}

void
Becasso::setGrab(bool b, bool down)
{
	if (b && fCurrentCursor != CURSOR_OPEN_HAND) {
		fBusySave = fBusy;
		fBusy = 0;
		SetCursor(down ? grab : hand);
		fCurrentCursorSave = fCurrentCursor;
		fCurrentCursor = CURSOR_OPEN_HAND;
	}
	else if (b && fCurrentCursor == CURSOR_OPEN_HAND) {
		SetCursor(down ? grab : hand);
	}
	else if (!b && fCurrentCursor == CURSOR_OPEN_HAND) {
		switch (fCurrentCursorSave) {
		case CURSOR_HAND:
			setHand();
			break;
		case CURSOR_CROSS:
			setCross();
			break;
		case CURSOR_SELECT:
			setSelect();
			break;
		case CURSOR_CCROSS:
			setCCross();
			break;
		case CURSOR_MOVER:
			setMover();
			break;
		case CURSOR_ROTATOR:
			setRotator();
			break;
		case CURSOR_PICKER:
			setCross(); //!
			break;
		default:
			break;
		}
		fBusy = fBusySave;
	}
}

void
Becasso::setCross()
{
	extern PicMenuButton* mode;
	if (mode->selected() == M_SELECT) {
		SetCursor(scross);
		fCurrentCursor = CURSOR_SELECT;
	}
	else {
		SetCursor(cross[0]);
		fCurrentCursor = CURSOR_CROSS;
	}
}

void
Becasso::setSelect()
{
	SetCursor(scross);
	fCurrentCursor = CURSOR_SELECT;
}

void
Becasso::setBusy()
{
	fBusy = 1;
	fCurrentCursor = CURSOR_CROSS;
}

void
Becasso::setReady()
{
	fBusy = 0;
	extern PicMenuButton* mode;
	if (mode->selected() == M_SELECT) {
		SetCursor(scross);
		fCurrentCursor = CURSOR_SELECT;
	}
	else {
		SetCursor(cross[0]);
		fCurrentCursor = CURSOR_CROSS;
	}
}

void
Becasso::setCCross()
{
	fBusy = 0;
	SetCursor(ccross);
	fCurrentCursor = CURSOR_CCROSS;
}

void
Becasso::setPicker(bool b)
{
	if (b && fCurrentCursor != CURSOR_PICKER) {
		fBusySave = fBusy;
		fBusy = 0;
		SetCursor(picker);
		fCurrentCursorSave = fCurrentCursor;
		fCurrentCursor = CURSOR_PICKER;
	}
	else if (!b && fCurrentCursor == CURSOR_PICKER) {
		switch (fCurrentCursorSave) {
		case CURSOR_HAND:
			setHand();
			break;
		case CURSOR_CROSS:
			setCross();
			break;
		case CURSOR_SELECT:
			setSelect();
			break;
		case CURSOR_CCROSS:
			setCCross();
			break;
		case CURSOR_MOVER:
			setMover();
			break;
		case CURSOR_ROTATOR:
			setRotator();
			break;
		case CURSOR_OPEN_HAND:
			setGrab(false);
			break;
		default:
			break;
		}
		fBusy = fBusySave;
	}
}

void
Becasso::setMover()
{
	fBusy = 0;
	SetCursor(mover);
	fCurrentCursor = CURSOR_MOVER;
}

void
Becasso::setRotator()
{
	fBusy = 0;
	SetCursor(rotator);
	fCurrentCursor = CURSOR_ROTATOR;
}

void
Becasso::FixCursor() // BeOS bug w.r.t. hot spot of cursors
{
	BMessage fix('fixC');
	mainWindow->PostMessage(&fix);
}
