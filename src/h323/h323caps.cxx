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
 */

#include <ptlib.h>

#include <opal_config.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h323caps.h"
#endif

#include <h323/h323caps.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/h323pdu.h>
#include <h323/transaddr.h>
#include <t38/h323t38.h>
#include <codec/opalplugin.h>
#include <codec/rfc2833.h>
#include <asn/h235_srtp.h>
#include <h224/h323h224.h>

#include <algorithm>


#define DEFINE_G711_CAPABILITY(cls, code, capName) \
class cls : public H323_G711Capability { \
  public: \
    cls() : H323_G711Capability(code) { } \
}; \
H323_REGISTER_CAPABILITY(cls, capName) \


#ifndef NO_H323_AUDIO_CODECS
  DEFINE_G711_CAPABILITY(H323_G711ALaw64Capability, H323_G711Capability::ALaw, OPAL_G711_ALAW_64K)
  DEFINE_G711_CAPABILITY(H323_G711uLaw64Capability, H323_G711Capability::muLaw, OPAL_G711_ULAW_64K)
#endif


#if OPAL_T38_CAPABILITY
  H323_REGISTER_CAPABILITY(H323_T38Capability, OPAL_T38);
#endif

#if OPAL_HAS_H281
  H323_REGISTER_CAPABILITY(H323_FECC_RTP_Capability, OPAL_FECC_RTP);
  H323_REGISTER_CAPABILITY(H323_FECC_HDLC_Capability, OPAL_FECC_HDLC);
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


#if OPAL_H235_6 || OPAL_H235_8

template <unsigned dt, unsigned mt, class DATA_TYPE, class CAP_TYPE>
bool H323GetMediaCapability(DATA_TYPE & dataType, CAP_TYPE * & cap)
{
  switch (dataType.GetTag()) {
    case dt :
      cap = &(CAP_TYPE &)dataType;
      return true;

    case H245_DataType::e_h235Media :
      const H245_H235Media & h235 = dataType;
      if (h235.m_mediaType.GetTag() == mt) {
        cap = &(CAP_TYPE &)h235.m_mediaType;
        return true;
      }
  }

  return false;
}


template <unsigned dt, unsigned mt, class DATA_TYPE, class CAP_TYPE>
void H323SetMediaCapability(const H323Capability & rtCap, DATA_TYPE & dataType, CAP_TYPE * & cap)
{
  if (rtCap.GetCryptoSuite() == NULL) {
    dataType.SetTag(dt);
    cap = &(CAP_TYPE &)dataType;
  }
  else {
    dataType.SetTag(H245_DataType::e_h235Media);
    H245_H235Media & h235 = dataType;
    h235.m_mediaType.SetTag(mt);
    cap = &(CAP_TYPE &)h235.m_mediaType;
  }
}

#else // OPAL_H235_6 || OPAL_H235_8

template <unsigned dt, unsigned mt, class DATA_TYPE, class CAP_TYPE>
bool H323GetMediaCapability(DATA_TYPE & dataType, CAP_TYPE * & cap)
{
  if (dataType.GetTag() != dt)
    return false;

  cap = &(CAP_TYPE &)dataType;
  return true;
}

template <unsigned dt, unsigned mt, class DATA_TYPE, class CAP_TYPE>
void H323SetMediaCapability(const H323Capability &, DATA_TYPE & dataType, CAP_TYPE * & cap)
{
  dataType.SetTag(dt);
  cap = &(CAP_TYPE &)dataType;
}

#endif // OPAL_H235_6 || OPAL_H235_8


/////////////////////////////////////////////////////////////////////////////

H323Capability::H323Capability()
  : assignedCapabilityNumber(0) // Unassigned
  , capabilityDirection(e_Unknown)
#if OPAL_H235_6 || OPAL_H235_8
  , m_cryptoCapability(NULL)
#endif
{
}


H323Capability::H323Capability(const H323Capability & other)
  : PObject(other)
  , assignedCapabilityNumber(other.assignedCapabilityNumber)
  , capabilityDirection(other.capabilityDirection)
  , m_mediaFormat(other.m_mediaFormat)
#if OPAL_H235_6 || OPAL_H235_8
  , m_cryptoCapability(other.m_cryptoCapability != NULL ? other.m_cryptoCapability->CloneAs<H235SecurityCapability>() : NULL)
#endif
{
}


H323Capability & H323Capability::operator=(const H323Capability & other)
{
  assignedCapabilityNumber = other.assignedCapabilityNumber;
  capabilityDirection = other.capabilityDirection;
  m_mediaFormat = other.m_mediaFormat;
#if OPAL_H235_6 || OPAL_H235_8
  delete m_cryptoCapability;
  m_cryptoCapability = other.m_cryptoCapability != NULL ? other.m_cryptoCapability->CloneAs<H235SecurityCapability>() : NULL;
#endif
  return *this;
}


