/*
 * mediafmt.h
 *
 * Media Format descriptions
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Log: mediafmt.h,v $
 * Revision 1.2027  2005/06/20 16:47:52  shorne
 * Fix STL compatibility issue on MSVC6
 *
 * Revision 2.25  2005/06/02 13:20:45  rjongbloed
 * Added minimum and maximum check to media format options.
 * Added ability to set the options on the primordial media format list.
 *
 * Revision 2.24  2005/03/12 00:33:26  csoutheren
 * Fixed problems with STL compatibility on MSVC 6
 * Fixed problems with video streams
 * Thanks to Adrian Sietsma
 *
 * Revision 2.23  2005/02/21 12:19:47  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.22  2004/07/11 12:32:51  rjongbloed
 * Added functions to add/subtract lists of media formats from a media format list
 *
 * Revision 2.21  2004/05/03 00:59:18  csoutheren
 * Fixed problem with OpalMediaFormat::GetMediaFormatsList
 * Added new version of OpalMediaFormat::GetMediaFormatsList that minimses copying
 *
 * Revision 2.20  2004/03/22 11:32:41  rjongbloed
 * Added new codec type for 16 bit Linear PCM as must distinguish between the internal
 *   format used by such things as the sound card and the RTP payload format which
 *   is always big endian.
 *
 * Revision 2.19  2004/03/11 06:54:27  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.18  2004/02/07 02:18:18  rjongbloed
 * Improved searching for media format to use payload type AND the encoding name.
 *
 * Revision 2.17  2003/03/17 10:12:02  robertj
 * Fixed mutex problem with media format database.
 *
 * Revision 2.16  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.15  2002/11/10 11:33:17  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.14  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.13  2002/09/04 06:01:47  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.12  2002/07/01 04:56:31  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.11  2002/03/22 06:57:49  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.10  2002/02/19 07:36:28  robertj
 * Added OpalRFC2833 as a OpalMediaFormat variable.
 *
 * Revision 2.9  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.8  2002/01/22 05:06:30  robertj
 * Added RTP encoding name string to media format database.
 * Changed time units to clock rate in Hz.
 *
 * Revision 2.7  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.6  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.5  2001/10/04 00:42:12  robertj
 * Added function to remove wildcard from list.
 * Added constructor to make a list with one format in it.
 *
 * Revision 2.4  2001/08/23 05:51:17  robertj
 * Completed implementation of codec reordering.
 *
 * Revision 2.3  2001/08/22 03:51:31  robertj
 * Added functions to look up media format by payload type.
 *
 * Revision 2.2  2001/08/17 08:23:02  robertj
 * Put in missing dots in G.729 media formats.
 *
 * Revision 2.1  2001/08/01 05:51:39  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.13  2002/12/02 03:06:26  robertj
 * Fixed over zealous removal of code when NO_AUDIO_CODECS set.
 *
 * Revision 1.12  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.11  2002/09/03 06:19:37  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 *
 * Revision 1.10  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.9  2002/06/25 08:30:08  robertj
 * Changes to differentiate between stright G.723.1 and G.723.1 Annex A using
 *   the OLC dataType silenceSuppression field so does not send SID frames
 *   to receiver codecs that do not understand them.
 *
 * Revision 1.8  2002/03/21 02:39:15  robertj
 * Added backward compatibility define
 *
 * Revision 1.7  2002/02/11 04:15:56  robertj
 * Put G.723.1 at 6.3kbps back to old string value of "G.723.1" to improve
 *   backward compatibility. New #define is a synonym for it.
 *
 * Revision 1.6  2002/01/22 07:08:26  robertj
 * Added IllegalPayloadType enum as need marker for none set
 *   and MaxPayloadType is a legal value.
 *
 * Revision 1.5  2001/12/11 04:27:50  craigs
 * Added support for 5.3kbps G723.1
 *
 * Revision 1.4  2001/09/21 02:49:44  robertj
 * Implemented static object for all "known" media formats.
 * Added default session ID to media format description.
 *
 * Revision 1.3  2001/05/11 04:43:41  robertj
 * Added variable names for standard PCM-16 media format name.
 *
 * Revision 1.2  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.1  2001/01/25 07:27:14  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 */

