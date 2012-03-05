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

#include <opal/manager.h>
#include <ptclib/cli.h>

#if P_CLI

class SIPEndPoint;
class H323EndPoint;
class OpalLineEndPoint;
class OpalCapiEndPoint;


/**OPAL manager class for console applications.
   An OpalManager derived class for use in a console application, providing
   a standard set of command line arguments for configuring many system
   parameters. Used by the sample applications such as faxopal, ovropal etc.
  */
class OpalManagerConsole : public OpalManager
{
    PCLASSINFO(OpalManagerConsole, OpalManager);

  public:
    OpalManagerConsole();

    virtual PString GetArgumentSpec() const;
    virtual PString GetArgumentUsage() const;
    virtual void Usage(ostream & strm, const PArgList & args);

    virtual bool Initialise(
      PArgList & args,
      bool verbose,
      const PString & defaultRoute = PString::Empty()
    );
    virtual void Run();
    virtual void EndRun();

  protected:
#if OPAL_SIP
    SIPEndPoint * CreateSIPEndPoint();
#endif
#if OPAL_H323
    H323EndPoint * CreateH323EndPoint();
#endif
#if OPAL_LID
    OpalLineEndPoint * CreateLineEndPoint();
#endif
#if OPAL_CAPI
    OpalCapiEndPoint * CreateCapiEndPoint();
#endif

    PSyncPoint m_endRun;
};


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
    OpalManagerCLI();
    ~OpalManagerCLI();

    virtual PString GetArgumentSpec() const;
    virtual bool Initialise(
      PArgList & args,
      bool verbose,
      const PString & defaultRoute = PString::Empty()
    );
    virtual void Run();
    virtual void EndRun();

  protected:
    PCLI * CreatePCLI(
#if P_TELNET
      WORD port /// Port to listen for telnet sessions, zero disables.
#endif
    );

#if OPAL_SIP
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdRegister);
#endif

#if P_NAT
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdNat);
#endif

#if PTRACING
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdTrace);
#endif

    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdListCodecs);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdDelay);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdVersion);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdQuit);
    PDECLARE_NOTIFIER(PCLI::Arguments, OpalManagerCLI, CmdShutDown);

    PCLI * m_cli;
};


/**Create a process for OpalConsoleManager based applications.
  */
template <class Manager,                   ///< Class of OpalManagerConsole derived class
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

  private:
    Manager * m_manager;
};


#endif // P_CLI

#endif // OPAL_OPAL_CONSOLE_MGR_H


/////////////////////////////////////////////////////////////////////////////
