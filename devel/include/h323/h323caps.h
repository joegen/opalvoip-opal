/*
 * h323caps.h
 *
 * H.323 protocol handler
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
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: h323caps.h,v $
 * Revision 2.22  2007/09/19 10:43:00  csoutheren
 * Exposed G.7231 capability class
 * Added macros to create empty transcoders and capabilities
 *
 * Revision 2.21  2007/09/12 04:19:53  rjongbloed
 * CHanges to avoid creation of long duration OpalMediaFormat instances, eg in
 *   the plug in capabilities, that then do not get updated values from the master
 *   list, or worse from the user modified master list, causing much confusion.
 *
 * Revision 2.20  2007/06/22 06:29:13  rjongbloed
 * Fixed GCC warnings.
 *
 * Revision 2.19  2007/06/22 05:49:12  rjongbloed
 * Major codec API update:
 *   Automatically map OpalMediaOptions to SIP/SDP FMTP parameters.
 *   Automatically map OpalMediaOptions to H.245 Generic Capability parameters.
 *   Largely removed need to distinguish between SIP and H.323 codecs.
 *   New mechanism for setting OpalMediaOptions from within a plug in.
 * Fixed removal of H.323 capabilities just because media format name is a
 *   substring of capability name.
 * Fixed inadequacies in H.245 Generic Capabilities (must be able to
 *   distinguish between TCS, OLC and ReqMode).
 *
 * Revision 2.18  2007/04/10 05:15:53  rjongbloed
 * Fixed issue with use of static C string variables in DLL environment,
 *   must use functional interface for correct initialisation.
 *
 * Revision 2.17  2006/08/11 07:52:01  csoutheren
 * Fix problem with media format factory in VC 2005
 * Fixing problems with Speex codec
 * Remove non-portable usages of PFactory code
 *
 * Revision 2.16  2006/07/24 14:03:38  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.15.4.3  2006/04/10 06:24:29  csoutheren
 * Backport from CVS head up to Plugin_Merge3
 *
 * Revision 2.15.4.2  2006/04/06 01:21:17  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 2.15.4.1  2006/03/23 07:55:18  csoutheren
 * Audio plugin H.323 capability merging completed.
 * GSM, LBC, G.711 working. Speex and LPC-10 are not
 *
 * Revision 2.15  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.14  2005/02/21 12:19:45  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.13  2004/09/01 12:21:27  rjongbloed
 * Added initialisation of H323EndPoints capability table to be all codecs so can
 *   correctly build remote caps from fqast connect params. This had knock on effect
 *   with const keywords added in numerous places.
 *
 * Revision 2.12  2004/02/19 10:46:43  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.11  2002/11/10 11:33:16  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.10  2002/09/16 02:52:33  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.9  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.8  2002/07/01 04:56:29  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.7  2002/02/19 07:44:13  robertj
 * Added function to set teh RTP payload type for the capability.
 *
 * Revision 2.6  2002/02/11 09:32:11  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.5  2002/01/22 04:59:04  robertj
 * Update from OpenH323, RFC2833 support
 *
 * Revision 2.4  2002/01/14 06:35:56  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.3  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.2  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/01 05:12:04  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 * Added "known" codecs capability clases.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.40  2003/10/27 06:03:39  csoutheren
 * Added support for QoS
 *   Thanks to Henry Harrison of AliceStreet
 *
 * Revision 1.39  2003/06/06 02:13:10  rjongbloed
 * Changed non-standard capability semantics so can use C style strings as
 *   the embedded data block (ie automatically call strlen)
 *
 * Revision 1.38  2003/04/28 07:00:00  robertj
 * Fixed problem with compiler(s) not correctly initialising static globals
 *
 * Revision 1.37  2003/04/27 23:49:21  craigs
 * Fixed some comments and made list of registered codecs
 * available outside h323caps.cxx
 *
 * Revision 1.36  2002/11/09 04:24:01  robertj
 * Fixed minor documentation errors.
 *
 * Revision 1.35  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.34  2002/09/03 06:19:36  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 *
 * Revision 1.33  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.32  2002/05/29 03:55:17  robertj
 * Added protocol version number checking infrastructure, primarily to improve
 *   interoperability with stacks that are unforgiving of new features.
 *
 * Revision 1.31  2002/05/10 05:44:50  robertj
 * Added the max bit rate field to the data channel capability class.
 *
 * Revision 1.30  2002/01/22 06:25:02  robertj
 * Moved payload type to ancestor so any capability can adjust it on logical channel.
 *
 * Revision 1.29  2002/01/17 07:04:57  robertj
 * Added support for RFC2833 embedded DTMF in the RTP stream.
 *
 * Revision 1.28  2002/01/16 05:37:41  robertj
 * Added missing mode change functions on non standard capabilities.
 *
 * Revision 1.27  2002/01/09 00:21:36  robertj
 * Changes to support outgoing H.245 RequstModeChange.
 *
 * Revision 1.26  2001/12/22 01:44:05  robertj
 * Added more support for H.245 RequestMode operation.
 *
 * Revision 1.25  2001/10/24 01:20:34  robertj
 * Added code to help with static linking of H323Capability names database.
 *
 * Revision 1.24  2001/09/21 02:48:51  robertj
 * Added default implementation for PDU encode/decode for codecs
 *   that have simple integer as frames per packet.
 *
 * Revision 1.23  2001/09/11 10:21:40  robertj
 * Added direction field to capabilities, thanks Nick Hoath.
 *
 * Revision 1.22  2001/07/19 09:50:40  robertj
 * Added code for default session ID on data channel being three.
 *
 * Revision 1.21  2001/07/17 04:44:29  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 * Revision 1.20  2001/05/21 07:20:47  robertj
 * Removed redundent class name in declaration.
 *
 * Revision 1.19  2001/05/14 05:56:26  robertj
 * Added H323 capability registration system so can add capabilities by
 *   string name instead of having to instantiate explicit classes.
 *
 * Revision 1.18  2001/05/02 16:22:21  rogerh
 * Add IsAllow() for a single capability to check if it is in the
 * capabilities set. This fixes the bug where OpenH323 would accept
 * incoming H261 video even when told not to accept it.
 *
 * Revision 1.17  2001/03/16 23:00:21  robertj
 * Improved validation of codec selection against capability set, thanks Chris Purvis.
 *
 * Revision 1.16  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.15  2001/01/25 07:27:14  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.14  2001/01/09 23:05:22  robertj
 * Fixed inability to have 2 non standard codecs in capability table.
 *
 * Revision 1.13  2000/10/16 08:49:30  robertj
 * Added single function to add all UserInput capability types.
 *
 * Revision 1.12  2000/08/23 14:23:11  craigs
 * Added prototype support for Microsoft GSM codec
 *
 * Revision 1.11  2000/07/13 12:25:47  robertj
 * Fixed problems with fast start frames per packet adjustment.
 *
 * Revision 1.10  2000/07/10 16:01:50  robertj
 * Started fixing capability set merging, still more to do.
 *
 * Revision 1.9  2000/07/04 01:16:49  robertj
 * Added check for capability allowed in "combinations" set, still needs more done yet.
 *
 * Revision 1.8  2000/06/03 03:16:47  robertj
 * Fixed using the wrong capability table (should be connections) for some operations.
 *
 * Revision 1.7  2000/05/23 11:32:27  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H323Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.6  2000/05/18 11:53:34  robertj
 * Changes to support doc++ documentation generation.
 *
 * Revision 1.5  2000/05/10 04:05:26  robertj
 * Changed capabilities so has a function to get name of codec, instead of relying on PrintOn.
 *
 * Revision 1.4  2000/05/02 04:32:24  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.3  2000/04/05 19:01:12  robertj
 * Added function so can change desired transmit packet size.
 *
 * Revision 1.2  2000/03/21 03:06:47  robertj
 * Changes to make RTP TX of exact numbers of frames in some codecs.
 *
 * Revision 1.1  1999/12/23 23:02:35  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 */

