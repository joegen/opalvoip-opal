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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_OPAL_MEDIAFMT_H
#define OPAL_OPAL_MEDIAFMT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifdef _MSC_VER
#if _MSC_VER < 1300   
#pragma warning(disable:4663)
#endif
#endif

#include <opal/buildopts.h>

#include <opal/mediatype.h>
#include <rtp/rtp.h>

#if OPAL_VIDEO
#include <ptlib/videoio.h>
#endif

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
      const OpalMediaFormat & format    ///<  Format to add
    );

    /**Create a copy of a media format list.
     */
    OpalMediaFormatList(const OpalMediaFormatList & l) : OpalMediaFormatBaseList(l) { }
  //@}

  /**@name Operations */
  //@{
    /**Add a format(s) to the list.
       If any format is invalid or already in the list then it is not added.
      */
    OpalMediaFormatList & operator+=(
      const char * wildcard    ///< Wildcard for formats to add
    ) { PConstString w(wildcard); return operator+=(w); }

    /**Add a format(s) to the list.
       If any format is invalid or already in the list then it is not added.
      */
    OpalMediaFormatList & operator+=(
      const PString & wildcard    ///< Wildcard for formats to add
    );

    /**Add a format to the list.
       If the format is invalid or already in the list then it is not added.
      */
    OpalMediaFormatList & operator+=(
      const OpalMediaFormat & format    ///<  Format to add
    );

    /**Add a format to the list.
       If the format is invalid or already in the list then it is not added.
      */
    OpalMediaFormatList & operator+=(
      const OpalMediaFormatList & formats    ///<  Formats to add
    );

    /**Remove a format to the list.
       If the format is invalid or not in the list then this does nothing.
      */
    OpalMediaFormatList & operator-=(
      const OpalMediaFormat & format    ///<  Format to remove
    );

    /**Remove a format to the list.
       If the format is invalid or not in the list then this does nothing.
      */
    OpalMediaFormatList & operator-=(
      const OpalMediaFormatList & formats    ///<  Formats to remove
    );

    /**Get a format iterator in the list matching the payload type.

       Returns end() if not in list.
      */
    const_iterator FindFormat(
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      const unsigned clockRate,                   ///<  clock rate
      const char * rtpEncodingName = NULL,        ///<  RTP payload type name
      const char * protocol = NULL,               ///<  protocol to be valid for (if NULL, then all)
      const_iterator start = const_iterator()     ///<  location to start search
    ) const;

    /**Get a position in the list of the first entry matching the wildcard.
       The wildcard string is a string match using several special
       characters.

       The '*' character indicates substrings, for example: "G.711*" would
       match "G.711-uLaw-64k" and "G.711-ALaw-64k".

       The '@' character indicates a type of media format, so say "\@video"
       would find the first video codec.

       The '!' character indicates a negative test. That is the first entry
       that does NOT match the string is returned. The string after the '!'
       may contain '*' and '@' characters.

       Returns P_MAX_INDEX if not in list.
      */
    const_iterator FindFormat(
      const PString & wildcard,    ///<  Wildcard string name.
      const_iterator start = const_iterator() ///< Starting position iterator for search
    ) const;

    /**Determine if a format matching the payload type is in the list.
      */
    PBoolean HasFormat(
      RTP_DataFrame::PayloadTypes rtpPayloadType ///<  RTP payload type code
    ) const { return FindFormat(rtpPayloadType) != end(); }

    /**Determine if a format matching the wildcard is in the list.
       The wildcard string is a simple substring match using the '*'
       character. For example: "G.711*" would match "G.711-uLaw-64k" and
       "G.711-ALaw-64k".
      */
    PBoolean HasFormat(
      const PString & wildcard    ///<  Wildcard string name.
    ) const { return FindFormat(wildcard) != end(); }

    /**Determine if a list contains a specific media type
      */
    bool HasType(
      const OpalMediaType & type,
      bool mustBeTransportable = true
    ) const;

    /**Reorder the formats in the list.
       The order variable is an array of names and the list is reordered
       according to the order in that array.

       Each string in the array is checked using the wildcard matching algorithm
       similar to FindFormat().

       The '*' character indicates substrings, for example: "G.711*" would
       match "G.711-uLaw-64k" and "G.711-ALaw-64k".

       The '@' character indicates a type of media format, so say "\@video"
       would sort by video codec.

        The '!' character is not supported.
      */
    void Reorder(
      const PStringArray & order
    );

    /**Remove all the formats specified.
       Each string in the array is checked using the wildcard matching algorithm in
       FindFormat().

       The '*' character indicates substrings, for example: "G.711*" would
       remove "G.711-uLaw-64k" and "G.711-ALaw-64k".

       The '@' character indicates a type of media format, so say "\@video"
       would remove all video codecs.

       The '!' character indicates a negative test. That is the entres that do
       NOT match the string are removed. The string after the '!' may contain
       '*' and '@' characters.

       I should be noted that when the ! operator is used, they are combined
       differently to the usual application of each entry in turn. Thus, the
       array {"!A","!B"} will result in keeping <i>both</i> A and B formats.
     */
    void Remove(
      const PStringArray & mask
    );

    /**Remove all the non-transportable formats specified.
      */
    void RemoveNonTransportable();
  //@}

  private:
    virtual PINDEX Append(PObject *) { return P_MAX_INDEX; }
    virtual PINDEX Insert(const PObject &, PObject *) { return P_MAX_INDEX; }
};


