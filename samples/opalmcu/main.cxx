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
  m_manager = new MyManager();

  PArgList & args = GetArguments();

  args.Parse(m_manager->GetArgumentSpec() +
             "a-attendant:"
             "c-cli:"
             "m-moderator:"
             "n-name:"
#if OPAL_VIDEO
             "A-audio-only."
             "s-size:"
#endif
             "-pass-thru.", false);
  if (args.HasOption('h')) {
    cerr << "usage: " << GetFile().GetTitle() << " [ options ] [ name ]\n"
            "\n"
            "Available options are:\n"
            "  -a or --attendant vxml  : VXML script to run for incoming calls not directed.\n"
            "                          : to a specific conference. If absent, then ad-hoc\n"
            "                          : conferences are created.\n"
            "  -m or --moderator pin   : PIN to allow to become a moderator and have talk\n"
            "                          : rights if absent, all participants are moderators.\n"
            "        --pass-thru       : Enable media pass through optimisation.\n"
            "  -n or --name alias      : Default name for ad-hoc conference.\n"
#if OPAL_VIDEO
            "  -A or --audio-only      : Audio only for ad-hoc conference.\n"
            "  -s or --size            : Set default video size for ad-hoc conference.\n"
#endif
            "\n"
         << m_manager->GetArgumentUsage();
    return;
  }

  if (!m_manager->Initialise(args, true, "mcu:<du>"))
    return;

  // Set up PCSS to do speaker playback
  new MyPCSSEndPoint(*m_manager);

  // Set up IVR to do recording or WAV file play
  OpalIVREndPoint * ivr = new OpalIVREndPoint(*m_manager);

  // Set up conference mixer
  MyMixerNodeInfo * info = NULL;
  if (args.HasOption('a')) {
    PFilePath path = args.GetOptionString('a');
    ivr->SetDefaultVXML(path);
    m_manager->AddRouteEntry(".*:.* = ivr:<da>");
    cout << "Using attendant IVR " << path << endl;
  }
  else {
    info = new MyMixerNodeInfo;
    info->m_name = args.GetOptionString('n', "room101");
    info->m_moderatorPIN = args.GetOptionString('m');
    info->m_listenOnly = !info->m_moderatorPIN.IsEmpty();
    info->m_mediaPassThru = args.HasOption("pass-thru");

#if OPAL_VIDEO
    info->m_audioOnly = args.HasOption('V');
    if (args.HasOption('s'))
      PVideoFrameInfo::ParseSize(args.GetOptionString('s'), info->m_width, info->m_height);
#endif
    cout << "Using ad-hoc conference, default name: " << info->m_name << endl;
  }

  MyMixerEndPoint * mixer = m_manager->m_mixer = new MyMixerEndPoint(*m_manager, info);


  // Wait for call to come in and finish
  PCLI * cli;
  if (args.HasOption('c')) {
    unsigned port = args.GetOptionString('c').AsUnsigned();
    if (port == 0 || port > 65535) {
      cerr << "Illegal CLI port " << port << endl;
      return;
    }
    cli = new PCLISocket((WORD)port);
    cli->StartContext(new PConsoleChannel(PConsoleChannel::StandardInput),
                      new PConsoleChannel(PConsoleChannel::StandardOutput));
  }
  else
    cli = new PCLIStandard();

  cli->SetPrompt("MCU> ");

  cli->SetCommand("conf add", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfAdd),
                  "Add a new conferance:",
#if OPAL_VIDEO
                  "[ -V ] [ -s size ] [ -m pin ] <name> [ <name> ... ]\n"
                  "  -V or --no-video       : Disable video\n"
                  "  -s or --size           : Set video size"
#else
                  "[ -m pin ] <name>"
#endif
                  "\n"
                  "  -m or --moderator pin  : PIN to allow to become a moderator and have talk rights\n"
                  "                         : if absent, all participants are moderators.\n"
                  "        --no-pass-thru   : Disable media pass through optimisation.\n"
                 );
  cli->SetCommand("conf list", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfList),
                  "List conferances");
  cli->SetCommand("conf remove", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfRemove),
                  "Remove conferance:",
                  "<name> | <guid>");
  cli->SetCommand("conf listen", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfListen),
                  "Toggle listening to conferance:",
                  "{ <conf-name> | <guid> }");
  cli->SetCommand("conf record", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfRecord),
                  "Record conferance:",
                  "{ <conf-name> | <guid> } { <filename> | stop }");
  cli->SetCommand("conf play", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdConfPlay),
                  "Play file to conferance:",
                  "{ <conf-name> | <guid> } <filename>");

  cli->SetCommand("member add", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdMemberAdd),
                  "Add a member to conferance:",
                  "{ <name> | <guid> } <address>");
  cli->SetCommand("member list", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdMemberList),
                  "List members in conferance:",
                  "<name> | <guid>");
  cli->SetCommand("member remove", PCREATE_NOTIFIER_EXT(mixer, MyMixerEndPoint, CmdMemberRemove),
                  "Remove conferance member:",
                  "{ <conf-name> | <guid> } <member-name>\n"
                  "member remove <call-token>");

  cli->SetCommand("list codecs", PCREATE_NOTIFIER(CmdListCodecs),
                  "List available codecs");
