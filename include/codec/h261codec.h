/*
 * h261codec.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2001 Equivalence Pty. Ltd.
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
 * Contributor(s): Michele Piccini (michele@piccini.com).
 *                 Derek Smithies (derek@indranet.co.nz)
 *
 * $Log: h261codec.h,v $
 * Revision 1.2013  2004/03/11 06:54:25  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.11  2004/02/19 10:46:43  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.10  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.9  2003/01/07 04:39:52  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.8  2002/11/10 23:21:26  robertj
 * Fixed capability class usage in static variable macros.
 *
 * Revision 2.7  2002/11/10 11:33:16  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.6  2002/09/16 02:52:33  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.5  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
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
 * Revision 2.1  2001/08/01 05:03:09  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 * Allowed codecs to be used without H.,323 being linked by using the
 *   new NO_H323 define.
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.35  2004/01/02 00:31:42  dereksmithies
 * Fix test on presence/absence of video.
 *
 * Revision 1.34  2003/12/14 10:42:29  rjongbloed
 * Changes for compilability without video support.
 *
 * Revision 1.33  2003/10/02 23:37:56  dereksmithies
 * Add fix for fast update problem in h261 codec. Thanks to Martin André for doing the initial legwork.
 *
 * Revision 1.32  2003/04/03 23:54:11  robertj
 * Added fast update to H.261 codec, thanks Gustavo García Bernardo
 *
 * Revision 1.31  2002/12/16 09:11:15  robertj
 * Added new video bit rate control, thanks Walter H. Whitlock
 *
 * Revision 1.30  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.29  2002/09/03 06:19:36  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 *
 * Revision 1.28  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.27  2002/01/09 06:06:37  robertj
 * Fixed setting of RTP timestamp values on video transmission.
 *
 * Revision 1.26  2002/01/09 00:21:36  robertj
 * Changes to support outgoing H.245 RequstModeChange.
 *
 * Revision 1.25  2001/12/04 04:26:03  robertj
 * Added code to allow change of video quality in H.261, thanks Damian Sandras
 *
 * Revision 1.24  2001/10/23 02:18:06  dereks
 * Initial release of CU30 video codec.
 *
 * Revision 1.23  2001/09/25 03:14:47  dereks
 * Add constant bitrate control for the h261 video codec.
 * Thanks Tiziano Morganti for the code to set bit rate. Good work!
 *
 * Revision 1.22  2001/05/25 01:10:26  dereks
 * Remove unnecessary packet receive variable.
 * Alter the position of the check for change in frame size.
 *
 * Revision 1.21  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.20  2001/01/25 07:27:14  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.19  2000/12/19 22:33:44  dereks
 * Adjust so that the video channel is used for reading/writing raw video
 * data, which better modularizes the video codec.
 *
 * Revision 1.18  2000/08/21 04:45:06  dereks
 * Fix dangling pointer that caused segfaults for windows&unix users.
 * Improved the test image which is used when video grabber won't open.
 * Added code to handle setting of video Tx Quality.
 * Added code to set the number of background blocks sent with every frame.
 *
 * Revision 1.17  2000/07/04 13:00:36  craigs
 * Fixed problem with selecting large and small video sizes
 *
 * Revision 1.16  2000/05/18 11:53:33  robertj
 * Changes to support doc++ documentation generation.
 *
 * Revision 1.15  2000/05/10 04:05:26  robertj
 * Changed capabilities so has a function to get name of codec, instead of relying on PrintOn.
 *
 * Revision 1.14  2000/05/02 04:32:24  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.13  2000/03/30 23:10:50  robertj
 * Fixed error in comments regarding GetFramerate() function.
 *
 * Revision 1.12  2000/03/21 03:06:47  robertj
 * Changes to make RTP TX of exact numbers of frames in some codecs.
 *
 * Revision 1.11  2000/02/04 05:00:08  craigs
 * Changes for video transmission
 *
 * Revision 1.10  2000/01/13 04:03:45  robertj
 * Added video transmission
 *
 * Revision 1.9  1999/12/23 23:02:34  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.8  1999/11/29 08:59:56  craigs
 * Added new stuff for new video codec interface
 *
 * Revision 1.7  1999/11/01 00:52:00  robertj
 * Fixed various problems in video, especially ability to pass error return value.
 *
 * Revision 1.6  1999/10/08 09:59:02  robertj
 * Rewrite of capability for sending multiple audio frames
 *
 * Revision 1.5  1999/10/08 04:58:37  robertj
 * Added capability for sending multiple audio frames in single RTP packet
 *
 * Revision 1.4  1999/09/21 14:03:41  robertj
 * Changed RTP data frame parameter in Write() to be const.
 *
 * Revision 1.3  1999/09/21 08:12:50  craigs
 * Added support for video codecs and H261
 *
 * Revision 1.2  1999/09/18 13:27:24  craigs
 * Added ability disable jitter buffer for codecs
 * Added ability to access entire RTP frame from codec Write
 *
 * Revision 1.1  1999/09/08 04:05:48  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 */

