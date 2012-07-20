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


///////////////////////////////////////////////////////////////////////////////

class MyManager;
class CallThread;

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

class MyManager : public OpalManager
{
    PCLASSINFO(MyManager, OpalManager);
  public:
    MyManager()
      { }

    virtual OpalCall * CreateCall(void * userData);

    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);

    PINDEX GetActiveCalls() const { return activeCalls.GetSize(); }
};


///////////////////////////////////////////////////////////////////////////////

class CallGen;

struct CallParams
{
  CallParams(CallGen & app)
    : callgen(app) { }

  CallGen & callgen;

  unsigned repeat;
  PTimeInterval tmax_est;
  PTimeInterval tmin_call;
  PTimeInterval tmax_call;
  PTimeInterval tmin_wait;
  PTimeInterval tmax_wait;
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
    PSyncPoint   m_exit;
};

PLIST(CallThreadList, CallThread);


///////////////////////////////////////////////////////////////////////////////

class CallGen : public PProcess 
{
  PCLASSINFO(CallGen, PProcess)

  public:
    CallGen();

    virtual void Main();
    virtual bool OnInterrupt(bool);

    static CallGen & Current() { return (CallGen&)PProcess::Current(); }

    MyManager  m_manager;
    PURL       m_outgoingMessageFile;
    PString    m_incomingAudioDirectory;
    PTextFile  m_cdrFile;

    PSyncPoint m_threadEnded;
    unsigned   m_totalAttempts;
    unsigned   m_totalEstablished;
    bool       m_quietMode;
    PMutex     m_coutMutex;

    PSyncPoint     m_completed;
    CallThreadList m_threadList;
};


///////////////////////////////////////////////////////////////////////////////
