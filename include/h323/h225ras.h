/*
 * h225ras.h
 *
 * H.225 RAS protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 *
 * $Log: h225ras.h,v $
 * Revision 1.2005  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.3  2002/07/01 04:56:29  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.2  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.19  2002/09/03 05:37:17  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 * Added RAS port constants to RAS clas name space.
 *
 * Revision 1.18  2002/08/12 06:29:55  robertj
 * Fixed problem with cached responses being aged before the RIP time which
 *   made retries by client appear as "new" requests when they were not.
 *
 * Revision 1.17  2002/08/12 05:38:20  robertj
 * Changes to the RAS subsystem to support ability to make requests to client
 *   from gkserver without causing bottlenecks and race conditions.
 *
 * Revision 1.16  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.15  2002/08/05 05:17:37  robertj
 * Fairly major modifications to support different authentication credentials
 *   in ARQ to the logged in ones on RRQ. For both client and server.
 * Various other H.235 authentication bugs and anomalies fixed on the way.
 *
 * Revision 1.14  2002/07/29 11:36:08  robertj
 * Fixed race condition if RIP is followed very quickly by actual response.
 *
 * Revision 1.13  2002/06/26 03:47:45  robertj
 * Added support for alternate gatekeepers.
 *
 * Revision 1.12  2002/06/21 02:52:44  robertj
 * Fixed problem with double checking H.235 hashing, this causes failure as
 *   the authenticator thinks it is a replay attack.
 *
 * Revision 1.11  2002/06/12 03:49:56  robertj
 * Added PrintOn function for trace output of RAS channel.
 *
 * Revision 1.10  2002/05/03 09:18:45  robertj
 * Added automatic retransmission of RAS responses to retried requests.
 *
 * Revision 1.9  2001/10/09 08:04:59  robertj
 * Fixed unregistration so still unregisters if gk goes offline, thanks Chris Purvis
 *
 * Revision 1.8  2001/09/18 10:36:54  robertj
 * Allowed multiple overlapping requests in RAS channel.
 *
 * Revision 1.7  2001/09/12 03:12:36  robertj
 * Added ability to disable the checking of RAS responses against
 *   security authenticators.
 * Fixed bug in having multiple authentications if have a retry.
 *
 * Revision 1.6  2001/08/10 11:03:49  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.5  2001/08/06 07:44:52  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.4  2001/08/06 03:18:35  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 * Improved access to H.235 secure RAS functionality.
 * Changes to H.323 secure RAS contexts to help use with gk server.
 *
 * Revision 1.3  2001/06/25 01:06:40  robertj
 * Fixed resolution of RAS timeout so not rounded down to second.
 *
 * Revision 1.2  2001/06/22 00:21:10  robertj
 * Fixed bug in H.225 RAS protocol with 16 versus 32 bit sequence numbers.
 *
 * Revision 1.1  2001/06/18 06:23:47  robertj
 * Split raw H.225 RAS protocol out of gatekeeper client class.
 *
 */

#ifndef __OPAL_H225RAS_H
#define __OPAL_H225RAS_H

#ifdef __GNUC__
#pragma interface
#endif


#include <h323/transaddr.h>
#include <h323/h235auth.h>


class PASN_Sequence;
class PASN_Choice;

class H225_GatekeeperRequest;
class H225_GatekeeperConfirm;
class H225_GatekeeperReject;
class H225_RegistrationRequest;
class H225_RegistrationConfirm;
class H225_RegistrationReject;
class H225_UnregistrationRequest;
class H225_UnregistrationConfirm;
class H225_UnregistrationReject;
class H225_AdmissionRequest;
class H225_AdmissionConfirm;
class H225_AdmissionReject;
class H225_BandwidthRequest;
class H225_BandwidthConfirm;
class H225_BandwidthReject;
class H225_DisengageRequest;
class H225_DisengageConfirm;
class H225_DisengageReject;
class H225_LocationRequest;
class H225_LocationConfirm;
class H225_LocationReject;
class H225_InfoRequest;
class H225_InfoRequestResponse;
class H225_NonStandardMessage;
class H225_UnknownMessageResponse;
class H225_RequestInProgress;
class H225_ResourcesAvailableIndicate;
class H225_ResourcesAvailableConfirm;
class H225_InfoRequestAck;
class H225_InfoRequestNak;
class H225_ArrayOf_TransportAddress;
class H225_ArrayOf_CryptoH323Token;

