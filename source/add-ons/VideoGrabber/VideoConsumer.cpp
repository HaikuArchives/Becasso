//	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
//	SMS
//	VideoConsumer.cpp

#include <View.h>
#include <stdio.h>
#include <fcntl.h>
#include <Buffer.h>
#include <unistd.h>
#include <string.h>
#include <NodeInfo.h>
#include <scheduler.h>
#include <TimeSource.h>
#include <StringView.h>
#include <MediaRoster.h>
#include <Application.h>
#include <BufferGroup.h>

#include "VideoConsumer.h"

#define M1 ((double)1000000.0)
#define JITTER 20000

#define FUNCTION (void)
#define ERROR printf
#define PROGRESS (void)
#define LOOP (void)

static status_t
SetFileType(BFile* file, int32 translator, uint32 type);

const media_raw_video_format vid_format = {
	29.97, 1, 0, 239, B_VIDEO_TOP_LEFT_RIGHT, 1, 1, {B_RGB16, 320, 240, 320 * 4, 0, 0}
};

//---------------------------------------------------------------

VideoConsumer::VideoConsumer(
	const char* name, BView* view, BMediaAddOn* addon, const int32 internal_id
)
	:

	  BMediaNode(name), BMediaEventLooper(), BBufferConsumer(B_MEDIA_RAW_VIDEO),
	  mInternalID(internal_id), mAddOn(addon), mConnectionActive(false), mMyLatency(20000),
	  mWindow(NULL), mView(view), mOurBuffers(false), mBuffers(NULL), mRate(1000000),
	  mImageFormat(0), mTranslator(0), fLastIndex(-1)
{
	AddNodeKind(B_PHYSICAL_OUTPUT);
	SetEventLatency(0);
	mWindow = mView->Window();

	for (uint32 j = 0; j < 3; j++) {
		mBitmap[j] = NULL;
		mBufferMap[j] = NULL;
	}

	SetPriority(B_DISPLAY_PRIORITY);

	mImageFormat = 'PNG ';
}

//---------------------------------------------------------------

VideoConsumer::~VideoConsumer()
{
	Quit();

	//	if (mWindow)
	//	{
	//		if (mWindow->Lock())
	//		{
	//			mWindow->Close();
	//			mWindow = 0;
	//		}
	//	}

	DeleteBuffers();
}

/********************************
	From BMediaNode
********************************/

//---------------------------------------------------------------

BMediaAddOn*
VideoConsumer::AddOn(int32* cookie) const
{
	// do the right thing if we're ever used with an add-on
	*cookie = mInternalID;
	return mAddOn;
}

//---------------------------------------------------------------

// This implementation is required to get around a bug in
// the ppc compiler.

void
VideoConsumer::Start(bigtime_t performance_time)
{
	BMediaEventLooper::Start(performance_time);
}

void
VideoConsumer::Stop(bigtime_t performance_time, bool immediate)
{
	BMediaEventLooper::Stop(performance_time, immediate);
}

void
VideoConsumer::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	BMediaEventLooper::Seek(media_time, performance_time);
}

void
VideoConsumer::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

status_t
VideoConsumer::DeleteHook(BMediaNode* node)
{
	return BMediaEventLooper::DeleteHook(node);
}

//---------------------------------------------------------------

void
VideoConsumer::NodeRegistered()
{
	mIn.destination.port = ControlPort();
	mIn.destination.id = 0;
	mIn.source = media_source::null;
	mIn.format.type = B_MEDIA_RAW_VIDEO;
	mIn.format.u.raw_video = vid_format;

	Run();
}

//---------------------------------------------------------------

status_t
VideoConsumer::RequestCompleted(const media_request_info& info)
{
	switch (info.what) {
	case media_request_info::B_SET_OUTPUT_BUFFERS_FOR: {
		if (info.status != B_OK)
			ERROR("VideoConsumer::RequestCompleted: Not using our buffers!\n");
	} break;
	default:
		break;
	}
	return B_OK;
}

//---------------------------------------------------------------

