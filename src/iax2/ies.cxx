/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the classes to carry Information Elements
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 *
 *
 * $Log: ies.cxx,v $
 * Revision 1.1  2005/07/30 07:01:33  csoutheren
 * Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 * Thanks to Derek Smithies of Indranet Technologies Ltd. for
 * writing and contributing this code
 *
 *
 *
 */


#include <ptlib.h>
#include <ptclib/cypher.h>


#ifdef P_USE_PRAGMA
#pragma implementation "ies.h"
#endif


#include <iax2/ies.h>
#include <iax2/frame.h>
#include <iax2/causecode.h>

#define new PNEW

Ie * Ie::BuildInformationElement(BYTE _typeCode, BYTE length, BYTE *srcData)
{
  switch (_typeCode) {
  case ie_calledNumber    : return new IeCalledNumber(length, srcData);
  case ie_callingNumber   : return new IeCallingNumber(length, srcData);
  case ie_callingAni      : return new IeCallingAni(length, srcData);
  case ie_callingName     : return new IeCallingName(length, srcData);
  case ie_calledContext   : return new IeCalledContext(length, srcData);
  case ie_userName        : return new IeUserName(length, srcData);
  case ie_password        : return new IePassword(length, srcData);
  case ie_capability      : return new IeCapability(length, srcData);
  case ie_format          : return new IeFormat(length, srcData);
  case ie_language        : return new IeLanguage(length, srcData);
  case ie_version         : return new IeVersion(length, srcData);
  case ie_adsicpe         : return new IeAdsicpe(length, srcData);
  case ie_dnid            : return new IeDnid(length, srcData);
  case ie_authMethods     : return new IeAuthMethods(length, srcData);
  case ie_challenge       : return new IeChallenge(length, srcData);
  case ie_md5Result       : return new IeMd5Result(length, srcData);
  case ie_rsaResult       : return new IeRsaResult(length, srcData);
  case ie_apparentAddr    : return new IeApparentAddr(length, srcData);
  case ie_refresh         : return new IeRefresh(length, srcData);
  case ie_dpStatus        : return new IeDpStatus(length, srcData);
  case ie_callNo          : return new IeCallNo(length, srcData);
  case ie_cause           : return new IeCause(length, srcData);
  case ie_iaxUnknown      : return new IeIaxUnknown(length, srcData);
  case ie_msgCount        : return new IeMsgCount(length, srcData);
  case ie_autoAnswer      : return new IeAutoAnswer(length, srcData);
  case ie_musicOnHold     : return new IeMusicOnHold(length, srcData);
  case ie_transferId      : return new IeTransferId(length, srcData);
  case ie_rdnis           : return new IeRdnis(length, srcData);
  case ie_provisioning    : return new IeProvisioning(length, srcData);
  case ie_aesProvisioning : return new IeAesProvisioning(length, srcData);
  case ie_dateTime        : return new IeDateTime(length, srcData);
  case ie_deviceType      : return new IeDeviceType(length, srcData);
  case ie_serviceIdent    : return new IeServiceIdent(length, srcData);
  case ie_firmwareVer     : return new IeFirmwareVer(length, srcData);
  case ie_fwBlockDesc     : return new IeFwBlockDesc(length, srcData);
  case ie_fwBlockData     : return new IeFwBlockData(length, srcData);
  case ie_provVer         : return new IeProvVer(length, srcData);
  case ie_callingPres     : return new IeCallingPres(length, srcData);
  case ie_callingTon      : return new IeCallingTon(length, srcData);
  case ie_callingTns      : return new IeCallingTns(length, srcData);
  case ie_samplingRate    : return new IeSamplingRate(length, srcData);
  case ie_causeCode       : return new IeCauseCode(length, srcData);
  case ie_encryption      : return new IeEncryption(length, srcData);
  case ie_encKey          : return new IeEncKey(length, srcData);
  case ie_codecPrefs      : return new IeCodecPrefs(length, srcData);
  case ie_recJitter       : return new IeReceivedJitter(length, srcData);
  case ie_recLoss         : return new IeReceivedLoss(length, srcData);
  case ie_recPackets      : return new IeDroppedFrames(length, srcData);
  case ie_recDelay        : return new IeReceivedDelay(length, srcData);
  case ie_recDropped      : return new IeDroppedFrames(length, srcData);
  case ie_recOoo          : return new IeReceivedOoo(length, srcData);
    
  default: PTRACE(1, "Ie\t Invalid IE type code " << ::hex << ((int)_typeCode) << ::dec);
  };
  
  return new IeInvalidElement();
}

