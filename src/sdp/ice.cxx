/*
 * ice.cxx
 *
 * Interactive Connectivity Establishment
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2010 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "ice.h"
#endif

#include <sdp/ice.h>

#if OPAL_ICE

#include <ptclib/random.h>
#include <opal/connection.h>
#include <opal/endpoint.h>
#include <opal/manager.h>


#define PTraceModule() "ICE"
#define new PNEW


// Magic numbers from RFC5245
const unsigned HostTypePriority = 126;
const unsigned PeerReflexiveTypePriority = 110;
const unsigned ServerReflexiveTypePriority = 100;
const unsigned RelayTypePriority = 0;
const unsigned CandidateTypePriority[PNatCandidate::NumTypes] = {
  HostTypePriority,
  ServerReflexiveTypePriority,
  PeerReflexiveTypePriority,
  RelayTypePriority
};

bool operator!=(const OpalICEMediaTransport::CandidatesArray & left, const OpalICEMediaTransport::CandidatesArray & right)
{
  if (left.size() != right.size())
    return true;

  for (size_t i = 0; i < left.size(); ++i) {
    if (left[i] != right[i])
      return true;
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////

OpalICEMediaTransport::OpalICEMediaTransport(const PString & name)
  : OpalUDPMediaTransport(name)
  , m_localUsername(PBase64::Encode(PRandom::Octets(12)))
  , m_localPassword(PBase64::Encode(PRandom::Octets(18)))
  , m_lite(false)
  , m_trickle(false)
  , m_useNetworkCost(false)
  , m_state(e_Disabled)
{
  PTRACE_CONTEXT_ID_TO(m_server);
  PTRACE_CONTEXT_ID_TO(m_client);
}


OpalICEMediaTransport::~OpalICEMediaTransport()
{
  InternalStop();
}


bool OpalICEMediaTransport::Open(OpalMediaSession & session,
                                 PINDEX count,
                                 const PString & localInterface,
                                 const OpalTransportAddress & remoteAddress)
{
  const PStringOptions & options = session.GetStringOptions();
  m_lite = options.GetBoolean(OPAL_OPT_ICE_LITE, true);
  if (!m_lite) {
    PTRACE(2, "Only ICE-Lite supported at this time");
    return false;
  }

  m_trickle = options.GetBoolean(OPAL_OPT_TRICKLE_ICE);
  m_useNetworkCost = options.GetBoolean(OPAL_OPT_NETWORK_COST_ICE);
  m_iceTimeout = options.GetVar(OPAL_OPT_ICE_TIMEOUT, session.GetConnection().GetEndPoint().GetManager().GetICETimeout());

  // As per RFC 5425
  static const PTimeInterval MinTimeout(0,15);
  if (m_iceTimeout < MinTimeout)
    m_iceTimeout = MinTimeout;
  if (m_mediaTimeout < MinTimeout)
    m_mediaTimeout = MinTimeout;

  if (!OpalUDPMediaTransport::Open(session, count, localInterface, remoteAddress))
      return false;

  // Open the STUN server and set what credentials we have so far
  m_server.Open(GetSubChannelAsSocket(e_Data), GetSubChannelAsSocket(e_Control));
  m_server.SetCredentials(m_localUsername + ':' + m_remoteUsername, m_localPassword, PString::Empty());

  /* With ICE we start the thread straight away, as we need to respond to STUN
     requests before we get an answer back from the remote, which is when we
     would usually start the read thread. */
  Start();
  return true;
}


bool OpalICEMediaTransport::IsEstablished() const
{
  if (m_state != e_Disabled && m_selectedCandidate.m_type == PNatCandidate::EndTypes)
    return false;
  return OpalUDPMediaTransport::IsEstablished();
}


void OpalICEMediaTransport::InternalRxData(SubChannels subchannel, const PBYTEArray & data)
{
  if (m_state == e_Disabled)
    OpalUDPMediaTransport::InternalRxData(subchannel, data);
  else
    OpalMediaTransport::InternalRxData(subchannel, data);
}


bool OpalICEMediaTransport::InternalOpenPinHole(PUDPSocket &)
{
  // Opening pin hole not needed as ICE protocol has already done so
  return true;
}


PChannel * OpalICEMediaTransport::AddWrapperChannels(SubChannels subchannel, PChannel * channel)
{
  if (m_localCandidates.size() <= (size_t)subchannel)
    m_localCandidates.resize(subchannel + 1);

  if (m_remoteCandidates.size() <= (size_t)subchannel)
    m_remoteCandidates.resize(subchannel + 1);

  return new ICEChannel(*this, subchannel, channel);
}