H323Capability::~H323Capability()
{
#if OPAL_H235_6 || OPAL_H235_8
  delete m_cryptoCapability;
#endif
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

  return GetMediaFormat().ValidateMerge(other.GetMediaFormat()) ? EqualTo : GreaterThan;
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


PBoolean H323Capability::IsMatch(const PASN_Object & subTypePDU, const PString & mediaPacketization) const
{
  if (subTypePDU.GetTag() != GetSubType())
    return false;

  if (mediaPacketization.IsEmpty())
    return true;

  PStringSet mediaPacketizations = GetMediaFormat().GetMediaPacketizationSet();
  if (mediaPacketizations.IsEmpty())
    return true;

  return mediaPacketizations.Contains(mediaPacketization);
}


H323Channel * H323Capability::CreateChannel(H323Connection &,
                                            H323Channel::Directions,
                                            unsigned,
                                            const H245_H2250LogicalChannelParameters *) const
{
  PAssertAlways(PUnimplementedFunction);
  return NULL;
}


#if OPAL_H235_6 || OPAL_H235_8
PBoolean H323Capability::OnSendingPDU(H245_DataType & pdu) const
{
  if (m_cryptoCapability != NULL) {
    H245_H235Media & h235 = pdu;
    if (!m_cryptoCapability->OnSendingPDU(h235.m_encryptionAuthenticationAndIntegrity))
      return false;
  }
#else
PBoolean H323Capability::OnSendingPDU(H245_DataType & /*pdu*/) const
{
#endif

  GetWritableMediaFormat().SetOptionString(OpalMediaFormat::ProtocolOption(), PLUGINCODEC_OPTION_PROTOCOL_H323);
  return m_mediaFormat.ToCustomisedOptions();
}


PBoolean H323Capability::OnSendingPDU(H245_ModeElement &) const
{
  PAssertAlways(PUnimplementedFunction);
  return false;
}


#if OPAL_H235_6 || OPAL_H235_8
const OpalMediaCryptoSuite * H323Capability::GetCryptoSuite() const
{
  return m_cryptoCapability != NULL ? &m_cryptoCapability->GetCryptoSuites().front() : NULL;
}


void H323Capability::SetCryptoSuite(const OpalMediaCryptoSuite & cryptoSuite)
{
  delete m_cryptoCapability;
  m_cryptoCapability = cryptoSuite.CreateCapability(*this);
  PAssertNULL(m_cryptoCapability);

  OpalMediaCryptoSuite::List cryptoSuites;
  cryptoSuites.Append(const_cast<OpalMediaCryptoSuite *>(&cryptoSuite));
  m_cryptoCapability->SetCryptoSuites(cryptoSuites);
}


bool H323Capability::OnSendingPDU(H245_EncryptionSync & encryptionSync,
                                  const H323Connection & connection,
                                  unsigned sessionID,
                                  bool rx)
{
  return m_cryptoCapability != NULL && m_cryptoCapability->OnSendingPDU(encryptionSync, connection, sessionID, rx);
}


bool H323Capability::OnReceivedPDU(const H245_EncryptionSync & encryptionSync,
                                   const H323Connection & connection,
                                   unsigned sessionID,
                                   bool rx)
{
  return m_cryptoCapability != NULL && m_cryptoCapability->OnReceivedPDU(encryptionSync, connection, sessionID, rx);
}


bool H323Capability::PostTCS(const H323Connection &, const H323Capabilities &)
{
  return true;
}
#endif // OPAL_H235_6 || OPAL_H235_8


PBoolean H323Capability::OnReceivedPDU(const H245_Capability & cap)
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

  GetWritableMediaFormat().SetOptionString(OpalMediaFormat::ProtocolOption(), PLUGINCODEC_OPTION_PROTOCOL_H323);
  return m_mediaFormat.ToNormalisedOptions();
}


#if OPAL_H235_6 || OPAL_H235_8
PBoolean H323Capability::OnReceivedPDU(const H245_DataType & pdu, PBoolean /*receiver*/)
{
  if (pdu.GetTag() == H245_DataType::e_h235Media && m_cryptoCapability != NULL) {
    const H245_H235Media & h235 = pdu;
    if (!m_cryptoCapability->OnReceivedPDU(h235.m_encryptionAuthenticationAndIntegrity))
      return false;
  }
#else
  PBoolean H323Capability::OnReceivedPDU(const H245_DataType & /*pdu*/, PBoolean /*receiver*/)
  {
#endif // OPAL_H235_6 || OPAL_H235_8

  GetWritableMediaFormat().SetOptionString(OpalMediaFormat::ProtocolOption(), PLUGINCODEC_OPTION_PROTOCOL_H323);
  return m_mediaFormat.ToNormalisedOptions();
}


PBoolean H323Capability::IsUsable(const H323Connection &) const
{
  return true;
}


OpalMediaFormat H323Capability::GetMediaFormat() const
{
  if (m_mediaFormat.IsValid())
    return m_mediaFormat;

#if OPAL_H239
  PString name = GetFormatName();
  OpalMediaFormat mediaFormat(name);
  if (!mediaFormat.IsValid()) {
    PINDEX plus = name.Find('+');
    mediaFormat = name.Left(plus);
    if (!mediaFormat.IsValid())
      mediaFormat = name.Mid(plus+1);
  }
  return mediaFormat;
#else
  return OpalMediaFormat(GetFormatName());
#endif
}


bool H323Capability::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  return GetWritableMediaFormat().Update(mediaFormat);
}


OpalMediaFormat & H323Capability::GetWritableMediaFormat() const
{
  if (!m_mediaFormat.IsValid())
    m_mediaFormat = GetMediaFormat();
  return m_mediaFormat;
}


/////////////////////////////////////////////////////////////////////////////

H323RealTimeCapability::H323RealTimeCapability()
{
}


H323RealTimeCapability::H323RealTimeCapability(const H323RealTimeCapability & rtc)
  : H323Capability(rtc)
{
}


H323RealTimeCapability::~H323RealTimeCapability()
{
}


H323Channel * H323RealTimeCapability::CreateChannel(H323Connection & connection,
                                                    H323Channel::Directions dir,
                                                    unsigned sessionID,
                                 const H245_H2250LogicalChannelParameters * param) const
{
  return connection.CreateRealTimeLogicalChannel(*this, dir, sessionID, param);
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


PBoolean H323NonStandardCapabilityInfo::OnSendingPDU(PBYTEArray & data) const
{
  data = nonStandardData;
  return data.GetSize() > 0;
}


PBoolean H323NonStandardCapabilityInfo::OnReceivedPDU(const PBYTEArray & data)
{
  if (CompareData(data) != PObject::EqualTo)
    return false;

  nonStandardData = data;
  return true;
}


PBoolean H323NonStandardCapabilityInfo::OnSendingNonStandardPDU(PASN_Choice & pdu,
                                                            unsigned nonStandardTag) const
{
  PBYTEArray data;
  if (!OnSendingPDU(data))
    return false;

  pdu.SetTag(nonStandardTag);
  H245_NonStandardParameter & param = (H245_NonStandardParameter &)pdu.GetObject();

  if (!oid.IsEmpty()) {
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


PBoolean H323NonStandardCapabilityInfo::OnReceivedNonStandardPDU(const PASN_Choice & pdu,
                                                             unsigned nonStandardTag)
{
  if (pdu.GetTag() != nonStandardTag)
    return false;

  const H245_NonStandardParameter & param = (const H245_NonStandardParameter &)pdu.GetObject();

  if (CompareParam(param) != PObject::EqualTo)
    return false;

  return OnReceivedPDU(param.m_data);
}


PBoolean H323NonStandardCapabilityInfo::IsMatch(const H245_NonStandardParameter & param) const
{
  return CompareParam(param) == PObject::EqualTo && CompareData(param.m_data) == PObject::EqualTo;
}


PObject::Comparison H323NonStandardCapabilityInfo::CompareParam(const H245_NonStandardParameter & param) const
{
  if (!oid.IsEmpty()) {
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

H323GenericCapabilityInfo::H323GenericCapabilityInfo(const PString & standardId, unsigned bitRate, bool fixed)
  : m_identifier(standardId)
  , m_maxBitRate(bitRate)
  , m_bitRateMode(fixed ? (standardId == OpalPluginCodec_Identifer_G7221 ? e_FixedBitRateG7221 : e_FixedBitRateStandard) : e_VariableBitRate)
{
}


struct OpalMediaOptionSortByPosition
{
  bool operator()(OpalMediaOption const * const & o1, OpalMediaOption const * const & o2)
  {
    return o1->GetH245Generic().position < o2->GetH245Generic().position;
  }
};

PBoolean H323GenericCapabilityInfo::OnSendingGenericPDU(H245_GenericCapability & pdu,
                                                    const OpalMediaFormat & mediaFormat,
                                                    H323Capability::CommandType type) const
{
  H323SetCapabilityIdentifier(m_identifier, pdu.m_capabilityIdentifier);

  switch (m_bitRateMode) {
    case e_FixedBitRateG7221:
      pdu.m_maxBitRate = m_maxBitRate;
      break;

    case e_FixedBitRateStandard:
      m_maxBitRate.SetH245(pdu.m_maxBitRate);
      break;

    default:
      if (type == H323Capability::e_TCS)
        mediaFormat.GetMaxBandwidth().SetH245(pdu.m_maxBitRate);
      else
        mediaFormat.GetUsedBandwidth().SetH245(pdu.m_maxBitRate);
  }

  if (pdu.m_maxBitRate != 0)
    pdu.IncludeOptionalField(H245_GenericCapability::e_maxBitRate);

  std::vector<OpalMediaOption const *> reorderedOptions;
  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    const OpalMediaOption::H245GenericInfo & genericInfo = option.GetH245Generic();
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

    if (option.AsString() != genericInfo.defaultValue)
      reorderedOptions.push_back(&option);
  }

  std::sort(reorderedOptions.begin(), reorderedOptions.end(), OpalMediaOptionSortByPosition());

  for (std::vector<OpalMediaOption const *>::iterator it = reorderedOptions.begin(); it != reorderedOptions.end(); ++it) {
    const OpalMediaOption & option = **it;
    const OpalMediaOption::H245GenericInfo & genericInfo = option.GetH245Generic();

    H245_ArrayOf_GenericParameter & params =
            genericInfo.mode == OpalMediaOption::H245GenericInfo::Collapsing ? pdu.m_collapsing : pdu.m_nonCollapsing;

    if (PIsDescendant(&option, OpalMediaOptionBoolean))
      H323AddGenericParameterBoolean(params, genericInfo.ordinal, ((const OpalMediaOptionBoolean &)option).GetValue());
    else if (PIsDescendant(&option, OpalMediaOptionUnsigned) || PIsDescendant(&option, OpalMediaOptionInteger)) {
      H245_ParameterValue::Choices tag;
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

      H323AddGenericParameterInteger(params, genericInfo.ordinal, ((const OpalMediaOptionUnsigned &)option).GetValue(), tag);
    }
    else if (PIsDescendant(&option, OpalMediaOptionOctets))
      H323AddGenericParameterOctets(params, genericInfo.ordinal, ((const OpalMediaOptionOctets &)option).GetValue());
    else
      H323AddGenericParameterString(params, genericInfo.ordinal, option.AsString());
  }

  if (pdu.m_collapsing.GetSize() > 0)
    pdu.IncludeOptionalField(H245_GenericCapability::e_collapsing);

  if (pdu.m_nonCollapsing.GetSize() > 0)
    pdu.IncludeOptionalField(H245_GenericCapability::e_nonCollapsing);

  return true;
}

static void H323GenericCapabilityInfo_OnReceivedGenericPDU(OpalMediaFormat & mediaFormat,
                                                           const H245_GenericCapability & pdu,
                                                           H323Capability::CommandType type)
{
  mediaFormat.MakeUnique();

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

    const H245_ParameterValue * param;
    if (genericInfo.mode == OpalMediaOption::H245GenericInfo::Collapsing) {
      if (!pdu.HasOptionalField(H245_GenericCapability::e_collapsing))
        continue;
      param = H323GetGenericParameter(pdu.m_collapsing, genericInfo.ordinal);
    }
    else {
      if (!pdu.HasOptionalField(H245_GenericCapability::e_nonCollapsing))
        continue;
      param = H323GetGenericParameter(pdu.m_nonCollapsing, genericInfo.ordinal);
    }

    if (PIsDescendant(&option, OpalMediaOptionBoolean))
      ((OpalMediaOptionBoolean &)option).SetValue(false);

    if (param == NULL)
      continue;

    if (PIsDescendant(&option, OpalMediaOptionBoolean)) {
      if (param->GetTag() == H245_ParameterValue::e_logical) {
        ((OpalMediaOptionBoolean &)option).SetValue(true);
        continue;
      }
    }
    else if (PIsDescendant(&option, OpalMediaOptionUnsigned) || PIsDescendant(&option, OpalMediaOptionInteger)) {
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

      if (param->GetTag() == tag) {
        ((OpalMediaOptionUnsigned &)option).SetValue((const PASN_Integer &)*param);
        continue;
      }
    }
    else {
      if (param->GetTag() == H245_ParameterValue::e_octetString) {
        const PASN_OctetString & octetString = *param;
        if (PIsDescendant(&option, OpalMediaOptionOctets))
          ((OpalMediaOptionOctets &)option).SetValue(octetString);
        else
          ((OpalMediaOption &)option).FromString(octetString.AsString());
        continue;
      }
    }

    PTRACE(2, "H323\tInvalid generic parameter type (" << param->GetTagName()
           << ") for option \"" << option.GetName() << "\" (" << option.GetClass() << ')');
  }
}


PBoolean H323GenericCapabilityInfo::OnReceivedGenericPDU(OpalMediaFormat & mediaFormat,
                                                     const H245_GenericCapability & pdu,
                                                     H323Capability::CommandType type)
{
  if (H323GetCapabilityIdentifier(pdu.m_capabilityIdentifier) != m_identifier)
    return false;

  if (m_bitRateMode == e_VariableBitRate && pdu.HasOptionalField(H245_GenericCapability::e_maxBitRate)) {
    m_maxBitRate.FromH245(pdu.m_maxBitRate);
    mediaFormat.SetOptionInteger(OpalMediaFormat::MaxBitRateOption(), m_maxBitRate);
  }

  H323GenericCapabilityInfo_OnReceivedGenericPDU(mediaFormat, pdu, type);
  return true;
}


PBoolean H323GenericCapabilityInfo::IsMatch(const OpalMediaFormat & mediaFormat, const H245_GenericCapability & param) const
{
  if (H323GetCapabilityIdentifier(param.m_capabilityIdentifier) != m_identifier)
    return false;

  OpalMediaFormat testFormat = mediaFormat;
  H323GenericCapabilityInfo_OnReceivedGenericPDU(testFormat, param, H323Capability::e_TCS);

  return mediaFormat.ValidateMerge(testFormat);
}


PObject::Comparison H323GenericCapabilityInfo::CompareInfo(const H323GenericCapabilityInfo & obj) const
{
  return m_identifier.Compare(obj.m_identifier);
}


/////////////////////////////////////////////////////////////////////////////

H323AudioCapability::H323AudioCapability()
{
}


H323Capability::MainTypes H323AudioCapability::GetMainType() const
{
  return e_Audio;
}


unsigned H323AudioCapability::GetDefaultSessionID() const
{
  return DefaultAudioSessionID;
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


PBoolean H323AudioCapability::OnSendingPDU(H245_Capability & cap) const
{
  static unsigned const tags[NumCapabilityDirections] = {
    H245_Capability::e_receiveAndTransmitAudioCapability,
    H245_Capability::e_receiveAudioCapability,
    H245_Capability::e_transmitAudioCapability,
    H245_Capability::e_receiveAndTransmitAudioCapability
  };
  cap.SetTag(tags[capabilityDirection]);
  return OnSendingPDU((H245_AudioCapability &)cap, GetRxFramesInPacket(), e_TCS);
}


PBoolean H323AudioCapability::OnSendingPDU(H245_DataType & dataType) const
{
  H245_AudioCapability * cap;
  H323SetMediaCapability<H245_DataType::e_audioData, H245_H235Media_mediaType::e_audioData>(*this, dataType, cap);
  return H323RealTimeCapability::OnSendingPDU(dataType) && OnSendingPDU(*cap, GetTxFramesInPacket(), e_OLC);
}


PBoolean H323AudioCapability::OnSendingPDU(H245_ModeElement & mode) const
{
  mode.m_type.SetTag(H245_ModeElementType::e_audioMode);
  return OnSendingPDU((H245_AudioMode &)mode.m_type);
}


PBoolean H323AudioCapability::OnSendingPDU(H245_AudioCapability & pdu,
                                       unsigned packetSize) const
{
  pdu.SetTag(GetSubType());

  // Set the maximum number of frames
  PASN_Integer & value = pdu;
  value = packetSize;
  return true;
}


PBoolean H323AudioCapability::OnSendingPDU(H245_AudioCapability & pdu,
                                       unsigned packetSize,
                                       CommandType) const
{
  return OnSendingPDU(pdu, packetSize);
}


PBoolean H323AudioCapability::OnSendingPDU(H245_AudioMode & pdu) const
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
    return false;

  pdu.SetTag(AudioTable[subType]);
  return true;
}


PBoolean H323AudioCapability::OnReceivedPDU(const H245_Capability & cap)
{
  if (cap.GetTag() != H245_Capability::e_receiveAudioCapability &&
      cap.GetTag() != H245_Capability::e_receiveAndTransmitAudioCapability)
    return false;

  unsigned txFramesInPacket = GetTxFramesInPacket();
  unsigned packetSize = GetRxFramesInPacket();
  if (!OnReceivedPDU((const H245_AudioCapability &)cap, packetSize, e_TCS))
    return false;

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

  return H323Capability::OnReceivedPDU(cap);
}


PBoolean H323AudioCapability::OnReceivedPDU(const H245_DataType & dataType, PBoolean receiver)
{
  const H245_AudioCapability * cap;
  if (!H323GetMediaCapability<H245_DataType::e_audioData, H245_H235Media_mediaType::e_audioData>(dataType, cap))
    return false;

  unsigned xFramesInPacket = receiver ? GetRxFramesInPacket() : GetTxFramesInPacket();
  unsigned packetSize = xFramesInPacket;
  if (!OnReceivedPDU(*cap, packetSize, e_OLC))
    return false;

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

  return H323RealTimeCapability::OnReceivedPDU(dataType, receiver);
}


PBoolean H323AudioCapability::OnReceivedPDU(const H245_AudioCapability & pdu,
                                        unsigned & packetSize)
{
  if (pdu.GetTag() != GetSubType())
    return false;

  const PASN_Integer & value = pdu;

  // Get the maximum number of frames
  packetSize = value;
  return true;
}


PBoolean H323AudioCapability::OnReceivedPDU(const H245_AudioCapability & pdu,
                                        unsigned & packetSize,
                                        CommandType)
{
  return OnReceivedPDU(pdu, packetSize);
}


/////////////////////////////////////////////////////////////////////////////

H323GenericAudioCapability::H323GenericAudioCapability(const PString &standardId, unsigned fixedBitRate)
  : H323AudioCapability()
  , H323GenericCapabilityInfo(standardId, fixedBitRate, fixedBitRate != 0)
{
}

PObject::Comparison H323GenericAudioCapability::Compare(const PObject & obj) const
{
  Comparison result = H323AudioCapability::Compare(obj);
  if (result != EqualTo)
    return result;

  const H323GenericAudioCapability & other = dynamic_cast<const H323GenericAudioCapability &>(obj);
  if (m_bitRateMode != e_VariableBitRate && m_maxBitRate != other.m_maxBitRate)
    return m_maxBitRate < other.m_maxBitRate ? PObject::LessThan : PObject::GreaterThan;

  return CompareInfo(other);
}


unsigned H323GenericAudioCapability::GetSubType() const
{
  return H245_AudioCapability::e_genericAudioCapability;
}


PBoolean H323GenericAudioCapability::OnSendingPDU(H245_AudioCapability & pdu, unsigned, CommandType type) const
{
  pdu.SetTag(H245_AudioCapability::e_genericAudioCapability);
  return OnSendingGenericPDU(pdu, GetMediaFormat(), type);
}

PBoolean H323GenericAudioCapability::OnSendingPDU(H245_AudioMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_genericVideoMode);
  return OnSendingGenericPDU(pdu, GetMediaFormat(), e_ReqMode);
}

PBoolean H323GenericAudioCapability::OnReceivedPDU(const H245_AudioCapability & pdu, unsigned & packetSize, CommandType type)
{
  if( pdu.GetTag() != H245_AudioCapability::e_genericAudioCapability)
    return false;
  if (!OnReceivedGenericPDU(GetWritableMediaFormat(), pdu, type))
    return false;

  packetSize = GetRxFramesInPacket();
  return true;
}

PBoolean H323GenericAudioCapability::IsMatch(const PASN_Object & subTypePDU, const PString & mediaPacketization) const
{
  if (!H323Capability::IsMatch(subTypePDU, mediaPacketization))
    return false;

  const H245_GenericCapability & genericCap = (const H245_GenericCapability &)dynamic_cast<const H245_AudioCapability &>(subTypePDU);
  if (!H323GenericCapabilityInfo::IsMatch(GetMediaFormat(), genericCap))
    return false;

  switch (m_bitRateMode) {
    case e_FixedBitRateG7221:
      if (m_maxBitRate == genericCap.m_maxBitRate.GetValue())
        return true;

    case e_FixedBitRateStandard:
      return m_maxBitRate == genericCap.m_maxBitRate.GetValue()*100;

    default:
      return true;
  }
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


PBoolean H323NonStandardAudioCapability::OnSendingPDU(H245_AudioCapability & pdu,
                                                  unsigned) const
{
  return OnSendingNonStandardPDU(pdu, H245_AudioCapability::e_nonStandard);
}


PBoolean H323NonStandardAudioCapability::OnSendingPDU(H245_AudioMode & pdu) const
{
  return OnSendingNonStandardPDU(pdu, H245_AudioMode::e_nonStandard);
}


PBoolean H323NonStandardAudioCapability::OnReceivedPDU(const H245_AudioCapability & pdu,
                                                   unsigned &)
{
  return OnReceivedNonStandardPDU(pdu, H245_AudioCapability::e_nonStandard);
}


PBoolean H323NonStandardAudioCapability::IsMatch(const PASN_Object & subTypePDU, const PString & mediaPacketization) const
{
  return H323Capability::IsMatch(subTypePDU, mediaPacketization) &&
         H323NonStandardCapabilityInfo::IsMatch((const H245_NonStandardParameter &)dynamic_cast<const H245_AudioCapability &>(subTypePDU));
}


/////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

H323Capability::MainTypes H323VideoCapability::GetMainType() const
{
  return e_Video;
}


PBoolean H323VideoCapability::OnSendingPDU(H245_Capability & cap) const
{
  static unsigned const tags[NumCapabilityDirections] = {
    H245_Capability::e_receiveAndTransmitVideoCapability,
    H245_Capability::e_receiveVideoCapability,
    H245_Capability::e_transmitVideoCapability,
    H245_Capability::e_receiveAndTransmitVideoCapability
  };
  cap.SetTag(tags[capabilityDirection]);
  return OnSendingPDU((H245_VideoCapability &)cap, e_TCS);
}


PBoolean H323VideoCapability::OnSendingPDU(H245_DataType & dataType) const
{
  H245_VideoCapability * cap;
  H323SetMediaCapability<H245_DataType::e_videoData, H245_H235Media_mediaType::e_videoData>(*this, dataType, cap);
  return H323RealTimeCapability::OnSendingPDU(dataType) && OnSendingPDU(*cap, e_OLC);
}


PBoolean H323VideoCapability::OnSendingPDU(H245_VideoCapability & /*pdu*/) const
{
  return false;
}


#if OPAL_H239
PBoolean H323VideoCapability::OnSendingPDU(H245_VideoCapability & pdu, CommandType type) const
{
  if (type != e_OLC)
    return OnSendingPDU(pdu);

  OpalVideoFormat::ContentRole role = GetMediaFormat().GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole);
  if (role == OpalVideoFormat::eNoRole)
    return OnSendingPDU(pdu);

  H323H239VideoCapability h239(GetMediaFormat());
  return h239.OnSendingPDU(pdu, type);
}
#else // OPAL_H239
PBoolean H323VideoCapability::OnSendingPDU(H245_VideoCapability & pdu, CommandType) const
{
  return OnSendingPDU(pdu);
}
#endif // OPAL_H239


PBoolean H323VideoCapability::OnSendingPDU(H245_VideoMode &) const
{
  return false;
}


PBoolean H323VideoCapability::OnSendingPDU(H245_ModeElement & mode) const
{
  mode.m_type.SetTag(H245_ModeElementType::e_videoMode);
  return OnSendingPDU((H245_VideoMode &)mode.m_type);
}


PBoolean H323VideoCapability::OnReceivedPDU(const H245_Capability & cap)
{
  if (cap.GetTag() != H245_Capability::e_receiveVideoCapability &&
      cap.GetTag() != H245_Capability::e_receiveAndTransmitVideoCapability)
    return false;

  return OnReceivedPDU((const H245_VideoCapability &)cap, e_TCS) && H323Capability::OnReceivedPDU(cap);
}


PBoolean H323VideoCapability::OnReceivedPDU(const H245_DataType & dataType, PBoolean receiver)
{
  const H245_VideoCapability * cap;
  return H323GetMediaCapability<H245_DataType::e_videoData, H245_H235Media_mediaType::e_videoData>(dataType, cap) &&
         OnReceivedPDU(*cap, e_OLC) &&
         H323RealTimeCapability::OnReceivedPDU(dataType, receiver);
}


PBoolean H323VideoCapability::OnReceivedPDU(const H245_VideoCapability &)
{
  return false;
}


PBoolean H323VideoCapability::OnReceivedPDU(const H245_VideoCapability & pdu, CommandType)
{
  return OnReceivedPDU(pdu);
}


unsigned H323VideoCapability::GetDefaultSessionID() const
{
  return DefaultVideoSessionID;
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


PBoolean H323NonStandardVideoCapability::OnSendingPDU(H245_VideoCapability & pdu) const
{
  return OnSendingNonStandardPDU(pdu, H245_VideoCapability::e_nonStandard);
}


PBoolean H323NonStandardVideoCapability::OnSendingPDU(H245_VideoMode & pdu) const
{
  return OnSendingNonStandardPDU(pdu, H245_VideoMode::e_nonStandard);
}


PBoolean H323NonStandardVideoCapability::OnReceivedPDU(const H245_VideoCapability & pdu)
{
  return OnReceivedNonStandardPDU(pdu, H245_VideoCapability::e_nonStandard);
}


PBoolean H323NonStandardVideoCapability::IsMatch(const PASN_Object & subTypePDU, const PString & mediaPacketization) const
{
  return H323Capability::IsMatch(subTypePDU, mediaPacketization) &&
         H323NonStandardCapabilityInfo::IsMatch((const H245_NonStandardParameter &)dynamic_cast<const H245_VideoCapability &>(subTypePDU));
}

/////////////////////////////////////////////////////////////////////////////

H323GenericVideoCapability::H323GenericVideoCapability(const PString &standardId, unsigned maxBitRate)
  : H323VideoCapability(),
    H323GenericCapabilityInfo(standardId, maxBitRate, false)
{
}

PObject::Comparison H323GenericVideoCapability::Compare(const PObject & obj) const
{
  Comparison result = H323VideoCapability::Compare(obj);
  if (result != EqualTo)
    return result;

  return CompareInfo(dynamic_cast<const H323GenericVideoCapability &>(obj));
}


unsigned H323GenericVideoCapability::GetSubType() const
{
  return H245_VideoCapability::e_genericVideoCapability;
}


PBoolean H323GenericVideoCapability::OnSendingPDU(H245_VideoCapability & pdu, CommandType type) const
{
  pdu.SetTag(H245_VideoCapability::e_genericVideoCapability);
  return OnSendingGenericPDU(pdu, GetMediaFormat(), type);
}

PBoolean H323GenericVideoCapability::OnSendingPDU(H245_VideoMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_genericVideoMode);
  return OnSendingGenericPDU(pdu, GetMediaFormat(), e_ReqMode);
}

PBoolean H323GenericVideoCapability::OnReceivedPDU(const H245_VideoCapability & pdu, CommandType type)
{
  if (pdu.GetTag() != H245_VideoCapability::e_genericVideoCapability)
    return false;
  return OnReceivedGenericPDU(GetWritableMediaFormat(), pdu, type);
}

PBoolean H323GenericVideoCapability::IsMatch(const PASN_Object & subTypePDU, const PString & mediaPacketization) const
{
  return H323Capability::IsMatch(subTypePDU, mediaPacketization) &&
         H323GenericCapabilityInfo::IsMatch(GetMediaFormat(), (const H245_GenericCapability &)dynamic_cast<const H245_VideoCapability &>(subTypePDU));
}


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H239

H323ExtendedVideoCapability::H323ExtendedVideoCapability(const PString & identifier)
  : H323GenericVideoCapability(identifier)
{
}


unsigned H323ExtendedVideoCapability::GetSubType() const
{
  return H245_VideoCapability::e_extendedVideoCapability;
}


PBoolean H323ExtendedVideoCapability::OnSendingPDU(H245_VideoCapability & pdu, CommandType type) const
{
  pdu.SetTag(H245_VideoCapability::e_extendedVideoCapability);
  H245_ExtendedVideoCapability & extcap = pdu;

  unsigned roleMask = UINT_MAX;

  for (OpalMediaFormatList::const_iterator videoFormat = m_videoFormats.begin(); videoFormat != m_videoFormats.end(); ++videoFormat) {
    if (videoFormat->GetMediaType() == OpalMediaType::Video()) {
      H323Capability * capability = H323Capability::Create(videoFormat->GetName());
      if (capability != NULL) {
        capability->UpdateMediaFormat(*videoFormat);
        H245_Capability h245Cap;
        if (capability->OnSendingPDU(h245Cap)) {
          PINDEX size = extcap.m_videoCapability.GetSize();
          extcap.m_videoCapability.SetSize(size+1);
          extcap.m_videoCapability[size] = h245Cap;
          if (type != e_TCS)
            roleMask = OpalVideoFormat::ContentRoleBit(videoFormat->GetOptionEnum(
                                    OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eMainRole));
          else
            roleMask &= videoFormat->GetOptionInteger(OpalVideoFormat::ContentRoleMaskOption());
        }
        delete capability;
      }
    }
  }
  if (extcap.m_videoCapability.GetSize() == 0) {
    PTRACE(2, "H323\tCannot encode H.239 video capability, no extended video codecs available");
    return false;
  }

  OpalMediaFormat videoCapExtMediaFormat(GetH239VideoMediaFormat());
  if ((roleMask&0xfffc) != 0)
    roleMask = (roleMask&3)|2;
  videoCapExtMediaFormat.SetOptionInteger(OpalVideoFormat::ContentRoleMaskOption(), roleMask);

  extcap.IncludeOptionalField(H245_ExtendedVideoCapability::e_videoCapabilityExtension);
  extcap.m_videoCapabilityExtension.SetSize(1);
  return OnSendingGenericPDU(extcap.m_videoCapabilityExtension[0], GetH239VideoMediaFormat(), type);
}


PBoolean H323ExtendedVideoCapability::OnSendingPDU(H245_VideoMode &) const
{
  return false;
}


PBoolean H323ExtendedVideoCapability::OnReceivedPDU(const H245_VideoCapability & pdu, CommandType type)
{
  if (pdu.GetTag() != H245_VideoCapability::e_extendedVideoCapability)
    return false;

  const H245_ExtendedVideoCapability & extcap = pdu;
  if (!extcap.HasOptionalField(H245_ExtendedVideoCapability::e_videoCapabilityExtension)) {
    PTRACE(2, "H323\tNo H.239 video capability extension");
    return false;
  }

  OpalMediaFormat videoCapExtMediaFormat(GetH239VideoMediaFormat());

  PINDEX i = 0;
  for (;;) {
    if (i >= extcap.m_videoCapabilityExtension.GetSize()) {
      PTRACE(2, "H323\tNo H.239 video capability extension for " << m_identifier);
      return false;
    }

    if (H323GenericCapabilityInfo::IsMatch(videoCapExtMediaFormat, extcap.m_videoCapabilityExtension[i]))
      break;

    ++i;
  }

  if (!OnReceivedGenericPDU(videoCapExtMediaFormat, extcap.m_videoCapabilityExtension[i], type))
    return false;

  unsigned roleMask = videoCapExtMediaFormat.GetOptionInteger(OpalVideoFormat::ContentRoleMaskOption());

  OpalVideoFormat::ContentRole role = OpalVideoFormat::EndContentRole;
  while (--role > OpalVideoFormat::BeginContentRole && (OpalVideoFormat::ContentRoleBit(role)&roleMask) == 0)
     ;

  H323CapabilityFactory::KeyList_T stdCaps = H323CapabilityFactory::GetKeyList();

  m_videoFormats.RemoveAll();

  for (i = 0; i < extcap.m_videoCapability.GetSize(); ++i) {
    const H245_VideoCapability & vidCap = extcap.m_videoCapability[i];
    for (H323CapabilityFactory::KeyList_T::const_iterator iterCap = stdCaps.begin(); iterCap != stdCaps.end(); ++iterCap) {
      H323Capability * capability = H323Capability::Create(*iterCap);
      if (capability->GetMainType() == H323Capability::e_Video &&
          capability->IsMatch(vidCap, PString::Empty()) &&
          dynamic_cast<H323VideoCapability*>(capability)->OnReceivedPDU(vidCap, type)) {
        OpalMediaFormat mediaFormat = capability->GetMediaFormat();
        mediaFormat.SetOptionInteger(OpalVideoFormat::ContentRoleMaskOption(), roleMask);
        if (type != e_TCS)
          mediaFormat.SetOptionEnum(OpalVideoFormat::ContentRoleOption(), role);
        m_videoFormats += mediaFormat;
      }
      delete capability;
    }
  }

  PTRACE(4, "H323\tExtended video: " << setfill(',') << m_videoFormats);
  return !m_videoFormats.IsEmpty();
}


PBoolean H323ExtendedVideoCapability::IsMatch(const PASN_Object & subTypePDU, const PString & mediaPacketization) const
{
  if (!H323Capability::IsMatch(subTypePDU, mediaPacketization))
    return false;

  const H245_ExtendedVideoCapability & extcap = (const H245_ExtendedVideoCapability &)dynamic_cast<const H245_VideoCapability &>(subTypePDU);
  if (!extcap.HasOptionalField(H245_ExtendedVideoCapability::e_videoCapabilityExtension))
    return false;

  for (PINDEX i = 0; i < extcap.m_videoCapabilityExtension.GetSize(); ++i) {
    if (H323GenericCapabilityInfo::IsMatch(GetH239VideoMediaFormat(), extcap.m_videoCapabilityExtension[i]))
      return true;
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////

H323GenericControlCapability::H323GenericControlCapability(const PString & identifier)
  : H323GenericCapabilityInfo(identifier, 0, true)
{
}


H323Capability::MainTypes H323GenericControlCapability::GetMainType() const
{
  return e_GenericControl;
}


unsigned H323GenericControlCapability::GetSubType() const
{
  return 0; // Unused
}


PBoolean H323GenericControlCapability::OnSendingPDU(H245_Capability & pdu) const
{
  pdu.SetTag(H245_Capability::e_genericControlCapability);
  return OnSendingGenericPDU(pdu, GetMediaFormat(), e_OLC);
}


PBoolean H323GenericControlCapability::OnReceivedPDU(const H245_Capability & pdu)
{
  if (pdu.GetTag() != H245_Capability::e_genericControlCapability)
    return false;
  return OnReceivedGenericPDU(GetWritableMediaFormat(), pdu, e_OLC);
}


PBoolean H323GenericControlCapability::IsMatch(const PASN_Object & subTypePDU, const PString &) const
{
  return H323GenericCapabilityInfo::IsMatch(GetMediaFormat(), dynamic_cast<const H245_GenericCapability &>(subTypePDU));
}


/////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetH239VideoMediaFormat()
{
  class OpalH239VideoMediaFormatInternal : public OpalMediaFormatInternal {
    public:
      OpalH239VideoMediaFormatInternal()
        : OpalMediaFormatInternal("H.239-Video",
                                  OpalPresentationVideoMediaDefinition::Name(),
                                  RTP_DataFrame::MaxPayloadType,
                                  NULL, false, 0, 0, 0, 0)
      {
        OpalMediaOption * option = new OpalMediaOptionUnsigned(OpalVideoFormat::ContentRoleMaskOption(),
                                                               true, OpalMediaOption::IntersectionMerge, 1, 1, 3);

        OpalMediaOption::H245GenericInfo genericInfo;
        genericInfo.ordinal = 1;
        genericInfo.mode = OpalMediaOption::H245GenericInfo::Collapsing;
        genericInfo.integerType = OpalMediaOption::H245GenericInfo::BooleanArray;
        genericInfo.excludeTCS = false;
        genericInfo.excludeOLC = false;
        genericInfo.excludeReqMode = true;
        option->SetH245Generic(genericInfo);

        AddOption(option);
      }
  };
  static OpalMediaFormatStatic<OpalMediaFormat> format(new OpalH239VideoMediaFormatInternal);
  return format;
}


H323H239VideoCapability::H323H239VideoCapability(const OpalMediaFormat & mediaFormat)
  : H323ExtendedVideoCapability("0.0.8.239.1.2")
{
  GetWritableMediaFormat() = mediaFormat;
}


PObject::Comparison H323H239VideoCapability::Compare(const PObject & obj) const
{
  Comparison comparison = H323ExtendedVideoCapability::Compare(obj);
  if (comparison != EqualTo)
    return comparison;

  OpalMediaFormat otherFormat = ((H323Capability &)obj).GetMediaFormat();
  for (PINDEX i = 0; i < m_videoFormats.GetSize(); ++i) {
    if (m_videoFormats[i] == otherFormat) {
      const_cast<H323H239VideoCapability *>(this)->GetWritableMediaFormat() = m_videoFormats[i];
      return EqualTo;
    }
  }
  return GetMediaFormat().Compare(otherFormat);
}


PObject * H323H239VideoCapability::Clone() const
{
  return new H323H239VideoCapability(*this);
}


PString H323H239VideoCapability::GetFormatName() const
{
  return PSTRSTRM(m_mediaFormat.GetName() << '+' << GetH239VideoMediaFormat());
}


PBoolean H323H239VideoCapability::OnSendingPDU(H245_VideoCapability & pdu, CommandType type) const
{
  const_cast<H323H239VideoCapability*>(this)->m_videoFormats += GetMediaFormat();
  return H323ExtendedVideoCapability::OnSendingPDU(pdu, type);
}


PBoolean H323H239VideoCapability::OnReceivedPDU(const H245_VideoCapability & pdu, CommandType type)
{
  if (!H323ExtendedVideoCapability::OnReceivedPDU(pdu, type))
    return false;

  OpalMediaFormatList::const_iterator it = m_videoFormats.FindFormat(GetMediaFormat());
  if (it != m_videoFormats.end())
    GetWritableMediaFormat().Merge(*it);
  else
    GetWritableMediaFormat() = m_videoFormats.front();
  return true;
}


/////////////////////////////////////////////////////////////////////////////

H323H239ControlCapability::H323H239ControlCapability()
  : H323GenericControlCapability("0.0.8.239.1.1")
{
}


PObject * H323H239ControlCapability::Clone() const
{
  return new H323H239ControlCapability(*this);
}


PString H323H239ControlCapability::GetFormatName() const
{
  static const char name[] = "H.239-Control";
  static OpalMediaFormatStatic<OpalMediaFormat> h239(new OpalMediaFormatInternal(name,
                                                     OpalPresentationVideoMediaDefinition::Name(),
                                                     RTP_DataFrame::MaxPayloadType,
                                                     NULL, false, 0, 0, 0, 0));
  return name;
}


#endif  // OPAL_H239

#endif // OPAL_VIDEO


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H235_6 || OPAL_H235_8

OPAL_INSTANTIATE_SIMPLE_MEDIATYPE(OpalH235MediaType, "H.235");

H235SecurityCapability::H235SecurityCapability(const H323Capability & mediaCapability)
  : m_mediaCapabilityNumber(mediaCapability.GetCapabilityNumber())
  , m_mediaCapabilityName(mediaCapability.GetFormatName())
{
  m_mediaCapabilityName += '+';
  m_mediaFormat = mediaCapability.GetMediaFormat();
}


H323Capability::MainTypes H235SecurityCapability::GetMainType() const
{
  return e_H235Security;
}


unsigned H235SecurityCapability::GetSubType()  const
{
  return 0;
}


PString H235SecurityCapability::GetFormatName() const
{
  return m_mediaCapabilityName;
}


PBoolean H235SecurityCapability::OnSendingPDU(H245_Capability & pdu) const
{
  pdu.SetTag(H245_Capability::e_h235SecurityCapability);
  H245_H235SecurityCapability & cap = pdu;
  cap.m_mediaCapability = m_mediaCapabilityNumber;
  return OnSendingPDU(cap.m_encryptionAuthenticationAndIntegrity);
}


PBoolean H235SecurityCapability::OnReceivedPDU(const H245_Capability & pdu)
{
  if (pdu.GetTag() != H245_Capability::e_h235SecurityCapability)
    return false;

  const H245_H235SecurityCapability & cap = pdu;
  m_mediaCapabilityNumber = cap.m_mediaCapability;
  return OnReceivedPDU(cap.m_encryptionAuthenticationAndIntegrity);
}


bool H235SecurityCapability::OnSendingPDU(H245_EncryptionSync & encryptionSync,
                                          const H323Connection & connection,
                                          unsigned sessionID,
                                          bool rx)
{
  OpalMediaSession * session = connection.GetMediaSession(sessionID);
  if (session == NULL) {
    PTRACE(3, "H323\tNot adding H.235 encryption key as no media session for id=" << sessionID);
    return false;
  }

  for (OpalMediaCryptoSuite::List::iterator it = m_cryptoSuites.begin(); it != m_cryptoSuites.end(); ++it)
    session->OfferCryptoSuite(it->GetFactoryName());

  OpalMediaCryptoKeyList & keys = session->GetOfferedCryptoKeys();
  if (keys.IsEmpty()) {
    PTRACE(3, "H323\tNot adding H.235 encryption key as no keys offered in session id=" << sessionID);
    return false;
  }

  if (!OnSendingPDU(encryptionSync, connection, keys))
    return false;

  return session->ApplyCryptoKey(keys, rx);
}


bool H235SecurityCapability::OnReceivedPDU(const H245_EncryptionSync & encryptionSync,
                                           const H323Connection & connection,
                                           unsigned sessionID,
                                           bool rx)
{
  OpalMediaSession * session = connection.GetMediaSession(sessionID);
  if (session == NULL) {
    PTRACE(3, "H323\tNot adding H.235 encryption key as no media session for id=" << sessionID);
    return false;
  }

  OpalMediaCryptoKeyList keys;
  if (!OnReceivedPDU(encryptionSync, connection, keys))
    return false;

  return session->ApplyCryptoKey(keys, rx);
}


bool H235SecurityCapability::PostTCS(const H323Connection & connection, const H323Capabilities & capabilities)
{
  PStringArray availableCryptoSuites = connection.OpalRTPConnection::GetMediaCryptoSuites();
  for (OpalMediaCryptoSuite::List::iterator it = m_cryptoSuites.begin(); it != m_cryptoSuites.end();) {
    if (availableCryptoSuites.GetValuesIndex(it->GetFactoryName()) != P_MAX_INDEX)
      ++it;
    else
      m_cryptoSuites.erase(it++);
  }
  if (m_cryptoSuites.IsEmpty()) {
    PTRACE(4, "H323\tH.235 crypto suite(s) not available.");
    return false;
  }

  H323Capability * cap = capabilities.FindCapability(m_mediaCapabilityNumber);
  if (cap == NULL) {
    PTRACE(3, "H323\tH.235 media capability number (" << m_mediaCapabilityNumber << ") does not match anything.");
    return false;
  }

  m_mediaFormat = cap->GetMediaFormat();
  m_mediaCapabilityName.Splice(cap->GetFormatName(), 0, m_mediaCapabilityName.FindLast('+'));
  return true;
}


void H235SecurityCapability::AddAllCapabilities(H323Capabilities & capabilities,
                                                const PStringArray & cryptoSuiteNames,
                                                const char * prefix)
{
  OpalMediaCryptoSuite::List cryptoSuites = OpalMediaCryptoSuite::FindAll(cryptoSuiteNames, prefix);
  if (cryptoSuites.IsEmpty())
    return;

  const H323CapabilitiesSet & set = capabilities.GetSet();
  PINDEX outerSize = set.GetSize();
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    PINDEX middleSize = set[outer].GetSize();
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      PINDEX innerSize = set[outer][middle].GetSize();
      for (PINDEX inner = 0; inner < innerSize; inner++) {
        H323Capability & capability = set[outer][middle][inner];
        if (capability.GetMediaFormat().GetMediaType()->GetMediaSessionType().Find("RTP") != P_MAX_INDEX) {
          H235SecurityCapability * cap = cryptoSuites.front().CreateCapability(capability);
          cap->SetCryptoSuites(cryptoSuites);
          capabilities.SetCapability(outer, middle, cap);
        }
      }
    }
  }
}

#endif // OPAL_H235_6 || OPAL_H235_8


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H235_6

H235SecurityAlgorithmCapability::H235SecurityAlgorithmCapability(const H323Capability & mediaCapability)
  : H235SecurityCapability(mediaCapability)
{
  static const char name[] = "H.235.6";
  static OpalMediaFormatStatic<OpalMediaFormat> h2356(new OpalMediaFormatInternal(name,
                                                      OpalH235MediaType::Name(),
                                                      RTP_DataFrame::MaxPayloadType,
                                                      NULL, false, 0, 0, 0, 0));
  m_mediaCapabilityName += name;
}


PObject * H235SecurityAlgorithmCapability::Clone() const
{
  return new H235SecurityAlgorithmCapability(*this);
}


static bool OpenCipher(PSSLCipherContext & cipher, const OpalMediaCryptoSuite & cryptoSuite, const H323Connection & connection)
{
  if (!cipher.SetAlgorithm(cryptoSuite.GetOID())) {
    PTRACE(2, "H323\tCould not set SSL cipher algorithm for " << cryptoSuite);
    return false;
  }

  PBYTEArray dhMasterkey = connection.GetDiffieHellman().FindMasterKey(cryptoSuite);
  if (dhMasterkey.IsEmpty()) {
    PTRACE(2, "H323\tNo Diffie-Hellman key for " << cryptoSuite);
    return false;
  }

  PINDEX keyLen = cryptoSuite.GetCipherKeyBytes();
  if (!cipher.SetKey(dhMasterkey.GetPointer() + dhMasterkey.GetSize() - keyLen, keyLen))
    return false;

  PINDEX ivLen = cipher.GetIVLength();
  BYTE * ivPtr = (BYTE *)alloca(ivLen);
  memset(ivPtr, 0, ivLen);
  return cipher.SetIV(ivPtr, ivLen) && cipher.SetPadding(PSSLCipherContext::NoPadding);
}


bool H235SecurityAlgorithmCapability::OnSendingPDU(H245_EncryptionSync & encryptionSync,
                                                   const H323Connection & connection,
                                                   const OpalMediaCryptoKeyList & keys)
{
  if (!connection.IsH245Master()) {
    PTRACE(3, "H323\tNot adding H.235 encryption key as we are not master");
    return false;
  }

  PASN_BMPString endpointID;
  if (connection.GetEndPoint().GetGatekeeper() != NULL)
    connection.GetEndPoint().GetGatekeeper()->GetEndpointIdentifier(endpointID);

  const OpalMediaCryptoSuite & cryptoSuite = keys.front().GetCryptoSuite();

  PSSLCipherContext enc(true);
  if (!OpenCipher(enc, cryptoSuite, connection))
    return false;

  H235_H235Key h235key;

  if (connection.GetDiffieHellman().IsVersion3()) {
    h235key.SetTag(H235_H235Key::e_secureSharedSecret);
    H235_V3KeySyncMaterial & v3data = h235key;

    if (!((PWCharArray)endpointID).IsEmpty()) {
      v3data.IncludeOptionalField(H235_V3KeySyncMaterial::e_generalID);
      v3data.m_generalID = endpointID;
    }

    v3data.IncludeOptionalField(H235_V3KeySyncMaterial::e_algorithmOID);
    v3data.m_algorithmOID = cryptoSuite.GetOID();

    v3data.IncludeOptionalField(H235_V3KeySyncMaterial::e_encryptedSessionKey);
    if (!enc.Process(keys.front().GetCipherKey(), v3data.m_encryptedSessionKey.GetWritableValue())) {
      PTRACE(2, "H323\tNot adding H.235 encryption key as encryption failed.");
      return false;
    }
  }
  else {
    h235key.SetTag(H235_H235Key::e_sharedSecret);
    H235_ENCRYPTED<H235_EncodedKeySyncMaterial> & eksm = h235key;
    eksm.m_algorithmOID = cryptoSuite.GetOID();

    H235_KeySyncMaterial ksm;
    ksm.m_generalID = endpointID;
    ksm.m_keyMaterial.SetData(keys.front().GetCipherKey());
    eksm.m_clearData.EncodeSubType(ksm);

    if (!enc.Process(eksm.m_clearData.GetValue(), eksm.m_encryptedData.GetWritableValue())) {
      PTRACE(2, "H323\tNot adding H.235 encryption key as encryption failed.");
      return false;
    }
  }

  encryptionSync.m_h235Key.EncodeSubType(h235key);
  return true;
}


bool H235SecurityAlgorithmCapability::OnReceivedPDU(const H245_EncryptionSync & encryptionSync,
                                                    const H323Connection & connection,
                                                    OpalMediaCryptoKeyList & keys)
{
  H235_H235Key h235key;
  if (!encryptionSync.m_h235Key.DecodeSubType(h235key)) {
    PTRACE(1, "H323\tCould not decode H.235 encryption key");
    return false;
  }
  PTRACE(4, "H323", "Decoded H.235 encryption key:\n  " << setprecision(2) << h235key);

  OpalMediaCryptoSuite * cryptoSuite;
  PBYTEArray sessionKey;

  switch (h235key.GetTag()) {
    case H235_H235Key::e_sharedSecret :
    {
      const H235_ENCRYPTED<H235_EncodedKeySyncMaterial> & eksm = h235key;

      if ((cryptoSuite = OpalMediaCryptoSuite::FindByOID(eksm.m_algorithmOID.AsString())) == NULL) {
        PTRACE(1, "H323\tH.235 encryption key uses unknown algorithm");
        return false;
      }

      PSSLCipherContext dec(false);
      if (!OpenCipher(dec, *cryptoSuite, connection))
        return false;

      if (!dec.Process(eksm.m_encryptedData.GetValue(), const_cast<H235_EncodedKeySyncMaterial &>(eksm.m_clearData).GetWritableValue())) {
        PTRACE(2, "H323\tH.235 encryption key decryption failed.");
        return false;
      }

      H235_KeySyncMaterial ksm;
      if (!eksm.m_clearData.DecodeSubType(ksm)) {
        PTRACE(1, "H323\tCould not decode H.235 KeySyncMaterial");
        return false;
      }
      PTRACE(4, "H323", "Decoded H.235 KeySyncMaterial:\n  " << setprecision(2) << ksm);

      sessionKey = ksm.m_keyMaterial.GetData();
      break;
    }

    case H235_H235Key::e_secureSharedSecret :
    {
      const H235_V3KeySyncMaterial & v3data = h235key;

      if (v3data.HasOptionalField(H235_V3KeySyncMaterial::e_algorithmOID)) {
        if ((cryptoSuite = OpalMediaCryptoSuite::FindByOID(v3data.m_algorithmOID.AsString())) == NULL) {
          PTRACE(1, "H323\tH.235 encryption key uses unknown algorithm");
          return false;
        }
      }
      else {
        if (m_cryptoSuites.IsEmpty()) {
          PTRACE(1, "H323\tH.235 encryption key has no algorithm, aborting");
          return false;
        }
        cryptoSuite = &m_cryptoSuites.front();
        PTRACE(3, "H323\tH.235 encryption key has no algorithm, using offer: " << *cryptoSuite);
      }

      if (!v3data.HasOptionalField(H235_V3KeySyncMaterial::e_encryptedSessionKey)) {
        PTRACE(1, "H323\tH.235 encryption key has no session data");
        return false;
      }

      PSSLCipherContext dec(false);
      if (!OpenCipher(dec, *cryptoSuite, connection))
        return false;

      if (!dec.Process(v3data.m_encryptedSessionKey.GetValue(), sessionKey)) {
        PTRACE(2, "H323\tH.235 encryption key decryption failed.");
        return false;
      }

      break;
    }

    default :
      PTRACE(1, "H323\tH.235 encryption key format not supported");
      return false;
  }

  if (sessionKey.GetSize() < cryptoSuite->GetCipherKeyBytes()) {
    PTRACE(2, "H323\tH.235 media session key not expected length");
    return false;
  }

  PTRACE(4, "H323", "Decoded H.235 media session key: " << hex << fixed << setfill('0') << sessionKey);

  keys.Append(cryptoSuite->CreateKeyInfo());
  keys.back().SetCipherKey(sessionKey);
  return true;
}


bool H235SecurityAlgorithmCapability::OnSendingPDU(H245_EncryptionAuthenticationAndIntegrity & cap) const
{
  cap.IncludeOptionalField(H245_EncryptionAuthenticationAndIntegrity::e_encryptionCapability);
  cap.m_encryptionCapability.SetSize(m_cryptoSuites.GetSize());

  for (PINDEX i = 0; i < m_cryptoSuites.GetSize(); ++i) {
    cap.m_encryptionCapability[i].SetTag(H245_MediaEncryptionAlgorithm::e_algorithm);
    (PASN_ObjectId &)cap.m_encryptionCapability[i] = m_cryptoSuites[i].GetOID();
  }

  return true;
}


bool H235SecurityAlgorithmCapability::OnReceivedPDU(const H245_EncryptionAuthenticationAndIntegrity & cap)
{
  if (!cap.HasOptionalField(H245_EncryptionAuthenticationAndIntegrity::e_encryptionCapability))
    return false;

  m_cryptoSuites.RemoveAll();

  for (PINDEX i = 0; i < cap.m_encryptionCapability.GetSize(); ++i) {
    OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuite::FindByOID(((const PASN_ObjectId &)cap.m_encryptionCapability[i]).AsString());
    if (cryptoSuite != NULL) {
      PTRACE(4, "H323\tFound Crypto-Suite for " << *cryptoSuite);
      m_cryptoSuites.Append(cryptoSuite);
    }
  }

  return true;
}


PBoolean H235SecurityAlgorithmCapability::IsMatch(const PASN_Object & subTypePDU, const PString &) const
{
  const H245_EncryptionAuthenticationAndIntegrity & cap = dynamic_cast<const H245_EncryptionAuthenticationAndIntegrity &>(subTypePDU);
  return cap.HasOptionalField(H245_EncryptionAuthenticationAndIntegrity::e_encryptionCapability) &&
         cap.m_encryptionCapability.GetSize() > 0;
}
#endif // OPAL_H235_6


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H235_8

H235SecurityGenericCapability::H235SecurityGenericCapability(const H323Capability & mediaCapability)
  : H235SecurityCapability(mediaCapability)
  , H323GenericCapabilityInfo("0.0.8.235.0.4.90", 0, true)
{
  static const char name[] = "H.235.8";
  static OpalMediaFormatStatic<OpalMediaFormat> h2358(new OpalMediaFormatInternal(name,
                                                       OpalH235MediaType::Name(),
                                                       RTP_DataFrame::MaxPayloadType,
                                                       NULL, false, 0, 0, 0, 0));
  m_mediaCapabilityName += name;
}


PObject * H235SecurityGenericCapability::Clone() const
{
  return new H235SecurityGenericCapability(*this);
}


bool H235SecurityGenericCapability::OnSendingPDU(H245_EncryptionSync & encryptionSync,
                                                 const H323Connection &,
                                                 const OpalMediaCryptoKeyList & keys)
{
  H235_SRTP_SrtpKeys h235;
  h235.SetSize(1);
  h235[0].m_masterKey.SetValue(keys[0].GetCipherKey());
  h235[0].m_masterSalt.SetValue(keys[0].GetAuthSalt());

  encryptionSync.m_h235Key.EncodeSubType(h235);
  return true;
}


bool H235SecurityGenericCapability::OnReceivedPDU(const H245_EncryptionSync & encryptionSync,
                                                  const H323Connection &,
                                                  OpalMediaCryptoKeyList & keys)
{
  H235_SRTP_SrtpKeys h235;
  if (!encryptionSync.m_h235Key.DecodeSubType(h235) || h235.GetSize() == 0) {
    PTRACE(1, "H323\tCould not decode SrtpKeys, or no keys present");
    return false;
  }
  PTRACE(4, "H323", "Decoded H.235 SRTP keys:\n  " << setprecision(2) << h235);

  for (PINDEX i = 0; i < h235.GetSize(); ++i) {
    H235_SRTP_SrtpKeyParameters & param = h235[i];
    OpalMediaCryptoKeyInfo * keyInfo = m_cryptoSuites.front().CreateKeyInfo();
    if (keyInfo != NULL) {
      keyInfo->SetCipherKey(param.m_masterKey.GetValue());
      keyInfo->SetAuthSalt(param.m_masterSalt.GetValue());
      keys.Append(keyInfo);
    }
  }

  return true;
}


bool H235SecurityGenericCapability::OnSendingPDU(H245_EncryptionAuthenticationAndIntegrity & cap) const
{
  if (!OnSendingGenericPDU(cap.m_genericH235SecurityCapability, GetMediaFormat(), e_OLC))
    return false;

  H235_SRTP_SrtpCryptoCapability srtpCap;
  for (PINDEX i = 0; i < m_cryptoSuites.GetSize(); ++i) {
    const OpalMediaCryptoSuite & cryptoSuite = m_cryptoSuites[i]; // Singleton, no need to destroy
    if (cryptoSuite.GetOID() != NULL) {
      PINDEX pos = srtpCap.GetSize();
      srtpCap.SetSize(pos+1);
      H235_SRTP_SrtpCryptoInfo & info = srtpCap[pos];

      info.IncludeOptionalField(H235_SRTP_SrtpCryptoInfo::e_cryptoSuite);
      info.m_cryptoSuite = cryptoSuite.GetOID();

      info.IncludeOptionalField(H235_SRTP_SrtpCryptoInfo::e_sessionParams);
      info.m_sessionParams.IncludeOptionalField(H235_SRTP_SrtpSessionParameters::e_unencryptedSrtp); // Default value is FALSE
      info.m_sessionParams.IncludeOptionalField(H235_SRTP_SrtpSessionParameters::e_unencryptedSrtcp); // Default value is FALSE
      info.m_sessionParams.IncludeOptionalField(H235_SRTP_SrtpSessionParameters::e_unauthenticatedSrtp); // Default value is FALSE
    }
  }
  if (srtpCap.GetSize() == 0) {
    PTRACE(1, "H323\tNo suitable Crypto-Suites to put into capability");
    return false;
  }

  cap.IncludeOptionalField(H245_EncryptionAuthenticationAndIntegrity::e_genericH235SecurityCapability);
  cap.m_genericH235SecurityCapability.IncludeOptionalField(H245_GenericCapability::e_nonCollapsingRaw);
  cap.m_genericH235SecurityCapability.m_nonCollapsingRaw.EncodeSubType(srtpCap);
  return true;
}


bool H235SecurityGenericCapability::OnReceivedPDU(const H245_EncryptionAuthenticationAndIntegrity & cap)
{
  if (!OnReceivedGenericPDU(GetWritableMediaFormat(), cap.m_genericH235SecurityCapability, e_TCS))
    return false;

  if (cap.m_genericH235SecurityCapability.m_nonCollapsingRaw.GetSize() == 0) {
    PTRACE(1, "H323\tMissing SrtpCryptoCapability");
    return false;
  }

  H235_SRTP_SrtpCryptoCapability srtpCap;
  if (!cap.m_genericH235SecurityCapability.m_nonCollapsingRaw.DecodeSubType(srtpCap)) {
    PTRACE(1, "H323\tCould not decode SrtpCryptoCapability");
    return false;
  }
  PTRACE(4, "H323", "Decoded H.235 SRTP capability:\n  " << setprecision(2) << srtpCap);

  if (srtpCap.GetSize() == 0) {
    PTRACE(1, "H323\tEmpty SrtpCryptoCapability");
    return false;
  }

  m_cryptoSuites.RemoveAll();

  for (PINDEX i = 0; i < srtpCap.GetSize(); ++i) {
    const H235_SRTP_SrtpCryptoInfo & info = srtpCap[i];
    OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuite::FindByOID(info.m_cryptoSuite.AsString());
    if (cryptoSuite != NULL) {
      PTRACE(4, "H323\tFound Crypto-Suite for " << *cryptoSuite);
      m_cryptoSuites.Append(cryptoSuite);
    }
  }

  return true;
}


PBoolean H235SecurityGenericCapability::IsMatch(const PASN_Object & subTypePDU, const PString &) const
{
  return H323GenericCapabilityInfo::IsMatch(GetMediaFormat(),
            dynamic_cast<const H245_EncryptionAuthenticationAndIntegrity &>(subTypePDU).m_genericH235SecurityCapability);
}
#endif // OPAL_H235_8


/////////////////////////////////////////////////////////////////////////////

H323DataCapability::H323DataCapability(unsigned rate)
  : m_maxBitRate(rate)
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


PBoolean H323DataCapability::OnSendingPDU(H245_Capability & cap) const
{
  static unsigned const tags[NumCapabilityDirections] = {
    H245_Capability::e_receiveAndTransmitDataApplicationCapability,
    H245_Capability::e_receiveDataApplicationCapability,
    H245_Capability::e_transmitDataApplicationCapability,
    H245_Capability::e_receiveAndTransmitDataApplicationCapability
  };
  cap.SetTag(tags[capabilityDirection]);
  H245_DataApplicationCapability & app = cap;
  m_maxBitRate.SetH245(app.m_maxBitRate);
  return OnSendingPDU(app, e_TCS);
}


PBoolean H323DataCapability::OnSendingPDU(H245_DataType & dataType) const
{
  H245_DataApplicationCapability * cap;
  H323SetMediaCapability<H245_DataType::e_data, H245_H235Media_mediaType::e_data>(*this, dataType, cap);
  m_maxBitRate.SetH245(cap->m_maxBitRate);
  return H323Capability::OnSendingPDU(dataType) && OnSendingPDU(*cap, e_OLC);
}


PBoolean H323DataCapability::OnSendingPDU(H245_DataApplicationCapability &) const
{
  return false;
}


PBoolean H323DataCapability::OnSendingPDU(H245_DataApplicationCapability & pdu, CommandType) const
{
  return OnSendingPDU(pdu);
}


PBoolean H323DataCapability::OnSendingPDU(H245_ModeElement & mode) const
{
  mode.m_type.SetTag(H245_ModeElementType::e_dataMode);
  H245_DataMode & type = mode.m_type;
  m_maxBitRate.SetH245(type.m_bitRate);
  return OnSendingPDU(type);
}


PBoolean H323DataCapability::OnReceivedPDU(const H245_Capability & cap)
{
  if (cap.GetTag() != H245_Capability::e_receiveDataApplicationCapability &&
      cap.GetTag() != H245_Capability::e_receiveAndTransmitDataApplicationCapability)
    return false;

  const H245_DataApplicationCapability & app = cap;
  m_maxBitRate.FromH245(app.m_maxBitRate);

  return OnReceivedPDU(app, e_TCS) && H323Capability::OnReceivedPDU(cap);
}


PBoolean H323DataCapability::OnReceivedPDU(const H245_DataType & dataType, PBoolean receiver)
{
  const H245_DataApplicationCapability * cap;
  if (!H323GetMediaCapability<H245_DataType::e_data, H245_H235Media_mediaType::e_data>(dataType, cap))
    return false;

  m_maxBitRate.FromH245(cap->m_maxBitRate);
  return OnReceivedPDU(*cap, e_OLC) && H323Capability::OnReceivedPDU(dataType, receiver);
}


PBoolean H323DataCapability::OnReceivedPDU(const H245_DataApplicationCapability &)
{
  return false;
}


PBoolean H323DataCapability::OnReceivedPDU(const H245_DataApplicationCapability & pdu, CommandType)
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
  Comparison result = H323DataCapability::Compare(obj);
  if (result != EqualTo)
    return result;

  return CompareInfo(dynamic_cast<const H323NonStandardDataCapability &>(obj));
}


unsigned H323NonStandardDataCapability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_nonStandard;
}


PBoolean H323NonStandardDataCapability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  return OnSendingNonStandardPDU(pdu.m_application, H245_DataApplicationCapability_application::e_nonStandard);
}


