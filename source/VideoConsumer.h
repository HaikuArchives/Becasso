//	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
//	SMS
/*	VideoConsumer.h	*/

#if !defined(VID_CONSUMER_H)
#define VID_CONSUMER_H

#include <View.h>
#include <Bitmap.h>
#include <Window.h>
#include <MediaNode.h>
#include <TranslationKit.h>
#include <BufferConsumer.h>
#include <TimedEventQueue.h>
#include <MediaEventLooper.h>

class VideoConsumer : public BMediaEventLooper, public BBufferConsumer
{
  public:
	VideoConsumer(const char* name, BView* view, BMediaAddOn* addon, const int32 internal_id);
	~VideoConsumer();

	/*	BMediaNode */
  public:
	virtual BMediaAddOn* AddOn(int32* cookie) const;

  protected:
	virtual void Start(bigtime_t performance_time);
	virtual void Stop(bigtime_t performance_time, bool immediate);
	virtual void Seek(bigtime_t media_time, bigtime_t performance_time);
	virtual void TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);

	virtual void NodeRegistered();
	virtual status_t RequestCompleted(const media_request_info& info);

	virtual status_t DeleteHook(BMediaNode* node);

	/*  BMediaEventLooper */
  protected:
	virtual void
	HandleEvent(const media_timed_event* event, bigtime_t lateness, bool realTimeEvent);
	/*	BBufferConsumer */
  public:
	virtual status_t AcceptFormat(const media_destination& dest, media_format* format);
	virtual status_t GetNextInput(int32* cookie, media_input* out_input);

	virtual void DisposeInputCookie(int32 cookie);

  protected:
	virtual void BufferReceived(BBuffer* buffer);

  private:
	virtual void
	ProducerDataStatus(const media_destination& for_whom, int32 status, bigtime_t at_media_time);
	virtual status_t
	GetLatencyFor(const media_destination& for_whom, bigtime_t* out_latency, media_node_id* out_id);
	virtual status_t Connected(
		const media_source& producer, const media_destination& where,
		const media_format& with_format, media_input* out_input
	);
	virtual void Disconnected(const media_source& producer, const media_destination& where);
	virtual status_t FormatChanged(
		const media_source& producer, const media_destination& consumer, int32 from_change_count,
		const media_format& format
	);

	/*	implementation */

  public:
	status_t CreateBuffers(const media_format& with_format);

	void DeleteBuffers();

	BBitmap* Grab();

	status_t LocalSave(char* filename, BBitmap* bitmap);


  private:
	int32 mInternalID;
	BMediaAddOn* mAddOn;

	bool mConnectionActive;
	media_input mIn;
	media_destination mDestination;
	bigtime_t mMyLatency;

	BWindow* mWindow;
	BView* mView;
	BBitmap* mBitmap[3];
	bool mOurBuffers;
	BBufferGroup* mBuffers;
	BBuffer* mBufferMap[3];

	bigtime_t mRate;
	uint32 mImageFormat;
	int32 mTranslator;
	int32 fLastIndex;
};

#endif
