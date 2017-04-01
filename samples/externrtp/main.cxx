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
 */

#include "precompile.h"
#include "main.h"


extern const char Manufacturer[] = "Vox Gratia";
extern const char Application[] = "OPAL External RTP";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);


bool MyManager::Initialise(PArgList & args, bool verbose, const PString &)
{
  if (!OpalManagerConsole::Initialise(args, verbose, EXTERNAL_SCHEME ":"))
    return false;

  // Set up local endpoint
  new MyLocalEndPoint(*this);


  if (args.HasOption('l')) {
    *LockedOutput() << "Awaiting incoming call ... " << flush;
    return true;
  }

  if (args.GetCount() == 0) {
    Usage(cerr, args);
    return false;
  }

  PString token;
  if (SetUpCall(EXTERNAL_SCHEME ":", args[0], token))
    return true;

  cerr << "Could not start call to \"" << args[0] << '"' << endl;
  return false;
}


PBoolean MyManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  stream.PrintDetail(*LockedOutput());
  return OpalManager::OnOpenMediaStream(connection, stream);
}


void MyManager::OnClearedCall(OpalCall & call)
{
  if (call.GetPartyA().NumCompare(EXTERNAL_SCHEME) == EqualTo)
    EndRun();
}


bool MyManager::GetMediaTransportAddresses(const OpalConnection & source,
                                           const OpalConnection &,
                                                       unsigned   sessionId,
                                            const OpalMediaType & mediaType,
                                      OpalTransportAddressArray & transports) const
{
  LockedOutput() << source.GetToken() << ' ' << mediaType << " session " << sessionId << ' ' << ::flush;
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
