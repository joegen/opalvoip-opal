/*
 * gkserver.cxx
 *
 * Gatekeeper client protocol handler
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
 * This code was based on original code from OpenGate of Egoboo Ltd. thanks
 * to Ashley Unitt for his efforts.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: gkserver.cxx,v $
 * Revision 1.2008  2002/07/01 04:56:31  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.6  2002/03/22 06:57:49  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.5  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.4  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.3  2001/11/13 04:29:47  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.2  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.44  2002/06/24 07:01:05  robertj
 * Fixed correct adding of call to endpoint structure, only if call confirmed.
 *
 * Revision 1.43  2002/06/21 02:52:47  robertj
 * Fixed problem with double checking H.235 hashing, this causes failure as
 *   the authenticator thinks it is a replay attack.
 *
 * Revision 1.42  2002/06/19 05:03:11  robertj
 * Changed gk code to allow for H.235 security on an endpoint by endpoint basis.
 *
 * Revision 1.41  2002/06/12 03:55:21  robertj
 * Added function to add/remove multiple listeners in one go comparing against
 *   what is already running so does not interrupt unchanged listeners.
 *
 * Revision 1.40  2002/06/07 08:12:38  robertj
 * Added check to reject ARQ if does not contain a call identifier.
 *
 * Revision 1.39  2002/06/07 06:19:21  robertj
 * Fixed GNU warning
 *
 * Revision 1.38  2002/05/29 00:03:19  robertj
 * Fixed unsolicited IRR support in gk client and server,
 *   including support for IACK and INAK.
 *
 * Revision 1.37  2002/05/17 03:41:46  robertj
 * Fixed problems with H.235 authentication on RAS for server and client.
 *
 * Revision 1.36  2002/05/09 02:53:20  robertj
 * Fixed some problems with setting host/alias field in call on ARQ
 *
 * Revision 1.35  2002/05/08 02:06:07  robertj
 * Fixed failure to remove call if is rejected on slow reply.
 * Fixed overwriting of application infor on lightweight RRQ.
 * Fixed inclusion of adjusted aliases in RCF.
 *
 * Revision 1.34  2002/05/07 06:25:20  robertj
 * Fixed GNU compiler compatibility.
 *
 * Revision 1.33  2002/05/07 06:15:54  robertj
 * Changed ARQ processing so rejects multiple ARQ with same call id, not this
 *   does NOT include the transport level retries (based on sequence number).
 *
 * Revision 1.32  2002/05/07 03:18:15  robertj
 * Added application info (name/version etc) into registered endpoint data.
 *
 * Revision 1.31  2002/05/06 09:05:01  craigs
 * Fixed compile problem on Linux
 *
 * Revision 1.30  2002/05/06 00:56:41  robertj
 * Sizeable rewrite of gatekeeper server code to make more bulletproof against
 *   multithreaded operation. Especially when using slow response/RIP feature.
 * Also changed the call indexing to use call id and direction as key.
 *
 * Revision 1.29  2002/04/09 02:34:14  robertj
 * Fixed problem with duplicate entries in endpoint list if ARQ gets retried,
 *   thanks Horacio J. Peña
 *
 * Revision 1.28  2002/03/06 02:51:31  robertj
 * Fixed race condition when starting slow server response thread.
 *
 * Revision 1.27  2002/03/06 02:01:26  craigs
 * Add log message when SlowHandler thread started
 *
 * Revision 1.26  2002/03/05 03:42:40  robertj
 * Fixed calling of endpoints OnUnregister() function, thanks Francisco Olarte Sanz
 *
 * Revision 1.25  2002/03/04 07:57:57  craigs
 * Changed log level of ageing message
 * Fixed assert caused by accessing byIdentifier with integer
 *
 * Revision 1.24  2002/03/03 21:34:54  robertj
 * Added gatekeeper monitor thread.
 *
 * Revision 1.23  2002/03/02 05:59:01  robertj
 * Fixed possible bandwidth leak (thanks Francisco Olarte Sanz) and in
 *   the process added OnBandwidth function to H323GatekeeperCall class.
 *
 * Revision 1.22  2002/03/01 04:09:35  robertj
 * Fixed problems with keeping track of calls. Calls are now indexed by call-id
 *   alone and maintain both endpoints of call in its structure. Fixes problem
 *   with calls form an endpoint to itself, and having two objects being tracked
 *   where there is really only one call.
 *
 * Revision 1.21  2002/02/25 10:26:39  robertj
 * Added ARQ with zero bandwidth as a request for a default, thanks Federico Pinna
 *
 * Revision 1.20  2002/02/25 10:16:21  robertj
 * Fixed bad mistake in getting new aliases from ARQ, thanks HoraPe
 *
 * Revision 1.19  2002/02/25 09:49:02  robertj
 * Fixed possible "bandwidth leak" on ARQ, thanks Francisco Olarte Sanz
 *
 * Revision 1.18  2002/02/11 05:19:21  robertj
 * Allowed for multiple ARQ requests for same call ID. Reuses existing call
 *   object and sends another ACF.
 *
 * Revision 1.17  2002/02/04 05:21:27  robertj
 * Lots of changes to fix multithreaded slow response code (RIP).
 * Fixed problem with having two entries for same call in call list.
 *
 * Revision 1.16  2002/01/31 06:45:44  robertj
 * Added more checking for invalid list processing in calls database.
 *
 * Revision 1.15  2002/01/30 05:07:54  robertj
 * Some robustness changes, temporarily removed threads in RIP.
 *
 * Revision 1.14  2001/12/15 10:10:48  robertj
 * GCC compatibility
 *
 * Revision 1.13  2001/12/15 08:09:08  robertj
 * Added alerting, connect and end of call times to be sent to RAS server.
 *
 * Revision 1.12  2001/12/14 06:41:17  robertj
 * Added call end reason codes in DisengageRequest for GK server use.
 *
 * Revision 1.11  2001/12/13 11:08:45  robertj
 * Significant changes to support slow request handling, automatically sending
 *   RIP and spawning thread to handle time consuming operation.
 *
 * Revision 1.10  2001/12/06 06:44:42  robertj
 * Removed "Win32 SSL xxx" build configurations in favour of system
 *   environment variables to select optional libraries.
 *
 * Revision 1.9  2001/11/21 13:35:23  craigs
 * Removed deadlock with empty registered endpoint list
 *
 * Revision 1.8  2001/11/19 06:56:44  robertj
 * Added prefix strings for gateways registered with this gk, thanks Mikael Stolt
 *
 * Revision 1.7  2001/09/18 10:06:54  robertj
 * Added trace for if RRJ on security because user has no password.
 *
 * Revision 1.6  2001/09/13 01:15:20  robertj
 * Added flag to H235Authenticator to determine if gkid and epid is to be
 *   automatically set as the crypto token remote id and local id.
 *
 * Revision 1.5  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.4  2001/08/06 07:44:55  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.3  2001/08/06 03:18:38  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 * Improved access to H.235 secure RAS functionality.
 * Changes to H.323 secure RAS contexts to help use with gk server.
 *
 * Revision 1.2  2001/07/24 10:12:33  craigs
 * Fixed problem with admission requests not having src signalling addresses
 *
 * Revision 1.1  2001/07/24 02:31:07  robertj
 * Added gatekeeper RAS protocol server classes.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "gkserver.h"
#endif

#include <h323/gkserver.h>

#include <h323/h323ep.h>
#include <h323/h323pdu.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperRequest::H323GatekeeperRequest(H323GatekeeperListener & ras,
                                             unsigned seqNum)
  : rasChannel(ras),
    replyAddress(ras.GetTransport().GetRemoteAddress()),
    confirm(ras),
    reject(ras)
{
  requestSequenceNumber = seqNum;
  authenticity = UnknownAuthentication;
  fastResponseRequired = TRUE;
  thread = NULL;
}


BOOL H323GatekeeperRequest::HandlePDU()
{
  int response = OnHandlePDU();
  switch (response) {
    case Ignore :
      return FALSE;

    case Confirm :
      WritePDU(confirm);
      return FALSE;

    case Reject :
      WritePDU(reject);
      return FALSE;
  }

  H323RasPDU pdu(rasChannel);
  pdu.BuildRequestInProgress(requestSequenceNumber, response);
  if (!WritePDU(pdu))
    return FALSE;

  if (fastResponseRequired) {
    fastResponseRequired = FALSE;
    thread = PThread::Create(PCREATE_NOTIFIER(SlowHandler), 0,
                             PThread::AutoDeleteThread,
                             PThread::NormalPriority,
                             "GkSrvr:%x");
    PTRACE(5, "RAS\tStarting slow PDU handler thread: " << thread->GetThreadName());
  }

  return TRUE;
}


void H323GatekeeperRequest::SlowHandler(PThread &, INT)
{
  PTRACE(3, "RAS\tStarted slow PDU handler thread.");

  while (HandlePDU())
    ;

  delete this;

  PTRACE(3, "RAS\tEnded slow PDU handler thread.");
}


BOOL H323GatekeeperRequest::WritePDU(H323RasPDU & pdu)
{
  PTRACE_BLOCK("H323GatekeeperRequest::WritePDU");

  rasChannel.AttachCurrentEP(endpoint);

  BOOL useDefault = TRUE;

  if (endpoint != NULL) {
    for (PINDEX i = 0; i < endpoint->GetRASAddressCount(); i++) {
      if (rasChannel.GetTransport().ConnectTo(endpoint->GetRASAddress(i))) {
        useDefault = FALSE;
        break;
      }
    }
  }

  if (useDefault)
    rasChannel.GetTransport().ConnectTo(replyAddress);

  BOOL ok = rasChannel.WritePDU(pdu);

  rasChannel.DetachCurrentEP();
  return ok;
}


BOOL H323GatekeeperRequest::CheckCryptoTokens(unsigned optionalField)
{
  if (authenticity == UnknownAuthentication) {
    rasChannel.AttachCurrentEP(endpoint);
    authenticity = rasChannel.CheckCryptoTokens(GetCryptoTokens(), GetRawPDU(), optionalField)
                                ? IsAuthenticated : FailedAuthentication;
    rasChannel.DetachCurrentEP();
  }

  return authenticity == IsAuthenticated;
}



/////////////////////////////////////////////////////////////////////////////

H323GatekeeperGRQ::H323GatekeeperGRQ(H323GatekeeperListener & rasChannel,
                                     const H225_GatekeeperRequest & grq_)
  : H323GatekeeperRequest(rasChannel, grq_.m_requestSeqNum),
    grq(grq_),
    gcf(confirm.BuildGatekeeperConfirm(grq.m_requestSeqNum)),
    grj(reject.BuildGatekeeperReject(grq.m_requestSeqNum,
                                     H225_GatekeeperRejectReason::e_terminalExcluded))
{
  if (rasChannel.GetTransport().IsCompatibleTransport(H323TransportAddress(grq.m_rasAddress)))
    replyAddress = grq.m_rasAddress;
}


const PASN_Sequence & H323GatekeeperGRQ::GetRawPDU() const
{
  return grq;
}


PString H323GatekeeperGRQ::GetGatekeeperIdentifier() const
{
  return grq.m_gatekeeperIdentifier;
}


PString H323GatekeeperGRQ::GetEndpointIdentifier() const
{
  return PString::Empty(); // Dummy, should never be used
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperGRQ::GetCryptoTokens() const
{
  return grq.m_cryptoTokens;
}


void H323GatekeeperGRQ::SetRejectReason(unsigned reasonCode)
{
  grj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperGRQ::OnHandlePDU()
{
  return rasChannel.OnDiscovery(*this);
}


H323GatekeeperRRQ::H323GatekeeperRRQ(H323GatekeeperListener & rasChannel,
                                     const H225_RegistrationRequest & rrq_)
  : H323GatekeeperRequest(rasChannel, rrq_.m_requestSeqNum),
    rrq(rrq_),
    rcf(confirm.BuildRegistrationConfirm(rrq.m_requestSeqNum)),
    rrj(reject.BuildRegistrationReject(rrq.m_requestSeqNum))
{
  for (PINDEX i = 0; i < rrq.m_rasAddress.GetSize(); i++) {
    if (rasChannel.GetTransport().IsCompatibleTransport(H323TransportAddress(rrq.m_rasAddress[i]))) {
      replyAddress = rrq.m_rasAddress[i];
      break;
    }
  }
}


const PASN_Sequence & H323GatekeeperRRQ::GetRawPDU() const
{
  return rrq;
}


PString H323GatekeeperRRQ::GetGatekeeperIdentifier() const
{
  return rrq.m_gatekeeperIdentifier;
}


PString H323GatekeeperRRQ::GetEndpointIdentifier() const
{
  return rrq.m_endpointIdentifier;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperRRQ::GetCryptoTokens() const
{
  return rrq.m_cryptoTokens;
}


void H323GatekeeperRRQ::SetRejectReason(unsigned reasonCode)
{
  rrj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperRRQ::OnHandlePDU()
{
  return rasChannel.OnRegistration(*this);
}


H323GatekeeperURQ::H323GatekeeperURQ(H323GatekeeperListener & rasChannel,
                                     const H225_UnregistrationRequest & urq_)
  : H323GatekeeperRequest(rasChannel, urq_.m_requestSeqNum),
    urq(urq_),
    ucf(confirm.BuildUnregistrationConfirm(urq.m_requestSeqNum)),
    urj(reject.BuildUnregistrationReject(urq.m_requestSeqNum))
{
}


const PASN_Sequence & H323GatekeeperURQ::GetRawPDU() const
{
  return urq;
}


PString H323GatekeeperURQ::GetGatekeeperIdentifier() const
{
  return urq.m_gatekeeperIdentifier;
}


PString H323GatekeeperURQ::GetEndpointIdentifier() const
{
  return urq.m_endpointIdentifier;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperURQ::GetCryptoTokens() const
{
  return urq.m_cryptoTokens;
}


void H323GatekeeperURQ::SetRejectReason(unsigned reasonCode)
{
  urj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperURQ::OnHandlePDU()
{
  return rasChannel.OnUnregistration(*this);
}


H323GatekeeperARQ::H323GatekeeperARQ(H323GatekeeperListener & rasChannel,
                                     const H225_AdmissionRequest & arq_)
  : H323GatekeeperRequest(rasChannel, arq_.m_requestSeqNum),
    arq(arq_),
    acf(confirm.BuildAdmissionConfirm(arq.m_requestSeqNum)),
    arj(reject.BuildAdmissionReject(arq.m_requestSeqNum))
{
}


const PASN_Sequence & H323GatekeeperARQ::GetRawPDU() const
{
  return arq;
}


PString H323GatekeeperARQ::GetGatekeeperIdentifier() const
{
  return arq.m_gatekeeperIdentifier;
}


PString H323GatekeeperARQ::GetEndpointIdentifier() const
{
  return arq.m_endpointIdentifier;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperARQ::GetCryptoTokens() const
{
  return arq.m_cryptoTokens;
}


void H323GatekeeperARQ::SetRejectReason(unsigned reasonCode)
{
  arj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperARQ::OnHandlePDU()
{
  return rasChannel.OnAdmission(*this);
}


H323GatekeeperDRQ::H323GatekeeperDRQ(H323GatekeeperListener & rasChannel,
                                     const H225_DisengageRequest & drq_)
  : H323GatekeeperRequest(rasChannel, drq_.m_requestSeqNum),
    drq(drq_),
    dcf(confirm.BuildDisengageConfirm(drq.m_requestSeqNum)),
    drj(reject.BuildDisengageReject(drq.m_requestSeqNum))
{
}


const PASN_Sequence & H323GatekeeperDRQ::GetRawPDU() const
{
  return drq;
}


PString H323GatekeeperDRQ::GetGatekeeperIdentifier() const
{
  return drq.m_gatekeeperIdentifier;
}


PString H323GatekeeperDRQ::GetEndpointIdentifier() const
{
  return drq.m_endpointIdentifier;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperDRQ::GetCryptoTokens() const
{
  return drq.m_cryptoTokens;
}


void H323GatekeeperDRQ::SetRejectReason(unsigned reasonCode)
{
  drj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperDRQ::OnHandlePDU()
{
  return rasChannel.OnDisengage(*this);
}


H323GatekeeperBRQ::H323GatekeeperBRQ(H323GatekeeperListener & rasChannel,
                                     const H225_BandwidthRequest & brq_)
  : H323GatekeeperRequest(rasChannel, brq_.m_requestSeqNum),
    brq(brq_),
    bcf(confirm.BuildBandwidthConfirm(brq.m_requestSeqNum)),
    brj(reject.BuildBandwidthReject(brq.m_requestSeqNum))
{
}


const PASN_Sequence & H323GatekeeperBRQ::GetRawPDU() const
{
  return brq;
}


PString H323GatekeeperBRQ::GetGatekeeperIdentifier() const
{
  return brq.m_gatekeeperIdentifier;
}


PString H323GatekeeperBRQ::GetEndpointIdentifier() const
{
  return brq.m_endpointIdentifier;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperBRQ::GetCryptoTokens() const
{
  return brq.m_cryptoTokens;
}


void H323GatekeeperBRQ::SetRejectReason(unsigned reasonCode)
{
  brj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperBRQ::OnHandlePDU()
{
  return rasChannel.OnBandwidth(*this);
}


H323GatekeeperLRQ::H323GatekeeperLRQ(H323GatekeeperListener & rasChannel,
                                     const H225_LocationRequest & lrq_)
  : H323GatekeeperRequest(rasChannel, lrq_.m_requestSeqNum),
    lrq(lrq_),
    lcf(confirm.BuildLocationConfirm(lrq.m_requestSeqNum)),
    lrj(reject.BuildLocationReject(lrq.m_requestSeqNum))
{
  if (rasChannel.GetTransport().IsCompatibleTransport(H323TransportAddress(lrq.m_replyAddress)))
    replyAddress = lrq.m_replyAddress;
}


const PASN_Sequence & H323GatekeeperLRQ::GetRawPDU() const
{
  return lrq;
}


PString H323GatekeeperLRQ::GetGatekeeperIdentifier() const
{
  return lrq.m_gatekeeperIdentifier;
}


PString H323GatekeeperLRQ::GetEndpointIdentifier() const
{
  return lrq.m_endpointIdentifier;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperLRQ::GetCryptoTokens() const
{
  return lrq.m_cryptoTokens;
}


void H323GatekeeperLRQ::SetRejectReason(unsigned reasonCode)
{
  lrj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperLRQ::OnHandlePDU()
{
  return rasChannel.OnLocation(*this);
}


H323GatekeeperIRR::H323GatekeeperIRR(H323GatekeeperListener & rasChannel,
                                     const H225_InfoRequestResponse & irr_)
  : H323GatekeeperRequest(rasChannel, irr_.m_requestSeqNum),
    irr(irr_),
    iack(confirm.BuildInfoRequestAck(irr.m_requestSeqNum)),
    inak(reject.BuildInfoRequestNak(irr.m_requestSeqNum))
{
}


const PASN_Sequence & H323GatekeeperIRR::GetRawPDU() const
{
  return irr;
}


PString H323GatekeeperIRR::GetGatekeeperIdentifier() const
{
  return PString::Empty();
}


PString H323GatekeeperIRR::GetEndpointIdentifier() const
{
  return irr.m_endpointIdentifier;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperIRR::GetCryptoTokens() const
{
  return irr.m_cryptoTokens;
}


void H323GatekeeperIRR::SetRejectReason(unsigned reasonCode)
{
  inak.m_nakReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperIRR::OnHandlePDU()
{
  return rasChannel.OnInfoResponse(*this);
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperCall::H323GatekeeperCall(H323GatekeeperServer& gk,
                                       const OpalGloballyUniqueID & id,
                                       Direction dir)
  : gatekeeper(gk),
    callIdentifier(id),
    conferenceIdentifier(NULL),
    alertingTime(0),
    connectedTime(0),
    callEndTime(0)
{
  endpoint = NULL;
  direction = dir;
  callReference = 0;

  callEndReason = OpalNumCallEndReasons;

  bandwidthUsed = 0;
  infoResponseRate = gatekeeper.GetInfoResponseRate();

  PTRACE(4, "RAS\tCall created: " << *this);
}


H323GatekeeperCall::~H323GatekeeperCall()
{
  PTRACE(4, "RAS\tCall destroyed: " << *this);
}


PObject::Comparison H323GatekeeperCall::Compare(const PObject & obj) const
{
  // Do not have to lock the object as these fields should never change for
  // life of the object

  PAssert(obj.IsDescendant(H323GatekeeperCall::Class()), PInvalidCast);
  const H323GatekeeperCall & other = (const H323GatekeeperCall &)obj;
  Comparison result = callIdentifier.Compare(other.callIdentifier);
  if (result != EqualTo)
    return result;

  if (direction == UnknownDirection || other.direction == UnknownDirection)
    return EqualTo;

  if (direction > other.direction)
    return GreaterThan;
  if (direction < other.direction)
    return LessThan;
  return EqualTo;
}


void H323GatekeeperCall::PrintOn(ostream & strm) const
{
  // Do not have to lock the object as these fields should never change for
  // life of the object

  strm << callIdentifier;

  switch (direction) {
    case AnsweringCall :
      strm << "-Answer";
      break;
    case OriginatingCall :
      strm << "-Originate";
      break;
    default :
      break;
  }
}


H323GatekeeperRequest::Response H323GatekeeperCall::OnAdmission(H323GatekeeperARQ & info)
{
  PTRACE_BLOCK("H323GatekeeperCall::OnAdmission");

  if (endpoint != NULL) {
    info.SetRejectReason(H225_AdmissionRejectReason::e_resourceUnavailable);
    PTRACE(2, "RAS\tARQ rejected, multiple use of same call id.");
    return H323GatekeeperRequest::Reject;
  }

  PINDEX i;

  LockReadWrite();

  PTRACE(3, "RAS\tProcessing OnAdmission for " << *this);

  endpoint = info.endpoint;

  callReference = info.arq.m_callReferenceValue;
  conferenceIdentifier = info.arq.m_conferenceID;

  for (i = 0; i < info.arq.m_srcInfo.GetSize(); i++) {
    PString alias = H323GetAliasAddressString(info.arq.m_srcInfo[i]);
    if (srcAliases.GetValuesIndex(alias) == P_MAX_INDEX)
      srcAliases += alias;
  }

  srcNumber = H323GetAliasAddressE164(info.arq.m_srcInfo);

  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress))
    srcHost = info.arq.m_srcCallSignalAddress;
  else
    srcHost = info.GetReplyAddress();

  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {
    for (i = 0; i < info.arq.m_destinationInfo.GetSize(); i++) {
      PString alias = H323GetAliasAddressString(info.arq.m_destinationInfo[i]);
      if (dstAliases.GetValuesIndex(alias) == P_MAX_INDEX)
        dstAliases += alias;
    }

    dstNumber = H323GetAliasAddressE164(info.arq.m_destinationInfo);
  }

  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress))
    dstHost = info.arq.m_destCallSignalAddress;

  UnlockReadWrite();

  if (direction == AnsweringCall) {
    // See if our policies allow the call
    BOOL denied = TRUE;
    for (i = 0; i < info.arq.m_srcInfo.GetSize(); i++) {
      if (gatekeeper.CheckAliasAddressPolicy(*endpoint, info.arq, info.arq.m_srcInfo[i])) {
        denied = FALSE;
        break;
      }
    }

    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress)) {
      if (gatekeeper.CheckSignalAddressPolicy(*endpoint, info.arq,
                                              H323TransportAddress(info.arq.m_srcCallSignalAddress)))
        denied = FALSE;
    }

    if (denied) {
      info.SetRejectReason(H225_AdmissionRejectReason::e_securityDenial);
      PTRACE(2, "RAS\tARQ rejected, not allowed to answer call");
      return H323GatekeeperRequest::Reject;
    }
  }
  else {
    PSafePtr<H323RegisteredEndPoint> destEP;

    // Do lookup by alias
    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {

      // Have provided an alias, see if we can route it.
      BOOL denied = TRUE;
      BOOL noAddress = TRUE;
      for (i = 0; i < info.arq.m_destinationInfo.GetSize(); i++) {
        if (gatekeeper.CheckAliasAddressPolicy(*endpoint, info.arq, info.arq.m_destinationInfo[i])) {
          denied = FALSE;
          H323TransportAddress host;
          if (gatekeeper.TranslateAliasAddressToSignalAddress(info.arq.m_destinationInfo[i], host)) {
            destEP = gatekeeper.FindEndPointByAliasAddress(info.arq.m_destinationInfo[i]);
            LockReadWrite();
            dstHost = host;
            UnlockReadWrite();
            noAddress = FALSE;
            break;
          }
        }
      }

      if (denied) {
        info.SetRejectReason(H225_AdmissionRejectReason::e_securityDenial);
        PTRACE(2, "RAS\tARQ rejected, not allowed to make call");
        return H323GatekeeperRequest::Reject;
      }

      if (noAddress) {
        info.SetRejectReason(H225_AdmissionRejectReason::e_calledPartyNotRegistered);
        PTRACE(2, "RAS\tARQ rejected, destination alias not registered");
        return H323GatekeeperRequest::Reject;
      }

      if (destEP != NULL) {
        destEP.SetSafetyMode(PSafeReadOnly);
        LockReadWrite();
        dstAliases.RemoveAll();
        dstNumber = PString::Empty();
        for (i = 0; i < destEP->GetAliasCount(); i++) {
          PString alias = destEP->GetAlias(i);
          dstAliases += alias;
          if (strspn(alias, "0123456789*#") == strlen(alias))
            dstNumber = alias;
        }
        UnlockReadWrite();
        destEP.SetSafetyMode(PSafeReference);
      }
    }

    // Has provided an alias and signal address, see if agree
    if (destEP != NULL &&
        info.arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress) &&
        gatekeeper.FindEndPointBySignalAddress(info.arq.m_destCallSignalAddress) != destEP) {
      info.SetRejectReason(H225_AdmissionRejectReason::e_aliasesInconsistent);
      PTRACE(2, "RAS\tARQ rejected, destination address not for specified alias");
      return H323GatekeeperRequest::Reject;
    }

    // Have no address from anywhere
    if (dstHost.IsEmpty()) {
      info.SetRejectReason(H225_AdmissionRejectReason::e_incompleteAddress);
      PTRACE(2, "RAS\tARQ rejected, must have destination address or alias");
      return H323GatekeeperRequest::Reject;
    }

    // Now see if security policy allows connection to that address
    if (!gatekeeper.CheckSignalAddressPolicy(*endpoint, info.arq, dstHost)) {
      info.SetRejectReason(H225_AdmissionRejectReason::e_securityDenial);
      PTRACE(2, "RAS\tARQ rejected, not allowed to make call");
      return H323GatekeeperRequest::Reject;
    }
  }

  // See if have bandwidth
  unsigned requestedBandwidth = info.arq.m_bandWidth;
  if (requestedBandwidth == 0)
    requestedBandwidth = gatekeeper.GetDefaultBandwidth();

  unsigned bandwidthAllocated = gatekeeper.AllocateBandwidth(info.arq.m_bandWidth);
  if (bandwidthAllocated == 0) {
    info.SetRejectReason(H225_AdmissionRejectReason::e_requestDenied);
    PTRACE(2, "RAS\tARQ rejected, not enough bandwidth");
    return H323GatekeeperRequest::Reject;
  }

  bandwidthUsed = bandwidthAllocated;  // Do we need to lock for atomic assignment?
  info.acf.m_bandWidth = bandwidthUsed;

  // Set the rate for getting unsolicited IRR's
  if (infoResponseRate > 0) {
    info.acf.IncludeOptionalField(H225_AdmissionConfirm::e_irrFrequency);
    info.acf.m_irrFrequency = infoResponseRate;
  }
  info.acf.m_willRespondToIRR = TRUE;

  // Set the destination to fixed value of gk routed
  if (gatekeeper.IsGatekeeperRouted())
    info.acf.m_callModel.SetTag(H225_CallModel::e_gatekeeperRouted);
  dstHost.SetPDU(info.acf.m_destCallSignalAddress);


  return H323GatekeeperRequest::Confirm;
}


BOOL H323GatekeeperCall::Disengage()
{
  // Send DRQ to endpoint(s)
  return FALSE;
}


H323GatekeeperRequest::Response H323GatekeeperCall::OnDisengage(H323GatekeeperDRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperCall::OnDisengage");

  LockReadWrite();

  if (info.drq.HasOptionalField(H225_DisengageRequest::e_usageInformation)) {
    if (info.drq.m_usageInformation.HasOptionalField(H225_RasUsageInformation::e_alertingTime))
      alertingTime = PTime((unsigned)info.drq.m_usageInformation.m_alertingTime);
    if (info.drq.m_usageInformation.HasOptionalField(H225_RasUsageInformation::e_connectTime))
      connectedTime = PTime((unsigned)info.drq.m_usageInformation.m_connectTime);
    if (info.drq.m_usageInformation.HasOptionalField(H225_RasUsageInformation::e_endTime))
      callEndTime = PTime((unsigned)info.drq.m_usageInformation.m_endTime);
  }

  if (info.drq.HasOptionalField(H225_DisengageRequest::e_terminationCause)) {
    if (info.drq.m_terminationCause.GetTag() == H225_CallTerminationCause::e_releaseCompleteReason) {
      H225_ReleaseCompleteReason & reason = info.drq.m_terminationCause;
      callEndReason = H323TranslateToCallEndReason(Q931::ErrorInCauseIE, reason);
    }
    else {
      PASN_OctetString & cause = info.drq.m_terminationCause;
      H225_ReleaseCompleteReason dummy;
      callEndReason = H323TranslateToCallEndReason((Q931::CauseValues)(cause[1]&0x7f), dummy);
    }
  }

  UnlockReadWrite();

  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323GatekeeperCall::OnBandwidth(H323GatekeeperBRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperCall::OnBandwidth");

  if (endpoint != info.endpoint) {
    info.SetRejectReason(H225_BandRejectReason::e_invalidPermission);
    PTRACE(2, "RAS\tBRQ rejected, call is not owned by endpoint");
    return H323GatekeeperRequest::Reject;
  }

  unsigned newBandwidth = gatekeeper.AllocateBandwidth(info.brq.m_bandWidth, bandwidthUsed);
  if (newBandwidth == 0) {
    info.SetRejectReason(H225_BandRejectReason::e_insufficientResources);
    PTRACE(2, "RAS\tBRQ rejected, no bandwidth");
    return H323GatekeeperRequest::Reject;
  }

  bandwidthUsed = newBandwidth;  // Do we need to lock for atomic assignment?

  info.bcf.m_bandWidth = newBandwidth;
  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323GatekeeperCall::OnInfoResponse(H323GatekeeperIRR &,
                                                                   H225_InfoRequestResponse_perCallInfo_subtype & /*info*/)
{
  PTRACE_BLOCK("H323GatekeeperCall::OnInfoResponse");

  lastInfoResponse = PTime();

  return H323GatekeeperRequest::Confirm;
}


