/*
 * gkclient.h
 *
 * Gatekeeper client protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * Portions of this code were written with the assisance of funding from
 * iFace, Inc. http://www.iface.com
 *
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_H323_GKCLIENT_H
#define OPAL_H323_GKCLIENT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_H323

#include <opal/mediafmt.h>
#include <h323/h225ras.h>
#include <h323/h235auth.h>
#include <h460/h460.h>

class H323Connection;
class H225_ArrayOf_AliasAddress;
class H225_H323_UU_PDU;
class H225_AlternateGK;
class H225_ArrayOf_AlternateGK;
class H225_ArrayOf_ServiceControlSession;
class H225_FeatureSet;
class H460_FeatureSet;


///////////////////////////////////////////////////////////////////////////////

/**This class embodies the H.225.0 RAS protocol to gatekeepers.
  */
class H323Gatekeeper : public H225_RAS
{
    PCLASSINFO(H323Gatekeeper, H225_RAS);
  public:
  /**@name Construction */
  //@{
    /**Create a new gatekeeper.
     */
    H323Gatekeeper(
      H323EndPoint & endpoint,  ///<  Endpoint gatekeeper is associated with.
      H323Transport * transport       ///<  Transport over which gatekeepers communicates.
    );

    /**Destroy gatekeeper.
     */
    ~H323Gatekeeper();
  //@}
    
  /**@name Overrides from H323Transactor */
  //@{
    /**Stop the channel processing transactions.
       Must be called in each descendants destructor.
      */
    virtual void StopChannel();

    virtual PBoolean WriteTo(
      H323TransactionPDU & pdu,
      const H323TransportAddressArray & addresses,
      PBoolean callback = true
    );
  //@}

  /**@name Overrides from H225_RAS */
  //@{
    PBoolean OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm & gcf);
    PBoolean OnReceiveGatekeeperReject(const H225_GatekeeperReject & grj);
    PBoolean OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & rcf);
    PBoolean OnReceiveRegistrationReject(const H225_RegistrationReject & rrj);
    PBoolean OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & urq);
    PBoolean OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & ucf);
    PBoolean OnReceiveUnregistrationReject(const H225_UnregistrationReject & urj);
    PBoolean OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & acf);
    PBoolean OnReceiveAdmissionReject(const H225_AdmissionReject & arj);
    PBoolean OnReceiveDisengageRequest(const H225_DisengageRequest & drq);
    PBoolean OnReceiveDisengageReject(const H323RasPDU &, const H225_DisengageReject &);
    PBoolean OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & bcf);
    PBoolean OnReceiveBandwidthRequest(const H225_BandwidthRequest & brq);
    PBoolean OnReceiveInfoRequest(const H225_InfoRequest & irq);
    PBoolean OnReceiveInfoRequestResponse(const H225_InfoRequestResponse & irr);
    PBoolean OnReceiveServiceControlIndication(const H225_ServiceControlIndication &);
    PBoolean OnReceiveNonStandardMessage(const H225_NonStandardMessage &);
    void OnSendGatekeeperRequest(H225_GatekeeperRequest & grq);
    void OnSendAdmissionRequest(H225_AdmissionRequest & arq);
#if OPAL_H460
    PBoolean OnSendFeatureSet(H460_MessageType pduType, H225_FeatureSet & features) const;
    void OnReceiveFeatureSet(H460_MessageType pduType, const H225_FeatureSet & features) const;
