/*
 * h450pdu.h
 *
 * H.450 Helper functions
 *
 * Open H323 Library
 *
 * Copyright (c) 2001 Norwood Systems Pty. Ltd.
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
 * $Log: h450pdu.h,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.1  2001/04/11 03:01:27  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 */

#ifndef __H323_H450PDU_H
#define __H323_H450PDU_H

#ifdef __GNUC__
#pragma interface
#endif


#include <asn/x880.h>
#include <asn/h4501.h>
#include <asn/h4502.h>


class H323TransportAddress;


///////////////////////////////////////////////////////////////////////////////

/**PDU definition for H.450 services.
  */
class H450ServiceAPDU : public X880_ROS
{
  public:
    X880_Invoke& BuildInvoke(int invokeId, int operation);
    X880_ReturnResult& BuildReturnResult(int invokeId);
    X880_ReturnError& BuildReturnError(int invokeId, int error);
    X880_Reject& BuildReject(int invokeId);

    void BuildCallTransferInitiate(int invokeId,
                                   const PString & callIdentity,
                                   const PString & alias,
                                   const H323TransportAddress & address);

    void BuildCallTransferSetup(int invokeId,
                                const PString & callIdentity);

    static void ParseEndpointAddress(H4501_EndpointAddress & address,
                                     PString & party);
};


#endif // __H323_H450PDU_H


/////////////////////////////////////////////////////////////////////////////
