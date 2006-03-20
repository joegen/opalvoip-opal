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
  The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: sipep.cxx,v $
 * Revision 1.2108.2.1  2006/03/20 02:25:27  csoutheren
 * Backports from CVS head
 *
 * Revision 2.118  2006/03/19 18:57:06  dsandras
 * More work on Ekiga report #334999.
 *
 * Revision 2.117  2006/03/19 18:14:11  dsandras
 * Removed unused variable.
 *
 * Revision 2.116  2006/03/19 13:17:15  dsandras
 * Ignore failures when unregistering and leaving. Fixes Ekiga #334997.
 *
 * Revision 2.115  2006/03/19 12:32:05  dsandras
 * RFC3261 says that "CANCEL messages "SHOULD NOT" be sent for anything but INVITE
 * requests". Fixes Ekiga report #334985.
 *
 * Revision 2.114  2006/03/19 11:45:48  dsandras
 * The remote address of the registrar transport might have changed due
 * to the Via field. This affected unregistering which was reusing
 * the exact same transport to unregister. Fixed Ekiga report #334999.
 *
 * Revision 2.113  2006/03/08 20:10:12  dsandras
 * Fixed logic error when receiving an incomplete MWI NOTIFY PDU.
 *
 * Revision 2.112  2006/03/08 18:55:04  dsandras
 * Added missing return TRUE.
 *
 * Revision 2.111  2006/03/08 18:34:41  dsandras
 * Added DNS SRV lookup.
 *
 * Revision 2.110  2006/03/06 22:52:59  csoutheren
 * Reverted experimental SRV patch due to unintended side effects
 *
 * Revision 2.109  2006/03/06 19:01:47  dsandras
 * Allow registering several accounts with the same realm but different
 * user names to the same provider. Fixed possible crash due to transport
 * deletion before the transaction is over.
 *
 * Revision 2.108  2006/03/06 12:56:03  csoutheren
 * Added experimental support for SIP SRV lookups
 *
 * Revision 2.107  2006/02/28 19:18:24  dsandras
 * Reset the expire time to its initial value accepted by the server when
 * refreshing the registration. Have a default value of 1 for voicemail
 * notifications.
 *
 * Revision 2.106  2006/02/14 19:04:07  dsandras
 * Fixed problem when the voice-message field is not mentionned in the NOTIFY.
 *
 * Revision 2.105  2006/02/08 04:51:19  csoutheren
 * Adde trace message when sending 502
 *
 * Revision 2.104  2006/02/05 22:19:18  dsandras
 * Only refresh registered accounts. In case of timeout of a previously
 * registered account, retry regularly.
 *
 * Revision 2.103  2006/01/31 03:27:19  csoutheren
 * Removed unused variable
 * Fixed typo in comparison
 *
 * Revision 2.102  2006/01/29 21:03:32  dsandras
 * Removed cout.
 *
 * Revision 2.101  2006/01/29 20:55:33  dsandras
 * Allow using a simple username or a fill url when registering.
 *
 * Revision 2.100  2006/01/29 17:09:48  dsandras
 * Added guards against timeout of 0 when registering or subscribing.
 * Make sure activeSIPInfo is empty and destroyed before exiting.
 *
 * Revision 2.99  2006/01/24 22:24:48  dsandras
 * Fixed possible bug with wait called on non existing SIPInfo.
 *
 * Revision 2.98  2006/01/24 19:49:29  dsandras
 * Simplified code, always recreate the transport.
 *
 * Revision 2.97  2006/01/14 10:43:06  dsandras
 * Applied patch from Brian Lu <Brian.Lu _AT_____ sun.com> to allow compilation
 * with OpenSolaris compiler. Many thanks !!!
 *
 * Revision 2.96  2006/01/09 12:15:42  csoutheren
 * Fixed warning under VS.net 2003
 *
 * Revision 2.95  2006/01/08 21:53:41  dsandras
 * Changed IsRegistered so that it takes the registration url as argument,
 * allowing it to work when there are several accounts on the same server.
 *
 * Revision 2.94  2006/01/08 14:43:47  dsandras
 * Improved the NAT binding refresh methods so that it works with all endpoint
 * created transports that require it and so that it can work by sending
 * SIP Options, or empty SIP requests. More methods can be added later.
 *
 * Revision 2.93  2006/01/07 18:30:20  dsandras
 * Further simplification.
 *
 * Revision 2.92  2006/01/07 18:29:44  dsandras
 * Fixed exit problems in case of unregistration failure.
 *
 * Revision 2.91  2006/01/02 11:28:07  dsandras
 * Some documentation. Various code cleanups to prevent duplicate code.
 *
 * Revision 2.90  2005/12/20 20:40:47  dsandras
 * Added missing parameter.
 *
 * Revision 2.89  2005/12/18 21:06:55  dsandras
 * Added function to clean up the registrations object. Moved DeleteObjectsToBeRemoved call outside of the loop.
 *
 * Revision 2.88  2005/12/18 19:18:18  dsandras
 * Regularly clean activeSIPInfo.
 * Fixed problem with Register ignoring the timeout parameter.
 *
 * Revision 2.87  2005/12/17 21:14:12  dsandras
 * Prevent loop when exiting if unregistration fails forever.
 *
 * Revision 2.86  2005/12/14 17:59:50  dsandras
 * Added ForwardConnection executed when the remote asks for a call forwarding.
 * Similar to what is done in the H.323 part with the method of the same name.
 *
 * Revision 2.85  2005/12/11 19:14:20  dsandras
 * Added support for setting a different user name and authentication user name
 * as required by some providers like digisip.
 *
 * Revision 2.84  2005/12/11 12:23:37  dsandras
 * Removed unused variable.
 *
 * Revision 2.83  2005/12/08 21:14:54  dsandras
 * Added function allowing to change the nat binding refresh timeout.
 *
 * Revision 2.82  2005/12/07 12:19:17  dsandras
 * Improved previous commit.
 *
 * Revision 2.81  2005/12/07 11:50:46  dsandras
 * Signal the registration failed in OnAuthenticationRequired with it happens
 * for a REGISTER or a SUBSCRIBE.
 *
 * Revision 2.80  2005/12/05 22:20:57  dsandras
 * Update the transport when the computer is behind NAT, using STUN, the IP
 * address has changed compared to the original transport and a registration
 * refresh must occur.
 *
 * Revision 2.79  2005/12/04 22:08:58  dsandras
 * Added possibility to provide an expire time when registering, if not
 * the default expire time for the endpoint will be used.
 *
 * Revision 2.78  2005/11/30 13:44:20  csoutheren
 * Removed compile warnings on Windows
 *
 * Revision 2.77  2005/11/28 19:07:56  dsandras
 * Moved OnNATTimeout to SIPInfo and use it for active conversations too.
 * Added E.164 support.
 *
 * Revision 2.76  2005/11/07 21:44:49  dsandras
 * Fixed origin of MESSAGE requests. Ignore refreshes for MESSAGE requests.
 *
 * Revision 2.75  2005/11/04 19:12:30  dsandras
 * Fixed OnReceivedResponse for MESSAGE PDU's.
 *
 * Revision 2.74  2005/11/04 13:40:52  dsandras
 * Initialize localPort (thanks Craig for pointing this)
 *
 * Revision 2.73  2005/11/02 22:17:56  dsandras
 * Take the port into account in TransmitSIPInfo. Added missing break statements.
 *
 * Revision 2.72  2005/10/30 23:01:29  dsandras
 * Added possibility to have a body for SIPInfo. Moved MESSAGE sending to SIPInfo for more efficiency during conversations.
 *
 * Revision 2.71  2005/10/30 15:55:55  dsandras
 * Added more calls to OnMessageFailed().
 *
 * Revision 2.70  2005/10/22 17:14:44  dsandras
 * Send an OPTIONS request periodically when STUN is being used to maintain the registrations binding alive.
 *
 * Revision 2.69  2005/10/22 10:29:29  dsandras
 * Increased refresh interval.
 *
 * Revision 2.68  2005/10/20 20:26:22  dsandras
 * Made the transactions handling thread-safe.
 *
 * Revision 2.67  2005/10/11 20:53:35  dsandras
 * Allow the expire field to be outside of the SIPURL formed by the contact field.
 *
 * Revision 2.66  2005/10/09 20:42:59  dsandras
 * Creating the connection thread can take more than 200ms. Send a provisional
 * response before creating it.
 *
 * Revision 2.65  2005/10/05 20:54:54  dsandras
 * Simplified some code.
 *
 * Revision 2.64  2005/10/03 21:46:20  dsandras
 * Fixed previous commit (sorry).
 *
 * Revision 2.63  2005/10/03 21:18:55  dsandras
 * Prevent looping at exit by removing SUBSCRIBE's and failed REGISTER from
 * the activeSIPInfo.
 *
 * Revision 2.62  2005/10/02 21:48:40  dsandras
 * - Use the transport port when STUN is being used when returning the contact address. That allows SIP proxies to regularly ping the UA so that the binding stays alive. As the REGISTER transport stays open, it permits to receive incoming calls when being behind a type of NAT supported by STUN without the need to forward any port (even not the listening port).
 * - Call OnFailed for other registration failure causes than 4xx.
 *
 * Revision 2.61  2005/10/02 17:48:15  dsandras
 * Added function to return the translated contact address of the endpoint.
 * Fixed GetRegisteredPartyName so that it returns a default SIPURL if we
 * have no registrations for the given host.
 *
 * Revision 2.60  2005/10/01 13:19:57  dsandras
 * Implemented back Blind Transfer.
 *
 * Revision 2.59  2005/09/27 16:17:36  dsandras
 * Remove the media streams on transfer.
 *
 * Revision 2.58  2005/09/21 19:50:30  dsandras
 * Cleaned code. Make use of the new SIP_PDU function that returns the correct address where to send responses to incoming requests.
 *
 * Revision 2.57  2005/09/20 17:02:57  dsandras
 * Adjust via when receiving an incoming request.
 *
 * Revision 2.56  2005/08/04 17:13:53  dsandras
 * More implementation of blind transfer.
 *
 * Revision 2.55  2005/06/02 13:18:02  rjongbloed
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
#include <ptclib/enum.h>

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
  registrations.RemoveAll();
  PWaitAndSignal m(transportMutex);

  if (registrarTransport) { 
    delete registrarTransport;
    registrarTransport = NULL;
  }
}


