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

#include <opal/opalmixer.h>

class OpalRecordManager
{
  public:
    class Mixer_T : public OpalAudioMixer
    {
      protected:
        OpalWAVFile file;
        BOOL mono;
        BOOL started;

      public:
        Mixer_T();
        BOOL Open(const PFilePath & fn);
        BOOL Close();
        BOOL OnWriteAudio(const MixerFrame & mixerFrame);
    };

    Mixer_T mixer;

  protected:
    PMutex mutex;
    PString token;
    BOOL started;

  public:
    OpalRecordManager();
    BOOL Open(const PString & _callToken, const PFilePath & fn);
    BOOL CloseStream(const PString & _callToken, const std::string & _strm);
    BOOL Close(const PString & _callToken);
    BOOL WriteAudio(const PString & _callToken, const std::string & strm, const RTP_DataFrame & rtp);
};


#endif // _OPALAUDIOMIXER_H
