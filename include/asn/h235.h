//
// h235.h
//
// Code automatically generated by asnparse.
//

#include <opal_config.h>

#if ! H323_DISABLE_H235

#ifndef __H235_H
#define __H235_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/asner.h>

//
// ChallengeString
//

class H235_ChallengeString : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ChallengeString, PASN_OctetString);
#endif
  public:
    H235_ChallengeString(unsigned tag = UniversalOctetString, TagClass tagClass = UniversalTagClass);

    H235_ChallengeString(const char * v);
    H235_ChallengeString(const PString & v);
    H235_ChallengeString(const PBYTEArray & v);

    H235_ChallengeString & operator=(const char * v);
    H235_ChallengeString & operator=(const PString & v);
    H235_ChallengeString & operator=(const PBYTEArray & v);
    PObject * Clone() const;
};


//
// TimeStamp
//

class H235_TimeStamp : public PASN_Integer
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_TimeStamp, PASN_Integer);
#endif
  public:
    H235_TimeStamp(unsigned tag = UniversalInteger, TagClass tagClass = UniversalTagClass);

    H235_TimeStamp & operator=(int v);
    H235_TimeStamp & operator=(unsigned v);
    PObject * Clone() const;
};


//
// RandomVal
//

class H235_RandomVal : public PASN_Integer
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_RandomVal, PASN_Integer);
#endif
  public:
    H235_RandomVal(unsigned tag = UniversalInteger, TagClass tagClass = UniversalTagClass);

    H235_RandomVal & operator=(int v);
    H235_RandomVal & operator=(unsigned v);
    PObject * Clone() const;
};


//
// Password
//

class H235_Password : public PASN_BMPString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_Password, PASN_BMPString);
#endif
  public:
    H235_Password(unsigned tag = UniversalBMPString, TagClass tagClass = UniversalTagClass);

    H235_Password & operator=(const char * v);
    H235_Password & operator=(const PString & v);
    H235_Password & operator=(const PWCharArray & v);
    H235_Password & operator=(const PASN_BMPString & v);
    PObject * Clone() const;
};


//
// Identifier
//

class H235_Identifier : public PASN_BMPString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_Identifier, PASN_BMPString);
#endif
  public:
    H235_Identifier(unsigned tag = UniversalBMPString, TagClass tagClass = UniversalTagClass);

    H235_Identifier & operator=(const char * v);
    H235_Identifier & operator=(const PString & v);
    H235_Identifier & operator=(const PWCharArray & v);
    H235_Identifier & operator=(const PASN_BMPString & v);
    PObject * Clone() const;
};


//
// KeyMaterial
//

class H235_KeyMaterial : public PASN_BitString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_KeyMaterial, PASN_BitString);
#endif
  public:
    H235_KeyMaterial(unsigned tag = UniversalBitString, TagClass tagClass = UniversalTagClass);

    PObject * Clone() const;
};


//
// KeyMaterialExt
//

class H235_KeyMaterialExt : public PASN_BitString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_KeyMaterialExt, PASN_BitString);
#endif
  public:
    H235_KeyMaterialExt(unsigned tag = UniversalBitString, TagClass tagClass = UniversalTagClass);

    PObject * Clone() const;
};


//
// NonStandardParameter
//

class H235_NonStandardParameter : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_NonStandardParameter, PASN_Sequence);
#endif
  public:
    H235_NonStandardParameter(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_ObjectId m_nonStandardIdentifier;
    PASN_OctetString m_data;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// DHset
//

class H235_DHset : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_DHset, PASN_Sequence);
#endif
  public:
    H235_DHset(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_BitString m_halfkey;
    PASN_BitString m_modSize;
    PASN_BitString m_generator;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// DHsetExt
//

class H235_DHsetExt : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_DHsetExt, PASN_Sequence);
#endif
  public:
    H235_DHsetExt(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_modSize,
      e_generator
    };

    PASN_BitString m_halfkey;
    PASN_BitString m_modSize;
    PASN_BitString m_generator;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ECpoint
//

