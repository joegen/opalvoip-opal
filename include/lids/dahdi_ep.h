/*
 * dahdi_ep.h
 *
 * DAHDI Line Interface Device EndPoint
 *
 * Open Phone Abstraction Library
 *
 * Copyright (C) 2011 Post Increment
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
 * The Original Code is Opal Library
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_LIDS_DAHDI_EP_H
#define OPAL_LIDS_DAHDI_EP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#include <ptlib.h>
#include <ptlib/mutex.h>
#include <ptclib/inetmail.h>
#include <ptclib/dtmf.h>

#include <lids/lid.h>
#include <codec/g711codec.h>

#include <dahdi/user.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <signal.h>

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <vector>
#include <queue>
using namespace std;

////////////////////////////////////////////////

class DahdiLineInterfaceDevice : public OpalLineInterfaceDevice
{
  PCLASSINFO(DahdiLineInterfaceDevice, OpalLineInterfaceDevice);

  public:
    static const char * DeviceName;

    DahdiLineInterfaceDevice();

    // inherited from OpalLineInterfaceDevice

    // these can be called before Open
    virtual PString GetDeviceType() const    { return "dahdi"; }
    virtual PString GetDeviceName() const    { return DeviceName; }
    virtual PStringArray GetAllNames() const { PStringArray l; l.AppendString(DeviceName); return l; }
    virtual PString GetDescription() const   { return "DAHDI"; }

    virtual OpalMediaFormatList GetMediaFormats() const  
    { 
      OpalMediaFormatList l; 
      l += OpalPCM16;  
      return l; 
    }

    // open the device
    virtual bool Open(const PString & device);
    virtual bool Close();

    // these can be called after Open
    virtual unsigned GetLineCount() const                                               
    { return m_channelInfoList.size(); }

    virtual bool IsLineTerminal(unsigned line)                                          
    { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->IsFXS(); }

    virtual bool IsLineOffHook(unsigned line)                                           
    { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->IsOffHook(); }

    virtual bool SetLineOffHook(unsigned line, bool newState = true)                    
    { if (!IsValidLine(line) || IsLineTerminal(line)) return false; return m_channelInfoList[line]->SetOffHook(newState); }

    virtual bool PlayTone(unsigned line, CallProgressTones tone)   { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->PlayTone(tone); }

    virtual bool IsTonePlaying(unsigned line)  { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->IsTonePlaying(); }

    virtual bool StopTone(unsigned line)       { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->StopTone(); }

    virtual char ReadDTMF(unsigned line)       { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->ReadDTMF(); }

    virtual PINDEX GetReadFrameSize(unsigned line) { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->GetReadFrameSize(); }

    virtual PINDEX GetWriteFrameSize(unsigned line) { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->GetWriteFrameSize(); }

    virtual bool SetReadFrameSize(unsigned line, PINDEX frameSize)   { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->SetReadFrameSize(frameSize); }
    virtual bool SetWriteFrameSize(unsigned line, PINDEX frameSize)  { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->SetWriteFrameSize(frameSize); }

    virtual bool SetReadFormat(unsigned line, const OpalMediaFormat & mediaFormat);
    virtual bool SetWriteFormat(unsigned line, const OpalMediaFormat & mediaFormat);

    virtual bool StopReading(unsigned line);
    virtual bool StopWriting(unsigned line);

    virtual OpalMediaFormat GetReadFormat(unsigned line)
    { return OpalG711ALaw; }

    virtual OpalMediaFormat GetWriteFormat(unsigned line)
    { return OpalG711ALaw; }

    virtual bool EnableAudio(unsigned line, bool enable = true)
    { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->EnableAudio(enable); }

    virtual bool ReadFrame(unsigned line, void * buf, PINDEX & count)
    { if (!IsValidLine(line)) return false; return m_channelInfoList[line]->ReadFrame(buf, count); }

    virtual bool WriteFrame(unsigned line, const void * buf, PINDEX count, PINDEX & written)
    {  if (!IsValidLine(line)) return false; return m_channelInfoList[line]->WriteFrame(buf, count, written); }

    virtual bool SetRecordVolume(unsigned line, unsigned volume)
    {  if (!IsValidLine(line)) return false; return m_channelInfoList[line]->SetRecordVolume(volume); }

    virtual bool SetPlayVolume(unsigned line, unsigned volume)
    {  if (!IsValidLine(line)) return false; return m_channelInfoList[line]->SetPlayVolume(volume); }

    virtual bool GetRecordVolume(unsigned line, unsigned & volume)
    {  if (!IsValidLine(line)) return false; return m_channelInfoList[line]->GetRecordVolume(volume); }

    virtual bool GetPlayVolume(unsigned line, unsigned & volume)
    {  if (!IsValidLine(line)) return false; return m_channelInfoList[line]->GetPlayVolume(volume); }

    virtual bool IsAudioEnabled(unsigned line) const { return false; return m_channelInfoList[line]->IsAudioEnabled(); }

    virtual bool IsValidLine(unsigned line) const { return line < (unsigned)m_channelInfoList.size(); }

    struct ChannelInfo : public PObject {

      ChannelInfo(dahdi_params & parms);
      virtual ~ChannelInfo();

      virtual bool Open(int samplesPerBlock);
      virtual bool Close();
      void ThreadMain();

      virtual bool IsFXS()          { return false; }
      virtual bool IsOffHook()      { return false; }
      virtual bool IsAudioEnabled() const { return m_audioEnable; }
      virtual bool IsMediaRunning() const { return m_mediaStarted; }

      virtual bool IsTonePlaying();
      virtual bool PlayTone(CallProgressTones tone);
      virtual bool StopTone();

      virtual bool EnableAudio(bool enable);
      virtual bool SetOffHook(bool newState) { return false; }

      virtual PINDEX GetReadFrameSize()                      { return m_samplesPerBlock*2; }
      virtual PINDEX GetWriteFrameSize()                     { return m_samplesPerBlock*2; }
      virtual bool SetReadFrameSize(PINDEX frameSize)        { return frameSize == m_samplesPerBlock*2; }
      virtual bool SetWriteFrameSize(PINDEX frameSize)       { return frameSize == m_samplesPerBlock*2; }

      virtual bool ReadFrame(void * buf, PINDEX & count);
      virtual bool WriteFrame(const void * buf, PINDEX count, PINDEX & written);
      virtual bool InternalReadFrame(void * buf);

      virtual bool SetReadFormat(const OpalMediaFormat & mediaFormat);
      virtual bool SetWriteFormat(const OpalMediaFormat & mediaFormat);

      virtual bool StopReading();
      virtual bool StopWriting();

      virtual bool StartMedia();
      virtual bool StopMedia();

      virtual char ReadDTMF();

      virtual bool LookForEvent();
      virtual bool DetectTones(void * buffer, int len);

      virtual bool SetRecordVolume(unsigned volume)
      {  /* m_readVol = volume; */return true; }

      virtual bool SetPlayVolume(unsigned volume)
      {  /* m_writeVol = volume; */ return true; }

      virtual bool GetRecordVolume(unsigned & volume)
      {  volume = m_readVol; return true; }

      virtual bool GetPlayVolume(unsigned & volume)
      {  volume = m_writeVol; return true; }

      virtual void OnHook() { }
      virtual void OffHook() { }

      virtual void Flush();

      short DecodeSample(BYTE sample);
      BYTE EncodeSample(short sample);

      int m_spanNumber;
      int m_channelNumber;
      int m_chanPos;

      bool m_hasHardwareToneDetection;
      PDTMFDecoder m_dtmfDecoder;

      PMutex m_mutex;
      int m_fd;
      int m_samplesPerBlock;
      bool m_audioEnable;
      bool m_mediaStarted;

      BYTE * m_toneBuffer;
      int m_toneBufferLen;
      int m_toneBufferPos;
      bool m_isALaw;

      PMutex m_dtmfMutex;
      std::queue<char> m_dtmfQueue;

      int m_writeVol;
      int m_readVol;

      std::vector<BYTE> m_readBuffer;
      std::vector<BYTE> m_writeBuffer;
    };

    struct FXSChannelInfo : public ChannelInfo {
      FXSChannelInfo(dahdi_params & parms);
      virtual ~FXSChannelInfo();

      virtual bool IsOffHook()   { return m_hookState == eOffHook; }

      virtual void OnHook();
      virtual void OffHook();
      virtual bool IsFXS()   { return true; }

      enum HookState {
        eOnHook,
        eOffHook
      } m_hookState;
    };

    static bool IsDigitalSpan(dahdi_spaninfo & span)
    { return span.linecompat > 0; }

    void BuildPollFDs();
    void ThreadMain();

  protected:
    int m_samplesPerBlock;
    PMutex m_mutex;
    typedef std::vector<ChannelInfo *> ChannelInfoList;
    ChannelInfoList m_channelInfoList;

    PThread * m_thread;
    bool m_running;
    std::vector<pollfd> m_pollFds;
    PMutex m_pollListMutex;
    bool m_pollListDirty;
};

#endif // OPAL_LIDS_DAHDI_EP_H


