/*
 * TigerJet USB HID Plugin LID for OPAL
 *
 * Copyright (C) 2006 Post Increment, All Rights Reserved
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
 * The Original Code is OPAL.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Based on code by Simon Horne of ISVO (Asia) Pte Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: tj_win.cpp,v $
 * Revision 1.2  2006/10/16 09:46:49  rjongbloed
 * Fixed various MSVC 8 warnings
 *
 * Revision 1.1  2006/10/15 07:19:21  rjongbloed
 * Added first cut of TigerJet USB handset LID plug in
 *
 */

#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>

#define PLUGIN_DLL_EXPORTS
#include <lids/lidplugin.h>

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <process.h>
#include <assert.h>
#include <queue>

#include "TjIpSys.h"


/////////////////////////////////////////////////////////////////////////////

static HINSTANCE g_hInstance;

static const char g_PCM16[] = "PCM-16";


class CriticalSection : CRITICAL_SECTION
{
public:
  inline CriticalSection()  { InitializeCriticalSection(this); }
  inline ~CriticalSection() { DeleteCriticalSection(this); }
  inline void Enter()       { EnterCriticalSection(this); }
  inline void Leave()       { LeaveCriticalSection(this); }
};


class Event
{
private:
  HANDLE m_hEvent;
public:
  inline Event()           { m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL); }
  inline ~Event()          { if (m_hEvent != NULL) CloseHandle(m_hEvent); }
  inline bool Wait()       { return WaitForSingleObject(m_hEvent, INFINITE) == WAIT_OBJECT_0; }
  inline void Set()        { SetEvent(m_hEvent); }
  inline DWORD GetHandle() { return (DWORD)m_hEvent; }
};


/////////////////////////////////////////////////////////////////////////////

class Context
{
  protected:
    HMODULE             m_hDLL;
    TJIPSYSCALL         m_pTjIpSysCall;
    TjIpProductID       m_eProductID;
    bool                m_hasBuzzer;

    bool                m_isOffHook;
    bool                m_tonePlaying;

    std::queue<char>    m_Keys;
    CriticalSection     m_KeyMutex;

    HWAVEIN             m_hWaveIn;
    unsigned            m_readFrameSize;
    std::queue<WAVEHDR> m_InBuffers;
    Event               m_InEvent;
    CriticalSection     m_InMutex;

    HWAVEOUT            m_hWaveOut;
    unsigned            m_writeFrameSize;
    std::queue<WAVEHDR> m_OutBuffers;
    Event               m_OutEvent;
    CriticalSection     m_OutMutex;



    static void StaticKeyCallback(void * context, UINT message, WPARAM wParam, LPARAM lParam)
    {
      ((Context *)context)->KeyCallback(message, wParam, lParam);
    }

    void KeyCallback(UINT message, WPARAM wParam, LPARAM lParam)
    {
      m_KeyMutex.Enter();

      switch (message) {
	case WM_TJIP_HID_NEW_KEY:
          switch (wParam) {
            case 0xb0:
              m_Keys.push('0');
              break;
            case 0xb1:
              m_Keys.push('1');
              break;
            case 0xb2:
              m_Keys.push('2');
              break;
            case 0xb3:
              m_Keys.push('3');
              break;
            case 0xb4:
              m_Keys.push('4');
              break;
            case 0xb5:
              m_Keys.push('5');
              break;
            case 0xb6:
              m_Keys.push('6');
              break;
            case 0xb7:
              m_Keys.push('7');
              break;
            case 0xb8:
              m_Keys.push('8');
              break;
            case 0xb9:
              m_Keys.push('9');
              break;
              break;
            case 0xba: // '*' key
              m_Keys.push('*');
              break;
              break;
            case 0xbb: // '#' key
              m_Keys.push('#');
              break;

            case 0x2a: // Clear/Backspace key
              m_Keys.push('\b');
              break;

            case 0x2f: // Mute
              m_Keys.push('M');
              break;

            case 0x51: // down
              m_Keys.push('D');
              break;
            case 0x52: // up
              m_Keys.push('U');
              break;

            case 0x31: // Enter key
              if (!m_isOffHook) {
                m_isOffHook = true;
                while (!m_Keys.empty())
                  m_Keys.pop();
              }
              break;

            case 0x26: // hangup
              m_isOffHook = false;
              break;
          }
          break;

	case WM_TJIP_HID_KEY_RELEASED:
          break;
      }
      m_KeyMutex.Leave();
    }

