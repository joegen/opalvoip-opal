/*
 * H.263 Plugin codec for OpenH323/OPAL
 *
 * This code is based on the following files from the OPAL project which
 * have been removed from the current build and distributions but are still
 * available in the CVS "attic"
 * 
 *    src/codecs/h263codec.cxx 
 *    include/codecs/h263codec.h 

 * The original files, and this version of the original code, are released under the same 
 * MPL 1.0 license. Substantial portions of the original code were contributed
 * by Salyens and March Networks and their right to be identified as copyright holders
 * of the original code portions and any parts now included in this new copy is asserted through 
 * their inclusion in the copyright notices below.
 *
 * Copyright (C) 2007 Matthias Schneider
 * Copyright (C) 2006 Post Increment
 * Copyright (C) 2005 Salyens
 * Copyright (C) 2001 March Networks Corporation
 * Copyright (C) 1999-2000 Equivalence Pty. Ltd.
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
 * Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
 *                 Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *
 */

/*
  Notes
  -----

 */

#ifndef __H263P_1998_H__
#define __H263P_1998_H__ 1
#include <codec/opalplugin.h>
#include "h263pframe.h"
#include "critsect.h"

typedef unsigned char BYTE;
typedef bool BOOL;

#define FALSE false
#define TRUE  true

#define H263P_CLOCKRATE        90000
#define H263P_BITRATE         327600
#define H263P_PAYLOAD_SIZE      1400
#define H263P_FRAME_RATE          25
#define H263P_KEY_FRAME_INTERVAL 2.0
#define H263P_TRACELEVEL           4

#define CIF_WIDTH       352
#define CIF_HEIGHT      288

#define CIF4_WIDTH      (CIF_WIDTH*2)
#define CIF4_HEIGHT     (CIF_HEIGHT*2)

#define CIF16_WIDTH     (CIF_WIDTH*4)
#define CIF16_HEIGHT    (CIF_HEIGHT*4)

#define QCIF_WIDTH     (CIF_WIDTH/2)
#define QCIF_HEIGHT    (CIF_HEIGHT/2)

#define SQCIF_WIDTH     128
#define SQCIF_HEIGHT    96

#define MAX_YUV420P_FRAME_SIZE (((CIF16_WIDTH * CIF16_HEIGHT * 3) / 2) + FF_INPUT_BUFFER_PADDING_SIZE)

class H263PEncoderContext
{
  public:
    H263PEncoderContext();
    ~H263PEncoderContext();
    int EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags);
    unsigned _frameWidth, _frameHeight;

  protected:
    BOOL OpenCodec();
    void CloseCodec();

    unsigned char _inputFrameBuffer[MAX_YUV420P_FRAME_SIZE];
    AVCodec        *_codec;
    AVCodecContext *_context;
    AVFrame        *_inputFrame;

    int _videoQMax, _videoQMin; // dynamic video quality min/max limits, 1..31
    int _videoQuality; // current video encode quality setting, 1..31

    int _bitRate;

    H263PFrame* _txH263PFrame;
    int _frameCount;
    CriticalSection _mutex;
};


class H263PDecoderContext
{
  public:
    H263PDecoderContext();
    ~H263PDecoderContext();

    bool DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags);

  protected:
    bool OpenCodec();
    void CloseCodec();

    AVCodec        *_codec;
    AVCodecContext *_context;
    AVFrame        *_outputFrame;

    H263PFrame* _rxH263PFrame;
    int _frameCount;
    unsigned int _skippedFrameCounter;
    bool _gotIFrame;
};

static void * create_encoder(const struct PluginCodec_Definition * /*codec*/);
static int encoder_set_options(const PluginCodec_Definition *, void * _context, const char * , void * parm, unsigned * parmLen);
static void destroy_encoder(const struct PluginCodec_Definition * /*codec*/, void * _context);
static int codec_encoder(const struct PluginCodec_Definition * , void * _context, const void * from, unsigned * fromLen,
                         void * to, unsigned * toLen, unsigned int * flag);
static void * create_decoder(const struct PluginCodec_Definition *);
static void destroy_decoder(const struct PluginCodec_Definition * /*codec*/, void * _context);
static int codec_decoder(const struct PluginCodec_Definition *, void * _context, const void * from, unsigned * fromLen, void * to,
                         unsigned * toLen, unsigned int * flag);
