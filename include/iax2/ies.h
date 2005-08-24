/*
 *
 * Inter Asterisk Exchange 2
 * 
 * The classes used to hold Information Elements.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 *  $Log: ies.h,v $
 *  Revision 1.2  2005/08/24 01:38:38  dereksmithies
 *  Add encryption, iax2 style. Numerous tidy ups. Use the label iax2, not iax
 *
 *  Revision 1.1  2005/07/30 07:01:32  csoutheren
 *  Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 *  Thanks to Derek Smithies of Indranet Technologies Ltd. for
 *  writing and contributing this code
 *
 *
 *
 *
 *
 */

#ifndef IES_H
#define IES_H

#include <ptlib.h>
#include <ptlib/sockets.h>
#include <iax2/iedata.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif


class Ie;
class Iax2Encryption;

/**Ie class is for handling information elements*/
class Ie : public PObject
{ 
  PCLASSINFO(Ie, PObject);
 public:
  /** Each of the 45 possible Information Element types */
  enum IeTypeCode {    
    ie_calledNumber      = 1,     /*!< Number or extension that is being being called  (string)    */
    ie_callingNumber     = 2,     /*!< The number of the node initating the call r (string)   */
    ie_callingAni        = 3,     /*!< The ANI (calling number) to use for billing   (string)   */
    ie_callingName       = 4,     /*!< The callers name (string)    */
    ie_calledContext     = 5,     /*!< The context we are calling to (string) */
    ie_userName          = 6,     /*!< UserName (peer or user) to use in the  authentication process (string)    */
    ie_password          = 7,     /*!< Password - which is used in the authentication process (string)    */
    ie_capability        = 8,     /*!< Bitmask of the codecs sending end supports or senders capability  (unsigned int)    */
    ie_format            = 9,     /*!< Bitmask of the desired codec  (unsigned int)    */
    ie_language          = 10,    /*!< Sending ends preferred language string)    */
    ie_version           = 11,    /*!< Sending ends protocol version - typically 2.  (short)    */
    ie_adsicpe           = 12,    /*!< CPE ADSI capability (short)    */
    ie_dnid              = 13,    /*!< Originally dialed DNID (string)    */
    ie_authMethods       = 14,    /*!< Bitmask of the available Authentication method(s)  (short)    */
    ie_challenge         = 15,    /*!< The challenge data used in  MD5/RSA authentication (string)    */
    ie_md5Result         = 16,    /*!< MD5 challenge (authentication process) result (string)    */
    ie_rsaResult         = 17,    /*!< RSA challenge (authentication process) result (string)    */
    ie_apparentAddr      = 18,    /*!< The peer's apparent address. (struct sockaddr_in)    */
    ie_refresh           = 19,    /*!< The frequency of on with to refresh registration, in units of seconds  (short)    */
    ie_dpStatus          = 20,    /*!< The dialplan status (short)    */
    ie_callNo            = 21,    /*!< Call number of the  peer  (short)    */
    ie_cause             = 22,    /*!< Description of why hangup or authentication or other failure happened (string)    */
    ie_iaxUnknown        = 23,    /*!< An IAX command that is unknown (byte)    */
    ie_msgCount          = 24,    /*!< A report on the number of  messages waiting (short)    */
    ie_autoAnswer        = 25,    /*!< auto-answering is requested (no type required, as this is a request) */
    ie_musicOnHold       = 26,    /*!< Demand for music on hold combined with  QUELCH (string or none)    */
    ie_transferId        = 27,    /*!< Identifier for a Transfer Request (int)    */
    ie_rdnis             = 28,    /*!< DNIS of the referring agent (string)    */
    ie_provisioning      = 29,    /*!< Info to be used for provisioning   (binary data)  */
    ie_aesProvisioning   = 30,    /*!< Info for provisioning AES      (binary data) */
    ie_dateTime          = 31,    /*!< Date and Time  (which is stored in binary format defined in IeDateTime)    */
    ie_deviceType        = 32,    /*!< The type of device  (string)    */
    ie_serviceIdent      = 33,    /*!< The Identifier for service (string)   */
    ie_firmwareVer       = 34,    /*!< The version of firmware    (unsigned 32 bit number)    */
    ie_fwBlockDesc       = 35,    /*!< The description for a block of firmware (unsigned 32 bit number ) */
    ie_fwBlockData       = 36,    /*!< Binary data for a block of fw   () */
    ie_provVer           = 37,    /*!< The version of provisiong     (unsigned 32 bit number)    */
    ie_callingPres       = 38,    /*!< Presentation of calling (unsigned 8 bit) */
    ie_callingTon        = 39,    /*!< Type of number calling (unsigned 8 bit)   */
    ie_callingTns        = 40,    /*!< Transit Network Select for calling (unsigned 16 bit)  */
    ie_samplingRate      = 41,    /*!< Bitmask of supported Rate of sampling . Sampling defaults to 8000 hz, (unsigned 16)    */
    ie_causeCode         = 42,    /*!< Code value which describes why the registration failed, or call hungup etc (unsigned 8 bit) */
    ie_encryption        = 43,    /*!< The method for encrption the remote code wants to use (U16)    */
    ie_encKey            = 44,    /*!< the Key for ncryption (raw binary data)    */
    ie_codecPrefs        = 45,    /*!< The codec we would prefer to use (unsigned 8 bit)    */
    ie_recJitter         = 46,    /*!< From rfc 1889, the received jitter (unsigned 32 bit number) */
    ie_recLoss           = 47,    /*!< The received loss rate, where the high byte is the loss packt, low 24 bits loss count, from rfc1889 (unsigned 32 bit)*/
    ie_recPackets        = 48,    /*!< Recevied number of frames (total frames received) (unsigned 32 bit) */
    ie_recDelay          = 49,    /*!< Received frame largest playout delay (in ms) (unsigned 16 bit)*/
    ie_recDropped        = 50,    /*!< The number of dropped received frames by the jitter buf, so it is a measure of the number of late frames (unsigned 32 bit) */
    ie_recOoo            = 51     /*!< The number of received frames which were Out Of Order (unsigned 32 number) */
  };
  
