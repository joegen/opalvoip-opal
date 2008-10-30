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
    TranscoderThread(unsigned _num, const char * name)
      : PThread(5000, NoAutoDeleteThread, NormalPriority, name)
      , running(true)
      , encoder(NULL)
      , decoder(NULL)
      , num(_num)
      , timestamp(0)
      , markerHandling(NormalMarkers)
      , rcEnable(false)
    {
    }

    ~TranscoderThread()
    {
      delete encoder;
      delete decoder;
    }

    int InitialiseCodec(PArgList & args, const OpalMediaFormat & rawFormat);

    virtual void Main();

    virtual bool Read(RTP_DataFrame & frame) = 0;
    virtual bool Write(const RTP_DataFrame & frame) = 0;
    virtual void Stop() = 0;

    virtual void UpdateStats(const RTP_DataFrame &) { }

    virtual void CalcSNR(const RTP_DataFrame & /*src*/, const RTP_DataFrame & /*dst*/)
    {  }

    virtual void ReportSNR()
    {  }

    bool running;

    PSyncPointAck pause;
    PSyncPoint    resume;

    OpalTranscoder * encoder;
    OpalTranscoder * decoder;
    unsigned         num;
    DWORD            timestamp;
    enum MarkerHandling {
      SuppressMarkers,
      ForceMarkers,
      NormalMarkers
    } markerHandling;

    PDECLARE_NOTIFIER(OpalMediaCommand, TranscoderThread, OnTranscoderCommand);
    bool forceIFrame;

    int framesToTranscode;
    int frameTime;
    bool calcSNR;

    bool rcEnable;
    OpalVideoRateController rateController;
};


class AudioThread : public TranscoderThread
{
  public:
    AudioThread(unsigned _num)
      : TranscoderThread(_num, "Audio")
      , recorder(NULL)
      , player(NULL)
      , readSize(0)
    {
    }

    ~AudioThread()
    {
      delete recorder;
      delete player;
    }

    bool Initialise(PArgList & args);

    virtual void Main();
    virtual bool Read(RTP_DataFrame & frame);
    virtual bool Write(const RTP_DataFrame & frame);
    virtual void Stop();

    PSoundChannel * recorder;
    PSoundChannel * player;
    PINDEX          readSize;
};

class VideoThread : public TranscoderThread
{
  public:
    VideoThread(unsigned _num)
      : TranscoderThread(_num, "Video")
      , grabber(NULL)
      , display(NULL)
      , singleStep(false)
      , frameWait(0, INT_MAX)
    {
      InitStats();
    }

    ~VideoThread()
    {
      delete grabber;
      delete display;
    }

    bool Initialise(PArgList & args);

    virtual void Main();
    virtual bool Read(RTP_DataFrame & frame);
    virtual bool Write(const RTP_DataFrame & frame);
    virtual void Stop();

    void InitStats();
    virtual void UpdateStats(const RTP_DataFrame &);
    void UpdateFrameStats();

    PVideoInputDevice  * grabber;
    PVideoOutputDevice * display;

    bool                 singleStep;
    PSemaphore           frameWait;
    unsigned             frameRate;

    void CalcSNR(const RTP_DataFrame & src, const RTP_DataFrame & dst);
    void ReportSNR();

    PString frameFn;
    PString packetFn;

    PTextFile packetStatFile, frameStatFile;
    PInt64 packetCount;
    PInt64 frameCount;
    PInt64 frameBytes;
    PInt64 totalFrameBytes;

    unsigned snrWidth, snrHeight;
    double sumYSNR;
    double sumCbSNR;
    double sumCrSNR;
    PInt64 snrCount;
};


class CodecTest : public PProcess
{
  PCLASSINFO(CodecTest, PProcess)

  public:
    CodecTest();
    ~CodecTest();

    virtual void Main();

    class TestThreadInfo {
      public:
        TestThreadInfo(unsigned _num)
          : audio(_num), video(_num) { }
        AudioThread audio;
        VideoThread video;
    };
};


#endif  // _CodecTest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
