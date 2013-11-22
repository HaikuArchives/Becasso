#include <TextControl.h>
#include <Box.h>
#include <Rect.h>
#include <View.h>
#include <Button.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <CheckBox.h>
#include <StringView.h>
#include "PrefsWindow.h"
#include "Colors.h"
#include <stdio.h>
#include "Settings.h"
#include <Path.h>
#include <Entry.h>
#include <Directory.h>
#include <Application.h>
#include <Roster.h>
#include "Slider.h"

extern becasso_settings g_settings;
extern BLocker g_settings_lock;

PrefsWindow::PrefsWindow ()
: BWindow (BRect (100, 100, 380, 308), lstring (380, "Preferences"), B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	BRect bgFrame, revertFrame, applyFrame, okFrame;
	bgFrame = Bounds();
	BView *bg = new BView (bgFrame, "SW bg", B_FOLLOW_ALL, B_WILL_DRAW);
	bg->SetViewColor (LightGrey);
	AddChild (bg);

	g_settings_lock.Lock();
	fLocalSettings = g_settings;
	fBackup = g_settings;
	g_settings_lock.Unlock();

	char cur[16];
	sprintf (cur, "%ld", fLocalSettings.recents);
	fNumEntriesTC = new BTextControl (BRect (8, 8, 272, 32), "recent", lstring (383, "Number of entries in Recent menu:"), cur, new BMessage ('prNR'));
	fNumEntriesTC->SetDivider (230);
	bg->AddChild (fNumEntriesTC);

	fLangPU = new BPopUpMenu ("Language");
	app_info info;
	be_app->GetAppInfo (&info);
	BEntry appEntry = BEntry (&info.ref);
	BEntry appDir;
	appEntry.GetParent (&appDir);
	BPath appDirPath (&appDir);
	appDirPath.Append ("data/strings");
	BDirectory dir (appDirPath.Path());
	BEntry entry;
	while (dir.GetNextEntry (&entry, false) == B_OK)
	{
		char name[B_FILE_NAME_LENGTH];
		entry.GetName (name);
		BMenuItem *item = new BMenuItem (name, new BMessage ('prLC'));
		if (!strcmp (name, fLocalSettings.language))
			item->SetMarked (true);
		fLangPU->AddItem (item);
	}
	BMenuField *langMF = new BMenuField (BRect (8, 34, 272, 54), "lan", lstring (384, "Interface language: "), fLangPU);
	bg->AddChild (langMF);
	BStringView *lanWarn = new BStringView (BRect (8, 54, 276, 72), "lanwarn", lstring (385, "(Will take effect at next program launch)"));
	bg->AddChild (lanWarn);

	fUndoSlider = new Slider (BRect (10, 80, 272, 96), 150, lstring (331, "Maximum # Undos"), 1, MAX_UNDO - 1, 1, new BMessage ('prMU'), B_HORIZONTAL, 16);
	fUndoSlider->SetValue (fLocalSettings.max_undo);
	bg->AddChild (fUndoSlider);
	
	fSelectionCB = new BCheckBox (BRect (10, 98, 272, 114), "selection", lstring (435, "Always invert selection"), new BMessage ('selI'));
	fSelectionCB->SetValue (fLocalSettings.selection_type == SELECTION_STATIC);
	bg->AddChild (fSelectionCB);

	BCheckBox *totdCB = new BCheckBox (BRect (10, 118, 272, 134), "totd", lstring (387, "Show tips at startup"), new BMessage ('totd'));
	totdCB->SetValue (fLocalSettings.totd);
	bg->AddChild (totdCB);

	fPrevSizePU = new BPopUpMenu ("Preview Size");
	BMenuItem *item = new BMenuItem ("64x64", new BMessage ('p064'));
	if (fLocalSettings.preview_size == 64)
		item->SetMarked (true);
	fPrevSizePU->AddItem (item);
	item = new BMenuItem ("128x128", new BMessage ('p128'));
	if (fLocalSettings.preview_size == 128)
		item->SetMarked (true);
	fPrevSizePU->AddItem(item);
	item = new BMenuItem ("256x256", new BMessage ('p256'));
	if (fLocalSettings.preview_size == 256)
		item->SetMarked (true);
	fPrevSizePU->AddItem(item);
	BMenuField *previewMF = new BMenuField (BRect (8, 138, 272, 158), "prv", lstring (386, "Filter Preview Size: "), fPrevSizePU);
	bg->AddChild (previewMF);

	okFrame.Set (bgFrame.right - 78, bgFrame.bottom - 32, bgFrame.right - 8, bgFrame.bottom - 8);
	applyFrame.Set (okFrame.left - 78, okFrame.top, okFrame.left - 8, okFrame.bottom);
	revertFrame.Set (applyFrame.left - 78, okFrame.top, applyFrame.left - 8, okFrame.bottom);
	BButton *revert = new BButton (revertFrame, "revert", lstring (381, "Revert"), new BMessage ('prfR'));
	BButton *apply = new BButton (applyFrame, "apply", lstring (382, "Apply"), new BMessage ('prfA')); 
	BButton *ok = new BButton (okFrame, "ok", lstring (136, "OK"), new BMessage ('prfK'));
	ok->MakeDefault (true);
	bg->AddChild (revert);
	bg->AddChild (apply);
	bg->AddChild (ok);
}

PrefsWindow::~PrefsWindow ()
{
}

void PrefsWindow::refresh()
{
	char cur[16];
	sprintf (cur, "%li", fLocalSettings.recents);
	fNumEntriesTC->SetText (cur);
	fLangPU->FindItem(fLocalSettings.language)->SetMarked (true);
	fUndoSlider->SetValue (fLocalSettings.max_undo);
	fSelectionCB->SetValue (fLocalSettings.selection_type == SELECTION_STATIC);
}

void PrefsWindow::MessageReceived (BMessage *message)
{
	switch (message->what)
	{
	case 'prMU':
	{
		fLocalSettings.max_undo = int (message->FindFloat ("value"));
		break;
	}
	case 'prNR':
	{
		fLocalSettings.recents = atoi (fNumEntriesTC->Text());
		fLocalSettings.settings_touched = true;
		break;
	}
	case 'prLC':
	{
		strcpy (fLocalSettings.language, fLangPU->FindMarked()->Label());
		break;
	}
	case 'selI':
		fLocalSettings.selection_type = fSelectionCB->Value() ? SELECTION_STATIC : SELECTION_IN_OUT;
		break;
	case 'totd':
		fLocalSettings.totd = message->FindInt32 ("be:value");
		break;
	case 'p064':
		fLocalSettings.preview_size = 64;
		break;
	case 'p128':
		fLocalSettings.preview_size = 128;
		break;
	case 'p256':
		fLocalSettings.preview_size = 256;
		break;
		
	case 'prfR':
		g_settings_lock.Lock();
		fLocalSettings = fBackup;
		g_settings = fLocalSettings;
		g_settings_lock.Unlock();
		refresh();
		break;
	case 'prfA':
		g_settings_lock.Lock();
		g_settings = fLocalSettings;
		g_settings_lock.Unlock();
		break;
	case 'prfK':
		g_settings_lock.Lock();
		g_settings = fLocalSettings;
		g_settings_lock.Unlock();
		Quit();
		break;
	default:
		inherited::MessageReceived(message);
		break;
	}
}