#ifndef __OPAL_MEDIAFMT_H
#define __OPAL_MEDIAFMT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <rtp/rtp.h>

#include <limits>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

class OpalMediaFormat;


///////////////////////////////////////////////////////////////////////////////

PLIST(OpalMediaFormatBaseList, OpalMediaFormat);

/**This class contains a list of media formats.
  */
class OpalMediaFormatList : public OpalMediaFormatBaseList
{
    PCLASSINFO(OpalMediaFormatList, OpalMediaFormatBaseList);
  public:
  /**@name Construction */
  //@{
    /**Create an empty media format list.
     */
    OpalMediaFormatList();

    /**Create a media format list with one media format in it.
     */
    OpalMediaFormatList(
      const OpalMediaFormat & format    /// Format to add
    );

    /**Create a copy of a media format list.
     */
    OpalMediaFormatList(const OpalMediaFormatList & l) : OpalMediaFormatBaseList(l) { }
  //@}

  /**@name Operations */
  //@{
    /**Add a format to the list.
       If the format is invalid or already in the list then it is not added.
      */
    OpalMediaFormatList & operator+=(
      const OpalMediaFormat & format    /// Format to add
    );

    /**Add a format to the list.
       If the format is invalid or already in the list then it is not added.
      */
    OpalMediaFormatList & operator+=(
      const OpalMediaFormatList & formats    /// Formats to add
    );

    /**Remove a format to the list.
       If the format is invalid or not in the list then this does nothing.
      */
    OpalMediaFormatList & operator-=(
      const OpalMediaFormat & format    /// Format to remove
    );

    /**Remove a format to the list.
       If the format is invalid or not in the list then this does nothing.
      */
    OpalMediaFormatList & operator-=(
      const OpalMediaFormatList & formats    /// Formats to remove
    );

    /**Get a format position in the list matching the payload type.

       Returns P_MAX_INDEX if not in list.
      */
    PINDEX FindFormat(
      RTP_DataFrame::PayloadTypes rtpPayloadType, /// RTP payload type code
      const char * rtpEncodingName = NULL          /// RTP payload type name
    ) const;

    /**Get a format position in the list matching the wildcard.
       The wildcard string is a simple substring match using the '*'
       character. For example: "G.711*" would match "G.711-uLaw-64k" and
       "G.711-ALaw-64k".

       Returns P_MAX_INDEX if not in list.
      */
    PINDEX FindFormat(
      const PString & wildcard    /// Wildcard string name.
    ) const;

    /**Determine if a format matching the payload type is in the list.
      */
    BOOL HasFormat(
      RTP_DataFrame::PayloadTypes rtpPayloadType /// RTP payload type code
    ) const { return FindFormat(rtpPayloadType) != P_MAX_INDEX; }

    /**Determine if a format matching the wildcard is in the list.
       The wildcard string is a simple substring match using the '*'
       character. For example: "G.711*" would match "G.711-uLaw-64k" and
       "G.711-ALaw-64k".
      */
    BOOL HasFormat(
      const PString & wildcard    /// Wildcard string name.
    ) const { return FindFormat(wildcard) != P_MAX_INDEX; }

    /**Remove all the formats specified.
      */
    void Remove(
      const PStringArray & mask
    );

    /**Reorder the formats in the list.
       The order variable is an array of wildcards and the list is reordered
       according to the order in that array.
      */
    void Reorder(
      const PStringArray & order
    );
  //@}

  private:
    virtual PINDEX Append(PObject *) { return P_MAX_INDEX; }
    virtual PINDEX Insert(const PObject &, PObject *) { return P_MAX_INDEX; }
    virtual PINDEX InsertAt(PINDEX, PObject *) { return P_MAX_INDEX; }
    virtual BOOL SetAt(PINDEX, PObject *) { return FALSE; }
};


///////////////////////////////////////////////////////////////////////////////

/**Base class for options attached to an OpalMediaFormat.
  */
