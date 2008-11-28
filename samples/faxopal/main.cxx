/*
 * main.cxx
 *
 * OPAL application source file for sending/receiving faxes via T.38
 *
 * Copyright (c) 2008 Vox Lucida Pty. Ltd.
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


PCREATE_PROCESS(FaxOPAL);


FaxOPAL::FaxOPAL()
  : PProcess("OPAL T.38 Fax", "FaxOPAL", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
  , m_manager(NULL)
{
}


FaxOPAL::~FaxOPAL()
{
  delete m_manager;
}


void FaxOPAL::Main()
{
  PArgList & args = GetArguments();

  args.Parse("h-help."
             "d-directory:"
             "s-spandsp:"
             "G-G711."
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
    cerr << "usage: " << GetFile().GetTitle() << " [ options ] filename [ url ]\n"
            "\n"
            "Available options are:\n"
            "  --help                  : print this help message.\n"
            "  -d or --directory dir   : Set default directory for fax receive\n"
            "  -s or --spandsp exe     : Set location of spandsp_util.exe\n"
            "  -G or -G711             : Send fax as G.711 audio\n"
#if PTRACING
            "  -o or --output file     : file name for output of log messages\n"       
            "  -t or --trace           : degree of verbosity in error log (more times for more detail)\n"     
#endif
            "\n"
            "e.g. " << GetFile().GetTitle() << " send_fax.tif sip:fred@bloggs.com\n"
            "\n"
            "     " << GetFile().GetTitle() << " received_fax.tif\n\n";
    return;
  }

  m_manager = new MyManager();

#if OPAL_SIP
  SIPEndPoint * sip  = new SIPEndPoint(*m_manager);
  if (!sip->StartListeners(PStringArray())) {
    cerr << "Could not start default SIP listeners." << endl;
    return;
  }
#endif // OPAL_SIP

#if OPAL_H323
  H323EndPoint * h323 = new H323EndPoint(*m_manager);
  if (!h323->StartListeners(PStringArray())) {
    cerr << "Could not start default H.323 listeners." << endl;
    return;
  }
#endif // OPAL_H323

  // Create audio or T.38 fax endpoint.
  OpalFaxEndPoint * fax  = args.HasOption('G') ? new OpalFaxEndPoint(*m_manager)
                                               : new OpalT38EndPoint(*m_manager);
  if (args.HasOption('d'))
    fax->SetDefaultDirectory(args.GetOptionString('d'));
  if (args.HasOption('s'))
    fax->SetSpanDSP(args.GetOptionString('s'));

  // Route SIP/H.323 calls to the Fax endpoint
#if OPAL_SIP
  m_manager->AddRouteEntry("sip.*:.* = " + fax->GetPrefixName() + ":" + args[0] + ";receive");
#endif
#if OPAL_H323
  m_manager->AddRouteEntry("h323.*:.* = " + fax->GetPrefixName() + ":" + args[0] + ";receive");
#endif

  // If no explicit protocol on URI, then send to SIP.
  m_manager->AddRouteEntry(fax->GetPrefixName()+":.* = sip:<da>");

  if (args.GetCount() == 1)
    cout << "Awaiting incoming fax, saving as " << args[0] << " ..." << flush;
  else {
    PString token;
    if (!m_manager->SetUpCall(fax->GetPrefixName() + ":" + args[0], args[1], token)) {
      cerr << "Could not start call to \"" << args[1] << '"' << endl;
      return;
    }
    cout << "Sending " << args[0] << " to " << args[1] << " ..." << flush;
  }

  // Wait for call to come in and finish
  m_manager->m_completed.Wait();
  cout << " completed.";
}


void MyManager::OnClearedCall(OpalCall & /*call*/)
{
  m_completed.Signal();
}


// End of File ///////////////////////////////////////////////////////////////
