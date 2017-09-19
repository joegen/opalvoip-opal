/*
 * h235auth1.cxx
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
 * Contributor(s): Fürbass Franz <franz.fuerbass@infonova.at>
 *
 */

#include <ptlib.h>

#include <opal_config.h>
#if OPAL_PTLIB_SSL

#if OPAL_H323

#include <ptclib/pssl.h>
#include <ptclib/random.h>

#include <h323/h235auth.h>
#include <h323/h323pdu.h>


#define REPLY_BUFFER_SIZE 1024


static const char OID_A[] = "0.0.8.235.0.2.1";
static const char OID_T[] = "0.0.8.235.0.2.5";
static const char OID_U[] = "0.0.8.235.0.2.6";

#define OID_VERSION_OFFSET 5

#define HASH_SIZE 12

static const BYTE SearchPattern[HASH_SIZE] = { // Must be 12 bytes
  't', 'W', 'e', 'l', 'V', 'e', '~', 'b', 'y', 't', 'e', 'S'
};


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

/* Function to print the digest */
#if 0
static void pr_sha(FILE* fp, char* s, int t)
{
        int     i ;

        fprintf(fp, "0x") ;
        for (i = 0 ; i < t ; i++)
                fprintf(fp, "%02x", s[i]) ;
        fprintf(fp, "0x") ;
}
#endif

static void truncate(unsigned char*    d1,    /* data to be truncated */
                     char*             d2,    /* truncated data */
                     int               len)   /* length in bytes to keep */
{
        int     i ;
        for (i = 0 ; i < len ; i++) d2[i] = d1[i];
}

/* Function to compute the digest */
static void hmac_sha (const unsigned char*    k,      /* secret key */
                      int      lk,              /* length of the key in bytes */
                      const unsigned char*    d,      /* data */
                      int      ld,              /* length of data in bytes */
                      char*    out,             /* output buffer, at least "t" bytes */
                      size_t   t)
{
  PSHA1Context::Digest key;
  char    buf[PSHA1Context::BlockSize];
  int     i ;

  if (lk > PSHA1Context::BlockSize) {
    PSHA1Context tctx;

    tctx.Update(k, lk);
    tctx.Finalise(key);

    k = key;
    lk = sizeof(key);
  }

  /**** Inner Digest ****/
  PSHA1Context ictx;

  /* Pad the key for inner digest */
  for (i = 0 ; i < lk ; ++i)
    buf[i] = (char)(k[i] ^ 0x36);
  for (i = lk ; i < PSHA1Context::BlockSize ; ++i)
    buf[i] = 0x36;

  ictx.Update(buf, PSHA1Context::BlockSize) ;
  ictx.Update(d, ld) ;

  PSHA1Context::Digest isha;
  ictx.Finalise(isha);

  /**** Outter Digest ****/
  PSHA1Context octx;

  /* Pad the key for outter digest */

  for (i = 0 ; i < lk ; ++i)
    buf[i] = (char)(k[i] ^ 0x5C);
  for (i = lk ; i < PSHA1Context::BlockSize ; ++i)
    buf[i] = 0x5C;

  octx.Update(buf, PSHA1Context::BlockSize);
  octx.Update(isha, sizeof(isha));

  PSHA1Context::Digest osha;
  octx.Finalise(osha);

  /* truncate and print the results */
  truncate(osha, out, std::min(t, sizeof(osha)));
}


/////////////////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PFactory<H235Authenticator>, H235AuthProcedure1, "H235Procedure1", false);

H235AuthProcedure1::H235AuthProcedure1()
{
  /* NOTE in h323plus, this method somehow isn't used in GKAdmission even
     though the usage was set to AnyApplication.
     Not sure if EPAuthentication is correct but in this version,
     GKAdmission fails when this auth proceedure is enabled. */
    usage = EPAuthentication;
}


PObject * H235AuthProcedure1::Clone() const
{
  H235AuthProcedure1 * auth = new H235AuthProcedure1(*this);

  // We do NOT copy these fields in Clone()
  auth->lastRandomSequenceNumber = 0;
  auth->lastTimestamp = 0;

  return auth;
}