#ifndef __OPAL_H323CAPS_H
#define __OPAL_H323CAPS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/mediafmt.h>
#include <h323/channels.h>


/* The following classes have forward references to avoid including the VERY
   large header files for H225 and H245. If an application requires access
   to the protocol classes they can include them, but for simple usage their
   inclusion can be avoided.
 */
class PASN_Choice;
class H245_Capability;
class H245_DataType;
class H245_ModeElement;
class H245_AudioCapability;
class H245_AudioMode;
class H245_VideoCapability;
class H245_VideoMode;
class H245_DataApplicationCapability;
class H245_DataMode;
class H245_DataProtocolCapability;
class H245_H2250LogicalChannelParameters;
class H245_TerminalCapabilitySet;
class H245_NonStandardParameter;
class H323Connection;
class H323Capabilities;
class H245_CapabilityIdentifier;
class H245_GenericCapability;
class H245_GenericParameter;

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
class H323Capability : public PObject
{
  PCLASSINFO(H323Capability, PObject);

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

    /**Print out the object to the stream, virtual version of << operator.
     */
    void PrintOn(ostream & strm) const;
  //@}

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

    /**Create the channel instance, allocating resources as required.
       This creates a logical channel object appropriate for the parameters
       provided. Not if param is NULL, sessionID must be provided, otherwise
       this is taken from the fields in param.
     */
    virtual H323Channel * CreateChannel(
      H323Connection & connection,    ///<  Owner connection for channel
      H323Channel::Directions dir,    ///<  Direction of channel
      unsigned sessionID,             ///<  Session ID for RTP channel
      const H245_H2250LogicalChannelParameters * param
                                      ///<  Parameters for channel
    ) const = 0;
  //@}

