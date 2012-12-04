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

static const char ParametersSection[] = "Parameters";

const WORD DefaultHTTPPort = 1719;
static const char UsernameKey[] = "Username";
static const char PasswordKey[] = "Password";
static const char LogLevelKey[] = "Log Level";
static const char DefaultAddressFamilyKey[] = "AddressFamily";
#if OPAL_PTLIB_SSL
static const char HTTPCertificateFileKey[]  = "HTTP Certificate";
#endif
static const char HttpPortKey[] = "HTTP Port";
static const char MediaTransferModeKey[] = "Media Transfer Mode";
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
#if P_NAT
static const char NATMethodKey[] = "NAT Method";
static const char NATServerKey[] = "NAT Server";
static const char STUNServerKey[] = "STUN Server";
#endif

#if OPAL_SIP
static const char SIPUsernameKey[] = "SIP User Name";
static const char SIPPrackKey[] = "SIP Provisional Responses";
static const char SIPProxyKey[] = "SIP Proxy URL";
static const char SIPListenersKey[] = "SIP Interfaces";
#if OPAL_PTLIB_SSL
static const char SIPSignalingSecurityKey[] = "SIP Security";
static const char SIPMediaCryptoSuitesKey[] = "SIP Crypto Suites";
#endif

#define REGISTRATIONS_SECTION "SIP Registrations"
#define REGISTRATIONS_KEY "Registration"

static const char SIPAddressofRecordKey[] = "Address of Record";
static const char SIPAuthIDKey[] = "Auth ID";
static const char SIPPasswordKey[] = "Password";
#endif // OPAL_SIP

#if OPAL_LID
static const char LIDKey[] = "Line Interface Devices";
#endif

#if OPAL_CAPI
static const char EnableCAPIKey[] = "CAPI ISDN";
#endif

#if OPAL_PTLIB_VXML
static const char VXMLKey[] = "VXML Script";
#endif

#if OPAL_SCRIPT
static const char ScriptLanguageKey[] = "Language";
static const char ScriptTextKey[] = "Script";
#endif

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
  "h323:.* = sip:<db>@",
  "sip:.* = h323:<du>",
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
  PConfig cfg(ParametersSection);

  // Sert log level as early as possible
  SetLogLevel(cfg.GetEnum(LogLevelKey, GetLogLevel()));
#if PTRACING
  PTrace::SetLevel(GetLogLevel());
  PTrace::ClearOptions(PTrace::Timestamp);
#if _DEBUG
  PTrace::SetOptions(PTrace::FileAndLine|PTrace::ContextIdentifier);
#endif
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
  PConfigPage * rsrc = new PConfigPage(*this, "Parameters", ParametersSection, authority);

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

#if OPAL_H323 | OPAL_SIP
  RegistrationStatusPage * registrationStatusPage = new RegistrationStatusPage(m_manager, authority);
  httpNameSpace.AddResource(registrationStatusPage, PHTTPSpace::Overwrite);
#endif

  CallStatusPage * callStatusPage = new CallStatusPage(m_manager, authority);
  httpNameSpace.AddResource(callStatusPage, PHTTPSpace::Overwrite);

#if OPAL_H323
  GkStatusPage * gkStatusPage = new GkStatusPage(m_manager, authority);
  httpNameSpace.AddResource(gkStatusPage, PHTTPSpace::Overwrite);
#endif // OPAL_H323

  // Log file resource
  PHTTPFile * fullLog = NULL;
  PHTTPTailFile * tailLog = NULL;

  PSystemLogToFile * logFile = dynamic_cast<PSystemLogToFile *>(&PSystemLog::GetTarget());
  if (logFile != NULL) {
    fullLog = new PHTTPFile("FullLog", logFile->GetFilePath(), PMIMEInfo::TextPlain(), authority);
    httpNameSpace.AddResource(fullLog, PHTTPSpace::Overwrite);

    tailLog = new PHTTPTailFile("TailLog", logFile->GetFilePath(), PMIMEInfo::TextPlain(), authority);
    httpNameSpace.AddResource(tailLog, PHTTPSpace::Overwrite);
  }

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

         << PHTML::HotLink(rsrc->GetHotLink()) << "System Parameters" << PHTML::HotLink()
