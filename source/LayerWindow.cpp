#include "LayerWindow.h"
#include "LayerView.h"
#include "CanvasView.h"
#include <Message.h>
#include <Menu.h>
#include <Window.h>
#include <MenuItem.h>
#include "Settings.h"

LayerWindow::LayerWindow(BRect frame, char* name, CanvasView* _myView)
	: BWindow(frame, name, B_DOCUMENT_WINDOW, B_WILL_ACCEPT_FIRST_CLICK)
{
	BRect menubarFrame, viewFrame, hFrame, vFrame;
	menubarFrame.Set(0, 0, 0, 0);
	menubar = new BMenuBar(menubarFrame, "LayerMenubar");
	layerM = new BMenu(lstring(170, "Layer"));
	layerM->AddItem(new BMenuItem(lstring(171, "Add New Layer"), new BMessage('newl'), 'N'));
	layerM->AddItem(new BMenuItem(lstring(172, "Insert New Layer"), new BMessage('insl'), 'I'));
	layerM->AddItem(new BMenuItem(lstring(173, "Duplicate Layer"), new BMessage('dupl'), 'D'));
	layerM->AddItem(new BMenuItem(lstring(174, "Merge Layers"), new BMessage('mrgl')));
	layerM->AddItem(new BMenuItem(lstring(175, "Remove Layer"), new BMessage('dell')));
	menubar->AddItem(layerM);
	AddChild(menubar);
	menubar->ResizeToPreferred();
	menubarFrame = menubar->Frame();
	viewFrame.Set(0, 0, LAYERITEMWIDTH, _myView->numLayers() * LAYERITEMHEIGHT);
	viewFrame.OffsetTo(menubarFrame.left, menubarFrame.bottom + 1);
	layerView = new LayerView(viewFrame, "Layer View", _myView);
	ResizeTo(viewFrame.right + B_V_SCROLL_BAR_WIDTH, viewFrame.bottom + B_H_SCROLL_BAR_HEIGHT);
	hFrame.Set(
		0, viewFrame.bottom + 1, viewFrame.right + 1, viewFrame.bottom + B_H_SCROLL_BAR_HEIGHT + 1
	);
	h = new BScrollBar(hFrame, NULL, layerView, 0, 0, B_HORIZONTAL);
	vFrame.Set(
		viewFrame.right + 1, viewFrame.top, viewFrame.right + B_V_SCROLL_BAR_WIDTH + 1,
		viewFrame.bottom + 1
	);
	v = new BScrollBar(vFrame, NULL, layerView, 0, 0, B_VERTICAL);
	AddChild(h);
	AddChild(v);
	layerView->setScrollBars(h, v);
	AddChild(layerView);
	myView = _myView;
	myWindow = _myView->Window();
	SetSizeLimits(128, Frame().Width(), 80, Frame().Height());
	if (myView->currentLayerIndex() == 0) {
		layerM->FindItem('mrgl')->SetEnabled(false);
		layerM->FindItem('dell')->SetEnabled(false);
	}
}

LayerWindow::~LayerWindow()
{
	RemoveChild(menubar);
	delete menubar;
	RemoveChild(h);
	delete h;
	RemoveChild(v);
	delete v;
	RemoveChild(layerView);
	delete layerView;
}

void
LayerWindow::doChanges(int index)
{
	if (index == -1) {
		// printf ("LayerWindow::doChanges (-1)\n");
		BRect viewFrame, menubarFrame;
		menubarFrame = menubar->Frame();
		viewFrame.Set(0, 0, LAYERITEMWIDTH, myView->numLayers() * LAYERITEMHEIGHT);
		viewFrame.OffsetTo(menubarFrame.left, menubarFrame.bottom + 1);
		RemoveChild(layerView);
		delete layerView;
		layerView = new LayerView(viewFrame, "Layer View", myView);
		SetSizeLimits(0, 1000, 0, 2000);
		ResizeTo(viewFrame.right + B_V_SCROLL_BAR_WIDTH, viewFrame.bottom + B_H_SCROLL_BAR_HEIGHT);
		SetSizeLimits(128, Frame().Width(), 80, Frame().Height());
		layerView->setScrollBars(h, v);
		AddChild(layerView);
		// printf ("Current Layer: %i\n", myView->currentLayerIndex());
		if (myView->currentLayerIndex() == 0) {
			layerM->FindItem('dell')->SetEnabled(false);
			layerM->FindItem('mrgl')->SetEnabled(false);
		}
		else {
			layerM->FindItem('dell')->SetEnabled(true);
			layerM->FindItem('mrgl')->SetEnabled(true);
		}
	}
	else {
		// printf ("Refresh %i\n", index);
		layerView->getLayerItem(index)->Refresh(myView->getLayer(index));
	}
}