Ie::Ie()
{
  validData = FALSE;
}


void Ie::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " information element " ;
  else
    str << setw(17) << Class() << " information element-invalid data " ;
}


void Ie::WriteBinary(void *_data, PINDEX &writeIndex)
{
  BYTE *data = (BYTE *)_data;
  data[writeIndex] = GetKeyValue();
  data[writeIndex + 1] = GetLengthOfData();
  
  writeIndex +=2;
  
  WriteBinary(data + writeIndex);
  writeIndex += GetLengthOfData();
}

////////////////////////////////////////////////////////////////////////////////

IeNone::IeNone(BYTE /*length*/, BYTE * /*srcData*/)
{
  validData = TRUE;
}


void IeNone::PrintOn(ostream & str) const
{
  str << setw(17) << Class();
}

////////////////////////////////////////////////////////////////////////////////

IeDateAndTime::IeDateAndTime(BYTE length, BYTE *srcData)
{
  if (length != sizeof(unsigned int)) {
    validData = FALSE;
    return;
  }
  
  unsigned int tmp = (srcData[0] << 24) | (srcData[1] << 16) | (srcData[2] << 8) | (srcData[3]);
  int second = (tmp & 0x1f) << 1;
  int minute = (tmp >> 5) & 0x3f;
  int hour   = (tmp >> 11) & 0x1f;
  int day    = (tmp >> 16) & 0x1f;
  int month  = (tmp >> 21) & 0x0f;
  int year   = ((tmp >> 25) & 0x7f) + 2000;
  dataValue = PTime(second, minute, hour, day, month, year, PTime::Local);
  
  validData = TRUE;
}


void IeDateAndTime::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << dataValue;
}

void IeDateAndTime::WriteBinary(BYTE *data)
{
  unsigned int second = dataValue.GetSecond() >> 1;
  unsigned int minute = dataValue.GetMinute()        << 5;
  unsigned int hour   = dataValue.GetHour()          << 11;
  unsigned int day    = dataValue.GetDay()           << 16;
  unsigned int month  = dataValue.GetMonth()         << 21;
  unsigned int year   = ((unsigned int)(dataValue.GetYear() - 2000)) << 25;
  
  unsigned int res = second | minute | hour | day | month | year;
  data[0] = (BYTE)((res >> 24) & 0xff);
  data[1] = (BYTE)((res >> 16) & 0xff);
  data[2] = (BYTE)((res >>  8) & 0xff);
  data[3] = (BYTE)((res      ) & 0xff);
}
////////////////////////////////////////////////////////////////////////////////

IeByte::IeByte(BYTE length, BYTE *srcData)
  : Ie()
{
  if (length != sizeof(BYTE)) {
    validData = FALSE;
    return;
  }
  
  validData = TRUE;
  dataValue = *((BYTE *)srcData);
}


void IeByte::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((int) dataValue);
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

////////////////////////////////////////////////////////////////////////////////
IeChar::IeChar(BYTE length, BYTE *srcData)
  : Ie()
{
  if (length != sizeof(BYTE)) {
    validData = FALSE;
    return;
  }
  
  validData = TRUE;
  dataValue = *((char *)srcData);
}


void IeChar::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

////////////////////////////////////////////////////////////////////////////////

