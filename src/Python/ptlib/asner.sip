/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

/////////////////////////////////////////////////////////////////////////////

/** Base class for ASN encoding/decoding.
*/
class PASN_Object : PObject /Abstract/
{
    
  public:
    /** Return a string giving the type of the object */
    virtual PString GetTypeAsString() const = 0;

    PINDEX GetObjectLength() const;
    virtual PINDEX GetDataLength() const = 0;
    virtual PBoolean IsPrimitive() const;

    virtual PBoolean Decode(PASN_Stream &) = 0;
    virtual void Encode(PASN_Stream &) const = 0;

    PBoolean IsExtendable() const;
    void SetExtendable(PBoolean ext = PTrue);

    enum TagClass {
      UniversalTagClass,
      ApplicationTagClass,
      ContextSpecificTagClass,
      PrivateTagClass,
      DefaultTagClass
    };
	
    TagClass GetTagClass() const;

    enum UniversalTags {
      InvalidUniversalTag,
      UniversalBoolean,
      UniversalInteger,
      UniversalBitString,
      UniversalOctetString,
      UniversalNull,
      UniversalObjectId,
      UniversalObjectDescriptor,
      UniversalExternalType,
      UniversalReal,
      UniversalEnumeration,
      UniversalEmbeddedPDV,
      UniversalSequence = 16,
      UniversalSet,
      UniversalNumericString,
      UniversalPrintableString,
      UniversalTeletexString,
      UniversalVideotexString,
      UniversalIA5String,
      UniversalUTCTime,
      UniversalGeneralisedTime,
      UniversalGeneralizedTime = UniversalGeneralisedTime,
      UniversalGraphicString,
      UniversalVisibleString,
      UniversalGeneralString,
      UniversalUniversalString,
      UniversalBMPString = 30
    };

    unsigned GetTag() const;
    virtual void SetTag(unsigned newTag, TagClass tagClass = DefaultTagClass);

    enum ConstraintType {
      Unconstrained,
      PartiallyConstrained,
      FixedConstraint,
      ExtendableConstraint
    };

    enum MinimumValueTag { MinimumValue = INT_MIN };
    enum MaximumValueTag { MaximumValue = INT_MAX };
    // void SetConstraints(ConstraintType type, int value);
    // void SetConstraints(ConstraintType, int lower, MaximumValueTag /*upper*/);
    // void SetConstraints(ConstraintType, MinimumValueTag lower, unsigned upper);
    // void SetConstraints(ConstraintType, MinimumValueTag lower, MaximumValueTag upper);
    void SetConstraints(ConstraintType type, int lower, unsigned upper);

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual void SetCharacterSet(ConstraintType ctype, const char * charSet);
    // virtual void SetCharacterSet(ConstraintType ctype, unsigned firstChar, unsigned lastChar);

    static PINDEX GetMaximumArraySize();
    static void SetMaximumArraySize(PINDEX sz);
    static PINDEX GetMaximumStringSize();
    static void SetMaximumStringSize(PINDEX sz);

  protected:
    PASN_Object(unsigned tag, TagClass tagClass, PBoolean extend = PFalse);

};


/** Base class for constrained ASN encoding/decoding.
*/
class PASN_ConstrainedObject : PASN_Object
{
    
  public:
    PBoolean IsConstrained() const;
    int GetLowerLimit() const;
    unsigned GetUpperLimit() const;

    PBoolean ConstrainedLengthDecode(PPER_Stream & strm, unsigned & length);
    void ConstrainedLengthEncode(PPER_Stream & strm, unsigned length) const;

    PBoolean ConstraintEncode(PPER_Stream & strm, unsigned value) const;

  protected:
    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    PASN_ConstrainedObject(unsigned tag, PASN_Object::TagClass tagClass);

};


/** Class for ASN Null type.
*/
class PASN_Null : PASN_Object
{
    
  public:
    PASN_Null(unsigned tag = UniversalNull,
              PASN_Object::TagClass tagClass = UniversalTagClass);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
   

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;
};


/** Class for ASN Boolean type.
*/
class PASN_Boolean : PASN_Object
{
    
  public:
    PASN_Boolean(PBoolean val = PFalse);
    PASN_Boolean(unsigned tag, PASN_Object::TagClass tagClass, PBoolean val = PFalse);

    operator PBoolean() const;
    PBoolean GetValue() const;
    void SetValue(PBoolean v);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
   

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

};


/** Class for ASN Integer type.
*/
class PASN_Integer : PASN_ConstrainedObject
{
    
