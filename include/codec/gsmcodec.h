/*
 * gsmcodec.h
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
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: gsmcodec.h,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.10  2001/02/11 22:48:30  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.9  2001/01/25 07:27:14  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.8  2000/10/13 03:43:14  robertj
 * Added clamping to avoid ever setting incorrect tx frame count.
 *
 * Revision 1.7  2000/05/10 04:05:26  robertj
 * Changed capabilities so has a function to get name of codec, instead of relying on PrintOn.
 *
 * Revision 1.6  2000/05/02 04:32:24  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.5  1999/12/31 00:05:36  robertj
 * Added Microsoft ACM G.723.1 codec capability.
 *
 * Revision 1.4  1999/12/23 23:02:34  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.3  1999/10/08 09:59:01  robertj
 * Rewrite of capability for sending multiple audio frames
 *
 * Revision 1.2  1999/10/08 04:58:37  robertj
 * Added capability for sending multiple audio frames in single RTP packet
 *
 * Revision 1.1  1999/09/08 04:05:48  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 */

#ifndef __CODEC_GSMCODEC_H
#define __CODEC_GSMCODEC_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/transcoders.h>
#include <h323/h323caps.h>


///////////////////////////////////////////////////////////////////////////////

struct gsm_state;

class Opal_GSM : public OpalFramedTranscoder {
  public:
    Opal_GSM(
      const OpalTranscoderRegistration & registration, /// Registration fro transcoder
      unsigned inputBytesPerFrame,  /// Number of bytes in an input frame
      unsigned outputBytesPerFrame  /// Number of bytes in an output frame
    );
    ~Opal_GSM();
  protected:
    gsm_state * gsm;
};


///////////////////////////////////////////////////////////////////////////////

class Opal_GSM_PCM : public Opal_GSM {
  public:
    Opal_GSM_PCM(
      const OpalTranscoderRegistration & registration /// Registration fro transcoder
    );
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_GSM : public Opal_GSM {
  public:
    Opal_PCM_GSM(
      const OpalTranscoderRegistration & registration /// Registration fro transcoder
    );
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

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
      unsigned frames   /// Number of frames per packet
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
      H245_AudioCapability & pdu,  /// PDU to set information on
      unsigned packetSize          /// Packet size to use in capability
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour gets the data rate field from the PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_AudioCapability & pdu,  /// PDU to get information from
      unsigned & packetSize              /// Packet size to use in capability
    );
  //@}
};


/////////////////////////////////////////////////////////////////////////////

extern OpalMediaFormat const OpalMediaFormat_GSM;


#endif // __CODEC_GSMCODEC_H


/////////////////////////////////////////////////////////////////////////////