  /**@name Protocol manipulation */
  //@{
    enum CommandType {
      e_TCS,
      e_OLC,
      e_ReqMode
    };

    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const = 0;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const = 0;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const = 0;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns FALSE then the received PDU codec description
       is not supported, so will be ignored.
       
         The default behaviour sets the capabilityDirection member variable
         from the PDU and then returns TRUE. Note that this means it is very
         important to call the ancestor function when overriding.
     */
    virtual BOOL OnReceivedPDU(
      const H245_Capability & pdu ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      BOOL receiver               ///<  Is receiver OLC
    ) = 0;

    /**Compare the PDU part of the capability.
      */
    virtual BOOL IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;

    /**Validate that the capability is usable given the connection.
       This checks agains the negotiated protocol version number and remote
       application to determine if this capability should be used in TCS or
       OLC pdus.

       The default behaviour returns TRUE.
      */
    virtual BOOL IsUsable(
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
    CapabilityDirection GetCapabilityDirection() const { return capabilityDirection; }

    /**Set the direction for this capability.
      */
    void SetCapabilityDirection(
      CapabilityDirection dir   ///<  New direction code
    ) { capabilityDirection = dir; }

    /// Get unique capability number.
    unsigned GetCapabilityNumber() const { return assignedCapabilityNumber; }

    /// Set unique capability number.
    void SetCapabilityNumber(unsigned num) { assignedCapabilityNumber = num; }

    /**Get media format of the media data this class represents.
      */
    OpalMediaFormat GetMediaFormat() const;

    /// Get the payload type for the capaibility
    RTP_DataFrame::PayloadTypes GetPayloadType() const { return rtpPayloadType; }

    /// Set the payload type for the capaibility
    void SetPayloadType(RTP_DataFrame::PayloadTypes pt) { rtpPayloadType = pt; }

    /// Attach a QoS specification to this channel
    virtual void AttachQoS(RTP_QOS *) { }
  //@}

#if PTRACING
    friend ostream & operator<<(ostream & o , MainTypes t);
    friend ostream & operator<<(ostream & o , CapabilityDirection d);
#endif

  protected:
    OpalMediaFormat & GetWritableMediaFormat();

    unsigned            assignedCapabilityNumber;  /// Unique ID assigned to capability
    CapabilityDirection capabilityDirection;
    RTP_DataFrame::PayloadTypes rtpPayloadType;

  private:
    OpalMediaFormat     mediaFormat;
};



/**This class describes the interface to a non-standard codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   It is expected that an application makes a descendent off
   H323NonStandardAudioCapability or H323NonStandardVideoCapability which
   multiply inherit from this class.
 */
class H323NonStandardCapabilityInfo
{
  public:
    typedef PObject::Comparison (*CompareFuncType)(struct PluginCodec_H323NonStandardCodecData *);

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardCapabilityInfo(
      CompareFuncType compareFunc,
      const BYTE * dataBlock,         ///< Non-Standard data for codec type
      PINDEX dataSize                 ///< Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
    );

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

