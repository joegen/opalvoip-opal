/*
 * main.h
 *
 * OPAL application source file for Interactive Voice Response using VXML
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

#ifndef _IvrOPAL_MAIN_H
#define _IvrOPAL_MAIN_H

#if OPAL_IVR

class MyIVREndPoint : public OpalIVREndPoint
{
    PCLASSINFO(MyIVREndPoint, OpalIVREndPoint)

  public:
    MyIVREndPoint(OpalManager & manager) : OpalIVREndPoint(manager) { }

    virtual void OnEndDialog(OpalIVRConnection & connection);
};

#endif

class MyManager : public OpalManagerConsole
{
    PCLASSINFO(MyManager, OpalManagerConsole)

  public:
    MyManager() : OpalManagerConsole(OPAL_CONSOLE_PREFIXES OPAL_PCSS_PREFIX) { }

#if OPAL_IVR
    virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute = PString::Empty());
    virtual void Usage(ostream & strm, const PArgList & args);
    virtual void OnEstablishedCall(OpalCall & call); // Callback override
    virtual void OnClearedCall(OpalCall & call); // Callback override
#endif
};


#endif  // _IvrOPAL_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
