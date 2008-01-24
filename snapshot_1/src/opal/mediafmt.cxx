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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediafmt.h"
#pragma implementation "mediacmd.h"
#endif

#include <opal/mediafmt.h>
#include <opal/mediacmd.h>
#include <codec/opalplugin.h>
#include <codec/opalwavfile.h>
#include <ptlib/videoio.h>
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
    true,   // Needs jitter
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
    true,   // Needs jitter
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


static void Clamp(OpalMediaFormatInternal & fmt1, const OpalMediaFormatInternal & fmt2, const PString & variableOption, const PString & minOption, const PString & maxOption)
{
  unsigned value    = fmt1.GetOptionInteger(variableOption, 0);

  unsigned minValue = fmt2.GetOptionInteger(minOption, 0);
  unsigned maxValue = fmt2.GetOptionInteger(maxOption, UINT_MAX);
  if (value < minValue)
    fmt1.SetOptionInteger(variableOption, minValue);
  else if (value > maxValue)
    fmt1.SetOptionInteger(variableOption, maxValue);
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
      return CompareValue(option) == EqualTo;

    case NotEqualMerge :
      return CompareValue(option) != EqualTo;
      
    case AlwaysMerge :
      assign = CompareValue(option) != EqualTo;
      break;

    default :
      assign = false;
      break;
  }

  if (assign) {
    PTRACE(4, "MediaFormat\tChanged media option \"" << m_name << "\" from " << *this << " to " << option);
    Assign(option);
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

const PString & OpalMediaFormat::NeedsJitterOption()  { static PString s = PLUGINCODEC_OPTION_NEEDS_JITTER;   return s; }
const PString & OpalMediaFormat::MaxFrameSizeOption() { static PString s = PLUGINCODEC_OPTION_MAX_FRAME_SIZE; return s; }
const PString & OpalMediaFormat::FrameTimeOption()    { static PString s = PLUGINCODEC_OPTION_FRAME_TIME;     return s; }
const PString & OpalMediaFormat::ClockRateOption()    { static PString s = PLUGINCODEC_OPTION_CLOCK_RATE;     return s; }
const PString & OpalMediaFormat::MaxBitRateOption()   { static PString s = PLUGINCODEC_OPTION_MAX_BIT_RATE;   return s; }
const PString & OpalMediaFormat::TargetBitRateOption(){ static PString s = PLUGINCODEC_OPTION_TARGET_BIT_RATE; return s; }


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
                                 PBoolean     nj,
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
    strm << *m_info;
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
                                                 PBoolean     nj,
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


PObject * OpalMediaFormatInternal::Clone() const
{
  return new OpalMediaFormatInternal(*this);
}


bool OpalMediaFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PTRACE(4, "MediaFormat\tMerging " << mediaFormat << " into " << *this);

  PWaitAndSignal m1(media_format_mutex);
  PWaitAndSignal m2(mediaFormat.media_format_mutex);

  ToNormalisedOptions();

  for (PINDEX i = 0; i < options.GetSize(); i++) {
    OpalMediaOption * option = mediaFormat.FindOption(options[i].GetName());
    if (option == NULL) {
      PTRACE_IF(2, formatName == mediaFormat.formatName, "MediaFormat\tCannot merge unmatched option " << options[i].GetName());
    }
    else if (!options[i].Merge(*option))
      return false;
  }

  ToNormalisedOptions();

  return true;
}


PStringToString OpalMediaFormatInternal::GetOptions() const
{
  PStringToString dict;
  for (PINDEX i = 0; i < options.GetSize(); i++)
    dict.SetAt(options[i].GetName(), options[i].AsString());
  return dict;
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


bool OpalMediaFormatInternal::ToNormalisedOptions()
{
  return true;
}


bool OpalMediaFormatInternal::ToCustomisedOptions()
{
  return true;
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


bool OpalMediaFormatInternal::AddOption(OpalMediaOption * option, PBoolean overwrite)
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

  return true;
}


void OpalMediaFormatInternal::PrintOn(ostream & strm) const
{
  PWaitAndSignal m(media_format_mutex);

  if (strm.width() != -1) {
    strm << formatName;
    return;
  }

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

#if OPAL_SIP
    if (!option.GetFMTPName().IsEmpty())
      strm << "  FMTP name: " << option.GetFMTPName() << " (" << option.GetFMTPDefault() << ')';
#endif // OPAL_SIP

#if OPAL_H323
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
#endif // OPAL_H323
    strm << '\n';
  }
  strm << endl;
}

///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