#if PTRACING
  cli->SetCommand("trace", PCREATE_NOTIFIER(CmdTrace),
                  "Set trace level (1..6) and filename",
                  "<n> [ <filename> ]");
#endif
  cli->SetCommand("shutdown", PCREATE_NOTIFIER(CmdShutDown),
                  "Shut down the application");
  cli->SetCommand("quit\nq\nexit", PCREATE_NOTIFIER(CmdQuit),
                  "Quit command line interpreter, note quitting from console also shuts down application.");

  m_manager->m_cli = cli;
  cli->Start(false); // Do not spawn thread, wait till end of input

  cout << "\nExiting ..." << endl;
}


void ConfOPAL::CmdListCodecs(PCLI::Arguments & args, INT)
{
  OpalMediaFormatList formats;
  OpalMediaFormat::GetAllRegisteredMediaFormats(formats);

  PCLI::Context & out = args.GetContext();
  OpalMediaFormatList::iterator format;
  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetMediaType() == OpalMediaType::Audio() && format->IsTransportable())
      out << *format << '\n';
  }
#if OPAL_VIDEO
  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetMediaType() == OpalMediaType::Video() && format->IsTransportable())
      out << *format << '\n';
  }
#endif
  out.flush();
}


#if PTRACING
void ConfOPAL::CmdTrace(PCLI::Arguments & args, INT)
{
  if (args.GetCount() == 0)
    args.WriteUsage();
  else
    PTrace::Initialise(args[0].AsUnsigned(),
                       args.GetCount() > 1 ? (const char *)args[1] : NULL,
                       PTrace::GetOptions());
}
#endif // PTRACING


void ConfOPAL::CmdShutDown(PCLI::Arguments & args, INT)
{
  m_manager->m_cli->Stop();
}


void ConfOPAL::CmdQuit(PCLI::Arguments & args, INT)
{
  if (PIsDescendant(args.GetContext().GetBaseReadChannel(), PConsoleChannel))
    m_manager->m_cli->Stop();
  else
    args.GetContext().Stop();
}


///////////////////////////////////////////////////////////////

MyManager::MyManager()
  : m_mixer(NULL)
  , m_cli(NULL)
{
}


MyManager::~MyManager()
{
  delete m_cli;
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

MyMixerEndPoint::MyMixerEndPoint(MyManager & manager, MyMixerNodeInfo * info)
  : OpalMixerEndPoint(manager, "mcu")
  , m_manager(manager)
{
  m_adHocNodeInfo = info;
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
  m_manager.m_cli->Broadcast(strm);

  return OpalMixerEndPoint::CreateNode(info);
}


void MyMixerEndPoint::CmdConfAdd(PCLI::Arguments & args, INT)
{
  args.Parse("s-size:V-no-video.-m-moderator:");
  if (args.GetCount() == 0) {
    args.WriteUsage();
    return;
  }

  for (PINDEX i = 0; i < args.GetCount(); ++i) {
    if (m_nodeManager.FindNode(args[i]) != NULL) {
      args.WriteError() << "Conference name \"" << args[i] << "\" already exists." << endl;
      return;
    }
  }

  MyMixerNodeInfo * info = new MyMixerNodeInfo();
  info->m_name = args[0];
  info->m_audioOnly = args.HasOption('V');
  if (args.HasOption('s'))
    PVideoFrameInfo::ParseSize(args.GetOptionString('s'), info->m_width, info->m_height);
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


void MyMixerEndPoint::CmdConfList(PCLI::Arguments & args, INT)
{
  ostream & out = args.GetContext();
  for (PSafePtr<OpalMixerNode> node = m_nodeManager.GetFirstNode(PSafeReadOnly); node != NULL; ++node)
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


void MyMixerEndPoint::CmdConfRemove(PCLI::Arguments & args, INT)
{
  PSafePtr<OpalMixerNode> node;
  if (!CmdConfXXX(args, node, 1))
    return;

  RemoveNode(*node);
  args.GetContext() << "Removed conference " << *node << endl;
}


void MyMixerEndPoint::CmdConfListen(PCLI::Arguments & args, INT)
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


void MyMixerEndPoint::CmdConfRecord(PCLI::Arguments & args, INT)
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


void MyMixerEndPoint::CmdConfPlay(PCLI::Arguments & args, INT)
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


void MyMixerEndPoint::CmdMemberAdd(PCLI::Arguments & args, INT)
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


void MyMixerEndPoint::CmdMemberList(PCLI::Arguments & args, INT)
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


void MyMixerEndPoint::CmdMemberRemove(PCLI::Arguments & args, INT)
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

    m_endpoint.m_manager.m_cli->Broadcast(strm);
  }

  return OpalMixerConnection::SendUserInputString(value);
}


// End of File ///////////////////////////////////////////////////////////////
