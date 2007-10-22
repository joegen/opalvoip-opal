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
 * $Log: vidcodec.h,v $
 * Revision 1.2017  2007/09/25 09:49:54  rjongbloed
 * Fixed videoFastUpdate, is not a count but a simple boolean.
 *
 * Revision 2.15  2007/08/17 11:14:17  csoutheren
 * Add OnLostPicture and OnLostPartialPicture commands
 *
 * Revision 2.14  2007/02/16 07:56:02  dereksmithies
 * Change flag used so H.323 capabiliites are included  if H.323 was enabled
 * when the library was configured.
 *
 * Revision 2.13  2006/07/24 14:03:38  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.12.4.2  2006/04/19 07:52:30  csoutheren
 * Add ability to have SIP-only and H.323-only codecs, and implement for H.261
 *
 * Revision 2.12.4.1  2006/04/06 01:21:16  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 2.12  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.11  2005/10/21 17:58:31  dsandras
 * Applied patch from Hannes Friederich <hannesf AATT ee.ethz.ch> to fix OpalVideoUpdatePicture - PIsDescendant problems. Thanks!
 *
 * Revision 2.10  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.9  2005/09/04 06:23:38  rjongbloed
 * Added OpalMediaCommand mechanism (via PNotifier) for media streams
 *   and media transcoders to send commands back to remote.
 *
 * Revision 2.8  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.7  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.6  2005/07/24 07:33:07  rjongbloed
 * Simplified "uncompressed" transcoder sp can test video media streams.
 *
 * Revision 2.5  2005/02/21 12:19:45  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.4  2004/09/01 12:21:27  rjongbloed
 * Added initialisation of H323EndPoints capability table to be all codecs so can
 *   correctly build remote caps from fqast connect params. This had knock on effect
 *   with const keywords added in numerous places.
 *
 * Revision 2.3  2004/03/11 06:54:26  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.2  2004/01/18 15:35:20  rjongbloed
 * More work on video support
 *
 * Revision 2.1  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 */

#ifndef __OPAL_VIDCODEC_H
#define __OPAL_VIDCODEC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/transcoders.h>

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

       The default behaviour checks for a OpalVideoUpdatePicture and sets the
       updatePicture member variable if that is the command.
      */
    virtual BOOL ExecuteCommand(
      const OpalMediaCommand & command    ///<  Command to execute.
    );

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       This is a dummy function as nearly all video conversion will does not
       have a one to one input to output frames ratio, so the ConvertFrames()
       function is used instead.

       Returns FALSE if the conversion fails.
      */
    virtual BOOL Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    );
  //@}

  protected:
    unsigned frameWidth;
    unsigned frameHeight;
    unsigned videoQuality;
    unsigned targetBitRate;
    bool     dynamicVideoQuality;
    bool     adaptivePacketDelay;
    unsigned fillLevel;

    bool     forceIFrame;
};


///////////////////////////////////////////////////////////////////////////////

OPAL_DEFINE_MEDIA_COMMAND(OpalVideoFreezePicture, "Freeze Picture");

class OpalVideoUpdatePicture : public OpalMediaCommand
{
  PCLASSINFO(OpalVideoUpdatePicture, OpalMediaCommand);
  public:
    OpalVideoUpdatePicture(int firstGOB = -1, int firstMB = -1, int numBlocks = 0)
      : m_firstGOB(firstGOB), m_firstMB(firstMB), m_numBlocks(numBlocks) { }

    virtual PString GetName() const;

    int GetFirstGOB() const { return m_firstGOB; }
    int GetFirstMB() const { return m_firstMB; }
    int GetNumBlocks() const { return m_numBlocks; }

  protected:
    int m_firstGOB;
    int m_firstMB;
    int m_numBlocks;
};


class OpalTemporalSpatialTradeOff : public OpalMediaCommand
{
  PCLASSINFO(OpalTemporalSpatialTradeOff, OpalMediaCommand);
  public:
    OpalTemporalSpatialTradeOff(int quality) : m_quality(quality) { }

    virtual PString GetName() const;

    int GetQuality() const { return m_quality; }

  protected:
    int m_quality;
};


class OpalLostPartialPicture : public OpalMediaCommand
{
  PCLASSINFO(OpalLostPartialPicture, OpalMediaCommand);
  public:
    OpalLostPartialPicture() { }
    virtual PString GetName() const;
};


class OpalLostPicture : public OpalMediaCommand
{
  PCLASSINFO(OpalLostPicture, OpalMediaCommand);
  public:
    OpalLostPicture() { }
    virtual PString GetName() const;
};


///////////////////////////////////////////////////////////////////////////////

#if OPAL_H323
/* This code is only built if the user has enabled the H.323 voip
   protocol in the configure step. The default configuration enables
   H.323 */

/**This class is a uncompressed video capability.
 */
class H323_UncompVideoCapability : public H323NonStandardVideoCapability
{
  PCLASSINFO(H323_UncompVideoCapability, H323NonStandardVideoCapability)

  public:
  /**@name Construction */
  //@{
    /**Create a new uncompressed video Capability
     */
    H323_UncompVideoCapability(
      const H323EndPoint & endpoint, ///<  Endpoint to get t35 information
      const PString & colourFormat   ///<  Video colour format name
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
    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

  protected:
    PString colourFormat;
};

#define OPAL_REGISTER_UNCOMPRESSED_VIDEO_H323 \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_RGB24, OPAL_RGB24, ep) \
    { return new H323_UncompVideoCapability(ep, OpalRGB24); } \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_RGB32, OPAL_RGB32, ep) \
    { return new H323_UncompVideoCapability(ep, OpalRGB32); }

#else // ifndef NO_H323

#define OPAL_REGISTER_UNCOMPRESSED_VIDEO_H323

#endif // ifndef NO_H323


///////////////////////////////////////////////////////////////////////////////

/**This class defines a transcoder implementation class that will
   encode/decode uncompressed video.
 */
class OpalUncompVideoTranscoder : public OpalVideoTranscoder
{
  PCLASSINFO(OpalUncompVideoTranscoder, OpalVideoTranscoder);
  public:
  /**@name Construction */
  //@{
    /** Create a new video transcoder implementation.
      */
    OpalUncompVideoTranscoder(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );

    /**Destroy the video transcoder cleaning up the colour converter.
      */
    ~OpalUncompVideoTranscoder();
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
  //@}
};


class Opal_RGB24_RGB24 : public OpalUncompVideoTranscoder {
  PCLASSINFO(Opal_RGB24_RGB24, OpalUncompVideoTranscoder);
  public:
    Opal_RGB24_RGB24() : OpalUncompVideoTranscoder(OpalRGB24, OpalRGB24) { }
};


class Opal_RGB32_RGB32 : public OpalUncompVideoTranscoder {
  PCLASSINFO(Opal_RGB32_RGB32, OpalUncompVideoTranscoder);
  public:
    Opal_RGB32_RGB32() : OpalUncompVideoTranscoder(OpalRGB32, OpalRGB32) { }
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_UNCOMPRESSED_VIDEO() \
          OPAL_REGISTER_UNCOMPRESSED_VIDEO_H323 \
          OPAL_REGISTER_TRANSCODER(Opal_RGB32_RGB32, OpalRGB32, OpalRGB32); \
          OPAL_REGISTER_TRANSCODER(Opal_RGB24_RGB24, OpalRGB24, OpalRGB24)


#endif // __OPAL_VIDCODEC_H


/////////////////////////////////////////////////////////////////////////////
