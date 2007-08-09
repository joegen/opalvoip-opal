/*
 * THEORA Plugin codec for OpenH323/OPAL
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

#include "theora_plugin.h"

#include "trace.h"
#include "rtpframe.h"

#include <stdlib.h>
#ifdef _WIN32
  #include <malloc.h>
  #define STRCMPI  _strcmpi
#else
  #include <semaphore.h>
  #define STRCMPI  strcasecmp
#endif
#include <string.h>

theoraEncoderContext::theoraEncoderContext()
{
  ogg_packet headerPacket, tablePacket;
  _frameCounter=0;
  
  _txTheoraFrame = new theoraFrame();
  _txTheoraFrame->SetMaxPayloadSize(THEORA_PAYLOAD_SIZE);

  theora_info_init( &_theoraInfo );
  _theoraInfo.frame_width        = 352;  // Must be multiple of 16
  _theoraInfo.frame_height       = 288; // Must be multiple of 16
  _theoraInfo.width              = _theoraInfo.frame_width;
  _theoraInfo.height             = _theoraInfo.frame_height;
  _theoraInfo.offset_x           = 0;
  _theoraInfo.offset_y           = 0;
  _theoraInfo.fps_numerator      = THEORA_FRAME_RATE;
  _theoraInfo.fps_denominator    = 1;
  _theoraInfo.aspect_numerator   = _theoraInfo.width;  // Aspect =  width/height
  _theoraInfo.aspect_denominator = _theoraInfo.height; //
  _theoraInfo.colorspace         = OC_CS_UNSPECIFIED;
  _theoraInfo.target_bitrate     = THEORA_BITRATE; // Anywhere between 45kbps and 2000kbps
  _theoraInfo.quality            = 16; 
  _theoraInfo.dropframes_p                 = 0;
  _theoraInfo.quick_p                      = 1;
  _theoraInfo.keyframe_auto_p              = 1;
  _theoraInfo.keyframe_frequency = (int)(THEORA_FRAME_RATE * THEORA_KEY_FRAME_INTERVAL);
  _theoraInfo.keyframe_frequency           = 64;
  _theoraInfo.keyframe_frequency_force     = _theoraInfo.keyframe_frequency;
  _theoraInfo.keyframe_data_target_bitrate = _theoraInfo.target_bitrate * 3 / 2;
  _theoraInfo.keyframe_auto_threshold      = 80;
  _theoraInfo.keyframe_mindistance         = 8;
  _theoraInfo.noise_sensitivity            = 1;

  theora_encode_init( &_theoraState, &_theoraInfo );

  theora_encode_header( &_theoraState, &headerPacket );
  _txTheoraFrame->SetFromHeaderConfig(&headerPacket);

  theora_encode_tables( &_theoraState, &tablePacket );
  _txTheoraFrame->SetFromTableConfig(&tablePacket);
}

theoraEncoderContext::~theoraEncoderContext()
{
  theora_clear( &_theoraState );
  theora_info_clear (&_theoraInfo);
  if (_txTheoraFrame) delete _txTheoraFrame;
}

void theoraEncoderContext::SetTargetBitrate(int rate)
{
  _theoraInfo.target_bitrate     = rate * 2 / 3; // Anywhere between 45kbps and 2000kbps}
  _theoraInfo.keyframe_data_target_bitrate = rate;
}

void theoraEncoderContext::SetFrameRate(int rate)
{
  _theoraInfo.keyframe_frequency = (int)(rate * THEORA_KEY_FRAME_INTERVAL);
  _theoraInfo.fps_numerator      = (int)((rate + .5) * 1000); // ???
  _theoraInfo.fps_denominator    = 1000;
}

void theoraEncoderContext::SetFrameWidth(int width)
{
  _theoraInfo.frame_width        = width;  // Must be multiple of 16
  _theoraInfo.width              = _theoraInfo.frame_width;
}

void theoraEncoderContext::SetFrameHeight(int height)
{
  _theoraInfo.frame_height       = height; // Must be multiple of 16
  _theoraInfo.height             = _theoraInfo.frame_height;
}

void theoraEncoderContext::ApplyOptions()
{
  ogg_packet headerPacket, tablePacket;

  theora_clear( &_theoraState );
  theora_encode_init( &_theoraState, &_theoraInfo );

  theora_encode_header( &_theoraState, &headerPacket );
  _txTheoraFrame->SetFromHeaderConfig(&headerPacket);

  theora_encode_tables( &_theoraState, &tablePacket );
  _txTheoraFrame->SetFromTableConfig(&tablePacket);
}

int theoraEncoderContext::EncodeFrames(const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned int & flags)
{
  int ret;
  yuv_buffer yuv;
  ogg_packet framePacket;

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen);

  dstLen = 0;

  // from here, we are encoding a new frame
  if (!_txTheoraFrame)
  {
    return 0;
  }

  // if there are RTP packets to return, return them
  if  (_txTheoraFrame->HasRTPFrames())
  {
    _txTheoraFrame->GetRTPFrame(dstRTP, flags);
    dstLen = dstRTP.GetFrameLen();
    return 1;
  }

  if (srcRTP.GetPayloadSize() < sizeof(frameHeader))
  {
   TRACE(1, "THEORA\tEncoder\tVideo grab too small, Close down video transmission thread");
   return 0;
  }

  frameHeader * header = (frameHeader *)srcRTP.GetPayloadPtr();
  if (header->x != 0 || header->y != 0)
  {
    TRACE(1, "THEORA\tEncoder\tVideo grab of partial frame unsupported, Close down video transmission thread");
    return 0;
  }

  // do a validation of size
  // if the incoming data has changed size, tell the encoder
  if (_theoraInfo.frame_width != header->width || _theoraInfo.frame_height != header->height)
  {
    _theoraInfo.frame_width        = header->width;  // Must be multiple of 16
    _theoraInfo.frame_height       = header->height; // Must be multiple of 16
    _theoraInfo.width              = _theoraInfo.frame_width;
    _theoraInfo.height             = _theoraInfo.frame_height;
    _theoraInfo.aspect_numerator   = _theoraInfo.width;  // Aspect =  width/height
    _theoraInfo.aspect_denominator = _theoraInfo.height; //
    ApplyOptions();
  } 

  // Prepare the frame to be encoded
  yuv.y_width   = header->width;
  yuv.y_height  = _theoraInfo.height;
  yuv.uv_width  = (int) (header->width / 2);
  yuv.uv_height = (int) (_theoraInfo.height / 2);
  yuv.y_stride  = header->width;
  yuv.uv_stride = (int) (header->width /2);
  yuv.y         = (uint8_t *)(((unsigned char *)header) + sizeof(frameHeader));
  yuv.u         = (uint8_t *)((((unsigned char *)header) + sizeof(frameHeader)) 
                           + (int)(yuv.y_stride*header->height)); 
  yuv.v         = (uint8_t *)(yuv.u + (int)(yuv.uv_stride *header->height/2)); 

  ret = theora_encode_YUVin( &_theoraState, &yuv );
  if (ret != 0) {
    if (ret == -1) 
      TRACE(1, "THEORA\tEncoder\tEncoding failed: The size of the given frame differs from those previously input (should not happen)")
     else
      TRACE(1, "THEORA\tEncoder\tEncoding failed: " << theoraErrorMessage(ret));
    return 0;
  }

  ret = theora_encode_packetout( &_theoraState, 0 /* not last Packet */, &framePacket );
  switch (ret) {
    case  0: TRACE(1, "THEORA\tEncoder\tEncoding failed (packet): No internal storage exists OR no packet is ready"); return 0; break;
    case -1: TRACE(1, "THEORA\tEncoder\tEncoding failed (packet): The encoding process has completed but something is not ready yet"); return 0; break;
    case  1: TRACE(4, "THEORA\tEncoder\tSuccessfully encoded OGG packet of " << framePacket.bytes << " bytes"); break;
    default: TRACE(1, "THEORA\tEncoder\tEncoding failed (packet): " << theoraErrorMessage(ret)); return 0; break;
  }

  _txTheoraFrame->SetFromFrame(&framePacket);
  _txTheoraFrame->SetTimestamp(srcRTP.GetTimestamp());
  _frameCounter++; 

  if (_txTheoraFrame->HasRTPFrames())
  {
    _txTheoraFrame->GetRTPFrame(dstRTP, flags);
    dstLen = dstRTP.GetFrameLen();
    return 1;
  }
}

