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
 */

#include <ptlib.h>

#include <opal_config.h>

#include <opal.h>
#include <opal/manager.h>

#include <ep/pcss.h>
#include <ep/localep.h>
#include <h323/h323ep.h>
#include <sip/sipep.h>
#include <sip/sippres.h>
#include <iax2/iax2ep.h>
#include <lids/lidep.h>
#include <t38/t38proto.h>
#include <ep/ivr.h>
#include <ep/opalmixer.h>
#include <im/im_ep.h>
#include <ep/GstEndPoint.h>
#include <ep/skinnyep.h>
#include <ep/lyncep.h>

#include <queue>


class OpalManager_C;


#define PTraceModule() "Opal C"

static PConstString const LocalPrefixes[] = {
  OPAL_PREFIX_PCSS,
  OPAL_PREFIX_GST,
  OPAL_PREFIX_POTS,
  OPAL_PREFIX_FAX,
  OPAL_PREFIX_T38,
  OPAL_PREFIX_IVR,
  OPAL_PREFIX_MIXER,
  OPAL_PREFIX_IM,
  OPAL_PREFIX_LOCAL
};


ostream & operator<<(ostream & strm, OpalMessageType type)
{
  static const char * const Types[] = {
    "IndCommandError",
    "CmdSetGeneralParameters",
    "CmdSetProtocolParameters",
    "CmdRegistration",
    "IndRegistration",
    "CmdSetUpCall",
    "IndIncomingCall",
    "CmdAnswerCall",
    "CmdClearCall",
    "IndAlerting",
    "IndEstablished",
    "IndUserInput",
    "IndCallCleared",
    "CmdHoldCall",
    "CmdRetrieveCall",
    "CmdTransferCall",
    "CmdUserInput",
    "IndMessageWaiting",
    "IndMediaStream",
    "CmdMediaStream",
    "CmdSetUserData",
    "IndLineAppearance",
    "CmdStartRecording",
    "CmdStopRecording",
    "IndProceeding",
    "CmdAlerting",
    "IndOnHold",
    "IndOffHold",
    "IndTransferCall",
    "IndCompletedIVR",
    "OpalCmdAuthorisePresence",
    "OpalCmdSubscribePresence",
    "OpalCmdSetLocalPresence",
    "OpalIndPresenceChange",
    "OpalCmdSendIM",
    "OpalIndReceiveIM",
    "OpalIndSentIM"
  };
  if (type >= 0 && type < PARRAYSIZE(Types))
    strm << Types[type];
  else
    strm << '<' << (unsigned)type << '>';
  return strm;
}


ostream & operator<<(ostream & strm, OpalRegistrationStates state)
{
  static const char * States[] = { "Successful", "Removed", "Failed", "Retrying", "Restored" };
  return strm << States[state];
}


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
    operator OpalMessage *() const   { return  (OpalMessage *)m_data; }

    void SetString(const char * * variable, const char * value);
    void SetData(const char * * variable, const void * value, size_t len);
    void SetMIME(unsigned & length, const OpalMIME * & variable, const PMultiPartList & mime);
    void SetError(const char * errorText);

    OpalMessage * Detach();

  private:
    size_t m_size;
    char * m_data;
    std::vector<size_t> m_strPtrOffset;
};

#define SET_MESSAGE_STRING(msg, member, str) (msg).SetString(&(msg)->member, str)
#define SET_MESSAGE_DATA(msg, member, data, len) (msg).SetData((const char **)&(msg)->member, data, len)


struct OpalMediaDataCallbacks
{
  bool OnReadMediaFrame(const OpalLocalConnection &, const OpalMediaStream & mediaStream, RTP_DataFrame &);
  bool OnWriteMediaFrame(const OpalLocalConnection &, const OpalMediaStream &, RTP_DataFrame & frame);
  bool OnReadMediaData(const OpalLocalConnection &, const OpalMediaStream &, void *, PINDEX, PINDEX &);
  bool OnWriteMediaData(const OpalLocalConnection &, const OpalMediaStream &, const void *, PINDEX, PINDEX &);

  OpalMediaDataFunction m_mediaReadData;
  OpalMediaDataFunction m_mediaWriteData;
  OpalMediaDataType     m_mediaDataHeader;

  OpalMediaDataCallbacks()
    : m_mediaReadData(NULL)
    , m_mediaWriteData(NULL)
    , m_mediaDataHeader(OpalMediaDataPayloadOnly)
  {
  }
};


#if OPAL_HAS_PCSS

class OpalPCSSEndPoint_C : public OpalPCSSEndPoint, public OpalMediaDataCallbacks
{
    PCLASSINFO(OpalPCSSEndPoint_C, OpalPCSSEndPoint);
  public:
    OpalPCSSEndPoint_C(OpalManager_C & manager);

    virtual PBoolean OnShowIncoming(const OpalPCSSConnection &);
    virtual PBoolean OnShowOutgoing(const OpalPCSSConnection &);

    virtual bool OnReadMediaFrame(const OpalLocalConnection &, const OpalMediaStream & mediaStream, RTP_DataFrame &);
    virtual bool OnWriteMediaFrame(const OpalLocalConnection &, const OpalMediaStream &, RTP_DataFrame & frame);
    virtual bool OnReadMediaData(const OpalLocalConnection &, const OpalMediaStream &, void *, PINDEX, PINDEX &);
    virtual bool OnWriteMediaData(const OpalLocalConnection &, const OpalMediaStream &, const void *, PINDEX, PINDEX &);

  protected:
    OpalManager_C & m_manager;
};

#endif // OPAL_HAS_PCSS


class OpalLocalEndPoint_C : public OpalLocalEndPoint, public OpalMediaDataCallbacks
{
    PCLASSINFO(OpalLocalEndPoint_C, OpalLocalEndPoint);
  public:
    OpalLocalEndPoint_C(OpalManager_C & manager);

    virtual bool OnOutgoingCall(const OpalLocalConnection &);
    virtual bool OnIncomingCall(OpalLocalConnection &);

    virtual bool OnReadMediaFrame(const OpalLocalConnection &, const OpalMediaStream & mediaStream, RTP_DataFrame &);
    virtual bool OnWriteMediaFrame(const OpalLocalConnection &, const OpalMediaStream &, RTP_DataFrame & frame);
    virtual bool OnReadMediaData(const OpalLocalConnection &, const OpalMediaStream &, void *, PINDEX, PINDEX &);
    virtual bool OnWriteMediaData(const OpalLocalConnection &, const OpalMediaStream &, const void *, PINDEX, PINDEX &);

  private:
    OpalManager_C & m_manager;
};


#if OPAL_GSTREAMER

class OpalGstEndPoint_C : public GstEndPoint
{
    PCLASSINFO(OpalGstEndPoint_C, GstEndPoint);
  public:
    OpalGstEndPoint_C(OpalManager_C & manager);

    virtual bool OnOutgoingCall(const OpalLocalConnection &);
    virtual bool OnIncomingCall(OpalLocalConnection & connection);

  private:
    OpalManager_C & m_manager;
};

#endif // OPAL_GSTREAMER


#if OPAL_IVR

class OpalIVREndPoint_C : public OpalIVREndPoint
{
    PCLASSINFO(OpalIVREndPoint_C, OpalIVREndPoint);
  public:
    OpalIVREndPoint_C(OpalManager_C & manager);

    virtual bool OnOutgoingCall(const OpalLocalConnection &);
    virtual bool OnIncomingCall(OpalLocalConnection & connection);
    virtual void OnEndDialog(OpalIVRConnection & connection);

  private:
    OpalManager_C & m_manager;
};

#endif // OPAL_IVR


#if OPAL_SIP
class SIPEndPoint_C : public SIPEndPoint
{
    PCLASSINFO(SIPEndPoint_C, SIPEndPoint);
  public:
    SIPEndPoint_C(OpalManager_C & manager);

    virtual void OnRegistrationStatus(
      const RegistrationStatus & status
    );
    virtual void OnSubscriptionStatus(
      const PString & eventPackage, ///< Event package subscribed to
      const SIPURL & uri,           ///< Target URI for the subscription.
      bool wasSubscribing,          ///< Indication the subscribing or unsubscribing
      bool reSubscribing,           ///< If subscribing then indication was refeshing subscription
      SIP_PDU::StatusCodes reason   ///< Status of subscription
    );
    virtual void OnDialogInfoReceived(
      const SIPDialogNotification & info  ///< Information on dialog state change
    );

  private:
    OpalManager_C & m_manager;
};
#endif


class OpalManager_C : public OpalManager
{
    PCLASSINFO(OpalManager_C, OpalManager);
  public:
    OpalManager_C(
      unsigned version,
      const PArgList & args
    );

    ~OpalManager_C();

    void PostMessage(OpalMessageBuffer & message);
    OpalMessage * GetMessage(unsigned timeout, const char * & error);
    OpalMessage * SendMessage(const OpalMessage * message);

    virtual void OnEstablishedCall(OpalCall & call);
    virtual void OnHold(OpalConnection & connection, bool fromRemote, bool onHold);
    virtual bool OnTransferNotify(OpalConnection &, const PStringToString &);
    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);
    virtual void OnClosedMediaStream(const OpalMediaStream & stream);
    virtual void OnUserInputString(OpalConnection & connection, const PString & value);
    virtual void OnUserInputTone(OpalConnection & connection, char tone, int duration);
    virtual void OnMWIReceived(const PString & party, MessageWaitingType type, const PString & extraInfo);
    virtual void OnProceeding(OpalConnection & conenction);
    virtual void OnClearedCall(OpalCall & call);

#if OPAL_HAS_IM
    virtual void OnMessageReceived(const OpalIM & message);
    virtual void OnMessageDisposition(const OpalIMContext::DispositionInfo &);
    virtual void OnCompositionIndication(const OpalIMContext::CompositionInfo &);
#endif

#if OPAL_HAS_PRESENCE
    PDECLARE_PresenceChangeNotifier(OpalManager_C, OnPresenceChange);
