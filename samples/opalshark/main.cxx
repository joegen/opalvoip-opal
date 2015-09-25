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

#include <ptlib/sound.h>
#include <ptlib/vconvert.h>
#include <codec/vidcodec.h>

#include <wx/xrc/xmlres.h>
#include <wx/config.h>
#include <wx/accel.h>
#include <wx/valgen.h>
#include <wx/progdlg.h>
#include <wx/cmdline.h>
#include <wx/splitter.h>
#include <wx/grid.h>
#include <wx/rawbmp.h>


#if defined(__WXGTK__)   || \
    defined(__WXMOTIF__) || \
    defined(__WXX11__)   || \
    defined(__WXMAC__)   || \
    defined(__WXMGL__)   || \
    defined(__WXCOCOA__)
  #include <wx/gdicmn.h>
  #include "opalshark.xpm"
#endif


extern void InitXmlResource(); // From resource.cpp whichis compiled openphone.xrc


static const wxChar OpalSharkString[] = wxT("OPAL Shark");
static const wxChar OpalSharkErrorString[] = wxT("OPAL Shark Error");

// Definitions of the configuration file section and key names

#define DEF_FIELD(name) static const wxChar name##Key[] = wxT(#name)

static const wxChar AppearanceGroup[] = wxT("/Appearance");
DEF_FIELD(MainFrameX);
DEF_FIELD(MainFrameY);
DEF_FIELD(MainFrameWidth);
DEF_FIELD(MainFrameHeight);

static const wxChar OptionsGroup[] = wxT("/Options");
DEF_FIELD(AudioDevice);
DEF_FIELD(VideoTiming);

// Menu and command identifiers
#define DEF_XRCID(name) static int ID_##name = XRCID(#name)
DEF_XRCID(MenuFullScreen);
DEF_XRCID(MenuCloseAll);
DEF_XRCID(Play);
DEF_XRCID(Stop);
DEF_XRCID(Pause);
DEF_XRCID(Resume);
DEF_XRCID(Step);

DEFINE_EVENT_TYPE(VideoUpdateEvent);
DEFINE_EVENT_TYPE(VideoEndedEvent);


#define PTraceModule() "OpalShark"


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
      wxCMD_LINE_DESC(wxCMD_LINE_PARAM,  "", "", "PCAP file to play", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL|wxCMD_LINE_PARAM_MULTIPLE),
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
  if (ok) {
    for (size_t i = 0; i < cmdLine.GetParamCount(); ++i)
      main->Load(cmdLine.GetParam(i));
  }

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


bool MyManager::Initialise(bool startMinimised)
{
  wxImage::AddHandler(new wxGIFHandler);
  wxXmlResource::Get()->InitAllHandlers();
  InitXmlResource();

  wxGridCellBoolEditor::UseStringValues(wxT("Yes"), wxT("No"));

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

  config->SetPath(PwxString(OptionsGroup));
  config->Read(AudioDeviceKey, m_options.m_AudioDevice);
  config->Read(VideoTimingKey, m_options.m_VideoTiming);

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

OptionsDialog::OptionsDialog(MyManager *manager, const MyOptions & options)
  : m_manager(*manager)
  , m_options(options)
{
  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("OptionsDialog"));

  FindWindowByName(AudioDeviceKey)->SetValidator(wxGenericValidator(&m_options.m_AudioDevice));
  FindWindowByName(VideoTimingKey)->SetValidator(wxGenericValidator(&m_options.m_VideoTiming));
}


bool OptionsDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(PwxString(OptionsGroup));
  config->Write(AudioDeviceKey, m_options.m_AudioDevice);
  config->Write(VideoTimingKey, m_options.m_VideoTiming);

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
    Load(dlg.GetPath());
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


void MyManager::Load(const PwxString & fname)
{
  new MyPlayer(this, fname);
}


