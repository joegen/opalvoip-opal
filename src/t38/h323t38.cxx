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
 * $Log: h323t38.cxx,v $
 * Revision 1.2007  2002/01/14 06:35:59  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.5  2001/11/12 05:32:12  robertj
 * Added OpalTransportAddress::GetIpAddress when don't need port number.
 *
 * Revision 2.4  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.3  2001/08/13 05:10:40  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.2  2001/08/01 05:07:52  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.1  2001/08/01 05:05:26  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.14  2002/01/10 04:08:42  robertj
 * Fixed missing bit rate in mode request PDU.
 *
 * Revision 1.13  2002/01/09 00:21:40  robertj
 * Changes to support outgoing H.245 RequstModeChange.
 *
 * Revision 1.12  2002/01/01 23:27:50  craigs
 * Added CleanupOnTermination functions
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.11  2001/12/22 03:21:58  robertj
 * Added create protocol function to H323Connection.
 *
 * Revision 1.10  2001/12/22 01:55:06  robertj
 * Removed vast quatities of redundent code that is done by ancestor class.
 *
 * Revision 1.9  2001/12/19 09:15:43  craigs
 * Added changes from Vyacheslav Frolov
 *
 * Revision 1.8  2001/12/14 08:36:36  robertj
 * More implementation of T.38, thanks Adam Lazur
 *
 * Revision 1.7  2001/11/20 03:04:30  robertj
 * Added ability to reuse t38 channels with same session ID.
 *
 * Revision 1.6  2001/11/09 05:39:54  craigs
 * Added initial T.38 support thanks to Adam Lazur
 *
 * Revision 1.5  2001/09/12 07:48:05  robertj
 * Fixed various problems with tracing.
 *
 * Revision 1.4  2001/08/06 03:08:57  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 *
 * Revision 1.3  2001/07/24 02:26:24  robertj
 * Added UDP, dual TCP and single TCP modes to T.38 capability.
 *
 * Revision 1.2  2001/07/19 10:48:20  robertj
 * Fixed bandwidth
 *
 * Revision 1.1  2001/07/17 04:44:32  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323t38.h"
#endif

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
  : H323DataCapability(OpalT38Protocol::MediaFormat)
{
  mode = m;
}


PObject::Comparison H323_T38Capability::Compare(const PObject & obj) const
{
  Comparison result = H323DataCapability::Compare(obj);
  if (result != EqualTo)
    return result;

  PAssert(obj.IsDescendant(H323_T38Capability::Class()), PInvalidCast);
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


unsigned H323_T38Capability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_t38fax;
}


H323Channel * H323_T38Capability::CreateChannel(H323Connection & connection,
                                                H323Channel::Directions direction,
                                                unsigned int sessionID,
                             const H245_H2250LogicalChannelParameters * /*params*/) const
{
  PTRACE(1, "H323T38\tCreateChannel, sessionID=" << sessionID << " direction=" << direction);

  H323_T38Channel * chan = (H323_T38Channel *)connection.FindChannel(sessionID,
                                                direction == H323Channel::IsTransmitter);
  return new H323_T38Channel(connection, *this, direction,
                             chan != NULL ? chan->GetHandler() : NULL);
}


BOOL H323_T38Capability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  PTRACE(3, "H323T38\tOnSendingPDU for capability");

  pdu.m_maxBitRate = FAX_BIT_RATE;
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_t38fax);
  H245_DataApplicationCapability_application_t38fax & fax = pdu.m_application;
  return OnSendingPDU(fax.m_t38FaxProtocol, fax.m_t38FaxProfile);
}


BOOL H323_T38Capability::OnSendingPDU(H245_DataMode & pdu) const
{
  pdu.m_bitRate = FAX_BIT_RATE;
  pdu.m_application.SetTag(H245_DataMode_application::e_t38fax);
  H245_DataMode_application_t38fax & fax = pdu.m_application;
  return OnSendingPDU(fax.m_t38FaxProtocol, fax.m_t38FaxProfile);
}


