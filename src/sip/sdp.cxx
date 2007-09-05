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
 * $Log: sdp.cxx,v $
 * Revision 1.2055  2007/09/05 05:39:11  csoutheren
 * Fix problem with parsing a= lines with no attribute values
 *
 * Revision 2.53  2007/09/05 04:04:49  csoutheren
 * Fixed spelling mistakes
 *
 * Revision 2.52  2007/09/04 08:27:45  rjongbloed
 * Added ptime and maxptime SDP attributes.
 *
 * Revision 2.51  2007/08/31 05:28:16  rjongbloed
 * Allow for boolean FMTP parameters to not include the =1 part, its mere presence is a "true".
 *
 * Revision 2.50  2007/07/23 06:34:19  csoutheren
 * Stop re-creation of new RTP sessions after SIP authentication fails
 * Do not create video RTP sessions if no video media formats
 *
 * Revision 2.49  2007/06/22 05:41:47  rjongbloed
 * Major codec API update:
 *   Automatically map OpalMediaOptions to SIP/SDP FMTP parameters.
 *   Automatically map OpalMediaOptions to H.245 Generic Capability parameters.
 *   Largely removed need to distinguish between SIP and H.323 codecs.
 *   New mechanism for setting OpalMediaOptions from within a plug in.
 *
 * Revision 2.48  2007/05/28 08:38:01  csoutheren
 * Don't use deleted media session info when parsing fails
 *
 * Revision 2.47  2007/05/04 15:24:20  vfrolov
 * Fixed Assertion fail in SetAttribute() if empty formats list
 *
 * Revision 2.46  2007/04/21 13:49:15  dsandras
 * Allow a different connect address per media description. Fixes
 * Ekiga bug #430870.
 *
 * Revision 2.45  2007/04/10 05:15:54  rjongbloed
 * Fixed issue with use of static C string variables in DLL environment,
 *   must use functional interface for correct initialisation.
 *
 * Revision 2.44  2007/04/04 02:12:02  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.43  2007/03/13 02:17:49  csoutheren
 * Remove warnings/errors when compiling with various turned off
 *
 * Revision 2.42  2007/03/13 00:33:11  csoutheren
 * Simple but messy changes to allow compile time removal of protocol
 * options such as H.450 and H.460
 * Fix MakeConnection overrides
 *
 * Revision 2.41  2006/11/21 01:01:00  csoutheren
 * Ensure SDP only uses codecs that are valid for SIP
 *
 * Revision 2.40  2006/08/20 03:45:55  csoutheren
 * Add OpalMediaFormat::IsValidForProtocol to allow plugin codecs to be enabled only for certain protocols
 * rather than relying on the presence of the IANA rtp encoding name field
 *
 * Revision 2.39  2006/08/15 23:52:55  csoutheren
 * Ensure codecs with same name but different clock rate get different payload types
 *
 * Revision 2.38  2006/07/28 10:41:51  rjongbloed
 * Fixed DevStudio 2005 warnings on time_t conversions.
 *
 * Revision 2.37  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.36  2006/04/30 09:58:44  csoutheren
 * Added IPV6 handlng to SDP
 *
 * Revision 2.35  2006/04/23 20:12:52  dsandras
 * The RFC tells that the SDP answer SHOULD have the same payload type than the
 * SDP offer. Added rtpmap support to allow this. Fixes problems with Asterisk,
 * and Ekiga report #337456.
 *
 * Revision 2.34  2006/04/22 09:32:18  dsandras
 * Fixed compilation error with GCC on Linux.
 *
 * Revision 2.33  2006/04/21 14:36:51  hfriederich
 * Adding ability to parse and transmit simple bandwidth information
 *
 * Revision 2.32  2006/03/29 23:53:54  csoutheren
 * Make sure OpalTransportAddresses that are parsed from SDP are always udp$ and not
 *  tcp$
 *
 * Revision 2.31  2006/03/20 00:41:43  csoutheren
 * Fixed typo in last submit
 *
 * Revision 2.30  2006/03/20 00:20:15  csoutheren
 * Fixed problem with output empty image media formats
 *
 * Revision 2.29  2006/03/08 10:59:03  csoutheren
 * Applied patch #1444783 - Add 'image' SDP meda type and 'udptl' transport protocol
 * Thanks to Drazen Dimoti
 *
 * Revision 2.28.2.6  2006/04/30 13:54:14  csoutheren
 * Added handling for IPV6
 *
 * Revision 2.28.2.5  2006/04/25 01:06:22  csoutheren
 * Allow SIP-only codecs
 *
 * Revision 2.28.2.4  2006/04/19 07:52:30  csoutheren
 * Add ability to have SIP-only and H.323-only codecs, and implement for H.261
 *
 * Revision 2.28.2.3  2006/04/06 05:33:09  csoutheren
 * Backports from CVS head up to Plugin_Merge2
 *
 * Revision 2.28.2.2  2006/03/20 02:25:27  csoutheren
 * Backports from CVS head
 *
 * Revision 2.28.2.1  2006/03/16 07:06:00  csoutheren
 * Initial support for audio plugins
 *
 * Revision 2.32  2006/03/29 23:53:54  csoutheren
 * Make sure OpalTransportAddresses that are parsed from SDP are always udp$ and not
 *  tcp$
 *
 * Revision 2.31  2006/03/20 00:41:43  csoutheren
 * Fixed typo in last submit
 *
 * Revision 2.30  2006/03/20 00:20:15  csoutheren
 * Fixed problem with output empty image media formats
 *
 * Revision 2.29  2006/03/08 10:59:03  csoutheren
 * Applied patch #1444783 - Add 'image' SDP meda type and 'udptl' transport protocol
 * Thanks to Drazen Dimoti
 *
 * Revision 2.28  2006/02/08 04:51:58  csoutheren
 * Don't output media description when no formats to output
 *
 * Revision 2.27  2006/02/02 07:02:58  csoutheren
 * Added RTP payload map to transcoders and connections to allow remote SIP endpoints
 * to change the payload type used for outgoing RTP.
 *
 * Revision 2.26  2006/01/02 11:28:07  dsandras
 * Some documentation. Various code cleanups to prevent duplicate code.
 *
 * Revision 2.25  2005/12/29 16:23:38  dsandras
 * Media formats with different clock rates are not identical.
 *
 * Revision 2.24  2005/12/27 20:51:20  dsandras
 * Added clockRate parameter support.
 *
 * Revision 2.23  2005/10/04 18:31:01  dsandras
 * Allow SetFMTP and GetFMTP to work with any option set for a=fmtp:.
 *
 * Revision 2.22  2005/09/15 17:01:08  dsandras
 * Added support for the direction attributes in the audio & video media descriptions and in the session.
 *
 * Revision 2.21  2005/08/25 18:50:55  dsandras
 * Added support for clockrate based on the media format.
 *
 * Revision 2.20  2005/07/18 01:19:37  csoutheren
 * Fixed proxy with FWD rejecting call because of the order of the SDP fields
 * I can't see any requirement for ordering in the RFC, so this is probably a
 * bug in the FWD proxy
 *
 * Revision 2.19  2005/07/14 08:52:19  csoutheren
 * Modified to output media desscription specific connection address if needed
 *
 * Revision 2.18  2005/04/28 20:22:54  dsandras
 * Applied big sanity patch for SIP thanks to Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.17  2005/04/10 21:19:38  dsandras
 * Added support to set / get the stream direction in a SDP.
 *
 * Revision 2.16  2004/10/24 10:45:19  rjongbloed
 * Back out change of strcasecmp to strcmp for WinCE
 *
 * Revision 2.15  2004/10/23 11:43:05  ykiryanov
 * Added ifdef _WIN32_WCE for PocketPC 2003 SDK port
 *
 * Revision 2.14  2004/04/27 07:22:40  rjongbloed
 * Adjusted some logging
 *
 * Revision 2.13  2004/03/25 11:51:12  rjongbloed
 * Changed PCM-16 from IllegalPayloadType to MaxPayloadType to avoid problems
 *   in other parts of the code.
 * Changed to hame m= line for every codec, including "known" ones, thanks Ted Szoczei
 *
 * Revision 2.12  2004/03/22 11:32:42  rjongbloed
 * Added new codec type for 16 bit Linear PCM as must distinguish between the internal
 *   format used by such things as the sound card and the RTP payload format which
 *   is always big endian.
 *
 * Revision 2.11  2004/02/09 13:13:02  rjongbloed
 * Added debug string output for media type enum.
 *
 * Revision 2.10  2004/02/07 02:18:19  rjongbloed
 * Improved searching for media format to use payload type AND the encoding name.
 *
 * Revision 2.9  2004/01/08 22:23:23  csoutheren
 * Fixed problem with not using session ID when constructing SDP lists
 *
 * Revision 2.8  2004/01/08 22:20:43  csoutheren
 * Fixed problem with not using session ID when constructing SDP lists
 *
 * Revision 2.7  2003/12/15 11:56:17  rjongbloed
 * Applied numerous bug fixes, thank you very much Ted Szoczei
 *
 * Revision 2.6  2003/03/17 22:31:35  robertj
 * Fixed warnings
 *
 * Revision 2.5  2002/06/16 02:22:49  robertj
 * Fixed memory leak of RFC2833 ordinals, thanks Ted Szoczei
 *
 * Revision 2.4  2002/02/19 07:51:37  robertj
 * Added OpalRFC2833 as a OpalMediaFormat variable.
 * Fixed encoding name being used correctly in SDP media list.
 *
 * Revision 2.3  2002/02/13 02:28:59  robertj
 * Normalised some function names.
 * Fixed incorrect port number usage stopping audio in one direction.
 * Added code to put individual c lines in each media description.
 *
 * Revision 2.2  2002/02/11 07:35:35  robertj
 * Changed SDP to use OpalTransport for hosts instead of IP addresses/ports
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sdp.h"
#endif