///////////////////////////////////////////////////////////////////////////////

/**Base class for options attached to an OpalMediaFormat.
  */
class OpalMediaOption : public PObject
{
    PCLASSINFO(OpalMediaOption, PObject);
  public:
    // Note the below enum must be identical to PluginCodec_OptionMerge in opalplugin.h
    enum MergeType {
      NoMerge,
      MinMerge,
      MaxMerge,
      EqualMerge,
      NotEqualMerge,
      AlwaysMerge,
      CustomMerge,
      IntersectionMerge, // Set intersection, applies to numeric (bit wise AND) or string (common substrings)

      // Synonyms
      AndMerge = MinMerge,  // Applies to Boolean option or Enum with two elements
      OrMerge  = MaxMerge   // Applies to Boolean option or Enum with two elements
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

    virtual bool ValidateMerge(
      const OpalMediaOption & option
    ) const;

    virtual Comparison CompareValue(
      const OpalMediaOption & option
    ) const = 0;
    virtual void Assign(
      const OpalMediaOption & option
    ) = 0;

    PString AsString() const;
    bool FromString(const PString & value);

    const PString & GetName() const { return m_name; }

    bool IsReadOnly() const { return m_readOnly; }
    void SetReadOnly(bool readOnly) { m_readOnly = readOnly; }

    MergeType GetMerge() const { return m_merge; }
    void SetMerge(MergeType merge) { m_merge = merge; }

#if OPAL_SIP
    const PString & GetFMTPName() const { return m_FMTPName; }
    void SetFMTPName(const char * name) { m_FMTPName = name; }

    const PString & GetFMTPDefault() const { return m_FMTPDefault; }
    void SetFMTPDefault(const char * value) { m_FMTPDefault = value; }
#endif // OPAL_SIP

#if OPAL_H323
    struct H245GenericInfo {
      H245GenericInfo() { memset(this, 0, sizeof(*this)); }
      unsigned ordinal:16;
      enum Modes {
        None,
        Collapsing,
        NonCollapsing
      } mode:3;
      enum IntegerTypes {
        UnsignedInt,
        Unsigned32,
        BooleanArray
      } integerType:3;
      bool excludeTCS:1;
      bool excludeOLC:1;
      bool excludeReqMode:1;
    };

    const H245GenericInfo & GetH245Generic() const { return m_H245Generic; }
    void SetH245Generic(const H245GenericInfo & genericInfo) { m_H245Generic = genericInfo; }
#endif // OPAL_H323

  protected:
    PCaselessString m_name;
    bool            m_readOnly;
    MergeType       m_merge;

#if OPAL_SIP
    PCaselessString m_FMTPName;
    PString         m_FMTPDefault;
#endif // OPAL_SIP

#if OPAL_H323
    H245GenericInfo m_H245Generic;
#endif // OPAL_H323
};


template <typename T>
class OpalMediaOptionValue : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionValue, OpalMediaOption);
  public:
    OpalMediaOptionValue(
      const char * name,
      bool readOnly,
      OpalMediaOption::MergeType merge = OpalMediaOption::MinMerge,
      T value = T()
    )
      : OpalMediaOption(name, readOnly, merge)
      , m_value(value)
    {
    }

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
      strm >> m_value;
    }

    virtual bool Merge(const OpalMediaOption & option)
    {
      if (m_merge != IntersectionMerge)
        return OpalMediaOption::Merge(option);

      const OpalMediaOptionValue * otherOption = PDownCast(const OpalMediaOptionValue, &option);
      if (otherOption == NULL)
        return false;

      m_value &= otherOption->m_value;
      return true;
    }

    virtual Comparison CompareValue(const OpalMediaOption & option) const
    {
      const OpalMediaOptionValue * otherOption = PDownCast(const OpalMediaOptionValue, &option);
      if (otherOption == NULL)
        return GreaterThan;
      if (m_value < otherOption->m_value)
        return LessThan;
      if (m_value > otherOption->m_value)
        return GreaterThan;
      return EqualTo;
    }

    virtual void Assign(const OpalMediaOption & option)
    {
      const OpalMediaOptionValue * otherOption = PDownCast(const OpalMediaOptionValue, &option);
      if (otherOption != NULL)
        m_value = otherOption->m_value;
    }

    T GetValue() const
    {
      return m_value;
    }

    void SetValue(T value)
    {
      m_value = value;
    }

  protected:
    T m_value;
};


template <typename T>
class OpalMediaOptionNumericalValue : public OpalMediaOptionValue<T>
{
    typedef OpalMediaOptionValue<T> BaseClass;
    PCLASSINFO(OpalMediaOptionNumericalValue, BaseClass);
  public:
    OpalMediaOptionNumericalValue(
      const char * name,
      bool readOnly,
      OpalMediaOption::MergeType merge = OpalMediaOption::MinMerge,
      T value = 0,
      T minimum = std::numeric_limits<T>::min(),
      T maximum = std::numeric_limits<T>::max()
    )
      : BaseClass(name, readOnly, merge, value)
      , m_minimum(minimum)
      , m_maximum(maximum)
    {
    }

    virtual PObject * Clone() const
    {
      return new OpalMediaOptionNumericalValue(*this);
    }

