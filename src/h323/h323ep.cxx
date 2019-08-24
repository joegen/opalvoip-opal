/*
 * h323ep.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *
 */

#include <ptlib.h>

#include <opal_config.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h323ep.h"
#endif

#include <h323/h323ep.h>

#include <ptclib/random.h>
#include <opal/call.h>
#include <h323/h323pdu.h>
#include <h323/gkclient.h>
#include <h323/h323rtp.h>
#include <ptclib/url.h>
#include <ptclib/enum.h>
#include <ptclib/pils.h>
#include <opal.h>


#define new PNEW

const OpalProductInfo & H323EndPoint::AvayaPhone()
{
	static OpalProductInfo instance(NULL, "IP_Phone", NULL, 181, 19540, "2.16.840.1.113778.4.2");
	return instance;
}



/////////////////////////////////////////////////////////////////////////////

H323EndPoint::H323EndPoint(OpalManager & manager)
  : OpalRTPEndPoint(manager, OPAL_PREFIX_H323, IsNetworkEndPoint | SupportsE164)
  , autoCallForward(true)
  , disableFastStart(false)
  , disableH245Tunneling(false)
  , disableH245inSetup(false)
  , m_forceSymmetricTCS(false)
  , m_bH245Disabled(false)
  , canDisplayAmountString(false)
  , canEnforceDurationLimit(true)
#if OPAL_H450
  , callIntrusionProtectionLevel(3) //H45011_CIProtectionLevel::e_fullProtection;
#endif
  , terminalType(e_TerminalOnly)
#if OPAL_H239
  , m_defaultH239Control(false)
#endif
  , clearCallOnRoundTripFail(false)
  , signallingChannelCallTimeout(0, 0, 1)  // Minutes
  , firstSignalPduTimeout(0, 2)            // Seconds
  , endSessionTimeout(0, 10)               // Seconds
  , masterSlaveDeterminationTimeout(0, 30) // Seconds
  , masterSlaveDeterminationRetries(10)
  , capabilityExchangeTimeout(0, 30)       // Seconds
  , logicalChannelTimeout(0, 30)           // Seconds
  , requestModeTimeout(0, 30)              // Seconds
  , roundTripDelayTimeout(0, 10)           // Seconds
  , roundTripDelayRate(0, 0, 1)            // Minutes
  , gatekeeperRequestTimeout(0, 5)         // Seconds
  , gatekeeperRequestRetries(3)
  , rasRequestTimeout(0, 3)                // Seconds
  , rasRequestRetries(2)
  , registrationTimeToLive(0, 0, 10)       // Minutes
  , m_sendGRQ(true)
  , m_oneSignalAddressInRRQ(true)
  , m_nextGatekeeperDiscovery(0)
  , callTransferT1(0,10)                   // Seconds
  , callTransferT2(0,10)                   // Seconds
  , callTransferT3(0,10)                   // Seconds
  , callTransferT4(0,10)                   // Seconds
  , callIntrusionT1(0,30)                  // Seconds
  , callIntrusionT2(0,30)                  // Seconds
  , callIntrusionT3(0,30)                  // Seconds
  , callIntrusionT4(0,30)                  // Seconds
  , callIntrusionT5(0,10)                  // Seconds
  , callIntrusionT6(0,10)                  // Seconds
  , m_gatekeeperAliasLimit(MaxGatekeeperAliasLimit)
  , m_gatekeeperSimulatePattern(false)
  , m_gatekeeperRasRedirect(true)
#if OPAL_H450
  , m_nextH450CallIdentity(0)
#endif
#if OPAL_H460
  , m_features(NULL)
#endif
#if OPAL_H460_NAT
  , m_H46019Server(NULL)
#endif
{
  m_localAliasNames[m_defaultLocalPartyName]; // Create entry

  m_connectionsByCallId.DisallowDeleteObjects();
#if OPAL_H450
  m_secondaryConnectionsActive.DisallowDeleteObjects();
#endif
  m_gatekeeperByAlias.DisallowDeleteObjects();

  manager.AttachEndPoint(this, OPAL_PREFIX_H323S);

  SetCompatibility(H323Connection::e_NoMultipleTunnelledH245, "Cisco IOS");
  SetCompatibility(H323Connection::e_BadMasterSlaveConflict,  "NetMeeting|HDX|RealPresence");
  SetCompatibility(H323Connection::e_NoUserInputCapability,   "AltiServ-ITG");
  SetCompatibility(H323Connection::e_H224MustBeSession3,      "HDX|Tandberg|RealPresence");
  SetCompatibility(H323Connection::e_NeedMSDAfterNonEmptyTCS, "Avaya|Radvision");
  SetCompatibility(H323Connection::e_ForceMaintainConnection, "Multivantage");

  m_capabilities.AddAllCapabilities(0, 0, "*");
  H323_UserInputCapability::AddAllCapabilities(m_capabilities, P_MAX_INDEX, P_MAX_INDEX);
#if OPAL_H239
  m_capabilities.Add(new H323H239VideoCapability(OpalMediaFormat()));
  m_capabilities.Add(new H323H239ControlCapability());
#endif
#if OPAL_H235_6
  m_capabilities.Add(new H235SecurityAlgorithmCapability(m_capabilities[0])); // Just a template for cloning
#endif
#if OPAL_H235_8
  m_capabilities.Add(new H235SecurityGenericCapability(m_capabilities[0])); // Just a template for cloning
#endif

  PTRACE(4, "H323\tCreated endpoint.");
}


H323EndPoint::~H323EndPoint()
{
#if OPAL_H460
  delete m_features;
#endif
}


void H323EndPoint::ShutDown()
{
  m_reusableTransportMutex.Wait();
  set<OpalTransportPtr> reusedTransports = m_reusableTransports;
  m_reusableTransports.clear();
  m_reusableTransportMutex.Signal();

  PTRACE(4, "H323\tShutting down: " << reusedTransports.size() << " maintained transports");
  for (set<OpalTransportPtr>::iterator it = reusedTransports.begin(); it != reusedTransports.end(); ++it)
    (*it)->CloseWait();

  /* Unregister request needs/depends OpalEndpoint listeners object, so shut
     down the gatekeeper (if there was one) before cleaning up the OpalEndpoint
     object which kills the listeners. */
  RemoveGatekeeper();

  OpalEndPoint::ShutDown();
}


PBoolean H323EndPoint::GarbageCollection()
{
  m_reusableTransportMutex.Wait();
  for (set<OpalTransportPtr>::iterator it = m_reusableTransports.begin(); it != m_reusableTransports.end(); ) {
    if ((*it)->IsOpen())
      ++it;
    else {
      PTRACE(4, "H323\tRemoving maintained transport " << **it);
      OpalTransportPtr transport = *it;
      m_reusableTransports.erase(it++);
      m_reusableTransportMutex.Signal();
      transport->CloseWait();
      m_reusableTransportMutex.Wait();
    }
  }
  m_reusableTransportMutex.Signal();

  return OpalRTPEndPoint::GarbageCollection();
}


PString H323EndPoint::GetDefaultTransport() const
{
  return OpalTransportAddress::TcpPrefix()
#if OPAL_PTLIB_SSL
            + ',' + OpalTransportAddress::TlsPrefix() + ":1300"
#endif
    ;
}


WORD H323EndPoint::GetDefaultSignalPort() const
{
  return DefaultTcpSignalPort;
}


void H323EndPoint::SetEndpointTypeInfo(H225_EndpointType & info) const
{
  info.IncludeOptionalField(H225_EndpointType::e_vendor);
  SetVendorIdentifierInfo(info.m_vendor);

  switch (terminalType) {
    case e_SimpleEndpointType :
      info.IncludeOptionalField(H225_EndpointType::e_set);
      info.m_set.Set(0); // Audio SET
      info.m_set.Clear(1); // Not conference aware
      break;

    case e_TerminalOnly :
    case e_TerminalAndMC :
      info.IncludeOptionalField(H225_EndpointType::e_terminal);
      break;

    case e_GatewayOnly :
    case e_GatewayAndMC :
    case e_GatewayAndMCWithDataMP :
    case e_GatewayAndMCWithAudioMP :
    case e_GatewayAndMCWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_gateway);
      if (SetGatewaySupportedProtocol(info.m_gateway.m_protocol))
        info.m_gateway.IncludeOptionalField(H225_GatewayInfo::e_protocol);
      break;

    case e_GatekeeperOnly :
    case e_GatekeeperWithDataMP :
    case e_GatekeeperWithAudioMP :
    case e_GatekeeperWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_gatekeeper);
      break;

    case e_MCUOnly :
    case e_MCUWithDataMP :
    case e_MCUWithAudioMP :
    case e_MCUWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_mcu);
      info.m_mc = true;
      if (SetGatewaySupportedProtocol(info.m_mcu.m_protocol))
        info.m_mcu.IncludeOptionalField(H225_McuInfo::e_protocol);
  }
}


