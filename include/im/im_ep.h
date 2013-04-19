/*
 * im_ep.h
 *
 * Endpoint for Instant Messaging
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

#ifndef OPAL_IM_EP_H
#define OPAL_IM_EP_H

#include <ptlib.h>
#include <opal_config.h>

#if OPAL_HAS_IM

#include <opal/endpoint.h>


/**Endpoint for IM connections.
 */
class OpalIMEndPoint : public OpalEndPoint
{
  public:
    static const PCaselessString & Prefix();

  /**@name Construction */
  //@{
    OpalIMEndPoint(OpalManager & manager);
    ~OpalIMEndPoint();
  //@}


  /**@name Overrides from OpalEndPoint */
  //@{
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,          ///<  Owner of connection
      const PString & party,    ///<  Remote party to call
      void * userData = NULL,          ///<  Arbitrary data to pass to connection
      unsigned int options = 0,     ///<  Options bit mask to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL ///< Options to pass to connection
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /** Look for old conversations and remove them.
        Generally only used internally.
        This will call notifiers indicating converstaion end.
      */
    virtual PBoolean GarbageCollection();

    /**Shut down all conversations.
      */
    virtual void ShutDown();
  //@}


  /**@name Context management */
  //@{
    /**Create a Instant Messaging context based on URL scheme.
       This uses a factory to create an approriate concrete type for the
       protocol indicated by the scheme of the \p remoteURL.

       The \p localURL may be empty and will be derived from the \p remoteURL
       and the default user name from the manager.
      */
    PSafePtr<OpalIMContext> CreateContext(
      const PURL & localURL,      ///< Local URL for conversation
      const PURL & remoteURL,     ///< Remote URL for conversation
      const char * scheme = NULL  ///< Scheme to use, default to remoteURL.GetScheme()
    ) { return InternalCreateContext(localURL, remoteURL, scheme, false, NULL); }

    /**Create a Instant Messaging context based on an existing call.
      */
    PSafePtr<OpalIMContext> CreateContext(
      OpalCall & call
    );

    /** Remove a new context.
        Generally only used internally.
        This will call notifiers indicating converstaion end.
      */
    void RemoveContext(OpalIMContext * context, bool byRemote);

    /** Find a context given a key.
        Generally only used internally.
      */
    PSafePtr<OpalIMContext> FindContextByIdWithLock(
      const PString & key,
      PSafetyMode mode = PSafeReadWrite
    );

    /** Find a context given a from/to addresses.
        Generally only used internally.
      */
    PSafePtr<OpalIMContext> FindContextByNamesWithLock(
      const PString & local, 
      const PString & remote, 
      PSafetyMode mode = PSafeReadWrite
    );

    /** Find a context given a message.
        Generally only used internally.
      */
    PSafePtr<OpalIMContext> FindContextForMessageWithLock(
      OpalIM & im,
      OpalConnection * conn = NULL
    );
  //@}


  /**@name Conversation notification */
  //@{
    /** Called when a context state changes, e.g. openign or closing.
        Default implementation calls the notifier, if set. If no notifier is
        set, then the OpalManager::OnConversation() function is called.
      */
    virtual void OnConversation(
      const OpalIMContext::ConversationInfo & info
    );

    /// Type for converstaion notifiers
    typedef PNotifierTemplate<OpalIMContext::ConversationInfo> ConversationNotifier;

    /// Macro to declare correctly typed converstaion notifier
    #define PDECLARE_ConversationNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMContext, cls, fn, OpalIMContext::ConversationInfo)

    /// Macro to declare correctly typed asynchronous converstaion notifier
    #define PDECLARE_ASYNC_ConversationNotifier(cls, fn) PDECLARE_ASYNC_NOTIFIER2(OpalIMContext, cls, fn, OpalIMContext::ConversationInfo)

    /// Macro to create correctly typed converstaion notifier
    #define PCREATE_ConversationNotifier(fn) PCREATE_NOTIFIER2(fn, OpalIMContext::ConversationInfo)

    /** Set the notifier for the OnConversation() function.
        The notifier can be called only for when a specific scheme is being
        used. If \p scheme is "*" then all scemes will call that notifier.

        More than one notifier can be applied and all are called.
      */
    void AddNotifier(
      const ConversationNotifier & notifier,  ///< New notifier to add
      const PString & scheme                  ///< Scheme to be notified about
    );

    /** Remove notifier for the OnConversation() function.
      */
    bool RemoveNotifier(const ConversationNotifier & notifier, const PString & scheme);
  //@}


    OpalIMContext::MessageDisposition OnRawMessageReceived(
      OpalIM & message,
      OpalConnection * connnection,
      PString & errorInfo
    );

    OpalManager & GetManager() const { return m_manager; }

  protected:
    PSafePtr<OpalIMContext> InternalCreateContext(
      const PURL & localURL,
      const PURL & remoteURL,
      const char * scheme,
      bool byRemote,
      OpalCall * call
    );

    OpalManager & m_manager;
    typedef PSafeDictionary<PString, OpalIMContext> ContextsByConversationId;
    ContextsByConversationId m_contextsByConversationId;

    PMutex m_contextsByNamesMutex;
    typedef std::multimap<PString, PString> ContextsByNames;
    ContextsByNames m_contextsByNames;

    PMutex m_notifierMutex;
    typedef std::multimap<PString, ConversationNotifier> ConversationMap;
    ConversationMap m_notifiers;

    PTime m_lastGarbageCollection;
    bool m_deleting;
};


/** Instant messaging connection.
    This class is primarily for internal use where a IM context requires and
    active connection, e.g. SIP with T.140.
 */
class OpalIMConnection : public OpalConnection
{
    PCLASSINFO(OpalIMConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalIMConnection(
      OpalCall & call,              ///<  Owner call for connection
      OpalIMEndPoint & endpoint,    ///<  Owner endpoint for connection
      void * userData,              ///<  Arbitrary data to pass to connection
      unsigned options,             ///< Option bit mask to pass to connection
      OpalConnection::StringOptions * stringOptions ///< Options to pass to connection
    );
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual PBoolean IsNetworkConnection() const { return false; }

    /**Callback for outgoing connection, it is invoked after SetUpConnection
       This function allows the application to set up some parameters or to log some messages
     */
    virtual PBoolean OnSetUpConnection();

    /**A call back function whenever a connection is established.
       This indicates that a connection to an endpoint was established. This
       usually occurs after OnConnected() and indicates that the connection
       is both connected and has media flowing.

       In the context of H.323 this means that the signalling and control
       channels are open and the TerminalCapabilitySet and MasterSlave
       negotiations are complete.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnEstablished();

    /**Clean up the termination of the connection.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       The default behaviour closes the context the calls the ancestor.
      */
    virtual void OnReleased();
  //@}

  protected:
    OpalIMContext * m_context;
};


#endif // OPAL_HAS_IM

#endif // OPAL_IM_EP_H
