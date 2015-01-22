/*
 * Common encoded frame handling
 *
 * Copyright (C) 2014 Vox Lucida Pty. Ltd.
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
 * Contributor(s): Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *                 Matthias Schneider (ma30002000@yahoo.de)
 *                 Robert Jongbloed (robertj@voxlucida.com.au)
 */

#ifndef __ENCFRAME_H__
#define __ENCFRAME_H__ 1

#include "platform.h"
#include <codec/opalplugin.hpp>


class OpalPluginFrame
{
  protected:
    size_t    m_length;
    size_t    m_maxSize;
    uint8_t * m_buffer;
    size_t    m_maxPayloadSize;

  public:
    OpalPluginFrame();
    virtual ~OpalPluginFrame();

    virtual const char * GetName() const { return ""; }

    uint8_t * GetBuffer() const { return m_buffer; }
    size_t GetMaxSize() const { return m_maxSize; }
    size_t GetLength() const { return m_length; }
    size_t GetMaxPayloadSize() const { return m_maxPayloadSize; }

    virtual void SetMaxPayloadSize(size_t size);
    virtual bool SetSize(size_t size);
    virtual bool Reset(size_t len = 0);

    virtual bool GetPacket(PluginCodec_RTP & rtp, unsigned & flags) = 0;
    virtual bool AddPacket(const PluginCodec_RTP & rtp, unsigned & flags) = 0;

    virtual bool IsIntraFrame() const = 0;

    virtual void RTPCallBack(void * data, int size, int mbCount);

  protected:
    virtual bool Append(const uint8_t * data, size_t len);
};


#endif // __ENCFRAME_H__
