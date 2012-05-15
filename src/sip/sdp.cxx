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
#include <opal/buildopts.h>

#if OPAL_SIP

#ifdef __GNUC__
#pragma implementation "sdp.h"
#endif

#include <sip/sdp.h>

#include <ptlib/socket.h>
#include <opal/transports.h>
#include <rtp/srtp_session.h>


#define SIP_DEFAULT_SESSION_NAME    "Opal SIP Session"

static const char SDPBandwidthPrefix[] = "SDP-Bandwidth-";


#define new PNEW

//
// uncommment this to output a=fmtp lines before a=rtpmap lines. Useful for testing
//
//#define FMTP_BEFORE_RTPMAP 1

#define SDP_MIN_PTIME 10


/////////////////////////////////////////////////////////
//
//  the following functions bind the media type factory to the SDP format types
//

static OpalMediaType GetMediaTypeFromSDPType(const std::string & sdpType)
{
  OpalMediaTypeList mediaTypes = OpalMediaType::GetList();
  for (OpalMediaTypeList::iterator iterMediaType = mediaTypes.begin(); iterMediaType != mediaTypes.end(); ++iterMediaType) {
    if (iterMediaType->GetDefinition()->GetSDPType() == sdpType)
      return *iterMediaType;
  }

  return OpalMediaType();
}

OpalMediaType OpalMediaType::GetMediaTypeFromSDP(const std::string & sdp, const std::string & transport)
{
  OpalMediaType mediaType = GetMediaTypeFromSDPType(sdp);
  if (mediaType.empty()) {
    mediaType = GetMediaTypeFromSDPType(sdp + '|' + transport);
    if (mediaType.empty()) {
      size_t slash = transport.find('/');
      if (slash != string::npos)
        mediaType = GetMediaTypeFromSDPType(sdp + '|' + transport.substr(0, slash));
    }
  }

  return mediaType;
}

SDPMediaDescription * OpalAudioMediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress,
                                                                    OpalMediaSession * /*session*/) const
{
  return new SDPAudioMediaDescription(localAddress);
}

#if OPAL_VIDEO
SDPMediaDescription * OpalVideoMediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress,
                                                                    OpalMediaSession * /*session*/) const
{
  return new SDPVideoMediaDescription(localAddress);
}
#endif


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
          if (ip.IsValid()) {
            OpalTransportAddress address(ip, port, OpalTransportAddress::UdpPrefix());
            PTRACE(4, "SDP\tParsed connection address " << address);
            return address;
          }
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
  PStringArray tokens = str.Tokenise(' ');
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

SDPMediaFormat::SDPMediaFormat(SDPMediaDescription & parent, RTP_DataFrame::PayloadTypes pt, const char * _name)
  : m_parent(parent) 
  , payloadType(pt)
  , clockRate(0)
  , encodingName(_name)
{
  PTRACE_CONTEXT_ID_FROM(parent);
}


SDPMediaFormat::SDPMediaFormat(SDPMediaDescription & parent, const OpalMediaFormat & fmt)
  : m_mediaFormat(fmt)
  , m_parent(parent) 
  , payloadType(fmt.GetPayloadType())
  , clockRate(fmt.GetClockRate())
  , encodingName(fmt.GetEncodingName())
{
  if (fmt.GetMediaType() == OpalMediaType::Audio()) 
    parameters = PString(PString::Unsigned, fmt.GetOptionInteger(OpalAudioFormat::ChannelsOption()));
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
  PAssert(!encodingName.IsEmpty(), "SDPMediaFormat encoding name is empty");

  PINDEX i;
  for (i = 0; i < 2; ++i) {
    switch (i) {
#ifdef FMTP_BEFORE_RTPMAP
      case 1:
#else
      case 0:
#endif
        /* Even though officially, case is not significant for SDP encoding
           types, we make it upper case anyway as this seems to be the custom,
           and some very stupid endpoints assume it is always the case. */
        strm << "a=rtpmap:" << (int)payloadType << ' ' << encodingName.ToUpper() << '/' << clockRate;
        if (!parameters.IsEmpty())
          strm << '/' << parameters;
        strm << "\r\n";
        break;
#ifdef FMTP_BEFORE_RTPMAP
      case 0:
#else
      case 1:
#endif
        {
          PString fmtpString = GetFMTP();
          if (!fmtpString.IsEmpty())
            strm << "a=fmtp:" << (int)payloadType << ' ' << fmtpString << "\r\n";
        }
    }
  }
}


