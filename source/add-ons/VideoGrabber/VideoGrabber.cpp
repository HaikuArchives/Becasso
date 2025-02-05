// © 2000-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include <Button.h>
#include <Application.h>
#include <scheduler.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include <Alert.h>
#include "VideoConsumer.h"

#include <string.h>

class CaptureWindow : public BWindow
{
  public:
	CaptureWindow(BRect rect)
		: BWindow(
			  rect, "Video Grabber", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK
		  ){};
	virtual ~CaptureWindow();

	virtual bool QuitRequested()
	{
		Hide();
		TearDownNodes();
		return false;
	};

	BBitmap* Grab();

	status_t SetUpNodes();
	void TearDownNodes();

  private:
	BMediaRoster* fMediaRoster;

	media_node fTimeSourceNode;
	media_node fProducerNode;

	VideoConsumer* fVideoConsumer;

	media_output fProducerOut;
	media_input fConsumerIn;

  public:
	BView* fVideoView;
};

void
ErrorAlert(const char* message, status_t err);

CaptureWindow::~CaptureWindow() {}

BBitmap*
CaptureWindow::Grab()
{
	BBitmap* ori = fVideoConsumer->Grab();
	BBitmap* grab = new BBitmap(ori->Bounds(), B_RGBA32, true);
	grab->Lock();
	BView* v = new BView(ori->Bounds(), "tmp", B_FOLLOW_NONE, B_WILL_DRAW);
	grab->AddChild(v);
	v->DrawBitmap(ori, B_ORIGIN); // convert to 32 bit
	v->Sync();
	grab->RemoveChild(v);
	delete v;
	uint32* b = (uint32*)grab->Bits();
	for (int32 i = 0; i < grab->BitsLength() / 4; i++) {
		*(b++) |= (255 << ALPHA_BPOS); // grabbed image is totally transparent by default...
	}
	grab->Unlock();
	return grab;
}

status_t
CaptureWindow::SetUpNodes()
{
	status_t status = B_OK;

	/* find the media roster */
	fMediaRoster = BMediaRoster::Roster(&status);
	if (status != B_OK) {
		ErrorAlert("Can't find the media roster", status);
		return status;
	}
	/* find the time source */
	status = fMediaRoster->GetTimeSource(&fTimeSourceNode);
	if (status != B_OK) {
		ErrorAlert("Can't get a time source", status);
		return status;
	}
	/* find a video producer node */
	// printf ("Grabber acquiring VideoInput node\n");
	status = fMediaRoster->GetVideoInput(&fProducerNode);
	if (status != B_OK) {
		ErrorAlert("Can't find a video input!", status);
		return status;
	}

	/* create the video consumer node */
	fVideoConsumer = new VideoConsumer("Grabber", fVideoView, NULL, 0);
	if (!fVideoConsumer) {
		ErrorAlert("Can't create a video window", B_ERROR);
		return B_ERROR;
	}

	/* register the node */
	status = fMediaRoster->RegisterNode(fVideoConsumer);
	if (status != B_OK) {
		ErrorAlert("Can't register the video window", status);
		return status;
	}
	//	fPort = fVideoConsumer->ControlPort();

	/* find free producer output */
	int32 cnt = 0;
	status =
		fMediaRoster->GetFreeOutputsFor(fProducerNode, &fProducerOut, 1, &cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		ErrorAlert("Can't find an available video stream", status);
		return status;
	}

	/* find free consumer input */
	cnt = 0;
	status = fMediaRoster->GetFreeInputsFor(
		fVideoConsumer->Node(), &fConsumerIn, 1, &cnt, B_MEDIA_RAW_VIDEO
	);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		ErrorAlert("Can't find an available connection to the video window", status);
		return status;
	}

	/* Connect The Nodes!!! */
	media_format format;
	format.type = B_MEDIA_RAW_VIDEO;
	media_raw_video_format vid_format = media_raw_video_format::wildcard;
	//	{ 0, 2, 0, VIDEO_SIZE_Y - 1, B_VIDEO_TOP_LEFT_RIGHT, 1, 1, {B_RGB32, VIDEO_SIZE_X,
	// VIDEO_SIZE_Y, VIDEO_SIZE_X*4, 0, 0}};
	format.u.raw_video = vid_format;

	/* connect producer to consumer */
	status = fMediaRoster->Connect(
		fProducerOut.source, fConsumerIn.destination, &format, &fProducerOut, &fConsumerIn
	);
	if (status != B_OK) {
		ErrorAlert("Can't connect the video source to the video window", status);
		return status;
	}

	/* set time sources */
	status = fMediaRoster->SetTimeSourceFor(fProducerNode.node, fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video source", status);
		return status;
	}

	status = fMediaRoster->SetTimeSourceFor(fVideoConsumer->ID(), fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video window", status);
		return status;
	}

	/* figure out what recording delay to use */
	bigtime_t latency = 0;
	status = fMediaRoster->GetLatencyFor(fProducerNode, &latency);
	status = fMediaRoster->SetProducerRunModeDelay(fProducerNode, latency);

	/* start the nodes */
	bigtime_t initLatency = 0;
	status = fMediaRoster->GetInitialLatencyFor(fProducerNode, &initLatency);
	if (status < B_OK) {
		ErrorAlert("error getting initial latency for fCaptureNode", status);
	}
	initLatency += estimate_max_scheduling_latency();

	BTimeSource* timeSource = fMediaRoster->MakeTimeSourceFor(fProducerNode);
	bool running = timeSource->IsRunning();

	/* workaround for people without sound cards */
	/* because the system time source won't be running */
	bigtime_t real = BTimeSource::RealTime();
	if (!running) {
		status = fMediaRoster->StartTimeSource(fTimeSourceNode, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("cannot start time source!", status);
			return status;
		}
		status = fMediaRoster->SeekTimeSource(fTimeSourceNode, 0, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("cannot seek time source!", status);
			return status;
		}
	}

	bigtime_t perf = timeSource->PerformanceTimeFor(real + latency + initLatency);
	timeSource->Release();

	/* start the nodes */
	status = fMediaRoster->StartNode(fProducerNode, perf);
	if (status != B_OK) {
		ErrorAlert("Can't start the video source", status);
		return status;
	}
	status = fMediaRoster->StartNode(fVideoConsumer->Node(), perf);
	if (status != B_OK) {
		ErrorAlert("Can't start the video window", status);
		return status;
	}

	return status;
}