void H323EndPoint::SetVendorIdentifierInfo(H225_VendorIdentifier & info) const
{
  SetH221NonStandardInfo(info.m_vendor);

  info.IncludeOptionalField(H225_VendorIdentifier::e_productId);
  info.m_productId = m_productInfo.vendor & m_productInfo.name;
  info.m_productId.SetSize(info.m_productId.GetSize()+2);

  info.IncludeOptionalField(H225_VendorIdentifier::e_versionId);
  info.m_versionId = m_productInfo.version + " (OPAL v" + OpalGetVersion() + ')';
  info.m_versionId.SetSize(info.m_versionId.GetSize()+2);
}


void H323EndPoint::SetH221NonStandardInfo(H225_H221NonStandard & info) const
{
  info.m_t35CountryCode = m_productInfo.t35CountryCode;
  info.m_t35Extension = m_productInfo.t35Extension;
  info.m_manufacturerCode = m_productInfo.manufacturerCode;
}


bool H323EndPoint::SetGatewaySupportedProtocol(H225_ArrayOf_SupportedProtocols & protocols) const
{
  PStringList prefixes;

  if (!OnSetGatewayPrefixes(prefixes))
    return false;

  PINDEX count = protocols.GetSize();
  protocols.SetSize(count+1);

  protocols[count].SetTag(H225_SupportedProtocols::e_h323);
  H225_H323Caps & caps = protocols[count];

  caps.IncludeOptionalField(H225_H323Caps::e_supportedPrefixes);
  H225_ArrayOf_SupportedPrefix & supportedPrefixes = caps.m_supportedPrefixes;  
  supportedPrefixes.SetSize(prefixes.GetSize());

  for (PINDEX i = 0; i < prefixes.GetSize(); i++)
    H323SetAliasAddress(prefixes[i], supportedPrefixes[i].m_prefix);

  return true;
}


bool H323EndPoint::OnSetGatewayPrefixes(PStringList & /*prefixes*/) const
{
  return false;
}


bool H323EndPoint::UseGatekeeper(const PString & address,
                                 const PString & identifier,
                                 const PString & localAddress)
{
  H323Gatekeeper * gatekeeper = GetGatekeeper();
  if (gatekeeper != NULL) {
    bool same = true;

    if (!address.IsEmpty() && address != "*")
      same = gatekeeper->GetTransport().GetRemoteAddress().IsEquivalent(
                OpalTransportAddress(address, H225_RAS::DefaultRasUdpPort, OpalTransportAddress::UdpPrefix()));

    if (!same && !identifier.IsEmpty())
      same = gatekeeper->GetIdentifier() == identifier;

    if (!same && !localAddress.IsEmpty())
      same = gatekeeper->GetTransport().GetLocalAddress().IsEquivalent(
                OpalTransportAddress(localAddress, H225_RAS::DefaultRasUdpPort, OpalTransportAddress::UdpPrefix()));

    if (same) {
      PTRACE(3, "H323\tUsing existing gatekeeper " << *gatekeeper);
      return true;
    }
  }

  if (address.IsEmpty() || address == "*") {
    if (identifier.IsEmpty())
      return DiscoverGatekeeper(localAddress);
    else
      return LocateGatekeeper(identifier, localAddress);
  }
  else {
    if (identifier.IsEmpty())
      return SetGatekeeper(address, localAddress);
    else
      return SetGatekeeperZone(address, identifier, localAddress);
  }
}


static H323TransportAddress GetGatekeeperAddress(const PString & address)
{
#if OPAL_PTLIB_DNS_RESOLVER
  PIPSocketAddressAndPortVector addresses;
  if (PDNS::LookupSRV(address, "_h323rs._udp", H225_RAS::DefaultRasUdpPort, addresses)) {
    PTRACE(4, "H323\tUsing DNS SRV record for gatekeeper at " << addresses[0]);
    return OpalTransportAddress(addresses[0], OpalTransportAddress::UdpPrefix());
  }
#endif //OPAL_PTLIB_DNS_RESOLVER

  return H323TransportAddress(address, H225_RAS::DefaultRasUdpPort, OpalTransportAddress::UdpPrefix());
}


bool H323EndPoint::SetGatekeeper(const PString & remoteAddress, const PString & localAddress)
{
  H323TransportAddress h323addr = GetGatekeeperAddress(remoteAddress);

  PWaitAndSignal mutex(m_gatekeeperMutex);
  if (!InternalStartGatekeeper(h323addr, localAddress))
    return false;

  for (GatekeeperList::iterator it = m_gatekeepers.begin(); it != m_gatekeepers.end(); ++it) {
    if (!it->DiscoverByAddress(h323addr))
      return false;
  }

  return true;
}


bool H323EndPoint::SetGatekeeperZone(const PString & remoteAddress,
                                     const PString & identifier,
                                     const PString & localAddress)
{
  H323TransportAddress h323addr = GetGatekeeperAddress(remoteAddress);

  PWaitAndSignal mutex(m_gatekeeperMutex);
  if (!InternalStartGatekeeper(h323addr, localAddress))
    return false;

  for (GatekeeperList::iterator it = m_gatekeepers.begin(); it != m_gatekeepers.end(); ++it) {
    if (!it->DiscoverByNameAndAddress(identifier, h323addr))
      return false;
  }

  return true;
}


PBoolean H323EndPoint::LocateGatekeeper(const PString & identifier, const PString & localAddress)
{
  PWaitAndSignal mutex(m_gatekeeperMutex);

  if (!InternalStartGatekeeper(H323TransportAddress(), localAddress))
    return false;

  for (GatekeeperList::iterator it = m_gatekeepers.begin(); it != m_gatekeepers.end(); ++it) {
    if (!it->DiscoverByName(identifier))
      return false;
  }

  return true;
}


PBoolean H323EndPoint::DiscoverGatekeeper(const PString & localAddress)
{
  PWaitAndSignal mutex(m_gatekeeperMutex);

  if (!InternalStartGatekeeper(H323TransportAddress(), localAddress))
    return false;

  for (GatekeeperList::iterator it = m_gatekeepers.begin(); it != m_gatekeepers.end(); ++it) {
    if (!it->DiscoverAny())
      return false;
  }

  return true;
}


bool H323EndPoint::InternalStartGatekeeper(const H323TransportAddress & remoteAddress, const PString & localAddress)
{
  RemoveGatekeeper(H225_UnregRequestReason::e_reregistrationRequired);

  m_gatekeeperInterfaces.clear();

  if (localAddress.IsEmpty()) {
    PIPSocket::Address interfaceIP(PIPSocket::GetInvalidAddress());

    // See if the system can tell us which interface would be used
    {
      PIPSocket::Address remoteIP;
      if (remoteAddress.GetIpAddress(remoteIP) && !remoteIP.IsAny()) {
        interfaceIP = PIPSocket::GetRouteInterfaceAddress(remoteIP);
        PTRACE_IF(4, interfaceIP.IsValid(), "H323\tUsing interface " << interfaceIP << " routed from " << remoteIP);
      }
    }

    if (!interfaceIP.IsValid()) {
      OpalTransportAddressArray interfaces = GetInterfaceAddresses();
      for (PINDEX i = 0; i < interfaces.GetSize(); ++i) {
        if (interfaces[i].IsCompatible(remoteAddress)) {
          PTRACE(4, "H323\tInterface " << interfaces[i] << " compatible with " << remoteAddress);
          if (interfaceIP.IsValid())
            interfaces[i].GetIpAddress(interfaceIP);
          else {
            // More than one so do gk on all interfaces
            interfaceIP = PIPSocket::GetDefaultIpAny();
            break;
          }
        }
      }
      if (!interfaceIP.IsValid()) {
        PTRACE(2, "H323\tCannot find a compatible listener for \"" << remoteAddress << '"');
        return false;
      }
    }

    if (m_gatekeeperInterfaces.AppendAddress(interfaceIP))
      return InternalRestartGatekeeper(false);
  }
  else {
    if (m_gatekeeperInterfaces.AppendString(localAddress))
      return InternalRestartGatekeeper(false);
  }

  PTRACE(2, "H323\tNo legal local interface address in \"" << localAddress << '"');
  return false;
}


