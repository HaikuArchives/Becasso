/*
 *  OutputFormatWindow.cpp
 *
 *  Release 3.1.1 (May. 21st, 1998)
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 */

// Modifications by Sander Stoks marked --SS

// Includes
#include <string.h>
#include <stdio.h>
#include <View.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Font.h>
#include <Button.h>
#include <Rect.h>
#include <Errors.h>
#include <TranslationDefs.h>
#include <TranslatorRoster.h>
#include <DataIO.h>
#include <Window.h>
#include <Invoker.h>
#include <translation/TranslatorFormats.h>
#include "OutputFormatWindow.h"


// Defines
#define DO_SETUP 'dosu'
#define MESSAGE_SENT 'msgs'
#define SEND_MESSAGE 'smsg'
#define ITEM_SELECTED 'itsl'
#define INVOKE_LIST 'invk'

// Struct for keeping output format info together
struct OutputFormat {
	uint32 type;
	translator_id translator;
};

// CaptionView class for displaying a caption with a separator line
class CaptionView : public BView {
  public:
	CaptionView(BRect frame, const char* caption);
	virtual ~CaptionView();
	virtual void Draw(BRect updateRect);

  private:
	char* mCaption;
	float text_height, text_descent;
};

// View for displaying the three info lines
class InfoLinesView : public BView {
  public:
	InfoLinesView(BRect frame, int32 nLines);
	virtual ~InfoLinesView();
	void SetInfoLines(const char** LineTexts);
	virtual void Draw(BRect updateRect);

  private:
	int32 mLines;
	const char** mLineTexts;
	float text_height, text_descent;
};

// The main view
class OutputFormatView : public BView {
  public:
	OutputFormatView(
		BRect frame, BPositionIO* stream, BInvoker* item_selected, BTranslatorRoster* roster
	);
	virtual ~OutputFormatView();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);

  private:
	void AddConfigView();
	void MakeFollowBottom(bool flag = true);
	void RecursiveSetMode(BView* view);
	BInvoker* selected_invoker;
	bool message_sent;			   // --SS
	BTranslatorRoster* the_roster; // switched to avoid compiler warning
	BListView* list_view;
	BScrollView* scroll_view;
	BButton* cancel_button;
	BButton* the_button;
	CaptionView* info_caption;
	InfoLinesView* info_view;
	OutputFormat* output_list;	 // -SS
	CaptionView* config_caption; // switched
	BView* config_view;			 // --SS
	BWindow* the_window;		 // switched
	int32 index;
	const char* name_line;
	const char* info_line;
	char version_line[40];
};

// OutputFormatWindow constructor
OutputFormatWindow::OutputFormatWindow(
	BPositionIO* in_stream, BInvoker* item_selected, BInvoker* window_cancelled,
	BTranslatorRoster* roster
)

	: BWindow(BRect(100.0, 45.0, 425.0, 322.0), "Select Format", B_TITLED_WINDOW, 0),
	  the_stream(in_stream), selected_invoker(item_selected), cancelled_invoker(window_cancelled),
	  the_roster(roster), message_sent(false), setup_done(false)
{
	// Don't do anything time consuming in the contructor,
	// let's get the message looper thread going first
	Run();
	PostMessage(DO_SETUP);
}

// Handle messages sent to the window
void
OutputFormatWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case DO_SETUP:
		if (!setup_done) {
			if (the_roster == 0)
				the_roster = BTranslatorRoster::Default();

			OutputFormatView* main_view =
				new OutputFormatView(Bounds(), the_stream, selected_invoker, the_roster);
			ResizeTo(main_view->Bounds().Width(), main_view->Bounds().Height());
			Lock();
			AddChild(main_view);
			Unlock();
			Show();
			setup_done = true;
		}
		break;

	case MESSAGE_SENT:
		message_sent = true;
		PostMessage(B_QUIT_REQUESTED);
		break;

	default:
		BWindow::MessageReceived(msg);
		break;
	}
}

OutputFormatWindow::~OutputFormatWindow()
{
	//	delete the_stream;
}

