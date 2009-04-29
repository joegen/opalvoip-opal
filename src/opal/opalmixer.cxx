/*
 * opalmixer.cxx
 *
 * OPAL media mixers
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2007 Post Increment
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): Craig Southeren (craigs@postincrement.com)
 *                 Robert Jongbloed (robertj@voxlucida.com.au)
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#include <ptlib.h>

#include <opal/buildopts.h>

#include <opal/opalmixer.h>
#include <ptlib/vconvert.h>


/////////////////////////////////////////////////////////////////////////////

OpalBaseMixer::OpalBaseMixer(bool pushThread, unsigned periodMS, unsigned periodTS)
  : m_pushThread(pushThread)
  , m_periodMS(periodMS)
  , m_periodTS(periodTS)
  , m_workerThread(NULL)
  , m_threadRunning(false)
  , m_outputTimestamp(10000000)
{
}


void OpalBaseMixer::RemoveStream(const Key_T & key)
{
  PWaitAndSignal mutex(m_mutex);
  StreamMap_T::iterator iter = m_streams.find(key);
  if (iter == m_streams.end())
    return;

  delete iter->second;
  m_streams.erase(iter);
}


void OpalBaseMixer::RemoveAllStreams()
{
  PWaitAndSignal mutex(m_mutex);

  for (StreamMap_T::iterator iter = m_streams.begin(); iter != m_streams.end(); ++iter)
    delete iter->second;
  m_streams.clear();
}


bool OpalBaseMixer::AddStream(const Key_T & key)
{
  PWaitAndSignal mutex(m_mutex);

  StreamMap_T::iterator iter = m_streams.find(key);
  if (iter != m_streams.end()) 
    return false;

  m_streams[key] = CreateStream();
  StartPushThread();
  return true;
}


bool OpalBaseMixer::WriteStream(const Key_T & key, const RTP_DataFrame & rtp)
{
  if (rtp.GetPayloadSize() == 0)
    return true;

  PWaitAndSignal mutex(m_mutex);

  StreamMap_T::iterator iter = m_streams.find(key);
  if (iter == m_streams.end())
    return true; // Yes, true, writing a stream not yet up is non-fatal

  iter->second->m_queue.push(rtp);
  iter->second->m_queue.back().MakeUnique();
  return iter->second->m_queue.back().GetSize() > 0;
}


RTP_DataFrame * OpalBaseMixer::ReadMixed(const Key_T & streamToIgnore)
{
  // create output frame
  RTP_DataFrame * mixed = new RTP_DataFrame(GetOutputSize());

  PWaitAndSignal mutex(m_mutex);

  if (!MixStreams(streamToIgnore, *mixed)) {
    delete mixed;
    return NULL;
  }

  mixed->SetTimestamp(m_outputTimestamp);
  m_outputTimestamp += m_periodTS;

  return mixed;
}


void OpalBaseMixer::StartPushThread()
{
  if (m_pushThread) {
    PWaitAndSignal mutex(m_mutex);
    if (m_workerThread == NULL) {
      m_threadRunning = true;
      m_workerThread = new PThreadObj<OpalBaseMixer>(*this, &OpalBaseMixer::PushThreadMain);
    }
  }
}


void OpalBaseMixer::StopPushThread()
{
  m_mutex.Wait();

  m_threadRunning = false;
  if (m_workerThread != NULL) {
    m_mutex.Signal();
    m_workerThread->WaitForTermination();
    m_mutex.Wait();
    delete m_workerThread;
    m_workerThread = NULL;
  }

  m_mutex.Signal();
}


void OpalBaseMixer::PushThreadMain()
{
  PAdaptiveDelay delay;
  while (m_threadRunning) {
    RTP_DataFrame * mixed = ReadMixed(Key_T());
    if (mixed != NULL) {
      OnMixed(mixed);
      delete mixed;
    }

    delay.Delay(m_periodMS);
  }
}


bool OpalBaseMixer::OnMixed(RTP_DataFrame * & /*mixed*/)
{
  return false;
}


OpalBaseMixer::Stream::Stream(OpalBaseMixer & mixer)
  : m_mixer(mixer)
  , m_nextTimeStamp(UINT_MAX)
{
}


/////////////////////////////////////////////////////////////////////////////

OpalAudioMixer::OpalAudioMixer(bool stereo, unsigned sampleRate, bool pushThread, unsigned period)
  : OpalBaseMixer(pushThread, period, period*sampleRate/1000)
  , m_stereo(stereo)
  , m_sampleRate(sampleRate)
  , m_left(NULL)
  , m_right(NULL)
{
}


OpalBaseMixer::Stream * OpalAudioMixer::CreateStream()
{
  AudioStream * stream = new AudioStream(*this);

  if (m_stereo) {
    if (m_left == NULL)
      m_left = stream;
    else if (m_right == NULL)
      m_right = stream;
    else {
      PTRACE(2, "Mixer\tCannot have more than two streams for stereo mode!");
      delete stream;
      return NULL;
    }
  }

  return stream;
}


