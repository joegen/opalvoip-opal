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
 * Revision 1.2002  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "ivr.h"
#endif

#include <opal/ivr.h>

#include <opal/call.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalIVREndPoint::OpalIVREndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall),
    defaultVXML("<?xml version=\"1.0\"?>\n"
                "<vxml version=\"1.0\">\n"
                "<form id=\"root\">\n"
                "  <block>\n"
                "    <prompt>\n"
                "      <audio src=\"welcome.wav\"/>\n"
                "        This is the OPAL, V X M L test program.\n"
                "        Thank you for listening.\n"
                "    </prompt>\n"
                "  </block>\n"
                "</form>\n"
                "</vxml>\n")
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

  connection->Lock();

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
    endpoint(ep)
{
  vxmlSession.Load(vxml);

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

  if (!vxmlSession.IsOpen()) {
    PTRACE(1, "IVR\tVXML session not open, aborting.");
    Release(EndedByNoAccept);
    return FALSE;
  }

  phase = AlertingPhase;
  OnAlerting();
  phase = ConnectedPhase;
  OnConnected();

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
  return TRUE;
}


PString OpalIVRConnection::GetDestinationAddress()
{
  return PString::Empty();
}


OpalMediaFormatList OpalIVRConnection::GetMediaFormats() const
{
  // Sound card can only do 16 bit PCM
  OpalMediaFormatList formatNames;
  formatNames += OpalPCM16;
  formatNames += OpalG7231_6k3;
  formatNames += OpalG7231_5k3;
  return formatNames;
}


OpalMediaStream * OpalIVRConnection::CreateMediaStream(BOOL isSource, unsigned sessionID)
{
  return new OpalIVRMediaStream(isSource, sessionID, vxmlSession);
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
  OnIncomingConnection();

  Unlock();
}


/////////////////////////////////////////////////////////////////////////////

OpalIVRMediaStream::OpalIVRMediaStream(BOOL isSourceStream,
                                       unsigned sessionID,
                                       PVXMLSession & vxml)
  : OpalRawMediaStream(isSourceStream, sessionID, &vxml, FALSE),
    vxmlSession(vxml)
{
}


BOOL OpalIVRMediaStream::Open(const OpalMediaFormat & format)
{
  if (format == OpalPCM16)
    return OpalMediaStream::Open(format) && vxmlSession.Open(TRUE);

  if (format == OpalG7231_6k3 || format == OpalG7231_5k3)
    return OpalMediaStream::Open(format) && vxmlSession.Open(FALSE);

  PTRACE(1, "IVR\tCannot open VXML engine: incompatible media format");
  return FALSE;
}


BOOL OpalIVRMediaStream::IsSynchronous() const
{
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