// Quit the window
bool
OutputFormatWindow::QuitRequested()
{
	// Only send a cancelled message if no message was sent
	// and there is an invoker
	if (!message_sent && cancelled_invoker)
		cancelled_invoker->Invoke();
	return true;
}

// Main view constructor
OutputFormatView::OutputFormatView(
	BRect frame, BPositionIO* stream, BInvoker* item_selected, BTranslatorRoster* roster
)
	: BView(frame, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW), selected_invoker(item_selected),
	  message_sent(false), the_roster(roster), output_list(0), config_caption(0), config_view(0)
{
	index = -1;
	version_line[0] = '\0';
	name_line = info_line = version_line;

	// Set the background colour to gray
	SetViewColor(222, 222, 222);
	SetLowColor(222, 222, 222);

	// Top caption
	CaptionView* caption_view =
		new CaptionView(BRect(8.0, 0.0, Bounds().right - 8.0, 0.0), "Output Formats");
	AddChild(caption_view);

	// Create the main list view; we'll put it in a scroll view later
	list_view = new BListView(
		BRect(
			10.0, caption_view->Frame().bottom + 3.0, Bounds().right - 10.0 - B_V_SCROLL_BAR_WIDTH,
			caption_view->Frame().bottom + 108.0
		),
		""
	);
	list_view->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM);
	list_view->SetSelectionMessage(new BMessage(ITEM_SELECTED));
	list_view->SetInvocationMessage(new BMessage(SEND_MESSAGE));

	// Identify type and group of incoming stream
	translator_info in_info;
	if (the_roster->Identify(stream, 0, &in_info, B_TRANSLATOR_BITMAP) == B_NO_ERROR) {
		uint32 in_type = in_info.type;
		uint32 in_group = in_info.group;
		//		printf ("in_type %c%c%c%c, in_group %c%c%c%c\n",
		//			((char *) &in_type)[0], ((char *) &in_type)[1], ((char *) &in_type)[2], ((char
		//*) &in_type)[3],
		//			((char *) &in_group)[0], ((char *) &in_group)[1], ((char *) &in_group)[2],
		//((char *) &in_group)[3]);

		translator_info* info_list = 0;
		int32 info_count = 0;
		if (the_roster->GetTranslators(stream, 0, &info_list, &info_count) == B_OK) {
			// One format per translator is a good starting point,
			// we'll dynamically resize if necessary
			int32 max_formats = info_count;
			output_list = new OutputFormat[max_formats];
			int32 output_count = 0;
			//			printf ("%ld translators found\n", info_count);	// --SS
			for (int32 i = 0; i < info_count; ++i) {
				const translation_format* format_list = 0; // --SS const
				int32 format_count = 0;
				if (the_roster->GetOutputFormats(
						info_list[i].translator, &format_list, &format_count
					) == B_NO_ERROR) {
					for (int32 j = 0; j < format_count; ++j) {
						// Only list formats that differ from the input format
						// and belong to the same group.
						if (format_list[j].type != in_type && format_list[j].group == in_group) {
							if (output_count == max_formats) {
								max_formats += 10;
								OutputFormat* new_list = new OutputFormat[max_formats];
								for (int32 k = 0; k < output_count; ++k)
									new_list[k] = output_list[k];

								delete[] output_list;
								output_list = new_list;
							}
							output_list[output_count].translator = info_list[i].translator;
							output_list[output_count].type = format_list[j].type;

							// Put the format names in the list view
							list_view->AddItem(new BStringItem(format_list[j].name));
							++output_count;
						}
						//						else
						//		printf ("rejected in_type %c%c%c%c, in_group %c%c%c%c\n",
						//			((char *) &format_list[j].type)[0], ((char *)
						//&format_list[j].type)[1], ((char *) &format_list[j].type)[2], ((char *)
						//&format_list[j].type)[3],
						//			((char *) &format_list[j].group)[0], ((char *)
						//&format_list[j].group)[1], ((char *) &format_list[j].group)[2], ((char *)
						//&format_list[j].group)[3]);
					}
				}
			}
		}
		delete[] info_list;
	}

	// Put the list view in a scroll view with a vertical scroll bar
	scroll_view = new BScrollView(
		"", list_view, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW, false, true, B_PLAIN_BORDER
	);
	AddChild(scroll_view);

	// Cancel button
	cancel_button = new BButton(
		BRect(0.0, 0.0, 0.0, 0.0), "", "Cancel", new BMessage(B_QUIT_REQUESTED),
		B_FOLLOW_RIGHT | B_FOLLOW_TOP
	);
	// Resize to make the text fit the button
	cancel_button->ResizeToPreferred();
	AddChild(cancel_button);

	// Main button
	the_button = new BButton(
		BRect(0.0, 0.0, 0.0, 0.0), "", "Use", new BMessage(INVOKE_LIST),
		B_FOLLOW_RIGHT | B_FOLLOW_TOP
	);
	// Make it the default button
	the_button->MakeDefault(true);
	// Resize to make the text fit the button
	the_button->ResizeToPreferred();
	// Move it to the extreme right
	the_button->MoveBy(
		scroll_view->Frame().right - the_button->Frame().right,
		scroll_view->Frame().bottom - the_button->Frame().top + 7.0
	);
	AddChild(the_button);

	// Center the cancel button next to the main button
	cancel_button->MoveBy(
		the_button->Frame().left - cancel_button->Frame().right - 10.0,
		the_button->Frame().top - cancel_button->Frame().top +
			(the_button->Frame().Height() - cancel_button->Frame().Height()) / 2.0
	);

	// Caption for the info view
	info_caption = new CaptionView(
		BRect(8.0, the_button->Frame().bottom + 1.0, Bounds().right - 8.0, 0.0),
		"Translator Information"
	);
	AddChild(info_caption);

	// Info view
	info_view =
		new InfoLinesView(BRect(16.0, info_caption->Frame().bottom + 1.0, Bounds().right, 0.0), 3);
	AddChild(info_view);

	// Resize the main view so that everything fits
	ResizeTo(Bounds().right, info_view->Frame().bottom + 8.0);
	// Make all views follow the bottom of the window when it is resized
	MakeFollowBottom();
}

