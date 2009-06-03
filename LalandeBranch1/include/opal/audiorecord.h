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


#ifndef OPAL_OPAL_AUDIORECORD_H
#define OPAL_OPAL_AUDIORECORD_H

/////////////////////////////////////////////////////////////////////////////
//
//  This class manages the recording of OPAL calls using the AudioMixer class
//
#include <opal/buildopts.h>

#include <opal/opalmixer.h>

class OpalRecordManager
{
  public:
    virtual ~OpalRecordManager() { }

    virtual bool Open(const PString & callToken, const PFilePath & fn, bool mono) = 0;
    virtual bool IsOpen(const PString & callToken) const = 0;
    virtual bool CloseStream(const PString & callToken, const std::string & strmId) = 0;
    virtual bool Close(const PString & callToken) = 0;
    virtual bool WriteAudio(const PString & callToken, const std::string & strmId, const RTP_DataFrame & rtp) = 0;
};


class OpalWAVRecordManager : public OpalRecordManager
{
  public:
    class Mixer_T : public OpalAudioMixer
    {
      protected:
        OpalWAVFile m_file;
        bool        m_mono;
        bool        m_started;

      public:
        Mixer_T();
        bool Open(const PFilePath & fn, bool mono);
        bool IsOpen() const { return m_file.IsOpen(); }
        bool Close();
        virtual PBoolean OnWriteAudio(const MixerFrame & mixerFrame);
    };

  public:
    OpalWAVRecordManager();
    ~OpalWAVRecordManager();

    virtual bool Open(const PString & callToken, const PFilePath & fn, bool mono);
    virtual bool IsOpen(const PString & callToken) const;
    virtual bool CloseStream(const PString & callToken, const std::string & strmId);
    virtual bool Close(const PString & callToken);
    virtual bool WriteAudio(const PString & callToken, const std::string & strmId, const RTP_DataFrame & rtp);

  protected:
    typedef std::map<PString, Mixer_T *> MixerMap_T;
    MixerMap_T m_mixers;
    PMutex     m_mutex;
};


#endif // OPAL_OPAL_AUDIORECORD_H