#include <sip/sdp.h>

#include <ptlib/socket.h>
#include <opal/transports.h>


#define SIP_DEFAULT_SESSION_NAME  "Opal SIP Session"
#define  SDP_MEDIA_TRANSPORT       "RTP/AVP"
#define  SDP_MEDIA_TRANSPORT_UDPTL "udptl"

#define new PNEW


/////////////////////////////////////////////////////////

static OpalTransportAddress ParseConnectAddress(const PStringArray & tokens, PINDEX offset)
{
  if (tokens.GetSize() == offset+3) {
    if (tokens[offset] *= "IN") {
      if (
        (tokens[offset+1] *= "IP4")
#if P_HAS_IPV6
        || (tokens[offset+1] *= "IP6")
#endif
        )
        return OpalTransportAddress(tokens[offset+2], 0, "udp");
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


static OpalTransportAddress ParseConnectAddress(const PString & str)
{
  PStringArray tokens = str.Tokenise(' ');
  return ParseConnectAddress(tokens, 0);
}


static PString GetConnectAddressString(const OpalTransportAddress & address)
{
  PStringStream str;

  PIPSocket::Address ip;
  if (address != 0 && address.GetIpAddress(ip))
    str << "IN IP" << ip.GetVersion() << ' ' << ip;
  else
    str << "IN IP4 0.0.0.0";

  return str;
}


/////////////////////////////////////////////////////////

SDPMediaFormat::SDPMediaFormat(RTP_DataFrame::PayloadTypes pt, const char * _name)
  : payloadType(pt),
    clockRate(0),
    encodingName(_name),
    nteSet(TRUE)
{
  if (encodingName == OpalRFC2833.GetEncodingName())
    AddNTEString("0-15,32-49");
#if OPAL_T38FAX
  else if (encodingName == OpalCiscoNSE.GetEncodingName())
    AddNSEString("192,193");
#endif

  GetMediaFormat(); // Initialise, if possible
}


SDPMediaFormat::SDPMediaFormat(const OpalMediaFormat & fmt,
                               RTP_DataFrame::PayloadTypes pt,
                               const char * nxeString)
  : mediaFormat(fmt),
    payloadType(pt),
    clockRate(fmt.GetClockRate()),
    encodingName(fmt.GetEncodingName()),
    nteSet(TRUE)
{
  if (nxeString != NULL) {
#if OPAL_T38FAX
    if (encodingName *= "nse")
      AddNSEString(nxeString);
    else
#endif
      AddNTEString(nxeString);
  }
}


void SDPMediaFormat::SetFMTP(const PString & str)
{
  if (str.IsEmpty())
    return;

  if (encodingName == OpalRFC2833.GetEncodingName()) {
    nteSet.RemoveAll();
    AddNTEString(str);
    return;
  }

#if OPAL_T38FAX
  else if (encodingName == OpalCiscoNSE.GetEncodingName()) {
    nseSet.RemoveAll();
    AddNSEString(str);
    return;
  }
#endif

  fmtp = str;
  if (GetMediaFormat().IsEmpty()) // Use GetMediaFormat() to force creation of member
    return;

  // Fill in the default values for (possibly) missing FMTP options
  PINDEX i;
  for (i = 0; i < mediaFormat.GetOptionCount(); i++) {
    OpalMediaOption & option = const_cast<OpalMediaOption &>(mediaFormat.GetOption(i));
    if (!option.GetFMTPName().IsEmpty() && !option.GetFMTPDefault().IsEmpty())
      option.FromString(option.GetFMTPDefault());
  }

  // See if standard format OPT=VAL;OPT=VAL
  if (str.FindOneOf(";=") == P_MAX_INDEX) {
    // Nope, just save the whole string as is
    mediaFormat.SetOptionString("FMTP", str);
    return;
  }

  // Parse the string for option names and values OPT=VAL;OPT=VAL
  PINDEX sep1prev = 0;
  do {
    PINDEX sep1next = str.Find(';', sep1prev);
    if (sep1next == P_MAX_INDEX)
      sep1next--; // Implicit assumption string is not a couple of gigabytes long ...

    PINDEX sep2pos = str.Find('=', sep1prev);
    if (sep2pos > sep1next)
      sep2pos = sep1next;

    PCaselessString key = str(sep1prev, sep2pos-1).Trim();
    if (key.IsEmpty()) {
      PTRACE(2, "SDP\tBadly formed FMTP parameter \"" << str << '"');
      break;
    }

    OpalMediaOption * option = mediaFormat.FindOption(key);
    if (option == NULL || key != option->GetFMTPName()) {
      for (i = 0; i < mediaFormat.GetOptionCount(); i++) {
        if (key == mediaFormat.GetOption(i).GetFMTPName()) {
          option = const_cast<OpalMediaOption *>(&mediaFormat.GetOption(i));
          break;
        }
      }
    }
    if (option != NULL) {
      PString value = str(sep2pos+1, sep1next-1).Trim();
      if (value.IsEmpty())
        value = "1"; // Assume it is a boolean
      if (!option->FromString(value)) {
        PTRACE(2, "SDP\tCould not set FMTP parameter \"" << key << "\" to value \"" << value << '"');
      }
    }

    sep1prev = sep1next+1;
  } while (sep1prev != P_MAX_INDEX);
}


PString SDPMediaFormat::GetFMTP() const
{
  if (encodingName == OpalRFC2833.GetEncodingName())
    return GetNTEString();

#if OPAL_T38FAX
  if (encodingName == OpalCiscoNSE.GetEncodingName())
    return GetNSEString();
#endif

  if (GetMediaFormat().IsEmpty()) // Use GetMediaFormat() to force creation of member
    return fmtp;

  PString str = mediaFormat.GetOptionString("FMTP");
  if (!str.IsEmpty())
    return str;

  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    const PString & name = option.GetFMTPName();
    if (!name.IsEmpty()) {
      PString value = option.AsString();
      if (value != option.GetFMTPDefault()) {
        if (!str.IsEmpty())
          str += ';';
        str += name + '=' + value;
      }
    }
  }

  return !str ? str : fmtp;
}


PString SDPMediaFormat::GetNTEString() const
{
  return GetNXEString(nteSet);
}

void SDPMediaFormat::AddNTEString(const PString & str)
{
  AddNXEString(nteSet, str);
}

void SDPMediaFormat::AddNTEToken(const PString & ostr)
{
  AddNXEToken(nteSet, ostr);
}

#if OPAL_T38FAX

PString SDPMediaFormat::GetNSEString() const
{
  return GetNXEString(nseSet);
}

void SDPMediaFormat::AddNSEString(const PString & str)
{
  AddNXEString(nseSet, str);
}

void SDPMediaFormat::AddNSEToken(const PString & ostr)
{
  AddNXEToken(nseSet, ostr);
}

#endif

PString SDPMediaFormat::GetNXEString(POrdinalSet & nxeSet) const
{
  PString str;
  PINDEX i = 0;
  while (i < 255) {
    if (!nxeSet.Contains(POrdinalKey(i)))
      i++;
    else {
      PINDEX start = i++;
      while (nxeSet.Contains(POrdinalKey(i)))
        i++;
      if (!str.IsEmpty())
        str += ",";
      str += PString(PString::Unsigned, start);
      if (i > start+1)
        str += PString('-') + PString(PString::Unsigned, i-1);
    }
  }

  return str;
}


void SDPMediaFormat::AddNXEString(POrdinalSet & nxeSet, const PString & str)
{
  PStringArray tokens = str.Tokenise(",", FALSE);
  PINDEX i;
  for (i = 0; i < tokens.GetSize(); i++)
    AddNXEToken(nxeSet, tokens[i]);
}


void SDPMediaFormat::AddNXEToken(POrdinalSet & nxeSet, const PString & ostr)
{
  PString str = ostr.Trim();
  if (str[0] == ',')
    str = str.Mid(1);
  if (str.Right(1) == ",")
    str = str.Left(str.GetLength()-1);
  PINDEX pos = str.Find('-');
  if (pos == P_MAX_INDEX)
    nxeSet.Include(new POrdinalKey(str.AsInteger()));
  else {
    PINDEX from = str.Left(pos).AsInteger();
    PINDEX to   = str.Mid(pos+1).AsInteger();
    while (from <= to)
      nxeSet.Include(new POrdinalKey(from++));
  }
}


void SDPMediaFormat::PrintOn(ostream & strm) const
{
  PAssert(!encodingName.IsEmpty(), "SDPAudioMediaFormat encoding name is empty");

  strm << "a=rtpmap:" << (int)payloadType << ' ' << encodingName << '/' << clockRate;
  if (!parameters.IsEmpty())
    strm << '/' << parameters;
  strm << "\r\n";

  PString fmtpString = GetFMTP();
  if (!fmtpString.IsEmpty())
    strm << "a=fmtp:" << (int)payloadType << ' ' << fmtpString << "\r\n";
}


const OpalMediaFormat & SDPMediaFormat::GetMediaFormat() const
{
  if (mediaFormat.IsEmpty()) {
    mediaFormat = OpalMediaFormat(payloadType, clockRate, encodingName, "sip");
  }
  return mediaFormat;
}


void SDPMediaFormat::SetPacketTime(const PString & optionName, unsigned ptime)
{
  if (mediaFormat.HasOption(optionName)) {
    unsigned frameTime = mediaFormat.GetFrameTime();
    unsigned newCount = (ptime*mediaFormat.GetTimeUnits()+frameTime-1)/frameTime;
    mediaFormat.SetOptionInteger(optionName, newCount);
    PTRACE(4, "SDP\tMedia format " << optionName << " set to " << newCount << " packets from " << ptime << " milliseconds");
  }
}


BOOL SDPMediaDescription::SetTransportAddress(const OpalTransportAddress &t)
{
  PIPSocket::Address ip;
  WORD port = 0;
  if (transportAddress.GetIpAndPort(ip, port)) {
    transportAddress = OpalTransportAddress(t, port);
    return TRUE;
  }
  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

#if PTRACING
ostream & operator<<(ostream & out, SDPMediaDescription::MediaType type)
{
  static const char * const MediaTypeNames[SDPMediaDescription::NumMediaTypes] = {
    "Audio", "Video", "Application", "Image", "Unknown"
  };

  if (type < PARRAYSIZE(MediaTypeNames) && MediaTypeNames[type] != NULL)
    out << MediaTypeNames[type];
  else
    out << "MediaTypes<" << (int)type << '>';

  return out;
}
#endif

SDPMediaDescription::SDPMediaDescription(const OpalTransportAddress & address, MediaType _mediaType)
  : mediaType(_mediaType),
    transportAddress(address)
{
  switch (mediaType) {
    case Audio:
      media = "audio";
      break;
    case Video:
      media = "video";
      break;
    case Image:
      media = "image";
      break;
    default:
      break;
  }

  transport = (mediaType == Image) ? SDP_MEDIA_TRANSPORT_UDPTL : SDP_MEDIA_TRANSPORT;
  direction = Undefined;
}


BOOL SDPMediaDescription::Decode(const PString & str)
{
  PStringArray tokens = str.Tokenise(" ");

  if (tokens.GetSize() < 4) {
    PTRACE(1, "SDP\tMedia session has only " << tokens.GetSize() << " elements");
    return FALSE;
  }

  media = tokens[0];
  if (media == "video")
    mediaType = Video;
  else if (media == "audio")
    mediaType = Audio;
  else if (media == "image")
    mediaType = Image;
  else {
    PTRACE(1, "SDP\tUnknown media type " << media);
    mediaType = Unknown;
  }

  PString portStr  = tokens[1];
  transport        = tokens[2];

  // parse the port and port count
  PINDEX pos = portStr.Find('/');
  if (pos == P_MAX_INDEX) 
    portCount = 1;
  else {
    PTRACE(3, "SDP\tMedia header contains port count - " << portStr);
    portCount = (WORD)portStr.Mid(pos+1).AsUnsigned();
    portStr   = portStr.Left(pos);
  }
  unsigned port = portStr.AsUnsigned();

  if (port == 0) 
    PTRACE(4, "SDP\tIgnoring media session with port=0");
  else {
    PTRACE(4, "SDP\tMedia session port=" << port);

    if ((transport != SDP_MEDIA_TRANSPORT) && (transport != SDP_MEDIA_TRANSPORT_UDPTL)) {
      PTRACE(2, "SDP\tMedia session has only " << tokens.GetSize() << " elements");
      return FALSE;
    }

    PIPSocket::Address ip;
    transportAddress.GetIpAddress(ip);
    transportAddress = OpalTransportAddress(ip, (WORD)port);

    // create the format list
    PINDEX i;
    for (i = 3; i < tokens.GetSize(); i++) {
      if (mediaType == Image)
        formats.Append(new SDPMediaFormat(RTP_DataFrame::DynamicBase, tokens[i]));
      else
        formats.Append(new SDPMediaFormat((RTP_DataFrame::PayloadTypes)tokens[i].AsUnsigned()));
    }
  }

  return TRUE;
}


void SDPMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  // get the attribute type
  if (attr *= "sendonly") {
    direction = SendOnly;
    return;
  }

  if (attr *= "recvonly") {
    direction = RecvOnly;
    return;
  }

  if (attr *= "sendrecv") {
    direction = SendRecv;
    return;
  }

  if (attr *= "inactive") {
    direction = Inactive;
    return;
  }

#if OPAL_T38FAX
  if (attr.Left(3) *= "t38") {
    t38Attributes.SetAt(attr, value);
    return;
  }
#endif

  // handle rtpmap attribute
  if (attr *= "rtpmap") {
    PString params = value;
    SDPMediaFormat * format = FindFormat(params);
    if (format != NULL) {
      PStringArray tokens = value.Tokenise('/');
      if (tokens.GetSize() < 2) {
        PTRACE(2, "SDP\tMalformed rtpmap attribute for " << format->GetEncodingName());
        return;
      }

      format->SetEncodingName(tokens[0]);
      format->SetClockRate(tokens[1].AsUnsigned());
      if (tokens.GetSize() > 2)
        format->SetParameters(tokens[2]);
    }
    return;
  }

  // handle fmtp attributes
  if (attr *= "fmtp") {
    PString params = value;
    SDPMediaFormat * format = FindFormat(params);
    if (format != NULL)
      format->SetFMTP(params);
    return;
  }

  if (attr *= "ptime") {
    SetPacketTime(OpalAudioFormat::TxFramesPerPacketOption(), value);
    return;
  }

  if (attr *= "maxptime") {
    SetPacketTime(OpalAudioFormat::RxFramesPerPacketOption(), value);
    return;
  }

  // unknown attriutes
  PTRACE(2, "SDP\tUnknown media attribute " << attr);
  return;
}


SDPMediaFormat * SDPMediaDescription::FindFormat(PString & str) const
{
  // extract the RTP payload type
  PINDEX pos = str.FindSpan("0123456789");
  if (str.GetLength() > pos && !isspace(str[pos])) {
    PTRACE(2, "SDP\tMalformed media attribute requiring format " << str);
    return NULL;
  }

  RTP_DataFrame::PayloadTypes pt = (RTP_DataFrame::PayloadTypes)str.Left(pos).AsUnsigned();

  // extract the attribute argument
  while (isspace(str[pos+1]))
    pos++;
  str.Delete(0, pos);

  // find the format that matches the payload type
  for (PINDEX fmt = 0; fmt < formats.GetSize(); fmt++) {
    if (formats[fmt].GetPayloadType() == pt)
      return &formats[fmt];
  }

  PTRACE(2, "SDP\tMedia attribute found for unknown RTP type " << pt);
  return NULL;
}


void SDPMediaDescription::SetPacketTime(const PString & optionName, const PString & value)
{
  unsigned newTime = value.AsUnsigned();
  if (newTime < 10) {
    PTRACE(2, "SDP\tMalformed (max)ptime attribute value " << value);
    return;
  }

  for (PINDEX i = 0; i < formats.GetSize(); i++)
   formats[i].SetPacketTime(optionName, newTime);
}


void SDPMediaDescription::PrintOn(const OpalTransportAddress & commonAddr, ostream & str) const
{
  PIPSocket::Address commonIP;
  commonAddr.GetIpAddress(commonIP);

  PIPSocket::Address transportIP;
  transportAddress.GetIpAddress(transportIP);

  PString connectString;
  if (commonIP != transportIP)
    connectString = GetConnectAddressString(transportAddress);

  PrintOn(str, connectString);
}

void SDPMediaDescription::PrintOn(ostream & str) const
{
  PIPSocket::Address ip;
  transportAddress.GetIpAddress(ip);
  PrintOn(str, GetConnectAddressString(transportAddress));
}

void SDPMediaDescription::PrintOn(ostream & str, const PString & connectString) const
{
  //
  // if no media formats, then do not output the media header
  // this avoids displaying an empty media header with no payload types
  // when (for example) video has been disabled
  //
  if (formats.GetSize() == 0)
    return;

  PIPSocket::Address ip;
  WORD port;
  transportAddress.GetIpAndPort(ip, port);

  // output media header
  str << "m=" 
      << media << " "
      << port << " "
      << transport;

  if (transport == SDP_MEDIA_TRANSPORT) {

    // output RTP payload types
    PINDEX i;
    for (i = 0; i < formats.GetSize(); i++)
      str << ' ' << (int)formats[i].GetPayloadType();
    str << "\r\n";

    // Fill in the ptime  as maximum tx packets of all media formats
    // and maxptime as minimum rx packets of all media formats
    unsigned ptime = 0;
    unsigned maxptime = UINT_MAX;

    // output attributes for each payload type
    for (i = 0; i < formats.GetSize(); i++) {
      str << formats[i];

      const OpalMediaFormat & mediaFormat = formats[i].GetMediaFormat();
      if (mediaFormat.HasOption(OpalAudioFormat::TxFramesPerPacketOption())) {
        unsigned ptime1 = mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption())*mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits();
        if (ptime < ptime1)
          ptime = ptime1;
      }
      if (mediaFormat.HasOption(OpalAudioFormat::RxFramesPerPacketOption())) {
        unsigned maxptime1 = mediaFormat.GetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption())*mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits();
        if (maxptime > maxptime1)
          maxptime = maxptime1;
      }
    }

    if (ptime > 0)
      str << "a=ptime:" << ptime << "\r\n";
    if (maxptime < UINT_MAX)
      str << "a=maxptime:" << maxptime << "\r\n";

    // media format direction
    switch (direction) {
      case SDPMediaDescription::RecvOnly:
        str << "a=recvonly" << "\r\n";
        break;
      case SDPMediaDescription::SendOnly:
        str << "a=sendonly" << "\r\n";
        break;
      case SDPMediaDescription::SendRecv:
        str << "a=sendrecv" << "\r\n";
        break;
      case SDPMediaDescription::Inactive:
        str << "a=inactive" << "\r\n";
        break;
      default:
        break;
    }
  }

#if OPAL_T38FAX
  else if (transport == SDP_MEDIA_TRANSPORT_UDPTL) {
    PINDEX i;
    for (i = 0; i < formats.GetSize(); i++)
      str << ' ' << formats[i].GetEncodingName();
    str << "\r\n";

    // output options
    for (i = 0; i < t38Attributes.GetSize(); i++) 
      str << "a=" << t38Attributes.GetKeyAt(i) << ":" << t38Attributes.GetDataAt(i) << "\r\n";
  }
#endif

  else {
    PINDEX i;
    for (i = 0; i < formats.GetSize(); i++)
      str << ' ' << formats[i].GetEncodingName();
    str << "\r\n";
  }

  if (!connectString.IsEmpty())
    str << "c=" << connectString << "\r\n";
}