    virtual void ReadFrom(istream & strm)
    {
      T temp = 0;
      strm >> temp;
      if (strm.fail())
        return;
      if (temp >= m_minimum && temp <= m_maximum)
        this->m_value = temp;
      else
        strm.setstate(ios::badbit);
    }

    void SetValue(T value)
    {
      if (value < m_minimum)
        this->m_value = m_minimum;
      else if (value > m_maximum)
        this->m_value = m_maximum;
      else
        this->m_value = value;
    }

    void SetMinimum(T m)
    {
      this->m_minimum = m;
    }

    void SetMaximum(T m)
    {
      this->m_maximum = m;
    }

  protected:
    T m_minimum;
    T m_maximum;
};


typedef OpalMediaOptionNumericalValue<bool>     OpalMediaOptionBoolean;
typedef OpalMediaOptionNumericalValue<int>      OpalMediaOptionInteger;
typedef OpalMediaOptionNumericalValue<unsigned> OpalMediaOptionUnsigned;

// Wrapper class so we can implement intersection (&= operator) for floating point
class OpalMediaOptionRealValue
{
    double m_value;
  public:
    OpalMediaOptionRealValue(double value = 0) : m_value(value) { }
    operator double() const { return m_value; }
    void operator&=(double other) { if (m_value > other) m_value = other; }
    friend ostream & operator<<(ostream & strm, const OpalMediaOptionRealValue & value) { return strm << value.m_value; }
    friend istream & operator>>(istream & strm,       OpalMediaOptionRealValue & value) { return strm >> value.m_value; }
};

typedef OpalMediaOptionNumericalValue<OpalMediaOptionRealValue> OpalMediaOptionReal;


class OpalMediaOptionEnum : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionEnum, OpalMediaOption);
  public:
    OpalMediaOptionEnum(
      const char * name,
      bool readOnly
    );
    OpalMediaOptionEnum(
      const char * name,
      bool readOnly,
      const char * const * enumerations,
      PINDEX count,
      MergeType merge = EqualMerge,
      PINDEX value = 0
    );

    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual Comparison CompareValue(const OpalMediaOption & option) const;
    virtual void Assign(const OpalMediaOption & option);

    PINDEX GetValue() const { return m_value; }
    void SetValue(PINDEX value);

    const PStringArray & GetEnumerations() const { return m_enumerations; }
    void SetEnumerations(const PStringArray & e)
    {
      m_enumerations = e;
    }

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
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual bool Merge(const OpalMediaOption & option);
    virtual Comparison CompareValue(const OpalMediaOption & option) const;
    virtual void Assign(const OpalMediaOption & option);

    const PString & GetValue() const { return m_value; }
    void SetValue(const PString & value);

  protected:
    PString m_value;
};


class OpalMediaOptionOctets : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionOctets, OpalMediaOption);
  public:
    OpalMediaOptionOctets(
      const char * name,
      bool readOnly,
      bool base64 = false
    );
    OpalMediaOptionOctets(
      const char * name,
      bool readOnly,
      bool base64,
      const PBYTEArray & value
    );
    OpalMediaOptionOctets(
      const char * name,
      bool readOnly,
      bool base64,
      const BYTE * data,
      PINDEX length
    );

    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual Comparison CompareValue(const OpalMediaOption & option) const;
    virtual void Assign(const OpalMediaOption & option);

    const PBYTEArray & GetValue() const { return m_value; }
    void SetValue(const PBYTEArray & value);
    void SetValue(const BYTE * data, PINDEX length);

    void SetBase64(bool b)
    {
      m_base64 = b;
    }

  protected:
    PBYTEArray m_value;
    bool       m_base64;
};


///////////////////////////////////////////////////////////////////////////////

class OpalMediaFormatInternal : public PObject
{
    PCLASSINFO(OpalMediaFormatInternal, PObject);
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

    const PCaselessString & GetName() const { return formatName; }

    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

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

  protected:
    PCaselessString              formatName;
    RTP_DataFrame::PayloadTypes  rtpPayloadType;
    PString                      rtpEncodingName;
    OpalMediaType                mediaType;
    PMutex                       media_format_mutex;
    PSortedList<OpalMediaOption> options;
    time_t                       codecVersionTime;
    bool                         forceIsTransportable;
    int                          m_channels;

  friend bool operator==(const char * other, const OpalMediaFormat & fmt);
  friend bool operator!=(const char * other, const OpalMediaFormat & fmt);
  friend bool operator==(const PString & other, const OpalMediaFormat & fmt);
  friend bool operator!=(const PString & other, const OpalMediaFormat & fmt);

  friend class OpalMediaFormat;
  friend class OpalMediaFormatList;
  friend class OpalAudioFormatInternal;
};


///////////////////////////////////////////////////////////////////////////////

/**This class describes a media format as used in the OPAL system. A media
   format is the type of any media data that is trasferred between OPAL
   entities. For example an audio codec such as G.723.1 is a media format, a
   video codec such as H.261 is also a media format.
  */
