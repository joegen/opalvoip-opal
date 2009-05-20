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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_IM_MSRP_H
#define OPAL_IM_MSRP_H

#include <ptlib.h>
#include <opal/buildopts.h>
#include <opal/rtpconn.h>
#include <opal/manager.h>
#include <opal/mediastrm.h>
#include <opal/mediatype.h>
#include <im/im.h>

#if OPAL_SIP
#include <sip/sdp.h>
#endif

#if OPAL_HAS_MSRP

class OpalMSRPMediaType : public OpalIMMediaType 
{
  public:
    OpalMSRPMediaType();
    virtual OpalMediaSession * CreateMediaSession(OpalConnection & conn, unsigned sessionID) const;

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};

////////////////////////////////////////////////////////////////////////////
//
//  Ancestor for all MSRP encoding types
//

class OpalMSRPEncoding {
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
    std::string OpenSession();

    //
    //  Dellocate an existing MSRP session ID
    //
    void CloseSession(const std::string & id);

    //
    //  return session ID as a path
    //
    std::string SessionIDToPath(const std::string & id);

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

    struct ConnectionInfo {
      PTCPSocket * socket;
    };
    typedef std::map<std::string, ConnectionInfo> ConnectionInfoMap;
    ConnectionInfoMap connectionInfoMap;

  private:
    static OpalMSRPManager * msrp;
};

////////////////////////////////////////////////////////////////////////////

class MSRPSession 
{
  public:
    MSRPSession(OpalMSRPManager & _manager);
    ~MSRPSession();

#if OPAL_SIP
    virtual SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif

    OpalMSRPManager & GetManager() { return manager; }

    PString GetURL() const { return url; }

  protected:
    OpalMSRPManager & manager;
    std::string msrpSessionId;
    PString url;
};

////////////////////////////////////////////////////////////////////////////

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

    virtual void SetRemoteMediaAddress(const OpalTransportAddress &, const OpalMediaFormatList & );

#if OPAL_SIP
    virtual SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    );
#endif

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      PBoolean isSource
    );

    MSRPSession * msrpSession;
};

////////////////////////////////////////////////////////////////////////////

class OpalMSRPMediaStream : public OpalIMMediaStream
{
  public:
    OpalMSRPMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      OpalMSRPMediaSession & msrpSession

    );

    ~OpalMSRPMediaStream();

    /**Read raw media data from the source media stream.
       The default behaviour reads from the PChannel object.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the PChannel object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Close the media stream.

       Closes the associated PChannel.
      */
    virtual PBoolean Close();
  //@}
  protected:
    OpalMSRPMediaSession & m_msrpSession;
};

#endif // OPAL_HAS_MSRP

#endif // OPAL_IM_MSRP_H

