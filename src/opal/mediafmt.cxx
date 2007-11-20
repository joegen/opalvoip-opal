/*
 * mediafmt.cxx
 *
 * Media Format descriptions
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Log: mediafmt.cxx,v $
 * Revision 2.78  2007/09/25 19:35:39  csoutheren
 * Fix compilation when using --disable-audio
 *
 * Revision 2.77  2007/09/21 00:51:38  rjongbloed
 * Fixed weird divide by zero error on clock rate.
 *
 * Revision 2.76  2007/09/20 04:26:36  rjongbloed
 * Fixed missing mutex in video media format merge.
 *
 * Revision 2.75  2007/09/18 03:20:11  rjongbloed
 * Allow frame width/height to be altered by user.
 *
 * Revision 2.74  2007/09/10 03:15:04  rjongbloed
 * Fixed issues in creating and subsequently using correctly unique
 *   payload types in OpalMediaFormat instances and transcoders.
 *
 * Revision 2.73  2007/09/10 00:16:16  rjongbloed
 * Fixed allocating dynamic payload types to media formats maked as internal via
 *   having an IllegalPayloadType
 * Added extra fields to the PrintOptions() function.
 *
 * Revision 2.72  2007/09/07 05:40:12  rjongbloed
 * Fixed issue where OpalMediaOptions are lost when an OpalMediaFormat
 *    is added to an OpalMediaFormatList.
 * Also fixes a memory leak in GetAllRegisteredMediaFormats().
 *
 * Revision 2.71  2007/09/05 07:56:03  csoutheren
 * Change default frame size for PCM-16 to 1
 *
 * Revision 2.70  2007/08/17 07:50:09  dsandras
 * Applied patch from Matthias Schneider <ma30002000 yahoo.de>. Thanks!
 *
 * Revision 2.69  2007/08/17 07:01:18  csoutheren
 * Shortcut media format copy when src and dest are the same
 *
 * Revision 2.68  2007/08/08 11:17:50  csoutheren
 * Removed warning
 *
 * Revision 2.67  2007/08/02 07:54:49  csoutheren
 * Add function to print options on media format
 *
 * Revision 2.66  2007/07/24 12:57:55  rjongbloed
 * Made sure all integer OpalMediaOptions are unsigned so is compatible with H.245 generic capabilities.
 *
 * Revision 2.65  2007/06/27 07:56:08  rjongbloed
 * Add new OpalMediaOption for octet strings (simple block of bytes).
 *
 * Revision 2.64  2007/06/22 05:41:47  rjongbloed
 * Major codec API update:
 *   Automatically map OpalMediaOptions to SIP/SDP FMTP parameters.
 *   Automatically map OpalMediaOptions to H.245 Generic Capability parameters.
 *   Largely removed need to distinguish between SIP and H.323 codecs.
 *   New mechanism for setting OpalMediaOptions from within a plug in.
 *
 * Revision 2.63  2007/06/16 21:37:01  dsandras
 * Added H.264 support thanks to Matthias Schneider <ma30002000 yahoo de>.
 * Thanks a lot !
 *
 * Baseline Profile:
 * no B-frames
 * We make use of the baseline profile (which is the designated profile for interactive vide) ,
 * that means:
 * no B-Frames (too much latency in interactive video)
 * CBR (we want to get the max. quality making use of all the bitrate that is available)
 * We allow one exeption: configuring a bitrate of > 786 kbit/s
 *
 * This plugin implements
 * - Single Time Aggregation Packets A
 * - Single NAL units
 * - Fragmentation Units
 * like described in RFC3984
 *
 * It requires x264 and ffmpeg.
 *
 * Revision 2.62  2007/04/10 05:15:54  rjongbloed
 * Fixed issue with use of static C string variables in DLL environment,
 *   must use functional interface for correct initialisation.
 *
 * Revision 2.61  2007/03/13 00:33:11  csoutheren
 * Simple but messy changes to allow compile time removal of protocol
 * options such as H.450 and H.460
 * Fix MakeConnection overrides
 *
 * Revision 2.60  2007/02/14 06:51:44  csoutheren
 * Extended FindFormat to allow finding multiple matching formats
 *
 * Revision 2.59  2007/02/10 18:14:32  hfriederich
 * Add copy constructor to have consistent code with assignment operator.
 * Only make options unique when they actually differ
 *
 * Revision 2.58  2006/12/08 07:33:13  csoutheren
 * Fix problem with wideband audio plugins and sound channel
 *
 * Revision 2.57  2006/11/21 01:01:00  csoutheren
 * Ensure SDP only uses codecs that are valid for SIP
 *
 * Revision 2.56  2006/10/10 07:18:18  csoutheren
 * Allow compilation with and without various options
 *
 * Revision 2.55  2006/09/11 04:48:55  csoutheren
 * Fixed problem with cloning plugin media formats
 *
 * Revision 2.54  2006/09/07 09:05:44  csoutheren
 * Fix case significance in IsValidForProtocol
 *
 * Revision 2.53  2006/09/06 22:36:11  csoutheren
 * Fix problem with IsValidForProtocol on video codecs
 *
 * Revision 2.52  2006/09/05 22:50:05  csoutheren
 * Make sure codecs match full name if specified, or not at all
 *
 * Revision 2.51  2006/09/05 06:21:07  csoutheren
 * Add useful comment
 *
 * Revision 2.50  2006/08/24 02:19:56  csoutheren
 * Fix problem with calculating the bandwidth of wide-band codecs
 *
 * Revision 2.49  2006/08/20 03:45:54  csoutheren
 * Add OpalMediaFormat::IsValidForProtocol to allow plugin codecs to be enabled only for certain protocols
 * rather than relying on the presence of the IANA rtp encoding name field
 *
 * Revision 2.48  2006/08/15 23:35:36  csoutheren
 * Fixed problem with OpalMediaFormat compare which stopped RTP payload map from working
 *   which disabled iLBC codec on SIP
 *
 * Revision 2.47  2006/08/01 12:46:32  rjongbloed
 * Added build solution for plug ins
 * Removed now redundent code due to plug ins addition
 *
 * Revision 2.46  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.45  2006/07/14 04:22:43  csoutheren
 * Applied 1517397 - More Phobos stability fix
 * Thanks to Dinis Rosario
 *
 * Revision 2.44  2006/04/09 12:01:44  rjongbloed
 * Added missing Clone() functions so media options propagate correctly.
 *
 * Revision 2.43  2006/03/20 10:37:47  csoutheren
 * Applied patch #1453753 - added locking on media stream manipulation
 * Thanks to Dinis Rosario
 *
 * Revision 2.42.2.4  2006/04/06 05:33:08  csoutheren
 * Backports from CVS head up to Plugin_Merge2
 *
 * Revision 2.42.2.3  2006/04/06 01:21:20  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 2.42.2.2  2006/03/16 07:06:00  csoutheren
 * Initial support for audio plugins
 *
 * Revision 2.42.2.1  2006/03/13 07:20:28  csoutheren
 * Added OpalMediaFormat clone function
 *
 * Revision 2.44  2006/04/09 12:01:44  rjongbloed
 * Added missing Clone() functions so media options propagate correctly.
 *
 * Revision 2.43  2006/03/20 10:37:47  csoutheren
 * Applied patch #1453753 - added locking on media stream manipulation
 * Thanks to Dinis Rosario
 *
 * Revision 2.42  2006/02/21 09:38:28  csoutheren
 * Fix problem with incorrect timestamps for uLaw and ALaw
 *
 * Revision 2.41  2006/02/13 03:46:17  csoutheren
 * Added initialisation stuff to make sure that everything works OK
 *
 * Revision 2.40  2006/01/23 22:53:14  csoutheren
 * Reverted previous change that prevents multiple codecs from being removed using the
 *  removeMask
 *
 * Revision 2.39  2005/12/27 20:50:46  dsandras
 * Added clockRate parameter to the media format. Added new merging method that
 * merges the parameter option from the source into the destination.
 *
 * Revision 2.38  2005/12/24 17:50:20  dsandras
 * Added clockRate parameter support to allow wideband audio codecs.
 *
 * Revision 2.37  2005/12/06 21:38:16  dsandras
 * Fixed SetMediaFormatMask thanks for Frederic Heem <frederic.heem _Atttt_ telsey.it>. Thanks! (Patch #1368040).
 *
 * Revision 2.36  2005/09/13 20:48:22  dominance
 * minor cleanups needed to support mingw compilation. Thanks goes to Julien Puydt.
 *
 * Revision 2.35  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.34  2005/09/02 14:49:21  csoutheren
 * Fixed link problem in Linux
 *
 * Revision 2.33  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.32  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.31  2005/08/24 10:18:23  rjongbloed
 * Fix incorrect session ID for video media format, doesn't work if thinks is audio!
 *
 * Revision 2.30  2005/08/20 07:33:30  rjongbloed
 * Added video specific OpalMediaFormat
 *
 * Revision 2.29  2005/06/02 13:20:46  rjongbloed
 * Added minimum and maximum check to media format options.
 * Added ability to set the options on the primordial media format list.
 *
 * Revision 2.28  2005/03/12 00:33:28  csoutheren
 * Fixed problems with STL compatibility on MSVC 6
 * Fixed problems with video streams
 * Thanks to Adrian Sietsma
 *
 * Revision 2.27  2005/02/21 20:27:18  dsandras
 * Fixed compilation with gcc.
 *
 * Revision 2.26  2005/02/21 12:20:05  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.25  2004/10/24 10:46:41  rjongbloed
 * Back out change of strcasecmp to strcmp for WinCE
 *
 * Revision 2.24  2004/10/23 11:42:38  ykiryanov
 * Added ifdef _WIN32_WCE for PocketPC 2003 SDK port
 *
 * Revision 2.23  2004/07/11 12:32:51  rjongbloed
 * Added functions to add/subtract lists of media formats from a media format list
 *
 * Revision 2.22  2004/05/03 00:59:19  csoutheren
 * Fixed problem with OpalMediaFormat::GetMediaFormatsList
 * Added new version of OpalMediaFormat::GetMediaFormatsList that minimses copying
 *
 * Revision 2.21  2004/03/25 11:48:48  rjongbloed
 * Changed PCM-16 from IllegalPayloadType to MaxPayloadType to avoid problems
 *   in other parts of the code.
 *
 * Revision 2.20  2004/03/22 11:32:42  rjongbloed
 * Added new codec type for 16 bit Linear PCM as must distinguish between the internal
 *   format used by such things as the sound card and the RTP payload format which
 *   is always big endian.
 *
 * Revision 2.19  2004/02/13 22:15:35  csoutheren
 * Changed stricmp to strcascmp thanks to Diana Cionoiu
 *
 * Revision 2.18  2004/02/07 02:18:19  rjongbloed
 * Improved searching for media format to use payload type AND the encoding name.
 *
 * Revision 2.17  2003/03/17 10:13:41  robertj
 * Fixed mutex problem with media format database.
 *
 * Revision 2.16  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.15  2002/11/10 11:33:19  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.14  2002/09/04 06:01:49  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.13  2002/07/01 04:56:33  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.12  2002/03/27 05:36:44  robertj
 * Set RFC2833 payload type to be 101 for Cisco compatibility
 *
 * Revision 2.11  2002/02/19 07:36:51  robertj
 * Added OpalRFC2833 as a OpalMediaFormat variable.
 *
 * Revision 2.10  2002/02/11 09:32:13  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.9  2002/01/22 05:14:38  robertj
 * Added RTP encoding name string to media format database.
 * Changed time units to clock rate in Hz.
 *
 * Revision 2.8  2002/01/14 06:35:58  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.7  2001/11/15 06:55:26  robertj
 * Fixed Reorder() function so reorders EVERY format that matches wildcard.
 *
 * Revision 2.6  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.5  2001/10/04 00:43:57  robertj
 * Added function to remove wildcard from list.
 * Added constructor to make a list with one format in it.
 * Fixed wildcard matching so trailing * works.
 * Optimised reorder so does not reorder if already in order.
 *
 * Revision 2.4  2001/08/23 05:51:17  robertj
 * Completed implementation of codec reordering.
 *
 * Revision 2.3  2001/08/22 03:51:44  robertj
 * Added functions to look up media format by payload type.
 *
 * Revision 2.2  2001/08/01 06:22:07  robertj
 * Fixed GNU warning.
 *
 * Revision 2.1  2001/08/01 05:45:34  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.11  2002/12/03 09:20:01  craigs
 * Fixed problem with RFC2833 and a dynamic RTP type using the same RTP payload number
 *
 * Revision 1.10  2002/12/02 03:06:26  robertj
 * Fixed over zealous removal of code when NO_AUDIO_CODECS set.
 *
 * Revision 1.9  2002/10/30 05:54:17  craigs
 * Fixed compatibilty problems with G.723.1 6k3 and 5k3
 *
 * Revision 1.8  2002/08/05 10:03:48  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.7  2002/06/25 08:30:13  robertj
 * Changes to differentiate between stright G.723.1 and G.723.1 Annex A using
 *   the OLC dataType silenceSuppression field so does not send SID frames
 *   to receiver codecs that do not understand them.
 *
 * Revision 1.6  2002/01/22 07:08:26  robertj
 * Added IllegalPayloadType enum as need marker for none set
 *   and MaxPayloadType is a legal value.
 *
 * Revision 1.5  2001/12/11 04:27:28  craigs
 * Added support for 5.3kbps G723.1
 *
 * Revision 1.4  2001/09/21 02:51:45  robertj
 * Implemented static object for all "known" media formats.
 * Added default session ID to media format description.
 *
 * Revision 1.3  2001/05/11 04:43:43  robertj
 * Added variable names for standard PCM-16 media format name.
 *
 * Revision 1.2  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.1  2001/01/25 07:27:16  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediafmt.h"
#pragma implementation "mediacmd.h"
#endif

#include <opal/mediafmt.h>
#include <opal/mediacmd.h>
#include <codec/opalwavfile.h>
#include <ptclib/cypher.h>


#define new PNEW


namespace PWLibStupidLinkerHacks {
extern int opalLoader;

static class InstantiateMe
{
  public:
    InstantiateMe()
    { 
      opalLoader = 1; 
    }
} instance;

}; // namespace PWLibStupidLinkerHacks

/////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

#define AUDIO_FORMAT(name, rtpPayloadType, encodingName, frameSize, frameTime, rxFrames, txFrames, maxFrames, clock) \
  const OpalAudioFormat & GetOpal##name() \
  { \
    static const OpalAudioFormat name(OPAL_##name, RTP_DataFrame::rtpPayloadType, \
                                    encodingName, frameSize, frameTime, rxFrames, txFrames, maxFrames, clock); \
    return name; \
  }

AUDIO_FORMAT(PCM16,          MaxPayloadType, "",     16, 8,  240,  1, 256,  8000);
AUDIO_FORMAT(PCM16_16KHZ,    MaxPayloadType, "",     16, 8,  240,  1, 256, 16000);
AUDIO_FORMAT(L16_MONO_8KHZ,  L16_Mono,       "L16",  16, 8,  240, 30, 256,  8000);
AUDIO_FORMAT(L16_MONO_16KHZ, L16_Mono,       "L16",  16, 4,  120, 15, 256, 16000);
AUDIO_FORMAT(G711_ULAW_64K,  PCMU,           "PCMU",  8, 8,  240, 30, 256,  8000);
AUDIO_FORMAT(G711_ALAW_64K,  PCMA,           "PCMA",  8, 8,  240, 30, 256,  8000);
AUDIO_FORMAT(G728,           G728,           "G728",  5, 20, 100, 10, 256,  8000);
AUDIO_FORMAT(G729,           G729,           "G729", 10, 80,  24,  5, 256,  8000);
AUDIO_FORMAT(G729A,          G729,           "G729", 10, 80,  24,  5, 256,  8000);
AUDIO_FORMAT(G729B,          G729,           "G729", 10, 80,  24,  5, 256,  8000);
AUDIO_FORMAT(G729AB,         G729,           "G729", 10, 80,  24,  5, 256,  8000);
AUDIO_FORMAT(G7231_6k3,      G7231,          "G723", 24, 240,  8,  3, 256,  8000);
AUDIO_FORMAT(G7231_5k3,      G7231,          "G723", 24, 240,  8,  3, 256,  8000);
AUDIO_FORMAT(G7231A_6k3,     G7231,          "G723", 24, 240,  8,  3, 256,  8000);
AUDIO_FORMAT(G7231A_5k3,     G7231,          "G723", 24, 240,  8,  3, 256,  8000);
AUDIO_FORMAT(GSM0610,        GSM,            "GSM",  33, 160,  7,  4, 7  ,  8000);

#endif

const OpalMediaFormat & GetOpalRFC2833()
{
  static const OpalMediaFormat RFC2833(
    OPAL_RFC2833,
    0,
    (RTP_DataFrame::PayloadTypes)101,  // Set to this for Cisco compatibility
    "telephone-event",
    TRUE,   // Needs jitter
    32*(1000/50), // bits/sec  (32 bits every 50ms)
    4,      // bytes/frame
    150*8,  // 150 millisecond
    OpalMediaFormat::AudioClockRate
  );
  return RFC2833;
}

const OpalMediaFormat & GetOpalCiscoNSE()
{
  static const OpalMediaFormat CiscoNSE(
    OPAL_CISCONSE,
    0,
    (RTP_DataFrame::PayloadTypes)100,  // Set to this for Cisco compatibility
    "NSE",
    TRUE,   // Needs jitter
    32*(1000/50), // bits/sec  (32 bits every 50ms)
    4,      // bytes/frame
    150*8,  // 150 millisecond
    OpalMediaFormat::AudioClockRate
  );
  return CiscoNSE;
}

static OpalMediaFormatList & GetMediaFormatsList()
{
  static class OpalMediaFormatListMaster : public OpalMediaFormatList
  {
    public:
      OpalMediaFormatListMaster()
      {
        DisallowDeleteObjects();
      }
  } registeredFormats;

  return registeredFormats;
}


static PMutex & GetMediaFormatsListMutex()
{
  static PMutex mutex;
  return mutex;
}


/////////////////////////////////////////////////////////////////////////////

OpalMediaOption::OpalMediaOption(const PString & name)
  : m_name(name)
  , m_readOnly(false)
  , m_merge(NoMerge)
{
}


OpalMediaOption::OpalMediaOption(const char * name, bool readOnly, MergeType merge)
  : m_name(name)
  , m_readOnly(readOnly)
  , m_merge(merge)
{
  m_name.Replace("=", "_", TRUE);
  memset(&m_H245Generic, 0, sizeof(m_H245Generic));
}


PObject::Comparison OpalMediaOption::Compare(const PObject & obj) const
{
  const OpalMediaOption * otherOption = PDownCast(const OpalMediaOption, &obj);
  if (otherOption == NULL)
    return GreaterThan;
  return m_name.Compare(otherOption->m_name);
}


bool OpalMediaOption::Merge(const OpalMediaOption & option)
{
  switch (m_merge) {
    case MinMerge :
      if (CompareValue(option) == GreaterThan)
        Assign(option);
      break;

    case MaxMerge :
      if (CompareValue(option) == LessThan)
        Assign(option);
      break;

    case EqualMerge :
      return CompareValue(option) == EqualTo;

    case NotEqualMerge :
      return CompareValue(option) != EqualTo;
      
    case AlwaysMerge :
      Assign(option);
      break;

    default :
      break;
  }

  return true;
}


PString OpalMediaOption::AsString() const
{
  PStringStream strm;
  PrintOn(strm);
  return strm;
}


bool OpalMediaOption::FromString(const PString & value)
{
  PStringStream strm;
  strm = value;
  ReadFrom(strm);
  return !strm.fail();
}


///////////////////////////////////////

OpalMediaOptionEnum::OpalMediaOptionEnum(const char * name,
                                         bool readOnly,
                                         const char * const * enumerations,
                                         PINDEX count,
                                         MergeType merge,
                                         PINDEX value)
  : OpalMediaOption(name, readOnly, merge),
    m_enumerations(count, enumerations),
    m_value(value)
{
  if (m_value >= count)
    m_value = count;
}


PObject * OpalMediaOptionEnum::Clone() const
{
  return new OpalMediaOptionEnum(*this);
}


void OpalMediaOptionEnum::PrintOn(ostream & strm) const
{
  if (m_value < m_enumerations.GetSize())
    strm << m_enumerations[m_value];
  else
    strm << m_value;
}


void OpalMediaOptionEnum::ReadFrom(istream & strm)
{
  PCaselessString str;
  while (strm.good()) {
    char ch;
    strm.get(ch);
    str += ch;
    for (PINDEX i = 0; i < m_enumerations.GetSize(); i++) {
      if (str == m_enumerations[i]) {
        m_value = i;
        return;
      }
    }
  }

  m_value = m_enumerations.GetSize();

#ifdef __USE_STL__
   strm.setstate(ios::badbit);
#else
   strm.setf(ios::badbit , ios::badbit);
#endif
}


PObject::Comparison OpalMediaOptionEnum::CompareValue(const OpalMediaOption & option) const
{
  const OpalMediaOptionEnum * otherOption = PDownCast(const OpalMediaOptionEnum, &option);
  if (otherOption == NULL)
    return GreaterThan;

  if (m_value > otherOption->m_value)
    return GreaterThan;

  if (m_value < otherOption->m_value)
    return LessThan;

  return EqualTo;
}


void OpalMediaOptionEnum::Assign(const OpalMediaOption & option)
{
  const OpalMediaOptionEnum * otherOption = PDownCast(const OpalMediaOptionEnum, &option);
  if (otherOption != NULL)
    m_value = otherOption->m_value;
}


void OpalMediaOptionEnum::SetValue(PINDEX value)
{
  if (value < m_enumerations.GetSize())
    m_value = value;
  else
    m_value = m_enumerations.GetSize();
}


///////////////////////////////////////

OpalMediaOptionString::OpalMediaOptionString(const char * name, bool readOnly)
  : OpalMediaOption(name, readOnly, MinMerge)
{
}


OpalMediaOptionString::OpalMediaOptionString(const char * name, bool readOnly, const PString & value)
  : OpalMediaOption(name, readOnly, MinMerge),
    m_value(value)
{
}


PObject * OpalMediaOptionString::Clone() const
{
  OpalMediaOptionString * newObj = new OpalMediaOptionString(*this);
  newObj->m_value.MakeUnique();
  return newObj;
}


void OpalMediaOptionString::PrintOn(ostream & strm) const
{
  strm << m_value.ToLiteral();
}


void OpalMediaOptionString::ReadFrom(istream & strm)
{
  char c;
  strm >> c; // Skip whitespace

  if (c != '"') {
    strm.putback(c);
    strm >> m_value; // If no " then read to end of line.
  }
  else {
    // If there was a '"' then assume it is a C style literal string with \ escapes etc

    PINDEX count = 0;
    PStringStream str;
    str << '"';

    while (strm.get(c).good()) {
      str << c;

      // Keep reading till get a '"' that is not preceded by a '\' that is not itself preceded by a '\'
      if (c == '"' && count > 0 && (str[count] != '\\' || !(count > 1 && str[count-1] == '\\')))
        break;

      count++;
    }

    m_value = PString(PString::Literal, (const char *)str);
  }
}


PObject::Comparison OpalMediaOptionString::CompareValue(const OpalMediaOption & option) const
{
  const OpalMediaOptionString * otherOption = PDownCast(const OpalMediaOptionString, &option);
  if (otherOption == NULL)
    return GreaterThan;

  return m_value.Compare(otherOption->m_value);
}


void OpalMediaOptionString::Assign(const OpalMediaOption & option)
{
  const OpalMediaOptionString * otherOption = PDownCast(const OpalMediaOptionString, &option);
  if (otherOption != NULL) {
    m_value = otherOption->m_value;
    m_value.MakeUnique();
  }
}


void OpalMediaOptionString::SetValue(const PString & value)
{
  m_value = value;
  m_value.MakeUnique();
}


///////////////////////////////////////

OpalMediaOptionOctets::OpalMediaOptionOctets(const char * name, bool readOnly, bool base64)
  : OpalMediaOption(name, readOnly, NoMerge)
  , m_base64(base64)
{
}


OpalMediaOptionOctets::OpalMediaOptionOctets(const char * name, bool readOnly, bool base64, const PBYTEArray & value)
  : OpalMediaOption(name, readOnly, NoMerge)
  , m_value(value)
  , m_base64(base64)
{
}


OpalMediaOptionOctets::OpalMediaOptionOctets(const char * name, bool readOnly, bool base64, const BYTE * data, PINDEX length)
  : OpalMediaOption(name, readOnly, NoMerge)
  , m_value(data, length)
  , m_base64(base64)
{
}


PObject * OpalMediaOptionOctets::Clone() const
{
  OpalMediaOptionOctets * newObj = new OpalMediaOptionOctets(*this);
  newObj->m_value.MakeUnique();
  return newObj;
}


void OpalMediaOptionOctets::PrintOn(ostream & strm) const
{
  if (m_base64)
    strm << PBase64::Encode(m_value);
  else {
    _Ios_Fmtflags flags = strm.flags();
    char fill = strm.fill();

    strm << hex << setfill('0');
    for (PINDEX i = 0; i < m_value.GetSize(); i++)
      strm << setw(2) << (unsigned)m_value[i];

    strm.fill(fill);
    strm.flags(flags);
  }
}


void OpalMediaOptionOctets::ReadFrom(istream & strm)
{
  if (m_base64) {
    PString str;
    strm >> str;
    PBase64::Decode(str, m_value);
  }
  else {
    char pair[3];
    pair[2] = '\0';

    PINDEX count = 0;

    while (isxdigit(strm.peek())) {
      pair[0] = (char)strm.get();
      if (!isxdigit(strm.peek())) {
        strm.putback(pair[0]);
        break;
      }
      pair[1] = (char)strm.get();
      if (!m_value.SetMinSize((count+1+99)%100))
        break;
      m_value[count++] = (BYTE)strtoul(pair, NULL, 16);
    }

    m_value.SetSize(count);
  }
}


PObject::Comparison OpalMediaOptionOctets::CompareValue(const OpalMediaOption & option) const
{
  const OpalMediaOptionOctets * otherOption = PDownCast(const OpalMediaOptionOctets, &option);
  if (otherOption == NULL)
    return GreaterThan;

  return m_value.Compare(otherOption->m_value);
}


void OpalMediaOptionOctets::Assign(const OpalMediaOption & option)
{
  const OpalMediaOptionOctets * otherOption = PDownCast(const OpalMediaOptionOctets, &option);
  if (otherOption != NULL) {
    m_value = otherOption->m_value;
    m_value.MakeUnique();
  }
}


void OpalMediaOptionOctets::SetValue(const PBYTEArray & value)
{
  m_value = value;
  m_value.MakeUnique();
}


void OpalMediaOptionOctets::SetValue(const BYTE * data, PINDEX length)
{
  m_value = PBYTEArray(data, length);
}


/////////////////////////////////////////////////////////////////////////////

const PString & OpalMediaFormat::NeedsJitterOption() { static PString s = "Needs Jitter";   return s; }
const PString & OpalMediaFormat::MaxBitRateOption()  { static PString s = "Max Bit Rate";   return s; }
const PString & OpalMediaFormat::MaxFrameSizeOption(){ static PString s = "Max Frame Size"; return s; }
const PString & OpalMediaFormat::FrameTimeOption()   { static PString s = "Frame Time";     return s; }
const PString & OpalMediaFormat::ClockRateOption()   { static PString s = "Clock Rate";     return s; }

OpalMediaFormat::OpalMediaFormat(OpalMediaFormatInternal * info)
  : m_info(NULL)
{
  Construct(info);
}


OpalMediaFormat::OpalMediaFormat(RTP_DataFrame::PayloadTypes pt, unsigned clockRate, const char * name, const char * protocol)
  : m_info(NULL)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  PINDEX idx = registeredFormats.FindFormat(pt, clockRate, name, protocol);
  if (idx != P_MAX_INDEX)
    *this = registeredFormats[idx];
}


OpalMediaFormat::OpalMediaFormat(const char * wildcard)
  : m_info(NULL)
{
  operator=(PString(wildcard));
}


OpalMediaFormat::OpalMediaFormat(const PString & wildcard)
  : m_info(NULL)
{
  operator=(wildcard);
}


OpalMediaFormat::OpalMediaFormat(const char * fullName,
                                 unsigned dsid,
                                 RTP_DataFrame::PayloadTypes pt,
                                 const char * en,
                                 BOOL     nj,
                                 unsigned bw,
                                 PINDEX   fs,
                                 unsigned ft,
                                 unsigned cr,
                                 time_t ts)
{
  Construct(new OpalMediaFormatInternal(fullName, dsid, pt, en, nj, bw, fs, ft,cr, ts));
}


void OpalMediaFormat::Construct(OpalMediaFormatInternal * info)
{
  if (info == NULL)
    return;

  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  PINDEX idx = registeredFormats.FindFormat(info->formatName);
  if (idx != P_MAX_INDEX)
    *this = registeredFormats[idx];
  else {
    m_info = info;
    registeredFormats.OpalMediaFormatBaseList::Append(this);
  }
}


OpalMediaFormat & OpalMediaFormat::operator=(RTP_DataFrame::PayloadTypes pt)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  PINDEX idx = registeredFormats.FindFormat(pt);
  if (idx == P_MAX_INDEX)
    *this = OpalMediaFormat();
  else if (this != &registeredFormats[idx])
    *this = registeredFormats[idx];

  return *this;
}


OpalMediaFormat & OpalMediaFormat::operator=(const char * wildcard)
{
  return operator=(PString(wildcard));
}


OpalMediaFormat & OpalMediaFormat::operator=(const PString & wildcard)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  PINDEX idx = registeredFormats.FindFormat(wildcard);
  if (idx == P_MAX_INDEX)
    *this = OpalMediaFormat();
  else
    *this = registeredFormats[idx];

  return *this;
}


void OpalMediaFormat::CloneContents(const OpalMediaFormat * c)
{
  m_info = (OpalMediaFormatInternal *)c->m_info->Clone();
  m_info->options.MakeUnique();
}


void OpalMediaFormat::CopyContents(const OpalMediaFormat & c)
{
  m_info = c.m_info;
}


void OpalMediaFormat::DestroyContents()
{
  delete m_info;
}


PObject * OpalMediaFormat::Clone() const
{
  return new OpalMediaFormat(*this);
}


PObject::Comparison OpalMediaFormat::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, OpalMediaFormat), PInvalidCast);
  const OpalMediaFormat & other = (const OpalMediaFormat &)obj;
  if (m_info == NULL)
    return other.m_info == NULL ? EqualTo : LessThan;
  if (other.m_info == NULL)
    return m_info == NULL ? EqualTo : GreaterThan;
  return m_info->formatName.Compare(other.m_info->formatName);
}


void OpalMediaFormat::PrintOn(ostream & strm) const
{
  if (m_info != NULL)
    strm << m_info->formatName;
}


void OpalMediaFormat::ReadFrom(istream & strm)
{
  char fmt[100];
  strm >> fmt;
  operator=(fmt);
}


OpalMediaFormatList OpalMediaFormat::GetAllRegisteredMediaFormats()
{
  OpalMediaFormatList copy;
  GetAllRegisteredMediaFormats(copy);
  return copy;
}


void OpalMediaFormat::GetAllRegisteredMediaFormats(OpalMediaFormatList & copy)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  for (PINDEX i = 0; i < registeredFormats.GetSize(); i++)
    copy += registeredFormats[i];
}


bool OpalMediaFormat::SetRegisteredMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  for (PINDEX i = 0; i < registeredFormats.GetSize(); i++) {
    if (registeredFormats[i] == mediaFormat) {
      /* Yes, this looks a little odd as we just did equality above and seem to
         be assigning the left hand side with exactly the same value. But what
         is really happening is the above only compares the name, and below
         copies all of the attributes (OpalMediaFormatOtions) across. */
      registeredFormats[i] = mediaFormat;
      return true;
    }
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////

