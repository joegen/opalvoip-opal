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
 * $Log: h235auth.cxx,v $
 * Revision 1.2009  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.7  2002/11/10 11:33:18  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.6  2002/09/04 06:01:48  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.5  2002/07/01 04:56:32  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.4  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.3  2001/08/17 08:31:54  robertj
 * Update from OpenH323
 *
 * Revision 2.2  2001/08/17 05:24:53  robertj
 * Fixed missing subdirectories in includes
 *
 * Revision 2.1  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 1.15  2002/11/11 07:04:22  robertj
 * Fixed typo in trace.
 *
 * Revision 1.14  2002/11/05 00:04:21  robertj
 * Returned code back to including trailing NULL in BMPString after
 *   PString::AsUCS2() implementation changes.
 *
 * Revision 1.13  2002/10/31 07:11:16  robertj
 * Added UTF-8/UCS-2 conversion functions to PString.
 *
 * Revision 1.12  2002/08/13 05:11:03  robertj
 * Removed redundent code.
 *
 * Revision 1.11  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.10  2002/08/05 05:17:41  robertj
 * Fairly major modifications to support different authentication credentials
 *   in ARQ to the logged in ones on RRQ. For both client and server.
 * Various other H.235 authentication bugs and anomalies fixed on the way.
 *
 * Revision 1.9  2002/06/24 00:11:21  robertj
 * Clarified error message during GRQ authentication.
 *
 * Revision 1.8  2002/05/17 03:40:09  robertj
 * Fixed problems with H.235 authentication on RAS for server and client.
 *
 * Revision 1.7  2001/09/21 04:55:27  robertj
 * Removed redundant code, thanks Chih-Wei Huang
 *
 * Revision 1.6  2001/09/14 00:13:39  robertj
 * Fixed problem with some athenticators needing extra conditions to be
 *   "active", so make IsActive() virtual and add localId to H235AuthSimpleMD5
 *
 * Revision 1.5  2001/09/13 01:15:20  robertj
 * Added flag to H235Authenticator to determine if gkid and epid is to be
 *   automatically set as the crypto token remote id and local id.
 *
 * Revision 1.4  2001/08/14 04:26:46  robertj
 * Completed the Cisco compatible MD5 authentications, thanks Wolfgang Platzer.
 *
 * Revision 1.3  2001/08/13 10:03:54  robertj
 * Fixed problem if do not have a localId when doing MD5 authentication.
 *
 * Revision 1.2  2001/08/10 13:49:35  robertj
 * Fixed alpha Linux warning.
 *
 * Revision 1.1  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h235auth.h"
#endif

#include <ptlib.h>
#include <ptclib/random.h>
#include <ptclib/cypher.h>

#include <h323/h235auth.h>
#include <h323/h323pdu.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H235Authenticator::H235Authenticator()
{
  enabled = TRUE;
  sentRandomSequenceNumber = PRandom::Number()&INT_MAX;
}


BOOL H235Authenticator::Prepare(H225_ArrayOf_CryptoH323Token & cryptoTokens)
{
  PINDEX lastToken = cryptoTokens.GetSize();
  cryptoTokens.SetSize(lastToken+1);
  if (PrepareToken(cryptoTokens[lastToken]))
    return TRUE;

  cryptoTokens.SetSize(lastToken);
  return FALSE;
}


H235Authenticator::State H235Authenticator::Verify(
                        const H225_ArrayOf_CryptoH323Token & cryptoTokens,
                        const PBYTEArray & rawPDU)
{
  if (password.IsEmpty())
    return e_Disabled;

  for (PINDEX i = 0; i < cryptoTokens.GetSize(); i++) {
    State s = VerifyToken(cryptoTokens[i], rawPDU);
    if (s != e_Absent)
      return s;
  }

  return e_Absent;
}


BOOL H235Authenticator::UseGkAndEpIdentifiers() const
{
  return FALSE;
}


BOOL H235Authenticator::IsActive() const
{
  return enabled && !password;
}