IeUShort::IeUShort(BYTE length, BYTE *srcData)
  : Ie()
{
  if (length != sizeof(unsigned short)) {
    validData = FALSE;
    return;
  }
  
  validData = TRUE;
  dataValue = (unsigned short)((srcData[1] << 8) | srcData[0]);
}


void IeUShort::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IeUShort::WriteBinary(BYTE *data)
{
  data[0] = (BYTE)((dataValue >> 8) & 0xff);
  data[1] = (BYTE)(dataValue & 0xff);
}
////////////////////////////////////////////////////////////////////////////////

IeShort::IeShort(BYTE length, BYTE *srcData)
  : Ie()
{
  if (length != sizeof(short)) {
    validData = FALSE;
    return;
  }
  
  validData = TRUE;
  dataValue = (short)((srcData[0] << 8) | srcData[1]);
}


void IeShort::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IeShort::WriteBinary(BYTE *data)
{
  data[0] = (BYTE)((dataValue >> 8) & 0xff);
  data[1] = (BYTE)(dataValue & 0xff);
}
////////////////////////////////////////////////////////////////////////////////

IeInt::IeInt(BYTE length, BYTE *srcData)
  : Ie()
{
  if (length != sizeof(int)) {
    validData = FALSE;
    return;
  }
  
  validData = TRUE;
  dataValue = (srcData[0] << 24) | (srcData[1] << 16) | (srcData[2] << 8) | srcData[3];
}


void IeInt::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IeInt::WriteBinary(BYTE *data)
{
  data[0] = (BYTE)((dataValue >> 24) & 0xff);
  data[1] = (BYTE)((dataValue >> 16) & 0xff);
  data[2] = (BYTE)((dataValue >> 8) & 0xff);
  data[3] = (BYTE)(dataValue & 0xff);
}

////////////////////////////////////////////////////////////////////////////////

IeUInt::IeUInt(BYTE length, BYTE *srcData)
  : Ie()
{
  if (length != sizeof(unsigned int)) {
    validData = FALSE;
    return;
  }
  
  validData = TRUE;
  dataValue = (srcData[0] << 24) | (srcData[1] << 16) | (srcData[2] << 8) | srcData[3];     
}


void IeUInt::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IeUInt::WriteBinary(BYTE *data)
{
  data[0] = (BYTE)((dataValue >> 24) & 0xff);
  data[1] = (BYTE)((dataValue >> 16) & 0xff);
  data[2] = (BYTE)((dataValue >> 8) & 0xff);
  data[3] = (BYTE)(dataValue & 0xff);
}
////////////////////////////////////////////////////////////////////////////////

IeString::IeString(BYTE length, BYTE *srcData)
  : Ie()
{
  validData = TRUE;
  dataValue = PString((const char *)srcData, length);
}


void IeString::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IeString::WriteBinary(BYTE *data)
{
  if(validData)
    memcpy(data, dataValue.GetPointer(), GetLengthOfData());
}

BYTE IeString::GetLengthOfData() 
{ 
  if (dataValue.GetSize() == 0)
    return 0;
  else 
    return (BYTE)(dataValue.GetSize() - 1); 
}

void IeString::SetData(PString & newData) 
{ 
  dataValue = newData; 
  validData = TRUE; 
}

void IeString::SetData(const char * newData) 
{ 
  dataValue = PString(newData); 
  validData = TRUE; 
}

////////////////////////////////////////////////////////////////////////////////

IeSockaddrIn::IeSockaddrIn(BYTE length, BYTE *srcData)
  : Ie()
{
  if (length != sizeof(sockaddr_in)) {
    validData = FALSE;
    return;
  }
  
  validData = TRUE;
  
  sockaddr_in a = * (sockaddr_in *)(srcData);
  portNumber = a.sin_port;
  
  dataValue = PIPSocket::Address(a.sin_addr);
}


void IeSockaddrIn::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue << ":" << portNumber;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IeSockaddrIn::WriteBinary(BYTE *data)
{
  sockaddr_in a;
  a.sin_addr = (in_addr)dataValue;
  a.sin_port = (unsigned short)portNumber;
  
  *((sockaddr_in *)data) = a;
}