    bool ReadRegister(unsigned regIndex, void * data, unsigned len = 1)
    {
      TJ_SEND_VENDOR_CMD vc;

      vc.vcCmd.direction = 1;	// read
      vc.vcCmd.bRequest = 4; // 320ns command pulse with auto incremented address
      vc.vcCmd.wValue = 0;
      vc.vcCmd.wIndex = regIndex;
      vc.vcCmd.wLength = len;
      vc.vcCmd.bData   = 0x55;
      vc.dwDataSize = len;
      vc.pDataBuf = data;

      return m_pTjIpSysCall(TJIP_TJ560VENDOR_COMMAND, 0, &vc) != 0;
    }

    bool WriteRegister(unsigned regIndex, const void * data, unsigned len)
    {
      TJ_SEND_VENDOR_CMD vc;

      vc.vcCmd.direction = 0;	// write
      vc.vcCmd.bRequest = 4; // 320ns command pulse with auto incremented address
      vc.vcCmd.wValue = 0;
      vc.vcCmd.wIndex = regIndex;
      vc.vcCmd.wLength = len;
      vc.dwDataSize = len;
      vc.pDataBuf = (PVOID)data;

      return m_pTjIpSysCall(TJIP_TJ560VENDOR_COMMAND, 0, &vc) != 0;
    }

    bool WriteRegister(unsigned regIndex, BYTE data)
    {
      return WriteRegister(regIndex, &data, 1);
    }

    void InitBuzzer()
    {
      //-----------------------------------------------------
      // Check if GIO[8] has hardwire with other pins 
      // Step 1: Store current GIO[3:0] setting
      // step 2: set GIO[3:0] : 0b0000
      // Step 3: set GIO[8] : 0b1, read back and check GIO[8]
      // Step 4: Restore GIO[3:0] to as before
      //-----------------------------------------------------
      BYTE data, Reg0xa;
      ReadRegister(0xa, &Reg0xa);     // store BIO[3:0] setting
      WriteRegister(0xa, 0x55);       // set GIO[3:0] all output as 0b0000
      WriteRegister(0x1a, 3);         // set GIO[8] to high
      ReadRegister(0x1a, &data);      // read back

      m_hasBuzzer = (data & 0x2) == 0x2;
      if (!m_hasBuzzer)              
      {
	WriteRegister(0x1a, 0);       // set GIO[8] to input mode
	WriteRegister(0xa, Reg0xa);   // restore GIO[3:0] setting
      }

      // set the counter frequency
      WriteRegister(0x18, 0xff);
      WriteRegister(0x1a, 1);         // GIO8 as buzzer control output 0
      WriteRegister(0xa, Reg0xa);     // Restore GIO[3:0] setting
    }

    void Buzz(unsigned frequency)
    {
      if (!m_hasBuzzer)
        return;

      if (frequency == 0) {
        WriteRegister(0x1a, 0x1);     // Turn buzzer off
        return;
      }

      int divisor = 46875 / frequency - 1;
      if (divisor < 0)
	divisor = 0;
      else if (divisor > 255)
	divisor = 255;

      WriteRegister(0x18, divisor); // set frequency
      WriteRegister(0x1a, 0x9);     // Turn buzzer on
    }



