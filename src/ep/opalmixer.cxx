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

#include <ep/opalmixer.h>

#if OPAL_HAS_MIXER

  #ifdef _MSC_VER
    #pragma message("Mixer (MCU) support enabled")
  #endif

#include <opal/patch.h>
#include <rtp/rtp.h>
#include <rtp/jitter.h>
#include <ptlib/vconvert.h>
#include <ptclib/pwavfile.h>
#include <sip/handlers.h>
#include <sip/sipcon.h>


#define DETAIL_LOG_LEVEL 6


#if OPAL_MIXER_AUDIO_DEBUG
class PAudioMixerDebug
{
public:
  PTextFile                     m_csv;
  std::map<PString, PWAVFile *> m_wavFiles;

  PAudioMixerDebug(const PString & name)
    : m_csv("MixerDebug-"+name+".csv", PFile::WriteOnly)
  {
  }
  ~PAudioMixerDebug()
  {
    for (std::map<PString, PWAVFile *>::iterator it = m_wavFiles.begin(); it != m_wavFiles.end(); ++it)
      delete it->second;
  }

  void SaveWAV(const PString & strm, const RTP_DataFrame & rtp)
  {
    PWAVFile * wav = m_wavFiles[strm];
    if (wav == NULL)
      m_wavFiles[strm] = wav = new PWAVFile("MixerDebug-"+strm+".wav", PFile::WriteOnly);
    wav->Write(rtp.GetPayloadPtr(), rtp.GetPayloadSize());
  }
};
#define MIXER_DEBUG_OUT(expr) m_audioDebug->m_csv << expr
#define MIXER_DEBUG_WAV(strm,rtp) m_audioDebug->SaveWAV(strm, rtp)
#else
#define MIXER_DEBUG_OUT(expr)
#define MIXER_DEBUG_WAV(strm,rtp)
#endif

/////////////////////////////////////////////////////////////////////////////

OpalBaseMixer::OpalBaseMixer(bool pushThread, unsigned periodMS, unsigned periodTS)
  : m_pushThread(pushThread)
  , m_periodMS(periodMS)
  , m_periodTS(periodTS)
  , m_outputTimestamp(10000000)
  , m_pushFrame(NULL)
  , m_workerThread(NULL)
  , m_threadRunning(false)
{
}


OpalBaseMixer::~OpalBaseMixer()
{
  delete m_pushFrame;
}


void OpalBaseMixer::RemoveStream(const Key_T & key)
{
  m_mutex.Wait();

  StreamMap_T::iterator iter = m_inputStreams.find(key);
  if (iter != m_inputStreams.end()) {
    delete iter->second;
    m_inputStreams.erase(iter);
    PTRACE(4, "Mixer\tRemoved stream at key " << key);
  }

  if (m_inputStreams.empty())
    StopPushThread(false); // Will unlock mutex
  else
    m_mutex.Signal();
}


void OpalBaseMixer::RemoveAllStreams()
{
  PTRACE(4, "Mixer\tRemoving all streams");

  m_mutex.Wait();

  for (StreamMap_T::iterator iter = m_inputStreams.begin(); iter != m_inputStreams.end(); ++iter)
    delete iter->second;
  m_inputStreams.clear();

  StopPushThread(false); // Will unlock mutex
}


bool OpalBaseMixer::AddStream(const Key_T & key)
{
  PWaitAndSignal mutex(m_mutex);

  StreamMap_T::iterator iter = m_inputStreams.find(key);
  if (iter != m_inputStreams.end()) 
    return false;

  m_inputStreams[key] = CreateStream();
  PTRACE(4, "Mixer\tAdded input stream at key " << key);

  StartPushThread();
  return true;
}


bool OpalBaseMixer::WriteStream(const Key_T & key, const RTP_DataFrame & rtp)
{
  if (rtp.GetPayloadSize() == 0)
    return true;

  RTP_DataFrame uniqueRTP = rtp;
  uniqueRTP.MakeUnique();
  if (uniqueRTP.IsEmpty())
    return false;

  m_mutex.Wait();

  // Search for stream, note: writing a stream not yet attached is non-fatal
  StreamMap_T::iterator iter = m_inputStreams.find(key);
  if (iter != m_inputStreams.end())
    iter->second->QueuePacket(uniqueRTP);

  m_mutex.Signal();

  return true;
}


RTP_DataFrame * OpalBaseMixer::ReadMixed()
{
  // create output frame
  RTP_DataFrame * mixed = new RTP_DataFrame((PINDEX)0, GetOutputSize());
  mixed->SetPayloadType(RTP_DataFrame::MaxPayloadType);
  if (ReadMixed(*mixed))
    return mixed;

  delete mixed;
  return NULL;
}


bool OpalBaseMixer::ReadMixed(RTP_DataFrame & mixed)
{
  PWaitAndSignal mutex(m_mutex);

  if (!MixStreams(mixed))
    return false;

  mixed.SetTimestamp(m_outputTimestamp);
  m_outputTimestamp += m_periodTS;

  return true;
}


void OpalBaseMixer::StartPushThread()
{
  if (m_pushThread) {
    PWaitAndSignal mutex(m_mutex);
    if (m_workerThread == NULL) {
      m_threadRunning = true;
      m_workerThread = new PThreadObj<OpalBaseMixer>(*this,
                                                     &OpalBaseMixer::PushThreadMain,
                                                     false,
                                                     "OpalMixer",
                                                     PThread::HighestPriority);
    }
  }
}


void OpalBaseMixer::StopPushThread(bool lock)
{
  if (lock)
    m_mutex.Wait();

  PThread * thread = m_workerThread;
  m_workerThread = NULL;
  m_threadRunning = false;

  m_mutex.Signal();

  if (thread != NULL) {
    PTRACE(4, "Mixer\tWaiting for push thread to terminate");
    PAssert(thread->WaitForTermination(5000), "Mixer worker thread took too long to terminate.");
    delete thread;
  }
}


void OpalBaseMixer::PushThreadMain()
{
  PTRACE(4, "Mixer\tPushThread start " << m_periodMS << " ms");
  PAdaptiveDelay delay;
  while (m_threadRunning && OnPush())
    delay.Delay(m_periodMS);

  PTRACE(4, "Mixer\tPushThread end");
}


bool OpalBaseMixer::OnPush()
{
  if (m_pushFrame == NULL) {
    m_pushFrame = new RTP_DataFrame((PINDEX)0, GetOutputSize());
    m_pushFrame->SetPayloadType(RTP_DataFrame::MaxPayloadType);
  }

  return ReadMixed(*m_pushFrame) && OnMixed(m_pushFrame);
}


bool OpalBaseMixer::OnMixed(RTP_DataFrame * & /*mixed*/)
{
  return false;
}


/////////////////////////////////////////////////////////////////////////////

