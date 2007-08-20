/*
 * main.cxx
 *
 * OPAL application source file for testing codecs
 *
 * Main program entry point.
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
 * $Log: main.cxx,v $
 * Revision 1.1  2007/08/20 06:28:15  rjongbloed
 * Added new application to test audio and video codecs by taking real media
 *   (camera/YUV file/microphone/WAV file etc) encoding it, decoding it and playing
 *   back on real output device (screen/speaker etc)
 *
 */

#include "precompile.h"
#include "main.h"


PCREATE_PROCESS(CodecTest);


CodecTest::CodecTest()
  : PProcess("OPAL Audio/Video Codec Tester", "codectest", 1, 0, ReleaseCode, 0)
{
}


CodecTest::~CodecTest()
{
}


void CodecTest::Main()
{
  PArgList & args = GetArguments();

  args.Parse("h-help."
             "-record-driver:"
             "R-record-device:"
             "-play-driver:"
             "P-play-device:"
             "-grab-driver:"
             "G-grab-device:"
             "-grab-format:"
             "-grab-channel:"
             "-display-driver:"
             "D-display-device:"
             "S-frame-size:"
             "R-frame-rate:"
             "C-crop."
#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
             , FALSE);

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('h') || args.GetCount() == 0) {
    PError << "usage: codectest [ options ] fmtname [ fmtname ]\n"
              "  where fmtname is the Media Format Name for the codec(s) to test, up to two\n"
              "  formats (one audio and one video) may be specified.\n"
              "\n"
              "Available options are:\n"
              "  --help                  : print this help message.\n"
              "  --record-driver drv     : audio recorder driver.\n"
              "  -R --record-device dev  : audio recorder device.\n"
              "  --play-driver drv       : audio player driver.\n"
              "  -P --play-device dev    : audio player device.\n"
              "  --grab-driver drv       : video grabber driver.\n"
              "  -G --grab-device dev    : video grabber device.\n"
              "  --grab-format fmt       : video grabber format (\"pal\"/\"ntsc\")\n"
              "  --grab-channel num      : video grabber channel.\n"
              "  --display-driver drv    : video display driver to use.\n"
              "  -D --display-device dev : video display device to use.\n"
              "  -S --frame-size size    : video frame size (\"qcif\", \"cif\", WxH)\n"
              "  -R --frame-rate size    : video frame rate (frames/second)\n"
              "  -C --crop               : crop rather than scale if resizing\n"
#if PTRACING
              "  -o or --output file     : file name for output of log messages\n"       
              "  -t or --trace           : degree of verbosity in error log (more times for more detail)\n"     
#endif
              "\n"
              "e.g. ./codectest --grab-device fake --grab-channel 2 GSM-AMR H.264\n\n";
    return;
  }

  if (!audio.Initialise(args) || !video.Initialise(args))
    return;

  audio.Resume();
  video.Resume();

  // command line
  for (;;) {

    // display the prompt
    cout << "codectest> " << flush;
    PCaselessString cmd;
    cin >> cmd;

    if (cmd == "q" || cmd == "x" || cmd == "quit" || cmd == "exit")
      break;

    if (cmd == "fg") {
      if (!video.grabber->SetVFlipState(!video.grabber->GetVFlipState()))
        cout << "\nCould not toggle Vflip state of video grabber device" << endl;
      continue;
    }

    if (cmd == "fd") {
      if (!video.display->SetVFlipState(!video.display->GetVFlipState()))
        cout << "\nCould not toggle Vflip state of video display device" << endl;
      continue;
    }

    unsigned width, height;
    if (PVideoFrameInfo::ParseSize(cmd, width, height)) {
      video.pause.Signal();
      if  (!video.grabber->SetFrameSizeConverter(width, height))
        cout << "Video grabber device could not be set to size " << width << 'x' << height << endl;
      if  (!video.display->SetFrameSizeConverter(width, height))
        cout << "Video display device could not be set to size " << width << 'x' << height << endl;
      video.resume.Signal();
      continue;
    }

    cout << "Select:\n"
            "  fg     : Flip video grabber top to bottom\n"
            "  fd     : Flip video display top to bottom\n"
            "  qcif   : Set size of grab & display to qcif\n"
            "  cif    : Set size of grab & display to cif\n"
            "  WxH    : Set size of grab & display W by H\n"
            "  Q or X : Exit program\n" << endl;
  } // end for

  cout << "Exiting." << endl;
  audio.Stop();
  video.Stop();
}