void SDPMediaFormat::SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const
{
  mediaFormat.MakeUnique();
  mediaFormat.SetPayloadType(payloadType);
  mediaFormat.SetOptionInteger(OpalAudioFormat::ChannelsOption(), parameters.IsEmpty() ? 1 : parameters.AsUnsigned());

  // Fill in the default values for (possibly) missing FMTP options
  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    OpalMediaOption & option = const_cast<OpalMediaOption &>(mediaFormat.GetOption(i));
    PString value = option.GetFMTPDefault();
    if (!value.IsEmpty())
      option.FromString(value);
  }

  // Set the frame size options from media's ptime & maxptime attributes
  if (m_parent.GetPTime() >= SDP_MIN_PTIME && mediaFormat.HasOption(OpalAudioFormat::TxFramesPerPacketOption())) {
    unsigned frames = (m_parent.GetPTime() * mediaFormat.GetTimeUnits()) / mediaFormat.GetFrameTime();
    if (frames < 1)
      frames = 1;
    mediaFormat.SetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), frames);
  }

  if (m_parent.GetMaxPTime() >= SDP_MIN_PTIME && mediaFormat.HasOption(OpalAudioFormat::RxFramesPerPacketOption())) {
    unsigned frames = (m_parent.GetMaxPTime() * mediaFormat.GetTimeUnits()) / mediaFormat.GetFrameTime();
    if (frames < 1)
      frames = 1;
    mediaFormat.SetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption(), frames);
  }

  // If format has an explicit FMTP option then we only use that, no high level parsing
  if (mediaFormat.SetOptionString("FMTP", m_fmtp))
    return;

  // Look for a specific option with "FMTP" as it's name, it is the whole thing then
  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    if (option.GetFMTPName() == "FMTP") {
      mediaFormat.SetOptionValue(option.GetName(), m_fmtp);
      return;
    }
  }

  // Save the RTCP feedback (RFC4585) capability.
  if (!m_rtcp_fb.IsEmpty())
    mediaFormat.AddOption(new OpalMediaOptionString("RTCP-FB", false, m_rtcp_fb), true);

  // No FMTP to parse, may as well stop here
  if (m_fmtp.IsEmpty())
    return;

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
  m_mediaFormat.SetOptionString(OpalMediaFormat::ProtocolOption(), "SIP");
  return m_mediaFormat.ToCustomisedOptions();
}


bool SDPMediaFormat::PostDecode(const OpalMediaFormatList & mediaFormats, unsigned bandwidth)
{
  // try to init encodingName from global list, to avoid PAssert when media has no rtpmap
  if (encodingName.IsEmpty())
    encodingName = m_mediaFormat.GetEncodingName();

  if (m_mediaFormat.IsEmpty()) {
    for (OpalMediaFormatList::const_iterator iterFormat = mediaFormats.FindFormat(payloadType, clockRate, encodingName, "sip");
         iterFormat != mediaFormats.end();
         iterFormat = mediaFormats.FindFormat(payloadType, clockRate, encodingName, "sip", iterFormat)) {
      OpalMediaFormat adjustedFormat = *iterFormat;
      SetMediaFormatOptions(adjustedFormat);
      // skip formats whose fmtp don't match options
      if (iterFormat->ValidateMerge(adjustedFormat)) {
        PTRACE(3, "SIP\tRTP payload type " << encodingName << " matched to codec " << *iterFormat);
        m_mediaFormat = adjustedFormat;
        break;
      }

      PTRACE(4, "SIP\tRTP payload type " << encodingName << " not matched to codec " << *iterFormat);
    }

    if (m_mediaFormat.IsEmpty()) {
      PTRACE(2, "SDP\tCould not find media format for \""
             << encodingName << "\", pt=" << payloadType << ", clock=" << clockRate);
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
      m_mediaFormat.AddOption(new OpalMediaOptionString(SDPBandwidthPrefix + r->first, false, r->second), true);
  }

  if (bandwidth > 1000 && bandwidth < (unsigned)m_mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption())) {
    PTRACE(4, "SDP\tAdjusting format \"" << m_mediaFormat << "\" bandwidth to " << bandwidth);
    m_mediaFormat.SetOptionInteger(OpalMediaFormat::MaxBitRateOption(), bandwidth);
  }

  m_mediaFormat.SetOptionString(OpalMediaFormat::ProtocolOption(), "SIP");
  if (m_mediaFormat.ToNormalisedOptions())
    return true;

  PTRACE(2, "SDP\tCould not normalise format \"" << encodingName << "\", pt=" << payloadType << ", removing.");
  return false;
}


