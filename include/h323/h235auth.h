/*
 * h235auth.h
 *
 * H.235 authorisation PDU's
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
 */

#ifndef OPAL_H323_H235AUTH_H
#define OPAL_H323_H235AUTH_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_H323

#include <ptlib/pfactory.h>


class H323TransactionPDU;
class H225_CryptoH323Token;
class H225_ArrayOf_AuthenticationMechanism;
class H225_ArrayOf_PASN_ObjectId;
class H235_ClearToken;
class H235_AuthenticationMechanism;
class PASN_ObjectId;
class PASN_Sequence;
class PASN_Array;


/** This abtract class embodies an H.235 authentication mechanism.
    NOTE: descendants must have a Clone() function for correct operation.
*/
class H235Authenticator : public PObject
{
    PCLASSINFO(H235Authenticator, PObject);
  public:
    H235Authenticator();

    virtual void PrintOn(
      ostream & strm
    ) const;

    virtual const char * GetName() const = 0;

    virtual PBoolean PrepareTokens(
      PASN_Array & clearTokens,
      PASN_Array & cryptoTokens,
      unsigned rasPDU
    );

    virtual H235_ClearToken * CreateClearToken(unsigned rasPDU);
    virtual H235_ClearToken * CreateClearToken();
    virtual H225_CryptoH323Token * CreateCryptoToken(bool digits, unsigned rasPDU);
    virtual H225_CryptoH323Token * CreateCryptoToken(bool digits);

    virtual PBoolean Finalise(
      PBYTEArray & rawPDU
    );

    enum ValidationResult {
      e_OK = 0,     /// Security parameters and Msg are ok, no security attacks
      e_Absent,     /// Security parameters are expected but absent
      e_Error,      /// Security parameters are present but incorrect
      e_InvalidTime,/// Security parameters indicate peer has bad real time clock
      e_BadPassword,/// Security parameters indicate bad password in token
      e_ReplyAttack,/// Security parameters indicate an attack was made
      e_Disabled    /// Security is disabled by local system
    };

    virtual ValidationResult ValidateTokens(
      const PASN_Array & clearTokens,
      const PASN_Array & cryptoTokens,
      const PBYTEArray & rawPDU
    );

    virtual ValidationResult ValidateClearToken(
      const H235_ClearToken & clearToken
    );

    virtual ValidationResult ValidateCryptoToken(
      const H225_CryptoH323Token & cryptoToken,
      const PBYTEArray & rawPDU
    );

    virtual PBoolean IsCapability(
      const H235_AuthenticationMechanism & mechansim,
      const PASN_ObjectId & algorithmOID
    ) = 0;

    virtual PBoolean SetCapability(
      H225_ArrayOf_AuthenticationMechanism & mechansims,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    ) = 0;

    virtual PBoolean UseGkAndEpIdentifiers() const;

    virtual PBoolean IsSecuredPDU(
      unsigned rasPDU,
      PBoolean received
    ) const;

    void Enable(
      PBoolean enab = true
    ) { m_enabled = enab; }
    void Disable() { m_enabled = false; }
    bool IsEnabled() const { return m_enabled; }

    const PString & GetRemoteId() const { return remoteId; }
    void SetRemoteId(const PString & id) { remoteId = id; }

    const PString & GetLocalId() const { return localId; }
    void SetLocalId(const PString & id) { localId = id; }

    const PString & GetPassword() const { return password; }
    void SetPassword(const PString & pw) { password = pw; }

    enum Application {
        GKAdmission,		    ///< To Be Used for GK Admission
        EPAuthentication,	  ///< To Be Used for EP Authentication
        LRQOnly,            ///< To Be Used for Location Request Authentication
        AnyApplication,		  ///< To Be Used for Any Application
    };

    Application GetApplication() { return usage; }  ///< Get Authentication Application


  protected:
    PINDEX AddCapabilityIfNeeded(
      unsigned mechanism,
      const PString & oid,
      H225_ArrayOf_AuthenticationMechanism & mechansims,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    );

    bool     m_enabled;

    PString  remoteId;      // ID of remote entity
    PString  localId;       // ID of local entity
    PString  password;      // shared secret

    unsigned sentRandomSequenceNumber;
    unsigned lastRandomSequenceNumber;
    unsigned lastTimestamp;
    int      timestampGracePeriod;

    Application usage;	       ///< Authenticator's Application 

    PDECLARE_MUTEX(mutex);

  private:
    P_REMOVE_VIRTUAL(H225_CryptoH323Token *,CreateCryptoToken(),NULL);
    P_REMOVE_VIRTUAL(PBoolean,PrepareTokens(PASN_Array &,PASN_Array &),false);
};


PDECLARE_LIST(H235Authenticators, H235Authenticator)
  protected:
    void InternalPreparePDU(
      H323TransactionPDU & pdu,
      PASN_Array & clearTokens,
      unsigned clearOptionalField,
      PASN_Array & cryptoTokens,
      unsigned cryptoOptionalField
    );
    H235Authenticator::ValidationResult InternalValidatePDU(
      const H323TransactionPDU & pdu,
      const PASN_Array & clearTokens,
      unsigned clearOptionalField,
      const PASN_Array & cryptoTokens,
      unsigned cryptoOptionalField,
      const PBYTEArray & rawPDU
    );
  public:
    template <class RAS> void PreparePDU(H323TransactionPDU & pdu, RAS & ras)
    {
      InternalPreparePDU(pdu, ras.m_tokens, RAS::e_tokens, ras.m_cryptoTokens, RAS::e_cryptoTokens);
    }
    template <class RAS> H235Authenticator::ValidationResult ValidatePDU(const H323TransactionPDU & pdu,const RAS & ras, const PBYTEArray & rawPDU)
    {
      return InternalValidatePDU(pdu, ras.m_tokens, RAS::e_tokens, ras.m_cryptoTokens, RAS::e_cryptoTokens, rawPDU);
    }
};