  public:
    PLUGIN_LID_CTOR()
    {
      m_eProductID = TJIP_NONE;
      m_hasBuzzer = false;
      m_isOffHook = false;
      m_tonePlaying = false;

      m_hWaveIn = NULL;
      m_readFrameSize = 160;

      m_hWaveOut = NULL;
      m_writeFrameSize = 160;

      m_hDLL = LoadLibrary("TjIpSys.dll");
      if (m_hDLL== NULL)
        return;

      m_pTjIpSysCall = (TJIPSYSCALL)GetProcAddress(m_hDLL, "TjIpSysCall");
      if (m_pTjIpSysCall == NULL)
        return;
      
      if (!m_pTjIpSysCall(TJIP_SYS_OPEN, (int)g_hInstance, &m_eProductID))
        return;

      TJ_CALLBACK_INFO info;
      info.fpCallback = StaticKeyCallback;
      info.context = this;
      m_pTjIpSysCall(TJIP_SET_CALLBACK, 0, &info);
    }

    PLUGIN_LID_DTOR()
    {
      Close();

      if (m_eProductID != TJIP_NONE)
        m_pTjIpSysCall(TJIP_SYS_CLOSE, 0, NULL);

      if (m_hDLL != NULL)
        FreeLibrary(m_hDLL);
    }


