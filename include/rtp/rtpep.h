/*
 * Endpoint abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Post Increment
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
 */

#ifndef OPAL_OPAL_RTPEP_H
#define OPAL_OPAL_RTPEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#include <opal/endpoint.h>


class OpalRTPMediaStream;


#if OPAL_RTP_FEC
namespace OpalFEC
{
  const OpalMediaType & MediaType();

  // RFC 2193 media format "place holders"
  #define OPAL_REDUNDANT_PREFIX  "Redundant"
  #define OPAL_REDUNDANT_AUDIO   OPAL_REDUNDANT_PREFIX"-Audio"
  #define OpalRedundantAudio     OpalFEC::RedundantAudio()
  extern const OpalMediaFormat & RedundantAudio();

#if OPAL_VIDEO
  #define OPAL_REDUNDANT_VIDEO   OPAL_REDUNDANT_PREFIX"-Video"
  #define OpalRedundantVideo     OpalFEC::RedundantVideo()
  extern const OpalMediaFormat & RedundantVideo();
#endif

  // RFC 5109 media format "place holders"
  #define OPAL_ULP_FEC_PREFIX    "ULP-FEC"
  #define OPAL_ULP_FEC_AUDIO     OPAL_ULP_FEC_PREFIX"-Audio"
  #define OpalUlpFecAudio        OpalFEC::UlpFecAudio()
  extern const OpalMediaFormat & UlpFecAudio();

#if OPAL_VIDEO
  #define OPAL_ULP_FEC_VIDEO     OPAL_ULP_FEC_PREFIX"-Video"
  #define OpalUlpFecVideo        OpalFEC::UlpFecVideo()
  extern const OpalMediaFormat & UlpFecVideo();
#endif

  const PString & MediaTypeOption();
};
#endif // OPAL_RTP_FEC


/**Base class for endpoint types that use RTP for media transport.
   Currently used by H323EndPoint and SIPEndPoint
  */
class OpalRTPEndPoint : public OpalEndPoint
{
  PCLASSINFO(OpalRTPEndPoint, OpalEndPoint);

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalRTPEndPoint(
      OpalManager & manager,          ///<  Manager of all endpoints.
      const PCaselessString & prefix, ///<  Prefix for URL style address strings
      Attributes attributes           ///<  Bit mask of attributes endpoint has
    );

    /**Destroy the endpoint.
     */
    ~OpalRTPEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /** Get available string option names.
      */
    virtual PStringList GetAvailableStringOptions() const;

    /**A call back function whenever a connection is broken.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if MakeConnection() was used
       but the call never completed.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour removes the connection from the internal database
       and calls the OpalManager function of the same name.
      */
    virtual void OnReleased(
      OpalConnection & connection   ///<  Connection that was established
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Call back for closed a media stream.

       The default behaviour checks for local RTP session then calls the
       OpalManager function of the same name.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     ///<  Media stream being closed
    );
  //@}

  /**@name RTP handling */
  //@{
    /**Determine if the RTP session needs to accommodate a NAT router.
       For endpoints that do not use STUN or something similar to set up all the
       correct protocol embeddded addresses correctly when a NAT router is between
       the endpoints, it is possible to still accommodate the call, with some
       restrictions. This function determines if the RTP can proceed with special
       NAT allowances.

       The special allowance is that the RTP code will ignore whatever the remote
       indicates in the protocol for the address to send RTP data and wait for
       the first packet to arrive from the remote and will then proceed to send
       all RTP data back to that address AND port.

       The default behaviour checks the values of the physical link
       (localAddr/peerAddr) against the signaling address the remote indicated in
       the protocol, eg H.323 SETUP sourceCallSignalAddress or SIP "To" or
       "Contact" fields, and makes a guess that the remote is behind a NAT router.
     */
    virtual PBoolean IsRTPNATEnabled(
      OpalConnection & connection,            ///< Connection being checked
      const PIPSocket::Address & localAddr,   ///< Local physical address of connection
      const PIPSocket::Address & peerAddr,    ///< Remote physical address of connection
      const PIPSocket::Address & signalAddr,  ///< Remotes signaling address as indicated by protocol of connection
      PBoolean incoming                       ///< Incoming/outgoing connection
    );

    /**Indicate is a local RTP connection.
       This is called when a new media stream has been created and it has been
       detected that media will be flowing between two RTP sessions within the
       same process. An application could take advantage of this by optimising
       the transfer in some way, rather than the full media path of codecs and
       sockets whcih might not be necessary.

       The return value is true if the application is going to execute some
       form of bypass, and the media patch threads should not be started.

       The default behaviour calls the OpanManager function of the same name.
      */
    virtual bool OnLocalRTP(
      OpalConnection & connection1, ///< First connection
      OpalConnection & connection2, ///< Second connection
      unsigned sessionID,           ///< Session ID of RTP session
      bool opened                   ///< Media streams are opened/closed
    ) const;

    // Check for local RTP connection. Internal function.
    bool CheckForLocalRTP(const OpalRTPMediaStream & stream);

    // Check for local RTP connection. Internal function.
    void CheckEndLocalRTP(OpalConnection & connection, OpalRTPSession * rtp);

    // Check for local RTP connection. Internal function.
    void RegisterLocalRTP(OpalRTPSession * rtp, bool removed);
  //@}

  protected:
    struct LocalRtpInfo {
      LocalRtpInfo(OpalConnection & connection) : m_connection(connection), m_previousResult(-1) { }

      OpalConnection & m_connection;
      int              m_previousResult;
    };
    typedef std::map<OpalTransportAddress, LocalRtpInfo> LocalRtpInfoMap;
    LocalRtpInfoMap m_connectionsByRtpLocalAddr;
    PDECLARE_MUTEX(m_connectionsByRtpMutex);
};


#endif // OPAL_OPAL_RTPEP_H
