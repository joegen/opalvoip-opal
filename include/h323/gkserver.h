/*
 * gkserver.h
 *
 * H225 Registration Admission and Security protocol handler
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
 * This code was based on original code from OpenGate of Egoboo Ltd. thanks
 * to Ashley Unitt for his efforts.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: gkserver.h,v $
 * Revision 1.2007  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.5  2002/07/01 04:56:29  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.4  2002/03/22 06:57:48  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.3  2002/02/11 09:32:11  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.2  2002/01/14 06:35:56  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.1  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.35  2002/09/03 06:19:36  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 *
 * Revision 1.34  2002/08/29 07:57:08  robertj
 * Added some statistics to gatekeeper server.
 *
 * Revision 1.33  2002/08/29 06:54:52  robertj
 * Removed redundent thread member variable from request info.
 *
 * Revision 1.32  2002/08/12 08:12:45  robertj
 * Added extra hint to help with ARQ using separate credentials from RRQ.
 *
 * Revision 1.31  2002/08/12 05:38:20  robertj
 * Changes to the RAS subsystem to support ability to make requests to client
 *   from gkserver without causing bottlenecks and race conditions.
 *
 * Revision 1.30  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.29  2002/08/05 05:17:37  robertj
 * Fairly major modifications to support different authentication credentials
 *   in ARQ to the logged in ones on RRQ. For both client and server.
 * Various other H.235 authentication bugs and anomalies fixed on the way.
 *
 * Revision 1.28  2002/07/16 13:49:22  robertj
 * Added missing lock when removing call from endpoint.
 *
 * Revision 1.27  2002/07/11 09:33:56  robertj
 * Added access functions to various call statistics member variables.
 *
 * Revision 1.26  2002/07/11 07:01:37  robertj
 * Added Disengage() function to force call drop from gk server.
 * Added InfoRequest() function to force client to send an IRR.
 * Added ability to automatically clear calls if do not get IRR for it.
 *
 * Revision 1.25  2002/06/21 02:52:44  robertj
 * Fixed problem with double checking H.235 hashing, this causes failure as
 *   the authenticator thinks it is a replay attack.
 *
 * Revision 1.24  2002/06/19 05:03:08  robertj
 * Changed gk code to allow for H.235 security on an endpoint by endpoint basis.
 *
 * Revision 1.23  2002/06/12 03:55:21  robertj
 * Added function to add/remove multiple listeners in one go comparing against
 *   what is already running so does not interrupt unchanged listeners.
 *
 * Revision 1.22  2002/05/29 00:03:15  robertj
 * Fixed unsolicited IRR support in gk client and server,
 *   including support for IACK and INAK.
 *
 * Revision 1.21  2002/05/21 06:30:31  robertj
 * Changed GRQ to the same as all the other xRQ request handlers.
 *
 * Revision 1.20  2002/05/17 03:42:08  robertj
 * Fixed problems with H.235 authentication on RAS for server and client.
 *
 * Revision 1.19  2002/05/07 03:18:12  robertj
 * Added application info (name/version etc) into registered endpoint data.
 *
 * Revision 1.18  2002/05/06 00:56:37  robertj
 * Sizeable rewrite of gatekeeper server code to make more bulletproof against
 *   multithreaded operation. Especially when using slow response/RIP feature.
 * Also changed the call indexing to use call id and direction as key.
 *
 * Revision 1.17  2002/04/30 23:19:00  dereks
 * Fix documentation typos.
 *
 * Revision 1.16  2002/03/06 02:01:31  robertj
 * Fixed race condition when starting slow server response thread.
 *
 * Revision 1.15  2002/03/05 00:36:01  craigs
 * Added GetReplyAddress for H323GatekeeperRequest
 *
 * Revision 1.14  2002/03/03 21:34:50  robertj
 * Added gatekeeper monitor thread.
 *
 * Revision 1.13  2002/03/02 05:58:57  robertj
 * Fixed possible bandwidth leak (thanks Francisco Olarte Sanz) and in
 *   the process added OnBandwidth function to H323GatekeeperCall class.
 *
 * Revision 1.12  2002/03/01 04:09:09  robertj
 * Fixed problems with keeping track of calls. Calls are now indexed by call-id
 *   alone and maintain both endpoints of call in its structure. Fixes problem
 *   with calls form an endpoint to itself, and having two objects being tracked
 *   where there is really only one call.
 *
 * Revision 1.11  2002/02/04 05:21:13  robertj
 * Lots of changes to fix multithreaded slow response code (RIP).
 * Fixed problem with having two entries for same call in call list.
 *
 * Revision 1.10  2002/01/31 06:45:44  robertj
 * Added more checking for invalid list processing in calls database.
 *
 * Revision 1.9  2002/01/31 00:16:15  robertj
 * Removed const to allow things to compile!
 *
 * Revision 1.8  2001/12/15 08:08:52  robertj
 * Added alerting, connect and end of call times to be sent to RAS server.
 *
 * Revision 1.7  2001/12/14 06:40:47  robertj
 * Added call end reason codes in DisengageRequest for GK server use.
 *
 * Revision 1.6  2001/12/13 11:08:45  robertj
 * Significant changes to support slow request handling, automatically sending
 *   RIP and spawning thread to handle time consuming operation.
 *
 * Revision 1.5  2001/11/19 06:56:44  robertj
 * Added prefix strings for gateways registered with this gk, thanks Mikael Stolt
 *
 * Revision 1.4  2001/08/10 11:03:49  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.3  2001/08/06 07:44:52  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.2  2001/08/06 03:18:35  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 * Improved access to H.235 secure RAS functionality.
 * Changes to H.323 secure RAS contexts to help use with gk server.
 *
 * Revision 1.1  2001/07/24 02:30:55  robertj
 * Added gatekeeper RAS protocol server classes.
 *
 */

#ifndef __OPAL_GKSERVER_H
#define __OPAL_GKSERVER_H

#ifdef __GNUC__
#pragma interface
#endif


#include <ptlib/safecoll.h>
#include <opal/guid.h>
#include <h323/h225ras.h>
#include <h323/transaddr.h>
#include <h323/h235auth.h>
#include <h323/h323pdu.h>


class PASN_Sequence;
class PASN_Choice;

class H225_AliasAddress;
class H225_EndpointIdentifier;
class H225_GatekeeperIdentifier;
class H225_ArrayOf_TransportAddress;
class H225_GatekeeperIdentifier;
class H225_EndpointIdentifier;
class H225_InfoRequestResponse_perCallInfo_subtype;