static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *);
static int get_codec_options(const struct PluginCodec_Definition * codec, void *, const char *, void * parm, unsigned * parmLen);

/////////////////////////////////////////////////////////////////////////////


static struct PluginCodec_information licenseInfo = {
  1145863600,                                                   // timestamp =  Mon 24 Apr 2006 07:26:40 AM UTC

  "Matthias Schneider, Craig Southeren"                         // source code author
  "Guilhem Tardy, Derek Smithies",
  "1.0",                                                        // source code version
  "openh323@openh323.org",                                      // source code email
  "http://sourceforge.net/projects/openh323",                   // source code URL
  "Copyright (C) 2007 Matthias Schneider"                       // source code copyright
  ", Copyright (C) 2006 by Post Increment"
  ", Copyright (C) 2005 Salyens"
  ", Copyright (C) 2001 March Networks Corporation"
  ", Copyright (C) 1999-2000 Equivalence Pty. Ltd.",
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  "FFMPEG",                                                     // codec description
  "Michael Niedermayer, Fabrice Bellard",                       // codec author
  "",                                                           // codec version
  "ffmpeg-devel-request@mplayerhq.hu",                          // codec email
  "http://ffmpeg.mplayerhq.hu",                                 // codec URL
  "Copyright (c) 2000-2001 Fabrice Bellard"                     // codec copyright information
  ", Copyright (c) 2002-2003 Michael Niedermayer",
  "GNU LESSER GENERAL PUBLIC LICENSE, Version 2.1, February 1999", // codec license
  PluginCodec_License_LGPL                                         // codec license code
};

static const char YUV420PDesc[]  = { "YUV420P" };

static const char h263QCIFDesc[]  = { "H.263-QCIF" };
static const char h263CIFDesc[]   = { "H.263-CIF" };
static const char h263Desc[]      = { "H.263" };

static const char sdpH263[]   = { "h263-1998" };

static PluginCodec_ControlDefn EncoderControls[] = {
  { "get_codec_options", get_codec_options },
  { "set_codec_options", encoder_set_options },
  { NULL }
};

