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
 * Revision 1.17  2004/07/11 12:42:11  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 1.16  2004/07/04 12:53:09  rjongbloed
 * Added support for route editing.
 *
 * Revision 1.15  2004/06/05 14:37:03  rjongbloed
 * More implemntation of options dialog.
 *
 * Revision 1.14  2004/05/25 12:55:52  rjongbloed
 * Added all silence suppression modes to Options dialog.
 *
 * Revision 1.13  2004/05/24 13:44:03  rjongbloed
 * More implementation on OPAL OpenPhone.
 *
 * Revision 1.12  2004/05/15 12:18:23  rjongbloed
 * More work on wxWindows based OpenPhone
 *
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

#include <wx/config.h>
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
#include <ptclib/pstun.h>

#ifdef OPAL_STATIC_LINK
#define H323_STATIC_LIB
#include <codec/allcodecs.h>
#include <lids/alllids.h>
#endif


#define DEF_FIELD(name) static const char name##Key[] = #name;
DEF_FIELD(ActiveView);
DEF_FIELD(MainFrameX);
DEF_FIELD(MainFrameY);
DEF_FIELD(MainFrameWidth);
DEF_FIELD(MainFrameHeight);

DEF_FIELD(Username);
DEF_FIELD(DisplayName);
DEF_FIELD(RingSoundFileName);
DEF_FIELD(AutoAnswer);
DEF_FIELD(IVRScript);
DEF_FIELD(Bandwidth);
DEF_FIELD(TCPPortBase);
DEF_FIELD(TCPPortMax);
DEF_FIELD(UDPPortBase);
DEF_FIELD(UDPPortMax);
DEF_FIELD(RTPPortBase);
DEF_FIELD(RTPPortMax);
DEF_FIELD(RTPTOS);
DEF_FIELD(STUNServer);
DEF_FIELD(NATRouter);
DEF_FIELD(SoundPlayer);
DEF_FIELD(SoundRecorder);
DEF_FIELD(SoundBuffers);
DEF_FIELD(LineInterfaceDevice);
DEF_FIELD(AEC);
DEF_FIELD(Country);
DEF_FIELD(MinJitter);
DEF_FIELD(MaxJitter);
DEF_FIELD(SilenceSuppression);
DEF_FIELD(SilenceThreshold);
DEF_FIELD(SignalDeadband);
DEF_FIELD(SilenceDeadband);
DEF_FIELD(VideoGrabber);
DEF_FIELD(VideoGrabFormat);
DEF_FIELD(VideoGrabSource);
DEF_FIELD(VideoEncodeQuality);
DEF_FIELD(VideoGrabFrameRate);
DEF_FIELD(VideoEncodeMaxBitRate);
DEF_FIELD(VideoGrabPreview);
DEF_FIELD(VideoFlipLocal);
DEF_FIELD(VideoAutoTransmit);
DEF_FIELD(VideoAutoReceive);
DEF_FIELD(VideoFlipRemote);
DEF_FIELD(NewAlias);
DEF_FIELD(GatekeeperMode);
DEF_FIELD(GatekeeperAddress);
DEF_FIELD(GatekeeperIdentifier);
DEF_FIELD(GatekeeperTTL);
DEF_FIELD(GatekeeperLogin);
DEF_FIELD(GatekeeperPassword);
DEF_FIELD(DTMFSendMode);
DEF_FIELD(CallIntrusionProtectionLevel);
DEF_FIELD(DisableFastStart);
DEF_FIELD(DisableH245Tunneling);
DEF_FIELD(DisableH245inSETUP);
DEF_FIELD(SIPProxy);
DEF_FIELD(SIPProxyUsername);
DEF_FIELD(SIPProxyPassword);
DEF_FIELD(RegistrarName);
DEF_FIELD(RegistrarUsername);
DEF_FIELD(RegistrarPassword);
DEF_FIELD(RouteSource);
DEF_FIELD(RoutePattern);
DEF_FIELD(RouteDestination);
#if PTRACING
DEF_FIELD(EnableTracing);
DEF_FIELD(TraceLevelThreshold);
DEF_FIELD(TraceLevelNumber);
DEF_FIELD(TraceFileLine);
DEF_FIELD(TraceBlocks);
DEF_FIELD(TraceDateTime);
DEF_FIELD(TraceTimestamp);
DEF_FIELD(TraceThreadName);
DEF_FIELD(TraceThreadAddress);
DEF_FIELD(TraceFileName);
DEF_FIELD(TraceOptions);
#endif

