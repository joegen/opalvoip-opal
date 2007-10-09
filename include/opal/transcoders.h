/*
 * transcoders.h
 *
 * Abstractions for converting media from one format to another.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: transcoders.h,v $
 * Revision 1.2029.4.2  2007/10/09 09:19:00  rjongbloed
 * Update fixes from HEAD
 *
 * Revision 2.29  2007/10/08 01:45:16  rjongbloed
 * Fixed bad virtual function causing uninitialised variable whcih prevented video from working.
 * Some more clean ups.
 *
 * Revision 2.28  2007/09/19 10:43:00  csoutheren
 * Exposed G.7231 capability class
 * Added macros to create empty transcoders and capabilities
 *
 * Revision 2.27  2007/09/10 03:15:04  rjongbloed
 * Fixed issues in creating and subsequently using correctly unique
 *   payload types in OpalMediaFormat instances and transcoders.
 *
 * Revision 2.26  2007/06/22 05:47:19  rjongbloed
 * Fixed setting of output RTP payload types on plug in video codecs.
 *
 * Revision 2.25  2007/03/29 05:22:32  csoutheren
 * Add extra logging
 *
 * Revision 2.24  2006/11/29 06:28:57  csoutheren
 * Add ability call codec control functions on all transcoders
 *
 * Revision 2.23  2006/04/09 12:12:54  rjongbloed
 * Changed the media format option merging to include the transcoder formats.
 *
 * Revision 2.22  2006/02/02 07:02:57  csoutheren
 * Added RTP payload map to transcoders and connections to allow remote SIP endpoints
 * to change the payload type used for outgoing RTP.
 *
 * Revision 2.21  2005/12/30 14:33:12  dsandras
 * Added support for Packet Loss Concealment frames for framed codecs supporting it similarly to what was done for OpenH323.
 *
 * Revision 2.20  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.19  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.18  2005/09/04 06:23:38  rjongbloed
 * Added OpalMediaCommand mechanism (via PNotifier) for media streams
 *   and media transcoders to send commands back to remote.
 *
 * Revision 2.17  2005/09/02 14:31:40  csoutheren
 * Use inline function to work around compiler foo in gcc
 *
 * Revision 2.16  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.15  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.14  2005/03/19 04:08:10  csoutheren
 * Fixed warnings with gcc snapshot 4.1-20050313
 * Updated to configure 2.59
 *
 * Revision 2.13  2005/02/17 03:25:05  csoutheren
 * Added support for audio codecs that consume and produce variable size
 * frames, such as G.723.1
 *
 * Revision 2.12  2004/07/11 12:34:48  rjongbloed
 * Added function to get a list of all possible media formats that may be used given
 *   a list of media and taking into account all of the registered transcoders.
 *
 * Revision 2.11  2004/03/22 11:32:41  rjongbloed
 * Added new codec type for 16 bit Linear PCM as must distinguish between the internal
 *   format used by such things as the sound card and the RTP payload format which
 *   is always big endian.
 *
 * Revision 2.10  2004/03/11 06:54:27  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.9  2004/02/17 08:47:38  csoutheren
 * Changed codec loading macros to work with Linux
 *
 * Revision 2.8  2004/01/18 15:35:20  rjongbloed
 * More work on video support
 *
 * Revision 2.7  2003/06/02 02:59:43  rjongbloed
 * Changed transcoder search so uses destination list as preference order.
 *
 * Revision 2.6  2003/03/24 04:32:11  robertj
 * Fixed macro for transcoder with parameter (not used yet!)
 * Fixed so OPAL_NO_PARAM can be defined in other modules.
 *
 * Revision 2.5  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.4  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.3  2002/02/13 02:30:21  robertj
 * Added ability for media patch (and transcoders) to handle multiple RTP frames.
 *
 * Revision 2.2  2002/01/22 05:07:02  robertj
 * Added ability to get input and output media format names from transcoder.
 *
 * Revision 2.1  2001/08/01 05:52:08  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 * Added functions to aid in determining if a transcoder can be used to get
 *   to another media format.
 * Fixed problem with streamed transcoder used in G.711.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __OPAL_TRANSCODERS_H
#define __OPAL_TRANSCODERS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <opal/mediacmd.h>

#include <rtp/rtp.h>

class RTP_DataFrame;
class OpalTranscoder;


///////////////////////////////////////////////////////////////////////////////

/** This class described an OpalTranscoder in terms of a pair of
    OpalMediaFormat instances, for input and output.
  */