class OpalMediaFormat : public PContainer
{
    PCLASSINFO(OpalMediaFormat, PContainer)
  public:
    OpalMediaFormat(const OpalMediaFormat & c) : PContainer(c), m_info(c.m_info) { }
    OpalMediaFormat & operator=(const OpalMediaFormat & c)     { AssignContents(c); return *this; }
    virtual ~OpalMediaFormat()                                 { Destruct(); }
    virtual PBoolean MakeUnique();
  protected:
    virtual void DestroyContents();
    virtual void AssignContents(const PContainer & c);

  public:
    /**Default constructor creates a PCM-16 media format.
      */
    OpalMediaFormat(
      OpalMediaFormatInternal * info = NULL
    );

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
    
    /**Search for the specified format type.
       This is equivalent to going fmt = OpalMediaFormat(rtpPayloadType);
      */
    OpalMediaFormat & operator=(
      RTP_DataFrame::PayloadTypes rtpPayloadType ///<  RTP payload type code
    );

    /**Search for the specified format name.
       This is equivalent to going fmt = OpalMediaFormat(search);
      */
    OpalMediaFormat & operator=(
      const char * wildcard  ///<  Wildcard name to search for
    );

    /**Search for the specified format name.
       This is equivalent to going fmt = OpalMediaFormat(search);
      */
    OpalMediaFormat & operator=(
      const PString & wildcard  ///<  Wildcard name to search for
    );

    /**Create a copy of the media format.
      */
    virtual PObject * Clone() const;

    /**Compare two media formats.
      */
    virtual Comparison Compare(const PObject & obj) const;

    /**Print media format.
       Note if the user specifies a width (using setw() for example) of -1, then
       a details multi-line output of all the options for the format is included.
      */
    virtual void PrintOn(ostream & strm) const;

    /**Read media format.
      */
    virtual void ReadFrom(istream & strm);

    /**This will translate the codec specific "custom" options to OPAL
       "normalised" options, e.g. For H.261 "QCIF MPI"="1", "CIF MPI"="5"
        would be translated to "Frame Width"="176", "Frame Height"="144".
      */
    bool ToNormalisedOptions();

    /**This will do the reverse of ToNormalisedOptions, translating the OPAL
       "normalised" options to codec specific "custom" options.
      */
    bool ToCustomisedOptions();

    /**Update media format information.
      */
    bool Update(
      const OpalMediaFormat & mediaFormat
    );

    /**Merge with another media format. This will alter and validate
       the options for this media format according to the merge rule for
       each option. The parameter is typically a "capability" while the
       current object isthe proposed channel format. This if the current
       object has a tx number of frames of 3, but the parameter has a value
       of 1, then the current object will be set to 1.

       Returns false if the media formats are incompatible and cannot be
       merged.
      */
    bool Merge(
      const OpalMediaFormat & mediaFormat
    );
    /**Return true if the given mediaFormat will successfully merge.
      */
    bool ValidateMerge(
      const OpalMediaFormat & mediaFormat
    ) const;