void OpalAudioMixer::RemoveStream(const Key_T & key)
{
  if (m_stereo) {
    StreamMap_T::iterator iter = m_streams.find(key);
    if (iter == m_streams.end())
      return;
    if (m_left == iter->second)
      m_left = NULL;
    else if (m_right == iter->second)
      m_right = NULL;
  }

  OpalBaseMixer::RemoveStream(key);
}


void OpalAudioMixer::RemoveAllStreams()
{
  OpalBaseMixer::RemoveAllStreams();
  m_left = m_right = NULL;
}


bool OpalAudioMixer::SetSampleRate(unsigned rate)
{
  if (m_streams.size() > 0)
    return rate == m_sampleRate;

  m_sampleRate = rate;
  m_periodTS = m_periodMS*rate/1000;
  return true;
}


bool OpalAudioMixer::MixStreams(const Key_T & streamToIgnore, RTP_DataFrame & mixed)
{
  return m_stereo ? MixStereo(mixed) : MixAdditive(streamToIgnore, mixed);
}


size_t OpalAudioMixer::GetOutputSize() const
{
  return m_periodTS*(m_stereo ? (sizeof(short) * 2) : sizeof(short));
}


bool OpalAudioMixer::MixStereo(RTP_DataFrame & mixed)
{
  if (m_left != NULL) {
    const short * src = m_left->GetAudioDataPtr();
    short * dst = (short *)mixed.GetPayloadPtr();
    for (unsigned i = 0; i < m_periodTS; ++i) {
      *dst++ = *src++;
      dst++;
    }
  }

  if (m_right != NULL) {
    const short * src = m_right->GetAudioDataPtr();
    short * dst = (short *)mixed.GetPayloadPtr();
    for (unsigned i = 0; i < m_periodTS; ++i) {
      dst++;
      *dst++ = *src++;
    }
  }

  return true;
}


bool OpalAudioMixer::MixAdditive(const Key_T & streamToIgnore, RTP_DataFrame & mixed)
{
  short * silence = (short *)alloca(m_periodTS*sizeof(short));
  memset(silence, 0, m_periodTS*sizeof(short));

  size_t streamCount = m_streams.size();
  const short ** buffers = (const short **)alloca(streamCount*sizeof(short *));

  size_t i = 0;
  for (StreamMap_T::iterator iter = m_streams.begin(); iter != m_streams.end(); ++iter, ++i) {
    if (iter->first == streamToIgnore)
      buffers[i] = silence;
    else
      buffers[i] = ((AudioStream *)iter->second)->GetAudioDataPtr();
  }

  short * dst  = (short *)mixed.GetPayloadPtr();
  for (unsigned samp = 0; samp < m_periodTS; ++samp) {
    int v = 0;
    for (size_t strm = 0; strm < streamCount; ++strm)
      v += *(buffers[strm])++;
    if (v < -32765)
      v = -32765;
    else if (v > 32765)
      v = 32765;
    *dst++ = (short)v;
  }

  return true;
}


OpalAudioMixer::AudioStream::AudioStream(OpalAudioMixer & mixer)
  : Stream(mixer)
  , m_samplesUsed(0)
{
}


const short * OpalAudioMixer::AudioStream::GetAudioDataPtr()
{
  size_t samplesLeft = m_mixer.GetPeriodTS();
  short * cachePtr = m_cacheSamples.GetPointer(samplesLeft);

  while (samplesLeft > 0 && !m_queue.empty()) {
    RTP_DataFrame * frame = &m_queue.front();
    if (m_nextTimeStamp == UINT_MAX)
      m_nextTimeStamp = frame->GetTimestamp();
    else {
      if (frame->GetTimestamp() > m_nextTimeStamp)
        break;
      while (frame->GetTimestamp()+frame->GetPayloadSize()/sizeof(short) < m_nextTimeStamp) {
        m_queue.pop();
        if (m_queue.empty())
          break;
        frame = &m_queue.front();
      }
      if (m_queue.empty())
        break;
    }

    size_t payloadSamples = frame->GetPayloadSize()/sizeof(short);
    size_t samplesToCopy = payloadSamples - m_samplesUsed;
    if (samplesToCopy > samplesLeft)
      samplesToCopy = samplesLeft;

    memcpy(cachePtr, ((const short *)frame->GetPayloadPtr())+m_samplesUsed, samplesToCopy*sizeof(short));

    cachePtr += samplesToCopy;
    samplesLeft -= samplesToCopy;
    m_nextTimeStamp += samplesToCopy;

    m_samplesUsed += samplesToCopy;
    if (m_samplesUsed >= payloadSamples) {
      m_queue.pop();
      m_samplesUsed = 0;
    }
  }

  if (samplesLeft > 0) {
    memset(cachePtr, 0, samplesLeft*sizeof(short)); // Silence
    if (m_nextTimeStamp != UINT_MAX)
      m_nextTimeStamp += samplesLeft;
  }

  return m_cacheSamples;
}


//////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalVideoMixer::OpalVideoMixer(Styles style, unsigned width, unsigned height, unsigned rate, bool pushThread)
  : OpalBaseMixer(pushThread, 1000/rate, OpalMediaFormat::VideoClockRate/rate)
  , m_style(style)
{
  SetFrameSize(width, height);
}


