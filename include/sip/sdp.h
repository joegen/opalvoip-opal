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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_SIP_SDP_H
#define OPAL_SIP_SDP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_SIP

#include <opal/transports.h>
#include <opal/mediatype.h>
#include <opal/mediafmt.h>
#include <rtp/rtp_session.h>



/**OpalConnection::StringOption key to a boolean indicating the SDP ptime
   parameter should be included in audio session streams. Default false.
  */
#define OPAL_OPT_OFFER_SDP_PTIME "Offer-SDP-PTime"

/**OpalConnection::StringOption key to an integer indicating the SDP
   rtpcp-fb parameter handling. A value of zero indicates it is not to be
   offerred to the remote. A value of 1 indicates it is to be offerred
   without requiring the RTP/AVPF transport, but is included in RTP/AVP.
   A value of 2 indicates it is to be only offerred in RTP/AVPF with a
   second session in RTP/AVP mode to be also offerred. Default is 1.
  */
#define OPAL_OPT_OFFER_RTCP_FB  "Offer-RTCP-FB"

/**OpalConnection::StringOption key to a boolean indicating the RTCP
   feedback commands are to be sent irrespective of the SDP rtpcp-fb
   parameter presented by the remote. Default false.
  */
#define OPAL_OPT_FORCE_RTCP_FB  "Force-RTCP-FB"


/////////////////////////////////////////////////////////

class SDPBandwidth : public std::map<PCaselessString, OpalBandwidth>
{
  typedef std::map<PCaselessString, OpalBandwidth> BaseClass;
  public:
    OpalBandwidth & operator[](const PCaselessString & type);
    OpalBandwidth operator[](const PCaselessString & type) const;
    friend ostream & operator<<(ostream & out, const SDPBandwidth & bw);
    bool Parse(const PString & param);
    void SetMax(const PCaselessString & type, OpalBandwidth value);
};

/////////////////////////////////////////////////////////

class SDPMediaDescription;

class SDPMediaFormat : public PObject
{
  PCLASSINFO(SDPMediaFormat, PObject);
  public:
    SDPMediaFormat(
      SDPMediaDescription & parent,
      RTP_DataFrame::PayloadTypes payloadType,
      const char * name = NULL
    );

    SDPMediaFormat(
      SDPMediaDescription & parent,
      const OpalMediaFormat & mediaFormat
    );

    virtual void PrintOn(ostream & str) const;

    RTP_DataFrame::PayloadTypes GetPayloadType() const { return payloadType; }

    const PCaselessString & GetEncodingName() const { return encodingName; }
    void SetEncodingName(const PString & v) { encodingName = v; }

    void SetFMTP(const PString & _fmtp); 
    PString GetFMTP() const;

    unsigned GetClockRate(void)                        { return clockRate ; }
    void SetClockRate(unsigned  v)                     { clockRate = v; }

    void SetParameters(const PString & v) { parameters = v; }
    void SetRTCP_FB(const PString & v) { m_rtcp_fb.FromString(v); }
    OpalVideoFormat::RTCPFeedback GetRTCP_FB() const { return m_rtcp_fb; }

    const OpalMediaFormat & GetMediaFormat() const { return m_mediaFormat; }
    OpalMediaFormat & GetWritableMediaFormat() { return m_mediaFormat; }

    bool PreEncode();
    bool PostDecode(const OpalMediaFormatList & mediaFormats, unsigned bandwidth);

  protected:
    void SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const;

    OpalMediaFormat m_mediaFormat;

    SDPMediaDescription & m_parent;
    RTP_DataFrame::PayloadTypes payloadType;
    unsigned clockRate;
    PCaselessString encodingName;
    PString parameters;
    PString m_fmtp;
    OpalVideoFormat::RTCPFeedback m_rtcp_fb; // RFC4585
};

typedef PList<SDPMediaFormat> SDPMediaFormatList;


/////////////////////////////////////////////////////////

