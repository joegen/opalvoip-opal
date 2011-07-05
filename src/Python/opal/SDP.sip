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

%ModuleHeaderCode
#include <ptlib.h>
#include <sip/sdp.h>
%End
/////////////////////////////////////////////////////////

class SDPBandwidth
{
  public:
    unsigned operator[](const PString & type) const;
    bool Parse(const PString & param);
    void SetMax(const PCaselessString & type, unsigned value);
};


class SDPMediaFormat : PObject
{
  public:
    SDPMediaFormat(
      SDPMediaDescription & parent,
      RTP_DataFrame::PayloadTypes payloadType,
      const char * name = NULL
    );

    SDPMediaFormat(
      SDPMediaDescription & parent,
      const OpalMediaFormat & mediaFormat
    );


    RTP_DataFrame::PayloadTypes GetPayloadType() const;

    const PCaselessString & GetEncodingName() const;
    void SetEncodingName(const PString & v);

    void SetFMTP(const PString & _fmtp); 
    PString GetFMTP() const;

    unsigned GetClockRate();
    void SetClockRate(unsigned  v);

    void SetParameters(const PString & v);
    void SetRTCP_FB(const PString & v);

    const OpalMediaFormat & GetMediaFormat() const;
    OpalMediaFormat & GetWritableMediaFormat();

    bool PreEncode();
    bool PostDecode(const OpalMediaFormatList & mediaFormats, unsigned bandwidth);

  protected:
    void SetMediaFormatOptions(OpalMediaFormat & mediaFormat) const;

};

typedef PList<SDPMediaFormat> SDPMediaFormatList;

/////////////////////////////////////////////////////////

class SDPMediaDescription : PObject /Abstract/
{
  public:
    // The following enum is arranged so it can be used as a bit mask,
    // e.g. GetDirection()&SendOnly indicates send is available
    enum Direction {
      Undefined = -1,
      Inactive,
      RecvOnly,
      SendOnly,
      SendRecv
    };

    SDPMediaDescription(
      const OpalTransportAddress & address,
      const OpalMediaType & mediaType
    );

    virtual bool PreEncode();
    //virtual void Encode(const OpalTransportAddress & commonAddr, ostream & str) const;

    virtual bool Decode(const PStringArray & tokens);
    virtual bool Decode(char key, const PString & value);
    virtual bool PostDecode(const OpalMediaFormatList & mediaFormats);

    virtual SDPMediaDescription * CreateEmpty() const = 0;

    // return the string used within SDP to identify this media type
    virtual PString GetSDPMediaType() const = 0;

    // return the string used within SDP to identify the transport used by this media
    virtual PCaselessString GetSDPTransportType() const = 0;

    virtual const SDPMediaFormatList & GetSDPMediaFormats() const;

    virtual OpalMediaFormatList GetMediaFormats() const;

    virtual void AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat);

    virtual void AddMediaFormat(const OpalMediaFormat & mediaFormat);
    virtual void AddMediaFormats(const OpalMediaFormatList & mediaFormats, const OpalMediaType & mediaType);

    virtual void SetAttribute(const PString & attr, const PString & value);

    virtual void SetDirection(const Direction & d);
    virtual Direction GetDirection() const;

    virtual const OpalTransportAddress & GetTransportAddress() const;
    virtual PBoolean SetTransportAddress(const OpalTransportAddress &t);

    virtual WORD GetPort() const;

    virtual OpalMediaType GetMediaType() const;

    virtual unsigned GetBandwidth(const PString & type) const;
    virtual void SetBandwidth(const PString & type, unsigned value);

    virtual const SDPBandwidth & GetBandwidth() const;

    virtual void CreateSDPMediaFormats(const PStringArray & tokens);
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString) = 0;

    virtual PString GetSDPPortList() const = 0;

    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);

    unsigned GetPTime () const;
    unsigned GetMaxPTime () const;

  protected:
    virtual SDPMediaFormat * FindFormat(PString & str) const;

};

class SDPDummyMediaDescription : SDPMediaDescription
{
  public:
    SDPDummyMediaDescription(const OpalTransportAddress & address, const PStringArray & tokens);
    virtual SDPMediaDescription * CreateEmpty() const;
    virtual PString GetSDPMediaType() const;
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual PString GetSDPPortList() const;
};


/////////////////////////////////////////////////////////
//
//  SDP media description for media classes using RTP/AVP transport (audio and video)
//

