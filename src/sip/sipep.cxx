/*
 * sipep.cxx
 *
 * Session Initiation Protocol endpoint.
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
 * $Log: sipep.cxx,v $
 * Revision 1.2056  2005/06/02 13:18:02  rjongbloed
 * Fixed compiler warnings
 *
 * Revision 2.54  2005/05/25 18:36:07  dsandras
 * Fixed unregistration of all accounts when exiting.
 *
 * Revision 2.53  2005/05/23 20:14:00  dsandras
 * Added preliminary support for basic instant messenging.
 *
 * Revision 2.52  2005/05/13 12:47:51  dsandras
 * Instantly remove unregistration from the collection, and do not process
 * removed SIPInfo objects, thanks to Ted Szoczei.
 *
 * Revision 2.51  2005/05/12 19:44:12  dsandras
 * Fixed leak thanks to Ted Szoczei.
 *
 * Revision 2.50  2005/05/06 07:37:06  csoutheren
 * Various changed while working with SIP carrier
 *   - remove assumption that authentication realm is a domain name.
 *   - stopped rewrite of "To" field when proxy being used
 *   - fix Contact field in REGISTER to match actual port used when Symmetric NATin use
 *   - lots of formatting changes and cleanups
 *
 * Revision 2.49  2005/05/04 17:10:42  dsandras
 * Get rid of the extra parameters in the Via field before using it to change
 * the transport remote address.
 *
 * Revision 2.48  2005/05/03 20:42:33  dsandras
 * Unregister accounts when exiting the program.
 *
 * Revision 2.47  2005/05/02 19:30:36  dsandras
 * Do not use the realm in the comparison for the case when none is specified.
 *
 * Revision 2.46  2005/04/28 20:22:55  dsandras
 * Applied big sanity patch for SIP thanks to Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.45  2005/04/28 07:59:37  dsandras
 * Applied patch from Ted Szoczei to fix problem when answering to PDUs containing
 * multiple Via fields in the message header. Thanks!
 *
 * Revision 2.44  2005/04/26 19:50:15  dsandras
 * Fixed remoteAddress and user parameters in MWIReceived.
 * Added function to return the number of registered accounts.
 *
 * Revision 2.43  2005/04/25 21:12:51  dsandras
 * Use the correct remote address.
 *
 * Revision 2.42  2005/04/15 10:48:34  dsandras
 * Allow reading on the transport until there is an EOF or it becomes bad. Fixes interoperability problem with QSC.DE which is sending keep-alive messages, leading to a timeout (transport.good() fails, but the stream is still usable).
 *
 * Revision 2.41  2005/04/11 10:31:32  dsandras
 * Cleanups.
 *
 * Revision 2.40  2005/04/11 10:26:30  dsandras
 * Added SetUpTransfer similar to the one from in the H.323 part.
 *
 * Revision 2.39  2005/03/11 18:12:09  dsandras
 * Added support to specify the realm when registering. That way softphones already know what authentication information to use when required. The realm/domain can also be used in the From field.
 *
 * Revision 2.38  2005/02/21 12:20:05  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.37  2005/02/19 22:48:48  dsandras
 * Added the possibility to register to several registrars and be able to do authenticated calls to each of them. Added SUBSCRIBE/NOTIFY support for Message Waiting Indications.
 *
 * Revision 2.36  2005/01/15 23:34:26  dsandras
 * Applied patch from Ted Szoczei to handle broken incoming connections.
 *
 * Revision 2.35  2005/01/09 03:42:46  rjongbloed
 * Fixed warning about unused parameter
 *
 * Revision 2.34  2004/12/27 22:20:17  dsandras
 * Added preliminary support for OPTIONS requests sent outside of a connection.
 *
 * Revision 2.33  2004/12/25 20:45:12  dsandras
 * Only fail a REGISTER when proxy authentication is required and has already
 * been done if the credentials are identical. Fixes registration refresh problem
 * where some registrars request authentication with a new nonce.
 *
 * Revision 2.32  2004/12/22 18:56:39  dsandras
 * Ignore ACK's received outside the context of a connection (e.g. when sending a non-2XX answer to an INVITE).
 *
 * Revision 2.31  2004/12/17 12:06:52  dsandras
 * Added error code to OnRegistrationFailed. Made Register/Unregister wait until the transaction is over. Fixed Unregister so that the SIPRegister is used as a pointer or the object is deleted at the end of the function and make Opal crash when transactions are cleaned. Reverted part of the patch that was sending authentication again when it had already been done on a Register.
 *
 * Revision 2.30  2004/12/12 13:42:31  dsandras
 * - Send back authentication when required when doing a REGISTER as it might be consecutive to an unregister.
 * - Update the registered variable when unregistering from a SIP registrar.
 * - Added OnRegistrationFailed () function to indicate when a registration failed. Call that function at various strategic places.
 *
 * Revision 2.29  2004/10/02 04:30:11  rjongbloed
 * Added unregister function for SIP registrar
 *
 * Revision 2.28  2004/08/22 12:27:46  rjongbloed
 * More work on SIP registration, time to live refresh and deregistration on exit.
 *
 * Revision 2.27  2004/08/20 12:54:48  rjongbloed
 * Fixed crash caused by double delete of registration transactions
 *
 * Revision 2.26  2004/08/18 13:04:56  rjongbloed
 * Fixed trying to start regitration if have empty string for domain/user
 *
 * Revision 2.25  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.24  2004/07/11 12:42:13  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.23  2004/06/05 14:36:32  rjongbloed
 * Added functions to get registration URL.
 * Added ability to set proxy bu host/user/password strings.
 *
 * Revision 2.22  2004/04/26 06:30:35  rjongbloed
 * Added ability to specify more than one defualt listener for an endpoint,
 *   required by SIP which listens on both UDP and TCP.
 *
 * Revision 2.21  2004/04/26 05:40:39  rjongbloed
 * Added RTP statistics callback to SIP
 *
 * Revision 2.20  2004/04/25 08:46:08  rjongbloed
 * Fixed GNU compatibility
 *
 * Revision 2.19  2004/03/29 10:56:31  rjongbloed
 * Made sure SIP UDP socket is "promiscuous", ie the host/port being sent to may not
 *   be where packets come from.
 *
 * Revision 2.18  2004/03/14 11:32:20  rjongbloed
 * Changes to better support SIP proxies.
 *
 * Revision 2.17  2004/03/14 10:13:04  rjongbloed
 * Moved transport on SIP top be constructed by endpoint as any transport created on
 *   an endpoint can receive data for any connection.
 * Changes to REGISTER to support authentication
 *
 * Revision 2.16  2004/03/14 08:34:10  csoutheren
 * Added ability to set User-Agent string
 *
 * Revision 2.15  2004/03/13 06:51:32  rjongbloed
 * Alllowed for empty "username" in registration
 *
 * Revision 2.14  2004/03/13 06:32:18  rjongbloed
 * Fixes for removal of SIP and H.323 subsystems.
 * More registration work.
 *
 * Revision 2.13  2004/03/09 12:09:56  rjongbloed
 * More work on SIP register.
 *
 * Revision 2.12  2004/02/17 12:41:54  csoutheren
 * Use correct Contact field when behind NAT
 *
 * Revision 2.11  2003/12/20 12:21:18  rjongbloed
 * Applied more enhancements, thank you very much Ted Szoczei
 *
 * Revision 2.10  2003/03/24 04:32:58  robertj
 * SIP register must have a server address
 *
 * Revision 2.9  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.8  2002/10/09 04:27:44  robertj
 * Fixed memory leak on error reading PDU, thanks Ted Szoczei
 *
 * Revision 2.7  2002/09/12 06:58:34  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.6  2002/07/04 07:41:47  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.5  2002/04/10 03:15:17  robertj
 * Fixed using incorrect field as default reply address on receiving a respons PDU.
 *
 * Revision 2.4  2002/04/09 08:04:01  robertj
 * Fixed typo in previous patch!
 *
 * Revision 2.3  2002/04/09 07:18:33  robertj
 * Fixed the default return address for PDU requests not attached to connection.
 *
 * Revision 2.2  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sipep.h"
#endif

#include <sip/sipep.h>

#include <sip/sippdu.h>
#include <sip/sipcon.h>

#include <opal/manager.h>
#include <opal/call.h>


#define new PNEW

 
////////////////////////////////////////////////////////////////////////////

SIPInfo::SIPInfo(SIPEndPoint &endpoint, const PString & adjustedUsername)
  :ep(endpoint)
{
  expire = 0;
  registered = FALSE; 
  registrarTransport = NULL;
  registrationAddress.Parse(adjustedUsername);
  registrationID = 
    OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
}


SIPInfo::~SIPInfo() 
{
  if (registrarTransport) 
    delete registrarTransport;
}


BOOL SIPInfo::CreateTransport (OpalTransportAddress & registrarAddress)
{
  if (registrarTransport == NULL)
    registrarTransport = ep.CreateTransport(registrarAddress);
  if (registrarTransport == NULL)
    return FALSE;

  PTRACE (1, "SIP\tCreated Transport for Registrar " << *registrarTransport);

  return TRUE;
}


void SIPInfo::Cancel (SIPTransaction & transaction)
{
  for (PINDEX i = 0; i < registrations.GetSize(); i++) {
    if (&registrations[i] != &transaction) 
      registrations[i].SendCANCEL();
  }
}


SIPRegisterInfo::SIPRegisterInfo(SIPEndPoint & endpoint, const PString & name, const PString & pass/*, const PString & r*/)
  :SIPInfo(endpoint, name)
{
  expire = ep.GetRegistrarTimeToLive().GetSeconds();
  password = pass;
  //authRealm = r;
}


