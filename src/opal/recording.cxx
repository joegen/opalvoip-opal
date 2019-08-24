/*
 * recording.cxx
 *
 * OPAL call recording
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2009 Post Increment
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
 * Contributor(s): ______________________________________.
 *
 */


#include <ptlib.h>

#include <opal_config.h>

#if defined(OPAL_HAS_MIXER) && defined(P_WAVFILE)

#include <opal/recording.h>

#include <ep/opalmixer.h>
#include <ptclib/mediafile.h>


#define PTraceModule() "OpalRecord"


//////////////////////////////////////////////////////////////////////////////

bool OpalRecordManager::Open(const PFilePath & fn)
{
  return OpenFile(fn);
}


bool OpalRecordManager::Open(const PFilePath & fn, bool mono)
{
  m_options.m_stereo = !mono;
  return OpenFile(fn);
}


bool OpalRecordManager::Open(const PFilePath & fn, const Options & options)
{
  m_options = options;
  return Open(fn);
}


//////////////////////////////////////////////////////////////////////////////

/** This class manages the recording of OPAL calls to files supported by PMediaFile.
  */
class OpalMediaFileRecordManager : public OpalRecordManager
{
  public:
    OpalMediaFileRecordManager();
    ~OpalMediaFileRecordManager();

    virtual bool OpenFile(const PFilePath & fn);
    virtual bool IsOpen() const;
    virtual bool Close();
    virtual bool OpenStream(const PString & strmId, const OpalMediaFormat & format);
    virtual bool CloseStream(const PString & strmId);

    struct FactoryInitialiser : OpalRecordManager::Factory::WorkerBase
    {
      FactoryInitialiser();
      virtual OpalRecordManager * Create(OpalRecordManager::Factory::Param_T) const;
    };

  protected:
    mutable PDECLARE_MUTEX(m_mutex);
    PMediaFile * m_file;

    // Audio
    virtual bool WriteAudio(const PString & strmId, const RTP_DataFrame & rtp);
    virtual bool OnPushAudio();
    virtual unsigned GetPushAudioPeriodMS() const;
    // Callback from OpalAudioMixer
    virtual bool OnMixedAudio(const RTP_DataFrame & frame);

    struct AudioMixer : public OpalAudioMixer
    {
      AudioMixer(
        OpalMediaFileRecordManager & manager,
        bool stereo,
        unsigned sampleRate,
        bool pushThread
      ) : OpalAudioMixer(stereo, sampleRate, pushThread)
        , m_manager(manager)
      { }
      ~AudioMixer() { StopPushThread(); }
      virtual bool OnMixed(RTP_DataFrame * & output) { return m_manager.OnMixedAudio(*output); }
      OpalMediaFileRecordManager & m_manager;
    };
    PSmartPtr<AudioMixer> m_audioMixer;
    unsigned m_audioTrack;

#if OPAL_VIDEO
    virtual bool WriteVideo(const PString & strmId, const RTP_DataFrame & rtp);
    virtual bool OnPushVideo();
    virtual unsigned GetPushVideoPeriodMS() const;

    // Callback from OpalVideoMixer
    virtual bool OnMixedVideo(const RTP_DataFrame & frame);

    struct VideoMixer : public OpalVideoMixer
    {
      VideoMixer(
        OpalMediaFileRecordManager & manager,
        OpalVideoMixer::Styles style,
        unsigned width,
        unsigned height,
        unsigned rate,
        bool pushThread
      ) : OpalVideoMixer(style, width, height, rate, pushThread)
        , m_manager(manager)
      { }
      ~VideoMixer() { StopPushThread(); }
      virtual bool OnMixed(RTP_DataFrame * & output) { return m_manager.OnMixedVideo(*output); }
      OpalMediaFileRecordManager & m_manager;
    };
    PSmartPtr<VideoMixer> m_videoMixer;
    unsigned m_videoTrack;
#endif
};


OpalMediaFileRecordManager::OpalMediaFileRecordManager()
  : m_file(NULL)
  , m_audioTrack(numeric_limits<unsigned>::max())
#if OPAL_VIDEO
  , m_videoTrack(numeric_limits<unsigned>::max())
#endif
{
}


OpalMediaFileRecordManager::~OpalMediaFileRecordManager()
{
  Close();
}


