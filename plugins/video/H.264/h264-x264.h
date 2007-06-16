/*
 * H.264 Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) Matthias Schneider, All Rights Reserved
 *
 * This code is based on the file h261codec.cxx from the OPAL project released
 * under the MPL 1.0 license which contains the following:
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
 * Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
 *                 Michele Piccini (michele@piccini.com)
 *                 Derek Smithies (derek@indranet.co.nz)
 *
 *
 */

/*
  Notes
  -----

 */

#ifndef __H264_X264_H__
#define __H264_X264_H__ 1

#include <stdarg.h>
#include <stdint.h>
#include <codec/opalplugin.h>
#include "shared/h264frame.h"
#include "critsect.h"


extern "C" {
  #include <ffmpeg/avcodec.h>
  PLUGIN_CODEC_IMPLEMENT(H264)
};

#define CIF_WIDTH 352
#define CIF_HEIGHT 288
#define QCIF_WIDTH 176
#define QCIF_HEIGHT 144
#define IT_QCIF 0
#define IT_CIF 1

typedef unsigned char u_char;

static void logCallbackFFMPEG (void* v, int level, const char* fmt , va_list arg);

class H264EncoderContext 
{
  public:
    H264EncoderContext ();
    ~H264EncoderContext ();

    void SetTargetBitRate (int rate);
    void SetEncodingQuality (int quality);
    void SetFrameWidth (int width);
    void SetFrameHeight (int height);
    void SetPayloadType (int payloadType);
    void ApplyOptions ();

    int EncodeFrames (const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned int & flags);

  protected:
    CriticalSection _mutex;
};

class H264DecoderContext
{
  public:
    H264DecoderContext();
    ~H264DecoderContext();
    int DecodeFrames(const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned int & flags);

  protected:
    CriticalSection _mutex;

    AVCodec* _codec;
    AVCodecContext* _context;
    AVFrame* _outputFrame;
    H264Frame* _rxH264Frame;

    bool _gotIFrame;
    int _frameCounter;
    int _skippedFrameCounter;
};

static int get_xcif_options(void * context, void * parm, unsigned * parmLen, const char ** default_parms);
static int coder_get_cif_options ( const PluginCodec_Definition * , void * context,  const char * ,
                                   void * parm, unsigned * parmLen);
static int coder_get_qcif_options( const PluginCodec_Definition * , void * context, const char * ,
                                   void * parm, unsigned * parmLen);
static int coder_get_sip_options ( const PluginCodec_Definition * , void * context , const char * , 
                                   void * parm , unsigned * parmLen);

static void * create_encoder     ( const struct PluginCodec_Definition * /*codec*/);
static int encoder_set_options   ( const struct PluginCodec_Definition *, void * _context, const char *, 
                                   void * parm, unsigned * parmLen);
static void destroy_encoder      ( const struct PluginCodec_Definition * /*codec*/, void * _context);
static int codec_encoder         ( const struct PluginCodec_Definition * , void * _context,
                                   const void * from, unsigned * fromLen,
                                   void * to, unsigned * toLen,
                                   unsigned int * flag);
static int encoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *);

static void * create_decoder     ( const struct PluginCodec_Definition *);
static int decoder_set_options   ( const struct PluginCodec_Definition *, void * _context, const char *, 
                                   void * parm, unsigned * parmLen);
static void destroy_decoder      ( const struct PluginCodec_Definition * /*codec*/, void * _context);
static int codec_decoder         ( const struct PluginCodec_Definition *, void * _context, 
                                   const void * from, unsigned * fromLen,
                                   void * to, unsigned * toLen,
                                   unsigned int * flag);
static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *);


static int valid_for_sip ( const PluginCodec_Definition *, void * context , const char * , void * parm , unsigned * parmLen);
static int valid_for_h323 ( const PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen);


static const char * default_sip_options[][3] = {
  { "h323_cifMPI",                               "<4" ,      "i" },
  { "h323_qcifMPI",                              "<2" ,      "i" },
  { "fmtp",                                      ""   ,      "s" },
//  { "Max Bit Rate",                              "<621700" , "i" },
  { NULL, NULL, NULL }
};

static const char * default_cif_h264_options[][3] = {
  { "h323_cifMPI",                               "<4" ,      "i" },
//  { "Max Bit Rate",                              "<621700" , "i" },
  { NULL, NULL, NULL }
};

static const char * default_qcif_h264_options[][3] = {
  { "h323_qcifMPI",                              "<2" ,      "i" },
//  { "Max Bit Rate",                              "<621700" , "i" },
  { NULL, NULL, NULL }
};
/////////////////////////////////////////////////////////////////////////////


static struct PluginCodec_information licenseInfo = {
  1143692893,                                                   // timestamp = Thu 30 Mar 2006 04:28:13 AM UTC

  "Matthias Schneider",                                         // source code author
  "1.0",                                                        // source code version
  "ma30002000@yahoo.de",                                        // source code email
  "",                                                           // source code URL
  "Copyright (C) 2006 by Matthias Schneider",                   // source code copyright
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license

