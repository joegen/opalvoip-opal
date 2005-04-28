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
 * Revision 1.2019  2005/04/28 20:22:54  dsandras
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
#define	SDP_MEDIA_TRANSPORT       "RTP/AVP"


#define new PNEW


/////////////////////////////////////////////////////////

static OpalTransportAddress ParseConnectAddress(const PStringArray & tokens, PINDEX offset)
{
  if (tokens.GetSize() == offset+3) {
    if (tokens[offset] *= "IN") {
      if (tokens[offset+1] *= "IP4")
        return tokens[offset+2];
      else {
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

SDPMediaFormat::SDPMediaFormat(RTP_DataFrame::PayloadTypes pt,
                               const char * _name,
                               unsigned _clockRate,
                               const char * _parms)
  : payloadType(pt),
    clockRate(_clockRate),
    encodingName(_name),
    parameters(_parms),
    nteSet(TRUE)
{
  if (encodingName == OpalRFC2833.GetEncodingName())
    AddNTEString("0-15");
}


SDPMediaFormat::SDPMediaFormat(const PString & nteString, RTP_DataFrame::PayloadTypes pt)

  : payloadType(pt),
    clockRate(8000),
    encodingName(OpalRFC2833.GetEncodingName()),
    nteSet(TRUE)
{
  AddNTEString(nteString);
}


void SDPMediaFormat::SetFMTP(const PString & str)
{
  if (encodingName == OpalRFC2833.GetEncodingName()) {
    nteSet.RemoveAll();
    AddNTEString(str);
  }
  else
    PTRACE(2, "SDP\tAttempt to set fmtp attributes for unknown meda type " << encodingName);
}


PString SDPMediaFormat::GetFMTP() const
{
  if (encodingName == OpalRFC2833.GetEncodingName())
    return GetNTEString();

  return PString();
}


PString SDPMediaFormat::GetNTEString() const
{
  POrdinalSet & nteSetNc = (POrdinalSet &)nteSet;
  PString str;
  PINDEX i = 0;
  while (i < nteSet.GetSize()) {
    if (!nteSetNc.Contains(POrdinalKey(i)))
      i++;
    else {
      PINDEX start = i++;
      while (nteSetNc.Contains(POrdinalKey(i)))
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


void SDPMediaFormat::AddNTEString(const PString & str)
{
  PStringArray tokens = str.Tokenise(",", FALSE);
  PINDEX i;
  for (i = 0; i < tokens.GetSize(); i++)
    AddNTEToken(tokens[i]);
}


void SDPMediaFormat::AddNTEToken(const PString & ostr)
{
  PString str = ostr.Trim();
  if (str[0] == ',')
    str = str.Mid(1);
  if (str.Right(1) == ",")
    str = str.Left(str.GetLength()-1);
  PINDEX pos = str.Find('-');
  if (pos == P_MAX_INDEX)
    nteSet.Include(new POrdinalKey(str.AsInteger()));
  else {
    PINDEX from = str.Left(pos).AsInteger();
    PINDEX to   = str.Mid(pos+1).AsInteger();
    while (from <= to)
      nteSet.Include(new POrdinalKey(from++));
  }
}


void SDPMediaFormat::PrintOn(ostream & strm) const
{
  PAssert(!encodingName.IsEmpty(), "SDPAudioMediaFormat encoding name is empty");

  strm << "a=rtpmap:" << (int)payloadType << ' ' << encodingName << '/' << clockRate;
  if (!parameters.IsEmpty())
    strm << '/' << parameters;
  strm << "\r\n";

  PString nteString = GetFMTP();

  if (!nteString.IsEmpty())
    strm << "a=fmtp:" << (int)payloadType << ' ' << nteString << "\r\n";
}


OpalMediaFormat SDPMediaFormat::GetMediaFormat() const
{
  return OpalMediaFormat(payloadType, encodingName);
}


//////////////////////////////////////////////////////////////////////////////

#if PTRACING
ostream & operator<<(ostream & out, SDPMediaDescription::MediaType type)
{
  static const char * const MediaTypeNames[SDPMediaDescription::NumMediaTypes] = {
    "Audio", "Video", "Application", "Unknown"
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
    transportAddress(address),
	packetTime(0)
{
  switch (mediaType) {
    case Audio:
      media = "audio";
      break;
    case Video:
      media = "video";
      break;
    default:
      break;
  }

  transport = SDP_MEDIA_TRANSPORT;
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
    PTRACE(1, "SDP\tMedia header contains port count - " << portStr);
    portCount = (WORD)portStr.Mid(pos+1).AsUnsigned();
    portStr   = portStr.Left(pos);
  }
  unsigned port = portStr.AsUnsigned();
  PTRACE(4, "SDP\tMedia session port=" << port);

  if (transport != SDP_MEDIA_TRANSPORT) {
    PTRACE(1, "SDP\tMedia session has only " << tokens.GetSize() << " elements");
    return FALSE;
  }

  PIPSocket::Address ip;
  transportAddress.GetIpAddress(ip);
  transportAddress = OpalTransportAddress(ip, (WORD)port);

  // create the format list
  PINDEX i;
  for (i = 3; i < tokens.GetSize(); i++)
    formats.Append(new SDPMediaFormat((RTP_DataFrame::PayloadTypes)tokens[i].AsUnsigned()));

  return TRUE;
}


void SDPMediaDescription::SetAttribute(const PString & ostr)
{
  // get the attribute type
  PINDEX pos = ostr.Find(":");
  if (pos == P_MAX_INDEX) {
    PTRACE(2, "SDP\tMalformed media attribute " << ostr);
    return;
  }
  PString attr = ostr.Left(pos);
  PString str  = ostr.Mid(pos+1);

  if (attr *= "ptime") {                // caseless comparison
      packetTime = str.AsUnsigned() ;
      return;
  }
  // extract the RTP payload type
  pos = str.Find(" ");
  if (pos == P_MAX_INDEX) {
    PTRACE(2, "SDP\tMalformed media attribute " << ostr);
    return;
  }
  RTP_DataFrame::PayloadTypes pt = (RTP_DataFrame::PayloadTypes)str.Left(pos).AsUnsigned();

  // find the format that matches the payload type
  PINDEX fmt = 0;
  while (formats[fmt].GetPayloadType() != pt) {
    fmt++;
    if (fmt >= formats.GetSize()) {
      PTRACE(2, "SDP\tMedia attribute " << attr << " found for unknown RTP type " << pt);
      return;
    }
  }
  SDPMediaFormat & format = formats[fmt];

  // extract the attribute argument
  str = str.Mid(pos+1).Trim();

  // handle rtpmap attribute
  if (attr *= "rtpmap") {

    PStringArray tokens = str.Tokenise('/');
    if (tokens.GetSize() < 2) {
      PTRACE(2, "SDP\tMalformed rtpmap attribute for " << pt);
      return;
    }

    format.SetEncodingName(tokens[0]);
    format.SetClockRate(tokens[1].AsUnsigned());
    if (tokens.GetSize() > 2)
      format.SetParameters(tokens[2]);

    return;
  }

  // handle fmtp attributes
  if (attr *= "fmtp") {
    format.SetFMTP(str);
    return;
  }

  // unknown attriutes
  PTRACE(2, "SDP\tUnknown media attribute " << ostr);
  return;
}


void SDPMediaDescription::PrintOn(ostream & str) const
{
  PIPSocket::Address ip;
  WORD port;
  transportAddress.GetIpAndPort(ip, port);

  // output media header
  str << "m=" 
      << media << " "
      << port << " "
      << transport;

  // output RTP payload types
  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++)
    str << ' ' << (int)formats[i].GetPayloadType();
  str << "\r\n";
  
  // output attributes for each payload type
  for (i = 0; i < formats.GetSize(); i++)
    str << formats[i];

  if (packetTime)
	  str << "a=ptime:" << packetTime << "\r\n";
}


OpalMediaFormatList SDPMediaDescription::GetMediaFormats() const
{
  OpalMediaFormatList list;

  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++) {
    OpalMediaFormat opalFormat = formats[i].GetMediaFormat();
    if (opalFormat.IsEmpty())
      PTRACE(2, "SIP\tRTP payload type " << formats[i].GetPayloadType() << " not matched to audio codec");
    else {
      PTRACE(2, "SIP\tRTP payload type " << formats[i].GetPayloadType() << " matched to audio codec " << opalFormat);
      list += opalFormat;
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
  RTP_DataFrame::PayloadTypes payloadType = mediaFormat.GetPayloadType();
  const char * encodingName = mediaFormat.GetEncodingName();

  if (payloadType >= RTP_DataFrame::MaxPayloadType || encodingName == NULL || *encodingName == '\0')
    return;

  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++) {
    if (formats[i].GetPayloadType() == payloadType ||
        strcasecmp(formats[i].GetEncodingName(), encodingName) == 0)
      return;
  }

  AddSDPMediaFormat(new SDPMediaFormat(payloadType, encodingName));
}


void SDPMediaDescription::AddMediaFormats(const OpalMediaFormatList & mediaFormats, unsigned session)
{
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++) {
    OpalMediaFormat mediaFormat = mediaFormats[i];
    if (mediaFormat.GetDefaultSessionID() == session &&
        mediaFormat.GetPayloadType() != RTP_DataFrame::IllegalPayloadType)
      AddMediaFormat(mediaFormat);
  }
}


//////////////////////////////////////////////////////////////////////////////

SDPSessionDescription::SDPSessionDescription(const OpalTransportAddress & address)
  : sessionName(SIP_DEFAULT_SESSION_NAME),
    ownerUsername('-'),
    ownerAddress(address),
    defaultConnectAddress(address)
{
  protocolVersion  = 0;
  ownerSessionId  = ownerVersion = PTime().GetTimeInSeconds();
  direction = SDPSessionDescription::Undefined;
}


void SDPSessionDescription::PrintOn(ostream & str) const
{
  // encode mandatory session information
  str << "v=" << protocolVersion << "\r\n"
         "o=" << ownerUsername << ' '
	      << ownerSessionId << ' '
	      << ownerVersion << ' '
              << GetConnectAddressString(ownerAddress)
              << "\r\n"
         "s=" << sessionName << "\r\n"
         "c=" << (direction == SDPSessionDescription::SendOnly?GetConnectAddressString("0"):GetConnectAddressString(defaultConnectAddress)) << "\r\n"
         "t=" << "0 0" << "\r\n";

  switch (direction) {
  
  case SDPSessionDescription::RecvOnly:
    str << "a=recvonly" << "\r\n";
    break;
  case SDPSessionDescription::SendOnly:
    str << "a=sendonly" << "\r\n";
    break;
  case SDPSessionDescription::SendRecv:
    str << "a=sendrecv" << "\r\n";
    break;
  case SDPSessionDescription::Inactive:
    str << "a=inactive" << "\r\n";
    break;
  default:
    break;
  }
  
  // encode media session information
  PINDEX i;
  for (i = 0; i < mediaDescriptions.GetSize(); i++) 
    str << mediaDescriptions[i];
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
          else
            delete currentMedia;
        }
	
        /////////////////////////////////
        //
        // Session description
        //
        /////////////////////////////////
	  
        else if (currentMedia == NULL) {
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
            case 'b' : // bandwidth information
            case 'z' : // time zone adjustments
            case 'k' : // encryption key
            case 'a' : // zero or more session attribute lines
            case 'r' : // zero or more repeat times
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
              break;

            case 'a' : // zero or more media attribute lines
	      if (value *= "sendonly")
		SetDirection (SendOnly);
	      else if (value *= "recvonly")
		SetDirection (RecvOnly);
	      else if (value *= "sendrecv")
		SetDirection (SendRecv);
	      else if (value *= "inactive")
		SetDirection (Inactive);
	      else
		currentMedia->SetAttribute(value);
              break;

            default:
              PTRACE(1, "SDP\tUnknown mediainformation key " << key[0]);
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
    PTRACE(1, "SDP\tOrigin has " << tokens.GetSize() << " elements");
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


// End of file ////////////////////////////////////////////////////////////////
