/*
 * Generic USB HID Plugin LID for OPAL
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
 * $Log: usb_win.cpp,v $
 * Revision 1.6  2006/11/05 05:04:46  rjongbloed
 * Improved the terminal LID line ringing, epecially for country emulation.
 *
 * Revision 1.5  2006/10/28 00:45:35  rjongbloed
 * Major change so that sound card based LIDs, eg USB handsets. are handled in
 *   common code so not requiring lots of duplication.
 *
 * Revision 1.4  2006/10/25 22:26:15  rjongbloed
 * Changed LID tone handling to use new tone generation for accurate country based tones.
 *
 * Revision 1.3  2006/10/16 09:46:49  rjongbloed
 * Fixed various MSVC 8 warnings
 *
 * Revision 1.2  2006/10/04 22:30:23  rjongbloed
 * Fixed background thread start up and shut down
 *
 * Revision 1.1  2006/10/02 13:30:53  rjongbloed
 * Added LID plug ins
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

#include "CM_HID.h"


// SubClass Values
#define WM_HID_DEV_ADDED      WM_USER+0x1000
#define WM_HID_DEV_REMOVED    WM_USER+0x1001
#define WM_HID_KEY_DOWN       WM_USER+0x1002
#define WM_HID_KEY_UP         WM_USER+0x1003
#define WM_HID_VOLUME_DOWN    WM_USER+0x1004
#define WM_HID_VOLUME_UP      WM_USER+0x1005
#define WM_HID_PLAYBACK_MUTE  WM_USER+0x1006
#define WM_HID_RECORD_MUTE    WM_USER+0x1007
#define WM_HID_SHUTDOWN       WM_USER+0x1008

// Key Input Mask
static enum Input {
  None		= 0x0000,
  KeyPadMask    = 0x0010,
  Key0		= 0x0010,
  Key1		= 0x0011,
  Key2		= 0x0012,
  Key3		= 0x0013,
  Key4		= 0x0014,
  Key5		= 0x0015,
  Key6		= 0x0016,
  Key7		= 0x0017,
  Key8		= 0x0018,
  Key9		= 0x0019,
  KeyStar	= 0x001a,   // '*' character
  KeyHash	= 0x001b,   // '#' character
  KeyA		= 0x001c,   // (USB) Dial Button 
  KeyB		= 0x001d,   // (USB) End Call Button 
  KeyC		= 0x001e,   // (USB) Left Menu Navigator key 
  KeyD		= 0x001f,   // (USB) Right Menu Navigator key 

  HookMask	= 0x0020,
  OffHook	= 0x0021,   // Hook State (OffHook) N/A for Cell Type
  OnHook	= 0x0022,   // Hook State (OnHook) N/A for Cell Type

  RingMask	= 0x0030,
  StartRing	= 0x0031,   // Start Ringing the device
  StopRing	= 0x0032,   // Stop Ringing the device

  VolumeMask	= 0x0040,
  VolumeUp	= 0x0040,   // Volume Up Key pressed
  VolumeDown	= 0x0041,   // Volume Down key presses
  SetRecVol	= 0x0042,	// Set the Record Volume 
  GetRecVol	= 0x0043,	// Get Record Volume 
  SetPlayVol    = 0x0044,   // Set Play Volume
  GetPlayVol	= 0x0045,   // Get Play Volume

  StateMask	= 0x0050,
  PluggedIn	= 0x0050,   // Device is pluggedIn
  Unplugged	= 0x0051,   // Device is unplugged

  FunctionMask  = 0x0060,   // Special Function Mark
  ClearDisplay  = 0x0061,	// Clear the digit buffer
  Redial	= 0x0062,	// Redial Button
  UpButton	= 0x0063,	// General Up button
  DownButton	= 0x0064,	// General Down button

};

static HINSTANCE g_hInstance;


/////////////////////////////////////////////////////////////////////////////

class Context
{
  protected:
    HANDLE m_hThread;
    DWORD  m_threadId;
    HANDLE m_hStartedEvent;
    HWND   m_hWnd;

    enum { MaxQueueLen = 20 };
    Input  m_queue[MaxQueueLen];
    int    m_queuePos;
    int    m_queueLen;
    CRITICAL_SECTION m_mutex;

    PBoolean   m_isOffHook;

    int    m_readFrameSize;
    int    m_writeFrameSize;

  public:
    PLUGIN_LID_CTOR()
    {
      m_hThread= NULL;
      m_threadId = 0;
      m_hStartedEvent = NULL;
      m_hWnd = NULL;

      m_queuePos = m_queueLen = 0;
      InitializeCriticalSection(&m_mutex);

      m_isOffHook = PFalse;
      m_readFrameSize = 1;
      m_writeFrameSize = 1;
    }

    PLUGIN_LID_DTOR()
    {
      Close();
      DeleteCriticalSection(&m_mutex);
    }


    PLUGIN_FUNCTION_ARG3(GetDeviceName,unsigned,index, char *,buffer, unsigned,bufsize)
    {
      if (buffer == NULL || bufsize < 4)
        return PluginLID_InvalidParameter;

      if (index >= 1)
        return PluginLID_NoMoreNames;

      UINT numDevs = mixerGetNumDevs();
      for(UINT i = 0; i < numDevs; i++){
        MIXERCAPS caps;
	mixerGetDevCaps(i, &caps, sizeof(caps));
        if (strstr(caps.szPname, "USB Audio") != NULL) {
          strcpy(buffer, "HID");
          return PluginLID_NoError;
        }
      }

      return PluginLID_NoMoreNames;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      m_hStartedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
      if (m_hStartedEvent == NULL)
        return PluginLID_DeviceOpenFailed;

      m_hThread = (HANDLE)_beginthread(ThreadMain, 0, this);
      if (m_hThread == NULL)
        return PluginLID_DeviceOpenFailed;

      if (WaitForSingleObject(m_hStartedEvent, 5000) == WAIT_OBJECT_0)
        return PluginLID_NoError;

      Close();
      return PluginLID_DeviceOpenFailed;
    }


    PLUGIN_FUNCTION_ARG0(Close)
    {
      if (m_hThread != NULL) {
        SendMessage(m_hWnd, WM_HID_SHUTDOWN, 0, 0);
        if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
          TerminateThread(m_hThread, -1);
      }

      if (m_hStartedEvent != NULL)
        CloseHandle(m_hStartedEvent);

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(GetLineCount, unsigned *,count)
    {
      if (count == NULL)
        return PluginLID_InvalidParameter;

      if (m_hWnd == 0)
        return PluginLID_DeviceNotOpen;

      *count = 1;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineTerminal, unsigned,line, PluginLID_Boolean *,isTerminal)
    {
      if (isTerminal == NULL)
        return PluginLID_InvalidParameter;

      if (m_hWnd == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *isTerminal = PFalse;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(IsLinePresent, unsigned,line, PluginLID_Boolean,forceTest, PluginLID_Boolean *,present)
    {
      if (present == NULL)
        return PluginLID_InvalidParameter;

      if (m_hWnd == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *present = PTrue;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (m_hWnd == 0)
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
    //PLUGIN_FUNCTION_ARG4(RingLine, unsigned,line, unsigned,nCadence, const unsigned *,pattern, unsigned,frequency)
    //PLUGIN_FUNCTION_ARG3(IsLineConnected, unsigned,line, PluginLID_Boolean,checkForWink, PluginLID_Boolean *,connected)
    //PLUGIN_FUNCTION_ARG3(SetLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean,connect)
    //PLUGIN_FUNCTION_ARG3(IsLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean *,connected)
    //PLUGIN_FUNCTION_ARG3(GetSupportedFormat, unsigned,index, char *,mediaFormat, unsigned,size)
    //PLUGIN_FUNCTION_ARG2(SetReadFormat, unsigned,line, const char *,mediaFormat)
    //PLUGIN_FUNCTION_ARG2(SetWriteFormat, unsigned,line, const char *,mediaFormat)
    //PLUGIN_FUNCTION_ARG3(GetReadFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    //PLUGIN_FUNCTION_ARG3(GetWriteFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    //PLUGIN_FUNCTION_ARG1(StopReading, unsigned,line)
    //PLUGIN_FUNCTION_ARG1(StopWriting, unsigned,line)
    //PLUGIN_FUNCTION_ARG2(SetReadFrameSize, unsigned,line, unsigned,frameSize)
    //PLUGIN_FUNCTION_ARG2(SetWriteFrameSize, unsigned,line, unsigned,frameSize)
    //PLUGIN_FUNCTION_ARG2(GetReadFrameSize, unsigned,line, unsigned *,frameSize)
    //PLUGIN_FUNCTION_ARG2(GetWriteFrameSize, unsigned,line, unsigned *,frameSize)
    //PLUGIN_FUNCTION_ARG3(ReadFrame, unsigned,line, void *,buffer, unsigned *,count)
    //PLUGIN_FUNCTION_ARG4(WriteFrame, unsigned,line, const void *,buffer, unsigned,count, unsigned *,written)
    //PLUGIN_FUNCTION_ARG3(GetAverageSignalLevel, unsigned,line, PluginLID_Boolean,playback, unsigned *,signal)
    //PLUGIN_FUNCTION_ARG2(EnableAudio, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG2(IsAudioEnabled, unsigned,line, PluginLID_Boolean *,enable)
    //PLUGIN_FUNCTION_ARG2(SetRecordVolume, unsigned,line, unsigned,volume)
    //PLUGIN_FUNCTION_ARG2(SetPlayVolume, unsigned,line, unsigned,volume)
    //PLUGIN_FUNCTION_ARG2(GetRecordVolume, unsigned,line, unsigned *,volume)
    //PLUGIN_FUNCTION_ARG2(GetPlayVolume, unsigned,line, unsigned *,volume)
    //PLUGIN_FUNCTION_ARG2(GetAEC, unsigned,line, unsigned *,level)
    //PLUGIN_FUNCTION_ARG2(SetAEC, unsigned,line, unsigned,level)
    //PLUGIN_FUNCTION_ARG2(GetVAD, unsigned,line, PluginLID_Boolean *,enable)
    //PLUGIN_FUNCTION_ARG2(SetVAD, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG4(GetCallerID, unsigned,line, char *,idString, unsigned,size, PluginLID_Boolean,full)
    //PLUGIN_FUNCTION_ARG2(SetCallerID, unsigned,line, const char *,idString)
    //PLUGIN_FUNCTION_ARG2(SendCallerIDOnCallWaiting, unsigned,line, const char *,idString)
    //PLUGIN_FUNCTION_ARG2(SendVisualMessageWaitingIndicator, unsigned,line, PluginLID_Boolean,on)
    //PLUGIN_FUNCTION_ARG4(PlayDTMF, unsigned,line, const char *,digits, unsigned,onTime, unsigned,offTime)


    PLUGIN_FUNCTION_ARG2(ReadDTMF, unsigned,line, char *,digit)
    {
      return PluginLID_NoError;
    }


    //PLUGIN_FUNCTION_ARG2(GetRemoveDTMF, unsigned,line, PluginLID_Boolean *,removeTones)
    //PLUGIN_FUNCTION_ARG2(SetRemoveDTMF, unsigned,line, PluginLID_Boolean,removeTones)
    //PLUGIN_FUNCTION_ARG2(IsToneDetected, unsigned,line, int *,tone)
    //PLUGIN_FUNCTION_ARG3(WaitForToneDetect, unsigned,line, unsigned,timeout, int *,tone)
    //PLUGIN_FUNCTION_ARG3(WaitForTone, unsigned,line, int,tone, unsigned,timeout)
    //PLUGIN_FUNCTION_ARG7(SetToneFilterParameters, unsigned        ,line,
    //                                              unsigned        ,tone,
    //                                              unsigned        ,lowFrequency,
    //                                              unsigned        ,highFrequency,
    //                                              unsigned        ,numCadences,
    //                                              const unsigned *,onTimes,
    //                                              const unsigned *,offTimes)
    //PLUGIN_FUNCTION_ARG2(PlayTone, unsigned,line, unsigned,tone)
    //PLUGIN_FUNCTION_ARG2(IsTonePlaying, unsigned,line, PluginLID_Boolean *,playing)
    //PLUGIN_FUNCTION_ARG1(StopTone, unsigned,line)
    //PLUGIN_FUNCTION_ARG4(DialOut, unsigned,line, const char *,number, PluginLID_Boolean,requireTones, unsigned,uiDialDelay)
    //PLUGIN_FUNCTION_ARG2(GetWinkDuration, unsigned,line, unsigned *,winkDuration)
    //PLUGIN_FUNCTION_ARG2(SetWinkDuration, unsigned,line, unsigned,winkDuration)
    //PLUGIN_FUNCTION_ARG1(SetCountryCode, unsigned,country)
    //PLUGIN_FUNCTION_ARG2(GetSupportedCountry, unsigned,index, unsigned *,countryCode)

  protected:
    static void ThreadMain(void * arg)
    {
      ((Context *)arg)->Main();
    }

    static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
      Context * context = (Context *)GetWindowLong(hWnd, 0);
      if (context != NULL)
        context->HandleMsg(message, wParam, lParam);
      return DefWindowProc(hWnd, message, wParam, lParam);
    }

    void Main()
    {
      m_threadId = GetCurrentThreadId();

      static const char WndClassName[] = "CM_HID";	 
      static PBoolean registered = PFalse;
      if (!registered) {
        // Register the main window class. 
        WNDCLASS wc;
        wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = (WNDPROC)WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(this);
        wc.hInstance = g_hInstance;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName =  NULL;
        wc.lpszClassName = WndClassName;

        registered = RegisterClass(&wc) != 0;
      }

      m_hWnd = CreateWindowEx(0, WndClassName, WndClassName,
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 200, 150,
                              0, 0, g_hInstance, 0);
      if (m_hWnd == NULL) {
        SetEvent(m_hStartedEvent);
        return;
      }

      SetWindowLong(m_hWnd, 0, (LONG)this);
      m_queuePos = m_queueLen = 0;

      int result = StartDeviceDetection(m_hWnd,
                                        WM_HID_DEV_ADDED,
                                        WM_HID_DEV_REMOVED,
                                        WM_HID_KEY_DOWN,
                                        WM_HID_KEY_UP,
                                        WM_HID_VOLUME_DOWN,
                                        WM_HID_VOLUME_UP,
                                        WM_HID_PLAYBACK_MUTE,
                                        WM_HID_RECORD_MUTE);

      SetEvent(m_hStartedEvent);

      MSG msg;
      for (;;) {
        switch (GetMessage(&msg, NULL, 0, 0)) {
          case -1 :
          case 0 :
            return;
          default :
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
      }
    }

    void Enqueue(Input flags)
    {
      EnterCriticalSection(&m_mutex);
      m_queue[(m_queuePos+m_queueLen)%MaxQueueLen] = flags;
      m_queueLen++;
      LeaveCriticalSection(&m_mutex);
    }

    void HandleMsg(UINT message, WPARAM wParam, LPARAM lParam)
    {
      switch (message) {
        case WM_HID_DEV_ADDED: 
          Enqueue(PluggedIn);
          StartKeyScan();
          break;

        case WM_HID_DEV_REMOVED: 
          Enqueue(Unplugged);
          StopKeyScan();
          break;

        case WM_HID_KEY_DOWN: 
          switch (wParam) {
            case 1:
              Enqueue(Key1);
              break;
            case 2:
              Enqueue(Key4);
              break;
            case 3:
              Enqueue(Key7);
              break;
            case 4:
              Enqueue(KeyStar);  // Star key
              break;
            case 5:
              Enqueue(Key2);
              break;
            case 6:
              Enqueue(Key5);
              break;
            case 7:
              Enqueue(Key8);
              break;
            case 8:
              Enqueue(Key0);
              break;
            case 9:
              Enqueue(Key3);
              break;
            case 10:
              Enqueue(Key6);
              break;
            case 11:
              Enqueue(Key9);
              break;
            case 12:
              Enqueue(KeyHash);  // Hash Key
              break;
            case 13:
              m_isOffHook = PTrue;  // Dial key
              break;
            case 14:
              m_isOffHook = PFalse;;  // Stop Dial key
              break;
            case 15:
              Enqueue(KeyC);  // left Navigator Keys
              break;
            case 16:
              Enqueue(KeyD);  // Right Navigator Keys
          }
          break;

        case WM_HID_KEY_UP: 
          // Do Nothing
          break;

        case WM_HID_VOLUME_DOWN: 
          if ((int)wParam == 1)
	    Enqueue(VolumeUp);
          else
	    Enqueue(VolumeDown);
          break;

        case WM_HID_VOLUME_UP: 
          // Do Nothing
          break;

        case WM_HID_PLAYBACK_MUTE: 
          // Not Implemented
          break;

        case WM_HID_RECORD_MUTE: 
          // Not Implemented
          break;

        case WM_DEVICECHANGE: 
          HandleUsbDeviceChange(wParam, lParam); 
          break;

        case WM_HID_SHUTDOWN :
          CloseDevice();
          DestroyWindow(m_hWnd);
          break;

        case WM_DESTROY :
          PostQuitMessage(0);
          break;
      }
    }
};


/////////////////////////////////////////////////////////////////////////////

static struct PluginLID_Definition definition[1] =
{

  { 
    // encoder
    PLUGIN_LID_VERSION,               // API version

    1083666706,                       // timestamp = Tue 04 May 2004 10:31:46 AM UTC = 

    "USB-HID",                        // LID name text
    "Generic USB HID",                // LID description text
    "Anyone",                         // LID manufacturer name
    "USB-HID",                        // LID model name
    "1.0",                            // LID hardware revision number
    "",                               // LID email contact information
    "",                               // LID web site

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
    NULL,//Context::RingLine,
    NULL,//Context::IsLineConnected,
    NULL,//Context::SetLineToLineDirect,
    NULL,//Context::IsLineToLineDirect,
    NULL,//Context::GetSupportedFormat,
    NULL,//Context::SetReadFormat,
    NULL,//Context::SetWriteFormat,
    NULL,//Context::GetReadFormat,
    NULL,//Context::GetWriteFormat,
    NULL,//Context::StopReading,
    NULL,//Context::StopWriting,
    NULL,//Context::SetReadFrameSize,
    NULL,//Context::SetWriteFrameSize,
    NULL,//Context::GetReadFrameSize,
    NULL,//Context::GetWriteFrameSize,
    NULL,//Context::ReadFrame,
    NULL,//Context::WriteFrame,
    NULL,//Context::GetAverageSignalLevel,
    NULL,//Context::EnableAudio,
    NULL,//Context::IsAudioEnabled,
    NULL,//Context::SetRecordVolume,
    NULL,//Context::SetPlayVolume,
    NULL,//Context::GetRecordVolume,
    NULL,//Context::GetPlayVolume,
    NULL,//Context::GetAEC,
    NULL,//Context::SetAEC,
    NULL,//Context::GetVAD,
    NULL,//Context::SetVAD,
    NULL,//Context::GetCallerID,
    NULL,//Context::SetCallerID,
    NULL,//Context::SendCallerIDOnCallWaiting,
    NULL,//Context::SendVisualMessageWaitingIndicator,
    NULL,//Context::PlayDTMF,
    Context::ReadDTMF,
    NULL,//Context::GetRemoveDTMF,
    NULL,//Context::SetRemoveDTMF,
    NULL,//Context::IsToneDetected,
    NULL,//Context::WaitForToneDetect,
    NULL,//Context::WaitForTone,
    NULL,//Context::SetToneFilterParameters,
    NULL,//Context::PlayTone,
    NULL,//Context::IsTonePlaying,
    NULL,//Context::StopTone,
    NULL,//Context::DialOut,
    NULL,//Context::GetWinkDuration,
    NULL,//Context::SetWinkDuration,
    NULL,//Context::SetCountryCode,
    NULL,//Context::GetSupportedCountry
  }
};


PLUGIN_LID_IMPLEMENTATION(definition);


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
    g_hInstance = hinstDLL;
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