bool H323EndPoint::InternalRestartGatekeeper(bool adjustingRegistrations)
{
  m_aliasMutex.Wait();
  AliasToGkMap allAliases = m_localAliasNames;
  AliasToGkMap patterns = m_localAliasPatterns;
  m_aliasMutex.Signal();

  PWaitAndSignal mutex(m_gatekeeperMutex);

  if (adjustingRegistrations && m_gatekeepers.IsEmpty())
      return true;

  // Create list of all aliases, including synthesised ones
  if (GetGatekeeperSimulatePattern()) {
    for (AliasToGkMap::iterator it = patterns.begin(); it != patterns.end(); ++it) {
      PString start, end;
      PINDEX fixedDigits = ParseAliasPatternRange(it->first, start, end);
      while (start != end) {
        allAliases[start] = it->second;
        for (PINDEX i = start.GetLength() - 1; i >= fixedDigits; --i) {
          if (start[i] < '9') {
            ++start[i];
            break;
          }
          start[i] = '0';
        }
      }
      allAliases[end] = it->second;
    }
  }

  GatekeeperList::iterator gk;

  // Now remove aliases already registered but no longer needed
  for (gk = m_gatekeepers.begin(); gk != m_gatekeepers.end(); ++gk) {
    gk->m_aliasMutex.Wait();
    bool deletedOne = false;
    for (PStringList::iterator alias = gk->m_aliases.begin(); alias != gk->m_aliases.end(); ) {
      AliasToGkMap::iterator existing = allAliases.find(*alias);
      if (existing != allAliases.end()) {
        allAliases.erase(existing); // Remove from set as do not need to register it again
        ++alias;
      }
      else {
        gk->m_aliases.erase(alias++);
        deletedOne = true;
      }
    }
    if (deletedOne && !gk->m_aliases.IsEmpty())
      gk->ReRegisterNow();
    gk->m_aliasMutex.Signal();
  }

  // Now add aliases not registered, filling partially filled registrations
  for (gk = m_gatekeepers.begin(); !allAliases.empty() && gk != m_gatekeepers.end(); ++gk) {
    gk->m_aliasMutex.Wait();
    bool addedOne = false;
    while (gk->m_aliases.GetSize() < m_gatekeeperAliasLimit) {
      AliasToGkMap::iterator alias;
      for (alias = allAliases.begin(); alias != allAliases.end(); ++alias) {
        if (alias->second.IsEmpty() || alias->second.IsEquivalent(gk->GetTransport().GetRemoteAddress()))
          break;
      }
      if (alias == allAliases.end())
        break;
      gk->m_aliases += alias->first.GetPointer(); // Do not wan't reference
      allAliases.erase(allAliases.begin());
      addedOne = true;
    }
    if (addedOne)
      gk->ReRegisterNow();
    gk->m_aliasMutex.Signal();
  }

  // If any sub-gatekeepers now have no aliases, remove them
  gk = m_gatekeepers.begin();
  while (gk != m_gatekeepers.end()) {
    if (gk->m_aliases.IsEmpty()) {
      gk->UnregistrationRequest(-1);
      m_gatekeepers.erase(gk++);
    }
    else
      ++gk;
  }

  // Determine default gatekeeper address, if have one.
  OpalTransportAddress remoteAddress;
  gk = m_gatekeepers.begin();
  if (gk != m_gatekeepers.end())
    remoteAddress = gk->GetTransport().GetRemoteAddress();

  // Rotate allaliases so grouped by gk address
  map<OpalTransportAddress, PStringList> rotatedAliases;
  for (AliasToGkMap::iterator alias = allAliases.begin(); alias != allAliases.end(); ++alias)
    rotatedAliases[adjustingRegistrations && !alias->second.IsEmpty() ? alias->second : remoteAddress].AppendString(alias->first);

  PTRACE(4, "Creating new gatekeepers for " << allAliases.size() << " aliases at " << rotatedAliases.size() << " remotes address(es).");

  // Now add remaining aliases, creating new registrations as required.
  PStringList aliasSubset;
  for (map<OpalTransportAddress, PStringList>::iterator it = rotatedAliases.begin(); it != rotatedAliases.end(); ++it) {
    for (PStringList::iterator alias = it->second.begin(); alias != it->second.end(); ++alias) {
      aliasSubset += *alias;
      if (aliasSubset.GetSize() >= m_gatekeeperAliasLimit) {
        if (!InternalCreateGatekeeper(it->first, aliasSubset))
          return false;
        aliasSubset = PStringList(); /// Don't use RemoveAll() which does it for all references
      }
    }
    if (!aliasSubset.IsEmpty() && !InternalCreateGatekeeper(it->first, aliasSubset))
      return false;
  }

  if (m_gatekeepers.IsEmpty())
    return false;

  m_gatekeeperByAlias.RemoveAll();
  for (gk = m_gatekeepers.begin(); gk != m_gatekeepers.end(); ++gk) {
    gk->m_aliasMutex.Wait();
    for (PStringList::iterator alias = gk->m_aliases.begin(); alias != gk->m_aliases.end(); ++alias)
      m_gatekeeperByAlias.SetAt(alias->GetPointer(), &*gk);
    gk->m_aliasMutex.Signal();
  }

  return true;
}


bool H323EndPoint::InternalCreateGatekeeper(const H323TransportAddress & remoteAddress, const PStringList & aliases)
{
  PIPAddressAndPort ap;
  m_gatekeeperInterfaces.GetIpAndPort(m_gatekeepers.GetSize(), ap);

  OpalTransportUDP * transport = new OpalTransportUDP(*this, ap.GetAddress(), ap.GetPort(), false, true);
  if (!transport->IsOpen()) {
    PTRACE(2, "H323\tCannot bind to local interface address.");
    return false;
  }

  H323Gatekeeper * gatekeeper = CreateGatekeeper(transport);
  if (gatekeeper == NULL)
    return false;

  PTRACE(3, "H323\tAdded gatekeeper (at=" << remoteAddress << ", if=" << transport->GetLocalAddress() << ") for aliases: " << setfill(',') << aliases);

  gatekeeper->m_aliasMutex.Wait();
  gatekeeper->m_aliases.clear();
  for (PStringList::const_iterator alias = aliases.begin(); alias != aliases.end(); ++alias)
    gatekeeper->m_aliases += alias->GetPointer(); // Don't make reference
  gatekeeper->m_aliasMutex.Signal();

  InternalSetGatekeeperPassword(*gatekeeper, remoteAddress);

  m_gatekeepers.Append(gatekeeper);

  if (remoteAddress.IsEmpty())
    return true;

  return gatekeeper->DiscoverByAddress(remoteAddress);
}


H323Gatekeeper * H323EndPoint::CreateGatekeeper(H323Transport * transport)
{
  return new H323Gatekeeper(*this, transport);
}


H323Gatekeeper * H323EndPoint::GetGatekeeper(const PString & alias) const
{
  PWaitAndSignal lock(m_gatekeeperMutex);

  if (m_gatekeepers.IsEmpty())
    return NULL;

  H323Gatekeeper * gk = alias.IsEmpty() ? NULL : m_gatekeeperByAlias.GetAt(alias);
  return gk != NULL ? gk : &m_gatekeepers[PRandom::Number(m_gatekeepers.GetSize()-1)];
}


PBoolean H323EndPoint::IsRegisteredWithGatekeeper(bool all) const
{
  PWaitAndSignal mutex(m_gatekeeperMutex);

  for (GatekeeperList::const_iterator gk = m_gatekeepers.begin(); gk != m_gatekeepers.end(); ++gk) {
    if (gk->IsRegistered()) {
      if (!all)
        return true;
    }
    else {
      if (all)
        return false;
    }
  }

  return all;
}