class OpalMediaOption : public PObject
{
    PCLASSINFO(OpalMediaOption, PObject);
  public:
    enum MergeType {
      NoMerge,
      MinMerge,
      MaxMerge,
      EqualMerge,
      NotEqualMerge,

      // Synonyms
      AndMerge = MaxMerge,
      OrMerge  = MinMerge,
      XorMerge = NotEqualMerge,
      NotXorMerge = EqualMerge
    };

  protected:
    OpalMediaOption(
      const char * name,
      bool readOnly,
      MergeType merge
    );

  public:
    virtual Comparison Compare(const PObject & obj) const;

    bool Merge(
      const OpalMediaOption & option
    );
    virtual void Assign(
      const OpalMediaOption & option
    ) = 0;

    PString AsString() const;
    bool FromString(const PString & value);

    const PString & GetName() const { return m_name; }

    bool IsReadOnly() const { return m_readOnly; }

    MergeType GetMerge() const { return m_merge; }
    void SetMerge(MergeType merge) { m_merge = merge; }

  protected:
    PString   m_name;
    bool      m_readOnly;
    MergeType m_merge;
};

#if ! __USE_STL__
__inline istream & operator>>(istream & strm, bool& b)
{
   int i;strm >> i;b = i; return strm;
}
#endif

template <typename T>
class OpalMediaOptionValue : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionValue, OpalMediaOption);
  public:
    OpalMediaOptionValue(
      const char * name,
      bool readOnly,
      MergeType merge = MinMerge,
      T value = 0,
      T minimum = std::numeric_limits<T>::min(),
      T maximum = std::numeric_limits<T>::max()
    ) : OpalMediaOption(name, readOnly, merge),
        m_value(value),
        m_minimum(minimum),
        m_maximum(maximum)
    { }

    virtual PObject * Clone() const
    {
      return new OpalMediaOptionValue(*this);
    }

    virtual void PrintOn(ostream & strm) const
    {
      strm << m_value;
    }
    virtual void ReadFrom(istream & strm)
    {
      T temp;
      strm >> temp;
      if (temp >= m_minimum && temp <= m_maximum)
        m_value = temp;
      else {
#if __USE_STL__
	   strm.setstate(ios::badbit);
#else
	   strm.setf(ios::badbit , ios::badbit);
#endif
       }
    }

    virtual void Assign(
      const OpalMediaOption & option
    ) {
      const OpalMediaOptionValue * otherOption = PDownCast(const OpalMediaOptionValue, &option);
      if (otherOption != NULL)
        m_value = otherOption->m_value;
    }

    T GetValue() const { return m_value; }
    void SetValue(T value) { m_value = value; }

  protected:
    T m_value;
    T m_minimum;
    T m_maximum;
};


typedef OpalMediaOptionValue<bool>   OpalMediaOptionBoolean;
typedef OpalMediaOptionValue<int>    OpalMediaOptionInteger;
typedef OpalMediaOptionValue<double> OpalMediaOptionReal;


class OpalMediaOptionEnum : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionEnum, OpalMediaOption);
  public:
    OpalMediaOptionEnum(
      const char * name,
      bool readOnly,
      const char * const * enumerations,
      PINDEX count,
      MergeType merge = EqualMerge,
      PINDEX value = 0
    );

    virtual PObject * Clone() const;
    virtual Comparison Compare(const PObject & obj) const;
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual void Assign(
      const OpalMediaOption & option
    );

    PINDEX GetValue() const { return m_value; }
    void SetValue(PINDEX value);

  protected:
    PStringArray m_enumerations;
    PINDEX       m_value;
};


class OpalMediaOptionString : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionString, OpalMediaOption);
  public:
    OpalMediaOptionString(
      const char * name,
      bool readOnly
    );
    OpalMediaOptionString(
      const char * name,
      bool readOnly,
      const PString & value
    );

    virtual PObject * Clone() const;
    virtual Comparison Compare(const PObject & obj) const;
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual void Assign(
      const OpalMediaOption & option
    );

    const PString & GetValue() const { return m_value; }
    void SetValue(const PString & value);

  protected:
    PString m_value;
};


