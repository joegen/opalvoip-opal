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


static PString CreateToken(const OpalSkinnyEndPoint::PhoneDevice & client, unsigned callIdentifier)
{
  PString token = "Skinny:" + client.GetName();
  if (callIdentifier > 0)
    token.sprintf("/%u", callIdentifier);

  return token;
}

static POrdinalToString::Initialiser const CodecCodes[] = {
  {   2, OPAL_G711_ALAW_64K  },
  {   4, OPAL_G711_ULAW_64K  },
  {   6, OPAL_G722           },
  {   9, OPAL_G7231          },
  {  10, OPAL_G728           },
  {  11, OPAL_G729           },
  {  12, OPAL_G729A          },
  {  15, OPAL_G729B          },
  {  16, OPAL_G729AB         },
  {  40, OPAL_G7221_32K      },
  {  41, OPAL_G7221_24K      },
  {  80, OPAL_GSM0610        },
  {  82, OPAL_G726_32K       },
  {  83, OPAL_G726_24K       },
  {  84, OPAL_G726_16K       },
  { 100, OPAL_H261           },
  { 101, OPAL_H263           },
  { 103, OPAL_H264           },
  { 106, OPAL_FECC_RTP       },
  { 257, OPAL_RFC2833        }
};
static PStringToOrdinal const MediaFormatToCodecCode(PARRAYSIZE(CodecCodes), CodecCodes);
static POrdinalToString const CodecCodeToMediaFormat(PARRAYSIZE(CodecCodes), CodecCodes);

static PConstString const RegisteredStatusText("Registered");


///////////////////////////////////////////////////////////////////////////////

OpalSkinnyEndPoint::SkinnyMsg::SkinnyMsg(uint32_t id, PINDEX len)
{
  memset(this, 0, len);
  m_length = len - sizeof(m_length) - sizeof(m_headerVersion);
  m_messageId = id;
}


void OpalSkinnyEndPoint::SkinnyMsg::Construct(const PBYTEArray & pdu)
{
  PINDEX len = m_length + sizeof(m_headerVersion);
  memcpy((char *)this + sizeof(m_length), pdu, std::min(len, pdu.GetSize()));
  PTRACE_IF(3, len > pdu.GetSize(), &pdu, PTraceModule(), "Received message size error: "
            "id=0x" << hex << GetID() << dec << ", expected = " << len << ", received = " << pdu.GetSize());
}


///////////////////////////////////////////////////////////////////////////////

OpalSkinnyEndPoint::OpalSkinnyEndPoint(OpalManager & manager, const char *prefix)
  : OpalRTPEndPoint(manager, prefix, IsNetworkEndPoint | SupportsE164)
{
}


OpalSkinnyEndPoint::~OpalSkinnyEndPoint()
{
}


void OpalSkinnyEndPoint::ShutDown()
{
  PTRACE(3, "Endpoint shutting down.");

  for (PhoneDeviceDict::iterator it = m_phoneDevices.begin(); it != m_phoneDevices.end(); ++it)
    it->second.Stop();

  // Wait a bit for ack replies.
  for (PINDEX wait = 0; wait < 10; ++wait) {
    PhoneDeviceDict::iterator it;
    for (it = m_phoneDevices.begin(); it != m_phoneDevices.end(); ++it) {
      if (it->second.m_status == RegisteredStatusText)
        break;
    }
    if (it == m_phoneDevices.end())
      break;
    PThread::Sleep(200);
  }

  for (PhoneDeviceDict::iterator it = m_phoneDevices.begin(); it != m_phoneDevices.end(); ++it)
    it->second.Close();

  PTRACE(4, "Endpoint shut down.");
}


PString OpalSkinnyEndPoint::GetDefaultTransport() const
{
  return OpalTransportAddress::TcpPrefix();
}


WORD OpalSkinnyEndPoint::GetDefaultSignalPort() const
{
  return 2000;
}


