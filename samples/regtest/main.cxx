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
 * $Revision: 22301 $
 * $Author: rjongbloed $
 * $Date: 2009-03-26 15:25:06 +1100 (Thu, 26 Mar 2009) $
 */

#include <opal/manager.h>
#include <sip/sipep.h>

#include <map>


struct Statistics {
  Statistics()
    : m_status(SIP_PDU::Information_Trying)
    , m_finishTime(0)
    { }

  SIP_PDU::StatusCodes m_status;
  PTime                m_startTime;
  PTime                m_finishTime;
};

typedef std::map<PString, Statistics> StatsMap;


class MySIPEndPoint : public SIPEndPoint
{
    PCLASSINFO(MySIPEndPoint, SIPEndPoint)
  public:
    MySIPEndPoint(OpalManager & mgr) : SIPEndPoint(mgr) { }

    virtual void OnRegistrationStatus(const RegistrationStatus & status);
    void PerformTest(const PArgList & args);
    void MyRegister(const SIPURL & aor);
    bool HasPending() const;

    PString  m_password;
    PString  m_contact;
    PString  m_proxy;
    StatsMap m_statistics;
};


class RegTest : public PProcess
{
    PCLASSINFO(RegTest, PProcess)
  public:
    RegTest();

    virtual void Main();
};


PCREATE_PROCESS(RegTest);


RegTest::RegTest()
  : PProcess("OPAL RegTest", "RegTest", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
{
}


void RegTest::Main()
{
  PArgList & args = GetArguments();

  PArgList::ParseResult parseResult = args.Parse("[Options:]"
      "c-count: Count of users to register.\n"
      "C-contact: Pre-define REGISTER Contact header.\n"
      "I-interfaces: Use specified interface(s)\n"
      "p-password: Pasword to use for all registrations\n"
      "P-proxy: Proxy to use for registration.\n"
      "v-verbose. Indicate verbose output.\n"
      PTRACE_ARGLIST
      "h-help."
      , false);

  PAssert(parseResult != PArgList::ParseInvalidOptions, PInvalidParameter);

  if (parseResult <= PArgList::ParseNoArguments || args.HasOption('h')) {
    args.Usage(cerr, "[ options ] aor [ ... ]") << "\n"
            "e.g. " << GetFile().GetTitle() << " sip:fred@bloggs.com\n"
            "If --count is used, then users sip:fredXXXXX@bloggs.com are registered where\n"
            "XXXXX is an integer from 1 to count.\n"
            ;
    return;
  }

  PTRACE_INITIALISE(args);

  {
    OpalManager manager;
    MySIPEndPoint * endpoint = new MySIPEndPoint(manager);
    endpoint->PerformTest(args);
    cout << "Unregistering ..." << endl;
  }

  cout << "Test completed." << endl;
}


void MySIPEndPoint::PerformTest(const PArgList & args)
{
  StartListeners(args.GetOptionString('I').Lines());

  m_password = args.GetOptionString('p');
  m_contact = args.GetOptionString('C');
  m_proxy = args.GetOptionString('P');

  unsigned count = args.GetOptionString('c').AsUnsigned();

  PTime startTime;

  for (PINDEX arg = 0; arg < args.GetCount(); ++arg) {
    SIPURL url;
    if (!url.Parse(args[arg]))
      cerr << "Could not parse \"" << args[arg] << "\" as a SIP address\n";
    else if (count == 0)
      MyRegister(url);
    else {
      PString nameFormat = url.GetUserName() + "%05u";
      for (unsigned index = 0; index < count; ++index) {
        url.SetUserName(psprintf(nameFormat, index));
        MyRegister(url);
      }
    }
  }

  if (count > 0) {
    PTimeInterval duration = PTime() - startTime;
    cout << "Registration requests took " << duration << " seconds, "
         << (1000.0*count/duration.GetMilliSeconds()) << "/second, "
         << ((double)duration.GetMilliSeconds()/count) << "ms/REGISTER" << endl;
  }

  cout << "Waiting for all registrations to complete ..." << endl;

  while (HasPending())
    PThread::Sleep(1000);

  if (args.HasOption('v'))
    cout << "Registrations completed, details:" << endl;

  double totalTime = 0;
  unsigned divisor = 0;
  unsigned failed = 0;
  for (StatsMap::const_iterator it = m_statistics.begin(); it != m_statistics.end(); ++it) {
    PTimeInterval duration = it->second.m_finishTime - it->second.m_startTime;
    totalTime += duration.GetMilliSeconds();
    ++divisor;
    if (it->second.m_status != 200)
      ++failed;
    if (args.HasOption('v'))
      cout << "aor: " << it->first
           << "  status: " << it->second.m_status
           << "  time: " << (it->second.m_finishTime - it->second.m_startTime) << "s\n";
  }
  cout << "Average registration time: " << (totalTime/divisor) << "ms, (" << failed << " failed)\n";
}


void MySIPEndPoint::MyRegister(const SIPURL & aor)
{
  SIPRegister::Params params;

  params.m_addressOfRecord  = aor.AsString();
  params.m_password         = m_password;
  params.m_contactAddress   = m_contact;
  params.m_proxyAddress     = m_proxy;
  params.m_expire           = 300;

  PString returnedAOR;

  if (Register(params, returnedAOR))
    m_statistics[returnedAOR]; // Create 
  else
    cerr << "Registration of " << aor << " failed" << endl;
}


void MySIPEndPoint::OnRegistrationStatus(const RegistrationStatus & status)
{
  SIPEndPoint::OnRegistrationStatus(status);

  StatsMap::iterator it = m_statistics.find(status.m_addressofRecord);
  if (it == m_statistics.end())
    return;

  it->second.m_status = status.m_reason;
  it->second.m_finishTime.SetCurrentTime();
}


bool MySIPEndPoint::HasPending() const
{
  for (StatsMap::const_iterator it = m_statistics.begin(); it != m_statistics.end(); ++it) {
    if (it->second.m_status/100 == 1)
      return true;
  }

  return false;
}


// End of File ///////////////////////////////////////////////////////////////