SIPTransaction * SIPRegisterInfo::CreateTransaction(OpalTransport &t, BOOL unregister)
{
  authentication.SetUsername(registrationAddress.GetUserName());
  authentication.SetPassword(password);
  if (!authRealm.IsEmpty())
    authentication.SetAuthRealm(authRealm);   // CRS 06/05/05 check added because zeroes realm in some cases
  
  if (unregister)
    SetExpire (0);
  else
    SetExpire (ep.GetRegistrarTimeToLive().GetSeconds());

  return new SIPRegister (ep, 
			  t, 
			  registrationAddress, 
			  registrationID, 
			  unregister ? 0:expire); 
}

void SIPRegisterInfo::OnSuccess ()
{ 
  SetRegistered((expire == 0)?FALSE:TRUE);
  ep.OnRegistered(registrationAddress.GetHostName(), 
		  registrationAddress.GetUserName(),
		  (expire > 0)); 
}

void SIPRegisterInfo::OnFailed (SIP_PDU::StatusCodes r)
{ 
  SetRegistered((expire == 0)?TRUE:FALSE);
  ep.OnRegistrationFailed (registrationAddress.GetHostName(), 
			   registrationAddress.GetUserName(),
			   r,
			   (expire > 0));
}

SIPMWISubscribeInfo::SIPMWISubscribeInfo (SIPEndPoint & endpoint, const PString & name)
  :SIPInfo(endpoint, name)
{
  expire = ep.GetNotifierTimeToLive().GetSeconds();
}


SIPTransaction * SIPMWISubscribeInfo::CreateTransaction(OpalTransport &t, BOOL unregister)
{ 
  if (unregister)
    SetExpire (0);
  else
    SetExpire (ep.GetRegistrarTimeToLive().GetSeconds());

  return new SIPMWISubscribe (ep, 
			      t, 
			      registrationAddress, 
			      registrationID, 
			      unregister ? 0:expire); 
}

