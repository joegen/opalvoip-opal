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

///////////////////////////////////////////////////////////////////////////////

/**Base class for options attached to an OpalMediaFormat.
  */
class OpalMediaOption : PObject /Abstract/
{
%TypeHeaderCode
#include <ptlib.h>
#include <opal/mediafmt.h>
%End

  public:
    enum MergeType {
      NoMerge,
      MinMerge,
      MaxMerge,
      EqualMerge,
      NotEqualMerge,
      AlwaysMerge,

      // Synonyms
      AndMerge = MinMerge,
      OrMerge  = MaxMerge
    };

  protected:
    OpalMediaOption(
      const PString & name
    );
    OpalMediaOption(
      const char * name,
      bool readOnly,
      MergeType merge
    );

  public:
    virtual Comparison Compare(const PObject & obj) const;

    virtual bool Merge(
      const OpalMediaOption & option
    );
    virtual Comparison CompareValue(
      const OpalMediaOption & option
    ) const = 0;
    virtual void Assign(
      const OpalMediaOption & option
    ) = 0;

    PString AsString() const;
    bool FromString(const PString & value);

    const PString & GetName() const;

    bool IsReadOnly() const;
    void SetReadOnly(bool readOnly);

    MergeType GetMerge() const;
    void SetMerge(MergeType merge);

    const PString & GetFMTPName() const;
    void SetFMTPName(const char * name);

    const PString & GetFMTPDefault() const;
    void SetFMTPDefault(const char * value);


    struct H245GenericInfo {
      H245GenericInfo();
      unsigned ordinal;
	  
      enum Modes {
        None,
        Collapsing,
        NonCollapsing
      };
	  
      enum IntegerTypes {
        UnsignedInt,
        Unsigned32,
        BooleanArray
      };
	  
      bool excludeTCS;
      bool excludeOLC;
      bool excludeReqMode;
    };

    const H245GenericInfo & GetH245Generic() const;
    void SetH245Generic(const H245GenericInfo & genericInfo);

};

///////////////////////////////////////////////////////////////////////////////

/**This class describes a media format as used in the OPAL system. A media
   format is the type of any media data that is trasferred between OPAL
   entities. For example an audio codec such as G.723.1 is a media format, a
   video codec such as H.261 is also a media format.
  */
class OpalMediaFormat : PContainer
{

%TypeHeaderCode
#include <ptlib.h>
#include <opal/mediafmt.h>
%End

  public:
    /**This form of the constructor will register the full details of the
       media format into an internal database. This would typically be used
       as a static global. In fact it would be very dangerous for an instance
       to use this constructor in any other way, especially local variables.

       If the rtpPayloadType is RTP_DataFrame::DynamicBase, then the RTP
       payload type is actually set to the first unused dynamic RTP payload
       type that is in the registers set of media formats.

       The frameSize parameter indicates that the media format has a maximum
       size for each data frame, eg G.723.1 frames are no more than 24 bytes
       long. If zero then there is no intrinsic maximum, eg G.711.
      */
    OpalMediaFormat(
      const char * fullName,                      ///<  Full name of media format
      const OpalMediaType & mediaType,            ///<  media type for this format
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      const char * encodingName,                  ///<  RTP encoding name
      PBoolean     needsJitter,                   ///<  Indicate format requires a jitter buffer
      unsigned bandwidth,                         ///<  Bandwidth in bits/second
      PINDEX   frameSize,                         ///<  Size of frame in bytes (if applicable)
      unsigned frameTime,                         ///<  Time for frame in RTP units (if applicable)
      unsigned clockRate,                         ///<  Clock rate for data (if applicable)
      time_t timeStamp = 0                        ///<  timestamp (for versioning)
    );

    /**Construct a media format, searching database for information.
       This constructor will search through the RegisteredMediaFormats list
       for the match of the payload type, if found the other information
       fields are set from the database. If not found then the ancestor
       string is set to the empty string.

       Note it is impossible to determine the order of registration so this
       should not be relied on.
      */
    OpalMediaFormat(
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      unsigned clockRate,                         ///<  clock rate
      const char * rtpEncodingName = NULL,        ///<  RTP payload type name
      const char * protocol = NULL                ///<  valid protocol (if NULL, then all)
    );

