#include "CanvasWindow.h"
#include <Application.h>
#include <Beep.h>
#include <FindDirectory.h>
#include <List.h>
#include <Mime.h>
#include <PropertyInfo.h>
#include <Screen.h>
#include <SupportDefs.h>
#include <TranslatorRoster.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "AddOn.h"
#include "BBP.h"
#include "BGView.h"
#include "BitmapStuff.h"
#include "ColorMenuButton.h"
#include "LayerWindow.h"
#include "MagWindow.h"
#include "OutputFormatWindow.h"
#include "PosView.h"
#include "ProgressiveBitmapStream.h"
#include "Properties.h"
#include "ResizeWindow.h"
#include "Settings.h"
#include "XpalWindow.h"
#include "debug.h"

extern bool DataTypes;

#if defined(__POWERPC__)
#pragma optimization_level 1
#endif

#define GET_AND_SET                       \
	{                                     \
		B_GET_PROPERTY, B_SET_PROPERTY, 0 \
	}
#define GET               \
	{                     \
		B_GET_PROPERTY, 0 \
	}
#define SET               \
	{                     \
		B_SET_PROPERTY, 0 \
	}
#define DIRECT_AND_INDEX                         \
	{                                            \
		B_DIRECT_SPECIFIER, B_INDEX_SPECIFIER, 0 \
	}
#define DIRECT                \
	{                         \
		B_DIRECT_SPECIFIER, 0 \
	}

static property_info prop_list[] = {{"Name", GET, DIRECT, "Get Canvas name"},
	{"Layer", {B_CREATE_PROPERTY, 0}, DIRECT, "Name (string)"},
	{"Layer", GET, DIRECT_AND_INDEX, ""},
	{"ActiveLayer", GET_AND_SET, DIRECT_AND_INDEX, "Get and set currently active layer"}, 0};

static value_info value_list[] = {
	{"Export", 'expt', B_COMMAND_KIND, "Export the current canvas. Name|Filename (string)"},
	{"Crop", 'Crop', B_COMMAND_KIND, "Crop the current canvas to the given BRect"}, 0};

float zoomLevels[] = {0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0};
char aspectLevels[][7] = {"(1:8)", "(1:4)", "(1:2)", "(1:1)", "(2:1)", "(4:1)", "(8:1)", "(16:1)"};

CanvasWindow::CanvasWindow(BRect frame, const char* name, BBitmap* map, BMessenger* target,
	bool AskForAlpha, rgb_color color)
	: BWindow(frame, name, B_DOCUMENT_WINDOW, B_PULSE_NEEDED)
{
	BEntry entry = BEntry(name);
	entry.GetRef(&myRef);
	fromRef = false;
	char fname[MAX_FNAME];
	strcpy(fName, name);
	strcpy(fname, name);
	fNumberFormat.FormatPercent(fPercentData, 1.0);
	fTitleString.SetToFormat("%s (%s)", fname, fPercentData.String());
	SetTitle(fTitleString.String());
	if (target) {
		// printf ("Target found\n");
		myTarget = new BMessenger(*target);
	} else
		myTarget = NULL;
	restOfCtor(frame, map, NULL, AskForAlpha, color);
}

