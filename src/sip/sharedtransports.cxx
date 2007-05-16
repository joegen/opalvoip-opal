/*
 * sharedtransports.cxx
 *
 * Session Initiation Protocol endpoint.
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
 * The Initial Developer of the Original Code is Damien Sandras. 
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: sharedtransports.cxx,v $
 * Revision 1.2  2007/05/16 01:17:07  csoutheren
 * Added new files to Windows build
 * Removed compiler warnings on Windows
 * Added backwards compatible SIP Register function
 *
 * Revision 1.1  2007/05/15 20:42:12  dsandras
 * Added basic class allowing to share transports between various SIP
 * handlers. This class should be deprecated soon with the new OpalTransport
 * changes. Code contributed by NOVACOM (http://www.novacom.be) thanks to
 * EuroWeb (http://www.euroweb.hu).
 *
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sharedtransports.h"
#endif

#include <sip/sharedtransports.h>
#include <sip/sipep.h>

SharedTransport::SharedTransport (const PString k, SIPEndPoint & ep)
: endpoint (ep), key (k)
{ 
  WORD port = 5060;
  PWaitAndSignal m(transportMutex);

  OpalTransportAddress transportAddress(key, port, "udp");
  transport = endpoint.CreateTransport(transportAddress);
  if (transport == NULL) {
    PTRACE(2, "SIP\tUnable to create transport for registrar");
  }
  referenceCount = 0;
}


SharedTransport::~SharedTransport () 
{ 
  PWaitAndSignal m(transportMutex);
  
  if (transport) {
    PTRACE(4, "SIP\tDeleting transport " << *transport);
    transport->CloseWait();
    delete transport;
  }
  
  transport = NULL;
}


OpalTransport *SharedTransport::GetTransport () 
{ 
  PWaitAndSignal m(transportMutex);
  
  return transport; 
}


void SharedTransport::IncrementReferenceCount ()
{
  PWaitAndSignal m(referenceCountMutex);
  
  referenceCount++;
}


void SharedTransport::DecrementReferenceCount ()
{
  PWaitAndSignal m(referenceCountMutex);
  
  referenceCount--;
}


BOOL SharedTransport::CanBeDeleted ()
{
  PWaitAndSignal m(referenceCountMutex);

  return (referenceCount == 0);
}


const PString SharedTransport::GetKey () 
{
  return key;
}


void SharedTransport::PrintOn(ostream & strm) const
{
  PWaitAndSignal m(transportMutex);
  if (transport)
    strm << *transport;
}
