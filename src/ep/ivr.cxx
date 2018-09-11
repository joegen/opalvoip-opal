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
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "ivr.h"
#endif

#include <opal_config.h>

#include <ep/ivr.h>
#include <opal/call.h>
#include <opal/patch.h>


#define new PNEW
#define PTraceModule() "IVR"


#if OPAL_IVR


/////////////////////////////////////////////////////////////////////////////

OpalIVREndPoint::OpalIVREndPoint(OpalManager & mgr, const char * prefix)
  : OpalLocalEndPoint(mgr, prefix, false)
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

  PTRACE(4, "Created endpoint.");
}


OpalIVREndPoint::~OpalIVREndPoint()
{
  PTRACE(4, "Deleted endpoint.");
}


PStringList OpalIVREndPoint::GetAvailableStringOptions() const
{
  PStringList options = OpalLocalEndPoint::GetAvailableStringOptions();
  options += OPAL_OPT_IVR_NATIVE_CODEC;
  return options;
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
  OpalMediaFormatList mediaFormats;

  mediaFormats += OpalPCM16;
  mediaFormats += OpalRFC2833;
#if P_VXML_VIDEO
  mediaFormats += OPAL_YUV420P;
#endif

  return mediaFormats;
}


OpalIVRConnection * OpalIVREndPoint::CreateConnection(OpalCall & call,
                                                      void * userData,
                                                      const PString & vxml,
                                                      unsigned int options,
                                                      OpalConnection::StringOptions * stringOptions)
{
  return new OpalIVRConnection(call, *this, userData, vxml, options, stringOptions);
}


static OpalConnection::StringOptions ExtractOptionsFromVXML(const PString & vxml)
{
  OpalConnection::StringOptions options;

  PString vxmlSource = vxml;
  PURL url(vxml);
  if (!url.IsEmpty())
    url.LoadResource(vxmlSource);
  else {
    PTextFile vxmlFile;
    if (vxmlFile.Open(vxml, PFile::ReadOnly))
      vxmlSource = vxmlFile.ReadString(P_MAX_INDEX);
  }

  PRegularExpression embeddedOption("<!--OPAL-[^=]*=");
  PINDEX pos = 0, len;
  while (vxmlSource.FindRegEx(embeddedOption, pos, len, pos)) {
    PINDEX end = vxmlSource.Find("-->", pos + len);
    PString name = vxmlSource.Mid(pos + 9, len - 10).RightTrim();
    PString value = vxmlSource(pos + len, end - 1);
    if (options.Contains(name))
      options.SetAt(name, options.GetString(name) + '\n' + value);
    else
      options.SetAt(name, value);

    pos = end + 3;
  }

  return options;
}


void OpalIVREndPoint::SetDefaultVXML(const PString & vxml)
{
  m_defaultsMutex.Wait();
  m_defaultVXML = vxml;
  m_defaultStringOptions.Merge(ExtractOptionsFromVXML(vxml), PStringOptions::e_MergeOverwrite);
  m_defaultsMutex.Signal();
}


