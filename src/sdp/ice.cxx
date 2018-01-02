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
#include <ptclib/pstunsrvr.h>
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

/////////////////////////////////////////////////////////////////////////////

OpalICEMediaTransport::OpalICEMediaTransport(const PString & name)
  : OpalUDPMediaTransport(name)
  , m_localUsername(PBase64::Encode(PRandom::Octets(12)))
  , m_localPassword(PBase64::Encode(PRandom::Octets(18)))
  , m_lite(false)
  , m_trickle(false)
  , m_state(e_Disabled)
  , m_server(NULL)
  , m_client(NULL)
  , m_selectedCandidate(NULL)
{
}


OpalICEMediaTransport::~OpalICEMediaTransport()
{
  InternalStop();
  delete m_server;
  delete m_client;
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
  m_iceTimeout = options.GetVar(OPAL_OPT_ICE_TIMEOUT, session.GetConnection().GetEndPoint().GetManager().GetICETimeout());

  // As per RFC 5425
  static const PTimeInterval MinTimeout(0,15);
  if (m_iceTimeout < MinTimeout)
    m_iceTimeout = MinTimeout;
  if (m_mediaTimeout < MinTimeout)
    m_mediaTimeout = MinTimeout;

  return OpalUDPMediaTransport::Open(session, count, localInterface, remoteAddress);
}


bool OpalICEMediaTransport::IsEstablished() const
{
  return m_state <= e_Completed && OpalUDPMediaTransport::IsEstablished();
}