void
CaptureWindow::TearDownNodes()
{
	if (!fMediaRoster)
		return;
	if (fVideoConsumer) {
		/* stop */
		// printf ("stopping nodes!\n");
		fMediaRoster->StopNode(fVideoConsumer->Node(), 0, true);

		/* disconnect */
		fMediaRoster->Disconnect(
			fProducerOut.node.node, fProducerOut.source, fConsumerIn.node.node,
			fConsumerIn.destination
		);

		if (fProducerNode != media_node::null) {
			// printf ("Grabber releasing fProducerNode\n");
			fMediaRoster->ReleaseNode(fProducerNode);
			fProducerNode = media_node::null;
		}
		fMediaRoster->ReleaseNode(fVideoConsumer->Node());
		fVideoConsumer = NULL;
	}
}

//---------------------------------------------------------------
// Utility functions
//---------------------------------------------------------------

void
ErrorAlert(const char* message, status_t err)
{
	char msg[256];
	sprintf(msg, "%s\n%s [%" B_PRIx32 "]", message, strerror(err), err);
	(new BAlert("", msg, "Quit"))->Go();
}

//---------------------------------------------------------------


CaptureWindow* window;

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "VideoGrabber");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 2000-2001 ∑ Sum Software");
	strcpy(info->description, "Captures images from a video node");
	info->type = BECASSO_CAPTURE;
	info->index = index;
	info->version = 0;
	info->release = 2;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = true;
	window = new CaptureWindow(BRect(100, 180, 100 + 168 + 40, 180 + 128));
	BView* bg = new BView(BRect(0, 0, 168 + 40, 128), "bg", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	window->fVideoView =
		new BView(BRect(4, 4, 4 + 159, 4 + 119), "video", B_FOLLOW_ALL, B_WILL_DRAW);
	bg->SetViewColor(LightGrey);
	window->AddChild(bg);
	bg->AddChild(window->fVideoView);
	BMessage* msg = new BMessage(CAPTURE_READY);
	msg->AddInt32("index", index);
	BButton* grab = new BButton(BRect(168, 96, 204, 112), "grab", "Grab", msg);
	grab->SetTarget(be_app);
	grab->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	bg->AddChild(grab);
	window->Run();
	return B_OK;
}

status_t
addon_exit(void)
{
	return B_OK;
}

status_t
addon_open(void)
{
	window->Lock();
	if (window->IsHidden())
		window->Show();
	else
		window->Activate();
	window->Unlock();
	status_t status = window->SetUpNodes();
	if (status != B_OK) {
		ErrorAlert("Error setting up nodes", status);
		return (1);
	}
	return B_OK;
}

BBitmap*
bitmap(char* title)
{
	strcpy(title, "Frame");
	BBitmap* b = window->Grab();
	return (b);
}