const char * H235AuthProcedure1::GetName() const
{
  return "H235AnnexD_Procedure1";
}


H225_CryptoH323Token * H235AuthProcedure1::CreateCryptoToken(bool digits)
{
  if (!IsEnabled() || digits)
    return NULL;

  if (password.IsEmpty()) {
    PTRACE(4, "H235RAS\tH235Procedure1 requires password.");
    return NULL;
  }


  H225_CryptoH323Token * cryptoToken = new H225_CryptoH323Token;

  // Create the H.225 crypto token in the H323 crypto token
  cryptoToken->SetTag(H225_CryptoH323Token::e_nestedcryptoToken);
  H235_CryptoToken & nestedCryptoToken = *cryptoToken;

  // We are doing hashed password
  nestedCryptoToken.SetTag(H235_CryptoToken::e_cryptoHashedToken);
  H235_CryptoToken_cryptoHashedToken & cryptoHashedToken = nestedCryptoToken;

  // tokenOID = "A"
  cryptoHashedToken.m_tokenOID = OID_A;
  
  //ClearToken
  H235_ClearToken & clearToken = cryptoHashedToken.m_hashedVals;
  
  // tokenOID = "T"
  clearToken.m_tokenOID  = OID_T;
  
  if (!remoteId.IsEmpty()) {
    clearToken.IncludeOptionalField(H235_ClearToken::e_generalID);
    clearToken.m_generalID = remoteId;
  }

  if (!localId.IsEmpty()) {
    clearToken.IncludeOptionalField(H235_ClearToken::e_sendersID);
    clearToken.m_sendersID = localId;
  }
  
  clearToken.IncludeOptionalField(H235_ClearToken::e_timeStamp);
  clearToken.m_timeStamp = (int)PTime().GetTimeInSeconds();

  clearToken.IncludeOptionalField(H235_ClearToken::e_random);
  clearToken.m_random = ++sentRandomSequenceNumber;

  //H235_HASHED
  H235_HASHED<H235_EncodedGeneralToken> & encodedToken = cryptoHashedToken.m_token;
  
  //  algorithmOID = "U"
  encodedToken.m_algorithmOID = OID_U;


  /*******
   * step 1
   *
   * set a pattern for the hash value
   *
   */

  encodedToken.m_hash.SetData(HASH_SIZE*8, SearchPattern);
  return cryptoToken;
}


PBoolean H235AuthProcedure1::Finalise(PBYTEArray & rawPDU)
{
  if (!IsEnabled())
    return false;

  // Find the pattern

  int foundat = -1;
  for (PINDEX i = 0; i <= rawPDU.GetSize() - HASH_SIZE; i++) {
    if (memcmp(&rawPDU[i], SearchPattern, HASH_SIZE) == 0) { // i'v found it !
      foundat = i;
      break;
    }
  }
  
  if (foundat == -1) {
    //Can't find the search pattern in the ASN1 packet.
    PTRACE(1, "H235RAS\tPDU not prepared for H235AuthProcedure1");
    return false;
  }
  
  // Zero out the search pattern
  memset(&rawPDU[foundat], 0, HASH_SIZE);

 /*******
  * 
  * generate a HMAC-SHA1 key over the hole message
  * and save it in at (step 3) located position.
  * in the asn1 packet.
  */
  
  char key[HASH_SIZE];
 
  /** make a SHA1 hash before send to the hmac_sha1 */
  unsigned char secretkey[20];
  PSHA1Context::Process(password, secretkey);
  hmac_sha(secretkey, 20, rawPDU.GetPointer(), rawPDU.GetSize(), key, HASH_SIZE);
  memcpy(&rawPDU[foundat], key, HASH_SIZE);
  
  PTRACE(4, "H235RAS\tH235AuthProcedure1 hashing completed: \"" << password << '"');
  return true;
}