PBoolean H323EndPoint::RemoveGatekeeper(int reason)
{
  PTRACE(3, "H323\tRemoving gatekeeper");

  m_gatekeeperMutex.Wait();
  GatekeeperList removedGatekeepers = m_gatekeepers;
  m_gatekeepers = GatekeeperList(); // Don't use RemoveAll, this just breaks the reference
  m_gatekeeperByAlias.RemoveAll();
  m_gatekeeperMutex.Signal();

  bool ok = true;

  for (GatekeeperList::iterator it = removedGatekeepers.begin(); it != removedGatekeepers.end(); ++it) {
    if (it->IsRegistered()) { // If we are registered send a URQ
      ClearAllCalls();
      if (!it->UnregistrationRequest(reason))
        ok = false;
    }
  }

  return ok;
}


void H323EndPoint::SetGatekeeperPassword(const PString & password, const PString & username)
{
  PWaitAndSignal mutex(m_gatekeeperMutex);

  m_gatekeeperUsername = username;
  m_gatekeeperPassword = password;

  for (GatekeeperList::iterator it = m_gatekeepers.begin(); it != m_gatekeepers.end(); ++it)
    InternalSetGatekeeperPassword(*it, it->m_transport->GetRemoteAddress());
}


void H323EndPoint::SetAliasPasswords(const PStringToString & aliasPasswords, const PString & defaultAddress)
{
  m_aliasPasswords = aliasPasswords;
  m_aliasPwdDefaultAddress = defaultAddress;
}


void H323EndPoint::InternalSetGatekeeperPassword(H323Gatekeeper& gatekeeper, const OpalTransportAddress& gatekeeperAddress) const
{
  PString alias;
  gatekeeper.m_aliasMutex.Wait();
  if (gatekeeper.m_aliases.size() == 1)
    alias = gatekeeper.m_aliases[0];
  gatekeeper.m_aliasMutex.Signal();

  if (!alias.IsEmpty()) {
    PIPSocket::Address ip;
    PString gkAddress = (gatekeeperAddress.GetIpAddress(ip) && !ip.IsAny()) ? ip.AsString() : m_aliasPwdDefaultAddress;
    PStringToString::const_iterator pwdIter = m_aliasPasswords.find(alias + '@' + gkAddress);

    if (pwdIter == m_aliasPasswords.end())
      pwdIter = m_aliasPasswords.find(alias);

    if (pwdIter != m_aliasPasswords.end()) {
      gatekeeper.SetPassword(pwdIter->second, GetGatekeeperUsername());
      return;
    }
  }

  gatekeeper.SetPassword(GetGatekeeperPassword(), GetGatekeeperUsername());
}


void H323EndPoint::SetGatekeeperAliasLimit(PINDEX limit)
{
  PWaitAndSignal mutex(m_gatekeeperMutex);

  if (m_gatekeeperAliasLimit == limit)
    return;

  m_gatekeeperAliasLimit = limit;
  InternalRestartGatekeeper();
}


void H323EndPoint::OnGatekeeperStatus(H323Gatekeeper &, H323Gatekeeper::RegistrationFailReasons reason)
{
  OnGatekeeperStatus(reason);
}


void H323EndPoint::OnGatekeeperStatus(H323Gatekeeper::RegistrationFailReasons)
{
}


H235Authenticators H323EndPoint::CreateAuthenticators()
{
  H235Authenticators authenticators;

  PFactory<H235Authenticator>::KeyList_T keyList = PFactory<H235Authenticator>::GetKeyList();
  for (PFactory<H235Authenticator>::KeyList_T::const_iterator it = keyList.begin(); it != keyList.end(); ++it) {
    H235Authenticator * auth = PFactory<H235Authenticator>::CreateInstance(*it);
    if (auth->GetApplication() == H235Authenticator::GKAdmission || auth->GetApplication() == H235Authenticator::AnyApplication)
      authenticators.Append(auth);
    else
      delete auth;
  }

  return authenticators;
}


PSafePtr<OpalConnection> H323EndPoint::MakeConnection(OpalCall & call,
                                                 const PString & remoteParty,
                                                          void * userData,
                                                    unsigned int options,
                                 OpalConnection::StringOptions * stringOptions)
{
  if (m_listeners.IsEmpty())
    return NULL;

  PTRACE(3, "H323\tMaking call to: " << remoteParty);
  return InternalMakeCall(call,
                          PString::Empty(),
                          PString::Empty(),
                          UINT_MAX,
                          remoteParty,
                          userData,
                          options,
                          stringOptions);
}


PStringList H323EndPoint::GetAvailableStringOptions() const
{
  static char const * const StringOpts[] = {
    OPAL_OPT_Q931_BEARER_CAPS,
    OPAL_OPT_Q931_BEARER_BANDWIDTH
  };

  PStringList list = OpalRTPEndPoint::GetAvailableStringOptions();
  list += PStringList(PARRAYSIZE(StringOpts), StringOpts, true);
  return list;
}


void H323EndPoint::OnReleased(OpalConnection & connection)
{
  m_connectionsByCallId.RemoveAt(connection.GetIdentifier());

  OpalTransportPtr signallingChannel = dynamic_cast<H323Connection &>(connection).GetSignallingChannel();
  if (signallingChannel != NULL && signallingChannel->IsOpen()) {
    PTRACE(3, "H323", "Maintaining TCP connection: " << *signallingChannel);
    m_reusableTransportMutex.Wait();
    m_reusableTransports.insert(signallingChannel);
    m_reusableTransportMutex.Signal();
    signallingChannel->AttachThread(new PThreadObj2Arg<H323EndPoint, OpalTransportPtr, bool>(*this,
                signallingChannel, true, &H323EndPoint::InternalNewIncomingConnection, false, "H225 Maintain"));
  }

  OpalRTPEndPoint::OnReleased(connection);
}


void H323EndPoint::NewIncomingConnection(OpalListener &, const OpalTransportPtr & transport)
{
  if (transport != NULL)
    InternalNewIncomingConnection(transport);
}


void H323EndPoint::InternalNewIncomingConnection(OpalTransportPtr transport, bool reused)
{
  if (transport == NULL)
    return;

  PTRACE(4, "H225\tAwaiting first PDU on " << (reused ? "reused" : "initial") << " connection " << *transport);
  transport->SetReadTimeout(GetFirstSignalPduTimeout());

  H323SignalPDU pdu;
  do {
    if (!pdu.Read(*transport)) {
      if (reused) {
        PTRACE(3, "H225\tReusable TCP connection not reused.");
        transport->Close();
        return;
      }

      PTRACE(2, "H225\tFailed to get initial Q.931 PDU, connection not started.");
      return;
    }
  } while (pdu.GetQ931().GetMessageType() != Q931::SetupMsg);

  unsigned callReference = pdu.GetQ931().GetCallReference();
  PTRACE(3, "H225\tIncoming call, first PDU: callReference=" << callReference
         << " on " << (reused ? "reused" : "initial") << " connection " << *transport);

  // Get a new (or old) connection from the endpoint, calculate token
  PString token = transport->GetRemoteAddress();
  token.sprintf("/%u", callReference);

  PSafePtr<H323Connection> connection = PSafePtrCast<OpalConnection, H323Connection>(GetConnectionWithLock(token, PSafeReadWrite));
  if (connection == NULL) {
    // Get new instance of a call, abort if none created
    OpalCall * call = m_manager.InternalCreateCall();
    if (call != NULL) {
      PTRACE_CONTEXT_ID_SET(*PThread::Current(), call);
      connection = CreateConnection(*call, token, NULL, *transport, PString::Empty(), PString::Empty(), &pdu);
      PTRACE(3, "H323\tCreated new connection: " << token);
    }
  }
  else {
    PTRACE_CONTEXT_ID_SET(*PThread::Current(), connection);
  }

  // Make sure transport is attached before AddConnection()
  if (connection != NULL) {
    m_reusableTransportMutex.Wait();
    m_reusableTransports.erase(transport);
    m_reusableTransportMutex.Signal();
    connection->AttachSignalChannel(token, transport, true);
  }

  if (AddConnection(connection) != NULL && connection->HandleSignalPDU(pdu)) {
    m_connectionsByCallId.SetAt(connection->GetIdentifier(), connection);
    // All subsequent PDU's should wait forever
    transport->SetReadTimeout(PMaxTimeInterval);
    connection->HandleSignallingChannel();
    return;
  }

  PTRACE(1, "H225\tEndpoint could not create connection, "
            "sending release complete PDU: callRef=" << callReference);

  H323SignalPDU releaseComplete;
  Q931 &q931PDU = releaseComplete.GetQ931();
  q931PDU.BuildReleaseComplete(callReference, true);
  releaseComplete.m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_releaseComplete);

  H225_ReleaseComplete_UUIE &release = releaseComplete.m_h323_uu_pdu.m_h323_message_body;
  release.m_protocolIdentifier.SetValue(psprintf("0.0.8.2250.0.%u", H225_PROTOCOL_VERSION));

  H225_Setup_UUIE &setup = pdu.m_h323_uu_pdu.m_h323_message_body;
  if (setup.HasOptionalField(H225_Setup_UUIE::e_callIdentifier)) {
    release.IncludeOptionalField(H225_Setup_UUIE::e_callIdentifier);
    release.m_callIdentifier = setup.m_callIdentifier;
  }

  // Set the cause value
  q931PDU.SetCause(Q931::TemporaryFailure);

  // Send the PDU
  releaseComplete.Write(*transport);
}


