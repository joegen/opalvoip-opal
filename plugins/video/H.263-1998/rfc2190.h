/*
 * RFC 2190 packetiser and unpacketiser 
 *
 * Copyright (C) 2008 Post Increment
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
 * The Original Code is Opal
 *
 * Contributor(s): Craig Southeren <craigs@postincrement.com>
 *
 */

#ifndef _RFC2190_H_
#define _RFC2190_H_

#include "../../common/ffmpeg.h"

#include <list>


class PluginCodec_RTP;


class RFC2190EncodedFrame : public FFMPEGCodec::EncodedFrame
{
  public:
    virtual const char * GetName() const { return "RFC2190"; }
};


class RFC2190Packetizer : public RFC2190EncodedFrame
{
  public:
    RFC2190Packetizer();

    virtual bool Reset(size_t len = 0);
    virtual bool SetResolution(unsigned width, unsigned height);

    virtual bool GetPacket(PluginCodec_RTP & rtp, unsigned & flags);
    virtual bool AddPacket(const PluginCodec_RTP & rtp, unsigned & flags);

    virtual void RTPCallBack(void * data, int size, int mbCount);

  private:
    unsigned int TR;
    size_t m_frameSize;
    int iFrame;
    int annexD, annexE, annexF, annexG, pQuant, cpm;
    int macroblocksPerGOB;

    struct fragment {
      size_t length;
      unsigned mbNum;
    };

    typedef std::list<fragment> FragmentListType;
    FragmentListType fragments;     // use list because we want fast insert and delete
    FragmentListType::iterator currFrag;
    unsigned char * fragPtr;

    unsigned m_currentMB;
    size_t m_currentBytes;
};


class RFC2190Depacketizer : public RFC2190EncodedFrame
{
  public:
    RFC2190Depacketizer();

    virtual void Reset();
    virtual bool GetPacket(PluginCodec_RTP & rtp, unsigned & flags);
    virtual bool AddPacket(const PluginCodec_RTP & rtp, unsigned & flags);
    virtual bool IsIntraFrame() const;

  protected:
    bool LostSync(unsigned & flags);

    bool     m_first;
    bool     m_skipUntilEndOfFrame;
    unsigned m_lastEbit;
    bool     m_isIFrame;
};

#endif // _RFC2190_H_

