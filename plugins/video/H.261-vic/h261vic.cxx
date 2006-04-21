/*
 * H.261 Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) 2005 Post Increment, All Rights Reserved
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
 * Contributor(s): Michele Piccini (michele@piccini.com)
 *                 Derek Smithies (derek@indranet.co.nz)
 *
 * $Log: h261vic.cxx,v $
 * Revision 1.1.2.5  2006/04/21 05:42:49  csoutheren
 * Checked in forgotten changes to fix iFrame requests
 *
 * Revision 1.1.2.4  2006/04/19 07:52:30  csoutheren
 * Add ability to have SIP-only and H.323-only codecs, and implement for H.261
 *
 * Revision 1.1.2.3  2006/04/19 05:56:23  csoutheren
 * Fix marker bits on outgoing video
 *
 * Revision 1.1.2.2  2006/04/19 04:59:29  csoutheren
 * Allow specification of CIF and QCIF capabilities
 *
 * Revision 1.1.2.1  2006/04/06 01:17:16  csoutheren
 * Initial version of H.261 video codec plugin for OPAL
 *
 */

#include <codec/opalplugin.h>

extern "C" {
PLUGIN_CODEC_IMPLEMENT(VIC_H261)
};

#include <stdlib.h>
#ifdef _WIN32
//#include <windows.h>
#include <malloc.h>
#endif
#include <string.h>

//#ifdef _MSC_VER
//#pragma warning(disable:4100)
//#endif

#define BEST_ENCODER_QUALITY   1
#define WORST_ENCODER_QUALITY 31

#define DEFAULT_FILL_LEVEL     5

#define H261_PAYLOAD    31

#define H261_CLOCKRATE    90000
#define H261_BITRATE      128000

typedef unsigned char u_char;
//typedef int BOOL;
typedef unsigned short u_short;
typedef unsigned int u_int;

#include "vic/p64.h"
#include "vic/p64encoder.h"

class CriticalSection
{
  public:
    CriticalSection()
    { 
#ifdef _WIN32
      ::InitializeCriticalSection(&criticalSection); 
#else
      ::sem_init(&sem, 0, 1);
#endif
    }

    ~CriticalSection()
    { 
#ifdef _WIN32
      ::DeleteCriticalSection(&criticalSection); 
#else
      ::sem_destroy(&sem);
#endif
    }

    void Wait()
    { 
#ifdef _WIN32
      ::EnterCriticalSection(&criticalSection); 
#else
      ::sem_wait(&sem);
#endif
    }

    void Signal()
    { 
#ifdef _WIN32
      ::LeaveCriticalSection(&criticalSection); 
#else
      ::sem_post(&sem); 
#endif
    }

  private:
    CriticalSection & operator=(const CriticalSection &) { return *this; }
#ifdef _WIN32
    mutable CRITICAL_SECTION criticalSection; 
#else
    mutable sem_t sem;
#endif
};
    
class WaitAndSignal {
  public:
    inline WaitAndSignal(const CriticalSection & cs)
      : sync((CriticalSection &)cs)
    { sync.Wait(); }

    ~WaitAndSignal()
    { sync.Signal(); }

    WaitAndSignal & operator=(const WaitAndSignal &) 
    { return *this; }

  protected:
    CriticalSection & sync;
};

/////////////////////////////////////////////////////////////////////////////
//
// define some simple RTP packet routines
//

#define RTP_MIN_HEADER_SIZE 12

class RTPFrame
{
  public:
    RTPFrame(const unsigned char * _packet, int _packetLen)
      : packet((unsigned char *)_packet), packetLen(_packetLen)
    {
    }

    RTPFrame(unsigned char * _packet, int _packetLen, unsigned char payloadType)
      : packet(_packet), packetLen(_packetLen)
    { 
      if (packetLen > 0)
        packet[0] = 0x80;    // set version, no extensions, zero contrib count
      SetPayloadType(payloadType);
    }

    inline unsigned long GetLong(int offs) const
    {
      if (offs + 4 > packetLen)
        return 0;
      return (packet[offs + 0] << 24) + (packet[offs+1] << 16) + (packet[offs+2] << 8) + packet[offs+3]; 
    }

