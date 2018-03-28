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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediafmt.h"
#pragma implementation "mediacmd.h"
#endif

#include <opal_config.h>

#include <opal/mediafmt.h>
#include <opal/mediacmd.h>
#include <codec/vidcodec.h>
#include <codec/opalplugin.hpp>
#include <codec/opalwavfile.h>
#include <ptlib/videoio.h>
#include <ptclib/cypher.h>


#define new PNEW
#define PTraceModule() "MediaFormat"


/////////////////////////////////////////////////////////////////////////////

#define AUDIO_FORMAT(name, rtpPayloadType, encodingName, frameSize, frameTime, rxFrames, txFrames, maxFrames, clock, chan) \
  const OpalAudioFormat & GetOpal##name() \
  { \
    static const OpalAudioFormat name(OPAL_##name, RTP_DataFrame::rtpPayloadType, \
                                      encodingName, frameSize, frameTime, rxFrames, txFrames, maxFrames, clock, 0, chan); \
    return name; \
  }
//           name            rtpPayloadType  encodingName frameSize frameTime rxFrames txFrames maxFrames clock
AUDIO_FORMAT(PCM16,          MaxPayloadType, "",          16,        8,       240,      0,      256,       8000, 1);
AUDIO_FORMAT(PCM16_12KHZ,    MaxPayloadType, "",          24,       12,       240,      0,      256,      12000, 1);
AUDIO_FORMAT(PCM16_16KHZ,    MaxPayloadType, "",          32,       16,       240,      0,      256,      16000, 1);
AUDIO_FORMAT(PCM16_24KHZ,    MaxPayloadType, "",          48,       24,       240,      0,      256,      24000, 1);
AUDIO_FORMAT(PCM16_32KHZ,    MaxPayloadType, "",          64,       32,       240,      0,      256,      32000, 1);
AUDIO_FORMAT(PCM16_48KHZ,    MaxPayloadType, "",          96,       48,       240,      0,      256,      48000, 1);

AUDIO_FORMAT(PCM16S,         MaxPayloadType, "",          32,        8,       240,      0,      256,       8000, 2);
AUDIO_FORMAT(PCM16S_12KHZ,   MaxPayloadType, "",          48,       12,       240,      0,      256,      12000, 2);
AUDIO_FORMAT(PCM16S_16KHZ,   MaxPayloadType, "",          64,       16,       240,      0,      256,      16000, 2);
AUDIO_FORMAT(PCM16S_24KHZ,   MaxPayloadType, "",          96,       24,       240,      0,      256,      24000, 2);
AUDIO_FORMAT(PCM16S_32KHZ,   MaxPayloadType, "",         128,       32,       240,      0,      256,      32000, 2);
AUDIO_FORMAT(PCM16S_48KHZ,   MaxPayloadType, "",         192,       48,       240,      0,      256,      48000, 2);

AUDIO_FORMAT(L16_MONO_8KHZ,  L16_Mono,       "L16",       16,        8,       240,     30,      256,       8000, 1);
AUDIO_FORMAT(L16_MONO_16KHZ, L16_Mono,       "L16",       32,       16,       240,     30,      256,      16000, 1);
AUDIO_FORMAT(L16_MONO_32KHZ, L16_Mono,       "L16",       64,       32,       240,     30,      256,      32000, 1);
AUDIO_FORMAT(L16_MONO_48KHZ, L16_Mono,       "L16",       96,       48,       240,     30,      256,      48000, 1);

AUDIO_FORMAT(L16_STEREO_8KHZ,  L16_Stereo,   "L16",       32,        8,       240,     30,      256,       8000, 2);
AUDIO_FORMAT(L16_STEREO_16KHZ, L16_Stereo,   "L16",       64,       16,       240,     30,      256,      16000, 2);
AUDIO_FORMAT(L16_STEREO_32KHZ, L16_Stereo,   "L16",      128,       32,       240,     30,      256,      32000, 2);
AUDIO_FORMAT(L16_STEREO_48KHZ, L16_Stereo,   "L16",      192,       48,       240,     30,      256,      48000, 2);

AUDIO_FORMAT(G711_ULAW_64K,  PCMU,           "PCMU",       8,        8,       240,     20,      256,       8000, 1);
AUDIO_FORMAT(G711_ALAW_64K,  PCMA,           "PCMA",       8,        8,       240,     20,      256,       8000, 1);


const OpalAudioFormat & GetOpalPCM16(unsigned clockRate, unsigned channels)
{
  switch (clockRate) {
    default :
      return channels == 2 ? GetOpalPCM16S()       : GetOpalPCM16();
    case 12000 :
      return channels == 2 ? GetOpalPCM16S_12KHZ() : GetOpalPCM16_12KHZ();
    case 16000 :
      return channels == 2 ? GetOpalPCM16S_16KHZ() : GetOpalPCM16_16KHZ();
    case 24000 :
      return channels == 2 ? GetOpalPCM16S_24KHZ() : GetOpalPCM16_24KHZ();
    case 32000 :
      return channels == 2 ? GetOpalPCM16S_32KHZ() : GetOpalPCM16_32KHZ();
    case 48000 :
      return channels == 2 ? GetOpalPCM16S_48KHZ() : GetOpalPCM16_48KHZ();
  }
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
  static PMutex mutex(PDebugLocation(__FILE__, __LINE__, "OpalMediaFormatsList"));
  return mutex;
}


static void Clamp(OpalMediaFormatInternal & fmt1, const OpalMediaFormatInternal & fmt2, const PString & variableOption, const PString & minOption, const PString & maxOption)
{
  if (fmt1.FindOption(variableOption) == NULL)
    return;

  unsigned value    = fmt1.GetOptionInteger(variableOption, 0);
  unsigned minValue = fmt2.GetOptionInteger(minOption, 0);
  unsigned maxValue = fmt2.GetOptionInteger(maxOption, UINT_MAX);
  if (value < minValue) {
    PTRACE(4, "Clamped media option \"" << variableOption << "\" from " << value << " to min " << minValue);
    fmt1.SetOptionInteger(variableOption, minValue);
  }
  else if (value > maxValue) {
    PTRACE(4, "Clamped media option \"" << variableOption << "\" from " << value << " to max " << maxValue);
    fmt1.SetOptionInteger(variableOption, maxValue);
  }
}


static bool WildcardMatch(const PCaselessString & str, const PStringArray & wildcards)
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
        return false;
    }

    // Check for having * at beginning of search string
    if (i == 0 && next != 0 && !wildcard.IsEmpty())
      return false;

    last = next + wildcard.GetLength();

    // Check for having * at end of search string
    if (i == wildcards.GetSize()-1 && !wildcard.IsEmpty() && last != str.GetLength())
      return false;
  }

  return true;
}


/////////////////////////////////////////////////////////////////////////////

OpalBandwidth::OpalBandwidth(const PString & str)
  : m_bps(0)
{
  PStringStream strm(str);
  strm >> *this;
}


std::ostream & operator<<(std::ostream & strm, OpalBandwidth::Direction dir)
{
  switch (dir) {
    case OpalBandwidth::Rx :
      return strm << "rx";
    case OpalBandwidth::Tx :
      return strm << "tx";
    default :
      return strm << "rx/tx";
  }
}


PObject::Comparison OpalBandwidth::Compare(const PObject & other) const
{
  return Compare2(m_bps, dynamic_cast<const OpalBandwidth &>(other).m_bps);
}


void OpalBandwidth::PrintOn(std::ostream & strm) const
{
  strm << PString(PString::ScaleSI, m_bps) << "b/s";
}


void OpalBandwidth::ReadFrom(std::istream & strm)
{
  strm >> m_bps;
  if (!strm.good())
      return;

  switch (tolower(strm.peek())) {
    case 'k' :
      m_bps *= 1000;
      strm.ignore(1);
      break;
    case 'm' :
      m_bps *= 1000000;
      strm.ignore(1);
      break;
    case 'g' :
      m_bps *= 1000000000;
      strm.ignore(1);
      break;
  }
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
  m_name.Replace("=", "_", true);
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
  bool assign;
  switch (m_merge) {
    case MinMerge :
      assign = CompareValue(option) == GreaterThan;
      break;

    case MaxMerge :
      assign = CompareValue(option) == LessThan;
      break;

    case EqualMerge :
      if (CompareValue(option) == EqualTo)
        return true;
      PTRACE(2, "Merge of media option \"" << m_name << "\" failed, "
                "required to be equal: \"" << *this << "\"!=\"" << option << '"');
      return false;

    case NotEqualMerge :
      if (CompareValue(option) != EqualTo)
        return true;
      PTRACE(2, "Merge of media option \"" << m_name << "\" failed, "
                "required to be not equal: \"" << *this << "\"==\"" << option << '"');
      return false;

    case AlwaysMerge :
      assign = CompareValue(option) != EqualTo;
      break;

    default :
      assign = false;
      break;
  }

  if (assign) {
    PTRACE(4, "Changed media option \"" << m_name << "\" "
              "from \"" << *this << "\" to \"" << option << '"');
    Assign(option);
  }

  return true;
}


