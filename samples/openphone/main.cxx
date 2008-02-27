/*
 * main.cpp
 *
 * An OPAL GUI phone application.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *    
 * Copyright (c) 2007 Vox Lucida
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
 * The Initial Developer of the Original Code is Robert Jongbloed
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUG__
#pragma implementation
#pragma interface
#endif

#include "main.h"
#include "version.h"

#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>
#include <wx/accel.h>
#include <wx/filesys.h>
#include <wx/gdicmn.h>     //Required for icons on linux. 
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/valgen.h>
#include <wx/notebook.h>
#undef LoadMenu // Bizarre but necessary before the xml code
#include <wx/xrc/xmlres.h>

#include <opal/transcoders.h>
#include <opal/ivr.h>
#include <lids/lidep.h>
#include <ptclib/pstun.h>


#if defined(__WXGTK__)   || \
    defined(__WXMOTIF__) || \
    defined(__WXX11__)   || \
    defined(__WXMAC__)   || \
    defined(__WXMGL__)   || \
    defined(__WXCOCOA__)
#include "openphone.xpm"
#include "h323phone.xpm"
#include "sipphone.xpm"
#include "otherphone.xpm"
#include "smallphone.xpm"

#define USE_SDL 1

#else

#define USE_SDL 0

#endif


extern void InitXmlResource(); // From resource.cpp whichis compiled openphone.xrc

// Definitions of the configuration file section and key names

#define DEF_FIELD(name) static const char name##Key[] = #name

static const char AppearanceGroup[] = "/Appearance";
DEF_FIELD(MainFrameX);
DEF_FIELD(MainFrameY);
DEF_FIELD(MainFrameWidth);
DEF_FIELD(MainFrameHeight);
DEF_FIELD(SashPosition);
DEF_FIELD(ActiveView);

static const char GeneralGroup[] = "/General";
DEF_FIELD(Username);
DEF_FIELD(DisplayName);
DEF_FIELD(RingSoundDeviceName);
DEF_FIELD(RingSoundFileName);
DEF_FIELD(AutoAnswer);
DEF_FIELD(IVRScript);
DEF_FIELD(SpeakerVolume);
DEF_FIELD(MicrophoneVolume);
DEF_FIELD(LastDialed);
DEF_FIELD(LastReceived);

static const char NetworkingGroup[] = "/Networking";
DEF_FIELD(Bandwidth);
DEF_FIELD(TCPPortBase);
DEF_FIELD(TCPPortMax);
DEF_FIELD(UDPPortBase);
DEF_FIELD(UDPPortMax);
DEF_FIELD(RTPPortBase);
DEF_FIELD(RTPPortMax);
DEF_FIELD(RTPTOS);
DEF_FIELD(NATHandling);
DEF_FIELD(NATRouter);
DEF_FIELD(STUNServer);

static const char LocalInterfacesGroup[] = "/Networking/Interfaces";

static const char AudioGroup[] = "/Audio";
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

static const char VideoGroup[] = "/Video";
DEF_FIELD(VideoGrabber);
DEF_FIELD(VideoGrabFormat);
DEF_FIELD(VideoGrabSource);
DEF_FIELD(VideoGrabFrameRate);
DEF_FIELD(VideoGrabFrameSize);
DEF_FIELD(VideoGrabPreview);
DEF_FIELD(VideoFlipLocal);
DEF_FIELD(VideoAutoTransmit);
DEF_FIELD(VideoAutoReceive);
DEF_FIELD(VideoFlipRemote);
DEF_FIELD(VideoMinFrameSize);
DEF_FIELD(VideoMaxFrameSize);
DEF_FIELD(LocalVideoFrameX);
DEF_FIELD(LocalVideoFrameY);
DEF_FIELD(RemoteVideoFrameX);
DEF_FIELD(RemoteVideoFrameY);

static const char CodecsGroup[] = "/Codecs";
static const char CodecNameKey[] = "Name";

static const char H323Group[] = "/H.323";
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

static const char H323AliasesGroup[] = "/H.323/Aliases";

static const char SIPGroup[] = "/SIP";
DEF_FIELD(SIPProxyUsed);
DEF_FIELD(SIPProxy);
DEF_FIELD(SIPProxyUsername);
DEF_FIELD(SIPProxyPassword);

static const char RegistrarGroup[] = "/SIP/Registrars";
DEF_FIELD(RegistrarUsed);
DEF_FIELD(RegistrarName);
DEF_FIELD(RegistrarDomain);
DEF_FIELD(RegistrarUsername);
DEF_FIELD(RegistrarPassword);
DEF_FIELD(RegistrarTimeToLive);

static const char RoutingGroup[] = "/Routes";

#if PTRACING
static const char TracingGroup[] = "/Tracing";
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

static const char SpeedDialsGroup[] = "/Speed Dials";
static const char SpeedDialAddressKey[] = "Address";
static const char SpeedDialNumberKey[] = "Number";
static const char SpeedDialDescriptionKey[] = "Description";

static const char RecentCallsGroup[] = "/Recent Calls";
static const size_t MaxSavedRecentCalls = 10;

static const char SIPonly[] = " (SIP only)";
static const char H323only[] = " (H.323 only)";


enum {
  ID_LOG_MESSAGE = 1001,
  ID_STATE_CHANGE,
  ID_UPDATE_STREAMS
};

DECLARE_EVENT_TYPE(wxEvtLogMessage, -1)
DEFINE_EVENT_TYPE(wxEvtLogMessage)

DECLARE_EVENT_TYPE(wxEvtUpdateStreams, -1)
DEFINE_EVENT_TYPE(wxEvtUpdateStreams)

DECLARE_EVENT_TYPE(wxEvtStateChange, -1)
DEFINE_EVENT_TYPE(wxEvtStateChange)


///////////////////////////////////////////////////////////////////////////////

template <class cls> cls * FindWindowByNameAs(wxWindow * window, const char * name)
{
  cls * child = dynamic_cast<cls *>(window->FindWindowByName(name));
  if (child != NULL)
    return child;
  PAssertAlways("Cannot cast window object to class");
  return NULL;
}


///////////////////////////////////////////////////////////////////////////////

class TextCtrlChannel : public PChannel
{
    PCLASSINFO(TextCtrlChannel, PChannel)
  public:
    TextCtrlChannel()
      : m_frame(NULL)
      { }

    virtual PBoolean Write(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
    ) {
      if (m_frame == NULL)
        return PFalse;

      wxCommandEvent theEvent(wxEvtLogMessage, ID_LOG_MESSAGE);
      theEvent.SetEventObject(m_frame);
      theEvent.SetString(wxString((const wxChar *)buf, (size_t)len));
      m_frame->GetEventHandler()->AddPendingEvent(theEvent);
      return PTrue;
    }

    void SetFrame(
      wxFrame * frame
    ) { m_frame = frame; }

    static TextCtrlChannel & Instance()
    {
      static TextCtrlChannel instance;
      return instance;
    }

  protected:
    wxFrame * m_frame;
};

#define LogWindow TextCtrlChannel::Instance()


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_APP(OpenPhoneApp)

OpenPhoneApp::OpenPhoneApp()
  : PProcess(MANUFACTURER_TEXT, PRODUCT_NAME_TEXT,
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
  // Create the main frame window
  MyManager * main = new MyManager();
  SetTopWindow(main);
  wxBeginBusyCursor();
  bool ok = main->Initialise();
  wxEndBusyCursor();
  return ok;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyManager, wxFrame)
  EVT_CLOSE(MyManager::OnClose)

  EVT_MENU_OPEN(MyManager::OnAdjustMenus)

  EVT_MENU(XRCID("MenuQuit"),     MyManager::OnMenuQuit)
  EVT_MENU(XRCID("MenuAbout"),    MyManager::OnMenuAbout)
  EVT_MENU(XRCID("MenuCall"),     MyManager::OnMenuCall)
  EVT_MENU(XRCID("MenuCallLastDialed"), MyManager::OnMenuCallLastDialed)
  EVT_MENU(XRCID("MenuCallLastReceived"),MyManager::OnMenuCallLastReceived)
  EVT_MENU(XRCID("MenuAnswer"),   MyManager::OnMenuAnswer)
  EVT_MENU(XRCID("MenuHangUp"),   MyManager::OnMenuHangUp)
  EVT_MENU(XRCID("NewSpeedDial"), MyManager::OnNewSpeedDial)
  EVT_MENU(XRCID("ViewLarge"),    MyManager::OnViewLarge)
  EVT_MENU(XRCID("ViewSmall"),    MyManager::OnViewSmall)
  EVT_MENU(XRCID("ViewList"),     MyManager::OnViewList)
  EVT_MENU(XRCID("ViewDetails"),  MyManager::OnViewDetails)
  EVT_MENU(XRCID("EditSpeedDial"),MyManager::OnEditSpeedDial)
  EVT_MENU(XRCID("MenuCut"),      MyManager::OnCutSpeedDial)
  EVT_MENU(XRCID("MenuCopy"),     MyManager::OnCopySpeedDial)
  EVT_MENU(XRCID("MenuPaste"),    MyManager::OnPasteSpeedDial)
  EVT_MENU(XRCID("MenuDelete"),   MyManager::OnDeleteSpeedDial)
  EVT_MENU(XRCID("MenuOptions"),  MyManager::OnOptions)

  EVT_SPLITTER_SASH_POS_CHANGED(SplitterID, MyManager::OnSashPositioned)
  EVT_LIST_ITEM_ACTIVATED(SpeedDialsID, MyManager::OnSpeedDialActivated)
  EVT_LIST_COL_END_DRAG(SpeedDialsID, MyManager::OnSpeedDialColumnResize)
  EVT_LIST_ITEM_RIGHT_CLICK(SpeedDialsID, MyManager::OnRightClick) 

  EVT_COMMAND(ID_LOG_MESSAGE, wxEvtLogMessage, MyManager::OnLogMessage)
  EVT_COMMAND(ID_STATE_CHANGE, wxEvtStateChange, MyManager::OnStateChange)
  EVT_COMMAND(ID_UPDATE_STREAMS, wxEvtUpdateStreams, MyManager::UpdateStreams)
END_EVENT_TABLE()

MyManager::MyManager()
  : wxFrame(NULL, -1, wxT("OpenPhone"), wxDefaultPosition, wxSize(640, 480))
  , m_speedDials(NULL)
  , pcssEP(NULL)
  , potsEP(NULL)
  , h323EP(NULL)
  , sipEP(NULL)
#if P_EXPAT
  , ivrEP(NULL)
#endif
  , m_autoAnswer(false)
  , m_VideoGrabPreview(true)
  , m_localVideoFrameX(INT_MIN)
  , m_localVideoFrameY(INT_MIN)
  , m_remoteVideoFrameX(INT_MIN)
  , m_remoteVideoFrameY(INT_MIN)
#if PTRACING
  , m_enableTracing(false)
#endif
  , m_callState(IdleState)
{
  // Give it an icon
  SetIcon(wxICON(AppIcon));

  // Make an image list containing large icons
  m_imageListNormal = new wxImageList(32, 32, true);

  // Order here is important!!
  m_imageListNormal->Add(wxICON(OtherPhone));
  m_imageListNormal->Add(wxICON(H323Phone));
  m_imageListNormal->Add(wxICON(SIPPhone));

  m_imageListSmall = new wxImageList(16, 16, true);
  m_imageListSmall->Add(wxICON(SmallPhone));

  m_RingSoundTimer.SetNotifier(PCREATE_NOTIFIER(OnRingSoundAgain));

  LogWindow.SetFrame(this);
}


MyManager::~MyManager()
{
  LogWindow.SetFrame(NULL);

  ShutDownEndpoints();

  delete m_imageListNormal;
  delete m_imageListSmall;
}


bool MyManager::Initialise()
{
  wxImage::AddHandler(new wxGIFHandler);
  wxXmlResource::Get()->InitAllHandlers();
  InitXmlResource();

  // Make a menubar
  wxMenuBar * menubar = wxXmlResource::Get()->LoadMenuBar("MenuBar");
  if (menubar == NULL)
    return false;

  SetMenuBar(menubar);

  wxAcceleratorEntry accelEntries[] = {
      wxAcceleratorEntry(wxACCEL_CTRL,  'E',         XRCID("EditSpeedDial")),
      wxAcceleratorEntry(wxACCEL_CTRL,  'X',         XRCID("MenuCut")),
      wxAcceleratorEntry(wxACCEL_CTRL,  'C',         XRCID("MenuCopy")),
      wxAcceleratorEntry(wxACCEL_CTRL,  'V',         XRCID("MenuPaste")),
      wxAcceleratorEntry(wxACCEL_NORMAL, WXK_DELETE, XRCID("MenuDelete"))
  };
  wxAcceleratorTable accel(PARRAYSIZE(accelEntries), accelEntries);
  SetAcceleratorTable(accel);

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);

  wxPoint initalPosition = wxDefaultPosition;
  if (config->Read(MainFrameXKey, &initalPosition.x) && config->Read(MainFrameYKey, &initalPosition.y))
    Move(initalPosition);

  wxSize initialSize(512, 384);
  if (config->Read(MainFrameWidthKey, &initialSize.x) && config->Read(MainFrameHeightKey, &initialSize.y))
    SetSize(initialSize);

  // Make the content of the main window, speed dial and log panes inside a splitter
  m_splitter = new wxSplitterWindow(this, SplitterID, wxPoint(), initialSize, wxSP_3D);

  // Log window - gets informative text
  initialSize.y /= 2;
  m_logWindow = new wxTextCtrl(m_splitter, -1, wxEmptyString, wxPoint(), initialSize, wxTE_MULTILINE | wxTE_DONTWRAP | wxSUNKEN_BORDER);
  m_logWindow->SetForegroundColour(wxColour(0,255,0)); // Green
  m_logWindow->SetBackgroundColour(wxColour(0,0,0)); // Black

  // Speed dial window - icons for each speed dial
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

  // Speed dial panel switches to answer panel on ring
  m_answerPanel = new AnswerPanel(*this, m_splitter);
  m_answerPanel->Show(false);

  // Speed dial panel switches to calling panel on dial
  m_callingPanel = new CallingPanel(*this, m_splitter);
  m_callingPanel->Show(false);

  // Speed dial/Answer/Calling panel switches to "in call" panel on successful call establishment
  m_inCallPanel = new InCallPanel(*this, m_splitter);
  m_inCallPanel->Show(false);

  // Set up sizer to automatically resize the splitter to size of window
  wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(m_splitter, 1, wxGROW, 0);
  SetSizer(sizer);

  // Show the frame window
  Show(PTrue);

  LogWindow << PProcess::Current().GetName()
            << " Version " << PProcess::Current().GetVersion(TRUE)
            << " by " << PProcess::Current().GetManufacturer()
            << " on " << PProcess::Current().GetOSClass() << ' ' << PProcess::Current().GetOSName()
            << " (" << PProcess::Current().GetOSVersion() << '-' << PProcess::Current().GetOSHardware() << ')' << endl;

  m_ClipboardFormat.SetId("OpenPhone Speed Dial");

#if PTRACING
  ////////////////////////////////////////
  // Tracing fields
  config->SetPath(TracingGroup);
  if (config->Read(EnableTracingKey, &m_enableTracing, false) && m_enableTracing &&
      config->Read(TraceFileNameKey, &m_traceFileName) && !m_traceFileName.empty()) {
    int traceLevelThreshold = 3;
    config->Read(TraceLevelThresholdKey, &traceLevelThreshold);
    int traceOptions = PTrace::DateAndTime|PTrace::Thread|PTrace::FileAndLine;
    config->Read(TraceOptionsKey, &traceOptions);
    PTrace::Initialise(traceLevelThreshold, m_traceFileName, traceOptions);
  }
#endif

  ////////////////////////////////////////
  // Creating the endpoints
  h323EP = new MyH323EndPoint(*this);

  sipEP = new MySIPEndPoint(*this);

#if P_EXPAT
  ivrEP = new OpalIVREndPoint(*this);
#endif

  potsEP = new OpalPOTSEndPoint(*this);
  pcssEP = new MyPCSSEndPoint(*this);

  PwxString str;
  bool onoff;
  int value1 = 0, value2 = 0;

  ////////////////////////////////////////
  // General fields
  config->SetPath(GeneralGroup);
  if (config->Read(UsernameKey, &str) && !str.IsEmpty())
    SetDefaultUserName(str);
  if (config->Read(DisplayNameKey, &str) && !str.IsEmpty())
    SetDefaultDisplayName(str);

  if (!config->Read(RingSoundDeviceNameKey, &m_RingSoundDeviceName))
    m_RingSoundDeviceName = PSoundChannel::GetDefaultDevice(PSoundChannel::Player);
  config->Read(RingSoundFileNameKey, &m_RingSoundFileName);

  config->Read(AutoAnswerKey, &m_autoAnswer);
#if P_EXPAT
  if (config->Read(IVRScriptKey, &str))
    ivrEP->SetDefaultVXML(str);
#endif

  ////////////////////////////////////////
  // Networking fields
  PIPSocket::InterfaceTable interfaceTable;
  if (PIPSocket::GetInterfaceTable(interfaceTable))
    LogWindow << "Detected " << interfaceTable.GetSize() << " network interfaces:\n"
              << setfill('\n') << interfaceTable << setfill(' ') << flush;

  config->SetPath(NetworkingGroup);
  if (config->Read(BandwidthKey, &value1))
    h323EP->SetInitialBandwidth(value1);
  if (config->Read(TCPPortBaseKey, &value1) && config->Read(TCPPortMaxKey, &value2))
    SetTCPPorts(value1, value2);
  if (config->Read(UDPPortBaseKey, &value1) && config->Read(UDPPortMaxKey, &value2))
    SetUDPPorts(value1, value2);
  if (config->Read(RTPPortBaseKey, &value1) && config->Read(RTPPortMaxKey, &value2))
    SetRtpIpPorts(value1, value2);
  if (config->Read(RTPTOSKey, &value1))
    SetRtpIpTypeofService(value1);
  config->Read(NATRouterKey, &m_NATRouter);
  config->Read(STUNServerKey, &m_STUNServer);
  if (!config->Read(NATHandlingKey, &m_NATHandling))
    m_NATHandling = m_STUNServer.IsEmpty() ? (m_NATRouter.IsEmpty() ? 0 : 1) : 2;

  SetNATHandling();

  config->SetPath(LocalInterfacesGroup);
  wxString entryName;
  long entryIndex;
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      wxString localInterface;
      if (config->Read(entryName, &localInterface) && !localInterface.empty())
        m_LocalInterfaces.AppendString(localInterface.c_str());
    } while (config->GetNextEntry(entryName, entryIndex));
  }

  StartAllListeners();

  ////////////////////////////////////////
  // Sound fields
  config->SetPath(AudioGroup);
  if (config->Read(SoundPlayerKey, &str))
    pcssEP->SetSoundChannelPlayDevice(str);
  if (config->Read(SoundRecorderKey, &str))
    pcssEP->SetSoundChannelRecordDevice(str);
  if (config->Read(SoundBuffersKey, &value1))
    pcssEP->SetSoundChannelBufferDepth(value1);

  if (config->Read(MinJitterKey, &value1)) {
    config->Read(MaxJitterKey, &value2, value1);
    SetAudioJitterDelay(value1, value2);
  }

  OpalSilenceDetector::Params silenceParams = GetSilenceDetectParams();
  if (config->Read(SilenceSuppressionKey, &value1) && value1 >= 0 && value1 < OpalSilenceDetector::NumModes)
    silenceParams.m_mode = (OpalSilenceDetector::Mode)value1;
  if (config->Read(SilenceThresholdKey, &value1))
    silenceParams.m_threshold = value1;
  if (config->Read(SignalDeadbandKey, &value1))
    silenceParams.m_signalDeadband = value1*8;
  if (config->Read(SilenceDeadbandKey, &value1))
    silenceParams.m_silenceDeadband = value1*8;
  SetSilenceDetectParams(silenceParams);

  StartLID();


  ////////////////////////////////////////
  // Video fields
  config->SetPath(VideoGroup);
  PVideoDevice::OpenArgs videoArgs = GetVideoInputDevice();
  if (config->Read(VideoGrabberKey, &str))
    videoArgs.deviceName = (PString)PString(str.c_str());
  if (config->Read(VideoGrabFormatKey, &value1) && value1 >= 0 && value1 < PVideoDevice::NumVideoFormats)
    videoArgs.videoFormat = (PVideoDevice::VideoFormat)value1;
  if (config->Read(VideoGrabSourceKey, &value1))
    videoArgs.channelNumber = value1;
  if (config->Read(VideoGrabFrameRateKey, &value1))
    videoArgs.rate = value1;
  config->Read(VideoFlipLocalKey, &videoArgs.flip);
  SetVideoInputDevice(videoArgs);

  config->Read(VideoGrabFrameSizeKey, &m_VideoGrabFrameSize,  "CIF");
  config->Read(VideoMinFrameSizeKey,  &m_VideoMinFrameSize, "SQCIF");
  config->Read(VideoMaxFrameSizeKey,  &m_VideoMaxFrameSize,   "CIF16");

  config->Read(VideoGrabPreviewKey, &m_VideoGrabPreview);
  if (config->Read(VideoAutoTransmitKey, &onoff))
    SetAutoStartTransmitVideo(onoff);
  if (config->Read(VideoAutoReceiveKey, &onoff))
    SetAutoStartReceiveVideo(onoff);

  videoArgs = GetVideoPreviewDevice();
#if USE_SDL
  videoArgs.driverName = "SDL";
#else
  videoArgs.driverName = "Window";
#endif
  SetVideoPreviewDevice(videoArgs);

  videoArgs = GetVideoOutputDevice();
#if USE_SDL
  videoArgs.driverName = "SDL";
#else
  videoArgs.driverName = "Window";
#endif
  config->Read(VideoFlipRemoteKey, &videoArgs.flip);
  SetVideoOutputDevice(videoArgs);

  config->Read(LocalVideoFrameXKey, &m_localVideoFrameX);
  config->Read(LocalVideoFrameYKey, &m_localVideoFrameY);
  config->Read(RemoteVideoFrameXKey, &m_remoteVideoFrameX);
  config->Read(RemoteVideoFrameYKey, &m_remoteVideoFrameY);

  ////////////////////////////////////////
  // Codec fields
  InitMediaInfo(pcssEP->GetPrefixName(), pcssEP->GetMediaFormats());
  InitMediaInfo(potsEP->GetPrefixName(), potsEP->GetMediaFormats());
#if P_EXPAT
  InitMediaInfo(ivrEP->GetPrefixName(), ivrEP->GetMediaFormats());
#endif

  OpalMediaFormatList mediaFormats;
  mediaFormats += pcssEP->GetMediaFormats();
  mediaFormats += potsEP->GetMediaFormats();
#if P_EXPAT
  mediaFormats += ivrEP->GetMediaFormats();
#endif
  InitMediaInfo("sw", OpalTranscoder::GetPossibleFormats(mediaFormats));

  config->SetPath(CodecsGroup);
  int codecIndex = 0;
  for (;;) {
    wxString groupName;
    groupName.sprintf("%04u", codecIndex);
    if (!config->HasGroup(groupName))
      break;

    config->SetPath(groupName);
    PwxString codecName;
    if (config->Read(CodecNameKey, &codecName) && !codecName.empty()) {
      for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
        if (codecName == mm->mediaFormat) {
          mm->preferenceOrder = codecIndex;
          bool changedSomething = false;
          for (PINDEX i = 0; i < mm->mediaFormat.GetOptionCount(); i++) {
            const OpalMediaOption & option = mm->mediaFormat.GetOption(i);
            if (!option.IsReadOnly()) {
              PwxString codecOptionName = option.GetName();
              PwxString codecOptionValue;
              PString oldOptionValue;
              mm->mediaFormat.GetOptionValue(codecOptionName, oldOptionValue);
              if (config->Read(codecOptionName, &codecOptionValue) &&
                              !codecOptionValue.empty() && codecOptionValue != oldOptionValue) {
                if (mm->mediaFormat.SetOptionValue(codecOptionName, codecOptionValue))
                  changedSomething = true;
              }
            }
          }
          if (changedSomething)
            OpalMediaFormat::SetRegisteredMediaFormat(mm->mediaFormat);
        }
      }
    }
    config->SetPath("..");
    codecIndex++;
  }

  if (codecIndex > 0)
    ApplyMediaInfo();
  else {
    PStringArray mediaFormatOrder = GetMediaFormatOrder();
    for (PINDEX i = 0; i < mediaFormatOrder.GetSize(); i++) {
      for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
        if (mm->mediaFormat == mediaFormatOrder[i])
          mm->preferenceOrder = codecIndex++;
      }
    }
    for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
      if (mm->preferenceOrder < 0)
        mm->preferenceOrder = codecIndex++;
    }
    m_mediaInfo.sort();
  }
  AdjustFrameSize();

#if PTRACING
  mediaFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
  ostream & traceStream = PTrace::Begin(3, __FILE__, __LINE__);
  traceStream << "OpenPhone\tRegistered media formats:\n";
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++)
    mediaFormats[i].PrintOptions(traceStream);
  traceStream << PTrace::End;
#endif

  ////////////////////////////////////////
  // H.323 fields
  config->SetPath(H323AliasesGroup);
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      wxString alias;
      if (config->Read(entryName, &alias) && !alias.empty())
        h323EP->AddAliasName(alias.c_str());
    } while (config->GetNextEntry(entryName, entryIndex));
  }

  config->SetPath(H323Group);
  if (config->Read(DTMFSendModeKey, &value1) && value1 >= 0 && value1 < H323Connection::NumSendUserInputModes)
    h323EP->SetSendUserInputMode((H323Connection::SendUserInputModes)value1);
  if (config->Read(CallIntrusionProtectionLevelKey, &value1))
    h323EP->SetCallIntrusionProtectionLevel(value1);
  if (config->Read(DisableFastStartKey, &onoff))
    h323EP->DisableFastStart(onoff);
  if (config->Read(DisableH245TunnelingKey, &onoff))
    h323EP->DisableH245Tunneling(onoff);
  if (config->Read(DisableH245inSETUPKey, &onoff))
    h323EP->DisableH245inSetup(onoff);

  PwxString username, password;
  config->Read(GatekeeperModeKey, &m_gatekeeperMode, 0);
  if (m_gatekeeperMode > 0) {
    if (config->Read(GatekeeperTTLKey, &value1))
      h323EP->SetGatekeeperTimeToLive(PTimeInterval(0, value1));

    config->Read(GatekeeperLoginKey, &username, "");
    config->Read(GatekeeperPasswordKey, &password, "");
    h323EP->SetGatekeeperPassword(password, username);

    config->Read(GatekeeperAddressKey, &m_gatekeeperAddress, "");
    config->Read(GatekeeperIdentifierKey, &m_gatekeeperIdentifier, "");
    if (!StartGatekeeper())
      return false;
  }

  ////////////////////////////////////////
  // SIP fields
  config->SetPath(SIPGroup);
  const SIPURL & proxy = sipEP->GetProxy();
  PwxString hostname;
  config->Read(SIPProxyUsedKey, &m_SIPProxyUsed, false);
  config->Read(SIPProxyKey, &hostname, PwxString(proxy.GetHostName()));
  config->Read(SIPProxyUsernameKey, &username, PwxString(proxy.GetUserName()));
  config->Read(SIPProxyPasswordKey, &password, PwxString(proxy.GetPassword()));
  if (m_SIPProxyUsed)
    sipEP->SetProxy(hostname, username, password);

  if (config->Read(RegistrarTimeToLiveKey, &value1))
    sipEP->SetRegistrarTimeToLive(PTimeInterval(0, value1));

  // Original backward compatibility entry
  RegistrarInfo registrar;
  if (config->Read(RegistrarUsedKey, &registrar.m_Active, false) &&
      config->Read(RegistrarNameKey, &registrar.m_Domain) &&
      config->Read(RegistrarUsernameKey, &registrar.m_User) &&
      config->Read(RegistrarPasswordKey, &registrar.m_Password))
    m_registrars.push_back(registrar);

  config->SetPath(RegistrarGroup);
  wxString groupName;
  long groupIndex;
  if (config->GetFirstGroup(groupName, groupIndex)) {
    do {
      config->SetPath(groupName);
      if (config->Read(RegistrarUsedKey, &registrar.m_Active, false) &&
          config->Read(RegistrarDomainKey, &registrar.m_Domain) &&
          config->Read(RegistrarUsernameKey, &registrar.m_User) &&
          config->Read(RegistrarPasswordKey, &registrar.m_Password) &&
          config->Read(RegistrarTimeToLiveKey, &registrar.m_TimeToLive))
        m_registrars.push_back(registrar);
      config->SetPath("..");
    } while (config->GetNextGroup(groupName, groupIndex));
  }

  StartRegistrars();


  ////////////////////////////////////////
  // Routing fields
  config->SetPath(RoutingGroup);
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      wxString routeSpec;
      if (config->Read(entryName, &routeSpec))
        AddRouteEntry(routeSpec.c_str());
    } while (config->GetNextEntry(entryName, entryIndex));
  }
  else {
#if P_EXPAT
    AddRouteEntry(".*:#  = ivr:"); // A hash from anywhere goes to IVR
#endif
    AddRouteEntry("pots:.*\\*.*\\*.* = sip:<dn2ip>");
    AddRouteEntry("pots:.*\\*.*\\*.* = sips:<dn2ip>");
    AddRouteEntry("pots:.*           = sip:<da>");
    AddRouteEntry("pots:.*           = sips:<da>");

    AddRouteEntry("pc:.*             = sip:<da>");
    AddRouteEntry("pc:.*             = sips:<da>");
    AddRouteEntry("pc:.*             = h323:<da>");
    AddRouteEntry("pc:.*             = h323s:<da>");

    AddRouteEntry("h323:.*           = pots:<dn>");
    AddRouteEntry("h323:.*           = pc:<du>");

    AddRouteEntry("h323s:.*          = pots:<dn>");
    AddRouteEntry("h323s:.*          = pc:<du>");

    AddRouteEntry("sip:.*            = pots:<dn>");
    AddRouteEntry("sip:.*            = pc:<du>");

    AddRouteEntry("sips:.*           = pots:<dn>");
    AddRouteEntry("sips:.*           = pc:<du>");
  }

  return true;
}


void MyManager::StartLID()
{
  wxConfigBase * config = wxConfig::Get();

  PwxString device;
  if (!config->Read(LineInterfaceDeviceKey, &device) ||
      device.IsEmpty() || (device.StartsWith("<<") && device.EndsWith(">>")))
    return;

  if (!potsEP->AddDeviceName(device)) {
    LogWindow << "Line Interface Device \"" << device << "\" has been unplugged!" << endl;
    return;
  }

  OpalLine * line = potsEP->GetLine("*");
  if (PAssertNULL(line) == NULL)
    return;

  int aec;
  if (config->Read(AECKey, &aec) && aec >= 0 && aec < OpalLineInterfaceDevice::AECError)
    line->SetAEC((OpalLineInterfaceDevice::AECLevels)aec);

  PwxString country;
  if (config->Read(CountryKey, &country) && !country.IsEmpty()) {
    if (line->GetDevice().SetCountryCodeName(country))
      LogWindow << "Using Line Interface Device \"" << line->GetDevice().GetDescription() << '"' << endl;
    else
      LogWindow << "Could not configure Line Interface Device to country \"" << country << '"' << endl;
  }
}


void MyManager::SetNATHandling()
{
  switch (m_NATHandling) {
    case 1 :
      if (!m_NATRouter.IsEmpty())
        SetTranslationHost(m_NATRouter);
      SetSTUNServer(PString::Empty());
      break;
      
    case 2 :
      if (!m_STUNServer.IsEmpty()) {
        LogWindow << "STUN server \"" << m_STUNServer << "\" being contacted ..." << endl;
        GetEventHandler()->ProcessPendingEvents();
        Update();

        PSTUNClient::NatTypes nat = SetSTUNServer(m_STUNServer);

        LogWindow << "STUN server \"" << stun->GetServer() << "\" replies " << nat;
        PIPSocket::Address externalAddress;
        if (nat != PSTUNClient::BlockedNat && GetSTUN()->GetExternalAddress(externalAddress))
          LogWindow << " with address " << externalAddress;
        LogWindow << endl;
      }
      SetTranslationHost(PString::Empty());
      break;

    default :
      SetTranslationHost(PString::Empty());
      SetSTUNServer(PString::Empty());
  }
}


static void StartListenerForEP(OpalEndPoint * ep, const PStringArray & allInterfaces)
{
  if (ep == NULL)
    return;

  PStringArray interfacesForEP;
  PString prefixAndColon = ep->GetPrefixName() + ':';

  for (PINDEX i = 0; i < allInterfaces.GetSize(); i++) {
    PCaselessString iface = allInterfaces[i];
    if (iface.NumCompare("all:", 4) == PObject::EqualTo)
      interfacesForEP += iface.Mid(4);
    else if (iface.NumCompare(prefixAndColon) == PObject::EqualTo)
      interfacesForEP.AppendString(iface.Mid(prefixAndColon.GetLength()));
  }

  ep->RemoveListener(NULL);
  if (ep->StartListeners(interfacesForEP))
    LogWindow << ep->GetPrefixName().ToUpper() << " listening on " << setfill(',') << ep->GetListeners() << setfill(' ') << endl;
  else {
    LogWindow << ep->GetPrefixName().ToUpper() << " listen failed";
    if (!interfacesForEP.IsEmpty())
      LogWindow << " with interfaces" << setfill(',') << interfacesForEP << setfill(' ');
    LogWindow << endl;
  }
}


void MyManager::StartAllListeners()
{
  StartListenerForEP(h323EP, m_LocalInterfaces);
  StartListenerForEP(sipEP, m_LocalInterfaces);
}


void MyManager::RecreateSpeedDials(SpeedDialViews view)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);

  config->Write(ActiveViewKey, view);

  wxListCtrl * oldSpeedDials = m_speedDials;

  static DWORD const ListCtrlStyle[e_NumViews] = {
    wxLC_ICON, wxLC_SMALL_ICON, wxLC_LIST, wxLC_REPORT
  };

  m_speedDials = new wxListCtrl(m_splitter, SpeedDialsID,
                               wxDefaultPosition, wxDefaultSize,
                               ListCtrlStyle[view] | wxLC_EDIT_LABELS | wxSUNKEN_BORDER);

  if (view != e_ViewDetails) {
    m_speedDials->SetImageList(m_imageListNormal, wxIMAGE_LIST_NORMAL);
    m_speedDials->SetImageList(m_imageListSmall, wxIMAGE_LIST_SMALL);
  }

  int width;
  static const char * const titles[e_NumColumns] = { "Name", "Number", "Address", "Description" };

  for (int i = 0; i < e_NumColumns; i++) {
    m_speedDials->InsertColumn(i, titles[i]);
    wxString key;
    key.sprintf("ColumnWidth%u", i);
    if (config->Read(key, &width))
      m_speedDials->SetColumnWidth(i, width);
  }

  // Now either replace the top half of the splitter or set it for the first time
  if (oldSpeedDials == NULL) {
    width = 0;
    config->Read(SashPositionKey, &width);
    m_splitter->SplitHorizontally(m_speedDials, m_logWindow, width);
  }
  else {
    m_splitter->ReplaceWindow(oldSpeedDials, m_speedDials);
    delete oldSpeedDials;
  }

  // Read the speed dials from the configuration
  config->SetPath(SpeedDialsGroup);
  wxString groupName;
  long groupIndex;
  if (config->GetFirstGroup(groupName, groupIndex)) {
    do {
      config->SetPath(groupName);
      wxString number, address, description;
      if (config->Read(SpeedDialAddressKey, &address) && !address.empty()) {
        int icon = 0;
        if (view == e_ViewLarge) {
          if (address.StartsWith("h323"))
            icon = 1;
          else if (address.StartsWith("sip"))
            icon = 2;
        }

        int pos = m_speedDials->InsertItem(INT_MAX, groupName, icon);
        m_speedDials->SetItem(pos, e_NumberColumn, config->Read(SpeedDialNumberKey, ""));
        m_speedDials->SetItem(pos, e_AddressColumn, address);
        m_speedDials->SetItem(pos, e_DescriptionColumn, config->Read(SpeedDialDescriptionKey, ""));
      }
      config->SetPath("..");
    } while (config->GetNextGroup(groupName, groupIndex));
  }
}


void MyManager::OnClose(wxCloseEvent& /*event*/)
{
  ::wxBeginBusyCursor();

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);

  int x, y;
  GetPosition(&x, &y);
  config->Write(MainFrameXKey, x);
  config->Write(MainFrameYKey, y);

  int w, h;
  GetSize(&w, &h);
  config->Write(MainFrameWidthKey, w);
  config->Write(MainFrameHeightKey, h);

  ShutDownEndpoints();

  Destroy();
}