    /**Get the name of the format
      */
    PString GetName() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? "" : m_info->formatName; }

    /**Return true if media format info is valid. This may be used if the
       single string constructor is used to check that it matched something
       in the registered media formats database.
      */
    PBoolean IsValid() const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->IsValid(); }

    /**Return true if media format info may be sent via RTP. Some formats are internal
       use only and are never transported "over the wire".
      */
    PBoolean IsTransportable() const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->IsTransportable(); }

    /**Get the RTP payload type that is to be used for this media format.
       This will either be an intrinsic one for the media format eg GSM or it
       will be automatically calculated as a dynamic media format that will be
       uniqueue amongst the registered media formats.
      */
    RTP_DataFrame::PayloadTypes GetPayloadType() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? RTP_DataFrame::IllegalPayloadType : m_info->rtpPayloadType; }
    void SetPayloadType(RTP_DataFrame::PayloadTypes type) { PWaitAndSignal m(m_mutex); MakeUnique(); if (m_info != NULL) m_info->rtpPayloadType = type; }

    /**Get the RTP encoding name that is to be used for this media format.
      */
    const char * GetEncodingName() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? "" : (const char *)m_info->rtpEncodingName; }

    /** Get the media type for this format
      */
    OpalMediaType GetMediaType() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? OpalMediaType() : m_info->mediaType; }

    /**Determine if the media format requires a jitter buffer. As a rule an
       audio codec needs a jitter buffer and all others do not.
      */
    bool NeedsJitterBuffer() const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->GetOptionBoolean(NeedsJitterOption(), false); }
    static const PString & NeedsJitterOption();

    /**Get the average bandwidth used in bits/second.
      */
    unsigned GetBandwidth() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? 0 : m_info->GetOptionInteger(MaxBitRateOption(), 0); }
    static const PString & MaxBitRateOption();
    static const PString & TargetBitRateOption();

    /**Get the maximum frame size in bytes. If this returns zero then the
       media format has no intrinsic maximum frame size, eg a video format
       would return zero but G.723.1 would return 24.
      */
    PINDEX GetFrameSize() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? 0 : m_info->GetOptionInteger(MaxFrameSizeOption(), 0); }
    static const PString & MaxFrameSizeOption();

    /**Get the frame time in RTP timestamp units. If this returns zero then
       the media format is not real time and has no intrinsic timing eg T.120
      */
    unsigned GetFrameTime() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? 0 : m_info->GetOptionInteger(FrameTimeOption(), 0); }
    static const PString & FrameTimeOption();

    /**Get the number of RTP timestamp units per millisecond.
      */
    unsigned GetTimeUnits() const { return GetClockRate()/1000; }

    enum StandardClockRate {
      AudioClockRate = 8000,  ///<  8kHz sample rate
      VideoClockRate = 90000  ///<  90kHz sample rate
    };

    /**Get the clock rate in Hz for this format.
      */
    unsigned GetClockRate() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? 0 : m_info->GetOptionInteger(ClockRateOption(), 1000); }
    static const PString & ClockRateOption();

    /**Get the name of the OpalMediaOption indicating the protocol the format is being used on.
      */
    static const PString & ProtocolOption();

    /**Get the name of the OpalMediaOption indicating the maximum RTP payload size.
       Note this is a read only parameter and not generally set by the user. It
       is intended to get the OpalManager::GetMaxRtpPayloadSize() value to the
       codec (especially plug ins) before encoding starts in case the codec
       requires some form of initialisation based on this value.
      */
    static const PString & MaxTxPacketSizeOption();

    /**Get all of the option values of the format as a dictionary.
       Each entry is a name value pair.
      */
    PStringToString GetOptions() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? PStringToString() : m_info->GetOptions(); }

    /**Get the number of options this media format has.
      */
    PINDEX GetOptionCount() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? 0 : m_info->options.GetSize(); }

    /**Get the option instance at the specified index. This contains the
       description and value for the option.
      */
    const OpalMediaOption & GetOption(
      PINDEX index   ///<  Index of option in list to get
    ) const { PWaitAndSignal m(m_mutex); return m_info->options[index]; }

    /**Get the option value of the specified name as a string.

       Returns false of the option is not present.
      */
    bool GetOptionValue(
      const PString & name,   ///<  Option name
      PString & value         ///<  String to receive option value
    ) const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->GetOptionValue(name, value); }

    /**Set the option value of the specified name as a string.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present.
      */
    bool SetOptionValue(
      const PString & name,   ///<  Option name
      const PString & value   ///<  New option value as string
    ) { PWaitAndSignal m(m_mutex); MakeUnique(); return m_info != NULL && m_info->SetOptionValue(name, value); }

    /**Get the option value of the specified name as a boolean. The default
       value is returned if the option is not present.
      */
    bool GetOptionBoolean(
      const PString & name,   ///<  Option name
      bool dflt = false       ///<  Default value if option not present
    ) const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->GetOptionBoolean(name, dflt); }

    /**Set the option value of the specified name as a boolean.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionBoolean(
      const PString & name,   ///<  Option name
      bool value              ///<  New value for option
    ) { PWaitAndSignal m(m_mutex); MakeUnique(); return m_info != NULL && m_info->SetOptionBoolean(name, value); }

    /**Get the option value of the specified name as an integer. The default
       value is returned if the option is not present.
      */
    int GetOptionInteger(
      const PString & name,   ///<  Option name
      int dflt = 0            ///<  Default value if option not present
    ) const { PWaitAndSignal m(m_mutex); return m_info == NULL ? dflt : m_info->GetOptionInteger(name, dflt); }

    /**Set the option value of the specified name as an integer.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present, not of the same type or
       is putside the allowable range.
      */
    bool SetOptionInteger(
      const PString & name,   ///<  Option name
      int value               ///<  New value for option
    ) { PWaitAndSignal m(m_mutex); MakeUnique(); return m_info != NULL && m_info->SetOptionInteger(name, value); }

    /**Get the option value of the specified name as a real. The default
       value is returned if the option is not present.
      */
    double GetOptionReal(
      const PString & name,   ///<  Option name
      double dflt = 0         ///<  Default value if option not present
    ) const { PWaitAndSignal m(m_mutex); return m_info == NULL ? dflt : m_info->GetOptionReal(name, dflt); }

    /**Set the option value of the specified name as a real.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionReal(
      const PString & name,   ///<  Option name
      double value            ///<  New value for option
    ) { PWaitAndSignal m(m_mutex); MakeUnique(); return m_info != NULL && m_info->SetOptionReal(name, value); }

    /**Get the option value of the specified name as an index into an
       enumeration list. The default value is returned if the option is not
       present.
      */
    PINDEX GetOptionEnum(
      const PString & name,   ///<  Option name
      PINDEX dflt = 0         ///<  Default value if option not present
    ) const { PWaitAndSignal m(m_mutex); return m_info == NULL ? dflt : m_info->GetOptionEnum(name, dflt); }

    /**Set the option value of the specified name as an index into an enumeration.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionEnum(
      const PString & name,   ///<  Option name
      PINDEX value            ///<  New value for option
    ) { PWaitAndSignal m(m_mutex); MakeUnique(); return m_info != NULL && m_info->SetOptionEnum(name, value); }

    /**Get the option value of the specified name as a string. The default
       value is returned if the option is not present.
      */
    PString GetOptionString(
      const PString & name,                   ///<  Option name
      const PString & dflt = PString::Empty() ///<  Default value if option not present
    ) const { PWaitAndSignal m(m_mutex); return m_info == NULL ? dflt : m_info->GetOptionString(name, dflt); }

    /**Set the option value of the specified name as a string.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionString(
      const PString & name,   ///<  Option name
      const PString & value   ///<  New value for option
    ) { PWaitAndSignal m(m_mutex); MakeUnique(); return m_info != NULL && m_info->SetOptionString(name, value); }

    /**Get the option value of the specified name as an octet array.
       Returns false if not present.
      */
    bool GetOptionOctets(
      const PString & name, ///<  Option name
      PBYTEArray & octets   ///<  Octets in option
    ) const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->GetOptionOctets(name, octets); }

    /**Set the option value of the specified name as an octet array.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionOctets(
      const PString & name,       ///<  Option name
      const PBYTEArray & octets   ///<  Octets in option
    ) { PWaitAndSignal m(m_mutex); MakeUnique(); return m_info != NULL && m_info->SetOptionOctets(name, octets); }
    bool SetOptionOctets(
      const PString & name,       ///<  Option name
      const BYTE * data,          ///<  Octets in option
      PINDEX length               ///<  Number of octets
    ) { PWaitAndSignal m(m_mutex); MakeUnique(); return m_info != NULL && m_info->SetOptionOctets(name, data, length); }

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
      PBoolean overwrite = false
    ) { PWaitAndSignal m(m_mutex); MakeUnique(); return m_info != NULL && m_info->AddOption(option, overwrite); }

    /**
      * Determine if media format has the specified option.
      */
    bool HasOption(const PString & name) const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->FindOption(name) != NULL; }

    /** Get a pointer to the specified media format option.
        Returns NULL if thee option does not exist.
      */
    OpalMediaOption * FindOption(
      const PString & name
    ) const { PWaitAndSignal m(m_mutex); return m_info == NULL ? NULL : m_info->FindOption(name); }

    /** Get a pointer to the specified media format option.
        Returns NULL if thee option does not exist.
      */
    template <class T> T * FindOptionAs(
      const PString & name
    ) const { return dynamic_cast<T *>(FindOption(name)); }

    /** Returns true if the media format is valid for the protocol specified
        This allow plugin codecs to customise which protocols they are valid for
        The default implementation returns true unless the protocol is H.323
        and the rtpEncodingName is NULL
      */
    bool IsValidForProtocol(const PString & protocol) const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->IsValidForProtocol(protocol); }

    time_t GetCodecVersionTime() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? 0 : m_info->codecVersionTime; }

    ostream & PrintOptions(ostream & strm) const
    {
      PWaitAndSignal m(m_mutex);
      if (m_info != NULL)
        strm << setw(-1) << *m_info;
      return strm;
    }