OpalAudioMixer::OpalAudioMixer(bool stereo,
                           unsigned sampleRate,
                               bool pushThread,
                           unsigned period)
  : OpalBaseMixer(pushThread, period, period*sampleRate/1000)
  , m_stereo(stereo)
  , m_sampleRate(sampleRate)
  , m_left(NULL)
  , m_right(NULL)
{
  m_mixedAudio.resize(m_periodTS);
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
    PWaitAndSignal mutex(m_mutex);
    StreamMap_T::iterator iter = m_inputStreams.find(key);
    if (iter == m_inputStreams.end())
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
  PWaitAndSignal mutex(m_mutex);

  if (m_inputStreams.size() > 0)
    return rate == m_sampleRate;

  m_periodTS = m_periodMS*rate/1000;
  m_sampleRate = rate;
  m_mixedAudio.resize(m_periodTS);
  for (StreamMap_T::iterator iter = m_inputStreams.begin(); iter != m_inputStreams.end(); ++iter)
    ((AudioStream *)iter->second)->m_cacheSamples.SetSize(m_periodTS);
  return true;
}


bool OpalAudioMixer::SetJitterBufferSize(const Key_T & key, unsigned minJitterDelay, unsigned maxJitterDelay)
{
  PWaitAndSignal mutex(m_mutex);

  StreamMap_T::iterator iter = m_inputStreams.find(key);
  if (iter == m_inputStreams.end())
    return false;

  OpalJitterBuffer * & jitter = ((AudioStream *)iter->second)->m_jitter;
  if (jitter != NULL) {
    if (minJitterDelay != 0 && maxJitterDelay != 0)
      jitter->SetDelay(minJitterDelay, maxJitterDelay);
    else {
      PTRACE(4, "AudioMix\tJitter buffer disabled");
      delete jitter;
      jitter = NULL;
    }
  }
  else {
    if (minJitterDelay != 0 && maxJitterDelay != 0) {
      PTRACE(4, "AudioMix\tJitter buffer enabled");
      jitter = new OpalJitterBuffer(minJitterDelay, maxJitterDelay, m_sampleRate/1000);
    }
  }

  return true;
}


void OpalAudioMixer::PreMixStreams()
{
  // Expected to already be mutexed

  size_t streamCount = m_inputStreams.size();
  const short ** buffers = (const short **)alloca(streamCount*sizeof(short *));

  size_t i = 0;
  for (StreamMap_T::iterator iter = m_inputStreams.begin(); iter != m_inputStreams.end(); ++iter, ++i)
    buffers[i] = ((AudioStream *)iter->second)->GetAudioDataPtr();

  for (unsigned samp = 0; samp < m_periodTS; ++samp) {
    m_mixedAudio[samp] = 0;
    for (size_t strm = 0; strm < streamCount; ++strm)
      m_mixedAudio[samp] += *(buffers[strm])++;
  }
}


bool OpalAudioMixer::MixStreams(RTP_DataFrame & frame)
{
  // Expected to already be mutexed

  if (m_stereo)
    MixStereo(frame);
  else {
    PreMixStreams();
    frame.SetPayloadSize(0);
    MixAdditive(frame, NULL);
  }
  return true;
}


void OpalAudioMixer::MixStereo(RTP_DataFrame & frame)
{
  // Expected to already be mutexed

  frame.SetPayloadSize(GetOutputSize());

  if (m_left != NULL) {
    const short * src = m_left->GetAudioDataPtr();
    short * dst = (short *)frame.GetPayloadPtr();
    for (unsigned i = 0; i < m_periodTS; ++i) {
      *dst++ = *src++;
      dst++;
    }
  }

  if (m_right != NULL) {
    const short * src = m_right->GetAudioDataPtr();
    short * dst = (short *)frame.GetPayloadPtr();
    for (unsigned i = 0; i < m_periodTS; ++i) {
      dst++;
      *dst++ = *src++;
    }
  }
}


void OpalAudioMixer::MixAdditive(RTP_DataFrame & frame, const short * audioToSubtract)
{
  // Expected to already be mutexed

  PINDEX size = frame.GetPayloadSize();
  frame.SetPayloadSize(size + m_periodTS*sizeof(short));

  if (size == 0)
    frame.SetTimestamp(m_outputTimestamp);

  short * dst = (short *)(frame.GetPayloadPtr()+size);
  for (unsigned i = 0; i < m_periodTS; ++i) {
    int value = m_mixedAudio[i];
    if (audioToSubtract != NULL)
      value -= *audioToSubtract++;
    if (value < -32765)
      value = -32765;
    else if (value > 32765)
      value = 32765;
    *dst++ = (short)value;
  }
}


size_t OpalAudioMixer::GetOutputSize() const
{
  return m_periodTS*(m_stereo ? (sizeof(short) * 2) : sizeof(short));
}


OpalAudioMixer::AudioStream::AudioStream(OpalAudioMixer & mixer)
  : m_mixer(mixer)
  , m_jitter(NULL)
  , m_nextTimestamp(0)
  , m_cacheSamples(mixer.GetPeriodTS())
  , m_samplesUsed(0)
{
}


OpalAudioMixer::AudioStream::~AudioStream()
{
  delete m_jitter;
}


void OpalAudioMixer::AudioStream::QueuePacket(const RTP_DataFrame & rtp)
{
  if (m_jitter == NULL)
    m_queue.push(rtp);
  else
    m_jitter->WriteData(rtp);
}


const short * OpalAudioMixer::AudioStream::GetAudioDataPtr()
{
  size_t samplesLeft = m_mixer.GetPeriodTS();
  short * cachePtr = m_cacheSamples.GetPointer(samplesLeft);

  while (samplesLeft > 0) {
    if (m_queue.empty()) {
      if (m_jitter == NULL)
        break;
      RTP_DataFrame frame;
      frame.SetTimestamp(m_nextTimestamp);
      if (!m_jitter->ReadData(frame) || frame.GetPayloadSize() == 0)
        break;
      m_nextTimestamp = frame.GetTimestamp();
      m_queue.push(frame);
    }

    size_t payloadSamples = m_queue.front().GetPayloadSize()/sizeof(short);
    size_t samplesToCopy = payloadSamples - m_samplesUsed;
    if (samplesToCopy > samplesLeft)
      samplesToCopy = samplesLeft;

    memcpy(cachePtr, ((const short *)m_queue.front().GetPayloadPtr())+m_samplesUsed, samplesToCopy*sizeof(short));

    cachePtr += samplesToCopy;
    samplesLeft -= samplesToCopy;
    m_nextTimestamp += samplesToCopy;

    m_samplesUsed += samplesToCopy;
    if (m_samplesUsed >= payloadSamples) {
      m_queue.pop();
      m_samplesUsed = 0;
    }
  }

  if (samplesLeft > 0) {
    memset(cachePtr, 0, samplesLeft*sizeof(short)); // Silence
    m_nextTimestamp += samplesLeft;
  }

  return m_cacheSamples;
}


//////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalVideoMixer::OpalVideoMixer(Styles style, unsigned width, unsigned height, unsigned rate, bool pushThread)
  : OpalBaseMixer(pushThread, 1000/rate, OpalMediaFormat::VideoClockRate/rate)
  , m_style(style)
  , m_bgFillRed(0)
  , m_bgFillGreen(0)
  , m_bgFillBlue(0)
  , m_lastStreamCount(0)
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
                                m_bgFillRed, m_bgFillGreen, m_bgFillBlue);

  m_mutex.Signal();
  return true;
}