OpalMediaFormatList SDPMediaDescription::GetMediaFormats(unsigned sessionID) const
{
  OpalMediaFormatList list;

  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++) {
    OpalMediaFormat opalFormat = formats[i].GetMediaFormat();
    if (opalFormat.IsEmpty())
      PTRACE(2, "SIP\tRTP payload type " << formats[i].GetPayloadType() << " not matched to audio codec");
    else {
      if (opalFormat.GetDefaultSessionID() == sessionID && 
          opalFormat.IsValidForProtocol("sip") &&
          opalFormat.GetEncodingName() != NULL) {
        PTRACE(3, "SIP\tRTP payload type " << formats[i].GetPayloadType() << " matched to codec " << opalFormat);
        list += opalFormat;
      }
    }
  }

  return list;
}

void SDPMediaDescription::CreateRTPMap(unsigned sessionID, RTP_DataFrame::PayloadMapType & map) const
{
  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++) {
    OpalMediaFormat opalFormat = formats[i].GetMediaFormat();
    if (!opalFormat.IsEmpty() && 
         opalFormat.GetDefaultSessionID() == sessionID &&
         opalFormat.GetPayloadType() != formats[i].GetPayloadType()) {
      map.insert(RTP_DataFrame::PayloadMapType::value_type(opalFormat.GetPayloadType(), formats[i].GetPayloadType()));
      PTRACE(3, "SDP\tAdding RTP translation from " << opalFormat.GetPayloadType() << " to " << formats[i].GetPayloadType());
    }
  }
}