#if OPAL_H323 | OPAL_SIP
         << PHTML::Paragraph()
         << PHTML::HotLink(registrationStatusPage->GetHotLink()) << "Registration Status" << PHTML::HotLink()
#endif
         << PHTML::BreakLine()
         << PHTML::HotLink(callStatusPage->GetHotLink()) << "Call Status" << PHTML::HotLink()
#if OPAL_H323
         << PHTML::BreakLine()
         << PHTML::HotLink(gkStatusPage->GetHotLink()) << "Gatekeeper Status" << PHTML::HotLink()
#endif // OPAL_H323
         << PHTML::Paragraph();

    if (logFile != NULL)
      html << PHTML::HotLink(fullLog->GetHotLink()) << "Full Log File" << PHTML::HotLink()
           << PHTML::BreakLine()
           << PHTML::HotLink(tailLog->GetHotLink()) << "Tail Log File" << PHTML::HotLink()
           << PHTML::Paragraph();
 
    html << PHTML::HRule()
         << GetCopyrightText()
         << PHTML::Body();
    httpNameSpace.AddResource(new PServiceHTTPString(welcomeHtml, html), PHTTPSpace::Overwrite);
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
  : m_mediaTransferMode(MediaTransferForward)
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
#if OPAL_PTLIB_VXML
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
  PINDEX arraySize;
  PHTTPFieldArray * fieldArray;

  PString defaultSection = cfg.GetDefaultSection();

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

#if OPAL_PTLIB_VXML
  if (m_ivrEP == NULL)
    m_ivrEP = new OpalIVREndPoint(*this);
#endif

  // General parameters for all endpoint types
  m_mediaTransferMode = cfg.GetEnum(MediaTransferModeKey, m_mediaTransferMode);
  static const char * const MediaTransferModeValues[] = { "0", "1", "2" };
  static const char * const MediaTransferModeTitles[] = { "Bypass", "Forward", "Transcode" };
  rsrc->Add(new PHTTPRadioField(MediaTransferModeKey,
                  PARRAYSIZE(MediaTransferModeValues), MediaTransferModeValues, MediaTransferModeTitles,
                               m_mediaTransferMode, "How media is to be routed between the endpoints."));

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

#if P_NAT
  {
    PString method = cfg.GetString(NATMethodKey, PSTUNClient::GetNatMethodName());
    PString server = cfg.GetString(NATServerKey, cfg.GetString(STUNServerKey, GetNATServer()));
    SetNATServer(method, server);
    rsrc->Add(new PHTTPStringField(NATMethodKey, 25, method,
              "Mothod for NAT traversal"));
    rsrc->Add(new PHTTPStringField(NATServerKey, 25, server,
              "Server IP/hostname for NAT traversal"));
  }
#endif // P_NAT

#if OPAL_H323
  if (!m_h323EP->Initialise(cfg, rsrc))
    return false;
#endif

