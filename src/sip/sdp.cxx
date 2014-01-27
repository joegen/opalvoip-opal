/*
 * sdp.cxx
 *
 * Session Description Protocol support.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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

#include <ptlib.h>
#include <opal_config.h>

#if OPAL_SIP

#ifdef __GNUC__
#pragma implementation "sdp.h"
#endif

#include <sip/sdp.h>

#include <ptlib/socket.h>
#include <opal/transports.h>
#include <codec/opalplugin.h>
#include <rtp/srtp_session.h>


#define SIP_DEFAULT_SESSION_NAME    "Opal SIP Session"

static const char SDPBandwidthPrefix[] = "SDP-Bandwidth-";


#define new PNEW

#define SDP_MIN_PTIME 10

static char const CRLF[] = "\r\n";
static PConstString const WhiteSpace(" \t\r\n");
static char const TokenChars[] = "!#$%&'*+-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`abcdefghijklmnopqrstuvwxyz{|}~"; // From RFC4566
static char const * const CandidateTypeNames[PNatCandidate::NumTypes] = { "host", "srflx", "prflx", "relay" };
static PConstString const IceLiteOption("ice-lite");


/////////////////////////////////////////////////////////
//
//  the following functions bind the media type factory to the SDP format types
//

PString OpalMediaTypeDefinition::GetSDPMediaType() const
{
  return m_mediaType.c_str();
}


PString OpalMediaTypeDefinition::GetSDPTransportType() const
{
  return PString::Empty();
}


bool OpalMediaTypeDefinition::MatchesSDP(const PCaselessString & sdpMediaType,
                                         const PCaselessString & sdpTransport,
                                         const PStringArray & /*sdpLines*/,
                                         PINDEX /*index*/)
{
  if (sdpMediaType != GetSDPMediaType())
    return false;

  PCaselessString myTransport = GetSDPTransportType();
  if (myTransport.IsEmpty())
    return true;

  return myTransport == sdpTransport;
}


SDPMediaDescription * OpalMediaTypeDefinition::CreateSDPMediaDescription(const OpalTransportAddress &) const
{
  return NULL;
}


bool OpalRTPAVPMediaDefinition::MatchesSDP(const PCaselessString & sdpMediaType,
                                           const PCaselessString & sdpTransport,
                                           const PStringArray & sdpLines,
                                           PINDEX index)
{
  if (!OpalMediaTypeDefinition::MatchesSDP(sdpMediaType, sdpTransport, sdpLines, index))
    return false;

  return sdpTransport.NumCompare("RTP/") == PObject::EqualTo &&
         sdpTransport.Find("AVP") != P_MAX_INDEX;
}


SDPMediaDescription * OpalAudioMediaDefinition::CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const
{
  return new SDPAudioMediaDescription(localAddress);
}


#if OPAL_VIDEO
SDPMediaDescription * OpalVideoMediaDefinition::CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const
{
  return new SDPVideoMediaDescription(localAddress);
}
#endif


static OpalMediaType GetMediaTypeFromSDP(const PCaselessString & sdpMediaType,
                                         const PCaselessString & sdpTransport,
                                         const PStringArray & sdpLines,
                                         PINDEX index)
{
  OpalMediaTypeList mediaTypes = OpalMediaType::GetList();
  for (OpalMediaTypeList::iterator iterMediaType = mediaTypes.begin(); iterMediaType != mediaTypes.end(); ++iterMediaType) {
    if (iterMediaType->GetDefinition()->MatchesSDP(sdpMediaType, sdpTransport, sdpLines, index))
      return *iterMediaType;
  }

  return OpalMediaType();
}


/////////////////////////////////////////////////////////

static OpalTransportAddress ParseConnectAddress(const PStringArray & tokens, PINDEX offset, WORD port = 0)
{
  if (tokens.GetSize() == offset+3) {
    if (tokens[offset] *= "IN") {
      if (
        (tokens[offset+1] *= "IP4")
#if OPAL_PTLIB_IPV6
        || (tokens[offset+1] *= "IP6")
#endif
        ) {
        if (tokens[offset+2] == "255.255.255.255") {
          PTRACE(2, "SDP\tInvalid connection address 255.255.255.255 used, treating like HOLD request.");
        }
        else if (tokens[offset+2] == "0.0.0.0") {
          PTRACE(3, "SDP\tConnection address of 0.0.0.0 specified for HOLD request.");
        }
        else {
          PIPSocket::Address ip(tokens[offset+2]);
          if (ip.IsValid())
            return OpalTransportAddress(ip, port, OpalTransportAddress::UdpPrefix());
          PTRACE(1, "SDP\tConnect address has invalid IP address \"" << tokens[offset+2] << '"');
        }
      }
      else
      {
        PTRACE(1, "SDP\tConnect address has invalid address type \"" << tokens[offset+1] << '"');
      }
    }
    else {
      PTRACE(1, "SDP\tConnect address has invalid network \"" << tokens[offset] << '"');
    }
  }
  else {
    PTRACE(1, "SDP\tConnect address has invalid (" << tokens.GetSize() << ") elements");
  }

  return OpalTransportAddress();
}


static OpalTransportAddress ParseConnectAddress(const PString & str, WORD port = 0)
{
  PStringArray tokens = str.Tokenise(WhiteSpace, false); // Spec says space only, but lets be forgiving
  return ParseConnectAddress(tokens, 0, port);
}


static PString GetConnectAddressString(const OpalTransportAddress & address)
{
  PStringStream str;

  PIPSocket::Address ip;
  if (!address.IsEmpty() && address.GetIpAddress(ip) && ip.IsValid())
    str << "IN IP" << ip.GetVersion() << ' ' << ip.AsString(false, true);
  else
    str << "IN IP4 0.0.0.0";

  return str;
}


/////////////////////////////////////////////////////////

SDPMediaFormat::SDPMediaFormat(SDPMediaDescription & parent)
  : m_parent(parent)
  , m_payloadType(RTP_DataFrame::DynamicBase)
  , m_clockRate(0)
{
  PTRACE_CONTEXT_ID_FROM(parent);
}


bool SDPMediaFormat::Initialise(const PString &)
{
  return true;
}


void SDPMediaFormat::Initialise(const OpalMediaFormat & fmt)
{
  m_mediaFormat = fmt;
  m_payloadType = fmt.GetPayloadType();
  m_clockRate = fmt.GetClockRate();
  m_encodingName = fmt.GetEncodingName();

  if (fmt.GetMediaType() == OpalMediaType::Audio())
    m_parameters = PString(PString::Unsigned, fmt.GetOptionInteger(OpalAudioFormat::ChannelsOption()));

  m_parent.ProcessMediaOptions(*this, m_mediaFormat);
}


void SDPMediaFormat::SetFMTP(const PString & str)
{
  m_fmtp = str;
}


PString SDPMediaFormat::GetFMTP() const
{
  // Use GetMediaFormat() to force creation of member
  OpalMediaFormat mediaFormat = GetMediaFormat();
  if (mediaFormat.IsEmpty())
    return m_fmtp;

  PString fmtp = mediaFormat.GetOptionString("FMTP");
  if (!fmtp.IsEmpty())
    return fmtp;

  PStringStream strm;
  PStringSet used;
  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    const PString & name = option.GetFMTPName();
    // If the FMTP name for option is "FMTP" then it is the WHOLE thing
    // any other options with FMTP names are ignored.
    if (name == "FMTP")
      return option.AsString();
    if (!name.IsEmpty() && !used.Contains(name)) {
      used.Include(name);
      PString value = option.AsString();
      if (value.IsEmpty() && value != option.GetFMTPDefault())
        strm << name;
      else {
        PStringArray values = value.Tokenise(';', false);
        for (PINDEX v = 0; v < values.GetSize(); ++v) {
          value = values[v];
          if (value != option.GetFMTPDefault()) {
            if (!strm.IsEmpty())
              strm << ';';
            strm << name << '=' << value;
          }
        }
      }
    }
  }

  return strm.IsEmpty() ? m_fmtp : strm;
}


void SDPMediaFormat::PrintOn(ostream & strm) const
{
  if (!PAssert(!m_encodingName.IsEmpty(), "SDPMediaFormat encoding name is empty"))
    return;

  /* Even though officially, case is not significant for SDP encoding
     types, we make it upper case anyway as this seems to be the custom,
     and some very stupid endpoints assume it is always the case. */
  strm << "a=rtpmap:" << (int)m_payloadType << ' ' << m_encodingName << '/' << m_clockRate;
  if (!m_parameters.IsEmpty())
    strm << '/' << m_parameters;
  strm << CRLF;

  PString fmtpString = GetFMTP();
  if (!fmtpString.IsEmpty())
    strm << "a=fmtp:" << (int)m_payloadType << ' ' << fmtpString << CRLF;
}


void SDPMediaFormat::SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const
{
  mediaFormat.MakeUnique();
  mediaFormat.SetPayloadType(m_payloadType);
  mediaFormat.SetOptionInteger(OpalAudioFormat::ChannelsOption(), m_parameters.IsEmpty() ? 1 : m_parameters.AsUnsigned());

  // Fill in the default values for (possibly) missing FMTP options
  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    OpalMediaOption & option = const_cast<OpalMediaOption &>(mediaFormat.GetOption(i));
    PString value = option.GetFMTPDefault();
    if (!value.IsEmpty())
      option.FromString(value);
  }

  // If format has an explicit FMTP option then we only use that, no high level parsing
  if (mediaFormat.SetOptionString("FMTP", m_fmtp))
    return;

  // No FMTP to parse, may as well stop here
  if (m_fmtp.IsEmpty())
    return;

  // Look for a specific option with "FMTP" as it's name, it is the whole thing then
  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    if (option.GetFMTPName() == "FMTP") {
      mediaFormat.SetOptionValue(option.GetName(), m_fmtp);
      return;
    }
  }

  // Save the raw 'fmtp=' line so it is available for information purposes.
  mediaFormat.AddOption(new OpalMediaOptionString("RawFMTP", false, m_fmtp), true);

  // guess at the seperator
  char sep = ';';
  if (m_fmtp.Find(sep) == P_MAX_INDEX)
    sep = ' ';

  PINDEX sep1next;

  // Parse the string for option names and values OPT=VAL;OPT=VAL
  for (PINDEX sep1prev = 0; sep1prev != P_MAX_INDEX; sep1prev = sep1next+1) {
    // find the next separator (' ' or ';')
    sep1next = m_fmtp.Find(sep, sep1prev);
    if (sep1next == P_MAX_INDEX)
      sep1next--; // Implicit assumption string is not a couple of gigabytes long ...

    // find the next '='. If past the next separator, then ignore it
    PINDEX sep2pos = m_fmtp.Find('=', sep1prev);
    if (sep2pos > sep1next)
      sep2pos = sep1next;

    PCaselessString key = m_fmtp(sep1prev, sep2pos-1).Trim();
    if (key.IsEmpty()) {
      PTRACE(2, "SDP\tBadly formed FMTP parameter \"" << m_fmtp << '"');
      continue;
    }

    OpalMediaOption * option = mediaFormat.FindOption(key);
    if (option == NULL || key != option->GetFMTPName()) {
      for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
        if (key == mediaFormat.GetOption(i).GetFMTPName()) {
          option = const_cast<OpalMediaOption *>(&mediaFormat.GetOption(i));
          break;
        }
      }
    }
    if (option == NULL) {
      PTRACE(2, "SDP\tUnknown FMTP parameter \"" << key << '"');
      continue;
    }

    PString value = m_fmtp(sep2pos+1, sep1next-1);

    if (dynamic_cast< OpalMediaOptionOctets * >(option) != NULL) {
      if (m_fmtp.GetLength() % 2 != 0)
        value = value.Trim();
    }
    else {
      // for non-octet string parameters, check for mixed separators
      PINDEX brokenSep = m_fmtp.Find(' ', sep2pos);
      if (brokenSep < sep1next) {
        sep1next = brokenSep;
        value = m_fmtp(sep2pos+1, sep1next-1);
      }
      value = value.Trim();
      if (value.IsEmpty())
        value = "1"; // Assume it is a boolean
    }

    if (dynamic_cast< OpalMediaOptionString * >(option) != NULL) {
      PString previous = option->AsString();
      if (!previous.IsEmpty())
        value = previous + ';' + value;
    }

    if (!option->FromString(value)) {
      PTRACE(2, "SDP\tCould not set FMTP parameter \"" << key << "\" to value \"" << value << '"');
    }
  }
}