#endif // OPAL_HAS_PRESENCE

    void SendIncomingCallInfo(const OpalConnection & connection);
    void SetOutgoingCallInfo(OpalMessageType type, OpalCall & call);

  protected:
    void HandleSetGeneral       (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleSetProtocol      (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleRegistration     (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleSetUpCall        (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleAlerting         (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleAnswerCall       (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleUserInput        (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleClearCall        (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleHoldCall         (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleRetrieveCall     (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleTransferCall     (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleMediaStream      (const OpalMessage & command, OpalMessageBuffer & response);
    void HandleSetUserData      (const OpalMessage & command, OpalMessageBuffer & response);
    void HandleStartRecording   (const OpalMessage & command, OpalMessageBuffer & response);
    void HandleStopRecording    (const OpalMessage & command, OpalMessageBuffer & response);
    void HandleAuthorisePresence(const OpalMessage & command, OpalMessageBuffer & response);
    void HandleSubscribePresence(const OpalMessage & command, OpalMessageBuffer & response);
    void HandleSetLocalPresence (const OpalMessage & command, OpalMessageBuffer & response);
    void HandleSendIM           (const OpalMessage & command, OpalMessageBuffer & response);

    void OnIndMediaStream(const OpalMediaStream & stream, OpalMediaStates state);

    bool FindCall(const char * token, OpalMessageBuffer & response, PSafePtr<OpalCall> & call);

    unsigned                  m_apiVersion;
    bool                      m_manualAlerting;
    PSyncQueue<OpalMessage *> m_messageQueue;
    OpalMessageAvailableFunction m_messageAvailableCallback;
    bool                      m_shuttingDown;

    typedef void (OpalManager_C:: * HandlerFunc)(const OpalMessage &, OpalMessageBuffer &);
    static HandlerFunc m_handlers[OpalMessageTypeCount];
};

OpalManager_C::HandlerFunc OpalManager_C::m_handlers[OpalMessageTypeCount] = {
  NULL,                                     // OpalIndCommandError
  &OpalManager_C::HandleSetGeneral,         // OpalCmdSetGeneralParameters
  &OpalManager_C::HandleSetProtocol,        // OpalCmdSetProtocolParameters
  &OpalManager_C::HandleRegistration,       // OpalCmdRegistration
  NULL,                                     // OpalIndRegistration
  &OpalManager_C::HandleSetUpCall,          // OpalCmdSetUpCall
  NULL,                                     // OpalIndIncomingCall
  &OpalManager_C::HandleAnswerCall,         // OpalCmdAnswerCall
  &OpalManager_C::HandleClearCall,          // OpalCmdClearCall
  NULL,                                     // OpalIndAlerting
  NULL,                                     // OpalIndEstablished
  NULL,                                     // OpalIndUserInput
  NULL,                                     // OpalIndCallCleared
  &OpalManager_C::HandleHoldCall,           // OpalCmdHoldCall
  &OpalManager_C::HandleRetrieveCall,       // OpalCmdRetrieveCall
  &OpalManager_C::HandleTransferCall,       // OpalCmdTransferCall
  &OpalManager_C::HandleUserInput,          // OpalCmdUserInput
  NULL,                                     // OpalIndMessageWaiting
  NULL,                                     // OpalIndMediaStream
  &OpalManager_C::HandleMediaStream,        // OpalCmdMediaStream
  &OpalManager_C::HandleSetUserData,        // OpalCmdSetUserData
  NULL,                                     // OpalIndLineAppearance
  &OpalManager_C::HandleStartRecording,     // OpalCmdStartRecording
  &OpalManager_C::HandleStopRecording,      // OpalCmdStopRecording
  NULL,                                     // OpalIndProceeding
  &OpalManager_C::HandleAlerting,           // OpalCmdAlerting
  NULL,                                     // OpalIndOnHold
  NULL,                                     // OpalIndOffHold
  NULL,                                     // OpalIndTransferCall
  NULL,                                     // OpalIndCompletedIVR
  &OpalManager_C::HandleAuthorisePresence,  // OpalCmdAuthorisePresence
  &OpalManager_C::HandleSubscribePresence,  // OpalCmdSubscribePresence
  &OpalManager_C::HandleSetLocalPresence,   // OpalCmdSetLocalPresence
  NULL,                                     // OpalIndPresenceChange
  &OpalManager_C::HandleSendIM,             // OpalCmdSendIM
  NULL,                                     // OpalIndReceiveIM
};


static PProcess::CodeStatus GetCodeStatus(const PString & str)
{
  static PConstCaselessString alpha("alpha");
  if (alpha.NumCompare(str) == PObject::EqualTo)
    return PProcess::AlphaCode;

  static PConstCaselessString beta("beta");
  if (beta.NumCompare(str) == PObject::EqualTo)
    return PProcess::BetaCode;

  return PProcess::ReleaseCode;
}


struct OpalHandleStruct
{
  OpalHandleStruct(unsigned version, const PArgList & args)
    : m_process(args.GetOptionString("manufacturer", "OPAL VoIP"),
                args.GetOptionString("name", "OPAL"),
                args.GetOptionString("major", "1").AsUnsigned(),
                args.GetOptionString("minor", "0").AsUnsigned(),
                GetCodeStatus(args.GetOptionString("status")),
                args.GetOptionString("build", "0").AsUnsigned(),
                true)
  {
    PTRACE_INITIALISE(args);

#if OPAL_PTLIB_CONFIG_FILE
    if (args.HasOption("config"))
      m_process.SetConfigurationPath(args.GetOptionString("config"));
#endif

    if (args.HasOption("plugin"))
      PPluginManager::GetPluginManager().SetDirectories(args.GetOptionString("plugin").Lines());

    m_process.Startup();

    m_manager = new OpalManager_C(version, args);

    PTRACE(1, "Start Up, OPAL version " << OpalGetVersion());
  }

  ~OpalHandleStruct()
  {
    PTRACE(1, "Shut Down.");
    delete m_manager;
  }

  PLibraryProcess m_process;
  OpalManager_C * m_manager;
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
  SetData(variable, value, strlen(value)+1);
}


void OpalMessageBuffer::SetData(const char * * variable, const void * value, size_t length)
{
  PAssert((char *)variable >= m_data && (char *)variable < m_data+m_size, PInvalidParameter);

  char * newData = (char *)realloc(m_data, m_size + length);
  if (PAssertNULL(newData) != m_data) {
    // Memory has moved, this invalidates pointer variables so recalculate them
    intptr_t delta = newData - m_data;
    char * endData = m_data + m_size;
    for (size_t i = 0; i < m_strPtrOffset.size(); ++i) {
      const char ** ptr = (const char **)(newData + m_strPtrOffset[i]);
      if (*ptr >= m_data && *ptr < endData)
        *ptr += delta;
    }
    variable += delta/sizeof(char *);
    m_data = newData;
  }

  char * stringData = m_data + m_size;
  if (value != NULL)
    memcpy(stringData, value, length);
  else
    memset(stringData, 0, length);
  m_size += length;

  *variable = stringData;

  m_strPtrOffset.push_back((char *)variable - m_data);
}


void OpalMessageBuffer::SetMIME(unsigned & length, const OpalMIME * & variable, const PMultiPartList & mime)
{
  if (mime.IsEmpty())
    return;

  length = mime.size();
  SetData((const char **)&variable, NULL, length*sizeof(OpalMIME));
  size_t offset = m_strPtrOffset.back();
  size_t index = 0;
  for (PMultiPartList::const_iterator it = mime.begin(); it != mime.end(); ++it, ++index) {
    // Recalculate pointers as SetString/SetData can do realloc and they move.
    SetString(&(*reinterpret_cast<OpalMIME **>(m_data+offset))[index].m_type, it->m_mime.GetString(PMIMEInfo::ContentTypeTag));
    OpalMIME & item = (*reinterpret_cast<OpalMIME **>(m_data+offset))[index];
    if (it->m_textBody.IsEmpty()) {
      item.m_length = it->m_binaryBody.GetSize();
      SetData(&item.m_data, it->m_binaryBody, item.m_length);
    }
    else {
      item.m_length = it->m_textBody.GetLength();
      SetData(&item.m_data, it->m_textBody.GetPointer(), item.m_length+1);
    }
  }
}


void OpalMessageBuffer::SetError(const char * errorText)
{
  OpalMessage * message = (OpalMessage *)m_data;
  PTRACE(2, "Command " << message->m_type << " error: " << errorText);

  message->m_type = OpalIndCommandError;
  m_strPtrOffset.clear();
  SetString(&message->m_param.m_commandError, errorText);
}


OpalMessage * OpalMessageBuffer::Detach()
{
  OpalMessage * message = (OpalMessage *)m_data;
  m_data = NULL;
  return message;
}


PString BuildProductName(const OpalProductInfo & info)
{
  if (info.comments.IsEmpty())
    return info.name;
  if (info.comments[0] == '(')
    return info.name + ' ' + info.comments;
  return info.name + " (" + info.comments + ')';
}


///////////////////////////////////////

OpalLocalEndPoint_C::OpalLocalEndPoint_C(OpalManager_C & mgr)
  : OpalLocalEndPoint(mgr)
  , m_manager(mgr)
{
}


bool OpalLocalEndPoint_C::OnOutgoingCall(const OpalLocalConnection & connection)
{
  m_manager.SetOutgoingCallInfo(OpalIndAlerting, connection.GetCall());
  return true;
}


bool OpalLocalEndPoint_C::OnIncomingCall(OpalLocalConnection & connection)
{
  m_manager.SendIncomingCallInfo(connection);
  return true;
}


bool OpalLocalEndPoint_C::OnReadMediaFrame(const OpalLocalConnection & connection,
                                           const OpalMediaStream & mediaStream,
                                           RTP_DataFrame & frame)
{
  return OpalMediaDataCallbacks::OnReadMediaFrame(connection, mediaStream, frame);
}


bool OpalLocalEndPoint_C::OnWriteMediaFrame(const OpalLocalConnection & connection,
                                            const OpalMediaStream & mediaStream,
                                            RTP_DataFrame & frame)
{
  return OpalMediaDataCallbacks::OnWriteMediaFrame(connection, mediaStream, frame);
}


bool OpalLocalEndPoint_C::OnReadMediaData(const OpalLocalConnection & connection,
                                          const OpalMediaStream & mediaStream,
                                          void * data,
                                          PINDEX size,
                                          PINDEX & length)
{
  return OpalMediaDataCallbacks::OnReadMediaData(connection, mediaStream, data, size, length);
}


bool OpalLocalEndPoint_C::OnWriteMediaData(const OpalLocalConnection & connection,
                                           const OpalMediaStream & mediaStream,
                                           const void * data,
                                           PINDEX length,
                                           PINDEX & written)
{
  return OpalMediaDataCallbacks::OnWriteMediaData(connection, mediaStream, data, length, written);
}


bool OpalMediaDataCallbacks::OnReadMediaFrame(const OpalLocalConnection & connection,
                                              const OpalMediaStream & mediaStream,
                                              RTP_DataFrame & frame)
{
  if (m_mediaDataHeader != OpalMediaDataWithHeader) {
    PTRACE(2, "OnReadMediaFrame failed due to illegal OpalMediaDataType.");
    return false;
  }

  if (m_mediaReadData == NULL) {
    PTRACE(2, "OnReadMediaFrame failed due to no call back set.");
    return false;
  }

  int result = m_mediaReadData(connection.GetCall().GetToken(),
                               mediaStream.GetID(),
                               mediaStream.GetMediaFormat().GetName(),
                               connection.GetUserData(),
                               frame.GetPointer(),
                               frame.GetSize());
  if (result < 0)
    return false;

  frame.SetPayloadSize(result-frame.GetHeaderSize());
  return true;
}


bool OpalMediaDataCallbacks::OnWriteMediaFrame(const OpalLocalConnection & connection,
                                               const OpalMediaStream & mediaStream,
                                               RTP_DataFrame & frame)
{
  if (m_mediaDataHeader != OpalMediaDataWithHeader) {
    PTRACE(2, "OnWriteMediaFrame failed due to illegal OpalMediaDataType.");
    return false;
  }

  if (m_mediaWriteData == NULL) {
    PTRACE(2, "OnWriteMediaFrame failed due to no call back set.");
    return false;
  }

  int result = m_mediaWriteData(connection.GetCall().GetToken(),
                                mediaStream.GetID(),
                                mediaStream.GetMediaFormat().GetName(),
                                connection.GetUserData(),
                                frame.GetPointer(),
                                frame.GetPacketSize());
  return result >= 0;
}


bool OpalMediaDataCallbacks::OnReadMediaData(const OpalLocalConnection & connection,
                                          const OpalMediaStream & mediaStream,
                                          void * data,
                                          PINDEX size,
                                          PINDEX & length)
{
  if (m_mediaDataHeader != OpalMediaDataPayloadOnly) {
    PTRACE(2, "OnReadMediaData failed due to illegal OpalMediaDataType.");
    return false;
  }

  if (m_mediaReadData == NULL) {
    PTRACE(2, "OnReadMediaData failed due to no call back set.");
    return false;
  }

  int result = m_mediaReadData(connection.GetCall().GetToken(),
                               mediaStream.GetID(),
                               mediaStream.GetMediaFormat().GetName(),
                               connection.GetUserData(),
                               data,
                               size);
  if (result < 0)
    return false;

  length = result;
  return true;
}


bool OpalMediaDataCallbacks::OnWriteMediaData(const OpalLocalConnection & connection,
                                           const OpalMediaStream & mediaStream,
                                           const void * data,
                                           PINDEX length,
                                           PINDEX & written)
{
  if (m_mediaDataHeader != OpalMediaDataPayloadOnly) {
    PTRACE(2, "OnWriteMediaData failed due to illegal OpalMediaDataType.");
    return false;
  }

  if (m_mediaWriteData == NULL) {
    PTRACE(2, "OnWriteMediaData failed due to no call back set.");
    return false;
  }

  int result = m_mediaWriteData(connection.GetCall().GetToken(),
                                mediaStream.GetID(),
                                mediaStream.GetMediaFormat().GetName(),
                                connection.GetUserData(),
                                (void *)data,
                                length);
  if (result < 0)
    return false;

  written = result;
  return true;
}


///////////////////////////////////////

#if OPAL_HAS_PCSS

OpalPCSSEndPoint_C::OpalPCSSEndPoint_C(OpalManager_C & mgr)
  : OpalPCSSEndPoint(mgr)
  , m_manager(mgr)
{
}


PBoolean OpalPCSSEndPoint_C::OnShowIncoming(const OpalPCSSConnection & connection)
{
  m_manager.SendIncomingCallInfo(connection);
  return true;
}


PBoolean OpalPCSSEndPoint_C::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  m_manager.SetOutgoingCallInfo(OpalIndAlerting, connection.GetCall());
  return true;
}


bool OpalPCSSEndPoint_C::OnReadMediaFrame(const OpalLocalConnection & connection,
                                          const OpalMediaStream & mediaStream,
                                          RTP_DataFrame & frame)
{
  return OpalMediaDataCallbacks::OnReadMediaFrame(connection, mediaStream, frame);
}


bool OpalPCSSEndPoint_C::OnWriteMediaFrame(const OpalLocalConnection & connection,
                                           const OpalMediaStream & mediaStream,
                                           RTP_DataFrame & frame)
{
  return OpalMediaDataCallbacks::OnWriteMediaFrame(connection, mediaStream, frame);
}


bool OpalPCSSEndPoint_C::OnReadMediaData(const OpalLocalConnection & connection,
                                         const OpalMediaStream & mediaStream,
                                         void * data,
                                         PINDEX size,
                                         PINDEX & length)
{
  return OpalMediaDataCallbacks::OnReadMediaData(connection, mediaStream, data, size, length);
}


bool OpalPCSSEndPoint_C::OnWriteMediaData(const OpalLocalConnection & connection,
                                          const OpalMediaStream & mediaStream,
                                          const void * data,
                                          PINDEX length,
                                          PINDEX & written)
{
  return OpalMediaDataCallbacks::OnWriteMediaData(connection, mediaStream, data, length, written);
}

#endif // OPAL_HAS_PCSS


///////////////////////////////////////

#if OPAL_GSTREAMER

OpalGstEndPoint_C::OpalGstEndPoint_C(OpalManager_C & manager)
  : GstEndPoint(manager, OPAL_PREFIX_GST)
  , m_manager(manager)
{
}


bool OpalGstEndPoint_C::OnOutgoingCall(const OpalLocalConnection & connection)
{
  m_manager.SetOutgoingCallInfo(OpalIndAlerting, connection.GetCall());
  return true;
}


bool OpalGstEndPoint_C::OnIncomingCall(OpalLocalConnection & connection)
{
  m_manager.SendIncomingCallInfo(connection);
  return true;
}

#endif


///////////////////////////////////////

#if OPAL_IVR

OpalIVREndPoint_C::OpalIVREndPoint_C(OpalManager_C & manager)
  : OpalIVREndPoint(manager)
  , m_manager(manager)
{
}


bool OpalIVREndPoint_C::OnOutgoingCall(const OpalLocalConnection & connection)
{
  m_manager.SetOutgoingCallInfo(OpalIndAlerting, connection.GetCall());
  return true;
}


bool OpalIVREndPoint_C::OnIncomingCall(OpalLocalConnection & connection)
{
  m_manager.SendIncomingCallInfo(connection);
  return true;
}


void OpalIVREndPoint_C::OnEndDialog(OpalIVRConnection & connection)
{
  PTRACE(4, "OnEndDialog for " << connection);

  // Do not call ancestor and start a long pause, as do not want it to hang up
  connection.TransferConnection("<vxml><form><break time=\"3600s\"/></form></vxml>");

  // Send message to app, which may (or may not) start a new IVR script
  OpalMessageBuffer message(OpalIndCompletedIVR);
  SET_MESSAGE_STRING(message, m_param.m_ivrStatus.m_callToken, connection.GetCall().GetToken());

  PStringStream varStr;
  varStr << connection.GetVXMLSession().GetVariables();
  SET_MESSAGE_STRING(message, m_param.m_ivrStatus.m_variables, varStr);

  m_manager.PostMessage(message);
}

#endif


///////////////////////////////////////

#if OPAL_SIP

SIPEndPoint_C::SIPEndPoint_C(OpalManager_C & mgr)
  : SIPEndPoint(mgr)
  , m_manager(mgr)
{
}


void SIPEndPoint_C::OnRegistrationStatus(const RegistrationStatus & status)
{
  SIPEndPoint::OnRegistrationStatus(status);

  OpalMessageBuffer message(OpalIndRegistration);
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_protocol, OPAL_PREFIX_SIP);
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_serverName, status.m_addressofRecord);

  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_product.m_vendor,  status.m_productInfo.vendor);
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_product.m_name,    BuildProductName(status.m_productInfo));
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_product.m_version, status.m_productInfo.version);

  message->m_param.m_registrationStatus.m_product.m_t35CountryCode   = status.m_productInfo.t35CountryCode;
  message->m_param.m_registrationStatus.m_product.m_t35Extension     = status.m_productInfo.t35Extension;
  message->m_param.m_registrationStatus.m_product.m_manufacturerCode = status.m_productInfo.manufacturerCode;

  if (status.m_reason == SIP_PDU::Information_Trying)
    message->m_param.m_registrationStatus.m_status = OpalRegisterRetrying;
  else if (status.m_reason/100 == 2) {
    if (status.m_wasRegistering)
      message->m_param.m_registrationStatus.m_status = status.m_reRegistering ? OpalRegisterRestored : OpalRegisterSuccessful;
    else
      message->m_param.m_registrationStatus.m_status = OpalRegisterRemoved;
  }
  else {
    PStringStream strm;
    strm << "Error " << status.m_reason << " in SIP ";
    if (!status.m_wasRegistering)
      strm << "un";
    strm << "registration.";
    SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_error, strm);
    message->m_param.m_registrationStatus.m_status = status.m_wasRegistering ? OpalRegisterFailed : OpalRegisterRemoved;
  }
  PTRACE(4, "OnRegistrationStatus " << status.m_addressofRecord << ", status=" << message->m_param.m_registrationStatus.m_status);
  m_manager.PostMessage(message);
}


void SIPEndPoint_C::OnSubscriptionStatus(const PString & eventPackage,
                                         const SIPURL & uri,
                                         bool wasSubscribing,
                                         bool reSubscribing,
                                         SIP_PDU::StatusCodes reason)
{
  SIPEndPoint::OnSubscriptionStatus(eventPackage, uri, wasSubscribing, reSubscribing, reason);

  if (reason == SIP_PDU::Successful_OK && !reSubscribing) {
    if (SIPEventPackage(SIPSubscribe::MessageSummary) == eventPackage) {
      OpalMessageBuffer message(OpalIndMessageWaiting);
      SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_party, uri.AsString());
      SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_extraInfo, wasSubscribing ? "SUBSCRIBED" : "UNSUBSCRIBED");
      PTRACE(4, "OnSubscriptionStatus - MWI: party=\"" << message->m_param.m_messageWaiting.m_party
                                              << "\" info=" << message->m_param.m_messageWaiting.m_extraInfo);
      m_manager.PostMessage(message);
    }
    else if (SIPEventPackage(SIPSubscribe::Dialog) == eventPackage) {
      OpalMessageBuffer message(OpalIndLineAppearance);
      SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_line, uri.AsString());
      message->m_param.m_lineAppearance.m_state = wasSubscribing ? OpalLineSubcribed : OpalLineUnsubcribed;
      PTRACE(4, "OnSubscriptionStatus - LineAppearance: line=\"" << message->m_param.m_lineAppearance.m_line);
      m_manager.PostMessage(message);
    }
  }
}


