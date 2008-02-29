/*
 * OpalWavFile.cxx
 *
 * WAV file class with auto-PCM conversion
 *
 * OpenH323 Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Log: opalwavfile.cxx,v $
 * Revision 1.2003  2004/02/19 10:53:04  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.1  2002/09/06 07:19:21  robertj
 * OPAL port.
 *
 * Revision 1.3  2003/12/28 00:07:56  csoutheren
 * Added support for 8-bit PCM WAV files
 *
 * Revision 1.2  2002/08/05 10:03:48  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.1  2002/06/20 01:21:32  craigs
 * Initial version
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "opalwavfile.h"
#endif

#include <codec/opalwavfile.h>

#include <codec/g711codec.h>



#define new PNEW


OpalWAVFile::OpalWAVFile(unsigned fmt)
  : PWAVFile(fmt)
{
}


OpalWAVFile::OpalWAVFile(OpenMode mode, int opts, unsigned fmt)
  : PWAVFile(mode, opts, fmt)
{
}


OpalWAVFile::OpalWAVFile(const PFilePath & name, 
                                  OpenMode mode,  /// Mode in which to open the file.
                                       int opts,  /// #OpenOptions enum# for open operation.
                                   unsigned fmt)  /// Type of WAV File to create
  : PWAVFile(name, mode, opts, fmt)
{
}


unsigned OpalWAVFile::GetFormat() const
{
  unsigned fmt = PWAVFile::GetFormat();
  switch (fmt) {
    case fmt_ALaw:
    case fmt_uLaw:
      fmt = fmt_PCM;
      break;

    default:
      break;
  }

  return fmt;
}


BOOL OpalWAVFile::Read(void * buf, PINDEX len)
{
  switch (format) {
    case fmt_uLaw:
      {
        // read the uLaw data
        PINDEX samples = (len / 2);
        PBYTEArray ulaw;
        if (!PWAVFile::Read(ulaw.GetPointer(samples), samples))
          return FALSE;

        // convert to PCM
        PINDEX i;
        short * pcmPtr = (short *)buf;
        for (i = 0; i < samples; i++)
          *pcmPtr++ = (short)Opal_PCM_G711_uLaw::ConvertSample(ulaw[i]);

        // fake the lastReadCount
        lastReadCount = len;
      }
      return TRUE;

    case fmt_ALaw:
      {
        // read the aLaw data
        PINDEX samples = (len / 2);
        PBYTEArray Alaw;
        if (!PWAVFile::Read(Alaw.GetPointer(samples), samples))
          return FALSE;

        // convert to PCM
        PINDEX i;
        short * pcmPtr = (short *)buf;
        for (i = 0; i < samples; i++)
          *pcmPtr++ = (short)Opal_G711_ALaw_PCM::ConvertSample(Alaw[i]);

        // fake the lastReadCount
        lastReadCount = len;
      }
      return TRUE;

    case fmt_PCM:
      if (bitsPerSample != 8)
        break;

      {
        // read the PCM data
        PINDEX samples = (len / 2);
        PBYTEArray pcm8;
        if (!PWAVFile::Read(pcm8.GetPointer(samples), samples))
          return FALSE;

        // convert to PCM-16
        PINDEX i;
        short * pcmPtr = (short *)buf;
        for (i = 0; i < samples; i++)
          *pcmPtr++ = (unsigned short)((pcm8[i] << 8) - 0x8000);

        // fake the lastReadCount
        lastReadCount = len;
      }
      return TRUE;

    default:
      break;
  }

  return PWAVFile::Read(buf, len);
}

BOOL OpalWAVFile::Write(const void * buf, PINDEX len)
{
  switch (format) {
    case fmt_ALaw:
    case fmt_uLaw:
      return FALSE;

    case fmt_PCM:
      if (bitsPerSample != 16)
        return FALSE;
      break;

    default:
      break;
  }

  return PWAVFile::Write(buf, len);
}

off_t OpalWAVFile::GetPosition() const
{
  // remember: the application thinks samples are 16 bits
  // so the actual position must be doubled before returning it
  off_t pos = PWAVFile::GetPosition();

  switch (format) {
    case fmt_ALaw:
    case fmt_uLaw:
      return pos * 2;

    case fmt_PCM:
      if (bitsPerSample == 8)
        return pos * 2;
      break;

    default:
      break;
  }

  return pos;
}

BOOL OpalWAVFile::SetPosition(off_t pos, FilePositionOrigin origin)
{
  // remember: the application thinks samples are 16 bits
  // so the applications request must be halved
  switch (format) {
    case fmt_ALaw:
    case fmt_uLaw:
      pos /= 2;
      break;

    case fmt_PCM:
      if (bitsPerSample == 8)
        return pos /= 2;
      break;

    default:
      break;
  }

  return PWAVFile::SetPosition(pos, origin);
}


unsigned OpalWAVFile::GetSampleSize() const
{
  switch (format) {
    case fmt_ALaw:
    case fmt_uLaw:
      return 16;

    case fmt_PCM:
      if (bitsPerSample == 8)
        return 16;

    default:
      break;
  }

  return PWAVFile::GetSampleSize();
}


off_t OpalWAVFile::GetDataLength()
{
  // get length of underlying file
  // if format is not one we can convert, then return length
  off_t len = PWAVFile::GetDataLength();

  switch (format) {
    case fmt_ALaw:
    case fmt_uLaw:
      // the application thinks samples are 16 bits
      // so the actual length must be doubled before returning it
      return len * 2;

    case fmt_PCM:
      if (bitsPerSample == 8)
        return len * 2;
      break;

    default:
      break;
  }

  return len;
}
