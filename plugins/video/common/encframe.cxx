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
 */

#include "encframe.h"

#ifndef PLUGIN_CODEC_DLL_EXPORTS
#include "plugin_config.h"
#endif


OpalPluginFrame::OpalPluginFrame()
  : m_length(0)
  , m_maxSize(0)
  , m_buffer(NULL)
  , m_maxPayloadSize(PluginCodec_RTP_MaxPayloadSize)
{
}


OpalPluginFrame::~OpalPluginFrame()
{
  if (m_buffer != NULL)
    free(m_buffer);
}


void OpalPluginFrame::SetMaxPayloadSize(size_t size)
{
  m_maxPayloadSize = size;
}


bool OpalPluginFrame::SetSize(size_t newSize)
{
  // Some encoders are a bit rude and overwrite the end of the buffer, usually caused
  // by some SIMD processor instructions that operate on 16 byte chunks and not being
  // aligned to that. So add a bit, just in case.
  if ((m_buffer = (uint8_t *)realloc(m_buffer, newSize+16)) == NULL) {
    PTRACE(1, "FFMPEG", "Could not (re)allocate " << newSize << " bytes of memory.");
    return false;
  }

  m_maxSize = newSize;
  return true;
}


bool OpalPluginFrame::Append(const uint8_t * data, size_t len)
{
  size_t newSize = m_length + len;
  if (newSize > m_maxSize && !SetSize(newSize))
    return false;

  memcpy(m_buffer+m_length, data, len);
  m_length += len;
  return true;
}


bool OpalPluginFrame::Reset(size_t len)
{
  if (len > m_maxSize)
    return false;

  m_length = len;
  return true;
}


void OpalPluginFrame::RTPCallBack(void *, int, int)
{
}

