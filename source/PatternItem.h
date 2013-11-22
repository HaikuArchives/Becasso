#ifndef PATTERNITEM_H
#define PATTERNITEM_H

#include <MenuItem.h>

class PatternItem : public BMenuItem
{
public:
			 PatternItem (const pattern _p);
virtual		~PatternItem ();
pattern		 getPattern ();
void		 setPattern (pattern _p);

virtual void Draw ();
protected:
virtual void GetContentSize (float *width, float *height);

private:
typedef BMenuItem inherited;
pattern		 pat;
};

#endif 