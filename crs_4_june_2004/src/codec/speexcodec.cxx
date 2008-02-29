/*
 * speexcodec.cxx
 *
 * Speex codec handler
 *
 * Open H323 Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * $Log: speexcodec.cxx,v $
 * Revision 1.2004  2004/03/11 06:54:28  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.2  2004/02/19 10:47:02  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.1  2002/11/10 11:33:18  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 1.23  2004/01/30 00:55:41  csoutheren
 * Removed the Xiph capability variants per Roger Hardiman as these
 * were development-only code that should not have made it into a release
 *
 * Revision 1.22  2004/01/02 01:23:12  csoutheren
 * More changes to allow correct autodetection of local Speex libraries
 *
 * Revision 1.21  2003/12/29 12:13:26  csoutheren
 * configure now checks for libspeex in system libraries and compares
 * version against version in local sources. Also use --enable-localspeex to
 * force use of local Speex or system Speex
 *
 * Revision 1.20  2002/12/08 22:59:41  rogerh
 * Add XiphSpeex codec. Not yet finished.
 *
 * Revision 1.19  2002/12/06 10:11:54  rogerh
 * Back out the Xiph Speex changes on a tempoary basis while the Speex
 * spec is being redrafted.
 *
 * Revision 1.18  2002/12/06 03:27:47  robertj
 * Fixed MSVC warnings
 *
 * Revision 1.17  2002/12/05 12:57:17  rogerh
 * Speex now uses the manufacturer ID assigned to Xiph.Org.
 * To support existing applications using Speex, applications can use the
 * EquivalenceSpeex capabilities.
 *
 * Revision 1.16  2002/11/25 10:24:50  craigs
 * Fixed problem with Speex codec names causing mismatched capabilities
 * Reported by Ben Lear
 *
 * Revision 1.15  2002/11/09 07:08:20  robertj
 * Hide speex library from OPenH323 library users.
 * Made public the media format names.
 * Other cosmetic changes.
 *
 * Revision 1.14  2002/10/24 05:33:19  robertj
 * MSVC compatibility
 *
 * Revision 1.13  2002/10/22 11:54:32  rogerh
 * Fix including of speex.h
 *
 * Revision 1.12  2002/10/22 11:33:04  rogerh
 * Use the local speex.h header file
 *
 * Revision 1.11  2002/10/09 10:55:21  rogerh
 * Update the bit rates to match what the codec now does
 *
 * Revision 1.10  2002/09/02 21:58:40  rogerh
 * Update for Speex 0.8.0
 *
 * Revision 1.9  2002/08/21 06:49:13  rogerh
 * Fix the RTP Payload size too small problem with Speex 0.7.0.
 *
 * Revision 1.8  2002/08/15 18:34:51  rogerh
 * Fix some more bugs
 *
 * Revision 1.7  2002/08/14 19:06:53  rogerh
 * Fix some bugs when using the speex library
 *
 * Revision 1.6  2002/08/14 04:35:33  craigs
 * CHanged Speex names to remove spaces
 *
 * Revision 1.5  2002/08/14 04:30:14  craigs
 * Added bit rates to Speex codecs
 *
 * Revision 1.4  2002/08/14 04:27:26  craigs
 * Fixed name of Speex codecs
 *
 * Revision 1.3  2002/08/14 04:24:43  craigs
 * Fixed ifdef problem
 *
 * Revision 1.2  2002/08/13 14:25:25  craigs
 * Added trailing newlines to avoid Linux warnings
 *
 * Revision 1.1  2002/08/13 14:14:59  craigs
 * Initial version
 *
 */

 /*
   This code requires the Speex codec code available from http://speex.sourceforge.net 
  */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "speexcodec.h"
#endif

#include <codec/speexcodec.h>

extern "C" {
#if H323_SYSTEM_SPEEX
#include <speex.h>
#else
#include "speex/libspeex/speex.h"
#endif
};


#define new PNEW

#define EQUIVALENCE_COUNTRY_CODE       9  // Country code for Australia
#define EQUIVALENCE_T35EXTENSION       0
#define EQUIVALENCE_MANUFACTURER_CODE  61 // Allocated by Australian Communications Authority, Oct 2000

#define SAMPLES_PER_FRAME        160

const float MaxSampleValue   = 32767.0;
const float MinSampleValue   = -32767.0;


/////////////////////////////////////////////////////////////////////////

static int Speex_Bits_Per_Second(int mode) {
    void *tmp_coder_state;
    int bitrate;
    tmp_coder_state = speex_encoder_init(&speex_nb_mode);
    speex_encoder_ctl(tmp_coder_state, SPEEX_SET_QUALITY, &mode);
    speex_encoder_ctl(tmp_coder_state, SPEEX_GET_BITRATE, &bitrate);
    speex_encoder_destroy(tmp_coder_state); 
    return bitrate;
}

static int Speex_Bytes_Per_Frame(int mode) {
    int bits_per_frame = Speex_Bits_Per_Second(mode) / 50; // (20ms frame size)
    return ((bits_per_frame+7)/8); // round up
}