// Main view destructor
OutputFormatView::~OutputFormatView() { delete[] output_list; }

// Stuff we can only do when the main view is attached to a window
void
OutputFormatView::AttachedToWindow()
{
	// Get the window and lock it
	the_window = Window();
	the_window->Lock();

	// Set some size limits on the window
	the_window->SetSizeLimits(
		200.0, 32767.0, Bounds().Height() - scroll_view->Bounds().Height() + 50.0, 32767.0
	);
	// Set the target for messages sent to this view
	list_view->SetTarget(this);
	the_button->SetTarget(this);

	// Make the list view the keyboard focus
	list_view->MakeFocus();

	// Select the first item in the list,
	// and make its config view show up
	if (list_view->CountItems() > 0)
		list_view->Select(0);
	else
		the_button->SetEnabled(false);

	// Unlock the window
	the_window->Unlock();

	// Call the base class
	BView::AttachedToWindow();
}

// Handle messages to the main view
void
OutputFormatView::MessageReceived(BMessage* msg)
{
	int32 item_index;
	const char* info_lines[3];

	switch (msg->what) {
	case ITEM_SELECTED:
		if (msg->FindInt32("index", &item_index) == B_OK && item_index >= 0 &&
			item_index < list_view->CountItems()) {
			// Store the currently selected item
			// in the class member 'index'
			index = item_index;
			// Update the info view and config view
			int32 outVersion;
			the_roster->GetTranslatorInfo(
				output_list[item_index].translator, &name_line, &info_line, &outVersion
			);
			int32 ver = outVersion / 100;
			int32 rev1 = (outVersion % 100) / 10;
			int32 rev2 = outVersion % 10;
			sprintf(version_line, "Version %ld.%ld.%ld", ver, rev1, rev2); // --SS %ld
			info_lines[0] = name_line;
			info_lines[1] = info_line;
			info_lines[2] = version_line;
			info_view->SetInfoLines(info_lines);
			AddConfigView();
		}
		else {
			// Reselect the original item
			if (index >= 0)
				list_view->Select(index);
		}
		break;

	case INVOKE_LIST: {
		// The main button was pressed
		// Find out which item is selected, and invoke it
		int32 item_count = list_view->CountItems();
		for (int32 i = 0; i < item_count; ++i) {
			if (list_view->IsItemSelected(i)) {
				list_view->Invoke();
				break;
			}
		}
		break;
	}

	case SEND_MESSAGE:
		// An output format has been selected
		if (!message_sent && msg->FindInt32("index", &item_index) == B_OK && item_index >= 0 &&
			item_index < list_view->CountItems()) {
			// Get the message from the invoker
			BMessage the_message(*(selected_invoker->Message()));

			// Add some info about the selected output format to it
			the_message.AddInt32("translator", output_list[item_index].translator);
			the_message.AddInt32("type", output_list[item_index].type);

			// Send the message
			selected_invoker->Invoke(&the_message);
			message_sent = true;
			the_window->PostMessage(MESSAGE_SENT);
		}
		break;

	default:
		BView::MessageReceived(msg);
		break;
	}
}

