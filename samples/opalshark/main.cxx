/*
 * main.cpp
 *
 * An OPAL analyser/player for RTP.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2015 Vox Lucida
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
 * The Original Code is Opal Shark.
 *
 * The Initial Developer of the Original Code is Robert Jongbloed
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include "main.h"

#include "version.h"

#include <rtp/pcapfile.h>
#include <ptlib/vconvert.h>

#include <wx/xrc/xmlres.h>
#include <wx/gdicmn.h>     //Required for icons on linux.
#include <wx/config.h>
#include <wx/accel.h>
#include <wx/valgen.h>
#include <wx/progdlg.h>
#include <wx/cmdline.h>

#if defined(__WXGTK__)   || \
    defined(__WXMOTIF__) || \
    defined(__WXX11__)   || \
    defined(__WXMAC__)   || \
    defined(__WXMGL__)   || \
    defined(__WXCOCOA__)
#include "openphone.xpm"

#define VIDEO_WINDOW_DRIVER P_SDL_VIDEO_DRIVER
#define VIDEO_WINDOW_DEVICE P_SDL_VIDEO_PREFIX

#else

#define VIDEO_WINDOW_DRIVER P_MSWIN_VIDEO_DRIVER
#define VIDEO_WINDOW_DEVICE P_MSWIN_VIDEO_PREFIX" STYLE=0x80C80000"  // WS_POPUP|WS_BORDER|WS_SYSMENU|WS_CAPTION

#endif


extern void InitXmlResource(); // From resource.cpp whichis compiled openphone.xrc


// Definitions of the configuration file section and key names

#define DEF_FIELD(name) static const wxChar name##Key[] = wxT(#name)

static const wxChar AppearanceGroup[] = wxT("/Appearance");
DEF_FIELD(MainFrameX);
DEF_FIELD(MainFrameY);
DEF_FIELD(MainFrameWidth);
DEF_FIELD(MainFrameHeight);
DEF_FIELD(SashPosition);
DEF_FIELD(ActiveView);
static const wxChar ColumnWidthKey[] = wxT("ColumnWidth%u");

static const wxChar OpalSharkString[] = wxT("OPAL Shark");
static const wxChar OpalSharkErrorString[] = wxT("OPAL Shark Error");


// Menu and command identifiers
#define DEF_XRCID(name) static int ID_##name = XRCID(#name)
DEF_XRCID(MenuFullScreen);
DEF_XRCID(MenuCloseAll);
DEF_XRCID(MenuPlay);


#define PTraceModule() "OpalShark"


///////////////////////////////////////////////////////////////////////////////

class UserCommandEvent : public wxCommandEvent
{
public:
  UserCommandEvent(const wxChar * name)
    : wxCommandEvent(wxNewEventType(), wxXmlResource::GetXRCID(name))
  { }

  const wxEventType & GetEventTypeRef() const { return m_eventType; }
};

#define EVT_USER_COMMAND(name, fn) EVT_COMMAND((name).GetId(), (name).GetEventTypeRef(), fn)
#define DEFINE_USER_COMMAND(name) static UserCommandEvent const name(wxT(#name))
DEFINE_USER_COMMAND(wxEvtLogMessage);


///////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

template <class cls, typename id_type>
cls * FindWindowByNameAs(cls * & derivedChild, wxWindow * window, id_type name)
{
  wxWindow * baseChild = window->FindWindow(name);
  if (PAssert(baseChild != NULL, "Windows control not found")) {
    derivedChild = dynamic_cast<cls *>(baseChild);
    if (PAssert(derivedChild != NULL, "Cannot cast window object to selected class"))
      return derivedChild;
  }

  return NULL;
}

#ifdef _MSC_VER
#pragma warning(default:4100)
#endif


///////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_APP(OpalSharkApp);

OpalSharkApp::OpalSharkApp()
  : PProcess(MANUFACTURER_TEXT, PRODUCT_NAME_TEXT,
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}


void OpalSharkApp::Main()
{
  // Dummy function
}

//////////////////////////////////

bool OpalSharkApp::OnInit()
{
  // Check command line arguments
  static const wxCmdLineEntryDesc cmdLineDesc[] =
  {
#if wxCHECK_VERSION(2,9,0)
  #define wxCMD_LINE_DESC(kind, shortName, longName, description, ...) \
      { kind, shortName, longName, description, __VA_ARGS__ }
#else
  #define wxCMD_LINE_DESC(kind, shortName, longName, description, ...) \
      { kind, wxT(shortName), wxT(longName), wxT(description), __VA_ARGS__ }
  #define wxCMD_LINE_DESC_END { wxCMD_LINE_NONE, NULL, NULL, NULL, wxCMD_LINE_VAL_NONE, 0 }
#endif
      wxCMD_LINE_DESC(wxCMD_LINE_SWITCH, "h", "help", "", wxCMD_LINE_VAL_NONE,  wxCMD_LINE_OPTION_HELP),
      wxCMD_LINE_DESC(wxCMD_LINE_OPTION, "n", "config-name", "Set name to use for configuration", wxCMD_LINE_VAL_STRING),
      wxCMD_LINE_DESC(wxCMD_LINE_OPTION, "f", "config-file", "Use specified file for configuration", wxCMD_LINE_VAL_STRING),
      wxCMD_LINE_DESC(wxCMD_LINE_SWITCH, "m", "minimised", "Start application minimised"),
      wxCMD_LINE_DESC(wxCMD_LINE_PARAM,  "", "", "PCAP file to play", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL),
      wxCMD_LINE_DESC_END
  };

  wxCmdLineParser cmdLine(cmdLineDesc, wxApp::argc,  wxApp::argv);
  if (cmdLine.Parse() != 0)
    return false;

  {
    PwxString name(GetName());
    PwxString manufacture(GetManufacturer());
    PwxString filename;
    cmdLine.Found(wxT("config-name"), &name);
    cmdLine.Found(wxT("config-file"), &filename);
    wxConfig::Set(new wxConfig(name, manufacture, filename));
  }

  // Create the main frame window
  MyManager * main = new MyManager();
  SetTopWindow(main);

  wxBeginBusyCursor();

  bool ok = main->Initialise(cmdLine.Found(wxT("minimised")));
  if (ok && cmdLine.GetParamCount() > 0)
    main->Play(cmdLine.GetParam());

  wxEndBusyCursor();
  return ok;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyManager, wxMDIParentFrame)
  EVT_CLOSE(MyManager::OnClose)

  EVT_MENU_OPEN(MyManager::OnMenuOpen)
  EVT_MENU_CLOSE(MyManager::OnMenuClose)

  EVT_MENU(wxID_EXIT,         MyManager::OnMenuQuit)
  EVT_MENU(wxID_ABOUT,        MyManager::OnMenuAbout)
  EVT_MENU(wxID_PREFERENCES,  MyManager::OnMenuOptions)
  EVT_MENU(wxID_OPEN,         MyManager::OnMenuOpenPCAP)
  EVT_MENU(wxID_CLOSE_ALL,    MyManager::OnMenuCloseAll)
  EVT_MENU(ID_MenuFullScreen, MyManager::OnMenuFullScreen)

END_EVENT_TABLE()


MyManager::MyManager()
  : wxMDIParentFrame(NULL, wxID_ANY, OpalSharkString, wxDefaultPosition, wxSize(640, 480))
{
  // Give it an icon
  SetIcon(wxICON(AppIcon));
}


MyManager::~MyManager()
{
  // and disconnect it to prevent accessing already deleted m_textWindow in
  // the size event handler if it's called during destruction
  Disconnect(wxEVT_SIZE, wxSizeEventHandler(MyManager::OnSize));

  delete wxXmlResource::Set(NULL);
}


void MyManager::PostEvent(const wxCommandEvent & cmdEvent, const PString & str, const void * data)
{
  wxCommandEvent theEvent(cmdEvent);
  theEvent.SetEventObject(this);
  theEvent.SetString(PwxString(str));
  theEvent.SetClientData((void *)data);
  GetEventHandler()->AddPendingEvent(theEvent);
}


bool MyManager::Initialise(bool startMinimised)
{
  wxImage::AddHandler(new wxGIFHandler);
  wxXmlResource::Get()->InitAllHandlers();
  InitXmlResource();

  // Make a menubar
  wxMenuBar * menubar;
  {
    PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE;
    if ((menubar = wxXmlResource::Get()->LoadMenuBar(wxT("MenuBar"))) == NULL)
      return false;
    SetMenuBar(menubar);
  }

  wxAcceleratorEntry accelEntries[] = {
      wxAcceleratorEntry(wxACCEL_CTRL,  'O',         wxID_OPEN),
      wxAcceleratorEntry(wxACCEL_CTRL,  'A',         wxID_ABOUT),
      wxAcceleratorEntry(wxACCEL_CTRL,  'X',         wxID_EXIT)
  };
  SetAcceleratorTable(wxAcceleratorTable(PARRAYSIZE(accelEntries), accelEntries));

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(PwxString(AppearanceGroup));

  wxPoint initalPosition = wxDefaultPosition;
  if (config->Read(MainFrameXKey, &initalPosition.x) && config->Read(MainFrameYKey, &initalPosition.y))
    Move(initalPosition);

  wxSize initialSize(1024, 768);
  if (config->Read(MainFrameWidthKey, &initialSize.x) && config->Read(MainFrameHeightKey, &initialSize.y))
    SetSize(initialSize);

  // connect it only now, after creating m_textWindow
  Connect(wxEVT_SIZE, wxSizeEventHandler(MyManager::OnSize));

  // Show the frame window
  if (startMinimised)
    Iconize();
  Show(true);

  return true;
}


void MyManager::OnClose(wxCloseEvent &)
{
  ::wxBeginBusyCursor();

  wxProgressDialog progress(OpalSharkString, wxT("Exiting ..."));
  progress.Pulse();

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);

  if (!IsIconized()) {
    int x, y;
    GetPosition(&x, &y);
    config->Write(MainFrameXKey, x);
    config->Write(MainFrameYKey, y);

    int w, h;
    GetSize(&w, &h);
    config->Write(MainFrameWidthKey, w);
    config->Write(MainFrameHeightKey, h);
  }

  ShutDownEndpoints();

  Destroy();
}


void MyManager::OnMenuQuit(wxCommandEvent &)
{
  Close(true);
}


void MyManager::OnMenuAbout(wxCommandEvent &)
{
  PwxString text;
  PTime compiled(__DATE__);
  text << PRODUCT_NAME_TEXT " Version " << PProcess::Current().GetVersion() << "\n"
           "\n"
           "Copyright (c) " << COPYRIGHT_YEAR;
  if (compiled.GetYear() != COPYRIGHT_YEAR)
    text << '-' << compiled.GetYear();
  text << " " COPYRIGHT_HOLDER ", All rights reserved.\n"
          "\n"
          "This application may be used for any purpose so long as it is not sold "
          "or distributed for profit on it's own, or it's ownership by " COPYRIGHT_HOLDER
          " disguised or hidden in any way.\n"
          "\n"
          "Part of the Open Phone Abstraction Library, http://www.opalvoip.org\n"
          "  OPAL Version:  " << OpalGetVersion() << "\n"
          "  PTLib Version: " << PProcess::GetLibVersion() << '\n';
  wxMessageDialog dialog(this, text, wxT("About ..."), wxOK);
  dialog.ShowModal();
}


void MyManager::OnMenuOptions(wxCommandEvent &)
{
  PTRACE(4, "Opening options dialog");
  OptionsDialog dlg(this, m_options);
  if (dlg.ShowModal())
    m_options = dlg.GetOptions();
}


BEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
END_EVENT_TABLE()

DEF_FIELD(AudioDevice);
DEF_FIELD(VideoDevice);

OptionsDialog::OptionsDialog(MyManager *manager, const MyOptions & options)
  : m_manager(*manager)
  , m_options(options)
{
  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("OptionsDialog"));

  FindWindowByName(AudioDeviceKey)->SetValidator(wxGenericValidator(&m_options.m_AudioDevice));
  FindWindowByName(VideoDeviceKey)->SetValidator(wxGenericValidator(&m_options.m_VideoDevice));
}


bool OptionsDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  return true;
}


void MyManager::OnMenuOpenPCAP(wxCommandEvent &)
{
  wxFileDialog dlg(this,
                   wxT("Capture file to play"),
                   wxEmptyString,
                   wxEmptyString,
                   "Capture Files (*.pcap)|*.pcap");
  if (dlg.ShowModal() == wxID_OK)
    Play(dlg.GetPath());
}


void MyManager::OnMenuCloseAll(wxCommandEvent &)
{
  for (wxWindowList::const_iterator it = GetChildren().begin(); it != GetChildren().end(); ++it) {
    if (wxDynamicCast(*it, wxMDIChildFrame))
      (*it)->Close();
  }
}


void MyManager::OnMenuFullScreen(wxCommandEvent& commandEvent)
{
  ShowFullScreen(commandEvent.IsChecked());
}


void MyManager::Play(const PwxString & fname)
{
  MyPlayer * player = new MyPlayer(this, fname);
  player->Play();
}


///////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyPlayer, wxMDIChildFrame)
  EVT_CLOSE(MyPlayer::OnCloseWindow)

  EVT_MENU_OPEN(MyManager::OnMenuOpen)
  EVT_MENU_CLOSE(MyManager::OnMenuClose)

  EVT_MENU(wxID_CLOSE, MyPlayer::OnClose)
END_EVENT_TABLE()


MyPlayer::MyPlayer(MyManager * manager, const PFilePath & filename)
  : wxMDIChildFrame(manager, wxID_ANY, PwxString(filename.GetTitle()))
  , m_manager(*manager)
  , m_pcapFile(filename)
{
  Show(true);
}


void MyPlayer::OnCloseWindow(wxCloseEvent& closeEvent)
{
  if (wxMessageBox("Really close?", OpalSharkString, wxICON_QUESTION | wxYES_NO) != wxYES)
    closeEvent.Veto();
  else
    closeEvent.Skip();
}


void MyPlayer::OnClose(wxCommandEvent &)
{
  Close(true);
}


void MyPlayer::Play()
{
  OpalPCAPFile pcap;
  if (!pcap.Open(m_pcapFile, PFile::ReadOnly)) {
    wxMessageBox("Could not open PCAP file", OpalSharkErrorString, wxICON_EXCLAMATION | wxOK);
    return;
  }
}


// End of File ///////////////////////////////////////////////////////////////
