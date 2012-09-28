/*
 * ivr.cxx
 *
 * Interactive Voice Response support.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "ivr.h"
#endif

#include <opal/buildopts.h>

#include <opal/ivr.h>
#include <opal/call.h>
#include <opal/patch.h>


#define new PNEW


#if OPAL_IVR

  #ifdef _MSC_VER
    #pragma message("IVR support (via VXML/Expat) enabled")
  #endif

/////////////////////////////////////////////////////////////////////////////

OpalIVREndPoint::OpalIVREndPoint(OpalManager & mgr, const char * prefix)
  : OpalLocalEndPoint(mgr, prefix)
{
  SetDefaultVXML("<?xml version=\"1.0\"?>\n"
                "<vxml version=\"1.0\">\n"
                "  <form id=\"root\">\n"
                "    <audio src=\"file:welcome.wav\">\n"
                "      This is the OPAL, V X M L test program, please speak after the tone.\n"
                "    </audio>\n"
                "    <record name=\"msg\" beep=\"true\" dtmfterm=\"true\" dest=\"file:recording.wav\" maxtime=\"10s\"/>\n"
                "  </form>\n"
                 "</vxml>\n");

  PTRACE(4, "IVR\tCreated endpoint.");
}


OpalIVREndPoint::~OpalIVREndPoint()
{
  PTRACE(4, "IVR\tDeleted endpoint.");
}


PSafePtr<OpalConnection> OpalIVREndPoint::MakeConnection(OpalCall & call,
                                                    const PString & remoteParty,
                                                             void * userData,
                                                       unsigned int options,
                                    OpalConnection::StringOptions * stringOptions)
{
  PString ivrString = remoteParty;

  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (ivrString.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PString vxml = ivrString.Mid(prefixLength);
  if (vxml.Left(2) == "//")
    vxml = vxml.Mid(2);
  if (vxml.IsEmpty() || vxml == "*") {
    m_defaultsMutex.Wait();
    vxml = m_defaultVXML;
    vxml.MakeUnique();
    m_defaultsMutex.Signal();
  }

  return AddConnection(CreateConnection(call, userData, vxml, options, stringOptions));
}


OpalMediaFormatList OpalIVREndPoint::GetMediaFormats() const
{
  PWaitAndSignal mutex(m_defaultsMutex);
  return m_defaultMediaFormats;
}


OpalIVRConnection * OpalIVREndPoint::CreateConnection(OpalCall & call,
                                                      void * userData,
                                                      const PString & vxml,
                                                      unsigned int options,
                                                      OpalConnection::StringOptions * stringOptions)
{
  return new OpalIVRConnection(call, *this, userData, vxml, options, stringOptions);
}


static OpalMediaFormatList IncludeMediaFormatsFromVXML(const PString & vxml)
{
  OpalMediaFormatList mediaFormats;

  mediaFormats += OpalPCM16;
  mediaFormats += OpalRFC2833;
#if OPAL_T38_CAPABILITY
  mediaFormats += OpalCiscoNSE;
#endif

  OpalMediaFormatList allFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
  for (OpalMediaFormatList::iterator format = allFormats.begin(); format != allFormats.end(); ++format) {
    if (vxml.Find("<--" + format->GetName() + "-->") != P_MAX_INDEX)
      mediaFormats += *format;
  }

  return mediaFormats;
}


void OpalIVREndPoint::SetDefaultVXML(const PString & vxml)
{
  m_defaultsMutex.Wait();
  m_defaultVXML = vxml;
  m_defaultMediaFormats = IncludeMediaFormatsFromVXML(vxml);
  m_defaultsMutex.Signal();
}


void OpalIVREndPoint::SetDefaultMediaFormats(const OpalMediaFormatList & formats)
{
  m_defaultsMutex.Wait();
  m_defaultMediaFormats = formats;
  m_defaultsMutex.Signal();
}


void OpalIVREndPoint::OnEndDialog(OpalIVRConnection & connection)
{
  PTRACE(3, "IVR\tOnEndDialog for connection " << connection);
  connection.Release();
}



/////////////////////////////////////////////////////////////////////////////

OpalIVRConnection::OpalIVRConnection(OpalCall & call,
                                     OpalIVREndPoint & ep,
                                     void * userData,
                                     const PString & vxml,
                                     unsigned int options,
                                     OpalConnection::StringOptions * stringOptions)
  : OpalLocalConnection(call, ep, userData, options, stringOptions, 'I')
  , endpoint(ep)
  , m_vxmlScript(vxml)
  , m_vxmlMediaFormats(IncludeMediaFormatsFromVXML(vxml))
  , P_DISABLE_MSVC_WARNINGS(4355, m_vxmlSession(*this, PFactory<PTextToSpeech>::CreateInstance(ep.GetDefaultTextToSpeech()), true))
{
  PTRACE(4, "IVR\tConstructed");
}


OpalIVRConnection::~OpalIVRConnection()
{
  PTRACE(4, "IVR\tDestroyed.");
}


PString OpalIVRConnection::GetLocalPartyURL() const
{
  return GetPrefixName() + ':' + m_vxmlScript.Left(m_vxmlScript.FindOneOf("\r\n"));
}


void OpalIVRConnection::OnEstablished()
{
  OpalConnection::OnEstablished();

  if (!m_vxmlSession.IsLoaded())
    StartVXML(m_vxmlScript);
}


bool OpalIVRConnection::OnTransferNotify(const PStringToString & info,
                                         const OpalConnection * transferringConnection)
{
  PString result = info["result"];
  if (result != "progress" && info["party"] == "B")
    m_vxmlSession.SetTransferComplete(result == "success");

  return OpalConnection::OnTransferNotify(info, transferringConnection);
}


bool OpalIVRConnection::TransferConnection(const PString & remoteParty)
{
  // First strip off the prefix if present
  PINDEX prefixLength = 0;
  if (remoteParty.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PString vxml = remoteParty.Mid(prefixLength);
  m_vxmlMediaFormats = IncludeMediaFormatsFromVXML(vxml);
  return StartVXML(vxml);
}


PBoolean OpalIVRConnection::StartVXML(const PString & vxml)
{
  PSafeLockReadWrite mutex(*this);
  if (!mutex.IsLocked())
    return false;

  PString vxmlToLoad = vxml;

  if (vxmlToLoad.IsEmpty() || vxmlToLoad == "*") {
    vxmlToLoad = endpoint.GetDefaultVXML();
    if (vxmlToLoad.IsEmpty())
      return false;
  }

  PURL remoteURL = GetRemotePartyURL();
  m_vxmlSession.SetVar("session.connection.local.uri", GetLocalPartyURL());
  m_vxmlSession.SetVar("session.connection.remote.ani", GetRemotePartyNumber());
  m_vxmlSession.SetVar("session.connection.remote.uri", remoteURL);
  m_vxmlSession.SetVar("session.connection.remote.ip", remoteURL.GetHostName());
  m_vxmlSession.SetVar("session.connection.remote.port", remoteURL.GetPort());
  m_vxmlSession.SetVar("session.time", PTime().AsString());

  bool ok;

  PCaselessString vxmlHead = vxmlToLoad.LeftTrim().Left(5);
  if (vxmlHead == "<?xml" || vxmlHead == "<vxml") {
    PTRACE(4, "IVR\tStarted using raw VXML:\n" << vxmlToLoad);
    ok = m_vxmlSession.LoadVXML(vxmlToLoad);
  }
  else {
    PURL vxmlURL(vxmlToLoad, NULL);
    if (vxmlURL.IsEmpty()) {
      PFilePath vxmlFile = vxmlToLoad;
      if (vxmlFile.GetType() != ".vxml")
        ok = StartScript(vxmlToLoad);
      else {
        PTRACE(4, "IVR\tStarted using VXML file: " << vxmlFile);
        ok = m_vxmlSession.LoadFile(vxmlFile);
      }
    }
    else if (vxmlURL.GetScheme() == "file" && (vxmlURL.AsFilePath().GetType() *= ".vxml"))
      ok = m_vxmlSession.LoadURL(vxmlURL);
    else
      ok = StartScript(vxmlToLoad);
  }

  if (ok)
    m_vxmlScript = vxmlToLoad;

  return ok;
}


bool OpalIVRConnection::StartScript(const PString & script)
{
  PINDEX repeat = 1;
  PINDEX delay = 0;
  PString voice;

  PTRACE(4, "IVR\tStarted using simplified script: " << script);

  PINDEX i;
  PStringArray tokens = script.Tokenise(';', false);
  for (i = 0; i < tokens.GetSize(); ++i) {
    PString str(tokens[i]);

    if (str.Find("file:") == 0) {
      PURL url = str;
      if (url.IsEmpty()) {
        PTRACE(2, "IVR\tInvalid URL \"" << str << '"');
        continue;
      }

      PFilePath fn = url.AsFilePath();
      if (fn.IsEmpty()) {
        PTRACE(2, "IVR\tUnsupported host in URL " << url);
        continue;
      }

      if (!voice.IsEmpty())
        fn = fn.GetDirectory() + voice + PDIR_SEPARATOR + fn.GetFileName();

      PTRACE(3, "IVR\tPlaying file " << fn << ' ' << repeat << " times, " << delay << "ms");
      m_vxmlSession.PlayFile(fn, repeat, delay);
      continue;
    }

    PString key, val;
    if (!str.Split('=', key, val))
      key = str;

    if (key *= "repeat") {
      if (!val.IsEmpty())
        repeat = val.AsInteger();
    }
    else if (key *= "delay") {
      if (!val.IsEmpty())
        delay = val.AsInteger();
    }
    else if (key *= "voice") {
      if (!val.IsEmpty()) {
        voice = val;
        PTextToSpeech * tts = m_vxmlSession.GetTextToSpeech();
        if (tts != NULL)
          tts->SetVoice(voice);
      }
    }

    else if (key *= "tone") {
      PTRACE(3, "IVR\tPlaying tone " << val);
      m_vxmlSession.PlayTone(val, repeat, delay);
   }

    else if (key *= "speak") {
      if (!val.IsEmpty() && (val[0] == '$'))
        val = m_vxmlSession.GetVar(val.Mid(1));
      PTRACE(3, "IVR\tSpeaking text '" << val << "'");
      m_vxmlSession.PlayText(val, PTextToSpeech::Default, repeat, delay);
    }

    else if (key *= "silence") {
      unsigned msecs;
      if (val.IsEmpty() && (val[0] == '$'))
        msecs = m_vxmlSession.GetVar(val.Mid(1)).AsUnsigned();
      else
        msecs = val.AsUnsigned();
      PTRACE(3, "IVR\tSpeaking silence for " << msecs << " msecs");
      m_vxmlSession.PlaySilence(msecs);
    }
    else {
      PTRACE(2, "IVR\tInvalid command in \"" << script << '"');
      return false;
    }
  }

  m_vxmlSession.PlayStop();
  m_vxmlSession.Trigger();

  return true;
}


void OpalIVRConnection::OnEndDialog()
{
  endpoint.OnEndDialog(*this);
}


OpalMediaFormatList OpalIVRConnection::GetMediaFormats() const
{
  return m_vxmlMediaFormats;
}


OpalMediaStream * OpalIVRConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                       unsigned sessionID,
                                                       PBoolean isSource)
{
  return mediaFormat.GetMediaType() != OpalMediaType::Audio()
            ? OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource)
            : new OpalIVRMediaStream(*this, mediaFormat, sessionID, isSource, m_vxmlSession);
}


PBoolean OpalIVRConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "IVR\tSendUserInputString(" << value << ')');

  for (PINDEX i = 0; i < value.GetLength(); i++)
    m_vxmlSession.OnUserInput(value[i]);

  return true;
}


/////////////////////////////////////////////////////////////////////////////

OpalIVRMediaStream::OpalIVRMediaStream(OpalIVRConnection & conn,
                                       const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       PBoolean isSourceStream,
                                       PVXMLSession & vxml)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSourceStream, &vxml, FALSE)
  , m_vxmlSession(vxml)
{
  PTRACE(3, "IVR\tOpalIVRMediaStream sessionID = " << sessionID << ", isSourceStream = " << isSourceStream);
}


PBoolean OpalIVRMediaStream::Open()
{
  if (m_isOpen)
    return true;

  if (m_vxmlSession.IsOpen()) {
    PTRACE(3, "IVR\tRe-opening");
    PVXMLChannel * vxmlChannel = m_vxmlSession.GetAndLockVXMLChannel();
    if (vxmlChannel == NULL) {
      PTRACE(1, "IVR\tVXML engine not really open");
      return false;
    }

    PString vxmlChannelMediaFormat = vxmlChannel->GetMediaFormat();
    m_vxmlSession.UnLockVXMLChannel();
    
    if (mediaFormat.GetName() != vxmlChannelMediaFormat) {
      PTRACE(1, "IVR\tCannot use VXML engine: asymmetrical media formats: " << mediaFormat << " <-> " << vxmlChannelMediaFormat);
      return false;
    }

    return OpalMediaStream::Open();
  }

  PTRACE(3, "IVR\tOpening");
  if (m_vxmlSession.Open(mediaFormat))
    return OpalMediaStream::Open();

  PTRACE(1, "IVR\tCannot open VXML engine: incompatible media format");
  return false;
}


void OpalIVRMediaStream::InternalClose()
{
  if (connection.IsReleased())
    OpalRawMediaStream::InternalClose();
}


PBoolean OpalIVRMediaStream::IsSynchronous() const
{
  return true;
}


#else

  #ifdef _MSC_VER
    #pragma message("IVR support (via VXML/Expat) DISABLED")
  #endif

#endif // OPAL_IVR


/////////////////////////////////////////////////////////////////////////////