void MyManager::OnLogMessage(wxCommandEvent & theEvent)
{
    m_logWindow->WriteText(theEvent.GetString());
}


void MyManager::OnAdjustMenus(wxMenuEvent& WXUNUSED(event))
{
  wxMenuBar * menubar = GetMenuBar();
  menubar->Enable(XRCID("MenuCall"),    m_callState == IdleState);
  menubar->Enable(XRCID("MenuCallLastDialed"), m_callState == IdleState && !m_LastDialed.IsEmpty());
  menubar->Enable(XRCID("MenuCallLastReceived"), m_callState == IdleState && !m_LastReceived.IsEmpty());
  menubar->Enable(XRCID("MenuAnswer"),  m_callState == RingingState);
  menubar->Enable(XRCID("MenuHangUp"),  m_callState == InCallState);

  int count = m_speedDials->GetSelectedItemCount();
  menubar->Enable(XRCID("MenuCut"),       count >= 1);
  menubar->Enable(XRCID("MenuCopy"),      count >= 1);
  menubar->Enable(XRCID("MenuDelete"),    count >= 1);
  menubar->Enable(XRCID("EditSpeedDial"), count == 1);

  bool hasFormat = false;
  if (wxTheClipboard->Open()) {
    hasFormat = wxTheClipboard->IsSupported(m_ClipboardFormat);
    wxTheClipboard->Close();
  }
  menubar->Enable(XRCID("MenuPaste"), hasFormat);
}


