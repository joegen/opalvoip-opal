/*
 * h460_std19.cxx
 *
 * H.460.19 H225 NAT Traversal class.
 *
 * Copyright (c) 2008 ISVO (Asia) Pte. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * triple-IT. http://www.triple-it.nl.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *                 Robert Jongbloed (robertj@voxlucida.com.au).
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#define P_FORCE_STATIC_PLUGIN 1

#include <h460/h460_std19.h>

#if OPAL_H460_NAT

#include <h323/h323ep.h>
#include <h323/h323pdu.h>
#include <h323/gkclient.h>
#include <h460/h46019.h>
#include <ptclib/random.h>
#include <ptclib/cypher.h>


#define PTraceModule() "H46019"

PCREATE_NAT_PLUGIN(H46019, "H.460.19");


///////////////////////////////////////////////////////
// H.460.19
//
// Must Declare for Factory Loader.
H460_FEATURE(Std19, "H.460.19");

H460_FeatureStd19::H460_FeatureStd19()
  : H460_Feature(19)
  , m_natMethod(NULL)
  , m_disabledByH46024(false)
  , m_remoteIsServer(false)
{
  PTRACE(6, "Instance Created");

  AddParameter(1);  // add compulsory parameter: we can transmit multiplexed data
}


bool H460_FeatureStd19::Initialise(H323EndPoint & ep, H323Connection * con)
{
  if (!H460_Feature::Initialise(ep, con))
    return false;

  if (m_endpoint->GetH46019Server() != NULL)
    AddParameter(2);

  m_natMethod = GetNatMethod(PNatMethod_H46019::MethodName());
  return m_natMethod != NULL;
}


bool H460_FeatureStd19::OnSendPDU(H460_MessageType pduType, H460_FeatureDescriptor & pdu)
{
  H460_Feature::OnSendPDU(pduType, pdu);
  return !m_disabledByH46024;
}


void H460_FeatureStd19::OnReceivePDU(H460_MessageType pduType, const H460_FeatureDescriptor & pdu)
{
  H460_Feature::OnReceivePDU(pduType, pdu);
  m_remoteIsServer = pdu.HasParameter(2);
}


bool H460_FeatureStd19::OnSendingOLCGenericInformation(unsigned sessionID, H245_ArrayOf_GenericParameter & param, bool /*isAck*/)
{
  if (!PAssert(m_connection != NULL, PLogicError))
    return false;

  OpalRTPSession * session = dynamic_cast<OpalRTPSession *>(m_connection->GetMediaSession(sessionID));
  if (session == NULL) {
    PTRACE(4, "Session " << sessionID << " not found or not RTP.");
    return false;
  }

  H46019_TraversalParameters traversal;

  H46019Server * server = m_endpoint->GetH46019Server();
  if (server == NULL) {
    H46019UDPSocket * rtp = dynamic_cast<H46019UDPSocket *>(&session->GetDataSocket());
    if (rtp == NULL) {
      PTRACE(3, "Session " << sessionID << " RTP socket not generated by H.460.19.");
      return false;
    }

    traversal.IncludeOptionalField(H46019_TraversalParameters::e_keepAlivePayloadType);
    ((PASN_Integer &)traversal.m_keepAlivePayloadType).SetValue(rtp->FindKeepAlivePayloadType(*m_connection));
  }
  else {
    if (server->GetKeepAliveAddress().SetPDU(traversal.m_keepAliveChannel)) {
      traversal.IncludeOptionalField(H46019_TraversalParameters::e_keepAliveChannel);
      traversal.IncludeOptionalField(H46019_TraversalParameters::e_keepAliveInterval);
      ((H225_TimeToLive &)traversal.m_keepAliveInterval).SetValue(server->GetKeepAliveTTL());
    }

    if (server->GetMuxRTPAddress().SetPDU(traversal.m_multiplexedMediaChannel))
      traversal.IncludeOptionalField(H46019_TraversalParameters::e_multiplexedMediaChannel);

    if (server->GetMuxRTCPAddress().SetPDU(traversal.m_multiplexedMediaControlChannel))
      traversal.IncludeOptionalField(H46019_TraversalParameters::e_multiplexedMediaControlChannel);

    if (traversal.HasOptionalField(H46019_TraversalParameters::e_multiplexedMediaChannel) ||
        traversal.HasOptionalField(H46019_TraversalParameters::e_multiplexedMediaControlChannel)) {
      traversal.IncludeOptionalField(H46019_TraversalParameters::e_multiplexID);
      traversal.m_multiplexID = server->CreateMultiplexID(session->GetLocalAddress(true), session->GetLocalAddress(false));
    }

    if (!traversal.HasOptionalField(H46019_TraversalParameters::e_keepAliveChannel) &&
        !traversal.HasOptionalField(H46019_TraversalParameters::e_multiplexID)) {
      PTRACE(4, "Session " << sessionID << " H.460.19 server side not configured correctly.");
      return false;
    }
  }

  PTRACE(4, "Sending Traversal Parameters:\n   " << traversal);
  param.SetSize(1);

  H245_ParameterIdentifier & id = param[0].m_parameterIdentifier;
  id.SetTag(H245_ParameterIdentifier::e_standard);
  ((PASN_Integer &)id).SetValue(1);

  H245_ParameterValue & val = param[0].m_parameterValue;
  val.SetTag(H245_ParameterValue::e_octetString);
  ((PASN_OctetString &)val).EncodeSubType(traversal);

  return true;
}