    /**This function gets the non-standard data field.

       The default behaviour sets data to fixedData.
      */
    virtual BOOL OnSendingPDU(
      PBYTEArray & data  ///<  Data field in PDU to send
    ) const;

    /**This function validates and uses the non-standard data field.

       The default behaviour returns TRUE if data is equal to fixedData.
      */
    virtual BOOL OnReceivedPDU(
      const PBYTEArray & data  ///<  Data field in PDU received
    );

    BOOL IsMatch(const H245_NonStandardParameter & param) const;

    PObject::Comparison CompareParam(
      const H245_NonStandardParameter & param
    ) const;

  protected:
    BOOL OnSendingNonStandardPDU(
      PASN_Choice & pdu,
      unsigned nonStandardTag
    ) const;
    BOOL OnReceivedNonStandardPDU(
      const PASN_Choice & pdu,
      unsigned nonStandardTag
    );

    PObject::Comparison CompareInfo(
      const H323NonStandardCapabilityInfo & obj
    ) const;
    PObject::Comparison CompareData(
      const PBYTEArray & data  ///<  Data field in PDU received
    ) const;

    PString    oid;
    BYTE       t35CountryCode;
    BYTE       t35Extension;
    WORD       manufacturerCode;
    PBYTEArray nonStandardData;
    PINDEX     comparisonOffset;
    PINDEX     comparisonLength;
    CompareFuncType compareFunc;
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
      PINDEX maxBitRate = 0   ///< maxBitRate parameter for the GenericCapability
    );
    H323GenericCapabilityInfo(const H323GenericCapabilityInfo & obj);
    virtual ~H323GenericCapabilityInfo();

  protected:
    virtual BOOL OnSendingGenericPDU(
      H245_GenericCapability & pdu,
      const OpalMediaFormat & mediaFormat,
      H323Capability::CommandType type
    ) const;
    virtual BOOL OnReceivedGenericPDU(
      OpalMediaFormat & mediaFormat,
      const H245_GenericCapability & pdu,
      H323Capability::CommandType type
    );

    BOOL IsMatch(
      const H245_GenericCapability & param  ///< Non standard field in PDU received
    ) const;
    PObject::Comparison CompareInfo(
      const H323GenericCapabilityInfo & obj
    ) const;


    H245_CapabilityIdentifier * identifier;
    unsigned                    maxBitRate;
};

/**This class describes the interface to a codec that has channels based on
   the RTP protocol.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323RealTimeCapability : public H323Capability
{
  PCLASSINFO(H323RealTimeCapability, H323Capability);

  public:
  /**@name Operations */
  //@{
    /**Create the channel instance, allocating resources as required.
     */
    virtual H323Channel * CreateChannel(
      H323Connection & connection,    ///<  Owner connection for channel
      H323Channel::Directions dir,    ///<  Direction of channel
      unsigned sessionID,             ///<  Session ID for RTP channel
      const H245_H2250LogicalChannelParameters * param
                                      ///<  Parameters for channel
    ) const;

    H323RealTimeCapability();
    H323RealTimeCapability(const H323RealTimeCapability &rtc);
    virtual ~H323RealTimeCapability();
    void AttachQoS(RTP_QOS * _rtpqos);

  protected:
    RTP_QOS * rtpqos;
  //@}
};

#if OPAL_AUDIO

/**This class describes the interface to an audio codec used to transfer data
   via the logical channels opened and managed by the H323 control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323AudioCapability : public H323RealTimeCapability
{
  PCLASSINFO(H323AudioCapability, H323RealTimeCapability);

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

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour assumes the pdu is an integer number of frames
       per packet.
     */
    virtual BOOL OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize          ///<  Packet size to use in capability
    ) const;
    virtual BOOL OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize,         ///<  Packet size to use in capability
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual BOOL OnSendingPDU(
      H245_AudioMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns FALSE then the received PDU codec description
       is not supported, so will be ignored.
       
       The default behaviour calls the OnReceivedPDU() that takes a
       H245_AudioCapability and clamps the txFramesInPacket.
     */
    virtual BOOL OnReceivedPDU(
      const H245_Capability & pdu  ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.
       
       The default behaviour calls the OnReceivedPDU() that takes a
       H245_AudioCapability and clamps the txFramesInPacket or
       rxFramesInPacket.
     */
    virtual BOOL OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      BOOL receiver               ///<  Is receiver OLC
    );

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour assumes the pdu is an integer number of frames
       per packet.
     */
    virtual BOOL OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///<  PDU to get information from
      unsigned & packetSize              ///<  Packet size to use in capability
    );
    virtual BOOL OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///< PDU to get information from
      unsigned & packetSize,             ///< Packet size to use in capability
      CommandType type                   ///<  Type of PDU to send in
    );
  //@}
};