void SIPMWISubscribeInfo::OnSuccess ()
{ 
  SetRegistered((expire == 0)?FALSE:TRUE);
}

void SIPMWISubscribeInfo::OnFailed(SIP_PDU::StatusCodes /*reason*/)
{ 
  SetRegistered((expire == 0)?TRUE:FALSE);
}

////////////////////////////////////////////////////////////////////////////

SIPEndPoint::SIPEndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, "sip", CanTerminateCall),
    retryTimeoutMin(500),     // 0.5 seconds
    retryTimeoutMax(0, 4),    // 4 seconds
    nonInviteTimeout(0, 16),  // 16 seconds
    pduCleanUpTimeout(0, 5),  // 5 seconds
    inviteTimeout(0, 32),     // 32 seconds
    ackTimeout(0, 32),        // 32 seconds
    registrarTimeToLive(0, 0, 0, 1), // 1 hour
    notifierTimeToLive(0, 0, 0, 1) // 1 hour
{
  defaultSignalPort = 5060;
  mimeForm = FALSE;
  maxRetries = 10;
  lastSentCSeq = 0;
  userAgentString = "OPAL/2.0";

  transactions.DisallowDeleteObjects();

  registrationTimer.SetNotifier(PCREATE_NOTIFIER(RegistrationRefresh));
  registrationTimer.RunContinuous (PTimeInterval(0, 60));

  PTRACE(3, "SIP\tCreated endpoint.");
}


SIPEndPoint::~SIPEndPoint()
{
  listeners.RemoveAll();

  /* Unregister */
  while (GetRegistrationsCount() > 0) {
    SIPURL url;
    SIPInfo *info = activeRegistrations.GetAt(0);
    url = info->GetRegistrationAddress ();
    if (info->IsRegistered() && info->GetMethod() == SIP_PDU::Method_REGISTER)
      Unregister(url.GetHostName(), url.GetUserName());
  }

  PTRACE(3, "SIP\tDeleted endpoint.");
}


PStringArray SIPEndPoint::GetDefaultListeners() const
{
  PStringArray listenerAddresses = OpalEndPoint::GetDefaultListeners();
  if (!listenerAddresses.IsEmpty())
    listenerAddresses.AppendString(psprintf("udp$*:%u", defaultSignalPort));
  return listenerAddresses;
}


BOOL SIPEndPoint::NewIncomingConnection(OpalTransport * transport)
{
  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread started.");

  transport->SetBufferSize(SIP_PDU::MaxSize);

  do {
    HandlePDU(*transport);
  } while (transport->IsOpen() && transport->IsReliable() && !transport->bad() && !transport->eof());

  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread finished.");

  return TRUE;
}


void SIPEndPoint::TransportThreadMain(PThread &, INT param)
{
  PTRACE(2, "SIP\tRead thread started.");

  OpalTransport * transport = (OpalTransport *)param;

  do {
    HandlePDU(*transport);
  } while (transport->IsOpen());

  PTRACE(2, "SIP\tRead thread finished.");
}


OpalTransport * SIPEndPoint::CreateTransport(const OpalTransportAddress & address)
{
  PIPSocket::Address ip(PIPSocket::GetDefaultIpAny());
  WORD port = GetDefaultSignalPort();
  if (!listeners.IsEmpty())
    GetListeners()[0].GetLocalAddress().GetIpAndPort(ip, port);

  OpalTransport * transport;
  if (ip.IsAny()) {
    // endpoint is listening to anything - attempt call using all interfaces
    transport = address.CreateTransport(*this, OpalTransportAddress::NoBinding);
    if (transport == NULL) {
      PTRACE(1, "SIP\tCould not create transport from " << address);
      return NULL;
    }
  }
  else {
    // endpoint has a specific listener - use only that interface
    OpalTransportAddress LocalAddress(ip, port, "udp$");
    transport = LocalAddress.CreateTransport(*this) ;
    if (transport == NULL) {
      PTRACE(1, "SIP\tCould not create transport for " << LocalAddress);
      return NULL;
    }
  }
  PTRACE(4, "SIP\tCreated transport " << *transport);

  transport->SetBufferSize(SIP_PDU::MaxSize);
  if (!transport->ConnectTo(address)) {
    PTRACE(1, "SIP\tCould not connect to " << address << " - " << transport->GetErrorText());
    delete transport;
    return NULL;
  }

  transport->SetPromiscuous(OpalTransport::AcceptFromAny);

  if (!transport->IsReliable())
    transport->AttachThread(PThread::Create(PCREATE_NOTIFIER(TransportThreadMain),
                                            (INT)transport,
                                            PThread::NoAutoDeleteThread,
                                            PThread::NormalPriority,
                                            "SIP Transport:%x"));

  return transport;
}


void SIPEndPoint::HandlePDU(OpalTransport & transport)
{
  // create a SIP_PDU structure, then get it to read and process PDU
  SIP_PDU * pdu = new SIP_PDU;

  PTRACE(4, "SIP\tWaiting for PDU on " << transport);
  if (pdu->Read(transport)) {
    if (OnReceivedPDU(transport, pdu))
      return;
  }
  else if (transport.good()) {
    PTRACE(1, "SIP\tMalformed request received on " << transport);
    SIP_PDU response(*pdu, SIP_PDU::Failure_BadRequest);
    response.Write(transport);
  }

  delete pdu;
}