#if OPAL_VIDEO
    /** Adjust the video device arguments from the media format.
     */
    void AdjustVideoArgs(
      PVideoDevice::OpenArgs & args  /// Videoo device arguments to adjust
    ) const;
#endif

    // Backward compatibility
    virtual PBoolean IsEmpty() const { PWaitAndSignal m(m_mutex); return m_info == NULL || !m_info->IsValid(); }
    operator PString() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? "" : m_info->formatName; }
    operator const char *() const { PWaitAndSignal m(m_mutex); return m_info == NULL ? "" : m_info->formatName; }
    bool operator==(const char * other) const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->formatName == other; }
    bool operator!=(const char * other) const { PWaitAndSignal m(m_mutex); return m_info == NULL || m_info->formatName != other; }
    bool operator==(const PString & other) const { PWaitAndSignal m(m_mutex); return m_info != NULL && m_info->formatName == other; }
    bool operator!=(const PString & other) const { PWaitAndSignal m(m_mutex); return m_info == NULL || m_info->formatName != other; }
    bool operator==(const OpalMediaFormat & other) const { PWaitAndSignal m(m_mutex); return Compare(other) == EqualTo; }
    bool operator!=(const OpalMediaFormat & other) const { PWaitAndSignal m(m_mutex); return Compare(other) != EqualTo; }
    friend bool operator==(const char * other, const OpalMediaFormat & fmt) { return fmt.m_info != NULL && fmt.m_info->formatName == other; }
    friend bool operator!=(const char * other, const OpalMediaFormat & fmt) { return fmt.m_info == NULL || fmt.m_info->formatName != other; }
    friend bool operator==(const PString & other, const OpalMediaFormat & fmt) { return fmt.m_info != NULL && fmt.m_info->formatName == other; }
    friend bool operator!=(const PString & other, const OpalMediaFormat & fmt) { return fmt.m_info == NULL || fmt.m_info->formatName != other; }

#if OPAL_H323
    static const PString & MediaPacketizationOption();
    static const PString & MediaPacketizationsOption();
#endif

  private:
    PBoolean SetSize(PINDEX) { return true; }

  protected:
    void Construct(OpalMediaFormatInternal * info);

    OpalMediaFormatInternal * m_info;
    PMutex                    m_mutex;

  friend class OpalMediaFormatInternal;
  friend class OpalMediaFormatList;
};


class OpalAudioFormatInternal : public OpalMediaFormatInternal
{
  public:
    OpalAudioFormatInternal(
      const char * fullName,
      RTP_DataFrame::PayloadTypes rtpPayloadType,
      const char * encodingName,
      PINDEX   frameSize,
      unsigned frameTime,
      unsigned rxFrames,
      unsigned txFrames,
      unsigned maxFrames,
      unsigned clockRate,
      time_t timeStamp
    );
    virtual PObject * Clone() const;
    virtual bool Merge(const OpalMediaFormatInternal & mediaFormat);
};