  /**@name construction/destruction*/
  //@{
  
  /**Constructor*/
  Ie();
  
  /**Destructor*/
  virtual ~Ie() { };
  //@{
  
  /**@name Worker methods*/
  //@{
  /** Given an arbitrary type code, build & initialise the Ie descendant class */
  static Ie *BuildInformationElement(BYTE _typeCode, BYTE length, BYTE *srcData);     
  
  /** returns TRUE if contains an initialised    information element */
  virtual BOOL IsValid() { return validData; }
  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return 0; }
  
  /**return the number of bytes to hold this Information Element when stored in a packet*/
  int GetBinarySize() { return 2 + GetLengthOfData(); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /** Get the enum value for this information element class */
  virtual BYTE GetKeyValue() const  { return 255; }
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(int &/*newData*/) { PAssertAlways("Ie class cannnot set data value"); };
  
  /**Read the value of the stored data for this class */
  int ReadData() { PAssertAlways("Ie class cannot read the internal data value"); return 0; };
  
  /** Take the data from this Ie and copy into the memory region.
      When finished, increment the writeIndex appropriately. */
  void WriteBinary(void *data, PINDEX & writeIndex);
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &/*res*/) { PTRACE(0, "UNIMPLEMENTED FUNCTION"); }     
  //@}
  
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE * /*data*/) { PTRACE(0, "UNIMPLEMENTED FUNCTION"); }
  
  /**A flag indicating if the data was read from the supplied bytes
     correctly, or if this structure is yet to be initialised */
  BOOL   validData;
};

/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that identifies an invalid code in the processed binary data.*/
class IeInvalidElement : public Ie
{
  PCLASSINFO(IeInvalidElement, Ie);
 public:
  IeInvalidElement() : Ie() {};
  