class SDPRTPAVPMediaDescription : SDPMediaDescription
{
  public:
    SDPRTPAVPMediaDescription(const OpalTransportAddress & address, const OpalMediaType & mediaType);
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual PString GetSDPPortList() const;
    virtual void SetAttribute(const PString & attr, const PString & value);
};

/////////////////////////////////////////////////////////
//
//  SDP media description for audio media
//

class SDPAudioMediaDescription : SDPRTPAVPMediaDescription
{
  public:
    SDPAudioMediaDescription(const OpalTransportAddress & address);
    virtual SDPMediaDescription * CreateEmpty() const;
    virtual PString GetSDPMediaType() const;
    virtual void SetAttribute(const PString & attr, const PString & value);

    bool GetOfferPTime() const;
    void SetOfferPTime(bool value);

};

/////////////////////////////////////////////////////////
//
//  SDP media description for video media
//

class SDPVideoMediaDescription : SDPRTPAVPMediaDescription
{
  public:
    SDPVideoMediaDescription(const OpalTransportAddress & address);
    virtual SDPMediaDescription * CreateEmpty() const;
    virtual PString GetSDPMediaType() const;
    virtual bool PreEncode();
    void SetAttribute(const PString & attr, const PString & value);
};

/////////////////////////////////////////////////////////
//
//  SDP media description for application media
//

class SDPApplicationMediaDescription :  SDPMediaDescription
{
  public:
    SDPApplicationMediaDescription(const OpalTransportAddress & address);
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual SDPMediaDescription * CreateEmpty() const;
    virtual PString GetSDPMediaType() const;
    virtual PString GetSDPPortList() const;
};

/////////////////////////////////////////////////////////

class SDPSessionDescription : PObject
{
  public:
    SDPSessionDescription(
      time_t sessionId,
      unsigned version,
      const OpalTransportAddress & address
    );

    PString Encode() const;
    bool Decode(const PString & str, const OpalMediaFormatList & mediaFormats);

    void SetSessionName(const PString & v);
    PString GetSessionName() const;

	// linker error: (?SetUserName@SDPSessionDescription@@QAEXABVPString@@@Z)
    void SetUserName(const PString & v);
    PString GetUserName() const;

    SIP_PYLIST GetMediaDescriptions() const;
%MethodCode
		// Create the list.
		PyObject *l;
		const SDPMediaDescriptionArray & array = sipCpp->GetMediaDescriptions();

		if ((l = PyList_New(array.GetSize())) == NULL)
			return NULL;
		
		// Go through each element in the C++ instance and convert it to a
		// wrapped PString.
		for (PINDEX i = 0; i < array.GetSize(); i++)
		{
			SDPMediaDescription *data = reinterpret_cast<SDPMediaDescription *>(array[i].Clone());
			PyObject *pobj;

		   // Get the Python wrapper for the SDPMediaDescription instance, creating a new
		   // one if necessary, transfer ownership to Python.
		   if ((pobj = sipConvertFromNewType(data, sipType_SDPMediaDescription, NULL)) == NULL)
		   {
			   // There was an error so garbage collect the Python list.
			   Py_DECREF(l);
			   return NULL;
		   }
			
			// Add the wrapper to the list.
			PyList_SET_ITEM(l, i++, pobj);
		}
		sipRes = l;
%End

    SDPMediaDescription * GetMediaDescriptionByType(const OpalMediaType & rtpMediaType) const;
    SDPMediaDescription * GetMediaDescriptionByIndex(PINDEX i) const;
    void AddMediaDescription(SDPMediaDescription * md);
    
    void SetDirection(const SDPMediaDescription::Direction & d);
    SDPMediaDescription::Direction GetDirection(unsigned) const;
    bool IsHold() const;

    const OpalTransportAddress & GetDefaultConnectAddress() const;
    void SetDefaultConnectAddress(
      const OpalTransportAddress & address
    );
  
    time_t GetOwnerSessionId() const;
    void SetOwnerSessionId(time_t value);

    PINDEX GetOwnerVersion() const;
    void SetOwnerVersion(PINDEX value);

    OpalTransportAddress GetOwnerAddress() const;
    void SetOwnerAddress(OpalTransportAddress addr);

    unsigned GetBandwidth(const PString & type) const;
    void SetBandwidth(const PString & type, unsigned value);

    OpalMediaFormatList GetMediaFormats() const;

    static const PString & ConferenceTotalBandwidthType();
    static const PString & ApplicationSpecificBandwidthType();
    static const PString & TransportIndependentBandwidthType(); // RFC3890

  protected:
    void ParseOwner(const PString & str);
  
};

