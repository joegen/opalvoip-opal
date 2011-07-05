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

/**This class describes the interface to a capability of the endpoint, usually
   a codec, used to transfer data via the logical channels opened and managed
   by the H323 control channel.

   Note that this is not an instance of the codec itself. Merely the
   description of that codec. There is typically only one instance of this
   class contained in the capability tables of the endpoint. There may be
   several instances of the actualy codec managing the conversion of an
   individual stream of data.

   An application may create a descendent off this class and override
   functions as required for describing a codec that it implements.
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <h323/h323caps.h>
%End

class H323Capability : PObject /Abstract/
{

  public:
  /**@name Construction */
  //@{
    /**Create a new capability specification.
     */
    H323Capability();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;

  /**@name Identification functions */
  //@{
    enum MainTypes {
      /// Audio codec capability
      e_Audio,
      /// Video codec capability
      e_Video,
      /// Arbitrary data capability
      e_Data,
      /// User Input capability
      e_UserInput,
      /// Generic Control
      e_GenericControl,
      /// Count of main types
      e_NumMainTypes
    };

    /**Get the main type of the capability.

       This function is overridden by one of the three main sub-classes off
       which real capabilities would be descendend.
     */
    virtual MainTypes GetMainType() const = 0;

    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.
     */
    virtual unsigned  GetSubType()  const = 0;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const = 0;
  //@}

  /**@name Operations */
  //@{
    /**Create an H323Capability descendant given a string name.
       This uses the registration system to create the capability.
      */
    static H323Capability * Create(
      const PString & name     ///<  Name of capability
    );

    enum {
      DefaultAudioSessionID = 1,
      DefaultVideoSessionID = 2,
      DefaultDataSessionID  = 3,
      DefaultH224SessionID  = 4
    };

    /**Get the default RTP session.
       This function gets the default RTP session ID for the capability
       type. For example audio capabilities return the value
       H323Capability::DefaultAudioSessionID etc.

       The default behaviour returns zero, indicating it is not an RTP
       based capability.
      */
    virtual unsigned GetDefaultSessionID() const;

    /**Set the maximum size (in frames) of data that will be transmitted in a
       single PDU.

       This will also be the desired number that will be sent by most codec
       implemetations.

       The default behaviour does nothing.
     */
    virtual void SetTxFramesInPacket(
      unsigned frames   ///<  Number of frames per packet
    );

    /**Get the maximum size (in frames) of data that will be transmitted in a single PDU.

       The default behaviour returns the value 1.
     */
    virtual unsigned GetTxFramesInPacket() const;

    /**Get the maximum size (in frames) of data that can be received in a single PDU.

       The default behaviour returns the value 1.
     */
    virtual unsigned GetRxFramesInPacket() const;

  /**@name Protocol manipulation */
  //@{
    enum CommandType {
      e_TCS,
      e_OLC,
      e_ReqMode
    };

    /**Validate that the capability is usable given the connection.
       This checks agains the negotiated protocol version number and remote
       application to determine if this capability should be used in TCS or
       OLC pdus.

       The default behaviour returns PTrue.
      */
    virtual PBoolean IsUsable(
      const H323Connection & connection
    ) const;
  //@}

  /**@name Member variable access */
  //@{
    enum CapabilityDirection {
      e_Unknown,
      e_Receive,
      e_Transmit,
      e_ReceiveAndTransmit,
      e_NoDirection,
      NumCapabilityDirections
    };

    /**Get the direction for this capability.
      */ 
    CapabilityDirection GetCapabilityDirection() const;

    /**Set the direction for this capability.
      */
    void SetCapabilityDirection(
      CapabilityDirection dir   ///<  New direction code
    );

    /// Get unique capability number.
    unsigned GetCapabilityNumber() const;

    /// Set unique capability number.
    void SetCapabilityNumber(unsigned num);

    /**Get media format of the media data this class represents.
      */
    OpalMediaFormat GetMediaFormat() const;

    /** Set media format option for the media data this class represents.
      */
    bool UpdateMediaFormat(
      const OpalMediaFormat & format
    );

    /// Get the payload type for the capaibility
    RTP_DataFrame::PayloadTypes GetPayloadType() const;

    /// Set the payload type for the capaibility
    void SetPayloadType(RTP_DataFrame::PayloadTypes pt);

    /// Attach a QoS specification to this channel
    virtual void AttachQoS(RTP_QOS *);
  //@}

};



