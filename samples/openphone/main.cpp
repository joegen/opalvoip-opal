/*
 * main.cpp
 *
 * Open Phone Client application source file for OPAL
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *    
 * Copyright (c) 2004 Post Increment
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
 * The Original Code is Open Phone client.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): 
 */

#ifdef __GNUG__
#pragma implementation
#pragma interface
#endif

#include "main.h"
#include "version.h"

#if OPAL_SIP
#include <sip/sip.h>
#endif

#if OPAL_H323
#include <h323/h323.h>
#include <h323/gkclient.h>
#endif

#include <opal/ivr.h>
#include <lids/lidep.h>


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_APP(OpenPhoneApp)

OpenPhoneApp::OpenPhoneApp()
  : PProcess("Equivalence", "OpenPhone",
              MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}


void OpenPhoneApp::Main()
{
  // Dummy function
}


bool OpenPhoneApp::OnInit()
{
  // Create the main frame window
  SetTopWindow(new MyFrame());

  return true;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
  EVT_SIZE(MyFrame::OnSize)

  EVT_MENU(MENU_FILE_QUIT, MyFrame::OnQuit)
  EVT_MENU(MENU_FILE_ABOUT, MyFrame::OnAbout)
END_EVENT_TABLE()

MyFrame::MyFrame()
  : wxFrame((wxFrame *)NULL, -1, wxT("wxListCtrl Test"), wxPoint(50, 50), wxSize(450, 340)),
    m_panel(NULL),
    m_logWindow(NULL),
    pcssEP(NULL),
    potsEP(NULL),
#if OPAL_H323
    h323EP(NULL),
#endif
#if OPAL_SIP
    sipEP(NULL),
#endif
#if P_EXPAT
    ivrEP(NULL)
#endif
{
  // Give it an icon
  SetIcon( wxICON(OpenOphone) );

  // Make a menubar
  wxMenuBar *menubar = new wxMenuBar;

  wxMenu *menu = new wxMenu;
  menu->Append(MENU_FILE_ABOUT, _T("&About"));
  menu->AppendSeparator();
  menu->Append(MENU_FILE_QUIT, _T("E&xit\tAlt-X"));
  menubar->Append(menu, _T("&File"));

  SetMenuBar(menubar);

  m_panel = new wxPanel(this, -1);
  m_logWindow = new wxTextCtrl(m_panel, -1, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxSUNKEN_BORDER);

#if OPAL_H323
  h323EP = new H323EndPoint(*this);
#endif

#if OPAL_SIP
  sipEP = new SIPEndPoint(*this);
#endif

#if P_EXPAT
  ivrEP = new OpalIVREndPoint(*this);
#endif

  pcssEP = new MyPCSSEndPoint(*this);

  // Show the frame
  Show(TRUE);
}


MyFrame::~MyFrame()
{
  // Must do this before we destroy the manager or a crash will result
  if (potsEP != NULL)
    potsEP->RemoveAllLines();
}


void MyFrame::OnSize(wxSizeEvent& event)
{
  if (m_logWindow == NULL)
    return;

  wxSize size = GetClientSize();
  m_logWindow->SetSize(0, 0, size.x, size.y);

  event.Skip();
}


void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(TRUE);
}


void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageDialog dialog(this,
                           _T("OpenPhone\nPost Increment (c) 2004"),
                           _T("About list test"),
                           wxOK|wxCANCEL);
    dialog.ShowModal();
}


void MyFrame::OnEstablishedCall(OpalCall & call)
{
  currentCallToken = call.GetToken();
//  cout << "In call with " << call.GetPartyB() << " using " << call.GetPartyA() << endl;
}


void MyFrame::OnClearedCall(OpalCall & call)
{
  if (currentCallToken == call.GetToken())
    currentCallToken = PString();

#if 0
  PString remoteName = '"' + call.GetPartyB() + '"';
  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByRemoteUser :
      cout << remoteName << " has cleared the call";
      break;
    case OpalConnection::EndedByCallerAbort :
      cout << remoteName << " has stopped calling";
      break;
    case OpalConnection::EndedByRefusal :
      cout << remoteName << " did not accept your call";
      break;
    case OpalConnection::EndedByNoAnswer :
      cout << remoteName << " did not answer your call";
      break;
    case OpalConnection::EndedByTransportFail :
      cout << "Call with " << remoteName << " ended abnormally";
      break;
    case OpalConnection::EndedByCapabilityExchange :
      cout << "Could not find common codec with " << remoteName;
      break;
    case OpalConnection::EndedByNoAccept :
      cout << "Did not accept incoming call from " << remoteName;
      break;
    case OpalConnection::EndedByAnswerDenied :
      cout << "Refused incoming call from " << remoteName;
      break;
    case OpalConnection::EndedByNoUser :
      cout << "Gatekeeper could find user " << remoteName;
      break;
    case OpalConnection::EndedByNoBandwidth :
      cout << "Call to " << remoteName << " aborted, insufficient bandwidth.";
      break;
    case OpalConnection::EndedByUnreachable :
      cout << remoteName << " could not be reached.";
      break;
    case OpalConnection::EndedByNoEndPoint :
      cout << "No phone running for " << remoteName;
      break;
    case OpalConnection::EndedByHostOffline :
      cout << remoteName << " is not online.";
      break;
    case OpalConnection::EndedByConnectFail :
      cout << "Transport error calling " << remoteName;
      break;
    default :
      cout << "Call with " << remoteName << " completed";
  }
  PTime now;
  cout << ", on " << now.AsString("w h:mma") << ". Duration "
       << setprecision(0) << setw(5) << (now - call.GetStartTime())
       << "s." << endl;
#endif
}


BOOL MyFrame::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return FALSE;

#if 0
  cout << connection.GetEndPoint().GetPrefixName() << " started ";

  if (stream.IsSource())
    cout << "sending ";
  else
    cout << "receiving ";

  cout << stream.GetMediaFormat() << endl;  
#endif

  return TRUE;
}


void MyFrame::OnUserInputString(OpalConnection & connection, const PString & value)
{
//  cout << "User input received: \"" << value << '"' << endl;
  OpalManager::OnUserInputString(connection, value);
}


///////////////////////////////////////////////////////////////////////////////

MyPCSSEndPoint::MyPCSSEndPoint(MyFrame & ep)
  : OpalPCSSEndPoint(ep),
    frame(ep)
{
  autoAnswer = FALSE;
}


PString MyPCSSEndPoint::OnGetDestination(const OpalPCSSConnection & /*connection*/)
{
  // Dialog to prompt for address
  return PString();
}


void MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  incomingConnectionToken = connection.GetToken();

  if (autoAnswer)
    AcceptIncomingConnection(incomingConnectionToken);
  else {
    PTime now;
//    cout << "\nCall on " << now.AsString("w h:mma")
//         << " from " << connection.GetRemotePartyName()
//         << ", answer (Y/N)? " << flush;
  }
}


BOOL MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  PTime now;
//  cout << connection.GetRemotePartyName() << " is ringing on "
//       << now.AsString("w h:mma") << " ..." << endl;
  return TRUE;
}


// End of File ///////////////////////////////////////////////////////////////
