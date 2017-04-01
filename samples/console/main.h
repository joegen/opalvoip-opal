/*
 * main.h
 *
 * OPAL application source file for console mode OPAL videophone
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
 */

#ifndef _OPAL_MAIN_H
#define _OPAL_MAIN_H

class MyManager;


class MyManager : public OpalManagerCLI
{
    PCLASSINFO(MyManager, OpalManagerCLI)

  public:
    MyManager();

    virtual PString GetArgumentSpec() const;
    virtual void Usage(ostream & strm, const PArgList & args);
    virtual bool Initialise(PArgList & args, bool verbose);

  protected:
    virtual bool OnLocalIncomingCall(OpalLocalConnection & connection);
    virtual void OnClearedCall(OpalCall & call);
    bool SetAutoAnswer(ostream & output, bool verbose, const PArgList & args, const char * option);
    PDECLARE_NOTIFIER(PTimer, MyManager, AutoAnswer);

    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdAutoAnswer);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdSpeedDial);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdCall);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdAnswer);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdHold);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdRetrieve);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdTransfer);

    PTimeInterval      m_autoAnswerTime;
    PTimer             m_autoAnswerTimer;
    PStringToString    m_speedDial;
    PSafePtr<OpalCall> m_activeCall;
    PSafePtr<OpalCall> m_heldCall;
};


#endif  // _OPAL_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