class H323RegisteredEndPoint;
class H323GatekeeperListener;
class H323GatekeeperServer;
class H323RasPDU;


class H323GatekeeperRequest : public PObject
{
    PCLASSINFO(H323GatekeeperRequest, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new gatekeeper server request.
     */
    H323GatekeeperRequest(
      H323GatekeeperListener & rasChannel,
      const H323RasPDU & pdu
    );
  //@}

    enum Response {
      Ignore = -2,
      Reject = -1,
      Confirm = 0
    };
    inline static Response InProgress(unsigned time) { return (Response)(time&0xffff); }

    BOOL HandlePDU();
    BOOL WritePDU(
      H323RasPDU & pdu
    );

    BOOL CheckGatekeeperIdentifier();
    BOOL GetRegisteredEndPoint();
    BOOL CheckCryptoTokens();
    BOOL CheckCryptoTokens(
      const H235Authenticators & authenticators
    );

#if PTRACING
    virtual const char * GetName() const = 0;
#endif
    virtual const PASN_Sequence & GetRawPDU() const = 0;
    virtual PString GetGatekeeperIdentifier() const = 0;
    virtual unsigned GetGatekeeperRejectTag() const = 0;
    virtual PString GetEndpointIdentifier() const = 0;
    virtual unsigned GetRegisteredEndPointRejectTag() const = 0;
    virtual const H225_ArrayOf_CryptoH323Token & GetCryptoTokens() const = 0;
    virtual unsigned GetCryptoTokensField() const = 0;
    virtual unsigned GetSecurityRejectTag() const = 0;
    virtual void SetRejectReason(
      unsigned reasonCode
    ) = 0;

    BOOL IsFastResponseRequired() const { return fastResponseRequired; }
    H323TransportAddress GetReplyAddress() const { return replyAddress; }
    H323GatekeeperListener & GetRasChannel() const { return rasChannel; }

    PSafePtr<H323RegisteredEndPoint> endpoint;

  protected:
    virtual Response OnHandlePDU() = 0;
    PDECLARE_NOTIFIER(PThread, H323GatekeeperRequest, SlowHandler);

    H323GatekeeperListener & rasChannel;
    unsigned                 requestSequenceNumber;
    H323TransportAddress     replyAddress;
    H323RasPDU               request;
    H323RasPDU               confirm;
    H323RasPDU               reject;
    BOOL                     fastResponseRequired;

    enum {
      UnknownAuthentication,
      FailedAuthentication,
      IsAuthenticated
    } authenticity;
};


class H323GatekeeperGRQ : public H323GatekeeperRequest
{
    PCLASSINFO(H323GatekeeperGRQ, H323GatekeeperRequest);
  public:
    H323GatekeeperGRQ(
      H323GatekeeperListener & listener,
      const H323RasPDU & pdu
    );

#if PTRACING
    virtual const char * GetName() const;
#endif
    virtual const PASN_Sequence & GetRawPDU() const;
    virtual PString GetGatekeeperIdentifier() const;
    virtual unsigned GetGatekeeperRejectTag() const;
    virtual PString GetEndpointIdentifier() const;
    virtual unsigned GetRegisteredEndPointRejectTag() const;
    virtual const H225_ArrayOf_CryptoH323Token & GetCryptoTokens() const;
    virtual unsigned GetCryptoTokensField() const;
    virtual unsigned GetSecurityRejectTag() const;
    virtual void SetRejectReason(
      unsigned reasonCode
    );

    H225_GatekeeperRequest & grq;
    H225_GatekeeperConfirm & gcf;
    H225_GatekeeperReject  & grj;

  protected:
    virtual Response OnHandlePDU();
};


class H323GatekeeperRRQ : public H323GatekeeperRequest
{
    PCLASSINFO(H323GatekeeperRRQ, H323GatekeeperRequest);
  public:
    H323GatekeeperRRQ(
      H323GatekeeperListener & listener,
      const H323RasPDU & pdu
    );

#if PTRACING
    virtual const char * GetName() const;
#endif
    virtual const PASN_Sequence & GetRawPDU() const;
    virtual PString GetGatekeeperIdentifier() const;
    virtual unsigned GetGatekeeperRejectTag() const;
    virtual PString GetEndpointIdentifier() const;
    virtual unsigned GetRegisteredEndPointRejectTag() const;
    virtual const H225_ArrayOf_CryptoH323Token & GetCryptoTokens() const;
    virtual unsigned GetCryptoTokensField() const;
    virtual unsigned GetSecurityRejectTag() const;
    virtual void SetRejectReason(
      unsigned reasonCode
    );

    H225_RegistrationRequest & rrq;
    H225_RegistrationConfirm & rcf;
    H225_RegistrationReject  & rrj;

  protected:
    virtual Response OnHandlePDU();
};


class H323GatekeeperURQ : public H323GatekeeperRequest
{
    PCLASSINFO(H323GatekeeperURQ, H323GatekeeperRequest);
  public:
    H323GatekeeperURQ(
      H323GatekeeperListener & listener,
      const H323RasPDU & pdu
    );

#if PTRACING
    virtual const char * GetName() const;
#endif
    virtual const PASN_Sequence & GetRawPDU() const;
    virtual PString GetGatekeeperIdentifier() const;
    virtual unsigned GetGatekeeperRejectTag() const;
    virtual PString GetEndpointIdentifier() const;
    virtual unsigned GetRegisteredEndPointRejectTag() const;
    virtual const H225_ArrayOf_CryptoH323Token & GetCryptoTokens() const;
    virtual unsigned GetCryptoTokensField() const;
    virtual unsigned GetSecurityRejectTag() const;
    virtual void SetRejectReason(
      unsigned reasonCode
    );

    H225_UnregistrationRequest & urq;
    H225_UnregistrationConfirm & ucf;
    H225_UnregistrationReject  & urj;

  protected:
    virtual Response OnHandlePDU();
};


class H323GatekeeperARQ : public H323GatekeeperRequest
{
    PCLASSINFO(H323GatekeeperARQ, H323GatekeeperRequest);
  public:
    H323GatekeeperARQ(
      H323GatekeeperListener & listener,
      const H323RasPDU & pdu
    );

#if PTRACING
    virtual const char * GetName() const;
#endif
    virtual const PASN_Sequence & GetRawPDU() const;
    virtual PString GetGatekeeperIdentifier() const;
    virtual unsigned GetGatekeeperRejectTag() const;
    virtual PString GetEndpointIdentifier() const;
    virtual unsigned GetRegisteredEndPointRejectTag() const;
    virtual const H225_ArrayOf_CryptoH323Token & GetCryptoTokens() const;
    virtual unsigned GetCryptoTokensField() const;
    virtual unsigned GetSecurityRejectTag() const;
    virtual void SetRejectReason(
      unsigned reasonCode
    );

