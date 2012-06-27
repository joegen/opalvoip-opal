/*
 * main.h
 *
 * OPAL application source file for testing codecs
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _CodecTest_MAIN_H
#define _CodecTest_MAIN_H

#include <codec/vidcodec.h>
#include <opal/patch.h>

class TranscoderThread : public PThread
{
  public:
    TranscoderThread(unsigned num, const char * name)
      : PThread(5000, NoAutoDeleteThread, NormalPriority, name)
      , m_running(false)
      , m_encoder(NULL)
      , m_decoder(NULL)
      , m_num(num)
      , m_timestamp(0)
      , m_markerHandling(NormalMarkers)
      , m_rateController(NULL)
      , m_dropPercent(0)
    {
    }

    ~TranscoderThread()
    {
      delete m_encoder;
      delete m_decoder;
    }

    int InitialiseCodec(PArgList & args, const OpalMediaFormat & rawFormat);

    virtual void Main();

    virtual bool Read(RTP_DataFrame & frame) = 0;
    virtual bool Write(const RTP_DataFrame & frame) = 0;
    void Start()
    {
      if (m_running)
        Resume();
    }
    virtual void Stop() = 0;

    virtual void UpdateStats(const RTP_DataFrame &) { }

    virtual void CalcSNR(const RTP_DataFrame & /*src*/, const RTP_DataFrame & /*dst*/)
    {  }

    virtual void ReportSNR()
    {  }

    bool m_running;

    PSyncPointAck m_pause;
    PSyncPoint    m_resume;

    OpalTranscoder * m_encoder;
    OpalTranscoder * m_decoder;
    unsigned         m_num;
    DWORD            m_timestamp;
    enum MarkerHandling {
      SuppressMarkers,
      ForceMarkers,
      NormalMarkers
    } m_markerHandling;

    PDECLARE_NOTIFIER(OpalMediaCommand, TranscoderThread, OnTranscoderCommand);
    bool m_forceIFrame;

    OpalVideoRateController * m_rateController;
    int  m_framesToTranscode;
    int  m_frameTime;
    bool m_calcSNR;
    BYTE m_extensionHeader;
    int  m_dropPercent;
};


class AudioThread : public TranscoderThread
{
  public:
    AudioThread(unsigned _num)
      : TranscoderThread(_num, "Audio")
      , m_recorder(NULL)
      , m_player(NULL)
      , m_readSize(0)
    {
    }

    ~AudioThread()
    {
      delete m_recorder;
      delete m_player;
    }

    bool Initialise(PArgList & args);

    virtual void Main();
    virtual bool Read(RTP_DataFrame & frame);
    virtual bool Write(const RTP_DataFrame & frame);
    virtual void Stop();

    PSoundChannel * m_recorder;
    PSoundChannel * m_player;
    PINDEX          m_readSize;
};


class VideoThread : public TranscoderThread
{
  public:
    VideoThread(unsigned num)
      : TranscoderThread(num, "Video")
      , m_grabber(NULL)
      , m_display(NULL)
      , m_singleStep(false)
      , m_frameWait(0, INT_MAX)
    {
    }

    ~VideoThread()
    {
      delete m_grabber;
      delete m_display;
    }

    bool Initialise(PArgList & args);

    virtual void Main();
    virtual bool Read(RTP_DataFrame & frame);
    virtual bool Write(const RTP_DataFrame & frame);
    virtual void Stop();

    void CalcVideoPacketStats(const RTP_DataFrameList & frames, bool isIFrame);
    void WriteFrameStats(const PString & str);

    PVideoInputDevice  * m_grabber;
    PVideoOutputDevice * m_display;

    bool                 m_singleStep;
    PSemaphore           m_frameWait;
    unsigned             m_frameRate;

    void SaveSNRFrame(const RTP_DataFrame * src)
    { m_snrSourceFrames.push(src); }

    void CalcSNR(const RTP_DataFrame & src);
    void CalcSNR(const RTP_DataFrame & src, const RTP_DataFrame & dst);
    void ReportSNR();

    PString   m_frameFilename;
    PTextFile m_frameStatFile;
    PInt64    m_frameCount;
    DWORD     m_frameStartTimestamp;
    std::queue<const RTP_DataFrame *> m_snrSourceFrames;

    unsigned m_snrWidth, m_snrHeight;
    double   m_sumYSNR;
    double   m_sumCbSNR;
    double   m_sumCrSNR;
    PInt64   m_snrCount;

    OpalBitRateCalculator m_bitRateCalc;
};


class CodecTest : public PProcess
{
  PCLASSINFO(CodecTest, PProcess)

  public:
    CodecTest();
    ~CodecTest();

    virtual void Main();

    class TestThreadInfo : public PObject
    {
      public:
        TestThreadInfo(unsigned num)
          : m_audio(num)
          , m_video(num)
        {
        }

        bool Initialise(PArgList & args) { return m_audio.Initialise(args)  &&  m_video.Initialise(args); }
        void Start()                            { m_audio.Start();              m_video.Start(); }
        void Stop()                             { m_audio.Stop();               m_video.Stop(); }
        void Wait()                             { m_audio.WaitForTermination(); m_video.WaitForTermination(); }

        AudioThread m_audio;
        VideoThread m_video;
    };
};


#endif  // _CodecTest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