class SDPCommonAttributes
{
  public:
    // The following enum is arranged so it can be used as a bit mask,
    // e.g. GetDirection()&SendOnly indicates send is available
    enum Direction {
      Undefined = -1,
      Inactive,
      RecvOnly,
      SendOnly,
      SendRecv
    };

    SDPCommonAttributes()
      : m_direction(Undefined)
    { }

    virtual ~SDPCommonAttributes() { }

    virtual void SetDirection(const Direction & d) { m_direction = d; }
    virtual Direction GetDirection() const { return m_direction; }

    virtual OpalBandwidth GetBandwidth(const PString & type) const { return m_bandwidth[type]; }
    virtual void SetBandwidth(const PString & type, OpalBandwidth value) { m_bandwidth[type] = value; }

    virtual const SDPBandwidth & GetBandwidth() const { return m_bandwidth; }

    virtual const RTPExtensionHeaders & GetExtensionHeaders() const { return m_extensionHeaders; }
    virtual void SetExtensionHeader(const RTPExtensionHeaderInfo & ext) { m_extensionHeaders.insert(ext); }

    virtual void ParseAttribute(const PString & value);
    virtual void SetAttribute(const PString & attr, const PString & value);

    virtual void OutputAttributes(ostream & strm) const;

    static const PCaselessString & ConferenceTotalBandwidthType();
    static const PCaselessString & ApplicationSpecificBandwidthType();
    static const PCaselessString & TransportIndependentBandwidthType(); // RFC3890

  protected:
    Direction           m_direction;
    SDPBandwidth        m_bandwidth;
    RTPExtensionHeaders m_extensionHeaders;
};


/////////////////////////////////////////////////////////

class SDPMediaDescription : public PObject, public SDPCommonAttributes
{
  PCLASSINFO(SDPMediaDescription, PObject);
  public:
    SDPMediaDescription(
      const OpalTransportAddress & address,
      const OpalMediaType & mediaType
    );

    virtual bool PreEncode();
    virtual void Encode(const OpalTransportAddress & commonAddr, ostream & str) const;

    virtual bool Decode(const PStringArray & tokens);
    virtual bool Decode(char key, const PString & value);
    virtual bool PostDecode(const OpalMediaFormatList & mediaFormats);

    // return the string used within SDP to identify this media type
    virtual PString GetSDPMediaType() const = 0;

    // return the string used within SDP to identify the transport used by this media
    virtual PCaselessString GetSDPTransportType() const = 0;

    virtual const SDPMediaFormatList & GetSDPMediaFormats() const
      { return formats; }

    virtual OpalMediaFormatList GetMediaFormats() const;

    virtual void AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat);

    virtual void AddMediaFormat(const OpalMediaFormat & mediaFormat);
    virtual void AddMediaFormats(const OpalMediaFormatList & mediaFormats, const OpalMediaType & mediaType);

    virtual void SetCryptoKeys(OpalMediaCryptoKeyList & cryptoKeys);
    virtual OpalMediaCryptoKeyList GetCryptoKeys() const;

    virtual void SetAttribute(const PString & attr, const PString & value);

    virtual Direction GetDirection() const { return m_transportAddress.IsEmpty() ? Inactive : m_direction; }

    virtual const OpalTransportAddress & GetTransportAddress() const { return m_transportAddress; }
    virtual PBoolean SetTransportAddress(const OpalTransportAddress &t);

    virtual WORD GetPort() const { return m_port; }

    virtual OpalMediaType GetMediaType() const { return m_mediaType; }

    virtual void CreateSDPMediaFormats(const PStringArray & tokens);
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString) = 0;

    virtual PString GetSDPPortList() const;

    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);

    virtual OpalVideoFormat::ContentRole GetContentRole() const { return OpalVideoFormat::eNoRole; }

    void SetOptionStrings(const PStringOptions & options) { m_stringOptions = options; }
    const PStringOptions & GetOptionStrings() const { return m_stringOptions; }

    virtual void Copy(SDPMediaDescription & mediaDescription);

  protected:
    virtual SDPMediaFormat * FindFormat(PString & str) const;

    OpalTransportAddress m_transportAddress;
    PCaselessString      m_transportType;
    PStringOptions       m_stringOptions;
    WORD                 m_port;
    WORD                 m_portCount;
    OpalMediaType        m_mediaType;

    SDPMediaFormatList   formats;
};