class H235_ECpoint : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ECpoint, PASN_Sequence);
#endif
  public:
    H235_ECpoint(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_x,
      e_y
    };

    PASN_BitString m_x;
    PASN_BitString m_y;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ECKASDH
//

class H235_ECKASDH_eckasdhp;
class H235_ECKASDH_eckasdh2;

class H235_ECKASDH : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ECKASDH, PASN_Choice);
#endif
  public:
    H235_ECKASDH(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_eckasdhp,
      e_eckasdh2
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_ECKASDH_eckasdhp &() const;
#else
    operator H235_ECKASDH_eckasdhp &();
    operator const H235_ECKASDH_eckasdhp &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_ECKASDH_eckasdh2 &() const;
#else
    operator H235_ECKASDH_eckasdh2 &();
    operator const H235_ECKASDH_eckasdh2 &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// ECGDSASignature
//

class H235_ECGDSASignature : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ECGDSASignature, PASN_Sequence);
#endif
  public:
    H235_ECGDSASignature(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_BitString m_r;
    PASN_BitString m_s;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// TypedCertificate
//

class H235_TypedCertificate : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_TypedCertificate, PASN_Sequence);
#endif
  public:
    H235_TypedCertificate(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_ObjectId m_type;
    PASN_OctetString m_certificate;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// AuthenticationBES
//

class H235_AuthenticationBES : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_AuthenticationBES, PASN_Choice);
#endif
  public:
    H235_AuthenticationBES(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_default,
      e_radius
    };

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// AuthenticationMechanism
//

class H235_NonStandardParameter;
class H235_AuthenticationBES;

class H235_AuthenticationMechanism : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_AuthenticationMechanism, PASN_Choice);
#endif
  public:
    H235_AuthenticationMechanism(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_dhExch,
      e_pwdSymEnc,
      e_pwdHash,
      e_certSign,
      e_ipsec,
      e_tls,
      e_nonStandard,
      e_authenticationBES,
      e_keyExch
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_NonStandardParameter &() const;
#else
    operator H235_NonStandardParameter &();
    operator const H235_NonStandardParameter &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_AuthenticationBES &() const;
#else
    operator H235_AuthenticationBES &();
    operator const H235_AuthenticationBES &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// Element
//

class H235_Element : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_Element, PASN_Choice);
#endif
  public:
    H235_Element(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_octets,
      e_integer,
      e_bits,
      e_name,
      e_flag
    };

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// IV8
//

class H235_IV8 : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_IV8, PASN_OctetString);
#endif
  public:
    H235_IV8(unsigned tag = UniversalOctetString, TagClass tagClass = UniversalTagClass);

    H235_IV8(const char * v);
    H235_IV8(const PString & v);
    H235_IV8(const PBYTEArray & v);

    H235_IV8 & operator=(const char * v);
    H235_IV8 & operator=(const PString & v);
    H235_IV8 & operator=(const PBYTEArray & v);
    PObject * Clone() const;
};


//
// IV16
//

class H235_IV16 : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_IV16, PASN_OctetString);
#endif
  public:
    H235_IV16(unsigned tag = UniversalOctetString, TagClass tagClass = UniversalTagClass);

    H235_IV16(const char * v);
    H235_IV16(const PString & v);
    H235_IV16(const PBYTEArray & v);

    H235_IV16 & operator=(const char * v);
    H235_IV16 & operator=(const PString & v);
    H235_IV16 & operator=(const PBYTEArray & v);
    PObject * Clone() const;
};


//
// Params
//

class H235_Params : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_Params, PASN_Sequence);
#endif
  public:
    H235_Params(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_ranInt,
      e_iv8,
      e_iv16,
      e_iv,
      e_clearSalt
    };

    PASN_Integer m_ranInt;
    H235_IV8 m_iv8;
    H235_IV16 m_iv16;
    PASN_OctetString m_iv;
    PASN_OctetString m_clearSalt;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ReturnSig
//

class H235_ReturnSig : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ReturnSig, PASN_Sequence);
#endif
  public:
    H235_ReturnSig(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_requestRandom,
      e_certificate
    };

    H235_Identifier m_generalId;
    H235_RandomVal m_responseRandom;
    H235_RandomVal m_requestRandom;
    H235_TypedCertificate m_certificate;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// EncodedReturnSig
