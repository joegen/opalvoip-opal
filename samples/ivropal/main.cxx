/*
 * main.cxx
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
 * $Revision$
 * $Author$
 * $Date$
 */

#include "precompile.h"
#include "main.h"

// Debug: -ttttodebugstream file:../callgen/ogm.wav sip:10.0.1.12
//        -ttttodebugstream "repeat=10;file:../callgen/ogm.wav"
//        -ttttodebugstream file:test.vxml


PCREATE_PROCESS(IvrOPAL);


IvrOPAL::IvrOPAL()
  : PProcess("Vox Gratia", "IvrOPAL", 1, 0, ReleaseCode, 0)
  , m_manager(NULL)
{
}


void IvrOPAL::Main()
{
  m_manager = new MyManager();

  PArgList & args = GetArguments();

  if (!args.Parse(m_manager->GetArgumentSpec()) || args.HasOption('h') || args.GetCount() == 0) {
    PString name = GetFile().GetTitle();
    cerr << "usage: " << name << " [ options ] vxml [ url ]\n"
            "\n"
            "Available options are:\n"
         << m_manager->GetArgumentUsage()
         << "\n"
            "where vxml is a VXML script, a URL to a VXML script or a WAV file, or a\n"
            "series of commands separated by ';'.\n"
            "\n"
            "Commands are:\n"
            "  repeat=n    Repeat next WAV file n times.\n"
            "  delay=n     Delay after repeats n milliseconds.\n"
            "  voice=name  Set Text To Speech voice to name\n"
            "  tone=t      Emit DTMF tone t\n"
            "  silence=n   Emit silence for n milliseconds\n"
            "  speak=text  Speak the text using the Text To Speech system.\n"
            "  speak=$var  Speak the internal variable using the Text To Speech system.\n"
            "\n"
            "Variables may be one of:\n"
            "  Time\n"
            "  Originator-Address\n"
            "  Remote-Address\n"
            "  Source-IP-Address\n"
            "\n"
            "If a second parameter is provided and outgoing call is made and when answered\n"
            "the script is executed. If no second paramter is provided then the program\n"
            "will continuosly listen for incoming calls and execute the script on each\n"
            "call. Simultaneous calls to the limits of the operating system arre possible.\n"
            "\n"
            "e.g. " << name << " file://message.wav sip:fred@bloggs.com\n"
            "     " << name << " http://voicemail.vxml\n"
            "     " << name << " \"repeat=5;delay=2000;speak=Hello, this is IVR!\"\n"
            "\n";
    return;
  }

  if (!m_manager->Initialise(args, true, "ivr:"))
    return;


  // Set up IVR
  MyIVREndPoint * ivr  = new MyIVREndPoint(*m_manager);
  ivr->SetDefaultVXML(args[0]);


  if (args.GetCount() == 1)
    cout << "Awaiting incoming call, using VXML \"" << args[0] << "\" ... " << flush;
  else {
    PString token;
    if (!m_manager->SetUpCall("ivr:", args[1], token)) {
      cerr << "Could not start call to \"" << args[1] << '"' << endl;
      return;
    }
    cout << "Playing " << args[0] << " to " << args[1] << " ... " << flush;
  }

  // Wait for call to come in and finish
  m_manager->m_completed.Wait();
  cout << "Completed." << endl;

  MyManager * mgr = m_manager;
  m_manager = NULL;
  delete mgr;
}


void MyManager::OnClearedCall(OpalCall & call)
{
  if (call.GetPartyA().NumCompare("ivr") == EqualTo)
    m_completed.Signal();
}


void MyIVREndPoint::OnEndDialog(OpalIVRConnection & connection)
{
  cout << "\nFinal variables:\n" << connection.GetVXMLSession().GetVariables() << endl;

  // Do default action which is to clear the call.
  OpalIVREndPoint::OnEndDialog(connection);
}


bool IvrOPAL::OnInterrupt(bool)
{
  if (m_manager == NULL)
    return false;

  m_manager->m_completed.Signal();
  return true;
}


// End of File ///////////////////////////////////////////////////////////////
