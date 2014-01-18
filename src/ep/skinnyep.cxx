/*
 * skinnyep.cxx
 *
 * Cisco SCCP "skinny" protocol support.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): Robert Jongbloed (robertj@voxlucida.com.au).
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <ep/skinnyep.h>

#if OPAL_SKINNY

#define PTraceModule() "Skinny"


OpalSkinnyEndPoint::OpalSkinnyEndPoint(OpalManager & manager, const char *prefix)
  : OpalRTPEndPoint(manager, prefix, CanTerminateCall | SupportsE164)
  , P_DISABLE_MSVC_WARNINGS(4355, m_serverTransport(*this))
{
  m_serverTransport.SetPDULengthFormat(-4, 4);
}


OpalSkinnyEndPoint::~OpalSkinnyEndPoint()
{
}


void OpalSkinnyEndPoint::ShutDown()
{
  PTRACE(3, prefixName << " endpoint shutting down.");
  if (m_serverTransport.IsOpen()) {
    PTRACE(4, "Unregistering");
    SendSkinnyMsg(UnregisterMsg());
    PThread::Sleep(1000); // Wait a abit for reply.
  }
}


PString OpalSkinnyEndPoint::GetDefaultTransport() const
{
  return OpalTransportAddress::TcpPrefix();
}


WORD OpalSkinnyEndPoint::GetDefaultSignalPort() const
{
  return 2000;
}


void OpalSkinnyEndPoint::NewIncomingConnection(OpalListener & /*listener*/, const OpalTransportPtr & /*transport*/)
{
}


PSafePtr<OpalConnection> OpalSkinnyEndPoint::MakeConnection(OpalCall & call,
                                                            const PString & party,
                                                            void * userData,
                                                            unsigned int options,
                                                            OpalConnection::StringOptions * stringOptions)
{
  PString number;
  if (party.NumCompare(GetPrefixName() + ':') == EqualTo)
    number = party.Mid(GetPrefixName().GetLength() + 1);
  else
    number = party;

  if (OpalIsE164(number))
    return AddConnection(CreateConnection(call, number, userData, options, stringOptions));

  PTRACE(2, "Remote party \"" << number << "\" is not an E.164 number.");
  return NULL;
}


void OpalSkinnyEndPoint::OnReleased(OpalConnection & connection)
{
  OpalSkinnyConnection & skinny = dynamic_cast<OpalSkinnyConnection &>(connection);
  SendSkinnyMsg(OpalSkinnyEndPoint::OnHookMsg(skinny.m_lineInstance, skinny.m_callidentifier));

  OpalEndPoint::OnReleased(connection);
}


PBoolean OpalSkinnyEndPoint::GarbageCollection()
{
  return OpalEndPoint::GarbageCollection();
}


OpalSkinnyConnection * OpalSkinnyEndPoint::CreateConnection(OpalCall & call,
                                                            const PString & number,
                                                            void * userData,
                                                            unsigned int options,
                                                            OpalConnection::StringOptions * stringOptions)
{
  return new OpalSkinnyConnection(call, *this, number, userData, options, stringOptions);
}


bool OpalSkinnyEndPoint::Register(const PString & server, unsigned deviceType)
{
  m_serverTransport.CloseWait();

  OpalTransportAddress addr(server, GetDefaultSignalPort(), GetDefaultTransport());
  if (!m_serverTransport.ConnectTo(addr))
    return false;

  m_serverTransport.AttachThread(new PThreadObj<OpalSkinnyEndPoint>(*this, &OpalSkinnyEndPoint::HandleServerTransport, false, "Skinny"));

  PString deviceName = "SEP";
  {
    PIPSocket::InterfaceTable interfaces;
    if (PIPSocket::GetInterfaceTable(interfaces)) {
      for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
        PString macAddrStr = interfaces[i].GetMACAddress();
        if (!macAddrStr && macAddrStr != "44-45-53-54-00-00") { /* not Win32 PPP device */
          deviceName += macAddrStr.ToUpper();
          break;
        }
      }
    }
  }

  PIPSocket::Address ip;
  m_serverTransport.GetLocalAddress().GetIpAddress(ip);
  if (!SendSkinnyMsg(RegisterMsg(deviceName, 0, 1, ip, deviceType, 0)))
    return false;
  
  PTRACE(4, "Registering client: " << deviceName << ", type=" << deviceType);
  return true;
}


