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
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
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

#ifndef __CODEC_H261CODEC_H
#define __CODEC_H261CODEC_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/transcoders.h>
#include <h323/h323caps.h>


class P64Decoder;
class P64Encoder;


///////////////////////////////////////////////////////////////////////////////

class Opal_H261_YUV411P : public OpalVideoTranscoder {
  public:
    Opal_H261_YUV411P(
      const OpalTranscoderRegistration & registration /// Registration fro transcoder
    );
    ~Opal_H261_YUV411P();
    virtual BOOL Convert(const RTP_DataFrame & src, RTP_DataFrame & dst);
  protected:
    P64Decoder * videoDecoder;
    BYTE * rvts;
    int ndblk, nblk;
    int now;    
    BOOL packetReceived;
};


///////////////////////////////////////////////////////////////////////////////

class Opal_YUV411P_H261 : public OpalVideoTranscoder {
  public:
    Opal_YUV411P_H261(
      const OpalTranscoderRegistration & registration /// Registration fro transcoder
    );
    ~Opal_YUV411P_H261();
    virtual BOOL Convert(const RTP_DataFrame & src, RTP_DataFrame & dst);
  protected:
    P64Encoder * videoEncoder;
};


///////////////////////////////////////////////////////////////////////////////

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
  //@}

  
  /**@name Overrides from class PObject */
  //@{
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

  //@}

  protected:
    unsigned qcifMPI;                   // 1..4 units 1/29.97 Hz
    unsigned cifMPI;                    // 1..4 units 1/29.97 Hz
    BOOL     temporalSpatialTradeOffCapability;
    unsigned maxBitRate;                // units of 100 bit/s
    BOOL     stillImageTransmission;    // Annex D of H.261
};


#endif // __CODEC_H261CODEC_H


/////////////////////////////////////////////////////////////////////////////

