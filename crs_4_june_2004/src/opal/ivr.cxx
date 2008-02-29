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
 * Revision 1.2006  2004/04/18 13:35:28  rjongbloed
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

  PTRACE(3, "IVR\tCreated endpoint.");
}


OpalIVREndPoint::~OpalIVREndPoint()
{
  PTRACE(3, "IVR\tDeleted endpoint.");
}


BOOL OpalIVREndPoint::MakeConnection(OpalCall & call,
                                     const PString & remoteParty,
                                     void * userData)
{
  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (remoteParty.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PString vxml = remoteParty.Mid(prefixLength);
  if (vxml.IsEmpty() || vxml == "*")
    vxml = defaultVXML;

  OpalIVRConnection * connection = CreateConnection(call, CreateConnectionToken(), userData, vxml);
  if (connection == NULL)
    return FALSE;

  // If we are the A-party then need to initiate a call now in this thread and
  // go through the routing engine via OnIncomingConnection. If we are the
  // B-Party then SetUpConnection() gets called in the context of the A-party
  // thread.
  if (&call.GetConnection(0) == connection)
    connection->InitiateCall();

  return TRUE;
}


OpalIVRConnection * OpalIVREndPoint::CreateConnection(OpalCall & call,
                                                      const PString & token,
                                                      void * userData,
                                                      const PString & vxml)
{
  return new OpalIVRConnection(call, *this, token, userData, vxml);
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


/////////////////////////////////////////////////////////////////////////////

OpalIVRConnection::OpalIVRConnection(OpalCall & call,
                                     OpalIVREndPoint & ep,
                                     const PString & token,
                                     void * /*userData*/,
                                     const PString & vxml)
  : OpalConnection(call, ep, token),
    endpoint(ep),
    vxmlToLoad(vxml),
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4355)
#endif
    vxmlSession(this)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
  phase = SetUpPhase;

  PTRACE(3, "IVR\tCreated connection.");
}


OpalIVRConnection::~OpalIVRConnection()
{
  PTRACE(3, "IVR\tDeleted connection.");
}


BOOL OpalIVRConnection::SetUpConnection()
{
  remotePartyName = ownerCall.GetConnection(0).GetRemotePartyName();

  PTRACE(3, "IVR\tSetUpConnection(" << remotePartyName << ')');

  phase = AlertingPhase;
  OnAlerting();

  phase = ConnectedPhase;
  OnConnected();

  if (!vxmlSession.Load(vxmlToLoad)) {
    PTRACE(1, "IVR\tVXML session not loaded, aborting.");
    Release(EndedByLocalUser);
    return FALSE;
  }

  return TRUE;
}


BOOL OpalIVRConnection::SetAlerting(const PString & calleeName, BOOL)
{
  PTRACE(3, "IVR\tSetAlerting(" << calleeName << ')');
  phase = AlertingPhase;
  remotePartyName = calleeName;
  return TRUE;
}


BOOL OpalIVRConnection::SetConnected()
{
  PTRACE(3, "IVR\tSetConnected()");
  phase = ConnectedPhase;

  if (!vxmlSession.Load(vxmlToLoad)) {
    PTRACE(1, "IVR\tVXML session not loaded, aborting.");
    Release(EndedByLocalUser);
    return FALSE;
  }

  return TRUE;
}


OpalMediaFormatList OpalIVRConnection::GetMediaFormats() const
{
  // Sound card can only do 16 bit PCM
  OpalMediaFormatList formatNames;

  formatNames += OpalPCM16;

#if 0
  /* This is removed until some code is added to add them it it detects that
     there is a directory containing G.723.1 and/or G.729 wav files. */
  formatNames += OpalG7231A_6k3;
  formatNames += OpalG7231_6k3;
  formatNames += OpalG729;
  formatNames += OpalG729A;
#endif

  return formatNames;
}


OpalMediaStream * OpalIVRConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                       unsigned sessionID,
                                                       BOOL isSource)
{
  return new OpalIVRMediaStream(mediaFormat, sessionID, isSource, vxmlSession);
}


BOOL OpalIVRConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "IVR\tSendUserInputString(" << value << ')');

  for (PINDEX i = 0; i < value.GetLength(); i++)
    vxmlSession.OnUserInput(value[i]);

  return TRUE;
}


void OpalIVRConnection::InitiateCall()
{
  if (!Lock())
    return;

  phase = SetUpPhase;
  if (!OnIncomingConnection())
    Release(EndedByCallerAbort);
  else {
    PTRACE(2, "IVR\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this))
      Release(EndedByNoAccept);
  }

  Unlock();
}


/////////////////////////////////////////////////////////////////////////////

OpalIVRMediaStream::OpalIVRMediaStream(const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       BOOL isSourceStream,
                                       PVXMLSession & vxml)
  : OpalRawMediaStream(mediaFormat, sessionID, isSourceStream, &vxml, FALSE),
    vxmlSession(vxml)
{
}


BOOL OpalIVRMediaStream::Open()
{
  if (isOpen)
    return TRUE;

  if (vxmlSession.IsOpen()) {
    PVXMLChannel * vxmlChannel;
    if (IsSource())
      vxmlChannel = vxmlSession.GetOutgoingChannel();
    else
      vxmlChannel = vxmlSession.GetIncomingChannel();

    if (vxmlChannel == NULL) {
      PTRACE(1, "IVR\tVXML engine not really open");
      return FALSE;
    }

    if (mediaFormat != vxmlChannel->GetFormatName()) {
      PTRACE(1, "IVR\tCannot use VXML engine: asymmetrical media format");
      return FALSE;
    }

    return OpalMediaStream::Open();
  }

  if (mediaFormat == OpalPCM16) {
    if (vxmlSession.Open(new PVXMLChannelPCM(vxmlSession, TRUE),
                            new PVXMLChannelPCM(vxmlSession, FALSE)))
      return OpalMediaStream::Open();
  }
  else if (mediaFormat == OpalG7231_6k3 || mediaFormat == OpalG7231_5k3) {
    if (vxmlSession.Open(new PVXMLChannelG7231(vxmlSession, TRUE),
                            new PVXMLChannelG7231(vxmlSession, FALSE)))
      return OpalMediaStream::Open();
  }
  else if (mediaFormat == OpalG729A) {
    if (vxmlSession.Open(new PVXMLChannelG729(vxmlSession, TRUE),
                            new PVXMLChannelG729(vxmlSession, FALSE)))
      return OpalMediaStream::Open();
  }

  PTRACE(1, "IVR\tCannot open VXML engine: incompatible media format");
  return FALSE;
}


BOOL OpalIVRMediaStream::IsSynchronous() const
{
  return FALSE;
}


#endif // P_EXPAT


/////////////////////////////////////////////////////////////////////////////
