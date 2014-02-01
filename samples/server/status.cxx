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
 * $Revision$
 * $Author$
 * $Date$
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
  string = html;

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
      << "&nbsp;&nbsp;&nbsp;&nbsp;"
      << PHTML::HotLink("/") << "Home page" << PHTML::HotLink();

  PServiceHTML::ProcessMacros(request, msg, "html/status.html",
                              PServiceHTML::LoadFromFile | PServiceHTML::NoSignatureForFile);
  return TRUE;
}


///////////////////////////////////////////////////////////////

ClearLogPage::ClearLogPage(MyManager & mgr, const PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "ClearLogFile")
{
  m_refreshRate = 0;
}


const char * ClearLogPage::GetTitle() const
{
  return "OPAL Server Clear Log File";
}


void ClearLogPage::CreateContent(PHTML & html, const PStringToString &) const
{
  html << PHTML::Paragraph() << "<center>" << PHTML::SubmitButton("Clear Log File", "Clear Log File");
}


bool ClearLogPage::OnPostControl(const PStringToString & data, PHTML & msg)
{
  if (data("Clear Log File") == "Clear Log File") {
    PSystemLogToFile * logFile = dynamic_cast<PSystemLogToFile *>(&PSystemLog::GetTarget());
    if (logFile == NULL)
      msg << "Not logging to a file";
    else if (logFile->Clear())
      msg << "Cleared log file " << logFile->GetFilePath();
    else
      msg << "Could not clear log file " << logFile->GetFilePath() << PHTML::Paragraph()
          << "Probably just in use, you can usually just try again.";
  }

  return true;
}


///////////////////////////////////////////////////////////////

#if OPAL_H323 | OPAL_SIP | OPAL_SKINNY

RegistrationStatusPage::RegistrationStatusPage(MyManager & mgr, const PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "RegistrationStatus")
{
}


PString RegistrationStatusPage::LoadText(PHTTPRequest & request)
{
#if OPAL_SKINNY
  OpalSkinnyEndPoint * ep = m_manager.FindEndPointAs<OpalSkinnyEndPoint>(OPAL_PREFIX_SKINNY);
  if (ep != NULL)
    m_skinnyNames = ep->GetPhoneDeviceNames();
#endif
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
       << PHTML::TableHeader(PHTML::NoWrap) << "Address"
       << PHTML::TableHeader(PHTML::NoWrap) << "Status"
#if OPAL_H323
       << PHTML::TableRow()
       << PHTML::TableHeader(PHTML::NoWrap)
       << " H.323 Gatekeeper"
       << PHTML::TableData(PHTML::NoWrap)
       << "<!--#macro H323GatekeeperAddress-->"
       << PHTML::TableData(PHTML::NoWrap)
       << "<!--#macro H323RegistrationStatus-->"
#endif // OPAL_H323

#if OPAL_SIP
       << "<!--#macrostart SIPRegistrationStatus-->"
           << PHTML::TableRow()
           << PHTML::TableHeader(PHTML::NoWrap)
           << " SIP Registrar "
           << PHTML::TableData(PHTML::NoWrap)
           << "<!--#status AoR-->"
           << PHTML::TableData(PHTML::NoWrap)
           << "<!--#status State-->"
       << "<!--#macroend SIPRegistrationStatus-->"
#endif // OPAL_SIP

#if OPAL_SKINNY
       << PHTML::TableRow()
       << PHTML::TableHeader(PHTML::NoWrap, "rowspan=<!--#macro SkinnyCount-->")
       << " SCCP Server "
       << "<!--#macrostart SkinnyRegistrationStatus-->"
         << PHTML::TableRow()
         << PHTML::TableData(PHTML::NoWrap)
         << "<!--#status Name-->"
         << PHTML::TableData(PHTML::NoWrap)
         << "<!--#status Status-->"
       << "<!--#macroend SkinnyRegistrationStatus-->"
#endif // OPAL_SKINNY
       << PHTML::TableEnd();
}


#if OPAL_H323
PCREATE_SERVICE_MACRO(H323GatekeeperAddress, resource, P_EMPTY)
{
  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return PString::Empty();

  PStringStream strm;

  H323Gatekeeper * gk = status->m_manager.GetH323EndPoint().GetGatekeeper();
  if (gk != NULL)
    strm << *gk;
  else
    strm << "&nbsp;";

  return strm;
}

PCREATE_SERVICE_MACRO(H323RegistrationStatus, resource, P_EMPTY)
{
  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return PString::Empty();

  H323Gatekeeper * gk = status->m_manager.GetH323EndPoint().GetGatekeeper();
  if (gk == NULL)
    return "Not registered";

  PStringStream strm;
  if (gk->IsRegistered())
    strm  << "Registered";
  else
    strm << "Failed: " << gk->GetRegistrationFailReason();
  return strm;
}
#endif // OPAL_H323

