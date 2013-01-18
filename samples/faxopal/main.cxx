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

extern const char Manufacturer[] = "Vox Gratia";
extern const char Application[] = "OPAL Fax";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);


static void PrintOption(ostream & strm, const char * name, const char * type)
{
  strm << "  " << setw(22) << name << ' ' << type << " (";
  if (strcmp(type, "string") == 0)
    strm << OpalT38.GetOptionString(name).ToLiteral();
  else if (strcmp(type, "bool") == 0)
    strm << (OpalT38.GetOptionBoolean(name) ? "true" : "false");
  else {
    PString value;
    OpalT38.GetOptionValue(name, value);
    strm << value;
  }
  strm << ")\n";
}


PString MyManager::GetArgumentSpec() const
{
  return "[Fax options:]"
         "d-directory: Set default directory for fax receive.\n"
         "-station-id: Set T.30 Station Identifier string.\n"
         "-header-info: Set transmitted fax page header string.\n"
         "a-audio. Send fax as G.711 audio.\n"
         "A-no-audio. No audio phase at all, starts T.38 immediately.\n"
         "F-no-fallback. Do not fall back to audio if T.38 switch fails.\n"
         "e-switch-on-ced. Switch to T.38 on receipt of CED tone as caller.\n"
         "X-switch-time: Set fail safe T.38 switch time in seconds.\n"
         "T-timeout: Set timeout to wait for fax rx/tx to complete in seconds.\n"
         "q-quiet. Only output error conditions.\n"
#if OPAL_STATISTICS
         "v-verbose. Output statistics during fax operation\n"
#endif
       + OpalManagerConsole::GetArgumentSpec();
}


void MyManager::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm,
             "[ options ] filename [ remote-url ... ]") << "\n"
            "Specific T.38 format options (using -O/--option):\n";
  PrintOption(strm, "Station-Identifier",    "string");
  PrintOption(strm, "Header-Info",           "string");
  PrintOption(strm, "Use-ECM",               "bool");
  PrintOption(strm, "T38FaxVersion",         "integer");
  PrintOption(strm, "T38FaxRateManagement",  "localTCF or transferredTCF");
  PrintOption(strm, "T38MaxBitRate",         "integer");
  PrintOption(strm, "T38FaxMaxBuffer",       "integer");
  PrintOption(strm, "T38FaxMaxDatagram",     "integer");
  PrintOption(strm, "T38FaxUdpEC",           "t38UDPFEC or t38UDPRedundancy");
  PrintOption(strm, "T38FaxFillBitRemoval",  "bool");
  PrintOption(strm, "T38FaxTranscodingMMR",  "bool");
  PrintOption(strm, "T38FaxTranscodingJBIG", "bool");
  strm << "\n"
          "e.g. " << args.GetCommandName() << " --option 'T.38:Header-Info=My custom header line' send_fax.tif sip:fred@bloggs.com\n"
          "\n"
          "     " << args.GetCommandName() << " received_fax.tif\n\n";
}


bool MyManager::Initialise(PArgList & args, bool, const PString &)
{
  if (!args.Parse(GetArgumentSpec())) {
    Usage(cerr, args);
    return false;
  }

  MyFaxEndPoint * faxEP  = new MyFaxEndPoint(*this);
  if (!faxEP->IsAvailable()) {
    cerr << "No fax codecs, SpanDSP plug-in probably not installed." << endl;
    return false;
  }

  static char const * FormatMask[] = { "!G.711*", "!@fax", "!@userinput" };
  SetMediaFormatMask(PStringArray(PARRAYSIZE(FormatMask), FormatMask));

  PString prefix = args.HasOption('a') && !args.HasOption('A') ?  "fax" : "t38";

  bool quiet = args.HasOption('q');
  if (quiet)
    cout.rdbuf(NULL);

  if (!OpalManagerConsole::Initialise(args, !quiet, prefix + ":" + args[0] + ";receive"))
    return false;

  cout << "Fax Mode: ";
  if (args.HasOption('A')) {
    OpalMediaType::Fax()->SetAutoStart(OpalMediaType::ReceiveTransmit);
    OpalMediaType::Audio()->SetAutoStart(OpalMediaType::DontOffer);
    cout << "Offer T.38 only";
  }
  else if (args.HasOption('a'))
    cout << "Audio Only";
  else
    cout << "Switch to T.38";
  cout << '\n';

  if (args.HasOption('d'))
    faxEP->SetDefaultDirectory(args.GetOptionString('d'));

  OpalConnection::StringOptions stringOptions;

  if (args.HasOption("station-id")) {
    PString str = args.GetOptionString("station-id");
    cout << "Station Identifier: " << str << '\n';
    stringOptions.SetAt(OPAL_OPT_STATION_ID, str);
  }

  if (args.HasOption("header-info")) {
    PString str = args.GetOptionString("header-info");
    cout << "Transmit Header Info: " << str << '\n';
    stringOptions.SetAt(OPAL_OPT_HEADER_INFO, str);
  }

  if (args.HasOption('F')) {
    stringOptions.SetBoolean(OPAL_NO_G111_FAX, true);
    cout << "Disabled fallback to audio (G.711) mode on T.38 switch failure\n";
  }

  if (args.HasOption('e')) {
    stringOptions.SetBoolean(OPAL_SWITCH_ON_CED, true);
    cout << "Enabled switch to T.38 on receipt of CED\n";
  }

  if (args.HasOption('X')) {
    unsigned seconds = args.GetOptionString('X').AsUnsigned();
    stringOptions.SetInteger(OPAL_T38_SWITCH_TIME, seconds);
    cout << "Switch to T.38 after " << seconds << " seconds\n";
  }
  else
    cout << "No T.38 switch timeout set\n";

  SetDefaultConnectionOptions(stringOptions);

  m_showProgress = args.HasOption('v');

  // Wait for call to come in and finish (default one year)
  m_competionTimeout = PTimeInterval(args.GetOptionString('T', "365:0:0:0"));
  cout << "Completion timeout is " << m_competionTimeout.AsString(0, PTimeInterval::IncludeDays) << '\n';

  PString tiff = args[0];
  if (args.GetCount() == 1) {
    cout << "Receive directory: " << faxEP->GetDefaultDirectory() << "\n"
            "\n"
            "Awaiting incoming fax, saving as " << tiff << " ..." << endl;
    return true;
  }

  bool atLeastOne = false;

  cout << '\n';
  for (PINDEX arg = 1; arg < args.GetCount(); ++arg) {
    PString destination = args[arg];
    if (SetUpCall(prefix + ":" + tiff, destination) != NULL)
      atLeastOne = true;
    else
      cerr << "Could not start call to \"" << destination << '"' << endl;
    cout << "Sending " << tiff << " to " << destination << endl;
  }
  cout << "Awaiting transmission ..." << endl;

  return atLeastOne;
}


void MyManager::Run()
{
#if OPAL_STATISTICS
  map<PString, OpalMediaStatistics> lastStatisticsByToken;
#endif

  while (!m_endRun.Wait(1000)) {
    if (m_competionTimeout.HasExpired()) {
      cout << " no call";
      break;
    }

#if OPAL_STATISTICS
    if (m_showProgress) {
      PArray<PString> tokens = GetAllCalls();
      for (PINDEX i = 0; i < tokens.GetSize(); ++i) {
        PSafePtr<OpalCall> call = FindCallWithLock(tokens[i], PSafeReadOnly);
        if (call != NULL) {
          PSafePtr<OpalFaxConnection> connection = call->GetConnectionAs<OpalFaxConnection>();
          if (connection != NULL) {
            PSafePtr<OpalMediaStream> stream = connection->GetMediaStream(OpalMediaType::Fax(), true);
            if (stream != NULL) {
              OpalMediaStatistics & lastStatistics = lastStatisticsByToken[tokens[i]];
              OpalMediaStatistics statistics;
              stream->GetStatistics(statistics);

              bool printTime = true;

#define SHOW_STAT(message, member) \
              if (lastStatistics.m_fax.member != statistics.m_fax.member) { \
                if (printTime) { \
                  cout << PTime().AsString("yyyy-MM-dd hh:mm:ss.uuu\n"); \
                  printTime = false; \
                } \
                cout << "  " << message << ": " << statistics.m_fax.member << endl; \
              }

              SHOW_STAT("Phase", m_phase);
              SHOW_STAT("Station identifier", m_stationId);
              SHOW_STAT("Tx pages", m_txPages);
              SHOW_STAT("Rx pages", m_rxPages);
              SHOW_STAT("Bit rate", m_bitRate);
              SHOW_STAT("Compression", m_compression);
              SHOW_STAT("Page width", m_pageWidth);
              SHOW_STAT("Page height", m_pageHeight);

              lastStatistics = statistics;
            }
          }
        }
      }
    }
#endif // OPAL_STATISTICS
  }

  cout << "\nCompleted." << endl;
}


void MyManager::OnClearedCall(OpalCall & call)
{
  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByLocalUser :
    case OpalConnection::EndedByRemoteUser :
      break;

    default :
      cerr << "Call error: " << OpalConnection::GetCallEndReasonText(call.GetCallEndReason());
  }

  if (GetCallCount() == 1)
    EndRun();
}


void MyFaxEndPoint::OnFaxCompleted(OpalFaxConnection & connection, bool failed)
{
#if OPAL_STATISTICS
  OpalMediaStatistics stats;
  connection.GetStatistics(stats);
  switch (stats.m_fax.m_result) {
    case -2 :
      cerr << "Failed to establish T.30\n";
      break;
    case 0 :
      cout << "Success, "
           << (connection.IsReceive() ? stats.m_fax.m_rxPages : stats.m_fax.m_txPages)
           << " of " << stats.m_fax.m_totalPages << " pages\n";
      PProcess::Current().SetTerminationValue(0); // Indicate success
      break;
    case 41 :
      cerr << "Failed to open TIFF file\n";
      break;
    case 42 :
    case 43 :
    case 44 :
    case 45 :
    case 46 :
      cerr << "Illegal TIFF file\n";
      break;
    default :
      cerr << "T.30 error " << stats.m_fax.m_result;
      if (!stats.m_fax.m_errorText.IsEmpty())
        cerr << " (" << stats.m_fax.m_errorText << ')';
      cerr << '\n';
  }
#else
  cerr << (failed ? "Success\n" : "Failed\n");
#endif // OPAL_STATISTICS

  OpalFaxEndPoint::OnFaxCompleted(connection, failed);
}


// End of File ///////////////////////////////////////////////////////////////