    H225_AdmissionRequest & arq;
    H225_AdmissionConfirm & acf;
    H225_AdmissionReject  & arj;

    PString alternateSecurityID;

  protected:
    virtual Response OnHandlePDU();
};


class H323GatekeeperDRQ : public H323GatekeeperRequest
{
    PCLASSINFO(H323GatekeeperDRQ, H323GatekeeperRequest);
  public:
    H323GatekeeperDRQ(
      H323GatekeeperListener & listener,
      const H323RasPDU & pdu
    );

#if PTRACING
    virtual const char * GetName() const;
#endif
    virtual const PASN_Sequence & GetRawPDU() const;
    virtual PString GetGatekeeperIdentifier() const;
    virtual unsigned GetGatekeeperRejectTag() const;
    virtual PString GetEndpointIdentifier() const;
    virtual unsigned GetRegisteredEndPointRejectTag() const;
    virtual const H225_ArrayOf_CryptoH323Token & GetCryptoTokens() const;
    virtual unsigned GetCryptoTokensField() const;
    virtual unsigned GetSecurityRejectTag() const;
    virtual void SetRejectReason(
      unsigned reasonCode
    );

    H225_DisengageRequest & drq;
    H225_DisengageConfirm & dcf;
    H225_DisengageReject  & drj;

  protected:
    virtual Response OnHandlePDU();
};


class H323GatekeeperBRQ : public H323GatekeeperRequest
{
    PCLASSINFO(H323GatekeeperBRQ, H323GatekeeperRequest);
  public:
    H323GatekeeperBRQ(
      H323GatekeeperListener & listener,
      const H323RasPDU & pdu
    );

#if PTRACING
    virtual const char * GetName() const;
#endif
    virtual const PASN_Sequence & GetRawPDU() const;
    virtual PString GetGatekeeperIdentifier() const;
    virtual unsigned GetGatekeeperRejectTag() const;
    virtual PString GetEndpointIdentifier() const;
    virtual unsigned GetRegisteredEndPointRejectTag() const;
    virtual const H225_ArrayOf_CryptoH323Token & GetCryptoTokens() const;
    virtual unsigned GetCryptoTokensField() const;
    virtual unsigned GetSecurityRejectTag() const;
    virtual void SetRejectReason(
      unsigned reasonCode
    );

    H225_BandwidthRequest & brq;
    H225_BandwidthConfirm & bcf;
    H225_BandwidthReject  & brj;

  protected:
    virtual Response OnHandlePDU();
};


class H323GatekeeperLRQ : public H323GatekeeperRequest
{
    PCLASSINFO(H323GatekeeperLRQ, H323GatekeeperRequest);
  public:
    H323GatekeeperLRQ(
      H323GatekeeperListener & listener,
      const H323RasPDU & pdu
    );

#if PTRACING
    virtual const char * GetName() const;
#endif
    virtual const PASN_Sequence & GetRawPDU() const;
    virtual PString GetGatekeeperIdentifier() const;
    virtual unsigned GetGatekeeperRejectTag() const;
    virtual PString GetEndpointIdentifier() const;
    virtual unsigned GetRegisteredEndPointRejectTag() const;
    virtual const H225_ArrayOf_CryptoH323Token & GetCryptoTokens() const;
    virtual unsigned GetCryptoTokensField() const;
    virtual unsigned GetSecurityRejectTag() const;
    virtual void SetRejectReason(
      unsigned reasonCode
    );

    H225_LocationRequest & lrq;
    H225_LocationConfirm & lcf;
    H225_LocationReject  & lrj;

  protected:
    virtual Response OnHandlePDU();
};


class H323GatekeeperIRR : public H323GatekeeperRequest
{
    PCLASSINFO(H323GatekeeperIRR, H323GatekeeperRequest);
  public:
    H323GatekeeperIRR(
      H323GatekeeperListener & listener,
      const H323RasPDU & pdu
    );

#if PTRACING
    virtual const char * GetName() const;
#endif
    virtual const PASN_Sequence & GetRawPDU() const;
    virtual PString GetGatekeeperIdentifier() const;
    virtual unsigned GetGatekeeperRejectTag() const;
    virtual PString GetEndpointIdentifier() const;
    virtual unsigned GetRegisteredEndPointRejectTag() const;
    virtual const H225_ArrayOf_CryptoH323Token & GetCryptoTokens() const;
    virtual unsigned GetCryptoTokensField() const;
    virtual unsigned GetSecurityRejectTag() const;
    virtual void SetRejectReason(
      unsigned reasonCode
    );

    H225_InfoRequestResponse & irr;
    H225_InfoRequestAck      & iack;
    H225_InfoRequestNak      & inak;

  protected:
    virtual Response OnHandlePDU();
};


/**This class describes an active call on a gatekeeper.
  */
class H323GatekeeperCall : public PSafeObject
{
    PCLASSINFO(H323GatekeeperCall, PSafeObject);
  public:
  /**@name Construction */
  //@{
    enum Direction {
      AnsweringCall,
      OriginatingCall,
      UnknownDirection
    };

    /**Create a new gatekeeper call tracking record.
     */
    H323GatekeeperCall(
      H323GatekeeperServer & server,               /// Owner gatekeeper server
      const OpalGloballyUniqueID & callIdentifier, /// Unique call identifier
      Direction direction                          /// Direction of call
    );

    /**Destroy the call, removing itself from the endpoint.
      */
    ~H323GatekeeperCall();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Compare two objects.
      */
    Comparison Compare(
      const PObject & obj  /// Other object
    ) const;