OpalMediaFormatInternal::OpalMediaFormatInternal(const char * fullName,
                                                 unsigned dsid,
                                                 RTP_DataFrame::PayloadTypes pt,
                                                 const char * en,
                                                 BOOL     nj,
                                                 unsigned bw,
                                                 PINDEX   fs,
                                                 unsigned ft,
                                                 unsigned cr,
                                                 time_t ts)
  : formatName(fullName)
{
  codecVersionTime = ts;
  rtpPayloadType = pt;
  rtpEncodingName = en;
  defaultSessionID = dsid;

  if (nj)
    AddOption(new OpalMediaOptionBoolean(OpalMediaFormat::NeedsJitterOption(), true, OpalMediaOption::OrMerge, true));

  AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::MaxBitRateOption(), true, OpalMediaOption::MinMerge, bw, 100));

  if (fs > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::MaxFrameSizeOption(), true, OpalMediaOption::NoMerge, fs));

  if (ft > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::FrameTimeOption(), true, OpalMediaOption::NoMerge, ft));

  if (cr > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::ClockRateOption(), true, OpalMediaOption::AlwaysMerge, cr));

  // assume non-dynamic payload types are correct and do not need deconflicting
  if (rtpPayloadType < RTP_DataFrame::DynamicBase || rtpPayloadType >= RTP_DataFrame::MaxPayloadType)
    return;

  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  // find the next unused dynamic number, and find anything with the new 
  // rtp payload type if it is explicitly required
  PINDEX i;
  OpalMediaFormat * match = NULL;
  RTP_DataFrame::PayloadTypes nextUnused = RTP_DataFrame::DynamicBase;
  do {
    for (i = 0; i < registeredFormats.GetSize(); i++) {
      if (registeredFormats[i].GetPayloadType() == nextUnused) {
        nextUnused = (RTP_DataFrame::PayloadTypes)(nextUnused + 1);
        break;
      }
      if ((rtpPayloadType >= RTP_DataFrame::DynamicBase) && 
          (registeredFormats[i].GetPayloadType() == rtpPayloadType))
        match = &registeredFormats[i];
    }
  } while (i < registeredFormats.GetSize());

  // if new format requires a specific payload type in the dynamic range, 
  // then move the old format to the next unused format
  if (match != NULL)
    match->SetPayloadType(nextUnused);
}