H323Connection * H323EndPoint::CreateConnection(OpalCall & call,
                                                const PString & token,
                                                void * /*userData*/,
                                                OpalTransport & /*transport*/,
                                                const PString & alias,
                                                const H323TransportAddress & address,
                                                H323SignalPDU * /*setupPDU*/,
                                                unsigned options,
                                                OpalConnection::StringOptions * stringOptions)
{
  return new H323Connection(call, *this, token, alias, address, options, stringOptions);
}


PBoolean H323EndPoint::SetupTransfer(const PString & oldToken,
                                 const PString & callIdentity,
                                 const PString & remoteParty,
                                 void * userData)
{
  // Make a new connection
  PSafePtr<OpalConnection> otherConnection = GetConnectionWithLock(oldToken, PSafeReference);
  if (otherConnection == NULL)
    return false;

  OpalCall & call = otherConnection->GetCall();

  call.CloseMediaStreams();

  PTRACE(3, "H323\tTransferring call to: " << remoteParty);
  PBoolean ok = InternalMakeCall(call, oldToken, callIdentity, UINT_MAX, remoteParty, userData) != NULL;
  call.OnReleased(*otherConnection);
  otherConnection->Release(OpalConnection::EndedByCallForwarded);

  return ok;
}


H323Connection * H323EndPoint::InternalMakeCall(OpalCall & call,
                                           const PString & existingToken,
#if OPAL_H450
                                           const PString & callIdentity,
                                                  unsigned capabilityLevel,
#else
                                           const PString & ,
                                                  unsigned ,
#endif
                                           const PString & remoteParty,
                                                    void * userData,
                                              unsigned int options,
                           OpalConnection::StringOptions * stringOptions)
{
  OpalConnection::StringOptions localStringOptions;
  if (stringOptions == NULL)
    stringOptions = &localStringOptions;

  PString alias;
  H323TransportAddress address;
  if (!ParsePartyName(remoteParty, alias, address, stringOptions)) {
    PTRACE(2, "H323\tCould not parse \"" << remoteParty << '"');
    return NULL;
  }

  // Restriction: the call must be made on the same local interface as the one
  // that the gatekeeper is using.
  H323Transport * transport;
  H323Gatekeeper * gatekeeper = GetGatekeeper(stringOptions->GetString(OPAL_OPT_CALLING_PARTY_NUMBER,
                                              stringOptions->GetString(OPAL_OPT_CALLING_PARTY_NAME)));
  OpalTransportAddress localInterface = stringOptions->GetString(OPAL_OPT_INTERFACE,
                          gatekeeper != NULL ? gatekeeper->GetTransport().GetInterface() : PString::Empty());
  if (localInterface.IsEmpty())
    transport = address.CreateTransport(*this, OpalTransportAddress::NoBinding);
  else
    transport = localInterface.CreateTransport(*this, OpalTransportAddress::HostOnly);

  if (transport == NULL) {
    PTRACE(1, "H323\tInvalid transport in \"" << remoteParty << '"');
    return NULL;
  }

  PString newToken(PString::Printf, "localhost/%u", Q931::GenerateCallReference());

  H323Connection * connection = CreateConnection(call, newToken, userData, *transport, alias, address, NULL, options, stringOptions);
  if (!AddConnection(connection)) {
    PTRACE(1, "H225\tEndpoint could not create connection, aborting setup.");
    return NULL;
  }

  connection->AttachSignalChannel(newToken, transport, false);

#if OPAL_H450
  if (!callIdentity.IsEmpty()) {
    if (capabilityLevel == UINT_MAX)
      connection->HandleTransferCall(existingToken, callIdentity);
    else {
      connection->HandleIntrudeCall(existingToken, callIdentity);
      connection->IntrudeCall(capabilityLevel);
    }
  }
#endif

  PTRACE(3, "H323\tCreated new connection: " << newToken);

  // See if we are starting an outgoing connection as first in a call
  if (call.GetConnection(0) == (OpalConnection*)connection || !existingToken.IsEmpty())
    connection->SetUpConnection();
  m_connectionsByCallId.SetAt(connection->GetIdentifier(), connection);

  return connection;
}


#if OPAL_H450

void H323EndPoint::TransferCall(const PString & token, 
                                const PString & remoteParty,
                                const PString & callIdentity)
{
  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);
  if (connection != NULL)
    connection->TransferCall(remoteParty, callIdentity);
}


void H323EndPoint::ConsultationTransfer(const PString & primaryCallToken,   
                                        const PString & secondaryCallToken)
{
  PSafePtr<H323Connection> secondaryCall = FindConnectionWithLock(secondaryCallToken);
  if (secondaryCall != NULL)
    secondaryCall->ConsultationTransfer(primaryCallToken);
}


PBoolean H323EndPoint::IntrudeCall(const PString & remoteParty,
                               unsigned capabilityLevel,
                               void * userData)
{
  // Get new instance of a call, abort if none created
  OpalCall * call = m_manager.InternalCreateCall();
  if (call == NULL)
    return false;

  return InternalMakeCall(*call, PString::Empty(), PString::Empty(), capabilityLevel, remoteParty, userData) != NULL;
}

#endif

PBoolean H323EndPoint::OnSendConnect(H323Connection & /*connection*/,
                                 H323SignalPDU & /*connectPDU*/
                                )
{
  return true;
}

PBoolean H323EndPoint::OnSendSignalSetup(H323Connection & /*connection*/,
                                     H323SignalPDU & /*setupPDU*/)
{
  return true;
}

PBoolean H323EndPoint::OnSendCallProceeding(H323Connection & /*connection*/,
                                        H323SignalPDU & /*callProceedingPDU*/)
{
  return true;
}

void H323EndPoint::OnReceivedInitiateReturnError()
{
}