    /**Construct a media format, searching database for information.
       This constructor will search through the RegisteredMediaFormats list
       for the wildcard match of the parameter string, if found the other
       information fields are set from the database. If not found then the
       ancestor string is set to the empty string.

       The wildcard string is a simple substring match using the '*'
       character. For example: "G.711*" would match the first of
       "G.711-uLaw-64k" and "G.711-ALaw-64k" to have been registered.

       Note it is impossible to determine the order of registration so this
       should not be relied on.
      */
    OpalMediaFormat(
      const char * wildcard  ///<  Wildcard name to search for
    );

    /**Construct a media format, searching database for information.
       This constructor will search through the RegisteredMediaFormats list
       for the wildcard match of the parameter string, if found the other
       information fields are set from the database. If not found then the
       ancestor string is set to the empty string.

       The wildcard string is a simple substring match using the '*'
       character. For example: "G.711*" would match the first of
       "G.711-uLaw-64k" and "G.711-ALaw-64k" to have been registered.

       Note it is impossible to determine the order of registration so this
       should not be relied on.
      */
    OpalMediaFormat(
      const PString & wildcard  ///<  Wildcard name to search for
    );
   

    /**Create a copy of the media format.
      */
    virtual PObject * Clone() const;

    /**Compare two media formats.
      */
    virtual Comparison Compare(const PObject & obj) const;

    /**This will translate the codec specific "custom" options to OPAL
       "normalised" options, e.g. For H.261 "QCIF MPI"="1", "CIF MPI"="5"
        would be translated to "Frame Width"="176", "Frame Height"="144".
      */
    bool ToNormalisedOptions();

    /**This will do the reverse of ToNormalisedOptions, translating the OPAL
       "normalised" options to codec specific "custom" options.
      */
    bool ToCustomisedOptions();

    /**Merge with another media format. This will alter and validate
       the options for this media format according to the merge rule for
       each option. The parameter is typically a "capability" while the
       current object isthe proposed channel format. This if the current
       object has a tx number of frames of 3, but the parameter has a value
       of 1, then the current object will be set to 1.

       Returns PFalse if the media formats are incompatible and cannot be
       merged.
      */
    bool Merge(
      const OpalMediaFormat & mediaFormat
    );

    /**Get the name of the format
      */
    PString GetName() const;

    /**Return PTrue if media format info is valid. This may be used if the
       single string constructor is used to check that it matched something
       in the registered media formats database.
      */
    PBoolean IsValid() const;

    /**Return PTrue if media format info may be sent via RTP. Some formats are internal
       use only and are never transported "over the wire".
      */
    PBoolean IsTransportable() const;

    /**Get the RTP payload type that is to be used for this media format.
       This will either be an intrinsic one for the media format eg GSM or it
       will be automatically calculated as a dynamic media format that will be
       uniqueue amongst the registered media formats.
      */
    RTP_DataFrame::PayloadTypes GetPayloadType() const;
    void SetPayloadType(RTP_DataFrame::PayloadTypes type);

    /**Get the RTP encoding name that is to be used for this media format.
      */
    const char * GetEncodingName() const;

    /** Get the media type for this format
      */
    OpalMediaType GetMediaType() const;

    /**Determine if the media format requires a jitter buffer. As a rule an
       audio codec needs a jitter buffer and all others do not.
      */
    bool NeedsJitterBuffer() const;
    static const PString & NeedsJitterOption();

    /**Get the average bandwidth used in bits/second.
      */
    unsigned GetBandwidth() const;
    static const PString & MaxBitRateOption();
    static const PString & TargetBitRateOption();

    /**Get the maximum frame size in bytes. If this returns zero then the
       media format has no intrinsic maximum frame size, eg a video format
       would return zero but G.723.1 would return 24.
      */
    PINDEX GetFrameSize() const;
    static const PString & MaxFrameSizeOption();

    /**Get the frame time in RTP timestamp units. If this returns zero then
       the media format is not real time and has no intrinsic timing eg T.120
      */
    unsigned GetFrameTime() const;
    static const PString & FrameTimeOption();

    /**Get the number of RTP timestamp units per millisecond.
      */
    unsigned GetTimeUnits() const;

