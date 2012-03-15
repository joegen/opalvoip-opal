/*
 * main.cxx
 *
 * PWLib application source file for OPAL Gateway
 *
 * Main program entry point.
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
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
#include "custom.h"


PCREATE_PROCESS(MyProcess);


const WORD DefaultHTTPPort = 1719;
static const char UsernameKey[] = "Username";
static const char PasswordKey[] = "Password";
static const char LogLevelKey[] = "Log Level";
static const char DefaultAddressFamilyKey[] = "AddressFamily";
#if OPAL_PTLIB_SSL
static const char HTTPCertificateFileKey[]  = "HTTP Certificate";
#endif
static const char HttpPortKey[] = "HTTP Port";
static const char MediaBypassKey[] = "Media Bypass";
static const char PreferredMediaKey[] = "Preferred Media";
static const char RemovedMediaKey[] = "Removed Media";
static const char MinJitterKey[] = "Minimum Jitter";
static const char MaxJitterKey[] = "Maximum Jitter";
static const char TCPPortBaseKey[] = "TCP Port Base";
static const char TCPPortMaxKey[] = "TCP Port Max";
static const char UDPPortBaseKey[] = "UDP Port Base";
static const char UDPPortMaxKey[] = "UDP Port Max";
static const char RTPPortBaseKey[] = "RTP Port Base";
static const char RTPPortMaxKey[] = "RTP Port Max";
static const char RTPTOSKey[] = "RTP Type of Service";
static const char STUNServerKey[] = "STUN Server";

static const char SIPUsernameKey[] = "SIP User Name";
static const char SIPProxyKey[] = "SIP Proxy URL";
static const char SIPRegistrarKey[] = "SIP Registrar";
static const char SIPAuthIDKey[] = "SIP Auth ID";
static const char SIPPasswordKey[] = "SIP Password";
static const char SIPListenersKey[] = "SIP Interfaces";

static const char LIDKey[] = "Line Interface Devices";
static const char EnableCAPIKey[] = "CAPI ISDN";

static const char VXMLKey[] = "IVR VXML URL";

#define ROUTES_SECTION "Routes"
#define ROUTES_KEY "Route"

static const char RouteAPartyKey[] = "A Party";
static const char RouteBPartyKey[] = "B Party";
static const char RouteDestKey[]   = "Destination";

static const char * const DefaultRoutes[] = {
  ".*:.*\t#|.*:#=ivr:",
  "pots:\\+*[0-9]+ = tel:<dn>",
  "pstn:\\+*[0-9]+ = tel:<dn>",
  "capi:\\+*[0-9]+ = tel:<dn>",
  "h323:\\+*[0-9]+ = tel:<dn>",
  "sip:\\+*[0-9]+@.* = tel:<dn>",
  "h323:.+@.+ = sip:<da>",
  "h323:.* = sip:<dn>@",
  "sip:.* = h323:<dn>",
  "tel:[0-9]+\\*[0-9]+\\*[0-9]+\\*[0-9]+ = sip:<dn2ip>",
  "tel:.*=sip:<dn>"
};


///////////////////////////////////////////////////////////////////////////////

MyProcess::MyProcess()
  : MyProcessAncestor(ProductInfo)
{
}


PBoolean MyProcess::OnStart()
{
  // change to the default directory to the one containing the executable
  PDirectory exeDir = GetFile().GetDirectory();

#if defined(_WIN32) && defined(_DEBUG)
  // Special check to aid in using DevStudio for debugging.
  if (exeDir.Find("\\Debug\\") != P_MAX_INDEX)
    exeDir = exeDir.GetParent();
#endif
  exeDir.Change();

  httpNameSpace.AddResource(new PHTTPDirectory("data", "data"));
  httpNameSpace.AddResource(new PServiceHTTPDirectory("html", "html"));

  return PHTTPServiceProcess::OnStart();
}


void MyProcess::OnStop()
{
  m_manager.ShutDownEndpoints();
  PHTTPServiceProcess::OnStop();
}


void MyProcess::OnControl()
{
  // This function get called when the Control menu item is selected in the
  // tray icon mode of the service.
  PStringStream url;
  url << "http://";

  PString host = PIPSocket::GetHostName();
  PIPSocket::Address addr;
  if (PIPSocket::GetHostAddress(host, addr))
    url << host;
  else
    url << "localhost";

  url << ':' << DefaultHTTPPort;

  PURL::OpenBrowser(url);
}


void MyProcess::OnConfigChanged()
{
}



PBoolean MyProcess::Initialise(const char * initMsg)
{
  PConfig cfg("Parameters");

  // Sert log level as early as possible
  SetLogLevel((PSystemLog::Level)cfg.GetInteger(LogLevelKey, GetLogLevel()));
#if PTRACING
  PTrace::SetLevel(GetLogLevel());
  PTrace::ClearOptions(PTrace::Timestamp);
  PTrace::SetOptions(PTrace::DateAndTime);
#endif

  // Get the HTTP basic authentication info
  PString username = cfg.GetString(UsernameKey);
  PString password = PHTTPPasswordField::Decrypt(cfg.GetString(PasswordKey));

  PString addressFamily = cfg.GetString(DefaultAddressFamilyKey, "IPV4");
#if P_HAS_IPV6
  if(addressFamily *= "IPV6")
	 PIPSocket::SetDefaultIpAddressFamilyV6();
#endif

  PHTTPSimpleAuth authority(GetName(), username, password);

  // Create the parameters URL page, and start adding fields to it
  PConfigPage * rsrc = new PConfigPage(*this, "Parameters", "Parameters", authority);

  // HTTP authentication username/password
  rsrc->Add(new PHTTPStringField(UsernameKey, 25, username,
            "User name to access HTTP user interface for gateway."));
  rsrc->Add(new PHTTPPasswordField(PasswordKey, 25, password));

  // Log level for messages
  rsrc->Add(new PHTTPIntegerField(LogLevelKey,
                                  PSystemLog::Fatal, PSystemLog::NumLogLevels-1,
                                  GetLogLevel(),
                                  "1=Fatal only, 2=Errors, 3=Warnings, 4=Info, 5=Debug"));

#if OPAL_PTLIB_SSL
  // SSL certificate file.
  PString certificateFile = cfg.GetString(HTTPCertificateFileKey);
  rsrc->Add(new PHTTPStringField(HTTPCertificateFileKey, 25, certificateFile,
            "Certificate for HTTPS user interface, if empty HTTP is used."));
  if (certificateFile.IsEmpty())
    disableSSL = true;
  else if (!SetServerCertificate(certificateFile, true)) {
    PSYSTEMLOG(Fatal, "MyProcess\tCould not load certificate \"" << certificateFile << '"');
    return false;
  }
#endif

  // HTTP Port number to use.
  WORD httpPort = (WORD)cfg.GetInteger(HttpPortKey, DefaultHTTPPort);
  rsrc->Add(new PHTTPIntegerField(HttpPortKey, 1, 32767, httpPort,
            "Port for HTTP user interface for gateway."));

  // Initialise the core of the system
  if (!m_manager.Initialise(cfg, rsrc))
    return false;

  // Finished the resource to add, generate HTML for it and add to name space
  PServiceHTML html("System Parameters");
  rsrc->BuildHTML(html);
  httpNameSpace.AddResource(rsrc, PHTTPSpace::Overwrite);


#if OPAL_H323
  // Create the status page
  httpNameSpace.AddResource(new GkStatusPage(m_manager, authority), PHTTPSpace::Overwrite);
#endif // OPAL_H323


  // Create the home page
  static const char welcomeHtml[] = "welcome.html";
  if (PFile::Exists(welcomeHtml))
    httpNameSpace.AddResource(new PServiceHTTPFile(welcomeHtml, true), PHTTPSpace::Overwrite);
  else {
    PHTML html;
    html << PHTML::Title("Welcome to " + GetName())
         << PHTML::Body()
         << GetPageGraphic()
         << PHTML::Paragraph() << "<center>"

         << PHTML::HotLink("Parameters") << "Parameters" << PHTML::HotLink()
#if OPAL_H323
         << PHTML::Paragraph()
         << PHTML::HotLink("GkStatus") << "Gatekeeper Status" << PHTML::HotLink()
#endif // OPAL_H323
         << PHTML::Paragraph();

    if (PIsDescendant(&PSystemLog::GetTarget(), PSystemLogToFile))
      html << PHTML::HotLink("logfile.txt") << "Full Log File" << PHTML::HotLink()
           << PHTML::BreakLine()
           << PHTML::HotLink("tail_logfile") << "Tail Log File" << PHTML::HotLink()
           << PHTML::Paragraph();
 
    html << PHTML::HRule()
         << GetCopyrightText()
         << PHTML::Body();
    httpNameSpace.AddResource(new PServiceHTTPString("welcome.html", html), PHTTPSpace::Overwrite);
  }

  // set up the HTTP port for listening & start the first HTTP thread
  if (ListenForHTTP(httpPort))
    PSYSTEMLOG(Info, "Opened master socket(s) for HTTP: " << m_httpListeningSockets.front().GetPort());
  else {
    PSYSTEMLOG(Fatal, "Cannot run without HTTP");
    return false;
  }

  PSYSTEMLOG(Info, "Service " << GetName() << ' ' << initMsg);
  return true;
}


void MyProcess::Main()
{
  Suspend();
}


///////////////////////////////////////////////////////////////

MyManager::MyManager()
  : m_allowMediaBypass(true)
#if OPAL_H323
  , m_h323EP(NULL)
#endif
#if OPAL_SIP
  , m_sipEP(NULL)
#endif
#if OPAL_LID
  , m_potsEP(NULL)
#endif
#if OPAL_CAPI
  , m_capiEP(NULL)
  , m_enableCAPI(true)
#endif
#if OPAL_PTLIB_EXPAT
  , m_ivrEP(NULL)
#endif
{
#if OPAL_VIDEO
  SetAutoStartReceiveVideo(false);
  SetAutoStartTransmitVideo(false);
#endif
}


MyManager::~MyManager()
{
}


PBoolean MyManager::Initialise(PConfig & cfg, PConfigPage * rsrc)
{
  PHTTPFieldArray * fieldArray;

  // Create all the endpoints

#if OPAL_H323
  if (m_h323EP == NULL)
    m_h323EP = new MyH323EndPoint(*this);
#endif

#if OPAL_SIP
  if (m_sipEP == NULL)
    m_sipEP = new SIPEndPoint(*this);
#endif

#if OPAL_LID
  if (m_potsEP == NULL)
    m_potsEP = new OpalLineEndPoint(*this);
#endif

#if OPAL_LID
  if (m_capiEP == NULL)
    m_capiEP = new OpalCapiEndPoint(*this);
#endif

#if OPAL_PTLIB_EXPAT
  if (m_ivrEP == NULL)
    m_ivrEP = new OpalIVREndPoint(*this);
#endif

  // General parameters for all endpoint types
  m_allowMediaBypass = cfg.GetBoolean(MediaBypassKey);
  rsrc->Add(new PHTTPBooleanField(MediaBypassKey, m_allowMediaBypass,
            "Allow media to bypass gateway and be routed directly between the endpoints."));

  fieldArray = new PHTTPFieldArray(new PHTTPStringField(PreferredMediaKey, 25,
                                   "", "Preference order for codecs to be offered to remotes"), true);
  PStringArray formats = fieldArray->GetStrings(cfg);
  if (formats.GetSize() > 0)
    SetMediaFormatOrder(formats);
  else
    fieldArray->SetStrings(cfg, GetMediaFormatOrder());
  rsrc->Add(fieldArray);

  fieldArray = new PHTTPFieldArray(new PHTTPStringField(RemovedMediaKey, 25,
                                   "", "Codecs to be prevented from being used"), true);
  SetMediaFormatMask(fieldArray->GetStrings(cfg));
  rsrc->Add(fieldArray);

  SetAudioJitterDelay(cfg.GetInteger(MinJitterKey, GetMinAudioJitterDelay()),
                      cfg.GetInteger(MaxJitterKey, GetMaxAudioJitterDelay()));
  rsrc->Add(new PHTTPIntegerField(MinJitterKey, 20, 2000, GetMinAudioJitterDelay(),
            "ms", "Minimum jitter buffer size"));
  rsrc->Add(new PHTTPIntegerField(MaxJitterKey, 20, 2000, GetMaxAudioJitterDelay(),
            "ms", " Maximum jitter buffer size"));

  SetTCPPorts(cfg.GetInteger(TCPPortBaseKey, GetTCPPortBase()),
              cfg.GetInteger(TCPPortMaxKey, GetTCPPortMax()));
  SetUDPPorts(cfg.GetInteger(UDPPortBaseKey, GetUDPPortBase()),
              cfg.GetInteger(UDPPortMaxKey, GetUDPPortMax()));
  SetRtpIpPorts(cfg.GetInteger(RTPPortBaseKey, GetRtpIpPortBase()),
                cfg.GetInteger(RTPPortMaxKey, GetRtpIpPortMax()));

  rsrc->Add(new PHTTPIntegerField(TCPPortBaseKey, 0, 65535, GetTCPPortBase(),
            "", "Base of port range for allocating TCP streams, e.g. H.323 signalling channel"));
  rsrc->Add(new PHTTPIntegerField(TCPPortMaxKey,  0, 65535, GetTCPPortMax(),
            "", "Maximum of port range for allocating TCP streams"));
  rsrc->Add(new PHTTPIntegerField(UDPPortBaseKey, 0, 65535, GetUDPPortBase(),
            "", "Base of port range for allocating UDP streams, e.g. SIP signalling channel"));
  rsrc->Add(new PHTTPIntegerField(UDPPortMaxKey,  0, 65535, GetUDPPortMax(),
            "", "Maximum of port range for allocating UDP streams"));
  rsrc->Add(new PHTTPIntegerField(RTPPortBaseKey, 0, 65535, GetRtpIpPortBase(),
            "", "Base of port range for allocating RTP/UDP streams"));
  rsrc->Add(new PHTTPIntegerField(RTPPortMaxKey,  0, 65535, GetRtpIpPortMax(),
            "", "Maximum of port range for allocating RTP/UDP streams"));

  SetMediaTypeOfService(cfg.GetInteger(RTPTOSKey, GetMediaTypeOfService()));
  rsrc->Add(new PHTTPIntegerField(RTPTOSKey,  0, 255, GetMediaTypeOfService(),
            "", "Value for DIFSERV Quality of Service"));

  PString STUNServer = cfg.GetString(STUNServerKey, GetSTUNServer());
  SetSTUNServer(STUNServer);
  rsrc->Add(new PHTTPStringField(STUNServerKey, 25, STUNServer,
            "STUN server IP/hostname for NAT traversal"));

#if OPAL_H323
  if (!m_h323EP->Initialise(cfg, rsrc))
    return false;
#endif

#if OPAL_SIP
  // Add SIP parameters
  m_sipEP->SetDefaultLocalPartyName(cfg.GetString(SIPUsernameKey, m_sipEP->GetDefaultLocalPartyName()));
  rsrc->Add(new PHTTPStringField(SIPUsernameKey, 25, m_sipEP->GetDefaultLocalPartyName(),
            "SIP local user name"));

  SIPRegister::Params registrar;
  registrar.m_remoteAddress = cfg.GetString(SIPRegistrarKey);
  rsrc->Add(new PHTTPStringField(SIPRegistrarKey, 25, registrar.m_registrarAddress,
            "SIP registrar/proxy IP address, hostname or domain"));

  registrar.m_authID = cfg.GetString(SIPAuthIDKey);
  rsrc->Add(new PHTTPStringField(SIPAuthIDKey, 25, registrar.m_authID,
            "SIP registrar/proxy authentication identifier, defaults to local user name"));

  registrar.m_password = cfg.GetString(SIPPasswordKey);
  rsrc->Add(new PHTTPStringField(SIPPasswordKey, 25, registrar.m_password,
            "SIP registrar/proxy authentication password"));

  PString proxy = m_sipEP->GetProxy().AsString();
  m_sipEP->SetProxy(cfg.GetString(SIPProxyKey, proxy));
  rsrc->Add(new PHTTPStringField(SIPProxyKey, 25, proxy,
            "SIP outbound proxy IP/hostname"));

  fieldArray = new PHTTPFieldArray(new PHTTPStringField(SIPListenersKey, 25,
                                   "", "Local network interfaces to listen for SIP, blank means all"), false);
  if (!m_sipEP->StartListeners(fieldArray->GetStrings(cfg))) {
    PSYSTEMLOG(Error, "Could not open any SIP listeners!");
  }
  rsrc->Add(fieldArray);

  if (!registrar.m_registrarAddress) {
    PString aor;
    if (m_sipEP->Register(registrar, aor)) {
      PSYSTEMLOG(Info, "Registered " << aor);
    }
    else {
      PSYSTEMLOG(Error, "Could not register with registrar!");
    }
  }

#endif

#if OPAL_LID
  // Add POTS and PSTN endpoints
  fieldArray = new PHTTPFieldArray(new PHTTPSelectField(LIDKey, OpalLineInterfaceDevice::GetAllDevices(),
                                   0, "Line Interface Devices to monitor, if any"), false);
  PStringArray devices = fieldArray->GetStrings(cfg);
  if (!m_potsEP->AddDeviceNames(devices))
    PSYSTEMLOG(Error, "No LID devices!");
  rsrc->Add(fieldArray);
#endif // OPAL_LID


#if OPAL_CAPI
  m_enableCAPI = cfg.GetBoolean(EnableCAPIKey);
  rsrc->Add(new PHTTPBooleanField(EnableCAPIKey, m_enableCAPI, "Enable CAPI ISDN controller(s), if available"));
  if (m_enableCAPI) {
    if (m_capiEP->OpenControllers() == 0)
      PSYSTEMLOG(Error, "No CAPI controllers!");
  }
#endif


#if OPAL_PTLIB_EXPAT
  // Set IVR protocol handler
  PString vxml = cfg.GetString(VXMLKey);
  rsrc->Add(new PHTTPStringField(VXMLKey, 60, vxml,
            "URL for Interactive Voice Response VXML script"));
  if (!vxml)
    m_ivrEP->SetDefaultVXML(vxml);
#endif


  // Routing
  RouteTable routes;
  PINDEX arraySize = cfg.GetInteger(ROUTES_SECTION, ROUTES_KEY" Array Size");
  for (PINDEX i = 0; i < arraySize; i++) {
    PString section(PString::Printf, ROUTES_SECTION"\\"ROUTES_KEY" %u", i+1);
    RouteEntry * entry = new RouteEntry(cfg.GetString(RouteAPartyKey),
                                        cfg.GetString(RouteBPartyKey),
                                        cfg.GetString(RouteDestKey));
    if (entry->IsValid())
      routes.Append(entry);
    else
      delete entry;
  }

  if (routes.IsEmpty()) {
    arraySize = 0;
    for (PINDEX i = 0; i < PARRAYSIZE(DefaultRoutes); ++i) {
      RouteEntry * entry = new RouteEntry(DefaultRoutes[i]);
      PAssert(entry->IsValid(), PLogicError);
      routes.Append(entry);

      cfg.SetDefaultSection(psprintf(ROUTES_SECTION"\\"ROUTES_KEY" %u", ++arraySize));
      cfg.SetString(RouteAPartyKey, entry->GetPartyA());
      cfg.SetString(RouteBPartyKey, entry->GetPartyB());
      cfg.SetString(RouteDestKey,   entry->GetDestination());
    }
    cfg.SetInteger(ROUTES_SECTION, ROUTES_KEY" Array Size", arraySize);
  }

  PHTTPCompositeField * routeFields = new PHTTPCompositeField(ROUTES_SECTION"\\"ROUTES_KEY" %u\\", ROUTES_SECTION,
                                                              "Internal routing of calls to varous sub-systems");
  routeFields->Append(new PHTTPStringField(RouteAPartyKey, 15));
  routeFields->Append(new PHTTPStringField(RouteBPartyKey, 15));
  routeFields->Append(new PHTTPStringField(RouteDestKey, 15));
  rsrc->Add(new PHTTPFieldArray(routeFields, true));

  SetRouteTable(routes);


  return true;
}

bool MyManager::AllowMediaBypass(const OpalConnection &, const OpalConnection &, const OpalMediaType &) const
{
  return m_allowMediaBypass;
}


///////////////////////////////////////////////////////////////

BaseStatusPage::BaseStatusPage(MyManager & mgr, PHTTPAuthority & auth, const char * name)
  : PServiceHTTPString(name, PString::Empty(), "text/html; charset=UTF-8", auth)
  , m_manager(mgr)
{
}


PString BaseStatusPage::LoadText(PHTTPRequest & request)
{
  if (string.IsEmpty()) {
    PHTML html;
    html << PHTML::Title(GetTitle())
         << "<meta http-equiv=\"Refresh\" content=\"30\">\n"
         << PHTML::Body()
         << MyProcessAncestor::Current().GetPageGraphic()
         << PHTML::Paragraph() << "<center>"

         << PHTML::Form("POST");

    CreateContent(html);

    html << PHTML::Form()
         << PHTML::HRule()

         << MyProcessAncestor::Current().GetCopyrightText()
         << PHTML::Body();
    string = html;
  }

  return PServiceHTTPString::LoadText(request);
}


PBoolean BaseStatusPage::Post(PHTTPRequest & request,
                              const PStringToString & data,
                              PHTML & msg)
{
  PTRACE(2, "Main\tClear call POST received " << data);

  msg << PHTML::Title() << "Accepted Control Command" << PHTML::Body()
      << PHTML::Heading(1) << "Accepted Control Command" << PHTML::Heading(1);

  if (!OnPostControl(data, msg))
    msg << PHTML::Heading(2) << "No calls or endpoints!" << PHTML::Heading(2);

  msg << PHTML::Paragraph()
      << PHTML::HotLink(request.url.AsString()) << "Reload page" << PHTML::HotLink()
      << "&nbsp;&nbsp;&nbsp;&nbsp;"
      << PHTML::HotLink("/") << "Home page" << PHTML::HotLink();

  PServiceHTML::ProcessMacros(request, msg, "html/status.html",
                              PServiceHTML::LoadFromFile|PServiceHTML::NoSignatureForFile);
  return TRUE;
}


///////////////////////////////////////////////////////////////

CallStatusPage::CallStatusPage(MyManager & mgr, PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "CallStatus")
{
}


const char * CallStatusPage::GetTitle() const
{
  return "OPAL Gateway Call Status";
}


void CallStatusPage::CreateContent(PHTML & html) const
{
  html << PHTML::TableStart("border=1")
       << PHTML::TableRow()
       << PHTML::TableHeader()
       << "&nbsp;A&nbsp;Party&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;B&nbsp;Party&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Duration&nbsp;"
       << "<!--#macrostart CallStatus-->"
         << PHTML::TableRow()
         << PHTML::TableData("NOWRAP")
         << PHTML::TableData("NOWRAP")
         << "<!--#status A-Party-->"
         << PHTML::BreakLine()
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


PString MyManager::OnLoadCallStatus(const PString & htmlBlock)
{
  PString substitution;

  PArray<PString> calls = GetAllCalls();
  for (PINDEX i = 0; i < calls.GetSize(); ++i) {
    PSafePtr<OpalCall> call = FindCallWithLock(calls[i], PSafeReadOnly);
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
      duration << (call->GetEstablishedTime() - PTime());
    else
      duration << '(' << (call->GetStartTime() - PTime()) << ')';
    PServiceHTML::SpliceMacro(insert, "status Duration", duration);

    // Then put it into the page, moving insertion point along after it.
    substitution += insert;
  }

  return substitution;
}


PCREATE_SERVICE_MACRO_BLOCK(CallStatus,resource,P_EMPTY,block)
{
  GkStatusPage * status = dynamic_cast<GkStatusPage *>(resource.m_resource);
  return PAssertNULL(status)->m_manager.OnLoadCallStatus(block);
}


// End of File ///////////////////////////////////////////////////////////////