#if OPAL_SIP
  // Add SIP parameters
  m_sipEP->SetDefaultLocalPartyName(cfg.GetString(SIPUsernameKey, m_sipEP->GetDefaultLocalPartyName()));
  rsrc->Add(new PHTTPStringField(SIPUsernameKey, 25, m_sipEP->GetDefaultLocalPartyName(),
            "SIP local user name"));

  fieldArray = new PHTTPFieldArray(new PHTTPStringField(SIPListenersKey, 25,
                                   "", "Local network interfaces to listen for SIP, blank means all"), false);
  if (!m_sipEP->StartListeners(fieldArray->GetStrings(cfg))) {
    PSYSTEMLOG(Error, "Could not open any SIP listeners!");
  }
  rsrc->Add(fieldArray);

  SIPConnection::PRACKMode prack = cfg.GetEnum(SIPPrackKey, m_sipEP->GetDefaultPRACKMode());
  static const char * const prackModes[] = { "Disabled", "Supported", "Required" };
  rsrc->Add(new PHTTPRadioField(SIPPrackKey, PARRAYSIZE(prackModes), prackModes, prack,
            "SIP provisional responses (100rel) handling."));
  m_sipEP->SetDefaultPRACKMode(prack);

  PString proxy = m_sipEP->GetProxy().AsString();
  m_sipEP->SetProxy(cfg.GetString(SIPProxyKey, proxy));
  rsrc->Add(new PHTTPStringField(SIPProxyKey, 25, proxy,
            "SIP outbound proxy IP/hostname"));

#if OPAL_PTLIB_SSL
  PINDEX security = 0;
  if (FindEndPoint("sip") != NULL)
    security |= 1;
  if (FindEndPoint("sips") != NULL)
    security |= 2;
  security = cfg.GetInteger(SIPSignalingSecurityKey, security);
  switch (security) {
    case 1 :
      AttachEndPoint(m_sipEP, "sip");
      DetachEndPoint("sips");
      break;

    case 2 :
      AttachEndPoint(m_sipEP, "sips");
      DetachEndPoint("sip");
      break;

    default :
      AttachEndPoint(m_sipEP, "sip");
      AttachEndPoint(m_sipEP, "sips");
  }
  static const char * const SignalingSecurityValues[] = { "1", "2", "3" };
  static const char * const SignalingSecurityTitles[] = { "sip only", "sips only", "sip & sips" };
  rsrc->Add(new PHTTPRadioField(SIPSignalingSecurityKey,
                     PARRAYSIZE(SignalingSecurityValues), SignalingSecurityValues, SignalingSecurityTitles,
                                                        security, "Signaling security methods available."));

  fieldArray = new PHTTPFieldArray(new PHTTPSelectField(SIPMediaCryptoSuitesKey,
              m_sipEP->GetAllMediaCryptoSuites(), 0, "Media security methods available."), true);
  PStringArray cryptoSuites = fieldArray->GetStrings(cfg);
  if (cryptoSuites.GetSize() > 0)
    m_sipEP->SetMediaCryptoSuites(cryptoSuites);
  else
    fieldArray->SetStrings(cfg, m_sipEP->GetMediaCryptoSuites());
  rsrc->Add(fieldArray);
#endif

  // Registrar
  list<SIPRegister::Params> registrations;
  arraySize = cfg.GetInteger(REGISTRATIONS_SECTION, REGISTRATIONS_KEY" Array Size");
  for (PINDEX i = 0; i < arraySize; i++) {
    SIPRegister::Params registrar;
    cfg.SetDefaultSection(psprintf(REGISTRATIONS_SECTION"\\"REGISTRATIONS_KEY" %u", i+1));
    registrar.m_addressOfRecord = cfg.GetString(SIPAddressofRecordKey);
    registrar.m_authID = cfg.GetString(SIPAuthIDKey);
    registrar.m_password = cfg.GetString(SIPPasswordKey);
    if (!registrar.m_addressOfRecord.IsEmpty())
      registrations.push_back(registrar);
  }
  cfg.SetDefaultSection(defaultSection);

  PHTTPCompositeField * registrationsFields = new PHTTPCompositeField(
        REGISTRATIONS_SECTION"\\"REGISTRATIONS_KEY" %u\\", REGISTRATIONS_SECTION,
        "Registration of SIP username at domain/hostname/IP address");
  registrationsFields->Append(new PHTTPStringField(SIPAddressofRecordKey, 20));
  registrationsFields->Append(new PHTTPStringField(SIPAuthIDKey, 15));
  registrationsFields->Append(new PHTTPStringField(SIPPasswordKey, 15));
  rsrc->Add(new PHTTPFieldArray(registrationsFields, false));

  for (list<SIPRegister::Params>::iterator it = registrations.begin(); it != registrations.end(); ++it) {
    PString aor;
    if (m_sipEP->Register(*it, aor))
      PSYSTEMLOG(Info, "Started register of " << aor);
    else
      PSYSTEMLOG(Error, "Could not register " << *it);
  }