//

class H235_EncodedReturnSig : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_EncodedReturnSig, PASN_OctetString);
#endif
  public:
    H235_EncodedReturnSig(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    PBoolean DecodeSubType(H235_ReturnSig & obj) { return PASN_OctetString::DecodeSubType(obj); }
    void EncodeSubType(const H235_ReturnSig & obj) { PASN_OctetString::EncodeSubType(obj); } 

    PObject * Clone() const;
};


//
// KeySyncMaterial
//

class H235_KeySyncMaterial : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_KeySyncMaterial, PASN_Sequence);
#endif
  public:
    H235_KeySyncMaterial(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    H235_Identifier m_generalID;
    H235_KeyMaterial m_keyMaterial;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// EncodedKeySyncMaterial
//

class H235_EncodedKeySyncMaterial : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_EncodedKeySyncMaterial, PASN_OctetString);
#endif
  public:
    H235_EncodedKeySyncMaterial(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    PBoolean DecodeSubType(H235_KeySyncMaterial & obj) const { return PASN_OctetString::DecodeSubType(obj); }
    void EncodeSubType(const H235_KeySyncMaterial & obj) { PASN_OctetString::EncodeSubType(obj); } 

    PObject * Clone() const;
};


//
// V3KeySyncMaterial
//

class H235_V3KeySyncMaterial : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_V3KeySyncMaterial, PASN_Sequence);
#endif
  public:
    H235_V3KeySyncMaterial(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_generalID,
      e_algorithmOID,
      e_encryptedSessionKey,
      e_encryptedSaltingKey,
      e_clearSaltingKey,
      e_paramSsalt,
      e_keyDerivationOID,
      e_genericKeyMaterial
    };

    H235_Identifier m_generalID;
    PASN_ObjectId m_algorithmOID;
    H235_Params m_paramS;
    PASN_OctetString m_encryptedSessionKey;
    PASN_OctetString m_encryptedSaltingKey;
    PASN_OctetString m_clearSaltingKey;
    H235_Params m_paramSsalt;
    PASN_ObjectId m_keyDerivationOID;
    PASN_OctetString m_genericKeyMaterial;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ECKASDH_eckasdhp
//

class H235_ECKASDH_eckasdhp : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ECKASDH_eckasdhp, PASN_Sequence);
#endif
  public:
    H235_ECKASDH_eckasdhp(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    H235_ECpoint m_public_key;
    PASN_BitString m_modulus;
    H235_ECpoint m_base;
    PASN_BitString m_weierstrassA;
    PASN_BitString m_weierstrassB;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ECKASDH_eckasdh2
//

class H235_ECKASDH_eckasdh2 : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ECKASDH_eckasdh2, PASN_Sequence);
#endif
  public:
    H235_ECKASDH_eckasdh2(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    H235_ECpoint m_public_key;
    PASN_BitString m_fieldSize;
    H235_ECpoint m_base;
    PASN_BitString m_weierstrassA;
    PASN_BitString m_weierstrassB;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ArrayOf_ProfileElement
//

class H235_ProfileElement;

class H235_ArrayOf_ProfileElement : public PASN_Array
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ArrayOf_ProfileElement, PASN_Array);
#endif
  public:
    H235_ArrayOf_ProfileElement(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_Object * CreateObject() const;
    H235_ProfileElement & operator[](PINDEX i) const;
    PObject * Clone() const;
};


//
// ProfileElement
//

class H235_ProfileElement : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ProfileElement, PASN_Sequence);
#endif
  public:
    H235_ProfileElement(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_paramS,
      e_element
    };

    PASN_Integer m_elementID;
    H235_Params m_paramS;
    H235_Element m_element;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// SIGNED
//

