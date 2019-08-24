/*
 * status.cxx
 *
 * OPAL Server status pages
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Vox Lucida Pty. Ltd.
 *
 */

#include "precompile.h"
#include "main.h"


///////////////////////////////////////////////////////////////

BaseStatusPage::BaseStatusPage(MyManager & mgr, const PHTTPAuthority & auth, const char * name)
  : PServiceHTTPString(name, PString::Empty(), "text/html; charset=UTF-8", auth)
  , m_manager(mgr)
  , m_refreshRate(30)
{
}


PString BaseStatusPage::LoadText(PHTTPRequest & request)
{
  PHTML html;
  CreateHTML(html, request.url.GetQueryVars());
  SetString(html);

  return PServiceHTTPString::LoadText(request);
}

void BaseStatusPage::CreateHTML(PHTML & html, const PStringToString & query)
{
  html << PHTML::Title(GetTitle());

  if (m_refreshRate > 0)
    html << "<meta http-equiv=\"Refresh\" content=\"" << m_refreshRate << "\">\n";

  html << PHTML::Body()
       << MyProcessAncestor::Current().GetPageGraphic()
       << PHTML::Paragraph() << "<center>"

       << PHTML::Form("POST");

  CreateContent(html, query);

  if (m_refreshRate > 0)
    html << PHTML::Paragraph() 
         << PHTML::TableStart()
         << PHTML::TableRow()
         << PHTML::TableData()
         << "Refresh rate"
         << PHTML::TableData()
         << PHTML::InputNumber("Page Refresh Time", 5, 3600, m_refreshRate)
         << PHTML::TableData()
         << PHTML::SubmitButton("Set", "Set Page Refresh Time")
         << PHTML::TableEnd();

  html << PHTML::Form()
       << PHTML::HRule()
       << "Last update: <!--#macro LongDateTime-->" << PHTML::Paragraph()
       << MyProcessAncestor::Current().GetCopyrightText()
       << PHTML::Body();
}


PBoolean BaseStatusPage::Post(PHTTPRequest & request,
                              const PStringToString & data,
                              PHTML & msg)
{
  PTRACE(2, "Main\tClear call POST received " << data);

  if (data("Set Page Refresh Time") == "Set") {
    m_refreshRate = data["Page Refresh Time"].AsUnsigned();
    CreateHTML(msg, request.url.GetQueryVars());
    PServiceHTML::ProcessMacros(request, msg, "", PServiceHTML::LoadFromFile);
    return true;
  }

  msg << PHTML::Title() << "Accepted Control Command" << PHTML::Body()
    << PHTML::Heading(1) << "Accepted Control Command" << PHTML::Heading(1);

  if (!OnPostControl(data, msg))
    msg << PHTML::Heading(2) << "No calls or endpoints!" << PHTML::Heading(2);

  msg << PHTML::Paragraph()
      << PHTML::HotLink(request.url.AsString()) << "Reload page" << PHTML::HotLink()
      << PHTML::NonBreakSpace(4)
      << PHTML::HotLink("/") << "Home page" << PHTML::HotLink();

  PServiceHTML::ProcessMacros(request, msg, "html/status.html",
                              PServiceHTML::LoadFromFile | PServiceHTML::NoSignatureForFile);
  return TRUE;
}


///////////////////////////////////////////////////////////////

RegistrationStatusPage::RegistrationStatusPage(MyManager & mgr, const PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "RegistrationStatus")
{
}


#if OPAL_H323
void RegistrationStatusPage::GetH323(StatusMap & copy) const
{
  PWaitAndSignal lock(m_mutex);
  copy = m_h323;
}
#endif

#if OPAL_SIP
void RegistrationStatusPage::GetSIP(StatusMap & copy) const
{
  PWaitAndSignal lock(m_mutex);
  copy = m_sip;
}
#endif

#if OPAL_SKINNY
void RegistrationStatusPage::GetSkinny(StatusMap & copy) const
{
  PWaitAndSignal lock(m_mutex);
  copy = m_skinny;
}
#endif


