/*
 * gsmcodec.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * $Log: gsmcodec.cxx,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.19  2001/05/14 05:56:28  robertj
 * Added H323 capability registration system so can add capabilities by
 *   string name instead of having to instantiate explicit classes.
 *
 * Revision 1.18  2001/02/09 05:13:55  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.17  2001/01/25 07:27:16  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.16  2000/10/13 03:43:29  robertj
 * Added clamping to avoid ever setting incorrect tx frame count.
 *
 * Revision 1.15  2000/08/25 03:18:40  craigs
 * More work on support for MS-GSM format
 *
 * Revision 1.14  2000/07/13 17:24:33  robertj
 * Fixed format name to be consistent will all others.
 *
 * Revision 1.13  2000/07/12 10:25:37  robertj
 * Renamed all codecs so obvious whether software or hardware.
 *
 * Revision 1.12  2000/07/09 14:55:15  robertj
 * Bullet proofed packet count so incorrect capabilities does not crash us.
 *
 * Revision 1.11  2000/05/10 04:05:33  robertj
 * Changed capabilities so has a function to get name of codec, instead of relying on PrintOn.
 *
 * Revision 1.10  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.9  2000/03/21 03:06:49  robertj
 * Changes to make RTP TX of exact numbers of frames in some codecs.
 *
 * Revision 1.8  1999/12/31 00:05:36  robertj
 * Added Microsoft ACM G.723.1 codec capability.
 *
 * Revision 1.7  1999/11/20 00:53:47  robertj
 * Fixed ability to have variable sized frames in single RTP packet under G.723.1
 *
 * Revision 1.6  1999/10/08 09:59:03  robertj
 * Rewrite of capability for sending multiple audio frames
 *
 * Revision 1.5  1999/10/08 08:30:45  robertj
 * Fixed maximum packet size, must be less than 256
 *
 * Revision 1.4  1999/10/08 04:58:38  robertj
 * Added capability for sending multiple audio frames in single RTP packet
 *
 * Revision 1.3  1999/09/27 01:13:09  robertj
 * Fixed old GNU compiler support
 *
 * Revision 1.2  1999/09/23 07:25:12  robertj
 * Added open audio and video function to connection and started multi-frame codec send functionality.
 *
 * Revision 1.1  1999/09/08 04:05:49  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "gsmcodec.h"
#endif

#include <codec/gsmcodec.h>
#include <asn/h245.h>

extern "C" {
#include "gsm/inc/gsm.h"
};


#define new PNEW


#define GSM_BYTES_PER_FRAME   33
#define GSM_SAMPLES_PER_FRAME 160

#define GSM_BASE_NAME  "GSM-06.10"
#define GSM_H323_NAME  GSM_BASE_NAME "{sw}"


OpalMediaFormat const OpalMediaFormat_GSM(GSM_BASE_NAME,
                                          RTP_Session::DefaultAudioSessionID,
                                          RTP_DataFrame::GSM,
                                          TRUE,  // Needs jitter
                                          13200, // bits/sec
                                          GSM_BYTES_PER_FRAME,
                                          GSM_SAMPLES_PER_FRAME, // 20 milliseconds
                                          OpalMediaFormat::AudioTimeUnits);

OPAL_REGISTER_TRANSCODER(Opal_GSM_PCM, GSM_BASE_NAME, OPAL_PCM16);
OPAL_REGISTER_TRANSCODER(Opal_PCM_GSM, OPAL_PCM16, GSM_BASE_NAME);
H323_REGISTER_CAPABILITY(H323_GSM0610Capability, GSM_H323_NAME);


///////////////////////////////////////////////////////////////////////////////

Opal_GSM::Opal_GSM(const OpalTranscoderRegistration & registration,
                   unsigned inputBytesPerFrame,
                   unsigned outputBytesPerFrame)
  : OpalFramedTranscoder(registration, inputBytesPerFrame, outputBytesPerFrame)
{
  gsm = gsm_create();
}


Opal_GSM::~Opal_GSM()
{
  gsm_destroy(gsm);
}


///////////////////////////////////////////////////////////////////////////////

Opal_GSM_PCM::Opal_GSM_PCM(const OpalTranscoderRegistration & registration)
  : Opal_GSM(registration, GSM_BYTES_PER_FRAME, GSM_SAMPLES_PER_FRAME*2)
{
  PTRACE(3, "Codec\tGSM decoder created");
}


BOOL Opal_GSM_PCM::ConvertFrame(const BYTE * src, BYTE * dst)
{
  gsm_decode(gsm, (gsm_byte *)src, (gsm_signal *)dst);
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

Opal_PCM_GSM::Opal_PCM_GSM(const OpalTranscoderRegistration & registration)
  : Opal_GSM(registration, GSM_SAMPLES_PER_FRAME*2, GSM_BYTES_PER_FRAME)
{
  PTRACE(3, "Codec\tGSM encoder created");
}


BOOL Opal_PCM_GSM::ConvertFrame(const BYTE * src, BYTE * dst)
{
  gsm_encode(gsm, (gsm_signal *)src, (gsm_byte *)dst);
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

H323_GSM0610Capability::H323_GSM0610Capability()
  : H323AudioCapability(7, 4)
{
}


PObject * H323_GSM0610Capability::Clone() const
{
  return new H323_GSM0610Capability(*this);
}


PString H323_GSM0610Capability::GetFormatName() const
{
  return GSM_H323_NAME;
}


unsigned H323_GSM0610Capability::GetSubType() const
{
  return H245_AudioCapability::e_gsmFullRate;
}


void H323_GSM0610Capability::SetTxFramesInPacket(unsigned frames)
{
  if (frames > 7)
    txFramesInPacket = 7;
  else
    H323AudioCapability::SetTxFramesInPacket(frames);
}


BOOL H323_GSM0610Capability::OnSendingPDU(H245_AudioCapability & cap,
                                          unsigned packetSize) const
{
  cap.SetTag(H245_AudioCapability::e_gsmFullRate);

  H245_GSMAudioCapability & gsm = cap;
  gsm.m_audioUnitSize = packetSize*GSM_BYTES_PER_FRAME;
  return TRUE;
}


BOOL H323_GSM0610Capability::OnReceivedPDU(const H245_AudioCapability & cap,
                                           unsigned & packetSize)
{
  if (cap.GetTag() != H245_AudioCapability::e_gsmFullRate)
    return FALSE;

  const H245_GSMAudioCapability & gsm = cap;
  packetSize = gsm.m_audioUnitSize / GSM_BYTES_PER_FRAME;
  if (packetSize == 0)
    packetSize = 1;
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