bool OpalMediaFileRecordManager::OpenFile(const PFilePath & fn)
{
  PWaitAndSignal mutex(m_mutex);

  if (m_file != NULL) {
    PTRACE(2, "Cannot open mixer after it has started.");
    return false;
  }

  m_file = PMediaFile::Create(fn);
  if (m_file == NULL)
    return false;

  PTRACE_CONTEXT_ID_TO(*m_file);

  if (!m_file->OpenForWriting(fn)) {
    PTRACE(2, "Cannot open media file for writing: " << m_file->GetErrorText());
    delete m_file;
    m_file = NULL;
    return false;
  }

  m_audioMixer = new AudioMixer(*this,
                                m_options.m_stereo,
                                8000, // Really need to make this more flexible ....
                                m_options.m_pushThreads);
  PTRACE_CONTEXT_ID_TO(*m_audioMixer);

#if OPAL_VIDEO
  OpalVideoMixer::Styles style;
  switch (m_options.m_videoMixing) {
    default :
      PAssertAlways(PInvalidParameter);
    case eSideBySideScaled :
      m_options.m_videoWidth *= 2;
      style = OpalVideoMixer::eSideBySideScaled;
      break;

    case eSideBySideLetterbox :
      style = OpalVideoMixer::eSideBySideLetterbox;
      break;

    case eStackedScaled :
      m_options.m_videoHeight *= 2;
      style = OpalVideoMixer::eStackedScaled;
      break;

    case eStackedPillarbox :
      style = OpalVideoMixer::eStackedPillarbox;
      break;
  }

  m_videoMixer = new VideoMixer(*this,
                                style,
                                m_options.m_videoWidth,
                                m_options.m_videoHeight,
                                m_options.m_videoRate,
                                m_options.m_pushThreads);
  PTRACE_CONTEXT_ID_TO(*m_videoMixer);

  PTRACE(4, (m_options.m_stereo ? "Stereo" : "Mono") << "-PCM/"
         << m_options.m_videoFormat << "-Video mixers opened for file \"" << fn << '"');
#else
  PTRACE(4, (m_options.m_stereo ? "Stereo" : "Mono") << "-PCM mixer opened for file \"" << fn << '"');
#endif // OPAL_VIDEO
  return true;
}


bool OpalMediaFileRecordManager::IsOpen() const
{
  return m_file != NULL;
}


bool OpalMediaFileRecordManager::Close()
{
  m_mutex.Wait();

  // Deleted when out of scope
  PSmartPtr<AudioMixer> audioMixer = m_audioMixer;
  m_audioMixer = NULL;

#if OPAL_VIDEO
  PSmartPtr<VideoMixer> videoMixer = m_videoMixer;
  m_videoMixer = NULL;
#endif

  delete m_file;
  m_file = NULL;

  m_mutex.Signal();

  return true;
}


bool OpalMediaFileRecordManager::OpenStream(const PString & strmId, const OpalMediaFormat & format)
{
  PWaitAndSignal mutex(m_mutex);

  OpalMediaType mediaType = format.GetMediaType();

  unsigned trackId;
  PString outputFormat;
  OpalBaseMixer * mixer;

  if (mediaType == OpalMediaType::Audio()) {
    trackId = m_audioTrack;
    outputFormat = m_options.m_audioFormat;
    mixer = m_audioMixer;
  }
#if OPAL_VIDEO
  else if (mediaType == OpalMediaType::Video()) {
    trackId = m_videoTrack;
    outputFormat = m_options.m_videoFormat;
    mixer = m_videoMixer;
  }
#endif
  else {
    PTRACE(2, "Unsupported media type " << mediaType);
    return false;
  }

  if (mixer == NULL)
    return false;

  if (trackId < m_file->GetTrackCount()) {
    PTRACE(4, "Added stream " << strmId << " to existing " << mediaType << " track number " << trackId);
    return mixer->AddStream(strmId);
  }

  PTRACE(4, "Creating media file track for " << mediaType << ": stream format=" << format << ","
            " file format=\"" << outputFormat << "\", id=" << strmId);

  PMediaFile::TracksInfo tracks;
  if (!m_file->GetTracks(tracks))
    return false;

  trackId = tracks.size();

  if (!outputFormat.IsEmpty())
    tracks.push_back(PMediaFile::TrackInfo(mediaType, outputFormat));
  else {
    PMediaFile::TrackInfo trackInfo;
    if (!m_file->GetDefaultTrackInfo(mediaType, trackInfo))
      return false;
    tracks.push_back(trackInfo);
  }

  PMediaFile::TrackInfo & track = tracks[trackId];

#if OPAL_VIDEO
  if (mediaType == OpalMediaType::Video()) {
    track.m_width = m_options.m_videoWidth;
    track.m_height = m_options.m_videoHeight;
    track.m_rate = m_options.m_videoRate;

    if (!m_file->SetTracks(tracks))
      return false;

    m_videoTrack = trackId;

    PVideoFrameInfo frameInfo(m_options.m_videoWidth, m_options.m_videoHeight);
    if (!m_file->ConfigureVideo(m_videoTrack, frameInfo)) {
      PTRACE(2, "Cannot use " << frameInfo << " as video output");
      return false;
    }
  }
  else
#endif
  {
    track.m_channels = m_options.m_stereo ? 2 : 1;
    track.m_size = track.m_channels * sizeof(short);

    if (!m_file->SetTracks(tracks))
      return false;

    m_audioTrack = trackId;

    if (!m_file->ConfigureAudio(trackId, track.m_channels, format.GetClockRate())) {
      PTRACE(2, "Cannot use " << track.m_channels << " channels, " << format.GetClockRate() << "Hz as audio output");
      return false;
    }

    if (!m_audioMixer->SetSampleRate(format.GetClockRate()))
      return false;
  }

  return mixer->AddStream(strmId);
}