PBoolean H323EndPoint::ParsePartyName(const PString & remoteParty,
                                            PString & alias,
                               H323TransportAddress & address,
                       OpalConnection::StringOptions * stringOptions)
{
  PURL url(remoteParty, GetPrefixName()); // Parses as per RFC3508

  if (stringOptions != NULL)
    stringOptions->ExtractFromURL(url);

#if OPAL_PTLIB_DNS_RESOLVER

  // if there is no gatekeeper, try altarnate address lookup methods
  if (m_gatekeepers.IsEmpty()) {
    if (url.GetHostName().IsEmpty()) {
      // No host, so lets try ENUM on the username part, and only has digits and +
      PString username = url.GetUserName();
      if (OpalIsE164(username)) {
        PString newName;
        if (PDNS::ENUMLookup(username, "E2U+h323", newName)) {
          PTRACE(4, "H323\tENUM converted remote party " << username << " to " << newName);
          url.Parse(newName, GetPrefixName());
        }
      }
    }

    /* URL default for H.323 is if no @ then is username, but that does
       not make sense when no GK, so we switch to being a host only */
    if (url.GetHostName().IsEmpty()) {
      if (url.GetScheme() == "tel") {
        PTRACE(2, "H323\tCannot use tel URI without phone-context or active gatekeeper");
        return false;
      }
      url.SetHostName(url.GetUserName());
      url.SetUserName(PString::Empty());
    }

    // If it is a valid IP address then can't be a domain so do not try SRV record lookup
    PIPSocketAddressAndPortVector addresses;
    if (PDNS::LookupSRV(url.GetHostName(), "_h323cs._tcp", url.GetPort(), addresses) && !addresses.empty()) {
      // Only use first entry
      url.SetHostName(addresses[0].GetAddress().AsString());
      url.SetPort(addresses[0].GetPort());
    }
  }

#endif // OPAL_PTLIB_DNS_RESOLVER

  address = OpalTransportAddress(url.GetHostName(), url.GetPort(),
#if OPAL_PTLIB_SSL
                                 url.GetScheme() == "h323s" ? OpalTransportAddress::TlsPrefix() :
#endif
                                 OpalTransportAddress::TcpPrefix());
  alias = url.GetUserName();
  if (alias.IsEmpty() && address.IsEmpty()) {
    PTRACE(1, "H323\tAttempt to use invalid URL \"" << remoteParty << '"');
    return false;
  }

  PBoolean gatekeeperSpecified = false;
  PBoolean gatewaySpecified = false;

  PCaselessString type = url.GetParamVars()("type");

  if (url.GetScheme() == "callto") {
    // Do not yet support ILS
    if (type == "directory") {
#if OPAL_PTLIB_LDAP
      PString server = url.GetHostName();
      if (server.IsEmpty())
        server = GetDefaultILSServer();
      if (server.IsEmpty())
        return false;

      PILSSession ils;
      if (!ils.Open(server, url.GetPort())) {
        PTRACE(1, "H323\tCould not open ILS server at \"" << server
               << "\" - " << ils.GetErrorText());
        return false;
      }

      PILSSession::RTPerson person;
      if (!ils.SearchPerson(alias, person)) {
        PTRACE(1, "H323\tCould not find "
               << server << '/' << alias << ": " << ils.GetErrorText());
        return false;
      }

      if (!person.sipAddress.IsValid()) {
        PTRACE(1, "H323\tILS user " << server << '/' << alias
               << " does not have a valid IP address");
        return false;
      }

      // Get the IP address
      address = OpalTransportAddress(person.sipAddress);

      // Get the port if provided
      for (PINDEX i = 0; i < person.sport.GetSize(); i++) {
        if (person.sport[i] != 1503) { // Dont use the T.120 port
          address = H323TransportAddress(person.sipAddress, person.sport[i]);
          break;
        }
      }

      alias = PString::Empty(); // No alias for ILS lookup, only host
      return true;
#else
      return false;
#endif
    }

    if (url.GetParamVars().Contains("gateway"))
      gatewaySpecified = true;
  }
  else if (url.GetScheme() == "h323") {
    if (type == "gw")
      gatewaySpecified = true;
    else if (type == "gk")
      gatekeeperSpecified = true;
    else if (!type.IsEmpty()) {
      PTRACE(1, "H323\tUnsupported host type \"" << type << "\" in h323 URL");
      return false;
    }
  }

  // User explicitly asked fo a look up to a gk
  if (gatekeeperSpecified) {
    if (alias.IsEmpty()) {
      PTRACE(1, "H323\tAttempt to use explict gatekeeper without alias!");
      return false;
    }

    if (address.IsEmpty()) {
      PTRACE(1, "H323\tAttempt to use explict gatekeeper without address!");
      return false;
    }

    H323TransportAddress gkAddr(url.GetHostName(), url.GetPort(), OpalTransportAddress::UdpPrefix());
    PTRACE(3, "H323\tLooking for \"" << alias << "\" on gatekeeper at " << gkAddr);

    H323Gatekeeper * gk = CreateGatekeeper(new H323TransportUDP(*this));

    PBoolean ok = gk->DiscoverByAddress(gkAddr);
    if (ok) {
      ok = gk->LocationRequest(alias, address);
      if (ok) {
        PTRACE(3, "H323\tLocation Request of \"" << alias << "\" on gk " << gkAddr << " found " << address);
      }
      else {
        PTRACE(1, "H323\tLocation Request failed for \"" << alias << "\" on gk " << gkAddr);
      }
    }
    else {
      PTRACE(1, "H323\tLocation Request discovery failed for gk " << gkAddr);
    }

    delete gk;

    return ok;
  }

  // User explicitly said to use a gw, or we do not have a gk to do look up
  if (m_gatekeepers.IsEmpty() || gatewaySpecified) {
    /* If URL did not have an '@' as per RFC3508 we get an alias but no
       address, but user said to use gw, or we do not have a gk to do a lookup
       so we MUST have a host. So, we swap the alias into the address field
       and blank out the alias. */
    if (address.IsEmpty()) {
      PStringArray transports = GetDefaultTransport().Tokenise(',');
      address = H323TransportAddress(alias, GetDefaultSignalPort(), transports[0]);
      alias = PString::Empty();
    }
    return true;
  }

  // We have a gatekeeper and user provided a host
  if (!address.IsEmpty()) {
    // Most H.323 gatekeepers want RFC4282 Network Access Identifier as alias,
    // so we treat the host part of the URL as the "realm" of the NAI
    alias += '@' + url.GetHostName();
    return true;
  }

  // We have a gk and user did not explicitly supply a host, so lets
  // do a check to see if it is an IP address 
  if (alias.FindRegEx("^[0-9][0-9]*\\.[0-9][0-9]*\\.[0-9][0-9]*\\.[0-9][0-9]*") != P_MAX_INDEX) {
    PINDEX colon = alias.Find(':');
    WORD port = GetDefaultSignalPort();
    if (colon != P_MAX_INDEX) {
      port = (WORD)alias.Mid(colon+1).AsInteger();
      alias = alias.Left(colon);
    }
    PIPSocket::Address ip(alias);
    if (ip.IsValid()) {
      alias = PString::Empty();
      address = H323TransportAddress(ip, port);
    }
  }

  return true;
}


PSafePtr<H323Connection> H323EndPoint::FindConnectionWithLock(const PString & token, PSafetyMode mode)
{
  PSafePtr<H323Connection> connection = PSafePtrCast<OpalConnection, H323Connection>(GetConnectionWithLock(token, mode));
  if (connection == NULL)
    connection = m_connectionsByCallId.Find(token, mode);

  return connection;
}


PBoolean H323EndPoint::OnIncomingCall(H323Connection & connection,
                                  const H323SignalPDU & /*setupPDU*/,
                                  H323SignalPDU & /*alertingPDU*/)
{
  return connection.OnIncomingConnection(0, NULL);
}


PBoolean H323EndPoint::OnCallTransferInitiate(H323Connection & /*connection*/,
                                          const PString & /*remoteParty*/)
{
  return true;
}


PBoolean H323EndPoint::OnCallTransferIdentify(H323Connection & /*connection*/)
{
  return true;
}


void H323EndPoint::OnSendARQ(H323Connection & /*conn*/, H225_AdmissionRequest & /*arq*/)
{
}


OpalConnection::AnswerCallResponse
       H323EndPoint::OnAnswerCall(H323Connection & connection,
                                  const PString & caller,
                                  const H323SignalPDU & /*setupPDU*/,
                                  H323SignalPDU & /*connectPDU*/,
                                  H323SignalPDU & /*progressPDU*/)
{
  return connection.OnAnswerCall(caller);
}

OpalConnection::AnswerCallResponse
       H323EndPoint::OnAnswerCall(OpalConnection & connection,
       const PString & caller
)
{
  return OpalEndPoint::OnAnswerCall(connection, caller);
}

PBoolean H323EndPoint::OnAlerting(H323Connection & connection,
                              const H323SignalPDU & /*alertingPDU*/,
                              const PString & /*username*/)
{
  PTRACE(3, "H225\tReceived alerting PDU.");
  ((OpalConnection&)connection).OnAlerting();
  return true;
}

PBoolean H323EndPoint::OnSendAlerting(H323Connection & PTRACE_PARAM(connection),
                                  H323SignalPDU & /*alerting*/,
                                  const PString & /*calleeName*/,   /// Name of endpoint being alerted.
                                  PBoolean /*withMedia*/                /// Open media with alerting
                                  )
{
  PTRACE(3, "H225\tOnSendAlerting conn = " << connection);
  return true;
}

PBoolean H323EndPoint::OnSentAlerting(H323Connection & PTRACE_PARAM(connection))
{
  PTRACE(3, "H225\tOnSentAlerting conn = " << connection);
  return true;
}

PBoolean H323EndPoint::OnConnectionForwarded(H323Connection & /*connection*/,
                                         const PString & /*forwardParty*/,
                                         const H323SignalPDU & /*pdu*/)
{
  return false;
}