  /**Access function to get the length of data, which is used when writing to network packet*/
  virtual BYTE GetlengthOfData() { return 0; }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const
    { str << "Invlalid Information Element" << endl; }
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.
      For an IeInvalidElement, there is no work done, as the data is invalid. */
  virtual void WriteBinary(BYTE * /*data*/) {  }
};
/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains no data. */
class IeNone : public Ie
{
  PCLASSINFO(IeNone, Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeNone(BYTE length, BYTE *srcData);     
  
  /**Constructor to an invalid and empty result*/
  IeNone() : Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return 0; }
  
  /**Report the value stored in this class */
  BYTE GetValue() { return 0; }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(void * /*newData*/) { PAssertAlways("IeNone cannot set data"); }
  
  /**Read the value of the stored data for this class */
  int ReadData() { PAssertAlways("IeNone cannot read the internal data value"); return 0; }
  
  //@}
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.
      For this Ie (IeNone) there is no work to do as there is no data. */
  virtual void WriteBinary(BYTE * /*data*/) {  }
};

/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains one byte of data*/
class IeByte : public Ie
{
  PCLASSINFO(IeByte, Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */  
  IeByte(BYTE length, BYTE *srcData);     
  
  /**Constructor to a specific value */
  IeByte(BYTE newValue) : Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IeByte() : Ie() { }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(BYTE newData) { dataValue = newData; validData = TRUE; }
  
  /**Report the value of the stored data for this class */
  BYTE ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data) { data[0] = dataValue; }
  
  /**The actual data stored in a IeByte class */
  BYTE dataValue;
};