/**This class describes the interface to a non-standard codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   It is expected that an application makes a descendent off
   H323NonStandardAudioCapability or H323NonStandardVideoCapability which
   multiply inherit from this class.
 */
class H323NonStandardCapabilityInfo /Abstract/
{
  public:

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardCapabilityInfo(
      const BYTE * dataBlock,         ///<  Non-Standard data for codec type
      PINDEX dataSize,                ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardCapabilityInfo(
      const PString & oid,
      const BYTE * dataBlock,         ///<  Non-Standard data for codec type
      PINDEX dataSize,                ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,
      PINDEX comparisonLength = P_MAX_INDEX
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardCapabilityInfo(
      BYTE country,                  ///<  t35 information
      BYTE extension,                ///<  t35 information
      WORD maufacturer,              ///<  t35 information
      const BYTE * dataBlock,         ///<  Non-Standard data for codec type
      PINDEX dataSize,                ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Destroy the capability information
     */
    virtual ~H323NonStandardCapabilityInfo();
};

/**This class describes the interface to a generic codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   It is expected that an application makes a descendent off
   H323GenericAudioCapability or H323GenericVideoCapability which
   multiply inherit from this class.
 */

class H323GenericCapabilityInfo
{
  public:
    H323GenericCapabilityInfo(
      const PString & id,     ///< generic codec identifier
      unsigned maxBitRate = 0   ///< maxBitRate parameter for the GenericCapability
    );

    virtual ~H323GenericCapabilityInfo();

};

/**This class describes the interface to a codec that has channels based on
   the RTP protocol.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323RealTimeCapability : H323Capability /Abstract/
{

  public:
    H323RealTimeCapability();
    H323RealTimeCapability(const H323RealTimeCapability &rtc);
    virtual ~H323RealTimeCapability();
    void AttachQoS(RTP_QOS * _rtpqos);

  //@}
};


/**This class describes the interface to an audio codec used to transfer data
   via the logical channels opened and managed by the H323 control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323AudioCapability : H323RealTimeCapability
{

  public:
  /**@name Construction */
  //@{
    /**Create an audio based capability.
      */
    H323AudioCapability();
  //@}

  /**@name Identification functions */
  //@{
    /**Get the main type of the capability.
       Always returns e_Audio.
     */
    virtual MainTypes GetMainType() const;
  //@}

  /**@name Operations */
  //@{
    /**Get the default RTP session.
       This function gets the default RTP session ID for the capability
       type. For example audio capabilities return the value
       RTP_Session::DefaultAudioSessionID etc.

       The default behaviour returns zero, indicating it is not an RTP
       based capability.
      */
    virtual unsigned GetDefaultSessionID() const;

    /**Set the maximum size (in frames) of data that will be transmitted in a
       single PDU.

       This will also be the desired number that will be sent by most codec
       implemetations.

       The default behaviour sets the txFramesInPacket variable.
     */
    virtual void SetTxFramesInPacket(
      unsigned frames   ///<  Number of frames per packet
    );

    /**Get the maximum size (in frames) of data that will be transmitted in a
       single PDU.

       The default behaviour sends the txFramesInPacket variable.
     */
    virtual unsigned GetTxFramesInPacket() const;

    /**Get the maximum size (in frames) of data that can be received in a
       single PDU.

       The default behaviour sends the rxFramesInPacket variable.
     */
    virtual unsigned GetRxFramesInPacket() const;
  //@}

};


/**This class describes the interface to a non-standard audio codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323NonStandardAudioCapability : H323AudioCapability,
                                       H323NonStandardCapabilityInfo
{

  public:

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardAudioCapability(
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
     );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardAudioCapability(
      const PString & oid,            ///<  OID for indentification of codec
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardAudioCapability(
      BYTE country,                   ///<  t35 information
      BYTE extension,                 ///<  t35 information
      WORD maufacturer,               ///<  t35 information
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_AudioCapability::e_nonStandard.
     */
    virtual unsigned GetSubType() const;
  //@}
};

/**This class describes the interface to a generic audio codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323GenericAudioCapability : H323AudioCapability,
                                   H323GenericCapabilityInfo
{

  public:
  /**@name Construction */
  //@{
    /**Create a new set of information about a non-standard codec.
      */
    H323GenericAudioCapability(
      const PString & capabilityId,    ///< generic codec identifier
      PINDEX maxBitRate = 0               ///< maxBitRate parameter for the GenericCapability
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_AudioCapability::e_genericCapability.
     */
    virtual unsigned GetSubType() const;
  //@}

};



/**This class describes the interface to a video codec used to transfer data
   via the logical channels opened and managed by the H323 control channel.

   An application may create a descendent off this class and override
   functions as required for descibing a codec.
 */
class H323VideoCapability : H323RealTimeCapability /Abstract/
{
  public:
  /**@name Identification functions */
  //@{
    /**Get the main type of the capability.
       Always returns e_Video.
     */
    virtual MainTypes GetMainType() const;
  //@}