class H323EndPoint;
class H323RasPDU;

class OpalTransport;


///////////////////////////////////////////////////////////////////////////////

/**This class embodies the H.225.0 RAS protocol to/from gatekeepers.
  */
class H225_RAS : public PObject
{
  PCLASSINFO(H225_RAS, PObject);
  public:
  /**@name Construction */
  //@{
    enum {
      DefaultRasMulticastPort = 1718,
      DefaultRasUdpPort = 1719
    };

    /**Create a new protocol handler.
     */
    H225_RAS(
      H323EndPoint & endpoint,  /// Endpoint gatekeeper is associated with.
      OpalTransport * transport /// Transport over which gatekeepers communicates.
    );

    /**Destroy protocol handler.
     */
    ~H225_RAS();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Print the name of the gatekeeper.
      */
    void PrintOn(
      ostream & strm    /// Stream to print to.
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Start the background thread.
      */
    BOOL StartRasChannel();

    /**Handle and dispatch a RAS PDU..
      */
    virtual BOOL HandleRasPDU(
      const H323RasPDU & response
    );

    /**Write PDU to transport after executing callback.
      */
    virtual BOOL WritePDU(
      H323RasPDU & pdu
    );
  //@}

  /**@name Protocol callbacks */
  //@{
    virtual void OnSendGatekeeperRequest(H323RasPDU &, H225_GatekeeperRequest &);
    virtual void OnSendGatekeeperConfirm(H323RasPDU &, H225_GatekeeperConfirm &);
    virtual void OnSendGatekeeperReject(H323RasPDU &, H225_GatekeeperReject &);
    virtual void OnSendGatekeeperRequest(H225_GatekeeperRequest &);
    virtual void OnSendGatekeeperConfirm(H225_GatekeeperConfirm &);
    virtual void OnSendGatekeeperReject(H225_GatekeeperReject &);
    virtual BOOL OnReceiveGatekeeperRequest(const H323RasPDU &, const H225_GatekeeperRequest &);
    virtual BOOL OnReceiveGatekeeperConfirm(const H323RasPDU &, const H225_GatekeeperConfirm &);
    virtual BOOL OnReceiveGatekeeperReject(const H323RasPDU &, const H225_GatekeeperReject &);
    virtual BOOL OnReceiveGatekeeperRequest(const H225_GatekeeperRequest &);
    virtual BOOL OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm &);
    virtual BOOL OnReceiveGatekeeperReject(const H225_GatekeeperReject &);

    virtual void OnSendRegistrationRequest(H323RasPDU &, H225_RegistrationRequest &);
    virtual void OnSendRegistrationConfirm(H323RasPDU &, H225_RegistrationConfirm &);
    virtual void OnSendRegistrationReject(H323RasPDU &, H225_RegistrationReject &);
    virtual void OnSendRegistrationRequest(H225_RegistrationRequest &);
    virtual void OnSendRegistrationConfirm(H225_RegistrationConfirm &);
    virtual void OnSendRegistrationReject(H225_RegistrationReject &);
    virtual BOOL OnReceiveRegistrationRequest(const H323RasPDU &, const H225_RegistrationRequest &);
    virtual BOOL OnReceiveRegistrationConfirm(const H323RasPDU &, const H225_RegistrationConfirm &);
    virtual BOOL OnReceiveRegistrationReject(const H323RasPDU &, const H225_RegistrationReject &);
    virtual BOOL OnReceiveRegistrationRequest(const H225_RegistrationRequest &);
    virtual BOOL OnReceiveRegistrationConfirm(const H225_RegistrationConfirm &);
    virtual BOOL OnReceiveRegistrationReject(const H225_RegistrationReject &);