    inline void SetLong(int offs, unsigned long n)
    {
      if (offs + 4 <= packetLen) {
        packet[offs + 0] = (BYTE)((n >> 24) & 0xff);
        packet[offs + 1] = (BYTE)((n >> 16) & 0xff);
        packet[offs + 2] = (BYTE)((n >> 8) & 0xff);
        packet[offs + 3] = (BYTE)(n & 0xff);
      }
    }

    inline unsigned short GetShort(int offs) const
    { 
      if (offs + 2 > packetLen)
        return 0;
      return (packet[offs + 0] << 8) + packet[offs + 1]; 
    }

    inline void SetShort(int offs, unsigned short n) 
    { 
      if (offs + 2 <= packetLen) {
        packet[offs + 0] = (BYTE)((n >> 8) & 0xff);
        packet[offs + 1] = (BYTE)(n & 0xff);
      }
    }

    inline int GetPacketLen() const                    { return packetLen; }
    inline unsigned GetVersion() const                 { return (packetLen < 1) ? 0 : (packet[0]>>6)&3; }
    inline bool GetExtension() const                   { return (packetLen < 1) ? 0 : (packet[0]&0x10) != 0; }
    inline bool GetMarker()  const                     { return (packetLen < 2) ? FALSE : ((packet[1]&0x80) != 0); }
    inline unsigned char GetPayloadType() const        { return (packetLen < 2) ? FALSE : (packet[1] & 0x7f);  }
    inline unsigned short GetSequenceNumber() const    { return GetShort(2); }
    inline unsigned long GetTimestamp() const          { return GetLong(4); }
    inline unsigned long GetSyncSource() const         { return GetLong(8); }
    inline int GetContribSrcCount() const              { return (packetLen < 1) ? 0  : (packet[0]&0xf); }
    inline int GetExtensionSize() const                { return !GetExtension() ? 0  : GetShort(RTP_MIN_HEADER_SIZE + 4*GetContribSrcCount() + 2); }
    inline int GetExtensionType() const                { return !GetExtension() ? -1 : GetShort(RTP_MIN_HEADER_SIZE + 4*GetContribSrcCount()); }
    inline int GetPayloadSize() const                  { return packetLen - GetHeaderSize(); }
    inline const unsigned char * GetPayloadPtr() const { return packet + GetHeaderSize(); }

    inline unsigned int GetHeaderSize() const    
    { 
      unsigned int sz = RTP_MIN_HEADER_SIZE + 4*GetContribSrcCount();
      if (GetExtension())
        sz += 4 + GetExtensionSize();
      return sz;
    }

    inline void SetMarker(bool m)                    { if (packetLen >= 2) packet[1] = (packet[1] & 0x7f) | (m ? 0x80 : 0x00); }
    inline void SetPayloadType(unsigned char t)      { if (packetLen >= 2) packet[1] = (packet[1] & 0x80) | (t & 0x7f); }
    inline void SetSequenceNumber(unsigned short v)  { SetShort(2, v); }
    inline void SetTimestamp(unsigned long n)        { SetLong(4, n); }
    inline void SetSyncSource(unsigned long n)       { SetLong(8, n); }
    inline void SetPayloadSize(int payloadSize)      { packetLen = GetHeaderSize() + payloadSize; }

  protected:
    unsigned char * packet;
    int packetLen;
};

/////////////////////////////////////////////////////////////////////////////

static const char * default_cif_h261_options[][3] = {
  { "h323_cifMPI",                               "<4" ,    "i" },
  { "h323_maxBitRate",                           "<6217" , "i" },
  { "h323_temporalSpatialTradeOffCapability",    "<f" ,    "b" },
  { "h323_stillImageTransmission",               "<f" ,    "b" },
  { NULL, NULL, NULL }
};