void H460_FeatureStd19::OnReceiveOLCGenericInformation(unsigned sessionID, const H245_ArrayOf_GenericParameter & params, bool /*isAck*/)
{
  if (!PAssert(m_connection != NULL, PLogicError))
    return;

  H46019_TraversalParameters traversal;
  if (  params.GetSize() == 0 ||
        params[0].m_parameterValue.GetTag() != H245_ParameterValue::e_octetString ||
       !((const PASN_OctetString &)params[0].m_parameterValue).DecodeSubType(traversal)) {
    PTRACE(2, "Error decoding Traversal Parameters!");
    return;
  }

  PTRACE(4, "Received Traversal Parameters:\n   " << traversal);

  OpalRTPSession * session = dynamic_cast<OpalRTPSession *>(m_connection->GetMediaSession(sessionID));
  if (session == NULL) {
    PTRACE(4,"Session " << sessionID << " not found or not RTP.");
    return;
  }

  H46019UDPSocket * rtp = dynamic_cast<H46019UDPSocket *>(&session->GetDataSocket());
  if (rtp == NULL) {
    PTRACE(4,"Session " << sessionID << " RTP socket not generated by H.460.19.");
    return;
  }

  H46019UDPSocket * rtcp = dynamic_cast<H46019UDPSocket *>(&session->GetControlSocket());
  if (rtcp == NULL) {
    PTRACE(4,"Session " << sessionID << " RTCP socket not generated by H.460.19.");
    return;
  }

  if (traversal.HasOptionalField(H46019_TraversalParameters::e_multiplexID)) {
    unsigned multiplexID = traversal.m_multiplexID;
    rtp->SetMultiplexID(multiplexID);
    rtcp->SetMultiplexID(multiplexID);
    PTRACE(4,"Session " << sessionID << " transmitting multiplexed data: id=" << multiplexID);

    if (traversal.HasOptionalField(H46019_TraversalParameters::e_multiplexedMediaChannel))
      session->SetRemoteAddress(H323TransportAddress(traversal.m_multiplexedMediaChannel), true);
    if (traversal.HasOptionalField(H46019_TraversalParameters::e_multiplexedMediaControlChannel))
      session->SetRemoteAddress(H323TransportAddress(traversal.m_multiplexedMediaControlChannel), false);
  }

  if (traversal.HasOptionalField(H46019_TraversalParameters::e_keepAliveChannel)) {
    H323TransportAddress keepAliveAddress(traversal.m_keepAliveChannel);
    unsigned ttl = 20;
    if (traversal.HasOptionalField(H46019_TraversalParameters::e_keepAliveInterval))
      ttl = traversal.m_keepAliveInterval;

    rtp->FindKeepAlivePayloadType(*m_connection);
    rtp->ActivateKeepAliveRTP(keepAliveAddress, ttl);
    rtcp->ActivateKeepAliveRTCP(ttl);
  }
}