template <class ToBeSigned>
class H235_SIGNED : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_SIGNED, PASN_Sequence);
#endif
  public:
    H235_SIGNED(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    ToBeSigned m_toBeSigned;
    PASN_ObjectId m_algorithmOID;
    H235_Params m_paramS;
    PASN_BitString m_signature;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ENCRYPTED
//

template <class ToBeEncrypted>
class H235_ENCRYPTED : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ENCRYPTED, PASN_Sequence);
#endif
  public:
    H235_ENCRYPTED(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_ObjectId m_algorithmOID;
    H235_Params m_paramS;
    PASN_OctetString m_encryptedData;
    ToBeEncrypted m_clearData;  // Note this is not actually encoded

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// HASHED
//

template <class ToBeHashed>
class H235_HASHED : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_HASHED, PASN_Sequence);
#endif
  public:
    H235_HASHED(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_ObjectId m_algorithmOID;
    H235_Params m_paramS;
    PASN_BitString m_hash;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// KeySignedMaterial
//

class H235_KeySignedMaterial : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_KeySignedMaterial, PASN_Sequence);
#endif
  public:
    H235_KeySignedMaterial(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_srandom,
      e_timeStamp
    };

    H235_Identifier m_generalId;
    H235_RandomVal m_mrandom;
    H235_RandomVal m_srandom;
    H235_TimeStamp m_timeStamp;
    H235_ENCRYPTED<H235_EncodedKeySyncMaterial> m_encrptval;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// EncodedKeySignedMaterial
//

class H235_EncodedKeySignedMaterial : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_EncodedKeySignedMaterial, PASN_OctetString);
#endif
  public:
    H235_EncodedKeySignedMaterial(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    PBoolean DecodeSubType(H235_KeySignedMaterial & obj) { return PASN_OctetString::DecodeSubType(obj); }
    void EncodeSubType(const H235_KeySignedMaterial & obj) { PASN_OctetString::EncodeSubType(obj); } 

    PObject * Clone() const;
};


//
// H235CertificateSignature
//

class H235_H235CertificateSignature : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_H235CertificateSignature, PASN_Sequence);
#endif
  public:
    H235_H235CertificateSignature(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_requesterRandom
    };

    H235_TypedCertificate m_certificate;
    H235_RandomVal m_responseRandom;
    H235_RandomVal m_requesterRandom;
    H235_SIGNED<H235_EncodedReturnSig> m_signature;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// H235Key
//

class H235_KeyMaterial;
class H235_V3KeySyncMaterial;
class H235_KeyMaterialExt;

class H235_H235Key : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_H235Key, PASN_Choice);
#endif
  public:
    H235_H235Key(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_secureChannel,
      e_sharedSecret,
      e_certProtectedKey,
      e_secureSharedSecret,
      e_secureChannelExt
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_KeyMaterial &() const;
#else
    operator H235_KeyMaterial &();
    operator const H235_KeyMaterial &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_ENCRYPTED<H235_EncodedKeySyncMaterial> &() const;
#else
    operator H235_ENCRYPTED<H235_EncodedKeySyncMaterial> &();
    operator const H235_ENCRYPTED<H235_EncodedKeySyncMaterial> &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_SIGNED<H235_EncodedKeySignedMaterial> &() const;
#else
    operator H235_SIGNED<H235_EncodedKeySignedMaterial> &();
    operator const H235_SIGNED<H235_EncodedKeySignedMaterial> &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_V3KeySyncMaterial &() const;
#else
    operator H235_V3KeySyncMaterial &();
    operator const H235_V3KeySyncMaterial &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_KeyMaterialExt &() const;
#else
    operator H235_KeyMaterialExt &();
    operator const H235_KeyMaterialExt &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// ClearToken
//

class H235_ClearToken : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_ClearToken, PASN_Sequence);
#endif
  public:
    H235_ClearToken(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_timeStamp,
      e_password,
      e_dhkey,
      e_challenge,
      e_random,
      e_certificate,
      e_generalID,
      e_nonStandard,
      e_eckasdhkey,
      e_sendersID,
      e_h235Key,
      e_profileInfo,
      e_dhkeyext
    };

    PASN_ObjectId m_tokenOID;
    H235_TimeStamp m_timeStamp;
    H235_Password m_password;
    H235_DHset m_dhkey;
    H235_ChallengeString m_challenge;
    H235_RandomVal m_random;
    H235_TypedCertificate m_certificate;
    H235_Identifier m_generalID;
    H235_NonStandardParameter m_nonStandard;
    H235_ECKASDH m_eckasdhkey;
    H235_Identifier m_sendersID;
    H235_H235Key m_h235Key;
    H235_ArrayOf_ProfileElement m_profileInfo;
    H235_DHsetExt m_dhkeyext;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// EncodedGeneralToken