static PString GetParticipantName(const SIPDialogNotification::Participant & participant)
{
  PStringStream strm;
  strm << '"' << participant.m_display << "\" <" << participant.m_URI << '>';
  return strm;
}


void SIPEndPoint_C::OnDialogInfoReceived(const SIPDialogNotification & info)
{
  SIPEndPoint::OnDialogInfoReceived(info);

  OpalMessageBuffer message(OpalIndLineAppearance);
  SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_line, info.m_entity);
  message->m_param.m_lineAppearance.m_state = (OpalLineAppearanceStates)info.m_state;
  message->m_param.m_lineAppearance.m_appearance = info.m_local.m_appearance;

  if (info.m_initiator) {
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_callId, info.m_callId+";to-tag="+info.m_remote.m_dialogTag+";from-tag="+info.m_local.m_dialogTag);
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_partyA, GetParticipantName(info.m_local));
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_partyB, GetParticipantName(info.m_remote));
  }
  else {
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_callId, info.m_callId+";to-tag="+info.m_local.m_dialogTag+";from-tag="+info.m_remote.m_dialogTag);
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_partyA, GetParticipantName(info.m_remote));
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_partyB, GetParticipantName(info.m_local));
  }

  PTRACE(4, "OnDialogInfoReceived: entity=\"" << message->m_param.m_lineAppearance.m_line
                                          << "\" callId=" << message->m_param.m_lineAppearance.m_callId);
  m_manager.PostMessage(message);
}


#endif

///////////////////////////////////////

static bool CheckProto(const PArgList & args, const char * proto, const char * dest, PString & defName, PINDEX & defPos)
{
  PCaselessString protocol = proto;

  for (PINDEX pos = 0; pos < args.GetCount(); ++pos) {
    if (args[pos] *= proto) {
      if (pos < defPos) {
        defName = PSTRSTRM(proto << ':' << dest);
        defPos = pos;
      }
      return true;
    }
  }

  return false;
}


OpalManager_C::OpalManager_C(unsigned version, const PArgList & args)
  : m_apiVersion(version)
  , m_manualAlerting(false)
  , m_shuttingDown(false)
{
  PString defProto, defUser;
  PINDEX  defProtoPos = P_MAX_INDEX, defUserPos = P_MAX_INDEX;

#if OPAL_H323
  bool hasH323  = CheckProto(args, OPAL_PREFIX_H323,  "<da>", defProto, defProtoPos);
  bool hasH323S = CheckProto(args, OPAL_PREFIX_H323S, "<da>", defProto, defProtoPos);
#endif

#if OPAL_SIP
  bool hasSIP  = CheckProto(args, OPAL_PREFIX_SIP,  "<da>", defProto, defProtoPos);
  bool hasSIPS = CheckProto(args, OPAL_PREFIX_SIPS, "<da>", defProto, defProtoPos);
#endif

#if OPAL_IAX2
  bool hasIAX2 = CheckProto(args, OPAL_PREFIX_IAX2, "<da>", defProto, defProtoPos);
#endif

#if OPAL_SKINNY
  bool hasSkinny = CheckProto(args, OPAL_PREFIX_SKINNY, "<dn>", defProto, defProtoPos);
#endif

#if OPAL_LYNC
  bool hasLync = CheckProto(args, OPAL_PREFIX_LYNC, "<dn>", defProto, defProtoPos);
#endif

#if OPAL_LID
  bool hasPOTS = CheckProto(args, OPAL_PREFIX_POTS, "<dn>", defUser, defUserPos);
  bool hasPSTN = CheckProto(args, OPAL_PREFIX_PSTN, "<dn>", defProto, defProtoPos);
#endif

#if OPAL_FAX
  bool hasFAX = CheckProto(args, OPAL_PREFIX_FAX, "<du>", defUser, defUserPos);
  bool hasT38 = CheckProto(args, OPAL_PREFIX_T38, "<du>", defUser, defUserPos);
#endif

#if OPAL_HAS_PCSS
  bool hasPC = CheckProto(args, OPAL_PREFIX_PCSS, "*", defUser, defUserPos);
#endif

  bool hasLocal = CheckProto(args, OPAL_PREFIX_LOCAL, "<du>", defUser, defUserPos);

#if OPAL_GSTREAMER
  bool hasGStreamer = CheckProto(args, OPAL_PREFIX_GST, "*", defUser, defUserPos);
#endif

#if OPAL_IVR
  bool hasIVR = CheckProto(args, OPAL_PREFIX_IVR, "", defUser, defUserPos);
#endif

#if OPAL_HAS_MIXER
  bool hasMIX = CheckProto(args, OPAL_PREFIX_MIXER, "", defUser, defUserPos);
#endif

  AddRouteEntry(".*\t[0-9]+=tel:<du>");

#if OPAL_H323
  if (hasH323 || hasH323S) {
    new H323EndPoint(*this);
    if (hasH323)
      AddRouteEntry(OPAL_PREFIX_H323":.*=" + defUser);
    else
      DetachEndPoint(OPAL_PREFIX_H323);
    if (hasH323S)
      AddRouteEntry(OPAL_PREFIX_H323S":.*=" + defUser);
    else
      DetachEndPoint(OPAL_PREFIX_H323S);
  }
#endif

#if OPAL_SIP
  if (hasSIP || hasSIPS) {
    new SIPEndPoint_C(*this);
    if (hasSIP)
      AddRouteEntry(OPAL_PREFIX_SIP":.*=" + defUser);
    else
      DetachEndPoint(OPAL_PREFIX_SIP);
    if (hasSIPS)
      AddRouteEntry(OPAL_PREFIX_SIPS":.*=" + defUser);
    else
      DetachEndPoint(OPAL_PREFIX_SIPS);
  }
#endif

#if OPAL_IAX2
  if (hasIAX2) {
    new IAX2EndPoint(*this);
    AddRouteEntry(OPAL_PREFIX_IAX2":.*=" + defUser);
  }
#endif

#if OPAL_SKINNY
  if (hasSkinny) {
    new OpalSkinnyEndPoint(*this);
    AddRouteEntry(OPAL_PREFIX_SKINNY":.*=" + defUser);
  }
#endif

#if OPAL_LYNC
  if (hasLync) {
    new OpalLyncEndPoint(*this);
    AddRouteEntry(OPAL_PREFIX_LYNC":.*=" + defUser);
  }
#endif

#if OPAL_LID
  if (hasPOTS || hasPSTN) {
    new OpalLineEndPoint(*this);

    if (hasPOTS)
      AddRouteEntry(OPAL_PREFIX_POTS":.*=" + defProto);
    if (hasPSTN)
      AddRouteEntry(OPAL_PREFIX_PSTN":.*=" + defUser);
  }
#endif

#if OPAL_FAX
  if (hasFAX || hasT38) {
    new OpalFaxEndPoint(*this);

    if (hasFAX)
      AddRouteEntry(OPAL_PREFIX_FAX":.*=" + defProto);
    if (hasT38)
      AddRouteEntry(OPAL_PREFIX_T38":.*=" + defProto);
  }
#endif

  if (hasLocal) {
    new OpalLocalEndPoint_C(*this);
    AddRouteEntry(OPAL_PREFIX_LOCAL":.*=" + defProto);
  }

#if OPAL_HAS_PCSS
  if (hasPC) {
    new OpalPCSSEndPoint_C(*this);
    AddRouteEntry(OPAL_PREFIX_PCSS":.*=" + defProto);
  }
#endif

#if OPAL_GSTREAMER
  if (hasGStreamer) {
    new OpalGstEndPoint_C(*this);
    AddRouteEntry(OPAL_PREFIX_GST":.*=" + defProto);
  }
#endif

#if OPAL_IVR
  if (hasIVR) {
    new OpalIVREndPoint_C(*this);
    AddRouteEntry(OPAL_PREFIX_IVR":.*=" + defProto);
  }
#endif

#if OPAL_HAS_MIXER
  if (hasMIX) {
    new OpalMixerEndPoint(*this, "mcu");
    AddRouteEntry("mcu:.*=" + defProto);
  }
#endif

#if OPAL_HAS_IM
  if (CheckProto(args, OPAL_PREFIX_IM, "", defUser, defUserPos))
    new OpalIMEndPoint(*this);
#endif

  // Add synonym for tel URI to default protocol
  AttachEndPoint(FindEndPoint(defProto.Left(defProto.Find(':'))), "tel");
}


OpalManager_C::~OpalManager_C()
{
  ShutDownEndpoints();

  m_shuttingDown = true;
  m_messageQueue.Close(true);
}


void OpalManager_C::PostMessage(OpalMessageBuffer & message)
{
  if (m_messageAvailableCallback == NULL || m_messageAvailableCallback(message))
    m_messageQueue.Enqueue(message.Detach());
}


OpalMessage * OpalManager_C::GetMessage(unsigned timeout, const char * & error)
{
  if (m_shuttingDown) {
    error = "System shut down";
    return NULL;
  }

  PTRACE(5, "GetMessage: timeout=" << timeout);
  OpalMessage * msg = NULL;
  if (m_messageQueue.Dequeue(msg, timeout)) {
    PTRACE(4, "Giving message " << msg->m_type << " to application");
    error = "";
  }
  else
    error = "Timeout getting message.";
  return msg;
}


OpalMessage * OpalManager_C::SendMessage(const OpalMessage * message)
{
  if (message == NULL)
    return NULL;

  PTRACE(4, "Handling message " << message->m_type << " from application");

  OpalMessageBuffer response(message->m_type);

  HandlerFunc func = message->m_type < OpalMessageTypeCount ? m_handlers[message->m_type] : NULL;
  if (func == (HandlerFunc)NULL)
    return NULL;

  (this->*func)(*message, response);
  PTRACE(5, "Handled message " << message->m_type << " from application");
  return response.Detach();
}


void OpalManager_C::SendIncomingCallInfo(const OpalConnection & connection)
{
  OpalMessageBuffer message(OpalIndIncomingCall);

  PSafePtr<OpalConnection> network = connection.GetOtherPartyConnection();
  PAssert(network != NULL, PLogicError); // Should not happen!

  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_localAddress, network->GetLocalPartyURL());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_remoteAddress, network->GetRemotePartyURL());
  if (m_apiVersion >= 33)
    SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_remoteIdentity, network->GetRemoteIdentity());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_remotePartyNumber, network->GetRemotePartyNumber());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_remoteDisplayName, network->GetRemotePartyName());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_calledAddress, network->GetCalledPartyURL());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_calledPartyNumber, network->GetCalledPartyNumber());

  if (m_apiVersion >= 22) {
    PString redirect = network->GetRedirectingParty();
    SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_referredByAddress, redirect);
    if (!OpalIsE164(redirect)) {
      redirect = PURL(redirect).GetUserName();
      if (!OpalIsE164(redirect))
        redirect.MakeEmpty();
    }
    SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_redirectingNumber, redirect);
  }

  const OpalProductInfo & info = network->GetRemoteProductInfo();
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_product.m_vendor,  info.vendor);
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_product.m_name,    BuildProductName(info));
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_product.m_version, info.version);

  message->m_param.m_incomingCall.m_product.m_t35CountryCode   = info.t35CountryCode;
  message->m_param.m_incomingCall.m_product.m_t35Extension     = info.t35Extension;
  message->m_param.m_incomingCall.m_product.m_manufacturerCode = info.manufacturerCode;

  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_alertingType,   network->GetAlertingType());
  if (m_apiVersion >= 35)
    SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_supportedFeatures, network->GetSupportedFeatures());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_protocolCallId, connection.GetIdentifier());

  if (m_apiVersion >= 32)
    message.SetMIME(message->m_param.m_incomingCall.m_extraCount,
                    message->m_param.m_incomingCall.m_extras,
                    network->GetExtraCallInfo());

  PTRACE(4, "OpalIndIncomingCall: token=\""  << message->m_param.m_incomingCall.m_callToken << "\"\n"
            "  Local  - URL=\"" << message->m_param.m_incomingCall.m_localAddress << "\"\n"
            "  Remote - URL=\"" << message->m_param.m_incomingCall.m_remoteAddress << "\""
                    " E.164=\"" << message->m_param.m_incomingCall.m_remotePartyNumber << "\""
                  " Display=\"" << message->m_param.m_incomingCall.m_remoteDisplayName << "\"\n"
            "  Dest.  - URL=\"" << message->m_param.m_incomingCall.m_calledAddress << "\""
                    " E.164=\"" << message->m_param.m_incomingCall.m_calledPartyNumber << "\"\n"
            "  AlertingType=\"" << message->m_param.m_incomingCall.m_alertingType << "\"\n"
            "        CallID=\"" << message->m_param.m_incomingCall.m_protocolCallId << '"');

  PostMessage(message);
}


void OpalManager_C::SetOutgoingCallInfo(OpalMessageType type, OpalCall & call)
{
  OpalMessageBuffer message(type);

  PSafePtr<OpalConnection> network = call.GetConnection(1);
  PAssert(network != NULL, PLogicError); // Should not happen!

  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyA, call.GetPartyA());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyB, call.GetPartyB());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_callToken, call.GetToken());

  if (m_apiVersion >= 32)
    message.SetMIME(message->m_param.m_callSetUp.m_extraCount,
                    message->m_param.m_callSetUp.m_extras,
                    network->GetExtraCallInfo());

  PTRACE(4, (type == OpalIndAlerting ? "OnOutgoingCall:" : "OnEstablished:")
         << " token=\"" << message->m_param.m_callSetUp.m_callToken << "\""
            " A=\""     << message->m_param.m_callSetUp.m_partyA << "\""
            " B=\""     << message->m_param.m_callSetUp.m_partyB << '"');

  PostMessage(message);
}