void OpalIVREndPoint::OnEndDialog(OpalIVRConnection & connection)
{
  PTRACE(3, "OnEndDialog for connection " << connection);
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
  , P_DISABLE_MSVC_WARNINGS(4355, m_vxmlSession(*this, PFactory<PTextToSpeech>::CreateInstance(ep.GetDefaultTextToSpeech()), true))
  , m_vxmlStarted(false)
{
#if P_VIDEO
  m_autoStartInfo[OpalMediaType::Video()] = OpalMediaType::DontOffer;
#endif

  m_vxmlSession.SetCache(ep.GetTextToSpeechCache());
  m_vxmlSession.SetRecordDirectory(ep.GetRecordDirectory());
  m_stringOptions.Merge(ExtractOptionsFromVXML(vxml), PStringOptions::e_MergeOverwrite);

  PTRACE(4, "Constructed");
}


OpalIVRConnection::~OpalIVRConnection()
{
  PTRACE(4, "Destroyed.");
}


PString OpalIVRConnection::GetLocalPartyURL() const
{
  return GetPrefixName() + ':' + m_vxmlScript.Left(m_vxmlScript.FindOneOf("\r\n"));
}


void OpalIVRConnection::OnStartMediaPatch(OpalMediaPatch & patch)
{
  OpalLocalConnection::OnStartMediaPatch(patch);

  for (StreamDict::const_iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalMediaStreamPtr mediaStream = it->second;
    if (mediaStream != NULL && !mediaStream->IsEstablished()) {
      PTRACE(4, "Delayed starting VXML, not yet established " << *mediaStream);
      return;
    }
  }

  if (!m_vxmlStarted.exchange(true))
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
  m_stringOptions.Merge(ExtractOptionsFromVXML(vxml), PStringOptions::e_MergeOverwrite);
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
  m_vxmlSession.SetVar("session.connection.local.dnis", GetCalledPartyNumber());
  m_vxmlSession.SetVar("session.connection.remote.ani", GetRemotePartyNumber());
  m_vxmlSession.SetVar("session.connection.calltoken", GetCall().GetToken());
  m_vxmlSession.SetVar("session.connection.remote.uri", remoteURL);
  m_vxmlSession.SetVar("session.connection.remote.ip", remoteURL.GetHostName());
  m_vxmlSession.SetVar("session.connection.remote.port", remoteURL.GetPort());
  m_vxmlSession.SetVar("session.time", PTime().AsString());

  bool ok;

  PCaselessString vxmlHead = vxmlToLoad.LeftTrim().Left(5);
  if (vxmlHead == "<?xml" || vxmlHead == "<vxml") {
    PTRACE(4, "Started using raw VXML:\n" << vxmlToLoad);
    ok = m_vxmlSession.LoadVXML(vxmlToLoad);
  }
  else {
    PURL vxmlURL(vxmlToLoad, NULL);
    if (vxmlURL.IsEmpty()) {
      PFilePath vxmlFile = vxmlToLoad;
      if (vxmlFile.GetType() != ".vxml")
        ok = StartScript(vxmlToLoad);
      else {
        PTRACE(4, "Started using VXML file: " << vxmlFile);
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

  PTRACE(4, "Started using simplified script: " << script);

  PINDEX i;
  PStringArray tokens = script.Tokenise(';', false);
  for (i = 0; i < tokens.GetSize(); ++i) {
    PString str(tokens[i]);

    if (str.Find("file:") == 0) {
      PURL url = str;
      if (url.IsEmpty()) {
        PTRACE(2, "Invalid URL \"" << str << '"');
        continue;
      }

      PFilePath fn = url.AsFilePath();
      if (fn.IsEmpty()) {
        PTRACE(2, "Unsupported host in URL " << url);
        continue;
      }

      if (!voice.IsEmpty())
        fn = fn.GetDirectory() + voice + PDIR_SEPARATOR + fn.GetFileName();

      PTRACE(3, "Playing file " << fn << ' ' << repeat << " times, " << delay << "ms");
      m_vxmlSession.PlayFile(fn, repeat, delay);
      continue;
    }

    PString key, val;
    str.Split('=', key, val, PString::SplitDefaultToBefore|PString::SplitTrimBefore);

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
      PTRACE(3, "Playing tone " << val);
      m_vxmlSession.PlayTone(val, repeat, delay);
   }

    else if (key *= "speak") {
      if (!val.IsEmpty() && (val[0] == '$'))
        val = m_vxmlSession.GetVar(val.Mid(1));
      PTRACE(3, "Speaking text '" << val << "'");
      m_vxmlSession.PlayText(val, PTextToSpeech::Default, repeat, delay);
    }

    else if (key *= "silence") {
      unsigned msecs;
      if (val.IsEmpty() && (val[0] == '$'))
        msecs = m_vxmlSession.GetVar(val.Mid(1)).AsUnsigned();
      else
        msecs = val.AsUnsigned();
      PTRACE(3, "Speaking silence for " << msecs << " msecs");
      m_vxmlSession.PlaySilence(msecs);
    }
    else {
      PTRACE(2, "Invalid command in \"" << script << '"');
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
  OpalMediaFormatList mediaFormats = m_endpoint.GetMediaFormats();

  PStringArray native = m_stringOptions.GetString(OPAL_OPT_IVR_NATIVE_CODEC).Lines();
  for (PINDEX i = 0; i < native.GetSize(); ++i)
    mediaFormats += native[i];

  return mediaFormats;
}


OpalMediaStream * OpalIVRConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                       unsigned sessionID,
                                                       PBoolean isSource)
{
  if (mediaFormat.GetMediaType() == OpalMediaType::Audio())
    return new OpalIVRMediaStream(*this, mediaFormat, sessionID, isSource, m_vxmlSession);

#if P_VXML_VIDEO
  if (mediaFormat.GetMediaType() == OpalMediaType::Video()) {
    if (isSource)
      return new OpalVideoMediaStream(*this, mediaFormat, sessionID, &m_vxmlSession.GetVideoSender(), NULL, false, false);
    else
      return new OpalVideoMediaStream(*this, mediaFormat, sessionID, NULL, &m_vxmlSession.GetVideoReceiver(), false, false);
  }
#endif // P_VXML_VIDEO

  return OpalLocalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);
}


PBoolean OpalIVRConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "SendUserInputString(" << value << ')');

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
  PTRACE(3, "OpalIVRMediaStream sessionID = " << sessionID << ", isSourceStream = " << isSourceStream);
}


PBoolean OpalIVRMediaStream::Open()
{
  if (m_isOpen)
    return true;

  P_INSTRUMENTED_LOCK_READ_WRITE(return false);

  if (m_vxmlSession.IsOpen()) {
    PTRACE(3, "Re-opening");
    PVXMLChannel * vxmlChannel = m_vxmlSession.GetAndLockVXMLChannel();
    if (vxmlChannel == NULL) {
      PTRACE(1, "VXML engine not really open");
      return false;
    }

    PString vxmlChannelMediaFormat = vxmlChannel->GetMediaFormat();
    m_vxmlSession.UnLockVXMLChannel();
    
    if (m_mediaFormat.GetName() != vxmlChannelMediaFormat) {
      PTRACE(1, "Cannot use VXML engine: asymmetrical media formats: " << m_mediaFormat << " <-> " << vxmlChannelMediaFormat);
      return false;
    }

    return OpalMediaStream::Open();
  }

  PTRACE(3, "Opening");
  if (m_vxmlSession.Open(m_mediaFormat))
    return OpalMediaStream::Open();

  PTRACE(1, "Cannot open VXML engine: incompatible media format");
  return false;
}


void OpalIVRMediaStream::InternalClose()
{
  if (m_connection.IsReleased())
    OpalRawMediaStream::InternalClose();
}


PBoolean OpalIVRMediaStream::IsSynchronous() const
{
  return true;
}
#endif // OPAL_IVR