bool OpalMediaOption::ValidateMerge(const OpalMediaOption & option) const
{
  switch (m_merge) {
    case EqualMerge :
      if (CompareValue(option) == EqualTo)
        return true;
      break;

    case NotEqualMerge :
      if (CompareValue(option) != EqualTo)
        return true;
      break;

    default :
      return true;
  }

  PTRACE(3, "Validation of merge for media option \"" << m_name << "\" failed.");
  return false;
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

#if OPAL_H323
OpalMediaOption::H245GenericInfo::H245GenericInfo()
  : ordinal(0)
  , mode(None)
  , integerType(UnsignedInt)
  , excludeTCS(false)
  , excludeOLC(false)
  , excludeReqMode(false)
  , position(-1)
{
}


OpalMediaOption::H245GenericInfo::H245GenericInfo(unsigned mask, const char * dflt)
  : ordinal(mask&PluginCodec_H245_OrdinalMask)
  , mode((mask&PluginCodec_H245_Collapsing) != 0 ? Collapsing : ((mask&PluginCodec_H245_NonCollapsing) != 0 ? NonCollapsing : None))
  , integerType((mask&PluginCodec_H245_Unsigned32) != 0 ? Unsigned32 : ((mask&PluginCodec_H245_BooleanArray) != 0 ? BooleanArray : UnsignedInt))
  , excludeTCS((mask&PluginCodec_H245_TCS) == 0)
  , excludeOLC((mask&PluginCodec_H245_OLC) == 0)
  , excludeReqMode((mask&PluginCodec_H245_ReqMode) == 0)
  , position((mask&PluginCodec_H245_PositionMask)>>PluginCodec_H245_PositionShift)
  , defaultValue(dflt)
{
  if (position == 0)
    position = ordinal;
}
#endif


///////////////////////////////////////

OpalMediaOptionEnum::OpalMediaOptionEnum(const char * name, bool readOnly)
  : OpalMediaOption(name, readOnly, EqualMerge)
  , m_value(0)
{
}


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
  if (m_merge == IntersectionMerge) {
    char ** arr = m_enumerations.ToCharArray();
    PPrintBitwiseEnum(strm, m_value, arr);
    free(arr);
  }
  else if (m_value < m_enumerations.GetSize())
    strm << m_enumerations[m_value];
  else
    strm << psprintf("<%u>", m_value); // Don't output direct to stream or width() is on '<' only
}


void OpalMediaOptionEnum::ReadFrom(istream & strm)
{
  if (m_merge == IntersectionMerge) {
    char ** arr = m_enumerations.ToCharArray();
    m_value = PReadBitwiseEnum(strm, arr);
    free(arr);
    return;
  }

  m_value = m_enumerations.GetSize();

  PINDEX longestMatch = 0;

  PCaselessString str;
  while (strm.peek() != EOF) {
    str += (char)strm.get();

    PINDEX i;
    for (i = 0; i < m_enumerations.GetSize(); i++) {
      if (str == m_enumerations[i].Left(str.GetLength())) {
        longestMatch = i;
        break;
      }
    }
    if (i >= m_enumerations.GetSize()) {
      i = str.GetLength()-1;
      strm.putback(str[i]);
      str.Delete(i, 1);
      break;
    }
  }

  if (str == m_enumerations[longestMatch])
    m_value = longestMatch;
  else {
    for (PINDEX i = str.GetLength(); i > 0; )
      strm.putback(str[--i]);
    strm.setstate(ios::failbit);
  }
}


bool OpalMediaOptionEnum::Merge(const OpalMediaOption & option)
{
  if (m_merge != IntersectionMerge)
    return OpalMediaOption::Merge(option);

  const OpalMediaOptionEnum * otherOption = PDownCast(const OpalMediaOptionEnum, &option);
  if (otherOption == NULL)
    return false;

  PINDEX newValue = m_value & otherOption->m_value;
  if (m_value != newValue) {
    PTRACE(4, "Changed media option \"" << m_name << "\" "
              "from 0x" << hex << m_value << " to 0x" << newValue << dec);
    m_value = newValue;
  }
  return true;
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
  PINDEX maxEnum = m_merge != IntersectionMerge ? m_enumerations.GetSize() : (1LL << m_enumerations.GetSize());
  if (
#ifndef __GNUC__
      value >= 0 &&
#endif
      value < maxEnum)
    m_value = value;
  else {
    m_value = maxEnum;
    PTRACE(1, "Illegal value (" << value << ") for OpalMediaOptionEnum");
  }
}


///////////////////////////////////////

OpalMediaOptionString::OpalMediaOptionString(const char * name, bool readOnly)
  : OpalMediaOption(name, readOnly, NoMerge)
{
}


OpalMediaOptionString::OpalMediaOptionString(const char * name, bool readOnly, const PString & value)
  : OpalMediaOption(name, readOnly, NoMerge),
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
  strm << m_value;
}


void OpalMediaOptionString::ReadFrom(istream & strm)
{
  while (isspace(strm.peek())) // Skip whitespace
    strm.get();

  if (strm.peek() != '"')
    strm >> m_value; // If no '"' then read to end of line or eof.
  else {
    // If there was a '"' then assume it is a C style literal string with \ escapes etc
    // The following will set the bad bit if eof occurs before

    char c = ' ';
    PINDEX count = 0;
    PStringStream str;
    while (strm.peek() != EOF) {
      strm.get(c);
      str << c;

      // Keep reading till get a '"' that is not preceded by a '\' that is not itself preceded by a '\'
      if (c == '"' && count > 0 && (str[count] != '\\' || !(count > 1 && str[count-1] == '\\')))
        break;

      count++;
    }

    if (c != '"') {
      // No closing quote, add one and set fail bit.
      strm.setstate(ios::failbit);
      str << '"';
    }

    m_value = PString(PString::Literal, (const char *)str);
  }
}


bool OpalMediaOptionString::Merge(const OpalMediaOption & option)
{
  if (m_merge != IntersectionMerge)
    return OpalMediaOption::Merge(option);

  const OpalMediaOptionString * otherOption = PDownCast(const OpalMediaOptionString, &option);
  if (otherOption == NULL)
    return false;

  PStringArray mySet = m_value.Tokenise(',');
  PStringArray otherSet = otherOption->m_value.Tokenise(',');
  PINDEX i = 0;
  while (i < mySet.GetSize()) {
    if (otherSet.GetValuesIndex(mySet[i]) != P_MAX_INDEX)
      ++i;
    else
      mySet.RemoveAt(i);
  }

  PStringStream newValue;
  newValue << setfill(',') << mySet;

  if (m_value != newValue) {
    PTRACE(4, "Changed media option \"" << m_name << "\" "
              "from " << m_value << " to " << newValue);
    m_value = newValue;
  }
  return true;
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
    streamsize width = strm.width();
    ios::fmtflags flags = strm.flags();
    char fill = strm.fill();

    streamsize fillLength = width - m_value.GetSize()*2;
    if (fillLength > 0 && (flags&ios_base::adjustfield) == ios::right) {
      for (streamsize i = 0; i < fillLength; i++)
        strm << fill;
    }

    strm << right << hex << setfill('0');
    for (PINDEX i = 0; i < m_value.GetSize(); i++)
      strm << setw(2) << (unsigned)m_value[i];

    if (fillLength > 0 && (flags&ios_base::adjustfield) == ios::left) {
      strm << setw(1);
      for (std::streamsize i = 0; i < fillLength; i++)
        strm << fill;
    }

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
    PINDEX nibble = 0;

    while (strm.peek() != EOF) {
      char ch = (char)strm.get();
      if (isxdigit(ch))
        pair[nibble++] = ch;
      else if (ch == ' ')
        pair[nibble++] = '0';
      else
        break;

      if (nibble == 2) {
        if (!m_value.SetMinSize(100*((count+1+99)/100)))
          break;
        m_value[count++] = (BYTE)strtoul(pair, NULL, 16);
        nibble = 0;
      }
    }

    // Report error if no legal hex, not empty is OK.
    if (count == 0 && !strm.eof())
      strm.setstate(ios::failbit);

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

const PString & OpalMediaFormat::DescriptionOption()   { static const PConstString s("Description");                      return s; }
const PString & OpalMediaFormat::NeedsJitterOption()   { static const PConstString s(PLUGINCODEC_OPTION_NEEDS_JITTER);    return s; }
const PString & OpalMediaFormat::MaxFrameSizeOption()  { static const PConstString s(PLUGINCODEC_OPTION_MAX_FRAME_SIZE);  return s; }
const PString & OpalMediaFormat::FrameTimeOption()     { static const PConstString s(PLUGINCODEC_OPTION_FRAME_TIME);      return s; }
const PString & OpalMediaFormat::ClockRateOption()     { static const PConstString s(PLUGINCODEC_OPTION_CLOCK_RATE);      return s; }
const PString & OpalMediaFormat::MaxBitRateOption()    { static const PConstString s(PLUGINCODEC_OPTION_MAX_BIT_RATE);    return s; }
const PString & OpalMediaFormat::TargetBitRateOption() { static const PConstString s(PLUGINCODEC_OPTION_TARGET_BIT_RATE); return s; }
const PString & OpalMediaFormat::RTCPFeedbackOption()  { static const PConstString s("RTCP Feedback");                    return s; }

#if OPAL_H323
const PString & OpalMediaFormat::MediaPacketizationOption()  { static const PConstString s(PLUGINCODEC_MEDIA_PACKETIZATION);  return s; }
const PString & OpalMediaFormat::MediaPacketizationsOption() { static const PConstString s(PLUGINCODEC_MEDIA_PACKETIZATIONS); return s; }
#endif

const PString & OpalMediaFormat::ProtocolOption()        { static const PConstString s(PLUGINCODEC_OPTION_PROTOCOL);           return s; }
const PString & OpalMediaFormat::MaxTxPacketSizeOption() { static const PConstString s(PLUGINCODEC_OPTION_MAX_TX_PACKET_SIZE); return s; }


OpalMediaFormat::OpalMediaFormat(OpalMediaFormatInternal * info, bool dyn)
  : m_info(NULL)
  , m_dynamic(dyn)
{
  Construct(info);
}


OpalMediaFormat::OpalMediaFormat(RTP_DataFrame::PayloadTypes pt, unsigned clockRate, const char * name, const char * protocol)
  : m_info(NULL)
  , m_dynamic(false)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  OpalMediaFormatList::const_iterator fmt = registeredFormats.FindFormat(pt, clockRate, name, protocol);
  if (fmt != registeredFormats.end())
    *this = *fmt;
}


