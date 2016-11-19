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

#using OPAL_LYNC_LIBRARY

using namespace Microsoft::Rtc;


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


ref class OpalLyncShim_Callbacks : public System::Object
{
    OpalLyncShim & m_shim;
  public:
    OpalLyncShim_Callbacks(OpalLyncShim & shim)
      : m_shim(shim)
    {
    }

    void ConferenceInvitationReceived(System::Object^ sender, Collaboration::ConferenceInvitationReceivedEventArgs^ args)
    {
      m_shim.OnConferenceInvitationReceived(msclr::interop::marshal_as<std::string>(args->Invitation->ConferenceUri));
    }

    void CallStateChanged(System::Object^ sender, Collaboration::CallStateChangedEventArgs^ args)
    {
      m_shim.OnCallStateChanged((int)args->PreviousState, (int)args->State);
    }

    void CallEndEstablish(System::IAsyncResult^ ar)
    {
      std::string error;
      try {
        Collaboration::Call^ call = dynamic_cast<Collaboration::Call^>(ar->AsyncState);
        call->EndEstablish(ar);
      }
      catch (System::Exception^ err) {
        error = msclr::interop::marshal_as<std::string>(err->ToString());
      }
      m_shim.OnEndCallEstablished(error);
    }
};


///////////////////////////////////////////////////////////////////////////////

OpalLyncShim::OpalLyncShim()
{
}


OpalLyncShim::~OpalLyncShim()
{
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


bool OpalLyncShim::DestroyPlatform(Platform * platform)
{
  m_lastError.clear();

  if (platform == nullptr)
    return false;

  Collaboration::CollaborationPlatform^ cp = *platform;
  delete platform;

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


OpalLyncShim::UserEndpoint * OpalLyncShim::CreateUserEndpoint(Platform & platform, const char * uri)
{
  m_lastError.clear();

  Collaboration::UserEndpoint^ uep;
  try {
    OpalLyncShim_Callbacks^ callbacks = gcnew OpalLyncShim_Callbacks(*this);
    Collaboration::UserEndpointSettings^ ues = gcnew Collaboration::UserEndpointSettings(gcnew System::String(uri));
    uep = gcnew Collaboration::UserEndpoint(platform, ues);
    uep->ConferenceInvitationReceived += gcnew System::EventHandler<Collaboration::ConferenceInvitationReceivedEventArgs^>
                                                        (callbacks, &OpalLyncShim_Callbacks::ConferenceInvitationReceived);
    uep->EndEstablish(uep->BeginEstablish(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new UserEndpoint(uep);
}


void OpalLyncShim::DestroyUserEndpoint(UserEndpoint * user)
{
  delete user;
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


void OpalLyncShim::DestroyConversation(Conversation * conv)
{
  delete conv;
}


OpalLyncShim::AudioVideoCall * OpalLyncShim::CreateAudioVideoCall(Conversation & conv, const char * uri)
{
  m_lastError.clear();

  Collaboration::AudioVideo::AudioVideoCall^ call;
  try {
    OpalLyncShim_Callbacks^ callbacks = gcnew OpalLyncShim_Callbacks(*this);
    call = gcnew Collaboration::AudioVideo::AudioVideoCall(conv);
    call->StateChanged += gcnew System::EventHandler<Collaboration::CallStateChangedEventArgs^>
                                            (callbacks, &OpalLyncShim_Callbacks::CallStateChanged);
    call->EndEstablish(call->BeginEstablish(gcnew System::String(uri),
                       nullptr,
                       gcnew System::AsyncCallback(callbacks, &OpalLyncShim_Callbacks::CallEndEstablish),
                       call));
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new AudioVideoCall(call);
}


void OpalLyncShim::DestroyAudioVideoCall(AudioVideoCall * call)
{
  delete call;
}


#endif // OPAL_LYNC

// End of File /////////////////////////////////////////////////////////////