#if OPAL_LYNC
void RegistrationStatusPage::GetLync(StatusMap & copy) const
{
  PWaitAndSignal lock(m_mutex);
  copy = m_lync;
}
#endif


PString RegistrationStatusPage::LoadText(PHTTPRequest & request)
{
  m_mutex.Wait();

#if OPAL_H323
  m_h323.clear();
  const PList<H323Gatekeeper> gkList = m_manager.GetH323EndPoint().GetGatekeepers();
  for (PList<H323Gatekeeper>::const_iterator gk = gkList.begin(); gk != gkList.end(); ++gk) {
    PString addr = '@' + gk->GetTransport().GetRemoteAddress().GetHostName(true);

    PStringStream status;
    if (gk->IsRegistered())
      status << "Registered";
    else
      status << "Failed: " << gk->GetRegistrationFailReason();

    const PStringList & aliases = gk->GetAliases();
    for (PStringList::const_iterator it = aliases.begin(); it != aliases.end(); ++it)
      m_h323[*it + addr] = status;
  }
#endif

#if OPAL_SIP
  m_sip.clear();
  SIPEndPoint & sipEP = m_manager.GetSIPEndPoint();
  const PStringList & registrations = sipEP.GetRegistrations(true);
  for (PStringList::const_iterator it = registrations.begin(); it != registrations.end(); ++it)
    m_sip[*it] = sipEP.IsRegistered(*it) ? "Registered" : (sipEP.IsRegistered(*it, true) ? "Offline" : "Failed");

#endif

#if OPAL_SKINNY
  m_skinny.clear();
  OpalSkinnyEndPoint * skinnyEP = m_manager.FindEndPointAs<OpalSkinnyEndPoint>(OPAL_PREFIX_SKINNY);
  if (skinnyEP != NULL) {
    PArray<PString> names = skinnyEP->GetPhoneDeviceNames();
    for (PINDEX i = 0; i < names.GetSize(); ++i) {
      OpalSkinnyEndPoint::PhoneDevice * phoneDevice = skinnyEP->GetPhoneDevice(names[i]);
      if (phoneDevice != NULL) {
        PStringStream str;
        str << *phoneDevice;
        PString name, status;
        if (str.Split('\t', name, status))
          m_skinny[name] = status;
      }
    }
  }
#endif

#if OPAL_LYNC
  m_lync.clear();
  OpalLyncEndPoint * lyncEP = m_manager.FindEndPointAs<OpalLyncEndPoint>(OPAL_PREFIX_LYNC);
  if (lyncEP != NULL) {
    PStringArray uris = lyncEP->GetRegisteredUsers();
    for (PINDEX i = 0; i < uris.GetSize(); ++i)
      m_lync[uris[i]] = lyncEP->GetRegisteredUser(uris[i]) ? "Registered" : "Failed";
  }
#endif

  m_mutex.Signal();

  return BaseStatusPage::LoadText(request);
}


const char * RegistrationStatusPage::GetTitle() const
{
  return "OPAL Server Registration Status";
}