void
VideoConsumer::BufferReceived(BBuffer* buffer)
{
	//	LOOP("VideoConsumer::Buffer #%ld received\n", (uint32) buffer->ID());

	if (RunState() == B_STOPPED) {
		buffer->Recycle();
		return;
	}

	media_timed_event event(
		buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER, buffer,
		BTimedEventQueue::B_RECYCLE_BUFFER
	);
	EventQueue()->AddEvent(event);
}

//---------------------------------------------------------------

void
VideoConsumer::ProducerDataStatus(
	const media_destination& for_whom, int32 status, bigtime_t at_media_time
)
{
	if (for_whom != mIn.destination)
		return;
}

//---------------------------------------------------------------

status_t
VideoConsumer::CreateBuffers(const media_format& with_format)
{
	// delete any old buffers
	DeleteBuffers();

	status_t status = B_OK;

	// create a buffer group
	uint32 mXSize = with_format.u.raw_video.display.line_width;
	uint32 mYSize = with_format.u.raw_video.display.line_count;
	//	uint32 mRowBytes = with_format.u.raw_video.display.bytes_per_row;
	color_space mColorspace = with_format.u.raw_video.display.format;
	//	PROGRESS("VideoConsumer::CreateBuffers - Colorspace = %d\n", mColorspace);

	mBuffers = new BBufferGroup();
	status = mBuffers->InitCheck();
	if (B_OK != status) {
		ERROR("VideoConsumer::CreateBuffers - ERROR CREATING BUFFER GROUP\n");
		return status;
	}
	// and attach the  bitmaps to the buffer group
	for (uint32 j = 0; j < 3; j++) {
		mBitmap[j] = new BBitmap(BRect(0, 0, (mXSize - 1), (mYSize - 1)), mColorspace, false, true);
		if (mBitmap[j]->IsValid()) {
			buffer_clone_info info;
			if ((info.area = area_for(mBitmap[j]->Bits())) == B_ERROR)
				ERROR("VideoConsumer::CreateBuffers - ERROR IN AREA_FOR\n");
			;
			info.offset = 0;
			info.size = (size_t)mBitmap[j]->BitsLength();
			info.flags = j;
			info.buffer = 0;

			if ((status = mBuffers->AddBuffer(info)) != B_OK) {
				ERROR("VideoConsumer::CreateBuffers - ERROR ADDING BUFFER TO GROUP\n");
				return status;
			} // else PROGRESS("VideoConsumer::CreateBuffers - SUCCESSFUL ADD BUFFER TO GROUP\n");
		}
		else {
			ERROR(
				"VideoConsumer::CreateBuffers - ERROR CREATING VIDEO RING BUFFER: %08lx\n", status
			);
			return B_ERROR;
		}
	}

	BBuffer** buffList = new BBuffer*[3];
	for (int j = 0; j < 3; j++)
		buffList[j] = 0;

	if ((status = mBuffers->GetBufferList(3, buffList)) == B_OK)
		for (int j = 0; j < 3; j++)
			if (buffList[j] != NULL) {
				mBufferMap[j] = buffList[j];
				//				PROGRESS(" j = %d buffer = %08lx\n", j, mBufferMap[j]);
			}
			else {
				ERROR("VideoConsumer::CreateBuffers ERROR MAPPING RING BUFFER\n");
				return B_ERROR;
			}
	else
		ERROR("VideoConsumer::CreateBuffers ERROR IN GET BUFFER LIST\n");

	return status;
}

//---------------------------------------------------------------

void
VideoConsumer::DeleteBuffers()
{
	if (mBuffers) {
		delete mBuffers;
		mBuffers = NULL;

		for (uint32 j = 0; j < 3; j++)
			if (mBitmap[j]->IsValid()) {
				delete mBitmap[j];
				mBitmap[j] = NULL;
			}
	}
}

//---------------------------------------------------------------

