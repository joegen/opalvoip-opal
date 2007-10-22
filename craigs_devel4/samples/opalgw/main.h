/*
 * main.h
 *
 * PWLib application header file for OPAL Gateway
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * Revision 1.1  2003/03/26 02:49:00  robertj
 * Added service/daemon sample application.
 *
 */

#ifndef _OpalGw_MAIN_H
#define _OpalGw_MAIN_H


#include <opal/manager.h>

#if P_SSL
#include <ptclib/shttpsvc.h>
typedef PSecureHTTPServiceProcess OpalGwProcessAncestor;
#else
#include <ptclib/httpsvc.h>
typedef PHTTPServiceProcess OpalGwProcessAncestor;
#endif


class SIPEndPoint;
class H323EndPoint;
class OpalPOTSEndPoint;
class OpalPSTNEndPoint;
class OpalIVREndPoint;


class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager);

  public:
    MyManager();
    ~MyManager();

    BOOL Initialise(PConfig & cfg, PConfigPage * rsrc);

  protected:
    H323EndPoint     * h323EP;
    SIPEndPoint      * sipEP;
    OpalPOTSEndPoint * potsEP;
    OpalPSTNEndPoint * pstnEP;
#if P_EXPAT
    OpalIVREndPoint  * ivrEP;
#endif
};


class OpalGw : public OpalGwProcessAncestor
{
  PCLASSINFO(OpalGw, OpalGwProcessAncestor)

  public:
    OpalGw();
    virtual void Main();
    virtual BOOL OnStart();
    virtual void OnStop();
    virtual void OnControl();
    virtual void OnConfigChanged();
    virtual BOOL Initialise(const char * initMsg);

  private:
    MyManager manager;
};


#endif  // _OpalGw_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
