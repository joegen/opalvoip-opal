/*
 * t120proto.cxx
 *
 * T.120 protocol handler
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: t120proto.cxx,v $
 * Revision 1.2003  2001/08/01 05:05:49  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.1  2001/07/30 01:07:52  robertj
 * Post first check in fix ups.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.1  2001/07/17 04:44:32  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "t120proto.h"
#endif

#include <t120/t120proto.h>

#include <h323/transaddr.h>
#include <t120/x224.h>
#include <asn/mcs.h>


#define new PNEW


OpalMediaFormat const OpalT120Protocol::MediaFormat("T.120",
                                                    OpalMediaFormat::DefaultDataSessionID,
                                                    RTP_DataFrame::MaxPayloadType,
                                                    FALSE,   // No jitter for data
                                                    825000); // 100's bits/sec


/////////////////////////////////////////////////////////////////////////////

OpalT120Protocol::OpalT120Protocol()
{
}


BOOL OpalT120Protocol::Originate(OpalTransport & /*transport*/)
{
  return FALSE;
}


BOOL OpalT120Protocol::Answer(OpalTransport & transport)
{
  PBYTEArray rawData;
  while (transport.ReadPDU(rawData)) {
    X224 pdu;
    if (pdu.Decode(rawData)) {
      PTRACE(4, "H323T120\tRead PDU:\n  " << setprecision(2) << pdu);
      if (!HandlePacket(pdu))
        return TRUE;
    }
    else {
      PTRACE(1, "H323T120\tDecode of PDU failed:\n  " << setprecision(2) << pdu);
    }
  }
  return FALSE;
}


BOOL OpalT120Protocol::HandlePacket(const X224 & pdu)
{
  switch (pdu.GetCode()) {
    case X224::DataPDU :
      break;
    case X224::ConnectRequest :
      return TRUE;
    default :
      return FALSE;
  }

  PBER_Stream ber = pdu.GetData();
  MCS_ConnectMCSPDU mcs_pdu;
  if (mcs_pdu.Decode(ber)) {
    PTRACE(4, "T120\tReceived ConnectMCSPDU:\n  " << setprecision(2) << mcs_pdu);
    return TRUE;
  }

  PTRACE(1, "T120\t\tMCS PDU invalid:\n  " << setprecision(2) << mcs_pdu);
  return FALSE;
}


BOOL OpalT120Protocol::HandleConnect(const MCS_ConnectMCSPDU & /*pdu*/)
{
  return FALSE;
}


BOOL OpalT120Protocol::HandleDomain(const MCS_DomainMCSPDU & /*pdu*/)
{
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
