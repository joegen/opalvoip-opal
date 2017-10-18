/*
 * lyncep.cxx
 *
 * Header file for Lync (UCMA) interface
 *
 * Copyright (C) 2016 Vox Lucida Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#define OPAL_LYNC_SHIM 1
#include <ep/lyncep.h>

#if OPAL_LYNC

#include <vcclr.h>
#include <msclr\marshal_cppstd.h>
#include <sstream>

#using OPAL_LYNC_LIBRARY

using namespace Microsoft::Rtc;
using namespace msclr::interop;


#if PTRACING
  #define PTRACE(level, args) { std::ostringstream strm; strm << args; OnTraceOutput(level, __FILE__, __LINE__, strm.str().c_str()); }
#else
  #define PTRACE(...)
#endif


template <class CLS> static void DeleteAndSetNull(CLS * & p)
{
  delete p;
  p = nullptr;
}


int const OpalLyncShim::CallStateEstablishing = (int)Collaboration::CallState::Establishing;
int const OpalLyncShim::CallStateEstablished = (int)Collaboration::CallState::Established;
int const OpalLyncShim::CallStateTerminating = (int)Collaboration::CallState::Terminating;

int const OpalLyncShim::MediaFlowActive = (int)Collaboration::MediaFlowState::Active;

#if PTRACING
std::string OpalLyncShim::GetCallStateName(int callState)
{
  return marshal_as<std::string>(((Collaboration::CallState)callState).ToString());
}

std::string OpalLyncShim::GetTransferStateName(int transferState)
{
  return marshal_as<std::string>(((Microsoft::Rtc::Signaling::ReferState)transferState).ToString());
}

std::string OpalLyncShim::GetToneIdName(int toneId)
{
  return marshal_as<std::string>(((Microsoft::Rtc::Collaboration::AudioVideo::ToneId)toneId).ToString());
}

std::string OpalLyncShim::GetMediaFlowName(int mediaFlow)
{
  return marshal_as<std::string>(((Collaboration::MediaFlowState)mediaFlow).ToString());
}
#endif // PTRACING


struct OpalLyncShim::ApplicationEndpoint : gcroot<Collaboration::ApplicationEndpoint^>
{
  ApplicationEndpoint(Collaboration::ApplicationEndpoint^ ep) : gcroot<Collaboration::ApplicationEndpoint^>(ep) { }
};

struct OpalLyncShim::UserEndpoint : gcroot<Collaboration::UserEndpoint^>
{
  UserEndpoint(Collaboration::UserEndpoint^ ep) : gcroot<Collaboration::UserEndpoint^>(ep) { }
};

struct OpalLyncShim::Platform : gcroot<Collaboration::CollaborationPlatform^>
{
  Platform(Collaboration::CollaborationPlatform^ p) : gcroot<Collaboration::CollaborationPlatform^>(p) { }
};

struct OpalLyncShim::Conversation : gcroot<Collaboration::Conversation^>
{
  Conversation(Collaboration::Conversation^ p) : gcroot<Collaboration::Conversation^>(p) { }
};

struct OpalLyncShim::AudioVideoCall : gcroot<Collaboration::AudioVideo::AudioVideoCall^>
{
  AudioVideoCall(Collaboration::AudioVideo::AudioVideoCall^ p) : gcroot<Collaboration::AudioVideo::AudioVideoCall^>(p) { }
};


struct OpalLyncShim::AudioVideoFlow : gcroot<Collaboration::AudioVideo::AudioVideoFlow^>
{
  AudioVideoFlow(Collaboration::AudioVideo::AudioVideoFlow^ p) : gcroot<Collaboration::AudioVideo::AudioVideoFlow^>(p) { }
};


struct OpalLyncShim::SpeechRecognitionConnector : gcroot<Collaboration::AudioVideo::SpeechRecognitionConnector^>
{
  SpeechRecognitionConnector(Collaboration::AudioVideo::SpeechRecognitionConnector^ p) : gcroot<Collaboration::AudioVideo::SpeechRecognitionConnector^>(p) { }
};

struct OpalLyncShim::ToneController : gcroot<Collaboration::AudioVideo::ToneController^>
{
  ToneController(Collaboration::AudioVideo::ToneController^ p) : gcroot<Collaboration::AudioVideo::ToneController^>(p) { }
};

struct OpalLyncShim::SpeechSynthesisConnector : gcroot<Collaboration::AudioVideo::SpeechSynthesisConnector^>
{
  SpeechSynthesisConnector(Collaboration::AudioVideo::SpeechSynthesisConnector^ p) : gcroot<Collaboration::AudioVideo::SpeechSynthesisConnector^>(p) { }
};


struct OpalLyncShim::AudioVideoStream : gcroot<System::IO::Stream^>
{
  AudioVideoStream(System::IO::Stream^ p) : gcroot<System::IO::Stream^>(p) { }
};


ref class OpalLyncShim_Notifications : public System::Object
{
    OpalLyncShim & m_shim;
  public:
    OpalLyncShim_Notifications(OpalLyncShim & shim)
      : m_shim(shim)
    {
    }

    void ApplicationEndpointOwnerDiscovered(System::Object^ sender, Collaboration::ApplicationEndpointSettingsDiscoveredEventArgs^ args)
    {
      Collaboration::CollaborationPlatform^ platform = dynamic_cast<Collaboration::CollaborationPlatform^>(sender);
      Collaboration::ApplicationEndpoint^ aep = gcnew Collaboration::ApplicationEndpoint(platform, args->ApplicationEndpointSettings);
      OpalLyncShim::ApplicationEndpoint * aepp = new OpalLyncShim::ApplicationEndpoint(aep);
      if (!m_shim.OnApplicationProvisioning(aepp))
        delete aepp;
    }

    void RegisterForIncomingCall(Collaboration::LocalEndpoint^ ep)
    {
      ep->RegisterForIncomingCall<Collaboration::AudioVideo::AudioVideoCall^>(
                 gcnew Collaboration::IncomingCallDelegate<Collaboration::AudioVideo::AudioVideoCall^>
                                                     (this, &OpalLyncShim_Notifications::OnIncomingCall));
    }

    void OnIncomingCall(System::Object^ /*sender*/, Collaboration::CallReceivedEventArgs<Collaboration::AudioVideo::AudioVideoCall^>^ args)
    {
      marshal_context marshal;
      OpalLyncShim::IncomingLyncCallInfo info;
      info.m_call = new OpalLyncShim::AudioVideoCall(args->Call);
      info.m_remoteUri = marshal.marshal_as<const char *>(args->RemoteParticipant->Uri);
      info.m_displayName = marshal.marshal_as<const char *>(args->RemoteParticipant->DisplayName);
      info.m_destinationUri = marshal.marshal_as<const char *>(args->RequestData->ToHeader->Uri);
      info.m_transferredBy = marshal.marshal_as<const char *>(args->TransferredBy);
      m_shim.OnIncomingLyncCall(info);
    }

    void RegisterForCallNotifications(Collaboration::AudioVideo::AudioVideoCall^ call)
    {
      call->TransferReceived += gcnew System::EventHandler<Collaboration::AudioVideo::AudioVideoCallTransferReceivedEventArgs^>
                                                                      (this, &OpalLyncShim_Notifications::CallTransferReceived);
      call->TransferStateChanged += gcnew System::EventHandler<Collaboration::TransferStateChangedEventArgs^>
                                                (this, &OpalLyncShim_Notifications::CallTransferStateChanged);

      call->StateChanged += gcnew System::EventHandler<Collaboration::CallStateChangedEventArgs^>
                                            (this, &OpalLyncShim_Notifications::CallStateChanged);
      call->AudioVideoFlowConfigurationRequested += gcnew System::EventHandler<Collaboration::AudioVideo::AudioVideoFlowConfigurationRequestedEventArgs^>
                                                                                (this, &OpalLyncShim_Notifications::AudioVideoFlowConfigurationRequested);
    }

    void CallStateChanged(System::Object^ /*sender*/, Collaboration::CallStateChangedEventArgs^ args)
    {
      m_shim.OnLyncCallStateChanged((int)args->PreviousState, (int)args->State);
    }

    void CallTransferReceived(System::Object^ /*sender*/, Collaboration::AudioVideo::AudioVideoCallTransferReceivedEventArgs^ args)
    {
      m_shim.OnLyncCallTransferReceived(marshal_as<std::string>(args->TransferDestination), marshal_as<std::string>(args->TransferredBy));
    }

    void CallTransferStateChanged(System::Object^ /*sender*/, Collaboration::TransferStateChangedEventArgs^ args)
    {
      m_shim.OnLyncCallTransferStateChanged((int)args->PreviousState, (int)args->State);
    }

    void AudioVideoFlowConfigurationRequested(System::Object^, Collaboration::AudioVideo::AudioVideoFlowConfigurationRequestedEventArgs^ args)
    {
      args->Flow->StateChanged += gcnew System::EventHandler<Collaboration::MediaFlowStateChangedEventArgs^>
                                                      (this, &OpalLyncShim_Notifications::MediaFlowStateChanged);
    }

    void MediaFlowStateChanged(System::Object^ /*sender*/, Collaboration::MediaFlowStateChangedEventArgs^ args)
    {
      //if (Collaboration::MediaFlowState::Active)
      m_shim.OnMediaFlowStateChanged((int)args->PreviousState, (int)args->State);
    }

    void CallEndEstablish(System::IAsyncResult^ ar)
    {
      try {
        Collaboration::Call^ call = dynamic_cast<Collaboration::Call^>(ar->AsyncState);
        call->EndEstablish(ar);
        m_shim.OnLyncCallEstablished();
      }
      catch (System::Exception^ err) {
        m_shim.OnLyncCallFailed(marshal_as<std::string>(err->ToString()));
      }
    }

    void RegisterForToneNotifications(Collaboration::AudioVideo::ToneController^ toneController)
    {
        // Subscribe to callback to receive DTMFs
      toneController->ToneReceived += gcnew System::EventHandler<Collaboration::AudioVideo::ToneControllerEventArgs^>
                                                                    (this, &OpalLyncShim_Notifications::ToneReceived);

      // Subscribe to callback to receive fax tones
      toneController->IncomingFaxDetected += gcnew System::EventHandler<Collaboration::AudioVideo::IncomingFaxDetectedEventArgs^>
                                                                         (this, &OpalLyncShim_Notifications::IncomingFaxDetected);
    }

    void ToneReceived(System::Object^ /*sender*/, Collaboration::AudioVideo::ToneControllerEventArgs^ args)
    {
      m_shim.OnLyncCallToneReceived(args->Tone, args->Volume);
    }

    void IncomingFaxDetected(System::Object^ /*sender*/, Collaboration::AudioVideo::IncomingFaxDetectedEventArgs^ /* args */)
    {
      m_shim.OnLyncCallIncomingFaxDetected();
    }
};


