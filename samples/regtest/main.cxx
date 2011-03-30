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
    void MyRegister(const SIPURL & aor);
    bool HasPending() const;
    void PrintResults() const;

    PString  m_password;
    PString  m_contact;
    StatsMap m_pending;
};


class RegTest : public PProcess
{
    PCLASSINFO(RegTest, PProcess)
  public:
    RegTest();

    virtual void Main();

    OpalManager     m_manager;
    MySIPEndPoint * m_endpoint;
};


PCREATE_PROCESS(RegTest);


RegTest::RegTest()
  : PProcess("OPAL RegTest", "RegTest", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
{
}


void RegTest::Main()
{
  PArgList & args = GetArguments();

  args.Parse("c-count:"
             "C-contact:"
             "I-interfaces:"
             "p-password:"
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
    cerr << "usage: " << GetFile().GetTitle() << " [ options ] aor [ ... ]\n"
            "\n"
            "Available options are:\n"
            "  -c or --count n            : Count of users to register.\n"
            "  -C or --contact url        : Pre-define REGISTER Contact header.\n"
            "  -I or --interfaces iface   : Use specified interface(s)\n"
            "  -p or --password pwd       : Pasword to use for all registrations\n"
#if PTRACING
            "  -o or --output file        : file name for output of log messages\n"       
            "  -t or --trace              : degree of verbosity in error log (more times for more detail)\n"     
#endif
            "  -h or --help               : print this help message.\n"
            "\n"
            "e.g. " << GetFile().GetTitle() << " sip:fred@bloggs.com\n"
            "If --count is used, then users sip:fredXXXXX@bloggs.com are registered where\n"
            "XXXXX is an integer from 1 to count.\n"
            ;
    return;
  }

  m_endpoint = new MySIPEndPoint(m_manager);

  m_endpoint->StartListeners(args.GetOptionString('I').Lines());

  m_endpoint->m_password = args.GetOptionString('p');
  m_endpoint->m_contact = args.GetOptionString('C');

  unsigned count = args.GetOptionString('c').AsUnsigned();

  PTime startTime;

  for (PINDEX arg = 0; arg < args.GetCount(); ++arg) {
    SIPURL url;
    if (!url.Parse(args[arg]))
      cerr << "Could not parse \"" << args[arg] << "\" as a SIP address\n";
    else if (count == 0)
      m_endpoint->MyRegister(url);
    else {
      PString nameFormat = url.GetUserName() + "%05u";
      for (unsigned index = 0; index < count; ++index) {
        url.SetUserName(psprintf(nameFormat, index));
        m_endpoint->MyRegister(url);
      }
    }
  }

  if (count > 0) {
    PTimeInterval duration = PTime() - startTime;
    cout << "Registration requests took " << duration << ", "
         << (1000.0*count/duration.GetMilliSeconds()) << "/second, "
         << ((double)duration.GetMilliSeconds()/count) << "ms/REGISTER" << endl;
  }

  cout << "Waiting for all registrations to complete ..." << endl;

  while (m_endpoint->HasPending())
    PThread::Sleep(1000);

  m_endpoint->PrintResults();

  PThread::Sleep(2000);

  cout << "Unregistering ..." << endl;
}


void MySIPEndPoint::MyRegister(const SIPURL & aor)
{
  SIPRegister::Params params;

  params.m_addressOfRecord  = aor.AsString();
  params.m_password         = m_password;
  params.m_contactAddress   = m_contact;
  params.m_expire           = 300;

  PString returnedAOR;

  if (Register(params, returnedAOR))
    m_pending[returnedAOR]; // Create 
  else
    cerr << "Registration of " << aor << " failed" << endl;
}


void MySIPEndPoint::OnRegistrationStatus(const RegistrationStatus & status)
{
  SIPEndPoint::OnRegistrationStatus(status);

  StatsMap::iterator it = m_pending.find(status.m_addressofRecord);
  if (it == m_pending.end())
    return;

  it->second.m_status = status.m_reason;
  it->second.m_finishTime.SetCurrentTime();
}


bool MySIPEndPoint::HasPending() const
{
  for (StatsMap::const_iterator it = m_pending.begin(); it != m_pending.end(); ++it) {
    if (it->second.m_status/100 == 1)
      return true;
  }

  return false;
}


void MySIPEndPoint::PrintResults() const
{
  cout << "Registrations completed:" << endl;
  for (StatsMap::const_iterator it = m_pending.begin(); it != m_pending.end(); ++it) {
    cout << "aor: " << it->first
         << "  status: " << it->second.m_status
         << "  time: " << (it->second.m_finishTime - it->second.m_startTime) << "s\n";
  }
}


// End of File ///////////////////////////////////////////////////////////////
