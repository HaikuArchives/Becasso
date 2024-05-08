#include "PicItem.h"
#include "Colors.h"
#include "MainWindow.h"	 // Hack alert!

PicItem::PicItem(
	const BPicture* _picture, AttribView* _attrib, AttribWindow* _myWindow, const char* _help)
	: BMenuItem("", NULL)
{
	picture = new BPicture(*_picture);
	attrib = _attrib;
	myWindow = _myWindow;
	help = new char[strlen(_help) + 1];
	strcpy(help, _help);
}

PicItem::~PicItem()
{
	delete[] help;
	delete picture;
}

void
PicItem::Draw()
{
	rgb_color oldHi = Menu()->HighColor();
	if (IsSelected()) {
		Menu()->SetHighColor(Grey19);
		Menu()->FillRect(Frame());
	}
	if (IsMarked()) {
		Menu()->SetHighColor(Black);
		Menu()->StrokeRect(Frame());
	}
	Menu()->SetHighColor(oldHi);
	Menu()->DrawPicture(picture, Frame().LeftTop());
}

void
PicItem::GetContentSize(float* width, float* height)
{
	// *** Hack!
	*width = 0;
	*height = BUTTON_SIZE;
}