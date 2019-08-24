/*
 * main.cxx
 *
 * Recording Calls demonstration for OPAL
 *
 * Copyright (c) 2014 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include "precompile.h"
#include "main.h"


extern const char Manufacturer[] = "Vox Gratia";
extern const char Application[] = "OPAL Recording";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);


PString MyManager::GetArgumentSpec() const
{
  return "[Application options:]"
         "m-mix. Mix all incoming calls to single WAV file.\n"
       + OpalManagerConsole::GetArgumentSpec();
}


void MyManager::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm, "<file-template>") <<
"\n"
"Record all incoming calls using a file template. The template will usually\n"
"have a directory path and an extension dictating the output container file\n"
"format, e.g. \"/somewhere/%CALL-ID%.wav\". The available extensions are\n"
"dependent on compile time options. The substitution macros may be:\n"
"  %CALL-ID%   Call identifier\n"
"  %FROM%      Calling party\n"
"  %TO%        Answer party\n"
"  %REMOTE%    Remote party\n"
"  %LOCAL%     Local party\n"
"  %DATE%      Date for call\n"
"  %TIME%      Time for call\n"
"  %TIMESTAMP% Date/Time for call in ISO9660 format\n"
"  %HOST%      Host name of machine\n"
"\n"
"The mix option will force audio only calls and mix the audio into\n"
"a single media file. Note in this case no template is used, the argument\n"
"is a normal file path\n";
}


bool MyManager::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  args.Parse(GetArgumentSpec());

  if (args.GetCount() == 0) {
    cerr << "Argument required\n\n";
    Usage(cerr, args);
    return false;
  }

  if (!OpalManagerConsole::Initialise(args, verbose, defaultRoute))
    return false;

  PFilePath arg(args[0]);
  m_fileTemplate = arg.GetTitle();
  m_fileType = arg.GetType();
  if (m_fileTemplate.IsEmpty() || m_fileType.IsEmpty()) {
    *LockedOutput() << "Invalid template \"" << args[0] << "\"." << endl;
    return false;
  }

  m_outputDir = arg.GetDirectory();
  if (!m_outputDir.Exists()) {
    *LockedOutput() << "Directory for template \"" << args[0] << "\" does not exist." << endl;
    return false;
  }

  // Set up local endpoint, SIP/H.323 connect to this via OPAL routing engine
  MyLocalEndPoint * ep = new MyLocalEndPoint(*this);

  if (!ep->Initialise(args))
    return false;

  *LockedOutput() << "\nAwaiting incoming calls, use ^C to exit ..." << endl;
  return true;
}


void MyManager::OnEstablishedCall(OpalCall & call)
{
  call.StartRecording(m_outputDir, m_fileTemplate, m_fileType,  m_options);
  OpalManager::OnEstablishedCall(call);
}


///////////////////////////////////////////////////////////////////////////////

MyLocalEndPoint::MyLocalEndPoint(OpalManagerConsole & manager)
  : OpalLocalEndPoint(manager, EXTERNAL_SCHEME)
  , m_manager(manager)
{
}


OpalMediaFormatList MyLocalEndPoint::GetMediaFormats() const
{
  if (m_mixer.m_mediaFile.IsNULL())
    return OpalLocalEndPoint::GetMediaFormats();

  // Override default and force only audio
  OpalMediaFormatList formats;
  formats += OpalPCM16;
  formats += OpalRFC2833;
  return formats;
}


bool MyLocalEndPoint::Initialise(PArgList & args)
{
  if (args.HasOption("mix")) {
    PFilePath filepath = args[0];
    m_mixer.m_mediaFile = PMediaFile::Create(filepath);
    if (m_mixer.m_mediaFile.IsNULL() || !m_mixer.m_mediaFile->OpenForWriting(filepath)) {
      *m_manager.LockedOutput() << "Could not open media file \"" << args[0] << '"' << endl;
      return false;
    }

    // As we use mixer and its jitter buffer, we use asynch mode to say to the
    // rest of OPAL just pass packets through unobstructed.
    SetDefaultAudioSynchronicity(OpalLocalEndPoint::e_Asynchronous);
  }
  else {
    // Need to simulate blocking write when going to disk file or jitter buffer breaks
    SetDefaultAudioSynchronicity(OpalLocalEndPoint::e_SimulateSynchronous);
  }

  // Set answer immediately
  SetDeferredAnswer(false);

  return true;
}


bool MyLocalEndPoint::OnIncomingCall(OpalLocalConnection & connection)
{
  *m_manager.LockedOutput() << "Recording call from " << connection.GetRemotePartyURL() << endl;

  // Must call ancestor function to accept call
  return OpalLocalEndPoint::OnIncomingCall(connection);
}


bool MyLocalEndPoint::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (!m_mixer.m_mediaFile.IsNULL()) {
    // Arbitrary, but unique, string is the mixer key, use connections token
    OpalBaseMixer::Key_T mixerKey = connection.GetToken();

    m_mixer.AddStream(mixerKey);

    /* As we disabled jitter buffer in normal media processing, we need to add
       it in the mixer. Note that we do this so all the calls get their audio
       synchronised correctly before writing to the WAV file. Without this you
       get littly clicks and pops due to slight timing mismatches. */
    OpalJitterBuffer::Init init(GetManager().GetJitterParameters(),
                                stream.GetMediaFormat().GetTimeUnits(),
                                GetManager().GetMaxRtpPacketSize());
    m_mixer.SetJitterBufferSize(mixerKey, init);
  }

  return OpalLocalEndPoint::OnOpenMediaStream(connection, stream);
}


bool MyLocalEndPoint::OnWriteMediaFrame(const OpalLocalConnection & connection,
                                        const OpalMediaStream & /*mediaStream*/,
                                        RTP_DataFrame & frame)
{
  if (m_mixer.m_mediaFile.IsNULL())
    return false; // false means OnWriteMediaData() will get called

  m_mixer.WriteStream(connection.GetToken(), frame);
  return true;
}


bool MyLocalEndPoint::Mixer::OnMixed(RTP_DataFrame * & output)
{
  PINDEX written;
  return m_mediaFile != NULL && m_mediaFile->WriteAudio(0, output->GetPayloadPtr(), output->GetPayloadSize(), written);
}


// End of File ///////////////////////////////////////////////////////////////