#if OPAL_SIP
PCREATE_SERVICE_MACRO_BLOCK(SIPRegistrationStatus,resource,P_EMPTY,htmlBlock)
{
  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return PString::Empty();

  PString substitution;

  SIPEndPoint & sip = status->m_manager.GetSIPEndPoint();
  PStringList registrations = sip.GetRegistrations(true);
  for (PStringList::iterator it = registrations.begin(); it != registrations.end(); ++it) {
    // make a copy of the repeating html chunk
    PString insert = htmlBlock;

    PServiceHTML::SpliceMacro(insert, "status AoR",   *it);
    PServiceHTML::SpliceMacro(insert, "status State", sip.IsRegistered(*it) ? "Registered" : (sip.IsRegistered(*it, true) ? "Offline" : "Failed"));

    // Then put it into the page, moving insertion point along after it.
    substitution += insert;
  }

  if (substitution.IsEmpty()) {
    substitution = htmlBlock;
    PServiceHTML::SpliceMacro(substitution, "status AoR", "&nbsp;");
    PServiceHTML::SpliceMacro(substitution, "status State", "Not registered");
  }

  return substitution;
}
#endif // OPAL_SIP

#if OPAL_SKINNY
PCREATE_SERVICE_MACRO(SkinnyCount, resource, P_EMPTY)
{
  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return PString::Empty();

  return PString(status->GetSkinnyNames().GetSize()+1);
}


PCREATE_SERVICE_MACRO_BLOCK(SkinnyRegistrationStatus, resource, P_EMPTY, htmlBlock)
{
  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return PString::Empty();

  PString substitution;

  OpalSkinnyEndPoint * ep = status->m_manager.FindEndPointAs<OpalSkinnyEndPoint>(OPAL_PREFIX_SKINNY);
  if (ep != NULL) {
    const PStringArray & names = status->GetSkinnyNames();
    for (PINDEX i = 0; i < names.GetSize(); ++i) {
      OpalSkinnyEndPoint::PhoneDevice * phoneDevice = ep->GetPhoneDevice(names[i]);
      if (phoneDevice != NULL) {
        // make a copy of the repeating html chunk
        PString insert = htmlBlock;

        PStringStream str;
        str << *phoneDevice;
        PString name, status;
        str.Split('\t', name, status);
        PServiceHTML::SpliceMacro(insert, "status Name", name);
        PServiceHTML::SpliceMacro(insert, "status Status", status);

        // Then put it into the page, moving insertion point along after it.
        substitution += insert;
      }
      else {
        PServiceHTML::SpliceMacro(substitution, "status Name", names[i]);
        PServiceHTML::SpliceMacro(substitution, "status Status", "Unregistered");
      }
    }
  }

  if (substitution.IsEmpty()) {
    substitution = htmlBlock;
    PServiceHTML::SpliceMacro(substitution, "status Name", "&nbsp;");
    PServiceHTML::SpliceMacro(substitution, "status Status", "Nothing registered");
  }

  return substitution;
}
#endif // OPAL_SKINNY


#endif // OPAL_H323 | OPAL_SIP | OPAL_SKINNY

///////////////////////////////////////////////////////////////

CallStatusPage::CallStatusPage(MyManager & mgr, const PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "CallStatus")
{
}


const char * CallStatusPage::GetTitle() const
{
  return "OPAL Server Call Status";
}


void CallStatusPage::CreateContent(PHTML & html, const PStringToString &) const
{
  html << PHTML::TableStart(PHTML::Border1, PHTML::CellPad4)
       << PHTML::TableRow()
       << PHTML::TableHeader()
       << "&nbsp;A&nbsp;Party&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;B&nbsp;Party&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Duration&nbsp;"
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


PCREATE_SERVICE_MACRO_BLOCK(CallStatus,resource,P_EMPTY,htmlBlock)
{
  CallStatusPage * status = dynamic_cast<CallStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return PString::Empty();

  PString substitution;

  PArray<PString> calls = status->m_manager.GetAllCalls();
  for (PINDEX i = 0; i < calls.GetSize(); ++i) {
    PSafePtr<OpalCall> call = status->m_manager.FindCallWithLock(calls[i], PSafeReadOnly);
    if (call == NULL)
      continue;

    // make a copy of the repeating html chunk
    PString insert = htmlBlock;

    PServiceHTML::SpliceMacro(insert, "status Token",    call->GetToken());
    PServiceHTML::SpliceMacro(insert, "status A-Party",  call->GetPartyA());
    PServiceHTML::SpliceMacro(insert, "status B-Party",  call->GetPartyB());

    PStringStream duration;
    duration.precision(0);
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
  , m_gkServer(mgr.FindEndPointAs<MyH323EndPoint>("h323")->GetGatekeeperServer())
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
       << "&nbsp;End&nbsp;Point&nbsp;Identifier&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Call&nbsp;Signal&nbsp;Addresses&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Aliases&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Application&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Active&nbsp;Calls&nbsp;"
       << "<!--#macrostart EndPointStatus-->"
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
       << "<!--#macroend EndPointStatus-->"
       << PHTML::TableEnd();
}


PBoolean GkStatusPage::OnPostControl(const PStringToString & data, PHTML & msg)
{
  bool gotOne = false;

  for (PStringToString::const_iterator it = data.begin(); it != data.end(); ++it) {
    PString key = it->first;
    PString value = it->second;
    if (value == "Unregister") {
      PSafePtr<H323RegisteredEndPoint> ep = m_gkServer.FindEndPointByIdentifier(key);
      if (ep != NULL) {
        msg << PHTML::Heading(2) << "Unregistered endpoint " << *ep << PHTML::Heading(2);
        ep->Unregister();
        gotOne = true;
      }
    }
  }

  return gotOne;
}


#endif //OPAL_H323


// End of File ///////////////////////////////////////////////////////////////
