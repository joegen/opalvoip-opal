/*
 * lidplugins.cxx
 *
 * Line Interface Device plugins manager
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2006 Post Increment
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
 * $Log: lidpluginmgr.cxx,v $
 * Revision 1.2003  2006/10/03 01:06:35  rjongbloed
 * Fixed GNU compiler compatibility.
 *
 * Revision 2.1  2006/10/02 13:30:51  rjongbloed
 * Added LID plug ins
 *
 */

#ifdef __GNUC__
#pragma implementation "lidpluginmgr.h"
#endif

#include <ptlib.h>

#include <lids/lidpluginmgr.h>


///////////////////////////////////////////////////////////////////////////////

OpalPluginLIDManager::OpalPluginLIDManager(PPluginManager * mgr)
  : PPluginModuleManager(PLUGIN_LID_GET_LIDS_FN_STR, mgr)
{
  // cause the plugin manager to load all dynamic plugins
  pluginMgr->AddNotifier(PCREATE_NOTIFIER(OnLoadModule), TRUE);
}


OpalPluginLIDManager::~OpalPluginLIDManager()
{
}


void OpalPluginLIDManager::OnLoadPlugin(PDynaLink & dll, INT code)
{
  PluginLID_GetDefinitionsFunction getDefinitions;
  if (!dll.GetFunction(PString(signatureFunctionName), (PDynaLink::Function &)getDefinitions)) {
    PTRACE(3, "LID Plugin\tDLL " << dll.GetName() << " is not a plugin LID");
    return;
  }

  unsigned count;
  PluginLID_Definition * lid = (*getDefinitions)(&count, PWLIB_PLUGIN_API_VERSION);
  if (lid == NULL || count == 0) {
    PTRACE(3, "LID Plugin\tDLL " << dll.GetName() << " contains no LID definitions");
    return;
  } 

  PTRACE(3, "LID Plugin\tDLL " << dll.GetName() << " loaded " << count << "LID" << (count > 1 ? "s" : ""));

  while (count-- > 0) {
    if (lid->name != NULL && *lid->name != '\0') {
      switch (code) {
        case 0 : // plugin loaded
          m_registrations.Append(new OpalPluginLIDRegistration(*lid));
          break;

        case 1 : // plugin unloaded
          for (PINDEX i = 0; i < m_registrations.GetSize(); i++) {
            if (m_registrations[i] == lid->name)
              m_registrations.RemoveAt(i--);
          }
      }
    }
    lid++;
  }
}


void OpalPluginLIDManager::OnShutdown()
{
  m_registrations.RemoveAll();
}


///////////////////////////////////////////////////////////////////////////////

OpalPluginLIDRegistration::OpalPluginLIDRegistration(const PluginLID_Definition & definition)
  : OpalLIDRegistration(definition.name)
  , m_definition(definition)
{
}


OpalLineInterfaceDevice * OpalPluginLIDRegistration::Create(void *) const
{
  return new OpalPluginLID(m_definition);
}


///////////////////////////////////////////////////////////////////////////////

OpalPluginLID::OpalPluginLID(const PluginLID_Definition & definition)
  : m_definition(definition)
{
  if (m_definition.Create != NULL) {
    m_context = definition.Create(&m_definition);
    PTRACE_IF(1, m_context == NULL, "LID Plugin\tNo context for " << m_definition.description); \
  }
  else {
    m_context = NULL;
    PTRACE(1, "LID Plugin\tDefinition for " << m_definition.description << " invalid.");
  }
}


OpalPluginLID::~OpalPluginLID()
{
  if (m_context != NULL && m_definition.Destroy != NULL)
    m_definition.Destroy(&m_definition, m_context);
}


#if PTRACING

bool OpalPluginLID::CheckContextAndFunction(void * fnPtr, const char * fnName) const
{
  if (m_context == NULL) {
    PTRACE(1, "LID Plugin\tNo context for " << m_definition.name);
    return FALSE;
  }

  if (fnPtr == NULL) {
    PTRACE(1, "LID Plugin\tFunction " << fnName << " not implemented in " << m_definition.name);
    return FALSE;
  }

  return TRUE;
}

bool OpalPluginLID::CheckError(int error, const char * fnName, int ignoredError) const
{
  if (error == PluginLID_NoError)
    return TRUE;

  osError = error;

  if (error != ignoredError)
    PTRACE(2, "LID Plugin\tFunction " << fnName << " in " << m_definition.name << " returned error " << error);

  return FALSE;
}