OpalBaseMixer::Stream * OpalVideoMixer::CreateStream()
{
  return new VideoStream(*this);
}


bool OpalVideoMixer::MixStreams(RTP_DataFrame & frame)
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

    case eGrid :
      x = left = 0;
      y = 0;
      if (m_lastStreamCount != m_inputStreams.size()) {
        PColourConverter::FillYUV420P(0, 0, m_width, m_height, m_width, m_height,
                                      m_frameStore.GetPointer(),
                                      m_bgFillRed, m_bgFillGreen, m_bgFillBlue);
        m_lastStreamCount = m_inputStreams.size();
      }
      switch (m_lastStreamCount) {
        case 0 :
        case 1 :
          w = m_width;
          h = m_height;
          break;

        case 2 :
          y = m_height/4;
          // Fall into next case

        case 3 :
        case 4 :
          w = m_width/2;
          h = m_height/2;
          break;

        case 5 :
        case 6 :
        case 7 :
        case 8 :
        case 9 :
          w = m_width/3;
          h = m_height/3;
          break;

        default :
          w = m_width/4;
          h = m_height/4;
          break;
      }
      break;

    default :
      return false;
  }

  // This makes sure subimage are on 32 bit boundary, some parts of the
  // system can get mightily upset if this is not the case.
  w &= 0xfffffffc;
  h &= 0xfffffffc;

  for (StreamMap_T::iterator iter = m_inputStreams.begin(); iter != m_inputStreams.end(); ++iter) {
    ((VideoStream *)iter->second)->InsertVideoFrame(x, y, w, h);

    x += w;
    if (x+w > m_width) {
      x = left;
      y += h;
      if (y+h > m_height)
        break;
    }
  }

  frame.SetPayloadSize(GetOutputSize());
  PluginCodec_Video_FrameHeader * video = (PluginCodec_Video_FrameHeader *)frame.GetPayloadPtr();
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
  : m_mixer(mixer)
{
}


void OpalVideoMixer::VideoStream::QueuePacket(const RTP_DataFrame & rtp)
{
  m_queue.push(rtp);
}


void OpalVideoMixer::VideoStream::InsertVideoFrame(unsigned x, unsigned y, unsigned w, unsigned h)
{
  if (m_queue.empty())
    return;

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)m_queue.front().GetPayloadPtr();

  PTRACE(DETAIL_LOG_LEVEL, "Mixer\tCopying video: " << header->width << 'x' << header->height
         << " -> " << x << ',' << y << '/' << w << 'x' << h);

  PColourConverter::CopyYUV420P(0, 0, header->width, header->height,
                                header->width, header->height, OPAL_VIDEO_FRAME_DATA_PTR(header),
                                x, y, w, h,
                                m_mixer.m_width, m_mixer.m_height, m_mixer.m_frameStore.GetPointer(),
                                PVideoFrameInfo::eScale);

  /* To avoid continual build up of frames in queue if input frame rate
     greater than mixer frame, we flush the queue, but keep one to allow for
     slight mismatches in timing when frame rates are identical. */
  do {
    m_queue.pop();
  } while (m_queue.size() > 1);
}


#endif // OPAL_VIDEO


///////////////////////////////////////////////////////////////////////////////

OpalMixerEndPoint::OpalMixerEndPoint(OpalManager & manager, const char * prefix)
  : OpalLocalEndPoint(manager, prefix)
  , OpalMixerNodeManager(manager)
  , m_adHocNodeInfo(NULL)
  , m_factoryNodeInfo(NULL)
{
  PTRACE(4, "MixerEP\tConstructed");
}


OpalMixerEndPoint::~OpalMixerEndPoint()
{
  delete m_adHocNodeInfo;
  PTRACE(4, "MixerEP\tDestroyed");
}


void OpalMixerEndPoint::ShutDown()
{
  PTRACE(4, "MixerEP\tShutting down");

  OpalMixerNodeManager::ShutDown();

  OpalLocalEndPoint::ShutDown();
}


OpalMediaFormatList OpalMixerEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;
  formats += OpalPCM16;
#if OPAL_VIDEO
  formats += OpalYUV420P;
#endif
  return formats;
}


PSafePtr<OpalConnection> OpalMixerEndPoint::MakeConnection(OpalCall & call,
                                                      const PString & party,
                                                               void * userData,
                                                         unsigned int options,
                                      OpalConnection::StringOptions * stringOptions)
{
  PTRACE(4, "MixerEP\tMaking connection to \"" << party << '"');

  PSafePtr<OpalMixerNode> node;

  PWaitAndSignal mutex(m_infoMutex);

  // Specify mixer node to use after endpoint name (':') and delimit it with ';' and '@'
  PINDEX semicolon = party.Find(';');
  PString name = party(party.Find(':')+1, std::min(semicolon, party.Find('@'))-1);

  if (m_factoryNodeInfo != NULL && name == m_factoryNodeInfo->m_name) {
    PSafePtr<OpalConnection> connection = call.GetConnection(0);
    if (connection == NULL)
      return NULL; // Huh? A-Party!

    OpalMixerNodeInfo * info = m_factoryNodeInfo->Clone();
    info->m_name = GetNewFactoryName();
    if (info->m_name.IsEmpty() || (node = AddNode(info)) == NULL) {
      PTRACE(2, "MixerEP\tCannot make factory node.");
      return NULL;
    }

    node->SetOwnerConnection(connection->GetRemotePartyURL());

    PURL uri = connection->GetLocalPartyURL();
    uri.SetUserName(info->m_name);
    connection->ForwardCall(uri.AsString());
    return NULL;
  }

  if (name.IsEmpty() || name == "*") {
    if (m_adHocNodeInfo == NULL || m_adHocNodeInfo->m_name.IsEmpty()) {
      PTRACE(2, "MixerEP\tCannot make ad-hoc node for default alias");
      return NULL;
    }
    name = m_adHocNodeInfo->m_name;
  }

  node = FindNode(name);
  if (node == NULL && m_adHocNodeInfo != NULL) {
    OpalMixerNodeInfo * info = m_adHocNodeInfo->Clone();
    info->m_name = name;
    node = AddNode(info);
  }

  if (node == NULL) {
    PTRACE(2, "MixerEP\tNode alias \"" << party << "\" does not exist and cannot make ad-hoc node.");
    return NULL;
  }

  OpalConnection::StringOptions localStringOptions;
  if (semicolon != P_MAX_INDEX) {
    if (stringOptions == NULL)
      stringOptions = &localStringOptions;

    PStringToString params;
    PURL::SplitVars(party.Mid(semicolon), params, ';', '=');
    for (PStringToString::iterator it = params.begin(); it != params.end(); ++it)
      stringOptions->SetAt(it->first, it->second);
  }

  return AddConnection(CreateConnection(node, call, userData, options, stringOptions));
}


