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


extern const char Manufacturer[] = "Vox Gratia";
extern const char Application[] = "OPAL Conference";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);

// Debug command: -m 123 -V -S udp$*:25060 -H tcp$*:21720 -ttttodebugstream


MyManager::MyManager()
  : m_mixer(NULL)
{
}


PString MyManager::GetArgumentSpec() const
{
  PString spec = OpalManagerCLI::GetArgumentSpec();
  spec.Replace("V-version", "-version"); // Want the 'V' option back

  return "[Application options:]"
         "a-attendant: VXML script to run for incoming calls not directed\r"
                      "to a specific conference.\n"
         "m-moderator: PIN to allow to become a moderator and have talk\r"
                      "rights if absent, all participants are moderators.\n"
         "n-name:      Default name for ad-hoc conference.\n"
         "f-factory:   Name for factory URI name.\n"
#if OPAL_VIDEO
         "s-size:      Set default video size for ad-hoc conference.\n"
         "V-no-video.  Disable video for ad-hoc conference.\n"
#endif
         "-pass-thru.  Enable media pass through optimisation.\n"
         + spec;
}


bool MyManager::Initialise(PArgList & args, bool verbose)
{
  if (!OpalManagerCLI::Initialise(args, verbose, "mcu:<du>"))
    return false;

#if OPAL_PCSS
  // Set up PCSS to do speaker playback
  new MyPCSSEndPoint(*this);
#endif

  // Set up IVR to do recording or WAV file play
  OpalIVREndPoint * ivr = new OpalIVREndPoint(*this);

  // Set up conference mixer
  m_mixer = new MyMixerEndPoint(*this);

  if (args.HasOption('a')) {
    PFilePath path = args.GetOptionString('a');
    ivr->SetDefaultVXML(path);
    AddRouteEntry(".*:.* = ivr:<da>");
    cout << "Using attendant IVR " << path << endl;
  }

  MyMixerNodeInfo info;
  info.m_moderatorPIN = args.GetOptionString('m');
  info.m_listenOnly = !info.m_moderatorPIN.IsEmpty();
  info.m_mediaPassThru = args.HasOption("pass-thru");

#if OPAL_VIDEO
  info.m_audioOnly = args.HasOption('V');
  if (args.HasOption('s'))
    PVideoFrameInfo::ParseSize(args.GetOptionString('s'), info.m_width, info.m_height);
  cout << "Video mixer resolution: " << info.m_width << 'x' << info.m_height << endl;
#endif

  if (args.HasOption('n')) {
    info.m_name = args.GetOptionString('n');
    m_mixer->SetAdHocNodeInfo(info);
    cout << "Ad-hoc conferences enabled with default name: " << info.m_name << endl;
  }

  if (args.HasOption('f')) {
    info.m_name = args.GetOptionString('f');
    m_mixer->SetFactoryNodeInfo(info);
    cout << "Factory conferences enabled with name: " << info.m_name << endl;
  }


  m_cli->SetPrompt("MCU> ");

  m_cli->SetCommand("conf add", PCREATE_NOTIFIER_EXT(m_mixer, MyMixerEndPoint, CmdConfAdd),
                    "Add a new conferance:",
#if OPAL_VIDEO
                    "[ -V ] [ -s size ] [ -m pin ] <name> [ <name> ... ]\n"
                    "  -V or --no-video       : Disable video\n"
                    "  -s or --size           : Set video size\n"
#else
                    "[ -m pin ] <name>\n"
#endif
                    "\n"
                    "  -m or --moderator pin  : PIN to allow to become a moderator and have talk rights\n"
                    "                         : if absent, all participants are moderators.\n"
                    "        --no-pass-thru   : Disable media pass through optimisation.\n"
                   );
  m_cli->SetCommand("conf list", PCREATE_NOTIFIER_EXT(m_mixer, MyMixerEndPoint, CmdConfList),
                    "List conferances");
  m_cli->SetCommand("conf remove", PCREATE_NOTIFIER_EXT(m_mixer, MyMixerEndPoint, CmdConfRemove),
                    "Remove conferance:",
                    "<name> | <guid>");
  m_cli->SetCommand("conf listen", PCREATE_NOTIFIER_EXT(m_mixer, MyMixerEndPoint, CmdConfListen),
                    "Toggle listening to conferance:",
                    "{ <conf-name> | <guid> }");
  m_cli->SetCommand("conf record", PCREATE_NOTIFIER_EXT(m_mixer, MyMixerEndPoint, CmdConfRecord),
                    "Record conferance:",
                    "{ <conf-name> | <guid> } { <filename> | stop }");
  m_cli->SetCommand("conf play", PCREATE_NOTIFIER_EXT(m_mixer, MyMixerEndPoint, CmdConfPlay),
                    "Play file to conferance:",
                    "{ <conf-name> | <guid> } <filename>");

  m_cli->SetCommand("member add", PCREATE_NOTIFIER_EXT(m_mixer, MyMixerEndPoint, CmdMemberAdd),
                    "Add a member to conferance:",
                    "{ <name> | <guid> } <address>");
  m_cli->SetCommand("member list", PCREATE_NOTIFIER_EXT(m_mixer, MyMixerEndPoint, CmdMemberList),
                    "List members in conferance:",
                    "<name> | <guid>");
  m_cli->SetCommand("member remove", PCREATE_NOTIFIER_EXT(m_mixer, MyMixerEndPoint, CmdMemberRemove),
                    "Remove conferance member:",
                    "{ <conf-name> | <guid> } <member-name>\n"
                    "member remove <call-token>");

  return true;
}