static PBoolean CheckOID(const PASN_ObjectId & oid1, const PASN_ObjectId & oid2)
{
  if (oid1.GetSize() != oid2.GetSize())
    return false;

  PINDEX i;
  for (i = 0; i < OID_VERSION_OFFSET; i++) {
    if (oid1[i] != oid2[i])
      return false;
  }

  for (i++; i < oid1.GetSize(); i++) {
    if (oid1[i] != oid2[i])
      return false;
  }

  return true;
}


H235Authenticator::ValidationResult H235AuthProcedure1::ValidateCryptoToken(
                                            const H225_CryptoH323Token & cryptoToken,
                                            const PBYTEArray & rawPDU)
{
  //verify the token is of correct type
  if (cryptoToken.GetTag() != H225_CryptoH323Token::e_nestedcryptoToken) {
    PTRACE(4, "H235\tNo nested crypto token!");
    return e_Absent;
  }
  
  const H235_CryptoToken & crNested = cryptoToken;
  if (crNested.GetTag() != H235_CryptoToken::e_cryptoHashedToken) {
    PTRACE(4, "H235\tNo crypto hash token!");
    return e_Absent;
  }
  
  const H235_CryptoToken_cryptoHashedToken & crHashed = crNested;
  
  //verify the crypto OIDs
  
  // "A" indicates that the whole messages is used for authentication.
  if (!CheckOID(crHashed.m_tokenOID, OID_A)) {
    PTRACE(2, "H235RAS\tH235AuthProcedure1 requires all fields are hashed, got OID " << crHashed.m_tokenOID);
    return e_Absent;
  }
  
  // "T" indicates that the hashed token of the CryptoToken is used for authentication.
  if (!CheckOID(crHashed.m_hashedVals.m_tokenOID, OID_T)) {
    PTRACE(2, "H235RAS\tH235AuthProcedure1 requires ClearToken, got OID " << crHashed.m_hashedVals.m_tokenOID);
    return e_Absent;
  }
  
  // "U" indicates that the HMAC-SHA1-96 alorigthm is used.
  if (!CheckOID(crHashed.m_token.m_algorithmOID, OID_U)) {
    PTRACE(2, "H235RAS\tH235AuthProcedure1 requires HMAC-SHA1-96, got OID " << crHashed.m_token.m_algorithmOID);
    return e_Absent;
  }
  
  //first verify the timestamp
  PTime now;
  int deltaTime = (int)now.GetTimeInSeconds() - crHashed.m_hashedVals.m_timeStamp;
  if (PABS(deltaTime) > timestampGracePeriod) {
    PTRACE(1, "H235RAS\tInvalid timestamp ABS(" << now.GetTimeInSeconds() << '-' 
           << (int)crHashed.m_hashedVals.m_timeStamp << ") > " << timestampGracePeriod);
    //the time has elapsed
    return e_InvalidTime;
  }
  
  //verify the randomnumber
  if (lastTimestamp == crHashed.m_hashedVals.m_timeStamp &&
      lastRandomSequenceNumber == crHashed.m_hashedVals.m_random) {
    //a message with this timespamp and the same random number was already verified
    PTRACE(2, "H235RAS\tConsecutive messages with the same random and timestamp");
    return e_ReplyAttack;
  }
  
  // save the values for the next call
  lastRandomSequenceNumber = crHashed.m_hashedVals.m_random;
  lastTimestamp = crHashed.m_hashedVals.m_timeStamp;
  
  //verify the username
  if (!localId.IsEmpty() && crHashed.m_tokenOID[OID_VERSION_OFFSET] > 1) {
    if (!crHashed.m_hashedVals.HasOptionalField(H235_ClearToken::e_generalID)) {
      PTRACE(1, "H235RAS\tH235AuthProcedure1 requires general ID.");
      return e_Error;
    }
  
    if (crHashed.m_hashedVals.m_generalID.GetValue() != localId) {
      PTRACE(1, "H235RAS\tGeneral ID is \"" << crHashed.m_hashedVals.m_generalID.GetValue()
             << "\", should be \"" << localId << '"');
      return e_Error;
    }
  }

  if (!remoteId.IsEmpty()) {
    if (!crHashed.m_hashedVals.HasOptionalField(H235_ClearToken::e_sendersID)) {
      PTRACE(1, "H235RAS\tH235AuthProcedure1 requires senders ID.");
      return e_Error;
    }
  
    if (crHashed.m_hashedVals.m_sendersID.GetValue() != remoteId) {
      PTRACE(1, "H235RAS\tSenders ID is \"" << crHashed.m_hashedVals.m_sendersID.GetValue()
             << "\", should be \"" << remoteId << '"');
      return e_Error;
    }
  }

  
  /****
  * step 1
  * extract the variable hash and save it
  *
  */
  BYTE RV[HASH_SIZE];
  
  if (crHashed.m_token.m_hash.GetSize() != HASH_SIZE*8) {
    PTRACE(1, "H235RAS\tH235AuthProcedure1 requires a hash!");
    return e_Error;
  }
  
  const unsigned char *data = crHashed.m_token.m_hash.GetDataPointer();
  memcpy(RV, data, HASH_SIZE);
  
  unsigned char secretkey[20];
  PSHA1Context::Process(password, secretkey);
  
  /****
  * step 4
  * lookup the variable int the orginal ASN1 packet
  * and set it to 0.
  */
  PINDEX foundat = 0;
  bool found = false;
  
  const BYTE * asnPtr = rawPDU;
  PINDEX asnLen = rawPDU.GetSize();
  while (foundat < asnLen - HASH_SIZE) {
    for (PINDEX i = foundat; i <= asnLen - HASH_SIZE; i++) {
      if (memcmp(asnPtr+i, data, HASH_SIZE) == 0) { // i'v found it !
        foundat = i;
        found = true;
        break;
      }
    }
    
    if (!found) {
      if (foundat != 0)
        break;

      PTRACE(1, "H235RAS\tH235AuthProcedure1 could not locate embedded hash!");
      return e_Error;
    }
    
    found = false;
    
    memset((BYTE *)asnPtr+foundat, 0, HASH_SIZE);
    
    /****
    * step 5
    * generate a HMAC-SHA1 key over the hole packet
    *
    */
    
    char key[HASH_SIZE];
    hmac_sha(secretkey, 20, asnPtr, asnLen, key, HASH_SIZE);
    
    
    /****
    * step 6
    * compare the two keys
    *
    */
    if(memcmp(key, RV, HASH_SIZE) == 0) // Keys are the same !! Ok
      return e_OK;

    // Put it back and look for another
    memcpy((BYTE *)asnPtr+foundat, data, HASH_SIZE);
    foundat++;
  }

  PTRACE(2, "H235RAS\tH235AuthProcedure1 hash does not match.");
  return e_BadPassword;
}