void OpalICEMediaTransport::SetCandidates(const PString & user, const PString & pass, const PNatCandidateList & remoteCandidates)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  if (user.IsEmpty() || pass.IsEmpty()) {
    PTRACE(3, *this << "ICE disabled");
    m_state = e_Disabled;
    return;
  }

  if (m_state == e_Completed && user == m_remoteUsername && pass == m_remotePassword) {
    PTRACE(4, *this << "completed ICE username/password unchanged, ignoring candidates.");
    return;
  }

  CandidatesArray newCandidates(m_subchannels.size());
  for (PNatCandidateList::const_iterator it = remoteCandidates.begin(); it != remoteCandidates.end(); ++it) {
    if (it->m_protocol == "udp" && it->m_component > 0 && (size_t)it->m_component <= newCandidates.size())
      newCandidates[it->m_component - 1].push_back(*it);
    else
      PTRACE(3, "Invalid/unsupported candidate: " << *it);
  }

  PTRACE_IF(2, newCandidates.empty(), *this << "no suitable ICE candidates from remote: state=" << m_state);

  switch (m_state) {
    case e_Disabled:
      PTRACE(3, *this << "ICE initial answer");
      m_state = e_Answering;
      break;

    case e_Completed:
      PTRACE(2, *this << "ICE restart (username/password changed)");
      m_state = e_Answering;
      break;

    case e_Offering:
      PTRACE(4, *this << "ICE offer answered");
      m_state = e_OfferAnswered;
      break;

    default:
      PTRACE_IF(3, newCandidates != m_remoteCandidates, *this << "ICE candidates in bundled session different!");
      return;
  }

  m_remoteUsername = user;
  m_remotePassword = pass;

  /* Merge candidates with what we might already have. Typically, that is
     candidates that came in via STUN before the SDP answer did. Though, in
     the future, it could also be trickle ICE too. */
  for (size_t i = 0; i < newCandidates.size(); ++i) {
    for (CandidateStateList::iterator itNew = newCandidates[i].begin(); itNew != newCandidates[i].end(); ++itNew) {
      bool add = true;
      for (CandidateStateList::iterator itOld = m_remoteCandidates[i].begin(); itOld != m_remoteCandidates[i].end(); ++itOld) {
        if (*itOld != *itNew && // Not identical
             itOld->m_foundation.IsEmpty() && // Came from early STUN
             itOld->m_component  == itNew->m_component &&
             itOld->m_protocol   == itNew->m_protocol &&
             itOld->m_baseTransportAddress == itNew->m_baseTransportAddress)
        {
          PTRACE(4, *this << "ICE candidate merged:\n"
                             "  New ICE candidate: " << setw(-1) << *itNew << "\n"
                             "  Old ICE candidate: " << setw(-1) << *itOld);
          /* Update the fields. For some, the SDP value is "better" and for
             others, like networkCost, the value in the STUN request is
             "better". */
          itOld->m_type = itNew->m_type;
          itOld->m_priority = itNew->m_priority;
          itOld->m_foundation = itNew->m_foundation;
          if (itOld->m_networkId == 0)
            itOld->m_networkId = itNew->m_networkId;
          if (itOld->m_networkCost == 0)
            itOld->m_networkCost = itNew->m_networkCost;
          itOld->m_localTransportAddress = itNew->m_localTransportAddress;

          // Update the early USE-CANDIDATE as well
          if (m_selectedCandidate.m_component  == itOld->m_component &&
              m_selectedCandidate.m_protocol == itOld->m_protocol &&
              m_selectedCandidate.m_baseTransportAddress == itOld->m_baseTransportAddress)
            m_selectedCandidate = *itOld;

          add = false;
          break;
        }
      }
      if (add) {
        PTRACE(4, *this << "ICE candidate added: " << PNatCandidate(*itNew)); // Remove statistics
        m_remoteCandidates[i].push_back(*itNew);
      }
    }
  }

  if (m_lite) {
    m_server.SetIceRole(PSTUN::IceLite);
    m_client.SetIceRole(PSTUN::IceLite);
  }
  else if (m_state == e_Answering) {
    m_server.SetIceRole(PSTUN::IceControlled);
    m_client.SetIceRole(PSTUN::IceControlled);
  }
  else {
    m_server.SetIceRole(PSTUN::IceControlling);
    m_client.SetIceRole(PSTUN::IceControlling);
  }

  m_server.SetCredentials(m_localUsername + ':' + m_remoteUsername, m_localPassword, PString::Empty());
  m_client.SetCredentials(m_remoteUsername + ':' + m_localUsername, m_remotePassword, PString::Empty());

  SetRemoteBehindNAT();

  // If we had an early USE-CANDIDATE before we got this answer to our offer, then we complete now.
  if (m_state == e_OfferAnswered && m_selectedCandidate.m_type != PNatCandidate::EndTypes) {
    PTRACE(4, *this << "Using early USE-CANDIDATE " << m_selectedCandidate);
    InternalSetRemoteAddress(m_selectedCandidate.m_baseTransportAddress,
                              static_cast<SubChannels>(m_selectedCandidate.m_component - 1),
                              e_RemoteAddressFromICE);
    m_state = e_Completed;
  }

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTRACE_BEGIN(3);
    trace << *this << "ICE ";
    if (m_state == e_OfferAnswered)
      trace << "remote response to local";
    else
      trace << "configured from remote";
    trace << " candidates:";
    for (size_t i = 0; i < m_localCandidates.size(); ++i)
      trace << " local-" << (SubChannels)i << '=' << m_localCandidates[i].size();
    for (size_t i = 0; i < m_remoteCandidates.size(); ++i)
      trace << " remote-" << (SubChannels)i << '=' << m_remoteCandidates[i].size();
    trace << PTrace::End;
  }
