/*
 * h235auth.cxx
 *
 * H.235 security PDU's
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * Contributor(s): __________________________________
 *
 */

#include <ptlib.h>

#include <opal_config.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h235auth.h"
#endif

#include <ptclib/random.h>
#include <ptclib/cypher.h>

#include <opal/endpoint.h>
#include <h323/h235auth.h>
#include <h323/h323pdu.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H235Authenticator::H235Authenticator()
{
  m_enabled = true;
  sentRandomSequenceNumber = PRandom::Number()&INT_MAX;
  lastRandomSequenceNumber = 0;
  lastTimestamp = 0;
  timestampGracePeriod = 2*60*60+10; // 2 hours 10 seconds to allow for DST adjustments
  usage = GKAdmission;
}


void H235Authenticator::PrintOn(ostream & strm) const
{
  PWaitAndSignal m(mutex);

  strm << GetName() << '<';
  if (!IsEnabled())
    strm << "disabled";
  else if (password.IsEmpty())
    strm << "no-pwd";
  else
    strm << "active";
  strm << '>';
}


PBoolean H235Authenticator::PrepareTokens(PASN_Array & clearTokens, PASN_Array & cryptoTokens, unsigned rasPDU)
{
  PWaitAndSignal m(mutex);

  if (!IsEnabled() || !IsSecuredPDU(rasPDU, false))
    return false;

  H235_ClearToken * clearToken = CreateClearToken(rasPDU);
  if (clearToken != NULL) {
    // Check if already have a token of thsi type and overwrite it
    for (PINDEX i = 0; i < clearTokens.GetSize(); i++) {
      H235_ClearToken & oldToken = (H235_ClearToken &)clearTokens[i];
      if (clearToken->m_tokenOID == oldToken.m_tokenOID) {
        oldToken = *clearToken;
        delete clearToken;
        clearToken = NULL;
        break;
      }
    }

    if (clearToken != NULL)
      clearTokens.Append(clearToken);
  }

  H225_CryptoH323Token * cryptoToken;
  if ((cryptoToken = CreateCryptoToken(false, rasPDU)) != NULL)
    cryptoTokens.Append(cryptoToken);

  if ((cryptoToken = CreateCryptoToken(true, rasPDU)) != NULL)
    cryptoTokens.Append(cryptoToken);

  return true;
}


H235_ClearToken * H235Authenticator::CreateClearToken(unsigned)
{
  return CreateClearToken();
}


H235_ClearToken * H235Authenticator::CreateClearToken()
{
  return NULL;
}


H225_CryptoH323Token * H235Authenticator::CreateCryptoToken(bool digits, unsigned)
{
  return CreateCryptoToken(digits);
}


H225_CryptoH323Token * H235Authenticator::CreateCryptoToken(bool)
{
  return NULL;
}


PBoolean H235Authenticator::Finalise(PBYTEArray & /*rawPDU*/)
{
  return true;
}


H235Authenticator::ValidationResult H235Authenticator::ValidateTokens(
                                        const PASN_Array & clearTokens,
                                        const PASN_Array & cryptoTokens,
                                        const PBYTEArray & rawPDU)
{
  PWaitAndSignal m(mutex);

  if (!IsEnabled())
    return e_Disabled;

  PINDEX i;
  for (i = 0; i < clearTokens.GetSize(); i++) {
    ValidationResult s = ValidateClearToken((H235_ClearToken &)clearTokens[i]);
    if (s != e_Absent)
      return s;
  }

  for (i = 0; i < cryptoTokens.GetSize(); i++) {
    ValidationResult s = ValidateCryptoToken((H225_CryptoH323Token &)cryptoTokens[i], rawPDU);
    if (s != e_Absent)
      return s;
  }

  return e_Absent;
}


H235Authenticator::ValidationResult H235Authenticator::ValidateClearToken(
                                                 const H235_ClearToken & /*clearToken*/)
{
  return e_Absent;
}


H235Authenticator::ValidationResult H235Authenticator::ValidateCryptoToken(
                                            const H225_CryptoH323Token & /*cryptoToken*/,
                                            const PBYTEArray & /*rawPDU*/)
{
  return e_Absent;
}


PBoolean H235Authenticator::UseGkAndEpIdentifiers() const
{
  return false;
}


PBoolean H235Authenticator::IsSecuredPDU(unsigned, PBoolean) const
{
  return true;
}