void OpalICEMediaTransport::InternalRxData(SubChannels subchannel, const PBYTEArray & data)
{
  if (m_state == e_Disabled)
    OpalUDPMediaTransport::InternalRxData(subchannel, data);
  else
    OpalMediaTransport::InternalRxData(subchannel, data);
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

  if (remoteCandidates.IsEmpty()) {
    PTRACE(4, *this << "no ICE candidates from remote");
    return;
  }

  CandidatesArray newCandidates(m_subchannels.size());
  for (PINDEX i = 0; i < newCandidates.GetSize(); ++i)
    newCandidates.SetAt(i, new CandidateStateList);

  bool noSuitableCandidates = true;
  for (PNatCandidateList::const_iterator it = remoteCandidates.begin(); it != remoteCandidates.end(); ++it) {
    PTRACE(4, "Checking candidate: " << *it);
    if (it->m_protocol == "udp" && it->m_component > 0 && (PINDEX)it->m_component <= newCandidates.GetSize()) {
      newCandidates[it->m_component - 1].push_back(*it);
      noSuitableCandidates = false;
    }
  }

  if (noSuitableCandidates) {
    PTRACE(2, *this << "no suitable ICE candidates from remote");
    m_state = e_Disabled;
    return;
  }

  switch (m_state) {
    case e_Disabled :
      PTRACE(3, *this << "ICE initial answer");
      m_state = e_Answering;
      break;

    case e_Completed :
      if (user == m_remoteUsername && pass == m_remotePassword) {
        PTRACE(4, *this << "ICE username/password unchanged");
        return;
      }
      PTRACE(2, *this << "ICE restart (username/password changed)");
      m_state = e_Answering;
      break;

    case e_Offering :
      if (m_remoteCandidates.IsEmpty())
        PTRACE(4, *this << "ICE offer answered");
      else {
        if (newCandidates == m_remoteCandidates) {
          PTRACE(4, *this << "ICE answer to offer unchanged");
          m_state = e_Completed;
          return;
        }

        /* My undersanding is that an ICE restart is only when user/pass changes.
           However, Firefox changes the candidates without changing the user/pass
           so include that in test for restart. */
        PTRACE(3, *this << "ICE offer already in progress for bundle, remote candidates changed");
      }

      m_state = e_OfferAnswered;
      break;

    default :
      PTRACE_IF(2, newCandidates != m_remoteCandidates, *this << "ICE candidates in bundled session different!");
      return;
  }

  m_remoteUsername = user;
  m_remotePassword = pass;
  m_remoteCandidates = newCandidates;

  if (m_server == NULL) {
    m_server = new PSTUNServer();
    PTRACE_CONTEXT_ID_TO(m_server);
    if (m_lite)
      m_server->SetIceRole(PSTUN::IceLite);
    else if (m_state == e_Answering)
      m_server->SetIceRole(PSTUN::IceControlled);
    else
      m_server->SetIceRole(PSTUN::IceControlling);
  }
  m_server->Open(GetSubChannelAsSocket(e_Data), GetSubChannelAsSocket(e_Control));
  m_server->SetCredentials(m_localUsername + ':' + m_remoteUsername, m_localPassword, PString::Empty());

  if (m_client == NULL) {
    m_client = new PSTUNClient;
    PTRACE_CONTEXT_ID_TO(m_client);
    if (m_lite)
      m_client->SetIceRole(PSTUN::IceLite);
    else if (m_state == e_Answering)
      m_client->SetIceRole(PSTUN::IceControlled);
    else
      m_client->SetIceRole(PSTUN::IceControlling);
  }
  m_client->SetCredentials(m_remoteUsername + ':' + m_localUsername, m_remotePassword, PString::Empty());

  SetRemoteBehindNAT();

  for (size_t subchannel = 0; subchannel < m_subchannels.size(); ++subchannel) {
    PUDPSocket * socket = GetSubChannelAsSocket((SubChannels)subchannel);
    socket->SetSendAddress(PIPAddressAndPort());

    if (dynamic_cast<ICEChannel *>(m_subchannels[subchannel].m_channel) == NULL)
      m_subchannels[subchannel].m_channel = new ICEChannel(*this, (SubChannels)subchannel, socket);
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
    for (PINDEX i = 0; i < m_localCandidates.GetSize(); ++i)
      trace << " local-" << (SubChannels)i << '=' << m_localCandidates[i].size();
    for (PINDEX i = 0; i < m_remoteCandidates.GetSize(); ++i)
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

  // Only do ICE-Lite right now so just offer "host" type using local address.
  static const char LiteFoundation[] = "xyzzy";

  CandidatesArray newCandidates(m_subchannels.size());
  for (size_t subchannel = 0; subchannel < m_subchannels.size(); ++subchannel) {
    newCandidates.SetAt(subchannel, new CandidateStateList);

    // Only do ICE-Lite right now so just offer "host" type using local address.
    static const PNatMethod::Component ComponentId[2] = { PNatMethod::eComponent_RTP, PNatMethod::eComponent_RTCP };
    PNatCandidate candidate(PNatCandidate::HostType, ComponentId[subchannel], LiteFoundation);
    GetSubChannelAsSocket((SubChannels)subchannel)->GetLocalAddress(candidate.m_baseTransportAddress);
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
    newCandidates[subchannel].push_back(candidate);
  }

  if (offering && m_localCandidates != newCandidates) {
    PTRACE_IF(2, m_state == e_Answering, *this << "ICE state error, making local offer when answering remote offer");
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
    for (PINDEX i = 0; i < m_localCandidates.GetSize(); ++i)
      trace << " local-" << (SubChannels)i << '=' << m_localCandidates[i].size();
    for (PINDEX i = 0; i < m_remoteCandidates.GetSize(); ++i)
      trace << " remote-" << (SubChannels)i << '=' << m_remoteCandidates[i].size();
    trace << PTrace::End;
  }
#endif

  if (candidates.empty())
    return false;

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
  if (m_selectedCandidate != NULL)
    statistics.m_selectedCandidate = *m_selectedCandidate;
}
#endif


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
  PSafeLockReadOnly lock(*this);
  if (!lock.IsLocked())
    return true;

  if (m_state == e_Disabled)
    return true;

  PUDPSocket * socket = GetSubChannelAsSocket(subchannel);
  PIPAddressAndPort ap;
  socket->GetLastReceiveAddress(ap);

  PSTUNMessage message((BYTE *)data, length, ap);
  if (!message.IsValid()) {
    if (m_state == e_Completed && PAssertNULL(m_selectedCandidate)->m_baseTransportAddress == ap)
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
    typedef PSTUNAttribute1<PSTUNAttribute::PRIORITY, PUInt32b> PSTUNIcePriority;
    PSTUNIcePriority * priAttr = PSTUNIcePriority::Find(message);
    if (priAttr != NULL) {
      newCandidate.m_priority = priAttr->m_parameter;
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
    m_remoteCandidates[subchannel].push_back(newCandidate);
    candidate = &m_remoteCandidates[subchannel].back();
    candidate->m_baseTransportAddress = ap;
    PTRACE(3, *this << subchannel << ", received STUN request for unknown ICE candidate, adding: " << newCandidate);
  }

  if (message.IsRequest()) {
    if (m_state == e_Offering) {
      PTRACE_IF(3, m_state != e_Completed, *this << subchannel << ", unexpected STUN request in ICE: " << message);
      return false; // Just eat the STUN packet
    }

    if (!PAssertNULL(m_server)->OnReceiveMessage(message, PSTUNServer::SocketInfo(socket)))
      return false; // Probably a authentication error

    /* Already got this candidate, so don't do any more processing, but still return
       false as we don't want next layer in stack trying to use this STUN packet. */
    if (m_state == e_Completed && PAssertNULL(m_selectedCandidate)->m_baseTransportAddress == ap)
      return false;

    if (message.FindAttribute(PSTUNAttribute::USE_CANDIDATE) == NULL) {
      PTRACE_IF(4, m_state != e_Completed, *this << subchannel << ", ICE awaiting USE-CANDIDATE");
      return false;
    }

    candidate->m_state = e_CandidateSucceeded;
    PTRACE(3, *this << subchannel << ", ICE found USE-CANDIDATE from " << ap);

    /* With ICE-lite (which only supports regular nomination), only one candidate pair
       should ever be selected. However, Firefox only supports aggressive nomination,
       see https://bugzilla.mozilla.org/show_bug.cgi?id=1034964 which means we have
       to implement at least that small part of full ICE. So, we check for if we
       already have a selected candidate, and ignore any others unless one with a
       higher priority arrives. */
    if (m_state == e_Completed && m_selectedCandidate->m_priority >= candidate->m_priority)
      return false;
  }
  else if (message.IsSuccessResponse()) {
    if (m_state != e_Offering) {
      PTRACE(3, *this << subchannel << ", unexpected STUN response in ICE: " << message);
      return false;
    }

    if (!PAssertNULL(m_client)->ValidateMessageIntegrity(message))
      return false;
  }
  else {
    PTRACE(5, *this << subchannel << ", unexpected STUN message in ICE: " << message);
    return false;
  }

  InternalSetRemoteAddress(ap, subchannel, false PTRACE_PARAM(, "ICE"));
  m_selectedCandidate = candidate;
  m_state = e_Completed;

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
        m_client->AppendMessageIntegrity(request);
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
