/*
 *  OutputFormatWindow.h
 *
 *  Release 3.1.1 (May 21st, 1998)
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 *
 *  The OutputFormatWindow presents the user with a list of available
 *  output formats to which the translation kit is able to convert a
 *  given input stream. Simply creating an OutputFormatWindow object
 *  will open the window with the list where the user can choose from.
 *
 *  If the user selects an output format then the 'item_selected' invoker
 *  will be invoked to send its message. The message will have the
 *  translator_id and the type of the selected format attached to it:
 *
 *    translator_id out_translator;
 *    msg->FindInt32("translator", reinterpret_cast<int32 *>(&out_translator));
 *
 *    uint32 out_type;
 *    msg->FindInt32("type", reinterpret_cast<int32 *>(&out_type));
 *
 *  These can then easily be passed into the version of BTranslatorRosters's
 *  Translate() function that takes a translator_id:
 *
 *    roster->Translate(out_translator, in_stream, io_msg, out_stream, out_type);
 *
 *  If the user cancels (closes) the window then the 'window_cancelled'
 *  invoker, if one was specified, will be invoked to send a cancel message.
 *
 *  You may pass a pointer to a BTranslatorRoster object in the constructor,
 *  or 0 to use the default.
 *
 *  The class is inherited from BWindow, so you can manipulate it like any
 *  other window. E.g. Activate() or Close() it.
 */


#ifndef OUTPUT_FORMAT_WINDOW_H
#define OUTPUT_FORMAT_WINDOW_H


#include <Window.h>


class BInvoker;
class BPositionIO;
class BTranslatorRoster;

class OutputFormatWindow : public BWindow {
public:
	OutputFormatWindow(
		// Input stream (e.g. a ProgressiveBitmapStream of the bitmap to save)
		BPositionIO* in_stream,
		// Invoker for when an output format is selected
		BInvoker* item_selected,
		// Invoker for when the window is cancelled (optional)
		BInvoker* window_cancelled = 0,
		// Translator roster to use, or 0 for the default
		BTranslatorRoster* roster = 0);

	virtual ~OutputFormatWindow();
	virtual void MessageReceived(BMessage* msg);
	virtual bool QuitRequested();

private:
	BPositionIO* the_stream;
	BInvoker* selected_invoker;
	BInvoker* cancelled_invoker;
	BTranslatorRoster* the_roster;
	bool message_sent;
	bool setup_done;
};


#endif	// OUTPUT_FORMAT_WINDOW_H