void OpalSkinnyEndPoint::HandleServerTransport()
{
  PTRACE(4, "Started client handler thread");
  while (m_serverTransport.IsOpen()) {
    PBYTEArray pdu;
    if (m_serverTransport.ReadPDU(pdu)) {
      unsigned msgId = pdu.GetAs<PUInt32l>(4);
      switch (msgId) {
        case RegisterAckMsg::ID :
          PTRACE(4, "Register acknowledged");
          HandleRegisterAck(RegisterAckMsg(pdu));
          break;

        case RegisterRejectMsg::ID :
          PTRACE(2, "Registration rejected");
          break;

        case CapabilityRequestMsg::ID:
          PTRACE(4, "Sending capability");
          SendSkinnyMsg(CapabilityResponseMsg(GetMediaFormats()));
          break;

        default:
          PTRACE(4, "Received unhandled message id=0x" << hex << msgId << dec);
          break;
      }
    }
  }
  PTRACE(4, "Ended client handler thread");
}


void OpalSkinnyEndPoint::HandleRegisterAck(const RegisterAckMsg & ack)
{
  PIPSocket::AddressAndPort ap;
  if (m_serverTransport.GetLocalAddress().GetIpAndPort(ap))
    SendSkinnyMsg(PortMsg(ap.GetPort()));

  KeepAliveMsg msg;
  m_serverTransport.SetKeepAlive(ack.GetKeepALive(), PBYTEArray((const BYTE *)&msg, sizeof(msg), false));
}


bool OpalSkinnyEndPoint::SendSkinnyMsg(const SkinnyMsg & msg)
{
  return m_serverTransport.Write(&msg, msg.GetLength());
}


///////////////////////////////////////////////////////////////////////////////

OpalSkinnyConnection::OpalSkinnyConnection(OpalCall & call,
                                           OpalSkinnyEndPoint & ep,
                                           const PString & number,
                                           void * /*userData*/,
                                           unsigned options,
                                           OpalConnection::StringOptions * stringOptions)
  : OpalRTPConnection(call, ep, PString::Empty(), options, stringOptions)
  , m_endpoint(ep)
  , m_lineInstance(0)
  , m_callidentifier(0)
  , m_numberToDial(number)
{
}


PBoolean OpalSkinnyConnection::SetUpConnection()
{
  InternalSetAsOriginating();
  SetPhase(SetUpPhase);
  OnApplyStringOptions();

  // At this point we just go off hook, wait for dial tone
  m_endpoint.SendSkinnyMsg(OpalSkinnyEndPoint::OffHookMsg(m_lineInstance, m_callidentifier));
  return true;
}


///////////////////////////////////////////////////////////////////////////////

OpalSkinnyEndPoint::SkinnyMsg::SkinnyMsg(uint32_t id, PINDEX len)
  : m_length(len - sizeof(m_length) - sizeof(m_headerVersion))
  , m_headerVersion(0) // Basic header
  , m_messageId(id)
{
}


OpalSkinnyEndPoint::SkinnyMsg::SkinnyMsg(const PBYTEArray & pdu, PINDEX len)
{
  if (pdu.GetSize() == (len - (PINDEX)sizeof(m_length)))
    memcpy(this + sizeof(m_length), pdu, pdu.GetSize());
}


OpalSkinnyEndPoint::RegisterMsg::RegisterMsg(const char * deviceName,
                                             unsigned userId,
                                             unsigned instance,
                                             const PIPSocket::Address & ip,
                                             unsigned deviceType,
                                             unsigned maxStreams)
  : SkinnyMsg(ID, sizeof(*this))
  , m_userId(userId)
  , m_instance(instance)
  , m_ip(ip)
  , m_deviceType(deviceType)
  , m_maxStreams(maxStreams)
{
  strncpy(m_deviceName, PAssertNULL(deviceName), sizeof(m_deviceName)-1);
  m_deviceName[sizeof(m_deviceName)-1] = '\0';

  static BYTE const UnknownValue[sizeof(m_unknown)] = { 0, 0, 0, 0, 0xb, 0, 0x60, 0x85 };
  memcpy(m_unknown, UnknownValue, sizeof(m_unknown));
}


