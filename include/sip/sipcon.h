/*
 * sipcon.h
 *
 * Session Initiation Protocol connection.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: sipcon.h,v $
 * Revision 1.2002  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#ifndef __OPAL_SIPCON_H
#define __OPAL_SIPCON_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/connection.h>
#include <sip/sippdu.h>


class OpalCall;
class SIPEndPoint;


/////////////////////////////////////////////////////////////////////////

/**Session Initiation Protocol connection.
 */
class SIPConnection : public OpalConnection
{
  PCLASSINFO(SIPConnection, OpalConnection);
  public:

  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    SIPConnection(
      OpalCall & call,            /// Owner calll for connection
      SIPEndPoint & endpoint,     /// Owner endpoint for connection
      const PString & token,      /// token to identify the connection
      SIP_PDU * invite            /// Original INVITE pdu
    );

    /**Destroy connection.
     */
    ~SIPConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get the phase of the connection.
      */
    virtual Phases GetPhase() const;

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remote endpoint is
       "ringing".

       The default behaviour does nothing.
      */
    virtual BOOL SetAlerting(
      const PString & calleeName,   /// Name of endpoint being alerted.
      BOOL withMedia
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour does nothing.
      */
    virtual BOOL SetConnected();

    /**Get the data formats this endpoint is capable of operating in.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Open a new media stream.
       This will create a media stream of an appropriate subclass as required
       by the underlying connection protocol. For instance H.323 would create
       an OpalRTPStream.

       The sessionID parameter may not be needed by a particular media stream
       and may be ignored. In the case of an OpalRTPStream it us used.

       Note that media streams may be created internally to the underlying
       protocol. This function is not the only way a stream can come into
       existance.

       The default behaviour is pure.
     */
    virtual OpalMediaStream * CreateMediaStream(
      BOOL isSource,      /// Is a source stream
      unsigned sessionID  /// Session number for stream
    );

    /**Clean up the termination of the connection.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if SetUpConnection() was used
       but the call never completed.

       The return value indicates if the connection object is to be deleted. A
       value of FALSE can be returned and it then someone elses responsibility
       to free the memory used.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual BOOL OnReleased();
  //@}

  /**@name Call progress functions */
  //@{
    /**Indicate in some manner that a call is alerting.
       If FALSE is returned the call is aborted.

       The default implementation calls OpalSIPEndPoint::OnShowAlerting().
      */
    virtual BOOL SendConnectionAlert(
      const PString & calleeName    /// Name of endpoint being alerted.
    );
  //@}
  
  /**@name Protocol handling functions */
  //@{
    /**Handle an incoming SIP PDU that has been full decoded
      */
    virtual BOOL OnReceivedPDU(SIP_PDU & pdu);

    /**Handle an incoming INVITE request
      */
    virtual void OnReceivedINVITE(SIP_PDU & pdu);

    /**Handle an incoming ACK PDU
      */
    virtual void OnReceivedACK(SIP_PDU & pdu);
  
    /**Handle an incoming OPTIONS PDU
      */
    virtual void OnReceivedOPTIONS(SIP_PDU & pdu);
  
    /**Handle an incoming BYE PDU
      */
    virtual void OnReceivedBYE(SIP_PDU & pdu);
  
    /**Handle an incoming CANCEL PDU
      */
    virtual void OnReceivedCANCEL(SIP_PDU & pdu);
  
    /**Handle an incoming REGISTER PDU
      */
    virtual void OnReceivedREGISTER(SIP_PDU & pdu);
  
    /**Handle an incoming Trying response PDU
      */
    virtual void OnReceivedTrying(SIP_PDU & pdu);
  
    /**Handle an incoming Session Progress response PDU
      */
    virtual void OnReceivedSessionProgress(SIP_PDU & pdu);
  
    /**Handle an incoming Proxy Authentication Required response PDU
      */
    virtual void OnReceivedAuthenticationRequired(SIP_PDU & pdu);
  
    /**Handle an incoming redirect response PDU
      */
    virtual void OnReceivedRedirection(SIP_PDU & pdu);

    /**Handle an incoming OK response PDU
      */
    virtual void OnReceivedOK(SIP_PDU & pdu);
  
    /**Handle an incoming response PDU, if not handled by specific function.
      */
    virtual void OnReceivedResponse(SIP_PDU & pdu);
  //@}

    void SendResponse(SIP_PDU::StatusCodes code, const char * str = NULL);

    PINDEX GetExpectedCSeq() const { return expectedCseq; }
    PINDEX GetLastSentCSeq() const { return lastSentCseq; }

    void InitiateCall(
      const SIPURL & destination
    );

    BOOL GetLocalIPAddress(PIPSocket::Address & localIP) const;
    RTP_Session * UseSession(unsigned rtpSessionId);

    PString GetLocalPartyAddress() const { return localPartyAddress; }
    void SetLocalPartyAddress();
    void SetLocalPartyAddress(
      const PString & addr
    ) { localPartyAddress = addr; }

    SIPEndPoint & GetEndPoint() const { return endpoint; }

    RTP_DataFrame::PayloadTypes GetRFC2833PayloadType() const;

  protected:
    PDECLARE_NOTIFIER(PThread, SIPConnection, InitiateCallThreadMain);
    PDECLARE_NOTIFIER(PThread, SIPConnection, HandlePDUsThreadMain);
    virtual void OnReceivedSDP(SIP_PDU & pdu);

    SIPEndPoint   & endpoint;
    Phases          currentPhase;
    OpalTransport * transport;

    PString   localPartyAddress;
    SIP_PDU * originalInvite;
    SIPURL    originalDestination;

    unsigned expectedCseq;
    unsigned lastSentCseq;
    BOOL     sendBYE;

    OpalMediaFormatList remoteFormatList;
    RTP_SessionManager  rtpSessions;
};


#endif // __OPAL_SIPCON_H


// End of File ///////////////////////////////////////////////////////////////
