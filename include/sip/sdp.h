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
 * Revision 1.2005  2002/03/15 07:08:24  robertj
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

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/transports.h>
#include <opal/mediafmt.h>
#include <rtp/rtp.h>


/////////////////////////////////////////////////////////

PSET(POrdinalSet, POrdinalKey);

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

    SDPMediaFormat::SDPMediaFormat(
      RTP_DataFrame::PayloadTypes payloadType,
      const char * name = "-",
      PINDEX rate = 8000,
      const char * param = ""
    );

    SDPMediaFormat(const PString & fmtp, RTP_DataFrame::PayloadTypes pt);

    void PrintOn(ostream & str) const;

    RTP_DataFrame::PayloadTypes GetPayloadType() const { return payloadType; }

    PString GetEncodingName() const         { return encodingName; }
    void SetEncodingName(const PString & v) { encodingName = v; }

    void SetFMTP(const PString & _fmtp); 
    PString GetFMTP() const;

    void SetClockRate(PINDEX v)                        { clockRate = v; }
    void SetParameters(const PString & v)              { parameters = v; }

    // used only for audio formats
    OpalMediaFormat GetMediaFormat() const;

  protected:
    void AddNTEString(const PString & str);
    void AddNTEToken(const PString & ostr);
    PString GetNTEString() const;

    RTP_DataFrame::PayloadTypes payloadType;

    PINDEX clockRate;
    PString encodingName;
    PString parameters;

    POrdinalSet nteSet;     // used for NTE formats only
};

PLIST(SDPMediaFormatList, SDPMediaFormat);


/////////////////////////////////////////////////////////

class SDPMediaDescription : public PObject
{
  PCLASSINFO(SDPMediaDescription, PObject);
  public:
    enum MediaType {
      Audio,
      Video,
      Unknown
    };

    SDPMediaDescription(
      const OpalTransportAddress & address,
      MediaType mediaType = Unknown
    );

    void PrintOn(ostream & strm) const;
    BOOL Decode(const PString & str);

    MediaType GetMediaType() const { return mediaType; }

    const SDPMediaFormatList & GetSDPMediaFormats() const
      { return formats; }

    OpalMediaFormatList GetMediaFormats() const;

    void AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat);

    void AddMediaFormat(const OpalMediaFormat & mediaFormat);
    void AddMediaFormats(const OpalMediaFormatList & mediaFormats);

    void SetAttribute(const PString & attr);

    const OpalTransportAddress & GetTransportAddress() const { return transportAddress; }

    PString GetTransport() const         { return transport; }
    void SetTransport(const PString & v) { transport = v; }

  protected:
    MediaType mediaType;
    WORD portCount;
    PCaselessString media;
    PCaselessString transport;
    OpalTransportAddress transportAddress;

    SDPMediaFormatList formats;
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
    );
    void AddMediaDescription(SDPMediaDescription * md) { mediaDescriptions.Append(md); }

    const OpalTransportAddress & GetDefaultConnectAddress() const { return defaultConnectAddress; }
    void SetDefaultConnectAddress(
      const OpalTransportAddress & address
    ) { defaultConnectAddress = address; }


  protected:
    void ParseOwner(const PString & str);

    SDPMediaDescriptionList mediaDescriptions;

    PINDEX protocolVersion;
    PString sessionName;

    PString ownerUsername;
    unsigned ownerSessionId;
    unsigned ownerVersion;
    OpalTransportAddress ownerAddress;
    OpalTransportAddress defaultConnectAddress;
};

/////////////////////////////////////////////////////////


#endif // __OPAL_SDP_H


// End of File ///////////////////////////////////////////////////////////////