int TranscoderThread::InitialiseCodec(PArgList & args, const OpalMediaFormat & rawFormat)
{
  for (PINDEX i = 0; i < args.GetCount(); i++) {
    OpalMediaFormat mediaFormat = args[i];
    if (mediaFormat.IsEmpty()) {
      cout << "Unknown media format name \"" << args[i] << '"' << endl;
      return false;
    }

    if (mediaFormat.GetDefaultSessionID() == rawFormat.GetDefaultSessionID()) {
      if ((encoder = OpalTranscoder::Create(rawFormat, mediaFormat)) == NULL) {
        cout << "Could not create encoder for media format \"" << mediaFormat << '"' << endl;
        return false;
      }

      if ((decoder = OpalTranscoder::Create(mediaFormat, rawFormat)) == NULL) {
        cout << "Could not create decoder for media format \"" << mediaFormat << '"' << endl;
        return false;
      }

      return -1;
    }
  }

  return 1;
}


bool AudioThread::Initialise(PArgList & args)
{
  PINDEX i;

  switch (InitialiseCodec(args, OpalPCM16)) {
    case 0 :
      return false;
    case 1 :
      return true;
  }

  readSize = encoder->GetOptimalDataFrameSize(TRUE);

  // Audio recorder
  PString recordDriverName = args.GetOptionString("record-driver");
  if (!recordDriverName .IsEmpty()) {
    recorder = PSoundChannel::CreateChannel(recordDriverName);
    if (recorder == NULL) {
      cerr << "Cannot use audio recorder driver name \"" << recordDriverName << "\", must be one of:\n";
      PStringList drivers = PSoundChannel::GetDriverNames();
      for (i = 0; i < drivers.GetSize(); i++)
        cerr << "  " << drivers[i] << '\n';
      cerr << endl;
      return false;
    }
  }

  PStringList devices = PSoundChannel::GetDriversDeviceNames(recordDriverName, PSoundChannel::Recorder);
  if (devices.IsEmpty()) {
    cerr << "No audio recorder devices present";
    if (!recordDriverName.IsEmpty())
      cerr << " for driver \"" << recordDriverName << '"';
    cerr << endl;
    return false;
  }

  PString recordDeviceName = args.GetOptionString("record-device");
  if (recordDeviceName.IsEmpty())
    recordDeviceName = PSoundChannel::GetDefaultDevice(PSoundChannel::Recorder);
    
  if (recorder == NULL)
    recorder = PSoundChannel::CreateChannelByName(recordDeviceName, PSoundChannel::Recorder);

  if (recorder == NULL || !recorder->Open(recordDeviceName, PSoundChannel::Recorder)) {
    cerr << "Cannot use audio recorder device name \"" << recordDeviceName << "\", must be one of:\n";
    PStringList drivers = PSoundChannel::GetDriverNames();
    for (i = 0; i < drivers.GetSize(); i++) {
      devices = PSoundChannel::GetDriversDeviceNames(drivers[i], PSoundChannel::Recorder);
      for (PINDEX j = 0; j < devices.GetSize(); j++)
        cerr << "  " << setw(20) << setiosflags(ios::left) << drivers[i] << devices[j] << '\n';
    }
    cerr << endl;
    return false;
  }

  cout << "Audio Recorder ";
  if (!recordDriverName.IsEmpty())
    cout << "driver \"" << recordDriverName << "\" and ";
  cout << "device \"" << recorder->GetName() << "\" opened." << endl;


  // Audio player
  PString playDriverName = args.GetOptionString("play-driver");
  if (!playDriverName .IsEmpty()) {
    player = PSoundChannel::CreateChannel(playDriverName);
    if (player == NULL) {
      cerr << "Cannot use audio player driver name \"" << playDriverName << "\", must be one of:\n";
      PStringList drivers = PSoundChannel::GetDriverNames();
      for (i = 0; i < drivers.GetSize(); i++)
        cerr << "  " << drivers[i] << '\n';
      cerr << endl;
      return false;
    }
  }

  devices = PSoundChannel::GetDriversDeviceNames(playDriverName, PSoundChannel::Player);
  if (devices.IsEmpty()) {
    cerr << "No audio player devices present";
    if (!playDriverName.IsEmpty())
      cerr << " for driver \"" << playDriverName << '"';
    cerr << endl;
    return false;
  }

  PString playDeviceName = args.GetOptionString("play-device");
  if (playDeviceName.IsEmpty())
    playDeviceName = PSoundChannel::GetDefaultDevice(PSoundChannel::Player);
    
  if (player == NULL)
    player = PSoundChannel::CreateChannelByName(playDeviceName, PSoundChannel::Player);

  if (player == NULL || !player->Open(playDeviceName, PSoundChannel::Player)) {
    cerr << "Cannot use audio player device name \"" << playDeviceName << "\", must be one of:\n";
    PStringList drivers = PSoundChannel::GetDriverNames();
    for (i = 0; i < drivers.GetSize(); i++) {
      devices = PSoundChannel::GetDriversDeviceNames(drivers[i], PSoundChannel::Player);
      for (PINDEX j = 0; j < devices.GetSize(); j++)
        cerr << "  " << setw(20) << setiosflags(ios::left) << drivers[i] << devices[j] << '\n';
    }
    cerr << endl;
    return false;
  }

  cout << "Audio Player ";
  if (!playDriverName.IsEmpty())
    cout << "driver \"" << playDriverName << "\" and ";
  cout << "device \"" << player->GetName() << "\" opened." << endl;

  return true;
}