#endif
}


bool OpalICEMediaTransport::GetCandidates(PString & user, PString & pass, PNatCandidateList & candidates, bool offering)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  if (m_subchannels.empty()) {
    PTRACE(3, *this << "ICE cannot offer when transport not open");
    return false;
  }

  user = m_localUsername;
  pass = m_localPassword;

  if (m_state == e_Completed) {
    PTRACE(4, *this << "completed ICE unchanged, no candidates being provided");
    return true;
  }

  // Only do ICE-Lite right now so just offer "host" type using local address.
  static const char LiteFoundation[] = "xyzzy";

  CandidatesArray newCandidates(m_subchannels.size());
  for (ChannelArray::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it) {
    // Only do ICE-Lite right now so just offer "host" type using local address.
    static const PNatMethod::Component ComponentId[2] = { PNatMethod::eComponent_RTP, PNatMethod::eComponent_RTCP };
    PNatCandidate candidate(PNatCandidate::HostType, ComponentId[it->m_subchannel], LiteFoundation);
    it->m_localAddress.GetIpAndPort(candidate.m_baseTransportAddress);
    candidate.m_priority = (CandidateTypePriority[candidate.m_type] << 24) | (256 - candidate.m_component);

    if (candidate.m_baseTransportAddress.GetAddress().GetVersion() != 6)
      candidate.m_priority |= 0xffff00;
    else {
      /* Incomplete need to get precedence from following table, for now use 50
            Prefix        Precedence Label
            ::1/128               50     0
            ::/0                  40     1
            2002::/16             30     2
            ::/96                 20     3
            ::ffff:0:0/96         10     4
      */
      candidate.m_priority |= 50 << 8;
    }

    candidates.push_back(candidate);
    newCandidates[it->m_subchannel].push_back(candidate);
  }

  if (candidates.empty()) {
    PTRACE(3, *this << "ICE has no candidates to offer");
    return false;
  }

  if (offering) {
    switch (m_state) {
      case e_Disabled:
      case e_Completed:
        break;

      case e_Offering:
        if (m_localCandidates == newCandidates) {
          PTRACE(2, *this << "ICE offer in progress, duplicate candidates");
          return true; // Duplicate, probably due to bundling, ignore
        }

        PTRACE(2, *this << "ICE local offer changed");
        break;

      default:
        PTRACE(2, *this << "ICE state error, making local offer when in state " << m_state);
        break;
    }
    m_localCandidates = newCandidates;
    m_state = e_Offering;
  }

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTRACE_BEGIN(3);
    trace << *this << "ICE ";
    switch (m_state) {
      case e_Answering:
        trace << "responding to received";
        break;
      case e_Offering:
        trace << "configured with offered";
        break;
      default :
        trace << "sending unchanged";
    }
    trace << " candidates:";
    for (size_t i = 0; i < m_localCandidates.size(); ++i)
      trace << " local-" << (SubChannels)i << '=' << m_localCandidates[i].size();
    for (size_t i = 0; i < m_remoteCandidates.size(); ++i)
      trace << " remote-" << (SubChannels)i << '=' << m_remoteCandidates[i].size();
    trace << PTrace::End;
  }
#endif

  if (m_state == e_Answering)
    m_state = e_OfferAnswered;

  if (m_trickle) {
      candidates.push_back(PNatCandidate(PNatCandidate::FinalType, PNatMethod::eComponent_RTP, LiteFoundation, 1));
      candidates.back().m_baseTransportAddress.SetAddress(PIPSocket::GetDefaultIpAny(), 9);
  }
  return true;
}


