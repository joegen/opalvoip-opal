/*
 * console_mgs.h
 *
 * An OpalManager derived class for use in a console application, providing
 * a standard set of command line arguments for configuring many system
 * parameters. Used by the sample applications such as faxopal, ovropal etc.
 *
 * Copyright (c) 2010 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_OPAL_CONSOLE_MGR_H
#define OPAL_OPAL_CONSOLE_MGR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal.h>
#include <opal/manager.h>
#include <h323/h323ep.h>
#include <sip/sipep.h>
#include <sdp/sdphttpep.h>
#include <lids/lidep.h>
#include <lids/capi_ep.h>
#include <ep/pcss.h>
#include <ep/ivr.h>
#include <ep/opalmixer.h>
#include <ep/skinnyep.h>
#include <ep/lyncep.h>

#include <ptclib/cli.h>


class OpalManagerConsole;
class OpalH281Client;


#define OPAL_CONSOLE_PREFIXES OPAL_PREFIX_SIP   " " \
                              OPAL_PREFIX_H323  " " \
                              OPAL_PREFIX_SDP   " " \
                              OPAL_PREFIX_SKINNY" " \
                              OPAL_PREFIX_LYNC" " \
                              OPAL_PREFIX_PSTN  " " \
                              OPAL_PREFIX_CAPI  " "


/**This class allows for each end point class, e.g. SIPEndPoint, to add it's
   set of parameters/commands to to the console application.
  */
class OpalConsoleEndPoint
{
protected:
  OpalConsoleEndPoint(OpalManagerConsole & console) : m_console(console) { }

  void AddRoutesFor(const OpalEndPoint * endpoint, const PString & defaultRoute);

public:
  virtual ~OpalConsoleEndPoint() { }
  virtual void GetArgumentSpec(ostream & strm) const = 0;
  enum InitResult {
    InitFailed,
    InitDisabled,
    InitSuccess
  };
  virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute) = 0;
#if P_CLI
  virtual void AddCommands(PCLI & cli) = 0;
#endif

protected:
  OpalManagerConsole & m_console;
};


#if OPAL_SIP || OPAL_H323
class OpalRTPEndPoint;

class OpalRTPConsoleEndPoint : public OpalConsoleEndPoint
{
protected:
  OpalRTPConsoleEndPoint(OpalManagerConsole & console, OpalRTPEndPoint * endpoint);

  void GetArgumentSpec(ostream & strm) const;
  bool Initialise(PArgList & args, ostream & output, bool verbose);

#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalRTPConsoleEndPoint, CmdInterfaces);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalRTPConsoleEndPoint, CmdCryptoSuites);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalRTPConsoleEndPoint, CmdBandwidth);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalRTPConsoleEndPoint, CmdUserInputMode);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalRTPConsoleEndPoint, CmdStringOption);
  void AddCommands(PCLI & cli);
#endif //P_CLI

  bool SetUIMode(const PCaselessString & str);

protected:
  OpalRTPEndPoint & m_endpoint;
};
#endif // OPAL_SIP || OPAL_H323


#if OPAL_SIP
class SIPConsoleEndPoint : public SIPEndPoint, public OpalRTPConsoleEndPoint
{
  PCLASSINFO(SIPConsoleEndPoint, SIPEndPoint)
public:
  SIPConsoleEndPoint(OpalManagerConsole & manager);

  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute);

#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, SIPConsoleEndPoint, CmdProxy);
  PDECLARE_NOTIFIER(PCLI::Arguments, SIPConsoleEndPoint, CmdRegister);
  virtual void AddCommands(PCLI & cli);
#endif // P_CLI

  virtual void OnRegistrationStatus(const RegistrationStatus & status);
  bool DoRegistration(ostream & output,
                      bool verbose,
                      const PString & aor,
                      const PString & pwd,
                      const PArgList & args,
                      const char * authId,
                      const char * realm,
                      const char * proxy,
                      const char * mode,
                      const char * ttl);
};
#endif // OPAL_SIP


#if OPAL_SDP && OPAL_PTLIB_HTTP
class OpalSDPHTTPConsoleEndPoint : public OpalSDPHTTPEndPoint, public OpalRTPConsoleEndPoint
{
  PCLASSINFO(OpalSDPHTTPConsoleEndPoint, OpalSDPHTTPEndPoint)
public:
  OpalSDPHTTPConsoleEndPoint(OpalManagerConsole & manager);
  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute);
};
#endif // OPAL_SDP && OPAL_PTLIB_HTTP


