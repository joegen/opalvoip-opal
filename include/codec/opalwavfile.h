/*
 * OpalWavFile.h
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
 * $Log: opalwavfile.h,v $
 * Revision 1.2002  2002/09/06 07:18:44  robertj
 * OPAL port.
 *
 * Revision 1.4  2002/09/06 06:20:37  robertj
 * More cosmetic changes
 *
 * Revision 1.3  2002/08/08 13:00:02  craigs
 * Remove unused definition of LastReadCount thanks to Peter 'Luna' Runestig
 *
 * Revision 1.2  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.1  2002/06/20 01:21:03  craigs
 * Initial version
 *
 */

#ifndef __OPALWAVFILE_H
#define __OPALWAVFILE_H

#ifdef __GNUC__
#pragma interface
#endif


#include <ptclib/pwavfile.h>



/**This class is similar to the PWavFile class found in the PWlib
   components library. However, it will tranparently convert all data
   to/from PCM format, allowing applications to be unconcerned with 
   the underlying data format. Note that this will only work with
   sample-based formats that can be converted to/from PCM data, such as
   uLaw and aLaw
  */

class OpalWAVFile : public PWAVFile
{
  PCLASSINFO(OpalWAVFile, PWAVFile);
  public:
    OpalWAVFile(
      unsigned format = fmt_PCM /// Type of WAV File to create
    );

    /**Create a unique temporary file name, and open the file in the specified
       mode and using the specified options. Note that opening a new, unique,
       temporary file name in ReadOnly mode will always fail. This would only
       be usefull in a mode and options that will create the file.

       If a WAV file is being created, the type parameter can be used
       to create a PCM Wave file or a G.723.1 Wave file by using
       #WaveType enum#

       The #PChannel::IsOpen()# function may be used after object
       construction to determine if the file was successfully opened.
     */
    OpalWAVFile(
      OpenMode mode,            /// Mode in which to open the file.
      int opts = ModeDefault,   /// #OpenOptions enum# for open operation.
      unsigned format = fmt_PCM /// Type of WAV File to create
    );

    /**Create a WAV file object with the specified name and open it in
       the specified mode and with the specified options.
       If a WAV file is being created, the type parameter can be used
       to create a PCM Wave file or a G.723.1 Wave file by using
       #WaveType enum#

       The #PChannel::IsOpen()# function may be used after object
       construction to determine if the file was successfully opened.
     */
    OpalWAVFile(
      const PFilePath & name,     /// Name of file to open.
      OpenMode mode = ReadWrite,  /// Mode in which to open the file.
      int opts = ModeDefault,     /// #OpenOptions enum# for open operation.
      unsigned format = fmt_PCM /// Type of WAV File to create
    );

    /** All of the following functions are the same for their PWavFile equivalents,
        but will automatically adjust if uLaw or ALaw format
      */
    unsigned GetFormat() const;

    BOOL Read(void * buf, PINDEX len);

    BOOL Write(const void * buf, PINDEX len);

    off_t GetPosition() const;

    BOOL SetPosition(off_t pos, FilePositionOrigin origin = Start);

    unsigned GetSampleSize() const;

    off_t GetDataLength();

  protected:
    void SetUnderlyingFormat(unsigned format);
    BOOL IsFormatXLaw() const;
    unsigned realFormat;
    BOOL translate;
};

#endif // __OPALWAVFILE_H


// End of File ///////////////////////////////////////////////////////////////
