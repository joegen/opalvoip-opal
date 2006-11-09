/*
 * sdp.h
 *
 * Session Description Protocol
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
 * $Log: sdp.h,v $
 * Revision 1.2021  2006/11/09 17:54:13  hfriederich
 * Allow matching of fixed RTP payload type media formats if no rtpmap attribute is present in the received SDP
 *
 * Revision 2.19  2006/04/23 20:12:52  dsandras
 * The RFC tells that the SDP answer SHOULD have the same payload type than the
 * SDP offer. Added rtpmap support to allow this. Fixes problems with Asterisk,
 * and Ekiga report #337456.
 *
 * Revision 2.18  2006/04/21 14:36:51  hfriederich
 * Adding ability to parse and transmit simple bandwidth information
 *
 * Revision 2.17  2006/03/08 10:59:02  csoutheren
 * Applied patch #1444783 - Add 'image' SDP meda type and 'udptl' transport protocol
 * Thanks to Drazen Dimoti
 *
 * Revision 2.16  2006/02/02 07:02:57  csoutheren
 * Added RTP payload map to transcoders and connections to allow remote SIP endpoints
 * to change the payload type used for outgoing RTP.
 *
 * Revision 2.15  2005/12/15 21:15:44  dsandras
 * Fixed compilation with gcc 4.1.
 *
 * Revision 2.14  2005/10/04 18:31:01  dsandras
 * Allow SetFMTP and GetFMTP to work with any option set for a=fmtp:.
 *
 * Revision 2.13  2005/09/15 17:01:08  dsandras
 * Added support for the direction attributes in the audio & video media descriptions and in the session.
 *
 * Revision 2.12  2005/07/14 08:52:19  csoutheren
 * Modified to output media desscription specific connection address if needed
 *
 * Revision 2.11  2005/04/28 20:22:52  dsandras
 * Applied big sanity patch for SIP thanks to Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.10  2005/04/10 20:51:25  dsandras
 * Added possibility to set/get the direction of a stream in an SDP.
 *
 * Revision 2.9  2004/02/09 13:13:01  rjongbloed
 * Added debug string output for media type enum.
 *
 * Revision 2.8  2004/02/07 02:18:19  rjongbloed
 * Improved searching for media format to use payload type AND the encoding name.
 *
 * Revision 2.7  2004/01/08 22:27:03  csoutheren
 * Fixed problem with not using session ID when constructing SDP lists
 *
 * Revision 2.6  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.5  2002/06/16 02:21:56  robertj
 * Utilised new standard PWLib class for POrdinalKey
 *
 * Revision 2.4  2002/03/15 07:08:24  robertj
 * Removed redundent return value on SetXXX function.
 *
 * Revision 2.3  2002/02/13 02:27:50  robertj
 * Normalised some function names.
 * Fixed incorrect port number usage stopping audio in one direction.
 *
 * Revision 2.2  2002/02/11 07:34:58  robertj
 * Changed SDP to use OpalTransport for hosts instead of IP addresses/ports
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#ifndef __OPAL_SDP_H
#define __OPAL_SDP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/transports.h>
#include <opal/mediafmt.h>
#include <rtp/rtp.h>


/////////////////////////////////////////////////////////

class SDPMediaFormat : public PObject
{
  PCLASSINFO(SDPMediaFormat, PObject);
  public:
    // the following values are mandated by RFC 2833
    enum NTEEvent {
      Digit0 = 0,
      Digit1 = 1,
      Digit2 = 2,
      Digit3 = 3,
      Digit4 = 4,
      Digit5 = 5,
      Digit6 = 6,
      Digit7 = 7,
      Digit8 = 8,
      Digit9 = 9,
      Star   = 10,
      Hash   = 11,
      A      = 12,
      B      = 13,
      C      = 14,
      D      = 15,
      Flash  = 16
    };
    
    SDPMediaFormat(
      RTP_DataFrame::PayloadTypes payloadType,
      const char * name = NULL,
      unsigned rate = 8000,
      const char * param = ""
    );

    SDPMediaFormat(const PString & fmtp, RTP_DataFrame::PayloadTypes pt);

    void PrintOn(ostream & str) const;

    RTP_DataFrame::PayloadTypes GetPayloadType() const { return payloadType; }

    PString GetEncodingName() const         { return encodingName; }
    void SetEncodingName(const PString & v) { encodingName = v; }

    void SetFMTP(const PString & _fmtp); 
    PString GetFMTP() const;

    unsigned GetClockRate(void)                        { return clockRate ; }
    void SetClockRate(unsigned  v)                     { clockRate = v; }

    void SetParameters(const PString & v)              { parameters = v; }

    OpalMediaFormat GetMediaFormat() const;

  protected:
    void AddNTEString(const PString & str);
    void AddNTEToken(const PString & ostr);
    PString GetNTEString() const;

    RTP_DataFrame::PayloadTypes payloadType;

    unsigned clockRate;
    PString encodingName;
    PString parameters;
    PString fmtp;

    POrdinalSet nteSet;     // used for NTE formats only
};

PLIST(SDPMediaFormatList, SDPMediaFormat);


/////////////////////////////////////////////////////////

class SDPMediaDescription : public PObject
{
  PCLASSINFO(SDPMediaDescription, PObject);
  public:
    enum Direction {
      RecvOnly,
      SendOnly,
      SendRecv,
      Inactive,
      Undefined
    };
    
    enum MediaType {
      Audio,
      Video,
      Application,
      Image,
      Unknown,
      NumMediaTypes
    };
#if PTRACING
    friend ostream & operator<<(ostream & out, MediaType type);
#endif

    SDPMediaDescription(
      const OpalTransportAddress & address,
      MediaType mediaType = Unknown
    );

    void PrintOn(ostream & strm) const;
    void PrintOn(const OpalTransportAddress & commonAddr, ostream & str) const;

    BOOL Decode(const PString & str);

    MediaType GetMediaType() const { return mediaType; }

    const SDPMediaFormatList & GetSDPMediaFormats() const
      { return formats; }

    OpalMediaFormatList GetMediaFormats(unsigned) const;
    void CreateRTPMap(unsigned sessionID, RTP_DataFrame::PayloadMapType & map) const;

    void AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat);

    void AddMediaFormat(const OpalMediaFormat & mediaFormat, const RTP_DataFrame::PayloadMapType & map);
    void AddMediaFormats(const OpalMediaFormatList & mediaFormats, unsigned session, const RTP_DataFrame::PayloadMapType & map);

    void SetAttribute(const PString & attr);

    void SetDirection(const Direction & d) { direction = d; }
    Direction GetDirection() const { return direction; }

    const OpalTransportAddress & GetTransportAddress() const { return transportAddress; }

    PString GetTransport() const         { return transport; }
    void SetTransport(const PString & v) { transport = v; }

	  PINDEX GetPacketTime () const            { return packetTime; }
	  void SetPacketTime (PINDEX milliseconds) { packetTime = milliseconds; }

  protected:
    void PrintOn(ostream & strm, const PString & str) const;
    MediaType mediaType;
    WORD portCount;
    PCaselessString media;
    PCaselessString transport;
    OpalTransportAddress transportAddress;

    Direction direction;

    SDPMediaFormatList formats;
    PINDEX packetTime;                  // ptime attribute, in milliseconds
};

PLIST(SDPMediaDescriptionList, SDPMediaDescription);


/////////////////////////////////////////////////////////

class SDPSessionDescription : public PObject
{
  PCLASSINFO(SDPSessionDescription, PObject);
  public:
    SDPSessionDescription(
      const OpalTransportAddress & address = OpalTransportAddress()
    );

    void PrintOn(ostream & strm) const;
    PString Encode() const;
    BOOL Decode(const PString & str);

    void SetSessionName(const PString & v) { sessionName = v; }
    PString GetSessionName() const         { return sessionName; }

    void SetUserName(const PString & v)    { ownerUsername = v; }
    PString GetUserName() const            { return ownerUsername; }

    const SDPMediaDescriptionList & GetMediaDescriptions() const { return mediaDescriptions; }

    SDPMediaDescription * GetMediaDescription(
      SDPMediaDescription::MediaType rtpMediaType
    ) const;
    void AddMediaDescription(SDPMediaDescription * md) { mediaDescriptions.Append(md); }
    
    void SetDirection(const SDPMediaDescription::Direction & d) { direction = d; }
    SDPMediaDescription::Direction GetDirection(unsigned) const;

    const OpalTransportAddress & GetDefaultConnectAddress() const { return defaultConnectAddress; }
    void SetDefaultConnectAddress(
      const OpalTransportAddress & address
    ) { defaultConnectAddress = address; }
	
	const PString & GetBandwidthModifier() const { return bandwidthModifier; }
	void SetBandwidthModifier(const PString & modifier) { bandwidthModifier = modifier; }
	
	PINDEX GetBandwidthValue() const { return bandwidthValue; }
	void SetBandwidthValue(PINDEX value) { bandwidthValue = value; }

	static const char * const ConferenceTotalBandwidthModifier;
	static const char * const ApplicationSpecificBandwidthModifier;

  protected:
    void ParseOwner(const PString & str);

    SDPMediaDescriptionList mediaDescriptions;
    SDPMediaDescription::Direction direction;

    PINDEX protocolVersion;
    PString sessionName;

    PString ownerUsername;
    unsigned ownerSessionId;
    unsigned ownerVersion;
    OpalTransportAddress ownerAddress;
    OpalTransportAddress defaultConnectAddress;
	
	PString bandwidthModifier;
	PINDEX bandwidthValue;
};

/////////////////////////////////////////////////////////


#endif // __OPAL_SDP_H


// End of File ///////////////////////////////////////////////////////////////