/** This class embodies a simple MD5 based authentication.
    The users password is concatenated with the 4 byte timestamp and 4 byte
    random fields and an MD5 generated and sent/verified
*/
class H235AuthSimpleMD5 : public H235Authenticator
{
    PCLASSINFO(H235AuthSimpleMD5, H235Authenticator);
  public:
    H235AuthSimpleMD5();

    PObject * Clone() const;

    virtual const char * GetName() const;

    virtual H225_CryptoH323Token * CreateCryptoToken(bool digits);

    virtual ValidationResult ValidateCryptoToken(
      const H225_CryptoH323Token & cryptoToken,
      const PBYTEArray & rawPDU
    );

    virtual PBoolean IsCapability(
      const H235_AuthenticationMechanism & mechansim,
      const PASN_ObjectId & algorithmOID
    );

    virtual PBoolean SetCapability(
      H225_ArrayOf_AuthenticationMechanism & mechansim,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    );

    virtual PBoolean IsSecuredPDU(
      unsigned rasPDU,
      PBoolean received
    ) const;
};

PFACTORY_LOAD(H235AuthSimpleMD5);


/** This class embodies a RADIUS compatible based authentication (aka Cisco
    Access Token or CAT).
    The users password is concatenated with the 4 byte timestamp and 1 byte
    random fields and an MD5 generated and sent/verified via the challenge
    field.
*/
class H235AuthCAT : public H235Authenticator
{
    PCLASSINFO(H235AuthCAT, H235Authenticator);
  public:
    H235AuthCAT();

    PObject * Clone() const;

    virtual const char * GetName() const;

    virtual H235_ClearToken * CreateClearToken();

    virtual ValidationResult ValidateClearToken(
      const H235_ClearToken & clearToken
    );

    virtual PBoolean IsCapability(
      const H235_AuthenticationMechanism & mechansim,
      const PASN_ObjectId & algorithmOID
    );

    virtual PBoolean SetCapability(
      H225_ArrayOf_AuthenticationMechanism & mechansim,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    );

    virtual PBoolean IsSecuredPDU(
      unsigned rasPDU,
      PBoolean received
    ) const;
};

PFACTORY_LOAD(H235AuthCAT);


#if OPAL_PTLIB_SSL

/** This class embodies a simple password encoded using DES-ECB encryption.
*/
class H235AuthPwd_DES_ECB : public H235Authenticator
{
    PCLASSINFO(H235AuthPwd_DES_ECB, H235Authenticator);
  public:
    H235AuthPwd_DES_ECB();

    PObject * Clone() const;

    virtual const char * GetName() const;

    virtual H235_ClearToken * CreateClearToken(unsigned rasPDU);
    virtual H225_CryptoH323Token * CreateCryptoToken(bool digits, unsigned rasPDU);

    virtual ValidationResult ValidateClearToken(
      const H235_ClearToken & clearToken
    );
    virtual ValidationResult ValidateCryptoToken(
      const H225_CryptoH323Token & cryptoToken,
      const PBYTEArray & rawPDU
    );

    virtual PBoolean SetCapability(
      H225_ArrayOf_AuthenticationMechanism & mechansim,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    );

    virtual PBoolean IsCapability(
      const H235_AuthenticationMechanism & mechansim,
      const PASN_ObjectId & algorithmOID
    );

    virtual PBoolean IsSecuredPDU(
      unsigned rasPDU,
      PBoolean received
    ) const;

  protected:
    bool EncryptToken(PBYTEArray & encryptedToken);

    PBYTEArray m_encodedToken;
};

PFACTORY_LOAD(H235AuthPwd_DES_ECB);


/** This class embodies the H.235 "base line Procedure 1" from Annex D.
*/
class H235AuthProcedure1 : public H235Authenticator
{
    PCLASSINFO(H235AuthProcedure1, H235Authenticator);
  public:
    H235AuthProcedure1();

    PObject * Clone() const;

    virtual const char * GetName() const;

    virtual H225_CryptoH323Token * CreateCryptoToken(bool digits);

    virtual PBoolean Finalise(
      PBYTEArray & rawPDU
    );

    virtual ValidationResult ValidateCryptoToken(
      const H225_CryptoH323Token & cryptoToken,
      const PBYTEArray & rawPDU
    );

    virtual PBoolean IsCapability(
      const H235_AuthenticationMechanism & mechansim,
      const PASN_ObjectId & algorithmOID
    );

    virtual PBoolean SetCapability(
      H225_ArrayOf_AuthenticationMechanism & mechansim,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    );

    virtual PBoolean UseGkAndEpIdentifiers() const;
};

PFACTORY_LOAD(H235AuthProcedure1);

#endif // OPAL_PTLIB_SSL

#endif // OPAL_H323

#endif //OPAL_H323_H235AUTH_H


/////////////////////////////////////////////////////////////////////////////