void theoraEncoderContext::SetMaxRTPFrameSize (int size)
{
  _txTheoraFrame->SetMaxPayloadSize(size);
}

theoraDecoderContext::theoraDecoderContext()
{
  _frameCounter = 0; 
  _gotHeader = false;
  _gotTable  = false;
  _gotIFrame = false;

  _rxTheoraFrame = new theoraFrame();
  theora_info_init( &_theoraInfo );
}

theoraDecoderContext::~theoraDecoderContext()
{
  if (_gotHeader && _gotTable) theora_clear( &_theoraState );
  theora_info_clear( &_theoraInfo );
  if (_rxTheoraFrame) delete _rxTheoraFrame;
}

int theoraDecoderContext::DecodeFrames(const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned int & flags)
{
  WaitAndSignal m(_mutex);

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen, 0);
  dstLen = 0;
  flags = 0;
  _rxTheoraFrame->SetFromRTPFrame(srcRTP, flags);

// linphone does not send markers so for now we ignore them (should be fixed)
//   if (srcRTP.GetMarker()==0)
//   {
//      return 1;
//   } 

  yuv_buffer yuv;
  ogg_packet oggPacket;
  theora_comment theoraComment;
  bool gotFrame = false;
  int ret;

  while (_rxTheoraFrame->HasOggPackets()) {
    _rxTheoraFrame->GetOggPacket (&oggPacket);
    if (theora_packet_isheader(&oggPacket)) {

      TRACE(4, "THEORA\tDecoder\tGot OGG header packet with size " << oggPacket.bytes);

      // In case we receive new header packets when the stream is already established
      // we have to wait to get both table and header packet until reopening the decoder
      if (_gotHeader && _gotTable) {
        TRACE(4, "THEORA\tDecoder\tGot OGG header packet after stream was established");
        theora_clear( &_theoraState );
        theora_info_clear( &_theoraInfo );
        theora_info_init( &_theoraInfo );
        _gotHeader = false;
        _gotTable = false; 
        _gotIFrame = false;
      }

      theora_comment_init(&theoraComment);
      theoraComment.vendor = "theora";
      ret = theora_decode_header( &_theoraInfo, &theoraComment, &oggPacket );
      if (ret != 0) {
        TRACE(1, "THEORA\tDecoder\tDecoding failed (header packet): " << theoraErrorMessage(ret));
        return 0;
      }

      if (oggPacket.bytes == THEORA_HEADER_PACKET_SIZE) 
        _gotHeader = true; 
       else 
        _gotTable = true;

      if (_gotHeader && _gotTable) theora_decode_init( &_theoraState, &_theoraInfo );
    } 
    else {

      if (_gotHeader && _gotTable) {

        if (theora_packet_iskeyframe(&oggPacket)) {

          TRACE(4, "THEORA\tDecoder\tGot OGG keyframe data packet with size " << oggPacket.bytes);
          ret = theora_decode_packetin( &_theoraState, &oggPacket );
          if (ret != 0) {
            TRACE(1, "THEORA\tDecoder\tDecoding failed (packet): " << theoraErrorMessage(ret));
            return 0;
          }
          theora_decode_YUVout( &_theoraState, &yuv);
          _gotIFrame = true;
          gotFrame = true;
        } 
        else {

          if  (_gotIFrame) {

            TRACE(4, "THEORA\tDecoder\tGot OGG non-keyframe data packet with size " << oggPacket.bytes);
            ret = theora_decode_packetin( &_theoraState, &oggPacket );
            if (ret != 0) {
              TRACE(1, "THEORA\tDecoder\tDecoding failed (packet): " << theoraErrorMessage(ret));
              return 0;
            }
            theora_decode_YUVout( &_theoraState, &yuv);
            gotFrame = true;
          } 
          else {

            TRACE(1, "THEORA\tDecoder\tGot OGG non-keyframe data Packet but still waiting for keyframe");
            return 0;
          }
        }
      } 
      else {

        TRACE(1, "THEORA\tDecoder\tGot OGG data packet but still waiting for header and/or table Packets");
        return 0;
      }
    }
  }

  TRACE(4, "THEORA\tDecoder\tNo more OGG packets to decode");

  if (gotFrame) {

    int size = _theoraInfo.width * _theoraInfo.height;
    int frameBytes = (int) (size * 3 / 2);
    PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)dstRTP.GetPayloadPtr();

    TRACE(4, "THEORA\tDecoder\tDecoded Frame with resolution: " << _theoraInfo.width << "x" << _theoraInfo.height);

    header->x = header->y = 0;
    header->width = _theoraInfo.width;
    header->height = _theoraInfo.height;

    unsigned int i = 0;
    int width2 = (header->width >> 1);

    uint8_t* dstY = (uint8_t*) OPAL_VIDEO_FRAME_DATA_PTR(header);
    uint8_t* dstU = (uint8_t*) dstY + size;
    uint8_t* dstV = (uint8_t*) dstU + (size >> 2);

    uint8_t* srcY = yuv.y;
    uint8_t* srcU = yuv.u;
    uint8_t* srcV = yuv.v;

    for (i = 0 ; i < header->height ; i+=2) {

      memcpy (dstY, srcY, header->width); 
      srcY +=yuv.y_stride; 
      dstY +=header->width;

      memcpy (dstY, srcY, header->width); 
      srcY +=yuv.y_stride; 
      dstY +=header->width;

      memcpy(dstU, srcU, width2); 
      srcU+=yuv.uv_stride;
      dstU+=width2; 

      memcpy (dstV, srcV, width2); 
      srcV += yuv.uv_stride;
      dstV +=width2; 
    }

    dstRTP.SetPayloadSize(sizeof(PluginCodec_Video_FrameHeader) + frameBytes);
    dstRTP.SetTimestamp(srcRTP.GetTimestamp());
    dstRTP.SetMarker(1);
    dstLen = dstRTP.GetFrameLen();
    flags = PluginCodec_ReturnCoderLastFrame;
    _frameCounter++;
    return 1;
  } 
  else { /*gotFrame */

    TRACE(4, "THEORA\tDecoder\tDid not get a decoded frame");
    return 0;
  }
}