  public:
    PASN_Integer(unsigned val = 0);
    PASN_Integer(unsigned tag, PASN_Object::TagClass tagClass, unsigned val = 0);

    operator unsigned() const;
    unsigned GetValue() const;
    void SetValue(unsigned v);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
   

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;

    PBoolean IsUnsigned() const;

};

struct PASN_Names{
    const char * name;
    PINDEX value; 
};

/** Class for ASN Enumerated type.
*/
class PASN_Enumeration : PASN_Object
{
    
  public:
    PASN_Enumeration(unsigned val = 0);
    PASN_Enumeration(unsigned tag,
                     PASN_Object::TagClass tagClass,
                     unsigned nEnums = P_MAX_INDEX,
                     PBoolean extendable = PFalse,
                     unsigned val = 0);
    PASN_Enumeration(unsigned tag,
                     PASN_Object::TagClass tagClass,
                     unsigned nEnums,
                     PBoolean extendable,
                     const PASN_Names * nameSpec,
                     unsigned namesCnt,
                     unsigned val = 0);


    operator unsigned() const;
    unsigned GetValue() const;
    void SetValue(unsigned v);

    unsigned GetMaximum() const;

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
   

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;


    PINDEX GetValueByName(PString name) const; 
};


/** Class for ASN floating point type.
*/
class PASN_Real : PASN_Object
{
    
  public:
    PASN_Real(double val = 0);
    PASN_Real(unsigned tag, PASN_Object::TagClass tagClass, double val = 0);

    operator double() const;
    double GetValue() const;
    void SetValue(double v);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
   

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

};


/** Class for ASN Object Identifier type.
*/
class PASN_ObjectId : PASN_Object
{
    
  public:
    PASN_ObjectId(const char * dotstr = NULL);
    PASN_ObjectId(unsigned tag, PASN_Object::TagClass tagClass);

    PASN_ObjectId(const PASN_ObjectId & other);

    //void SetValue(const PString & dotstr);
    void SetValue(const PUnsignedArray & numbers);

    bool operator==(const char * dotstr) const;
    bool operator!=(const char * dotstr) const;
    bool operator==(const PString & dotstr) const;
    bool operator!=(const PString & dotstr) const;
    bool operator==(const PASN_ObjectId & id) const;

    PINDEX GetSize() const;
    unsigned operator[](PINDEX idx) const;
    const PUnsignedArray & GetValue() const;
    PString AsString() const;

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
   

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    PBoolean CommonDecode(PASN_Stream & strm, unsigned dataLen);
    void CommonEncode(PBYTEArray & eObjId) const;

};


/** Class for ASN Bit String type.
*/
class PASN_BitString : PASN_ConstrainedObject
{
    
  public:
    PASN_BitString(unsigned nBits = 0, const BYTE * buf = NULL);
    PASN_BitString(unsigned tag, PASN_Object::TagClass tagClass, unsigned nBits = 0);

    PASN_BitString(const PASN_BitString & other);

    void SetData(unsigned nBits, const PBYTEArray & bytes);
    void SetData(unsigned nBits, const BYTE * buf, PINDEX size = 0);

    const BYTE * GetDataPointer() const;

    unsigned GetSize() const;
    PBoolean SetSize(unsigned nBits);

    bool operator[](PINDEX bit) const;
    void Set(unsigned bit);
    void Clear(unsigned bit);
    void Invert(unsigned bit);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;  

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    PBoolean DecodeBER(PBER_Stream & strm, unsigned len);
    void EncodeBER(PBER_Stream & strm) const;

    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;


    PBoolean DecodeSequenceExtensionBitmap(PPER_Stream & strm);
    void EncodeSequenceExtensionBitmap(PPER_Stream & strm) const;

};


/** Class for ASN Octet String type.
*/
class PASN_OctetString : PASN_ConstrainedObject
{
    
  public:
    PASN_OctetString(const char * str = NULL, PINDEX size = 0);
    PASN_OctetString(unsigned tag, PASN_Object::TagClass tagClass);

    PASN_OctetString(const PASN_OctetString & other);


    void SetValue(const char * str);
    void SetValue(const PString & str);
    void SetValue(const PBYTEArray & arr);
    void SetValue(const BYTE * data, PINDEX len);
	PString AsString() const;
    const PBYTEArray & GetValue() const;

    BYTE operator[](PINDEX i) const;
	
    //BYTE & operator[](PINDEX i);
	