void MyManager::OnMenuQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(PTrue);
}


void MyManager::OnMenuAbout(wxCommandEvent& WXUNUSED(event))
{
  wxMessageDialog dialog(this,
                         _T("OpenPhone\nPost Increment (c) 2004"),
                         _T("About OpenPhone"),
                         wxOK);
  dialog.ShowModal();
}


void MyManager::OnMenuCall(wxCommandEvent& WXUNUSED(event))
{
  CallDialog dlg(this);
  if (dlg.ShowModal() == wxID_OK)
    MakeCall(dlg.m_Address);
}


void MyManager::OnMenuCallLastDialed(wxCommandEvent& WXUNUSED(event))
{
  MakeCall(m_LastDialed);
}


void MyManager::OnMenuCallLastReceived(wxCommandEvent& WXUNUSED(event))
{
  MakeCall(m_LastReceived);
}


void MyManager::OnMenuAnswer(wxCommandEvent& WXUNUSED(event))
{
  AnswerCall();
}


void MyManager::OnMenuHangUp(wxCommandEvent& WXUNUSED(event))
{
  HangUpCall();
}


static wxString MakeUniqueSpeedDialName(wxListCtrl * speedDials, const char * baseName)
{
  wxString name = baseName;
  unsigned tieBreaker = 0;
  int count = speedDials->GetItemCount();
  int i = 0;
  while (i < count) {
    if (speedDials->GetItemText(i).CmpNoCase(name) != 0)
      i++;
    else {
      name.Printf("%s (%u)", baseName, ++tieBreaker);
      i = 0;
    }
  }
  return name;
}


void MyManager::OnNewSpeedDial(wxCommandEvent& WXUNUSED(event))
{
  wxString groupName = MakeUniqueSpeedDialName(m_speedDials, "New Speed Dial");

  int pos = m_speedDials->InsertItem(INT_MAX, groupName);
  m_speedDials->SetItem(pos, e_NumberColumn, "");
  m_speedDials->SetItem(pos, e_AddressColumn, "");
  m_speedDials->SetItem(pos, e_DescriptionColumn, "");
  EditSpeedDial(pos, true);
}


void MyManager::OnViewLarge(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewLarge);
}


void MyManager::OnViewSmall(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewSmall);
}


void MyManager::OnViewList(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewList);
}


void MyManager::OnViewDetails(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewDetails);
}


void MyManager::OnEditSpeedDial(wxCommandEvent& WXUNUSED(event))
{
  EditSpeedDial(m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED), false);
}


void MyManager::OnCutSpeedDial(wxCommandEvent& event)
{
  OnCopySpeedDial(event);
  OnDeleteSpeedDial(event);
}


void MyManager::OnCopySpeedDial(wxCommandEvent& WXUNUSED(event))
{
  if (!wxTheClipboard->Open())
    return;

  wxString tabbedText;

  wxListItem item;
  item.m_itemId = -1;
  while ((item.m_itemId = m_speedDials->GetNextItem(item.m_itemId, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) >= 0) {
    item.m_mask = wxLIST_MASK_TEXT;
    item.m_col = e_NameColumn;
    if (m_speedDials->GetItem(item)) {
      tabbedText += item.m_text;
      tabbedText += '\t';

      item.m_col = e_NumberColumn;
      if (m_speedDials->GetItem(item))
        tabbedText += item.m_text;
      tabbedText += '\t';

      item.m_col = e_AddressColumn;
      if (m_speedDials->GetItem(item))
        tabbedText += item.m_text;
      tabbedText += '\t';

      item.m_col = e_DescriptionColumn;
      if (m_speedDials->GetItem(item))
        tabbedText += item.m_text;
      tabbedText += "\r\n";
    }
  }

  // Want pure text so can copy to notepad or something, and our built
  // in format, even though the latter is exactly the same string. This
  // just guarantees the format of teh string, where just using CF_TEXT
  // coupld provide anything.
  wxDataObjectComposite * multiFormatData = new wxDataObjectComposite;
  wxTextDataObject * myFormatData = new wxTextDataObject(tabbedText);
  myFormatData->SetFormat(m_ClipboardFormat);
  multiFormatData->Add(myFormatData);
  multiFormatData->Add(new wxTextDataObject(tabbedText));
  wxTheClipboard->SetData(multiFormatData);
  wxTheClipboard->Close();
}


void MyManager::OnPasteSpeedDial(wxCommandEvent& WXUNUSED(event))
{
  if (wxTheClipboard->Open()) {
    if (wxTheClipboard->IsSupported(m_ClipboardFormat)) {
      wxTextDataObject myFormatData;
      myFormatData.SetFormat(m_ClipboardFormat);
      if (wxTheClipboard->GetData(myFormatData)) {
        wxConfigBase * config = wxConfig::Get();
        wxStringTokenizer tabbedLines(myFormatData.GetText(), "\r\n");
        while (tabbedLines.HasMoreTokens()) {
          wxStringTokenizer tabbedText(tabbedLines.GetNextToken(), "\t", wxTOKEN_RET_EMPTY_ALL);
          wxString name = MakeUniqueSpeedDialName(m_speedDials, tabbedText.GetNextToken());
          wxString number = tabbedText.GetNextToken();
          wxString address = tabbedText.GetNextToken();
          wxString description = tabbedText.GetNextToken();

          int pos = m_speedDials->InsertItem(INT_MAX, name);
          m_speedDials->SetItem(pos, e_NumberColumn, number);
          m_speedDials->SetItem(pos, e_AddressColumn, address);
          m_speedDials->SetItem(pos, e_DescriptionColumn, description);

          config->SetPath(SpeedDialsGroup);
          config->SetPath(name);
          config->Write(SpeedDialNumberKey, number);
          config->Write(SpeedDialAddressKey, address);
          config->Write(SpeedDialDescriptionKey, description);
        }
      }
    }
    wxTheClipboard->Close();
  }
}


void MyManager::OnDeleteSpeedDial(wxCommandEvent& WXUNUSED(event))
{
  int count = m_speedDials->GetSelectedItemCount();
  if (count == 0)
    return;

  wxString str;
  str.Printf("Delete %u item%s?", count, count != 1 ? "s" : "");
  wxMessageDialog dlg(this, str, "OpenPhone Speed Dials", wxYES_NO);
  if (dlg.ShowModal() != wxID_YES)
    return;

  wxListItem item;
  while ((item.m_itemId = m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) >= 0) {
    item.m_mask = wxLIST_MASK_TEXT;
    item.m_col = e_NameColumn;
    if (m_speedDials->GetItem(item)) {
      wxConfigBase * config = wxConfig::Get();
      config->SetPath(SpeedDialsGroup);
      config->DeleteGroup(item.m_text);
      m_speedDials->DeleteItem(item.m_itemId);
    }
  }
}


void MyManager::OnSashPositioned(wxSplitterEvent& event)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);
  config->Write(SashPositionKey, event.GetSashPosition());
}


void MyManager::OnSpeedDialActivated(wxListEvent& event)
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


void MyManager::OnSpeedDialColumnResize(wxListEvent& event)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);
  wxString key;
  key.sprintf("ColumnWidth%u", event.GetColumn());
  config->Write(key, m_speedDials->GetColumnWidth(event.GetColumn()));
}


void MyManager::OnRightClick(wxListEvent& event)
{
  wxMenuBar * menuBar = wxXmlResource::Get()->LoadMenuBar("PopUpMenu");
  PopupMenu(menuBar->GetMenu(0), event.GetPoint());
  delete menuBar;
}


void MyManager::EditSpeedDial(int index, bool newItem)
{
  if (index < 0)
    return;

  wxListItem item;
  item.m_itemId = index;
  item.m_mask = wxLIST_MASK_TEXT;

  item.m_col = e_NameColumn;
  if (!m_speedDials->GetItem(item))
    return;

  // Should display a menu, but initially just allow editing
  SpeedDialDialog dlg(this);

  wxString originalName = dlg.m_Name = item.m_text;

  item.m_col = e_NumberColumn;
  if (m_speedDials->GetItem(item))
    dlg.m_Number = item.m_text;

  item.m_col = e_AddressColumn;
  if (m_speedDials->GetItem(item))
    dlg.m_Address = item.m_text;

  item.m_col = e_DescriptionColumn;
  if (m_speedDials->GetItem(item))
    dlg.m_Description = item.m_text;

  if (dlg.ShowModal() == wxID_CANCEL) {
    if (newItem)
      m_speedDials->DeleteItem(item);
    return;
  }

  item.m_col = e_NameColumn;
  item.m_text = dlg.m_Name;
  m_speedDials->SetItem(item);

  item.m_col = e_NumberColumn;
  item.m_text = dlg.m_Number;
  m_speedDials->SetItem(item);

  item.m_col = e_AddressColumn;
  item.m_text = dlg.m_Address;
  m_speedDials->SetItem(item);

  item.m_col = e_DescriptionColumn;
  item.m_text = dlg.m_Description;
  m_speedDials->SetItem(item);

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(SpeedDialsGroup);
  config->DeleteGroup(originalName);
  config->SetPath(dlg.m_Name);
  config->Write(SpeedDialNumberKey, dlg.m_Number);
  config->Write(SpeedDialAddressKey, dlg.m_Address);
  config->Write(SpeedDialDescriptionKey, dlg.m_Description);
}


bool MyManager::HasSpeedDialName(const wxString & name) const
{
  return m_speedDials->FindItem(-1, name) >= 0;
}


int MyManager::GetSpeedDialIndex(const char * number, const char * ignore) const
{
  int count = m_speedDials->GetItemCount();
  wxListItem item;
  item.m_mask = wxLIST_MASK_TEXT;
  item.m_col = e_NumberColumn;
  for (item.m_itemId = 0; item.m_itemId < count; item.m_itemId++) {
    if (m_speedDials->GetItem(item)) {
      int len = item.m_text.Length();
      if (len > 0 && (ignore == NULL || strcmp(ignore, item.m_text) != 0) && strncmp(number, item.m_text, len) == 0)
        return item.m_itemId;
    }
  }
  return -1;
}


void MyManager::MakeCall(const PwxString & address)
{
  if (address.IsEmpty())
    return;

  SetState(CallingState);

  LogWindow << "Calling \"" << address << '"' << endl;

  m_LastDialed = address;
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(GeneralGroup);
  config->Write(LastDialedKey, m_LastDialed);

  if (potsEP != NULL && potsEP->GetLine("*") != NULL)
    SetUpCall("pots:*", address, m_currentCallToken);
  else
    SetUpCall("pc:*", address, m_currentCallToken);
}


void MyManager::AnswerCall()
{
  if (m_callState != RingingState)
    return;

  StopRingSound();
  SetState(AnsweringState);
  pcssEP->AcceptIncomingConnection(m_ringingConnectionToken);
}


void MyManager::RejectCall()
{
  if (m_callState != RingingState)
    return;

  ClearCall(m_currentCallToken);
  SetState(IdleState);
}


void MyManager::HangUpCall()
{
  if (m_callState == IdleState)
    return;

  LogWindow << "Hanging up \"" << m_currentCallToken << '"' << endl;
  ClearCall(m_currentCallToken);
  SetState(IdleState);
}


void MyManager::OnRinging(const OpalPCSSConnection & connection)
{
  m_ringingConnectionToken = connection.GetToken();
  m_currentCallToken = connection.GetCall().GetToken();

  PTime now;
  LogWindow << "\nIncoming call at " << now.AsString("w h:mma")
            << " from " << connection.GetRemotePartyName() << endl;

  m_LastReceived = connection.GetRemotePartyAddress();
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(GeneralGroup);
  config->Write(LastReceivedKey, m_LastDialed);

  if (!m_autoAnswer && !m_RingSoundFileName.empty()) {
    m_RingSoundChannel.Open(m_RingSoundDeviceName, PSoundChannel::Player);
    m_RingSoundChannel.PlayFile(m_RingSoundFileName.c_str(), PFalse);
    m_RingSoundTimer.RunContinuous(5000);
  }

  SetState(RingingState);
}


void MyManager::OnRingSoundAgain(PTimer &, INT)
{
  m_RingSoundChannel.PlayFile(m_RingSoundFileName.c_str(), PFalse);
}


void MyManager::StopRingSound()
{
  m_RingSoundTimer.Stop();
  m_RingSoundChannel.Close();
}


PBoolean MyManager::OnIncomingConnection(OpalConnection & connection)
{
  if (connection.GetEndPoint().GetPrefixName() == "pots")
    LogWindow << "Line interface device \"" << connection.GetRemotePartyName() << "\" has gone off hook." << endl;

  return OpalManager::OnIncomingConnection(connection);
}


void MyManager::OnEstablishedCall(OpalCall & call)
{
  m_currentCallToken = call.GetToken();
  LogWindow << "Established call from " << call.GetPartyA() << " to " << call.GetPartyB() << endl;
  SetState(InCallState);
}


void MyManager::OnClearedCall(OpalCall & call)
{
  StopRingSound();

  PString name = call.GetPartyB().IsEmpty() ? call.GetPartyA() : call.GetPartyB();

  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByRemoteUser :
      LogWindow << '"' << name << "\" has cleared the call";
      break;
    case OpalConnection::EndedByCallerAbort :
      LogWindow << '"' << name << "\" has stopped calling";
      break;
    case OpalConnection::EndedByRefusal :
      LogWindow << '"' << name << "\" did not accept your call";
      break;
    case OpalConnection::EndedByNoAnswer :
      LogWindow << '"' << name << "\" did not answer your call";
      break;
    case OpalConnection::EndedByTransportFail :
      LogWindow << "Call with \"" << name << "\" ended abnormally";
      break;
    case OpalConnection::EndedByCapabilityExchange :
      LogWindow << "Could not find common codec with \"" << name << '"';
      break;
    case OpalConnection::EndedByNoAccept :
      LogWindow << "Did not accept incoming call from \"" << name << '"';
      break;
    case OpalConnection::EndedByAnswerDenied :
      LogWindow << "Refused incoming call from \"" << name << '"';
      break;
    case OpalConnection::EndedByNoUser :
      LogWindow << "Gatekeeper could find user \"" << name << '"';
      break;
    case OpalConnection::EndedByNoBandwidth :
      LogWindow << "Call to \"" << name << "\" aborted, insufficient bandwidth.";
      break;
    case OpalConnection::EndedByUnreachable :
      LogWindow << '"' << name << "\" could not be reached.";
      break;
    case OpalConnection::EndedByNoEndPoint :
      LogWindow << "No phone running for \"" << name << '"';
      break;
    case OpalConnection::EndedByHostOffline :
      LogWindow << '"' << name << "\" is not online.";
      break;
    case OpalConnection::EndedByConnectFail :
      LogWindow << "Transport error calling \"" << name << '"';
      break;
    default :
      LogWindow << "Call with \"" << name << "\" completed";
  }
  PTime now;
  LogWindow << ", on " << now.AsString("w h:mma") << ". Duration "
            << setprecision(0) << setw(5) << (now - call.GetStartTime())
            << "s." << endl;

  SetState(IdleState);
}