bool OpalMixerEndPoint::GetConferenceStates(OpalConferenceStates & states, const PString & name) const
{
  states.clear();

  if (name.IsEmpty()) {
    for (PSafePtr<OpalMixerNode> node(m_nodesByUID, PSafeReadOnly); node != NULL; ++node) {
      states.push_back(OpalConferenceState());
      node->GetConferenceState(states.back());
    }
  }
  else {
    PSafePtr<OpalMixerNode> node;
    if (name.NumCompare(GetPrefixName()+':') == EqualTo)
      node = m_nodesByUID.FindWithLock(name.Mid(GetPrefixName().GetLength()+1));
    else
      node = m_nodesByName.GetAt(name);

    if (node != NULL) {
      states.push_back(OpalConferenceState());
      node->GetConferenceState(states.back());
    }
  }

  return true;
}


PBoolean OpalMixerEndPoint::GarbageCollection()
{
  // Use bitwise | not boolean || so both get executed
  return OpalMixerNodeManager::GarbageCollection() | OpalEndPoint::GarbageCollection();
}


OpalMixerConnection * OpalMixerEndPoint::CreateConnection(PSafePtr<OpalMixerNode> node,
                                                                       OpalCall & call,
                                                                           void * userData,
                                                                         unsigned options,
                                                  OpalConnection::StringOptions * stringOptions)
{
  return new OpalMixerConnection(node, call, *this, userData, options, stringOptions);
}


void OpalMixerEndPoint::SetAdHocNodeInfo(const OpalMixerNodeInfo & info)
{
  SetAdHocNodeInfo(info.Clone());
}


void OpalMixerEndPoint::SetAdHocNodeInfo(OpalMixerNodeInfo * info)
{
  m_infoMutex.Wait();
  delete m_adHocNodeInfo;
  m_adHocNodeInfo = info;
  m_infoMutex.Signal();
}


void OpalMixerEndPoint::SetFactoryNodeInfo(const OpalMixerNodeInfo & info)
{
  SetFactoryNodeInfo(info.Clone());
}


void OpalMixerEndPoint::SetFactoryNodeInfo(OpalMixerNodeInfo * info)
{
  m_infoMutex.Wait();
  delete m_factoryNodeInfo;
  m_factoryNodeInfo = info;
  m_infoMutex.Signal();
}


PString OpalMixerEndPoint::GetNewFactoryName()
{
  if (m_factoryNodeInfo == NULL)
    return PString::Empty();

  return m_factoryNodeInfo->m_name + psprintf("%04u", ++m_factoryIndex);
}


PString OpalMixerEndPoint::CreateInternalURI(const PGloballyUniqueID & guid)
{
  return GetPrefixName() + ':' + guid.AsString();
}


void OpalMixerEndPoint::OnNodeStatusChanged(const OpalMixerNode & node, OpalConferenceState::ChangeType change)
{
  manager.OnConferenceStatusChanged(*this, CreateInternalURI(node.GetGUID()), change);
}


///////////////////////////////////////////////////////////////////////////////

OpalMixerConnection::OpalMixerConnection(PSafePtr<OpalMixerNode> node,
                                                      OpalCall & call,
                                             OpalMixerEndPoint & ep,
                                                          void * userData,
                                                        unsigned options,
                                 OpalConnection::StringOptions * stringOptions)
  : OpalLocalConnection(call, ep, userData, options, stringOptions, 'M')
  , m_endpoint(ep)
  , m_node(node)
  , m_listenOnly(node->GetNodeInfo().m_listenOnly)
{
  m_node->AttachConnection(this);

  const PStringSet & names = node->GetNames();
  if (names.IsEmpty())
    localPartyName = node->GetGUID().AsString();
  else
    localPartyName = *names.begin();

  PTRACE(4, "MixerCon\tConstructed");
}


OpalMixerConnection::~OpalMixerConnection()
{
  PTRACE(4, "MixerCon\tDestroyed");
}


void OpalMixerConnection::OnReleased()
{
  m_node->DetachConnection(this);
  OpalLocalConnection::OnReleased();
}


OpalMediaFormatList OpalMixerConnection::GetMediaFormats() const
{
  OpalMediaFormatList list = OpalTranscoder::GetPossibleFormats(OpalPCM16);
  list += OpalRFC2833;
#if OPAL_T38_CAPABILITY
  list += OpalCiscoNSE;
#endif

#if OPAL_VIDEO
  if (!m_node->GetNodeInfo().m_audioOnly)
    list += OpalTranscoder::GetPossibleFormats(OpalYUV420P);
#endif
  return list;
}


OpalMediaStream * OpalMixerConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                         unsigned sessionID,
                                                         PBoolean isSource)
{
  return new OpalMixerMediaStream(*this, mediaFormat, sessionID, isSource, m_node, m_listenOnly);
}


void OpalMixerConnection::OnStartMediaPatch(OpalMediaPatch & patch)
{
  OpalLocalConnection::OnStartMediaPatch(patch);
  m_node->UseMediaPassThrough(patch.GetSource().GetSessionID());
}


void OpalMixerConnection::OnApplyStringOptions()
{
  OpalLocalConnection::OnApplyStringOptions();
  SetListenOnly(m_stringOptions.GetBoolean(OPAL_OPT_LISTEN_ONLY, GetListenOnly()));
  if (m_stringOptions.GetBoolean(OPAL_OPT_CONF_OWNER))
    m_node->SetOwnerConnection(GetToken());
}


bool OpalMixerConnection::SendUserInputString(const PString & value)
{
  m_node->BroadcastUserInput(this, value);
  return true;
}


PBoolean OpalMixerConnection::SendUserInputTone(char tone, unsigned /*duration*/)
{
  m_node->BroadcastUserInput(this, tone);
  return true;
}


bool OpalMixerConnection::GetConferenceState(OpalConferenceState * state) const
{
  if (m_node == NULL)
    return false;

  if (state != NULL)
    m_node->GetConferenceState(*state);

  return true;
}


void OpalMixerConnection::SetListenOnly(bool listenOnly)
{
  PTRACE(3, "MixerCon\tSet listen only mode to " << (listenOnly ? "ON" : "OFF"));

  m_listenOnly = listenOnly;

  for (PSafePtr<OpalMediaStream> mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    OpalMixerMediaStream * mixerStream = dynamic_cast<OpalMixerMediaStream *>(&*mediaStream);
    if (mixerStream != NULL && mixerStream->IsSink()) {
      mixerStream->SetPaused(listenOnly);
      if (listenOnly)
        m_node->DetachStream(mixerStream);
      else
        m_node->AttachStream(mixerStream);
    }
  }
}


///////////////////////////////////////////////////////////////////////////////

OpalMixerMediaStream::OpalMixerMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & format,
                                                        unsigned sessionID,
                                                            bool isSource,
                                         PSafePtr<OpalMixerNode> node,
                                                            bool listenOnly)
  : OpalMediaStream(conn, format, sessionID, isSource)
  , m_node(node)
  , m_listenOnly(listenOnly)
#if OPAL_VIDEO
  , m_video(mediaFormat.GetMediaType() == OpalMediaType::Video())
