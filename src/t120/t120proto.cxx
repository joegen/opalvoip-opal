/*
 * t120proto.cxx
 *
 * T.120 protocol handler
 *
 * Open Phone Abstraction Library
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "t120proto.h"
#endif

#include <t120/t120proto.h>

#include <h323/transaddr.h>
#include <t120/x224.h>
#include <asn/mcs.h>


class T120_X224 : public X224 {
    PCLASSINFO(T120_X224, X224);
  public:
    BOOL Read(H323Transport & transport);
    BOOL Write(H323Transport & transport);
};


class T120ConnectPDU : public MCS_ConnectMCSPDU {
    PCLASSINFO(T120ConnectPDU, MCS_ConnectMCSPDU);
  public:
    BOOL Read(H323Transport & transport);
    BOOL Write(H323Transport & transport);
  protected:
    T120_X224 x224;
};


const OpalMediaFormat OpalT120(
  OPAL_T120,
  OpalMediaFormat::DefaultDataSessionID,
  RTP_DataFrame::IllegalPayloadType,
  "t120",
  FALSE,   // No jitter for data
  825000, // 100's bits/sec
  0,
  0,
  0);


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

BOOL T120_X224::Read(H323Transport & transport)
{
  PBYTEArray rawData;

  if (!transport.ReadPDU(rawData)) {
    PTRACE(1, "T120\tRead of X224 failed: " << transport.GetErrorText());
    return FALSE;
  }

  if (Decode(rawData)) {
    PTRACE(1, "T120\tDecode of PDU failed:\n  " << setprecision(2) << *this);
    return FALSE;
  }

  PTRACE(4, "T120\tRead X224 PDU:\n  " << setprecision(2) << *this);
  return TRUE;
}


BOOL T120_X224::Write(H323Transport & transport)
{
  PBYTEArray rawData;

  PTRACE(4, "T120\tWrite X224 PDU:\n  " << setprecision(2) << *this);

  if (!Encode(rawData)) {
    PTRACE(1, "T120\tEncode of PDU failed:\n  " << setprecision(2) << *this);
    return FALSE;
  }

  if (!transport.WritePDU(rawData)) {
    PTRACE(1, "T120\tWrite X224 PDU failed: " << transport.GetErrorText());
    return FALSE;
  }

  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

BOOL T120ConnectPDU::Read(H323Transport & transport)
{
  if (!x224.Read(transport))
    return FALSE;

  // An X224 Data PDU...
  if (x224.GetCode() != X224::DataPDU) {
    PTRACE(1, "T120\tX224 must be data PDU");
    return FALSE;
  }

  // ... contains the T120 MCS PDU
  PBER_Stream ber = x224.GetData();
  if (!Decode(ber)) {
    PTRACE(1, "T120\tDecode of PDU failed:\n  " << setprecision(2) << *this);
    return FALSE;
  }

  PTRACE(4, "T120\tReceived MCS Connect PDU:\n  " << setprecision(2) << *this);
  return TRUE;
}


BOOL T120ConnectPDU::Write(H323Transport & transport)
{
  PTRACE(4, "T120\tSending MCS Connect PDU:\n  " << setprecision(2) << *this);

  PBER_Stream ber;
  Encode(ber);
  ber.CompleteEncoding();
  x224.BuildData(ber);
  return x224.Write(transport);
}


/////////////////////////////////////////////////////////////////////////////

OpalT120Protocol::OpalT120Protocol()
{
}


BOOL OpalT120Protocol::Originate(H323Transport & transport)
{
  PTRACE(3, "T120\tOriginate, sending X224 CONNECT-REQUEST");

  T120_X224 x224;
  x224.BuildConnectRequest();
  if (!x224.Write(transport))
    return FALSE;

  transport.SetReadTimeout(10000); // Wait 10 seconds for reply
  if (!x224.Read(transport))
    return FALSE;

  if (x224.GetCode() != X224::ConnectConfirm) {
    PTRACE(1, "T120\tPDU was not X224 CONNECT-CONFIRM");
    return FALSE;
  }

  T120ConnectPDU pdu;
  while (pdu.Read(transport)) {
    if (!HandleConnect(pdu))
      return TRUE;
  }

  return FALSE;
}


BOOL OpalT120Protocol::Answer(H323Transport & transport)
{
  PTRACE(3, "T120\tAnswer, awaiting X224 CONNECT-REQUEST");

  T120_X224 x224;
  transport.SetReadTimeout(60000); // Wait 60 seconds for reply

  do {
    if (!x224.Read(transport))
      return FALSE;
  } while (x224.GetCode() != X224::ConnectRequest);

  x224.BuildConnectConfirm();
  if (!x224.Write(transport))
    return FALSE;

  T120ConnectPDU pdu;
  while (pdu.Read(transport)) {
    if (!HandleConnect(pdu))
      return TRUE;
  }

  return FALSE;
}


BOOL OpalT120Protocol::HandleConnect(const MCS_ConnectMCSPDU & /*pdu*/)
{
  return TRUE;
}


BOOL OpalT120Protocol::HandleDomain(const MCS_DomainMCSPDU & /*pdu*/)
{
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