bool SDPMediaFormat::PreEncode()
{
  m_mediaFormat.SetOptionString(OpalMediaFormat::ProtocolOption(), PLUGINCODEC_OPTION_PROTOCOL_SIP);
  return m_mediaFormat.ToCustomisedOptions();
}


bool SDPMediaFormat::PostDecode(const OpalMediaFormatList & mediaFormats, unsigned bandwidth)
{
  // try to init encodingName from global list, to avoid PAssert when media has no rtpmap
  if (m_encodingName.IsEmpty())
    m_encodingName = m_mediaFormat.GetEncodingName();

  if (m_mediaFormat.IsEmpty()) {
    PTRACE(5, "SDP\tMatching \"" << m_encodingName << "\", pt=" << m_payloadType << ", clock=" << m_clockRate);
    for (OpalMediaFormatList::const_iterator iterFormat = mediaFormats.FindFormat(m_payloadType, m_clockRate, m_encodingName, "sip");
         iterFormat != mediaFormats.end();
         iterFormat = mediaFormats.FindFormat(m_payloadType, m_clockRate, m_encodingName, "sip", iterFormat)) {
      OpalMediaFormat adjustedFormat = *iterFormat;
      SetMediaFormatOptions(adjustedFormat);
      // skip formats whose fmtp don't match options
      if (iterFormat->ValidateMerge(adjustedFormat)) {
        PTRACE(3, "SDP\tMatched \"" << m_encodingName << "\", pt=" << m_payloadType << ", clock=" << m_clockRate << " to " << *iterFormat);
        m_mediaFormat = adjustedFormat;
        break;
      }

      PTRACE(4, "SDP\tDid not match\"" << m_encodingName << "\", pt=" << m_payloadType << ", clock=" << m_clockRate << " to " << *iterFormat);
    }

    if (m_mediaFormat.IsEmpty()) {
      PTRACE(2, "SDP\tCould not find media format for \"" << m_encodingName << "\", pt=" << m_payloadType << ", clock=" << m_clockRate);
      return false;
    }
  }

  SetMediaFormatOptions(m_mediaFormat);

  /* Set bandwidth limits, if present, e.g. "SDP-Bandwidth-AS" and "SDP-Bandwidth-TIAS".

     Some codecs (e.g. MPEG4, H.264) have difficulties with controlling max
     bit rate. This is due to the very poor support of TIAS by SIP clients.
     So, if we HAVE got a TIAS we indicate it with a special media option
     so the codec can then trust the "MaxBitRate" option. If this is not
     present then the codec has to play games with downgrading profile
     levels to assure that a max bit rate is not exceeded. */
  for (SDPBandwidth::const_iterator r = m_parent.GetBandwidth().begin(); r != m_parent.GetBandwidth().end(); ++r) {
    if (r->second > 0)
      m_mediaFormat.AddOption(new OpalMediaOptionValue<OpalBandwidth>(SDPBandwidthPrefix + r->first,
                                                  false, OpalMediaOption::MinMerge, r->second), true);
  }

  if (bandwidth > 1000 && bandwidth < (unsigned)m_mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption())) {
    PTRACE(4, "SDP\tAdjusting format \"" << m_mediaFormat << "\" bandwidth to " << bandwidth);
    m_mediaFormat.SetOptionInteger(OpalMediaFormat::MaxBitRateOption(), bandwidth);
  }

  m_mediaFormat.SetOptionString(OpalMediaFormat::ProtocolOption(), PLUGINCODEC_OPTION_PROTOCOL_SIP);
  if (m_mediaFormat.ToNormalisedOptions())
    return true;

  PTRACE(2, "SDP\tCould not normalise format \"" << m_encodingName << "\", pt=" << m_payloadType << ", removing.");
  return false;
}


//////////////////////////////////////////////////////////////////////////////

OpalBandwidth & SDPBandwidth::operator[](const PCaselessString & type)
{
  return BaseClass::operator[](type);
}


OpalBandwidth SDPBandwidth::operator[](const PCaselessString & type) const
{
  const_iterator it = find(type);
  return it != end() ? it->second : OpalBandwidth::Max();
}


ostream & operator<<(ostream & out, const SDPBandwidth & bw)
{
  for (SDPBandwidth::const_iterator iter = bw.begin(); iter != bw.end(); ++iter)
    out << "b=" << iter->first << ':' << (unsigned)iter->second << CRLF;
  return out;
}


bool SDPBandwidth::Parse(const PString & param)
{
  PINDEX pos = param.FindSpan(TokenChars); // Legal chars from RFC
  if (pos == P_MAX_INDEX || param[pos] != ':') {
    PTRACE(2, "SDP\tMalformed bandwidth attribute " << param);
    return false;
  }

  (*this)[param.Left(pos)] = param.Mid(pos+1).AsUnsigned();
  return true;
}


void SDPBandwidth::SetMax(const PCaselessString & type, OpalBandwidth value)
{
  iterator it = find(type);
  if (it == end())
    (*this)[type] = value;
  else if (it->second < value)
    it->second = value;
}


//////////////////////////////////////////////////////////////////////////////

const PCaselessString & SDPCommonAttributes::ConferenceTotalBandwidthType()      { static const PConstCaselessString s("CT");   return s; }
const PCaselessString & SDPCommonAttributes::ApplicationSpecificBandwidthType()  { static const PConstCaselessString s("AS");   return s; }
const PCaselessString & SDPCommonAttributes::TransportIndependentBandwidthType() { static const PConstCaselessString s("TIAS"); return s; }

void SDPCommonAttributes::ParseAttribute(const PString & value)
{
  PINDEX pos = value.FindSpan(TokenChars); // Legal chars from RFC
  PCaselessString attr(value.Left(pos));
  if (pos == P_MAX_INDEX)
    SetAttribute(attr, "1");
  else if (value[pos] == ':')
    SetAttribute(attr, value.Mid(pos + 1));
  else {
    PTRACE(2, "SDP\tMalformed media attribute " << value);
  }
}


void SDPCommonAttributes::SetAttribute(const PString & attr, const PString & value)
{
  // get the attribute type
  if (attr *= "sendonly") {
    m_direction = SendOnly;
    return;
  }

  if (attr *= "recvonly") {
    m_direction = RecvOnly;
    return;
  }

  if (attr *= "sendrecv") {
    m_direction = SendRecv;
    return;
  }

  if (attr *= "inactive") {
    m_direction = Inactive;
    return;
  }

  if (attr *= "extmap") {
    RTPExtensionHeaderInfo ext;
    if (ext.ParseSDP(value))
      SetExtensionHeader(ext);
    return;
  }

  if (attr *= "ice-options") {
    PStringArray tokens = value.Tokenise(WhiteSpace, false); // Spec says space only, but lets be forgiving
    for (PINDEX i = 0; i < tokens.GetSize(); ++i)
      m_iceOptions += tokens[i];
    return;
  }

  // unknown attributes
  PTRACE(2, "SDP\tUnknown attribute " << attr);
}


void SDPCommonAttributes::OutputAttributes(ostream & strm) const
{
  // media format direction
  switch (m_direction) {
    case SDPMediaDescription::RecvOnly:
      strm << "a=recvonly\r\n";
      break;
    case SDPMediaDescription::SendOnly:
      strm << "a=sendonly\r\n";
      break;
    case SDPMediaDescription::SendRecv:
      strm << "a=sendrecv\r\n";
      break;
    case SDPMediaDescription::Inactive:
      strm << "a=inactive\r\n";
      break;
    default:
      break;
  }

  for (RTPExtensionHeaders::const_iterator it = m_extensionHeaders.begin(); it != m_extensionHeaders.end(); ++it)
    it->OutputSDP(strm);

  const char * crlf = "";
  for (PStringSet::const_iterator it = m_iceOptions.begin(); it != m_iceOptions.end(); ++it) {
    if (*it == IceLiteOption)
      continue;
    if (it != m_iceOptions.begin())
      strm << ' ';
    else {
      strm << "a=ice-options:";
      crlf = CRLF;
    }
    strm << *it;
  }
  strm << crlf;
}


//////////////////////////////////////////////////////////////////////////////

SDPMediaDescription::SDPMediaDescription()
  : m_port(0)
  , m_portCount(1)
{
}


SDPMediaDescription::SDPMediaDescription(const OpalTransportAddress & address, const OpalMediaType & type)
  : m_mediaAddress(address)
  , m_port(0)
  , m_portCount(1)
  , m_mediaType(type)
{
  PIPSocket::Address ip;
  if (m_mediaAddress.GetIpAndPort(ip, m_port))
    m_controlAddress = OpalTransportAddress(ip, m_port+1, OpalTransportAddress::UdpPrefix());
}


static void CalculateCandidatePriority(PNatCandidate & candidate)
{
  candidate.m_priority = (126 << 24) | (256 - candidate.m_component);

  PIPSocket::Address ip = candidate.m_localTransportAddress.GetAddress();
  if (ip.GetVersion() != 6)
    candidate.m_priority |= 0xffff00;
  else {
    /* Incomplete need to get precedence from following table, for now use 50
          Prefix        Precedence Label
          ::1/128               50     0
          ::/0                  40     1
          2002::/16             30     2
          ::/96                 20     3
          ::ffff:0:0/96         10     4
    */
    candidate.m_priority |= 50 << 8;
  }
}


