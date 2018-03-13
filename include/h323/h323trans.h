/*
 * h323trans.h
 *
 * H.323 Transactor handler
 *
 * Open H323 Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 */

#ifndef OPAL_H323_H323TRANS_H
#define OPAL_H323_H323TRANS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_H323

#include <h323/transaddr.h>
#include <h323/h235auth.h>

#include <ptclib/asner.h>


class H323EndPoint;


class H323TransactionPDU {
  public:
    H323TransactionPDU();
    H323TransactionPDU(const H235Authenticators & auth);

    virtual ~H323TransactionPDU() { }

    virtual PBoolean Read(H323Transport & transport);
    virtual PBoolean Write(H323Transport & transport);

    virtual PASN_Object & GetPDU() = 0;
    virtual PASN_Choice & GetChoice() = 0;
    virtual const PASN_Object & GetPDU() const = 0;
    virtual const PASN_Choice & GetChoice() const = 0;
    virtual unsigned GetSequenceNumber() const = 0;
    virtual unsigned GetRequestInProgressDelay() const = 0;
#if PTRACING
    virtual const char * GetProtocolName() const = 0;
#endif
    virtual H323TransactionPDU * ClonePDU() const = 0;
    virtual void DeletePDU() = 0;

    const H235Authenticators & GetAuthenticators() const { return authenticators; }
    H235Authenticators & GetAuthenticators() { return authenticators; }
    void SetAuthenticators(
      const H235Authenticators & auth
    ) { authenticators = auth; }

    template <class RAS> H235Authenticator::ValidationResult Validate(const RAS & ras) const
    {
      return authenticators.ValidatePDU(*this, ras, rawPDU);
    }

    template <class PDU> void Prepare(PDU & pdu)
    {
      authenticators.PreparePDU(*this, pdu);
    }

    const PBYTEArray & GetRawPDU() const { return rawPDU; }

  protected:
    mutable H235Authenticators authenticators;
    PPER_Stream rawPDU;
};


///////////////////////////////////////////////////////////

class H323Transactor : public PObject
{
  PCLASSINFO(H323Transactor, PObject);
  public:
  /**@name Construction */
  //@{

    /**Create a new protocol handler.
     */
    H323Transactor(
      H323EndPoint & endpoint,   ///<  Endpoint gatekeeper is associated with.
      H323Transport * transport, ///<  Transport over which to communicate.
      WORD localPort,                     ///<  Local port to listen on
      WORD remotePort                     ///<  Remote port to connect on
    );
    H323Transactor(
      H323EndPoint & endpoint,   ///<  Endpoint gatekeeper is associated with.
      const H323TransportAddress & iface, ///<  Local interface over which to communicate.
      WORD localPort,                     ///<  Local port to listen on
      WORD remotePort                     ///<  Remote port to connect on
    );

    /**Destroy protocol handler.
     */
    ~H323Transactor();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Print the name of the gatekeeper.
      */
    void PrintOn(
      ostream & strm    ///<  Stream to print to.
    ) const;
  //@}

  /**@name new operations */
  //@{
    /**Set a new transport for use by the transactor.
      */
    PBoolean SetTransport(
      const H323TransportAddress & iface ///<  Local interface for transport
    );

    /**Start the channel processing transactions
      */
    virtual PBoolean StartChannel();

    /**Stop the channel processing transactions.
       Must be called in each descendants destructor.
      */
    virtual void StopChannel();

    /**Create the transaction PDU for reading.
      */
    virtual H323TransactionPDU * CreateTransactionPDU() const = 0;

    /**Handle and dispatch a transaction PDU
      */
    virtual PBoolean HandleTransaction(
      const PASN_Object & rawPDU
    ) = 0;

    /**Allow for modifications to PDU on send.
      */
    virtual void OnSendingPDU(
      PASN_Object & rawPDU
    ) = 0;

    /**Write PDU to transport after executing callback.
      */
    virtual PBoolean WritePDU(
      H323TransactionPDU & pdu
    );