///////////////////////////////////////////////////////////////////////////////

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
  wxConfig::Set(new wxConfig((const char *)GetName(), (const char *)GetManufacturer()));
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
  EVT_CLOSE(MyFrame::OnClose)

  EVT_MENU_OPEN(MyFrame::OnAdjustMenus)

  EVT_MENU(XRCID("MenuQuit"),   MyFrame::OnMenuQuit)
  EVT_MENU(XRCID("MenuAbout"),  MyFrame::OnMenuAbout)
  EVT_MENU(XRCID("MenuCall"),   MyFrame::OnMenuCall)
  EVT_MENU(XRCID("MenuAnswer"), MyFrame::OnMenuAnswer)
  EVT_MENU(XRCID("MenuHangUp"), MyFrame::OnMenuHangUp)
  EVT_MENU(XRCID("ViewLarge"),  MyFrame::OnViewLarge)
  EVT_MENU(XRCID("ViewSmall"),  MyFrame::OnViewSmall)
  EVT_MENU(XRCID("ViewList"),   MyFrame::OnViewList)
  EVT_MENU(XRCID("ViewDetails"),MyFrame::OnViewDetails)
  EVT_MENU(XRCID("MenuOptions"),MyFrame::OnOptions)

  EVT_LIST_ITEM_ACTIVATED(SpeedDialsID, MyFrame::OnSpeedDial)
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
  wxConfigBase * config = wxConfig::Get();

  // Give it an icon
  SetIcon( wxICON(OpenOphone) );

  // Make a menubar
  wxMenuBar * menubar = wxXmlResource::Get()->LoadMenuBar("MenuBar");
  SetMenuBar(menubar);

  // Make the content of the main window, speed dial and log panes
  m_splitter = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxSP_3D);

  m_logWindow = new wxTextCtrl(m_splitter, -1, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxSUNKEN_BORDER);
  LogWindow.SetTextCtrl(m_logWindow);
  m_logWindow->SetForegroundColour(wxColour(0,255,0)); // Green
  m_logWindow->SetBackgroundColour(wxColour(0,0,0)); // Black

  int i;
  if (!config->Read(ActiveViewKey, &i) || i < 0 || i >= e_NumViews)
  {
    i = e_ViewList;
  }
  static const char * const ViewMenuNames[e_NumViews] = {
    "ViewLarge", "ViewSmall", "ViewList", "ViewDetails"
  };
  menubar->Check(XRCID(ViewMenuNames[i]), true);
  RecreateSpeedDials((SpeedDialViews)i);

  // Set up sizer to automatically resize the splitter to size of window
  wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(m_splitter, 1, wxGROW, 0);
  SetSizer(sizer);

  int x, y = 0;
  if (config->Read(MainFrameXKey, &x) && config->Read(MainFrameYKey, &y))
    Move(x, y);

  int w, h = 0;
  if (config->Read(MainFrameWidthKey, &w) && config->Read(MainFrameHeightKey, &h))
    SetSize(w, h);

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


void MyFrame::RecreateSpeedDials(SpeedDialViews view)
{
  static DWORD const ListCtrlStyle[e_NumViews] = {
    wxLC_ICON, wxLC_SMALL_ICON, wxLC_LIST, wxLC_REPORT
  };

  wxListCtrl * oldSpeedDials = m_speedDials;

  m_speedDials = new wxListCtrl(m_splitter, SpeedDialsID,
                               wxDefaultPosition, wxDefaultSize,
                               ListCtrlStyle[view] | wxLC_EDIT_LABELS | wxSUNKEN_BORDER);

  m_speedDials->InsertColumn(0, _T("Name"));
  m_speedDials->InsertColumn(1, _T("Number"));
  m_speedDials->InsertColumn(e_AddressColumn, _T("Address"));

  // Test data ....
  int pos = m_speedDials->InsertItem(INT_MAX, _T("H.323 client"));
  m_speedDials->SetItem(pos, e_NumberColumn, _T("1"));
  m_speedDials->SetItem(pos, e_AddressColumn, _T("h323:10.0.1.1"));

  pos = m_speedDials->InsertItem(INT_MAX, _T("SIP Client"));
  m_speedDials->SetItem(pos, e_NumberColumn, _T("234"));
  m_speedDials->SetItem(pos, e_AddressColumn, _T("sip:10.0.2.250"));

  pos = m_speedDials->InsertItem(INT_MAX, _T("Vox Gratia"));
  m_speedDials->SetItem(pos, e_NumberColumn, _T("24"));
  m_speedDials->SetItem(pos, e_AddressColumn, _T("h323:h323.voxgratia.org"));

  // Now either replace the top half of the splitter or set it for the first time
  if (oldSpeedDials == NULL)
    m_splitter->SplitHorizontally(m_speedDials, m_logWindow);
  else {
    m_splitter->ReplaceWindow(oldSpeedDials, m_speedDials);
    delete oldSpeedDials;
  }

  wxConfig::Get()->Write(ActiveViewKey, view);
}


void MyFrame::OnClose(wxCloseEvent& event)
{
  wxConfigBase * config = wxConfig::Get();

  int x, y;
  GetPosition(&x, &y);
  config->Write(MainFrameXKey, x);
  config->Write(MainFrameYKey, y);

  int w, h;
  GetSize(&w, &h);
  config->Write(MainFrameWidthKey, w);
  config->Write(MainFrameHeightKey, h);

  Destroy();
}


void MyFrame::OnAdjustMenus(wxMenuEvent& WXUNUSED(event))
{
  wxMenuBar * menubar = GetMenuBar();
  bool inCall = !m_currentCallToken.IsEmpty();
  menubar->Enable(XRCID("MenuCall"),   !inCall);
  menubar->Enable(XRCID("MenuAnswer"), !pcssEP->m_incomingConnectionToken);
  menubar->Enable(XRCID("MenuHangUp"),  inCall);
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
  if (dlg.ShowModal() == wxID_OK)
    MakeCall(dlg.m_Address);
}


void MyFrame::OnMenuAnswer(wxCommandEvent& WXUNUSED(event))
{
  m_currentCallToken = pcssEP->m_incomingConnectionToken;
  pcssEP->m_incomingConnectionToken.MakeEmpty();
  pcssEP->AcceptIncomingConnection(m_currentCallToken);
}


void MyFrame::OnMenuHangUp(wxCommandEvent& WXUNUSED(event))
{
  if (!m_currentCallToken.IsEmpty())
    ClearCall(m_currentCallToken);
}


void MyFrame::OnViewLarge(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewLarge);
}


