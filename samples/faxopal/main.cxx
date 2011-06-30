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


// -ttttt c:\temp\testfax.tif sip:fax@10.0.1.11
// -ttttt c:\temp\incoming.tif

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
  m_manager = new MyManager();

  PArgList & args = GetArguments();

  if (!args.Parse(m_manager->GetArgumentSpec() +
                  "a-audio."
                  "A-no-audio."
                  "d-directory:"
                  "-station-id:"
                  "-header-info:"
                  "e-ignore-ced."
                  "E-suppress-ced."
                  "F-no-fallback."
                  "T-timeout:"
                  "X-switch-time:") ||
       args.HasOption('h') ||
       args.GetCount() == 0) {
    PString name = GetFile().GetTitle();
    cerr << "usage: " << name << " [ options ] filename [ url ]\n"
            "\n"
            "Available options are:\n"
            "  -d or --directory dir      : Set default directory for fax receive.\n"
            "        --station-id id      : Set T.30 Station Identifier string.\n"
            "        --header-info msg    : Set transmitted fax page header string.\n"
            "  -a or --audio              : Send fax as G.711 audio.\n"
            "  -A or --no-audio           : No audio phase at all, starts T.38 immediately.\n"
            "  -F or --no-fallback n      : Do not fall back to audio it T.38 switch fails.\n"
            "  -E or --suppress-ced       : Suppress transmission of CED tone.\n"
            "  -e or --ignore-ced         : Ignore receipt of CED tone, don't switch to T.38 mode.\n"
            "  -X or --switch-time n      : Set fail safe T.38 switch time in seconds.\n"
            "  -T or --timeout n          : Set timeout to wait for fax rx/tx to complete in seconds.\n"
            "\n"
         << m_manager->GetArgumentUsage()
         << "\n"
            "Specific T.38 format options (using -O/--option):\n"
            "  Station-Identifier    string (\"-\"\n"
            "  Header-Info           string (\"\")\n"
            "  Use-ECM               bool (1)\n"
            "  T38FaxVersion         integer (0)\n"
            "  T38FaxRateManagement  localTCF or transferredTCF (transferredTCF) \n"
            "  T38MaxBitRate         integer (14400)\n"
            "  T38FaxMaxBuffer       integer (2000\n"
            "  T38FaxMaxDatagram     integer (528)\n"
            "  T38FaxUdpEC           t38UDPFEC or t38UDPRedundancy (t38UDPRedundancy)\n"
            "  T38FaxFillBitRemoval  bool (0)\n"
            "  T38FaxTranscodingMMR  bool (0)\n"
            "  T38FaxTranscodingJBIG bool (0)\n"
            "\n"
            "e.g. " << name << " --option 'T.38:Header-Info=My custom header line' send_fax.tif sip:fred@bloggs.com\n"
            "\n"
            "     " << name << " received_fax.tif\n\n";
    return;
  }

  if (!m_manager->Initialise(args, true))
    return;

  PString prefix = args.HasOption('a') ? "fax" : "t38";

  // Create audio or T.38 fax endpoint.
  MyFaxEndPoint * fax  = new MyFaxEndPoint(*m_manager);
  if (args.HasOption('d'))
    fax->SetDefaultDirectory(args.GetOptionString('d'));

  if (!fax->IsAvailable()) {
    cerr << "No fax codecs, SpanDSP plug-in probably not installed." << endl;
    return;
  }


  m_manager->AddRouteEntry("sip.*:.* = " + prefix + ":" + args[0] + ";receive");
  m_manager->AddRouteEntry("h323.*:.* = " + prefix + ":" + args[0] + ";receive");

  if (args.HasOption('A')) {
    OpalMediaType::Fax().GetDefinition()->SetAutoStart(OpalMediaType::ReceiveTransmit);
    OpalMediaType::Audio().GetDefinition()->SetAutoStart(OpalMediaType::DontOffer);
  }
  OpalMediaType::Video().GetDefinition()->SetAutoStart(OpalMediaType::DontOffer);


  OpalConnection::StringOptions stringOptions;
  if (args.HasOption('I'))
    stringOptions.SetAt(OPAL_OPT_DETECT_INBAND_DTMF, "false");
  if (args.HasOption('i'))
    stringOptions.SetAt(OPAL_OPT_SEND_INBAND_DTMF, "false");
  if (args.HasOption("station-id"))
    stringOptions.SetAt(OPAL_OPT_STATION_ID, args.GetOptionString("station-id"));
  if (args.HasOption("header-info"))
    stringOptions.SetAt(OPAL_OPT_HEADER_INFO, args.GetOptionString("header-info"));
  if (args.HasOption('F'))
    stringOptions.SetAt(OPAL_NO_G111_FAX, "true");
  if (args.HasOption('E'))
    stringOptions.SetAt(OPAL_SUPPRESS_CED, "true");
  if (args.HasOption('e'))
    stringOptions.SetAt(OPAL_IGNORE_CED, "true");
  if (args.HasOption('X'))
    stringOptions.SetAt(OPAL_T38_SWITCH_TIME, args.GetOptionString('X').AsUnsigned());

  if (args.GetCount() == 1)
    cout << "Awaiting incoming fax, saving as " << args[0] << " ... " << flush;
  else {
    PString token;
    if (!m_manager->SetUpCall(prefix + ":" + args[0], args[1], token, NULL, 0, &stringOptions)) {
      cerr << "Could not start call to \"" << args[1] << '"' << endl;
      return;
    }
    cout << "Sending " << args[0] << " to " << args[1] << " ... " << flush;
  }

  // Wait for call to come in and finish
  if (args.HasOption('T')) {
    if (!m_manager->m_completed.Wait(args.GetOptionString('T')))
      cout << "no call";
  }
  else
    m_manager->m_completed.Wait();

  cout << " ... completed." << endl;
}


bool FaxOPAL::OnInterrupt(bool)
{
  if (m_manager == NULL)
    return false;

  m_manager->m_completed.Signal();
  return true;
}


void MyManager::OnClearedCall(OpalCall & call)
{
  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByLocalUser :
    case OpalConnection::EndedByRemoteUser :
      break;

    default :
      cout << "call error " << OpalConnection::GetCallEndReasonText(call.GetCallEndReason());
  }

  m_completed.Signal();
}


void MyFaxEndPoint::OnFaxCompleted(OpalFaxConnection & connection, bool failed)
{
  OpalMediaStatistics stats;
  connection.GetStatistics(stats);
  switch (stats.m_fax.m_result) {
    case -2 :
      cout << "failed to establish T.30";
      break;
    case 0 :
      cout << "success, "
           << (connection.IsReceive() ? stats.m_fax.m_rxPages : stats.m_fax.m_txPages)
           << " of " << stats.m_fax.m_totalPages << " pages";
      break;
    case 41 :
      cout << "failed to open TIFF file";
      break;
    case 42 :
    case 43 :
    case 44 :
    case 45 :
    case 46 :
      cout << "illegal TIFF file";
      break;
    default :
      cout << " T.30 error " << stats.m_fax.m_result;
      if (!stats.m_fax.m_errorText.IsEmpty())
        cout << " (" << stats.m_fax.m_errorText << ')';
  }
  OpalFaxEndPoint::OnFaxCompleted(connection, failed);
}


// End of File ///////////////////////////////////////////////////////////////