bool OpalMediaFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PWaitAndSignal m1(media_format_mutex);
  PWaitAndSignal m2(mediaFormat.media_format_mutex);
  for (PINDEX i = 0; i < options.GetSize(); i++) {
    OpalMediaOption * option = mediaFormat.FindOption(options[i].GetName());
    if (option != NULL && !options[i].Merge(*option))
      return false;
  }

  return true;
}


bool OpalMediaFormatInternal::GetOptionValue(const PString & name, PString & value) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  value = option->AsString();
  return true;
}


bool OpalMediaFormatInternal::SetOptionValue(const PString & name, const PString & value)
{
  PWaitAndSignal m(media_format_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  return option->FromString(value);
}


bool OpalMediaFormatInternal::GetOptionBoolean(const PString & name, bool dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return dflt;

  return PDownCast(OpalMediaOptionBoolean, option)->GetValue();
}


bool OpalMediaFormatInternal::SetOptionBoolean(const PString & name, bool value)
{
  PWaitAndSignal m(media_format_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  PDownCast(OpalMediaOptionBoolean, option)->SetValue(value);
  return true;
}


int OpalMediaFormatInternal::GetOptionInteger(const PString & name, int dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return dflt;

  OpalMediaOptionUnsigned * optUnsigned = dynamic_cast<OpalMediaOptionUnsigned *>(option);
  if (optUnsigned != NULL)
    return optUnsigned->GetValue();

  OpalMediaOptionInteger * optInteger = dynamic_cast<OpalMediaOptionInteger *>(option);
  if (optInteger != NULL)
    return optInteger->GetValue();

  PAssertAlways(PInvalidCast);
  return dflt;
}


bool OpalMediaFormatInternal::SetOptionInteger(const PString & name, int value)
{
  PWaitAndSignal m(media_format_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  OpalMediaOptionUnsigned * optUnsigned = dynamic_cast<OpalMediaOptionUnsigned *>(option);
  if (optUnsigned != NULL) {
    optUnsigned->SetValue(value);
    return true;
  }

  OpalMediaOptionInteger * optInteger = dynamic_cast<OpalMediaOptionInteger *>(option);
  if (optInteger != NULL) {
    optInteger->SetValue(value);
    return true;
  }

  PAssertAlways(PInvalidCast);
  return false;
}


double OpalMediaFormatInternal::GetOptionReal(const PString & name, double dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return dflt;

  return PDownCast(OpalMediaOptionReal, option)->GetValue();
}


bool OpalMediaFormatInternal::SetOptionReal(const PString & name, double value)
{
  PWaitAndSignal m(media_format_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  PDownCast(OpalMediaOptionReal, option)->SetValue(value);
  return true;
}


PINDEX OpalMediaFormatInternal::GetOptionEnum(const PString & name, PINDEX dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return dflt;

  return PDownCast(OpalMediaOptionEnum, option)->GetValue();
}


bool OpalMediaFormatInternal::SetOptionEnum(const PString & name, PINDEX value)
{
  PWaitAndSignal m(media_format_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  PDownCast(OpalMediaOptionEnum, option)->SetValue(value);
  return true;
}


PString OpalMediaFormatInternal::GetOptionString(const PString & name, const PString & dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return dflt;

  return PDownCast(OpalMediaOptionString, option)->GetValue();
}


bool OpalMediaFormatInternal::SetOptionString(const PString & name, const PString & value)
{
  PWaitAndSignal m(media_format_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  PDownCast(OpalMediaOptionString, option)->SetValue(value);
  return true;
}


bool OpalMediaFormatInternal::GetOptionOctets(const PString & name, PBYTEArray & octets) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  octets = PDownCast(OpalMediaOptionOctets, option)->GetValue();
  return true;
}


bool OpalMediaFormatInternal::SetOptionOctets(const PString & name, const PBYTEArray & octets)
{
  PWaitAndSignal m(media_format_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  PDownCast(OpalMediaOptionOctets, option)->SetValue(octets);
  return true;
}


bool OpalMediaFormatInternal::SetOptionOctets(const PString & name, const BYTE * data, PINDEX length)
{
  PWaitAndSignal m(media_format_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  PDownCast(OpalMediaOptionOctets, option)->SetValue(data, length);
  return true;
}


bool OpalMediaFormatInternal::AddOption(OpalMediaOption * option, BOOL overwrite)
{
  PWaitAndSignal m(media_format_mutex);
  if (PAssertNULL(option) == NULL)
    return false;

  PINDEX index = options.GetValuesIndex(*option);
  if (index != P_MAX_INDEX) {
    if (!overwrite) {
      delete option;
      return false;
    }

    options.RemoveAt(index);
  }

  options.Append(option);
  return true;
}


class OpalMediaOptionSearchArg : public OpalMediaOption
{
public:
  OpalMediaOptionSearchArg(const PString & name) : OpalMediaOption(name) { }
  virtual Comparison CompareValue(const OpalMediaOption &) const { return EqualTo; }
  virtual void Assign(const OpalMediaOption &) { }
};

OpalMediaOption * OpalMediaFormatInternal::FindOption(const PString & name) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOptionSearchArg search(name);
  PINDEX index = options.GetValuesIndex(search);
  if (index == P_MAX_INDEX)
    return NULL;

  return &options[index];
}


bool OpalMediaFormatInternal::IsValidForProtocol(const PString & protocol) const
{
  // the protocol is only valid for SIP if the RTP name is not NULL
  if (protocol *= "sip")
    return rtpEncodingName != NULL;

  return TRUE;
}


void OpalMediaFormatInternal::PrintOptions(ostream & strm) const
{
  PWaitAndSignal m(media_format_mutex);

  static const char * const SessionNames[] = { "", " Audio", " Video", " Data", " H.224" };
  static const int TitleWidth = 25;

  strm << right << setw(TitleWidth) <<   "Format Name" << left << "       = " << formatName << '\n'
       << right << setw(TitleWidth) <<    "Session ID" << left << "       = " << defaultSessionID << (defaultSessionID < PARRAYSIZE(SessionNames) ? SessionNames[defaultSessionID] : "") << '\n'
       << right << setw(TitleWidth) <<  "Payload Type" << left << "       = " << rtpPayloadType << '\n'
       << right << setw(TitleWidth) << "Encoding Name" << left << "       = " << rtpEncodingName << '\n';
  for (PINDEX i = 0; i < options.GetSize(); i++) {
    const OpalMediaOption & option = options[i];
    strm << right << setw(TitleWidth) << option.GetName() << " (R/" << (option.IsReadOnly() ? 'O' : 'W')
         << ") = " << left << setw(10) << option.AsString();
    if (!option.GetFMTPName().IsEmpty())
      strm << "  FMTP name: " << option.GetFMTPName() << " (" << option.GetFMTPDefault() << ')';
    const OpalMediaOption::H245GenericInfo & genericInfo = option.GetH245Generic();
    if (genericInfo.mode != OpalMediaOption::H245GenericInfo::None) {
      strm << "  H.245 Ordinal: " << genericInfo.ordinal
           << ' ' << (genericInfo.mode == OpalMediaOption::H245GenericInfo::Collapsing ? "Collapsing" : "Non-Collapsing");
      if (!genericInfo.excludeTCS)
        strm << " TCS";
      if (!genericInfo.excludeOLC)
        strm << " OLC";
      if (!genericInfo.excludeReqMode)
        strm << " RM";
    }
    strm << '\n';
  }
  strm << endl;
}

///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

const PString & OpalAudioFormat::RxFramesPerPacketOption() { static PString s = "Rx Frames Per Packet"; return s; }
const PString & OpalAudioFormat::TxFramesPerPacketOption() { static PString s = "Tx Frames Per Packet"; return s; }

OpalAudioFormat::OpalAudioFormat(const char * fullName,
                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                 const char * encodingName,
                                 PINDEX   frameSize,
                                 unsigned frameTime,
                                 unsigned rxFrames,
                                 unsigned txFrames,
                                 unsigned maxFrames,
                                 unsigned clockRate,
                                 time_t timeStamp)
{
  Construct(new OpalAudioFormatInternal(fullName,
                                        rtpPayloadType,
                                        encodingName,
                                        frameSize,
                                        frameTime,
                                        rxFrames,
                                        txFrames,
                                        maxFrames,
                                        clockRate,
                                        timeStamp));
}


OpalAudioFormatInternal::OpalAudioFormatInternal(const char * fullName,
                                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                                 const char * encodingName,
                                                 PINDEX   frameSize,
                                                 unsigned frameTime,
                                                 unsigned rxFrames,
                                                 unsigned txFrames,
                                                 unsigned maxFrames,
                                                 unsigned clockRate,
                                                 time_t timeStamp)
  : OpalMediaFormatInternal(fullName,
                            OpalMediaFormat::DefaultAudioSessionID,
                            rtpPayloadType,
                            encodingName,
                            TRUE,
                            8*frameSize*OpalAudioFormat::AudioClockRate/frameTime,  // bits per second = 8*frameSize * framesPerSecond
                            frameSize,
                            frameTime,
                            clockRate,
                            timeStamp)
{
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::RxFramesPerPacketOption(), false, OpalMediaOption::MinMerge, rxFrames, 1, maxFrames));
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::TxFramesPerPacketOption(), false, OpalMediaOption::NoMerge, txFrames, 1, maxFrames));
}


PObject * OpalAudioFormatInternal::Clone() const
{
  return new OpalAudioFormatInternal(*this);
}


bool OpalAudioFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PWaitAndSignal m(media_format_mutex);

  if (!OpalMediaFormatInternal::Merge(mediaFormat))
    return false;

  return true;
}

#endif

///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

const PString & OpalVideoFormat::FrameWidthOption()          { static PString s = "Frame Width";           return s; }
const PString & OpalVideoFormat::FrameHeightOption()         { static PString s = "Frame Height";          return s; }
const PString & OpalVideoFormat::EncodingQualityOption()     { static PString s = "Encoding Quality";      return s; }
const PString & OpalVideoFormat::TargetBitRateOption()       { static PString s = "Target Bit Rate";       return s; }
const PString & OpalVideoFormat::DynamicVideoQualityOption() { static PString s = "Dynamic Video Quality"; return s; }
const PString & OpalVideoFormat::AdaptivePacketDelayOption() { static PString s = "Adaptive Packet Delay"; return s; }

OpalVideoFormat::OpalVideoFormat(const char * fullName,
                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                 const char * encodingName,
                                 unsigned frameWidth,
                                 unsigned frameHeight,
                                 unsigned frameRate,
                                 unsigned bitRate,
                                 time_t timeStamp)
{
  Construct(new OpalVideoFormatInternal(fullName,
                                        rtpPayloadType,
                                        encodingName,
                                        frameWidth,
                                        frameHeight,
                                        frameRate,
                                        bitRate,
                                        timeStamp));
}


OpalVideoFormatInternal::OpalVideoFormatInternal(const char * fullName,
                                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                                 const char * encodingName,
                                                 unsigned frameWidth,
                                                 unsigned frameHeight,
                                                 unsigned frameRate,
                                                 unsigned bitRate,
                                                 time_t timeStamp)
  : OpalMediaFormatInternal(fullName,
                            OpalMediaFormat::DefaultVideoSessionID,
                            rtpPayloadType,
                            encodingName,
                            FALSE,
                            bitRate,
                            0,
                            OpalMediaFormat::VideoClockRate/frameRate,
                            OpalMediaFormat::VideoClockRate,
                            timeStamp)
{
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::FrameWidthOption(),         false, OpalMediaOption::MinMerge, frameWidth, 11, 32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::FrameHeightOption(),        false, OpalMediaOption::MinMerge, frameHeight, 9, 32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::EncodingQualityOption(),    false, OpalMediaOption::MinMerge, 15,          1, 31));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::TargetBitRateOption(),      false, OpalMediaOption::MinMerge, 10000000,    1000));
  AddOption(new OpalMediaOptionBoolean(OpalVideoFormat::DynamicVideoQualityOption(), false, OpalMediaOption::NoMerge,  false));
  AddOption(new OpalMediaOptionBoolean(OpalVideoFormat::AdaptivePacketDelayOption(), false, OpalMediaOption::NoMerge,  false));

  // For video the max bit rate and frame rate is adjustable by user
  FindOption(OpalVideoFormat::MaxBitRateOption())->SetReadOnly(false);
  FindOption(OpalVideoFormat::FrameTimeOption())->SetReadOnly(false);
  FindOption(OpalVideoFormat::FrameTimeOption())->SetMerge(OpalMediaOption::MinMerge);
}


