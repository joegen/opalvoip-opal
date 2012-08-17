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

#if OPAL_HAS_MIXER
#else
#error Cannot compile MCU test program without OPAL_HAS_MIXER set!
#endif

#if OPAL_IVR
#else
#error Cannot compile MCU test program without OPAL_IVR set!
#endif


class MyManager;
class MyMixerEndPoint;


struct MyMixerNodeInfo : public OpalMixerNodeInfo
{
  virtual OpalMixerNodeInfo * Clone() const { return new MyMixerNodeInfo(*this); }

  PString m_moderatorPIN;
};


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
    MyMixerEndPoint(MyManager & manager);

    virtual OpalMixerConnection * CreateConnection(
      PSafePtr<OpalMixerNode> node,
      OpalCall & call,
      void * userData,
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );

    virtual OpalMixerNode * CreateNode(
      OpalMixerNodeInfo * info ///< Initial info for node
    );

    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfAdd);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfList);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfRemove);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfRecord);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfPlay);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdConfListen);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdMemberAdd);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdMemberList);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyMixerEndPoint, CmdMemberRemove);

    bool CmdConfXXX(PCLI::Arguments & args, PSafePtr<OpalMixerNode> & node, PINDEX argCount);

    MyManager & m_manager;
};


#if OPAL_PCSS

class MyPCSSEndPoint : public OpalPCSSEndPoint
{
    PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);
  public:
    MyPCSSEndPoint(OpalManager & manager)
      : OpalPCSSEndPoint(manager)
    {
    }

    virtual PBoolean OnShowIncoming(const OpalPCSSConnection & connection)
    {
      return AcceptIncomingCall(connection.GetToken());
    }

    virtual PBoolean OnShowOutgoing(const OpalPCSSConnection &)
    {
      return true;
    }
};

#endif // OPAL_PCSS


class MyManager : public OpalManagerCLI
{
    PCLASSINFO(MyManager, OpalManagerCLI)

  public:
    MyManager();

    PString GetArgumentSpec() const;
    bool Initialise(PArgList & args, bool verbose);

    virtual void OnEstablishedCall(OpalCall & call);
    virtual void OnClearedCall(OpalCall & call);

    void Broadcast(const PString & str) { m_cli->Broadcast(str); }

    MyMixerEndPoint * m_mixer;
};


#endif  // _OPAL_MCU_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