// Private function to add the config view of the currently
// selected output format to our window
void
OutputFormatView::AddConfigView()
{
	the_window->Lock();
	// Remove the window size limits,
	// so we are free to do whatever we like
	the_window->SetSizeLimits(0.0, 32767.0, 0.0, 32767.0);
	// The current views must stay put while we resize
	// the window to make room for the config view
	MakeFollowBottom(false);

	// Remove the old config view if there is one
	if (config_view) {
		RemoveChild(config_view);
		delete config_view;
		config_view = 0;
	}

	BRect config_rect;
	if (the_roster->MakeConfigurationView(
			output_list[index].translator, 0, &config_view, &config_rect
		) == B_NO_ERROR &&
		config_view != 0) {
		// Create a caption for the config view
		// if there wasn't one already
		if (!config_caption) {
			config_caption = new CaptionView(
				BRect(8.0, info_view->Frame().bottom + 1.0, Bounds().right - 8.0, 0.0),
				"Translator Settings"
			);
			config_caption->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
			AddChild(config_caption);
		}

		// Move the config view to just below the config caption
		config_view->MoveBy(
			16.0 - config_view->Frame().left,
			config_caption->Frame().bottom + 1.0 - config_view->Frame().top
		);

		// Colour it gray
		config_view->SetViewColor(222, 222, 222);
		config_view->SetLowColor(222, 222, 222);

		// First resize the window so that the config view will fit,
		// otherwise the config view may want to change with us
		float width_diff = config_view->Frame().right + 16.0 - Bounds().Width();
		the_window->ResizeTo(
			Bounds().Width() + (width_diff > 0.0 ? width_diff : 0.0),
			config_view->Frame().bottom + 8.0
		);

		// Add it to the main view
		AddChild(config_view);

		// Make the config_view and its children follow the bottom of the window
		RecursiveSetMode(config_view);
	}
	else {
		// This translator doesn't have a config view
		if (config_caption) {
			RemoveChild(config_caption);
			delete config_caption;
			config_caption = 0;
		}
		the_window->ResizeTo(Bounds().right, info_view->Frame().bottom + 8.0);
	}

	// Make all views follow the bottom of the window again
	MakeFollowBottom();

	// Set the window's minimum size
	float min_width = (config_view ? config_view->Frame().right : 200.0);
	if (min_width < 200.0)
		min_width = 200.0;
	the_window->SetSizeLimits(
		min_width, 32767.0, Bounds().Height() - scroll_view->Bounds().Height() + 50.0, 32767.0
	);

	the_window->Unlock();
}