PBoolean H235AuthProcedure1::IsCapability(const H235_AuthenticationMechanism & mechansim,
                                      const PASN_ObjectId & algorithmOID)
{
  return mechansim.GetTag() == H235_AuthenticationMechanism::e_pwdHash &&
         algorithmOID.AsString() == OID_U;
}


PBoolean H235AuthProcedure1::SetCapability(H225_ArrayOf_AuthenticationMechanism & mechanisms,
                                      H225_ArrayOf_PASN_ObjectId & algorithmOIDs)
{
  return AddCapabilityIfNeeded(H235_AuthenticationMechanism::e_pwdHash, OID_U, mechanisms, algorithmOIDs) != P_MAX_INDEX;
}


PBoolean H235AuthProcedure1::UseGkAndEpIdentifiers() const
{
  return true;
}


///////////////////////////////////////////////////////////////////////////////

static const char Name_DES_ECB[] = "PWD-DES-ECB";
static const char OID_DES_ECB[] = "1.3.14.3.2.6";

PFACTORY_CREATE(PFactory<H235Authenticator>, H235AuthPwd_DES_ECB, Name_DES_ECB, false);

H235AuthPwd_DES_ECB::H235AuthPwd_DES_ECB()
{
  usage = AnyApplication;
}


PObject * H235AuthPwd_DES_ECB::Clone() const
{
  return new H235AuthPwd_DES_ECB(*this);
}