///////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyPlayer, wxMDIChildFrame)
  EVT_CLOSE(MyPlayer::OnCloseWindow)

  EVT_MENU_OPEN(MyPlayer::OnMenuOpen)
  EVT_MENU_CLOSE(MyPlayer::OnMenuClose)
  EVT_MENU(wxID_CLOSE, MyPlayer::OnClose)

  EVT_GRID_CELL_CHANGED(MyPlayer::OnListChanged)

  EVT_BUTTON(ID_Play,   MyPlayer::OnPlay)
  EVT_BUTTON(ID_Stop,   MyPlayer::OnStop)
  EVT_BUTTON(ID_Pause,  MyPlayer::OnPause)
  EVT_BUTTON(ID_Resume, MyPlayer::OnResume)
  EVT_BUTTON(ID_Step,   MyPlayer::OnStep)

  EVT_COMMAND(wxID_HIGHEST, VideoEndedEvent, MyPlayer::OnStop)
END_EVENT_TABLE()


MyPlayer::MyPlayer(MyManager * manager, const PFilePath & filename)
  : wxMDIChildFrame(manager, wxID_ANY, PwxString(filename.GetTitle()))
  , m_manager(*manager)
  , m_discoverThread(NULL)
  , m_discoverProgress(NULL)
  , m_selectedRTP(0)
  , m_playThreadCtrl(CtlIdle)
  , m_playThread(NULL)
{
  wxXmlResource::Get()->LoadPanel(this, wxT("PlayerPanel"));

  FindWindowByNameAs(m_rtpList, this, wxT("RTPList"));
  FindWindowByNameAs(m_videoOutput, this, wxT("VideoOutput"));

  FindWindowByNameAs(m_play,   this, wxT("Play"));
  FindWindowByNameAs(m_stop,   this, wxT("Stop"));
  FindWindowByNameAs(m_pause,  this, wxT("Pause"));
  FindWindowByNameAs(m_resume, this, wxT("Resume"));
  FindWindowByNameAs(m_step,   this, wxT("Step"));

  if (m_pcapFile.Open(filename, PFile::ReadOnly)) {
    m_discoverThread = new PThreadObj<MyPlayer>(*this, &MyPlayer::Discover, false, "Discover");
    Show(true);
  }
  else {
    wxMessageBox("Could not open PCAP file", OpalSharkErrorString, wxICON_EXCLAMATION | wxOK);
    Close();
  }
}


