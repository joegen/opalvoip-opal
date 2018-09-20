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
 */

#ifndef OPAL_SIP_SDP_H
#define OPAL_SIP_SDP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_SDP

#include <opal/transports.h>
#include <opal/mediatype.h>
#include <opal/mediafmt.h>
#include <rtp/rtp_session.h>
#include <ptclib/pssl.h>


/**OpalConnection::StringOption key to a boolean indicating the SDP ptime
   parameter should be included in audio session streams. Default true.
  */
#define OPAL_OPT_OFFER_SDP_PTIME "Offer-SDP-PTime"

/**OpalConnection::StringOption key to an integer indicating the SDP rtpcp-fb
   parameter handling. A value of zero indicates no rtcp-fb options are ever
   to be offered to the remote. A value of 1 indicates if some media formats
   contain other than OpalMediaFormat::e_NoRTCPFb, an RTP/AVPF is to be
   offered. Also, in this case, an answer will contain rtcp-fb options without
   requiring the RTP/AVPF transport in the remote offer, but is included in
   the basic RTP/AVP. A value of 2 indicates RTP/AVPF is offered even if no
   media formats contains RTCPFb options. Note if remote indicated RTP/AVPF
   then mode 2 is assumed for the answer. Default is 1.
  */
#define OPAL_OPT_OFFER_RTCP_FB  "Offer-RTCP-FB"

/**OpalConnection::StringOption key to a boolean indicating the RTCP
   feedback commands are to be sent irrespective of the SDP rtpcp-fb
   parameter presented by the remote. Default false.
  */
#define OPAL_OPT_FORCE_RTCP_FB  "Force-RTCP-FB"

/**Enable offer of RTP/RTCP "single port" mode.
   While if offered by the remote, we always honour the SDP a=rtcp-mux
   attribute, we only offer it if this string option is set to true.

   Defaults to false.
  */
#define OPAL_OPT_RTCP_MUX "RTCP-Mux"

/**OpalConnection::StringOption key to a boolean indicating the RFC5506
   reduced size RTCP support should be offered in the SDP. Default true.
  */
#define OPAL_OPT_OFFER_REDUCED_SIZE_RTCP "Offer-Reduced-Size-RTCP"

/**Suppress UDP/TLS in SDP transport offer.
   When offering DTLS, should use UDP/TLS/RTP/SAVPF as the transport, but for
   interoperability reasons (*cough*Chrome*cough*) need to set only the
   RTP/SAVPF part.
   Defaults to false.
*/
#define OPAL_OPT_SUPPRESS_UDP_TLS "Suppress-UDP-TLS"

/**Include SSRC information in SDP.
   Defaults to true.
*/
#define OPAL_OPT_SDP_SSRC_INFO "SDP-SSRC-Info"