bool VideoThread::Initialise(PArgList & args)
{
  PINDEX i;

  switch (InitialiseCodec(args, OpalYUV420P)) {
    case 0 :
      return false;
    case 1 :
      return true;
  }

  // Video grabber
  PString grabDriverName = args.GetOptionString("grab-driver");
  if (!grabDriverName.IsEmpty()) {
    grabber = PVideoInputDevice::CreateDevice(grabDriverName);
    if (grabber == NULL) {
      cerr << "Cannot use video grabber driver name \"" << grabDriverName << "\", must be one of:\n";
      PStringList drivers = PVideoInputDevice::GetDriverNames();
      for (i = 0; i < drivers.GetSize(); i++)
        cerr << "  " << drivers[i] << '\n';
      cerr << endl;
      return false;
    }
  }

  PStringList devices = PVideoInputDevice::GetDriversDeviceNames(grabDriverName);
  if (devices.IsEmpty()) {
    cerr << "No video grabber devices present";
    if (!grabDriverName.IsEmpty())
      cerr << " for driver \"" << grabDriverName << '"';
    cerr << endl;
    return false;
  }

  PString grabDeviceName = args.GetOptionString("grab-device");
  if (grabDeviceName.IsEmpty())
    grabDeviceName = devices[0];
    
  if (grabber == NULL)
    grabber = PVideoInputDevice::CreateDeviceByName(grabDeviceName);

  if (grabber == NULL || !grabber->Open(grabDeviceName, false)) {
    cerr << "Cannot use video grabber device name \"" << grabDeviceName << "\", must be one of:\n";
    PStringList drivers = PVideoInputDevice::GetDriverNames();
    for (i = 0; i < drivers.GetSize(); i++) {
      devices = PVideoInputDevice::GetDriversDeviceNames(drivers[i]);
      for (PINDEX j = 0; j < devices.GetSize(); j++)
        cerr << "  " << setw(20) << setiosflags(ios::left) << drivers[i] << devices[j] << '\n';
    }
    cerr << endl;
    return false;
  }

  cout << "Video Grabber ";
  if (!grabDriverName.IsEmpty())
    cout << "driver \"" << grabDriverName << "\" and ";
  cout << "device \"" << grabber->GetDeviceName() << "\" opened." << endl;

  if (args.HasOption("grab-format")) {
    PVideoDevice::VideoFormat format;
    PCaselessString formatString = args.GetOptionString("grab-format");
    if (formatString == "PAL")
      format = PVideoDevice::PAL;
    else if (formatString == "NTSC")
      format = PVideoDevice::NTSC;
    else if (formatString == "SECAM")
      format = PVideoDevice::SECAM;
    else if (formatString == "Auto")
      format = PVideoDevice::Auto;
    else {
      cerr << "Illegal video grabber format name \"" << formatString << '"' << endl;
      return false;
    }
    if (!grabber->SetVideoFormat(format)) {
      cerr << "Video grabber device could not be set to input format \"" << formatString << '"' << endl;
      return false;
    }
  }
  cout << "Grabber input format set to " << grabber->GetVideoFormat() << endl;

  if (args.HasOption("grab-channel")) {
    int videoInput = args.GetOptionString("grab-channel").AsInteger();
    if (!grabber->SetChannel(videoInput)) {
      cerr << "Video grabber device could not be set to channel " << videoInput << endl;
    return false;
  }
  }
  cout << "Grabber grabber channel set to " << grabber->GetChannel() << endl;

  
  int frameRate;
  if (args.HasOption("frame-rate"))
    frameRate = args.GetOptionString("frame-rate").AsInteger();
  else
    frameRate = grabber->GetFrameRate();

  if (!grabber->SetFrameRate(frameRate)) {
    cerr << "Video grabber device could not be set to frame rate " << frameRate << endl;
    return false;
  }
  cout << "Grabber frame rate set to " << grabber->GetFrameRate() << endl;


  // Video display
  PString displayDriverName = args.GetOptionString("display-driver");
  if (!displayDriverName.IsEmpty()) {
    display = PVideoOutputDevice::CreateDevice(displayDriverName);
    if (display == NULL) {
      cerr << "Cannot use video display driver name \"" << displayDriverName << "\", must be one of:\n";
      PStringList drivers = PVideoOutputDevice::GetDriverNames();
      for (i = 0; i < drivers.GetSize(); i++)
        cerr << "  " << drivers[i] << '\n';
      cerr << endl;
      return false;
    }
  }

  devices = PVideoOutputDevice::GetDriversDeviceNames(displayDriverName);
  if (devices.IsEmpty()) {
    cerr << "No video video display devices present";
    if (!displayDriverName.IsEmpty())
      cerr << " for driver \"" << displayDriverName << '"';
    cerr << endl;
    return false;
  }

  PString displayDeviceName = args.GetOptionString("display-device");
  if (displayDeviceName.IsEmpty()) {
    displayDeviceName = devices[0];
    if (displayDeviceName == "NULL" && devices.GetSize() > 1)
      displayDeviceName = devices[1];
  }

  if (display == NULL)
    display = PVideoOutputDevice::CreateDeviceByName(displayDeviceName);

  if (display == NULL || !display->Open(displayDeviceName, false)) {
    cerr << "Cannot use video display device name \"" << displayDeviceName << "\", must be one of:\n";
    PStringList drivers = PVideoOutputDevice::GetDriverNames();
    for (i = 0; i < drivers.GetSize(); i++) {
      devices = PVideoOutputDevice::GetDriversDeviceNames(drivers[i]);
      for (PINDEX j = 0; j < devices.GetSize(); j++)
        cerr << "  " << setw(20) << setiosflags(ios::left) << drivers[i] << devices[j] << '\n';
    }
    cerr << endl;
    return false;
  }

  cout << "Display ";
  if (!displayDriverName.IsEmpty())
    cout << "driver \"" << displayDriverName << "\" and ";
  cout << "device \"" << display->GetDeviceName() << "\" opened." << endl;

  // Configure sizes/speeds
  unsigned width, height;
  if (args.HasOption("frame-size")) {
    PString sizeString = args.GetOptionString("frame-size");
    if (!PVideoFrameInfo::ParseSize(sizeString, width, height)) {
      cerr << "Illegal video frame size \"" << sizeString << '"' << endl;
      return false;
    }
  }
  else {
    grabber->GetFrameSize(width, height);
  }

  PVideoFrameInfo::ResizeMode resizeMode = args.HasOption("crop") ? PVideoFrameInfo::eCropCentre : PVideoFrameInfo::eScale;
  if (!grabber->SetFrameSizeConverter(width, height, resizeMode)) {
    cerr << "Video grabber device could not be set to size " << width << 'x' << height << endl;
    return false;
  }
  cout << "Grabber frame size set to " << grabber->GetFrameWidth() << 'x' << grabber->GetFrameHeight() << endl;

  if  (!display->SetFrameSizeConverter(width, height, resizeMode)) {
    cerr << "Video display device could not be set to size " << width << 'x' << height << endl;
    return false;
  }

  cout << "Display frame size set to " << display->GetFrameWidth() << 'x' << display->GetFrameHeight() << endl;


  if (!grabber->SetColourFormatConverter("YUV420P") ) {
    cerr << "Video grabber device could not be set to colour format YUV420P" << endl;
    return false;
  }

  cout << "Grabber colour format set to " << grabber->GetColourFormat() << " (";
  if (grabber->GetColourFormat() == "YUV420P")
    cout << "native";
  else
    cout << "converted to YUV420P";
  cout << ')' << endl;

  if (!display->SetColourFormatConverter("YUV420P")) {
    cerr << "Video display device could not be set to colour format YUV420P" << endl;
    return false;
  }

  cout << "Display colour format set to " << display->GetColourFormat() << " (";
  if (display->GetColourFormat() == "YUV420P")
    cout << "native";
  else
    cout << "converted from YUV420P";
  cout << ')' << endl;


  return true;
}


