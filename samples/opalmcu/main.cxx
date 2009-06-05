/*
 * main.cxx
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

#include "precompile.h"
#include "main.h"


PCREATE_PROCESS(ConfOPAL);


// Debug command: -m 123 -V -S udp$*:25060 -H tcp$*:21720 -ttttodebugstream


ConfOPAL::ConfOPAL()
  : PProcess("Vox Gratia", "ConfOPAL", 1, 0, ReleaseCode, 0)
  , m_manager(NULL)
{
}


ConfOPAL::~ConfOPAL()
{
  delete m_manager;
}


void ConfOPAL::Main()
{
  PArgList & args = GetArguments();

  args.Parse("g-gk-host:"
             "G-gk-id:"
             "h-help."
             "H-h323:"
             "m-moderator:"
             "n-name:"
             "N-stun:"
             "o-output:"
             "p-password:"
             "P-proxy:"
             "r-register:"
             "s-size:"
             "S-sip:"
             "t-trace."
             "u-user:"
             "V-no-video."
             , FALSE);

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('h')) {
    cerr << "usage: " << GetFile().GetTitle() << " [ options ] [ name ]\n"
            "\n"
            "Available options are:\n"
            "  -h or --help            : print this help message.\n"
            "  -n or --name alias      : Name of default conference\n"
            "  -m or --moderator pin   : PIN to allow to become a moderator and have talk rights\n"
            "                          : if absent, all participants are moderators.\n"
#if OPAL_VIDEO
            "  -V or --no-video        : Disable video\n"
            "  -s or --size            : Set default video size\n"
#endif
            "  -u or --user name       : Set local username, defaults to OS username.\n"
            "  -p or --password pwd    : Set password for authentication.\n"
#if OPAL_SIP
            "  -S or --sip interface   : SIP listens on interface, defaults to udp$*:5060, 'x' disables.\n"
            "  -r or --register server : SIP registration to server.\n"
            "  -P or --proxy url       : SIP outbound proxy.\n"
#endif
#if OPAL_H323
            "  -H or --h323 interface  : H.323 listens on interface, defaults to tcp$*:1720, 'x' disables.\n"
            "  -g or --gk-host host    : H.323 gatekeeper host.\n"
            "  -G or --gk-id id        : H.323 gatekeeper identifier.\n"
#endif
            "  -N or --stun server     : Set NAT traversal STUN server.\n"
#if PTRACING
            "  -o or --output file     : file name for output of log messages\n"       
            "  -t or --trace           : degree of verbosity in error log (more times for more detail)\n"     
#endif
            "\n"
            "\n";
    return;
  }

  m_manager = new MyManager();

  if (args.HasOption('N')) {
    PSTUNClient::NatTypes nat = m_manager->SetSTUNServer(args.GetOptionString('N'));
    cout << "STUN server \"" << m_manager->GetSTUNClient()->GetServer() << "\" replies " << nat;
    PIPSocket::Address externalAddress;
    if (nat != PSTUNClient::BlockedNat && m_manager->GetSTUNClient()->GetExternalAddress(externalAddress))
      cout << " with address " << externalAddress;
    cout << endl;
  }

  if (args.HasOption('u'))
    m_manager->SetDefaultUserName(args.GetOptionString('u'));

  PCaselessString interfaces;

#if OPAL_SIP
  // Set up SIP
  interfaces = args.GetOptionString('S');
  if (interfaces != "x") {
    SIPEndPoint * sip  = new SIPEndPoint(*m_manager);
    if (!sip->StartListeners(interfaces.Lines())) {
      cerr << "Could not start SIP listeners." << endl;
      return;
    }

    if (args.HasOption('P'))
      sip->SetProxy(args.GetOptionString('P'), args.GetOptionString('u'), args.GetOptionString('p'));

    if (args.HasOption('r')) {
      SIPRegister::Params params;
      params.m_addressOfRecord = args.GetOptionString('r');
      params.m_password = args.GetOptionString('p');
      params.m_expire = 300;

      PString aor;
      if (!sip->Register(params, aor)) {
        cerr << "Could not start SIP registration to " << params.m_addressOfRecord << endl;
        return;
      }
    }

    m_manager->AddRouteEntry("sip.*:.* = mcu:<du>");
    m_manager->AddRouteEntry("mcu:.* = sip:<da>");
  }
#endif // OPAL_SIP


#if OPAL_H323
  // Set up H.323
  interfaces = args.GetOptionString('H');
  if (interfaces != "x") {
    H323EndPoint * h323 = new H323EndPoint(*m_manager);
    if (!h323->StartListeners(interfaces.Lines())) {
      cerr << "Could not start H.323 listeners." << endl;
      return;
    }

    if (args.HasOption('g') || args.HasOption('G'))
      h323->UseGatekeeper(args.GetOptionString('g'), args.GetOptionString('G'));

    m_manager->AddRouteEntry("h323.*:.* = mcu:<du>");
    m_manager->AddRouteEntry("mcu:.* = h323:<da>");
  }
#endif // OPAL_H323


  // Set up IVR
  MyMixerEndPoint * mixer = new MyMixerEndPoint(*m_manager, args);

  // Wait for call to come in and finish
  PCLIStandard cli("MCU> ");

  cli.SetCommand("conf add", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfAdd),
                 "Add a new conferance:",
#if OPAL_VIDEO
                 "[ -V ] [ -s size ] <name> [ <name> ... ]\n"
                 "  -V or --no-video : Disable video\n"
                 "  -s or --size     : Set video size"
#else
                 "conf add <name>"
#endif
                 );
  cli.SetCommand("conf list", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfList),
                 "List conferances");
  cli.SetCommand("conf members", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfMembers),
                 "List members in conferance:",
                 "{ <name> | <guid> }");
  cli.SetCommand("conf remove", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfRemove),
                 "Remove conferance:",
                 "{ <name> | <guid> }");
  cli.SetCommand("quit\nq\nexit", PCREATE_NOTIFIER(CmdQuit),"Quit application");

  cli.Start(false); // Do not spawn thread, wait till end of input

  cout << "\nExiting ..." << endl;
}


void ConfOPAL::CmdQuit(PCLI::Arguments & args, INT)
{
  args.GetContext().Stop();
}


void MyManager::OnEstablishedCall(OpalCall & call)
{
  cout << "Call from " << call.GetPartyA() << " entered conference at " << call.GetPartyB() << endl;
}


void MyManager::OnClearedCall(OpalCall & call)
{
  cout << "Call from " << call.GetPartyA() << " left conference at " << call.GetPartyB() << endl;
}


///////////////////////////////////////////////////////////////

MyMixerEndPoint::MyMixerEndPoint(OpalManager & manager, PArgList & args)
  : OpalMixerEndPoint(manager, "mcu")
  , m_moderatorPIN(args.GetOptionString('m'))
{
  m_adHocNodeInfo.m_name = args.GetOptionString('n', "room101");
  m_adHocNodeInfo.m_listenOnly = !m_moderatorPIN.IsEmpty();

#if OPAL_VIDEO
  m_adHocNodeInfo.m_audioOnly = args.HasOption('V');
  if (args.HasOption('s'))
    PVideoFrameInfo::ParseSize(args.GetOptionString('s'), m_adHocNodeInfo.m_width, m_adHocNodeInfo.m_height);
#endif
}


OpalMixerConnection * MyMixerEndPoint::CreateConnection(PSafePtr<OpalMixerNode> node,
                                                        OpalCall & call,
                                                        void * userData,
                                                        unsigned options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new MyMixerConnection(node, call, *this, userData, options, stringOptions);
}


void MyMixerEndPoint::CmdConfAdd(PCLI::Arguments & args, INT)
{
  args.Parse("s-size:V-no-video.");
  if (args.GetCount() == 0) {
    args.WriteUsage();
    return;
  }

  for (PINDEX i = 0; i < args.GetCount(); ++i) {
    if (m_nodesByName.Contains(args[i])) {
      args.WriteError("conference name already exists");
      return;
    }
  }

  OpalMixerNodeInfo * info = new OpalMixerNodeInfo();
  info->m_name = args[0];
  info->m_audioOnly = args.HasOption('V');
  if (args.HasOption('s'))
    PVideoFrameInfo::ParseSize(args.GetOptionString('s'), info->m_width, info->m_height);

  PSafePtr<OpalMixerNode> node = AddNode(info);

  if (node == NULL)
    args.WriteError("could not create conference");
  else {
    for (PINDEX i = 1; i < args.GetCount(); ++i)
      node->AddName(args[i]);
    PCLI::Context & out = args.GetContext();
    out << "Added conference " << *node << out.GetNewLine() << flush;
  }
}


void MyMixerEndPoint::CmdConfList(PCLI::Arguments & args, INT)
{
  PCLI::Context & out = args.GetContext();
  for (PSafePtr<OpalMixerNode> node(m_nodesByUID, PSafeReadOnly); node != NULL; ++node)
    out << *node << out.GetNewLine();
  out.flush();
}


void MyMixerEndPoint::CmdConfMembers(PCLI::Arguments & args, INT)
{
  if (args.GetCount() == 0) {
    args.WriteUsage();
    return;
  }

  PSafePtr<OpalMixerNode> node = FindNode(args[0]);
  if (node == NULL) {
    args.WriteError("conference does not exist");
    return;
  }

  PCLI::Context & out = args.GetContext();
  for (PSafePtr<OpalMixerConnection> connection = node->GetFirstConnection(); connection != NULL; ++connection)
    out << connection->GetToken() << ' '
        << connection->GetRemotePartyURL() << " \""
        << connection->GetRemotePartyName() << '"' << out.GetNewLine();
  out.flush();
}


void MyMixerEndPoint::CmdConfRemove(PCLI::Arguments & args, INT)
{
  if (args.GetCount() == 0) {
    args.WriteUsage();
    return;
  }

  PSafePtr<OpalMixerNode> node = FindNode(args[0]);
  if (node == NULL) {
    args.WriteError("conference does not exist");
    return;
  }

  PCLI::Context & out = args.GetContext();
  out << "Removed conference " << *node << out.GetNewLine() << flush;

  RemoveNode(*node);
}


///////////////////////////////////////////////////////////////

MyMixerConnection::MyMixerConnection(PSafePtr<OpalMixerNode> node,
                                     OpalCall & call,
                                     MyMixerEndPoint & ep,
                                     void * userData,
                                     unsigned options,
                                     OpalConnection::StringOptions * stringOptions)
  : OpalMixerConnection(node, call, ep, userData, options, stringOptions)
  , m_endpoint(ep)
{
}


bool MyMixerConnection::SendUserInputString(const PString & value)
{
  m_userInput += value;

  if (m_endpoint.GetModeratorPIN().NumCompare(m_userInput) != EqualTo)
    m_userInput.MakeEmpty();
  else if (m_endpoint.GetModeratorPIN() == m_userInput) {
    m_userInput.MakeEmpty();
    SetListenOnly(false);
  }

  return true;
}


// End of File ///////////////////////////////////////////////////////////////