PARRAY(SDPMediaDescriptionArray, SDPMediaDescription);


class SDPDummyMediaDescription : public SDPMediaDescription
{
    PCLASSINFO(SDPDummyMediaDescription, SDPMediaDescription);
  public:
    SDPDummyMediaDescription(const OpalTransportAddress & address, const PStringArray & tokens);
    virtual PString GetSDPMediaType() const;
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual PString GetSDPPortList() const;
    virtual void Copy(SDPMediaDescription & mediaDescription);

  private:
    PStringArray m_tokens;
};


class SDPDummySession : public OpalMediaSession
{
    PCLASSINFO(SDPDummySession, OpalMediaSession)
  public:
    SDPDummySession(const Init & init);
    static const PCaselessString & SessionType();
    virtual const PCaselessString & GetSessionType() const;
    virtual SDPMediaDescription * CreateSDPMediaDescription();
    virtual OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource);

  private:
    PStringArray m_tokens;
};


class SDPCryptoSuite : public PObject
{
    PCLASSINFO(SDPCryptoSuite, PObject)
  public:
    SDPCryptoSuite(unsigned tag);

    bool SetKeyInfo(const OpalMediaCryptoKeyInfo & keyInfo);
    OpalMediaCryptoKeyInfo * GetKeyInfo() const;

    bool Decode(const PString & attrib);
    void PrintOn(ostream & strm) const;

    struct KeyParam {
      KeyParam(const PString & keySalt)
        : m_keySalt(keySalt)
        , m_lifetime(0)
        , m_mkiIndex(0)
        , m_mkiLength(0)
      { }

      PString  m_keySalt;
      PUInt64  m_lifetime;
      unsigned m_mkiIndex;
      unsigned m_mkiLength;
    };

    unsigned GetTag() const { return m_tag; }
    const PString & GetName() const { return m_suiteName; }

  protected:
    unsigned       m_tag;
    PString        m_suiteName;
    list<KeyParam> m_keyParams;
    PStringOptions m_sessionParams;
};

/////////////////////////////////////////////////////////
//
//  SDP media description for media classes using RTP/AVP transport (audio and video)
//

class SDPRTPAVPMediaDescription : public SDPMediaDescription
{
    PCLASSINFO(SDPRTPAVPMediaDescription, SDPMediaDescription);
  public:
    SDPRTPAVPMediaDescription(const OpalTransportAddress & address, const OpalMediaType & mediaType);
    virtual bool Decode(const PStringArray & tokens);
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual PString GetSDPPortList() const;
    virtual bool PreEncode();
    virtual void OutputAttributes(ostream & str) const;
    virtual void SetCryptoKeys(OpalMediaCryptoKeyList & cryptoKeys);
    virtual OpalMediaCryptoKeyList GetCryptoKeys() const;
    virtual void SetAttribute(const PString & attr, const PString & value);

    void EnableFeedback() { m_enableFeedback = true; }

  protected:
    bool m_enableFeedback;
    OpalVideoFormat::RTCPFeedback m_rtcp_fb;
    PList<SDPCryptoSuite>         m_cryptoSuites;
};

/////////////////////////////////////////////////////////
//
//  SDP media description for audio media
//

class SDPAudioMediaDescription : public SDPRTPAVPMediaDescription
{
    PCLASSINFO(SDPAudioMediaDescription, SDPRTPAVPMediaDescription);
  public:
    SDPAudioMediaDescription(const OpalTransportAddress & address);
    virtual PString GetSDPMediaType() const;
    virtual void OutputAttributes(ostream & str) const;
    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual bool PostDecode(const OpalMediaFormatList & mediaFormats);

