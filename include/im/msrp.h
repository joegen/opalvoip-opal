/*
 * msrp.h
 *
 * Support for RFC 4975 Message Session Relay Protocol (MSRP)
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-13 10:24:41 +1100 (Mon, 13 Oct 2008) $
 */

#ifndef OPAL_MSRP_MSRP_H
#define OPAL_MSRP_MSRP_H

#include <ptlib.h>
#include <opal/buildopts.h>
#include <opal/rtpconn.h>
#include <opal/manager.h>

#if OPAL_SIP
#include <sip/sdp.h>
#endif


////////////////////////////////////////////////////////////////////////////
//
//  Ancestor for all MSRP media types
//

class MSRPMediaType {
  public:
    MSRPMediaType()
    { }

    virtual const char * GetMediaFormatName() const = 0;
};

////////////////////////////////////////////////////////////////////////////
//
//
//

class OpalMSRPManager : public PObject
{
  public:
    enum {
      DefaultPort = 2855
    };

    typedef PFactory<MSRPMediaType> MSRPMediaTypeFactory;

    //
    //  Create an MSRP manager. This is a singleton class
    //
    OpalMSRPManager(OpalManager & opal, WORD port = DefaultPort);
    ~OpalMSRPManager();

    //
    //  Get the local address of the MSRP manager
    //
    bool GetLocalAddress(OpalTransportAddress & addr);

    //
    //  Get the local port for the MSRP manager
    //
    bool GetLocalPort(WORD & port);

    //
    //  Allocate a new MSRP session ID
    //
    std::string AllocateID();

    //
    //  Dellocate an existing MSRP session ID
    //
    void DeallocateID(const std::string & id);

    //
    //  Main listening thread
    //
    void ThreadMain();
    OpalManager & GetOpalManager() { return opalManager; }

  protected:
    OpalManager & opalManager;
    WORD listeningPort;
    PMutex mutex;
    PAtomicInteger lastID;
    PTCPSocket listeningSocket;
    PThread * listeningThread;
    OpalTransportAddress listeningAddress;

    struct SessionInfo {
    };

    typedef std::map<std::string, SessionInfo> SessionInfoMap;
    SessionInfoMap sessionInfoMap;

  private:
    static OpalMSRPManager * msrp;
};

////////////////////////////////////////////////////////////////////////////

class MSRPSession 
{
  public:
    MSRPSession(OpalMSRPManager & _manager);
    ~MSRPSession();

    virtual SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);

    OpalMSRPManager & GetManager() { return manager; }

  protected:
    OpalMSRPManager & manager;
    std::string msrpSessionId;
    PString url;
    PTCPSocket msrpSocket;
};


/** Class for carrying MSRP session information
  */
class OpalMSRPMediaSession : public OpalMediaSession
{
  PCLASSINFO(OpalMSRPMediaSession, OpalMediaSession);
  public:
    OpalMSRPMediaSession(OpalConnection & connection, unsigned sessionId);
    OpalMSRPMediaSession(const OpalMSRPMediaSession & _obj);

    virtual void Close();

    virtual PObject * Clone() const { return new OpalMSRPMediaSession(*this); }

    virtual bool IsActive() const { return msrpSession != NULL; }

    virtual bool IsRTP() const { return false; }

    virtual bool HasFailed() const { return false; }

    virtual OpalTransportAddress GetLocalMediaAddress() const;

    virtual SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    );

    MSRPSession * msrpSession;
};

#endif // OPAL_MSRP_MSRP_H
