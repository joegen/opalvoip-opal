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
 * Revision 1.2012  2004/03/22 11:32:41  rjongbloed
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


class RTP_DataFrame;


///////////////////////////////////////////////////////////////////////////////

class OpalTranscoder;

/**This class embodies the description of an object used to convert media data
   from one format to another.

   An application may create a descendent off this class and override
   the Create() function to make the instance of a class implementing a
   transcoder.
 */
class OpalTranscoderRegistration : public PCaselessString
{
    PCLASSINFO(OpalTranscoderRegistration, PCaselessString);
  public:
  /**@name Construction */
  //@{
    /**Create a new transcoder registration.
     */
    OpalTranscoderRegistration(
      const char * inputFormat,  /// Input format name
      const char * outputFormat  /// Output format name
    );
  //@}

  /**@name Operations */
  //@{
    /**Create an instance of the transcoder implementation.
      */
    virtual OpalTranscoder * Create(
      void * parameters   /// Arbitrary parameters for the transcoder
    ) const = 0;

    /**Get the names of the input or output formats.
      */
    PString GetInputFormat() const;

    /**Get the names of the input or output formats.
      */
    PString GetOutputFormat() const;
  //@}

  protected:
    OpalTranscoderRegistration * link;

  friend class OpalTranscoder;
};


#define OPAL_REGISTER_TRANSCODER_FUNCTION(cls, src, dst, param) \
static class Transcoder_##cls##_Registration : public OpalTranscoderRegistration { \
  public: \
    Transcoder_##cls##_Registration() : OpalTranscoderRegistration(src, dst) { } \
    OpalTranscoder * Create(void * param) const; \
} Transcoder_##cls##_Registration_instance; \
static Transcoder_##cls##_Registration * static_Transcoder_##cls##_Registration = &Transcoder_##cls##_Registration_instance; \
OpalTranscoder * Transcoder_##cls##_Registration::Create(void * param) const 

#ifndef OPAL_NO_PARAM
#define OPAL_NO_PARAM
#endif

#define OPAL_REGISTER_TRANSCODER(cls, src, dst) \
  OPAL_REGISTER_TRANSCODER_FUNCTION(cls, src, dst, OPAL_NO_PARAM) \
  { return new cls(*this); }

#define OPAL_REGISTER_TRANSCODER_PARAM(cls, src, dst) \
  OPAL_REGISTER_TRANSCODER_FUNCTION(cls, src, dst, parameter) \
  { return new cls(*this, parameter); }



/**This class embodies the implementation of a specific transcoder instance
   used to convert data from one format to another.

   An application may create a descendent off this class and override
   functions as required for implementing a transcoder.
 */