// Private function to switch between follow bottom behaviour
void
OutputFormatView::MakeFollowBottom(bool flag)
{
	scroll_view->SetResizingMode(B_FOLLOW_LEFT_RIGHT | (flag ? B_FOLLOW_TOP_BOTTOM : B_FOLLOW_TOP));
	cancel_button->SetResizingMode(B_FOLLOW_RIGHT | (flag ? B_FOLLOW_BOTTOM : B_FOLLOW_TOP));
	the_button->SetResizingMode(B_FOLLOW_RIGHT | (flag ? B_FOLLOW_BOTTOM : B_FOLLOW_TOP));
	info_caption->SetResizingMode(B_FOLLOW_LEFT_RIGHT | (flag ? B_FOLLOW_BOTTOM : B_FOLLOW_TOP));
	info_view->SetResizingMode(B_FOLLOW_LEFT_RIGHT | (flag ? B_FOLLOW_BOTTOM : B_FOLLOW_TOP));
	if (config_caption)
		config_caption->SetResizingMode(
			B_FOLLOW_LEFT_RIGHT | (flag ? B_FOLLOW_BOTTOM : B_FOLLOW_TOP)
		);
}

// Recursively set the resizing mode of a view and its children
void
OutputFormatView::RecursiveSetMode(BView* view)
{
	// Set the parent view itself
	view->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);

	// Now set all its children
	BView* child;
	if ((child = view->ChildAt(0)) != 0) {
		while (child) {
			RecursiveSetMode(child);
			child = child->NextSibling();
		}
	}
}

// InfoLinesView constructor
InfoLinesView::InfoLinesView(BRect frame, int32 nLines)
	: BView(frame, "", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW), mLines(nLines)
{
	mLineTexts = new const char*[mLines]; // --SS const
	for (int32 i = 0; i < mLines; ++i)
		mLineTexts[i] = 0;

	SetViewColor(222, 222, 222);
	SetLowColor(222, 222, 222);

	BFont view_font;
	GetFont(&view_font);
	font_height fh;
	view_font.GetHeight(&fh);
	text_descent = fh.descent;
	text_height = fh.ascent + fh.descent + fh.leading;

	ResizeTo(frame.Width(), nLines * text_height);
}

// InfoLinesView destructor
InfoLinesView::~InfoLinesView() { delete[] mLineTexts; }

// Update the texts in the info lines
void
InfoLinesView::SetInfoLines(const char** LineTexts)
{
	for (int32 i = 0; i < mLines; ++i)
		mLineTexts[i] = LineTexts[i];
	Invalidate(Bounds());
}

// Draw the info lines
void
InfoLinesView::Draw(BRect /*updateRect*/)
{
	for (int32 i = 0; i < mLines; ++i) {
		if (mLineTexts[i]) {
			DrawString(mLineTexts[i], BPoint(0.0, (i + 1.0) * text_height - text_descent));
		}
	}
}

// CaptionView constructor
CaptionView::CaptionView(BRect frame, const char* caption)
	: BView(frame, "", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	mCaption = new char[strlen(caption) + 1];
	strcpy(mCaption, caption);

	SetViewColor(222, 222, 222);
	SetLowColor(222, 222, 222);

	SetFont(be_bold_font);
	font_height fh;
	be_bold_font->GetHeight(&fh);
	text_descent = fh.descent;
	text_height = fh.ascent + fh.descent + fh.leading;

	ResizeTo(frame.Width(), text_height + 19.0);
}

// CaptionView destructor
CaptionView::~CaptionView() { delete[] mCaption; }

// Draw the caption
void
CaptionView::Draw(BRect /*updateRect*/)
{
	// Caption text
	SetHighColor(0, 0, 0);
	DrawString(mCaption, BPoint(0.0, 10.0 + text_height - text_descent));

	// Dark line
	SetHighColor(144, 144, 144);
	FillRect(BRect(0.0, 12.0 + text_height, Bounds().right, 12.0 + text_height));

	// Bright line
	SetHighColor(255, 255, 255);
	FillRect(BRect(0.0, 13.0 + text_height, Bounds().right, 13.0 + text_height));
}
