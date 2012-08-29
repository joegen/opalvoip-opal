/*
 * main.h
 *
 * OPAL application source file for EXternal RTP demonstration for OPAL
 *
 * Copyright (c) 2011 Vox Lucida Pty. Ltd.
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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _ExtRTP_MAIN_H
#define _ExtRTP_MAIN_H


#define EXTERNAL_SCHEME "ext"

class MyLocalEndPoint : public OpalLocalEndPoint
{
    PCLASSINFO(MyLocalEndPoint, OpalLocalEndPoint)

  public:
    MyLocalEndPoint(OpalManager & manager)
      : OpalLocalEndPoint(manager, EXTERNAL_SCHEME) { }

    virtual bool OnOutgoingCall(
      const OpalLocalConnection & connection
    );
    virtual bool OnIncomingCall(
      OpalLocalConnection & connection
    );

    virtual OpalLocalConnection * CreateConnection(
      OpalCall & call,    ///<  Owner of connection
      void * userData,    ///<  Arbitrary data to pass to connection
      unsigned options,   ///< Option bit mask to pass to connection
      OpalConnection::StringOptions * stringOptions ///< Options to pass to connection
    );
};


class MyLocalConnection : public OpalLocalConnection
{
    PCLASSINFO(MyLocalConnection, OpalLocalConnection)

  public:
    MyLocalConnection(
      OpalCall & call,
      OpalLocalEndPoint & endpoint,
      void * userData,
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    ) : OpalLocalConnection(call, endpoint, userData, options, stringOptions, 'X') { }

    virtual bool GetMediaTransportAddresses(
      const OpalMediaType & mediaType,
      OpalTransportAddressArray & transports
    ) const;
};


class MyManager : public OpalManagerConsole
{
    PCLASSINFO(MyManager, OpalManagerConsole)

  public:
    virtual MediaTransferMode GetMediaTransferMode(const OpalConnection &, const OpalConnection &, const OpalMediaType &) const;
    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);
    virtual void OnClearedCall(OpalCall & call);

    PSyncPoint m_completed;
};


class MyProcess : public PProcess
{
    PCLASSINFO(MyProcess, PProcess)

  public:
    MyProcess();

    virtual void Main();
    virtual bool OnInterrupt(bool);

  private:
    MyManager * m_manager;
};


#endif  // _ExtRTP_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