///////////////////////////////////////////////////////////////////////////////

/**This class describes a media format as used in the OPAL system. A media
   format is the type of any media data that is trasferred between OPAL
   entities. For example an audio codec such as G.723.1 is a media format, a
   video codec such as H.261 is also a media format.

   There
  */
class OpalMediaFormat : public PCaselessString
{
  PCLASSINFO(OpalMediaFormat, PCaselessString);

  public:
    /**Default constructor creates a PCM-16 media format.
      */
    OpalMediaFormat();

    /**This form of the constructor will register the full details of the
       media format into an internal database. This would typically be used
       as a static global. In fact it would be very dangerous for an instance
       to use this constructor in any other way, especially local variables.

       If the rtpPayloadType is RTP_DataFrame::DynamicBase, then the RTP
       payload type is actually set to teh first unused dynamic RTP payload
       type that is in the registers set of media formats.

       The frameSize parameter indicates that the media format has a maximum
       size for each data frame, eg G.723.1 frames are no more than 24 bytes
       long. If zero then there is no intrinsic maximum, eg G.711.
      */
    OpalMediaFormat(
      const char * fullName,  /// Full name of media format
      unsigned defaultSessionID,  /// Default session for codec type
      RTP_DataFrame::PayloadTypes rtpPayloadType, /// RTP payload type code
      const char * encodingName, /// RTP encoding name
      BOOL     needsJitter,   /// Indicate format requires a jitter buffer
      unsigned bandwidth,     /// Bandwidth in bits/second
      PINDEX   frameSize, /// Size of frame in bytes (if applicable)
      unsigned frameTime, /// Time for frame in RTP units (if applicable)
      unsigned clockRate  /// Clock rate for data (if applicable)
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
      RTP_DataFrame::PayloadTypes rtpPayloadType, /// RTP payload type code
      const char * rtpEncodingName = NULL          /// RTP payload type name
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
      const char * wildcard  /// Wildcard name to search for
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
      const PString & wildcard  /// Wildcard name to search for
    );

    /**Search for the specified format type.
       This is equivalent to going fmt = OpalMediaFormat(rtpPayloadType);
      */
    OpalMediaFormat & operator=(
      RTP_DataFrame::PayloadTypes rtpPayloadType /// RTP payload type code
    );

    /**Search for the specified format name.
       This is equivalent to going fmt = OpalMediaFormat(search);
      */
    OpalMediaFormat & operator=(
      const char * wildcard  /// Wildcard name to search for
    );

    /**Search for the specified format name.
       This is equivalent to going fmt = OpalMediaFormat(search);
      */
    OpalMediaFormat & operator=(
      const PString & wildcard  /// Wildcard name to search for
    );

    /**Get the RTP payload type that is to be used for this media format.
       This will either be an intrinsic one for the media format eg GSM or it
       will be automatically calculated as a dynamic media format that will be
       uniqueue amongst the registered media formats.
      */
    RTP_DataFrame::PayloadTypes GetPayloadType() const { return rtpPayloadType; }

    /**Get the RTP encoding name that is to be used for this media format.
      */
    const char * GetEncodingName() const { return rtpEncodingName; }

    enum {
      DefaultAudioSessionID = 1,
      DefaultVideoSessionID = 2,
      DefaultDataSessionID  = 3
    };

    /**Get the default session ID for media format.
      */
    unsigned GetDefaultSessionID() const { return defaultSessionID; }

    /**Determine if the media format requires a jitter buffer. As a rule an
       audio codec needs a jitter buffer and all others do not.
      */
    bool NeedsJitterBuffer() const { return GetOptionBoolean(NeedsJitterOption); }
    static const char * const NeedsJitterOption;

    /**Get the average bandwidth used in bits/second.
      */
    unsigned GetBandwidth() const { return GetOptionInteger(MaxBitRateOption); }
    static const char * const MaxBitRateOption;