class OpalMediaFormatPair : public PObject
{
    PCLASSINFO(OpalMediaFormatPair, PObject);
  public:
  /**@name Construction */
  //@{
    /** Create a new transcoder implementation.
      */
    OpalMediaFormatPair(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const;

    /** Compare the two objects and return their relative rank. This function is
       usually overridden by descendent classes to yield the ranking according
       to the semantics of the object.
       
       The default function is to use the #CompareObjectMemoryDirect()#
       function to do a byte wise memory comparison of the two objects.

       @return
       #LessThan#, #EqualTo# or #GreaterThan#
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   ///<  Object to compare against.
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Get the names of the input or output formats.
      */
    const OpalMediaFormat & GetInputFormat() const { return inputMediaFormat; }

    /**Get the names of the input or output formats.
      */
    const OpalMediaFormat & GetOutputFormat() const { return outputMediaFormat; }
  //@}

  protected:
    OpalMediaFormat inputMediaFormat;
    OpalMediaFormat outputMediaFormat;
};


typedef std::pair<PString, PString>                                      OpalTranscoderKey;
typedef PFactory<OpalTranscoder, OpalTranscoderKey>                      OpalTranscoderFactory;
typedef PFactory<OpalTranscoder, OpalTranscoderKey>::KeyList_T           OpalTranscoderList;
typedef PFactory<OpalTranscoder, OpalTranscoderKey>::KeyList_T::iterator OpalTranscoderIterator;

#define OPAL_REGISTER_TRANSCODER(cls, input, output) \
  OpalTranscoderFactory::Worker<cls> OpalTranscoder_##cls(OpalTranscoderKey(input, output))


/**This class embodies the implementation of a specific transcoder instance
   used to convert data from one format to another.

   An application may create a descendent off this class and override
   functions as required for implementing a transcoder.
 */
class OpalTranscoder : public OpalMediaFormatPair
{
    PCLASSINFO(OpalTranscoder, OpalMediaFormatPair);
  public:
  /**@name Construction */
  //@{
    /** Create a new transcoder implementation.
      */
    OpalTranscoder(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );
  //@}

  /**@name Operations */
  //@{
    /**Update the output media format. This can be used to adjust the
       parameters of a codec at run time. Note you cannot change the basic
       media format, eg change GSM0610 to G.711, only options for that
       format, eg 6k3 mode to 5k3 mode in G.723.1.

       The default behaviour updates the outputMediaFormat member variable
       and sets the outputMediaFormatUpdated flag.
      */
    virtual BOOL UpdateOutputMediaFormat(
      const OpalMediaFormat & mediaFormat  ///<  New media format
    );

    /**Execute the command specified to the transcoder. The commands are
       highly context sensitive, for example VideoFastUpdate would only apply
       to a video transcoder.

       The default behaviour simply returns FALSE.
      */
    virtual BOOL ExecuteCommand(
      const OpalMediaCommand & command    ///<  Command to execute.
    );

    /**Get the optimal size for data frames to be converted.
       This function returns the size of frames that will be most efficient
       in conversion. A RTP_DataFrame will attempt to provide or use data in
       multiples of this size. Note that it may not do so, so the transcoder
       must be able to handle any sized packets.
      */
    virtual PINDEX GetOptimalDataFrameSize(
      BOOL input      ///<  Flag for input or output data size
    ) const = 0;

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it (possibly) into multiple RTP_DataFrame
       objects.

       The default behaviour makes sure the output list has only one element
       in it and calls the Convert() function.

       Returns FALSE if the conversion fails.
      */
    virtual BOOL ConvertFrames(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrameList & output    ///<  Output data
    );

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       Returns FALSE if the conversion fails.
      */
    virtual BOOL Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    ) = 0;

    /**Create an instance of a media conversion function.
       Returns NULL if there is no registered media transcoder between the two
       named formats.
      */
    static OpalTranscoder * Create(
      const OpalMediaFormat & srcFormat,  ///<  Name of source format
      const OpalMediaFormat & dstFormat,  ///<  Name of destination format
      const BYTE * instance = NULL,       ///<  Unique instance identifier for transcoder
      unsigned instanceLen = 0            ///<  Length of instance identifier
    );

    /**Find media format(s) for transcoders.
       This function attempts to find and intermediate media format that will
       allow two transcoders to be used to get data from the source format to
       the destination format.

       There could be many possible matches between the two lists, so
       preference is given to the order of the destination formats.

       Returns FALSE if there is no registered media transcoder that can be used
       between the two named formats.
      */
    static BOOL SelectFormats(
      unsigned sessionID,               ///<  Session ID for media formats
      const OpalMediaFormatList & srcFormats, ///<  Names of possible source formats
      const OpalMediaFormatList & dstFormats, ///<  Names of possible destination formats
      OpalMediaFormat & srcFormat,      ///<  Selected source format to be used
      OpalMediaFormat & dstFormat       ///<  Selected destination format to be used
    );