void SDPMediaDescription::AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat)
{
  formats.Append(sdpMediaFormat);
}


void SDPMediaDescription::AddMediaFormat(const OpalMediaFormat & mediaFormat, const RTP_DataFrame::PayloadMapType & map)
{
  const char * encodingName = mediaFormat.GetEncodingName();
  if (!mediaFormat.IsValidForProtocol("sip") || encodingName == NULL || *encodingName == '\0')
    return;

  RTP_DataFrame::PayloadTypes payloadType = mediaFormat.GetPayloadType();
  if (map.size() != 0) {
    RTP_DataFrame::PayloadMapType::const_iterator r = map.find(payloadType);
    if (r != map.end())
      payloadType = r->second;
  }

  if (payloadType >= RTP_DataFrame::MaxPayloadType)
    return;

  unsigned clockRate = mediaFormat.GetClockRate();

  for (PINDEX i = 0; i < formats.GetSize(); i++) {
    if (formats[i].GetPayloadType() == payloadType ||
        ((formats[i].GetEncodingName() *= encodingName) && formats[i].GetClockRate() == clockRate)
        )
      return;
  }

  SDPMediaFormat * sdpFormat = new SDPMediaFormat(mediaFormat, payloadType);

#if OPAL_T38FAX
  if (mediaFormat.GetDefaultSessionID() == OpalMediaFormat::DefaultDataSessionID) {
    for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); ++i) {
      const OpalMediaOption & option = mediaFormat.GetOption(i);
      if (option.GetName().Left(3) *= "t38") 
        t38Attributes.SetAt(option.GetName(), option.AsString());
    }
  }