static PluginCodec_ControlDefn DecoderControls[] = {
  { "get_codec_options",    get_codec_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

static struct PluginCodec_Option const qcifMPI =
  { PluginCodec_IntegerOption, "QCIF MPI", false, PluginCodec_MaxMerge, "1", "QCIF", "0", 0, "1", "32" };

static struct PluginCodec_Option const cifMPI =
  { PluginCodec_IntegerOption, "CIF MPI",  false, PluginCodec_MaxMerge, "1", "CIF",  "0", 0, "1", "32" };

static struct PluginCodec_Option const sqcifMPI =
  { PluginCodec_IntegerOption, "SQCIF MPI", false, PluginCodec_MaxMerge, "1", "SQCIF", "0", 0, "1", "32" };

static struct PluginCodec_Option const cif4MPI =
  { PluginCodec_IntegerOption, "CIF4 MPI",  false, PluginCodec_MaxMerge, "0", "CIF4", "0", 0, "1", "32" };

static struct PluginCodec_Option const cif16MPI =
  { PluginCodec_IntegerOption, "CIF16 MPI", false, PluginCodec_MaxMerge, "0", "CIF16", "0", 0, "1", "32" };

static struct PluginCodec_Option const sifMPI =
  { PluginCodec_StringOption, "SIF MPI", false, PluginCodec_EqualMerge, "320,240,1", "CUSTOM"};

static struct PluginCodec_Option const sif4MPI =
  { PluginCodec_StringOption, "SIF4 MPI", false, PluginCodec_EqualMerge, "640,480,1", "CUSTOM"};

/* All of the annexes below are turned off and set to read/only because this
   implementation does not support them. Their presence here is so that if
   someone out there does a different implementation of the codec and copies
   this file as a template, they will get them and hopefully notice that they
   can just make them read/write and/or turned on.
 */
static struct PluginCodec_Option const annexF =
  { PluginCodec_BoolOption,    "Annex F",   false,  PluginCodec_MinMerge, "T", "F", "F" };

static struct PluginCodec_Option const annexI =
  { PluginCodec_BoolOption,    "Annex I",   false,  PluginCodec_MinMerge, "T", "I", "F" };

static struct PluginCodec_Option const annexJ =
  { PluginCodec_BoolOption,    "Annex J",   true,  PluginCodec_MinMerge, "T", "J", "F" };

static struct PluginCodec_Option const annexK =
  { PluginCodec_IntegerOption, "Annex K",   true,  PluginCodec_EqualMerge, "0", "K", "0", 0, "0", "4" };

static struct PluginCodec_Option const annexN =
  { PluginCodec_BoolOption,    "Annex N",   true,  PluginCodec_AndMerge, "0", "N", "0" };

static struct PluginCodec_Option const annexP =
  { PluginCodec_BoolOption,    "Annex P",   true,  PluginCodec_AndMerge, "0", "P", "0" };

static struct PluginCodec_Option const annexT =
  { PluginCodec_BoolOption,    "Annex T",   true,  PluginCodec_AndMerge, "0", "T", "0" };

static struct PluginCodec_Option const annexD =
  { PluginCodec_BoolOption,    "Annex D",   true,  PluginCodec_MinMerge, "T", "D", "F" };

/*
static struct PluginCodec_Option const * const qcifOptionTable[] = {
  &qcifMPI,
  NULL
};

static struct PluginCodec_Option const * const cifOptionTable[] = {
  &cifMPI,
  NULL
};
*/

static struct PluginCodec_Option const * const xcifOptionTable[] = {
  &qcifMPI,
  &cifMPI,
  &sqcifMPI,
  &cif4MPI,
  &cif16MPI,
  &sifMPI,
  &sif4MPI,
  &annexF,
  &annexI,
  &annexJ,
  &annexK,
  &annexN,
  &annexP,
  &annexT,
  &annexD,
  NULL
};


/////////////////////////////////////////////////////////////////////////////
/*
static struct PluginCodec_Definition h263CodecDefn[6] = {

{ 
  // CIF only encoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |         // specified RTP type
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  h263CIFDesc,                        // text decription
  YUV420PDesc,                        // source format
  h263CIFDesc,                        // destination format

  cifOptionTable,                     // user data 

  H263P_CLOCKRATE,                     // samples per second
  H263P_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  EncoderControls,                    // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // CIF only decoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |         // specified RTP type
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  h263CIFDesc,                        // text decription
  h263CIFDesc,                        // source format
  YUV420PDesc,                        // destination format

  cifOptionTable,                     // user data 

  H263P_CLOCKRATE,                     // samples per second
  H263P_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  DecoderControls,                    // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // QCIF only encoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |         // specified RTP type
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  h263QCIFDesc,                       // text decription
  YUV420PDesc,                        // source format
  h263QCIFDesc,                       // destination format

  qcifOptionTable,                    // user data 

  H263P_CLOCKRATE,                     // samples per second
  H263P_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  EncoderControls,                    // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // QCIF only decoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |         // specified RTP type
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  h263QCIFDesc,                       // text decription
  h263QCIFDesc,                       // source format
  YUV420PDesc,                        // destination format

  qcifOptionTable,                    // user data 

  H263P_CLOCKRATE,                     // samples per second
  H263P_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  DecoderControls,                    // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
*/
static struct PluginCodec_Definition h263CodecDefn[2] = {
{ 
  // All frame sizes (dynamic) encoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |         // specified RTP type
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  h263Desc,                           // text decription
  YUV420PDesc,                        // source format
  h263Desc,                           // destination format

  xcifOptionTable,                    // user data 

  H263P_CLOCKRATE,                     // samples per second
  H263P_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF4_WIDTH,                         // frame width
  CIF4_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  EncoderControls,                    // codec controls

  PluginCodec_H323Codec_NoH323,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // All frame sizes (dynamic) decoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |         // specified RTP type
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  h263Desc,                           // text decription
  h263Desc,                           // source format
  YUV420PDesc,                        // destination format

  xcifOptionTable,                    // user data 

  H263P_CLOCKRATE,                     // samples per second
  H263P_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF4_WIDTH,                         // frame width
  CIF4_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  DecoderControls,                    // codec controls

  PluginCodec_H323Codec_NoH323,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

};


/////////////////////////////////////////////////////////////////////////////
#endif /* __H263P_1998_H__ */