////////////////////////////////////////////////////////////////////////////////

IeBlockOfData::IeBlockOfData(BYTE length, BYTE *srcData)
  : Ie()
{
  validData = TRUE;
  
  dataValue = PBYTEArray(srcData, length);
}


void IeBlockOfData::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << " " << dataValue;
}


void IeBlockOfData::WriteBinary(BYTE *data)
{
  memcpy(data, dataValue.GetPointer(), dataValue.GetSize());
}


////////////////////////////////////////////////////////////////////////////////

IeList::~IeList()
{
    AllowDeleteObjects();
}

Ie *IeList::RemoveIeAt(PINDEX i)
{ 
  if (i >= GetSize())
    return NULL;
  
  return (Ie *) PAbstractList::RemoveAt(i);
}

Ie *IeList::RemoveLastIe()
{
  PINDEX elems = PAbstractList::GetSize();
  if (elems > 0) {
    return RemoveIeAt(elems - 1);	  
  }
  
  return NULL;
}

void IeList::DeleteAt(PINDEX idex)
{
  if (idex >= PAbstractList::GetSize())
    return;
  
  Ie *obj = RemoveIeAt(idex);
  
  delete obj;
}

int IeList::GetBinaryDataSize()
{
  PINDEX totalSize = 0;
  for(PINDEX i = 0; i < PAbstractList::GetSize(); i++)
    totalSize += GetIeAt(i)->GetBinarySize();
  
  return totalSize;
}

Ie *IeList::GetIeAt(int i)
{
  if (i >= GetSize())
    return NULL;
  
  return (Ie *)GetAt(i); 
}

////////////////////////////////////////////////////////////////////////////////

void IeCalledNumber::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCallingNumber::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCallingAni::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCallingName::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCalledContext::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeUserName::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IePassword::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCapability::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeFormat::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeLanguage::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeVersion::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeAdsicpe::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeDnid::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeAuthMethods::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeChallenge::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

IeMd5Result::IeMd5Result(PString challenge, PString password)
{
  PMessageDigest5 stomach;
  stomach.Process(challenge);
  stomach.Process(password);
  PMessageDigest5::Code digester;
  stomach.Complete(digester);
  
  PStringStream res;
  for (PINDEX i = 0; i < (PINDEX)sizeof(digester); i++)
    res  << ::hex << ::setfill('0') << ::setw(2) << (int)(*(((BYTE *)&digester)+i));
  
  res.Trim();
  res.MakeMinimumSize();
  
  SetData(res);
}

void IeMd5Result::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeRsaResult::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeApparentAddr::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeRefresh::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeDpStatus::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCallNo::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCause::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " \"" << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeIaxUnknown::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeMsgCount::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeAutoAnswer::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << "   key(" << ((int) GetKeyValue()) << ")";
}
////////////////////////////////////////////////////////////////////////////////

void IeMusicOnHold::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << "    key(" << ((int) GetKeyValue()) << ")";
}
////////////////////////////////////////////////////////////////////////////////

void IeTransferId::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeRdnis::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeProvisioning::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeAesProvisioning::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << "   key(" << ((int) GetKeyValue()) << ")";
}
////////////////////////////////////////////////////////////////////////////////

void IeDateTime::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeDeviceType::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeServiceIdent::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeFirmwareVer::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeFwBlockDesc::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeFwBlockData::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeProvVer::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCallingPres::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCallingTon::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCallingTns::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeSamplingRate::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeEncryption::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeEncKey::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeCodecPrefs::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////
void IeCauseCode::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeReceivedJitter::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}

////////////////////////////////////////////////////////////////////////////////

void IeReceivedLoss::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////
void IeReceivedFrames::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeReceivedDelay::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned short)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}

////////////////////////////////////////////////////////////////////////////////

void IeDroppedFrames::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IeReceivedOoo::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */

