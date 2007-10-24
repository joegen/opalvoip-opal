/*
 * h323caps.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: h323caps.cxx,v $
 * Revision 2.43  2007/10/10 04:57:48  csoutheren
 * Include capability type in logging for FindCapability
 * Fix problem where MatchWildcard incorrectly matched prefix of media format
 *   i.e. returned G.729A/B for G.729
 *
<<<<<<< h323caps.cxx
 * Revision 2.42  2007/09/18 18:35:51  dsandras
 * Fixed compilation warning with GCC 4.2.
 *
 * Revision 2.41  2007/09/14 01:26:18  csoutheren
 * Fixed warnings
 *
 * Revision 2.40  2007/09/12 04:19:53  rjongbloed
 * CHanges to avoid creation of long duration OpalMediaFormat instances, eg in
 *   the plug in capabilities, that then do not get updated values from the master
 *   list, or worse from the user modified master list, causing much confusion.
 *
 * Revision 2.39  2007/09/09 23:37:19  rjongbloed
 * Fixed confusion over MaxPayloadType meaning
 *
 * Revision 2.38  2007/09/05 04:13:32  rjongbloed
 * Fixed getting same media packetization information in TCS multiple times.
 *
 * Revision 2.37  2007/09/05 01:36:00  rjongbloed
 * Added media packetization fields to H.323 TCS.
 *
 * Revision 2.36  2007/08/22 05:09:48  rjongbloed
 * Improved logging
 *
 * Revision 2.35  2007/08/11 00:03:23  rjongbloed
 * Fixed incorrect cast in H.323 request mode.
 *
 * Revision 2.34  2007/07/24 12:55:52  rjongbloed
 * Fixed correct setting of outgoing bit rate in H.245 generic capabilities.
 *
 * Revision 2.33  2007/07/20 05:46:20  rjongbloed
 * Fixed incorrect maxBitRate field in Generic Audio Capabilities.
 * Implemented H323GenericCapabilityInfo::OnReceivedGenericPDU()
 *
 * Revision 2.32  2007/06/29 06:59:57  rjongbloed
 * Major improvement to the "product info", normalising H.221 and User-Agent mechanisms.
 *
 * Revision 2.31  2007/06/27 07:56:08  rjongbloed
 * Add new OpalMediaOption for octet strings (simple block of bytes).
 *
 * Revision 2.30  2007/06/22 05:49:13  rjongbloed
 * Major codec API update:
 *   Automatically map OpalMediaOptions to SIP/SDP FMTP parameters.
 *   Automatically map OpalMediaOptions to H.245 Generic Capability parameters.
 *   Largely removed need to distinguish between SIP and H.323 codecs.
 *   New mechanism for setting OpalMediaOptions from within a plug in.
 * Fixed removal of H.323 capabilities just because media format name is a
 *   substring of capability name.
 * Fixed inadequacies in H.245 Generic Capabilities (must be able to
 *   distinguish between TCS, OLC and ReqMode).
 *
 * Revision 2.29  2007/04/10 05:15:54  rjongbloed
 * Fixed issue with use of static C string variables in DLL environment,
 *   must use functional interface for correct initialisation.
 *
 * Revision 2.28  2007/04/04 02:12:00  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.27  2006/09/28 07:42:17  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.26  2006/08/11 07:52:01  csoutheren
 * Fix problem with media format factory in VC 2005
 * Fixing problems with Speex codec
 * Remove non-portable usages of PFactory code
 *
 * Revision 2.25  2006/08/10 06:30:53  csoutheren
 * Fix problem with key type in plugin factory
 *
 * Revision 2.24  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.23.4.3  2006/04/19 04:58:56  csoutheren
 * Debugging and testing of new video plugins
 * H.261 working in both CIF and QCIF modes in H.323
 *
 * Revision 2.23.4.2  2006/04/06 01:21:18  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 2.23.4.1  2006/03/23 07:55:18  csoutheren
 * Audio plugin H.323 capability merging completed.
 * GSM, LBC, G.711 working. Speex and LPC-10 are not
 *
 * Revision 2.23  2005/02/21 12:19:54  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.22  2004/11/07 12:26:40  rjongbloed
 * Fixed incorrect conditional in decoding non standard application data
 *   types in OLC, thanks Dmitriy
 *
 * Revision 2.21  2004/09/01 12:21:28  rjongbloed
 * Added initialisation of H323EndPoints capability table to be all codecs so can
 *   correctly build remote caps from fqast connect params. This had knock on effect
 *   with const keywords added in numerous places.
 *
 * Revision 2.20  2004/06/08 13:47:58  rjongbloed
 * Fixed (pre)condition for checking and matching capabilities, thanks Guilhem Tardy
 *
 * Revision 2.19  2004/06/04 04:09:59  csoutheren
 * Fixed for updates to H225v5
 *
 * Revision 2.18  2004/04/07 08:21:03  rjongbloed
 * Changes for new RTTI system.
 *
 * Revision 2.17  2004/02/19 10:47:04  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.16  2003/04/08 02:44:04  robertj
 * Removed the {sw} from media format name, this is a hang over from
 *   OpenH323, thanks Guilhem Tardy for pointing this out.
 *
 * Revision 2.15  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.14  2002/11/10 11:33:18  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.13  2002/09/06 02:43:03  robertj
 * Fixed function to add capabilities to return correct table entry index.
 *
 * Revision 2.12  2002/09/04 06:01:48  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.11  2002/07/01 04:56:32  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.10  2002/03/27 02:21:51  robertj
 * Updated to OpenH323 v1.8.4
 *
 * Revision 2.9  2002/03/22 06:57:49  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.8  2002/02/19 07:47:29  robertj
 * Added OpalRFC2833 as a OpalMediaFormat variable.
 * Fixes to capability tables to make RFC2833 work properly.
 *
 * Revision 2.7  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.6  2002/01/22 05:29:12  robertj
 * Revamp of user input API triggered by RFC2833 support
 * Update from OpenH323
 *
 * Revision 2.5  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.4  2002/01/14 02:22:03  robertj
 * Fixed bug in wildcard name lookup.
 *
 * Revision 2.3  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.2  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.1  2001/08/01 05:12:59  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 * Added "known" codecs capability clases.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.68  2003/11/08 03:11:29  rjongbloed
 * Fixed failure to call ancestor in copy ctor, thanks  Victor Ivashin.
 *
 * Revision 1.67  2003/10/27 06:03:39  csoutheren
 * Added support for QoS
 *   Thanks to Henry Harrison of AliceStreet
 *
 * Revision 1.66  2003/06/06 02:13:48  rjongbloed
 * Changed non-standard capability semantics so can use C style strings as
 *   the embedded data block (ie automatically call strlen)
 *
 * Revision 1.65  2003/05/16 07:30:20  rjongbloed
 * Fixed correct matching of OLC data types to capabilities, for example CIF
 *   and QCIF video are different and should match exactly.
 *
 * Revision 1.64  2003/04/28 07:00:09  robertj
 * Fixed problem with compiler(s) not correctly initialising static globals
 *
 * Revision 1.63  2003/04/27 23:50:38  craigs
 * Made list of registered codecs available outside h323caps.cxx
 *
 * Revision 1.62  2003/03/18 05:11:22  robertj
 * Fixed OID based non-standard capabilities, thanks Philippe Massicotte
 *
 * Revision 1.61  2002/12/05 12:29:31  rogerh
 * Add non standard codec identifier for Xiph.org
 *
 * Revision 1.60  2002/11/27 11:47:09  robertj
 * Fixed GNU warning
 *
 * Revision 1.59  2002/11/26 22:48:18  craigs
 * Changed nonStd codec identification to use a table for MS codecs
 *
 * Revision 1.58  2002/11/26 13:52:59  craigs
 * Added PrintOn function for outputting names of nonStandard codecs
 *
 * Revision 1.57  2002/11/09 04:44:24  robertj
 * Fixed function to add capabilities to return correct table entry index.
 * Cosmetic changes.
 *
 * Revision 1.56  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.55  2002/07/18 08:28:52  robertj
 * Adjusted some trace log levels
 *
 * Revision 1.54  2002/06/04 07:16:14  robertj
 * Fixed user indications (DTMF) not working on some endpoints which indicated
 *   receiveAndTransmitUserInputCapability in TCS.
 *
 * Revision 1.53  2002/05/29 03:55:21  robertj
 * Added protocol version number checking infrastructure, primarily to improve
 *   interoperability with stacks that are unforgiving of new features.
 *
 * Revision 1.52  2002/05/14 23:20:03  robertj
 * Fixed incorrect comparison in non-standard capability, tnanks Vyacheslav Frolov
 *
 * Revision 1.51  2002/05/10 05:45:41  robertj
 * Added the max bit rate field to the data channel capability class.
 *
 * Revision 1.50  2002/03/26 05:51:12  robertj
 * Forced RFC2833 to payload type 101 as some IOS's go nuts otherwise.
 *
 * Revision 1.49  2002/03/05 06:18:46  robertj
 * Fixed problem with some custom local capabilities not being used in getting
 *   remote capability list, especially in things like OpenAM.
 *
 * Revision 1.48  2002/02/25 04:38:42  robertj
 * Fixed wildcard lookup with * at end of string.
 * Fixed being able to create remote capability table before have local table.
 * Fixed using add all with wildcards adding capability multiple times.
 *
 * Revision 1.47  2002/02/14 07:15:15  robertj
 * Fixed problem with creation of remoteCapabilities and the "set" part contianing
 *   pointers to objects that have been deleted. Does not seem to be a practical
 *   problem but certainly needs fixing!
 *
 * Revision 1.46  2002/01/22 07:08:26  robertj
 * Added IllegalPayloadType enum as need marker for none set
 *   and MaxPayloadType is a legal value.
 *
 * Revision 1.45  2002/01/22 06:07:35  robertj
 * Moved payload type to ancestor so any capability can adjust it on logical channel.
 *
 * Revision 1.44  2002/01/17 07:05:03  robertj
 * Added support for RFC2833 embedded DTMF in the RTP stream.
 *
 * Revision 1.43  2002/01/16 05:38:04  robertj
 * Added missing mode change functions on non standard capabilities.
 *
 * Revision 1.42  2002/01/10 05:13:54  robertj
 * Added support for external RTP stacks, thanks NuMind Software Systems.
 *
 * Revision 1.41  2002/01/09 00:21:39  robertj
 * Changes to support outgoing H.245 RequstModeChange.
 *
 * Revision 1.40  2001/12/22 01:44:30  robertj
 * Added more support for H.245 RequestMode operation.
 *
 * Revision 1.39  2001/09/21 02:52:56  robertj
 * Added default implementation for PDU encode/decode for codecs
 *   that have simple integer as frames per packet.
 *
 * Revision 1.38  2001/09/11 10:21:42  robertj
 * Added direction field to capabilities, thanks Nick Hoath.
 *
 * Revision 1.37  2001/08/06 03:08:56  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 *
 * Revision 1.36  2001/07/19 09:50:30  robertj
 * Added code for default session ID on data channel being three.
 *
 * Revision 1.35  2001/07/17 04:44:31  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 * Revision 1.34  2001/06/29 04:58:57  robertj
 * Added wildcard character '*' to capability name string searches.
 *
 * Revision 1.33  2001/06/15 16:10:19  rogerh
 * Fix the "capabilities are the same" assertion
 *
 * Revision 1.32  2001/05/31 06:28:37  robertj
 * Made sure capability descriptors alternate capability sets are in the same
 *   order as the capability table when doing reorder. This improves compatibility
 *   with endpoints that select the first capability in that list rather than the table.
 *
 * Revision 1.31  2001/05/14 05:56:28  robertj
 * Added H323 capability registration system so can add capabilities by
 *   string name instead of having to instantiate explicit classes.
 *
 * Revision 1.30  2001/05/02 16:22:21  rogerh
 * Add IsAllow() for a single capability to check if it is in the
 * capabilities set. This fixes the bug where OpenH323 would accept
 * incoming H261 video even when told not to accept it.
 *
 * Revision 1.29  2001/04/12 03:22:44  robertj
 * Fixed fast start checking of returned OLC frame count to use minimum
 *   of user setting and remotes maximum limitation, Was always just
 *   sending whatever the remote said it could do.
 *
 * Revision 1.28  2001/03/16 23:00:22  robertj
 * Improved validation of codec selection against capability set, thanks Chris Purvis.
 *
 * Revision 1.27  2001/03/06 04:44:47  robertj
 * Fixed problem where could send capability set twice. This should not be
 *   a problem except when talking to another broken stack, eg Cisco routers.
 *
 * Revision 1.26  2001/02/09 05:13:55  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.25  2001/01/25 07:27:16  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.24  2001/01/16 03:14:01  craigs
 * Changed nonstanard capability Compare functions to not assert
 * if compared to other capability types
 *
 * Revision 1.23  2001/01/09 23:05:24  robertj
 * Fixed inability to have 2 non standard codecs in capability table.
 *
 * Revision 1.22  2001/01/02 07:50:46  robertj
 * Fixed inclusion of arrays (with bad size) in TCS=0 pdu, thanks Yura Aksyonov.
 *
 * Revision 1.21  2000/12/19 22:32:26  dereks
 * Removed MSVC warning about unused parameter
 *
 * Revision 1.20  2000/11/08 04:50:22  craigs
 * Changed capability reorder function to reorder all capabilities matching
 * preferred order, rather than just the first
 *
 * Revision 1.19  2000/10/16 08:50:08  robertj
 * Added single function to add all UserInput capability types.
 *
 * Revision 1.18  2000/10/13 03:43:29  robertj
 * Added clamping to avoid ever setting incorrect tx frame count.
 *
 * Revision 1.17  2000/10/13 02:20:32  robertj
 * Fixed capability clone so gets all fields including those in ancestor.
 *
 * Revision 1.16  2000/08/23 14:27:04  craigs
 * Added prototype support for Microsoft GSM codec
 *
 * Revision 1.15  2000/07/13 12:30:46  robertj
 * Fixed problems with fast start frames per packet adjustment.
 *
 * Revision 1.14  2000/07/12 10:25:37  robertj
 * Renamed all codecs so obvious whether software or hardware.
 *
 * Revision 1.13  2000/07/10 16:03:02  robertj
 * Started fixing capability set merging, still more to do.
 *
 * Revision 1.12  2000/07/04 01:16:49  robertj
 * Added check for capability allowed in "combinations" set, still needs more done yet.
 *
 * Revision 1.11  2000/07/02 14:08:43  craigs
 * Fixed problem with removing capabilities based on wildcard
 *
 * Revision 1.10  2000/06/03 03:16:39  robertj
 * Fixed using the wrong capability table (should be connections) for some operations.
 *
 * Revision 1.9  2000/05/30 06:53:48  robertj
 * Fixed bug where capability numbers in duplicate table are not identical (should be!).
 *
 * Revision 1.8  2000/05/23 11:32:37  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H323Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.7  2000/05/10 04:05:34  robertj
 * Changed capabilities so has a function to get name of codec, instead of relying on PrintOn.
 *
 * Revision 1.6  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.5  2000/04/05 19:01:12  robertj
 * Added function so can change desired transmit packet size.
 *
 * Revision 1.4  2000/03/22 01:29:43  robertj
 * Fixed default "frame" size for audio codecs, caused crash using G.711
 *
 * Revision 1.3  2000/03/21 03:06:50  robertj
 * Changes to make RTP TX of exact numbers of frames in some codecs.
 *
 * Revision 1.2  2000/02/16 03:24:27  robertj
 * Fixed bug in clamping maximum transmit packet size in G.711 capabilities.
 *
 * Revision 1.1  1999/12/23 23:02:36  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323caps.h"
#endif

#include <h323/h323caps.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/h323pdu.h>
#include <h323/transaddr.h>


#define DEFINE_G711_CAPABILITY(cls, code, capName) \
class cls : public H323_G711Capability { \
  public: \
    cls() : H323_G711Capability(code) { } \
}; \
H323_REGISTER_CAPABILITY(cls, capName) \

#ifndef NO_H323_AUDIO_CODECS

DEFINE_G711_CAPABILITY(H323_G711ALaw64Capability, H323_G711Capability::ALaw, OpalG711_ALAW_64K)
DEFINE_G711_CAPABILITY(H323_G711uLaw64Capability, H323_G711Capability::muLaw, OpalG711_ULAW_64K)

#endif


#if PTRACING
ostream & operator<<(ostream & o , H323Capability::MainTypes t)
{
  const char * const names[] = {
    "Audio", "Video", "Data", "UserInput"
  };
  return o << names[t];
}

ostream & operator<<(ostream & o , H323Capability::CapabilityDirection d)
{
  const char * const names[] = {
    "Unknown", "Receive", "Transmit", "ReceiveAndTransmit", "NoDirection"
  };
  return o << names[d];
}
#endif


/////////////////////////////////////////////////////////////////////////////

H323Capability::H323Capability()
{
  assignedCapabilityNumber = 0; // Unassigned
  capabilityDirection = e_Unknown;
  rtpPayloadType = RTP_DataFrame::IllegalPayloadType;
}


PObject::Comparison H323Capability::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, H323Capability), PInvalidCast);
  const H323Capability & other = (const H323Capability &)obj;

  int mt = GetMainType();
  int omt = other.GetMainType();
  if (mt < omt)
    return LessThan;
  if (mt > omt)
    return GreaterThan;

  int st = GetSubType();
  int ost = other.GetSubType();
  if (st < ost)
    return LessThan;
  if (st > ost)
    return GreaterThan;

  return EqualTo;
}


void H323Capability::PrintOn(ostream & strm) const
{
  strm << GetFormatName();
  if (assignedCapabilityNumber != 0)
    strm << " <" << assignedCapabilityNumber << '>';
}


H323Capability * H323Capability::Create(const PString & name)
{
  H323Capability * cap = H323CapabilityFactory::CreateInstance(name);
  if (cap == NULL)
    return NULL;

  return (H323Capability *)cap->Clone();
}


unsigned H323Capability::GetDefaultSessionID() const
{
  return 0;
}


void H323Capability::SetTxFramesInPacket(unsigned /*frames*/)
{
}


