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
 * $Id$
 */

#ifndef __OPAL_H225RAS_H
#define __OPAL_H225RAS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <h323/transaddr.h>
#include <h323/h235auth.h>
#include <h323/h323trans.h>
#include <h323/svcctrl.h>


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
class H225_ArrayOf_CryptoH323Token;
class H225_FeatureSet;

class H323EndPoint;
class H323Connection;
class H323RasPDU;


///////////////////////////////////////////////////////////////////////////////

/**This class embodies the H.225.0 RAS protocol to/from gatekeepers.
  */
class H225_RAS : public H323Transactor
{
  PCLASSINFO(H225_RAS, H323Transactor);
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
      H323EndPoint & endpoint,  ///<  Endpoint gatekeeper is associated with.
      H323Transport * transport ///<  Transport over which gatekeepers communicates.
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
      ostream & strm    ///<  Stream to print to.
    ) const;
  //@}

  /**@name Overrides from H323Transactor */
  //@{
    /**Create the transaction PDU for reading.
      */
    virtual H323TransactionPDU * CreateTransactionPDU() const;

    /**Handle and dispatch a transaction PDU
      */
    virtual BOOL HandleTransaction(
      const PASN_Object & rawPDU
    );

    /**Allow for modifications to PDU on send.
      */
    virtual void OnSendingPDU(
      PASN_Object & rawPDU
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

    virtual void OnSendServiceControlIndication(H323RasPDU &, H225_ServiceControlIndication &);
    virtual void OnSendServiceControlResponse(H323RasPDU &, H225_ServiceControlResponse &);
    virtual void OnSendServiceControlIndication(H225_ServiceControlIndication &);
    virtual void OnSendServiceControlResponse(H225_ServiceControlResponse &);
    virtual BOOL OnReceiveServiceControlIndication(const H323RasPDU &, const H225_ServiceControlIndication &);
    virtual BOOL OnReceiveServiceControlResponse(const H323RasPDU &, const H225_ServiceControlResponse &);
    virtual BOOL OnReceiveServiceControlIndication(const H225_ServiceControlIndication &);
    virtual BOOL OnReceiveServiceControlResponse(const H225_ServiceControlResponse &);

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
	
	virtual BOOL OnSendFeatureSet(unsigned, H225_FeatureSet &) const { return FALSE; }
	virtual void OnReceiveFeatureSet(unsigned, const H225_FeatureSet &) const {}


    /**Handle unknown PDU type.
      */
    virtual BOOL OnReceiveUnknown(
      const H323RasPDU & pdu  ///<  PDU that was not handled.
    );
  //@}

  /**@name Member variable access */
  //@{
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
  //@}

  protected:
    // Option variables
    PString gatekeeperIdentifier;
};


#endif // __OPAL_H225RAS_H


/////////////////////////////////////////////////////////////////////////////
