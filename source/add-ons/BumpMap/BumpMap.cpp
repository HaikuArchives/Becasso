// © 2000-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include "AddOnSupport.h"
#include "Slider.h"
#include <RadioButton.h>
#include <Box.h>
#include <string.h>

#define WHITE 0
#define FOREGROUND 1
#define BACKGROUND 2

typedef struct
{
	float x;
	float y;
	float z;
} v3;

inline v3
cross(const v3& a, const v3& b);
inline v3
prod(const float a, const v3& v);
inline v3
minus(const v3& a, const v3& b);
inline float
dot(const v3& a, const v3& b);
inline void
normalize(v3* a);

v3
cross(const v3& a, const v3& b)
{
	v3 res;
	res.x = a.y * b.z - a.z * b.y;
	res.y = a.x * b.z - a.z * b.x;
	res.z = a.x * b.y - a.y * b.x;
	return res;
}

v3
prod(const float a, const v3& v)
{
	v3 res;
	res.x = a * v.x;
	res.y = a * v.y;
	res.z = a * v.z;
	return res;
}

v3
minus(const v3& a, const v3& b)
{
	v3 res;
	res.x = a.x - b.x;
	res.y = a.y - b.y;
	res.z = a.z - b.z;
	return res;
}

float
dot(const v3& a, const v3& b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

void
normalize(v3* a)
{
	float r = sqrt(a->x * a->x + a->y * a->y + a->z * a->z);
	if (r) {
		a->x /= r;
		a->y /= r;
		a->z /= r;
	}
}

float gElevation;
float gAngle;
float gSpecular;
float gReflection;
float gTranslucent;
int gColorType;

class BumpView : public BView
{
  public:
	BumpView(BRect rect);

	virtual ~BumpView() {}

	virtual void MessageReceived(BMessage* msg);

  private:
};

BumpView::BumpView(BRect rect) : BView(rect, "bumpview", B_FOLLOW_ALL, B_WILL_DRAW)
{
	ResizeTo(188, 190);
	Slider* eSlid =
		new Slider(BRect(8, 8, 180, 24), 64, "Elevation", 0, 90, 1, new BMessage('bmpE'));
	Slider* aSlid = new Slider(BRect(8, 28, 180, 44), 64, "Angle", 0, 360, 1, new BMessage('bmpA'));
	Slider* sSlid =
		new Slider(BRect(8, 48, 180, 64), 64, "Specular", 0, 100, 1, new BMessage('bmpS'));
	Slider* rSlid =
		new Slider(BRect(8, 68, 180, 84), 64, "Reflection", 0, 100, 1, new BMessage('bmpR'));
	Slider* tSlid =
		new Slider(BRect(8, 88, 180, 104), 64, "Translucent", 0, 100, 1, new BMessage('bmpT'));
	AddChild(aSlid);
	AddChild(eSlid);
	AddChild(sSlid);
	AddChild(rSlid);
	AddChild(tSlid);
	aSlid->SetValue(135);
	eSlid->SetValue(45);
	sSlid->SetValue(0);
	rSlid->SetValue(0);
	tSlid->SetValue(0);
	BBox* light = new BBox(BRect(8, 110, 180, 188), "light");
	light->SetLabel("Light Color");
	AddChild(light);
	BRadioButton* rbW =
		new BRadioButton(BRect(6, 18, 170, 34), "rbW", "White", new BMessage('bmlW'));
	BRadioButton* rbF =
		new BRadioButton(BRect(6, 36, 170, 52), "rbF", "Foreground Color", new BMessage('bmlF'));
	BRadioButton* rbB =
		new BRadioButton(BRect(6, 54, 170, 70), "rbB", "Background Color", new BMessage('bmlB'));
	light->AddChild(rbW);
	light->AddChild(rbF);
	light->AddChild(rbB);
	rbW->SetValue(true);
}

void
BumpView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'bmpE':
		gElevation = msg->FindFloat("value");
		break;
	case 'bmpA':
		gAngle = msg->FindFloat("value");
		break;
	case 'bmpS':
		gSpecular = msg->FindFloat("value");
		break;
	case 'bmpR':
		gReflection = msg->FindFloat("value");
		break;
	case 'bmpT':
		gTranslucent = msg->FindFloat("value");
		break;
	case 'bmlW':
		gColorType = WHITE;
		break;
	case 'bmlF':
		gColorType = FOREGROUND;
		break;
	case 'bmlB':
		gColorType = BACKGROUND;
		break;
	default:
		BView::MessageReceived(msg);
		return;
	}
	addon_preview();
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "BumpMap");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 2000-2001 ∑ Sum Software");
	strcpy(info->description, "Uses the selection as a bump map for lighting");
	info->type = BECASSO_FILTER;
	info->index = index;
	info->version = 0;
	info->release = 2;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = PREVIEW_FULLSCALE;
	info->flags = LAYER_ONLY;
	return B_OK;
}

status_t
addon_close(void)
{
	return B_OK;
}

status_t
addon_exit(void)
{
	return B_OK;
}

status_t
addon_make_config(BView** view, BRect rect)
{
	*view = new BumpView(rect);
	return B_OK;
}