OpalMediaFormat::OpalMediaFormat(const char * wildcard)
  : m_info(NULL)
  , m_dynamic(false)
{
  operator=(PString(wildcard));
}


OpalMediaFormat::OpalMediaFormat(const PString & wildcard)
  : m_info(NULL)
  , m_dynamic(false)
{
  operator=(wildcard);
}


OpalMediaFormat::OpalMediaFormat(const char * fullName,
                                 const OpalMediaType & mediaType,
                                 RTP_DataFrame::PayloadTypes pt,
                                 const char * en,
                                 PBoolean     nj,
                                 OpalBandwidth bw,
                                 PINDEX   fs,
                                 unsigned ft,
                                 unsigned cr,
                                 time_t ts,
                                 bool am)
  : m_dynamic(false)
{
  Construct(new OpalMediaFormatInternal(fullName, mediaType, pt, en, nj, bw, fs, ft,cr, ts, am));
}


OpalMediaFormat::OpalMediaFormat(const OpalMediaFormat & c)
  : PContainer() // can't use PContainer copy c-tor as this c-tor must be synchronized
  , m_info(NULL)
  , m_dynamic(false)
{
  PWaitAndSignal m(c.m_mutex); // here is no need to use mutex of the shared object
  PContainer::AssignContents(c);
  m_info = c.m_info;
}


OpalMediaFormat::~OpalMediaFormat()
{
  if (m_info != NULL)
    m_info->m_mutex.Wait(); // don't use PWaitAndSignal as m_info can be removed

  Destruct();

  if (m_info != NULL)
    m_info->m_mutex.Signal();
}


void OpalMediaFormat::Construct(OpalMediaFormatInternal * info)
{
  if (info == NULL)
    return;

  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  OpalMediaFormatList::const_iterator fmt = registeredFormats.FindFormat(info->formatName);
  if (fmt != registeredFormats.end()) {
    PAssert(!m_dynamic, PLogicError);

    if (info->codecVersionTime > fmt->m_info->codecVersionTime)
      *fmt->m_info = *info;
    else
      *this = *fmt;
    delete info;
  }
  else {
    m_info = info;
    registeredFormats.OpalMediaFormatBaseList::Append(this);
  }
}


OpalMediaFormat & OpalMediaFormat::operator=(RTP_DataFrame::PayloadTypes pt)
{
  PWaitAndSignal m(m_mutex);

  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  OpalMediaFormatList::const_iterator fmt = registeredFormats.FindFormat(pt);
  if (fmt == registeredFormats.end())
    *this = OpalMediaFormat();
  else if (this != &*fmt)
    *this = *fmt;

  return *this;
}


OpalMediaFormat & OpalMediaFormat::operator=(const char * wildcard)
{
  PWaitAndSignal m(m_mutex);
  return operator=(PString(wildcard));
}


OpalMediaFormat & OpalMediaFormat::operator=(const PString & wildcard)
{
  PWaitAndSignal m(m_mutex);
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  OpalMediaFormatList::const_iterator fmt = registeredFormats.FindFormat(wildcard);
  if (fmt == registeredFormats.end())
    *this = OpalMediaFormat();
  else
    *this = *fmt;

  return *this;
}


PBoolean OpalMediaFormat::MakeUnique()
{
  PWaitAndSignal m1(m_mutex);
  if (m_info == NULL)
    return true;

  PWaitAndSignal m2(m_info->m_mutex);

  if (PContainer::MakeUnique())
    return true;

  m_info = (OpalMediaFormatInternal *)m_info->Clone();
  m_info->options.MakeUnique();
  return false;
}


void OpalMediaFormat::AssignContents(const PContainer & c)
{
  PWaitAndSignal m1(m_mutex);

  const OpalMediaFormat & other = (const OpalMediaFormat &)c;
  PWaitAndSignal m2(other.m_mutex);

  if (m_info != NULL) {
    m_info->m_mutex.Wait(); // don't use PWaitAndSignal as m_info can be removed
    PContainer::AssignContents(c);
    if (m_info != NULL)
      m_info->m_mutex.Signal();
  }
  else // current object can't be removed
    PContainer::AssignContents(c);

  m_info = other.m_info;
}


void OpalMediaFormat::DestroyContents()
{
  m_mutex.Wait();

  if (m_info != NULL) {
    delete m_info;
    m_info = NULL;
  }

  m_mutex.Signal();
}


PObject * OpalMediaFormat::Clone() const
{
  return new OpalMediaFormat(*this);
}


PObject::Comparison OpalMediaFormat::Compare(const PObject & obj) const
{
  PWaitAndSignal m(m_mutex);
  const OpalMediaFormat & other = dynamic_cast<const OpalMediaFormat &>(obj);
  if (m_info == other.m_info)
    return EqualTo;
  if (other.m_info == NULL)
    return GreaterThan;
  if (m_info == NULL)
    return LessThan;
  return m_info->formatName.Compare(other.m_info->formatName);
}


void OpalMediaFormat::PrintOn(ostream & strm) const
{
  PWaitAndSignal m(m_mutex);
  if (m_info != NULL)
    strm << *m_info;
}


void OpalMediaFormat::ReadFrom(istream & strm)
{
  PWaitAndSignal m(m_mutex);
  char fmt[100];
  strm >> fmt;
  operator=(fmt);
}


#if OPAL_VIDEO
static bool IsPresentationRole(const OpalMediaFormat & mediaFormat)
{
  unsigned mask = mediaFormat.GetOptionInteger(OpalVideoFormat::ContentRoleMaskOption());
  mask &= ~OpalVideoFormat::ContentRoleBit(OpalVideoFormat::eMainRole);
  return mask != 0;
}
#endif


bool OpalMediaFormat::IsMediaType(const OpalMediaType & mediaType) const
{
  PWaitAndSignal m(m_mutex);
  if (m_info == NULL)
    return false;

  if (m_info->mediaType == mediaType)
    return true;

#if OPAL_VIDEO
  if (mediaType == OpalPresentationVideoMediaDefinition::Name() && IsPresentationRole(*this))
    return true;
#endif

  return false;
}


bool OpalMediaFormat::ToNormalisedOptions()
{
  PWaitAndSignal m(m_mutex);
  MakeUnique();
  return m_info != NULL && m_info->ToNormalisedOptions();
}


bool OpalMediaFormat::ToCustomisedOptions()
{
  PWaitAndSignal m(m_mutex);
  MakeUnique();
  return m_info != NULL && m_info->ToCustomisedOptions();
}


bool OpalMediaFormat::Update(const OpalMediaFormat & mediaFormat)
{
  if (!mediaFormat.IsValid())
    return true;

  PWaitAndSignal m(m_mutex);

  if (m_info == mediaFormat.m_info) {
    PTRACE(4, "Update to same object for " << *this);
    return true;
  }

  if (!IsValid()) {
    PTRACE(4, "Update (initial) of " << *this);
    *this = mediaFormat;
    return true;
  }

  if (*this != mediaFormat) {
    PTRACE(4, "Update (merge) of " << *this << " from " << mediaFormat);
    SetPayloadType(mediaFormat.GetPayloadType()); // Does MakeUnique()
    return m_info->OpalMediaFormatInternal::Merge(*mediaFormat.m_info);
  }

  PTRACE(4, "Update (overwrite) of " << *this);
  *this = mediaFormat;
  return true;
}


bool OpalMediaFormat::Merge(const OpalMediaFormat & mediaFormat, bool copyPayloadType)
{
  PWaitAndSignal m(m_mutex);
  MakeUnique();

  if (copyPayloadType && GetPayloadType() != mediaFormat.GetPayloadType()) {
    PTRACE(4, "Changing payload type from " << GetPayloadType()
           << " to " << mediaFormat.GetPayloadType() << " in " << *this);
    SetPayloadType(mediaFormat.GetPayloadType());
  }

  return m_info != NULL && mediaFormat.m_info != NULL && m_info->Merge(*mediaFormat.m_info);
}


bool OpalMediaFormat::ValidateMerge(const OpalMediaFormat & mediaFormat) const
{
  PWaitAndSignal m(m_mutex);
  return m_info != NULL && mediaFormat.m_info != NULL && m_info->ValidateMerge(*mediaFormat.m_info);
}


#if OPAL_H323
PStringArray OpalMediaFormat::GetMediaPacketizations() const
{
  return GetOptionString(OpalMediaFormat::MediaPacketizationsOption(),
                         GetOptionString(OpalMediaFormat::MediaPacketizationOption())).Tokenise(",");
}


