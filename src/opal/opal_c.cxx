/*
 * opal_c.cxx
 *
 * "C" language interface for OPAL
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * This code was initially written with the assisance of funding from
 * Stonevoice. http://www.stonevoice.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <opal.h>
#include <opal/manager.h>
#include <opal/pcss.h>
#include <h323/h323ep.h>
#include <sip/sipep.h>
#include <iax2/iax2ep.h>
#include <lids/lidep.h>
#include <opal/ivr.h>

#include <queue>


class OpalManager_C;


inline bool IsNullString(const char * str)
{
  return str == NULL || *str == '\0';
}


class OpalMessageBuffer
{
  public:
    OpalMessageBuffer(OpalMessageType type);
    ~OpalMessageBuffer();

    OpalMessage * operator->() const { return  (OpalMessage *)m_data; }
    OpalMessage & operator *() const { return *(OpalMessage *)m_data; }

    void SetString(const char * * variable, const char * value);
    void SetError(const char * errorText);

    OpalMessage * Detach();

  private:
    size_t m_size;
    char * m_data;
};

#define SET_MESSAGE_STRING(msg, member, str) (msg).SetString(&(msg)->member, str)


class OpalPCSSEndPoint_C : public OpalPCSSEndPoint
{
  public:
    OpalPCSSEndPoint_C(OpalManager_C & manager);

    PBoolean OpalPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection &);
    PBoolean OpalPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection &);

  private:
    OpalManager_C & manager;
};


#if OPAL_SIP
class SIPEndPoint_C : public SIPEndPoint
{
  public:
    SIPEndPoint_C(OpalManager_C & manager);

    virtual void OnRegistrationStatus(
      const PString & aor,
      PBoolean wasRegistering,
      PBoolean reRegistering,
      SIP_PDU::StatusCodes reason
    );

  private:
    OpalManager_C & manager;
};
#endif


class OpalManager_C : public OpalManager
{
  public:
    OpalManager_C(unsigned version)
      : pcssEP(NULL)
      , m_apiVersion(version)
      , m_messagesAvailable(0, INT_MAX)
    {
    }

    bool Initialise(const PCaselessString & options);

    void PostMessage(OpalMessageBuffer & message);
    OpalMessage * GetMessage(unsigned timeout);
    OpalMessage * SendMessage(const OpalMessage * message);

    virtual void OnEstablishedCall(OpalCall & call);
    virtual void OnUserInputString(OpalConnection & connection, const PString & value);
    virtual void OnUserInputTone(OpalConnection & connection, char tone, int duration);
    virtual void OnClearedCall(OpalCall & call);

  private:
    void HandleSetGeneral   (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleSetProtocol  (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleRegistration (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleSetUpCall    (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleAnswerCall   (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleClearCall    (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleHoldCall     (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleRetrieveCall (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleTransferCall (const OpalMessage & message, OpalMessageBuffer & response);

    OpalPCSSEndPoint_C * pcssEP;

    unsigned                  m_apiVersion;
    std::queue<OpalMessage *> m_messageQueue;
    PMutex                    m_messageMutex;
    PSemaphore                m_messagesAvailable;
};


class PProcess_C : public PProcess
{
public:
  PProcess_C(const PCaselessString & options)
  {
#if PTRACING
    unsigned level = 0;
    static char const TraceLevelKey[] = "TraceLevel=";
    PINDEX pos = options.Find(TraceLevelKey);
    if (pos != P_MAX_INDEX)
      level = options.Mid(pos+sizeof(TraceLevelKey)-1).AsUnsigned();

    PString filename = "DEBUGSTREAM";
    static char const TraceFileKey[] = "TraceFile=";
    pos = options.Find(TraceFileKey);
    if (pos != P_MAX_INDEX) {
      pos += sizeof(TraceFileKey) - 1;
      PINDEX end;
      if (options[pos] == '"')
        end = options.Find('"', ++pos);
      else
        end = options.Find(' ', pos);
      filename = options(pos, end-1);
    }

    PTrace::Initialise(level, filename);
#endif
  }

private:
  virtual void Main()
  {
  }
};

struct OpalHandleStruct
{
  OpalHandleStruct(unsigned version, const PCaselessString & options)
    : process(options)
    , manager(version)
  {
  }

  PProcess_C     process;
  OpalManager_C  manager;
};


///////////////////////////////////////////////////////////////////////////////

OpalMessageBuffer::OpalMessageBuffer(OpalMessageType type)
  : m_size(sizeof(OpalMessage))
  , m_data((char *)malloc(m_size))
{
  memset(m_data, 0, m_size);
  (*this)->m_type = type;
}


OpalMessageBuffer::~OpalMessageBuffer()
{
  if (m_data != NULL)
    free(m_data);
}


void OpalMessageBuffer::SetString(const char * * variable, const char * value)
{
  size_t length = strlen(value)+1;

  char * newData = (char *)realloc(m_data, m_size + length);
  if (PAssertNULL(newData) != m_data) {
    // Memory has moved, this probably invalidates "variable" is now invalid
    if ((void *)variable >= m_data && (void *)variable < m_data+m_size)
      variable = (const char **)(newData + ((char *)variable - m_data));
    m_data = newData;
  }

  char * stringData = m_data + m_size;
  memcpy(stringData, value, length);
  m_size += length;

  *variable = stringData;
}


void OpalMessageBuffer::SetError(const char * errorText)
{
  OpalMessage * message = (OpalMessage *)m_data;
  message->m_type = OpalIndCommandError;
  SetString(&message->m_param.m_commandError, errorText);
}


OpalMessage * OpalMessageBuffer::Detach()
{
  OpalMessage * message = (OpalMessage *)m_data;
  m_data = NULL;
  return message;
}


///////////////////////////////////////

OpalPCSSEndPoint_C::OpalPCSSEndPoint_C(OpalManager_C & mgr)
  : OpalPCSSEndPoint(mgr)
  , manager(mgr)
{
}


PBoolean OpalPCSSEndPoint_C::OnShowIncoming(const OpalPCSSConnection & connection)
{
  OpalMessageBuffer message(OpalIndIncomingCall);
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_localAddress, connection.GetLocalPartyURL());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_remoteAddress, connection.GetRemotePartyURL());
  manager.PostMessage(message);
  return true;
}


PBoolean OpalPCSSEndPoint_C::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  const OpalCall & call = connection.GetCall();
  OpalMessageBuffer message(OpalIndAlerting);
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyA, call.GetPartyA());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyB, call.GetPartyB());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_callToken, call.GetToken());
  manager.PostMessage(message);
  return true;
}


///////////////////////////////////////

#if OPAL_SIP

SIPEndPoint_C::SIPEndPoint_C(OpalManager_C & mgr)
  : SIPEndPoint(mgr)
  , manager(mgr)
{
}


void SIPEndPoint_C::OnRegistrationStatus(const PString & aor,
                                         PBoolean wasRegistering,
                                         PBoolean reRegistering,
                                         SIP_PDU::StatusCodes reason)
{
  OpalMessageBuffer message(OpalIndRegistration);
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_protocol, OPAL_PREFIX_SIP);
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_serverName, aor);
  if (reason != SIP_PDU::Successful_OK) {
    PStringStream strm;
    strm << "Error " << reason << " in SIP registration.";
    SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_error, strm);
  }
  manager.PostMessage(message);
}

#endif

///////////////////////////////////////

bool OpalManager_C::Initialise(const PCaselessString & options)
{
  PString defProto, defUser;
  PINDEX  defProtoPos = P_MAX_INDEX, defUserPos = P_MAX_INDEX;

#if OPAL_H323
  PINDEX h323Pos = options.Find("h323");
  if (h323Pos < defProtoPos) {
    defProto = "h323";
    defProtoPos = h323Pos;
  }
#endif

#if OPAL_SIP
  PINDEX sipPos = options.Find("sip");
  if (sipPos < defProtoPos) {
    defProto = "sip";
    defProtoPos = sipPos;
  }
#endif

#if OPAL_IAX2
  PINDEX iaxPos = options.Find("iax2");
  if (iaxPos < defProtoPos) {
    defProto = "iax2:<da>";
    defProtoPos = iaxPos;
  }
#endif

#if OPAL_LID
  PINDEX potsPos = options.Find("pots");
  if (potsPos < defUserPos) {
    defUser = "pots:<dn>";
    defUserPos = potsPos;
  }

  PINDEX pstnPos = options.Find("pstn");
  if (pstnPos < defProtoPos) {
    defProto = "pstn:<dn>";
    defProtoPos = pstnPos;
  }
#endif

  PINDEX pcPos = options.Find("pc");
  if (pcPos < defUserPos) {
    defUser = "pc:<du>";
    defUserPos = pcPos;
  }


#if OPAL_IVR
  if (options.Find("ivr") != P_MAX_INDEX) {
    new OpalIVREndPoint(*this);
    AddRouteEntry(".*:#=ivr:"); // A hash from anywhere goes to IVR
  }
#endif

#if OPAL_H323
  if (h323Pos != P_MAX_INDEX) {
    new H323EndPoint(*this);
    AddRouteEntry("h323:.*=" + defUser);
  }
#endif

#if OPAL_SIP
  if (sipPos != P_MAX_INDEX) {
    new SIPEndPoint_C(*this);
    AddRouteEntry("sip:.*=" + defUser);
  }
#endif

#if OPAL_IAX2
  if (options.Find("iax2") != P_MAX_INDEX) {
    new IAX2EndPoint(*this);
    AddRouteEntry("iax2:.*=" + defUser);
  }
#endif

#if OPAL_LID
  if (potsPos != P_MAX_INDEX || pstnPos != P_MAX_INDEX) {
    new OpalLineEndPoint(*this);

    if (potsPos != P_MAX_INDEX)
      AddRouteEntry("pots:.*=" + defProto + ":<da>");
    if (pstnPos != P_MAX_INDEX)
      AddRouteEntry("pstn:.*=" + defUser + ":<da>");
  }
#endif

  if (pcPos != P_MAX_INDEX) {
    pcssEP = new OpalPCSSEndPoint_C(*this);
    AddRouteEntry("pc:.*=" + defProto + ":<da>");
  }

  return true;
}


void OpalManager_C::PostMessage(OpalMessageBuffer & message)
{
  m_messageMutex.Wait();
  m_messageQueue.push(message.Detach());
  m_messageMutex.Signal();
  m_messagesAvailable.Signal();
}


OpalMessage * OpalManager_C::GetMessage(unsigned timeout)
{
  OpalMessage * msg = NULL;

  if (m_messagesAvailable.Wait(timeout)) {
    m_messageMutex.Wait();

    if (!m_messageQueue.empty()) {
      msg = m_messageQueue.front();
      m_messageQueue.pop();
    }

    m_messageMutex.Signal();
  }

  return msg;
}


OpalMessage * OpalManager_C::SendMessage(const OpalMessage * message)
{
  if (message == NULL)
    return NULL;

  OpalMessageBuffer response(message->m_type);

  switch (message->m_type) {
    case OpalCmdSetGeneralParameters :
      HandleSetGeneral(*message, response);
      break;
    case OpalCmdSetProtocolParameters :
      HandleSetProtocol(*message, response);
      break;
    case OpalCmdRegistration :
      HandleRegistration(*message, response);
      break;
    case OpalCmdSetUpCall :
      HandleSetUpCall(*message, response);
      break;
    case OpalCmdAnswerCall :
      HandleAnswerCall(*message, response);
      break;
    case OpalCmdClearCall :
      HandleClearCall(*message, response);
      break;
    case OpalCmdHoldCall :
      HandleHoldCall(*message, response);
      break;
    case OpalCmdRetrieveCall :
      HandleRetrieveCall(*message, response);
      break;
    case OpalCmdTransferCall :
      HandleTransferCall(*message, response);
      break;
    default :
      return NULL;
  }

  return response.Detach();
}


void OpalManager_C::HandleSetGeneral(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (pcssEP != NULL) {
    SET_MESSAGE_STRING(response, m_param.m_general.m_audioRecordDevice, pcssEP->GetSoundChannelRecordDevice());
    if (!IsNullString(command.m_param.m_general.m_audioRecordDevice))
      pcssEP->SetSoundChannelRecordDevice(command.m_param.m_general.m_audioRecordDevice);

    SET_MESSAGE_STRING(response, m_param.m_general.m_audioPlayerDevice, pcssEP->GetSoundChannelPlayDevice());
    if (!IsNullString(command.m_param.m_general.m_audioPlayerDevice))
      pcssEP->SetSoundChannelPlayDevice(command.m_param.m_general.m_audioPlayerDevice);
  }

#if OPAL_VIDEO
  PVideoDevice::OpenArgs video = GetVideoInputDevice();
  SET_MESSAGE_STRING(response, m_param.m_general.m_videoInputDevice, video.deviceName);
  if (!IsNullString(command.m_param.m_general.m_videoInputDevice)) {
    video.deviceName = command.m_param.m_general.m_videoInputDevice;
    SetVideoInputDevice(video);
  }

  video = GetVideoOutputDevice();
  SET_MESSAGE_STRING(response, m_param.m_general.m_videoOutputDevice, video.deviceName);
  if (!IsNullString(command.m_param.m_general.m_videoOutputDevice)) {
    video.deviceName = command.m_param.m_general.m_videoOutputDevice;
    SetVideoOutputDevice(video);
  }

  video = GetVideoPreviewDevice();
  SET_MESSAGE_STRING(response, m_param.m_general.m_videoPreviewDevice, video.deviceName);
  if (!IsNullString(command.m_param.m_general.m_videoPreviewDevice)) {
    video.deviceName = command.m_param.m_general.m_videoPreviewDevice;
    SetVideoPreviewDevice(video);
  }
#endif

  PStringStream strm;
  strm << setfill('\n') << GetMediaFormatOrder();
  SET_MESSAGE_STRING(response, m_param.m_general.m_mediaOrder, strm);
  if (!IsNullString(command.m_param.m_general.m_mediaOrder))
    SetMediaFormatOrder(PString(command.m_param.m_general.m_mediaOrder).Lines());

  strm.flush();
  strm << setfill('\n') << GetMediaFormatMask();
  SET_MESSAGE_STRING(response, m_param.m_general.m_mediaMask, strm);
  if (!IsNullString(command.m_param.m_general.m_mediaMask))
    SetMediaFormatMask(PString(command.m_param.m_general.m_mediaMask).Lines());

  strm = "audio";
#if OPAL_VIDEO
  if (CanAutoStartReceiveVideo())
    strm << " video";
  if (!IsNullString(command.m_param.m_general.m_autoRxMedia))
    SetAutoStartReceiveVideo(strstr(command.m_param.m_general.m_autoRxMedia, "video") != NULL);
#endif
  SET_MESSAGE_STRING(response, m_param.m_general.m_autoRxMedia, strm);

  strm = "audio";
#if OPAL_VIDEO
  if (CanAutoStartTransmitVideo())
    strm << " video";
  if (!IsNullString(command.m_param.m_general.m_autoTxMedia))
    SetAutoStartTransmitVideo(strstr(command.m_param.m_general.m_autoTxMedia, "video") != NULL);
#endif
  SET_MESSAGE_STRING(response, m_param.m_general.m_autoTxMedia, strm);

  SET_MESSAGE_STRING(response, m_param.m_general.m_natRouter, GetTranslationHost());
  if (!IsNullString(command.m_param.m_general.m_natRouter)) {
    if (!SetTranslationHost(command.m_param.m_general.m_natRouter)) {
      response.SetError("Could not set NAT router address.");
      return;
    }
  }

  SET_MESSAGE_STRING(response, m_param.m_general.m_stunServer, GetSTUNServer());
  if (!IsNullString(command.m_param.m_general.m_stunServer)) {
    if (!SetSTUNServer(command.m_param.m_general.m_stunServer)) {
      response.SetError("Could not set STUN server address.");
      return;
    }
  }

  response->m_param.m_general.m_tcpPortBase = GetTCPPortBase();
  response->m_param.m_general.m_tcpPortMax = GetTCPPortMax();
  if (command.m_param.m_general.m_tcpPortBase != 0)
    SetTCPPorts(command.m_param.m_general.m_tcpPortBase, command.m_param.m_general.m_tcpPortMax);

  response->m_param.m_general.m_udpPortBase = GetUDPPortBase();
  response->m_param.m_general.m_udpPortMax = GetUDPPortMax();
  if (command.m_param.m_general.m_udpPortBase != 0)
    SetUDPPorts(command.m_param.m_general.m_udpPortBase, command.m_param.m_general.m_udpPortMax);

  response->m_param.m_general.m_rtpPortBase = GetRtpIpPortBase();
  response->m_param.m_general.m_rtpPortMax = GetRtpIpPortMax();
  if (command.m_param.m_general.m_rtpPortBase != 0)
    SetRtpIpPorts(command.m_param.m_general.m_rtpPortBase, command.m_param.m_general.m_rtpPortMax);

  response->m_param.m_general.m_rtpTypeOfService = GetRtpIpTypeofService();
  if (command.m_param.m_general.m_rtpTypeOfService != 0)
    SetRtpIpTypeofService(command.m_param.m_general.m_rtpTypeOfService);

  response->m_param.m_general.m_rtpMaxPayloadSize = GetMaxRtpPayloadSize();
  if (command.m_param.m_general.m_rtpMaxPayloadSize != 0)
    SetMaxRtpPayloadSize(command.m_param.m_general.m_rtpMaxPayloadSize);

  response->m_param.m_general.m_minAudioJitter = GetMinAudioJitterDelay();
  response->m_param.m_general.m_maxAudioJitter = GetMaxAudioJitterDelay();
  if (command.m_param.m_general.m_minAudioJitter != 0 && command.m_param.m_general.m_maxAudioJitter != 0)
    SetAudioJitterDelay(command.m_param.m_general.m_minAudioJitter, command.m_param.m_general.m_maxAudioJitter);

  if (m_apiVersion < 2)
    return;

  OpalSilenceDetector::Params silenceDetectParams = GetSilenceDetectParams();
  response->m_param.m_general.m_silenceDetectMode = silenceDetectParams.m_mode+1;
  if (command.m_param.m_general.m_silenceDetectMode != 0)
    silenceDetectParams.m_mode = (OpalSilenceDetector::Mode)(command.m_param.m_general.m_silenceDetectMode-1);
  response->m_param.m_general.m_silenceThreshold = silenceDetectParams.m_threshold;
  if (command.m_param.m_general.m_silenceThreshold != 0)
    silenceDetectParams.m_threshold = command.m_param.m_general.m_silenceThreshold;
  response->m_param.m_general.m_signalDeadband = silenceDetectParams.m_signalDeadband;
  if (command.m_param.m_general.m_signalDeadband != 0)
    silenceDetectParams.m_signalDeadband = command.m_param.m_general.m_signalDeadband;
  response->m_param.m_general.m_silenceDeadband = silenceDetectParams.m_silenceDeadband;
  if (command.m_param.m_general.m_silenceDeadband != 0)
    silenceDetectParams.m_silenceDeadband = command.m_param.m_general.m_silenceDeadband;
  response->m_param.m_general.m_silenceAdaptPeriod = silenceDetectParams.m_adaptivePeriod;
  if (command.m_param.m_general.m_silenceAdaptPeriod != 0)
    silenceDetectParams.m_adaptivePeriod = command.m_param.m_general.m_silenceAdaptPeriod;
  SetSilenceDetectParams(silenceDetectParams);

  OpalEchoCanceler::Params echoCancelParams = GetEchoCancelParams();
  response->m_param.m_general.m_echoCancellation = echoCancelParams.m_mode+1;
  if (command.m_param.m_general.m_echoCancellation != 0)
    echoCancelParams.m_mode = (OpalEchoCanceler::Mode)(command.m_param.m_general.m_echoCancellation-1);
  SetEchoCancelParams(echoCancelParams);

  if (m_apiVersion < 3)
    return;

  response->m_param.m_general.m_audioBuffers = pcssEP->GetSoundChannelBufferDepth();
  if (command.m_param.m_general.m_audioBuffers != 0)
    pcssEP->SetSoundChannelBufferDepth(command.m_param.m_general.m_audioBuffers);
}


void FillOpalProductInfo(const OpalMessage & command, OpalMessageBuffer & response, OpalProductInfo & info)
{
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_vendor,  info.vendor);
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_name,    info.name);
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_version, info.version);

  response->m_param.m_protocol.m_t35CountryCode   = info.t35CountryCode;
  response->m_param.m_protocol.m_t35Extension     = info.t35Extension;
  response->m_param.m_protocol.m_manufacturerCode = info.manufacturerCode;

  if (!IsNullString(command.m_param.m_protocol.m_vendor))
    info.vendor = command.m_param.m_protocol.m_vendor;

  if (!IsNullString(command.m_param.m_protocol.m_name))
    info.name = command.m_param.m_protocol.m_name;

  if (!IsNullString(command.m_param.m_protocol.m_version))
    info.version = command.m_param.m_protocol.m_version;

  if (command.m_param.m_protocol.m_t35CountryCode != 0 && command.m_param.m_protocol.m_manufacturerCode != 0) {
    info.t35CountryCode   = command.m_param.m_protocol.m_t35CountryCode;
    info.t35Extension     = command.m_param.m_protocol.m_t35Extension;
    info.manufacturerCode = command.m_param.m_protocol.m_manufacturerCode;
  }
}


static void StartStopListeners(OpalEndPoint * ep, const PString & interfaces, OpalMessageBuffer & response)
{
  if (ep == NULL)
    return;

  if (interfaces.IsEmpty())
    ep->RemoveListener(NULL);
  else {
    PStringArray interfaceArray;
    if (interfaces != "*")
      interfaceArray = interfaces.Lines();
    if (!ep->StartListeners(interfaceArray))
      response.SetError("Could not start listener(s).");
  }
}


void OpalManager_C::HandleSetProtocol(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_protocol.m_prefix)) {
    SET_MESSAGE_STRING(response, m_param.m_protocol.m_userName, GetDefaultUserName());
    if (!IsNullString(command.m_param.m_protocol.m_userName))
      SetDefaultUserName(command.m_param.m_protocol.m_userName);

    SET_MESSAGE_STRING(response, m_param.m_protocol.m_displayName, GetDefaultUserName());
    if (!IsNullString(command.m_param.m_protocol.m_displayName))
      SetDefaultDisplayName(command.m_param.m_protocol.m_displayName);

    OpalProductInfo product = GetProductInfo();
    FillOpalProductInfo(command, response, product);
    SetProductInfo(product);

    if (command.m_param.m_protocol.m_interfaceAddresses != NULL) {
#if OPAL_H323
      StartStopListeners(FindEndPoint(OPAL_PREFIX_H323), command.m_param.m_protocol.m_interfaceAddresses, response);
#endif
#if OPAL_SIP
      StartStopListeners(FindEndPoint(OPAL_PREFIX_SIP),  command.m_param.m_protocol.m_interfaceAddresses, response);
#endif
#if OPAL_IAX2
      StartStopListeners(FindEndPoint(OPAL_PREFIX_IAX2),  command.m_param.m_protocol.m_interfaceAddresses, response);
#endif
    }

    return;
  }

  OpalEndPoint * ep = FindEndPoint(command.m_param.m_protocol.m_prefix);
  if (ep == NULL) {
    response.SetError("No such protocol prefix");
    return;
  }

  SET_MESSAGE_STRING(response, m_param.m_protocol.m_userName, ep->GetDefaultLocalPartyName());
  if (!IsNullString(command.m_param.m_protocol.m_userName))
    ep->SetDefaultLocalPartyName(command.m_param.m_protocol.m_userName);

  SET_MESSAGE_STRING(response, m_param.m_protocol.m_displayName, ep->GetDefaultDisplayName());
  if (!IsNullString(command.m_param.m_protocol.m_displayName))
    ep->SetDefaultDisplayName(command.m_param.m_protocol.m_displayName);

  OpalProductInfo product = ep->GetProductInfo();
  FillOpalProductInfo(command, response, product);
  ep->SetProductInfo(product);

  if (command.m_param.m_protocol.m_interfaceAddresses != NULL)
    StartStopListeners(ep, command.m_param.m_protocol.m_interfaceAddresses, response);
}


void OpalManager_C::HandleRegistration(const OpalMessage & command, OpalMessageBuffer & response)
{
  OpalEndPoint * ep = FindEndPoint(command.m_param.m_registrationInfo.m_protocol);
  if (ep == NULL) {
    response.SetError("No such protocol prefix");
    return;
  }

#if OPAL_H323
  H323EndPoint * h323 = dynamic_cast<H323EndPoint *>(ep);
  if (h323 != NULL) {
    if (command.m_param.m_registrationInfo.m_timeToLive == 0) {
      if (!h323->RemoveGatekeeper())
        response.SetError("Failed to initiate H.323 gatekeeper unregistration.");
    }
    else {
      if (!IsNullString(command.m_param.m_registrationInfo.m_identifier))
        h323->AddAliasName(command.m_param.m_registrationInfo.m_identifier);
      h323->SetGatekeeperPassword(command.m_param.m_registrationInfo.m_password, command.m_param.m_registrationInfo.m_authUserName);
      if (!h323->UseGatekeeper(command.m_param.m_registrationInfo.m_hostName, command.m_param.m_registrationInfo.m_adminEntity))
        response.SetError("Failed to initiate H.323 gatekeeper registration.");
    }
    return;
  }
#endif

#if OPAL_SIP
  SIPEndPoint * sip = dynamic_cast<SIPEndPoint *>(ep);
  if (sip != NULL) {
    if (IsNullString(command.m_param.m_registrationInfo.m_hostName) &&
          (IsNullString(command.m_param.m_registrationInfo.m_identifier) ||
                 strchr(command.m_param.m_registrationInfo.m_identifier, '@') == NULL)) {
      response.SetError("No domain specified for SIP registration.");
      return;
    }

    PStringStream aor;
    PString host;
    if (IsNullString(command.m_param.m_registrationInfo.m_identifier))
      aor << GetDefaultUserName() << '@' << command.m_param.m_registrationInfo.m_hostName;
    else {
      aor << command.m_param.m_registrationInfo.m_identifier;
      if (strchr(command.m_param.m_registrationInfo.m_identifier, '@') != NULL)
        host = command.m_param.m_registrationInfo.m_hostName;
      else
        aor << '@' << command.m_param.m_registrationInfo.m_hostName;
    }

    if (command.m_param.m_registrationInfo.m_timeToLive == 0) {
      if (!sip->Unregister(aor))
        response.SetError("Failed to initiate SIP unregistration.");
    }
    else {
      SIPRegister::Params params;
      params.m_addressOfRecord = aor;
      params.m_contactAddress = host;
      params.m_authID = command.m_param.m_registrationInfo.m_authUserName;
      if (params.m_authID.IsEmpty())
        params.m_authID = params.m_addressOfRecord.Left(params.m_addressOfRecord.Find('@'));
      params.m_password = command.m_param.m_registrationInfo.m_password;
      params.m_realm = command.m_param.m_registrationInfo.m_adminEntity;
      params.m_expire = command.m_param.m_registrationInfo.m_timeToLive;
      if (!sip->Register(params))
        response.SetError("Failed to initiate SIP registration.");
    }
    return;
  }
#endif

  response.SetError("Protocol prefix does not support registration.");
}


void OpalManager_C::HandleSetUpCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callSetUp.m_partyB)) {
    response.SetError("No destination address provided.");
    return;
  }

  PString partyA = command.m_param.m_callSetUp.m_partyA;
  if (partyA.IsEmpty())
    partyA = "pc:*";

  PString token;
  if (SetUpCall(partyA, command.m_param.m_callSetUp.m_partyB, token)) {
    SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_partyA, partyA);
    SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_partyB, command.m_param.m_callSetUp.m_partyB);
    SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_callToken, token);
  }
  else
    response.SetError("Call set up failed.");
}


void OpalManager_C::HandleAnswerCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  if (pcssEP == NULL) {
    response.SetError("Can only answer calls to PC.");
    return;
  }

  pcssEP->AcceptIncomingConnection(command.m_param.m_callToken);
}


void OpalManager_C::HandleClearCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  ClearCall(command.m_param.m_callToken);
}


void OpalManager_C::HandleHoldCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  PSafePtr<OpalCall> call = FindCallWithLock(command.m_param.m_callToken);
  if (call == NULL) {
    response.SetError("No call by the token provided.");
    return;
  }

  if (call->IsOnHold()) {
    response.SetError("Call is already on hold.");
    return;
  }

  call->Hold();
}


void OpalManager_C::HandleRetrieveCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  PSafePtr<OpalCall> call = FindCallWithLock(command.m_param.m_callToken);
  if (call == NULL) {
    response.SetError("No call by the token provided.");
    return;
  }

  if (!call->IsOnHold()) {
    response.SetError("Call is not on hold.");
    return;
  }

  call->Retrieve();
}


void OpalManager_C::HandleTransferCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callSetUp.m_callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  if (IsNullString(command.m_param.m_callSetUp.m_partyB)) {
    response.SetError("No destination address provided.");
    return;
  }

  PSafePtr<OpalCall> call = FindCallWithLock(command.m_param.m_callSetUp.m_callToken);
  if (call == NULL) {
    response.SetError("No call available by the token provided.");
    return;
  }

  PSafePtr<OpalConnection> connection = call->GetConnection(0, PSafeReadOnly);
  if (IsNullString(command.m_param.m_callSetUp.m_partyA)) {
    PString url = connection->GetLocalPartyURL();
    if (url.NumCompare("pc") == EqualTo || url.NumCompare("pots") == EqualTo)
      ++connection;
  }
  else {
    do {
      if (connection->GetLocalPartyURL() == command.m_param.m_callSetUp.m_partyA)
        break;
      ++connection;
    } while (connection != NULL);
  }

  if (connection == NULL) {
    response.SetError("Call does not have suitable connection to transfer.");
    return;
  }

  connection->TransferConnection(command.m_param.m_callSetUp.m_partyB);
}


void OpalManager_C::OnEstablishedCall(OpalCall & call)
{
  OpalMessageBuffer message(OpalIndEstablished);
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyA, call.GetPartyA());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyB, call.GetPartyB());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_callToken, call.GetToken());
  PostMessage(message);
}


void OpalManager_C::OnUserInputString(OpalConnection & connection, const PString & value)
{
  OpalMessageBuffer message(OpalIndUserInput);
  SET_MESSAGE_STRING(message, m_param.m_userInput.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_userInput.m_userInput, value);
  message->m_param.m_userInput.m_duration = 0;
  PostMessage(message);

  OpalManager::OnUserInputString(connection, value);
}


void OpalManager_C::OnUserInputTone(OpalConnection & connection, char tone, int duration)
{
  char input[2];
  input[0] = tone;
  input[1] = '\0';

  OpalMessageBuffer message(OpalIndUserInput);
  SET_MESSAGE_STRING(message, m_param.m_userInput.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_userInput.m_userInput, input);
  message->m_param.m_userInput.m_duration = duration;
  PostMessage(message);

  OpalManager::OnUserInputTone(connection, tone, duration);
}


void OpalManager_C::OnClearedCall(OpalCall & call)
{
  OpalMessageBuffer message(OpalIndCallCleared);
  SET_MESSAGE_STRING(message, m_param.m_callCleared.m_callToken, call.GetToken());
  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByRemoteUser :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Remote has cleared the call");
      break;
    case OpalConnection::EndedByCallerAbort :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Remote has stopped calling");
      break;
    case OpalConnection::EndedByRefusal :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Remote did not accept your call");
      break;
    case OpalConnection::EndedByNoAnswer :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Remote did not answer your call");
      break;
    case OpalConnection::EndedByTransportFail :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Call ended abnormally");
      break;
    case OpalConnection::EndedByCapabilityExchange :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Call ended with no common codec");
      break;
    case OpalConnection::EndedByNoAccept :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Did not accept incoming call");
      break;
    case OpalConnection::EndedByAnswerDenied :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Refused incoming call");
      break;
    case OpalConnection::EndedByNoUser :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Gatekeeper or registrar could not find user");
      break;
    case OpalConnection::EndedByNoBandwidth :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Call aborted, insufficient bandwidth");
      break;
    case OpalConnection::EndedByUnreachable :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Remote could not be reached");
      break;
    case OpalConnection::EndedByNoEndPoint :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "No phone running at remote");
      break;
    case OpalConnection::EndedByHostOffline :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Remote is not online");
      break;
    case OpalConnection::EndedByConnectFail :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Transport error");
      break;
    default :
      SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, "Call completed");
  }
  PostMessage(message);

  OpalManager::OnClearedCall(call);
}


///////////////////////////////////////////////////////////////////////////////

extern "C" {

  OpalHandle OPAL_EXPORT OpalInitialise(unsigned * version, const char * options)
  {
    PCaselessString optionsString = IsNullString(options) ? "pcss h323 sip iax2 pots pstn ivr" : options;

    unsigned callerVersion = 1;
    if (version != NULL) {
      callerVersion = *version;
      if (callerVersion > OPAL_C_API_VERSION)
        *version = OPAL_C_API_VERSION;
    }

    OpalHandle opal = new OpalHandleStruct(callerVersion, optionsString);
    if (opal->manager.Initialise(optionsString))
      return opal;

    delete opal;
    return NULL;
  }


  void OPAL_EXPORT OpalShutDown(OpalHandle handle)
  {
    delete handle;
  }


  OpalMessage * OPAL_EXPORT OpalGetMessage(OpalHandle handle, unsigned timeout)
  {
    return handle == NULL ? NULL : handle->manager.GetMessage(timeout);
  }


  OpalMessage * OPAL_EXPORT OpalSendMessage(OpalHandle handle, const OpalMessage * message)
  {
    return handle == NULL ? NULL : handle->manager.SendMessage(message);
  }


  void OPAL_EXPORT OpalFreeMessage(OpalMessage * message)
  {
    if (message != NULL)
      free(message);
  }

}; // extern "C"


///////////////////////////////////////////////////////////////////////////////
