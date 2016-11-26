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

#if PTRACING
  #define PTRACE(level, args) { std::ostringstream strm; strm << args; OnTraceOutput(level, __FILE__, __LINE__, strm.str()); }
#else
  #define PTRACE(...)
#endif


template <class CLS> void DeleteAndSetNull(CLS * & p)
{
  delete p;
  p = nullptr;
}


int const OpalLyncShim::CallStateEstablishing = (int)Collaboration::CallState::Establishing;
int const OpalLyncShim::CallStateEstablished = (int)Collaboration::CallState::Established;
int const OpalLyncShim::CallStateTerminating = (int)Collaboration::CallState::Terminating;
int const OpalLyncShim::MediaFlowActive = (int)Collaboration::MediaFlowState::Active;

struct OpalLyncShim::AppEndpoint : gcroot<Collaboration::ApplicationEndpoint^>
{
  AppEndpoint(Collaboration::ApplicationEndpoint^ ep) : gcroot<Collaboration::ApplicationEndpoint^>(ep) { }
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

    void RegisterForIncomingCall(Collaboration::LocalEndpoint^ ep)
    {
      ep->RegisterForIncomingCall<Collaboration::AudioVideo::AudioVideoCall^>(
                 gcnew Collaboration::IncomingCallDelegate<Collaboration::AudioVideo::AudioVideoCall^>
                                                     (this, &OpalLyncShim_Notifications::OnIncomingCall));
    }

    void OnIncomingCall(System::Object^ /*sender*/, Collaboration::CallReceivedEventArgs<Collaboration::AudioVideo::AudioVideoCall^>^ args)
    {
      OpalLyncShim::IncomingLyncCallInfo info;
      info.m_call = new OpalLyncShim::AudioVideoCall(args->Call);
      info.m_remoteUri = msclr::interop::marshal_as<std::string>(args->RemoteParticipant->Uri);
      info.m_displayName = msclr::interop::marshal_as<std::string>(args->RemoteParticipant->DisplayName);
      info.m_destinationUri = msclr::interop::marshal_as<std::string>(args->RequestData->ToHeader->Uri);
      info.m_transferredBy = msclr::interop::marshal_as<std::string>(args->TransferredBy);
      m_shim.OnIncomingLyncCall(info);
    }

    void RegisterForCallNotifications(Collaboration::AudioVideo::AudioVideoCall^ call)
    {
      call->StateChanged += gcnew System::EventHandler<Collaboration::CallStateChangedEventArgs^>
                                            (this, &OpalLyncShim_Notifications::CallStateChanged);
      call->AudioVideoFlowConfigurationRequested += gcnew System::EventHandler<Collaboration::AudioVideo::AudioVideoFlowConfigurationRequestedEventArgs^>
                                                                                (this, &OpalLyncShim_Notifications::AudioVideoFlowConfigurationRequested);
    }

    void CallStateChanged(System::Object^ /*sender*/, Collaboration::CallStateChangedEventArgs^ args)
    {
      m_shim.OnLyncCallStateChanged((int)args->PreviousState, (int)args->State);
    }

    void AudioVideoFlowConfigurationRequested(System::Object^, Collaboration::AudioVideo::AudioVideoFlowConfigurationRequestedEventArgs^ args)
    {
      args->Flow->StateChanged += gcnew System::EventHandler<Collaboration::MediaFlowStateChangedEventArgs^>
                                                      (this, &OpalLyncShim_Notifications::MediaFlowStateChanged);
    }

    void MediaFlowStateChanged(System::Object^, Collaboration::MediaFlowStateChangedEventArgs^ args)
    {
      //if (Collaboration::MediaFlowState::Active)
      m_shim.OnMediaFlowStateChanged((int)args->PreviousState, (int)args->State);
    }

    void CallEndEstablish(System::IAsyncResult^ ar)
    {
      try {
        Collaboration::Call^ call = dynamic_cast<Collaboration::Call^>(ar->AsyncState);
        call->EndEstablish(ar);
      }
      catch (System::Exception^ err) {
        m_shim.OnLyncCallFailed(msclr::interop::marshal_as<std::string>(err->ToString()));
      }
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


OpalLyncShim::Platform * OpalLyncShim::CreatePlatform(const char * appName)
{
  m_lastError.clear();

  System::String^ userAgent = nullptr;
  if (appName != NULL && *appName != '\0')
    userAgent = gcnew System::String(appName);

  Collaboration::CollaborationPlatform^ cp;
  try {
    Collaboration::ClientPlatformSettings^ cps = gcnew Collaboration::ClientPlatformSettings(userAgent, Signaling::SipTransportType::Tls);
    cp = gcnew Collaboration::CollaborationPlatform(cps);
    cp->EndStartup(cp->BeginStartup(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
    return false;
  }

  return true;
}


OpalLyncShim::AppEndpoint * OpalLyncShim::CreateAppEndpoint(Platform & platform)
{
  m_lastError.clear();

  Collaboration::ApplicationEndpoint^ aep;
  try {
    Collaboration::ApplicationEndpointSettings^ aes = gcnew Collaboration::ApplicationEndpointSettings(nullptr);
    aep = gcnew Collaboration::ApplicationEndpoint(platform, aes);
    m_notifications->RegisterForIncomingCall(aep);
    aep->EndEstablish(aep->BeginEstablish(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new AppEndpoint(aep);
}


void OpalLyncShim::DestroyAppEndpoint(AppEndpoint * & app)
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
    Collaboration::UserEndpointSettings^ ues = gcnew Collaboration::UserEndpointSettings(gcnew System::String(uri));
    ues->Credential = gcnew System::Net::NetworkCredential(gcnew System::String(authID),
                                                           gcnew System::String(password),
                                                           gcnew System::String(domain));
    uep = gcnew Collaboration::UserEndpoint(platform, ues);
    m_notifications->RegisterForIncomingCall(uep);
    uep->EndEstablish(uep->BeginEstablish(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new UserEndpoint(uep);
}


void OpalLyncShim::DestroyUserEndpoint(UserEndpoint * & user)
{
  DeleteAndSetNull(user);
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
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
  }
}


OpalLyncShim::AudioVideoCall * OpalLyncShim::CreateAudioVideoCall(Conversation & conv, const char * uri, bool answering)
{
  m_lastError.clear();

  Collaboration::AudioVideo::AudioVideoCall^ call;
  try {
    call = gcnew Collaboration::AudioVideo::AudioVideoCall(conv);
    m_notifications->RegisterForCallNotifications(call);

    if (answering) {
    }
    else
      call->BeginEstablish(gcnew System::String(uri),
                           nullptr,
                           gcnew System::AsyncCallback(m_notifications, &OpalLyncShim_Notifications::CallEndEstablish),
                           call);
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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


OpalLyncShim::SpeechSynthesisConnector * OpalLyncShim::CreateSpeechSynthesisConnector(AudioVideoFlow & flow)
{
  m_lastError.clear();

  Collaboration::AudioVideo::SpeechSynthesisConnector^ connector;
  try {
    connector = gcnew Collaboration::AudioVideo::SpeechSynthesisConnector();
    connector->AttachFlow(flow);
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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

  System::IO::Stream^ stream;
  try {
    stream = connector->Start();
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new AudioVideoStream(stream);
}


OpalLyncShim::AudioVideoStream * OpalLyncShim::CreateAudioVideoStream(SpeechSynthesisConnector & connector)
{
  m_lastError.clear();

  System::IO::Stream^ stream;
  try {
    stream = connector->Stream;
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
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