bool OpalVideoMixer::SetFrameRate(unsigned rate)
{
  if (rate == 0 || rate > 100)
    return false;

  m_mutex.Wait();
  m_periodMS = 1000/rate;
  m_periodTS = OpalMediaFormat::VideoClockRate/rate;
  m_mutex.Signal();

  return true;
}


bool OpalVideoMixer::SetFrameSize(unsigned width, unsigned height)
{
  m_mutex.Wait();

  m_width = width;
  m_height = height;
  PColourConverter::FillYUV420P(0, 0, m_width, m_height, m_width, m_height,
                                m_frameStore.GetPointer(m_width*m_height*3/2),
                                0, 0, 0);

  m_mutex.Signal();
  return true;
}


OpalBaseMixer::Stream * OpalVideoMixer::CreateStream()
{
  return new VideoStream(*this);
}


bool OpalVideoMixer::MixStreams(const Key_T & streamToIgnore, RTP_DataFrame & mixed)
{
  // create output frame
  unsigned left, x, y, w, h;
  switch (m_style) {
    case eSideBySideLetterbox :
      x = left = 0;
      y = m_height/4;
      w = m_width/2;
      h = m_height/2;
      break;

    case eSideBySideScaled :
      x = left = 0;
      y = 0;
      w = m_width/2;
      h = m_height;
      break;

    case eStackedPillarbox :
      x = left = m_width/4;
      y = 0;
      w = m_width/2;
      h = m_height/2;
      break;

    case eStackedScaled :
      x = left = 0;
      y = 0;
      w = m_width;
      h = m_height/2;
      break;

    case eGrid2x2 :
      x = left = 0;
      y = 0;
      w = m_width/2;
      h = m_height/2;
      break;

    case eGrid3x3 :
      x = left = 0;
      y = 0;
      w = m_width/3;
      h = m_height/3;
      break;

    case eGrid4x4 :
      x = left = 0;
      y = 0;
      w = m_width/4;
      h = m_height/4;
      break;

    default :
      return false;
  }

  for (StreamMap_T::iterator iter = m_streams.begin(); iter != m_streams.end(); ++iter) {
    if (iter->first == streamToIgnore)
      continue;

    ((VideoStream *)iter->second)->InsertVideoFrame(x, y, w, h);

    x += w;
    if (x+w > m_width) {
      x = left;
      y += h;
      if (y+h > m_height)
        break;
    }
  }

  PluginCodec_Video_FrameHeader * video = (PluginCodec_Video_FrameHeader *)mixed.GetPayloadPtr();
  video->width = m_width;
  video->height = m_height;
  memcpy(OPAL_VIDEO_FRAME_DATA_PTR(video), m_frameStore, m_frameStore.GetSize());

  return true;
}


size_t OpalVideoMixer::GetOutputSize() const
{
  return m_frameStore.GetSize() + sizeof(PluginCodec_Video_FrameHeader);
}


OpalVideoMixer::VideoStream::VideoStream(OpalVideoMixer & mixer)
  : Stream(mixer)
  , m_mixer(mixer)
{
}


void OpalVideoMixer::VideoStream::InsertVideoFrame(unsigned x, unsigned y, unsigned w, unsigned h)
{
  if (m_queue.empty()) {
    IncrementTimestamp();
    return;
  }

  RTP_DataFrame * frame = &m_queue.front();

  if (m_nextTimeStamp == UINT_MAX)
    m_nextTimeStamp = frame->GetTimestamp();
  else {
    if (frame->GetTimestamp() > m_nextTimeStamp) {
      PTRACE(6, "Mixer\tLeaving frame @" << frame->GetTimestamp() << " (" << m_nextTimeStamp << ')');
      IncrementTimestamp();
      return;
    }

    while (frame->GetTimestamp()+m_mixer.GetPeriodTS() < m_nextTimeStamp) {
      PTRACE(5, "Mixer\tDropping frame @" << frame->GetTimestamp() << " (" << m_nextTimeStamp << ')');

      m_queue.pop();
      if (m_queue.empty()) {
        IncrementTimestamp();
        return;
      }

      frame = &m_queue.front();
    }
  }

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)frame->GetPayloadPtr();

  PTRACE(6, "Mixer\tCopying video: " << header->width << 'x' << header->height
         << " -> " << x << ',' << y << '/' << w << 'x' << h << " @"
         << frame->GetTimestamp() << " (" << m_nextTimeStamp << ')');

  PColourConverter::CopyYUV420P(0, 0, header->width, header->height,
                                header->width, header->height, OPAL_VIDEO_FRAME_DATA_PTR(header),
                                x, y, w, h,
                                m_mixer.m_width, m_mixer.m_height, m_mixer.m_frameStore.GetPointer(),
                                PVideoFrameInfo::eScale);

  IncrementTimestamp();
  m_queue.pop();
}


void OpalVideoMixer::VideoStream::IncrementTimestamp()
{
  if (m_nextTimeStamp != UINT_MAX)
    m_nextTimeStamp += m_mixer.GetPeriodTS();
}


#endif // OPAL_VIDEO

//////////////////////////////////////////////////////////////////////////////
