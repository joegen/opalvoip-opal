/*
 * h323pdu.cxx
 *
 * H.323 PDU definitions
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
 * $Log: h323pdu.cxx,v $
 * Revision 1.2004  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.2  2001/08/17 08:29:44  robertj
 * Update from OpenH323
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.1  2001/08/13 05:10:40  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.85  2001/09/26 07:05:29  robertj
 * Fixed incorrect tags in building some PDU's, thanks Chris Purvis.
 *
 * Revision 1.84  2001/09/14 00:08:20  robertj
 * Optimised H323SetAliasAddress to use IsE164 function.
 *
 * Revision 1.83  2001/09/12 07:48:05  robertj
 * Fixed various problems with tracing.
 *
 * Revision 1.82  2001/08/16 07:49:19  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H323Connection.
 *
 * Revision 1.81  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.80  2001/08/06 03:08:56  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 *
 * Revision 1.79  2001/06/14 06:25:16  robertj
 * Added further H.225 PDU build functions.
 * Moved some functionality from connection to PDU class.
 *
 * Revision 1.78  2001/06/14 00:45:21  robertj
 * Added extra parameters for Q.931 fields, thanks Rani Assaf
 *
 * Revision 1.77  2001/06/05 03:14:41  robertj
 * Upgraded H.225 ASN to v4 and H.245 ASN to v7.
 *
 * Revision 1.76  2001/05/30 23:34:54  robertj
 * Added functions to send TCS=0 for transmitter side pause.
 *
 * Revision 1.75  2001/05/09 04:07:55  robertj
 * Added more call end codes for busy and congested.
 *
 * Revision 1.74  2001/05/03 06:45:21  robertj
 * Changed trace so dumps PDU if gets an error in decode.
 *
 * Revision 1.73  2001/04/11 03:01:29  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 * Revision 1.72  2001/03/24 00:58:03  robertj
 * Fixed MSVC warnings.
 *
 * Revision 1.71  2001/03/24 00:34:49  robertj
 * Added read/write hook functions so don't have to duplicate code in
 *    H323RasH235PDU descendant class of H323RasPDU.
 *
 * Revision 1.70  2001/03/23 05:38:30  robertj
 * Added PTRACE_IF to output trace if a conditional is TRUE.
 *
 * Revision 1.69  2001/03/02 06:59:59  robertj
 * Enhanced the globally unique identifier class.
 *
 * Revision 1.68  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.67  2001/01/18 06:04:46  robertj
 * Bullet proofed code so local alias can not be empty string. This actually
 *   fixes an ASN PER encoding bug causing an assert.
 *
 * Revision 1.66  2000/10/12 05:11:54  robertj
 * Added trace log if get transport error on writing PDU.
 *
 * Revision 1.65  2000/09/25 06:48:11  robertj
 * Removed use of alias if there is no alias present, ie only have transport address.
 *
 * Revision 1.64  2000/09/22 01:35:51  robertj
 * Added support for handling LID's that only do symmetric codecs.
 *
 * Revision 1.63  2000/09/20 01:50:22  craigs
 * Added ability to set jitter buffer on a per-connection basis
 *
 * Revision 1.62  2000/09/05 01:16:20  robertj
 * Added "security" call end reason code.
 *
 * Revision 1.61  2000/07/15 09:51:41  robertj
 * Changed adding of Q.931 party numbers to only occur in SETUP.
 *
 * Revision 1.60  2000/07/13 12:29:49  robertj
 * Added some more cause codes on release complete,
 *
 * Revision 1.59  2000/07/12 10:20:43  robertj
 * Fixed incorrect tag code in H.245 ModeChange reject PDU.
 *
 * Revision 1.58  2000/07/09 15:21:11  robertj
 * Changed reference to the word "field" to be more correct IE or "Information Element"
 * Fixed return value of Q.931/H.225 PDU read so returns TRUE if no H.225 data in the
 *     User-User IE. Just flag it as empty and continue processing PDU's.
 *
 * Revision 1.57  2000/06/21 23:59:44  robertj
 * Fixed copy/paste error setting Q.931 display name to incorrect value.
 *
 * Revision 1.56  2000/06/21 08:07:47  robertj
 * Added cause/reason to release complete PDU, where relevent.
 *
 * Revision 1.55  2000/06/07 05:48:06  robertj
 * Added call forwarding.
 *
 * Revision 1.54  2000/05/25 01:59:05  robertj
 * Fixed bugs in calculation of GlLobally Uniqie ID according to DCE/H.225 rules.
 *
 * Revision 1.53  2000/05/23 11:32:37  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H323Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.52  2000/05/15 08:38:59  robertj
 * Removed addition of calling/called party number field in Q.931 if there isn't one.
 *
 * Revision 1.51  2000/05/09 12:19:31  robertj
 * Added ability to get and set "distinctive ring" Q.931 functionality.
 *
 * Revision 1.50  2000/05/08 14:07:35  robertj
 * Improved the provision and detection of calling and caller numbers, aliases and hostnames.
 *
 * Revision 1.49  2000/05/08 05:06:27  robertj
 * Fixed bug in H.245 close logical channel timeout, thanks XuPeili.
 *
 * Revision 1.48  2000/05/02 04:32:27  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.47  2000/04/14 17:29:43  robertj
 * Fixed display of error message on timeout when timeouts are not errors.
 *
 * Revision 1.46  2000/04/10 20:39:18  robertj
 * Added support for more sophisticated DTMF and hook flash user indication.
 * Added function to extract E164 address from Q.931/H.225 PDU.
 *
 * Revision 1.45  2000/03/29 04:42:19  robertj
 * Improved some trace logging messages.
 *
 * Revision 1.44  2000/03/25 02:01:07  robertj
 * Added adjustable caller name on connection by connection basis.
 *
 * Revision 1.43  2000/03/21 01:08:10  robertj
 * Fixed incorrect call reference code being used in originated call.
 *
 * Revision 1.42  2000/02/17 12:07:43  robertj
 * Used ne wPWLib random number generator after finding major problem in MSVC rand().
 *
 * Revision 1.41  1999/12/23 22:47:09  robertj
 * Added calling party number field.
 *
 * Revision 1.40  1999/12/11 02:21:00  robertj
 * Added ability to have multiple aliases on local endpoint.
 *
 * Revision 1.39  1999/11/16 13:21:38  robertj
 * Removed extraneous error trace when doing asynchronous answer call.
 *
 * Revision 1.38  1999/11/15 14:11:29  robertj
 * Fixed trace output stream being put back after setting hex/fillchar modes.
 *
 * Revision 1.37  1999/11/10 23:30:20  robertj
 * Fixed unexpected closing of transport on PDU read error.
 *
 * Revision 1.36  1999/11/01 00:48:31  robertj
 * Added assert for illegal condition in capabilities, must have set if have table.
 *
 * Revision 1.35  1999/10/30 23:48:21  robertj
 * Fixed incorrect PDU type for H225 RAS location request.
 *
 * Revision 1.34  1999/10/29 03:35:06  robertj
 * Fixed setting of unique ID using fake MAC address from Win32 PPP device.
 *
 * Revision 1.33  1999/09/21 14:09:49  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.32  1999/09/10 09:03:01  robertj
 * Used new GetInterfaceTable() function to get ethernet address for UniqueID
 *
 * Revision 1.31  1999/09/10 03:36:48  robertj
 * Added simple Q.931 Status response to Q.931 Status Enquiry
 *
 * Revision 1.30  1999/08/31 12:34:19  robertj
 * Added gatekeeper support.
 *
 * Revision 1.29  1999/08/31 11:37:30  robertj
 * Fixed problem with apparently randomly losing signalling channel.
 *
 * Revision 1.28  1999/08/25 05:08:14  robertj
 * File fission (critical mass reached).
 *
 * Revision 1.27  1999/08/13 06:34:38  robertj
 * Fixed problem in CallPartyNumber Q.931 encoding.
 * Added field name display to Q.931 protocol.
 *
 * Revision 1.26  1999/08/10 13:14:15  robertj
 * Added Q.931 Called Number field if have "phone number" style destination addres.
 *
 * Revision 1.25  1999/08/10 11:38:03  robertj
 * Changed population of setup UUIE destinationAddress if can be IA5 string.
 *
 * Revision 1.24  1999/07/26 05:10:30  robertj
 * Fixed yet another race condition on connection termination.
 *
 * Revision 1.23  1999/07/16 14:03:52  robertj
 * Fixed bug in Master/Slave negotiation that can cause looping.
 *
 * Revision 1.22  1999/07/16 06:15:59  robertj
 * Corrected semantics for tunnelled master/slave determination in fast start.
 *
 * Revision 1.21  1999/07/16 02:15:30  robertj
 * Fixed more tunneling problems.
 *
 * Revision 1.20  1999/07/15 14:45:36  robertj
 * Added propagation of codec open error to shut down logical channel.
 * Fixed control channel start up bug introduced with tunnelling.
 *
 * Revision 1.19  1999/07/15 09:08:04  robertj
 * Added extra debugging for if have PDU decoding error.
 *
 * Revision 1.18  1999/07/15 09:04:31  robertj
 * Fixed some fast start bugs
 *
 * Revision 1.17  1999/07/10 02:51:36  robertj
 * Added mutexing in H245 procedures. Also fixed MSD state bug.
 *
 * Revision 1.16  1999/07/09 06:09:50  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.15  1999/06/25 10:25:35  robertj
 * Added maintentance of callIdentifier variable in H.225 channel.
 *
 * Revision 1.14  1999/06/22 13:45:40  robertj
 * Fixed conferenceIdentifier generation algorithm to bas as in spec.
 *
 * Revision 1.13  1999/06/19 15:18:38  robertj
 * Fixed bug in MasterSlaveDeterminationAck pdu has incorrect master/slave state.
 *
 * Revision 1.12  1999/06/14 15:08:40  robertj
 * Added GSM codec class frame work (still no actual codec).
 *
 * Revision 1.11  1999/06/14 06:39:08  robertj
 * Fixed problem with getting transmit flag to channel from PDU negotiator
 *
 * Revision 1.10  1999/06/14 05:15:56  robertj
 * Changes for using RTP sessions correctly in H323 Logical Channel context
 *
 * Revision 1.9  1999/06/13 12:41:14  robertj
 * Implement logical channel transmitter.
 * Fixed H245 connect on receiving call.
 *
 * Revision 1.8  1999/06/09 05:26:19  robertj
 * Major restructuring of classes.
 *
 * Revision 1.7  1999/06/06 06:06:36  robertj
 * Changes for new ASN compiler and v2 protocol ASN files.
 *
 * Revision 1.6  1999/04/26 06:20:22  robertj
 * Fixed bugs in protocol
 *
 * Revision 1.5  1999/04/26 06:14:47  craigs
 * Initial implementation for RTP decoding and lots of stuff
 * As a whole, these changes are called "First Noise"
 *
 * Revision 1.4  1999/02/23 11:04:28  robertj
 * Added capability to make outgoing call.
 *
 * Revision 1.3  1999/02/06 09:23:39  robertj
 * BeOS port
 *
 * Revision 1.2  1999/01/16 02:34:57  robertj
 * GNU compiler compatibility.
 *
 * Revision 1.1  1999/01/16 01:30:54  robertj
 * Initial revision
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323pdu.h"
#endif

#include <h323/h323pdu.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/transaddr.h>
#include <h323/h225ras.h>
#include <h323/h235auth.h>


#define new PNEW

const char H225_ProtocolID[] = "0.0.8.2250.0.2";
const char H245_ProtocolID[] = "0.0.8.245.0.3";


///////////////////////////////////////////////////////////////////////////////

void H323SetAliasAddresses(const PStringList & names,
                           H225_ArrayOf_AliasAddress & aliases)
{
  aliases.SetSize(names.GetSize());
  for (PINDEX i = 0; i < names.GetSize(); i++)
    H323SetAliasAddress(names[i], aliases[i]);
}


static BOOL IsE164(const PString & str)
{
  return strspn(str, "1234567890*#") == strlen(str);
}


void H323SetAliasAddress(const PString & name, H225_AliasAddress & alias)
{
  if (IsE164(name)) {
    alias.SetTag(H225_AliasAddress::e_dialedDigits);
    (PASN_IA5String &)alias = name;
  }
  else {
    // Could not encode it as a phone number, so do it as a full string.
    alias.SetTag(H225_AliasAddress::e_h323_ID);
    (PASN_BMPString &)alias = name;
  }
}


/////////////////////////////////////////////////////////////////////////////

PString H323GetAliasAddressString(const H225_AliasAddress & alias)
{
  switch (alias.GetTag()) {
    case H225_AliasAddress::e_dialedDigits :
    case H225_AliasAddress::e_url_ID :
    case H225_AliasAddress::e_email_ID :
      return (const PASN_IA5String &)alias;

    case H225_AliasAddress::e_h323_ID :
      return (const PASN_BMPString &)alias;

    case H225_AliasAddress::e_transportID :
      return H323TransportAddress(alias);

    case H225_AliasAddress::e_partyNumber :
    {
      const H225_PartyNumber & party = alias;
      switch (party.GetTag()) {
        case H225_PartyNumber::e_e164Number :
        {
          const H225_PublicPartyNumber & number = party;
          return "PublicParty:" + (PString)number.m_publicNumberDigits;
        }

        case H225_PartyNumber::e_privateNumber :
        {
          const H225_PrivatePartyNumber & number = party;
          return "PrivateParty:" + (PString)number.m_privateNumberDigits;
        }

        case H225_PartyNumber::e_dataPartyNumber :
        case H225_PartyNumber::e_telexPartyNumber :
        case H225_PartyNumber::e_nationalStandardPartyNumber :
          return (const H225_NumberDigits &)party;
      }
      break;
    }

    default :
      break;
  }

  return PString();
}


///////////////////////////////////////////////////////////////////////////////

H323SignalPDU::H323SignalPDU()
{
}


H225_Setup_UUIE & H323SignalPDU::BuildSetup(const H323Connection & connection,
                                            const H323TransportAddress & destAddr)
{
  H323EndPoint & endpoint = connection.GetEndPoint();

  q931pdu.BuildSetup(connection.GetCallReference());
  SetQ931Fields(connection, TRUE);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_setup);
  H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;
  setup.m_protocolIdentifier.SetValue(H225_ProtocolID);

  setup.IncludeOptionalField(H225_Setup_UUIE::e_sourceAddress);
  H323SetAliasAddresses(endpoint.GetAliasNames(), setup.m_sourceAddress);

  setup.m_conferenceID = OpalGloballyUniqueID();
  setup.m_conferenceGoal.SetTag(H225_Setup_UUIE_conferenceGoal::e_create);
  setup.m_callType.SetTag(H225_CallType::e_pointToPoint);

  setup.m_callIdentifier.m_guid = OpalGloballyUniqueID();
  setup.m_mediaWaitForConnect = FALSE;
  setup.m_canOverlapSend = FALSE;

  if (!destAddr) {
    setup.IncludeOptionalField(H225_Setup_UUIE::e_destCallSignalAddress);
    destAddr.SetPDU(setup.m_destCallSignalAddress);
  }

  PString destAlias = connection.GetRemotePartyName();
  if (!destAlias && destAlias != destAddr) {
    setup.IncludeOptionalField(H225_Setup_UUIE::e_destinationAddress);
    setup.m_destinationAddress.SetSize(1);

    // Try and encode it as a phone number
    H323SetAliasAddress(destAlias, setup.m_destinationAddress[0]);
    if (setup.m_destinationAddress[0].GetTag() == H225_AliasAddress::e_dialedDigits)
      q931pdu.SetCalledPartyNumber(destAlias);
  }

  endpoint.SetEndpointTypeInfo(setup.m_sourceInfo);

  return setup;
}


H225_CallProceeding_UUIE &
        H323SignalPDU::BuildCallProceeding(const H323Connection & connection)
{
  q931pdu.BuildCallProceeding(connection.GetCallReference());
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_callProceeding);
  H225_CallProceeding_UUIE & proceeding = m_h323_uu_pdu.m_h323_message_body;

  proceeding.m_protocolIdentifier.SetValue(H225_ProtocolID);
  proceeding.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  connection.GetEndPoint().SetEndpointTypeInfo(proceeding.m_destinationInfo);

  return proceeding;
}


H225_Connect_UUIE & H323SignalPDU::BuildConnect(const H323Connection & connection)
{
  q931pdu.BuildConnect(connection.GetCallReference());
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_connect);
  H225_Connect_UUIE & connect = m_h323_uu_pdu.m_h323_message_body;

  connect.m_protocolIdentifier.SetValue(H225_ProtocolID);
  connect.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  connect.m_conferenceID = connection.GetConferenceIdentifier();

  connection.GetEndPoint().SetEndpointTypeInfo(connect.m_destinationInfo);

  return connect;
}


H225_Connect_UUIE & H323SignalPDU::BuildConnect(const H323Connection & connection,
                                                const PIPSocket::Address & h245Address,
                                                WORD port)
{
  H225_Connect_UUIE & connect = BuildConnect(connection);

  // indicate we are including the optional H245 address in the PDU
  connect.IncludeOptionalField(H225_Connect_UUIE::e_h245Address);

  // convert IP address into the correct H245 type
  connect.m_h245Address.SetTag(H225_TransportAddress::e_ipAddress);
  H225_TransportAddress_ipAddress & ipAddress = connect.m_h245Address;
  ipAddress.m_ip.SetSize(4);
  ipAddress.m_ip[0] = h245Address.Byte1();
  ipAddress.m_ip[1] = h245Address.Byte2();
  ipAddress.m_ip[2] = h245Address.Byte3();
  ipAddress.m_ip[3] = h245Address.Byte4();
  ipAddress.m_port  = port;

  return connect;
}


H225_Alerting_UUIE & H323SignalPDU::BuildAlerting(const H323Connection & connection)
{
  q931pdu.BuildAlerting(connection.GetCallReference());
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_alerting);
  H225_Alerting_UUIE & alerting = m_h323_uu_pdu.m_h323_message_body;

  alerting.m_protocolIdentifier.SetValue(H225_ProtocolID);
  alerting.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  connection.GetEndPoint().SetEndpointTypeInfo(alerting.m_destinationInfo);

  return alerting;
}


H225_Information_UUIE & H323SignalPDU::BuildInformation(const H323Connection & connection)
{
  q931pdu.BuildInformation(connection.GetCallReference(), connection.HadAnsweredCall());
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_information);
  H225_Information_UUIE & information = m_h323_uu_pdu.m_h323_message_body;

  information.m_protocolIdentifier.SetValue(H225_ProtocolID);
  information.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return information;
}


H225_ReleaseComplete_UUIE &
        H323SignalPDU::BuildReleaseComplete(const H323Connection & connection)
{
  q931pdu.BuildReleaseComplete(connection.GetCallReference(), connection.HadAnsweredCall());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_releaseComplete);

  H225_ReleaseComplete_UUIE & release = m_h323_uu_pdu.m_h323_message_body;

  release.m_protocolIdentifier.SetValue(H225_ProtocolID);
  release.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  OpalCallEndReason reason = connection.GetCallEndReason();
  static Q931::CauseValues const Q931cause[OpalNumCallEndReasons] = {
    Q931::NormalCallClearing, /// Local endpoint application cleared call
    Q931::UserBusy,           /// Local endpoint did not accept call OnIncomingCall()=FALSE
    Q931::CallRejected,       /// Local endpoint declined to answer call
    Q931::NormalCallClearing, /// Remote endpoint application cleared call
    Q931::ErrorInCauseIE,     /// Remote endpoint refused call
    Q931::NormalCallClearing, /// Remote endpoint did not answer in required time
    Q931::NormalCallClearing, /// Remote endpoint stopped calling
    Q931::ErrorInCauseIE,     /// Transport error cleared call
    Q931::NoRouteToDestination, /// Transport connection failed to establish call
    Q931::NormalCallClearing, /// Gatekeeper has cleared call
    Q931::ErrorInCauseIE,     /// Call failed as could not find user (in GK)
    Q931::ErrorInCauseIE,     /// Call failed as could not get enough bandwidth
    Q931::ErrorInCauseIE,     /// Could not find common capabilities
    Q931::Redirection,        /// Call was forwarded using FACILITY message
    Q931::ErrorInCauseIE,     /// Call failed a security check and was ended
    Q931::UserBusy,           /// Local endpoint busy
    Q931::Congestion,          /// Local endpoint congested
    Q931::NormalCallClearing, /// Remote endpoint busy
    Q931::NormalCallClearing, /// Remote endpoint congested
  };
  if (Q931cause[reason] != Q931::ErrorInCauseIE)
    q931pdu.SetCause(Q931cause[reason]);

  static unsigned const H225reason[OpalNumCallEndReasons] = {
    0, /// Local endpoint application cleared call
    0, /// Local endpoint did not accept call OnIncomingCall()=FALSE
    0, /// Local endpoint declined to answer call
    0, /// Remote endpoint application cleared call
    1+H225_ReleaseCompleteReason::e_destinationRejection, /// Remote endpoint refused call
    0, /// Remote endpoint did not answer in required time
    0, /// Remote endpoint stopped calling
    1+H225_ReleaseCompleteReason::e_undefinedReason, /// Transport error cleared call
    1+H225_ReleaseCompleteReason::e_unreachableDestination, /// Transport connection failed to establish call
    0, /// Gatekeeper has cleared call
    1+H225_ReleaseCompleteReason::e_calledPartyNotRegistered, /// Call failed as could not find user (in GK)
    1+H225_ReleaseCompleteReason::e_noBandwidth, /// Call failed as could not get enough bandwidth
    1+H225_ReleaseCompleteReason::e_undefinedReason, /// Could not find common capabilities
    1+H225_ReleaseCompleteReason::e_facilityCallDeflection, /// Call was forwarded using FACILITY message
    1+H225_ReleaseCompleteReason::e_securityDenied,  /// Call failed a security check and was ended
    0, /// Local endpoint busy
    0, /// Local endpoint congested
    0, /// Remote endpoint busy
    0, /// Remote endpoint congested
  };
  if (H225reason[reason] != 0) {
    release.IncludeOptionalField(H225_ReleaseComplete_UUIE::e_reason);
    release.m_reason.SetTag(H225reason[reason]-1);
  }

  return release;
}


H225_Facility_UUIE * H323SignalPDU::BuildFacility(const H323Connection & connection,
                                                  BOOL empty)
{
  q931pdu.BuildFacility(connection.GetCallReference(), connection.HadAnsweredCall());
  if (empty) {
    m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_empty);
    return NULL;
  }

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_facility);
  H225_Facility_UUIE & fac = m_h323_uu_pdu.m_h323_message_body;

  fac.m_protocolIdentifier.SetValue(H225_ProtocolID);
  fac.IncludeOptionalField(H225_Facility_UUIE::e_callIdentifier);
  fac.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return &fac;
}


H225_Progress_UUIE & H323SignalPDU::BuildProgress(const H323Connection & connection)
{
  q931pdu.BuildProgress(connection.GetCallReference(), connection.HadAnsweredCall(), 0);
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_progress);
  H225_Progress_UUIE & progress = m_h323_uu_pdu.m_h323_message_body;

  progress.m_protocolIdentifier.SetValue(H225_ProtocolID);
  progress.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  connection.GetEndPoint().SetEndpointTypeInfo(progress.m_destinationInfo);

  return progress;
}


H225_Status_UUIE & H323SignalPDU::BuildStatus(const H323Connection & connection)
{
  q931pdu.BuildStatus(connection.GetCallReference(), connection.HadAnsweredCall());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_status);
  H225_Status_UUIE & status = m_h323_uu_pdu.m_h323_message_body;

  status.m_protocolIdentifier.SetValue(H225_ProtocolID);
  status.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return status;
}

H225_StatusInquiry_UUIE & H323SignalPDU::BuildStatusInquiry(const H323Connection & connection)
{
  q931pdu.BuildStatusEnquiry(connection.GetCallReference(), connection.HadAnsweredCall());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_statusInquiry);
  H225_StatusInquiry_UUIE & inquiry = m_h323_uu_pdu.m_h323_message_body;

  inquiry.m_protocolIdentifier.SetValue(H225_ProtocolID);
  inquiry.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return inquiry;
}


H225_SetupAcknowledge_UUIE & H323SignalPDU::BuildSetupAcknowledge(const H323Connection & connection)
{
  q931pdu.BuildSetupAcknowledge(connection.GetCallReference());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_setupAcknowledge);
  H225_SetupAcknowledge_UUIE & setupAck = m_h323_uu_pdu.m_h323_message_body;

  setupAck.m_protocolIdentifier.SetValue(H225_ProtocolID);
  setupAck.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return setupAck;
}


H225_Notify_UUIE & H323SignalPDU::BuildNotify(const H323Connection & connection)
{
  q931pdu.BuildNotify(connection.GetCallReference(), connection.HadAnsweredCall());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_notify);
  H225_Notify_UUIE & notify = m_h323_uu_pdu.m_h323_message_body;

  notify.m_protocolIdentifier.SetValue(H225_ProtocolID);
  notify.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return notify;
}


void H323SignalPDU::BuildQ931()
{
  // Encode the H225 PDu into the Q931 PDU as User-User data
  PPER_Stream strm;
  Encode(strm);
  strm.CompleteEncoding();
  q931pdu.SetIE(Q931::UserUserIE, strm);
}


void H323SignalPDU::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n"
       << setw(indent+10) << "q931pdu = " << setprecision(indent) << q931pdu << '\n'
       << setw(indent+10) << "h225pdu = " << setprecision(indent);
  H225_H323_UserInformation::PrintOn(strm);
  strm << '\n'
       << setw(indent-1) << "}";
}


BOOL H323SignalPDU::Read(OpalTransport & transport)
{
  PBYTEArray rawData;
  if (!transport.ReadPDU(rawData)) {
    PTRACE_IF(1, transport.GetErrorCode(PChannel::LastReadError) != PChannel::Timeout,
              "H225\tRead error (" << transport.GetErrorNumber(PChannel::LastReadError)
              << "): " << transport.GetErrorText(PChannel::LastReadError));
    return FALSE;
  }

  PTRACE(4, "H225\tReceived raw data:\n" << hex << setfill('0')
                                         << setprecision(2) << rawData
                                         << dec << setfill(' '));

  if (!q931pdu.Decode(rawData)) {
    PTRACE(1, "H225\tParse error of Q931 PDU");
    return FALSE;
  }

  if (!q931pdu.HasIE(Q931::UserUserIE)) {
    m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_empty);
    PTRACE(1, "H225\tNo Q931 User-User Information Element, "
              "Q.931 PDU:\n  " << setprecision(2) << q931pdu);
    return TRUE;
  }

  PPER_Stream strm = q931pdu.GetIE(Q931::UserUserIE);
  if (!Decode(strm)) {
    PTRACE(1, "H225\tRead error: PER decode failure in Q.931 User-User Information Element, "
              "Partial PDU:\n  " << setprecision(2) << *this);
    m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_empty);
    return TRUE;
  }

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PTRACE(4, "H225\tReceived PDU:\n  " << setprecision(2) << *this);
  }
  else {
    PTRACE(3, "H225\tReceived PDU: " << m_h323_uu_pdu.m_h323_message_body.GetTagName());
  }
#endif
  return TRUE;
}


BOOL H323SignalPDU::Write(OpalTransport & transport)
{
  if (!q931pdu.HasIE(Q931::UserUserIE) && m_h323_uu_pdu.m_h323_message_body.IsValid())
    BuildQ931();

  PBYTEArray rawData;
  if (!q931pdu.Encode(rawData))
    return FALSE;
  
#if PTRACING
  if (PTrace::CanTrace(4)) {
    PTRACE(4, "H225\tSending PDU:\n  " << setprecision(2) << *this << '\n'
                                       << hex << setfill('0')
                                       << setprecision(2) << rawData
                                       << dec << setfill(' '));
  }
  else {
    PTRACE(3, "H225\tSending PDU: " << m_h323_uu_pdu.m_h323_message_body.GetTagName());
  }
#endif

  if (transport.WritePDU(rawData))
    return TRUE;

  PTRACE(1, "H225\tWrite PDU failed ("
         << transport.GetErrorNumber(PChannel::LastWriteError)
         << "): " << transport.GetErrorText(PChannel::LastWriteError));
  return FALSE;
}


PString H323SignalPDU::GetSourceAliases(const OpalTransport * transport) const
{
  PString remoteHostName;
  
  if (transport != NULL)
    remoteHostName = transport->GetRemoteAddress().GetHostName();

  PString displayName = GetQ931().GetDisplayName();

  PStringStream aliases;
  if (displayName != remoteHostName)
    aliases << displayName;

  if (m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_setup) {
    const H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;

    if (remoteHostName.IsEmpty() &&
        setup.HasOptionalField(H225_Setup_UUIE::e_sourceCallSignalAddress)) {
      H323TransportAddress remoteAddress(setup.m_sourceCallSignalAddress);
      remoteHostName = remoteAddress.GetHostName();
    }

    if (setup.m_sourceAddress.GetSize() > 0) {
      BOOL needParen = !aliases.IsEmpty();
      BOOL needComma = FALSE;
      for (PINDEX i = 0; i < setup.m_sourceAddress.GetSize(); i++) {
        PString alias = H323GetAliasAddressString(setup.m_sourceAddress[i]);
        if (alias != displayName && alias != remoteHostName) {
          if (needComma)
            aliases << ", ";
          else if (needParen)
            aliases << " (";
          aliases << alias;
          needComma = TRUE;
        }
      }
      if (needParen && needComma)
        aliases << ')';
    }
  }

  if (aliases.IsEmpty())
    return remoteHostName;

  aliases << " [" << remoteHostName << ']';
  return aliases;
}


PString H323SignalPDU::GetDestinationAlias(BOOL firstAliasOnly) const
{
  PStringStream aliases;

  PString number;
  if (GetQ931().GetCalledPartyNumber(number)) {
    if (firstAliasOnly)
      return number;
    aliases << number;
  }

  if (m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_setup) {
    const H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;
    if (setup.m_destinationAddress.GetSize() > 0) {
      if (firstAliasOnly)
        return H323GetAliasAddressString(setup.m_destinationAddress[0]);

      for (PINDEX i = 0; i < setup.m_sourceAddress.GetSize(); i++) {
        if (!aliases.IsEmpty())
          aliases << '\t';
        aliases << H323GetAliasAddressString(setup.m_sourceAddress[i]);
      }
    }

    if (setup.HasOptionalField(H225_Setup_UUIE::e_destCallSignalAddress)) {
      if (aliases.IsEmpty())
        aliases << '\t';
      aliases << H323TransportAddress(setup.m_destCallSignalAddress);
    }
  }

  return aliases;
}


BOOL H323SignalPDU::GetSourceE164(PString & number) const
{
  if (GetQ931().GetCallingPartyNumber(number))
    return TRUE;

  if (m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_setup)
    return FALSE;

  const H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;
  if (!setup.HasOptionalField(H225_Setup_UUIE::e_sourceAddress))
    return FALSE;

  PINDEX i;
  for (i = 0; i < setup.m_sourceAddress.GetSize(); i++) {
    if (setup.m_sourceAddress[i].GetTag() == H225_AliasAddress::e_dialedDigits) {
      number = (PASN_IA5String &)setup.m_sourceAddress[i];
      return TRUE;
    }
  }

  for (i = 0; i < setup.m_sourceAddress.GetSize(); i++) {
    PString str = H323GetAliasAddressString(setup.m_sourceAddress[i]);
    if (IsE164(str)) {
      number = str;
      return TRUE;
    }
  }

  return FALSE;
}


BOOL H323SignalPDU::GetDestinationE164(PString & number) const
{
  if (GetQ931().GetCalledPartyNumber(number))
    return TRUE;

  if (m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_setup)
    return FALSE;

  const H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;
  if (!setup.HasOptionalField(H225_Setup_UUIE::e_destinationAddress))
    return FALSE;

  PINDEX i;
  for (i = 0; i < setup.m_destinationAddress.GetSize(); i++) {
    if (setup.m_destinationAddress[i].GetTag() == H225_AliasAddress::e_dialedDigits) {
      number = (PASN_IA5String &)setup.m_destinationAddress[i];
      return TRUE;
    }
  }

  for (i = 0; i < setup.m_destinationAddress.GetSize(); i++) {
    PString str = H323GetAliasAddressString(setup.m_destinationAddress[i]);
    if (IsE164(str)) {
      number = str;
      return TRUE;
    }
  }

  return FALSE;
}


unsigned H323SignalPDU::GetDistinctiveRing() const
{
  Q931::SignalInfo sig = GetQ931().GetSignalInfo();
  if (sig < Q931::SignalAlertingPattern0 || sig > Q931::SignalAlertingPattern7)
    return 0;

  return sig - Q931::SignalAlertingPattern0;
}


void H323SignalPDU::SetQ931Fields(const H323Connection & connection,
                                  BOOL insertPartyNumbers,
                                  unsigned plan,
                                  unsigned type,
                                  int presentation,
                                  int screening)
{
  PINDEX i;
  const PStringList & aliases = connection.GetEndPoint().GetAliasNames();

  PString number;
  PString localName = connection.GetLocalPartyName();
  if (IsE164(localName)) {
    number = localName;
    for (i = 0; i < aliases.GetSize(); i++) {
      if (!IsE164(aliases[i])) {
        q931pdu.SetDisplayName(aliases[i]);
        break;
      }
    }
  }
  else {
    q931pdu.SetDisplayName(localName);
    for (i = 0; i < aliases.GetSize(); i++) {
      if (IsE164(aliases[i])) {
        number = aliases[i];
        break;
      }
    }
  }

  if (insertPartyNumbers) {
    PString otherNumber = connection.GetRemotePartyNumber();
    if (otherNumber.IsEmpty()) {
      PString otherName = connection.GetRemotePartyName();
      if (IsE164(otherName))
        otherNumber = otherName;
    }

    if (connection.HadAnsweredCall()) {
      if (!number)
        q931pdu.SetCalledPartyNumber(number, plan, type);
      if (!otherNumber)
        q931pdu.SetCallingPartyNumber(otherNumber, presentation, screening);
    }
    else {
      if (!number)
        q931pdu.SetCallingPartyNumber(number, presentation, screening);
      if (!otherNumber)
        q931pdu.SetCalledPartyNumber(otherNumber, plan, type);
    }
  }

  unsigned ring = connection.GetDistinctiveRing();
  if (ring != 0)
    q931pdu.SetSignalInfo((Q931::SignalInfo)(ring + Q931::SignalAlertingPattern0));
}


/////////////////////////////////////////////////////////////////////////////

H245_RequestMessage & H323ControlPDU::Build(H245_RequestMessage::Choices request)
{
  SetTag(e_request);
  H245_RequestMessage & msg = *this;
  msg.SetTag(request);
  return msg;
}


H245_ResponseMessage & H323ControlPDU::Build(H245_ResponseMessage::Choices response)
{
  SetTag(e_response);
  H245_ResponseMessage & resp = *this;
  resp.SetTag(response);
  return resp;
}


H245_CommandMessage & H323ControlPDU::Build(H245_CommandMessage::Choices command)
{
  SetTag(e_command);
  H245_CommandMessage & cmd = *this;
  cmd.SetTag(command);
  return cmd;
}


H245_IndicationMessage & H323ControlPDU::Build(H245_IndicationMessage::Choices indication)
{
  SetTag(e_indication);
  H245_IndicationMessage & ind = *this;
  ind.SetTag(indication);
  return ind;
}


H245_MasterSlaveDetermination & 
      H323ControlPDU::BuildMasterSlaveDetermination(unsigned terminalType,
                                                    unsigned statusDeterminationNumber)
{
  H245_MasterSlaveDetermination & msd = Build(H245_RequestMessage::e_masterSlaveDetermination);
  msd.m_terminalType = terminalType;
  msd.m_statusDeterminationNumber = statusDeterminationNumber;
  return msd;
}


H245_MasterSlaveDeterminationAck &
      H323ControlPDU::BuildMasterSlaveDeterminationAck(BOOL isMaster)
{
  H245_MasterSlaveDeterminationAck & msda = Build(H245_ResponseMessage::e_masterSlaveDeterminationAck);
  msda.m_decision.SetTag(isMaster
                            ? H245_MasterSlaveDeterminationAck_decision::e_slave
                            : H245_MasterSlaveDeterminationAck_decision::e_master);
  return msda;
}


H245_MasterSlaveDeterminationReject &
      H323ControlPDU::BuildMasterSlaveDeterminationReject(unsigned cause)
{
  H245_MasterSlaveDeterminationReject & msdr = Build(H245_ResponseMessage::e_masterSlaveDeterminationReject);
  msdr.m_cause.SetTag(cause);
  return msdr;
}


H245_TerminalCapabilitySet &
      H323ControlPDU::BuildTerminalCapabilitySet(const H323Connection & connection,
                                                 unsigned sequenceNumber,
                                                 BOOL empty)
{
  H245_TerminalCapabilitySet & cap = Build(H245_RequestMessage::e_terminalCapabilitySet);

  cap.m_sequenceNumber = sequenceNumber;
  cap.m_protocolIdentifier = H245_ProtocolID;

  if (empty)
    return cap;

  cap.IncludeOptionalField(H245_TerminalCapabilitySet::e_multiplexCapability);
  cap.m_multiplexCapability.SetTag(H245_MultiplexCapability::e_h2250Capability);
  H245_H2250Capability & h225_0 = cap.m_multiplexCapability;
  h225_0.m_maximumAudioDelayJitter = connection.GetMaxAudioDelayJitter();
  h225_0.m_receiveMultipointCapability.m_mediaDistributionCapability.SetSize(1);
  h225_0.m_transmitMultipointCapability.m_mediaDistributionCapability.SetSize(1);
  h225_0.m_receiveAndTransmitMultipointCapability.m_mediaDistributionCapability.SetSize(1);

  // Set the table of capabilities
  connection.GetLocalCapabilities().BuildPDU(cap);

  return cap;
}


H245_TerminalCapabilitySetAck &
      H323ControlPDU::BuildTerminalCapabilitySetAck(unsigned sequenceNumber)
{
  H245_TerminalCapabilitySetAck & cap = Build(H245_ResponseMessage::e_terminalCapabilitySetAck);
  cap.m_sequenceNumber = sequenceNumber;
  return cap;
}


H245_TerminalCapabilitySetReject &
      H323ControlPDU::BuildTerminalCapabilitySetReject(unsigned sequenceNumber,
                                                       unsigned cause)
{
  H245_TerminalCapabilitySetReject & cap = Build(H245_ResponseMessage::e_terminalCapabilitySetReject);
  cap.m_sequenceNumber = sequenceNumber;
  cap.m_cause.SetTag(cause);

  return cap;
}


H245_OpenLogicalChannel &
      H323ControlPDU::BuildOpenLogicalChannel(unsigned forwardLogicalChannelNumber)
{
  H245_OpenLogicalChannel & open = Build(H245_RequestMessage::e_openLogicalChannel);
  open.m_forwardLogicalChannelNumber = forwardLogicalChannelNumber;
  return open;
}


H245_RequestChannelClose &
      H323ControlPDU::BuildRequestChannelClose(unsigned channelNumber,
                                               unsigned reason)
{
  H245_RequestChannelClose & rcc = Build(H245_RequestMessage::e_requestChannelClose);
  rcc.m_forwardLogicalChannelNumber = channelNumber;
  rcc.IncludeOptionalField(H245_RequestChannelClose::e_reason);
  rcc.m_reason.SetTag(reason);
  return rcc;
}


H245_CloseLogicalChannel &
      H323ControlPDU::BuildCloseLogicalChannel(unsigned channelNumber)
{
  H245_CloseLogicalChannel & clc = Build(H245_RequestMessage::e_closeLogicalChannel);
  clc.m_forwardLogicalChannelNumber = channelNumber;
  clc.m_source.SetTag(H245_CloseLogicalChannel_source::e_lcse);
  return clc;
}


H245_OpenLogicalChannelAck &
      H323ControlPDU::BuildOpenLogicalChannelAck(unsigned channelNumber)
{
  H245_OpenLogicalChannelAck & ack = Build(H245_ResponseMessage::e_openLogicalChannelAck);
  ack.m_forwardLogicalChannelNumber = channelNumber;
  return ack;
}


H245_OpenLogicalChannelReject &
      H323ControlPDU::BuildOpenLogicalChannelReject(unsigned channelNumber,
                                                    unsigned cause)
{
  H245_OpenLogicalChannelReject & reject = Build(H245_ResponseMessage::e_openLogicalChannelReject);
  reject.m_forwardLogicalChannelNumber = channelNumber;
  reject.m_cause.SetTag(cause);
  return reject;
}


H245_OpenLogicalChannelConfirm &
      H323ControlPDU::BuildOpenLogicalChannelConfirm(unsigned channelNumber)
{
  H245_OpenLogicalChannelConfirm & chan = Build(H245_IndicationMessage::e_openLogicalChannelConfirm);
  chan.m_forwardLogicalChannelNumber = channelNumber;
  return chan;
}


H245_CloseLogicalChannelAck &
      H323ControlPDU::BuildCloseLogicalChannelAck(unsigned channelNumber)
{
  H245_CloseLogicalChannelAck & chan = Build(H245_ResponseMessage::e_closeLogicalChannelAck);
  chan.m_forwardLogicalChannelNumber = channelNumber;
  return chan;
}


H245_RequestChannelCloseAck &
      H323ControlPDU::BuildRequestChannelCloseAck(unsigned channelNumber)
{
  H245_RequestChannelCloseAck & rcca = Build(H245_ResponseMessage::e_requestChannelCloseAck);
  rcca.m_forwardLogicalChannelNumber = channelNumber;
  return rcca;
}


H245_RequestChannelCloseReject &
      H323ControlPDU::BuildRequestChannelCloseReject(unsigned channelNumber)
{
  H245_RequestChannelCloseReject & rccr = Build(H245_ResponseMessage::e_requestChannelCloseReject);
  rccr.m_forwardLogicalChannelNumber = channelNumber;
  return rccr;
}


H245_RequestChannelCloseRelease &
      H323ControlPDU::BuildRequestChannelCloseRelease(unsigned channelNumber)
{
  H245_RequestChannelCloseRelease & rccr = Build(H245_IndicationMessage::e_requestChannelCloseRelease);
  rccr.m_forwardLogicalChannelNumber = channelNumber;
  return rccr;
}


H245_RequestMode & H323ControlPDU::BuildRequestMode(unsigned sequenceNumber)
{
  H245_RequestMode & rm = Build(H245_RequestMessage::e_requestMode);
  rm.m_sequenceNumber = sequenceNumber;

  return rm;
}


H245_RequestModeAck & H323ControlPDU::BuildRequestModeAck(unsigned sequenceNumber,
                                                          unsigned response)
{
  H245_RequestModeAck & ack = Build(H245_ResponseMessage::e_requestModeAck);
  ack.m_sequenceNumber = sequenceNumber;
  ack.m_response.SetTag(response);
  return ack;
}


H245_RequestModeReject & H323ControlPDU::BuildRequestModeReject(unsigned sequenceNumber,
                                                                unsigned cause)
{
  H245_RequestModeReject & reject = Build(H245_ResponseMessage::e_requestModeReject);
  reject.m_sequenceNumber = sequenceNumber;
  reject.m_cause.SetTag(cause);
  return reject;
}


H245_RoundTripDelayRequest &
      H323ControlPDU::BuildRoundTripDelayRequest(unsigned sequenceNumber)
{
  H245_RoundTripDelayRequest & req = Build(H245_RequestMessage::e_roundTripDelayRequest);
  req.m_sequenceNumber = sequenceNumber;
  return req;
}


H245_RoundTripDelayResponse &
      H323ControlPDU::BuildRoundTripDelayResponse(unsigned sequenceNumber)
{
  H245_RoundTripDelayResponse & resp = Build(H245_ResponseMessage::e_roundTripDelayResponse);
  resp.m_sequenceNumber = sequenceNumber;
  return resp;
}


H245_UserInputIndication &
      H323ControlPDU::BuildUserInputIndication(const PString & value)
{
  H245_UserInputIndication & ind = Build(H245_IndicationMessage::e_userInput);
  ind.SetTag(H245_UserInputIndication::e_alphanumeric);
  (PASN_GeneralString &)ind = value;
  return ind;
}


H245_UserInputIndication & H323ControlPDU::BuildUserInputIndication(char tone,
                                                                    unsigned duration,
                                                                    unsigned logicalChannel,
                                                                    unsigned rtpTimestamp)
{
  H245_UserInputIndication & ind = Build(H245_IndicationMessage::e_userInput);

  ind.SetTag(H245_UserInputIndication::e_signal);
  H245_UserInputIndication_signal & sig = ind;

  sig.m_signalType.SetValue(tone);

  if (duration > 0) {
    sig.IncludeOptionalField(H245_UserInputIndication_signal::e_duration);
    sig.m_duration = duration;
  }

  if (logicalChannel > 0) {
    sig.IncludeOptionalField(H245_UserInputIndication_signal::e_rtp);
    sig.m_rtp.m_logicalChannelNumber = logicalChannel;
    sig.m_rtp.m_timestamp = rtpTimestamp;
  }

  return ind;
}


H245_FunctionNotUnderstood &
      H323ControlPDU::BuildFunctionNotUnderstood(const H323ControlPDU & pdu)
{
  H245_FunctionNotUnderstood & fnu = Build(H245_IndicationMessage::e_functionNotUnderstood);

  switch (pdu.GetTag()) {
    case H245_MultimediaSystemControlMessage::e_request :
      fnu.SetTag(H245_FunctionNotUnderstood::e_request);
      (H245_RequestMessage &)fnu = (const H245_RequestMessage &)pdu;
      break;

    case H245_MultimediaSystemControlMessage::e_response :
      fnu.SetTag(H245_FunctionNotUnderstood::e_response);
      (H245_ResponseMessage &)fnu = (const H245_ResponseMessage &)pdu;
      break;

    case H245_MultimediaSystemControlMessage::e_command :
      fnu.SetTag(H245_FunctionNotUnderstood::e_command);
      (H245_CommandMessage &)fnu = (const H245_CommandMessage &)pdu;
      break;
  }

  return fnu;
}


H245_EndSessionCommand & H323ControlPDU::BuildEndSessionCommand(unsigned reason)
{
  H245_EndSessionCommand & end = Build(H245_CommandMessage::e_endSessionCommand);
  end.SetTag(reason);
  return end;
}


/////////////////////////////////////////////////////////////////////////////

H323RasPDU::H323RasPDU(H225_RAS & ras)
  : rasChannel(ras)
{
}


H225_GatekeeperRequest & H323RasPDU::BuildGatekeeperRequest(unsigned seqNum)
{
  SetTag(e_gatekeeperRequest);
  H225_GatekeeperRequest & grq = *this;
  grq.m_requestSeqNum = seqNum;
  grq.m_protocolIdentifier.SetValue(H225_ProtocolID);
  return grq;
}


H225_GatekeeperConfirm & H323RasPDU::BuildGatekeeperConfirm(unsigned seqNum)
{
  SetTag(e_gatekeeperConfirm);
  H225_GatekeeperConfirm & gcf = *this;
  gcf.m_requestSeqNum = seqNum;
  gcf.m_protocolIdentifier.SetValue(H225_ProtocolID);
  return gcf;
}


H225_GatekeeperReject & H323RasPDU::BuildGatekeeperReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_gatekeeperReject);
  H225_GatekeeperReject & grj = *this;
  grj.m_requestSeqNum = seqNum;
  grj.m_protocolIdentifier.SetValue(H225_ProtocolID);
  grj.m_rejectReason.SetTag(reason);
  return grj;
}


H225_RegistrationRequest & H323RasPDU::BuildRegistrationRequest(unsigned seqNum)
{
  SetTag(e_registrationRequest);
  H225_RegistrationRequest & rrq = *this;
  rrq.m_requestSeqNum = seqNum;
  rrq.m_protocolIdentifier.SetValue(H225_ProtocolID);
  return rrq;
}


H225_RegistrationConfirm & H323RasPDU::BuildRegistrationConfirm(unsigned seqNum)
{
  SetTag(e_registrationConfirm);
  H225_RegistrationConfirm & rcf = *this;
  rcf.m_requestSeqNum = seqNum;
  rcf.m_protocolIdentifier.SetValue(H225_ProtocolID);
  return rcf;
}


H225_RegistrationReject & H323RasPDU::BuildRegistrationReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_registrationReject);
  H225_RegistrationReject & rrj = *this;
  rrj.m_requestSeqNum = seqNum;
  rrj.m_protocolIdentifier.SetValue(H225_ProtocolID);
  rrj.m_rejectReason.SetTag(reason);
  return rrj;
}


H225_UnregistrationRequest & H323RasPDU::BuildUnregistrationRequest(unsigned seqNum)
{
  SetTag(e_unregistrationRequest);
  H225_UnregistrationRequest & urq = *this;
  urq.m_requestSeqNum = seqNum;
  return urq;
}


H225_UnregistrationConfirm & H323RasPDU::BuildUnregistrationConfirm(unsigned seqNum)
{
  SetTag(e_unregistrationConfirm);
  H225_UnregistrationConfirm & ucf = *this;
  ucf.m_requestSeqNum = seqNum;
  return ucf;
}


H225_UnregistrationReject & H323RasPDU::BuildUnregistrationReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_unregistrationReject);
  H225_UnregistrationReject & urj = *this;
  urj.m_requestSeqNum = seqNum;
  urj.m_rejectReason.SetTag(reason);
  return urj;
}


H225_LocationRequest & H323RasPDU::BuildLocationRequest(unsigned seqNum)
{
  SetTag(e_locationRequest);
  H225_LocationRequest & lrq = *this;
  lrq.m_requestSeqNum = seqNum;
  return lrq;
}


H225_LocationConfirm & H323RasPDU::BuildLocationConfirm(unsigned seqNum)
{
  SetTag(e_locationConfirm);
  H225_LocationConfirm & lcf = *this;
  lcf.m_requestSeqNum = seqNum;
  return lcf;
}


H225_LocationReject & H323RasPDU::BuildLocationReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_locationReject);
  H225_LocationReject & lrj = *this;
  lrj.m_requestSeqNum = seqNum;
  lrj.m_rejectReason.SetTag(reason);
  return lrj;
}


H225_AdmissionRequest & H323RasPDU::BuildAdmissionRequest(unsigned seqNum)
{
  SetTag(e_admissionRequest);
  H225_AdmissionRequest & arq = *this;
  arq.m_requestSeqNum = seqNum;
  return arq;
}


H225_AdmissionConfirm & H323RasPDU::BuildAdmissionConfirm(unsigned seqNum)
{
  SetTag(e_admissionConfirm);
  H225_AdmissionConfirm & acf = *this;
  acf.m_requestSeqNum = seqNum;
  return acf;
}


H225_AdmissionReject & H323RasPDU::BuildAdmissionReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_admissionReject);
  H225_AdmissionReject & arj = *this;
  arj.m_requestSeqNum = seqNum;
  arj.m_rejectReason.SetTag(reason);
  return arj;
}


H225_DisengageRequest & H323RasPDU::BuildDisengageRequest(unsigned seqNum)
{
  SetTag(e_disengageRequest);
  H225_DisengageRequest & drq = *this;
  drq.m_requestSeqNum = seqNum;
  return drq;
}


H225_DisengageConfirm & H323RasPDU::BuildDisengageConfirm(unsigned seqNum)
{
  SetTag(e_disengageConfirm);
  H225_DisengageConfirm & dcf = *this;
  dcf.m_requestSeqNum = seqNum;
  return dcf;
}


H225_DisengageReject & H323RasPDU::BuildDisengageReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_disengageReject);
  H225_DisengageReject & drj = *this;
  drj.m_requestSeqNum = seqNum;
  drj.m_rejectReason.SetTag(reason);
  return drj;
}


H225_BandwidthRequest & H323RasPDU::BuildBandwidthRequest(unsigned seqNum)
{
  SetTag(e_bandwidthRequest);
  H225_BandwidthRequest & brq = *this;
  brq.m_requestSeqNum = seqNum;
  return brq;
}


H225_BandwidthConfirm & H323RasPDU::BuildBandwidthConfirm(unsigned seqNum, unsigned bandWidth)
{
  SetTag(e_bandwidthConfirm);
  H225_BandwidthConfirm & bcf = *this;
  bcf.m_requestSeqNum = seqNum;
  bcf.m_bandWidth = bandWidth;
  return bcf;
}


H225_BandwidthReject & H323RasPDU::BuildBandwidthReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_bandwidthReject);
  H225_BandwidthReject & brj = *this;
  brj.m_requestSeqNum = seqNum;
  brj.m_rejectReason.SetTag(reason);
  return brj;
}


H225_InfoRequestResponse & H323RasPDU::BuildInfoRequestResponse(unsigned seqNum)
{
  SetTag(e_infoRequestResponse);
  H225_InfoRequestResponse & irr = *this;
  irr.m_requestSeqNum = seqNum;
  return irr;
}


H225_UnknownMessageResponse & H323RasPDU::BuildUnknownMessageResponse(unsigned seqNum)
{
  SetTag(e_unknownMessageResponse);
  H225_UnknownMessageResponse & umr = *this;
  umr.m_requestSeqNum = seqNum;
  return umr;
}


BOOL H323RasPDU::Read(OpalTransport & transport)
{
  if (!transport.ReadPDU(rawPDU)) {
    PTRACE(1, "H225RAS\tRead error ("
           << transport.GetErrorNumber(PChannel::LastReadError)
           << "): " << transport.GetErrorText(PChannel::LastReadError));
    return FALSE;
  }

  rawPDU.ResetDecoder();
  BOOL ok = Decode(rawPDU);
  if (!ok) {
    PTRACE(1, "H225RAS\tRead error: PER decode failure:\n  "
           << setprecision(2) << rawPDU << "\n "  << setprecision(2) << *this);
    SetTag(UINT_MAX);
    return TRUE;
  }

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PTRACE(4, "H225RAS\tReceived PDU:\n  " << setprecision(2) << rawPDU
                                 << "\n "  << setprecision(2) << *this);
  }
  else {
    PTRACE(3, "H225RAS\tReceived PDU: " << GetTagName());
  }
#endif

  return TRUE;
}


BOOL H323RasPDU::Write(OpalTransport & transport) const
{
  PPER_Stream strm;
  Encode(strm);
  strm.CompleteEncoding();

  // Finalise the security if present
  H235Authenticators authenticators = rasChannel.GetAuthenticators();
  for (PINDEX i = 0; i < authenticators.GetSize(); i++)
    authenticators[i].Finalise(strm);

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PTRACE(4, "H225RAS\tSending PDU:\n  " << setprecision(2) << *this
                                << "\n "  << setprecision(2) << strm);
  }
  else {
    PTRACE(3, "H225\tSending PDU: " << GetTagName());
  }
#endif

  if (transport.WritePDU(strm))
    return TRUE;

  PTRACE(1, "H225\tWrite PDU failed ("
         << transport.GetErrorNumber(PChannel::LastWriteError)
         << "): " << transport.GetErrorText(PChannel::LastWriteError));
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
