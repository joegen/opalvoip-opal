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
 * $Log: ivr.cxx,v $
 * Revision 2.26  2007/09/18 06:23:22  csoutheren
 * Add ability queue multiple events in IVR kickoff
 *
 * Revision 2.25  2007/09/04 07:46:32  csoutheren
 * Fixed tts
 *
 * Revision 2.24  2007/09/04 05:36:42  csoutheren
 * Add tone generator capability to IVR
 *
 * Revision 2.23  2007/09/04 04:48:41  csoutheren
 * Fix state machine problems
 *
 * Revision 2.22  2007/04/04 02:12:01  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.21  2007/03/29 23:55:46  rjongbloed
 * Tidied some code when a new connection is created by an endpoint. Now
 *   if someone needs to derive a connection class they can create it without
 *   needing to remember to do any more than the new.
 *
 * Revision 2.20  2007/03/29 05:16:50  csoutheren
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and outgoing codecs
 *
 * Revision 2.19  2007/03/13 00:33:10  csoutheren
 * Simple but messy changes to allow compile time removal of protocol
 * options such as H.450 and H.460
 * Fix MakeConnection overrides
 *
 * Revision 2.18  2007/03/01 05:51:04  rjongbloed
 * Fixed backward compatibility of OnIncomingConnection() virtual
 *   functions on various classes. If an old override returned PFalse
 *   then it will now abort the call as it used to.
 *
 * Revision 2.17  2007/01/24 04:00:57  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 2.16  2006/12/18 03:18:42  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 2.15  2006/10/15 06:23:35  rjongbloed
 * Fixed the mechanism where both A-party and B-party are indicated by the application. This now works
 *   for LIDs as well as PC endpoint, wheich is the only one that was used before.
 *
 * Revision 2.14  2006/09/28 07:42:18  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.13  2006/06/27 13:14:17  csoutheren
 * Patch 1353831 - Fixed ivr with h323 faststart
 * Thanks to Frederich Heem
 *
 * Revision 2.12  2006/04/30 14:34:42  csoutheren
 * Backport of IVR updates from PluginBranch
 *
 * Revision 2.11.4.1  2006/04/30 13:49:35  csoutheren
 * Add ability to set TextToSpeech driver
 * Add useful defaults for VXML handling
 *
 * Revision 2.11  2005/05/13 10:05:06  dsandras
 * Slightly modified code so that it compiles with pwlib HEAD.
 *
 * Revision 2.10  2004/08/14 07:56:39  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.9  2004/07/17 09:45:58  rjongbloed
 * Fixed issues with the propagation of the "established" phase of a call. Now
 *   calling an OnEstablished() chain like OnAlerting() and OnConnected() to
 *   finally arrive at OnEstablishedCall() on OpalManager
 *
 * Revision 2.8  2004/07/15 12:32:29  rjongbloed
 * Various enhancements to the VXML code
 *
 * Revision 2.7  2004/07/15 12:19:24  rjongbloed
 * Various enhancements to the VXML code
 *
 * Revision 2.6  2004/07/11 12:42:13  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.5  2004/04/18 13:35:28  rjongbloed
 * Fixed ability to make calls where both endpoints are specified a priori. In particular
 *   fixing the VXML support for an outgoing sip/h323 call.
 *
 * Revision 2.4  2003/06/02 03:15:34  rjongbloed
 * Changed default VXML to be simple "answering machine".
 * Added additional media format names.
 * Allowed for Open() to be called multiple times.
 *
 * Revision 2.3  2003/03/19 02:30:45  robertj
 * Added removal of IVR stuff if EXPAT is not installed on system.
 *
 * Revision 2.2  2003/03/17 10:15:01  robertj
 * Fixed IVR support using VXML.
 * Added video support.
 *
 * Revision 2.1  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "ivr.h"
#endif

#include <opal/ivr.h>

#include <opal/call.h>


#define new PNEW


#if P_EXPAT

/////////////////////////////////////////////////////////////////////////////

OpalIVREndPoint::OpalIVREndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall),
    defaultVXML("<?xml version=\"1.0\"?>"
                "<vxml version=\"1.0\">"
                  "<form id=\"root\">"
                    "<audio src=\"welcome.wav\">"
                      "This is the OPAL, V X M L test program, please speak after the tone."
                    "</audio>"
                    "<record name=\"msg\" beep=\"true\" dtmfterm=\"true\" dest=\"recording.wav\" maxtime=\"10s\"/>"
                  "</form>"
                "</vxml>")
{
  nextTokenNumber = 1;

  defaultMediaFormats += OpalPCM16;

  PTRACE(4, "IVR\tCreated endpoint.");
}


OpalIVREndPoint::~OpalIVREndPoint()
{
  PTRACE(4, "IVR\tDeleted endpoint.");
}