void
LayerWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'draw':
		layerView->Invalidate();
		break;
	case 'layO':
		Show();
		break;
	case 'newl': {
		BMessage* message = new BMessage('newL');
		myWindow->PostMessage(message);
		delete message;
		break;
	}
	case 'dell': {
		BMessage* message = new BMessage('delL');
		myWindow->PostMessage(message);
		delete message;
		break;
	}
	case 'dupl': {
		BMessage* message = new BMessage('dupL');
		myWindow->PostMessage(message);
		delete message;
		break;
	}
	case 'insl': {
		BMessage* message = new BMessage('insL');
		myWindow->PostMessage(message);
		delete message;
		break;
	}
	case 'mrgl': {
		BMessage* message = new BMessage('mrgL');
		myWindow->PostMessage(message);
		delete message;
		break;
	}
	case 'movl': // Redundant.  The LayerItem sends the message to the window itself.
	{
		int32 from, to;
		BMessage* message = new BMessage('movL');
		msg->FindInt32("from", &from);
		msg->FindInt32("to", &to);
		message->AddInt32("from", from);
		message->AddInt32("to", to);
		myWindow->PostMessage(message);
		delete message;
		break;
	}
	case 'lChg': {
		int32 prev, current;
		if (!msg->FindInt32("prev", &prev) && !msg->FindInt32("current", &current)) {
			// We can get away by just selecting and deselecting.
			// This saves a lot of flicker - and selecting layers is the
			// most common operation anyway.
			layerView->getLayerItem(prev)->select(false);
			layerView->getLayerItem(current)->select(true);
		}
		else // ugh - rebuild the entire menu.  Bad bad bad.
		{
			BRect viewFrame, menubarFrame;
			menubarFrame = menubar->Frame();
			viewFrame.Set(0, 0, LAYERITEMWIDTH, myView->numLayers() * LAYERITEMHEIGHT);
			viewFrame.OffsetTo(menubarFrame.left, menubarFrame.bottom + 1);
			RemoveChild(layerView);
			delete layerView;
			layerView = new LayerView(viewFrame, "Layer View", myView);
			SetSizeLimits(0, 1000, 0, 2000);
			ResizeTo(
				viewFrame.right + B_V_SCROLL_BAR_WIDTH, viewFrame.bottom + B_H_SCROLL_BAR_HEIGHT
			);
			SetSizeLimits(128, Frame().Width(), 80, Frame().Height());
			layerView->setScrollBars(h, v);
			AddChild(layerView);
		}
		if (myView->currentLayerIndex() == 0)
			layerM->FindItem('dell')->SetEnabled(false);
		else
			layerM->FindItem('dell')->SetEnabled(true);
		break;
	}
	case 'lSel': {
		break;
	}
	case 'LIga': {
		int16 index = msg->FindInt32("index");
		uchar ga = uchar(msg->FindFloat("value"));
		//		BMessage *message = new BMessage ('Liga');
		//		message->AddInt32 ("index", index);
		//		message->AddInt32 ("alpha", ga);
		//		myWindow->PostMessage (message);
		//		delete message;
		//		printf ("Trying to get the lock... "); fflush (stdout);
		if (myView->Window()->LockWithTimeout(100000) == B_OK) {
			//			printf ("Got it\n");
			myView->setGlobalAlpha(index, ga);
			myView->Window()->Unlock();
		}
		//		else
		//			printf ("Failed\n");
		break;
	}
	default:
		inherited::MessageReceived(msg);
		break;
	}
}

void
LayerWindow::Quit()
{
	myWindow->PostMessage('layQ');
	inherited::Quit();
}

void
LayerWindow::WindowActivated(bool /* active */)
{
	//	if (active)
	//		doChanges (-1);
}