void OpalMediaFormat::SetMediaPacketizations(const PStringSet & packetizations)
{
  if (packetizations.IsEmpty()) {
    SetOptionString(MediaPacketizationsOption(), PString::Empty());
    SetOptionString(MediaPacketizationOption(),  PString::Empty());
  }
  else {
    PStringStream strm;
    strm << setfill(',') << packetizations;
    SetOptionString(MediaPacketizationsOption(), strm);
    SetOptionString(MediaPacketizationOption(),  packetizations.GetKeyAt(0));
  }
}
#endif


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

  for (OpalMediaFormatList::const_iterator format = registeredFormats.begin(); format != registeredFormats.end(); ++format)
    copy += *format;
}


bool OpalMediaFormat::RegisterKnownMediaFormats(const PString & name)
{
  typedef OpalMediaFormat & (*MediaFormatFunction)();
  static struct {
    const char * m_name;
    MediaFormatFunction m_function;
    #define KNOWN2(name, func) { OPAL_##name, (MediaFormatFunction)GetOpal##func }
    #define KNOWN(codec) KNOWN2(codec, codec)
  } const known[] = {
    KNOWN(G722),
    KNOWN(G7221_24K),
    KNOWN(G7221_32K),
    KNOWN(G7222),
    KNOWN(G726_40K),
    KNOWN(G726_32K),
    KNOWN(G726_24K),
    KNOWN(G726_16K),
    KNOWN(G728),
    KNOWN(G729),
    KNOWN(G729A),
    KNOWN(G729B),
    KNOWN(G729AB),
    KNOWN(G7231_6k3),
    KNOWN(G7231_5k3),
    KNOWN(G7231A_6k3),
    KNOWN(G7231A_5k3),
    KNOWN(G7231_Cisco_A),
    KNOWN(G7231_Cisco_AR),
    KNOWN(GSM0610),
    KNOWN(GSMAMR),
    KNOWN(iLBC),
    KNOWN2(SPEEX_NB, SpeexNB),
    KNOWN2(SPEEX_WB, SpeexWB),
    KNOWN2(OPUS8,   Opus8),
    KNOWN2(OPUS8S,  Opus8S),
    KNOWN2(OPUS12,  Opus12),
    KNOWN2(OPUS12S, Opus12S),
    KNOWN2(OPUS16,  Opus16),
    KNOWN2(OPUS16S, Opus16S),
    KNOWN2(OPUS24,  Opus24),
    KNOWN2(OPUS24S, Opus24S),
    KNOWN2(OPUS48,  Opus48),
    KNOWN2(OPUS48S, Opus48S),
#if OPAL_VIDEO
    KNOWN(H261),
    KNOWN(H263),
    KNOWN(H263plus),
    KNOWN(H264_MODE0),
    KNOWN(H264_MODE1),
    KNOWN(MPEG4),
    KNOWN(VP8),
#endif
  };

  bool atLeastOne = false;

  for (PINDEX i = 0; i < PARRAYSIZE(known); ++i) {
    if (name.IsEmpty() || (name *= known[i].m_name)) {
      (known[i].m_function)();
      PTRACE(5, NULL, PTraceModule(),
             "Known media " << known[i].m_name << " registered as:\n"
             << setw(-1) << OpalMediaFormat(known[i].m_name));
      atLeastOne = true;
    }
  }

  return atLeastOne;
}


bool OpalMediaFormat::SetRegisteredMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  for (OpalMediaFormatList::iterator format = registeredFormats.begin(); format != registeredFormats.end(); ++format) {
    if (*format == mediaFormat) {
      /* Yes, this looks a little odd as we just did equality above and seem to
         be assigning the left hand side with exactly the same value. But what
         is really happening is the above only compares the name, and below
         copies all of the attributes (OpalMediaFormatOtions) across. */
      *format->m_info = *mediaFormat.m_info;
      format->m_info->options.MakeUnique();
      return true;
    }
  }

  return false;
}


bool OpalMediaFormat::RemoveRegisteredMediaFormats(const PString & wildcard)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  bool found = false;
  OpalMediaFormatList::const_iterator format;
  while ((format = registeredFormats.FindFormat(wildcard)) != registeredFormats.end()) {
    if (format->m_dynamic)
      delete &*format;
    registeredFormats.erase(format);
    found = true;
  }

  return found;
}


/////////////////////////////////////////////////////////////////////////////

OpalMediaFormatInternal::OpalMediaFormatInternal(const char * fullName,
                                                 const OpalMediaType & _mediaType,
                                                 RTP_DataFrame::PayloadTypes pt,
                                                 const char * en,
                                                 PBoolean     nj,
                                                 OpalBandwidth bw,
                                                 PINDEX   fs,
                                                 unsigned ft,
                                                 unsigned cr,
                                                 time_t   ts,
                                                 bool     am)
  : formatName(fullName)
  , rtpPayloadType(pt)
  , rtpEncodingName(en)
  , mediaType(_mediaType)
  , codecVersionTime(ts)
  , forceIsTransportable(false)
  , m_allowMultiple(am)
{

  AddOption(new OpalMediaOptionString(OpalMediaFormat::DescriptionOption(), true, fullName));

  if (nj)
    AddOption(new OpalMediaOptionBoolean(OpalMediaFormat::NeedsJitterOption(), true, OpalMediaOption::OrMerge, true));

  if (bw > 0) {
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::MaxBitRateOption(), true, OpalMediaOption::MinMerge, bw, 100));
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::TargetBitRateOption(), false, OpalMediaOption::AlwaysMerge, bw, 100));
  }

  if (fs > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::MaxFrameSizeOption(), true, OpalMediaOption::NoMerge, fs));

  if (ft > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::FrameTimeOption(), true, OpalMediaOption::NoMerge, ft));

  if (cr > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::ClockRateOption(), true, OpalMediaOption::NoMerge, cr));

  AddOption(new OpalMediaOptionEnum(OpalMediaFormat::RTCPFeedbackOption(), false,
                                    OpalMediaFormat::RTCPFeedback().Names(), P_MAX_INDEX,
                                    OpalMediaOption::IntersectionMerge,
                                    OpalMediaFormat::e_NoRTCPFb));

  AddOption(new OpalMediaOptionString(OpalMediaFormat::ProtocolOption(), true));

  // assume non-dynamic payload types are correct and do not need deconflicting
  if (rtpPayloadType < RTP_DataFrame::DynamicBase || rtpPayloadType >= RTP_DataFrame::MaxPayloadType) {
    if (rtpPayloadType == RTP_DataFrame::MaxPayloadType && 
        rtpEncodingName.GetLength() > 0 &&
        rtpEncodingName[0] == '+') {
      forceIsTransportable = true;
      rtpEncodingName.Delete(0, 1);
    }
    return;
  }

  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  DeconflictPayloadTypes(GetMediaFormatsList());
}


void OpalMediaFormatInternal::DeconflictPayloadTypes(OpalMediaFormatList & formats)
{
  OpalMediaFormat * conflictingFormat = NULL;

  // Build a table of all the unused payload types
  bool inUse[RTP_DataFrame::IllegalPayloadType+1];
  memset(inUse, 0, sizeof(inUse));

  // Do not use the "forbidden zone"
  for (int i = RTP_DataFrame::StartConflictRTCP; i <= RTP_DataFrame::EndConflictRTCP; ++i)
    inUse[i] = true;

  // Search for conflicting RTP Payload Type, collecting in use payload types along the way
  for (OpalMediaFormatList::iterator format = formats.begin(); format != formats.end(); ++format) {
    inUse[format->GetPayloadType()] = true;

    // A conflict is when we are after an explicit payload type, we have found one already using it
    if (rtpPayloadType > RTP_DataFrame::DynamicBase && rtpPayloadType  == format->GetPayloadType()) {
      // If it is a shared payload types, which happens when encoding name is the same, then allow it
      if (!m_allowMultiple && rtpEncodingName == format->GetEncodingName())
        return;

      // Have a conflicting media format, move it later when we know where to
      conflictingFormat = &*format;
    }
  }

  if (!inUse[rtpPayloadType]) {
    PTRACE(5, "Using provided payload type " << rtpPayloadType << " for " << formatName);
    return;
  }

  // Determine next unused payload type, if all the dynamic ones are allocated then
  // we start downward toward the well known values.
  int nextUnused = RTP_DataFrame::DynamicBase;
  while (inUse[nextUnused]) {
    if (nextUnused < RTP_DataFrame::DynamicBase)
      --nextUnused;
    else if (++nextUnused >= RTP_DataFrame::MaxPayloadType)
      nextUnused = RTP_DataFrame::DynamicBase-1;
  }

  // If we had a conflict we change the older one, as it is assumed that the
  // application really wanted that value and internal OPAL ones can move
  if (conflictingFormat == NULL || m_allowMultiple) {
    RTP_DataFrame::PayloadTypes newPT = (RTP_DataFrame::PayloadTypes)nextUnused;
    PTRACE(4, "Replacing payload type " << rtpPayloadType << " with " << newPT << " for " << formatName);
    rtpPayloadType = newPT;
  }
  else {
    PTRACE(3, "Conflicting payload type: "
           << *conflictingFormat << " moved to " << nextUnused
           << " as " << formatName << " requires " << rtpPayloadType);
    conflictingFormat->SetPayloadType((RTP_DataFrame::PayloadTypes)nextUnused);
  }
}