bool SDPMediaDescription::SetSessionInfo(const OpalMediaSession * session, bool ice)
{
  if (session == NULL) {
    m_port = 0;
    m_mediaAddress = OpalTransportAddress();
    m_controlAddress = OpalTransportAddress();
    return true;
  }

  m_mediaAddress = session->GetLocalAddress(true);
  m_controlAddress = session->GetLocalAddress(false);

  PIPSocket::Address dummy;
  if (!m_mediaAddress.GetIpAndPort(dummy, m_port))
    return false;

  if (ice) {
    PNatCandidateList candidates;
    PNatCandidate candidate(PNatCandidate::HostType, PNatMethod::eComponent_RTP, "xyzzy");
    if (m_mediaAddress.GetIpAndPort(candidate.m_localTransportAddress)) {
      CalculateCandidatePriority(candidate);
      candidates.Append(new PNatCandidate(candidate));

      if (m_controlAddress.GetIpAndPort(candidate.m_localTransportAddress)) {
        candidate.m_component = PNatMethod::eComponent_RTCP;
        CalculateCandidatePriority(candidate);
        candidates.Append(new PNatCandidate(candidate));
      }

      SetICE(session->GetLocalUsername(), session->GetLocalPassword(), candidates);
    }
  }

  return true;
}


bool SDPMediaDescription::Decode(const PStringArray & tokens)
{
  if (tokens.GetSize() < 3) {
    PTRACE(1, "SDP\tUnknown SDP media type " << tokens[0]);
    return false;
  }

  // parse the port and port count
  PString portStr = tokens[1];
  PINDEX pos = portStr.Find('/');
  if (pos == P_MAX_INDEX)
    m_portCount = 1;
  else {
    PTRACE(3, "SDP\tMedia header contains port count - " << portStr);
    m_portCount = (WORD)portStr.Mid(pos+1).AsUnsigned();
    portStr = portStr.Left(pos);
  }
  m_port = (WORD)portStr.AsUnsigned();

  // parse the transport
  m_transportType = tokens[2];

  // check everything
  switch (m_port) {
    case 0 :
      PTRACE(3, "SDP\tIgnoring media session " << m_mediaType << " with port=0");
      m_direction = Inactive;
      m_mediaAddress = m_controlAddress = PString::Empty();
      break;

    case 65535 :
      PTRACE(2, "SDP\tIllegal port=65535 in media session " << m_mediaType << ", trying to continue.");
      m_port = 65534;
      // Do next case

    default :
      PIPSocket::Address ip;
      if (m_mediaAddress.GetIpAddress(ip)) {
        m_mediaAddress = OpalTransportAddress(ip, m_port, OpalTransportAddress::UdpPrefix());
        PTRACE(4, "SDP\tSetting media connection address " << m_mediaAddress);
      }
      else {
        PTRACE(4, "SDP\tMedia session port=" << m_port);
      }
  }

  CreateSDPMediaFormats(tokens);

  return true;
}


void SDPMediaDescription::CreateSDPMediaFormats(const PStringArray & tokens)
{
  // create the format list
  for (PINDEX i = 3; i < tokens.GetSize(); i++) {
    SDPMediaFormat * fmt = CreateSDPMediaFormat();
    if (fmt != NULL) {
      if (fmt->Initialise(tokens[i]))
        m_formats.Append(fmt);
      else
        delete fmt;
    }
    else {
      PTRACE(2, "SDP\tCannot create SDP media format for port " << tokens[i]);
    }
  }
}


bool SDPMediaDescription::Decode(char key, const PString & value)
{
  /* NOTE: must make sure anything passed through to a SDPFormat isntance does
           not require it to have an OpalMediaFormat yet, as that is not created
           until PostDecode() */
  switch (key) {
    case 'i' : // media title
    case 'k' : // encryption key
      break;

    case 'b' : // bandwidth information
      m_bandwidth.Parse(value);
      break;

    case 'c' : // connection information - optional if included at session-level
      if (m_port != 0) {
        m_mediaAddress = ParseConnectAddress(value, m_port);
        PTRACE_IF(4, !m_mediaAddress.IsEmpty(), "SDP\tParsed media connection address " << m_mediaAddress);
      }
      break;

    case 'a' : // zero or more media attribute lines
      ParseAttribute(value);
      break;

    default:
      PTRACE(1, "SDP\tUnknown media information key " << key);
  }

  return true;
}


bool SDPMediaDescription::PostDecode(const OpalMediaFormatList & mediaFormats)
{
  unsigned bw = GetBandwidth(SDPSessionDescription::TransportIndependentBandwidthType());
  if (bw == UINT_MAX) {
    bw = GetBandwidth(SDPSessionDescription::ApplicationSpecificBandwidthType());
    if (bw < UINT_MAX/1000)
      bw *= 1000;
  }

  SDPMediaFormatList::iterator format = m_formats.begin();
  while (format != m_formats.end()) {
    if (format->PostDecode(mediaFormats, bw))
      ++format;
    else
      m_formats.erase(format++);
  }

  return true;
}


#if OPAL_SRTP

void SDPMediaDescription::SetCryptoKeys(OpalMediaCryptoKeyList &)
{
  // Do nothing
}


OpalMediaCryptoKeyList SDPMediaDescription::GetCryptoKeys() const
{
  return OpalMediaCryptoKeyList();
}


bool SDPMediaDescription::HasCryptoKeys() const
{
  return false;
}

#endif // OPAL_SRTP


void SDPMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  /* NOTE: must make sure anything passed through to a SDPFormat instance does
           not require it to have an OpalMediaFormat yet, as that is not created
           until PostDecode() */

  // handle fmtp attributes
  if (attr *= "fmtp") {
    PString params = value;
    SDPMediaFormat * format = FindFormat(params);
    if (format != NULL)
      format->SetFMTP(params);
    return;
  }

  if (attr *= "mid") {
    m_mediaGroupId = value;
    return;
  }

  if (attr *= "ice-ufrag") {
    m_username = value;
    return;
  }

  if (attr *= "ice-pwd") {
    m_password = value;
    return;
  }

  if (attr *= "candidate") {
    PStringArray words = value.Tokenise(WhiteSpace, false); // Spec says space only, but lets be forgiving
    if (words.GetSize() < 8) {
      PTRACE(2, "SDP\tNot enough parameters in candidate: \"" << value << '"');
      return;
    }

    PNatCandidate candidate;
    candidate.m_foundation = words[0];
    candidate.m_component = (PNatMethod::Component)words[1].AsUnsigned();
    candidate.m_protocol = words[2];
    candidate.m_priority = words[3].AsUnsigned();

    PIPSocket::Address ip(words[4]);
    if (!ip.IsValid()) {
      PTRACE(2, "SDP\tIllegal IP address in candidate: \"" << words[4] << '"');
      return;
    }

    candidate.m_localTransportAddress.SetAddress(ip, (WORD)words[5].AsUnsigned());
    if (words[6] *= "typ") {
      for (candidate.m_type = PNatCandidate::BeginTypes; candidate.m_type < PNatCandidate::EndTypes; ++candidate.m_type) {
        if (words[7] *= CandidateTypeNames[candidate.m_type])
          break;
      }
      PTRACE_IF(3, candidate.m_type == PNatCandidate::EndTypes, "SDP\tUnknown candidate type: \"" << words[7] << '"');
    }
    else {
      PTRACE(2, "SDP\tMissing candidate type: \"" << words[6] << '"');
      return;
    }

    m_candidates.Append(new PNatCandidate(candidate));
    return;
  }

  SDPCommonAttributes::SetAttribute(attr, value);
}


SDPMediaFormat * SDPMediaDescription::FindFormat(PString & params) const
{
  SDPMediaFormatList::const_iterator format;

  // extract the RTP payload type
  PINDEX pos = params.FindSpan("0123456789");
  if (pos == P_MAX_INDEX || isspace(params[pos])) {

    // find the format that matches the payload type
    RTP_DataFrame::PayloadTypes pt = (RTP_DataFrame::PayloadTypes)params.Left(pos).AsUnsigned();
    for (format = m_formats.begin(); format != m_formats.end(); ++format) {
      if (format->GetPayloadType() == pt)
        break;
    }
  }
  else {
    // Check for it a format name
    pos = params.Find(' ');
    PString encodingName = params.Left(pos);
    for (format = m_formats.begin(); format != m_formats.end(); ++format) {
      if (format->GetEncodingName() == encodingName)
        break;
    }
  }

  if (format == m_formats.end()) {
    PTRACE(2, "SDP\tMedia attribute found for unknown RTP type/name " << params.Left(pos));
    return NULL;
  }

  // extract the attribute argument
  if (pos != P_MAX_INDEX) {
    while (isspace(params[pos]))
      pos++;
    params.Delete(0, pos);
  }

  return const_cast<SDPMediaFormat *>(&*format);
}


bool SDPMediaDescription::PreEncode()
{
  for (SDPMediaFormatList::iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
    if (!format->PreEncode())
      return false;
  }
  return true;
}


void SDPMediaDescription::Encode(const OpalTransportAddress & commonAddr, ostream & strm) const
{
  /* output media header, note the order is important according to RFC!
     Must be icbka */
  strm << "m="
       << GetSDPMediaType() << ' '
       << m_port;
  if (m_portCount > 1)
    strm << '/' << m_portCount;

  strm << ' ' << GetSDPTransportType();

  PString portList = GetSDPPortList();
  if (portList.IsEmpty())
    strm << " *";
  else if (portList[0] == ' ')
    strm << portList;
  else
    strm << ' ' << portList;
  strm << CRLF;

  // If we have a port of zero, then shutting down SDP stream. No need for anything more
  if (m_port == 0)
    return;

  PIPSocket::Address commonIP, transportIP;
  if (m_mediaAddress.GetIpAddress(transportIP) && commonAddr.GetIpAddress(commonIP) && commonIP != transportIP)
    strm << "c=" << GetConnectAddressString(m_mediaAddress) << CRLF;

  strm << m_bandwidth;
  OutputAttributes(strm);
}


static bool CanUseCandidate(const PNatCandidate & candidate)
{
  return !candidate.m_foundation.IsEmpty() &&
          candidate.m_component > 0 &&
         !candidate.m_protocol.IsEmpty() &&
          candidate.m_priority > 0 &&
          candidate.m_localTransportAddress.IsValid() &&
          candidate.m_type < PNatCandidate::NumTypes;
}

void SDPMediaDescription::OutputAttributes(ostream & strm) const
{
  SDPCommonAttributes::OutputAttributes(strm);

  if (!m_mediaGroupId.IsEmpty())
    strm << "a=mid:" << m_mediaGroupId << CRLF;

  if (m_username.IsEmpty() || m_password.IsEmpty())
    return;

  bool haveCandidate = false;
  for (PNatCandidateList::const_iterator it = m_candidates.begin(); it != m_candidates.end(); ++it) {
    if (CanUseCandidate(*it)) {
      strm << "a=candidate:"
           << it->m_foundation << ' '
           << it->m_component << ' '
           << it->m_protocol << ' '
           << it->m_priority << ' '
           << it->m_localTransportAddress.GetAddress() << ' '
           << it->m_localTransportAddress.GetPort()
           << " typ " << CandidateTypeNames[it->m_type]
           << CRLF;
      haveCandidate = true;
    }
  }

  if (haveCandidate)
    strm << "a=ice-ufrag:" << m_username << CRLF
         << "a=ice-pwd:" << m_password << CRLF;
}