OpalMediaFormatList OpalSkinnyEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;
  for (PINDEX i = 0; i < PARRAYSIZE(CodecCodes); ++i)
    formats += CodecCodes[i].value;
  return formats;
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
  PString numberName;
  if (party.NumCompare(GetPrefixName() + ':') == EqualTo)
    numberName = party.Mid(GetPrefixName().GetLength() + 1);
  else
    numberName = party;

  PString number, name;
  if (!numberName.Split('@', number, name))
    number = numberName;

  if (!OpalIsE164(number)) {
    PTRACE(2, "Remote party \"" << number << "\" is not an E.164 number.");
    return NULL;
  }

  PhoneDevice * client;
  if (name.IsEmpty()) {
    if (m_phoneDevices.IsEmpty()) {
      PTRACE(2, "Cannot call, nothing registered.");
      return NULL;
    }
    client = &m_phoneDevices.begin()->second;
  }
  else {
    client = m_phoneDevices.GetAt(name);
    if (client == NULL) {
      PTRACE(2, "Local client \"" << name << "\" does not exist.");
      return NULL;
    }
  }

  return AddConnection(CreateConnection(call, *client, 0, number, userData, options, stringOptions));
}


PBoolean OpalSkinnyEndPoint::GarbageCollection()
{
  return OpalEndPoint::GarbageCollection();
}


OpalSkinnyConnection * OpalSkinnyEndPoint::CreateConnection(OpalCall & call,
                                                            OpalSkinnyEndPoint::PhoneDevice & client,
                                                            unsigned callIdentifier,
                                                            const PString & dialNumber,
                                                            void * userData,
                                                            unsigned int options,
                                                            OpalConnection::StringOptions * stringOptions)
{
  return new OpalSkinnyConnection(call, *this, client, callIdentifier, dialNumber, userData, options, stringOptions);
}


bool OpalSkinnyEndPoint::Register(const PString & server, const PString & name, unsigned deviceType)
{
  if (name.IsEmpty() || name.GetLength() > RegisterMsg::MaxNameSize) {
    PTRACE(2, "Illegal client device anme \"" << name << '"');
    return false;
  }

  if (m_phoneDevices.Contains(name)) {
    PTRACE(3, "PhoneDevice \"" << name << "\" already registered");
    return false;
  }

  PhoneDevice * phoneDevice = new PhoneDevice(*this, name, deviceType);
  if (phoneDevice->Start(server)) {
    m_phoneDevices.SetAt(name, phoneDevice);
    return true;
  }

  delete phoneDevice;
  return false;
}


bool OpalSkinnyEndPoint::Unregister(const PString & name)
{
  PhoneDevice * phoneDevice = m_phoneDevices.GetAt(name);
  if (phoneDevice == NULL) {
    PTRACE(3, "PhoneDevice \"" << name << "\" not registered");
    return false;
  }

  phoneDevice->Stop();
  phoneDevice->Close();
  m_phoneDevices.RemoveAt(name);
  return true;
}


OpalSkinnyEndPoint::PhoneDevice::PhoneDevice(OpalSkinnyEndPoint & ep, const PString & name, unsigned deviceType)
  : m_endpoint(ep)
  , m_name(name)
  , m_deviceType(deviceType)
  , m_transport(ep)
{
  m_transport.SetPDULengthFormat(-4, 4);
}


void OpalSkinnyEndPoint::PhoneDevice::PrintOn(ostream & strm) const
{
  strm << m_name << '@' << m_transport.GetRemoteAddress().GetHostName() << '\t' << m_status;
}


bool OpalSkinnyEndPoint::PhoneDevice::Start(const PString & server)
{
  OpalTransportAddress addr(server, m_endpoint.GetDefaultSignalPort(), m_endpoint.GetDefaultTransport());
  if (!m_transport.SetRemoteAddress(addr)) {
    m_status = "Transport error: " + m_transport.GetErrorText();
    return false;
  }

  m_status = "Registering";
  m_transport.AttachThread(new PThreadObj<PhoneDevice>(*this, &PhoneDevice::HandleTransport, false, "Skinny"));
  return true;
}