MyPlayer::~MyPlayer()
{
  if (m_discoverThread != NULL) {
    m_discoverProgress = NULL;
    m_discoverThread->WaitForTermination();
    delete m_discoverThread;
  }

  if (m_playThread != NULL) {
    m_playThreadCtrl = CtlStop;
    m_playThread->WaitForTermination();
    delete m_playThread;
  }
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

void MyPlayer::Discover()
{
  wxProgressDialog progress(OpalSharkString,
                            PwxString(PSTRSTRM("Loading " << m_pcapFile.GetFilePath())),
                            m_pcapFile.GetLength(),
                            this,
                            wxPD_CAN_ABORT|wxPD_AUTO_HIDE);
  m_discoverProgress = &progress;

  bool found = m_pcapFile.DiscoverRTP(m_discoveredRTP, PCREATE_NOTIFIER(DiscoverProgress));

  m_discoverProgress = NULL;

  if (!found)
    return;

  {
    OpalPCAPFile::DiscoveredRTPInfo * info = new OpalPCAPFile::DiscoveredRTPInfo;
    info->m_src.SetAddress(PIPSocket::GetDefaultIpAny(), 5000);
    info->m_dst.SetAddress(PIPSocket::GetDefaultIpAny(), 5000);
    info->m_payloadType = RTP_DataFrame::PCMU;
    info->m_mediaFormat = OpalG711uLaw;
    m_discoveredRTP.Append(info);
  }

  m_rtpList->CreateGrid(m_discoveredRTP.size(), NumCols);

  for (int col = ColSrcIP; col < NumCols; ++col) {
    static wxChar const * const headings[] = {
      wxT("Src IP"),
      wxT("Src Port"),
      wxT("Dst IP"),
      wxT("Dst Port"),
      wxT("SSRC"),
      wxT("Type"),
      wxT("Format"),
      wxT("Play")
    };
    m_rtpList->SetColLabelValue(col, headings[col]);
  }
  m_rtpList->SetColLabelSize(wxGRID_AUTOSIZE);
  m_rtpList->AutoSizeColLabelSize(0);
  m_rtpList->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_TOP);
  m_rtpList->HideRowLabels();

  size_t row;
  for (row = 0; row < m_discoveredRTP.size(); ++row) {
    for (int col = ColSrcIP; col < NumCols; ++col) {
      m_rtpList->SetCellAlignment(row, col, wxALIGN_CENTRE, wxALIGN_TOP);
      if (row < m_discoveredRTP.size()-1 && col < ColFormat)
        m_rtpList->SetReadOnly(row, col);
    }

    wxArrayString formatNames;
    OpalMediaFormatList mediaFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
    for (OpalMediaFormatList::iterator it = mediaFormats.begin(); it != mediaFormats.end(); ++it) {
      if (it->IsTransportable() && (it->GetMediaType() == OpalMediaType::Audio() || it->GetMediaType() == OpalMediaType::Video()))
        formatNames.push_back(PwxString(it->GetName()));
    }
    m_rtpList->SetCellEditor(row, ColFormat, new wxGridCellChoiceEditor(formatNames));

    m_rtpList->SetCellEditor(row, ColPlay, new wxGridCellBoolEditor);

    const OpalPCAPFile::DiscoveredRTPInfo & info = m_discoveredRTP[row];
    m_rtpList->SetCellValue(row, ColSrcIP,       PwxString(PSTRSTRM(info.m_src.GetAddress())));
    m_rtpList->SetCellValue(row, ColSrcPort,     wxString() << info.m_src.GetPort());
    m_rtpList->SetCellValue(row, ColDstIP,       PwxString(PSTRSTRM(info.m_dst.GetAddress())));
    m_rtpList->SetCellValue(row, ColDstPort,     wxString() << info.m_dst.GetPort());
    m_rtpList->SetCellValue(row, ColSSRC,        wxString() << info.m_ssrc);
    m_rtpList->SetCellValue(row, ColPayloadType, PwxString(PSTRSTRM(info.m_payloadType)));
    m_rtpList->SetCellValue(row, ColFormat,      wxString() << info.m_mediaFormat);
    m_rtpList->SetCellValue(row, ColPlay,        wxT("No"));
  }

  m_rtpList->AutoSizeColumns();
  m_rtpList->SetColSize(ColFormat, m_rtpList->GetColSize(ColFormat)+40);
}


void  MyPlayer::DiscoverProgress(OpalPCAPFile &, OpalPCAPFile::Progress & progress)
{
  if (m_discoverProgress == NULL)
    progress.m_abort = true;
  else {
    progress.m_abort = m_discoverProgress->WasCancelled();
    m_discoverProgress->Update(progress.m_filePosition);
  }
}


void MyPlayer::OnListChanged(wxGridEvent & evt)
{
  PString value = PwxString(m_rtpList->GetCellValue(evt.GetRow(), evt.GetCol()));
  OpalPCAPFile::DiscoveredRTPInfo & info = m_discoveredRTP[evt.GetRow()];

  switch (evt.GetCol()) {
    case ColSrcIP :
      info.m_src.SetAddress(PIPAddress(value));
      break;
    case ColSrcPort :
      info.m_src.SetPort((WORD)value.AsUnsigned());
      break;
    case ColDstIP :
      info.m_dst.SetAddress(PIPAddress(value));
      break;
    case ColDstPort :
      info.m_dst.SetPort((WORD)value.AsUnsigned());
      break;
    case ColSSRC :
      info.m_ssrc = value.AsUnsigned();
      break;
    case ColPayloadType :
      info.m_payloadType = (RTP_DataFrame::PayloadTypes)value.AsUnsigned();
      break;
    case ColFormat :
      info.m_mediaFormat = value;
      break;
    case ColPlay:
      if (wxGridCellBoolEditor::IsTrueValue(PwxString(value))) {
        m_selectedRTP = evt.GetRow();
        for (size_t row = 0; row < m_discoveredRTP.size(); ++row) {
          if (m_selectedRTP != row && wxGridCellBoolEditor::IsTrueValue(m_rtpList->GetCellValue(row, ColPlay)))
            m_rtpList->SetCellValue(row, ColPlay, "No");
        }
        m_play->Enable(m_discoveredRTP[m_selectedRTP].m_mediaFormat.IsTransportable());
      }
      else {
        bool allOff = true;
        for (int row = 0; row < m_rtpList->GetNumberRows(); ++row) {
          if (wxGridCellBoolEditor::IsTrueValue(m_rtpList->GetCellValue(row, ColPlay))) {
            allOff = false;
            break;
          }
        }
        if (allOff)
          m_play->Disable();
      }
  }
}