void OpalManager_C::HandleSetGeneral(const OpalMessage & command, OpalMessageBuffer & response)
{
#if OPAL_HAS_PCSS
  OpalPCSSEndPoint_C * pcssEP = FindEndPointAs<OpalPCSSEndPoint_C>(OPAL_PREFIX_PCSS);
  if (pcssEP != NULL) {
    SET_MESSAGE_STRING(response, m_param.m_general.m_audioRecordDevice, pcssEP->GetSoundChannelRecordDevice());
    if (!IsNullString(command.m_param.m_general.m_audioRecordDevice))
      pcssEP->SetSoundChannelRecordDevice(command.m_param.m_general.m_audioRecordDevice);

    SET_MESSAGE_STRING(response, m_param.m_general.m_audioPlayerDevice, pcssEP->GetSoundChannelPlayDevice());
    if (!IsNullString(command.m_param.m_general.m_audioPlayerDevice))
      pcssEP->SetSoundChannelPlayDevice(command.m_param.m_general.m_audioPlayerDevice);

#if OPAL_VIDEO
    PVideoDevice::OpenArgs video = GetVideoInputDevice();
    SET_MESSAGE_STRING(response, m_param.m_general.m_videoInputDevice, video.deviceName);
    if (!IsNullString(command.m_param.m_general.m_videoInputDevice)) {
      video.deviceName = command.m_param.m_general.m_videoInputDevice;
      if (!SetVideoInputDevice(video)) {
        response.SetError("Could not set video input device.");
        return;
      }
    }

    video = GetVideoOutputDevice();
    SET_MESSAGE_STRING(response, m_param.m_general.m_videoOutputDevice, video.deviceName);
    if (!IsNullString(command.m_param.m_general.m_videoOutputDevice)) {
      video.deviceName = command.m_param.m_general.m_videoOutputDevice;
      if (!SetVideoOutputDevice(video)) {
        response.SetError("Could not set video output device.");
        return;
      }
    }

    video = GetVideoPreviewDevice();
    SET_MESSAGE_STRING(response, m_param.m_general.m_videoPreviewDevice, video.deviceName);
    if (!IsNullString(command.m_param.m_general.m_videoPreviewDevice)) {
      video.deviceName = command.m_param.m_general.m_videoPreviewDevice;
      if (!SetVideoPreviewDevice(video)) {
        response.SetError("Could not set video preview device.");
        return;
      }
    }
#endif // OPAL_VIDEO
  }
  else
#endif // OPAL_HAS_PCSS
  {
#if OPAL_GSTREAMER
    OpalGstEndPoint_C * gstEP = FindEndPointAs<OpalGstEndPoint_C>(OPAL_PREFIX_GST);
    if (gstEP != NULL) {
      SET_MESSAGE_STRING(response, m_param.m_general.m_audioRecordDevice, gstEP->GetAudioSourceDevice());
      if (!IsNullString(command.m_param.m_general.m_audioRecordDevice)) {
        if (!gstEP->SetAudioSourceDevice(command.m_param.m_general.m_audioRecordDevice)) {
          response.SetError("Could not set GStreamer audio source.");
          return;
        }
      }

      SET_MESSAGE_STRING(response, m_param.m_general.m_audioPlayerDevice, gstEP->GetAudioSinkDevice());
      if (!IsNullString(command.m_param.m_general.m_audioPlayerDevice)) {
        if (!gstEP->SetAudioSinkDevice(command.m_param.m_general.m_audioPlayerDevice)) {
          response.SetError("Could not set GStreamer audio sink.");
          return;
        }
      }

#if OPAL_VIDEO
      SET_MESSAGE_STRING(response, m_param.m_general.m_videoInputDevice, gstEP->GetVideoSourceDevice());
      if (!IsNullString(command.m_param.m_general.m_videoInputDevice)) {
        if (!gstEP->SetVideoSourceDevice(command.m_param.m_general.m_videoInputDevice)) {
          response.SetError("Could not set GStreamer video source.");
          return;
        }
      }

      SET_MESSAGE_STRING(response, m_param.m_general.m_videoOutputDevice, gstEP->GetVideoSinkDevice());
      if (!IsNullString(command.m_param.m_general.m_videoOutputDevice)) {
        if (!gstEP->SetVideoSinkDevice(command.m_param.m_general.m_videoOutputDevice)) {
          response.SetError("Could not set GStreamer video sink.");
          return;
        }
      }
#endif // OPAL_VIDEO
    }
#endif // OPAL_GSTREAMER
  }

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

  OpalMediaTypeList allMediaTypes = OpalMediaType::GetList();

  for (OpalMediaType::AutoStartMode autoStart = OpalMediaType::Receive; autoStart < OpalMediaType::ReceiveTransmit; ++autoStart) {
    strm.MakeEmpty();

    OpalMediaTypeList::iterator iterMediaType;
    for (iterMediaType = allMediaTypes.begin(); iterMediaType != allMediaTypes.end(); ++iterMediaType) {
      OpalMediaTypeDefinition * definition = OpalMediaType::GetDefinition(*iterMediaType);
      if ((definition->GetAutoStart()&autoStart) != 0) {
        if (!strm.IsEmpty())
          strm << ' ';
        strm << *iterMediaType;
        definition->SetAutoStart(autoStart, false);
      }
    }

    PString autoXxMedia;
    if (autoStart == OpalMediaType::Receive) {
      SET_MESSAGE_STRING(response, m_param.m_general.m_autoRxMedia, strm);
      if (command.m_param.m_general.m_autoRxMedia != NULL)
        autoXxMedia = command.m_param.m_general.m_autoRxMedia;
      else
        autoXxMedia = strm;
    }
    else {
      SET_MESSAGE_STRING(response, m_param.m_general.m_autoTxMedia, strm);
      if (command.m_param.m_general.m_autoTxMedia != NULL)
        autoXxMedia = command.m_param.m_general.m_autoTxMedia;
      else
        autoXxMedia = strm;
    }

    PStringArray enabledMediaTypes = autoXxMedia.Tokenise(" \t\n", false);
    for (PINDEX i = 0; i < enabledMediaTypes.GetSize(); ++i) {
      OpalMediaTypeDefinition * definition = OpalMediaType::GetDefinition(enabledMediaTypes[i]);
      if (definition != NULL)
        definition->SetAutoStart(autoStart, true);
    }

#if PTRACING
    static unsigned const Level = 3;
    if (PTrace::CanTrace(Level)) {
      ostream & trace = PTRACE_BEGIN(Level);
      trace << "Auto start " << (autoStart == OpalMediaType::Receive ? "receive" : "transmit") << ':';
      for (iterMediaType = allMediaTypes.begin(); iterMediaType != allMediaTypes.end(); ++iterMediaType) {
        if (((*iterMediaType)->GetAutoStart()&autoStart) != 0)
          trace << ' ' << *iterMediaType;
      }
      trace << PTrace::End;
    }
#endif
  }

#if OPAL_PTLIB_NAT
  {
    PStringStream natMethods;
    PStringStream natServers;
    for (PNatMethods::iterator it = GetNatMethods().begin(); it != GetNatMethods().end(); ++it) {
      if (it->IsActive()) {
        natMethods << it->GetMethodName() << '\n';
        natServers << it->GetServer() << '\n';
      }
      it->Activate(false);
    }
    SET_MESSAGE_STRING(response, m_param.m_general.m_natMethod, natMethods);
    SET_MESSAGE_STRING(response, m_param.m_general.m_natServer, natServers);
  }

  // Note: in this case there is a difference between NULL and "".
  if (command.m_param.m_general.m_natMethod == NULL) {
#if P_STUN
    if (command.m_param.m_general.m_natServer != NULL) {
      PString server, iface;
      PString(command.m_param.m_general.m_natServer).Split('\t', server, iface, PString::SplitDefaultToBefore);
      if (!SetNATServer(PSTUNClient::MethodName(), server, true, 0, iface)) {
        response.SetError("Error setting STUN server.");
        return;
      }
    }
#endif
  }
  else {
    PIPSocket::Address ip;
    if (GetNatMethods().GetMethodByName(command.m_param.m_general.m_natMethod) == NULL &&
        PIPSocket::GetHostAddress(command.m_param.m_general.m_natMethod, ip))
      SetNATServer(PNatMethod_Fixed::MethodName(), ip.AsString()); // Backward compatibility with earlier API
    else {
      PStringStream error;
      PStringArray natMethods = PString(command.m_param.m_general.m_natMethod).Lines();
      PStringArray natServers = PString(command.m_param.m_general.m_natServer).Lines();
      for (PINDEX methodIndex = 0; methodIndex < natMethods.GetSize(); ++methodIndex) {
        PString server, iface;
        natServers[methodIndex].Split('\t', server, iface, PString::SplitDefaultToBefore);
        if (!SetNATServer(natMethods[methodIndex], server, true, (methodIndex+1)*10, iface)) {
          error << "Error setting NAT method " << natMethods[methodIndex];
          if (!natServers[methodIndex].IsEmpty())
            error << " to server \"" << natServers[methodIndex] << '"';
          error << '\n';
        }
      }
      if (!error.IsEmpty()) {
        response.SetError(error);
        return;
      }
    }
  }
#endif // OPAL_PTLIB_NAT

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

  PIPSocket::QoS qos = GetMediaQoS(OpalMediaType::Audio());
  response->m_param.m_general.m_rtpTypeOfService = qos.m_dscp < 0 ? 0 : (qos.m_dscp<<2);
  if (command.m_param.m_general.m_rtpTypeOfService != 0) {
    qos.m_dscp = (command.m_param.m_general.m_rtpTypeOfService>>2)&0x3f;
    SetMediaQoS(OpalMediaType::Audio(), qos);
  }

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
  response->m_param.m_general.m_silenceDetectMode = (OpalSilenceDetectMode)(silenceDetectParams.m_mode+1);
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

#if OPAL_AEC
  OpalEchoCanceler::Params echoCancelParams = GetEchoCancelParams();
  response->m_param.m_general.m_echoCancellation = echoCancelParams.m_enabled ? OpalEchoCancelEnabled : OpalEchoCancelDisabled;
  switch (command.m_param.m_general.m_echoCancellation) {
    case OpalEchoCancelDisabled :
      echoCancelParams.m_enabled = false;
      break;
    case OpalEchoCancelEnabled :
      echoCancelParams.m_enabled = true;
      break;

    default :
      break;
  }
  SetEchoCancelParams(echoCancelParams);
#endif

  if (m_apiVersion < 3)
    return;

#if OPAL_HAS_PCSS
  if (pcssEP != NULL) {
    response->m_param.m_general.m_audioBuffers = pcssEP->GetSoundChannelBufferDepth();
    if (command.m_param.m_general.m_audioBuffers != 0)
      pcssEP->SetSoundChannelBufferDepth(command.m_param.m_general.m_audioBuffers);
  }
#endif

  if (m_apiVersion < 5)
    return;

  OpalLocalEndPoint_C * localEP = FindEndPointAs<OpalLocalEndPoint_C>(OPAL_PREFIX_LOCAL);
  if (localEP != NULL) {
    response->m_param.m_general.m_mediaReadData = localEP->m_mediaReadData;
    if (command.m_param.m_general.m_mediaReadData != NULL)
      localEP->m_mediaReadData = command.m_param.m_general.m_mediaReadData;

    response->m_param.m_general.m_mediaWriteData = localEP->m_mediaWriteData;
    if (command.m_param.m_general.m_mediaWriteData != NULL)
      localEP->m_mediaWriteData = command.m_param.m_general.m_mediaWriteData;

    response->m_param.m_general.m_mediaDataHeader = localEP->m_mediaDataHeader;
    if (command.m_param.m_general.m_mediaDataHeader != 0)
      localEP->m_mediaDataHeader = command.m_param.m_general.m_mediaDataHeader;

    if (m_apiVersion >= 20) {
      response->m_param.m_general.m_mediaTiming = (OpalMediaTiming)(localEP->GetDefaultAudioSynchronicity()+1);
      if (command.m_param.m_general.m_mediaTiming != 0)
        localEP->SetDefaultAudioSynchronicity((OpalLocalEndPoint::Synchronicity)(command.m_param.m_general.m_mediaTiming-1));
      if (m_apiVersion >= 27) {
        response->m_param.m_general.m_videoSourceTiming = (OpalMediaTiming)(localEP->GetDefaultVideoSourceSynchronicity()+1);
        if (command.m_param.m_general.m_mediaTiming != 0)
          localEP->SetDefaultVideoSourceSynchronicity((OpalLocalEndPoint::Synchronicity)(command.m_param.m_general.m_videoSourceTiming-1));
      }
    }
  }

  if (m_apiVersion < 8)
    return;

  response->m_param.m_general.m_messageAvailable = m_messageAvailableCallback;
  m_messageAvailableCallback = command.m_param.m_general.m_messageAvailable;

  if (m_apiVersion < 14)
    return;

  OpalMediaFormatList allCodecs = OpalMediaFormat::GetAllRegisteredMediaFormats();

  PStringStream mediaOptions;
  for (OpalMediaFormatList::iterator itMediaFormat = allCodecs.begin(); itMediaFormat != allCodecs.end(); ++itMediaFormat) {
    if (itMediaFormat->IsTransportable()) {
      mediaOptions << *itMediaFormat << ":Media Type#" << itMediaFormat->GetMediaType() << '\n';
      for (PINDEX i = 0; i < itMediaFormat->GetOptionCount(); ++i) {
        const OpalMediaOption & option = itMediaFormat->GetOption(i);
        mediaOptions << *itMediaFormat << ':' << option.GetName() << (option.IsReadOnly() ? '#' : '=') << option << '\n';
      }
    }
  }
  SET_MESSAGE_STRING(response, m_param.m_general.m_mediaOptions, mediaOptions);

  PStringArray options = PString(command.m_param.m_general.m_mediaOptions).Lines();
  for (PINDEX i = 0; i < options.GetSize(); ++i) {
    PString optionSpec = options[i];
    PINDEX colon = optionSpec.Find(':');
    PINDEX equal = optionSpec.Find('=', colon);
    PString mediaName = optionSpec.Left(colon);
    PString optionName = optionSpec(colon+1, equal-1);
    PString optionValue = optionSpec.Mid(equal+1);

    if (mediaName.IsEmpty() || optionName.IsEmpty()) {
      PTRACE(2, "Invalid syntax for media format option: \"" << optionSpec << '"');
    }
    else {
      OpalMediaType mediaType = mediaName.ToLower();
      if (mediaType.GetDefinition() != NULL) {
        // Known media type name, change all codecs of that type
        for (OpalMediaFormatList::iterator it = allCodecs.begin(); it != allCodecs.end(); ++it) {
          if (it->IsMediaType(mediaType)) {
            if (it->SetOptionValue(optionName, optionValue)) {
              OpalMediaFormat::SetRegisteredMediaFormat(*it);
              PTRACE(4, "Set " << mediaType << " media format \"" << *it
                     << "\" option \"" << optionName << "\" to \"" << optionValue << '"');
            }
            else {
              PTRACE(2, "Could not set " << mediaType
                     << " media format option \"" << optionName << "\" to \"" << optionValue << '"');
            }
          }
        }
      }
      else {
        OpalMediaFormat mediaFormat = mediaName;
        if (mediaFormat.IsValid()) {
          if (mediaFormat.SetOptionValue(optionName, optionValue)) {
            OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat);
            PTRACE(2, "Set media format \"" << mediaFormat
                   << "\" option \"" << optionName << "\" to \"" << optionValue << '"');
          }
          else {
            PTRACE(2, "Could not set media format \"" << mediaFormat
                   << "\" option \"" << optionName << "\" to \"" << optionValue << '"');
          }
        }
        else {
          PTRACE(2, "Tried to set option for unknown media format: \"" << mediaName << '"');
        }
      }
    }
  }

  if (m_apiVersion < 17)
    return;

#if OPAL_HAS_PCSS
  if (pcssEP != NULL) {
    response->m_param.m_general.m_audioBufferTime = pcssEP->GetSoundChannelBufferTime();
    if (command.m_param.m_general.m_audioBufferTime != 0)
      pcssEP->SetSoundChannelBufferTime(command.m_param.m_general.m_audioBufferTime);
  }
#endif

  if (m_apiVersion < 19)
    return;

  response->m_param.m_general.m_manualAlerting = m_manualAlerting ? 2 : 1;
  if (command.m_param.m_general.m_manualAlerting != 0) {
    m_manualAlerting = command.m_param.m_general.m_manualAlerting != 1;
    for (PINDEX i = 0; i < PARRAYSIZE(LocalPrefixes); ++i) {
      OpalLocalEndPoint * ep = FindEndPointAs<OpalLocalEndPoint>(LocalPrefixes[i]);
      if (ep != NULL)
        ep->SetDeferredAlerting(m_manualAlerting);
    }
  }

  if (m_apiVersion < 30)
    return;