void AudioThread::Main()
{
  if (recorder == NULL || player == NULL)
    return;

  TranscoderThread::Main();
}


void VideoThread::Main()
{
  if (grabber == NULL || display == NULL)
    return;

  grabber->Start();
  display->Start();

  TranscoderThread::Main();
}


void TranscoderThread::Main()
{
  if (encoder == NULL || decoder == NULL)
    return;

  unsigned frameCount = 0;
  unsigned packetCount = 0;
  bool oldSrcState = true;
  bool oldOutState = true;

  RTP_DataFrame srcFrame;

  PTimeInterval startTick = PTimer::Tick();
  while (running) {
    bool srcState = Read(srcFrame);
    if (oldSrcState != srcState) {
      oldSrcState = srcState;
      cerr << "Source " << (srcState ? "restored." : "failed!") << endl;
    }

    RTP_DataFrameList encFrames;
    if (!encoder->ConvertFrames(srcFrame, encFrames)) {
      cerr << "Error in encoder!" << endl;
      return;
    }

    for (PINDEX i = 0; i < encFrames.GetSize(); i++) {
      RTP_DataFrameList outFrames;
      if (!decoder->ConvertFrames(encFrames[i], outFrames)) {
        cerr << "Error in decoder!" << endl;
        return;
      }
      for (PINDEX j = 0; j < outFrames.GetSize(); j++) {
        bool outState = Write(outFrames[j]);
        if (oldOutState != outState)
        {
          oldOutState = outState;
          cerr << "Frame display " << (outState ? "restored." : "failed!") << endl;
        }
      }
      packetCount++;
    }

    if (pause.Wait(0)) {
      pause.Acknowledge();
      resume.Wait();
    }

    frameCount++;
  }

  PTimeInterval duration = PTimer::Tick() - startTick;
  cout << frameCount << " frames over " << duration << " seconds at " << (frameCount*1000.0/duration.GetMilliSeconds()) << " fps." << endl;
}