BOOL H323GatekeeperCall::OnHeartbeat()
{
  if ((lastInfoResponse - PTime()).GetSeconds() > (int)(infoResponseRate+10)) {
    // Do IRQ and wait for IRR on call
  }

  return TRUE;
}


static PString MakeAddress(const PString & number,
                           const PStringArray aliases,
                           const H323TransportAddress host)
{
  PStringStream addr;

  if (!number)
    addr << number;
  else if (!aliases.IsEmpty())
    addr << aliases[0];

  if (!host) {
    if (!addr.IsEmpty())
      addr << '@';
    addr << host;
  }

  return addr;
}


PString H323GatekeeperCall::GetSourceAddress() const
{
  LockReadOnly();
  PString addr = MakeAddress(srcNumber, srcAliases, srcHost);
  UnlockReadOnly();
  return addr;
}


PString H323GatekeeperCall::GetDestinationAddress() const
{
  LockReadOnly();
  PString addr = MakeAddress(dstNumber, dstAliases, dstHost);
  UnlockReadOnly();
  return addr;
}


/////////////////////////////////////////////////////////////////////////////

H323RegisteredEndPoint::H323RegisteredEndPoint(H323GatekeeperServer & gk,
                                               const PString & id)
  : gatekeeper(gk),
    identifier(id),
    authenticators(gk.CreateAuthenticators())
{
  activeCalls.DisallowDeleteObjects();

  PTRACE(3, "RAS\tCreated registered endpoint: " << id);
}