struct OpalLyncShim::Notifications : gcroot<OpalLyncShim_Notifications^>
{
  Notifications(OpalLyncShim_Notifications^ p) : gcroot<OpalLyncShim_Notifications^>(p) { }
};


///////////////////////////////////////////////////////////////////////////////

OpalLyncShim::OpalLyncShim()
  : m_allocatedNotifications(new Notifications(gcnew OpalLyncShim_Notifications(*this)))
  , m_notifications(*m_allocatedNotifications)
{
}


OpalLyncShim::~OpalLyncShim()
{
  delete m_allocatedNotifications;
}


static System::String^ GetNonEmptyString(const char * str)
{
  return str != NULL && *str != '\0' ? marshal_as<System::String^>(str) : nullptr;
}

OpalLyncShim::Platform * OpalLyncShim::CreatePlatform(const char * userAgent, const PlatformParams & params)
{
  m_lastError.clear();

  Collaboration::CollaborationPlatform^ cp;
  try {
    Collaboration::ServerPlatformSettings^ sps = nullptr;
    if (params.m_certificateFriendlyName.empty()) {
      PTRACE(4, "Creating Collaboration::ServerPlatformSettings");
      sps = gcnew Collaboration::ServerPlatformSettings(GetNonEmptyString(userAgent),
                                                        marshal_as<System::String^>(params.m_localHost),
                                                        params.m_localPort,
                                                        marshal_as<System::String^>(params.m_GRUU));
    }
    else {
      using namespace System::Security::Cryptography::X509Certificates;
      X509Store^ store = gcnew X509Store(StoreLocation::LocalMachine);
      store->Open(OpenFlags::ReadOnly);
      X509Certificate2Collection^ certificates = store->Certificates;
      store->Close();

      System::String^ friendlyName = marshal_as<System::String^>(params.m_certificateFriendlyName);
      for each(X509Certificate2^ certificate in certificates) {
        if (certificate->FriendlyName->Equals(friendlyName, System::StringComparison::OrdinalIgnoreCase)) {
          sps = gcnew Collaboration::ServerPlatformSettings(GetNonEmptyString(userAgent),
                                                            marshal_as<System::String^>(params.m_localHost),
                                                            params.m_localPort,
                                                            marshal_as<System::String^>(params.m_GRUU),
                                                            certificate);
          break;
        }
      }

      if (sps == nullptr) {
        m_lastError = "Unable to find a certificate with Friendly Name '" + params.m_certificateFriendlyName  + "' within Local Store";
        return nullptr;
      }
    }

    if (sps != nullptr) {
      cp = gcnew Collaboration::CollaborationPlatform(sps);
      PTRACE(4, "Starting Collaboration::CollaborationPlatform");
      cp->EndStartup(cp->BeginStartup(nullptr, nullptr));
    }
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new Platform(cp);
}


OpalLyncShim::Platform * OpalLyncShim::CreatePlatform(const char * userAgent, const char * provisioningID)
{
  m_lastError.clear();

  Collaboration::CollaborationPlatform^ cp;
  try {
    Collaboration::ProvisionedApplicationPlatformSettings^ paps = gcnew Collaboration::ProvisionedApplicationPlatformSettings(
                                                    GetNonEmptyString(userAgent), marshal_as<System::String^>(provisioningID));
    cp = gcnew Collaboration::CollaborationPlatform(paps);
    cp->RegisterForApplicationEndpointSettings(gcnew System::EventHandler<Collaboration::ApplicationEndpointSettingsDiscoveredEventArgs^>
                                                       (m_notifications, &OpalLyncShim_Notifications::ApplicationEndpointOwnerDiscovered));
    cp->EndStartup(cp->BeginStartup(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new Platform(cp);
}


OpalLyncShim::Platform * OpalLyncShim::CreatePlatform(const char * userAgent)
{
  m_lastError.clear();

  Collaboration::CollaborationPlatform^ cp;
  try {
    Collaboration::ClientPlatformSettings^ cps = gcnew Collaboration::ClientPlatformSettings(
                              GetNonEmptyString(userAgent), Signaling::SipTransportType::Tls);
    cp = gcnew Collaboration::CollaborationPlatform(cps);
    cp->EndStartup(cp->BeginStartup(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new Platform(cp);
}


bool OpalLyncShim::DestroyPlatform(Platform * &platform)
{
  m_lastError.clear();

  if (platform == nullptr)
    return false;

  Collaboration::CollaborationPlatform^ cp = *platform;
  DeleteAndSetNull(platform);

  if (cp == nullptr)
    return false;

  try {
    cp->EndShutdown(cp->BeginShutdown(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return false;
  }

  return true;
}


bool OpalLyncShim::OnApplicationProvisioning(ApplicationEndpoint * aep)
{
  m_lastError.clear();

  try {
    m_notifications->RegisterForIncomingCall(*aep);
    (*aep)->EndEstablish((*aep)->BeginEstablish(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return false;
  }

  return true;
}


OpalLyncShim::ApplicationEndpoint * OpalLyncShim::CreateApplicationEndpoint(Platform & platform, const ApplicationParams & params)
{
  m_lastError.clear();

  Collaboration::ApplicationEndpoint^ aep;
  try {
    Collaboration::ApplicationEndpointSettings^ aes = gcnew Collaboration::ApplicationEndpointSettings(marshal_as<System::String^>(params.m_ownerURI),
                                                                                                       marshal_as<System::String^>(params.m_proxyHost),
                                                                                                       params.m_proxyPort);
    aes->IsDefaultRoutingEndpoint = params.m_defaultRoutingEndpoint;
    aes->AutomaticPresencePublicationEnabled = params.m_publicisePresence;
    aep = gcnew Collaboration::ApplicationEndpoint(platform, aes);
    m_notifications->RegisterForIncomingCall(aep);
    aep->EndEstablish(aep->BeginEstablish(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new ApplicationEndpoint(aep);
}


void OpalLyncShim::DestroyApplicationEndpoint(ApplicationEndpoint * & app)
{
  DeleteAndSetNull(app);
}


OpalLyncShim::UserEndpoint * OpalLyncShim::CreateUserEndpoint(Platform & platform,
                                                              const char * uri,
                                                              const char * password,
                                                              const char * authID,
                                                              const char * domain)
{
  m_lastError.clear();

  Collaboration::UserEndpoint^ uep;
  try {
    Collaboration::UserEndpointSettings^ ues = gcnew Collaboration::UserEndpointSettings(marshal_as<System::String^>(uri));
    ues->Credential = gcnew System::Net::NetworkCredential(marshal_as<System::String^>(authID),
                                                           marshal_as<System::String^>(password),
                                                           marshal_as<System::String^>(domain));
    uep = gcnew Collaboration::UserEndpoint(platform, ues);
    m_notifications->RegisterForIncomingCall(uep);
    uep->EndEstablish(uep->BeginEstablish(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new UserEndpoint(uep);
}


void OpalLyncShim::DestroyUserEndpoint(UserEndpoint * & user)
{
  DeleteAndSetNull(user);
}


OpalLyncShim::Conversation * OpalLyncShim::CreateConversation(ApplicationEndpoint & aep,
                                                              const char * uri,
                                                              const char * phone,
                                                              const char * display)
{
  m_lastError.clear();

  Collaboration::Conversation^ conv;
  try {
    Collaboration::ConversationSettings^ cs = gcnew Collaboration::ConversationSettings();
    conv = gcnew Collaboration::Conversation(aep, cs);
    PTRACE(4, "Impersonating Uri=" << (uri != NULL && *uri != '\0' ? uri : "NULL") 
           << ", phoneUri=" << (phone != NULL && *phone != '\0' ? phone : "NULL")
           << ", display name=" << (display != NULL && *display != '\0' ? display : "NULL"));
    conv->Impersonate(GetNonEmptyString(uri), GetNonEmptyString(phone), GetNonEmptyString(display));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new Conversation(conv);
}


OpalLyncShim::Conversation * OpalLyncShim::CreateConversation(UserEndpoint & uep)
{
  m_lastError.clear();

  Collaboration::Conversation^ conv;
  try {
    Collaboration::ConversationSettings^ cs = gcnew Collaboration::ConversationSettings();
    conv = gcnew Collaboration::Conversation(uep, cs);
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new Conversation(conv);
}


void OpalLyncShim::DestroyConversation(Conversation * & conv)
{
  if (conv == nullptr)
    return;

  Collaboration::Conversation^ conversation = *conv;
  DeleteAndSetNull(conv);

  if (conversation == nullptr)
    return;

  try {
    conversation->EndTerminate(conversation->BeginTerminate(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
  }
}


OpalLyncShim::AudioVideoCall * OpalLyncShim::CreateAudioVideoCall(Conversation & conv, const char * uri)
{
  m_lastError.clear();

  Collaboration::AudioVideo::AudioVideoCall^ call;
  try {
    call = gcnew Collaboration::AudioVideo::AudioVideoCall(conv);
    m_notifications->RegisterForCallNotifications(call);

    call->BeginEstablish(marshal_as<System::String^>(uri),
                          nullptr,
                          gcnew System::AsyncCallback(m_notifications, &OpalLyncShim_Notifications::CallEndEstablish),
                          call);
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new AudioVideoCall(call);
}


bool OpalLyncShim::AcceptAudioVideoCall(AudioVideoCall & call)
{
  try {
    m_notifications->RegisterForCallNotifications(call);
    call->EndAccept(call->BeginAccept(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return false;
  }

  return true;
}


bool OpalLyncShim::ForwardAudioVideoCall(AudioVideoCall & call, const char * targetURI)
{
  try {
    PTRACE(4, "ForwardAudioVideoCall:"
              " call-Id=" << marshal_as<std::string>(call->CallId)  << ","
              " URI=" << targetURI);
    call->Forward(marshal_as<System::String^>(targetURI));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return false;
  }

  return true;
}


bool OpalLyncShim::TransferAudioVideoCall(AudioVideoCall & call, AudioVideoCall & target, int delayMillis)
{
  try {
    if (delayMillis > 0) {
      PTRACE(4, "TransferAudioVideoCall: sleeping " << delayMillis << "ms before transfer");
      System::Threading::Thread::Sleep(delayMillis);
    }

    PTRACE(4, "TransferAudioVideoCall:"
              " call-id=" << marshal_as<std::string>(call->CallId) << ","
              " target-id=" << marshal_as<std::string>(target->CallId) << ","
              " mode=default ");

    target->EndTransfer(target->BeginTransfer(call, nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return false;
  }

  return true;
}


bool OpalLyncShim::TransferAudioVideoCall(AudioVideoCall & call, const char * targetURI)
{
  try {
    PTRACE(4, "TransferAudioVideoCall:"
              " call-Id=" << marshal_as<std::string>(call->CallId)  << ","
              " URI=" << targetURI << ","
              " mode=Unattended ");
    Collaboration::CallTransferOptions^ options = gcnew Collaboration::CallTransferOptions(Collaboration::CallTransferType::Unattended);
    call->EndTransfer(call->BeginTransfer(marshal_as<System::String^>(targetURI), options, nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return false;
  }

  return true;
}


void OpalLyncShim::DestroyAudioVideoCall(AudioVideoCall * & avc)
{
  if (avc == nullptr)
    return;

  Collaboration::AudioVideo::AudioVideoCall^ call = *avc;
  DeleteAndSetNull(avc);

  if (call == nullptr)
    return;

  try {
    call->EndTerminate(call->BeginTerminate(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
  }
}


OpalLyncShim::AudioVideoFlow * OpalLyncShim::CreateAudioVideoFlow(AudioVideoCall & call)
{
  if (call->Flow == nullptr)
    return nullptr;

  return new OpalLyncShim::AudioVideoFlow(call->Flow);
}


void OpalLyncShim::DestroyAudioVideoFlow(AudioVideoFlow * & flow)
{
  DeleteAndSetNull(flow);
}


OpalLyncShim::SpeechRecognitionConnector * OpalLyncShim::CreateSpeechRecognitionConnector(AudioVideoFlow & flow)
{
  m_lastError.clear();

  Collaboration::AudioVideo::SpeechRecognitionConnector^ connector;
  try {
    connector = gcnew Collaboration::AudioVideo::SpeechRecognitionConnector();
    connector->AttachFlow(flow);
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new SpeechRecognitionConnector(connector);
}


void OpalLyncShim::DestroySpeechRecognitionConnector(SpeechRecognitionConnector * & connector)
{
  if (connector != nullptr) {
    (*connector)->DetachFlow();
    DeleteAndSetNull(connector);
  }
}

OpalLyncShim::ToneController * OpalLyncShim::CreateToneController(AudioVideoFlow & flow)
{
  m_lastError.clear();

  PTRACE(4, "CreateToneController:"
            " Tone=" << (flow->ToneEnabled ? "enabled" : "disabled") << ","
            " Policy=" << marshal_as<std::string>(flow->TonePolicy.ToString()) << ","
            " call-id=" << marshal_as<std::string>(flow->Call->CallId));

  Collaboration::AudioVideo::ToneController^ toneController;
  try {
    toneController = gcnew Collaboration::AudioVideo::ToneController();

    // Subscribe to callback to receive DTMFs
    m_notifications->RegisterForToneNotifications(toneController);

    toneController->AttachFlow(flow);
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new ToneController(toneController);
}


bool OpalLyncShim::SendTone(ToneController & toneController, int toneId)
{
  m_lastError.clear();

  try {
    toneController->Send((Collaboration::AudioVideo::ToneId)toneId);
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return false;
  }

  return true;
}

void OpalLyncShim::DestroyToneController(ToneController * & toneController)
{
  if (toneController != nullptr) {
    (*toneController)->DetachFlow();
    DeleteAndSetNull(toneController);
  }
}


OpalLyncShim::SpeechSynthesisConnector * OpalLyncShim::CreateSpeechSynthesisConnector(AudioVideoFlow & flow)
{
  m_lastError.clear();

  Collaboration::AudioVideo::SpeechSynthesisConnector^ connector;
  try {
    connector = gcnew Collaboration::AudioVideo::SpeechSynthesisConnector();
    connector->AttachFlow(flow);
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new SpeechSynthesisConnector(connector);
}


void OpalLyncShim::DestroySpeechSynthesisConnector(SpeechSynthesisConnector * & connector)
{
  if (connector != nullptr) {
    (*connector)->DetachFlow();
    DeleteAndSetNull(connector);
  }
}


OpalLyncShim::AudioVideoStream * OpalLyncShim::CreateAudioVideoStream(SpeechRecognitionConnector & connector)
{
  m_lastError.clear();

  Collaboration::AudioVideo::SpeechRecognitionStream^ stream;
  try {
    stream = connector->Start();
    PTRACE(3, "Started SpeechRecognitionConnector: format=" << (int)stream->AudioFormat);
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new AudioVideoStream(stream);
}


OpalLyncShim::AudioVideoStream * OpalLyncShim::CreateAudioVideoStream(SpeechSynthesisConnector & connector)
{
  m_lastError.clear();

  System::IO::Stream^ stream;
  try {
    connector->AudioFormat = Collaboration::AudioVideo::AudioFormat::LinearPCM8Khz16bitMono;
    connector->Start();
    PTRACE(3, "Started SpeechSynthesisConnector: format=" << (int)connector->AudioFormat);
    stream = connector->Stream;
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new AudioVideoStream(stream);
}


int OpalLyncShim::ReadAudioVideoStream(AudioVideoStream & stream, unsigned char * data, int size)
{
  int result = -1;
  try {
    array<unsigned char>^ buffer = gcnew array<unsigned char>(size);
    result = stream->Read(buffer, 0, size);
    if (result > 0) {
      pin_ptr<unsigned char> pinned = &buffer[0];
      memcpy(data, pinned, result);
    }
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
  }
  return result;
}


int OpalLyncShim::WriteAudioVideoStream(AudioVideoStream & stream, const unsigned char * data, int length)
{
  int result = -1;
  try {
    array<unsigned char>^ buffer = gcnew array<unsigned char>(length);
    pin_ptr<unsigned char> pinned = &buffer[0];
    memcpy(pinned, data, length);
    stream->Write(buffer, 0, length);
    result = length;
  }
  catch (System::Exception^ err) {
    m_lastError = marshal_as<std::string>(err->ToString());
  }
  return result;
}


void OpalLyncShim::DestroyAudioVideoStream(SpeechRecognitionConnector & connector, AudioVideoStream * & stream)
{
  if (stream != nullptr) {
    connector->Stop();
    (*stream)->Close();
    DeleteAndSetNull(stream);
  }
}


void OpalLyncShim::DestroyAudioVideoStream(SpeechSynthesisConnector & connector, AudioVideoStream * & stream)
{
  if (stream != nullptr) {
    connector->Stop();
    (*stream)->Close();
    DeleteAndSetNull(stream);
  }
}


#endif // OPAL_LYNC

// End of File /////////////////////////////////////////////////////////////