BOOL SIPEndPoint::MakeConnection(OpalCall & call,
                                 const PString & remoteParty,
                                 void * userData)
{
  if (remoteParty.Find("sip:") != 0)
    return FALSE;

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = CreateConnection(call, callID, userData, remoteParty, NULL, NULL);
  if (connection == NULL)
    return FALSE;

  connectionsActive.SetAt(connection->GetToken(), connection);

  // If we are the A-party then need to initiate a call now in this thread. If
  // we are the B-Party then SetUpConnection() gets called in the context of
  // the A-party thread.
  if (call.GetConnection(0) == connection)
    connection->SetUpConnection();

  return TRUE;
}


OpalMediaFormatList SIPEndPoint::GetMediaFormats() const
{
  return OpalMediaFormat::GetAllRegisteredMediaFormats();
}


BOOL SIPEndPoint::IsAcceptedAddress(const SIPURL & /*toAddr*/)
{
  return TRUE;
}


SIPConnection * SIPEndPoint::CreateConnection(OpalCall & call,
                                              const PString & token,
                                              void * /*userData*/,
                                              const SIPURL & destination,
                                              OpalTransport * transport,
                                              SIP_PDU * /*invite*/)
{
  return new SIPConnection(call, *this, token, destination, transport);
}


BOOL SIPEndPoint::SetupTransfer(const PString & token,  
				const PString & /*callIdentity*/, 
				const PString & remoteParty,  
				void * userData)
{
  // Make a new connection
  PSafePtr<OpalConnection> otherConnection = 
    GetConnectionWithLock(token, PSafeReference);
  if (otherConnection == NULL) {
    return FALSE;
  }
  
  OpalCall call = otherConnection->GetCall();

  // Move old connection on token to new value and flag for removal
  PString adjustedToken;
  unsigned tieBreaker = 0;
  do {
    adjustedToken = token + "-replaced";
    adjustedToken.sprintf("-%u", ++tieBreaker);
  } while (connectionsActive.Contains(adjustedToken));
  connectionsActive.SetAt(adjustedToken, otherConnection);
  call.OnReleased(*otherConnection);
  
  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = 
    CreateConnection(call, callID, userData, remoteParty, NULL, NULL);
  
  if (connection == NULL)
    return FALSE;

  connectionsActive.SetAt(callID, connection);
  connection->SetUpConnection();

  return TRUE;
}


BOOL SIPEndPoint::OnReceivedPDU(OpalTransport & transport, SIP_PDU * pdu)
{
  PSafePtr<SIPConnection> connection = GetSIPConnectionWithLock(pdu->GetMIME().GetCallID());
  if (connection != NULL) {
    SIPTransaction * transaction = connection->GetTransaction(pdu->GetTransactionID());
    if (transaction != NULL && transaction->GetMethod() == SIP_PDU::Method_INVITE) {
      // Have a response to the INVITE, so end Connect mode on the transport
      transport.EndConnect(transaction->GetLocalAddress());
    }
    connection->QueuePDU(pdu);
    return TRUE;
  }

  // PDUs outside of connection context
  if (!transport.IsReliable()) {

    // Get response address from new request
    if (pdu->GetMethod() != SIP_PDU::NumMethods) {
      PStringList viaList = pdu->GetMIME().GetViaList();
      PString via = viaList[0];
      PINDEX j = 0;
      if ((j = via.FindLast (' ')) != P_MAX_INDEX)
	      via = via.Mid(j+1);
      if ((j = via.Find (';')) != P_MAX_INDEX)
	      via = via.Left(j);
      // sets return port to 5060 if none supplied and selects UDP transport
      OpalTransportAddress viaAddress(via, GetDefaultSignalPort(), "udp$");
      transport.SetRemoteAddress(viaAddress);
      PTRACE(4, "SIP\tTranport remote address change from Via: " << transport);
    }
  }

  switch (pdu->GetMethod()) {
    case SIP_PDU::NumMethods :
      {
        SIPTransaction * transaction = transactions.GetAt(pdu->GetTransactionID());
        if (transaction != NULL)
          transaction->OnReceivedResponse(*pdu);
      }
      break;

    case SIP_PDU::Method_INVITE :
      return OnReceivedINVITE(transport, pdu);

    case SIP_PDU::Method_REGISTER :
    case SIP_PDU::Method_SUBSCRIBE :
      {
        SIP_PDU response(*pdu, SIP_PDU::Failure_MethodNotAllowed);
        response.GetMIME().SetAt("Allow", "INVITE");
        response.Write(transport);
      }
      break;

    case SIP_PDU::Method_NOTIFY :
       return OnReceivedNOTIFY(transport, *pdu);
       break;

    case SIP_PDU::Method_MESSAGE :
      {
	OnReceivedMESSAGE(transport, *pdu);
        SIP_PDU response(*pdu, SIP_PDU::Successful_OK);
        response.Write(transport);
      }
   
    case SIP_PDU::Method_OPTIONS :
     {
       SIP_PDU response(*pdu, SIP_PDU::Successful_OK);
       response.Write(transport);
     }
    case SIP_PDU::Method_ACK :
      {
	// If we receive an ACK outside of the context of a connection,
	// ignore it.
      }
      break;

    default :
      {
        SIP_PDU response(*pdu, SIP_PDU::Failure_TransactionDoesNotExist);
        response.Write(transport);
      }
  }

  return FALSE;
}