BOOL H235Authenticator::AddCapability(unsigned mechanism,
                                      const PString & oid,
                                      H225_ArrayOf_AuthenticationMechanism & mechanisms,
                                      H225_ArrayOf_PASN_ObjectId & algorithmOIDs)
{
  if (!IsActive()) {
    PTRACE(2, "RAS\tAuthenticator " << *this
            << " not active during GRQ SetCapability negotiation");
    return FALSE;
  }

  PINDEX i;
  PINDEX size = mechanisms.GetSize();
  for (i = 0; i < size; i++) {
    if (mechanisms[i].GetTag() == mechanism)
      break;
  }
  if (i >= size) {
    mechanisms.SetSize(size+1);
    mechanisms[size].SetTag(mechanism);
  }

  size = algorithmOIDs.GetSize();
  for (i = 0; i < size; i++) {
    if (algorithmOIDs[i] == oid)
      break;
  }
  if (i >= size) {
    algorithmOIDs.SetSize(size+1);
    algorithmOIDs[size] = oid;
  }

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

H235Authenticators H235Authenticators::Adjust(const PString & remoteId,
                                              const PString & localId,
                                              const PString & password) const
{
  H235Authenticators adjustedAuthenticators;

  PINDEX i;
  for (i = 0; i < GetSize(); i++) {
    H235Authenticator & authenticator = (*this)[i];
    if (authenticator.IsActive()) {
      H235Authenticator * adjustedAuthenticator = (H235Authenticator *)authenticator.Clone();
      adjustedAuthenticators.Append(adjustedAuthenticator);

      if (!remoteId)
        adjustedAuthenticator->SetRemoteId(remoteId);
      if (!localId)
        adjustedAuthenticator->SetLocalId(localId);
      adjustedAuthenticator->SetPassword(password);
    }
  }

  return adjustedAuthenticators;
}


void H235Authenticators::PreparePDU(H225_ArrayOf_CryptoH323Token & cryptoTokens,
                                    PASN_Sequence & pdu,
                                    unsigned optionalField) const
{
  // Clean out any crypto tokens in case this is a retry message
  // and we are regenerating the tokens due to possible timestamp
  // issues.
  cryptoTokens.RemoveAll();

  for (PINDEX i = 0; i < GetSize(); i++) {
    H235Authenticator & authenticator = (*this)[i];
    if (authenticator.Prepare(cryptoTokens)) {
      PTRACE(4, "H235RAS\tPrepared PDU with authenticator " << authenticator);
      pdu.IncludeOptionalField(optionalField);
    }
  }
}


BOOL H235Authenticators::ValidatePDU(const H225_ArrayOf_CryptoH323Token & cryptoTokens,
                                     const PASN_Sequence & pdu,
                                     unsigned optionalField,
                                     const PBYTEArray & rawPDU) const
{
  BOOL noneActive = TRUE;
  PINDEX i;
  for (i = 0; i < GetSize(); i++) {
    if ((*this)[i].IsActive()) {
      noneActive = FALSE;
      break;
    }
  }

  if (noneActive)
    return TRUE;

  //do not accept non secure RAS Messages
  if (!pdu.HasOptionalField(optionalField)) {
    PTRACE(2, "H235RAS\tReceived unsecured RAS message (no crypto tokens)");
    return FALSE;
  }

  BOOL ok = FALSE;
  for (i = 0; i < GetSize(); i++) {
    H235Authenticator & authenticator = (*this)[i];
    switch (authenticator.Verify(cryptoTokens, rawPDU)) {
      case H235Authenticator::e_OK :
        PTRACE(4, "H235RAS\tAuthenticator " << authenticator << " OK");
        ok = TRUE;
        break;

      case H235Authenticator::e_Absent :
        PTRACE(4, "H235RAS\tAuthenticator " << authenticator << " absent from GK");
        authenticator.Disable();
        break;

      case H235Authenticator::e_Disabled :
        PTRACE(4, "H235RAS\tAuthenticator " << authenticator << " disabled");
        break;

      default : // e_Error, e_Attacked
        break;
    }
  }

  return ok;
}


///////////////////////////////////////////////////////////////////////////////

static const char OID_MD5[] = "1.2.840.113549.2.5";

H235AuthSimpleMD5::H235AuthSimpleMD5()
{
}


PObject * H235AuthSimpleMD5::Clone() const
{
  return new H235AuthSimpleMD5(*this);
}


static PWORDArray GetUCS2plusNULL(const PString & str)
{
  PWORDArray ucs2 = str.AsUCS2();
  PINDEX len = ucs2.GetSize();
  if (len > 0 && ucs2[len-1] != 0)
    ucs2.SetSize(len+1);
  return ucs2;
}


BOOL H235AuthSimpleMD5::PrepareToken(H225_CryptoH323Token & cryptoToken)
{
  if (!IsActive())
    return FALSE;

  // Cisco compatible hash calculation
  H235_ClearToken clearToken;

  // fill the PwdCertToken to calculate the hash
  clearToken.m_tokenOID = "0.0";

  clearToken.IncludeOptionalField(H235_ClearToken::e_generalID);
  clearToken.m_generalID = GetUCS2plusNULL(localId);

  clearToken.IncludeOptionalField(H235_ClearToken::e_password);
  clearToken.m_password = GetUCS2plusNULL(password);

  clearToken.IncludeOptionalField(H235_ClearToken::e_timeStamp);
  clearToken.m_timeStamp = (int)time(NULL);

  // Encode it into PER
  PPER_Stream strm;
  clearToken.Encode(strm);
  strm.CompleteEncoding();

  // Generate an MD5 of the clear tokens PER encoding.
  PMessageDigest5 stomach;
  stomach.Process(strm.GetPointer(), strm.GetSize());
  PMessageDigest5::Code digest;
  stomach.Complete(digest);

  // Create the H.225 crypto token
  cryptoToken.SetTag(H225_CryptoH323Token::e_cryptoEPPwdHash);
  H225_CryptoH323Token_cryptoEPPwdHash & cryptoEPPwdHash = cryptoToken;

  // Set the token data that actually goes over the wire
  H323SetAliasAddress(localId, cryptoEPPwdHash.m_alias);

  cryptoEPPwdHash.m_timeStamp = clearToken.m_timeStamp;
  cryptoEPPwdHash.m_token.m_algorithmOID = OID_MD5;
  cryptoEPPwdHash.m_token.m_hash.SetData(sizeof(digest)*8, (const BYTE *)&digest);

  return TRUE;
}


BOOL H235AuthSimpleMD5::Finalise(PBYTEArray &)
{
  // Do not need to do anything.
  return TRUE;
}


H235Authenticator::State H235AuthSimpleMD5::VerifyToken(
                                     const H225_CryptoH323Token & cryptoToken,
                                     const PBYTEArray &)
{
  if (!IsActive())
    return e_Disabled;

  //verify the token is of correct type
  if (cryptoToken.GetTag() != H225_CryptoH323Token::e_cryptoEPPwdHash)
    return e_Absent;

  const H225_CryptoH323Token_cryptoEPPwdHash & cryptoEPPwdHash = cryptoToken;

  PString alias = H323GetAliasAddressString(cryptoEPPwdHash.m_alias);
  if (!remoteId && alias != remoteId) {
    PTRACE(1, "H235RAS\tH235AuthSimpleMD5 alias is \"" << alias
           << "\", should be \"" << remoteId << '"');
    return e_Error;
  }

  // Build the clear token
  H235_ClearToken clearToken;
  clearToken.m_tokenOID = "0.0";

  clearToken.IncludeOptionalField(H235_ClearToken::e_generalID);
  clearToken.m_generalID = GetUCS2plusNULL(alias);

  clearToken.IncludeOptionalField(H235_ClearToken::e_password);
  clearToken.m_password = GetUCS2plusNULL(password);

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

  if (cryptoEPPwdHash.m_token.m_hash.GetSize() == sizeof(digest)*8 &&
      memcmp(cryptoEPPwdHash.m_token.m_hash.GetDataPointer(), &digest, sizeof(digest)) == 0)
    return e_OK;

  PTRACE(1, "H235RAS\tH235AuthSimpleMD5 digest does not match.");
  return e_Attacked;
}


BOOL H235AuthSimpleMD5::IsCapability(const H235_AuthenticationMechanism & mechansim,
                                     const PASN_ObjectId & algorithmOID)
{
  return mechansim.GetTag() == H235_AuthenticationMechanism::e_pwdHash &&
         algorithmOID.AsString() == OID_MD5;
}


BOOL H235AuthSimpleMD5::SetCapability(H225_ArrayOf_AuthenticationMechanism & mechanisms,
                                      H225_ArrayOf_PASN_ObjectId & algorithmOIDs)
{
  return AddCapability(H235_AuthenticationMechanism::e_pwdHash, OID_MD5, mechanisms, algorithmOIDs);
}


BOOL H235AuthSimpleMD5::IsActive() const
{
  return !localId && H235Authenticator::IsActive();
}


/////////////////////////////////////////////////////////////////////////////
