/*
 * t38proto.h
 *
 * T.38 protocol handler
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * $Log: t38proto.h,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.1  2001/07/17 04:44:29  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 */

#ifndef __T38_T38PROTO_H
#define __T38_T38PROTO_H

#ifdef __GNUC__
#pragma interface
#endif


class OpalTransport;
class T38_IFPPacket;


///////////////////////////////////////////////////////////////////////////////

/**This class describes the T.38 protocol handler.
 */
class OpalT38Protocol : public PObject
{
    PCLASSINFO(OpalT38Protocol, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new protocol handler.
     */
    OpalT38Protocol();
  //@}

  /**@name Operations */
  //@{
    /**Handle the origination of a T.38 connection.
      */
    virtual BOOL Originate(
      OpalTransport & transport
    );

    /**Handle the origination of a T.38 connection.
      */
    virtual BOOL Answer(
      OpalTransport & transport
    );

    /**Handle incoming T.38 packet.

       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL HandlePacket(
      const T38_IFPPacket & pdu
    );
  //@}
};


#endif // __T38_T38PROTO_H


/////////////////////////////////////////////////////////////////////////////
