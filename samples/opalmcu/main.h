/*
 * main.h
 *
 * OPAL application source file for EXTREMELY simple Multipoint Conferencing Unit
 *
 * Copyright (c) 2008 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _OPAL_MCU_MAIN_H
#define _OPAL_MCU_MAIN_H


class MyMixerEndPoint;

class MyMixerConnection : public OpalMixerConnection
{
    PCLASSINFO(MyMixerConnection, OpalMixerConnection);
  public:
    MyMixerConnection(
      PSafePtr<OpalMixerNode> node,
      OpalCall & call,
      MyMixerEndPoint & endpoint,
      void * userData,
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );

    virtual bool SendUserInputString(const PString & value);

  protected:
    MyMixerEndPoint & m_endpoint;
    PString           m_userInput;
};


class MyMixerEndPoint : public OpalMixerEndPoint
{
    PCLASSINFO(MyMixerEndPoint, OpalMixerEndPoint);
  public:
    MyMixerEndPoint(OpalManager & manager, PArgList & args);

    virtual OpalMixerConnection * CreateConnection(
      PSafePtr<OpalMixerNode> node,
      OpalCall & call,
      void * userData,
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );

    const PString & GetModeratorPIN() const { return m_moderatorPIN; }

    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfAdd);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfList);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfMembers);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfRemove);

  protected:
    PString m_moderatorPIN;
};


class MyManager : public OpalManager
{
    PCLASSINFO(MyManager, OpalManager)

  public:
    virtual void OnEstablishedCall(OpalCall & call);
    virtual void OnClearedCall(OpalCall & call);
};


class ConfOPAL : public PProcess
{
    PCLASSINFO(ConfOPAL, PProcess)

  public:
    ConfOPAL();
    ~ConfOPAL();

    virtual void Main();

  private:
    PDECLARE_NOTIFIER(PCLI::Arguments, ConfOPAL, CmdQuit);

    MyManager * m_manager;
};


#endif  // _OPAL_MCU_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