PBoolean H323NonStandardDataCapability::OnSendingPDU(H245_DataMode & pdu) const
{
  return OnSendingNonStandardPDU(pdu.m_application, H245_DataMode_application::e_nonStandard);
}


PBoolean H323NonStandardDataCapability::OnReceivedPDU(const H245_DataApplicationCapability & pdu)
{
  return OnReceivedNonStandardPDU(pdu.m_application, H245_DataApplicationCapability_application::e_nonStandard);
}


PBoolean H323NonStandardDataCapability::IsMatch(const PASN_Object & subTypePDU, const PString & mediaPacketization) const
{
  return H323Capability::IsMatch(subTypePDU, mediaPacketization) &&
         H323NonStandardCapabilityInfo::IsMatch((const H245_NonStandardParameter &)dynamic_cast<const H245_DataApplicationCapability_application & >(subTypePDU));
}


/////////////////////////////////////////////////////////////////////////////

H323GenericDataCapability::H323GenericDataCapability(const PString & standardId, unsigned maxBitRate)
  : H323DataCapability(maxBitRate)
  , H323GenericCapabilityInfo(standardId, 0, true)
{
}


PObject::Comparison H323GenericDataCapability::Compare(const PObject & obj) const
{
  Comparison result = H323DataCapability::Compare(obj);
  if (result != EqualTo)
    return result;

  return CompareInfo(dynamic_cast<const H323GenericDataCapability &>(obj));
}