/////////////////////////////////////////////////////////////////////////////

const char*
theoraErrorMessage(int code)
{
  static char *error;
  static char errormsg [1024];	

  switch (code) {
    case OC_FAULT:
      error = "General failure";
      break;
    case OC_EINVAL:
      error = "Library encountered invalid internal data";
      break;
    case OC_DISABLED:
      error = "Requested action is disabled";
      break;
    case OC_BADHEADER:
      error = "Header packet was corrupt/invalid";
      break;
    case OC_NOTFORMAT:
      error = "Packet is not a theora packet";
      break;
    case OC_VERSION:
      error = "Bitstream version is not handled";
      break;
    case OC_IMPL:
      error = "Feature or action not implemented";
      break;
    case OC_BADPACKET:
      error = "Packet is corrupt";
      break;
    case OC_NEWPACKET:
      error = "Packet is an (ignorable) unhandled extension";
      break;
    case OC_DUPFRAME:
      error = "Packet is a dropped frame";
      break;
    default:
      snprintf (errormsg, sizeof (errormsg), "%u", code);
      return (errormsg);
  }
  snprintf (errormsg, sizeof(errormsg), "%s (%u)", error, code);
  return (errormsg);
}


/////////////////////////////////////////////////////////////////////////////

static void * create_encoder(const struct PluginCodec_Definition * /*codec*/)
{
  return new theoraEncoderContext;
}

static int encoder_set_options(
      const struct PluginCodec_Definition *, 
      void * _context, 
      const char *, 
      void * parm, 
      unsigned * parmLen)
{
  theoraEncoderContext * context = (theoraEncoderContext *)_context;

  if (parmLen == NULL || *parmLen != sizeof(const char **)) return 0;

  if (parm != NULL) {
    const char ** options = (const char **)parm;
    int i;
    for (i = 0; options[i] != NULL; i += 2) {
      if (STRCMPI(options[i], "Target Bit Rate") == 0)
         context->SetTargetBitrate(atoi(options[i+1]));
      if (STRCMPI(options[i], "Frame Time") == 0)
         context->SetFrameRate((int)(THEORA_CLOCKRATE / atoi(options[i+1])));
      if (STRCMPI(options[i], "Frame Height") == 0)
         context->SetFrameHeight(atoi(options[i+1]));
      if (STRCMPI(options[i], "Frame Width") == 0)
         context->SetFrameWidth(atoi(options[i+1]));
      if (STRCMPI(options[i], "Max Frame Size") == 0)
	 context->SetMaxRTPFrameSize (atoi(options[i+1]));
      TRACE (4, "THEORA\tEncoder\tOption " << options[i] << " = " << atoi(options[i+1]));
    }
    context->ApplyOptions();

  }
  return 1;
}

static void destroy_encoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  theoraEncoderContext * context = (theoraEncoderContext *)_context;
  delete context;
}

static int codec_encoder(const struct PluginCodec_Definition * ,
                                           void * _context,
                                     const void * from,
                                       unsigned * fromLen,
                                           void * to,
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  theoraEncoderContext * context = (theoraEncoderContext *)_context;
  return context->EncodeFrames((const u_char *)from, *fromLen, (u_char *)to, *toLen, *flag);
}

/////////////////////////////////////////////////////////////////////////////


static void * create_decoder(const struct PluginCodec_Definition *)
{
  return new theoraDecoderContext;
}

static int get_codec_options(const struct PluginCodec_Definition * codec,
                                                  void *,
                                                  const char *,
                                                  void * parm,
                                                  unsigned * parmLen)
{
    if (parmLen == NULL || parm == NULL || *parmLen != sizeof(struct PluginCodec_Option **))
        return 0;

    *(const void **)parm = codec->userData;
    return 1;
}

static void destroy_decoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  theoraDecoderContext * context = (theoraDecoderContext *)_context;
  delete context;
}

static int codec_decoder(const struct PluginCodec_Definition *, 
                                           void * _context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  theoraDecoderContext * context = (theoraDecoderContext *)_context;
  return context->DecodeFrames((const u_char *)from, *fromLen, (u_char *)to, *toLen, *flag);
}

static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *)
{
  return sizeof(PluginCodec_Video_FrameHeader) + ((codec->parm.video.maxFrameWidth * codec->parm.video.maxFrameHeight * 3) / 2);
}

/////////////////////////////////////////////////////////////////////////////

extern "C" {

PLUGIN_CODEC_IMPLEMENT(THEORA)

PLUGIN_CODEC_DLL_API struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
{
  char * debug_level = getenv ("PWLIB_TRACE_CODECS");
  if (debug_level!=NULL) {
    Trace::SetLevel(atoi(debug_level));
  } 
  else {
    Trace::SetLevel(0);
  }

  if (version < PLUGIN_CODEC_VERSION_VIDEO) {
    *count = 0;
    return NULL;
  }
  else {
    *count = sizeof(theoraCodecDefn) / sizeof(struct PluginCodec_Definition);
    return theoraCodecDefn;
  }
  
}
};
