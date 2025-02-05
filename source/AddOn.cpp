#include "AddOn.h"
#include <Entry.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <stdio.h>
#include <stdlib.h>

static AddOnWindow* currentAddOnWindow = 0;
static AddOn* currentAddOn = 0;

AddOn::AddOn(BEntry entry)
{
	extern bool VerbAddOns;
	BPath path;
	entry.GetPath(&path);
	fAddOnID = load_add_on(path.Path());
	addon_open = 0;
	addon_make_config = 0;
	process = 0;
	if (fAddOnID < 0) {
		if (VerbAddOns)
			fprintf(stderr, "Problems loading add-on %s\n", path.Path());
		throw(0);
	}
	if (get_image_symbol(fAddOnID, "addon_init", B_SYMBOL_TYPE_TEXT, (void**)&addon_init)) {
		if (VerbAddOns)
			fprintf(stderr, "Problems finding addon_init routine in %s\n", path.Path());
		throw(1);
	}
	if (get_image_symbol(fAddOnID, "addon_exit", B_SYMBOL_TYPE_TEXT, (void**)&addon_exit)) {
		if (VerbAddOns)
			fprintf(stderr, "Problems finding addon_exit routine in %s\n", path.Path());
		throw(1);
	}
	if (get_image_symbol(fAddOnID, "process", B_SYMBOL_TYPE_TEXT, (void**)&process)) {
		// Maybe it's a capture add-on:
		if (get_image_symbol(fAddOnID, "bitmap", B_SYMBOL_TYPE_TEXT, (void**)&bitmap)) {
			if (VerbAddOns)
				fprintf(stderr, "Problems finding process/bitmap routine in %s\n", path.Path());
			throw(1);
		}
		if (get_image_symbol(fAddOnID, "addon_open", B_SYMBOL_TYPE_TEXT, (void**)&addon_open)) {
			if (VerbAddOns)
				fprintf(stderr, "Problems finding addon_open routine in %s\n", path.Path());
			throw(1);
		}
	} else if (get_image_symbol(
				   fAddOnID, "addon_make_config", B_SYMBOL_TYPE_TEXT, (void**)&addon_make_config)) {
		if (VerbAddOns)
			fprintf(stderr, "Problems finding addon_make_config routine in %s\n", path.Path());
		throw(1);
	}
	if (process
		&& get_image_symbol(fAddOnID, "addon_close", B_SYMBOL_TYPE_TEXT, (void**)&addon_close)) {
		if (VerbAddOns)
			fprintf(stderr, "Problems finding addon_close routine in %s\n", path.Path());
		throw(1);
	}

	// optional hooks
	if (get_image_symbol(
			fAddOnID, "addon_mode_changed", B_SYMBOL_TYPE_TEXT, (void**)&addon_mode_changed))
		addon_mode_changed = 0;
	else if (VerbAddOns)
		fprintf(stderr, "Found addon_mode_changed() hook in %s\n", path.Path());

	if (get_image_symbol(
			fAddOnID, "addon_color_changed", B_SYMBOL_TYPE_TEXT, (void**)&addon_color_changed))
		addon_color_changed = 0;
	else if (VerbAddOns)
		fprintf(stderr, "Found addon_color_changed() hook in %s\n", path.Path());

	if (VerbAddOns)
		fprintf(stderr, "Loaded %s\n", path.Path());
}

AddOn::~AddOn()
{
	if ((*addon_exit)()) {
		fprintf(stderr, "Warning: Add-on returned an error from addon_exit.\n");
	}
	// unload_add_on (fAddOnID);
	// Crashes on exit...  Hmm!
}

status_t
AddOn::Init(uint32 index)
{
	extern const char* Version;
	extern bool VerbAddOns;
	int res = (*addon_init)(index, &fInfo);
	if (res)
		return (res);
	if (fInfo.becasso_version > atoi(Version)) {
		fprintf(
			stderr, "%s expects a newer Becasso version (%i)\n", fInfo.name, fInfo.becasso_version);
		return (1);
	}
	if (VerbAddOns)
		fprintf(stderr, "%s version %i.%i successfully initialized.\n", fInfo.name, fInfo.version,
			fInfo.release);
	return 0;
}

void
AddOn::ColorChanged()
{
	if (addon_color_changed)
		(*addon_color_changed)();
	addon_preview();
}

void
AddOn::ModeChanged()
{
	if (addon_mode_changed)
		(*addon_mode_changed)();
	addon_preview();
}

status_t
AddOn::Open(BWindow* client, const char* /*name*/)
{
	//	printf ("AddOn::Open() [%s]\n", fInfo.name);

	currentAddOn = this;
	if (addon_open)	 // this is a capture add-on
	{
		return (*addon_open)();
	}

	BView* configView = 0;
	BRect frame(0, 0, 190, 1);
	MakeConfig(&configView, frame);
	if (configView)
		frame = configView->Bounds();
	if (frame.right < 188)
		frame.right = 188;
	frame.bottom += 64;
	frame.OffsetBy(100, 100);  // FIXME: make a setting or at least memorize
	// note: see AddOnWindow - ADDON_RESIZED

	if (!currentAddOnWindow)
		currentAddOnWindow = new AddOnWindow(frame);
	else
		currentAddOnWindow->ResizeTo(frame.Width(), frame.Height());

	currentAddOnWindow->Lock();
	BView* old_config = currentAddOnWindow->FindView("config view");
	if (old_config)
		currentAddOnWindow->Background()->RemoveChild(old_config);
	if (configView) {
		configView->SetViewColor(LightGrey);
		configView->SetName("config view");
		currentAddOnWindow->Background()->AddChild(configView);
		SetTargetOfControlsRecurse(configView, configView);
		configView->AttachedToWindow();
	}
	currentAddOnWindow->Unlock();

	currentAddOnWindow->SetAddOn(&fInfo);
	currentAddOnWindow->SetClient(client);
	currentAddOnWindow->Lock();
	if (currentAddOnWindow->IsHidden())
		currentAddOnWindow->Show();
	currentAddOnWindow->Unlock();

	return B_OK;
}