void SIPEndPoint::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  if (transaction.GetMethod() == SIP_PDU::Method_MESSAGE) {

    // Failure, in the 4XX class, handle some cases to give feedback.
    if (response.GetStatusCode()/100 == 4)
      OnMessageFailed(SIPURL(transaction.GetMIME().GetTo()),
		      response.GetStatusCode());

    if (response.GetStatusCode()/100 != 1)
      transaction.GetTransport().Close();
  }
  else if (transaction.GetMethod() == SIP_PDU::Method_REGISTER
	   || transaction.GetMethod() == SIP_PDU::Method_SUBSCRIBE) {
    
    PString callID = transaction.GetMIME().GetCallID ();

    // Have a response to the REGISTER or to the SUBSCRIBE, 
    // so CANCEL all the other REGISTER or SUBSCRIBE requests
    // sent to that host.
    PSafePtr<SIPInfo> info = activeRegistrations.FindSIPInfoByCallID (callID, PSafeReadOnly);
    if (info == NULL) 
      return;

    info->Cancel (transaction);

    // Have a response to the INVITE, so end Connect mode on the transport
    transaction.GetTransport().EndConnect(transaction.GetLocalAddress());

    // Failure, in the 4XX class, handle some cases to give feedback.
    if (response.GetStatusCode()/100 == 4
     && response.GetStatusCode() != SIP_PDU::Failure_UnAuthorised
     && response.GetStatusCode() != SIP_PDU::Failure_ProxyAuthenticationRequired) {
      // Trigger the callback 
      info->OnFailed (response.GetStatusCode());
    }
  }
  
  switch (response.GetStatusCode()) {
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      OnReceivedAuthenticationRequired(transaction, response);
      break;

    default :
      switch (response.GetStatusCode()/100) {
        case 1 :
          // Do nothing on 1xx
          break;
        case 2 :
          OnReceivedOK(transaction, response);
          break;
        default :
          ;
      }
  }
}


BOOL SIPEndPoint::OnReceivedINVITE(OpalTransport & transport, SIP_PDU * request)
{
  SIPMIMEInfo & mime = request->GetMIME();

  // parse the incoming To field, and check if we accept incoming calls for this address
  SIPURL toAddr(mime.GetTo());
  if (!IsAcceptedAddress(toAddr)) {
    PTRACE(1, "SIP\tIncoming INVITE from " << request->GetURI() << " for unknown address " << toAddr);
    SIP_PDU response(*request, SIP_PDU::Failure_NotFound);
    response.Write(transport);
    return FALSE;
  }

  // ask the endpoint for a connection
  SIPConnection *connection = 
    CreateConnection(*GetManager().CreateCall(), mime.GetCallID(),
		     NULL, request->GetURI(), &transport, request);
  if (connection == NULL) {
    PTRACE(2, "SIP\tFailed to create SIPConnection for INVITE from " << request->GetURI() << " for " << toAddr);
    SIP_PDU response(*request, SIP_PDU::Failure_NotFound);
    response.Write(transport);
    return FALSE;
  }

  // add the connection to the endpoint list
  connectionsActive.SetAt(connection->GetToken(), connection);
  
  // Get the connection to handle the rest of the INVITE
  connection->QueuePDU(request);
  return TRUE;
}


void SIPEndPoint::OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafePtr<SIPInfo> realm_info = NULL;
  PSafePtr<SIPInfo> callid_info = NULL;
  SIPTransaction * request = NULL;
  SIPAuthentication auth;
  
  BOOL isProxy = 
    response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;
  PString lastRealm;
  PString lastNonce;
  PString lastUsername;
  
#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  PTRACE(2, "SIP\tReceived " << proxyTrace << "Authentication Required response");
  
  // Only support REGISTER and SUBSCRIBE for now
  if (transaction.GetMethod() != SIP_PDU::Method_REGISTER
      && transaction.GetMethod() != SIP_PDU::Method_SUBSCRIBE) {
    PTRACE(1, "SIP\tCannot do " << proxyTrace << "Authentication Required for non REGISTER and non SUBSCRIBE");
    return;
  }
  
  // Received authentication required response, try to find authentication
  // for the given realm
  if (!auth.Parse(response.GetMIME()(isProxy 
				     ? "Proxy-Authenticate"
				     : "WWW-Authenticate"),
		  isProxy)) {
    return;
  }

  // Try to find authentication information for the requested realm
  // That realm is specified when registering
  realm_info = activeRegistrations.FindSIPInfoByAuthRealm(auth.GetAuthRealm(), PSafeReadWrite);

  // Try to find authentication information for the given call ID
  callid_info = activeRegistrations.FindSIPInfoByCallID(response.GetMIME().GetCallID(), PSafeReadWrite);
  
  // No authentication information found for the realm, 
  // use what we have for the given CallID
  if (realm_info == NULL)
    realm_info = callid_info;
  
  // No realm info, no call ID info, weird
  if (realm_info == NULL || callid_info == NULL) {
    PTRACE(2, "SIP\tNo Authentication info found for that realm, authentication impossible");
    return;
  }
  
  if (realm_info->GetAuthentication().IsValid()) {
    lastRealm = realm_info->GetAuthentication().GetAuthRealm();
    lastUsername = realm_info->GetAuthentication().GetUsername();
    lastNonce = realm_info->GetAuthentication().GetNonce();
  }

  if (!realm_info->GetAuthentication().Parse(response.GetMIME()(isProxy 
								? "Proxy-Authenticate"
								: "WWW-Authenticate"),
					     isProxy)) {
    return;
  }
  
  // Already sent info for that callID. Check if params are different
  // from the ones found for the given realm
  if (callid_info->GetAuthentication().IsValid()
      && lastUsername == callid_info->GetAuthentication().GetUsername ()
      && lastNonce == callid_info->GetAuthentication().GetNonce ()) {
    PTRACE(1, "SIP\tAlready done REGISTER/SUBSCRIBE for " << proxyTrace << "Authentication Required");
    callid_info->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  // Restart the transaction with new authentication info
  request = callid_info->CreateTransaction(transaction.GetTransport(), 
					   (callid_info->GetExpire () == 0)); 
  if (!realm_info->GetAuthentication().Authorise(*request)) {
    // don't REGISTER again if no authentication info available
    delete request;
	callid_info->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }
  if (request->Start()) 
    callid_info->AppendTransaction(request);
  else {
    delete request;
    PTRACE(1, "SIP\tCould not restart REGISTER/SUBSCRIBE for Authentication Required");
  }
}


