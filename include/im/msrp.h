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
#include <ptclib/inetprot.h>
#include <ptclib/guid.h>
#include <ptclib/mime.h>

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
//  Ancestor for MSRP protocol
//

class MSRPProtocol : public PInternetProtocol
{
  public:
    enum Commands {
      SEND,  
      REPORT, 
      NumCommands
    };

    static const int MaximumMessageLength = 1024;

    class Message
    {
      public: 
        struct Chunk {
          Chunk(const PString & id, unsigned f, unsigned t)
            : m_chunkId(id), m_rangeFrom(f), m_rangeTo(t) { }

          PString m_chunkId;
          unsigned m_rangeFrom;
          unsigned m_rangeTo;
        };
        typedef std::vector<Chunk> ChunkList;
        ChunkList m_chunks;

        PString m_id;
        PURL    m_fromURL;
        PURL    m_toURL;
        PString m_contentType;
        unsigned m_length;
    };

    MSRPProtocol();

    bool SendMessage(
      const PURL & from, 
      const PURL & to,
      const PString & text,
      const PString & contentType,
      PString & messageId
    );

    bool SendMessage(
      const Message & message, 
      const PString & text, 
      const PString & contentType
    );

    bool SendMessage(
      const PString & transactionId, 
      const PMIMEInfo & mime, 
      const PString & body
    );

    bool ReadMessage(
      int & command, 
      PString & transactionId, 
      PMIMEInfo & mime, 
      PString & body
    );

    //typedef std::map<std::string, Message> MessageMap;
    //MessageMap m_messageMap;
    PMutex m_mutex;
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
    std::string CreateSession();

    //
    //  Dellocate an existing MSRP session ID
    //
    void DestroySession(const std::string & id);

    //
    //  return session ID as a path
    //
    PURL SessionIDToURL(const std::string & id);

    //
    // map of connections to other MSRP managers
    // indexed by scheme://remote:port;transport
    //
    class Connection : public PSafeObject {
      public:
        Connection(MSRPProtocol * protocol = NULL);
        ~Connection();

        MSRPProtocol * m_protocol;
        bool m_running;
        PThread * m_handlerThread;
        bool m_originating;
    };

    PSafePtr<Connection> OpenConnection(const PURL & localURL, const PURL & remoteURL);

    //
    //  Main listening thread for new connections
    //
    void ListenerThread();

    //
    //  Handler thread for a connection
    //
    void HandlerThread(PSafePtr<Connection> connection);

    void SetNotifier(
      const PURL & localUrl, 
      const PURL & remoteURL, 
      const PNotifier2 & notifier
    );

    void RemoveNotifier(
      const PURL & localUrl, 
      const PURL & remoteURL
    );

    //OpalManager & GetOpalManager() { return opalManager; }

    struct IncomingMSRP {
      int       m_command;
      PString   m_transactionId;
      PMIMEInfo m_mime;
      PString   m_body;
    };

  protected:
    OpalManager & opalManager;
    WORD m_listenerPort;
    PMutex mutex;
    PAtomicInteger lastID;
    PTCPSocket m_listenerSocket;
    PThread * m_listenerThread;
    OpalTransportAddress m_listenerAddress;

    PMutex m_connectionInfoMapAddMutex;
    PSafeDictionary<PString, Connection> m_connectionInfoMap;

    class CallBack : public PObject {
      PCLASSINFO(CallBack, PObject);
      public:
        CallBack(const PNotifier2 & n) : m_notifier(n) { }
        PNotifier2 m_notifier;
    };
    typedef std::map<std::string, CallBack> CallBackMap;
    CallBackMap m_callBacks;
    PMutex m_callBacksMutex;

  private:
    static OpalMSRPManager * msrp;
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

    bool Open(const PURL & remoteParty);

    virtual void Close();

    virtual PObject * Clone() const { return new OpalMSRPMediaSession(*this); }

    virtual bool IsActive() const { return true; }

    virtual bool IsRTP() const { return false; }

    virtual bool HasFailed() const { return false; }

    virtual OpalTransportAddress GetLocalMediaAddress() const;

    PURL GetLocalURL() const { return m_localUrl; }
    PURL GetRemoteURL() const { return m_remoteUrl; }
    void SetRemoteURL(const PURL & url) { m_remoteUrl = url; }

    virtual void SetRemoteMediaAddress(const OpalTransportAddress &, const OpalMediaFormatList & );

    virtual bool WriteData(      
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    PBoolean ReadData(
      BYTE * data,
      PINDEX length,
      PINDEX & read
    );

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

    OpalMSRPManager & GetManager() { return m_manager; }

    bool OpenMSRP(const PURL & remoteUrl);

    OpalMSRPManager & m_manager;
    bool m_isOriginating;
    std::string m_localMSRPSessionId;
    PURL m_localUrl, m_remoteUrl;
    PSafePtr<OpalMSRPManager::Connection> m_connectionPtr;
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

    virtual bool Open();

    virtual bool Close();

    PURL GetRemoteURL() const           { return m_msrpSession.GetRemoteURL(); }
    void SetRemoteURL(const PURL & url) { m_msrpSession.SetRemoteURL(url); }

    PDECLARE_NOTIFIER2(OpalMSRPManager, OpalMSRPMediaStream, OnReceiveMSRP);

  //@}
  protected:
    OpalMSRPMediaSession & m_msrpSession;
    PString m_remoteParty;
    RFC4103Context m_rfc4103Context;
};

//                       schemeName,user,   passwd, host,   defUser,defhost, query,  params, frags,  path,   rel,    port

#endif // OPAL_HAS_MSRP

#endif // OPAL_IM_MSRP_H