/////////////////////////////////////////////////////////////////////////////

H46019Server::H46019Server(H323EndPoint & ep)
  : m_endpoint(ep)
  , m_keepAliveTTL(20)
  , m_keepAliveSocket(NULL)
  , m_rtpSocket(NULL)
  , m_rtcpSocket(NULL)
  , m_multiplexThread(NULL)
{
}


H46019Server::H46019Server(H323EndPoint & ep, const PIPSocket::Address & localInterface)
  : m_endpoint(ep)
  , m_keepAliveTTL(20)
{
  m_keepAliveSocket = m_rtpSocket = new PUDPSocket;
  m_rtcpSocket = m_rtpSocket = new PUDPSocket;

  PIPSocket * sockets[2] = { m_rtpSocket, m_rtcpSocket };
  if (ep.GetManager().GetRtpIpPortRange().Listen(sockets, 2, localInterface))
    m_multiplexThread = new PThreadObj<H46019Server>(*this, &H46019Server::MultiplexThread, false, "MuxH.460.19");
  else {
    delete m_rtpSocket;
    delete m_rtcpSocket;
    m_keepAliveSocket = m_rtcpSocket = m_rtpSocket = NULL;
    m_multiplexThread = NULL;
  }
}


H46019Server::~H46019Server()
{
  if (m_keepAliveSocket != m_rtpSocket)
    delete m_keepAliveSocket;
  delete m_rtpSocket;
  delete m_rtcpSocket;
}


H323TransportAddress H46019Server::GetKeepAliveAddress() const
{
  PIPSocketAddressAndPort ipAndPort;
  if (m_keepAliveSocket != NULL && m_keepAliveSocket->GetLocalAddress(ipAndPort))
    return H323TransportAddress(ipAndPort);

  return H323TransportAddress();
}


H323TransportAddress H46019Server::GetMuxRTPAddress() const
{
  PIPSocketAddressAndPort ipAndPort;
  if (m_rtpSocket != NULL && m_rtpSocket->GetLocalAddress(ipAndPort))
    return H323TransportAddress(ipAndPort);

  return H323TransportAddress();
}


H323TransportAddress H46019Server::GetMuxRTCPAddress() const
{
  PIPSocketAddressAndPort ipAndPort;
  if (m_rtcpSocket != NULL && m_rtcpSocket->GetLocalAddress(ipAndPort))
    return H323TransportAddress(ipAndPort);

  return H323TransportAddress();
}


unsigned H46019Server::CreateMultiplexID(const PIPSocketAddressAndPort & rtp, const PIPSocketAddressAndPort & rtcp)
{
  unsigned id;
  do {
    id = PRandom::Number();
  } while (!m_multiplexedSockets.insert(MuxMap::value_type(id, SocketPair(rtp, rtcp))).second);
  return id;
}


void H46019Server::MultiplexThread()
{
  PTRACE(4, "Started multiplex thread");

  while (m_rtpSocket->IsOpen()) {
    int selectStatus = PSocket::Select(*m_rtpSocket, *m_rtcpSocket, PMaxTimeInterval);
    if (selectStatus > 0) {
      PTRACE(1, "Select error: " << PChannel::GetErrorText((PChannel::Errors)selectStatus));
      break;
    }

    if ((-selectStatus & 1) != 0 && !Multiplex(false))
      break;

    if ((-selectStatus & 2) != 0 && !Multiplex(true))
      break;
  }

  PTRACE(4, "Ended multiplex thread");
}