void
AddOn::SetTargetOfControlsRecurse(BView* target, BView* view)
{
	extern int DebugLevel;
	BControl* control = dynamic_cast<BControl*>(view);
	if (control) {
		if (control->Target() == currentAddOnWindow) {
			if (DebugLevel > 2)
				printf("setting target of control %s\n", control->Name());
			control->SetTarget(target);
		} else if (DebugLevel > 2)
			printf("skipping target for control %s\n", control->Name());
		return;
	}
	BMenuField* mf = dynamic_cast<BMenuField*>(view);
	if (mf) {
		if (DebugLevel > 2)
			printf("setting target for popup %s\n", mf->Name());
		BMenu* menu = mf->Menu();
		for (int i = 0; i < menu->CountItems(); i++) {
			BMenuItem* item = menu->ItemAt(i);
			if (item)
				item->SetTarget(target);
		}
	}

	for (int i = 0; i < view->CountChildren(); i++) {
		SetTargetOfControlsRecurse(target, view->ChildAt(i));
	}
}

status_t
AddOn::MakeConfig(BView** view, BRect rect)
{
	return ((*addon_make_config)(view, rect));
}

status_t
AddOn::Close(bool client_quits)
{
	//	printf ("AddOn::Close() [%s]\n", fInfo.name);

	if (currentAddOn != this) {
		fprintf(stderr, "Warning: add-on %s closes but wasn't the current?\n", fInfo.name);
	}
	currentAddOn = 0;

	if (addon_close) {
		status_t err = (*addon_close)();
		if (err)
			fprintf(stderr, "Warning: add-on %s returned 0x%" B_PRIx32 " from addon_close()\n",
				fInfo.name, err);
	}

	if (client_quits)
		currentAddOnWindow->SetClient(NULL);
	currentAddOnWindow->Hide();
	return B_OK;
}

status_t
AddOn::Process(Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection,
	int32 mode, BRect* frame, bool final, BPoint point, uint32 buttons)
{
	return ((*process)(
		inLayer, inSelection, outLayer, outSelection, mode, frame, final, point, buttons));
}

BBitmap*
AddOn::Bitmap(char* title)
{
	return ((*bitmap)(title));
}

void
addon_start(void)
{
	if (!currentAddOnWindow) {
		fprintf(stderr, "addon_start: No currentAddOnWindow\n");
		return;
	}
	currentAddOnWindow->Lock();
	currentAddOnWindow->Start();
	currentAddOnWindow->Unlock();
}

bool
addon_stop(void)
{
	if (!currentAddOnWindow) {
		fprintf(stderr, "addon_stop: No currentAddOnWindow\n");
		return true;
	}
	//	currentAddOnWindow->Lock();
	bool val = currentAddOnWindow->Stop();
	//	currentAddOnWindow->Unlock();
	return val;
}

void
addon_done(void)
{
	if (!currentAddOnWindow) {
		fprintf(stderr, "addon_stopped: No currentAddOnWindow\n");
		return;
	}
	currentAddOnWindow->Lock();
	currentAddOnWindow->Stopped();
	currentAddOnWindow->Unlock();
}

void
addon_update_statusbar(float delta, const char* text, const char* trailingText)
{
	if (!currentAddOnWindow) {
		fprintf(stderr, "addon_update_statusbar: No currentAddOnWindow\n");
		return;
	}
	currentAddOnWindow->Lock();
	currentAddOnWindow->UpdateStatusBar(delta, text, trailingText);
	currentAddOnWindow->Unlock();
}

void
addon_reset_statusbar(const char* label, const char* trailingText)
{
	if (!currentAddOnWindow) {
		fprintf(stderr, "addon_reset_statusbar: No currentAddOnWindow\n");
		return;
	}
	currentAddOnWindow->Lock();
	currentAddOnWindow->ResetStatusBar(label, trailingText);
	currentAddOnWindow->Unlock();
}

void
addon_preview(void)
{
	if (!currentAddOnWindow) {
		fprintf(stderr, "addon_preview: No currentAddOnWindow\n");
		return;
	}
	currentAddOnWindow->Lock();
	currentAddOnWindow->aPreview();
	currentAddOnWindow->Unlock();
}

void
addon_refresh_config(void)
{
	if (!currentAddOnWindow) {
		fprintf(stderr, "addon_refresh_config: No currentAddOnWindow\n");
		return;
	}
	if (!currentAddOn) {
		fprintf(stderr, "addon_refresh_config: No currentAddOn\n");
		return;
	}
	BView* configView = 0;
	BRect frame(0, 0, 190, 1);
	currentAddOn->MakeConfig(&configView, frame);
	if (configView)
		frame = configView->Bounds();
	if (frame.right < 188)
		frame.right = 188;
	frame.bottom += 64;

	currentAddOnWindow->Lock();
	currentAddOnWindow->ResizeTo(frame.Width(), frame.Height());
	BView* old_config = currentAddOnWindow->FindView("config view");
	if (old_config)
		currentAddOnWindow->Background()->RemoveChild(old_config);
	if (configView) {
		configView->SetViewColor(LightGrey);
		configView->SetName("config view");
		currentAddOnWindow->Background()->AddChild(configView);
		currentAddOn->SetTargetOfControlsRecurse(configView, configView);
	}
	currentAddOnWindow->Unlock();
}
