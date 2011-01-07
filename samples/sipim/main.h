/*
 * main.h
 *
 * OPAL application source file for seing IM via SIP
 *
 * Copyright (c) 2008 Post Increment
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

#ifndef _SipIM_MAIN_H
#define _SipIM_MAIN_H

#include <opal/manager.h>
#include <sip/sipep.h>
#include <ptclib/cli.h>

#if !OPAL_HAS_IM
#error Cannot compile IM sample program without IM!
#endif

class MyManager : public OpalManager
{
    PCLASSINFO(MyManager, OpalManager)

  public:
    virtual void OnClearedCall(OpalCall & call); // Callback override
    void OnApplyStringOptions(OpalConnection & conn, OpalConnection::StringOptions & options);

    virtual void OnMessageReceived(const OpalIM & message);

    PSyncPoint m_connected;
    PSyncPoint m_completed;
    PString m_callToken;
    OpalMediaFormat m_imFormat;
};

class MyPCSSEndPoint : public OpalPCSSEndPoint
{
  PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);

  public:
    MyPCSSEndPoint(MyManager & manager)
      : OpalPCSSEndPoint(manager)
    { }

    virtual PBoolean OnShowIncoming(const OpalPCSSConnection & connection);
    virtual PBoolean OnShowOutgoing(const OpalPCSSConnection & connection);
};

class SipIM : public PProcess
{
    PCLASSINFO(SipIM, PProcess)

  public:
    SipIM();
    ~SipIM();

    enum Mode {
      Use_MSRP,
      Use_SIPIM,
      Use_T140
    };

    virtual void Main();

  protected:
    bool CheckForVar(PString & var);
    PStringToString m_variables;

  private:
    PDECLARE_NOTIFIER(PCLI::Arguments, SipIM, CmdSet);
    PDECLARE_NOTIFIER(PCLI::Arguments, SipIM, CmdSend);
    //PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdList);
    //PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdSubscribeToPresence);
    //PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdUnsubscribeToPresence);
    //PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdPresenceAuthorisation);
    //PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdSetLocalPresence);
    //PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdBuddyList);
    //PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdBuddyAdd);
    //PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdBuddyRemove);
    //PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdBuddySusbcribe);
    
    PDECLARE_NOTIFIER(PCLI::Arguments, SipIM, CmdStun);
    PDECLARE_NOTIFIER(PCLI::Arguments, SipIM, CmdTranslate);
    PDECLARE_NOTIFIER(PCLI::Arguments, SipIM, CmdRegister);
    PDECLARE_NOTIFIER(PCLI::Arguments, SipIM, CmdDelay);
    PDECLARE_NOTIFIER(PCLI::Arguments, SipIM, CmdQuit);

  private:
    MyManager * m_manager;
    SIPEndPoint * m_sipEP;
};


#endif  // _SipIM_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
