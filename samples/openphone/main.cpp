/*
 * main.cpp
 *
 * An OPAL GUI phone application.
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
 *
 * $Log: main.cpp,v $
 * Revision 1.8  2004/05/01 13:38:05  rjongbloed
 * Some early implementation of wxWIndows based OPAL GUI client.
 *
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


class TextCtrlChannel : public PChannel
{
    PCLASSINFO(TextCtrlChannel, PChannel)
  public:
    TextCtrlChannel(wxTextCtrl * textCtrl = NULL)
      : m_textCtrl(textCtrl)
      { }

    virtual BOOL Write(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
    ) {
      if (m_textCtrl == NULL)
        return FALSE;
      m_textCtrl->WriteText(wxString((const wxChar *)buf, (size_t)len));
      return TRUE;
    }

    void SetTextCtrl(
      wxTextCtrl * textCtrl
    ) { m_textCtrl = textCtrl; }

  protected:
    wxTextCtrl * m_textCtrl;
} LogWindow;



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
  MyFrame * frame = new MyFrame();
  SetTopWindow(frame);
  return frame->Initialise();
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
  EVT_SIZE(MyFrame::OnSize)

  EVT_MENU(MENU_FILE_QUIT,  MyFrame::OnMenuQuit)
  EVT_MENU(MENU_FILE_ABOUT, MyFrame::OnMenuAbout)
  EVT_MENU(MENU_FILE_CALL,  MyFrame::OnMenuCall)
END_EVENT_TABLE()

MyFrame::MyFrame()
  : wxFrame(NULL, -1, wxT("OpenPhone"), wxDefaultPosition, wxSize(640, 480)),
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
  menu->Append(MENU_FILE_CALL, _T("&Call"));
  menu->AppendSeparator();
  menu->Append(MENU_FILE_QUIT, _T("E&xit\tAlt-X"));
  menubar->Append(menu, _T("&File"));

  menu = new wxMenu;
  menu->Append(MENU_FILE_ABOUT, _T("&About"));
  menubar->Append(menu, _T("&Help"));

  SetMenuBar(menubar);

  // Make the content of the main window, speed dial and log panes
  m_panel = new wxPanel(this, -1);
  m_logWindow = new wxTextCtrl(m_panel, -1, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxSUNKEN_BORDER);
  LogWindow.SetTextCtrl(m_logWindow);

  // Show the frame
  Show(TRUE);
}


MyFrame::~MyFrame()
{
  ClearAllCalls();

  // Must do this before we destroy the manager or a crash will result
  if (potsEP != NULL)
    potsEP->RemoveAllLines();

  LogWindow.SetTextCtrl(NULL);
}


bool MyFrame::Initialise()
{
#if OPAL_H323
  h323EP = new H323EndPoint(*this);

  if (h323EP->StartListeners(PStringArray()))
    LogWindow << "H.323 listening on " << setfill(',') << h323EP->GetListeners() << setfill(' ') << endl;
#endif

#if OPAL_SIP
  sipEP = new SIPEndPoint(*this);

  if (sipEP->StartListeners(PStringArray()))
    LogWindow << "SIP listening on " << setfill(',') << sipEP->GetListeners() << setfill(' ') << endl;
#endif

#if P_EXPAT
  ivrEP = new OpalIVREndPoint(*this);
#endif

  pcssEP = new MyPCSSEndPoint(*this);

  return true;
}


void MyFrame::OnSize(wxSizeEvent& event)
{
  if (m_logWindow == NULL)
    return;

  wxSize size = GetClientSize();

  m_logWindow->SetSize(0, 0, size.x, size.y);

  event.Skip();
}


void MyFrame::OnMenuQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(TRUE);
}


void MyFrame::OnMenuAbout(wxCommandEvent& WXUNUSED(event))
{
  wxMessageDialog dialog(this,
                         _T("OpenPhone\nPost Increment (c) 2004"),
                         _T("About OpenPhone"),
                         wxOK);
  dialog.ShowModal();
}


void MyFrame::OnMenuCall(wxCommandEvent& WXUNUSED(event))
{
  CallDialog dlg(this);

  if (dlg.ShowModal() == wxID_OK) {
    LogWindow << "Calling \"" << dlg.m_Address << '"' << endl;
    if (potsEP != NULL)
      SetUpCall("pots:*", dlg.m_Address.c_str(), currentCallToken);
    else
      SetUpCall("pc:*", dlg.m_Address.c_str(), currentCallToken);
  }
}


void MyFrame::OnEstablishedCall(OpalCall & call)
{
  currentCallToken = call.GetToken();
  LogWindow << "In call with " << call.GetPartyB() << " using " << call.GetPartyA() << endl;
}


void MyFrame::OnClearedCall(OpalCall & call)
{
  if (currentCallToken == call.GetToken())
    currentCallToken = PString();

  PString remoteName = '"' + call.GetPartyB() + '"';
  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByRemoteUser :
      LogWindow << remoteName << " has cleared the call";
      break;
    case OpalConnection::EndedByCallerAbort :
      LogWindow << remoteName << " has stopped calling";
      break;
    case OpalConnection::EndedByRefusal :
      LogWindow << remoteName << " did not accept your call";
      break;
    case OpalConnection::EndedByNoAnswer :
      LogWindow << remoteName << " did not answer your call";
      break;
    case OpalConnection::EndedByTransportFail :
      LogWindow << "Call with " << remoteName << " ended abnormally";
      break;
    case OpalConnection::EndedByCapabilityExchange :
      LogWindow << "Could not find common codec with " << remoteName;
      break;
    case OpalConnection::EndedByNoAccept :
      LogWindow << "Did not accept incoming call from " << remoteName;
      break;
    case OpalConnection::EndedByAnswerDenied :
      LogWindow << "Refused incoming call from " << remoteName;
      break;
    case OpalConnection::EndedByNoUser :
      LogWindow << "Gatekeeper could find user " << remoteName;
      break;
    case OpalConnection::EndedByNoBandwidth :
      LogWindow << "Call to " << remoteName << " aborted, insufficient bandwidth.";
      break;
    case OpalConnection::EndedByUnreachable :
      LogWindow << remoteName << " could not be reached.";
      break;
    case OpalConnection::EndedByNoEndPoint :
      LogWindow << "No phone running for " << remoteName;
      break;
    case OpalConnection::EndedByHostOffline :
      LogWindow << remoteName << " is not online.";
      break;
    case OpalConnection::EndedByConnectFail :
      LogWindow << "Transport error calling " << remoteName;
      break;
    default :
      LogWindow << "Call with " << remoteName << " completed";
  }
  PTime now;
  LogWindow << ", on " << now.AsString("w h:mma") << ". Duration "
            << setprecision(0) << setw(5) << (now - call.GetStartTime())
            << "s." << endl;
}


BOOL MyFrame::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return FALSE;

  LogWindow << connection.GetEndPoint().GetPrefixName() << " started ";

  if (stream.IsSource())
    LogWindow << "sending ";
  else
    LogWindow << "receiving ";

  LogWindow << stream.GetMediaFormat() << endl;  

  return TRUE;
}


void MyFrame::OnUserInputString(OpalConnection & connection, const PString & value)
{
  LogWindow << "User input received: \"" << value << '"' << endl;
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
    LogWindow << "\nCall on " << now.AsString("w h:mma")
              << " from " << connection.GetRemotePartyName()
              << ", answer (Y/N)? " << endl;
  }
}


BOOL MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  PTime now;
  LogWindow << connection.GetRemotePartyName() << " is ringing on "
            << now.AsString("w h:mma") << " ..." << endl;
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

enum {
  CALL_DIALOG_ADDRESS = 10
};

BEGIN_EVENT_TABLE(CallDialog, wxDialog)
  EVT_TEXT(CALL_DIALOG_ADDRESS, CallDialog::OnAddressChange)
END_EVENT_TABLE()

CallDialog::CallDialog(wxWindow *parent)
  : wxDialog(parent, -1, wxString(_T("Call")),
             wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
  wxStaticText * label = new wxStaticText(this, -1, "Address:",
                                          wxDefaultPosition, wxDefaultSize,
                                          wxALIGN_RIGHT);

  m_AddressCtrl = new wxTextCtrl(this, CALL_DIALOG_ADDRESS, wxEmptyString,
                                 wxDefaultPosition, wxSize(200, 24),
                                 wxSUNKEN_BORDER,
                                 wxTextValidator(wxFILTER_NONE, &m_Address));

  m_ok = new wxButton(this, wxID_OK, _T("OK"));
  wxButton * cancel = new wxButton(this, wxID_CANCEL, _T("Cancel"));

  wxBoxSizer * sizerTop = new wxBoxSizer(wxHORIZONTAL);
  sizerTop->Add(label,         0, wxALIGN_CENTER | wxTOP | wxBOTTOM | wxLEFT, 12);
  sizerTop->Add(m_AddressCtrl, 1, wxALIGN_CENTER | wxTOP | wxBOTTOM | wxRIGHT, 12);

  wxBoxSizer * sizerBottom = new wxBoxSizer(wxHORIZONTAL);
  sizerBottom->Add(m_ok,   0, wxALIGN_CENTER | wxALL, 12);
  sizerBottom->Add(cancel, 0, wxALIGN_CENTER | wxALL, 12);

  wxBoxSizer *sizerAll = new wxBoxSizer(wxVERTICAL);
  sizerAll->Add(sizerTop,    0, wxALIGN_CENTER | wxEXPAND);
  sizerAll->Add(sizerBottom, 0, wxALIGN_CENTER);

  SetAutoLayout(TRUE);
  SetSizer(sizerAll);

  sizerAll->SetSizeHints(this);
  sizerAll->Fit(this);

  m_ok->Disable();
}


void CallDialog::OnAddressChange(wxCommandEvent & WXUNUSED(event))
{
  m_ok->Enable(!m_AddressCtrl->GetValue().IsEmpty());
}


// End of File ///////////////////////////////////////////////////////////////