bool OpalMediaFileRecordManager::CloseStream(const PString & streamId)
{
  m_mutex.Wait();

  PSmartPtr<AudioMixer> audioMixer = m_audioMixer;
#if OPAL_VIDEO
  PSmartPtr<VideoMixer> videoMixer = m_videoMixer;
#endif

  m_mutex.Signal();

  if (audioMixer != NULL)
    audioMixer->RemoveStream(streamId);
#if OPAL_VIDEO
  if (videoMixer != NULL)
    videoMixer->RemoveStream(streamId);
#endif

  PTRACE(4, "Closed stream " << streamId);
  return true;
}


bool OpalMediaFileRecordManager::OnPushAudio()
{
  m_mutex.Wait();
  PSmartPtr<AudioMixer> audioMixer = m_audioMixer;
  m_mutex.Signal();
  return audioMixer != NULL && audioMixer->OnPush();
}


unsigned OpalMediaFileRecordManager::GetPushAudioPeriodMS() const
{
  m_mutex.Wait();
  PSmartPtr<AudioMixer> audioMixer = m_audioMixer;
  m_mutex.Signal();
  return audioMixer != NULL ? audioMixer->GetPeriodMS() : 0;
}


bool OpalMediaFileRecordManager::WriteAudio(const PString & strmId, const RTP_DataFrame & rtp)
{
  m_mutex.Wait();
  PSmartPtr<AudioMixer> audioMixer = m_audioMixer;
  m_mutex.Signal();
  return audioMixer != NULL && audioMixer->WriteStream(strmId, rtp);
}


bool OpalMediaFileRecordManager::OnMixedAudio(const RTP_DataFrame & frame)
{
  PWaitAndSignal mutex(m_mutex);

  if (!IsOpen() && m_audioTrack < m_file->GetTrackCount())
    return false;

  PINDEX written;
  if (!m_file->WriteAudio(m_audioTrack, frame.GetPayloadPtr(), frame.GetPayloadSize(), written))
    return false;

  PTRACE(6, "Written " << written << " audio samples");
  return true;
}


#if OPAL_VIDEO

bool OpalMediaFileRecordManager::WriteVideo(const PString & strmId, const RTP_DataFrame & rtp)
{
  m_mutex.Wait();
  PSmartPtr<VideoMixer> videoMixer = m_videoMixer;
  m_mutex.Signal();
  return videoMixer != NULL && videoMixer->WriteStream(strmId, rtp);
}


bool OpalMediaFileRecordManager::OnPushVideo()
{
  m_mutex.Wait();
  PSmartPtr<VideoMixer> videoMixer = m_videoMixer;
  m_mutex.Signal();
  return videoMixer != NULL && videoMixer->OnPush();
}


unsigned OpalMediaFileRecordManager::GetPushVideoPeriodMS() const
{
  m_mutex.Wait();
  PSmartPtr<VideoMixer> videoMixer = m_videoMixer;
  m_mutex.Signal();
  return videoMixer != NULL ? videoMixer->GetPeriodMS() : 0;
}


bool OpalMediaFileRecordManager::OnMixedVideo(const RTP_DataFrame & frame)
{
  PWaitAndSignal mutex(m_mutex);

  if (!IsOpen() && m_videoTrack < m_file->GetTrackCount())
    return false;

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)frame.GetPayloadPtr();
  if (header->x != 0 || header->y != 0 || header->width != m_options.m_videoWidth || header->height != m_options.m_videoHeight) {
    PTRACE(2, "Unexpected change of video frame size!");
    return false;
  }

  if (!m_file->WriteVideo(m_videoTrack, OPAL_VIDEO_FRAME_DATA_PTR(header)))
    return false;

  PTRACE(6, "Written video frame");
  return true;
}

#endif // OPAL_VIDEO


OpalMediaFileRecordManager::FactoryInitialiser::FactoryInitialiser()
{
  PStringSet fileTypes = PMediaFile::GetAllFileTypes();

  PCaselessString firstExt;
  for (PStringSet::iterator it = fileTypes.begin(); it != fileTypes.end(); ++it) {
    if (!firstExt.IsEmpty())
      OpalRecordManager::Factory::RegisterAs(*it, firstExt);
    else {
      OpalRecordManager::Factory::Register(*it, this);
      firstExt = *it;
    }
  }
}


OpalRecordManager * OpalMediaFileRecordManager::FactoryInitialiser::Create(OpalRecordManager::Factory::Param_T) const
{
  return new OpalMediaFileRecordManager();
}


static OpalMediaFileRecordManager::FactoryInitialiser OpalMediaFileRecordManager_FactoryInitialiser_instance;


#endif // OPAL_HAS_MIXER


/////////////////////////////////////////////////////////////////////////////