PObject * OpalMediaFormatInternal::Clone() const
{
  PWaitAndSignal m1(m_mutex);
  return new OpalMediaFormatInternal(*this);
}


bool OpalMediaFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PTRACE(4, "Merging " << mediaFormat << " into " << *this);

  PWaitAndSignal m1(m_mutex);
  PWaitAndSignal m2(mediaFormat.m_mutex);

  for (PINDEX i = 0; i < options.GetSize(); i++) {
    OpalMediaOption & opt = options[i];
    PString name = opt.GetName();
    OpalMediaOption * option = mediaFormat.FindOption(opt.GetName());
    if (option == NULL) {
      PTRACE_IF(3, formatName == mediaFormat.formatName, "Cannot merge unmatched option " << opt.GetName());
    }
    else 
    {
      PAssert(option->GetName() == opt.GetName(), "find returned bad name");
      if (!opt.Merge(*option))
        return false;
    }
  }

  return true;
}


bool OpalMediaFormatInternal::ValidateMerge(const OpalMediaFormatInternal & mediaFormat) const
{
  PWaitAndSignal m1(m_mutex);
  PWaitAndSignal m2(mediaFormat.m_mutex);

  for (PINDEX i = 0; i < options.GetSize(); i++) {
    OpalMediaOption & opt = options[i];
    PString name = opt.GetName();
    OpalMediaOption * option = mediaFormat.FindOption(opt.GetName());
    if (option == NULL) {
      PTRACE_IF(2, formatName == mediaFormat.formatName, "Validate: unmatched option " << opt.GetName());
    }
    else {
      PAssert(option->GetName() == opt.GetName(), "find returned bad name");
      if (!opt.ValidateMerge(*option))
        return false;
    }
  }

  return true;
}


bool OpalMediaFormatInternal::ToNormalisedOptions()
{
  return true;
}


bool OpalMediaFormatInternal::ToCustomisedOptions()
{
  return true;
}


bool OpalMediaFormatInternal::IsValid() const
{
  return rtpPayloadType < RTP_DataFrame::IllegalPayloadType && !formatName.IsEmpty();
}


bool OpalMediaFormatInternal::IsTransportable() const
{
  return forceIsTransportable || !rtpEncodingName.IsEmpty() || rtpPayloadType < RTP_DataFrame::LastKnownPayloadType;
}


PStringToString OpalMediaFormatInternal::GetOptions() const
{
  PWaitAndSignal m1(m_mutex);
  PStringToString dict;
  for (PINDEX i = 0; i < options.GetSize(); i++)
    dict.SetAt(options[i].GetName(), options[i].AsString());
  return dict;
}


bool OpalMediaFormatInternal::GetOptionValue(const PString & name, PString & value) const
{
  PWaitAndSignal m(m_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  value = option->AsString();
  return true;
}


bool OpalMediaFormatInternal::SetOptionValue(const PString & name, const PString & value)
{
  PWaitAndSignal m(m_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  return option->FromString(value);
}


template <class OptionType, typename ValueType>
static ValueType GetOptionOfType(const OpalMediaFormatInternal & format, const PString & name, ValueType dflt)
{
  OpalMediaOption * option = format.FindOption(name);
  if (option == NULL)
    return dflt;

  OptionType * typedOption = dynamic_cast<OptionType *>(option);
  if (typedOption != NULL)
    return typedOption->GetValue();

  PTRACE(1, "Invalid type for getting option " << name << " in " << format);
  PAssertAlways(PInvalidCast);
  return dflt;
}


template <class OptionType, typename ValueType>
static bool SetOptionOfType(OpalMediaFormatInternal & format, const PString & name, ValueType value)
{
  OpalMediaOption * option = format.FindOption(name);
  if (option == NULL)
    return false;

  OptionType * typedOption = dynamic_cast<OptionType *>(option);
  if (typedOption != NULL) {
    typedOption->SetValue(value);
    return true;
  }

  PTRACE(1, "Invalid type for setting option " << name << " in " << format);
  PAssertAlways(PInvalidCast);
  return false;
}


bool OpalMediaFormatInternal::GetOptionBoolean(const PString & name, bool dflt) const
{
  PWaitAndSignal m(m_mutex);
  const OpalMediaOptionEnum * optEnum = dynamic_cast<const OpalMediaOptionEnum *>(FindOption(name));
  if (optEnum != NULL && optEnum->GetEnumerations().GetSize() == 2)
    return optEnum->GetValue() != 0;

  return GetOptionOfType<OpalMediaOptionBoolean, bool>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionBoolean(const PString & name, bool value)
{
  PWaitAndSignal m(m_mutex);
  OpalMediaOptionEnum * optEnum = dynamic_cast<OpalMediaOptionEnum *>(FindOption(name));
  if (optEnum != NULL && optEnum->GetEnumerations().GetSize() == 2) {
    optEnum->SetValue(value);
    return true;
  }

  return SetOptionOfType<OpalMediaOptionBoolean, bool>(*this, name, value);
}


int OpalMediaFormatInternal::GetOptionInteger(const PString & name, int dflt) const
{
  PWaitAndSignal m(m_mutex);
  OpalMediaOptionUnsigned * optUnsigned = dynamic_cast<OpalMediaOptionUnsigned *>(FindOption(name));
  if (optUnsigned != NULL)
    return optUnsigned->GetValue();

  return GetOptionOfType<OpalMediaOptionInteger, int>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionInteger(const PString & name, int value)
{
  PWaitAndSignal m(m_mutex);
  OpalMediaOptionUnsigned * optUnsigned = dynamic_cast<OpalMediaOptionUnsigned *>(FindOption(name));
  if (optUnsigned != NULL) {
    optUnsigned->SetValue(value);
    return true;
  }

  return SetOptionOfType<OpalMediaOptionInteger, int>(*this, name, value);
}


double OpalMediaFormatInternal::GetOptionReal(const PString & name, double dflt) const
{
  PWaitAndSignal m(m_mutex);
  return GetOptionOfType<OpalMediaOptionReal, double>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionReal(const PString & name, double value)
{
  PWaitAndSignal m(m_mutex);
  return SetOptionOfType<OpalMediaOptionReal, double>(*this, name, value);
}


PINDEX OpalMediaFormatInternal::GetOptionEnum(const PString & name, PINDEX dflt) const
{
  PWaitAndSignal m(m_mutex);
  return GetOptionOfType<OpalMediaOptionEnum, PINDEX>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionEnum(const PString & name, PINDEX value)
{
  PWaitAndSignal m(m_mutex);
  return SetOptionOfType<OpalMediaOptionEnum, PINDEX>(*this, name, value);
}


PString OpalMediaFormatInternal::GetOptionString(const PString & name, const PString & dflt) const
{
  PWaitAndSignal m(m_mutex);
  return GetOptionOfType<OpalMediaOptionString, PString>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionString(const PString & name, const PString & value)
{
  PWaitAndSignal m(m_mutex);
  return SetOptionOfType<OpalMediaOptionString, PString>(*this, name, value);
}


bool OpalMediaFormatInternal::GetOptionOctets(const PString & name, PBYTEArray & octets) const
{
  PWaitAndSignal m(m_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  octets = PDownCast(OpalMediaOptionOctets, option)->GetValue();
  return true;
}


bool OpalMediaFormatInternal::SetOptionOctets(const PString & name, const PBYTEArray & octets)
{
  PWaitAndSignal m(m_mutex);
  return SetOptionOfType<OpalMediaOptionOctets, const PBYTEArray &>(*this, name, octets);
}


bool OpalMediaFormatInternal::SetOptionOctets(const PString & name, const BYTE * data, PINDEX length)
{
  PWaitAndSignal m(m_mutex);
  return SetOptionOfType<OpalMediaOptionOctets, const PBYTEArray &>(*this, name, PBYTEArray(data, length));
}


bool OpalMediaFormatInternal::AddOption(OpalMediaOption * option, PBoolean overwrite)
{
  PWaitAndSignal m(m_mutex);
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
  PWaitAndSignal m(m_mutex);
  OpalMediaOptionSearchArg search(name);
  PSortedList<OpalMediaOption>::const_iterator it = options.find(search);
  if (it == options.end())
    return NULL;

  PAssert(it->GetName() == name, "OpalMediaOption name mismatch");

  return const_cast<OpalMediaOption *>(&*it);
}


bool OpalMediaFormatInternal::IsValidForProtocol(const PString & protocol) const
{
  PWaitAndSignal m(m_mutex);

  // the protocol is only valid for SIP if the RTP name is not NULL
  if (protocol *= "sip")
    return !rtpEncodingName.IsEmpty() || forceIsTransportable;

  return true;
}


bool OpalMediaFormatInternal::AdjustByOptionMaps(PTRACE_PARAM(const char * operation,)
                  bool (*adjuster)(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed))
{
  PWaitAndSignal m(m_mutex);

#if PTRACING
  if (PTrace::CanTrace(5))
    PTRACE(5, operation << ":\n" << setw(-1) << *this);
  else
    PTRACE(4, operation << ": " << *this);
#endif

  PluginCodec_OptionMap original;
  for (PINDEX i = 0; i < options.GetSize(); i++)
    original[options[i].GetName()] = options[i].AsString().GetPointer();

  PluginCodec_OptionMap changed;
  if (!adjuster(original, changed))
    return false;

  for (PluginCodec_OptionMap::const_iterator it = changed.begin(); it != changed.end(); ++it) {
    PString oldValue;
    if (GetOptionValue(it->first, oldValue) && oldValue != it->second.c_str()) {
      PTRACE(3, "Changed option \"" << it->first << "\" from \"" << oldValue << "\" to \"" << it->second << '"');
      SetOptionValue(it->first, it->second);
    }
  }

  return true;
}


void OpalMediaFormatInternal::PrintOn(ostream & strm) const
{
  PINDEX i;
  PWaitAndSignal m(m_mutex);

  if (strm.width() != -1) {
    strm << formatName;
    return;
  }

  PINDEX TitleWidth = 20;
  for (i = 0; i < options.GetSize(); i++) {
    PINDEX width =options[i].GetName().GetLength();
    if (width > TitleWidth)
      TitleWidth = width;
  }

  strm << right << setw(TitleWidth) <<   "Format Name" << left << "       = " << formatName << '\n'
       << right << setw(TitleWidth) <<    "Media Type" << left << "       = " << mediaType << '\n'
       << right << setw(TitleWidth) <<  "Payload Type" << left << "       = " << rtpPayloadType << '\n'
       << right << setw(TitleWidth) << "Encoding Name" << left << "       = " << rtpEncodingName << '\n';
  for (i = 0; i < options.GetSize(); i++) {
    const OpalMediaOption & option = options[i];
#if OPAL_H323
    const OpalMediaOption::H245GenericInfo & genericInfo = option.GetH245Generic();
#endif // OPAL_H323

    strm << right << setw(TitleWidth) << option.GetName() << " (R/" << (option.IsReadOnly() ? 'O' : 'W')
         << ") = " << left << setw(20) << option << ' ' << setw(14);

    // Show the type of the option: Boolean, Unsigned, String, etc.
    if (PIsDescendant(&option, OpalMediaOptionBoolean))
      strm << "Boolean";
    else if (PIsDescendant(&option, OpalMediaOptionUnsigned))
#if OPAL_H323
      switch (genericInfo.integerType) {
        default :
        case OpalMediaOption::H245GenericInfo::UnsignedInt :
          strm << "UnsignedInt";
          break;
        case OpalMediaOption::H245GenericInfo::Unsigned32 :
          strm << "Unsigned32";
          break;
        case OpalMediaOption::H245GenericInfo::BooleanArray :
          strm << "BooleanArray";
          break;
      }
#else
      strm << "UnsignedInt";
#endif // OPAL_H323
    else if (PIsDescendant(&option, OpalMediaOptionOctets))
      strm << "OctetString";
    else if (PIsDescendant(&option, OpalMediaOptionString))
      strm << "String";
    else if (PIsDescendant(&option, OpalMediaOptionEnum))
      strm << "Enum";
    else
      strm << "Unknown";

#if OPAL_SDP
    if (!option.GetFMTPName().IsEmpty())
      strm << " FMTP name: " << option.GetFMTPName() << " (" << option.GetFMTPDefault() << ')';
#endif // OPAL_SDP

#if OPAL_H323
    if (genericInfo.mode != OpalMediaOption::H245GenericInfo::None) {
      strm << " H.245 Ordinal: " << setw(2) << genericInfo.ordinal
        << ' ' << (genericInfo.mode == OpalMediaOption::H245GenericInfo::Collapsing ? "Collapsing" : "Non-Collapsing");
      if (!genericInfo.excludeTCS)
        strm << " TCS";
      if (!genericInfo.excludeOLC)
        strm << " OLC";
      if (!genericInfo.excludeReqMode)
        strm << " RM";
    }
#endif // OPAL_H323

    strm << '\n';
  }
  strm << endl;
}

///////////////////////////////////////////////////////////////////////////////

const PString & OpalAudioFormat::RxFramesPerPacketOption() { static const PConstString s(PLUGINCODEC_OPTION_RX_FRAMES_PER_PACKET); return s; }
const PString & OpalAudioFormat::TxFramesPerPacketOption() { static const PConstString s(PLUGINCODEC_OPTION_TX_FRAMES_PER_PACKET); return s; }
const PString & OpalAudioFormat::MaxFramesPerPacketOption(){ static const PConstString s("Max Frames Per Packet"); return s; }
const PString & OpalAudioFormat::ChannelsOption()          { static const PConstString s("Channels"); return s; }
#if OPAL_SDP
const PString & OpalAudioFormat::MinPacketTimeOption()     { static const PConstString s("minptime"); return s; }
const PString & OpalAudioFormat::MaxPacketTimeOption()     { static const PConstString s("maxptime"); return s; }
#endif

OpalAudioFormat::OpalAudioFormat(Internal * info, bool dynamic)
  : OpalMediaFormat(info, dynamic)
{
}


OpalAudioFormat::OpalAudioFormat(const char * fullName,
                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                 const char * encodingName,
                                 PINDEX   frameSize,
                                 unsigned frameTime,
                                 unsigned rxFrames,
                                 unsigned txFrames,
                                 unsigned maxFrames,
                                 unsigned clockRate,
                                 time_t timeStamp,
                                 unsigned channels)
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
                                        timeStamp,
                                        channels));
}


OpalAudioFormat & OpalAudioFormat::operator=(const OpalMediaFormat & other)
{
  if (dynamic_cast<OpalAudioFormatInternal *>(other.m_info) != NULL)
    OpalMediaFormat::operator=(other);
  else
    OpalMediaFormat::operator=(OpalMediaFormat());
  return *this;
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
                                                 time_t timeStamp,
                                                 unsigned channels)
  : OpalMediaFormatInternal(fullName,
                            "audio",
                            rtpPayloadType,
                            encodingName,
                            true,
                            8*frameSize*clockRate/frameTime,  // bits per second = 8*frameSize * framesPerSecond
                            frameSize,
                            frameTime,
                            clockRate,
                            timeStamp)
{
  if (rxFrames > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::RxFramesPerPacketOption(), false, OpalMediaOption::MinMerge,  rxFrames, 1, maxFrames));
  if (txFrames > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::TxFramesPerPacketOption(), false, OpalMediaOption::AlwaysMerge, txFrames, 1, maxFrames));

  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::MaxFramesPerPacketOption(), true,  OpalMediaOption::NoMerge,  maxFrames));
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::ChannelsOption(),           false, OpalMediaOption::NoMerge,  channels, 1, 5));
#if OPAL_SDP
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::MinPacketTimeOption(),      false, OpalMediaOption::NoMerge));
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::MaxPacketTimeOption(),      false, OpalMediaOption::NoMerge));
#endif
}