unsigned H323GenericDataCapability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_genericDataCapability;
}


PBoolean H323GenericDataCapability::OnSendingPDU(H245_DataApplicationCapability & pdu, CommandType type) const
{
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_genericDataCapability);
  return OnSendingGenericPDU(pdu.m_application, GetMediaFormat(), type);
}


PBoolean H323GenericDataCapability::OnSendingPDU(H245_DataMode & pdu) const
{
  return OnSendingGenericPDU(pdu.m_application, GetMediaFormat(), e_ReqMode);
}


PBoolean H323GenericDataCapability::OnReceivedPDU(const H245_DataApplicationCapability & pdu, CommandType type)
{
  if (pdu.m_application.GetTag() != H245_DataApplicationCapability_application::e_genericDataCapability)
    return false;

  return OnReceivedGenericPDU(GetWritableMediaFormat(), pdu.m_application, type);
}


PBoolean H323GenericDataCapability::IsMatch(const PASN_Object & subTypePDU, const PString & mediaPacketization) const
{
  return H323Capability::IsMatch(subTypePDU, mediaPacketization) &&
         H323GenericCapabilityInfo::IsMatch(GetMediaFormat(),
                (const H245_GenericCapability &)dynamic_cast<const H245_DataApplicationCapability_application & >(subTypePDU));
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
    OpalMediaType::UserInput(), RTP_DataFrame::MaxPayloadType, NULL, false, 1, 0, 0, 0 \
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


PBoolean H323_UserInputCapability::OnSendingPDU(H245_Capability & pdu) const
{
  if (subType == SignalToneRFC2833) {
    pdu.SetTag(H245_Capability::e_receiveRTPAudioTelephonyEventCapability);
    H245_AudioTelephonyEventCapability & atec = pdu;
    OpalMediaFormat mediaFormat = GetMediaFormat();
    atec.m_dynamicRTPPayloadType = mediaFormat.GetPayloadType();
    PString events;
    if (!mediaFormat.GetOptionValue(OpalRFC288EventsName(), events))
      return false;
    atec.m_audioTelephoneEvent = events;
  }
  else {
    static unsigned const tags[NumCapabilityDirections] = {
      H245_Capability::e_receiveAndTransmitUserInputCapability,
      H245_Capability::e_receiveUserInputCapability,
      H245_Capability::e_transmitUserInputCapability,
      H245_Capability::e_receiveAndTransmitUserInputCapability
    };
    pdu.SetTag(tags[capabilityDirection]);
    H245_UserInputCapability & ui = pdu;
    ui.SetTag(UserInputCapabilitySubTypeCodes[subType]);
  }
  return true;
}


PBoolean H323_UserInputCapability::OnSendingPDU(H245_DataType &) const
{
  PTRACE(1, "H323\tCannot have UserInputCapability in DataType");
  return false;
}


PBoolean H323_UserInputCapability::OnReceivedPDU(const H245_Capability & pdu)
{
  if (pdu.GetTag() == H245_Capability::e_receiveRTPAudioTelephonyEventCapability) {
    subType = SignalToneRFC2833;
    const H245_AudioTelephonyEventCapability & atec = pdu;
    OpalMediaFormat & mediaFormat = GetWritableMediaFormat();
    mediaFormat.SetPayloadType((RTP_DataFrame::PayloadTypes)(int)atec.m_dynamicRTPPayloadType);
    mediaFormat.SetOptionValue(OpalRFC288EventsName(), atec.m_audioTelephoneEvent);
    return H323Capability::OnReceivedPDU(pdu);
  }

  if (pdu.GetTag() != H245_Capability::e_receiveUserInputCapability &&
      pdu.GetTag() != H245_Capability::e_receiveAndTransmitUserInputCapability)
    return false;

  const H245_UserInputCapability & ui = pdu;
  return ui.GetTag() == UserInputCapabilitySubTypeCodes[subType] && H323Capability::OnReceivedPDU(pdu);
}


PBoolean H323_UserInputCapability::OnReceivedPDU(const H245_DataType &, PBoolean)
{
  PTRACE(1, "H323\tCannot have UserInputCapability in DataType");
  return false;
}


PBoolean H323_UserInputCapability::IsUsable(const H323Connection & connection) const
{
  if (connection.GetControlVersion() >= 7)
    return true;

  if (connection.HasCompatibilityIssue(H323Connection::e_NoUserInputCapability))
    return false;

  return subType != SignalToneRFC2833;
}


static PINDEX SetUserInputCapability(H323Capabilities & capabilities,
                                     PINDEX descriptorNum,
                                     PINDEX simultaneous,
                                     H323_UserInputCapability::SubTypes subType)
{
  H323Capability * capability = capabilities.FindCapability(H323Capability::e_UserInput,
                                               UserInputCapabilitySubTypeCodes[subType]);
  if (capability == NULL)
    capability = new H323_UserInputCapability(subType);
  return capabilities.SetCapability(descriptorNum, simultaneous, capability);
}


void H323_UserInputCapability::AddAllCapabilities(H323Capabilities & capabilities,
                                                  PINDEX descriptorNum,
                                                  PINDEX simultaneous,
                                                  H323Capability * rfc2833)
{
  PINDEX num = SetUserInputCapability(capabilities, descriptorNum, simultaneous, HookFlashH245);
  if (descriptorNum == P_MAX_INDEX) {
    descriptorNum = num;
    simultaneous = P_MAX_INDEX;
  }
  else if (simultaneous == P_MAX_INDEX)
    simultaneous = num+1;

  num = SetUserInputCapability(capabilities, descriptorNum, simultaneous, BasicString);
  if (simultaneous == P_MAX_INDEX)
    simultaneous = num;

  SetUserInputCapability(capabilities, descriptorNum, simultaneous, SignalToneH245);

  if (rfc2833 != NULL)
    capabilities.SetCapability(descriptorNum, simultaneous, rfc2833);
}


#if OPAL_RTP_FEC

H323FECCapability::H323FECCapability(const OpalMediaFormat & mediaFormat, unsigned protectedCapability)
  : m_protectedCapability(protectedCapability)
{
  m_mediaFormat = mediaFormat;
}


H323Capability::MainTypes H323FECCapability::GetMainType() const
{
  return e_FEC;
}


unsigned H323FECCapability::GetSubType() const
{
  return 0;
}


PString H323FECCapability::GetFormatName() const
{
  return m_mediaFormat.GetName();
}


PBoolean H323FECCapability::OnSendingPDU(H245_Capability & pdu) const
{
  pdu.SetTag(H245_Capability::e_fecCapability);
  H245_FECCapability & cap = pdu;
  cap.m_protectedCapability = m_protectedCapability;
  cap.IncludeOptionalField(H245_FECCapability::e_rfc2733Format); // Now RFC 5109
  cap.m_rfc2733Format.SetTag(m_mediaFormat.GetName().NumCompare(OPAL_REDUNDANT_PREFIX) == EqualTo
                             ? H245_FECCapability_rfc2733Format::e_rfc2733rfc2198
                             : H245_FECCapability_rfc2733Format::e_rfc2733sameport);
  return true;
}


PBoolean H323FECCapability::OnReceivedPDU(const H245_Capability & pdu)
{
  if (pdu.GetTag() != H245_Capability::e_fecCapability)
    return false;
  
  const H245_FECCapability & cap = pdu;
  m_protectedCapability = cap.m_protectedCapability;
  if (!cap.HasOptionalField(H245_FECCapability::e_rfc2733Format)) { // Now RFC 5109
    PTRACE(3, "H323\tOnly RFC2733/RFC5109 FEC is supported.");
    return false;
  }

  switch (cap.m_rfc2733Format.GetTag()) {
    case H245_FECCapability_rfc2733Format::e_rfc2733rfc2198 :
      break;

    case H245_FECCapability_rfc2733Format::e_rfc2733sameport :
      break;

    default:
      PTRACE(3, "H323\tUsupported RFC2733/RFC5109 FEC mode.");
      return false;
  }

  return true;
}


void H323FECCapability::AddAllCapabilities(H323Capabilities & capabilities, const OpalMediaFormatList & localFormats)
{
  for (OpalMediaFormatList::const_iterator fmt = localFormats.begin(); fmt != localFormats.end(); ++fmt) {
    if (fmt->GetMediaType() == OpalFEC::MediaType()) {
      const H323CapabilitiesSet & set = capabilities.GetSet();
      PINDEX outerSize = set.GetSize();
      for (PINDEX outer = 0; outer < outerSize; outer++) {
        PINDEX middleSize = set[outer].GetSize();
        for (PINDEX middle = 0; middle < middleSize; middle++) {
          PINDEX innerSize = set[outer][middle].GetSize();
          for (PINDEX inner = 0; inner < innerSize; inner++) {
            H323Capability & capability = set[outer][middle][inner];
            OpalMediaType mediaType = capability.GetMediaFormat().GetMediaType();
            if (fmt->GetOptionString(OpalFEC::MediaTypeOption()) == mediaType &&
                mediaType->GetMediaSessionType().Find("RTP") != P_MAX_INDEX) {
              capabilities.SetCapability(outer, middle, new H323FECCapability(*fmt, capability.GetCapabilityNumber()));
            }
          }
        }
      }
    }
  }
}

#endif // OPAL_RTP_FEC


/////////////////////////////////////////////////////////////////////////////

PBoolean H323SimultaneousCapabilities::SetSize(PINDEX newSize)
{
  PINDEX oldSize = GetSize();

  if (!H323CapabilitiesListArray::SetSize(newSize))
    return false;

  while (oldSize < newSize) {
    H323CapabilitiesList * list = new H323CapabilitiesList;
    // The lowest level list should not delete codecs on destruction
    list->DisallowDeleteObjects();
    SetAt(oldSize++, list);
  }

  return true;
}


PBoolean H323CapabilitiesSet::SetSize(PINDEX newSize)
{
  PINDEX oldSize = GetSize();

  if (!H323CapabilitiesSetArray::SetSize(newSize))
    return false;

  while (oldSize < newSize)
    SetAt(oldSize++, new H323SimultaneousCapabilities);

  return true;
}


H323Capabilities::H323Capabilities()
{
}


H323Capabilities::H323Capabilities(H323Connection & connection,
                                   const H245_TerminalCapabilitySet & pdu)
{
  PTRACE_CONTEXT_ID_FROM(connection);

  PTRACE(4, "H323\tH323Capabilities(ctor)");

  /** If mediaPacketization information is available, use this with the FindCapability() logic.
   *  Certain codecs, such as H.263, need additional information in order to match the specific
   *  version of the codec against possibly multiple codec with the same 'subtype' such as
   *  e_h263VideoCapability.
   */
  m_mediaPacketizations += "RFC2190";  // Always supported
  m_mediaPacketizations += OpalPluginCodec_Identifer_H264_Aligned; // Always supported
  const H245_MultiplexCapability * muxCap = NULL;
  if (pdu.HasOptionalField(H245_TerminalCapabilitySet::e_multiplexCapability)) {
    muxCap = &pdu.m_multiplexCapability;

    if (muxCap != NULL && muxCap->GetTag() == H245_MultiplexCapability::e_h2250Capability) {
      // H2250Capability ::=SEQUENCE
      const H245_H2250Capability & h225_0 = *muxCap;

      //  MediaPacketizationCapability ::=SEQUENCE
      const H245_MediaPacketizationCapability & mediaPacket = h225_0.m_mediaPacketizationCapability;
      if (mediaPacket.HasOptionalField(H245_MediaPacketizationCapability::e_rtpPayloadType)) {
        for (PINDEX i = 0; i < mediaPacket.m_rtpPayloadType.GetSize(); i++) {
          PString mediaPacketization = H323GetRTPPacketization(mediaPacket.m_rtpPayloadType[i]);
          if (!mediaPacketization.IsEmpty()) {
            m_mediaPacketizations += mediaPacketization;
            PTRACE(4, "H323\tH323Capabilities(ctor) Appended mediaPacketization="
                   << mediaPacketization << ", mediaPacketization count=" << m_mediaPacketizations.GetSize());
          }
        } // for ... m_rtpPayloadType.GetSize()
      } // e_rtpPayloadType
    } // e_h2250Capability
  } // e_multiplexCapability

  // Decode out of the PDU, the list of known codecs.
  if (pdu.HasOptionalField(H245_TerminalCapabilitySet::e_capabilityTable)) {
    H323Capabilities allCapabilities(dynamic_cast<const H323EndPoint &>(connection.GetEndPoint()).GetCapabilities());
    OpalMediaFormatList localFormats = connection.GetLocalMediaFormats();
    PTRACE(4, "H323\tParsing remote capabilities");

    for (PINDEX i = 0; i < pdu.m_capabilityTable.GetSize(); i++) {
      if (pdu.m_capabilityTable[i].HasOptionalField(H245_CapabilityTableEntry::e_capability)) {
        H323Capability * capability = allCapabilities.FindCapability(pdu.m_capabilityTable[i].m_capability);
        if (capability != NULL) {
          H323Capability * copy = capability->CloneAs<H323Capability>();
          OpalMediaFormatList::const_iterator it = localFormats.FindFormat(copy->GetMediaFormat());
          if (it != localFormats.end())
            copy->UpdateMediaFormat(*it);
          if (!copy->OnReceivedPDU(pdu.m_capabilityTable[i].m_capability))
            delete copy;
          else {
            copy->SetCapabilityNumber(pdu.m_capabilityTable[i].m_capabilityTableEntryNumber);
            m_table.Append(copy);
          }
        }
      }
    }
  }

#if OPAL_H235_6 || OPAL_H235_8
  for (PINDEX i = 0; i < m_table.GetSize(); ) {
    if (m_table[i].PostTCS(connection, *this))
      ++i;
    else
      m_table.RemoveAt(i);
  }
#endif

  if (!m_mediaPacketizations.IsEmpty()) { // also update the mediaPacketizations option
    for (PINDEX i = 0; i < m_table.GetSize(); ++i) {
      OpalMediaFormat & mediaFormat = m_table[i].GetWritableMediaFormat();
      PStringSet intersection;
      if (PStringSet::Intersection(m_mediaPacketizations, mediaFormat.GetMediaPacketizationSet(), &intersection))
        mediaFormat.SetMediaPacketizations(intersection);
    }
  }

  PINDEX outerSize = pdu.m_capabilityDescriptors.GetSize();
  m_set.SetSize(outerSize);
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    H245_CapabilityDescriptor & desc = pdu.m_capabilityDescriptors[outer];
    if (desc.HasOptionalField(H245_CapabilityDescriptor::e_simultaneousCapabilities)) {
      PINDEX middleSize = desc.m_simultaneousCapabilities.GetSize();
      m_set[outer].m_capabilityDescriptorNumber = desc.m_capabilityDescriptorNumber;
      m_set[outer].SetSize(middleSize);
      for (PINDEX middle = 0; middle < middleSize; middle++) {
        H245_AlternativeCapabilitySet & alt = desc.m_simultaneousCapabilities[middle];
        for (PINDEX inner = 0; inner < alt.GetSize(); inner++) {
          for (PINDEX cap = 0; cap < m_table.GetSize(); cap++) {
            if (m_table[cap].GetCapabilityNumber() == alt[inner]) {
              m_set[outer][middle].Append(&m_table[cap]);
              break;
            }
          }
        }
      }
    }
  }
}