void MyPlayer::OnPlay(wxCommandEvent &)
{
  m_rtpList->Disable();
  m_play->Disable();
  m_stop->Enable();
  m_pause->Enable();
  m_resume->Disable();
  m_step->Enable();

  m_pcapFile.SetFilters(m_discoveredRTP[m_selectedRTP]);

  if (!m_pcapFile.Restart()) {
    wxMessageBox("Could not restart PCAP file", OpalSharkErrorString);
    return;
  }

  m_playThreadCtrl = CtlRunning;
  if (m_discoveredRTP[m_selectedRTP].m_mediaFormat.GetMediaType() == OpalMediaType::Audio())
    m_playThread = new PThreadObj<MyPlayer>(*this, &MyPlayer::PlayAudio, false, "AudioPlayer");
  else
    m_playThread = new PThreadObj<MyPlayer>(*this, &MyPlayer::PlayVideo, false, "VideoPlayer");
}


void MyPlayer::OnStop(wxCommandEvent &)
{
  if (m_playThread != NULL) {
    PThread * thread = m_playThread;
    m_playThread = NULL;
    m_playThreadCtrl = CtlStop;
    thread->WaitForTermination();
    delete thread;
  }

  m_rtpList->Enable();
  m_play->Enable();
  m_stop->Disable();
  m_pause->Disable();
  m_resume->Disable();
  m_step->Disable();
}


void MyPlayer::OnPause(wxCommandEvent &)
{
  m_playThreadCtrl = CtlPause;
  m_pause->Disable();
  m_resume->Enable();
}


void MyPlayer::OnResume(wxCommandEvent &)
{
  m_playThreadCtrl = CtlRunning;
  m_pause->Disable();
  m_resume->Enable();
}


void MyPlayer::OnStep(wxCommandEvent &)
{
  m_playThreadCtrl = CtlStep;
}


void MyPlayer::PlayAudio()
{
  PTRACE(3, "Started audio player thread.");

  PSoundChannel * soundChannel = NULL;
  OpalTranscoder * transcoder = NULL;
  while (m_playThreadCtrl != CtlStop && !m_pcapFile.IsEndOfFile()) {
    while (m_playThreadCtrl == CtlPause) {
      PThread::Sleep(200);
    }

    RTP_DataFrame data;
    if (m_pcapFile.GetDecodedRTP(data, transcoder) <= 0)
      continue;

    if (soundChannel == NULL) {
      OpalMediaFormat format = transcoder->GetOutputFormat();
      soundChannel = new PSoundChannel(m_manager.GetAudioDevice(),
                                       PSoundChannel::Player,
                                       format.GetOptionInteger(OpalAudioFormat::ChannelsOption(), 1),
                                       format.GetClockRate());
    }

    if (!soundChannel->Write(data.GetPayloadPtr(), data.GetPaddingSize()))
      break;
  }

  delete transcoder;
  delete soundChannel;

  QueueEvent(new wxCommandEvent(VideoEndedEvent, wxID_HIGHEST));
  PTRACE(3, "Ended audio player thread.");
}