BOOL H323_T38Capability::OnSendingPDU(H245_DataProtocolCapability & proto,
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

  return TRUE;
}


BOOL H323_T38Capability::OnReceivedPDU(const H245_DataApplicationCapability & cap)
{
  PTRACE(3, "H323T38\tOnRecievedPDU for capability");

  if (cap.m_application.GetTag() != H245_DataApplicationCapability_application::e_t38fax)
    return FALSE;

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

  return TRUE;
}


//////////////////////////////////////////////////////////////

H323_T38Channel::H323_T38Channel(H323Connection & connection,
                        const H323_T38Capability & capability,
                        H323Channel::Directions dir,
                        OpalT38Protocol * handler)
        : H323DataChannel(connection, capability, dir)
{
  PTRACE(3, "H323T38\tH323 channel created");

  separateReverseChannel = capability.GetTransportMode() != H323_T38Capability::e_SingleTCP;
  usesTCP = capability.GetTransportMode() != H323_T38Capability::e_UDP;

  if (handler != NULL) {
    PTRACE(3, "H323T38\tConnected to existing T.38 handler");
    t38handler = handler;
  }
  else {
    PTRACE(3, "H323T38\tCreating new T.38 handler");
    t38handler = connection.CreateT38ProtocolHandler();
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


BOOL H323_T38Channel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  if (t38handler != NULL)
    return H323DataChannel::OnSendingPDU(open);

  PTRACE(1, "H323T38\tNo protocol handler, aborting OpenLogicalChannel.");
  return FALSE;
}


BOOL H323_T38Channel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                    unsigned & errorCode)
{
  if (t38handler != NULL)
    return H323DataChannel::OnReceivedPDU(open, errorCode);

  errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
  PTRACE(1, "H323T38\tNo protocol handler, refusing OpenLogicalChannel.");
  return FALSE;
}


void H323_T38Channel::Receive()
{
  PTRACE(2, "H323T38\tReceive thread started.");

  if (t38handler != NULL) {
    if (listener != NULL)
      transport = listener->Accept(30000);  // 30 second wait for connect back

    if (transport != NULL)
      t38handler->Answer(*transport);
    else {
      PTRACE(1, "H323T38\tNo transport, aborting thread.");
    }
  }
  else {
    PTRACE(1, "H323T38\tNo protocol handler, aborting thread.");
  }

  connection.CloseLogicalChannelNumber(number);

  PTRACE(2, "H323T38\tReceive thread ended");
}


void H323_T38Channel::Transmit()
{
  if (terminating)
    return;

  PTRACE(2, "H323T38\tTransmit thread starting");

  if (t38handler != NULL)
    t38handler->Originate(*transport);
  else {
    PTRACE(1, "H323T38\tTransmit no proto handler");
  }

  connection.CloseLogicalChannelNumber(number);

  PTRACE(2, "H323T38\tTransmit thread terminating");
}


BOOL H323_T38Channel::CreateTransport()
{
  if (transport != NULL)
    return TRUE;

  if (usesTCP)
    return H323DataChannel::CreateTransport();

  PIPSocket::Address ip;
  if (!connection.GetControlChannel().GetLocalAddress().GetIpAddress(ip)) {
    PTRACE(2, "H323T38\tTrying to use UDP when base transport is not IP");
    PIPSocket::GetHostAddress(ip);
  }

  transport = new OpalTransportUDP(connection.GetEndPoint(), ip);
  PTRACE(3, "H323T38\tCreated transport: " << *transport);
  return TRUE;
}


BOOL H323_T38Channel::CreateListener()
{
  if (listener != NULL)
    return TRUE;

  if (usesTCP) {
    return H323DataChannel::CreateListener();
    PTRACE(3, "H323T38\tCreated listener " << *listener);
  }

  return CreateTransport();
}


/////////////////////////////////////////////////////////////////////////////