#if OPAL_HAS_PCSS
  if (pcssEP != NULL && !IsNullString(command.m_param.m_general.m_pcssMediaOverride)) {
    PStringArray overrides = PString(command.m_param.m_general.m_pcssMediaOverride).Tokenise(" \t\r\n", false);
    for (PINDEX i = 0; i < overrides.GetSize(); ++i) {
      PCaselessString str = overrides[i];
      bool ok = false;
      if (str.NumCompare("rx-") == EqualTo)
        ok = pcssEP->SetCallbackUsage(str.Mid(3), OpalLocalEndPoint::UseSinkCallback);
      else if (str.NumCompare("tx-") == EqualTo)
        ok = pcssEP->SetCallbackUsage(str.Mid(3), OpalLocalEndPoint::UseSourceCallback);
      if (!ok)
        response.SetError("Invalid PCSS media override.");
    }
  }
#endif

  if (m_apiVersion < 31)
    return;

  if (command.m_param.m_general.m_noMediaTimeout > 0)
    SetNoMediaTimeout(command.m_param.m_general.m_noMediaTimeout);

  if (m_apiVersion < 34)
    return;

#if P_SSL
  SET_MESSAGE_STRING(response, m_param.m_general.m_caFiles, GetSSLCertificateAuthorityFiles());
  SET_MESSAGE_STRING(response, m_param.m_general.m_certificate, GetSSLCertificateFile());
  SET_MESSAGE_STRING(response, m_param.m_general.m_privateKey, GetSSLPrivateKeyFile());
  response->m_param.m_general.m_autoCreateCertificate = m_autoCreateCertificate ? 1 : 2;

  if (command.m_param.m_general.m_caFiles != NULL)
    SetSSLCertificateAuthorityFiles(command.m_param.m_general.m_caFiles);
  if (command.m_param.m_general.m_certificate != NULL)
    SetSSLCertificateFile(command.m_param.m_general.m_certificate);
  if (command.m_param.m_general.m_privateKey != NULL)
    SetSSLPrivateKeyFile(command.m_param.m_general.m_privateKey);
  if (command.m_param.m_general.m_autoCreateCertificate > 0)
    SetSSLAutoCreateCertificate(command.m_param.m_general.m_autoCreateCertificate == 1);
#endif
}


static void FillOpalProductInfo(const OpalMessage & command, OpalMessageBuffer & response, OpalProductInfo & info)
{
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_product.m_vendor,  info.vendor);
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_product.m_name,    BuildProductName(info));
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_product.m_version, info.version);

  response->m_param.m_protocol.m_product.m_t35CountryCode   = info.t35CountryCode;
  response->m_param.m_protocol.m_product.m_t35Extension     = info.t35Extension;
  response->m_param.m_protocol.m_product.m_manufacturerCode = info.manufacturerCode;

  if (command.m_param.m_protocol.m_product.m_vendor != NULL && info.vendor != command.m_param.m_protocol.m_product.m_vendor) {
    info.vendor = command.m_param.m_protocol.m_product.m_vendor;
    PTRACE(4, "Set product vendor to \"" << info.vendor << '"');
  }

  if (command.m_param.m_protocol.m_product.m_name != NULL) {
    PString name, comments;
    PString(command.m_param.m_protocol.m_product.m_name).Split('(', name, comments, PString::SplitDefaultToBefore|PString::SplitTrimBefore);
    if (info.name != name) {
      info.name = name;
      PTRACE(4, "Set product name to \"" << info.name << '"');
    }
    if (info.comments != comments) {
      info.comments = comments;
      PTRACE(4, "Set product comments to \"" << info.comments << '"');
    }
  }

  if (command.m_param.m_protocol.m_product.m_version != NULL && info.version != command.m_param.m_protocol.m_product.m_version) {
    info.version = command.m_param.m_protocol.m_product.m_version;
    PTRACE(4, "Set product version to \"" << info.version << '"');
  }

  if (command.m_param.m_protocol.m_product.m_t35CountryCode != 0 && command.m_param.m_protocol.m_product.m_manufacturerCode != 0) {
    info.t35CountryCode   = (BYTE)command.m_param.m_protocol.m_product.m_t35CountryCode;
    info.t35Extension     = (BYTE)command.m_param.m_protocol.m_product.m_t35Extension;
    info.manufacturerCode = (WORD)command.m_param.m_protocol.m_product.m_manufacturerCode;
  }
}


static void StartStopListeners(OpalEndPoint * ep, const PString & interfaces, OpalMessageBuffer & response)
{
  if (ep == NULL)
    return;

  ep->RemoveListener(NULL);
  if (interfaces.IsEmpty()) {
    PTRACE(4, "Stopped all listeners for " << *ep);
    return;
  }

  PStringArray interfaceArray;
  if (interfaces != "*")
    interfaceArray = interfaces.Lines();

  PTRACE(4, "Starting listeners " << setfill(',') << interfaceArray << " for " << *ep);
  if (!ep->StartListeners(interfaceArray))
    response.SetError(PSTRSTRM("Could not start " << ep->GetPrefixName() << " listener(s) " << interfaces));
}


void OpalManager_C::HandleSetProtocol(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_protocol.m_prefix)) {
    PTRACE(4, "Setting protocol parameters for all protocols");
    SET_MESSAGE_STRING(response, m_param.m_protocol.m_userName, GetDefaultUserName());
    if (command.m_param.m_protocol.m_userName != NULL)
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
    response.SetError(PSTRSTRM("No such protocol prefix as " << command.m_param.m_protocol.m_prefix));
    return;
  }

  PTRACE(4, "Setting protocol parameters for " << *ep);
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_userName, ep->GetDefaultLocalPartyName());
  if (command.m_param.m_protocol.m_userName != NULL)
    ep->SetDefaultLocalPartyName(command.m_param.m_protocol.m_userName);

  SET_MESSAGE_STRING(response, m_param.m_protocol.m_displayName, ep->GetDefaultDisplayName());
  if (!IsNullString(command.m_param.m_protocol.m_displayName))
    ep->SetDefaultDisplayName(command.m_param.m_protocol.m_displayName);

  OpalProductInfo product = ep->GetProductInfo();
  FillOpalProductInfo(command, response, product);
  ep->SetProductInfo(product);

#if OPAL_GSTREAMER
  OpalGstEndPoint_C * gstEP = dynamic_cast<OpalGstEndPoint_C *>(ep);
  if (gstEP != NULL) {
    PStringArray lines = PConstString(command.m_param.m_protocol.m_interfaceAddresses).Lines();
    for (PINDEX line = 0; line < lines.GetSize(); ++line) {
      PStringArray fields = lines[line].Tokenise('\t');
#if OPAL_VIDEO
      if (fields[0] *= "SourceColourConverter") {
        if (fields.GetSize() < 1) {
          response.SetError("Not enough fields in gstreamer source colour converter");
          return;
        }
        if (!gstEP->SetVideoSourceColourConverter(fields[1])) {
          response.SetError("Could not set pipeline elements for source colour converter");
          return;
        }
        continue;
      }

      if (fields[0] *= "SinkColourConverter") {
        if (fields.GetSize() < 1) {
          response.SetError("Not enough fields in gstreamer sink colour converter");
          return;
        }
        if (!gstEP->SetVideoSinkColourConverter(fields[1])) {
          response.SetError("Could not set pipeline elements for sink colour converter");
          return;
        }
        continue;
      }
#endif // OPAL_VIDEO

      if (fields.GetSize() < 3) {
        response.SetError("Not enough fields in gstreamer mapping");
        return;
      }

      OpalMediaFormat mediaFormat = fields[0];
      if (!mediaFormat.IsValid()) {
        response.SetError(PSTRSTRM("No such protocol media format as " << fields[0]));
        return;
      }

      GstEndPoint::CodecPipelines codecPipeline;
      gstEP->GetMapping(mediaFormat, codecPipeline);

      codecPipeline.m_encoder = fields[1];
      codecPipeline.m_decoder = fields[2];
      if (fields.GetSize() > 3)
        codecPipeline.m_packetiser = fields[3];
      if (fields.GetSize() > 4)
        codecPipeline.m_depacketiser = fields[4];
      if (!gstEP->SetMapping(mediaFormat, codecPipeline)) {
        response.SetError(PSTRSTRM("Could not set pipeline elements for media format " << mediaFormat));
        return;
      }
    }
  }
  else
#endif //OPAL_GSTREAMER

#if OPAL_IVR
  if (dynamic_cast<OpalIVREndPoint_C *>(ep) != NULL)
    dynamic_cast<OpalIVREndPoint_C *>(ep)->SetDefaultVXML(command.m_param.m_protocol.m_interfaceAddresses);
  else
#endif // OPAL_IVR

  if (command.m_param.m_protocol.m_interfaceAddresses != NULL)
    StartStopListeners(ep, command.m_param.m_protocol.m_interfaceAddresses, response);

  if (m_apiVersion < 22)
    return;

  unsigned mode = ep->GetSendUserInputMode();
  if (mode != OpalConnection::SendUserInputAsProtocolDefault)
    ++mode;
  else
    mode = OpalUserInputDefault;
  response->m_param.m_protocol.m_userInputMode = (OpalUserInputModes)mode;

  mode = command.m_param.m_protocol.m_userInputMode;
  if (mode != OpalUserInputDefault && mode <= OpalConnection::NumSendUserInputModes)
    --mode;
  else
    mode = OpalConnection::SendUserInputAsProtocolDefault;
  ep->SetSendUserInputMode((OpalConnection::SendUserInputModes)mode);

  if (m_apiVersion < 23)
    return;

  PStringStream strm;
  strm << ep->GetDefaultStringOptions();
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_defaultOptions, strm);
  if (!IsNullString(command.m_param.m_protocol.m_defaultOptions)) {
    OpalConnection::StringOptions newOptions;
    strm = command.m_param.m_protocol.m_defaultOptions;
    strm >> newOptions;
    ep->SetDefaultStringOptions(newOptions);
  }

  if (m_apiVersion < 34)
    return;

  strm.MakeEmpty();
  strm << setfill('\n') << ep->GetMediaCryptoSuites();
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_mediaCryptoSuites, strm);
  if (!IsNullString(command.m_param.m_protocol.m_mediaCryptoSuites))
    ep->SetMediaCryptoSuites(PString(command.m_param.m_protocol.m_mediaCryptoSuites).Lines());

  PStringArray allMediaCryptoSutes = ep->GetAllMediaCryptoSuites();
  strm.MakeEmpty();
  for (PINDEX i = 0; i < allMediaCryptoSutes.GetSize(); ++i) {
    OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(allMediaCryptoSutes[i]);
    strm << cryptoSuite->GetFactoryName() << '=' << cryptoSuite->GetDescription() << 'n';
  }
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_allMediaCryptoSuites, strm);

  if (m_apiVersion < 37)
    return;

  response->m_param.m_protocol.m_maxSizeUDP = ep->GetMaxSizeUDP();
  if (command.m_param.m_protocol.m_maxSizeUDP > 0)
    ep->SetMaxSizeUDP(command.m_param.m_protocol.m_maxSizeUDP);
}


void OpalManager_C::HandleRegistration(const OpalMessage & command, OpalMessageBuffer & response)
{
#if OPAL_HAS_PRESENCE
  static const PConstCaselessString PresPrefix("pres");
  if (PresPrefix == command.m_param.m_registrationInfo.m_protocol) {
    if (IsNullString(command.m_param.m_registrationInfo.m_identifier))
      response.SetError("Must have URI as identifier for presence.");
    else if (command.m_param.m_registrationInfo.m_timeToLive == 0) {
      if (GetPresentity(command.m_param.m_registrationInfo.m_identifier) == NULL)
        response.SetError("URI is not registered for presence.");
      else
        RemovePresentity(command.m_param.m_registrationInfo.m_identifier);
    }
    else {
      PSafePtr<OpalPresentity> presentity = AddPresentity(command.m_param.m_registrationInfo.m_identifier);
      if (presentity == NULL)
        response.SetError("Illegal/unknown URI for presence.");
      else {
        PStringOptions & attr = presentity->GetAttributes();
        attr.Set(OpalPresentity::AuthNameKey, command.m_param.m_registrationInfo.m_authUserName);
        attr.Set(OpalPresentity::AuthPasswordKey, command.m_param.m_registrationInfo.m_password);
        attr.SetInteger(OpalPresentity::TimeToLiveKey, command.m_param.m_registrationInfo.m_timeToLive);
#if OPAL_SIP_PRESENCE
        attr.Set(SIP_Presentity::PresenceAgentKey, command.m_param.m_registrationInfo.m_hostName);
#endif
        if (m_apiVersion >= 28) {
          PStringOptions newAttr(command.m_param.m_registrationInfo.m_attributes);
          for (PStringOptions::iterator it = newAttr.begin(); it != newAttr.end(); ++it)
            attr.Set(it->first, it->second);
        }

        presentity->SetPresenceChangeNotifier(PCREATE_PresenceChangeNotifier(OnPresenceChange));

        if (presentity->Open())
          SET_MESSAGE_STRING(response, m_param.m_registrationInfo.m_identifier, presentity->GetAOR().AsString());
        else
          response.SetError("Could not register URI for presence.");
      }
    }
    return;
  }
#endif // OPAL_HAS_PRESENCE

  OpalEndPoint * ep = FindEndPoint(command.m_param.m_registrationInfo.m_protocol);
  if (ep == NULL) {
    response.SetError(PSTRSTRM("No such protocol prefix as \"" << command.m_param.m_registrationInfo.m_protocol << '"'));
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
#endif // OPAL_H323

#if OPAL_SIP
  SIPEndPoint * sip = dynamic_cast<SIPEndPoint *>(ep);
  if (sip != NULL) {
    if (IsNullString(command.m_param.m_registrationInfo.m_hostName) &&
          (IsNullString(command.m_param.m_registrationInfo.m_identifier) ||
                 strchr(command.m_param.m_registrationInfo.m_identifier, '@') == NULL)) {
      response.SetError("No domain specified for SIP registration.");
      return;
    }

    if (command.m_param.m_registrationInfo.m_timeToLive == 0) {
      if (!sip->Unregister(command.m_param.m_registrationInfo.m_identifier))
        response.SetError("Failed to initiate SIP unregistration.");
    }
    else {
      PString aor;

      if (m_apiVersion < 13 || command.m_param.m_registrationInfo.m_eventPackage == NULL) {
        SIPRegister::Params regParams;
        regParams.m_addressOfRecord = command.m_param.m_registrationInfo.m_identifier;
        regParams.m_registrarAddress = command.m_param.m_registrationInfo.m_hostName;
        regParams.m_authID = command.m_param.m_registrationInfo.m_authUserName;
        regParams.m_password = command.m_param.m_registrationInfo.m_password;
        regParams.m_realm = command.m_param.m_registrationInfo.m_adminEntity;
        regParams.m_expire = command.m_param.m_registrationInfo.m_timeToLive;

        if (m_apiVersion >= 7 && command.m_param.m_registrationInfo.m_restoreTime > 0)
          regParams.m_restoreTime = command.m_param.m_registrationInfo.m_restoreTime;

        if (m_apiVersion >= 28 &&  !IsNullString(command.m_param.m_registrationInfo.m_attributes)) {
          PStringOptions attr(command.m_param.m_registrationInfo.m_attributes);
          PCaselessString compatibility = attr("compatibility");
          if (compatibility == "single" || compatibility == "CannotRegisterMultipleContacts")
            regParams.m_compatibility = SIPRegister::e_CannotRegisterMultipleContacts;
          else if (compatibility == "public" || compatibility == "CannotRegisterPrivateContacts")
            regParams.m_compatibility = SIPRegister::e_CannotRegisterPrivateContacts;
          else if (compatibility == "ALG" || compatibility == "HasApplicationLayerGateway")
            regParams.m_compatibility = SIPRegister::e_HasApplicationLayerGateway;
          else if (compatibility == "RFC5626")
            regParams.m_compatibility = SIPRegister::e_RFC5626;

          regParams.m_proxyAddress = attr("proxy");
          regParams.m_interface = attr("interface");
          regParams.m_instanceId = attr("instance-id");
        }

        if (sip->Register(regParams, aor))
          SET_MESSAGE_STRING(response, m_param.m_registrationInfo.m_identifier, aor);
        else
          response.SetError("Failed to initiate SIP registration.");
      }

      if (m_apiVersion >= 10) {
        SIPSubscribe::Params subParams;
        if (m_apiVersion < 13)
          subParams.m_eventPackage = SIPSubscribe::MessageSummary;

        else {
          if (command.m_param.m_registrationInfo.m_eventPackage == NULL)
            return;
          subParams.m_eventPackage = command.m_param.m_registrationInfo.m_eventPackage;
        }

        subParams.m_addressOfRecord = command.m_param.m_registrationInfo.m_identifier;
        subParams.m_agentAddress = command.m_param.m_registrationInfo.m_hostName;
        subParams.m_authID = command.m_param.m_registrationInfo.m_authUserName;
        subParams.m_password = command.m_param.m_registrationInfo.m_password;
        subParams.m_realm = command.m_param.m_registrationInfo.m_adminEntity;
#if P_64BIT
        subParams.m_expire = m_apiVersion = command.m_param.m_registrationInfo.m_timeToLive;
#else
        subParams.m_expire = m_apiVersion >= 13 ? command.m_param.m_registrationInfo.m_timeToLive
                                               : *(unsigned*)&command.m_param.m_registrationInfo.m_eventPackage; // Backward compatibility
#endif
        subParams.m_restoreTime = command.m_param.m_registrationInfo.m_restoreTime;
        bool ok = sip->Subscribe(subParams, aor);
        if (m_apiVersion >= 13) {
          if (ok)
            SET_MESSAGE_STRING(response, m_param.m_registrationInfo.m_identifier, aor);
          else
            response.SetError("Failed to initiate SIP subscription.");
        }
      }
    }
    return;
  }
#endif // OPAL_SIP

#if OPAL_SKINNY
  OpalSkinnyEndPoint * skinnyEP = dynamic_cast<OpalSkinnyEndPoint *>(ep);
  if (skinnyEP != NULL) {
    if (!skinnyEP->Register(command.m_param.m_registrationInfo.m_hostName, command.m_param.m_registrationInfo.m_identifier))
      response.SetError("Failed to initiate SCCP registration.");
    return;
  }
#endif // OPAL_SKINNY

#if OPAL_LYNC
  OpalLyncEndPoint * lyncEP = dynamic_cast<OpalLyncEndPoint *>(ep);
  if (lyncEP != NULL) {
    OpalLyncEndPoint::UserParams info;
    info.m_uri = command.m_param.m_registrationInfo.m_identifier;
    info.m_password = command.m_param.m_registrationInfo.m_password;
    PString registeredURI = lyncEP->RegisterUser(info);
    if (registeredURI.IsEmpty())
      response.SetError("Failed to initiate Lync registration.");
    else
      SET_MESSAGE_STRING(response, m_param.m_registrationInfo.m_identifier, registeredURI);
    return;
  }
#endif // OPAL_LYNC

    response.SetError("Protocol prefix does not support registration.");
}


static void SetOptionOverrides(bool originating,
                               OpalConnection::StringOptions & options,
                               const OpalParamProtocol & params)
{
  if (!IsNullString(params.m_defaultOptions)) {
    PStringStream strm(params.m_defaultOptions);
    strm >> options;
  }

  if (!IsNullString(params.m_userName))
    options.Set(originating ? OPAL_OPT_CALLING_PARTY_NAME : OPAL_OPT_CALLED_PARTY_NAME, params.m_userName);

  if (!IsNullString(params.m_displayName))
    options.Set(originating ? OPAL_OPT_CALLING_DISPLAY_NAME : OPAL_OPT_CALLED_DISPLAY_NAME, params.m_displayName);

  if (!IsNullString(params.m_mediaCryptoSuites))
    options.Set(OPAL_OPT_CRYPTO_SUITES, params.m_mediaCryptoSuites);

  if (params.m_userInputMode > OpalUserInputDefault && params.m_userInputMode <= OpalUserInputInBand) {
    static char const * const ModeNames[] = { "Q.931", "String", "Tone", "RFC2833", "InBand" };
    options.Set(OPAL_OPT_USER_INPUT_MODE, ModeNames[params.m_userInputMode-1]);
  }
}


void OpalManager_C::HandleSetUpCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callSetUp.m_partyB)) {
    response.SetError("No destination address provided.");
    return;
  }