bool SDPMediaDescription::HasICE() const
{
  if (m_username.IsEmpty())
    return false;
  
  if (m_password.IsEmpty())
    return false;

  if (m_candidates.IsEmpty())
    return false;

  for (PNatCandidateList::const_iterator it = m_candidates.begin(); it != m_candidates.end(); ++it) {
    if (CanUseCandidate(*it))
      return true;
  }

  return false;
}

PString SDPMediaDescription::GetSDPMediaType() const
{
  return m_mediaType->GetSDPMediaType();
}


PCaselessString SDPMediaDescription::GetSDPTransportType() const
{
  return m_mediaType->GetSDPTransportType();
}


PCaselessString SDPMediaDescription::GetSessionType() const
{
  return GetSDPTransportType();
}


const SDPMediaFormatList & SDPMediaDescription::GetSDPMediaFormats() const
{
  return m_formats;
}


OpalMediaFormatList SDPMediaDescription::GetMediaFormats() const
{
  OpalMediaFormatList list;

  if (m_port != 0) {
    for (SDPMediaFormatList::const_iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
      OpalMediaFormat mediaFormat = format->GetMediaFormat();
      if (mediaFormat.IsValid())
        list += mediaFormat;
      else {
        PTRACE(2, "SDP\tRTP payload type " << format->GetPayloadType()
               << ", name=" << format->GetEncodingName() << ", not matched to supported codecs");
      }
    }
  }

  return list;
}


void SDPMediaDescription::AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat)
{
  if (sdpMediaFormat != NULL)
    m_formats.Append(sdpMediaFormat);
}


void SDPMediaDescription::AddMediaFormat(const OpalMediaFormat & mediaFormat)
{
  if (!mediaFormat.IsTransportable() || !mediaFormat.IsValidForProtocol("sip")) {
    PTRACE(4, "SDP\tSDP not including " << mediaFormat << " as it is not a SIP transportable format");
    return;
  }

  RTP_DataFrame::PayloadTypes payloadType = mediaFormat.GetPayloadType();
  const char * encodingName = mediaFormat.GetEncodingName();
  unsigned clockRate = mediaFormat.GetClockRate();

  for (SDPMediaFormatList::iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
    const OpalMediaFormat & sdpMediaFormat = format->GetMediaFormat();
    if (mediaFormat == sdpMediaFormat) {
      PTRACE(2, "SDP\tSDP not including " << mediaFormat << " as already included");
      return;
    }

    if (format->GetPayloadType() == payloadType) {
      PTRACE(2, "SDP\tSDP not including " << mediaFormat << " as it is has duplicate payload type " << payloadType);
      return;
    }

    if (format->GetEncodingName() == encodingName &&
        format->GetClockRate() == clockRate &&
        mediaFormat.ValidateMerge(sdpMediaFormat)) {
      PTRACE(2, "SDP\tSDP not including " << mediaFormat
             << " as an equivalent (" << sdpMediaFormat << ") is already included");
      return;
    }
  }

  SDPMediaFormat * sdpFormat = CreateSDPMediaFormat();
  sdpFormat->Initialise(mediaFormat);
  AddSDPMediaFormat(sdpFormat);
}


void SDPMediaDescription::ProcessMediaOptions(SDPMediaFormat & /*sdpFormat*/, const OpalMediaFormat & /*mediaFormat*/)
{
}


void SDPMediaDescription::AddMediaFormats(const OpalMediaFormatList & mediaFormats, const OpalMediaType & mediaType)
{
  for (OpalMediaFormatList::const_iterator format = mediaFormats.begin(); format != mediaFormats.end(); ++format) {
    if (format->GetMediaType() == mediaType && (format->IsTransportable()))
      AddMediaFormat(*format);
  }
}


PString SDPMediaDescription::GetSDPPortList() const
{
  PStringStream strm;

  // output encoding names for non RTP
  SDPMediaFormatList::const_iterator format;
  for (format = m_formats.begin(); format != m_formats.end(); ++format)
    strm << ' ' << format->GetEncodingName();

  return strm;
}


void SDPMediaDescription::Copy(SDPMediaDescription & desc)
{
  SDPMediaFormatList::iterator format;
  for (format = desc.m_formats.begin(); format != desc.m_formats.end(); ++format)
    AddSDPMediaFormat(format->CloneAs<SDPMediaFormat>());
}


//////////////////////////////////////////////////////////////////////////////

SDPDummyMediaDescription::SDPDummyMediaDescription(const OpalTransportAddress & address, const PStringArray & tokens)
  : SDPMediaDescription(address, "")
  , m_tokens(tokens)
{
  switch (m_tokens.GetSize()) {
    case 0 :
      m_tokens.AppendString("unknown");
    case 1 :
      m_tokens.AppendString("0");
    case 2 :
      m_tokens.AppendString("unknown");
    case 3 :
      m_tokens.AppendString("127");
  }

  m_transportType = m_tokens[2];
}


PString SDPDummyMediaDescription::GetSDPMediaType() const
{
  return m_tokens[0];
}


PCaselessString SDPDummyMediaDescription::GetSDPTransportType() const
{
  return m_tokens[2];
}


PCaselessString SDPDummyMediaDescription::GetSessionType() const
{
  return OpalDummySession::SessionType();
}


SDPMediaFormat * SDPDummyMediaDescription::CreateSDPMediaFormat()
{
  return NULL;
}


PString SDPDummyMediaDescription::GetSDPPortList() const
{
  return m_tokens[3];
}


void SDPDummyMediaDescription::Copy(SDPMediaDescription & from)
{
  m_tokens.SetSize(4);
  m_tokens[0] = from.GetSDPMediaType();
  m_tokens[1] = '0';
  m_tokens[2] = m_transportType = from.GetSDPTransportType();
  m_tokens[3] = from.GetSDPPortList();

  m_port = 0;
}


//////////////////////////////////////////////////////////////////////////////

#if OPAL_SRTP

SDPCryptoSuite::SDPCryptoSuite(unsigned tag)
  : m_tag(tag)
{
  if (m_tag == 0)
    m_tag = 1;
}


bool SDPCryptoSuite::Decode(const PString & sdp)
{
  if (sdp.GetLength() < 7 || sdp[0] < '1' || sdp[0] > '9' || sdp[1] != ' ')
    return false;

  m_tag = sdp[0] - '0';

  PINDEX space = sdp.Find(' ', 2);
  if (space == P_MAX_INDEX)
    return false;

  m_suiteName = sdp(2, space-1);

  PINDEX sessionParamsPos = sdp.Find(' ', space+1);
  PURL::SplitVars(sdp.Mid(sessionParamsPos+1), m_sessionParams, ' ', '=', PURL::QuotedParameterTranslation);

  PStringArray keyParams = sdp(space+1, sessionParamsPos-1).Tokenise(';');
  for (PINDEX kp =0; kp < keyParams.GetSize(); ++kp) {
    PCaselessString method, info;
    if (!keyParams[kp].Split(':', method, info) || method != "inline" || info.IsEmpty()) {
      PTRACE(2, "SDP", "Unsupported method \"" << method << '"');
      return false;
    }

    PStringArray keyParts = info.Tokenise('|');
    m_keyParams.push_back(KeyParam(keyParts[0]));
    KeyParam & newKeyParam = m_keyParams.back();

    switch (keyParts.GetSize()) {
      case 3 :
        {
          PString idx, len;
          if (!keyParts[2].Split(':', idx, len)) {
            PTRACE(2, "SDP", "Expected colon in mki index/length: \"" << keyParts[2] << '"');
            return false;
          }
          if ((newKeyParam.m_mkiIndex = idx.AsUnsigned()) == 0) {
            PTRACE(2, "SDP", "Must have non-zero mki index");
            return false;
          }
          if ((newKeyParam.m_mkiLength = len.AsUnsigned()) == 0) {
            PTRACE(2, "SDP", "Must have non-zero mki length");
            return false;
          }
        }
        // Do next case

      case 2 :
        {
          PCaselessString lifetime = keyParts[1];
          if (lifetime.NumCompare("2^") == PObject::EqualTo)
            newKeyParam.m_lifetime = 1ULL << lifetime.Mid(2).AsUnsigned();
          else
            newKeyParam.m_lifetime = lifetime.AsUnsigned64();
          if (newKeyParam.m_lifetime == 0) {
            PTRACE(2, "SDP", "Must have non-zero lifetime");
            return false;
          }
        }
    }
  }

  return !m_keyParams.empty();
}


bool SDPCryptoSuite::SetKeyInfo(const OpalMediaCryptoKeyInfo & keyInfo)
{
  PString str = keyInfo.ToString();
  if (str.IsEmpty())
    return false;

  m_suiteName = keyInfo.GetCryptoSuite().GetFactoryName();

  m_keyParams.clear();
  m_keyParams.push_back(KeyParam(str));
  return true;
}


OpalMediaCryptoKeyInfo * SDPCryptoSuite::GetKeyInfo() const
{
  OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(m_suiteName);
  if (cryptoSuite == NULL) {
    PTRACE(2, "SDP\tUnknown crypto suite \"" << m_suiteName << '"');
    return NULL;
  }

  OpalMediaCryptoKeyInfo * keyInfo = cryptoSuite->CreateKeyInfo();
  if (PAssertNULL(keyInfo) == NULL)
    return NULL;

  keyInfo->SetTag(m_tag);

  if (keyInfo->FromString(m_keyParams.front().m_keySalt))
    return keyInfo;

  delete keyInfo;
  return NULL;
}


void SDPCryptoSuite::PrintOn(ostream & strm) const
{
  strm << "a=crypto:" << m_tag << ' ' << m_suiteName << " inline:";
  for (list<KeyParam>::const_iterator it = m_keyParams.begin(); it != m_keyParams.end(); ++it) {
    strm << it->m_keySalt;
    if (it->m_lifetime > 0) {
      strm << '|';
      unsigned bit = 1;
      for (bit = 48; bit > 0; --bit) {
        if ((1ULL<<bit) == it->m_lifetime) {
          strm << "2^" << bit;
          break;
        }
      }
      if (bit == 0)
        strm << it->m_lifetime;

      if (it->m_mkiIndex > 0)
        strm << '|' << it->m_mkiIndex << ':' << it->m_mkiLength;
    }
  }

  for (PStringOptions::const_iterator it = m_sessionParams.begin(); it != m_sessionParams.end(); ++it) {
    strm << ' ' << it->first;
    if (!it->second.IsEmpty())
      strm << '=' << it->second;
  }

  strm << CRLF;
}

#endif // OPAL_SRTP


//////////////////////////////////////////////////////////////////////////////

