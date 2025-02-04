#include "ColorItem.h"
#include "ColorMenuButton.h"
#include "Colors.h"

ColorItem::ColorItem(const rgb_color _color)
	: BMenuItem("", NULL)
{
	color = _color;
}

ColorItem::~ColorItem() {}

rgb_color
ColorItem::getColor()
{
	//	printf ("r = %i, g = %i, b = %i\n", color.red, color.green, color.blue);
	return (color);
}

void
ColorItem::setColor(rgb_color _color)
{
	color = _color;
}

void
ColorItem::Draw()
{
	BRect frame = Frame();
	frame.right -= 1;
	frame.bottom -= 1;
	rgb_color oldLo = Menu()->LowColor();
	Menu()->SetLowColor(color);
	if (IsSelected() || IsMarked()) {
		rgb_color oldHi = Menu()->HighColor();
		Menu()->SetHighColor(White);
		Menu()->StrokeRect(frame);
		frame.InsetBy(1, 1);
		Menu()->SetHighColor(Black);
		Menu()->StrokeRect(frame);
		Menu()->SetHighColor(oldHi);
		frame.InsetBy(1, 1);
		Menu()->FillRect(frame, B_SOLID_LOW);
	} else
		Menu()->FillRect(frame, B_SOLID_LOW);

	Menu()->SetLowColor(oldLo);
}

void
ColorItem::GetContentSize(float* width, float* height)
{
	*width = 0;
	*height = C_SIZE;
}