    BYTE * GetPointer(PINDEX sz = 0);
    PINDEX GetSize() const;
    PBoolean SetSize(PINDEX newSize);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;

    PBoolean DecodeSubType(PASN_Object &) const;
    void EncodeSubType(const PASN_Object &);

};


/** Base class for ASN String types.
*/
class PASN_ConstrainedString : PASN_ConstrainedObject
{
    
  public:

    const PString & GetValue() const;
    void SetValue(const char * v);
    void SetValue(const PString & v);
	
    char operator[](PINDEX idx) const;

    void SetCharacterSet(ConstraintType ctype, const char * charSet);
    void SetCharacterSet(ConstraintType ctype, unsigned firstChar = 0, unsigned lastChar = 255);
    void SetCharacterSet(const char * charSet, PINDEX size, ConstraintType ctype);

    virtual Comparison Compare(const PObject & obj) const;
   
    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;


    PBoolean DecodeBER(PBER_Stream & strm, unsigned len);
    void EncodeBER(PBER_Stream & strm) const;

    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;


  protected:
    PASN_ConstrainedString(const char * canonicalSet, PINDEX setSize,
                           unsigned tag, PASN_Object::TagClass tagClass);
};


class PASN_NumericString : PASN_ConstrainedString 
{ 

public: 
  PASN_NumericString(const char * str = NULL); 
  PASN_NumericString(unsigned tag, PASN_Object::TagClass tagClass); 

  virtual PObject * Clone() const; 
  virtual PString GetTypeAsString() const; 
};
  
class PASN_PrintableString : PASN_ConstrainedString 
{
public: 
  PASN_PrintableString(const char * str = NULL); 
  PASN_PrintableString(unsigned tag, PASN_Object::TagClass tagClass); 

  virtual PObject * Clone() const; 
  virtual PString GetTypeAsString() const; 
};
  
class PASN_VisibleString : PASN_ConstrainedString { 

public: 
  PASN_VisibleString(const char * str = NULL); 
  PASN_VisibleString(unsigned tag, PASN_Object::TagClass tagClass); 
 
  virtual PObject * Clone() const; 
  virtual PString GetTypeAsString() const; 
};
  
class PASN_IA5String : PASN_ConstrainedString { 

public: 
  PASN_IA5String(const char * str = NULL); 
  PASN_IA5String(unsigned tag, PASN_Object::TagClass tagClass); 

  virtual PObject * Clone() const; 
  virtual PString GetTypeAsString() const; 
};
  
class PASN_GeneralString : PASN_ConstrainedString { 

public: 
  PASN_GeneralString(const char * str = NULL); 
  PASN_GeneralString(unsigned tag, PASN_Object::TagClass tagClass); 

  virtual PObject * Clone() const; 
  virtual PString GetTypeAsString() const; 
};


/** Class for ASN BMP (16 bit) String type.
*/
class PASN_BMPString : PASN_ConstrainedObject
{
    
  public:
    PASN_BMPString(const char * str = NULL);
    PASN_BMPString(const PWCharArray & wstr);
	PASN_BMPString(unsigned tag, PASN_Object::PASN_Object::TagClass tagClass);

    PASN_BMPString(const PASN_BMPString & other);

    PString GetValue() const;
	
    void GetValue(PWCharArray & v) const;
    void SetValue(const char * v);
    void SetValue(const PString & v);
    void SetValue(const PWCharArray & v);
    void SetValue(const PASN_BMPString & v);
    void SetValueRaw(const PWCharArray & v);
    void SetValueRaw(const wchar_t * val, PINDEX len);

    void SetCharacterSet(ConstraintType ctype, const char * charSet);
    void SetCharacterSet(ConstraintType ctype, const PWCharArray & charSet);
    void SetCharacterSet(ConstraintType ctype, unsigned firstChar, unsigned lastChar);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
   
    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    PBoolean DecodeBER(PBER_Stream & strm, unsigned len);
    void EncodeBER(PBER_Stream & strm) const;

    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;


  protected:
    void Construct();
    PBoolean IsLegalCharacter(WORD ch);

};


class PASN_GeneralisedTime : PASN_VisibleString
{
    
  public:
    PASN_GeneralisedTime();
    PASN_GeneralisedTime(const PTime & time);
    PASN_GeneralisedTime(unsigned theTag, PASN_Object::PASN_Object theTagClass);

	
    void SetValue(const PTime & time);
    PTime GetValue() const;
};


class PASN_UniversalTime : PASN_VisibleString
{
    