SDPRTPAVPMediaDescription::SDPRTPAVPMediaDescription(const OpalTransportAddress & address, const OpalMediaType & mediaType)
  : SDPMediaDescription(address, mediaType)
  , m_enableFeedback(false)
{
}


bool SDPRTPAVPMediaDescription::Decode(const PStringArray & tokens)
{
  if (!SDPMediaDescription::Decode(tokens))
    return false;

  PIPSocket::Address ip;
  if (m_mediaAddress.GetIpAddress(ip)) {
    m_controlAddress = OpalTransportAddress(ip, m_port+1, OpalTransportAddress::UdpPrefix());
    PTRACE(4, "SDP\tSetting rtcp connection address " << m_controlAddress);
  }

  m_enableFeedback = m_transportType.Find("AVPF") != P_MAX_INDEX;
  return true;
}


PCaselessString SDPRTPAVPMediaDescription::GetSDPTransportType() const
{
#if OPAL_SRTP
  if (!m_cryptoSuites.IsEmpty())
    return m_enableFeedback ? OpalSRTPSession::RTP_SAVPF() : OpalSRTPSession::RTP_SAVP();
#endif
  return m_enableFeedback ? OpalRTPSession::RTP_AVPF() : OpalRTPSession::RTP_AVP();
}


bool SDPRTPAVPMediaDescription::Format::Initialise(const PString & portString)
{
  int pt = portString.AsInteger();
  if (pt < 0 || pt > 127)
    return false;

  m_payloadType = (RTP_DataFrame::PayloadTypes)pt;
  return true;
}


SDPMediaFormat * SDPRTPAVPMediaDescription::CreateSDPMediaFormat()
{
  return new Format(*this);
}


PString SDPRTPAVPMediaDescription::GetSDPPortList() const
{
  if (m_formats.IsEmpty())
    return "127"; // Have to have SOMETHING

  PStringStream strm;

  // output RTP payload types
  for (SDPMediaFormatList::const_iterator format = m_formats.begin(); format != m_formats.end(); ++format)
    strm << ' ' << (int)format->GetPayloadType();

  return strm;
}


void SDPRTPAVPMediaDescription::OutputAttributes(ostream & strm) const
{
  // call ancestor
  SDPMediaDescription::OutputAttributes(strm);

  // output attributes for each payload type
  for (SDPMediaFormatList::const_iterator format = m_formats.begin(); format != m_formats.end(); ++format)
    strm << *format;

#if OPAL_SRTP
  for (PList<SDPCryptoSuite>::const_iterator crypto = m_cryptoSuites.begin(); crypto != m_cryptoSuites.end(); ++crypto)
    strm << *crypto;
#endif

  if (m_controlAddress == m_mediaAddress)
    strm << "a=rtcp-mux\r\n";
  else if (!m_controlAddress.IsEmpty()) {
    PIPSocket::Address ip;
    WORD port = 0;
    if (m_controlAddress.GetIpAndPort(ip, port) && port != (m_port+1))
      strm << "a=rtcp:" << port << ' ' << GetConnectAddressString(m_mediaAddress) << CRLF;
  }

  for (SsrcInfo::const_iterator it1 = m_ssrcInfo.begin(); it1 != m_ssrcInfo.end(); ++it1) {
    for (PStringOptions::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
      strm << "a=ssrc:" << it1->first << ' ' << it2->first << ':' << it2->second << CRLF;
  }
}


#if OPAL_SRTP

void SDPRTPAVPMediaDescription::SetCryptoKeys(OpalMediaCryptoKeyList & cryptoKeys)
{
  m_cryptoSuites.RemoveAll();

  unsigned nextTag = 1;
  for (OpalMediaCryptoKeyList::iterator it = cryptoKeys.begin(); it != cryptoKeys.end(); ++it) {
    unsigned tag = it->GetTag().AsUnsigned();
    if (tag == 0)
      tag = nextTag;
    SDPCryptoSuite * cryptoSuite = new SDPCryptoSuite(tag);
    if (!cryptoSuite->SetKeyInfo(*it))
      delete cryptoSuite;
    else {
      m_cryptoSuites.Append(cryptoSuite);
      it->SetTag(tag);
      nextTag = tag + 1;
    }
  }
}


OpalMediaCryptoKeyList SDPRTPAVPMediaDescription::GetCryptoKeys() const
{
  OpalMediaCryptoKeyList keys;

  for (PList<SDPCryptoSuite>::const_iterator it = m_cryptoSuites.begin(); it != m_cryptoSuites.end(); ++it)
    keys.Append(it->GetKeyInfo());

  return keys;
}


bool SDPRTPAVPMediaDescription::HasCryptoKeys() const
{
  return !m_cryptoSuites.IsEmpty();
}

#endif // OPAL_SRTP


void SDPRTPAVPMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  /* NOTE: must make sure anything passed through to a SDPFormat isntance does
           not require it to have an OpalMediaFormat yet, as that is not created
           until PostDecode() */

  if (attr *= "rtpmap") {
    PString params = value;
    SDPMediaFormat * format = FindFormat(params);
    if (format == NULL)
      return;

    PStringArray tokens = params.Tokenise('/');
    if (tokens.GetSize() < 2) {
      PTRACE(2, "SDP\tMalformed rtpmap attribute for " << format->GetEncodingName());
      return;
    }

    format->SetEncodingName(tokens[0]);
    format->SetClockRate(tokens[1].AsUnsigned());
    if (tokens.GetSize() > 2)
      format->SetParameters(tokens[2]);

    return;
  }

#if OPAL_SRTP
  if (attr *= "crypto") {
    SDPCryptoSuite * cryptoSuite = new SDPCryptoSuite(0);
    if (cryptoSuite->Decode(value)) {
      m_cryptoSuites.Append(cryptoSuite);
      if (m_transportType == OpalRTPSession::RTP_AVP())
        m_transportType = OpalSRTPSession::RTP_SAVP();
    }
    else {
      delete cryptoSuite;
      PTRACE(2, "SDP\tCannot decode SDP crypto attribute: \"" << value << '"');
    }
    return;
  }
#endif // OPAL_SRTP

  if (attr *= "rtcp-mux") {
    m_controlAddress = m_mediaAddress;
    return;
  }

  if (attr *= "rtcp") {
    m_controlAddress = ParseConnectAddress(value.Mid(value.Find(' ')+1), (WORD)value.AsUnsigned());
    PTRACE_IF(4, !m_controlAddress.IsEmpty(), "SDP\tParsed rtcp connection address " << m_controlAddress);
    return;
  }

  if (attr *= "ssrc") {
    DWORD ssrc = value.AsUnsigned();
    PINDEX space = value.Find(' ');
    PINDEX endToken = value.FindSpan(TokenChars, space+1);
    if (ssrc == 0 || space == P_MAX_INDEX || (endToken != P_MAX_INDEX && value[endToken] != ':')) {
      PTRACE(2, "SDP\tCannot decode ssrc attribute: \"" << value << '"');
    }
    else {
      PCaselessString key = value(space + 1, endToken - 1);
      PString val = value.Mid(endToken + 1);
      m_ssrcInfo[ssrc].SetAt(key, val);
      PTRACE_IF(4, key == "cname", "SDP\tSSRC: " << RTP_TRACE_SRC(ssrc) << " CNAME: " << val);
    }
    return;
  }

  SDPMediaDescription::SetAttribute(attr, value);
}


bool SDPRTPAVPMediaDescription::SetSessionInfo(const OpalMediaSession * session, bool ice)
{
  const OpalRTPSession * rtpSession = dynamic_cast<const OpalRTPSession *>(session);
  if (rtpSession != NULL) {
    DWORD ssrc = rtpSession->GetSyncSourceOut();
    PStringOptions & info = m_ssrcInfo[ssrc];
    info.SetAt("cname", rtpSession->GetCanonicalName());
    PString mslabel = rtpSession->GetGroupId();
    if (!mslabel.IsEmpty()) {
      PString label = mslabel + '+' + session->GetMediaType();
      info.SetAt("mslabel", mslabel);
      info.SetAt("label", label);
      info.SetAt("msid", mslabel & label);

      // Probably should be arbitrary string, but everyone seems to use
      // the media type, as in "audio" and "video".
      m_mediaGroupId = GetMediaType();
    }
  }

  return SDPMediaDescription::SetSessionInfo(session, ice);
}


//////////////////////////////////////////////////////////////////////////////

SDPAudioMediaDescription::SDPAudioMediaDescription(const OpalTransportAddress & address)
  : SDPRTPAVPMediaDescription(address, OpalMediaType::Audio())
  , m_PTime(0)
  , m_maxPTime(0)
{
}


void SDPAudioMediaDescription::OutputAttributes(ostream & strm) const
{
  // call ancestor
  SDPRTPAVPMediaDescription::OutputAttributes(strm);

  /* The ptime parameter is a recommendation to the remote that we want them
     to send that number of milliseconds of audio in each RTP packet. The
     maxptime parameter specifies the absolute maximum packet size that can
     be handled. There purpose is to limit the size of the packets sent by the
     remote to prevent buffer overflows at this end.

     OPAL does not have a parameter equivalent to ptime. This code uses the
     TxFramesPerPacketOption. We go through all of the codecs offered and
     calculate the largest of them. The value can be controlled by
     manipulating the registered formats set or the set of formats given to
     AddMediaFormat. It would make most sense here if they were all the same.

     The maxptime parameter can be represented by the RxFramesPerPacketOption,
     so we go through all the codecs offered and calculate a maxptime based on
     the smallest maximum rx packets of the codecs.

     Allowance must be made for either value to be at least big enough for 1
     frame per packet for the largest frame size of those codecs. In practice
     this generally means if we mix GSM and G.723.1 then the value cannot be
     smaller than 30ms even if GSM wants one frame per packet. That should
     still work as the remote cannot send 2fpp as it would exceed the 30ms.
     However, certain combinations cannot be represented, e.g. if you want 2fpp
     of G.729 (20ms) and 1fpp of G.723.1 (30ms) then the G.729 codec COULD
     receive 3fpp. This is really a failing in SIP/SDP and the techniques for
     woking around the limitation are for too complicated to be worth doing for
     what should be rare cases.
    */

  if (m_stringOptions.GetBoolean(OPAL_OPT_OFFER_SDP_PTIME)) {
    unsigned ptime = 0;
    for (SDPMediaFormatList::const_iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
      const OpalMediaFormat & mediaFormat = format->GetMediaFormat();
      if (mediaFormat.HasOption(OpalAudioFormat::TxFramesPerPacketOption())) {
        unsigned ptime1 = mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption()) * mediaFormat.GetFrameTime() / mediaFormat.GetTimeUnits();
        if (ptime < ptime1)
          ptime = ptime1;
      }
    }
    if (ptime > 0)
      strm << "a=ptime:" << ptime << CRLF;
  }

  unsigned largestFrameTime = 0;
  unsigned maxptime = UINT_MAX;

  // output attributes for each payload type
  for (SDPMediaFormatList::const_iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
    const OpalMediaFormat & mediaFormat = format->GetMediaFormat();
    if (mediaFormat.HasOption(OpalAudioFormat::RxFramesPerPacketOption())) {
      unsigned frameTime = mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits();
      if (largestFrameTime < frameTime)
        largestFrameTime = frameTime;

      unsigned maxptime1 = mediaFormat.GetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption())*frameTime;
      if (maxptime > maxptime1)
        maxptime = maxptime1;
    }
  }

  if (maxptime < UINT_MAX) {
    if (maxptime < largestFrameTime)
      maxptime = largestFrameTime;
    strm << "a=maxptime:" << maxptime << CRLF;
  }
}


void SDPAudioMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  /* NOTE: must make sure anything passed through to a SDPFormat isntance does
           not require it to have an OpalMediaFormat yet, as that is not created
           until PostDecode() */

  if (attr *= "ptime") {
    unsigned newTime = value.AsUnsigned();
    if (newTime < SDP_MIN_PTIME) {
      PTRACE(2, "SDP\tMalformed ptime attribute value " << value);
      return;
    }
    m_PTime = newTime;
    return;
  }

  if (attr *= "maxptime") {
    unsigned newTime = value.AsUnsigned();
    if (newTime < SDP_MIN_PTIME) {
      PTRACE(2, "SDP\tMalformed maxptime attribute value " << value);
      return;
    }
    m_maxPTime = newTime;
    return;
  }

  return SDPRTPAVPMediaDescription::SetAttribute(attr, value);
}


bool SDPAudioMediaDescription::PostDecode(const OpalMediaFormatList & mediaFormats)
{
  if (!SDPRTPAVPMediaDescription::PostDecode(mediaFormats))
    return false;

  for (SDPMediaFormatList::iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
    OpalMediaFormat & mediaFormat = format->GetWritableMediaFormat();

    // Set the frame size options from media's ptime & maxptime attributes
    if (m_PTime >= SDP_MIN_PTIME && mediaFormat.HasOption(OpalAudioFormat::TxFramesPerPacketOption())) {
      unsigned frames = (m_PTime * mediaFormat.GetTimeUnits()) / mediaFormat.GetFrameTime();
      if (frames < 1)
        frames = 1;
      mediaFormat.SetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), frames);
    }

    if (m_maxPTime >= SDP_MIN_PTIME && mediaFormat.HasOption(OpalAudioFormat::RxFramesPerPacketOption())) {
      unsigned frames = (m_maxPTime * mediaFormat.GetTimeUnits()) / mediaFormat.GetFrameTime();
      if (frames < 1)
        frames = 1;
      mediaFormat.SetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption(), frames);
    }
  }

  return true;
}


//////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

SDPVideoMediaDescription::SDPVideoMediaDescription(const OpalTransportAddress & address)
  : SDPRTPAVPMediaDescription(address, OpalMediaType::Video())
  , m_contentRole(OpalVideoFormat::eNoRole)
  , m_contentMask(0)
{
}


SDPMediaFormat * SDPVideoMediaDescription::CreateSDPMediaFormat()
{
  return new Format(*this);
}


static const char * const ContentRoleNames[OpalVideoFormat::EndContentRole] = { NULL, "slides", "main", "speaker", "sl" };


static void OuputRTCP_FB(ostream & strm, int payloadType, OpalVideoFormat::RTCPFeedback rtcp_fb)
{
  static struct {
    const char * m_prefix;
    OpalVideoFormat::RTCPFeedback m_bits;
  } const Prefixes[] = {
    { "nack", OpalVideoFormat::e_PLI|OpalVideoFormat::e_NACK|OpalVideoFormat::e_SLI },
    { "ccm",  OpalVideoFormat::e_FIR|OpalVideoFormat::e_TMMBR|OpalVideoFormat::e_TSTR|OpalVideoFormat::e_VBCM }
  };

  for (PINDEX i = 0; i < PARRAYSIZE(Prefixes); ++i) {
    OpalVideoFormat::RTCPFeedback masked(rtcp_fb - ~Prefixes[i].m_bits);
    if (masked != OpalVideoFormat::e_NoRTCPFb) {
      strm << "a=rtcp-fb:";
      if (payloadType < 0)
        strm << '*';
      else
        strm << payloadType;
      masked -= OpalVideoFormat::e_NACK; // Or ends up in there twice
      strm << ' ' << Prefixes[i].m_prefix << ' ' << masked << CRLF;
    }
  }
}


void SDPVideoMediaDescription::OutputAttributes(ostream & strm) const
{
  // call ancestor
  SDPRTPAVPMediaDescription::OutputAttributes(strm);

  if (m_contentRole < OpalVideoFormat::EndContentRole && ContentRoleNames[m_contentRole] != NULL)
    strm << "a=content:" << ContentRoleNames[m_contentRole] << CRLF;

  // m_rtcp_fb is set via SDPRTPAVPMediaDescription::PreEncode according to various options
  OuputRTCP_FB(strm, -1, m_rtcp_fb);
}


void SDPVideoMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  /* NOTE: must make sure anything passed through to a SDPFormat isntance does
           not require it to have an OpalMediaFormat yet, as that is not created
           until PostDecode() */

  if (attr *= "content") {
    PStringArray tokens = value.Tokenise(',');
    for (PINDEX i = 0; i < tokens.GetSize(); ++i) {
      OpalVideoFormat::ContentRole content = OpalVideoFormat::EndContentRole;
      while ((content = (OpalVideoFormat::ContentRole)(content-1)) > OpalVideoFormat::eNoRole) {
        if (tokens[i] *= ContentRoleNames[content]) {
          m_contentMask |= OpalVideoFormat::ContentRoleBit(content);

          if (m_contentRole == OpalVideoFormat::eNoRole) {
            m_contentRole = content;
            PTRACE(4, "SDP\tContent Role set to " << m_contentRole << " from \"" << value << '"');
          }
        }
      }
    }
    return;
  }

  if (attr *= "rtcp-fb") {
    if (value[0] == '*') {
      PString params = value.Mid(1).Trim();
      SDPMediaFormatList::iterator format;
      for (format = m_formats.begin(); format != m_formats.end(); ++format) {
        Format * vidFmt = dynamic_cast<Format *>(&*format);
        if (PAssertNULL(vidFmt) != NULL)
          vidFmt->AddRTCP_FB(params);
      }
    }
    else {
      PString params = value;
      Format * format = dynamic_cast<Format *>(FindFormat(params));
      if (format != NULL)
        format->AddRTCP_FB(params);
    }
    return;
  }

  // As per RFC 6263
  if (attr *= "imageattr") {
    PString params = value;
    Format * format = dynamic_cast<Format *>(FindFormat(params));
    if (format != NULL)
      format->ParseImageAttr(params);
    return;
  }

  return SDPRTPAVPMediaDescription::SetAttribute(attr, value);
}


bool SDPVideoMediaDescription::PostDecode(const OpalMediaFormatList & mediaFormats)
{
  if (!SDPRTPAVPMediaDescription::PostDecode(mediaFormats))
    return false;

  for (SDPMediaFormatList::iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
    OpalMediaFormat & mediaFormat = format->GetWritableMediaFormat();
    mediaFormat.SetOptionEnum(OpalVideoFormat::ContentRoleOption(), m_contentRole);
    m_contentMask |= mediaFormat.GetOptionInteger(OpalVideoFormat::ContentRoleMaskOption());
    mediaFormat.SetOptionInteger(OpalVideoFormat::ContentRoleMaskOption(), m_contentMask);
  }

  return true;
}


bool SDPVideoMediaDescription::PreEncode()
{
  if (!SDPRTPAVPMediaDescription::PreEncode())
    return false;

  if (m_formats.IsEmpty())
    return true;

  /* Even though the bandwidth parameter COULD be used for audio, we don't
     do it as it has very limited use. Variable bit rate audio codecs are
     not common, and usually there is an fmtp value to select the different
     bit rates. So, as it might cause interoperabity difficulties with the
     dumber implementations out there we just don't.

     As per RFC3890 we set both AS and TIAS parameters.
  */
  for (SDPMediaFormatList::iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
    const OpalMediaFormat & mediaFormat = format->GetMediaFormat();

    for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); ++i) {
      const OpalMediaOption & option = mediaFormat.GetOption(i);
      PCaselessString name = option.GetName();
      if (name.NumCompare(SDPBandwidthPrefix, sizeof(SDPBandwidthPrefix)-1) == EqualTo)
        m_bandwidth.SetMax(name.Mid(sizeof(SDPBandwidthPrefix)-1), option.AsString().AsUnsigned());
    }

    /**We set the bandwidth parameter to the largest of all the formats offerred.
       And individual format may be able to further retrict the bandwidth in it's
       FMTP line, e.g. H.264 can use a max-br=XXX option.
      */
    OpalBandwidth bw = mediaFormat.GetMaxBandwidth();
    m_bandwidth.SetMax(SDPSessionDescription::TransportIndependentBandwidthType(), bw);
    m_bandwidth.SetMax(SDPSessionDescription::ApplicationSpecificBandwidthType(),  bw.kbps());

    if (m_contentRole == OpalVideoFormat::eNoRole)
      m_contentRole = format->GetMediaFormat().GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole);
  }

  m_rtcp_fb = OpalVideoFormat::e_NoRTCPFb;

  if (m_enableFeedback || m_stringOptions.GetInteger(OPAL_OPT_OFFER_RTCP_FB, 1) == 1) {
    bool first = true;
    for (SDPMediaFormatList::iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
      Format * vidFmt = dynamic_cast<Format *>(&*format);
      if (vidFmt == NULL)
        continue;
      if (first) {
        first = false;
        m_rtcp_fb = vidFmt->GetRTCP_FB();
      }
      else if (m_rtcp_fb != vidFmt->GetRTCP_FB()) {
        m_rtcp_fb = OpalVideoFormat::e_NoRTCPFb;
        break;
      }
    }
  }

  if (m_rtcp_fb != OpalVideoFormat::e_NoRTCPFb) {
    for (SDPMediaFormatList::iterator format = m_formats.begin(); format != m_formats.end(); ++format) {
      Format * vidFmt = dynamic_cast<Format *>(&*format);
      if (PAssertNULL(vidFmt) != NULL)
        vidFmt->SetRTCP_FB(OpalVideoFormat::e_NoRTCPFb);
    }
  }

  return true;
}


SDPVideoMediaDescription::Format::Format(SDPVideoMediaDescription & parent)
  : SDPRTPAVPMediaDescription::Format(parent)
  , m_minRxWidth(0)
  , m_minRxHeight(0)
  , m_maxRxWidth(0)
  , m_maxRxHeight(0)
  , m_minTxWidth(0)
  , m_minTxHeight(0)
  , m_maxTxWidth(0)
  , m_maxTxHeight(0)
{
}


