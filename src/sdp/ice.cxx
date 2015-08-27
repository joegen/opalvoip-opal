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
 * $Revision$
 * $Author$
 * $Date$
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


/////////////////////////////////////////////////////////////////////////////

class OpalICEMediaTransport::Server : public PSTUNServer
{
  public:
    virtual void OnBindingResponse(const PSTUNMessage &, PSTUNMessage & response)
    {
      response.AddAttribute(PSTUNAttribute::USE_CANDIDATE);
    }
};


OpalICEMediaTransport::OpalICEMediaTransport(const PString & name)
  : OpalUDPMediaTransport(name)
  , m_localUsername(PBase64::Encode(PRandom::Octets(12)))
  , m_localPassword(PBase64::Encode(PRandom::Octets(18)))
  , m_promiscuous(false)
  , m_state(e_Disabled)
  , m_server(NULL)
  , m_client(NULL)
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
  m_readTimeout = session.GetStringOptions().GetVar(OPAL_OPT_ICE_TIMEOUT, session.GetConnection().GetEndPoint().GetManager().GetICETimeout());
  m_promiscuous = session.GetStringOptions().GetBoolean(OPAL_OPT_ICE_PROMISCUOUS);

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
      m_server = new Server();
      PTRACE_CONTEXT_ID_TO(m_server);
  }
  m_server->Open(GetSocket(e_Data),GetSocket(e_Control));
  m_server->SetCredentials(m_localUsername + ':' + m_remoteUsername, m_localPassword, PString::Empty());

  if (m_client == NULL) {
      m_client = new PSTUNClient;
      PTRACE_CONTEXT_ID_TO(m_client);
  }
  m_client->SetCredentials(m_remoteUsername + ':' + m_localUsername, m_remotePassword, PString::Empty());

  m_remoteBehindNAT = true;

  for (size_t subchannel = 0; subchannel < m_subchannels.size(); ++subchannel) {
    PUDPSocket * socket = GetSocket((SubChannels)subchannel);
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

  CandidatesArray newCandidates(m_subchannels.size());
  for (size_t subchannel = 0; subchannel < m_subchannels.size(); ++subchannel) {
    newCandidates.SetAt(subchannel, new CandidateStateList);

    // Only do ICE-Lite right now so just offer "host" type using local address.
    static const PNatMethod::Component ComponentId[2] = { PNatMethod::eComponent_RTP, PNatMethod::eComponent_RTCP };
    PNatCandidate candidate(PNatCandidate::HostType, ComponentId[subchannel], "xyzzy");
    GetSocket((SubChannels)subchannel)->GetLocalAddress(candidate.m_baseTransportAddress);
    candidate.m_priority = (126 << 24) | (256 - candidate.m_component);

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

  return !candidates.empty();
}


OpalICEMediaTransport::ICEChannel::ICEChannel(OpalICEMediaTransport & owner, SubChannels subchannel, PChannel * channel)
  : m_owner(owner)
  , m_subchannel(subchannel)
{
  Open(channel);
}


PBoolean OpalICEMediaTransport::ICEChannel::Read(void * data, PINDEX size)
{
  PTimeInterval oldTimeout = GetReadTimeout();
  if (m_owner.m_state > e_Completed)
    SetReadTimeout(m_owner.m_readTimeout);

  while (PIndirectChannel::Read(data, size)) {
    if (m_owner.InternalHandleICE(m_subchannel, data, GetLastReadCount())) {
      SetReadTimeout(oldTimeout);
      return true;
    }
  }
  return false;
}


bool OpalICEMediaTransport::InternalHandleICE(SubChannels subchannel, const void * data, PINDEX length)
{
  PSafeLockReadOnly lock(*this);
  if (!lock.IsLocked())
    return true;

  if (m_state == e_Disabled)
    return true;

  PUDPSocket * socket = GetSocket(subchannel);
  PIPAddressAndPort ap;
  socket->GetLastReceiveAddress(ap);

  PSTUNMessage message((BYTE *)data, length, ap);
  if (!message.IsValid()) {
    if (m_state == e_Completed)
      return true;

    PTRACE(5, *this << subchannel << ", invalid STUN message or data before ICE completed");
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
    if (!m_promiscuous) {
      PTRACE(2, *this << subchannel << ", ignoring STUN message for unknown ICE candidate: " << ap);
      return false;
    }

    m_remoteCandidates[subchannel].push_back(PNatCandidate(PNatCandidate::HostType, (PNatMethod::Component)(subchannel+1)));
    candidate = &m_remoteCandidates[subchannel].back();
    candidate->m_baseTransportAddress = ap;
    PTRACE(2, *this << subchannel << ", received STUN message for unknown ICE candidate, adding: " << ap);
  }

  if (message.IsRequest()) {
    if (m_state == e_Offering) {
      PTRACE_IF(3, m_state != e_Completed, *this << subchannel << ", unexpected STUN request in ICE");
      return false; // Just eat the STUN packet
    }

    if (!PAssertNULL(m_server)->OnReceiveMessage(message, PSTUNServer::SocketInfo(socket)))
      return false;

    if (message.FindAttribute(PSTUNAttribute::USE_CANDIDATE) == NULL) {
      PTRACE(4, *this << subchannel << ", ICE awaiting USE-CANDIDATE");
      return false;
    }

    candidate->m_state = e_CandidateSucceeded;
    PTRACE(3, *this << subchannel << ", ICE found USE-CANDIDATE");
  }
  else {
    if (m_state != e_Offering) {
      PTRACE(3, *this << subchannel << ", unexpected STUN response in ICE");
      return false;
    }

    if (!PAssertNULL(m_client)->ValidateMessageIntegrity(message))
      return false;
  }

  InternalSetRemoteAddress(ap, subchannel, false PTRACE_PARAM(, "ICE"));
  m_state = e_Completed;

  return true;
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
