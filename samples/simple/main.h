/*
 * main.h
 *
 * A simple OPAL "net telephone" application.
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: main.h,v $
 * Revision 1.2002  2001/08/01 06:19:00  robertj
 * Added flags for disabling H.323 or Quicknet endpoints.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.5  2001/03/21 04:52:40  robertj
 * Added H.235 security to gatekeepers, thanks Fürbass Franz!
 *
 * Revision 1.4  2001/03/20 23:42:55  robertj
 * Used the new PTrace::Initialise function for starting trace code.
 *
 * Revision 1.3  2000/07/31 14:08:09  robertj
 * Added fast start and H.245 tunneling flags to the H323Connection constructor so can
 *    disabled these features in easier manner to overriding virtuals.
 *
 * Revision 1.2  2000/06/07 05:47:56  robertj
 * Added call forwarding.
 *
 * Revision 1.1  2000/05/11 04:05:57  robertj
 * Simple sample program.
 *
 */

#ifndef _SimpleOpal_MAIN_H
#define _SimpleOpal_MAIN_H

#include <opal/manager.h>
#include <opal/pcss.h>
#include <lids/ixjlid.h>


class MyManager;


class MyPCSSEndPoint : public OpalPCSSEndPoint
{
  PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);

  public:
    MyPCSSEndPoint(MyManager & manager);

    void OnShowRinging(const PString & callerName);
    BOOL OnShowAlerting(const PString & calleeName);

    BOOL SetSoundDevice(PArgList & args, const char * optionName, PSoundChannel::Directions dir);
};


class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager);

  public:
    MyManager();
    ~MyManager();

    BOOL Initialise(PArgList & args);
    void Main(PArgList & args);

    PString OnRouteConnection(OpalConnection & connection);
    BOOL OnConnectionForwarded(OpalConnection & connection, const PString & forwardParty);
    void OnEstablishedConnection(OpalConnection & connection);
    void OnReleasedConnection(OpalConnection & connection);
    BOOL OnOpenMediaStream(OpalMediaStream & stream);
    void OnUserIndicationString(OpalConnection & connection, const PString & value);

  protected:
    PString currentCallToken;
    BOOL silenceOn;
    BOOL autoAnswer;
    BOOL noFastStart;
    BOOL noH245Tunnelling;
    PString busyForwardParty;

    OpalIxJDevice xJack;
    OpalPOTSEndPoint * potsEP;
    MyPCSSEndPoint * pcssEP;
    H323EndPoint * h323EP;
};


class SimpleOpalProcess : public PProcess
{
  PCLASSINFO(SimpleOpalProcess, PProcess)

  public:
    SimpleOpalProcess();

    void Main();

  protected:
    MyManager * opal;
};


#endif  // _SimpleOpal_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