#endif
{
  /* We are a bit sneaky here. OpalCall::OpenSourceMediaStream will have
     selected the network codec (e.g. G.723.1) anbd passed it to us, but for
     the case of incoming media to the mixer (sink), we switch it to the raw
     codec type so the OpalPatch system creates the codec for us. With the
     transmitter (source) we keep the required media format so the mixed data
     thread can cache and optimise an encoded frame across multiple remote
     connections. */
  if (IsSink()) {
#if OPAL_VIDEO
    if (m_video)
      mediaFormat = OpalYUV420P;
    else
#endif
      mediaFormat = OpalPCM16;
  }
}


OpalMixerMediaStream::~OpalMixerMediaStream()
{
  Close();
}


PBoolean OpalMixerMediaStream::Open()
{
  if (m_isOpen)
    return true;

  if (mediaFormat.GetMediaType() != OpalMediaType::Audio()
#if OPAL_VIDEO
   && mediaFormat.GetMediaType() != OpalMediaType::Video()
#endif
  ) {
    PTRACE(3, "MixerStrm\tCannot open media stream of type " << mediaFormat.GetMediaType());
    return false;
  }

  SetPaused(IsSink() && m_listenOnly);

  if (!IsPaused() && !m_node->AttachStream(this))
    return false;

  return OpalMediaStream::Open();
}


void OpalMixerMediaStream::InternalClose()
{
  m_node->DetachStream(this);
}


PBoolean OpalMixerMediaStream::WritePacket(RTP_DataFrame & packet)
{
#if OPAL_VIDEO
  if (m_video)
    return m_node->WriteVideo(GetID(), packet);
#endif

  return m_node->WriteAudio(GetID(), packet);
}


PBoolean OpalMixerMediaStream::IsSynchronous() const
{
  return false;
}


PBoolean OpalMixerMediaStream::RequiresPatchThread() const
{
  return IsSink();
}


bool OpalMixerMediaStream::EnableJitterBuffer(bool enab) const
{
  if (IsSource())
    return false;

  unsigned minJitter, maxJitter;

  if (enab) {
    minJitter = connection.GetMinAudioJitterDelay();
    maxJitter = connection.GetMaxAudioJitterDelay();
  }
  else {
    minJitter = 0;
    maxJitter = 0;
  }

  return m_node->SetJitterBufferSize(GetID(), minJitter, maxJitter);
}


///////////////////////////////////////////////////////////////////////////////

OpalMixerNode::OpalMixerNode(OpalMixerNodeManager & manager,
                                OpalMixerNodeInfo * info)
  : m_manager(manager)
  , m_info(info != NULL ? info : new OpalMixerNodeInfo)
  , m_audioMixer(*m_info)
#if OPAL_VIDEO
  , m_videoMixer(*m_info)
#endif
{
  m_connections.DisallowDeleteObjects();

  AddName(m_info->m_name);

  PTRACE(4, "MixerNode\tConstructed " << *this);
}


OpalMixerNode::~OpalMixerNode()
{
  ShutDown(); // Fail safe

  delete m_info;
  PTRACE(4, "MixerNode\tDestroyed " << *this);
}


void OpalMixerNode::ShutDown()
{
  PTRACE(4, "MixerNode\tShutting down " << *this);

  m_manager.OnNodeStatusChanged(*this, OpalConferenceState::Destroyed);

  for (PSafePtr<OpalConnection> connection = GetFirstConnection(); connection != NULL; ++connection)
    connection->Release();

  while (GetConnectionCount() > 0)
    PThread::Sleep(100);

  m_audioMixer.RemoveAllStreams();
#if OPAL_VIDEO
  m_videoMixer.RemoveAllStreams();
#endif
  m_manager.RemoveNodeNames(GetNames());
  m_names.RemoveAll();
}


void OpalMixerNode::PrintOn(ostream & strm) const
{
  char prevfill = strm.fill();
  strm << m_guid << " (" << setfill(',') << m_names << ')' << setfill(prevfill);
}


void OpalMixerNode::AddName(const PString & name)
{
  if (name.IsEmpty())
    return;

  if (m_names.Contains(name)) {
    PTRACE(4, "MixerNode\tName \"" << name << "\" already added to " << *this);
    return;
  }

  PTRACE(4, "MixerNode\tAdding name \"" << name << "\" to " << *this);
  m_names += name;
  m_manager.AddNodeName(name, this);
}


void OpalMixerNode::RemoveName(const PString & name)
{
  if (name.IsEmpty())
    return;

  PStringSet::iterator it = m_names.find(name);
  if (it == m_names.end())
    PTRACE(4, "MixerNode\tName \"" << name << "\" not present in " << *this);
  else {
    PTRACE(4, "MixerNode\tRemoving name \"" << name << "\" from " << *this);
    m_names.erase(it);
    m_manager.RemoveNodeName(name);
  }
}


void OpalMixerNode::AttachConnection(OpalConnection * connection)
{
  if (PAssertNULL(connection) == NULL)
    return;

  m_connections.Append(connection);

  UseMediaPassThrough(0);

  m_manager.OnNodeStatusChanged(*this, OpalConferenceState::UserAdded);
}


void OpalMixerNode::DetachConnection(OpalConnection * connection)
{
  if (PAssertNULL(connection) == NULL)
    return;

  if (m_connections.Remove(connection))
    UseMediaPassThrough(0, connection);

  m_manager.OnNodeStatusChanged(*this, OpalConferenceState::UserRemoved);

  if (!m_ownerConnection.IsEmpty() &&
      (m_ownerConnection == connection->GetToken() ||
       m_ownerConnection == connection->GetLocalPartyURL() ||
       m_ownerConnection == connection->GetRemotePartyURL()))
    ShutDown();
}


bool OpalMixerNode::AttachStream(OpalMixerMediaStream * stream)
{
  PTRACE(4, "MixerNode\tAttaching " << stream->GetMediaFormat()
         << ' ' << (stream->IsSource() ? "source" : "sink")
         << " stream with id " << stream->GetID() << " to " << *this);

#if OPAL_VIDEO
  if (stream->GetMediaFormat().GetMediaType() == OpalMediaType::Video()) {
    if (stream->IsSink())
      return m_videoMixer.AddStream(stream->GetID());
    m_videoMixer.m_outputStreams.Append(stream);
    return true;
  }
#endif

  if (stream->IsSink())
    return m_audioMixer.AddStream(stream->GetID());

  m_audioMixer.m_outputStreams.Append(stream);
  return true;
}


void OpalMixerNode::DetachStream(OpalMixerMediaStream * stream)
{
  PTRACE(4, "MixerNode\tDetaching " << stream->GetMediaFormat()
         << ' ' << (stream->IsSource() ? "source" : "sink")
         << " stream with id " << stream->GetID() << " from " << *this);

#if OPAL_VIDEO
  if (stream->GetMediaFormat().GetMediaType() == OpalMediaType::Video()) {
    if (stream->IsSource())
      m_videoMixer.m_outputStreams.Remove(stream);
    else
      m_videoMixer.RemoveStream(stream->GetID());
    return;
  }
#endif

  if (stream->IsSource())
    m_audioMixer.m_outputStreams.Remove(stream);
  else
    m_audioMixer.RemoveStream(stream->GetID());
}