void MyManager::OnEstablishedCall(OpalCall & call)
{
  PStringStream strm;
  strm << "Call from " << call.GetPartyA() << " entered conference at " << call.GetPartyB();
  m_cli->Broadcast(strm);
}


void MyManager::OnClearedCall(OpalCall & call)
{
  PStringStream strm;
  strm << "Call from " << call.GetPartyA() << " left conference at " << call.GetPartyB();
  m_cli->Broadcast(strm);
}


///////////////////////////////////////////////////////////////

MyMixerEndPoint::MyMixerEndPoint(MyManager & manager)
  : OpalMixerEndPoint(manager, "mcu")
  , m_manager(manager)
{
}


OpalMixerConnection * MyMixerEndPoint::CreateConnection(PSafePtr<OpalMixerNode> node,
                                                        OpalCall & call,
                                                        void * userData,
                                                        unsigned options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new MyMixerConnection(node, call, *this, userData, options, stringOptions);
}


OpalMixerNode * MyMixerEndPoint::CreateNode(OpalMixerNodeInfo * info)
{
  PStringStream strm;
  strm << "Created new conference \"" << info->m_name << '"';
  m_manager.Broadcast(strm);

  return OpalMixerEndPoint::CreateNode(info);
}


void MyMixerEndPoint::CmdConfAdd(PCLI::Arguments & args, P_INT_PTR)
{
  args.Parse("s-size:V-no-video.-m-moderator:");
  if (args.GetCount() == 0) {
    args.WriteUsage();
    return;
  }

  for (PINDEX i = 0; i < args.GetCount(); ++i) {
    if (FindNode(args[i]) != NULL) {
      args.WriteError() << "Conference name \"" << args[i] << "\" already exists." << endl;
      return;
    }
  }

  MyMixerNodeInfo * info = new MyMixerNodeInfo();
  info->m_name = args[0];
#if OPAL_VIDEO
  info->m_audioOnly = args.HasOption('V');
  if (args.HasOption('s'))
    PVideoFrameInfo::ParseSize(args.GetOptionString('s'), info->m_width, info->m_height);
#endif
  info->m_moderatorPIN = args.GetOptionString('m');
  info->m_mediaPassThru = args.HasOption("pass-thru");

  PSafePtr<OpalMixerNode> node = AddNode(info);

  if (node == NULL)
    args.WriteError() << "Could not create conference \"" << args[0] << '"' << endl;
  else {
    for (PINDEX i = 1; i < args.GetCount(); ++i)
      node->AddName(args[i]);
    args.GetContext() << "Added conference " << *node << endl;
  }
}


void MyMixerEndPoint::CmdConfList(PCLI::Arguments & args, P_INT_PTR)
{
  ostream & out = args.GetContext();
  for (PSafePtr<OpalMixerNode> node = GetFirstNode(PSafeReadOnly); node != NULL; ++node)
    out << *node << '\n';
  out.flush();
}


bool MyMixerEndPoint::CmdConfXXX(PCLI::Arguments & args, PSafePtr<OpalMixerNode> & node, PINDEX argCount)
{
  if (args.GetCount() < argCount) {
    args.WriteUsage();
    return false;
  }

  node = FindNode(args[0]);
  if (node == NULL) {
    args.WriteError() << "Conference \"" << args[0] << "\" does not exist" << endl;
    return false;
  }

  return true;
}


void MyMixerEndPoint::CmdConfRemove(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalMixerNode> node;
  if (!CmdConfXXX(args, node, 1))
    return;

  RemoveNode(*node);
  args.GetContext() << "Removed conference " << *node << endl;
}


void MyMixerEndPoint::CmdConfListen(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalMixerNode> node;
  if (!CmdConfXXX(args, node, 1))
    return;

  for (PSafePtr<OpalConnection> connection = node->GetFirstConnection(); connection != NULL; ++connection) {
    if (connection->GetCall().GetPartyB().NumCompare("pc:") == EqualTo) {
      connection->Release();
      args.GetContext() << "Stopped listening to conference " << *node << endl;
      return;
    }
  }

  PString token;
  if (manager.SetUpCall("mcu:"+node->GetGUID().AsString()+";Listen-Only=1", "pc:*", token))
    args.GetContext() << "Started";
  else
    args.WriteError() << "Could not start";
  args.GetContext() << " listening to conference " << *node << endl;
}


