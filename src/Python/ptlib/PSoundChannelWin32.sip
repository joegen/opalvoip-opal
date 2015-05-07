/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
%End

class PWaveFormat : PObject
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\sound.h>
#include <ptlib\msos\ptlib\sound_win32.h>
%End
  public:
    PWaveFormat();
    ~PWaveFormat();
    PWaveFormat(const PWaveFormat & fmt);
    // PWaveFormat & operator=(const PWaveFormat & fmt);

    void SetFormat(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample);
    void SetFormat(const void * data, PINDEX size);

    PBoolean       SetSize   (PINDEX sz);
    PINDEX         GetSize   () const;
};


class PSoundChannelWin32: PSoundChannel
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\sound.h>
#include <ptlib\msos\ptlib\sound_win32.h>
%End
 public:
    PSoundChannelWin32();
    void Construct();
    PSoundChannelWin32(const PString &device,
                     PSoundChannel::Directions dir,
                     unsigned numChannels,
                     unsigned sampleRate,
                     unsigned bitsPerSample);
    ~PSoundChannelWin32();
    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);
    PBoolean Open(
      const PString & device,
      Directions dir,
      unsigned numChannels,
      unsigned sampleRate,
      unsigned bitsPerSample
    );

    PBoolean Close();
    PBoolean IsOpen() const;
    PBoolean Write(const void * buf, PINDEX len);
    PBoolean Read(void * buf, PINDEX len);
    PBoolean SetFormat(unsigned numChannels,
                   unsigned sampleRate,
                   unsigned bitsPerSample);
    unsigned GetChannels() const;
    unsigned GetSampleRate() const;
    unsigned GetSampleSize() const;
    PBoolean SetBuffers(PINDEX size, PINDEX count);
    PBoolean GetBuffers(PINDEX & size, PINDEX & count);
    PBoolean PlaySound(const PSound & sound, PBoolean wait);
    PBoolean PlayFile(const PFilePath & filename, PBoolean wait);
    PBoolean HasPlayCompleted();
    PBoolean WaitForPlayCompletion();
    PBoolean RecordSound(PSound & sound);
    PBoolean RecordFile(const PFilePath & filename);
    PBoolean StartRecording();
    PBoolean IsRecordBufferFull();
    PBoolean AreAllRecordBuffersFull();
    PBoolean WaitForRecordBufferFull();
    PBoolean WaitForAllRecordBuffersFull();
    PBoolean Abort();
    PBoolean SetVolume(unsigned newVal);
    PBoolean GetVolume(unsigned &devVol);

  public:
    // Overrides from class PChannel
    virtual PString GetName() const;
      // Return the name of the channel.

      
    PString GetErrorText(ErrorGroup group = NumErrorGroups) const;
    // Get a text form of the last error encountered.

    PBoolean SetFormat(const PWaveFormat & format);

    PBoolean Open(const PString & device, Directions dir,const PWaveFormat & format);
    // Open with format other than PCM
};

