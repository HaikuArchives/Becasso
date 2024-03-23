// © 1998 Sum Software

#include "BecassoAddOn.h"
#include "AddOnWindow.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <stdlib.h>

#define MAXAMOUNT 100

class IEWindow : public AddOnWindow
{
  public:
	IEWindow(BRect rect, becasso_addon_info* info) : AddOnWindow(rect, info){};
	virtual ~IEWindow(){};
	virtual void MessageReceived(BMessage* msg);

  private:
	typedef AddOnWindow inherited;
};

void
IEWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	default:
		inherited::MessageReceived(msg);
		return;
	}
	// aPreview();
}

IEWindow* window;

int
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "IE Network");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 1998 ∑ Sum Software");
	strcpy(info->description, "Bridge between Becasso and Image Elements by Attila Mezei");
	info->type = BECASSO_TRANSFORMER;
	info->index = index;
	info->version = 0;
	info->release = 2;
	info->becasso_version = 1;
	info->becasso_release = 0;
	info->does_preview = PREVIEW_FULLSCALE;
	window = new IEWindow(BRect(100, 180, 100 + 188, 180 + 116), info);
	window->Run();
	return (0);
}

int
addon_exit(void)
{
	return (0);
}

int
addon_open(BWindow* client, const char* name)
{
	char title[B_FILE_NAME_LENGTH];
	sprintf(title, "ImageElements %s", name);
	window->Lock();
	window->SetTitle(title);
	if (window->IsHidden())
		window->aShow(client);
	else
		window->aActivate(client);
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
	BRect* frame, bool /* final */, BPoint /* point */, uint32 buttons
)
{
	int error = ADDON_OK;
	//	BRect bounds = inLayer->Bounds();

	//	if (*outLayer == NULL && mode== M_DRAW)
	//		*outLayer = new Layer (*inLayer);

	//	if (mode == M_SELECT)
	//	{
	//		if (inSelection)
	//			*outSelection = new Selection (*inSelection);
	//		else	// No Selection to translate!
	//			return (error);
	//	}

	BRect oldrect = *frame;
	BRect newrect = oldrect;

	//	if (*outLayer)
	//		(*outLayer)->Lock();
	//	if (*outSelection)
	//		(*outSelection)->Lock();

	switch (mode) {
	case M_DRAW: {
		printf("Shouting!\n");
		system("hey ImageElements set IsActive of Window Untitled-1 to true");
		break;
	}
	case M_SELECT: {
		break;
	}
	default:
		fprintf(stderr, "Translate: Unknown mode\n");
		error = 1;
	}

	//	if (*outSelection)
	//		(*outSelection)->Unlock();
	//	if (*outLayer)
	//		(*outLayer)->Unlock();

	*frame = newrect;
	return (error);
}
