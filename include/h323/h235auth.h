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
 *
 * $Log: h235auth.h,v $
 * Revision 1.2002  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 1.1  2001/08/10 11:03:49  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 */

#ifndef __H235AUTH_H
#define __H235AUTH_H


class H225_CryptoH323Token;
class H225_ArrayOf_CryptoH323Token;
class H225_ArrayOf_AuthenticationMechanism;
class H225_ArrayOf_PASN_ObjectId;


/** This abtract class embodies an H.235 authentication mechanism.
*/
class H235Authenticator : public PObject
{
    PCLASSINFO(H235Authenticator, PObject);
  public:
    H235Authenticator();

    virtual BOOL Prepare(
      H225_ArrayOf_CryptoH323Token & cryptoTokens
    );

    virtual BOOL PrepareToken(
      H225_CryptoH323Token & cryptoTokens
    ) = 0;

    virtual BOOL Finalise(
      PBYTEArray & rawPDU
    ) = 0;

    enum State {
      e_OK = 0,    /// Security parameters and Msg are ok, no security attacks
      e_Absent,    /// Security parameters are expected but absent
      e_Error,     /// Security parameters are present but incorrect
      e_Attacked,  /// Security parameters indicate an attack was made
      e_Disabled   /// Security is disabled by local system
    };

    virtual State Verify(
      const H225_ArrayOf_CryptoH323Token & cryptoTokens,
      const PBYTEArray & rawPDU
    );

    virtual State VerifyToken(
      const H225_CryptoH323Token & cryptoToken,
      const PBYTEArray & rawPDU
    ) = 0;

    virtual BOOL SetCapability(
      H225_ArrayOf_AuthenticationMechanism & authenticationCapabilities,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    );

    BOOL IsActive() const { return enabled && !password; }

    void Enable(
      BOOL enab = TRUE
    ) { enabled = enab; }
    void Disable() { enabled = FALSE; }

    const PString & GetRemoteId() const { return remoteId; }
    void SetRemoteId(const PString & id) { remoteId = id; }

    const PString & GetLocalId() const { return localId; }
    void SetLocalId(const PString & id) { localId = id; }

    const PString & GetPassword() const { return password; }
    void SetPassword(const PString & pw) { password = pw; }

  protected:
    BOOL     enabled;

    PString  remoteId;      // ID of remote entity
    PString  localId;       // ID of local entity
    PString  password;      // shared secret

    unsigned sentRandomSequenceNumber;
};


PLIST(H235Authenticators, H235Authenticator);


/** This class embodies a simple MD5 based authentication.
    The users password is concatenated with the 4 byte timestamp and 4 byte
    random fields and an MD5 generated and sent/verified
*/
class H235AuthSimpleMD5 : public H235Authenticator
{
    PCLASSINFO(H235AuthSimpleMD5, H235Authenticator);
  public:
    H235AuthSimpleMD5();

    virtual BOOL PrepareToken(
      H225_CryptoH323Token & cryptoTokens
    );

    virtual BOOL Finalise(
      PBYTEArray & rawPDU
    );

    virtual State VerifyToken(
      const H225_CryptoH323Token & cryptoToken,
      const PBYTEArray & rawPDU
    );

    virtual BOOL SetCapability(
      H225_ArrayOf_AuthenticationMechanism & authenticationCapabilities,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    );
};


#ifdef P_SSL

/** This class embodies the H.235 "base line Procedure 1" from Annex D.
*/
class H235AuthProcedure1 : public H235Authenticator
{
    PCLASSINFO(H235AuthProcedure1, H235Authenticator);
  public:
    H235AuthProcedure1();

    virtual BOOL PrepareToken(
      H225_CryptoH323Token & cryptoTokens
    );

    virtual BOOL Finalise(
      PBYTEArray & rawPDU
    );

    virtual State VerifyToken(
      const H225_CryptoH323Token & cryptoToken,
      const PBYTEArray & rawPDU
    );

  protected:
    unsigned   lastRandomSequenceNumber;
    unsigned   lastTimestamp;
};

#endif


#endif //__H235AUTH_H


/////////////////////////////////////////////////////////////////////////////