  "x264 / ffmpeg H.264",                                        // codec description
  "x264: Laurent Aimar, ffmpeg: Michael Niedermayer",           // codec author
  "", 							        // codec version
  "fenrir@via.ecp.fr, ffmpeg-devel-request@mplayerhq.hu",       // codec email
  "http://developers.videolan.org/x264.html, \
   http://ffmpeg.mplayerhq.hu", 				// codec URL
  "x264: Copyright (C) 2003 Laurent Aimar, \
   ffmpeg: Copyright (c) 2002-2003 Michael Niedermayer",        // codec copyright information
  "x264: GNU General Public License as published Version 2, \
   ffmpeg: GNU Lesser General Public License, Version 2.1",     // codec license
  PluginCodec_License_GPL                                       // codec license code
};

/////////////////////////////////////////////////////////////////////////////

static const char YUV420PDesc[]  = { "YUV420P" };

static const char h264QCIFDesc[]  = { "H.264-QCIF" };
static const char h264CIFDesc[]   = { "H.264-CIF" };
static const char h264Desc[]      = { "H.264" };

static const char sdpH264[]   = { "h264" };
#define H264_CLOCKRATE        90000
#define H264_BITRATE         768000
#define H264_PAYLOAD_SIZE      1400
#define H264_FRAME_RATE          25
#define H264_KEY_FRAME_INTERVAL 2.0
#define H264_TRACELEVEL 4
// in seconds
/////////////////////////////////////////////////////////////////////////////

static PluginCodec_ControlDefn cifEncoderControls[] = {
  { "valid_for_protocol",       valid_for_h323 },
  { "get_codec_options",    coder_get_cif_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn qcifEncoderControls[] = {
  { "valid_for_protocol",       valid_for_h323 },
  { "get_codec_options",    coder_get_qcif_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn sipEncoderControls[] = {
  { "valid_for_protocol",       valid_for_sip },
  { "get_codec_options",    coder_get_sip_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};


static PluginCodec_ControlDefn cifDecoderControls[] = {
  { "valid_for_protocol",       valid_for_h323 },
  { "get_codec_options",    coder_get_cif_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn qcifDecoderControls[] = {
  { "valid_for_protocol",       valid_for_h323 },
  { "get_codec_options",    coder_get_qcif_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn sipDecoderControls[] = {
  { "valid_for_protocol",       valid_for_sip },
  { "get_codec_options",    coder_get_sip_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition h264CodecDefn[6] = {

{ 
  // H.323 CIF encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |        // specified RTP type
  PluginCodec_RTPTypeDynamic,        // specified RTP type

  h264CIFDesc,                        // text decription
  YUV420PDesc,                        // source format
  h264CIFDesc,                        // destination format

  0,                                  // user data 

  H264_CLOCKRATE,                     // samples per second
  H264_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                // IANA RTP payload code
  sdpH264,                            // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  cifEncoderControls,                 // codec controls

  PluginCodec_H323VideoCodec_h261,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // H.323 CIF decoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |        // specified RTP type
  PluginCodec_RTPTypeDynamic,        // specified RTP type

  h264CIFDesc,                        // text decription
  h264CIFDesc,                        // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  H264_CLOCKRATE,                     // samples per second
  H264_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                // IANA RTP payload code
  sdpH264,                            // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  cifDecoderControls,                 // codec controls

  PluginCodec_H323VideoCodec_h261,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // H.323 QCIF encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |        // specified RTP type
  PluginCodec_RTPTypeDynamic,        // specified RTP type

  h264QCIFDesc,                       // text decription
  YUV420PDesc,                        // source format
  h264QCIFDesc,                       // destination format

  0,                                  // user data 

  H264_CLOCKRATE,                     // samples per second
  H264_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                // IANA RTP payload code
  sdpH264,                            // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  qcifEncoderControls,                // codec controls

  PluginCodec_H323VideoCodec_h261,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // H.323 QCIF decoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |        // specified RTP type
  PluginCodec_RTPTypeDynamic,        // specified RTP type

  h264QCIFDesc,                       // text decription
  h264QCIFDesc,                       // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  H264_CLOCKRATE,                     // samples per second
  H264_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                // IANA RTP payload code
  sdpH264,                            // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  qcifDecoderControls,                // codec controls

  PluginCodec_H323VideoCodec_h261,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // SIP encoder#define NUM_DEFNS   (sizeof(h264CodecDefn) / sizeof(struct PluginCodec_Definition))
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |        // specified RTP type
  PluginCodec_RTPTypeDynamic,        // specified RTP type

  h264Desc,                           // text decription
  YUV420PDesc,                        // source format
  h264Desc,                           // destination format

  0,                                  // user data 

  H264_CLOCKRATE,                     // samples per second
  H264_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                // IANA RTP payload code
  sdpH264,                            // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  sipEncoderControls,                 // codec controls

  PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // SIP decoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |        // specified RTP type
  PluginCodec_RTPTypeDynamic,        // specified RTP type

  h264Desc,                           // text decription
  h264Desc,                           // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  H264_CLOCKRATE,                     // samples per second
  H264_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                // IANA RTP payload code
  sdpH264,                            // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  sipDecoderControls,                 // codec controls

  PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
  NULL                                // h323CapabilityData
},
};
#define NUM_DEFNS   (sizeof(h264CodecDefn) / sizeof(struct PluginCodec_Definition))

#endif /* __H264-X264_H__ */