bool OpalSkinnyEndPoint::PhoneDevice::SendRegisterMsg()
{
  RegisterMsg msg;

  PIPSocket::Address ip;
  m_transport.GetLocalAddress().GetIpAddress(ip);


  strncpy(msg.m_deviceName, m_name, sizeof(msg.m_deviceName) - 1);
  strncpy(msg.m_macAddress, PIPSocket::GetInterfaceMACAddress().ToUpper(), sizeof(msg.m_macAddress));
  msg.m_ip = ip;
  msg.m_maxStreams = 2;
  msg.m_deviceType = m_deviceType;

  if (!SendSkinnyMsg(msg))
    return false;
  
  PTRACE(4, "Sent register message for " << m_name << ", type=" << m_deviceType);
  return true;
}


void OpalSkinnyEndPoint::PhoneDevice::Stop()
{
  if (m_status == RegisteredStatusText) {
    PTRACE(4, "Unregistering " << m_name);
    SendSkinnyMsg(UnregisterMsg());
  }
}


void OpalSkinnyEndPoint::PhoneDevice::Close()
{
  PTRACE(4, "Closing " << m_name);

  // Wait a bit for ack reply.
  if (m_status == RegisteredStatusText) {
    for (PINDEX wait = 0; wait < 10; ++wait) {
      if (!m_transport.IsOpen())
        break;
      PThread::Sleep(200);
    }
  }

  m_exit.Signal();
  m_transport.CloseWait();
}


bool OpalSkinnyEndPoint::PhoneDevice::SendSkinnyMsg(const SkinnyMsg & msg)
{
  if (m_transport.Write(&msg, msg.GetLength()))
    return true;

  PTRACE(3, "Error writing message for " << m_name << " to " << m_transport.GetRemoteAddress().GetHostName());
  return false;
}


#define ON_RECEIVE_MSG(cls) \
  case cls::ID : \
  PTRACE(4, "Received " << typeid(cls).name()); \
  if (!m_endpoint.OnReceiveMsg(*this, cls(pdu))) \
    running = false; \
  break

