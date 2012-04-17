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

#include "../common/ffmpeg.h"
#include "critsect.h"


typedef unsigned char BYTE;

#define RTP_RFC2190_PAYLOAD       34

#define VIDEO_CLOCKRATE        90000
#define H263_BITRATE          327600
#define H263_KEY_FRAME_INTERVAL  125

#define CIF_WIDTH       352
#define CIF_HEIGHT      288

#define CIF4_WIDTH      (CIF_WIDTH*2)
#define CIF4_HEIGHT     (CIF_HEIGHT*2)

#define CIF16_WIDTH     (CIF_WIDTH*4)
#define CIF16_HEIGHT    (CIF_HEIGHT*4)

#define QCIF_WIDTH     (CIF_WIDTH/2)
#define QCIF_HEIGHT    (CIF_HEIGHT/2)

#define QCIF4_WIDTH     (CIF4_WIDTH/2)
#define QCIF4_HEIGHT    (CIF4_HEIGHT/2)

#define SQCIF_WIDTH     128
#define SQCIF_HEIGHT    96


class PluginCodec_RTP;


////////////////////////////////////////////////////////////////////////////

class Packetizer
{
  public:
    Packetizer();
    virtual ~Packetizer() { }

    virtual bool Reset(unsigned width, unsigned height) = 0;
    virtual bool GetPacket(PluginCodec_RTP & frame, unsigned int & flags) = 0;
    virtual unsigned char * GetBuffer() = 0;
    virtual size_t GetMaxSize() = 0;
    virtual bool SetLength(size_t len) = 0;

    void SetMaxPayloadSize (uint16_t maxPayloadSize) 
    {
      m_maxPayloadSize = maxPayloadSize;
    }

  protected:
    uint16_t m_maxPayloadSize;
};

////////////////////////////////////////////////////////////////////////////

class H263_Base_EncoderContext
{
  protected:
    H263_Base_EncoderContext(const char * prefix, Packetizer * packetizer);
  public:
    virtual ~H263_Base_EncoderContext();

    virtual bool Init() = 0;
    virtual bool Init(CodecID codecId);

    virtual bool SetOptions(const char * const * options);
    virtual void SetOption(const char * option, const char * value);

    virtual bool EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags);

    bool OpenCodec();
    void CloseCodec();

    void Lock();
    void Unlock();

    bool GetStatistics(char * stats, size_t maxSize);

  protected:
    const char     * m_prefix;
    AVCodec        * m_codec;
    AVCodecContext * m_context;
    AVFrame        * m_inputFrame;
    uint8_t        * m_alignedInputYUV;
    Packetizer     * m_packetizer;
    CriticalSection  m_mutex;
};

////////////////////////////////////////////////////////////////////////////

class H263_RFC2190_EncoderContext : public H263_Base_EncoderContext
{
  public:
    H263_RFC2190_EncoderContext();
    bool Init();
    static void RTPCallBack(AVCodecContext *avctx, void * data, int size, int mb_nb);
};

////////////////////////////////////////////////////////////////////////////

class H263_RFC2429_EncoderContext : public H263_Base_EncoderContext
{
  public:
    H263_RFC2429_EncoderContext();
    ~H263_RFC2429_EncoderContext();

    bool Init();
};

////////////////////////////////////////////////////////////////////////////

class Depacketizer
{
  public:
    virtual ~Depacketizer() { }
    virtual void NewFrame() = 0;
    virtual bool AddPacket(const PluginCodec_RTP & packet) = 0;
    virtual bool IsValid() = 0;
    virtual bool IsIntraFrame() = 0;
    virtual BYTE * GetBuffer() = 0;
    virtual size_t GetLength() = 0;
};

////////////////////////////////////////////////////////////////////////////

class H263_Base_DecoderContext
{
  protected:
    H263_Base_DecoderContext(const char * prefix, Depacketizer * depacketizer);
  public:
    ~H263_Base_DecoderContext();

    bool OpenCodec();
    void CloseCodec();

    virtual bool DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags);

    bool GetStatistics(char * stats, size_t maxSize);

  protected:
    const char *     m_prefix;
    AVCodec        * m_codec;
    AVCodecContext * m_context;
    AVFrame        * m_outputFrame;
    Depacketizer   * m_depacketizer;
    CriticalSection  m_mutex;
};

////////////////////////////////////////////////////////////////////////////

class H263_RFC2190_DecoderContext : public H263_Base_DecoderContext
{
  public:
    H263_RFC2190_DecoderContext();
};

////////////////////////////////////////////////////////////////////////////

class H263_RFC2429_DecoderContext : public H263_Base_DecoderContext
{
  public:
     H263_RFC2429_DecoderContext();
};

////////////////////////////////////////////////////////////////////////////


#endif /* __H263P_1998_H__ */