PObject * OpalAudioFormatInternal::Clone() const
{
  PWaitAndSignal m(m_mutex);
  return new OpalAudioFormatInternal(*this);
}


bool OpalAudioFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PWaitAndSignal m1(m_mutex);
  PWaitAndSignal m2(mediaFormat.m_mutex);

  if (!OpalMediaFormatInternal::Merge(mediaFormat))
    return false;

  Clamp(*this, mediaFormat, OpalAudioFormat::TxFramesPerPacketOption(), PString::Empty(), OpalAudioFormat::RxFramesPerPacketOption());
  return true;
}


OpalAudioFormat::FrameType OpalAudioFormat::GetFrameType(const BYTE * payloadPtr, PINDEX payloadSize, FrameDetectorPtr & detector) const
{
  PWaitAndSignal m(m_mutex);
  if (m_info == NULL)
    return e_UnknownFrameType;

  return dynamic_cast<OpalAudioFormatInternal *>(m_info)->GetFrameType(payloadPtr, payloadSize, detector);
}


OpalAudioFormat::FrameType OpalAudioFormatInternal::GetFrameType(const BYTE * payloadPtr,
                                                                 PINDEX payloadSize,
                                                                 OpalAudioFormat::FrameDetectorPtr & detector) const
{
  if (detector.get() == NULL) {
    detector.reset(OpalAudioFormat::FrameDetectFactory::CreateInstance(rtpEncodingName));
    if (detector.get() == NULL)
      return OpalAudioFormat::e_UnknownFrameType;
  }

  return detector->GetFrameType(payloadPtr, payloadSize, GetOptionInteger(OpalMediaFormat::ClockRateOption(), OpalMediaFormat::AudioClockRate));
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

const PString & OpalVideoFormat::FrameWidthOption()               { static const PConstString s(PLUGINCODEC_OPTION_FRAME_WIDTH);               return s; }
const PString & OpalVideoFormat::FrameHeightOption()              { static const PConstString s(PLUGINCODEC_OPTION_FRAME_HEIGHT);              return s; }
const PString & OpalVideoFormat::MinRxFrameWidthOption()          { static const PConstString s(PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);        return s; }
const PString & OpalVideoFormat::MinRxFrameHeightOption()         { static const PConstString s(PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);       return s; }
const PString & OpalVideoFormat::MaxRxFrameWidthOption()          { static const PConstString s(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);        return s; }
const PString & OpalVideoFormat::MaxRxFrameHeightOption()         { static const PConstString s(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);       return s; }
const PString & OpalVideoFormat::TemporalSpatialTradeOffOption()  { static const PConstString s(PLUGINCODEC_OPTION_TEMPORAL_SPATIAL_TRADE_OFF);return s; }
const PString & OpalVideoFormat::TxKeyFramePeriodOption()         { static const PConstString s(PLUGINCODEC_OPTION_TX_KEY_FRAME_PERIOD);       return s; }
const PString & OpalVideoFormat::RateControlPeriodOption()        { static const PConstString s(PLUGINCODEC_OPTION_RATE_CONTROL_PERIOD);       return s; }
const PString & OpalVideoFormat::FrameDropOption()                { static const PConstString s("Frame Drop");                                 return s; }
const PString & OpalVideoFormat::FreezeUntilIntraFrameOption()    { static const PConstString s("Freeze Until Intra-Frame");                   return s; }
const PString & OpalVideoFormat::ContentRoleOption()              { static const PConstString s("Content Role");                               return s; }
const PString & OpalVideoFormat::ContentRoleMaskOption()          { static const PConstString s("Content Role Mask");                          return s; }
#if OPAL_SDP
const PString & OpalVideoFormat::UseImageAttributeInSDP()         { static const PConstString s("Use Image Attribute in SDP"); return s; }
#endif


OpalVideoFormat::OpalVideoFormat(OpalVideoFormatInternal * info, bool dynamic)
  : OpalMediaFormat(info, dynamic)
{
}


OpalVideoFormat & OpalVideoFormat::operator=(const OpalMediaFormat & other)
{
  if (dynamic_cast<OpalVideoFormatInternal *>(other.m_info) != NULL)
    OpalMediaFormat::operator=(other);
  else
    OpalMediaFormat::operator=(OpalMediaFormat());
  return *this;
}


OpalVideoFormat::OpalVideoFormat(const char * fullName,
                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                 const char * encodingName,
                                 unsigned maxFrameWidth,
                                 unsigned maxFrameHeight,
                                 unsigned maxFrameRate,
                                 unsigned maxBitRate,
                                 time_t timeStamp)
{
  Construct(new OpalVideoFormatInternal(fullName,
                                        rtpPayloadType,
                                        encodingName,
                                        maxFrameWidth,
                                        maxFrameHeight,
                                        maxFrameRate,
                                        maxBitRate,
                                        timeStamp));
}


OpalVideoFormatInternal::OpalVideoFormatInternal(const char * fullName,
                                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                                 const char * encodingName,
                                                 unsigned maxFrameWidth,
                                                 unsigned maxFrameHeight,
                                                 unsigned maxFrameRate,
                                                 unsigned maxBitRate,
                                                 time_t timeStamp)
  : OpalMediaFormatInternal(fullName,
                            "video",
                            rtpPayloadType,
                            encodingName,
                            false,
                            maxBitRate,
                            0,
                            OpalMediaFormat::VideoClockRate/maxFrameRate,
                            OpalMediaFormat::VideoClockRate,
                            timeStamp)
{
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::FrameWidthOption(),               false, OpalMediaOption::AlwaysMerge, PVideoFrameInfo::CIFWidth,   16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::FrameHeightOption(),              false, OpalMediaOption::AlwaysMerge, PVideoFrameInfo::CIFHeight,  16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MinRxFrameWidthOption(),          false, OpalMediaOption::MaxMerge,    PVideoFrameInfo::SQCIFWidth, 16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MinRxFrameHeightOption(),         false, OpalMediaOption::MaxMerge,    PVideoFrameInfo::SQCIFHeight,16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MaxRxFrameWidthOption(),          false, OpalMediaOption::MinMerge,    maxFrameWidth,               16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MaxRxFrameHeightOption(),         false, OpalMediaOption::MinMerge,    maxFrameHeight,              16,  32767));
  if (rtpPayloadType < RTP_DataFrame::LastKnownPayloadType || encodingName != NULL) {
    AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::TxKeyFramePeriodOption(),       false, OpalMediaOption::AlwaysMerge, 125,                         0, INT_MAX));
    AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::RateControlPeriodOption(),      false, OpalMediaOption::AlwaysMerge, 1000,                        100, 60000));
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::MaxTxPacketSizeOption(),        true,  OpalMediaOption::MinMerge, PluginCodec_RTP_MaxPayloadSize, 100       ));
    AddOption(new OpalMediaOptionBoolean (OpalVideoFormat::FrameDropOption(),              false, OpalMediaOption::NoMerge,     true                                   ));
    AddOption(new OpalMediaOptionBoolean (OpalVideoFormat::FreezeUntilIntraFrameOption(),  false, OpalMediaOption::NoMerge,     false                                  ));
