/*
 * ilbccodec.cxx
 *
 * Internet Low Bitrate Codec
 *
 * Open H323 Library
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: ilbccodec.cxx,v $
 * Revision 1.2006  2005/05/25 17:05:17  dsandras
 * Fixed iLBC codec, thanks Robert!
 *
 * Revision 2.4  2005/02/21 12:19:54  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.3  2004/09/01 12:21:27  rjongbloed
 * Added initialisation of H323EndPoints capability table to be all codecs so can
 *   correctly build remote caps from fqast connect params. This had knock on effect
 *   with const keywords added in numerous places.
 *
 * Revision 2.2  2004/03/11 06:54:28  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.1  2004/02/19 10:47:02  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 1.2  2004/01/27 12:44:01  csoutheren
 * Fixed incorrect initialisation in iLBC codec initializer
 *   Thanks to Borko Jandras
 *
 * Revision 1.1  2003/06/06 02:19:12  rjongbloed
 * Added iLBC codec
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "ilbccodec.h"
#endif

#include <codec/ilbccodec.h>

extern "C" {
#include "iLBC/iLBC_encode.h" 
#include "iLBC/iLBC_decode.h" 
};
    

#define new PNEW


static OpalAudioFormat Opal_iLBC_13k3(
                          OPAL_ILBC_13k3,
                          RTP_DataFrame::DynamicBase,
                          "ilbc",
                          NO_OF_BYTES_30MS,
                          BLOCKL_30MS,
                          7, 3);

static OpalAudioFormat Opal_iLBC_15k2(
                          OPAL_ILBC_15k2,
                          RTP_DataFrame::DynamicBase,
                          "ilbc",
                          NO_OF_BYTES_20MS,
                          BLOCKL_20MS,
                          7, 4);


/////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

H323_iLBC_Capability::H323_iLBC_Capability(const H323EndPoint & endpoint, Speed s)
  : H323NonStandardAudioCapability(endpoint,
                                   (const BYTE *)(s == e_13k3 ? OPAL_ILBC_13k3 : OPAL_ILBC_15k2))
{
  speed = s;
}


PObject * H323_iLBC_Capability::Clone() const
{
  return new H323_iLBC_Capability(*this);
}


PString H323_iLBC_Capability::GetFormatName() const
{
  return speed == e_13k3 ? OPAL_ILBC_13k3 : OPAL_ILBC_15k2;
}


#endif


/////////////////////////////////////////////////////////////////////////////

Opal_iLBC_Decoder::Opal_iLBC_Decoder(const OpalTranscoderRegistration & registration, int speed)
  : OpalFramedTranscoder(registration,
                         speed == 30 ? NO_OF_BYTES_30MS : NO_OF_BYTES_20MS,
                         speed == 30 ? BLOCKL_30MS * 2  : BLOCKL_20MS * 2)
{
  decoder = (struct iLBC_Dec_Inst_t_ *)malloc((unsigned)sizeof(struct iLBC_Dec_Inst_t_));
  if (decoder != NULL) 
    initDecode(decoder, speed, 1); 

  PTRACE(3, "Codec\tiLBC decoder created");
}


Opal_iLBC_Decoder::~Opal_iLBC_Decoder()
{
  if (decoder != NULL)
    free(decoder);
}
 

BOOL Opal_iLBC_Decoder::ConvertFrame(const BYTE * src, BYTE * dst)
{
  float block[BLOCKL_MAX];

  /* do actual decoding of block */ 
  iLBC_decode(block, (unsigned char *)src, decoder, 1);

  /* convert to short */     
  for (int i = 0; i < decoder->blockl; i++) {
    float tmp = block[i];
    if (tmp < MIN_SAMPLE)
      tmp = MIN_SAMPLE;
    else if (tmp > MAX_SAMPLE)
      tmp = MAX_SAMPLE;
    ((short *)dst)[i] = (short)tmp;
  }

  return TRUE;
}


Opal_iLBC_13k3_PCM::Opal_iLBC_13k3_PCM(const OpalTranscoderRegistration & registration)
  : Opal_iLBC_Decoder(registration, 30)
{
}


Opal_iLBC_15k2_PCM::Opal_iLBC_15k2_PCM(const OpalTranscoderRegistration & registration)
  : Opal_iLBC_Decoder(registration, 20)
{
}


Opal_iLBC_Encoder::Opal_iLBC_Encoder(const OpalTranscoderRegistration & registration, int speed)
  : OpalFramedTranscoder(registration,
                         speed == 30 ? BLOCKL_30MS * 2  : BLOCKL_20MS * 2,
                         speed == 30 ? NO_OF_BYTES_30MS : NO_OF_BYTES_20MS)
{
    encoder = (struct iLBC_Enc_Inst_t_ *)malloc((unsigned)sizeof(struct iLBC_Enc_Inst_t_));
    if (encoder != 0) 
      initEncode(encoder, speed); 

  PTRACE(3, "Codec\tiLBC encoder created");
}


Opal_iLBC_Encoder::~Opal_iLBC_Encoder()
{
  if (encoder != NULL)
    free(encoder);
}
 

BOOL Opal_iLBC_Encoder::ConvertFrame(const BYTE * src, BYTE * dst)
{
  float block[BLOCKL_MAX];

  /* convert signal to float */
  for (int i = 0; i < encoder->blockl; i++)
    block[i] = (float)((short *)src)[i];

  /* do the actual encoding */
  iLBC_encode(dst, block, encoder);
  return TRUE; 
}


Opal_PCM_iLBC_13k3::Opal_PCM_iLBC_13k3(const OpalTranscoderRegistration & registration)
  : Opal_iLBC_Encoder(registration, 30)
{
}


Opal_PCM_iLBC_15k2::Opal_PCM_iLBC_15k2(const OpalTranscoderRegistration & registration)
  : Opal_iLBC_Encoder(registration, 20)
{
}


/////////////////////////////////////////////////////////////////////////////