OpalMediaFormat const OpalSpeexNarrow_5k95(OPAL_SPEEX_NARROW_5k95,
                                           OpalMediaFormat::DefaultAudioSessionID,
                                           RTP_DataFrame::DynamicBase,
                                           OPAL_SPEEX_NARROW_5k95,
                                           TRUE,  // Needs jitter
                                           Speex_Bits_Per_Second(2),
                                           Speex_Bytes_Per_Frame(2),
                                           SAMPLES_PER_FRAME, // 20 milliseconds
                                           OpalMediaFormat::AudioClockRate);

OpalMediaFormat const OpalSpeexNarrow_8k(OPAL_SPEEX_NARROW_8k,
                                           OpalMediaFormat::DefaultAudioSessionID,
                                           RTP_DataFrame::DynamicBase,
                                           OPAL_SPEEX_NARROW_8k,
                                           TRUE,  // Needs jitter
                                           Speex_Bits_Per_Second(3),
                                           Speex_Bytes_Per_Frame(3),
                                           SAMPLES_PER_FRAME, // 20 milliseconds
                                           OpalMediaFormat::AudioClockRate);

OpalMediaFormat const OpalSpeexNarrow_11k(OPAL_SPEEX_NARROW_11k,
                                           OpalMediaFormat::DefaultAudioSessionID,
                                           RTP_DataFrame::DynamicBase,
                                           OPAL_SPEEX_NARROW_11k,
                                           TRUE,  // Needs jitter
                                           Speex_Bits_Per_Second(4),
                                           Speex_Bytes_Per_Frame(4),
                                           SAMPLES_PER_FRAME, // 20 milliseconds
                                           OpalMediaFormat::AudioClockRate);

OpalMediaFormat const OpalSpeexNarrow_15k(OPAL_SPEEX_NARROW_15k,
                                           OpalMediaFormat::DefaultAudioSessionID,
                                           RTP_DataFrame::DynamicBase,
                                           OPAL_SPEEX_NARROW_15k,
                                           TRUE,  // Needs jitter
                                           Speex_Bits_Per_Second(5),
                                           Speex_Bytes_Per_Frame(5),
                                           SAMPLES_PER_FRAME, // 20 milliseconds
                                           OpalMediaFormat::AudioClockRate);

OpalMediaFormat const OpalSpeexNarrow_18k2(OPAL_SPEEX_NARROW_18k2,
                                           OpalMediaFormat::DefaultAudioSessionID,
                                           RTP_DataFrame::DynamicBase,
                                           OPAL_SPEEX_NARROW_18k2,
                                           TRUE,  // Needs jitter
                                           Speex_Bits_Per_Second(6),
                                           Speex_Bytes_Per_Frame(6),
                                           SAMPLES_PER_FRAME, // 20 milliseconds
                                           OpalMediaFormat::AudioClockRate);


/////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

SpeexNonStandardAudioCapability::SpeexNonStandardAudioCapability(int mode)
  : H323NonStandardAudioCapability(1, 1,
                                   EQUIVALENCE_COUNTRY_CODE,
                                   EQUIVALENCE_T35EXTENSION,
                                   EQUIVALENCE_MANUFACTURER_CODE,
                                   NULL, 0, 0, P_MAX_INDEX)
{
  PStringStream s;
  s << "Speex bs" << speex_nb_mode.bitstream_version << " Narrow" << mode;
  PINDEX len = s.GetLength();
  memcpy(nonStandardData.GetPointer(len), (const char *)s, len);
}


/////////////////////////////////////////////////////////////////////////

SpeexNarrow2AudioCapability::SpeexNarrow2AudioCapability()
  : SpeexNonStandardAudioCapability(2) 
{
}


PObject * SpeexNarrow2AudioCapability::Clone() const
{
  return new SpeexNarrow2AudioCapability(*this);
}


PString SpeexNarrow2AudioCapability::GetFormatName() const
{
  return OpalSpeexNarrow_5k95;
}


/////////////////////////////////////////////////////////////////////////

SpeexNarrow3AudioCapability::SpeexNarrow3AudioCapability()
  : SpeexNonStandardAudioCapability(3) 
{
}


PObject * SpeexNarrow3AudioCapability::Clone() const
{
  return new SpeexNarrow3AudioCapability(*this);
}


PString SpeexNarrow3AudioCapability::GetFormatName() const
{
  return OpalSpeexNarrow_8k;
}


/////////////////////////////////////////////////////////////////////////

SpeexNarrow4AudioCapability::SpeexNarrow4AudioCapability()
  : SpeexNonStandardAudioCapability(4) 
{
}


PObject * SpeexNarrow4AudioCapability::Clone() const
{
  return new SpeexNarrow4AudioCapability(*this);
}


PString SpeexNarrow4AudioCapability::GetFormatName() const
{
  return OpalSpeexNarrow_11k;
}


/////////////////////////////////////////////////////////////////////////

SpeexNarrow5AudioCapability::SpeexNarrow5AudioCapability()
  : SpeexNonStandardAudioCapability(5) 
{
}


PObject * SpeexNarrow5AudioCapability::Clone() const
{
  return new SpeexNarrow5AudioCapability(*this);
}