static void LogMediaStream(const char * stopStart, const OpalMediaStream & stream, const PString & epPrefix)
{
  if (epPrefix == "pc" || epPrefix == "pots")
    return;

  OpalMediaFormat mediaFormat = stream.GetMediaFormat();
  LogWindow << stopStart << (stream.IsSource() ? " receiving " : " sending ") << mediaFormat;

  if (!stream.IsSource() && mediaFormat.GetDefaultSessionID() == OpalMediaFormat::DefaultAudioSessionID)
    LogWindow << " (" << mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption())*mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits() << "ms)";

  LogWindow << (stream.IsSource() ? " from " : " to ")
            << epPrefix << " endpoint"
            << endl;
}


PBoolean MyManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return false;

  LogMediaStream("Started", stream, connection.GetEndPoint().GetPrefixName());

  wxCommandEvent theEvent(wxEvtUpdateStreams, ID_UPDATE_STREAMS);
  theEvent.SetEventObject(this);
  GetEventHandler()->AddPendingEvent(theEvent);

  return true;
}


void MyManager::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalManager::OnClosedMediaStream(stream);

  LogMediaStream("Stopped", stream, stream.GetConnection().GetEndPoint().GetPrefixName());

  wxCommandEvent theEvent(wxEvtUpdateStreams, ID_UPDATE_STREAMS);
  theEvent.SetEventObject(this);
  GetEventHandler()->AddPendingEvent(theEvent);

  if (PIsDescendant(&stream, OpalVideoMediaStream)) {
    PVideoOutputDevice * device = ((const OpalVideoMediaStream &)stream).GetVideoOutputDevice();
    if (device != NULL) {
      int x, y;
      if (device->GetPosition(x, y)) {
        wxConfigBase * config = wxConfig::Get();
        config->SetPath(VideoGroup);

        if (stream.IsSource()) {
          if (x != m_localVideoFrameX || y != m_localVideoFrameY) {
            config->Write(LocalVideoFrameXKey, m_localVideoFrameX = x);
            config->Write(LocalVideoFrameYKey, m_localVideoFrameY = y);
          }
        }
        else {
          if (x != m_remoteVideoFrameX || y != m_remoteVideoFrameY) {
            config->Write(RemoteVideoFrameXKey, m_remoteVideoFrameX = x);
            config->Write(RemoteVideoFrameYKey, m_remoteVideoFrameY = y);
          }
        }
      }
    }
  }
}


PSafePtr<OpalConnection> MyManager::GetUserConnection()
{
  PSafePtr<OpalCall> call = GetCall();
  if (call == NULL)
    return NULL;

  for (int i = 0; ; i++) {
    PSafePtr<OpalConnection> connection = call->GetConnection(i);
    if (connection == NULL)
      return NULL;

    if (PIsDescendant(&(*connection), OpalPCSSConnection) || PIsDescendant(&(*connection), OpalLineConnection))
      return connection;
  }

  return NULL;
}


void MyManager::SendUserInput(char tone)
{
  PSafePtr<OpalConnection> connection = GetUserConnection();
  if (connection != NULL)
    connection->OnUserInputTone(tone, 100);
}


void MyManager::OnUserInputString(OpalConnection & connection, const PString & value)
{
  LogWindow << "User input \"" << value << "\" received from \"" << connection.GetRemotePartyName() << '"' << endl;
  OpalManager::OnUserInputString(connection, value);
}


PString MyManager::ReadUserInput(OpalConnection & connection,
                                 const char * terminators,
                                 unsigned,
                                 unsigned firstDigitTimeout)
{
  // The usual behaviour is to read until a '#' or timeout and that yields the
  // entire destination address. However for this application we want to disable
  // the timeout and short circuit the need for '#' as the speed dial number is
  // always unique.

  PTRACE(3, "OpalPhone\tReadUserInput from " << connection);

  connection.PromptUserInput(PTrue);
  PString digit = connection.GetUserInput(firstDigitTimeout);
  connection.PromptUserInput(PFalse);

  PString input;
  while (!digit.IsEmpty()) {
    input += digit;

    int index = GetSpeedDialIndex(input, NULL);
    if (index >= 0) {
      wxListItem nameItem;
      nameItem.m_itemId = index;
      nameItem.m_mask = wxLIST_MASK_TEXT;
      nameItem.m_col = e_NameColumn;
      wxListItem addressItem = nameItem;
      addressItem.m_col = e_AddressColumn;
      if (m_speedDials->GetItem(nameItem) && m_speedDials->GetItem(addressItem)) {
        LogWindow << "Calling \"" << nameItem.m_text << "\" using \"" << addressItem.m_text << '"' << endl;
        return addressItem.m_text.c_str();
      }
    }

    digit = connection.GetUserInput(firstDigitTimeout);
  }

  PTRACE(2, "OpalPhone\tReadUserInput timeout (" << firstDigitTimeout << "ms) on " << *this);
  return PString::Empty();
}


static const PVideoDevice::OpenArgs & AdjustVideoArgs(PVideoDevice::OpenArgs & videoArgs, const char * title, int x, int y)
{
#if USE_SDL
  videoArgs.deviceName = "SDL";
#else
  videoArgs.deviceName = psprintf("MSWIN STYLE=0x%08X", WS_POPUP|WS_BORDER|WS_SYSMENU|WS_CAPTION);
#endif

  videoArgs.deviceName.sprintf(" TITLE=\"%s\" X=%i Y=%i", title, x, y);

  return videoArgs;
}

PBoolean MyManager::CreateVideoOutputDevice(const OpalConnection & connection,
                                        const OpalMediaFormat & mediaFormat,
                                        PBoolean preview,
                                        PVideoOutputDevice * & device,
                                        PBoolean & autoDelete)
{
  if (preview && !m_VideoGrabPreview)
    return PFalse;

  if (m_localVideoFrameX == INT_MIN) {
    wxRect rect(GetPosition(), GetSize());
    m_localVideoFrameX = rect.GetLeft() + mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
    m_localVideoFrameY = rect.GetBottom();
    m_remoteVideoFrameX = rect.GetLeft();
    m_remoteVideoFrameY = rect.GetBottom();
  }

  PVideoDevice::OpenArgs videoArgs;
  if (preview)
    SetVideoPreviewDevice(AdjustVideoArgs(videoArgs = GetVideoPreviewDevice(), "Local", m_localVideoFrameX, m_localVideoFrameY));
  else
    SetVideoOutputDevice(AdjustVideoArgs(videoArgs = GetVideoOutputDevice(), "Remote", m_remoteVideoFrameX, m_remoteVideoFrameY));

  return OpalManager::CreateVideoOutputDevice(connection, mediaFormat, preview, device, autoDelete);
}


ostream & operator<<(ostream & strm, MyManager::CallState state)
{
  static const char * const names[] = {
    "Idle", "Calling", "Ringing", "Answering", "In Call"
  };
  return strm << names[state];
}


void MyManager::SetState(CallState newState)
{
  wxCommandEvent theEvent(wxEvtStateChange, ID_STATE_CHANGE);
  theEvent.SetEventObject(this);
  theEvent.SetInt(newState);
  GetEventHandler()->AddPendingEvent(theEvent);
}


void MyManager::OnStateChange(wxCommandEvent & event)
{
  CallState newState = (CallState)event.GetInt();

  if (m_callState == newState)
    return;

  PTRACE(3, "OpenPhone\tGUI state changed from " << m_callState << " to " << newState);
  m_callState = newState;

  wxWindow * newWindow;
  switch (m_callState) {
    case RingingState :
      Raise();
      RequestUserAttention();

      if (!m_autoAnswer) {
        newWindow = m_answerPanel;
        break;
      }

      m_callState = AnsweringState;
      pcssEP->AcceptIncomingConnection(m_ringingConnectionToken);
      // Do next state

    case AnsweringState :
    case CallingState :
      newWindow = m_callingPanel;
      break;

    case InCallState :
      newWindow = m_inCallPanel;
      break;


    default :
      newWindow = m_speedDials;
  }

  m_speedDials->Hide();
  m_answerPanel->Hide();
  m_callingPanel->Hide();
  m_inCallPanel->Hide();
  newWindow->Show();

  m_splitter->ReplaceWindow(m_splitter->GetWindow1(), newWindow);
}


void MyManager::UpdateStreams(wxCommandEvent &)
{
  m_inCallPanel->UpdateButtons(potsEP);
}


bool MyManager::StartGatekeeper()
{
  if (m_gatekeeperMode == 0)
    h323EP->RemoveGatekeeper();
  else {
    PString gkDesc = m_gatekeeperIdentifier;
    if (!m_gatekeeperIdentifier.IsEmpty() || !m_gatekeeperAddress.IsEmpty())
      gkDesc == '@';
    gkDesc += (const char *)m_gatekeeperAddress;
    LogWindow << "H.323 registration started for " << gkDesc << endl;
    GetEventHandler()->ProcessPendingEvents();
    Update();

    if (h323EP->UseGatekeeper(m_gatekeeperAddress, m_gatekeeperIdentifier)) {
      LogWindow << "H.323 registration successful to " << *h323EP->GetGatekeeper() << endl;
      return true;
    }

    LogWindow << "H.323 registration failed for " << gkDesc << endl;
  }

  return m_gatekeeperMode < 2;
}


void MyManager::StartRegistrars()
{
  if (sipEP == NULL)
    return;

  for (RegistrarList::iterator iter = m_registrars.begin(); iter != m_registrars.end(); ++iter) {
    if (iter->m_Active) {
      SIPRegister::Params param;
      param.m_addressOfRecord = iter->m_User + '@' + iter->m_Domain;
      param.m_authID = (const char *)iter->m_User;
      param.m_password = (const char *)iter->m_Password;
      param.m_expire = iter->m_TimeToLive;
      bool ok = sipEP->Register(param);
      LogWindow << "SIP registration " << (ok ? "start" : "fail") << "ed for " << iter->m_User << '@' << iter->m_Domain << endl;
    }
  }
}


void MyManager::StopRegistrars()
{
  if (sipEP != NULL)
    sipEP->UnregisterAll();
}


bool MyManager::AdjustFrameSize()
{
  unsigned width, height;
  if (!PVideoFrameInfo::ParseSize(m_VideoGrabFrameSize, width, height)) {
    width = PVideoFrameInfo::CIFWidth;
    height = PVideoFrameInfo::CIFWidth;
  }

  unsigned minWidth, minHeight;
  if (!PVideoFrameInfo::ParseSize(m_VideoMinFrameSize, minWidth, minHeight)) {
    minWidth = PVideoFrameInfo::SQCIFWidth;
    minHeight = PVideoFrameInfo::SQCIFHeight;
  }

  unsigned maxWidth, maxHeight;
  if (!PVideoFrameInfo::ParseSize(m_VideoMaxFrameSize, maxWidth, maxHeight)) {
    maxWidth = PVideoFrameInfo::CIF16Width;
    maxHeight = PVideoFrameInfo::CIF16Height;
  }

  OpalMediaFormatList allMediaFormats;
  OpalMediaFormat::GetAllRegisteredMediaFormats(allMediaFormats);
  for (PINDEX i = 0; i < allMediaFormats.GetSize(); i++) {
    OpalMediaFormat mediaFormat = allMediaFormats[i];
    if (mediaFormat.GetDefaultSessionID() == OpalMediaFormat::DefaultVideoSessionID) {
      mediaFormat.SetOptionInteger(OpalVideoFormat::FrameWidthOption(), width);
      mediaFormat.SetOptionInteger(OpalVideoFormat::FrameHeightOption(), height);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MinRxFrameWidthOption(), minWidth);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MinRxFrameHeightOption(), minHeight);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MaxRxFrameWidthOption(), maxWidth);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MaxRxFrameHeightOption(), maxHeight);
      OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat);
    }
  }

  return true;
}


void MyManager::InitMediaInfo(const char * source, const OpalMediaFormatList & mediaFormats)
{
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++) {
    const OpalMediaFormat & mediaFormat = mediaFormats[i];
    if (mediaFormat.IsTransportable())
      m_mediaInfo.push_back(MyMedia(source, mediaFormat));
  }
}


void MyManager::ApplyMediaInfo()
{
  PStringList mediaFormatOrder, mediaFormatMask;

  m_mediaInfo.sort();

  for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
    if (mm->preferenceOrder < 0)
      mediaFormatMask.AppendString(mm->mediaFormat);
    else
      mediaFormatOrder.AppendString(mm->mediaFormat);
  }

  if (!mediaFormatOrder.IsEmpty()) {
    PTRACE(3, "OpenPhone\tMedia order:\n"<< setfill('\n') << mediaFormatOrder << setfill(' '));
    SetMediaFormatOrder(mediaFormatOrder);
    PTRACE(3, "OpenPhone\tMedia mask:\n"<< setfill('\n') << mediaFormatMask << setfill(' '));
    SetMediaFormatMask(mediaFormatMask);
  }
}


///////////////////////////////////////////////////////////////////////////////

MyMedia::MyMedia()
  : sourceProtocol(NULL)
  , validProtocols(NULL)
  , preferenceOrder(-1) // -1 indicates disabled
  , dirty(false)
{
}


MyMedia::MyMedia(const char * source, const PString & format)
  : sourceProtocol(source)
  , mediaFormat(format)
  , preferenceOrder(-1) // -1 indicates disabled
  , dirty(false)
{
  bool hasSIP = mediaFormat.IsValidForProtocol("sip");
  bool hasH323 = mediaFormat.IsValidForProtocol("h.323");
  if (hasSIP && !hasH323)
    validProtocols = SIPonly;
  else if (!hasSIP && hasH323)
    validProtocols = H323only;
  else
    validProtocols = NULL;
}


///////////////////////////////////////////////////////////////////////////////

class wxFrameSizeValidator: public wxGenericValidator
{
public:
  wxFrameSizeValidator(wxString* val)
    : wxGenericValidator(val)
  {
  }

  virtual wxObject *Clone() const
  {
    return new wxFrameSizeValidator(*this);
  }

  virtual bool Validate(wxWindow * parent)
  {
    unsigned width, height;
    if (PVideoFrameInfo::ParseSize(GetWindow()->GetLabel().c_str(), width, height))
      return true;

    wxMessageBox("Illegal value for video size.", "Error", wxCANCEL|wxICON_EXCLAMATION);
    return false;
  }
};

///////////////////////////////////////////////////////////////////////////////

void MyManager::OnOptions(wxCommandEvent& /*event*/)
{
  OptionsDialog dlg(this);
  dlg.ShowModal();
}

BEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
  ////////////////////////////////////////
  // General fields
  EVT_BUTTON(XRCID("BrowseSoundFile"), OptionsDialog::BrowseSoundFile)
  EVT_BUTTON(XRCID("PlaySoundFile"), OptionsDialog::PlaySoundFile)

  ////////////////////////////////////////
  // Networking fields
  EVT_CHOICE(XRCID("BandwidthClass"), OptionsDialog::BandwidthClass)
  EVT_RADIOBUTTON(XRCID("NoNATUsed"), OptionsDialog::NATHandling)
  EVT_RADIOBUTTON(XRCID("UseNATRouter"), OptionsDialog::NATHandling)
  EVT_RADIOBUTTON(XRCID("UseSTUNServer"), OptionsDialog::NATHandling)
  EVT_LISTBOX(XRCID("LocalInterfaces"), OptionsDialog::SelectedLocalInterface)
  EVT_RADIOBOX(XRCID("InterfaceProtocol"), OptionsDialog::ChangedInterfaceInfo)
  EVT_TEXT(XRCID("InterfaceAddress"), OptionsDialog::ChangedInterfaceInfo)
  EVT_TEXT(XRCID("InterfacePort"), OptionsDialog::ChangedInterfaceInfo)
  EVT_BUTTON(XRCID("AddInterface"), OptionsDialog::AddInterface)
  EVT_BUTTON(XRCID("RemoveInterface"), OptionsDialog::RemoveInterface)

  ////////////////////////////////////////
  // Audio fields
  EVT_COMBOBOX(XRCID(LineInterfaceDeviceKey), OptionsDialog::SelectedLID)

  ////////////////////////////////////////
  // Codec fields
  EVT_BUTTON(XRCID("AddCodec"), OptionsDialog::AddCodec)
  EVT_BUTTON(XRCID("RemoveCodec"), OptionsDialog::RemoveCodec)
  EVT_BUTTON(XRCID("MoveUpCodec"), OptionsDialog::MoveUpCodec)
  EVT_BUTTON(XRCID("MoveDownCodec"), OptionsDialog::MoveDownCodec)
  EVT_LISTBOX(XRCID("AllCodecs"), OptionsDialog::SelectedCodecToAdd)
  EVT_LISTBOX(XRCID("SelectedCodecs"), OptionsDialog::SelectedCodec)
  EVT_LIST_ITEM_SELECTED(XRCID("CodecOptionsList"), OptionsDialog::SelectedCodecOption)
  EVT_LIST_ITEM_DESELECTED(XRCID("CodecOptionsList"), OptionsDialog::DeselectedCodecOption)
  EVT_TEXT(XRCID("CodecOptionValue"), OptionsDialog::ChangedCodecOptionValue)

  ////////////////////////////////////////
  // SIP fields
  EVT_BUTTON(XRCID("AddRegistrar"), OptionsDialog::AddRegistrar)
  EVT_BUTTON(XRCID("ChangeRegistrar"), OptionsDialog::ChangeRegistrar)
  EVT_BUTTON(XRCID("RemoveRegistrar"), OptionsDialog::RemoveRegistrar)
  EVT_LIST_ITEM_SELECTED(XRCID("Registrars"), OptionsDialog::SelectedRegistrar)
  EVT_LIST_ITEM_DESELECTED(XRCID("Registrars"), OptionsDialog::DeselectedRegistrar)
  EVT_TEXT(XRCID("RegistrarDomain"), OptionsDialog::ChangedRegistrarInfo)
  EVT_TEXT(XRCID("RegistrarUsername"), OptionsDialog::ChangedRegistrarInfo)
  EVT_TEXT(XRCID("RegistrarPassword"), OptionsDialog::ChangedRegistrarInfo)
  EVT_TEXT(XRCID("RegistrarTimeToLive"), OptionsDialog::ChangedRegistrarInfo)
  EVT_BUTTON(XRCID("RegistrarUsed"), OptionsDialog::ChangedRegistrarInfo)

  ////////////////////////////////////////
  // Routing fields
  EVT_BUTTON(XRCID("AddRoute"), OptionsDialog::AddRoute)
  EVT_BUTTON(XRCID("RemoveRoute"), OptionsDialog::RemoveRoute)
  EVT_BUTTON(XRCID("MoveUpRoute"), OptionsDialog::MoveUpRoute)
  EVT_BUTTON(XRCID("MoveDownRoute"), OptionsDialog::MoveDownRoute)
  EVT_LIST_ITEM_SELECTED(XRCID("Routes"), OptionsDialog::SelectedRoute)
  EVT_LIST_ITEM_DESELECTED(XRCID("Routes"), OptionsDialog::DeselectedRoute)
  EVT_TEXT(XRCID("RouteDevice"), OptionsDialog::ChangedRouteInfo)
  EVT_TEXT(XRCID("RoutePattern"), OptionsDialog::ChangedRouteInfo)
  EVT_TEXT(XRCID("RouteDestination"), OptionsDialog::ChangedRouteInfo)

  ////////////////////////////////////////
  // H.323 fields
  EVT_LISTBOX(XRCID("Aliases"), OptionsDialog::SelectedAlias)
  EVT_BUTTON(XRCID("AddAlias"), OptionsDialog::AddAlias)
  EVT_BUTTON(XRCID("RemoveAlias"), OptionsDialog::RemoveAlias)
  EVT_TEXT(XRCID("NewAlias"), OptionsDialog::ChangedNewAlias)

  ////////////////////////////////////////
  // Tracing fields