PINDEX H235Authenticator::AddCapabilityIfNeeded(unsigned mechanism,
                                                const PString & oid,
                                                H225_ArrayOf_AuthenticationMechanism & mechanisms,
                                                H225_ArrayOf_PASN_ObjectId & algorithmOIDs)
{
  PWaitAndSignal m(mutex);

  if (!IsEnabled()) {
    PTRACE(3, "RAS\tAuthenticator " << *this << " not enabled during GRQ SetCapability negotiation");
    return P_MAX_INDEX;
  }

  PINDEX i;
  PINDEX size = algorithmOIDs.GetSize();
  for (i = 0; i < size; i++) {
    if (algorithmOIDs[i] == oid)
      break;
  }
  if (i >= size) {
    algorithmOIDs.SetSize(size+1);
    algorithmOIDs[size] = oid;
  }

  size = mechanisms.GetSize();
  for (i = 0; i < size; i++) {
    if (mechanisms[i].GetTag() == mechanism)
      return i;
  }

  mechanisms.SetSize(size + 1);
  mechanisms[size].SetTag(mechanism);
  return size;
}


///////////////////////////////////////////////////////////////////////////////

void H235Authenticators::InternalPreparePDU(H323TransactionPDU & pdu,
                                            PASN_Array & clearTokens,
                                            unsigned clearOptionalField,
                                            PASN_Array & cryptoTokens,
                                            unsigned cryptoOptionalField)
{
  // Clean out any crypto tokens in case this is a retry message
  // and we are regenerating the tokens due to possible timestamp
  // issues. We don't do this for clear tokens which may be used by
  // other endpoints and should be passed through unchanged.
  cryptoTokens.RemoveAll();

  unsigned pduTag = pdu.GetChoice().GetTag();
  for (iterator iterAuth = begin(); iterAuth != end(); ++iterAuth) {
    if (iterAuth->IsEnabled() && iterAuth->PrepareTokens(clearTokens, cryptoTokens, pduTag)) {
      PTRACE(4, "H235RAS\tPrepared PDU with authenticator " << *iterAuth);
    }
  }

  PASN_Sequence & subPDU = (PASN_Sequence &)pdu.GetChoice().GetObject();
  if (clearTokens.GetSize() > 0)
    subPDU.IncludeOptionalField(clearOptionalField);

  if (cryptoTokens.GetSize() > 0)
    subPDU.IncludeOptionalField(cryptoOptionalField);
}


H235Authenticator::ValidationResult
       H235Authenticators::InternalValidatePDU(const H323TransactionPDU & pdu,
                                               const PASN_Array & clearTokens,
                                               unsigned clearOptionalField,
                                               const PASN_Array & cryptoTokens,
                                               unsigned cryptoOptionalField,
                                               const PBYTEArray & rawPDU)
{
  unsigned pduTag = pdu.GetChoice().GetTag();

  PBoolean noneActive = true;
  for (iterator iterAuth = begin(); iterAuth != end(); ++iterAuth) {
    if (iterAuth->IsEnabled() && iterAuth->IsSecuredPDU(pduTag, true)) {
      noneActive = false;
      break;
    }
  }

  if (noneActive)
    return H235Authenticator::e_OK;

  //do not accept non secure RAS Messages
  const PASN_Sequence & subPDU = (const PASN_Sequence &)pdu.GetChoice().GetObject();
  if (!subPDU.HasOptionalField(clearOptionalField) &&
      !subPDU.HasOptionalField(cryptoOptionalField)) {
    PTRACE(2, "H235RAS\tReceived unsecured RAS message (no crypto tokens),"
              " need one of:\n" << setfill(',') << *this << setfill(' '));
    return H235Authenticator::e_Absent;
  }

  for (iterator iterAuth = begin(); iterAuth != end(); ++iterAuth) {
    if (iterAuth->IsSecuredPDU(pduTag, true)) {
      H235Authenticator::ValidationResult result = iterAuth->ValidateTokens(clearTokens, cryptoTokens, rawPDU);
      switch (result) {
        case H235Authenticator::e_OK :
          PTRACE(4, "H235RAS\tAuthenticator " << *iterAuth << " succeeded");
          return H235Authenticator::e_OK;

        case H235Authenticator::e_Absent :
          PTRACE(4, "H235RAS\tAuthenticator " << *iterAuth << " absent from PDU");
          iterAuth->Disable();
          break;

        case H235Authenticator::e_Disabled :
          PTRACE(4, "H235RAS\tAuthenticator " << *iterAuth << " disabled");
          break;

        default : // Various other failure modes
          PTRACE(4, "H235RAS\tAuthenticator " << *iterAuth << " failed: " << (int)result);
          return result;
      }
    }
  }

  return H235Authenticator::e_Absent;
}