#if OPAL_HAS_PCSS
  {
    OpalPCSSEndPoint_C * pcssEP = FindEndPointAs<OpalPCSSEndPoint_C>(OPAL_PREFIX_PCSS);
    if (pcssEP != NULL) {
      PCaselessString partyB = command.m_param.m_callSetUp.m_partyB;
      if (partyB == "testplayer") {
        PSoundChannel::Params params(PSoundChannel::Player, pcssEP->GetSoundChannelPlayDevice());
        params.SetBufferCountFromMS(pcssEP->GetSoundChannelBufferTime());
        response.SetError(PSoundChannel::TestPlayer(params));
        return;
      }
      else if (partyB == "testrecorder") {
        PSoundChannel::Params recordParams(PSoundChannel::Recorder, pcssEP->GetSoundChannelRecordDevice());
        recordParams.SetBufferCountFromMS(pcssEP->GetSoundChannelBufferTime());
        PSoundChannel::Params playerParams(PSoundChannel::Player, pcssEP->GetSoundChannelPlayDevice());
        playerParams.SetBufferCountFromMS(pcssEP->GetSoundChannelBufferTime());
        response.SetError(PSoundChannel::TestRecorder(recordParams, playerParams));
        return;
      }
    }
  }
#endif

  PString partyA = command.m_param.m_callSetUp.m_partyA;
  if (partyA.IsEmpty()) {
    for (PINDEX i = 0; i < PARRAYSIZE(LocalPrefixes); ++i) {
      OpalLocalEndPoint * ep = FindEndPointAs<OpalLocalEndPoint>(LocalPrefixes[i]);
      if (ep != NULL) {
        partyA = LocalPrefixes[i] + ':';
        break;
      }
    }
    if (partyA.IsEmpty()) {
        response.SetError("No local endpoint available");
        return;
    }
  }

  OpalConnection::StringOptions options;
  if (!IsNullString(command.m_param.m_callSetUp.m_alertingType))
    options.SetAt(OPAL_OPT_ALERTING_TYPE, command.m_param.m_callSetUp.m_alertingType);
  if (m_apiVersion >= 26)
    SetOptionOverrides(true, options, command.m_param.m_answerCall.m_overrides);

  PString token;
  if (SetUpCall(partyA, command.m_param.m_callSetUp.m_partyB, token, NULL, 0, &options)) {
    SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_partyA, partyA);
    SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_partyB, command.m_param.m_callSetUp.m_partyB);
    SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_callToken, token);
    PSafePtr<OpalCall> call = FindCallWithLock(token);
    if (call != NULL) {
      PSafePtr<OpalConnection> other = call->GetConnection(1);
      if (other != NULL)
        SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_protocolCallId, other->GetIdentifier());
    }
  }
  else
    response.SetError("Call set up failed.");
}


void OpalManager_C::HandleAlerting(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  OpalConnection::StringOptions options;
  if (m_apiVersion >= 26)
    SetOptionOverrides(false, options, command.m_param.m_answerCall.m_overrides);

  bool withMedia = m_apiVersion >= 29 && command.m_param.m_answerCall.m_withMedia;

  for (PINDEX i = 0; i < PARRAYSIZE(LocalPrefixes); ++i) {
    OpalLocalEndPoint * ep = FindEndPointAs<OpalLocalEndPoint>(LocalPrefixes[i]);
    if (ep != NULL && ep->AlertingIncomingCall(command.m_param.m_callToken, &options, withMedia))
      return;
  }

  response.SetError("No call found by the token provided.");
}


void OpalManager_C::HandleAnswerCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_answerCall.m_callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  OpalConnection::StringOptions options;
  if (m_apiVersion >= 26)
    SetOptionOverrides(false, options, command.m_param.m_answerCall.m_overrides);

  for (PINDEX i = 0; i < PARRAYSIZE(LocalPrefixes); ++i) {
    OpalLocalEndPoint * ep = FindEndPointAs<OpalLocalEndPoint>(LocalPrefixes[i]);
    if (ep != NULL && ep->AcceptIncomingCall(command.m_param.m_callToken, &options))
      return;
  }

  response.SetError("No call found by the token provided.");
}


bool OpalManager_C::FindCall(const char * token, OpalMessageBuffer & response, PSafePtr<OpalCall> & call)
{
  if (IsNullString(token)) {
    response.SetError("No call token provided.");
    return false;
  }

  call = FindCallWithLock(token);
  if (call == NULL) {
    response.SetError("No call found by the token provided.");
    return false;
  }

  return true;
}


void OpalManager_C::HandleUserInput(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_userInput.m_userInput)) {
    response.SetError("No user input provided.");
    return;
  }

  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  PSafePtr<OpalConnection> connection = call->GetConnection(0, PSafeReadOnly);
  while (connection->IsNetworkConnection()) {
    ++connection;
    if (connection == NULL) {
      response.SetError("No suitable connection for user input.");
      return;
    }
  }

  if (command.m_param.m_userInput.m_duration == 0)
    connection->OnUserInputString(command.m_param.m_userInput.m_userInput);
  else
    connection->OnUserInputTone(command.m_param.m_userInput.m_userInput[0], command.m_param.m_userInput.m_duration);
}


void OpalManager_C::HandleClearCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  const char * callToken;
  OpalConnection::CallEndReason reason;

  if (m_apiVersion < 9) {
    callToken = command.m_param.m_callToken;
    reason = OpalConnection::EndedByLocalUser;
  }
  else {
    callToken = command.m_param.m_clearCall.m_callToken;
    reason.code = (OpalConnection::CallEndReasonCodes)command.m_param.m_clearCall.m_reason;
  }

  if (IsNullString(callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  if (!ClearCall(callToken, reason))
    response.SetError("No call found by the token provided.");
}


void OpalManager_C::HandleHoldCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  if (call->IsOnHold()) {
    response.SetError("Call is already on hold.");
    return;
  }

  call->Hold();
}


void OpalManager_C::HandleRetrieveCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  if (!call->IsOnHold()) {
    response.SetError("Call is not on hold.");
    return;
  }

  call->Retrieve();
}


static PString GetPrefixFromPartyAddress(const PString & party)
{
  PINDEX colon = party.Find(':');
  return colon != P_MAX_INDEX ? party.Left(colon) : PString::Empty();
}


void OpalManager_C::HandleTransferCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  PString partyB = command.m_param.m_callSetUp.m_partyB;
  if (partyB.IsEmpty()) {
    response.SetError("No transfer address provided.");
    return;
  }

  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_callSetUp.m_callToken, response, call))
    return;

  PSafePtr<OpalConnection> connection;

  PString search = command.m_param.m_callSetUp.m_partyA;
  if (search == "*") {
	PString prefixB = GetPrefixFromPartyAddress(partyB);
	if (prefixB.IsEmpty()) {
      response.SetError("Invalid transfer address provided, must have prefix.");
      return;
	}
    OpalEndPoint * ep = FindEndPoint(prefixB);
    if (ep != NULL) {
      PTRACE(4, "Searching for local/network connection the same as " << *ep);
      bool isNetwork = ep->HasAttribute(OpalEndPoint::IsNetworkEndPoint);
      connection = call->GetConnection(0);
      while (connection != NULL && !connection->IsReleased() && isNetwork != connection->IsNetworkConnection())
        ++connection;
    }
  }
  else {
    if (search.IsEmpty()) {
      search = GetPrefixFromPartyAddress(partyB);
      if (search.IsEmpty()) {
        response.SetError("Invalid transfer address provided, must have prefix.");
        return;
      }
    }
    else
      search.Delete(search.Find(':'), P_MAX_INDEX);
    search += ':';

    connection = call->GetConnection(0);
    while (connection != NULL && !connection->IsReleased() && connection->GetLocalPartyURL().NumCompare(search) != EqualTo)
      ++connection;
  }

  if (connection != NULL)
    call->Transfer(partyB, connection);
  else
    response.SetError("Call does not have suitable connection to transfer to " + partyB);
}


void OpalManager_C::HandleMediaStream(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_mediaStream.m_callToken, response, call))
    return;

  PSafePtr<OpalConnection> connection = call->GetConnection(0, PSafeReadOnly);
  while (connection->IsNetworkConnection()) {
    ++connection;
    if (connection == NULL) {
      response.SetError("No suitable connection for media stream control.");
      return;
    }
  }

  PCaselessString typeStr = command.m_param.m_mediaStream.m_type;
  bool source = typeStr.Find("out") != P_MAX_INDEX;
  if (!source && typeStr.Find("in") == P_MAX_INDEX) {
    response.SetError("No direction indication in media stream control.");
    return;
  }
  OpalMediaType mediaType = typeStr.Left(typeStr.Find(' '));

  OpalMediaStreamPtr stream;
  if (!IsNullString(command.m_param.m_mediaStream.m_identifier))
    stream = connection->GetMediaStream(PString(command.m_param.m_mediaStream.m_identifier), source);
  else if (!mediaType.empty())
    stream = connection->GetMediaStream(mediaType, source);
  else {
    response.SetError("No identifer or media type provided to locate media stream.");
    return;
  }

  if (stream == NULL && command.m_param.m_mediaStream.m_state != OpalMediaStateOpen) {
    response.SetError("Could not locate media stream.");
    return;
  }

  SET_MESSAGE_STRING(response, m_param.m_mediaStream.m_identifier, stream->GetID());
  SET_MESSAGE_STRING(response, m_param.m_mediaStream.m_type,
                     (stream->GetMediaFormat().GetMediaType() + (stream->IsSource() ? " out" : " in")).c_str());

  switch (command.m_param.m_mediaStream.m_state) {
    case OpalMediaStateNoChange :
      break;

    case OpalMediaStateOpen :
      if (mediaType.empty())
        response.SetError("Must provide type and direction to open media stream.");
      else {
        OpalMediaFormat mediaFormat(command.m_param.m_mediaStream.m_format);
        unsigned sessionID = 0;
        if (stream != NULL)
          sessionID = stream->GetSessionID();
        if (source)
          call->OpenSourceMediaStreams(*connection, mediaType, sessionID, mediaFormat);
        else
          call->OpenSourceMediaStreams(*call->GetOtherPartyConnection(*connection), mediaType, sessionID, mediaFormat);
      }
      break;

    case OpalMediaStateClose :
      stream->Close();
      break;

    case OpalMediaStatePause :
      stream->SetPaused(true);
      break;

    case OpalMediaStateResume :
      stream->SetPaused(false);
      break;
  }

  if (m_apiVersion < 25)
    return;

  unsigned volume;
  if (connection->GetAudioVolume(stream->IsSource(), volume))
    response->m_param.m_mediaStream.m_volume = volume > 0 ? (int)volume : -1;

  if (command.m_param.m_mediaStream.m_volume != 0) {
    if (command.m_param.m_mediaStream.m_volume < 0)
      volume = 0;
    else if (command.m_param.m_mediaStream.m_volume > 100)
      volume = 100;
    else
      volume = command.m_param.m_mediaStream.m_volume;
    connection->SetAudioVolume(stream->IsSource(), volume);
  }