#define CHECK_FN(fn)                  CheckContextAndFunction((void *)m_definition.fn, #fn)
#define CHECK_FN_ARG(fn, args)       (CheckContextAndFunction((void *)m_definition.fn, #fn) && CheckError(m_definition.fn args, #fn))

#else // PTRACING

#define CHECK_FN(fn) (m_context != NULL && m_definition.fn != NULL)
#define CHECK_FN_ARG(fn, arg) (CHECK_FN(fn) && (osError = m_definition.fn arg) == PluginLID_NoError)

#endif // PTRACING


BOOL OpalPluginLID::Open(const PString & device)
{
  Close();

  if (CHECK_FN_ARG(Open, (m_context, device))) {
    os_handle = 1;
    m_deviceName = device;
    return TRUE;
  }

  return FALSE;
}


BOOL OpalPluginLID::Close()
{
  if (CHECK_FN(Close))
    m_definition.Close(m_context);

  return OpalLineInterfaceDevice::Close();
}


PString OpalPluginLID::GetDeviceType() const
{
  return m_definition.name;
}


PString OpalPluginLID::GetDeviceName() const
{
  return m_deviceName;
}


PStringArray OpalPluginLID::GetAllNames() const
{
  PStringArray devices;

  if (CHECK_FN(GetDeviceName)) {
    char buffer[200];
    unsigned index = 0;
    while ((osError = m_definition.GetDeviceName(m_context, index++, buffer, sizeof(buffer))) == PluginLID_NoError)
      devices.AppendString(buffer);
#if PTRACING
    if (osError != PluginLID_NoMoreNames)
      CheckError(osError, "GetDeviceName");
#endif
  }

  return devices;
}


PString OpalPluginLID::GetDescription() const
{
  return m_definition.description;
}


unsigned OpalPluginLID::GetLineCount()
{
  unsigned count;
  if (CHECK_FN_ARG(GetLineCount, (m_context, &count)))
    return count;

  return 0;
}


BOOL OpalPluginLID::IsLineTerminal(unsigned line)
{
  BOOL isTerminal;
  if (CHECK_FN_ARG(IsLineTerminal, (m_context, line, &isTerminal)))
    return isTerminal;
  return FALSE;
}


BOOL OpalPluginLID::IsLinePresent(unsigned line, BOOL force)
{
  BOOL isPresent;
  if (CHECK_FN_ARG(IsLinePresent, (m_context, line, force, &isPresent)))
    return isPresent;
  return FALSE;
}


BOOL OpalPluginLID::IsLineOffHook(unsigned line)
{
  BOOL offHook;
  if (CHECK_FN_ARG(IsLineOffHook, (m_context, line, &offHook)))
    return offHook;
  return FALSE;
}


BOOL OpalPluginLID::SetLineOffHook(unsigned line, BOOL newState)
{
  return CHECK_FN_ARG(SetLineOffHook, (m_context, line, newState));
}


BOOL OpalPluginLID::HookFlash(unsigned line, unsigned flashTime)
{
  if (m_definition.HookFlash == NULL)
    return OpalLineInterfaceDevice::HookFlash(line, flashTime);

  return CHECK_FN_ARG(HookFlash, (m_context, line, flashTime));
}


BOOL OpalPluginLID::HasHookFlash(unsigned line)
{
  if (m_definition.HasHookFlash == NULL)
    return OpalLineInterfaceDevice::HasHookFlash(line);

  BOOL flashed;
  if (CHECK_FN_ARG(HasHookFlash, (m_context, line, &flashed)))
    return flashed;
  return FALSE;
}


BOOL OpalPluginLID::IsLineRinging(unsigned line, DWORD * cadence)
{
  DWORD localCadence;
  if (cadence == NULL)
    cadence = &localCadence;

  if (CHECK_FN_ARG(IsLineRinging, (m_context, line, (unsigned long *)cadence)))
    return *cadence != 0;

  return FALSE;
}


BOOL OpalPluginLID::RingLine(unsigned line, PINDEX nCadence, unsigned * pattern)
{
  return CHECK_FN_ARG(RingLine, (m_context, line, nCadence, pattern));
}


