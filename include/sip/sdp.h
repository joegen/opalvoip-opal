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
 * Revision 1.2021.2.4  2007/08/05 13:12:17  hfriederich
 * Backport from HEAD - Changes since last commit
 *
 * Revision 2.20.2.3  2007/05/04 09:51:29  hfriederich
 * Backport from HEAD - Changes since Apr 1, 2007
 *
 * Revision 2.20.2.2  2007/02/16 10:43:41  hfriederich
 * - Extend SDP capability system for merging local / remote format parameters.
 * - Propagate media format options to the media streams
 *
 * Revision 2.20.2.1  2007/02/07 08:51:01  hfriederich
 * New branch with major revision of the core Opal media format handling system.
 *
 * - Session IDs have been replaced by new OpalMediaType class.
 * - The creation of H.245 TCS and SDP media descriptions have been extended
 *   to dynamically handle all available media types
 * - The H.224 code has been rewritten for better integration into the Opal
 *   system. It takes advantage of the new media type system and removes
 *   all hooks found in the core Opal classes.
 *
 * More work will follow as the current version breaks lots of important code.
 *
 * Revision 2.20  2006/11/09 17:54:13  hfriederich
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

class SDPCapability;
class SDPCapabilityList;
class SDPSessionDescription;

/////////////////////////////////////////////////////////

/**This class encapsulates all SDP parameters applying
   to one specific media format
  */
class SDPMediaFormat : public PObject
{
  PCLASSINFO(SDPMediaFormat, PObject);
  public:
    
    SDPMediaFormat(
      RTP_DataFrame::PayloadTypes payloadType, ///< RTP Payload type for this media format.
      const char * name = NULL                 ///< RTP Payload type name
    );

    void PrintOn(ostream & str) const;

    RTP_DataFrame::PayloadTypes GetPayloadType() const { return payloadType; }
    void SetPayloadType(RTP_DataFrame::PayloadTypes _payloadType) { payloadType = _payloadType; }

    const PString & GetEncodingName() const { return encodingName; }
    void SetEncodingName(const PString & v) { encodingName = v; }

    unsigned GetClockRate(void)                        { return clockRate ; }
    void SetClockRate(unsigned  v)                     { clockRate = v; }

    const PString & GetParameters() const              { return parameters; }
    void SetParameters(const PString & v)              { parameters = v; }
    
    const PString & GetFMTP() const     { return fmtp; }
    void SetFMTP(const PString & _fmtp) { fmtp = _fmtp; } 

    /**Tries to find a media format matching this SDP media format
      */
    const OpalMediaFormat & GetMediaFormat() const;

  protected:

    mutable OpalMediaFormat mediaFormat;
    
    RTP_DataFrame::PayloadTypes payloadType;
    unsigned clockRate;
    PString encodingName;
    PString parameters;
    PString fmtp;
};

PLIST(SDPMediaFormatList, SDPMediaFormat);


/////////////////////////////////////////////////////////

/**This class encapsulates SDP media description, which
   consists of general media parameters and a list of
   SDP media format descriptions
  */
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
    
    enum Transport {
      RTP,
      UDPTL,
      Unknown
    };

    SDPMediaDescription(
      const OpalTransportAddress & address,
      OpalMediaType::MIMEMediaType mediaType = OpalMediaType::Unknown,
      Transport transport = RTP
    );

    void PrintOn(ostream & strm) const;
    void PrintOn(const OpalTransportAddress & commonAddr, ostream & str) const;

    BOOL Decode(const PString & str);
    
    OpalMediaType::MIMEMediaType GetMIMEMediaType() const { return mimeMediaType; }
    void SetMIMEMediaType(OpalMediaType::MIMEMediaType _mimeMediaType) { mimeMediaType = _mimeMediaType; }
    
    const OpalMediaType & GetMediaType() const;
    
    const OpalTransportAddress & GetTransportAddress() const { return transportAddress; }
    BOOL SetTransportAddress(const OpalTransportAddress & _transportAddress);
    
    WORD GetPortCount() const { return portCount; }
    void SetPortCount(WORD _portCount) { portCount = _portCount; }
    
    Transport GetTransport() const         { return transport; }
    void SetTransport(const Transport & v) { transport = v; }
    
    void SetDirection(const Direction & d) { direction = d; }
    Direction GetDirection() const { return direction; }
    
    PINDEX GetPacketTime () const            { return packetTime; }
    void SetPacketTime (PINDEX milliseconds) { packetTime = milliseconds; }

    const SDPMediaFormatList & GetSDPMediaFormats() const
      { return formats; }
    void AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat);
    
    OpalMediaFormatList GetMediaFormats(const OpalMediaType & mediaType) const;
    void AddMediaFormat(const SDPCapability & capability, const RTP_DataFrame::PayloadMapType & map);
    void UpdateCapabilities(SDPCapabilityList & capabilities, 
                            const SDPSessionDescription & sessionDescription) const;
    void CreateRTPMap(const OpalMediaType & mediaType, RTP_DataFrame::PayloadMapType & map) const;

    /**Internal method that is used during decode stage */
    void SetAttribute(const PString & attr);
    
    static PString GetTransportString(Transport transport);
    static Transport ParseTransport(const PString & transportString);

  protected:
    void PrintOn(ostream & strm, const PString & str) const;
    
    OpalMediaType::MIMEMediaType mimeMediaType;
    OpalTransportAddress transportAddress;
    WORD portCount;
    Transport transport;

    Direction direction;
    PINDEX packetTime;                  // ptime attribute, in milliseconds

    SDPMediaFormatList formats;
};

PLIST(SDPMediaDescriptionList, SDPMediaDescription);

inline ostream & operator<<(ostream & strm, SDPMediaDescription::Transport transport) { return strm << SDPMediaDescription::GetTransportString(transport); }


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

    PString GetSessionName() const         { return sessionName; }
    void SetSessionName(const PString & v) { sessionName = v; }

    PString GetUserName() const            { return ownerUsername; }
    void SetUserName(const PString & v)    { ownerUsername = v; }

    const SDPMediaDescriptionList & GetMediaDescriptions() const { return mediaDescriptions; }

    /**Returns the first media description for the given MIME media type
      */
    SDPMediaDescription * GetMediaDescription(
      OpalMediaType::MIMEMediaType mimeMediaType
    ) const;
    /**Returns a media description fitting the given OpalMediaType
      */
    SDPMediaDescription * GetMediaDescription(
      const OpalMediaType & mediaType
    ) const;
    void AddMediaDescription(SDPMediaDescription * md) { mediaDescriptions.Append(md); }
    
    void SetDirection(const SDPMediaDescription::Direction & d) { direction = d; }
    SDPMediaDescription::Direction GetDirection(const OpalMediaType & mediaType) const;

    const OpalTransportAddress & GetDefaultConnectAddress() const { return defaultConnectAddress; }
    void SetDefaultConnectAddress(
      const OpalTransportAddress & address
    ) { defaultConnectAddress = address; }
	
	const PString & GetBandwidthModifier() const { return bandwidthModifier; }
	void SetBandwidthModifier(const PString & modifier) { bandwidthModifier = modifier; }
	
	PINDEX GetBandwidthValue() const { return bandwidthValue; }
	void SetBandwidthValue(PINDEX value) { bandwidthValue = value; }

	static const PString & ConferenceTotalBandwidthModifier();
	static const PString & ApplicationSpecificBandwidthModifier();

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