/**This class describes the interface to a non-standard audio codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323NonStandardAudioCapability : public H323AudioCapability,
                                       public H323NonStandardCapabilityInfo
{
  PCLASSINFO(H323NonStandardAudioCapability, H323AudioCapability);

  public:
  /**@name Construction */
  //@{
    H323NonStandardAudioCapability(
      H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
      const BYTE * dataBlock,         ///< Non-Standard data for codec type
      PINDEX dataSize                 ///< Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
    );
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

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize          ///<  Packet size to use in capability
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_AudioMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///<  PDU to get information from
      unsigned & packetSize              ///<  Packet size to use in capability
    );

    /**Compare the nonStandardData part of the capability, if applicable.
      */
    virtual BOOL IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}
};

/**This class describes the interface to a generic audio codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323GenericAudioCapability : public H323AudioCapability,
                                   public H323GenericCapabilityInfo
{
  PCLASSINFO(H323NonStandardAudioCapability, H323AudioCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new set of information about a non-standard codec.
      */
    H323GenericAudioCapability(
      const PString & capabilityId,    ///< generic codec identifier
      PINDEX maxBitRate = 0	           ///< maxBitRate parameter for the GenericCapability
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

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize,         ///<  Packet size to use in capability
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual BOOL OnSendingPDU(
      H245_AudioMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///< PDU to get information from
      unsigned & packetSize,             ///< Packet size to use in capability
      CommandType type                   ///<  Type of PDU to send in
    );

    /**Compare the generic part of the capability, if applicable.
     */
    virtual BOOL IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}
};

#endif  // OPAL_AUDIO

#if OPAL_VIDEO

/**This class describes the interface to a video codec used to transfer data
   via the logical channels opened and managed by the H323 control channel.

   An application may create a descendent off this class and override
   functions as required for descibing a codec.
 */
class H323VideoCapability : public H323RealTimeCapability
{
  PCLASSINFO(H323VideoCapability, H323RealTimeCapability);

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

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnSendingPDU(
      H245_VideoCapability & pdu  ///<  PDU to set information on
    ) const;
    virtual BOOL OnSendingPDU(
      H245_VideoCapability & pdu,  ///<  PDU to set information on
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual BOOL OnSendingPDU(
      H245_VideoMode & pdu  ///<  PDU to set information on
    ) const = 0;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns FALSE then the received PDU codec description
       is not supported, so will be ignored. The default behaviour simply
       returns TRUE.
     */
    virtual BOOL OnReceivedPDU(
      const H245_Capability & pdu  ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      BOOL receiver               ///<  Is receiver OLC
    );

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu  ///<  PDU to set information on
    );
    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu,  ///< PDU to get information from
      CommandType type                   ///<  Type of PDU to send in
    );
  //@}
};


/**This class describes the interface to a non-standard video codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323NonStandardVideoCapability : public H323VideoCapability,
                                       public H323NonStandardCapabilityInfo
{
  PCLASSINFO(H323NonStandardVideoCapability, H323VideoCapability);

  public:
  /**@name Construction */
  //@{
    H323NonStandardVideoCapability(
      H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
      const BYTE * dataBlock,         ///< Non-Standard data for codec type
      PINDEX dataSize                 ///< Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
    );
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

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_VideoCapability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_VideoMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu  ///<  PDU to set information on
    );

    /**Compare the nonStandardData part of the capability, if applicable.
      */
    virtual BOOL IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}
};