unsigned H323Capability::GetTxFramesInPacket() const
{
  return 1;
}


unsigned H323Capability::GetRxFramesInPacket() const
{
  return 1;
}


BOOL H323Capability::IsMatch(const PASN_Choice & subTypePDU) const
{
  return subTypePDU.GetTag() == GetSubType();
}


BOOL H323Capability::OnReceivedPDU(const H245_Capability & cap)
{
  switch (cap.GetTag()) {
    case H245_Capability::e_receiveVideoCapability:
    case H245_Capability::e_receiveAudioCapability:
    case H245_Capability::e_receiveDataApplicationCapability:
    case H245_Capability::e_h233EncryptionReceiveCapability:
    case H245_Capability::e_receiveUserInputCapability:
      capabilityDirection = e_Receive;
      break;

    case H245_Capability::e_transmitVideoCapability:
    case H245_Capability::e_transmitAudioCapability:
    case H245_Capability::e_transmitDataApplicationCapability:
    case H245_Capability::e_h233EncryptionTransmitCapability:
    case H245_Capability::e_transmitUserInputCapability:
      capabilityDirection = e_Transmit;
      break;

    case H245_Capability::e_receiveAndTransmitVideoCapability:
    case H245_Capability::e_receiveAndTransmitAudioCapability:
    case H245_Capability::e_receiveAndTransmitDataApplicationCapability:
    case H245_Capability::e_receiveAndTransmitUserInputCapability:
      capabilityDirection = e_ReceiveAndTransmit;
      break;

    case H245_Capability::e_conferenceCapability:
    case H245_Capability::e_h235SecurityCapability:
    case H245_Capability::e_maxPendingReplacementFor:
      capabilityDirection = e_NoDirection;
  }

  return TRUE;
}


BOOL H323Capability::IsUsable(const H323Connection &) const
{
  return TRUE;
}


OpalMediaFormat H323Capability::GetMediaFormat() const
{
  return mediaFormat.IsEmpty() ? OpalMediaFormat(GetFormatName()) : mediaFormat;
}


OpalMediaFormat & H323Capability::GetWritableMediaFormat()
{
  if (mediaFormat.IsEmpty())
    mediaFormat = GetFormatName();
  return mediaFormat;
}


/////////////////////////////////////////////////////////////////////////////

H323RealTimeCapability::H323RealTimeCapability()
{
    rtpqos = NULL;
}

H323RealTimeCapability::H323RealTimeCapability(const H323RealTimeCapability & rtc)
  : H323Capability(rtc)
{
  if (rtc.rtpqos == NULL) 
    rtpqos = NULL;
  else {
    rtpqos  = new RTP_QOS();
    *rtpqos = *rtc.rtpqos;
  }
}

H323RealTimeCapability::~H323RealTimeCapability()
{
  if (rtpqos != NULL)
    delete rtpqos;
}

void H323RealTimeCapability::AttachQoS(RTP_QOS * _rtpqos)
{
  if (rtpqos != NULL)
    delete rtpqos;
    
  rtpqos = _rtpqos;
}

H323Channel * H323RealTimeCapability::CreateChannel(H323Connection & connection,
                                                    H323Channel::Directions dir,
                                                    unsigned sessionID,
                                 const H245_H2250LogicalChannelParameters * param) const
{
  return connection.CreateRealTimeLogicalChannel(*this, dir, sessionID, param, rtpqos);
}


/////////////////////////////////////////////////////////////////////////////

H323NonStandardCapabilityInfo::H323NonStandardCapabilityInfo(CompareFuncType _compareFunc,
                                                             const BYTE * dataPtr,
                                                             PINDEX dataSize)
  :
    t35CountryCode(OpalProductInfo::Default().t35CountryCode),
    t35Extension(OpalProductInfo::Default().t35Extension),
    manufacturerCode(OpalProductInfo::Default().manufacturerCode),
    nonStandardData(dataPtr, dataSize == 0 && dataPtr != NULL
                                 ? strlen((const char *)dataPtr) : dataSize),
    comparisonOffset(0),
    comparisonLength(0),
    compareFunc(_compareFunc)
{
}

H323NonStandardCapabilityInfo::H323NonStandardCapabilityInfo(const BYTE * dataPtr,
                                                             PINDEX dataSize,
                                                             PINDEX _offset,
                                                             PINDEX _len)
  : t35CountryCode(OpalProductInfo::Default().t35CountryCode),
    t35Extension(OpalProductInfo::Default().t35Extension),
    manufacturerCode(OpalProductInfo::Default().manufacturerCode),
    nonStandardData(dataPtr, dataSize == 0 && dataPtr != NULL
                                 ? strlen((const char *)dataPtr) : dataSize),
    comparisonOffset(_offset),
    comparisonLength(_len),
    compareFunc(NULL)
{
}


H323NonStandardCapabilityInfo::H323NonStandardCapabilityInfo(const PString & _oid,
                                                             const BYTE * dataPtr,
                                                             PINDEX dataSize,
                                                             PINDEX _offset,
                                                             PINDEX _len)
  : oid(_oid),
    nonStandardData(dataPtr, dataSize == 0 && dataPtr != NULL
                                 ? strlen((const char *)dataPtr) : dataSize),
    comparisonOffset(_offset),
    comparisonLength(_len),
    compareFunc(NULL)
{
}


H323NonStandardCapabilityInfo::H323NonStandardCapabilityInfo(BYTE country,
                                                             BYTE extension,
                                                             WORD maufacturer,
                                                             const BYTE * dataPtr,
                                                             PINDEX dataSize,
                                                             PINDEX _offset,
                                                             PINDEX _len)
  : t35CountryCode(country),
    t35Extension(extension),
    manufacturerCode(maufacturer),
    nonStandardData(dataPtr, dataSize == 0 && dataPtr != NULL
                                 ? strlen((const char *)dataPtr) : dataSize),
    comparisonOffset(_offset),
    comparisonLength(_len),
    compareFunc(NULL)
{
}


H323NonStandardCapabilityInfo::~H323NonStandardCapabilityInfo()
{
}


BOOL H323NonStandardCapabilityInfo::OnSendingPDU(PBYTEArray & data) const
{
  data = nonStandardData;
  return data.GetSize() > 0;
}


BOOL H323NonStandardCapabilityInfo::OnReceivedPDU(const PBYTEArray & data)
{
  if (CompareData(data) != PObject::EqualTo)
    return FALSE;

  nonStandardData = data;
  return TRUE;
}