void MyFrame::OnViewSmall(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewSmall);
}


void MyFrame::OnViewList(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewList);
}


void MyFrame::OnViewDetails(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewDetails);
}


void MyFrame::OnSpeedDial(wxListEvent& event)
{
  wxListItem item;
  item.m_itemId = event.GetIndex();
  if (item.m_itemId < 0)
    return;

  item.m_col = e_AddressColumn;
  item.m_mask = wxLIST_MASK_TEXT;
  if (m_speedDials->GetItem(item))
    MakeCall(item.m_text);
}


void MyFrame::MakeCall(const PwxString & address)
{
  if (address.IsEmpty())
    return;

  LogWindow << "Calling \"" << address << '"' << endl;

  if (potsEP != NULL && potsEP->GetLine("*") != NULL)
    SetUpCall("pots:*", address, m_currentCallToken);
  else
    SetUpCall("pc:*", address, m_currentCallToken);
}


void MyFrame::OnEstablishedCall(OpalCall & call)
{
  m_currentCallToken = call.GetToken();
  LogWindow << "In call with " << call.GetPartyB() << " using " << call.GetPartyA() << endl;
}


void MyFrame::OnClearedCall(OpalCall & call)
{
  if (m_currentCallToken == call.GetToken())
    m_currentCallToken.MakeEmpty();

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

  PString prefix = connection.GetEndPoint().GetPrefixName();
  if (prefix == pcssEP->GetPrefixName())
    return TRUE;

  LogWindow << "Started ";

  if (stream.IsSource())
    LogWindow << "receiving ";
  else
    LogWindow << "sending ";

  LogWindow << stream.GetMediaFormat();

  if (stream.IsSource())
    LogWindow << " from ";
  else
    LogWindow << " to ";

  LogWindow << connection.GetEndPoint().GetPrefixName() << " endpoint" << endl;

  return TRUE;
}


void MyFrame::OnUserInputString(OpalConnection & connection, const PString & value)
{
  LogWindow << "User input received: \"" << value << '"' << endl;
  OpalManager::OnUserInputString(connection, value);
}