#if PTRACING
  EVT_BUTTON(XRCID("BrowseTraceFile"), OptionsDialog::BrowseTraceFile)
#endif
END_EVENT_TABLE()


#define INIT_FIELD(name, value) \
  m_##name = value; \
  FindWindowByName(name##Key)->SetValidator(wxGenericValidator(&m_##name))

OptionsDialog::OptionsDialog(MyManager * manager)
  : m_manager(*manager)
{
  PINDEX i;

  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
  wxXmlResource::Get()->LoadDialog(this, manager, "OptionsDialog");

  ////////////////////////////////////////
  // General fields
  INIT_FIELD(Username, m_manager.GetDefaultUserName());
  INIT_FIELD(DisplayName, m_manager.GetDefaultDisplayName());

  PStringList devices = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
  wxChoice * choice = FindWindowByNameAs<wxChoice>(this, RingSoundDeviceNameKey);
  choice->SetValidator(wxGenericValidator(&m_RingSoundDeviceName));
  for (i = 0; i < devices.GetSize(); i++)
    choice->Append((const char *)devices[i]);
  m_RingSoundDeviceName = m_manager.m_RingSoundDeviceName;
  INIT_FIELD(RingSoundFileName, m_manager.m_RingSoundFileName);

  INIT_FIELD(AutoAnswer, m_manager.m_autoAnswer);
#if P_EXPAT
  INIT_FIELD(IVRScript, m_manager.ivrEP->GetDefaultVXML());
#endif

  ////////////////////////////////////////
  // Networking fields
  int bandwidth = m_manager.h323EP->GetInitialBandwidth();
  m_Bandwidth.sprintf(bandwidth%10 == 0 ? "%u" : "%u.%u", bandwidth/10, bandwidth%10);
  FindWindowByName(BandwidthKey)->SetValidator(wxTextValidator(wxFILTER_NUMERIC, &m_Bandwidth));
  int bandwidthClass;
  if (bandwidth <= 144)
    bandwidthClass = 0;
  else if (bandwidth <= 288)
    bandwidthClass = 1;
  else if (bandwidth <= 640)
    bandwidthClass = 2;
  else if (bandwidth <= 1280)
    bandwidthClass = 3;
  else if (bandwidth <= 15000)
    bandwidthClass = 4;
  else
    bandwidthClass = 5;
  FindWindowByNameAs<wxChoice>(this, "BandwidthClass")->SetSelection(bandwidthClass);

  INIT_FIELD(TCPPortBase, m_manager.GetTCPPortBase());
  INIT_FIELD(TCPPortMax, m_manager.GetTCPPortMax());
  INIT_FIELD(UDPPortBase, m_manager.GetUDPPortBase());
  INIT_FIELD(UDPPortMax, m_manager.GetUDPPortMax());
  INIT_FIELD(RTPPortBase, m_manager.GetRtpIpPortBase());
  INIT_FIELD(RTPPortMax, m_manager.GetRtpIpPortMax());
  INIT_FIELD(RTPTOS, m_manager.GetRtpIpTypeofService());

  m_NoNATUsedRadio = FindWindowByNameAs<wxRadioButton>(this, "NoNATUsed");
  m_NATRouterRadio = FindWindowByNameAs<wxRadioButton>(this, "UseNATRouter");
  m_STUNServerRadio= FindWindowByNameAs<wxRadioButton>(this, "UseSTUNServer");
  m_NATRouter = m_manager.m_NATRouter;
  m_NATRouterWnd = FindWindowByNameAs<wxTextCtrl>(this, "NATRouter");
  m_NATRouterWnd->SetValidator(wxGenericValidator(&m_NATRouter));
  m_STUNServer = m_manager.m_STUNServer;
  m_STUNServerWnd = FindWindowByNameAs<wxTextCtrl>(this, "STUNServer");
  m_STUNServerWnd->SetValidator(wxGenericValidator(&m_STUNServer));
  switch (m_manager.m_NATHandling) {
    case 2 :
      m_STUNServerRadio->SetValue(true);
      m_NATRouterWnd->Disable();
      break;
    case 1 :
      m_NATRouterRadio->SetValue(true);
      m_STUNServerWnd->Disable();
      break;
    default :
      m_NoNATUsedRadio->SetValue(true);
      m_NATRouterWnd->Disable();
      m_STUNServerWnd->Disable();
  }

  m_AddInterface = FindWindowByNameAs<wxButton>(this, "AddInterface");
  m_AddInterface->Disable();
  m_RemoveInterface = FindWindowByNameAs<wxButton>(this, "RemoveInterface");
  m_RemoveInterface->Disable();
  m_InterfaceProtocol = FindWindowByNameAs<wxRadioBox>(this, "InterfaceProtocol");
  m_InterfacePort = FindWindowByNameAs<wxTextCtrl>(this, "InterfacePort");
  m_InterfaceAddress = FindWindowByNameAs<wxComboBox>(this, "InterfaceAddress");
  m_InterfaceAddress->Append("*");
  PIPSocket::InterfaceTable ifaces;
  if (PIPSocket::GetInterfaceTable(ifaces)) {
    for (i = 0; i < ifaces.GetSize(); i++) {
      PwxString addr = ifaces[i].GetAddress().AsString();
      PwxString name = "%";
      name += ifaces[i].GetName();
      m_InterfaceAddress->Append(addr);
      m_InterfaceAddress->Append(name);
      m_InterfaceAddress->Append(addr + name);
    }
  }
  m_LocalInterfaces = FindWindowByNameAs<wxListBox>(this, "LocalInterfaces");
  for (i = 0; i < m_manager.m_LocalInterfaces.GetSize(); i++)
    m_LocalInterfaces->Append((const char *)m_manager.m_LocalInterfaces[i]);

  ////////////////////////////////////////
  // Sound fields
  INIT_FIELD(SoundBuffers, m_manager.pcssEP->GetSoundChannelBufferDepth());
  INIT_FIELD(MinJitter, m_manager.GetMinAudioJitterDelay());
  INIT_FIELD(MaxJitter, m_manager.GetMaxAudioJitterDelay());
  INIT_FIELD(SilenceSuppression, m_manager.GetSilenceDetectParams().m_mode);
  INIT_FIELD(SilenceThreshold, m_manager.GetSilenceDetectParams().m_threshold);
  INIT_FIELD(SignalDeadband, m_manager.GetSilenceDetectParams().m_signalDeadband/8);
  INIT_FIELD(SilenceDeadband, m_manager.GetSilenceDetectParams().m_silenceDeadband/8);

  // Fill sound player combo box with available devices and set selection
  wxComboBox * combo = FindWindowByNameAs<wxComboBox>(this, SoundPlayerKey);
  combo->SetValidator(wxGenericValidator(&m_SoundPlayer));
  for (i = 0; i < devices.GetSize(); i++)
    combo->Append((const char *)devices[i]);
  m_SoundPlayer = m_manager.pcssEP->GetSoundChannelPlayDevice();

  // Fill sound recorder combo box with available devices and set selection
  combo = FindWindowByNameAs<wxComboBox>(this, SoundRecorderKey);
  combo->SetValidator(wxGenericValidator(&m_SoundRecorder));
  devices = PSoundChannel::GetDeviceNames(PSoundChannel::Recorder);
  for (i = 0; i < devices.GetSize(); i++)
    combo->Append((const char *)devices[i]);
  m_SoundRecorder = m_manager.pcssEP->GetSoundChannelRecordDevice();

  // Fill line interface combo box with available devices and set selection
  m_selectedAEC = FindWindowByNameAs<wxChoice>(this, AECKey);
  m_selectedCountry = FindWindowByNameAs<wxComboBox>(this, CountryKey);
  m_selectedCountry->SetValidator(wxGenericValidator(&m_Country));
  m_selectedLID = FindWindowByNameAs<wxComboBox>(this, LineInterfaceDeviceKey);
  m_selectedLID->SetValidator(wxGenericValidator(&m_LineInterfaceDevice));
  devices = OpalLineInterfaceDevice::GetAllDevices();
  if (devices.IsEmpty()) {
    m_LineInterfaceDevice = "<< None available >>";
    m_selectedLID->Append(m_LineInterfaceDevice);
    m_selectedAEC->Disable();
    m_selectedCountry->Disable();
  }
  else {
    static const char UseSoundCard[] = "<< Use sound card only >>";
    m_selectedLID->Append(UseSoundCard);
    for (i = 0; i < devices.GetSize(); i++)
      m_selectedLID->Append((const char *)devices[i]);

    OpalLine * line = m_manager.potsEP->GetLine("*");
    if (line != NULL) {
      m_LineInterfaceDevice = line->GetDevice().GetDeviceType() + ": " + line->GetDevice().GetDeviceName();
      for (i = 0; i < devices.GetSize(); i++) {
        if (m_LineInterfaceDevice == devices[i])
          break;
      }
      if (i >= devices.GetSize()) {
        for (i = 0; i < devices.GetSize(); i++) {
          if (devices[i].Find(m_LineInterfaceDevice.c_str()) == 0)
            break;
        }
        if (i >= devices.GetSize())
          m_LineInterfaceDevice = devices[0];
      }

      INIT_FIELD(AEC, line->GetAEC());

      PStringList countries = line->GetDevice().GetCountryCodeNameList();
      for (i = 0; i < countries.GetSize(); i++)
        m_selectedCountry->Append((const char *)countries[i]);
      INIT_FIELD(Country, line->GetDevice().GetCountryCodeName());
    }
    else {
      m_LineInterfaceDevice = UseSoundCard;
      m_selectedAEC->Disable();
      m_selectedCountry->Disable();
    }
  }

  ////////////////////////////////////////
  // Video fields
  INIT_FIELD(VideoGrabber, m_manager.GetVideoInputDevice().deviceName);
  INIT_FIELD(VideoGrabFormat, m_manager.GetVideoInputDevice().videoFormat);
  INIT_FIELD(VideoGrabSource, m_manager.GetVideoInputDevice().channelNumber);
  INIT_FIELD(VideoGrabFrameRate, m_manager.GetVideoInputDevice().rate);
  INIT_FIELD(VideoFlipLocal, m_manager.GetVideoInputDevice().flip != PFalse);
  INIT_FIELD(VideoGrabPreview, m_manager.m_VideoGrabPreview);
  INIT_FIELD(VideoAutoTransmit, m_manager.CanAutoStartTransmitVideo() != PFalse);
  INIT_FIELD(VideoAutoReceive, m_manager.CanAutoStartReceiveVideo() != PFalse);
  INIT_FIELD(VideoFlipRemote, m_manager.GetVideoOutputDevice().flip != PFalse);

  m_VideoGrabFrameSize = m_manager.m_VideoGrabFrameSize;
  FindWindowByName(VideoGrabFrameSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoGrabFrameSize));
  m_VideoMinFrameSize = m_manager.m_VideoMinFrameSize;
  FindWindowByName(VideoMinFrameSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoMinFrameSize));
  m_VideoMaxFrameSize = m_manager.m_VideoMaxFrameSize;
  FindWindowByName(VideoMaxFrameSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoMaxFrameSize));

  combo = FindWindowByNameAs<wxComboBox>(this, "VideoGrabber");
  devices = PVideoInputDevice::GetDriversDeviceNames("*");
  for (i = 0; i < devices.GetSize(); i++)
    combo->Append((const char *)devices[i]);

  ////////////////////////////////////////
  // Codec fields
  m_AddCodec = FindWindowByNameAs<wxButton>(this, "AddCodec");
  m_AddCodec->Disable();
  m_RemoveCodec = FindWindowByNameAs<wxButton>(this, "RemoveCodec");
  m_RemoveCodec->Disable();
  m_MoveUpCodec = FindWindowByNameAs<wxButton>(this, "MoveUpCodec");
  m_MoveUpCodec->Disable();
  m_MoveDownCodec = FindWindowByNameAs<wxButton>(this, "MoveDownCodec");
  m_MoveDownCodec->Disable();

  m_allCodecs = FindWindowByNameAs<wxListBox>(this, "AllCodecs");
  m_selectedCodecs = FindWindowByNameAs<wxListBox>(this, "SelectedCodecs");
  for (MyMediaList::iterator mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm) {
    wxString str = mm->sourceProtocol;
    str += ": ";
    str += (const char *)mm->mediaFormat;
    str += mm->validProtocols;
    m_allCodecs->Append(str, &*mm);

    str = (const char *)mm->mediaFormat;
    if (mm->preferenceOrder >= 0 && m_selectedCodecs->FindString(str) < 0)
      m_selectedCodecs->Append(str, &*mm);
  }
  m_codecOptions = FindWindowByNameAs<wxListCtrl>(this, "CodecOptionsList");
  int columnWidth = (m_codecOptions->GetClientSize().GetWidth()-30)/2;
  m_codecOptions->InsertColumn(0, "Option", wxLIST_FORMAT_LEFT, columnWidth);
  m_codecOptions->InsertColumn(1, "Value", wxLIST_FORMAT_LEFT, columnWidth);
  m_codecOptionValue = FindWindowByNameAs<wxTextCtrl>(this, "CodecOptionValue");
  m_codecOptionValue->Disable();

  ////////////////////////////////////////
  // H.323 fields
  m_AddAlias = FindWindowByNameAs<wxButton>(this, "AddAlias");
  m_AddAlias->Disable();
  m_RemoveAlias = FindWindowByNameAs<wxButton>(this, "RemoveAlias");
  m_RemoveAlias->Disable();
  m_NewAlias = FindWindowByNameAs<wxTextCtrl>(this, "NewAlias");
  m_Aliases = FindWindowByNameAs<wxListBox>(this, "Aliases");
  PStringList aliases = m_manager.h323EP->GetAliasNames();
  for (i = 1; i < aliases.GetSize(); i++)
    m_Aliases->Append((const char *)aliases[i]);

  INIT_FIELD(DTMFSendMode, m_manager.h323EP->GetSendUserInputMode());
  if (m_DTMFSendMode > OpalConnection::SendUserInputAsInlineRFC2833)
    m_DTMFSendMode = OpalConnection::SendUserInputAsString;
  INIT_FIELD(CallIntrusionProtectionLevel, m_manager.h323EP->GetCallIntrusionProtectionLevel());
  INIT_FIELD(DisableFastStart, m_manager.h323EP->IsFastStartDisabled() != PFalse);
  INIT_FIELD(DisableH245Tunneling, m_manager.h323EP->IsH245TunnelingDisabled() != PFalse);
  INIT_FIELD(DisableH245inSETUP, m_manager.h323EP->IsH245inSetupDisabled() != PFalse);
  INIT_FIELD(GatekeeperMode, m_manager.m_gatekeeperMode);
  INIT_FIELD(GatekeeperAddress, m_manager.m_gatekeeperAddress);
  INIT_FIELD(GatekeeperIdentifier, m_manager.m_gatekeeperIdentifier);
  INIT_FIELD(GatekeeperTTL, m_manager.h323EP->GetGatekeeperTimeToLive().GetSeconds());
  INIT_FIELD(GatekeeperLogin, m_manager.h323EP->GetGatekeeperUsername());
  INIT_FIELD(GatekeeperPassword, m_manager.h323EP->GetGatekeeperPassword());

  ////////////////////////////////////////
  // SIP fields
  INIT_FIELD(SIPProxyUsed, m_manager.m_SIPProxyUsed);
  INIT_FIELD(SIPProxy, m_manager.sipEP->GetProxy().GetHostName());
  INIT_FIELD(SIPProxyUsername, m_manager.sipEP->GetProxy().GetUserName());
  INIT_FIELD(SIPProxyPassword, m_manager.sipEP->GetProxy().GetPassword());

  m_SelectedRegistrar = INT_MAX;

  m_Registrars = FindWindowByNameAs<wxListCtrl>(this, "Registrars");
  m_Registrars->InsertColumn(0, _T("Domain"));
  m_Registrars->InsertColumn(1, _T("User"));
  m_Registrars->InsertColumn(2, _T("Refresh"));
  m_Registrars->InsertColumn(3, _T("Status"));
  for (RegistrarList::iterator registrar = m_manager.m_registrars.begin(); registrar != m_manager.m_registrars.end(); ++registrar)
    RegistrarToList(false, new RegistrarInfo(*registrar), INT_MAX);
  m_Registrars->SetColumnWidth(0, 200);
  m_Registrars->SetColumnWidth(1, 150);
  m_Registrars->SetColumnWidth(2, 75);
  m_Registrars->SetColumnWidth(3, 75);

  m_AddRegistrar = FindWindowByNameAs<wxButton>(this, "AddRegistrar");
  m_AddRegistrar->Disable();

  m_ChangeRegistrar = FindWindowByNameAs<wxButton>(this, "ChangeRegistrar");
  m_ChangeRegistrar->Disable();

  m_RemoveRegistrar = FindWindowByNameAs<wxButton>(this, "RemoveRegistrar");
  m_RemoveRegistrar->Disable();

  m_RegistrarDomain = FindWindowByNameAs<wxTextCtrl>(this, "RegistrarDomain");
  m_RegistrarUser = FindWindowByNameAs<wxTextCtrl>(this, "RegistrarUsername");
  m_RegistrarPassword = FindWindowByNameAs<wxTextCtrl>(this, "RegistrarPassword");
  m_RegistrarTimeToLive = FindWindowByNameAs<wxSpinCtrl>(this, "RegistrarTimeToLive");
  m_RegistrarActive = FindWindowByNameAs<wxCheckBox>(this, "RegistrarUsed");

  ////////////////////////////////////////
  // Routing fields
  m_SelectedRoute = INT_MAX;

  m_RouteDevice = FindWindowByNameAs<wxTextCtrl>(this, "RouteDevice");
  m_RoutePattern = FindWindowByNameAs<wxTextCtrl>(this, "RoutePattern");
  m_RouteDestination = FindWindowByNameAs<wxTextCtrl>(this, "RouteDestination");

  m_AddRoute = FindWindowByNameAs<wxButton>(this, "AddRoute");
  m_AddRoute->Disable();
  m_RemoveRoute = FindWindowByNameAs<wxButton>(this, "RemoveRoute");
  m_RemoveRoute->Disable();
  m_MoveUpRoute = FindWindowByNameAs<wxButton>(this, "MoveUpRoute");
  m_MoveUpRoute->Disable();
  m_MoveDownRoute = FindWindowByNameAs<wxButton>(this, "MoveDownRoute");
  m_MoveDownRoute->Disable();

  // Fill list box with active routes
  static char const AllSources[] = "<ALL>";
  m_Routes = FindWindowByNameAs<wxListCtrl>(this, "Routes");
  m_Routes->InsertColumn(0, _T("Source"));
  m_Routes->InsertColumn(1, _T("Dev/If"));
  m_Routes->InsertColumn(2, _T("Pattern"));
  m_Routes->InsertColumn(3, _T("Destination"));
  const OpalManager::RouteTable & routeTable = m_manager.GetRouteTable();
  for (i = 0; i < routeTable.GetSize(); i++) {
    PString expression = routeTable[i].pattern;

    PINDEX tab = expression.Find('\t');
    if (tab == P_MAX_INDEX)
      tab = expression.Find("\\t");

    PINDEX colon = expression.Find(':');

    PwxString source, device, pattern;
    if (colon >= tab) {
      source = AllSources;
      device = (const char *)expression(colon+1, tab-1);
      pattern = expression.Mid(tab+1);
    }
    else {
      source = expression.Left(colon);
      if (source == ".*")
        source = AllSources;
      if (tab == P_MAX_INDEX)
        pattern = expression.Mid(colon+1);
      else {
        device = expression(colon+1, tab-1);
        if (device == ".*")
          device = "";
        pattern = expression.Mid(tab + (expression[tab] == '\t' ? 1 : 2));
      }
    }

    int pos = m_Routes->InsertItem(INT_MAX, source);
    m_Routes->SetItem(pos, 1, device);
    m_Routes->SetItem(pos, 2, pattern);
    m_Routes->SetItem(pos, 3, (const char *)routeTable[i].destination);
  }

  for (i = 0; i < m_Routes->GetColumnCount(); i++)
    m_Routes->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);

  // Fill combo box with possible protocols
  m_RouteSource = FindWindowByNameAs<wxComboBox>(this, "RouteSource");
  m_RouteSource->Append(AllSources);
  PList<OpalEndPoint> endpoints = m_manager.GetEndPoints();
  for (i = 0; i < endpoints.GetSize(); i++)
    m_RouteSource->Append((const char *)endpoints[i].GetPrefixName());
  m_RouteSource->SetSelection(0);


