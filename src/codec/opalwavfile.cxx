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
 * Revision 1.2002  2002/09/06 07:19:21  robertj
 * OPAL port.
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

  // remember: the application thinks samples are 16 bits
  // so the actual position must be doubled before returning it
  switch (format) {
    case fmt_ALaw:
    case fmt_uLaw:
      return len * 2;

    default:
      break;
  }

  return len;
}