BOOL H323NonStandardCapabilityInfo::OnSendingNonStandardPDU(PASN_Choice & pdu,
                                                            unsigned nonStandardTag) const
{
  PBYTEArray data;
  if (!OnSendingPDU(data))
    return FALSE;

  pdu.SetTag(nonStandardTag);
  H245_NonStandardParameter & param = (H245_NonStandardParameter &)pdu.GetObject();

  if (!oid) {
    param.m_nonStandardIdentifier.SetTag(H245_NonStandardIdentifier::e_object);
    PASN_ObjectId & nonStandardIdentifier = param.m_nonStandardIdentifier;
    nonStandardIdentifier = oid;
  }
  else {
    param.m_nonStandardIdentifier.SetTag(H245_NonStandardIdentifier::e_h221NonStandard);
    H245_NonStandardIdentifier_h221NonStandard & h221 = param.m_nonStandardIdentifier;
    h221.m_t35CountryCode = (unsigned)t35CountryCode;
    h221.m_t35Extension = (unsigned)t35Extension;
    h221.m_manufacturerCode = (unsigned)manufacturerCode;
  }

  param.m_data = data;
  return data.GetSize() > 0;
}


BOOL H323NonStandardCapabilityInfo::OnReceivedNonStandardPDU(const PASN_Choice & pdu,
                                                             unsigned nonStandardTag)
{
  if (pdu.GetTag() != nonStandardTag)
    return FALSE;

  const H245_NonStandardParameter & param = (const H245_NonStandardParameter &)pdu.GetObject();

  if (CompareParam(param) != PObject::EqualTo)
    return FALSE;

  return OnReceivedPDU(param.m_data);
}


BOOL H323NonStandardCapabilityInfo::IsMatch(const H245_NonStandardParameter & param) const
{
  return CompareParam(param) == PObject::EqualTo && CompareData(param.m_data) == PObject::EqualTo;
}


PObject::Comparison H323NonStandardCapabilityInfo::CompareParam(const H245_NonStandardParameter & param) const
{
  if (!oid) {
    if (param.m_nonStandardIdentifier.GetTag() != H245_NonStandardIdentifier::e_object)
      return PObject::LessThan;

    const PASN_ObjectId & nonStandardIdentifier = param.m_nonStandardIdentifier;
    return oid.Compare(nonStandardIdentifier.AsString());
  }

  if (param.m_nonStandardIdentifier.GetTag() != H245_NonStandardIdentifier::e_h221NonStandard)
    return PObject::LessThan;

  const H245_NonStandardIdentifier_h221NonStandard & h221 = param.m_nonStandardIdentifier;

  if (h221.m_t35CountryCode < (unsigned)t35CountryCode)
    return PObject::LessThan;
  if (h221.m_t35CountryCode > (unsigned)t35CountryCode)
    return PObject::GreaterThan;

  if (h221.m_t35Extension < (unsigned)t35Extension)
    return PObject::LessThan;
  if (h221.m_t35Extension > (unsigned)t35Extension)
    return PObject::GreaterThan;

  if (h221.m_manufacturerCode < (unsigned)manufacturerCode)
    return PObject::LessThan;
  if (h221.m_manufacturerCode > (unsigned)manufacturerCode)
    return PObject::GreaterThan;


  return PObject::EqualTo;
}


PObject::Comparison H323NonStandardCapabilityInfo::CompareInfo(const H323NonStandardCapabilityInfo & other) const
{
  return CompareData(other.nonStandardData);
}


PObject::Comparison H323NonStandardCapabilityInfo::CompareData(const PBYTEArray & data) const
{
  if (comparisonOffset >= nonStandardData.GetSize())
    return PObject::LessThan;
  if (comparisonOffset >= data.GetSize())
    return PObject::GreaterThan;

  PINDEX len = comparisonLength;
  if (comparisonOffset+len > nonStandardData.GetSize())
    len = nonStandardData.GetSize() - comparisonOffset;

  if (comparisonOffset+len > data.GetSize())
    return PObject::GreaterThan;

  int cmp = memcmp((const BYTE *)nonStandardData + comparisonOffset,
                   (const BYTE *)data + comparisonOffset,
                   len);
  if (cmp < 0)
    return PObject::LessThan;
  if (cmp > 0)
    return PObject::GreaterThan;
  return PObject::EqualTo;
}


/////////////////////////////////////////////////////////////////////////////

H323GenericCapabilityInfo::H323GenericCapabilityInfo(const PString & standardId, PINDEX maxBitRate)
  : maxBitRate(maxBitRate)
{
  identifier = new H245_CapabilityIdentifier(H245_CapabilityIdentifier::e_standard);
  PASN_ObjectId & object_id = *identifier;
  object_id = standardId;
}


H323GenericCapabilityInfo::H323GenericCapabilityInfo(const H323GenericCapabilityInfo & obj)
  : maxBitRate(obj.maxBitRate)
{
  identifier = new H245_CapabilityIdentifier(*obj.identifier);
}


H323GenericCapabilityInfo::~H323GenericCapabilityInfo()
{
  delete identifier;
}


BOOL H323GenericCapabilityInfo::OnSendingGenericPDU(H245_GenericCapability & pdu,
                                                    const OpalMediaFormat & mediaFormat,
                                                    H323Capability::CommandType type) const
{
  pdu.m_capabilityIdentifier = *identifier;

  unsigned bitRate = maxBitRate != 0 ? maxBitRate : ((mediaFormat.GetBandwidth()+99)/100);
  if (bitRate != 0) {
    pdu.IncludeOptionalField(H245_GenericCapability::e_maxBitRate);
    pdu.m_maxBitRate = bitRate;
  }

  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    OpalMediaOption::H245GenericInfo genericInfo = option.GetH245Generic();
    if (genericInfo.mode == OpalMediaOption::H245GenericInfo::None)
      continue;
    switch (type) {
      case H323Capability::e_TCS :
        if (genericInfo.excludeTCS)
          continue;
        break;
      case H323Capability::e_OLC :
        if (genericInfo.excludeOLC)
          continue;
        break;
      case H323Capability::e_ReqMode :
        if (genericInfo.excludeReqMode)
          continue;
        break;
    }

    H245_GenericParameter * param = new H245_GenericParameter;

    param->m_parameterIdentifier.SetTag(H245_ParameterIdentifier::e_standard);
    (PASN_Integer &)param->m_parameterIdentifier = genericInfo.ordinal;

    if (PIsDescendant(&option, OpalMediaOptionBoolean)) {
      if (!((const OpalMediaOptionBoolean &)option).GetValue()) {
        delete param;
        continue; // Do not include a logical at all if it is false
      }
      param->m_parameterValue.SetTag(H245_ParameterValue::e_logical);
    }
    else if (PIsDescendant(&option, OpalMediaOptionUnsigned)) {
      unsigned tag;
      switch (genericInfo.integerType) {
        default :
        case OpalMediaOption::H245GenericInfo::UnsignedInt :
          tag = option.GetMerge() == OpalMediaOption::MinMerge ? H245_ParameterValue::e_unsignedMin : H245_ParameterValue::e_unsignedMax;
          break;

        case OpalMediaOption::H245GenericInfo::Unsigned32 :
          tag = option.GetMerge() == OpalMediaOption::MinMerge ? H245_ParameterValue::e_unsigned32Min : H245_ParameterValue::e_unsigned32Max;
          break;

        case OpalMediaOption::H245GenericInfo::BooleanArray :
          tag = H245_ParameterValue::e_booleanArray;
          break;
      }

      param->m_parameterValue.SetTag(tag);
      (PASN_Integer &)param->m_parameterValue = ((const OpalMediaOptionUnsigned &)option).GetValue();
    }
    else {
      param->m_parameterValue.SetTag(H245_ParameterValue::e_octetString);
      PASN_OctetString & octetString = param->m_parameterValue;
      if (PIsDescendant(&option, OpalMediaOptionOctets))
        octetString = ((const OpalMediaOptionOctets &)option).GetValue();
      else
        octetString = option.AsString();
    }

    if (genericInfo.mode == OpalMediaOption::H245GenericInfo::Collapsing) {
      pdu.IncludeOptionalField(H245_GenericCapability::e_collapsing);
      pdu.m_collapsing.Append(param);
    }
    else {
      pdu.IncludeOptionalField(H245_GenericCapability::e_nonCollapsing);
      pdu.m_nonCollapsing.Append(param);
    }
  }

  return TRUE;
}

BOOL H323GenericCapabilityInfo::OnReceivedGenericPDU(OpalMediaFormat & mediaFormat,
                                                     const H245_GenericCapability & pdu,
                                                     H323Capability::CommandType type)
{
  if (pdu.m_capabilityIdentifier != *identifier)
    return FALSE;

  if (pdu.HasOptionalField(H245_GenericCapability::e_maxBitRate)) {
    maxBitRate = pdu.m_maxBitRate;
    mediaFormat.SetOptionInteger(OpalMediaFormat::MaxBitRateOption(), maxBitRate*100);
  }

  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    OpalMediaOption::H245GenericInfo genericInfo = option.GetH245Generic();
    if (genericInfo.mode == OpalMediaOption::H245GenericInfo::None)
      continue;
    switch (type) {
      case H323Capability::e_TCS :
        if (genericInfo.excludeTCS)
          continue;
        break;
      case H323Capability::e_OLC :
        if (genericInfo.excludeOLC)
          continue;
        break;
      case H323Capability::e_ReqMode :
        if (genericInfo.excludeReqMode)
          continue;
        break;
    }

    const H245_ArrayOf_GenericParameter * params;
    if (genericInfo.mode == OpalMediaOption::H245GenericInfo::Collapsing) {
      if (!pdu.HasOptionalField(H245_GenericCapability::e_collapsing))
        continue;
      params = &pdu.m_collapsing;
    }
    else {
      if (!pdu.HasOptionalField(H245_GenericCapability::e_nonCollapsing))
        continue;
      params = &pdu.m_nonCollapsing;
    }

    if (PIsDescendant(&option, OpalMediaOptionBoolean))
      ((OpalMediaOptionBoolean &)option).SetValue(false);

    for (PINDEX j = 0; j < params->GetSize(); j++) {
      const H245_GenericParameter & param = (*params)[j];
      if (param.m_parameterIdentifier.GetTag() == H245_ParameterIdentifier::e_standard &&
                         (const PASN_Integer &)param.m_parameterIdentifier == genericInfo.ordinal) {
        if (PIsDescendant(&option, OpalMediaOptionBoolean)) {
          if (param.m_parameterValue.GetTag() == H245_ParameterValue::e_logical) {
            ((OpalMediaOptionBoolean &)option).SetValue(true);
            break;
          }
        }
        else if (PIsDescendant(&option, OpalMediaOptionUnsigned)) {
          unsigned tag;
          switch (genericInfo.integerType) {
            default :
            case OpalMediaOption::H245GenericInfo::UnsignedInt :
              tag = option.GetMerge() == OpalMediaOption::MinMerge ? H245_ParameterValue::e_unsignedMin : H245_ParameterValue::e_unsignedMax;
              break;
 
            case OpalMediaOption::H245GenericInfo::Unsigned32 :
              tag = option.GetMerge() == OpalMediaOption::MinMerge ? H245_ParameterValue::e_unsigned32Min : H245_ParameterValue::e_unsigned32Max;
              break;
 
            case OpalMediaOption::H245GenericInfo::BooleanArray :
              tag = H245_ParameterValue::e_booleanArray;
              break;
          }
 
          if (param.m_parameterValue.GetTag() == tag) {
            ((OpalMediaOptionUnsigned &)option).SetValue((const PASN_Integer &)param.m_parameterValue);
            break;
          }
        }
        else {
          if (param.m_parameterValue.GetTag() == H245_ParameterValue::e_octetString) {
            const PASN_OctetString & octetString = param.m_parameterValue;
            if (PIsDescendant(&option, OpalMediaOptionOctets))
              ((OpalMediaOptionOctets &)option).SetValue(octetString);
            else
              ((OpalMediaOption &)option).FromString(octetString.AsString());
            break;
          }
        }

        PTRACE(2, "Invalid generic parameter type (" << param.m_parameterValue.GetTagName()
               << ") for option \"" << option.GetName() << "\" (" << option.GetClass() << ')');
      }
    }
  }

  return TRUE;
}

BOOL H323GenericCapabilityInfo::IsMatch(const H245_GenericCapability & param) const
{
  return param.m_capabilityIdentifier == *identifier;
}

PObject::Comparison H323GenericCapabilityInfo::CompareInfo(const H323GenericCapabilityInfo & obj) const
{
  return identifier->Compare(*obj.identifier);
}


/////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

H323AudioCapability::H323AudioCapability()
{
}


H323Capability::MainTypes H323AudioCapability::GetMainType() const
{
  return e_Audio;
}


unsigned H323AudioCapability::GetDefaultSessionID() const
{
  return OpalMediaFormat::DefaultAudioSessionID;
}


void H323AudioCapability::SetTxFramesInPacket(unsigned frames)
{
  GetWritableMediaFormat().SetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), frames);
}