PBoolean OpalIVREndPoint::MakeConnection(OpalCall & call,
                                     const PString & remoteParty,
                                     void * userData,
                               unsigned int /*options*/,
                               OpalConnection::StringOptions * stringOptions)
{
  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (remoteParty.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PString vxml = remoteParty.Mid(prefixLength);
  if (vxml.Left(2) == "//")
    vxml = vxml.Mid(2);
  if (vxml.IsEmpty() || vxml == "*")
    vxml = defaultVXML;

  return AddConnection(CreateConnection(call, CreateConnectionToken(), userData, vxml, stringOptions));
}


OpalMediaFormatList OpalIVREndPoint::GetMediaFormats() const
{
  PWaitAndSignal mutex(inUseFlag);
  return defaultMediaFormats;
}


OpalIVRConnection * OpalIVREndPoint::CreateConnection(OpalCall & call,
                                                      const PString & token,
                                                      void * userData,
                                                      const PString & vxml,
                                                      OpalConnection::StringOptions * stringOptions)
{
  return new OpalIVRConnection(call, *this, token, userData, vxml, stringOptions);
}


PString OpalIVREndPoint::CreateConnectionToken()
{
  inUseFlag.Wait();
  unsigned tokenNumber = nextTokenNumber++;
  inUseFlag.Signal();
  return psprintf("IVR/%u", tokenNumber);
}


void OpalIVREndPoint::SetDefaultVXML(const PString & vxml)
{
  inUseFlag.Wait();
  defaultVXML = vxml;
  inUseFlag.Signal();
}


void OpalIVREndPoint::SetDefaultMediaFormats(const OpalMediaFormatList & formats)
{
  inUseFlag.Wait();
  defaultMediaFormats = formats;
  inUseFlag.Signal();
}

PBoolean OpalIVREndPoint::StartVXML()
{
  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////

OpalIVRConnection::OpalIVRConnection(OpalCall & call,
                                     OpalIVREndPoint & ep,
                                     const PString & token,
                                     void * /*userData*/,
                                     const PString & vxml,
                                     OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, token, 0, stringOptions),
    endpoint(ep),
    vxmlToLoad(vxml),
    vxmlMediaFormats(ep.GetMediaFormats()),
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4355)
#endif
    vxmlSession(this, PFactory<PTextToSpeech>::CreateInstance(ep.GetDefaultTextToSpeech()))
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
  phase = SetUpPhase;

  PTRACE(4, "IVR\tConstructed");
}


OpalIVRConnection::~OpalIVRConnection()
{
  PTRACE(4, "IVR\tDestroyed.");
}


PBoolean OpalIVRConnection::SetUpConnection()
{
  // Check if we are A-Party in thsi call, so need to do things differently
  if (ownerCall.GetConnection(0) == this) {
    phase = SetUpPhase;
    if (!OnIncomingConnection(0, NULL)) {
      Release(EndedByCallerAbort);
      return PFalse;
    }

    PTRACE(3, "IVR\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return PFalse;
    }

    return PTrue;
  }

  remotePartyName = ownerCall.GetOtherPartyConnection(*this)->GetRemotePartyName();

  PTRACE(3, "IVR\tSetUpConnection(" << remotePartyName << ')');

  // load the vxml file before calling OnAlerting() in case of h323 is used with faststart,
  // in this case, the media will be opened ealier and the vxml file needs to be already loaded 
  //if (!StartVXML()) {
  //  PTRACE(1, "IVR\tVXML session not loaded, aborting.");
  //  Release(EndedByLocalUser);
  //  return PFalse;
  //}

  phase = AlertingPhase;
  OnAlerting();

  phase = ConnectedPhase;
  OnConnected();

  return PTrue;
}

void OpalIVRConnection::OnEstablished()
{
  OpalConnection::OnEstablished();
  StartVXML();
}