//

class H235_EncodedGeneralToken : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_EncodedGeneralToken, PASN_OctetString);
#endif
  public:
    H235_EncodedGeneralToken(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    PBoolean DecodeSubType(H235_ClearToken & obj) { return PASN_OctetString::DecodeSubType(obj); }
    void EncodeSubType(const H235_ClearToken & obj) { PASN_OctetString::EncodeSubType(obj); } 

    PObject * Clone() const;
};


//
// PwdCertToken
//

class H235_PwdCertToken : public H235_ClearToken
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_PwdCertToken, H235_ClearToken);
#endif
  public:
    H235_PwdCertToken(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PObject * Clone() const;
};


//
// EncodedPwdCertToken
//

class H235_EncodedPwdCertToken : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_EncodedPwdCertToken, PASN_OctetString);
#endif
  public:
    H235_EncodedPwdCertToken(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    PBoolean DecodeSubType(H235_PwdCertToken & obj) { return PASN_OctetString::DecodeSubType(obj); }
    void EncodeSubType(const H235_PwdCertToken & obj) { PASN_OctetString::EncodeSubType(obj); } 

    PObject * Clone() const;
};


//
// CryptoToken
//

class H235_CryptoToken_cryptoEncryptedToken;
class H235_CryptoToken_cryptoSignedToken;
class H235_CryptoToken_cryptoHashedToken;

class H235_CryptoToken : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_CryptoToken, PASN_Choice);
#endif
  public:
    H235_CryptoToken(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_cryptoEncryptedToken,
      e_cryptoSignedToken,
      e_cryptoHashedToken,
      e_cryptoPwdEncr
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_CryptoToken_cryptoEncryptedToken &() const;
#else
    operator H235_CryptoToken_cryptoEncryptedToken &();
    operator const H235_CryptoToken_cryptoEncryptedToken &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_CryptoToken_cryptoSignedToken &() const;
#else
    operator H235_CryptoToken_cryptoSignedToken &();
    operator const H235_CryptoToken_cryptoSignedToken &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_CryptoToken_cryptoHashedToken &() const;
#else
    operator H235_CryptoToken_cryptoHashedToken &();
    operator const H235_CryptoToken_cryptoHashedToken &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H235_ENCRYPTED<H235_EncodedPwdCertToken> &() const;
#else
    operator H235_ENCRYPTED<H235_EncodedPwdCertToken> &();
    operator const H235_ENCRYPTED<H235_EncodedPwdCertToken> &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// CryptoToken_cryptoEncryptedToken
//

class H235_CryptoToken_cryptoEncryptedToken : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_CryptoToken_cryptoEncryptedToken, PASN_Sequence);
#endif
  public:
    H235_CryptoToken_cryptoEncryptedToken(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_ObjectId m_tokenOID;
    H235_ENCRYPTED<H235_EncodedGeneralToken> m_token;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// CryptoToken_cryptoSignedToken
//

class H235_CryptoToken_cryptoSignedToken : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_CryptoToken_cryptoSignedToken, PASN_Sequence);
#endif
  public:
    H235_CryptoToken_cryptoSignedToken(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_ObjectId m_tokenOID;
    H235_SIGNED<H235_EncodedGeneralToken> m_token;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// CryptoToken_cryptoHashedToken
//

class H235_CryptoToken_cryptoHashedToken : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H235_CryptoToken_cryptoHashedToken, PASN_Sequence);
#endif
  public:
    H235_CryptoToken_cryptoHashedToken(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_ObjectId m_tokenOID;
    H235_ClearToken m_hashedVals;
    H235_HASHED<H235_EncodedGeneralToken> m_token;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


#endif // __H235_H

#endif // if ! H323_DISABLE_H235


// End of h235.h
