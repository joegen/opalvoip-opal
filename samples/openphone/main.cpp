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
 * Revision 1.11  2004/05/12 12:41:38  rjongbloed
 * More work on wxWindows based OpenPhone
 *
 * Revision 1.10  2004/05/09 13:24:25  rjongbloed
 * More work on wxWindows based OpenPhone
 *
 * Revision 1.9  2004/05/06 13:23:43  rjongbloed
 * Work on wxWindows based OpenPhone
 *
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

#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/image.h>
#include <wx/valgen.h>
#include <wx/xrc/xmlres.h>

#if OPAL_SIP
#include <sip/sip.h>
#endif

#if OPAL_H323
#include <h323/h323.h>
#include <h323/gkclient.h>
#endif

#include <opal/ivr.h>
#include <lids/lidep.h>

#ifdef OPAL_STATIC_LINK
#define H323_STATIC_LIB
#include <codec/allcodecs.h>
#include <lids/alllids.h>
#endif


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
  wxImage::AddHandler(new wxGIFHandler);
  wxXmlResource::Get()->InitAllHandlers();
  wxXmlResource::Get()->Load("openphone.xrc");

  // Create the main frame window
  MyFrame * frame = new MyFrame();
  SetTopWindow(frame);
  return frame->Initialise();
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
  EVT_MENU(XRCID("MenuQuit"),   MyFrame::OnMenuQuit)
  EVT_MENU(XRCID("MenuAbout"),  MyFrame::OnMenuAbout)
  EVT_MENU(XRCID("MenuCall"),   MyFrame::OnMenuCall)
  EVT_MENU(XRCID("ViewLarge"),  MyFrame::OnViewLarge)
  EVT_MENU(XRCID("ViewSmall"),  MyFrame::OnViewSmall)
  EVT_MENU(XRCID("ViewList"),   MyFrame::OnViewList)
  EVT_MENU(XRCID("ViewDetails"),MyFrame::OnViewDetails)
  EVT_MENU(XRCID("MenuOptions"),MyFrame::OnOptions)
END_EVENT_TABLE()

MyFrame::MyFrame()
  : wxFrame(NULL, -1, wxT("OpenPhone"), wxDefaultPosition, wxSize(640, 480)),
    m_speedDials(NULL),
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
  SetMenuBar(wxXmlResource::Get()->LoadMenuBar("MenuBar"));

  // Make the content of the main window, speed dial and log panes
  m_splitter = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxSP_3D);

  m_logWindow = new wxTextCtrl(m_splitter, -1, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxSUNKEN_BORDER);
  LogWindow.SetTextCtrl(m_logWindow);

  RecreateSpeedDials(wxLC_LIST);

  // Set up sizer to automatically resize the splitter to size of window
  wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(m_splitter, 1, wxGROW, 0);
  SetSizer(sizer);

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
  PTrace::Initialise(4, "trace.log", PTrace::DateAndTime|PTrace::Thread|PTrace::FileAndLine);
  PTrace::SetLevel(4);

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


void MyFrame::RecreateSpeedDials(long flags)
{
  wxListCtrl * oldSpeedDials = m_speedDials;

  m_speedDials = new wxListCtrl(m_splitter, -1,
                               wxDefaultPosition, wxDefaultSize,
                               flags | wxLC_EDIT_LABELS | wxSUNKEN_BORDER);

  m_speedDials->InsertColumn(0, _T("Name"));
  m_speedDials->InsertColumn(1, _T("Number"));
  m_speedDials->InsertColumn(2, _T("Address"));

  // Test data ....
  m_speedDials->InsertItem(0, _T("SpeedDialOne"));
  m_speedDials->SetItem(0, 1, _T("1"));
  m_speedDials->SetItem(0, 2, _T("h323:h323.voxgratia.org"));

  m_speedDials->InsertItem(1, _T("SpeedDialTwo"));
  m_speedDials->SetItem(1, 1, _T("234"));
  m_speedDials->SetItem(1, 2, _T("sip:gw.voxgratia.org"));

  // Now either replace the top half of the splitter or set it for the first time
  if (oldSpeedDials == NULL)
    m_splitter->SplitHorizontally(m_speedDials, m_logWindow);
  else {
    m_splitter->ReplaceWindow(oldSpeedDials, m_speedDials);
    delete oldSpeedDials;
  }
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
      SetUpCall("pots:*", dlg.m_Address, currentCallToken);
    else
      SetUpCall("pc:*", dlg.m_Address, currentCallToken);
  }
}


void MyFrame::OnViewLarge(wxCommandEvent& WXUNUSED(event))
{
  RecreateSpeedDials(wxLC_ICON);
}


void MyFrame::OnViewSmall(wxCommandEvent& WXUNUSED(event))
{
  RecreateSpeedDials(wxLC_SMALL_ICON);
}


void MyFrame::OnViewList(wxCommandEvent& WXUNUSED(event))
{
  RecreateSpeedDials(wxLC_LIST);
}


void MyFrame::OnViewDetails(wxCommandEvent& WXUNUSED(event))
{
  RecreateSpeedDials(wxLC_REPORT);
}


void MyFrame::OnOptions(wxCommandEvent& event)
{
  OptionsDialog dlg(this);

  dlg.m_SoundPlayer = pcssEP->GetSoundChannelPlayDevice();
  dlg.m_SoundRecorder = pcssEP->GetSoundChannelRecordDevice();

  if (dlg.ShowModal() == wxID_OK) {
    pcssEP->SetSoundChannelPlayDevice(dlg.m_SoundPlayer);
    pcssEP->SetSoundChannelRecordDevice(dlg.m_SoundRecorder);
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

BEGIN_EVENT_TABLE(CallDialog, wxDialog)
  EVT_TEXT(XRCID("Address"), CallDialog::OnAddressChange)
END_EVENT_TABLE()

CallDialog::CallDialog(wxWindow *parent)
{
  wxXmlResource::Get()->LoadDialog(this, parent, "CallDialog");

  m_ok = (wxButton *)FindWindowByName("wxID_OK", this);
  m_ok->Disable();

  m_AddressCtrl = (wxComboBox *)FindWindowByName("Address");
  m_AddressCtrl->SetValidator(wxGenericValidator(&m_Address));

  // Temprary, must get from config
  m_AddressCtrl->Append("h323:h323.voxgratia.org");
  m_AddressCtrl->Append("sip:gw.voxgratia.org");
}


void CallDialog::OnAddressChange(wxCommandEvent & WXUNUSED(event))
{
  m_ok->Enable(!m_AddressCtrl->GetValue().IsEmpty());
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
END_EVENT_TABLE()

OptionsDialog::OptionsDialog(wxWindow *parent)
{
  PINDEX i;

  wxXmlResource::Get()->LoadDialog(this, parent, "OptionsDialog");

  wxComboBox * combo = (wxComboBox *)FindWindowByName("SoundPlayer");
  combo->SetValidator(wxGenericValidator(&m_SoundPlayer));
  PStringList devices = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
  for (i = 0; i < devices.GetSize(); i++)
    combo->Append((const char *)devices[i]);

  combo = (wxComboBox *)FindWindowByName("SoundRecorder");
  combo->SetValidator(wxGenericValidator(&m_SoundRecorder));
  devices = PSoundChannel::GetDeviceNames(PSoundChannel::Recorder);
  for (i = 0; i < devices.GetSize(); i++)
    combo->Append((const char *)devices[i]);
}


// End of File ///////////////////////////////////////////////////////////////