BOOL SIPInfo::CreateTransport (OpalTransportAddress & transportAddress)
{
  PWaitAndSignal m(transportMutex);

  registrarAddress = transportAddress;
  
  // Only delete if we are refreshing
  if (registrarTransport != NULL && HasExpired()) {
    delete registrarTransport;
    registrarTransport = NULL;
  }

  if (registrarTransport == NULL) {
    registrarTransport = ep.CreateTransport(registrarAddress);
  }
  
  if (registrarTransport == NULL) {
    PTRACE(2, "SIP\tUnable to create transport for registrar");
    OnFailed(SIP_PDU::Failure_BadGateway);
    return FALSE;
  }
  
  PTRACE (1, "SIP\tCreated Transport for Registrar " << *registrarTransport);

  return TRUE;
}


SIPRegisterInfo::SIPRegisterInfo(SIPEndPoint & endpoint, const PString & name, const PString & authName, const PString & pass, int exp)
  :SIPInfo(endpoint, name)
{
  expire = exp;
  if (expire == 0)
    expire = ep.GetRegistrarTimeToLive().GetSeconds();
  password = pass;
  authUser = authName;
}


SIPRegisterInfo::~SIPRegisterInfo()
{
}

SIPTransaction * SIPRegisterInfo::CreateTransaction(OpalTransport &t, BOOL unregister)
{
  authentication.SetUsername(authUser);
  authentication.SetPassword(password);
  if (!authRealm.IsEmpty())
    authentication.SetAuthRealm(authRealm);   
  
  if (unregister)
    SetExpire (0);

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


SIPMWISubscribeInfo::SIPMWISubscribeInfo (SIPEndPoint & endpoint, const PString & name, int exp)
  :SIPInfo(endpoint, name)
{
  expire = exp;
  if (expire == 0)
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

SIPMessageInfo::SIPMessageInfo (SIPEndPoint & endpoint, const PString & name, const PString & b)
  :SIPInfo(endpoint, name)
{
  expire = 500; 
  body = b;
  SetRegistered(TRUE);
}


SIPTransaction * SIPMessageInfo::CreateTransaction(OpalTransport &t, BOOL /*unregister*/)
{ 
  SIPMessage * message = new SIPMessage(ep, t, registrationAddress, body);
  registrationID = message->GetMIME().GetCallID();
  return message;
}

void SIPMessageInfo::OnSuccess()
{ 
}

void SIPMessageInfo::OnFailed(SIP_PDU::StatusCodes reason)
{ 
  ep.OnMessageFailed(registrationAddress, reason);
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
    notifierTimeToLive(0, 0, 0, 1), // 1 hour
    natBindingTimeout(0, 0, 1) // 1 minute
{
  defaultSignalPort = 5060;
  mimeForm = FALSE;
  maxRetries = 10;
  lastSentCSeq = 0;
  userAgentString = "OPAL/2.0";

  transactions.DisallowDeleteObjects();
  activeSIPInfo.AllowDeleteObjects();

  registrationTimer.SetNotifier(PCREATE_NOTIFIER(RegistrationRefresh));
  registrationTimer.RunContinuous (PTimeInterval(0, 30));
  
  natBindingTimer.SetNotifier(PCREATE_NOTIFIER(NATBindingRefresh));
  natBindingTimer.RunContinuous (natBindingTimeout);

  natMethod = None;

  PTRACE(3, "SIP\tCreated endpoint.");
}


SIPEndPoint::~SIPEndPoint()
{
  listeners.RemoveAll();

  while (activeSIPInfo.GetSize()>0) {
    SIPURL url;
    SIPInfo *info = activeSIPInfo.GetAt(0);
    url = info->GetRegistrationAddress ();
    if (info->GetMethod() == SIP_PDU::Method_REGISTER && info->IsRegistered()) { 
      Unregister(url.GetHostName(), url.GetUserName());
      info->SetRegistered(FALSE);
    }
    else 
      activeSIPInfo.Remove(info);
   
    activeSIPInfo.DeleteObjectsToBeRemoved();
  }

  PWaitAndSignal m(transactionsMutex);
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


void SIPEndPoint::NATBindingRefresh(PTimer &, INT)
{
  PTRACE(5, "SIP\tNAT Binding refresh started.");

  if (natMethod == None)
    return;

  for (PSafePtr<SIPInfo> info(activeSIPInfo, PSafeReadWrite); info != NULL; ++info) {

    OpalTransport *transport = info->GetTransport();
    if (transport && transport->SetRemoteAddress(info->GetRegistrarAddress())) {
      BOOL stunTransport = FALSE;

      stunTransport = (!transport->IsReliable() && GetManager().GetSTUN(transport->GetRemoteAddress().GetHostName()));

      if (!stunTransport) 
	return;

      switch (natMethod) {

      case Options: 
	  {
	    SIPOptions options(*this, *transport, transport->GetRemoteAddress());
	    options.Write(*transport);
	    break;
	  }

      case EmptyRequest:
	  {
	    transport->Write("\r\n", 2);
	    break;
	  }

      default:
	break;
      }
    }
  }

  PTRACE(5, "SIP\tNAT Binding refresh finished.");
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
                                 const PString & _remoteParty,
                                 void * userData)
{
  PString remoteParty;
  
  if (_remoteParty.Find("sip:") != 0)
    return FALSE;
  
  ParsePartyName(_remoteParty, remoteParty);

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
  if (call.GetConnection(0) == (OpalConnection*)connection)
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
				const PString & _remoteParty,  
				void * userData)
{
  PString remoteParty;
  // Make a new connection
  PSafePtr<OpalConnection> otherConnection = 
    GetConnectionWithLock(token, PSafeReference);
  if (otherConnection == NULL) {
    return FALSE;
  }
  
  OpalCall & call = otherConnection->GetCall();
  
  call.RemoveMediaStreams();
  
  ParsePartyName(_remoteParty, remoteParty);

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = 
    CreateConnection(call, callID, userData, remoteParty, NULL, NULL);
  
  if (connection == NULL)
    return FALSE;

  connectionsActive.SetAt(callID, connection);
  call.OnReleased(*otherConnection);
  
  connection->SetUpConnection();
  otherConnection->Release(OpalConnection::EndedByCallForwarded);

  return TRUE;
}


BOOL SIPEndPoint::ForwardConnection(SIPConnection & connection,  
				    const PString & forwardParty)
{
  OpalCall & call = connection.GetCall();
  
  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();

  SIPConnection * conn = 
    CreateConnection(call, callID, NULL, forwardParty, NULL, NULL);
  
  if (conn == NULL)
    return FALSE;

  connectionsActive.SetAt(callID, conn);
  call.OnReleased(connection);
  
  conn->SetUpConnection();
  connection.Release(OpalConnection::EndedByCallForwarded);

  return TRUE;
}


BOOL SIPEndPoint::OnReceivedPDU(OpalTransport & transport, SIP_PDU * pdu)
{
  // Adjust the Via list 
  if (pdu && pdu->GetMethod() != SIP_PDU::NumMethods)
    pdu->AdjustVia(transport);

  // Find a corresponding connection
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
      transport.SetRemoteAddress(pdu->GetViaAddress(*this));
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
	PString username = SIPURL(response.GetMIME().GetTo()).GetUserName();
	response.GetMIME().SetContact(GetLocalURL(transport, username));
        response.Write(transport);
	break;
      }
   
    case SIP_PDU::Method_OPTIONS :
     {
       SIP_PDU response(*pdu, SIP_PDU::Successful_OK);
       response.Write(transport);
       break;
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
  PSafePtr<SIPInfo> info = NULL;
 
  if (transaction.GetMethod() == SIP_PDU::Method_REGISTER
      || transaction.GetMethod() == SIP_PDU::Method_SUBSCRIBE) {

    PString callID = transaction.GetMIME().GetCallID ();

    // Have a response to the REGISTER/SUBSCRIBE/MESSAGE, 
    info = activeSIPInfo.FindSIPInfoByCallID (callID, PSafeReadOnly);
    if (info == NULL) 
      return;

    // Have a response, so end Connect mode on the transport
    transaction.GetTransport().EndConnect(transaction.GetLocalAddress()); 
  }
  else if (transaction.GetMethod() == SIP_PDU::Method_MESSAGE) {
  
    info = activeSIPInfo.FindSIPInfoByUrl(SIPURL(transaction.GetMIME().GetTo()).AsString(), SIP_PDU::Method_MESSAGE, PSafeReadOnly);

    if (info == NULL) 
      return;
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
	  if (info != NULL) {
	    // Failure for a SUBSCRIBE/REGISTER/MESSAGE 
	    info->OnFailed (response.GetStatusCode());
	  }
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
  
  // send provisional response
  SIP_PDU response(*request, SIP_PDU::Information_Trying);
  response.Write(transport);

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
  PString lastNonce;
  PString lastUsername;
  
#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  PTRACE(2, "SIP\tReceived " << proxyTrace << "Authentication Required response");
  
  // Only support REGISTER and SUBSCRIBE for now
  if (transaction.GetMethod() != SIP_PDU::Method_REGISTER
      && transaction.GetMethod() != SIP_PDU::Method_SUBSCRIBE
      && transaction.GetMethod() != SIP_PDU::Method_MESSAGE) {
    PTRACE(1, "SIP\tCannot do " << proxyTrace << "Authentication Required for non REGISTER, SUBSCRIBE or MESSAGE");
    return;
  }
  
  // Try to find authentication information for the given call ID
  callid_info = activeSIPInfo.FindSIPInfoByCallID(response.GetMIME().GetCallID(), PSafeReadWrite);

  if (!callid_info)
    return;
  
  // Received authentication required response, try to find authentication
  // for the given realm
  if (!auth.Parse(response.GetMIME()(isProxy 
				     ? "Proxy-Authenticate"
				     : "WWW-Authenticate"),
		  isProxy)) {
    callid_info->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  // Try to find authentication information for the requested realm
  // That realm is specified when registering
  realm_info = activeSIPInfo.FindSIPInfoByAuthRealm(auth.GetAuthRealm(), auth.GetUsername().IsEmpty()?SIPURL(response.GetMIME().GetFrom()).GetUserName():auth.GetUsername(), PSafeReadWrite);

  // No authentication information found for the realm, 
  // use what we have for the given CallID
  if (realm_info == NULL)
    realm_info = callid_info;
  
  // No realm info, weird
  if (realm_info == NULL) {
    PTRACE(2, "SIP\tNo Authentication info found for that realm, authentication impossible");
    return;
  }
  
  if (realm_info->GetAuthentication().IsValid()) {
    lastUsername = realm_info->GetAuthentication().GetUsername();
    lastNonce = realm_info->GetAuthentication().GetNonce();
  }

  if (!realm_info->GetAuthentication().Parse(response.GetMIME()(isProxy 
								? "Proxy-Authenticate"
								: "WWW-Authenticate"),
					     isProxy)) {
    callid_info->OnFailed(SIP_PDU::Failure_UnAuthorised);
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
    // don't send again if no authentication info available
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
      && transaction.GetMethod() != SIP_PDU::Method_MESSAGE
      && transaction.GetMethod() != SIP_PDU::Method_SUBSCRIBE) {
    return;
  }
  
  info = activeSIPInfo.FindSIPInfoByCallID(response.GetMIME().GetCallID(), PSafeReadWrite);
 
  if (info == NULL) 
    return;
  
  // We were registering, update the expire field
  if (info->GetExpire() > 0) {

    PString contact = response.GetMIME().GetContact();
    int sec = SIPURL(contact).GetParamVars()("expires").AsUnsigned();  
    if (!sec && response.GetMIME().HasFieldParameter("expires", contact))
      sec = response.GetMIME().GetFieldParameter("expires", contact).AsUnsigned();
    if (!sec)
      sec = 3600;
    info->SetExpire(sec*9/10);
  }
  else 
    activeSIPInfo.Remove(info);

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
  info = activeSIPInfo.FindSIPInfoByCallID(pdu.GetMIME().GetCallID(), PSafeReadWrite);
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
      activeSIPInfo.Remove (info);
    }

    state = pdu.GetMIME().GetSubscriptionState();

    // Check the susbscription state
    if (state.Find("terminated") != P_MAX_INDEX) {

      PTRACE(3, "SIP\tSubscription is terminated");
      activeSIPInfo.Remove (info);
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
      
      for (int i = 0 ; i < bodylines.GetSize () ; i++) {

	PCaselessString line (bodylines [i]);
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

    // Received MWI, unknown messages number
    OnMWIReceived (url_from.GetHostName(),
		   url_to.GetUserName(), 
		   (SIPMWISubscribe::MWIType) 0, 
		   "1/0");
  } 

  return TRUE;
}


void SIPEndPoint::OnReceivedMESSAGE(OpalTransport & /*transport*/, 
				    SIP_PDU & pdu)
{
  PString from = pdu.GetMIME().GetFrom();
  PINDEX j = from.Find (';');
  if (j != P_MAX_INDEX)
    from = from.Left(j); // Remove all parameters
  OnMessageReceived(from, pdu.GetEntityBody());
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


BOOL SIPEndPoint::IsRegistered(const PString & url) 
{
  PSafePtr<SIPInfo> info = activeSIPInfo.FindSIPInfoByUrl(url, SIP_PDU::Method_REGISTER, PSafeReadOnly);

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

  PSafePtr<SIPInfo> info = activeSIPInfo.FindSIPInfoByUrl(adjustedUsername, SIP_PDU::Method_SUBSCRIBE, PSafeReadOnly);

  if (info == NULL)
    return FALSE;

  return info->IsRegistered ();
}


void SIPEndPoint::RegistrationRefresh(PTimer &, INT)
{
  SIPTransaction *request = NULL;
  OpalTransport *infoTransport = NULL;

  // Timer has elapsed
  for (PINDEX i = 0 ; i < activeSIPInfo.GetSize () ; i++) {

    PSafePtr<SIPInfo> info = activeSIPInfo.GetAt (i);

    if (info->GetExpire() == -1) {
      activeSIPInfo.Remove(info); // Was invalid the last time, delete it
    }
    else {

      // Need to refresh
      if (info->GetExpire() > 0 
	  && info->IsRegistered()
	  && info->GetTransport() != NULL 
	  && info->GetMethod() != SIP_PDU::Method_MESSAGE
	  && info->HasExpired()) {
	PTRACE(2, "SIP\tStarting REGISTER/SUBSCRIBE for binding refresh");
	infoTransport = info->GetTransport(); // Get current transport
	OpalTransportAddress registrarAddress = infoTransport->GetRemoteAddress();
	// Will update the transport if required. For example, if STUN
	// is used, and the external IP changed. Otherwise, OPAL would
	// keep registering the old IP.
	if (info->CreateTransport(registrarAddress)) { 
	  infoTransport = info->GetTransport();
	  info->RemoveTransactions();
	  info->SetExpire(info->GetExpire()*10/9);
	  request = info->CreateTransaction(*infoTransport, FALSE); 

	  if (request->Start()) 
	    info->AppendTransaction(request);
	  else {
	    delete request;
	    PTRACE(1, "SIP\tCould not start REGISTER/SUBSCRIBE for binding refresh");
	    info->SetExpire(-1); // Mark as Invalid
	  }
	}
	else {
	  PTRACE(1, "SIP\tCould not start REGISTER/SUBSCRIBE for binding refresh: Transport creation failed");
	  info->SetExpire(-1); // Mark as Invalid
	}
      }
      else if (info->HasExpired())
	info->SetExpire(-1); // Mark as Invalid
    }
  }

  activeSIPInfo.DeleteObjectsToBeRemoved();
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
    PTRACE(2, "SIP\tDid not start transaction on " << transport);
    return FALSE;
  }

  info->AppendTransaction(request);

  return TRUE;
}


BOOL SIPEndPoint::Register(const PString & host,
                           const PString & username,
			   const PString & authName,
                           const PString & password,
			   const PString & realm,
			   int timeout)
{
  if (timeout == 0)
    timeout = GetRegistrarTimeToLive().GetSeconds(); 
  return TransmitSIPInfo(SIP_PDU::Method_REGISTER, host, username, authName, password, realm, PString::Empty(), timeout);
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


BOOL SIPEndPoint::MWISubscribe(const PString & host, const PString & username, int timeout)
{
  if (timeout == 0)
    timeout = GetNotifierTimeToLive().GetSeconds(); 
  return TransmitSIPInfo (SIP_PDU::Method_SUBSCRIBE, host, username, timeout);
}


BOOL SIPEndPoint::TransmitSIPInfo(SIP_PDU::Methods m,
				  const PString & host,
				  const PString & username,
				  const PString & authName,
				  const PString & password,
				  const PString & realm,
				  const PString & body,
				  int timeout)
{
  PSafePtr<SIPInfo> info = NULL;
  OpalTransport *transport = NULL;
  SIPURL hosturl = SIPURL (host);
  
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
    // Do a DNS SRV lookup
#if P_DNS
    PIPSocketAddressAndPortVector addrs;
    if (PDNS::LookupSRV(hosturl.GetHostName(), "_sip._udp", hosturl.GetPort(), addrs)) {
      hostname = addrs[0].address.AsString();
      port = addrs [0].port;
    }
    else { 
#endif
      hostname = hosturl.GetHostName();
      port = hosturl.GetPort();
#if P_DNS
    }
#endif
  }
  else {
    hostname = proxy.GetHostName();
    port = proxy.GetPort();
    if (port == 0)
      port = defaultSignalPort;
  }

  OpalTransportAddress transportAddress(hostname, port, "udp");

  // Create the SIPInfo structure
  info = activeSIPInfo.FindSIPInfoByUrl(adjustedUsername, m, PSafeReadWrite);
  
  // if there is already a request with this URL and method, then update it with the new information
  if (info != NULL) {
    if (!password.IsEmpty())
      info->SetPassword(password); // Adjust the password if required 
    if (!realm.IsEmpty())
      info->SetAuthRealm(realm);   // Adjust the realm if required 
    if (!authName.IsEmpty())
      info->SetAuthUser(authName); // Adjust the authUser if required 
    if (!body.IsEmpty())
      info->SetBody(body);         // Adjust the body if required 
    info->SetExpire(timeout);      // Adjust the expire field
  } 
  // otherwise create a new request with this method type
  else {
    if (m == SIP_PDU::Method_REGISTER)
      info = new SIPRegisterInfo(*this, adjustedUsername, authName, password, timeout);
    else if (m == SIP_PDU::Method_SUBSCRIBE) {
      info = new SIPMWISubscribeInfo(*this, adjustedUsername, timeout);
    }
    else if (m == SIP_PDU::Method_MESSAGE)
      info = new SIPMessageInfo(*this, adjustedUsername, body);
    if (info == NULL) {
      PTRACE(1, "SIP\tUnknown SIP request method " << m);
      return FALSE;
    }
    activeSIPInfo.Append(info);
  }

  if (!info->CreateTransport(transportAddress)) {
    activeSIPInfo.Remove (info);
    return FALSE;
  }
  
  transport = info->GetTransport ();
    
  if (transport != NULL && !transport->WriteConnect(WriteSIPInfo, &*info)) {
    PTRACE(1, "SIP\tCould not write to " << transportAddress << " - " << transport->GetErrorText());
    activeSIPInfo.Remove (info);
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
  OpalTransport *registrarTransport = NULL;
  SIPTransaction *request = NULL;

  // Adjusted user name
  PString adjustedUsername = username;
  if (adjustedUsername.IsEmpty())
    adjustedUsername = GetDefaultLocalPartyName();

  if (adjustedUsername.Find('@') == P_MAX_INDEX)
    adjustedUsername += '@' + host;
  
    {
      PSafePtr<SIPInfo> info = NULL;
      info = activeSIPInfo.FindSIPInfoByUrl (adjustedUsername, method, PSafeReadWrite);
      if (info == NULL) {
        PTRACE(1, "SIP\tCould not find active registration/subscription for " << adjustedUsername);
        return FALSE;
      }
      
      registrarTransport = info->GetTransport();
      if (!info->IsRegistered() || registrarTransport == NULL) {
        PTRACE(1, "SIP\tRemoving local registration/subscription info for apparently unregistered/subscribed " << adjustedUsername);
        activeSIPInfo.Remove(info);
        return FALSE;
      }

      registrarTransport->SetRemoteAddress(info->GetRegistrarAddress());
      request = info->CreateTransaction(*registrarTransport, TRUE);

      if (!request->Start()) {
        PTRACE(1, "SIP\tCould not start UNREGISTER/UNSUBSCRIBE transaction");
        delete (request);
        request = NULL;
        return FALSE;
      }
      info->AppendTransaction(request);
      
      // Do this synchronously
      request->Wait ();
    }

  return TRUE;
}


void SIPEndPoint::ParsePartyName(const PString & remoteParty, PString & party)
{
  party = remoteParty;
  
#if P_DNS
  // if there is no '@', and then attempt to use ENUM
  if (remoteParty.Find('@') == P_MAX_INDEX) {

    // make sure the number has only digits
    PString e164 = remoteParty;
    if (e164.Left(4) *= "sip:")
      e164 = e164.Mid(4);
    PINDEX i;
    for (i = 0; i < e164.GetLength(); ++i)
      if (!isdigit(e164[i]) && (i != 0 || e164[0] != '+'))
	      break;
    if (i >= e164.GetLength()) {
      PString str;
      if (PDNS::ENUMLookup(e164, "E2U+SIP", str)) {
	      PTRACE(4, "SIP\tENUM converted remote party " << remoteParty << " to " << str);
	      party = str;
      }
    }
  }
#endif
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
  PSafePtr<SIPInfo> info = activeSIPInfo.FindSIPInfoByAuthRealm(realm, PString::Empty(), PSafeReadOnly);
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
  
  PSafePtr<SIPInfo> info = activeSIPInfo.FindSIPInfoByDomain(host, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (info == NULL) {
   
    PIPSocket::Address localIP(PIPSocket::GetDefaultIpAny());
    WORD localPort = GetDefaultSignalPort();
    if (!GetListeners().IsEmpty())
      GetListeners()[0].GetLocalAddress().GetIpAndPort(localIP, localPort);
    OpalTransportAddress address = OpalTransportAddress(localIP, localPort, "udp");
    SIPURL party(GetManager().GetDefaultUserName(), address, localPort);
    return party;
  }

  return info->GetRegistrationAddress();
}


const SIPURL SIPEndPoint::GetLocalURL(const OpalTransport &transport, const PString & userName)
{
  PIPSocket::Address ip(PIPSocket::GetDefaultIpAny());
  OpalTransportAddress contactAddress = transport.GetLocalAddress();
  WORD contactPort = GetDefaultSignalPort();
  if (transport.IsRunning())
    transport.GetLocalAddress().GetIpAndPort(ip, contactPort);
  else if (!GetListeners().IsEmpty())
    GetListeners()[0].GetLocalAddress().GetIpAndPort(ip, contactPort);

  PIPSocket::Address localIP;
  WORD localPort;
  
  if (contactAddress.GetIpAndPort(localIP, localPort)) {
    PIPSocket::Address remoteIP;
    if (transport.GetRemoteAddress().GetIpAddress(remoteIP)) {
      GetManager().TranslateIPAddress(localIP, remoteIP); 	 
      PIPSocket::Address _localIP(localIP);
      PSTUNClient * stun = manager.GetSTUN(remoteIP);
      if (stun != NULL || localIP != _localIP)
	contactPort = localPort;
      contactAddress = OpalTransportAddress(localIP, contactPort, "udp");
    }
  }

  SIPURL contact(userName, contactAddress, contactPort);

  return contact;
}


BOOL SIPEndPoint::SendMessage (const SIPURL & url, 
			       const PString & body)
{
  return TransmitSIPInfo(SIP_PDU::Method_MESSAGE, url.AsQuotedString(), url.GetUserName(), "", "", "", body);
}


void SIPEndPoint::OnRTPStatistics(const SIPConnection & /*connection*/,
                                  const RTP_Session & /*session*/) const
{
}
// End of file ////////////////////////////////////////////////////////////////