#if OPAL_H323
class H323ConsoleEndPoint : public H323EndPoint, public OpalRTPConsoleEndPoint
{
  PCLASSINFO(H323ConsoleEndPoint, H323EndPoint)
public:
  H323ConsoleEndPoint(OpalManagerConsole & manager);
  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute);

#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, H323ConsoleEndPoint, CmdTerminalType);
  PDECLARE_NOTIFIER(PCLI::Arguments, H323ConsoleEndPoint, CmdAlias);
  PDECLARE_NOTIFIER(PCLI::Arguments, H323ConsoleEndPoint, CmdGatekeeper);
  PDECLARE_NOTIFIER(PCLI::Arguments, H323ConsoleEndPoint, CmdCompatibility);
  virtual void AddCommands(PCLI & cli);
#endif // P_CLI

  virtual void OnGatekeeperStatus(H323Gatekeeper & gk, H323Gatekeeper::RegistrationFailReasons status);
  bool UseGatekeeperFromArgs(const PArgList & args, const char * host, const char * ident, const char * pass, const char * inter);
};
#endif // OPAL_H323


#if OPAL_SKINNY
class OpalConsoleSkinnyEndPoint : public OpalSkinnyEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsoleSkinnyEndPoint, OpalSkinnyEndPoint)
public:
  OpalConsoleSkinnyEndPoint(OpalManagerConsole & manager);

  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute);

#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsoleSkinnyEndPoint, CmdRegister);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsoleSkinnyEndPoint, CmdStatus);
  virtual void AddCommands(PCLI & cli);
#endif // P_CLI
};
#endif // OPAL_SKINNY


#if OPAL_LYNC
class OpalConsoleLyncEndPoint : public OpalLyncEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsoleLyncEndPoint, OpalLyncEndPoint)
public:
  OpalConsoleLyncEndPoint(OpalManagerConsole & manager);

  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute);

#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsoleLyncEndPoint, CmdRegister);
  virtual void AddCommands(PCLI & cli);
#endif // P_CLI
};
#endif // OPAL_LYNC


#if OPAL_LID
class OpalConsoleLineEndPoint : public OpalLineEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsoleLineEndPoint, OpalLineEndPoint)
public:
  OpalConsoleLineEndPoint(OpalManagerConsole & manager);

  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute);

#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsoleLineEndPoint, CmdCountry);
  virtual void AddCommands(PCLI & cli);
#endif // P_CLI
};
#endif // OPAL_LID


#if OPAL_CAPI
class OpalConsoleCapiEndPoint : public OpalCapiEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsoleCapiEndPoint, OpalCapiEndPoint)
public:
  OpalConsoleCapiEndPoint(OpalManagerConsole & manager);

  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute);

#if P_CLI
  virtual void AddCommands(PCLI & cli);
#endif // P_CLI
};
#endif // OPAL_CAPI


#if OPAL_HAS_PCSS
class OpalConsolePCSSEndPoint : public OpalPCSSEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsolePCSSEndPoint, OpalPCSSEndPoint)
public:
  OpalConsolePCSSEndPoint(OpalManagerConsole & manager);

  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool verbose, const PString &);

