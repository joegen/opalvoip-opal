/*
 * t38proto.cxx
 *
 * T.38 protocol handler
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
 * $Log: t38proto.cxx,v $
 * Revision 1.2002  2001/08/01 05:05:26  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
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
#pragma implementation "t38proto.h"
#endif

#include <t38/t38proto.h>

#include <h323/transaddr.h>
#include <asn/t38.h>


#define new PNEW


OpalMediaFormat const OpalT38Protocol::MediaFormat("T.38",
                                                   OpalMediaFormat::DefaultDataSessionID,
                                                   RTP_DataFrame::MaxPayloadType,
                                                   FALSE, // No jitter for data
                                                   1440); // 100's bits/sec


/////////////////////////////////////////////////////////////////////////////

OpalT38Protocol::OpalT38Protocol()
{
}


BOOL OpalT38Protocol::Originate(OpalTransport & /*transport*/)
{
  return FALSE;
}


BOOL OpalT38Protocol::Answer(OpalTransport & transport)
{
  PPER_Stream rawData;
  while (transport.ReadPDU(rawData)) {
    T38_IFPPacket pdu;
    if (pdu.Decode(rawData)) {
      PTRACE(4, "H323T38\tRead PDU:\n  " << setprecision(2) << pdu);
      if (!HandlePacket(pdu))
        return TRUE;
    }
    else {
      PTRACE(1, "H323T38\tDecode of PDU failed:\n  " << setprecision(2) << pdu);
    }
  }
  return FALSE;
}


BOOL OpalT38Protocol::HandlePacket(const T38_IFPPacket & /*pdu*/)
{
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
