/*
 * rtspep.cxx
 *
 * Real Time Streaming Protocol endpoint.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2009 Post Increment
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
 * $Revision: 21807 $
 * $Author: csoutheren $
 * $Date: 2008-12-16 14:28:12 +1100 (Tue, 16 Dec 2008) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_HAS_RTSP

#ifdef __GNUC__
#pragma implementation "rtspep.h"
#endif

#include "rtsp/rtspep.h"

OpalRTSPEndPoint::OpalRTSPEndPoint(OpalManager & manager)
  : OpalEndPoint(manager, "rtsp", CanTerminateCall)
{
  //manager.AttachEndPoint(this, "rtspu");
  defaultSignalPort = 554;
}

OpalRTSPEndPoint::~OpalRTSPEndPoint()
{
}

bool OpalRTSPEndPoint::AddResource(Resource * resource)
{
  PWaitAndSignal m(resourceMutex);

  resourceMap.insert(ResourceMap::value_type(*resource, resource));

  return true;
}

void OpalRTSPEndPoint::ShutDown()
{
  OpalEndPoint::ShutDown();
}

PString OpalRTSPEndPoint::GetDefaultTransport() const
{
  return "tcp$";
}

PBoolean OpalRTSPEndPoint::MakeConnection(
  OpalCall & call,                         ///<  Owner of connection
  const PString & party,                   ///<  Remote party to call
  void * userData,                         ///<  Arbitrary data to pass to connection
  unsigned int options,                    ///<  options to pass to conneciton
  OpalConnection::StringOptions * stringOptions  ///<  complex string options
)
{
  return false;
}

PBoolean OpalRTSPEndPoint::NewIncomingConnection(OpalTransport * transport)
{
  PTRACE_IF(2, transport->IsReliable(), "RTSP\tListening thread started.");

  transport->SetBufferSize(RTSP_PDU::MaxSize);

  do {
    HandlePDU(*transport);
  } while (transport->IsOpen() && transport->IsReliable() && !transport->bad() && !transport->eof());

  PTRACE_IF(2, transport->IsReliable(), "RTSP\tListening thread finished.");

  return PTrue;
}

void OpalRTSPEndPoint::HandlePDU(OpalTransport & transport)
{
  // create a RTSP_PDU structure, then get it to read and process PDU
  RTSP_PDU * pdu = new RTSP_PDU;

  PTRACE(4, "RTSP\tWaiting for PDU on " << transport);
  if (pdu->Read(transport)) {
    if (OnReceivedPDU(transport, pdu)) 
      return;
  }
  else {
    PTRACE_IF(1, transport.GetErrorCode(PChannel::LastReadError) != PChannel::NoError,
              "RTSP\tPDU Read failed: " << transport.GetErrorText(PChannel::LastReadError));
    if (transport.good()) {
      PTRACE(2, "RTSP\tMalformed request received on " << transport);
      //pdu->SendResponse(transport, RTSP_PDU::Failure_BadRequest, this);
    }
  }

  delete pdu;
}

#endif