status_t
VideoConsumer::Connected(
	const media_source& producer, const media_destination& where, const media_format& with_format,
	media_input* out_input
)
{
	mIn.source = producer;
	mIn.format = with_format;
	mIn.node = Node();
	sprintf(mIn.name, "Video Consumer");
	*out_input = mIn;

	uint32 user_data = 0;
	int32 change_tag = 1;
	if (CreateBuffers(with_format) == B_OK)
		BBufferConsumer::SetOutputBuffersFor(
			producer, mDestination, mBuffers, (void*)&user_data, &change_tag, true
		);
	else {
		ERROR("VideoConsumer::Connected - COULDN'T CREATE BUFFERS\n");
		return B_ERROR;
	}

	mConnectionActive = true;

	return B_OK;
}

//---------------------------------------------------------------

void
VideoConsumer::Disconnected(const media_source& producer, const media_destination& where)
{
	if (where == mIn.destination && producer == mIn.source) {
		// disconnect the connection
		mIn.source = media_source::null;
		mConnectionActive = false;
	}
}

//---------------------------------------------------------------

status_t
VideoConsumer::AcceptFormat(const media_destination& dest, media_format* format)
{
	if (dest != mIn.destination) {
		ERROR("VideoConsumer::AcceptFormat - BAD DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	if (format->type == B_MEDIA_NO_TYPE)
		format->type = B_MEDIA_RAW_VIDEO;

	if (format->type != B_MEDIA_RAW_VIDEO) {
		ERROR("VideoConsumer::AcceptFormat - BAD FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.format != B_RGB32 &&
		format->u.raw_video.display.format != B_RGB16 &&
		format->u.raw_video.display.format != B_RGB15 &&
		format->u.raw_video.display.format != B_GRAY8 &&
		format->u.raw_video.display.format != media_raw_video_format::wildcard.display.format) {
		ERROR("AcceptFormat - not a format we know about!\n");
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.format == media_raw_video_format::wildcard.display.format) {
		format->u.raw_video.display.format = B_RGB16;
	}

	char format_string[256];
	string_for_format(*format, format_string, 256);
	return B_OK;
}

//---------------------------------------------------------------

status_t
VideoConsumer::GetNextInput(int32* cookie, media_input* out_input)
{
	// custom build a destination for this connection
	// put connection number in id

	if (*cookie < 1) {
		mIn.node = Node();
		mIn.destination.id = *cookie;
		sprintf(mIn.name, "Video Consumer");
		*out_input = mIn;
		(*cookie)++;
		return B_OK;
	}
	else {
		//		ERROR("VideoConsumer::GetNextInput - - BAD INDEX\n");
		return B_MEDIA_BAD_DESTINATION;
	}
}

//---------------------------------------------------------------

void
VideoConsumer::DisposeInputCookie(int32 /*cookie*/)
{
}

//---------------------------------------------------------------

status_t
VideoConsumer::GetLatencyFor(
	const media_destination& for_whom, bigtime_t* out_latency, media_node_id* out_timesource
)
{
	if (for_whom != mIn.destination)
		return B_MEDIA_BAD_DESTINATION;

	*out_latency = mMyLatency;
	*out_timesource = TimeSource()->ID();
	return B_OK;
}

//---------------------------------------------------------------

status_t
VideoConsumer::FormatChanged(
	const media_source& producer, const media_destination& consumer, int32 from_change_count,
	const media_format& format
)
{
	FUNCTION("VideoConsumer::FormatChanged\n");

	if (consumer != mIn.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (producer != mIn.source)
		return B_MEDIA_BAD_SOURCE;

	mIn.format = format;

	return CreateBuffers(format);
}

//---------------------------------------------------------------

void
VideoConsumer::HandleEvent(const media_timed_event* event, bigtime_t lateness, bool realTimeEvent)

{
	//	LOOP("VideoConsumer::HandleEvent\n");

	BBuffer* buffer;

	switch (event->type) {
	case BTimedEventQueue::B_START:
		PROGRESS("VideoConsumer::HandleEvent - START\n");
		break;
	case BTimedEventQueue::B_STOP:
		PROGRESS("VideoConsumer::HandleEvent - STOP\n");
		EventQueue()->FlushEvents(
			event->event_time, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER
		);
		break;
	case BTimedEventQueue::B_USER_EVENT:
		PROGRESS("VideoConsumer::HandleEvent - USER EVENT\n");
		if (RunState() == B_STARTED) {
			PROGRESS(
				"Pushing user event for %.4f, time now %.4f\n", (event->event_time + mRate) / M1,
				event->event_time / M1
			);
			media_timed_event newEvent(event->event_time + mRate, BTimedEventQueue::B_USER_EVENT);
			EventQueue()->AddEvent(newEvent);
		}
		break;
	case BTimedEventQueue::B_HANDLE_BUFFER:
		// LOOP("VideoConsumer::HandleEvent - HANDLE BUFFER\n");
		buffer = (BBuffer*)event->pointer;
		if (RunState() == B_STARTED && mConnectionActive) {
			// see if this is one of our buffers
			uint32 index = 0;
			mOurBuffers = true;
			while (index < 3)
				if (buffer == mBufferMap[index])
					break;
				else
					index++;

			if (index == 3) {
				// no, buffers belong to consumer
				mOurBuffers = false;
				index = 0;
			}

			if ((RunMode() == B_OFFLINE) ||
				((TimeSource()->Now() > (buffer->Header()->start_time - JITTER)) &&
				 (TimeSource()->Now() < (buffer->Header()->start_time + JITTER)))) {
				if (!mOurBuffers)
					// not our buffers, so we need to copy
					memcpy(mBitmap[index]->Bits(), buffer->Data(), mBitmap[index]->BitsLength());

				if (mWindow->Lock()) {
					uint32 flags;
					if ((mBitmap[index]->ColorSpace() == B_GRAY8) &&
						!bitmaps_support_space(mBitmap[index]->ColorSpace(), &flags)) {
						// handle mapping of GRAY8 until app server knows how
						uint32* start = (uint32*)mBitmap[index]->Bits();
						int32 size = mBitmap[index]->BitsLength();
						uint32* end = start + size / 4;
						for (uint32* p = start; p < end; p++)
							*p = (*p >> 3) & 0x1f1f1f1f;
					}

					mView->DrawBitmap(mBitmap[index], mView->Bounds());
					fLastIndex = index;
					mWindow->Unlock();
				}
			}
			else
				PROGRESS("VidConsumer::HandleEvent - DROPPED FRAME\n");
			buffer->Recycle();
		}
		else
			buffer->Recycle();
		break;
	default:
		ERROR("VideoConsumer::HandleEvent - BAD EVENT\n");
		break;
	}
}

//---------------------------------------------------------------

BBitmap*
VideoConsumer::Grab()
{
	if (fLastIndex == -1)
		return 0;

	return (mBitmap[fLastIndex]);
}

status_t
VideoConsumer::LocalSave(char* filename, BBitmap* bitmap)
{
	BFile* output;

	/* save a local copy of the image in the requested format */
	output = new BFile();
	if (output->SetTo(filename, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE) == B_NO_ERROR) {
		BBitmapStream input(bitmap);
		status_t err =
			BTranslatorRoster::Default()->Translate(&input, NULL, NULL, output, mImageFormat);
		if (err == B_OK) {
			err = SetFileType(output, mTranslator, mImageFormat);
			if (err != B_OK)
				printf("Error setting type of output file");
		}
		else {
			printf("Error writing output file");
		}
		input.DetachBitmap(&bitmap);
		output->Unset();
		delete output;
		return B_OK;
	}
	else {
		printf("Error creating output file");
		return B_ERROR;
	}
}

//---------------------------------------------------------------

status_t
SetFileType(BFile* file, int32 translator, uint32 type)
{
	translation_format* formats;
	int32 count;

	status_t err = BTranslatorRoster::Default()->GetOutputFormats(
		translator, (const translation_format**)&formats, &count
	);
	if (err < B_OK)
		return err;
	const char* mime = NULL;
	for (int ix = 0; ix < count; ix++) {
		if (formats[ix].type == type) {
			mime = formats[ix].MIME;
			break;
		}
	}
	if (mime == NULL) { /* this should not happen, but being defensive might be prudent */
		return B_ERROR;
	}

	/* use BNodeInfo to set the file type */
	BNodeInfo ninfo(file);
	return ninfo.SetType(mime);
}