#endif  

  AddSDPMediaFormat(sdpFormat);
}


void SDPMediaDescription::AddMediaFormats(const OpalMediaFormatList & mediaFormats, unsigned session, const RTP_DataFrame::PayloadMapType & map)
{
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++) {
    OpalMediaFormat & mediaFormat = mediaFormats[i];
    if (mediaFormat.GetDefaultSessionID() == session &&
            (session == OpalMediaFormat::DefaultDataSessionID ||
             mediaFormat.GetPayloadType() < RTP_DataFrame::MaxPayloadType))
      AddMediaFormat(mediaFormat, map);
  }
}


//////////////////////////////////////////////////////////////////////////////

const PString & SDPSessionDescription::ConferenceTotalBandwidthModifier()     { static PString s = "CT"; return s; }
const PString & SDPSessionDescription::ApplicationSpecificBandwidthModifier() { static PString s = "AS"; return s; }

SDPSessionDescription::SDPSessionDescription(const OpalTransportAddress & address)
  : sessionName(SIP_DEFAULT_SESSION_NAME),
    ownerUsername('-'),
    ownerAddress(address),
    defaultConnectAddress(address)
{
  protocolVersion  = 0;
  ownerSessionId  = ownerVersion = (unsigned)PTime().GetTimeInSeconds();
  direction = SDPMediaDescription::Undefined;
  
  bandwidthModifier = "";
  bandwidthValue = 0;
}