CanvasWindow::CanvasWindow(BRect frame, entry_ref ref, bool AskForAlpha, BMessenger* target)
	: BWindow(frame, "", B_DOCUMENT_WINDOW, uint32(NULL))
{
	extern const char* Version;
	myRef = ref;
	fromRef = true;
	if (target) {
		// printf ("Target found\n");
		myTarget = new BMessenger(*target);
	} else
		myTarget = NULL;
	BBitmap* map = NULL;
	char fname[MAX_FNAME];
	BEntry entry;
	entry.SetTo(&ref, true);
	BFile file = BFile(&ref, B_READ_ONLY);
	entry.GetName(fname);
	strcpy(fName, fname);
	fNumberFormat.FormatPercent(fPercentData, 1.0);
	fTitleString.SetToFormat("%s (%s)", fname, fPercentData.String());
	SetTitle(fTitleString.String());
	char type[80];
	BNode node = BNode(&entry);
	ssize_t s;
	FILE* fp = NULL;
	verbose(2, "Checking file type\n");
	if ((s = node.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, type, 79)) > 0) {
		if (!strcmp(type, "image/becasso") || !strcmp(type, "image/x-becasso")) {
			verbose(2, "Becasso native!\n");
			BPath path;
			entry.GetPath(&path);
			fp = fopen(path.Path(), "r");
			char line[81];
			do {
				fgets(line, 80, fp);
			} while (line[0] == '#');
			char* endp = line;
			float tvers = atof(Version);
			float cvers = strtod(endp, &endp);
			*endp = 0;
			if (int(cvers) > int(tvers))  // Check major version
			{
				char string[B_FILE_NAME_LENGTH + 128];
				sprintf(string,
					lstring(130,
						"‘%s’ is an image from a newer Becasso version (%s).\nYou can't load "
						"these files with this Becasso version."),
					fName, line);
				BAlert* alert = new BAlert("", string, lstring(131, "Cancel"), NULL, NULL,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go();
				fclose(fp);
				throw(1);
			}
			if (int(cvers) == int(tvers) &&
				cvers - int(cvers) > tvers - int(tvers))  // Check release
			{
				char string[B_FILE_NAME_LENGTH + 128];
				sprintf(string,
					lstring(132,
						"‘%s’ is an image from a newer release of Becasso (%s).\nTry to load "
						"anyway?"),
					fName, line);
				BAlert* alert = new BAlert("", string, lstring(131, "Cancel"),
					lstring(133, "Proceed"), NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				ulong res = alert->Go();
				if (res == 0) {
					fclose(fp);
					throw(2);
				}
			}
			fgets(line, 80, fp);
			endp = line;
			frame.left = strtod(endp, &endp);
			frame.top = strtod(endp, &endp);
			frame.right = strtod(endp, &endp);
			frame.bottom = strtod(endp, &endp);
			add_to_recent(ref);
			restOfCtor(frame, NULL, fp, AskForAlpha);
			return;
		}
	}

	if (DataTypes) {
		ProgressiveBitmapStream* bms = new ProgressiveBitmapStream();
		//		bms->DisplayProgressBar (BPoint (200, 200), "Reading Image…");
		BFile inStream(file);
#if defined(DATATYPES)
		if (DATATranslate(inStream, NULL, NULL, *bms, DATA_BITMAP)) {
			char string[B_FILE_NAME_LENGTH + 64];
			sprintf(string, "Datatype error:\nCouldn't translate `%s'", fName);
			BAlert* alert =
				new BAlert("", string, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->Go();
			throw(0);
		} else {
			map = bms->/*Get*/ Bitmap();
			bms->SetDispose(false);
		}
		delete bms;
#else
		BPath path;
		entry.GetPath(&path);
		long err = B_ERROR;
		const char* mimeStr = NULL;
		char str[B_FILE_NAME_LENGTH];
		translator_info info;

		if (file.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, str, B_FILE_NAME_LENGTH) == B_OK)
			mimeStr = str;

		if (mimeStr)
			err = BTranslatorRoster::Default()->Identify(&inStream, NULL, &info, 0, mimeStr);

		if (err)  // try without a hint
			err = BTranslatorRoster::Default()->Identify(&inStream, NULL, &info);

		if (!err) {
			err = BTranslatorRoster::Default()->Translate(
				&inStream, &info, NULL, bms, B_TRANSLATOR_BITMAP);
			if (!err) {
				map = bms->Bitmap();
				bms->SetDispose(false);
			}
		}
		delete bms;

		if (err || !map) {
			char errstring[B_FILE_NAME_LENGTH + 64];
			sprintf(
				errstring, lstring(134, "Translation Kit error:\nCouldn't translate `%s'"), fName);
			BAlert* alert = new BAlert("", errstring, lstring(135, "Help"), lstring(136, "OK"),
				NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			uint32 button = alert->Go();
			if (button == 0) {
				BNode node(&entry);
				char filetype[80];
				filetype[node.ReadAttr("BEOS:TYPE", 0, 0, filetype, 80)] = 0;
				if (!strncmp(filetype, "image/", 6)) {
					sprintf(errstring,
						lstring(137,
							"The file '%s' is of type '%s'.\nThis does look like an image format "
							"to me.\nMaybe you haven't installed the corresponding Translator?"),
						fName, filetype);
				} else {
					sprintf(errstring,
						lstring(138,
							"The file '%s' is of type '%s'.\nI don't think this is an image at "
							"all."),
						fName, filetype);
				}
				alert = new BAlert("", errstring, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
				alert->Go();
			}
			throw(0);
		}
#endif
	} else {
		printf("No DataTypes (?!)\n");
		// No DataTypes...
		throw(0);
	}
	//	add_to_recent (ref);
	restOfCtor(frame, map, fp, AskForAlpha);
}

void
CanvasWindow::restOfCtor(
	BRect canvasFrame, BBitmap* map, FILE* fp, bool AskForAlpha, rgb_color color)
{
	SetPulseRate(50000);
	magWindow = NULL;
	layerOpen = false;
	menuIsOn = false;
	scripted = false;
	BRect frame, hFrame, vFrame, menubarFrame, posFrame;
	menubarFrame.Set(0, 0, 0, 0);
	menubar = new BMenuBar(menubarFrame, "CanvasMenubar");
	fileMenu = new BMenu(lstring(10, "File"));
	BMenu* captureMenu = new BMenu(lstring(14, "Capture"));
	captureMenu->SetEnabled(false);
	fileMenu->AddItem(new BMenuItem(lstring(11, "New Canvas…"), new BMessage('newc'), 'N'));
	//	fileMenu->AddItem (new BMenuItem (lstring (12, "Open File…"), new BMessage ('open'), 'O'));
	BMenuItem* recent = make_recent_menu();
	recent->SetTarget(be_app);
	fileMenu->AddItem(recent);	// new BMenu (lstring (13, "Open Recent"));
	fileMenu->AddItem(captureMenu);
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(lstring(50, "Save"), new BMessage('save'), 'S'));
	fileMenu->AddItem(
		new BMenuItem(lstring(51, "Save As…"), new BMessage('savs'), 'S', B_SHIFT_KEY));
	fileMenu->AddItem(
		new BMenuItem(lstring(52, "Export…"), new BMessage('Expt'), 'S', B_CONTROL_KEY));
	extern bool ExportCstruct;
	if (ExportCstruct)
		fileMenu->AddItem(new BMenuItem("Export as C struct", new BMessage('ExpC')));
	fileMenu->AddSeparatorItem();
	BMenuItem* item = new BMenuItem(lstring(15, "Print Setup…"), new BMessage('PrSU'));
	item->SetTarget(be_app);
	fileMenu->AddItem(item);
	fileMenu->AddItem(new BMenuItem(lstring(53, "Print"), new BMessage('PrPP')));
	fileMenu->AddSeparatorItem();
	item = new BMenuItem(lstring(16, "About Becasso"), new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	fileMenu->AddItem(item);
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(lstring(54, "Close"), new BMessage(B_QUIT_REQUESTED), 'W'));
	fileMenu->AddItem(new BMenuItem(lstring(55, "Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));
	menubar->AddItem(fileMenu);
	fileMenu->FindItem('newc')->SetTarget(be_app);
#ifndef BE_RECENT_MENU
	fileMenu->FindItem('open')->SetTarget(be_app);
#endif
	fileMenu->FindItem(lstring(55, "Quit"))->SetTarget(be_app);
	// N.B. FindItem(Q_QUIT_REQUESTED) doesn't work!?
	editMenu = new BMenu(lstring(60, "Edit"));
	editMenu->AddItem(new BMenuItem(lstring(61, "Undo"), new BMessage(B_UNDO), 'Z'));
	editMenu->AddItem(new BMenuItem(lstring(62, "Redo"), new BMessage('redo'), 'Z', B_SHIFT_KEY));
	editMenu->AddSeparatorItem();
	editMenu->AddItem(new BMenuItem(lstring(63, "Cut"), new BMessage(B_CUT), 'X'));
	editMenu->AddItem(new BMenuItem(lstring(64, "Copy"), new BMessage(B_COPY), 'C'));
	editMenu->AddItem(new BMenuItem(lstring(65, "Paste"), new BMessage(B_PASTE), 'V'));
	editMenu->AddItem(
		new BMenuItem(lstring(403, "Paste to New Layer"), new BMessage('psNL'), 'V', B_SHIFT_KEY));
	editMenu->AddItem(new BMenuItem(
		lstring(404, "Paste to New Canvas"), new BMessage('psNC'), 'V', B_CONTROL_KEY));
	editMenu->AddItem(new BMenuItem(lstring(66, "Select All"), new BMessage(B_SELECT_ALL), 'A'));
	editMenu->AddItem(new BMenuItem(lstring(67, "Deselect All"), new BMessage('sund'), 'D'));
	editMenu->AddItem(new BMenuItem(lstring(68, "Invert Selection"), new BMessage('sinv')));
	BMenu* ktsMenu = new BMenu(lstring(69, "Select by Color"));
	ktsMenu->AddItem(new BMenuItem(lstring(70, "Foreground"), new BMessage('ktsF')));
	ktsMenu->AddItem(new BMenuItem(lstring(71, "Background"), new BMessage('ktsB')));
	ktsMenu->AddItem(new BMenuItem(lstring(72, "Automatic"), new BMessage('ktsA')));
	editMenu->AddItem(ktsMenu);
	BMenu* stkMenu = new BMenu(lstring(73, "Colorize Selection"));
	stkMenu->AddItem(new BMenuItem(lstring(70, "Foreground"), new BMessage('stkF')));
	stkMenu->AddItem(new BMenuItem(lstring(71, "Background"), new BMessage('stkB')));
	editMenu->AddItem(stkMenu);
	editMenu->AddItem(new BMenuItem(lstring(74, "Extract Palette…"), new BMessage('xpal')));
	BMenu* ctsMenu = new BMenu(lstring(75, "Channel to Selection"));
	ctsMenu->AddItem(new BMenuItem(lstring(76, "Alpha"), new BMessage('ctsA')));
	ctsMenu->AddItem(new BMenuItem(lstring(77, "Red"), new BMessage('ctsR')));
	ctsMenu->AddItem(new BMenuItem(lstring(78, "Green"), new BMessage('ctsG')));
	ctsMenu->AddItem(new BMenuItem(lstring(79, "Blue"), new BMessage('ctsB')));
	ctsMenu->AddSeparatorItem();
	ctsMenu->AddItem(new BMenuItem(lstring(80, "Cyan"), new BMessage('ctsC')));
	ctsMenu->AddItem(new BMenuItem(lstring(81, "Magenta"), new BMessage('ctsM')));
	ctsMenu->AddItem(new BMenuItem(lstring(82, "Yellow"), new BMessage('ctsY')));
	ctsMenu->AddItem(new BMenuItem(lstring(83, "Black"), new BMessage('ctsK')));
	ctsMenu->AddSeparatorItem();
	ctsMenu->AddItem(new BMenuItem(lstring(84, "Hue"), new BMessage('ctsH')));
	ctsMenu->AddItem(new BMenuItem(lstring(85, "Saturation"), new BMessage('ctsS')));
	ctsMenu->AddItem(new BMenuItem(lstring(86, "Value"), new BMessage('ctsV')));
	editMenu->AddItem(ctsMenu);
	BMenu* stcMenu = new BMenu(lstring(87, "Selection to Channel"));
	stcMenu->AddItem(new BMenuItem(lstring(76, "Alpha"), new BMessage('stcA')));
	stcMenu->AddItem(new BMenuItem(lstring(77, "Red"), new BMessage('stcR')));
	stcMenu->AddItem(new BMenuItem(lstring(78, "Green"), new BMessage('stcG')));
	stcMenu->AddItem(new BMenuItem(lstring(79, "Blue"), new BMessage('stcB')));
	stcMenu->AddSeparatorItem();
	stcMenu->AddItem(new BMenuItem(lstring(80, "Cyan"), new BMessage('stcC')));
	stcMenu->AddItem(new BMenuItem(lstring(81, "Magenta"), new BMessage('stcM')));
	stcMenu->AddItem(new BMenuItem(lstring(82, "Yellow"), new BMessage('stcY')));
	stcMenu->AddItem(new BMenuItem(lstring(83, "Black"), new BMessage('stcK')));
	stcMenu->AddSeparatorItem();
	stcMenu->AddItem(new BMenuItem(lstring(84, "Hue"), new BMessage('stcH')));
	stcMenu->AddItem(new BMenuItem(lstring(85, "Saturation"), new BMessage('stcS')));
	stcMenu->AddItem(new BMenuItem(lstring(86, "Value"), new BMessage('stcV')));
	editMenu->AddItem(stcMenu);
	editMenu->AddSeparatorItem();
	BMenu* imageMenu = new BMenu(lstring(90, "Entire Canvas"));
	imageMenu->AddItem(new BMenuItem(lstring(91, "Crop to Window"), new BMessage('cwin')));
	imageMenu->AddItem(new BMenuItem(lstring(92, "Crop to Selection"), new BMessage('csel')));
	imageMenu->AddItem(new BMenuItem(lstring(93, "Autocrop"), new BMessage('caut')));
	BMenu* padMenu = new BMenu(lstring(94, "Pad to Window"));
	padMenu->AddItem(new BMenuItem(lstring(95, "Left Top"), new BMessage('pwlt')));
	padMenu->AddItem(new BMenuItem(lstring(96, "Right Top"), new BMessage('pwrt')));
	padMenu->AddItem(new BMenuItem(lstring(97, "Left Bottom"), new BMessage('pwlb')));
	padMenu->AddItem(new BMenuItem(lstring(98, "Right Bottom"), new BMessage('pwrb')));
	padMenu->AddItem(new BMenuItem(lstring(99, "Center"), new BMessage('pwct')));
	imageMenu->AddItem(padMenu);
	BMenu* resizeMenu = new BMenu(lstring(100, "Resize to Window"));
	resizeMenu->AddItem(new BMenuItem(lstring(101, "Preserve Ratio"), new BMessage('rwkr')));
	resizeMenu->AddItem(new BMenuItem(lstring(102, "Adapt Ratio"), new BMessage('rwar')));
	imageMenu->AddItem(resizeMenu);
	imageMenu->AddItem(new BMenuItem(lstring(107, "Resize To…"), new BMessage('rszT')));
	BMenu* rotateMenu = new BMenu(lstring(89, "Rotate"));
	rotateMenu->AddItem(new BMenuItem("90°", new BMessage('r90')));
	rotateMenu->AddItem(new BMenuItem("180°", new BMessage('r180')));
	rotateMenu->AddItem(new BMenuItem("270°", new BMessage('r270')));
	imageMenu->AddItem(rotateMenu);
	editMenu->AddItem(imageMenu);
	BMenu* filters = new BMenu(lstring(103, "Filter"));
	BMenu* transformers = new BMenu(lstring(104, "Transform"));
	BMenu* generators = new BMenu(lstring(105, "Generate"));
	extern BList* AddOns;
	AddOn* addon;
	for (long i = 0; addon = (AddOn*)AddOns->ItemAt(i), addon; i++) {
		BMessage* m = new BMessage('adon');
		m->AddInt32("index", i);
		switch (addon->Type()) {
			case BECASSO_FILTER:
			{
				filters->AddItem(new BMenuItem(addon->Name(), m));
				break;
			}
			case BECASSO_TRANSFORMER:
			{
				transformers->AddItem(new BMenuItem(addon->Name(), m));
				break;
			}
			case BECASSO_GENERATOR:
			{
				generators->AddItem(new BMenuItem(addon->Name(), m));
				break;
			}
			case BECASSO_CAPTURE:
			{
				delete m;
				BMessage* ms = new BMessage('capt');
				ms->AddInt32("index", i);
				//			BMenuItem *it = new BMenuItem (addon->Name(), ms);
				captureMenu->AddItem(new BMenuItem(addon->Name(), ms));
				captureMenu->SetEnabled(true);
				break;
			}
			default:
				fprintf(stderr, "Unknown Add-On type! (%li: %i)\n", i, addon->Type());
				break;
		}
	}
	editMenu->AddItem(filters);
	editMenu->AddItem(transformers);
	editMenu->AddItem(generators);
	editMenu->AddSeparatorItem();
	editMenu->AddItem(new BMenuItem(lstring(106, "Center Mouse"), new BMessage('cmus'), 'M'));
	menubar->AddItem(editMenu);
	windowMenu = new BMenu(lstring(110, "Window"));
	windowMenu->AddItem(new BMenuItem(lstring(111, "Layers"), new BMessage('layr'), 'L'));
	windowMenu->AddItem(new BMenuItem(lstring(116, "Magnify"), new BMessage('mwin')));

	BMenu* magnifyMenu = new BMenu(lstring(112, "Scale"));
	magnifyMenu->AddItem(new BMenuItem(lstring(113, "Zoom In"), new BMessage('Min'), '+'));
	magnifyMenu->AddItem(new BMenuItem(lstring(114, "Zoom Out"), new BMessage('Mout'), '-'));

	for (int i = 0; i < B_COUNT_OF(zoomLevels) && i < B_COUNT_OF(aspectLevels); ++i) {
		fNumberFormat.FormatPercent(fPercentData, zoomLevels[i]);
		BMessage* message = new BMessage('M_S');
		message->AddFloat("scale", zoomLevels[i]);
		message->AddInt32("index", 1);

		if (zoomLevels[i] >= 10.0) {
			BString separator(fNumberFormat.GetSeparator(B_GROUPING_SEPARATOR));
			fPercentData.RemoveFirst(separator);
		}

		BString label;
		label.SetToFormat("%s %s", fPercentData.String(), aspectLevels[i]);
		magnifyMenu->AddItem(new BMenuItem(label.String(), message));
	}

	windowMenu->AddItem(magnifyMenu);

	windowMenu->AddItem(new BMenuItem(lstring(115, "Resize to Fit"), new BMessage('rstf'), 'Y'));
	menubar->AddItem(windowMenu);
	layerMenu = new BMenu(lstring(120, "Layer"));
	layerMenu->AddItem(new BMenuItem(lstring(121, "Add New"), new BMessage('newL')));
	layerMenu->AddItem(new BMenuItem(lstring(122, "Insert New"), new BMessage('insL')));
	layerMenu->AddItem(new BMenuItem(lstring(123, "Duplicate"), new BMessage('dupL')));
	layerMenu->AddItem(new BMenuItem(lstring(124, "Remove"), new BMessage('delL')));
	layerMenu->AddItem(new BMenuItem(lstring(125, "Merge"), new BMessage('mrgL')));
	layerMenu->AddSeparatorItem();
	layerMenu->AddItem(new BMenuItem(lstring(88, "Translate"), new BMessage('trsL')));
	layerMenu->AddItem(new BMenuItem(lstring(89, "Rotate"), new BMessage('rotL')));
	BMenu* flipMenu = new BMenu(lstring(108, "Flip"));
	flipMenu->AddItem(new BMenuItem(lstring(400, "Horizontal"), new BMessage('flpH')));
	flipMenu->AddItem(new BMenuItem(lstring(401, "Vertical"), new BMessage('flpV')));
	layerMenu->AddItem(flipMenu);
	layerMenu->AddSeparatorItem();
	layerMenu->AddItem(new BMenuItem(lstring(126, "Up One"), new BMessage('Loup'), '['));
	layerMenu->AddItem(new BMenuItem(lstring(127, "Down One"), new BMessage('Lodn'), ']'));
	layerNamesMenu = new BMenu(lstring(128, "Select"));
	layerMenu->AddItem(layerNamesMenu);
	menubar->AddItem(layerMenu);
	AddChild(menubar);
	menubar->ResizeToPreferred();
	menubarFrame = menubar->Frame();
	if (map)
		canvasFrame = map->Bounds();
	//	canvasFrame.PrintToStream();
	frame.Set(0, 0, max_c(canvasFrame.Width() + B_V_SCROLL_BAR_WIDTH, MIN_WIDTH),
		max_c(
			canvasFrame.Height() + B_H_SCROLL_BAR_HEIGHT + menubarFrame.Height() + 1, MIN_HEIGHT));
	//	frame.PrintToStream();
	{
		BScreen screen;
		SetZoomLimits(screen.Frame().Width(), screen.Frame().Height());
	}
	SetSizeLimits(MIN_WIDTH, 9999, MIN_HEIGHT, 9999);
	ResizeTo(frame.Width(), frame.Height());
	BRect bgFrame = BRect(0, 0, max_c(MIN_WIDTH - B_V_SCROLL_BAR_WIDTH, canvasFrame.Width()),
		max_c(
			MIN_HEIGHT - B_H_SCROLL_BAR_HEIGHT - menubarFrame.Height() - 1, canvasFrame.Height()));
	bgFrame.OffsetTo(menubarFrame.left, menubarFrame.bottom + 1);
	//	bgFrame.PrintToStream();
	frame.OffsetTo(menubarFrame.left, menubarFrame.bottom + 1);
	bg = new BGView(bgFrame, "Background View", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS);
	AddChild(bg);
	canvasFrame.OffsetTo(B_ORIGIN);

	//////////////////
	if (fp)
		canvas = new CanvasView(canvasFrame, "Canvas", fp);
	else
		canvas = new CanvasView(canvasFrame, "Canvas", map, color);
	//////////////////
	if (map && AskForAlpha)	 // AskForAlpha check added here as optimization
	{
		////////////////////////////////////////
		float avg = AverageAlpha(map);
		if (avg < 128) {
			if (AskForAlpha) {
				char alert[2048];
				sprintf(alert,
					lstring(139,
						"%s the image `%s' uses alpha as transparency instead of opacity.\n%s "
						"inverting the alpha channel."),
					(avg < 10) ? lstring(140, "I'm pretty sure") : lstring(141, "I suspect"), fName,
					(avg < 10)
						? lstring(142, "If you're seeing only a checkered pattern,\nI suggest ")
						: lstring(143,
							  "Either that, or this image has an awful lot of transparent "
							  "pixels.  If it looks strange, try "));
				BAlert* alphalert = new BAlert(
					"alphalert", alert, lstring(144, "Keep As-Is"), lstring(145, "Invert"));
				/* int32 button_index = */ alphalert->Go(new BInvoker(new BMessage('aliv'), this));
			} else	// Changed in 1.5: Don't invert without asking.
			{
				//				verbose (1, "Inverting alpha channel\n");
				//				BMessage invert ('aliv');
				//				invert.AddMessenger ("canvas", this);
				//				be_app->PostMessage (&invert);
			}
		}
		////////////////////////////////////////
		delete map;
		map = NULL;
	}
	posFrame.Set(-1, frame.Height() - B_H_SCROLL_BAR_HEIGHT + 1, POSWIDTH - 1, frame.Height() + 1);
	posview = new PosView(posFrame, "PosView", canvas);
	hFrame.Set(POSWIDTH, frame.Height() - B_H_SCROLL_BAR_HEIGHT + 1,
		frame.Width() - B_V_SCROLL_BAR_WIDTH + 1, frame.Height() + 1);
	h = new BScrollBar(hFrame, "sbH", bg, 0, canvasFrame.Width(), B_HORIZONTAL);
	vFrame.Set(frame.Width() - B_V_SCROLL_BAR_WIDTH + 1, menubarFrame.Height(), frame.Width() + 1,
		frame.Height() - B_H_SCROLL_BAR_HEIGHT + 1);
	v = new BScrollBar(vFrame, "sbV", bg, 0, canvasFrame.Height(), B_VERTICAL);
	AddChild(h);
	AddChild(v);
	bg->AddChild(canvas);
	bg->FrameResized(bgFrame.Width(), bgFrame.Height());
	canvas->setBGView(bg);
	AddChild(posview);
	quitting = false;
	savePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false);
	extern entry_ref gSaveRef;
	// Make this a preference!
	savePanel->SetPanelDirectory(&gSaveRef);
	outputFormatWindow = NULL;
	bitmapStream = NULL;
	of_selected = new BInvoker(new BMessage('oFsl'), this);
	out_type = 0;
	fScale = 1;
	setLayerMenu();
	fCurrentProperty = 0;
	ieTarget = NULL;  // BeDC hack for interaction with ImageElements
	extractWindow = NULL;

	// Notify the main app so it can add us to its canvases list.
	BMessage notify('creg');
	notify.AddPointer("looper", Looper());
	notify.AddString("name", fName);
	be_app->PostMessage(&notify);
	strcpy(fCanvasName, fName);	 // fName can change due to "save as" and exporting.

	//	printf ("CanvasWindow::ctor done\n");
}

CanvasWindow::~CanvasWindow()
{
	delete bitmapStream;
	delete of_selected;
	delete savePanel;
	delete myTarget;

	// Notify the main app so it can remove us from its canvases list.
	BMessage notify('cdel');
	notify.AddPointer("looper", Looper());
	be_app->PostMessage(&notify);
}

void
CanvasWindow::MenusBeginning()
{
	//	make_recent_menu (recentMenu);
	menuIsOn = true;
}

void
CanvasWindow::MenusEnded()
{
	menuIsOn = false;
}

void
CanvasWindow::CanvasResized(float /* width */, float /* height */)
{
	//	printf ("width = %f, height = %f\n", width, height);
	//	BRect canvasFrame = BRect (0, 0, width, height);
	//	BRect menubarFrame = menubar->Frame();
	//	BRect frame = Frame();
	//	frame.Set (0, 0,
	//		max_c (frame.Width() + B_V_SCROLL_BAR_WIDTH, MIN_WIDTH),
	//		max_c (frame.Height() + B_H_SCROLL_BAR_HEIGHT + menubarFrame.Height() + 1, MIN_HEIGHT));
	//	frame.PrintToStream();
	//	ResizeTo (frame.Width(), frame.Height());
}

#if defined(__POWERPC__)
#pragma optimization_level 4
#endif

void
CanvasWindow::setLayerMenu()
{
	layerMenu->RemoveItem(layerNamesMenu);
	delete layerNamesMenu;
	layerNamesMenu = new BMenu(lstring(129, "Select Layer"));
	for (int i = canvas->numLayers() - 1; i >= 0; i--) {
		BMessage* lmsg = new BMessage('lSel');
		lmsg->AddInt32("index", i);
		BMenuItem* item = new BMenuItem(canvas->getLayer(i)->getName(), lmsg);
		if (i == canvas->currentLayerIndex())
			item->SetMarked(true);
		layerNamesMenu->AddItem(item);
	}
	layerMenu->AddItem(layerNamesMenu);
	if (canvas->currentLayerIndex() == 0) {
		layerMenu->FindItem('delL')->SetEnabled(false);
		layerMenu->FindItem('mrgL')->SetEnabled(false);
		layerMenu->FindItem('Lodn')->SetEnabled(false);
	} else {
		layerMenu->FindItem('delL')->SetEnabled(true);
		layerMenu->FindItem('mrgL')->SetEnabled(true);
		layerMenu->FindItem('Lodn')->SetEnabled(true);
	}
	if (canvas->currentLayerIndex() < canvas->numLayers() - 1)
		layerMenu->FindItem('Loup')->SetEnabled(true);
	else
		layerMenu->FindItem('Loup')->SetEnabled(false);
}

void
CanvasWindow::CanvasScaled(float s)
{
	fScale = s;
	fNumberFormat.FormatPercent(fPercentData, (double)s);
	fTitleString.SetToFormat("%s (%s)", fName, fPercentData.String());
	SetTitle(fTitleString.String());
	FrameResized(Bounds().Width(), Bounds().Height());
}

void
CanvasWindow::setName(char* s)
{
	strcpy(fName, s);
	fNumberFormat.FormatPercent(fPercentData, (double)fScale);
	fTitleString.SetToFormat("%s (%s)", fName, fPercentData.String());
	SetTitle(fTitleString.String());
}

void
CanvasWindow::setMenuItem(uint32 command, bool v)
{
	BMenuItem* item = editMenu->FindItem(command);
	if (item) {
		//		char *cmd = (char *)(&command);
		//		printf ("Item '%c%c%c%c' %sabled\n", cmd[3], cmd[2], cmd[1], cmd[0], v ? "en" :
		//"dis");
		item->SetEnabled(v);
	} else
		printf("Item not found\n");
}

void
CanvasWindow::setPaste(bool v)
{
	BMessage* msg = new BMessage('SPst');
	msg->AddBool("status", v);
	be_app->PostMessage(msg);
	delete msg;
}

void
CanvasWindow::FrameResized(float width, float height)
{
	BRect canvasFrame = canvas->Frame();
	width -= B_V_SCROLL_BAR_WIDTH;
	height -= B_H_SCROLL_BAR_HEIGHT + menubar->Frame().Height() + 1;
	//	printf ("%f, %f; %f, %f\n", width, canvasFrame.Width(), height, canvasFrame.Height());
	if (width < canvasFrame.Width() || height < canvasFrame.Height()) {
		setMenuItem('cwin', true);
		setMenuItem('pwlt', false);
		setMenuItem('pwrt', false);
		setMenuItem('pwlb', false);
		setMenuItem('pwrb', false);
		setMenuItem('pwct', false);
	} else {
		setMenuItem('cwin', false);
		setMenuItem('pwlt', true);
		setMenuItem('pwrt', true);
		setMenuItem('pwlb', true);
		setMenuItem('pwrb', true);
		setMenuItem('pwct', true);
	}
}

void
CanvasWindow::FrameMoved(BPoint screenpoint)
{
	// 	Align the window on an 8x8 grid to prevent pattern misalignment.
	//	if (MessageQueue()->FindMessage (B_WINDOW_MOVED, 1) == NULL)
	//	{
	//		double dx = fmod (screenpoint.x, 8.0);
	//		double dy = fmod (screenpoint.y, 8.0);
	//		if (dx || dy)
	//			MoveBy (dx, dy);
	//	}
	inherited::FrameMoved(screenpoint);
}

void
CanvasWindow::ScreenChanged(BRect frame, color_space mode)
{
	Lock();
	canvas->ScreenChanged(frame, mode);
	Unlock();
}

void
CanvasWindow::Quit()
{
	extern bool inPaste;
	quitting = true;
	inPaste = false;
	if (layerOpen) {
#if defined(VERBOSEQUIT)
		printf("LayerWindow Open\n");
#endif
		if (layerWindow->Lock()) {
#if defined(VERBOSEQUIT)
			printf("Closing...\n");
#endif
			layerWindow->Close();
		}
	}
	if (magWindow) {
#if defined(VERBOSEQUIT)
		printf("MagWindow Open\n");
#endif
		if (magWindow->Lock()) {
#if defined(VERBOSEQUIT)
			printf("Closing\n");
#endif
			magWindow->Close();
		}
	}
	if (canvas->FilterOpen()) {
		//		printf ("FilterOpen\n");
		canvas->FilterOpen()->Close(true);
	}
	if (canvas->TransformerOpen()) {
		//		printf ("TransformerOpen\n");
		canvas->TransformerOpen()->Close(true);
	}
	if (canvas->GeneratorOpen()) {
		//		printf ("GeneratorOpen\n");
		canvas->GeneratorOpen()->Close(true);
	}
	if (myTarget) {
		BMessage* ie = new BMessage(BBP_BBITMAP_CLOSED);
		myTarget->SendMessage(ie);
		delete ie;
	}
	inherited::Quit();
}

BMessage*
CanvasWindow::sendbitmapreply()
{
	BMessage* ie = new BMessage(BBP_SEND_BBITMAP);
	BMessage ieb;
	ie->AddMessenger("target", this);
	BBitmap* b = canvas->canvas(true);
	if (b->Archive(&ieb, false) == B_OK) {
		ie->AddMessage("BBitmap", &ieb);
		canvas->changed = false;
	} else {
		// Error
		fprintf(stderr, "Error archiving BBitmap for sending\n");
	}
	delete b;
	return ie;
}

bool
CanvasWindow::QuitRequested()
{
	//	printf ("%s: QuitRequested\n", fName);
	if (!(canvas->changed)) {
		return (true);
	}

	if (savePanel->IsShowing()) {
		while (savePanel->IsShowing())
			snooze(500000);	 // YUCK!!!
		return (!(canvas->changed));
	}

	char question[MAX_ALERTSIZE];
	sprintf(question, lstring(146, "Save changes to ‘%s’?"), fName);

	BAlert* alert = new BAlert("", question, lstring(131, "Cancel"), lstring(147, "Abandon"),
		lstring(148, "Save"), B_WIDTH_FROM_WIDEST, B_INFO_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	int32 result = alert->Go();
	switch (result) {
		case 2:	 // Save
			if (myTarget && !fromRef) {
				myTarget->SendMessage(sendbitmapreply());
				return (true);
			} else {
				savePanel->Show();
				while (savePanel->IsShowing())
					snooze(500000);	 // YUCK!!!
				return (!(canvas->changed));
			}
		case 1:	 // Abandon
			return (true);
		default:  // Cancel
			return (false);
	}
}

status_t
CanvasWindow::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-canvas");
	BPropertyInfo info(prop_list, value_list);
	message->AddFlat("messages", &info);
	return inherited::GetSupportedSuites(message);
}

BHandler*
CanvasWindow::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property)
{
	//	printf ("CanvasWindow::ResolveSpecifier!!  message:\n");
	//	message->PrintToStream();
	//	printf ("\nspecifier:\n");
	//	specifier->PrintToStream();
	//	printf ("command: %d, property: ""%s""\n", command, property);

#if 0

	// Currently, this stuff is app-global.  We shouldn't be here.

	if (!strcasecmp (property, "Tool"))
	{
		fCurrentProperty = PROP_TOOL;
		return this;
	}
	if (!strcasecmp (property, "Mode"))
	{
		fCurrentProperty = PROP_MODE;
		return this;
	}
	if (!strcasecmp (property, "Foreground"))
	{
		fCurrentProperty = PROP_FGCOLOR;
		return this;
	}
	if (!strcasecmp (property, "Background"))
	{
		fCurrentProperty = PROP_BGCOLOR;
		return this;
	}
	if (!strcasecmp (property, "ExportFormat"))
	{
		fCurrentProperty = PROP_EXPFMT;
		return this;
	}

#endif

	if (!strcasecmp(property, "Size")) {
		fCurrentProperty = PROP_SIZE;
		return this;
	}
	if (!strcasecmp(property, "Name")) {
		fCurrentProperty = PROP_NAME;
		return this;
	}
	//	if (!strcasecmp (property, "Canvas"))
	//	{
	//		fCurrentProperty = PROP_CANVAS;
	//		return this;
	//	}
	if (!strcasecmp(property, "ActiveLayer")) {
		fCurrentProperty = PROP_ACTIVELAYER;
		return this;
	}
	if (!strcasecmp(property, "Layer")) {
		if (message->what == B_CREATE_PROPERTY) {
			fCurrentProperty = PROP_LAYER;
			return this;
		} else	// We get here if "Layer" was a specifier.
		{
			// Don't PopSpecifier()... The canvas still needs to find out which layer...
			return canvas;
		}
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

#if defined(__POWERPC__)
#pragma optimization_level 1
#endif

void
CanvasWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_CREATE_PROPERTY:
		{
			// message->PrintToStream();
			switch (fCurrentProperty) {
				case PROP_LAYER:
				{
					char title[MAXLAYERNAME];
					const char* namestring;
					if (message->FindString("Name", &namestring) == B_OK ||
						message->FindString("name", &namestring) == B_OK)
						strcpy(title, namestring);
					else
						strcpy(title, lstring(149, "Untitled"));

					canvas->addLayer(title);
					setLayerMenu();
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
			fCurrentProperty = 0;
			break;
		}
		case B_DELETE_PROPERTY:
		{
			fCurrentProperty = 0;
			inherited::MessageReceived(message);
			break;
		}
		case B_GET_PROPERTY:
		{
			switch (fCurrentProperty) {
				case PROP_NAME:
					if (message->IsSourceWaiting()) {
						BMessage reply(B_REPLY);
						reply.AddString("result", fCanvasName);
						message->SendReply(&reply);
					} else
						fprintf(stderr, "Canvas Name: \"%s\"\n", fCanvasName);
					break;
				case PROP_ACTIVELAYER:
					if (message->IsSourceWaiting()) {
						BMessage reply(B_REPLY);
						reply.AddInt32("error", B_NO_ERROR);
						reply.AddSpecifier("Layer", canvas->currentLayer()->getName());
						message->SendReply(&reply);
					} else
						fprintf(stderr, "Current Layer: %d: \"%s\"\n", canvas->currentLayerIndex(),
							canvas->currentLayer()->getName());
					break;
				default:
					break;
			}
			fCurrentProperty = 0;
			inherited::MessageReceived(message);
			break;
		}
		case B_SET_PROPERTY:
		{
			switch (fCurrentProperty) {
				case PROP_ACTIVELAYER:
				{
					const char* namedspecifier;
					int32 indexspecifier;
					if (message->FindString("data", &namedspecifier) == B_OK) {
						int32 index;
						int32 numlayers = canvas->numLayers();
						for (index = 0; index < numlayers; index++) {
							Layer* l = canvas->getLayer(index);
							if (!strcmp(namedspecifier, l->getName())) {
								canvas->makeCurrentLayer(index);
								setLayerMenu();
								if (message->IsSourceWaiting()) {
									BMessage error(B_REPLY);
									error.AddInt32("error", B_NO_ERROR);
									message->SendReply(&error);
								}
								break;
							}
						}
						if (index == numlayers) {
							char errstring[256];
							sprintf(errstring, "Unknown Layer \"%s\"", namedspecifier);
							if (message->IsSourceWaiting()) {
								BMessage error(B_ERROR);
								error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
								error.AddString("message", errstring);
								message->SendReply(&error);
							} else
								fprintf(stderr, "%s\n", errstring);
						}
					} else if (message->FindInt32("data", &indexspecifier) == B_OK) {
						if (indexspecifier >= 0 && indexspecifier < canvas->numLayers()) {
							canvas->makeCurrentLayer(indexspecifier);
							setLayerMenu();
							if (message->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								message->SendReply(&error);
							}
						} else {
							BString indexSpecifierData, numberOfLayersData, errorString;
							fNumberFormat.Format(numberOfLayersData, canvas->numLayers() - 1);
							fNumberFormat.Format(indexSpecifierData, indexspecifier);
							errorString.SetToFormat("Layer index out of range [0..%s]: %s",
								numberOfLayersData.String(), indexSpecifierData.String());
							if (message->IsSourceWaiting()) {
								BMessage error(B_ERROR);
								error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
								error.AddString("message", errorString.String());
								message->SendReply(&error);
							} else
								fprintf(stderr, "%s\n", errorString.String());
						}
					} else {
						char errstring[256];
						sprintf(errstring, "Invalid Layer Type (either string or int32 please)");
						if (message->IsSourceWaiting()) {
							BMessage error(B_ERROR);
							error.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
							error.AddString("message", errstring);
							message->SendReply(&error);
						} else
							fprintf(stderr, "%s\n", errstring);
					}
					break;
				}
				case PROP_GLOBALALPHA:
				{
					printf("PROP_GLOBALALPHA\n");
					break;
				}
				default:
					break;
			}
			fCurrentProperty = 0;
			inherited::MessageReceived(message);
			break;
		}
		case 'aliv':
		{
			int32 button;
			message->FindInt32("which", &button);
			if (button == 1)
				canvas->invertAlpha();
			break;
		}
		case BBP_REQUEST_BBITMAP:
			// printf ("CanvasWindow got BBP_REQUEST_BBITMAP\n");
			if (myTarget || message->HasString("hack"))	 // BeDC hack :-)
			{
				if (message->IsSourceWaiting())	 // Sync
				{
					// printf ("Source is waiting\n");
					message->SendReply(sendbitmapreply());
				} else	// Async
				{
					// printf ("Async\n");
					myTarget->SendMessage(sendbitmapreply());
				}
			} else
				beep();
			break;
		case BBP_REPLACE_BBITMAP:
		{
			//			printf ("Got Replace message\n");
			//			char title[MAXTITLE];
			//			char *name;
			//			if (message->FindString ("name", &name) == B_OK)
			//				strcpy (title, name);
			//			else
			//				sprintf (title, "Untitled %i", newnum++);
			BMessage archive;
			if (message->FindMessage("BBitmap", &archive) == B_OK) {
				BBitmap* map = new BBitmap(&archive);
				if (map->IsValid()) {
					BMessenger ie;
					message->FindMessenger("target", &ie);
					// float newWidth = map->Bounds().Width();
					// float newHeight = map->Bounds().Height();
					canvas->ReplaceCurrentLayer(map);
					//					BRect canvasWindowFrame;
					//					canvasWindowFrame.Set (128 + newnum*16, 128 + newnum*16,
					//						128 + newnum*16 + newWidth - 1, 128 + newnum*16 +
					//newHeight
					//- 1); 					CanvasWindow *canvasWindow = new CanvasWindow
					//(canvasWindowFrame, title, map, &ie); canvasWindow->Show();
					BMessage* ok = new BMessage(BBP_BBITMAP_OPENED);
					ok->AddMessenger("target", BMessenger(this, NULL));
					if (message->IsSourceWaiting()) {
						message->SendReply(ok);
					} else {
						ie.SendMessage(ok);
					}
					delete ok;
					float zoom;
					if (message->FindFloat("zoom", &zoom) == B_OK) {
						BMessage* m = new BMessage('Mnnn');
						m->AddFloat("zoom", zoom);
						PostMessage(m);
						delete m;
					}
				} else {
					fprintf(stderr, "Invalid BBitmap\n");
					// Error
				}
			} else {
				fprintf(stderr, "Message didn't contain a valid BBitmap\n");
				// Error
			}

			//			BMessage *copymsg = new BMessage (BBP_REPLACE_BBITMAP);
			//			copymsg->AddMessage ("message", message);
			//			copymsg->AddMessenger ("target", *myTarget);
			//			myTarget = NULL;
			//			canvas->changed = false;
			//			be_app->PostMessage (copymsg);
			//			printf ("Sent it to the main app.\n");
			//			Close();
			break;
		}
		case 'IErq':  // BeDC hack to interact with ImageElements
		{
			printf("CanvasWindow received IErq message\n");
			BMessage original;
			message->FindMessage("original", &original);
			//			original.FindMessenger ("target", ieTarget);
			//			ieTarget->SendMessage (sendbitmapreply());
			printf("Sending Reply...\n");
			original.SendReply(sendbitmapreply());
			printf("Got through\n");
			break;
		}
		case 'save':
		{
			int32 button = 1;
			if (myTarget && !fromRef)
				myTarget->SendMessage(sendbitmapreply());
			else if (!strncmp(fName, lstring(149, "Untitled"), strlen(lstring(149, "Untitled"))))
				savePanel->Show();
			else {
				if (myTarget && fromRef) {
					canvas->Save(BEntry(&myRef));

					BMessage* ie = new BMessage(BBP_SEND_BBITMAP);
					BMessage ieb;
					ie->AddMessenger("target", this);
					ie->AddRef("ref", &myRef);
					myTarget->SendMessage(ie);
				} else {
					BEntry entry(&myRef);

					if (entry.Exists())	 //// *** NOT TRANSLATED - COPIED FROM OS!!
					{
						char exists[2048];
						BPath path(&entry);
						sprintf(exists,
							"The file \"%s\" already exists in the specified folder. Do you want "
							"to "
							"replace it?",
							path.Leaf());
						button = (new BAlert("", exists, "Cancel", "Replace", NULL,
									  B_WIDTH_AS_USUAL, B_WARNING_ALERT))
									 ->Go();
					}

					if (button)
						canvas->Save(entry);
				}
			}
			if (button)
				break;
			// else - fall through to "save as" below
			// **************************************
		}
		case 'savs':
			savePanel->Show();
			break;
		case 'ExpC':
		{
			char name[1024];
			sprintf(name, "%s.c", fName);
			canvas->ExportCstruct(name);
			break;
		}
		case 'Expt':
			delete bitmapStream;
			bitmapStream = new ProgressiveBitmapStream(canvas->canvas(true));
			bitmapStream->SetDispose(true);
			//			if (outputFormatWindow)
			//			{
			//				printf ("Er was al een OutputFormatWindow\n");
			//				outputFormatWindow->Lock();
			//				outputFormatWindow->Quit();
			//			}
			//			delete outputFormatWindow;
			outputFormatWindow = new OutputFormatWindow(bitmapStream, of_selected);
			break;
		case 'oFsl':
		{
			message->FindInt32("type", (int32*)&out_type);
			message->FindInt32("translator", reinterpret_cast<int32*>(&out_translator));
			extern translator_id def_out_translator;
			def_out_translator = out_translator;
			savePanel->Window()->Lock();
			BView* background = savePanel->Window()->ChildAt(0);
			BTextControl* tv = dynamic_cast<BTextControl*>(background->FindView("text view"));
			if (tv) {
				char suggestion[B_FILE_NAME_LENGTH];
				strcpy(suggestion, fName);
				if (out_type != 'SBec')	 // Becasso files don't need an extension.
				// (however, we shouldn't be _exporting_ to Becasso via the TK at all...)
				{
					int il = strlen(suggestion);
					while (
						il > 0 &&
						suggestion[il] !=
							'.')  // Yes >0; .dotfiles shouldn't have their entire name cut off...
						il--;

					if (!il) {
						il = strlen(suggestion);
						suggestion[il] = '.';
					}
					suggestion[il + 1] = 0;	 // Cut off the existing extension
					const translation_format* format_list = 0;
					int32 format_count = 0;

					if (BTranslatorRoster::Default()->GetOutputFormats(
							out_translator, &format_list, &format_count) == B_NO_ERROR) {
						// printf ("%ld formats found\n", format_count);
						for (int i = 0; i < format_count; ++i) {
							// printf ("Trying format %d...\n", i);
							if (format_list[i].type == out_type) {
								// printf ("Match\n");
								BMimeType mime(format_list[i].MIME);
								BMessage extensions;
								extern bool PatronizeMIME;
								char* ext;
								if (mime.GetFileExtensions(&extensions) == B_OK &&
									extensions.FindString("extensions", 0, (const char**)&ext) ==
										B_OK)
								// Only look for the first one (if there are more)
								{
									// extensions.PrintToStream();
									// strcat (suggestion, ext);
								} else if (PatronizeMIME) {
									// extensions.PrintToStream();
									char alertstring[1024];
									sprintf(alertstring,
										lstring(150,
											"Excuse me for interrupting.  I was trying to suggest "
											"an "
											"appropriate extension for this file, "
											"yet couldn't find any in your MIME database for "
											"'%s'.\n"
											"You can add these extensions yourself with the "
											"FileTypes "
											"preferences application, "
											"or I would be happy to fill in my suggestions.  You "
											"can "
											"also simply forget about the "
											"outdated concept of extensions alltogether, although "
											"many "
											"other operating systems "
											"still rely on them for file identification."),
										format_list[i].MIME);
									BAlert* alert = new BAlert(lstring(151, "File Type"),
										alertstring, lstring(152, "Update MIME"),
										lstring(153, "Don't Patronize Me"), NULL,
										B_WIDTH_FROM_WIDEST);
									uint32 button = alert->Go();
									if (button == 1) {
										PatronizeMIME = false;
									} else	// Update the MIME database
									{
										sprintf(alertstring,
											lstring(154,
												"I have a list of common image file types and "
												"their extension but you might have "
												"some exotic types I don't know about.  In that "
												"case, I will try to make an educated "
												"guess, and if I can't think of anything, I'll "
												"simply skip that type.  I will also "
												"stay away from file types that do have any "
												"extensions set.\n"
												"You can always manually change my suggestions "
												"using the FileTypes preferences application."));
										alert = new BAlert(lstring(152, "Update MIME"), alertstring,
											lstring(131, "Cancel"), lstring(133, "Proceed"));
										button = alert->Go();
										if (button == 1) {
											// Do it...
											BMessage types;
											BMessage pmtext;
											char* type;
											int32 t = 0;

											// First, look for application/postscript as a special
											// case
											BMimeType psmt("application/postscript");
											if (psmt.GetFileExtensions(&pmtext) != B_OK ||
												!pmtext.HasString("extensions")) {
												pmtext.AddString("extensions", "eps");
												pmtext.AddString("extensions", "ps");
												psmt.SetFileExtensions(&pmtext);
											}

											BMimeType::GetInstalledTypes("image", &types);
											t = 0;
											while (types.FindString(
													   "types", t++, (const char**)&type) == B_OK) {
												// printf ("Working on %s\n", type);
												BMessage mtext;
												BMimeType mt(type);
												if (mt.InitCheck() != B_OK)
													printf("ALARM!\n");
												bool foundsomething = false;
												if (mt.GetFileExtensions(&mtext) != B_OK ||
													!mtext.HasString("extensions")) {
													char* ext = type + 6;  // skip "image/"
													if (strlen(ext) == 3)  // Easy!
													{
														// printf ("Easy: %s\n", ext);
														mtext.AddString("extensions", ext);
														foundsomething = true;
													} else if ((ext[0] == 'x') && (ext[1] == '-') &&
															   (strlen(ext + 2) == 3))	// x-foo?
													{
														// printf ("Easy: x-%s\n", ext + 2);
														mtext.AddString("extensions", ext + 2);
														foundsomething = true;
													} else {
														char sd[B_MIME_TYPE_LENGTH];
														if (mt.GetShortDescription(sd) == B_OK) {
															// Maybe we can find a hint in the short
															// description (often something like
															// "PPM Image")
															int spacep;
															for (spacep = 0; spacep < strlen(sd);
																 spacep++) {
																if (sd[spacep] == ' ')
																	break;
															}
															if (spacep == 3)  // We're lucky!
															{
																sd[3] = 0;
																for (spacep = 0; spacep < 3;
																	 spacep++)
																	sd[spacep] =
																		tolower(sd[spacep]);
																// printf ("From desc: %s\n", sd);
																mtext.AddString("extensions", sd);
																foundsomething = true;
															}
														}
													}
													if (!foundsomething)  // Well, the last resort.
													{
														if (!strcmp(ext, "jpeg")) {
															mtext.AddString("extensions", "jpg");
															mtext.AddString("extensions", "jpeg");
															foundsomething = true;
														} else if (!strcmp(ext, "targa") ||
																   !strcmp(ext, "x-targa")) {
															mtext.AddString("extensions", "tga");
															foundsomething = true;
														} else if (!strcmp(ext, "tiff")) {
															mtext.AddString("extensions", "tif");
															mtext.AddString("extensions", "tiff");
															foundsomething = true;
														}
														// Add more File Types here when we learn
														// about them...
													}
													if (foundsomething) {
														// char desc[256];
														// mt.GetShortDescription (desc);
														// printf ("Setting extensions for %s to\n",
														// desc); mtext.PrintToStream();
														mt.SetFileExtensions(&mtext);
													}
												} else {
													// else: There already are extensions set.
													// printf ("Already set\n");
													// mtext.PrintToStream();
												}
											}
										}
									}
								}
								PatronizeMIME = false;
								// Try again now...
								if (mime.GetFileExtensions(&extensions) == B_OK) {
									// printf ("Second try...\n");
									// extensions.PrintToStream();
									char* ext;
									if (extensions.FindString(
											"extensions", 0, (const char**)&ext) == B_OK)
									// Only look for the first one (if there are more)
									{
										// printf ("Adding extension %s\n", ext);
										strcat(suggestion, ext);
									}
								} else
									suggestion[il] = 0;

								break;	// I.e. don't look for further out_types.
							}
						}
					}
				}
				tv->SetText(suggestion);
			} else
				printf("Hmmm, no 'text view'\n");

			savePanel->Window()->Unlock();
			savePanel->Show();
			// savePanel->Show();	// Don't ask.
			outputFormatWindow = NULL;
			break;
		}
		case 'expt':
		{
			delete bitmapStream;
			bitmapStream = new ProgressiveBitmapStream(canvas->canvas(true));
			bitmapStream->SetDispose(true);
			extern int32 def_out_type;
			extern translator_id def_out_translator;
			out_translator = def_out_translator;
			out_type = def_out_type;
			// message->PrintToStream();
			if (message->HasString("filename"))
			// Courtesy to the end user, the 'expt' message can contain a normal path string.
			// We'll then split it up in a "directory" ref and a file name (leaf).
			{
				const char* pathname;
				message->FindString("filename", &pathname);
				BPath path;
				path.SetTo(pathname);
				if (pathname[0] == '/')	 // Filename is an absolute path
				{
					BPath dir;
					path.GetParent(&dir);
					entry_ref lref;
					get_ref_for_path(dir.Path(), &lref);
					message->AddRef("directory", &lref);
					message->AddString("name", path.Leaf());
				} else
					message->AddString("name", pathname);
			}
			if (!message->HasRef("directory"))
			// We didn't get a directory; point it to $HOME.
			{
				entry_ref lref;
				BPath home;
				find_directory(B_USER_DIRECTORY, &home);
				get_ref_for_path(home.Path(), &lref);
				message->AddRef("directory", &lref);
				// printf ("Path set to %s\n", home.Path());
			}
			// If we didn't get a file name, no sweat.  The following case block will
			// default it to the canvas name.

			scripted = true;
		}  // Fall through to B_SAVE_REQUESTED
		case B_SAVE_REQUESTED:
		{
			// savePanel->Hide();

			// synchronize all other save panels...
			// Make this a preference!
			extern entry_ref gSaveRef;
			savePanel->GetPanelDirectory(&gSaveRef);

			entry_ref lref;
			message->FindRef("directory", &lref);
			BEntry entry = BEntry(&lref);
			BString fName;
			BString percentData;
			BString title;
			BDirectory dir = BDirectory(&lref);
			if (message->FindString("name")) {	// The Save Panel sends the name separately
				fName.SetTo(message->FindString("name"));
				if (!out_type) {
					fNumberFormat.FormatPercent(percentData, (double)fScale);
					title.SetToFormat("%s (%s)", fName, percentData.String());
					SetTitle(title.String());
				}
			}
			entry.SetTo(&dir, fName, false);
			if (out_type)  // i.o.w. exporting
			{
				extern int SilentOperation;
				if (SilentOperation < 3)
					bitmapStream->DisplayProgressBar(
						BPoint(200, 200), lstring(155, "Writing Image…"));
				BFile outStream;
				if (outStream.SetTo(&entry, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE))
					printf("Error opening %s!\n", fName);
				bitmapStream->SetDispose(true);
#if defined(DATATYPES)
				if (DATATranslate(*bitmapStream, &out_info, NULL, outStream, out_type)) {
					char errstring[B_FILE_NAME_LENGTH + 64];
					sprintf(errstring, "Datatype error:\nCouldn't translate `%s'", fName);
					BAlert* alert =
						new BAlert("", string, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					alert->Go();
				}
#else
				BTranslatorRoster* roster = BTranslatorRoster::Default();
				if (roster->Translate(bitmapStream, NULL, NULL, &outStream, out_type)) {
					char errstring[B_FILE_NAME_LENGTH + 64];
					sprintf(errstring,
						lstring(134, "Translation Kit error:\nCouldn't translate `%s'"), fName);
					BAlert* alert = new BAlert(
						"", errstring, "Help", "OK", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					uint32 button = alert->Go();
					if (button == 0) {
						alert = new BAlert("",
							lstring(156,
								"This is a rare error.  Something went wrong inside the "
								"Translator.\nIt could range from shared library conflicts to "
								"running out of hard disk space."),
							lstring(136, "OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
						alert->Go();
					}
				}
#endif
				BPath path;
				entry.GetPath(&path);
				const translation_format* format_list = 0;
				int32 format_count = 0;
				if (BTranslatorRoster::Default()->GetOutputFormats(
						out_translator, &format_list, &format_count) == B_NO_ERROR) {
					for (int i = 0; i < format_count; ++i) {
						if (format_list[i].type == out_type) {
							outStream.RemoveAttr("BEOS:TYPE");
							char mime[B_FILE_NAME_LENGTH];
							strcpy(mime, format_list[i].MIME);
							mime[strlen(mime)] = 0;
							mime[strlen(mime) + 1] = 0;
							outStream.WriteAttr(
								"BEOS:TYPE", B_MIME_STRING_TYPE, 0L, mime, strlen(mime) + 1);
							break;
						}
					}
				}
				extern int DebugLevel;
				if (DebugLevel > 1)
					printf("Wrote: %s\n", path.Path());

				delete bitmapStream;
				bitmapStream = NULL;
				outputFormatWindow = NULL;
				out_type = 0;
			} else {
				canvas->Save(entry);
			}
			canvas->changed = false;
			if (scripted) {
				if (message->IsSourceWaiting()) {
					BMessage error(B_REPLY);
					error.AddInt32("error", B_NO_ERROR);
					message->SendReply(&error);
				}
			}
			scripted = false;
			entry_ref ref;
			entry.GetRef(&ref);
			add_to_recent(ref);
			if (myTarget && fromRef) {
				BMessage* ie = new BMessage(BBP_SEND_BBITMAP);
				BMessage ieb;
				ie->AddMessenger("target", this);
				ie->AddRef("ref", &ref);
				myTarget->SendMessage(ie);
			}
			break;
		}
		case 'tTed':
			canvas->tTextD();
			break;
		case 'M_S':
			float scale;
			int32 index;
			if (message->FindFloat("scale", &scale) == B_OK &&
				message->FindInt32("index", &index) == B_OK) {
				canvas->setScale(scale);
			}
			break;
		case 'Mnnn':
		{
			float s;
			if (message->FindFloat("zoom", &s))
				canvas->setScale(s);
			break;
		}
		case 'Min':
			canvas->ZoomIn();
			break;
		case 'Mout':
			canvas->ZoomOut();
			break;
		case 'mwin':
		{
			canvas->SetupUndo(M_DRAW);
			if (!magWindow) {
				BRect frame = Frame();
				frame.OffsetBy(16, 16);
				magWindow = new MagWindow(frame, FileName(), canvas);
				magWindow->Show();
			} else
				magWindow->Activate();
			break;
		}
		case 'magQ':
			if (magWindow->Lock()) {
				magWindow->Quit();
				magWindow = NULL;
			}
			break;
		case 'cmus':
			canvas->CenterMouse();
			break;
		case 'clrX':  // color changed
			canvas->InvalidateAddons();
			break;
		case 'xpal':
		{
			if (extractWindow) {
				extractWindow->Activate();
			} else {
				extractWindow = new XpalWindow(BRect(100, 100, 300, 180),
					lstring(157, "Extract Palette"), new BMessenger(this));
				extractWindow->Show();
				// extractWindow->Quit();
				// extractWindow = NULL;
			}
			break;
		}
		case 'xclF':
		{
			int32 num = 256;
			message->FindInt32("num_cols", &num);
			bool clobber = false;
			message->FindBool("clobber", &clobber);
			extern ColorMenuButton* hicolor;
			hicolor->extractPalette(canvas->currentLayer(), num, clobber);
			if (extractWindow) {
				extractWindow->Lock();
				extractWindow->Quit();
				extractWindow = NULL;
			}
			break;
		}
		case 'xclB':
		{
			int32 num = 256;
			message->FindInt32("num_cols", &num);
			bool clobber = false;
			message->FindBool("clobber", &clobber);
			extern ColorMenuButton* locolor;
			locolor->extractPalette(canvas->currentLayer(), num, clobber);
			if (extractWindow) {
				extractWindow->Lock();
				extractWindow->Quit();
				extractWindow = NULL;
			}
			break;
		}
		case 'rstf':  // MYSTERY!!!
		{
			Lock();
			BRect cf = canvas->Frame();
			BRect mf = menubar->Frame();
			//			printf ("MIN_WIDTH = %f, MAX_HEIGHT = %f\n", MIN_WIDTH, MIN_HEIGHT);
			//			printf ("canvas->Frame().Width() = %f, Height() = %f\n", cf.Width(),
			// cf.Height()); 			printf ("menubar->Frame().Height() = %f\n", mf.Height());
			// printf ("w = %f, h = %f\n", max_c (MIN_WIDTH, cf.Width() + B_V_SCROLL_BAR_WIDTH),
			// max_c
			//(MIN_HEIGHT, cf.Height() + B_H_SCROLL_BAR_HEIGHT + mf.Height()));
			ResizeTo(max_c(MIN_WIDTH, cf.Width() + B_V_SCROLL_BAR_WIDTH),
				max_c(MIN_HEIGHT, cf.Height() + B_H_SCROLL_BAR_HEIGHT + mf.Height() + 1));
			canvas->Invalidate();
			Unlock();
			break;
		}
		case 'rszT':  // Resize To
		{
			char title[1024];
			sprintf(title, "%s %s", lstring(402, "Resize"), FileName());
			ResizeWindow* rw = new ResizeWindow(
				this, title, canvas->Frame().Height() + 1, canvas->Frame().Width() + 1);
			rw->Show();
			break;
		}
		case 'rszt':
		{
			int32 w = canvas->Frame().IntegerWidth() + 1;
			int32 h = canvas->Frame().IntegerHeight() + 1;
			message->FindInt32("width", &w);
			message->FindInt32("height", &h);
			canvas->resizeTo(w, h);
			break;
		}
		case 'flpH':  // Flip Horizontal
		{
			int32 index = -1;
			message->FindInt32("layer", &index);
			canvas->flipLayer(0, index);
			break;
		}
		case 'flpV':  // Flip Vertical
		{
			int32 index = -1;
			message->FindInt32("layer", &index);
			canvas->flipLayer(1, index);
			break;
		}
		case 'layr':
			if (layerOpen) {
				//				layerWindow->Lock();
				layerWindow->Activate();
				//				windowMenu->FindItem('layr')->SetMarked (false);
				//				layerOpen = false;
			} else {
				char title[B_FILE_NAME_LENGTH + 10];
				sprintf(title, lstring(160, "Layers in %s"), fName);
				BRect layerFrame = BRect(100, 100, 310, 294);
				layerWindow = new LayerWindow(layerFrame, title, canvas);
				layerWindow->Show();
				windowMenu->FindItem('layr')->SetMarked(true);
				layerOpen = true;
			}
			break;
		case 'layQ':
			windowMenu->FindItem('layr')->SetMarked(false);
			layerOpen = false;
			break;
		case 'delL':
			// printf ("CanvasWindow::MessageReceived ('delL');\n");
			canvas->removeLayer(canvas->currentLayerIndex());
			//			setLayerMenu();
			break;
		case 'newL':
			if (LockWithTimeout(100000) == B_OK) {
				canvas->addLayer(lstring(161, "Untitled Layer"));
				Unlock();
				//				setLayerMenu();
			} else
				printf("Hey, couldn't lock!!\n");
			break;
		case 'insL':
			canvas->insertLayer(canvas->currentLayerIndex(), lstring(161, "Untitled Layer"));
			//			setLayerMenu();
			break;
		case 'trsL':
			canvas->translateLayer(canvas->currentLayerIndex());
			break;
		case 'rotL':
			canvas->rotateLayer(canvas->currentLayerIndex());
			break;
		case 'mrgL':
			canvas->mergeLayers(canvas->currentLayerIndex() - 1, canvas->currentLayerIndex());
			//			setLayerMenu();
			break;
		case 'lNch':
		{
			int32 index;
			message->FindInt32("index", &index);
			BMessage* msg = new BMessage('lNch');
			msg->AddInt32("index", index);
			canvas->MessageReceived(msg);
			delete msg;
			setLayerMenu();
			break;
		}
		case 'dupL':
			//			canvas->WriteAsHex ("logo.dat");
			canvas->duplicateLayer(canvas->currentLayerIndex());
			//			setLayerMenu();
			break;
		case 'movL':
		{
			int32 a, b;
			message->FindInt32("from", &a);
			message->FindInt32("to", &b);
			canvas->moveLayers(a, b);
			//			setLayerMenu();
			break;
		}
		case 'lChg':
			if (isLayerOpen())
				layerWindow->PostMessage(message);
			canvas->changed = true;
			setLayerMenu();
			break;
		case 'lSel':
		{
			int32 ind;
			message->FindInt32("index", &ind);
			canvas->makeCurrentLayer(ind);
			//			setLayerMenu();
			break;
		}
		case 'Loup':
		{
			int current = canvas->currentLayerIndex();
			if (current < canvas->numLayers() - 1) {
				canvas->makeCurrentLayer(current + 1);
				//				setLayerMenu();
			}
			break;
		}
		case 'Lodn':
		{
			int current = canvas->currentLayerIndex();
			if (current > 0) {
				canvas->makeCurrentLayer(current - 1);
				//				setLayerMenu();
			}
			break;
		}
		case 'DMch':
		{
			int32 index, newmode;
			message->FindInt32("index", &index);
			message->FindInt32("newmode", &newmode);
			canvas->setChannelOperation(index, newmode);
			break;
		}
		case 'Liga':
		{
			int32 index, ga;
			message->FindInt32("index", &index);
			message->FindInt32("alpha", &ga);
			canvas->setGlobalAlpha(index, ga);
			break;
		}
		case B_UNDO:
			canvas->Undo(true, true);
			break;
		case 'redo':
			canvas->Redo(true);
			break;
		case B_CUT:
			canvas->Cut();
			break;
		case B_COPY:
			canvas->Copy();
			break;
		case B_PASTE:
			if (message->WasDropped()) {
				rgb_color* dropped;
				BPoint droppoint = message->DropPoint();
				canvas->SetScale(canvas->getScale());  // ?
				droppoint = canvas->ConvertFromScreen(droppoint);
				canvas->SetScale(1);
				long dummy;
				if (message->FindData(
						"RGBColor", B_RGB_COLOR_TYPE, (const void**)&dropped, &dummy) == B_OK)
					canvas->Fill(M_DRAW, droppoint, dropped);
			} else
				canvas->Paste(false);
			break;
		case 'psNL':  // copy to new layer
			canvas->CopyToNewLayer();
			break;
		case 'psNC':  // copy to new canvas
		{
			//			canvas->Copy();
			BRect canvasWindowFrame;
			extern BBitmap* clip;
			if (clip) {
				char title[256];
				strcpy(title, fName);
				strcat(title, lstring(405, " (detail)"));
				canvasWindowFrame = clip->Bounds();
				BBitmap* newClip = new BBitmap(clip);
				canvasWindowFrame.OffsetTo(Frame().left + 16, Frame().top + 16);
				CanvasWindow* canvasWindow =
					new CanvasWindow(canvasWindowFrame, title, newClip, NULL, false);
				canvasWindow->Show();  // will register itself with the app
			}
			break;
		}
		case B_SELECT_ALL:
			canvas->SelectAll(true);
			break;
		case 'sund':
			canvas->UndoSelection(true);
			break;
		case 'sinv':
			canvas->InvertSelection(true);
			break;
		case 'ctsA':
		case 'ctsR':
		case 'ctsG':
		case 'ctsB':
		case 'ctsC':
		case 'ctsM':
		case 'ctsY':
		case 'ctsK':
		case 'ctsH':
		case 'ctsS':
		case 'ctsV':
			canvas->ChannelToSelection(message->what, true);
			break;
		case 'stcA':
		case 'stcR':
		case 'stcG':
		case 'stcB':
		case 'stcC':
		case 'stcM':
		case 'stcY':
		case 'stcK':
		case 'stcH':
		case 'stcS':
		case 'stcV':
			canvas->SelectionToChannel(message->what);
			break;
		case 'ktsA':
		{
			rgb_color c = canvas->GuessBackgroundColor();
			if (c.alpha == 0) {
				canvas->ChannelToSelection('ctsA', true);
				canvas->InvertSelection(true);
			} else
				canvas->SelectByColor(c, true);
			break;
		}
		case 'ktsF':
		{
			extern ColorMenuButton* hicolor;
			canvas->SelectByColor(hicolor->color(), true);
			break;
		}
		case 'ktsB':
		{
			extern ColorMenuButton* locolor;
			canvas->SelectByColor(locolor->color(), true);
			break;
		}
		case 'stkF':
		{
			extern ColorMenuButton* hicolor;
			canvas->ColorizeSelection(hicolor->color());
			break;
		}
		case 'stkB':
		{
			extern ColorMenuButton* locolor;
			canvas->ColorizeSelection(locolor->color());
			break;
		}
		case 'SPst':
		{
			bool v;
			message->FindBool("status", &v);
			editMenu->FindItem(B_PASTE)->SetEnabled(v);
			editMenu->FindItem('psNL')->SetEnabled(v);
			editMenu->FindItem('psNC')->SetEnabled(v);
			break;
		}
		case 'cbHd':
		{
			int32 ind;
			message->FindInt32("index", &ind);
			Layer* il = canvas->getLayer(ind);
			il->Hide(!(il->IsHidden()));
			canvas->Invalidate();
			UpdateIfNeeded();
			break;
		}
		case 'Crsd':
		{
			float width, height, scale;
			message->FindFloat("width", &width);
			message->FindFloat("height", &height);
			message->FindFloat("scale", &scale);
			CanvasResized(width, height);
			CanvasScaled(scale);
			break;
		}
		case 'PrPP':
			canvas->Print();
			break;
		case 'cwin':
			canvas->CropToWindow();
			break;
		case 'Crop':
		{
			BRect cropRect;
			// 			message->PrintToStream();
			status_t err = B_NO_ERROR;
			if (message->FindRect("data", &cropRect) == B_OK) {
				err = canvas->Crop(cropRect);
			} else
				err = -4;
			if (message->IsSourceWaiting()) {
				BMessage error(B_REPLY);
				switch (err) {
					case -1:
						error.AddString("message", "Crop Rect == Canvas Rect");
						err = 0;  // not really a problem
						break;
					case -2:
						error.AddString("message", "Crop Rect > Canvas Rect");
						break;
					case -3:
						error.AddString("message", "Crop Rect outside Canvas Rect");
						break;
					case -4:
						error.AddString("message", "No Crop Rect found in message");
						break;
				}
				error.AddInt32("error", err);
				message->SendReply(&error);
			}
			break;
		}
		case 'csel':
			canvas->CropToSelection();
			break;
		case 'caut':
			canvas->AutoCrop();
			break;
		case 'pwlt':
		case 'pwrt':
		case 'pwlb':
		case 'pwrb':
		case 'pwct':
			canvas->Pad(message->what);
			break;
		case 'rwkr':
		case 'rwar':
			canvas->ResizeToWindow(message->what);
			break;
		case 'r90':
		case 'r180':
		case 'r270':
			canvas->RotateCanvas(message->what);
			break;
		case 'adon':
		{
			extern BList* AddOns;
			AddOn* addon;
			int32 index;
			message->FindInt32("index", &index);
			//			printf ("%i\n", index);
			addon = (AddOn*)AddOns->ItemAt(index);
			canvas->OpenAddon(addon, fName);
			break;
		}
		case 'adcl':
		{
			extern BList* AddOns;
			AddOn* addon;
			int32 index;
			message->FindInt32("index", &index);
			//			printf ("Saying goodbye to AddOn %i\n", index);
			addon = (AddOn*)AddOns->ItemAt(index);
			canvas->CloseAddon(addon);
			if (message->IsSourceWaiting()) {
				//				printf ("Sending Reply...\n");
				message->SendReply(new BMessage('cack'));
			}
			break;
		}
		case 'capt':
		{
			extern BList* AddOns;
			AddOn* addon;
			int32 index;
			message->FindInt32("index", &index);
			addon = (AddOn*)AddOns->ItemAt(index);
			addon->Open(NULL, NULL);
			break;
		}
		case ADDON_PREVIEW:
		{
			extern BList* AddOns;
			AddOn* addon;
			int32 index, type;
			message->FindInt32("index", &index);
			message->FindInt32("type", &type);
			//			printf ("%i\n", index);
			addon = (AddOn*)AddOns->ItemAt(index);
			switch (type) {
				case BECASSO_FILTER:
					canvas->Filter(addon, true);
					break;
				case BECASSO_TRANSFORMER:
					canvas->Transform(addon, true);
					break;
				case BECASSO_GENERATOR:
					canvas->Generate(addon, true);
					break;
				default:
					fprintf(
						stderr, "Unknown addon requested preview: %li, type: %li\n", index, type);
			}
			break;
		}
		case ADDON_FILTER:
		{
			extern BList* AddOns;
			AddOn* addon;
			int32 index;
			message->FindInt32("index", &index);
			//			printf ("%i\n", index);
			addon = (AddOn*)AddOns->ItemAt(index);
			canvas->Filter(addon);
			break;
		}
		case ADDON_TRANSFORMER:
		{
			extern BList* AddOns;
			AddOn* addon;
			int32 index;
			message->FindInt32("index", &index);
			//			printf ("%i\n", index);
			addon = (AddOn*)AddOns->ItemAt(index);
			canvas->Transform(addon);
			break;
		}
		case ADDON_GENERATOR:
		{
			extern BList* AddOns;
			AddOn* addon;
			int32 index;
			message->FindInt32("index", &index);
			//			printf ("%i\n", index);
			addon = (AddOn*)AddOns->ItemAt(index);
			canvas->Generate(addon);
			break;
		}
		default:
			// printf ("CanvasWindow::MessageReceived:\n");
			// message->PrintToStream();
			inherited::MessageReceived(message);
			break;
	}
}

#if defined(__POWERPC__)
#pragma optimization_level 4
#endif