void OpalSkinnyEndPoint::PhoneDevice::HandleTransport()
{
  PTRACE(4, "Started client handler thread: " << m_name);

  bool running = true;
  while (running) {
    PBYTEArray pdu;
    if (m_transport.ReadPDU(pdu)) {
      unsigned msgId = pdu.GetAs<PUInt32l>(4);
      switch (msgId) {
        ON_RECEIVE_MSG(KeepAliveMsg);
        ON_RECEIVE_MSG(KeepAliveAckMsg);
        ON_RECEIVE_MSG(RegisterMsg);
        ON_RECEIVE_MSG(RegisterAckMsg);
        ON_RECEIVE_MSG(RegisterRejectMsg);
        ON_RECEIVE_MSG(UnregisterMsg);
        ON_RECEIVE_MSG(UnregisterAckMsg);
        ON_RECEIVE_MSG(PortMsg);
        ON_RECEIVE_MSG(CapabilityRequestMsg);
        ON_RECEIVE_MSG(CapabilityResponseMsg);
        ON_RECEIVE_MSG(CallStateMsg);
        ON_RECEIVE_MSG(CallInfoMsg);
        ON_RECEIVE_MSG(SetRingerMsg);
        ON_RECEIVE_MSG(OffHookMsg);
        ON_RECEIVE_MSG(OnHookMsg);
        ON_RECEIVE_MSG(StartToneMsg);
        ON_RECEIVE_MSG(StopToneMsg);
        ON_RECEIVE_MSG(KeyPadButtonMsg);
        ON_RECEIVE_MSG(SoftKeyEventMsg);
        ON_RECEIVE_MSG(OpenReceiveChannelMsg);
        ON_RECEIVE_MSG(OpenReceiveChannelAckMsg);
        ON_RECEIVE_MSG(CloseReceiveChannelMsg);
        ON_RECEIVE_MSG(StartMediaTransmissionMsg);
        ON_RECEIVE_MSG(StopMediaTransmissionMsg);

        default:
          PTRACE(4, "Received unhandled message id=0x" << hex << msgId << dec);
          break;
      }
    }
    else {
      switch (m_transport.GetErrorCode(PChannel::LastReadError)) {
        case PChannel::NoError:
          PTRACE(3, "Lost transport to " << m_transport);
          m_status = "Lost transport";
        case PChannel::NotOpen:
          // Remote close of TCP
          m_transport.Close();
          while (!m_transport.Connect() || !SendRegisterMsg()) {
            PTRACE(2, "Server for " << m_name << " transport reconnect error: " << m_transport.GetErrorText());
            if (m_exit.Wait(10000)) {
              PTRACE(4, "Exiting thread for " << m_name);
              running = false;
              break;
            }
          }
          break;

        case PChannel::Timeout :
        case PChannel::Interrupted :
          break;

        default :
          m_status = "Transport error: " + m_transport.GetErrorText(PChannel::LastReadError);
          PTRACE(2, m_status);
          running = false;
      }
    }
  }

  m_transport.Close();
  PTRACE(4, "Ended client handler thread: " << m_name);
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const KeepAliveMsg &)
{
  return client.SendSkinnyMsg(KeepAliveAckMsg());
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const KeepAliveAckMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const RegisterMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const RegisterAckMsg & ack)
{
  client.m_status = RegisteredStatusText;

  PIPSocket::AddressAndPort ap;
  if (client.m_transport.GetLocalAddress().GetIpAndPort(ap))
    client.SendSkinnyMsg(PortMsg(ap.GetPort()));

  KeepAliveMsg msg;
  client.m_transport.SetKeepAlive(PTimeInterval(0, ack.m_keepAlive), PBYTEArray((const BYTE *)&msg, sizeof(msg)));
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const RegisterRejectMsg & msg)
{
  PTRACE(2, "Server rejected registration: " << msg.m_errorText);
  client.m_status = PConstString("Rejected: ") + msg.m_errorText;
  return false;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const UnregisterMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const UnregisterAckMsg &)
{
  PTRACE(2, "Unregistered from server");
  client.m_status = "Unregistered";
  return false;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const PortMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const CapabilityRequestMsg &)
{
  CapabilityResponseMsg msg;

  PINDEX count = 0;
  OpalMediaFormatList formats = GetMediaFormats();
  formats.Remove(manager.GetMediaFormatMask());
  formats.Reorder(manager.GetMediaFormatOrder());
  for (OpalMediaFormatList::iterator it = formats.begin(); it != formats.end(); ++it) {
    if (MediaFormatToCodecCode.Contains(it->GetName())) {
      msg.m_capability[count].m_codec = MediaFormatToCodecCode[it->GetName()];
      msg.m_capability[count].m_maxFramesPerPacket = (uint16_t)it->GetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption(), 1);
      ++count;
      if (count >= PARRAYSIZE(msg.m_capability))
        break;
    }
  }

  if (PAssert(count > 0, "Must have a valid codec!")) {
    msg.m_count = count;
    msg.SetLength(sizeof(SkinnyMsg)+sizeof(msg.m_count) + count*sizeof(CapabilityResponseMsg::Info));
  }

  return client.SendSkinnyMsg(msg);
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const CapabilityResponseMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const CallStateMsg & msg)
{
  // See if we are dialling out 
  PString dialOutToken = CreateToken(client, 0);
  PSafePtr<OpalSkinnyConnection> connection = PSafePtrCast<OpalConnection, OpalSkinnyConnection>(connectionsActive.FindWithLock(dialOutToken, PSafeReadWrite));
  if (connection != NULL) {
    if (!connection->OnReceiveMsg(msg))
      return false;

    // Now have call/line id's do token swapping
    connectionsActive.Move(dialOutToken, connection->GetToken());
    return true;
  }

  connection = GetSkinnyConnection(client, msg.m_callIdentifier);
  if (connection != NULL)
    return connection->OnReceiveMsg(msg);

  if (msg.GetState() != eStateRingIn) {
    PTRACE(4, "Unhandled state: " << msg.GetState());
    return true;
  }

  // Incoming call
  OpalCall * call = manager.InternalCreateCall();
  if (call == NULL) {
    PTRACE(2, "Internal failure to create call");
    return true;
  }

  connection = CreateConnection(*call, client, msg.m_callIdentifier, PString::Empty(), NULL, 0, NULL);
  if (AddConnection(connection) == NULL)
    return true;

  return connection->OnReceiveMsg(msg);
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const CallInfoMsg & msg)
{
  return DelegateMsg(client, msg);
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const SetRingerMsg & msg)
{
  return DelegateMsg(client, msg);
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const OffHookMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const OnHookMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const StartToneMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const StopToneMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const KeyPadButtonMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const SoftKeyEventMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const OpenReceiveChannelMsg & msg)
{
  return DelegateMsg(client, msg);
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice &, const OpenReceiveChannelAckMsg &)
{
  return true;
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const CloseReceiveChannelMsg & msg)
{
  return DelegateMsg(client, msg);
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const StartMediaTransmissionMsg & msg)
{
  return DelegateMsg(client, msg);
}