PString SpeexNarrow5AudioCapability::GetFormatName() const
{
  return OpalSpeexNarrow_15k;
}


/////////////////////////////////////////////////////////////////////////

SpeexNarrow6AudioCapability::SpeexNarrow6AudioCapability()
  : SpeexNonStandardAudioCapability(6) 
{
}


PObject * SpeexNarrow6AudioCapability::Clone() const
{
  return new SpeexNarrow6AudioCapability(*this);
}


PString SpeexNarrow6AudioCapability::GetFormatName() const
{
  return OpalSpeexNarrow_18k2;
}

#endif


/////////////////////////////////////////////////////////////////////////////

Opal_Speex_Transcoder::Opal_Speex_Transcoder(const OpalTranscoderRegistration & registration,
                                             unsigned inputBytesPerFrame,
                                             unsigned outputBytesPerFrame)
  : OpalFramedTranscoder(registration, inputBytesPerFrame, outputBytesPerFrame)
{
  bits = new SpeexBits;
  speex_bits_init(bits);
}


Opal_Speex_Transcoder::~Opal_Speex_Transcoder()
{
  speex_bits_destroy(bits); 
  delete bits;
}


///////////////////////////////////////////////////////////////////////////////

Opal_Speex_Decoder::Opal_Speex_Decoder(const OpalTranscoderRegistration & registration,
                                       int mode)
  : Opal_Speex_Transcoder(registration, Speex_Bytes_Per_Frame(mode), SAMPLES_PER_FRAME*2)
{
  decoder = speex_decoder_init(&speex_nb_mode);
}


Opal_Speex_Decoder::~Opal_Speex_Decoder()
{
  if (decoder != NULL)
    speex_decoder_destroy(decoder);
}


BOOL Opal_Speex_Decoder::ConvertFrame(const BYTE * src, BYTE * dst)
{
  float floatData[SAMPLES_PER_FRAME];

  // decode Speex data to floats
  speex_bits_read_from(bits, (char *)src, inputBytesPerFrame); 
  speex_decode(decoder, bits, floatData); 

  // convert float to PCM
  PINDEX i;
  for (i = 0; i < SAMPLES_PER_FRAME; i++) {
    float sample = floatData[i];
    if (sample < MinSampleValue)
      sample = MinSampleValue;
    else if (sample > MaxSampleValue)
      sample = MaxSampleValue;
    ((short*)dst)[i] = (short)sample;
  }

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

Opal_Speex_Encoder::Opal_Speex_Encoder(const OpalTranscoderRegistration & registration,
                                       int mode)
  : Opal_Speex_Transcoder(registration, SAMPLES_PER_FRAME*2, Speex_Bytes_Per_Frame(mode))
{
  encoder = speex_encoder_init(&speex_nb_mode);
  speex_encoder_ctl(encoder, SPEEX_GET_FRAME_SIZE, &encoder_frame_size);
  speex_encoder_ctl(encoder, SPEEX_SET_QUALITY,    &mode);
  PTRACE(3, "Codec\tSpeex encoder created");
}


Opal_Speex_Encoder::~Opal_Speex_Encoder()
{
  if (encoder != NULL)
    speex_encoder_destroy(encoder);
}
 

BOOL Opal_Speex_Encoder::ConvertFrame(const BYTE * src, BYTE * dst)
{
  // convert PCM to float
  float floatData[SAMPLES_PER_FRAME];
  PINDEX i;
  for (i = 0; i < SAMPLES_PER_FRAME; i++)
    floatData[i] = ((short *)src)[i];

  // encode PCM data in sampleBuffer to buffer
  speex_bits_reset(bits); 
  speex_encode(encoder, floatData, bits); 

  speex_bits_write(bits, (char *)dst, encoder_frame_size); 

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

Opal_Speex_5k95_PCM::Opal_Speex_5k95_PCM(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Decoder(registration, 2)
{
}


Opal_PCM_Speex_5k95::Opal_PCM_Speex_5k95(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Encoder(registration, 2)
{
}


Opal_Speex_8k_PCM::Opal_Speex_8k_PCM(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Decoder(registration, 3)
{
}


Opal_PCM_Speex_8k::Opal_PCM_Speex_8k(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Encoder(registration, 3)
{
}


Opal_Speex_11k_PCM::Opal_Speex_11k_PCM(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Decoder(registration, 4)
{
}


Opal_PCM_Speex_11k::Opal_PCM_Speex_11k(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Encoder(registration, 4)
{
}


Opal_Speex_15k_PCM::Opal_Speex_15k_PCM(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Decoder(registration, 5)
{
}


Opal_PCM_Speex_15k::Opal_PCM_Speex_15k(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Encoder(registration, 5)
{
}


Opal_Speex_18k2_PCM::Opal_Speex_18k2_PCM(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Decoder(registration, 6)
{
}


Opal_PCM_Speex_18k2::Opal_PCM_Speex_18k2(const OpalTranscoderRegistration & registration)
  : Opal_Speex_Encoder(registration, 6)
{
}


/////////////////////////////////////////////////////////////////////////