    /**Get the maximum frame size in bytes. If this returns zero then the
       media format has no intrinsic maximum frame size, eg G.711 would 
       return zero but G.723.1 whoud return 24.
      */
    PINDEX GetFrameSize() const { return GetOptionInteger(MaxFrameSizeOption); }
    static const char * const MaxFrameSizeOption;

    /**Get the frame rate in RTP timestamp units. If this returns zero then
       the media format is not real time and has no intrinsic timing eg
      */
    unsigned GetFrameTime() const { return GetOptionInteger(FrameTimeOption); }
    static const char * const FrameTimeOption;

    /**Get the number of RTP timestamp units per millisecond.
      */
    unsigned GetTimeUnits() const { return GetClockRate()/1000; }

    enum StandardClockRate {
      AudioClockRate = 8000,  // 8kHz sample rate
      VideoClockRate = 90000  // 90kHz sample rate
    };

    /**Get the clock rate in Hz for this format.
      */
    unsigned GetClockRate() const { return GetOptionInteger(ClockRateOption); }
    static const char * const ClockRateOption;

    /**Get the number of options this media format has.
      */
    PINDEX GetOptionCount() const { return options.GetSize(); }

    /**Get the option instance at the specified index. This contains the
       description and value for the option.
      */
    const OpalMediaOption & GetOption(
      PINDEX index   /// Index of option in list to get
    ) { return options[index]; }

    /**Get the option value of the specified name as a string.

       Returns false of the option is not present.
      */
    bool GetOptionValue(
      const PString & name,   /// Option name
      PString & value         /// String to receive option value
    ) const;

    /**Set the option value of the specified name as a string.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present.
      */
    bool SetOptionValue(
      const PString & name,   /// Option name
      const PString & value   /// New option value as string
    );

    /**Get the option value of the specified name as a boolean. The default
       value is returned if the option is not present.
      */
    bool GetOptionBoolean(
      const PString & name,   /// Option name
      bool dflt = FALSE       /// Default value if option not present
    ) const;

    /**Set the option value of the specified name as a boolean.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionBoolean(
      const PString & name,   /// Option name
      bool value              /// New value for option
    );

    /**Get the option value of the specified name as an integer. The default
       value is returned if the option is not present.
      */
    int GetOptionInteger(
      const PString & name,   /// Option name
      int dflt = 0            /// Default value if option not present
    ) const;

    /**Set the option value of the specified name as an integer.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present, not of the same type or
       is putside the allowable range.
      */
    bool SetOptionInteger(
      const PString & name,   /// Option name
      int value               /// New value for option
    );

    /**Get the option value of the specified name as a real. The default
       value is returned if the option is not present.
      */
    double GetOptionReal(
      const PString & name,   /// Option name
      double dflt = 0         /// Default value if option not present
    ) const;

    /**Set the option value of the specified name as a real.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionReal(
      const PString & name,   /// Option name
      double value            /// New value for option
    );

    /**Get the option value of the specified name as an index into an
       enumeration list. The default value is returned if the option is not
       present.
      */
    PINDEX GetOptionEnum(
      const PString & name,   /// Option name
      PINDEX dflt = 0         /// Default value if option not present
    ) const;

    /**Set the option value of the specified name as an index into an enumeration.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionEnum(
      const PString & name,   /// Option name
      PINDEX value            /// New value for option
    );

    /**Get the option value of the specified name as a string. The default
       value is returned if the option is not present.
      */
    PString GetOptionString(
      const PString & name,                   /// Option name
      const PString & dflt = PString::Empty() /// Default value if option not present
    ) const;

    /**Set the option value of the specified name as a string.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionString(
      const PString & name,   /// Option name
      const PString & value   /// New value for option
    );

    /**Get a copy of the list of media formats that have been registered.
      */
    static OpalMediaFormatList GetAllRegisteredMediaFormats();
    static void GetAllRegisteredMediaFormats(
      OpalMediaFormatList & copy    /// List to receive the copy of the master list
    );

    /**Set the options on the master format list entry.
       The media format must already be registered. Returns false if not.
      */
    static bool SetRegisteredMediaFormat(
      const OpalMediaFormat & mediaFormat  /// Media format to copy to master list
    );

