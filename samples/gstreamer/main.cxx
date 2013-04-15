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
 * $Revision$
 * $Author$
 * $Date$
 */

#include "precompile.h"
#include "main.h"


// Debug: -ttttodebugstream file:../callgen/ogm.wav sip:10.0.1.12
//        -ttttodebugstream "repeat=10;file:../callgen/ogm.wav"
//        -ttttodebugstream file:test.vxml


extern const char Manufacturer[] = "Vox Gratia";
extern const char Application[] = "OPAL GStreamer";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);


PString MyManager::GetArgumentSpec() const
{
  return "[GStreamer options]"
         "-audio-source: Audio source device in pipeline (autoaudiosrc)\n"
         "-audio-sink:   Audio sink device in pipeline (autoaudiosink)\n"
#if OPAL_VIDEO
         "-video-source: Video source device in pipeline (autovideosrc)\n"
         "-video-sink:   Video sink device in pipeline (autovideosink)\n"
#endif // OPAL_VIDEO
         + OpalManagerConsole::GetArgumentSpec();
}


bool MyManager::Initialise(PArgList & args, bool verbose, const PString &)
{
  if (!OpalManagerConsole::Initialise(args, verbose, "gst:"))
    return false;

  // Set up GStreamer
  MyGstEndPoint * gst  = new MyGstEndPoint(*this);
  gst->SetAudioSourceDevice(args.GetOptionString("audio-source", gst->GetAudioSourceDevice()));
  gst->SetAudioSinkDevice(args.GetOptionString("audio-sink", gst->GetAudioSinkDevice()));
#if OPAL_VIDEO
  gst->SetVideoSourceDevice(args.GetOptionString("video-source", gst->GetVideoSourceDevice()));
  gst->SetVideoSinkDevice(args.GetOptionString("video-sink", gst->GetVideoSinkDevice()));
#endif // OPAL_VIDEO

  cout << "GStreamer Supported Media Formats: " << setfill(',') << gst->GetMediaFormats() << setfill(' ') << endl;

  if (args.GetCount() == 0)
    cout << "Awaiting incoming call ... " << flush;
  else if (SetUpCall("gst:*", args[0]) != NULL)
    cout << "Making call to " << args[0] << " ... " << flush;
  else {
    cerr << "Could not start call to \"" << args[0] << '"' << endl;
    return false;
  }

  return true;
}


void MyManager::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm, "[ options ] [ url ]");

}


void MyManager::OnClearedCall(OpalCall & call)
{
  if (call.GetPartyA().NumCompare("gst") == EqualTo)
    EndRun();
}


// End of File ///////////////////////////////////////////////////////////////