  /**@name Operations */
  //@{
    /**Get the default RTP session.
       This function gets the default RTP session ID for the capability
       type. For example audio capabilities return the value
       RTP_Session::DefaultAudioSessionID etc.

       The default behaviour returns zero, indicating it is not an RTP
       based capability.
      */
    virtual unsigned GetDefaultSessionID() const;
  //@}

};


/**This class describes the interface to a non-standard video codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323NonStandardVideoCapability : H323VideoCapability,
                                       H323NonStandardCapabilityInfo
{

  public:
  /**@name Construction */
  //@{
    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardVideoCapability(
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardVideoCapability(
      const PString & oid,            ///<  OID for indentification of codec
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardVideoCapability(
      BYTE country,                   ///<  t35 information
      BYTE extension,                 ///<  t35 information
      WORD maufacturer,               ///<  t35 information
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns one of the four possible combinations of mode and speed
       using the enum values of the protocol ASN H245_AudioCapability class.
     */
    virtual unsigned GetSubType() const;
  //@}

};

/**This class describes the interface to a generic video codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323GenericVideoCapability : H323VideoCapability,
                                   H323GenericCapabilityInfo
{
  public:
  /**@name Construction */
  //@{
    /**Create a new set of information about a non-standard codec.
      */
    H323GenericVideoCapability(
      const PString & capabilityId,    ///< generic codec identifier (OID)
      PINDEX maxBitRate = 0           ///< maxBitRate parameter for the GenericCapability
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_VideoCapability::e_genericCapability.
     */
    virtual unsigned GetSubType() const;
  //@}

};


/**This class descibes an extended ideo capability, as used in H.239.
  */
class H323ExtendedVideoCapability : H323GenericVideoCapability
{

  public:
  /**@name Construction */
  //@{
    /**Create a new Extended Video capability.
      */
    H323ExtendedVideoCapability(
      const PString & identifier   ///< generic codec identifier (OID)
    );
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_VideoCapability::e_extendedVideoCapability.
     */
    virtual unsigned GetSubType() const;
  //@}
   

};


///////////////////////////////////////////////////////////////////////////////

/**This class describes a generic control capability, as used in H.239
 */
class H323GenericControlCapability : H323Capability,
                                     H323GenericCapabilityInfo
{

  public:
  /**@name Construction */
  //@{
    /**Create a new data capability.
      */
    H323GenericControlCapability(
      const PString & identifier  ///< Indentifer (OID) for generic control
    );
  //@}

  /**@name Identification functions */
  //@{
    /**Get the main type of the capability.
       Always returns e_Data.
     */
    virtual MainTypes GetMainType() const;

    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_VideoCapability::e_extendedVideoCapability.
     */
    virtual unsigned GetSubType() const;
  //@}

};


///////////////////////////////////////////////////////////////////////////////

class H323H239VideoCapability : H323ExtendedVideoCapability
{

  public:
  /**@name Construction */
  //@{
    /**Create a new Extended Video capability.
      */
    H323H239VideoCapability(
      const OpalMediaFormat & videoFormat
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;

    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

};


///////////////////////////////////////////////////////////////////////////////

class H323H239ControlCapability : H323GenericControlCapability
{

  public:
  /**@name Construction */
  //@{
    /**Create a new Extended Video capability.
      */
    H323H239ControlCapability();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}
};


/**This class describes the interface to a data channel used to transfer data
   via the logical channels opened and managed by the H323 control channel.

   An application may create a descendent off this class and override
   functions as required for descibing a codec.
 */
class H323DataCapability : H323Capability /Abstract/
{

  public:
  /**@name Construction */
  //@{
    /**Create a new data capability.
      */
    H323DataCapability(
      unsigned maxBitRate = 0  ///<  Maximum bit rate for data in 100's b/s
    );
  //@}

  /**@name Identification functions */
  //@{
    /**Get the main type of the capability.
       Always returns e_Data.
     */
    virtual MainTypes GetMainType() const;
  //@}

  /**@name Operations */
  //@{
    /**Get the default RTP session.
       This function gets the default RTP session ID for the capability
       type. For example audio capabilities return the value
       RTP_Session::DefaultAudioSessionID etc.

       The default behaviour returns 3, indicating a data session.
      */
    virtual unsigned GetDefaultSessionID() const;
  //@}


};


/**This class describes the interface to a non-standard data codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323NonStandardDataCapability : H323DataCapability,
                                      H323NonStandardCapabilityInfo /Abstract/
{
  public:
  /**@name Construction */
  //@{
    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardDataCapability(
      unsigned maxBitRate,            ///<  Maximum bit rate for data in 100's b/s
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardDataCapability(
      unsigned maxBitRate,            ///<  Maximum bit rate for data in 100's b/s
      const PString & oid,            ///<  OID for indentification of codec
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardDataCapability(
      unsigned maxBitRate,            ///<  Maximum bit rate for data in 100's b/s
      BYTE country,                   ///<  t35 information
      BYTE extension,                 ///<  t35 information
      WORD maufacturer,               ///<  t35 information
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns one of the four possible combinations of mode and speed
       using the enum values of the protocol ASN H245_AudioCapability class.
     */
    virtual unsigned GetSubType() const;
  //@}

};


///////////////////////////////////////////////////////////////////////////////
// Known audio codecs

/**This class describes the G.711 codec capability.
 */
class H323_G711Capability : H323AudioCapability
{

  public:
    /// Specific G.711 encoding algorithm.
    enum Mode {
      /// European standard
      ALaw,
      /// American standard
      muLaw
    };
    /// Specific G.711 encoding bit rates.
    enum Speed {
      /// European standard
      At64k,
      /// American standard
      At56k
    };

  /**@name Construction */
  //@{
    /**Create a new G.711 capability.
     */
    H323_G711Capability(
      Mode mode = muLaw,    ///<  Type of encoding.
      Speed speed = At64k   ///<  Encoding bit rate.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns one of the four possible combinations of mode and speed
       using the enum values of the protocol ASN H245_AudioCapability class.
     */
    virtual unsigned GetSubType() const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

};

///////////////////////////////////////////////////////////////////////////////

/**This class describes the UserInput psuedo-channel.
 */
class H323_UserInputCapability : H323Capability
{

  public:
  /**@name Construction */
  //@{
    enum SubTypes {
      BasicString,
      IA5String,
      GeneralString,
      SignalToneH245,
      HookFlashH245,
      SignalToneRFC2833,
      NumSubTypes
    };
	
    static const char * GetSubTypeName(SubTypes subType);

    /**Create the capability for User Input.
       The subType parameter is a value from the enum
       H245_UserInputCapability::Choices.
      */
    H323_UserInputCapability(
      SubTypes subType
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the main type of the capability.

       This function is overridden by one of the three main sub-classes off
       which real capabilities would be descendend.
     */
    virtual MainTypes GetMainType() const;

    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.
     */
    virtual unsigned  GetSubType()  const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}


    /**Validate that the capability is usable given the connection.
       This checks agains the negotiated protocol version number and remote
       application to determine if this capability should be used in TCS or
       OLC pdus.

       The default behaviour will check for early versions and return PFalse
       for RFC2833 mode.
      */
    virtual PBoolean IsUsable(
      const H323Connection & connection
    ) const;
  //@}

    static void AddAllCapabilities(
      H323Capabilities & capabilities,        ///<  Table to add capabilities to
      PINDEX descriptorNum,   ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,     ///<  The member of the SimultaneousCapabilitySet to add
      bool includeRFC2833 = true
    );
};


/**This class contains all of the capabilities and their combinations.
  */
class H323Capabilities : PObject
{
  public:
  /**@name Construction */
  //@{
    /**Construct an empty capability set.
      */
    H323Capabilities();

     /**Construct a copy of a capability set.
       Note this will completely duplicate the set by making clones of every
       capability in the original set.
      */
    H323Capabilities(
      const H323Capabilities & original ///<  Original capabilities to duplicate
    );

  //@}
  /**@name Operations */
  //@{
    /**Get the number of capabilities in the set.
      */
    PINDEX GetSize() const;

    /**Get the capability at the specified index.
      */
    H323Capability & operator[](PINDEX i) const;

    /**Set the capability descriptor lists. This is three tier set of
       codecs. The top most level is a list of particular capabilities. Each
       of these consists of a list of alternatives that can operate
       simultaneously. The lowest level is a list of codecs that cannot
       operate together. See H323 section 6.2.8.1 and H245 section 7.2 for
       details.

       If descriptorNum is P_MAX_INDEX, the the next available index in the
       array of descriptors is used. Similarly if simultaneous is P_MAX_INDEX
       the the next available SimultaneousCapabilitySet is used. The return
       value is the index used for the new entry. Note if both are P_MAX_INDEX
       then the return value is the descriptor index as the simultaneous index
       must be zero.

       Note that the capability specified here is automatically added to the
       capability table using the AddCapability() function. A specific
       instance of a capability is only ever added once, so multiple
       SetCapability() calls with the same H323Capability pointer will only
       add that capability once.
     */
    PINDEX SetCapability(
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,  ///<  The member of the SimultaneousCapabilitySet to add
      H323Capability * cap  ///<  New capability specification
    );

    /**Add matching capability to media format.
      */
    PINDEX AddMediaFormat(
      PINDEX descriptorNum,    ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,     ///<  The member of the SimultaneousCapabilitySet to add
      const OpalMediaFormat & mediaFormat ///<  Media format to add.
    );

    /**Add all matching capabilities to descriptor lists.
       All capabilities that match the specified name are added as in the other
       form of the SetCapability() function.
      */
    PINDEX AddAllCapabilities(
      PINDEX descriptorNum,    ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,     ///<  The member of the SimultaneousCapabilitySet to add
      const PString & name,    ///<  New capabilities name, if using "known" one.
      PBoolean exact = PFalse       ///<  Capability name must be exact match
    );

    // this function is retained for backwards compatibility
    PINDEX AddAllCapabilities(
      const H323EndPoint &,    ///<  The endpoint adding the capabilities.
      PINDEX descriptorNum,    ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,     ///<  The member of the SimultaneousCapabilitySet to add
      const PString & name,    ///<  New capabilities name, if using "known" one.
      PBoolean exact = PFalse       ///<  Capability name must be exact match
    )
;

    /**Add a codec to the capabilities table. This will assure that the
       assignedCapabilityNumber field in the capability is unique for all
       capabilities installed on this set.

       If the specific instance of the capability is already in the table, it
       is not added again. Ther can be multiple instances of the same
       capability class however.
     */
    void Add(
      H323Capability * capability   ///<  New capability specification
    );

    /**Copy a codec to the capabilities table. This will make a clone of the
       capability and assure that the assignedCapabilityNumber field in the
       capability is unique for all capabilities installed on this set.

       Returns the copy that is put in the table.
     */
    H323Capability * Copy(
      const H323Capability & capability   ///<  New capability specification
    );

    /**Remove a capability from the table. Note that the the parameter must be
       the actual instance of the capability in the table. The instance is
       deleted when removed from the table.
      */
    void Remove(
      H323Capability * capability   ///<  Existing capability specification
    );

    /**Remove all capabilities matching the string. This uses FindCapability()
       to locate the first capability whose format name does a partial match
       for the argument.
      */
    void Remove(
      const PString & formatName   ///<  Format name to search for.
    );

    /**Remove all capabilities matching any of the strings provided. This
       simply calls Remove() for each string in the list.
      */
    void Remove(
      const PStringArray & formatNames  ///<  Array of format names to remove
    );

    /**Remove all of the capabilities.
      */
    void RemoveAll();

    /**Find the capability given the capability number. This number is
       guarenteed to be unique for a give capability table. Note that is may
       not be the same as the index into the table.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      unsigned capabilityNumber
    ) const /PyName=FindCapabilityByNumber/;

    /**Find the capability given the capability format name string. This does
       a partial match for the supplied argument. If the argument matches a
       substring of the actual capabilities name, then it is returned. For
       example "GSM" or "0610" will match "GSM 0610". Note case is not
       significant.

       The user should be carefull of using short strings such as "G"!

       The direction parameter can further refine the search for specific
       receive or transmit capabilities. The default value of e_Unknown will
       wildcard that field.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const PString & formatName, ///<  Wildcard format name to search for
      H323Capability::CapabilityDirection direction = H323Capability::e_Unknown,
            ///<  Optional direction to include into search criteria
      PBoolean exact = PFalse       ///<  Capability name must be exact match
    ) const;

    /**Find the first capability in the table of the specified direction.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      H323Capability::CapabilityDirection direction ///<  Direction to search for
    ) const;

    /**Find the capability given the capability. This does a value compare of
       the two capabilities. Usually this means the mainType and subType are
       the same.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const H323Capability & capability ///<  Capability to search for
    ) const;

    /**Find the capability given the type codecs.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      H323Capability::MainTypes mainType, ///<  Main type to find
      unsigned subType = UINT_MAX         ///<  Sub-type to find (UINT_MAX=ignore)
    ) const;

     /**Merge the capabilities into this set.
      */
    PBoolean Merge(
      const H323Capabilities & newCaps
    );

    /**Change the order of capabilities in the table to the order specified.
       Note that this does not change the unique capability numbers assigned
       when the capability is first added to the set.

       The string matching rules are as for the FindCapability() function.
      */
    void Reorder(
      const PStringArray & preferenceOrder  ///<  New order
    );

    /**Test if the capability is allowed.
      */
    PBoolean IsAllowed(
      const H323Capability & capability
    );

    /**Test if the capability is allowed.
      */
    PBoolean IsAllowed(
      unsigned capabilityNumber
    );

    /**Test if the capabilities are an allowed combination.
      */
    PBoolean IsAllowed(
      const H323Capability & capability1,
      const H323Capability & capability2
    );

    /**Test if the capabilities are an allowed combination.
      */
    PBoolean IsAllowed(
      unsigned capabilityNumber1,
      unsigned capabilityNumber2
    );

    /**Get the list of capabilities as a list of media formats.
      */
    OpalMediaFormatList GetMediaFormats() const;

    const PStringSet & GetMediaPacketizations() const;
  //@}

};