class OpalAudioFormat : public OpalMediaFormat
{
    PCLASSINFO(OpalAudioFormat, OpalMediaFormat);
  public:
    OpalAudioFormat(
      OpalMediaFormatInternal * info = NULL
    ) : OpalMediaFormat(info) { }
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

#if OPAL_VIDEO
class OpalVideoFormatInternal : public OpalMediaFormatInternal
{
  public:
    OpalVideoFormatInternal(
      const char * fullName,
      RTP_DataFrame::PayloadTypes rtpPayloadType,
      const char * encodingName,
      unsigned maxFrameWidth,
      unsigned maxFrameHeight,
      unsigned maxFrameRate,
      unsigned maxBitRate,
      time_t timeStamp
    );
    virtual PObject * Clone() const;
    virtual bool Merge(const OpalMediaFormatInternal & mediaFormat);
};


class OpalVideoFormat : public OpalMediaFormat
{
    PCLASSINFO(OpalVideoFormat, OpalMediaFormat);
  public:
    OpalVideoFormat(
      OpalMediaFormatInternal * info = NULL
    ) : OpalMediaFormat(info) { }
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
    static const PString & RateControllerBitRateScalerOption();

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
    enum { ContentRoleMask = 15 };
    __inline static unsigned ContentRoleBit(ContentRole contentRole) { return contentRole != eNoRole ? (1<<(contentRole-1)) : 0; }
    static const PString & ContentRoleOption();
    static const PString & ContentRoleMaskOption();
};
#endif

// List of known media formats

#define OPAL_PCM16          "PCM-16"
#define OPAL_PCM16S         "PCM-16S"
#define OPAL_PCM16_16KHZ    "PCM-16-16kHz"
#define OPAL_PCM16S_16KHZ   "PCM-16S-16kHz"
#define OPAL_PCM16_32KHZ    "PCM-16-32kHz"
#define OPAL_PCM16S_32KHZ   "PCM-16S-32kHz"
#define OPAL_PCM16_48KHZ    "PCM-16-48kHz"
#define OPAL_PCM16S_48KHZ   "PCM-16S-48kHz"
#define OPAL_L16_MONO_8KHZ  "Linear-16-Mono-8kHz"
#define OPAL_L16_STEREO_8KHZ "Linear-16-Stereo-8kHz"
#define OPAL_L16_MONO_16KHZ "Linear-16-Mono-16kHz"
#define OPAL_L16_STEREO_16KHZ "Linear-16-Stereo-16kHz"
#define OPAL_L16_MONO_32KHZ "Linear-16-Mono-32kHz"
#define OPAL_L16_STEREO_32KHZ "Linear-16-Stereo-32kHz"
#define OPAL_L16_MONO_48KHZ "Linear-16-Mono-48kHz"
#define OPAL_L16_STEREO_48KHZ "Linear-16-Stereo-48kHz"
#define OPAL_G711_ULAW_64K  "G.711-uLaw-64k"
#define OPAL_G711_ALAW_64K  "G.711-ALaw-64k"
#define OPAL_G722           "G.722"
#define OPAL_G7221          "G.722.1"
#define OPAL_G7222          "G.722.2"
#define OPAL_G726_40K       "G.726-40K"
#define OPAL_G726_32K       "G.726-32K"
#define OPAL_G726_24K       "G.726-24K"
#define OPAL_G726_16K       "G.726-16K"
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
#define OPAL_GSMAMR         "GSM-AMR"
#define OPAL_iLBC           "iLBC"
#define OPAL_H261           "H.261"
#define OPAL_H263           "H.263"
#define OPAL_H264_MODE0     "H.264-0"
#define OPAL_H264_MODE1     "H.264-1"
#define OPAL_MPEG4          "MPEG4"
#define OPAL_RFC2833        "UserInput/RFC2833"
#define OPAL_CISCONSE       "NamedSignalEvent"
#define OPAL_T38            "T.38"

extern const OpalAudioFormat & GetOpalPCM16();
extern const OpalAudioFormat & GetOpalPCM16S();
extern const OpalAudioFormat & GetOpalPCM16_16KHZ();
extern const OpalAudioFormat & GetOpalPCM16S_16KHZ();
extern const OpalAudioFormat & GetOpalPCM16_32KHZ();
extern const OpalAudioFormat & GetOpalPCM16S_32KHZ();
extern const OpalAudioFormat & GetOpalPCM16_48KHZ();
extern const OpalAudioFormat & GetOpalPCM16S_48KHZ();
extern const OpalAudioFormat & GetOpalL16_MONO_8KHZ();
extern const OpalAudioFormat & GetOpalL16_STEREO_8KHZ();
extern const OpalAudioFormat & GetOpalL16_MONO_16KHZ();
extern const OpalAudioFormat & GetOpalL16_STEREO_16KHZ();
extern const OpalAudioFormat & GetOpalL16_MONO_32KHZ();
extern const OpalAudioFormat & GetOpalL16_STEREO_32KHZ();
extern const OpalAudioFormat & GetOpalL16_MONO_48KHZ();
extern const OpalAudioFormat & GetOpalL16_STEREO_48KHZ();
extern const OpalAudioFormat & GetOpalG711_ULAW_64K();
extern const OpalAudioFormat & GetOpalG711_ALAW_64K();
extern const OpalAudioFormat & GetOpalG722();
extern const OpalAudioFormat & GetOpalG7221();
extern const OpalAudioFormat & GetOpalG7222();
extern const OpalAudioFormat & GetOpalG726_40K();
extern const OpalAudioFormat & GetOpalG726_32K();
extern const OpalAudioFormat & GetOpalG726_24K();
extern const OpalAudioFormat & GetOpalG726_16K();
extern const OpalAudioFormat & GetOpalG728();
extern const OpalAudioFormat & GetOpalG729();
extern const OpalAudioFormat & GetOpalG729A();
extern const OpalAudioFormat & GetOpalG729B();
extern const OpalAudioFormat & GetOpalG729AB();
extern const OpalAudioFormat & GetOpalG7231_6k3();
extern const OpalAudioFormat & GetOpalG7231_5k3();
extern const OpalAudioFormat & GetOpalG7231A_6k3();
extern const OpalAudioFormat & GetOpalG7231A_5k3();
extern const OpalAudioFormat & GetOpalGSM0610();
extern const OpalAudioFormat & GetOpalGSMAMR();
extern const OpalAudioFormat & GetOpaliLBC();

extern const OpalMediaFormat & GetOpalRFC2833();

#if OPAL_T38_CAPABILITY
extern const OpalMediaFormat & GetOpalCiscoNSE();
#if OPAL_PTLIB_ASN
extern const OpalMediaFormat & GetOpalT38();
#endif
#endif


#define OpalPCM16          GetOpalPCM16()
#define OpalPCM16S         GetOpalPCM16S()
#define OpalPCM16_16KHZ    GetOpalPCM16_16KHZ()
#define OpalPCM16S_16KHZ   GetOpalPCM16S_16KHZ()
#define OpalPCM16_32KHZ    GetOpalPCM16_32KHZ()
#define OpalPCM16S_32KHZ   GetOpalPCM16S_32KHZ()
#define OpalPCM16_48KHZ    GetOpalPCM16_48KHZ()
#define OpalPCM16S_48KHZ   GetOpalPCM16S_48KHZ()
#define OpalL16_MONO_8KHZ  GetOpalL16_MONO_8KHZ()
#define OpalL16_STEREO_8KHZ GetOpalL16_STEREO_8KHZ()
#define OpalL16_MONO_16KHZ GetOpalL16_MONO_16KHZ()
#define OpalL16_STEREO_16KHZ GetOpalL16_STEREO_16KHZ()
#define OpalL16_MONO_32KHZ GetOpalL16_MONO_32KHZ()
#define OpalL16_STEREO_32KHZ GetOpalL16_STEREO_32KHZ()
#define OpalL16_MONO_48KHZ GetOpalL16_MONO_48KHZ()
#define OpalL16_STEREO_48KHZ GetOpalL16_STEREO_48KHZ()
#define OpalG711_ULAW_64K  GetOpalG711_ULAW_64K()
#define OpalG711_ALAW_64K  GetOpalG711_ALAW_64K()
#define OpalG722           GetOpalG722()
#define OpalG7221          GetOpalG7221()
#define OpalG7222          GetOpalG7222()
#define OpalG726_40K       GetOpalG726_40K()
#define OpalG726_32K       GetOpalG726_32K()
#define OpalG726_24K       GetOpalG726_24K()
#define OpalG726_16K       GetOpalG726_16K()
#define OpalG728           GetOpalG728()
#define OpalG729           GetOpalG729()
#define OpalG729A          GetOpalG729A()
#define OpalG729B          GetOpalG729B()
#define OpalG729AB         GetOpalG729AB()
#define OpalG7231_6k3      GetOpalG7231_6k3()
#define OpalG7231_5k3      GetOpalG7231_5k3()
#define OpalG7231A_6k3     GetOpalG7231A_6k3()
#define OpalG7231A_5k3     GetOpalG7231A_5k3()
#define OpalGSM0610        GetOpalGSM0610()
#define OpalGSMAMR         GetOpalGSMAMR()
#define OpaliLBC           GetOpaliLBC()
#define OpalRFC2833        GetOpalRFC2833()
#define OpalCiscoNSE       GetOpalCiscoNSE()
#define OpalT38            GetOpalT38()

#define OpalL16Mono8kHz    OpalL16_MONO_8KHZ
#define OpalL16Mono16kHz   OpalL16_MONO_16KHZ
#define OpalG711uLaw       OpalG711_ULAW_64K
#define OpalG711ALaw       OpalG711_ALAW_64K

#define OPAL_T140             "T.140"
#define OpalT140              GetOpalT140()
extern const OpalMediaFormat & GetOpalT140();

#if OPAL_HAS_MSRP
#define OPAL_MSRP           "MSRP"
#define OpalMSRP            GetOpalMSRP()
extern const OpalMediaFormat & GetOpalMSRP();
#endif

#if OPAL_HAS_SIPIM
#define OPAL_SIPIM             "SIP-IM"
#define OpalSIPIM              GetOpalSIPIM()
extern const OpalMediaFormat & GetOpalSIPIM();
#endif

#ifdef _MSC_VER
#if _MSC_VER < 1300
#pragma warning(default:4663)
#endif
#endif

#endif  // OPAL_OPAL_MEDIAFMT_H


// End of File ///////////////////////////////////////////////////////////////