void RegistrationStatusPage::CreateContent(PHTML & html, const PStringToString &) const
{
  html << PHTML::TableStart(PHTML::Border1, PHTML::CellPad4)
       << PHTML::TableRow()
       << PHTML::TableHeader() << ' '
       << PHTML::TableHeader(PHTML::NoWrap) << "Name/Address"
       << PHTML::TableHeader(PHTML::NoWrap) << "Status"
#if OPAL_H323
       << PHTML::TableRow()
       << PHTML::TableHeader(PHTML::NoWrap, "rowspan=<!--#macro H323ListenerCount-->")
       << " H.323 Listeners"
       << "<!--#macrostart H323ListenerStatus-->"
           << PHTML::TableRow()
           << PHTML::TableData(PHTML::NoWrap)
           << "<!--#status Address-->"
           << PHTML::TableData(PHTML::NoWrap, PHTML::AlignCentre)
           << "<!--#status Status-->"
       << "<!--#macroend H323ListenerStatus-->"
       << PHTML::TableRow()
       << PHTML::TableHeader(PHTML::NoWrap, "rowspan=<!--#macro H323RegistrationCount-->")
       << " H.323 Gatekeeper"
       << "<!--#macrostart H323RegistrationStatus-->"
           << PHTML::TableRow()
           << PHTML::TableData(PHTML::NoWrap)
           << "<!--#status Name-->"
           << PHTML::TableData(PHTML::NoWrap)
           << "<!--#status Status-->"
       << "<!--#macroend H323RegistrationStatus-->"
#endif // OPAL_H323

#if OPAL_SIP
       << PHTML::TableRow()
       << PHTML::TableHeader(PHTML::NoWrap, "rowspan=<!--#macro SIPListenerCount-->")
       << " SIP Listeners "
       << "<!--#macrostart SIPListenerStatus-->"
           << PHTML::TableRow()
           << PHTML::TableData(PHTML::NoWrap)
           << "<!--#status Address-->"
           << PHTML::TableData(PHTML::NoWrap, PHTML::AlignCentre)
           << "<!--#status Status-->"
       << "<!--#macroend SIPListenerStatus-->"
       << PHTML::TableRow()
       << PHTML::TableHeader(PHTML::NoWrap, "rowspan=<!--#macro SIPRegistrationCount-->")
       << " SIP Registrars "
       << "<!--#macrostart SIPRegistrationStatus-->"
           << PHTML::TableRow()
           << PHTML::TableData(PHTML::NoWrap)
           << "<!--#status Name-->"
           << PHTML::TableData(PHTML::NoWrap)
           << "<!--#status Status-->"
       << "<!--#macroend SIPRegistrationStatus-->"
#endif // OPAL_SIP

#if OPAL_SKINNY
       << PHTML::TableRow()
       << PHTML::TableHeader(PHTML::NoWrap, "rowspan=<!--#macro SkinnyRegistrationCount-->")
       << " SCCP Servers "
       << "<!--#macrostart SkinnyRegistrationStatus-->"
         << PHTML::TableRow()
         << PHTML::TableData(PHTML::NoWrap)
         << "<!--#status Name-->"
         << PHTML::TableData(PHTML::NoWrap)
         << "<!--#status Status-->"
       << "<!--#macroend SkinnyRegistrationStatus-->"
#endif // OPAL_SKINNY

#if OPAL_LYNC
       << PHTML::TableRow()
       << PHTML::TableHeader(PHTML::NoWrap, "rowspan=<!--#macro LyncRegistrationCount-->")
       << " Lync URIs "
       << "<!--#macrostart LyncRegistrationStatus-->"
         << PHTML::TableRow()
         << PHTML::TableData(PHTML::NoWrap)
         << "<!--#status Name-->"
         << PHTML::TableData(PHTML::NoWrap)
         << "<!--#status Status-->"
       << "<!--#macroend LyncRegistrationStatus-->"
#endif // OPAL_LYNC

#if OPAL_PTLIB_NAT
       << PHTML::TableRow()
       << PHTML::TableHeader(PHTML::NoWrap)
       << " STUN Server "
       << PHTML::TableData(PHTML::NoWrap)
       << "<!--#macro STUNServer-->"
       << PHTML::TableData(PHTML::NoWrap)
       << "<!--#macro STUNStatus-->"
#endif // OPAL_PTLIB_NAT

       << PHTML::TableEnd();
}


static PINDEX GetListenerCount(PHTTPRequest & resource, const char * prefix)
{
  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return 2;

  OpalEndPoint * ep = status->m_manager.FindEndPoint(prefix);
  if (ep == NULL)
    return 2;

  return ep->GetListeners().GetSize() + 1;
}