#endif
  //@}

  /**@name Protocol operations */
  //@{
    /**Discover a gatekeeper on the local network.
     */
    PBoolean DiscoverAny();

    /**Discover a gatekeeper on the local network.
       If the identifier string is empty then the first gatekeeper to respond
       to a broadcast is used.
     */
    PBoolean DiscoverByName(
      const PString & identifier  ///<  Gatekeeper identifier to find
    );

    /**Discover a gatekeeper on the local network.
       If the address string is empty then the first gatekeeper to respond
       to a broadcast is used.
     */
    PBoolean DiscoverByAddress(
      const H323TransportAddress & address ///<  Address of gatekeeper.
    );

    /**Discover a gatekeeper on the local network.
       Combination of DiscoverByName() and DiscoverByAddress().
     */
    PBoolean DiscoverByNameAndAddress(
      const PString & identifier,
      const H323TransportAddress & address
    );

    /**Register with gatekeeper.
     */
    bool RegistrationRequest(
      bool autoReregister = true,  ///<  Automatic register on unregister
      bool didGkDiscovery = false, ///<  discovery procedure was done right before
      bool lightweight = true      ///<  Do lightweight RRQ, if possible
    );

    /**Unregister with gatekeeper.
     */
    PBoolean UnregistrationRequest(
      int reason      ///<  Reason for unregistration
    );

    /**Location request to gatekeeper.
     */
    PBoolean LocationRequest(
      const PString & alias,          ///<  Alias name we wish to find.
      H323TransportAddress & address  ///<  Resultant transport address.
    );

    /**Location request to gatekeeper.
     */
    PBoolean LocationRequest(
      const PStringList & aliases,    ///<  Alias names we wish to find.
      H323TransportAddress & address  ///<  Resultant transport address.
    );

    struct AdmissionResponse {
      AdmissionResponse();

      unsigned rejectReason;                      /// Reject reason if returns false

      PBoolean gatekeeperRouted;                      /// Flag for call is through gk
      PINDEX endpointCount;                       /// Number of endpoints that can be returned
      H323TransportAddress * transportAddress;    /// Transport address or remote endpoint.
      PBYTEArray * accessTokenData;               /// iNow Gatekeeper Access Token data

      H225_ArrayOf_AliasAddress * aliasAddresses; /// DestinationInfo to use in SETUP if not empty
      H225_ArrayOf_AliasAddress * destExtraCallInfo; /// DestinationInfo to use in SETUP if not empty
    };

    /**Admission request to gatekeeper.
     */
    PBoolean AdmissionRequest(
      H323Connection & connection,      ///<  Connection we wish to change.
      AdmissionResponse & response,     ///<  Response parameters to ARQ
      PBoolean ignorePreGrantedARQ = false  ///<  Flag to force ARQ to be sent
    );

    /**Disengage request to gatekeeper.
     */
    PBoolean DisengageRequest(
      H323Connection & connection,  ///<  Connection we wish admitted.
      unsigned reason               ///<  Reason code for disengage
    );

    /**Bandwidth request to gatekeeper.
     */
    PBoolean BandwidthRequest(
      H323Connection & connection,    ///<  Connection we wish to change.
      OpalBandwidth requestedBandwidth     ///<  New bandwidth wanted in bps
    );

    /**Send an unsolicited info response to the gatekeeper.
     */
    void InfoRequestResponse();

    /**Send an unsolicited info response to the gatekeeper.
     */
    void InfoRequestResponse(
      const H323Connection & connection  ///<  Connection to send info about
    );

    /**Send an unsolicited info response to the gatekeeper.
     */
    void InfoRequestResponse(
      const H323Connection & connection,  ///<  Connection to send info about
      const H225_H323_UU_PDU & pdu,       ///<  PDU that was sent or received
      PBoolean sent                           ///<  Flag for PDU was sent or received
    );

    /**Send a non-standard message and wait for non-standard reply.
      */
    bool NonStandardMessage(
      const PString & identifer,
      const PBYTEArray & outData,
      PBYTEArray & replyData
    );
    bool NonStandardMessage(
      const PString & identifer,
      const void * outDataPtr,
      PINDEX outDataSize,
      PBYTEArray & replyData
      ) { return NonStandardMessage(identifer, PBYTEArray((const BYTE *)outDataPtr, outDataSize), replyData); }

    /**Send a non-standard message and dont wait for a reply.
      */
    bool SendNonStandardMessage(
      const PString & identifer,
      const PBYTEArray & outData
    );

    /**Handle incoming service control session information.
     */
    virtual void OnServiceControlSessions(
      const H225_ArrayOf_ServiceControlSession & serviceControl,
      H323Connection * connection
    );
    
    /** Handle terminal alias changes
      */
    virtual void OnTerminalAliasChanged();
  //@}

  /**@name Member variable access */
  //@{
    /**Determine if the endpoint has discovered the gatekeeper.
      */
    PBoolean IsDiscoveryComplete() const { return discoveryComplete; }

    /**Determine if the endpoint is registered with the gatekeeper.
      */
    PBoolean IsRegistered() const { return m_registrationFailReason == RegistrationSuccessful; }

    enum RegistrationFailReasons {
      RegistrationSuccessful,
      UnregisteredLocally,
      UnregisteredByGatekeeper,
      GatekeeperLostRegistration,
      InvalidListener,
      DuplicateAlias,
      SecurityDenied,
      TransportError,
      TryingAlternate,
      NumRegistrationFailReasons,
      GatekeeperRejectReasonMask = 0x4000,
      RegistrationRejectReasonMask = 0x8000,
      UnregistrationRejectReasonMask = 0x10000
    };
    static PString GetRegistrationFailReasonString(RegistrationFailReasons reason);
    friend ostream & operator<<(ostream & strm, RegistrationFailReasons reason) { return strm << GetRegistrationFailReasonString(reason); }

    /**Get the registration fail reason.
     */
    RegistrationFailReasons GetRegistrationFailReason() const { return m_registrationFailReason; }

    /**Get the gatekeeper name.
       The gets the name of the gatekeeper. It will be of the form id@address
       where id is the gatekeeperIdentifier and address is the transport
       address used. If the gatekeeperIdentifier is empty the '@' is not
       included and only the transport is shown. The transport is minimised
       also, with the type removed if IP is used and the :port removed if the
       default port is used.
      */
    PString GetName() const;

    /** Get the endpoint identifier
      */
    PString GetEndpointIdentifier() const;
    void GetEndpointIdentifier(PASN_BMPString & id) const { id.SetValueRaw(m_endpointIdentifier); }

    /**Set the H.235 password in the gatekeeper.
       If no username is present then it will default to the endpoint local
       user name (ie first alias).
      */
    void SetPassword(
      const PString & password,            ///<  New password
      const PString & username = PString() ///<  Username for password
    );

    /**Get fixed, single, alias for gatekeeper.
      */
    PStringList GetAliases() const;

    /*
     * Return the call signalling address for the gatekeeper (if present)
     */
    H323TransportAddress GetGatekeeperRouteAddress() const
    { return gkRouteAddress; }
  //@}

