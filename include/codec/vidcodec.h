/*
 * vidcodec.h
 *
 * Uncompressed video handler
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * Contributor(s): 
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_CODEC_VIDCODEC_H
#define OPAL_CODEC_VIDCODEC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/transcoders.h>

#if OPAL_VIDEO

#if OPAL_H323
#include <h323/h323caps.h>
#endif

#include <codec/opalplugin.h>


#define OPAL_RGB24   "RGB24"
#define OPAL_RGB32   "RGB32"
#define OPAL_YUV420P "YUV420P"

extern const OpalVideoFormat & GetOpalRGB24();
extern const OpalVideoFormat & GetOpalRGB32();
extern const OpalVideoFormat & GetOpalYUV420P();

#define OpalRGB24   GetOpalRGB24()
#define OpalRGB32   GetOpalRGB32()
#define OpalYUV420P GetOpalYUV420P()


///////////////////////////////////////////////////////////////////////////////

/**This class defines a transcoder implementation class that will
   encode/decode video.

   An application may create a descendent off this class and override
   functions as required for descibing a specific transcoder.
 */
class OpalVideoTranscoder : public OpalTranscoder
{
    PCLASSINFO(OpalVideoTranscoder, OpalTranscoder);
  public:
    typedef PluginCodec_Video_FrameHeader FrameHeader;

  /**@name Construction */
  //@{
    /** Create a new video transcoder implementation.
      */
    OpalVideoTranscoder(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );
  //@}

  /**@name Operations */
  //@{
    /**Update the input and output media formats. This can be used to adjust
       the parameters of a codec at run time. Note you cannot change the basic
       media format, eg change GSM0610 to G.711, only options for that
       format, eg 6k3 mode to 5k3 mode in G.723.1. If the formats are
       different then a OpalMediaFormat::Merge() is performed.

       If a format is empty (invalid) it is ignored and does not update the
       internal variable. In this way only the input or output side can be
       updated.

       The default behaviour updates the inputMediaFormat and outputMediaFormat
       member variables.
      */
    virtual bool UpdateMediaFormats(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );

    /**Get the optimal size for data frames to be converted.
       This function returns the size of frames that will be most efficient
       in conversion. A RTP_DataFrame will attempt to provide or use data in
       multiples of this size. Note that it may not do so, so the transcoder
       must be able to handle any sized packets.
      */
    virtual PINDEX GetOptimalDataFrameSize(
      PBoolean input      ///<  Flag for input or output data size
    ) const;

    /**Execute the command specified to the transcoder. The commands are
       highly context sensitive, for example OpalVideoUpdatePicture would only
       apply to a video transcoder.

       The default behaviour checks for a OpalVideoUpdatePicture and sets the
       updatePicture member variable if that is the command.

       @returns true if command is handled.
      */
    virtual PBoolean ExecuteCommand(
      const OpalMediaCommand & command    ///<  Command to execute.
    );

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       This is a dummy function as nearly all video conversion will does not
       have a one to one input to output frames ratio, so the ConvertFrames()
       function is used instead.

       Returns false if the conversion fails.
      */
    virtual PBoolean Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    );

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics) const;
#endif

    virtual bool HasErrorConcealment() const  { return m_errorConcealment; }
    bool WasLastFrameIFrame() const { return lastFrameWasIFrame; }
    void ForceIFrame()              { forceIFrame = true; }
  //@}

  protected:
    bool   m_errorConcealment;
    PINDEX inDataSize;
    PINDEX outDataSize;
    bool   forceIFrame;
    bool   lastFrameWasIFrame;

    DWORD m_totalFrames;
    DWORD m_keyFrames;
};


///////////////////////////////////////////////////////////////////////////////

OPAL_DEFINE_MEDIA_COMMAND(OpalVideoFreezePicture, "Freeze Picture");

/**This indicates that a force video picture update is required, that
   is an Intra or Key frame MUST be sent. This may be called when the
   system logic requires a scene chage, e.g. when recording a video an
   Intra frame is required before it can start, the sender must send one.
  */
class OpalVideoUpdatePicture : public OpalMediaCommand
{
  PCLASSINFO(OpalVideoUpdatePicture, OpalMediaCommand);
  public:
    virtual PString GetName() const;
};

/**This indicates that the remote has had a picture loss and that a
   video picture update is desired, that is an Intra or Key frame should be
   sent, however, the sender can opt due to say bandwidth constraints not to
   do so and suffer with the corrupted video.
  */
class OpalVideoPictureLoss : public OpalVideoUpdatePicture
{
  PCLASSINFO(OpalVideoPictureLoss, OpalVideoUpdatePicture);
  public:
    OpalVideoPictureLoss(unsigned sequenceNumber = 0, unsigned timestamp = 0);

    virtual PString GetName() const;

    unsigned GetSequenceNumber() const { return m_sequenceNumber; }
    unsigned GetTimestamp() const { return m_timestamp; }

  protected:
    unsigned m_sequenceNumber;
    unsigned m_timestamp;
};


/**This is an indication to the video encoder to change the way it is to
   encode the video stream.

   The trade off integer is a value that trade spatial quality (blockiness)
   for temparaly quality (frame rate). A value of zero is maxium spatial
   quality and a value fo 31 is maximum frame rate.
  */
class OpalTemporalSpatialTradeOff : public OpalMediaCommand
{
  PCLASSINFO(OpalTemporalSpatialTradeOff, OpalMediaCommand);
  public:
    OpalTemporalSpatialTradeOff(int tradeoff) : m_tradeOff(tradeoff) { }

    virtual PString GetName() const;

    int GetTradeOff() const { return m_tradeOff; }

  protected:
    int m_tradeOff;
};


#endif // OPAL_VIDEO

#endif // OPAL_CODEC_VIDCODEC_H

/////////////////////////////////////////////////////////////////////////////