/**Enable ICE offered in SDP.
   Defaults to false.
*/
#define OPAL_OPT_OFFER_ICE "Offer-ICE"


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
  protected:
    SDPMediaFormat(SDPMediaDescription & parent);

  public:
    virtual bool FromSDP(
      const PString & portString
    );

    virtual void FromMediaFormat(
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
    virtual bool AdjustMediaFormat(OpalMediaFormat & mediaFormat, unsigned bandwidth) const;
    virtual void SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const;

    SDPMediaDescription       & m_parent;
    OpalMediaFormat             m_mediaFormat;
    RTP_DataFrame::PayloadTypes m_payloadType;
    unsigned                    m_clockRate;
    PCaselessString             m_encodingName;
    PString                     m_parameters;
    PString                     m_fmtp;

    P_REMOVE_VIRTUAL(bool,Initialise(const PString &),false);
    P_REMOVE_VIRTUAL_VOID(Initialise(const OpalMediaFormat &));

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

#if OPAL_SRTP // DTLS
    P_DECLARE_BITWISE_ENUM_EX(SetupModes, 3, (
      SetupNotSet,
      SetupActive,
      SetupPassive,
      SetupHoldConnection
    ),
      SetupActivePassive = SetupActive | SetupPassive
    );

    enum ConnectionMode
    {
      ConnectionNotSet,
      ConnectionNew,
      ConnectionExisting
    };
#endif

    typedef PDictionary<PString, PStringSet> GroupDict;

    SDPCommonAttributes()
      : m_direction(Undefined)
#if OPAL_SRTP // DTLS
      , m_setupMode(SetupNotSet)
      , m_connectionMode(ConnectionNotSet)
#endif
    { }

    virtual ~SDPCommonAttributes() { }

    virtual void SetDirection(const Direction & d) { m_direction = d; }
    virtual Direction GetDirection() const { return m_direction; }

    virtual OpalBandwidth GetBandwidth(const PString & type) const { return m_bandwidth[type]; }
    virtual void SetBandwidth(const PString & type, OpalBandwidth value) { m_bandwidth[type] = value; }

    virtual const SDPBandwidth & GetBandwidth() const { return m_bandwidth; }

    virtual const RTPHeaderExtensions & GetHeaderExtensions() const { return m_headerExtensions; }
    virtual void SetHeaderExtension(RTPHeaderExtensionInfo & ext);

    virtual void ParseAttribute(const PString & value);
    virtual void SetAttribute(const PString & attr, const PString & value);

    virtual void OutputAttributes(ostream & strm) const;

#if OPAL_SRTP
    SetupModes GetSetupMode() const { return m_setupMode; }
    void SetSetupMode(SetupModes setupType) { m_setupMode = setupType; }
    ConnectionMode GetConnectionMode() const { return m_connectionMode; }
    void SetConnectionMode(ConnectionMode mode) { m_connectionMode = mode; }
    const PSSLCertificateFingerprint& GetFingerprint() const { return m_fingerprint; }
    void SetFingerprint(const PSSLCertificateFingerprint& fp) { m_fingerprint = fp; }
#endif

    void SetStringOptions(const PStringOptions & options) { m_stringOptions = options; }
    const PStringOptions & GetStringOptions() const { return m_stringOptions; }

    static const PCaselessString & ConferenceTotalBandwidthType();
    static const PCaselessString & ApplicationSpecificBandwidthType();
    static const PCaselessString & TransportIndependentBandwidthType(); // RFC3890

#if OPAL_ICE
    const PString & GetUsername() const { return m_username; }
    const PString & GetPassword() const { return m_password; }
    void SetUserPass(
      const PString & username,
      const PString & password
    ) {
      m_username = username;
      m_password = password;
    }
#endif //OPAL_ICE

  protected:
    Direction           m_direction;
    SDPBandwidth        m_bandwidth;
    RTPHeaderExtensions m_headerExtensions;
#if OPAL_SRTP // DTLS
    SetupModes                 m_setupMode;
    ConnectionMode             m_connectionMode;
    PSSLCertificateFingerprint m_fingerprint;
#endif
#if OPAL_ICE
    PStringSet          m_iceOptions;
    PString             m_username;
    PString             m_password;
#endif //OPAL_ICE
    PStringOptions      m_stringOptions;
};


/////////////////////////////////////////////////////////

class SDPMediaDescription : public PObject, public SDPCommonAttributes
{
    PCLASSINFO(SDPMediaDescription, PObject);
  protected:
    SDPMediaDescription();
    SDPMediaDescription(
      const OpalTransportAddress & address,
      const OpalMediaType & mediaType
    );

  public:
    virtual bool PreEncode();
    virtual void Encode(const OpalTransportAddress & commonAddr, ostream & str) const;

    virtual bool Decode(const PStringArray & tokens);
    virtual bool Decode(char key, const PString & value);
    virtual bool PostDecode(const OpalMediaFormatList & mediaFormats);

    // return the string used within SDP to identify this media type
    virtual PString GetSDPMediaType() const;

    // return the string used within SDP to identify the transport used by this media
    virtual PCaselessString GetSDPTransportType() const;
    virtual void SetSDPTransportType(const PString & type);

    // return the string used in factory to create session
    virtual PCaselessString GetSessionType() const;

    virtual const SDPMediaFormatList & GetSDPMediaFormats() const;

    virtual OpalMediaFormatList GetMediaFormats() const;

    virtual void AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat);

    virtual void AddMediaFormat(const OpalMediaFormat & mediaFormat);
    virtual void AddMediaFormats(const OpalMediaFormatList & mediaFormats, const OpalMediaType & mediaType);