/**This class describes the interface to a generic video codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323GenericVideoCapability : public H323VideoCapability,
                                   public H323GenericCapabilityInfo
{
  PCLASSINFO(H323GenericVideoCapability, H323VideoCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new set of information about a non-standard codec.
      */
    H323GenericVideoCapability(
      const PString & capabilityId,    ///< generic codec identifier (OID)
      PINDEX maxBitRate = 0	       ///< maxBitRate parameter for the GenericCapability
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

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_VideoCapability & pdu,  ///<  PDU to set information on
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual BOOL OnSendingPDU(
      H245_VideoMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu,  ///< PDU to get information from
      CommandType type                   ///<  Type of PDU to send in
    );
  //@}

    /**Compare the generic part of the capability, if applicable.
     */
    virtual BOOL IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
};

#endif  // OPAL_VIDEO

/**This class describes the interface to a data channel used to transfer data
   via the logical channels opened and managed by the H323 control channel.

   An application may create a descendent off this class and override
   functions as required for descibing a codec.
 */
class H323DataCapability : public H323Capability
{
  PCLASSINFO(H323DataCapability, H323Capability);

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

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnSendingPDU(
      H245_DataApplicationCapability & pdu  ///<  PDU to set information on
    ) const;
    virtual BOOL OnSendingPDU(
      H245_DataApplicationCapability & pdu, ///<  PDU to set information on
      CommandType type                      ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual BOOL OnSendingPDU(
      H245_DataMode & pdu  ///<  PDU to set information on
    ) const = 0;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns FALSE then the received PDU codec description
       is not supported, so will be ignored. The default behaviour simply
       returns TRUE.
     */
    virtual BOOL OnReceivedPDU(
      const H245_Capability & pdu  ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      BOOL receiver               ///<  Is receiver OLC
    );

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnReceivedPDU(
      const H245_DataApplicationCapability & pdu  ///<  PDU to set information on
    );
    virtual BOOL OnReceivedPDU(
      const H245_DataApplicationCapability & pdu, ///<  PDU to set information on
      CommandType type                            ///<  Type of PDU to send in
    );
  //@}

  protected:
    unsigned maxBitRate;
};


/**This class describes the interface to a non-standard data codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323NonStandardDataCapability : public H323DataCapability,
                                      public H323NonStandardCapabilityInfo
{
  PCLASSINFO(H323NonStandardDataCapability, H323DataCapability);

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

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_DataApplicationCapability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_DataMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_DataApplicationCapability & pdu  ///<  PDU to set information on
    );

    /**Compare the nonStandardData part of the capability, if applicable.
      */
    virtual BOOL IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}
};


///////////////////////////////////////////////////////////////////////////////
// Known audio codecs

/**This class describes the G.711 codec capability.
 */
class H323_G711Capability : public H323AudioCapability
{
  PCLASSINFO(H323_G711Capability, H323AudioCapability)

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

  protected:
    Mode     mode;
    Speed    speed;
};

#if 0

/**This class describes the G.728 codec capability.
 */
class H323_G728Capability : public H323AudioCapability
{
  PCLASSINFO(H323_G728Capability, H323AudioCapability)

  public:
  /**@name Construction */
  //@{
    /**Create a new G.728 capability.
     */
    H323_G728Capability();
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
     */
    virtual unsigned GetSubType() const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}
};


/**This class describes the G.729 codec capability.
 */
class H323_G729Capability : public H323AudioCapability
{
  PCLASSINFO(H323_G729Capability, H323AudioCapability)

  public:
    /// Specific G.729 encoding algorithm.
    enum Mode {
      e_Normal,
      e_AnnexA,
      e_AnnexB,
      e_AnnexA_AnnexB
    };

  /**@name Construction */
  //@{
    /**Create a new G.729 capability.
     */
    H323_G729Capability(
      Mode mode 
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
     */
    virtual unsigned GetSubType() const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

  protected:
    Mode mode;
};


/**This class describes the G.723.1 codec capability.
 */
class H323_G7231Capability : public H323AudioCapability
{
  PCLASSINFO(H323_G7231Capability, H323AudioCapability)