#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdRingFileAndDevice);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdRingbackTone);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdVolume);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdDefaultAudioDevice);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdChangeAudioDevice);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdAudioBuffers);
#if OPAL_VIDEO
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdDefaultVideoDevice);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdChangeVideoDevice);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdOpenVideoStream);
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdCloseVideoStream);

  bool SetPresentationVideoDevice(const PVideoDevice::OpenArgs & args) { return m_manager.SetVideoInputDevice(args, OpalVideoFormat::ePresentation); }
  const PVideoDevice::OpenArgs & GetPresentationVideoDevice() const { return m_manager.GetVideoInputDevice(OpalVideoFormat::ePresentation); }
  bool SetPresentationPreviewDevice(const PVideoDevice::OpenArgs & args) { return m_manager.SetVideoPreviewDevice(args, OpalVideoFormat::ePresentation); }
  const PVideoDevice::OpenArgs & GetPresentationPreviewDevice() const { return m_manager.GetVideoPreviewDevice(OpalVideoFormat::ePresentation); }
  bool SetPresentationOutputDevice(const PVideoDevice::OpenArgs & args) { return m_manager.SetVideoOutputDevice(args, OpalVideoFormat::ePresentation); }
  const PVideoDevice::OpenArgs & GetPresentationOutputDevice() const { return m_manager.GetVideoOutputDevice(OpalVideoFormat::ePresentation); }
  bool SetSpeakerVideoDevice(const PVideoDevice::OpenArgs & args) { return m_manager.SetVideoInputDevice(args, OpalVideoFormat::eSpeaker); }
  const PVideoDevice::OpenArgs & GetSpeakerVideoDevice() const { return m_manager.GetVideoInputDevice(OpalVideoFormat::eSpeaker); }
  bool SetSpeakerPreviewDevice(const PVideoDevice::OpenArgs & args) { return m_manager.SetVideoPreviewDevice(args, OpalVideoFormat::eSpeaker); }
  const PVideoDevice::OpenArgs & GetSpeakerPreviewDevice() const { return m_manager.GetVideoPreviewDevice(OpalVideoFormat::eSpeaker); }
  bool SetSpeakerOutputDevice(const PVideoDevice::OpenArgs & args) { return m_manager.SetVideoOutputDevice(args, OpalVideoFormat::eSpeaker); }
  const PVideoDevice::OpenArgs & GetSpeakerOutputDevice() const { return m_manager.GetVideoOutputDevice(OpalVideoFormat::eSpeaker); }
  bool SetSignVideoDevice(const PVideoDevice::OpenArgs & args) { return m_manager.SetVideoInputDevice(args, OpalVideoFormat::eSignLanguage); }
  const PVideoDevice::OpenArgs & GetSignVideoDevice() const { return m_manager.GetVideoInputDevice(OpalVideoFormat::eSignLanguage); }
  bool SetSignPreviewDevice(const PVideoDevice::OpenArgs & args) { return m_manager.SetVideoPreviewDevice(args, OpalVideoFormat::eSignLanguage); }
  const PVideoDevice::OpenArgs & GetSignPreviewDevice() const { return m_manager.GetVideoPreviewDevice(OpalVideoFormat::eSignLanguage); }
  bool SetSignOutputDevice(const PVideoDevice::OpenArgs & args) { return m_manager.SetVideoOutputDevice(args, OpalVideoFormat::eSignLanguage); }
  const PVideoDevice::OpenArgs & GetSignOutputDevice() const { return m_manager.GetVideoOutputDevice(OpalVideoFormat::eSignLanguage); }
#endif // OPAL_VIDEO
#if OPAL_HAS_H281
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsolePCSSEndPoint, CmdExternalCameraControl);
  PDECLARE_NOTIFIER(OpalH281Client, OpalConsolePCSSEndPoint, ExternalCameraControlNotification);
#endif

  virtual void AddCommands(PCLI & cli);
#endif // P_CLI

  void SetRingInfo(ostream & out, bool verbose, const PString & filename, const PString & device, const PString & driver);

  // Various call backs
  virtual bool OnIncomingCall(OpalLocalConnection & connection);
  virtual void OnConnected(OpalConnection & connection);
  virtual void OnReleased(OpalConnection & connection);
  virtual void ShutDown();

  protected:
    PFilePath             m_ringFileName;
    PSoundChannel::Params m_ringChannelParams;
    PThread             * m_ringThread;
    PSyncPoint            m_ringSignal;
    enum RingState
    {
      e_RingIdle,
      e_Ringing,
      e_RingShutDown
    } m_ringState;
    void RingThreadMain();
};
#endif // OPAL_HAS_PCSS


#if OPAL_IVR
class OpalConsoleIVREndPoint : public OpalIVREndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsoleIVREndPoint, OpalIVREndPoint)
public:
  OpalConsoleIVREndPoint(OpalManagerConsole & manager);

  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool, const PString &);

#if P_CLI
  virtual void AddCommands(PCLI &);
#endif // P_CLI
};
#endif // OPAL_IVR


#if OPAL_HAS_MIXER
class OpalConsoleMixerEndPoint : public OpalMixerEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsoleMixerEndPoint, OpalMixerEndPoint)
public:
  OpalConsoleMixerEndPoint(OpalManagerConsole & manager);

  virtual void GetArgumentSpec(ostream & strm) const;
  virtual bool Initialise(PArgList & args, bool, const PString &);

#if P_CLI
  virtual void AddCommands(PCLI &);
#endif // P_CLI
};
#endif // OPAL_HAS_MIXER