#if OPAL_STATISTICS
void OpalICEMediaTransport::GetStatistics(OpalMediaStatistics & statistics) const
{
  OpalMediaTransport::GetStatistics(statistics);

  statistics.m_candidates.clear();
  for (size_t subchannel = 0; subchannel < m_subchannels.size(); ++subchannel) {
    for (CandidateStateList::const_iterator it = m_remoteCandidates[subchannel].begin(); it != m_remoteCandidates[subchannel].end(); ++it)
      statistics.m_candidates.push_back(*it);
  }
}
#endif // OPAL_STATISTICS


OpalICEMediaTransport::ICEChannel::ICEChannel(OpalICEMediaTransport & owner, SubChannels subchannel, PChannel * channel)
  : m_owner(owner)
  , m_subchannel(subchannel)
{
  Open(channel);
}


PBoolean OpalICEMediaTransport::ICEChannel::Read(void * data, PINDEX size)
{
  for (;;) {
    SetReadTimeout(m_owner.m_state <= e_Completed ? m_owner.m_mediaTimeout : m_owner.m_iceTimeout);
    if (!PIndirectChannel::Read(data, size))
      return false;
    if (m_owner.InternalHandleICE(m_subchannel, data, GetLastReadCount()))
      return true;
  }
}


bool OpalICEMediaTransport::InternalHandleICE(SubChannels subchannel, const void * data, PINDEX length)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return true;

  if (m_state == e_Disabled)
    return true;

  PUDPSocket * socket = GetSubChannelAsSocket(subchannel);
  PIPAddressAndPort ap;
  socket->GetLastReceiveAddress(ap);

  PSTUNMessage message((BYTE *)data, length, ap);
  if (!message.IsValid()) {
    /* During ICE restart, continue to accept traffic from previously-selected candidate
       until a new one is selected. */
    if (m_state == e_Completed && m_selectedCandidate.m_baseTransportAddress == ap)
      return true; // Only process non-STUN packets from the selected candidate

    PTRACE(5, *this << subchannel << ", ignoring data "
           << (m_state == e_Completed ? "from un-selected ICE candidate" : "before ICE completed")
           << ": from=" << ap << " len=" << length);
    return false;
  }

  CandidateState * candidate = NULL;
  for (CandidateStateList::iterator it = m_remoteCandidates[subchannel].begin(); it != m_remoteCandidates[subchannel].end(); ++it) {
    if (it->m_baseTransportAddress == ap) {
      candidate = &*it;
      break;
    }
  }
  if (candidate == NULL) {
    if (!message.IsRequest()) {
      PTRACE(4, *this << subchannel << ", ignoring unexpected STUN message: " << message);
      return false;
    }

    /* If we had not got the candidate via SDP, then there was some sort of short cut to
       a trickle ICE and this candidate took too long to determine by the signalling system.
       We will assume that was a TURN server, as that would be typically slowest to set up,
       and give it lowest possible priority. Note, that if we have a PRIORITY attribute in
       the STUN, and it was constructed as per RFC recommendation, we can actually determine
       the type from it. */
    PNatCandidate newCandidate(PNatCandidate::RelayType, (PNatMethod::Component)(subchannel+1));

    PSTUNIcePriority * priAttr = PSTUNIcePriority::Find(message);
    if (priAttr != NULL) {
      newCandidate.m_priority = priAttr->m_priority;
      switch (newCandidate.m_priority >> 24) {
        case HostTypePriority :
          newCandidate.m_type = PNatCandidate::HostType;
          break;

        case PeerReflexiveTypePriority :
          newCandidate.m_type = PNatCandidate::PeerReflexiveType;
          break;

        case ServerReflexiveTypePriority :
          newCandidate.m_type = PNatCandidate::ServerReflexiveType;
          break;

        case RelayTypePriority :
          break;

        default :
          PTRACE(4, "Could not derive the candidate type from priority (" << newCandidate.m_priority << ") in STUN message.");
      }
    }

    newCandidate.m_baseTransportAddress = ap;
    m_remoteCandidates[subchannel].push_back(newCandidate);
    candidate = &m_remoteCandidates[subchannel].back();
    PTRACE(3, *this << subchannel << ", received STUN request for unknown ICE candidate, adding: " << newCandidate);
  }

  if (message.IsRequest()) {
#if OPAL_STATISTICS
    candidate->m_rxRequests.Count();
#endif

    if (!m_server.OnReceiveMessage(message, PSTUNServer::SocketInfo(socket)))
      return false; // Probably a authentication error

    PSTUNIceNetworkCost * costAttr = PSTUNIceNetworkCost::Find(message);
    if (costAttr != NULL) {
      unsigned networkId = costAttr->m_networkId;
      unsigned networkCost = costAttr->m_networkCost;
      if (candidate->m_networkId != networkId || candidate->m_networkCost != networkCost) {
        PTRACE(4, *this << subchannel << ","
               " updating candidate network cost:"
               " old-id=" << candidate->m_networkId << ","
               " new-id=" << networkId << ","
               " old-cost " << candidate->m_networkCost << ","
               " new-cost=" << networkCost);
        candidate->m_networkId = networkId;
        candidate->m_networkCost = networkCost;
      }
    }

    if (message.FindAttribute(PSTUNAttribute::USE_CANDIDATE) == NULL) {
      PTRACE_IF(4, m_state != e_Completed, *this << subchannel << ", "
                << (m_state == e_Offering ? "early" : "answer")
                << " STUN request in ICE, awaiting USE-CANDIDATE");
      return false;
    }

#if OPAL_STATISTICS
    ++candidate->m_nominations;
    candidate->m_lastNomination.SetCurrentTime();
#endif

    /* Already got this candidate, so don't do any more processing, but still return
       false as we don't want next layer in stack trying to use this STUN packet. */
    if (m_state == e_Completed && m_selectedCandidate.m_baseTransportAddress == ap)
      return false;

    candidate->m_state = e_CandidateSucceeded;
    PTRACE(3, *this << subchannel << ", ICE found USE-CANDIDATE from " << ap);

    /* With ICE-lite (which only supports regular nomination), only one candidate pair
       should ever be selected. However, Firefox only supports aggressive nomination,
       see https://bugzilla.mozilla.org/show_bug.cgi?id=1034964 which means we have
       to implement at least that small part of full ICE. So, we check for if we
       already have a selected candidate, and ignore any others unless one with a
       higher priority arrives. */
    if (m_state == e_Completed) {
      bool useNewCandidate;
      if (!m_useNetworkCost)
        useNewCandidate = candidate->m_priority > m_selectedCandidate.m_priority;
      else if (candidate->m_networkCost < m_selectedCandidate.m_networkCost)
        useNewCandidate = true;
      else if (candidate->m_networkCost > m_selectedCandidate.m_networkCost)
        useNewCandidate = false;
      else
        useNewCandidate = candidate->m_priority > m_selectedCandidate.m_priority;
      if (!useNewCandidate)
        return false;
      PTRACE(3, *this << subchannel << ", ICE found better candidate: " << *candidate);
    }
  }
  else if (message.IsSuccessResponse()) {
    if (m_state != e_Offering) {
      PTRACE(3, *this << subchannel << ", unexpected STUN response in ICE: " << message);
      return false;
    }

    if (!m_client.ValidateMessageIntegrity(message))
      return false;
  }
  else {
    PTRACE(5, *this << subchannel << ", unexpected STUN message in ICE: " << message);
    return false;
  }