#if OPAL_SRTP
    virtual void SetCryptoKeys(OpalMediaCryptoKeyList & cryptoKeys);
    virtual OpalMediaCryptoKeyList GetCryptoKeys() const;
    virtual bool IsSecure() const;
#endif

    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual void OutputAttributes(ostream & str) const;

    virtual Direction GetDirection() const { return m_mediaAddress.IsEmpty() ? Inactive : m_direction; }

    virtual bool FromSession(OpalMediaSession * session, const SDPMediaDescription * offer, RTP_SyncSourceId ssrc);
    virtual bool ToSession(OpalMediaSession * session, RTP_SyncSourceArray & ssrcs) const;
    virtual bool IsGroupMember(const PString & groupId) const { return m_groups.Contains(groupId); }
    virtual PStringArray GetGroups() const { return m_groups.GetKeys(); }
    virtual PString GetGroupMediaId(const PString & groupId) const { return m_groups(groupId); }
    virtual void MatchGroupInfo(const GroupDict & groups);
    bool IsBundleOnly() const { return m_bundleOnly; }

    const OpalTransportAddress & GetMediaAddress() const { return m_mediaAddress; }
    const OpalTransportAddress & GetControlAddress() const { return m_controlAddress; }
    bool SetAddresses(const OpalTransportAddress & media, const OpalTransportAddress & control);

    WORD GetPort() const { return m_port; }
    void SetPort(WORD port) { m_port = port; }

#if OPAL_ICE
    PNatCandidateList GetCandidates() const { return m_candidates; }
    bool HasICE() const;
    void SetICE(
        const PString & username,
        const PString & password,
        const PNatCandidateList & candidates
    );
#endif //OPAL_ICE

    virtual OpalMediaType GetMediaType() const { return m_mediaType; }

    virtual void CreateSDPMediaFormats(const PStringArray & tokens);
    virtual SDPMediaFormat * CreateSDPMediaFormat() = 0;

    virtual PString GetSDPPortList() const;

    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);

#if OPAL_VIDEO
    virtual OpalVideoFormat::ContentRole GetContentRole() const { return OpalVideoFormat::eNoRole; }
#endif

  protected:
    virtual SDPMediaFormat * FindFormat(PString & str) const;

    OpalTransportAddress m_mediaAddress;
    OpalTransportAddress m_controlAddress;
    WORD                 m_port;
    WORD                 m_portCount;
    OpalMediaType        m_mediaType;
    bool                 m_bundleOnly; // draft-ietf-mmusic-sdp-bundle-negotiation-52
    PStringList          m_mids;
    PStringToString      m_groups;
#if OPAL_ICE
    PNatCandidateList    m_candidates;
#endif //OPAL_ICE
    SDPMediaFormatList   m_formats;

  P_REMOVE_VIRTUAL(SDPMediaFormat *,CreateSDPMediaFormat(const PString &),0);
  P_REMOVE_VIRTUAL(OpalTransportAddress,GetTransportAddress(),OpalTransportAddress());
  P_REMOVE_VIRTUAL(PBoolean,SetTransportAddress(const OpalTransportAddress &),false);
  P_REMOVE_VIRTUAL_VOID(Copy(SDPMediaDescription &));
};

PARRAY(SDPMediaDescriptionArray, SDPMediaDescription);


class SDPDummyMediaDescription : public SDPMediaDescription
{
    PCLASSINFO(SDPDummyMediaDescription, SDPMediaDescription);
  public:
    SDPDummyMediaDescription() { }
    SDPDummyMediaDescription(const OpalTransportAddress & address, const PStringArray & tokens);

    virtual PString GetSDPMediaType() const;
    virtual PCaselessString GetSDPTransportType() const;
    virtual void SetSDPTransportType(const PString & type);
    virtual PCaselessString GetSessionType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat();
    virtual PString GetSDPPortList() const;

  private:
    PStringArray m_tokens;
};


#if OPAL_SRTP
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
#endif // OPAL_SRTP


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
    virtual void SetSDPTransportType(const PString & type);
    virtual PCaselessString GetSessionType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat();
    virtual PString GetSDPPortList() const;
    virtual bool PreEncode();
    virtual void OutputAttributes(ostream & str) const;

#if OPAL_SRTP
    virtual void SetCryptoKeys(OpalMediaCryptoKeyList & cryptoKeys);
    virtual OpalMediaCryptoKeyList GetCryptoKeys() const;
    virtual bool IsSecure() const;