    /**Print the name of the gatekeeper.
      */
    void PrintOn(
      ostream & strm    /// Stream to print to.
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Handle an admission ARQ PDU.
       The default behaviour sets some internal variables from the ARQ data
       and then calls OnResolveAdmission, if necessary, and OnReplyAdmission
       every time.
      */
    virtual H323GatekeeperRequest::Response OnAdmission(
      H323GatekeeperARQ & request
    );

    /**Shut down a call.
       This sendsa DRQ to the endpoint(s) to close the call down.
      */
    virtual BOOL Disengage();

    /**Handle a disengage DRQ PDU.
       The default behaviour simply returns TRUE.
      */
    virtual H323GatekeeperRequest::Response OnDisengage(
      H323GatekeeperDRQ & request
    );

    /**Handle a bandwidth BRQ PDU.
       The default behaviour adjusts the bandwidth used by the gatekeeper and
       adjusts the remote endpoint according to those limits.
      */
    virtual H323GatekeeperRequest::Response OnBandwidth(
      H323GatekeeperBRQ & request
    );

    /**Handle an info request response IRR PDU.
       The default behaviour resets the heartbeat time monitoring the call.
      */
    virtual H323GatekeeperRequest::Response OnInfoResponse(
      H323GatekeeperIRR & request,
      H225_InfoRequestResponse_perCallInfo_subtype & call
    );

    /**Function called to do heartbeat check of the call.
       Monitor the state of the call and make sure everything is OK.

       A return value of FALSE indicates the call is to be closed for some
       reason.

       Default behaviour checks the time since the last received IRR and if
       it has been too long does an IRQ to see if the call (and endpoint!) is
       still there and running. If the IRQ fails, FALSE is returned.
      */
    virtual BOOL OnHeartbeat();
  //@}

  /**@name Access functions */
  //@{
    H323GatekeeperServer & GetGatekeeper() const { return gatekeeper; }
    H323RegisteredEndPoint & GetEndPoint() const { return *PAssertNULL(endpoint); }
    BOOL IsAnsweringCall() const { return direction == AnsweringCall; }
    unsigned GetCallReference() const { return callReference; }
    const OpalGloballyUniqueID & GetCallIdentifier() const { return callIdentifier; }
    const OpalGloballyUniqueID & GetConferenceIdentifier() const { return conferenceIdentifier; }
    const PString & GetSourceNumber() const { return srcNumber; }
    const PStringArray & GetSourceAliases() const { return srcAliases; }
    const H323TransportAddress & GetSourceHost() const { return srcHost; }
    PString GetSourceAddress() const;
    const PString & GetDestinationNumber() const { return dstNumber; }
    const PStringArray & GetDestinationAliases() const { return dstAliases; }
    const H323TransportAddress & GetDestinationHost() const { return dstHost; }
    PString GetDestinationAddress() const;
    unsigned GetBandwidthUsed() const { return bandwidthUsed; }
    void SetBandwidthUsed(unsigned bandwidth) { bandwidthUsed = bandwidth; }
    const PTime & GetLastInfoResponseTime() const { return lastInfoResponse; }
    const PTime & GetAlertingTime() const { return alertingTime; }
    const PTime & GetConnectedTime() const { return connectedTime; }
    const PTime & GetCallEndTime() const { return callEndTime; }
    OpalCallEndReason GetCallEndReason() const { return callEndReason; }
  //@}

  protected:
    H323GatekeeperServer   & gatekeeper;
    H323RegisteredEndPoint * endpoint;
    H323GatekeeperListener * rasChannel;

    Direction            direction;
    unsigned             callReference;
    OpalGloballyUniqueID callIdentifier;
    OpalGloballyUniqueID conferenceIdentifier;
    PString              srcNumber;
    PStringArray         srcAliases;
    H323TransportAddress srcHost;
    PString              dstNumber;
    PStringArray         dstAliases;
    H323TransportAddress dstHost;
    unsigned             bandwidthUsed;
    unsigned             infoResponseRate;
    PTime                lastInfoResponse;
    PTime                alertingTime;
    PTime                connectedTime;
    PTime                callEndTime;
    OpalCallEndReason    callEndReason;
};


PSORTED_LIST(H323GatekeeperCallList, H323GatekeeperCall);


/**This class describes endpoints that are registered with a gatekeeper server.
   Note that a registered endpoint has no realationship in this software to a
   H323EndPoint class. This is purely a description of endpoints that are
   registered with the gatekeeper.
  */
class H323RegisteredEndPoint : public PSafeObject
{
    PCLASSINFO(H323RegisteredEndPoint, PSafeObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint registration record.
     */
    H323RegisteredEndPoint(
      H323GatekeeperServer & server, /// Gatekeeper server data
      const PString & id             /// Identifier
    );
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Compare two objects.
      */
    Comparison Compare(
      const PObject & obj  /// Other object
    ) const;

    /**Print the name of the gatekeeper.
      */
    void PrintOn(
      ostream & strm    /// Stream to print to.
    ) const;
  //@}

  /**@name Call Operations */
  //@{
    /**Add a call to the endpoints list of active calls.
       This is largely an internal routine, it is not expected the user would
       need to deal with this function.
      */
    void AddCall(
      H323GatekeeperCall * call
    );

    /**Remove a call from the endpoints list of active calls.
       This is largely an internal routine, it is not expected the user would
       need to deal with this function.
      */
    BOOL RemoveCall(
      H323GatekeeperCall * call
    );

    /**Get the count of active calls on this endpoint.
      */
    PINDEX GetCallCount() const { return activeCalls.GetSize(); }

    /**Get the details of teh active call on this endpoint.
      */
    H323GatekeeperCall & GetCall(
      PINDEX idx
    ) { return activeCalls[idx]; }
  //@}

  /**@name Protocol Operations */
  //@{
    /**Call back on receiving a RAS registration for this endpoint.
       The default behaviour extract information from the RRQ and sets
       internal variables to that data.

       If returns TRUE then a RCF is sent otherwise an RRJ is sent.
      */
    virtual H323GatekeeperRequest::Response OnRegistration(
      H323GatekeeperRRQ & request
    );

    /**Call back on receiving a RAS full registration for this endpoint.
       This is not called if the keepAlive flag is set indicating a
       lightweight RRQ has been received.

       The default behaviour extract information from the RRQ and sets
       internal variables to that data.

       If returns TRUE then a RCF is sent otherwise an RRJ is sent.
      */
    virtual H323GatekeeperRequest::Response OnFullRegistration(
      H323GatekeeperRRQ & request
    );

    /**Call back to set security on RAS full registration for this endpoint.
       This is called from OnFullRegistration().

       The default behaviour extract information from the RRQ and sets
       internal variables to that data.

       If returns TRUE then a RCF is sent otherwise an RRJ is sent.
      */
    virtual H323GatekeeperRequest::Response OnSecureRegistration(
      H323GatekeeperRRQ & request
    );

    /**Call back on receiving a RAS unregistration for this endpoint.
       The default behaviour clears all calls owned by this endpoint.
      */
    virtual H323GatekeeperRequest::Response OnUnregistration(
      H323GatekeeperURQ & request
    );

    /**Handle an info request response IRR PDU.
       The default behaviour finds each call current for endpoint and calls 
       the function of the same name in the H323GatekeeperCall instance.
      */
    virtual H323GatekeeperRequest::Response OnInfoResponse(
      H323GatekeeperIRR & request
    );

    /**Set password for user activating H.235 security.
      */
    virtual BOOL SetPassword(
      const PString & password
    );

    /**Determine of this endpoint has exceeded its time to live.
      */
    BOOL HasExceededTimeToLive() const;
  //@}

  /**@name Access functions */
  //@{
    /**Get the endpoint identifier assigned to the endpoint.
      */
    const PString & GetIdentifier() const { return identifier; }

    /**Get the gatekeeper server data object that owns this endpoint.
      */
    H323GatekeeperServer & GetGatekeeper() const { return gatekeeper; }

    /**Get the number of addresses that can be used to contact this
       endpoint via the RAS protocol.
      */
    PINDEX GetRASAddressCount() const { return rasAddresses.GetSize(); }

    /**Get an address that can be used to contact this endpoint via the RAS
       protocol.
      */
    OpalTransportAddress GetRASAddress(
      PINDEX idx
    ) const { return rasAddresses[idx]; }

    /**Get the number of addresses that can be used to contact this
       endpoint via the H.225/Q.931 protocol, ie normal calls.
      */
    PINDEX GetSignalAddressCount() const { return signalAddresses.GetSize(); }

    /**Get an address that can be used to contact this endpoint via the
       H.225/Q.931 protocol, ie normal calls.
      */
    OpalTransportAddress GetSignalAddress(
      PINDEX idx
    ) const { return signalAddresses[idx]; }

    /**Get the number of aliases this endpoint may be identified by.
      */
    PINDEX GetAliasCount() const { return aliases.GetSize(); }

    /**Get an alias that this endpoint may be identified by.
      */
    PString GetAlias(
      PINDEX idx
    ) const { return aliases[idx]; }

    /**Remove an alias that this endpoint may be identified by.
      */
    void RemoveAlias(
      const PString & alias
    ) { aliases.RemoveAt(aliases.GetValuesIndex(alias)); }

    /**Get the security context for this RAS connection.
      */
    virtual const H235Authenticators & GetAuthenticators() const { return authenticators; }

    /**Get the number of prefixes this endpoint can accept.
      */
    PINDEX GetPrefixCount() const { return voicePrefixes.GetSize(); }

    /**Get a prefix that this endpoint can accept.
      */
    PString GetPrefix(
      PINDEX idx
    ) const { return voicePrefixes[idx]; }

    /**Get application info (name/version etc) for endpoint.
      */
    const PString GetApplicationInfo() const { return applicationInfo; }

  //@}

  protected:
    H323GatekeeperServer    & gatekeeper;

    PString                   identifier;
    OpalTransportAddressArray rasAddresses;
    OpalTransportAddressArray signalAddresses;
    PStringArray              aliases;
    PStringArray              voicePrefixes;
    PString                   applicationInfo;
    unsigned                  timeToLive;
    H235Authenticators        authenticators;
    PTime                     lastRegistration;
    H323GatekeeperCallList    activeCalls;
};


/**This class embodies the low level H.225.0 RAS protocol on gatekeepers.
   One or more instances of this class may be used to access a single
   H323GatekeeperServer instance. Thus specific interfaces could be set up
   to receive UDP packets, all operating as the same gatekeeper.
  */
class H323GatekeeperListener : public H225_RAS
{
    PCLASSINFO(H323GatekeeperListener, H225_RAS);
  public:
  /**@name Construction */
  //@{
    /**Create a new gatekeeper listener.
     */
    H323GatekeeperListener(
      H323EndPoint & endpoint,               /// Local endpoint
      H323GatekeeperServer & server,         /// Database for gatekeeper
      const PString & gatekeeperIdentifier,  /// Name of this gatekeeper
      OpalTransport * transport = NULL       /// Transport over which gatekeepers communicates.
    );

