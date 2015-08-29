/*
 * main.cxx
 *
 * OPAL Server main program
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 *                 BCS Global, Inc.
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
const WORD DefaultTelnetPort = 1718;
static const PConstString TelnetPortKey("Console Port");
static const PConstString DisplayNameKey("Display Name");
static const PConstString MediaTransferModeKey("Media Transfer Mode");
static const PConstString AutoStartKeyPrefix("Auto Start ");
static const PConstString PreferredMediaKey("Preferred Media");
static const PConstString RemovedMediaKey("Removed Media");
static const PConstString MinJitterKey("Minimum Jitter");
static const PConstString MaxJitterKey("Maximum Jitter");
static const PConstString NoMediaTimeoutKey("No Media Timeout");
static const PConstString TCPPortBaseKey("TCP Port Base");
static const PConstString TCPPortMaxKey("TCP Port Max");
static const PConstString UDPPortBaseKey("UDP Port Base");
static const PConstString UDPPortMaxKey("UDP Port Max");
static const PConstString RTPPortBaseKey("RTP Port Base");
static const PConstString RTPPortMaxKey("RTP Port Max");
static const PConstString RTPTOSKey("RTP Type of Service");
#if P_NAT
static const PConstString NATActiveKey("Active");
static const PConstString NATServerKey("Server");
#endif

static const PConstString OverrideProductInfoKey("Override Product Info");
static const PConstString VendorNameKey("Product Vendor");
static const PConstString ProductNameKey("Product Name");
static const PConstString ProductVersionKey("Product Version");

#if OPAL_SKINNY
static const PConstString SkinnyServerKey("SCCP Server");
static const PConstString SkinnyInterfaceKey("SCCP Interface");
static const PConstString SkinnyTypeKey("SCCP Device Type");
static const PConstString SkinnyNamesKey("SCCP Device Names");
static const PConstString SkinnySimulatedAudioFileKey("SCCP Simulated Audio File");
#endif

#if OPAL_LID
static const PConstString LIDKey("Line Interface Devices");
#endif

#if OPAL_CAPI
static const PConstString EnableCAPIKey("CAPI ISDN");
#endif

#if OPAL_IVR
static const PConstString VXMLKey("VXML Script");
static const PConstString IVRCacheKey("TTS Cache Directory");
static const PConstString IVRRecordDirKey("Record Message Directory");
#endif

#if OPAL_HAS_MIXER
static const PConstString ConfAudioOnlyKey("Conference Audio Only");
static const PConstString ConfMediaPassThruKey("Conference Media Pass Through");
static const PConstString ConfVideoResolutionKey("Conference Video Resolution");
#endif

#if OPAL_SCRIPT
static const PConstString ScriptLanguageKey("Language");
static const PConstString ScriptTextKey("Script");
#endif

#define ROUTES_SECTION "Routes"
#define ROUTES_KEY     ROUTES_SECTION"\\Route %u\\"

static const PConstString RouteAPartyKey("A Party");
static const PConstString RouteBPartyKey("B Party");
static const PConstString RouteDestKey("Destination");

#define CONFERENCE_NAME "conference"

#define ROUTE_USERNAME(name, to) ".*:.*\t" name "|.*:" name "@.* = " to

static const char * const DefaultRoutes[] = {
  ROUTE_USERNAME("loopback", "loopback:"),
#if OPAL_IVR
  ROUTE_USERNAME("#", "ivr:"),
#endif
#if OPAL_HAS_MIXER
  ROUTE_USERNAME(CONFERENCE_NAME, OPAL_PREFIX_MIXER":<du>"),
#endif
#if OPAL_LID
  "pots:\\+*[0-9]+ = tel:<dn>",
  "pstn:\\+*[0-9]+ = tel:<dn>",
#endif
#if OPAL_CAPI
  "capi:\\+*[0-9]+ = tel:<dn>",
#endif
#if OPAL_H323
  "h323:\\+*[0-9]+ = tel:<dn>",
 #if OPAL_SIP
  "h323:.+@.+ = sip:<da>",
  "h323:.* = sip:<db>@",
  "sip:.* = h323:<du>",
 #else
  "tel:[0-9]+\\*[0-9]+\\*[0-9]+\\*[0-9]+ = h323:<dn2ip>",
  "tel:.*=h323:<dn>"
 #endif
#endif
#if OPAL_SIP
  "sip:\\+*[0-9]+@.* = tel:<dn>",
  "tel:[0-9]+\\*[0-9]+\\*[0-9]+\\*[0-9]+ = sip:<dn2ip>",
  "tel:.*=sip:<dn>"
#endif
};

static PConstString LoopbackPrefix("loopback");


#if P_STUN
struct NATInfo {
  PString m_method;
  PString m_friendly;
  bool    m_active;
  PString m_server;
  NATInfo(const PNatMethod & method)
    : m_method(method.GetMethodName())
    , m_friendly(method.GetFriendlyName())
    , m_active(method.IsActive())
    , m_server(method.GetServer())
  { }

  __inline bool operator<(const NATInfo & other) const { return m_method < other.m_method; }
};
#endif // P_STUN


///////////////////////////////////////////////////////////////////////////////

MyProcess::MyProcess()
  : MyProcessAncestor(ProductInfo)
  , m_manager(NULL)
{
}


MyProcess::~MyProcess()
{
  delete m_manager;
}


PBoolean MyProcess::OnStart()
{
  // Set log level as early as possible
  Params params(NULL);
  params.m_forceRotate = true;
  InitialiseBase(params);

  if (m_manager == NULL)
    m_manager = new MyManager();
  return m_manager->PreInitialise(GetArguments(), false) &&
         PHTTPServiceProcess::OnStart() &&
         m_manager->Initialise(GetArguments(), false);
}


void MyProcess::OnStop()
{
  if (m_manager != NULL)
    m_manager->ShutDownEndpoints();
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
  PSYSTEMLOG(Warning, "Service " << GetName() << ' ' << initMsg);

  Params params("Parameters");
  params.m_httpPort = DefaultHTTPPort;
  if (!InitialiseBase(params))
    return false;

  // Configure the core of the system
  PConfig cfg(params.m_section);
  if (!m_manager->Configure(cfg, params.m_configPage))
    return false;

  // Finished the resource to add, generate HTML for it and add to name space
  PServiceHTML html("System Parameters");
  params.m_configPage->BuildHTML(html);

  m_httpNameSpace.AddResource(new OpalHTTPConnector(*m_manager, "/websocket", params.m_authority), PHTTPSpace::Overwrite);
#if OPAL_SDP
  m_httpNameSpace.AddResource(new OpalSDPHTTPResource(*m_manager->FindEndPointAs<OpalSDPHTTPEndPoint>(OPAL_PREFIX_SDP), "/sdp", params.m_authority), PHTTPSpace::Overwrite);
#endif


  RegistrationStatusPage * registrationStatusPage = new RegistrationStatusPage(*m_manager, params.m_authority);
  m_httpNameSpace.AddResource(registrationStatusPage, PHTTPSpace::Overwrite);

  CallStatusPage * callStatusPage = new CallStatusPage(*m_manager, params.m_authority);
  m_httpNameSpace.AddResource(callStatusPage, PHTTPSpace::Overwrite);

#if OPAL_H323
  GkStatusPage * gkStatusPage = new GkStatusPage(*m_manager, params.m_authority);
  m_httpNameSpace.AddResource(gkStatusPage, PHTTPSpace::Overwrite);
#endif // OPAL_H323

  CDRListPage * cdrListPage = new CDRListPage(*m_manager, params.m_authority);
  m_httpNameSpace.AddResource(cdrListPage, PHTTPSpace::Overwrite);
  m_httpNameSpace.AddResource(new CDRPage(*m_manager, params.m_authority), PHTTPSpace::Overwrite);

  // Create the home page
  static const char welcomeHtml[] = "welcome.html";
  if (PFile::Exists(welcomeHtml))
    m_httpNameSpace.AddResource(new PServiceHTTPFile(welcomeHtml, true), PHTTPSpace::Overwrite);
  else {
    PHTML html;
    html << PHTML::Title("Welcome to " + GetName())
         << PHTML::Body()
         << GetPageGraphic()
         << PHTML::Paragraph() << "<center>"

         << PHTML::HotLink(params.m_configPage->GetHotLink()) << "System Parameters" << PHTML::HotLink()
         << PHTML::Paragraph()
         << PHTML::HotLink(callStatusPage->GetHotLink()) << "Call Status" << PHTML::HotLink()
#if OPAL_H323
         << PHTML::BreakLine()
         << PHTML::HotLink(gkStatusPage->GetHotLink()) << "Gatekeeper Status" << PHTML::HotLink()
#endif // OPAL_H323
#if OPAL_H323 | OPAL_SIP | OPAL_SKINNY
         << PHTML::BreakLine()
         << PHTML::HotLink(registrationStatusPage->GetHotLink()) << "Registration Status" << PHTML::HotLink()
#endif
         << PHTML::Paragraph()
         << PHTML::HotLink(cdrListPage->GetHotLink()) << "Call Detail Records" << PHTML::HotLink()
         << PHTML::Paragraph();

    if (params.m_fullLogPage != NULL)
      html << PHTML::HotLink(params.m_fullLogPage->GetHotLink()) << "Full Log File" << PHTML::HotLink() << PHTML::BreakLine();
    if (params.m_clearLogPage != NULL)
      html << PHTML::HotLink(params.m_clearLogPage->GetHotLink()) << "Clear Log File" << PHTML::HotLink() << PHTML::BreakLine();
    if (params.m_tailLogPage != NULL)
      html << PHTML::HotLink(params.m_tailLogPage->GetHotLink()) << "Tail Log File" << PHTML::HotLink() << PHTML::BreakLine();

    html << PHTML::HRule()
         << GetCopyrightText()
         << PHTML::Body();
    m_httpNameSpace.AddResource(new PServiceHTTPString(welcomeHtml, html), PHTTPSpace::Overwrite);
  }

  // set up the HTTP port for listening & start the first HTTP thread
  if (ListenForHTTP(params.m_httpPort))
    PSYSTEMLOG(Info, "Opened master socket(s) for HTTP: " << m_httpListeningSockets.front().GetPort());
  else {
    PSYSTEMLOG(Fatal, "Cannot run without HTTP");
    return false;
  }

  PSYSTEMLOG(Info, "Completed configuring service " << GetName() << " (" << initMsg << ')');
  return true;
}


bool MyManager::ConfigureCommon(OpalEndPoint * ep,
                                const PString & cfgPrefix,
                                PConfig & cfg,
                                PConfigPage * rsrc)
{
  PString normalPrefix = ep->GetPrefixName();

  if (!ep->StartListeners(rsrc->AddStringArrayField(cfgPrefix + " Interfaces", false, 25, PStringArray(),
                          "Local network interfaces and ports to listen on, blank means all"))) {
    PSYSTEMLOG(Error, "Could not open any listeners for " << normalPrefix);
  }

#if OPAL_PTLIB_SSL
  PString securePrefix = normalPrefix + 's';

  PINDEX security = 0;
  if (FindEndPoint(normalPrefix) != NULL)
    security |= 1;
  if (FindEndPoint(securePrefix) != NULL)
    security |= 2;

  PString signalingKey = cfgPrefix + " Security";
  security = cfg.GetInteger(signalingKey, security);

  switch (security) {
    case 1:
      AttachEndPoint(ep, normalPrefix);
      DetachEndPoint(securePrefix);
      break;

    case 2:
      AttachEndPoint(ep, securePrefix);
      DetachEndPoint(normalPrefix);
      break;

    default:
      AttachEndPoint(ep, normalPrefix);
      AttachEndPoint(ep, securePrefix);
  }

  PStringArray valueArray, titleArray;
  valueArray[0] = '1'; titleArray[0] = normalPrefix & "only";
  valueArray[1] = '2'; titleArray[1] = securePrefix & "only";
  valueArray[2] = '3'; titleArray[2] = normalPrefix & '&' & securePrefix;
  rsrc->Add(new PHTTPRadioField(signalingKey, valueArray, titleArray, security - 1, "Signaling security methods available."));

  ep->SetMediaCryptoSuites(rsrc->AddSelectArrayField(cfgPrefix + " Crypto Suites", true, ep->GetMediaCryptoSuites(),
                                            ep->GetAllMediaCryptoSuites(), "Media security methods available."));
#endif // OPAL_PTLIB_SSL

  PString section = cfgPrefix + " Default Options";
  PHTTPCompositeField * optionsFields = new PHTTPCompositeField(section + "\\Option %u\\", section,
                                                                "Default options for calls using " + cfgPrefix);
  optionsFields->Append(new PHTTPStringField("Name", 0, NULL, NULL, 1, 30));
  optionsFields->Append(new PHTTPStringField("Value", 0, NULL, NULL, 1, 30));
  PHTTPFieldArray * optionsArray = new PHTTPFieldArray(optionsFields, false);
  rsrc->Add(optionsArray);

  if (!optionsArray->LoadFromConfig(cfg)) {
    for (PINDEX i = 0; i < optionsArray->GetSize(); ++i) {
      PHTTPCompositeField & item = dynamic_cast<PHTTPCompositeField &>((*optionsArray)[i]);
      ep->SetDefaultStringOption(item[0].GetValue(), item[1].GetValue());
    }
  }

  return true;
}


///////////////////////////////////////////////////////////////

MyManager::MyManager()
  : MyManagerParent(OPAL_CONSOLE_PREFIXES OPAL_PREFIX_PCSS " " OPAL_PREFIX_IVR " " OPAL_PREFIX_MIXER)
  , m_systemLog(PSystemLog::Info)
  , m_mediaTransferMode(MediaTransferForward)
  , m_savedProductInfo(GetProductInfo())
#if OPAL_CAPI
  , m_enableCAPI(true)
#endif
  , m_cdrListMax(100)
{
  OpalLocalEndPoint * ep = new OpalLocalEndPoint(*this, LoopbackPrefix);
  ep->SetDefaultAudioSynchronicity(OpalLocalEndPoint::e_SimulateSynchronous);
  OpalMediaFormat::RegisterKnownMediaFormats(); // Make sure codecs are loaded
  m_outputStream = &m_systemLog;
}


MyManager::~MyManager()
{
}


void MyManager::EndRun(bool)
{
  PServiceProcess::Current().OnStop();
}


#if OPAL_H323
H323ConsoleEndPoint * MyManager::CreateH323EndPoint()
{
  return new MyH323EndPoint(*this);
}
#endif


#if OPAL_SIP
SIPConsoleEndPoint * MyManager::CreateSIPEndPoint()
{
  return new MySIPEndPoint(*this);
}
#endif


PBoolean MyManager::Configure(PConfig & cfg, PConfigPage * rsrc)
{
  // Make sure all endpoints created
  for (PINDEX i = 0; i < m_endpointPrefixes.GetSize(); ++i)
    GetConsoleEndPoint(m_endpointPrefixes[i]);

  PString defaultSection = cfg.GetDefaultSection();

#if P_CLI && P_TELNET
  // Telnet Port number to use.
  {
    WORD telnetPort = (WORD)rsrc->AddIntegerField(TelnetPortKey, 0, 65535, DefaultTelnetPort,
                                                  "", "Port for console user interface (telnet) of server (zero disables).");
    PCLISocket * cliSocket = dynamic_cast<PCLISocket *>(m_cli);
    if (cliSocket != NULL && telnetPort != 0)
      cliSocket->Listen(telnetPort);
    else {
      delete m_cli;
      if (telnetPort == 0)
        m_cli = NULL;
      else {
        m_cli = new PCLITelnet(telnetPort);
        m_cli->Start();
      }
    }
  }
#endif //P_CLI && P_TELNET

  // General parameters for all endpoint types
  SetDefaultDisplayName(rsrc->AddStringField(DisplayNameKey, 25, GetDefaultDisplayName(), "Display name used in various protocols"));

  m_mediaTransferMode = cfg.GetEnum(MediaTransferModeKey, m_mediaTransferMode);
  static const char * const MediaTransferModeValues[] = { "0", "1", "2" };
  static const char * const MediaTransferModeTitles[] = { "Bypass", "Forward", "Transcode" };
  rsrc->Add(new PHTTPRadioField(MediaTransferModeKey,
    PARRAYSIZE(MediaTransferModeValues), MediaTransferModeValues, MediaTransferModeTitles,
    m_mediaTransferMode, "How media is to be routed between the endpoints."));

  {
    OpalMediaTypeList mediaTypes = OpalMediaType::GetList();
    for (OpalMediaTypeList::iterator it = mediaTypes.begin(); it != mediaTypes.end(); ++it) {
      PString key = AutoStartKeyPrefix;
      key &= it->c_str();

      (*it)->SetAutoStart(cfg.GetEnum<OpalMediaType::AutoStartMode::Enumeration>(key, (*it)->GetAutoStart()));

      static const char * const AutoStartValues[] = { "Offer inactive", "Receive only", "Send only", "Send & Receive", "Don't offer" };
      rsrc->Add(new PHTTPEnumField<OpalMediaType::AutoStartMode::Enumeration>(key,
                        PARRAYSIZE(AutoStartValues), AutoStartValues, (*it)->GetAutoStart(),
                        "Initial start up mode for media type."));
    }
  }

  {
    OpalMediaFormatList allFormats;
    PList<OpalEndPoint> endpoints = GetEndPoints();
    for (PList<OpalEndPoint>::iterator it = endpoints.begin(); it != endpoints.end(); ++it)
      allFormats += it->GetMediaFormats();
    OpalMediaFormatList transportableFormats;
    for (OpalMediaFormatList::iterator it = allFormats.begin(); it != allFormats.end(); ++it) {
      if (it->IsTransportable())
        transportableFormats += *it;
    }
    PStringStream help;
    help << "Preference order for codecs to be offered to remotes.<p>"
      "Note, these are not regular expressions, just simple "
      "wildcards where '*' matches any number of characters.<p>"
      "Known media formats are:<br>";
    for (OpalMediaFormatList::iterator it = transportableFormats.begin(); it != transportableFormats.end(); ++it) {
      if (it != transportableFormats.begin())
        help << ", ";
      help << *it;
    }
    SetMediaFormatOrder(rsrc->AddStringArrayField(PreferredMediaKey, true, 25, GetMediaFormatOrder(), help));
  }

  SetMediaFormatMask(rsrc->AddStringArrayField(RemovedMediaKey, true, 25, GetMediaFormatMask(),
    "Codecs to be prevented from being used.<p>"
    "These are wildcards as in the above Preferred Media, with "
    "the addition of preceding the expression with a '!' which "
    "removes all formats <i>except</i> the indicated wildcard. "
    "Also, the '@' character may also be used to indicate a "
    "media type, e.g. <code>@video</code> removes all video "
    "codecs."));

  {
    PString section = "Media Options";
    PHTTPCompositeField * optionsFields = new PHTTPCompositeField(section + "\\Option %u\\", section,
        "Default options for media formats.<p>"
        "The \"Format\" field may contain wildcard '*' character, e.g. \"G.711*\""
        " or begin with an '@' to indicate a media type, e.g. \"@audio\". The"
        " media option will be set on all media formats that match.");
    optionsFields->Append(new PHTTPBooleanField("Set", true));
    optionsFields->Append(new PHTTPStringField("Format", 0, NULL, NULL, 1, 15));
    optionsFields->Append(new PHTTPStringField("Option", 0, NULL, NULL, 1, 20));
    optionsFields->Append(new PHTTPStringField("Value", 0, NULL, NULL, 1, 10));
    PHTTPFieldArray * optionsArray = new PHTTPFieldArray(optionsFields, false);
    rsrc->Add(optionsArray);

    if (!optionsArray->LoadFromConfig(cfg)) {
      for (PINDEX i = 0; i < optionsArray->GetSize(); ++i) {
        PHTTPCompositeField & item = dynamic_cast<PHTTPCompositeField &>((*optionsArray)[i]);
        if (item[0].GetValue() *= "true") {
          PString wildcard = item[1].GetValue();
          PString option = item[2].GetValue();
          PString value = item[3].GetValue();

          OpalMediaFormatList all;
          OpalMediaFormat::GetAllRegisteredMediaFormats(all);

          bool notSet = true;
          OpalMediaFormatList::const_iterator it;
          while ((it = all.FindFormat(wildcard, it)) != all.end()) {
            if (const_cast<OpalMediaFormat &>(*it).SetOptionValue(option, value)) {
              PSYSTEMLOG(Info, "Set media format \"" << *it << "\", option \"" << option << "\", to \"" << value << '"');
              OpalMediaFormat::SetRegisteredMediaFormat(*it);
              notSet = false;
            }
            else
              PSYSTEMLOG(Warning, "Could not set media format \"" << *it << "\", option \"" << option << "\", to \"" << value << '"');
          }
          if (notSet) {
            PSYSTEMLOG(Warning, "Could not set any media formats using wildcare \"" << wildcard << '"');
            item[0].SetValue("false");
            optionsArray->SaveToConfig(cfg);
          }
        }
      }
    }
  }

  SetAudioJitterDelay(rsrc->AddIntegerField(MinJitterKey, 20, 2000, GetMinAudioJitterDelay(), "ms", "Minimum jitter buffer size"),
                      rsrc->AddIntegerField(MaxJitterKey, 20, 2000, GetMaxAudioJitterDelay(), "ms", "Maximum jitter buffer size"));

  SetNoMediaTimeout(PTimeInterval(0, rsrc->AddIntegerField(NoMediaTimeoutKey, 1, UINT_MAX, GetNoMediaTimeout().GetSeconds(),
                                                           "seconds", "Clear call when no media is received from remote for this time")));

  SetTCPPorts(rsrc->AddIntegerField(TCPPortBaseKey, 0, 65535, GetTCPPortBase(), "", "Base of port range for allocating TCP streams, e.g. H.323 signalling channel"),
              rsrc->AddIntegerField(TCPPortMaxKey, 0, 65535, GetTCPPortMax(), "", "Maximum of port range for allocating TCP streams"));
  SetUDPPorts(rsrc->AddIntegerField(UDPPortBaseKey, 0, 65535, GetUDPPortBase(), "", "Base of port range for allocating UDP streams, e.g. SIP signalling channel"),
              rsrc->AddIntegerField(UDPPortMaxKey, 0, 65535, GetUDPPortMax(), "", "Maximum of port range for allocating UDP streams"));
  SetRtpIpPorts(rsrc->AddIntegerField(RTPPortBaseKey, 0, 65535, GetRtpIpPortBase(), "", "Base of port range for allocating RTP/UDP streams"),
                rsrc->AddIntegerField(RTPPortMaxKey, 0, 65535, GetRtpIpPortMax(), "", "Maximum of port range for allocating RTP/UDP streams"));

  SetMediaTypeOfService(rsrc->AddIntegerField(RTPTOSKey, 0, 255, GetMediaTypeOfService(), "", "Value for DIFSERV Quality of Service"));

#if P_STUN
  {
    std::set<NATInfo> natInfo;

    // Need to make a copy of info as call SetNATServer alters GetNatMethods() so iterator fails
    for (PNatMethods::iterator it = GetNatMethods().begin(); it != GetNatMethods().end(); ++it)
      natInfo.insert(*it);

    for (std::set<NATInfo>::iterator it = natInfo.begin(); it != natInfo.end(); ++it) {
      PHTTPCompositeField * fields = new PHTTPCompositeField("NAT\\" + it->m_method, it->m_method,
                   "Enable flag and Server IP/hostname for NAT traversal using " + it->m_friendly);
      fields->Append(new PHTTPBooleanField(NATActiveKey, it->m_active));
      fields->Append(new PHTTPStringField(NATServerKey, 0, 0, it->m_server, NULL, 1, 50));
      rsrc->Add(fields);
      if (!fields->LoadFromConfig(cfg))
        SetNATServer(it->m_method, (*fields)[1].GetValue(), (*fields)[0].GetValue() *= "true");
    }
  }
#endif // P_NAT

  bool overrideProductInfo = rsrc->AddBooleanField(OverrideProductInfoKey, false, "Override the default product information");
  m_savedProductInfo.vendor = rsrc->AddStringField(VendorNameKey, 20, m_savedProductInfo.vendor);
  m_savedProductInfo.name = rsrc->AddStringField(ProductNameKey, 20, m_savedProductInfo.name);
  m_savedProductInfo.version = rsrc->AddStringField(ProductVersionKey, 20, m_savedProductInfo.version);
  if (overrideProductInfo)
    SetProductInfo(m_savedProductInfo);

#if OPAL_H323
  if (!GetH323EndPoint().Configure(cfg, rsrc))
    return false;
#endif // OPAL_H323

#if OPAL_SIP
  if (!GetSIPEndPoint().Configure(cfg, rsrc))
    return false;
#endif // OPAL_SIP

#if OPAL_SDP_HTTP
  {
    OpalSDPHTTPEndPoint * ep = FindEndPointAs<OpalSDPHTTPEndPoint>(OPAL_PREFIX_SDP);
    if (!ConfigureCommon(ep, "SDP", cfg, rsrc))
      return false;
  }
#endif // OPAL_SDP_HTTP

#if OPAL_SKINNY
  if (!GetSkinnyEndPoint().Configure(cfg, rsrc))
    return false;
#endif // OPAL_SKINNY

#if OPAL_LID
  // Add POTS and PSTN endpoints
  if (!FindEndPointAs<OpalLineEndPoint>(OPAL_PREFIX_POTS)->AddDeviceNames(rsrc->AddSelectArrayField(LIDKey, false,
    OpalLineInterfaceDevice::GetAllDevices(), PStringArray(), "Line Interface Devices (PSTN, ISDN etc) to monitor, if any"))) {
    PSYSTEMLOG(Error, "No LID devices!");
  }
#endif // OPAL_LID


#if OPAL_CAPI
  m_enableCAPI = rsrc->AddBooleanField(EnableCAPIKey, m_enableCAPI, "Enable CAPI ISDN controller(s), if available");
  if (m_enableCAPI && FindEndPointAs<OpalCapiEndPoint>(OPAL_PREFIX_CAPI)->OpenControllers() == 0) {
    PSYSTEMLOG(Error, "No CAPI controllers!");
  }
#endif


#if OPAL_IVR
  {
    OpalIVREndPoint * ivrEP = FindEndPointAs<OpalIVREndPoint>(OPAL_PREFIX_IVR);
    // Set IVR protocol handler
    ivrEP->SetDefaultVXML(rsrc->AddStringField(VXMLKey, 0, ivrEP->GetDefaultVXML(),
      "Interactive Voice Response VXML script, may be a URL or the actual VXML", 10, 80));
    ivrEP->SetCacheDir(rsrc->AddStringField(IVRCacheKey, 0, ivrEP->GetCacheDir(),
      "Interactive Voice Response directory to cache Text To Speech phrases", 1, 50));
    ivrEP->SetRecordDirectory(rsrc->AddStringField(IVRRecordDirKey, 0, ivrEP->GetRecordDirectory(),
      "Interactive Voice Response directory to save recorded messages", 1, 50));
  }
#endif

#if OPAL_HAS_MIXER
  {
    OpalMixerEndPoint * mcuEP = FindEndPointAs<OpalMixerEndPoint>(OPAL_PREFIX_MIXER);
    OpalMixerNodeInfo adHoc;
    if (mcuEP->GetAdHocNodeInfo() != NULL)
      adHoc = *mcuEP->GetAdHocNodeInfo();
    adHoc.m_mediaPassThru = rsrc->AddBooleanField(ConfMediaPassThruKey, adHoc.m_mediaPassThru, "Conference media pass though optimisation");

#if OPAL_VIDEO
    adHoc.m_audioOnly = rsrc->AddBooleanField(ConfAudioOnlyKey, adHoc.m_audioOnly, "Conference is audio only");

    PVideoFrameInfo::ParseSize(rsrc->AddStringField(ConfVideoResolutionKey, 10,
      PVideoFrameInfo::AsString(adHoc.m_width, adHoc.m_height),
      "Conference video frame resolution"),
      adHoc.m_width, adHoc.m_height);
#endif

    adHoc.m_closeOnEmpty = true;
    mcuEP->SetAdHocNodeInfo(adHoc);
  }
#endif

#if OPAL_SCRIPT
  PFactory<PScriptLanguage>::KeyList_T keys = PFactory<PScriptLanguage>::GetKeyList();
  if (!keys.empty()) {
    PStringArray languages;
    for (PFactory<PScriptLanguage>::KeyList_T::iterator it = keys.begin(); it != keys.end(); ++it)
      languages.AppendString(*it);
    PCaselessString language = cfg.GetString(ScriptLanguageKey, languages[0]);
    rsrc->Add(new PHTTPRadioField(ScriptLanguageKey, languages,
              languages.GetValuesIndex(language),"Interpreter script language."));

    PString script = rsrc->AddStringField(ScriptTextKey, 0, PString::Empty(),
                                          "Interpreter script, may be a filename or the actual script text", 10, 80);
    if (language != m_scriptLanguage || script != m_scriptText) {
      m_scriptLanguage = language;
      m_scriptText = script;
      RunScript(script, language);
    }
  }
#endif //OPAL_SCRIPT

  // Routing
  PHTTPCompositeField * routeFields = new PHTTPCompositeField(ROUTES_KEY, ROUTES_SECTION,
    "Internal routing of calls to varous sub-systems.<p>"
    "The A Party and B Party columns are regular expressions for the call "
    "originator and receiver respectively. The Destination string determines "
    "the endpoint used for the outbound leg of the route. This can be be "
    "constructed using various meta-strings that correspond to parts of the "
    "B Party address.<p>"
    "A Destination starting with the string 'label:' causes the router "
    "to restart searching from the beginning of the route table using "
    "the new string as the A Party<p>"
    "The available meta-strings are:<p>"
    "<dl>"
    "<dt><code>&lt;da&gt;</code></dt><dd>"
    "Replaced by the B Party string. For example A Party=\"pc:.*\" "
    "B Party=\".*\" Destination=\"sip:&lt;da&gt;\" directs calls "
    "to the SIP protocol. In this case there is a special condition "
    "where if the original destination had a valid protocol, eg "
    "h323:fred.com, then the entire string is replaced not just "
    "the &lt;da&gt; part.</dd>"
    "<dt><code>&lt;db&gt;</code></dt><dd>"
    "Same as &lt;da&gt;, but without the special condtion.</dd>"
    "<dt><code>&lt;du&gt;</code></dt><dd>"
    "Copy the \"user\" part of the B Party string. This is "
    "essentially the component after the : and before the '@', or "
    "the whole B Party string if these are not present.</dd>"
    "<dt><code>&lt;!du&gt;</code></dt><dd>"
    "The rest of the B Party string after the &lt;du&gt; section. The "
    "protocol is still omitted. This is usually the '@' and onward. "
    "Note, if there is already an '@' in the destination before the "
    "&lt;!du&gt; and what is about to replace it also has an '@' then "
    "everything between the @ and the &lt;!du&gt; (inclusive) is deleted, "
    "then the substitution is made so a legal URL can be produced.</dd>"
    "<dt><code>&lt;dn&gt;</code></dt><dd>"
    "Copy all valid consecutive E.164 digits from the B Party so "
    "pots:0061298765@vpb:1/2 becomes sip:0061298765@carrier.com</dd>"
    "<dt><code>&lt;dnX&gt;</code></dt><dd>"
    "As above but skip X digits, eg &lt;dn2&gt; skips 2 digits, so "
    "pots:00612198765 becomes sip:61298765@carrier.com</dd>"
    "<dt><code>&lt;!dn&gt;</code></dt><dd>"
    "The rest of the B Party after the &lt;dn&gt; or &lt;dnX&gt; sections.</dd>"
    "<dt><code>&lt;dn2ip&gt;</code></dt><dd>"
    "Translate digits separated by '*' characters to an IP "
    "address. e.g. 10*0*1*1 becomes 10.0.1.1, also "
    "1234*10*0*1*1 becomes 1234@10.0.1.1 and "
    "1234*10*0*1*1*1722 becomes 1234@10.0.1.1:1722.</dd>"
    "</dl>"
    );
  routeFields->Append(new PHTTPStringField(RouteAPartyKey, 0, NULL, NULL, 1, 20));
  routeFields->Append(new PHTTPStringField(RouteBPartyKey, 0, NULL, NULL, 1, 20));
  routeFields->Append(new PHTTPStringField(RouteDestKey, 0, NULL, NULL, 1, 30));
  PHTTPFieldArray * routeArray = new PHTTPFieldArray(routeFields, true);
  rsrc->Add(routeArray);

  RouteTable routes;
  if (routeArray->LoadFromConfig(cfg) || routeArray->GetSize() == 0) {
    routeArray->SetSize(PARRAYSIZE(DefaultRoutes));
    for (PINDEX i = 0; i < PARRAYSIZE(DefaultRoutes); ++i) {
      PHTTPCompositeField & item = dynamic_cast<PHTTPCompositeField &>((*routeArray)[i]);
      RouteEntry * entry = new RouteEntry(DefaultRoutes[i]);
      PAssert(entry->IsValid(), PLogicError);
      routes.Append(entry);
      item[0].SetValue(entry->GetPartyA());
      item[1].SetValue(entry->GetPartyB());
      item[2].SetValue(entry->GetDestination());
    }
    routeArray->SaveToConfig(cfg);
  }
  else {
    for (PINDEX i = 0; i < routeArray->GetSize(); ++i) {
      PHTTPCompositeField & item = dynamic_cast<PHTTPCompositeField &>((*routeArray)[i]);
      RouteEntry * entry = new RouteEntry(item[0].GetValue(), item[1].GetValue(), item[2].GetValue());
      if (entry->IsValid())
        routes.Append(entry);
      else
        delete entry;
    }
  }

  SetRouteTable(routes);

  return ConfigureCDR(cfg, rsrc);
}


OpalCall * MyManager::CreateCall(void *)
{
  return new MyCall(*this);
}


MyManager::MediaTransferMode MyManager::GetMediaTransferMode(const OpalConnection &, const OpalConnection &, const OpalMediaType &) const
{
  return m_mediaTransferMode;
}


void MyManager::OnStartMediaPatch(OpalConnection & connection, OpalMediaPatch & patch)
{
  PSafePtr<OpalConnection> other = connection.GetOtherPartyConnection();
  if (other != NULL) {
    // Detect if we have loopback endpoint, and start pas thro on the other connection
    if (connection.GetPrefixName() == LoopbackPrefix)
      SetMediaPassThrough(*other, *other, true, patch.GetSource().GetSessionID());
    else if (other->GetPrefixName() == LoopbackPrefix)
      SetMediaPassThrough(connection, connection, true, patch.GetSource().GetSessionID());
  }

  dynamic_cast<MyCall &>(connection.GetCall()).OnStartMediaPatch(connection, patch);
  OpalManager::OnStartMediaPatch(connection, patch);
}


void MyManager::OnStopMediaPatch(OpalConnection & connection, OpalMediaPatch & patch)
{
  dynamic_cast<MyCall &>(connection.GetCall()).OnStopMediaPatch(patch);
  OpalManager::OnStopMediaPatch(connection, patch);
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_SKINNY

OpalConsoleSkinnyEndPoint * MyManager::CreateSkinnyEndPoint()
{
  return new MySkinnyEndPoint(*this);
}


MySkinnyEndPoint::MySkinnyEndPoint(MyManager & mgr)
  : OpalConsoleSkinnyEndPoint(mgr)
  , m_manager(mgr)
  , m_deviceType(OpalSkinnyEndPoint::DefaultDeviceType)
{
}


bool MySkinnyEndPoint::Configure(PConfig &, PConfigPage * rsrc)
{
  m_defaultServer = rsrc->AddStringField(SkinnyServerKey, 0, PString::Empty(),
                                  "Default server for Skinny Client Control Protocol (SCCP).", 1, 30);
  SetServerInterfaces(m_defaultInterface = rsrc->AddStringField(SkinnyInterfaceKey, 0, m_defaultInterface,
                                  "Default network interface for Skinny Client Control Protocol (SCCP).", 1, 30));
  m_deviceType = rsrc->AddIntegerField(SkinnyTypeKey, 1, 32767, m_deviceType, "",
                                       "Device type for SCCP. Default 30016 = Cisco IP Communicator.");
  m_deviceNames = rsrc->AddStringArrayField(SkinnyNamesKey, false, 0, m_deviceNames,
                                            "Names of all devices to simulate for SCCP.", 1, 50);
  SetSimulatedAudioFile(rsrc->AddStringField(SkinnySimulatedAudioFileKey, 0, GetSimulatedAudioFile(),
                        "WAV file to simulate audio on SCCP when can't gateway channel.", 1, 80));

  PStringArray newExpandedNames, servers;
  ExpandWildcards(m_deviceNames, newExpandedNames, servers);
  unsigned unregisterCount = 0;
  for (PINDEX i = 0; i < m_expandedDeviceNames.GetSize(); ++i) {
    if (newExpandedNames.GetValuesIndex(m_expandedDeviceNames[i]) == P_MAX_INDEX) {
      Unregister(m_expandedDeviceNames[i]);
      ++unregisterCount;
    }
  }

  unsigned registerCount = 0;
  for (PINDEX i = 0; i < newExpandedNames.GetSize(); ++i) {
    PString server, localInterface;
    servers[i].Split(";interface=", server, localInterface, PString::SplitDefaultToBefore | PString::SplitTrim);
    if (Register(server, newExpandedNames[i], m_deviceType, localInterface))
      ++registerCount;
  }

  PSYSTEMLOG(Info, "SCCP unregistered " << unregisterCount << " of " << m_expandedDeviceNames.GetSize()
                          << " devices and registered " << registerCount << " new devices");

  m_expandedDeviceNames = newExpandedNames;
  return true;
}


void MySkinnyEndPoint::AutoRegister(const PString & server, const PString & wildcard, const PString & localInterface, bool registering)
{
  PString actualServer = server.IsEmpty() ? m_defaultServer : server;

  PStringArray names, servers;
  ExpandWildcards(wildcard, names, servers);

  for (PINDEX i = 0; i < names.GetSize(); ++i) {
    PString name = names[i];

    if (registering)
      Register(actualServer, name, m_deviceType, localInterface);
    else
      Unregister(name);
  }
}


void MySkinnyEndPoint::ExpandWildcards(const PStringArray & input, PStringArray & names, PStringArray & servers)
{
  for (PINDEX i = 0; i < input.GetSize(); ++i) {
    PString str = input[i];

    PIntArray starts(4), ends(4);
    static PRegularExpression const Wildcards("([0-9]+)\\.\\.([0-9]+)(@.*)?$", PRegularExpression::Extended);
    if (Wildcards.Execute(str, starts, ends)) {
      PString server = (ends[3] - starts[3]) > 2 ? str(starts[3]+1, ends[3]-1) : m_defaultServer;
      uint64_t number = str(starts[1], ends[1] - 1).AsUnsigned64();
      uint64_t lastNumber = str(starts[2], ends[2] - 1).AsUnsigned64();
      unsigned digits = ends[1] - starts[1];
      str.Delete(starts[1], P_MAX_INDEX);
      while (number <= lastNumber) {
        names.AppendString(PSTRSTRM(str << setfill('0') << setw(digits) << number++));
        servers.AppendString(server);
      }
    }
    else {
      PString name, server;
      if (str.Split('@', name, server)) {
        names.AppendString(name);
        servers.AppendString(server);
      }
      else {
        names.AppendString(str);
        servers.AppendString(m_defaultServer);
      }
    }
  }
}

#endif // OPAL_SKINNY


// End of File ///////////////////////////////////////////////////////////////
