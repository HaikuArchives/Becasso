#ifndef COLORITEM_H
#define COLORITEM_H

#include <MenuItem.h>

class ColorItem : public BMenuItem
{
public:
			 ColorItem (const rgb_color _color);
virtual		~ColorItem ();
rgb_color	 getColor ();
void		 setColor (rgb_color _color);

virtual void Draw ();
protected:
virtual void GetContentSize (float *width, float *height);

private:
typedef BMenuItem inherited;
rgb_color	 color;
};

#endif 