static const char * default_qcif_h261_options[][3] = {
  { "h323_qcifMPI",                              "<2" ,    "i" },
  { "h323_maxBitRate",                           "<6217" , "i" },
  { "h323_temporalSpatialTradeOffCapability",    "<f" ,    "b" },
  { "h323_stillImageTransmission",               "<f" ,    "b" },
  { NULL, NULL, NULL }
};

static int get_xcif_options(void * context, void * parm, unsigned * parmLen, const char ** default_parms)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char **))
    return 0;

  const char ***options = (const char ***)parm;

  if (context == NULL) {
    *options = default_parms;
    return 1;
  }

  return 0;
}

static int coder_get_cif_options(
      const PluginCodec_Definition * , 
      void * context, 
      const char * , 
      void * parm, 
      unsigned * parmLen)
{
  return get_xcif_options(context, parm, parmLen, &default_cif_h261_options[0][0]);
}

static int coder_get_qcif_options(
      const PluginCodec_Definition * , 
      void * context, 
      const char * , 
      void * parm, 
      unsigned * parmLen)
{
  return get_xcif_options(context, parm, parmLen, &default_qcif_h261_options[0][0]);
}

static int coder_get_sip_options(
      const PluginCodec_Definition * , 
      void * , 
      const char * , 
      void * , 
      unsigned * )
{
  return 1;
}


/////////////////////////////////////////////////////////////////////////////

class H261EncoderContext 
{
  public:
    P64Encoder * videoEncoder;
    //PTimeInterval newTime;
    unsigned frameWidth;
    unsigned frameHeight;
    unsigned maxOutputSize;
    bool forceIFrame;
    int videoQuality;
    CriticalSection updateMutex;
    unsigned long lastTimeStamp;

    H261EncoderContext()
    {
      frameWidth = frameHeight = 0;
      maxOutputSize = 0;
      videoEncoder = new P64Encoder(BEST_ENCODER_QUALITY, DEFAULT_FILL_LEVEL);
      forceIFrame = FALSE;
      videoQuality = 10;
    }

    ~H261EncoderContext()
    {
      delete videoEncoder;
    }

    void SetFrameSize(int w, int h)
    {
      frameWidth = w;
      frameHeight = h;
      videoEncoder->SetSize(frameWidth, frameHeight);
      maxOutputSize = (frameWidth * frameHeight * 12) / 8;
    }

    int EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
    {
      WaitAndSignal mutex(updateMutex);

      // create RTP frame from source buffer
      RTPFrame srcRTP(src, srcLen);

      // create RTP frame from destination buffer
      RTPFrame dstRTP(dst, dstLen, H261_PAYLOAD);

      // return more pending data frames, if any
      if (videoEncoder->MoreToIncEncode()) {
        unsigned payloadLength = 0;
        videoEncoder->IncEncodeAndGetPacket((u_char *)dstRTP.GetPayloadPtr(), payloadLength); //get next packet on list
        dstRTP.SetPayloadSize(payloadLength);
        dstRTP.SetMarker(FALSE);
        dstLen = dstRTP.GetPacketLen();

        flags = 0;
        flags |= videoEncoder->MoreToIncEncode() ? 0 : PluginCodec_ReturnCoderLastFrame;  // marker bit on last frame of video
        flags |= PluginCodec_ReturnCoderIFrame;                                           // sadly, this encoder *always* returns I-frames :(

        dstRTP.SetMarker(!videoEncoder->MoreToIncEncode());
        dstRTP.SetTimestamp(lastTimeStamp);
        return 1;
      }

      //
      // from here on, this is encoding a new frame
      //

      // get the timestamp we will be using for the rest of the frame
      lastTimeStamp = srcRTP.GetTimestamp();

      // update the video quality
      videoEncoder->SetQualityLevel(videoQuality);

      // get and validate header
      if (srcRTP.GetPayloadSize() < sizeof(PluginCodec_Video_FrameHeader)) {
        //PTRACE(1,"H261\tVideo grab too small");
        dstLen = 0;
        return 0;
      } 
      PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)srcRTP.GetPayloadPtr();
      if (header->x != 0 && header->y != 0) {
        //PTRACE(1,"H261\tVideo grab of partial frame unsupported");
        dstLen = 0;
        return 0;
      }