//////////////////////////////////////////////////////////////////////////////

unsigned & SDPBandwidth::operator[](const PCaselessString & type)
{
  return std::map<PCaselessString, unsigned>::operator[](type);
}


unsigned SDPBandwidth::operator[](const PCaselessString & type) const
{
  const_iterator it = find(type);
  return it != end() ? it->second : UINT_MAX;
}


ostream & operator<<(ostream & out, const SDPBandwidth & bw)
{
  for (SDPBandwidth::const_iterator iter = bw.begin(); iter != bw.end(); ++iter)
    out << "b=" << iter->first << ':' << iter->second << "\r\n";
  return out;
}


bool SDPBandwidth::Parse(const PString & param)
{
  PINDEX pos = param.FindSpan("!#$%&'*+-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`abcdefghijklmnopqrstuvwxyz{|}~"); // Legal chars from RFC
  if (pos == P_MAX_INDEX || param[pos] != ':') {
    PTRACE(2, "SDP\tMalformed bandwidth attribute " << param);
    return false;
  }

  (*this)[param.Left(pos)] = param.Mid(pos+1).AsUnsigned();
  return true;
}


void SDPBandwidth::SetMax(const PCaselessString & type, unsigned value)
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
  PINDEX pos = value.FindSpan("!#$%&'*+-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`abcdefghijklmnopqrstuvwxyz{|}~"); // Legal chars from RFC
  if (pos == P_MAX_INDEX)
    SetAttribute(value, "1");
  else if (value[pos] == ':')
    SetAttribute(value.Left(pos), value.Mid(pos+1));
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
}


//////////////////////////////////////////////////////////////////////////////

SDPMediaDescription::SDPMediaDescription(const OpalTransportAddress & address, const OpalMediaType & type)
  : transportAddress(address)
  , mediaType(type)
  , ptime(0)
  , maxptime(0)
{
  port      = 0;
  portCount = 0;
}


PBoolean SDPMediaDescription::SetTransportAddress(const OpalTransportAddress &t)
{
  PIPSocket::Address ip;
  WORD port = 0;
  if (transportAddress.GetIpAndPort(ip, port)) {
    transportAddress = OpalTransportAddress(t, port, OpalTransportAddress::UdpPrefix());
    return PTrue;
  }
  return PFalse;
}


bool SDPMediaDescription::Decode(const PStringArray & tokens)
{
  if (tokens.GetSize() < 3) {
    PTRACE(1, "SDP\tUnknown SDP media type " << tokens[0]);
    return false;
  }

  // parse the media type
  mediaType = OpalMediaType::GetMediaTypeFromSDP(tokens[0], tokens[2]);
  if (mediaType.empty()) {
    PTRACE(1, "SDP\tUnknown SDP media type " << tokens[0]);
    return false;
  }
  OpalMediaTypeDefinition * defn = mediaType.GetDefinition();
  if (defn == NULL) {
    PTRACE(1, "SDP\tNo definition for SDP media type " << tokens[0]);
    return false;
  }

  // parse the port and port count
  PString portStr  = tokens[1];
  PINDEX pos = portStr.Find('/');
  if (pos == P_MAX_INDEX) 
    portCount = 1;
  else {
    PTRACE(3, "SDP\tMedia header contains port count - " << portStr);
    portCount = (WORD)portStr.Mid(pos+1).AsUnsigned();
    portStr   = portStr.Left(pos);
  }
  port = (WORD)portStr.AsUnsigned();

  // parse the transport
  m_transportType = tokens[2];

  // check everything
  switch (port) {
    case 0 :
      PTRACE(3, "SDP\tIgnoring media session " << mediaType << " with port=0");
      m_direction = Inactive;
      break;

    case 65535 :
      PTRACE(2, "SDP\tIllegal port=65535 in media session " << mediaType << ", trying to continue.");
      port = 65534;
      // Do next case

    default :
      PTRACE(4, "SDP\tMedia session port=" << port);

      PIPSocket::Address ip;
      if (transportAddress.GetIpAddress(ip))
        transportAddress = OpalTransportAddress(ip, (WORD)port, OpalTransportAddress::UdpPrefix());
  }

  CreateSDPMediaFormats(tokens);

  return PTrue;
}