H323Capabilities::H323Capabilities(const H323Capabilities & original)
  : PObject(original)
{
  operator=(original);
}


H323Capabilities & H323Capabilities::operator=(const H323Capabilities & original)
{
  RemoveAll();
  Merge(original);
  return *this;
}


void H323Capabilities::PrintOn(ostream & strm) const
{
  std::streamsize indent = strm.precision()-1;
  strm << setw(indent) << " " << "Table:\n";
  for (PINDEX i = 0; i < m_table.GetSize(); i++)
    strm << setw(indent+2) << " " << m_table[i] << '\n';

  strm << setw(indent) << " " << "Set:\n";
  for (PINDEX outer = 0; outer < m_set.GetSize(); outer++) {
    strm << setw(indent+2) << " " << outer << ": capabilityDescriptorNumber = " << m_set[outer].m_capabilityDescriptorNumber << '\n';
    for (PINDEX middle = 0; middle < m_set[outer].GetSize(); middle++) {
      strm << setw(indent+4) << " " << middle << ":\n";
      for (PINDEX inner = 0; inner < m_set[outer][middle].GetSize(); inner++)
        strm << setw(indent+6) << " " << m_set[outer][middle][inner] << '\n';
    }
  }
}


PINDEX H323Capabilities::SetCapability(PINDEX descriptorNum,
                                       PINDEX simultaneousNum,
                                       H323Capability * capability,
                                       H323Capability * before)
{
  // Make sure capability has been added to table.
  Add(capability);

  bool newDescriptor = descriptorNum == P_MAX_INDEX;
  if (newDescriptor)
    descriptorNum = m_set.GetSize();

  // Make sure the outer array is big enough
  m_set.SetMinSize(descriptorNum+1);

  // Set to unique value
  m_set[descriptorNum].m_capabilityDescriptorNumber = 1;
  for (PINDEX i = 0; i < descriptorNum; ++i) {
    if (m_set[i].m_capabilityDescriptorNumber >= m_set[descriptorNum].m_capabilityDescriptorNumber)
      m_set[descriptorNum].m_capabilityDescriptorNumber = m_set[i].m_capabilityDescriptorNumber+1;
  }

  if (simultaneousNum == P_MAX_INDEX)
    simultaneousNum = m_set[descriptorNum].GetSize();

  // Make sure the middle array is big enough
  m_set[descriptorNum].SetMinSize(simultaneousNum+1);

  // Now we can put the new entry in.
  if (before != NULL)
    m_set[descriptorNum][simultaneousNum].Insert(*before, capability);
  else
    m_set[descriptorNum][simultaneousNum].Append(capability);
  return newDescriptor ? descriptorNum : simultaneousNum;
}


