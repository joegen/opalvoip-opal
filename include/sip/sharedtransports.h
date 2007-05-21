
/*
 * sharedtransports.h
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
 * $Log: sharedtransports.h,v $
 * Revision 1.2  2007/05/21 04:30:30  dereksmithies
 * put #ifndef _PTLIB_H protection around the include of ptlib.h
 *
 * Revision 1.1  2007/05/15 20:42:12  dsandras
 * Added basic class allowing to share transports between various SIP
 * handlers. This class should be deprecated soon with the new OpalTransport
 * changes. Code contributed by NOVACOM (http://www.novacom.be) thanks to
 * EuroWeb (http://www.euroweb.hu).
 *
 *
 */

#ifndef __OPAL_SHARED_TRANSPORTS_H
#define __OPAL_SHARED_TRANSPORTS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <ptlib/safecoll.h>

#include <opal/transports.h>

class SIPEndPoint;


/* This class should disappear soon */
class SharedTransport : public PSafeObject
{
public :

  SharedTransport (const PString k, SIPEndPoint & ep); 

  ~SharedTransport (); 

  // The Trnasport is only deleted when the SharedTrasport is deleted, which can not 
  // happen as long as a PSafePtr is being held on it (PSafeObject)
  OpalTransport *GetTransport ();

  void IncrementReferenceCount ();

  void DecrementReferenceCount ();

  BOOL CanBeDeleted ();

  const PString GetKey ();
  
  void PrintOn(ostream & strm) const;

private :
  PMutex referenceCountMutex;
  PMutex transportMutex;
  OpalTransport *transport;
  SIPEndPoint & endpoint;
  const PString key;
  unsigned referenceCount;
};

#endif