    /**Destroy gatekeeper listener.
     */
    ~H323GatekeeperListener();
  //@}

  /**@name Operations */
  //@{
    /**Send a DisengageRequest (DRQ) to endpoint.
      */
    BOOL DisengageRequest(
      const H323GatekeeperCall & call,
      unsigned reason
    );

    /**Send an InfoRequest (IRQ) to endpoint.
      */
    virtual BOOL InfoRequest(
      H323RegisteredEndPoint & ep,
      H323GatekeeperCall * call = NULL
    );
  //@}

  /**@name Operation callbacks */
  //@{
    /**Handle a discovery GRQ PDU.
       The default behaviour does some checks and calls the gatekeeper server
       instances function of the same name.
      */
    virtual H323GatekeeperRequest::Response OnDiscovery(
      H323GatekeeperGRQ & request
    );

    /**Handle a registration RRQ PDU.
       The default behaviour does some checks and calls the gatekeeper server
       instances function of the same name.
      */
    virtual H323GatekeeperRequest::Response OnRegistration(
      H323GatekeeperRRQ & request
    );

    /**Handle an unregistration URQ PDU.
       The default behaviour does some checks and calls the gatekeeper server
       instances function of the same name.
      */
    virtual H323GatekeeperRequest::Response OnUnregistration(
      H323GatekeeperURQ & request
    );

    /**Handle an admission ARQ PDU.
       The default behaviour does some checks and calls the gatekeeper server
       instances function of the same name.
      */
    virtual H323GatekeeperRequest::Response OnAdmission(
      H323GatekeeperARQ & request
    );

    /**Handle a disengage DRQ PDU.
       The default behaviour does some checks and calls the gatekeeper server
       instances function of the same name.
      */
    virtual H323GatekeeperRequest::Response OnDisengage(
      H323GatekeeperDRQ & request
    );

    /**Handle a bandwidth BRQ PDU.
       The default behaviour does some checks and calls the gatekeeper server
       instances function of the same name.
      */
    virtual H323GatekeeperRequest::Response OnBandwidth(
      H323GatekeeperBRQ & request
    );

    /**Handle a location LRQ PDU.
       The default behaviour does some checks and calls the gatekeeper server
       instances function of the same name.
      */
    virtual H323GatekeeperRequest::Response OnLocation(
      H323GatekeeperLRQ & request
    );

    /**Handle an info request response IRR PDU.
       The default behaviour does some checks and calls the gatekeeper server
       instances function of the same name.
      */
    virtual H323GatekeeperRequest::Response OnInfoResponse(
      H323GatekeeperIRR & request
    );
  //@}