void SIPEndPoint::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafePtr<SIPInfo> info = NULL;

  if (transaction.GetMethod() != SIP_PDU::Method_REGISTER
      && transaction.GetMethod() != SIP_PDU::Method_SUBSCRIBE) {
    PTRACE(3, "SIP\tReceived OK response for non REGISTER and non SUBSCRIBE");
    return;
  }
  
  info = activeRegistrations.FindSIPInfoByCallID(response.GetMIME().GetCallID(), PSafeReadWrite);
 
  if (info == NULL) 
    return;
  
  // We were registering, update the expire field
  if (info->GetExpire() > 0) {

    SIPURL contact = response.GetMIME().GetContact();
    int sec = contact.GetParamVars()("expires", "3600").AsUnsigned()*9/10;  
    info->SetExpire(sec);
  }
  else
    activeRegistrations.Remove(info);

  // Callback
  info->OnSuccess();
}


BOOL SIPEndPoint::OnReceivedNOTIFY (OpalTransport & transport, SIP_PDU & pdu)
{
  PSafePtr<SIPInfo> info = NULL;
  PCaselessString state, event;
  
  PTRACE(3, "SIP\tReceived NOTIFY");
  
  event = pdu.GetMIME().GetEvent();
  
  // Only support MWI Subscribe for now, other events are unrequested
  if (event.Find("message-summary") == P_MAX_INDEX) {

    SIP_PDU response(pdu, SIP_PDU::Failure_BadEvent);
    response.Write(transport);

    return FALSE;
  }
  
  SIPURL url (pdu.GetMIME().GetContact ());
    
  // A NOTIFY will have the same CallID than the SUBSCRIBE request
  // it corresponds to
  info = activeRegistrations.FindSIPInfoByCallID(pdu.GetMIME().GetCallID(), PSafeReadWrite);
  if (info == NULL) {

    // We should reject the NOTIFY here, but some proxies still use
    // the old draft for NOTIFY and send unsollicited NOTIFY.
    PTRACE(3, "SIP\tCould not find a SUBSCRIBE corresponding to the NOTIFY");
  }
  else {

    PTRACE(3, "SIP\tFound a SUBSCRIBE corresponding to the NOTIFY");
    // We received a NOTIFY corresponding to an active SUBSCRIBE
    // for which we have just unSUBSCRIBEd. That is the final NOTIFY.
    // We can remove the SUBSCRIBE from the list.
    if (!info->IsRegistered() && info->GetExpire () == 0) {
     
      PTRACE(3, "SIP\tFinal NOTIFY received");
      activeRegistrations.Remove (info);
    }

    state = pdu.GetMIME().GetSubscriptionState();

    // Check the susbscription state
    if (state.Find("terminated") != P_MAX_INDEX) {

      PTRACE(3, "SIP\tSubscription is terminated");
      activeRegistrations.Remove (info);
    }
    else if (state.Find("active") != P_MAX_INDEX
	     || state.Find("pending") != P_MAX_INDEX) {

      PTRACE(3, "SIP\tSubscription is " << pdu.GetMIME().GetSubscriptionState());
      if (pdu.GetMIME().GetExpires(0) != 0) {
	int sec = pdu.GetMIME().GetExpires(0)*9/10;  
	info->SetExpire(sec);
      }
    }
  }

  // We should compare the to and from tags and the call ID here
  // but many proxies are still using the old way of "unsollicited"
  // notifies for MWI, so we simply check if we requested the NOTIFY
  // or not through a SUBSCRIBE and accept it in all cases.
  SIP_PDU response(pdu, SIP_PDU::Successful_OK);
  response.Write(transport);

  PString msgs;
  PString body = pdu.GetEntityBody();
  // Extract the string describing the number of new messages
  if (!body.IsEmpty ()) {

    const char * validMessageClasses [] = 
      {
	"voice-message", 
	"fax-message", 
	"pager-message", 
	"multimedia-message", 
	"text-message", 
	"none", 
	NULL
      };
    PStringArray bodylines = body.Lines ();
    SIPURL url_from (pdu.GetMIME().GetFrom());
    SIPURL url_to (pdu.GetMIME().GetTo());
    for (int z = 0 ; validMessageClasses [z] != NULL ; z++) {
      
      for (int i = 0 ; i <= bodylines.GetSize () ; i++) {

	PCaselessString line = bodylines [i];
	PINDEX j = line.FindLast(validMessageClasses [z]);
	if (j != P_MAX_INDEX) {
	  line.Replace (validMessageClasses[z], "");
	  line.Replace (":", "");
	  msgs = line.Trim ();
	  OnMWIReceived (url_from.GetHostName(),
			 url_to.GetUserName(), 
			 (SIPMWISubscribe::MWIType) z, 
			 msgs);
	  return TRUE;
	}
      }
    }
  } 

  return TRUE;
}


void SIPEndPoint::OnReceivedMESSAGE(OpalTransport & /*transport*/, 
				    SIP_PDU & pdu)
{
  OnMessageReceived(pdu.GetMIME().GetFrom(), pdu.GetEntityBody());
}


void SIPEndPoint::OnRegistrationFailed(const PString & /*host*/, 
				       const PString & /*userName*/,
				       SIP_PDU::StatusCodes /*reason*/, 
				       BOOL /*wasRegistering*/)
{
}
    

void SIPEndPoint::OnRegistered(const PString & /*host*/, 
			       const PString & /*userName*/,
			       BOOL /*wasRegistering*/)
{
}


