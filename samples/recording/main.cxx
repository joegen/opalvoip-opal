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
 * $Revision$
 * $Author$
 * $Date$
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
  args.Usage(strm, "<dir>\n-m <wav-file>")
     << "\n"
        "Record all incoming calls to a directory, or mix all incoming calls to\n"
        " single WAV file.\n";
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

  // Set up local endpoint, SIP/H.323 connect to this via OPAL routing engine
  MyLocalEndPoint * ep = new MyLocalEndPoint(*this);

  if (!ep->Initialise(args))
    return false;

  *LockedOutput() << "\nAwaiting incoming calls, use ^C to exit ..." << endl;
  return true;
}


MyLocalEndPoint::MyLocalEndPoint(OpalConsoleManager & manager)
  : OpalLocalEndPoint(manager, EXTERNAL_SCHEME)
  , m_manager(manager)
{
}


OpalMediaFormatList MyLocalEndPoint::GetMediaFormats() const
{
  // Override default and force only audio at 8kHz
  OpalMediaFormatList formats;
  formats += OpalPCM16;
  formats += OpalRFC2833;
  return formats;
}


OpalLocalConnection * MyLocalEndPoint::CreateConnection(OpalCall & call,
                                                        void * userData,
                                                        unsigned options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new MyLocalConnection(call, *this, userData, options, stringOptions);
}


bool MyLocalEndPoint::Initialise(PArgList & args)
{
  if (args.HasOption("mix")) {
    if (!m_mixer.m_wavFile.Open(args[0], PFile::WriteOnly)) {
      *m_manager.LockedOutput() << "Could not open WAV file \"" << args[0] << '"' << endl;
      return false;
    }

    // As we use mixer and its jitter buffer, we use asynch mode to say to the
    // rest of OPAL just pass packets through unobstructed.
    SetDefaultAudioSynchronicity(OpalLocalEndPoint::e_Asynchronous);
  }
  else {
    m_wavDir = args[0];
    if (!m_wavDir.Exists()) {
      *m_manager.LockedOutput() << "Directory \"" << m_wavDir << "\" does not exist." << endl;
      return false;
    }

    // Need to simulate blocking write when going to disk file or jitter buffer breaks
    SetDefaultAudioSynchronicity(OpalLocalEndPoint::e_SimulateSyncronous);
  }

  // Set answer immediately
  SetDeferredAnswer(false);

  return true;
}


bool MyLocalEndPoint::OnIncomingCall(OpalLocalConnection & connection)
{
  if (!m_mixer.m_wavFile.IsOpen()) {
    PFilePath uniqueFilename("Call_" + connection.GetCall().GetToken() + '_', m_wavDir);
    uniqueFilename.SetType(".wav");
    MyLocalConnection & myConnection = dynamic_cast<MyLocalConnection &>(connection);
    if (!myConnection.m_wavFile.Open(uniqueFilename, PFile::WriteOnly)) {
      *m_manager.LockedOutput() << "Could not open WAV file \"" << uniqueFilename << "\""
                                   " for call from " << connection.GetRemotePartyURL() << endl;
      return false; // Refuse call
    }
  }

  *m_manager.LockedOutput() << "Recording call from " << connection.GetRemotePartyURL() << endl;

  // Must call ancestor function to accept call
  return OpalLocalEndPoint::OnIncomingCall(connection);
}


bool MyLocalEndPoint::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (m_mixer.m_wavFile.IsOpen()) {
    // Arbitrary, but unique, string is the mixer key, use connections token
    OpalBaseMixer::Key_T mixerKey = connection.GetToken();

    m_mixer.AddStream(mixerKey);

    /* As we disabled jitter buffer in normal media processing, we need to add
       it in the mixer. Note that we do this so all the calls get their audio
       synchronised correctly before writing to the WAV file. Without this you
       get littly clicks and pops due to slight timing mismatches. */
    unsigned timeUnits = stream.GetMediaFormat().GetTimeUnits();
    OpalJitterBuffer::Init init;
    init.m_minJitterDelay = manager.GetMinAudioJitterDelay()*timeUnits;
    init.m_maxJitterDelay = manager.GetMaxAudioJitterDelay()*timeUnits;
    init.m_timeUnits = timeUnits;
    init.m_packetSize = manager.GetMaxRtpPacketSize();
    m_mixer.SetJitterBufferSize(mixerKey, init);
  }

  return OpalLocalEndPoint::OnOpenMediaStream(connection, stream);
}


bool MyLocalEndPoint::OnWriteMediaFrame(const OpalLocalConnection & connection,
                                        const OpalMediaStream & /*mediaStream*/,
                                        RTP_DataFrame & frame)
{
  if (!m_mixer.m_wavFile.IsOpen())
    return false; // false means OnWriteMediaData() will get called

  m_mixer.WriteStream(connection.GetToken(), frame);
  return true;
}


bool MyLocalEndPoint::OnWriteMediaData(const OpalLocalConnection & connection,
                                       const OpalMediaStream & /*mediaStream*/,
                                       const void * data,
                                       PINDEX length,
                                       PINDEX & written)
{
  MyLocalConnection & myConnection = dynamic_cast<MyLocalConnection &>(const_cast<OpalLocalConnection &>(connection));
  if (myConnection.m_wavFile.Write(data, length))
    written = length;

  return true;
}


bool MyLocalEndPoint::Mixer::OnMixed(RTP_DataFrame * & output)
{
  return m_wavFile.Write(output->GetPayloadPtr(), output->GetPayloadSize());
}


MyLocalConnection::MyLocalConnection(OpalCall & call,
                                     MyLocalEndPoint & ep,
                                     void * userData,
                                     unsigned options,
                                     OpalConnection::StringOptions * stringOptions)
  : OpalLocalConnection(call, ep, userData, options, stringOptions)
{
}


// End of File ///////////////////////////////////////////////////////////////