void SDPSessionDescription::PrintOn(ostream & str) const
{
  OpalTransportAddress connectionAddress(defaultConnectAddress);
  BOOL useCommonConnect = TRUE;

  // see common connect address is needed
  {
    OpalTransportAddress descrAddress;
    PINDEX matched = 0;
    PINDEX descrMatched = 0;
    PINDEX i;
    for (i = 0; i < mediaDescriptions.GetSize(); i++) {
      if (i == 0)
        descrAddress = mediaDescriptions[i].GetTransportAddress();
      if (mediaDescriptions[i].GetTransportAddress() == connectionAddress)
        ++matched;
      if (mediaDescriptions[i].GetTransportAddress() == descrAddress)
        ++descrMatched;
    }
    if (connectionAddress != descrAddress) {
      if ((descrMatched > matched))
        connectionAddress = descrAddress;
      else
        useCommonConnect = FALSE;
    }
  }

  // encode mandatory session information
  str << "v=" << protocolVersion << "\r\n"
         "o=" << ownerUsername << ' '
        << ownerSessionId << ' '
        << ownerVersion << ' '
              << GetConnectAddressString(ownerAddress)
              << "\r\n"
         "s=" << sessionName << "\r\n";

  // make sure the "c=" line (if required) is before the "t=" otherwise the proxy
  // used by FWD will reject the call with an SDP parse error. This does not seem to be 
  // required by the RFC so it is probably a bug in the proxy
  if (useCommonConnect)
    str << "c=" << GetConnectAddressString(connectionAddress) << "\r\n";
  
  if(bandwidthModifier != "" && bandwidthValue != 0) {
    str << "b=" << bandwidthModifier << ":" << bandwidthValue << "\r\n";
  }
  
  str << "t=" << "0 0" << "\r\n";

  switch (direction) {
    case SDPMediaDescription::RecvOnly:
      str << "a=recvonly" << "\r\n";
      break;
    case SDPMediaDescription::SendOnly:
      str << "a=sendonly" << "\r\n";
      break;
    case SDPMediaDescription::SendRecv:
      str << "a=sendrecv" << "\r\n";
      break;
    case SDPMediaDescription::Inactive:
      str << "a=inactive" << "\r\n";
      break;
    default:
      break;
  }

  // encode media session information
  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) {
    if (useCommonConnect) 
      mediaDescriptions[i].PrintOn(connectionAddress, str);
    else
      str << mediaDescriptions[i];
  }
}