BOOL SIPEndPoint::IsRegistered(const PString & host) 
{
  PSafePtr<SIPInfo> info = activeRegistrations.FindSIPInfoByDomain(host, SIP_PDU::Method_REGISTER, PSafeReadOnly);

  if (info == NULL)
    return FALSE;
  
  return info->IsRegistered ();
}


BOOL SIPEndPoint::IsSubscribed(const PString & host, const PString & user) 
{
  // Adjusted user name
  PString adjustedUsername = user;
  if (adjustedUsername.IsEmpty())
    adjustedUsername = GetDefaultLocalPartyName();

  if (adjustedUsername.Find('@') == P_MAX_INDEX)
    adjustedUsername += '@' + host;

  PSafePtr<SIPInfo> info = activeRegistrations.FindSIPInfoByUrl(adjustedUsername, SIP_PDU::Method_SUBSCRIBE, PSafeReadOnly);

  if (info == NULL)
    return FALSE;

  return info->IsRegistered ();
}


void SIPEndPoint::RegistrationRefresh(PTimer &, INT)
{
  SIPTransaction *request = NULL;

  // Timer has elapsed
  for (PINDEX i = 0 ; i < activeRegistrations.GetSize () ; i++) {

    PSafePtr<SIPInfo> info = activeRegistrations.GetAt (i);

    if (info->GetExpire() == -1) 
      activeRegistrations.Remove(info); // Was invalid the last time, delete it
    else {

      if (info->GetExpire() > 0 && !info->IsRegistered ())
	info->SetExpire(-1); // Mark it as invalid, REGISTER/SUBSCRIBE not successful

      // Need to refresh
      if (info->GetExpire() > 0 
	  && info->GetTransport() != NULL 
	  && info->HasExpired()) {
	PTRACE(2, "SIP\tStarting REGISTER/SUBSCRIBE for binding refresh");
	info->SetRegistered(FALSE);
	request = info->CreateTransaction(*info->GetTransport(), FALSE); 

	if (request->Start()) 
	  info->AppendTransaction(request);
	else {
	  delete request;
	  PTRACE(1, "SIP\tCould not start REGISTER/SUBSCRIBE for binding refresh");
	}
      }
    }
  }
}


BOOL SIPEndPoint::WriteMESSAGE(OpalTransport & transport, void * _message)
{
  if (_message == NULL)
    return FALSE;

  SIPMessageInfo * minfo = (SIPMessageInfo *) _message;
  SIPTransaction * message = NULL;
  
  message = new SIPMessage(minfo->ep, transport, minfo->url, minfo->body);

  if (!message->Start()) {
    delete message;
    message = NULL;
    PTRACE(2, "SIP\tDid not start MESSAGE transaction on " << transport);
    return FALSE;
  }
  minfo->ep.messages.Append(message);

  return TRUE;
}


BOOL SIPEndPoint::WriteSIPInfo(OpalTransport & transport, void * _info)
{
  if (_info == NULL)
    return FALSE;

  SIPInfo * info = (SIPInfo *)_info;
  SIPTransaction * request = NULL;
  request = info->CreateTransaction(transport, FALSE);

  if (!request->Start()) {
    delete request;
    request = NULL;
    PTRACE(2, "SIP\tDid not start REGISTER/SUBSCRIBE transaction on " << transport);
    return FALSE;
  }

  info->AppendTransaction(request);

  return TRUE;
}


BOOL SIPEndPoint::Register(const PString & host,
                           const PString & username,
                           const PString & password,
			                     const PString & realm)
{
  return TransmitSIPRegistrationInfo(host, username, password, realm, SIP_PDU::Method_REGISTER);
}


void SIPEndPoint::OnMessageReceived (const SIPURL & /*from*/,
				     const PString & /*body*/)
{
}


void SIPEndPoint::OnMWIReceived (const PString & /*remoteAddress*/,
				 const PString & /*user*/,
				 SIPMWISubscribe::MWIType /*type*/,
				 const PString & /*msgs*/)
{
}


BOOL SIPEndPoint::MWISubscribe(const PString & host, const PString & username)
{
  return TransmitSIPRegistrationInfo (host, username, "", "", SIP_PDU::Method_SUBSCRIBE);
}


BOOL SIPEndPoint::TransmitSIPRegistrationInfo(const PString & host,
					      const PString & username,
					      const PString & password,
					      const PString & realm,
					      SIP_PDU::Methods m)
{
  PSafePtr<SIPInfo> info = NULL;
  OpalTransport *transport = NULL;
  
  if (listeners.IsEmpty() || host.IsEmpty())
    return FALSE;
  
  // Adjusted user name
  PString adjustedUsername = username;
  if (adjustedUsername.IsEmpty())
    adjustedUsername = GetDefaultLocalPartyName();

  if (adjustedUsername.Find('@') == P_MAX_INDEX)
    adjustedUsername += '@' + host;

  // If we have a proxy, use it
  PString hostname;
  WORD port;

  if (proxy.IsEmpty()) {
    // Should do DNS SRV record lookup to get registrar address
    hostname = host;
    port = defaultSignalPort;
  }
  else {
    hostname = proxy.GetHostName();
    port = proxy.GetPort();
    if (port == 0)
      port = defaultSignalPort;
  }

  OpalTransportAddress transportAddress(hostname, port, "udp");

  // Create the SIPInfo structure
  info = activeRegistrations.FindSIPInfoByUrl(adjustedUsername, m, PSafeReadWrite);
  
  // if there is already a request with this URL and method, then update it with the new information
  if (info != NULL) {
    if (!password.IsEmpty())
      info->SetPassword(password); // Adjust the password if required 
    if (!realm.IsEmpty())
      info->SetAuthRealm(realm);   // Adjust the realm if required 
  } 

  // otherwise create a new request with this method type
  else {
    if (m == SIP_PDU::Method_REGISTER)
      info = new SIPRegisterInfo(*this, adjustedUsername, password/*, realm*/);
    else if (m == SIP_PDU::Method_SUBSCRIBE)
      info = new SIPMWISubscribeInfo(*this, adjustedUsername);
    if (info == NULL) {
      PTRACE(1, "SIP\tUnknown SIP request method " << m);
      return FALSE;
    }
    activeRegistrations.Append(info);
  }

  if (!info->CreateTransport(transportAddress)) {
    activeRegistrations.Remove (info);
    return FALSE;
  }
  
  transport = info->GetTransport ();
    
  if (transport != NULL && !transport->WriteConnect(WriteSIPInfo, &*info)) {
    PTRACE(1, "SIP\tCould not write to " << transportAddress << " - " << transport->GetErrorText());
    activeRegistrations.Remove (info);
    return FALSE;
  }

  return TRUE;
}


