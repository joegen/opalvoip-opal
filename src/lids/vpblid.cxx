/*
 * vpblid.cxx
 *
 * Voicetronix VPB4 line interface device
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * $Log: vpblid.cxx,v $
 * Revision 1.2002  2001/08/01 05:21:21  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.7  2001/05/11 04:43:43  robertj
 * Added variable names for standard PCM-16 media format name.
 *
 * Revision 1.6  2001/01/25 07:27:17  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.5  2000/11/24 10:54:45  robertj
 * Modified the ReadFrame/WriteFrame functions to allow for variable length codecs.
 *
 * Revision 1.4  2000/11/20 04:37:03  robertj
 * Changed tone detection API slightly to allow detection of multiple
 * simultaneous tones
 *
 * Revision 1.3  2000/05/02 04:32:28  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.2  2000/01/07 08:28:09  robertj
 * Additions and changes to line interface device base class.
 *
 * Revision 1.1  1999/12/23 23:02:36  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "vpblid.cxx"
#endif

#include <lids/vpblid.h>

#include <lids/vpbapi.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalVpbDevice::OpalVpbDevice()
{
  cardNumber = 0;
  lineCount = 0;
  vpb_seterrormode(VPB_ERROR_CODE);
}


BOOL OpalVpbDevice::Open(const PString & device)
{
  Close();

  cardNumber = device.AsUnsigned(10);

  lineCount = 0;
  while (lineCount < MaxLineCount && lineState[lineCount].Open(cardNumber, lineCount))
    lineCount++;

  os_handle = lineCount > 0 ? 1 : -1;

  return IsOpen();
}


BOOL OpalVpbDevice::LineState::Open(unsigned cardNumber, unsigned lineNumber)
{
  handle = vpb_open(cardNumber, lineNumber+1);
  if (handle < 0)
    return FALSE;

  readIdle = writeIdle = TRUE;
  readFrameSize = writeFrameSize = 480;
  currentHookState = FALSE;
  vpb_sethook_sync(handle, VPB_ONHOOK);
  vpb_set_ring(handle, 1, 5000);
  vpb_set_event_mask(handle, VPB_MRING|VPB_MTONEDETECT);

  return TRUE;
}


BOOL OpalVpbDevice::Close()
{
  for (unsigned line = 0; line < lineCount; line++)
    vpb_close(lineState[line].handle);

  os_handle = -1;
  return TRUE;
}


PString OpalVpbDevice::GetName() const
{
  char buf[100];
  vpb_get_model(buf);
  return psprintf("%s/%u", buf, cardNumber);
}


unsigned OpalVpbDevice::GetLineCount()
{
  return lineCount;
}


BOOL OpalVpbDevice::IsLineOffHook(unsigned line)
{
  if (line >= MaxLineCount)
    return FALSE;

  return lineState[line].currentHookState;
}


BOOL OpalVpbDevice::SetLineOffHook(unsigned line, BOOL newState)
{
  if (line >= MaxLineCount)
    return FALSE;

  return lineState[line].SetLineOffHook(newState);
}


BOOL OpalVpbDevice::LineState::SetLineOffHook(BOOL newState)
{
  currentHookState = newState;
  return vpb_sethook_sync(handle, newState ? VPB_OFFHOOK : VPB_ONHOOK) >= 0;
}


BOOL OpalVpbDevice::IsLineRinging(unsigned line, DWORD * cadence)
{
  if (line >= MaxLineCount)
    return FALSE;

  return lineState[line].IsLineRinging(cadence);
}


BOOL OpalVpbDevice::LineState::IsLineRinging(DWORD * /*cadence*/)
{
  VPB_EVENT event;
  if (vpb_get_event_ch_async(handle, &event) == VPB_OK && event.type == VPB_RING)
    ringTimeout = 5000;

  return ringTimeout.IsRunning();
}


static const struct {
  const char * mediaFormat;
  WORD         mode;
} CodecInfo[] = {
  { OPAL_PCM16,       VPB_LINEAR },
  { "G.711-uLaw-64k", VPB_MULAW },
  { "G.711-ALaw-64k", VPB_ALAW  },
};


OpalMediaFormatList OpalVpbDevice::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  for (PINDEX i = 0; i < PARRAYSIZE(CodecInfo); i++)
    formats += CodecInfo[i].mediaFormat;

  return formats;
}


static PINDEX FindCodec(const OpalMediaFormat & mediaFormat)
{
  for (PINDEX codecType = 0; codecType < PARRAYSIZE(CodecInfo); codecType++) {
    if (mediaFormat == CodecInfo[codecType].mediaFormat)
      return codecType;
  }

  return P_MAX_INDEX;
}


BOOL OpalVpbDevice::SetReadFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  if (line >= MaxLineCount)
    return FALSE;

  PTRACE(4, "VPB\tSetReadFormat(" << mediaFormat << ')');

  lineState[line].readFormat = FindCodec(mediaFormat);
  if (lineState[line].readFormat == P_MAX_INDEX)
    return FALSE;

  if (vpb_record_buf_start(lineState[line].handle,
                           CodecInfo[lineState[line].readFormat].mode) < 0)
    return FALSE;

  lineState[line].readIdle = FALSE;
  return TRUE;
}


