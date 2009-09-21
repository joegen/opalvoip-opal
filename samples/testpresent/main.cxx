/*
 * main.h
 *
 * OPAL application source file for testing Opal presentities
 *
 * Copyright (c) 2009 Post Increment
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

#include <ptlib.h>

#include <sip/sippres.h>



//////////////////////////////////////////////////////////////

#include <ptclib/http.h>

class XCAPClient : public PHTTPClient
{
  public:
};


//////////////////////////////////////////////////////////////

class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager)
  public:
};

class TestPresEnt : public PProcess
{
  PCLASSINFO(TestPresEnt, PProcess)

  public:
    TestPresEnt();
    ~TestPresEnt();

    PDECLARE_AuthorisationRequestNotifier(TestPresEnt, AuthorisationRequest);
    PDECLARE_PresenceChangeNotifier(TestPresEnt, PresenceChange);

    virtual void Main();

  private:
    MyManager * m_manager;
    OpalPresentity * sipEntity1;
    OpalPresentity * sipEntity2;
};


PCREATE_PROCESS(TestPresEnt);

TestPresEnt::TestPresEnt()
  : PProcess("OPAL Test Presentity", "TestPresEnt", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
  , m_manager(NULL)
{
}


TestPresEnt::~TestPresEnt()
{
  delete m_manager; 
}



void TestPresEnt::Main()
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

  if (args.HasOption('h')) {
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

  XCAPClient xcap;

#if 0

  PString uri = "http://192.168.1.1/";
  xcap.SetAuthenticationInfo("admin", "adsl4me");

  PMIMEInfo outMime, inMime;
  PBYTEArray body;
  if (!xcap.GetDocument(uri, outMime, inMime) || !xcap.ReadContentBody(inMime, body))
    cout << "Read failed:" << xcap.GetLastResponseCode() << " " << xcap.GetLastResponseInfo() << endl;
  else {
    cout << "Read document" << endl;
    cout << PString((const char *)body.GetPointer(), body.GetSize());
  }

#endif
 
#if 0

  PString server   = "siptest.colibria.com:8080/services";
  PString auid     = "pres-rules";
  PString subtree  = "users";
  PString xui      = "requestec1";
  PString document = "index~~*";

  xcap.SetAuthenticationInfo("requestec1", "req1ec1");

  PString uri("http://");
  uri += server + '/' 
      + auid + '/' 
      + subtree + '/' 
      + xui + '/' 
      + document;

  PMIMEInfo outMime, inMime;
  PBYTEArray body;
  if (!xcap.GetDocument(uri, outMime, inMime) || !xcap.ReadContentBody(inMime, body)) {
    cout << "Read failed:" << xcap.GetLastResponseCode() << " " << xcap.GetLastResponseInfo() << endl;
    cout << PString((const char *)body.GetPointer(), body.GetSize());
  }
  else {
    cout << "Read document" << endl;
    cout << PString((const char *)body.GetPointer(), body.GetSize());
  }

#endif

#if 1

  if (args.GetCount() < 5) {
    cerr << "error: insufficient arguments" << endl;
    return;
  }

  const char * server = args[0];
  const char * pres1  = args[1];
  const char * pass1  = args[2];
  const char * pres2  = args[3];
  const char * pass2  = args[4];

  MyManager m_manager;
  //m_manager.SetTranslationAddress(PIPSocket::Address("115.64.157.126"));
  SIPEndPoint * sip  = new SIPEndPoint(m_manager);
  if (!sip->StartListener("")) { //udp$192.168.2.2:7777")) {
    cerr << "Could not start SIP listeners." << endl;
    return;
  }

  // create presentity #1
  {
    sipEntity1 = OpalPresentity::Create(m_manager, pres1);
    sipEntity1->SetAuthorisationRequestNotifier(PCREATE_AuthorisationRequestNotifier(AuthorisationRequest));
    sipEntity1->SetPresenceChangeNotifier(PCREATE_PresenceChangeNotifier(PresenceChange));

    if (sipEntity1 == NULL) {
      cerr << "error: cannot create presentity for '" << pres1 << "'" << endl;
      return;
    }

    sipEntity1->GetAttributes().Set(SIP_Presentity::DefaultPresenceServerKey, server);
    sipEntity1->GetAttributes().Set(SIP_Presentity::AuthPasswordKey,          pass1);
    sipEntity1->GetAttributes().Set(OpalPresentity::TimeToLiveKey,            "1200");

    if (!sipEntity1->Open()) {
      cerr << "error: cannot open presentity '" << pres1 << endl;
      return;
    }

    cout << "Opened '" << pres1 << "' using presence server '" << sipEntity1->GetAttributes().Get(SIP_Presentity::PresenceServerKey) << "'" << endl;

    cout << "Setting presence for '" << pres1 << "'" << endl;
    sipEntity1->SetLocalPresence(OpalPresentity::Available);

  }

  // create presentity #2
  {
    sipEntity2 = OpalPresentity::Create(m_manager, pres2);
    if (sipEntity2 == NULL) {
      cerr << "error: cannot create presentity for '" << pres2 << "'" << endl;
      return;
    }

    sipEntity2->GetAttributes().Set(SIP_Presentity::DefaultPresenceServerKey, server);
    sipEntity2->GetAttributes().Set(SIP_Presentity::AuthPasswordKey,          pass2);
    sipEntity2->GetAttributes().Set(OpalPresentity::TimeToLiveKey,            "1200");

    if (!sipEntity2->Open()) {
      cerr << "error: cannot open presentity '" << pres1 << endl;
      return;
    }

    cout << "Opened '" << pres2 << " using presence server '" << sipEntity2->GetAttributes().Get(SIP_Presentity::PresenceServerKey) << "'" << endl;
  }

  // presentity #2 asks for access to presentity #1's presence information
  sipEntity2->SubscribeToPresence(pres1);

  for (int i = 0;i < 10000; ++i) {
    Sleep(1000);
  }

  delete sipEntity1;
  delete sipEntity2;

#endif

  return;
}


void TestPresEnt::AuthorisationRequest(OpalPresentity & presentity, const PString & aor)
{
  cout << aor << " requesting access to presence for " << presentity.GetAOR() << endl;

  presentity.SetPresenceAuthorisation(aor, OpalPresentity::AuthorisationPermitted);
}


void TestPresEnt::PresenceChange(OpalPresentity & presentity, const SIPPresenceInfo & info)
{
  cout << "Presence for " << info.m_entity << " changed to " << info.m_basic << endl;
}



// End of File ///////////////////////////////////////////////////////////////