void SDPVideoMediaDescription::Format::PrintOn(ostream & strm) const
{
  SDPMediaFormat::PrintOn(strm);

  OuputRTCP_FB(strm, m_payloadType, m_rtcp_fb);

  if (m_mediaFormat.GetOptionEnum(OpalVideoFormat::UseImageAttributeInSDP(), OpalVideoFormat::ImageAttrSuppressed) != OpalVideoFormat::ImageAttrSuppressed)
    strm << "a=imageattr:" << (int)m_payloadType
         << " recv [x=[" << m_mediaFormat.GetOptionInteger(OpalVideoFormat::MinRxFrameWidthOption())
         << ":16:" << m_mediaFormat.GetOptionInteger(OpalVideoFormat::MaxRxFrameWidthOption())
         << "],y=[" << m_mediaFormat.GetOptionInteger(OpalVideoFormat::MinRxFrameHeightOption())
         << ":16:" << m_mediaFormat.GetOptionInteger(OpalVideoFormat::MaxRxFrameHeightOption())
         << "]] send [x=[128:16:" << m_mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption())
         << "],y=[96:16:" << m_mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption())
         << "]]\r\n";
}


bool SDPVideoMediaDescription::Format::PreEncode()
{
  m_rtcp_fb = m_mediaFormat.GetOptionEnum(OpalVideoFormat::RTCPFeedbackOption(), OpalVideoFormat::e_NoRTCPFb);
  return SDPMediaFormat::PreEncode();
}


static bool AdjustResolution(OpalMediaFormat & mediaFormat, const PString & name, unsigned value, bool lower)
{
  if (value == 0)
    return false;

  unsigned oldValue = mediaFormat.GetOptionInteger(name);
  if (lower ? (value < oldValue) : (value > oldValue)) {
    mediaFormat.SetOptionInteger(name, value);
    PTRACE(4, "SDP\tAdjusted " << mediaFormat << " option \"" << name << "\" from " << oldValue << " to " << value);
  }

  return true;
}

void SDPVideoMediaDescription::Format::SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const
{
  SDPMediaFormat::SetMediaFormatOptions(mediaFormat);

  // Save the RTCP feedback (RFC4585) capability.
  if (m_rtcp_fb != OpalVideoFormat::e_NoRTCPFb || !m_parent.GetOptionStrings().GetBoolean(OPAL_OPT_FORCE_RTCP_FB))
    mediaFormat.SetOptionEnum(OpalVideoFormat::RTCPFeedbackOption(), m_rtcp_fb);

  if (mediaFormat.GetOptionEnum(OpalVideoFormat::UseImageAttributeInSDP(), OpalVideoFormat::ImageAttrSuppressed) != OpalVideoFormat::ImageAttrSuppressed) {
    bool ok = (
                AdjustResolution(mediaFormat, OpalVideoFormat::FrameWidthOption(),  m_minTxWidth,  true) &&
                AdjustResolution(mediaFormat, OpalVideoFormat::FrameHeightOption(), m_minTxHeight, true) &&
                AdjustResolution(mediaFormat, OpalVideoFormat::FrameWidthOption(),  m_maxTxWidth,  false) &&
                AdjustResolution(mediaFormat, OpalVideoFormat::FrameHeightOption(), m_maxTxHeight, false)
              )
              ||
              (
                AdjustResolution(mediaFormat, OpalVideoFormat::MinRxFrameWidthOption(),  m_minRxWidth,  true) &&
                AdjustResolution(mediaFormat, OpalVideoFormat::MinRxFrameHeightOption(), m_minRxHeight, true) &&
                AdjustResolution(mediaFormat, OpalVideoFormat::MaxRxFrameWidthOption(),  m_maxRxWidth,  false) &&
                AdjustResolution(mediaFormat, OpalVideoFormat::MaxRxFrameHeightOption(), m_maxRxHeight, false)
              );
    mediaFormat.SetOptionEnum(OpalVideoFormat::UseImageAttributeInSDP(), ok ? OpalVideoFormat::ImageAttrAnswerRequired
                                                                            : OpalVideoFormat::ImageAttrSuppressed);
    PTRACE(4, "SDP\t" << (ok ? "Enabled" : "Disabled") << " imageattr in reply for " << mediaFormat);
  }
}


void SDPVideoMediaDescription::Format::ParseImageAttr(const PString & params)
{
  bool sendAttr = false;
  PINDEX pos = 0;
  while (pos < params.GetLength()) {
    while (isspace(params[pos]))
      ++pos;

    static PConstString send("send");
    if (params.NumCompare(send, send.GetLength(), pos) == EqualTo) {
      pos +=send.GetLength();
      sendAttr = true;
      continue;
    }

    static PConstString recv("recv");
    if (params.NumCompare(recv, recv.GetLength(), pos) == EqualTo) {
      pos += recv.GetLength();
      sendAttr = false;
      continue;
    }

    if (params[pos] == '*') {
      while (isspace(params[++pos]))
        ;
      continue;
    }

    if (params[pos] != '[') {
      PTRACE(2, "SDP\tIllegal imageattr (expected '['): \"" << params << '"');
      return;
    }

    do {
      PINDEX equal = params.Find('=', pos);
      if (equal == P_MAX_INDEX) {
        PTRACE(2, "SDP\tIllegal imageattr (expected '='): \"" << params << '"');
        return;
      }

      PCaselessString attrName = params(pos+1, equal-1);
      pos = equal+1;

      PString attrValue;
      if (params[pos] == '[') {
        if ((pos = params.Find(']', equal)) == P_MAX_INDEX) {
          PTRACE(2, "SDP\tIllegal imageattr (expected ']'): \"" << params << '"');
          return;
        }

        attrValue = params(equal+2, pos-1);

        ++pos;
        if (params[pos] != ',' && params[pos] != ']') {
          PTRACE(2, "SDP\tIllegal imageattr (expected ',' or ']'): \"" << params << '"');
          return;
        }
      }
      else {
        while (params[pos] != ',' && params[pos] != ']') {
          if (++pos >= params.GetLength()) {
            PTRACE(2, "SDP\tIllegal imageattr (expected ',' or ']'): \"" << params << '"');
            return;
          }
        }

        attrValue = params(equal+1, pos-1);
      }

      if (attrName == "x" || attrName == "y") {
        unsigned minimum, maximum;
        PStringArray values = attrValue.Tokenise(':');
        switch (values.GetSize()) {
          case 3 :
            minimum = values[0].AsUnsigned();
            maximum = values[2].AsUnsigned();
            break;

          case 2 :
            minimum = values[0].AsUnsigned();
            maximum = values[1].AsUnsigned();
            break;

          default :
            PTRACE(2, "SDP\tIllegal imageattr (expected ':'): \"" << params << '"');
            return;
        }

        if (attrName == "x") {
          if (sendAttr) {
            m_minTxWidth = minimum;
            m_maxTxWidth = maximum;
          }
          else {
            m_minRxWidth = minimum;
            m_maxRxWidth = maximum;
          }
        }
        else {
          if (sendAttr) {
            m_minTxHeight = minimum;
            m_maxTxHeight = maximum;
          }
          else {
            m_minRxHeight = minimum;
            m_maxRxHeight = maximum;
          }
        }
      }
#if 0
      else if (attrName == "sar") {
      }
      else if (attrName == "par") {
        PString w, h;
        if (!attrValue.Split('-', w, h) {
          PTRACE(2, "SDP\tIllegal imageattr (expected '-'): \"" << params << '"');
          return;
        }
      }
      else if (attrName == "q") {
        PINDEX end = params.FindSpan("0123456789.", pos);
        double q = params(pos, end).AsReal();
        pos = end+1;
      }
#endif
      else {
        PTRACE(3, "SDP\tUnknown/unsupported imageattr attribute: \"" << params << '"');
      }
    } while (params[pos] == ',');

    if (params[pos++] != ']') {
      PTRACE(2, "SDP\tIllegal imageattr (expected ']'): \"" << params << '"');
      return;
    }
  }

  // Allow response SDP to include imageattr from RFC 6263
  PTRACE(4, "SDP\tparsed imageattr:"
             " minRx=" << m_minRxWidth << 'x' << m_minRxHeight
          << " maxRx=" << m_maxRxWidth << 'x' << m_maxRxHeight
          << " minTx=" << m_minTxWidth << 'x' << m_minTxHeight
          << " maxTx=" << m_maxTxWidth << 'x' << m_maxTxHeight);
}


#endif // OPAL_VIDEO


//////////////////////////////////////////////////////////////////////////////

const PCaselessString & SDPApplicationMediaDescription::TypeName() { static PConstCaselessString const s("application"); return s; }

SDPApplicationMediaDescription::SDPApplicationMediaDescription(const OpalTransportAddress & address)
  : SDPMediaDescription(address, "")
{
}


PCaselessString SDPApplicationMediaDescription::GetSDPTransportType() const
{
  return OpalRTPSession::RTP_AVP();
}


PString SDPApplicationMediaDescription::GetSDPMediaType() const
{
  return TypeName();
}


SDPMediaFormat * SDPApplicationMediaDescription::CreateSDPMediaFormat()
{
  return new Format(*this);
}


bool SDPApplicationMediaDescription::Format::Initialise(const PString & portString)
{
  if (portString.IsEmpty())
    return false;

  m_encodingName = portString;
  return true;
}


//////////////////////////////////////////////////////////////////////////////

SDPSessionDescription::SDPSessionDescription(time_t sessionId, unsigned version, const OpalTransportAddress & address)
  : protocolVersion(0)
  , sessionName(SIP_DEFAULT_SESSION_NAME)
  , ownerUsername('-')
  , ownerSessionId(sessionId)
  , ownerVersion(version)
  , ownerAddress(address)
  , defaultConnectAddress(address)
{
}


void SDPSessionDescription::PrintOn(ostream & strm) const
{
  /* encode mandatory session information, note the order is important according to RFC!
     Must be vosiuepcbzkatrm and within the m it is icbka */
  strm << "v=" << protocolVersion << CRLF
       << "o=" << ownerUsername << ' '
       << ownerSessionId << ' '
       << ownerVersion << ' '
       << GetConnectAddressString(ownerAddress)
       << CRLF
       << "s=" << sessionName << CRLF;

  if (!defaultConnectAddress.IsEmpty())
    strm << "c=" << GetConnectAddressString(defaultConnectAddress) << CRLF;

  strm << m_bandwidth
       << "t=" << "0 0" << CRLF;

  OutputAttributes(strm);

  // find groups
  bool hasICE = false;
  PString bundle;
  PString msid;
  for (PINDEX i = 0; i < mediaDescriptions.GetSize(); i++) {
    const SDPRTPAVPMediaDescription * rtp = dynamic_cast<const SDPRTPAVPMediaDescription *>(&mediaDescriptions[i]);
    if (rtp != NULL) {
      if (rtp->HasICE())
        hasICE = true;

      PString id = rtp->GetMediaGroupId();
      if (!id.IsEmpty()) {
        SDPRTPAVPMediaDescription::SsrcInfo::const_iterator it = rtp->GetSsrcInfo().begin();
        if (it != rtp->GetSsrcInfo().end())
          msid = it->second.GetString("mslabel");
        bundle &= id;
      }
    }
  }

  if (!bundle.IsEmpty())
    strm << "a=group:BUNDLE " << bundle << CRLF;
  if (!msid.IsEmpty())
    strm << "a=msid-semantic: WMS " << msid << CRLF;

  if (hasICE)
    strm << "a=" << IceLiteOption << CRLF;

  // encode media session information
  for (PINDEX i = 0; i < mediaDescriptions.GetSize(); i++) {
    if (mediaDescriptions[i].PreEncode())
      mediaDescriptions[i].Encode(defaultConnectAddress, strm);
  }
}


PString SDPSessionDescription::Encode() const
{
  PStringStream str;
  PrintOn(str);
  return str;
}


bool SDPSessionDescription::Decode(const PString & str, const OpalMediaFormatList & mediaFormats)
{
  PTRACE(5, "SDP\tDecode using media formats:\n    " << setfill(',') << mediaFormats << setfill(' '));

  bool atLeastOneValidMedia = false;
  bool ok = true;

  // break string into lines
  PStringArray lines = str.Lines();

  // parse keyvalue pairs
  SDPMediaDescription * currentMedia = NULL;
  PINDEX i;
  for (i = 0; i < lines.GetSize(); i++) {
    const PString & line = lines[i];
    if (line.GetLength() < 3 || line[1] != '=')
      continue; // Ignore illegal lines

    PString value = line.Mid(2).Trim();

    /////////////////////////////////
    //
    // Session description
    //
    /////////////////////////////////

    if (currentMedia != NULL && line[0] != 'm')
      currentMedia->Decode(line[0], value);
    else {
      switch (line[0]) {
        case 'v' : // protocol version (mandatory)
          protocolVersion = value.AsInteger();
          break;

        case 'o' : // owner/creator and session identifier (mandatory)
          ParseOwner(value);
          break;

        case 's' : // session name (mandatory)
          sessionName = value;
          break;

        case 'c' : // connection information - not required if included in all media
          defaultConnectAddress = ParseConnectAddress(value);
          PTRACE_IF(4, !defaultConnectAddress.IsEmpty(), "SDP\tParsed default connection address " << defaultConnectAddress);
          break;

        case 't' : // time the session is active (mandatory)
        case 'i' : // session information
        case 'u' : // URI of description
        case 'e' : // email address
        case 'p' : // phone number
          break;
        case 'b' : // bandwidth information
          m_bandwidth.Parse(value);
          break;
        case 'z' : // time zone adjustments
        case 'k' : // encryption key
        case 'r' : // zero or more repeat times
          break;
        case 'a' : // zero or more session attribute lines
          ParseAttribute(value);
          break;

        case 'm' : // media name and transport address (mandatory)
          {
            if (currentMedia != NULL) {
              PTRACE(3, "SDP\tParsed media session with " << currentMedia->GetSDPMediaFormats().GetSize()
                                                          << " '" << currentMedia->GetSDPMediaType() << "' formats");
              if (!currentMedia->PostDecode(mediaFormats))
                ok = false;
            }

            currentMedia = NULL;

            OpalMediaType mediaType;
            OpalMediaTypeDefinition * defn;
            PStringArray tokens = value.Tokenise(WhiteSpace, false); // Spec says space only, but lets be forgiving
            if (tokens.GetSize() < 4) {
              PTRACE(1, "SDP\tMedia session has only " << tokens.GetSize() << " elements");
            }
            else if ((mediaType = GetMediaTypeFromSDP(tokens[0], tokens[2], lines, i)).empty()) {
              PTRACE(1, "SDP\tUnknown SDP media type parsing \"" << lines[i] << '"');
            }
            else if ((defn = mediaType.GetDefinition()) == NULL) {
              PTRACE(1, "SDP\tNo definition for media type " << mediaType << " parsing \"" << lines[i] << '"');
            }
            else if ((currentMedia = defn->CreateSDPMediaDescription(defaultConnectAddress)) == NULL) {
              PTRACE(1, "SDP\tCould not create SDP media description for media type " << mediaType << " parsing \"" << lines[i] << '"');
            }
            else {
              PTRACE_CONTEXT_ID_TO(currentMedia);
              if (currentMedia->Decode(tokens))
                atLeastOneValidMedia = true;
              else {
                delete currentMedia;
                currentMedia = NULL;
              }
            }

            if (currentMedia == NULL) {
              currentMedia = new SDPDummyMediaDescription(defaultConnectAddress, tokens);
              PTRACE_CONTEXT_ID_TO(currentMedia);
            }

            mediaDescriptions.Append(currentMedia);
          }
          break;

        default:
          PTRACE(1, "SDP\tUnknown session information key " << line[0]);
      }
    }
  }

  if (currentMedia != NULL) {
    PTRACE(3, "SDP\tParsed final media session with " << currentMedia->GetSDPMediaFormats().GetSize()
                                                << " '" << currentMedia->GetSDPMediaType() << "' formats");
    if (!currentMedia->PostDecode(mediaFormats))
      ok = false;
  }

  return ok && (atLeastOneValidMedia || mediaDescriptions.IsEmpty());
}


void SDPSessionDescription::SetAttribute(const PString & attr, const PString & value)
{
  if (attr *= "group") {
    PStringArray tokens = value.Tokenise(WhiteSpace, false); // Spec says space only, but lets be forgiving
    if (tokens.GetSize() > 2) {
      PString name = tokens[0];
      tokens.RemoveAt(0);
      m_groups.SetAt(name, new PStringArray(tokens));
    }
    return;
  }

  if (attr *= "msid-semantic") {
    if (value.NumCompare("WMS") == EqualTo)
      m_groupId = value.Mid(3).Trim();
    return;
  }

  if (attr *= IceLiteOption) {
    m_iceOptions += IceLiteOption;
    return;
  }

  SDPCommonAttributes::SetAttribute(attr, value);
}


void SDPSessionDescription::ParseOwner(const PString & str)
{
  PStringArray tokens = str.Tokenise(WhiteSpace, false); // Spec says space only, but lets be forgiving

  if (tokens.GetSize() != 6) {
    PTRACE(2, "SDP\tOrigin has incorrect number of elements (" << tokens.GetSize() << ')');
  }
  else {
    ownerUsername    = tokens[0];
    ownerSessionId   = tokens[1].AsUnsigned();
    ownerVersion     = tokens[2].AsUnsigned();
    ownerAddress = defaultConnectAddress = ParseConnectAddress(tokens, 3);
    PTRACE_IF(4, !ownerAddress.IsEmpty(), "SDP\tParsed owner connection address " << ownerAddress);
  }
}


SDPMediaDescription * SDPSessionDescription::GetMediaDescriptionByType(const OpalMediaType & rtpMediaType) const
{
  // look for matching media type
  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) {
    if (mediaDescriptions[i].GetMediaType() == rtpMediaType)
      return &mediaDescriptions[i];
  }

  return NULL;
}

