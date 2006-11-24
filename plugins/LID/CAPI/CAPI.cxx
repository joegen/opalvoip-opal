/*
 * Common-ISDN API LID for OPAL
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
 * Contributor(s): ______________________________________.
 *
 * $Log: CAPI.cxx,v $
 * Revision 1.1  2006/11/24 09:22:01  rjongbloed
 * Added files for CAPI LID on both Windows and Linux. Still much implementationto be done!
 *
 */

#define _CRT_SECURE_NO_DEPRECATE
#define PLUGIN_DLL_EXPORTS
#include <lids/lidplugin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WINDOWS

  #include "win32/capi2032.h"

  typedef DWORD CAPI_APPLID_T;
  const CAPI_APPLID_T CAPI_APPLID_INVALID = UINT_MAX;

#else

  // Assume Linux
  #include <sys/types.h>
  #include <capi20.h>

  typedef unsigned CAPI_APPLID_T;
  const CAPI_APPLID_T CAPI_APPLID_INVALID = 0xffffffff;

  static inline _cdword CAPI_REGISTER(_cdword MessageBufferSize,
                                      _cdword MaxLogicalConnection,
                                      _cdword MaxBDataBlocks,
                                      _cdword MaxBDataLen,
                                      CAPI_APPLID_T * pApplID)
  {
    return capi20_register(MaxLogicalConnection, MaxBDataBlocks, MaxBDataLen, (unsigned *)pApplID);
  }

  static inline _cdword CAPI_RELEASE(CAPI_APPLID_T ApplID)
  {
    return capi20_release(ApplID);
  }

  static inline _cdword CAPI_PUT_MESSAGE(CAPI_APPLID_T ApplID, void * pCAPIMessage)
  {
    return capi20_put_message(ApplID, (unsigned char *)pCAPIMessage);
  }

  static inline _cdword CAPI_GET_MESSAGE(CAPI_APPLID_T ApplID, void ** ppCAPIMessage)
  {
    return capi20_get_message(ApplID, (unsigned char **)ppCAPIMessage);
  }

  static inline _cdword CAPI_WAIT_FOR_SIGNAL(CAPI_APPLID_T ApplID)
  {
    return capi20_waitformessage(ApplID, NULL);
  }

  static inline void CAPI_GET_MANUFACTURER(char * szBuffer)
  {
    capi20_get_manufacturer(0, (unsigned char *)szBuffer);
  }

  static inline _cdword CAPI_GET_VERSION(_cdword *pCAPIMajor,
                                         _cdword *pCAPIMinor,
                                         _cdword *pManufacturerMajor,
                                         _cdword *pManufacturerMinor)
  {
    unsigned char buffer[4*sizeof(_cdword)];
    if (capi20_get_version(0, buffer) == NULL)
      return 0x100;

    if (pCAPIMajor) *pCAPIMajor = ((_cdword *)buffer)[0];
    if (pCAPIMinor) *pCAPIMinor = ((_cdword *)buffer)[1];
    if (pManufacturerMajor) *pManufacturerMajor = ((_cdword *)buffer)[2];
    if (pManufacturerMinor) *pManufacturerMinor = ((_cdword *)buffer)[3];

    return 0;
  }

  static inline _cdword CAPI_GET_SERIAL_NUMBER(char * szBuffer)
  {
      return capi20_get_serial_number(0, (unsigned char *)szBuffer) == NULL ? 0x100 : 0;
  }

  static inline _cdword CAPI_GET_PROFILE(void * pBuffer, _cdword CtrlNr)
  {
    return capi20_get_profile(CtrlNr, (unsigned char *)pBuffer);
  }

  static inline _cdword CAPI_INSTALLED()
  {
      return capi20_isinstalled();
  }

#endif


// For some reason this is missing from capiutil.h!
struct CAPI_PROFILE {
  _cword  NumControllers;     // # of installed Controllers
  _cword  NumBChannels;       // # of supported B-channels
  _cdword GlobalOpttions;     // Global options
  _cdword B1ProtocolOptions;  // B1 protocol support
  _cdword B2ProtocolOptions;  // B2 protocol support
  _cdword B3ProtocolOptions;  // B3 protocol support
  char szReserved[64];
};


