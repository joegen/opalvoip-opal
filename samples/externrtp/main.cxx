/*
 * main.cxx
 *
 * OPAL application source file for EXternal RTP demonstration for OPAL
 *
 * Copyright (c) 2011 Vox Lucida Pty. Ltd.
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



PCREATE_PROCESS(MyProcess);


MyProcess::MyProcess()
  : PProcess("Vox Gratia", "MyProcess", 1, 0, ReleaseCode, 0)
  , m_manager(NULL)
{
}


void MyProcess::Main()
{
  m_manager = new MyManager();

  PArgList & args = GetArguments();

  if (!args.Parse(m_manager->GetArgumentSpec()) || args.HasOption('h') || args.GetCount() == 0) {
    PString name = GetFile().GetTitle();
    cerr << "usage: " << name << " [ options ] [ -l | url ]\n"
            "\n"
            "Available options are:\n"
         << m_manager->GetArgumentUsage()
         << "\n"
            "\n";
    return;
  }

  if (!m_manager->Initialise(args, true))
    return;


  // Set up local endpoint
  new MyLocalEndPoint(*m_manager);


  m_manager->AddRouteEntry("sip.*:.* = " EXTERNAL_SCHEME ":");
  m_manager->AddRouteEntry("h323.*:.* = " EXTERNAL_SCHEME ":");


  if (args.HasOption('l'))
    cout << "Awaiting incoming call ... " << flush;
  else {
    PString token;
    if (!m_manager->SetUpCall(EXTERNAL_SCHEME ":", args[0], token)) {
      cerr << "Could not start call to \"" << args[0] << '"' << endl;
      return;
    }
  }

  // Wait for call to come in and finish
  m_manager->m_completed.Wait();
  cout << "Completed." << endl;

  MyManager * mgr = m_manager;
  m_manager = NULL;
  delete mgr;
}


PBoolean MyManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  OpalRTPMediaStream * rtpStream = dynamic_cast<OpalRTPMediaStream *>(&stream);
  if (rtpStream != NULL)
    cout << "Remote RTP media:   " << rtpStream->GetRtpSession().GetRemoteAddress(true) << "\n"
            "Remote RTP control: " << rtpStream->GetRtpSession().GetRemoteAddress(false) << endl;

  return OpalManager::OnOpenMediaStream(connection, stream);
}


void MyManager::OnClearedCall(OpalCall & call)
{
  if (call.GetPartyA().NumCompare(EXTERNAL_SCHEME) == EqualTo)
    m_completed.Signal();
}


bool MyProcess::OnInterrupt(bool)
{
  if (m_manager == NULL)
    return false;

  m_manager->m_completed.Signal();
  return true;
}


bool MyLocalEndPoint::OnOutgoingCall(const OpalLocalConnection & connection)
{
  // Default action is to return true allowing the outgoing call.
  return OpalLocalEndPoint::OnOutgoingCall(connection);
}


bool MyLocalEndPoint::OnIncomingCall(OpalLocalConnection & connection)
{
  // Default action is to return true answering the outgoing call.
  return OpalLocalEndPoint::OnIncomingCall(connection);
}


OpalLocalConnection * MyLocalEndPoint::CreateConnection(OpalCall & call,
                                                        void * userData,
                                                        unsigned options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new MyLocalConnection(call, *this, userData, options, stringOptions);
}


bool MyLocalConnection::GetMediaTransportAddresses(const OpalMediaType & mediaType,
                                                   OpalTransportAddressArray & transports) const
{
  cout << mediaType << ::flush;
  while (cin.good()) {
    OpalTransportAddress address;
    cin >> address;
    if (cin.fail())
      break;
    transports.AppendAddress(address);
  }
  return true;
}


// End of File ///////////////////////////////////////////////////////////////
