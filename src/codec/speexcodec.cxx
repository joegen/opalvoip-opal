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
 * Revision 1.2018.2.1  2006/03/20 05:20:55  csoutheren
 * *** empty log message ***
 *
 * Revision 2.17  2006/01/31 03:28:04  csoutheren
 * Changed to compile on MSVC 6
 *
 * Revision 2.16  2006/01/30 02:47:22  csoutheren
 * Fixed use of system speex library on systems with includes in strange places
 *
 * Revision 2.15  2006/01/30 02:23:16  csoutheren
 * First cut at fixing problem with speex libraries
 *
 * Revision 2.14  2006/01/14 10:40:16  dsandras
 * Applied typecast patch thanks to Dennis White <denniswhite98 ___AT_ yahoo.com>
 *
 * Revision 2.13  2005/12/30 14:33:12  dsandras
 * Added support for Packet Loss Concealment frames for framed codecs supporting it similarly to what was done for OpenH323.
 *
 * Revision 2.12  2005/12/29 16:22:33  dsandras
 * Simplified code and fixed wideband support.
 *
 * Revision 2.11  2005/12/28 20:01:52  dsandras
 * Reverted mode to mode 6. Get rid of encoder_frame_size.
 *
 * Revision 2.10  2005/12/28 16:05:24  dsandras
 * Changed speex mode to 8.
 *
 * Revision 2.9  2005/12/27 20:52:22  dsandras
 * Use SPEEX encoding name only for the default versions of Speex defined
 * in the draft.
 *
 * Revision 2.8  2005/12/24 17:52:09  dsandras
 * Fixed calls to speex_encoder_ctl so that the correct mode is used. Added support for the mode 6 wideband codec.
 *
 * Revision 2.7  2005/12/20 20:41:16  dsandras
 * Fixed encoding name used by SIP.
 *
 * Revision 2.6  2005/11/23 21:57:52  dsandras
 * Updated to Speex 1.1.11.
 *
 * Revision 2.5  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.4  2005/02/21 12:19:54  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.3  2004/03/11 06:54:28  csoutheren
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
#if OPAL_SYSTEM_SPEEX
#  include "speex/libspeex/speex_config_types.h"
#  if OPAL_HAVE_SPEEX_SPEEX_H
#    include <speex/speex.h>
#  else
    #include <speex.h>
#  endif
#else
#  include "speex/libspeex/speex.h"
#endif
};


#define new PNEW

#define EQUIVALENCE_COUNTRY_CODE       9  // Country code for Australia
#define EQUIVALENCE_T35EXTENSION       0
#define EQUIVALENCE_MANUFACTURER_CODE  61 // Allocated by Australian Communications Authority, Oct 2000

#define NARROW_SAMPLES_PER_FRAME        160
#define WIDE_SAMPLES_PER_FRAME          320

const float MaxSampleValue   = 32767.0;
const float MinSampleValue   = -32767.0;


/////////////////////////////////////////////////////////////////////////

static int Speex_Bits_Per_Second(int mode, int sampleRate) {
    void *tmp_coder_state;
    int bitrate;
    if (sampleRate == 8000)
      tmp_coder_state = speex_encoder_init(&speex_nb_mode);
    else
      tmp_coder_state = speex_encoder_init(&speex_wb_mode);
    speex_encoder_ctl(tmp_coder_state, SPEEX_SET_MODE, &mode);
    speex_encoder_ctl(tmp_coder_state, SPEEX_GET_BITRATE, &bitrate);
    speex_encoder_destroy(tmp_coder_state); 
    return bitrate;
}

static int Speex_Bytes_Per_Frame(int mode, int sampleRate) {
    int bits_per_frame = Speex_Bits_Per_Second(mode, sampleRate) / 50; // (20ms frame size)
    return ((bits_per_frame+7)/8); // round up
}


const OpalAudioFormat & GetOpalSpeexNarrow_5k95()
{
  static const OpalAudioFormat SpeexNarrow_5k95(
    OPAL_SPEEX_NARROW_5k95,
    RTP_DataFrame::DynamicBase,
    OPAL_SPEEX_NARROW_5k95,
    Speex_Bytes_Per_Frame(2, 8000),
    NARROW_SAMPLES_PER_FRAME, // 20 milliseconds
    1, 1);
  return SpeexNarrow_5k95;
}

const OpalAudioFormat & GetOpalSpeexNarrow_8k()
{
  static const OpalAudioFormat SpeexNarrow_8k(
    OPAL_SPEEX_NARROW_8k,
    RTP_DataFrame::DynamicBase,
    "SPEEX",
    Speex_Bytes_Per_Frame(3, 8000),
    NARROW_SAMPLES_PER_FRAME, // 20 milliseconds
    1, 1);
  return SpeexNarrow_8k;
}

