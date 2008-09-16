/*
 * audiorecord.h
 *
 * OPAL audio record manager
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2007 Post Increment
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#ifndef _OPALAUDIORECORD_H
#define _OPALAUDIORECORD_H

/////////////////////////////////////////////////////////////////////////////
//
//  This class manages the recording of OPAL calls using the AudioMixer class
//
#include <opal/buildopts.h>

#include <opal/opalmixer.h>

class OpalRecordManager
{
  public:
    class Mixer_T : public OpalAudioMixer
    {
      protected:
        OpalWAVFile file;
        PBoolean mono;
        PBoolean started;

      public:
        Mixer_T();
        PBoolean Open(const PFilePath & fn);
        bool IsOpen() const { return file.IsOpen(); }
        PBoolean Close();
        PBoolean OnWriteAudio(const MixerFrame & mixerFrame);
    };

    Mixer_T mixer;

  protected:
    PMutex mutex;
    PString token;
    PBoolean started;

  public:
    OpalRecordManager();
    PBoolean Open(const PString & _callToken, const PFilePath & fn);
    bool IsOpen() const { return mixer.IsOpen(); }
    PBoolean CloseStream(const PString & _callToken, const std::string & _strm);
    PBoolean Close(const PString & _callToken);
    PBoolean WriteAudio(const PString & _callToken, const std::string & strm, const RTP_DataFrame & rtp);
};


#endif // _OPALAUDIOMIXER_H