  public:
  /**@name Construction */
  //@{
    /**Create a new G.723.1 capability.
     */
    H323_G7231Capability(
      BOOL allowSIDFrames = TRUE   ///<  Allow SID frames in data stream.
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
     */
    virtual unsigned GetSubType() const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour sets the data rate field in the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize          ///<  Packet size to use in capability
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour gets the data rate field from the PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///<  PDU to get information from
      unsigned & packetSize              ///<  Packet size to use in capability
    );
  //@}

  protected:
    BOOL allowSIDFrames;
};


/**This class describes the GSM 06.10 codec capability.
 */
class H323_GSM0610Capability : public H323AudioCapability
{
  PCLASSINFO(H323_GSM0610Capability, H323AudioCapability)

  public:
  /**@name Construction */
  //@{
    /**Create a new GSM 06.10 capability.
     */
    H323_GSM0610Capability();
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

    /**Set the maximum size (in frames) of data that will be transmitted in a
       single PDU.

       This will also be the desired number that will be sent by most codec
       implemetations.

       The default behaviour sets the txFramesInPacket variable.
     */
    virtual void SetTxFramesInPacket(
      unsigned frames   ///<  Number of frames per packet
    );
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour sets the data rate field in the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize          ///<  Packet size to use in capability
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour gets the data rate field from the PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///<  PDU to get information from
      unsigned & packetSize              ///<  Packet size to use in capability
    );
  //@}
};

#endif


///////////////////////////////////////////////////////////////////////////////

/**This class describes the UserInput psuedo-channel.
 */
class H323_UserInputCapability : public H323Capability
{
  PCLASSINFO(H323_UserInputCapability, H323Capability);

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
    friend ostream & operator<<(ostream & strm, SubTypes subType) { return strm << GetSubTypeName(subType); }

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

  /**@name Operations */
  //@{
    /**Create the channel instance, allocating resources as required.
       This creates a logical channel object appropriate for the parameters
       provided. Not if param is NULL, sessionID must be provided, otherwise
       this is taken from the fields in param.
     */
    virtual H323Channel * CreateChannel(
      H323Connection & connection,    ///<  Owner connection for channel
      H323Channel::Directions dir,    ///<  Direction of channel
      unsigned sessionID,             ///<  Session ID for RTP channel
      const H245_H2250LogicalChannelParameters * param
                                      ///<  Parameters for channel
    ) const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual BOOL OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns FALSE then the received PDU codec description
       is not supported, so will be ignored. The default behaviour simply
       returns TRUE.
     */
    virtual BOOL OnReceivedPDU(
      const H245_Capability & pdu  ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual BOOL OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      BOOL receiver               ///<  Is receiver OLC
    );

    /**Validate that the capability is usable given the connection.
       This checks agains the negotiated protocol version number and remote
       application to determine if this capability should be used in TCS or
       OLC pdus.

       The default behaviour will check for early versions and return FALSE
       for RFC2833 mode.
      */
    virtual BOOL IsUsable(
      const H323Connection & connection
    ) const;
  //@}

    static void AddAllCapabilities(
      H323Capabilities & capabilities,        ///<  Table to add capabilities to
      PINDEX descriptorNum,   ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous     ///<  The member of the SimultaneousCapabilitySet to add
    );

  protected:
    SubTypes subType;
};



///////////////////////////////////////////////////////////////////////////////

PLIST(H323CapabilitiesList, H323Capability);

PARRAY(H323CapabilitiesListArray, H323CapabilitiesList);

class H323SimultaneousCapabilities : public H323CapabilitiesListArray
{
  PCLASSINFO(H323SimultaneousCapabilities, H323CapabilitiesListArray);
  public:
    BOOL SetSize(PINDEX newSize);
};


PARRAY(H323CapabilitiesSetArray, H323SimultaneousCapabilities);


class H323CapabilitiesSet : public H323CapabilitiesSetArray
{
  PCLASSINFO(H323CapabilitiesSet, H323CapabilitiesSetArray);
  public:
    /// Set the new size of the table, internal use only.
    BOOL SetSize(PINDEX newSize);
};


/**This class contains all of the capabilities and their combinations.
  */
class H323Capabilities : public PObject
{
    PCLASSINFO(H323Capabilities, PObject);
  public:
  /**@name Construction */
  //@{
    /**Construct an empty capability set.
      */
    H323Capabilities();