SDPMediaDescription * SDPSessionDescription::GetMediaDescriptionByIndex(PINDEX index) const
{
  if (index > mediaDescriptions.GetSize())
    return NULL;

  return &mediaDescriptions[index-1];
}

SDPMediaDescription::Direction SDPSessionDescription::GetDirection(unsigned sessionID) const
{
  if (sessionID > 0 && sessionID <= (unsigned)mediaDescriptions.GetSize())
    return mediaDescriptions[sessionID-1].GetDirection();

  return defaultConnectAddress.IsEmpty() ? SDPMediaDescription::Inactive : m_direction;
}


bool SDPSessionDescription::IsHold() const
{
  if (defaultConnectAddress.IsEmpty()) // Old style "hold"
    return true;

  if (GetBandwidth(SDPSessionDescription::ApplicationSpecificBandwidthType()) == 0)
    return true;

  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) {
    SDPMediaDescription::Direction dir = mediaDescriptions[i].GetDirection();
    if ((dir == SDPMediaDescription::Undefined) || ((dir & SDPMediaDescription::RecvOnly) != 0))
      return false;
  }

  return true;
}


bool SDPSessionDescription::HasActiveSend() const
{
  if (defaultConnectAddress.IsEmpty()) // Old style "hold"
    return false;

  if (GetBandwidth(SDPSessionDescription::ApplicationSpecificBandwidthType()) == 0)
    return false;

  for (PINDEX i = 0; i < mediaDescriptions.GetSize(); i++) {
    if ((mediaDescriptions[i].GetDirection() & SDPMediaDescription::SendOnly) != 0)
      return true;
  }

  return false;
}


void SDPSessionDescription::SetDefaultConnectAddress(const OpalTransportAddress & address)
{
   defaultConnectAddress = address;
   if (ownerAddress.IsEmpty())
     ownerAddress = address;
}


static OpalMediaFormat GetNxECapabilities(const SDPMediaDescription & incomingMedia, const OpalMediaFormat & mediaFormat)
{
  // Find the payload type and capabilities used for telephone-event, if present
  const SDPMediaFormatList & sdpMediaList = incomingMedia.GetSDPMediaFormats();
  for (SDPMediaFormatList::const_iterator format = sdpMediaList.begin(); format != sdpMediaList.end(); ++format) {
    if (format->GetEncodingName() == mediaFormat.GetEncodingName())
      return format->GetMediaFormat();
  }

  return OpalMediaFormat();
}


OpalMediaFormatList SDPSessionDescription::GetMediaFormats() const
{
  OpalMediaFormatList formatList;

  // Extract the media from SDP into our remote capabilities list
  for (PINDEX i = 0; i < mediaDescriptions.GetSize(); ++i) {
    SDPMediaDescription & mediaDescription = mediaDescriptions[i];

    formatList += mediaDescription.GetMediaFormats();

    // Find the payload type and capabilities used for telephone-event, if present
    formatList += GetNxECapabilities(mediaDescription, OpalRFC2833);
#if OPAL_T38_CAPABILITY
    formatList += GetNxECapabilities(mediaDescription, OpalCiscoNSE);
#endif
  }

#if OPAL_T38_CAPABILITY
  /* We default to having T.38 included as most UAs do not actually
     tell you that they support it or not. For the re-INVITE mechanism
     to work correctly, the rest ofthe system has to assume that the
     UA is capable of it, even it it isn't. */
  formatList += OpalT38;
#endif

  return formatList;
}


static void SanitiseName(PString & str)
{
  PINDEX i = 0;
  while (i < str.GetLength()) {
    if (isprint(str[i]))
      ++i;
    else
      str.Delete(i, 1);
  }
}


void SDPSessionDescription::SetSessionName(const PString & v)
{
  sessionName = v;
  SanitiseName(sessionName);
  if (sessionName.IsEmpty())
    sessionName = '-';
}


void SDPSessionDescription::SetUserName(const PString & v)
{
  ownerUsername = v;
  SanitiseName(ownerUsername);
  ownerUsername.Replace(' ', '_', true);
  if (ownerUsername.IsEmpty())
    ownerUsername = '-';
}


#endif // OPAL_SIP

// End of file ////////////////////////////////////////////////////////////////