BOOL OpalPluginLID::IsLineDisconnected(unsigned line, BOOL checkForWink)
{
  if (m_definition.IsLineConnected == NULL)
    return OpalLineInterfaceDevice::IsLineDisconnected(line, checkForWink);

  BOOL disconnected;
  if (CHECK_FN_ARG(IsLineConnected, (m_context, line, checkForWink, &disconnected)))
    return disconnected;
  return FALSE;
}


BOOL OpalPluginLID::SetLineToLineDirect(unsigned line1, unsigned line2, BOOL connect)
{
  return CHECK_FN_ARG(SetLineToLineDirect, (m_context, line1, line2, connect));
}


BOOL OpalPluginLID::IsLineToLineDirect(unsigned line1, unsigned line2)
{
  BOOL connected;
  if (CHECK_FN_ARG(IsLineToLineDirect, (m_context, line1, line2, &connected)))
    return connected;
  return FALSE;
}


OpalMediaFormatList OpalPluginLID::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  if (CHECK_FN(GetSupportedFormat)) {
    char buffer[100];
    unsigned index = 0;
    while ((osError = m_definition.GetSupportedFormat(m_context, index++, buffer, sizeof(buffer))) == PluginLID_NoError) {
      OpalMediaFormat format = buffer;
      if (format.IsEmpty()) {
        PTRACE(2, "LID Plugin\tCodec format \"" << buffer << "\" in " << m_definition.name << " is not supported.");
      }
      else
        formats += format;
    }
#if PTRACING
    if (osError != PluginLID_NoMoreNames)
      CheckError(osError, "GetSupportedFormat");
#endif
  }

  return formats;
}


BOOL OpalPluginLID::SetReadFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  return CHECK_FN_ARG(SetReadFormat, (m_context, line, mediaFormat));
}


BOOL OpalPluginLID::SetWriteFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  return CHECK_FN_ARG(SetWriteFormat, (m_context, line, mediaFormat));
}


OpalMediaFormat OpalPluginLID::GetReadFormat(unsigned line)
{
  char buffer[100];
  if (CHECK_FN_ARG(GetReadFormat, (m_context, line, buffer, sizeof(buffer))))
    return buffer;

  return FALSE;
}


OpalMediaFormat OpalPluginLID::GetWriteFormat(unsigned line)
{
  char buffer[100];
  if (CHECK_FN_ARG(GetWriteFormat, (m_context, line, buffer, sizeof(buffer))))
    return buffer;

  return FALSE;
}


BOOL OpalPluginLID::StopReading(unsigned line)
{
  return CHECK_FN_ARG(StopReading, (m_context, line)) && OpalLineInterfaceDevice::StopReading(line);
}


BOOL OpalPluginLID::StopWriting(unsigned line)
{
  return CHECK_FN_ARG(StopWriting, (m_context, line)) && OpalLineInterfaceDevice::StopWriting(line);
}


BOOL OpalPluginLID::SetReadFrameSize(unsigned line, PINDEX frameSize)
{
  return CHECK_FN_ARG(SetReadFrameSize, (m_context, line, frameSize));
}


BOOL OpalPluginLID::SetWriteFrameSize(unsigned line, PINDEX frameSize)
{
  return CHECK_FN_ARG(SetWriteFrameSize, (m_context, line, frameSize));
}


PINDEX OpalPluginLID::GetReadFrameSize(unsigned line)
{
  unsigned frameSize;
  if (CHECK_FN_ARG(GetReadFrameSize, (m_context, line, &frameSize)))
    return frameSize;
  return 0;
}


PINDEX OpalPluginLID::GetWriteFrameSize(unsigned line)
{
  unsigned frameSize;
  if (CHECK_FN_ARG(GetWriteFrameSize, (m_context, line, &frameSize)))
    return frameSize;
  return 0;
}


BOOL OpalPluginLID::ReadFrame(unsigned line, void * buffer, PINDEX & count)
{
  unsigned uiCount;
  if (CHECK_FN_ARG(ReadFrame, (m_context, line, buffer, &uiCount))) {
    count = uiCount;
    return TRUE;
  }
  return FALSE;
}


BOOL OpalPluginLID::WriteFrame(unsigned line, const void * buffer, PINDEX count, PINDEX & written)
{
  unsigned uiCount;
  if (CHECK_FN_ARG(WriteFrame, (m_context, line, buffer, count, &uiCount))) {
    written = uiCount;
    return TRUE;
  }
  return FALSE;
}


