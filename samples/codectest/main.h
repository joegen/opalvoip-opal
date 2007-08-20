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
 * $Log: main.h,v $
 * Revision 1.1  2007/08/20 06:28:15  rjongbloed
 * Added new application to test audio and video codecs by taking real media
 *   (camera/YUV file/microphone/WAV file etc) encoding it, decoding it and playing
 *   back on real output device (screen/speaker etc)
 *
 */

#ifndef _CodecTest_MAIN_H
#define _CodecTest_MAIN_H


class TranscoderThread : public PThread
{
  public:
    TranscoderThread(const char * name)
      : PThread(5000, NoAutoDeleteThread, NormalPriority, name)
      , running(true)
      , encoder(NULL)
      , decoder(NULL)
    {
    }

    ~TranscoderThread()
    {
      delete encoder;
      delete decoder;
    }

    int InitialiseCodec(PArgList & args, const OpalMediaFormat & rawFormat);

    void Stop()
    {
      running = false;
      WaitForTermination();
    }

    virtual void Main();

    virtual bool Read(RTP_DataFrame & frame) = 0;
    virtual bool Write(const RTP_DataFrame & frame) = 0;

    bool running;

    PSyncPointAck pause;
    PSyncPoint    resume;

    OpalTranscoder * encoder;
    OpalTranscoder * decoder;
};


class AudioThread : public TranscoderThread
{
  public:
    AudioThread()
      : TranscoderThread("Audio")
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

    PSoundChannel * recorder;
    PSoundChannel * player;
    PINDEX          readSize;
};


class VideoThread : public TranscoderThread
{
  public:
    VideoThread()
      : TranscoderThread("Video")
      , grabber(NULL)
      , display(NULL)
    {
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

    PVideoInputDevice  * grabber;
    PVideoOutputDevice * display;
};


class CodecTest : public PProcess
{
  PCLASSINFO(CodecTest, PProcess)

  public:
    CodecTest();
    ~CodecTest();

    virtual void Main();

  protected:
    AudioThread audio;
    VideoThread video;
};


#endif  // _CodecTest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