status_t
process(
	Layer* inLayer, Selection* inSelection, Layer** outLayer, Selection** outSelection, int32 mode,
	BRect* /*frame*/, bool final, BPoint /* point */, uint32 /* buttons */
)
{
	int error = ADDON_OK;
	BRect bounds = inLayer->Bounds();
	//	printf ("Bounds: ");
	//	bounds.PrintToStream();
	//	printf ("Frame:  ");
	//	frame->PrintToStream();

	if (!inSelection || mode == M_SELECT)
		// we don't bump map the bump map itself :-)
		return (0);

	if (*outLayer == NULL && mode == M_DRAW)
		*outLayer = new Layer(*inLayer);

	if (*outLayer)
		(*outLayer)->Lock();
	if (*outSelection)
		(*outSelection)->Lock();
	uint32 h = bounds.IntegerHeight() + 1;
	uint32 w = bounds.IntegerWidth() + 1;
	//	uint32 dbpr  = (*outLayer)->BytesPerRow()/4;
	//	uint32 sbpr  = inLayer->BytesPerRow()/4;

	grey_pixel* mapbits = (grey_pixel*)inSelection->Bits();
	uint32 mbpr = inSelection->BytesPerRow();
	uint32 mdiff = mbpr - w + 2;

	float angle = gAngle * M_PI / 180;
	float cangle = cos(angle);
	float sangle = sin(angle);

	float elevation = gElevation * M_PI / 180;
	//	float celevation = cos (elevation);
	float selevation = sin(elevation);

	rgb_color color;
	switch (gColorType) {
	case WHITE:
		color.red = 255;
		color.green = 255;
		color.blue = 255;
		break;

	case FOREGROUND:
		color = highcolor();
		break;

	case BACKGROUND:
		color = lowcolor();
		break;
	}

	v3 light;
	light.x = cangle;
	light.y = sangle;
	light.z = selevation;
	normalize(&light);

	double specular = gSpecular;
	double reflection = gReflection / 25;
	double translucent = gTranslucent / 25;

	if (final)
		addon_start();

	float delta = 100.0 / h; // For the Status Bar.

	switch (mode) {
	case M_DRAW: {
		bgra_pixel* sbits = (bgra_pixel*)inLayer->Bits();
		bgra_pixel* dbits = (bgra_pixel*)(*outLayer)->Bits();

		// (we skip the edges);
		mapbits += mbpr;
		mapbits++;
		for (uint32 x = 0; x < w; x++)
			*dbits++ = *sbits++;

		dbits++;
		sbits++;
		for (uint32 y = 1; y < h - 1; y++) {
			if (final) {
				addon_update_statusbar(delta);
				if (addon_stop()) {
					error = ADDON_ABORT;
					break;
				}
			}
			for (uint32 x = 1; x < w - 1; x++) {
				// Determine the normal vector on the bump map
				float b11, b12, b13, b21, b22, b23, b31, b32, b33;
				b11 = float(*(mapbits - mbpr - 1)) / 255;
				b12 = float(*(mapbits - mbpr)) / 255;
				b13 = float(*(mapbits - mbpr + 1)) / 255;
				b21 = float(*(mapbits - 1)) / 255;
				b22 = float(*(mapbits)) / 255;
				b23 = float(*(mapbits + 1)) / 255;
				b31 = float(*(mapbits + mbpr - 1)) / 255;
				b32 = float(*(mapbits + mbpr)) / 255;
				b33 = float(*(mapbits + mbpr + 1)) / 255;
				v3 a, b, z;
				a.x = 1;
				a.y = 0;
				a.z = (((b13 + b33) / 2 + b23) - ((b11 + b31) / 2 + b21)) / 2;
				b.x = 0;
				b.y = 1;
				b.z = (((b31 + b33) / 2 + b32) - ((b11 + b13) / 2 + b12)) / 2;
				z.x = 0;
				z.y = 0;
				z.z = 1;
				v3 n = cross(a, b);
				normalize(&n);
				float amount = dot(light, n);

				// calculate reflection vector
				v3 r = minus(prod(2 * dot(n, light), n), light);
				double spec = dot(r, z);
				double aspec = fabs(spec);
				double samount_tran =
					(specular == 0) ? 0 : pow(aspec, (101 - specular) / 4) * translucent;
				double samount_spec =
					(specular == 0 || spec < 0) ? 0 : pow(spec, (101 - specular) / 4) * reflection;
				double samount = samount_tran + samount_spec;

				// printf ("(%ld, %ld) %f, %f\n", x, y, spec, samount);

				float sr = samount * color.red;
				float sg = samount * color.green;
				float sb = samount * color.blue;

				float ramount = 1 + (amount > 0 ? amount * color.red / 255 : amount);
				float gamount = 1 + (amount > 0 ? amount * color.green / 255 : amount);
				float bamount = 1 + (amount > 0 ? amount * color.blue / 255 : amount);
				bgra_pixel pixel = *sbits++;
				*dbits++ = PIXEL(
					clipchar(ramount * RED(pixel) + sr), clipchar(gamount * GREEN(pixel) + sg),
					clipchar(bamount * BLUE(pixel) + sb), ALPHA(pixel)
				);
				mapbits++;
			}
			*dbits++ = *sbits++;
			*dbits++ = *sbits++;
			mapbits += mdiff;
		}
		for (uint32 x = 1; x < w; x++)
			*dbits++ = *sbits++;
		break;
	}
	case M_SELECT: {
		break;
	}
	default:
		fprintf(stderr, "Bumpmap: Unknown mode\n");
		error = ADDON_UNKNOWN;
	}

	if (*outSelection)
		(*outSelection)->Unlock();
	if (*outLayer)
		(*outLayer)->Unlock();

	if (final)
		addon_done();
	return (error);
}