class OpalTranscoder : public PObject
{
    PCLASSINFO(OpalTranscoder, PObject);
  public:
  /**@name Construction */
  //@{
    /** Create a new transcoder implementation.
      */
    OpalTranscoder(
      const OpalTranscoderRegistration & registration /// Registration fro transcoder
    );
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    /// Stream to output text representation
    ) const;
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
      BOOL input      /// Flag for input or output data size
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
      const RTP_DataFrame & input,  /// Input data
      RTP_DataFrameList & output    /// Output data
    );

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       Returns FALSE if the conversion fails.
      */
    virtual BOOL Convert(
      const RTP_DataFrame & input,  /// Input data
      RTP_DataFrame & output        /// Output data
    ) = 0;

    /**Create an instance of a media conversion function.
       Returns NULL if there is no registered media transcoder between the two
       named formats.
      */
    static OpalTranscoder * Create(
      const OpalMediaFormat & srcFormat,  /// Name of source format
      const OpalMediaFormat & dstFormat,  /// Name of destination format
      void * parameters = NULL            /// Arbitrary parameters for the transcoder
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
      unsigned sessionID,               /// Session ID for media formats
      const OpalMediaFormatList & srcFormats, /// Names of possible source formats
      const OpalMediaFormatList & dstFormats, /// Names of possible destination formats
      OpalMediaFormat & srcFormat,      /// Selected source format to be used
      OpalMediaFormat & dstFormat       /// Selected destination format to be used
    );

    /**Find media intermediate format for transcoders.
       This function attempts to find the intermediate media format that will
       allow two transcoders to be used to get data from the source format to
       the destination format.

       Returns FALSE if there is no registered media transcoder that can be used
       between the two named formats.
      */
    static BOOL FindIntermediateFormat(
      const OpalMediaFormat & srcFormat,    /// Selected destination format to be used
      const OpalMediaFormat & dstFormat,    /// Selected destination format to be used
      OpalMediaFormat & intermediateFormat  /// Intermediate format that can be used
    );

    /**Get a list of possible destination media formats for the destination.
      */
    static OpalMediaFormatList GetDestinationFormats(
      const OpalMediaFormat & srcFormat    /// Selected source format
    );

    /**Get a list of possible source media formats for the destination.
      */
    static OpalMediaFormatList GetSourceFormats(
      const OpalMediaFormat & dstFormat    /// Selected destination format
    );
  //@}

  /**@name Operations */
  //@{
    /**Get the transcoder registration that created this transcoder.
     */
    const OpalTranscoderRegistration & GetRegistration() const { return registration; }

    /**Get the names of the input or output formats.
      */
    const OpalMediaFormat & GetInputFormat() const { return inputMediaFormat; }

    /**Get the names of the input or output formats.
      */
    const OpalMediaFormat & GetOutputFormat() const { return outputMediaFormat; }

    /**Get maximum output size.
      */
    PINDEX GetMaxOutputSize() const { return maxOutputSize; }

    /**Set the maximum output size.
      */
    void SetMaxOutputSize(
      PINDEX size
    ) { maxOutputSize = size; }
  //@}

  protected:
    const OpalTranscoderRegistration & registration;
    OpalMediaFormat                    inputMediaFormat;
    OpalMediaFormat                    outputMediaFormat;
    PINDEX                             maxOutputSize;
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
      const OpalTranscoderRegistration & registration, /// Registration fro transcoder
      PINDEX inputBytesPerFrame,  /// Number of bytes in an input frame
      PINDEX outputBytesPerFrame  /// Number of bytes in an output frame
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
      BOOL input      /// Flag for input or output data size
    ) const;

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       Returns FALSE if the conversion fails.
      */
    virtual BOOL Convert(
      const RTP_DataFrame & input,  /// Input data
      RTP_DataFrame & output        /// Output data
    );

    /**Convert a frame of data from one format to another.
       This function implicitly knows the input and output frame sizes.
      */
    virtual BOOL ConvertFrame(
      const BYTE * input,   /// Input data
      BYTE * output         /// Output data
    ) = 0;
  //@}

  protected:
    PINDEX     inputBytesPerFrame;
    PINDEX     outputBytesPerFrame;
    PBYTEArray partialFrame;
    PINDEX     partialBytes;
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
      const OpalTranscoderRegistration & registration, /// Registration fro transcoder
      unsigned inputBits,           /// Bits per sample in input data
      unsigned outputBits,          /// Bits per sample in output data
      PINDEX   optimalSamples       /// Optimal number of samples for read
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
      BOOL input      /// Flag for input or output data size
    ) const;

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       Returns FALSE if the conversion fails.
      */
    virtual BOOL Convert(
      const RTP_DataFrame & input,  /// Input data
      RTP_DataFrame & output        /// Output data
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
    Opal_Linear16Mono_PCM(
      const OpalTranscoderRegistration & registration /// Registration for transcoder
    );
    virtual int ConvertOne(int sample) const;
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_Linear16Mono : public OpalStreamedTranscoder {
  public:
    Opal_PCM_Linear16Mono(
      const OpalTranscoderRegistration & registration /// Registration for transcoder
    );
    virtual int ConvertOne(int sample) const;
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_OPAL_L16_MONO() \
  OPAL_REGISTER_TRANSCODER(Opal_Linear16Mono_PCM, OPAL_OPAL_L16_MONO, OPAL_PCM16); \
  OPAL_REGISTER_TRANSCODER(Opal_PCM_Linear16Mono, OPAL_PCM16, OPAL_OPAL_L16_MONO)



#endif // __OPAL_TRANSCODERS_H


// End of File ///////////////////////////////////////////////////////////////