const OpalAudioFormat & GetOpalSpeexNarrow_11k()
{
  static const OpalAudioFormat SpeexNarrow_11k(
    OPAL_SPEEX_NARROW_11k,
    RTP_DataFrame::DynamicBase,
    OPAL_SPEEX_NARROW_11k,
    Speex_Bytes_Per_Frame(4, 8000),
    NARROW_SAMPLES_PER_FRAME, // 20 milliseconds
    1, 1);
  return SpeexNarrow_11k;
}

const OpalAudioFormat & GetOpalSpeexNarrow_15k()
{
  static const OpalAudioFormat SpeexNarrow_15k(
    OPAL_SPEEX_NARROW_15k,
    RTP_DataFrame::DynamicBase,
    OPAL_SPEEX_NARROW_15k,
    Speex_Bytes_Per_Frame(5, 8000),
    NARROW_SAMPLES_PER_FRAME, // 20 milliseconds
    1, 1);
  return SpeexNarrow_15k;
}

const OpalAudioFormat & GetOpalSpeexNarrow_18k2()
{
  static const OpalAudioFormat SpeexNarrow_18k2(
    OPAL_SPEEX_NARROW_18k2,
    RTP_DataFrame::DynamicBase,
    OPAL_SPEEX_NARROW_18k2,
    Speex_Bytes_Per_Frame(6, 8000),
    NARROW_SAMPLES_PER_FRAME, // 20 milliseconds
    1, 1);
  return SpeexNarrow_18k2;
}

const OpalAudioFormat & GetOpalSpeexWide_20k6()
{
  static const OpalAudioFormat SpeexWide_20k6(
    OPAL_SPEEX_WIDE_20k6,
    RTP_DataFrame::DynamicBase,
    "SPEEX",
    Speex_Bytes_Per_Frame(6, 16000),
    WIDE_SAMPLES_PER_FRAME, // 20 milliseconds
    1, 1,
    256,
    16000);
  return SpeexWide_20k6;
}

/////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

SpeexNonStandardAudioCapability::SpeexNonStandardAudioCapability(int mode,
								 int sampleRate)
  : H323NonStandardAudioCapability(EQUIVALENCE_COUNTRY_CODE,
                                   EQUIVALENCE_T35EXTENSION,
                                   EQUIVALENCE_MANUFACTURER_CODE,
                                   NULL, 0, 0, P_MAX_INDEX)
{
  PStringStream s;
  if (sampleRate == 8000)
    s << "Speex bs" << speex_nb_mode.bitstream_version << " Narrow" << mode;
  else
    s << "Speex bs" << speex_wb_mode.bitstream_version << " Wide" << mode;
  PINDEX len = s.GetLength();
  memcpy(nonStandardData.GetPointer(len), (const char *)s, len);
}


/////////////////////////////////////////////////////////////////////////

SpeexNarrow2AudioCapability::SpeexNarrow2AudioCapability()
  : SpeexNonStandardAudioCapability(2, 8000) 
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
  : SpeexNonStandardAudioCapability(3, 8000) 
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
  : SpeexNonStandardAudioCapability(4, 8000) 
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
  : SpeexNonStandardAudioCapability(5, 8000) 
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
  : SpeexNonStandardAudioCapability(6, 8000) 
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


/////////////////////////////////////////////////////////////////////////

SpeexWide6AudioCapability::SpeexWide6AudioCapability()
  : SpeexNonStandardAudioCapability(6, 16000) 
{
}


PObject * SpeexWide6AudioCapability::Clone() const
{
  return new SpeexWide6AudioCapability(*this);
}


PString SpeexWide6AudioCapability::GetFormatName() const
{
  return OpalSpeexWide_20k6;
}


#endif


/////////////////////////////////////////////////////////////////////////////

Opal_Speex_Transcoder::Opal_Speex_Transcoder(const OpalMediaFormat & inputMediaFormat,
                                             const OpalMediaFormat & outputMediaFormat,
                                             unsigned inputBytesPerFrame,
                                             unsigned outputBytesPerFrame)
  : OpalFramedTranscoder(inputMediaFormat, outputMediaFormat, inputBytesPerFrame, outputBytesPerFrame)
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

Opal_Speex_Decoder::Opal_Speex_Decoder(const OpalMediaFormat & inputMediaFormat, int mode, int sampleRate)
  : Opal_Speex_Transcoder(inputMediaFormat, OpalPCM16, Speex_Bytes_Per_Frame(mode, sampleRate), (sampleRate == 8000)?NARROW_SAMPLES_PER_FRAME*2:WIDE_SAMPLES_PER_FRAME*2)
{
  if (sampleRate == 8000)
    decoder = speex_decoder_init(&speex_nb_mode);
  else
    decoder = speex_decoder_init(&speex_wb_mode);
  samples_per_frame = (sampleRate == 8000)?NARROW_SAMPLES_PER_FRAME:WIDE_SAMPLES_PER_FRAME;
}