      // make sure the incoming frame is big enough for the specified frame size
      if (srcRTP.GetPayloadSize() < (int)(sizeof(PluginCodec_Video_FrameHeader) + frameWidth*frameHeight*12/8)) {
        //PTRACE(1,"H261\tPayload of grabbed frame too small for full frame");
        dstLen = 0;
        return 0;
      }

      // if the incoming data has changed size, tell the encoder
      if (frameWidth != header->width || frameHeight != header->height) {
        frameWidth = header->width;
        frameHeight = header->height;
        videoEncoder->SetSize(frameWidth, frameHeight);
      }

      // "grab" the frame
      memcpy(videoEncoder->GetFramePtr(), header->data, frameWidth*frameHeight*12/8);

      // force I-frame, if requested
      if (forceIFrame || (flags & PluginCodec_CoderForceIFrame) != 0) {
        videoEncoder->FastUpdatePicture();
        forceIFrame = FALSE;
      }

      // preprocess the data
      videoEncoder->PreProcessOneFrame();

      // get next frame from list created by preprocessor
      if (!videoEncoder->MoreToIncEncode())
        dstRTP.SetPayloadSize(0);
      else {
        unsigned payloadLength = 0;
        videoEncoder->IncEncodeAndGetPacket((u_char *)dstRTP.GetPayloadPtr(), payloadLength); 
        dstRTP.SetPayloadSize(payloadLength);
        dstLen = dstRTP.GetPacketLen();
      }

      
      flags = 0;
      flags |= videoEncoder->MoreToIncEncode() ? 0 : PluginCodec_ReturnCoderLastFrame;  // marker flag set on last frame of video
      flags |= PluginCodec_ReturnCoderIFrame;                                           // sadly, this encoder *always* returns I-frames :(

      dstRTP.SetMarker(!videoEncoder->MoreToIncEncode());
      dstRTP.SetTimestamp(lastTimeStamp);
     
      return 1;
    }
};

static void * create_encoder(const struct PluginCodec_Definition * /*codec*/)
{
  return new H261EncoderContext;
}

static int encoder_set_options(
      const PluginCodec_Definition * , 
      void *, 
      const char * , 
      void *, 
      unsigned * )
{
  //H261EncoderContext * context = (H261EncoderContext *)_context;
  return 1;
}

static void destroy_encoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H261EncoderContext * context = (H261EncoderContext *)_context;
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
  H261EncoderContext * context = (H261EncoderContext *)_context;
  return context->EncodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag);
}

static int encoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *)
{
  // this is really frame height * frame width;
  return codec->samplesPerFrame * codec->bytesPerFrame * 3 / 2;
}