const PString & OpalAudioFormat::RxFramesPerPacketOption() { static PString s = PLUGINCODEC_OPTION_RX_FRAMES_PER_PACKET; return s; }
const PString & OpalAudioFormat::TxFramesPerPacketOption() { static PString s = PLUGINCODEC_OPTION_TX_FRAMES_PER_PACKET; return s; }

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
                            true,
                            8*frameSize*OpalAudioFormat::AudioClockRate/frameTime,  // bits per second = 8*frameSize * framesPerSecond
                            frameSize,
                            frameTime,
                            clockRate,
                            timeStamp)
{
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::RxFramesPerPacketOption(), false, OpalMediaOption::NoMerge,  rxFrames, 1, maxFrames));
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

  Clamp(*this, mediaFormat, OpalAudioFormat::TxFramesPerPacketOption(), PString::Empty(), OpalAudioFormat::RxFramesPerPacketOption());
  return true;
}

#endif

///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

const PString & OpalVideoFormat::FrameWidthOption()             { static PString s = PLUGINCODEC_OPTION_FRAME_WIDTH;               return s; }
const PString & OpalVideoFormat::FrameHeightOption()            { static PString s = PLUGINCODEC_OPTION_FRAME_HEIGHT;              return s; }
const PString & OpalVideoFormat::MinRxFrameWidthOption()        { static PString s = PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH;        return s; }
const PString & OpalVideoFormat::MinRxFrameHeightOption()       { static PString s = PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT;       return s; }
const PString & OpalVideoFormat::MaxRxFrameWidthOption()        { static PString s = PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH;        return s; }
const PString & OpalVideoFormat::MaxRxFrameHeightOption()       { static PString s = PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT;       return s; }
const PString & OpalVideoFormat::TemporalSpatialTradeOffOption(){ static PString s = PLUGINCODEC_OPTION_TEMPORAL_SPATIAL_TRADE_OFF;return s; }
const PString & OpalVideoFormat::TxKeyFramePeriodOption()       { static PString s = PLUGINCODEC_OPTION_TX_KEY_FRAME_PERIOD;       return s; }


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
                            PFalse,
                            bitRate,
                            0,
                            OpalMediaFormat::VideoClockRate/frameRate,
                            OpalMediaFormat::VideoClockRate,
                            timeStamp)
{
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::FrameWidthOption(),         false, OpalMediaOption::AlwaysMerge, frameWidth,  16, 32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::FrameHeightOption(),        false, OpalMediaOption::AlwaysMerge, frameHeight, 16, 32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MinRxFrameWidthOption(),    false, OpalMediaOption::MinMerge, PVideoFrameInfo::SQCIFWidth, 16, 32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MinRxFrameHeightOption(),   false, OpalMediaOption::MinMerge, PVideoFrameInfo::SQCIFHeight,16, 32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MaxRxFrameWidthOption(),    false, OpalMediaOption::MinMerge, PVideoFrameInfo::CIF16Width, 16, 32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MaxRxFrameHeightOption(),   false, OpalMediaOption::MinMerge, PVideoFrameInfo::CIF16Height,16, 32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::TargetBitRateOption(),      false, OpalMediaOption::MinMerge, 10000000, 100));

  // For video the max bit rate and frame rate is adjustable by user
  FindOption(OpalVideoFormat::MaxBitRateOption())->SetReadOnly(false);
  FindOption(OpalVideoFormat::FrameTimeOption())->SetReadOnly(false);
  FindOption(OpalVideoFormat::FrameTimeOption())->SetMerge(OpalMediaOption::MaxMerge);
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

  Clamp(*this, mediaFormat, OpalVideoFormat::TargetBitRateOption(), PString::Empty(),                          OpalMediaFormat::MaxBitRateOption());
  Clamp(*this, mediaFormat, OpalVideoFormat::FrameWidthOption(),    OpalVideoFormat::MinRxFrameWidthOption(),  OpalVideoFormat::MaxRxFrameWidthOption());
  Clamp(*this, mediaFormat, OpalVideoFormat::FrameHeightOption(),   OpalVideoFormat::MinRxFrameHeightOption(), OpalVideoFormat::MaxRxFrameHeightOption());

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
    if (i == 0 && next != 0 && !wildcard)
      return false;

    last = next + wildcard.GetLength();

    // Check for having * at end of search string
    if (i == wildcards.GetSize()-1 && !wildcard && last != str.GetLength())
      return false;
  }

  return true;
}


PINDEX OpalMediaFormatList::FindFormat(const PString & search, PINDEX pos) const
{
  PStringArray wildcards = search.Tokenise('*', true);
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
    PStringArray wildcards = order[i].Tokenise('*', true);

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