unsigned H323AudioCapability::GetTxFramesInPacket() const
{
  return GetMediaFormat().GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);
}


unsigned H323AudioCapability::GetRxFramesInPacket() const
{
  return GetMediaFormat().GetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption(), 1);
}


BOOL H323AudioCapability::OnSendingPDU(H245_Capability & cap) const
{
  cap.SetTag(H245_Capability::e_receiveAudioCapability);
  return OnSendingPDU((H245_AudioCapability &)cap, GetRxFramesInPacket(), e_TCS);
}


BOOL H323AudioCapability::OnSendingPDU(H245_DataType & dataType) const
{
  dataType.SetTag(H245_DataType::e_audioData);
  return OnSendingPDU((H245_AudioCapability &)dataType, GetTxFramesInPacket(), e_OLC);
}


BOOL H323AudioCapability::OnSendingPDU(H245_ModeElement & mode) const
{
  mode.m_type.SetTag(H245_ModeElementType::e_audioMode);
  return OnSendingPDU((H245_AudioMode &)mode.m_type);
}


BOOL H323AudioCapability::OnSendingPDU(H245_AudioCapability & pdu,
                                       unsigned packetSize) const
{
  pdu.SetTag(GetSubType());

  // Set the maximum number of frames
  PASN_Integer & value = pdu;
  value = packetSize;
  return TRUE;
}


BOOL H323AudioCapability::OnSendingPDU(H245_AudioCapability & pdu,
                                       unsigned packetSize,
                                       CommandType) const
{
  return OnSendingPDU(pdu, packetSize);
}


BOOL H323AudioCapability::OnSendingPDU(H245_AudioMode & pdu) const
{
  static const H245_AudioMode::Choices AudioTable[] = {
    H245_AudioMode::e_nonStandard,
    H245_AudioMode::e_g711Alaw64k,
    H245_AudioMode::e_g711Alaw56k,
    H245_AudioMode::e_g711Ulaw64k,
    H245_AudioMode::e_g711Ulaw56k,
    H245_AudioMode::e_g722_64k,
    H245_AudioMode::e_g722_56k,
    H245_AudioMode::e_g722_48k,
    H245_AudioMode::e_g7231,
    H245_AudioMode::e_g728,
    H245_AudioMode::e_g729,
    H245_AudioMode::e_g729AnnexA,
    H245_AudioMode::e_is11172AudioMode,
    H245_AudioMode::e_is13818AudioMode,
    H245_AudioMode::e_g729wAnnexB,
    H245_AudioMode::e_g729AnnexAwAnnexB,
    H245_AudioMode::e_g7231AnnexCMode,
    H245_AudioMode::e_gsmFullRate,
    H245_AudioMode::e_gsmHalfRate,
    H245_AudioMode::e_gsmEnhancedFullRate,
    H245_AudioMode::e_genericAudioMode,
    H245_AudioMode::e_g729Extensions
  };

  unsigned subType = GetSubType();
  if (subType >= PARRAYSIZE(AudioTable))
    return FALSE;

  pdu.SetTag(AudioTable[subType]);
  return TRUE;
}


BOOL H323AudioCapability::OnReceivedPDU(const H245_Capability & cap)
{
  H323Capability::OnReceivedPDU(cap);

  if (cap.GetTag() != H245_Capability::e_receiveAudioCapability &&
      cap.GetTag() != H245_Capability::e_receiveAndTransmitAudioCapability)
    return FALSE;

  unsigned txFramesInPacket = GetTxFramesInPacket();
  unsigned packetSize = txFramesInPacket;
  if (!OnReceivedPDU((const H245_AudioCapability &)cap, packetSize, e_TCS))
    return FALSE;

  // Clamp our transmit size to maximum allowed
  if (txFramesInPacket > packetSize) {
    PTRACE(4, "H323\tCapability tx frames reduced from "
           << txFramesInPacket << " to " << packetSize);
    SetTxFramesInPacket(packetSize);
  }
  else {
    PTRACE(4, "H323\tCapability tx frames left at "
           << txFramesInPacket << " as remote allows " << packetSize);
  }

  return TRUE;
}


BOOL H323AudioCapability::OnReceivedPDU(const H245_DataType & dataType, BOOL receiver)
{
  if (dataType.GetTag() != H245_DataType::e_audioData)
    return FALSE;

  unsigned xFramesInPacket = receiver ? GetRxFramesInPacket() : GetTxFramesInPacket();
  unsigned packetSize = xFramesInPacket;
  if (!OnReceivedPDU((const H245_AudioCapability &)dataType, packetSize, e_OLC))
    return FALSE;

  // Clamp our transmit size to maximum allowed
  if (xFramesInPacket > packetSize) {
    PTRACE(4, "H323\tCapability " << (receiver ? 'r' : 't') << "x frames reduced from "
           << xFramesInPacket << " to " << packetSize);
    if (!receiver)
      SetTxFramesInPacket(packetSize);
  }
  else {
    PTRACE(4, "H323\tCapability " << (receiver ? 'r' : 't') << "x frames left at "
           << xFramesInPacket << " as remote allows " << packetSize);
  }

  return TRUE;
}


BOOL H323AudioCapability::OnReceivedPDU(const H245_AudioCapability & pdu,
                                        unsigned & packetSize)
{
  if (pdu.GetTag() != GetSubType())
    return FALSE;

  const PASN_Integer & value = pdu;

  // Get the maximum number of frames
  packetSize = value;
  return TRUE;
}


BOOL H323AudioCapability::OnReceivedPDU(const H245_AudioCapability & pdu,
                                        unsigned & packetSize,
                                        CommandType)
{
  return OnReceivedPDU(pdu, packetSize);
}


/////////////////////////////////////////////////////////////////////////////

H323GenericAudioCapability::H323GenericAudioCapability(const PString &standardId, PINDEX maxBitRate)
  : H323AudioCapability(),
    H323GenericCapabilityInfo(standardId, maxBitRate)
{
}

PObject::Comparison H323GenericAudioCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323GenericAudioCapability))
    return LessThan;

  return CompareInfo((const H323GenericAudioCapability &)obj);
}


unsigned H323GenericAudioCapability::GetSubType() const
{
  return H245_AudioCapability::e_genericAudioCapability;
}


BOOL H323GenericAudioCapability::OnSendingPDU(H245_AudioCapability & pdu, unsigned, CommandType type) const
{
  pdu.SetTag(H245_AudioCapability::e_genericAudioCapability);
  return OnSendingGenericPDU(pdu, GetMediaFormat(), type);
}

BOOL H323GenericAudioCapability::OnSendingPDU(H245_AudioMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_genericVideoMode);
  return OnSendingGenericPDU(pdu, GetMediaFormat(), e_ReqMode);
}

BOOL H323GenericAudioCapability::OnReceivedPDU(const H245_AudioCapability & pdu, unsigned &, CommandType type)
{
  if( pdu.GetTag() != H245_AudioCapability::e_genericAudioCapability)
    return FALSE;
  return OnReceivedGenericPDU(GetWritableMediaFormat(), pdu, type);
}

BOOL H323GenericAudioCapability::IsMatch(const PASN_Choice & subTypePDU) const
{
  return H323Capability::IsMatch(subTypePDU) &&
         H323GenericCapabilityInfo::IsMatch((const H245_GenericCapability &)subTypePDU.GetObject());
}


/////////////////////////////////////////////////////////////////////////////

H323NonStandardAudioCapability::H323NonStandardAudioCapability(
      H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
      const BYTE * fixedData,
      PINDEX dataSize)
  : H323AudioCapability(),
    H323NonStandardCapabilityInfo(compareFunc, fixedData, dataSize)
{
}

H323NonStandardAudioCapability::H323NonStandardAudioCapability(const BYTE * fixedData,
                                                                     PINDEX dataSize,
                                                                     PINDEX offset,
                                                                     PINDEX length)
  : H323AudioCapability(),
    H323NonStandardCapabilityInfo(fixedData, dataSize, offset, length)
{
}

H323NonStandardAudioCapability::H323NonStandardAudioCapability(const PString & oid,
                                                                 const BYTE * fixedData,
                                                                       PINDEX dataSize,
                                                                       PINDEX offset,
                                                                      PINDEX length)
  : H323AudioCapability(),
    H323NonStandardCapabilityInfo(oid, fixedData, dataSize, offset, length)
{
}

H323NonStandardAudioCapability::H323NonStandardAudioCapability(BYTE country,
                                                               BYTE extension,
                                                               WORD maufacturer,
                                                       const BYTE * fixedData,
                                                             PINDEX dataSize,
                                                             PINDEX offset,
                                                             PINDEX length)
  : H323AudioCapability(),
    H323NonStandardCapabilityInfo(country, extension, maufacturer, fixedData, dataSize, offset, length)
{
}


PObject::Comparison H323NonStandardAudioCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323NonStandardAudioCapability))
    return PObject::LessThan;

  return CompareInfo((const H323NonStandardAudioCapability &)obj);
}


unsigned H323NonStandardAudioCapability::GetSubType() const
{
  return H245_AudioCapability::e_nonStandard;
}


BOOL H323NonStandardAudioCapability::OnSendingPDU(H245_AudioCapability & pdu,
                                                  unsigned) const
{
  return OnSendingNonStandardPDU(pdu, H245_AudioCapability::e_nonStandard);
}


BOOL H323NonStandardAudioCapability::OnSendingPDU(H245_AudioMode & pdu) const
{
  return OnSendingNonStandardPDU(pdu, H245_AudioMode::e_nonStandard);
}


BOOL H323NonStandardAudioCapability::OnReceivedPDU(const H245_AudioCapability & pdu,
                                                   unsigned &)
{
  return OnReceivedNonStandardPDU(pdu, H245_AudioCapability::e_nonStandard);
}


BOOL H323NonStandardAudioCapability::IsMatch(const PASN_Choice & subTypePDU) const
{
  return H323Capability::IsMatch(subTypePDU) &&
         H323NonStandardCapabilityInfo::IsMatch((const H245_NonStandardParameter &)subTypePDU.GetObject());
}

#endif // OPAL_AUDIO

/////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

H323Capability::MainTypes H323VideoCapability::GetMainType() const
{
  return e_Video;
}


BOOL H323VideoCapability::OnSendingPDU(H245_Capability & cap) const
{
  cap.SetTag(H245_Capability::e_receiveVideoCapability);
  return OnSendingPDU((H245_VideoCapability &)cap, e_TCS);
}


BOOL H323VideoCapability::OnSendingPDU(H245_DataType & dataType) const
{
  dataType.SetTag(H245_DataType::e_videoData);
  return OnSendingPDU((H245_VideoCapability &)dataType, e_OLC);
}


BOOL H323VideoCapability::OnSendingPDU(H245_VideoCapability & /*pdu*/) const
{
  return FALSE;
}


BOOL H323VideoCapability::OnSendingPDU(H245_VideoCapability & pdu, CommandType) const
{
  return OnSendingPDU(pdu);
}


BOOL H323VideoCapability::OnSendingPDU(H245_ModeElement & mode) const
{
  mode.m_type.SetTag(H245_ModeElementType::e_videoMode);
  return OnSendingPDU((H245_VideoMode &)mode.m_type);
}


BOOL H323VideoCapability::OnReceivedPDU(const H245_Capability & cap)
{
  H323Capability::OnReceivedPDU(cap);

  if (cap.GetTag() != H245_Capability::e_receiveVideoCapability &&
      cap.GetTag() != H245_Capability::e_receiveAndTransmitVideoCapability)
    return FALSE;

  return OnReceivedPDU((const H245_VideoCapability &)cap, e_TCS);
}


BOOL H323VideoCapability::OnReceivedPDU(const H245_DataType & dataType, BOOL)
{
  if (dataType.GetTag() != H245_DataType::e_videoData)
    return FALSE;

  return OnReceivedPDU((const H245_VideoCapability &)dataType, e_OLC);
}


BOOL H323VideoCapability::OnReceivedPDU(const H245_VideoCapability &)
{
  return FALSE;
}


BOOL H323VideoCapability::OnReceivedPDU(const H245_VideoCapability & pdu, CommandType)
{
  return OnReceivedPDU(pdu);
}


unsigned H323VideoCapability::GetDefaultSessionID() const
{
  return OpalMediaFormat::DefaultVideoSessionID;
}


/////////////////////////////////////////////////////////////////////////////

H323NonStandardVideoCapability::H323NonStandardVideoCapability(
      H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
      const BYTE * fixedData,
      PINDEX dataSize)
  : H323VideoCapability(),
    H323NonStandardCapabilityInfo(compareFunc, fixedData, dataSize)
{
}