  protected:
    unsigned m_PTime;
    unsigned m_maxPTime;
};


#if OPAL_VIDEO

/////////////////////////////////////////////////////////
//
//  SDP media description for video media
//

class SDPVideoMediaDescription : public SDPRTPAVPMediaDescription
{
    PCLASSINFO(SDPVideoMediaDescription, SDPRTPAVPMediaDescription);
  public:
    SDPVideoMediaDescription(const OpalTransportAddress & address);
    virtual PString GetSDPMediaType() const;
    virtual bool PreEncode();
    virtual void OutputAttributes(ostream & str) const;
    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual bool PostDecode(const OpalMediaFormatList & mediaFormats);
    virtual OpalVideoFormat::ContentRole GetContentRole() const { return m_contentRole; }

  protected:
    OpalVideoFormat::ContentRole m_contentRole;
    unsigned                     m_contentMask;
};

#endif // OPAL_VIDEO


/////////////////////////////////////////////////////////
//
//  SDP media description for application media
//

class SDPApplicationMediaDescription : public SDPMediaDescription
{
  PCLASSINFO(SDPApplicationMediaDescription, SDPMediaDescription);
  public:
    SDPApplicationMediaDescription(const OpalTransportAddress & address);
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual PString GetSDPMediaType() const;
};

/////////////////////////////////////////////////////////

class SDPSessionDescription : public PObject, public SDPCommonAttributes
{
  PCLASSINFO(SDPSessionDescription, PObject);
  public:
    SDPSessionDescription(
      time_t sessionId,
      unsigned version,
      const OpalTransportAddress & address
    );

    void PrintOn(ostream & strm) const;
    PString Encode() const;
    bool Decode(const PString & str, const OpalMediaFormatList & mediaFormats);

    void SetSessionName(const PString & v);
    PString GetSessionName() const { return sessionName; }

    void SetUserName(const PString & v);
    PString GetUserName() const { return ownerUsername; }

    const SDPMediaDescriptionArray & GetMediaDescriptions() const { return mediaDescriptions; }

    SDPMediaDescription * GetMediaDescriptionByType(const OpalMediaType & rtpMediaType) const;
    SDPMediaDescription * GetMediaDescriptionByIndex(PINDEX i) const;
    void AddMediaDescription(SDPMediaDescription * md) { mediaDescriptions.Append(PAssertNULL(md)); }
    
    virtual SDPMediaDescription::Direction GetDirection(unsigned) const;
    bool IsHold() const;

    const OpalTransportAddress & GetDefaultConnectAddress() const { return defaultConnectAddress; }
    void SetDefaultConnectAddress(
      const OpalTransportAddress & address
    );
  
    time_t GetOwnerSessionId() const { return ownerSessionId; }
    void SetOwnerSessionId(time_t value) { ownerSessionId = value; }

    unsigned GetOwnerVersion() const { return ownerVersion; }
    void SetOwnerVersion(unsigned value) { ownerVersion = value; }

    OpalTransportAddress GetOwnerAddress() const { return ownerAddress; }
    void SetOwnerAddress(OpalTransportAddress addr) { ownerAddress = addr; }

    OpalMediaFormatList GetMediaFormats() const;

  protected:
    void ParseOwner(const PString & str);

    SDPMediaDescriptionArray mediaDescriptions;

    PINDEX protocolVersion;
    PString sessionName;

    PString ownerUsername;
    time_t ownerSessionId;
    unsigned ownerVersion;
    OpalTransportAddress ownerAddress;
    OpalTransportAddress defaultConnectAddress;
};

/////////////////////////////////////////////////////////


#endif // OPAL_SIP

#endif // OPAL_SIP_SDP_H


// End of File ///////////////////////////////////////////////////////////////
