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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "lidpluginmgr.h"
#endif

#include <ptlib.h>

#include <lids/lidpluginmgr.h>
#include <ptclib/dtmf.h>


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
  , m_tonePlayer(NULL)
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
  StopTone(0);
  if (m_context != NULL && m_definition.Destroy != NULL)
    m_definition.Destroy(&m_definition, m_context);
}


#if PTRACING

bool OpalPluginLID::BadContext() const
{
  if (m_context != NULL)
    return false;

  PTRACE(1, "LID Plugin\tNo context for " << m_definition.name);
  return true;
}

bool OpalPluginLID::BadFunction(void * fnPtr, const char * fnName) const
{
  if (fnPtr != NULL)
    return false;

  PTRACE(1, "LID Plugin\tFunction " << fnName << " not implemented in " << m_definition.name);
  return true;
}

PluginLID_Errors OpalPluginLID::CheckError(PluginLID_Errors error, const char * fnName) const
{
  if (error != PluginLID_NoError && error != PluginLID_UnimplementedFunction && error != PluginLID_NoMoreNames)
    PTRACE(2, "LID Plugin\tFunction " << fnName << " in " << m_definition.name << " returned error " << error);

  osError = error;
  return error;
}

#define BAD_FN(fn)         (BadContext() || BadFunction((void *)m_definition.fn, #fn))
#define CHECK_FN(fn, args) (BadContext() ? PluginLID_BadContext : m_definition.fn == NULL ? PluginLID_UnimplementedFunction : CheckError(m_definition.fn args, #fn))

#else // PTRACING

#define BAD_FN(fn) (m_context == NULL || m_definition.fn == NULL)
#define CHECK_FN(fn, args) (m_context == NULL ? PluginLID_BadContext : m_definition.fn == NULL ? PluginLID_UnimplementedFunction : (osError = m_definition.fn args))

#endif // PTRACING


BOOL OpalPluginLID::Open(const PString & device)
{
  if (BAD_FN(Open))
    return FALSE;

  Close();


  switch (osError = m_definition.Open(m_context, device)) {
    case PluginLID_NoError :
      break;

    case PluginLID_UsesSoundChannel :
      if (!m_player.Open(device, PSoundChannel::Player)) {
        PTRACE(1, "LID Plugin\t" << m_definition.name << " requires sound system, but cannot open player for \"" << device << '"');
        return FALSE;
      }

      if (!m_recorder.Open(device, PSoundChannel::Recorder)) {
        PTRACE(1, "LID Plugin\t" << m_definition.name << " requires sound system, but cannot open recorder for \"" << device << '"');
        return FALSE;
      }
      break;

    case PluginLID_NoSuchDevice :
      PTRACE(1, "LID Plugin\tNo such device as \"" << device << "\" in " << m_definition.name);
      return FALSE;

    default :
      PTRACE(1, "LID Plugin\tOpen of \"" << device << "\" in " << m_definition.name << " returned error " << osError);
      return FALSE;
  }

  m_deviceName = device;
  os_handle = 1;
  return TRUE;
}


BOOL OpalPluginLID::Close()
{
  OpalLineInterfaceDevice::Close();

  StopTone(0);
  m_player.Close();
  m_recorder.Close();

  if (BAD_FN(Close))
    return FALSE;

  return m_definition.Close(m_context) == PluginLID_NoError;
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

  char buffer[200];
  unsigned index = 0;
  while (CHECK_FN(GetDeviceName, (m_context, index++, buffer, sizeof(buffer))) == PluginLID_NoError)
    devices.AppendString(buffer);

  return devices;
}


PString OpalPluginLID::GetDescription() const
{
  return m_definition.description;
}


unsigned OpalPluginLID::GetLineCount()
{
  unsigned count = 0;
  CHECK_FN(GetLineCount, (m_context, &count));
  return count;
}