static PluginCodec_ControlDefn cifEncoderControls[] = {
  { "get_codec_options",    coder_get_cif_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn qcifEncoderControls[] = {
  { "get_codec_options",    coder_get_qcif_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn sipEncoderControls[] = {
  { "get_codec_options",    coder_get_sip_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

/////////////////////////////////////////////////////////////////////////////

class H261DecoderContext
{
  public:
    P64Decoder * videoDecoder;
    WORD expectedSequenceNumber;
    BYTE * rvts;
    int ndblk, nblk;
    int now;
    BOOL packetReceived;
    unsigned frameWidth;
    unsigned frameHeight;

    CriticalSection mutex;

  public:
    H261DecoderContext()
    {
      expectedSequenceNumber = 0;
      nblk = ndblk = 0;
      rvts = NULL;
      now = 1;
      packetReceived = FALSE;
      frameWidth = frameHeight = 0;

      // Create the actual decoder
      videoDecoder = new FullP64Decoder();
      videoDecoder->marks(rvts);
    }

    ~H261DecoderContext()
    {
      if (rvts)
        delete rvts;
      delete videoDecoder;
    }

    int DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
    {
      WaitAndSignal mutex(mutex);

      // create RTP frame from source buffer
      RTPFrame srcRTP(src, srcLen);

      // create RTP frame from destination buffer
      RTPFrame dstRTP(dst, dstLen, 0);

      // Check for lost packets to help decoder
      bool lostPreviousPacket = FALSE;
      if (expectedSequenceNumber != 0 && expectedSequenceNumber != srcRTP.GetSequenceNumber()) {
        lostPreviousPacket = TRUE;
        //PTRACE(3,"H261\tDetected loss of one video packet. "
        //      << expectedSequenceNumber << " != "
        //      << src.GetSequenceNumber() << " Will recover.");
      }
      expectedSequenceNumber = (WORD)(srcRTP.GetSequenceNumber()+1);

      videoDecoder->mark(now);
      if (!videoDecoder->decode(srcRTP.GetPayloadPtr(), srcRTP.GetPayloadSize(), lostPreviousPacket)) {
        dstLen = 0;
        flags = PluginCodec_ReturnCoderRequestIFrame;
        return 1;
      }

      //Check for a resize - can change at any time!
      if (frameWidth  != (unsigned)videoDecoder->width() ||
          frameHeight != (unsigned)videoDecoder->height()) {

        frameWidth = videoDecoder->width();
        frameHeight = videoDecoder->height();

        nblk = (frameWidth * frameHeight) / 64;
        delete [] rvts;
        rvts = new BYTE[nblk];
        memset(rvts, 0, nblk);
        videoDecoder->marks(rvts);
      }

      // Have not built an entire frame yet
      if (!srcRTP.GetMarker()) {
        dstLen = 0;
        flags = 0;
        return TRUE;
      }

      videoDecoder->sync();
      ndblk = videoDecoder->ndblk();

      int wraptime = now ^ 0x80;
      BYTE * ts = rvts;
      int k;
      for (k = nblk; --k >= 0; ++ts) {
        if (*ts == wraptime)
          *ts = (BYTE)now;
      }

      now = (now + 1) & 0xff;

      int frameBytes = (frameWidth * frameHeight * 12) / 8;
      dstRTP.SetPayloadSize(sizeof(PluginCodec_Video_FrameHeader) + frameBytes);
      dstRTP.SetMarker(true);

      PluginCodec_Video_FrameHeader * frameHeader = (PluginCodec_Video_FrameHeader *)dstRTP.GetPayloadPtr();
      frameHeader->x = frameHeader->y = 0;
      frameHeader->width = frameWidth;
      frameHeader->height = frameHeight;
      memcpy(frameHeader->data, videoDecoder->GetFramePtr(), frameBytes);

      videoDecoder->resetndblk();

      dstLen = dstRTP.GetPacketLen();
      flags = PluginCodec_ReturnCoderLastFrame | PluginCodec_ReturnCoderIFrame;   // THIS NEEDS TO BE CHANGED TO DO CORRECT I-FRAME DETECTION
      return TRUE;
    }
};

static void * create_decoder(const struct PluginCodec_Definition *)
{
  return new H261DecoderContext;
}

static int decoder_set_options(
      const struct PluginCodec_Definition *, 
      void * _context, 
      const char *, 
      void * parm, 
      unsigned * parmLen)
{
  H261DecoderContext * context = (H261DecoderContext *)_context;

  if (parmLen == NULL || *parmLen != sizeof(const char **))
    return 0;

  // get the "frame width" media format parameter to use as a hint for the encoder to start off
  if (parm != NULL) {
    const char ** options = (const char **)parm;
    int i;
    int frameWidth = 0;
    for (i = 0; options[i] != NULL; i += 2) {
      if (strcmpi(options[i], "Frame Width") == 0)
        frameWidth = atoi(options[i+1]);
    }
    context->videoDecoder->fmt_ = (frameWidth == QCIF_WIDTH) ? IT_QCIF : IT_CIF;
    context->videoDecoder->init();
  }

  return 1;
}

static void destroy_decoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H261DecoderContext * context = (H261DecoderContext *)_context;
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
  H261DecoderContext * context = (H261DecoderContext *)_context;
  return context->DecodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag);
}

static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *)
{
  // this is really frame height * frame width;
  return sizeof(PluginCodec_Video_FrameHeader) + ((codec->samplesPerFrame * codec->bytesPerFrame * 3) / 2);
}

static PluginCodec_ControlDefn cifDecoderControls[] = {
  { "get_codec_options",    coder_get_cif_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn qcifDecoderControls[] = {
  { "get_codec_options",    coder_get_qcif_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn sipDecoderControls[] = {
  { "get_codec_options",    coder_get_sip_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

/////////////////////////////////////////////////////////////////////////////


static struct PluginCodec_information licenseInfo = {
  1143692893,                                                   // timestamp = Thu 30 Mar 2006 04:28:13 AM UTC

  "Craig Southeren, Post Increment",                            // source code author
  "1.0",                                                        // source code version
  "craigs@postincrement.com",                                   // source code email
  "http://www.postincrement.com",                               // source code URL
  "Copyright (C) 2004 by Post Increment, All Rights Reserved",  // source code copyright
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  "VIC H.261",                                                   // codec description
  "",                                                            // codec author
  "",                                                            // codec version
  "",                                                            // codec email
  "",                                                            // codec URL
  "Copyright (c) 1994 Regents of the University of California",  // codec copyright information
  NULL,                                                          // codec license
  PluginCodec_License_BSD                                        // codec license code
};

/////////////////////////////////////////////////////////////////////////////

static const char YUV420PDesc[]  = { "YUV420P" };

static const char h261QCIFDesc[]  = { "H.261-QCIF" };
static const char h261CIFDesc[]   = { "H.261-CIF" };
static const char h261Desc[]      = { "H.261" };

static const char sdpH261[]   = { "h261" };

/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition h261CodecDefn[6] = {

{ 
  // H.323 CIF encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h261CIFDesc,                        // text decription
  YUV420PDesc,                        // source format
  h261CIFDesc,                        // destination format

  0,                                  // user data 

  H261_CLOCKRATE,                     // samples per second
  H261_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  H261_PAYLOAD,                       // IANA RTP payload code
  NULL,                               // RTP payload name

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
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h261CIFDesc,                        // text decription
  h261CIFDesc,                        // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  H261_CLOCKRATE,                     // samples per second
  H261_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  H261_PAYLOAD,                       // IANA RTP payload code
  NULL,                               // RTP payload name

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
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h261QCIFDesc,                       // text decription
  YUV420PDesc,                        // source format
  h261QCIFDesc,                       // destination format

  0,                                  // user data 

  H261_CLOCKRATE,                     // samples per second
  H261_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  H261_PAYLOAD,                       // IANA RTP payload code
  NULL,                               // RTP payload name

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
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h261QCIFDesc,                       // text decription
  h261QCIFDesc,                       // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  H261_CLOCKRATE,                     // samples per second
  H261_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  H261_PAYLOAD,                       // IANA RTP payload code
  NULL,                               // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  qcifDecoderControls,                // codec controls

  PluginCodec_H323VideoCodec_h261,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // SIP encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h261Desc,                           // text decription
  YUV420PDesc,                        // source format
  h261Desc,                           // destination format

  0,                                  // user data 

  H261_CLOCKRATE,                     // samples per second
  H261_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  H261_PAYLOAD,                       // IANA RTP payload code
  sdpH261,                            // RTP payload name

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
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h261Desc,                           // text decription
  h261Desc,                           // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  H261_CLOCKRATE,                     // samples per second
  H261_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  H261_PAYLOAD,                       // IANA RTP payload code
  sdpH261,                            // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  sipDecoderControls,                 // codec controls

  PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
  NULL                                // h323CapabilityData
},

};

#define NUM_DEFNS   (sizeof(h261CodecDefn) / sizeof(struct PluginCodec_Definition))

/////////////////////////////////////////////////////////////////////////////

extern "C" {
PLUGIN_CODEC_DLL_API struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
{
  if (version < PLUGIN_CODEC_VERSION_VIDEO) {
    *count = 0;
    return NULL;
  }
else {
    *count = NUM_DEFNS;
    return h261CodecDefn;
  }
}
};