#if PTRACING
  ////////////////////////////////////////
  // Tracing fields
  INIT_FIELD(EnableTracing, m_manager.m_enableTracing);
  INIT_FIELD(TraceLevelThreshold, PTrace::GetLevel());
  INIT_FIELD(TraceLevelNumber, (PTrace::GetOptions()&PTrace::TraceLevel) != 0);
  INIT_FIELD(TraceFileLine, (PTrace::GetOptions()&PTrace::FileAndLine) != 0);
  INIT_FIELD(TraceBlocks, (PTrace::GetOptions()&PTrace::Blocks) != 0);
  INIT_FIELD(TraceDateTime, (PTrace::GetOptions()&PTrace::DateAndTime) != 0);
  INIT_FIELD(TraceTimestamp, (PTrace::GetOptions()&PTrace::Timestamp) != 0);
  INIT_FIELD(TraceThreadName, (PTrace::GetOptions()&PTrace::Thread) != 0);
  INIT_FIELD(TraceThreadAddress, (PTrace::GetOptions()&PTrace::ThreadAddress) != 0);
  INIT_FIELD(TraceFileName, m_manager.m_traceFileName);
#else
  wxNotebook * book = FindWindowByNameAs<wxNotebook>(this, "OptionsNotebook");
  book->DeletePage(book->GetPageCount()-1);
#endif // PTRACING
}


OptionsDialog::~OptionsDialog()
{
  for (int i = 0; i < m_Registrars->GetItemCount(); ++i)
    delete (RegistrarInfo *)m_Registrars->GetItemData(i);
}


#define SAVE_FIELD(name, set) \
  set(m_##name); \
  config->Write(name##Key, m_##name)

#define SAVE_FIELD2(name1, name2, set) \
  set(m_##name1, m_##name2); \
  config->Write(name1##Key, m_##name1); \
  config->Write(name2##Key, m_##name2)

bool OptionsDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  double floatBandwidth;
  if (!m_Bandwidth.ToDouble(&floatBandwidth) || floatBandwidth < 10)
    return false;

  ::wxBeginBusyCursor();

  wxConfigBase * config = wxConfig::Get();

  ////////////////////////////////////////
  // General fields
  config->SetPath(GeneralGroup);
  SAVE_FIELD(Username, m_manager.SetDefaultUserName);
  SAVE_FIELD(DisplayName, m_manager.SetDefaultDisplayName);
  SAVE_FIELD(RingSoundDeviceName, m_manager.m_RingSoundDeviceName = );
  SAVE_FIELD(RingSoundFileName, m_manager.m_RingSoundFileName = );
  SAVE_FIELD(AutoAnswer, m_manager.m_autoAnswer = );
#if P_EXPAT
  SAVE_FIELD(IVRScript, m_manager.ivrEP->SetDefaultVXML);
#endif

  ////////////////////////////////////////
  // Networking fields
  config->SetPath(NetworkingGroup);
  int adjustedBandwidth = (int)(floatBandwidth*10);
  m_manager.h323EP->SetInitialBandwidth(adjustedBandwidth);
  config->Write(BandwidthKey, adjustedBandwidth);
  SAVE_FIELD2(TCPPortBase, TCPPortMax, m_manager.SetTCPPorts);
  SAVE_FIELD2(UDPPortBase, UDPPortMax, m_manager.SetUDPPorts);
  SAVE_FIELD2(RTPPortBase, RTPPortMax, m_manager.SetRtpIpPorts);
  SAVE_FIELD(RTPTOS, m_manager.SetRtpIpTypeofService);
  m_manager.m_NATHandling = m_STUNServerRadio->GetValue() ? 2 : m_NATRouterRadio->GetValue() ? 1 : 0;
  config->Write(NATHandlingKey, m_manager.m_NATHandling);
  SAVE_FIELD(STUNServer, m_manager.m_STUNServer = );
  SAVE_FIELD(NATRouter, m_manager.m_NATRouter = );
  m_manager.SetNATHandling();

  config->DeleteGroup(LocalInterfacesGroup);
  config->SetPath(LocalInterfacesGroup);
  PStringArray newInterfaces(m_LocalInterfaces->GetCount());
  bool changed = m_manager.m_LocalInterfaces.GetSize() != newInterfaces.GetSize();
  for (int i = 0; i < newInterfaces.GetSize(); i++) {
    newInterfaces[i] = m_LocalInterfaces->GetString(i);
    PINDEX oldIndex = m_manager.m_LocalInterfaces.GetValuesIndex(newInterfaces[i]);
    if (oldIndex == P_MAX_INDEX || newInterfaces[i] != m_manager.m_LocalInterfaces[oldIndex])
      changed = true;
    wxString key;
    key.sprintf("%u", i+1);
    config->Write(key, (const char *)newInterfaces[i]);
  }
  if (changed) {
    m_manager.m_LocalInterfaces = newInterfaces;
    m_manager.StartAllListeners();
  }

  ////////////////////////////////////////
  // Sound fields
  config->SetPath(AudioGroup);
  SAVE_FIELD(SoundPlayer, m_manager.pcssEP->SetSoundChannelPlayDevice);
  SAVE_FIELD(SoundRecorder, m_manager.pcssEP->SetSoundChannelRecordDevice);
  SAVE_FIELD(SoundBuffers, m_manager.pcssEP->SetSoundChannelBufferDepth);
  SAVE_FIELD2(MinJitter, MaxJitter, m_manager.SetAudioJitterDelay);

  OpalSilenceDetector::Params silenceParams;
  SAVE_FIELD(SilenceSuppression, silenceParams.m_mode=(OpalSilenceDetector::Mode));
  SAVE_FIELD(SilenceThreshold, silenceParams.m_threshold=);
  SAVE_FIELD(SignalDeadband, silenceParams.m_signalDeadband=8*);
  SAVE_FIELD(SilenceDeadband, silenceParams.m_silenceDeadband=8*);
  m_manager.SetSilenceDetectParams(silenceParams);

  if (m_LineInterfaceDevice.StartsWith("<<") && m_LineInterfaceDevice.EndsWith(">>"))
    m_LineInterfaceDevice.Empty();
  config->Write(LineInterfaceDeviceKey, m_LineInterfaceDevice);
  config->Write(AECKey, m_AEC);
  config->Write(CountryKey, m_Country);
  m_manager.StartLID();

  ////////////////////////////////////////
  // Video fields
  config->SetPath(VideoGroup);
  PVideoDevice::OpenArgs grabber = m_manager.GetVideoInputDevice();
  SAVE_FIELD(VideoGrabber, grabber.deviceName = (const char *));
  SAVE_FIELD(VideoGrabFormat, grabber.videoFormat = (PVideoDevice::VideoFormat));
  SAVE_FIELD(VideoGrabSource, grabber.channelNumber = );
  SAVE_FIELD(VideoGrabFrameRate, grabber.rate = );
  SAVE_FIELD(VideoGrabFrameSize, m_manager.m_VideoGrabFrameSize = );
  SAVE_FIELD(VideoFlipLocal, grabber.flip = );
  m_manager.SetVideoInputDevice(grabber);
  SAVE_FIELD(VideoGrabPreview, m_manager.m_VideoGrabPreview = );
  SAVE_FIELD(VideoAutoTransmit, m_manager.SetAutoStartTransmitVideo);
  SAVE_FIELD(VideoAutoReceive, m_manager.SetAutoStartReceiveVideo);
//  SAVE_FIELD(VideoFlipRemote, );
  SAVE_FIELD(VideoMinFrameSize, m_manager.m_VideoMinFrameSize = );
  SAVE_FIELD(VideoMaxFrameSize, m_manager.m_VideoMaxFrameSize = );
  m_manager.AdjustFrameSize();

  ////////////////////////////////////////
  // Codec fields
  MyMediaList::iterator mm;
  for (mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm)
    mm->preferenceOrder = -1;

  size_t codecIndex;
  for (codecIndex = 0; codecIndex < m_selectedCodecs->GetCount(); codecIndex++) {
    PwxString selectedFormat = m_selectedCodecs->GetString(codecIndex);
    for (mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm) {
      if (selectedFormat == mm->mediaFormat) {
        mm->preferenceOrder = codecIndex;
        break;
      }
    }
  }

  m_manager.ApplyMediaInfo();

  config->DeleteGroup(CodecsGroup);
  for (mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm) {
    if (mm->preferenceOrder >= 0) {
      wxString groupName;
      groupName.sprintf("%s/%04u", CodecsGroup, mm->preferenceOrder);
      config->SetPath(groupName);
      config->Write(CodecNameKey, (const char *)mm->mediaFormat);
      for (PINDEX i = 0; i < mm->mediaFormat.GetOptionCount(); i++) {
        const OpalMediaOption & option = mm->mediaFormat.GetOption(i);
        if (!option.IsReadOnly())
          config->Write(PwxString(option.GetName()), PwxString(option.AsString()));
      }
      if (mm->dirty) {
        OpalMediaFormat::SetRegisteredMediaFormat(mm->mediaFormat);
        mm->dirty = false;
      }
    }
  }


  ////////////////////////////////////////
  // H.323 fields
  config->DeleteGroup(H323AliasesGroup);
  config->SetPath(H323AliasesGroup);
  m_manager.h323EP->SetLocalUserName(m_Username);
  PStringList aliases = m_manager.h323EP->GetAliasNames();
  for (size_t i = 0; i < m_Aliases->GetCount(); i++) {
    wxString alias = m_Aliases->GetString(i);
    m_manager.h323EP->AddAliasName(alias.c_str());
    wxString key;
    key.sprintf("%u", i+1);
    config->Write(key, alias);
  }

  config->SetPath(H323Group);
  m_manager.h323EP->SetSendUserInputMode((H323Connection::SendUserInputModes)m_DTMFSendMode);
  config->Write(DTMFSendModeKey, m_DTMFSendMode);
  SAVE_FIELD(CallIntrusionProtectionLevel, m_manager.h323EP->SetCallIntrusionProtectionLevel);
  SAVE_FIELD(DisableFastStart, m_manager.h323EP->DisableFastStart);
  SAVE_FIELD(DisableH245Tunneling, m_manager.h323EP->DisableH245Tunneling);
  SAVE_FIELD(DisableH245inSETUP, m_manager.h323EP->DisableH245inSetup);

  config->Write(GatekeeperTTLKey, m_GatekeeperTTL);
  m_manager.h323EP->SetGatekeeperTimeToLive(PTimeInterval(0, m_GatekeeperTTL));

  if (m_manager.m_gatekeeperMode != m_GatekeeperMode ||
      m_manager.m_gatekeeperAddress != m_GatekeeperAddress ||
      m_manager.m_gatekeeperIdentifier != m_GatekeeperIdentifier ||
      m_manager.h323EP->GetGatekeeperUsername() != m_GatekeeperLogin.c_str() ||
      m_manager.h323EP->GetGatekeeperPassword() != m_GatekeeperPassword.c_str()) {
    SAVE_FIELD(GatekeeperMode, m_manager.m_gatekeeperMode = );
    SAVE_FIELD(GatekeeperAddress, m_manager.m_gatekeeperAddress = );
    SAVE_FIELD(GatekeeperIdentifier, m_manager.m_gatekeeperIdentifier = );
    SAVE_FIELD2(GatekeeperPassword, GatekeeperLogin, m_manager.h323EP->SetGatekeeperPassword);

    if (!m_manager.StartGatekeeper())
      m_manager.Close();
  }

  ////////////////////////////////////////
  // SIP fields
  config->SetPath(SIPGroup);
  SAVE_FIELD(SIPProxyUsed, m_manager.m_SIPProxyUsed =);
  m_manager.sipEP->SetProxy(m_SIPProxy, m_SIPProxyUsername, m_SIPProxyPassword);
  config->Write(SIPProxyKey, m_SIPProxy);
  config->Write(SIPProxyUsernameKey, m_SIPProxyUsername);
  config->Write(SIPProxyPasswordKey, m_SIPProxyPassword);

  RegistrarList newRegistrars;

  for (int i = 0; i < m_Registrars->GetItemCount(); ++i)
    newRegistrars.push_back(*(RegistrarInfo *)m_Registrars->GetItemData(i));

  if (newRegistrars != m_manager.m_registrars) {
    config->DeleteEntry(RegistrarUsedKey);
    config->DeleteEntry(RegistrarNameKey);
    config->DeleteEntry(RegistrarUsernameKey);
    config->DeleteEntry(RegistrarPasswordKey);
    config->DeleteGroup(RegistrarGroup);

    int registrarIndex = 1;
    for (RegistrarList::iterator iterReg = newRegistrars.begin(); iterReg != newRegistrars.end(); ++iterReg) {
      wxString group;
      group.sprintf("%s/%04u", RegistrarGroup, registrarIndex++);
      config->SetPath(group);
      config->Write(RegistrarUsedKey, iterReg->m_Active);
      config->Write(RegistrarDomainKey, iterReg->m_Domain);
      config->Write(RegistrarUsernameKey, iterReg->m_User);
      config->Write(RegistrarPasswordKey, iterReg->m_Password);
      config->Write(RegistrarTimeToLiveKey, iterReg->m_TimeToLive);
    }

    m_manager.StopRegistrars();
    m_manager.m_registrars = newRegistrars;
    m_manager.StartRegistrars();
  }

  ////////////////////////////////////////
  // Routing fields

  config->DeleteGroup(RoutingGroup);
  config->SetPath(RoutingGroup);
  PStringArray routeSpecs;
  for (int i = 0; i < m_Routes->GetItemCount(); i++) {
    PwxString spec;
    wxListItem item;
    item.m_itemId = i;
    item.m_mask = wxLIST_MASK_TEXT;
    m_Routes->GetItem(item);
    spec += item.m_text;
    spec += ':';
    item.m_col++;
    m_Routes->GetItem(item);
    spec += item.m_text.empty() ? ".*" : item.m_text;
    spec += "\\t";
    item.m_col++;
    m_Routes->GetItem(item);
    spec += item.m_text;
    spec += '=';
    item.m_col++;
    m_Routes->GetItem(item);
    spec += item.m_text;
    routeSpecs.AppendString(spec);

    wxString key;
    key.sprintf("%04u", i+1);
    config->Write(key, spec);
  }
  m_manager.SetRouteTable(routeSpecs);


#if PTRACING
  ////////////////////////////////////////
  // Tracing fields
  config->SetPath(TracingGroup);
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
  if (m_manager.m_enableTracing && (!m_EnableTracing || m_TraceFileName.empty()))
    PTrace::SetStream(NULL);
  else if (m_EnableTracing && (!m_manager.m_enableTracing || m_manager.m_traceFileName != m_TraceFileName))
    PTrace::Initialise(m_TraceLevelThreshold, m_TraceFileName, traceOptions);
  else {
    PTrace::SetLevel(m_TraceLevelThreshold);
    PTrace::SetOptions(traceOptions);
    PTrace::ClearOptions(~traceOptions);
  }

  m_manager.m_enableTracing = m_EnableTracing;
  m_manager.m_traceFileName = m_TraceFileName;
#endif // PTRACING

  ::wxEndBusyCursor();

  return true;
}


////////////////////////////////////////
// General fields

void OptionsDialog::BrowseSoundFile(wxCommandEvent & /*event*/)
{
  wxString newFile = wxFileSelector("Sound file to play on incoming calls",
                                    "",
                                    m_RingSoundFileName,
                                    ".wav",
                                    "WAV files (*.wav)|*.wav",
                                    wxOPEN|wxFILE_MUST_EXIST);
  if (!newFile.empty()) {
    m_RingSoundFileName = newFile;
    TransferDataToWindow();
  }
}


void OptionsDialog::PlaySoundFile(wxCommandEvent & /*event*/)
{
  PSoundChannel speaker(m_manager.m_RingSoundDeviceName, PSoundChannel::Player);
  speaker.PlayFile(m_RingSoundFileName.c_str());
}


////////////////////////////////////////
// Networking fields

void OptionsDialog::BandwidthClass(wxCommandEvent & event)
{
  static const char * bandwidthClasses[] = {
    "14.4", "28.8", "64.0", "128", "1500", "10000"
  };

  m_Bandwidth = bandwidthClasses[event.GetSelection()];
  TransferDataToWindow();
}


void OptionsDialog::NATHandling(wxCommandEvent &)
{
  if (m_STUNServerRadio->GetValue()) {
    m_STUNServerWnd->Enable();
    m_NATRouterWnd->Disable();
  }
  else if (m_NATRouterRadio->GetValue()) {
    m_STUNServerWnd->Disable();
    m_NATRouterWnd->Enable();
  }
  else {
    m_STUNServerWnd->Disable();
    m_NATRouterWnd->Disable();
  }
}


void OptionsDialog::SelectedLocalInterface(wxCommandEvent & /*event*/)
{
  m_RemoveInterface->Enable(m_LocalInterfaces->GetSelection() != wxNOT_FOUND);
}


void OptionsDialog::ChangedInterfaceInfo(wxCommandEvent & /*event*/)
{
  bool enab = true;
  PString iface = m_InterfaceAddress->GetValue().c_str();
  if (iface.IsEmpty())
    enab = false;
  else if (iface != "*") {
    PIPSocket::Address test(iface);
    if (!test.IsValid())
      enab = false;
  }

  if (m_InterfaceProtocol->GetSelection() == 0)
    m_InterfacePort->Disable();
  else {
    m_InterfacePort->Enable();
    if (m_InterfacePort->GetValue().IsEmpty())
      enab = false;
  }

  m_AddInterface->Enable(enab);
}


static const char * const InterfacePrefixes[] = {
  "all:", "h323:tcp$", "sip:udp$", "sip:tcp$"
};

void OptionsDialog::AddInterface(wxCommandEvent & /*event*/)
{
  int proto = m_InterfaceProtocol->GetSelection();
  wxString iface = InterfacePrefixes[proto];
  iface += m_InterfaceAddress->GetValue();
  if (proto > 0)
    iface += ':' + m_InterfacePort->GetValue();
  m_LocalInterfaces->Append(iface);
}


void OptionsDialog::RemoveInterface(wxCommandEvent & /*event*/)
{
  wxString iface = m_LocalInterfaces->GetStringSelection();

  for (int i = 0; i < PARRAYSIZE(InterfacePrefixes); i++) {
    if (iface.StartsWith(InterfacePrefixes[i])) {
      m_InterfaceProtocol->SetSelection(i);
      iface.Remove(0, strlen(InterfacePrefixes[i]));
    }
  }

  size_t colon = iface.find(':');
  if (colon != string::npos) {
    m_InterfacePort->SetValue(iface.Mid(colon+1));
    iface.Remove(colon);
  }

  m_InterfaceAddress->SetValue(iface);
  m_LocalInterfaces->Delete(m_LocalInterfaces->GetSelection());
  m_RemoveInterface->Disable();
}


////////////////////////////////////////
// Audio fields

void OptionsDialog::SelectedLID(wxCommandEvent & /*event*/)
{
  bool enabled = m_selectedLID->GetSelection() > 0;
  m_selectedAEC->Enable(enabled);
  m_selectedCountry->Enable(enabled);
}


////////////////////////////////////////
// Codec fields

void OptionsDialog::AddCodec(wxCommandEvent & /*event*/)
{
  int insertionPoint = -1;
  wxArrayInt destinationSelections;
  if (m_selectedCodecs->GetSelections(destinationSelections) > 0)
    insertionPoint = destinationSelections[0];

  wxArrayInt sourceSelections;
  m_allCodecs->GetSelections(sourceSelections);
  for (size_t i = 0; i < sourceSelections.GetCount(); i++) {
    int sourceSelection = sourceSelections[i];
    wxString value = m_allCodecs->GetString(sourceSelection);
    void * data = m_allCodecs->GetClientData(sourceSelection);
    value.Remove(0, value.Find(':')+2);
    value.Replace(SIPonly, "");
    value.Replace(H323only, "");
    if (m_selectedCodecs->FindString(value) < 0) {
      if (insertionPoint < 0)
        m_selectedCodecs->Append(value, data);
      else {
        m_selectedCodecs->InsertItems(1, &value, insertionPoint);
        m_selectedCodecs->SetClientData(insertionPoint, data);
      }
    }
    m_allCodecs->Deselect(sourceSelections[i]);
  }

  m_AddCodec->Enable(false);
}


void OptionsDialog::RemoveCodec(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  m_selectedCodecs->GetSelections(selections);
  for (int i = selections.GetCount()-1; i >= 0; i--)
    m_selectedCodecs->Delete(selections[i]);
  m_RemoveCodec->Enable(false);
  m_MoveUpCodec->Enable(false);
  m_MoveDownCodec->Enable(false);
}


void OptionsDialog::MoveUpCodec(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  m_selectedCodecs->GetSelections(selections);
  int selection = selections[0];
  wxString value = m_selectedCodecs->GetString(selection);
  MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selection);
  m_selectedCodecs->Delete(selection);
  m_selectedCodecs->InsertItems(1, &value, --selection);
  m_selectedCodecs->SetClientData(selection, media);
  m_selectedCodecs->SetSelection(selection);
  m_MoveUpCodec->Enable(selection > 0);
  m_MoveDownCodec->Enable(true);
}


void OptionsDialog::MoveDownCodec(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  m_selectedCodecs->GetSelections(selections);
  int selection = selections[0];
  wxString value = m_selectedCodecs->GetString(selection);
  MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selection);
  m_selectedCodecs->Delete(selection);
  m_selectedCodecs->InsertItems(1, &value, ++selection);
  m_selectedCodecs->SetClientData(selection, media);
  m_selectedCodecs->SetSelection(selection);
  m_MoveUpCodec->Enable(true);
  m_MoveDownCodec->Enable(selection < (int)m_selectedCodecs->GetCount()-1);
}


void OptionsDialog::SelectedCodecToAdd(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  m_AddCodec->Enable(m_allCodecs->GetSelections(selections) > 0);
}


void OptionsDialog::SelectedCodec(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  size_t count = m_selectedCodecs->GetSelections(selections);
  m_RemoveCodec->Enable(count > 0);
  m_MoveUpCodec->Enable(count == 1 && selections[0] > 0);
  m_MoveDownCodec->Enable(count == 1 && selections[0] < (int)m_selectedCodecs->GetCount()-1);

  m_codecOptions->DeleteAllItems();
  m_codecOptionValue->SetValue("");
  m_codecOptionValue->Disable();

  if (count == 1) {
    MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selections[0]);
    PAssert(media != NULL, PLogicError);
    for (PINDEX i = 0; i < media->mediaFormat.GetOptionCount(); i++) {
      const OpalMediaOption & option = media->mediaFormat.GetOption(i);
      wxListItem item;
      item.m_mask = wxLIST_MASK_TEXT|wxLIST_MASK_DATA;
      item.m_itemId = LONG_MAX;
      item.m_text = PwxString(option.GetName());
      item.m_data = option.IsReadOnly();
      long index = m_codecOptions->InsertItem(item);
      m_codecOptions->SetItem(index, 1, PwxString(option.AsString()));
    }
  }
}


void OptionsDialog::SelectedCodecOption(wxListEvent & /*event*/)
{
  wxListItem item;
  item.m_mask = wxLIST_MASK_TEXT|wxLIST_MASK_DATA;
  item.m_itemId = m_codecOptions->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  item.m_col = 1;
  m_codecOptions->GetItem(item);
  m_codecOptionValue->Enable(!item.m_data);
  if (!item.m_data)
    m_codecOptionValue->SetValue(item.m_text);
}


void OptionsDialog::DeselectedCodecOption(wxListEvent & /*event*/)
{
  m_codecOptionValue->SetValue("");
  m_codecOptionValue->Disable();
}


void OptionsDialog::ChangedCodecOptionValue(wxCommandEvent & /*event*/)
{
  PwxString newValue = m_codecOptionValue->GetValue();

  wxListItem item;
  item.m_itemId = m_codecOptions->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (item.m_itemId < 0)
    return;

  item.m_mask = wxLIST_MASK_TEXT;
  item.m_col = 1;
  m_codecOptions->GetItem(item);

  if (item.m_text == newValue)
    return;

  item.m_text = newValue;
  m_codecOptions->SetItem(item);

  item.m_col = 0;
  m_codecOptions->GetItem(item);

  wxArrayInt selections;
  PAssert(m_selectedCodecs->GetSelections(selections) == 1, PLogicError);
  MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selections[0]);
  PAssert(media != NULL, PLogicError);
  if (media->mediaFormat.SetOptionValue(PwxString(item.m_text), newValue))
    media->dirty = true;
  else
    wxMessageBox("Could not set option to specified value!", "Error", wxCANCEL|wxICON_EXCLAMATION);
}


