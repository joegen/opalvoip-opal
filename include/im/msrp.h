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

#if OPAL_HAS_MSRP

#include <opal/mediastrm.h>
#include <opal/mediasession.h>
#include <ptclib/url.h>
#include <ptclib/inetprot.h>


class OpalManager;


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

    static const unsigned MaximumMessageLength = 1024;

    class Message
    {
      public: 
        struct Chunk {
          Chunk(const PString & id, unsigned from, unsigned len)
            : m_chunkId(id), m_rangeFrom(from + 1), m_rangeTo(from + len) { }

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

    bool SendSEND(
      const PURL & from, 
      const PURL & to,
      const PString & text,
      const PString & contentType,
      PString & messageId
    );

    bool SendChunk(
      const PString & transactionId, 
      const PString toUrl,
      const PString fromUrl,
      const PMIMEInfo & mime, 
      const PString & body
    );

    bool SendResponse(const PString & chunkId, 
                             unsigned response,
                      const PString & text,
                      const PString & toUrl,
                      const PString & fromUrl);

    bool SendREPORT(const PString & chunkId, 
                    const PString & toUrl,
                    const PString & fromUrl,
                  const PMIMEInfo & mime);

    bool ReadMessage(
      int & command,
      PString & chunkId,
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
    //  Get the local port for the MSRP manager
    //
    bool GetLocalPort(WORD & port);

    //
    // Information about a connection to another MSRP manager
    //
    class Connection : public PSafeObject {
      public:
        Connection(OpalMSRPManager & manager, const std::string & key, MSRPProtocol * protocol = NULL);
        ~Connection();

        //
        //  Start handler thread for a connection
        //
        void StartHandler();

        //
        //  Handler thread for a connection
        //
        void HandlerThread();

        OpalMSRPManager & m_manager;
        std::string m_key;
        MSRPProtocol * m_protocol;
        bool m_running;
        PThread * m_handlerThread;
        bool m_originating;
        PAtomicInteger m_refCount;
    };

    //
    //  Get the connection to use for communicating with a remote URL
    //
    PSafePtr<Connection> OpenConnection(
      const PURL & localURL, 
      const PURL & remoteURL
    );

    //
    //  close a connection
    //
    bool CloseConnection(
      PSafePtr<OpalMSRPManager::Connection> & connection
    );

    //
    //  Create a new MSRP session ID
    //
    std::string CreateSessionID();

    //
    //  return session ID as a path
    //
    PURL SessionIDToURL(const OpalTransportAddress & addr, const std::string & id);

    //
    //  Main listening thread for new connections
    //
    void ListenerThread();

    struct IncomingMSRP {
      int       m_command;
      PString   m_chunkId;
      PMIMEInfo m_mime;
      PString   m_body;
      PSafePtr<Connection> m_connection;
    };

    //
    // dispatch an incoming MSRP message to the correct callback
    //
    void DispatchMessage(
      IncomingMSRP & incomingMsg
    );

    typedef PNotifierTemplate<IncomingMSRP &> CallBack;

    void SetNotifier(
      const PURL & localUrl, 
      const PURL & remoteURL, 
      const CallBack & notifier
    );

    void RemoveNotifier(
      const PURL & localUrl, 
      const PURL & remoteURL
    );

    OpalManager & GetOpalManager() { return opalManager; }

  protected:
    OpalManager & opalManager;
    WORD m_listenerPort;
    PMutex mutex;
    PAtomicInteger lastID;
    PTCPSocket m_listenerSocket;
    PThread * m_listenerThread;

    PMutex m_connectionInfoMapAddMutex;
    typedef std::map<PString, PSafePtr<Connection> > ConnectionInfoMapType;
    ConnectionInfoMapType m_connectionInfoMap;

    typedef std::map<PString, CallBack> CallBackMap;
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
    static const PCaselessString & TCP_MSRP();

    OpalMSRPMediaSession(const Init & init);
    ~OpalMSRPMediaSession();

    virtual PObject * Clone() const { return new OpalMSRPMediaSession(*this); }

    virtual const PCaselessString & GetSessionType() const { return TCP_MSRP(); }
    virtual bool Open(const PString & localInterface, const OpalTransportAddress & remoteAddress, bool isMediaAddress);
    virtual bool Close();
    virtual OpalTransportAddress GetLocalAddress(bool isMediaAddress = true) const;
    virtual OpalTransportAddress GetRemoteAddress(bool isMediaAddress = true) const;
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress = true);
#if OPAL_SIP
    virtual SDPMediaDescription * CreateSDPMediaDescription();
#endif

    PURL GetLocalURL() const { return m_localUrl; }
    PURL GetRemoteURL() const { return m_remoteUrl; }
    void SetRemoteURL(const PURL & url) { m_remoteUrl = url; }

    virtual bool WritePacket(      
      RTP_DataFrame & frame
    );

    PBoolean ReadData(
      BYTE * data,
      PINDEX length,
      PINDEX & read
    );

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      PBoolean isSource
    );

    OpalMSRPManager & GetManager() { return m_manager; }

    bool OpenMSRP(const PURL & remoteUrl);
    void CloseMSRP();

    void SetConnection(PSafePtr<OpalMSRPManager::Connection> & conn);

    OpalMSRPManager & m_manager;
    bool m_isOriginating;
    std::string m_localMSRPSessionId;
    PURL m_localUrl;
    PURL m_remoteUrl;
    PSafePtr<OpalMSRPManager::Connection> m_connectionPtr;
    OpalTransportAddress m_remoteAddress;

  private:
    OpalMSRPMediaSession(const OpalMSRPMediaSession & other)
      : OpalMediaSession(Init(other.m_connection, other.m_sessionId, other.m_mediaType, false)), m_manager(other.m_manager) { }
    void operator=(const OpalMSRPMediaSession &) { }
};


////////////////////////////////////////////////////////////////////////////

class OpalMSRPMediaStream : public OpalMediaStream
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

    virtual PBoolean IsSynchronous() const         { return false; }
    virtual PBoolean RequiresPatchThread() const   { return IsSink(); }

    /**Read raw media data from the source media stream.
       The default behaviour reads from the PChannel object.
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & frame
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the PChannel object.
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & frame
    );

    virtual bool Open();

    PURL GetRemoteURL() const           { return m_msrpSession.GetRemoteURL(); }
    void SetRemoteURL(const PURL & url) { m_msrpSession.SetRemoteURL(url); }

    PDECLARE_NOTIFIER2(OpalMSRPManager, OpalMSRPMediaStream, OnReceiveMSRP, OpalMSRPManager::IncomingMSRP &);
  //@}

  protected:
    virtual void InternalClose() { }

    OpalMSRPMediaSession & m_msrpSession;
    PString                m_remoteParty;
};


#endif // OPAL_HAS_MSRP

#endif // OPAL_IM_MSRP_H

