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
 */


#include <ptlib.h>

#include <opal_config.h>

#if defined(OPAL_HAS_MIXER) && defined(P_WAVFILE)

#include <ep/opalmixer.h>
#include <opal/recording.h>
#include <codec/opalwavfile.h>

#define PTraceModule() "OpalRecord"


//////////////////////////////////////////////////////////////////////////////

/** This class manages the recording of OPAL calls to WAV files.
  */
class OpalWAVRecordManager : public OpalRecordManager
{
  public:
    OpalWAVRecordManager();
    ~OpalWAVRecordManager();

    virtual bool OpenFile(const PFilePath & fn);
    virtual bool IsOpen() const;
    virtual bool Close();
    virtual bool OpenStream(const PString & strmId, const OpalMediaFormat & format);
    virtual bool CloseStream(const PString & strmId);
    virtual bool OnPushAudio();
    virtual bool OnPushVideo();
    virtual bool WriteAudio(const PString & strmId, const RTP_DataFrame & rtp);
    virtual bool WriteVideo(const PString & strmId, const RTP_DataFrame & rtp);

  protected:
    struct Mixer : public OpalAudioMixer {
      Mixer() { }
      ~Mixer() { StopPushThread(); }

      bool Open(const PFilePath & fn, const Options & options);
      virtual bool OnMixed(RTP_DataFrame * & output);

      OpalWAVFile m_file;
    } * m_mixer;

    PMutex m_mutex;
};

PFACTORY_CREATE(OpalRecordManager::Factory, OpalWAVRecordManager, ".wav", false);


OpalWAVRecordManager::OpalWAVRecordManager()
  :	m_mixer(NULL)
{
}


OpalWAVRecordManager::~OpalWAVRecordManager()
{
  Close();
}


bool OpalWAVRecordManager::OpenFile(const PFilePath & fn)
{
  if (m_options.m_audioFormat.IsEmpty())
    m_options.m_audioFormat = OpalPCM16.GetName();

  PWaitAndSignal mutex(m_mutex);

  if (IsOpen()) {
    PTRACE(2, "Cannot open mixer after it has started.");
    return false;
  }

  m_mixer = new Mixer();
  if (m_mixer->Open(fn, m_options))
    return true;

  delete m_mixer;
  m_mixer = NULL;
  return false;
}


bool OpalWAVRecordManager::IsOpen() const
{
  PWaitAndSignal mutex(m_mutex);
  return m_mixer != NULL && m_mixer->m_file.IsOpen();
}


bool OpalWAVRecordManager::Close()
{
  m_mutex.Wait();

  delete m_mixer;
  m_mixer = NULL;

  m_mutex.Signal();

  return true;
}


bool OpalWAVRecordManager::OpenStream(const PString & strmId, const OpalMediaFormat & format)
{
  PWaitAndSignal mutex(m_mutex);

  if (m_mixer == NULL || format.GetMediaType() != OpalMediaType::Audio())
    return false;

  m_mixer->m_file.SetSampleRate(format.GetClockRate());
  return m_mixer->SetSampleRate(format.GetClockRate()) &&
         m_mixer->AddStream(strmId);
}


bool OpalWAVRecordManager::CloseStream(const PString & streamId)
{
  m_mutex.Wait();
  if (m_mixer != NULL)
    m_mixer->RemoveStream(streamId);
  m_mutex.Signal();

  PTRACE(4, "Closed stream " << streamId);
  return true;
}


bool OpalWAVRecordManager::OnPushAudio()
{
  PWaitAndSignal mutex(m_mutex);
  return m_mixer != NULL && m_mixer->OnPush();
}


bool OpalWAVRecordManager::OnPushVideo()
{
  return false;
}


bool OpalWAVRecordManager::WriteAudio(const PString & strm, const RTP_DataFrame & rtp)
{
  PWaitAndSignal mutex(m_mutex);
  return m_mixer != NULL && m_mixer->WriteStream(strm, rtp);
}