///////////////////////////////////////////////////////////////////////////////

static const char Name_MD5[] = "MD5";
static const char OID_MD5[] = "1.2.840.113549.2.5";

PFACTORY_CREATE(PFactory<H235Authenticator>, H235AuthSimpleMD5, Name_MD5, false);

H235AuthSimpleMD5::H235AuthSimpleMD5()
{
  usage = AnyApplication;
}


PObject * H235AuthSimpleMD5::Clone() const
{
  return new H235AuthSimpleMD5(*this);
}


const char * H235AuthSimpleMD5::GetName() const
{
  return Name_MD5;
}


H225_CryptoH323Token * H235AuthSimpleMD5::CreateCryptoToken(bool digits)
{
  if (!IsEnabled())
    return NULL;

  if (localId.IsEmpty()) {
    PTRACE(2, "H235RAS\tH235AuthSimpleMD5 requires local ID for encoding.");
    return NULL;
  }

  if (digits && !OpalIsE164(localId, true))
    return NULL;

  // Cisco compatible hash calculation
  H235_ClearToken clearToken;

  // fill the PwdCertToken to calculate the hash
  clearToken.m_tokenOID = "0.0";

  // Create the H.225 crypto token
  H225_CryptoH323Token * cryptoToken = new H225_CryptoH323Token;
  cryptoToken->SetTag(H225_CryptoH323Token::e_cryptoEPPwdHash);
  H225_CryptoH323Token_cryptoEPPwdHash & cryptoEPPwdHash = *cryptoToken;

  // Set the alias
  if (digits) {
    cryptoEPPwdHash.m_alias.SetTag(H225_AliasAddress::e_dialedDigits);
    (PASN_IA5String &)cryptoEPPwdHash.m_alias = localId;
  }
  else {
    cryptoEPPwdHash.m_alias.SetTag(H225_AliasAddress::e_h323_ID);
    /* Avaya ECS Gatekeeper needs a trailing NULL character.
       Awaiting compatibility errors with other gatekeepers now. */
    (PASN_BMPString &)cryptoEPPwdHash.m_alias = localId + '\0';
  }

  // Use SetValueRaw to make sure trailing NULL is included
  clearToken.IncludeOptionalField(H235_ClearToken::e_generalID);
  clearToken.m_generalID.SetValueRaw(localId.AsUCS2());

  clearToken.IncludeOptionalField(H235_ClearToken::e_password);
  clearToken.m_password.SetValueRaw(password.AsUCS2());

  clearToken.IncludeOptionalField(H235_ClearToken::e_timeStamp);
  clearToken.m_timeStamp = (int)PTime().GetTimeInSeconds();

  // Encode it into PER
  PPER_Stream strm;
  clearToken.Encode(strm);
  strm.CompleteEncoding();

  // Generate an MD5 of the clear tokens PER encoding.
  PMessageDigest5 stomach;
  stomach.Process(strm.GetPointer(), strm.GetSize());
  PMessageDigest5::Code digest;
  stomach.Complete(digest);

  // Set the token data that actually goes over the wire
  cryptoEPPwdHash.m_timeStamp = clearToken.m_timeStamp;
  cryptoEPPwdHash.m_token.m_algorithmOID = OID_MD5;
  cryptoEPPwdHash.m_token.m_hash.SetData(digest);

  return cryptoToken;
}


H235Authenticator::ValidationResult H235AuthSimpleMD5::ValidateCryptoToken(
                                             const H225_CryptoH323Token & cryptoToken,
                                             const PBYTEArray &)
{
  if (!IsEnabled())
    return e_Disabled;

  // verify the token is of correct type
  if (cryptoToken.GetTag() != H225_CryptoH323Token::e_cryptoEPPwdHash)
    return e_Absent;

  const H225_CryptoH323Token_cryptoEPPwdHash & cryptoEPPwdHash = cryptoToken;

  PString alias = H323GetAliasAddressString(cryptoEPPwdHash.m_alias);
  if (!remoteId.IsEmpty() && alias != remoteId) {
    PTRACE(1, "H235RAS\tH235AuthSimpleMD5 alias is \"" << alias
           << "\", should be \"" << remoteId << '"');
    return e_Error;
  }

  // Build the clear token
  H235_ClearToken clearToken;
  clearToken.m_tokenOID = "0.0";

  clearToken.IncludeOptionalField(H235_ClearToken::e_generalID);
  clearToken.m_generalID.SetValueRaw(alias.AsUCS2()); // Use SetValueRaw to make sure trailing NULL is included

  clearToken.IncludeOptionalField(H235_ClearToken::e_password);
  clearToken.m_password.SetValueRaw(password.AsUCS2());

  clearToken.IncludeOptionalField(H235_ClearToken::e_timeStamp);
  clearToken.m_timeStamp = cryptoEPPwdHash.m_timeStamp;

  // Encode it into PER
  PPER_Stream strm;
  clearToken.Encode(strm);
  strm.CompleteEncoding();

  // Generate an MD5 of the clear tokens PER encoding.
  PMessageDigest5 stomach;
  stomach.Process(strm.GetPointer(), strm.GetSize());
  PMessageDigest5::Code digest;
  stomach.Complete(digest);

  if (cryptoEPPwdHash.m_token.m_hash.GetData() == digest)
    return e_OK;

  PTRACE(1, "H235RAS\tH235AuthSimpleMD5 digest does not match.");
  return e_BadPassword;
}