PObject::Comparison H323RegisteredEndPoint::Compare(const PObject & obj) const
{
  // Do not have to lock the object as these fields should never change for
  // life of the object

  PAssert(obj.IsDescendant(H323RegisteredEndPoint::Class()), PInvalidCast);
  return identifier.Compare(((const H323RegisteredEndPoint &)obj).identifier);
}


void H323RegisteredEndPoint::PrintOn(ostream & strm) const
{
  // Do not have to lock the object as these fields should never change for
  // life of the object

  strm << identifier;
}


void H323RegisteredEndPoint::AddCall(H323GatekeeperCall * call)
{
  LockReadWrite();

  if (activeCalls.GetObjectsIndex(call) == P_MAX_INDEX)
    activeCalls.Append(call);

  UnlockReadWrite();
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnRegistration(H323GatekeeperRRQ & info)
{
  PTRACE_BLOCK("H323RegisteredEndPoint::OnRegistration");

  lastRegistration = PTime();

  timeToLive = gatekeeper.GetTimeToLive();
  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_timeToLive) &&
      timeToLive > info.rrq.m_timeToLive)
    timeToLive = info.rrq.m_timeToLive;

  if (timeToLive > 0) {
    info.rcf.IncludeOptionalField(H225_RegistrationRequest::e_timeToLive);
    info.rcf.m_timeToLive = timeToLive;
  }

  info.rcf.m_endpointIdentifier = identifier;

  if (info.rrq.m_keepAlive)
    return H323GatekeeperRequest::Confirm;

  return OnFullRegistration(info);
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnFullRegistration(H323GatekeeperRRQ & info)
{
  PINDEX i, count;

  rasAddresses.RemoveAll();
  if (info.rrq.m_rasAddress.GetSize() > 0) {
    count = 0;
    for (i = 0; i < info.rrq.m_rasAddress.GetSize(); i++) {
      H323TransportAddress address(info.rrq.m_rasAddress[i]);
      if (!address)
        rasAddresses.SetAt(count++, new OpalTransportAddress(address));
    }
  }

  if (rasAddresses.IsEmpty()) {
    info.SetRejectReason(H225_RegistrationRejectReason::e_invalidRASAddress);
    PTRACE(2, "RAS\tRRQ rejected, does not have valid RAS address!");
    return H323GatekeeperRequest::Reject;
  }


  signalAddresses.RemoveAll();
  count = 0;
  for (i = 0; i < info.rrq.m_callSignalAddress.GetSize(); i++) {
    H323TransportAddress address(info.rrq.m_callSignalAddress[i]);
    signalAddresses.SetAt(count++, new H323TransportAddress(address));
  }

  if (signalAddresses.IsEmpty()) {
    info.SetRejectReason(H225_RegistrationRejectReason::e_invalidCallSignalAddress);
    return H323GatekeeperRequest::Reject;
  }

  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_terminalAlias)) {
    aliases.RemoveAll();
    for (i = 0; i < info.rrq.m_terminalAlias.GetSize(); i++) {
      PString alias = H323GetAliasAddressString(info.rrq.m_terminalAlias[i]);
      if (!alias)
        aliases.AppendString(alias);
    }
  }

  info.rcf.IncludeOptionalField(H225_RegistrationConfirm::e_terminalAlias);
  info.rcf.m_terminalAlias.SetSize(aliases.GetSize());
  for (i = 0; i < aliases.GetSize(); i++)
    H323SetAliasAddress(aliases[i], info.rcf.m_terminalAlias[i]);

  const H225_EndpointType & terminalType = info.rrq.m_terminalType;
  if (terminalType.HasOptionalField(H225_EndpointType::e_gateway) &&
      terminalType.m_gateway.HasOptionalField(H225_GatewayInfo::e_protocol)) {
    const H225_ArrayOf_SupportedProtocols & protocols = terminalType.m_gateway.m_protocol;
    for (i = 0; i < protocols.GetSize(); i++) {

      // Only voice prefixes are supported
      if (protocols[i].GetTag() == H225_SupportedProtocols::e_voice) {
	H225_VoiceCaps & voiceCaps = protocols[i];
	if (voiceCaps.HasOptionalField(H225_VoiceCaps::e_supportedPrefixes)) {
	  H225_ArrayOf_SupportedPrefix & prefixes = voiceCaps.m_supportedPrefixes;
	  voicePrefixes.SetSize(prefixes.GetSize());
	  for (PINDEX j = 0; j < prefixes.GetSize(); j++) {
	    PString prefix = H323GetAliasAddressString(prefixes[j].m_prefix);
	    voicePrefixes[j] = prefix;
	  }
	}
	break;  // If voice protocol is found, don't look any further
      }
    }
  }

  applicationInfo = H323GetApplicationInfo(info.rrq.m_endpointVendor);

  return OnSecureRegistration(info);
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnSecureRegistration(H323GatekeeperRRQ & info)
{
  for (PINDEX i = 0; i < aliases.GetSize(); i++) {
    PString password;
    if (gatekeeper.GetUsersPassword(aliases[i], password)) {
      if (!password)
        SetPassword(password);
      return H323GatekeeperRequest::Confirm;
    }
  }

  PTRACE(2, "RAS\tRejecting RRQ, no aliases have a password.");
  info.SetRejectReason(H225_RegistrationRejectReason::e_securityDenial);
  return H323GatekeeperRequest::Reject;
}