BOOL SIPEndPoint::Unregister(const PString & host,
			     const PString & user)
{
  return TransmitSIPUnregistrationInfo (host, 
					user, 
					SIP_PDU::Method_REGISTER); 
}


BOOL SIPEndPoint::MWIUnsubscribe(const PString & host,
				 const PString & user)
{
  return TransmitSIPUnregistrationInfo (host, 
					user, 
					SIP_PDU::Method_SUBSCRIBE); 
}


void SIPEndPoint::OnMessageFailed(const SIPURL & /* messageUrl */,
				  SIP_PDU::StatusCodes /* reason */)
{
}


BOOL SIPEndPoint::TransmitSIPUnregistrationInfo(const PString & host, const PString & username, SIP_PDU::Methods method)
{
  SIPTransaction *request = NULL;

  // Adjusted user name
  PString adjustedUsername = username;
  if (adjustedUsername.IsEmpty())
    adjustedUsername = GetDefaultLocalPartyName();

  if (adjustedUsername.Find('@') == P_MAX_INDEX)
    adjustedUsername += '@' + host;
  
    {
      PSafePtr<SIPInfo> info = NULL;
      info = activeRegistrations.FindSIPInfoByUrl (adjustedUsername, method, PSafeReadWrite);
      if (info == NULL) {
        PTRACE(1, "SIP\tCould not find active registration/subscription for " << adjustedUsername);
        return FALSE;
      }
      
      if (!info->IsRegistered() || info->GetTransport() == NULL) {
        PTRACE(1, "SIP\tRemoving local registration/subscription info for apparently unregistered/subscribed " << adjustedUsername);
        activeRegistrations.Remove(info);
        return FALSE;
      }

      request = info->CreateTransaction(*info->GetTransport(), TRUE);

      if (!request->Start()) {
        PTRACE(1, "SIP\tCould not start UNREGISTER/UNSUBSCRIBE transaction");
        delete (request);
        request = NULL;
        return FALSE;
      }
      info->AppendTransaction(request);
    }

  // Do this synchronously
  request->Wait ();

  return TRUE;
}


void SIPEndPoint::SetProxy(const PString & hostname,
                           const PString & username,
                           const PString & password)
{
  PStringStream str;
  if (!hostname) {
    str << "sip:";
    if (!username) {
      str << username;
      if (!password)
        str << ':' << password;
      str << '@';
    }
    str << hostname;
  }
  proxy = str;
}

void SIPEndPoint::SetProxy(const SIPURL & url) 
{ 
  proxy = url; 
}

PString SIPEndPoint::GetUserAgent() const 
{ 
  return userAgentString;
}

BOOL SIPEndPoint::GetAuthentication(const PString & realm, SIPAuthentication &auth) 
{
  PSafePtr<SIPInfo> info = activeRegistrations.FindSIPInfoByAuthRealm(realm, PSafeReadOnly);
  if (info == NULL)
    return FALSE;

  auth = info->GetAuthentication();

  return TRUE;
}


const SIPURL SIPEndPoint::GetRegisteredPartyName(const PString & host)
{
  PString partyName;
  PString contactDomain;
  PString realm;
  
  PSafePtr<SIPInfo> info = activeRegistrations.FindSIPInfoByDomain(host, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (info == NULL)
    return SIPURL();

  return info->GetRegistrationAddress();
}


BOOL SIPEndPoint::SendMessage (const SIPURL & url, 
			       const PString & body)
{
  SIPMessageInfo *minfo = NULL;
  OpalTransport *transport = NULL;

  minfo = new SIPMessageInfo(*this, url, body);

  // If we have a proxy, use it
  PString hostname;
  WORD port;

  if (proxy.IsEmpty()) {
    // Should do DNS SRV record lookup to get registrar address
    hostname = url.GetHostName();
    port = url.GetPort();
  }
  else {
    hostname = proxy.GetHostName();
    port = proxy.GetPort();
    if (port == 0)
      port = defaultSignalPort;
  }

  OpalTransportAddress transportAddress(hostname, port, "udp");
  transport = CreateTransport(transportAddress);
  if (transport != NULL && !transport->WriteConnect(WriteMESSAGE, &*minfo)) {
    PTRACE(1, "SIP\tCould not write to " << transportAddress << " - " << transport->GetErrorText());
    delete minfo;
    return FALSE;
  }
  delete minfo;

  return TRUE;
}


void SIPEndPoint::OnRTPStatistics(const SIPConnection & /*connection*/,
                                  const RTP_Session & /*session*/) const
{
}


SIPEndPoint::
SIPMessageInfo::SIPMessageInfo(SIPEndPoint &endpoint, 
			     const SIPURL & u, 
			     const PString & b)
  :ep(endpoint),url(u),body(b)
{
}


// End of file ////////////////////////////////////////////////////////////////