static PBoolean MatchWildcard(const PCaselessString & str, const PStringArray & wildcard)
{
  PINDEX last = 0;
  for (PINDEX i = 0; i < wildcard.GetSize(); i++) {
    if (wildcard[i].IsEmpty())
      last = str.GetLength();
    else {
      PINDEX next = str.Find(wildcard[i], last);
      if (next == P_MAX_INDEX)
        return false;
      last = next + wildcard[i].GetLength();
    }
  }

  return last == str.GetLength();
}


PINDEX H323Capabilities::AddMediaFormat(PINDEX descriptorNum,
                                        PINDEX simultaneous,
                                        const OpalMediaFormat & mediaFormat,
                                        H323Capability::CapabilityDirection direction)
{
  PINDEX reply = descriptorNum == P_MAX_INDEX ? P_MAX_INDEX : simultaneous;

  if (!mediaFormat.IsValidForProtocol(PLUGINCODEC_OPTION_PROTOCOL_H323))
    return reply;

  if (FindCapability(mediaFormat, direction, true) != NULL)
    return reply;

  H323Capability * capability = H323Capability::Create(mediaFormat);
  if (capability == NULL)
    return reply;

  capability->SetCapabilityDirection(direction);
  capability->GetWritableMediaFormat() = mediaFormat;
  m_mediaPacketizations.Union(mediaFormat.GetMediaPacketizationSet());

  return SetCapability(descriptorNum, simultaneous, capability);
}