#ifndef __OPAL_H261CODEC_H
#define __OPAL_H261CODEC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#ifndef NO_OPAL_VIDEO

#include <codec/vidcodec.h>

#ifndef NO_H323
#include <h323/h323caps.h>
#endif

class P64Decoder;
class P64Encoder;

#define OPAL_H261      "H.261"
#define OPAL_H261_CIF  "H.261-CIF"
#define OPAL_H261_QCIF "H.261-QCIF"

extern OpalMediaFormat const OpalH261;
extern OpalMediaFormat const OpalH261_CIF;
extern OpalMediaFormat const OpalH261_QCIF;


///////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

/**This class is a H.261 video capability.
 */
class H323_H261Capability : public H323VideoCapability
{
  PCLASSINFO(H323_H261Capability, H323VideoCapability)

  public:
  /**@name Construction */
  //@{
    /**Create a new H261 Capability
     */
    H323_H261Capability(
      unsigned qcifMPI,
      unsigned cifMPI,
      BOOL temporalSpatialTradeOffCapability = TRUE,
      BOOL stillImageTransmission = FALSE,
      unsigned maxBitRate = 850
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;

    /**Compare object
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
      H245_VideoCapability & pdu  /// PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the resolution and bit rate.
     */
    virtual BOOL OnSendingPDU(
      H245_VideoMode & pdu  /// PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour gets the data rate field from the PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu  /// PDU to set information on
    );

    /** Get temporal/spatial tradeoff capabilty
     */
    BOOL GetTemporalSpatialTradeOffCapability() const
      { return temporalSpatialTradeOffCapability; }

    /** Get still image transmission flag
     */
    BOOL GetStillImageTransmission() const
      { return stillImageTransmission; }

    /** Get maximum bit rate
     */
    unsigned GetMaxBitRate() const
      { return maxBitRate; }

    /** Get qcifMPI
     */
    unsigned GetQCIFMPI() const
      { return qcifMPI; }

    /** Get cifMPI
     */
    unsigned GetCIFMPI() const
      { return cifMPI; }

    /**Set the quality level of transmitted video data. 
       Is irrelevant when this codec is used to receive video data.
       Has a value of 1 (good quality) to 31 (poor quality).
       Quality is improved at the expense of bit rate.
    */
    void SetTxQualityLevel(int qLevel);
  //@}

  protected:
    unsigned qcifMPI;                   // 1..4 units 1/29.97 Hz
    unsigned cifMPI;                    // 1..4 units 1/29.97 Hz
    BOOL     temporalSpatialTradeOffCapability;

    unsigned maxBitRate;                // units of 100 bit/s
    BOOL     stillImageTransmission;    // Annex D of H.261
};

#define OPAL_REGISTER_H261_H323 \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_H261_QCIF_CIF, OPAL_H261, H323_NO_EP_VAR) \
    { return new H323_H261Capability(2, 4, FALSE, FALSE, 6217); } \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_H261_CIF, OPAL_H261_CIF, H323_NO_EP_VAR) \
    { return new H323_H261Capability(0, 4, FALSE, FALSE, 6217); } \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_H261_QCIF, OPAL_H261_QCIF, H323_NO_EP_VAR) \
    { return new H323_H261Capability(2, 0, FALSE, FALSE, 6217); }

#else // ifndef NO_H323

#define OPAL_REGISTER_H261_H323

#endif // ifndef NO_H323


///////////////////////////////////////////////////////////////////////////////

class Opal_H261_YUV420P : public OpalVideoTranscoder {
  public:
    Opal_H261_YUV420P(
      const OpalTranscoderRegistration & registration /// Registration fro transcoder
    );
    ~Opal_H261_YUV420P();
    virtual PINDEX GetOptimalDataFrameSize(BOOL input) const;
    virtual BOOL ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dst);
  protected:
    P64Decoder * videoDecoder;
    BYTE * rvts;
    int ndblk, nblk;
    int now;    
    BOOL packetReceived;
};


class Opal_YUV420P_H261 : public OpalVideoTranscoder {
  public:
    Opal_YUV420P_H261(
      const OpalTranscoderRegistration & registration /// Registration fro transcoder
    );
    ~Opal_YUV420P_H261();
    virtual PINDEX GetOptimalDataFrameSize(BOOL input) const;
    virtual BOOL ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dst);
  protected:
    P64Encoder * videoEncoder;
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_H261() \
          OPAL_REGISTER_H261_H323 \
          OPAL_REGISTER_TRANSCODER(Opal_H261_YUV420P, OPAL_H261, OPAL_YUV420P); \
          OPAL_REGISTER_TRANSCODER(Opal_YUV420P_H261, OPAL_YUV420P, OPAL_H261)


#endif // __OPAL_H261CODEC_H


#endif // NO_OPAL_VIDEO


/////////////////////////////////////////////////////////////////////////////