  public:
    PASN_UniversalTime();
    PASN_UniversalTime(const PTime & time);
    PASN_UniversalTime(unsigned theTag, PASN_Object::PASN_Object theTagClass);

    void SetValue(const PTime & time);
    PTime GetValue() const;
};


/** Class for ASN Choice type.
*/
class PASN_Choice : PASN_Object /Abstract/
{
    
  public:
    ~PASN_Choice();

    virtual void SetTag(unsigned newTag, PASN_Object::TagClass tagClass = DefaultTagClass);
    PString GetTagName() const;
    PASN_Object & GetObject() const;
    PBoolean IsValid() const;


    operator PASN_Null &();
    operator PASN_Boolean &();
    operator PASN_Integer &();
    operator PASN_Enumeration &();
    operator PASN_Real &();
    operator PASN_ObjectId &();
    operator PASN_BitString &();
    operator PASN_OctetString &();
    operator PASN_NumericString &();
    operator PASN_PrintableString &();
    operator PASN_VisibleString &();
    operator PASN_IA5String &();
    operator PASN_GeneralString &();
    operator PASN_BMPString &();
    operator PASN_Sequence &();


    virtual PBoolean CreateObject() = 0;

    virtual Comparison Compare(const PObject & obj) const;
   

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean IsPrimitive() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    virtual PBoolean DecodePER(PPER_Stream &);
    virtual void EncodePER(PPER_Stream &) const;

    PINDEX GetValueByName(PString name) const;
	
  protected:
    PASN_Choice(unsigned nChoices = 0, PBoolean extend = PFalse);
    PASN_Choice(unsigned tag, PASN_Object::TagClass tagClass, unsigned nChoices, PBoolean extend);
    PASN_Choice(unsigned tag, PASN_Object::TagClass tagClass, unsigned nChoices, PBoolean extend, const PASN_Names * nameSpec,unsigned namesCnt);

    PASN_Choice(const PASN_Choice & other);

    PBoolean CheckCreate() const;

};


typedef PArray<PASN_Object> PASN_ObjectArray;


/** Class for ASN Sequence type.
*/
class PASN_Sequence : PASN_Object
{
    
  public:
    PASN_Sequence(unsigned tag = UniversalSequence,
                  PASN_Object::TagClass tagClass = UniversalTagClass,
                  unsigned nOpts = 0, PBoolean extend = PFalse, unsigned nExtend = 0);

    PASN_Sequence(const PASN_Sequence & other);

    PINDEX GetSize() const;
    PBoolean SetSize(PINDEX newSize);
    PASN_Object & operator[](PINDEX i) const;

    PBoolean HasOptionalField(PINDEX opt) const;
    void IncludeOptionalField(PINDEX opt);
    void RemoveOptionalField(PINDEX opt);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
   

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean IsPrimitive() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    PBoolean PreambleDecode(PASN_Stream & strm);
    void PreambleEncode(PASN_Stream & strm) const;
    PBoolean KnownExtensionDecode(PASN_Stream & strm, PINDEX fld, PASN_Object & field);
    void KnownExtensionEncode(PASN_Stream & strm, PINDEX fld, const PASN_Object & field) const;
    PBoolean UnknownExtensionsDecode(PASN_Stream & strm);
    void UnknownExtensionsEncode(PASN_Stream & strm) const;

    PBoolean PreambleDecodeBER(PBER_Stream & strm);
    void PreambleEncodeBER(PBER_Stream & strm) const;
    PBoolean KnownExtensionDecodeBER(PBER_Stream & strm, PINDEX fld, PASN_Object & field);
    void KnownExtensionEncodeBER(PBER_Stream & strm, PINDEX fld, const PASN_Object & field) const;
    PBoolean UnknownExtensionsDecodeBER(PBER_Stream & strm);
    void UnknownExtensionsEncodeBER(PBER_Stream & strm) const;

    PBoolean PreambleDecodePER(PPER_Stream & strm);
    void PreambleEncodePER(PPER_Stream & strm) const;
    PBoolean KnownExtensionDecodePER(PPER_Stream & strm, PINDEX fld, PASN_Object & field);
    void KnownExtensionEncodePER(PPER_Stream & strm, PINDEX fld, const PASN_Object & field) const;
    PBoolean UnknownExtensionsDecodePER(PPER_Stream & strm);
    void UnknownExtensionsEncodePER(PPER_Stream & strm) const;


  protected:
    PBoolean NoExtensionsToDecode(PPER_Stream & strm);
    PBoolean NoExtensionsToEncode(PPER_Stream & strm);

};