    /**Find media intermediate format for transcoders.
       This function attempts to find the intermediate media format that will
       allow two transcoders to be used to get data from the source format to
       the destination format.

       Returns FALSE if there is no registered media transcoder that can be used
       between the two named formats.
      */
    static BOOL FindIntermediateFormat(
      OpalMediaFormat & srcFormat,          ///<  Selected destination format to be used
      OpalMediaFormat & dstFormat,          ///<  Selected destination format to be used
      OpalMediaFormat & intermediateFormat  ///<  Intermediate format that can be used
    );

    /**Get a list of possible destination media formats for the destination.
      */
    static OpalMediaFormatList GetDestinationFormats(
      const OpalMediaFormat & srcFormat    ///<  Selected source format
    );

    /**Get a list of possible source media formats for the destination.
      */
    static OpalMediaFormatList GetSourceFormats(
      const OpalMediaFormat & dstFormat    ///<  Selected destination format
    );

    /**Get a list of possible media formats that can do bi-directional media.
      */
    static OpalMediaFormatList GetPossibleFormats(
      const OpalMediaFormatList & formats    ///<  Destination format list
    );
  //@}

  /**@name Operations */
  //@{
    /**Get maximum output size.
      */
    PINDEX GetMaxOutputSize() const { return maxOutputSize; }

    /**Set the maximum output size.
      */
    void SetMaxOutputSize(
      PINDEX size
    ) { maxOutputSize = size; }

    /**Set a notifier to receive commands generated by the transcoder. The
       commands are highly context sensitive, for example VideoFastUpdate
       would only apply to a video transcoder.
      */
    void SetCommandNotifier(
      const PNotifier & notifier    ///<  Command to execute.
    ) { commandNotifier = notifier; }

    /**Get the notifier to receive commands generated by the transcoder. The
       commands are highly context sensitive, for example VideoFastUpdate
       would only apply to a video transcoder.
      */
    const PNotifier & GetCommandNotifier() const { return commandNotifier; }

    /** Set the unique instance identifier for transcoder
      */
    virtual void SetInstanceID(
      const BYTE * instance,              ///<  Unique instance identifier for transcoder
      unsigned instanceLen                ///<  Length of instance identifier
    );

    void SetRTPPayloadMap(const RTP_DataFrame::PayloadMapType & v)
    { payloadTypeMap = v; }

    void AddRTPPayloadMapping(RTP_DataFrame::PayloadTypes from, RTP_DataFrame::PayloadTypes to)
    { payloadTypeMap.insert(RTP_DataFrame::PayloadMapType::value_type(from, to)); }

    RTP_DataFrame::PayloadTypes GetPayloadType(
      BOOL input      ///<  Flag for input or output data size
    ) const;
  //@}

  protected:
    PINDEX    maxOutputSize;
    bool      outputMediaFormatUpdated;
    PNotifier commandNotifier;
    PMutex    updateMutex;

    RTP_DataFrame::PayloadMapType payloadTypeMap;

    BOOL outputIsRTP, inputIsRTP;
};


/**This class defines a transcoder implementation class that will
   encode/decode fixed sized blocks. That is each input block of n bytes
   is encoded to exactly m bytes of data, eg GSM etc.

   An application may create a descendent off this class and override
   functions as required for descibing a specific transcoder.
 */
class OpalFramedTranscoder : public OpalTranscoder
{
    PCLASSINFO(OpalFramedTranscoder, OpalTranscoder);
  public:
  /**@name Construction */
  //@{
    /** Create a new framed transcoder implementation.
      */
    OpalFramedTranscoder(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat, ///<  Output media format
      PINDEX inputBytesPerFrame,  ///<  Number of bytes in an input frame
      PINDEX outputBytesPerFrame  ///<  Number of bytes in an output frame
    );
  //@}

  /**@name Operations */
  //@{
    /**Get the optimal size for data frames to be converted.
       This function returns the size of frames that will be most efficient
       in conversion. A RTP_DataFrame will attempt to provide or use data in
       multiples of this size. Note that it may not do so, so the transcoder
       must be able to handle any sized packets.
      */
    virtual PINDEX GetOptimalDataFrameSize(
      BOOL input      ///<  Flag for input or output data size
    ) const;

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       Returns FALSE if the conversion fails.
      */
    virtual BOOL Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    );

    /**Convert a frame of data from one format to another.
       This function implicitly knows the input and output frame sizes.
      */
    virtual BOOL ConvertFrame(
      const BYTE * input,   ///<  Input data
      BYTE * output         ///<  Output data
    );
    virtual BOOL ConvertFrame(
      const BYTE * input,   ///<  Input data
      PINDEX & consumed,    ///<  number of input bytes consumed
      BYTE * output,        ///<  Output data
      PINDEX & created      ///<  number of output bytes created  
    );
    virtual BOOL ConvertSilentFrame(
      BYTE * output         ///<  Output data
    );
  //@}

  protected:
    PINDEX     inputBytesPerFrame;
    PINDEX     outputBytesPerFrame;
    //PBYTEArray partialFrame;
    //PINDEX     partialBytes;
};


