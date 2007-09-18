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
 * Revision 1.2060  2007/09/18 12:52:34  rjongbloed
 * Removed duplicate change logs
 *
 * Revision 2.58  2007/09/12 04:19:53  rjongbloed
 * CHanges to avoid creation of long duration OpalMediaFormat instances, eg in
 *   the plug in capabilities, that then do not get updated values from the master
 *   list, or worse from the user modified master list, causing much confusion.
 *
 * Revision 2.57  2007/09/10 00:11:13  rjongbloed
 * AddedOpalMediaFormat::IsTransportable() function as better test than simply
 *   checking the payload type, condition is more complex.
 *
 * Revision 2.56  2007/08/10 09:30:18  rjongbloed
 * Fixed typos in comments
 *
 * Revision 2.55  2007/08/07 01:38:40  csoutheren
 * Fix problem with rtpEncodingName member going out of scope
 *
 * Revision 2.54  2007/08/02 07:54:15  csoutheren
 * Add function to print options on media format
 *
 * Revision 2.53  2007/07/24 12:51:26  rjongbloed
 * Fixed odd problem where need to allow for sign bit on an unsigned enum in a struct bitfield.
 *
 * Revision 2.52  2007/06/27 07:56:08  rjongbloed
 * Add new OpalMediaOption for octet strings (simple block of bytes).
 *
 * Revision 2.51  2007/06/22 05:41:47  rjongbloed
 * Major codec API update:
 *   Automatically map OpalMediaOptions to SIP/SDP FMTP parameters.
 *   Automatically map OpalMediaOptions to H.245 Generic Capability parameters.
 *   Largely removed need to distinguish between SIP and H.323 codecs.
 *   New mechanism for setting OpalMediaOptions from within a plug in.
 *
 * Revision 2.50  2007/06/16 21:36:59  dsandras
 * Added H.264 support thanks to Matthias Schneider <ma30002000 yahoo de>.
 * Thanks a lot !
 *
 * Baseline Profile:
 * no B-frames
 * We make use of the baseline profile (which is the designated profile for interactive vide) ,
 * that means:
 * no B-Frames (too much latency in interactive video)
 * CBR (we want to get the max. quality making use of all the bitrate that is available)
 * We allow one exeption: configuring a bitrate of > 786 kbit/s
 *
 * This plugin implements
 * - Single Time Aggregation Packets A
 * - Single NAL units
 * - Fragmentation Units
 * like described in RFC3984
 *
 * It requires x264 and ffmpeg.
 *
 * Revision 2.49  2007/04/10 05:15:53  rjongbloed
 * Fixed issue with use of static C string variables in DLL environment,
 *   must use functional interface for correct initialisation.
 *
 * Revision 2.48  2007/03/13 00:32:16  csoutheren
 * Simple but messy changes to allow compile time removal of protocol
 * options such as H.450 and H.460
 * Fix MakeConnection overrides
 *
 * Revision 2.47  2007/02/14 06:51:28  csoutheren
 * Extended FindFormat to allow finding multiple matching formats
 *
 * Revision 2.46  2007/02/10 18:14:31  hfriederich
 * Add copy constructor to have consistent code with assignment operator.
 * Only make options unique when they actually differ
 *
 * Revision 2.45  2006/12/08 07:33:13  csoutheren
 * Fix problem with wideband audio plugins and sound channel
 *
 * Revision 2.44  2006/11/21 01:00:59  csoutheren
 * Ensure SDP only uses codecs that are valid for SIP
 *
 * Revision 2.43  2006/08/20 03:45:54  csoutheren
 * Add OpalMediaFormat::IsValidForProtocol to allow plugin codecs to be enabled only for certain protocols
 * rather than relying on the presence of the IANA rtp encoding name field
 *
 * Revision 2.42  2006/08/11 07:52:01  csoutheren
 * Fix problem with media format factory in VC 2005
 * Fixing problems with Speex codec
 * Remove non-portable usages of PFactory code
 *
 * Revision 2.41  2006/07/24 14:03:38  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.40  2006/07/14 04:22:42  csoutheren
 * Applied 1517397 - More Phobos stability fix
 * Thanks to Dinis Rosario
 *
 * Revision 2.39  2006/04/20 16:52:22  hfriederich
 * Adding support for H.224/H.281
 *
 * Revision 2.38  2006/04/09 12:01:43  rjongbloed
 * Added missing Clone() functions so media options propagate correctly.
 *
 * Revision 2.37.4.6  2006/04/26 05:05:59  csoutheren
 * H.263 decoding working via codec plugin
 *
 * Revision 2.37.4.5  2006/04/19 04:58:56  csoutheren
 * Debugging and testing of new video plugins
 * H.261 working in both CIF and QCIF modes in H.323
 *
 * Revision 2.37.4.4  2006/04/10 06:24:30  csoutheren
 * Backport from CVS head up to Plugin_Merge3
 *
 * Revision 2.37.4.3  2006/04/06 01:21:17  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 2.37.4.2  2006/03/16 07:06:00  csoutheren
 * Initial support for audio plugins
 *
 * Revision 2.37.4.1  2006/03/13 07:20:28  csoutheren
 * Added OpalMediaFormat clone function
 *

 * Revision 2.37  2005/12/27 20:46:09  dsandras
 * Added clockRate to the media format. Added "AlwaysMerge" method for merging
 * media format options.
 *
 * Revision 2.36  2005/12/24 17:51:02  dsandras
 * Added clockRate parameter to allow wideband audio codecs.
 *
 * Revision 2.35  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.34  2005/09/13 20:48:22  dominance
 * minor cleanups needed to support mingw compilation. Thanks goes to Julien Puydt.
 *
 * Revision 2.33  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.32  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.31  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.30  2005/08/24 02:07:56  dereksmithies
 * Put guard around a MSVC pragma, so GCC does not generate zillions of warnings.
 *
 * Revision 2.29  2005/08/22 01:26:25  shorne
 * Removed warning on numeric_limits on MSVC6
 *
 * Revision 2.28  2005/08/20 07:32:49  rjongbloed
 * Added video specific OpalMediaFormat
 *
 * Revision 2.27  2005/07/11 01:42:21  csoutheren
 * Fixed problems with some constants names not being available
 *
 * Revision 2.26  2005/06/20 16:47:52  shorne
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