bool OpalWAVRecordManager::WriteVideo(const PString &, const RTP_DataFrame &)
{
  return false;
}


bool OpalWAVRecordManager::Mixer::Open(const PFilePath & fn, const Options & options)
{
  if (!m_file.SetFormat(options.m_audioFormat)) {
    PTRACE(2, "WAV file recording does not support format " << options.m_audioFormat);
    return false;
  }

  if (!m_file.Open(fn, PFile::WriteOnly, PFile::Create|PFile::Truncate)) {
    PTRACE(2, "Could not open file \"" << fn << '"');
    return false;
  }

  if (options.m_stereo) {
    m_file.SetChannels(2);
    if (m_file.GetChannels() == 2)
      m_stereo = true;
  }

  PTRACE(4, (m_stereo ? "Stereo" : "Mono") << " mixer opened for file \"" << fn << '"');
  return true;
}


bool OpalWAVRecordManager::Mixer::OnMixed(RTP_DataFrame * & output)
{
  if (!m_file.IsOpen())
    return false;

  if (m_file.Write(output->GetPayloadPtr(), output->GetPayloadSize()))
    return true;

  PTRACE(1, "Error writing WAV file " << m_file.GetFilePath());
  return false;
}


/////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO && P_VFW_CAPTURE

#include <ptlib/vconvert.h>

#include <vfw.h>
#pragma comment(lib, "vfw32.lib")


/** This class manages the recording of OPAL calls to AVI files.
  */
class OpalAVIRecordManager : public OpalRecordManager
{
  protected:
    struct AudioMixer : public OpalAudioMixer
    {
      AudioMixer(
        OpalAVIRecordManager & manager,
        bool stereo,
        unsigned sampleRate,
        bool pushThread
      ) : OpalAudioMixer(stereo, sampleRate, pushThread)
        , m_manager(manager)
      { }
      ~AudioMixer() { StopPushThread(); }
      virtual bool OnMixed(RTP_DataFrame * & output) { return m_manager.OnMixedAudio(*output); }
      OpalAVIRecordManager & m_manager;
    } * m_audioMixer;

    struct VideoMixer : public OpalVideoMixer
    {
      VideoMixer(
        OpalAVIRecordManager & manager,
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
      OpalAVIRecordManager & m_manager;
    } * m_videoMixer;

  public:
    OpalAVIRecordManager();
    ~OpalAVIRecordManager();

    virtual bool OpenFile(const PFilePath & fn);
    virtual bool IsOpen() const;
    virtual bool Close();
    virtual bool OpenStream(const PString & strmId, const OpalMediaFormat & format);
    virtual bool CloseStream(const PString & strmId);
    virtual bool OnPushAudio();
    virtual bool OnPushVideo();
    virtual bool WriteAudio(const PString & strmId, const RTP_DataFrame & rtp);
    virtual bool WriteVideo(const PString & strmId, const RTP_DataFrame & rtp);

  protected:
    // Callback from OpalAudioMixer
    virtual bool OpenAudio(const PString & strmId, const OpalMediaFormat & format);
    virtual bool OnMixedAudio(const RTP_DataFrame & frame);

    // Callback from OpalVideoMixer
    virtual bool OpenVideo(const PString & strmId, const OpalMediaFormat & format);
    virtual bool OnMixedVideo(const RTP_DataFrame & frame);

    bool GetVideoCompressorInfo(const PString & format);

    #if PTRACING
      bool IsResultError(HRESULT result, const char * msg)
      {
        if (result == AVIERR_OK)
          return false;

        if (!PTrace::CanTrace(2))
          return true;

        ostream & strm = PTRACE_BEGIN(2);
        strm << "Error " << msg << ": ";
        switch (result) {
          case AVIERR_UNSUPPORTED :
            strm << "Unsupported compressor '" << m_options.m_videoFormat << '\'';
            break;

          case AVIERR_NOCOMPRESSOR :
            strm << "No compressor '" << m_options.m_videoFormat << '\'';
            break;

          case AVIERR_BADFORMAT :
            strm << "Bad format for '" << m_options.m_videoFormat << '\'';
            break;

          default :
            strm << "Error=0x" << hex << result;
        }
        strm << PTrace::End;

        return true;
      }