bool OpalSkinnyEndPoint::OnReceiveMsg(PhoneDevice & client, const StopMediaTransmissionMsg & msg)
{
  return DelegateMsg(client, msg);
}


PSafePtr<OpalSkinnyConnection> OpalSkinnyEndPoint::GetSkinnyConnection(const PhoneDevice & client, uint32_t callIdentifier, PSafetyMode mode)
{
  return PSafePtrCast<OpalConnection, OpalSkinnyConnection>(connectionsActive.FindWithLock(CreateToken(client, callIdentifier), mode));
}


///////////////////////////////////////////////////////////////////////////////

OpalSkinnyConnection::OpalSkinnyConnection(OpalCall & call,
                                           OpalSkinnyEndPoint & ep,
                                           OpalSkinnyEndPoint::PhoneDevice & client,
                                           unsigned callIdentifier,
                                           const PString & dialNumber,
                                           void * /*userData*/,
                                           unsigned options,
                                           OpalConnection::StringOptions * stringOptions)
  : OpalRTPConnection(call, ep, CreateToken(client, callIdentifier), options, stringOptions)
  , m_endpoint(ep)
  , m_client(client)
  , m_lineInstance(0)
  , m_callIdentifier(callIdentifier)
  , m_audioId(0)
  , m_videoId(0)
{
  m_calledPartyNumber = dialNumber;
}


PBoolean OpalSkinnyConnection::SetUpConnection()
{
  InternalSetAsOriginating();
  SetPhase(SetUpPhase);
  OnApplyStringOptions();

  // At this point we just go off hook, wait for CallStateMsg which creates the connection
  m_client.SendSkinnyMsg(OpalSkinnyEndPoint::OffHookMsg());
  return true;
}


void OpalSkinnyConnection::OnReleased()
{
  OpalSkinnyEndPoint::SoftKeyEventMsg msg;
  msg.m_event = OpalSkinnyEndPoint::eSoftKeyEndcall;
  msg.m_callIdentifier = m_callIdentifier;
  msg.m_lineInstance = m_lineInstance;
  m_client.SendSkinnyMsg(msg);

  OpalRTPConnection::OnReleased();
}


OpalMediaFormatList OpalSkinnyConnection::GetMediaFormats() const
{
  return m_remoteMediaFormats;
}


PBoolean OpalSkinnyConnection::SetConnected()
{
  if (GetPhase() >= ConnectedPhase)
    return true;

  SetPhase(ConnectedPhase);

  OpalSkinnyEndPoint::SoftKeyEventMsg msg;
  msg.m_event = OpalSkinnyEndPoint::eSoftKeyAnswer;
  msg.m_callIdentifier = m_callIdentifier;
  msg.m_lineInstance = m_lineInstance;
  return m_client.SendSkinnyMsg(msg);
}


OpalMediaType::AutoStartMode OpalSkinnyConnection::GetAutoStart(const OpalMediaType &) const
{
  // Driven from server, so we don't try and open media streams directly.
  return OpalMediaType::DontOffer;
}


PString OpalSkinnyConnection::GetAlertingType() const
{
  return m_alertingType;
}