BOOL OpalVpbDevice::SetWriteFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  if (line >= MaxLineCount)
    return FALSE;

  PTRACE(4, "VPB\tSetWriteFormat(" << mediaFormat << ')');

  lineState[line].writeFormat = FindCodec(mediaFormat);
  if (lineState[line].writeFormat == P_MAX_INDEX)
    return FALSE;

  if (vpb_play_buf_start(lineState[line].handle,
                         CodecInfo[lineState[line].writeFormat].mode) < 0)
    return FALSE;

  lineState[line].writeIdle = FALSE;
  return TRUE;
}


OpalMediaFormat OpalVpbDevice::GetReadFormat(unsigned line)
{
  if (lineState[line].readFormat == P_MAX_INDEX)
    return "";
  return CodecInfo[lineState[line].readFormat].mediaFormat;
}


OpalMediaFormat OpalVpbDevice::GetWriteFormat(unsigned line)
{
  if (lineState[line].writeFormat == P_MAX_INDEX)
    return "";
  return CodecInfo[lineState[line].writeFormat].mediaFormat;
}


BOOL OpalVpbDevice::StopReadCodec(unsigned line)
{
  if (line >= MaxLineCount)
    return FALSE;

  PTRACE(3, "VPB\tStopping read codec");

  if (lineState[line].readIdle)
    return FALSE;

  lineState[line].readIdle = TRUE;
  return vpb_record_buf_finish(lineState[line].handle) >= 0;
}


BOOL OpalVpbDevice::StopWriteCodec(unsigned line)
{
  if (line >= MaxLineCount)
    return FALSE;

  PTRACE(3, "xJack\tStopping write codec");

  if (lineState[line].writeIdle)
    return FALSE;

  lineState[line].writeIdle = TRUE;
  return vpb_play_buf_finish(lineState[line].handle) >= 0;
}


BOOL OpalVpbDevice::SetReadFrameSize(unsigned line, PINDEX size)
{
  if (line >= MaxLineCount)
    return FALSE;

  lineState[line].readFrameSize = size;
  return TRUE;
}


PINDEX OpalVpbDevice::SetWriteFrameSize(unsigned line, PINDEX size)
{
  if (line >= MaxLineCount)
    return FALSE;

  lineState[line].writeFrameSize = size;
  return TRUE;
}


PINDEX OpalVpbDevice::GetReadFrameSize(unsigned line)
{
  if (line >= MaxLineCount)
    return FALSE;

  return lineState[line].readFrameSize;
}


PINDEX OpalVpbDevice::GetWriteFrameSize(unsigned line)
{
  if (line >= MaxLineCount)
    return FALSE;

  return lineState[line].writeFrameSize;
}


BOOL OpalVpbDevice::ReadFrame(unsigned line, void * buf, PINDEX & count)
{
  if (line >= MaxLineCount)
    return FALSE;

  count = lineState[line].readFrameSize;
  return vpb_record_buf_sync(lineState[line].handle, (char *)buf, (WORD)count) >= 0;
}


BOOL OpalVpbDevice::WriteFrame(unsigned line, const void * buf, PINDEX count, PINDEX & written)
{
  written = 0;
  if (line >= MaxLineCount)
    return FALSE;

  if (vpb_play_buf_sync(lineState[line].handle, (char *)buf, (WORD)count) < 0)
    return FALSE;

  written = count;
  return TRUE;
}


BOOL OpalVpbDevice::SetRecordVolume(unsigned line, unsigned volume)
{
  if (line >= MaxLineCount)
    return FALSE;

  return vpb_record_set_gain(lineState[line].handle, (float)(volume/100.0*24.0-12.0)) >= 0;
}


BOOL OpalVpbDevice::SetPlayVolume(unsigned line, unsigned volume)
{
  if (line >= MaxLineCount)
    return FALSE;

  return vpb_play_set_gain(lineState[line].handle, (float)(volume/100.0*24.0-12.0)) >= 0;
}


char OpalVpbDevice::ReadDTMF(unsigned line)
{
  if (line >= MaxLineCount)
    return '\0';

  VPB_DIGITS vd;
  vd.term_digits = "";
  vd.max_digits = 1;
  vd.digit_time_out = 10;
  vd.inter_digit_time_out = 10;

  char buf[VPB_MAX_STR];

  if (vpb_get_digits_sync(lineState[line].handle, &vd, buf) == 1)
    return buf[0];

  return '\0';
}


BOOL OpalVpbDevice::PlayDTMF(unsigned line, const char * digits, DWORD, DWORD)
{
  if (line >= MaxLineCount)
    return FALSE;

  return vpb_dial_sync(lineState[line].handle, (char *)digits) >= 0;
}


unsigned OpalVpbDevice::IsToneDetected(unsigned line)
{
  if (line >= MaxLineCount)
    return NoTone;

  VPB_EVENT event;
  if (vpb_get_event_ch_async(lineState[line].handle, &event) == VPB_NO_EVENTS)
    return NoTone;

  if (event.type != VPB_TONEDETECT)
    return NoTone;

  switch (event.data) {
    case VPB_DIAL :
      return DialTone;

    case VPB_RINGBACK :
      return RingTone;

    case VPB_BUSY :
      return BusyTone;

//    case VPB_GRUNT :
//      return DisconnectTone;
  }

  return NoTone;
}


/////////////////////////////////////////////////////////////////////////////