BOOL H323RegisteredEndPoint::SetPassword(const PString & password)
{
  if (authenticators.IsEmpty() || password.IsEmpty())
    return FALSE;

  for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
    H235Authenticator & authenticator = authenticators[i];
    if (authenticator.UseGkAndEpIdentifiers())
      authenticator.SetRemoteId(identifier);
    authenticator.SetPassword(password);
    authenticator.Enable();
  }

  return TRUE;
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnUnregistration(H323GatekeeperURQ & /*info*/)
{
  PTRACE_BLOCK("H323RegisteredEndPoint::OnUnregistration");

  LockReadWrite();

  while (activeCalls.GetSize() > 0)
    gatekeeper.RemoveCall(&activeCalls[0]);

  UnlockReadWrite();

  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnInfoResponse(H323GatekeeperIRR & info)
{
  PTRACE_BLOCK("H323RegisteredEndPoint::OnInfoResponse");

  LockReadOnly();

  for (PINDEX i = 0; i < info.irr.m_perCallInfo.GetSize(); i++) {
    H225_InfoRequestResponse_perCallInfo_subtype & perCallInfo = info.irr.m_perCallInfo[i];
    H323GatekeeperCall search(gatekeeper,
                              perCallInfo.m_callIdentifier.m_guid,
                              perCallInfo.m_originator ? H323GatekeeperCall::OriginatingCall
                                                       : H323GatekeeperCall::AnsweringCall);
    PINDEX idx = activeCalls.GetValuesIndex(search);
    if (idx != P_MAX_INDEX)
      activeCalls[idx].OnInfoResponse(info, perCallInfo);
    else {
      PTRACE(2, "RAS\tEndpoint has call gk does not know about: " << search);
    }
  }

  UnlockReadOnly();

  return H323GatekeeperRequest::Confirm;
}


BOOL H323RegisteredEndPoint::HasExceededTimeToLive() const
{
  BOOL exceeded = FALSE;

  LockReadOnly();

  if (timeToLive != 0) {
    PTime now;
    PTimeInterval delta = now - lastRegistration;
    if ((unsigned)delta.GetSeconds() > timeToLive)
      exceeded = TRUE;
  }

  UnlockReadOnly();

  return exceeded;
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperListener::H323GatekeeperListener(H323EndPoint & ep,
                                               H323GatekeeperServer & gk,
                                               const PString & id,
                                               OpalTransport * trans)
  : H225_RAS(ep, trans),
    gatekeeper(gk)
{
  gatekeeperIdentifier = id;
  currentEP = NULL;

  transport->SetPromiscuous(TRUE);

  PTRACE(2, "H323gk\tGatekeeper server created.");
}


H323GatekeeperListener::~H323GatekeeperListener()
{
  PTRACE(2, "H323gk\tGatekeeper server destroyed.");
}


BOOL H323GatekeeperListener::CheckGatekeeperIdentifier(H323GatekeeperRequest & info,
                                                       unsigned optionalField,
                                                       unsigned rejectTag)
{
  if (!info.GetRawPDU().HasOptionalField(optionalField))
    return TRUE;

  PString gkid = info.GetGatekeeperIdentifier();

  if (gatekeeperIdentifier == gkid)
    return TRUE;

  info.SetRejectReason(rejectTag);
  PTRACE(2, "RAS\t" << lastReceivedPDU->GetTagName()
         << " rejected, has different identifier, got \"" << gkid << '"');
  return FALSE;
}


BOOL H323GatekeeperListener::GetRegisteredEndPoint(H323GatekeeperRequest & info,
                                                   unsigned rejectTag,
                                                   unsigned securityRejectTag,
                                                   unsigned securityField)
{
  PTRACE_BLOCK("H323GatekeeperListener::GetRegisteredEndPoint");

  PString id = info.GetEndpointIdentifier();
  info.endpoint = gatekeeper.FindEndPointByIdentifier(id);
  if (info.endpoint == NULL) {
    info.SetRejectReason(rejectTag);
    PTRACE(2, "RAS\t" << lastReceivedPDU->GetTagName()
           << " rejected, \"" << id << "\" not registered");
    return FALSE;
  }

  if (!info.CheckCryptoTokens(securityField)) {
    info.SetRejectReason(securityRejectTag);
    PTRACE(2, "RAS\t" << lastReceivedPDU->GetTagName()
           << " rejected, security tokens for \"" << id << "\" invalid.");
    return FALSE;
  }

  PTRACE(4, "RAS\tLocated endpoint: " << *info.endpoint);
  return TRUE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnDiscovery(H323GatekeeperGRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnDiscovery");

  if (info.grq.m_protocolIdentifier.GetSize() != 6 || info.grq.m_protocolIdentifier[5] < 2) {
    info.SetRejectReason(H225_GatekeeperRejectReason::e_invalidRevision);
    PTRACE(2, "RAS\tGRQ rejected, version 1 not supported");
    return H323GatekeeperRequest::Reject;
  }

  if (!CheckGatekeeperIdentifier(info, H225_GatekeeperRequest::e_gatekeeperIdentifier,
                                       H225_GatekeeperRejectReason::e_terminalExcluded))
    return H323GatekeeperRequest::Reject;

  H323TransportAddress address = transport->GetLocalAddress();
  PTRACE(3, "RAS\tGRQ accepted on " << address);
  address.SetPDU(info.gcf.m_rasAddress);

  return gatekeeper.OnDiscovery(info);
}


BOOL H323GatekeeperListener::OnReceiveGatekeeperRequest(const H225_GatekeeperRequest & grq)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveGatekeeperRequest");

  H323GatekeeperGRQ * info = new H323GatekeeperGRQ(*this, grq);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnRegistration(H323GatekeeperRRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnRegistration");

  if (info.rrq.m_keepAlive && info.rrq.HasOptionalField(H225_RegistrationRequest::e_endpointIdentifier))
    info.endpoint = gatekeeper.FindEndPointByIdentifier(info.rrq.m_endpointIdentifier);

  if (!CheckGatekeeperIdentifier(info, H225_RegistrationRequest::e_gatekeeperIdentifier,
                                       H225_RegistrationRejectReason::e_undefinedReason))
    return H323GatekeeperRequest::Reject;

  if (info.rrq.m_protocolIdentifier.GetSize() != 6 || info.rrq.m_protocolIdentifier[5] < 2) {
    info.SetRejectReason(H225_RegistrationRejectReason::e_invalidRevision);
    PTRACE(2, "RAS\tRRQ rejected, version 1 not supported");
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response = gatekeeper.OnRegistration(info);
  switch (response) {
    case H323GatekeeperRequest::Reject :
      PTRACE(2, "RAS\tRRQ rejected: " << info.rrj.m_rejectReason.GetTagName());
      break;

    case H323GatekeeperRequest::Confirm :
      if (!info.CheckCryptoTokens(H225_RegistrationRequest::e_cryptoTokens)) {
        info.SetRejectReason(H225_RegistrationRejectReason::e_securityDenial);
        PTRACE(2, "RAS\tRRQ rejected, security tokens for \"" << info.rcf.m_endpointIdentifier.GetValue() << "\" invalid.");
        return H323GatekeeperRequest::Reject;
      }

      PTRACE(2, "RAS\tRRQ accepted: \"" << info.rcf.m_endpointIdentifier.GetValue() << '"');

      SetUpCallSignalAddresses(info.rcf.m_callSignalAddress);
      break;

    default :
      break;
  }

  return response;
}


BOOL H323GatekeeperListener::OnReceiveRegistrationRequest(const H225_RegistrationRequest & rrq)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveRegistrationRequest");

  H323GatekeeperRRQ * info = new H323GatekeeperRRQ(*this, rrq);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnUnregistration(H323GatekeeperURQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnUnregistration");

  if (info.urq.HasOptionalField(H225_RegistrationRequest::e_endpointIdentifier))
    info.endpoint = gatekeeper.FindEndPointByIdentifier(info.urq.m_endpointIdentifier);
  else
    info.endpoint = gatekeeper.FindEndPointBySignalAddresses(info.urq.m_callSignalAddress);

  if (info.endpoint == NULL) {
    info.SetRejectReason(H225_UnregRejectReason::e_notCurrentlyRegistered);
    PTRACE(2, "RAS\tURQ rejected, not registered");
    return H323GatekeeperRequest::Reject;
  }

  return gatekeeper.OnUnregistration(info);
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & urq)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveUnregistrationRequest");

  H323GatekeeperURQ * info = new H323GatekeeperURQ(*this, urq);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveUnregistrationConfirm");

  if (!H225_RAS::OnReceiveUnregistrationConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationReject(const H225_UnregistrationReject & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveUnregistrationReject");

  if (!H225_RAS::OnReceiveUnregistrationReject(pdu))
    return FALSE;

  return TRUE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnAdmission(H323GatekeeperARQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnAdmission");

  if (!GetRegisteredEndPoint(info, H225_AdmissionRejectReason::e_invalidEndpointIdentifier,
                                   H225_AdmissionRejectReason::e_securityDenial,
                                   H225_AdmissionRequest::e_cryptoTokens))
    return H323GatekeeperRequest::Reject;

  if (!CheckGatekeeperIdentifier(info, H225_AdmissionRequest::e_gatekeeperIdentifier,
                                       H225_AdmissionRejectReason::e_undefinedReason))
    return H323GatekeeperRequest::Reject;

  H323GatekeeperRequest::Response response = gatekeeper.OnAdmission(info);
  if (response != H323GatekeeperRequest::Confirm)
    return response;

  if (info.acf.m_callModel.GetTag() == H225_CallModel::e_gatekeeperRouted) {
    H225_ArrayOf_TransportAddress addresses;
    if (SetUpCallSignalAddresses(addresses))
      info.acf.m_destCallSignalAddress = addresses[0];
  }

  return H323GatekeeperRequest::Confirm;
}


BOOL H323GatekeeperListener::OnReceiveAdmissionRequest(const H225_AdmissionRequest & arq)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveAdmissionRequest");

  H323GatekeeperARQ * info = new H323GatekeeperARQ(*this, arq);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnDisengage(H323GatekeeperDRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnDisengage");

  if (!GetRegisteredEndPoint(info, H225_DisengageRejectReason::e_notRegistered,
                                   H225_DisengageRejectReason::e_securityDenial,
                                   H225_DisengageRequest::e_cryptoTokens))
    return H323GatekeeperRequest::Reject;

  if (!CheckGatekeeperIdentifier(info, H225_DisengageRequest::e_gatekeeperIdentifier,
                                       H225_DisengageRejectReason::e_notRegistered))
    return H323GatekeeperRequest::Reject;

  return gatekeeper.OnDisengage(info);
}


BOOL H323GatekeeperListener::OnReceiveDisengageRequest(const H225_DisengageRequest & drq)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveDisengageRequest");

  H323GatekeeperDRQ * info = new H323GatekeeperDRQ(*this, drq);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


BOOL H323GatekeeperListener::OnReceiveDisengageConfirm(const H225_DisengageConfirm & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveDisengageConfirm");

  if (!H225_RAS::OnReceiveDisengageConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveDisengageReject(const H225_DisengageReject & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveDisengageReject");

  if (!H225_RAS::OnReceiveDisengageReject(pdu))
    return FALSE;

  return TRUE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnBandwidth(H323GatekeeperBRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnBandwidth");

  if (!GetRegisteredEndPoint(info, H225_BandRejectReason::e_notBound,
                                   H225_BandRejectReason::e_securityDenial,
                                   H225_BandwidthRequest::e_cryptoTokens))
    return H323GatekeeperRequest::Reject;

  if (!CheckGatekeeperIdentifier(info, H225_BandwidthRequest::e_gatekeeperIdentifier,
                                       H225_BandRejectReason::e_undefinedReason))
    return H323GatekeeperRequest::Reject;

  return gatekeeper.OnBandwidth(info);
}


BOOL H323GatekeeperListener::OnReceiveBandwidthRequest(const H225_BandwidthRequest & brq)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveBandwidthRequest");

  H323GatekeeperBRQ * info = new H323GatekeeperBRQ(*this, brq);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


BOOL H323GatekeeperListener::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveBandwidthConfirm");

  if (!H225_RAS::OnReceiveBandwidthConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveBandwidthReject(const H225_BandwidthReject & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveBandwidthReject");

  if (!H225_RAS::OnReceiveBandwidthReject(pdu))
    return FALSE;

  return TRUE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnLocation(H323GatekeeperLRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnLocation");

  if (info.lrq.HasOptionalField(H225_LocationRequest::e_endpointIdentifier)) {
    if (!GetRegisteredEndPoint(info, H225_LocationRejectReason::e_notRegistered,
                                     H225_LocationRejectReason::e_securityDenial,
                                     H225_LocationRequest::e_cryptoTokens))
      return H323GatekeeperRequest::Reject;
  }

  if (!CheckGatekeeperIdentifier(info, H225_LocationRequest::e_gatekeeperIdentifier,
                                       H225_LocationRejectReason::e_undefinedReason))
    return H323GatekeeperRequest::Reject;

  return gatekeeper.OnLocation(info);
}


BOOL H323GatekeeperListener::OnReceiveLocationRequest(const H225_LocationRequest & lrq)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveLocationRequest");

  H323GatekeeperLRQ * info = new H323GatekeeperLRQ(*this, lrq);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnInfoResponse(H323GatekeeperIRR & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnInfoResponse");

  H323GatekeeperRequest::Response response;
  if (GetRegisteredEndPoint(info, H225_InfoRequestNakReason::e_notRegistered,
                                   H225_InfoRequestNakReason::e_securityDenial,
                                   H225_InfoRequestResponse::e_cryptoTokens))
    response = gatekeeper.OnInfoResponse(info);
  else
    response = H323GatekeeperRequest::Reject;

  if (info.irr.m_unsolicited)
    return response;

  return H323GatekeeperRequest::Ignore;
}


BOOL H323GatekeeperListener::OnReceiveInfoRequestResponse(const H225_InfoRequestResponse & irr)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveInfoRequestResponse");

  if (!irr.m_unsolicited) {
    if (!H225_RAS::OnReceiveInfoRequestResponse(irr))
      return FALSE;
  }

  H323GatekeeperIRR * info = new H323GatekeeperIRR(*this, irr);
  if (!info->HandlePDU())
    delete info;

  return !irr.m_unsolicited;
}


BOOL H323GatekeeperListener::OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveResourcesAvailableConfirm");

  if (!H225_RAS::OnReceiveResourcesAvailableConfirm(pdu))
    return FALSE;

  return TRUE;
}


H235Authenticators H323GatekeeperListener::GetAuthenticators() const
{
  H235Authenticators empty;
  if (currentEP == NULL)
    return empty;

  H235Authenticators authenticators = currentEP->GetAuthenticators();
  if (authenticators.IsEmpty())
    return empty;

  for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
    H235Authenticator & authenticator = authenticators[i];
    if (authenticator.UseGkAndEpIdentifiers())
      authenticator.SetLocalId(gatekeeperIdentifier);
  }

  return authenticators;
}


void H323GatekeeperListener::AttachCurrentEP(H323RegisteredEndPoint * ep)
{
  epMutex.Wait();
  currentEP = ep;
}


void H323GatekeeperListener::DetachCurrentEP()
{
  currentEP = NULL;
  epMutex.Signal();
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperServer::H323GatekeeperServer(H323EndPoint & ep)
  : endpoint(ep)
{
  availableBandwidth = UINT_MAX;  // Unlimited total bandwidth
  defaultBandwidth = 2560;        // Enough for bidirectional G.711 and 64k H.261
  maximumBandwidth = 200000;      // 10baseX LAN bandwidth
  defaultTimeToLive = 3600;       // One hour, zero disables
  defaultInfoResponseRate = 60;   // One minute, zero disables
  overwriteOnSameSignalAddress = TRUE;
  canOnlyCallRegisteredEP = FALSE;
  canOnlyAnswerRegisteredEP = FALSE;
  answerCallPreGrantedARQ = FALSE;
  makeCallPreGrantedARQ = FALSE;
  isGatekeeperRouted = FALSE;
  aliasCanBeHostName = TRUE;
  requireH235 = FALSE;

  identifierBase = time(NULL);
  nextIdentifier = 1;

  byAddress.DisallowDeleteObjects();
  byAlias.DisallowDeleteObjects();
  byVoicePrefix.DisallowDeleteObjects();

  monitorThread = PThread::Create(PCREATE_NOTIFIER(MonitorMain), 0,
                                  PThread::NoAutoDeleteThread,
                                  PThread::NormalPriority,
                                  "GkSrv Monitor");
}


H323GatekeeperServer::~H323GatekeeperServer()
{
  monitorExit.Signal();
  PAssert(monitorThread->WaitForTermination(10000), "Gatekeeper monitor thread did not terminate!");
  delete monitorThread;
}


BOOL H323GatekeeperServer::AddListeners(const OpalTransportAddressArray & ifaces)
{
  if (ifaces.IsEmpty())
    return AddListener("*");

  PINDEX i;

  mutex.Wait();
  for (i = 0; i < listeners.GetSize(); i++) {
    BOOL remove = TRUE;
    for (PINDEX j = 0; j < ifaces.GetSize(); j++) {
      if (listeners[i].GetTransport().GetLocalAddress().IsEquivalent(ifaces[j])) {
        remove = FALSE;
        break;
      }
    }
    if (remove) {
      PTRACE(3, "RAS\tRemoving listener " << listeners[i]);
      listeners.RemoveAt(i--);
    }
  }
  mutex.Signal();

  for (i = 0; i < ifaces.GetSize(); i++) {
    if (!ifaces[i])
      AddListener(ifaces[i]);
  }

  return listeners.GetSize() > 0;
}


BOOL H323GatekeeperServer::AddListener(const OpalTransportAddress & interfaceName)
{
  PWaitAndSignal wait(mutex);

  PINDEX i;
  for (i = 0; i < listeners.GetSize(); i++) {
    if (listeners[i].GetTransport().GetLocalAddress().IsEquivalent(interfaceName)) {
      PTRACE(2, "H323\tAlready have listener for " << interfaceName);
      return TRUE;
    }
  }

  PIPSocket::Address addr;
  WORD port = H225_RAS::DefaultRasUdpPort;
  if (!interfaceName.GetIpAndPort(addr, port))
    return AddListener(interfaceName.CreateTransport(endpoint, OpalTransportAddress::FullTSAP));

  if (addr != INADDR_ANY)
    return AddListener(new OpalTransportUDP(endpoint, addr, port));

  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "RAS\tNo interfaces on system!");
    if (!PIPSocket::GetHostAddress(addr))
      return FALSE;
    return AddListener(new OpalTransportUDP(endpoint, addr, port));
  }

  PTRACE(4, "RAS\tAdding interfaces:\n" << setfill('\n') << interfaces << setfill(' '));

  BOOL atLeastOne = FALSE;

  for (i = 0; i < interfaces.GetSize(); i++) {
    addr = interfaces[i].GetAddress();
    if (addr != 0 && addr != PIPSocket::Address()) {
      if (AddListener(new OpalTransportUDP(endpoint, addr, port)))
        atLeastOne = TRUE;
    }
  }

  return atLeastOne;
}


BOOL H323GatekeeperServer::AddListener(OpalTransport * transport)
{
  if (transport == NULL)
    return FALSE;

  if (!transport->IsOpen()) {
    delete transport;
    return FALSE;
  }

  return AddListener(CreateListener(transport));
}


BOOL H323GatekeeperServer::AddListener(H323GatekeeperListener * listener)
{
  if (listener == NULL)
    return FALSE;

  PTRACE(3, "H323\tStarted listener " << *listener);

  mutex.Wait();
  listeners.Append(listener);
  mutex.Signal();

  listener->StartRasChannel();

  return TRUE;
}


H323GatekeeperListener * H323GatekeeperServer::CreateListener(OpalTransport * transport)
{
  return new H323GatekeeperListener(endpoint, *this, gatekeeperIdentifier, transport);
}


BOOL H323GatekeeperServer::RemoveListener(H323GatekeeperListener * listener)
{
  BOOL ok = TRUE;

  mutex.Wait();
  if (listener != NULL) {
    PTRACE(3, "RAS\tRemoving listener " << *listener);
    ok = listeners.Remove(listener);
  }
  else {
    PTRACE(3, "RAS\tRemoving all listeners");
    listeners.RemoveAll();
  }
  mutex.Signal();

  return ok;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnDiscovery(H323GatekeeperGRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnDiscovery");

  H235Authenticators authenticators = CreateAuthenticators();
  for (PINDEX auth = 0; auth < authenticators.GetSize(); auth++) {
    for (PINDEX cap = 0; cap < info.grq.m_authenticationCapability.GetSize(); cap++) {
      for (PINDEX alg = 0; alg < info.grq.m_algorithmOIDs.GetSize(); alg++) {
        if (authenticators[auth].IsCapability(info.grq.m_authenticationCapability[cap],
                                              info.grq.m_algorithmOIDs[alg])) {
          PTRACE(3, "RAS\tGRQ accepted on " << H323TransportAddress(info.gcf.m_rasAddress)
                 << " using authenticator " << authenticators[auth]);
          info.gcf.IncludeOptionalField(H225_GatekeeperConfirm::e_authenticationMode);
          info.gcf.m_authenticationMode = info.grq.m_authenticationCapability[cap];
          info.gcf.IncludeOptionalField(H225_GatekeeperConfirm::e_algorithmOID);
          info.gcf.m_algorithmOID = info.grq.m_algorithmOIDs[alg];
          return H323GatekeeperRequest::Confirm;
        }
      }
    }
  }

  if (requireH235) {
    info.SetRejectReason(H225_GatekeeperRejectReason::e_securityDenial);
    return H323GatekeeperRequest::Reject;
  }
  else {
    PTRACE(3, "RAS\tGRQ accepted on " << H323TransportAddress(info.gcf.m_rasAddress));
    return H323GatekeeperRequest::Confirm;
  }
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnRegistration(H323GatekeeperRRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnRegistration");

  PINDEX i;

  // Initialise reply with default stuff
  info.rcf.IncludeOptionalField(H225_RegistrationConfirm::e_preGrantedARQ);
  info.rcf.m_preGrantedARQ.m_answerCall = answerCallPreGrantedARQ;
  info.rcf.m_preGrantedARQ.m_useGKCallSignalAddressToAnswer = answerCallPreGrantedARQ && isGatekeeperRouted;
  info.rcf.m_preGrantedARQ.m_makeCall = makeCallPreGrantedARQ;
  info.rcf.m_preGrantedARQ.m_useGKCallSignalAddressToMakeCall = makeCallPreGrantedARQ && isGatekeeperRouted;
  if (defaultInfoResponseRate > 0) {
    info.rcf.m_preGrantedARQ.IncludeOptionalField(H225_RegistrationConfirm_preGrantedARQ::e_irrFrequencyInCall);
    info.rcf.m_preGrantedARQ.m_irrFrequencyInCall = defaultInfoResponseRate;
  }
  info.rcf.m_willRespondToIRR = TRUE;

  if (info.rrq.m_keepAlive) {
    if (info.endpoint == NULL) {
      info.SetRejectReason(H225_RegistrationRejectReason::e_fullRegistrationRequired);
      PTRACE(2, "RAS\tRRQ keep alive rejected, not registered");
      return H323GatekeeperRequest::Reject;
    }

    return info.endpoint->OnRegistration(info);
  }

  for (i = 0; i < info.rrq.m_callSignalAddress.GetSize(); i++) {
    PSafePtr<H323RegisteredEndPoint> ep2 = FindEndPointBySignalAddress(info.rrq.m_callSignalAddress[i]);
    if (ep2 != NULL && ep2 != info.endpoint) {
      if (overwriteOnSameSignalAddress) {
        PTRACE(2, "RAS\tOverwriting existing endpoint " << *ep2);
        RemoveEndPoint(ep2);
      }
      else {
        info.SetRejectReason(H225_RegistrationRejectReason::e_invalidCallSignalAddress);
        PTRACE(2, "RAS\tRRQ rejected, duplicate callSignalAddress");
        return H323GatekeeperRequest::Reject;
      }
    }
  }

  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_terminalAlias)) {
    for (i = 0; i < info.rrq.m_terminalAlias.GetSize(); i++) {
      PSafePtr<H323RegisteredEndPoint> ep2 = FindEndPointByAliasAddress(info.rrq.m_terminalAlias[i]);
      if (ep2 != NULL && ep2 != info.endpoint) {
        info.SetRejectReason(H225_RegistrationRejectReason::e_duplicateAlias);
        ((H225_AliasAddress&)info.rrj.m_rejectReason) = info.rrq.m_terminalAlias[i];
        PTRACE(2, "RAS\tRRQ rejected, duplicate alias");
        return H323GatekeeperRequest::Reject;
      }
    }
  }

  // Check if the endpoint is trying to register a prefix that can be resolved to another endpoint
  const H225_EndpointType & terminalType = info.rrq.m_terminalType;
  if (terminalType.HasOptionalField(H225_EndpointType::e_gateway) &&
      terminalType.m_gateway.HasOptionalField(H225_GatewayInfo::e_protocol)) {
    const H225_ArrayOf_SupportedProtocols & protocols = terminalType.m_gateway.m_protocol;
    for (i = 0; i < protocols.GetSize(); i++) {

      // Only voice prefixes are supported
      if (protocols[i].GetTag() == H225_SupportedProtocols::e_voice) {
	H225_VoiceCaps & voiceCaps = protocols[i];
	if (voiceCaps.HasOptionalField(H225_VoiceCaps::e_supportedPrefixes)) {
	  H225_ArrayOf_SupportedPrefix & prefixes = voiceCaps.m_supportedPrefixes;
	  for (PINDEX j = 0; j < prefixes.GetSize(); j++) {

	    // Reject if the prefix be matched to a registered alias or prefix
	    PSafePtr<H323RegisteredEndPoint> ep2 = FindEndPointByAliasAddress(prefixes[j].m_prefix);
	    if (ep2 != NULL && ep2 != info.endpoint) {
	      info.SetRejectReason(H225_RegistrationRejectReason::e_duplicateAlias);
	      ((H225_AliasAddress&)info.rrj.m_rejectReason) = prefixes[j].m_prefix;
	      PTRACE(2, "RAS\tRRQ rejected, duplicate prefix");
	      return H323GatekeeperRequest::Reject;
	    }
	  }
	}
	break;  // If voice protocol is found, don't look any further
      }
    }
  }

  if (info.endpoint != NULL)
    return info.endpoint->OnRegistration(info);

  // Need to create a new endpoint object
  H323RegisteredEndPoint * newEP = CreateRegisteredEndPoint(info);
  if (newEP == NULL) {
    PTRACE(1, "RAS\tRRQ rejected, CreateRegisteredEndPoint() returned NULL");
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response = newEP->OnRegistration(info);
  if (response != H323GatekeeperRequest::Confirm)
    delete newEP;
  else {
    info.endpoint = newEP;
    AddEndPoint(newEP);
  }

  return response;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnUnregistration(H323GatekeeperURQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnUnregistration");

  PINDEX i;

  if (info.urq.HasOptionalField(H225_UnregistrationRequest::e_endpointAlias)) {
    // See if all aliases to be removed are on the same endpoint
    for (i = 0; i < info.urq.m_endpointAlias.GetSize(); i++) {
      if (FindEndPointByAliasAddress(info.urq.m_endpointAlias[i]) != info.endpoint) {
        info.SetRejectReason(H225_UnregRejectReason::e_permissionDenied);
        PTRACE(2, "RAS\tURQ rejected, alias " << info.urq.m_endpointAlias[i]
               << " not owned by registration");
        return H323GatekeeperRequest::Reject;
      }
    }
    // Remove all the aliases specified in PDU, if no aliases left, then
    // RemoveAlias() will remove the endpoint
    for (i = 0; i < info.urq.m_endpointAlias.GetSize(); i++) {
      RemoveAlias(H323GetAliasAddressString(info.urq.m_endpointAlias[i]));
    }
  }
  else
    RemoveEndPoint(info.endpoint);

  return info.endpoint->OnUnregistration(info);
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnInfoResponse(H323GatekeeperIRR & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnInfoResponse");

  return info.endpoint->OnInfoResponse(info);
}


void H323GatekeeperServer::AddEndPoint(H323RegisteredEndPoint * ep)
{
  PTRACE(3, "RAS\tAdding registered endpoint: " << *ep);

  PINDEX i;

  mutex.Wait();

  byIdentifier.SetAt(ep->GetIdentifier(), ep);

  for (i = 0; i < ep->GetSignalAddressCount(); i++)
    byAddress.SetAt(ep->GetSignalAddress(i), ep->GetIdentifier());

  for (i = 0; i < ep->GetAliasCount(); i++)
    byAlias.SetAt(ep->GetAlias(i), ep->GetIdentifier());

  for (i = 0; i < ep->GetPrefixCount(); i++)
    byVoicePrefix.SetAt(ep->GetPrefix(i), ep->GetIdentifier());

  mutex.Signal();
}


void H323GatekeeperServer::RemoveEndPoint(H323RegisteredEndPoint * ep)
{
  PTRACE(3, "RAS\tRemoving registered endpoint: " << *ep);

  PINDEX i;

  mutex.Wait();

  for (i = 0; i < ep->GetPrefixCount(); i++)
    byVoicePrefix.RemoveAt(ep->GetPrefix(i));

  for (i = 0; i < ep->GetAliasCount(); i++)
    byAlias.RemoveAt(ep->GetAlias(i));

  for (i = 0; i < ep->GetSignalAddressCount(); i++)
    byAddress.RemoveAt(ep->GetSignalAddress(i));

  byIdentifier.RemoveAt(ep->GetIdentifier());    // ep is deleted by this

  mutex.Signal();
}


BOOL H323GatekeeperServer::RemoveAlias(const PString & alias)
{
  PTRACE(3, "RAS\tRemoving registered endpoint alias: " << alias);

  BOOL removedEP = FALSE;

  mutex.Wait();
  byAlias.RemoveAt(alias);
  mutex.Signal();

  PSafePtr<H323RegisteredEndPoint> ep = FindEndPointByAliasString(alias, PSafeReadWrite);
  if (ep != NULL) {
    ep->RemoveAlias(alias);
    if (ep->GetAliasCount() == 0) {
      RemoveEndPoint(ep);
      removedEP = TRUE;
    }
  }

  return removedEP;
}


H323RegisteredEndPoint * H323GatekeeperServer::CreateRegisteredEndPoint(H323GatekeeperRRQ &)
{
  return new H323RegisteredEndPoint(*this, CreateEndPointIdentifier());
}


PString H323GatekeeperServer::CreateEndPointIdentifier()
{
  PWaitAndSignal wait(mutex);
  return psprintf("%x:%u", identifierBase, nextIdentifier++);
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointByIdentifier(
                                            const PString & identifier, PSafetyMode mode)
{
  PWaitAndSignal wait(mutex);
  return byIdentifier.FindWithLock(identifier, mode);
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointBySignalAddresses(
                            const H225_ArrayOf_TransportAddress & addresses, PSafetyMode mode)
{
  PWaitAndSignal wait(mutex);

  PString id;
  PINDEX i = 0;
  while (i < addresses.GetSize() &&
                (id = byAddress(H323TransportAddress(addresses[i]))).IsEmpty())
    i++;

  return FindEndPointByIdentifier(id, mode);
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointBySignalAddress(
                                     const H323TransportAddress & address, PSafetyMode mode)
{
  PWaitAndSignal wait(mutex);
  return FindEndPointByIdentifier(byAddress(address), mode);
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointByAliasAddress(
                                         const H225_AliasAddress & alias, PSafetyMode mode)
{
  return FindEndPointByAliasString(H323GetAliasAddressString(alias), mode);
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointByAliasString(
                                                  const PString & alias, PSafetyMode mode)
{
  PWaitAndSignal wait(mutex);
  PString id = byAlias(alias);
  
  // If alias is not registered, check if there is a prefix that matches any substring of the alias.
  // ( Would it be more efficient to check every registered prefix instead ? )
  if (id.IsEmpty()) {
    PINDEX i = alias.GetLength();
    while (id.IsEmpty() && i > 0)
      id = byVoicePrefix(alias.Left(i--));
  }

  return FindEndPointByIdentifier(id, mode);
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnAdmission(H323GatekeeperARQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnAdmission");

  OpalGloballyUniqueID id = info.arq.m_callIdentifier.m_guid;
  if (id == NULL) {
    PTRACE(2, "RAS\tNo call identifier provided in ARQ!");
    info.SetRejectReason(H225_AdmissionRejectReason::e_undefinedReason);
    return H323GatekeeperRequest::Reject;
  }

  PSafePtr<H323GatekeeperCall> oldCall = FindCall(id, info.arq.m_answerCall);
  if (oldCall != NULL) {
    H323GatekeeperRequest::Response response = oldCall->OnAdmission(info);
    if (response == H323GatekeeperRequest::Reject)
      RemoveCall(oldCall);
    return response;
  }

  H323GatekeeperCall * newCall = CreateCall(id,
                          info.arq.m_answerCall ? H323GatekeeperCall::AnsweringCall
                                                : H323GatekeeperCall::OriginatingCall);

  H323GatekeeperRequest::Response response = newCall->OnAdmission(info);

  if (response != H323GatekeeperRequest::Reject) {
    info.endpoint->AddCall(newCall);
    activeCalls.Append(newCall);
    PTRACE(2, "RAS\tAdded new call (total=" << activeCalls.GetSize() << ") " << *newCall);
  }

  return response;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnDisengage(H323GatekeeperDRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnDisengage");

  OpalGloballyUniqueID callIdentifier = info.drq.m_callIdentifier.m_guid;
  PSafePtr<H323GatekeeperCall> call = FindCall(callIdentifier, info.drq.m_answeredCall);
  if (call == NULL) {
    info.SetRejectReason(H225_DisengageRejectReason::e_requestToDropOther);
    PTRACE(2, "RAS\tDRQ rejected, no call with ID " << callIdentifier);
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response = call->OnDisengage(info);
  if (response != H323GatekeeperRequest::Confirm)
    return response;

  RemoveCall(call);
  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnBandwidth(H323GatekeeperBRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnBandwidth");

  PSafePtr<H323GatekeeperCall> call = FindCall(info.brq.m_callIdentifier.m_guid, info.brq.m_answeredCall);
  if (call == NULL) {
    info.SetRejectReason(H225_BandRejectReason::e_invalidConferenceID);
    PTRACE(2, "RAS\tBRQ rejected, no call with ID");
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response = call->OnBandwidth(info);
  return response;
}


BOOL H323GatekeeperServer::GetUsersPassword(const PString & alias,
                                            PString & password) const
{
  password = passwords(alias);
  if (!password)
    return TRUE;

  return !requireH235;
}


H235Authenticators H323GatekeeperServer::CreateAuthenticators() const
{
  H235Authenticators authenticators;

#if P_SSL
  authenticators.Append(new H235AuthProcedure1);
#endif
  authenticators.Append(new H235AuthSimpleMD5);

  return authenticators;
}


unsigned H323GatekeeperServer::AllocateBandwidth(unsigned newBandwidth,
                                                 unsigned oldBandwidth)
{
  PWaitAndSignal wait(mutex);

  // If first request for bandwidth, then only give them a maximum of the
  // configured default bandwidth
  if (oldBandwidth == 0 && newBandwidth > defaultBandwidth)
    newBandwidth = defaultBandwidth;

  // If then are asking for more than we have in total, drop it down to whatevers left
  if (newBandwidth > oldBandwidth && (newBandwidth - oldBandwidth) > availableBandwidth)
    newBandwidth = availableBandwidth  - oldBandwidth;

  // If greater than the absolute maximum configured for any endpoint, clamp it
  if (newBandwidth > maximumBandwidth)
    newBandwidth = maximumBandwidth;

  // Finally have adjusted new bandwidth, allocate it!
  availableBandwidth -= (newBandwidth - oldBandwidth);
  return newBandwidth;
}


void H323GatekeeperServer::RemoveCall(H323GatekeeperCall * call)
{
  PAssertNULL(call);

  AllocateBandwidth(0, call->GetBandwidthUsed());
  PAssert(call->GetEndPoint().RemoveCall(call), PLogicError);

  PTRACE(2, "RAS\tRemoved call (total=" << (activeCalls.GetSize()-1) << ") id=" << *call);
  PAssert(activeCalls.Remove(call), PLogicError);
}


H323GatekeeperCall * H323GatekeeperServer::CreateCall(const OpalGloballyUniqueID & id,
                                                      H323GatekeeperCall::Direction dir)
{
  return new H323GatekeeperCall(*this, id, dir);
}


PSafePtr<H323GatekeeperCall> H323GatekeeperServer::FindCall(const OpalGloballyUniqueID & id,
                                                            BOOL answer,
                                                            PSafetyMode mode)
{
  return FindCall(id, answer ? H323GatekeeperCall::AnsweringCall
                             : H323GatekeeperCall::OriginatingCall, mode);
}


PSafePtr<H323GatekeeperCall> H323GatekeeperServer::FindCall(const OpalGloballyUniqueID & id,
                                                            H323GatekeeperCall::Direction dir,
                                                            PSafetyMode mode)
{
  return activeCalls.FindWithLock(H323GatekeeperCall(*this, id, dir), mode);
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnLocation(H323GatekeeperLRQ & info)
{
  PINDEX i;
  for (i = 0; i < info.lrq.m_destinationInfo.GetSize(); i++) {
    H323TransportAddress address;
    if (TranslateAliasAddressToSignalAddress(info.lrq.m_destinationInfo[i], address)) {
      address.SetPDU(info.lcf.m_callSignalAddress);
      return H323GatekeeperRequest::Confirm;
    }
  }

  info.SetRejectReason(H225_LocationRejectReason::e_requestDenied);
  PTRACE(2, "RAS\tLRQ rejected, not found");
  return H323GatekeeperRequest::Reject;
}


BOOL H323GatekeeperServer::TranslateAliasAddressToSignalAddress(const H225_AliasAddress & alias,
                                                                H323TransportAddress & address)
{
  PWaitAndSignal wait(mutex);

  PString aliasString = H323GetAliasAddressString(alias);

  if (isGatekeeperRouted) {
    const OpalListenerList & listeners = endpoint.GetListeners();
    address = listeners[0].GetLocalAddress();
    PTRACE(2, "RAS\tTranslating alias " << aliasString << " to " << address << ", gatekeeper routed");
    return TRUE;
  }

  PSafePtr<H323RegisteredEndPoint> ep = FindEndPointByAliasAddress(alias, PSafeReadOnly);
  if (ep != NULL) {
    address = ep->GetSignalAddress(0);
    PTRACE(2, "RAS\tTranslating alias " << aliasString << " to " << address << ", registered endpoint");
    return TRUE;
  }

  if (aliasCanBeHostName) {
    PIPSocket::Address ip;
    if (PIPSocket::GetHostAddress(aliasString, ip)) {
      address = H323TransportAddress(ip, endpoint.GetDefaultSignalPort());
      PTRACE(2, "RAS\tTranslating alias " << aliasString << " to " << address << ", DNS host name");
    }
  }

  return !address;
}


BOOL H323GatekeeperServer::CheckSignalAddressPolicy(const H323RegisteredEndPoint &,
                                                    const H225_AdmissionRequest &,
                                                    const OpalTransportAddress &)
{
  return TRUE;
}


BOOL H323GatekeeperServer::CheckAliasAddressPolicy(const H323RegisteredEndPoint &,
                                                   const H225_AdmissionRequest & arq,
                                                   const H225_AliasAddress & alias)
{
  PWaitAndSignal wait(mutex);

  if (arq.m_answerCall ? canOnlyAnswerRegisteredEP : canOnlyCallRegisteredEP) {
    PSafePtr<H323RegisteredEndPoint> ep = FindEndPointByAliasAddress(alias);
    if (ep == NULL)
      return FALSE;
  }

  return TRUE;
}


BOOL H323GatekeeperServer::CheckAliasStringPolicy(const H323RegisteredEndPoint &,
                                                  const H225_AdmissionRequest & arq,
                                                  const PString & alias)
{
  PWaitAndSignal wait(mutex);

  if (arq.m_answerCall ? canOnlyAnswerRegisteredEP : canOnlyCallRegisteredEP) {
    PSafePtr<H323RegisteredEndPoint> ep = FindEndPointByAliasString(alias);
    if (ep == NULL)
      return FALSE;
  }

  return TRUE;
}


void H323GatekeeperServer::SetGatekeeperIdentifier(const PString & id,
                                                   BOOL adjustListeners)
{
  mutex.Wait();

  gatekeeperIdentifier = id;

  if (adjustListeners) {
    for (PINDEX i = 0; i < listeners.GetSize(); i++)
      listeners[i].SetIdentifier(id);
  }

  mutex.Signal();
}


void H323GatekeeperServer::MonitorMain(PThread &, INT)
{
  while (!monitorExit.Wait(1000)) {
    PTRACE(6, "RAS\tAging registered endpoints");

    for (PSafePtr<H323RegisteredEndPoint> ep = GetFirstEndPoint(PSafeReadOnly); ep != NULL; ep++) {
      if (ep->HasExceededTimeToLive()) {
        PTRACE(2, "RAS\tRemoving expired endpoint");
        RemoveEndPoint(ep);
      }
    }

    byIdentifier.DeleteObjectsToBeRemoved();

    for (PSafePtr<H323GatekeeperCall> call = GetFirstCall(PSafeReadOnly); call != NULL; call++) {
      if (!call->OnHeartbeat())
        call->Disengage();
    }

    activeCalls.DeleteObjectsToBeRemoved();
  }
}


/////////////////////////////////////////////////////////////////////////////