static PString GetListenerStatus(PHTTPRequest & resource, const PString htmlBlock, const char * prefix)
{
  PString substitution;

  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) != NULL) {
    OpalEndPoint * ep = status->m_manager.FindEndPoint(prefix);
    if (ep != NULL) {
      const OpalListenerList & listeners = ep->GetListeners();
      for (OpalListenerList::const_iterator it = listeners.begin(); it != listeners.end(); ++it) {
        PString insert = htmlBlock;
        PServiceHTML::SpliceMacro(insert, "status Address", it->GetLocalAddress());
        PServiceHTML::SpliceMacro(insert, "status Status", it->IsOpen() ? "Active" : "Offline");
        substitution += insert;
      }
    }
  }

  if (substitution.IsEmpty()) {
    substitution = htmlBlock;
    PServiceHTML::SpliceMacro(substitution, "status Address", PHTML::GetNonBreakSpace());
    PServiceHTML::SpliceMacro(substitution, "status Status", "Not listening");
  }

  return substitution;
}


static PINDEX GetRegistrationCount(PHTTPRequest & resource, size_t (RegistrationStatusPage::*func)() const)
{
  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return 2;

  PINDEX count = (status->*func)();
  if (count == 0)
    return 2;

  return count+1;
}


static PString GetRegistrationStatus(PHTTPRequest & resource,
                                     const PString htmlBlock,
                                     void (RegistrationStatusPage::*func)(RegistrationStatusPage::StatusMap &) const)
{
  PString substitution;

  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) != NULL) {
    RegistrationStatusPage::StatusMap statuses;
    (status->*func)(statuses);
    for (RegistrationStatusPage::StatusMap::const_iterator it = statuses.begin(); it != statuses.end(); ++it) {
      PString insert = htmlBlock;
      PServiceHTML::SpliceMacro(insert, "status Name", it->first);
      PServiceHTML::SpliceMacro(insert, "status Status", it->second);
      substitution += insert;
    }
  }

  if (substitution.IsEmpty()) {
    substitution = htmlBlock;
    PServiceHTML::SpliceMacro(substitution, "status Name", PHTML::GetNonBreakSpace());
    PServiceHTML::SpliceMacro(substitution, "status Status", "Not registered");
  }

  return substitution;
}


#if OPAL_H323
PCREATE_SERVICE_MACRO(H323ListenerCount, resource, P_EMPTY)
{
  return GetListenerCount(resource, OPAL_PREFIX_H323);
}


PCREATE_SERVICE_MACRO_BLOCK(H323ListenerStatus, resource, P_EMPTY, htmlBlock)
{
  return GetListenerStatus(resource, htmlBlock, OPAL_PREFIX_H323);
}


PCREATE_SERVICE_MACRO(H323RegistrationCount, resource, P_EMPTY)
{
  return GetRegistrationCount(resource, &RegistrationStatusPage::GetH323Count);
}


PCREATE_SERVICE_MACRO_BLOCK(H323RegistrationStatus, resource, P_EMPTY, htmlBlock)
{
  return GetRegistrationStatus(resource, htmlBlock, &RegistrationStatusPage::GetH323);
}
#endif // OPAL_H323

#if OPAL_SIP
PCREATE_SERVICE_MACRO(SIPListenerCount, resource, P_EMPTY)
{
  return GetListenerCount(resource, OPAL_PREFIX_SIP);
}


PCREATE_SERVICE_MACRO_BLOCK(SIPListenerStatus, resource, P_EMPTY, htmlBlock)
{
  return GetListenerStatus(resource, htmlBlock, OPAL_PREFIX_SIP);
}


PCREATE_SERVICE_MACRO(SIPRegistrationCount, resource, P_EMPTY)
{
  return GetRegistrationCount(resource, &RegistrationStatusPage::GetSIPCount);
}


PCREATE_SERVICE_MACRO_BLOCK(SIPRegistrationStatus, resource, P_EMPTY, htmlBlock)
{
  return GetRegistrationStatus(resource, htmlBlock, &RegistrationStatusPage::GetSIP);
}
#endif // OPAL_SIP