/**OPAL manager class for console applications.
   An OpalManager derived class for use in a console application, providing
   a standard set of command line arguments for configuring many system
   parameters. Used by the sample applications such as faxopal, ovropal etc.
  */
class OpalManagerConsole : public OpalManager
{
    PCLASSINFO(OpalManagerConsole, OpalManager);

  public:
    OpalManagerConsole(
      const char * endpointPrefixes = OPAL_CONSOLE_PREFIXES
    );
    ~OpalManagerConsole();

    virtual PString GetArgumentSpec() const;
    virtual void Usage(ostream & strm, const PArgList & args);

    bool PreInitialise(PArgList & args, bool verbose = false);

    virtual bool Initialise(
      PArgList & args,
      bool verbose = false,
      const PString & defaultRoute = PString::Empty()
    );
    virtual void Run();
    virtual void EndRun(bool interrupt = false);
    virtual void Broadcast(const PString & msg);

    virtual bool OnLocalOutgoingCall(const OpalLocalConnection & connection);
    virtual void OnEstablishedCall(OpalCall & call);
    virtual void OnHold(OpalConnection & connection, bool fromRemote, bool onHold);
    virtual bool OnChangedPresentationRole(OpalConnection & connection, const PString & newChairURI, bool request);
    virtual void OnStartMediaPatch(OpalConnection & connection, OpalMediaPatch & patch);
    virtual void OnClosedMediaStream(const OpalMediaStream & stream);
    virtual void OnFailedMediaStream(OpalConnection & connection, bool fromRemote, const PString & reason);
    virtual void OnUserInputString(OpalConnection & connection, const PString & value);
    virtual void OnClearedCall(OpalCall & call);

    class LockedStream : PWaitAndSignal
    {
      protected:
        ostream & m_stream;
      public:
        LockedStream(const OpalManagerConsole & mgr)
          : PWaitAndSignal(mgr.m_outputMutex)
          , m_stream(*mgr.m_outputStream)
        {
        }

        ostream & operator *() const { return m_stream; }
        operator  ostream & () const { return m_stream; }
    };
    friend class LockedStream;
    __inline LockedStream LockedOutput() const { return *this; }


    bool GetCallFromArgs(PCLI::Arguments & args, PSafePtr<OpalCall> & call) const;

    template <class CONTYPE> bool GetConnectionFromArgs(PCLI::Arguments & args, PSafePtr<CONTYPE> & connection) const
    {
      PSafePtr<OpalCall> call;
      if (!GetCallFromArgs(args, call))
        return false;

      if ((connection = call->GetConnectionAs<CONTYPE>(0)) != NULL)
        return true;

      args.WriteError("Not a suitable call for operation.");
      return false;
    }

    bool GetStreamFromArgs(
      PCLI::Arguments & args,
      const OpalMediaType & mediaType,
      bool source,
      PSafePtr<OpalMediaStream> & stream
    ) const;

  protected:
    OpalConsoleEndPoint * GetConsoleEndPoint(const PString & prefix);

#if OPAL_H323
    virtual H323ConsoleEndPoint * CreateH323EndPoint();
#endif
#if OPAL_SIP
    virtual SIPConsoleEndPoint * CreateSIPEndPoint();
#endif
#if OPAL_SDP_HTTP
    virtual OpalSDPHTTPConsoleEndPoint * CreateSDPHTTPEndPoint();
#endif
#if OPAL_SKINNY
    virtual OpalConsoleSkinnyEndPoint * CreateSkinnyEndPoint();
#endif
#if OPAL_LYNC
    virtual OpalConsoleLyncEndPoint * CreateLyncEndPoint();
#endif
#if OPAL_LID
    virtual OpalConsoleLineEndPoint * CreateLineEndPoint();
#endif
#if OPAL_CAPI
    virtual OpalConsoleCapiEndPoint * CreateCapiEndPoint();
#endif

#if OPAL_HAS_PCSS
    virtual OpalConsolePCSSEndPoint * CreatePCSSEndPoint();
#endif
#if OPAL_IVR
    virtual OpalConsoleIVREndPoint * CreateIVREndPoint();
#endif
#if OPAL_HAS_MIXER
    virtual OpalConsoleMixerEndPoint * CreateMixerEndPoint();
#endif

    PStringArray m_endpointPrefixes;