PBoolean H323EndPoint::ForwardConnection(H323Connection & connection,
                                     const PString & forwardParty,
                                     const H323SignalPDU & /*pdu*/)
{
  connection.SetPhase (OpalConnection::ForwardingPhase);

  if (InternalMakeCall(connection.GetCall(),
                       connection.GetCallToken(),
                       PString::Empty(),
                       UINT_MAX,
                       forwardParty,
                       NULL) == NULL)
    return false;

  connection.SetCallEndReason(H323Connection::EndedByCallForwarded);

  return true;
}


void H323EndPoint::OnConnectionEstablished(H323Connection & /*connection*/,
                                           const PString & /*token*/)
{
}


PBoolean H323EndPoint::IsConnectionEstablished(const PString & token)
{
  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);
  return connection != NULL && connection->IsEstablished();
}


PBoolean H323EndPoint::OnOutgoingCall(H323Connection & /*connection*/,
                                  const H323SignalPDU & /*connectPDU*/)
{
  PTRACE(3, "H225\tReceived connect PDU.");
  return true;
}


#if PTRACING
static void OnStartStopChannel(const char * startstop, const H323Channel & channel)
{
  const char * dir;
  switch (channel.GetDirection()) {
    case H323Channel::IsTransmitter :
      dir = "send";
      break;

    case H323Channel::IsReceiver :
      dir = "receiv";
      break;

    default :
      dir = "us";
      break;
  }

  PTRACE(3, "H323\t" << startstop << "ed "
                     << dir << "ing logical channel: "
                     << channel.GetCapability());
}
#endif


PBoolean H323EndPoint::OnStartLogicalChannel(H323Connection & /*connection*/,
                                         H323Channel & PTRACE_PARAM(channel))
{
#if PTRACING
  OnStartStopChannel("Start", channel);
#endif
  return true;
}


void H323EndPoint::OnClosedLogicalChannel(H323Connection & /*connection*/,
                                          const H323Channel & PTRACE_PARAM(channel))
{
#if PTRACING
  OnStartStopChannel("Stopp", channel);
#endif
}


void H323EndPoint::OnGatekeeperNATDetect(const PIPSocket::Address & /* publicAddr*/,
                                         H323TransportAddress & /*gkRouteAddress*/)
{
}


void H323EndPoint::OnHTTPServiceControl(unsigned /*opeartion*/,
                                        unsigned /*sessionId*/,
                                        const PString & /*url*/)
{
}


void H323EndPoint::OnCallCreditServiceControl(const PString & /*amount*/, PBoolean /*mode*/)
{
}


void H323EndPoint::OnServiceControlSession(unsigned type,
                                           unsigned sessionId,
                                           const H323ServiceControlSession & session,
                                           H323Connection * connection)
{
  session.OnChange(type, sessionId, *this, connection);
}

PBoolean H323EndPoint::OnCallIndependentSupplementaryService(const H323SignalPDU & /* setupPDU */)
{
  return false;
}

PBoolean H323EndPoint::OnNegotiateConferenceCapabilities(const H323SignalPDU & /* setupPDU */)
{
  return false;
}

H323ServiceControlSession * H323EndPoint::CreateServiceControlSession(const H225_ServiceControlDescriptor & contents)
{
  switch (contents.GetTag()) {
    case H225_ServiceControlDescriptor::e_url :
      return new H323HTTPServiceControl(contents);

    case H225_ServiceControlDescriptor::e_callCreditServiceControl :
      return new H323CallCreditServiceControl(contents);
  }

  return NULL;
}


void H323EndPoint::SetDefaultLocalPartyName(const PString & name)
{
  if (!PAssert(!name.IsEmpty(), "Must have non-empty string in AliasAddress!"))
    return;

  OpalEndPoint::SetDefaultLocalPartyName(name);
  SetAliasNames(name);
}


bool H323EndPoint::SetAliasNames(const PStringList & names)
{
  if (!PAssert(!names.IsEmpty() && !names.front().IsEmpty(), "Must have non-empty string in first AliasAddress!"))
    return false;

  m_aliasMutex.Wait();

  m_localAliasNames.clear();
  for (PStringList::const_iterator it = names.begin(); it != names.end(); ++it)
    AddAliasName(*it, PString::Empty(), false);

  m_aliasMutex.Signal();

  return InternalRestartGatekeeper();
}


bool H323EndPoint::AddAliasNames(const PStringList & names, const PString & gk, bool updateGk)
{
  bool addedOne = false;

  m_aliasMutex.Wait();

  for (PStringList::const_iterator it = names.begin(); it != names.end(); ++it)
    addedOne = AddAliasName(*it, gk, false) || addedOne;

  m_aliasMutex.Signal();

  return addedOne && (!updateGk || InternalRestartGatekeeper());
}


bool H323EndPoint::RemoveAliasNames(const PStringList & names, bool updateGk)
{
  bool removedOne = false;

  m_aliasMutex.Wait();

  for (PStringList::const_iterator it = names.begin(); it != names.end(); ++it)
    removedOne = RemoveAliasName(*it, false) || removedOne;

  m_aliasMutex.Signal();

  return removedOne && (!updateGk || InternalRestartGatekeeper());
}


bool H323EndPoint::AddAliasName(const PString & name, const PString & gk, bool updateGk)
{
  if (!PAssert(!name.IsEmpty(), "Must have non-empty string in alias name!"))
    return false;

  {
    PWaitAndSignal mutex(m_aliasMutex);

    if (m_localAliasNames.find(name) != m_localAliasNames.end()) {
      PTRACE(4, "H323\tAlias already present: \"" << name << '"');
      return false;
    }

    OpalTransportAddress gkAddr(gk, H225_RAS::DefaultRasUdpPort, OpalTransportAddress::UdpPrefix());
    PTRACE(3, "H323\tAdded alias: \"" << name << "\" to gk at " << gkAddr << " update=" << boolalpha << updateGk);
    m_localAliasNames[name] = gkAddr;
  }

  return !updateGk || InternalRestartGatekeeper();
}


bool H323EndPoint::RemoveAliasName(const PString & name, bool updateGk)
{
  if (!PAssert(!name.IsEmpty(), "Must have non-empty string in alias name!"))
    return false;

  {
    PWaitAndSignal mutex(m_aliasMutex);

    AliasToGkMap::iterator it = m_localAliasNames.find(name);
    if (it == m_localAliasNames.end()) {
      PTRACE(3, "H323\tAlias already removed: \"" << name << '"');
      return false;
    }

    if (m_localAliasNames.size() == 1) {
      PTRACE(3, "H323\tLast alias cannot be removed");
      return false;
    }

    m_localAliasNames.erase(it);
  }

  PTRACE(3, "H323\tAlias removed: \"" << name << "\" update=" << boolalpha << updateGk);
  return !updateGk || InternalRestartGatekeeper();
}


PStringList H323EndPoint::GetAliasNames() const
{
  PWaitAndSignal mutex(m_aliasMutex);

  PStringList names;
  for (AliasToGkMap::const_iterator it = m_localAliasNames.begin(); it != m_localAliasNames.end(); ++it)
    names += it->first;
  return names;
}


int H323EndPoint::ParseAliasPatternRange(const PString & pattern, PString & start, PString & end)
{
  if (!pattern.Split('-', start, end, PString::SplitTrim | PString::SplitNonEmpty) &&
      !pattern.Split("..", start, end, PString::SplitTrim | PString::SplitNonEmpty))
      return 0;

  if (start.GetLength() != end.GetLength())
    return 0;
  if (!OpalIsE164(start))
    return 0;
  if (!OpalIsE164(end))
    return 0;

  PINDEX commonDigits = 0;
  while (start[commonDigits] == end[commonDigits]) {
    if (++commonDigits >= start.GetLength())
      return 0;
  }

  if (start.Mid(commonDigits) >= end.Mid(commonDigits))
    return 0;

  return commonDigits;
}


bool H323EndPoint::SetAliasNamePatterns(const PStringList & patterns)
{
  for (PStringList::const_iterator it = patterns.begin(); it != patterns.end(); ++it) {
    PString start, end;
    if (ParseAliasPatternRange(*it, start, end) == 0) {
      PTRACE(2, "H323\tIllegal range pattern  \"" << *it << '"');
      return false;
    }
  }

  PWaitAndSignal mutex(m_aliasMutex);
  m_localAliasPatterns.clear();
  return AddAliasNamePatterns(patterns);
}