#if OPAL_SKINNY
PCREATE_SERVICE_MACRO(SkinnyRegistrationCount, resource, P_EMPTY)
{
  return GetRegistrationCount(resource, &RegistrationStatusPage::GetSkinnyCount);
}


PCREATE_SERVICE_MACRO_BLOCK(SkinnyRegistrationStatus, resource, P_EMPTY, htmlBlock)
{
  return GetRegistrationStatus(resource, htmlBlock, &RegistrationStatusPage::GetSkinny);
}
#endif // OPAL_SKINNY


#if OPAL_LYNC
PCREATE_SERVICE_MACRO(LyncRegistrationCount, resource, P_EMPTY)
{
  return GetRegistrationCount(resource, &RegistrationStatusPage::GetLyncCount);
}


PCREATE_SERVICE_MACRO_BLOCK(LyncRegistrationStatus, resource, P_EMPTY, htmlBlock)
{
  return GetRegistrationStatus(resource, htmlBlock, &RegistrationStatusPage::GetLync);
}
#endif // OPAL_SKINNY


#if OPAL_PTLIB_NAT
static bool GetSTUN(PHTTPRequest & resource, PSTUNClient * & stun)
{
  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return false;

  stun = dynamic_cast<PSTUNClient *>(status->m_manager.GetNatMethods().GetMethodByName(PSTUNClient::MethodName()));
  return stun != NULL;
}


PCREATE_SERVICE_MACRO(STUNServer, resource, P_EMPTY)
{
  PSTUNClient * stun;
  if (!GetSTUN(resource, stun))
    return PString::Empty();

  PHTML html(PHTML::InBody);
  html << stun->GetServer();

  PIPAddressAndPort ap;
  if (stun->GetServerAddress(ap))
    html << PHTML::BreakLine() << ap;

  return html;
}


PCREATE_SERVICE_MACRO(STUNStatus, resource, P_EMPTY)
{
  PSTUNClient * stun;
  if (!GetSTUN(resource, stun))
    return PString::Empty();

  PHTML html(PHTML::InBody);
  PNatMethod::NatTypes type = stun->GetNatType();
  html << type;

  PIPAddress ip;
  if (stun->GetExternalAddress(ip))
    html << PHTML::BreakLine() << ip;

  return html;
}
#endif // OPAL_PTLIB_NAT


///////////////////////////////////////////////////////////////

CallStatusPage::CallStatusPage(MyManager & mgr, const PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "CallStatus")
{
}


void CallStatusPage::GetCalls(PArray<PString> & copy) const
{
  PWaitAndSignal lock(m_mutex);
  copy = m_calls;
  copy.MakeUnique();
}


PString CallStatusPage::LoadText(PHTTPRequest & request)
{
  m_mutex.Wait();
  m_calls = m_manager.GetAllCalls();
  m_mutex.Signal();
  return BaseStatusPage::LoadText(request);
}


const char * CallStatusPage::GetTitle() const
{
  return "OPAL Server Call Status";
}


void CallStatusPage::CreateContent(PHTML & html, const PStringToString &) const
{
  html << "Current call count: <!--#macro CallCount-->" << PHTML::Paragraph()
       << PHTML::TableStart(PHTML::Border1, PHTML::CellPad4)
       << PHTML::TableRow()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << 'A' << PHTML::NonBreakSpace() << "Party" << PHTML::NonBreakSpace()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << 'B' << PHTML::NonBreakSpace() << "Party" << PHTML::NonBreakSpace()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "Duration" << PHTML::NonBreakSpace()
       << "<!--#macrostart CallStatus-->"
         << PHTML::TableRow()
         << PHTML::TableData(PHTML::NoWrap)
         << "<!--#status A-Party-->"
         << PHTML::TableData(PHTML::NoWrap)
         << "<!--#status B-Party-->"
         << PHTML::TableData()
         << "<!--#status Duration-->"
         << PHTML::TableData()
         << PHTML::SubmitButton("Clear", "!--#status Token--")
       << "<!--#macroend CallStatus-->"
       << PHTML::TableEnd();
}