H323NonStandardVideoCapability::H323NonStandardVideoCapability(const BYTE * fixedData,
                                                               PINDEX dataSize,
                                                               PINDEX offset,
                                                               PINDEX length)
  : H323NonStandardCapabilityInfo(fixedData, dataSize, offset, length)
{
}


H323NonStandardVideoCapability::H323NonStandardVideoCapability(const PString & oid,
                                                               const BYTE * fixedData,
                                                               PINDEX dataSize,
                                                               PINDEX offset,
                                                               PINDEX length)
  : H323NonStandardCapabilityInfo(oid, fixedData, dataSize, offset, length)
{
}


H323NonStandardVideoCapability::H323NonStandardVideoCapability(BYTE country,
                                                               BYTE extension,
                                                               WORD maufacturer,
                                                               const BYTE * fixedData,
                                                               PINDEX dataSize,
                                                               PINDEX offset,
                                                               PINDEX length)
  : H323NonStandardCapabilityInfo(country, extension, maufacturer, fixedData, dataSize, offset, length)
{
}


PObject::Comparison H323NonStandardVideoCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323NonStandardVideoCapability))
    return PObject::LessThan;

  return CompareInfo((const H323NonStandardVideoCapability &)obj);
}


unsigned H323NonStandardVideoCapability::GetSubType() const
{
  return H245_VideoCapability::e_nonStandard;
}


BOOL H323NonStandardVideoCapability::OnSendingPDU(H245_VideoCapability & pdu) const
{
  return OnSendingNonStandardPDU(pdu, H245_VideoCapability::e_nonStandard);
}


BOOL H323NonStandardVideoCapability::OnSendingPDU(H245_VideoMode & pdu) const
{
  return OnSendingNonStandardPDU(pdu, H245_VideoMode::e_nonStandard);
}


BOOL H323NonStandardVideoCapability::OnReceivedPDU(const H245_VideoCapability & pdu)
{
  return OnReceivedNonStandardPDU(pdu, H245_VideoCapability::e_nonStandard);
}


BOOL H323NonStandardVideoCapability::IsMatch(const PASN_Choice & subTypePDU) const
{
  return H323Capability::IsMatch(subTypePDU) &&
         H323NonStandardCapabilityInfo::IsMatch((const H245_NonStandardParameter &)subTypePDU.GetObject());
}

/////////////////////////////////////////////////////////////////////////////

H323GenericVideoCapability::H323GenericVideoCapability(const PString &standardId, PINDEX maxBitRate)
  : H323VideoCapability(),
    H323GenericCapabilityInfo(standardId, maxBitRate)
{
}

PObject::Comparison H323GenericVideoCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323GenericVideoCapability))
    return LessThan;

  return CompareInfo((const H323GenericVideoCapability &)obj);
}


unsigned H323GenericVideoCapability::GetSubType() const
{
  return H245_VideoCapability::e_genericVideoCapability;
}


BOOL H323GenericVideoCapability::OnSendingPDU(H245_VideoCapability & pdu, CommandType type) const
{
  pdu.SetTag(H245_VideoCapability::e_genericVideoCapability);
  return OnSendingGenericPDU(pdu, GetMediaFormat(), type);
}

BOOL H323GenericVideoCapability::OnSendingPDU(H245_VideoMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_genericVideoMode);
  return OnSendingGenericPDU(pdu, GetMediaFormat(), e_ReqMode);
}

BOOL H323GenericVideoCapability::OnReceivedPDU(const H245_VideoCapability & pdu, CommandType type)
{
  if (pdu.GetTag() != H245_VideoCapability::e_genericVideoCapability)
    return FALSE;
  return OnReceivedGenericPDU(GetWritableMediaFormat(), pdu, type);
}

BOOL H323GenericVideoCapability::IsMatch(const PASN_Choice & subTypePDU) const
{
  return H323Capability::IsMatch(subTypePDU) &&
         H323GenericCapabilityInfo::IsMatch((const H245_GenericCapability &)subTypePDU.GetObject());
}


#endif // OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////

H323DataCapability::H323DataCapability(unsigned rate)
  : maxBitRate(rate)
{
}


H323Capability::MainTypes H323DataCapability::GetMainType() const
{
  return e_Data;
}


unsigned H323DataCapability::GetDefaultSessionID() const
{
  return 3;
}


BOOL H323DataCapability::OnSendingPDU(H245_Capability & cap) const
{
  cap.SetTag(H245_Capability::e_receiveAndTransmitDataApplicationCapability);
  H245_DataApplicationCapability & app = cap;
  app.m_maxBitRate = maxBitRate;
  return OnSendingPDU(app, e_TCS);
}


BOOL H323DataCapability::OnSendingPDU(H245_DataType & dataType) const
{
  dataType.SetTag(H245_DataType::e_data);
  H245_DataApplicationCapability & app = dataType;
  app.m_maxBitRate = maxBitRate;
  return OnSendingPDU(app, e_OLC);
}


BOOL H323DataCapability::OnSendingPDU(H245_DataApplicationCapability &) const
{
  return FALSE;
}


BOOL H323DataCapability::OnSendingPDU(H245_DataApplicationCapability & pdu, CommandType) const
{
  return OnSendingPDU(pdu);
}


BOOL H323DataCapability::OnSendingPDU(H245_ModeElement & mode) const
{
  mode.m_type.SetTag(H245_ModeElementType::e_dataMode);
  H245_DataMode & type = mode.m_type;
  type.m_bitRate = maxBitRate;
  return OnSendingPDU(type);
}


BOOL H323DataCapability::OnReceivedPDU(const H245_Capability & cap)
{
  H323Capability::OnReceivedPDU(cap);

  if (cap.GetTag() != H245_Capability::e_receiveDataApplicationCapability &&
      cap.GetTag() != H245_Capability::e_receiveAndTransmitDataApplicationCapability)
    return FALSE;

  const H245_DataApplicationCapability & app = cap;
  maxBitRate = app.m_maxBitRate;
  return OnReceivedPDU(app, e_TCS);
}


BOOL H323DataCapability::OnReceivedPDU(const H245_DataType & dataType, BOOL)
{
  if (dataType.GetTag() != H245_DataType::e_data)
    return FALSE;

  const H245_DataApplicationCapability & app = dataType;
  maxBitRate = app.m_maxBitRate;
  return OnReceivedPDU(app, e_OLC);
}


BOOL H323DataCapability::OnReceivedPDU(const H245_DataApplicationCapability &)
{
  return FALSE;
}


BOOL H323DataCapability::OnReceivedPDU(const H245_DataApplicationCapability & pdu, CommandType)
{
  return OnReceivedPDU(pdu);
}


/////////////////////////////////////////////////////////////////////////////

H323NonStandardDataCapability::H323NonStandardDataCapability(unsigned maxBitRate,
                                                         const BYTE * fixedData,
                                                             PINDEX dataSize,
                                                             PINDEX offset,
                                                             PINDEX length)
  : H323DataCapability(maxBitRate),
    H323NonStandardCapabilityInfo(fixedData, dataSize, offset, length)
{
}


H323NonStandardDataCapability::H323NonStandardDataCapability(unsigned maxBitRate,
                                                             const PString & oid,
                                                             const BYTE * fixedData,
                                                             PINDEX dataSize,
                                                             PINDEX offset,
                                                             PINDEX length)
  : H323DataCapability(maxBitRate),
    H323NonStandardCapabilityInfo(oid, fixedData, dataSize, offset, length)
{
}


H323NonStandardDataCapability::H323NonStandardDataCapability(unsigned maxBitRate,
                                                             BYTE country,
                                                             BYTE extension,
                                                             WORD maufacturer,
                                                             const BYTE * fixedData,
                                                             PINDEX dataSize,
                                                             PINDEX offset,
                                                             PINDEX length)
  : H323DataCapability(maxBitRate),
    H323NonStandardCapabilityInfo(country, extension, maufacturer, fixedData, dataSize, offset, length)
{
}


PObject::Comparison H323NonStandardDataCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323NonStandardDataCapability))
    return PObject::LessThan;

  return CompareInfo((const H323NonStandardDataCapability &)obj);
}


unsigned H323NonStandardDataCapability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_nonStandard;
}


BOOL H323NonStandardDataCapability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  return OnSendingNonStandardPDU(pdu.m_application, H245_DataApplicationCapability_application::e_nonStandard);
}


BOOL H323NonStandardDataCapability::OnSendingPDU(H245_DataMode & pdu) const
{
  return OnSendingNonStandardPDU(pdu.m_application, H245_DataMode_application::e_nonStandard);
}


BOOL H323NonStandardDataCapability::OnReceivedPDU(const H245_DataApplicationCapability & pdu)
{
  return OnReceivedNonStandardPDU(pdu.m_application, H245_DataApplicationCapability_application::e_nonStandard);
}


BOOL H323NonStandardDataCapability::IsMatch(const PASN_Choice & subTypePDU) const
{
  return H323Capability::IsMatch(subTypePDU) &&
         H323NonStandardCapabilityInfo::IsMatch((const H245_NonStandardParameter &)subTypePDU.GetObject());
}


/////////////////////////////////////////////////////////////////////////////

H323_G711Capability::H323_G711Capability(Mode m, Speed s)
  : H323AudioCapability()
{
  mode = m;
  speed = s;
  SetTxFramesInPacket(240);   // 240ms max, 30ms desired
}


PObject * H323_G711Capability::Clone() const
{
  return new H323_G711Capability(*this);
}


unsigned H323_G711Capability::GetSubType() const
{
  static const unsigned G711SubType[2][2] = {
    { H245_AudioCapability::e_g711Alaw64k, H245_AudioCapability::e_g711Alaw56k },
    { H245_AudioCapability::e_g711Ulaw64k, H245_AudioCapability::e_g711Ulaw56k }
  };
  return G711SubType[mode][speed];
}


PString H323_G711Capability::GetFormatName() const
{
  static const char * const G711Name[2][2] = {
    { OPAL_G711_ALAW_64K, "G.711-ALaw-56k" },
    { OPAL_G711_ULAW_64K, "G.711-uLaw-56k" }
  };
  return G711Name[mode][speed];
}


/////////////////////////////////////////////////////////////////////////////

#if 0

H323_G728Capability::H323_G728Capability()
  : H323AudioCapability(240, 30) // 240ms max, 30ms desired
{
}


PObject * H323_G728Capability::Clone() const
{
  return new H323_G728Capability(*this);
}


unsigned H323_G728Capability::GetSubType() const
{
  return H245_AudioCapability::e_g728;
}


PString H323_G728Capability::GetFormatName() const
{
  return OpalG728;
}


/////////////////////////////////////////////////////////////////////////////

static const char * const G729Name[4] = {
  OPAL_G729,
  OPAL_G729A,
  OPAL_G729B,
  OPAL_G729AB
};

H323_G729Capability::H323_G729Capability(Mode m)
{
  mode = m;
}


PObject * H323_G729Capability::Clone() const
{
  return new H323_G729Capability(*this);
}


unsigned H323_G729Capability::GetSubType() const
{
  static const unsigned G729SubType[4] = {
    H245_AudioCapability::e_g729,
    H245_AudioCapability::e_g729AnnexA,
    H245_AudioCapability::e_g729wAnnexB,
    H245_AudioCapability::e_g729AnnexAwAnnexB
  };
  return G729SubType[mode];
}


PString H323_G729Capability::GetFormatName() const
{
  return G729Name[mode];
}


/////////////////////////////////////////////////////////////////////////////

H323_G7231Capability::H323_G7231Capability(BOOL sid)
{
  allowSIDFrames = sid;
}


PObject * H323_G7231Capability::Clone() const
{
  return new H323_G7231Capability(*this);
}


unsigned H323_G7231Capability::GetSubType() const
{
  return H245_AudioCapability::e_g7231;
}


PString H323_G7231Capability::GetFormatName() const
{
  return OpalG7231_6k3;
}


BOOL H323_G7231Capability::OnSendingPDU(H245_AudioCapability & pdu,
                                        unsigned packetSize) const
{
  pdu.SetTag(H245_AudioCapability::e_g7231);
  H245_AudioCapability_g7231 & g7231 = pdu;
  g7231.m_maxAl_sduAudioFrames = packetSize;
  g7231.m_silenceSuppression = allowSIDFrames;
  return TRUE;
}