      #define IS_RESULT_ERROR(result, msg) IsResultError(result, msg)

    #else

      #define IS_RESULT_ERROR(result, msg) ((result) != AVIERR_OK)

    #endif


    PMutex     m_mutex;
    PAVIFILE   m_file;
    PAVISTREAM m_audioStream;
    DWORD      m_audioSampleCount;
    DWORD      m_audioSampleSize;
    PAVISTREAM m_videoStream;
    PAVISTREAM m_videoCompressor;
    DWORD      m_videoCompressorKeyFrameRate;
    DWORD      m_videoCompressorQuality;
    DWORD      m_VideoFrameCount;
    PBYTEArray m_videoBuffer;
    PColourConverter * m_videoConverter;
};

PFACTORY_CREATE(OpalRecordManager::Factory, OpalAVIRecordManager, ".avi", false);


static char const * const VideoModeNames[OpalRecordManager::NumVideoMixingModes] = {
  "Side by Side Letterbox Video",
  "Side by Side Scaled Video",
  "Stacked Pillarbox Video",
  "Stacked Scaled Video",
  "Separate Video Stream"
};


OpalAVIRecordManager::OpalAVIRecordManager()
  :	m_audioMixer(NULL)
  ,	m_videoMixer(NULL)
  ,	m_file(NULL)
  , m_audioStream(NULL)
  , m_audioSampleCount(0)
  , m_audioSampleSize(sizeof(short))
  , m_videoStream(NULL)
  , m_videoCompressor(NULL)
  , m_VideoFrameCount(0)
  , m_videoConverter(NULL)
{
  AVIFileInit();
}


OpalAVIRecordManager::~OpalAVIRecordManager()
{
  Close();
  AVIFileExit();
}


bool OpalAVIRecordManager::GetVideoCompressorInfo(const PString & format)
{
  if (format.GetLength() != 4)
    return false;

  HIC hic = ICOpen(mmioFOURCC('v','i','d','c'), mmioFOURCC(format[0], format[1], format[2], format[3]), ICMODE_COMPRESS);
  if (hic == NULL)
    return false;

#if PTRACING
  ICINFO info;
  ICGetInfo(hic, &info,sizeof(info));
  PTRACE(4, "Found " << format << ' ' << PString(info.szDescription));
#endif

  ICSendMessage(hic, ICM_GETDEFAULTKEYFRAMERATE, (DWORD_PTR)(LPVOID)&m_videoCompressorKeyFrameRate, sizeof(DWORD));
  ICSendMessage(hic, ICM_GETDEFAULTQUALITY, (DWORD_PTR)(LPVOID)&m_videoCompressorQuality, sizeof(DWORD));

  ICClose(hic);
  return true;
}


bool OpalAVIRecordManager::OpenFile(const PFilePath & fn)
{
  if (m_options.m_audioFormat.IsEmpty())
    m_options.m_audioFormat = OpalPCM16.GetName();
  else if (m_options.m_audioFormat != OpalPCM16 && m_options.m_audioFormat != OpalGSM0610) {
    PTRACE(2, "AVI file recording does not (yet) support format " << m_options.m_audioFormat);
    return false;
  }

  if (m_options.m_videoFormat.IsEmpty()) {
    static const char * const DefaultCompressor[] =
      { "H264", "XVID", "DIVX", "MPEG", "H263", "IV50", "CVID", "MSVC" }; // Final default to Microsoft Video 1, every system has that!
    for (PINDEX i = 0; i < PARRAYSIZE(DefaultCompressor); ++i) {
      if (GetVideoCompressorInfo(DefaultCompressor[i])) {
        m_options.m_videoFormat = DefaultCompressor[i];
        break;
      }
    }
  }
  else if (!GetVideoCompressorInfo(m_options.m_videoFormat)) {
    PTRACE(2, "AVI file recording does not (yet) support format " << m_options.m_videoFormat);
    return false;
  }

  PWaitAndSignal mutex(m_mutex);

  if (m_file != NULL) {
    PTRACE(2, "Cannot open mixer after it has started.");
    return false;
  }

  if (IS_RESULT_ERROR(AVIFileOpen(&m_file, fn, OF_WRITE|OF_CREATE, NULL), "creating AVI file"))
    return false;

  m_audioMixer = new AudioMixer(*this,
                                m_options.m_stereo,
                                8000, // Really need to make this more flexible ....
                                m_options.m_pushThreads);

  OpalVideoMixer::Styles style;
  switch (m_options.m_videoMixing) {
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

    default :
      PAssertAlways(PInvalidParameter);
      return false;
  }

  m_videoMixer = new VideoMixer(*this,
                                style,
                                m_options.m_videoWidth,
                                m_options.m_videoHeight,
                                m_options.m_videoRate,
                                m_options.m_pushThreads);

  PTRACE(4, (m_options.m_stereo ? "Stereo" : "Mono") << "-PCM/"
         << m_options.m_videoFormat << "-Video mixers opened for file \"" << fn << '"');
  return true;
}


bool OpalAVIRecordManager::IsOpen() const
{
  return m_file != NULL;
}


bool OpalAVIRecordManager::Close()
{
  m_mutex.Wait();

  AudioMixer * audioMixer = m_audioMixer;
  m_audioMixer = NULL;

  VideoMixer * videoMixer = m_videoMixer;
  m_videoMixer = NULL;

  delete m_videoConverter;
  m_videoConverter = NULL;

  if (m_videoCompressor != NULL) {
    AVIStreamRelease(m_videoCompressor);
    m_videoCompressor = NULL;
  }

  if (m_videoStream != NULL) {
    AVIStreamRelease(m_videoStream);
    m_videoStream = NULL;
  }

  if (m_audioStream != NULL) {
    AVIStreamRelease(m_audioStream);
    m_audioStream = NULL;
  }

  if (m_file != NULL) {
    AVIFileRelease(m_file);
    m_file = NULL;
  }

  m_mutex.Signal();

  delete audioMixer;
  delete videoMixer;

  return true;
}


bool OpalAVIRecordManager::OpenStream(const PString & strmId, const OpalMediaFormat & format)
{
  PWaitAndSignal mutex(m_mutex);

  if (format.GetMediaType() == OpalMediaType::Audio())
    return OpenAudio(strmId, format);

  if (format.GetMediaType() == OpalMediaType::Video())
    return OpenVideo(strmId, format);

  return false;
}


bool OpalAVIRecordManager::CloseStream(const PString & streamId)
{
  m_mutex.Wait();

  if (m_audioMixer != NULL)
    m_audioMixer->RemoveStream(streamId);

  if (m_videoMixer != NULL)
    m_videoMixer->RemoveStream(streamId);

  m_mutex.Signal();

  PTRACE(4, "Closed stream " << streamId);
  return true;
}


bool OpalAVIRecordManager::OnPushAudio()
{
  PWaitAndSignal mutex(m_mutex);
  return m_audioMixer != NULL && m_audioMixer->OnPush();
}


bool OpalAVIRecordManager::OnPushVideo()
{
  PWaitAndSignal mutex(m_mutex);
  return m_videoMixer != NULL && m_videoMixer->OnPush();
}


bool OpalAVIRecordManager::WriteAudio(const PString & strmId, const RTP_DataFrame & rtp)
{
  PWaitAndSignal mutex(m_mutex);
  return m_audioMixer != NULL && m_audioMixer->WriteStream(strmId, rtp);
}


bool OpalAVIRecordManager::WriteVideo(const PString & strmId, const RTP_DataFrame & rtp)
{
  PWaitAndSignal mutex(m_mutex);
  return m_videoMixer != NULL && m_videoMixer->WriteStream(strmId, rtp);
}


bool OpalAVIRecordManager::OpenAudio(const PString & strmId, const OpalMediaFormat & format)
{
  // m_mutex already locked
  if (m_audioMixer == NULL)
    return false;

  if (!m_audioMixer->SetSampleRate(format.GetClockRate()))
    return false;

  if (m_audioStream != NULL)
    return m_audioMixer->AddStream(strmId);

  PTRACE(4, "Creating AVI stream for audio format '" << m_options.m_audioFormat << '\'');

  WAVEFORMATEX fmt;
  fmt.wFormatTag = m_options.m_audioFormat == OpalGSM0610 ? PWAVFile::fmt_GSM : WAVE_FORMAT_PCM;
  fmt.wBitsPerSample = 16;
  fmt.nChannels = m_audioMixer->IsStereo() ? 2 : 1;
  fmt.nSamplesPerSec = m_audioMixer->GetSampleRate();
  fmt.nBlockAlign = (fmt.nChannels*fmt.wBitsPerSample+7)/8;
  fmt.nAvgBytesPerSec = fmt.nSamplesPerSec*fmt.nBlockAlign;
  fmt.cbSize = 0;

  m_audioSampleSize = fmt.nBlockAlign;

  AVISTREAMINFO info;
  memset(&info, 0, sizeof(info));
  info.fccType = streamtypeAUDIO;
  info.dwScale = fmt.nBlockAlign;
  info.dwRate = fmt.nAvgBytesPerSec;
  info.dwSampleSize = fmt.nBlockAlign;
  info.dwQuality = (DWORD)-1;
  strcpy(info.szName, fmt.nChannels == 2 ? "Stereo Audio" : "Mixed Audio");

  if (IS_RESULT_ERROR(AVIFileCreateStream(m_file, &m_audioStream, &info), "creating AVI audio stream"))
    return false;

  if (IS_RESULT_ERROR(AVIStreamSetFormat(m_audioStream, 0, &fmt, sizeof(fmt)), "setting format of AVI audio stream"))
    return false;

  return m_audioMixer->AddStream(strmId);
}


bool OpalAVIRecordManager::OnMixedAudio(const RTP_DataFrame & frame)
{
  PWaitAndSignal mutex(m_mutex);

  if (!IsOpen() || m_audioStream == NULL)
    return false;

  DWORD samples = frame.GetPayloadSize()/m_audioSampleSize;
  if (IS_RESULT_ERROR(AVIStreamWrite(m_audioStream,
                                   m_audioSampleCount, samples,
                                   frame.GetPayloadPtr(), frame.GetPayloadSize(),
                                   0, NULL, NULL), "writing AVI audio stream"))
    return false;

  PTRACE(6, "Written video samples at " << m_audioSampleCount << ", count=" << samples);
  m_audioSampleCount += samples;
  return true;
}


bool OpalAVIRecordManager::OpenVideo(const PString & strmId, const OpalMediaFormat & /*format*/)
{
  // m_mutex already locked
  if (m_videoMixer == NULL)
    return false;

  if (m_videoStream != NULL)
    return m_videoMixer->AddStream(strmId);

  PTRACE(4, "Creating AVI stream for video format '" << m_options.m_videoFormat << '\'');

  PVideoFrameInfo yuv(m_options.m_videoWidth, m_options.m_videoHeight, "YUV420P");
  PVideoFrameInfo rgb(m_options.m_videoWidth, m_options.m_videoHeight, "BGR24");

  AVISTREAMINFO info;
  memset(&info, 0, sizeof(info));
  info.fccType = streamtypeVIDEO;
  info.dwRate = 1000;
  info.dwScale = info.dwRate/m_options.m_videoRate;
  info.rcFrame.right = m_options.m_videoWidth;
  info.rcFrame.bottom = m_options.m_videoHeight;
  info.dwSuggestedBufferSize = m_options.m_videoWidth*m_options.m_videoHeight;
  info.dwQuality = m_videoCompressorQuality;
  strcpy(info.szName, VideoModeNames[m_options.m_videoMixing]);

  if (IS_RESULT_ERROR(AVIFileCreateStream(m_file, &m_videoStream, &info), "creating AVI video stream"))
    return false;

  AVICOMPRESSOPTIONS opts;
  memset(&opts, 0, sizeof(opts));
  opts.fccType = streamtypeVIDEO;
  opts.fccHandler = mmioFOURCC(m_options.m_videoFormat[0],
                               m_options.m_videoFormat[1],
                               m_options.m_videoFormat[2],
                               m_options.m_videoFormat[3]);
  opts.dwQuality = m_videoCompressorQuality;
  opts.dwKeyFrameEvery = m_videoCompressorKeyFrameRate;
  opts.dwBytesPerSecond = 100000;
  opts.dwFlags = AVICOMPRESSF_KEYFRAMES|AVICOMPRESSF_DATARATE;
  if (IS_RESULT_ERROR(AVIMakeCompressedStream(&m_videoCompressor, m_videoStream, &opts, NULL), "creating AVI video compressor"))
    return false;

  BITMAPINFOHEADER fmt;
  memset(&fmt, 0, sizeof(fmt));
  fmt.biSize = sizeof(fmt);
  fmt.biCompression = mmioFOURCC('I','4','2','0');
  fmt.biWidth = m_options.m_videoWidth;
  fmt.biHeight = -(int)m_options.m_videoHeight;
  fmt.biBitCount = 12;
  fmt.biPlanes = 3;
  fmt.biSizeImage = yuv.CalculateFrameBytes();

  if (AVIStreamSetFormat(m_videoCompressor, 0, &fmt, sizeof(fmt)) != AVIERR_OK) {
    fmt.biCompression = BI_RGB;
    fmt.biBitCount = 24;
    fmt.biPlanes = 1;
    fmt.biSizeImage = rgb.CalculateFrameBytes();

    if (IS_RESULT_ERROR(AVIStreamSetFormat(m_videoCompressor, 0, &fmt, sizeof(fmt)), "setting format of AVI video compressor"))
      return false;

    m_videoConverter = PColourConverter::Create(yuv, rgb);
    if (m_videoConverter == NULL)
      return false;

    PTRACE(4, "Compressor requires RGB, creating converter, buffer size=" << fmt.biSizeImage);
  }
  else {
    PTRACE(4, "Compressor can use YUV420P directly, no converter needed, buffer size=" << fmt.biSizeImage);
  }

  return m_videoBuffer.SetSize(fmt.biSizeImage) &&
         m_videoMixer->AddStream(strmId);
}


bool OpalAVIRecordManager::OnMixedVideo(const RTP_DataFrame & frame)
{
  PWaitAndSignal mutex(m_mutex);

  if (!IsOpen() || m_videoStream == NULL)
    return false;

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)frame.GetPayloadPtr();
  if (header->x != 0 || header->y != 0 || header->width != m_options.m_videoWidth || header->height != m_options.m_videoHeight) {
    PTRACE(2, "Unexpected change of video frame size!");
    return false;
  }

  PINDEX bytesReturned;
  if (m_videoConverter == NULL)
    bytesReturned = m_videoBuffer.GetSize();
  else if (!m_videoConverter->Convert(OPAL_VIDEO_FRAME_DATA_PTR(header),
                                      m_videoBuffer.GetPointer(m_videoConverter->GetMaxDstFrameBytes()),
                                      &bytesReturned)) {
    PTRACE(2, "Conversion of YUV420P to RGB24 failed!");
    return false;
  }

  if (IS_RESULT_ERROR(AVIStreamWrite(m_videoCompressor,
                                     ++m_VideoFrameCount, 1,
                                     m_videoBuffer.GetPointer(), bytesReturned,
                                     0, NULL, NULL), "writing AVI video stream"))
    return false;

  PTRACE(5, "Written video frame " << m_VideoFrameCount << ", size=" << bytesReturned);
  return true;
}


#endif // OPAL_VIDEO && P_VFW_CAPTURE

#endif // OPAL_HAS_MIXER


/////////////////////////////////////////////////////////////////////////////