PINDEX H323Capabilities::AddAllCapabilities(PINDEX descriptorNum,
                                            PINDEX simultaneous,
                                            const PString & name,
                                            PBoolean exact)
{
  PINDEX reply = descriptorNum == P_MAX_INDEX ? P_MAX_INDEX : simultaneous;

  PStringArray wildcard = name.Tokenise('*', false);

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
  if (m_table.GetObjectsIndex(capability) != P_MAX_INDEX)
    return;

  capability->SetCapabilityNumber(MergeCapabilityNumber(m_table, 1));
  m_table.Append(capability);

  PTRACE_CONTEXT_ID_TO(capability);

  PTRACE(4, "H323\tAdded capability: " << *capability);
}


H323Capability * H323Capabilities::Copy(const H323Capability & capability)
{
  H323Capability * newCapability = (H323Capability *)capability.Clone();
  newCapability->SetCapabilityNumber(MergeCapabilityNumber(m_table, capability.GetCapabilityNumber()));
  m_table.Append(newCapability);

  PTRACE(4, "H323\tAdded capability: " << *newCapability);
  return newCapability;
}


void H323Capabilities::Remove(H323Capability * capability)
{
  if (capability == NULL)
    return;

  PTRACE(4, "H323\tRemoving capability: " << *capability);

  unsigned capabilityNumber = capability->GetCapabilityNumber();

  for (PINDEX outer = 0; outer < m_set.GetSize(); ) {
    for (PINDEX middle = 0; middle < m_set[outer].GetSize(); ) {
      for (PINDEX inner = 0; inner < m_set[outer][middle].GetSize(); inner++) {
        if (m_set[outer][middle][inner].GetCapabilityNumber() == capabilityNumber) {
          m_set[outer][middle].RemoveAt(inner);
          break;
        }
      }
      if (m_set[outer][middle].GetSize() == 0)
        m_set[outer].RemoveAt(middle);
      else
        middle++;
    }
    if (m_set[outer].GetSize() == 0)
      m_set.RemoveAt(outer);
    else
      outer++;
  }

  m_table.Remove(capability);
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
  m_table.RemoveAll();
  m_set.RemoveAll();
}


H323Capability * H323Capabilities::FindCapability(unsigned capabilityNumber) const
{
  for (PINDEX i = 0; i < m_table.GetSize(); i++) {
    if (m_table[i].GetCapabilityNumber() == capabilityNumber) {
      PTRACE(4, "H323\tFound capability: " << m_table[i]);
      return &m_table[i];
    }
  }

  PTRACE(4, "H323\tCould not find capability: " << capabilityNumber);
  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const PString & formatName,
                                                  H323Capability::CapabilityDirection direction,
                                                  PBoolean exact) const
{
  PStringArray wildcard = formatName.Tokenise('*', false);

  for (PINDEX i = 0; i < m_table.GetSize(); i++) {
    PCaselessString str = m_table[i].GetFormatName();
    if ((exact ? (str == formatName) : MatchWildcard(str, wildcard)) &&
              (direction == H323Capability::e_Unknown || m_table[i].GetCapabilityDirection() == direction)) {
      PTRACE(4, "H323\tFound capability: " << m_table[i]);
      return &m_table[i];
    }
  }

  PTRACE(4, "H323\tCould not find capability: \"" << formatName << '"');
  return NULL;
}


H323Capability * H323Capabilities::FindCapability(
                              H323Capability::CapabilityDirection direction) const
{
  for (PINDEX i = 0; i < m_table.GetSize(); i++) {
    if (m_table[i].GetCapabilityDirection() == direction) {
      PTRACE(4, "H323\tFound capability: " << m_table[i]);
      return &m_table[i];
    }
  }

  PTRACE(4, "H323\tCould not find capability: \"" << direction << '"');
  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const H323Capability & capability) const
{

  for (PINDEX i = 0; i < m_table.GetSize(); i++) {
    if (m_table[i] == capability) {
      PTRACE(4, "H323\tFound capability: " << m_table[i]);
      return &m_table[i];
    }
  }

  PTRACE(4, "H323\tCould not find capability: " << capability);
  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const H245_Capability & cap) const
{
  for (PINDEX i = 0; i < m_table.GetSize(); i++) {
    H323Capability & capability = m_table[i];

    for (PINDEX j = 0; j <= m_mediaPacketizations.GetSize(); ++j) {
      PString mediaPacketization;
      if (j < m_mediaPacketizations.GetSize())
        mediaPacketization = m_mediaPacketizations.GetKeyAt(j);

      switch (cap.GetTag()) {
        case H245_Capability::e_receiveAudioCapability :
        case H245_Capability::e_transmitAudioCapability :
        case H245_Capability::e_receiveAndTransmitAudioCapability :
          if (capability.GetMainType() == H323Capability::e_Audio) {
            const H245_AudioCapability & audio = cap;
            if (capability.IsMatch(audio, mediaPacketization))
              return &capability;
          }
          break;

        case H245_Capability::e_receiveVideoCapability :
        case H245_Capability::e_transmitVideoCapability :
        case H245_Capability::e_receiveAndTransmitVideoCapability :
          if (capability.GetMainType() == H323Capability::e_Video) {
            const H245_VideoCapability & video = cap;
            if (capability.IsMatch(video, mediaPacketization))
              return &capability;
          }
          break;

        case H245_Capability::e_receiveDataApplicationCapability :
        case H245_Capability::e_transmitDataApplicationCapability :
        case H245_Capability::e_receiveAndTransmitDataApplicationCapability :
          if (capability.GetMainType() == H323Capability::e_Data) {
            const H245_DataApplicationCapability & data = cap;
            if (capability.IsMatch(data.m_application, mediaPacketization))
              return &capability;
          }
          break;

        case H245_Capability::e_receiveUserInputCapability :
        case H245_Capability::e_transmitUserInputCapability :
        case H245_Capability::e_receiveAndTransmitUserInputCapability :
          if (capability.GetMainType() == H323Capability::e_UserInput) {
            const H245_UserInputCapability & ui = cap;
            if (capability.IsMatch(ui, mediaPacketization))
              return &capability;
          }
          break;

        case H245_Capability::e_receiveRTPAudioTelephonyEventCapability :
          return FindCapability(H323Capability::e_UserInput, SignalToneRFC2833_SubType);

        case H245_Capability::e_genericControlCapability :
          if (capability.GetMainType() == H323Capability::e_GenericControl) {
            if (capability.IsMatch((const H245_GenericCapability &)cap, mediaPacketization))
              return &capability;
          }
          break;

#if OPAL_H235_6 || OPAL_H235_8
        case H245_Capability::e_h235SecurityCapability :
          if (capability.GetMainType() == H323Capability::e_H235Security) {
            const H245_H235SecurityCapability & h235 = cap;
            if (capability.IsMatch(h235.m_encryptionAuthenticationAndIntegrity, mediaPacketization))
              return &capability;
          }
#endif
      }
    }
  }

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PString tagName;
    switch (cap.GetTag()) {
      case H245_Capability::e_receiveAudioCapability :
      case H245_Capability::e_transmitAudioCapability :
      case H245_Capability::e_receiveAndTransmitAudioCapability :
        tagName = ((const H245_AudioCapability &)cap).GetTagName();
        break;

      case H245_Capability::e_receiveVideoCapability :
      case H245_Capability::e_transmitVideoCapability :
      case H245_Capability::e_receiveAndTransmitVideoCapability :
        tagName = ((const H245_VideoCapability &)cap).GetTagName();
        break;

      case H245_Capability::e_receiveDataApplicationCapability :
      case H245_Capability::e_transmitDataApplicationCapability :
      case H245_Capability::e_receiveAndTransmitDataApplicationCapability :
        tagName = ((const H245_DataApplicationCapability &)cap).m_application.GetTagName();
        break;

      case H245_Capability::e_receiveUserInputCapability :
      case H245_Capability::e_transmitUserInputCapability :
      case H245_Capability::e_receiveAndTransmitUserInputCapability :
        tagName = ((const H245_UserInputCapability &)cap).GetTagName();
        break;

      default :
        tagName = "unknown";
        break;
    }
    PTRACE(4, "H323\tCould not find capability: " << cap.GetTagName() << ", type " << tagName);
  }
#endif
  return NULL;
}


template <class CAP_TYPE>
H323Capability * H323FindMediaCapability(const H323Capabilities & caps,
                                         H323Capability::MainTypes mainType,
                                         const CAP_TYPE & cap,
                                         const PString & mediaPacketization)
{
  for (PINDEX i = 0; i < caps.GetSize(); i++) {
    H323Capability & capability = caps[i];
    if (capability.GetMainType() == mainType && capability.IsMatch(cap, mediaPacketization))
      return &capability;
  }
  return NULL;
}


H323Capability * H323CheckExactCapability(const H245_DataType & dataType, H323Capability * capability)
{
  if (capability != NULL) {
    std::auto_ptr<H323Capability> compare(capability->CloneAs<H323Capability>());
    if (compare->OnReceivedPDU(dataType, false) && *compare == *capability)
      return capability;
  }

  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const H245_DataType & dataType, const PString & mediaPacketization) const
{
  H323Capability * capability = NULL;

  /* Hate special cases ... but this is ... expedient.
     Due to an ambiguity in the TCS syntax, you cannot easily distinguish
     between H.263 and variants such as H,263+. Thus we have to allow for
     if we advertise H.263+ the other side may ask for baseline H.263. So
     we basically need to include all variants of H.263 if the TCS
     capability is there at all.
   */
  if (dataType.GetTag() == H245_DataType::e_videoData &&
      ((const H245_VideoCapability &)dataType).GetTag() == H245_VideoCapability::e_h263VideoCapability &&
       (capability = FindCapability("*H.263*")) != NULL)
    return capability;

  switch (dataType.GetTag()) {
    case H245_DataType::e_audioData :
      capability = H323CheckExactCapability(dataType, H323FindMediaCapability<H245_AudioCapability>(*this, H323Capability::e_Audio, dataType, mediaPacketization));
      break;

    case H245_DataType::e_videoData :
      capability = H323CheckExactCapability(dataType, H323FindMediaCapability<H245_VideoCapability>(*this, H323Capability::e_Video, dataType, mediaPacketization));
      break;

    case H245_DataType::e_data :
      capability = H323CheckExactCapability(dataType, H323FindMediaCapability<>(*this, H323Capability::e_Data, ((const H245_DataApplicationCapability &)dataType).m_application, mediaPacketization));
      break;

#if OPAL_H235_6 || OPAL_H235_8
    case H245_DataType::e_h235Media :
      const H245_H235Media & h235 = (const H245_H235Media &)dataType;
      capability = H323CheckExactCapability(dataType, H323FindMediaCapability<>(*this, H323Capability::e_H235Security, h235.m_encryptionAuthenticationAndIntegrity, mediaPacketization));
      if (capability != NULL) {
        const OpalMediaCryptoSuite * cryptoSuite = NULL;
        {
          const H235SecurityCapability * h235cap = dynamic_cast<const H235SecurityCapability *>(capability);
          if (h235cap != NULL && !h235cap->GetCryptoSuites().empty())
            cryptoSuite = &h235cap->GetCryptoSuites().front();
        }

        switch (h235.m_mediaType.GetTag()) {
          case H245_H235Media_mediaType::e_audioData :
            capability = H323FindMediaCapability<H245_AudioCapability>(*this, H323Capability::e_Audio, h235.m_mediaType, mediaPacketization);
            break;

          case H245_H235Media_mediaType::e_videoData :
            capability = H323FindMediaCapability<H245_VideoCapability>(*this, H323Capability::e_Video, h235.m_mediaType, mediaPacketization);
            break;

          case H245_H235Media_mediaType::e_data :
            capability = H323FindMediaCapability<>(*this, H323Capability::e_Data, ((const H245_DataApplicationCapability &)h235.m_mediaType).m_application, mediaPacketization);
            break;

          default :
            capability = NULL;
        }

        if (capability != NULL) {
          if (cryptoSuite != NULL)
            capability->SetCryptoSuite(*cryptoSuite);
          capability = H323CheckExactCapability(dataType, capability);
        }
      }
      break;
#endif
  }

  if (capability != NULL)
    return capability;

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PString tagName;
    switch (dataType.GetTag()) {
      case H245_DataType::e_audioData :
        tagName = ((const H245_AudioCapability &)dataType).GetTagName();
        break;

      case H245_DataType::e_videoData :
        tagName = ((const H245_VideoCapability &)dataType).GetTagName();
        break;

      case H245_DataType::e_data :
        tagName = ((const H245_DataApplicationCapability &)dataType).m_application.GetTagName();
        break;

      default :
        tagName = "unknown";
        break;
    }
    PTRACE(4, "H323\tCould not find capability: " << dataType.GetTagName() << ", type " << tagName);
  }