    enum StandardClockRate {
      AudioClockRate = 8000,  ///<  8kHz sample rate
      VideoClockRate = 90000  ///<  90kHz sample rate
    };

    /**Get the clock rate in Hz for this format.
      */
    unsigned GetClockRate() const;
    static const PString & ClockRateOption();

    /**Get the name of the OpalMediaOption indicating the protocol the format is being used on.
      */
    static const PString & ProtocolOption();

    /**Get the name of the OpalMediaOption indicating the maximum RTP payload size.
      */
    static const PString & MaxTxPacketSizeOption();

    /**Get all of the option values of the format as a dictionary.
       Each entry is a name value pair.
      */
    PStringToString GetOptions() const;

    /**Get the number of options this media format has.
      */
    PINDEX GetOptionCount() const;

    /**Get the option instance at the specified index. This contains the
       description and value for the option.
      */
    const OpalMediaOption & GetOption(
      PINDEX index   ///<  Index of option in list to get
    ) const;

    /**Get the option value of the specified name as a string.

       Returns false of the option is not present.
      */
    bool GetOptionValue(
      const PString & name,   ///<  Option name
      PString & value /Out/        ///<  String to receive option value
    ) const;

    /**Set the option value of the specified name as a string.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present.
      */
    bool SetOptionValue(
      const PString & name,   ///<  Option name
      const PString & value   ///<  New option value as string
    );

