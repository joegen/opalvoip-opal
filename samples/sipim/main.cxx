/*
 * main.cxx
 *
 * OPAL application source file for seing IM via SIP
 *
 * Copyright (c) 2008 Post Increment
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

#include "main.h"

#include <opal/pcss.h>

PCREATE_PROCESS(SipIM);


SipIM::SipIM()
  : PProcess("OPAL SIP IM", "SipIM", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
  , m_manager(NULL)
{
}


SipIM::~SipIM()
{
  delete m_manager;
}


void SipIM::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
             "u-user:"
             "h-help."
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
    cerr << "usage: " << GetFile().GetTitle() << " [ options ] [url]\n"
            "\n"
            "Available options are:\n"
            "  -u or --user            : set local username.\n"
            "  --help                  : print this help message.\n"
#if PTRACING
            "  -o or --output file     : file name for output of log messages\n"       
            "  -t or --trace           : degree of verbosity in error log (more times for more detail)\n"     
#endif
            "\n"
            "e.g. " << GetFile().GetTitle() << " sip:fred@bloggs.com\n"
            ;
    return;
  }

  MyManager m_manager;

  if (args.HasOption('u'))
    m_manager.SetDefaultUserName(args.GetOptionString('u'));

  OpalMediaFormatList allMediaFormats;

  SIPEndPoint * sip  = new SIPEndPoint(m_manager);
  if (!sip->StartListeners(PStringArray())) {
    cerr << "Could not start SIP listeners." << endl;
    return;
  }
  allMediaFormats += sip->GetMediaFormats();

  MyPCSSEndPoint * pcss = new MyPCSSEndPoint(m_manager);
  allMediaFormats += pcss->GetMediaFormats();

  PString imFormatMask("!*IM-*");
  m_manager.SetMediaFormatMask(imFormatMask);

  allMediaFormats = OpalTranscoder::GetPossibleFormats(allMediaFormats); // Add transcoders
  for (PINDEX i = 0; i < allMediaFormats.GetSize(); i++) {
    if (!allMediaFormats[i].IsTransportable())
      allMediaFormats.RemoveAt(i--); // Don't show media formats that are not used over the wire
  }

  allMediaFormats.Remove(imFormatMask);
  
  cout << "Available codecs: " << setfill(',') << allMediaFormats << setfill(' ') << endl;

  OpalConnection::StringOptions * options = new OpalConnection::StringOptions();
  options->SetAt("autostart", "im:exclusive");

  if (args.GetCount() == 0)
    cout << "Awaiting incoming IM ..." << flush;
  else {
    PString token;
    if (!m_manager.SetUpCall("pc:", args[0], token, NULL, 0, options)) {
      cerr << "Could not start IM to \"" << args[0] << '"' << endl;
      return;
    }
  }

  // Wait for call to come in and finish
  m_manager.m_completed.Wait();
  cout << " completed.";
}

void MyManager::OnClearedCall(OpalCall & /*call*/)
{
  m_completed.Signal();
}

PBoolean MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  AcceptIncomingConnection(connection.GetToken());
  return PTrue;
}


PBoolean MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  return PTrue;
}



// End of File ///////////////////////////////////////////////////////////////