unsigned OpalPluginLID::GetAverageSignalLevel(unsigned line, BOOL playback)
{
  if (m_definition.GetAverageSignalLevel == NULL)
    return OpalLineInterfaceDevice::GetAverageSignalLevel(line, playback);

  unsigned signal;
  if (CHECK_FN_ARG(GetAverageSignalLevel, (m_context, line, playback, &signal)))
    return signal;
  return 0;
}


BOOL OpalPluginLID::EnableAudio(unsigned line, BOOL enable)
{
  if (m_definition.EnableAudio == NULL)
    return OpalLineInterfaceDevice::EnableAudio(line, enable);

  return CHECK_FN_ARG(EnableAudio, (m_context, line, enable));
}


BOOL OpalPluginLID::IsAudioEnabled(unsigned line)
{
  if (m_definition.IsAudioEnabled == NULL)
    return OpalLineInterfaceDevice::IsAudioEnabled(line);

  BOOL enabled;
  if (CHECK_FN_ARG(IsAudioEnabled, (m_context, line, &enabled)))
    return enabled;
  return FALSE;
}


BOOL OpalPluginLID::SetRecordVolume(unsigned line, unsigned volume)
{
  return CHECK_FN_ARG(SetRecordVolume, (m_context, line, volume));
}


BOOL OpalPluginLID::SetPlayVolume(unsigned line, unsigned volume)
{
  return CHECK_FN_ARG(SetPlayVolume, (m_context, line, volume));
}


BOOL OpalPluginLID::GetRecordVolume(unsigned line, unsigned & volume)
{
  return CHECK_FN_ARG(GetRecordVolume, (m_context, line, &volume));
}


BOOL OpalPluginLID::GetPlayVolume(unsigned line, unsigned & volume)
{
  return CHECK_FN_ARG(GetPlayVolume, (m_context, line, &volume));
}


OpalLineInterfaceDevice::AECLevels OpalPluginLID::GetAEC(unsigned line)
{
  unsigned level;
  if (CHECK_FN_ARG(GetAEC, (m_context, line, &level)))
    return (AECLevels)level;
  return AECError;
}


BOOL OpalPluginLID::SetAEC(unsigned line, AECLevels level)
{
  return CHECK_FN_ARG(SetAEC, (m_context, line, level));
}


BOOL OpalPluginLID::GetVAD(unsigned line)
{
  BOOL vad;
  if (CHECK_FN_ARG(GetVAD, (m_context, line, &vad)))
    return vad;
  return FALSE;
}


BOOL OpalPluginLID::SetVAD(unsigned line, BOOL enable)
{
  return CHECK_FN_ARG(SetVAD, (m_context, line, enable));
}


BOOL OpalPluginLID::GetCallerID(unsigned line, PString & idString, BOOL full)
{
  return CHECK_FN_ARG(GetCallerID, (m_context, line, idString.GetPointer(500), 500, full));
}


BOOL OpalPluginLID::SetCallerID(unsigned line, const PString & idString)
{
  return CHECK_FN_ARG(SetCallerID, (m_context, line, idString));
}


BOOL OpalPluginLID::SendCallerIDOnCallWaiting(unsigned line, const PString & idString)
{
  return CHECK_FN_ARG(SendCallerIDOnCallWaiting, (m_context, line, idString));
}


BOOL OpalPluginLID::SendVisualMessageWaitingIndicator(unsigned line, BOOL on)
{
  return CHECK_FN_ARG(SendVisualMessageWaitingIndicator, (m_context, line, on));
}


BOOL OpalPluginLID::PlayDTMF(unsigned line, const char * digits, DWORD onTime, DWORD offTime)
{
  return CHECK_FN_ARG(PlayDTMF, (m_context, line, digits, onTime, offTime));
}


char OpalPluginLID::ReadDTMF(unsigned line)
{
  char dtmf;
  if (CHECK_FN_ARG(ReadDTMF, (m_context, line, &dtmf)))
    return dtmf;
  return '\0';
}


BOOL OpalPluginLID::GetRemoveDTMF(unsigned line)
{
  BOOL remove;
  if (CHECK_FN_ARG(GetRemoveDTMF, (m_context, line, &remove)))
    return remove;
  return FALSE;
}


BOOL OpalPluginLID::SetRemoveDTMF(unsigned line, BOOL removeTones)
{
  return CHECK_FN_ARG(SetRemoveDTMF, (m_context, line, removeTones));
}