#if OPAL_VIDEO
  if (m_apiVersion < 32)
    return;

  if (IsNullString(command.m_param.m_mediaStream.m_watermark))
    return;

  OpalVideoMediaStream * videoStream = dynamic_cast<OpalVideoMediaStream *>(&*stream);
  if (videoStream == NULL) {
    response.SetError("Watermark can only be set on video stream.");
    return;
  }

  PVideoInputDevice * device = PVideoInputDevice::CreateOpenedDevice(command.m_param.m_mediaStream.m_watermark, false);
  if (device == NULL) {
    response.SetError("Could not open watermark device.");
    return;
  }

  videoStream->SetVideoWatermarkDevice(device);
#endif // OPAL_VIDEO
}


void OpalManager_C::HandleStartRecording(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_recording.m_callToken, response, call))
    return;

#if OPAL_HAS_MIXER
  if (IsNullString(command.m_param.m_recording.m_file)) {
    if (!call->IsRecording())
      response.SetError("No recording active for call.");
    return;
  }

  OpalRecordManager::Options options;
  options.m_stereo = command.m_param.m_recording.m_channels == 2;
  if (m_apiVersion >= 21) {
    options.m_audioFormat = command.m_param.m_recording.m_audioFormat;
#if OPAL_VIDEO
    options.m_videoFormat = command.m_param.m_recording.m_videoFormat;
    options.m_videoWidth  = command.m_param.m_recording.m_videoWidth;
    options.m_videoHeight = command.m_param.m_recording.m_videoHeight;
    options.m_videoRate   = command.m_param.m_recording.m_videoRate;
    options.m_videoMixing = (OpalRecordManager::VideoMode)command.m_param.m_recording.m_videoMixing;
#endif // OPAL_VIDEO
    if (m_apiVersion >= 36 && command.m_param.m_recording.m_audioBufferSize > 0)
      options.m_audioBufferSize = command.m_param.m_recording.m_audioBufferSize;
  }

  if (!call->StartRecording(command.m_param.m_recording.m_file, options))
#else
  if (!IsNullString(command.m_param.m_recording.m_file))
#endif
    response.SetError("Could not start recording for call.");
}


void OpalManager_C::HandleStopRecording(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

#if OPAL_HAS_MIXER
  call->StopRecording();
#endif
}


void OpalManager_C::HandleSetUserData(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  PSafePtr<OpalLocalConnection> connection = call->GetConnectionAs<OpalLocalConnection>();
  if (connection == NULL) {
    response.SetError("No suitable connection for media stream control.");
    return;
  }

  connection->SetUserData(command.m_param.m_setUserData.m_userData);
}


void OpalManager_C::OnEstablishedCall(OpalCall & call)
{
  SetOutgoingCallInfo(OpalIndEstablished, call);
}


void OpalManager_C::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  if (fromRemote) {
    OpalMessageBuffer message(onHold ? OpalIndOnHold : OpalIndOffHold);
    SET_MESSAGE_STRING(message, m_param.m_callToken, connection.GetCall().GetToken());
    PostMessage(message);
  }

  OpalManager::OnHold(connection, fromRemote, onHold);
}


bool OpalManager_C::OnTransferNotify(OpalConnection & connection,
                                     const PStringToString & info)
{
  OpalMessageBuffer message(OpalIndTransferCall);

  SET_MESSAGE_STRING(message, m_param.m_transferStatus.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_transferStatus.m_result, info["result"]);

  PStringStream infoStr;
  infoStr << info;
  SET_MESSAGE_STRING(message, m_param.m_transferStatus.m_info, infoStr);

  PostMessage(message);

  return OpalManager::OnTransferNotify(connection, info);
}


void OpalManager_C::OnIndMediaStream(const OpalMediaStream & stream, OpalMediaStates state)
{
  const OpalConnection & connection = stream.GetConnection();
  if (!connection.IsNetworkConnection())
    return;

  OpalMessageBuffer message(OpalIndMediaStream);
  SET_MESSAGE_STRING(message, m_param.m_mediaStream.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_mediaStream.m_identifier, stream.GetID());
  PStringStream type;
  type << stream.GetMediaFormat().GetMediaType() << (stream.IsSource() ? " in" : " out");
  SET_MESSAGE_STRING(message, m_param.m_mediaStream.m_type, type);
  SET_MESSAGE_STRING(message, m_param.m_mediaStream.m_format, stream.GetMediaFormat().GetName());
  message->m_param.m_mediaStream.m_state = state;
  PTRACE(4, "OnIndMediaStream:"
            " token=\"" << message->m_param.m_userInput.m_callToken << "\""
            " id=\"" << message->m_param.m_mediaStream.m_identifier << '"');
  PostMessage(message);
}


PBoolean OpalManager_C::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return false;

  OnIndMediaStream(stream, OpalMediaStateOpen);
  return true;
}


void OpalManager_C::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OnIndMediaStream(stream, OpalMediaStateClose);
  OpalManager::OnClosedMediaStream(stream);
}


static const PConstCaselessString AllowOnUserInputString("OPAL-C-API-Allow-OnUserInputString");

void OpalManager_C::OnUserInputString(OpalConnection & connection, const PString & value)
{
  if (connection.IsNetworkConnection() && connection.GetStringOptions().GetBoolean(AllowOnUserInputString, true)) {
    OpalMessageBuffer message(OpalIndUserInput);
    SET_MESSAGE_STRING(message, m_param.m_userInput.m_callToken, connection.GetCall().GetToken());
    SET_MESSAGE_STRING(message, m_param.m_userInput.m_userInput, value);
    message->m_param.m_userInput.m_duration = 0;
    PTRACE(4, "OnUserInputString:"
              " token=\"" << message->m_param.m_userInput.m_callToken << "\""
              " input=\"" << message->m_param.m_userInput.m_userInput << '"');
    PostMessage(message);
  }

  OpalManager::OnUserInputString(connection, value);
}


void OpalManager_C::OnUserInputTone(OpalConnection & connection, char tone, int duration)
{
  if (connection.IsNetworkConnection()) {
    char input[2];
    input[0] = tone;
    input[1] = '\0';

    OpalMessageBuffer message(OpalIndUserInput);
    SET_MESSAGE_STRING(message, m_param.m_userInput.m_callToken, connection.GetCall().GetToken());
    SET_MESSAGE_STRING(message, m_param.m_userInput.m_userInput, input);
    message->m_param.m_userInput.m_duration = duration;
    PTRACE(4, "OnUserInputTone:"
              " token=\"" << message->m_param.m_userInput.m_callToken << "\""
              " input=\"" << message->m_param.m_userInput.m_userInput << '"');
    PostMessage(message);
  }

  connection.GetStringOptions().SetBoolean(AllowOnUserInputString, false);
  OpalManager::OnUserInputTone(connection, tone, duration);
  connection.GetStringOptions().Remove(AllowOnUserInputString);
}


void OpalManager_C::OnProceeding(OpalConnection & connection)
{
  OpalCall & call = connection.GetCall();

  OpalMessageBuffer message(OpalIndProceeding);
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyA, call.GetPartyA());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyB, call.GetPartyB());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_callToken, call.GetToken());
  PTRACE(4, "OnProceeding:"
            " token=\"" << message->m_param.m_callSetUp.m_callToken << "\""
            " A=\""     << message->m_param.m_callSetUp.m_partyA << "\""
            " B=\""     << message->m_param.m_callSetUp.m_partyB << '"');
  PostMessage(message);

  OpalManager::OnProceeding(connection);
}


void OpalManager_C::OnClearedCall(OpalCall & call)
{
  OpalMessageBuffer message(OpalIndCallCleared);
  SET_MESSAGE_STRING(message, m_param.m_callCleared.m_callToken, call.GetToken());


  PStringStream str;
  str << (unsigned)call.GetCallEndReason() << ": " << call.GetCallEndReasonText();

  SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, str);
  PTRACE(4, "OnClearedCall:"
            " token=\""  << message->m_param.m_callCleared.m_callToken << "\""
            " reason=\"" << message->m_param.m_callCleared.m_reason << '"');
  PostMessage(message);

  OpalManager::OnClearedCall(call);
}


void OpalManager_C::OnMWIReceived(const PString & party, MessageWaitingType type, const PString & extraInfo)
{
  OpalMessageBuffer message(OpalIndMessageWaiting);
  SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_party, party);
  static const char * const TypeNames[] = { "Voice", "Fax", "Pager", "Multimedia", "Text", "None" };
  if ((size_t)type < sizeof(TypeNames)/sizeof(TypeNames[0]))
    SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_type, TypeNames[type]);
  SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_extraInfo, extraInfo);
  PTRACE(4, "OnMWIReceived: party=\"" << message->m_param.m_messageWaiting.m_party
                                   << "\" type=" << message->m_param.m_messageWaiting.m_type
                                   << "\" info=" << message->m_param.m_messageWaiting.m_extraInfo);
  PostMessage(message);

  OpalManager::OnMWIReceived(party, type, extraInfo);
}


#if OPAL_HAS_PRESENCE
PString ConvertStringSetWithoutLastNewine(const PStringSet & set)
{
  PStringStream strm;
  strm << setfill('\n') << set;
  return strm.Left(strm.GetLength()-1);
}

void OpalManager_C::OnPresenceChange(OpalPresentity &, std::auto_ptr<OpalPresenceInfo> info)
{
  OpalMessageBuffer message(OpalIndPresenceChange);
  SET_MESSAGE_STRING(message, m_param.m_presenceStatus.m_entity,   info->m_entity.AsString());
  SET_MESSAGE_STRING(message, m_param.m_presenceStatus.m_target,   info->m_target.AsString());
  SET_MESSAGE_STRING(message, m_param.m_presenceStatus.m_service,  info->m_service);
  SET_MESSAGE_STRING(message, m_param.m_presenceStatus.m_contact,  info->m_contact);
  SET_MESSAGE_STRING(message, m_param.m_presenceStatus.m_note,     info->m_note);
  SET_MESSAGE_STRING(message, m_param.m_presenceStatus.m_infoType, info->m_infoType);
  SET_MESSAGE_STRING(message, m_param.m_presenceStatus.m_infoData, info->m_infoData);

  SET_MESSAGE_STRING(message, m_param.m_presenceStatus.m_activities, ConvertStringSetWithoutLastNewine(info->m_activities));
  SET_MESSAGE_STRING(message, m_param.m_presenceStatus.m_capabilities, ConvertStringSetWithoutLastNewine(info->m_capabilities));

  message->m_param.m_presenceStatus.m_state = (OpalPresenceStates)info->m_state;

  PTRACE(4, "OpalC API\tOnPresenceChange:\n"
            " entity=\"" << message->m_param.m_presenceStatus.m_entity << "\""
            " target=\"" << message->m_param.m_presenceStatus.m_target << "\""
            " service=\"" << message->m_param.m_presenceStatus.m_service << "\""
            " contact=\"" << message->m_param.m_presenceStatus.m_contact << "\""
            " state=" << message->m_param.m_presenceStatus.m_state <<
            " note=\"" << message->m_param.m_presenceStatus.m_note << '"');
  PostMessage(message);
}
#endif // OPAL_HAS_PRESENCE


void OpalManager_C::HandleAuthorisePresence(const OpalMessage & command, OpalMessageBuffer & response)
{
#if OPAL_HAS_PRESENCE
  OpalPresentity::Authorisation auth;
  switch (command.m_param.m_presenceStatus.m_state) {
    case OpalPresenceForbidden :
      auth = OpalPresentity::AuthorisationDenied;
      break;
    case OpalPresenceUnavailable :
      auth = OpalPresentity::AuthorisationDeniedPolitely;
      break;
    case OpalPresenceAvailable :
      auth = OpalPresentity::AuthorisationPermitted;
      break;
    case OpalPresenceNone :
      auth = OpalPresentity::AuthorisationRemove;
      break;
    default :
      response.SetError("Invalid state for presence authorisation.");
      return;
  }

  PSafePtr<OpalPresentity> presentity = GetPresentity(command.m_param.m_presenceStatus.m_entity);
  if (presentity == NULL)
    response.SetError("URI is not registered for presence.");
  else if (!presentity->SetPresenceAuthorisation(command.m_param.m_presenceStatus.m_target, auth))
    response.SetError("Could not set presence authorisation.");
#else
  response.SetError(PSTRSTRM("Presence for " << command.m_param.m_presenceStatus.m_entity << " not supported by library."));
#endif // OPAL_HAS_PRESENCE
}


void OpalManager_C::HandleSubscribePresence(const OpalMessage & command, OpalMessageBuffer & response)
{
#if OPAL_HAS_PRESENCE
  PSafePtr<OpalPresentity> presentity = GetPresentity(command.m_param.m_presenceStatus.m_entity);
  if (presentity == NULL)
    response.SetError("URI is not registered for presence.");
  else if (IsNullString(command.m_param.m_presenceStatus.m_target))
    response.SetError("No target URI provided.");
  else if (!presentity->SubscribeToPresence(command.m_param.m_presenceStatus.m_target,
                                            command.m_param.m_presenceStatus.m_state != OpalPresenceNone,
                                            command.m_param.m_presenceStatus.m_note))
    response.SetError("Could not subscribe for presence status.");
#else
  response.SetError(PSTRSTRM("Presence for " << command.m_param.m_presenceStatus.m_entity << " not supported by library."));
#endif // OPAL_HAS_PRESENCE
}


void OpalManager_C::HandleSetLocalPresence(const OpalMessage & command, OpalMessageBuffer & response)
{
#if OPAL_HAS_PRESENCE
  PSafePtr<OpalPresentity> presentity = GetPresentity(command.m_param.m_presenceStatus.m_entity);
  if (presentity == NULL)
    response.SetError("URI is not registered for presence.");
  else {
    OpalPresenceInfo::State oldState;
    PString note;
    if (!presentity->GetLocalPresence(oldState, note))
      response.SetError("Could not get local presence state.");
    else {
      OpalPresenceInfo info((OpalPresenceInfo::State)command.m_param.m_presenceStatus.m_state);
      info.m_note = command.m_param.m_presenceStatus.m_note;
      info.m_activities = PString(command.m_param.m_presenceStatus.m_activities).Lines();
      info.m_capabilities = PString(command.m_param.m_presenceStatus.m_capabilities).Lines();
      if (!presentity->SetLocalPresence(info))
        response.SetError("Could not set local presence state.");
      else {
        response->m_param.m_presenceStatus.m_state = (OpalPresenceStates)oldState;
        SET_MESSAGE_STRING(response, m_param.m_presenceStatus.m_note, note);
      }
    }
  }
#else
  response.SetError(PSTRSTRM("Presence for " << command.m_param.m_presenceStatus.m_entity << " not supported by library."));
#endif // OPAL_HAS_PRESENCE
}


#if OPAL_HAS_IM

void OpalManager_C::HandleSendIM(const OpalMessage & command, OpalMessageBuffer & response)
{
  OpalIM im;
  im.m_from = command.m_param.m_instantMessage.m_from;
  im.m_to = command.m_param.m_instantMessage.m_to;
  im.m_toAddr = command.m_param.m_instantMessage.m_host;
  im.m_conversationId = command.m_param.m_instantMessage.m_conversationId;

  if (command.m_param.m_instantMessage.m_bodyCount == 0)
    im.m_bodies.SetAt(PMIMEInfo::TextPlain(), command.m_param.m_instantMessage.m_textBody);
  else {
    for (unsigned i = 0; i < command.m_param.m_instantMessage.m_bodyCount; ++i)
      im.m_bodies.SetAt(command.m_param.m_instantMessage.m_mimeType[i], command.m_param.m_instantMessage.m_bodies[i]);
  }

  if (Message(im)) {
    SET_MESSAGE_STRING(response, m_param.m_instantMessage.m_conversationId, im.m_conversationId);
    response->m_param.m_instantMessage.m_messageId = im.m_messageId;
  }
  else
    response.SetError("Could not send instant message.");
}