static const char G711ULawMediaFmt[] = "G.711-uLaw-64k";

class Context
{
  protected:
    enum {
        MaxLineCount = 30,
        MaxBlockCount = 2,
        MaxBlockSize = 128
    };
    CAPI_APPLID_T m_ApplicationId;
    unsigned      m_ControllerNumber;
    unsigned      m_LineCount;

    struct LineState
    {
        _cdword   m_PLCI;
        bool      m_OffHook;

        bool SetLineOffHook(bool newState)
        {
            return false;
        }
    } m_LineState[MaxLineCount];

  public:
    PLUGIN_LID_CTOR()
    {
      m_LineCount = 0;
      unsigned result = CAPI_REGISTER(MaxLineCount*MaxBlockCount*MaxBlockSize+1024,
                                      MaxLineCount,
                                      MaxBlockCount,
                                      MaxBlockSize,
                                      &m_ApplicationId);
      if (result != 0)
        m_ApplicationId = CAPI_APPLID_INVALID;
    }

    PLUGIN_LID_DTOR()
    {
      Close();
      if (m_ApplicationId != CAPI_APPLID_INVALID)
        CAPI_RELEASE(m_ApplicationId);
    }


    PLUGIN_FUNCTION_ARG3(GetDeviceName,unsigned,index, char *,name, unsigned,size)
    {
      if (name == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (m_ApplicationId == CAPI_APPLID_INVALID)
        return PluginLID_InternalError;

      CAPI_PROFILE profile;
      if (CAPI_GET_PROFILE(&profile, 0) != 0)
        return PluginLID_InternalError;

      if (index >= profile.NumControllers)
        return PluginLID_NoMoreNames;

      if (size < 3)
        return PluginLID_BufferTooSmall;

      sprintf(name, "%u", index+1);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      if (m_ApplicationId == CAPI_APPLID_INVALID)
        return PluginLID_InternalError;

      m_ControllerNumber = atoi(device);
      if (m_ControllerNumber <= 0)
        return PluginLID_NoSuchDevice;

      CAPI_PROFILE profile;
      if (CAPI_GET_PROFILE(&profile, m_ControllerNumber) != 0)
        return PluginLID_NoSuchDevice;

      m_LineCount = profile.NumBChannels;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG0(Close)
    {
      if (m_LineCount == 0)
        return PluginLID_NoError;

      m_LineCount = 0;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(GetLineCount, unsigned *,count)
    {
      if (count == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      *count = m_LineCount;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineTerminal, unsigned,line, PluginLID_Boolean *,isTerminal)
    {
      if (isTerminal == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *isTerminal = FALSE;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(IsLinePresent, unsigned,line, PluginLID_Boolean,forceTest, PluginLID_Boolean *,present)
    {
      if (present == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *present = TRUE;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *offHook = m_LineState[line].m_OffHook;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetLineOffHook, unsigned,line, PluginLID_Boolean,newState)
    {
      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return m_LineState[line].SetLineOffHook(newState != FALSE) ? PluginLID_NoError : PluginLID_InternalError;
    }


    //PLUGIN_FUNCTION_ARG2(HookFlash, unsigned,line, unsigned,flashTime)
    //PLUGIN_FUNCTION_ARG2(HasHookFlash, unsigned,line, PluginLID_Boolean *,flashed)

    PLUGIN_FUNCTION_ARG2(IsLineRinging, unsigned,line, unsigned long *,cadence)
    {
      if (cadence == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *cadence = 0; // Not ringing

      return PluginLID_NoError;
    }

    //PLUGIN_FUNCTION_ARG4(RingLine, unsigned,line, unsigned,nCadence, const unsigned *,pattern, unsigned,frequency)
    //PLUGIN_FUNCTION_ARG3(IsLineConnected, unsigned,line, PluginLID_Boolean,checkForWink, PluginLID_Boolean *,connected)
    //PLUGIN_FUNCTION_ARG3(SetLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean,connect)
    //PLUGIN_FUNCTION_ARG3(IsLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean *,connected)


    PLUGIN_FUNCTION_ARG3(GetSupportedFormat, unsigned,index, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (index > 0)
        return PluginLID_NoMoreNames;

      if (size < sizeof(G711ULawMediaFmt))
        return PluginLID_BufferTooSmall;

      strcpy(mediaFormat, G711ULawMediaFmt);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFormat, unsigned,line, const char *,mediaFormat)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return strcmp(mediaFormat, G711ULawMediaFmt) == 0 ? PluginLID_NoError : PluginLID_UnsupportedMediaFormat;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFormat, unsigned,line, const char *,mediaFormat)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return strcmp(mediaFormat, G711ULawMediaFmt) == 0 ? PluginLID_NoError : PluginLID_UnsupportedMediaFormat;
    }


    PLUGIN_FUNCTION_ARG3(GetReadFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      if (size < sizeof(G711ULawMediaFmt))
        return PluginLID_BufferTooSmall;

      strcpy(mediaFormat, G711ULawMediaFmt);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(GetWriteFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      if (size < sizeof(G711ULawMediaFmt))
        return PluginLID_BufferTooSmall;

      strcpy(mediaFormat, G711ULawMediaFmt);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopReading, unsigned,line)
    {
      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopWriting, unsigned,line)
    {
      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetReadFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *frameSize = MaxBlockSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetWriteFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *frameSize = MaxBlockSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(ReadFrame, unsigned,line, void *,buffer, unsigned *,count)
    {
      if (buffer == NULL || count == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *count = MaxBlockSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG4(WriteFrame, unsigned,line, const void *,buffer, unsigned,count, unsigned *,written)
    {
      if (buffer == NULL || written == NULL || count != MaxBlockSize)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *written = MaxBlockSize;
      return PluginLID_NoError;
    }



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
    //PLUGIN_FUNCTION_ARG2(ReadDTMF, unsigned,line, char *,digit)
    //PLUGIN_FUNCTION_ARG2(GetRemoveDTMF, unsigned,line, PluginLID_Boolean *,removeTones)
    //PLUGIN_FUNCTION_ARG2(SetRemoveDTMF, unsigned,line, PluginLID_Boolean,removeTones)

    PLUGIN_FUNCTION_ARG2(IsToneDetected, unsigned,line, int *,tone)
    {
      if (tone == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *tone = PluginLID_NoTone;

      return PluginLID_NoError;
    }

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

    PLUGIN_FUNCTION_ARG4(DialOut, unsigned,line, const char *,number, PluginLID_Boolean,requireTones, unsigned,uiDialDelay)
    {
      if (number == NULL)
        return PluginLID_InvalidParameter;

      if (m_LineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }

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

    "CAPI",                           // LID name text
    "Common-ISDN API",                // LID description text
    "Generic",                        // LID manufacturer name
    "Generic",                        // LID model name
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
    Context::SetLineOffHook,
    NULL, //Context::HookFlash,
    NULL, //Context::HasHookFlash,
    Context::IsLineRinging,
    NULL, //Context::RingLine,
    NULL, //Context::IsLineConnected,
    NULL, //Context::SetLineToLineDirect,
    NULL, //Context::IsLineToLineDirect,
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
    NULL,//Context::ReadDTMF,
    NULL,//Context::GetRemoveDTMF,
    NULL,//Context::SetRemoveDTMF,
    NULL,//Context::IsToneDetected,
    NULL,//Context::WaitForToneDetect,
    NULL,//Context::WaitForTone,
    NULL,//Context::SetToneFilterParameters,
    NULL,//Context::PlayTone,
    NULL,//Context::IsTonePlaying,
    NULL,//Context::StopTone,
    Context::DialOut,
    NULL,//Context::GetWinkDuration,
    NULL,//Context::SetWinkDuration,
    NULL,//Context::SetCountryCode,
    NULL,//Context::GetSupportedCountry
  }

};

PLUGIN_LID_IMPLEMENTATION(definition);

/////////////////////////////////////////////////////////////////////////////