BOOL H323_G7231Capability::OnReceivedPDU(const H245_AudioCapability & pdu,
                                         unsigned & packetSize)
{
  if (pdu.GetTag() != H245_AudioCapability::e_g7231)
    return FALSE;

  const H245_AudioCapability_g7231 & g7231 = pdu;
  packetSize = g7231.m_maxAl_sduAudioFrames;
  allowSIDFrames = g7231.m_silenceSuppression;
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

H323_GSM0610Capability::H323_GSM0610Capability()
{
}


PObject * H323_GSM0610Capability::Clone() const
{
  return new H323_GSM0610Capability(*this);
}


unsigned H323_GSM0610Capability::GetSubType() const
{
  return H245_AudioCapability::e_gsmFullRate;
}


PString H323_GSM0610Capability::GetFormatName() const
{
  return OpalGSM0610;
}


void H323_GSM0610Capability::SetTxFramesInPacket(unsigned frames)
{
  H323AudioCapability::SetTxFramesInPacket(frames > 7 ? 7 : frames);
}


BOOL H323_GSM0610Capability::OnSendingPDU(H245_AudioCapability & cap,
                                          unsigned packetSize) const
{
  cap.SetTag(H245_AudioCapability::e_gsmFullRate);

  H245_GSMAudioCapability & gsm = cap;
  gsm.m_audioUnitSize = packetSize*33;
  return TRUE;
}


BOOL H323_GSM0610Capability::OnReceivedPDU(const H245_AudioCapability & cap,
                                           unsigned & packetSize)
{
  if (cap.GetTag() != H245_AudioCapability::e_gsmFullRate)
    return FALSE;

  const H245_GSMAudioCapability & gsm = cap;
  packetSize = gsm.m_audioUnitSize/33;
  if (packetSize == 0)
    packetSize = 1;
  return TRUE;
}

#endif   // 0

/////////////////////////////////////////////////////////////////////////////

static const char * const UIISubTypeNames[H323_UserInputCapability::NumSubTypes] = {
  "UserInput/basicString",
  "UserInput/iA5String",
  "UserInput/generalString",
  "UserInput/dtmf",
  "UserInput/hookflash",
  OPAL_RFC2833
};

const char * H323_UserInputCapability::GetSubTypeName(SubTypes s)
{
  return s < PARRAYSIZE(UIISubTypeNames) ? UIISubTypeNames[s] : "<Unknown>";
}

#define DECLARE_USER_INPUT_CLASS(type) \
class H323_UserInputCapability_##type : public H323_UserInputCapability \
{ \
  public: \
    H323_UserInputCapability_##type() \
    : H323_UserInputCapability(H323_UserInputCapability::type) { } \
};\

#define DEFINE_USER_INPUT(type) \
  DECLARE_USER_INPUT_CLASS(type) \
  const OpalMediaFormat UserInput_##type( \
    UIISubTypeNames[H323_UserInputCapability::type], \
    0, RTP_DataFrame::IllegalPayloadType, NULL, FALSE, 1, 0, 0, 0 \
  ); \
  H323_REGISTER_CAPABILITY(H323_UserInputCapability_##type, UIISubTypeNames[H323_UserInputCapability::type]) \

DEFINE_USER_INPUT(BasicString);
DEFINE_USER_INPUT(IA5String);
DEFINE_USER_INPUT(GeneralString);
DEFINE_USER_INPUT(SignalToneH245);
DEFINE_USER_INPUT(HookFlashH245);

DECLARE_USER_INPUT_CLASS(SignalToneRFC2833)
H323_REGISTER_CAPABILITY(H323_UserInputCapability_SignalToneRFC2833, UIISubTypeNames[H323_UserInputCapability::SignalToneRFC2833])

H323_UserInputCapability::H323_UserInputCapability(SubTypes _subType)
{
  subType = _subType;
  rtpPayloadType = OpalRFC2833.GetPayloadType();
}


PObject * H323_UserInputCapability::Clone() const
{
  return new H323_UserInputCapability(*this);
}


H323Capability::MainTypes H323_UserInputCapability::GetMainType() const
{
  return e_UserInput;
}


#define SignalToneRFC2833_SubType 10000

static unsigned UserInputCapabilitySubTypeCodes[] = {
  H245_UserInputCapability::e_basicString,
  H245_UserInputCapability::e_iA5String,
  H245_UserInputCapability::e_generalString,
  H245_UserInputCapability::e_dtmf,
  H245_UserInputCapability::e_hookflash,
  SignalToneRFC2833_SubType
};

unsigned  H323_UserInputCapability::GetSubType()  const
{
  return UserInputCapabilitySubTypeCodes[subType];
}


PString H323_UserInputCapability::GetFormatName() const
{
  return UIISubTypeNames[subType];
}


H323Channel * H323_UserInputCapability::CreateChannel(H323Connection &,
                                                      H323Channel::Directions,
                                                      unsigned,
                                                      const H245_H2250LogicalChannelParameters *) const
{
  PTRACE(1, "Codec\tCannot create UserInputCapability channel");
  return NULL;
}


BOOL H323_UserInputCapability::OnSendingPDU(H245_Capability & pdu) const
{
  if (subType == SignalToneRFC2833) {
    pdu.SetTag(H245_Capability::e_receiveRTPAudioTelephonyEventCapability);
    H245_AudioTelephonyEventCapability & atec = pdu;
    atec.m_dynamicRTPPayloadType = rtpPayloadType;
    atec.m_audioTelephoneEvent = "0-16"; // Support DTMF 0-9,*,#,A-D & hookflash
  }
  else {
    pdu.SetTag(H245_Capability::e_receiveUserInputCapability);
    H245_UserInputCapability & ui = pdu;
    ui.SetTag(UserInputCapabilitySubTypeCodes[subType]);
  }
  return TRUE;
}


BOOL H323_UserInputCapability::OnSendingPDU(H245_DataType &) const
{
  PTRACE(1, "Codec\tCannot have UserInputCapability in DataType");
  return FALSE;
}


BOOL H323_UserInputCapability::OnSendingPDU(H245_ModeElement &) const
{
  PTRACE(1, "Codec\tCannot have UserInputCapability in ModeElement");
  return FALSE;
}


BOOL H323_UserInputCapability::OnReceivedPDU(const H245_Capability & pdu)
{
  H323Capability::OnReceivedPDU(pdu);

  if (pdu.GetTag() == H245_Capability::e_receiveRTPAudioTelephonyEventCapability) {
    subType = SignalToneRFC2833;
    const H245_AudioTelephonyEventCapability & atec = pdu;
    rtpPayloadType = (RTP_DataFrame::PayloadTypes)(int)atec.m_dynamicRTPPayloadType;
    // Really should verify atec.m_audioTelephoneEvent here
    return TRUE;
  }

  if (pdu.GetTag() != H245_Capability::e_receiveUserInputCapability &&
      pdu.GetTag() != H245_Capability::e_receiveAndTransmitUserInputCapability)
    return FALSE;

  const H245_UserInputCapability & ui = pdu;
  return ui.GetTag() == UserInputCapabilitySubTypeCodes[subType];
}


BOOL H323_UserInputCapability::OnReceivedPDU(const H245_DataType &, BOOL)
{
  PTRACE(1, "Codec\tCannot have UserInputCapability in DataType");
  return FALSE;
}


BOOL H323_UserInputCapability::IsUsable(const H323Connection & connection) const
{
  if (connection.GetControlVersion() >= 7)
    return TRUE;

  if (connection.GetRemoteApplication().Find("AltiServ-ITG") != P_MAX_INDEX)
    return FALSE;

  return subType != SignalToneRFC2833;
}


void H323_UserInputCapability::AddAllCapabilities(H323Capabilities & capabilities,
                                                  PINDEX descriptorNum,
                                                  PINDEX simultaneous)
{
  PINDEX num = capabilities.SetCapability(descriptorNum, simultaneous, new H323_UserInputCapability(HookFlashH245));
  if (descriptorNum == P_MAX_INDEX) {
    descriptorNum = num;
    simultaneous = P_MAX_INDEX;
  }
  else if (simultaneous == P_MAX_INDEX)
    simultaneous = num+1;

  num = capabilities.SetCapability(descriptorNum, simultaneous, new H323_UserInputCapability(BasicString));
  if (simultaneous == P_MAX_INDEX)
    simultaneous = num;

  capabilities.SetCapability(descriptorNum, simultaneous, new H323_UserInputCapability(SignalToneH245));
  capabilities.SetCapability(descriptorNum, simultaneous, new H323_UserInputCapability(SignalToneRFC2833));
}


/////////////////////////////////////////////////////////////////////////////

BOOL H323SimultaneousCapabilities::SetSize(PINDEX newSize)
{
  PINDEX oldSize = GetSize();

  if (!H323CapabilitiesListArray::SetSize(newSize))
    return FALSE;

  while (oldSize < newSize) {
    H323CapabilitiesList * list = new H323CapabilitiesList;
    // The lowest level list should not delete codecs on destruction
    list->DisallowDeleteObjects();
    SetAt(oldSize++, list);
  }

  return TRUE;
}


BOOL H323CapabilitiesSet::SetSize(PINDEX newSize)
{
  PINDEX oldSize = GetSize();

  if (!H323CapabilitiesSetArray::SetSize(newSize))
    return FALSE;

  while (oldSize < newSize)
    SetAt(oldSize++, new H323SimultaneousCapabilities);

  return TRUE;
}


H323Capabilities::H323Capabilities()
{
}


H323Capabilities::H323Capabilities(const H323Connection & connection,
                                   const H245_TerminalCapabilitySet & pdu)
{
  H323Capabilities allCapabilities;
  const H323Capabilities & localCapabilities = connection.GetLocalCapabilities();
  for (PINDEX c = 0; c < localCapabilities.GetSize(); c++)
    allCapabilities.Add(allCapabilities.Copy(localCapabilities[c]));
  allCapabilities.AddAllCapabilities(connection.GetEndPoint(), 0, 0, "*");
  H323_UserInputCapability::AddAllCapabilities(allCapabilities, P_MAX_INDEX, P_MAX_INDEX);

  // Decode out of the PDU, the list of known codecs.
  if (pdu.HasOptionalField(H245_TerminalCapabilitySet::e_capabilityTable)) {
    for (PINDEX i = 0; i < pdu.m_capabilityTable.GetSize(); i++) {
      if (pdu.m_capabilityTable[i].HasOptionalField(H245_CapabilityTableEntry::e_capability)) {
        H323Capability * capability = allCapabilities.FindCapability(pdu.m_capabilityTable[i].m_capability);
        if (capability != NULL) {
          H323Capability * copy = (H323Capability *)capability->Clone();
          copy->SetCapabilityNumber(pdu.m_capabilityTable[i].m_capabilityTableEntryNumber);
          if (copy->OnReceivedPDU(pdu.m_capabilityTable[i].m_capability))
            table.Append(copy);
          else
            delete copy;
        }
      }
    }
  }

  PINDEX outerSize = pdu.m_capabilityDescriptors.GetSize();
  set.SetSize(outerSize);
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    H245_CapabilityDescriptor & desc = pdu.m_capabilityDescriptors[outer];
    if (desc.HasOptionalField(H245_CapabilityDescriptor::e_simultaneousCapabilities)) {
      PINDEX middleSize = desc.m_simultaneousCapabilities.GetSize();
      set[outer].SetSize(middleSize);
      for (PINDEX middle = 0; middle < middleSize; middle++) {
        H245_AlternativeCapabilitySet & alt = desc.m_simultaneousCapabilities[middle];
        for (PINDEX inner = 0; inner < alt.GetSize(); inner++) {
          for (PINDEX cap = 0; cap < table.GetSize(); cap++) {
            if (table[cap].GetCapabilityNumber() == alt[inner]) {
              set[outer][middle].Append(&table[cap]);
              break;
            }
          }
        }
      }
    }
  }
}


H323Capabilities::H323Capabilities(const H323Capabilities & original)
{
  operator=(original);
}


H323Capabilities & H323Capabilities::operator=(const H323Capabilities & original)
{
  RemoveAll();

  for (PINDEX i = 0; i < original.GetSize(); i++)
    Copy(original[i]);

  PINDEX outerSize = original.set.GetSize();
  set.SetSize(outerSize);
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    PINDEX middleSize = original.set[outer].GetSize();
    set[outer].SetSize(middleSize);
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      PINDEX innerSize = original.set[outer][middle].GetSize();
      for (PINDEX inner = 0; inner < innerSize; inner++)
        set[outer][middle].Append(FindCapability(original.set[outer][middle][inner].GetCapabilityNumber()));
    }
  }

  return *this;
}


