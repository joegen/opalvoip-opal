/*
 * h323t38.cxx
 *
 * H.323 T.38 logical channel establishment
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323t38.h"
#endif

#include <opal/buildopts.h>

#if OPAL_T38FAX

#include <t38/h323t38.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/transaddr.h>
#include <asn/h245.h>
#include <t38/t38proto.h>


#define new PNEW

#define FAX_BIT_RATE 144  //14.4k



/////////////////////////////////////////////////////////////////////////////

H323_T38Capability::H323_T38Capability(TransportMode m)
  : H323DataCapability(FAX_BIT_RATE),
    mode(m)
{
}


PObject::Comparison H323_T38Capability::Compare(const PObject & obj) const
{
  Comparison result = H323DataCapability::Compare(obj);
  if (result != EqualTo)
    return result;

  PAssert(PIsDescendant(&obj, H323_T38Capability), PInvalidCast);
  const H323_T38Capability & other = (const H323_T38Capability &)obj;

  if (mode < other.mode)
    return LessThan;

  if (mode > other.mode)
    return GreaterThan;

  return EqualTo;
}


PObject * H323_T38Capability::Clone() const
{
  return new H323_T38Capability(*this);
}

OpalMediaType H323_T38Capability::GetMediaType() const
{
  return OpalMediaType::Fax();
}

unsigned H323_T38Capability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_t38fax;
}


PString H323_T38Capability::GetFormatName() const
{
  static const char * const modes[NumTransportModes] = {
    "UDP", "TCP2", "TCP"
  };
  return PString(OPAL_T38"{") + modes[mode] + '}';
}


H323Channel * H323_T38Capability::CreateChannel(H323Connection & connection,
                                         H323Channel::Directions direction,
                                                        unsigned sessionID,
                             const H245_H2250LogicalChannelParameters * /*params*/) const
{
  PTRACE(1, "H323T38\tCreateChannel, sessionID=" << sessionID << " direction=" << direction);

  return new H323_T38Channel(connection, *this, direction, sessionID, mode);
}


PBoolean H323_T38Capability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  PTRACE(3, "H323T38\tOnSendingPDU for capability");

  pdu.m_maxBitRate = FAX_BIT_RATE;
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_t38fax);
  H245_DataApplicationCapability_application_t38fax & fax = pdu.m_application;
  return OnSendingPDU(fax.m_t38FaxProtocol, fax.m_t38FaxProfile);
}


PBoolean H323_T38Capability::OnSendingPDU(H245_DataMode & pdu) const
{
  pdu.m_bitRate = FAX_BIT_RATE;
  pdu.m_application.SetTag(H245_DataMode_application::e_t38fax);
  H245_DataMode_application_t38fax & fax = pdu.m_application;
  return OnSendingPDU(fax.m_t38FaxProtocol, fax.m_t38FaxProfile);
}


PBoolean H323_T38Capability::OnSendingPDU(H245_DataProtocolCapability & proto,
                                      H245_T38FaxProfile & profile) const
{
  if (mode == e_UDP) {
    proto.SetTag(H245_DataProtocolCapability::e_udp);
    profile.m_t38FaxRateManagement.SetTag(H245_T38FaxRateManagement::e_transferredTCF); // recommended for UDP

    profile.IncludeOptionalField(H245_T38FaxProfile::e_t38FaxUdpOptions);
    profile.m_t38FaxUdpOptions.IncludeOptionalField(H245_T38FaxUdpOptions::e_t38FaxMaxBuffer);
    profile.m_t38FaxUdpOptions.m_t38FaxMaxBuffer = 200;
    profile.m_t38FaxUdpOptions.IncludeOptionalField(H245_T38FaxUdpOptions::e_t38FaxMaxDatagram);
    profile.m_t38FaxUdpOptions.m_t38FaxMaxDatagram = 72;
    profile.m_t38FaxUdpOptions.m_t38FaxUdpEC.SetTag(H245_T38FaxUdpOptions_t38FaxUdpEC::e_t38UDPRedundancy);
  }
  else {
    proto.SetTag(H245_DataProtocolCapability::e_tcp);
    profile.m_t38FaxRateManagement.SetTag(H245_T38FaxRateManagement::e_localTCF); // recommended for TCP

    profile.IncludeOptionalField(H245_T38FaxProfile::e_t38FaxTcpOptions);
    profile.m_t38FaxTcpOptions.m_t38TCPBidirectionalMode = mode == e_SingleTCP;
  }

  return PTrue;
}


PBoolean H323_T38Capability::OnReceivedPDU(const H245_DataApplicationCapability & cap)
{
  PTRACE(3, "H323T38\tOnRecievedPDU for capability");

  if (cap.m_application.GetTag() != H245_DataApplicationCapability_application::e_t38fax)
    return PFalse;

  const H245_DataApplicationCapability_application_t38fax & fax = cap.m_application;
  const H245_DataProtocolCapability & proto = fax.m_t38FaxProtocol;

  if (proto.GetTag() == H245_DataProtocolCapability::e_udp)
    mode = e_UDP;
  else {
    const H245_T38FaxProfile & profile = fax.m_t38FaxProfile;
    if (profile.m_t38FaxTcpOptions.m_t38TCPBidirectionalMode)
      mode = e_SingleTCP;
    else
      mode = e_DualTCP;
  }

  return PTrue;
}


//////////////////////////////////////////////////////////////

static const char T38NonStandardCapabilityName[] = "T38FaxUDP";

