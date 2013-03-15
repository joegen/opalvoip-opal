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

class SDPBandwidth : public std::map<PCaselessString, unsigned>
{
  public:
    unsigned & operator[](const PCaselessString & type);
    unsigned operator[](const PCaselessString & type) const;
    friend ostream & operator<<(ostream & out, const SDPBandwidth & bw);
    bool Parse(const PString & param);
    void SetMax(const PCaselessString & type, unsigned value);
};

/////////////////////////////////////////////////////////

class SDPMediaDescription;

class SDPMediaFormat : public PObject
{
  PCLASSINFO(SDPMediaFormat, PObject);
  protected:
    SDPMediaFormat(SDPMediaDescription & parent);

  public:
    virtual bool Initialise(
      const PString & portString
    );

    virtual void Initialise(
      const OpalMediaFormat & mediaFormat
    );

    virtual void PrintOn(ostream & str) const;
    virtual PObject * Clone() const { return new SDPMediaFormat(*this); }

    RTP_DataFrame::PayloadTypes GetPayloadType() const { return m_payloadType; }

    const PCaselessString & GetEncodingName() const { return m_encodingName; }
    void SetEncodingName(const PString & v) { m_encodingName = v; }

    void SetFMTP(const PString & _fmtp); 
    PString GetFMTP() const;

    unsigned GetClockRate(void)    { return m_clockRate ; }
    void SetClockRate(unsigned  v) { m_clockRate = v; }

    void SetParameters(const PString & v) { m_parameters = v; }

    const OpalMediaFormat & GetMediaFormat() const { return m_mediaFormat; }
    OpalMediaFormat & GetWritableMediaFormat() { return m_mediaFormat; }

    virtual bool PreEncode();
    virtual bool PostDecode(const OpalMediaFormatList & mediaFormats, unsigned bandwidth);

  protected:
    virtual void SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const;

    SDPMediaDescription       & m_parent;
    OpalMediaFormat             m_mediaFormat;
    RTP_DataFrame::PayloadTypes m_payloadType;
    unsigned                    m_clockRate;
    PCaselessString             m_encodingName;
    PString                     m_parameters;
    PString                     m_fmtp;
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

    virtual unsigned GetBandwidth(const PString & type) const { return m_bandwidth[type]; }
    virtual void SetBandwidth(const PString & type, unsigned value) { m_bandwidth[type] = value; }

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
      { return m_formats; }

    virtual OpalMediaFormatList GetMediaFormats() const;

    virtual void AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat);

    virtual void AddMediaFormat(const OpalMediaFormat & mediaFormat);
    virtual void AddMediaFormats(const OpalMediaFormatList & mediaFormats, const OpalMediaType & mediaType);

    virtual void SetAttribute(const PString & attr, const PString & value);

    virtual Direction GetDirection() const { return m_transportAddress.IsEmpty() ? Inactive : m_direction; }

    virtual const OpalTransportAddress & GetTransportAddress() const { return m_transportAddress; }
    virtual PBoolean SetTransportAddress(const OpalTransportAddress &t);

    virtual WORD GetPort() const { return m_port; }

    virtual OpalMediaType GetMediaType() const { return m_mediaType; }

    virtual void CreateSDPMediaFormats(const PStringArray & tokens);
    virtual SDPMediaFormat * CreateSDPMediaFormat() = 0;

    virtual PString GetSDPPortList() const;

    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);

#if OPAL_VIDEO
    virtual OpalVideoFormat::ContentRole GetContentRole() const { return OpalVideoFormat::eNoRole; }
#endif

    void SetOptionStrings(const PStringOptions & options) { m_stringOptions = options; }
    const PStringOptions & GetOptionStrings() const { return m_stringOptions; }

  protected:
    virtual SDPMediaFormat * FindFormat(PString & str) const;

    OpalTransportAddress m_transportAddress;
    PCaselessString      m_transportType;
    PStringOptions       m_stringOptions;
    WORD                 m_port;
    WORD                 m_portCount;
    OpalMediaType        m_mediaType;

    SDPMediaFormatList   m_formats;

  P_REMOVE_VIRTUAL(SDPMediaFormat *,CreateSDPMediaFormat(const PString &),0);
};

PARRAY(SDPMediaDescriptionArray, SDPMediaDescription);


class SDPDummyMediaDescription : public SDPMediaDescription
{
  PCLASSINFO(SDPDummyMediaDescription, SDPMediaDescription);
  public:
    SDPDummyMediaDescription(const OpalTransportAddress & address, const PStringArray & tokens);
    virtual PString GetSDPMediaType() const;
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat();
    virtual PString GetSDPPortList() const;

  private:
    PStringArray m_tokens;
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
    virtual SDPMediaFormat * CreateSDPMediaFormat();
    virtual PString GetSDPPortList() const;
    virtual void OutputAttributes(ostream & str) const;
    virtual void SetAttribute(const PString & attr, const PString & value);

    void EnableFeedback() { m_enableFeedback = true; }

  protected:
    class Format : public SDPMediaFormat
    {
      public:
        Format(SDPRTPAVPMediaDescription & parent) : SDPMediaFormat(parent) { }
        bool Initialise(const PString & portString);
    };

    bool m_enableFeedback;
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
    virtual SDPMediaFormat * CreateSDPMediaFormat();
    virtual bool PreEncode();
    virtual void OutputAttributes(ostream & str) const;
    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual bool PostDecode(const OpalMediaFormatList & mediaFormats);
    virtual OpalVideoFormat::ContentRole GetContentRole() const { return m_contentRole; }

  protected:
    class Format : public SDPRTPAVPMediaDescription::Format
    {
      public:
        Format(SDPVideoMediaDescription & parent);

        virtual void PrintOn(ostream & str) const;
        virtual PObject * Clone() const { return new Format(*this); }

        virtual bool PreEncode();

        void SetRTCP_FB(const PString & v) { m_rtcp_fb.FromString(v); }
        OpalVideoFormat::RTCPFeedback GetRTCP_FB() const { return m_rtcp_fb; }

        void ParseImageAttr(const PString & params);

      protected:
        virtual void SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const;

        OpalVideoFormat::RTCPFeedback m_rtcp_fb; // RFC4585

        unsigned m_minRxWidth;
        unsigned m_minRxHeight;
        unsigned m_maxRxWidth;
        unsigned m_maxRxHeight;
        unsigned m_txWidth;
        unsigned m_txHeight;
    };

    OpalVideoFormat::RTCPFeedback m_rtcp_fb;
    OpalVideoFormat::ContentRole  m_contentRole;
    unsigned                      m_contentMask;
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
    virtual SDPMediaFormat * CreateSDPMediaFormat();
    virtual PString GetSDPMediaType() const;

  protected:
    class Format : public SDPMediaFormat
    {
      public:
        Format(SDPApplicationMediaDescription & parent) : SDPMediaFormat(parent) { }
        bool Initialise(const PString & portString);
    };
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