    PSyncPoint m_endRun;
    bool       m_interrupted;
    bool       m_verbose;
    ostream  * m_outputStream;
    PDECLARE_MUTEX(m_outputMutex, OpalConsoleOutput);

#if OPAL_STATISTICS
    PTimeInterval m_statsPeriod;
    PFilePath     m_statsFile;
    typedef map<PString, OpalMediaStatistics> StatsMap;
    StatsMap m_statistics;
    PDECLARE_MUTEX(m_statsMutex, OpalConsoleStats);
    virtual bool OutputStatistics();
    virtual bool OutputStatistics(ostream & strm);
    virtual bool OutputCallStatistics(ostream & strm, OpalCall & call);
    virtual bool OutputStreamStatistics(ostream & strm, const OpalMediaStream & stream);
#endif
};

typedef OpalManagerConsole OpalConsoleManager; // Backward compatibility

#if P_CLI

/**OPAL manager class for applications with command line interpreter.
   An OpalManager derived class for use in a console application, providing
   a standard set of command line arguments for configuring many system
   parameters,  and a standard command line interpreter for control of many
   functions. Used by the sample applications such as faxopal, ovropal etc.
  */
class OpalManagerCLI : public OpalManagerConsole
{
    PCLASSINFO(OpalManagerCLI, OpalManagerConsole);

  public:
    OpalManagerCLI(
      const char * endpointPrefixes = OPAL_CONSOLE_PREFIXES
    );
    ~OpalManagerCLI();

    // Overrides from OpalManagerConsole
    virtual PString GetArgumentSpec() const;
    virtual bool Initialise(
      PArgList & args,
      bool verbose,
      const PString & defaultRoute = PString::Empty()
    );
    virtual void Run();
    virtual void EndRun(bool interrupt = false);
    virtual void Broadcast(const PString & msg);

  protected:
    PCLI * CreateCLIStandard();
#if P_TELNET
    PCLITelnet * CreateCLITelnet(WORD port);
#endif
#if P_CURSES
    PCLICurses * CreateCLICurses();
#endif

#if OPAL_PTLIB_SSL
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdSSL);
#endif
#if P_NAT
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdNatList);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdNatAddress);
#endif

#if PTRACING
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdTrace);
#endif

#if OPAL_STATISTICS
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdStatistics);
#endif

#if OPAL_HAS_H281
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdFarEndCamera);
#endif

    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdAudioCodec);
#if OPAL_VIDEO
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdVideoCodec);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdVideoDefault);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdVideoTransmit);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdVideoReceive);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdPresentationToken);
#endif // OPAL_VIDEO

#if OPAL_HAS_MIXER
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdRecord);
#endif
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdSilenceDetect);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdAutoStart);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdCodecList);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdCodecOrder);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdCodecMask);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdCodecOption);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdShowCalls);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdSendUserInput);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdHangUp);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdDelay);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdVersion);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdQuit);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdShutDown);

    PCLI * m_cli;
};

#endif // P_CLI


/**Create a process for OpalManagerConsole based applications.
  */
template <class Manager,                   ///< Class of OpalManagerConsole derived class
          const char Manuf[],              ///< Name of manufacturer
          const char Name[],               ///< Name of product
          unsigned MajorVersion = OPAL_MAJOR,  ///< Major version number of the product
          unsigned MinorVersion = OPAL_MINOR,  ///< Minor version number of the product
          PProcess::CodeStatus Status = PProcess::ReleaseCode, ///< Development status of the product
          unsigned BuildNumber = OPAL_BUILD,   ///< Build number of the product
          bool Verbose = true>
class OpalConsoleProcess : public PProcess
{
    PCLASSINFO(OpalConsoleProcess, PProcess)
  public:
    OpalConsoleProcess()
      : PProcess(Manuf, Name, MajorVersion, MinorVersion, Status, BuildNumber)
      , m_manager(NULL)
    {
    }

    ~OpalConsoleProcess()
    {
      delete this->m_manager;
    }

    virtual void Main()
    {
      this->SetTerminationValue(1);
      this->m_manager = new Manager;
      if (this->m_manager->Initialise(this->GetArguments(), Verbose)) {
        this->SetTerminationValue(0);
        this->m_manager->Run();
      }
    }

    virtual bool OnInterrupt(bool)
    {
      if (this->m_manager == NULL)
        return false;

      this->m_manager->EndRun(true);
      return true;
    }

  private:
    Manager * m_manager;
};


#endif // OPAL_OPAL_CONSOLE_MGR_H


/////////////////////////////////////////////////////////////////////////////