////////////////////////////////////////
// H.323 fields

void OptionsDialog::SelectedAlias(wxCommandEvent & /*event*/)
{
  m_RemoveAlias->Enable(m_Aliases->GetSelection() != wxNOT_FOUND);
}


void OptionsDialog::ChangedNewAlias(wxCommandEvent & /*event*/)
{

  m_AddAlias->Enable(!m_NewAlias->GetValue().IsEmpty());
}


void OptionsDialog::AddAlias(wxCommandEvent & /*event*/)
{
  m_Aliases->Append(m_NewAlias->GetValue());
}


void OptionsDialog::RemoveAlias(wxCommandEvent & /*event*/)
{
  m_NewAlias->SetValue(m_Aliases->GetStringSelection());
  m_Aliases->Delete(m_Aliases->GetSelection());
  m_RemoveAlias->Disable();
}


////////////////////////////////////////
// SIP fields

void OptionsDialog::FieldsToRegistrar(RegistrarInfo & registrar)
{
  registrar.m_Domain = m_RegistrarDomain->GetValue();
  registrar.m_User = m_RegistrarUser->GetValue();
  registrar.m_Password = m_RegistrarPassword->GetValue();
  registrar.m_TimeToLive = m_RegistrarTimeToLive->GetValue();
  registrar.m_Active = m_RegistrarActive->GetValue();
}


void OptionsDialog::RegistrarToList(bool overwrite, RegistrarInfo * registrar, int position)
{
  if (overwrite)
    m_Registrars->SetItem(position, 0, registrar->m_Domain);
  else {
    position = m_Registrars->InsertItem(position, registrar->m_Domain);
    m_Registrars->SetItemPtrData(position, (wxUIntPtr)registrar);
  }

  m_Registrars->SetItem(position, 1, registrar->m_User);

  wxString str;
  str.sprintf("%u:%02u", registrar->m_TimeToLive/60, registrar->m_TimeToLive%60);
  m_Registrars->SetItem(position, 2, str);

  m_Registrars->SetItem(position, 3, registrar->m_Active ? "ACTIVE" : "disabled");
}