void SDPMediaDescription::CreateSDPMediaFormats(const PStringArray & tokens)
{
  // create the format list
  for (PINDEX i = 3; i < tokens.GetSize(); i++) {
    SDPMediaFormat * fmt = CreateSDPMediaFormat(tokens[i]);
    if (fmt != NULL) 
      formats.Append(fmt);
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
      SetTransportAddress(ParseConnectAddress(value, port));
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
  if (m_transportType != GetSDPTransportType()) {
    PTRACE(2, "SDP\tMedia session transport " << m_transportType << " not compatible with " << GetSDPTransportType());
    return false;
  }

  unsigned bw = GetBandwidth(SDPSessionDescription::TransportIndependentBandwidthType());
  if (bw == UINT_MAX) {
    bw = GetBandwidth(SDPSessionDescription::ApplicationSpecificBandwidthType());
    if (bw < UINT_MAX/1000)
      bw *= 1000;
  }

  SDPMediaFormatList::iterator format = formats.begin();
  while (format != formats.end()) {
    if (format->PostDecode(mediaFormats, bw))
      ++format;
    else
      formats.erase(format++);
  }

  return true;
}


void SDPMediaDescription::SetCryptoKeys(OpalMediaCryptoKeyList &)
{
  // Do nothing
}


OpalMediaCryptoKeyList SDPMediaDescription::GetCryptoKeys() const
{
  return OpalMediaCryptoKeyList();
}


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
    for (format = formats.begin(); format != formats.end(); ++format) {
      if (format->GetPayloadType() == pt)
        break;
    }
  }
  else {
    // Check for it a format name
    pos = params.Find(' ');
    PString encodingName = params.Left(pos);
    for (format = formats.begin(); format != formats.end(); ++format) {
      if (format->GetEncodingName() == encodingName)
        break;
    }
  }

  if (format == formats.end()) {
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
  for (SDPMediaFormatList::iterator format = formats.begin(); format != formats.end(); ++format) {
    if (!format->PreEncode())
      return false;
  }
  return true;
}


void SDPMediaDescription::Encode(const OpalTransportAddress & commonAddr, ostream & strm) const
{
  PString connectString;
  PIPSocket::Address commonIP, transportIP;
  if (transportAddress.GetIpAddress(transportIP) && commonAddr.GetIpAddress(commonIP) && commonIP != transportIP)
    connectString = GetConnectAddressString(transportAddress);

  //
  // if no media formats, then do not output the media header
  // this avoids displaying an empty media header with no payload types
  // when (for example) video has been disabled
  //
  if (formats.GetSize() == 0)
    return;

  PIPSocket::Address ip;
  WORD port = 0;
  transportAddress.GetIpAndPort(ip, port);

  /* output media header, note the order is important according to RFC!
     Must be icbka */
  strm << "m=" 
       << GetSDPMediaType() << ' '
       << port << ' '
       << GetSDPTransportType()
       << GetSDPPortList() << "\r\n";

  // If we have a port of zero, then shutting down SDP stream. No need for anything more
  if (port == 0)
    return;

  if (!connectString.IsEmpty())
    strm << "c=" << connectString << "\r\n";

  strm << m_bandwidth;
  OutputAttributes(strm);
}


OpalMediaFormatList SDPMediaDescription::GetMediaFormats() const
{
  OpalMediaFormatList list;

  for (SDPMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format) {
    OpalMediaFormat mediaFormat = format->GetMediaFormat();
    if (mediaFormat.IsValid())
      list += mediaFormat;
    else {
      PTRACE(2, "SIP\tRTP payload type " << format->GetPayloadType() 
             << ", name=" << format->GetEncodingName() << ", not matched to supported codecs");
    }
  }

  return list;
}


