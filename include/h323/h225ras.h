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
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
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

#ifndef __H323_H225RAS_H
#define __H323_H225RAS_H

#ifdef __GNUC__
#pragma interface
#endif


#include <h323/transaddr.h>


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
    virtual void OnSendGatekeeperRequest(H225_GatekeeperRequest &);
    virtual void OnSendGatekeeperConfirm(H225_GatekeeperConfirm &);
    virtual void OnSendGatekeeperReject(H225_GatekeeperReject &);
    virtual void OnReceiveGatekeeperRequest(const H225_GatekeeperRequest &);
    virtual BOOL OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm &);
    virtual BOOL OnReceiveGatekeeperReject(const H225_GatekeeperReject &);

    virtual void OnSendRegistrationRequest(H225_RegistrationRequest &);
    virtual void OnSendRegistrationConfirm(H225_RegistrationConfirm &);
    virtual void OnSendRegistrationReject(H225_RegistrationReject &);
    virtual void OnReceiveRegistrationRequest(const H225_RegistrationRequest &);
    virtual BOOL OnReceiveRegistrationConfirm(const H225_RegistrationConfirm &);
    virtual BOOL OnReceiveRegistrationReject(const H225_RegistrationReject &);

    virtual void OnSendUnregistrationRequest(H225_UnregistrationRequest &);
    virtual void OnSendUnregistrationConfirm(H225_UnregistrationConfirm &);
    virtual void OnSendUnregistrationReject(H225_UnregistrationReject &);
    virtual void OnReceiveUnregistrationRequest(const H225_UnregistrationRequest &);
    virtual BOOL OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm &);
    virtual BOOL OnReceiveUnregistrationReject(const H225_UnregistrationReject &);

    virtual void OnSendAdmissionRequest(H225_AdmissionRequest &);
    virtual void OnSendAdmissionConfirm(H225_AdmissionConfirm &);
    virtual void OnSendAdmissionReject(H225_AdmissionReject &);
    virtual void OnReceiveAdmissionRequest(const H225_AdmissionRequest &);
    virtual BOOL OnReceiveAdmissionConfirm(const H225_AdmissionConfirm &);
    virtual BOOL OnReceiveAdmissionReject(const H225_AdmissionReject &);

    virtual void OnSendBandwidthRequest(H225_BandwidthRequest &);
    virtual void OnSendBandwidthConfirm(H225_BandwidthConfirm &);
    virtual void OnSendBandwidthReject(H225_BandwidthReject &);
    virtual void OnReceiveBandwidthRequest(const H225_BandwidthRequest &);
    virtual BOOL OnReceiveBandwidthConfirm(const H225_BandwidthConfirm &);
    virtual BOOL OnReceiveBandwidthReject(const H225_BandwidthReject &);

    virtual void OnSendDisengageRequest(H225_DisengageRequest &);
    virtual void OnSendDisengageConfirm(H225_DisengageConfirm &);
    virtual void OnSendDisengageReject(H225_DisengageReject &);
    virtual void OnReceiveDisengageRequest(const H225_DisengageRequest &);
    virtual BOOL OnReceiveDisengageConfirm(const H225_DisengageConfirm &);
    virtual BOOL OnReceiveDisengageReject(const H225_DisengageReject &);

    virtual void OnSendLocationRequest(H225_LocationRequest &);
    virtual void OnSendLocationConfirm(H225_LocationConfirm &);
    virtual void OnSendLocationReject(H225_LocationReject &);
    virtual void OnReceiveLocationRequest(const H225_LocationRequest &);
    virtual BOOL OnReceiveLocationConfirm(const H225_LocationConfirm &);
    virtual BOOL OnReceiveLocationReject(const H225_LocationReject &);

    virtual void OnSendInfoRequest(H225_InfoRequest &);
    virtual void OnSendInfoRequestAck(H225_InfoRequestAck &);
    virtual void OnSendInfoRequestNak(H225_InfoRequestNak &);
    virtual void OnSendInfoRequestResponse(H225_InfoRequestResponse &);
    virtual void OnReceiveInfoRequest(const H225_InfoRequest &);
    virtual BOOL OnReceiveInfoRequestAck(const H225_InfoRequestAck &);
    virtual BOOL OnReceiveInfoRequestNak(const H225_InfoRequestNak &);
    virtual BOOL OnReceiveInfoRequestResponse(const H225_InfoRequestResponse &);

    virtual void OnSendResourcesAvailableIndicate(H225_ResourcesAvailableIndicate &);
    virtual void OnSendResourcesAvailableConfirm(H225_ResourcesAvailableConfirm &);
    virtual void OnReceiveResourcesAvailableIndicate(const H225_ResourcesAvailableIndicate &);
    virtual BOOL OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm &);

    virtual void OnSendNonStandardMessage(H225_NonStandardMessage &);
    virtual void OnReceiveNonStandardMessage(const H225_NonStandardMessage &);

    virtual void OnSendUnknownMessageResponse(H225_UnknownMessageResponse &);
    virtual void OnReceiveUnknownMessageResponse(const H225_UnknownMessageResponse &);

    virtual void OnSendRequestInProgress(H225_RequestInProgress &);
    virtual BOOL OnReceiveRequestInProgress(const H225_RequestInProgress &);


    /**Handle unknown PDU type.
      */
    virtual void OnReceiveUnkown(
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

    /**Get reject reason code for xxxRequest() function.
      */
    unsigned GetRejectReason() const { return rejectReason; }
  //@}


  protected:
    //Background thread handler.
    PDECLARE_NOTIFIER(PThread, H225_RAS, HandleRasChannel);

    unsigned GetNextSequenceNumber();

    BOOL MakeRequest(H323RasPDU & request);
    BOOL CheckForResponse(unsigned, unsigned);
    BOOL CheckForRejectResponse(unsigned, unsigned, const char *, const PASN_Choice &);

    BOOL SetUpCallSignalAddresses(
      H225_ArrayOf_TransportAddress & addresses
    );


    // Configuration variables
    H323EndPoint  & endpoint;
    OpalTransport * transport;

    // Option variables
    PString gatekeeperIdentifier;
    enum {
      RequireARQ,
      PregrantARQ,
      PreGkRoutedARQ
    } pregrantMakeCall, pregrantAnswerCall;

    // Inter-thread synchronisation variables
    PMutex        makeRequestMutex;
    unsigned      lastSequenceNumber;
    unsigned      requestTag;
    PTimeInterval whenResponseExpected;
    PSyncPoint    responseHandled;
    enum {
      AwaitingResponse,
      ConfirmReceived,
      RejectReceived,
      RequestInProgress
    } responseResult;

    // Inter-thread transfer variables
    H323TransportAddress locatedAddress;
    unsigned             rejectReason;
};


#endif // __H323_H225RAS_H


/////////////////////////////////////////////////////////////////////////////