void OptionsDialog::AddRegistrar(wxCommandEvent & event)
{
  RegistrarInfo * registrar = new RegistrarInfo();
  FieldsToRegistrar(*registrar);
  RegistrarToList(false, registrar, m_SelectedRegistrar);
  if (m_SelectedRegistrar != INT_MAX) {
    m_Registrars->SetItemState(m_SelectedRegistrar, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    m_Registrars->SetItemState(m_SelectedRegistrar+1, 0, wxLIST_STATE_SELECTED);
  }
  ChangedRegistrarInfo(event);
}


void OptionsDialog::ChangeRegistrar(wxCommandEvent & event)
{
  RegistrarInfo * registrar = (RegistrarInfo *)m_Registrars->GetItemData(m_SelectedRegistrar);
  FieldsToRegistrar(*registrar);
  RegistrarToList(true, registrar, m_SelectedRegistrar);
  ChangedRegistrarInfo(event);
}


void OptionsDialog::RemoveRegistrar(wxCommandEvent & event)
{
  delete (RegistrarInfo *)m_Registrars->GetItemData(m_SelectedRegistrar);
  m_Registrars->DeleteItem(m_SelectedRegistrar);
  ChangedRegistrarInfo(event);
}


void OptionsDialog::SelectedRegistrar(wxListEvent & event)
{
  m_SelectedRegistrar = event.GetIndex();
  m_ChangeRegistrar->Enable(true);
  m_RemoveRegistrar->Enable(true);

  RegistrarInfo & registrar = *(RegistrarInfo *)m_Registrars->GetItemData(m_SelectedRegistrar);
  m_RegistrarDomain->SetValue(registrar.m_Domain);
  m_RegistrarUser->SetValue(registrar.m_User);
  m_RegistrarPassword->SetValue(registrar.m_Password);
  m_RegistrarTimeToLive->SetValue(registrar.m_TimeToLive);
  m_RegistrarActive->SetValue(registrar.m_Active);

  ChangedRegistrarInfo(event);
}


void OptionsDialog::DeselectedRegistrar(wxListEvent & event)
{
  m_SelectedRegistrar = INT_MAX;
  m_ChangeRegistrar->Enable(false);
  m_RemoveRegistrar->Enable(false);
  ChangedRegistrarInfo(event);
}


void OptionsDialog::ChangedRegistrarInfo(wxCommandEvent & /*event*/)
{
  RegistrarInfo registrar;
  FieldsToRegistrar(registrar);

  bool add = !registrar.m_Domain.IsEmpty() && !registrar.m_User.IsEmpty();
  bool change = add && m_SelectedRegistrar != INT_MAX;

  // Check for uniqueness
  for (int i = 0; i < m_Registrars->GetItemCount(); ++i) {
    if (m_Registrars->GetItemText(i) == registrar.m_Domain) {
      add = false;
      if (i != m_SelectedRegistrar)
        change = false;
      break;
    }
  }

  m_AddRegistrar->Enable(add);
  m_ChangeRegistrar->Enable(change);
}


////////////////////////////////////////
// Routing fields

void OptionsDialog::AddRoute(wxCommandEvent & /*event*/)
{
  int pos = m_Routes->InsertItem(m_SelectedRoute, m_RouteSource->GetValue());
  m_Routes->SetItem(pos, 1, m_RouteDevice->GetValue());
  m_Routes->SetItem(pos, 2, m_RoutePattern->GetValue());
  m_Routes->SetItem(pos, 3, m_RouteDestination->GetValue());
}


void OptionsDialog::RemoveRoute(wxCommandEvent & /*event*/)
{
  wxListItem item;
  item.m_itemId = m_SelectedRoute;
  item.m_mask = wxLIST_MASK_TEXT;
  m_Routes->GetItem(item);
  m_RouteSource->SetValue(item.m_text);
  item.m_col++;
  m_Routes->GetItem(item);
  m_RouteDevice->SetValue(item.m_text);
  item.m_col++;
  m_Routes->GetItem(item);
  m_RoutePattern->SetValue(item.m_text);
  item.m_col++;
  m_Routes->GetItem(item);
  m_RouteDestination->SetValue(item.m_text);

  m_Routes->DeleteItem(m_SelectedRoute);
}


static int MoveRoute(wxListCtrl * routes, int selection, int delta)
{
  wxStringList cols;
  wxListItem item;
  item.m_itemId = selection;
  item.m_mask = wxLIST_MASK_TEXT;
  for (item.m_col = 0; item.m_col < routes->GetColumnCount(); item.m_col++) {
    routes->GetItem(item);
    cols.Add(item.m_text);
  }

  routes->DeleteItem(selection);
  selection += delta;
  routes->InsertItem(selection, cols.front());

  item.m_itemId = selection;
  for (item.m_col = 1; item.m_col < routes->GetColumnCount(); item.m_col++) {
    item.m_text = cols[item.m_col];
    routes->SetItem(item);
  }

  routes->SetItemState(selection, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
  return selection;
}


void OptionsDialog::MoveUpRoute(wxCommandEvent & /*event*/)
{
  m_SelectedRoute = MoveRoute(m_Routes, m_SelectedRoute, -1);
  m_MoveUpRoute->Enable(m_SelectedRoute > 0);
  m_MoveDownRoute->Enable(true);
}


void OptionsDialog::MoveDownRoute(wxCommandEvent & /*event*/)
{
  m_SelectedRoute = MoveRoute(m_Routes, m_SelectedRoute, 1);
  m_MoveUpRoute->Enable(true);
  m_MoveDownRoute->Enable(m_SelectedRoute < (int)m_Routes->GetItemCount()-1);
}


void OptionsDialog::SelectedRoute(wxListEvent & event)
{
  m_SelectedRoute = event.GetIndex();
  m_RemoveRoute->Enable(true);
  m_MoveUpRoute->Enable(m_SelectedRoute > 0);
  m_MoveDownRoute->Enable(m_SelectedRoute < (int)m_Routes->GetItemCount()-1);
}


void OptionsDialog::DeselectedRoute(wxListEvent & /*event*/)
{
  m_SelectedRoute = INT_MAX;
  m_RemoveRoute->Enable(false);
  m_MoveUpRoute->Enable(false);
  m_MoveDownRoute->Enable(false);
}


void OptionsDialog::ChangedRouteInfo(wxCommandEvent & /*event*/)
{
  m_AddRoute->Enable(!m_RoutePattern->GetValue().IsEmpty() && !m_RouteDestination->GetValue().IsEmpty());
}


#if PTRACING
////////////////////////////////////////
// Tracing fields

void OptionsDialog::BrowseTraceFile(wxCommandEvent & /*event*/)
{
  wxString newFile = wxFileSelector("Trace log file",
                                    "",
                                    m_TraceFileName,
                                    ".log",
                                    "Log Files (*.log)|*.log|Text Files (*.txt)|*.txt||",
                                    wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
  if (!newFile.empty()) {
    m_TraceFileName = newFile;
    TransferDataToWindow();
  }
}
#endif



///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CallDialog, wxDialog)
  EVT_BUTTON(XRCID("wxID_OK"), CallDialog::OnOK)
  EVT_TEXT(XRCID("Address"), CallDialog::OnAddressChange)
END_EVENT_TABLE()

CallDialog::CallDialog(wxFrame * parent)
{
  wxXmlResource::Get()->LoadDialog(this, parent, "CallDialog");

  m_ok = FindWindowByNameAs<wxButton>(this, "wxID_OK");
  m_ok->Disable();

  m_AddressCtrl = FindWindowByNameAs<wxComboBox>(this, "Address");
  m_AddressCtrl->SetValidator(wxGenericValidator(&m_Address));

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(RecentCallsGroup);
  wxString entryName;
  long entryIndex;
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      wxString address;
      if (config->Read(entryName, &address))
        m_AddressCtrl->AppendString(address);
    } while (config->GetNextEntry(entryName, entryIndex));
  }
}


void CallDialog::OnOK(wxCommandEvent & event)
{
  wxConfigBase * config = wxConfig::Get();
  config->DeleteGroup(RecentCallsGroup);
  config->SetPath(RecentCallsGroup);

  config->Write("1", m_Address);

  unsigned keyNumber = 1;
  for (size_t i = 0; i < m_AddressCtrl->GetCount() && keyNumber < MaxSavedRecentCalls; ++i) {
    wxString entry = m_AddressCtrl->GetString(i);

    if (m_Address != entry) {
      wxString key;
      key.sprintf("%u", ++keyNumber);
      config->Write(key, entry);
    }
  }

  EndModal(wxID_OK);
}


void CallDialog::OnAddressChange(wxCommandEvent & WXUNUSED(event))
{
  TransferDataFromWindow();
  m_ok->Enable(!m_Address.IsEmpty());
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(AnswerPanel, wxPanel)
  EVT_BUTTON(XRCID("AnswerCall"), AnswerPanel::OnAnswer)
  EVT_BUTTON(XRCID("RejectCall"), AnswerPanel::OnReject)
END_EVENT_TABLE()

AnswerPanel::AnswerPanel(MyManager & manager, wxWindow * parent)
  : m_manager(manager)
{
  wxXmlResource::Get()->LoadPanel(this, parent, "AnswerPanel");
}


void AnswerPanel::OnAnswer(wxCommandEvent & /*event*/)
{
  m_manager.AnswerCall();
}


void AnswerPanel::OnReject(wxCommandEvent & /*event*/)
{
  m_manager.RejectCall();
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CallingPanel, wxPanel)
  EVT_BUTTON(XRCID("HangUpCall"), CallingPanel::OnHangUp)
END_EVENT_TABLE()

CallingPanel::CallingPanel(MyManager & manager, wxWindow * parent)
  : m_manager(manager)
{
  wxXmlResource::Get()->LoadPanel(this, parent, "CallingPanel");
}


void CallingPanel::OnHangUp(wxCommandEvent & /*event*/)
{
  m_manager.HangUpCall();
}


///////////////////////////////////////////////////////////////////////////////

const int VU_UPDATE_TIMER_ID = 1000;

BEGIN_EVENT_TABLE(InCallPanel, wxPanel)
  EVT_BUTTON(XRCID("HangUp"), InCallPanel::OnHangUp)
  EVT_BUTTON(XRCID("Hold"), InCallPanel::OnHold)
  EVT_BUTTON(XRCID("StartStopVideo"), InCallPanel::OnStartStopVideo)
  EVT_CHECKBOX(XRCID("SpeakerMute"), InCallPanel::OnSpeakerMute)
  EVT_CHECKBOX(XRCID("MicrophoneMute"), InCallPanel::OnMicrophoneMute)
  EVT_BUTTON(XRCID("Input1"), InCallPanel::OnUserInput1)
  EVT_BUTTON(XRCID("Input2"), InCallPanel::OnUserInput2)
  EVT_BUTTON(XRCID("Input3"), InCallPanel::OnUserInput3)
  EVT_BUTTON(XRCID("Input4"), InCallPanel::OnUserInput4)
  EVT_BUTTON(XRCID("Input5"), InCallPanel::OnUserInput5)
  EVT_BUTTON(XRCID("Input6"), InCallPanel::OnUserInput6)
  EVT_BUTTON(XRCID("Input7"), InCallPanel::OnUserInput7)
  EVT_BUTTON(XRCID("Input8"), InCallPanel::OnUserInput8)
  EVT_BUTTON(XRCID("Input9"), InCallPanel::OnUserInput9)
  EVT_BUTTON(XRCID("Input0"), InCallPanel::OnUserInput0)
  EVT_BUTTON(XRCID("InputStar"), InCallPanel::OnUserInputStar)
  EVT_BUTTON(XRCID("InputHash"), InCallPanel::OnUserInputHash)
  EVT_BUTTON(XRCID("InputFlash"), InCallPanel::OnUserInputFlash)

  EVT_COMMAND_SCROLL(XRCID("SpeakerVolume"), InCallPanel::SpeakerVolume)
  EVT_COMMAND_SCROLL(XRCID("MicrophoneVolume"), InCallPanel::MicrophoneVolume)

  EVT_TIMER(VU_UPDATE_TIMER_ID, InCallPanel::OnUpdateVU)
END_EVENT_TABLE()


InCallPanel::InCallPanel(MyManager & manager, wxWindow * parent)
  : m_manager(manager)
  , m_vuTimer(this, VU_UPDATE_TIMER_ID)
{
  wxXmlResource::Get()->LoadPanel(this, parent, "InCallPanel");

  m_Hold = FindWindowByNameAs<wxButton>(this, "Hold");
  m_StartStopVideo = FindWindowByNameAs<wxButton>(this, "StartStopVideo");
  m_SpeakerHandset = FindWindowByNameAs<wxButton>(this, "SpeakerHandset");
  m_SpeakerMute = FindWindowByNameAs<wxCheckBox>(this, "SpeakerMute");
  m_MicrophoneMute = FindWindowByNameAs<wxCheckBox>(this, "MicrophoneMute");
  m_SpeakerVolume = FindWindowByNameAs<wxSlider>(this, "SpeakerVolume");
  m_MicrophoneVolume = FindWindowByNameAs<wxSlider>(this, "MicrophoneVolume");
  m_vuSpeaker = FindWindowByNameAs<wxGauge>(this, "SpeakerGauge");
  m_vuMicrophone = FindWindowByNameAs<wxGauge>(this, "MicrophoneGauge");

  m_vuTimer.Start(250);

  m_FirstTime = true;
}


bool InCallPanel::Show(bool show)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AudioGroup);

  if (show || m_FirstTime) {
    m_FirstTime = false;

    int value = 50;
    config->Read(SpeakerVolumeKey, &value);
    m_SpeakerVolume->SetValue(value);
    SetVolume(false, value, false);

    value = 50;
    config->Read(MicrophoneVolumeKey, &value);
    m_MicrophoneVolume->SetValue(value);
    SetVolume(true, value, false);
  }
  else {
    config->Write(SpeakerVolumeKey, m_SpeakerVolume->GetValue());
    config->Write(MicrophoneVolumeKey, m_MicrophoneVolume->GetValue());
  }

  return wxPanel::Show(show);
}


void InCallPanel::UpdateButtons(OpalPOTSEndPoint * potsEP)
{
  // Must do this before getting lock on OpalCall to avoid deadlock
  m_SpeakerHandset->Enable(potsEP->GetLine("*") != NULL);

  PSafePtr<OpalCall> call = m_manager.GetCall();
  if (call != NULL) {
    PSafePtr<OpalConnection> connection;
    for (int connIdx = 0; (connection = call->GetConnection(connIdx)) != NULL; connIdx++) {
      if (!PIsDescendant(&(*connection), OpalPCSSConnection) && !PIsDescendant(&(*connection), OpalLineConnection)) {
        OpalMediaFormatList availableFormats = connection->GetMediaFormats();
        for (PINDEX idx = 0; idx < availableFormats.GetSize(); idx++) {
          if (availableFormats[idx].GetDefaultSessionID() == OpalMediaFormat::DefaultVideoSessionID) {
            m_StartStopVideo->Enable();
            OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaFormat::DefaultVideoSessionID, false);
            m_StartStopVideo->SetLabel(stream != NULL && stream->IsOpen() ? "Stop Video" : "Start Video");
            return;
          }
        }
      }
    }
  }

  m_StartStopVideo->Disable();
}


void InCallPanel::OnHangUp(wxCommandEvent & /*event*/)
{
  m_manager.HangUpCall();
}


void InCallPanel::OnHold(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalCall> call = m_manager.GetCall();
  if (call != NULL) {
    if (call->IsOnHold()) {
      call->Retrieve();
      m_Hold->SetLabel("Hold");
    }
    else {
      call->Hold();
      m_Hold->SetLabel("Retrieve");
    }
  }
}


void InCallPanel::OnStartStopVideo(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalConnection> connection = m_manager.GetUserConnection();
  if (connection != NULL) {
    m_StartStopVideo->Disable();
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaFormat::DefaultVideoSessionID, true);
    if (stream != NULL)
      connection->CloseMediaStream(*stream);
    else {
      if (!connection->GetCall().OpenSourceMediaStreams(*connection, OpalMediaFormat::DefaultVideoSessionID))
        LogWindow << "Could not open video to remote!" << endl;
    }
  }
}


void InCallPanel::OnSpeakerMute(wxCommandEvent & event)
{
  SetVolume(false, m_SpeakerVolume->GetValue(), !event.IsChecked());
}


void InCallPanel::OnMicrophoneMute(wxCommandEvent & event)
{
  SetVolume(true, m_MicrophoneVolume->GetValue(), !event.IsChecked());
}


#define ON_USER_INPUT_HANDLER(i,c) \
  void InCallPanel::OnUserInput##i(wxCommandEvent &) \
  { m_manager.SendUserInput(c); }

ON_USER_INPUT_HANDLER(1,'1')
ON_USER_INPUT_HANDLER(2,'2')
ON_USER_INPUT_HANDLER(3,'3')
ON_USER_INPUT_HANDLER(4,'4')
ON_USER_INPUT_HANDLER(5,'5')
ON_USER_INPUT_HANDLER(6,'6')
ON_USER_INPUT_HANDLER(7,'7')
ON_USER_INPUT_HANDLER(8,'8')
ON_USER_INPUT_HANDLER(9,'9')
ON_USER_INPUT_HANDLER(0,'0')
ON_USER_INPUT_HANDLER(Star,'*')
ON_USER_INPUT_HANDLER(Hash,'#')
ON_USER_INPUT_HANDLER(Flash,'!')


void InCallPanel::SpeakerVolume(wxScrollEvent & event)
{
  SetVolume(false, event.GetPosition(), !m_SpeakerMute->GetValue());
}


void InCallPanel::MicrophoneVolume(wxScrollEvent & event)
{
  SetVolume(true, event.GetPosition(), !m_MicrophoneMute->GetValue());
}


void InCallPanel::SetVolume(bool isMicrophone, int value, bool muted)
{
  PSafePtr<OpalConnection> connection = m_manager.GetUserConnection();
  if (connection != NULL)
    connection->SetAudioVolume(isMicrophone, muted ? 0 : value);
}


static void SetGauge(wxGauge * gauge, int level)
{
  if (level < 0 || level > 32767) {
    gauge->Show(false);
    return;
  }
  gauge->Show();
  gauge->SetValue((int)(100*log10(1.0 + 9.0*level/8192.0))); // Convert to logarithmic scale
}


void InCallPanel::OnUpdateVU(wxTimerEvent& WXUNUSED(event))
{
  if (IsShown()) {
    int micLevel = -1;
    int spkLevel = -1;
    PSafePtr<OpalConnection> connection = m_manager.GetUserConnection();
    if (connection != NULL) {
      spkLevel = connection->GetAudioSignalLevel(false);
      micLevel = connection->GetAudioSignalLevel(true);
    }

    SetGauge(m_vuSpeaker, spkLevel);
    SetGauge(m_vuMicrophone, micLevel);
  }
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(SpeedDialDialog, wxDialog)
  EVT_TEXT(XRCID("SpeedDialName"), SpeedDialDialog::OnChange)
  EVT_TEXT(XRCID("SpeedDialNumber"), SpeedDialDialog::OnChange)
  EVT_TEXT(XRCID("SpeedDialAddress"), SpeedDialDialog::OnChange)
END_EVENT_TABLE()

SpeedDialDialog::SpeedDialDialog(MyManager * manager)
  : m_manager(*manager)
{
  wxXmlResource::Get()->LoadDialog(this, manager, "SpeedDialDialog");

  m_ok = FindWindowByNameAs<wxButton>(this, "wxID_OK");

  m_nameCtrl = FindWindowByNameAs<wxTextCtrl>(this, "SpeedDialName");
  m_nameCtrl->SetValidator(wxGenericValidator(&m_Name));

  m_numberCtrl = FindWindowByNameAs<wxTextCtrl>(this, "SpeedDialNumber");
  m_numberCtrl->SetValidator(wxGenericValidator(&m_Number));

  m_addressCtrl = FindWindowByNameAs<wxTextCtrl>(this, "SpeedDialAddress");
  m_addressCtrl->SetValidator(wxGenericValidator(&m_Address));

  FindWindowByName("SpeedDialDescription")->SetValidator(wxGenericValidator(&m_Description));

  m_inUse = FindWindowByNameAs<wxStaticText>(this, "SpeedDialInUse");
  m_ambiguous = FindWindowByNameAs<wxStaticText>(this, "SpeedDialAmbiguous");
}


void SpeedDialDialog::OnChange(wxCommandEvent & WXUNUSED(event))
{
  wxString newName = m_nameCtrl->GetValue();
  bool inUse = newName != m_Name && m_manager.HasSpeedDialName(newName);
  m_inUse->Show(inUse);

  m_ok->Enable(!inUse && !newName.IsEmpty() && !m_addressCtrl->GetValue().IsEmpty());

  m_ambiguous->Show(m_manager.GetSpeedDialIndex(m_numberCtrl->GetValue(), m_Number) >= 0);
}


///////////////////////////////////////////////////////////////////////////////

MyPCSSEndPoint::MyPCSSEndPoint(MyManager & manager)
  : OpalPCSSEndPoint(manager),
    m_manager(manager)
{
}


PBoolean MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  m_manager.OnRinging(connection);
  return PTrue;

}


PBoolean MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  PTime now;
  LogWindow << connection.GetRemotePartyName() << " is ringing on "
            << now.AsString("w h:mma") << " ..." << endl;
  return PTrue;
}


///////////////////////////////////////////////////////////////////////////////

MyH323EndPoint::MyH323EndPoint(MyManager & manager)
  : H323EndPoint(manager),
    m_manager(manager)
{
}


void MyH323EndPoint::OnRegistrationConfirm()
{
  LogWindow << "H.323 registration successful." << endl;
}


///////////////////////////////////////////////////////////////////////////////

MySIPEndPoint::MySIPEndPoint(MyManager & manager)
  : SIPEndPoint(manager),
    m_manager(manager)
{
}


void MySIPEndPoint::OnRegistrationStatus(const PString & aor,
                                         PBoolean wasRegistering,
                                         PBoolean reRegistering,
                                         SIP_PDU::StatusCodes reason)
{
  if (reason == SIP_PDU::Failure_UnAuthorised || (reRegistering && reason == SIP_PDU::Successful_OK))
    return;

  LogWindow << "SIP " << (wasRegistering ? "" : "un") << "registration of " << aor << ' ';
  switch (reason) {
    case SIP_PDU::Successful_OK :
      LogWindow << "successful";
      break;

    case SIP_PDU::Failure_RequestTimeout :
      LogWindow << "timed out";
      break;

    default :
      LogWindow << "failed (" << reason << ')';
  }
  LogWindow << '.' << endl;
}


// End of File ///////////////////////////////////////////////////////////////