void OpalManager_C::OnMessageReceived(const OpalIM & im)
{
  OpalMessageBuffer message(OpalIndReceiveIM);
  SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_from, im.m_from.AsString());
  SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_to,   im.m_to.AsString());
  SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_conversationId, im.m_conversationId);
  SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_textBody, im.m_bodies(PMIMEInfo::TextPlain()));
  SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_htmlBody, im.m_bodies(PMIMEInfo::TextHTML()));

  PINDEX count = im.m_bodies.GetSize();
  if (count > 0) {
    message->m_param.m_instantMessage.m_bodyCount = count;
    SET_MESSAGE_DATA(message, m_param.m_instantMessage.m_mimeType, NULL, count*sizeof(char *));
    SET_MESSAGE_DATA(message, m_param.m_instantMessage.m_bodies, NULL, count*sizeof(char *));
    for (PINDEX i = 0; i < count; ++i) {
      SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_mimeType[i], im.m_bodies.GetKeyAt(i));
      SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_bodies[i], im.m_bodies.GetDataAt(i));
    }
  }

  if (m_apiVersion >= 32)
    message.SetMIME(message->m_param.m_instantMessage.m_bodyCount, message->m_param.m_instantMessage.m_bodyData, im.m_bodyParts);

  PTRACE(4, "OpalC API\tOnMessageReceived:"
            " from=\"" << message->m_param.m_instantMessage.m_from << "\""
            " to=\"" << message->m_param.m_instantMessage.m_to << "\""
            " ID=\"" << message->m_param.m_instantMessage.m_conversationId << '"');
  PostMessage(message);
}


void OpalManager_C::OnMessageDisposition(const OpalIMContext::DispositionInfo & dispostion)
{
  OpalMessageBuffer message(OpalIndSentIM);

//  SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_from, im.m_from.AsString());
//  SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_to,   im.m_to.AsString());
  SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_conversationId, dispostion.m_conversationId);
  message->m_param.m_instantMessage.m_messageId = dispostion.m_messageId;
  SET_MESSAGE_STRING(message, m_param.m_instantMessage.m_textBody, PSTRSTRM(dispostion.m_disposition));
  PTRACE(4, "OpalC API\tOnMessageDisposition:"
            " ID=\"" << message->m_param.m_instantMessage.m_conversationId << "\""
            " state=\"" << message->m_param.m_instantMessage.m_textBody << '"');
  PostMessage(message);
}


void OpalManager_C::OnCompositionIndication(const OpalIMContext::CompositionInfo &)
{
}

#else

void OpalManager_C::HandleSendIM(const OpalMessage &, OpalMessageBuffer & response)
{
  response.SetError("Presence not supported by library.");
}

#endif // OPAL_HAS_IM


///////////////////////////////////////////////////////////////////////////////

extern "C" {

  OpalHandle OPAL_EXPORT OpalInitialise(unsigned * version, const char * options)
  {
    PCaselessString optionsString = IsNullString(options) ? "pcss h323 sip iax2 pots pstn fax t38 ivr" : options;

    // For backward compatibility
    optionsString.Replace("TraceLevel=", "--trace-level ",  true);
    optionsString.Replace("TraceFile=",  "--output ", true);
    optionsString.Replace("TraceAppend", "--trace-option +append", true);

    PArgList args(optionsString,
                  PTRACE_ARGLIST_EXT("t","l","o","R","O")
                  "c-config:"
                  "p-plugin:"
                  "m-manufacturer:"
                  "n-name:"
                  "v-version:", false);
    if (!args.IsParsed())
      return NULL;

    unsigned callerVersion = 1;
    if (version != NULL) {
      callerVersion = *version;
      if (callerVersion > OPAL_C_API_VERSION)
        *version = OPAL_C_API_VERSION;
    }

    OpalHandle handle = new OpalHandleStruct(callerVersion, args);
    if (!handle->m_manager->GetEndPoints().IsEmpty())
      return handle;

    PTRACE(1, "No endpoints were available or selected using \"" << optionsString << '"');
    delete handle;
    return NULL;
  }


  void OPAL_EXPORT OpalShutDown(OpalHandle handle)
  {
    delete handle;
  }


  OpalMessage * OPAL_EXPORT OpalGetMessage(OpalHandle handle, unsigned timeout)
  {
    const char * error;
    return handle == NULL ? NULL : handle->m_manager->GetMessage(timeout, error);
  }


  OpalMessage * OPAL_EXPORT OpalSendMessage(OpalHandle handle, const OpalMessage * message)
  {
    return handle == NULL ? NULL : handle->m_manager->SendMessage(message);
  }


  void OPAL_EXPORT OpalFreeMessage(OpalMessage * message)
  {
    if (message != NULL)
      free(message);
  }

}; // extern "C"


///////////////////////////////////////////////////////////////////////////////

OpalContext::OpalContext()
  : m_handle(NULL)
{
}


OpalContext::~OpalContext()
{
  ShutDown();
}


unsigned OpalContext::Initialise(const char * options, unsigned version)
{
  ShutDown();

  m_handle = OpalInitialise(&version, options);
  return m_handle != NULL ? version : 0;
}


void OpalContext::ShutDown()
{
  if (m_handle != NULL) {
    OpalShutDown(m_handle);
    m_handle = NULL;
  }
}


bool OpalContext::GetMessage(OpalMessagePtr & message, unsigned timeout)
{
  if (m_handle == NULL) {
    PTRACE(1, "OpalContext::GetMessage() called when conext not initialised");
    message.SetType(OpalIndCommandError);
    message.m_message->m_param.m_commandError = "Uninitialised OPAL context.";
    return false;
  }

  const char * error;
  message.m_message = m_handle->m_manager->GetMessage(timeout, error);
  if (message.m_message != NULL)
    return true;

  PTRACE_IF(4, timeout > 0, "OpalContext::GetMessage() " << error);
  message.SetType(OpalIndCommandError);
  message.m_message->m_param.m_commandError = error;
  return false;
}


bool OpalContext::SendMessage(const OpalMessagePtr & message)
{
  OpalMessagePtr response;
  return SendMessage(message, response);
}


bool OpalContext::SendMessage(const OpalMessagePtr & message, OpalMessagePtr & response)
{
  if (m_handle == NULL) {
    response.SetType(OpalIndCommandError);
    response.m_message->m_param.m_commandError = "Uninitialised OPAL context.";
    return false;
  }

  response.m_message = OpalSendMessage(m_handle, message.m_message);
  if (response.m_message != NULL)
    return response.GetType() != OpalIndCommandError;

  response.SetType(OpalIndCommandError);
  response.m_message->m_param.m_commandError = "Invalid message.";
  return false;
}


bool OpalContext::SetUpCall(OpalMessagePtr & response,
                            const char * partyB,
                            const char * partyA,
                            const char * alertingType)
{
  OpalMessagePtr message(OpalCmdSetUpCall);
  OpalParamSetUpCall * param = message.GetCallSetUp();
  param->m_partyA = partyA;
  param->m_partyB = partyB;
  param->m_alertingType = alertingType;
  return SendMessage(message, response);
}


bool OpalContext::AnswerCall(const char * callToken)
{
  OpalMessagePtr message(OpalCmdAnswerCall), response;
  message.SetCallToken(callToken);
  return SendMessage(message, response);
}


bool OpalContext::ClearCall(const char * callToken, OpalCallEndReason reason)
{
  OpalMessagePtr message(OpalCmdClearCall), response;
  OpalParamCallCleared * param = message.GetClearCall();
  param->m_callToken = callToken;
  param->m_reason = reason;
  return SendMessage(message, response);
}


bool OpalContext::SendUserInput(const char * callToken, const char * userInput, unsigned duration)
{
  OpalMessagePtr message(OpalCmdUserInput), response;
  OpalParamUserInput * param = message.GetUserInput();
  param->m_callToken = callToken;
  param->m_userInput = userInput;
  param->m_duration = duration;
  return SendMessage(message, response);
}


OpalMessagePtr::OpalMessagePtr(OpalMessageType type)
  : m_message(NULL)
{
  SetType(type);
}


OpalMessagePtr::~OpalMessagePtr()
{
  OpalFreeMessage(m_message);
}


OpalMessageType OpalMessagePtr::GetType() const
{
  return m_message->m_type;
}


OpalMessagePtr & OpalMessagePtr::SetType(OpalMessageType type)
{
  OpalFreeMessage(m_message);

  m_message = (OpalMessage *)malloc(sizeof(OpalMessage)); // Use malloc to be compatible with OpalFreeMessage
  memset(m_message, 0, sizeof(OpalMessage));
  m_message->m_type = type;
  return *this;
}


const char * OpalMessagePtr::GetCommandError() const
{
  return m_message->m_type == OpalIndCommandError ? m_message->m_param.m_commandError : NULL;
}


const char * OpalMessagePtr::GetCallToken() const
{
  switch (m_message->m_type) {
    case OpalCmdAnswerCall :
    case OpalCmdHoldCall :
    case OpalCmdRetrieveCall :
    case OpalCmdStopRecording :
    case OpalCmdAlerting :
      return m_message->m_param.m_callToken;

    case OpalCmdSetUpCall :
    case OpalIndProceeding :
    case OpalIndAlerting :
    case OpalIndEstablished :
      return m_message->m_param.m_callSetUp.m_callToken;

    case OpalIndIncomingCall :
      return m_message->m_param.m_incomingCall.m_callToken;

    case OpalIndMediaStream :
    case OpalCmdMediaStream :
      return m_message->m_param.m_mediaStream.m_callToken;

    case OpalCmdSetUserData :
      return m_message->m_param.m_setUserData.m_callToken;

    case OpalIndUserInput :
      return m_message->m_param.m_userInput.m_callToken;

    case OpalCmdStartRecording :
      return m_message->m_param.m_recording.m_callToken;

    case OpalIndCallCleared :
      return m_message->m_param.m_callCleared.m_callToken;

    case OpalCmdClearCall :
      return m_message->m_param.m_clearCall.m_callToken;

    default :
      return NULL;
  }
}


void OpalMessagePtr::SetCallToken(const char * callToken)
{
  switch (m_message->m_type) {
    case OpalCmdAnswerCall :
    case OpalCmdHoldCall :
    case OpalCmdRetrieveCall :
    case OpalCmdStopRecording :
    case OpalCmdAlerting :
      m_message->m_param.m_callToken = callToken;
      break;

    case OpalCmdSetUpCall :
    case OpalIndProceeding :
    case OpalIndAlerting :
    case OpalIndEstablished :
      m_message->m_param.m_callSetUp.m_callToken = callToken;
      break;

    case OpalIndIncomingCall :
      m_message->m_param.m_incomingCall.m_callToken = callToken;
      break;

    case OpalIndMediaStream :
    case OpalCmdMediaStream :
      m_message->m_param.m_mediaStream.m_callToken = callToken;
      break;

    case OpalCmdSetUserData :
      m_message->m_param.m_setUserData.m_callToken = callToken;
      break;

    case OpalIndUserInput :
      m_message->m_param.m_userInput.m_callToken = callToken;
      break;

    case OpalCmdStartRecording :
      m_message->m_param.m_recording.m_callToken = callToken;
      break;

    case OpalIndCallCleared :
      m_message->m_param.m_callCleared.m_callToken = callToken;
      break;

    case OpalCmdClearCall :
      m_message->m_param.m_clearCall.m_callToken = callToken;
      break;

    default :
      break;
  }
}


OpalParamGeneral * OpalMessagePtr::GetGeneralParams() const
{
  return m_message->m_type == OpalCmdSetGeneralParameters ? &m_message->m_param.m_general : NULL;
}


OpalParamProtocol * OpalMessagePtr::GetProtocolParams() const
{
  return m_message->m_type == OpalCmdSetProtocolParameters ? &m_message->m_param.m_protocol : NULL;
}


OpalParamRegistration * OpalMessagePtr::GetRegistrationParams() const
{
  return m_message->m_type == OpalCmdRegistration ? &m_message->m_param.m_registrationInfo : NULL;
}


OpalStatusRegistration * OpalMessagePtr::GetRegistrationStatus() const
{
  return m_message->m_type == OpalIndRegistration ? &m_message->m_param.m_registrationStatus : NULL;
}


OpalParamSetUpCall * OpalMessagePtr::GetCallSetUp() const
{
  switch (m_message->m_type) {
    case OpalCmdSetUpCall :
    case OpalIndProceeding :
    case OpalIndAlerting :
    case OpalIndEstablished :
    case OpalIndCompletedIVR :
      return &m_message->m_param.m_callSetUp;

    default :
      return NULL;
  }
}


OpalStatusIncomingCall * OpalMessagePtr::GetIncomingCall() const
{
  return m_message->m_type == OpalIndIncomingCall ? &m_message->m_param.m_incomingCall : NULL;
}


OpalParamAnswerCall * OpalMessagePtr::GetAnswerCall() const
{
  return m_message->m_type == OpalCmdAlerting ||
         m_message->m_type == OpalCmdAnswerCall ? &m_message->m_param.m_answerCall : NULL;
}


OpalStatusUserInput * OpalMessagePtr::GetUserInput() const
{
  switch (m_message->m_type) {
    case OpalIndUserInput :
    case OpalCmdUserInput :
      return &m_message->m_param.m_userInput;

    default :
      return NULL;
  }
}


OpalStatusMessageWaiting * OpalMessagePtr::GetMessageWaiting() const
{
  return m_message->m_type == OpalIndMessageWaiting ? &m_message->m_param.m_messageWaiting : NULL;
}


OpalStatusLineAppearance * OpalMessagePtr::GetLineAppearance() const
{
  return m_message->m_type == OpalIndLineAppearance ? &m_message->m_param.m_lineAppearance : NULL;
}


OpalStatusCallCleared * OpalMessagePtr::GetCallCleared() const
{
  return m_message->m_type == OpalIndCallCleared ? &m_message->m_param.m_callCleared : NULL;
}


OpalParamCallCleared * OpalMessagePtr::GetClearCall() const
{
  return m_message->m_type == OpalCmdClearCall ? &m_message->m_param.m_clearCall : NULL;
}


OpalStatusMediaStream * OpalMessagePtr::GetMediaStream() const
{
  switch (m_message->m_type) {
    case OpalIndMediaStream :
    case OpalCmdMediaStream :
      return &m_message->m_param.m_mediaStream;

    default :
      return NULL;
  }
}


OpalParamSetUserData * OpalMessagePtr::GetSetUserData() const
{
  return m_message->m_type == OpalCmdSetUserData ? &m_message->m_param.m_setUserData : NULL;
}


OpalParamRecording * OpalMessagePtr::GetRecording() const
{
  return m_message->m_type == OpalCmdStartRecording ? &m_message->m_param.m_recording : NULL;
}


OpalStatusTransferCall * OpalMessagePtr::GetTransferStatus() const
{
  return m_message->m_type == OpalIndTransferCall ? &m_message->m_param.m_transferStatus : NULL;
}


OpalPresenceStatus * OpalMessagePtr::GetPresenceStatus() const
{
  switch (m_message->m_type) {
    case OpalCmdAuthorisePresence :
    case OpalCmdSubscribePresence :
    case OpalCmdSetLocalPresence :
    case OpalIndPresenceChange :
      return &m_message->m_param.m_presenceStatus;

    default :
      return NULL;
  }
}


OpalInstantMessage * OpalMessagePtr::GetInstantMessage() const
{
  switch (m_message->m_type) {
    case OpalCmdSendIM :
    case OpalIndReceiveIM :
      return &m_message->m_param.m_instantMessage;

    default :
      return NULL;
  }
}


///////////////////////////////////////////////////////////////////////////////