PObject * OpalVideoFormatInternal::Clone() const
{
  return new OpalVideoFormatInternal(*this);
}


bool OpalVideoFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PWaitAndSignal m(media_format_mutex);

  if (!OpalMediaFormatInternal::Merge(mediaFormat))
    return false;

  unsigned maxBitRate = GetOptionInteger(OpalVideoFormat::MaxBitRateOption(), 0);
  unsigned targetBitRate = GetOptionInteger(OpalVideoFormat::TargetBitRateOption(), 0);
  if (targetBitRate > maxBitRate)
    SetOptionInteger(OpalVideoFormat::TargetBitRateOption(), maxBitRate);

  return true;
}

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////

OpalMediaFormatList::OpalMediaFormatList()
{
}


OpalMediaFormatList::OpalMediaFormatList(const OpalMediaFormat & format)
{
  *this += format;
}


OpalMediaFormatList & OpalMediaFormatList::operator+=(const OpalMediaFormat & format)
{
  if (format.IsValid() && !HasFormat(format))
    OpalMediaFormatBaseList::Append(format.Clone());
  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator+=(const OpalMediaFormatList & formats)
{
  for (PINDEX i = 0; i < formats.GetSize(); i++)
    *this += formats[i];
  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator-=(const OpalMediaFormat & format)
{
  PINDEX idx = FindFormat(format);
  if (idx != P_MAX_INDEX)
    RemoveAt(idx);

  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator-=(const OpalMediaFormatList & formats)
{
  for (PINDEX i = 0; i < formats.GetSize(); i++)
    *this -= formats[i];
  return *this;
}


void OpalMediaFormatList::Remove(const PStringArray & mask)
{
  PINDEX i;
  for (i = 0; i < mask.GetSize(); i++) {
    PINDEX idx;
    while ((idx = FindFormat(mask[i])) != P_MAX_INDEX)
      RemoveAt(idx);
  }
}

PINDEX OpalMediaFormatList::FindFormat(RTP_DataFrame::PayloadTypes pt, unsigned clockRate, const char * name, const char * protocol) const
{
  for (PINDEX idx = 0; idx < GetSize(); idx++) {
    OpalMediaFormat & mediaFormat = (*this)[idx];

    // clock rates must always match
    if (clockRate != 0 && clockRate != mediaFormat.GetClockRate())
      continue;

    // if protocol is specified, must be valid for the protocol
    if ((protocol != NULL) && !mediaFormat.IsValidForProtocol(protocol))
      continue;

    // if an encoding name is specified, and it matches exactly, then use it
    // regardless of payload code. This allows the payload code mapping in SIP to work
    // if it doesn't match, then don't bother comparing payload codes
    if (name != NULL && *name != '\0') {
      const char * otherName = mediaFormat.GetEncodingName();
      if (otherName != NULL && strcasecmp(otherName, name) == 0)
        return idx;
      continue;
    }

    // if the payload type is not dynamic, and matches, then this is a match
    if (pt < RTP_DataFrame::DynamicBase && mediaFormat.GetPayloadType() == pt)
      return idx;

    //if (RTP_DataFrame::IllegalPayloadType == pt)
    //  return idx;
  }

  return P_MAX_INDEX;
}


static BOOL WildcardMatch(const PCaselessString & str, const PStringArray & wildcards)
{
  if (wildcards.GetSize() == 1)
    return str == wildcards[0];

  PINDEX i;
  PINDEX last = 0;
  for (i = 0; i < wildcards.GetSize(); i++) {
    PString wildcard = wildcards[i];

    PINDEX next;
    if (wildcard.IsEmpty())
      next = last;
    else {
      next = str.Find(wildcard, last);
      if (next == P_MAX_INDEX)
        return FALSE;
    }

    // Check for having * at beginning of search string
    if (i == 0 && next != 0 && !wildcard)
      return FALSE;

    last = next + wildcard.GetLength();

    // Check for having * at end of search string
    if (i == wildcards.GetSize()-1 && !wildcard && last != str.GetLength())
      return FALSE;
  }

  return TRUE;
}


PINDEX OpalMediaFormatList::FindFormat(const PString & search, PINDEX pos) const
{
  PStringArray wildcards = search.Tokenise('*', TRUE);
  PINDEX idx;
  for (idx = pos; idx < GetSize(); idx++) {
    if (WildcardMatch((*this)[idx].m_info->formatName, wildcards))
      return idx;
  }

  return P_MAX_INDEX;
}


void OpalMediaFormatList::Reorder(const PStringArray & order)
{
  DisallowDeleteObjects();
  PINDEX nextPos = 0;
  for (PINDEX i = 0; i < order.GetSize(); i++) {
    PStringArray wildcards = order[i].Tokenise('*', TRUE);

    PINDEX findPos = 0;
    while (findPos < GetSize()) {
      if (WildcardMatch((*this)[findPos].m_info->formatName, wildcards)) {
        if (findPos > nextPos)
          OpalMediaFormatBaseList::InsertAt(nextPos, RemoveAt(findPos));
        nextPos++;
      }
      findPos++;
    }
  }
  AllowDeleteObjects();
}


// End of File ///////////////////////////////////////////////////////////////
