/*
 * q931.cxx
 *
 * Q.931 protocol handler
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
 * $Log: q931.cxx,v $
 * Revision 1.2002  2001/08/13 05:10:40  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.36  2001/08/07 02:57:09  robertj
 * Fixed incorrect Q.931 bearer capability, thanks Carlo Kielstra.
 *
 * Revision 1.35  2001/07/24 23:40:15  craigs
 * Added ability to remove Q931 IE
 *
 * Revision 1.34  2001/06/14 06:25:16  robertj
 * Added further H.225 PDU build functions.
 * Moved some functionality from connection to PDU class.
 *
 * Revision 1.33  2001/05/30 04:38:40  robertj
 * Added BuildStatusEnquiry() Q.931 function, thanks Markus Storm
 *
 * Revision 1.32  2001/04/05 00:06:31  robertj
 * Fixed some more encoding/decoding problems with little used bits of
 *   the Q.931 protocol, thanks Hans Verbeek.
 *
 * Revision 1.31  2001/04/03 23:06:15  robertj
 * Fixed correct encoding and decoding of Q.850 cause field, thanks Hans Verbeek.
 *
 * Revision 1.30  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.29  2001/01/19 06:57:26  robertj
 * Added Information message type.
 *
 * Revision 1.28  2000/10/13 02:16:04  robertj
 * Added support for Progress Indicator Q.931/H.225 message.
 *
 * Revision 1.27  2000/07/11 11:17:01  robertj
 * Improved trace log display of Q.931 PDU's (correct order and extra IE fields).
 *
 * Revision 1.26  2000/07/09 14:54:11  robertj
 * Added facility IE to facility message.
 * Changed reference to the word "field" to be more correct IE or "Information Element"
 *
 * Revision 1.25  2000/06/21 08:07:47  robertj
 * Added cause/reason to release complete PDU, where relevent.
 *
 * Revision 1.24  2000/05/09 12:19:31  robertj
 * Added ability to get and set "distinctive ring" Q.931 functionality.
 *
 * Revision 1.23  2000/05/08 14:07:35  robertj
 * Improved the provision and detection of calling and caller numbers, aliases and hostnames.
 *
 * Revision 1.22  2000/05/06 02:18:26  robertj
 * Changed the new CallingPartyNumber code so defaults for octet3a are application dependent.
 *
 * Revision 1.21  2000/05/05 00:44:05  robertj
 * Added presentation and screening fields to Calling Party Number field, thanks Dean Anderson.
 *
 * Revision 1.20  2000/05/02 04:32:27  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.19  2000/03/21 01:08:11  robertj
 * Fixed incorrect call reference code being used in originated call.
 *
 * Revision 1.18  2000/02/17 12:07:43  robertj
 * Used ne wPWLib random number generator after finding major problem in MSVC rand().
 *
 * Revision 1.17  1999/12/23 22:44:06  robertj
 * Added calling party number field.
 *
 * Revision 1.16  1999/09/22 04:18:29  robertj
 * Fixed missing "known" message types in debugging output.
 *
 * Revision 1.15  1999/09/10 03:36:48  robertj
 * Added simple Q.931 Status response to Q.931 Status Enquiry
 *
 * Revision 1.14  1999/08/31 13:54:35  robertj
 * Fixed problem with memory overrun building PDU's
 *
 * Revision 1.13  1999/08/31 12:34:19  robertj
 * Added gatekeeper support.
 *
 * Revision 1.12  1999/08/13 06:34:38  robertj
 * Fixed problem in CallPartyNumber Q.931 encoding.
 * Added field name display to Q.931 protocol.
 *
 * Revision 1.11  1999/08/10 13:14:15  robertj
 * Added Q.931 Called Number field if have "phone number" style destination addres.
 *
 * Revision 1.10  1999/07/16 02:15:30  robertj
 * Fixed more tunneling problems.
 *
 * Revision 1.9  1999/07/09 14:59:59  robertj
 * Fixed GNU C++ compatibility.
 *
 * Revision 1.8  1999/07/09 06:09:50  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.7  1999/06/14 15:19:48  robertj
 * GNU C compatibility
 *
 * Revision 1.6  1999/06/13 12:41:14  robertj
 * Implement logical channel transmitter.
 * Fixed H245 connect on receiving call.
 *
 * Revision 1.5  1999/06/09 05:26:20  robertj
 * Major restructuring of classes.
 *
 * Revision 1.4  1999/02/23 11:04:29  robertj
 * Added capability to make outgoing call.
 *
 * Revision 1.3  1999/01/16 01:31:38  robertj
 * Major implementation.
 *
 * Revision 1.2  1999/01/02 04:00:52  robertj
 * Added higher level protocol negotiations.
 *
 * Revision 1.1  1998/12/14 09:13:37  robertj
 * Initial revision
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "q931.h"
#endif

#include <h323/q931.h>

#include <ptclib/random.h>


#define new PNEW


Q931::Q931()
{
  protocolDiscriminator = 8;  // Q931 always has 00001000
  messageType = NationalEscapeMsg;
  fromDestination = FALSE;
  callReference = 0;
}


Q931 & Q931::operator=(const Q931 & other)
{
  protocolDiscriminator = other.protocolDiscriminator;
  callReference = other.callReference;
  fromDestination = other.fromDestination;
  messageType = other.messageType;

  informationElements.RemoveAll();
  for (PINDEX i = 0; i < other.informationElements.GetSize(); i++)
    informationElements.SetAt(other.informationElements.GetKeyAt(i), new PBYTEArray(other.informationElements.GetDataAt(i)));

  return *this;
}


void Q931::BuildFacility(int callRef, BOOL fromDest)
{
  messageType = FacilityMsg;
  callReference = callRef;
  fromDestination = fromDest;
  informationElements.RemoveAll();
  PBYTEArray data;
  SetIE(FacilityIE, data);
}


void Q931::BuildInformation(int callRef, BOOL fromDest)
{
  messageType = InformationMsg;
  callReference = callRef;
  fromDestination = fromDest;
  informationElements.RemoveAll();
}


void Q931::BuildProgress(int callRef,
                         BOOL fromDest,
                         unsigned description,
                         unsigned codingStandard,
                         unsigned location)
{
  messageType = ProgressMsg;
  callReference = callRef;
  fromDestination = fromDest;
  informationElements.RemoveAll();
  SetProgressIndicator(description, codingStandard, location);
}


void Q931::BuildNotify(int callRef, BOOL fromDest)
{
  messageType = NotifyMsg;
  callReference = callRef;
  fromDestination = fromDest;
  informationElements.RemoveAll();
}


void Q931::BuildSetupAcknowledge(int callRef)
{
  messageType = SetupAckMsg;
  callReference = callRef;
  fromDestination = TRUE;
  informationElements.RemoveAll();
}


void Q931::BuildCallProceeding(int callRef)
{
  messageType = CallProceedingMsg;
  callReference = callRef;
  fromDestination = TRUE;
  informationElements.RemoveAll();
}


void Q931::BuildAlerting(int callRef)
{
  messageType = AlertingMsg;
  callReference = callRef;
  fromDestination = TRUE;
  informationElements.RemoveAll();
}


static const BYTE BearerCapabilityData[3] = { 0x88, 0x80, 0xa5 };

void Q931::BuildSetup(int callRef)
{
  messageType = SetupMsg;
  if (callRef < 0)
    GenerateCallReference();
  else
    callReference = callRef;
  fromDestination = FALSE;
  informationElements.RemoveAll();
  SetIE(BearerCapabilityIE, PBYTEArray(BearerCapabilityData, sizeof(BearerCapabilityData)));
}


void Q931::BuildConnect(int callRef)
{
  messageType = ConnectMsg;
  callReference = callRef;
  fromDestination = TRUE;
  informationElements.RemoveAll();
  SetIE(BearerCapabilityIE, PBYTEArray(BearerCapabilityData, sizeof(BearerCapabilityData)));
}


void Q931::BuildStatus(int callRef, BOOL fromDest)
{
  messageType = StatusMsg;
  callReference = callRef;
  fromDestination = fromDest;
  informationElements.RemoveAll();
  PBYTEArray data(1);

  // Call State as per Q.931 section 4.5.7
  data[0] = 0; // Use CCITT Null
  SetIE(CallStateIE, data);

  // Cause field as per Q.850
  SetCause(StatusEnquiryResponse);
}


void Q931::BuildStatusEnquiry(int callRef, BOOL fromDest)
{
  messageType = StatusEnquiryMsg;
  callReference = callRef;
  fromDestination = fromDest;
  informationElements.RemoveAll();
}


void Q931::BuildReleaseComplete(int callRef, BOOL fromDest)
{
  messageType = ReleaseCompleteMsg;
  callReference = callRef;
  fromDestination = fromDest;
  informationElements.RemoveAll();
}


BOOL Q931::Decode(const PBYTEArray & data)
{
  // Clear all existing data before reading new
  informationElements.RemoveAll();

  if (data.GetSize() < 5) // Packet too short
    return FALSE;

  protocolDiscriminator = data[0];

  if (data[1] != 2) // Call reference must be 2 bytes long
    return FALSE;

  callReference = ((data[2]&0x7f) << 8) | data[3];
  fromDestination = (data[2]&0x80) != 0;

  messageType = (MsgTypes)data[4];

  // Have preamble, start getting the informationElements into buffers
  PINDEX offset = 5;
  while (offset < data.GetSize()) {
    // Get field discriminator
    int discriminator = data[offset++];

    PBYTEArray * item = new PBYTEArray;

    // For discriminator with high bit set there is no data
    if ((discriminator&0x80) == 0) {
      int len = data[offset++];

      if (discriminator == UserUserIE) {
        // Special case of User-user field, there is some confusion here as
        // the Q931 documentation claims the length is a single byte,
        // unfortunately all H.323 based apps have a 16 bit length here, so
        // we allow for said longer length. There is presumably an addendum
        // to Q931 which describes this, and provides a means to discriminate
        // between the old 1 byte and the new 2 byte systems. However, at
        // present we assume it is always 2 bytes until we find something that
        // breaks it.
        len <<= 8;
        len |= data[offset++];

        // we also have a protocol discriminator, which we ignore
        offset++;
        len--;
      }

      if (offset + len > data.GetSize())
        return FALSE;

      memcpy(item->GetPointer(len), (const BYTE *)data+offset, len);
      offset += len;
    }

    informationElements.SetAt(discriminator, item);
  }

  return TRUE;
}


BOOL Q931::Encode(PBYTEArray & data) const
{
  PINDEX totalBytes = 5;
  unsigned discriminator;
  for (discriminator = 0; discriminator < 256; discriminator++) {
    if (informationElements.Contains(discriminator)) {
      if (discriminator < 128)
        totalBytes += informationElements[discriminator].GetSize() +
                            (discriminator != UserUserIE ? 2 : 4);
      else
        totalBytes++;
    }
  }

  if (!data.SetMinSize(totalBytes))
    return FALSE;

  // Put in Q931 header
  PAssert(protocolDiscriminator < 256, PInvalidParameter);
  data[0] = (BYTE)protocolDiscriminator;
  data[1] = 2; // Length of call reference
  data[2] = (BYTE)(callReference >> 8);
  if (fromDestination)
    data[2] |= 0x80;
  data[3] = (BYTE)callReference;
  PAssert(messageType < 256, PInvalidParameter);
  data[4] = (BYTE)messageType;

  // The following assures disciminators are in ascending value order
  // as required by Q931 specification
  PINDEX offset = 5;
  for (discriminator = 0; discriminator < 256; discriminator++) {
    if (informationElements.Contains(discriminator)) {
      if (discriminator < 128) {
        int len = informationElements[discriminator].GetSize();

        if (discriminator != UserUserIE) {
          data[offset++] = (BYTE)discriminator;
          data[offset++] = (BYTE)len;
        }
        else {
          len++; // Allow for protocol discriminator
          data[offset++] = (BYTE)discriminator;
          data[offset++] = (BYTE)(len >> 8);
          data[offset++] = (BYTE)len;
          len--; // Then put the length back again
          // We shall assume that the user-user field is an ITU protocol block (5)
          data[offset++] = 5;
        }

        memcpy(&data[offset], (const BYTE *)informationElements[discriminator], len);
        offset += len;
      }
      else
        data[offset++] = (BYTE)discriminator;
    }
  }

  return data.SetSize(offset);
}


static PString Q931IENameString(int number)
{
  switch (number) {
    case Q931::BearerCapabilityIE :
      return "Bearer-Capability";
    case Q931::CauseIE :
      return "Cause";
    case Q931::FacilityIE :
      return "Facility";
    case Q931::ProgressIndicatorIE :
      return "Progress-Indicator";
    case Q931::CallStateIE :
      return "Call-State";
    case Q931::DisplayIE :
      return "Display";
    case Q931::SignalIE :
      return "Signal";
    case Q931::CallingPartyNumberIE :
      return "Calling-Party-Number";
    case Q931::CalledPartyNumberIE :
      return "Called-Party-Number";
    case Q931::UserUserIE :
      return "User-User";
  }
  return psprintf("0x%02x", number);
}


void Q931::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n"
       << setw(indent+24) << "protocolDiscriminator = " << protocolDiscriminator << '\n'
       << setw(indent+16) << "callReference = " << callReference << '\n'
       << setw(indent+7)  << "from = " << (fromDestination ? "destination" : "originator") << '\n'
       << setw(indent+14) << "messageType = " << GetMessageTypeName() << '\n';

  for (unsigned discriminator = 0; discriminator < 256; discriminator++)
    if (informationElements.Contains(discriminator))
      strm << setw(indent+4)
           << "IE: " << Q931IENameString(discriminator) << " = {\n"
           << hex << setfill('0')
           << setprecision(indent+2) << informationElements[discriminator] << '\n'
           << dec << setfill(' ')
           << setw(indent+2) << "}\n";

  strm << setw(indent-1) << "}";
}


PString Q931::GetMessageTypeName() const
{
  switch (messageType) {
    case AlertingMsg :
      return "Alerting";
    case CallProceedingMsg :
      return "CallProceeding";
    case ConnectMsg :
      return "Connect";
    case ConnectAckMsg :
      return "ConnectAck";
    case ProgressMsg :
      return "Progress";
    case SetupMsg :
      return "Setup";
    case SetupAckMsg :
      return "SetupAck";
    case FacilityMsg :
      return "Facility";
    case ReleaseCompleteMsg :
      return "ReleaseComplete";
    case StatusEnquiryMsg :
      return "StatusEnquiry";
    case StatusMsg :
      return "Status";
    case InformationMsg :
      return "Information";
    case NationalEscapeMsg :
      return "Escape";
    default :
      break;
  }

  return psprintf("<%u>", messageType);
}


void Q931::GenerateCallReference()
{
  static const char LastCallReference[] = "Last Call Reference";

  PConfig cfg("Globals");
  callReference = cfg.GetInteger(LastCallReference);

  if (callReference == 0)
    callReference = PRandom::Number();
  else
    callReference++;

  callReference &= 0x7fff;

  if (callReference == 0)
    callReference = 1;

  cfg.SetInteger(LastCallReference, callReference);
}


BOOL Q931::HasIE(InformationElementCodes ie) const
{
  return informationElements.Contains(POrdinalKey(ie));
}


PBYTEArray Q931::GetIE(InformationElementCodes ie) const
{
  if (informationElements.Contains(POrdinalKey(ie)))
    return informationElements[ie];

  return PBYTEArray();
}


void Q931::SetIE(InformationElementCodes ie, const PBYTEArray & userData)
{
  informationElements.SetAt(ie, new PBYTEArray(userData));
}

void Q931::RemoveIE(InformationElementCodes ie)
{
  informationElements.RemoveAt(ie);
}

void Q931::SetCause(CauseValues value, unsigned standard, unsigned location)
{
  PBYTEArray data(2);
  data[0] = (BYTE)(0x80 | ((standard&3) << 5) | (location&15));
  data[1] = (BYTE)(0x80 | value);
  SetIE(CauseIE, data);
}


Q931::CauseValues Q931::GetCause(unsigned * standard, unsigned * location) const
{
  if (!HasIE(CauseIE))
    return ErrorInCauseIE;

  PBYTEArray data = GetIE(CauseIE);
  if (data.GetSize() < 2)
    return ErrorInCauseIE;

  if (standard != NULL)
    *standard = (data[0] >> 5)&3;
  if (location != NULL)
    *location = data[0]&15;

  if ((data[0]&0x80) != 0)
    return (CauseValues)(data[1]&0x7f);

  // Allow for optional octet
  if (data.GetSize() < 3)
    return ErrorInCauseIE;

  return (CauseValues)(data[2]&0x7f);
}


void Q931::SetSignalInfo(SignalInfo value)
{
  PBYTEArray data(1);
  data[0] = (BYTE)value;
  SetIE(SignalIE, data);
}


Q931::SignalInfo Q931::GetSignalInfo() const
{
  if (!HasIE(SignalIE))
    return SignalErrorInIE;

  PBYTEArray data = GetIE(SignalIE);
  if (data.IsEmpty())
    return SignalErrorInIE;

  return (SignalInfo)data[0];
}


void Q931::SetProgressIndicator(unsigned description,
                                unsigned codingStandard,
                                unsigned location)
{
  PBYTEArray data(2);
  data[0] = (BYTE)(0x80+((codingStandard&0x03)<<5)+(location&0x0f));
  data[1] = (BYTE)(0x80+(description&0x7f));
  SetIE(ProgressIndicatorIE, data);
}


BOOL Q931::GetProgressIndicator(unsigned & description,
                                unsigned * codingStandard,
                                unsigned * location)
{
  if (!HasIE(ProgressIndicatorIE))
    return FALSE;

  PBYTEArray data = GetIE(ProgressIndicatorIE);
  if (data.GetSize() < 2)
    return FALSE;

  if (codingStandard != NULL)
    *codingStandard = (data[0]>>5)&0x03;
  if (location != NULL)
    *location = data[0]&0x0f;
  description = data[1]&0x7f;

  return TRUE;
}


void Q931::SetDisplayName(const PString & name)
{
  PBYTEArray bytes((const BYTE *)(const char *)name, name.GetLength()+1);
  SetIE(DisplayIE, bytes);
}


PString Q931::GetDisplayName() const
{
  if (!HasIE(Q931::DisplayIE))
    return PString();

  PBYTEArray display = GetIE(Q931::DisplayIE);
  if (display.IsEmpty())
    return PString();

  return PString((const char *)(const BYTE *)display, display.GetSize());
}


void Q931::SetCallingPartyNumber(const PString & number,
                                 unsigned plan,
                                 unsigned type,
                                 int presentation,
                                 int screening)
{
  PINDEX len = number.GetLength();
  PBYTEArray bytes;
  if (presentation == -1 || screening == -1) {
    bytes.SetSize(len+1);
    bytes[0] = (BYTE)(0x80|((type&7)<<4)|(plan&15));
    memcpy(bytes.GetPointer()+1, (const char *)number, len);
  }
  else {
    bytes.SetSize(len+2);
    bytes[0] = (BYTE)(((type&7)<<4)|(plan&15));
    bytes[1] = (BYTE)(0x80|((presentation&3)<<5)|(screening&3));
    memcpy(bytes.GetPointer()+2, (const char *)number, len);
  }
  SetIE(CallingPartyNumberIE, bytes);
}


BOOL Q931::GetCallingPartyNumber(PString  & number,
                                 unsigned * plan,
                                 unsigned * type,
                                 unsigned * presentation,
                                 unsigned * screening,
                                 unsigned   defPresentation,
                                 unsigned   defScreening) const
{
  number = PString();

  if (!HasIE(CallingPartyNumberIE))
    return FALSE;

  PBYTEArray bytes = GetIE(CallingPartyNumberIE);
  if (bytes.IsEmpty())
    return FALSE;

  if (plan != NULL)
    *plan = bytes[0]&15;

  if (type != NULL)
    *type = (bytes[0]>>4)&7;

  PINDEX offset;
  if (bytes[0] & 0x80) {  // Octet 3a not provided, set defaults
    if (presentation != NULL)
      *presentation = defPresentation;

    if (screening != NULL)
      *screening = defScreening;

    offset = 1;
  }
  else {
    if (presentation != NULL)
      *presentation = (bytes[1]>>5)&3;

    if (screening != NULL)
      *screening = bytes[1]&3;

    offset = 2;
   }

  PINDEX len = bytes.GetSize()-offset;
  memcpy(number.GetPointer(len+1), &bytes[offset], len);

  return !number;
}


void Q931::SetCalledPartyNumber(const PString & number, unsigned plan, unsigned type)
{
  PINDEX len = number.GetLength();
  PBYTEArray bytes(len+1);
  bytes[0] = (BYTE)(0x80|((type&7)<<4)|(plan&15));
  memcpy(bytes.GetPointer()+1, (const char *)number, len);
  SetIE(CalledPartyNumberIE, bytes);
}


BOOL Q931::GetCalledPartyNumber(PString & number, unsigned * plan, unsigned * type) const
{
  number = PString();

  if (!HasIE(CalledPartyNumberIE))
    return FALSE;

  PBYTEArray bytes = GetIE(CalledPartyNumberIE);
  if (bytes.IsEmpty())
    return FALSE;

  if (plan != NULL)
    *plan = bytes[0]&15;

  if (type != NULL)
    *type = (bytes[0]>>4)&7;

  PINDEX len = bytes.GetSize()-1;
  memcpy(number.GetPointer(len+1), &bytes[1], len);

  return !number;
}


/////////////////////////////////////////////////////////////////////////////
