#include <ptlib.h>

#include <opal/buildopts.h>

#include <opal/opalmixer.h>

#define MS_TO_BYTES(ms)         (ms*16)
#define MS_TO_SAMPLES(ms)       (ms*8)

#define BYTES_TO_MS(bytes)      (bytes/16)
#define BYTES_TO_SAMPLES(bytes) (bytes/2)

OpalAudioMixerStream::StreamFrame::StreamFrame(const RTP_DataFrame & rtp)
  :	PMemBuffer<PMutex>(rtp.GetPayloadPtr(), rtp.GetPayloadSize()), timestamp(rtp.GetTimestamp())
  ,	channelNumber(0)
{
}

OpalAudioMixerStream::OpalAudioMixerStream()
{ 
  active = PFalse; 
  first  = PTrue;
  writtenTimeStamp = cacheTimeStamp = 80000000;
}

void OpalAudioMixerStream::WriteFrame(const StreamFrame & frame)
{
  PWaitAndSignal m(mutex);
  if (frame.GetSize() != 0 && frame.timestamp != writtenTimeStamp) {
    writtenTimeStamp = frame.timestamp;
    frameQueue.push(frame);
    PTRACE(6, "Mixer\tWrite CH=" << channelNumber << " TS=" << frame.timestamp << " SZ=" << frame.GetSize());
  }
}

void OpalAudioMixerStream::FillSilence(StreamFrame & retFrame, PINDEX ms)
{
  retFrame.SetSize(MS_TO_BYTES(ms));
  memset(retFrame.GetPointerAndLock(), 0, MS_TO_BYTES(ms));
  retFrame.Unlock();
}

void OpalAudioMixerStream::PopFrame(StreamFrame & retFrame, PINDEX ms)
{
  PAssert(frameQueue.size() > 0, "attempt to pop from empty queue");

  // get the top frame
  StreamFrame & frame = frameQueue.front();

  // if we want the entire frame, return all of it
  // otherwise return partial frame and put the rest into the cache
  if (BYTES_TO_MS(frame.GetSize()) == ms) {

    // zerocopy cached frame to the returned frame
    retFrame = frame;

    // clear the current frame cache
    frameCache.SetSize(0);
    PTRACE(6, "Mixer\tPop full CH=" << channelNumber << " TS=" << frame.timestamp);
  }
  else
  {
    // zerocopy current frame into cache and returned frame
    frameCache = frame;
    retFrame = frameCache;

    // data length is shorter of what we want, or what we got
    PINDEX len = PMIN(MS_TO_BYTES(ms), frameCache.GetSize());
    PAssert(len == MS_TO_BYTES(ms), "attempt to copy partial frame");

    // adjust to correct length
    retFrame.SetSize(len);

    // rebase cache to reflect removed data
    frameCache.Rebase(len);
//    frameCache.SetSize(frameCache.GetSize() - len);
    PTRACE(6, "Mixer\tPop part CH=" << channelNumber << " TS=" << frame.timestamp << " SZ=" << len);
  }

  // remove the frame from the queue
  frameQueue.pop();
}

PBoolean OpalAudioMixerStream::ReadFrame(StreamFrame & retFrame, PINDEX ms)
{
  mutex.Wait();

  // always return silence until first frame arrives
  if (first) {
    if (frameQueue.size() == 0) {
      mutex.Signal();
      PTRACE(6, "Mixer\tRead queue empty 1 CH=" << channelNumber);
      return PFalse;
    }
    cacheTimeStamp = frameQueue.front().timestamp;
    first = PFalse;
  }

  // if there is data in the cache, return it
  if (frameCache.GetSize() > 0) {

    // zercopy cache into returned frame
    retFrame = frameCache;

    // return lesser of cached data size and what we requested
    PINDEX len = PMIN(MS_TO_BYTES(ms), frameCache.GetSize());
    PAssert(len == MS_TO_BYTES(ms), "attempt to copy partial frame");

    // zero out data if needed (we don't do partial frames)
    if (len < MS_TO_BYTES(ms)) {
      memset(retFrame.GetPointerAndLock()+len, 0, MS_TO_BYTES(ms) - len);
      retFrame.Unlock();
    }

    // set the timestamp on the returned data
    retFrame.timestamp = cacheTimeStamp;

    // rebase cache to reflect removed data
    frameCache.Rebase(len);
    //frameCache.SetSize(frameCache.GetSize()-len);
    cacheTimeStamp += BYTES_TO_SAMPLES(len);

    mutex.Signal();

    PTRACE(6, "Mixer\tRead cached CH=" << channelNumber << " TS=" << retFrame.timestamp << " SZ=" << len);
    return PTrue;
  }

  // invariant: no data in the "cache" buffer

  // if the stream is not active, check if there are two frames
  // if not, return silence
  // if there are, activate the stream and return from the first frame
  if (!active) {
    if (frameQueue.size() == 0) {
      cacheTimeStamp += MS_TO_SAMPLES(ms);
      mutex.Signal();
      PTRACE(6, "Mixer\tRead queue empty 2 CH=" << channelNumber);
      return PFalse;
    }

    // the stream is now active
    active = PTrue;

    // get from the first frame
    PopFrame(retFrame, ms);

    retFrame.timestamp = cacheTimeStamp;
    cacheTimeStamp += MS_TO_SAMPLES(ms);

    mutex.Signal();

    return PTrue;
  }

  // invariant: stream is active and cacheTimeStamp is valid

  // if no data available, deactivate the stream and return silence
  if (frameQueue.size() == 0) {
    cacheTimeStamp += MS_TO_SAMPLES(ms);
    active = PFalse;
    mutex.Signal();
    PTRACE(6, "Mixer\tRead queue empty 3 CH=" << channelNumber);
    return PFalse;
  }

  // invariant: the queue contains one or more frames of data

  // check timestamp on the top frame 
  StreamFrame & frame = frameQueue.front();

  // if looking for data before the queue, fill with silence and return
  if (cacheTimeStamp < frame.timestamp) {
    cacheTimeStamp += MS_TO_SAMPLES(ms);
    mutex.Signal();
    PTRACE(6, "Mixer\tRead early CH=" << channelNumber << " TS " << cacheTimeStamp << " < " << frame.timestamp);
    return PFalse;
  }

  // get cached data
  PopFrame(retFrame, ms);

  // realign to current timestamp
  cacheTimeStamp = frame.timestamp;
  cacheTimeStamp += MS_TO_SAMPLES(ms);

  mutex.Signal();

  return PTrue;
}

/////////////////////////////////////////////////////////////////////////////

OpalAudioMixer::MixerFrame::MixerFrame(PINDEX _frameLengthSamples)
  : frameLengthSamples(_frameLengthSamples)
{
}

void OpalAudioMixer::MixerFrame::CreateMixedData() const
{
  PWaitAndSignal m(mutex);
  if (mixedData.GetSize() != 0)
    return;

  mixedData.SetSize(frameLengthSamples);
  memset(mixedData.GetPointer(), 0, frameLengthSamples*sizeof(int));

  MixerPCMMap_T::const_iterator r;
  for (r = channelData.begin(); r != channelData.end(); ++r) {
    PINDEX i;
    const short * src = (short *)r->second.GetPointerAndLock();
    int * dst = mixedData.GetPointer();
    for (i = 0; i < frameLengthSamples; ++i)
      *dst++ += *src++;
    r->second.Unlock();
  }
}

PBoolean OpalAudioMixer::MixerFrame::GetMixedFrame(OpalAudioMixerStream::StreamFrame & frame) const
{
  CreateMixedData();

  frame.SetSize(frameLengthSamples * 2);
  PINDEX i;
  int *   src = mixedData.GetPointer();
  short * dst  = (short *)frame.GetPointerAndLock();
  for (i = 0; i < frameLengthSamples; ++i) {
    int v = *src++;
    if (v < -32765)
      v = -32765;
    else if (v > 32765)
      v = 32765;
    *dst++ = (short)v;
  }
  frame.Unlock();
  return PTrue;
}

PBoolean OpalAudioMixer::MixerFrame::GetStereoFrame(OpalAudioMixerStream::StreamFrame & frame) const
{
  frame.SetSize(frameLengthSamples * 2 * 2);

  PWaitAndSignal m(mutex);

  if (channelData.size() == 0 || channelData.size() > 2)
    return PFalse;

  short * dst = (short *)frame.GetPointerAndLock();

  if (channelData.size() == 1) {
    const OpalAudioMixerStream::StreamFrame & srcFrame = channelData.begin()->second;
    const short * src = (const short *)srcFrame.GetPointerAndLock();
    PINDEX i = frameLengthSamples;
    PINDEX offs = srcFrame.channelNumber;
    PAssert(offs < 2, "cannot create stereo with more than 2 sources");
    while (i-- > 0) {
      dst[offs]     = *src++;
      dst[offs ^ 1] = 0;
      dst += 2;
    }
    srcFrame.Unlock();
  }
  else {
    MixerPCMMap_T::const_iterator r = channelData.begin();
    const OpalAudioMixerStream::StreamFrame & srcFrame1 = (r++)->second;
    const OpalAudioMixerStream::StreamFrame & srcFrame2 = (r++)->second;
    const short * src1 = (const short *)srcFrame1.GetPointerAndLock();
    const short * src2 = (const short *)srcFrame2.GetPointerAndLock();
    PINDEX i = frameLengthSamples;
    PINDEX offs1 = srcFrame1.channelNumber;
    PINDEX offs2 = srcFrame2.channelNumber;
    PAssert(offs1 < 2 && offs2 < 2, "cannot create stereo with more than 2 sources");
    while (i-- > 0) {
      dst[offs1] = *src1++;
      dst[offs2] = *src2++;
      dst += 2;
    }
    srcFrame2.Unlock();
    srcFrame1.Unlock();
  }

  frame.Unlock();

  return PTrue;
}

PBoolean OpalAudioMixer::MixerFrame::GetChannelFrame(Key_T key, OpalAudioMixerStream::StreamFrame & frame) const
{
  MixerPCMMap_T::const_iterator r = channelData.find(key);
  if (r == channelData.end())
    return PFalse;

  CreateMixedData();

  frame.SetSize(frameLengthSamples * 2);
  PINDEX i;
  int *   src1 = mixedData.GetPointer();
  short * src2 = (short *)r->second.GetPointerAndLock();
  short * dst  = (short *)frame.GetPointerAndLock();
  for (i = 0; i < frameLengthSamples; ++i) {
    int v = *src1++ - *src2++;
    if (v < -32765)
      v = -32765;
    else if (v > 32765)
      v = 32765;
    *dst++ = (short)v;
  }

  r->second.Unlock();
  frame.Unlock();

  return PTrue;
}

void OpalAudioMixer::MixerFrame::InsertFrame(Key_T key, OpalAudioMixerStream::StreamFrame &frame)
{
	channelData.insert(MixerPCMMap_T::value_type(key, frame));
}

OpalAudioMixer::OpalAudioMixer(PBoolean _realTime, PBoolean _pushThread)
  : frameLengthMs(10)
  , channelNumber(0)
  , realTime(_realTime)
  , pushThread(_pushThread)
  , mixerWorkerThread(NULL)
  , threadRunning(false)
  , audioStarted(false)
  , firstRead(true)
  , outputTimestamp(10000000)
{
}


PBoolean OpalAudioMixer::OnWriteAudio(const MixerFrame &)
{
  return true;
}

void OpalAudioMixer::AddStream(const Key_T & key, OpalAudioMixerStream *stream)
{
  PWaitAndSignal m(mutex);
  stream->channelNumber = channelNumber++;
  streamInfoMap.insert(StreamInfoMap_T::value_type(key, stream));
  StartThread();
}


void OpalAudioMixer::RemoveStream(const Key_T & key)
{
  PWaitAndSignal m(mutex);
  StreamInfoMap_T::iterator r = streamInfoMap.find(key);
  if (r != streamInfoMap.end()) {
    delete r->second;
    streamInfoMap.erase(r);
  }
}


void OpalAudioMixer::RemoveAllStreams()
{
  threadRunning = false;
  if (mixerWorkerThread != NULL) {
    mixerWorkerThread->WaitForTermination();
    delete mixerWorkerThread;
    mixerWorkerThread = NULL;
  }

  while (streamInfoMap.size() > 0)
    RemoveStream(streamInfoMap.begin()->first);

  channelNumber = 0;
}

void OpalAudioMixer::StartThread()
{
  if (pushThread) {
    PWaitAndSignal m(mutex);
    if (mixerWorkerThread == NULL) {
  	  threadRunning = true;
      mixerWorkerThread = new PThreadObj<OpalAudioMixer>(*this, &OpalAudioMixer::ThreadMain);
      mixerWorkerThread->SetThreadName("Mixer");
    }
  }
}

void OpalAudioMixer::ThreadMain()
{
  PAdaptiveDelay delay;

  while (threadRunning) {
    delay.Delay(frameLengthMs);
    ReadRoutine();
  }
}

//
// read a block of mixed audio from the mixer
//
void OpalAudioMixer::ReadRoutine()
{
  PTime now;

  PWaitAndSignal m(mutex);

  // if this is the first read, set the first read time
  if (firstRead) {
    timeOfNextRead = PTime() + 5 * frameLengthMs;
    firstRead = PFalse;
    return;
  }

  // output as many frames as needed
  while (now >= timeOfNextRead) {
    WriteMixedFrame();
    timeOfNextRead += frameLengthMs;
  }
}

void OpalAudioMixer::WriteMixedFrame()
{
  // create output frame
  MixerFrame * mixerFrame = new MixerFrame(MS_TO_SAMPLES(frameLengthMs));

  // lock the mixer
  mutex.Wait();

  // iterate through the streams and get an unmixed frame from each one
  StreamInfoMap_T::iterator r;
  OpalAudioMixerStream::StreamFrame tempFrame;
  for (r = streamInfoMap.begin(); r != streamInfoMap.end(); ++r) {
    OpalAudioMixerStream & stream = *r->second;
    if (stream.ReadFrame(tempFrame, frameLengthMs)) {
      tempFrame.timestamp = outputTimestamp;
      tempFrame.channelNumber = stream.channelNumber;
      mixerFrame->InsertFrame(r->first, tempFrame);
    }
  }

  // increment the output timestamp
  outputTimestamp += MS_TO_SAMPLES(frameLengthMs);

  // Unlock the mixer before calling the possibly time consuming OnWriteAudio() function
  mutex.Signal();

  if (!pushThread) {
    //outputQueue.push_back(mixerFrame);
  }
  else {
    OnWriteAudio(*mixerFrame);
    delete mixerFrame;
  }
}

//
// write a frame of data to a named stream of the mixer
//
PBoolean OpalAudioMixer::Write(const Key_T & key, const RTP_DataFrame & rtp)
{
  if (rtp.GetPayloadSize() == 0)
    return PTrue;

  // copy the data to PMembuffer
  OpalAudioMixerStream::StreamFrame frame(rtp);

  {
    PWaitAndSignal m(mutex);

    // find or create the stream we writing to
    StreamInfoMap_T::iterator r = streamInfoMap.find(key);
    OpalAudioMixerStream * stream;
    if (r != streamInfoMap.end()) 
      stream = r->second;
    else {
      stream = new OpalAudioMixerStream();
      AddStream(key, stream);
    }

    // write the data
    stream->WriteFrame(frame);

    // and tag the stream as started
    audioStarted = PTrue;
  }

  return PTrue;
}