#endif
    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual bool FromSession(OpalMediaSession * session, const SDPMediaDescription * offer, RTP_SyncSourceId ssrc);
    virtual bool ToSession(OpalMediaSession * session, RTP_SyncSourceArray & ssrcs) const;

    // RFC5576
    typedef std::map<RTP_SyncSourceId, PStringOptions> SsrcInfo;
    const SsrcInfo & GetSsrcInfo() const { return m_ssrcInfo; }

  protected:
    class Format : public SDPMediaFormat
    {
      public:
        Format(SDPRTPAVPMediaDescription & parent) : SDPMediaFormat(parent) { }
        virtual bool FromSDP(const PString & portString);

        virtual void PrintOn(ostream & str) const;
        virtual bool PreEncode();

        void AddRTCP_FB(const PString & str);
        void SetRTCP_FB(const OpalMediaFormat::RTCPFeedback & v) { m_rtcp_fb = v; }
        OpalMediaFormat::RTCPFeedback GetRTCP_FB() const { return m_rtcp_fb; }

      protected:
        virtual void SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const;

        OpalMediaFormat::RTCPFeedback m_rtcp_fb; // RFC4585
    };

    PCaselessString               m_transportType;
    bool                          m_reducedSizeRTCP;
    SsrcInfo                      m_ssrcInfo;  // RFC5576
    PString                       m_label;
    PString                       m_msid;
    vector<RTP_SyncSourceArray>   m_flowSSRC;
    OpalMediaFormat::RTCPFeedback m_rtcp_fb;
#if OPAL_SRTP
    PList<SDPCryptoSuite>         m_cryptoSuites;
#endif
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

        void ParseImageAttr(const PString & params);

      protected:
        virtual void SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const;

        unsigned m_minRxWidth;
        unsigned m_minRxHeight;
        unsigned m_maxRxWidth;
        unsigned m_maxRxHeight;
        unsigned m_minTxWidth;
        unsigned m_minTxHeight;
        unsigned m_maxTxWidth;
        unsigned m_maxTxHeight;
    };

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

    static const PCaselessString & TypeName();

  protected:
    class Format : public SDPMediaFormat
    {
      public:
        Format(SDPApplicationMediaDescription & parent) : SDPMediaFormat(parent) { }
        virtual bool FromSDP(const PString & portString);
    };
};

/////////////////////////////////////////////////////////

class SDPSessionDescription : public PObject, public SDPCommonAttributes
{
    PCLASSINFO_WITH_CLONE(SDPSessionDescription, PObject);
  public:
    SDPSessionDescription(
      time_t sessionId,
      unsigned version,
      const OpalTransportAddress & address
    );

    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual PString Encode() const;
    virtual bool Decode(const PString & str, const OpalMediaFormatList & mediaFormats);
    virtual bool Decode(const PStringArray & lines, const OpalMediaFormatList & mediaFormats);
    virtual void SetAttribute(const PString & attr, const PString & value);

    void SetSessionName(const PString & v);
    PString GetSessionName() const { return sessionName; }

    void SetUserName(const PString & v);
    PString GetUserName() const { return ownerUsername; }

    const SDPMediaDescriptionArray & GetMediaDescriptions() const { return mediaDescriptions; }

    PINDEX GetMediaDescriptionIndexByType(const OpalMediaType & rtpMediaType) const;
    SDPMediaDescription * GetMediaDescriptionByType(const OpalMediaType & rtpMediaType) const;
    SDPMediaDescription * GetMediaDescriptionByIndex(PINDEX i) const;
    void AddMediaDescription(SDPMediaDescription * md);
    
    virtual SDPMediaDescription::Direction GetDirection(unsigned) const;
    bool IsHold() const;
    bool HasActiveSend() const;

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

    GroupDict GetGroups() const { return m_groups; }

#if OPAL_ICE
    PStringSet GetICEOptions() const { return m_iceOptions; }
#endif

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

    GroupDict    m_groups;

    PStringArray m_mediaStreamIds;
};

/////////////////////////////////////////////////////////


#endif // OPAL_SDP

#endif // OPAL_SIP_SDP_H


// End of File ///////////////////////////////////////////////////////////////