#ifdef _MSC_VER
#if _MSC_VER < 1300   
#pragma warning(disable:4663)
#endif
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
      const OpalMediaFormat & format    ///<  Format to add
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

    /**Get a format position in the list matching the payload type.

       Returns P_MAX_INDEX if not in list.
      */
    PINDEX FindFormat(
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      const unsigned clockRate,                   ///<  clock rate
      const char * rtpEncodingName = NULL,        ///<  RTP payload type name
      const char * protocol = NULL                ///<  protocol to be valid for (if NULL, then all)
    ) const;

    /**Get a format position in the list matching the wildcard.
       The wildcard string is a simple substring match using the '*'
       character. For example: "G.711*" would match "G.711-uLaw-64k" and
       "G.711-ALaw-64k".

       Returns P_MAX_INDEX if not in list.
      */
    PINDEX FindFormat(
      const PString & wildcard,    ///<  Wildcard string name.
      PINDEX pos = 0
    ) const;

    /**Determine if a format matching the payload type is in the list.
      */
    BOOL HasFormat(
      RTP_DataFrame::PayloadTypes rtpPayloadType ///<  RTP payload type code
    ) const { return FindFormat(rtpPayloadType) != P_MAX_INDEX; }

    /**Determine if a format matching the wildcard is in the list.
       The wildcard string is a simple substring match using the '*'
       character. For example: "G.711*" would match "G.711-uLaw-64k" and
       "G.711-ALaw-64k".
      */
    BOOL HasFormat(
      const PString & wildcard    ///<  Wildcard string name.
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
      AlwaysMerge,

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

    const PString & GetFMTPName() const { return m_FMTPName; }
    void SetFMTPName(const char * name) { m_FMTPName = name; }

    const PString & GetFMTPDefault() const { return m_FMTPDefault; }
    void SetFMTPDefault(const char * value) { m_FMTPDefault = value; }

    struct H245GenericInfo {
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
    void SetH245Generic(const H245GenericInfo & generic) { m_H245Generic = generic; }

  protected:
    PCaselessString m_name;
    bool            m_readOnly;
    MergeType       m_merge;
    PCaselessString m_FMTPName;
    PString         m_FMTPDefault;
    H245GenericInfo m_H245Generic;
};

#ifndef __USE_STL__
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
#ifdef __USE_STL__
	   strm.setstate(ios::badbit);
#else
	   strm.setf(ios::badbit , ios::badbit);
#endif
       }
    }

    virtual Comparison CompareValue(const OpalMediaOption & option) const {
      const OpalMediaOptionValue * otherOption = PDownCast(const OpalMediaOptionValue, &option);
      if (otherOption == NULL)
        return GreaterThan;
      if (m_value < otherOption->m_value)
        return LessThan;
      if (m_value > otherOption->m_value)
        return GreaterThan;
      return EqualTo;
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


typedef OpalMediaOptionValue<bool>     OpalMediaOptionBoolean;
typedef OpalMediaOptionValue<int>      OpalMediaOptionInteger;
typedef OpalMediaOptionValue<unsigned> OpalMediaOptionUnsigned;
typedef OpalMediaOptionValue<double>   OpalMediaOptionReal;


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
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual Comparison CompareValue(const OpalMediaOption & option) const;
    virtual void Assign(const OpalMediaOption & option);

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
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

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
      bool base64
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

  protected:
    PBYTEArray m_value;
    bool       m_base64;
};