H323_T38NonStandardCapability::H323_T38NonStandardCapability(BYTE country,
                                                             BYTE extension,
                                                             WORD manufacturer)
  : H323NonStandardDataCapability(FAX_BIT_RATE,
                                  country, extension, manufacturer,
                                  (const BYTE *)T38NonStandardCapabilityName,
                                  sizeof(T38NonStandardCapabilityName)-1)
{
}


PObject * H323_T38NonStandardCapability::Clone() const
{
  return new H323_T38NonStandardCapability(*this);
}

OpalMediaType H323_T38NonStandardCapability::GetMediaType() const
{
  return "fax";
}


PString H323_T38NonStandardCapability::GetFormatName() const
{
  return PString(OPAL_T38"{") + T38NonStandardCapabilityName + '}';
}


H323Channel * H323_T38NonStandardCapability::CreateChannel(H323Connection & connection,
                                                H323Channel::Directions direction,
                                                unsigned int sessionID,
                             const H245_H2250LogicalChannelParameters * /*params*/) const
{
  PTRACE(1, "H323T38\tCreateChannel, sessionID=" << sessionID << " direction=" << direction);

  return new H323_T38Channel(connection, *this, direction, sessionID, H323_T38Capability::e_UDP);
}


//////////////////////////////////////////////////////////////

H323_T38Channel::H323_T38Channel(H323Connection & connection,
                                 const H323Capability & capability,
                                 H323Channel::Directions dir,
                                 unsigned sessionID,
                                 H323_T38Capability::TransportMode mode)
  : H323DataChannel(connection, capability, dir, sessionID)
{
  PTRACE(3, "H323T38\tH323 channel created");

  // Transport will be owned by OpalT38Protocol
  autoDeleteTransport = PFalse;

  separateReverseChannel = mode != H323_T38Capability::e_SingleTCP;
  usesTCP = mode != H323_T38Capability::e_UDP;

  t38handler = NULL;

  H323Channel * chan = connection.FindChannel(sessionID, dir == H323Channel::IsTransmitter);
  if (chan != NULL) {
    if (PIsDescendant(chan, H323_T38Channel)) {
      PTRACE(3, "H323T38\tConnected to existing T.38 handler");
      t38handler = ((H323_T38Channel *)chan)->GetHandler();
    }
    else
      PTRACE(1, "H323T38\tCreateChannel, channel " << *chan << " is not H323_T38Channel");
  }

  if (t38handler == NULL) {
    PTRACE(3, "H323T38\tCreating new T.38 handler");
    t38handler = connection.CreateT38ProtocolHandler();
  }

  if (t38handler != NULL) {
    transport = t38handler->GetTransport();

    if (transport == NULL && !usesTCP && CreateTransport())
      t38handler->SetTransport(transport, PTrue);
  }
}


H323_T38Channel::~H323_T38Channel()
{
}

void H323_T38Channel::Close()
{
  if (terminating)
    return;

  PTRACE(3, "H323T38\tCleanUpOnTermination");
    
  if (t38handler != NULL) 
    t38handler->Close();

  H323DataChannel::Close();
}


PBoolean H323_T38Channel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  if (t38handler != NULL)
    return H323DataChannel::OnSendingPDU(open);

  PTRACE(1, "H323T38\tNo protocol handler, aborting OpenLogicalChannel.");
  return PFalse;
}


PBoolean H323_T38Channel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                    unsigned & errorCode)
{
  if (t38handler != NULL)
    return H323DataChannel::OnReceivedPDU(open, errorCode);

  errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
  PTRACE(1, "H323T38\tNo protocol handler, refusing OpenLogicalChannel.");
  return PFalse;
}


void H323_T38Channel::Receive()
{
  PTRACE(2, "H323T38\tReceive thread started.");

  if (t38handler != NULL) {
    if (listener != NULL) {
      transport = listener->Accept(30000);  // 30 second wait for connect back
      t38handler->SetTransport(transport);
    }

    if (transport != NULL)
      t38handler->Answer();
    else {
      PTRACE(1, "H323T38\tNo transport, aborting thread.");
    }
  }
  else {
    PTRACE(1, "H323T38\tNo protocol handler, aborting thread.");
  }

  if (!terminating)
    connection.CloseLogicalChannelNumber(number);

  PTRACE(2, "H323T38\tReceive thread ended");
}


void H323_T38Channel::Transmit()
{
  if (terminating)
    return;

  PTRACE(2, "H323T38\tTransmit thread starting");

  if (t38handler != NULL)
    t38handler->Originate();
  else {
    PTRACE(1, "H323T38\tTransmit no proto handler");
  }

  if (!terminating)
    connection.CloseLogicalChannelNumber(number);

  PTRACE(2, "H323T38\tTransmit thread terminating");
}


PBoolean H323_T38Channel::CreateTransport()
{
  if (transport != NULL)
    return PTrue;

  if (usesTCP)
    return H323DataChannel::CreateTransport();

  PIPSocket::Address ip;
  if (!connection.GetControlChannel().GetLocalAddress().GetIpAddress(ip)) {
    PTRACE(2, "H323T38\tTrying to use UDP when base transport is not IP");
    PIPSocket::GetHostAddress(ip);
  }

  transport = new H323TransportUDP(connection.GetEndPoint(), ip);
  PTRACE(3, "H323T38\tCreated transport: " << *transport);
  return PTrue;
}


PBoolean H323_T38Channel::CreateListener()
{
  if (listener != NULL)
    return PTrue;

  if (usesTCP) {
    return H323DataChannel::CreateListener();
    PTRACE(3, "H323T38\tCreated listener " << *listener);
  }

  return CreateTransport();
}

#endif // OPAL_T38FAX

/////////////////////////////////////////////////////////////////////////////