bool CallStatusPage::OnPostControl(const PStringToString & data, PHTML & msg)
{
  bool gotOne = false;

  for (PStringToString::const_iterator it = data.begin(); it != data.end(); ++it) {
    PString key = it->first;
    PString value = it->second;
    if (value == "Clear") {
      PSafePtr<OpalCall> call = m_manager.FindCallWithLock(key, PSafeReference);
      if (call != NULL) {
        msg << PHTML::Heading(2) << "Clearing call " << *call << PHTML::Heading(2);
        call->Clear();
        gotOne = true;
      }
    }
  }

  return gotOne;
}


PCREATE_SERVICE_MACRO(CallCount, resource, P_EMPTY)
{
  CallStatusPage * status = dynamic_cast<CallStatusPage *>(resource.m_resource);
  return PAssertNULL(status) == NULL ? 0 : status->GetCallCount();
}


static PString BuildPartyStatus(const PString & party, const PString & name, const PString & identity)
{
  if (name.IsEmpty() && identity.IsEmpty())
    return party;

  if (identity.IsEmpty() || party == identity)
    return PSTRSTRM(name << "<BR>" << party);

  if (name.IsEmpty() || name == identity)
    return PSTRSTRM(identity << "<BR>" << party);

  return PSTRSTRM(name << "<BR>" << identity << "<BR>" << party);
}


PCREATE_SERVICE_MACRO_BLOCK(CallStatus,resource,P_EMPTY,htmlBlock)
{
  CallStatusPage * status = dynamic_cast<CallStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return PString::Empty();

  PString substitution;

  PArray<PString> calls;
  status->GetCalls(calls);
  for (PINDEX i = 0; i < calls.GetSize(); ++i) {
    PSafePtr<OpalCall> call = status->m_manager.FindCallWithLock(calls[i], PSafeReadOnly);
    if (call == NULL)
      continue;

    // make a copy of the repeating html chunk
    PString insert = htmlBlock;

    PServiceHTML::SpliceMacro(insert, "status Token",    call->GetToken());
    PServiceHTML::SpliceMacro(insert, "status A-Party",  BuildPartyStatus(call->GetPartyA(), call->GetNameA(), call->GetIdentityA()));
    PServiceHTML::SpliceMacro(insert, "status B-Party",  BuildPartyStatus(call->GetPartyB(), call->GetNameB(), call->GetIdentityB()));

    PStringStream duration;
    duration.precision(0);
    duration.width(5);
    if (call->GetEstablishedTime().IsValid())
      duration << call->GetEstablishedTime().GetElapsed();
    else
      duration << '(' << call->GetStartTime().GetElapsed() << ')';
    PServiceHTML::SpliceMacro(insert, "status Duration", duration);

    // Then put it into the page, moving insertion point along after it.
    substitution += insert;
  }

  return substitution;
}


///////////////////////////////////////////////////////////////

#if OPAL_H323

GkStatusPage::GkStatusPage(MyManager & mgr, const PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "GkStatus")
  , m_gkServer(mgr.FindEndPointAs<MyH323EndPoint>(OPAL_PREFIX_H323)->GetGatekeeperServer())
{
}


const char * GkStatusPage::GetTitle() const
{
  return "OPAL Gatekeeper Status";
}