void H323Capabilities::PrintOn(ostream & strm) const
{
  int indent = strm.precision()-1;
  strm << setw(indent) << " " << "Table:\n";
  for (PINDEX i = 0; i < table.GetSize(); i++)
    strm << setw(indent+2) << " " << table[i] << '\n';

  strm << setw(indent) << " " << "Set:\n";
  for (PINDEX outer = 0; outer < set.GetSize(); outer++) {
    strm << setw(indent+2) << " " << outer << ":\n";
    for (PINDEX middle = 0; middle < set[outer].GetSize(); middle++) {
      strm << setw(indent+4) << " " << middle << ":\n";
      for (PINDEX inner = 0; inner < set[outer][middle].GetSize(); inner++)
        strm << setw(indent+6) << " " << set[outer][middle][inner] << '\n';
    }
  }
}


PINDEX H323Capabilities::SetCapability(PINDEX descriptorNum,
                                       PINDEX simultaneousNum,
                                       H323Capability * capability)
{
  // Make sure capability has been added to table.
  Add(capability);

  BOOL newDescriptor = descriptorNum == P_MAX_INDEX;
  if (newDescriptor)
    descriptorNum = set.GetSize();

  // Make sure the outer array is big enough
  set.SetMinSize(descriptorNum+1);

  if (simultaneousNum == P_MAX_INDEX)
    simultaneousNum = set[descriptorNum].GetSize();

  // Make sure the middle array is big enough
  set[descriptorNum].SetMinSize(simultaneousNum+1);

  // Now we can put the new entry in.
  set[descriptorNum][simultaneousNum].Append(capability);
  return newDescriptor ? descriptorNum : simultaneousNum;
}


static BOOL MatchWildcard(const PCaselessString & str, const PStringArray & wildcard)
{
  PINDEX last = 0;
  for (PINDEX i = 0; i < wildcard.GetSize(); i++) {
    if (wildcard[i].IsEmpty())
      last = str.GetLength();
    else {
      PINDEX next = str.Find(wildcard[i], last);
      if (next == P_MAX_INDEX)
        return FALSE;
      last = next + wildcard[i].GetLength();
    }
  }

  return last == str.GetLength();
}


PINDEX H323Capabilities::AddAllCapabilities(PINDEX descriptorNum,
                                            PINDEX simultaneous,
                                            const PString & name,
                                            BOOL exact)
{
  PINDEX reply = descriptorNum == P_MAX_INDEX ? P_MAX_INDEX : simultaneous;

  PStringArray wildcard = name.Tokenise('*', FALSE);

  H323CapabilityFactory::KeyList_T stdCaps = H323CapabilityFactory::GetKeyList();
  H323CapabilityFactory::KeyList_T::const_iterator r;

  for (r = stdCaps.begin(); r != stdCaps.end(); ++r) {
    PCaselessString capName = *r;
    if ((exact ? (capName == name) : MatchWildcard(capName, wildcard)) &&
        FindCapability(capName, H323Capability::e_Unknown, exact) == NULL) {
      H323Capability * capability = H323Capability::Create(capName);
      PINDEX num = SetCapability(descriptorNum, simultaneous, capability);
      if (descriptorNum == P_MAX_INDEX) {
        reply = num;
        descriptorNum = num;
        simultaneous = P_MAX_INDEX;
      }
      else if (simultaneous == P_MAX_INDEX) {
        if (reply == P_MAX_INDEX)
          reply = num;
        simultaneous = num;
      }
    }
  }

  return reply;
}


static unsigned MergeCapabilityNumber(const H323CapabilitiesList & table,
                                      unsigned newCapabilityNumber)
{
  // Assign a unique number to the codec, check if the user wants a specific
  // value and start with that.
  if (newCapabilityNumber == 0)
    newCapabilityNumber = 1;

  PINDEX i = 0;
  while (i < table.GetSize()) {
    if (table[i].GetCapabilityNumber() != newCapabilityNumber)
      i++;
    else {
      // If it already in use, increment it
      newCapabilityNumber++;
      i = 0;
    }
  }

  return newCapabilityNumber;
}


void H323Capabilities::Add(H323Capability * capability)
{
  // See if already added, confuses things if you add the same instance twice
  if (table.GetObjectsIndex(capability) != P_MAX_INDEX)
    return;

  capability->SetCapabilityNumber(MergeCapabilityNumber(table, 1));
  table.Append(capability);

  PTRACE(3, "H323\tAdded capability: " << *capability);
}


H323Capability * H323Capabilities::Copy(const H323Capability & capability)
{
  H323Capability * newCapability = (H323Capability *)capability.Clone();
  newCapability->SetCapabilityNumber(MergeCapabilityNumber(table, capability.GetCapabilityNumber()));
  table.Append(newCapability);

  PTRACE(3, "H323\tAdded capability: " << *newCapability);
  return newCapability;
}


void H323Capabilities::Remove(H323Capability * capability)
{
  if (capability == NULL)
    return;

  PTRACE(3, "H323\tRemoving capability: " << *capability);

  unsigned capabilityNumber = capability->GetCapabilityNumber();

  for (PINDEX outer = 0; outer < set.GetSize(); outer++) {
    for (PINDEX middle = 0; middle < set[outer].GetSize(); middle++) {
      for (PINDEX inner = 0; inner < set[outer][middle].GetSize(); inner++) {
        if (set[outer][middle][inner].GetCapabilityNumber() == capabilityNumber) {
          set[outer][middle].RemoveAt(inner);
          break;
        }
      }
      if (set[outer][middle].GetSize() == 0)
        set[outer].RemoveAt(middle);
    }
    if (set[outer].GetSize() == 0)
      set.RemoveAt(outer);
  }

  table.Remove(capability);
}


void H323Capabilities::Remove(const PString & codecName)
{
  H323Capability * cap = FindCapability(codecName);
  while (cap != NULL) {
    Remove(cap);
    cap = FindCapability(codecName);
  }
}


void H323Capabilities::Remove(const PStringArray & codecNames)
{
  for (PINDEX i = 0; i < codecNames.GetSize(); i++)
    Remove(codecNames[i]);
}


void H323Capabilities::RemoveAll()
{
  table.RemoveAll();
  set.RemoveAll();
}


H323Capability * H323Capabilities::FindCapability(unsigned capabilityNumber) const
{
  PTRACE(4, "H323\tFindCapability: " << capabilityNumber);

  for (PINDEX i = 0; i < table.GetSize(); i++) {
    if (table[i].GetCapabilityNumber() == capabilityNumber) {
      PTRACE(3, "H323\tFound capability: " << table[i]);
      return &table[i];
    }
  }

  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const PString & formatName,
                                                  H323Capability::CapabilityDirection direction,
                                                  BOOL exact) const
{
  PTRACE(4, "H323\tFindCapability: \"" << formatName << '"');

  PStringArray wildcard = formatName.Tokenise('*', FALSE);

  for (PINDEX i = 0; i < table.GetSize(); i++) {
    PCaselessString str = table[i].GetFormatName();
    if ((exact ? (str == formatName) : MatchWildcard(str, wildcard)) &&
          (direction == H323Capability::e_Unknown ||
           table[i].GetCapabilityDirection() == direction)) {
      PTRACE(3, "H323\tFound capability: " << table[i]);
      return &table[i];
    }
  }

  return NULL;
}


H323Capability * H323Capabilities::FindCapability(
                              H323Capability::CapabilityDirection direction) const
{
  PTRACE(4, "H323\tFindCapability: \"" << direction << '"');

  for (PINDEX i = 0; i < table.GetSize(); i++) {
    if (table[i].GetCapabilityDirection() == direction) {
      PTRACE(3, "H323\tFound capability: " << table[i]);
      return &table[i];
    }
  }

  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const H323Capability & capability) const
{
  PTRACE(4, "H323\tFindCapability: " << capability);

  for (PINDEX i = 0; i < table.GetSize(); i++) {
    if (table[i] == capability) {
      PTRACE(3, "H323\tFound capability: " << table[i]);
      return &table[i];
    }
  }

  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const H245_Capability & cap) const
{
  for (PINDEX i = 0; i < table.GetSize(); i++) {
    H323Capability & capability = table[i];
    switch (cap.GetTag()) {
      case H245_Capability::e_receiveAudioCapability :
      case H245_Capability::e_transmitAudioCapability :
      case H245_Capability::e_receiveAndTransmitAudioCapability :
        if (capability.GetMainType() == H323Capability::e_Audio) {
          const H245_AudioCapability & audio = cap;
          PTRACE(4, "H323\tFindCapability: " << audio.GetTagName());
          if (capability.IsMatch(audio))
            return &capability;
        }
        break;

      case H245_Capability::e_receiveVideoCapability :
      case H245_Capability::e_transmitVideoCapability :
      case H245_Capability::e_receiveAndTransmitVideoCapability :
        if (capability.GetMainType() == H323Capability::e_Video) {
          const H245_VideoCapability & video = cap;
          PTRACE(4, "H323\tFindCapability: " << video.GetTagName());
          if (capability.IsMatch(video))
            return &capability;
        }
        break;

      case H245_Capability::e_receiveDataApplicationCapability :
      case H245_Capability::e_transmitDataApplicationCapability :
      case H245_Capability::e_receiveAndTransmitDataApplicationCapability :
        if (capability.GetMainType() == H323Capability::e_Data) {
          const H245_DataApplicationCapability & data = cap;
          PTRACE(4, "H323\tFindCapability: " << data.m_application.GetTagName());
          if (capability.IsMatch(data.m_application))
            return &capability;
        }
        break;

      case H245_Capability::e_receiveUserInputCapability :
      case H245_Capability::e_transmitUserInputCapability :
      case H245_Capability::e_receiveAndTransmitUserInputCapability :
        if (capability.GetMainType() == H323Capability::e_UserInput) {
          const H245_UserInputCapability & ui = cap;
          PTRACE(4, "H323\tFindCapability: " << ui.GetTagName());
          if (capability.IsMatch(ui))
            return &capability;
        }
        break;

      case H245_Capability::e_receiveRTPAudioTelephonyEventCapability :
        PTRACE(4, "H323\tFindCapability: " << cap.GetTagName());
        return FindCapability(H323Capability::e_UserInput, SignalToneRFC2833_SubType);

      default :
        PTRACE(4, "H323\tFindCapability: " << cap.GetTagName());
        break;
    }
  }

  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const H245_DataType & dataType) const
{
  for (PINDEX i = 0; i < table.GetSize(); i++) {
    H323Capability & capability = table[i];
    BOOL checkExact;
    switch (dataType.GetTag()) {
      case H245_DataType::e_audioData :
      {
        const H245_AudioCapability & audio = dataType;
        PTRACE(4, "H323\tFindCapability: " << dataType.GetTagName() << ", type " << audio.GetTagName());
        checkExact = capability.IsMatch(audio);
        break;
      }

      case H245_DataType::e_videoData :
      {
        const H245_VideoCapability & video = dataType;
        PTRACE(4, "H323\tFindCapability: " << dataType.GetTagName() << ", type " << video.GetTagName());
        checkExact = capability.IsMatch(video);
        break;
      }

      case H245_DataType::e_data :
      {
        const H245_DataApplicationCapability & data = dataType;
        PTRACE(4, "H323\tFindCapability: " << dataType.GetTagName() << ", type " << data.m_application.GetTagName());
        checkExact = capability.IsMatch(data.m_application);
        break;
      }

      default :
        checkExact = FALSE;
        PTRACE(4, "H323\tFindCapability: " << dataType.GetTagName() << ", unknown type");
    }

    if (checkExact) {
      H323Capability * compare = (H323Capability *)capability.Clone();
      if (compare->OnReceivedPDU(dataType, FALSE) && *compare == capability) {
        delete compare;
        PTRACE(3, "H323\tFound capability: " << capability);
        return &capability;
      }
      delete compare;
    }
  }

  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const H245_ModeElement & modeElement) const
{
  PTRACE(4, "H323\tFindCapability: " << modeElement.m_type.GetTagName());

  for (PINDEX i = 0; i < table.GetSize(); i++) {
    H323Capability & capability = table[i];
    switch (modeElement.m_type.GetTag()) {
      case H245_ModeElementType::e_audioMode :
        if (capability.GetMainType() == H323Capability::e_Audio) {
          const H245_AudioMode & audio = modeElement.m_type;
          if (capability.IsMatch(audio))
            return &capability;
        }
        break;

      case H245_ModeElementType::e_videoMode :
        if (capability.GetMainType() == H323Capability::e_Video) {
          const H245_VideoMode & video = modeElement.m_type;
          if (capability.IsMatch(video))
            return &capability;
        }
        break;

      case H245_ModeElementType::e_dataMode :
        if (capability.GetMainType() == H323Capability::e_Data) {
          const H245_DataMode & data = modeElement.m_type;
          if (capability.IsMatch(data.m_application))
            return &capability;
        }
        break;

      default :
        break;
    }
  }

  return NULL;
}