PString SDPSessionDescription::Encode() const
{
  PStringStream str;
  PrintOn(str);
  return str;
}


BOOL SDPSessionDescription::Decode(const PString & str)
{
  // break string into lines
  PStringArray lines = str.Lines();

  // parse keyvalue pairs
  SDPMediaDescription * currentMedia = NULL;
  PINDEX i;
  for (i = 0; i < lines.GetSize(); i++) {
    PString & line = lines[i];
    PINDEX pos = line.Find('=');
    if (pos != P_MAX_INDEX) {
      PString key   = line.Left(pos).Trim();
      PString value = line.Mid(pos+1).Trim();
      if (key.GetLength() == 1) {

        // media name and transport address (mandatory)
        if (key[0] == 'm') {
          currentMedia = new SDPMediaDescription(defaultConnectAddress);
          if (currentMedia->Decode(value)) {
            mediaDescriptions.Append(currentMedia);
            PTRACE(3, "SDP\tAdding media session with " << currentMedia->GetSDPMediaFormats().GetSize() << " formats");
          }
          else {
            delete currentMedia;
            currentMedia = NULL;
          }
        }
  
        /////////////////////////////////
        //
        // Session description
        //
        /////////////////////////////////
    
        else if (currentMedia == NULL) {
          PINDEX thePos;
          switch (key[0]) {
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
              thePos = value.Find(':');
              if (thePos != P_MAX_INDEX) {
                bandwidthModifier = value.Left(thePos);
                bandwidthValue = value.Mid(thePos+1).AsInteger();
              }
              break;
            case 'z' : // time zone adjustments
            case 'k' : // encryption key
            case 'r' : // zero or more repeat times
              break;
            case 'a' : // zero or more session attribute lines
              if (value *= "sendonly")
                SetDirection (SDPMediaDescription::SendOnly);
              else if (value *= "recvonly")
                SetDirection (SDPMediaDescription::RecvOnly);
              else if (value *= "sendrecv")
                SetDirection (SDPMediaDescription::SendRecv);
              else if (value *= "inactive")
                SetDirection (SDPMediaDescription::Inactive);
              break;

            default:
              PTRACE(1, "SDP\tUnknown session information key " << key[0]);
          }
        }

        /////////////////////////////////
        //
        // media information
        //
        /////////////////////////////////
    
        else {
          switch (key[0]) {
            case 'i' : // media title
            case 'b' : // bandwidth information
            case 'k' : // encryption key
              break;

            case 'c' : // connection information - optional if included at session-level
              currentMedia->SetTransportAddress(ParseConnectAddress(value));
              break;

            case 'a' : // zero or more media attribute lines
              pos = value.FindSpan("!#$%&'*+-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`abcdefghijklmnopqrstuvwxyz{|}~"); // Legal chars from RFC
              if (pos == P_MAX_INDEX)
                currentMedia->SetAttribute(value, "1");
              else if (value[pos] == ':')
                currentMedia->SetAttribute(value.Left(pos), value.Mid(pos+1));
              else {
                PTRACE(2, "SDP\tMalformed media attribute " << value);
              }
              break;

            default:
              PTRACE(1, "SDP\tUnknown media information key " << key[0]);
          }
        }
      }
    }
  }

  return TRUE;
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


SDPMediaDescription * SDPSessionDescription::GetMediaDescription(
                                    SDPMediaDescription::MediaType rtpMediaType) const
{
  // look for matching media type
  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) {
    if (mediaDescriptions[i].GetMediaType() == rtpMediaType)
      return &mediaDescriptions[i];
  }

  return NULL;
}


SDPMediaDescription::Direction SDPSessionDescription::GetDirection(unsigned sessionID) const
{
  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) {
    if ((mediaDescriptions[i].GetMediaType() == SDPMediaDescription::Video && sessionID == OpalMediaFormat::DefaultVideoSessionID) || (mediaDescriptions[i].GetMediaType() == SDPMediaDescription::Audio && sessionID == OpalMediaFormat::DefaultAudioSessionID)) {
      if (mediaDescriptions[i].GetDirection() != SDPMediaDescription::Undefined)
        return mediaDescriptions[i].GetDirection();
      else
        return direction;
    }
  }
  
  return direction;
}

// End of file ////////////////////////////////////////////////////////////////