#if OPAL_STATISTICS
  for (CandidateStateList::iterator it = m_remoteCandidates[subchannel].begin(); it != m_remoteCandidates[subchannel].end(); ++it)
    it->m_selected = &*it == candidate;
#endif
  m_selectedCandidate = *candidate;

  // Do not compelte, if we just got an early USE-CANDIDATE
  if (m_state != e_Offering) {
    InternalSetRemoteAddress(ap, subchannel, e_RemoteAddressFromICE);
    m_state = e_Completed;
  }

  // Don't pass this STUN packet up the protocol stack
  return false;
}


#if EXPERIMENTAL_ICE
OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendICE(Channel channel)
{
  if (m_candidateType == e_LocalCandidates && m_socket[channel] != NULL) {
    for (CandidateStateList::iterator it = m_candidates[channel].begin(); it != m_candidates[channel].end(); ++it) {
      if (it->m_baseTransportAddress.IsValid()) {
        PTRACE(4, *this << "sending BINDING-REQUEST to " << *it);
        PSTUNMessage request(PSTUNMessage::BindingRequest);
        request.AddAttribute(PSTUNAttribute::ICE_CONTROLLED); // We are ICE-lite and always controlled
        m_client.AppendMessageIntegrity(request);
        if (!request.Write(*m_socket[channel], it->m_baseTransportAddress))
          return e_AbortTransport;
      }
    }
  }
  return m_remotePort[channel] != 0 ? e_ProcessPacket : e_IgnorePacket;
}
#endif


#endif // OPAL_ICE

/////////////////////////////////////////////////////////////////////////////
