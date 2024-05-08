#ifndef HELPVIEW_H
#define HELPVIEW_H

#include <Message.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>
#include "Colors.h"

#define MAX_HLPNAME 64

class HelpView : public BView {
public:
	HelpView(const BRect frame, const char* name);
	virtual void Draw(BRect updaterect);

	void setText(const char* t);

private:
	typedef BView inherited;
	char text[MAX_HLPNAME];
};

#endif