void OpalMixerNode::UseMediaPassThrough(unsigned sessionID, OpalConnection * connection)
{
  if (!m_info->m_mediaPassThru)
    return;

  PSafePtr<OpalConnection> other2;

  if (connection != NULL && m_connections.GetSize() == 1)
    other2 = connection->GetOtherPartyConnection();
  else {
    if (m_connections.GetSize() < 2)
      return;

    PSafePtr<OpalConnection> connection2 = m_connections.GetAt(1, PSafeReference);
    if (connection2 == NULL)
      return;

    other2 = connection2->GetOtherPartyConnection();
  }
  if (other2 == NULL)
    return;

  PSafePtr<OpalConnection> connection1 = m_connections.GetAt(0, PSafeReference);
  if (connection1 == NULL)
    return;

  PSafePtr<OpalConnection> other1 = connection1->GetOtherPartyConnection();
  if (other1 == NULL)
    return;

  OpalManager::SetMediaPassThrough(*other1, *other2, m_connections.GetSize() == 2, sessionID);
}


void OpalMixerNode::BroadcastUserInput(const OpalConnection * connection, const PString & value)
{
  for (PSafePtr<OpalConnection> conn(m_connections, PSafeReference); conn != NULL; ++conn) {
    if (connection != conn)
      conn->GetEndPoint().GetManager().QueueDecoupledEvent(
            new PSafeWorkArg1<OpalConnection, PString>(conn, value, &OpalConnection::OnUserInputStringCallback));
  }
}