bool AudioThread::Read(RTP_DataFrame & frame)
{
  frame.SetPayloadSize(readSize);
  return recorder->Read(frame.GetPayloadPtr(), readSize);
}


bool AudioThread::Write(const RTP_DataFrame & frame)
{
  return player->Write(frame.GetPayloadPtr(), frame.GetPayloadSize());
}


bool VideoThread::Read(RTP_DataFrame & data)
{
  data.SetPayloadSize(grabber->GetMaxFrameBytes()+sizeof(OpalVideoTranscoder::FrameHeader));

  unsigned width, height;
  grabber->GetFrameSize(width, height);
  OpalVideoTranscoder::FrameHeader * frame = (OpalVideoTranscoder::FrameHeader *)data.GetPayloadPtr();
  frame->x = frame->y = 0;
  frame->width = width;
  frame->height = height;

  return grabber->GetFrameData(OPAL_VIDEO_FRAME_DATA_PTR(frame));
}


bool VideoThread::Write(const RTP_DataFrame & data)
{
  const OpalVideoTranscoder::FrameHeader * frame = (const OpalVideoTranscoder::FrameHeader *)data.GetPayloadPtr();
  display->SetFrameSize(frame->width, frame->height);
  return display->SetFrameData(frame->x, frame->y,
                               frame->width, frame->height,
                               OPAL_VIDEO_FRAME_DATA_PTR(frame), data.GetMarker());
}


// End of File ///////////////////////////////////////////////////////////////