  /**@name Low level protocol callbacks */
  //@{
    virtual BOOL OnReceiveGatekeeperRequest(const H323RasPDU &, const H225_GatekeeperRequest &);
    virtual BOOL OnReceiveRegistrationRequest(const H323RasPDU &, const H225_RegistrationRequest &);
    virtual BOOL OnReceiveUnregistrationRequest(const H323RasPDU &, const H225_UnregistrationRequest &);
    virtual BOOL OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm &);
    virtual BOOL OnReceiveUnregistrationReject(const H225_UnregistrationReject &);
    virtual BOOL OnReceiveAdmissionRequest(const H323RasPDU &, const H225_AdmissionRequest &);
    virtual BOOL OnReceiveBandwidthRequest(const H323RasPDU &, const H225_BandwidthRequest &);
    virtual BOOL OnReceiveBandwidthConfirm(const H225_BandwidthConfirm &);
    virtual BOOL OnReceiveBandwidthReject(const H225_BandwidthReject &);
    virtual BOOL OnReceiveDisengageRequest(const H323RasPDU &, const H225_DisengageRequest &);
    virtual BOOL OnReceiveDisengageConfirm(const H225_DisengageConfirm &);
    virtual BOOL OnReceiveDisengageReject(const H225_DisengageReject &);
    virtual BOOL OnReceiveLocationRequest(const H323RasPDU &, const H225_LocationRequest &);
    virtual BOOL OnReceiveInfoRequestResponse(const H323RasPDU &, const H225_InfoRequestResponse &);
    virtual BOOL OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm &);
  //@}

  /**@name Member access */
  //@{
    H323GatekeeperServer & GetGatekeeper() const { return gatekeeper; }
  //@}