const char * H235AuthPwd_DES_ECB::GetName() const
{
  return Name_DES_ECB;
}


bool H235AuthPwd_DES_ECB::EncryptToken(PBYTEArray & encryptedToken)
{
  PSSLCipherContext cipher(true);
  cipher.SetPadding(PSSLCipherContext::PadCipherStealing);

  if (!cipher.SetAlgorithm(OID_DES_ECB)) {
    PTRACE(2, "H323\tCould not set SSL cipher algorithm for DES-ECB");
    return false;
  }

  PBYTEArray key(cipher.GetKeyLength());

  /* Build key from password according to H.235.0/8.2.1
     At least we would, except, Avaya do not follow those rules quite correctly.
     They seem to think the OpenSSL implementation has the 56 bit key in the
     first 7 bytes, when it is actually the low 7 bits of all 8 bytes. Due to
     this bug, they only really have 48 bit encryption.
   */
  PINDEX keySize = key.GetSize()-1;
  memcpy(key.GetPointer(), password.GetPointer(), std::min(keySize, password.GetLength()));
  for (PINDEX i = keySize; i < password.GetLength(); ++i)
    key[i%keySize] ^= password[i];

#if 0
  // But some things imply LSB of OpenSSL DES key is parity bit ...
  for (PINDEX i = 0; i < key.GetSize(); ++i) {
    static const struct ParityTable {
      BYTE table[128];
      ParityTable()
      {
        for (PINDEX i = 0; i < 128; ++i)
          table[i] = (BYTE)((i&1)^1);
      }
    } parity;

    unsigned value = key[i] & 0x7f;
    key[i] = (BYTE)((value << 1)|parity.table[value]);
  }
#endif

  if (!cipher.SetKey(key))
    return false;

  if (cipher.Process(m_encodedToken, encryptedToken))
    return true;

  PTRACE(2, "H323\tNot adding H.235 encryption key as encryption failed.");
  return false;
}


H235_ClearToken * H235AuthPwd_DES_ECB::CreateClearToken(unsigned rasPDU)
{
  if (rasPDU != H225_RasMessage::e_gatekeeperConfirm)
    return NULL;

  H235_PwdCertToken * clearToken =  new H235_PwdCertToken;
  clearToken->m_tokenOID = "0.0";

  clearToken->m_generalID = localId;

  clearToken->IncludeOptionalField(H235_ClearToken::e_timeStamp);
  clearToken->m_timeStamp.SetValue((unsigned)PTime().GetTimeInSeconds());

  clearToken->IncludeOptionalField(H235_ClearToken::e_random);
  clearToken->m_random = ++sentRandomSequenceNumber;

#ifdef AS_PER_SPECIFICATION_BUT_AVAYA_DOES_NOT
  clearToken->IncludeOptionalField(H235_ClearToken::e_challenge);
  PRandom::Octets(clearToken->m_challenge.GetWritableValue(), 16);
#endif

  PPER_Stream stream;
  clearToken->Encode(stream);
  stream.CompleteEncoding();
  m_encodedToken = stream;

  PTRACE(4, "H235RAS\tH235AuthPwd_DES_ECB: sending token " << m_encodedToken.GetSize() << " bytes.");
  return clearToken;
}


H225_CryptoH323Token * H235AuthPwd_DES_ECB::CreateCryptoToken(bool digits, unsigned rasPDU)
{
  if (!IsEnabled() || digits || rasPDU != H225_RasMessage::e_registrationRequest || m_encodedToken.IsEmpty())
    return NULL;

  // Create the H.225 crypto token using the encrypted clear token we got earlier
  H225_CryptoH323Token * cryptoToken = new H225_CryptoH323Token;
  cryptoToken->SetTag(H225_CryptoH323Token::e_cryptoEPPwdEncr);
  H235_ENCRYPTED<H235_EncodedPwdCertToken> & pwd = *cryptoToken;
  pwd.m_algorithmOID = OID_DES_ECB;
  if (EncryptToken(pwd.m_encryptedData.GetWritableValue()))
    return cryptoToken;

  delete cryptoToken;
  return NULL;
}