H323Capability * H323Capabilities::FindCapability(H323Capability::MainTypes mainType,
                                                  unsigned subType) const
{
  PTRACE(4, "H323\tFindCapability: " << mainType << " subtype=" << subType);

  for (PINDEX i = 0; i < table.GetSize(); i++) {
    H323Capability & capability = table[i];
    if (capability.GetMainType() == mainType &&
                        (subType == UINT_MAX || capability.GetSubType() == subType)) {
      PTRACE(3, "H323\tFound capability: " << capability);
      return &capability;
    }
  }

  return NULL;
}


void H323Capabilities::BuildPDU(const H323Connection & connection,
                                H245_TerminalCapabilitySet & pdu) const
{
  PINDEX tableSize = table.GetSize();
  PINDEX setSize = set.GetSize();
  PAssert((tableSize > 0) == (setSize > 0), PLogicError);
  if (tableSize == 0 || setSize == 0)
    return;

  // Set the table of capabilities
  pdu.IncludeOptionalField(H245_TerminalCapabilitySet::e_capabilityTable);

  H245_H2250Capability & h225_0 = pdu.m_multiplexCapability;
  PINDEX rtpPacketizationCount = 0;

  // encode the capabilities
  PINDEX count = 0;
  PINDEX i;
  for (i = 0; i < tableSize; i++) {
    H323Capability & capability = table[i];
    if (capability.IsUsable(connection)) {
      pdu.m_capabilityTable.SetSize(count+1);
      H245_CapabilityTableEntry & entry = pdu.m_capabilityTable[count++];
      entry.m_capabilityTableEntryNumber = capability.GetCapabilityNumber();
      entry.IncludeOptionalField(H245_CapabilityTableEntry::e_capability);
      capability.OnSendingPDU(entry.m_capability);

      h225_0.m_mediaPacketizationCapability.m_rtpPayloadType.SetSize(rtpPacketizationCount+1);
      if (H323SetRTPPacketization(h225_0.m_mediaPacketizationCapability.m_rtpPayloadType[rtpPacketizationCount],
                                                    capability.GetMediaFormat(), RTP_DataFrame::IllegalPayloadType)) {
        // Check if already in list
        PINDEX test;
        for (test = 0; test < rtpPacketizationCount; test++) {
          if (h225_0.m_mediaPacketizationCapability.m_rtpPayloadType[test] == h225_0.m_mediaPacketizationCapability.m_rtpPayloadType[rtpPacketizationCount])
            break;
        }
        if (test == rtpPacketizationCount)
          rtpPacketizationCount++;
      }
    }
  }

  // Have some mediaPacketizations to include.
  if (rtpPacketizationCount > 0) {
    h225_0.m_mediaPacketizationCapability.m_rtpPayloadType.SetSize(rtpPacketizationCount);
    h225_0.m_mediaPacketizationCapability.IncludeOptionalField(H245_MediaPacketizationCapability::e_rtpPayloadType);
  }

  // Set the sets of compatible capabilities
  pdu.IncludeOptionalField(H245_TerminalCapabilitySet::e_capabilityDescriptors);

  pdu.m_capabilityDescriptors.SetSize(setSize);
  for (PINDEX outer = 0; outer < setSize; outer++) {
    H245_CapabilityDescriptor & desc = pdu.m_capabilityDescriptors[outer];
    desc.m_capabilityDescriptorNumber = (unsigned)(outer + 1);
    desc.IncludeOptionalField(H245_CapabilityDescriptor::e_simultaneousCapabilities);
    PINDEX middleSize = set[outer].GetSize();
    desc.m_simultaneousCapabilities.SetSize(middleSize);
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      H245_AlternativeCapabilitySet & alt = desc.m_simultaneousCapabilities[middle];
      PINDEX innerSize = set[outer][middle].GetSize();
      alt.SetSize(innerSize);
      count = 0;
      for (PINDEX inner = 0; inner < innerSize; inner++) {
        H323Capability & capability = set[outer][middle][inner];
        if (capability.IsUsable(connection)) {
          alt.SetSize(count+1);
          alt[count++] = capability.GetCapabilityNumber();
        }
      }
    }
  }
}


BOOL H323Capabilities::Merge(const H323Capabilities & newCaps)
{
  PTRACE_IF(4, !table.IsEmpty(), "H245\tCapability merge of:\n" << newCaps
            << "\nInto:\n" << *this);

  // Add any new capabilities not already in set.
  PINDEX i;
  for (i = 0; i < newCaps.GetSize(); i++) {
    // Only add if not already in
    if (!FindCapability(newCaps[i]))
      Copy(newCaps[i]);
  }

  // This should merge instead of just adding to it.
  PINDEX outerSize = newCaps.set.GetSize();
  PINDEX outerBase = set.GetSize();
  set.SetSize(outerBase+outerSize);
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    PINDEX middleSize = newCaps.set[outer].GetSize();
    set[outerBase+outer].SetSize(middleSize);
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      PINDEX innerSize = newCaps.set[outer][middle].GetSize();
      for (PINDEX inner = 0; inner < innerSize; inner++) {
        H323Capability * cap = FindCapability(newCaps.set[outer][middle][inner].GetCapabilityNumber());
        if (cap != NULL)
          set[outerBase+outer][middle].Append(cap);
      }
    }
  }

  PTRACE_IF(4, !table.IsEmpty(), "H245\tCapability merge result:\n" << *this);
  PTRACE(3, "H245\tReceived capability set, is "
                 << (table.IsEmpty() ? "rejected" : "accepted"));
  return !table.IsEmpty();
}


void H323Capabilities::Reorder(const PStringArray & preferenceOrder)
{
  if (preferenceOrder.IsEmpty())
    return;

  table.DisallowDeleteObjects();

  PINDEX preference = 0;
  PINDEX base = 0;

  for (preference = 0; preference < preferenceOrder.GetSize(); preference++) {
    PStringArray wildcard = preferenceOrder[preference].Tokenise('*', FALSE);
    for (PINDEX idx = base; idx < table.GetSize(); idx++) {
      PCaselessString str = table[idx].GetFormatName();
      if (MatchWildcard(str, wildcard)) {
        if (idx != base)
          table.InsertAt(base, table.RemoveAt(idx));
        base++;
      }
    }
  }

  for (PINDEX outer = 0; outer < set.GetSize(); outer++) {
    for (PINDEX middle = 0; middle < set[outer].GetSize(); middle++) {
      H323CapabilitiesList & list = set[outer][middle];
      for (PINDEX idx = 0; idx < table.GetSize(); idx++) {
        for (PINDEX inner = 0; inner < list.GetSize(); inner++) {
          if (&table[idx] == &list[inner]) {
            list.Append(list.RemoveAt(inner));
            break;
          }
        }
      }
    }
  }

  table.AllowDeleteObjects();
}


BOOL H323Capabilities::IsAllowed(const H323Capability & capability)
{
  return IsAllowed(capability.GetCapabilityNumber());
}


BOOL H323Capabilities::IsAllowed(const unsigned a_capno)
{
  // Check that capno is actually in the set
  PINDEX outerSize = set.GetSize();
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    PINDEX middleSize = set[outer].GetSize();
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      PINDEX innerSize = set[outer][middle].GetSize();
      for (PINDEX inner = 0; inner < innerSize; inner++) {
        if (a_capno == set[outer][middle][inner].GetCapabilityNumber()) {
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}


BOOL H323Capabilities::IsAllowed(const H323Capability & capability1,
                                 const H323Capability & capability2)
{
  return IsAllowed(capability1.GetCapabilityNumber(),
                   capability2.GetCapabilityNumber());
}


BOOL H323Capabilities::IsAllowed(const unsigned a_capno1, const unsigned a_capno2)
{
  if (a_capno1 == a_capno2) {
    PTRACE(2, "H323\tH323Capabilities::IsAllowed() capabilities are the same.");
    return TRUE;
  }

  PINDEX outerSize = set.GetSize();
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    PINDEX middleSize = set[outer].GetSize();
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      PINDEX innerSize = set[outer][middle].GetSize();
      for (PINDEX inner = 0; inner < innerSize; inner++) {
        if (a_capno1 == set[outer][middle][inner].GetCapabilityNumber()) {
          /* Now go searching for the other half... */
          for (PINDEX middle2 = 0; middle2 < middleSize; ++middle2) {
            if (middle != middle2) {
              PINDEX innerSize2 = set[outer][middle2].GetSize();
              for (PINDEX inner2 = 0; inner2 < innerSize2; ++inner2) {
                if (a_capno2 == set[outer][middle2][inner2].GetCapabilityNumber()) {
                  return TRUE;
                }
              }
            }
          }
        }
      }
    }
  }
  return FALSE;
}


OpalMediaFormatList H323Capabilities::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  for (PINDEX i = 0; i < table.GetSize(); i++)
    formats += table[i].GetMediaFormat();

  return formats;
}


/////////////////////////////////////////////////////////////////////////////

#ifndef PASN_NOPRINTON


struct msNonStandardCodecDef {
  const char * name;
  BYTE sig[2];
};


static msNonStandardCodecDef msNonStandardCodec[] = {
  { "L&H CELP 4.8k", { 0x01, 0x11 } },
  { "ADPCM",         { 0x02, 0x00 } },
  { "L&H CELP 8k",   { 0x02, 0x11 } },
  { "L&H CELP 12k",  { 0x03, 0x11 } },
  { "L&H CELP 16k",  { 0x04, 0x11 } },
  { "IMA-ADPCM",     { 0x11, 0x00 } },
  { "GSM",           { 0x31, 0x00 } },
  { NULL,            { 0,    0    } }
};

void H245_AudioCapability::PrintOn(ostream & strm) const
{
  strm << GetTagName();

  // tag 0 is nonstandard
  if (tag == 0) {

    H245_NonStandardParameter & param = (H245_NonStandardParameter &)GetObject();
    const PBYTEArray & data = param.m_data;

    switch (param.m_nonStandardIdentifier.GetTag()) {
      case H245_NonStandardIdentifier::e_h221NonStandard:
        {
          H245_NonStandardIdentifier_h221NonStandard & h221 = param.m_nonStandardIdentifier;

          // Microsoft is 181/0/21324
          if ((h221.m_t35CountryCode   == 181) &&
              (h221.m_t35Extension     == 0) &&
              (h221.m_manufacturerCode == 21324)
            ) {
            PString name = "Unknown";
            PINDEX i;
            if (data.GetSize() >= 21) {
              for (i = 0; msNonStandardCodec[i].name != NULL; i++) {
                if ((data[20] == msNonStandardCodec[i].sig[0]) && 
                    (data[21] == msNonStandardCodec[i].sig[1])) {
                  name = msNonStandardCodec[i].name;
                  break;
                }
              }
            }
            strm << (PString(" [Microsoft") & name) << "]";
          }

          // Equivalence is 9/0/61
          else if ((h221.m_t35CountryCode   == 9) &&
                   (h221.m_t35Extension     == 0) &&
                   (h221.m_manufacturerCode == 61)
                  ) {
            PString name;
            if (data.GetSize() > 0)
              name = PString((const char *)(const BYTE *)data, data.GetSize());
            strm << " [Equivalence " << name << "]";
          }

          // Xiph is 181/0/38
          else if ((h221.m_t35CountryCode   == 181) &&
                   (h221.m_t35Extension     == 0) &&
                   (h221.m_manufacturerCode == 38)
                  ) {
            PString name;
            if (data.GetSize() > 0)
              name = PString((const char *)(const BYTE *)data, data.GetSize());
            strm << " [Xiph " << name << "]";
          }

          // Cisco is 181/0/18
          else if ((h221.m_t35CountryCode   == 181) &&
                   (h221.m_t35Extension     == 0) &&
                   (h221.m_manufacturerCode == 18)
                  ) {
            PString name;
            if (data.GetSize() > 0)
              name = PString((const char *)(const BYTE *)data, data.GetSize());
            strm << " [Cisco " << name << "]";
          }

        }
        break;
      default:
        break;
    }
  }

  if (choice == NULL)
    strm << " (NULL)";
  else {
    strm << ' ' << *choice;
  }

  //PASN_Choice::PrintOn(strm);
}
#endif