#if OPAL_SDP
    AddOption(new OpalMediaOptionEnum    (OpalVideoFormat::UseImageAttributeInSDP(),       false,
                                          OpalVideoFormat::PEnumNames_ImageAttributeInSDP::Names(), OpalVideoFormat::NumImageAttributeInSDP,
                                          OpalMediaOption::AlwaysMerge, OpalVideoFormat::ImageAddrOffered));
#endif
  }

  static const char * const RoleEnumerations[OpalVideoFormat::EndContentRole] = {
    "No Role",
    "Presentation",
    "Main",
    "Speaker",
    "Sign Language"
  };
  AddOption(new OpalMediaOptionEnum(OpalVideoFormat::ContentRoleOption(), false,
                                    RoleEnumerations, PARRAYSIZE(RoleEnumerations),
                                    OpalMediaOption::AlwaysMerge));

  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::ContentRoleMaskOption(),
                                        false, OpalMediaOption::IntersectionMerge,
                                        OpalVideoFormat::ContentRoleMask,
                                        0, OpalVideoFormat::ContentRoleMask));

  // For video the max bit rate and frame rate is adjustable by user
  FindOption(OpalVideoFormat::MaxBitRateOption())->SetReadOnly(false);
  FindOption(OpalVideoFormat::FrameTimeOption())->SetReadOnly(false);
  FindOption(OpalVideoFormat::FrameTimeOption())->SetMerge(OpalMediaOption::MaxMerge);
  SetOptionEnum(OpalMediaFormat::RTCPFeedbackOption(), OpalMediaFormat::e_PLI|OpalMediaFormat::e_FIR|OpalMediaFormat::e_TMMBR|OpalMediaFormat::e_TSTR);
}


PObject * OpalVideoFormatInternal::Clone() const
{
  PWaitAndSignal m(m_mutex);
  return new OpalVideoFormatInternal(*this);
}


bool OpalVideoFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PWaitAndSignal m(m_mutex);

  if (!OpalMediaFormatInternal::Merge(mediaFormat))
    return false;

  Clamp(*this, mediaFormat, OpalVideoFormat::TargetBitRateOption(), PString::Empty(),                          OpalMediaFormat::MaxBitRateOption());
  Clamp(*this, mediaFormat, OpalVideoFormat::FrameWidthOption(),    OpalVideoFormat::MinRxFrameWidthOption(),  OpalVideoFormat::MaxRxFrameWidthOption());
  Clamp(*this, mediaFormat, OpalVideoFormat::FrameHeightOption(),   OpalVideoFormat::MinRxFrameHeightOption(), OpalVideoFormat::MaxRxFrameHeightOption());

  return true;
}


void OpalMediaFormat::AdjustVideoArgs(PVideoDevice::OpenArgs & args) const
{
  args.width = GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  args.height = GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);
  unsigned maxRate = GetClockRate()/GetFrameTime();
  if (args.rate > maxRate)
    args.rate = maxRate;
}


OpalVideoFormat::FrameType OpalVideoFormat::GetFrameType(const BYTE * payloadPtr, PINDEX payloadSize, FrameDetectorPtr & detector) const
{
  PWaitAndSignal m(m_mutex);
  if (m_info == NULL)
    return e_UnknownFrameType;

  return dynamic_cast<OpalVideoFormatInternal *>(m_info)->GetFrameType(payloadPtr, payloadSize, detector);
}


