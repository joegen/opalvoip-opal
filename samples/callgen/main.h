/*
 * main.h
 *
 * OPAL call generator
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
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
 * The Original Code is CallGen.
 *
 * Contributor(s): Equivalence Pty. Ltd.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


class MyManager;

///////////////////////////////////////////////////////////////////////////////

struct CallParams
{
  CallParams(MyManager & mgr)
    : m_callgen(mgr) { }

  MyManager & m_callgen;

  unsigned      m_repeat;
  PTimeInterval m_tmax_est;
  PTimeInterval m_tmin_call;
  PTimeInterval m_tmax_call;
  PTimeInterval m_tmin_wait;
  PTimeInterval m_tmax_wait;
};


///////////////////////////////////////////////////////////////////////////////

class CallThread : public PThread
{
    PCLASSINFO(CallThread, PThread);
  public:
    CallThread(
      unsigned index,
      const PStringArray & destinations,
      const CallParams & params
    );

    void Main();
    void Stop();

    PStringArray m_destinations;
    unsigned     m_index;
    CallParams   m_params;
    bool         m_running;
    PSyncPoint   m_exit;
};

PLIST(CallThreadList, CallThread);


///////////////////////////////////////////////////////////////////////////////

class MyCall : public OpalCall
{
    PCLASSINFO(MyCall, OpalCall);
  public:
    MyCall(MyManager & manager, CallThread * caller);

    virtual void OnNewConnection(OpalConnection & connection);
    virtual void OnEstablishedCall();
    virtual void OnCleared();

    void OnOpenMediaStream(OpalMediaStream & stream);

    MyManager          & m_manager;
    unsigned             m_index;
    PString              m_callIdentifier;
    PTime                m_openedTransmitMedia;
    PTime                m_openedReceiveMedia;
    PTime                m_receivedMedia;
    OpalTransportAddress m_mediaGateway;
};


///////////////////////////////////////////////////////////////////////////////

class MyPCSSEndPoint : public OpalPCSSEndPoint
{
    PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);
  public:
    MyPCSSEndPoint(OpalManager & mgr) : OpalPCSSEndPoint(mgr) { }
    virtual PBoolean OnShowIncoming(const OpalPCSSConnection &);
    virtual PBoolean OnShowOutgoing(const OpalPCSSConnection &);
};


///////////////////////////////////////////////////////////////////////////////

class MyManager : public OpalManagerConsole
{
    PCLASSINFO(MyManager, OpalManagerConsole);
  public:
    MyManager();
    ~MyManager();

    virtual PString GetArgumentSpec() const;
    virtual void Usage(ostream & strm, const PArgList & args);
    virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute = PString::Empty());
    virtual void Run();

    virtual OpalCall * CreateCall(void * userData);

    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);

    PINDEX GetActiveCalls() const { return activeCalls.GetSize(); }

    PTextFile      m_cdrFile;
    unsigned       m_totalCalls;
    unsigned       m_totalEstablished;
    CallThreadList m_threadList;
};


///////////////////////////////////////////////////////////////////////////////