bool H46019Server::Multiplex(bool rtcp)
{
  struct {
    PUInt32b m_muxId;
    BYTE     m_packet[65532];
  } buffer;

  PUDPSocket & socket = *(rtcp ? m_rtcpSocket : m_rtpSocket);
  if (!socket.Read(&buffer, sizeof(buffer))) {
    PTRACE(3, "Multiplex socket read error: " << socket.GetErrorText(PChannel::LastReadError));
    return false;
  }

  MuxMap::iterator it = m_multiplexedSockets.find(buffer.m_muxId);
  if (it == m_multiplexedSockets.end()) {
    PTRACE(3, "Received packet with unknown multiplex ID: " << buffer.m_muxId);
    return true;
  }

  if (socket.WriteTo(buffer.m_packet, socket.GetLastReadCount()-sizeof(buffer.m_muxId), rtcp ? it->second.m_rtcp : it->second.m_rtp))
    return true;

  PTRACE(3, "Multiplex socket write error: " << socket.GetErrorText(PChannel::LastWriteError));
  return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////

PNatMethod_H46019::PNatMethod_H46019(unsigned priority)
  : PNatMethod(priority)
{
}


const char * PNatMethod_H46019::MethodName()
{
  return PPlugin_PNatMethod_H46019::ServiceName();
}


PCaselessString PNatMethod_H46019::GetMethodName() const
{
  return MethodName();
}


PString PNatMethod_H46019::GetServer() const
{
  return PString::Empty();
}


void PNatMethod_H46019::InternalUpdate()
{
  m_natType = PortRestrictedNat; // Assume worst possible NAT type that can do media at all
}


PNATUDPSocket * PNatMethod_H46019::InternalCreateSocket(Component component, PObject * context)
{
  return new H46019UDPSocket(component, *dynamic_cast<OpalRTPSession *>(context));
}


bool PNatMethod_H46019::IsAvailable(const PIPSocket::Address & binding, PObject * context)
{
  if (context == NULL) {
    PTRACE(5, "Not available without context");
    return false;
  }

  if (dynamic_cast<H323Connection *>(context) == NULL) {
    OpalRTPSession * session = dynamic_cast<OpalRTPSession *>(context);
    if (session == NULL) {
      PTRACE(4, "Not available without OpalRTPSession as context");
      return false;
    }

    H323Connection * connection = dynamic_cast<H323Connection *>(&session->GetConnection());
    if (connection == NULL) {
      PTRACE(4, "Not available without H323Connection");
      return false;
    }

    if (connection->GetFeatureSet() == NULL) {
      PTRACE(4, "Not available without feature set in connection");
      return false;
    }

    H460_FeatureStd19 * feature = connection->GetFeatureSet()->GetFeatureAs<H460_FeatureStd19>(19);
    if (feature == NULL) {
      PTRACE(4, "Not available without feature in connection");
      return false;
    }

    if (!feature->IsRemoteServer()) {
      PTRACE(4, "Not available if remote is not a server");
      return false;
    }
  }

  return PNatMethod::IsAvailable(binding, context);
}


/////////////////////////////////////////////////////////////////////////////////////////////

H46019UDPSocket::H46019UDPSocket(PNatMethod::Component component, OpalRTPSession & session)
  : PNATUDPSocket(component)
  , m_session(session)
  , m_keepAlivePayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_keepAliveSequence(0)
  , m_multiplexedTransmit(false)
#if H46024_ANNEX_A_SUPPORT
  , m_localCUI(PRandom::Number(m_sessionID*100,m_sessionID*100+99)) // Some random number bases on the session id (for H.460.24A)
  , m_state(e_notRequired)
  , m_probes(0)
  , m_SSRC(session.GetSyncSourceOut())
  , m_altAddr(PIPSocket::GetInvalidAddress())
  , m_altPort(0)
  , m_h46024b(false)
#endif
{
  m_keepAliveTimer.SetNotifier(PCREATE_NOTIFIER(KeepAliveTimeout));
}


H46019UDPSocket::~H46019UDPSocket()
{
  m_keepAliveTimer.Stop();

#if H46024_ANNEX_A_SUPPORT
  m_Probe.Stop();
#endif
}


RTP_DataFrame::PayloadTypes H46019UDPSocket::FindKeepAlivePayloadType(H323Connection & connection)
{
  if (m_keepAlivePayloadType == RTP_DataFrame::IllegalPayloadType) {
    OpalMediaFormatList mediaFormats = connection.GetLocalMediaFormats();
    m_keepAlivePayloadType = RTP_DataFrame::MaxPayloadType;
    do {
      m_keepAlivePayloadType = (RTP_DataFrame::PayloadTypes)(m_keepAlivePayloadType-1);
    } while (mediaFormats.FindFormat(m_keepAlivePayloadType) != mediaFormats.end());
  }

  return m_keepAlivePayloadType;
}


void H46019UDPSocket::ActivateKeepAliveRTP(const H323TransportAddress & address, unsigned ttl)
{
  if (!PAssert(m_component == PNatMethod::eComponent_RTP, "ActivateKeepAliveRTP called on RTP socket"))
    return;

  address.GetIpAndPort(m_keepAliveAddress);
  m_keepAliveTTL.SetInterval(0, ttl);

  PTRACE(4, "Started RTP Keep Alive to " << m_keepAliveAddress << " every " << m_keepAliveTTL << " secs.");
  SendKeepAliveRTP(m_keepAliveAddress);
}


void H46019UDPSocket::ActivateKeepAliveRTCP(unsigned ttl)
{
  if (!PAssert(m_component == PNatMethod::eComponent_RTCP, "ActivateKeepAliveRTCP called on RTP socket"))
    return;

  m_keepAliveTTL.SetInterval(0, ttl);

  PTRACE(4, "Started RTCP Keep Alive reports every " << m_keepAliveTTL << " secs.");
  m_session.SendReport(true);
}


void H46019UDPSocket::KeepAliveTimeout(PTimer &, P_INT_PTR)
{
  PTRACE(4, "Keep Alive timer fired for " << (m_component == PNatMethod::eComponent_RTP ? "RTP" : "RTCP"));
  if (m_component == PNatMethod::eComponent_RTCP)
    m_session.SendReport(true);
  else
    SendKeepAliveRTP(m_keepAliveAddress);
}


void H46019UDPSocket::SendKeepAliveRTP(const PIPSocketAddressAndPort & ipAndPort)
{
  RTP_DataFrame rtp;
  rtp.SetSequenceNumber(m_keepAliveSequence);
  rtp.SetPayloadType(m_keepAlivePayloadType);
  m_session.WriteData(rtp, &ipAndPort, false);
}


PBoolean H46019UDPSocket::InternalWriteTo(const Slice * slices, size_t sliceCount, const PIPSocketAddressAndPort & ipAndPort)
{
  if (sliceCount == 1 && slices[0].GetLength() <= 1) {
    PTRACE(5, "Ignoring old NAT opening packet for " << (m_component == PNatMethod::eComponent_RTP ? "RTP" : "RTCP"));
    lastWriteCount = 1;
    return true;
  }

  // Sent something to keep alive, so reset timer
  if (m_component == PNatMethod::eComponent_RTCP || ipAndPort == m_keepAliveAddress)
    m_keepAliveTimer = m_keepAliveTTL;

  Slice * adjustedSlices;
  size_t adjustedCount;

  if (m_multiplexedTransmit) {
    PTRACE(5, "Multiplexing " << (m_component == PNatMethod::eComponent_RTP ? "RTP" : "RTCP")
           << " to " << ipAndPort << ", id=" << m_multiplexID << ", size=" << slices[0].GetLength());
    // If a muxId was given in traversal parameters, then we need to insert it
    // as the 1st four bytes of rtp data.
    adjustedCount = sliceCount+1;
    adjustedSlices = (Slice *)alloca(adjustedCount*sizeof(Slice));
    adjustedSlices[0].SetBase(&m_multiplexID);
    adjustedSlices[0].SetLength(sizeof(m_multiplexID));
    memmove(adjustedSlices+1, slices, sliceCount*sizeof(Slice));
  }
  else {
    adjustedSlices = const_cast<Slice *>(slices);
    adjustedCount = sliceCount;
  }

#if H46024_ANNEX_A_SUPPORT
  return PNATUDPSocket::InternalWriteTo(adjustedSlices, adjustedCount, m_state == e_direct ? m_detectedAddress : ipAndPort);
#else
  return PNATUDPSocket::InternalWriteTo(adjustedSlices, adjustedCount, ipAndPort);
#endif
}


#if H46024_ANNEX_A_SUPPORT
void H46019UDPSocket::H46024Adirect(bool starter)
{
  if (starter) {  // We start the direct channel 
    m_detAddr = m_altAddr;  m_detPort = m_altPort;
    PTRACE(4, "s:" << m_sessionID << (m_component == PNatMethod::eComponent_RTP ? " RTP " : " RTCP ")  
      << "Switching to " << m_detAddr << ":" << m_detPort);
    m_state = e_direct;
  } else         // We wait for the remote to start channel
    m_state = e_wait;

  m_keepAliveTimer.Stop();  // Stop the keepAlive Packets
}


PBoolean H46019UDPSocket::InternalReadFrom(Slice * slices, size_t sliceCount, PIPSocketAddressAndPort & ipAndPort)
{
  PUInt32b muxId;
  Slice * extraSlice = (Slice *)alloca((sliceCount+1)*sizeof(Slice));
  extraSlice[0].SetBase(&muxId);
  extraSlice[0].SetLength(sizeof(muxId));
  memmove(extraSlice+1, slices, sliceCount*sizeof(Slice));

  do {
    if (!PNATUDPSocket::InternalReadFrom(extraSlice, sliceCount+1, ipAndPort))
      return false;

    /// Set the detected routed remote address (on first packet received)
    if (m_h46024b && addr == m_altAddr && port == m_altPort) {
      PTRACE(4, "H46024B\ts:" << m_sessionID << (m_component == PNatMethod::eComponent_RTP ? " RTP " : " RTCP ")  
             << "Switching to " << ipAndPort);
      m_detectedAddress = ipAndPort;
      m_state = e_direct;
      m_keepAliveTimer.Stop();  // Stop the keepAlive Packets
      m_h46024b = false;
      continue;
    }

    /// Check the probe state
    switch (m_state) {
    case e_initialising:                        // RTCP only
    case e_idle:                                // RTCP only
    case e_probing:                                // RTCP only
    case e_verify_receiver:                        // RTCP only
      frame.SetSize(len);
      memcpy(frame.GetPointer(),buf,len);
      if (ReceivedProbePacket(frame,probe,success)) {
        if (success)
          ProbeReceived(probe,addr,port);
        else {
          m_pendAddr = addr; m_pendPort = port;
        }
        continue;  // don't forward on probe packets.
      }
      break;
    case e_wait:
      if (addr == keepip) {// We got a keepalive ping...
        keepAliveTimer.Stop();  // Stop the keepAlive Packets
      } else if ((addr == m_altAddr) && (port == m_altPort)) {
        PTRACE(4, "s:" << m_sessionID << (m_component == PNatMethod::eComponent_RTP ? " RTP " : " RTCP ")  << "Already sending direct!");
        m_detAddr = addr;  m_detPort = port;
        m_state = e_direct;
      } else if ((addr == m_pendAddr) && (port == m_pendPort)) {
        PTRACE(4, "s:" << m_sessionID << (m_component == PNatMethod::eComponent_RTP ? " RTP " : " RTCP ")  
          << "Switching to Direct " << addr << ":" << port);
        m_detAddr = addr;  m_detPort = port;
        m_state = e_direct;
      } else if ((addr != m_remAddr) || (port != m_remPort)) {
        PTRACE(4, "s:" << m_sessionID << (m_component == PNatMethod::eComponent_RTP ? " RTP " : " RTCP ")  
          << "Switching to " << addr << ":" << port << " from " << m_remAddr << ":" << m_remPort);
        m_detAddr = addr;  m_detPort = port;
        m_state = e_direct;
      } 
      break;
    case e_direct:    
    default:
      break;
    }
    return true;

  } while (muxId != m_muxId);
  return true;
}


PBoolean H46019UDPSocket::ReceivedProbePacket(const RTP_ControlFrame & frame, bool & probe, bool & success)
{
  success = false;

  //Inspect the probe packet
  if (frame.GetPayloadType() == RTP_ControlFrame::e_ApplDefined) {

    int cstate = m_state;
    if (cstate == e_notRequired) {
      PTRACE(6, "s:" << m_sessionID <<" received RTCP probe packet. LOGIC ERROR!");
      return true;  
    }

    if (cstate > e_probing) {
      PTRACE(6, "s:" << m_sessionID <<" received RTCP probe packet. IGNORING! Already authenticated.");
      return true;  
    }

    probe = (frame.GetCount() > 0);
    PTRACE(4, "s:" << m_sessionID <<" RTCP Probe " << (probe ? "Reply" : "Request") << " received.");    

    BYTE * data = frame.GetPayloadPtr();
    PBYTEArray bytes(20);
    memcpy(bytes.GetPointer(),data+12, 20);
#if OPAL_PTLIB_SSL
    PMessageDigest::Result bin_digest;
    PMessageDigestSHA1::Encode(m_connection.GetCallIdentifier().AsString() + m_localCUI, bin_digest);
    PBYTEArray val(bin_digest.GetPointer(),bin_digest.GetSize());
#else
    PBYTEArray val(20);
    //android_sha1(val.GetPointer(), m_CallId.AsString() + m_localCUI);
#endif

    if (bytes == val) {
      if (probe)  // We have a reply
        m_state = e_verify_sender;
      else 
        m_state = e_verify_receiver;

      m_Probe.Stop();
      PTRACE(4, "s" << m_sessionID <<" RTCP Probe " << (probe ? "Reply" : "Request") << " verified.");
      if (!m_remoteCUI.IsEmpty())
        success = true;
      else {
        PTRACE(4, "s" << m_sessionID <<" Remote not ready.");
      }
    } else {
      PTRACE(4, "s" << m_sessionID <<" RTCP Probe " << (probe ? "Reply" : "Request") << " verify FAILURE");    
    }
    return true;
  }
  else 
    return false;
}


void H46019UDPSocket::SetAlternateAddresses(const H323TransportAddress & address, const PString & cui)
{
  address.GetIpAndPort(m_altAddr,m_altPort);

  PTRACE(6, "s: " << m_sessionID << (m_component == PNatMethod::eComponent_RTP ? " RTP " : " RTCP ")  
    << "Remote Alt: " << m_altAddr << ":" << m_altPort << " CUI: " << cui);

  if (m_component != PNatMethod::eComponent_RTP) {
    m_remoteCUI = cui;
    if (m_state < e_idle) {
      m_state = e_idle;
      StartProbe();
      // We Already have a direct connection but we are waiting on the CUI for the reply
    } else if (m_state == e_verify_receiver) 
      ProbeReceived(false,m_pendingAddress);
  }
}


void H46019UDPSocket::GetAlternateAddresses(H323TransportAddress & address, PString & cui)
{
  PIPSocketAddressAndPort hostAddr;
  GetBaseAddress(hostAddr);
  address = H323TransportAddress(hostAddr);

  if (m_component != PNatMethod::eComponent_RTP)
    cui = m_localCUI;
  else
    cui.MakeEmpty();

  if (m_state < e_idle)
    m_state = e_initialising;

  PTRACE(6,"s:" << m_sessionID << (m_component == PNatMethod::eComponent_RTP ? " RTP " : " RTCP ") << " Alt:" << address << " CUI " << cui);

}


void H46019UDPSocket::StartProbe()
{
  PTRACE(4,"s: " << m_sessionID << " Starting direct connection probe.");

  m_state = e_probing;
  m_probes = 0;
  m_Probe.SetNotifier(PCREATE_NOTIFIER(Probe));
  m_Probe.RunContinuous(100); 
}


void H46019UDPSocket::BuildProbe(RTP_ControlFrame & report, bool probing)
{
  report.SetPayloadType(RTP_ControlFrame::e_ApplDefined);
  report.SetCount((probing ? 0 : 1));  // SubType Probe

  report.SetPayloadSize(sizeof(probe_packet));

  probe_packet data;
  data.SSRC = m_SSRC;
  data.Length = sizeof(probe_packet);
  PString id = "24.1";
  PBYTEArray bytes(id,id.GetLength(), false);
  memcpy(&data.name[0], bytes, 4);

#if OPAL_PTLIB_SSL
  PMessageDigest::Result bin_digest;
  PMessageDigestSHA1::Encode(m_connection.GetCallIdentifier().AsString() + m_remoteCUI, bin_digest);
  memcpy(&data.cui[0], bin_digest.GetPointer(), bin_digest.GetSize());
#else
  //android_sha1(&data.cui[0],m_CallId.AsString() + m_remoteCUI);
#endif

  memcpy(report.GetPayloadPtr(),&data,sizeof(probe_packet));

}


void H46019UDPSocket::Probe(PTimer &, P_INT_PTR)
{ 
  m_probes++;

  if (m_probes >= 5) {
    m_Probe.Stop();
    return;
  }

  if (m_state != e_probing)
    return;

  RTP_ControlFrame report;
  report.SetSize(4+sizeof(probe_packet));
  BuildProbe(report, true);

  if (!PUDPSocket::WriteTo(report.GetPointer(),report.GetSize(), m_altAddr, m_altPort)) {
    switch (GetErrorCode(LastWriteError)) {
      case PChannel::Unavailable :
        PTRACE(2, "" << m_altAddr << ":" << m_altPort << " not ready.");
        break;

      default:
        PTRACE(1, "" << m_altAddr << ":" << m_altPort 
          << ", Write error on port ("
          << GetErrorNumber(PChannel::LastWriteError) << "): "
          << GetErrorText(PChannel::LastWriteError));
      }
  } else {
    PTRACE(6, "s" << m_sessionID <<" RTCP Probe sent: " << m_altAddr << ":" << m_altPort);    
  }
}


void H46019UDPSocket::ProbeReceived(bool probe, const PIPSocketAddressAndPort & ipAndPort)
{
  if (probe) {
  }
  else {
    RTP_ControlFrame reply;
    reply.SetSize(4+sizeof(probe_packet));
    BuildProbe(reply, false);
    if (SendRTCPFrame(reply,ipAndPort)) {
      PTRACE(4, "RTCP Reply packet sent: " << ipAndPort);    
    }
  }
}


void H46019UDPSocket::H46024Bdirect(const H323TransportAddress & address)
{
  address.GetIpAndPort(m_altAddr,m_altPort);
  PTRACE(6,"H46024b\ts: " << m_sessionID << " RTP Remote Alt: " << m_altAddr << ":" << m_altPort);

  m_h46024b = true;

  // Sending an empty RTP frame to the alternate address
  // will add a mapping to the router to receive RTP from
  // the remote
  SendRTPPing(m_altAddr, m_altPort);
}
#endif


#endif  // OPAL_H460_NAT