/**This class defines a transcoder implementation class where the encode/decode
   is streamed. That is each input n bit PCM sample is encoded to m bits of
   encoded data, eg G.711 etc.

   An application may create a descendent off this class and override
   functions as required for descibing a specific transcoder.
 */
class OpalStreamedTranscoder : public OpalTranscoder
{
    PCLASSINFO(OpalStreamedTranscoder, OpalTranscoder);
  public:
  /**@name Construction */
  //@{
    /** Create a new streamed transcoder implementation.
      */
    OpalStreamedTranscoder(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat, ///<  Output media format
      unsigned inputBits,           ///<  Bits per sample in input data
      unsigned outputBits,          ///<  Bits per sample in output data
      PINDEX   optimalSamples       ///<  Optimal number of samples for read
    );
  //@}

  /**@name Operations */
  //@{
    /**Get the optimal size for data frames to be converted.
       This function returns the size of frames that will be most efficient
       in conversion. A RTP_DataFrame will attempt to provide or use data in
       multiples of this size. Note that it may not do so, so the transcoder
       must be able to handle any sized packets.
      */
    virtual PINDEX GetOptimalDataFrameSize(
      BOOL input      ///<  Flag for input or output data size
    ) const;

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       Returns FALSE if the conversion fails.
      */
    virtual BOOL Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    );

    /**Convert one sample from one format to another.
       This function takes the input data as a single sample value and
       converts it to its output format.

       Returns converted value.
      */
    virtual int ConvertOne(int sample) const = 0;
  //@}

  protected:
    unsigned inputBitsPerSample;
    unsigned outputBitsPerSample;
    PINDEX   optimalSamples;
};


///////////////////////////////////////////////////////////////////////////////

class Opal_Linear16Mono_PCM : public OpalStreamedTranscoder {
  public:
    Opal_Linear16Mono_PCM();
    virtual int ConvertOne(int sample) const;
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_Linear16Mono : public OpalStreamedTranscoder {
  public:
    Opal_PCM_Linear16Mono();
    virtual int ConvertOne(int sample) const;
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_L16_MONO() \
  OPAL_REGISTER_TRANSCODER(Opal_Linear16Mono_PCM, OpalL16_MONO_8KHZ, OpalPCM16); \
  OPAL_REGISTER_TRANSCODER(Opal_PCM_Linear16Mono, OpalPCM16,         OpalL16_MONO_8KHZ)


class OpalEmptyFramedAudioTranscoder : public OpalFramedTranscoder
{
  PCLASSINFO(OpalEmptyFramedAudioTranscoder, OpalFramedTranscoder);
  public:
    OpalEmptyFramedAudioTranscoder(const char * inFormat, const char * outFormat)
      : OpalFramedTranscoder(inFormat, outFormat, 100, 100)
    {  }

    BOOL ConvertFrame(const BYTE *, PINDEX &, BYTE *, PINDEX &)
    { return FALSE; }
};

#define OPAL_DECLARE_EMPTY_TRANSCODER(fmt) \
class Opal_Empty_##fmt##_Encoder : public OpalEmptyFramedAudioTranscoder \
{ \
  public: \
    Opal_Empty_##fmt##_Encoder() \
      : OpalEmptyFramedAudioTranscoder(OpalPCM16, fmt) \
    { } \
}; \
class Opal_Empty_##fmt##_Decoder : public OpalEmptyFramedAudioTranscoder \
{ \
  public: \
    Opal_Empty_##fmt##_Decoder() \
      : OpalEmptyFramedAudioTranscoder(fmt, OpalPCM16) \
    { } \
}; \

#define OPAL_DEFINE_EMPTY_TRANSCODER(fmt) \
OPAL_REGISTER_TRANSCODER(Opal_Empty_##fmt##_Encoder, OpalPCM16, fmt); \
OPAL_REGISTER_TRANSCODER(Opal_Empty_##fmt##_Decoder, fmt,       OpalPCM16); \

#endif // __OPAL_TRANSCODERS_H


// End of File ///////////////////////////////////////////////////////////////