PBoolean H235AuthSimpleMD5::IsCapability(const H235_AuthenticationMechanism & mechanism,
                                     const PASN_ObjectId & algorithmOID)
{
  return mechanism.GetTag() == H235_AuthenticationMechanism::e_pwdHash &&
         algorithmOID.AsString() == OID_MD5;
}


PBoolean H235AuthSimpleMD5::SetCapability(H225_ArrayOf_AuthenticationMechanism & mechanisms,
                                      H225_ArrayOf_PASN_ObjectId & algorithmOIDs)
{
  return AddCapabilityIfNeeded(H235_AuthenticationMechanism::e_pwdHash, OID_MD5, mechanisms, algorithmOIDs) != P_MAX_INDEX;
}


PBoolean H235AuthSimpleMD5::IsSecuredPDU(unsigned rasPDU, PBoolean received) const
{
  if (password.IsEmpty())
    return false;

  switch (rasPDU) {
    case H225_RasMessage::e_registrationRequest :
    case H225_RasMessage::e_unregistrationRequest :
    case H225_RasMessage::e_admissionRequest :
    case H225_RasMessage::e_disengageRequest :
    case H225_RasMessage::e_bandwidthRequest :
    case H225_RasMessage::e_infoRequestResponse :
      return received ? !remoteId.IsEmpty() : !localId.IsEmpty();

    default :
      return false;
  }
}


///////////////////////////////////////////////////////////////////////////////

static const char Name_CAT[] = "CAT";
static const char OID_CAT[] = "1.2.840.113548.10.1.2.1";

PFACTORY_CREATE(PFactory<H235Authenticator>, H235AuthCAT, Name_CAT, false);

H235AuthCAT::H235AuthCAT()
{
  usage = GKAdmission;
}


PObject * H235AuthCAT::Clone() const
{
  return new H235AuthCAT(*this);
}


const char * H235AuthCAT::GetName() const
{
  return Name_CAT;
}


H235_ClearToken * H235AuthCAT::CreateClearToken()
{
  if (!IsEnabled())
    return NULL;

  if (password.IsEmpty()) {
    PTRACE(4, "H235RAS\tH235AuthCAT requires password.");
    return NULL;
  }

  if (localId.IsEmpty()) {
    PTRACE(2, "H235RAS\tH235AuthCAT requires local ID for encoding.");
    return NULL;
  }

  H235_ClearToken * clearToken = new H235_ClearToken;

  // Cisco compatible hash calculation
  clearToken->m_tokenOID = OID_CAT;

  clearToken->IncludeOptionalField(H235_ClearToken::e_generalID);
  clearToken->m_generalID.SetValueRaw(localId.AsUCS2()); // Use SetValueRaw to make sure trailing NULL is included

  clearToken->IncludeOptionalField(H235_ClearToken::e_timeStamp);
  clearToken->m_timeStamp = (int)PTime().GetTimeInSeconds();
  PUInt32b timeStamp = (DWORD)clearToken->m_timeStamp;

  clearToken->IncludeOptionalField(H235_ClearToken::e_random);
  BYTE random = (BYTE)++sentRandomSequenceNumber;
  clearToken->m_random = (unsigned)random;

  // Generate an MD5 of the clear tokens PER encoding.
  PMessageDigest5 stomach;
  stomach.Process(&random, 1);
  stomach.Process(password);
  stomach.Process(&timeStamp, 4);
  PMessageDigest5::Code digest;
  stomach.Complete(digest);

  clearToken->IncludeOptionalField(H235_ClearToken::e_challenge);
  clearToken->m_challenge.SetValue(digest);

  return clearToken;
}


