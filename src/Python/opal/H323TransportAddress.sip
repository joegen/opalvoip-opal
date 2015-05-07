/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <opal/transports.h>
#include <h323/transaddr.h>
%End

typedef OpalListener H323Listener;
typedef PList<OpalListenerTCP> H323ListenerList;
typedef OpalTransport H323Transport;
typedef OpalTransportUDP H323TransportUDP;


///////////////////////////////////////////////////////////////////////////////

/**Transport address for H.323.
   This adds functions to the basic OpalTransportAddress for conversions to
   and from H.225 and H.245 PDU structures.
 */
class H323TransportAddress : OpalTransportAddress
{
  public:
    H323TransportAddress();
    H323TransportAddress(const char * addr, WORD port = 0, const char * proto = NULL);
    H323TransportAddress(const PString & addr, WORD port = 0, const char * proto = NULL);
    H323TransportAddress(const OpalTransportAddress & addr);
    H323TransportAddress(PIPSocket::Address ip, WORD port, const char * proto = NULL);
};