void MyPlayer::PlayVideo()
{
  PTRACE(3, "Started video player thread.");

  PTime realStartTime;
  PTime fileStartTime(0);
  RTP_Timestamp startTimestamp = 0;

  OpalTranscoder * transcoder = NULL;
  while (m_playThreadCtrl != CtlStop && !m_pcapFile.IsEndOfFile()) {
    while (m_playThreadCtrl == CtlPause) {
      PThread::Sleep(200);
      realStartTime.SetCurrentTime();
      fileStartTime.SetTimestamp(0);
      startTimestamp = 0;
    }

    RTP_DataFrame data;
    if (m_pcapFile.GetDecodedRTP(data, transcoder) <= 0)
      continue;

    PTimeInterval delay;
    if (m_manager.UseFileVideoTiming()) {
      if (fileStartTime.IsValid())
        delay = m_pcapFile.GetPacketTime() - fileStartTime - realStartTime.GetElapsed();
      else
        fileStartTime = m_pcapFile.GetPacketTime();
    }
    else {
      if (startTimestamp != 0)
        delay = (data.GetTimestamp() - startTimestamp)/90 - realStartTime.GetElapsed();
      else
        startTimestamp = data.GetTimestamp();
    }
    if (delay > 0)
      PThread::Sleep(delay);

    m_videoOutput->OutputVideo(data);
  }

  delete transcoder;

  QueueEvent(new wxCommandEvent(VideoEndedEvent, wxID_HIGHEST));
  PTRACE(3, "Ended video player thread.");
}


//////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(VideoOutputWindow, wxScrolledWindow);

BEGIN_EVENT_TABLE(VideoOutputWindow, wxScrolledWindow)
  EVT_PAINT(VideoOutputWindow::OnPaint)
  EVT_COMMAND(wxID_HIGHEST, VideoUpdateEvent, VideoOutputWindow::OnVideoUpdate)
END_EVENT_TABLE()

VideoOutputWindow::VideoOutputWindow()
  : m_converter(NULL)
  , m_bitmap(352,288)
{
}


VideoOutputWindow::~VideoOutputWindow()
{
  delete m_converter;
}


void VideoOutputWindow::OutputVideo(const RTP_DataFrame & data)
{
  m_mutex.Wait();

  const OpalVideoTranscoder::FrameHeader * header = (const OpalVideoTranscoder::FrameHeader *)data.GetPayloadPtr();

  if (m_converter == NULL)
    m_converter = PColourConverter::Create(PVideoFrameInfo(header->width, header->height),
                                           PVideoFrameInfo(header->width, header->height,
                                                           psprintf("BGR%u", m_bitmap.GetDepth())));
  else
    m_converter->SetSrcFrameSize(header->width, header->height);

  if (m_bitmap.Create(header->width, header->height)) {
    wxNativePixelData bmdata(m_bitmap);
    wxNativePixelData::Iterator it = bmdata.GetPixels();
    if (it.IsOk()) {
      bool flipped = bmdata.GetRowStride() < 0;
      if (flipped)
        it.Offset(bmdata, 0, header->height - 1);
      m_converter->SetVFlipState(flipped);

      if (PAssertNULL(m_converter)->Convert(OPAL_VIDEO_FRAME_DATA_PTR(header), (BYTE *)&it.Data())) {
        QueueEvent(new wxCommandEvent(VideoUpdateEvent, wxID_HIGHEST));
        PTRACE(4, "Posted video update event: " << header->width << 'x' << header->height << '@' << m_bitmap.GetDepth());
      }
    }
    else
      PTRACE(1, "Could not get pixel iterator in wxBitmap");
  }

  m_mutex.Signal();
}


void VideoOutputWindow::OnVideoUpdate(wxCommandEvent &)
{
  PTRACE(4, "VideoOutputWindow::OnVideoUpdate");
  Refresh(false);
}


void VideoOutputWindow::OnPaint(wxPaintEvent &)
{
  wxPaintDC dc(this);

  m_mutex.Wait();

  if (m_bitmap.IsOk()) {
    wxMemoryDC bmDC;
    bmDC.SelectObject(m_bitmap);
    if (dc.Blit(0, 0, m_bitmap.GetWidth(), m_bitmap.GetHeight(), &bmDC, 0, 0))
      PTRACE(5, "Updated screen.");
    else
      PTRACE(1, "Cannot update screen, wxBitmap Blit failed.");
  }
  else
    PTRACE(1, "Cannot update screen, wxBitmap invalid.");

  m_mutex.Signal();
}


// End of File ///////////////////////////////////////////////////////////////
