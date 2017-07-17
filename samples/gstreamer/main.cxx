/*
 * main.cxx
 *
 * OPAL application source file for GStreamer OPAL sample
 *
 * Copyright (c) 2013 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include "precompile.h"
#include "main.h"


// Debug: -tttttodebugstream --video-source videotestsrc -P G.711* -P H.263plus -D H.264* -D VP* -D RFC* -S "udp$10.0.1.11:25060" --map --decoder dshowvdec_mpeg4 --encoder ffenc_msmpeg4 MPEG4 -- --map --decoder fake263dec --encoder fake263enc H.263 -- sip:10.0.1.11


extern const char Manufacturer[] = "Vox Gratia";
extern const char Application[] = "OPAL GStreamer";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);


PString MyManager::GetArgumentSpec() const
{
  return "[GStreamer options]"
         "-rtp-element:   Set RTP bin element (gstrtpbin)\n"
         "-jb-element:    Set jitter buffer element (gstrtpjitterbuffer)\n"
         "-audio-source:  Audio source device in pipeline (autoaudiosrc)\n"
         "-audio-sink:    Audio sink device in pipeline (autoaudiosink)\n"
#if OPAL_VIDEO
         "-video-source:  Video source device in pipeline (autovideosrc)\n"
         "-video-sink:    Video sink device in pipeline (autovideosink)\n"
         "-source-colour: Video colour space conversion pipeline for video source (autoconvert)\n"
         "-sink-colour:   Video colour space conversion pipeline for video sink (autoconvert)\n"
#endif // OPAL_VIDEO
         "-map.           Begin a mapping from Media Format to GStreamer pipeline\n"
         "-encoder:       Set encoder for mapping.\n"
         "-decoder:       Set decoder for mapping.\n"
         "-packetiser:    Set packetiser for mapping\n"
         "-depacketiser:  Set de-packetiser for mapping.\n"
         + OpalManagerConsole::GetArgumentSpec();
}


bool MyManager::Initialise(PArgList & args, bool verbose, const PString &)
{
  OpalMediaFormat::RegisterKnownMediaFormats(); // Make sure codecs are loaded before Initialise()

  if (!OpalManagerConsole::Initialise(args, verbose, "gst:"))
    return false;

#if OPAL_GSTREAMER

  LockedStream lockedOutput(*this);
  ostream & output = lockedOutput;

  // Set up GStreamer
  GstEndPoint * gst  = new GstEndPoint(*this);
  if (!gst->SetRTPPipeline(args.GetOptionString("rtp-element", gst->GetRTPPipeline()))) {
    output << "Could not set RTP element.\n";
    return false;
  }
  if (!gst->SetJitterBufferPipeline(args.GetOptionString("jb-element", gst->GetJitterBufferPipeline()))) {
    output << "Could not set jitter buffer element.\n";
    return false;
  }
  if (!gst->SetAudioSourceDevice(args.GetOptionString("audio-source", gst->GetAudioSourceDevice()))) {
    output << "Could not set audio source.\n";
    return false;
  }
  if (!gst->SetAudioSinkDevice(args.GetOptionString("audio-sink", gst->GetAudioSinkDevice()))) {
    output << "Could not set audio sink.\n";
    return false;
  }
#if OPAL_VIDEO
  if (!gst->SetVideoSourceDevice(args.GetOptionString("video-source", gst->GetVideoSourceDevice()))) {
    output << "Could not set video source.\n";
    return false;
  }
  if (!gst->SetVideoSinkDevice(args.GetOptionString("video-sink", gst->GetVideoSinkDevice()))) {
    output << "Could not set video sink.\n";
    return false;
  }
  if (!gst->SetVideoSourceColourConverter(args.GetOptionString("source-colour", gst->GetVideoSourceColourConverter()))) {
    output << "Could not set video source colour converter\n";
    return false;
  }
  if (!gst->SetVideoSinkColourConverter(args.GetOptionString("sink-colour", gst->GetVideoSinkColourConverter()))) {
    output << "Could not set video sink colour converter\n";
    return false;
  }
#endif // OPAL_VIDEO

  while (args.HasOption("map")) {
    if (args.GetCount() == 0) {
      output << "No media format specified for mapping\n";
      return false;
    }
    OpalMediaFormat mediaFormat(args[0]);
    if (!mediaFormat.IsValid()) {
      output << "No media format of name \"" << args[0] << '"' << endl;
      return false;
    }

    GstEndPoint::CodecPipelines codec;
    if (gst->GetMapping(mediaFormat, codec))
      output << "Overriding existing media format mapping for \"" << mediaFormat << '"' << endl;

    if (args.HasOption("encoder"))
      codec.m_encoder = args.GetOptionString("encoder");
    if (args.HasOption("decoder"))
      codec.m_decoder = args.GetOptionString("decoder");
    if (args.HasOption("packetiser"))
      codec.m_packetiser = args.GetOptionString("packetiser");
    if (args.HasOption("depacketiser"))
      codec.m_depacketiser = args.GetOptionString("depacketiser");
    if (!gst->SetMapping(mediaFormat, codec)) {
      output << "Could not set media format mapping for \"" << mediaFormat << '"' << endl;
      return false;
    }

    if (!args.Parse())
      break;
  }

  output << "GStreamer Supported Media Formats: " << setfill(',') << gst->GetMediaFormats() << setfill(' ') << endl;

  if (args.GetCount() == 0)
    output << "Awaiting incoming call ... " << endl;
  else if (SetUpCall("gst:*", args[0]) != NULL)
    output << "Making call to " << args[0] << " ... " << endl;
  else {
    output << "Could not start call to \"" << args[0] << '"' << endl;
    return false;
  }

#else

  #pragma message("GStreamer application will not work without gstreamer!")

#endif

  return true;
}


void MyManager::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm, "[ options ] [ mapping -- [ mapping -- ] ] [ url ]")
    << "\n\n"
       "The mapping sections allow for configuring the GStream pipeline segments for\n"
       "Media Formats. IT always begins with --map and has four options to set the\n"
       "encoder, decoder, packetiser, depacketiser respectively, followed by the\n"
       "media format name. A \"--\" separates each mapping, and the mapping from the\n"
       "final argument(s) being the URL(s) to call.\n"
       "\n"
       "Example:\n"
       "  --video-source videotestsrc --map --encoder my264enc H.264-1 -- --map --decoder my263dec H.263 -- sip:10.0.1.11\n"
       "\n";
}


void MyManager::OnClearedCall(OpalCall & call)
{
  if (call.GetPartyA().NumCompare("gst") == EqualTo)
    EndRun();
}


// End of File ///////////////////////////////////////////////////////////////
