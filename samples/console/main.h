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
 * $Revision: 28208 $
 * $Author: rjongbloed $
 * $Date: 2012-08-17 11:54:46 +1000 (Fri, 17 Aug 2012) $
 */

#ifndef _OPAL_MAIN_H
#define _OPAL_MAIN_H

class MyManager;


class MyManager : public OpalManagerCLI
{
    PCLASSINFO(MyManager, OpalManagerCLI)

  public:
    MyManager();

    PString GetArgumentSpec() const;
    bool Initialise(PArgList & args, bool verbose);

  protected:
    virtual bool OnLocalIncomingCall(OpalCall & call);
    virtual bool OnLocalOutgoingCall(OpalCall & call);
    virtual void OnEstablishedCall(OpalCall & call);
    virtual void OnClearedCall(OpalCall & call);

    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdCall);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdAnswer);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdHold);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdRetrieve);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdTransfer);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdHangUp);

    OpalPCSSEndPoint * m_pcss;
    bool               m_autoAnswer;
    PSafePtr<OpalCall> m_activeCall;
    PSafePtr<OpalCall> m_heldCall;
};


#endif  // _OPAL_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
