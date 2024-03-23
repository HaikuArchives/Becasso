#ifndef PICITEM_H
#define PICITEM_H

#include <MenuItem.h>
#include <Picture.h>
#include <Window.h>
#include "HelpView.h"
#include "AttribWindow.h"
#include "AttribView.h"

class PicItem : public BMenuItem
{
  public:
	PicItem(
		const BPicture* _picture, AttribView* _attrib, AttribWindow* _myWindow, const char* _help
	);
	virtual ~PicItem();

	BPicture* getPicture() { return picture; };

	AttribView* getAttrib() { return attrib; };

	const char* helptext() { return help; };

	AttribWindow* getMyWindow() { return myWindow; };

	virtual void Draw();


  protected:
	virtual void GetContentSize(float* width, float* height);

  private:
	typedef BMenuItem inherited;
	char* help;
	BPicture* picture;
	AttribView* attrib;
	AttribWindow* myWindow;
};

#endif