/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains one character of data*/
class IeChar : public Ie
{
  PCLASSINFO(IeChar, Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeChar(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialised value */
  IeChar(char newValue) : Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IeChar() : Ie() { }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(char newData) { dataValue = newData; validData = TRUE; }
  
  /**Report the value of the stored data for this class */
  char ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data) { data[0] = dataValue; }
  
  /**The actual data stored in a IeChar class */
  char dataValue;
};

/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains one short (signed 16 bits) of data*/
class IeShort : public Ie
{
  PCLASSINFO(IeShort, Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeShort(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialied value */
  IeShort(short newValue) : Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IeShort() : Ie() { }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(short newData) { dataValue = newData; validData = TRUE; }
  
  /**Report the value of the stored data for this class */
  short ReadData() { return dataValue; }  
  //@}
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IeShort class */
  short dataValue;
};
/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains one integer (signed 16 bits) of data*/
class IeInt : public Ie
{
  PCLASSINFO(IeInt, Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeInt(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialised value */
  IeInt(int  newValue) : Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IeInt() : Ie() { }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(int newData) { dataValue = newData; validData = TRUE; }
  
  /**Report the value of the stored data for this class */
  int ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IeInt class */
  int dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an unsigned short (16 bits) of data*/
class IeUShort : public Ie
{
  PCLASSINFO(IeUShort, Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeUShort(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialised value */
  IeUShort(unsigned short newValue) : Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IeUShort() : Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(unsigned short newData) { dataValue = newData; validData = TRUE; }
  
  /**Report the value of the stored data for this class */
  unsigned short ReadData() { return dataValue; }         
  //@}
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IeUShort class */
  unsigned short dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an unsigned int (32 bits) of data*/
class IeUInt : public Ie
{
  PCLASSINFO(IeUInt, Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeUInt(BYTE length, BYTE *srcData);     
  
  /**Constructor to an invalid and empty result*/
  IeUInt() : Ie() {}
  
  /**Constructor to an initialised value, (Typically used prior to transmission)*/
  IeUInt(unsigned int newValue) { SetData(newValue); }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(unsigned int &newData) { dataValue = newData; validData = TRUE; }
  
  /**Report the value of the stored data for this class */
  unsigned int ReadData() { return dataValue; }          
  //@}
  
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IeUInt class */
  unsigned int dataValue;
};

////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an array of characters. */
class IeString : public Ie
{
  PCLASSINFO(IeString, Ie);
  /**@name construction/destruction*/
  //@{
  
 public:
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeString(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialised value */
  IeString(PString & newValue) : Ie() { SetData(newValue); }
  
  /**Construct to an initialised value */
  IeString(const char * newValue) : Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IeString() : Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData();
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(PString & newData);
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(const char * newData);
  
  /**Report the value of the stored data for this class */
  PString ReadData() { return dataValue; }          
  //@}
  
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IeString class */
  PString dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains the date and time, from a 32 bit long representation*/
class IeDateAndTime : public Ie
{
  PCLASSINFO(IeDateAndTime, Ie);
  /**@name construction/destruction*/
  //@{
  
 public:
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeDateAndTime(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialized value */
  IeDateAndTime(PTime & newValue) : Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IeDateAndTime() : Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return 4; }
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(PTime & newData) { dataValue = newData; validData = TRUE; }
  
  /**Report the value of the stored data for this class */
  PTime ReadData() { return dataValue; }
  //@}
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IeDateAndTime class */
  PTime dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an array of BYTES (with possible nulls in middle) */
class IeBlockOfData : public Ie
{
  PCLASSINFO(IeBlockOfData, Ie);
  /**@name construction/destruction*/
  //@{
  
 public:
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeBlockOfData(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialized value */
  IeBlockOfData(PBYTEArray & newData) : Ie() { SetData(newData); }
  
  /**Constructor to an invalid and empty result*/
  IeBlockOfData() : Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return (BYTE)dataValue.GetSize(); }
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(PBYTEArray newData) { dataValue = newData; validData = TRUE; }
  
  /**Report the value of the stored data for this class */
  PBYTEArray ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IeBlockOfData class */
  PBYTEArray dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an Ip address and port */
class IeSockaddrIn : public Ie
{
  PCLASSINFO(IeSockaddrIn, Ie);
  /**@name construction/destruction*/
  //@{
  
 public:
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IeSockaddrIn(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialized value */
  IeSockaddrIn(PIPSocket::Address &addr, PINDEX port) : Ie() { SetData(addr, port); }
  
  /**Constructor to an invalid and empty result*/
  IeSockaddrIn() : Ie() {}
  
  /**Destructor*/
  ~IeSockaddrIn() { } ;
  //@}
  
  /**@name Worker methods*/
  //@{
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(struct sockaddr_in); }
  
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(PIPSocket::Address & newAddr, PINDEX newPort) 
    { dataValue = newAddr; portNumber = newPort; validData = TRUE; }
  
  /**Report the value of the stored data for this class */
  PIPSocket::Address ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual ip address data stored in a IeSockaddrIn class */
  PIPSocket::Address dataValue;
  
  /**The actual port number data stored in a IeSockaddrIn class */  
  PINDEX               portNumber;
};

////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains the number/extension being called.*/
class IeCalledNumber : public IeString
{
  PCLASSINFO(IeCalledNumber, IeString);
 public:
  /**Constructor from data read from the network.
     
  Contents are undefined if network data is bogus.*/
  IeCalledNumber(BYTE length, BYTE *srcData) : IeString(length, srcData) {};
  
  /**Initialise to the supplied string value */
  IeCalledNumber(PString newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_calledNumber; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.calledNumber = dataValue; }
 protected:
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains the Calling number (number trying to make call?)*/
class IeCallingNumber : public IeString
{
  PCLASSINFO(IeCallingNumber, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus. */
  IeCallingNumber(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeCallingNumber(PString newValue)  { SetData(newValue); } 
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingNumber; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.callingNumber = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the calling number ANI (for billing)*/
class IeCallingAni : public IeString
{
  PCLASSINFO(IeCallingAni, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus. */
  IeCallingAni(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeCallingAni(PString newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingAni; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.callingAni = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains name of the the person making the call*/
class IeCallingName : public IeString
{
  PCLASSINFO(IeCallingName, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeCallingName(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeCallingName(PString newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingName; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.callingName = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the context for this number*/
class IeCalledContext : public IeString
{
  PCLASSINFO(IeCalledContext, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeCalledContext(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeCalledContext(PString newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_calledContext; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.calledContext = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the userName (peer or user) for authentication*/
class IeUserName : public IeString
{
  PCLASSINFO(IeUserName, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeUserName(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeUserName(PString newValue)  { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_userName; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.userName = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the password (for authentication)*/
class IePassword : public IeString
{
  PCLASSINFO(IePassword, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IePassword(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IePassword(PString newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_password; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.password = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the actual codecs available*/
class IeCapability : public IeUInt
{
  PCLASSINFO(IeCapability, IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeCapability(BYTE length, BYTE *srcData) : IeUInt(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IeCapability(unsigned int newValue) : IeUInt(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_capability; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.capability = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the desired codec format*/
class IeFormat : public IeUInt
{
  PCLASSINFO(IeFormat, IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeFormat(BYTE length, BYTE *srcData) : IeUInt(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IeFormat(unsigned int newValue) : IeUInt(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_format; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.format = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the desired language*/
class IeLanguage : public IeString
{
  PCLASSINFO(IeLanguage, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeLanguage(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeLanguage(PString newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_language; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.language = dataValue; }     
 protected:
};

//////////////////////////////////////////////////////////////////////
/**An Information Element that contains the protocol version*/
class IeVersion : public IeShort
{
  PCLASSINFO(IeVersion, IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeVersion(BYTE length, BYTE *srcData) : IeShort(length, srcData) { };
  
  /**Construct to the one possible value - version 2 */
  IeVersion() { dataValue = 2; validData = TRUE; }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_version; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.version = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the CPE ADSI capability*/
class IeAdsicpe : public IeShort
{
  PCLASSINFO(IeAdsicpe, IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeAdsicpe(BYTE length, BYTE *srcData) : IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_adsicpe; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.adsicpe = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains  the originally dialed DNID*/
class IeDnid : public IeString
{
  PCLASSINFO(IeDnid, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeDnid(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeDnid(PString newValue)  { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_dnid; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.dnid = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the authentication methods */
class IeAuthMethods : public IeShort
{
  PCLASSINFO(IeAuthMethods, IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeAuthMethods(BYTE length, BYTE *srcData) : IeShort(length, srcData) { };
  
  /** Initialise to the supplied short value, which is usually done prior to transmission*/
  IeAuthMethods(short newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_authMethods; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.authMethods = dataValue; }     
  
  /**Return true if the supplied value has the RSA key on*/
  static BOOL IsRsaAuthentication(short testValue) { return InternalIsRsa(testValue); }
  
  /**Return true if the supplied value has the MD5 key on*/
  static BOOL IsMd5Authentication(short testValue) { return InternalIsMd5(testValue); }
  
  /**Return true if the supplied value has the plain text  key on*/
  static BOOL IsPlainTextAuthentication(short testValue) { return InternalIsPlainText(testValue); }     
  
  /**Return true if the internal value has the RSA key on*/
  BOOL IsRsaAuthentication() { if (IsValid()) return InternalIsRsa(dataValue); else return FALSE; }
  
  /**Return true if the internal value has the MD5 key on*/
  BOOL IsMd5Authentication() { if (IsValid()) return InternalIsMd5(dataValue); else return FALSE; }
  
  /**Return true if the internal value has the plain text  key on*/
  BOOL IsPlainTextAuthentication() { if (IsValid()) return InternalIsPlainText(dataValue); else return FALSE; }
  
 protected:
  
  /**Return true if the supplied value has the RSA key on*/
  static BOOL InternalIsRsa(short testValue) { return testValue  && 0x04; }
  
  /**Return true if the supplied value has the MD5 key on*/
  static BOOL InternalIsMd5(short testValue) { return testValue  && 0x02; }
  
  /**Return true if the supplied value has the plain text  key on*/
  static BOOL InternalIsPlainText(short testValue) { return testValue  && 0x01; }
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the challenge data for MD5/RSA*/
class IeChallenge : public IeString
{
  PCLASSINFO(IeChallenge, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeChallenge(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeChallenge(PString newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_challenge; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.challenge = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the MD5 chaallenge result*/
class IeMd5Result : public IeString
{
  PCLASSINFO(IeMd5Result, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeMd5Result(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeMd5Result(PString newValue) { SetData(newValue); }
  
  /**Take the challenge and password, calculate the result, and store */
  IeMd5Result(PString &challenge, PString &password);
  
  /**Take the supplied Iax2Encrption arguement, calculate the result, and store */
  IeMd5Result(Iax2Encryption & encryption);
  
  /**Initialize the internal structurees */
  void InitializeChallengePassword(const PString &newChallenge, const PString &newPassword);

  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_md5Result; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.md5Result = dataValue; }     

  /**GetAccess to the stomach, which is the concatanation of the various
     components used to make a key */
  PBYTEArray & GetDataBlock() { return dataBlock; }

 protected:

  /**The contents of the stomach in a binary block */
  PBYTEArray dataBlock;
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the RSA challenge result*/
class IeRsaResult : public IeString
{
  PCLASSINFO(IeRsaResult, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeRsaResult(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IeRsaResult(PString newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_rsaResult; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.rsaResult = dataValue; }
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the apparent daddress of peer*/
class IeApparentAddr : public IeSockaddrIn
{
  PCLASSINFO(IeApparentAddr, IeSockaddrIn);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeApparentAddr(BYTE length, BYTE *srcData) : IeSockaddrIn(length, srcData) { };
  
  /**Desttructor, which does nothing */
  ~IeApparentAddr() { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_apparentAddr; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.apparentAddr = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the time when to refresh registration*/
class IeRefresh : public IeShort
{
  PCLASSINFO(IeRefresh, IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeRefresh(BYTE length, BYTE *srcData) : IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_refresh; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.refresh = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the dial plan status*/
class IeDpStatus : public IeShort
{
  PCLASSINFO(IeDpStatus, IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeDpStatus(BYTE length, BYTE *srcData) : IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_dpStatus; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.dpStatus = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains  the call number of peer*/
class IeCallNo : public IeShort
{
  PCLASSINFO(IeCallNo, IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeCallNo(BYTE length, BYTE *srcData) : IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callNo; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.callNo = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the cause*/
class IeCause : public IeString
{
  PCLASSINFO(IeCause, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeCause(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IeCause(PString & newValue) : IeString(newValue) { }
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IeCause(const char *newValue) : IeString(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_cause; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.cause = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains  an unknown IAX command*/
class IeIaxUnknown : public IeByte
{
  PCLASSINFO(IeIaxUnknown, IeByte);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeIaxUnknown(BYTE length, BYTE *srcData) : IeByte(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_iaxUnknown; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.iaxUnknown = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains how many messages are waiting*/
class IeMsgCount : public IeShort
{
  PCLASSINFO(IeMsgCount, IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeMsgCount(BYTE length, BYTE *srcData) : IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_msgCount; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.msgCount = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains a request for auto-answering*/
class IeAutoAnswer : public IeNone
{
  PCLASSINFO(IeAutoAnswer, IeNone);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeAutoAnswer(BYTE length, BYTE *srcData) : IeNone(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_autoAnswer; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.autoAnswer = TRUE;; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains a request for music on hold with Quelch*/
class IeMusicOnHold : public IeNone
{
  PCLASSINFO(IeMusicOnHold, IeNone);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeMusicOnHold(BYTE length, BYTE *srcData) : IeNone(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_musicOnHold; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.musicOnHold = TRUE; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains a transfer request identifier*/
class IeTransferId : public IeInt
{
  PCLASSINFO(IeTransferId, IeInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeTransferId(BYTE length, BYTE *srcData) : IeInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_transferId; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.transferId = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the referring DNIs*/
class IeRdnis : public IeString
{
  PCLASSINFO(IeRdnis, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeRdnis(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_rdnis; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.rdnis = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains Provisioning - a great big data block */
class IeProvisioning : public IeBlockOfData
{
  PCLASSINFO(IeProvisioning, IeBlockOfData);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeProvisioning(BYTE length, BYTE *srcData) : IeBlockOfData   (length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_provisioning; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &/*res*/) {  }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains aes provisioning info*/
class IeAesProvisioning : public IeNone
{
  PCLASSINFO(IeAesProvisioning, IeNone);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeAesProvisioning(BYTE length, BYTE *srcData) : IeNone(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_aesProvisioning; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &/*res*/) {  }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the date and time (in 32 bits)*/
class IeDateTime : public IeDateAndTime
{
  PCLASSINFO(IeDateTime, IeDateAndTime);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeDateTime(BYTE length, BYTE *srcData) : IeDateAndTime(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_dateTime; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.dateTime = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the device type*/
class IeDeviceType : public IeString
{
  PCLASSINFO(IeDeviceType, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeDeviceType(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_deviceType; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.deviceType = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the service identifier*/
class IeServiceIdent : public IeString
{
  PCLASSINFO(IeServiceIdent, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeServiceIdent(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_serviceIdent; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.serviceIdent = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the firmware version*/
class IeFirmwareVer : public IeShort
{
  PCLASSINFO(IeFirmwareVer, IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeFirmwareVer(BYTE length, BYTE *srcData) : IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_firmwareVer; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.firmwareVer = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains firmware block description*/
class IeFwBlockDesc : public IeUInt
{
  PCLASSINFO(IeFwBlockDesc, IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeFwBlockDesc(BYTE length, BYTE *srcData) : IeUInt(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IeFwBlockDesc(unsigned int newValue) : IeUInt(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_fwBlockDesc; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.fwBlockDesc = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the block of firmware data */
class IeFwBlockData : public IeBlockOfData
{
  PCLASSINFO(IeFwBlockData, IeBlockOfData);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeFwBlockData(BYTE length, BYTE *srcData) : IeBlockOfData(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_fwBlockData; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.fwBlockData = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the Provisioning version*/
class IeProvVer : public IeUInt
{
  PCLASSINFO(IeProvVer, IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeProvVer(BYTE length, BYTE *srcData) : IeUInt(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IeProvVer(unsigned int newValue) : IeUInt(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_provVer; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.provVer = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the calling presentation*/
class IeCallingPres : public IeByte
{
  PCLASSINFO(IeCallingPres, IeByte);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeCallingPres(BYTE length, BYTE *srcData) : IeByte(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingPres; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.callingPres = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the calling type of number*/
class IeCallingTon : public IeByte
{
  PCLASSINFO(IeCallingTon, IeByte);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeCallingTon(BYTE length, BYTE *srcData) : IeByte(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingTon; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.callingTon = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the calling transit network select */
class IeCallingTns : public IeUShort
{
  PCLASSINFO(IeCallingTns, IeUShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeCallingTns(BYTE length, BYTE *srcData) : IeUShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingTns; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.callingTns = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the supported sampling rates*/
class IeSamplingRate : public IeUShort
{
  PCLASSINFO(IeSamplingRate, IeUShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeSamplingRate(BYTE length, BYTE *srcData) : IeUShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_samplingRate; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.samplingRate = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the encryption format*/
class IeEncryption : public IeUShort
{
  PCLASSINFO(IeEncryption, IeUShort);
 public:
  /**Specify the different encryption methods */
  enum IeEncryptionMethod {
    encryptAes128 = 1    /*!< Specify to use AES 128 bit encryption */
  };

  /**Constructor to specify a particular encryption method */
  IeEncryption(IeEncryptionMethod method = encryptAes128);

  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeEncryption(BYTE length, BYTE *srcData) : IeUShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_encryption; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.encryptionMethods = dataValue; }
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the encryption key (raw)*/
class IeEncKey : public IeString
{
  PCLASSINFO(IeEncKey, IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeEncKey(BYTE length, BYTE *srcData) : IeString(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_encKey; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.encKey = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains data for codec negotiation. */
class IeCodecPrefs : public IeByte
{
  PCLASSINFO(IeCodecPrefs, IeByte);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeCodecPrefs(BYTE length, BYTE *srcData) : IeByte(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_codecPrefs; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.codecPrefs = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the received jitter */
class IeReceivedJitter : public IeUInt
{
  PCLASSINFO(IeReceivedJitter, IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeReceivedJitter(BYTE length, BYTE *srcData) : IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recJitter; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.receivedJitter = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the received loss */
class IeReceivedLoss : public IeUInt
{
  PCLASSINFO(IeReceivedLoss, IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeReceivedLoss(BYTE length, BYTE *srcData) : IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recLoss; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.receivedLoss = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the received frames */
class IeReceivedFrames : public IeUInt
{
  PCLASSINFO(IeReceivedFrames, IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeReceivedFrames(BYTE length, BYTE *srcData) : IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recPackets; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.receivedPackets = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the received delay */
class IeReceivedDelay : public IeUShort
{
  PCLASSINFO(IeReceivedDelay, IeUShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeReceivedDelay(BYTE length, BYTE *srcData) : IeUShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recDelay; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.receivedDelay = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the number of dropped frames */
class IeDroppedFrames : public IeUInt
{
  PCLASSINFO(IeDroppedFrames, IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeDroppedFrames(BYTE length, BYTE *srcData) : IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recDropped; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.receivedDropped = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the number of frames received out of order */
class IeReceivedOoo : public IeUInt
{
  PCLASSINFO(IeReceivedOoo, IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IeReceivedOoo(BYTE length, BYTE *srcData) : IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recOoo; }
  
  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IeData &res) { res.receivedOoo = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////


PDECLARE_LIST (IeList, Ie *)
#ifdef DOC_PLUS_PLUS 
/**An array of IE* elements are stored in this list */
class IeList : public Ie *
{
#endif
 public:
  /**Destructor, so all eleents are destroyed on destruction */
  ~IeList();
  
  /**Access method, get pointer to infromation element at index. 
     Returns NULL if index is out of bounds.
     This will remove the specified Ie from the list. */
  Ie *RemoveIeAt(PINDEX i);
  
  /**Access method, get pointer to last information element in the list.
     Returns NULL if index is out of bounds.
     This will remove the specified Ie from the list. */
  Ie *RemoveLastIe();
  
  /**Initialisation - Objects are not automatically deleted on removal */
  void Initialise() {  DisallowDeleteObjects(); }
  
  /**Delete item at a particular index */
  void DeleteAt(PINDEX idex);
  
  /**Test to see if list is empty - returns TRUE if no elements stored in this list */
  BOOL Empty() { return GetSize() == 0; }
  
  /**Test to see if list is empty - returns TRUE if no elements stored in this list */
  BOOL IsEmpty() { return GetSize() == 0; }
  
  /**Add a new Ie to the list */
  void AppendIe(Ie *newMember) { Append(newMember);}
  
  /**Get the number of bytes to store all these Ie's in a network packet */
  int GetBinaryDataSize();
  
  /**Get a pointer to the Ie which is stored at index i*/
  Ie *GetIeAt(int i); 
  
 protected:
  
};


#endif // IAX_IES_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */
