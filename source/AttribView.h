#ifndef ATTRIBVIEW_H
#define ATTRIBVIEW_H

#include <View.h>
#include <PropertyInfo.h>
#include <Message.h>

#define GET_AND_SET                                                                                \
	{                                                                                              \
		B_GET_PROPERTY, B_SET_PROPERTY, 0                                                          \
	}
#define SET                                                                                        \
	{                                                                                              \
		B_SET_PROPERTY, 0                                                                          \
	}
#define GET                                                                                        \
	{                                                                                              \
		B_GET_PROPERTY, 0                                                                          \
	}
#define DIRECT_AND_INDEX                                                                           \
	{                                                                                              \
		B_DIRECT_SPECIFIER, B_INDEX_SPECIFIER, 0                                                   \
	}
#define DIRECT                                                                                     \
	{                                                                                              \
		B_DIRECT_SPECIFIER, 0                                                                      \
	}

class AttribView : public BView {
  public:
	AttribView(BRect frame, const char* title);
	virtual ~AttribView();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);

  private:
	typedef BView inherited;
};

#endif