#endif

#if OPAL_LID
  // Add POTS and PSTN endpoints
  fieldArray = new PHTTPFieldArray(new PHTTPSelectField(LIDKey, OpalLineInterfaceDevice::GetAllDevices(),
                                   0, "Line Interface Devices to monitor, if any"), false);
  PStringArray devices = fieldArray->GetStrings(cfg);
  if (!m_potsEP->AddDeviceNames(devices)) {
    PSYSTEMLOG(Error, "No LID devices!");
  }
  rsrc->Add(fieldArray);
#endif // OPAL_LID


#if OPAL_CAPI
  m_enableCAPI = cfg.GetBoolean(EnableCAPIKey);
  rsrc->Add(new PHTTPBooleanField(EnableCAPIKey, m_enableCAPI, "Enable CAPI ISDN controller(s), if available"));
  if (m_enableCAPI && m_capiEP->OpenControllers() == 0) {
    PSYSTEMLOG(Error, "No CAPI controllers!");
  }
#endif


#if OPAL_PTLIB_VXML
  // Set IVR protocol handler
  PString vxml = cfg.GetString(VXMLKey);
  rsrc->Add(new PHTTPStringField(VXMLKey, 800, vxml,
            "Interactive Voice Response VXML script, may be a URL or the actual VXML"));
  if (!vxml)
    m_ivrEP->SetDefaultVXML(vxml);
#endif

#if OPAL_SCRIPT
  PFactory<PScriptLanguage>::KeyList_T keys = PFactory<PScriptLanguage>::GetKeyList();
  PStringArray languages;
  for (PFactory<PScriptLanguage>::KeyList_T::iterator it = keys.begin(); it != keys.end(); ++it)
    languages.AppendString(*it);
  PString language = cfg.GetString(ScriptLanguageKey, languages[0]);
  rsrc->Add(new PHTTPRadioField(ScriptLanguageKey, languages,
            languages.GetValuesIndex(language),"Interpreter script language."));

  PString script = cfg.GetString(ScriptTextKey);
  rsrc->Add(new PHTTPStringField(ScriptTextKey, 800, script,
            "Interpreter script, may be a filename or the actual script text"));
  if (m_scriptLanguage != language || m_scriptText != script) {
    m_scriptLanguage = language;
    m_scriptText = script;
    RunScript(script, language);
  }
#endif //OPAL_SCRIPT

  // Routing
  RouteTable routes;
  arraySize = cfg.GetInteger(ROUTES_SECTION, ROUTES_KEY" Array Size");
  for (PINDEX i = 0; i < arraySize; i++) {
    cfg.SetDefaultSection(psprintf(ROUTES_SECTION"\\"ROUTES_KEY" %u", i+1));
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
  cfg.SetDefaultSection(defaultSection);

  PHTTPCompositeField * routeFields = new PHTTPCompositeField(
        ROUTES_SECTION"\\"ROUTES_KEY" %u\\", ROUTES_SECTION,
        "Internal routing of calls to varous sub-systems");
  routeFields->Append(new PHTTPStringField(RouteAPartyKey, 15));
  routeFields->Append(new PHTTPStringField(RouteBPartyKey, 15));
  routeFields->Append(new PHTTPStringField(RouteDestKey, 25));
  rsrc->Add(new PHTTPFieldArray(routeFields, true));

  SetRouteTable(routes);


  return true;
}