  protected:
    bool AddOption(
      OpalMediaOption * option
    );

    OpalMediaOption * FindOption(
      const PString & name
    ) const;

    RTP_DataFrame::PayloadTypes  rtpPayloadType;
    const char *                 rtpEncodingName;
    unsigned                     defaultSessionID;
    PSortedList<OpalMediaOption> options;

  friend class OpalMediaFormatList;
};


// A pair of macros to simplify cration of OpalMediFormat instances.

#define OPAL_MEDIA_FORMAT(name, fullName, defaultSessionID, rtpPayloadType, encodingName, needsJitter, bandwidth, frameSize, frameTime, timeUnits) \
const class name##_Class : public OpalMediaFormat \
{ \
  public: \
    name##_Class(); \
} name; \
name##_Class::name##_Class() \
      : OpalMediaFormat(fullName, defaultSessionID, rtpPayloadType, encodingName, needsJitter, bandwidth, frameSize, frameTime, timeUnits) \


class OpalAudioFormat : public OpalMediaFormat
{
    PCLASSINFO(OpalAudioFormat, OpalMediaFormat);
  public:
    OpalAudioFormat(
      const char * fullName,    /// Full name of media format
      RTP_DataFrame::PayloadTypes rtpPayloadType, /// RTP payload type code
      const char * encodingName,/// RTP encoding name
      PINDEX   frameSize,       /// Size of frame in bytes (if applicable)
      unsigned frameTime,       /// Time for frame in RTP units (if applicable)
      unsigned rxFrames,        /// Maximum number of frames per packet we can receive
      unsigned txFrames,        /// Desired number of frames per packet we transmit
      unsigned maxFrames = 256  /// Maximum possible frames per packet
    );

    static const char * const RxFramesPerPacketOption;
    static const char * const TxFramesPerPacketOption;
};


// List of known media formats

#define OPAL_PCM16          "PCM-16"
#define OPAL_L16_MONO_8KHZ  "Linear-16-Mono-8kHz"
#define OPAL_L16_MONO_16KHZ "Linear-16-Mono-16kHz"
#define OPAL_G711_ULAW_64K  "G.711-uLaw-64k"
#define OPAL_G711_ALAW_64K  "G.711-ALaw-64k"
#define OPAL_G728           "G.728"
#define OPAL_G729           "G.729"
#define OPAL_G729A          "G.729A"
#define OPAL_G729B          "G.729B"
#define OPAL_G729AB         "G.729A/B"
#define OPAL_G7231          "G.723.1"
#define OPAL_G7231_6k3      OPAL_G7231
#define OPAL_G7231_5k3      "G.723.1(5.3k)"
#define OPAL_G7231A_6k3     "G.723.1A(6.3k)"
#define OPAL_G7231A_5k3     "G.723.1A(5.3k)"
#define OPAL_GSM0610        "GSM-06.10"
#define OPAL_RFC2833        "UserInput/RFC2833"

extern const OpalAudioFormat OpalPCM16;
extern const OpalAudioFormat OpalL16Mono8kHz;
extern const OpalAudioFormat OpalL16Mono16kHz;
extern const OpalAudioFormat OpalG711uLaw;
extern const OpalAudioFormat OpalG711ALaw;
extern const OpalAudioFormat OpalG728;
extern const OpalAudioFormat OpalG729;
extern const OpalAudioFormat OpalG729A;
extern const OpalAudioFormat OpalG729B;
extern const OpalAudioFormat OpalG729AB;
extern const OpalAudioFormat OpalG7231_6k3;
extern const OpalAudioFormat OpalG7231_5k3;
extern const OpalAudioFormat OpalG7231A_6k3;
extern const OpalAudioFormat OpalG7231A_5k3;
extern const OpalAudioFormat OpalGSM0610;
extern const OpalMediaFormat OpalRFC2833;


#endif  // __OPAL_MEDIAFMT_H


// End of File ///////////////////////////////////////////////////////////////