///////////////////////////////////////////////////////////////////////////////

/**This class describes a media format as used in the OPAL system. A media
   format is the type of any media data that is trasferred between OPAL
   entities. For example an audio codec such as G.723.1 is a media format, a
   video codec such as H.261 is also a media format.
  */
class OpalMediaFormat : public PCaselessString
{
  friend class OpalPluginCodecManager;
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
      const char * fullName,      ///<  Full name of media format
      unsigned defaultSessionID,  ///<  Default session for codec type
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      const char * encodingName,  ///<  RTP encoding name
      BOOL     needsJitter,       ///<  Indicate format requires a jitter buffer
      unsigned bandwidth,         ///<  Bandwidth in bits/second
      PINDEX   frameSize,         ///<  Size of frame in bytes (if applicable)
      unsigned frameTime,         ///<  Time for frame in RTP units (if applicable)
      unsigned clockRate,         ///<  Clock rate for data (if applicable)
      time_t timeStamp = 0        ///<  timestamp (for versioning)
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
    
    /**Copy Constructor
      */
    OpalMediaFormat(
      const OpalMediaFormat & mediaFormat ///< other media format
    );

    /**Return TRUE if media format info is valid. This may be used if the
       single string constructor is used to check that it matched something
       in the registered media formats database.
      */
    virtual BOOL IsValid() const { return rtpPayloadType < RTP_DataFrame::IllegalPayloadType && !IsEmpty(); }

    /**Return TRUE if media format info may be sent via RTP. Some formats are internal
       use only and are never transported "over the wire".
      */
    BOOL IsTransportable() const { return rtpPayloadType < RTP_DataFrame::MaxPayloadType && !rtpEncodingName.IsEmpty(); }