Opal_Speex_Decoder::~Opal_Speex_Decoder()
{
  if (decoder != NULL)
    speex_decoder_destroy(decoder);
}


BOOL Opal_Speex_Decoder::ConvertFrame(const BYTE * src, BYTE * dst)
{
  speex_bits_reset(bits);
  speex_bits_read_from(bits, (char *)src, inputBytesPerFrame); 
  speex_decode_int(decoder, bits, (spx_int16_t *) dst); 

  return TRUE;
}


BOOL Opal_Speex_Decoder::ConvertSilentFrame(BYTE * dst)
{
  speex_decode_int(decoder, NULL, (spx_int16_t *) dst);

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

Opal_Speex_Encoder::Opal_Speex_Encoder(const OpalMediaFormat & outputMediaFormat, int mode, int sampleRate)
  : Opal_Speex_Transcoder(OpalPCM16, outputMediaFormat, (sampleRate == 8000)?NARROW_SAMPLES_PER_FRAME*2:WIDE_SAMPLES_PER_FRAME*2, Speex_Bytes_Per_Frame(mode, sampleRate))
{
  if (sampleRate == 8000)
    encoder = speex_encoder_init(&speex_nb_mode);
  else
    encoder = speex_encoder_init(&speex_wb_mode);
  speex_encoder_ctl(encoder, SPEEX_SET_MODE, &mode);
  samples_per_frame = (sampleRate == 8000)?NARROW_SAMPLES_PER_FRAME:WIDE_SAMPLES_PER_FRAME;
  PTRACE(3, "Codec\tSpeex encoder created");
}


Opal_Speex_Encoder::~Opal_Speex_Encoder()
{
  if (encoder != NULL)
    speex_encoder_destroy(encoder);
}
 

BOOL Opal_Speex_Encoder::ConvertFrame(const BYTE * src, BYTE * dst)
{
  speex_bits_reset(bits); 
  speex_encode_int(encoder, (spx_int16_t *) src, bits); 

  speex_bits_write(bits, (char *)dst, outputBytesPerFrame); 

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

Opal_Speex_5k95_PCM::Opal_Speex_5k95_PCM()
  : Opal_Speex_Decoder(OpalSpeexNarrow_5k95, 2, 8000)
{
}


Opal_PCM_Speex_5k95::Opal_PCM_Speex_5k95()
  : Opal_Speex_Encoder(OpalSpeexNarrow_5k95, 2, 8000)
{
}


Opal_Speex_8k_PCM::Opal_Speex_8k_PCM()
  : Opal_Speex_Decoder(OpalSpeexNarrow_8k, 3, 8000)
{
}


Opal_PCM_Speex_8k::Opal_PCM_Speex_8k()
  : Opal_Speex_Encoder(OpalSpeexNarrow_8k, 3, 8000)
{
}


Opal_Speex_11k_PCM::Opal_Speex_11k_PCM()
  : Opal_Speex_Decoder(OpalSpeexNarrow_11k, 4, 8000)
{
}


Opal_PCM_Speex_11k::Opal_PCM_Speex_11k()
  : Opal_Speex_Encoder(OpalSpeexNarrow_11k, 4, 8000)
{
}


Opal_Speex_15k_PCM::Opal_Speex_15k_PCM()
  : Opal_Speex_Decoder(OpalSpeexNarrow_15k, 5, 8000)
{
}


Opal_PCM_Speex_15k::Opal_PCM_Speex_15k()
  : Opal_Speex_Encoder(OpalSpeexNarrow_15k, 5, 8000)
{
}


Opal_Speex_18k2_PCM::Opal_Speex_18k2_PCM()
  : Opal_Speex_Decoder(OpalSpeexNarrow_18k2, 6, 8000)
{
}


Opal_PCM_Speex_18k2::Opal_PCM_Speex_18k2()
  : Opal_Speex_Encoder(OpalSpeexNarrow_18k2, 6, 8000)
{
}


Opal_Speex_20k6_PCM::Opal_Speex_20k6_PCM()
  : Opal_Speex_Decoder(OpalSpeexWide_20k6, 6, 16000)
{
}


Opal_PCM_Speex_20k6::Opal_PCM_Speex_20k6()
  : Opal_Speex_Encoder(OpalSpeexWide_20k6, 6, 16000)
{
}


/////////////////////////////////////////////////////////////////////////