OpalVideoFormat::FrameType OpalVideoFormatInternal::GetFrameType(const BYTE * payloadPtr,
                                                                 PINDEX payloadSize,
                                                                 OpalVideoFormat::FrameDetectorPtr & detector) const
{
  if (detector.get() == NULL) {
    detector.reset(OpalVideoFormat::FrameDetectFactory::CreateInstance(rtpEncodingName));
    if (detector.get() == NULL)
      return OpalVideoFormat::e_UnknownFrameType;
  }

  return detector->GetFrameType(payloadPtr, payloadSize);
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


OpalMediaFormatList & OpalMediaFormatList::operator+=(const PString & wildcard)
{
  MakeUnique();

  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  OpalMediaFormatList::const_iterator fmt;
  while ((fmt = registeredFormats.FindFormat(wildcard, fmt)) != registeredFormats.end())
    *this += *fmt;

  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator+=(const OpalMediaFormat & format)
{
  MakeUnique();

  if (!format.IsValid())
    return *this;

  const_iterator it = FindFormat(format);
  if (it == end())
    OpalMediaFormatBaseList::Append(format.Clone());
  else if (format.m_info->m_allowMultiple) {
    OpalMediaFormat * copy = format.CloneAs<OpalMediaFormat>();
    copy->m_info->DeconflictPayloadTypes(*this);
    OpalMediaFormatBaseList::Append(copy);
  }

  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator+=(const OpalMediaFormatList & formats)
{
  MakeUnique();
  for (OpalMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format)
    *this += *format;
  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator-=(const OpalMediaFormat & format)
{
  MakeUnique();
  OpalMediaFormatList::const_iterator fmt = FindFormat(format);
  if (fmt != end())
    erase(fmt);

  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator-=(const OpalMediaFormatList & formats)
{
  MakeUnique();
  for (OpalMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format)
    *this -= *format;
  return *this;
}


void OpalMediaFormatList::Remove(const PStringArray & maskList)
{
  if (maskList.IsEmpty())
    return;

  PTRACE(4, "Removing codecs " << setfill(',') << maskList);

  PINDEX i;
  std::list<PStringArray> notMasks;
  std::list<OpalMediaType> notTypes;

  for (i = 0; i < maskList.GetSize(); i++) {
    PString mask = maskList[i];
    if (mask[0] == '!') {
      if (mask[1] == '@')
        notTypes.push_back(mask.Mid(2));
      else
        notMasks.push_back(mask.Mid(1).Tokenise('*', true));
    }
    else {
      const_iterator fmt;
      while ((fmt = FindFormat(mask)) != end())
        erase(fmt);
    }
  }

  if (notMasks.empty() && notTypes.empty())
      return;

  PStringList formatsToRemove;
  for (iterator fmt = begin(); fmt != end(); ++fmt) {
    PCaselessString name = fmt->GetName();
    bool remove = true;
    for (std::list<PStringArray>::iterator notMask = notMasks.begin(); notMask != notMasks.end(); ++notMask) {
      if (WildcardMatch(name, *notMask)) {
        remove = false;
        break;
      }
    }

    for (std::list<OpalMediaType>::iterator notType = notTypes.begin(); notType != notTypes.end(); ++notType) {
      if (fmt->GetMediaType() == *notType) {
        remove = false;
        break;
      }
    }

    if (remove)
      formatsToRemove += name;
  }

  for (PStringList::iterator it = formatsToRemove.begin(); it != formatsToRemove.end(); ++it) {
    for (iterator fmt = begin(); fmt != end(); ++fmt) {
      if (*it == fmt->GetName()) {
        erase(fmt);
        break;
      }
    }
  }
}


void OpalMediaFormatList::RemoveNonTransportable()
{
  iterator it = begin();
  while (it != end()) {
    if (it->IsTransportable())
      ++it;
    else
      erase(it++);
  }
}


OpalMediaFormatList::const_iterator OpalMediaFormatList::FindFormat(RTP_DataFrame::PayloadTypes pt,
                                                                    const unsigned clockRate,
                                                                    const char * name,
                                                                    const char * protocol,
                                                                    const_iterator format) const
{
  if (format == const_iterator())
    format = begin();
  else
    ++format;

  // First look for a matching encoding name
  if (name != NULL && *name != '\0') {
    OpalMediaFormatList::const_iterator savedIterationStart = format;

    for (; format != end(); ++format) {
      // If encoding name matches exactly, then use it regardless of payload code.
      const char * otherName = format->GetEncodingName();
      if (otherName != NULL && strcasecmp(otherName, name) == 0 &&
          (clockRate == 0    || clockRate == format->GetClockRate()) && // if have clock rate, clock rate must match
          (protocol  == NULL || format->IsValidForProtocol(protocol))) // if protocol is specified, must be valid for the protocol
        return format;
    }

    format = savedIterationStart;
  }

  // Can't match by encoding name, try by known payload type.
  // Note we do two separate loops as it is possible (though discouraged) for
  // someone to override a standard payload type with another encoding name, so
  // have to search all formats by name before trying by number.
  if (pt < RTP_DataFrame::LastKnownPayloadType) {
    for (; format != end(); ++format) {
      if (format->GetPayloadType() == pt &&
          (clockRate == 0    || clockRate == format->GetClockRate()) && // if have clock rate, clock rate must match
          (protocol  == NULL || format->IsValidForProtocol(protocol))) // if protocol is specified, must be valid for the protocol
        return format;
    }
  }

  return end();
}


OpalMediaFormatList::const_iterator OpalMediaFormatList::FindFormat(const PString & search, const_iterator iter) const
{
  if (search.IsEmpty())
    return end();

  if (iter == const_iterator())
    iter = begin();
  else
    ++iter;

  bool negative = search[0] == '!';

  PString adjustedSearch = search.Mid(negative ? 1 : 0);
  if (adjustedSearch.IsEmpty())
    return end();

  if (adjustedSearch[0] == '@') {
    OpalMediaType searchType = adjustedSearch.Mid(1);
    while (iter != end()) {
      if ((iter->GetMediaType() == searchType) != negative)
        return iter;
      ++iter;
    }
  }
  else {
    PStringArray wildcards = adjustedSearch.Tokenise('*', true);
    while (iter != end()) {
      if (WildcardMatch(iter->m_info->formatName, wildcards) != negative)
        return iter;
      ++iter;
    }
  }

  return end();
}


void OpalMediaFormatList::Reorder(const PStringArray & order)
{
  DisallowDeleteObjects();

  OpalMediaFormatBaseList::iterator orderedIter = begin();

  for (PINDEX i = 0; i < order.GetSize(); i++) {
    if (order[i][0] == '@') {
      OpalMediaType mediaType = order[i].Mid(1);

      OpalMediaFormatBaseList::iterator findIter = orderedIter;
      while (findIter != end()) {
        if (findIter->GetMediaType() != mediaType)
          ++findIter;
        else if (findIter == orderedIter) {
          ++orderedIter;
          ++findIter;
        }
        else {
          insert(orderedIter, &*findIter);
          erase(findIter++);
        }
      }
    }
    else {
      PStringArray wildcards = order[i].Tokenise('*', true);

      OpalMediaFormatBaseList::iterator findIter = orderedIter;
      while (findIter != end()) {
        if (!WildcardMatch(findIter->GetName(), wildcards))
          ++findIter;
        else if (findIter == orderedIter) {
          ++orderedIter;
          ++findIter;
        }
        else {
          insert(orderedIter, &*findIter);
          erase(findIter++);
        }
      }
    }
  }

  AllowDeleteObjects();
}


bool OpalMediaFormatList::HasType(const OpalMediaType & type, bool mustBeTransportable) const
{
  for (OpalMediaFormatList::const_iterator format = begin(); format != end(); ++format) {
    if ((!mustBeTransportable || format->IsTransportable()) && format->IsMediaType(type))
        return true;
  }

  return false;
}


static void AddMediaType(OpalMediaTypeList & mediaTypes, const OpalMediaType & mediaType)
{
  if (std::find(mediaTypes.begin(), mediaTypes.end(), mediaType) == mediaTypes.end())
    mediaTypes.push_back(mediaType);
}


OpalMediaTypeList OpalMediaFormatList::GetMediaTypes() const
{
  OpalMediaTypeList mediaTypes;

  for (OpalMediaFormatList::const_iterator format = begin(); format != end(); ++format) {
    AddMediaType(mediaTypes, format->GetMediaType());
#if OPAL_VIDEO
    if (IsPresentationRole(*format))
      AddMediaType(mediaTypes, OpalPresentationVideoMediaDefinition::Name());
#endif
  }

  return mediaTypes;
}


/////////////////////////////////////////////////////////////////////////////

namespace OpalRtx
{
  const PString & AssociatedPayloadTypeOption() { static PConstString s("Associated-Payload-Type"); return s; }
  const PString & RetransmitTimeOption() { static PConstString s("Retransmit-Time"); return s; }
  const PCaselessString & EncodingName() { static PConstCaselessString s("rtx"); return s; }

  class OpalRtxMediaFormat : public OpalMediaFormat
  {
  public:
    OpalRtxMediaFormat(const OpalMediaType & mediaType)
      : OpalMediaFormat(new OpalMediaFormatInternal(OpalRtx::GetName(mediaType),
                                                    mediaType,
                                                    RTP_DataFrame::DynamicBase,
                                                    EncodingName(),
                                                    false, 0, 0, 0,
#if OPAL_VIDEO
                                                    mediaType == OpalMediaType::Video() ? VideoClockRate :
#endif
                                                    AudioClockRate), true)
    {
      OpalMediaOptionUnsigned * opt = new OpalMediaOptionUnsigned(AssociatedPayloadTypeOption(),
                                          true, OpalMediaOption::EqualMerge, 0, 0, RTP_DataFrame::MaxPayloadType);
      OPAL_SET_MEDIA_OPTION_FMTP(opt, "apt", "");
      AddOption(opt);

      opt = new OpalMediaOptionUnsigned(RetransmitTimeOption(), false, OpalMediaOption::NoMerge, 3000, 100);
      OPAL_SET_MEDIA_OPTION_FMTP(opt, "rtx-time", "");
      AddOption(opt);
    }
  };

  PString GetName(const OpalMediaType & mediaType)
  {
    return "rtx-" + mediaType;
  }

  static PMutex s_rtxMutex(P_DEBUG_LOCATION);

  OpalMediaFormat GetMediaFormat(const OpalMediaType & mediaType)
  {
    PString name = GetName(mediaType);
    OpalMediaFormat fmt(name);
    if (!fmt.IsValid()) {
      PWaitAndSignal lock(s_rtxMutex);
      fmt = name; // Check again after mutex to avoid race condition
      if (!fmt.IsValid()) {
        new OpalRtxMediaFormat(mediaType); // Will be deleted (indirectly) in ~OpalManager
        fmt = name;
      }
    }

    return fmt;
  }
};


// End of File ///////////////////////////////////////////////////////////////
