#include "PatternItem.h"
#include "PatternMenuButton.h"
#include "ColorMenuButton.h"
#include "Colors.h"

PatternItem::PatternItem(const pattern _p) : BMenuItem("", NULL) { pat = _p; }

PatternItem::~PatternItem() {}

pattern
PatternItem::getPattern()
{
	return (pat);
}

void
PatternItem::setPattern(pattern _p)
{
	pat = _p;
}

void
PatternItem::Draw()
{
	extern ColorMenuButton *locolor, *hicolor;
	BRect frame = Frame();
	frame.right--;
	frame.bottom--;
	rgb_color oldLo = Menu()->LowColor();
	Menu()->SetLowColor(locolor->color());
	Menu()->SetHighColor(hicolor->color());
	Menu()->FillRect(frame, pat);
	rgb_color oldHi = Menu()->HighColor();
	frame.right++;
	frame.bottom++;
	Menu()->SetHighColor(Black);
	Menu()->StrokeRect(frame);
	Menu()->SetHighColor(oldHi);
	if (IsSelected() || IsMarked()) {
		frame.InsetBy(1, 1);
		oldHi = Menu()->HighColor();
		Menu()->SetHighColor(White);
		Menu()->StrokeRect(frame);
		frame.InsetBy(1, 1);
		Menu()->SetHighColor(Black);
		Menu()->StrokeRect(frame);
		Menu()->SetHighColor(oldHi);
	}
	Menu()->SetLowColor(oldLo);
}

void
PatternItem::GetContentSize(float* width, float* height)
{
	*width = 0;
	*height = P_SIZE;
}