void SDPMediaDescription::AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat)
{
  formats.Append(sdpMediaFormat);
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

  for (SDPMediaFormatList::iterator format = formats.begin(); format != formats.end(); ++format) {
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

  SDPMediaFormat * sdpFormat = new SDPMediaFormat(*this, mediaFormat);

  ProcessMediaOptions(*sdpFormat, mediaFormat);

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


SDPMediaFormat * SDPDummyMediaDescription::CreateSDPMediaFormat(const PString & /*portString*/)
{
  return NULL;
}


PString SDPDummyMediaDescription::GetSDPPortList() const
{
  return ' ' + m_tokens[3];
}


//////////////////////////////////////////////////////////////////////////////

SDPCryptoSuite::SDPCryptoSuite(unsigned tag)
  : m_tag(tag)
{
  if (m_tag == 0)
    m_tag = 1;
}


bool SDPCryptoSuite::Decode(const PString & sdp)
{
  if (sdp.GetLength() < 7 || sdp[0] < '1' || sdp[0] > '2' || sdp[1] != ' ')
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
    if (!keyParams[kp].Split(':', method, info) || method != "inline" || info.IsEmpty())
      return false;

    PStringArray keyParts = info.Tokenise('|');
    m_keyParams.push_back(KeyParam(keyParts[0]));
    KeyParam & newKeyParam = m_keyParams.back();

    switch (keyParts.GetSize()) {
      case 3 :
        {
          PString idx, len;
          if (!keyParts[2].Split(':', idx, len))
            return false;
          if ((newKeyParam.m_mkiIndex = idx.AsUnsigned()) == 0)
            return false;
          if ((newKeyParam.m_mkiLength = len.AsUnsigned()) == 0)
            return false;
        }
        // Do next case

      case 2 :
        {
          PCaselessString lifetime = keyParts[1];
          if (lifetime.NumCompare("2^") == PObject::EqualTo)
            newKeyParam.m_lifetime = 1ULL << lifetime.Mid(2).AsUnsigned();
          else
            newKeyParam.m_lifetime = lifetime.AsUnsigned64();
          if (newKeyParam.m_lifetime == 0)
            return false;
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

  strm << "\r\n";
}


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


SDPMediaFormat * SDPRTPAVPMediaDescription::CreateSDPMediaFormat(const PString & portString)
{
  return new SDPMediaFormat(*this, (RTP_DataFrame::PayloadTypes)portString.AsUnsigned());
}


PString SDPRTPAVPMediaDescription::GetSDPPortList() const
{
  if (formats.IsEmpty())
    return " 127"; // Have to have SOMETHING

  PStringStream strm;

  // output RTP payload types
  for (SDPMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format)
    strm << ' ' << (int)format->GetPayloadType();

  return strm;
}


void SDPRTPAVPMediaDescription::OutputAttributes(ostream & strm) const
{
  // call ancestor
  SDPMediaDescription::OutputAttributes(strm);

  // output attributes for each payload type
  for (SDPMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format)
    strm << *format;

  if (m_enableFeedback)
    strm << "a=rtcp-fb:* fir pli tmmbr\r\n";

  for (PList<SDPCryptoSuite>::const_iterator crypto = m_cryptoSuites.begin(); crypto != m_cryptoSuites.end(); ++crypto)
    strm << *crypto;
}


void SDPRTPAVPMediaDescription::SetCryptoKeys(OpalMediaCryptoKeyList & cryptoKeys)
{
  m_cryptoSuites.RemoveAll();
  unsigned tag = 1;
  for (OpalMediaCryptoKeyList::iterator it = cryptoKeys.begin(); it != cryptoKeys.end(); ++it) {
    SDPCryptoSuite * cryptoSuite = new SDPCryptoSuite(tag);
    if (!cryptoSuite->SetKeyInfo(*it))
      delete cryptoSuite;
    else {
      m_cryptoSuites.Append(cryptoSuite);
      it->SetTag(cryptoSuite->GetTag());
      ++tag;
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

  if (attr *= "rtcp-fb") {
    if (value[0] == '*') {
      PString params = value.Mid(1).Trim();
      SDPMediaFormatList::iterator format;
      for (format = formats.begin(); format != formats.end(); ++format)
        format->SetRTCP_FB(params);
    }
    else {
      PString params = value;
      SDPMediaFormat * format = FindFormat(params);
      if (format != NULL)
        format->SetRTCP_FB(params);
    }
    return;
  }

  if (attr *= "crypto") {
    SDPCryptoSuite * cryptoSuite = new SDPCryptoSuite(0);
    if (cryptoSuite->Decode(value))
      m_cryptoSuites.Append(cryptoSuite);
    else {
      delete cryptoSuite;
      PTRACE(2, "SDP\tCannot decode SDP crypto attribute: \"" << value << '"');
    }
    return;
  }

  SDPMediaDescription::SetAttribute(attr, value);
}


//////////////////////////////////////////////////////////////////////////////

SDPAudioMediaDescription::SDPAudioMediaDescription(const OpalTransportAddress & address)
  : SDPRTPAVPMediaDescription(address, OpalMediaType::Audio())
  , m_offerPTime(false)
{
}


PString SDPAudioMediaDescription::GetSDPMediaType() const 
{ 
  return "audio"; 
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

  if (m_offerPTime) {
    unsigned ptime = 0;
    for (SDPMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format) {
      const OpalMediaFormat & mediaFormat = format->GetMediaFormat();
      if (mediaFormat.HasOption(OpalAudioFormat::TxFramesPerPacketOption())) {
        unsigned ptime1 = mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption()) * mediaFormat.GetFrameTime() / mediaFormat.GetTimeUnits();
        if (ptime < ptime1)
          ptime = ptime1;
      }
    }
    if (ptime > 0)
      strm << "a=ptime:" << ptime << "\r\n";
  }

  unsigned largestFrameTime = 0;
  unsigned maxptime = UINT_MAX;

  // output attributes for each payload type
  for (SDPMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format) {
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
    strm << "a=maxptime:" << maxptime << "\r\n";
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
    ptime = newTime;
    return;
  }

  if (attr *= "maxptime") {
    unsigned newTime = value.AsUnsigned();
    if (newTime < SDP_MIN_PTIME) {
      PTRACE(2, "SDP\tMalformed maxptime attribute value " << value);
      return;
    }
    maxptime = newTime;
    return;
  }

  return SDPRTPAVPMediaDescription::SetAttribute(attr, value);
}


//////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

SDPVideoMediaDescription::SDPVideoMediaDescription(const OpalTransportAddress & address)
  : SDPRTPAVPMediaDescription(address, OpalMediaType::Video())
{
}


PString SDPVideoMediaDescription::GetSDPMediaType() const 
{ 
  return "video"; 
}


static const char * const ContentRoleNames[] = { NULL, "slides", "main", "speaker", "sl" };


void SDPVideoMediaDescription::OutputAttributes(ostream & strm) const
{
  // call ancestor
  SDPRTPAVPMediaDescription::OutputAttributes(strm);

  for (SDPMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format) {
    PINDEX role = format->GetMediaFormat().GetOptionEnum(OpalVideoFormat::ContentRoleOption());
    if (role > 0) {
      strm << "a=content:" << ContentRoleNames[role] << "\r\n";
      break;
    }
  }
}


void SDPVideoMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  /* NOTE: must make sure anything passed through to a SDPFormat isntance does
           not require it to have an OpalMediaFormat yet, as that is not created
           until PostDecode() */

  if (attr *= "content") {
    PINDEX role = 0;
    PStringArray tokens = value.Tokenise(',');
    for (PINDEX i = 0; i < tokens.GetSize(); ++i) {
      for (role = PARRAYSIZE(ContentRoleNames)-1; role > 0; --role) {
        if (tokens[i] *= ContentRoleNames[role])
          break;
      }
      if (role > 0)
        break;
    }

    for (SDPMediaFormatList::iterator format = formats.begin(); format != formats.end(); ++format)
      format->GetWritableMediaFormat().SetOptionEnum(OpalVideoFormat::ContentRoleOption(), role);

    return;
  }

  return SDPRTPAVPMediaDescription::SetAttribute(attr, value);
}



bool SDPVideoMediaDescription::PreEncode()
{
  if (!SDPRTPAVPMediaDescription::PreEncode())
    return false;

  /* Even though the bandwidth parameter COULD be used for audio, we don't
     do it as it has very limited use. Variable bit rate audio codecs are
     not common, and usually there is an fmtp value to select the different
     bit rates. So, as it might cause interoperabity difficulties with the
     dumber implementations out there we just don't.

     As per RFC3890 we set both AS and TIAS parameters.
  */
  for (SDPMediaFormatList::iterator format = formats.begin(); format != formats.end(); ++format) {
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
    unsigned bw = mediaFormat.GetBandwidth();
    m_bandwidth.SetMax(SDPSessionDescription::TransportIndependentBandwidthType(), bw);
    m_bandwidth.SetMax(SDPSessionDescription::ApplicationSpecificBandwidthType(), (bw+999)/1000);
  }

  return true;
}

#endif // OPAL_VIDEO


//////////////////////////////////////////////////////////////////////////////

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
  return "application"; 
}


SDPMediaFormat * SDPApplicationMediaDescription::CreateSDPMediaFormat(const PString & portString)
{
  return new SDPMediaFormat(*this, RTP_DataFrame::DynamicBase, portString);
}


PString SDPApplicationMediaDescription::GetSDPPortList() const
{
  if (formats.IsEmpty())
    return " na"; // Have to have SOMETHING

  PStringStream str;

  // output encoding names for non RTP
  SDPMediaFormatList::const_iterator format;
  for (format = formats.begin(); format != formats.end(); ++format)
    str << ' ' << format->GetEncodingName();

  return str;
}

//////////////////////////////////////////////////////////////////////////////

SDPSessionDescription::SDPSessionDescription(time_t sessionId, unsigned version, const OpalTransportAddress & address)
  : sessionName(SIP_DEFAULT_SESSION_NAME)
  , ownerUsername('-')
  , ownerSessionId(sessionId)
  , ownerVersion(version)
  , ownerAddress(address)
  , defaultConnectAddress(address)
{
  protocolVersion  = 0;
}


void SDPSessionDescription::PrintOn(ostream & str) const
{
  /* encode mandatory session information, note the order is important according to RFC!
     Must be vosiuepcbzkatrm and within the m it is icbka */
  str << "v=" << protocolVersion << "\r\n"
         "o=" << ownerUsername << ' '
      << ownerSessionId << ' '
      << ownerVersion << ' '
      << GetConnectAddressString(ownerAddress)
      << "\r\n"
         "s=" << sessionName << "\r\n";

  if (!defaultConnectAddress.IsEmpty())
    str << "c=" << GetConnectAddressString(defaultConnectAddress) << "\r\n";
  
  str << m_bandwidth
      << "t=" << "0 0" << "\r\n";

  OutputAttributes(str);

  // encode media session information
  for (PINDEX i = 0; i < mediaDescriptions.GetSize(); i++) {
    if (mediaDescriptions[i].PreEncode())
      mediaDescriptions[i].Encode(defaultConnectAddress, str);
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
            PStringArray tokens = value.Tokenise(" ");
            if (tokens.GetSize() < 4) {
              PTRACE(1, "SDP\tMedia session has only " << tokens.GetSize() << " elements");
            }
            else if ((mediaType = OpalMediaType::GetMediaTypeFromSDP(tokens[0], tokens[2])).empty()) {
              PTRACE(1, "SDP\tUnknown SDP media type " << tokens[0] << '|' << tokens[2]);
            }
            else if ((defn = mediaType.GetDefinition()) == NULL) {
              PTRACE(1, "SDP\tNo definition for SDP media type " << tokens[0]);
            }
            else if ((currentMedia = defn->CreateSDPMediaDescription(defaultConnectAddress, NULL)) == NULL) {
              PTRACE(1, "SDP\tCould not create SDP media description for SDP media type " << tokens[0]);
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


void SDPSessionDescription::ParseOwner(const PString & str)
{
  PStringArray tokens = str.Tokenise(" ");

  if (tokens.GetSize() != 6) {
    PTRACE(2, "SDP\tOrigin has incorrect number of elements (" << tokens.GetSize() << ')');
  }
  else {
    ownerUsername    = tokens[0];
    ownerSessionId   = tokens[1].AsUnsigned();
    ownerVersion     = tokens[2].AsUnsigned();
    ownerAddress = defaultConnectAddress = ParseConnectAddress(tokens, 3);
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