    virtual void OnSendUnregistrationRequest(H323RasPDU &, H225_UnregistrationRequest &);
    virtual void OnSendUnregistrationConfirm(H323RasPDU &, H225_UnregistrationConfirm &);
    virtual void OnSendUnregistrationReject(H323RasPDU &, H225_UnregistrationReject &);
    virtual void OnSendUnregistrationRequest(H225_UnregistrationRequest &);
    virtual void OnSendUnregistrationConfirm(H225_UnregistrationConfirm &);
    virtual void OnSendUnregistrationReject(H225_UnregistrationReject &);
    virtual BOOL OnReceiveUnregistrationRequest(const H323RasPDU &, const H225_UnregistrationRequest &);
    virtual BOOL OnReceiveUnregistrationConfirm(const H323RasPDU &, const H225_UnregistrationConfirm &);
    virtual BOOL OnReceiveUnregistrationReject(const H323RasPDU &, const H225_UnregistrationReject &);
    virtual BOOL OnReceiveUnregistrationRequest(const H225_UnregistrationRequest &);
    virtual BOOL OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm &);
    virtual BOOL OnReceiveUnregistrationReject(const H225_UnregistrationReject &);

    virtual void OnSendAdmissionRequest(H323RasPDU &, H225_AdmissionRequest &);
    virtual void OnSendAdmissionConfirm(H323RasPDU &, H225_AdmissionConfirm &);
    virtual void OnSendAdmissionReject(H323RasPDU &, H225_AdmissionReject &);
    virtual void OnSendAdmissionRequest(H225_AdmissionRequest &);
    virtual void OnSendAdmissionConfirm(H225_AdmissionConfirm &);
    virtual void OnSendAdmissionReject(H225_AdmissionReject &);
    virtual BOOL OnReceiveAdmissionRequest(const H323RasPDU &, const H225_AdmissionRequest &);
    virtual BOOL OnReceiveAdmissionConfirm(const H323RasPDU &, const H225_AdmissionConfirm &);
    virtual BOOL OnReceiveAdmissionReject(const H323RasPDU &, const H225_AdmissionReject &);
    virtual BOOL OnReceiveAdmissionRequest(const H225_AdmissionRequest &);
    virtual BOOL OnReceiveAdmissionConfirm(const H225_AdmissionConfirm &);
    virtual BOOL OnReceiveAdmissionReject(const H225_AdmissionReject &);

    virtual void OnSendBandwidthRequest(H323RasPDU &, H225_BandwidthRequest &);
    virtual void OnSendBandwidthConfirm(H323RasPDU &, H225_BandwidthConfirm &);
    virtual void OnSendBandwidthReject(H323RasPDU &, H225_BandwidthReject &);
    virtual void OnSendBandwidthRequest(H225_BandwidthRequest &);
    virtual void OnSendBandwidthConfirm(H225_BandwidthConfirm &);
    virtual void OnSendBandwidthReject(H225_BandwidthReject &);
    virtual BOOL OnReceiveBandwidthRequest(const H323RasPDU &, const H225_BandwidthRequest &);
    virtual BOOL OnReceiveBandwidthConfirm(const H323RasPDU &, const H225_BandwidthConfirm &);
    virtual BOOL OnReceiveBandwidthReject(const H323RasPDU &, const H225_BandwidthReject &);
    virtual BOOL OnReceiveBandwidthRequest(const H225_BandwidthRequest &);
    virtual BOOL OnReceiveBandwidthConfirm(const H225_BandwidthConfirm &);
    virtual BOOL OnReceiveBandwidthReject(const H225_BandwidthReject &);

    virtual void OnSendDisengageRequest(H323RasPDU &, H225_DisengageRequest &);
    virtual void OnSendDisengageConfirm(H323RasPDU &, H225_DisengageConfirm &);
    virtual void OnSendDisengageReject(H323RasPDU &, H225_DisengageReject &);
    virtual void OnSendDisengageRequest(H225_DisengageRequest &);
    virtual void OnSendDisengageConfirm(H225_DisengageConfirm &);
    virtual void OnSendDisengageReject(H225_DisengageReject &);
    virtual BOOL OnReceiveDisengageRequest(const H323RasPDU &, const H225_DisengageRequest &);
    virtual BOOL OnReceiveDisengageConfirm(const H323RasPDU &, const H225_DisengageConfirm &);
    virtual BOOL OnReceiveDisengageReject(const H323RasPDU &, const H225_DisengageReject &);
    virtual BOOL OnReceiveDisengageRequest(const H225_DisengageRequest &);
    virtual BOOL OnReceiveDisengageConfirm(const H225_DisengageConfirm &);
    virtual BOOL OnReceiveDisengageReject(const H225_DisengageReject &);

    virtual void OnSendLocationRequest(H323RasPDU &, H225_LocationRequest &);
    virtual void OnSendLocationConfirm(H323RasPDU &, H225_LocationConfirm &);
    virtual void OnSendLocationReject(H323RasPDU &, H225_LocationReject &);
    virtual void OnSendLocationRequest(H225_LocationRequest &);
    virtual void OnSendLocationConfirm(H225_LocationConfirm &);
    virtual void OnSendLocationReject(H225_LocationReject &);
    virtual BOOL OnReceiveLocationRequest(const H323RasPDU &, const H225_LocationRequest &);
    virtual BOOL OnReceiveLocationConfirm(const H323RasPDU &, const H225_LocationConfirm &);
    virtual BOOL OnReceiveLocationReject(const H323RasPDU &, const H225_LocationReject &);
    virtual BOOL OnReceiveLocationRequest(const H225_LocationRequest &);
    virtual BOOL OnReceiveLocationConfirm(const H225_LocationConfirm &);
    virtual BOOL OnReceiveLocationReject(const H225_LocationReject &);

    virtual void OnSendInfoRequest(H323RasPDU &, H225_InfoRequest &);
    virtual void OnSendInfoRequestAck(H323RasPDU &, H225_InfoRequestAck &);
    virtual void OnSendInfoRequestNak(H323RasPDU &, H225_InfoRequestNak &);
    virtual void OnSendInfoRequestResponse(H323RasPDU &, H225_InfoRequestResponse &);
    virtual void OnSendInfoRequest(H225_InfoRequest &);
    virtual void OnSendInfoRequestAck(H225_InfoRequestAck &);
    virtual void OnSendInfoRequestNak(H225_InfoRequestNak &);
    virtual void OnSendInfoRequestResponse(H225_InfoRequestResponse &);
    virtual BOOL OnReceiveInfoRequest(const H323RasPDU &, const H225_InfoRequest &);
    virtual BOOL OnReceiveInfoRequestAck(const H323RasPDU &, const H225_InfoRequestAck &);
    virtual BOOL OnReceiveInfoRequestNak(const H323RasPDU &, const H225_InfoRequestNak &);
    virtual BOOL OnReceiveInfoRequestResponse(const H323RasPDU &, const H225_InfoRequestResponse &);
    virtual BOOL OnReceiveInfoRequest(const H225_InfoRequest &);
    virtual BOOL OnReceiveInfoRequestAck(const H225_InfoRequestAck &);
    virtual BOOL OnReceiveInfoRequestNak(const H225_InfoRequestNak &);
    virtual BOOL OnReceiveInfoRequestResponse(const H225_InfoRequestResponse &);

    virtual void OnSendResourcesAvailableIndicate(H323RasPDU &, H225_ResourcesAvailableIndicate &);
    virtual void OnSendResourcesAvailableConfirm(H323RasPDU &, H225_ResourcesAvailableConfirm &);
    virtual void OnSendResourcesAvailableIndicate(H225_ResourcesAvailableIndicate &);
    virtual void OnSendResourcesAvailableConfirm(H225_ResourcesAvailableConfirm &);
    virtual BOOL OnReceiveResourcesAvailableIndicate(const H323RasPDU &, const H225_ResourcesAvailableIndicate &);
    virtual BOOL OnReceiveResourcesAvailableConfirm(const H323RasPDU &, const H225_ResourcesAvailableConfirm &);
    virtual BOOL OnReceiveResourcesAvailableIndicate(const H225_ResourcesAvailableIndicate &);
    virtual BOOL OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm &);

    virtual void OnSendNonStandardMessage(H323RasPDU &, H225_NonStandardMessage &);
    virtual void OnSendNonStandardMessage(H225_NonStandardMessage &);
    virtual BOOL OnReceiveNonStandardMessage(const H323RasPDU &, const H225_NonStandardMessage &);
    virtual BOOL OnReceiveNonStandardMessage(const H225_NonStandardMessage &);

    virtual void OnSendUnknownMessageResponse(H323RasPDU &, H225_UnknownMessageResponse &);
    virtual void OnSendUnknownMessageResponse(H225_UnknownMessageResponse &);
    virtual BOOL OnReceiveUnknownMessageResponse(const H323RasPDU &, const H225_UnknownMessageResponse &);
    virtual BOOL OnReceiveUnknownMessageResponse(const H225_UnknownMessageResponse &);

    virtual void OnSendRequestInProgress(H323RasPDU &, H225_RequestInProgress &);
    virtual void OnSendRequestInProgress(H225_RequestInProgress &);
    virtual BOOL OnReceiveRequestInProgress(const H323RasPDU &, const H225_RequestInProgress &);
    virtual BOOL OnReceiveRequestInProgress(const H225_RequestInProgress &);


    /**Handle unknown PDU type.
      */
    virtual BOOL OnReceiveUnkown(
      const H323RasPDU & pdu  /// PDU that was not handled.
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the gatekeepers transport channel.
      */
    OpalTransport & GetTransport() const { return *transport; }

    /**Get the gatekeeper identifer.
       For clients at least one successful registration must have been
       achieved for this field to be filling in.
      */
    const PString & GetIdentifier() const { return gatekeeperIdentifier; }

    /**Set the gatekeeper identifer.
       For servers this allows the identifier to be set and provided to all
       remote clients.
      */
    void SetIdentifier(const PString & id) { gatekeeperIdentifier = id; }

    /**Set flag to check all crypto tokens on responses.
      */
    void SetCheckResponseCryptoTokens(
      BOOL value    /// New value for checking crypto tokens.
    ) { checkResponseCryptoTokens = value; }

    /**Get flag to check all crypto tokens on responses.
      */
    BOOL GetCheckResponseCryptoTokens() { return checkResponseCryptoTokens; }

    /**Check all crypto tokens on responses for the PDU.
      */
    BOOL CheckCryptoTokens(
      const H323RasPDU & pdu,
      const H225_ArrayOf_CryptoH323Token & cryptoTokens,
      unsigned optionalField
    );
  //@}


  protected:
    //Background thread handler.
    PDECLARE_NOTIFIER(PThread, H225_RAS, HandleRasChannel);

    unsigned GetNextSequenceNumber();
    BOOL SetUpCallSignalAddresses(
      H225_ArrayOf_TransportAddress & addresses
    );

    class Request : public PObject
    {
        PCLASSINFO(Request, PObject);
      public:
        Request(unsigned seqNum, H323RasPDU  & pdu);

        BOOL Poll(H323EndPoint &, OpalTransport &);
        void CheckResponse(unsigned, const PASN_Choice *);
        void OnReceiveRIP(const H225_RequestInProgress & rip);

        // Inter-thread transfer variables
        unsigned rejectReason;
        void   * responseInfo;

        unsigned      sequenceNumber;
        H323RasPDU  & requestPDU;
        PTimeInterval whenResponseExpected;
        PSyncPoint    responseHandled;
        PMutex        responseMutex;
        enum {
          AwaitingResponse,
          ConfirmReceived,
          RejectReceived,
          BadCryptoTokens,
          RequestInProgress,
          NoResponseReceived
        } responseResult;
    };

    virtual BOOL MakeRequest(Request & request);
    BOOL CheckForResponse(unsigned, unsigned, const PASN_Choice * = NULL);

    void AgeResponses();
    BOOL SendCachedResponse(const H323RasPDU & pdu);


    // Configuration variables
    H323EndPoint  & endpoint;
    OpalTransport * transport;

    // Option variables
    PString gatekeeperIdentifier;
    BOOL    checkResponseCryptoTokens;

    // Inter-thread synchronisation variables
    H323TransportAddress lastReceivedFrom;

    unsigned      nextSequenceNumber;
    PMutex        nextSequenceNumberMutex;

    PDICTIONARY(RequestDict, POrdinalKey, Request);
    RequestDict requests;
    PMutex      requestsMutex;
    Request   * lastRequest;

    class Response : public PString
    {
        PCLASSINFO(Response, PString);
      public:
        Response(const H323TransportAddress & addr, unsigned seqNum);
        ~Response();

        void SetPDU(const H323RasPDU & pdu);
        BOOL SendCachedResponse(OpalTransport & transport);

        PTime         lastUsedTime;
        PTimeInterval retirementAge;
        H323RasPDU  * replyPDU;
    };
    PSORTED_LIST(ResponseDict, Response);
    ResponseDict responses;
};


#endif // __OPAL_H225RAS_H


/////////////////////////////////////////////////////////////////////////////