void MyMixerEndPoint::CmdConfRecord(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalMixerNode> node;
  if (!CmdConfXXX(args, node, 2))
    return;

  for (PSafePtr<OpalConnection> connection = node->GetFirstConnection(); connection != NULL; ++connection) {
    if (connection->GetCall().GetPartyB().NumCompare("ivr:") == EqualTo)
      connection->Release();
  }

  if (args[1] *= "stop")
    return;

  PFilePath path = args[1];
  if (PFile::Exists(path)) {
    if (!PFile::Access(path, PFile::WriteOnly)) {
      args.WriteError() << "Cannot overwrite file \"" << path << '"' << endl;
      return;
    }
  }
  else {
    PDirectory dir = path.GetDirectory();
    if (!PFile::Access(dir, PFile::WriteOnly)) {
      args.WriteError() << "Cannot write to directory \"" << dir << '"' << endl;
      return;
    }
  }

  PString token;
  if (manager.SetUpCall("mcu:"+node->GetGUID().AsString()+";Listen-Only",
                        "ivr:<vxml>"
                              "<form>"
                                "<record name=\"msg\" dtmfterm=\"false\" dest=\"" + PURL(path).AsString() + "\"/>"
                              "</form>"
                            "</vxml>",
                        token))
    args.GetContext() << "Started";
  else
    args.WriteError() << "Could not start";
  args.GetContext() << " recording conference " << *node << " to \"" << path << '"' << endl;
}


void MyMixerEndPoint::CmdConfPlay(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalMixerNode> node;
  if (!CmdConfXXX(args, node, 2))
    return;

  PFilePath path = args[1];
  if (!PFile::Access(path, PFile::ReadOnly)) {
    args.WriteError() << "Cannot read from file \"" << path << '"' << endl;
    return;
  }

  PString token;
  if (manager.SetUpCall("mcu:"+node->GetGUID().AsString()+";Listen-Only=0",
                        "ivr:<vxml>"
                              "<form>"
                                "<audio src=\"" + PURL(path).AsString() + "\"/>"
                              "</form>"
                            "</vxml>",
                        token))
    args.GetContext() << "Started";
  else
    args.WriteError() << "Could not start";
  args.GetContext() << " playing \"" << path << "\" to conference " << *node << endl;
}


void MyMixerEndPoint::CmdMemberAdd(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalMixerNode> node;
  if (!CmdConfXXX(args, node, 2))
    return;

  PString token;
  if (manager.SetUpCall("mcu:"+node->GetGUID().AsString(), args[1], token))
    args.GetContext() << "Adding";
  else
    args.WriteError() << "Could not add";
  args.GetContext() << " new member \"" << args[1] << "\" to conference " << *node << endl;
}


void MyMixerEndPoint::CmdMemberList(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalMixerNode> node;
  if (!CmdConfXXX(args, node, 1))
    return;

  ostream & out = args.GetContext();
  for (PSafePtr<OpalConnection> connection = node->GetFirstConnection(); connection != NULL; ++connection)
    out << connection->GetToken() << ' '
        << connection->GetRemotePartyURL() << " \""
        << connection->GetRemotePartyName() << '"' << '\n';
  out.flush();
}


void MyMixerEndPoint::CmdMemberRemove(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() == 1) {
    if (ClearCall(args[0]))
      args.GetContext() << "Removed member";
    else
      args.WriteError() << "Member does not exist";
    args.GetContext() << " using token " << args[0] << endl;
    return;
  }

  PSafePtr<OpalMixerNode> node;
  if (!CmdConfXXX(args, node, 2))
    return;

  PCaselessString name = args[1];
  for (PSafePtr<OpalConnection> connection = node->GetFirstConnection(); connection != NULL; ++connection) {
    if (name == connection->GetRemotePartyName()) {
      connection->ClearCall();
      args.GetContext() << "Removed member using name \"" << name << '"' << endl;
      return;
    }
  }

  args.WriteError() << "No member in conference " << *node << " with name \"" << name << '"' << endl;
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

  PString pin = ((const MyMixerNodeInfo &)m_node->GetNodeInfo()).m_moderatorPIN;
  if (pin.NumCompare(m_userInput) != EqualTo)
    m_userInput.MakeEmpty();
  else if (pin == m_userInput) {
    m_userInput.MakeEmpty();
    SetListenOnly(false);

    PStringStream strm;
    strm << "Connection " << GetToken() << ' '
         << GetRemotePartyURL() << " \""
         << GetRemotePartyName() << "\""
            " promoted to moderator.\n";

    m_endpoint.m_manager.Broadcast(strm);
  }

  return OpalMixerConnection::SendUserInputString(value);
}


// End of File ///////////////////////////////////////////////////////////////
