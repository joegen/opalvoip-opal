/*
 * gkerv.cxx
 *
 * PWLib application source file for OPAL Gateway
 *
 * Main program entry point.
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include "precompile.h"
#include "main.h"

#if OPAL_H323

#include <ptclib/random.h>

#ifdef H323_TRANSNEXUS_OSP
#include <opalosp.h>
#endif

//#define TEST_TOKEN

static const char H323AliasesKey[] = "H.323 Aliases";
static const char DisableFastStartKey[] = "Disable Fast Start";
static const char DisableH245TunnelingKey[] = "Disable H.245 Tunneling";
static const char DisableH245inSetupKey[] = "Disable H.245 in Setup";
static const char H323BandwidthKey[] = "H.323 Bandwidth";
static const char H323ListenersKey[] = "H.323 Interfaces";
static const char GatekeeperEnableKey[] = "Remote Gatekeeper Enable";
static const char GatekeeperAddressKey[] = "Remote Gatekeeper Address";
static const char RemoteGatekeeperIdentifierKey[] = "Remote Gatekeeper Identifier";
static const char GatekeeperInterfaceKey[] = "Remote Gatekeeper Interface";
static const char GatekeeperPasswordKey[] = "Remote Gatekeeper Password";
static const char GatekeeperTokenOIDKey[] = "Remote Gatekeeper Token OID";

static const char ServerGatekeeperIdentifierKey[] = "Server Gatekeeper Identifier";
static const char AvailableBandwidthKey[] = "Total Bandwidth";
static const char DefaultBandwidthKey[] = "Default Bandwidth Allocation";
static const char MaximumBandwidthKey[] = "Maximum Bandwidth Allocation";
static const char DefaultTimeToLiveKey[] = "Default Time To Live";
static const char CallHeartbeatTimeKey[]    = "Call Heartbeat Time";
static const char OverwriteOnSameSignalAddressKey[] = "Overwrite EP On Same Signal Address";
static const char CanHaveDuplicateAliasKey[] = "Can Have Duplicate Alias";
static const char CanOnlyCallRegisteredEPKey[] = "Can Only Call Registered EP";
static const char CanOnlyAnswerRegisteredEPKey[] = "Can Only Answer Registered EP";
static const char AnswerCallPreGrantedARQKey[] = "Answer Call Pregranted ARQ";
static const char MakeCallPreGrantedARQKey[] = "Make Call Pregranted ARQ";
static const char IsGatekeeperRoutedKey[] = "Gatekeeper Routed";
static const char AliasCanBeHostNameKey[] = "Alias Can Be Host Name";
static const char RequireH235Key[] = "Require H.235";
static const char UsernameKey[] = "Username";
static const char PasswordKey[] = "Password";
static const char RouteAliasKey[] = "Alias";
static const char RouteHostKey[] = "Host";
static const char ClearingHouseKey[] = "H501 Clearing House";
static const char H501InterfaceKey[] = "H501 Interface";
static const char GkServerListenersKey[] = "Server Gatekeeper Interfaces";

#ifdef H323_TRANSNEXUS_OSP
static const char OSPRoutingURLKey[] = "OSP Routing URL";
static const char OSPPrivateKeyFileKey[] = "OSP Private Key";
static const char OSPPublicKeyFileKey[] = "OSP Public Key";
static const char OSPServerKeyFileKey[] = "OSP Server Key";
#endif

#define LISTENER_INTERFACE_KEY        "Interface"

#define USERS_SECTION "Authentication"
#define USERS_KEY     "Credentials"

#define ROUTES_SECTION "Route Maps"
#define ROUTES_KEY     "Mapping"

static const char * CompatibilityIssueKey[H323Connection::NumCompatibilityIssues] = {
  "No Multiple Tunnelled H245",
  "Bad Master Slave Conflict",
  "No User Input Capability"
};


#define new PNEW


///////////////////////////////////////////////////////////////

MyH323EndPoint::MyH323EndPoint(MyManager & mgr)
  : H323EndPoint(mgr)
  , P_DISABLE_MSVC_WARNINGS(4355, m_gkServer(*this))
{
  terminalType = e_MCUWithAVMP;
}


bool MyH323EndPoint::Initialise(PConfig & cfg, PConfigPage * rsrc)
{
  PHTTPFieldArray * fieldArray;


  // Add H.323 parameters
  fieldArray = new PHTTPFieldArray(new PHTTPStringField(H323AliasesKey, 25,
                                   "", "H.323 Alias names for local user"), true);
  PStringArray aliases = fieldArray->GetStrings(cfg);
  if (aliases.IsEmpty())
    fieldArray->SetStrings(cfg, GetAliasNames());
  else {
    SetLocalUserName(aliases[0]);
    for (PINDEX i = 1; i < aliases.GetSize(); i++)
      AddAliasName(aliases[i]);
  }
  rsrc->Add(fieldArray);

  DisableFastStart(cfg.GetBoolean(DisableFastStartKey, IsFastStartDisabled()));
  rsrc->Add(new PHTTPBooleanField(DisableFastStartKey,  IsFastStartDisabled(),
            "Disable H.323 Fast Connect feature"));

  DisableH245Tunneling(cfg.GetBoolean(DisableH245TunnelingKey, IsH245TunnelingDisabled()));
  rsrc->Add(new PHTTPBooleanField(DisableH245TunnelingKey,  IsH245TunnelingDisabled(),
            "Disable H.245 tunneled in H.225.0 signalling channel"));

  DisableH245inSetup(cfg.GetBoolean(DisableH245inSetupKey, IsH245inSetupDisabled()));
  rsrc->Add(new PHTTPBooleanField(DisableH245inSetupKey,  IsH245inSetupDisabled(),
            "Disable sending initial tunneled H.245 PDU in SETUP PDU"));

  OpalBandwidth bandwidth = cfg.GetInteger(H323BandwidthKey, GetInitialBandwidth(OpalBandwidth::RxTx)/1000);
  SetInitialBandwidth(OpalBandwidth::RxTx, bandwidth*1000);
  rsrc->Add(new PHTTPIntegerField(H323BandwidthKey, 1, OpalBandwidth::Max()/1000, bandwidth,
            "kb/s", "Bandwidth to request to gatekeeper on originating/answering calls"));

  for (PINDEX i = 0; i < H323Connection::NumCompatibilityIssues; ++i) {
    H323Connection::CompatibilityIssues issue = (H323Connection::CompatibilityIssues)i;
    PString key = PAssertNULL(CompatibilityIssueKey[issue]);
    SetCompatibility(issue, cfg.GetString(key, GetCompatibility(issue)));
    rsrc->Add(new PHTTPStringField(key, 25, GetCompatibility(issue),
              "Compatibility issue work around, product name/version regular expression"));
  }

  fieldArray = new PHTTPFieldArray(new PHTTPStringField(H323ListenersKey, 25,
                                   "", "Local network interfaces to listen for H.323, blank means all"), false);
  if (!StartListeners(fieldArray->GetStrings(cfg))) {
    PSYSTEMLOG(Error, "Could not open any H.323 listeners!");
  }
  rsrc->Add(fieldArray);

  bool gkEnable = cfg.GetBoolean(GatekeeperEnableKey, false);
  rsrc->Add(new PHTTPBooleanField(GatekeeperEnableKey, gkEnable,
            "Enable registration with gatekeeper as client"));

  PString gkAddress = cfg.GetString(GatekeeperAddressKey);
  rsrc->Add(new PHTTPStringField(GatekeeperAddressKey, 25, gkAddress,
            "IP/hostname of gatekeeper to register with, if blank a broadcast is used"));

  PString gkIdentifier = cfg.GetString(RemoteGatekeeperIdentifierKey);
  rsrc->Add(new PHTTPStringField(RemoteGatekeeperIdentifierKey, 25, gkIdentifier,
            "Gatekeeper identifier to register with, if blank any gatekeeper is used"));

  PString gkInterface = cfg.GetString(GatekeeperInterfaceKey);
  rsrc->Add(new PHTTPStringField(GatekeeperInterfaceKey, 25, gkInterface,
            "Local network interface to use to register with gatekeeper, if blank all are used"));

  PString gkPassword = PHTTPPasswordField::Decrypt(cfg.GetString(GatekeeperPasswordKey));
  if (!gkPassword)
    SetGatekeeperPassword(gkPassword);
  rsrc->Add(new PHTTPPasswordField(GatekeeperPasswordKey, 25, gkPassword,
            "Password for gatekeeper authentication, user is the first alias"));

  SetGkAccessTokenOID(cfg.GetString(GatekeeperTokenOIDKey));
  rsrc->Add(new PHTTPStringField(GatekeeperTokenOIDKey, 25, GetGkAccessTokenOID(),
            "Gatekeeper access token OID for H.235 support"));

  if (gkEnable) {
    if (UseGatekeeper(gkAddress, gkIdentifier, gkInterface)) {
      PSYSTEMLOG(Info, "Register with gatekeeper " << *GetGatekeeper());
    }
    else {
      PSYSTEMLOG(Error, "Could not register with gatekeeper!");
    }
  }
  else {
    PSYSTEMLOG(Info, "Not using gatekeeper.");
    RemoveGatekeeper();
  }

  return m_gkServer.Initialise(cfg, rsrc);
}


///////////////////////////////////////////////////////////////

GkStatusPage::GkStatusPage(MyManager & mgr, PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "GkStatus")
  , m_gkServer(mgr.FindEndPointAs<MyH323EndPoint>("h323")->GetGatekeeperServer())
{
}


const char * GkStatusPage::GetTitle() const
{
  return "OPAL Gatekeeper Status";
}


void GkStatusPage::CreateContent(PHTML & html) const
{
  html << PHTML::TableStart("border=1")
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
         << PHTML::TableData("NOWRAP")
         << "<!--#status EndPointAliases-->"
         << PHTML::TableData("NOWRAP")
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


PString MyGatekeeperServer::OnLoadEndPointStatus(const PString & htmlBlock)
{
  PINDEX i;
  PString substitution;

  for (PSafePtr<H323RegisteredEndPoint> ep = GetFirstEndPoint(PSafeReadOnly); ep != NULL; ep++) {
    // make a copy of the repeating html chunk
    PString insert = htmlBlock;

    PServiceHTML::SpliceMacro(insert, "status EndPointIdentifier", ep->GetIdentifier());

    PStringStream addresses;
    for (i = 0; i < ep->GetSignalAddressCount(); i++) {
      if (i > 0)
        addresses << "<br>";
      addresses << ep->GetSignalAddress(i);
    }
    PServiceHTML::SpliceMacro(insert, "status CallSignalAddresses", addresses);

    PStringStream aliases;
    for (i = 0; i < ep->GetAliasCount(); i++) {
      if (i > 0)
        aliases << "<br>";
      aliases << ep->GetAlias(i);
    }
    PServiceHTML::SpliceMacro(insert, "status EndPointAliases", aliases);

    PString str = "<i>Name:</i> " + ep->GetApplicationInfo();
    str.Replace("\t", "<BR><i>Version:</i> ");
    str.Replace("\t", " <i>Vendor:</i> ");
    PServiceHTML::SpliceMacro(insert, "status Application", str);

    PServiceHTML::SpliceMacro(insert, "status ActiveCalls", ep->GetCallCount());

    // Then put it into the page, moving insertion point along after it.
    substitution += insert;
  }

  return substitution;
}


PCREATE_SERVICE_MACRO_BLOCK(EndPointStatus,resource,P_EMPTY,block)
{
  GkStatusPage * status = dynamic_cast<GkStatusPage *>(resource.m_resource);
  return PAssertNULL(status)->m_gkServer.OnLoadEndPointStatus(block);
}


///////////////////////////////////////////////////////////////

MyGatekeeperServer::MyGatekeeperServer(H323EndPoint & ep)
  : H323GatekeeperServer(ep),
    endpoint(ep)
{
#ifdef H323_TRANSNEXUS_OSP
  ospProvider = NULL;
#endif
}


bool MyGatekeeperServer::Initialise(PConfig & cfg, PConfigPage * rsrc)
{
  PINDEX i;

  PWaitAndSignal mutex(reconfigurationMutex);

#ifdef H323_H501

  // set clearing house address
  PString clearingHouse = cfg.GetString(ClearingHouseKey);
  rsrc->Add(new PHTTPStringField(ClearingHouseKey, 25, clearingHouse));

  PString h501Interface = cfg.GetString(H501InterfaceKey);
  rsrc->Add(new PHTTPStringField(H501InterfaceKey, 25, h501Interface));

  if (clearingHouse.IsEmpty())
    SetPeerElement(NULL);
  else {
    if (!h501Interface)
      CreatePeerElement(h501Interface);
    if (!OpenPeerElement(clearingHouse)) {
      PSYSTEMLOG(Error, "Main\tCould not open clearing house at: " << clearingHouse);
    }
  }
#endif

#ifdef H323_TRANSNEXUS_OSP
  PString oldOSPServer = ospRoutingURL;
  ospRoutingURL = cfg.GetString(OSPRoutingURLKey, ospRoutingURL);
  rsrc->Add(new PHTTPStringField(OSPRoutingURLKey, 25, ospRoutingURL));
  bool ospChanged = oldOSPServer != ospRoutingURL;

  ospPrivateKeyFileName = cfg.GetString(OSPPrivateKeyFileKey, ospPrivateKeyFileName);
  rsrc->Add(new PHTTPStringField(OSPPrivateKeyFileKey, 25, ospPrivateKeyFileName));

  ospPublicKeyFileName = cfg.GetString(OSPPublicKeyFileKey, ospPublicKeyFileName);
  rsrc->Add(new PHTTPStringField(OSPPublicKeyFileKey, 25, ospPublicKeyFileName));

  ospServerKeyFileName = cfg.GetString(OSPServerKeyFileKey, ospServerKeyFileName);
  rsrc->Add(new PHTTPStringField(OSPServerKeyFileKey, 25, ospServerKeyFileName));

  if (!ospRoutingURL.IsEmpty()) {
    if (ospProvider != NULL && ospProvider->IsOpen() && ospChanged) {
      delete ospProvider;
      ospProvider = NULL;
    }
    ospProvider = new OpalOSP::Provider();
    int status;
    if (ospPrivateKeyFileName.IsEmpty() && ospPublicKeyFileName.IsEmpty() && ospServerKeyFileName.IsEmpty())
      status = ospProvider->Open(ospRoutingURL);
    else
      status = ospProvider->Open(ospRoutingURL, ospPrivateKeyFileName, ospPublicKeyFileName, ospServerKeyFileName);
    if (status != 0) {
      delete ospProvider;
      ospProvider = NULL;
    }
  } 
  
  else if (ospProvider != NULL) {
    ospProvider->Close();
    delete ospProvider;
    ospProvider = NULL;
  }

#endif

  PString gkid = cfg.GetString(ServerGatekeeperIdentifierKey, MyProcess::Current().GetName() + " on " + PIPSocket::GetHostName());
  rsrc->Add(new PHTTPStringField(ServerGatekeeperIdentifierKey, 25, gkid));
  SetGatekeeperIdentifier(gkid);

  // Interfaces to listen on
  PHTTPFieldArray * fieldArray = new PHTTPFieldArray(new PHTTPStringField(GkServerListenersKey, 25), false);
  H323TransportAddressArray interfaces = fieldArray->GetStrings(cfg);
  AddListeners(interfaces);
  rsrc->Add(fieldArray);

  SetAvailableBandwidth(cfg.GetInteger(AvailableBandwidthKey, GetAvailableBandwidth()/10)*10);
  rsrc->Add(new PHTTPIntegerField(AvailableBandwidthKey, 1, INT_MAX, GetAvailableBandwidth()/10, "kb/s"));

  defaultBandwidth = cfg.GetInteger(DefaultBandwidthKey, defaultBandwidth/10)*10;
  rsrc->Add(new PHTTPIntegerField(DefaultBandwidthKey, 1, INT_MAX, defaultBandwidth/10, "kb/s"));

  maximumBandwidth = cfg.GetInteger(MaximumBandwidthKey, maximumBandwidth/10)*10;
  rsrc->Add(new PHTTPIntegerField(MaximumBandwidthKey, 1, INT_MAX, maximumBandwidth/10, "kb/s"));

  defaultTimeToLive = cfg.GetInteger(DefaultTimeToLiveKey, defaultTimeToLive);
  rsrc->Add(new PHTTPIntegerField(DefaultTimeToLiveKey, 10, 86400, defaultTimeToLive, "seconds"));

  defaultInfoResponseRate = cfg.GetInteger(CallHeartbeatTimeKey, defaultInfoResponseRate);
  rsrc->Add(new PHTTPIntegerField(CallHeartbeatTimeKey, 0, 86400, defaultInfoResponseRate, "seconds"));

  overwriteOnSameSignalAddress = cfg.GetBoolean(OverwriteOnSameSignalAddressKey, overwriteOnSameSignalAddress);
  rsrc->Add(new PHTTPBooleanField(OverwriteOnSameSignalAddressKey, overwriteOnSameSignalAddress));

  canHaveDuplicateAlias = cfg.GetBoolean(CanHaveDuplicateAliasKey, canHaveDuplicateAlias);
  rsrc->Add(new PHTTPBooleanField(CanHaveDuplicateAliasKey, canHaveDuplicateAlias));

  canOnlyCallRegisteredEP = cfg.GetBoolean(CanOnlyCallRegisteredEPKey, canOnlyCallRegisteredEP);
  rsrc->Add(new PHTTPBooleanField(CanOnlyCallRegisteredEPKey, canOnlyCallRegisteredEP));

  canOnlyAnswerRegisteredEP = cfg.GetBoolean(CanOnlyAnswerRegisteredEPKey, canOnlyAnswerRegisteredEP);
  rsrc->Add(new PHTTPBooleanField(CanOnlyAnswerRegisteredEPKey, canOnlyAnswerRegisteredEP));

  answerCallPreGrantedARQ = cfg.GetBoolean(AnswerCallPreGrantedARQKey, answerCallPreGrantedARQ);
  rsrc->Add(new PHTTPBooleanField(AnswerCallPreGrantedARQKey, answerCallPreGrantedARQ));

  makeCallPreGrantedARQ = cfg.GetBoolean(MakeCallPreGrantedARQKey, makeCallPreGrantedARQ);
  rsrc->Add(new PHTTPBooleanField(MakeCallPreGrantedARQKey, makeCallPreGrantedARQ));

  aliasCanBeHostName = cfg.GetBoolean(AliasCanBeHostNameKey, aliasCanBeHostName);
  rsrc->Add(new PHTTPBooleanField(AliasCanBeHostNameKey, aliasCanBeHostName));

  isGatekeeperRouted = cfg.GetBoolean(IsGatekeeperRoutedKey, isGatekeeperRouted);
  rsrc->Add(new PHTTPBooleanField(IsGatekeeperRoutedKey, isGatekeeperRouted));

  routes.RemoveAll();
  PINDEX arraySize = cfg.GetInteger(ROUTES_SECTION, ROUTES_KEY" Array Size");
  for (i = 0; i < arraySize; i++) {
    cfg.SetDefaultSection(psprintf(ROUTES_SECTION"\\"ROUTES_KEY" %u", i+1));
    RouteMap map(cfg.GetString(RouteAliasKey), cfg.GetString(RouteHostKey));
    if (map.IsValid())
      routes.Append(new RouteMap(map));
  }

  PHTTPCompositeField * routeFields = new PHTTPCompositeField(ROUTES_SECTION"\\"ROUTES_KEY" %u\\", "Alias Route Maps");
  routeFields->Append(new PHTTPStringField(RouteAliasKey, 20));
  routeFields->Append(new PHTTPStringField(RouteHostKey, 30));
  rsrc->Add(new PHTTPFieldArray(routeFields, true));

  requireH235 = cfg.GetBoolean(RequireH235Key, requireH235);
  rsrc->Add(new PHTTPBooleanField(RequireH235Key, requireH235));

  passwords.RemoveAll();
  arraySize = cfg.GetInteger(USERS_SECTION, USERS_KEY" Array Size");
  for (i = 0; i < arraySize; i++) {
    cfg.SetDefaultSection(psprintf(USERS_SECTION"\\"USERS_KEY" %u", i+1));
    PString username = cfg.GetString(UsernameKey);
    if (!username) {
      PString password = PHTTPPasswordField::Decrypt(cfg.GetString(PasswordKey));
      passwords.SetAt(username, password);
    }
  }

  PHTTPCompositeField * security = new PHTTPCompositeField(USERS_SECTION"\\"USERS_KEY" %u\\", "Authentication Credentials");
  security->Append(new PHTTPStringField(UsernameKey, 25));
  security->Append(new PHTTPPasswordField(PasswordKey, 25));
  rsrc->Add(new PHTTPFieldArray(security, false));

  return true;
}


H323GatekeeperCall * MyGatekeeperServer::CreateCall(const OpalGloballyUniqueID & id,
                                                    H323GatekeeperCall::Direction dir)
{
  return new MyGatekeeperCall(*this, id, dir);
}


PBoolean MyGatekeeperServer::TranslateAliasAddress(const H225_AliasAddress & alias,
                                                   H225_ArrayOf_AliasAddress & aliases,
                                                   H323TransportAddress & address,
                                                   PBoolean & isGkRouted,
                                                   H323GatekeeperCall * call)
{
#ifdef H323_TRANSNEXUS_OSP
  if (ospProvider != NULL) {
    address = "127.0.0.1";
    return true;
  }
#endif

  if (H323GatekeeperServer::TranslateAliasAddress(alias, aliases, address, isGkRouted, call))
    return true;

  PString aliasString = H323GetAliasAddressString(alias);
  PTRACE(4, "Translating \"" << aliasString << "\" through route maps");

  PWaitAndSignal mutex(reconfigurationMutex);

  for (PINDEX i = 0; i < routes.GetSize(); i++) {
    PTRACE(4, "Checking route map " << routes[i]);
    if (routes[i].IsMatch(aliasString)) {
      address = routes[i].GetHost();
      PTRACE(3, "Translated \"" << aliasString << "\" to " << address);
      break;
    }
  }

  return true;
}


MyGatekeeperServer::RouteMap::RouteMap(const PString & theAlias, const PString & theHost)
  : alias(theAlias),
    regex('^' + theAlias + '$'),
    host(theHost)
{
}


void MyGatekeeperServer::RouteMap::PrintOn(ostream & strm) const
{
  strm << '"' << alias << "\" => " << host;
}


bool MyGatekeeperServer::RouteMap::IsValid() const
{
  return !alias && regex.GetErrorCode() == PRegularExpression::NoError && !host;
}


bool MyGatekeeperServer::RouteMap::IsMatch(const PString & alias) const
{
  PINDEX start;
  return regex.Execute(alias, start);
}


///////////////////////////////////////////////////////////////

MyGatekeeperCall::MyGatekeeperCall(MyGatekeeperServer & gk,
                                   const OpalGloballyUniqueID & id,
                                   Direction dir)
  : H323GatekeeperCall(gk, id, dir)
{
#ifdef H323_TRANSNEXUS_OSP
  ospTransaction = NULL;
#endif
}

#ifdef H323_TRANSNEXUS_OSP
static bool GetE164Alias(const H225_ArrayOf_AliasAddress & aliases, H225_AliasAddress & destAlias)
{
  PINDEX i;
  for (i = 0; i < aliases.GetSize(); ++i) {
    if (aliases[i].GetTag() == H225_AliasAddress::e_dialedDigits) {
      destAlias = aliases[i];
      return true;
    }
  }
  return false;
}

PBoolean MyGatekeeperCall::AuthoriseOSPCall(H323GatekeeperARQ & info)
{
  int result;

  // if making call, authorise the call and insert the token
  if (!info.arq.m_answerCall) {

    OpalOSP::Transaction::AuthorisationInfo authInfo;

    // get the source call signalling address
    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress))
      authInfo.ospvSource = OpalOSP::TransportAddressToOSPString(info.arq.m_srcCallSignalAddress);
    else
      authInfo.ospvSource = OpalOSP::TransportAddressToOSPString(info.endpoint->GetSignalAddress(0));

    // get the source number
    if (!GetE164Alias(info.arq.m_srcInfo, authInfo.callingNumber)) {
      PTRACE(1, "OSP\tNo E164 source address in ARQ");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      return false;
    }

    // get the dest number
    if (!info.arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {
      PTRACE(1, "OSP\tNo dest aliases in ARQ");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      return false;
    }
    if (!GetE164Alias(info.arq.m_destinationInfo, authInfo.calledNumber)) {
      PTRACE(1, "OSP\tNo E164 source address in ARQ");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      return false;
    }

    // get the call ID
    authInfo.callID           = this->GetCallIdentifier();

    // authorise the call
    unsigned numberOfDestinations = 1;
    if ((result = ospTransaction->Authorise(authInfo, numberOfDestinations)) != 0) {
      PTRACE(1, "OSP\tCannot authorise ARQ - result = " << result);
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      return false;
    } 
    if (numberOfDestinations == 0) {
      PTRACE(1, "OSP\tNo destinations available");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_noRouteToDestination);
      return false;
    }

    // get the destination
    OpalOSP::Transaction::DestinationInfo destInfo;
    if ((result = ospTransaction->GetFirstDestination(destInfo)) != 0) {
      PTRACE(1, "OSP\tCannot get first destination - result = " << result);
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_undefinedReason);
      return false;
    } 

    // insert destination into the ACF
    if (!destInfo.Insert(info.acf)) {
      PTRACE(1, "OSP\tCannot insert information info ACF");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_undefinedReason);
      return false;
    }

    PTRACE(4, "OSP routed call to " << destInfo.calledNumber << "@" << destInfo.destinationAddress);
    return true;
  }

  // if answering call, validate the token
  OpalOSP::Transaction::ValidationInfo valInfo;

  // get the token
  if (!info.arq.HasOptionalField(H225_AdmissionRequest::e_tokens) || 
     !valInfo.ExtractToken(info.arq.m_tokens)) {
    PTRACE(1, "OSP\tNo tokens in in ARQ");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_invalidPermission);
    return false;
  }

  // get the source call signalling address
  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress))
    valInfo.ospvSource = OpalOSP::TransportAddressToOSPString(info.arq.m_srcCallSignalAddress);
  else
    valInfo.ospvSource = OpalOSP::TransportAddressToOSPString(info.endpoint->GetSignalAddress(0));

  // get the dest call signalling address
  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress))
    valInfo.ospvDest = OpalOSP::TransportAddressToOSPString(info.arq.m_destCallSignalAddress);
  else
    valInfo.ospvDest = OpalOSP::TransportAddressToOSPString(info.endpoint->GetSignalAddress(0));

  // get the source number
  if (!GetE164Alias(info.arq.m_srcInfo, valInfo.callingNumber)) {
    PTRACE(1, "OSP\tNo E164 source address in ARQ");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
    return false;
  }

  // get the dest number
  if (!info.arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {
    PTRACE(1, "OSP\tNo dest aliases in ARQ");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
    return false;
  }
  if (!GetE164Alias(info.arq.m_destinationInfo, valInfo.calledNumber)) {
    PTRACE(1, "OSP\tNo E164 source address in ARQ");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
    return false;
  }

  // get the call ID
  valInfo.callID = this->GetCallIdentifier();

  // validate the token
  bool authorised;
  unsigned timeLimit;
  if ((result = ospTransaction->Validate(valInfo, authorised, timeLimit)) != 0) {
    PTRACE(1, "OSP\tCannot validate ARQ - result = " << result);
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_invalidPermission);
    return false;
  }

  if (!authorised) {
    PTRACE(1, "OSP\tCall not authorised");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_requestDenied);
    return false;
  }

  PTRACE(4, "OSP authorised call with time limit of " << timeLimit);
  return true;
}

#endif

H323GatekeeperRequest::Response MyGatekeeperCall::OnAdmission(H323GatekeeperARQ & info)
{
#ifdef TEST_TOKEN
  info.acf.IncludeOptionalField(H225_AdmissionConfirm::e_tokens);
  info.acf.m_tokens.SetSize(1);
  info.acf.m_tokens[0].m_tokenOID = "1.2.36.76840296.1";
  info.acf.m_tokens[0].IncludeOptionalField(H235_ClearToken::e_nonStandard);
  info.acf.m_tokens[0].m_nonStandard.m_nonStandardIdentifier = "1.2.36.76840296.1.1";
  info.acf.m_tokens[0].m_nonStandard.m_data = "SnfYt0jUuZ4lVQv8umRYaH2JltXDRW6IuYcnASVU";
#endif

#ifdef H323_TRANSNEXUS_OSP
  OpalOSP::Provider * ospProvider = ((MyGatekeeperServer &)gatekeeper).GetOSPProvider();
  if (ospProvider != NULL) {
    // need to add RIP as clearing house is involved
    if (info.IsFastResponseRequired())
      return H323GatekeeperRequest::InProgress(30000);

    ospTransaction = new OpalOSP::Transaction();
    int result;
    if ((result = ospTransaction->Open(*ospProvider)) != 0) {
      delete ospTransaction;
      ospTransaction = NULL;
      PTRACE(1, "OSP\tCannot open transaction - result = " << result);
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_exceedsCallCapacity);
      return H323GatekeeperRequest::Reject;
    }

    if (!AuthoriseOSPCall(info)) {
      delete ospTransaction;
      ospTransaction = NULL;
      return H323GatekeeperRequest::Reject;
    }

    return H323GatekeeperRequest::Confirm;
#endif

  return H323GatekeeperCall::OnAdmission(info);
}


MyGatekeeperCall::~MyGatekeeperCall()
{
#ifdef H323_TRANSNEXUS_OSP
  if (ospTransaction != NULL) {
    ospTransaction->SetEndReason(callEndReason);
    PTimeInterval duration;
    if (connectedTime.GetTimeInSeconds() != 0 && callEndTime.GetTimeInSeconds() != 0)
      duration = callEndTime - connectedTime;
    ospTransaction->CallEnd(callEndTime.GetTimeInSeconds());
    delete ospTransaction;
  }
#endif
}

#endif // OPAL_H323


// End of File ///////////////////////////////////////////////////////////////