H235Authenticator::ValidationResult H235AuthCAT::ValidateClearToken(const H235_ClearToken & clearToken)
{
  if (!IsEnabled())
    return e_Disabled;

  if (password.IsEmpty()) {
    PTRACE(4, "H235RAS\tH235AuthCAT requires password.");
    return e_BadPassword;
  }


  if (clearToken.m_tokenOID != OID_CAT)
    return e_Absent;

  if (!clearToken.HasOptionalField(H235_ClearToken::e_generalID) ||
      !clearToken.HasOptionalField(H235_ClearToken::e_timeStamp) ||
      !clearToken.HasOptionalField(H235_ClearToken::e_random) ||
      !clearToken.HasOptionalField(H235_ClearToken::e_challenge)) {
    PTRACE(1, "H235RAS\tCAT requires generalID, timeStamp, random and challenge fields");
    return e_Error;
  }

  //first verify the timestamp
  PTime now;
  int deltaTime = (int)now.GetTimeInSeconds() - clearToken.m_timeStamp;
  if (PABS(deltaTime) > timestampGracePeriod) {
    PTRACE(1, "H235RAS\tInvalid timestamp ABS(" << now.GetTimeInSeconds() << '-' 
           << (int)clearToken.m_timeStamp << ") > " << timestampGracePeriod);
    //the time has elapsed
    return e_InvalidTime;
  }

  //verify the randomnumber
  if (lastTimestamp == clearToken.m_timeStamp &&
      lastRandomSequenceNumber == clearToken.m_random) {
    //a message with this timespamp and the same random number was already verified
    PTRACE(1, "H235RAS\tConsecutive messages with the same random and timestamp");
    return e_ReplyAttack;
  }

  // save the values for the next call
  lastRandomSequenceNumber = clearToken.m_random;
  lastTimestamp = clearToken.m_timeStamp;
  
  if (!remoteId.IsEmpty() && clearToken.m_generalID.GetValue() != remoteId) {
    PTRACE(1, "H235RAS\tGeneral ID is \"" << clearToken.m_generalID.GetValue()
           << "\", should be \"" << remoteId << '"');
    return e_Error;
  }

  int randomInt = clearToken.m_random;
  if (randomInt < -127 || randomInt > 255) {
    PTRACE(1, "H235RAS\tCAT requires single byte random field, got " << randomInt);
    return e_Error;
  }

  PUInt32b timeStamp = (DWORD)clearToken.m_timeStamp;
  BYTE randomByte = (BYTE)randomInt;

  // Generate an MD5 of the clear tokens PER encoding.
  PMessageDigest5 stomach;
  stomach.Process(&randomByte, 1);
  stomach.Process(password);
  stomach.Process(&timeStamp, 4);
  PMessageDigest5::Code digest;
  stomach.Complete(digest);

  if (clearToken.m_challenge.GetValue() == digest)
    return e_OK;

  PTRACE(2, "H235RAS\tCAT hash does not match");
  return e_BadPassword;
}


PBoolean H235AuthCAT::IsCapability(const H235_AuthenticationMechanism & mechanism,
                                     const PASN_ObjectId & algorithmOID)
{
  if (mechanism.GetTag() != H235_AuthenticationMechanism::e_authenticationBES ||
         algorithmOID.AsString() != OID_CAT)
    return false;

  const H235_AuthenticationBES & bes = mechanism;
  return bes.GetTag() == H235_AuthenticationBES::e_radius;
}


PBoolean H235AuthCAT::SetCapability(H225_ArrayOf_AuthenticationMechanism & mechanisms,
                                H225_ArrayOf_PASN_ObjectId & algorithmOIDs)
{
  PINDEX idx = AddCapabilityIfNeeded(H235_AuthenticationMechanism::e_authenticationBES, OID_CAT, mechanisms, algorithmOIDs);
  if (idx == P_MAX_INDEX)
    return false;

  H235_AuthenticationBES & bes = mechanisms[idx];
  bes.SetTag(H235_AuthenticationBES::e_radius);
  return true;
}


PBoolean H235AuthCAT::IsSecuredPDU(unsigned rasPDU, PBoolean received) const
{
  if (password.IsEmpty())
    return false;

  switch (rasPDU) {
    case H225_RasMessage::e_registrationRequest :
    case H225_RasMessage::e_admissionRequest :
      return received ? !remoteId.IsEmpty() : !localId.IsEmpty();

    default :
      return false;
  }
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