OpalLineInterfaceDevice::CallProgressTones OpalPluginLID::IsToneDetected(unsigned line)
{
  unsigned tone;
  if (CHECK_FN_ARG(IsToneDetected, (m_context, line, &tone)))
    return (CallProgressTones)tone;
  return NoTone;
}


OpalLineInterfaceDevice::CallProgressTones OpalPluginLID::WaitForToneDetect(unsigned line, unsigned timeout)
{
  if (m_definition.WaitForToneDetect == NULL)
    return OpalLineInterfaceDevice::WaitForToneDetect(line, timeout);

  unsigned tone;
  if (CHECK_FN_ARG(WaitForToneDetect, (m_context, line, timeout, &tone)))
    return (CallProgressTones)tone;
  return NoTone;
}


BOOL OpalPluginLID::WaitForTone(unsigned line, CallProgressTones tone, unsigned timeout)
{
  if (m_definition.WaitForTone == NULL)
    return OpalLineInterfaceDevice::WaitForTone(line, tone, timeout);

  return CHECK_FN_ARG(WaitForTone, (m_context, line, tone, timeout));
}


BOOL OpalPluginLID::SetToneFilterParameters(unsigned line,
                                            CallProgressTones tone,
                                            unsigned lowFrequency,
                                            unsigned highFrequency,
                                            PINDEX numCadences,
                                            const unsigned * onTimes,
                                            const unsigned * offTimes)
{
  return CHECK_FN_ARG(SetToneFilterParameters, (m_context, line, tone, lowFrequency, highFrequency, numCadences, onTimes, offTimes));
}


BOOL OpalPluginLID::PlayTone(unsigned line, CallProgressTones tone)
{
  return CHECK_FN_ARG(PlayTone, (m_context, line, tone));
}


BOOL OpalPluginLID::IsTonePlaying(unsigned line)
{
  BOOL playing;
  if (CHECK_FN_ARG(IsTonePlaying, (m_context, line, &playing)))
    return playing;
  return FALSE;
}


BOOL OpalPluginLID::StopTone(unsigned line)
{
  return CHECK_FN_ARG(StopTone, (m_context, line));
}


OpalLineInterfaceDevice::CallProgressTones OpalPluginLID::DialOut(unsigned line, const PString & number, BOOL requireTones, unsigned uiDialDelay)
{
  if (m_definition.DialOut == NULL)
    return OpalLineInterfaceDevice::DialOut(line, number, requireTones, uiDialDelay);

  if (CHECK_FN(DialOut)) {
    osError = m_definition.DialOut(m_context, line, number, requireTones, uiDialDelay);
    switch (osError)
    {
      case PluginLID_NoError :
        return RingTone;
      case PluginLID_NoDialTone :
        return DialTone;
      case PluginLID_LineBusy :
        return BusyTone;
      case PluginLID_NoAnswer :
        return ClearTone;
#if PTRACING
      default :
        CheckError(osError, "DialOut");
#endif
    }
  }

  return NoTone;
}


unsigned OpalPluginLID::GetWinkDuration(unsigned line)
{
  unsigned duration;
  if (CHECK_FN_ARG(GetWinkDuration, (m_context, line, &duration)))
    return duration;
  return 0;
}


BOOL OpalPluginLID::SetWinkDuration(unsigned line, unsigned winkDuration)
{
  return CHECK_FN_ARG(SetWinkDuration, (m_context, line, winkDuration));
}


BOOL OpalPluginLID::SetCountryCode(T35CountryCodes country)
{
  OpalLineInterfaceDevice::SetCountryCode(country);

  if (m_definition.DialOut == NULL)
    return TRUE;

  return CHECK_FN_ARG(SetCountryCode, (m_context, country));
}


PStringList OpalPluginLID::GetCountryCodeNameList() const
{
  PStringList countries;

  if (CHECK_FN(GetSupportedCountry)) {
    unsigned countryCode;
    unsigned index = 0;
    while ((osError = m_definition.GetSupportedCountry(m_context, index++, &countryCode)) == PluginLID_NoError && countryCode < NumCountryCodes)
      countries.AppendString(GetCountryCodeName((T35CountryCodes)countryCode));
#if PTRACING
    if (osError != PluginLID_NoMoreNames)
      CheckError(osError, "GetSupportedCountry");
#endif
  }

  return countries;
}

