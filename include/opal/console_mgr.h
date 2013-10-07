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
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_OPAL_CONSOLE_MGR_H
#define OPAL_OPAL_CONSOLE_MGR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal.h>
#include <opal/manager.h>
#include <ptclib/cli.h>

class SIPEndPoint;
class H323EndPoint;
class OpalLineEndPoint;
class OpalCapiEndPoint;

class OpalPCSSEndPoint;
class OpalIVREndPoint;
class OpalMixerEndPoint;

#define OPAL_CONSOLE_PREFIXES OPAL_PREFIX_SIP   " " \
                              OPAL_PREFIX_H323  " " \
                              OPAL_PREFIX_PSTN  " " \
                              OPAL_PREFIX_CAPI  " "


/**This class allows for each end point class, e.g. SIPEndPoint, to add it's
   set of parameters/commands to to the console application.
  */
struct OpalConsoleEndPoint
{
  virtual void GetArgumentSpec(ostream & strm) const = 0;
  enum InitResult {
    InitFailed,
    InitDisabled,
    InitSuccess
  };
  virtual bool Initialise(PArgList & args, ostream & output, bool verbose, const PString & defaultRoute) = 0;
#if P_CLI
  virtual void AddCommands(PCLI & cli) = 0;
#endif
};


/**OPAL manager class for console applications.
   An OpalManager derived class for use in a console application, providing
   a standard set of command line arguments for configuring many system
   parameters. Used by the sample applications such as faxopal, ovropal etc.
  */
class OpalConsoleManager : public OpalManager
{
    PCLASSINFO(OpalConsoleManager, OpalManager);

  public:
    OpalConsoleManager(
      const char * endpointPrefixes = OPAL_CONSOLE_PREFIXES
    );

    virtual PString GetArgumentSpec() const;
    virtual void Usage(ostream & strm, const PArgList & args);

    virtual bool Initialise(
      PArgList & args,
      bool verbose,
      const PString & defaultRoute = PString::Empty()
    );
    virtual void Run();
    virtual void EndRun(bool interrupt = false);

    void OnEstablishedCall(OpalCall & call);
    void OnHold(OpalConnection & connection, bool fromRemote, bool onHold);
    PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);
    void OnClosedMediaStream(const OpalMediaStream & stream);
    virtual void OnClearedCall(OpalCall & call);

    __inline ostream & Output() const { return *m_outputStream; }

  protected:
    OpalConsoleEndPoint * GetConsoleEndPoint(const PString & prefix);

#if OPAL_SIP
    virtual SIPEndPoint * CreateSIPEndPoint();
#endif
#if OPAL_H323
    virtual H323EndPoint * CreateH323EndPoint();
#endif
#if OPAL_LID
    virtual OpalLineEndPoint * CreateLineEndPoint();
#endif
#if OPAL_CAPI
    virtual OpalCapiEndPoint * CreateCapiEndPoint();
#endif

#if OPAL_HAS_PCSS
    virtual OpalPCSSEndPoint * CreatePCSSEndPoint();
#endif
#if OPAL_IVR
    virtual OpalIVREndPoint * CreateIVREndPoint();
#endif
#if OPAL_HAS_MIXER
    virtual OpalMixerEndPoint * CreateMixerEndPoint();
#endif

    bool PreInitialise(PArgList & args, bool verbose);

    PStringArray m_endpointPrefixes;

    PSyncPoint m_endRun;
    bool       m_interrupted;
    bool       m_verbose;
    ostream  * m_outputStream;

#if OPAL_STATISTICS
    PTimeInterval m_statsPeriod;
    PFilePath     m_statsFile;
    typedef map<PString, OpalMediaStatistics> StatsMap;
    StatsMap m_statistics;
    PMutex   m_statsMutex;
    virtual bool OutputStatistics();
    virtual bool OutputStatistics(ostream & strm);
    virtual bool OutputCallStatistics(ostream & strm, OpalCall & call);
    virtual bool OutputStreamStatistics(ostream & strm, const OpalMediaStream & stream);
#endif
};

typedef class OpalConsoleManager OpalManagerConsole;

#if P_CLI

/**OPAL manager class for applications with command line interpreter.
   An OpalManager derived class for use in a console application, providing
   a standard set of command line arguments for configuring many system
   parameters,  and a standard command line interpreter for control of many
   functions. Used by the sample applications such as faxopal, ovropal etc.
  */
class OpalManagerCLI : public OpalConsoleManager
{
    PCLASSINFO(OpalManagerCLI, OpalConsoleManager);

  public:
    OpalManagerCLI(
      const char * endpointPrefixes = OPAL_CONSOLE_PREFIXES
    );
    ~OpalManagerCLI();

    virtual PString GetArgumentSpec() const;
    virtual bool Initialise(
      PArgList & args,
      bool verbose,
      const PString & defaultRoute = PString::Empty()
    );
    virtual void Run();
    virtual void EndRun(bool interrupt);

  protected:
    PCLI * CreateCLIStandard();
#if P_TELNET
    PCLITelnet * CreateCLITelnet(WORD port);
#endif
#if P_CURSES
    PCLICurses * CreateCLICurses();
#endif

#if P_NAT
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdNat);
#endif

#if PTRACING
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdTrace);
#endif

#if OPAL_STATISTICS
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdStatistics);
#endif

    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdListCodecs);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdDelay);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdVersion);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdQuit);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdShutDown);

    PCLI * m_cli;
};

#endif // P_CLI


/**Create a process for OpalConsoleManager based applications.
  */
template <class Manager,                   ///< Class of OpalConsoleManager derived class
          const char Manuf[],              ///< Name of manufacturer
          const char Name[],               ///< Name of product
          WORD MajorVersion = OPAL_MAJOR,  ///< Major version number of the product
          WORD MinorVersion = OPAL_MINOR,  ///< Minor version number of the product
          PProcess::CodeStatus Status = PProcess::ReleaseCode, ///< Development status of the product
          WORD BuildNumber = OPAL_BUILD,   ///< Build number of the product
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