bool OpalSkinnyConnection::OnReceiveMsg(const OpalSkinnyEndPoint::CallStateMsg & msg)
{
  if (m_callIdentifier != msg.m_callIdentifier) {
    m_callIdentifier = msg.m_callIdentifier;
    PTRACE(3, "Call identifier set to " << m_callIdentifier);
    PString newToken = CreateToken(m_client, m_callIdentifier);
    if (callToken != newToken) {
      callToken = newToken;
      PTRACE(3, "Set incoming calls new token to \"" << callToken << '"');
    }
  }

  if (m_lineInstance != msg.m_lineInstance) {
    m_lineInstance = msg.m_lineInstance;
    PTRACE(4, "Line instance set to " << m_lineInstance);
  }

  switch (msg.GetState()) {
    case OpalSkinnyEndPoint::eStateRingIn :
      break;

    case OpalSkinnyEndPoint::eStateOffHook:
      if (IsOriginating())
        OnProceeding();
      break;

    case OpalSkinnyEndPoint::eStateOnHook:
      Release(EndedByRemoteUser);
      break;

    case OpalSkinnyEndPoint::eStateConnected :
      if (IsOriginating())
        OnConnectedInternal();
      else
        SetPhase(EstablishedPhase);
      break;

    default :
      PTRACE(4, "Unhandled state: " << msg.GetState());
  }
  return true;
}


bool OpalSkinnyConnection::OnReceiveMsg(const OpalSkinnyEndPoint::CallInfoMsg & msg)
{
  if (msg.GetType() == OpalSkinnyEndPoint::eTypeOutboundCall) {
    remotePartyName = msg.m_calledPartyName;
    remotePartyNumber = msg.m_calledPartyNumber;
  }
  else {
    remotePartyName = msg.m_callingPartyName;
    remotePartyNumber = msg.m_callingPartyNumber;
    m_calledPartyName = msg.m_calledPartyName;
    m_calledPartyNumber = msg.m_calledPartyNumber;
    m_redirectingParty = GetPrefixName() + ':' + msg.m_lastRedirectingPartyNumber;
  }

  if (remotePartyNumber.IsEmpty() && OpalIsE164(remotePartyName))
    remotePartyNumber = remotePartyName;
  if (m_calledPartyNumber.IsEmpty() && OpalIsE164(m_calledPartyName))
    m_calledPartyNumber = m_calledPartyName;

  PTRACE(3, "Called party: number=\"" << m_calledPartyName << "\", name=\"" << m_calledPartyNumber << "\" - "
            "Remote party: number=\"" << remotePartyNumber << "\", name=\"" << remotePartyName << "\" "
            "for " << *this);

  if (GetPhase() == UninitialisedPhase) {
    SetPhase(SetUpPhase);
    OnApplyStringOptions();
    if (OnIncomingConnection(0, NULL))
      ownerCall.OnSetUp(*this);
  }
  return true;
}


bool OpalSkinnyConnection::OnReceiveMsg(const OpalSkinnyEndPoint::SetRingerMsg & msg)
{
  switch (msg.GetType()) {
    case OpalSkinnyEndPoint::eRingOff :
      return true;

    case OpalSkinnyEndPoint::eRingInside :
      m_alertingType = "Inside";
      break;

    case OpalSkinnyEndPoint::eRingOutside :
      m_alertingType = "Outside";
      break;

    default:
      break;
  }
  SetPhase(AlertingPhase);
  return true;
}


OpalMediaSession * OpalSkinnyConnection::SetUpMediaSession(uint32_t payloadCapability, bool rx)
{
  OpalMediaFormat mediaFormat = CodecCodeToMediaFormat(payloadCapability);
  if (mediaFormat.IsEmpty()) {
    PTRACE(2, "Could not find media format for capability=" << payloadCapability);
    Release(EndedByCapabilityExchange);
    return NULL;
  }

  m_remoteMediaFormats += mediaFormat;

  OpalMediaType mediaType = mediaFormat.GetMediaType();
  unsigned sessionId = mediaType->GetDefaultSessionId();

  OpalMediaSession * mediaSession = UseMediaSession(sessionId, mediaType);
  if (mediaSession == NULL) {
    PTRACE(2, "Could not create session for " << mediaFormat);
    Release(EndedByCapabilityExchange);
    return NULL;
  }

  if (!mediaSession->Open(m_client.m_transport.GetInterface(), m_client.m_transport.GetRemoteAddress(), false)) {
    PTRACE(2, "Could not open media session for " << mediaFormat);
    return NULL;
  }

  PSafePtr<OpalConnection> con = rx ? this : GetOtherPartyConnection();
  if (con == NULL || !ownerCall.OpenSourceMediaStreams(*con, mediaType, sessionId, mediaFormat)) {
    PTRACE(2, "Could not open media streams for " << mediaFormat);
    return NULL;
  }

  return mediaSession;
}