    PLUGIN_FUNCTION_ARG3(GetDeviceName,unsigned,index, char *,buffer, unsigned,bufsize)
    {
      if (buffer == NULL || bufsize == 0)
        return PluginLID_InvalidParameter;

      if (m_hDLL== NULL)
        return PluginLID_InternalError;

      if (index > 0)
        return PluginLID_NoMoreNames;

      const char * searchName;
      switch (m_eProductID)
      {
        case TJIP_NONE :
          return PluginLID_NoSuchDevice;

	case TJIP_TJ560BPPG :
	case TJIP_TJ560BPPG_NO_EC :
	case TJIP_TJ560CPPG_NO_EC :
	case TJIP_TJ560BPPGPROSLIC :
	case TJIP_TJ560BPPGPROSLIC_NO_EC :
          searchName = "Personal PhoneGateway-USB";
          break;

        case TJIP_TJ320PHONE :
          searchName = "PCI Internet Phone";
          break;

        default :
          searchName = "USB Audio";
      }

      for (UINT id = 0; id < waveInGetNumDevs(); id++) {
        WAVEINCAPS caps;
        if (waveInGetDevCaps(id, &caps, sizeof(caps)) == 0 && _strnicmp(caps.szPname, searchName, strlen(searchName)) == 0) {
          if (bufsize <= strlen(caps.szPname))
            return PluginLID_BufferTooSmall;

          strcpy(buffer, caps.szPname);
          return PluginLID_NoError;
        }
      }

      return PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      switch (m_eProductID)
      {
        case TJIP_TJ560BHANDSET_KEYPAD_HID :
	  WriteRegister(0x0b, 100);	// default is 48, now set to 100 ==> period = 100/2 = 50ms

          InitBuzzer();

	  BYTE reg12, reg13;
	  ReadRegister(0x12, &reg12);
	  WriteRegister(0x12, reg12 & 0xdf);    // set AUX5 to low
	  ReadRegister(0x13, &reg13);
	  WriteRegister(0x13, reg13 | 0x20);    // set AUX5 to output
	  WriteRegister(0x12, reg12);           // restore original AUX data value
	  WriteRegister(0x13, reg13);           // restore original AUX control

	  WriteRegister(0xc, 0);
	  WriteRegister(0xd, 0);
	  WriteRegister(0xe, 0);
          break;
      }

      if (!m_pTjIpSysCall(TJIP_INIT_DTMF_TO_PPG, 0, (void *)device))
        OutputDebugString("Could not initialise DTMF on TigerJet\n");

      TJ_MIXER_OPEN tjMixerOpen;
      memset(&tjMixerOpen, 0, sizeof(tjMixerOpen));
      strncpy(tjMixerOpen.szMixerName, device, sizeof(tjMixerOpen.szMixerName)-1);

      if (!m_pTjIpSysCall(TJIP_OPEN_MIXER, 0, &tjMixerOpen))
        OutputDebugString("Could not initialise mixer on TigerJet\n");


      WAVEFORMATEX format;
      format.wFormatTag = WAVE_FORMAT_PCM;
      format.nChannels = 1;
      format.nSamplesPerSec = 8000;
      format.wBitsPerSample = 16;
      format.nBlockAlign = 2;
      format.nAvgBytesPerSec = 16000;
      format.cbSize = 0;

      UINT id;
      for (id = 0; id < waveInGetNumDevs(); id++) {
        WAVEINCAPS caps;
        if (waveInGetDevCaps(id, &caps, sizeof(caps)) == 0 && _stricmp(caps.szPname, device) == 0) {
          waveInOpen(&m_hWaveIn, id, &format, m_InEvent.GetHandle(), 0, CALLBACK_EVENT);
          break;
        }
      }
      if (m_hWaveIn == NULL) {
        Close();
        return PluginLID_InternalError;
      }

      for (id = 0; id < waveInGetNumDevs(); id++) {
        WAVEOUTCAPS caps;
        if (waveOutGetDevCaps(id, &caps, sizeof(caps)) == 0 && _stricmp(caps.szPname, device) == 0) {
          waveOutOpen(&m_hWaveOut, id, &format, m_OutEvent.GetHandle(), 0, CALLBACK_EVENT);
          break;
        }
      }
      if (m_hWaveOut == NULL) {
        Close();
        return PluginLID_InternalError;
      }

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG0(Close)
    {
      StopReading(0);
      StopWriting(0);

      if (m_hWaveOut != NULL) {
        while (waveOutClose(m_hWaveOut) == WAVERR_STILLPLAYING)
          waveOutReset(m_hWaveOut);
        m_hWaveOut = NULL;
      }

      if (m_hWaveIn != NULL) {
        while (waveInClose(m_hWaveIn) == WAVERR_STILLPLAYING)
          waveInReset(m_hWaveIn);
        m_hWaveIn = NULL;
      }

      if (!m_pTjIpSysCall(TJIP_CLOSE_MIXER, 0, NULL))
        OutputDebugString("Could not close mixer on TigerJet\n");

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(GetLineCount, unsigned *,count)
    {
      if (count == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      *count = 1;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineTerminal, unsigned,line, PluginLID_Boolean *,isTerminal)
    {
      if (isTerminal == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *isTerminal = TRUE;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(IsLinePresent, unsigned,line, PluginLID_Boolean,forceTest, PluginLID_Boolean *,present)
    {
      if (present == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *present = m_pTjIpSysCall(TJIP_CHECK_IS_TJ560DEVICE_PLUGGED, 0, NULL);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *offHook = m_isOffHook;
      return PluginLID_NoError;
    }


    //PLUGIN_FUNCTION_ARG2(SetLineOffHook, unsigned,line, PluginLID_Boolean,newState)
    //PLUGIN_FUNCTION_ARG2(HookFlash, unsigned,line, unsigned,flashTime)
    //PLUGIN_FUNCTION_ARG2(HasHookFlash, unsigned,line, PluginLID_Boolean *,flashed)
    //PLUGIN_FUNCTION_ARG2(IsLineRinging, unsigned,line, unsigned long *,cadence)

    PLUGIN_FUNCTION_ARG3(RingLine, unsigned,line, unsigned,nCadence, unsigned *,pattern)
    {
      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      Buzz(nCadence == 0 ? 0 : 800);

      return PluginLID_NoError;
    }

    //PLUGIN_FUNCTION_ARG3(IsLineConnected, unsigned,line, PluginLID_Boolean,checkForWink, PluginLID_Boolean *,connected)
    //PLUGIN_FUNCTION_ARG3(SetLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean,connect)
    //PLUGIN_FUNCTION_ARG3(IsLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean *,connected)


    PLUGIN_FUNCTION_ARG3(GetSupportedFormat, unsigned,index, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (index >= 1)
        return PluginLID_NoMoreNames;

      if (size < sizeof(g_PCM16))
        return PluginLID_BufferTooSmall;

      strcpy(mediaFormat, g_PCM16);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFormat, unsigned,line, const char *,mediaFormat)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (strcmp(mediaFormat, g_PCM16) != 0)
        return PluginLID_UnsupportedMediaFormat;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFormat, unsigned,line, const char *,mediaFormat)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (strcmp(mediaFormat, g_PCM16) != 0)
        return PluginLID_UnsupportedMediaFormat;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(GetReadFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (size < sizeof(g_PCM16))
        return PluginLID_BufferTooSmall;

      strcpy(mediaFormat, g_PCM16);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(GetWriteFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (size < sizeof(g_PCM16))
        return PluginLID_BufferTooSmall;

      strcpy(mediaFormat, g_PCM16);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopReading, unsigned,line)
    {
      if (m_eProductID == TJIP_NONE || m_hWaveIn == NULL)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      waveInReset(m_hWaveIn);

      m_InMutex.Enter();

      while (m_InBuffers.size() > 0) {
        WAVEHDR & header = m_InBuffers.front();

        while (waveInUnprepareHeader(m_hWaveIn, &header, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
          waveInReset(m_hWaveIn);

        free(header.lpData);
        m_InBuffers.pop();
      }

      // Signal any threads waiting on this event, they should then check
      // the bufferByteOffset variable for an abort.
      m_InEvent.Set();

      m_InMutex.Leave();

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopWriting, unsigned,line)
    {
      if (m_eProductID == TJIP_NONE || m_hWaveOut == NULL)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      waveOutReset(m_hWaveOut);

      m_OutMutex.Enter();

      while (m_OutBuffers.size() > 0) {
        WAVEHDR & header = m_OutBuffers.front();

        while (waveOutUnprepareHeader(m_hWaveOut, &header, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
          waveOutReset(m_hWaveOut);

        free(header.lpData);
        m_OutBuffers.pop();
      }

      // Signal any threads waiting on this event, they should then check
      // the bufferByteOffset variable for an abort.
      m_OutEvent.Set();

      m_OutMutex.Leave();

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (frameSize < 8)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (m_readFrameSize != frameSize) {
        StopReading(line);
        m_readFrameSize = frameSize;
      }

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (frameSize < 8)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (m_writeFrameSize != frameSize) {
        StopWriting(line);
        m_writeFrameSize = frameSize;
      }

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetReadFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *frameSize = m_readFrameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetWriteFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *frameSize = m_writeFrameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(ReadFrame, unsigned,line, void *,buffer, unsigned *,count)
    {
      if (buffer == NULL || count == NULL)
        return PluginLID_InvalidParameter;

      if (m_hWaveIn == NULL)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      bool ok = true;

      m_InMutex.Enter();

      if (m_InBuffers.size() == 0) {
        int bufferCount = 2000/m_readFrameSize; // Want about 250ms of buffering
        if (bufferCount < 2)
          bufferCount = 2; // And at least be double buffered, just doesn't work otherwise
        while (ok && bufferCount-- > 0) {
          static const WAVEHDR EmptyHeader = { 0 };
          m_InBuffers.push(EmptyHeader);
          WAVEHDR & header = m_InBuffers.back();
          header.lpData = (LPSTR)malloc(m_readFrameSize);
          header.dwBufferLength = m_readFrameSize;
          ok = waveInPrepareHeader(m_hWaveIn, &header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR &&
               waveInAddBuffer(m_hWaveIn, &header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR;
        }

        ok = ok && waveInStart(m_hWaveIn) == MMSYSERR_NOERROR; // start recording
      }

      if (ok) {
        WAVEHDR & header = m_InBuffers.front();
        while ((header.dwFlags&WHDR_DONE) == 0 || header.dwBytesRecorded == 0) {
          m_InMutex.Leave();

          if (!m_InEvent.Wait())
            return PluginLID_InternalError;

          // Check for if StopReading() was called.
          if (m_InBuffers.size() == 0)
            return PluginLID_Aborted;

          m_InMutex.Enter();
        }

        *count = header.dwBytesRecorded;
        memcpy(buffer, header.lpData, *count);
        m_InBuffers.push(header);
        m_InBuffers.pop();
        ok = waveInAddBuffer(m_hWaveIn, &m_InBuffers.back(), sizeof(WAVEHDR)) == MMSYSERR_NOERROR;
      }

      m_InMutex.Leave();

      if (ok)
        return PluginLID_NoError;

      StopReading(line);
      return PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG4(WriteFrame, unsigned,line, const void *,buffer, unsigned,count, unsigned *,written)
    {
      if (buffer == NULL || written == NULL || count != m_writeFrameSize)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      m_OutMutex.Enter();

      if (m_OutBuffers.size() == 0) {
        int bufferCount = 1000/m_writeFrameSize; // Want about 125ms of buffering
        if (bufferCount < 2)
          bufferCount = 2; // And at least be double buffered, just doesn't work otherwise
        while (bufferCount-- > 0) {
          static const WAVEHDR EmptyHeader = { 0 };
          m_OutBuffers.push(EmptyHeader);
          WAVEHDR & header = m_OutBuffers.back();
          header.lpData = (LPSTR)malloc(m_writeFrameSize);
          header.dwBufferLength = m_writeFrameSize;
          header.dwFlags = WHDR_DONE;
        }
      }

      while ((m_OutBuffers.front().dwFlags&WHDR_DONE) == 0) {
        m_OutMutex.Leave();

        // No free buffers, so wait for one
        if (!m_OutEvent.Wait())
          return PluginLID_InternalError;

        m_OutMutex.Enter();
      }

      m_OutBuffers.push(m_OutBuffers.front());
      m_OutBuffers.pop();

      WAVEHDR & header = m_OutBuffers.back();
      memcpy(header.lpData, buffer, count);
      *written = count;

      bool ok = waveOutPrepareHeader(m_hWaveOut, &header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR &&
                waveOutWrite(m_hWaveOut, &header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR;

      m_OutMutex.Leave();

      if (ok)
        return PluginLID_NoError;

      StopWriting(line);
      return PluginLID_InternalError;
    }



    //PLUGIN_FUNCTION_ARG3(GetAverageSignalLevel, unsigned,line, PluginLID_Boolean,playback, unsigned *,signal)
    //PLUGIN_FUNCTION_ARG2(EnableAudio, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG2(IsAudioEnabled, unsigned,line, PluginLID_Boolean *,enable)


    PLUGIN_FUNCTION_ARG2(SetRecordVolume, unsigned,line, unsigned,volume)
    {
      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      DWORD dwVolume = volume*32768/100;
      if (dwVolume > 32767)
        dwVolume = 32767;
      return m_pTjIpSysCall(TJIP_SET_WAVEIN_VOL, 0, (void *)dwVolume) ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(SetPlayVolume, unsigned,line, unsigned,volume)
    {
      if (volume > 100)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      DWORD dwVolume = volume*32768/100;
      if (dwVolume > 32767)
        dwVolume = 32767;
      return m_pTjIpSysCall(TJIP_SET_WAVEOUT_VOL, 0, (void *)dwVolume) ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(GetRecordVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      DWORD dwVolume;
      if (!m_pTjIpSysCall(TJIP_GET_WAVEIN_VOL, 0, &dwVolume))
        return PluginLID_InternalError;

      *volume = dwVolume*100/32768;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetPlayVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      DWORD dwVolume;
      if (!m_pTjIpSysCall(TJIP_GET_WAVEOUT_VOL, 0, &dwVolume))
        return PluginLID_InternalError;

      *volume = dwVolume*100/32768;
      return PluginLID_NoError;
    }



    //PLUGIN_FUNCTION_ARG2(GetAEC, unsigned,line, unsigned *,level)
    //PLUGIN_FUNCTION_ARG2(SetAEC, unsigned,line, unsigned,level)
    //PLUGIN_FUNCTION_ARG2(GetVAD, unsigned,line, PluginLID_Boolean *,enable)
    //PLUGIN_FUNCTION_ARG2(SetVAD, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG4(GetCallerID, unsigned,line, char *,idString, unsigned,size, PluginLID_Boolean,full)
    //PLUGIN_FUNCTION_ARG2(SetCallerID, unsigned,line, const char *,idString)
    //PLUGIN_FUNCTION_ARG2(SendCallerIDOnCallWaiting, unsigned,line, const char *,idString)
    //PLUGIN_FUNCTION_ARG2(SendVisualMessageWaitingIndicator, unsigned,line, PluginLID_Boolean,on)

    PLUGIN_FUNCTION_ARG4(PlayDTMF, unsigned,line, const char *,digits, unsigned,onTime, unsigned,offTime)
    {
      if (digits == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      while (*digits != '\0') {
        int nTone = 0;
        if (isdigit(*digits))
          nTone = *digits - '0';
        else if (*digits == '*')
          nTone = 10;
        else if (*digits == '#')
          nTone = 11;

        if (nTone > 0) {
          if (!m_pTjIpSysCall(TJIP_SEND_DTMF_TO_PPG, nTone, NULL))
            return PluginLID_InternalError;
        }
        digits++;
      }
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(ReadDTMF, unsigned,line, char *,digit)
    {
      if (digit == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      m_KeyMutex.Enter();

      if (m_Keys.size() == 0)
        *digit = '\0';
      else {
        *digit = m_Keys.front();
        m_Keys.pop();
      }

      m_KeyMutex.Leave();

      return PluginLID_NoError;
    }


    //PLUGIN_FUNCTION_ARG2(GetRemoveDTMF, unsigned,line, PluginLID_Boolean *,removeTones)
    //PLUGIN_FUNCTION_ARG2(SetRemoveDTMF, unsigned,line, PluginLID_Boolean,removeTones)
    //PLUGIN_FUNCTION_ARG2(IsToneDetected, unsigned,line, unsigned *,tone)
    //PLUGIN_FUNCTION_ARG3(WaitForToneDetect, unsigned,line, unsigned,timeout, unsigned *,tone)
    //PLUGIN_FUNCTION_ARG3(WaitForTone, unsigned,line, unsigned,tone, unsigned,timeout)
    //PLUGIN_FUNCTION_ARG7(SetToneFilterParameters, unsigned        ,line,
    //                                              unsigned        ,tone,
    //                                              unsigned        ,lowFrequency,
    //                                              unsigned        ,highFrequency,
    //                                              unsigned        ,numCadences,
    //                                              const unsigned *,onTimes,
    //                                              const unsigned *,offTimes)
    PLUGIN_FUNCTION_ARG2(PlayTone, unsigned,line, unsigned,tone)
    {
      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      int nTone = 0;
      switch (tone) {
        case PluginLID_RingTone :
          nTone = 101;
          break;
        case PluginLID_BusyTone :
          nTone = 102;
          break;
        case PluginLID_DialTone :
          nTone = 201;
          break;
        case PluginLID_FastBusyTone :
          nTone = 202;
          break;
        case PluginLID_ClearTone :
          nTone = 203;
          break;
      }

      if (nTone > 0) {
        if (!m_pTjIpSysCall(TJIP_SEND_DTMF_TO_PPG, nTone, NULL))
          return PluginLID_InternalError;
        m_tonePlaying = true;
      }
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(IsTonePlaying, unsigned,line, PluginLID_Boolean *,playing)
    {
      if (playing == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *playing = m_tonePlaying;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG1(StopTone, unsigned,line)
    {
      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      m_tonePlaying = false;
      return m_pTjIpSysCall(TJIP_SEND_DTMF_TO_PPG, -1, NULL) ? PluginLID_NoError : PluginLID_InternalError;
    }

    //PLUGIN_FUNCTION_ARG4(DialOut, unsigned,line, const char *,number, PluginLID_Boolean,requireTones, unsigned,uiDialDelay)
    //PLUGIN_FUNCTION_ARG2(GetWinkDuration, unsigned,line, unsigned *,winkDuration)
    //PLUGIN_FUNCTION_ARG2(SetWinkDuration, unsigned,line, unsigned,winkDuration)
    //PLUGIN_FUNCTION_ARG1(SetCountryCode, unsigned,country)
    //PLUGIN_FUNCTION_ARG2(GetSupportedCountry, unsigned,index, unsigned *,countryCode)
};


/////////////////////////////////////////////////////////////////////////////

static struct PluginLID_Definition definition[1] =
{

  { 
    // encoder
    PLUGIN_LID_VERSION,               // API version

    1083666706,                       // timestamp = Tue 04 May 2004 10:31:46 AM UTC = 

    "TigerJet",                       // LID name text
    "TigerJet Network Tiger560",      // LID description text
    "TigerJet Network, Inc",          // LID manufacturer name
    "Tiger560",                       // LID model name
    "1.0",                            // LID hardware revision number
    "info@tjnet.com",                 // LID email contact information
    "http://www.tjnet,com",           // LID web site

    "Robert Jongbloed, Post Increment",                          // source code author
    "robertj@postincrement.com",                                 // source code email
    "http://www.postincrement.com",                              // source code URL
    "Copyright (C) 2006 by Post Increment, All Rights Reserved", // source code copyright
    "MPL 1.0",                                                   // source code license
    "1.0",                                                       // source code version

    NULL,                             // user data value

    Context::Create,
    Context::Destroy,
    Context::GetDeviceName,
    Context::Open,
    Context::Close,
    Context::GetLineCount,
    Context::IsLineTerminal,
    Context::IsLinePresent,
    Context::IsLineOffHook,
    NULL,//Context::SetLineOffHook,
    NULL,//Context::HookFlash,
    NULL,//Context::HasHookFlash,
    NULL,//Context::IsLineRinging,
    Context::RingLine,
    NULL,//Context::IsLineConnected,
    NULL,//Context::SetLineToLineDirect,
    NULL,//Context::IsLineToLineDirect,
    Context::GetSupportedFormat,
    Context::SetReadFormat,
    Context::SetWriteFormat,
    Context::GetReadFormat,
    Context::GetWriteFormat,
    Context::StopReading,
    Context::StopWriting,
    Context::SetReadFrameSize,
    Context::SetWriteFrameSize,
    Context::GetReadFrameSize,
    Context::GetWriteFrameSize,
    Context::ReadFrame,
    Context::WriteFrame,
    NULL,//Context::GetAverageSignalLevel,
    NULL,//Context::EnableAudio,
    NULL,//Context::IsAudioEnabled,
    Context::SetRecordVolume,
    Context::SetPlayVolume,
    Context::GetRecordVolume,
    Context::GetPlayVolume,
    NULL,//Context::GetAEC,
    NULL,//Context::SetAEC,
    NULL,//Context::GetVAD,
    NULL,//Context::SetVAD,
    NULL,//Context::GetCallerID,
    NULL,//Context::SetCallerID,
    NULL,//Context::SendCallerIDOnCallWaiting,
    NULL,//Context::SendVisualMessageWaitingIndicator,
    Context::PlayDTMF,
    Context::ReadDTMF,
    NULL,//Context::GetRemoveDTMF,
    NULL,//Context::SetRemoveDTMF,
    NULL,//Context::IsToneDetected,
    NULL,//Context::WaitForToneDetect,
    NULL,//Context::WaitForTone,
    NULL,//Context::SetToneFilterParameters,
    Context::PlayTone,
    Context::IsTonePlaying,
    Context::StopTone,
    NULL,//Context::DialOut,
    NULL,//Context::GetWinkDuration,
    NULL,//Context::SetWinkDuration,
    NULL,//Context::SetCountryCode,
    NULL,//Context::GetSupportedCountry
  }
};


PLUGIN_LID_IMPLEMENTATION(definition);


/////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpArg)
{
  if (fdwReason == DLL_PROCESS_ATTACH) {
    g_hInstance = hinstDLL;
  }
  return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
