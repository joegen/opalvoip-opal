/*
 * t120proto.h
 *
 * T.120 protocol handler
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
 * $Id$
 */

#ifndef __OPAL_T120PROTO_H
#define __OPAL_T120PROTO_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/mediafmt.h>


class OpalTransport;

class X224;
class MCS_ConnectMCSPDU;
class MCS_DomainMCSPDU;


#define OPAL_T120 "T.120"


///////////////////////////////////////////////////////////////////////////////

/**This class describes the T.120 protocol handler.
 */
class OpalT120Protocol : public PObject
{
    PCLASSINFO(OpalT120Protocol, PObject);
  public:
    enum {
      DefaultTcpPort = 1503
    };

  /**@name Construction */
  //@{
    /**Create a new protocol handler.
     */
    OpalT120Protocol();
  //@}

  /**@name Operations */
  //@{
    /**Handle the origination of a T.120 connection.
      */
    virtual BOOL Originate(
      OpalTransport & transport
    );

    /**Handle the origination of a T.120 connection.
      */
    virtual BOOL Answer(
      OpalTransport & transport
    );

    /**Handle incoming T.120 connection.

       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL HandleConnect(
      const MCS_ConnectMCSPDU & pdu
    );

    /**Handle incoming T.120 packet.

       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL HandleDomain(
      const MCS_DomainMCSPDU & pdu
    );
  //@}
};


#endif // __OPAL_T120PROTO_H


/////////////////////////////////////////////////////////////////////////////