  protected:
    H323GatekeeperServer & gatekeeper;
};


/**This class implements a basic gatekeeper server functionality.
   An instance of this class contains all of the state information and
   operations for a gatekeeper. Multiple gatekeeper listeners may be using
   this class to link individual UDP (or other protocol) packets from various
   sources (interfaces etc) into a single instance.

   There is typically only one instance of this class, though it is not
   limited to that. An application would also quite likely descend from this
   class and override call back functions to implement more complex policy.
  */
class H323GatekeeperServer : public PObject
{
    PCLASSINFO(H323GatekeeperServer, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new gatekeeper.
     */
    H323GatekeeperServer(
      H323EndPoint & endpoint
    );

    /**Destroy gatekeeper.
     */
    ~H323GatekeeperServer();
  //@}

  /**@name Protocol Handler Operations */
  //@{
    /**Add listeners to the gatekeeper server.
       If a listener already exists on the interface specified in the list
       then it is ignored. If a listener does not yet exist a new one is
       created and if a listener is running that is not in the list then it
       is stopped and removed.

       If the array is empty then the string "*" is assumed which will listen
       on the standard UDP port on INADDR_ANY.

       Returns TRUE if at least one interface was successfully started.
      */
    BOOL AddListeners(
      const OpalTransportAddressArray & ifaces /// Interfaces to listen on.
    );

    /**Add a gatekeeper listener to this gatekeeper server given the
       transport address for the local interface.
      */
    BOOL AddListener(
      const OpalTransportAddress & interfaceName
    );

    /**Add a gatekeeper listener to this gatekeeper server given the transport.
       Note that the transport is then owned by the listener and will be
       deleted automatically when the listener is destroyed. Note also the
       transport is deleted if this function returns FALSE and no listener was
       created.
      */
    BOOL AddListener(
      OpalTransport * transport
    );

    /**Add a gatekeeper listener to this gatekeeper server.
       Note that the gatekeeper listener is then owned by the gatekeeper
       server and will be deleted automatically when the listener is removed.
       Note also the listener is deleted if this function returns FALSE and
       the listener was not used.
      */
    BOOL AddListener(
      H323GatekeeperListener * listener
    );

    /**Create a new H323GatkeeperListener.
       The user woiuld not usually use this function as it is used internally
       by the server when new listeners are added by OpalTransportAddress.

       However, a user may override this function to create objects that are
       user defined descendants of H323GatekeeperListener so the user can
       maintain extra information on a interface by interface basis.
      */
    virtual H323GatekeeperListener * CreateListener(
      OpalTransport * transport  // Transport for listener
    );

    /**Remove a gatekeeper listener from this gatekeeper server.
       The gatekeeper listener is automatically deleted.
      */
    BOOL RemoveListener(
      H323GatekeeperListener * listener
    );
  //@}

  /**@name EndPoint Operations */
  //@{
    /**Handle a discovery GRQ PDU.
       The default behaviour deals with the authentication scheme nogotiation.
      */
    virtual H323GatekeeperRequest::Response OnDiscovery(
      H323GatekeeperGRQ & request
    );

    /**Call back on receiving a RAS registration for this endpoint.
       The default behaviour checks if the registered endpoint already exists
       and if not creates a new endpoint. It then calls the OnRegistration()
       on that new endpoint instance.

       If returns TRUE then a RCF is sent otherwise an RRJ is sent.
      */
    virtual H323GatekeeperRequest::Response OnRegistration(
      H323GatekeeperRRQ & request
    );

    /**Handle an unregistration URQ PDU.
       The default behaviour removes the aliases defined in the URQ and if all
       aliases for the registered endpoint are removed then the endpoint itself
       is removed.
      */
    virtual H323GatekeeperRequest::Response OnUnregistration(
      H323GatekeeperURQ & request
    );

    /**Handle an info request response IRR PDU.
       The default behaviour calls the function of the same name in the
       endpoint instance.
      */
    virtual H323GatekeeperRequest::Response OnInfoResponse(
      H323GatekeeperIRR & request
    );

    /**Add a new registered endpoint to the server database.
       Once the endpoint has been added it is then owned by the server and
       will be deleted when it is removed.

       The user woiuld not usually use this function as it is used internally
       by the server when new registration requests (RRQ) are received.

       Note that a registered endpoint has no realationship in this software
       to a H323EndPoint class.
      */
    virtual void AddEndPoint(
      H323RegisteredEndPoint * ep
    );

    /**Remove a registered endpoint from the server database.
      */
    virtual void RemoveEndPoint(
      H323RegisteredEndPoint * ep
    );

    /**Remove an alias from the server database.
       A registered endpoint is searched for and the alias removed from that
       endpoint. If there are no more aliases left for the endpoint then the
       registered endpoint itself is removed from the database.

       Returns TRUE if the endpoint itself was removed.
      */
    virtual BOOL RemoveAlias(
      const PString & alias
    );

    /**Create a new registered endpoint object.
       The user woiuld not usually use this function as it is used internally
       by the server when new registration requests (RRQ) are received.

       However, a user may override this function to create objects that are
       user defined descendants of H323RegisteredEndPoint so the user can
       maintain extra information on a endpoint by endpoint basis.
      */
    virtual H323RegisteredEndPoint * CreateRegisteredEndPoint(
      H323GatekeeperRRQ & request
    );

    /**Create a new unique identifier for the registered endpoint.
       The returned identifier must be unique over the lifetime of this
       gatekeeper server.

       The default behaviour simply returns the string representation of the
       member variable nextIdentifier. There could be a problem in this
       implementation after 4,294,967,296 have been registered.
      */
    virtual PString CreateEndPointIdentifier();

    /**Find a registered alias given its endpoint identifier.
      */
    virtual PSafePtr<H323RegisteredEndPoint> FindEndPointByIdentifier(
      const PString & identifier,
      PSafetyMode mode = PSafeReference
    );

    /**Find a registered alias given a list of signal addresses.
      */
    virtual PSafePtr<H323RegisteredEndPoint> FindEndPointBySignalAddresses(
      const H225_ArrayOf_TransportAddress & addresses,
      PSafetyMode mode = PSafeReference
    );

    /**Find a registered alias given its signal address.
      */
    virtual PSafePtr<H323RegisteredEndPoint> FindEndPointBySignalAddress(
      const H323TransportAddress & address,
      PSafetyMode mode = PSafeReference
    );

    /**Find a registered alias given its raw alias address.
      */
    virtual PSafePtr<H323RegisteredEndPoint> FindEndPointByAliasAddress(
      const H225_AliasAddress & alias,
      PSafetyMode mode = PSafeReadWrite
    );

    /**Find a registered alias given its simple alias string.
      */
    virtual PSafePtr<H323RegisteredEndPoint> FindEndPointByAliasString(
      const PString & alias,
      PSafetyMode mode = PSafeReference
    );

    /**Get first endpoint for enumeration.
      */
    PSafePtr<H323RegisteredEndPoint> GetFirstEndPoint(
      PSafetyMode mode = PSafeReference
    ) { return PSafePtr<H323RegisteredEndPoint>(byIdentifier, mode); }
  //@}

  /**@name Call Operations */
  //@{
    /**Handle an admission ARQ PDU.
       The default behaviour verifies that the call is allowed by the policies
       the gatekeeper server requires, then attempts to look up the required
       signal address for the call. It also manages bandwidth allocations.
      */
    virtual H323GatekeeperRequest::Response OnAdmission(
      H323GatekeeperARQ & request
    );

    /**Handle a disengage DRQ PDU.
       The default behaviour finds the call by its id provided in the DRQ and
       removes it from the gatekeeper server database.
      */
    virtual H323GatekeeperRequest::Response OnDisengage(
      H323GatekeeperDRQ & request
    );

    /**Handle a bandwidth BRQ PDU.
       The default behaviour finds the call and does some checks then calls
       the H323GatekeeperCall function of the same name.
      */
    virtual H323GatekeeperRequest::Response OnBandwidth(
      H323GatekeeperBRQ & request
    );

    /**Create a new call object.
       The user woiuld not usually use this function as it is used internally
       by the server when new calls (ARQ) are made.

       However, a user may override this function to create objects that are
       user defined descendants of H323GatekeeperCall so the user can
       maintain extra information on a call by call basis.
      */
    virtual H323GatekeeperCall * CreateCall(
      const OpalGloballyUniqueID & callIdentifier,
      H323GatekeeperCall::Direction direction
    );

    /**Remove a call from the server database.
      */
    virtual void RemoveCall(
      H323GatekeeperCall * call
    );

    /**Find the call given the identifier.
      */
    virtual PSafePtr<H323GatekeeperCall> FindCall(
      const OpalGloballyUniqueID & callIdentifier,
      BOOL answeringCall,
      PSafetyMode mode = PSafeReference
    );

    /**Find the call given the identifier.
      */
    virtual PSafePtr<H323GatekeeperCall> FindCall(
      const OpalGloballyUniqueID & callIdentifier,
      H323GatekeeperCall::Direction direction = H323GatekeeperCall::UnknownDirection,
      PSafetyMode mode = PSafeReference
    );

    /**Get first endpoint for enumeration.
      */
    PSafePtr<H323GatekeeperCall> GetFirstCall(
      PSafetyMode mode = PSafeReference
    ) { return PSafePtr<H323GatekeeperCall>(activeCalls, mode); }
  //@}

  /**@name Routing operations */
  //@{
    /**Handle a location LRQ PDU.
       The default behaviour just uses TranslateAliasAddressToSignalAddress to
       determine the endpoints location.

       It is expected that a user would override this function to implement
       application specified look up algorithms.
      */
    virtual H323GatekeeperRequest::Response OnLocation(
      H323GatekeeperLRQ & request
    );

    /**Translate a given alias to a signal address.
       This is called by the OnAdmission() handler to fill in the ACF
       informing the calling endpoint where to actually connect to.

       It is expected that a user would override this function to implement
       application specified look up algorithms.

       The default behaviour checks the isGatekeeperRouted and if TRUE simply
       returns the gatekeepers associated endpoints (not registered endpoint,
       but real H323EndPoint) listening address.

       If isGatekeeperRouted is FALSE then it looks up the registered
       endpoints by alias and uses the saved signal address in the database.

       If the alias is not registered then the address parameter is not
       changed and the function returns TRUE if it is a valid address, FALSE
       if it was empty.
      */
    virtual BOOL TranslateAliasAddressToSignalAddress(
      const H225_AliasAddress & alias,
      H323TransportAddress & address
    );
  //@}

  /**@name Policy operations */
  //@{
    /**Check the signal address against the security policy.
       This validates that the specified endpoint is allowed to make a
       connection to or from the specified signal address.

       It is expected that a user would override this function to implement
       application specified security policy algorithms.

       The default behaviour simply returns TRUE.
      */
    virtual BOOL CheckSignalAddressPolicy(
      const H323RegisteredEndPoint & ep,
      const H225_AdmissionRequest & arq,
      const OpalTransportAddress & address
    );

    /**Check the alias address against the security policy.
       This validates that the specified endpoint is allowed to make a
       connection to or from the specified alias address.

       It is expected that a user would override this function to implement
       application specified security policy algorithms.

       The default behaviour checks the canOnlyAnswerRegisteredEP or
       canOnlyCallRegisteredEP meber variables depending on if it is an
       incoming call and if that is TRUE only allows the call to proceed
       if the alias is also registered with the gatekeeper.
      */
    virtual BOOL CheckAliasAddressPolicy(
      const H323RegisteredEndPoint & ep,
      const H225_AdmissionRequest & arq,
      const H225_AliasAddress & alias
    );

    /**Check the alias address against the security policy.
       This validates that the specified endpoint is allowed to make a
       connection to or from the specified simple alias string.

       It is expected that a user would override this function to implement
       application specified security policy algorithms.

       The default behaviour checks the canOnlyAnswerRegisteredEP or
       canOnlyCallRegisteredEP meber variables depending on if it is an
       incoming call and if that is TRUE only allows the call to proceed
       if the alias is also registered with the gatekeeper.
      */
    virtual BOOL CheckAliasStringPolicy(
      const H323RegisteredEndPoint & ep,
      const H225_AdmissionRequest & arq,
      const PString & alias
    );

    /**Allocate or change the bandwidth being used.
       This function modifies the total bandwidth used by the all endpoints
       registered with this gatekeeper. It is called when ARQ or BRQ PDU's are
       received.
      */
    virtual unsigned AllocateBandwidth(
      unsigned newBandwidth,
      unsigned oldBandwidth = 0
    );
  //@}

  /**@name Security and authentication functions */
  //@{
    /**Get the default list of authenticators for endpoint.
      */
    virtual H235Authenticators CreateAuthenticators() const;

    /**Get separate H.235 authentication for the connection.
       This allows an individual ARQ to override the authentical credentials
       used in H.235 based RAS for this particular connection.

       A return value of FALSE indicates to use the default credentials of the
       endpoint, while TRUE indicates that new credentials are to be used.

       The default behavour does nothing and returns FALSE.
      */
    virtual BOOL GetAdmissionRequestAuthentication(
      H323GatekeeperARQ & info,  /// ARQ being constructed
      PString & remoteId,        /// Remote ID to be modified
      PString & localId,         /// Local ID to be modified
      PString & password         /// Password to be modified
    );

    /**Get password for user if H.235 security active.
       Returns FALSE if security active and user not found. Returns TRUE if
       security active and user is found. Also returns TRUE if security
       inactive, and the password is always the empty string.
      */
    virtual BOOL GetUsersPassword(
      const PString & alias,
      PString & password
    ) const;
  //@}

  /**@name Access functions */
  //@{
    /**Get the identifier name for this gatekeeper.
      */
    const PString & GetGatekeeperIdentifier() const { return gatekeeperIdentifier; }

    /**Set the identifier name for this gatekeeper.
       If adjustListeners is TRUE then all gatekeeper listeners that are
       attached to this gatekeeper server have their identifier names changed
       as well.
      */
    void SetGatekeeperIdentifier(
      const PString & id,
      BOOL adjustListeners = TRUE
    );

    /**Get the total bandwidth available in 100's of bits per second.
      */
    unsigned GetAvailableBandwidth() const { return availableBandwidth; }

    /**Set the total bandwidth available in 100's of bits per second.
      */
    void SetAvailableBandwidth(unsigned bps100) { availableBandwidth = bps100; }

    /**Get the default bandwidth for calls.
      */
    unsigned GetDefaultBandwidth() const { return defaultBandwidth; }

    /**Get the default time to live for new registered endpoints.
      */
    unsigned GetTimeToLive() const { return defaultTimeToLive; }

    /**Set the default time to live for new registered endpoints.
      */
    void SetTimeToLive(unsigned seconds) { defaultTimeToLive = seconds; }

    /**Get the default time for monitoring calls via IRR.
      */
    unsigned GetInfoResponseRate() const { return defaultInfoResponseRate; }

    /**Set the default time for monitoring calls via IRR.
      */
    void SetInfoResponseRate(unsigned seconds) { defaultInfoResponseRate = seconds; }

    /**Get flag for is gatekeeper routed.
      */
    BOOL IsGatekeeperRouted() const { return isGatekeeperRouted; }

    /**Get flag for if H.235 authentication is required.
      */
    BOOL IsRequiredH235() const { return requireH235; }

    /**Get the currently active registration count.
      */
    unsigned GetActiveRegistrations() const { return byIdentifier.GetSize(); }

    /**Get the peak registration count.
      */
    unsigned GetPeakRegistrations() const { return peakRegistrations; }

    /**Get the total registrations since start up.
      */
    unsigned GetTotalRegistrations() const { return totalRegistrations; }

    /**Get the currently active call count.
      */
    unsigned GetActiveCalls() const { return activeCalls.GetSize(); }

    /**Get the peak calls count.
      */
    unsigned GetPeakCalls() const { return peakCalls; }

    /**Get the total calls since start up.
      */
    unsigned GetTotalCalls() const { return totalCalls; }
  //@}

  protected:
    PDECLARE_NOTIFIER(PThread, H323GatekeeperServer, MonitorMain);

    // Configuration & policy variables
    PString  gatekeeperIdentifier;
    unsigned availableBandwidth;
    unsigned defaultBandwidth;
    unsigned maximumBandwidth;
    unsigned defaultTimeToLive;
    unsigned defaultInfoResponseRate;
    BOOL     overwriteOnSameSignalAddress;
    BOOL     canOnlyCallRegisteredEP;
    BOOL     canOnlyAnswerRegisteredEP;
    BOOL     answerCallPreGrantedARQ;
    BOOL     makeCallPreGrantedARQ;
    BOOL     isGatekeeperRouted;
    BOOL     aliasCanBeHostName;
    BOOL     requireH235;
    BOOL     disengageOnHearbeatFail;

    PStringToString passwords;

    // Dynamic variables
    H323EndPoint & endpoint;
    PMutex         mutex;
    time_t         identifierBase;
    unsigned       nextIdentifier;
    PThread      * monitorThread;
    PSyncPoint     monitorExit;

    PLIST(ListenerList, H323GatekeeperListener);
    ListenerList listeners;

    PDICTIONARY(IdentifierDict, PString, H323RegisteredEndPoint);
    PSafeDictionary<IdentifierDict, PString, H323RegisteredEndPoint> byIdentifier;
    PStringToString byAddress;
    PStringToString byAlias;
    PStringToString byVoicePrefix;

    PSafeList<H323GatekeeperCallList, H323GatekeeperCall> activeCalls;

    PINDEX peakRegistrations;
    PINDEX totalRegistrations;
    PINDEX peakCalls;
    PINDEX totalCalls;
};


#endif // __OPAL_GKSERVER_H


/////////////////////////////////////////////////////////////////////////////