#if OPAL_H460
    H460_FeatureSet * GetFeatures() { return m_features; }
#endif

    void ReRegisterNow();

  protected:
    bool StartGatekeeper(const H323TransportAddress & address);
    virtual bool DiscoverGatekeeper();
    unsigned SetupGatekeeperRequest(H323RasPDU & request);
	
    void Connect(const H323TransportAddress & address, const PString & gatekeeperIdentifier);
    void ClearAllCalls();

    void SetInfoRequestRate(
      const PTimeInterval & rate
    );
    void ClearInfoRequestRate();
    H225_InfoRequestResponse & BuildInfoRequestResponse(
      H323RasPDU & response,
      unsigned seqNum
    );
    PBoolean SendUnsolicitedIRR(
      H225_InfoRequestResponse & irr,
      H323RasPDU & response
    );

    void SetAlternates(
      const H225_ArrayOf_AlternateGK & alts,
      bool permanent
    );

    virtual PBoolean MakeRequest(
      Request & request
    );
    PBoolean MakeRequestWithReregister(
      Request & request,
      unsigned unregisteredTag
    );
    
    virtual H323Transport * CreateTransport(PIPSocket::Address bindng = PIPSocket::GetDefaultIpAny(), WORD port = 0, PBoolean reuseAddr = false);

    // Handling interface changes
    void OnAddInterface(const PIPSocket::InterfaceEntry & entry, PINDEX priority);
    void OnRemoveInterface(const PIPSocket::InterfaceEntry & entry, PINDEX priority);
    bool SetListenerAddresses(H225_ArrayOf_TransportAddress & pdu);

    // Gatekeeper registration state variables
    bool                    discoveryComplete;
    PStringList             m_aliases;
    PDECLARE_MUTEX(         m_aliasMutex);
    PWCharArray             m_endpointIdentifier;
    RegistrationFailReasons m_registrationFailReason;
    void SetRegistrationFailReason(unsigned reason, unsigned commandMask);
    void SetRegistrationFailReason(RegistrationFailReasons reason);
    
    PDECLARE_InterfaceNotifier(H323Gatekeeper, OnHighPriorityInterfaceChange);
    PInterfaceMonitor::Notifier m_onHighPriorityInterfaceChange;
    PDECLARE_InterfaceNotifier(H323Gatekeeper, OnLowPriorityInterfaceChange);
    PInterfaceMonitor::Notifier m_onLowPriorityInterfaceChange;
    
    class AlternateInfo : public PObject {
      PCLASSINFO(AlternateInfo, PObject);
      public:
        AlternateInfo(H225_AlternateGK & alt);
        ~AlternateInfo();
        Comparison Compare(const PObject & obj) const;
        void PrintOn(ostream & strm) const;

        H323TransportAddress rasAddress;
        PString              gatekeeperIdentifier;
        unsigned             priority;
        enum {
          NoRegistrationNeeded,
          NeedToRegister,
          IsRegistered,
          RegistrationFailed
        } registrationState;

      private:
        // Disable copy constructor and assignment
        AlternateInfo(const AlternateInfo &other): PObject(other) { }
        AlternateInfo & operator=(const AlternateInfo &) { return *this; }
    };
    typedef PSortedList<AlternateInfo> AlternateList;
    AlternateList m_alternates;
    bool          m_alternateTemporary;

    PDECLARE_MUTEX(    m_requestMutex);
    H235Authenticators m_authenticators;
	
#if OPAL_H460
    H460_FeatureSet * m_features;
#endif

    enum {
      RequireARQ,
      PregrantARQ,
      PreGkRoutedARQ
    } pregrantMakeCall, pregrantAnswerCall;
    H323TransportAddress gkRouteAddress;

    PDictionary<POrdinalKey, H323ServiceControlSession> serviceControlSessions;

    // Gatekeeper operation variables
    bool          m_autoReregister;
    bool          m_forceRegister;
    PTimeInterval m_currentTimeToLive;
    bool          m_requiresDiscovery;
    PTimeInterval m_infoRequestTime;
    bool          m_willRespondToIRR;
    PThread     * m_monitorThread;
    bool          m_monitorRunning;
    PSyncPoint    m_monitorTickle;
    void Monitor();
    PTimeInterval InternalRegister();

    std::set< PSafePtr<H323Connection> > m_activeConnections;

  friend class H323EndPoint;
};


PLIST(H323GatekeeperList, H323Gatekeeper);


#endif // OPAL_H323

#endif // OPAL_H323_GKCLIENT_H


/////////////////////////////////////////////////////////////////////////////