bool MyFrame::Initialise()
{
  wxConfigBase * config = wxConfig::Get();

#if PTRACING
  if (config->Read(EnableTracingKey, &m_enableTracing, false) && m_enableTracing &&
      config->Read(TraceFileNameKey, &m_traceFileName) && !m_traceFileName.empty()) {
    int traceLevelThreshold = 3;
    config->Read(TraceLevelThresholdKey, &traceLevelThreshold);
    int traceOptions = PTrace::DateAndTime|PTrace::Thread|PTrace::FileAndLine;
    config->Read(TraceOptionsKey, &traceOptions);
    PTrace::Initialise(traceLevelThreshold, m_traceFileName, traceOptions);
  }
#endif

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

  potsEP = new OpalPOTSEndPoint(*this);
  pcssEP = new MyPCSSEndPoint(*this);

  PwxString str;
  bool on;
  int value1, value2;

#define LOAD_FIELD_STR(name, set) \
  if (config->Read(name##Key, &str)) set(str)
#define LOAD_FIELD_BOOL(name, set) \
  if (config->Read(name##Key, &on)) set(on)
#define LOAD_FIELD_INT(name, set) \
  if (config->Read(name##Key, &value1)) set(value1)
#define LOAD_FIELD_INT2(name1, name2, set) \
  if (config->Read(name1##Key, &value1)) { \
    config->Read(name2##Key, &value2, 0); \
    set(value1, value2); \
  }
#define LOAD_FIELD_ENUM(name, set, type) \
  if (config->Read(name##Key, &value1)) set((type)value1)

  LOAD_FIELD_STR(Username, SetDefaultUserName);
  LOAD_FIELD_STR(DisplayName, SetDefaultDisplayName);
  LOAD_FIELD_BOOL(AutoAnswer, pcssEP->SetAutoAnswer);
#if P_EXPAT
  LOAD_FIELD_STR(IVRScript, ivrEP->SetDefaultVXML);
#endif
  LOAD_FIELD_INT2(TCPPortBase, TCPPortBase, SetTCPPorts);
  LOAD_FIELD_INT2(UDPPortBase, UDPPortBase, SetUDPPorts);
  LOAD_FIELD_INT2(RTPPortBase, RTPPortBase, SetRtpIpPorts);
  LOAD_FIELD_INT(RTPTOS, SetRtpIpTypeofService);
  LOAD_FIELD_STR(NATRouter, SetTranslationAddress);
  LOAD_FIELD_STR(STUNServer, SetSTUNServer);
  LOAD_FIELD_STR(SoundPlayer, pcssEP->SetSoundChannelPlayDevice);
  LOAD_FIELD_STR(SoundRecorder, pcssEP->SetSoundChannelRecordDevice);
  LOAD_FIELD_INT(SoundBuffers, pcssEP->SetSoundChannelBufferDepth);

  OpalLine * line = potsEP->GetLine("*");
  if (line != NULL) {
    LOAD_FIELD_STR(LineInterfaceDevice, line->GetDevice().Open);
    LOAD_FIELD_ENUM(AEC, line->SetAEC, OpalLineInterfaceDevice::AECLevels);
    LOAD_FIELD_STR(Country, line->GetDevice().SetCountryCodeName);
  }

  LOAD_FIELD_INT2(MinJitter, MaxJitter, SetAudioJitterDelay);

  OpalSilenceDetector::Params silenceParams = GetSilenceDetectParams();
  LOAD_FIELD_INT(SilenceSuppression, silenceParams.m_mode = (OpalSilenceDetector::Mode));
  LOAD_FIELD_INT(SilenceThreshold, silenceParams.m_threshold = );
  LOAD_FIELD_INT(SignalDeadband, silenceParams.m_signalDeadband = 8*);
  LOAD_FIELD_INT(SilenceDeadband, silenceParams.m_silenceDeadband = 8*);
  SetSilenceDetectParams(silenceParams);

  PVideoDevice::OpenArgs videoArgs = GetVideoInputDevice();
  LOAD_FIELD_STR(VideoGrabber, videoArgs.deviceName = (PString));
  LOAD_FIELD_INT(VideoGrabFormat, videoArgs.videoFormat = (PVideoDevice::VideoFormat));
  LOAD_FIELD_INT(VideoGrabSource, videoArgs.channelNumber = );
  LOAD_FIELD_INT(VideoGrabFrameRate, videoArgs.rate = );
  LOAD_FIELD_BOOL(VideoFlipLocal, videoArgs.flip = );
  SetVideoInputDevice(videoArgs);

//  LOAD_FIELD_INT(VideoEncodeQuality, );
//  LOAD_FIELD_INT(VideoEncodeMaxBitRate, );
//  LOAD_FIELD_BOOL(VideoGrabPreview, );
  LOAD_FIELD_BOOL(VideoAutoTransmit, SetAutoStartTransmitVideo);
  LOAD_FIELD_BOOL(VideoAutoReceive, SetAutoStartReceiveVideo);

  videoArgs = GetVideoOutputDevice();
  LOAD_FIELD_BOOL(VideoFlipRemote, videoArgs.flip = );
  SetVideoOutputDevice(videoArgs);

  LOAD_FIELD_ENUM(DTMFSendMode, h323EP->SetSendUserInputMode, H323Connection::SendUserInputModes);
  LOAD_FIELD_INT(CallIntrusionProtectionLevel, h323EP->SetCallIntrusionProtectionLevel);
  LOAD_FIELD_BOOL(DisableFastStart, h323EP->DisableFastStart);
  LOAD_FIELD_BOOL(DisableH245Tunneling, h323EP->DisableH245Tunneling);
  LOAD_FIELD_BOOL(DisableH245inSETUP, h323EP->DisableH245inSetup);

  PwxString hostname, username, password;
  const SIPURL & proxy = sipEP->GetProxy();
  config->Read(SIPProxyKey, &hostname, PwxString(proxy.GetHostName()));
  config->Read(SIPProxyUsernameKey, &username, PwxString(proxy.GetUserName()));
  config->Read(SIPProxyPasswordKey, &password, PwxString(proxy.GetPassword()));
  sipEP->SetProxy(hostname, username, password);

  const SIPURL & registrationAddress = sipEP->GetRegistrationAddress();
  config->Read(RegistrarNameKey, &hostname, PwxString(registrationAddress.GetHostName()));
  config->Read(RegistrarUsernameKey, &username, PwxString(registrationAddress.GetUserName()));
  config->Read(RegistrarPasswordKey, &password, PwxString(registrationAddress.GetPassword()));
  sipEP->Register(hostname, username, password);

  int mode;
  config->Read(GatekeeperModeKey, &mode, 0);
  if (mode > 0) {
    config->Read(GatekeeperAddressKey, &hostname, "");
    config->Read(GatekeeperIdentifierKey, &username, "");
    if (h323EP->UseGatekeeper(hostname, username)) {
      if (config->Read(GatekeeperTTLKey, &value1))
        h323EP->SetGatekeeperTimeToLive(PTimeInterval(0, value1));

      config->Read(GatekeeperLoginKey, &username, "");
      config->Read(GatekeeperPasswordKey, &password, "");
      h323EP->SetGatekeeperPassword(password, username);
    }
    else {
      if (mode > 1) {
        return false;
      }
    }
  }

  {
#if OPAL_SIP
    if (sipEP != NULL) {
      AddRouteEntry("pots:.*\\*.*\\*.* = sip:<dn2ip>");
      AddRouteEntry("pots:.*           = sip:<da>");
      AddRouteEntry("pc:.*             = sip:<da>");
    }
#if OPAL_H323
    else
#endif
#endif

#if OPAL_H323
    if (h323EP != NULL) {
      AddRouteEntry("pots:.*\\*.*\\*.* = h323:<dn2ip>");
      AddRouteEntry("pots:.*           = h323:<da>");
      AddRouteEntry("pc:.*             = h323:<da>");
    }
#endif

#if P_EXPAT
    if (ivrEP != NULL)
      AddRouteEntry(".*:#  = ivr:"); // A hash from anywhere goes to IVR
#endif

    if (potsEP != NULL && potsEP->GetLine("*") != NULL) {
#if OPAL_H323
      AddRouteEntry("h323:.* = pots:<da>");
#endif
#if OPAL_SIP
      AddRouteEntry("sip:.*  = pots:<da>");
#endif
    }
    else if (pcssEP != NULL) {
#if OPAL_H323
      AddRouteEntry("h323:.* = pc:<da>");
#endif
#if OPAL_SIP
      AddRouteEntry("sip:.*  = pc:<da>");
#endif
    }
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////////

void MyFrame::OnOptions(wxCommandEvent& event)
{
  OptionsDialog dlg(this);
  dlg.ShowModal();
}

BEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
  EVT_BUTTON(XRCID("AddRoute"), OptionsDialog::AddRoute)
  EVT_BUTTON(XRCID("RemoveRoute"), OptionsDialog::RemoveRoute)
  EVT_LIST_ITEM_SELECTED(XRCID("Routes"), OptionsDialog::SelectedRoute)
  EVT_LIST_ITEM_DESELECTED(XRCID("Routes"), OptionsDialog::DeselectedRoute)
  EVT_TEXT(XRCID("RoutePattern"), OptionsDialog::ChangedRouteInfo)
  EVT_TEXT(XRCID("RouteDestination"), OptionsDialog::ChangedRouteInfo)
END_EVENT_TABLE()

#define INIT_FIELD(name, value) \
  m_##name = value; \
  FindWindowByName(name##Key)->SetValidator(wxGenericValidator(&m_##name))

static void PopulateCodecList(const char * prefix,
                              const OpalMediaFormatList & mediaFormats,
                              wxListBox * listBox,
                              OpalMediaFormatList * allMediaFormats)
{
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++) {
    const OpalMediaFormat & mediaFormat = mediaFormats[i];

    if (mediaFormat.GetPayloadType() != RTP_DataFrame::MaxPayloadType) {
      wxString str = prefix;
      str += (const char *)mediaFormat;
      listBox->Append(str);
    }

    if (allMediaFormats != NULL)
    *allMediaFormats += mediaFormat;
  }
}

OptionsDialog::OptionsDialog(MyFrame *parent)
  : mainFrame(*parent)
{
  PINDEX i;

  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
  wxXmlResource::Get()->LoadDialog(this, parent, "OptionsDialog");

  ////////////////////////////////////////
  // General fields
  INIT_FIELD(Username, mainFrame.GetDefaultUserName());
  INIT_FIELD(DisplayName, mainFrame.GetDefaultDisplayName());
  INIT_FIELD(RingSoundFileName, "");
  INIT_FIELD(AutoAnswer, mainFrame.pcssEP->GetAutoAnswer());
#if P_EXPAT
  INIT_FIELD(IVRScript, mainFrame.ivrEP->GetDefaultVXML());
#endif

  ////////////////////////////////////////
  // Networking fields
  INIT_FIELD(Bandwidth, mainFrame.h323EP->GetInitialBandwidth());
  INIT_FIELD(TCPPortBase, mainFrame.GetTCPPortBase());
  INIT_FIELD(TCPPortMax, mainFrame.GetTCPPortMax());
  INIT_FIELD(UDPPortBase, mainFrame.GetUDPPortBase());
  INIT_FIELD(UDPPortMax, mainFrame.GetUDPPortMax());
  INIT_FIELD(RTPPortBase, mainFrame.GetRtpIpPortBase());
  INIT_FIELD(RTPPortMax, mainFrame.GetRtpIpPortMax());
  INIT_FIELD(RTPTOS, mainFrame.GetRtpIpTypeofService());
  INIT_FIELD(STUNServer, mainFrame.GetSTUN() != NULL ? mainFrame.GetSTUN()->GetServer() : PString());
  INIT_FIELD(NATRouter, mainFrame.GetTranslationAddress().AsString());

  ////////////////////////////////////////
  // Sound fields
  INIT_FIELD(SoundBuffers, mainFrame.pcssEP->GetSoundChannelBufferDepth());
  INIT_FIELD(MinJitter, mainFrame.GetMinAudioJitterDelay());
  INIT_FIELD(MaxJitter, mainFrame.GetMaxAudioJitterDelay());
  INIT_FIELD(SilenceSuppression, mainFrame.GetSilenceDetectParams().m_mode);
  INIT_FIELD(SilenceThreshold, mainFrame.GetSilenceDetectParams().m_threshold);
  INIT_FIELD(SignalDeadband, mainFrame.GetSilenceDetectParams().m_signalDeadband/8);
  INIT_FIELD(SilenceDeadband, mainFrame.GetSilenceDetectParams().m_silenceDeadband/8);

  // Fill sound player combo box with available devices and set selection
  wxComboBox * combo = (wxComboBox *)FindWindowByName(SoundPlayerKey);
  combo->SetValidator(wxGenericValidator(&m_SoundPlayer));
  PStringList devices = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
  for (i = 0; i < devices.GetSize(); i++)
    combo->Append((const char *)devices[i]);
  m_SoundPlayer = mainFrame.pcssEP->GetSoundChannelPlayDevice();

  // Fill sound recorder combo box with available devices and set selection
  combo = (wxComboBox *)FindWindowByName(SoundRecorderKey);
  combo->SetValidator(wxGenericValidator(&m_SoundRecorder));
  devices = PSoundChannel::GetDeviceNames(PSoundChannel::Recorder);
  for (i = 0; i < devices.GetSize(); i++)
    combo->Append((const char *)devices[i]);
  m_SoundRecorder = mainFrame.pcssEP->GetSoundChannelRecordDevice();

  // Fill line interface combo box with available devices and set selection
  combo = (wxComboBox *)FindWindowByName(LineInterfaceDeviceKey);
  combo->SetValidator(wxGenericValidator(&m_LineInterfaceDevice));
  OpalLine * line = mainFrame.potsEP->GetLine("*");
  if (line != NULL) {
    devices = line->GetDevice().GetAllNames();
    for (i = 0; i < devices.GetSize(); i++)
      combo->Append((const char *)devices[i]);
    m_LineInterfaceDevice = line->GetDevice().GetName();
    INIT_FIELD(AEC, line->GetAEC());
    INIT_FIELD(Country, line->GetDevice().GetCountryCodeName());
  }
  else {
    m_LineInterfaceDevice = "<< None available >>";
    combo->Append(m_LineInterfaceDevice);
    FindWindowByName(AECKey)->Disable();
    FindWindowByName(CountryKey)->Disable();
  }

  ////////////////////////////////////////
  // Video fields
  INIT_FIELD(VideoGrabber, mainFrame.GetVideoInputDevice().deviceName);
  INIT_FIELD(VideoGrabFormat, mainFrame.GetVideoInputDevice().videoFormat);
  INIT_FIELD(VideoGrabSource, mainFrame.GetVideoInputDevice().channelNumber);
  INIT_FIELD(VideoGrabFrameRate, mainFrame.GetVideoInputDevice().rate);
  INIT_FIELD(VideoFlipLocal, mainFrame.GetVideoInputDevice().flip != FALSE);
//  INIT_FIELD(VideoEncodeQuality, mainFrame);
//  INIT_FIELD(VideoEncodeMaxBitRate, mainFrame);
//  INIT_FIELD(VideoGrabPreview, mainFrame);
  INIT_FIELD(VideoAutoTransmit, mainFrame.CanAutoStartTransmitVideo() != FALSE);
  INIT_FIELD(VideoAutoReceive, mainFrame.CanAutoStartReceiveVideo() != FALSE);
  INIT_FIELD(VideoFlipRemote, mainFrame.GetVideoOutputDevice().flip != FALSE);

  ////////////////////////////////////////
  // Codec fields
  wxListBox * listBox = (wxListBox *)FindWindowByName("AllCodecs");
  OpalMediaFormatList allMediaFormats;
  PopulateCodecList("pc: ", mainFrame.pcssEP->GetMediaFormats(), listBox, &allMediaFormats);
  PopulateCodecList("pots: ", mainFrame.potsEP->GetMediaFormats(), listBox, &allMediaFormats);
#if P_EXPAT
  PopulateCodecList("ivr: ", mainFrame.ivrEP->GetMediaFormats(), listBox, &allMediaFormats);
#endif
  PopulateCodecList("sw: ", OpalTranscoder::GetPossibleFormats(allMediaFormats), listBox, NULL);

  ////////////////////////////////////////
  // H.323 fields
  INIT_FIELD(DTMFSendMode, mainFrame.h323EP->GetSendUserInputMode());
  INIT_FIELD(CallIntrusionProtectionLevel, mainFrame.h323EP->GetCallIntrusionProtectionLevel());
  INIT_FIELD(DisableFastStart, mainFrame.h323EP->IsFastStartDisabled() != FALSE);
  INIT_FIELD(DisableH245Tunneling, mainFrame.h323EP->IsH245TunnelingDisabled() != FALSE);
  INIT_FIELD(DisableH245inSETUP, mainFrame.h323EP->IsH245inSetupDisabled() != FALSE);

  ////////////////////////////////////////
  // SIP fields
  INIT_FIELD(SIPProxy, mainFrame.sipEP->GetProxy().GetHostName());
  INIT_FIELD(SIPProxyUsername, mainFrame.sipEP->GetProxy().GetUserName());
  INIT_FIELD(SIPProxyPassword, mainFrame.sipEP->GetProxy().GetPassword());
  INIT_FIELD(RegistrarName, mainFrame.sipEP->GetRegistrationAddress().GetHostName());
  INIT_FIELD(RegistrarUsername, mainFrame.sipEP->GetRegistrationAddress().GetUserName());
  INIT_FIELD(RegistrarPassword, mainFrame.sipEP->GetRegistrationAddress().GetPassword());

  ////////////////////////////////////////
  // Routing fields
  m_SelectedRoute = INT_MAX;

  m_RoutePattern = (wxTextCtrl *)FindWindowByName("RoutePattern");
  m_RouteDestination = (wxTextCtrl *)FindWindowByName("RouteDestination");

  m_AddRoute = (wxButton *)FindWindowByName("AddRoute");
  m_AddRoute->Disable();

  m_RemoveRoute = (wxButton *)FindWindowByName("RemoveRoute");
  m_RemoveRoute->Disable();

  // Fill list box with active routes
  static char const AllSources[] = "<ALL>";
  m_Routes = (wxListCtrl *)FindWindowByName("Routes");
  m_Routes->InsertColumn(0, _T("Source"));
  m_Routes->InsertColumn(1, _T("Pattern"));
  m_Routes->InsertColumn(2, _T("Destination"));
  const OpalManager::RouteTable & routeTable = mainFrame.GetRouteTable();
  for (i = 0; i < routeTable.GetSize(); i++) {
    PString pattern = routeTable[i].pattern;
    PINDEX colon = pattern.Find(':');
    wxString source;
    if (colon == P_MAX_INDEX) {
      source = AllSources;
      colon = 0;
    }
    else {
      source = (const char *)pattern.Left(colon);
      if (source == ".*")
        source = AllSources;
      ++colon;
    }
    int pos = m_Routes->InsertItem(INT_MAX, source);
    m_Routes->SetItem(pos, 1, (const char *)pattern.Mid(colon));
    m_Routes->SetItem(pos, 2, (const char *)routeTable[i].destination);
  }

  // Fill combo box with possible protocols
  m_RouteSource = (wxComboBox *)FindWindowByName("RouteSource");
  m_RouteSource->Append(AllSources);
  const OpalEndPointList & endponts = mainFrame.GetEndPoints();
  for (i = 0; i < endponts.GetSize(); i++)
    m_RouteSource->Append((const char *)endponts[i].GetPrefixName());
  m_RouteSource->SetSelection(0);


#if PTRACING
  ////////////////////////////////////////
  // Tracing fields
  INIT_FIELD(EnableTracing, mainFrame.m_enableTracing);
  INIT_FIELD(TraceLevelThreshold, PTrace::GetLevel());
  INIT_FIELD(TraceLevelNumber, (PTrace::GetOptions()&PTrace::TraceLevel) != 0);
  INIT_FIELD(TraceFileLine, (PTrace::GetOptions()&PTrace::FileAndLine) != 0);
  INIT_FIELD(TraceBlocks, (PTrace::GetOptions()&PTrace::Blocks) != 0);
  INIT_FIELD(TraceDateTime, (PTrace::GetOptions()&PTrace::DateAndTime) != 0);
  INIT_FIELD(TraceTimestamp, (PTrace::GetOptions()&PTrace::Timestamp) != 0);
  INIT_FIELD(TraceThreadName, (PTrace::GetOptions()&PTrace::Thread) != 0);
  INIT_FIELD(TraceThreadAddress, (PTrace::GetOptions()&PTrace::ThreadAddress) != 0);
  INIT_FIELD(TraceFileName, mainFrame.m_traceFileName);
#endif // PTRACING
}


#define SAVE_FIELD(name, set) \
  set(m_##name); \
  config->Write(name##Key, m_##name)

#define SAVE_FIELD2(name1, name2, set) \
  set(m_##name1, m_##name2); \
  config->Write(name1##Key, m_##name1); \
  config->Write(name2##Key, m_##name2)

#define SAVE_FIELD3(name1, name2, name3, set) \
  set(m_##name1, m_##name2, m_##name3); \
  config->Write(name1##Key, m_##name1); \
  config->Write(name2##Key, m_##name2); \
  config->Write(name3##Key, m_##name3)

bool OptionsDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  wxConfigBase * config = wxConfig::Get();

  SAVE_FIELD(Username, mainFrame.SetDefaultUserName);
  SAVE_FIELD(DisplayName, mainFrame.SetDefaultDisplayName);
  SAVE_FIELD(AutoAnswer, mainFrame.pcssEP->SetAutoAnswer);
#if P_EXPAT
  SAVE_FIELD(IVRScript, mainFrame.ivrEP->SetDefaultVXML);
#endif

  SAVE_FIELD(Bandwidth, mainFrame.h323EP->SetInitialBandwidth);
  SAVE_FIELD2(TCPPortBase, TCPPortMax, mainFrame.SetTCPPorts);
  SAVE_FIELD2(UDPPortBase, UDPPortMax, mainFrame.SetUDPPorts);
  SAVE_FIELD2(RTPPortBase, RTPPortMax, mainFrame.SetRtpIpPorts);
  SAVE_FIELD(RTPTOS, mainFrame.SetRtpIpTypeofService);
  SAVE_FIELD(STUNServer, mainFrame.SetSTUNServer);
  SAVE_FIELD(NATRouter, mainFrame.SetTranslationAddress);

  SAVE_FIELD(SoundPlayer, mainFrame.pcssEP->SetSoundChannelPlayDevice);
  SAVE_FIELD(SoundRecorder, mainFrame.pcssEP->SetSoundChannelRecordDevice);
  SAVE_FIELD(SoundBuffers, mainFrame.pcssEP->SetSoundChannelBufferDepth);
//  SAVE_FIELD(LineInterfaceDevice, mainFrame.potsEP->);
//  SAVE_FIELD(AEC, mainFrame.potsEP->);
//  SAVE_FIELD(Country, mainFrame.potsEP->);
  SAVE_FIELD2(MinJitter, MaxJitter, mainFrame.SetAudioJitterDelay);

  OpalSilenceDetector::Params silenceParams;
  SAVE_FIELD(SilenceSuppression, silenceParams.m_mode=(OpalSilenceDetector::Mode));
  SAVE_FIELD(SilenceThreshold, silenceParams.m_threshold=);
  SAVE_FIELD(SignalDeadband, silenceParams.m_signalDeadband=8*);
  SAVE_FIELD(SilenceDeadband, silenceParams.m_silenceDeadband=8*);
  mainFrame.SetSilenceDetectParams(silenceParams);

  PVideoDevice::OpenArgs grabber = mainFrame.GetVideoInputDevice();
  SAVE_FIELD(VideoGrabber, grabber.deviceName = (PString));
  SAVE_FIELD(VideoGrabFormat, grabber.videoFormat = (PVideoDevice::VideoFormat));
  SAVE_FIELD(VideoGrabSource, grabber.channelNumber = );
  SAVE_FIELD(VideoGrabFrameRate, grabber.rate = );
  SAVE_FIELD(VideoFlipLocal, grabber.flip = );
  mainFrame.SetVideoInputDevice(grabber);
//  SAVE_FIELD(VideoEncodeQuality, );
//  SAVE_FIELD(VideoEncodeMaxBitRate, );
//  SAVE_FIELD(VideoGrabPreview, );
  SAVE_FIELD(VideoAutoTransmit, mainFrame.SetAutoStartTransmitVideo);
  SAVE_FIELD(VideoAutoReceive, mainFrame.SetAutoStartReceiveVideo);
//  SAVE_FIELD(VideoFlipRemote, );

  SAVE_FIELD2(GatekeeperPassword, GatekeeperLogin, mainFrame.h323EP->SetGatekeeperPassword);

  mainFrame.h323EP->SetSendUserInputMode((H323Connection::SendUserInputModes)m_DTMFSendMode);
  config->Write(DTMFSendModeKey, m_DTMFSendMode);
  SAVE_FIELD(CallIntrusionProtectionLevel, mainFrame.h323EP->SetCallIntrusionProtectionLevel);
  SAVE_FIELD(DisableFastStart, mainFrame.h323EP->DisableFastStart);
  SAVE_FIELD(DisableH245Tunneling, mainFrame.h323EP->DisableH245Tunneling);
  SAVE_FIELD(DisableH245inSETUP, mainFrame.h323EP->DisableH245inSetup);

  SAVE_FIELD3(SIPProxy, SIPProxyUsername, SIPProxyPassword, mainFrame.sipEP->SetProxy);
  SAVE_FIELD3(RegistrarName, RegistrarUsername, RegistrarPassword, mainFrame.sipEP->Register);

#if PTRACING
  int traceOptions = 0;
  if (m_TraceLevelNumber)
    traceOptions |= PTrace::TraceLevel;
  if (m_TraceFileLine)
    traceOptions |= PTrace::FileAndLine;
  if (m_TraceBlocks)
    traceOptions |= PTrace::Blocks;
  if (m_TraceDateTime)
    traceOptions |= PTrace::DateAndTime;
  if (m_TraceTimestamp)
    traceOptions |= PTrace::Timestamp;
  if (m_TraceThreadName)
    traceOptions |= PTrace::Thread;
  if (m_TraceThreadAddress)
    traceOptions |= PTrace::ThreadAddress;

  config->Write(EnableTracingKey, m_EnableTracing);
  config->Write(TraceLevelThresholdKey, m_TraceLevelThreshold);
  config->Write(TraceFileNameKey, m_TraceFileName);
  config->Write(TraceOptionsKey, traceOptions);

  // Check for stopping tracing
  if (mainFrame.m_enableTracing && (!m_EnableTracing || m_TraceFileName.empty()))
    PTrace::SetStream(NULL);
  else if (m_EnableTracing && (!mainFrame.m_enableTracing || mainFrame.m_traceFileName != m_TraceFileName))
    PTrace::Initialise(m_TraceLevelThreshold, m_TraceFileName, traceOptions);
  else {
    PTrace::SetLevel(m_TraceLevelThreshold);
    PTrace::SetOptions(traceOptions);
    PTrace::ClearOptions(~traceOptions);
  }

  mainFrame.m_enableTracing = m_EnableTracing;
  mainFrame.m_traceFileName = m_TraceFileName;
#endif // PTRACING

  return true;
}


void OptionsDialog::BrowseSoundFile(wxCommandEvent & event)
{
}


void OptionsDialog::PlaySoundFile(wxCommandEvent & event)
{
}


void OptionsDialog::BandwidthClass(wxCommandEvent & event)
{
}


void OptionsDialog::SelectedLocalInterface(wxCommandEvent & event)
{
}


void OptionsDialog::AddInterface(wxCommandEvent & event)
{
}


void OptionsDialog::RemoveInterface(wxCommandEvent & event)
{
}


void OptionsDialog::SelectedCodecToAdd(wxCommandEvent & event)
{
}


void OptionsDialog::AddCodec(wxCommandEvent & event)
{
}


void OptionsDialog::RemoveCodec(wxCommandEvent & event)
{
}


void OptionsDialog::MoveUpCodec(wxCommandEvent & event)
{
}


void OptionsDialog::MoveDownCodec(wxCommandEvent & event)
{
}


void OptionsDialog::ConfigureCodec(wxCommandEvent & event)
{
}


void OptionsDialog::SelectedCodec(wxCommandEvent & event)
{
}


void OptionsDialog::AddAlias(wxCommandEvent & event)
{
}


void OptionsDialog::RemoveAlias(wxCommandEvent & event)
{
}


void OptionsDialog::AddRoute(wxCommandEvent & )
{
  int pos = m_Routes->InsertItem(m_SelectedRoute, m_RouteSource->GetValue());
  m_Routes->SetItem(pos, 1, m_RoutePattern->GetValue());
  m_Routes->SetItem(pos, 2, m_RouteDestination->GetValue());
}


void OptionsDialog::RemoveRoute(wxCommandEvent & event)
{
  m_Routes->DeleteItem(m_SelectedRoute);
}


void OptionsDialog::SelectedRoute(wxListEvent & event)
{
  m_SelectedRoute = event.GetIndex();
  m_RemoveRoute->Enable(true);
}


void OptionsDialog::DeselectedRoute(wxListEvent & event)
{
  m_SelectedRoute = INT_MAX;
  m_RemoveRoute->Enable(false);
}


void OptionsDialog::ChangedRouteInfo(wxCommandEvent & event)
{
  m_AddRoute->Enable(!m_RoutePattern->GetValue().IsEmpty() && !m_RouteDestination->GetValue().IsEmpty());
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

MyPCSSEndPoint::MyPCSSEndPoint(MyFrame & ep)
  : OpalPCSSEndPoint(ep),
    frame(ep)
{
  m_autoAnswer = false;
}


PString MyPCSSEndPoint::OnGetDestination(const OpalPCSSConnection & /*connection*/)
{
  // Dialog to prompt for address
  return PString();
}


void MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  m_incomingConnectionToken = connection.GetToken();

  if (m_autoAnswer)
    AcceptIncomingConnection(m_incomingConnectionToken);
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


// End of File ///////////////////////////////////////////////////////////////