    /**Copy a media format
      */
    OpalMediaFormat & operator=(
      const OpalMediaFormat & fmt ///<  other media format
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

    /**Merge with another media format. This will alter and validate
       the options for this media format according to the merge rule for
       each option. The parameter is typically a "capability" while the
       current object isthe proposed channel format. This if the current
       object has a tx number of frames of 3, but the parameter has a value
       of 1, then the current object will be set to 1.

       Returns FALSE if the media formats are incompatible and cannot be
       merged.
      */
    virtual bool Merge(
      const OpalMediaFormat & mediaFormat
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
      DefaultDataSessionID  = 3,
      DefaultH224SessionID  = 4
    };

    /**Get the default session ID for media format.
      */
    unsigned GetDefaultSessionID() const { return defaultSessionID; }

    /**Determine if the media format requires a jitter buffer. As a rule an
       audio codec needs a jitter buffer and all others do not.
      */
    bool NeedsJitterBuffer() const { return GetOptionBoolean(NeedsJitterOption()); }
    static const PString & NeedsJitterOption();

    /**Get the average bandwidth used in bits/second.
      */
    unsigned GetBandwidth() const { return GetOptionInteger(MaxBitRateOption()); }
    static const PString & MaxBitRateOption();

    /**Get the maximum frame size in bytes. If this returns zero then the
       media format has no intrinsic maximum frame size, eg G.711 would 
       return zero but G.723.1 whoud return 24.
      */
    PINDEX GetFrameSize() const { return GetOptionInteger(MaxFrameSizeOption()); }
    static const PString & MaxFrameSizeOption();

    /**Get the frame time in RTP timestamp units. If this returns zero then
       the media format is not real time and has no intrinsic timing eg T.120
      */
    unsigned GetFrameTime() const { return GetOptionInteger(FrameTimeOption()); }
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
    unsigned GetClockRate() const { return GetOptionInteger(ClockRateOption()); }
    static const PString & ClockRateOption();

    /**Get the number of options this media format has.
      */
    PINDEX GetOptionCount() const { return options.GetSize(); }

    /**Get the option instance at the specified index. This contains the
       description and value for the option.
      */
    const OpalMediaOption & GetOption(
      PINDEX index   ///<  Index of option in list to get
    ) const { return options[index]; }

    /**Get the option value of the specified name as a string.

       Returns false of the option is not present.
      */
    bool GetOptionValue(
      const PString & name,   ///<  Option name
      PString & value         ///<  String to receive option value
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
      bool dflt = FALSE       ///<  Default value if option not present
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

    /**Get the option value of the specified name as an octet array.
       Returns FALSE if not present.
      */
    bool GetOptionOctets(
      const PString & name, ///<  Option name
      PBYTEArray & octets   ///<  Octets in option
    ) const;

    /**Set the option value of the specified name as an octet array.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionOctets(
      const PString & name,       ///<  Option name
      const PBYTEArray & octets   ///<  Octets in option
    );
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
      BOOL overwrite = FALSE
    );

    /**
      * Determine if media format has the specified option.
      */
    bool HasOption(const PString & name) const
    { return FindOption(name) != NULL; }

    /**
      * Get a pointer to the specified media format option.
      * Returns NULL if thee option does not exist.
      */
    OpalMediaOption * FindOption(
      const PString & name
    ) const;

    /** Returns TRUE if the media format is valid for the protocol specified
        This allow plugin codecs to customise which protocols they are valid for
        The default implementation returns true unless the protocol is H.323
        and the rtpEncodingName is NULL
      */
    virtual bool IsValidForProtocol(const PString & protocol) const;

    virtual time_t GetCodecBaseTime() const;

    virtual ostream & PrintOptions(ostream & strm) const;

  protected:
    RTP_DataFrame::PayloadTypes  rtpPayloadType;
    PString                      rtpEncodingName;
    unsigned                     defaultSessionID;
    PMutex                       media_format_mutex;
    PSortedList<OpalMediaOption> options;
    time_t codecBaseTime;

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

#if OPAL_AUDIO
class OpalAudioFormat : public OpalMediaFormat
{
  friend class OpalPluginCodecManager;
    PCLASSINFO(OpalAudioFormat, OpalMediaFormat);
  public:
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
};
#endif

#if OPAL_VIDEO
class OpalVideoFormat : public OpalMediaFormat
{
  friend class OpalPluginCodecManager;
    PCLASSINFO(OpalVideoFormat, OpalMediaFormat);
  public:
    OpalVideoFormat(
      const char * fullName,    ///<  Full name of media format
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      const char * encodingName,///<  RTP encoding name
      unsigned frameWidth,      ///<  Width of video frame
      unsigned frameHeight,     ///<  Height of video frame
      unsigned frameRate,       ///<  Number of frames per second
      unsigned bitRate,         ///<  Maximum bits per second
      time_t timeStamp = 0        ///<  timestamp (for versioning)
    );

    virtual PObject * Clone() const;

    virtual bool Merge(const OpalMediaFormat & mediaFormat);

    static const PString & FrameWidthOption();
    static const PString & FrameHeightOption();
    static const PString & EncodingQualityOption();
    static const PString & TargetBitRateOption();
    static const PString & DynamicVideoQualityOption();
    static const PString & AdaptivePacketDelayOption();
};
#endif

// List of known media formats

#define OPAL_PCM16          "PCM-16"
#define OPAL_PCM16_16KHZ    "PCM-16-16kHz"
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
#define OPAL_CISCONSE       "NamedSignalEvent"

extern const OpalAudioFormat & GetOpalPCM16();
extern const OpalAudioFormat & GetOpalPCM16_16KHZ();
extern const OpalAudioFormat & GetOpalL16_MONO_8KHZ();
extern const OpalAudioFormat & GetOpalL16_MONO_16KHZ();
extern const OpalAudioFormat & GetOpalG711_ULAW_64K();
extern const OpalAudioFormat & GetOpalG711_ALAW_64K();
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
extern const OpalMediaFormat & GetOpalRFC2833();
extern const OpalMediaFormat & GetOpalCiscoNSE();

#define OpalPCM16          GetOpalPCM16()
#define OpalPCM16_16KHZ    GetOpalPCM16_16KHZ()
#define OpalL16_MONO_8KHZ  GetOpalL16_MONO_8KHZ()
#define OpalL16_MONO_16KHZ GetOpalL16_MONO_16KHZ()
#define OpalG711_ULAW_64K  GetOpalG711_ULAW_64K()
#define OpalG711_ALAW_64K  GetOpalG711_ALAW_64K()
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
#define OpalRFC2833        GetOpalRFC2833()
#define OpalCiscoNSE       GetOpalCiscoNSE()

#define OpalL16Mono8kHz    OpalL16_MONO_8KHZ
#define OpalL16Mono16kHz   OpalL16_MONO_16KHZ
#define OpalG711uLaw       OpalG711_ULAW_64K
#define OpalG711ALaw       OpalG711_ALAW_64K


#ifdef _MSC_VER
#if _MSC_VER < 1300
#pragma warning(default:4663)
#endif
#endif

#endif  // __OPAL_MEDIAFMT_H


// End of File ///////////////////////////////////////////////////////////////
