#include "AttribDraw.h"
#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <stdio.h>
#include <string.h>
#include "Colors.h"
#include "Settings.h"
#include "Slider.h"
#include "Tablet.h"

static property_info prop_list[] = { 0 };

AttribDraw::AttribDraw()
	: AttribView(BRect(0, 0, 164, 58), lstring(20, "Draw"))
{
	extern bool BuiltInTablet;
	SetViewColor(LightGrey);
	fModePU = new BPopUpMenu("");
	BMenuItem* item = new BMenuItem(lstring(320, "Copy"), NULL);
	item->SetMarked(true);
	fModePU->AddItem(item);
	fModePU->AddItem(new BMenuItem(lstring(321, "Over"), NULL));
	fModePU->AddItem(new BMenuItem(lstring(322, "Erase"), NULL));
	fModePU->AddItem(new BMenuItem(lstring(323, "Min"), NULL));
	fModePU->AddItem(new BMenuItem(lstring(324, "Max"), NULL));
	fModePU->AddItem(new BMenuItem(lstring(325, "Invert"), NULL));
	fModePU->AddItem(new BMenuItem(lstring(326, "Add"), NULL));
	fModePU->AddItem(new BMenuItem(lstring(327, "Subtract"), NULL));
	fModePU->AddItem(new BMenuItem(lstring(328, "Blend"), NULL));
	BMenuField* dMode
		= new BMenuField(BRect(8, 6, 156, 24), "dMode", lstring(329, "Drawing Mode:"), fModePU);
	dMode->SetDivider(82);
	drawmode[0] = B_OP_COPY;
	drawmode[1] = B_OP_OVER;
	drawmode[2] = B_OP_ERASE;
	drawmode[3] = B_OP_MIN;
	drawmode[4] = B_OP_MAX;
	drawmode[5] = B_OP_INVERT;
	drawmode[6] = B_OP_ADD;
	drawmode[7] = B_OP_SUBTRACT;
	drawmode[8] = B_OP_BLEND;
	AddChild(dMode);
	if (BuiltInTablet) {
		fTabletPU = new BPopUpMenu("");
		fTabletPU->AddItem(new BMenuItem("Serial 1", new BMessage('TBL1')));
		fTabletPU->AddItem(new BMenuItem("Serial 2", new BMessage('TBL2')));
		item = new BMenuItem("Serial 3", new BMessage('TBL3'));
		item->SetMarked(true);
		fTabletPU->AddItem(item);
		fTabletPU->AddItem(new BMenuItem("Serial 4", new BMessage('TBL4')));
		BMenuField* dTablet
			= new BMenuField(BRect(8, 30, 156, 48), "dTablet", lstring(330, "Tablet: "), fTabletPU);
		dTablet->SetDivider(82);
		AddChild(dTablet);
	}
}

AttribDraw::~AttribDraw()
{
	// printf ("Deleting Draw Window\n");
}

status_t
AttribDraw::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Draw");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribDraw::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property)
{
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribDraw::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'TBL1':
		{
			extern Tablet* wacom;
			delete wacom;
			wacom = new Tablet("serial1");
			wacom->Init();
			break;
		}
		case 'TBL2':
		{
			extern Tablet* wacom;
			delete wacom;
			wacom = new Tablet("serial2");
			wacom->Init();
			break;
		}
		case 'TBL3':
		{
			extern Tablet* wacom;
			delete wacom;
			wacom = new Tablet("serial3");
			wacom->Init();
			break;
		}
		case 'TBL4':
		{
			extern Tablet* wacom;
			delete wacom;
			wacom = new Tablet("serial4");
			wacom->Init();
			break;
		}
		default:
			inherited::MessageReceived(msg);
			break;
	}
}