H235Authenticator::ValidationResult H235AuthPwd_DES_ECB::ValidateClearToken(const H235_ClearToken & clearToken)
{
  /* This really should check that timestamp is withing some range of the
     current wall clock time. But Avaya don't do that, so we don't */
  if (clearToken.m_timeStamp.GetValue() == 0) {
    PTRACE(2, "H235RAS\tH235AuthPwd_DES_ECB: zero timestamp");
    return e_InvalidTime;
  }

  PPER_Stream stream;
  clearToken.Encode(stream);
  stream.CompleteEncoding();
  m_encodedToken = stream;

  PTRACE(4, "H235RAS\tH235AuthPwd_DES_ECB: received token " << m_encodedToken.GetSize() << " bytes.");
  return m_encodedToken.IsEmpty() ? e_Error : e_OK;
}


H235Authenticator::ValidationResult H235AuthPwd_DES_ECB::ValidateCryptoToken(const H225_CryptoH323Token & cryptoToken, const PBYTEArray &)
{
  if (!IsEnabled())
    return e_Disabled;

  // verify the token is of correct type
  if (cryptoToken.GetTag() != H225_CryptoH323Token::e_cryptoEPPwdEncr)
    return e_Absent;

  PBYTEArray localEncryptedData;
  if (!EncryptToken(localEncryptedData))
    return e_Error;

  const PBYTEArray & remoteEncryptedData = ((const H235_ENCRYPTED<H235_EncodedPwdCertToken> &)cryptoToken).m_encryptedData;
  if (remoteEncryptedData == localEncryptedData)
    return e_OK;

  PTRACE(4, "H235RAS\tH235AuthPwd_DES_ECB::ValidateCryptoToken failed:\n"
          << hex << setfill('0') << setw(2) <<
            "   clear\n" << m_encodedToken << "\n"
            "   remote\n" << remoteEncryptedData << "\n"
            "   local\n" << localEncryptedData << dec);
  return e_BadPassword;
}


PBoolean H235AuthPwd_DES_ECB::IsCapability(const H235_AuthenticationMechanism & mechanism, const PASN_ObjectId & algorithmOID)
{
  return mechanism.GetTag() == H235_AuthenticationMechanism::e_pwdSymEnc && algorithmOID.AsString() == OID_DES_ECB;
}


PBoolean H235AuthPwd_DES_ECB::SetCapability(H225_ArrayOf_AuthenticationMechanism & mechanisms, H225_ArrayOf_PASN_ObjectId & algorithmOIDs)
{
  return AddCapabilityIfNeeded(H235_AuthenticationMechanism::e_pwdSymEnc, OID_DES_ECB, mechanisms, algorithmOIDs) != P_MAX_INDEX;
}


PBoolean H235AuthPwd_DES_ECB::IsSecuredPDU(unsigned rasPDU, PBoolean received) const
{
  switch (rasPDU) {
    case H225_RasMessage::e_gatekeeperConfirm :
      if (received)
        return false;

      if (localId.IsEmpty()) {
        PTRACE(4, "H235RAS\tH235AuthPwd_DES_ECB cannot add challenge to GCF without localId");
        return false;
      }

      PTRACE(4, "H235RAS\tH235AuthPwd_DES_ECB adding challenge to GCF for \"" << localId << '"');
      return true;

    case  H225_RasMessage::e_registrationRequest :
      if (m_encodedToken.IsEmpty()) {
        PTRACE(4, "H235RAS\tH235AuthPwd_DES_ECB cannot secure RRQ without challenge");
        return false;
      }

      if (password.IsEmpty()) {
        PTRACE(4, "H235RAS\tH235AuthPwd_DES_ECB cannot secure RRQ without password");
        return false;
      }

      PTRACE(4, "H235RAS\tH235AuthPwd_DES_ECB securing RRQ");
      return true;
  }

  return false;
}


#endif // OPAL_H323

#endif // #if OPAL_PTLIB_SSL


/////////////////////////////////////////////////////////////////////////////