    /**Write PDU to transport after executing callback.
      */
    virtual PBoolean WriteTo(
      H323TransactionPDU & pdu,
      const H323TransportAddressArray & addresses,
      PBoolean callback = true
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the gatekeepers associated endpoint.
      */
    H323EndPoint & GetEndPoint() const { return m_endpoint; }

    /**Get the gatekeepers transport channel.
      */
    H323Transport & GetTransport() const { return *m_transport; }

    /**Set flag to check all crypto tokens on responses.
      */
    void SetCheckResponseCryptoTokens(
      PBoolean value    ///<  New value for checking crypto tokens.
    ) { m_checkResponseCryptoTokens = value; }

    /**Get flag to check all crypto tokens on responses.
      */
    PBoolean GetCheckResponseCryptoTokens() { return m_checkResponseCryptoTokens; }
  //@}

  protected:
    void Construct();

    unsigned GetNextSequenceNumber();
    PBoolean SetUpCallSignalAddresses(
      H225_ArrayOf_TransportAddress & addresses
    );

    //Background thread handler.
    PDECLARE_NOTIFIER(PThread, H323Transactor, HandleTransactions);
	
    class Request : public PObject
    {
        PCLASSINFO(Request, PObject);
      public:
        Request(
          unsigned seqNum,
          H323TransactionPDU & pdu,
          const H323TransportAddressArray & addresses = H323TransportAddressArray()
        );

        PBoolean Poll(H323Transactor &,
                      unsigned numRetries = 0,
                      const PTimeInterval & timeout = 0);
        void CheckResponse(unsigned, const PASN_Choice *);
        void OnReceiveRIP(unsigned milliseconds);

        unsigned                  m_sequenceNumber;
        H323TransactionPDU      & m_requestPDU;
        H323TransportAddressArray m_requestAddresses;

        // Inter-thread transfer variables
        unsigned m_rejectReason;
        PObject * m_responseInfo;

        enum {
          AwaitingResponse,
          ConfirmReceived,
          RejectReceived,
          TryAlternate,
          BadCryptoTokens,
          RequestInProgress,
          NoResponseReceived
        } m_responseResult;

        PTimeInterval m_whenResponseExpected;
        PSyncPoint    m_responseHandled;
        PDECLARE_MUTEX(m_responseMutex);
    };

    virtual PBoolean MakeRequest(
      Request & request
    );
    PBoolean CheckForResponse(
      unsigned,
      unsigned,
      const PASN_Choice * = NULL
    );
    PBoolean HandleRequestInProgress(
      const H323TransactionPDU & pdu,
      unsigned delay
    );
    bool CheckCryptoTokens1(const H323TransactionPDU & pdu);
    bool CheckCryptoTokens2();
    template <class RAS> bool CheckCryptoTokens(
      const H323TransactionPDU & pdu,
      const RAS & ras
    ) { return CheckCryptoTokens1(pdu) || (pdu.Validate(ras) == H235Authenticator::e_OK || CheckCryptoTokens2()); }

    void AgeResponses();
    PBoolean SendCachedResponse(
      const H323TransactionPDU & pdu
    );

    class Response : public PString
    {
        PCLASSINFO(Response, PString);
      public:
        Response(const H323TransportAddress & addr, unsigned seqNum);
        ~Response();

        void SetPDU(const H323TransactionPDU & pdu);
        PBoolean SendCachedResponse(H323Transport & transport);

        PTime                m_lastUsedTime;
        PTimeInterval        m_retirementAge;
        H323TransactionPDU * m_replyPDU;
    };

    // Configuration variables
    H323EndPoint  & m_endpoint;
    WORD            m_defaultLocalPort;
    WORD            m_defaultRemotePort;
    H323Transport * m_transport;
    bool            m_checkResponseCryptoTokens;

    atomic<uint16_t> m_nextSequenceNumber;

    PDictionary<POrdinalKey, Request> m_requests;
    PDECLARE_MUTEX(                   m_requestsMutex);
    Request                         * m_lastRequest;

    PDECLARE_MUTEX(       m_pduWriteMutex);
    PSortedList<Response> m_responses;
};


////////////////////////////////////////////////////////////////////////////////////

class H323Transaction : public PObject
{
    PCLASSINFO(H323Transaction, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new transaction handler.
     */
    H323Transaction(
      H323Transactor & transactor,
      const H323TransactionPDU & requestToCopy,
      H323TransactionPDU * confirm,
      H323TransactionPDU * reject
    );
    ~H323Transaction();
  //@}

    enum Response {
      Ignore = -2,
      Reject = -1,
      Confirm = 0
    };
    inline static Response InProgress(unsigned time) { return (Response)(time&0xffff); }

    virtual H323TransactionPDU * CreateRIP(
      unsigned sequenceNumber,
      unsigned delay
    ) const = 0;

    PBoolean HandlePDU();

    virtual PBoolean WritePDU(
      H323TransactionPDU & pdu
    );

    virtual void PrepareConfirm() { }

    virtual bool CheckCryptoTokens();

#if PTRACING
    virtual const char * GetName() const = 0;
#endif
    virtual H235Authenticator::ValidationResult ValidatePDU() const = 0;
    virtual unsigned GetSecurityRejectTag() const { return 0; }
    virtual void SetRejectReason(
      unsigned reasonCode
    ) = 0;

    PBoolean IsFastResponseRequired() const { return m_fastResponseRequired && m_canSendRIP; }
    PBoolean CanSendRIP() const { return m_canSendRIP; }
    H323TransportAddress GetReplyAddress() const { return m_replyAddresses[0]; }
    const H323TransportAddressArray & GetReplyAddresses() const { return m_replyAddresses; }
    PBoolean IsBehindNAT() const { return m_isBehindNAT; }
    H323Transactor & GetTransactor() const { return m_transactor; }
    H235Authenticator::ValidationResult GetAuthenticatorResult() const { return m_authenticatorResult; }
    void SetAuthenticators(const H235Authenticators & auth) { m_authenticators = auth; }

  protected:
    virtual Response OnHandlePDU() = 0;
    PDECLARE_NOTIFIER(PThread, H323Transaction, SlowHandler);

    H323Transactor          & m_transactor;
    unsigned                  m_requestSequenceNumber;
    H323TransportAddressArray m_replyAddresses;
    bool                      m_fastResponseRequired;
    H323TransactionPDU      * m_request;
    H323TransactionPDU      * m_confirm;
    H323TransactionPDU      * m_reject;

    H235Authenticators                  m_authenticators;
    H235Authenticator::ValidationResult m_authenticatorResult;
    bool                                m_isBehindNAT;
    bool                                m_canSendRIP;
};


///////////////////////////////////////////////////////////

class H323TransactionServer : public PObject
{
  PCLASSINFO(H323TransactionServer, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new gatekeeper.
     */
    H323TransactionServer(
      H323EndPoint & endpoint
    );

    /**Destroy gatekeeper.
     */
    ~H323TransactionServer();
  //@}

    virtual WORD GetDefaultUdpPort() = 0;

  /**@name Access functions */
  //@{
    /**Get the owner endpoint.
     */
    H323EndPoint & GetOwnerEndPoint() const { return m_ownerEndPoint; }

  /**@name Protocol Handler Operations */
  //@{
    /**Add listeners to the transaction server.
       If a listener already exists on the interface specified in the list
       then it is ignored. If a listener does not yet exist a new one is
       created and if a listener is running that is not in the list then it
       is stopped and removed.

       If the array is empty then the string "*" is assumed which will listen
       on the standard UDP port on INADDR_ANY.

       Returns true if at least one interface was successfully started.
      */
    PBoolean AddListeners(
      const H323TransportAddressArray & ifaces ///<  Interfaces to listen on.
    );

    /**Add a gatekeeper listener to this gatekeeper server given the
       transport address for the local interface.
      */
    PBoolean AddListener(
      const H323TransportAddress & interfaceName
    );

    /**Add a gatekeeper listener to this gatekeeper server given the transport.
       Note that the transport is then owned by the listener and will be
       deleted automatically when the listener is destroyed. Note also the
       transport is deleted if this function returns false and no listener was
       created.
      */
    PBoolean AddListener(
      H323Transport * transport
    );

    /**Add a gatekeeper listener to this gatekeeper server.
       Note that the gatekeeper listener is then owned by the gatekeeper
       server and will be deleted automatically when the listener is removed.
       Note also the listener is deleted if this function returns false and
       the listener was not used.
      */
    PBoolean AddListener(
      H323Transactor * listener
    );

    /**Create a new H323GatkeeperListener.
       The user woiuld not usually use this function as it is used internally
       by the server when new listeners are added by H323TransportAddress.

       However, a user may override this function to create objects that are
       user defined descendants of H323GatekeeperListener so the user can
       maintain extra information on a interface by interface basis.
      */
    virtual H323Transactor * CreateListener(
      H323Transport * transport  ///<  Transport for listener
    ) = 0;

    /**Remove a gatekeeper listener from this gatekeeper server.
       The gatekeeper listener is automatically deleted.
      */
    PBoolean RemoveListener(
      H323Transactor * listener
    );

    PBoolean SetUpCallSignalAddresses(H225_ArrayOf_TransportAddress & addresses);
  //@}

  protected:
    H323EndPoint & m_ownerEndPoint;

    PThread      * m_monitorThread;
    PSyncPoint     m_monitorExit;

    PDECLARE_MUTEX(m_mutex);

    PLIST(ListenerList, H323Transactor);
    ListenerList m_listeners;
};


#endif // OPAL_H323

#endif // OPAL_H323_H323TRANS_H


/////////////////////////////////////////////////////////////////////////////