void OpalMixerNode::GetConferenceState(OpalConferenceState & state) const
{
  state.m_internalURI = m_manager.CreateInternalURI(m_guid);
  state.m_displayText = m_info->m_displayText.IsEmpty() ? m_info->m_name : m_info->m_displayText;
  state.m_subject     = m_info->m_subject;
  state.m_notes       = m_info->m_notes;
  state.m_keywords    = m_info->m_keywords;

  PStringList uriList;
  for (PStringSet::const_iterator alias = m_names.begin(); alias != m_names.end(); ++alias)
    uriList += m_manager.GetManager().GetNetworkURIs(*alias);

  for (PStringList::iterator it = uriList.begin(); it != uriList.end(); ++it) {
    OpalConferenceState::URI newURI;
    newURI.m_uri = *it;
    newURI.m_purpose = "participation";
    state.m_accessURI.push_back(newURI);
  }

  for (PSafePtr<OpalConnection> conn(m_connections, PSafeReference); conn != NULL; ++conn) {
    PSafePtr<OpalConnection> other = conn->GetOtherPartyConnection();
    if (other != NULL && other->IsNetworkConnection()) {
      OpalConferenceState::User user;
      user.m_uri = other->GetRemotePartyURL();
      user.m_displayText = other->GetRemotePartyName();
      user.m_roles += "participant";
      state.m_users.push_back(user);
    }
  }
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_SIP

static void OutputIfNotEmpty(ostream & xml, const PString & str, const char * tag, int indent)
{
  if (!str.IsEmpty())
    xml << setw(indent+1) << '<' << tag << '>' << str << "</" << tag << ">\n";
}

static void OutputURIs(ostream & xml, const OpalConferenceState::URIs & uris, const char * tag)
{
  if (uris.empty())
    return;

  xml << "    <" << tag << ">\n";
  for (OpalConferenceState::URIs::const_iterator uri = uris.begin(); uri != uris.end(); ++uri) {
    if (!uri->m_uri.IsEmpty()) {
      xml << "      <entry>\n"
             "        <uri>" << uri->m_uri << "</uri>\n";
      OutputIfNotEmpty(xml, uri->m_displayText, "display-text", 8);
      OutputIfNotEmpty(xml, uri->m_purpose, "purpose", 8);
      xml << "      </entry>\n";
    }
  }
  xml << "    </" << tag << ">\n";
}


// This package is on for backward compatibility, presence should now use the
// the OpalPresence classes to manage SIP presence.
class SIPConferenceEventPackageHandler : public SIPEventPackageHandler
{
  unsigned m_expectedSequenceNumber;
  unsigned m_txSequenceNumber;

public:
  SIPConferenceEventPackageHandler()
    : m_expectedSequenceNumber(0)
    , m_txSequenceNumber(0)
  {
  }

  virtual PCaselessString GetContentType() const
  {
    return "application/conference-info+xml";
  }

  virtual void OnReceivedNOTIFY(SIPSubscribe::NotifyCallbackInfo & 
#if P_EXPAT
    notifyInfo
#endif
    )
  {
    PTRACE(4, "SIP\tProcessing conference NOTIFY");

    OpalConferenceState state;

#if P_EXPAT
    static PXML::ValidationInfo const ConferenceInfoValidation[] = {
      { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:conference-info" },
      { PXML::ElementName,                "conference-info", },
      { PXML::RequiredNonEmptyAttribute,  "version"},
      { PXML::RequiredAttributeWithValue, "state",   { "full\npartial" } },

      { PXML::EndOfValidationList }
    };
    if (!notifyInfo.LoadAndValidate(state.m_xml, ConferenceInfoValidation))
      return;

    unsigned newSequenceNumber = state.m_xml.GetRootElement()->GetAttribute("version").AsUnsigned();
    if (m_expectedSequenceNumber > 0 && m_expectedSequenceNumber != newSequenceNumber)
      return;
    m_expectedSequenceNumber = newSequenceNumber+1;
#endif
  }

  virtual PString OnSendNOTIFY(SIPHandler & handler, const PObject * body)
  {
    const OpalConferenceState * state = dynamic_cast<const OpalConferenceState *>(body);
    if (state == NULL)
      return PString::Empty();

#if P_EXPAT
    if (state->m_xml.IsLoaded())
      return state->m_xml.AsString();
#endif

    PStringStream xml;

    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<conference-info xmlns=\"urn:ietf:params:xml:ns:conference-info\"\n"
            "                 entity=\"" << handler.GetAddressOfRecord() << "\"\n"
            "                 state=\"full\"\n"
            "                 version=\"" << ++m_txSequenceNumber << "\">\n";

    xml << "  <conference-description>\n";
    OutputIfNotEmpty(xml, state->m_displayText, "display-text", 4);
    OutputIfNotEmpty(xml, state->m_subject, "subject", 4);
    OutputIfNotEmpty(xml, state->m_notes, "free-text", 4);
    OutputIfNotEmpty(xml, state->m_keywords, "keywords", 4);
    OutputURIs(xml, state->m_accessURI, "conf-uris");
    OutputURIs(xml, state->m_serviceURI, "service-uris");
    if (state->m_maxUsers > 0)
      xml << "    <maximum-user-count>" << state->m_maxUsers << "</maximum-user-count>\n";
    xml << "  </conference-description>\n";

    xml << "  <conference-state>\n"
            "    <user-count>" << state->m_users.size() << "</user-count>\n"
            "    <active>" << (state->m_active ? "true" : "false") << "</active>\n"
            "    <locked>" << (state->m_locked ? "true" : "false") << "</locked>\n"
            "  </conference-state>\n";

    if (!state->m_users.empty()) {
      xml << "  <users>\n";
      for (OpalConferenceState::Users::const_iterator user = state->m_users.begin(); user != state->m_users.end(); ++user) {
        xml << "    <user entity=\"" << user->m_uri << "\" state=\"full\">\n";
        OutputIfNotEmpty(xml, user->m_displayText, "display-text", 6);
        if (!user->m_roles.IsEmpty()) {
          xml << "      <roles>\n";
          for (PStringSet::const_iterator role = user->m_roles.begin(); role != user->m_roles.end(); ++role)
            OutputIfNotEmpty(xml, *role, "entry", 8);
          xml << "      </roles>\n";
        }
        xml << "    </user>\n";
      }
      xml << "  </users>\n";
    }

    xml << "</conference-info>";

    return xml;
  }
};

static SIPEventPackageFactory::Worker<SIPConferenceEventPackageHandler> conferenceEventPackageHandler(SIPSubscribe::Conference);

#endif // OPAL_SIP


///////////////////////////////////////////////////////////////////////////////

OpalMixerNode::MediaMixer::MediaMixer()
{
  m_outputStreams.DisallowDeleteObjects();
}


///////////////////////////////////////////////////////////////////////////////

OpalMixerNode::AudioMixer::AudioMixer(const OpalMixerNodeInfo & info)
  : OpalAudioMixer(false, info.m_sampleRate)
#if OPAL_MIXER_AUDIO_DEBUG
  , m_audioDebug(new PAudioMixerDebug(info.m_name))
#endif
{
}


OpalMixerNode::AudioMixer::~AudioMixer()
{
  StopPushThread();
}


void OpalMixerNode::AudioMixer::PushOne(PSafePtr<OpalMixerMediaStream> & stream,
                                        CachedAudio & cache,
                                        const short * audioToSubtract)
{
  MIXER_DEBUG_OUT(stream->GetID() << ',');

  switch (cache.m_state) {
    case CachedAudio::Collecting :
      MixAdditive(cache.m_raw, audioToSubtract);
      cache.m_state = CachedAudio::Collected;
      m_mutex.Signal();
      break;

    case CachedAudio::Collected :
      m_mutex.Signal();
      MIXER_DEBUG_OUT(",,,");
      return;

    case CachedAudio::Completed :
      m_mutex.Signal();
      MIXER_DEBUG_OUT(cache.m_encoded.GetPayloadType() << ','
          << cache.m_encoded.GetTimestamp() << ','
          << cache.m_encoded.GetPayloadSize() << ',');
      stream.SetSafetyMode(PSafeReference); // OpalMediaStream::PushPacket might block
      stream->PushPacket(cache.m_encoded);
      stream.SetSafetyMode(PSafeReadOnly); // restore lock
      return;
  }

  OpalMediaFormat mediaFormat = stream->GetMediaFormat();
  if (mediaFormat == OpalPCM16) {
    if (cache.m_raw.GetPayloadSize() < stream->GetDataSize()) {
      MIXER_DEBUG_OUT(','
                   << cache.m_raw.GetTimestamp() << ','
                   << cache.m_raw.GetPayloadSize() << ',');
      return;
    }

    cache.m_state = CachedAudio::Completed;
    MIXER_DEBUG_OUT("PCM-16,"
        << cache.m_raw.GetTimestamp() << ','
        << cache.m_raw.GetPayloadSize() << ',');
    MIXER_DEBUG_WAV(stream->GetID(), cache.m_raw);
    stream.SetSafetyMode(PSafeReference); // OpalMediaStream::PushPacket might block
    stream->PushPacket(cache.m_raw);
    stream.SetSafetyMode(PSafeReadOnly); // restore lock
    return;
  }

  if (cache.m_transcoder == NULL) {
    cache.m_transcoder = OpalTranscoder::Create(OpalPCM16, mediaFormat);
    if (cache.m_transcoder == NULL) {
      PTRACE(2, "MixerNode\tCould not create transcoder to "
             << mediaFormat << " for stream id " << stream->GetID());
      stream->Close();
      return;
    }
  }

  if (cache.m_raw.GetPayloadSize() < cache.m_transcoder->GetOptimalDataFrameSize(true)) {
    MIXER_DEBUG_OUT(','
                 << cache.m_raw.GetTimestamp() << ','
                 << cache.m_raw.GetPayloadSize() << ',');
    return;
  }

  if (cache.m_encoded.SetPayloadSize(cache.m_transcoder->GetOptimalDataFrameSize(false)) &&
      cache.m_transcoder->Convert(cache.m_raw, cache.m_encoded)) {
    cache.m_encoded.SetPayloadType(cache.m_transcoder->GetPayloadType(false));
    cache.m_encoded.SetTimestamp(cache.m_raw.GetTimestamp());
    cache.m_state = CachedAudio::Completed;
    MIXER_DEBUG_OUT(cache.m_encoded.GetPayloadType() << ','
        << cache.m_encoded.GetTimestamp() << ','
        << cache.m_encoded.GetPayloadSize() << ',');
    MIXER_DEBUG_WAV(stream->GetID(), cache.m_raw);
    stream.SetSafetyMode(PSafeReference); // OpalMediaStream::PushPacket might block
    stream->PushPacket(cache.m_encoded);
    stream.SetSafetyMode(PSafeReadOnly); // restore lock
  }
  else {
    PTRACE(2, "MixerNode\tCould not convert audio to "
           << mediaFormat << " for stream id " << stream->GetID());
    stream->Close();
  }
}


bool OpalMixerNode::AudioMixer::OnPush()
{
  MIXER_DEBUG_OUT(PTimer::Tick().GetMilliSeconds() << ',' << m_outputTimestamp << ',');

  m_mutex.Wait();
  PreMixStreams();
  m_mutex.Signal();

  for (PSafePtr<OpalMixerMediaStream> stream(m_outputStreams, PSafeReadOnly); stream != NULL; ++stream) {
    m_mutex.Wait(); // Signal() call for this mutex is inside PushOne()

    // Check for full participant, so can subtract their signal
    StreamMap_T::iterator inputStream = m_inputStreams.find(stream->GetID());
    if (inputStream != m_inputStreams.end())
      PushOne(stream, m_cache[stream->GetID()], ((AudioStream *)inputStream->second)->m_cacheSamples);
    else {
      // Listen only participant, can use cached encoded audio
      PString encodedFrameKey = stream->GetMediaFormat();
      encodedFrameKey.sprintf(":%u", stream->GetDataSize());
      PushOne(stream, m_cache[encodedFrameKey], NULL);
    }
  }

  for (std::map<PString, CachedAudio>::iterator iterCache = m_cache.begin(); iterCache != m_cache.end(); ++iterCache) {
    switch (iterCache->second.m_state) {
      case CachedAudio::Collected :
        iterCache->second.m_state = CachedAudio::Collecting;
        break;

      case CachedAudio::Completed :
        iterCache->second.m_raw.SetPayloadSize(0);
        iterCache->second.m_encoded.SetPayloadSize(0);
        iterCache->second.m_state = CachedAudio::Collecting;
        break;

      default :
        break;
    }
  }

  MIXER_DEBUG_OUT(endl);

  m_outputTimestamp += m_periodTS;

  return true;
}


OpalMixerNode::AudioMixer::CachedAudio::CachedAudio()
  : m_state(Collecting)
  , m_transcoder(NULL)
{
}


OpalMixerNode::AudioMixer::CachedAudio::~CachedAudio()
{
  delete m_transcoder;
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO
OpalMixerNode::VideoMixer::VideoMixer(const OpalMixerNodeInfo & info)
  : OpalVideoMixer(info.m_style, info.m_width, info.m_height, info.m_rate)
{
}


OpalMixerNode::VideoMixer::~VideoMixer()
{
  StopPushThread();
}


bool OpalMixerNode::VideoMixer::OnMixed(RTP_DataFrame * & output)
{
  std::map<PString, RTP_DataFrameList> cachedVideo;
  std::map<PString, RTP_DataFrame>     cachedFrameStore;

  for (PSafePtr<OpalMixerMediaStream> stream(m_outputStreams, PSafeReadOnly); stream != NULL; ++stream) {
    OpalMediaFormat mediaFormat = stream->GetMediaFormat();
    if (mediaFormat == OpalYUV420P) {
      stream.SetSafetyMode(PSafeReference); // OpalMediaStream::PushPacket might block
      stream->PushPacket(*output);
      stream.SetSafetyMode(PSafeReadOnly); // restore lock
    }
    else {
      unsigned width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption());
      unsigned height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption());

      PStringStream key;
      key << mediaFormat << width << height;

      if (cachedVideo.find(key) == cachedVideo.end()) {
        OpalTranscoder * transcoder = m_transcoders.GetAt(key);
        if (transcoder == NULL) {
          transcoder = OpalTranscoder::Create(OpalYUV420P, mediaFormat);
          if (transcoder == NULL) {
            PTRACE(2, "MixerNode\tCould not create transcoder to "
                   << mediaFormat << " for stream id " << stream->GetID());
            stream->Close();
            continue;
          }
          PTRACE(3, "MixerNode\tCreated transcoder to "
                 << mediaFormat << " for stream id " << stream->GetID());
          m_transcoders.SetAt(key, transcoder);
        }

        const RTP_DataFrame * rawRTP;
        const OpalVideoTranscoder::FrameHeader * header = (const OpalVideoTranscoder::FrameHeader *)output->GetPayloadPtr();
        if (header->width == width && header->height == height)
          rawRTP = output;
        else {
          if (cachedFrameStore.find(key) == cachedFrameStore.end()) {
            RTP_DataFrame & store = cachedFrameStore[key];
            store.SetPayloadSize(width*height*3/2+sizeof(OpalVideoTranscoder::FrameHeader));
            OpalVideoTranscoder::FrameHeader * resized = (OpalVideoTranscoder::FrameHeader *)store.GetPayloadPtr();
            resized->width = width;
            resized->height = height;
            PColourConverter::CopyYUV420P(0, 0, header->width, header->height,
                                            header->width, header->height, OPAL_VIDEO_FRAME_DATA_PTR(header),
                                          0, 0, width, height,
                                          width, height, OPAL_VIDEO_FRAME_DATA_PTR(resized),
                                          PVideoFrameInfo::eScale);
          }
          rawRTP = &cachedFrameStore[key];
        }

        if (!transcoder->ConvertFrames(*rawRTP, cachedVideo[key])) {
          PTRACE(2, "MixerNode\tCould not convert video to "
                 << mediaFormat << " for stream id " << stream->GetID());
          stream->Close();
          continue;
        }
      }

      stream.SetSafetyMode(PSafeReference); // OpalMediaStream::PushPacket might block

      RTP_DataFrameList & list = cachedVideo[key];
      for (RTP_DataFrameList::iterator frame = list.begin(); frame != list.end(); ++frame)
        stream->PushPacket(*frame);

      stream.SetSafetyMode(PSafeReadOnly); // restore lock
    }
  }

  return true;
}
#endif


//////////////////////////////////////////////////////////////////////////////


OpalMixerNodeManager::OpalMixerNodeManager(OpalManager & manager)
  : m_manager(manager)
{
  m_nodesByName.DisallowDeleteObjects();
}


OpalMixerNodeManager::~OpalMixerNodeManager()
{
  ShutDown(); // just in case
}


void OpalMixerNodeManager::ShutDown()
{
  PTRACE(4, "Mixer\tDestroying " << m_nodesByUID.GetSize() << ' ' << m_nodesByName.GetSize() << " nodes");
  while (!m_nodesByUID.IsEmpty()) {
    PSafePtr<OpalMixerNode> node = m_nodesByUID.GetAt(0);
    node->ShutDown();
    m_nodesByUID.RemoveAt(node->GetGUID());
  }
  GarbageCollection();
}


PBoolean OpalMixerNodeManager::GarbageCollection()
{
  return m_nodesByUID.DeleteObjectsToBeRemoved();
}


OpalMixerNode * OpalMixerNodeManager::CreateNode(OpalMixerNodeInfo * info)
{
  return new OpalMixerNode(*this, info);
}


PSafePtr<OpalMixerNode> OpalMixerNodeManager::AddNode(OpalMixerNodeInfo * info)
{
  PSafePtr<OpalMixerNode> node(CreateNode(info), PSafeReference);
  if (node == NULL)
    delete info;
  else {
    m_nodesByUID.SetAt(node->GetGUID(), node);
    PTRACE(3, "MixerEP\tAdded new node, id=" << node->GetGUID());
    OnNodeStatusChanged(*node, OpalConferenceState::Created);
  }

  return node;
}


void OpalMixerNodeManager::AddNode(OpalMixerNode * node)
{
  if (node != NULL)
    m_nodesByUID.SetAt(node->GetGUID(), node);
}


PSafePtr<OpalMixerNode> OpalMixerNodeManager::FindNode(const PString & name, PSafetyMode mode)
{
  PGloballyUniqueID guid(name);
  if (!guid.IsNULL())
    return m_nodesByUID.FindWithLock(guid, mode);

  return PSafePtr<OpalMixerNode>(m_nodesByName.GetAt(name), mode);
}


void OpalMixerNodeManager::RemoveNode(OpalMixerNode & node)
{
  node.ShutDown();
  m_nodesByUID.RemoveAt(node.GetGUID());
}


void OpalMixerNodeManager::AddNodeName(PString name,
                               OpalMixerNode * node)
{
  m_nodesByName.SetAt(name, node);
}


void OpalMixerNodeManager::RemoveNodeName(PString name)
{
  m_nodesByName.RemoveAt(name);
}


void OpalMixerNodeManager::RemoveNodeNames(const PStringSet & names)
{
  for (PStringSet::const_iterator it = names.begin(); it != names.end(); ++it)
    m_nodesByName.RemoveAt(*it);
}


PString OpalMixerNodeManager::CreateInternalURI(const PGloballyUniqueID & guid)
{
  return "mixer:" + guid.AsString();
}


void OpalMixerNodeManager::OnNodeStatusChanged(const OpalMixerNode &, OpalConferenceState::ChangeType)
{
}


#else

  #ifdef _MSC_VER
    #pragma message("Mixer (MCU) support DISABLED")
  #endif

#endif // OPAL_HAS_MIXER


//////////////////////////////////////////////////////////////////////////////