PBoolean OpalIVRConnection::StartVXML()
{
  PStringToString & vars = vxmlSession.GetSessionVars();

  if (stringOptions != NULL) {
    PString originator = (*stringOptions)("Originator-Address");
    if (originator.IsEmpty())
      originator = (*stringOptions)("Remote-Address");
    if (!originator.IsEmpty()) {
      PIPSocketAddressAndPort ap(originator);
      vars.SetAt("Source-IP-Address", ap.address.AsString());
    }
  }

  vars.SetAt("Time", PTime().AsString());

  if (vxmlToLoad.IsEmpty()) 
    vxmlToLoad = endpoint.GetDefaultVXML();

  if (vxmlToLoad.IsEmpty())
    return endpoint.StartVXML();

  if (vxmlToLoad.Find("<?xml") == 0)
    return vxmlSession.LoadVXML(vxmlToLoad);

  ////////////////////////////////

  PINDEX repeat = 1;
  PINDEX delay = 0;
  PString voice;

  PINDEX i;
  PStringArray tokens = vxmlToLoad.Tokenise(';', PFalse);
  for (i = 0; i < tokens.GetSize(); ++i) {
    PString str(tokens[i]);

    if (str.Find("file://") == 0) {
      PString fn = str.Mid(7).Trim();
      if (fn.Right(5) *= ".vxml") {
        if (!voice.IsEmpty())
          fn = voice + PDIR_SEPARATOR + fn;
        vxmlSession.LoadFile(fn);
        continue;
      }
      else if (fn.Right(4) *= ".wav") {
        if (!voice.IsEmpty())
          fn = voice + PDIR_SEPARATOR + fn;
        vxmlSession.PlayFile(fn, repeat, delay);
        continue;
      }
    }

    PINDEX pos = str.Find("=");
    PString key(str);
    PString val;
    if (pos != P_MAX_INDEX) {
      key = str.Left(pos);
      val = str.Mid(pos+1);
    }

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
        PTextToSpeech * tts = vxmlSession.GetTextToSpeech();
        if (tts != NULL)
          tts->SetVoice(voice);
      }
    }

    else if (key *= "tone") 
      vxmlSession.PlayTone(val, repeat, delay);

    else if (key *= "speak") {
      if (!val.IsEmpty() && (val[0] == '$'))
        val = vars(val.Mid(1));
      vxmlSession.PlayText(val, PTextToSpeech::Default, repeat, delay);
    }
  }

  vxmlSession.SetFinishWhenEmpty(PTrue);

  return PTrue;
}

PBoolean OpalIVRConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "IVR\tSetAlerting(" << calleeName << ')');

  if (!LockReadWrite())
    return PFalse;

  phase = AlertingPhase;
  remotePartyName = calleeName;
  UnlockReadWrite();

  return PTrue;
}


PBoolean OpalIVRConnection::SetConnected()
{
  PTRACE(3, "IVR\tSetConnected()");

  {
    PSafeLockReadWrite safeLock(*this);
    if (!safeLock.IsLocked())
      return PFalse;

    phase = ConnectedPhase;

    if (!StartVXML()) {
      PTRACE(1, "IVR\tVXML session not loaded, aborting.");
      Release(EndedByLocalUser);
      return PFalse;
    }

    if (mediaStreams.IsEmpty())
      return PTrue;

    phase = EstablishedPhase;
  }

  OnEstablished();

  return PTrue;
}


OpalMediaFormatList OpalIVRConnection::GetMediaFormats() const
{
  return vxmlMediaFormats;
}


OpalMediaStream * OpalIVRConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                       unsigned sessionID,
                                                       PBoolean isSource)
{
  return new OpalIVRMediaStream(*this, mediaFormat, sessionID, isSource, vxmlSession);
}


PBoolean OpalIVRConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "IVR\tSendUserInputString(" << value << ')');

  for (PINDEX i = 0; i < value.GetLength(); i++)
    vxmlSession.OnUserInput(value[i]);

  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////

OpalIVRMediaStream::OpalIVRMediaStream(OpalIVRConnection & conn,
                                       const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       PBoolean isSourceStream,
                                       PVXMLSession & vxml)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSourceStream, &vxml, PFalse),
    vxmlSession(vxml)
{
  PTRACE(3, "IVR\tOpalIVRMediaStream sessionID = " << sessionID << ", isSourceStream = " << isSourceStream);
}


PBoolean OpalIVRMediaStream::Open()
{
  PTRACE(3, "IVR\tOpen");
  if (isOpen)
    return PTrue;

  if (vxmlSession.IsOpen()) {
    PVXMLChannel * vxmlChannel = vxmlSession.GetAndLockVXMLChannel();
    PString vxmlChannelMediaFormat;
    
    if (vxmlChannel == NULL) {
      PTRACE(1, "IVR\tVXML engine not really open");
      return PFalse;
    }

    vxmlChannelMediaFormat = vxmlChannel->GetMediaFormat();
    vxmlSession.UnLockVXMLChannel();
    
    if (mediaFormat != vxmlChannelMediaFormat) {
      PTRACE(1, "IVR\tCannot use VXML engine: asymmetrical media format");
      return PFalse;
    }

    return OpalMediaStream::Open();
  }

  if (vxmlSession.Open(mediaFormat))
    return OpalMediaStream::Open();

  PTRACE(1, "IVR\tCannot open VXML engine: incompatible media format");
  return PFalse;
}


PBoolean OpalIVRMediaStream::IsSynchronous() const
{
  return PFalse;
}


#endif // P_EXPAT


/////////////////////////////////////////////////////////////////////////////