BOOL OpalPluginLID::IsLineTerminal(unsigned line)
{
  BOOL isTerminal = FALSE;
  CHECK_FN(IsLineTerminal, (m_context, line, &isTerminal));
  return isTerminal;
}


BOOL OpalPluginLID::IsLinePresent(unsigned line, BOOL force)
{
  BOOL isPresent = FALSE;
  CHECK_FN(IsLinePresent, (m_context, line, force, &isPresent));
  return isPresent;
}


BOOL OpalPluginLID::IsLineOffHook(unsigned line)
{
  BOOL offHook = FALSE;
  CHECK_FN(IsLineOffHook, (m_context, line, &offHook));
  return offHook;
}


BOOL OpalPluginLID::SetLineOffHook(unsigned line, BOOL newState)
{
  return CHECK_FN(SetLineOffHook, (m_context, line, newState)) == PluginLID_NoError;
}


BOOL OpalPluginLID::HookFlash(unsigned line, unsigned flashTime)
{
  switch (CHECK_FN(HookFlash, (m_context, line, flashTime))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::HookFlash(line, flashTime);

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::HasHookFlash(unsigned line)
{
  BOOL flashed = FALSE;
  CHECK_FN(HasHookFlash, (m_context, line, &flashed));
  return flashed;
}


BOOL OpalPluginLID::IsLineRinging(unsigned line, DWORD * cadence)
{
  DWORD localCadence;
  if (cadence == NULL)
    cadence = &localCadence;

  if (CHECK_FN(IsLineRinging, (m_context, line, (unsigned long *)cadence)) == PluginLID_NoError)
    return *cadence != 0;

  return FALSE;
}


BOOL OpalPluginLID::RingLine(unsigned line, PINDEX nCadence, const unsigned * pattern, unsigned frequency)
{
  PUnsignedArray cadence;

  if (nCadence > 0 && pattern == NULL) {
    PString description = m_callProgressTones[RingTone];
    PINDEX colon = description.Find(':');
    if (colon != P_MAX_INDEX) {
      unsigned newFrequency = description.Left(colon).AsUnsigned();
      if (newFrequency > 5 && newFrequency < 3000) {
        PStringArray times = description.Mid(colon+1).Tokenise('-');
        if (times.GetSize() > 1) {
          cadence.SetSize(times.GetSize());
          for (PINDEX i = 0; i < cadence.GetSize(); i++)
            cadence[i] = (unsigned)(times[i].AsReal()*1000);
          return CHECK_FN(RingLine, (m_context, line, cadence.GetSize(), cadence, newFrequency)) == PluginLID_NoError;
        }
      }
    }
  }

  return CHECK_FN(RingLine, (m_context, line, nCadence, pattern, frequency)) == PluginLID_NoError;
}


BOOL OpalPluginLID::IsLineDisconnected(unsigned line, BOOL checkForWink)
{
  BOOL disconnected = FALSE;
  switch (CHECK_FN(IsLineConnected, (m_context, line, checkForWink, &disconnected))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::IsLineDisconnected(line, checkForWink);

    case PluginLID_NoError :
      return disconnected;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::SetLineToLineDirect(unsigned line1, unsigned line2, BOOL connect)
{
  return CHECK_FN(SetLineToLineDirect, (m_context, line1, line2, connect)) == PluginLID_NoError;
}


BOOL OpalPluginLID::IsLineToLineDirect(unsigned line1, unsigned line2)
{
  BOOL connected = FALSE;
  CHECK_FN(IsLineToLineDirect, (m_context, line1, line2, &connected));
  return connected;
}


OpalMediaFormatList OpalPluginLID::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  char buffer[100];
  unsigned index = 0;
  for (;;) {
    switch (CHECK_FN(GetSupportedFormat, (m_context, index++, buffer, sizeof(buffer)))) {
      case PluginLID_NoMoreNames :
        return formats;

      case PluginLID_UnimplementedFunction :
        formats += OPAL_PCM16;
        return formats;

      case PluginLID_NoError :
      {
        OpalMediaFormat format = buffer;
        if (format.IsEmpty()) {
          PTRACE(2, "LID Plugin\tCodec format \"" << buffer << "\" in " << m_definition.name << " is not supported by OPAL.");
        }
        else
          formats += format;
      }
      break;

      default : ;
    }
  }
}


BOOL OpalPluginLID::SetReadFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  switch (CHECK_FN(SetReadFormat, (m_context, line, mediaFormat))) {
    case PluginLID_UnimplementedFunction :
      return mediaFormat == OPAL_PCM16;

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::SetWriteFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  switch (CHECK_FN(SetWriteFormat, (m_context, line, mediaFormat))) {
    case PluginLID_UnimplementedFunction :
      return mediaFormat == OPAL_PCM16;

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


OpalMediaFormat OpalPluginLID::GetReadFormat(unsigned line)
{
  char buffer[100];
  switch (CHECK_FN(GetReadFormat, (m_context, line, buffer, sizeof(buffer)))) {
    case PluginLID_UnimplementedFunction :
      return OPAL_PCM16;

    case PluginLID_NoError :
      return buffer;

    default : ;
  }
  return OpalMediaFormat();
}


OpalMediaFormat OpalPluginLID::GetWriteFormat(unsigned line)
{
  char buffer[100];
  switch (CHECK_FN(GetWriteFormat, (m_context, line, buffer, sizeof(buffer)))) {
    case PluginLID_UnimplementedFunction :
      return OPAL_PCM16;

    case PluginLID_NoError :
      return buffer;

    default : ;
  }
  return OpalMediaFormat();
}


BOOL OpalPluginLID::StopReading(unsigned line)
{
  OpalLineInterfaceDevice::StopReading(line);

  switch (CHECK_FN(StopReading, (m_context, line))) {
    case PluginLID_UnimplementedFunction :
      return m_recorder.Abort();

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::StopWriting(unsigned line)
{
  OpalLineInterfaceDevice::StopWriting(line);

  switch (CHECK_FN(StopWriting, (m_context, line))) {
    case PluginLID_UnimplementedFunction :
      return m_player.Abort();

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::SetReadFrameSize(unsigned line, PINDEX frameSize)
{
  switch (CHECK_FN(SetReadFrameSize, (m_context, line, frameSize))) {
    case PluginLID_UnimplementedFunction :
      return m_recorder.SetBuffers(frameSize, 2000/frameSize+2); // Want about 250ms of buffering

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::SetWriteFrameSize(unsigned line, PINDEX frameSize)
{
  switch (CHECK_FN(SetWriteFrameSize, (m_context, line, frameSize))) {
    case PluginLID_UnimplementedFunction :
      StopTone(line);
      return m_player.SetBuffers(frameSize, 1000/frameSize+2); // Want about 125ms of buffering

    case PluginLID_NoError :
      return TRUE;

   default : ;
  }
  return FALSE;
}


PINDEX OpalPluginLID::GetReadFrameSize(unsigned line)
{
  unsigned frameSize = 0;
  switch (CHECK_FN(GetReadFrameSize, (m_context, line, &frameSize))) {
    case PluginLID_NoError :
      return frameSize;

    case PluginLID_UnimplementedFunction :
      PINDEX size, buffers;
      return m_recorder.GetBuffers(size, buffers) ? size : 0;

      default : ;
  }
  return 0;
}


PINDEX OpalPluginLID::GetWriteFrameSize(unsigned line)
{
  unsigned frameSize = 0;
  switch (CHECK_FN(GetWriteFrameSize, (m_context, line, &frameSize))) {
    case PluginLID_NoError :
      return frameSize;

    case PluginLID_UnimplementedFunction :
      PINDEX size, buffers;
      return m_player.GetBuffers(size, buffers) ? size : 0;

      default : ;
  }
  return 0;
}


BOOL OpalPluginLID::ReadFrame(unsigned line, void * buffer, PINDEX & count)
{
  unsigned uiCount = 0;
  switch (CHECK_FN(ReadFrame, (m_context, line, buffer, &uiCount))) {
    case PluginLID_UnimplementedFunction :
      count = GetReadFrameSize(line);
      if (!m_recorder.Read(buffer, count))
        return FALSE;
      count = m_recorder.GetLastReadCount();
      return TRUE;

    case PluginLID_NoError :
      count = uiCount;
      return TRUE;

   default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::WriteFrame(unsigned line, const void * buffer, PINDEX count, PINDEX & written)
{
  unsigned uiCount = 0;
  switch (CHECK_FN(WriteFrame, (m_context, line, buffer, count, &uiCount))) {
    case PluginLID_UnimplementedFunction :
      if (!m_player.Write(buffer, count))
        return FALSE;
      written = m_player.GetLastWriteCount();
      return TRUE;

    case PluginLID_NoError :
      written = uiCount;
      return TRUE;

    default : ;
  }
  return FALSE;
}


unsigned OpalPluginLID::GetAverageSignalLevel(unsigned line, BOOL playback)
{
  unsigned signal = UINT_MAX;
  CHECK_FN(GetAverageSignalLevel, (m_context, line, playback, &signal));
  return signal;
}


BOOL OpalPluginLID::EnableAudio(unsigned line, BOOL enable)
{
  switch (CHECK_FN(EnableAudio, (m_context, line, enable))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::EnableAudio(line, enable);

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::IsAudioEnabled(unsigned line)
{
  BOOL enabled = FALSE;
  if (CHECK_FN(IsAudioEnabled, (m_context, line, &enabled)) == PluginLID_UnimplementedFunction)
    return OpalLineInterfaceDevice::IsAudioEnabled(line);
  return enabled;
}


BOOL OpalPluginLID::SetRecordVolume(unsigned line, unsigned volume)
{
  switch (CHECK_FN(SetRecordVolume, (m_context, line, volume))) {
    case PluginLID_UnimplementedFunction :
      return m_recorder.SetVolume(volume);

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::SetPlayVolume(unsigned line, unsigned volume)
{
  switch (CHECK_FN(SetPlayVolume, (m_context, line, volume))) {
    case PluginLID_UnimplementedFunction :
      return m_player.SetVolume(volume);

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::GetRecordVolume(unsigned line, unsigned & volume)
{
  switch (CHECK_FN(GetRecordVolume, (m_context, line, &volume))) {
    case PluginLID_UnimplementedFunction :
      return m_recorder.GetVolume(volume);

    case PluginLID_NoError :
      return TRUE;

    default : ;
  }
  return FALSE;
}


BOOL OpalPluginLID::GetPlayVolume(unsigned line, unsigned & volume)
{
  switch (CHECK_FN(GetPlayVolume, (m_context, line, &volume))) {
    case PluginLID_UnimplementedFunction :
      return m_player.GetVolume(volume);

    case PluginLID_NoError :
      return TRUE;

      default:;

  }
  return FALSE;
}


OpalLineInterfaceDevice::AECLevels OpalPluginLID::GetAEC(unsigned line)
{
  unsigned level = AECError;
  CHECK_FN(GetAEC, (m_context, line, &level));
  return (AECLevels)level;
}


BOOL OpalPluginLID::SetAEC(unsigned line, AECLevels level)
{
  return CHECK_FN(SetAEC, (m_context, line, level)) == PluginLID_NoError;
}


BOOL OpalPluginLID::GetVAD(unsigned line)
{
  BOOL vad = FALSE;
  CHECK_FN(GetVAD, (m_context, line, &vad));
  return vad;
}


BOOL OpalPluginLID::SetVAD(unsigned line, BOOL enable)
{
  return CHECK_FN(SetVAD, (m_context, line, enable)) == PluginLID_NoError;
}


BOOL OpalPluginLID::GetCallerID(unsigned line, PString & idString, BOOL full)
{
  return CHECK_FN(GetCallerID, (m_context, line, idString.GetPointer(500), 500, full)) == PluginLID_NoError;
}


BOOL OpalPluginLID::SetCallerID(unsigned line, const PString & idString)
{
  if (idString.IsEmpty())
    return FALSE;

  return CHECK_FN(SetCallerID, (m_context, line, idString)) == PluginLID_NoError;
}


BOOL OpalPluginLID::SendCallerIDOnCallWaiting(unsigned line, const PString & idString)
{
  if (idString.IsEmpty())
    return FALSE;

  return CHECK_FN(SendCallerIDOnCallWaiting, (m_context, line, idString)) == PluginLID_NoError;
}


BOOL OpalPluginLID::SendVisualMessageWaitingIndicator(unsigned line, BOOL on)
{
  return CHECK_FN(SendVisualMessageWaitingIndicator, (m_context, line, on)) == PluginLID_NoError;
}


BOOL OpalPluginLID::PlayDTMF(unsigned line, const char * digits, DWORD onTime, DWORD offTime)
{
  return CHECK_FN(PlayDTMF, (m_context, line, digits, onTime, offTime)) == PluginLID_NoError;
}


char OpalPluginLID::ReadDTMF(unsigned line)
{
  char dtmf = '\0';
  CHECK_FN(ReadDTMF, (m_context, line, &dtmf));
  return dtmf;
}


BOOL OpalPluginLID::GetRemoveDTMF(unsigned line)
{
  BOOL remove = FALSE;
  CHECK_FN(GetRemoveDTMF, (m_context, line, &remove));
  return remove;
}


BOOL OpalPluginLID::SetRemoveDTMF(unsigned line, BOOL removeTones)
{
  return CHECK_FN(SetRemoveDTMF, (m_context, line, removeTones)) == PluginLID_NoError;
}


OpalLineInterfaceDevice::CallProgressTones OpalPluginLID::IsToneDetected(unsigned line)
{
  int tone = NoTone;
  CHECK_FN(IsToneDetected, (m_context, line, &tone));
  return (CallProgressTones)tone;
}


OpalLineInterfaceDevice::CallProgressTones OpalPluginLID::WaitForToneDetect(unsigned line, unsigned timeout)
{
  int tone = NoTone;
  if (CHECK_FN(WaitForToneDetect, (m_context, line, timeout, &tone)) == PluginLID_UnimplementedFunction)
    return OpalLineInterfaceDevice::WaitForToneDetect(line, timeout);
  return (CallProgressTones)tone;
}


BOOL OpalPluginLID::WaitForTone(unsigned line, CallProgressTones tone, unsigned timeout)
{
  switch (CHECK_FN(WaitForTone, (m_context, line, tone, timeout))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::WaitForTone(line, tone, timeout);

    case PluginLID_NoError :
      return TRUE;

    default:
      break;
  }

  return FALSE;
}


BOOL OpalPluginLID::SetToneFilterParameters(unsigned line,
                                            CallProgressTones tone,
                                            unsigned lowFrequency,
                                            unsigned highFrequency,
                                            PINDEX numCadences,
                                            const unsigned * onTimes,
                                            const unsigned * offTimes)
{
  return CHECK_FN(SetToneFilterParameters, (m_context, line, tone, lowFrequency, highFrequency, numCadences, onTimes, offTimes)) == PluginLID_NoError;
}


void OpalPluginLID::TonePlayer(PThread &, INT tone)
{
  if (tone >= NumTones)
    return;

  PTones toneData;
  if (!toneData.Generate(m_callProgressTones[tone])) {
    PTRACE(2, "LID Plugin\tTone generation generation for \"" << m_callProgressTones[tone] << "\"failed.");
    return;
  }

  while (!m_stopTone.Wait(0)) {
    if (!m_player.Write(toneData, toneData.GetSize()*2)) {
      PTRACE(2, "LID Plugin\tTone generation write failed.");
      break;
    }
  }
}


BOOL OpalPluginLID::PlayTone(unsigned line, CallProgressTones tone)
{
  switch (CHECK_FN(PlayTone, (m_context, line, tone))) {
    case PluginLID_UnimplementedFunction :
      // Stop previous tone, if running
      if (m_tonePlayer != NULL) {
        m_stopTone.Signal();
        m_tonePlayer->WaitForTermination(1000);
        delete m_tonePlayer;
      }

      // Clear out extraneous signals
      while (m_stopTone.Wait(0))
        ;

      // Start new tone thread
      m_tonePlayer = PThread::Create(PCREATE_NOTIFIER(TonePlayer), tone, PThread::NoAutoDeleteThread, PThread::NormalPriority, "TonePlayer");
      return m_tonePlayer != NULL;

    case PluginLID_NoError :
      return TRUE;

    default:
      break;
  }

  return FALSE;
}


BOOL OpalPluginLID::IsTonePlaying(unsigned line)
{
  BOOL playing = FALSE;
  if (m_tonePlayer == NULL || m_tonePlayer->IsTerminated())
    CHECK_FN(IsTonePlaying, (m_context, line, &playing));
  return playing;
}


BOOL OpalPluginLID::StopTone(unsigned line)
{
  if (m_tonePlayer == NULL || m_tonePlayer->IsTerminated()) {
    switch (CHECK_FN(StopTone, (m_context, line))) {
      case PluginLID_UnimplementedFunction :
      case PluginLID_NoError :
        return TRUE;
      default:
        break;
    }
    return false;
  }

  m_stopTone.Signal();
  m_tonePlayer->WaitForTermination(1000);
  delete m_tonePlayer;
  m_tonePlayer = NULL;
  return true;
}


OpalLineInterfaceDevice::CallProgressTones OpalPluginLID::DialOut(unsigned line, const PString & number, BOOL requireTones, unsigned uiDialDelay)
{
  if (m_definition.DialOut == NULL)
    return OpalLineInterfaceDevice::DialOut(line, number, requireTones, uiDialDelay);

  if (BAD_FN(DialOut))
    return NoTone;

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
      CheckError((PluginLID_Errors)osError, "DialOut");
#endif
  }

  return NoTone;
}


unsigned OpalPluginLID::GetWinkDuration(unsigned line)
{
  unsigned duration = 0;
  CHECK_FN(GetWinkDuration, (m_context, line, &duration));
  return duration;
}


BOOL OpalPluginLID::SetWinkDuration(unsigned line, unsigned winkDuration)
{
  return CHECK_FN(SetWinkDuration, (m_context, line, winkDuration)) == PluginLID_NoError;
}


BOOL OpalPluginLID::SetCountryCode(T35CountryCodes country)
{
  switch (CHECK_FN(SetCountryCode, (m_context, country))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::SetCountryCode(country);

    case PluginLID_NoError :
      return TRUE;

    default:
      break;
  }

  return FALSE;
}


PStringList OpalPluginLID::GetCountryCodeNameList() const
{
  PStringList countries;

  unsigned index = 0;
  for (;;) {
    unsigned countryCode = NumCountryCodes;
    switch (CHECK_FN(GetSupportedCountry, (m_context, index++, &countryCode))) {
      case PluginLID_UnimplementedFunction :
        return OpalLineInterfaceDevice::GetCountryCodeNameList();

      case PluginLID_NoError :
        if (countryCode < NumCountryCodes)
          countries.AppendString(GetCountryCodeName((T35CountryCodes)countryCode));
        break;

      case PluginLID_NoMoreNames :
        return countries;

      default :
        return PStringList();
    }
  }
}