    /**Construct a capability set from the H.245 PDU provided.
      */
    H323Capabilities(
      const H323Connection & connection,      ///<  Connection for capabilities
      const H245_TerminalCapabilitySet & pdu  ///<  PDU to convert to a capability set.
    );

    /**Construct a copy of a capability set.
       Note this will completely duplicate the set by making clones of every
       capability in the original set.
      */
    H323Capabilities(
      const H323Capabilities & original ///<  Original capabilities to duplicate
    );

    /**Assign a copy of a capability set.
       Note this will completely duplicate the set by making clones of every
       capability in the original set.
      */
    H323Capabilities & operator=(
      const H323Capabilities & original ///<  Original capabilities to duplicate
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Print out the object to the stream, virtual version of << operator.
     */
    void PrintOn(
      ostream & strm    ///<  Stream to print out to.
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Get the number of capabilities in the set.
      */
    PINDEX GetSize() const { return table.GetSize(); }

    /**Get the capability at the specified index.
      */
    H323Capability & operator[](PINDEX i) const { return table[i]; }

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

    /**Add all matching capabilities to descriptor lists.
       All capabilities that match the specified name are added as in the other
       form of the SetCapability() function.
      */
    PINDEX AddAllCapabilities(
      PINDEX descriptorNum,    ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,     ///<  The member of the SimultaneousCapabilitySet to add
      const PString & name,    ///<  New capabilities name, if using "known" one.
      BOOL exact = FALSE       ///<  Capability name must be exact match
    );

    // this function is retained for backwards compatibility
    PINDEX AddAllCapabilities(
      const H323EndPoint &,    ///<  The endpoint adding the capabilities.
      PINDEX descriptorNum,    ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,     ///<  The member of the SimultaneousCapabilitySet to add
      const PString & name,    ///<  New capabilities name, if using "known" one.
      BOOL exact = FALSE       ///<  Capability name must be exact match
    )
    { return AddAllCapabilities(descriptorNum, simultaneous, name, exact); }

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
    ) const;

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
      BOOL exact = FALSE       ///<  Capability name must be exact match
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

    /**Find the capability given the H.245 capability PDU.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const H245_Capability & cap  ///<  H245 capability table entry
    ) const;

    /**Find the capability given the H.245 data type PDU.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const H245_DataType & dataType  ///<  H245 data type of codec
    ) const;

    /**Find the capability given the H.245 data type PDU.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const H245_ModeElement & modeElement  ///<  H245 data type of codec
    ) const;

    /**Find the capability given the type codecs.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      H323Capability::MainTypes mainType, ///<  Main type to find
      unsigned subType = UINT_MAX         ///<  Sub-type to find (UINT_MAX=ignore)
    ) const;

    /**Build a H.245 PDU from the information in the capability set.
      */
    void BuildPDU(
      const H323Connection & connection,  ///<  Connection building PDU for
      H245_TerminalCapabilitySet & pdu    ///<  PDU to build
    ) const;

    /**Merge the capabilities into this set.
      */
    BOOL Merge(
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
    BOOL IsAllowed(
      const H323Capability & capability
    );

    /**Test if the capability is allowed.
      */
    BOOL IsAllowed(
      unsigned capabilityNumber
    );

    /**Test if the capabilities are an allowed combination.
      */
    BOOL IsAllowed(
      const H323Capability & capability1,
      const H323Capability & capability2
    );

    /**Test if the capabilities are an allowed combination.
      */
    BOOL IsAllowed(
      unsigned capabilityNumber1,
      unsigned capabilityNumber2
    );

    /**Get the list of capabilities as a list of media formats.
      */
    OpalMediaFormatList GetMediaFormats() const;
  //@}

  protected:
    H323CapabilitiesList table;
    H323CapabilitiesSet  set;
};


///////////////////////////////////////////////////////////////////////////////

/* New capability registration macros based on abstract factories
 */
typedef PFactory<H323Capability, std::string> H323CapabilityFactory;

#define H323_REGISTER_CAPABILITY(cls, capName)   static H323CapabilityFactory::Worker<cls> cls##Factory(capName, true); \

#endif // __OPAL_H323CAPS_H


/////////////////////////////////////////////////////////////////////////////