/** Class for ASN Set type.
*/
class PASN_Set : PASN_Sequence
{
    
  public:
    PASN_Set(unsigned tag = UniversalSet,
             PASN_Object::TagClass tagClass = UniversalTagClass,
             unsigned nOpts = 0, PBoolean extend = PFalse, unsigned nExtend = 0);

    virtual PObject * Clone() const;
    virtual PString GetTypeAsString() const;
};


/** Class for ASN Array type.
*/
class PASN_Array : PASN_ConstrainedObject /Abstract/
{
    
  public:
    PINDEX GetSize() const;
    PBoolean SetSize(PINDEX newSize);
    PASN_Object & operator[](PINDEX i) const;
    void Append(PASN_Object * obj);
    void RemoveAt(PINDEX i);
    void RemoveAll();

    virtual Comparison Compare(const PObject & obj) const;
   

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean IsPrimitive() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    virtual PASN_Object * CreateObject() const = 0;


  protected:
    PASN_Array(unsigned tag = UniversalSequence,
               PASN_Object::TagClass tagClass = UniversalTagClass);

    PASN_Array(const PASN_Array & other);

};


/////////////////////////////////////////////////////////////////////////////

/** Base class for ASN decoder/encoder stream.
*/
class PASN_Stream /Abstract/
{

  public:
    PASN_Stream();
    PASN_Stream(const PBYTEArray & bytes);
    PASN_Stream(const BYTE * buf, PINDEX size);

    PINDEX GetPosition() const;
    void SetPosition(PINDEX newPos);
    PBoolean IsAtEnd();
    void ResetDecoder();
    void BeginEncoding();
    void CompleteEncoding();

    virtual PBoolean Read(PChannel & chan) = 0;
    virtual PBoolean Write(PChannel & chan) = 0;

    virtual PBoolean NullDecode(PASN_Null &) = 0;
    virtual void NullEncode(const PASN_Null &) = 0;
    virtual PBoolean BooleanDecode(PASN_Boolean &) = 0;
    virtual void BooleanEncode(const PASN_Boolean &) = 0;
    virtual PBoolean IntegerDecode(PASN_Integer &) = 0;
    virtual void IntegerEncode(const PASN_Integer &) = 0;
    virtual PBoolean EnumerationDecode(PASN_Enumeration &) = 0;
    virtual void EnumerationEncode(const PASN_Enumeration &) = 0;
    virtual PBoolean RealDecode(PASN_Real &) = 0;
    virtual void RealEncode(const PASN_Real &) = 0;
    virtual PBoolean ObjectIdDecode(PASN_ObjectId &) = 0;
    virtual void ObjectIdEncode(const PASN_ObjectId &) = 0;
    virtual PBoolean BitStringDecode(PASN_BitString &) = 0;
    virtual void BitStringEncode(const PASN_BitString &) = 0;
    virtual PBoolean OctetStringDecode(PASN_OctetString &) = 0;
    virtual void OctetStringEncode(const PASN_OctetString &) = 0;
    virtual PBoolean ConstrainedStringDecode(PASN_ConstrainedString &) = 0;
    virtual void ConstrainedStringEncode(const PASN_ConstrainedString &) = 0;
    virtual PBoolean BMPStringDecode(PASN_BMPString &) = 0;
    virtual void BMPStringEncode(const PASN_BMPString &) = 0;
    virtual PBoolean ChoiceDecode(PASN_Choice &) = 0;
    virtual void ChoiceEncode(const PASN_Choice &) = 0;
    virtual PBoolean ArrayDecode(PASN_Array &) = 0;
    virtual void ArrayEncode(const PASN_Array &) = 0;
    virtual PBoolean SequencePreambleDecode(PASN_Sequence &) = 0;
    virtual void SequencePreambleEncode(const PASN_Sequence &) = 0;
    virtual PBoolean SequenceKnownDecode(PASN_Sequence &, PINDEX, PASN_Object &) = 0;
    virtual void SequenceKnownEncode(const PASN_Sequence &, PINDEX, const PASN_Object &) = 0;
    virtual PBoolean SequenceUnknownDecode(PASN_Sequence &) = 0;
    virtual void SequenceUnknownEncode(const PASN_Sequence &) = 0;

    BYTE ByteDecode();
    void ByteEncode(unsigned value);

    unsigned BlockDecode(BYTE * bufptr, unsigned nBytes);
    void BlockEncode(const BYTE * bufptr, PINDEX nBytes);

    void ByteAlign();


  private:
    void Construct();
};