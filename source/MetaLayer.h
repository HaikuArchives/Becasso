#include "Layer.h"

#define BITMAP_LAYER	0
#define TEXT_LAYER		1

class MetaLayer : public Layer
{
public:
			 MetaLayer (BRect bounds, const char *name, int type = BITMAP_LAYER);
virtual		~MetaLayer ();

private:
int			 fType;
};