#endif
  return NULL;
}


H323Capability * H323Capabilities::FindCapability(const H245_ModeElement & modeElement,
                                                  const PString & mediaPacketization) const
{
  PTRACE(4, "H323\tFindCapability: " << modeElement.m_type.GetTagName());

  for (PINDEX i = 0; i < m_table.GetSize(); i++) {
    H323Capability & capability = m_table[i];
    switch (modeElement.m_type.GetTag()) {
      case H245_ModeElementType::e_audioMode :
        if (capability.GetMainType() == H323Capability::e_Audio) {
          const H245_AudioMode & audio = modeElement.m_type;
          if (capability.IsMatch(audio, mediaPacketization))
            return &capability;
        }
        break;

      case H245_ModeElementType::e_videoMode :
        if (capability.GetMainType() == H323Capability::e_Video) {
          const H245_VideoMode & video = modeElement.m_type;
          if (capability.IsMatch(video, mediaPacketization))
            return &capability;
        }
        break;

      case H245_ModeElementType::e_dataMode :
        if (capability.GetMainType() == H323Capability::e_Data) {
          const H245_DataMode & data = modeElement.m_type;
          if (capability.IsMatch(data.m_application, mediaPacketization))
            return &capability;
        }
        break;

      default :
        break;
    }
  }

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PString tagName;
    switch (modeElement.m_type.GetTag()) {
      case H245_ModeElementType::e_audioMode :
        tagName = ((const H245_AudioMode &)modeElement.m_type).GetTagName();
        break;

      case H245_ModeElementType::e_videoMode :
        tagName = ((const H245_VideoMode &)modeElement.m_type).GetTagName();
        break;

      case H245_ModeElementType::e_dataMode :
        tagName = ((const H245_DataMode &)modeElement.m_type).m_application.GetTagName();
        break;

      default :
        tagName = "unknown";
        break;
    }
    PTRACE(4, "H323\tCould not find capability: " << modeElement.m_type.GetTagName() << ", type " << tagName);
  }
#endif
  return NULL;
}


H323Capability * H323Capabilities::FindCapability(H323Capability::MainTypes mainType,
                                                  unsigned subType) const
{
  for (PINDEX i = 0; i < m_table.GetSize(); i++) {
    H323Capability & capability = m_table[i];
    if (capability.GetMainType() == mainType &&
                        (subType == UINT_MAX || capability.GetSubType() == subType)) {
      PTRACE(4, "H323\tFound capability: " << capability);
      return &capability;
    }
  }

  PTRACE(4, "H323\tCould not find capability: " << mainType << " subtype=" << subType);
  return NULL;
}


void H323Capabilities::BuildPDU(const H323Connection & connection,
                                H245_TerminalCapabilitySet & pdu) const
{
  PINDEX tableSize = m_table.GetSize();
  PINDEX setSize = m_set.GetSize();
  PAssert((tableSize > 0) == (setSize > 0), PLogicError);
  if (tableSize == 0 || setSize == 0)
    return;

  // Set the table of capabilities
  pdu.IncludeOptionalField(H245_TerminalCapabilitySet::e_capabilityTable);

  H245_H2250Capability & h225_0 = pdu.m_multiplexCapability;
  PStringSet mediaPacketizations;

  // encode the capabilities
  PINDEX count = 0;
  PINDEX i;
  for (i = 0; i < tableSize; i++) {
    H323Capability & capability = m_table[i];
    if (capability.IsUsable(connection)) {
      pdu.m_capabilityTable.SetSize(count+1);
      H245_CapabilityTableEntry & entry = pdu.m_capabilityTable[count++];
      entry.m_capabilityTableEntryNumber = capability.GetCapabilityNumber();
      entry.IncludeOptionalField(H245_CapabilityTableEntry::e_capability);
      capability.GetWritableMediaFormat().ToCustomisedOptions();
      if (capability.OnSendingPDU(entry.m_capability))
        mediaPacketizations.Union(capability.GetMediaFormat().GetMediaPacketizationSet());
      else
        pdu.m_capabilityTable.SetSize(--count);
    }
  }

  // Have some mediaPacketizations to include.
  if (H323SetRTPPacketization(h225_0.m_mediaPacketizationCapability.m_rtpPayloadType, mediaPacketizations))
    h225_0.m_mediaPacketizationCapability.IncludeOptionalField(H245_MediaPacketizationCapability::e_rtpPayloadType);

  // Set the sets of compatible capabilities
  pdu.IncludeOptionalField(H245_TerminalCapabilitySet::e_capabilityDescriptors);

  pdu.m_capabilityDescriptors.SetSize(setSize);
  for (PINDEX outer = 0; outer < setSize; outer++) {
    H245_CapabilityDescriptor & desc = pdu.m_capabilityDescriptors[outer];
    desc.m_capabilityDescriptorNumber = m_set[outer].m_capabilityDescriptorNumber;
    desc.IncludeOptionalField(H245_CapabilityDescriptor::e_simultaneousCapabilities);
    PINDEX middleSize = m_set[outer].GetSize();
    desc.m_simultaneousCapabilities.SetSize(middleSize);
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      H245_AlternativeCapabilitySet & alt = desc.m_simultaneousCapabilities[middle];
      PINDEX innerSize = m_set[outer][middle].GetSize();
      alt.SetSize(innerSize);
      count = 0;
      for (PINDEX inner = 0; inner < innerSize; inner++) {
        H323Capability & capability = m_set[outer][middle][inner];
        if (capability.IsUsable(connection)) {
          alt.SetSize(count+1);
          alt[count++] = capability.GetCapabilityNumber();
        }
      }
    }
  }
}


PBoolean H323Capabilities::Merge(const H323Capabilities & newCaps)
{
  PTRACE_IF(4, !m_table.IsEmpty(), "H323\tCapability merge of:\n" << newCaps << "\nInto:\n" << *this);

  // Remove any descriptors we already have, then add them back in.
  for (PINDEX newDesc = 0; newDesc < newCaps.m_set.GetSize(); ++newDesc) {
    for (PINDEX oldDesc = 0; oldDesc < m_set.GetSize(); ++oldDesc) {
      if (newCaps.m_set[newDesc].m_capabilityDescriptorNumber == m_set[oldDesc].m_capabilityDescriptorNumber) {
        m_set.RemoveAt(oldDesc);
        break;
      }
    }
  }

  // Remove any capabilities from old set that are in the new set, then add them back in.
  for (PINDEX newCap = 0; newCap < newCaps.m_table.GetSize(); ++newCap) {
    for (PINDEX oldCap = 0; oldCap < m_table.GetSize(); ++oldCap) {
      if (newCaps.m_table[newCap].assignedCapabilityNumber == m_table[oldCap].assignedCapabilityNumber) {
        Remove(&m_table[oldCap]);
        break;
      }
    }
  }

  // Add any new and replacement capabilities.
  for (PINDEX i = 0; i < newCaps.m_table.GetSize(); i++)
    Copy(newCaps.m_table[i]);

  // Add any new and replacement descriptors.
  PINDEX outerSize = newCaps.m_set.GetSize();
  PINDEX outerBase = m_set.GetSize();
  m_set.SetSize(outerBase+outerSize);
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    PINDEX middleSize = newCaps.m_set[outer].GetSize();
    m_set[outerBase+outer].m_capabilityDescriptorNumber = newCaps.m_set[outer].m_capabilityDescriptorNumber;
    m_set[outerBase+outer].SetSize(middleSize);
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      PINDEX innerSize = newCaps.m_set[outer][middle].GetSize();
      for (PINDEX inner = 0; inner < innerSize; inner++) {
        H323Capability * cap = FindCapability(newCaps.m_set[outer][middle][inner].GetCapabilityNumber());
        if (cap != NULL)
          m_set[outerBase+outer][middle].Append(cap);
      }
    }
  }

  return !m_table.IsEmpty();
}


void H323Capabilities::Reorder(const PStringArray & preferenceOrder)
{
  if (preferenceOrder.IsEmpty())
    return;

  m_table.DisallowDeleteObjects();

  PINDEX preference = 0;
  PINDEX base = 0;

  for (preference = 0; preference < preferenceOrder.GetSize(); preference++) {
    PStringArray wildcard = preferenceOrder[preference].Tokenise('*', false);
    for (PINDEX idx = base; idx < m_table.GetSize(); idx++) {
      PCaselessString str = m_table[idx].GetFormatName();
      if (MatchWildcard(str, wildcard)) {
        if (idx != base)
          m_table.InsertAt(base, m_table.RemoveAt(idx));
        base++;
      }
    }
  }

  for (PINDEX outer = 0; outer < m_set.GetSize(); outer++) {
    for (PINDEX middle = 0; middle < m_set[outer].GetSize(); middle++) {
      H323CapabilitiesList & list = m_set[outer][middle];
      for (PINDEX idx = 0; idx < m_table.GetSize(); idx++) {
        for (PINDEX inner = 0; inner < list.GetSize(); inner++) {
          if (&m_table[idx] == &list[inner]) {
            list.Append(list.RemoveAt(inner));
            break;
          }
        }
      }
    }
  }

  m_table.AllowDeleteObjects();
}


PBoolean H323Capabilities::IsAllowed(const H323Capability & capability)
{
  return IsAllowed(capability.GetCapabilityNumber());
}


PBoolean H323Capabilities::IsAllowed(const unsigned a_capno)
{
  // Check that capno is actually in the set
  PINDEX outerSize = m_set.GetSize();
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    PINDEX middleSize = m_set[outer].GetSize();
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      PINDEX innerSize = m_set[outer][middle].GetSize();
      for (PINDEX inner = 0; inner < innerSize; inner++) {
        if (a_capno == m_set[outer][middle][inner].GetCapabilityNumber()) {
          return true;
        }
      }
    }
  }
  return false;
}


PBoolean H323Capabilities::IsAllowed(const H323Capability & capability1,
                                 const H323Capability & capability2)
{
  return IsAllowed(capability1.GetCapabilityNumber(),
                   capability2.GetCapabilityNumber());
}


PBoolean H323Capabilities::IsAllowed(const unsigned a_capno1, const unsigned a_capno2)
{
  if (a_capno1 == a_capno2) {
    PTRACE(2, "H323\tH323Capabilities::IsAllowed() capabilities are the same.");
    return true;
  }

  PINDEX outerSize = m_set.GetSize();
  for (PINDEX outer = 0; outer < outerSize; outer++) {
    PINDEX middleSize = m_set[outer].GetSize();
    for (PINDEX middle = 0; middle < middleSize; middle++) {
      PINDEX innerSize = m_set[outer][middle].GetSize();
      for (PINDEX inner = 0; inner < innerSize; inner++) {
        if (a_capno1 == m_set[outer][middle][inner].GetCapabilityNumber()) {
          /* Now go searching for the other half... */
          for (PINDEX middle2 = 0; middle2 < middleSize; ++middle2) {
            if (middle != middle2) {
              PINDEX innerSize2 = m_set[outer][middle2].GetSize();
              for (PINDEX inner2 = 0; inner2 < innerSize2; ++inner2) {
                if (a_capno2 == m_set[outer][middle2][inner2].GetCapabilityNumber()) {
                  return true;
                }
              }
            }
          }
        }
      }
    }
  }
  return false;
}


OpalMediaFormatList H323Capabilities::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  for (PINDEX i = 0; i < m_table.GetSize(); i++) {
    OpalMediaFormat fmt = m_table[i].GetMediaFormat();
#if 0 // Yep, proved unworkable!
    OpalMediaFormatList::const_iterator it = formats.FindFormat(fmt);
    if (it != formats.end()) {
      /* We really should create new OpalMediaFormat entries "on the fly" for
         each H.239 version of a codec. Bit for now we just get a lowest common
         denominator, until it proves unworkable. */
      fmt.Merge(*it);
      formats.erase(it);
    }
#endif
    formats += fmt;
  }

  // Reorder to first entry, really should be selected entry, but we don't have that
  if (!m_set.IsEmpty()) {
    PStringArray order;
    for (PINDEX middle = 0;  middle < m_set[0].GetSize(); ++middle) {
      for (PINDEX inner = 0; inner < m_set[0][middle].GetSize(); ++inner) {
        PString name = m_set[0][middle][inner].GetMediaFormat().GetName();
        if (order.GetValuesIndex(name) == P_MAX_INDEX)
          order += name;
      }
    }

    formats.Reorder(order);
  }

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
  if (GetTag() == 0) {

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
              name = PString(data);
            strm << " [Equivalence " << name << "]";
          }

          // Xiph is 181/0/38
          else if ((h221.m_t35CountryCode   == 181) &&
                   (h221.m_t35Extension     == 0) &&
                   (h221.m_manufacturerCode == 38)
                  ) {
            PString name;
            if (data.GetSize() > 0)
              name = PString(data);
            strm << " [Xiph " << name << "]";
          }

          // Cisco is 181/0/18
          else if ((h221.m_t35CountryCode   == 181) &&
                   (h221.m_t35Extension     == 0) &&
                   (h221.m_manufacturerCode == 18)
                  ) {
            PString name;
            if (data.GetSize() > 0)
              name = PString(data);
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

#endif // PASN_NOPRINTON

#endif // OPAL_H323