bool OpalSkinnyConnection::OnReceiveMsg(const OpalSkinnyEndPoint::OpenReceiveChannelMsg & msg)
{
  OpalMediaSession * mediaSession = SetUpMediaSession(msg.m_payloadCapability, true);
  if (mediaSession != NULL) {
    PIPSocket::AddressAndPort ap;
    mediaSession->GetLocalAddress().GetIpAndPort(ap);

    OpalSkinnyEndPoint::OpenReceiveChannelAckMsg ack;
    ack.m_passThruPartyId = msg.m_passThruPartyId;
    ack.m_ip = ap.GetAddress();
    ack.m_port = ap.GetPort();
    m_client.SendSkinnyMsg(ack);

    SetFromIdMediaType(mediaSession->GetMediaType(), msg.m_passThruPartyId);
    PTRACE(3, "Opened receive channel for " << *this);
  }

  return true;
}


bool OpalSkinnyConnection::OnReceiveMsg(const OpalSkinnyEndPoint::CloseReceiveChannelMsg & msg)
{
  OpalMediaStreamPtr mediaStream = GetMediaStream(GetMediaTypeFromId(msg.m_passThruPartyId), true);
  if (mediaStream != NULL)
    m_endpoint.GetManager().QueueDecoupledEvent(
                     new PSafeWorkArg1<OpalSkinnyConnection, OpalMediaStreamPtr>(this,
                            mediaStream, &OpalSkinnyConnection::DelayCloseMediaStream));
  return true;
}


bool OpalSkinnyConnection::OnReceiveMsg(const OpalSkinnyEndPoint::StartMediaTransmissionMsg & msg)
{
  OpalMediaSession * mediaSession = SetUpMediaSession(msg.m_payloadCapability, false);
  if (mediaSession != NULL) {
    mediaSession->SetRemoteAddress(OpalTransportAddress(msg.m_ip, msg.m_port, OpalTransportAddress::UdpPrefix()));
    SetFromIdMediaType(mediaSession->GetMediaType(), msg.m_passThruPartyId);
    PTRACE(3, "Started media transmission for " << *this);
  }

  return true;
}


bool OpalSkinnyConnection::OnReceiveMsg(const OpalSkinnyEndPoint::StopMediaTransmissionMsg & msg)
{
  OpalMediaStreamPtr mediaStream = GetMediaStream(GetMediaTypeFromId(msg.m_passThruPartyId), false);
  if (mediaStream != NULL)
    m_endpoint.GetManager().QueueDecoupledEvent(
                     new PSafeWorkArg1<OpalSkinnyConnection, OpalMediaStreamPtr>(this,
                            mediaStream, &OpalSkinnyConnection::DelayCloseMediaStream));
  return true;
}


void OpalSkinnyConnection::DelayCloseMediaStream(OpalMediaStreamPtr mediaStream)
{
  /* We delay closing the media stream slightly as Skinny server closes them
    before sending the "on hook" message for ending the call. This means that
    phantom re-INVITE or CLC gets sent when in gateway mode */
  for (PINDEX delay = 0; delay < 10; ++delay) {
    if (IsReleased())
      return;
    PThread::Sleep(50);
  }

  mediaStream->Close();
}


OpalMediaType OpalSkinnyConnection::GetMediaTypeFromId(uint32_t id)
{
  if (id == m_audioId)
    return OpalMediaType::Audio();

  if (id == m_videoId)
    return OpalMediaType::Video();

  return OpalMediaType();
}


void OpalSkinnyConnection::SetFromIdMediaType(const OpalMediaType & mediaType, uint32_t id)
{
  if (mediaType == OpalMediaType::Audio()) {
    m_audioId = id;
    PTRACE(4, "Setting audio stream id to " << id);
  }

  if (mediaType == OpalMediaType::Video()) {
    m_videoId = id;
    PTRACE(4, "Setting video stream id to " << id);
  }
}


#endif // OPAL_SKINNY

///////////////////////////////////////////////////////////////////////////////
