#ifndef ATTRIBSELECT_H
#define ATTRIBSELECT_H

#include <Message.h>
#include "AttribView.h"

class AttribSelect : public AttribView {
public:
	AttribSelect();
	virtual ~AttribSelect();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property);
	virtual status_t GetSupportedSuites(BMessage* message);

private:
	typedef AttribView inherited;
};

#endif