MyManager::MediaTransferMode MyManager::GetMediaTransferMode(const OpalConnection &, const OpalConnection &, const OpalMediaType &) const
{
  return m_mediaTransferMode;
}


void MyManager::AdjustMediaFormats(bool local,
                                   const OpalConnection & connection,
                                   OpalMediaFormatList & mediaFormats) const
{
  // Don't do the reorder done in OpalManager::AdjustMediaFormats if a gateway
  if (local && connection.IsNetworkConnection()) {
    PSafePtr<OpalConnection> otherConnection = connection.GetOtherPartyConnection();
    if (otherConnection != NULL && otherConnection->IsNetworkConnection())
      return;
  }

  OpalManager::AdjustMediaFormats(local, connection, mediaFormats);
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

#if OPAL_H323 | OPAL_SIP

RegistrationStatusPage::RegistrationStatusPage(MyManager & mgr, PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "RegistrationStatus")
{
}


const char * RegistrationStatusPage::GetTitle() const
{
  return "OPAL Gateway Registration Status";
}


void RegistrationStatusPage::CreateContent(PHTML & html) const
{
#if OPAL_H323
  html << PHTML::TableStart("border=1 cellpadding=5")
       << PHTML::TableRow()
       << PHTML::TableData("NOWRAP")
       << "&nbsp;H.323&nbsp;Gatekeeper&nbsp;"
       << PHTML::TableData("NOWRAP")
       << "<!--#macro H323RegistrationStatus-->"
       << PHTML::TableEnd();
#endif
#if OPAL_H323 | OPAL_SIP
  html << PHTML::Paragraph();
#endif
#if OPAL_SIP
  html << PHTML::TableStart("border=1 cellpadding=5")
       << PHTML::TableRow()
       << PHTML::TableHeader()
       << "&nbsp;SIP&nbsp;Address&nbsp;of&nbsp;Record&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Status&nbsp;"
       << "<!--#macrostart SIPRegistrationStatus-->"
         << PHTML::TableRow()
         << PHTML::TableData("NOWRAP")
         << "<!--#status AoR-->"
         << PHTML::TableData("NOWRAP")
         << "<!--#status State-->"
       << "<!--#macroend SIPRegistrationStatus-->"
       << PHTML::TableEnd();
#endif
}


bool RegistrationStatusPage::OnPostControl(const PStringToString & data, PHTML & msg)
{
  return false;
}

#if OPAL_H323
PCREATE_SERVICE_MACRO(H323RegistrationStatus,resource,P_EMPTY)
{
  RegistrationStatusPage * status = dynamic_cast<RegistrationStatusPage *>(resource.m_resource);
  if (PAssertNULL(status) == NULL)
    return PString::Empty();

  H323Gatekeeper * gk = status->m_manager.GetH323EndPoint().GetGatekeeper();
  if (gk == NULL)
    return "None";

  if (gk->IsRegistered())
    return "Registered";
  
  PStringStream strm;
  strm << "Failed: " << gk->GetRegistrationFailReason();
  return strm;
}
#endif

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
    PServiceHTML::SpliceMacro(insert, "status State",
          sip.IsRegistered(*it) ? "Registered" : (sip.IsRegistered(*it, true) ? "Offline" : "Failed"));

    // Then put it into the page, moving insertion point along after it.
    substitution += insert;
  }

  return substitution;
}
#endif


#endif

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
  html << PHTML::TableStart("border=1 cellpadding=3")
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
         << "<!--#status A-Party-->"
         << PHTML::TableData("NOWRAP")
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
      duration << (call->GetEstablishedTime() - PTime());
    else
      duration << '(' << (call->GetStartTime() - PTime()) << ')';
    PServiceHTML::SpliceMacro(insert, "status Duration", duration);

    // Then put it into the page, moving insertion point along after it.
    substitution += insert;
  }

  return substitution;
}


// End of File ///////////////////////////////////////////////////////////////