OpalSkinnyEndPoint::RegisterAckMsg::RegisterAckMsg(const PBYTEArray & pdu)
  : SkinnyMsg(pdu, sizeof(*this))
{
}


OpalSkinnyEndPoint::RegisterRejectMsg::RegisterRejectMsg(const PBYTEArray & pdu)
  : SkinnyMsg(pdu, sizeof(*this))
{
}


OpalSkinnyEndPoint::UnregisterMsg::UnregisterMsg()
  : SkinnyMsg(ID, sizeof(*this))
{
}


OpalSkinnyEndPoint::PortMsg::PortMsg(unsigned port)
  : SkinnyMsg(ID, sizeof(*this))
  , m_port((WORD)port)
{
}


OpalSkinnyEndPoint::CapabilityResponseMsg::CapabilityResponseMsg(const OpalMediaFormatList & formats)
  : SkinnyMsg(ID, sizeof(*this))
{
  static PStringToOrdinal::Initialiser const Codes[] = {
    { OPAL_G711_ALAW_64K,  2 },
    { OPAL_G711_ULAW_64K,  4 },
    { OPAL_G722,           6 },
    { OPAL_G7231,          9 },
    { OPAL_G728,          10 },
    { OPAL_G729,          11 },
    { OPAL_G729A,         12 },
    { OPAL_G729B,         15 },
    { OPAL_G729AB,        16 },
    { OPAL_G7221_32K,     40 },
    { OPAL_G7221_24K,     41 },
    { OPAL_GSM0610,       80 },
    { OPAL_G726_32K,      82 },
    { OPAL_G726_24K,      83 },
    { OPAL_G726_16K,      84 },
    { OPAL_H261,         100 },
    { OPAL_H263,         101 },
    { OPAL_H264,         103 },
    { OPAL_FECC_RTP,     106 },
    { OPAL_RFC2833,      257 }
  };
  static PStringToOrdinal const CodeMap(PARRAYSIZE(Codes), Codes);

  PINDEX count = 0;
  for (OpalMediaFormatList::const_iterator it = formats.begin(); it != formats.end(); ++it) {
    if (CodeMap.Contains(it->GetName())) {
      m_capability[count].m_codec = CodeMap[it->GetName()];
      m_capability[count].m_maxFramesPerPacket = (uint16_t)it->GetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption(), 1);
      ++count;
      if (count >= PARRAYSIZE(m_capability))
        break;
    }
  }

  if (PAssert(count > 0, "Must have a valid codec!")) {
    m_count = count;
    SetLength(sizeof(SkinnyMsg) + sizeof(m_count) + count*sizeof(Info));
  }
}


OpalSkinnyEndPoint::KeepAliveMsg::KeepAliveMsg()
  : SkinnyMsg(ID, sizeof(*this))
  , m_reserved(0)
{
}


OpalSkinnyEndPoint::OffHookMsg::OffHookMsg(uint32_t line, uint32_t call)
  : SkinnyMsg(ID, sizeof(*this))
  , m_lineInstance(line)
  , m_callIdentifier(call)
{
}


OpalSkinnyEndPoint::OnHookMsg::OnHookMsg(uint32_t line, uint32_t call)
  : SkinnyMsg(ID, sizeof(*this))
  , m_lineInstance(line)
  , m_callIdentifier(call)
{
}


OpalSkinnyEndPoint::StartToneMsg::StartToneMsg(const PBYTEArray & pdu)
  : SkinnyMsg(pdu, sizeof(*this))
{
}


OpalSkinnyEndPoint::KeyPadButtonMsg::KeyPadButtonMsg(char button, uint32_t line, uint32_t call)
  : SkinnyMsg(ID, sizeof(*this))
  , m_button(button)
  , m_lineInstance(line)
  , m_callIdentifier(call)
{
}


OpalSkinnyEndPoint::CallStateMsg::CallStateMsg(const PBYTEArray & pdu)
  : SkinnyMsg(pdu, sizeof(*this))
{
}


#endif // OPAL_SKINNY

///////////////////////////////////////////////////////////////////////////////