    /**Get the option value of the specified name as a boolean. The default
       value is returned if the option is not present.
      */
    bool GetOptionBoolean(
      const PString & name,   ///<  Option name
      bool dflt = PFalse       ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as a boolean.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionBoolean(
      const PString & name,   ///<  Option name
      bool value              ///<  New value for option
    );

    /**Get the option value of the specified name as an integer. The default
       value is returned if the option is not present.
      */
    int GetOptionInteger(
      const PString & name,   ///<  Option name
      int dflt = 0            ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as an integer.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present, not of the same type or
       is putside the allowable range.
      */
    bool SetOptionInteger(
      const PString & name,   ///<  Option name
      int value               ///<  New value for option
    );

    /**Get the option value of the specified name as a real. The default
       value is returned if the option is not present.
      */
    double GetOptionReal(
      const PString & name,   ///<  Option name
      double dflt = 0         ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as a real.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionReal(
      const PString & name,   ///<  Option name
      double value            ///<  New value for option
    );

    /**Get the option value of the specified name as an index into an
       enumeration list. The default value is returned if the option is not
       present.
      */
    PINDEX GetOptionEnum(
      const PString & name,   ///<  Option name
      PINDEX dflt = 0         ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as an index into an enumeration.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionEnum(
      const PString & name,   ///<  Option name
      PINDEX value            ///<  New value for option
    );

    /**Get the option value of the specified name as a string. The default
       value is returned if the option is not present.
      */
    PString GetOptionString(
      const PString & name,                   ///<  Option name
      const PString & dflt = PString::Empty() ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as a string.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionString(
      const PString & name,   ///<  Option name
      const PString & value   ///<  New value for option
    );
   
    /**Set the option value of the specified name as an octet array.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionOctets(
      const PString & name,       ///<  Option name
      const BYTE * data,          ///<  Octets in option
      PINDEX length               ///<  Number of octets
    );

    /**Get a copy of the list of media formats that have been registered.
      */
    static OpalMediaFormatList GetAllRegisteredMediaFormats();
    static void GetAllRegisteredMediaFormats(
      OpalMediaFormatList & copy    ///<  List to receive the copy of the master list
    );

    /**Set the options on the master format list entry.
       The media format must already be registered. Returns false if not.
      */
    static bool SetRegisteredMediaFormat(
      const OpalMediaFormat & mediaFormat  ///<  Media format to copy to master list
    );

    /**
      * Add a new option to this media format
      */
    bool AddOption(
      OpalMediaOption * option,
      PBoolean overwrite = PFalse
    );

    /**
      * Determine if media format has the specified option.
      */
    bool HasOption(const PString & name) const;

    /**
      * Get a pointer to the specified media format option.
      * Returns NULL if thee option does not exist.
      */
    OpalMediaOption * FindOption(
      const PString & name
    ) const;

    /** Returns PTrue if the media format is valid for the protocol specified
        This allow plugin codecs to customise which protocols they are valid for
        The default implementation returns true unless the protocol is H.323
        and the rtpEncodingName is NULL
      */
    bool IsValidForProtocol(const PString & protocol) const;

    time_t GetCodecVersionTime() const;
 
    static const PString & MediaPacketizationOption();
    static const PString & MediaPacketizationsOption();
	
	SIP_PYOBJECT __repr__();
%MethodCode
	sipRes = PyString_FromString(sipCpp->GetName());
%End

	int __nonzero__();
%MethodCode
	sipRes = sipCpp->IsValid();
%End

	int __contains__(const PString & name);
%MethodCode
	sipRes = (*a0 == sipCpp->GetName());
%End

	int __eq__(const PString & name);
%MethodCode
	sipRes = (*a0 == sipCpp->GetName());
%End

};

class OpalMediaFormatInternal : PObject
{
  public:
    OpalMediaFormatInternal(
      const char * fullName,
      const OpalMediaType & mediaType,
      RTP_DataFrame::PayloadTypes rtpPayloadType,
      const char * encodingName,
      PBoolean     needsJitter,
      unsigned bandwidth,
      PINDEX   frameSize,
      unsigned frameTime,
      unsigned clockRate,
      time_t timeStamp
    );

    const PCaselessString & GetName() const;

    virtual PObject * Clone() const;

    virtual bool IsValid() const;
    virtual bool IsTransportable() const;

    virtual PStringToString GetOptions() const;
    virtual bool GetOptionValue(const PString & name, PString & value) const;
    virtual bool SetOptionValue(const PString & name, const PString & value);
    virtual bool GetOptionBoolean(const PString & name, bool dflt) const;
    virtual bool SetOptionBoolean(const PString & name, bool value);
    virtual int GetOptionInteger(const PString & name, int dflt) const;
    virtual bool SetOptionInteger(const PString & name, int value);
    virtual double GetOptionReal(const PString & name, double dflt) const;
    virtual bool SetOptionReal(const PString & name, double value);
    virtual PINDEX GetOptionEnum(const PString & name, PINDEX dflt) const;
    virtual bool SetOptionEnum(const PString & name, PINDEX value);
    virtual PString GetOptionString(const PString & name, const PString & dflt) const;
    virtual bool SetOptionString(const PString & name, const PString & value);
    virtual bool GetOptionOctets(const PString & name, PBYTEArray & octets) const;
    virtual bool SetOptionOctets(const PString & name, const PBYTEArray & octets);
    virtual bool SetOptionOctets(const PString & name, const BYTE * data, PINDEX length);
    virtual bool AddOption(OpalMediaOption * option, PBoolean overwrite = false);
    virtual OpalMediaOption * FindOption(const PString & name) const;

    virtual bool ToNormalisedOptions();
    virtual bool ToCustomisedOptions();
    virtual bool Merge(const OpalMediaFormatInternal & mediaFormat);

    virtual bool ValidateMerge(const OpalMediaFormatInternal & mediaFormat) const;

    virtual bool IsValidForProtocol(const PString & protocol) const;
};

class OpalAudioFormat : OpalMediaFormat
{
  public:
    OpalAudioFormat(
      OpalMediaFormatInternal * info = NULL
    );
	
    OpalAudioFormat(
      const char * fullName,    ///<  Full name of media format
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      const char * encodingName,///<  RTP encoding name
      PINDEX   frameSize,       ///<  Size of frame in bytes (if applicable)
      unsigned frameTime,       ///<  Time for frame in RTP units (if applicable)
      unsigned rxFrames,        ///<  Maximum number of frames per packet we can receive
      unsigned txFrames,        ///<  Desired number of frames per packet we transmit
      unsigned maxFrames = 256, ///<  Maximum possible frames per packet
      unsigned clockRate = 8000, ///<  Clock Rate 
      time_t timeStamp = 0       ///<  timestamp (for versioning)
    );

    static const PString & RxFramesPerPacketOption();
    static const PString & TxFramesPerPacketOption();
    static const PString & MaxFramesPerPacketOption();
    static const PString & ChannelsOption();
};


%MappedType OpalMediaFormatList
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib/lists.h>
#include <opal/mediafmt.h>
%End

%ConvertFromTypeCode
    // Create the list.
    PyObject *l;
	SIP_SSIZE_T i = 0;

    if ((l = PyList_New(sipCpp->GetSize())) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to a
    // wrapped TYPE.
    for (OpalMediaFormatList::iterator iter = sipCpp->begin(); iter != sipCpp->end(); iter++)
    {
		OpalMediaFormat *fmt = new OpalMediaFormat(*iter);
        PyObject *pobj;

		// Get the Python wrapper for the Type instance, creating a new
		// one if necessary, and handle any ownership transfer.
		if ((pobj = sipConvertFromNewType(fmt, sipType_OpalMediaFormat, Py_None)) == NULL)
		{
			// There was an error so garbage collect the Python list.
			Py_DECREF(l);
			return NULL;
		}
		
		// Add the wrapper to the list.
        PyList_SET_ITEM(l, i++, pobj);
	}
	
	// Return the Python list.
    return l;
%End

%ConvertToTypeCode
	// Check if type is compatible
	if (sipIsErr == NULL)
	{
		// Must be any iterable
		PyObject *i = PyObject_GetIter(sipPy);
		bool iterable = (i != NULL);
		Py_XDECREF(i);
		return iterable;
	}

	// Iterate over the object
	PyObject *iterator = PyObject_GetIter(sipPy);
	PyObject *item;
	int state;
   
	OpalMediaFormatList *plist = new OpalMediaFormatList;
 
	while ((item = PyIter_Next(iterator)))
	{
	   if (!sipCanConvertToType(item, sipType_OpalMediaFormat, SIP_NOT_NONE))
	   {
		   PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to OpalMediaFormat");
		   *sipIsErr = 1;
		   break;
	   }
	   
	   //Python should retain ownership, as we are creating a copy of this object in our list.
	   OpalMediaFormat* p = reinterpret_cast<OpalMediaFormat*>(
			sipConvertToType(item, sipType_OpalMediaFormat, Py_None, SIP_NOT_NONE, &state, sipIsErr));

	   if (!*sipIsErr)
		   *plist+=*p;

	   sipReleaseType(p, sipType_OpalMediaFormat, state);
	   Py_DECREF(item);
	}

	Py_DECREF(iterator);

	if (*sipIsErr)
	{
	   delete plist;
	   return 0;
	}

	*sipCppPtr = plist;
	return sipGetState(sipTransferObj);
%End
};


class OpalVideoFormat : OpalMediaFormat
{
%TypeHeaderCode
#include <ptlib.h>
#include <opal/mediafmt.h>
%End

  public:
    OpalVideoFormat(
      const char * fullName,    ///<  Full name of media format
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      const char * encodingName,///<  RTP encoding name
      unsigned maxFrameWidth,   ///<  Width of video frame
      unsigned maxFrameHeight,  ///<  Height of video frame
      unsigned maxFrameRate,    ///<  Number of frames per second
      unsigned maxBitRate,      ///<  Maximum bits per second
      time_t timeStamp = 0      ///<  timestamp (for versioning)
    );

    static const PString & FrameWidthOption();
    static const PString & FrameHeightOption();
    static const PString & MinRxFrameWidthOption();
    static const PString & MinRxFrameHeightOption();
    static const PString & MaxRxFrameWidthOption();
    static const PString & MaxRxFrameHeightOption();
    static const PString & TemporalSpatialTradeOffOption();
    static const PString & TxKeyFramePeriodOption();
    static const PString & RateControlEnableOption();
    static const PString & RateControllerOption();

    /**The "role" of the content in the video stream based on this media
       format. This is based on RFC4796 and H.239 semantics and is an
       enumeration consisting of:
          Value          H.239                RFC4796
          NoRole         Not used             (not populated)
          Presentation   Presentation (0x01)  slides
          Main           Live (0x02)          main
          Speaker        Live (0x02)          speaker
          SignLanguage   Live (0x02)          sl
      */
    enum ContentRole {
      eNoRole,
      ePresentation,
      eMainRole,
      eSpeaker,
      eSignLanguage,
      eNumRoles
    };

    enum 
	{ 
		ContentRoleMask = 15 
	};
    
	static unsigned ContentRoleBit(ContentRole contentRole);
    static const PString & ContentRoleOption();
    static const PString & ContentRoleMaskOption();
};