void GkStatusPage::CreateContent(PHTML & html, const PStringToString &) const
{
  html << PHTML::TableStart(PHTML::Border1)
       << PHTML::TableRow()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "End" << PHTML::NonBreakSpace() << "Point" << PHTML::NonBreakSpace() << "Identifier" << PHTML::NonBreakSpace()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "Call" << PHTML::NonBreakSpace() << "Signal" << PHTML::NonBreakSpace() << "Addresses" << PHTML::NonBreakSpace()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "Aliases" << PHTML::NonBreakSpace()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "Application" << PHTML::NonBreakSpace()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "Active" << PHTML::NonBreakSpace() << "Calls" << PHTML::NonBreakSpace()
       << "<!--#macrostart H323EndPointStatus-->"
       << PHTML::TableRow()
       << PHTML::TableData()
       << "<!--#status EndPointIdentifier-->"
       << PHTML::TableData()
       << "<!--#status CallSignalAddresses-->"
       << PHTML::TableData(PHTML::NoWrap)
       << "<!--#status EndPointAliases-->"
       << PHTML::TableData(PHTML::NoWrap)
       << "<!--#status Application-->"
       << PHTML::TableData("align=center")
       << "<!--#status ActiveCalls-->"
       << PHTML::TableData()
       << PHTML::SubmitButton("Unregister", "!--#status EndPointIdentifier--")
       << "<!--#macroend H323EndPointStatus-->"
       << PHTML::TableEnd();
}


PBoolean GkStatusPage::OnPostControl(const PStringToString & data, PHTML & msg)
{
  bool gotOne = false;

  for (PStringToString::const_iterator it = data.begin(); it != data.end(); ++it) {
    PString id = it->first;
    if (it->second == "Unregister" && m_gkServer.ForceUnregister(id)) {
      msg << PHTML::Heading(2) << "Unregistered " << id << PHTML::Heading(2);
      gotOne = true;
    }
  }

  return gotOne;
}


#endif //OPAL_H323


///////////////////////////////////////////////////////////////

#if OPAL_SIP

RegistrarStatusPage::RegistrarStatusPage(MyManager & mgr, const PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "RegistrarStatus")
  , m_registrar(*mgr.FindEndPointAs<MySIPEndPoint>(OPAL_PREFIX_SIP))
{
}


const char * RegistrarStatusPage::GetTitle() const
{
  return "OPAL Registrar Status";
}


void RegistrarStatusPage::CreateContent(PHTML & html, const PStringToString &) const
{
  html << PHTML::TableStart(PHTML::Border1)
       << PHTML::TableRow()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "End" << PHTML::NonBreakSpace() << "Point" << PHTML::NonBreakSpace() << "Identifier" << PHTML::NonBreakSpace()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "Call" << PHTML::NonBreakSpace() << "Signal" << PHTML::NonBreakSpace() << "Addresses" << PHTML::NonBreakSpace()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "Application" << PHTML::NonBreakSpace()
       << PHTML::TableHeader()
       << PHTML::NonBreakSpace() << "Active" << PHTML::NonBreakSpace() << "Calls" << PHTML::NonBreakSpace()
       << "<!--#macrostart SIPEndPointStatus-->"
       << PHTML::TableRow()
       << PHTML::TableData()
       << "<!--#status EndPointIdentifier-->"
       << PHTML::TableData()
       << "<!--#status CallSignalAddresses-->"
       << PHTML::TableData(PHTML::NoWrap)
       << "<!--#status Application-->"
       << PHTML::TableData("align=center")
       << "<!--#status ActiveCalls-->"
       << PHTML::TableData()
       << PHTML::SubmitButton("Unregister", "!--#status EndPointIdentifier--")
       << "<!--#macroend SIPEndPointStatus-->"
       << PHTML::TableEnd();
}


PBoolean RegistrarStatusPage::OnPostControl(const PStringToString & data, PHTML & msg)
{
  bool gotOne = false;

  for (PStringToString::const_iterator it = data.begin(); it != data.end(); ++it) {
    PString aor = it->first;
    if (it->second == "Unregister" && m_registrar.ForceUnregister(aor)) {
      msg << PHTML::Heading(2) << "Unregistered " << aor << PHTML::Heading(2);
      gotOne = true;
    }
  }

  return gotOne;
}

#endif //OPAL_SIP


// End of File ///////////////////////////////////////////////////////////////
