#include "Datatypes.h"
#include <DataFormats.h>

class MyWindow : public BWindow
{
  public:
	MyWindow();
	~MyWindow();

	bool QuitRequested();
	static status_t RecordThread(void* data);
	void MessageReceived(BMessage* message);

	DATAID fHandler;
	thread_id fRecordThread;
	BView* fCapture;
};

class TestApplication : public BApplication
{
  public:
	TestApplication();
	void preRun(void);
	MyWindow* mw;
};

void
main()
{
	TestApplication* app;
	app = new TestApplication();
	app->preRun();
	app->Run();
	DATAShutdown();
	delete (app);
}

const char* SIGNATURE = "application/x-redrackam-testCaptureHandler";

TestApplication::TestApplication() : BApplication(SIGNATURE) {}

class BitmapView : public BView
{
  public:
	BitmapView(BRect a);
	void Draw(BRect area);
	void Decode(BPositionIO* stream);
	void MouseDown(BPoint point);

  private:
	BBitmap* bitmap;
};

BitmapView ::BitmapView(BRect a) : BView(a, "BitmapInput", B_FOLLOW_NONE, B_WILL_DRAW) {}

void
BitmapView ::Draw(BRect area)
{
	// Si la view est en train d'imprimer
	//  Alors on dessine la bitmap au centre, la plus grande possible
	if (IsPrinting() == true) {
		// Calcul de la taille de l'image en pixels (12 x 9 cm)
		float w_pixel = (12.0 / 2.54) * 72.0;
		float h_pixel = (9.0 / 2.54) * 72.0;
		BRect rect(0.0, 0.0, w_pixel - 1.0, h_pixel - 1.0);

		// Centrer l'image
		rect.OffsetTo(
			0.5 * (Bounds().Width() - rect.Width()), 0.5 * (Bounds().Height() - rect.Height())
		);

		// dessine l'image
		DrawBitmap(bitmap, rect);
	} else {
		DrawBitmap(bitmap, BPoint(0, 0));
	}
}

void
BitmapView::MouseDown(BPoint point)
{
	BMenuItem* selected;
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		BMessage* msg;
		BMenuItem* item;
		BPopUpMenu* menu = new BPopUpMenu("Print");
		msg = new BMessage('prnt');
		item = new BMenuItem("print", msg);
		menu->AddItem(item);
		ConvertToScreen(&point);
		selected = menu->Go(point);
		if (selected) {
			// POUR MATHIAS:: METTRE ICI TON PRINT
			// LA VIEW A DESSINER, c'est THIS
			// TU PEUX MEME ENVOYER LE MESSAGE GENERE PAR LE BOUTON DIRECTEMENT AU PRINT SERVEUR
			// (avec un SetTarget et en completant le message pour qu'il contienne ce qu'il faut)

			// Quelques variables utiles
			BMessage* setup = NULL; // normalement, c'est le message de configuration
			status_t err;

			// On cree un print job avec un nom pour le spool
			// Dans tous les cas l'unicite du nom est assure
			// par le print server
			BPrintJob job("document");

			// D'abord parametrer le format de la page
			// Normalement, ca se fait sur demande de l'utilisateur
			job.ConfigPage();

			// Si on a une conf par defaut (par ex. sauvee avec le document)
			// Alors il faut la passer au print server
			if (setup)
				job.SetSettings(new BMessage(setup));

			// Dans tous les cas, on affiche la page de reglage
			if ((err = job.ConfigJob()) == B_OK) {
				if (setup != NULL)
					delete setup;
				// On recupere les choix de l'utilisateur
				setup = job.Settings();
			}

			// Ouvrir le fichier de spool
			job.BeginJob();

			// Dessiner la page
			BRect rect = job.PrintableRect();
			BPoint pointOnPage(B_ORIGIN);
			job.DrawView(this, rect, pointOnPage);

			// Envoyer la page
			job.SpoolPage();

			// On a fini
			job.CommitJob();
		}
	}
}

void
BitmapView::Decode(BPositionIO* stream)
{
	char* bits;
	DATABitmap header;
	stream->Seek(0, SEEK_SET);
	stream->Read(&header, sizeof(header));
	this->ResizeTo(header.bounds.Width(), header.bounds.Height());
	Window()->ResizeTo(header.bounds.Width(), header.bounds.Height());
	if (header.magic != DATA_BITMAP) {
		return;
	}


	bitmap = new BBitmap(header.bounds, header.colors, true);
	bits = (char*)bitmap->Bits();
	//	bits=(char *)malloc(header.dataSize);;
	//	printf("length (bitmap)=%lx header=%lx\n",bitmap->BitsLength(),header.dataSize);
	stream->Read(bits, header.dataSize);
	// view->OffsetTo(B_ORIGIN);
	//	bitmap->SetBits(bits,header.dataSize,0,header.colors);
	//	free(bits);
	//	view->AddChild(bitmap);
}

void
TestApplication::preRun(void)
{
	status_t res;
	DATAInit("application/x-redrakam-CaptureTestHandler");
	mw = new MyWindow;
	mw->Show();
	BitmapView* view;
	BMallocIO stream;
	BMessage message;
	do {
		res = DATACapture(
			mw->fHandler,
			mw->fCapture, //	can be NULL
			&message,	  //	can be NULL
			stream, DATA_BITMAP
		);
		if (mw) {
			mw->Lock();
			BRect area = mw->fCapture->Frame();
			mw->Unlock();
			view = new BitmapView(area);
			area.OffsetTo(100, 100);
			BWindow* new_bw =
				new BWindow(area, "Capture", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE);

			new_bw->AddChild(view);
			view->Decode(&stream);
			new_bw->Show();
			DATAGetConfigMessage(mw->fHandler, &message);
			//	message.PrintToStream();
		}
	} while (res == B_OK);
}

MyWindow::~MyWindow() { be_app->PostMessage(B_QUIT_REQUESTED); }

MyWindow::MyWindow()
	: BWindow(
		  BRect(200, 200, 400, 400), "Capture", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
	  )
{
	DATAID* list = NULL;
	int32 count = 0;
	int32 ix;
	BRect x;
	if (DATAListCaptures(DATA_BITMAP, list, count) || (count < 1)) {
		BAlert* alrt =
			new BAlert("Not!", "There are no bitmap capture datatypes installed.", "Quit");
		alrt->Go();
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	BMessage* new_message = new BMessage('test');

	for (ix = 0; (ix < count) && DATAMakeCapturePanel(list[ix], new_message, fCapture, x); ix++) {
		fCapture = NULL;
	}
	if (!fCapture) {
		BAlert* alrt =
			new BAlert("Not!", "None of the installed btmap capture datatypes liked me.", "Quit");
		alrt->Go();
		PostMessage(B_QUIT_REQUESTED);
		delete[] list;
		return;
	} else {
		char *name, *info;
		int32 version;
		fHandler = list[ix];
		DATAGetHandlerInfo(fHandler, name, info, version);
		//	printf("name:%s  info %s version %ld\n",name,info,version);
		DATAGetConfigMessage(fHandler, new_message);
		//	new_message->PrintToStream();
	}

	delete[] list;
	ResizeTo(x.Width(), x.Height());
	x.top = x.bottom + 3;
	x.bottom = x.top + 20;
	x.left += 10;
	x.right -= 10;
	AddChild(fCapture);
}

void
MyWindow::MessageReceived(BMessage* message)
{
	char a[5];
	*(long*)a = message->what;
	// printf("message recu %s\n",a);
	inherited::MessageReceived(message);
}

bool
MyWindow::QuitRequested()
{
	((TestApplication*)be_app)->mw = NULL;
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
