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
 * Revision 1.2002  2002/02/01 04:53:01  robertj
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


#define SIP_DEFAULT_SESSION_NAME	"Opal SIP Session"
#define	SDP_MEDIA_TRANSPORT		    "RTP/AVP"
#define RFC2833_NTE_TYPE          "telephone-event"

#define new PNEW


/////////////////////////////////////////////////////////

SDPMediaFormat::SDPMediaFormat(RTP_DataFrame::PayloadTypes pt,
                               const char * _name,
                               PINDEX _clockRate,
                               const char * _parms)
  : payloadType(pt),
    encodingName(_name),
    clockRate(_clockRate), 
    parameters(_parms)
{
}

SDPMediaFormat::SDPMediaFormat(const PString & nteString, RTP_DataFrame::PayloadTypes pt)

  : payloadType(pt),
    encodingName(RFC2833_NTE_TYPE),
    clockRate(8000)
{
  AddNTEString(nteString);
}

void SDPMediaFormat::SetFMTP(const PString & str)
{
  if (encodingName == RFC2833_NTE_TYPE) {
    nteSet.RemoveAll();
    AddNTEString(str);
  }

  else
    PTRACE(2, "SDP\tAttempt to set fmtp attributes for unknown meda type " << encodingName);
}


PString SDPMediaFormat::GetFMTP() const
{
  if (encodingName == RFC2833_NTE_TYPE)
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
  OpalMediaFormat mediaFormat(encodingName);

  if (mediaFormat.IsEmpty())
    mediaFormat = payloadType;

  return mediaFormat;
}

/////////////////////////////////////////////////////////

SDPMediaDescription::SDPMediaDescription(const PString & str)
{
  PStringArray tokens = str.Tokenise(" ");

  if (tokens.GetSize() < 4) {
    PTRACE(1, "SDP\tMedia session has only " << tokens.GetSize() << " elements");
  } else {
    media            = tokens[0];
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
    port = (WORD)portStr.AsUnsigned();

    // create the format list
    PINDEX i;
    for (i = 3; i < tokens.GetSize(); i++)
      formats.Append(new SDPMediaFormat((RTP_DataFrame::PayloadTypes)tokens[i].AsUnsigned()));
  }
}


SDPMediaDescription::SDPMediaDescription(MediaType _mediaType, WORD _port)
  : mediaType(_mediaType)
{
  switch (mediaType) {
    case Audio:
      media = "audio";
      break;
    case Video:
      media = "video";
      break;
    default:
      media = "unknown";
      break;
  }
  transport = SDP_MEDIA_TRANSPORT; 
  port = _port;
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
    format.SetClockRate(tokens[1].AsInteger());
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
  PAssert(port != 0, "SDPMediaDescription has no port defined");

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
  for (i = 0; i < formats.GetSize(); i++) {
    const SDPMediaFormat & format = formats[i];
    if (format.GetPayloadType() > RTP_DataFrame::LastKnownPayloadType)
      str << format;
  }
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
  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++)
    if (formats[i].GetPayloadType() == mediaFormat.GetPayloadType())
      return;

  AddSDPMediaFormat(new SDPMediaFormat(mediaFormat.GetPayloadType(), mediaFormat));
}


void SDPMediaDescription::AddMediaFormats(const OpalMediaFormatList & mediaFormats)
{
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++)
    AddMediaFormat(mediaFormats[i]);
}


/////////////////////////////////////////////////////////

SDPSessionDescription::SDPSessionDescription()
{
  PIPSocket::Address addr;
  Construct(addr);
}


SDPSessionDescription::SDPSessionDescription(const PString & str)
{
  Decode(str);
}


SDPSessionDescription::SDPSessionDescription(const PIPSocket::Address & ip)
{
  Construct(ip);
}


void SDPSessionDescription::Construct(const PIPSocket::Address & addr)
{
  PString timeStamp = PTime().GetTimeInSeconds();

  protocolVersion  = 0;
  sessionName      = SIP_DEFAULT_SESSION_NAME;

  ownerUsername    = "-";
  ownerSessionId   = timeStamp;
  ownerVersion     = timeStamp;
  ownerNetworkType = connectNetworkType = "IN";
  ownerAddressType = connectAddressType = "IP4";
  ownerAddress     = connectAddress     = addr.AsString();
}


void SDPSessionDescription::PrintOn(ostream & str) const
{
  // encode mandatory session information
  str << "v=" << protocolVersion << "\r\n"
         "o=" << ownerUsername << ' '
	      << ownerSessionId << ' '
	      << ownerVersion << ' '
	      << ownerNetworkType << ' '
	      << ownerAddressType << ' '
	      << ownerAddress << "\r\n"
         "s=" << sessionName << "\r\n"
         "t=" << "0 0" << "\r\n"
         "c=" << connectNetworkType << ' '
              << connectAddressType << ' '
	      << connectAddress << "\r\n";

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


void SDPSessionDescription::Decode(const PString & str)
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
          currentMedia = new SDPMediaDescription(value);
          mediaDescriptions.Append(currentMedia);
          PTRACE(2, "SDP\tAdding media session with " << currentMedia->GetSDPMediaFormats().GetSize() << " formats");
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
	            ParseConnect(value);
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
	          case 'c' : // connection information - optional if included at session-level
	          case 'b' : // bandwidth information
	          case 'k' : // encryption key
	          case 'a' : // zero or more media attribute lines
              currentMedia->SetAttribute(value);
              break;

            default:
              PTRACE(1, "SDP\tUnknown mediainformation key " << key[0]);
	        }
	      }
      }
    }
  }
}


void SDPSessionDescription::ParseOwner(const PString & str)
{
  PStringArray tokens = str.Tokenise(" ");

  if (tokens.GetSize() != 6) {
    PTRACE(1, "SDP\tOrigin has " << tokens.GetSize() << " elements");
  } else {
    ownerUsername    = tokens[0];
    ownerSessionId   = tokens[1];
    ownerVersion     = tokens[2];
    ownerNetworkType = connectNetworkType = tokens[3];
    ownerAddressType = connectAddressType = tokens[4];
    ownerAddress     = connectAddress     = tokens[5];
  }
}


void SDPSessionDescription::ParseConnect(const PString & str)
{
  PStringArray tokens = str.Tokenise(" ");

  if (tokens.GetSize() != 3) {
    PTRACE(1, "SDP\tConnect has " << tokens.GetSize() << " elements");
  } else {
    connectNetworkType = tokens[0];
    connectAddressType = tokens[1];
    connectAddress     = tokens[2];
  }
}


BOOL SDPSessionDescription::SetConnectAddress(const OpalTransportAddress & address)
{
  PIPSocket::Address ip;
  if (address.GetIpAddress(ip)) {
    connectNetworkType = "IN";
    connectAddressType = "IP4";
    connectAddress     = ip.AsString();
    return TRUE;
  }

  return FALSE;
}


SDPMediaDescription * SDPSessionDescription::GetMediaDescription(
                                    SDPMediaDescription::MediaType rtpMediaType)
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
