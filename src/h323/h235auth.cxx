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
 * Revision 1.2002  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 1.2  2001/08/10 13:49:35  robertj
 * Fixed alpha Linux warning.
 *
 * Revision 1.1  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 */

#ifdef __GNUC__
#pragma implementation "h235auth.h"
#endif

#include <ptlib.h>
#include <ptclib/random.h>
#include <ptclib/cypher.h>

#include "h235auth.h"
#include "h323pdu.h"


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H235Authenticator::H235Authenticator()
{
  enabled = TRUE;
  sentRandomSequenceNumber = PRandom::Number();
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


BOOL H235Authenticator::SetCapability(H225_ArrayOf_AuthenticationMechanism &,
                                      H225_ArrayOf_PASN_ObjectId &)
{
  return FALSE;
}


///////////////////////////////////////////////////////////////////////////////

static const char OID_MD5[] = "1.2.840.113549.2.5";

H235AuthSimpleMD5::H235AuthSimpleMD5()
{
}


BOOL H235AuthSimpleMD5::PrepareToken(H225_CryptoH323Token & cryptoToken)
{
  if (!IsActive())
    return FALSE;

  // Build the clear token
  H235_ClearToken clearToken;
  clearToken.m_tokenOID = OID_MD5;
  clearToken.m_generalID = localId;
  clearToken.m_password = password;
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
    PTRACE(1, "H235RAS\tWrong alias.");
    return e_Error;
  }

  // Build the clear token
  H235_ClearToken clearToken;
  clearToken.m_tokenOID = OID_MD5;
  clearToken.m_generalID = alias;
  clearToken.m_password = password;
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

  PTRACE(1, "H235RAS\tMD5 digest does not match.");
  return e_Error;
}


BOOL H235AuthSimpleMD5::SetCapability(
                      H225_ArrayOf_AuthenticationMechanism & authenticationCapabilities,
                      H225_ArrayOf_PASN_ObjectId & algorithmOIDs)
{
  if (!IsActive())
    return FALSE;

  PINDEX size = authenticationCapabilities.GetSize();
  authenticationCapabilities.SetSize(size+1);
  authenticationCapabilities[size].SetTag(H235_AuthenticationMechanism::e_pwdHash);

  size = algorithmOIDs.GetSize();
  algorithmOIDs.SetSize(size+1);
  algorithmOIDs[size] = OID_MD5;
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