bool H323EndPoint::AddAliasNamePatterns(const PStringList & patterns, const PString & gk, bool updateGk)
{
  bool addedOne = false;

  m_aliasMutex.Wait();

  for (PStringList::const_iterator it = patterns.begin(); it != patterns.end(); ++it)
    addedOne = AddAliasNamePattern(*it, gk, false) || addedOne;

  m_aliasMutex.Signal();

  return addedOne && (!updateGk || InternalRestartGatekeeper());
}


bool H323EndPoint::RemoveAliasNamePatterns(const PStringList & patterns, bool updateGk)
{
  bool removedOne = false;

  m_aliasMutex.Wait();

  for (PStringList::const_iterator it = patterns.begin(); it != patterns.end(); ++it)
    removedOne = RemoveAliasNamePattern(*it, false) || removedOne;

  m_aliasMutex.Signal();

  return removedOne && (!updateGk || InternalRestartGatekeeper());
}


bool H323EndPoint::AddAliasNamePattern(const PString & pattern, const PString & gk, bool updateGk)
{
  if (pattern.IsEmpty()) {
    PTRACE(2, "H323\tMust have non-empty string in alias pattern !");
    return false;
  }

  {
    PString start, end;
    if (ParseAliasPatternRange(pattern, start, end) == 0) {
      PTRACE(2, "H323\tIllegal range pattern  \"" << pattern << '"');
      return false;
    }
  }

  {
    PWaitAndSignal mutex(m_aliasMutex);

    if (m_localAliasPatterns.find(pattern) != m_localAliasPatterns.end()) {
      PTRACE(3, "H323\tAlias pattern already present: \"" << pattern << '"');
      return false;
    }

    OpalTransportAddress gkAddr(gk, H225_RAS::DefaultRasUdpPort, OpalTransportAddress::UdpPrefix());
    PTRACE(3, "H323\tAdded alias pattern: \"" << pattern << "\" to gk at " << gkAddr << " update=" << boolalpha << updateGk);
    m_localAliasPatterns[pattern] = gkAddr;
  }

  return !updateGk || InternalRestartGatekeeper();
}


bool H323EndPoint::RemoveAliasNamePattern(const PString & pattern, bool updateGk)
{
  if (!PAssert(!pattern.IsEmpty(), "Must have non-empty string in alias pattern !"))
    return false;

  {
    PWaitAndSignal mutex(m_aliasMutex);

    AliasToGkMap::iterator it = m_localAliasPatterns.find(pattern);
    if (it == m_localAliasPatterns.end()) {
      PTRACE(3, "H323\tAlias pattern already removed: \"" << pattern << '"');
      return false;
    }

    m_localAliasPatterns.erase(it);
  }

  PTRACE(3, "H323\tRemoved alias pattern: \"" << pattern << "\" update=" << boolalpha << updateGk);
  return !updateGk || InternalRestartGatekeeper();
}


PStringList H323EndPoint::GetAliasNamePatterns() const
{
  PWaitAndSignal mutex(m_aliasMutex);

  PStringList names;
  for (AliasToGkMap::const_iterator it = m_localAliasPatterns.begin(); it != m_localAliasPatterns.end(); ++it)
    names += it->first;
  return names;
}


bool H323EndPoint::HasAlias(const PString & alias) const
{
  PWaitAndSignal mutex(m_aliasMutex);

  for (AliasToGkMap::const_iterator it = m_localAliasNames.begin(); it != m_localAliasNames.end(); ++it) {
    if (alias == it->first)
      return true;
  }

  for (AliasToGkMap::const_iterator it = m_localAliasPatterns.begin(); it != m_localAliasPatterns.end(); ++it) {
    PString start, end;
    if (ParseAliasPatternRange(it->first, start, end) > 0 && start.GetLength() == alias.GetLength() && start <= alias && alias <= end)
      return true;
  }

  return false;
}


PBoolean H323EndPoint::IsTerminal() const
{
  switch (terminalType) {
    case e_TerminalOnly :
    case e_TerminalAndMC :
      return true;

    default :
      return false;
  }
}


PBoolean H323EndPoint::IsGateway() const
{
  switch (terminalType) {
    case e_GatewayOnly :
    case e_GatewayAndMC :
    case e_GatewayAndMCWithDataMP :
    case e_GatewayAndMCWithAudioMP :
    case e_GatewayAndMCWithAVMP :
      return true;

    default :
      return false;
  }
}


PBoolean H323EndPoint::IsGatekeeper() const
{
  switch (terminalType) {
    case e_GatekeeperOnly :
    case e_GatekeeperWithDataMP :
    case e_GatekeeperWithAudioMP :
    case e_GatekeeperWithAVMP :
      return true;

    default :
      return false;
  }
}


PBoolean H323EndPoint::IsMCU() const
{
  switch (terminalType) {
    case e_MCUOnly :
    case e_MCUWithDataMP :
    case e_MCUWithAudioMP :
    case e_MCUWithAVMP :
      return true;

    default :
      return false;
  }
}


void H323EndPoint::TranslateTCPAddress(PIPSocket::Address & localAddr,
                                       const PIPSocket::Address & remoteAddr)
{
  m_manager.TranslateIPAddress(localAddr, remoteAddr);
}


#if OPAL_H460
H460_FeatureSet * H323EndPoint::CreateFeatureSet(H323Connection *)
{
  return new H460_FeatureSet(*this);
}


H460_FeatureSet * H323EndPoint::InternalCreateFeatureSet(H323Connection * connection)
{
  if (m_features == NULL) {
    m_features = CreateFeatureSet(NULL);
    if (m_features != NULL)
      m_features->LoadFeatureSet();
  }

  H460_FeatureSet * features = CreateFeatureSet(connection);
  if (features != NULL)
    features->LoadFeatureSet(connection);
  return features;
}


PBoolean H323EndPoint::OnSendFeatureSet(H460_MessageType pduType, H225_FeatureSet & featureSet)
{
  return m_features != NULL && m_features->OnSendPDU(pduType, featureSet);
}


void H323EndPoint::OnReceiveFeatureSet(H460_MessageType pduType, const H225_FeatureSet & featureSet)
{
  if (m_features != NULL)
    m_features->OnReceivePDU(pduType, featureSet);
}


bool H323EndPoint::OnLoadFeature(H460_Feature & /*feature*/)
{
  return true;
}

#endif // OPAL_H460


PString H323EndPoint::GetCompatibility(H323Connection::CompatibilityIssues issue) const
{
  CompatibilityEndpoints::const_iterator it = m_compatibility.find(issue);
  return it != m_compatibility.end() ? it->second.GetPattern() : PString::Empty();
}


bool H323EndPoint::SetCompatibility(H323Connection::CompatibilityIssues issue, const PString & regex)
{
  if (regex.IsEmpty()) {
    m_compatibility.erase(issue);
    return true;
  }

  PRegularExpression re;
  if (!re.Compile(regex, PRegularExpression::Extended|PRegularExpression::IgnoreCase))
    return false;

  m_compatibility[issue] = re;
  return true;
}


bool H323EndPoint::AddCompatibility(H323Connection::CompatibilityIssues issue, const PString & regex)
{
  if (!PAssert(!regex.IsEmpty(), PInvalidParameter))
    return false;

  PString pattern;

  CompatibilityEndpoints::iterator it = m_compatibility.find(issue);
  if (it != m_compatibility.end())
    pattern = it->second.GetPattern() + '|';

  pattern += regex;

  return SetCompatibility(issue, pattern);
}


bool H323EndPoint::HasCompatibilityIssue(H323Connection::CompatibilityIssues issue, const OpalProductInfo & productInfo) const
{
  bool found = false;
  CompatibilityEndpoints::const_iterator it = m_compatibility.find(issue);
  if (it != m_compatibility.end())
    found = productInfo.AsString().FindRegEx(it->second) != P_MAX_INDEX;

  PTRACE(found ? 3 : 4, "H.323\t" << (found ? "Found" : "Checked")
         << " compatibility issue " << issue << ", "
            "product=\"" << productInfo << "\", "
            "regex=\"" << (it != m_compatibility.end() ? it->second.GetPattern() : PString::Empty()) << '"');
  return found;
}
#endif // OPAL_H323
