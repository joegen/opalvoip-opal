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

//#ifdef __GNUG__
//#pragma implementation
//#pragma interface
//#endif

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
#include <wx/checklst.h>
#include <wx/grid.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/valgen.h>
#include <wx/notebook.h>
#undef LoadMenu // Bizarre but necessary before the xml code
#include <wx/xrc/xmlres.h>

#include <opal/mediasession.h>
#include <opal/transcoders.h>
#include <opal/ivr.h>
#include <opal/opalmixer.h>
#include <sip/sippres.h>
#include <lids/lidep.h>
#include <lids/capi_ep.h>
#include <ptclib/pstun.h>
#include <codec/vidcodec.h>
#include <ptclib/pwavfile.h>

#include <algorithm>


#if defined(__WXGTK__)   || \
    defined(__WXMOTIF__) || \
    defined(__WXX11__)   || \
    defined(__WXMAC__)   || \
    defined(__WXMGL__)   || \
    defined(__WXCOCOA__)
#include "openphone.xpm"
#include "unknown16.xpm"
#include "unknown48.xpm"
#include "absent16.xpm"
#include "absent48.xpm"
#include "present16.xpm"
#include "present48.xpm"
#include "busy16.xpm"
#include "busy48.xpm"

#define VIDEO_WINDOW_DRIVER "SDL"
#define VIDEO_WINDOW_DEVICE "SDL"

#else

#define VIDEO_WINDOW_DRIVER "Window"
#define VIDEO_WINDOW_DEVICE "MSWIN STYLE=0x80C80000"  // WS_POPUP|WS_BORDER|WS_SYSMENU|WS_CAPTION

#endif

#if wxUSE_UNICODE
typedef wstringstream tstringstream;
#else
typedef  stringstream tstringstream;
#endif

extern void InitXmlResource(); // From resource.cpp whichis compiled openphone.xrc


#define CONFERENCE_NAME "conference"


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

static const wxChar GeneralGroup[] = wxT("/General");
DEF_FIELD(Username);
DEF_FIELD(DisplayName);
DEF_FIELD(RingSoundDeviceName);
DEF_FIELD(RingSoundFileName);
DEF_FIELD(AutoAnswer);
DEF_FIELD(VendorName);
DEF_FIELD(ProductName);
DEF_FIELD(ProductVersion);
DEF_FIELD(IVRScript);
DEF_FIELD(AudioRecordingMode);
DEF_FIELD(AudioRecordingFormat);
DEF_FIELD(VideoRecordingMode);
DEF_FIELD(VideoRecordingSize);
DEF_FIELD(SpeakerVolume);
DEF_FIELD(MicrophoneVolume);
DEF_FIELD(SpeakerMute);
DEF_FIELD(MicrophoneMute);
DEF_FIELD(LastDialed);
DEF_FIELD(LastReceived);
DEF_FIELD(CurrentSIPConnection);

static const wxChar NetworkingGroup[] = wxT("/Networking");
DEF_FIELD(Bandwidth);
DEF_FIELD(RTPTOS);
DEF_FIELD(MaxRtpPayloadSize);
#if OPAL_PTLIB_SSL
DEF_FIELD(CertificateAuthority);
DEF_FIELD(LocalCertificate);
DEF_FIELD(PrivateKey);
#endif
DEF_FIELD(TCPPortBase);
DEF_FIELD(TCPPortMax);
DEF_FIELD(UDPPortBase);
DEF_FIELD(UDPPortMax);
DEF_FIELD(RTPPortBase);
DEF_FIELD(RTPPortMax);
DEF_FIELD(NATHandling);
DEF_FIELD(NATRouter);
DEF_FIELD(STUNServer);

static const wxChar LocalInterfacesGroup[] = wxT("/Networking/Interfaces");

static const wxChar AudioGroup[] = wxT("/Audio");
DEF_FIELD(SoundPlayer);
DEF_FIELD(SoundRecorder);
DEF_FIELD(SoundBufferTime);
DEF_FIELD(LineInterfaceDevice);
DEF_FIELD(AEC);
DEF_FIELD(Country);
DEF_FIELD(MinJitter);
DEF_FIELD(MaxJitter);
DEF_FIELD(SilenceSuppression);
DEF_FIELD(SilenceThreshold);
DEF_FIELD(SignalDeadband);
DEF_FIELD(SilenceDeadband);
DEF_FIELD(DisableDetectInBandDTMF);

static const wxChar VideoGroup[] = wxT("/Video");
DEF_FIELD(VideoGrabDevice);
DEF_FIELD(VideoGrabFormat);
DEF_FIELD(VideoGrabSource);
DEF_FIELD(VideoGrabFrameRate);
DEF_FIELD(VideoGrabFrameSize);
DEF_FIELD(VideoGrabPreview);
DEF_FIELD(VideoGrabBitRate);
DEF_FIELD(VideoMaxBitRate);
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

static const wxChar FaxGroup[] = wxT("/Fax");
DEF_FIELD(FaxStationIdentifier);
DEF_FIELD(FaxReceiveDirectory);
DEF_FIELD(FaxAutoAnswerMode);

static const wxChar PresenceGroup[] = wxT("/Presence");
DEF_FIELD(PresenceAOR);
DEF_FIELD(LastPresenceState);
static const PConstString PresenceActiveKey("Active");

static const wxChar CodecsGroup[] = wxT("/Codecs");
static const wxChar CodecNameKey[] = wxT("Name");

static const wxChar H323Group[] = wxT("/H.323");
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
DEF_FIELD(ExtendedVideoRoles);
DEF_FIELD(EnableH239Control);
#if OPAL_PTLIB_SSL
DEF_FIELD(H323SignalingSecurity);
DEF_FIELD(H323MediaCryptoSuites);
#endif


static const wxChar H323AliasesGroup[] = wxT("/H.323/Aliases");

static const wxChar SIPGroup[] = wxT("/SIP");
DEF_FIELD(SIPProxyUsed);
DEF_FIELD(SIPProxy);
DEF_FIELD(SIPProxyUsername);
DEF_FIELD(SIPProxyPassword);
DEF_FIELD(LineAppearanceCode);
DEF_FIELD(SIPUserInputMode);
DEF_FIELD(SIPPRACKMode);
#if OPAL_PTLIB_SSL
DEF_FIELD(SIPSignalingSecurity);
DEF_FIELD(SIPMediaCryptoSuites);
#endif

static const wxChar RegistrarGroup[] = wxT("/SIP/Registrars");
DEF_FIELD(RegistrationType);
DEF_FIELD(RegistrarUsed);
DEF_FIELD(RegistrarName);
DEF_FIELD(RegistrarDomain);
DEF_FIELD(RegistrarContact);
DEF_FIELD(RegistrarAuthID);
DEF_FIELD(RegistrarUsername);
DEF_FIELD(RegistrarPassword);
DEF_FIELD(RegistrarProxy);
DEF_FIELD(RegistrarTimeToLive);
DEF_FIELD(RegistrarCompatibility);

static const wxChar RoutingGroup[] = wxT("/Routes");
DEF_FIELD(ForwardingAddress);
DEF_FIELD(ForwardingTimeout);
DEF_FIELD(telURI);

#if PTRACING
static const wxChar TracingGroup[] = wxT("/Tracing");
DEF_FIELD(EnableTracing);
DEF_FIELD(TraceLevelThreshold);
DEF_FIELD(TraceLevelNumber);
DEF_FIELD(TraceFileLine);
DEF_FIELD(TraceBlocks);
DEF_FIELD(TraceDateTime);
DEF_FIELD(TraceTimestamp);
DEF_FIELD(TraceThreadName);
DEF_FIELD(TraceThreadAddress);
DEF_FIELD(TraceObjectInstance);
DEF_FIELD(TraceContextId);
DEF_FIELD(TraceFileName);
DEF_FIELD(TraceOptions);
#endif

static const wxChar SpeedDialsGroup[] = wxT("/Speed Dials");
static const wxChar SpeedDialAddressKey[] = wxT("Address");
static const wxChar SpeedDialStateURLKey[] = wxT("State URL");
static const wxChar SpeedDialPresentityKey[] = wxT("Presentity");
static const wxChar SpeedDialNumberKey[] = wxT("Number");
static const wxChar SpeedDialDescriptionKey[] = wxT("Description");

static const wxChar RecentCallsGroup[] = wxT("/Recent Calls");
static const size_t MaxSavedRecentCalls = 10;

static const wxChar RecentIMCallsGroup[] = wxT("/Recent IM Calls");
static const size_t MaxSavedRecentIMCalls = 10;

static const wxChar SIPonly[] = wxT(" (SIP only)");
static const wxChar H323only[] = wxT(" (H.323 only)");

static const wxChar AllRouteSources[] = wxT("<ALL>");

static const wxChar SpeedDialTabTitle[] = wxT("Speed Dials");


static const wxChar NotAvailableString[] = wxT("N/A");


enum IconStates {
  Icon_Unknown,
  Icon_Unavailable,
  Icon_Present,
  Icon_Busy,
  Icon_Away,
  NumIconStates
};

static const wxChar * const IconStatusNames[NumIconStates] =
{
  wxT("Unknown"),
  wxT("Unavailable"),
  wxT("Online"),
  wxT("Busy"),
  wxT("Away")
};


static const char * const DefaultRoutes[] = {
#if OPAL_IVR
    ".*:#  = ivr:", // A hash from anywhere goes to IVR
#endif
    "pots:.*\\*.*\\*.* = sip:<dn2ip>",
    "pots:.* = sip:<da>",
    "pc:.*   = sip:<da>",

#if OPAL_FAX
    "t38:.* = sip:<da>",
    "fax:.* = sip:<da>",
    ".*:.*\t.*:(fax|329)@.*|(fax|329) = fax:incoming.tif;receive",
    ".*:.*\t.*:(t38|838)@.*|(t38|838) = t38:incoming.tif;receive",
#endif

    "h323:.*  = pots:<dn>",
    "h323:.*  = pc:*",

    "h323s:.* = pots:<dn>",
    "h323s:.* = pc:*",

    "sip:.*   = pots:<dn>",
    "sip:.*   = pc:*",

    "sips:.*  = pots:<dn>",
    "sips:.*  = pc:*"
};


enum {
  ID_RETRIEVE_MENU_BASE = wxID_HIGHEST+1,
  ID_RETRIEVE_MENU_TOP = ID_RETRIEVE_MENU_BASE+999,
  ID_CONFERENCE_MENU_BASE,
  ID_CONFERENCE_MENU_TOP = ID_CONFERENCE_MENU_BASE+999,
  ID_TRANSFER_MENU_BASE,
  ID_TRANSFER_MENU_TOP = ID_TRANSFER_MENU_BASE+999,
  ID_AUDIO_DEVICE_MENU_BASE,
  ID_AUDIO_DEVICE_MENU_TOP = ID_AUDIO_DEVICE_MENU_BASE+99,
  ID_AUDIO_CODEC_MENU_BASE,
  ID_AUDIO_CODEC_MENU_TOP = ID_AUDIO_CODEC_MENU_BASE+99,
  ID_VIDEO_CODEC_MENU_BASE,
  ID_VIDEO_CODEC_MENU_TOP = ID_VIDEO_CODEC_MENU_BASE+99,
  ID_LOG_MESSAGE,
  ID_RINGING,
  ID_ESTABLISHED,
  ID_ON_HOLD,
  ID_CLEARED,
  ID_STREAMS_CHANGED,
  ID_ASYNC_NOTIFICATION
};

DEFINE_EVENT_TYPE(wxEvtLogMessage)
DEFINE_EVENT_TYPE(wxEvtStreamsChanged)
DEFINE_EVENT_TYPE(wxEvtRinging)
DEFINE_EVENT_TYPE(wxEvtEstablished)
DEFINE_EVENT_TYPE(wxEvtOnHold)
DEFINE_EVENT_TYPE(wxEvtCleared)
DEFINE_EVENT_TYPE(wxEvtAsyncNotification)


///////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

template <class cls> cls * FindWindowByNameAs(wxWindow * window, const wxChar * name)
{
  wxWindow * baseChild = window->FindWindow(name);
  if (PAssert(baseChild != NULL, "Windows control not found")) {
    cls * derivedChild = dynamic_cast<cls *>(baseChild);
    if (PAssert(derivedChild != NULL, "Cannot cast window object to selected class"))
      return derivedChild;
  }

  return NULL;
}


void RemoveNotebookPage(wxWindow * window, const wxChar * name)
{
  wxNotebook * book = FindWindowByNameAs<wxNotebook>(window, wxT("OptionsNotebook"));
  for (size_t i = 0; i < book->GetPageCount(); ++i) {
    if (book->GetPageText(i) == name) {
      book->DeletePage(i);
      break;
    }
  }
}

#ifdef _MSC_VER
#pragma warning(default:4100)
#endif


static PwxString AudioDeviceNameToScreen(const PString & name)
{
  PwxString str = name;
  str.Replace(wxT("\t"), wxT(": "));
  return str;
}

static PString AudioDeviceNameFromScreen(const wxString & name)
{
  PwxString str = name;
  str.Replace(wxT(": "), wxT("\t"));
  return str.p_str();
}

static void FillAudioDeviceComboBox(wxItemContainer * list, PSoundChannel::Directions dir)
{
  PStringArray devices = PSoundChannel::GetDeviceNames(dir);
  for (PINDEX i = 0; i < devices.GetSize(); i++)
    list->Append(AudioDeviceNameToScreen(devices[i]));
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
        return false;

      wxCommandEvent theEvent(wxEvtLogMessage, ID_LOG_MESSAGE);
      theEvent.SetEventObject(m_frame);
      PwxString str(wxString::FromUTF8((const char *)buf, (size_t)len));
      theEvent.SetString(str);
      m_frame->GetEventHandler()->AddPendingEvent(theEvent);
      PTRACE(3, "OpenPhone\t" << str);
      return true;
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
  wxConfig::Set(new wxConfig(PwxString(GetName()), PwxString(GetManufacturer())));
}


void OpenPhoneApp::Main()
{
  // Dummy function
}

//////////////////////////////////

bool OpenPhoneApp::OnInit()
{
  // make sure various URL types are registered to this application
  {
    PString urlTypes("sip\nh323\nsips\nh323s");
    PProcess::HostSystemURLHandlerInfo::RegisterTypes(urlTypes, true);
  }

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

  EVT_MENU(wxID_EXIT,                    MyManager::OnMenuQuit)
  EVT_MENU(wxID_ABOUT,                   MyManager::OnMenuAbout)
  EVT_MENU(wxID_PREFERENCES,             MyManager::OnOptions)
  EVT_MENU(XRCID("MenuCall"),            MyManager::OnMenuCall)
  EVT_MENU(XRCID("MenuCallLastDialed"),  MyManager::OnMenuCallLastDialed)
  EVT_MENU(XRCID("MenuCallLastReceived"),MyManager::OnMenuCallLastReceived)
  EVT_MENU(XRCID("CallSpeedDialAudio"),  MyManager::OnCallSpeedDialAudio)
  EVT_MENU(XRCID("CallSpeedDialHandset"),MyManager::OnCallSpeedDialHandset)
  EVT_MENU(XRCID("MenuSendFax"),         MyManager::OnSendFax)
  EVT_MENU(XRCID("SendFaxSpeedDial"),    MyManager::OnSendFaxSpeedDial)
  EVT_MENU(XRCID("MenuAnswer"),          MyManager::OnMenuAnswer)
  EVT_MENU(XRCID("MenuHangUp"),          MyManager::OnMenuHangUp)
  EVT_MENU(XRCID("NewSpeedDial"),        MyManager::OnNewSpeedDial)
  EVT_MENU(XRCID("ViewLarge"),           MyManager::OnViewLarge)
  EVT_MENU(XRCID("ViewSmall"),           MyManager::OnViewSmall)
  EVT_MENU(XRCID("ViewList"),            MyManager::OnViewList)
  EVT_MENU(XRCID("ViewDetails"),         MyManager::OnViewDetails)
  EVT_MENU(XRCID("EditSpeedDial"),       MyManager::OnEditSpeedDial)
  EVT_MENU(XRCID("MenuCut"),             MyManager::OnCutSpeedDial)
  EVT_MENU(XRCID("MenuCopy"),            MyManager::OnCopySpeedDial)
  EVT_MENU(XRCID("MenuPaste"),           MyManager::OnPasteSpeedDial)
  EVT_MENU(XRCID("MenuDelete"),          MyManager::OnDeleteSpeedDial)
  EVT_MENU(XRCID("MenuHold"),            MyManager::OnRequestHold)
  EVT_MENU_RANGE(ID_RETRIEVE_MENU_BASE,ID_RETRIEVE_MENU_TOP, MyManager::OnRetrieve)
  EVT_MENU(XRCID("MenuTransfer"),        MyManager::OnTransfer)
  EVT_MENU_RANGE(ID_TRANSFER_MENU_BASE,ID_TRANSFER_MENU_TOP, MyManager::OnTransfer)
  EVT_MENU_RANGE(ID_CONFERENCE_MENU_BASE,ID_CONFERENCE_MENU_TOP, MyManager::OnConference)
  EVT_MENU(XRCID("MenuStartRecording"),  MyManager::OnStartRecording)
  EVT_MENU(XRCID("MenuStopRecording"),   MyManager::OnStopRecording)
  EVT_MENU(XRCID("MenuSendAudioFile"),   MyManager::OnSendAudioFile)
  EVT_MENU(XRCID("MenuAudioDevice"),     MyManager::OnAudioDevicePair)
  EVT_MENU_RANGE(ID_AUDIO_DEVICE_MENU_BASE, ID_AUDIO_DEVICE_MENU_TOP, MyManager::OnAudioDevicePreset)
  EVT_MENU_RANGE(ID_AUDIO_CODEC_MENU_BASE, ID_AUDIO_CODEC_MENU_TOP, MyManager::OnNewCodec)
  EVT_MENU_RANGE(ID_VIDEO_CODEC_MENU_BASE, ID_VIDEO_CODEC_MENU_TOP, MyManager::OnNewCodec)
  EVT_MENU(XRCID("MenuStartVideo"),      MyManager::OnStartVideo)
  EVT_MENU(XRCID("MenuStopVideo"),       MyManager::OnStopVideo)
  EVT_MENU(XRCID("MenuSendVFU"),         MyManager::OnSendVFU)
  EVT_MENU(XRCID("MenuSendIntra"),       MyManager::OnSendIntra)
  EVT_MENU(XRCID("MenuTxVideoControl"),  MyManager::OnTxVideoControl)
  EVT_MENU(XRCID("MenuRxVideoControl"),  MyManager::OnRxVideoControl)
  EVT_MENU(XRCID("MenuDefVidWinPos"),    MyManager::OnDefVidWinPos)

  EVT_MENU(XRCID("MenuPresence"),        MyManager::OnMyPresence)
#if OPAL_HAS_IM
  EVT_MENU(XRCID("MenuStartIM"),         MyManager::OnStartIM)
  EVT_MENU(XRCID("MenuInCallMessage"),   MyManager::OnInCallIM)
  EVT_MENU(XRCID("SendIMSpeedDial"),     MyManager::OnSendIMSpeedDial)
#endif // OPAL_HAS_IM

  EVT_SPLITTER_SASH_POS_CHANGED(SplitterID, MyManager::OnSashPositioned)
  EVT_LIST_ITEM_ACTIVATED(SpeedDialsID, MyManager::OnSpeedDialActivated)
  EVT_LIST_COL_END_DRAG(SpeedDialsID, MyManager::OnSpeedDialColumnResize)
  EVT_LIST_ITEM_RIGHT_CLICK(SpeedDialsID, MyManager::OnSpeedDialRightClick) 
  EVT_LIST_END_LABEL_EDIT(SpeedDialsID, MyManager::OnSpeedDialEndEdit)

  EVT_COMMAND(ID_LOG_MESSAGE,         wxEvtLogMessage,         MyManager::OnLogMessage)
  EVT_COMMAND(ID_RINGING,             wxEvtRinging,            MyManager::OnEvtRinging)
  EVT_COMMAND(ID_ESTABLISHED,         wxEvtEstablished,        MyManager::OnEvtEstablished)
  EVT_COMMAND(ID_ON_HOLD,             wxEvtOnHold,             MyManager::OnEvtOnHold)
  EVT_COMMAND(ID_CLEARED,             wxEvtCleared,            MyManager::OnEvtCleared)
  EVT_COMMAND(ID_STREAMS_CHANGED,     wxEvtStreamsChanged,     MyManager::OnStreamsChanged)
  EVT_COMMAND(ID_ASYNC_NOTIFICATION,  wxEvtAsyncNotification,  MyManager::OnEvtAsyncNotification)
  
END_EVENT_TABLE()


MyManager::MyManager()
  : wxFrame(NULL, -1, wxT("OpenPhone"), wxDefaultPosition, wxSize(640, 480))
#if OPAL_FAX
  , m_currentAnswerMode(AnswerDetect)
  , m_defaultAnswerMode(AnswerDetect)
#endif // OPAL_FAX
  , m_speedDials(NULL)
  , pcssEP(NULL)
  , potsEP(NULL)
#if OPAL_H323
  , h323EP(NULL)
#endif
#if OPAL_SIP
  , sipEP(NULL)
#endif
#if OPAL_CAPI
  , capiEP(NULL)
#endif
#if OPAL_IVR
  , ivrEP(NULL)
#endif
#if OPAL_HAS_MIXER
  , m_mcuEP(NULL)
#endif
  , m_autoAnswer(false)
  , m_ForwardingTimeout(30)
  , m_VideoGrabPreview(true)
  , m_localVideoFrameX(INT_MIN)
  , m_localVideoFrameY(INT_MIN)
  , m_remoteVideoFrameX(INT_MIN)
  , m_remoteVideoFrameY(INT_MIN)
  , m_primaryVideoGrabber(NULL)
  , m_SecondaryVideoGrabPreview(false)
  , m_SecondaryVideoOpening(false)
  , m_VideoMaxBitRate(1024)
  , m_VideoTargetBitRate(256)
  , m_ExtendedVideoRoles(e_DisabledExtendedVideoRoles)
#if PTRACING
  , m_enableTracing(false)
#endif
{
  // Give it an icon
  SetIcon(wxICON(AppIcon));

  // Make an image list containing large icons
  m_imageListSmall = new wxImageList(16, 16, true);
  m_imageListNormal = new wxImageList(48, 48, true);

  // Order here is important!! Must be same as IconStates enum
  m_imageListSmall ->Add(wxICON(unknown16));
  m_imageListNormal->Add(wxICON(unknown48));
  m_imageListSmall ->Add(wxICON(absent16));
  m_imageListNormal->Add(wxICON(absent48));
  m_imageListSmall ->Add(wxICON(present16));
  m_imageListNormal->Add(wxICON(present48));
  m_imageListSmall ->Add(wxICON(busy16));
  m_imageListNormal->Add(wxICON(busy48));
  m_imageListSmall ->Add(wxICON(absent16));
  m_imageListNormal->Add(wxICON(absent48));

  m_RingSoundTimer.SetNotifier(PCREATE_NOTIFIER(OnRingSoundAgain));
  m_ForwardingTimer.SetNotifier(PCREATE_NOTIFIER(OnForwardingTimeout));

  LogWindow.SetFrame(this);
}


MyManager::~MyManager()
{
  LogWindow.SetFrame(NULL);

  ShutDownEndpoints();

  delete m_primaryVideoGrabber;

  wxMenuBar * menubar = GetMenuBar();
  SetMenuBar(NULL);
  delete menubar;

  delete m_imageListNormal;
  delete m_imageListSmall;

  delete wxXmlResource::Set(NULL);
}


void MyManager::PostEvent(const wxEventType & type, unsigned id, const PString & str, const void * data)
{
  wxCommandEvent theEvent(type, id);
  theEvent.SetEventObject(this);
  theEvent.SetString(PwxString(str));
  theEvent.SetClientData((void *)data);
  GetEventHandler()->AddPendingEvent(theEvent);
}


void MyManager::AsyncNotifierSignal()
{
  PostEvent(wxEvtAsyncNotification, ID_ASYNC_NOTIFICATION);
}


void MyManager::OnEvtAsyncNotification(wxCommandEvent &)
{
  while (AsyncNotifierExecute())
    ;
}


bool MyManager::Initialise()
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
      wxAcceleratorEntry(wxACCEL_CTRL,  'E',         XRCID("EditSpeedDial")),
      wxAcceleratorEntry(wxACCEL_CTRL,  'X',         XRCID("MenuCut")),
      wxAcceleratorEntry(wxACCEL_CTRL,  'C',         XRCID("MenuCopy")),
      wxAcceleratorEntry(wxACCEL_CTRL,  'V',         XRCID("MenuPaste")),
      wxAcceleratorEntry(wxACCEL_NORMAL, WXK_DELETE, XRCID("MenuDelete"))
  };
  wxAcceleratorTable accel(PARRAYSIZE(accelEntries), accelEntries);
  SetAcceleratorTable(accel);

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(PwxString(AppearanceGroup));

  wxPoint initalPosition = wxDefaultPosition;
  if (config->Read(MainFrameXKey, &initalPosition.x) && config->Read(MainFrameYKey, &initalPosition.y))
    Move(initalPosition);

  wxSize initialSize(512, 384);
  if (config->Read(MainFrameWidthKey, &initialSize.x) && config->Read(MainFrameHeightKey, &initialSize.y))
    SetSize(initialSize);

  // Make the content of the main window, speed dial and log panes inside a splitter
  m_splitter = new wxSplitterWindow(this, SplitterID, wxPoint(), initialSize, wxSP_3D);

  // Create notebook for tabs in top half of splitter
  m_tabs = new wxNotebook(m_splitter, TabsID);

  // Log window - gets informative text - bottom half of splitter
  m_logWindow = new wxTextCtrl(m_splitter, -1, wxEmptyString, wxPoint(), wxSize(initialSize.x, 128), wxTE_MULTILINE | wxTE_DONTWRAP | wxSUNKEN_BORDER);
  m_logWindow->SetBackgroundColour(*wxBLACK);
  m_logWindow->SetForegroundColour(*wxGREEN);

  // Set up sizer to automatically resize the splitter to size of window
  wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(m_splitter, 1, wxGROW, 0);
  SetSizer(sizer);

  int width = initialSize.y/2; // Sash at half way by default
  config->Read(SashPositionKey, &width);
  m_splitter->SplitHorizontally(m_tabs, m_logWindow, width);

  // Read the speed dials from the configuration
  config->SetPath(SpeedDialsGroup);
  SpeedDialInfo info;
  long groupIndex;
  if (config->GetFirstGroup(info.m_Name, groupIndex)) {
    do {
      config->SetPath(info.m_Name);
      if (config->Read(SpeedDialAddressKey, &info.m_Address) && !info.m_Address.empty()) {
        config->Read(SpeedDialNumberKey, &info.m_Number);
        if (!config->Read(SpeedDialPresentityKey, &info.m_Presentity))
          config->Read(SpeedDialStateURLKey, &info.m_Presentity);
        config->Read(SpeedDialDescriptionKey, &info.m_Description);
        m_speedDialInfo.insert(info);
      }
      config->SetPath(wxT(".."));
    } while (config->GetNextGroup(info.m_Name, groupIndex));
  }

  // Speed dial window - icons for each speed dial
  int view;
  config->SetPath(AppearanceGroup);
  if (!config->Read(ActiveViewKey, &view) || view < 0 || view >= e_NumViews)
    view = e_ViewDetails;
  static const wxChar * const ViewMenuNames[e_NumViews] = {
    wxT("ViewLarge"), wxT("ViewSmall"), wxT("ViewList"), wxT("ViewDetails")
  };
  menubar->Check(wxXmlResource::GetXRCID(ViewMenuNames[view]), true);
  RecreateSpeedDials((SpeedDialViews)view);

  // Show the frame window
  Show(true);

  LogWindow << PProcess::Current().GetName()
            << " Version " << PProcess::Current().GetVersion(TRUE)
            << " by " << PProcess::Current().GetManufacturer()
            << " on " << PProcess::Current().GetOSClass() << ' ' << PProcess::Current().GetOSName()
            << " (" << PProcess::Current().GetOSVersion() << '-' << PProcess::Current().GetOSHardware() << ')' << endl;

  m_ClipboardFormat.SetId(wxT("OpenPhone Speed Dial"));

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
    PTrace::Initialise(traceLevelThreshold, m_traceFileName.ToUTF8(), traceOptions);
  }
#endif

  ////////////////////////////////////////
  // Creating the endpoints
#if OPAL_H323
  h323EP = new MyH323EndPoint(*this);
#endif

#if OPAL_SIP
  sipEP = new MySIPEndPoint(*this);
#endif

#if OPAL_CAPI
  capiEP = new OpalCapiEndPoint(*this);
#endif

#if OPAL_IVR
  ivrEP = new OpalIVREndPoint(*this);
#endif

#if OPAL_HAS_MIXER
  m_mcuEP = new OpalMixerEndPoint(*this, "mcu");
  m_mcuEP->AddNode(new OpalMixerNodeInfo(CONFERENCE_NAME));
#endif

#if OPAL_FAX
  m_faxEP = new MyFaxEndPoint(*this);
#endif

#if OPAL_HAS_IM
  m_imEP = new OpalIMEndPoint(*this);
#endif

  potsEP = new OpalLineEndPoint(*this);
  pcssEP = new MyPCSSEndPoint(*this);

  PwxString str;
  bool onoff;
  int value1 = 0, value2 = 0;

  ////////////////////////////////////////
  // General fields
  config->SetPath(GeneralGroup);
  if (config->Read(UsernameKey, &str) && !str.IsEmpty())
    SetDefaultUserName(str);
  if (config->Read(DisplayNameKey, &str))
    SetDefaultDisplayName(str);

  if (!config->Read(RingSoundDeviceNameKey, &m_RingSoundDeviceName))
    m_RingSoundDeviceName = PSoundChannel::GetDefaultDevice(PSoundChannel::Player);
  config->Read(RingSoundFileNameKey, &m_RingSoundFileName);

  config->Read(AutoAnswerKey, &m_autoAnswer);
  config->Read(LastDialedKey, &m_LastDialed);


  OpalProductInfo productInfo = GetProductInfo();
  if (config->Read(VendorNameKey, &str))
    productInfo.vendor = str.p_str();
  if (config->Read(ProductNameKey, &str) && !str.IsEmpty())
    productInfo.name = str.p_str();
  if (config->Read(ProductVersionKey, &str))
    productInfo.version = str.p_str();
  SetProductInfo(productInfo);

#if OPAL_IVR
  if (config->Read(IVRScriptKey, &str))
    ivrEP->SetDefaultVXML(str);
#endif

  config->Read(AudioRecordingModeKey, &m_recordingOptions.m_stereo);
  if (config->Read(AudioRecordingFormatKey, &str))
    m_recordingOptions.m_audioFormat = str.p_str();
  value1 = m_recordingOptions.m_videoMixing;
  if (config->Read(VideoRecordingModeKey, &value1) && value1 >= 0 && value1 < OpalRecordManager::NumVideoMixingModes)
    m_recordingOptions.m_videoMixing = (OpalRecordManager::VideoMode)value1;
  if (config->Read(VideoRecordingSizeKey, &str))
    PVideoFrameInfo::ParseSize(str, m_recordingOptions.m_videoWidth, m_recordingOptions.m_videoHeight);

  ////////////////////////////////////////
  // Networking fields
  PIPSocket::InterfaceTable interfaceTable;
  if (PIPSocket::GetInterfaceTable(interfaceTable))
    LogWindow << "Detected " << interfaceTable.GetSize() << " network interfaces:\n"
              << setfill('\n') << interfaceTable << setfill(' ') << flush;

  config->SetPath(NetworkingGroup);
#if OPAL_H323
  if (config->Read(BandwidthKey, &value1))
    h323EP->SetInitialBandwidth(value1);
#endif
  if (config->Read(RTPTOSKey, &value1))
    SetMediaTypeOfService(value1);
  if (config->Read(MaxRtpPayloadSizeKey, &value1))
    SetMaxRtpPayloadSize(value1);
#if OPAL_PTLIB_SSL
  if (config->Read(CertificateAuthorityKey, &str))
    SetSSLCertificateAuthorityFile(str);
  if (config->Read(LocalCertificateKey, &str))
    SetSSLCertificateFile(str);
  if (config->Read(PrivateKeyKey, &str))
    SetSSLPrivateKeyFile(str);
#endif
  if (config->Read(TCPPortBaseKey, &value1) && config->Read(TCPPortMaxKey, &value2))
    SetTCPPorts(value1, value2);
  if (config->Read(UDPPortBaseKey, &value1) && config->Read(UDPPortMaxKey, &value2))
    SetUDPPorts(value1, value2);
  if (config->Read(RTPPortBaseKey, &value1) && config->Read(RTPPortMaxKey, &value2))
    SetRtpIpPorts(value1, value2);
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
      PwxString localInterface;
      if (config->Read(entryName, &localInterface) && !localInterface.empty())
        m_LocalInterfaces.push_back(localInterface);
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
  if (config->Read(SoundBufferTimeKey, &value1))
    pcssEP->SetSoundChannelBufferTime(value1);

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
    silenceParams.m_signalDeadband = value1;
  if (config->Read(SilenceDeadbandKey, &value1))
    silenceParams.m_silenceDeadband = value1;
  SetSilenceDetectParams(silenceParams);

  if (config->Read(DisableDetectInBandDTMFKey, &onoff))
    DisableDetectInBandDTMF(onoff);

  UpdateAudioDevices();
  StartLID();


  ////////////////////////////////////////
  // Video fields
  config->SetPath(VideoGroup);
  PVideoDevice::OpenArgs videoArgs = GetVideoInputDevice();
  if (config->Read(VideoGrabDeviceKey, &str))
    videoArgs.deviceName = str.p_str();
  if (config->Read(VideoGrabFormatKey, &value1) && value1 >= 0 && value1 < PVideoDevice::NumVideoFormats)
    videoArgs.videoFormat = (PVideoDevice::VideoFormat)value1;
  if (config->Read(VideoGrabSourceKey, &value1))
    videoArgs.channelNumber = value1;
  if (config->Read(VideoGrabFrameRateKey, &value1))
    videoArgs.rate = value1;
  if (videoArgs.rate == 0)
    videoArgs.rate = 15;
  config->Read(VideoFlipLocalKey, &videoArgs.flip);
  SetVideoInputDevice(videoArgs);
  m_SecondaryVideoGrabber = videoArgs;

  m_primaryVideoGrabber = PVideoInputDevice::CreateOpenedDevice(videoArgs, true);
  if (m_primaryVideoGrabber != NULL)
    m_primaryVideoGrabber->Stop();

  config->Read(VideoGrabFrameSizeKey, &m_VideoGrabFrameSize,  wxT("CIF"));
  config->Read(VideoMinFrameSizeKey,  &m_VideoMinFrameSize, wxT("SQCIF"));
  config->Read(VideoMaxFrameSizeKey,  &m_VideoMaxFrameSize,   wxT("CIF16"));
  config->Read(VideoGrabBitRateKey, &m_VideoTargetBitRate);
  config->Read(VideoMaxBitRateKey, &m_VideoMaxBitRate);

  config->Read(VideoGrabPreviewKey, &m_VideoGrabPreview);
  if (config->Read(VideoAutoTransmitKey, &onoff))
    SetAutoStartTransmitVideo(onoff);
  if (config->Read(VideoAutoReceiveKey, &onoff))
    SetAutoStartReceiveVideo(onoff);

  videoArgs = GetVideoPreviewDevice();
  videoArgs.driverName = VIDEO_WINDOW_DRIVER;
  SetVideoPreviewDevice(videoArgs);

  videoArgs = GetVideoOutputDevice();
  videoArgs.driverName = VIDEO_WINDOW_DRIVER;
  config->Read(VideoFlipRemoteKey, &videoArgs.flip);
  SetVideoOutputDevice(videoArgs);

  config->Read(LocalVideoFrameXKey, &m_localVideoFrameX);
  config->Read(LocalVideoFrameYKey, &m_localVideoFrameY);
  config->Read(RemoteVideoFrameXKey, &m_remoteVideoFrameX);
  config->Read(RemoteVideoFrameYKey, &m_remoteVideoFrameY);

  ////////////////////////////////////////
  // Fax fields
#if OPAL_FAX
  config->SetPath(FaxGroup);
  if (config->Read(FaxStationIdentifierKey, &str))
    m_faxEP->SetDefaultDisplayName(str);
  if (config->Read(FaxReceiveDirectoryKey, &str))
    m_faxEP->SetDefaultDirectory(str);
  if (config->Read(FaxAutoAnswerModeKey, &value1) && value1 >= AnswerVoice && value1 <= AnswerDetect)
    m_defaultAnswerMode = (FaxAnswerModes)value1;
#endif

  ////////////////////////////////////////
  // Presence fields
  config->SetPath(PresenceGroup);
  int presIndex = 0;
  for (;;) {
    wxString groupName;
    groupName.sprintf(wxT("%04u"), presIndex);
    if (!config->HasGroup(groupName))
      break;

    config->SetPath(groupName);
    PwxString aor;
    if (config->Read(PresenceAORKey, &aor) && !aor.empty()) {
      PSafePtr<OpalPresentity> presentity = AddPresentity(aor);
      if (presentity != NULL) {
        presentity->GetAttributes().Set(PresenceActiveKey, "Yes"); // OpenPhone extra attribute
        presentity->SetPresenceChangeNotifier(PCREATE_PresenceChangeNotifier(OnPresenceChange));
        long idx;
        PwxString name;
        if (config->GetFirstEntry(name, idx)) {
          do {
            if (name != LastPresenceStateKey)
              presentity->GetAttributes().Set(name.p_str(), PwxString(config->Read(name)));
          } while (config->GetNextEntry(name, idx));
        }
        if (presentity->GetAttributes().GetBoolean(PresenceActiveKey)) {
          LogWindow << (presentity->Open() ? "Establishing" : "Could not establish")
                    << " presence for identity " << aor << endl;
          presentity->SetLocalPresence((OpalPresenceInfo::State)config->Read(LastPresenceStateKey,
                                                               (long)OpalPresenceInfo::Available));
        }
      }
    }

    config->SetPath(wxT(".."));
    presIndex++;
  }

  ////////////////////////////////////////
  // Codec fields
  {
    OpalMediaFormatList mediaFormats;
    mediaFormats += pcssEP->GetMediaFormats();
    mediaFormats += potsEP->GetMediaFormats();
#if OPAL_IVR
    mediaFormats += ivrEP->GetMediaFormats();
#endif
#if OPAL_FAX
    mediaFormats += m_faxEP->GetMediaFormats();
#endif

    OpalMediaFormatList possibleFormats = OpalTranscoder::GetPossibleFormats(mediaFormats);
    for (OpalMediaFormatList::iterator format = possibleFormats.begin(); format != possibleFormats.end(); ++format) {
      if (format->IsTransportable())
        m_mediaInfo.push_back(MyMedia(*format));
    }
  }

  config->SetPath(CodecsGroup);
  int codecIndex = 0;
  for (;;) {
    wxString groupName;
    groupName.sprintf(wxT("%04u"), codecIndex);
    if (!config->HasGroup(groupName))
      break;

    config->SetPath(groupName);
    PwxString codecName;
    if (config->Read(CodecNameKey, &codecName) && !codecName.empty()) {
      for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
        if (codecName == mm->mediaFormat) {
          mm->preferenceOrder = codecIndex;
          for (PINDEX i = 0; i < mm->mediaFormat.GetOptionCount(); i++) {
            const OpalMediaOption & option = mm->mediaFormat.GetOption(i);
            if (!option.IsReadOnly()) {
              PwxString codecOptionName = option.GetName();
              PwxString codecOptionValue;
              PString oldOptionValue;
              mm->mediaFormat.GetOptionValue(codecOptionName, oldOptionValue);
              if (config->Read(codecOptionName, &codecOptionValue) &&
                              !codecOptionValue.empty() && codecOptionValue != oldOptionValue)
                mm->mediaFormat.SetOptionValue(codecOptionName, codecOptionValue);
            }
          }
        }
      }
    }
    config->SetPath(wxT(".."));
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

#if PTRACING
  if (PTrace::CanTrace(4)) {
    OpalMediaFormatList mediaFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
    ostream & traceStream = PTrace::Begin(4, __FILE__, __LINE__, this);
    traceStream << "OpenPhone\tRegistered media formats:\n";
    for (PINDEX i = 0; i < mediaFormats.GetSize(); i++)
      mediaFormats[i].PrintOptions(traceStream);
    traceStream << PTrace::End;
  }
#endif

  PwxString username, password;

#if OPAL_H323
  ////////////////////////////////////////
  // H.323 fields
  config->SetPath(H323AliasesGroup);
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      PwxString alias;
      if (config->Read(entryName, &alias) && !alias.empty())
        h323EP->AddAliasName(alias);
    } while (config->GetNextEntry(entryName, entryIndex));
  }

  config->SetPath(H323Group);

  if (config->Read(DTMFSendModeKey, &value1) && value1 >= 0 && value1 < H323Connection::NumSendUserInputModes)
    h323EP->SetSendUserInputMode((H323Connection::SendUserInputModes)value1);
#if OPAL_450
  if (config->Read(CallIntrusionProtectionLevelKey, &value1))
    h323EP->SetCallIntrusionProtectionLevel(value1);
#endif
  if (config->Read(DisableFastStartKey, &onoff))
    h323EP->DisableFastStart(onoff);
  if (config->Read(DisableH245TunnelingKey, &onoff))
    h323EP->DisableH245Tunneling(onoff);
  if (config->Read(DisableH245inSETUPKey, &onoff))
    h323EP->DisableH245inSetup(onoff);

#if OPAL_H239
  value1 = m_ExtendedVideoRoles;
  config->Read(ExtendedVideoRolesKey, &value1);
  m_ExtendedVideoRoles = (ExtendedVideoRoles)value1;
  h323EP->SetDefaultH239Control(config->Read(EnableH239ControlKey, h323EP->GetDefaultH239Control()));
#endif

#if OPAL_PTLIB_SSL
  if (!config->Read(H323SignalingSecurityKey, &value1))
    value1 = 0;
  SetSignalingSecurity(h323EP, value1, "h323", "h323s");
  if (config->Read(H323MediaCryptoSuitesKey, &str))
    h323EP->SetMediaCryptoSuites(str.p_str().Tokenise(','));
#endif

  config->Read(GatekeeperModeKey, &m_gatekeeperMode, 0);
  if (m_gatekeeperMode > 0) {
    if (config->Read(GatekeeperTTLKey, &value1))
      h323EP->SetGatekeeperTimeToLive(PTimeInterval(0, value1));

    config->Read(GatekeeperLoginKey, &username, wxT(""));
    config->Read(GatekeeperPasswordKey, &password, wxT(""));
    h323EP->SetGatekeeperPassword(password, username);

    config->Read(GatekeeperAddressKey, &m_gatekeeperAddress, wxT(""));
    config->Read(GatekeeperIdentifierKey, &m_gatekeeperIdentifier, wxT(""));
    if (!StartGatekeeper())
      return false;
  }
#endif

#if OPAL_SIP
  ////////////////////////////////////////
  // SIP fields
  config->SetPath(GeneralGroup);
  if (config->Read(CurrentSIPConnectionKey, &str)) {
    if (sipEP->ClearDialogContext(str))
      config->DeleteEntry(CurrentSIPConnectionKey);
  }

  config->SetPath(SIPGroup);
  const SIPURL & proxy = sipEP->GetProxy();
  PwxString hostname;
  config->Read(SIPProxyUsedKey, &m_SIPProxyUsed, false);
  config->Read(SIPProxyKey, &hostname, PwxString(proxy.GetHostName()));
  config->Read(SIPProxyUsernameKey, &username, PwxString(proxy.GetUserName()));
  config->Read(SIPProxyPasswordKey, &password, PwxString(proxy.GetPassword()));
  if (m_SIPProxyUsed)
    sipEP->SetProxy(hostname, username, password);

  if (config->Read(LineAppearanceCodeKey, &value1))
    sipEP->SetDefaultAppearanceCode(value1);

  if (config->Read(SIPUserInputModeKey, &value1) && value1 >= 0 && value1 < H323Connection::NumSendUserInputModes)
    sipEP->SetSendUserInputMode((OpalConnection::SendUserInputModes)value1);

  if (config->Read(SIPPRACKModeKey, &value1))
    sipEP->SetDefaultPRACKMode((SIPConnection::PRACKMode)value1);

#if OPAL_PTLIB_SSL
  if (!config->Read(SIPSignalingSecurityKey, &value1))
    value1 = 0;
  SetSignalingSecurity(sipEP, value1, "sip", "sips");
  if (config->Read(SIPMediaCryptoSuitesKey, &str))
    sipEP->SetMediaCryptoSuites(str.p_str().Tokenise(','));
#endif

  if (config->Read(RegistrarTimeToLiveKey, &value1))
    sipEP->SetRegistrarTimeToLive(PTimeInterval(0, value1));

  // Original backward compatibility entry
  RegistrationInfo registration;
  if (config->Read(RegistrarUsedKey, &registration.m_Active, false) &&
      config->Read(RegistrarNameKey, &registration.m_Domain) &&
      config->Read(RegistrarUsernameKey, &registration.m_User) &&
      config->Read(RegistrarPasswordKey, &registration.m_Password) &&
      config->Read(RegistrarProxyKey, &registration.m_Proxy))
    m_registrations.push_back(registration);

  config->SetPath(RegistrarGroup);
  wxString groupName;
  if (config->GetFirstGroup(groupName, groupIndex)) {
    do {
      config->SetPath(groupName);
      if (registration.Read(*config))
        m_registrations.push_back(registration);
      config->SetPath(wxT(".."));
    } while (config->GetNextGroup(groupName, groupIndex));
  }

  StartRegistrations();

  int count = m_speedDials->GetItemCount();
  for (int i = 0; i < count; i++) {
    SpeedDialInfo * info = (SpeedDialInfo *)m_speedDials->GetItemData(i);
    if (info != NULL && MonitorPresence(info->m_Presentity, info->m_Address, true))
      m_speedDials->SetItem(i, e_StatusColumn, IconStatusNames[Icon_Unknown]);
  }
#endif // OPAL_SIP


#if OPAL_CAPI
  unsigned capiCount = capiEP->OpenControllers();
  if (capiCount > 0) {
    LogWindow << "ISDN (CAPI) detected " << capiCount << " controller";
    if (capiCount > 1)
      LogWindow << 's';
    LogWindow << endl;
  }
#endif

  ////////////////////////////////////////
  // Routing fields
  config->SetPath(GeneralGroup);
  config->Read(ForwardingAddressKey, &m_ForwardingAddress);
  config->Read(ForwardingTimeoutKey, &m_ForwardingTimeout);

  if (config->Read(telURIKey, &str) && !str.empty()) {
    OpalEndPoint * ep = FindEndPoint(str);
    if (ep != NULL)
      AttachEndPoint(ep, "tel");
  }

  config->SetPath(RoutingGroup);
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      PwxString routeSpec;
      if (config->Read(entryName, &routeSpec))
        AddRouteEntry(routeSpec);
    } while (config->GetNextEntry(entryName, entryIndex));
  }
  else {
    for (PINDEX i = 0; i < PARRAYSIZE(DefaultRoutes); i++)
      AddRouteEntry(DefaultRoutes[i]);
  }

  AdjustVideoFormats();

#if OPAL_HAS_IM
  m_imEP->AddNotifier(PCREATE_ConversationNotifier(OnConversation), "*");
#endif

  LogWindow << "Ready ..." << endl;
  return true;
}


void MyManager::StartLID()
{
  wxConfigBase * config = wxConfig::Get();

  PwxString device;
  if (!config->Read(LineInterfaceDeviceKey, &device) ||
      device.IsEmpty() || (device.StartsWith(wxT("<<")) && device.EndsWith(wxT(">>"))))
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

        if (!SetNATServer("STUN", m_STUNServer))
          LogWindow << "STUN server offline or unsuitable NAT type";
        else {
          LogWindow << "STUN server \"" << m_natMethod->GetServer() << " replies " << m_natMethod->GetNatType();
          PIPSocket::Address externalAddress;
          if (m_natMethod->GetExternalAddress(externalAddress))
            LogWindow << " with address " << externalAddress;
        }
        LogWindow << endl;
      }
      SetTranslationHost(PString::Empty());
      break;

    default :
      SetTranslationHost(PString::Empty());
      SetSTUNServer(PString::Empty());
  }
}


static void StartListenerForEP(OpalEndPoint * ep, const vector<PwxString> & allInterfaces)
{
  if (ep == NULL)
    return;

  PStringArray interfacesForEP;
  PString prefixAndColon = ep->GetPrefixName() + ':';

  for (size_t i = 0; i < allInterfaces.size(); i++) {
    PCaselessString iface = allInterfaces[i].p_str();
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
#if OPAL_H323
  StartListenerForEP(h323EP, m_LocalInterfaces);
#endif
#if OPAL_SIP
  StartListenerForEP(sipEP, m_LocalInterfaces);
#endif
}


#if OPAL_PTLIB_SSL
void MyManager::SetSignalingSecurity(OpalEndPoint * ep, int security, const char * unsecure, const char * secure)
{
  switch (security) {
    case 0 :
      AttachEndPoint(ep, unsecure);
      DetachEndPoint(secure);
      break;

    case 1 :
      AttachEndPoint(ep, secure);
      DetachEndPoint(unsecure);
      break;

    default :
      AttachEndPoint(ep, unsecure);
      AttachEndPoint(ep, secure);
  }
}
#endif


void MyManager::RecreateSpeedDials(SpeedDialViews view)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);

  config->Write(ActiveViewKey, (int)view);

  static DWORD const ListCtrlStyle[e_NumViews] = {
    wxLC_ICON, wxLC_SMALL_ICON, wxLC_LIST, wxLC_REPORT
  };

  m_speedDials = new wxListCtrl(m_tabs, SpeedDialsID,
                                wxDefaultPosition, wxSize(512, 128),
                                ListCtrlStyle[view] | wxLC_EDIT_LABELS | wxSUNKEN_BORDER);

  m_speedDialDetail = view == e_ViewDetails;
  if (m_speedDialDetail) {
    int width;
    static const wxChar * const titles[e_NumColumns] = {
      wxT("Name"),
      wxT("Status"),
      wxT("Number"),
      wxT("Address"),
      wxT("Presence Id"),
      wxT("Description")
    };
    
    for (int i = 0; i < e_NumColumns; i++) {
      m_speedDials->InsertColumn(i, titles[i]);
      wxString key;
      key.sprintf(ColumnWidthKey, i);
      if (config->Read(key, &width) && width > 0)
        m_speedDials->SetColumnWidth(i, width);
    }    
  }
  else {
    m_speedDials->SetImageList(m_imageListNormal, wxIMAGE_LIST_NORMAL);
    m_speedDials->SetImageList(m_imageListSmall, wxIMAGE_LIST_SMALL);
  }
  
  // Now either replace the tab or set it for the first time
  if (m_tabs->GetPageCount() == 0)
    m_tabs->AddPage(m_speedDials, SpeedDialTabTitle);
  else {
    m_tabs->DeletePage(0);
    m_tabs->InsertPage(0, m_speedDials, SpeedDialTabTitle);
  }

  for (set<SpeedDialInfo>::iterator it = m_speedDialInfo.begin(); it != m_speedDialInfo.end(); ++it) {
    long index = m_speedDials->InsertItem(INT_MAX, it->m_Name);
    m_speedDials->SetItemData(index, (intptr_t)&*it);
    UpdateSpeedDial(index, *it, false);
  }
  
  m_speedDials->Show();
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

  if (m_speedDialDetail) {
    for (int i = 0; i < e_NumColumns; i++) {
      int width = m_speedDials->GetColumnWidth(i);
      if (width > 0) {
        wxString key;
        key.sprintf(ColumnWidthKey, i);
        config->Write(key, width);
      }
    }
  }
  
  potsEP = NULL;
  m_activeCall.SetNULL();
  m_callsOnHold.clear();
  m_registrations.clear();
  ShutDownEndpoints();

  Destroy();
}


void MyManager::OnLogMessage(wxCommandEvent & theEvent)
{
  m_logWindow->WriteText(theEvent.GetString());
#if defined(__WXMAC__) && wxCHECK_VERSION(2,9,2)
  // Workaround for OSX (2.9.2) bug
  m_logWindow->SetStyle(0, m_logWindow->GetLastPosition(), wxTextAttr(*wxGREEN, *wxBLACK));
#endif
}


bool MyManager::CanDoFax() const
{
#if OPAL_FAX
  return GetMediaFormatMask().GetValuesIndex(OpalT38.GetName()) == P_MAX_INDEX &&
         OpalMediaFormat("TIFF-File").IsValid();
#else
  return false;
#endif
}


bool MyManager::CanDoIM() const
{
  SpeedDialInfo * info = GetSelectedSpeedDial();
  return info != NULL && !info->m_Presentity.IsEmpty();
}


void MyManager::OnAdjustMenus(wxMenuEvent& WXUNUSED(event))
{
  int id;

  wxMenuBar * menubar = GetMenuBar();
  menubar->Enable(XRCID("MenuCall"),            m_activeCall == NULL);
  menubar->Enable(XRCID("MenuCallLastDialed"),  m_activeCall == NULL && !m_LastDialed.IsEmpty());
  menubar->Enable(XRCID("MenuCallLastReceived"),m_activeCall == NULL && !m_LastReceived.IsEmpty());
  menubar->Enable(XRCID("MenuAnswer"),          !m_incomingToken.IsEmpty());
  menubar->Enable(XRCID("MenuHangUp"),          m_activeCall != NULL);
  menubar->Enable(XRCID("MenuHold"),            m_activeCall != NULL);
  menubar->Enable(XRCID("MenuTransfer"),        m_activeCall != NULL);
  menubar->Enable(XRCID("MenuStartRecording"),  m_activeCall != NULL && !m_activeCall->IsRecording());
  menubar->Enable(XRCID("MenuStopRecording"),   m_activeCall != NULL &&  m_activeCall->IsRecording());
  menubar->Enable(XRCID("MenuSendAudioFile"),   m_activeCall != NULL);
  menubar->Enable(XRCID("MenuInCallMessage"),   m_activeCall != NULL);

  menubar->Enable(XRCID("MenuSendFax"),         CanDoFax());

  for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
    menubar->Enable(it->m_retrieveMenuId,   m_activeCall == NULL);
    menubar->Enable(it->m_conferenceMenuId, m_activeCall != NULL);
    menubar->Enable(it->m_transferMenuId,   m_activeCall != NULL);
  }

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

  wxString deviceName;
  menubar->Enable(XRCID("SubMenuSound"), m_activeCall != NULL);
  PSafePtr<OpalPCSSConnection> pcss = PSafePtrCast<OpalConnection, OpalPCSSConnection>(GetConnection(true, PSafeReadOnly));
  if (pcss != NULL)
    deviceName = AudioDeviceNameToScreen(pcss->GetSoundChannelPlayDevice());
  for (id = ID_AUDIO_DEVICE_MENU_BASE; id <= ID_AUDIO_DEVICE_MENU_TOP; id++) {
    wxMenuItem * item = menubar->FindItem(id);
    if (item != NULL)
      item->Check(item->GetItemLabelText() == deviceName);
  }

  bool hasStartVideo = false;
  bool hasTxVideo = false;
  bool hasRxVideo = false;
  wxString audioFormat, videoFormat;

  PSafePtr<OpalConnection> connection = GetConnection(false, PSafeReadOnly);
  if (connection != NULL) {
    // Get ID of open audio to check the menu item
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Audio(), true);
    if (stream != NULL)
      audioFormat = PwxString(stream->GetMediaFormat());

    stream = connection->GetMediaStream(OpalMediaType::Video(), false);
    hasTxVideo = stream != NULL && stream->Open();

    stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream != NULL) {
      videoFormat = PwxString(stream->GetMediaFormat());
      hasRxVideo = stream->Open();
    }

    // Determine if video is startable
    hasStartVideo = connection->GetMediaFormats().HasType(OpalMediaType::Video());
  }

  menubar->Enable(XRCID("SubMenuAudio"), m_activeCall != NULL);
  for (id = ID_AUDIO_CODEC_MENU_BASE; id <= ID_AUDIO_CODEC_MENU_TOP; id++) {
    wxMenuItem * item = menubar->FindItem(id);
    if (item != NULL)
      item->Check(item->GetItemLabelText() == audioFormat);
  }

  menubar->Enable(XRCID("SubMenuVideo"), !videoFormat.IsEmpty() && m_activeCall != NULL);
  for (id = ID_VIDEO_CODEC_MENU_BASE; id <= ID_VIDEO_CODEC_MENU_TOP; id++) {
    wxMenuItem * item = menubar->FindItem(id);
    if (item != NULL)
      item->Check(item->GetItemLabelText() == videoFormat);
  }

  menubar->Enable(XRCID("MenuStartVideo"), hasStartVideo);
  menubar->Enable(XRCID("MenuStopVideo"), hasTxVideo);
  menubar->Enable(XRCID("MenuSendVFU"), hasRxVideo);
  menubar->Enable(XRCID("MenuSendIntra"), hasRxVideo);
  menubar->Enable(XRCID("MenuTxVideoControl"), hasTxVideo);
  menubar->Enable(XRCID("MenuRxVideoControl"), hasRxVideo);
  menubar->Enable(XRCID("MenuDefVidWinPos"), hasRxVideo || hasTxVideo);

  menubar->Enable(XRCID("SubMenuRetrieve"), !m_callsOnHold.empty());
  menubar->Enable(XRCID("SubMenuConference"), !m_callsOnHold.empty());
}


void MyManager::OnMenuQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}


void MyManager::OnMenuAbout(wxCommandEvent& WXUNUSED(event))
{
  tstringstream text;
  text  << PRODUCT_NAME_TEXT " Version " << PProcess::Current().GetVersion() << "\n"
           "\n"
           "Copyright (c) 2007-2008 " COPYRIGHT_HOLDER ", All rights reserved.\n"
           "\n"
           "This application may be used for any purpose so long as it is not sold "
           "or distributed for profit on it's own, or it's ownership by " COPYRIGHT_HOLDER
           " disguised or hidden in any way.\n"
           "\n"
           "Part of the Open Phone Abstraction Library, http://www.opalvoip.org\n"
           "  OPAL Version:  " << OpalGetVersion() << "\n"
           "  PTLib Version: " << PProcess::GetLibVersion() << '\n';
  wxMessageDialog dialog(this, text.str().c_str(), wxT("About ..."), wxOK);
  dialog.ShowModal();
}


void MyManager::OnMenuCall(wxCommandEvent& WXUNUSED(event))
{
  CallDialog dlg(this, false, true);
  if (dlg.ShowModal() == wxID_OK)
    MakeCall(dlg.m_Address, dlg.m_UseHandset ? "pots:*" : "pc:*");
}


void MyManager::OnMenuCallLastDialed(wxCommandEvent& WXUNUSED(event))
{
  MakeCall(m_LastDialed);
}


void MyManager::OnMenuCallLastReceived(wxCommandEvent& WXUNUSED(event))
{
  MakeCall(m_LastReceived);
}


SpeedDialInfo * MyManager::GetSelectedSpeedDial() const
{
  int idx = m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  return idx < 0 ? NULL : (SpeedDialInfo *)m_speedDials->GetItemData(idx);
}


void MyManager::OnCallSpeedDialAudio(wxCommandEvent & /*event*/)
{
  SpeedDialInfo * info = GetSelectedSpeedDial();
  if (info != NULL)
    MakeCall(info->m_Address);
}


void MyManager::OnCallSpeedDialHandset(wxCommandEvent & /*event*/)
{  
  SpeedDialInfo * info = GetSelectedSpeedDial();
  if (info != NULL)
    MakeCall(info->m_Address, "pots:*");
}


void MyManager::OnSendFax(wxCommandEvent & /*event*/)
{
  wxFileDialog faxDlg(this,
                      wxT("Send FAX file"),
                      wxEmptyString,
                      wxEmptyString,
                      wxT("*.tif"));
  if (faxDlg.ShowModal() == wxID_OK) {
    CallDialog callDlg(this, true, false);
    if (callDlg.ShowModal() == wxID_OK) {
      wxString prefix = callDlg.m_FaxMode ? wxT("fax:") : wxT("t38:");
      MakeCall(callDlg.m_Address, prefix + faxDlg.GetPath());
    }
  }
}

                                  
void MyManager::OnMyPresence(wxCommandEvent & /*event*/)
{
  PresenceDialog dlg(this);
  dlg.ShowModal();
}


#if OPAL_HAS_IM
void MyManager::OnStartIM(wxCommandEvent & /*event*/)
{
  CallIMDialog dlg(this);
  if (dlg.ShowModal() == wxID_OK) {
    if (m_imEP->CreateContext(dlg.m_Presentity.p_str(), dlg.m_Address.p_str()) == NULL)
      wxMessageBox(wxString(wxT("Cannot send IMs to ")) + dlg.m_Address, wxT("Error"));
  }
}


void MyManager::OnInCallIM(wxCommandEvent & /*event*/)
{
  if (m_activeCall == NULL)
    return;

  if (m_imEP->CreateContext(*m_activeCall) == NULL)
    wxMessageBox(wxT("Cannot send IMs to active call"), wxT("Error"));
}


void MyManager::OnSendIMSpeedDial(wxCommandEvent & /*event*/)
{
  SpeedDialInfo * info = GetSelectedSpeedDial();
  if (info != NULL && !info->m_Presentity.IsEmpty() && !info->m_Address.IsEmpty()) {
    if (m_imEP->CreateContext(info->m_Presentity, info->m_Address) == NULL)
      wxMessageBox(wxString(wxT("Cannot send IMs to")) + info->m_Address, wxT("Error"));
  }
}
#endif // OPAL_HAS_IM


void MyManager::OnSendFaxSpeedDial(wxCommandEvent & /*event*/)
{
  SpeedDialInfo * info = GetSelectedSpeedDial();
  if (info != NULL) {
    wxFileDialog faxDlg(this,
                        wxT("Send FAX file"),
                        wxEmptyString,
                        wxEmptyString,
                        wxT("*.tif"));
    if (faxDlg.ShowModal() == wxID_OK)
      MakeCall(info->m_Address, wxString(wxT("t38:")) + faxDlg.GetPath());
  }
}


void MyManager::OnMenuAnswer(wxCommandEvent& WXUNUSED(event))
{
  AnswerCall();
}


void MyManager::OnMenuHangUp(wxCommandEvent& WXUNUSED(event))
{
  HangUpCall();
}


static wxString MakeUniqueSpeedDialName(wxListCtrl * speedDials, const wxChar * baseName)
{
  wxString name(baseName);
  unsigned tieBreaker = 0;
  int count = speedDials->GetItemCount();
  int i = 0;
  while (i < count) {
    if (speedDials->GetItemText(i).CmpNoCase(name) != 0)
      i++;
    else {
      name.Printf(wxT("%s (%u)"), baseName, ++tieBreaker);
      i = 0;
    }
  }
  return name;
}


void MyManager::OnNewSpeedDial(wxCommandEvent& WXUNUSED(event))
{
  SpeedDialInfo info;
  info.m_Name = MakeUniqueSpeedDialName(m_speedDials, wxT("New Speed Dial"));
  EditSpeedDial(INT_MAX);
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
  EditSpeedDial(m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED));
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

  tstringstream tabbedText;
  int pos = -1;
  while ((pos = m_speedDials->GetNextItem(pos, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) >= 0) {
    SpeedDialInfo * info = (SpeedDialInfo *)m_speedDials->GetItemData(pos);
    if (PAssertNULL(info) != NULL) {
      tabbedText << info->m_Name << '\t'
                 << info->m_Number << '\t'
                 << info->m_Address << '\t'
                 << info->m_Description << '\t'
                 << info->m_Presentity
                 << "\r\n";
    }
  }

  // Want pure text so can copy to notepad or something, and our built
  // in format, even though the latter is exactly the same string. This
  // just guarantees the format of teh string, where just using CF_TEXT
  // coupld provide anything.
  wxDataObjectComposite * multiFormatData = new wxDataObjectComposite;
  wxTextDataObject * myFormatData = new wxTextDataObject(tabbedText.str());
  myFormatData->SetFormat(m_ClipboardFormat);
  multiFormatData->Add(myFormatData);
  multiFormatData->Add(new wxTextDataObject(tabbedText.str()));
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
        wxStringTokenizer tabbedLines(myFormatData.GetText(), wxT("\r\n"));
        while (tabbedLines.HasMoreTokens()) {
          wxStringTokenizer tabbedText(tabbedLines.GetNextToken(), wxT("\t"), wxTOKEN_RET_EMPTY_ALL);
          SpeedDialInfo info;
          info.m_Name = MakeUniqueSpeedDialName(m_speedDials, tabbedText.GetNextToken());
          info.m_Number = tabbedText.GetNextToken();
          info.m_Address = tabbedText.GetNextToken();
          info.m_Description = tabbedText.GetNextToken();
          info.m_Presentity = tabbedText.GetNextToken();

          UpdateSpeedDial(INT_MAX, info, true);
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

  tstringstream strm;
  strm << "Delete " << count << " item";
  if (count > 1)
    strm << 's';
  strm << '?';
  wxMessageDialog dlg(this, strm.str(), wxT("OpenPhone Speed Dials"), wxYES_NO);
  if (dlg.ShowModal() != wxID_YES)
    return;

  SpeedDialInfo * info;
  while ((info = GetSelectedSpeedDial()) != NULL) {
    wxConfigBase * config = wxConfig::Get();
    config->SetPath(SpeedDialsGroup);
    config->DeleteGroup(info->m_Name);
    m_speedDials->DeleteItem(m_speedDials->FindItem(-1, info->m_Name, true));
    m_speedDialInfo.erase(*info);
  }
}


void MyManager::OnSashPositioned(wxSplitterEvent& event)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);
  config->Write(SashPositionKey, event.GetSashPosition());
  config->Flush();
}


void MyManager::OnSpeedDialActivated(wxListEvent& event)
{
  SpeedDialInfo * info = (SpeedDialInfo *)m_speedDials->GetItemData(event.GetIndex());
  if (info != NULL)
    MakeCall(info->m_Address);
}


void MyManager::OnSpeedDialColumnResize(wxListEvent& event)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);
  wxString key;
  key.sprintf(ColumnWidthKey, event.GetColumn());
  config->Write(key, m_speedDials->GetColumnWidth(event.GetColumn()));
  config->Flush();
}


void MyManager::OnSpeedDialRightClick(wxListEvent& event)
{
  wxMenuBar * menuBar = wxXmlResource::Get()->LoadMenuBar(wxT("SpeedDialMenu"));
  wxMenu * menu = menuBar->Remove(0);
  delete menuBar;

  menu->Enable(XRCID("CallSpeedDialHandset"), HasHandset());
  menu->Enable(XRCID("SendFaxSpeedDial"),     CanDoFax());
  menu->Enable(XRCID("SendIMSpeedDial"),      CanDoIM());
  PopupMenu(menu, event.GetPoint());
  delete menu;
}


void MyManager::OnSpeedDialEndEdit(wxListEvent& event)
{
  if (event.IsEditCancelled())
    return;

  int index = event.GetIndex();
  SpeedDialInfo * oldInfo = (SpeedDialInfo *)m_speedDials->GetItemData(index);
  if (oldInfo == NULL)
    return;

  if (oldInfo->m_Name == event.GetLabel())
    return;

  if (HasSpeedDialName(event.GetLabel())) {
    event.Veto();
    return;
  }

  SpeedDialInfo newInfo(*oldInfo);
  newInfo.m_Name = event.GetLabel();
  UpdateSpeedDial(index, newInfo, true);
}


void MyManager::EditSpeedDial(int index)
{
  SpeedDialInfo * info;
  SpeedDialInfo newEntry;
  if (index == INT_MAX)
    info = &newEntry;
  else {
    info = (SpeedDialInfo *)m_speedDials->GetItemData(index);
    if (info == NULL)
      return;
  }

  // Should display a menu, but initially just allow editing
  SpeedDialDialog dlg(this, *info);
  if (dlg.ShowModal() == wxID_CANCEL)
    return;

  if (info->m_Presentity != dlg.m_Presentity) {
    if (!info->m_Presentity.empty())
      MonitorPresence(info->m_Presentity, dlg.m_Address, false);

    if (!dlg.m_Presentity.empty())
      MonitorPresence(dlg.m_Presentity, dlg.m_Address, true);
  }

  UpdateSpeedDial(index, dlg, true);
}


bool MyManager::UpdateSpeedDial(int index, const SpeedDialInfo & newInfo, bool saveConfig)
{
  if (index != INT_MAX)
    m_speedDials->SetItem(index, e_NameColumn, newInfo.m_Name);
  else {
    std::pair<set<SpeedDialInfo>::iterator, bool> result = m_speedDialInfo.insert(newInfo);
    if (!result.second)
      return false;

    index = m_speedDials->InsertItem(INT_MAX, newInfo.m_Name);
    m_speedDials->SetItemData(index, (intptr_t)&*result.first);
  }

  SpeedDialInfo * oldInfo = (SpeedDialInfo *)m_speedDials->GetItemData(index);

  if (saveConfig) {
    wxConfigBase * config = wxConfig::Get();
    config->SetPath(SpeedDialsGroup);
    if (!oldInfo->m_Name.IsEmpty())
      config->DeleteGroup(oldInfo->m_Name);
    config->SetPath(newInfo.m_Name);
    config->Write(SpeedDialNumberKey, newInfo.m_Number);
    config->Write(SpeedDialAddressKey, newInfo.m_Address);
    config->Write(SpeedDialPresentityKey, newInfo.m_Presentity);
    config->Write(SpeedDialDescriptionKey, newInfo.m_Description);
    config->Flush();
  }

  m_speedDials->SetItemImage(index, Icon_Unknown);

  if (m_speedDialDetail) {
    m_speedDials->SetItem(index, e_NumberColumn, newInfo.m_Number);
    m_speedDials->SetItem(index, e_StatusColumn, IconStatusNames[Icon_Unknown]);
    m_speedDials->SetItem(index, e_AddressColumn, newInfo.m_Address);
    m_speedDials->SetItem(index, e_PresentityColumn, newInfo.m_Presentity);
    m_speedDials->SetItem(index, e_DescriptionColumn, newInfo.m_Description);
  }

  *oldInfo = newInfo;
  return true;
}


bool MyManager::HasSpeedDialName(const wxString & name) const
{
  return m_speedDials->FindItem(-1, name) >= 0;
}


bool MyManager::HasSpeedDialNumber(const wxString & number, const wxString & ignore) const
{
  int count = m_speedDials->GetItemCount();
  for (int index = 0; index < count; index++) {
    SpeedDialInfo * info = (SpeedDialInfo *)m_speedDials->GetItemData(index);
    if (info != NULL && info->m_Number == number && info->m_Number != ignore)
      return true;
  }

  return false;
}


void MyManager::MakeCall(const PwxString & address, const PwxString & local, OpalConnection::StringOptions * options)
{
  if (address.IsEmpty())
    return;

  m_LastDialed = address;
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(GeneralGroup);
  config->Write(LastDialedKey, m_LastDialed);
  config->Flush();

  PwxString from = local;
  if (from.empty())
    from = "pc:*";

  m_activeCall = SetUpCall(from, address, NULL, 0, options);
  if (m_activeCall == NULL)
    LogWindow << "Could not call \"" << address << '"' << endl;
  else {
    LogWindow << "Calling \"" << address << '"' << endl;
    m_tabs->AddPage(new CallingPanel(*this, m_activeCall->GetToken(), m_tabs), wxT("Calling"), true);
    if (m_primaryVideoGrabber != NULL)
      m_primaryVideoGrabber->Start();
  }
}


void MyManager::AnswerCall()
{
  if (PAssert(!m_incomingToken.IsEmpty(), PLogicError)) {
    StopRingSound();

    m_tabs->AddPage(new CallingPanel(*this, m_incomingToken, m_tabs), wxT("Answering"), true);

    pcssEP->AcceptIncomingConnection(m_incomingToken);
    m_incomingToken.clear();
  }
}


void MyManager::RejectCall()
{
  if (PAssert(!m_incomingToken.IsEmpty(), PLogicError)) {
    StopRingSound();
    pcssEP->RejectIncomingConnection(m_incomingToken);
    m_incomingToken.clear();
  }
}


void MyManager::HangUpCall()
{
  if (PAssert(m_activeCall != NULL, PLogicError)) {
    LogWindow << "Hanging up \"" << *m_activeCall << '"' << endl;
    m_activeCall->Clear();
  }
}


void MyManager::OnEvtRinging(wxCommandEvent & theEvent)
{
  m_incomingToken = theEvent.GetString();
  PSafePtr<OpalCall> call = FindCallWithLock(m_incomingToken, PSafeReadOnly);
  if (!PAssert(call != NULL, PLogicError))
    return;

  PSafePtr<OpalConnection> connection = call->GetConnection(0, PSafeReadOnly);
  if (!PAssert(connection != NULL, PLogicError))
    return;

  PString alertingType;
  PSafePtr<OpalConnection> network = connection->GetOtherPartyConnection();
  if (network != NULL)
    alertingType = network->GetAlertingType();

  PTime now;
  LogWindow << "\nIncoming call at " << now.AsString("w h:mma")
            << " from " << connection->GetRemotePartyName();
  if (!alertingType.IsEmpty())
    LogWindow << ", type=" << alertingType;
  LogWindow << endl;

  m_LastReceived = connection->GetRemotePartyAddress();
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(GeneralGroup);
  config->Write(LastReceivedKey, m_LastDialed);
  config->Flush();

  // Bring window to front if not already
  if (!IsActive())
    RequestUserAttention();
  Raise();

  if (!m_ForwardingAddress.IsEmpty()) {
    if (m_ForwardingTimeout != 0)
      m_ForwardingTimer.SetInterval(0, m_ForwardingTimeout);
    else
      OnForwardingTimeout(m_ForwardingTimer, 0);
  }

  if ((m_autoAnswer && m_activeCall == NULL) || connection->GetStringOptions().Contains("Auto-Answer")) {
#if OPAL_FAX
    m_currentAnswerMode = m_defaultAnswerMode;
#endif
    m_tabs->AddPage(new CallingPanel(*this, m_incomingToken, m_tabs), wxT("Answering"), true);

    pcssEP->AcceptIncomingConnection(m_incomingToken);
    m_incomingToken.clear();
  }
  else {
    AnswerPanel * answerPanel = new AnswerPanel(*this, m_incomingToken, m_tabs);

    // Want the network side connection to get calling and called party names.
    answerPanel->SetPartyNames(call->GetPartyA(), call->GetPartyB());
    m_tabs->AddPage(answerPanel, wxT("Incoming"), true);

    if (!m_RingSoundFileName.empty()) {
      PTRACE(4, "OpenPhone\tPlaying ring file \"" << m_RingSoundFileName << '"');
      m_RingSoundChannel.Open(m_RingSoundDeviceName, PSoundChannel::Player);
      m_RingSoundChannel.PlayFile(m_RingSoundFileName, false);
      m_RingSoundTimer.RunContinuous(5000);
    }
  }
}


void MyManager::OnRingSoundAgain(PTimer &, INT)
{
  PTRACE(4, "OpenPhone\tReplaying ring file \"" << m_RingSoundFileName << '"');
  m_RingSoundChannel.PlayFile(m_RingSoundFileName, false);
}


void MyManager::StopRingSound()
{
  PTRACE(4, "OpenPhone\tStopping play of ring file \"" << m_RingSoundFileName << '"');
  m_RingSoundTimer.Stop();
  m_RingSoundChannel.Close();
}


PBoolean MyManager::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  bool usingHandset = connection.GetEndPoint().GetPrefixName() == "pots";
  if (usingHandset)
    LogWindow << "Line interface device \"" << connection.GetRemotePartyName() << "\" has gone off hook." << endl;

  if (!OpalManager::OnIncomingConnection(connection, options, stringOptions))
    return false;

  if (usingHandset) {
    m_activeCall = &connection.GetCall();
    PostEvent(wxEvtRinging, ID_RINGING, connection.GetCall().GetToken());
  }

  return true;
}


void MyManager::OnEstablishedCall(OpalCall & call)
{
  m_activeCall = &call;

  LogWindow << "Established call from " << call.GetPartyA() << " to " << call.GetPartyB() << endl;
  PostEvent(wxEvtEstablished, ID_ESTABLISHED, call.GetToken());

#if OPAL_FAX
  if (m_currentAnswerMode == AnswerFax)
    new PThreadObj<MyManager>(*this, &MyManager::SwitchToFax, true, "SwitchToFax");
#endif // OPAL_FAX

#if OPAL_SIP
  PSafePtr<SIPConnection> connection = call.GetConnectionAs<SIPConnection>(0);
  if (connection != NULL) {
    wxConfigBase * config = wxConfig::Get();
    config->SetPath(GeneralGroup);
    config->Write(CurrentSIPConnectionKey, PwxString(connection->GetDialog().AsString()));
    config->Flush();
  }
#endif // OPAL_SIP
}


void MyManager::OnEvtEstablished(wxCommandEvent & theEvent)
{
  PwxString token(theEvent.GetString());

  if (m_activeCall == NULL) {
    // Retrieve call from hold
    RemoveCallOnHold(token);
    m_activeCall = FindCallWithLock(token, PSafeReference);
  }
  else {
    bool createInCallPanel = true;
    for (size_t i = 0; i < m_tabs->GetPageCount(); ++i) {
      CallPanelBase * panel = dynamic_cast<CallPanelBase *>(m_tabs->GetPage(i));
      if (panel != NULL && panel->GetToken() == token) {
        if (dynamic_cast<InCallPanel *>(panel) != NULL)
          createInCallPanel = false;
        else
          m_tabs->DeletePage(i--);
      }
    }
    if (createInCallPanel) {
      PwxString title = m_activeCall->IsNetworkOriginated() ? m_activeCall->GetPartyA() : m_activeCall->GetPartyB();
      m_tabs->AddPage(new InCallPanel(*this, token, m_tabs), title, true);
    }
    else
      RemoveCallOnHold(token);
  }
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
    case OpalConnection::EndedByNoAccept :
      LogWindow << "Did not accept incoming call from \"" << name << '"';
      break;
    case OpalConnection::EndedByNoUser :
      LogWindow << "Could find user \"" << name << '"';
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
      LogWindow << call.GetCallEndReasonText() << " with \"" << name << '"';
  }
  PTime now;
  LogWindow << ", on " << now.AsString("w h:mma") << ". Duration "
            << setprecision(0) << setw(5) << (now - call.GetStartTime())
            << "s." << endl;

  PostEvent(wxEvtCleared, ID_CLEARED, call.GetToken());

#if OPAL_SIP
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(GeneralGroup);
  config->DeleteEntry(CurrentSIPConnectionKey);
#endif // OPAL_SIP

#if OPAL_HAS_MIXER
  if (m_mcuEP != NULL && m_mcuEP->GetConnectionCount() == 1)
    m_mcuEP->ShutDown();
#endif // OPAL_HAS_MIXER
}


void MyManager::OnEvtCleared(wxCommandEvent & theEvent)
{
  PwxString token(theEvent.GetString());

  // Call gone away, get rid of any panels associated with it
  for (size_t i = 0; i < m_tabs->GetPageCount(); ++i) {
    CallPanelBase * panel = dynamic_cast<CallPanelBase *>(m_tabs->GetPage(i));
    if (panel != NULL && panel->GetToken() == token)
      m_tabs->DeletePage(i--);
  }

  if (m_activeCall == NULL || token != m_activeCall->GetToken()) {
    // A call on hold got cleared
    if (RemoveCallOnHold(token))
      return;
  }

  m_activeCall.SetNULL();
}


void MyManager::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  OpalManager::OnHold(connection, fromRemote, onHold);

  if (fromRemote) {
    LogWindow << "Remote " << connection.GetRemotePartyName() << " has "
              << (onHold ? "put you on" : "released you from") << " hold." << endl;
    return;
  }

  LogWindow << "Remote " << connection.GetRemotePartyName() << " has been "
            << (onHold ? "put on" : "released from") << " hold." << endl;
  if (onHold)
    PostEvent(wxEvtOnHold, ID_ON_HOLD);
  else
    PostEvent(wxEvtEstablished, ID_ESTABLISHED, connection.GetCall().GetToken());
}


bool MyManager::OnTransferNotify(OpalConnection & connection, const PStringToString & info)
{
  LogWindow << "Transfer ";
  if (info["party"] == "A")
    LogWindow << "by ";
  else if (info["party"] == "B")
    LogWindow << "to ";
  else
    LogWindow << "from ";
  LogWindow << connection.GetRemotePartyName() << ' ' << info["result"] << endl;
  return OpalManager::OnTransferNotify(connection, info);
}


void MyManager::OnEvtOnHold(wxCommandEvent & )
{
  if (m_activeCall != NULL) {
    AddCallOnHold(*m_activeCall);
    m_activeCall.SetNULL();
  }
}


static void LogMediaStream(const char * stopStart, const OpalMediaStream & stream, const OpalConnection & connection)
{
  if (!connection.IsNetworkConnection())
    return;

  OpalMediaFormat mediaFormat = stream.GetMediaFormat();
  LogWindow << stopStart << (stream.IsSource() ? " receiving " : " sending ") << mediaFormat;

  if (!stream.IsSource() && mediaFormat.GetMediaType() == OpalMediaType::Audio())
    LogWindow << " (" << mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption())*mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits() << "ms)";

  LogWindow << (stream.IsSource() ? " from " : " to ")
            << connection.GetPrefixName() << " endpoint"
            << endl;
}


void MyManager::AdjustMediaFormats(bool   local,
                   const OpalConnection & connection,
                    OpalMediaFormatList & mediaFormats) const
{
  OpalManager::AdjustMediaFormats(local, connection, mediaFormats);

  if (local) {
    for (MyMediaList::const_iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
      if (mm->preferenceOrder >= 0) {
        for (OpalMediaFormatList::iterator it = mediaFormats.begin(); it != mediaFormats.end(); ++it) {
          if (*it == mm->mediaFormat)
            *it = mm->mediaFormat;
        }
      }
    }
  }
}


PBoolean MyManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return false;

  LogMediaStream("Started", stream, connection);

  PostEvent(wxEvtStreamsChanged, ID_STREAMS_CHANGED, connection.GetCall().GetToken());
  return true;
}


void MyManager::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalManager::OnClosedMediaStream(stream);

  LogMediaStream("Stopped", stream, stream.GetConnection());

  PostEvent(wxEvtStreamsChanged, ID_STREAMS_CHANGED);

  if (PIsDescendant(&stream, OpalVideoMediaStream)) {
    PVideoOutputDevice * device = ((const OpalVideoMediaStream &)stream).GetVideoOutputDevice();
    if (device != NULL && device->GetDeviceName().FindRegEx(" ([0-9])\"") == P_MAX_INDEX) {
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

        config->Flush();
      }
    }
  }
}


PSafePtr<OpalCall> MyManager::GetCall(PSafetyMode mode)
{
  if (m_activeCall == NULL)
    return NULL;

  PSafePtr<OpalCall> call = m_activeCall;
  return call.SetSafetyMode(mode) ? call : NULL;
}


PSafePtr<OpalConnection> MyManager::GetConnection(bool user, PSafetyMode mode)
{
  if (m_activeCall == NULL)
    return NULL;

  PSafePtr<OpalConnection> connection = m_activeCall->GetConnection(0, PSafeReference);
  while (connection != NULL && connection->IsNetworkConnection() == user)
    ++connection;

  return connection.SetSafetyMode(mode) ? connection : NULL;
}


bool MyManager::HasHandset() const
{
  return potsEP != NULL && potsEP->GetLine("*") != NULL;
}


MyManager::CallsOnHold::CallsOnHold(OpalCall & call)
  : m_call(&call, PSafeReference)
{
  static int lastMenuId;
  m_retrieveMenuId   = lastMenuId + ID_RETRIEVE_MENU_BASE;
  m_conferenceMenuId = lastMenuId + ID_CONFERENCE_MENU_BASE;
  m_transferMenuId   = lastMenuId + ID_TRANSFER_MENU_BASE;
  lastMenuId++;
}


void MyManager::AddCallOnHold(OpalCall & call)
{
  m_callsOnHold.push_back(call);

  PwxString otherParty = call.IsNetworkOriginated() ? call.GetPartyA() : call.GetPartyB();

  wxMenuBar * menubar = GetMenuBar();
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(XRCID("SubMenuRetrieve"));
  wxMenu * menu = PAssertNULL(item)->GetSubMenu();
  PAssertNULL(menu)->Append(m_callsOnHold.back().m_retrieveMenuId, otherParty);
  item = menu->FindItemByPosition(0);
  if (item->IsSeparator())
    menu->Delete(item);

  item = menubar->FindItem(XRCID("SubMenuConference"));
  menu = PAssertNULL(item)->GetSubMenu();
  PAssertNULL(menu)->Append(m_callsOnHold.back().m_conferenceMenuId, otherParty);
  item = menu->FindItemByPosition(0);
  if (item->IsSeparator())
    menu->Delete(item);

  item = menubar->FindItem(XRCID("SubMenuTransfer"));
  menu = PAssertNULL(item)->GetSubMenu();
  PAssertNULL(menu)->Append(m_callsOnHold.back().m_transferMenuId, otherParty);

  OnHoldChanged(call.GetToken(), true);

  if (!m_switchHoldToken.IsEmpty()) {
    PSafePtr<OpalCall> call = FindCallWithLock(m_switchHoldToken, PSafeReadWrite);
    if (call != NULL)
      call->Retrieve();
    m_switchHoldToken.clear();
  }
}


bool MyManager::RemoveCallOnHold(const PString & token)
{
  list<CallsOnHold>::iterator it = m_callsOnHold.begin();
  for (;;) {
    if (it == m_callsOnHold.end())
      return false;
    if (it->m_call->GetToken() == token)
      break;
    ++it;
  }

  wxMenuBar * menubar = GetMenuBar();
  wxMenu * menu;
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(it->m_retrieveMenuId, &menu);
  if (PAssert(menu != NULL && item != NULL, PLogicError)) {
    if (m_callsOnHold.size() == 1)
      menu->AppendSeparator();
    menu->Delete(item);
  }

  item = menubar->FindItem(it->m_conferenceMenuId, &menu);
  if (PAssert(menu != NULL && item != NULL, PLogicError)) {
    if (m_callsOnHold.size() == 1)
      menu->AppendSeparator();
    menu->Delete(item);
  }

  item = menubar->FindItem(it->m_transferMenuId, &menu);
  if (PAssert(menu != NULL && item != NULL, PLogicError))
    menu->Delete(item);

  m_callsOnHold.erase(it);

  OnHoldChanged(token, false);

  return true;
}


void MyManager::OnHoldChanged(const PString & token, bool onHold)
{
  for (size_t i = 0; i < m_tabs->GetPageCount(); ++i) {
    InCallPanel * panel = dynamic_cast<InCallPanel *>(m_tabs->GetPage(i));
    if (panel != NULL && panel->GetToken() == token) {
      panel->OnHoldChanged(onHold);
      break;
    }
  }
}


void MyManager::SendUserInput(char tone)
{
  PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadWrite);
  if (connection != NULL)
    connection->OnUserInputTone(tone, 100);
}


void MyManager::OnUserInputString(OpalConnection & connection, const PString & value)
{
  if (connection.IsNetworkConnection())
    LogWindow << "User input \"" << value << "\" received from \"" << connection.GetRemotePartyName() << '"' << endl;
  OpalManager::OnUserInputString(connection, value);
}


#if OPAL_FAX
void MyManager::OnUserInputTone(OpalConnection & connection, char tone, int duration)
{
  if (toupper(tone) == 'X' && m_currentAnswerMode == AnswerDetect)
    new PThreadObj<MyManager>(*this, &MyManager::SwitchToFax, true, "SwitchToFax");

  OpalManager::OnUserInputTone(connection, tone, duration);
}


void MyManager::SwitchToFax()
{
  if (m_activeCall == NULL)
    return; // Huh?

  if (m_activeCall->GetConnectionAs<OpalFaxConnection>(0, PSafeReference) != NULL)
    return; // Already switched

  if (!m_activeCall->IsNetworkOriginated())
    return; // We originated call

  PSafePtr<OpalConnection> connection = m_activeCall->GetConnection(1);
  if (connection == NULL)
    return; // Only one connection!

  if (m_activeCall->Transfer("t38:*;receive", connection))
    LogWindow << "Switching to T.38 fax mode." << endl;
  else
    LogWindow << "Could not switch to T.38 fax mode." << endl;
}
#endif // OPAL_FAX


void MyManager::OnRequestHold(wxCommandEvent& /*event*/)
{
  PSafePtr<OpalCall> call = GetCall(PSafeReadWrite);
  if (call != NULL)
    call->Hold();
}


void MyManager::OnRetrieve(wxCommandEvent& theEvent)
{
  if (PAssert(m_activeCall == NULL, PLogicError)) {
    for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
      if (theEvent.GetId() == it->m_retrieveMenuId) {
        it->m_call->Retrieve();
        break;
      }
    }
  }
}


void MyManager::OnConference(wxCommandEvent& theEvent)
{
  if (PAssert(m_activeCall != NULL, PLogicError)) {
    for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
      if (theEvent.GetId() == it->m_conferenceMenuId) {
        AddToConference(*it->m_call);
        return;
      }
    }
  }
}


void MyManager::AddToConference(OpalCall & call)
{
  if (m_activeCall != NULL) {
    PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReference);
    m_activeCall->Transfer("mcu:"CONFERENCE_NAME, connection);
    LogWindow << "Added \"" << connection->GetRemotePartyName() << "\" to conference." << endl;
    PString pc = "pc:*";
    if (connection->GetMediaStream(OpalMediaType::Video(), true) == NULL)
      pc += ";OPAL-AutoStart=video:no";
    SetUpCall(pc, "mcu:"CONFERENCE_NAME);
    m_activeCall.SetNULL();
  }

  PSafePtr<OpalLocalConnection> connection = call.GetConnectionAs<OpalLocalConnection>();
  call.Transfer("mcu:"CONFERENCE_NAME, connection);
  call.Retrieve();
  LogWindow << "Added \"" << connection->GetRemotePartyName() << "\" to conference." << endl;
}


void MyManager::OnTransfer(wxCommandEvent& theEvent)
{
  if (PAssert(m_activeCall != NULL, PLogicError)) {
    for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
      if (theEvent.GetId() == it->m_transferMenuId) {
        m_activeCall->Transfer(it->m_call->GetToken(), GetConnection(false, PSafeReference));
        return;
      }
    }

    CallDialog dlg(this, true, true);
    dlg.SetTitle(wxT("Transfer Call"));
    if (dlg.ShowModal() == wxID_OK)
      m_activeCall->Transfer(dlg.m_Address, GetConnection(false, PSafeReference));
  }
}


void MyManager::OnStartRecording(wxCommandEvent & /*event*/)
{
  wxFileDialog dlg(this,
                   wxT("Save call to file"),
                   wxEmptyString,
                   m_lastRecordFile,
                   wxT("WAV Files (*.wav)|*.wav|AVI Files (*.avi)|*.avi"),
                   wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() == wxID_OK && m_activeCall != NULL) {
    m_lastRecordFile = dlg.GetPath();
    if (!m_activeCall->StartRecording(m_lastRecordFile, m_recordingOptions))
      wxMessageBox(wxT("Cannot record to ")+m_lastRecordFile,
                   wxT("OpenPhone Error"), wxCANCEL|wxICON_EXCLAMATION);
  }
}


void MyManager::OnStopRecording(wxCommandEvent & /*event*/)
{
  if (m_activeCall != NULL)
    m_activeCall->StopRecording();
}


void MyManager::OnSendAudioFile(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalPCSSConnection> connection = PSafePtrCast<OpalConnection, OpalPCSSConnection>(GetConnection(true, PSafeReadOnly));
  if (connection == NULL)
    return;

  wxFileDialog dlg(this,
                   wxT("File to send"),
                   wxEmptyString,
                   m_lastPlayFile,
                   wxT("WAV Files (*.wav)|*.wav"),
                   wxFD_OPEN|wxFD_FILE_MUST_EXIST);
  if (dlg.ShowModal() == wxID_OK && m_activeCall != NULL) {
    m_lastPlayFile = dlg.GetPath();
    LogWindow << "Playing " << m_lastPlayFile << ", please wait ..." << endl;

    PStringStream ivrXML;
    ivrXML << "ivr:<?xml version=\"1.0\"?>"
                  "<vxml version=\"1.0\">"
                    "<form id=\"PlayFile\">"
                      "<transfer bridge=\"false\" dest=\"pc:*;Auto-Answer=1\">"
                        "<audio src=\"" << PURL(PFilePath(m_lastPlayFile.p_str())) << "\"/>"
                      "</transfer>"
                    "</form>"
                  "</vxml>";
    if (!m_activeCall->Transfer(ivrXML, connection))
      wxMessageBox(wxT("Cannot send ")+m_lastPlayFile,
                   wxT("OpenPhone Error"), wxCANCEL|wxICON_EXCLAMATION);
  }
}


void MyManager::OnAudioDevicePair(wxCommandEvent & /*theEvent*/)
{
  PSafePtr<OpalPCSSConnection> connection = PSafePtrCast<OpalConnection, OpalPCSSConnection>(GetConnection(true, PSafeReadOnly));
  if (connection != NULL) {
    AudioDevicesDialog dlg(this, *connection);
    if (dlg.ShowModal() == wxID_OK && connection.SetSafetyMode(PSafeReadWrite))
      m_activeCall->Transfer(dlg.GetTransferAddress(), connection);
  }
}


void MyManager::OnAudioDevicePreset(wxCommandEvent & theEvent)
{
  m_activeCall->Transfer("pc:"+AudioDeviceNameFromScreen(GetMenuBar()->FindItem(theEvent.GetId())->GetItemLabelText()));
}


void MyManager::OnNewCodec(wxCommandEvent& theEvent)
{
  OpalMediaFormat mediaFormat(PwxString(GetMenuBar()->FindItem(theEvent.GetId())->GetItemLabelText()).p_str());
  if (mediaFormat.IsValid()) {
    PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadWrite);
    if (connection != NULL) {
      OpalMediaStreamPtr stream = connection->GetMediaStream(mediaFormat.GetMediaType(), true);
      if (!connection->GetCall().OpenSourceMediaStreams(*connection,
                                                        mediaFormat.GetMediaType(),
                                                        stream != NULL ? stream->GetSessionID() : 0,
                                                        mediaFormat))
        LogWindow << "Could not change codec to " << mediaFormat << endl;
    }
  }
}


void MyManager::OnStartVideo(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadWrite);
  if (connection == NULL)
    return;

  OpalVideoFormat::ContentRole contentRole = OpalVideoFormat::eNoRole;

  OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), true);
  if (stream != NULL) {
    StartVideoDialog dlg(this, true);
    dlg.m_device = m_SecondaryVideoGrabber.deviceName;
    dlg.m_preview = m_SecondaryVideoGrabPreview;
    if (dlg.ShowModal() != wxID_OK)
      return;

    m_SecondaryVideoGrabber.deviceName = dlg.m_device.p_str();
    m_SecondaryVideoGrabPreview = dlg.m_preview;
    m_SecondaryVideoOpening = true;
    contentRole = (OpalVideoFormat::ContentRole)dlg.m_contentRole;
  }
  else {
    StartVideoDialog dlg(this, false);
    dlg.m_device = videoInputDevice.deviceName;
    dlg.m_preview = m_VideoGrabPreview;
    if (dlg.ShowModal() != wxID_OK)
      return;

    videoInputDevice.deviceName = dlg.m_device.p_str();
    m_VideoGrabPreview = dlg.m_preview;
  }

  if (!connection->GetCall().OpenSourceMediaStreams(*connection,
                                                    OpalMediaType::Video(),
                                                    0, // Allocate session automatically
                                                    OpalMediaFormat(),
                                                    contentRole))
    LogWindow << "Could not open video to remote!" << endl;
}


void MyManager::OnStopVideo(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadWrite);
  if (connection != NULL) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream != NULL)
      connection->CloseMediaStream(*stream);
  }
}


void MyManager::OnSendVFU(wxCommandEvent& /*event*/)
{
  PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadOnly);
  if (connection != NULL) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), false);
    if (stream != NULL)
      stream->ExecuteCommand(OpalVideoUpdatePicture());
  }
}


void MyManager::OnSendIntra(wxCommandEvent& /*event*/)
{
  PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadOnly);
  if (connection != NULL) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream != NULL)
      stream->ExecuteCommand(OpalVideoUpdatePicture());
  }
}


void MyManager::OnTxVideoControl(wxCommandEvent& /*event*/)
{
  VideoControlDialog dlg(this, false);
  dlg.ShowModal();
}


void MyManager::OnRxVideoControl(wxCommandEvent& /*event*/)
{
  VideoControlDialog dlg(this, true);
  dlg.ShowModal();
}


void MyManager::OnDefVidWinPos(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadOnly);
  if (connection == NULL)
    return;

  PVideoOutputDevice * preview = NULL;
  {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream != NULL)
      preview = dynamic_cast<const OpalVideoMediaStream *>(&*stream)->GetVideoOutputDevice();
  }

  PVideoOutputDevice * remote = NULL;
  {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), false);
    if (stream != NULL)
      remote = dynamic_cast<const OpalVideoMediaStream *>(&*stream)->GetVideoOutputDevice();
  }

  wxRect rect(GetPosition(), GetSize());

  m_remoteVideoFrameX = rect.GetLeft();
  m_remoteVideoFrameY = rect.GetBottom();
  if (remote != NULL)
    remote->SetPosition(m_remoteVideoFrameX, m_remoteVideoFrameY);

  m_localVideoFrameX = rect.GetLeft();
  m_localVideoFrameY = rect.GetBottom();
  if (remote != NULL)
    m_localVideoFrameX += remote->GetFrameWidth();
  if (preview != NULL)
    preview->SetPosition(m_localVideoFrameX, m_localVideoFrameY);

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(VideoGroup);
  config->Write(LocalVideoFrameXKey, m_localVideoFrameX);
  config->Write(LocalVideoFrameYKey, m_localVideoFrameY);
  config->Write(RemoteVideoFrameXKey, m_remoteVideoFrameX);
  config->Write(RemoteVideoFrameYKey, m_remoteVideoFrameY);
  config->Flush();
}


PString MyManager::ReadUserInput(OpalConnection & connection,
                                 const char *,
                                 unsigned,
                                 unsigned firstDigitTimeout)
{
  // The usual behaviour is to read until a '#' or timeout and that yields the
  // entire destination address. However for this application we want to disable
  // the timeout and short circuit the need for '#' as the speed dial number is
  // always unique.

  PTRACE(3, "OpalPhone\tReadUserInput from " << connection);

  connection.PromptUserInput(true);
  PwxString digit = connection.GetUserInput(firstDigitTimeout);
  connection.PromptUserInput(false);

  if (digit == "#")
    return digit;

  PwxString input;
  while (!digit.IsEmpty()) {
    input += digit;

    int count = m_speedDials->GetItemCount();
    for (int index = 0; index < count; index++) {
      SpeedDialInfo * info = (SpeedDialInfo *)m_speedDials->GetItemData(index);
      if (info == NULL)
        continue;

      size_t specialCharPos = info->m_Number.find_first_of(wxT("-+"));
      if (specialCharPos == wxString::npos) {
        if (input  != info->m_Number)
          continue;
      }
      else {
        if (digit != "#" || info->m_Number.compare(0, specialCharPos, input) != 0)
          continue;
        if (info->m_Number[specialCharPos] == '-')
          input.Remove(0, specialCharPos);    // Using '-' so strip the prefix off
        input.Remove(input.length()-1, 1); // Also get rid of the '#' at the end
      }

      PwxString address = info->m_Address;
      address.Replace(wxT("<dn>"), input);

      LogWindow << "Calling \"" << info->m_Name << "\" using \"" << address << '"' << endl;
      return address;
    }

    digit = connection.GetUserInput(firstDigitTimeout);
  }

  PTRACE(2, "OpalPhone\tReadUserInput timeout (" << firstDigitTimeout << " seconds) on " << *this);
  return PString::Empty();
}


PBoolean MyManager::CreateVideoInputDevice(const OpalConnection & connection,
                                           const OpalMediaFormat & mediaFormat,
                                           PVideoInputDevice * & device,
                                           PBoolean & autoDelete)
{
  if (!m_SecondaryVideoOpening) {
    if (m_primaryVideoGrabber == NULL)
      return OpalManager::CreateVideoInputDevice(connection, mediaFormat, device, autoDelete);

    device = m_primaryVideoGrabber;
    autoDelete = false;
    return true;
  }

  mediaFormat.AdjustVideoArgs(m_SecondaryVideoGrabber);

  autoDelete = true;
  device = PVideoInputDevice::CreateOpenedDevice(m_SecondaryVideoGrabber, false);
  PTRACE_IF(2, device == NULL, "OpenPhone\tCould not open secondary video device \"" << m_SecondaryVideoGrabber.deviceName << '"');

  m_SecondaryVideoOpening = false;

  return device != NULL;
}


PBoolean MyManager::CreateVideoOutputDevice(const OpalConnection & connection,
                                        const OpalMediaFormat & mediaFormat,
                                        PBoolean preview,
                                        PVideoOutputDevice * & device,
                                        PBoolean & autoDelete)
{
  unsigned openChannelCount = 0;
  OpalMediaStreamPtr mediaStream;
  while ((mediaStream = connection.GetMediaStream(OpalMediaType::Video(), preview, mediaStream)) != NULL)
    ++openChannelCount;

  if (preview && !(openChannelCount > 0 ? m_SecondaryVideoGrabPreview : m_VideoGrabPreview))
    return false;

  unsigned deltaX = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);

  if (m_localVideoFrameX == INT_MIN) {
    wxRect rect(GetPosition(), GetSize());
    m_localVideoFrameX = rect.GetLeft() + deltaX;
    m_localVideoFrameY = rect.GetBottom();
    m_remoteVideoFrameX = rect.GetLeft();
    m_remoteVideoFrameY = rect.GetBottom();
  }

  PVideoDevice::OpenArgs videoArgs;
  PString title;
  int x, y;
  if (preview) {
    videoArgs = GetVideoPreviewDevice();
    title = "Preview";
    x = m_localVideoFrameX;
    y = m_localVideoFrameY;
  }
  else {
    videoArgs = GetVideoOutputDevice();
    title = connection.GetRemotePartyName();
    x = m_remoteVideoFrameX;
    y = m_remoteVideoFrameY;
  }

  if (openChannelCount > 0)
    title.sprintf(" (%u)", openChannelCount);

  x += deltaX*openChannelCount;

  videoArgs.deviceName = psprintf(VIDEO_WINDOW_DEVICE" TITLE=\"%s\" X=%i Y=%i", (const char *)title, x, y);
  mediaFormat.AdjustVideoArgs(videoArgs);

  autoDelete = true;
  device = PVideoOutputDevice::CreateOpenedDevice(videoArgs, false);
  return device != NULL;
}


void MyManager::OnForwardingTimeout(PTimer &, INT)
{
  if (m_incomingToken.IsEmpty())
    return;

  // Transfer the incoming call to the forwarding address
  PSafePtr<OpalCall> call = FindCallWithLock(m_incomingToken, PSafeReadWrite);
  if (call == NULL)
    return;

  if (call->Transfer(m_ForwardingAddress, call->GetConnection(1)))
    LogWindow << "Forwarded \"" << call->GetPartyB() << "\" to \"" << m_ForwardingAddress << '"' << endl;
  else
    LogWindow << "Could not forward \"" << call->GetPartyB() << "\" to \"" << m_ForwardingAddress << '"' << endl;

  m_incomingToken.clear();
}


void MyManager::OnStreamsChanged(wxCommandEvent & evnt)
{
  for (size_t i = 0; i < m_tabs->GetPageCount(); ++i) {
    InCallPanel * panel = dynamic_cast<InCallPanel *>(m_tabs->GetPage(i));
    if (panel != NULL && panel->GetToken() == evnt.GetString()) {
      panel->OnStreamsChanged();
      break;
    }
  }
}


bool MyManager::StartGatekeeper()
{
#if OPAL_H323
  if (m_gatekeeperMode == 0)
    h323EP->RemoveGatekeeper();
  else {
    if (h323EP->UseGatekeeper(m_gatekeeperAddress, m_gatekeeperIdentifier)) {
      LogWindow << "H.323 registration started for " << *h323EP->GetGatekeeper() << endl;
      return true;
    }

    LogWindow << "H.323 registration failed for " << m_gatekeeperIdentifier;
    if (!m_gatekeeperIdentifier.IsEmpty() && !m_gatekeeperAddress.IsEmpty())
      LogWindow << '@';
    LogWindow << m_gatekeeperAddress << endl;
  }

  return m_gatekeeperMode < 2;
#else
   return false;
#endif
}


#if OPAL_SIP

void MyManager::StartRegistrations()
{
  if (sipEP == NULL)
    return;

  for (RegistrationList::iterator iter = m_registrations.begin(); iter != m_registrations.end(); ++iter)
    iter->Start(*sipEP);
}


void MyManager::ReplaceRegistrations(const RegistrationList & newRegistrations)
{
  for (RegistrationList::iterator iter = m_registrations.begin(); iter != m_registrations.end(); ) {
    RegistrationList::const_iterator newReg = std::find(newRegistrations.begin(), newRegistrations.end(), *iter);
    if (newReg != newRegistrations.end())
      ++iter;
    else {
      iter->Stop(*sipEP);
      m_registrations.erase(iter++);
    }
  }

  for (RegistrationList::const_iterator iter = newRegistrations.begin(); iter != newRegistrations.end(); ++iter) {
    RegistrationList::iterator oldReg = std::find(m_registrations.begin(), m_registrations.end(), *iter);
    if (oldReg == m_registrations.end()) {
      m_registrations.push_back(*iter);
      m_registrations.back().Start(*sipEP);
    }
  }
}

#endif // OPAL_SIP


bool MyManager::MonitorPresence(const PString & aor, const PString & uri, bool start)
{
  if (aor.IsEmpty() || uri.IsEmpty())
    return false;

  PSafePtr<OpalPresentity> presentity = GetPresentity(aor);
  if (presentity == NULL) {
    LogWindow << "Presence identity missing for " << aor << endl;
    return false;
  }

  if (start) {
    OpalPresentity::BuddyInfo buddy;
    buddy.m_presentity = uri;
    presentity->SetBuddy(buddy);

    if (!presentity->SubscribeToPresence(uri))
      return false;

    LogWindow << "Presence identity " << aor << " monitoring " << uri << endl;
  }
  else {
    presentity->UnsubscribeFromPresence(uri);
    presentity->DeleteBuddy(uri);
    LogWindow << "Presence identity " << aor << " no longer monitoring " << uri << endl;
  }

  return true;
}


void MyManager::OnPresenceChange(OpalPresentity &, OpalPresenceInfo info)
{
  int count = m_speedDials->GetItemCount();
  for (int index = 0; index < count; index++) {
    SpeedDialInfo * sdInfo = (SpeedDialInfo *)m_speedDials->GetItemData(index);

    SIPURL speedDialURL(sdInfo->m_Address.p_str());
    if (info.m_entity == speedDialURL ||
        (info.m_entity.GetScheme() == "pres" &&
        info.m_entity.GetUserName() == speedDialURL.GetUserName() &&
        info.m_entity.GetHostName() == speedDialURL.GetHostName())) {
      PwxString status = info.m_note;

      wxListItem item;
      item.m_itemId = index;
      item.m_mask = wxLIST_MASK_IMAGE;
      if (!m_speedDials->GetItem(item))
        continue;

      IconStates oldIcon = (IconStates)item.m_image;

      IconStates newIcon;
      switch (info.m_state) {
        case OpalPresenceInfo::Unchanged :
          newIcon = oldIcon;
          break;

        case OpalPresenceInfo::NoPresence :
          newIcon = Icon_Unavailable;
          break;

        case OpalPresenceInfo::Busy :
          newIcon = Icon_Busy;
          break;

        case OpalPresenceInfo::Away :
          newIcon = Icon_Away;
          break;

        default :
          if (status.CmpNoCase(wxT("busy")) == 0)
            newIcon = Icon_Busy;
          else if (status.CmpNoCase(wxT("away")) == 0)
            newIcon = Icon_Away;
          else
            newIcon = Icon_Present;
          break;
      }

      if (status.IsEmpty())
        status = IconStatusNames[newIcon];

      if (m_speedDialDetail)
        m_speedDials->SetItem(index, e_StatusColumn, status);

      if (oldIcon != newIcon) {
        m_speedDials->SetItemImage(index, newIcon);
        LogWindow << "Presence notification received for " << info.m_entity << ", \"" << status << '"' << endl;
      }
      break;
    }
  }
}


bool MyManager::AdjustVideoFormats()
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

  for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
    OpalMediaFormat & mediaFormat = mm->mediaFormat;
    unsigned frameRate = GetVideoInputDevice().rate;
    if (frameRate == 0)
      frameRate = 15;
    if (mediaFormat.GetMediaType() == OpalMediaType::Video()) {
      mediaFormat.SetOptionInteger(OpalVideoFormat::FrameWidthOption(), width);
      mediaFormat.SetOptionInteger(OpalVideoFormat::FrameHeightOption(), height);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MinRxFrameWidthOption(), minWidth);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MinRxFrameHeightOption(), minHeight);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MaxRxFrameWidthOption(), maxWidth);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MaxRxFrameHeightOption(), maxHeight);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MaxBitRateOption(), m_VideoMaxBitRate*1000U);
      mediaFormat.SetOptionInteger(OpalVideoFormat::TargetBitRateOption(), m_VideoTargetBitRate*1000U);
      mediaFormat.SetOptionInteger(OpalVideoFormat::FrameTimeOption(), mediaFormat.GetClockRate()/frameRate);

      switch (m_ExtendedVideoRoles) {
        case e_DisabledExtendedVideoRoles :
          mediaFormat.SetOptionInteger(OpalVideoFormat::ContentRoleMaskOption(), 0);
          break;

        case e_ForcePresentationVideoRole :
          mediaFormat.SetOptionInteger(OpalVideoFormat::ContentRoleMaskOption(),
                                       OpalVideoFormat::ContentRoleBit(OpalVideoFormat::ePresentation));
          break;

        case e_ForceLiveVideoRole :
          mediaFormat.SetOptionInteger(OpalVideoFormat::ContentRoleMaskOption(),
                                       OpalVideoFormat::ContentRoleBit(OpalVideoFormat::eMainRole));
          break;

        default :
          break;
      }
    }
  }

  return true;
}


void MyManager::UpdateAudioDevices()
{
  wxMenuBar * menubar = GetMenuBar();
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(XRCID("SubMenuSound"));
  wxMenu * audioMenu = PAssertNULL(item)->GetSubMenu();
  while (audioMenu->GetMenuItemCount() > 2)
    audioMenu->Delete(audioMenu->FindItemByPosition(2));

  unsigned id = ID_AUDIO_DEVICE_MENU_BASE;
  PStringList playDevices = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
  PStringList recordDevices = PSoundChannel::GetDeviceNames(PSoundChannel::Recorder);
  for (PINDEX i = 0; i < playDevices.GetSize(); i++) {
    if (recordDevices.GetValuesIndex(playDevices[i]) != P_MAX_INDEX)
      audioMenu->Append(id++, AudioDeviceNameToScreen(playDevices[i]), wxEmptyString, true);
  }
}


void MyManager::ApplyMediaInfo()
{
  PStringList mediaFormatOrder, mediaFormatMask;

  m_mediaInfo.sort();

  wxMenuBar * menubar = GetMenuBar();
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(XRCID("SubMenuAudio"));
  wxMenu * audioMenu = PAssertNULL(item)->GetSubMenu();
  while (audioMenu->GetMenuItemCount() > 0)
    audioMenu->Delete(audioMenu->FindItemByPosition(0));

  item = PAssertNULL(menubar)->FindItem(XRCID("SubMenuVideo"));
  wxMenu * videoMenu = PAssertNULL(item)->GetSubMenu();
  while (videoMenu->GetMenuItemCount() > 0)
    videoMenu->Delete(videoMenu->FindItemByPosition(0));

  for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
    if (mm->preferenceOrder < 0)
      mediaFormatMask.AppendString(mm->mediaFormat);
    else {
      mediaFormatOrder.AppendString(mm->mediaFormat);
      if (mm->mediaFormat.GetMediaType() == OpalMediaType::Audio())
        audioMenu->Append(mm->preferenceOrder+ID_AUDIO_CODEC_MENU_BASE,
                          PwxString(mm->mediaFormat.GetName()),
                          wxEmptyString,
                          true);
      else if (mm->mediaFormat.GetMediaType() == OpalMediaType::Video())
        videoMenu->Append(mm->preferenceOrder+ID_VIDEO_CODEC_MENU_BASE,
                          PwxString(mm->mediaFormat.GetName()),
                          wxEmptyString,
                          true);
    }
  }

  if (!mediaFormatOrder.IsEmpty()) {
    PTRACE(3, "OpenPhone\tMedia order:\n"<< setfill('\n') << mediaFormatOrder << setfill(' '));
    SetMediaFormatOrder(mediaFormatOrder);
    PTRACE(3, "OpenPhone\tMedia mask:\n"<< setfill('\n') << mediaFormatMask << setfill(' '));
    SetMediaFormatMask(mediaFormatMask);
  }
}


///////////////////////////////////////////////////////////////////////////////

RegistrationInfo::RegistrationInfo()
  : m_Type(Register)
  , m_Active(true)
  , m_TimeToLive(300)
  , m_Compatibility(SIPRegister::e_FullyCompliant)
{
}


bool RegistrationInfo::operator==(const RegistrationInfo & other) const
{
  return m_Type          == other.m_Type &&
         m_Active        == other.m_Active &&
         m_User          == other.m_User &&
         m_Domain        == other.m_Domain &&
         m_Contact       == other.m_Contact &&
         m_AuthID        == other.m_AuthID &&
         m_Password      == other.m_Password &&
         m_TimeToLive    == other.m_TimeToLive &&
         m_Proxy         == other.m_Proxy &&
         m_Compatibility == other.m_Compatibility;
}


bool RegistrationInfo::Read(wxConfigBase & config)
{
  if (!config.Read(RegistrarUsedKey, &m_Active, false))
    return false;

  if (!config.Read(RegistrarUsernameKey, &m_User))
    return false;

  if (!config.Read(RegistrarDomainKey, &m_Domain))
    return false;

  int iType;
  if (config.Read(RegistrationTypeKey, &iType, Register))
    m_Type = (Types)iType;

  config.Read(RegistrarContactKey, &m_Contact);
  config.Read(RegistrarAuthIDKey, &m_AuthID);
  config.Read(RegistrarPasswordKey, &m_Password);
  config.Read(RegistrarProxyKey, &m_Proxy);
  config.Read(RegistrarTimeToLiveKey, &m_TimeToLive);
  if (config.Read(RegistrarCompatibilityKey, &iType, SIPRegister::e_FullyCompliant))
    m_Compatibility = (SIPRegister::CompatibilityModes)iType;

  return true;
}


void RegistrationInfo::Write(wxConfigBase & config)
{
  config.Write(RegistrationTypeKey, (int)m_Type);
  config.Write(RegistrarUsedKey, m_Active);
  config.Write(RegistrarUsernameKey, m_User);
  config.Write(RegistrarDomainKey, m_Domain);
  config.Write(RegistrarContactKey, m_Contact);
  config.Write(RegistrarAuthIDKey, m_AuthID);
  config.Write(RegistrarPasswordKey, m_Password);
  config.Write(RegistrarProxyKey, m_Proxy);
  config.Write(RegistrarTimeToLiveKey, m_TimeToLive);
  config.Write(RegistrarCompatibilityKey, (int)m_Compatibility);
  config.Flush();
}

// these must match the drop-down box on the Registration/Subcription dialog box
static struct {
  const wxChar *                   m_name;
  SIPSubscribe::PredefinedPackages m_package;
} const RegistrationInfoTable[RegistrationInfo::NumRegType] = {
  { wxT("Registration"),        SIPSubscribe::NumPredefinedPackages }, // Skip Register enum
  { wxT("Message Waiting"),     SIPSubscribe::MessageSummary },
  { wxT("Others Presence"),     SIPSubscribe::Presence },
  { wxT("Line Appearance"),     SIPSubscribe::Dialog },
  { wxT("My Presence"),         SIPSubscribe::NumPredefinedPackages }, // Skip PublishPresence enum
  { wxT("Presence Watcher"),    SIPSubscribe::Presence | SIPSubscribe::Watcher },  // watch presence
  { wxT("Watch Registration"),  SIPSubscribe::Reg }
};


bool RegistrationInfo::Start(SIPEndPoint & sipEP)
{
  if (!m_Active)
    return false;

  bool success;

  if (m_Type == Register) {
    if (sipEP.IsRegistered(m_aor, true))
      return true;

    SIPRegister::Params param;
    param.m_addressOfRecord = m_User.p_str();
    param.m_registrarAddress = m_Domain.p_str();
    param.m_contactAddress = m_Contact.p_str();
    param.m_authID = m_AuthID.p_str();
    param.m_password = m_Password.p_str();
    param.m_proxyAddress = m_Proxy.p_str();
    param.m_expire = m_TimeToLive;
    param.m_compatibility = m_Compatibility;
    success = sipEP.Register(param, m_aor);
  }
  else {
    if (sipEP.IsSubscribed(RegistrationInfoTable[m_Type].m_package, m_aor, true))
      return true;

    SIPSubscribe::Params param(RegistrationInfoTable[m_Type].m_package);
    param.m_addressOfRecord = m_User.p_str();
    param.m_agentAddress = m_Domain.p_str();
    param.m_contactAddress = m_Contact.p_str();
    param.m_authID = m_AuthID.p_str();
    param.m_password = m_Password.p_str();
    param.m_expire = m_TimeToLive;
    success = sipEP.Subscribe(param, m_aor);
  }

  LogWindow << "SIP " << PString(RegistrationInfoTable[m_Type].m_name)
            << ' ' << (success ? "start" : "fail") << "ed for " << m_aor << endl;
  return success;
}


bool RegistrationInfo::Stop(SIPEndPoint & sipEP)
{
  if (!m_Active || m_aor.IsEmpty())
    return false;

  if (m_Type == Register)
    sipEP.Unregister(m_aor);
  else
    sipEP.Unsubscribe(RegistrationInfoTable[m_Type].m_package, m_aor);

  m_aor.MakeEmpty();
  return true;
}


///////////////////////////////////////////////////////////////////////////////

MyMedia::MyMedia()
  : validProtocols(NULL)
  , preferenceOrder(-1) // -1 indicates disabled
{
}


MyMedia::MyMedia(const OpalMediaFormat & format)
  : mediaFormat(format)
  , preferenceOrder(-1) // -1 indicates disabled
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

  virtual bool Validate(wxWindow *)
  {
    wxComboBox *control = (wxComboBox *) m_validatorWindow;
    PwxString size = control->GetValue();

    unsigned width, height;
    if (PVideoFrameInfo::ParseSize(size, width, height))
      return true;

    wxMessageBox(wxT("Illegal value \"") + size + wxT("\" for video size."),
                 wxT("OpenPhone Error"), wxCANCEL|wxICON_EXCLAMATION);
    return false;
  }
};


///////////////////////////////////////////////////////////////////////////////

void MyManager::OnOptions(wxCommandEvent& /*event*/)
{
  PTRACE(4, "OpenPhone\tOpening options dialog");
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
  EVT_BUTTON(XRCID("FindCertificateAuthority"), OptionsDialog::FindCertificateAuthority)
  EVT_BUTTON(XRCID("FindLocalCertificate"), OptionsDialog::FindLocalCertificate)
  EVT_BUTTON(XRCID("FindPrivateKey"), OptionsDialog::FindPrivateKey)
  EVT_LISTBOX(XRCID("LocalInterfaces"), OptionsDialog::SelectedLocalInterface)
  EVT_CHOICE(XRCID("InterfaceProtocol"), OptionsDialog::ChangedInterfaceInfo)
  EVT_TEXT(XRCID("InterfaceAddress"), OptionsDialog::ChangedInterfaceInfo)
  EVT_TEXT(XRCID("InterfacePort"), OptionsDialog::ChangedInterfaceInfo)
  EVT_BUTTON(XRCID("AddInterface"), OptionsDialog::AddInterface)
  EVT_BUTTON(XRCID("RemoveInterface"), OptionsDialog::RemoveInterface)

  ////////////////////////////////////////
  // Audio fields
  EVT_COMBOBOX(wxXmlResource::GetXRCID(SoundPlayerKey), OptionsDialog::ChangedSoundPlayer)
  EVT_COMBOBOX(wxXmlResource::GetXRCID(SoundRecorderKey), OptionsDialog::ChangedSoundRecorder)
  EVT_COMBOBOX(wxXmlResource::GetXRCID(LineInterfaceDeviceKey), OptionsDialog::SelectedLID)

  ////////////////////////////////////////
  // Video fields
  EVT_COMBOBOX(XRCID("VideoGrabDevice"), OptionsDialog::ChangeVideoGrabDevice)
  EVT_BUTTON(XRCID("TestVideoCapture"), OptionsDialog::TestVideoCapture)

  ////////////////////////////////////////
  // Fax fields
  EVT_BUTTON(XRCID("FaxBrowseReceiveDirectory"), OptionsDialog::BrowseFaxDirectory)

  ////////////////////////////////////////
  // Presence fields
  EVT_BUTTON(XRCID("AddPresentity"), OptionsDialog::AddPresentity)
  EVT_BUTTON(XRCID("RemovePresentity"), OptionsDialog::RemovePresentity)
  EVT_LIST_ITEM_SELECTED(XRCID("Presentities"), OptionsDialog::SelectedPresentity)
  EVT_LIST_ITEM_DESELECTED(XRCID("Presentities"), OptionsDialog::DeselectedPresentity)
  EVT_LIST_END_LABEL_EDIT(XRCID("Presentities"), OptionsDialog::EditedPresentity)
  EVT_GRID_CMD_CELL_CHANGE(XRCID("PresentityAttributes"), OptionsDialog::ChangedPresentityAttribute)

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
  // H.323 fields
#if OPAL_PTLIB_SSL
  EVT_RADIOBOX(wxXmlResource::GetXRCID(H323SignalingSecurityKey), OptionsDialog::H323SignalingSecurityChanged)
  EVT_LISTBOX(wxXmlResource::GetXRCID(H323MediaCryptoSuitesKey), OptionsDialog::H323MediaCryptoSuiteChanged)
  EVT_BUTTON(XRCID("H323MediaCryptoSuiteUp"), OptionsDialog::H323MediaCryptoSuiteUp)
  EVT_BUTTON(XRCID("H323MediaCryptoSuiteDown"), OptionsDialog::H323MediaCryptoSuiteDown)
#endif

  ////////////////////////////////////////
  // SIP fields
#if OPAL_PTLIB_SSL
  EVT_RADIOBOX(wxXmlResource::GetXRCID(SIPSignalingSecurityKey), OptionsDialog::SIPSignalingSecurityChanged)
  EVT_LISTBOX(wxXmlResource::GetXRCID(SIPMediaCryptoSuitesKey), OptionsDialog::SIPMediaCryptoSuiteChanged)
  EVT_BUTTON(XRCID("SIPMediaCryptoSuiteUp"), OptionsDialog::SIPMediaCryptoSuiteUp)
  EVT_BUTTON(XRCID("SIPMediaCryptoSuiteDown"), OptionsDialog::SIPMediaCryptoSuiteDown)
#endif

  EVT_BUTTON(XRCID("AddRegistrar"), OptionsDialog::AddRegistration)
  EVT_BUTTON(XRCID("ChangeRegistrar"), OptionsDialog::ChangeRegistration)
  EVT_BUTTON(XRCID("RemoveRegistrar"), OptionsDialog::RemoveRegistration)
  EVT_BUTTON(XRCID("MoveUpRegistrar"), OptionsDialog::MoveUpRegistration)
  EVT_BUTTON(XRCID("MoveDownRegistrar"), OptionsDialog::MoveDownRegistration)
  EVT_LIST_ITEM_SELECTED(XRCID("Registrars"), OptionsDialog::SelectedRegistration)
  EVT_LIST_ITEM_DESELECTED(XRCID("Registrars"), OptionsDialog::DeselectedRegistration)
  EVT_LIST_ITEM_ACTIVATED(XRCID("Registrars"), OptionsDialog::ActivateRegistration)

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
  EVT_BUTTON(XRCID("RestoreDefaultRoutes"), OptionsDialog::RestoreDefaultRoutes)

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


#if OPAL_PTLIB_SSL
static void InitSecurityFields(OpalEndPoint * ep, wxCheckListBox * listbox, bool securedSignaling)
{
  PStringArray allMethods = ep->GetAllMediaCryptoSuites();
  PStringArray enabledMethods = ep->GetMediaCryptoSuites();

  listbox->Clear();

  bool noneEnabled = true;
  for (PINDEX i = 0; i < enabledMethods.GetSize(); ++i) {
    OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(enabledMethods[i]);
    if (cryptoSuite != NULL) {
      listbox->Check(listbox->Append(PwxString(cryptoSuite->GetDescription())), securedSignaling);
      noneEnabled = false;
    }
  }

  for (PINDEX i = 0; i < allMethods.GetSize(); ++i) {
    if (enabledMethods.GetValuesIndex(allMethods[i]) != P_MAX_INDEX)
      continue;

    OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(allMethods[i]);
    if (cryptoSuite == NULL)
      continue;

    listbox->Check(listbox->Append(PwxString(cryptoSuite->GetDescription())), false);
  }

  if (noneEnabled || !securedSignaling)
    listbox->Check(listbox->FindString(PwxString(OpalMediaCryptoSuite::ClearText())));

  listbox->Enable(securedSignaling && allMethods.GetSize() > 1);
}
#endif // OPAL_PTLIB_SSL


OptionsDialog::OptionsDialog(MyManager * manager)
  : m_manager(*manager)
  , m_TestVideoThread(NULL)
  , m_TestVideoGrabber(NULL)
  , m_TestVideoDisplay(NULL)
{
  PINDEX i;

  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("OptionsDialog"));

  PTRACE(4, "OpenPhone\tLoaded options dialog");

  ////////////////////////////////////////
  // General fields
  INIT_FIELD(Username, m_manager.GetDefaultUserName());
  INIT_FIELD(DisplayName, m_manager.GetDefaultDisplayName());

  wxChoice * choice = FindWindowByNameAs<wxChoice>(this, RingSoundDeviceNameKey);
  choice->SetValidator(wxGenericValidator(&m_RingSoundDeviceName));
  FillAudioDeviceComboBox(choice, PSoundChannel::Player);
  m_RingSoundDeviceName = AudioDeviceNameToScreen(m_manager.m_RingSoundDeviceName);
  INIT_FIELD(RingSoundFileName, m_manager.m_RingSoundFileName);

  INIT_FIELD(AutoAnswer, m_manager.m_autoAnswer);
  const OpalProductInfo & productInfo = m_manager.GetProductInfo();
  INIT_FIELD(VendorName, productInfo.vendor);
  INIT_FIELD(ProductName, productInfo.name);
  INIT_FIELD(ProductVersion, productInfo.version);

  ////////////////////////////////////////
  // Networking fields
#if OPAL_H323
  int bandwidth = m_manager.h323EP->GetInitialBandwidth();
  if (bandwidth%10 == 0)
    m_Bandwidth.sprintf(wxT("%u"), bandwidth/10);
  else
    m_Bandwidth.sprintf(wxT("%u.%u"), bandwidth/10, bandwidth%10);
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
  FindWindowByNameAs<wxChoice>(this, wxT("BandwidthClass"))->SetSelection(bandwidthClass);
#endif

  INIT_FIELD(RTPTOS, m_manager.GetMediaTypeOfService());
  INIT_FIELD(MaxRtpPayloadSize, m_manager.GetMaxRtpPayloadSize());
#if OPAL_PTLIB_SSL
  INIT_FIELD(CertificateAuthority, m_manager.GetSSLCertificateAuthorityFile());
  INIT_FIELD(LocalCertificate, m_manager.GetSSLCertificateFile());
  INIT_FIELD(PrivateKey, m_manager.GetSSLPrivateKeyFile());
#endif
  INIT_FIELD(TCPPortBase, m_manager.GetTCPPortBase());
  INIT_FIELD(TCPPortMax, m_manager.GetTCPPortMax());
  INIT_FIELD(UDPPortBase, m_manager.GetUDPPortBase());
  INIT_FIELD(UDPPortMax, m_manager.GetUDPPortMax());
  INIT_FIELD(RTPPortBase, m_manager.GetRtpIpPortBase());
  INIT_FIELD(RTPPortMax, m_manager.GetRtpIpPortMax());

  m_NoNATUsedRadio = FindWindowByNameAs<wxRadioButton>(this, wxT("NoNATUsed"));
  m_NATRouterRadio = FindWindowByNameAs<wxRadioButton>(this, wxT("UseNATRouter"));
  m_STUNServerRadio= FindWindowByNameAs<wxRadioButton>(this, wxT("UseSTUNServer"));
  m_NATRouter = m_manager.m_NATRouter;
  m_NATRouterWnd = FindWindowByNameAs<wxTextCtrl>(this, wxT("NATRouter"));
  m_NATRouterWnd->SetValidator(wxGenericValidator(&m_NATRouter));
  m_STUNServer = m_manager.m_STUNServer;
  m_STUNServerWnd = FindWindowByNameAs<wxTextCtrl>(this, wxT("STUNServer"));
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

  m_AddInterface = FindWindowByNameAs<wxButton>(this, wxT("AddInterface"));
  m_AddInterface->Disable();
  m_RemoveInterface = FindWindowByNameAs<wxButton>(this, wxT("RemoveInterface"));
  m_RemoveInterface->Disable();
  m_InterfaceProtocol = FindWindowByNameAs<wxChoice>(this, wxT("InterfaceProtocol"));
  m_InterfacePort = FindWindowByNameAs<wxTextCtrl>(this, wxT("InterfacePort"));
  m_InterfaceAddress = FindWindowByNameAs<wxComboBox>(this, wxT("InterfaceAddress"));
  m_InterfaceAddress->Append(wxT("*"));
  PIPSocket::InterfaceTable ifaces;
  if (PIPSocket::GetInterfaceTable(ifaces)) {
    for (i = 0; i < ifaces.GetSize(); i++) {
      PIPSocket::Address ip = ifaces[i].GetAddress();
      PwxString addr = ip.AsString(true);
      PwxString name = wxT("%");
      name += PwxString(ifaces[i].GetName());
      m_InterfaceAddress->Append(addr);
      m_InterfaceAddress->Append(PwxString(PIPSocket::Address::GetAny(ip.GetVersion()).AsString(true)) + name);
      m_InterfaceAddress->Append(addr + name);
    }
  }
  m_LocalInterfaces = FindWindowByNameAs<wxListBox>(this, wxT("LocalInterfaces"));
  for (i = 0; (size_t)i < m_manager.m_LocalInterfaces.size(); i++)
    m_LocalInterfaces->Append(PwxString(m_manager.m_LocalInterfaces[i]));

  ////////////////////////////////////////
  // Sound fields
  INIT_FIELD(SoundBufferTime, m_manager.pcssEP->GetSoundChannelBufferTime());
  INIT_FIELD(MinJitter, m_manager.GetMinAudioJitterDelay());
  INIT_FIELD(MaxJitter, m_manager.GetMaxAudioJitterDelay());
  INIT_FIELD(SilenceSuppression, m_manager.GetSilenceDetectParams().m_mode);
  INIT_FIELD(SilenceThreshold, m_manager.GetSilenceDetectParams().m_threshold);
  INIT_FIELD(SignalDeadband, m_manager.GetSilenceDetectParams().m_signalDeadband);
  INIT_FIELD(SilenceDeadband, m_manager.GetSilenceDetectParams().m_silenceDeadband);
  INIT_FIELD(DisableDetectInBandDTMF, m_manager.DetectInBandDTMFDisabled());

  // Fill sound player combo box with available devices and set selection
  m_soundPlayerCombo = FindWindowByNameAs<wxComboBox>(this, SoundPlayerKey);
  m_soundPlayerCombo->SetValidator(wxGenericValidator(&m_SoundPlayer));
  FillAudioDeviceComboBox(m_soundPlayerCombo, PSoundChannel::Player);
  m_SoundPlayer = AudioDeviceNameToScreen(m_manager.pcssEP->GetSoundChannelPlayDevice());

  // Fill sound recorder combo box with available devices and set selection
  m_soundRecorderCombo = FindWindowByNameAs<wxComboBox>(this, SoundRecorderKey);
  m_soundRecorderCombo->SetValidator(wxGenericValidator(&m_SoundRecorder));
  FillAudioDeviceComboBox(m_soundRecorderCombo, PSoundChannel::Recorder);
  m_SoundRecorder = AudioDeviceNameToScreen(m_manager.pcssEP->GetSoundChannelRecordDevice());

  // Fill line interface combo box with available devices and set selection
  m_selectedAEC = FindWindowByNameAs<wxChoice>(this, AECKey);
  m_selectedCountry = FindWindowByNameAs<wxComboBox>(this, CountryKey);
  m_selectedCountry->SetValidator(wxGenericValidator(&m_Country));
  m_selectedLID = FindWindowByNameAs<wxComboBox>(this, LineInterfaceDeviceKey);
  m_selectedLID->SetValidator(wxGenericValidator(&m_LineInterfaceDevice));
  PStringList devices = OpalLineInterfaceDevice::GetAllDevices();
  if (devices.IsEmpty()) {
    m_LineInterfaceDevice = "<< None available >>";
    m_selectedLID->Append(m_LineInterfaceDevice);
    m_selectedAEC->Disable();
    m_selectedCountry->Disable();
  }
  else {
    static const wxChar UseSoundCard[] = wxT("<< Use sound card only >>");
    m_selectedLID->Append(UseSoundCard);
    for (i = 0; i < devices.GetSize(); i++)
      m_selectedLID->Append(PwxString(devices[i]));

    OpalLine * line = m_manager.potsEP->GetLine("*");
    if (line != NULL) {
      m_LineInterfaceDevice = line->GetDevice().GetDeviceType() + ": " + line->GetDevice().GetDeviceName();
      for (i = 0; i < devices.GetSize(); i++) {
        if (m_LineInterfaceDevice == devices[i])
          break;
      }
      if (i >= devices.GetSize()) {
        for (i = 0; i < devices.GetSize(); i++) {
          if (devices[i].Find(m_LineInterfaceDevice.p_str()) == 0)
            break;
        }
        if (i >= devices.GetSize())
          m_LineInterfaceDevice = devices[0];
      }

      INIT_FIELD(AEC, line->GetAEC());

      PStringList countries = line->GetDevice().GetCountryCodeNameList();
      for (PStringList::iterator country = countries.begin(); country != countries.end(); ++country)
        m_selectedCountry->Append(PwxString(*country));
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
  INIT_FIELD(VideoGrabDevice, m_manager.GetVideoInputDevice().deviceName);
  INIT_FIELD(VideoGrabFormat, m_manager.GetVideoInputDevice().videoFormat);
  m_videoSourceChoice = FindWindowByNameAs<wxChoice>(this, wxT("VideoGrabSource"));
  m_VideoGrabSource = m_manager.GetVideoInputDevice().channelNumber+1;
  m_videoSourceChoice->SetValidator(wxGenericValidator(&m_VideoGrabSource));
  INIT_FIELD(VideoGrabFrameRate, m_manager.GetVideoInputDevice().rate);
  INIT_FIELD(VideoFlipLocal, m_manager.GetVideoInputDevice().flip != false);
  INIT_FIELD(VideoGrabPreview, m_manager.m_VideoGrabPreview);
  INIT_FIELD(VideoAutoTransmit, m_manager.CanAutoStartTransmitVideo() != false);
  INIT_FIELD(VideoAutoReceive, m_manager.CanAutoStartReceiveVideo() != false);
  INIT_FIELD(VideoFlipRemote, m_manager.GetVideoOutputDevice().flip != false);
  INIT_FIELD(VideoGrabBitRate, m_manager.m_VideoTargetBitRate);
  INIT_FIELD(VideoMaxBitRate, m_manager.m_VideoMaxBitRate);

  PStringArray knownSizes = PVideoFrameInfo::GetSizeNames();
  m_VideoGrabFrameSize = m_manager.m_VideoGrabFrameSize;
  wxComboBox * combo = FindWindowByNameAs<wxComboBox>(this, VideoGrabFrameSizeKey);
  combo->SetValidator(wxFrameSizeValidator(&m_VideoGrabFrameSize));
  for (i = 0; i < knownSizes.GetSize(); ++i)
    combo->Append(PwxString(knownSizes[i]));

  m_VideoMinFrameSize = m_manager.m_VideoMinFrameSize;
  combo = FindWindowByNameAs<wxComboBox>(this, VideoMinFrameSizeKey);
  combo->SetValidator(wxFrameSizeValidator(&m_VideoMinFrameSize));
  for (i = 0; i < knownSizes.GetSize(); ++i)
    combo->Append(PwxString(knownSizes[i]));

  m_VideoMaxFrameSize = m_manager.m_VideoMaxFrameSize;
  combo = FindWindowByNameAs<wxComboBox>(this, VideoMaxFrameSizeKey);
  combo->SetValidator(wxFrameSizeValidator(&m_VideoMaxFrameSize));
  for (i = 0; i < knownSizes.GetSize(); ++i)
    combo->Append(PwxString(knownSizes[i]));

  m_videoGrabDeviceCombo = FindWindowByNameAs<wxComboBox>(this, wxT("VideoGrabDevice"));
  devices = PVideoInputDevice::GetDriversDeviceNames("*");
  for (i = 0; i < devices.GetSize(); i++)
    m_videoGrabDeviceCombo->Append(PwxString(devices[i]));

  AdjustVideoControls(m_VideoGrabDevice);

  m_TestVideoCapture = FindWindowByNameAs<wxButton>(this, wxT("TestVideoCapture"));

  ////////////////////////////////////////
  // Fax fields
#if OPAL_FAX
  INIT_FIELD(FaxStationIdentifier, m_manager.m_faxEP->GetDefaultDisplayName());
  INIT_FIELD(FaxReceiveDirectory, m_manager.m_faxEP->GetDefaultDirectory());
  INIT_FIELD(FaxAutoAnswerMode, m_manager.m_defaultAnswerMode);
#else
  RemoveNotebookPage(this, wxT("Fax"));
#endif

  ////////////////////////////////////////
  // IVR fields
#if OPAL_IVR
  INIT_FIELD(IVRScript, m_manager.ivrEP->GetDefaultVXML());
#endif

  INIT_FIELD(AudioRecordingMode, m_manager.m_recordingOptions.m_stereo);
  INIT_FIELD(AudioRecordingFormat, m_manager.m_recordingOptions.m_audioFormat);
  if (m_AudioRecordingFormat.empty())
    m_AudioRecordingFormat = OpalPCM16.GetName();
  INIT_FIELD(VideoRecordingMode, m_manager.m_recordingOptions.m_videoMixing);
  m_VideoRecordingSize = PVideoFrameInfo::AsString(m_manager.m_recordingOptions.m_videoWidth,
                                                   m_manager.m_recordingOptions.m_videoHeight);
  combo = FindWindowByNameAs<wxComboBox>(this, VideoRecordingSizeKey);
  combo->SetValidator(wxFrameSizeValidator(&m_VideoRecordingSize));
  for (i = 0; i < knownSizes.GetSize(); ++i)
    combo->Append(PwxString(knownSizes[i]));

  choice = FindWindowByNameAs<wxChoice>(this, AudioRecordingFormatKey);
  PWAVFileFormatByFormatFactory::KeyList_T wavFileFormats = PWAVFileFormatByFormatFactory::GetKeyList();
  for (PWAVFileFormatByFormatFactory::KeyList_T::iterator iterFmt = wavFileFormats.begin(); iterFmt != wavFileFormats.end(); ++iterFmt)
    choice->Append(PwxString(*iterFmt));

  ////////////////////////////////////////
  // Presence fields

  m_Presentities = FindWindowByNameAs<wxListCtrl>(this, wxT("Presentities"));
  m_Presentities->InsertColumn(0, _T("Address"));
  PStringArray presentities = m_manager.GetPresentities();
  for (i = 0; i < presentities.GetSize(); ++i) {
    long pos = m_Presentities->InsertItem(INT_MAX, PwxString(presentities[i]));
    m_Presentities->SetItemPtrData(pos, (wxUIntPtr)m_manager.GetPresentity(presentities[i])->Clone());
  }
  m_Presentities->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);

  m_AddPresentity = FindWindowByNameAs<wxButton>(this, wxT("AddPresentity"));

  m_RemovePresentity = FindWindowByNameAs<wxButton>(this, wxT("RemovePresentity"));
  m_RemovePresentity->Disable();

  m_PresentityAttributes = FindWindowByNameAs<wxGrid>(this, wxT("PresentityAttributes"));
  m_PresentityAttributes->Disable();
  m_PresentityAttributes->CreateGrid(0, 1);
  m_PresentityAttributes->SetColLabelValue(0, wxT("Attribute Value"));
  m_PresentityAttributes->SetColLabelSize(wxGRID_AUTOSIZE);
  m_PresentityAttributes->AutoSizeColLabelSize(0);
  m_PresentityAttributes->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_TOP);

  ////////////////////////////////////////
  // Codec fields
  m_AddCodec = FindWindowByNameAs<wxButton>(this, wxT("AddCodec"));
  m_AddCodec->Disable();
  m_RemoveCodec = FindWindowByNameAs<wxButton>(this, wxT("RemoveCodec"));
  m_RemoveCodec->Disable();
  m_MoveUpCodec = FindWindowByNameAs<wxButton>(this, wxT("MoveUpCodec"));
  m_MoveUpCodec->Disable();
  m_MoveDownCodec = FindWindowByNameAs<wxButton>(this, wxT("MoveDownCodec"));
  m_MoveDownCodec->Disable();

  m_allCodecs = FindWindowByNameAs<wxListBox>(this, wxT("AllCodecs"));
  m_selectedCodecs = FindWindowByNameAs<wxListBox>(this, wxT("SelectedCodecs"));
  for (MyMediaList::iterator mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm) {
    tstringstream details;
    details << mm->mediaFormat.GetMediaType().c_str() << ": " << mm->mediaFormat.GetName();
    if (mm->validProtocols != NULL)
      details << mm->validProtocols;
    m_allCodecs->Append(details.str().c_str(), &*mm);

    PwxString str(mm->mediaFormat);
    if (mm->preferenceOrder >= 0 && m_selectedCodecs->FindString(str) < 0)
      m_selectedCodecs->Append(str, &*mm);
  }
  m_codecOptions = FindWindowByNameAs<wxListCtrl>(this, wxT("CodecOptionsList"));
  int columnWidth = (m_codecOptions->GetClientSize().GetWidth()-30)/2;
  m_codecOptions->InsertColumn(0, wxT("Option"), wxLIST_FORMAT_LEFT, columnWidth);
  m_codecOptions->InsertColumn(1, wxT("Value"), wxLIST_FORMAT_LEFT, columnWidth);
  m_codecOptionValue = FindWindowByNameAs<wxTextCtrl>(this, wxT("CodecOptionValue"));
  m_codecOptionValue->Disable();
  m_CodecOptionValueLabel = FindWindowByNameAs<wxStaticText>(this, wxT("CodecOptionValueLabel"));
  m_CodecOptionValueLabel->Disable();
  m_CodecOptionValueError = FindWindowByNameAs<wxStaticText>(this, wxT("CodecOptionValueError"));
  m_CodecOptionValueError->Show(false);

#if OPAL_H323
  ////////////////////////////////////////
  // H.323 fields
  m_AddAlias = FindWindowByNameAs<wxButton>(this, wxT("AddAlias"));
  m_AddAlias->Disable();
  m_RemoveAlias = FindWindowByNameAs<wxButton>(this, wxT("RemoveAlias"));
  m_RemoveAlias->Disable();
  m_NewAlias = FindWindowByNameAs<wxTextCtrl>(this, wxT("NewAlias"));
  m_Aliases = FindWindowByNameAs<wxListBox>(this, wxT("Aliases"));
  PStringList aliases = m_manager.h323EP->GetAliasNames();
  for (i = 1; i < aliases.GetSize(); i++)
    m_Aliases->Append(PwxString(aliases[i]));

  INIT_FIELD(DTMFSendMode, m_manager.h323EP->GetSendUserInputMode());
  if (m_DTMFSendMode >= OpalConnection::SendUserInputAsProtocolDefault)
    m_DTMFSendMode = OpalConnection::SendUserInputAsString;
#if OPAL_450
  INIT_FIELD(CallIntrusionProtectionLevel, m_manager.h323EP->GetCallIntrusionProtectionLevel());
#endif
  INIT_FIELD(DisableFastStart, m_manager.h323EP->IsFastStartDisabled() != false);
  INIT_FIELD(DisableH245Tunneling, m_manager.h323EP->IsH245TunnelingDisabled() != false);
  INIT_FIELD(DisableH245inSETUP, m_manager.h323EP->IsH245inSetupDisabled() != false);

  INIT_FIELD(ExtendedVideoRoles, m_manager.m_ExtendedVideoRoles);
  INIT_FIELD(EnableH239Control, m_manager.h323EP->GetDefaultH239Control());

#if OPAL_PTLIB_SSL
  m_H323SignalingSecurity = 0;
  if (m_manager.FindEndPoint("h323") != NULL)
    m_H323SignalingSecurity |= 1;
  if (m_manager.FindEndPoint("h323s") != NULL)
    m_H323SignalingSecurity |= 2;
  --m_H323SignalingSecurity;
  FindWindowByName(H323SignalingSecurityKey)->SetValidator(wxGenericValidator(&m_H323SignalingSecurity));
  m_H323MediaCryptoSuites = FindWindowByNameAs<wxCheckListBox>(this, H323MediaCryptoSuitesKey);
  m_H323MediaCryptoSuiteUp = FindWindowByNameAs<wxButton>(this, wxT("H323MediaCryptoSuiteUp"));
  m_H323MediaCryptoSuiteDown = FindWindowByNameAs<wxButton>(this, wxT("H323MediaCryptoSuiteDown"));
  InitSecurityFields(m_manager.h323EP, m_H323MediaCryptoSuites, m_H323SignalingSecurity > 0);
#else
  FindWindowByName(H323SignalingSecurityKey)->Disable();
  FindWindowByName(H323MediaCryptoSuitesKey)->Disable();
#endif // OPAL_PTLIB_SSL

  INIT_FIELD(GatekeeperMode, m_manager.m_gatekeeperMode);
  INIT_FIELD(GatekeeperAddress, m_manager.m_gatekeeperAddress);
  INIT_FIELD(GatekeeperIdentifier, m_manager.m_gatekeeperIdentifier);
  INIT_FIELD(GatekeeperTTL, m_manager.h323EP->GetGatekeeperTimeToLive().GetSeconds());
  INIT_FIELD(GatekeeperLogin, m_manager.h323EP->GetGatekeeperUsername());
  INIT_FIELD(GatekeeperPassword, m_manager.h323EP->GetGatekeeperPassword());
#endif

#if OPAL_SIP
  ////////////////////////////////////////
  // SIP fields
  INIT_FIELD(SIPProxyUsed, m_manager.m_SIPProxyUsed);
  INIT_FIELD(SIPProxy, m_manager.sipEP->GetProxy().GetHostName());
  INIT_FIELD(SIPProxyUsername, m_manager.sipEP->GetProxy().GetUserName());
  INIT_FIELD(SIPProxyPassword, m_manager.sipEP->GetProxy().GetPassword());
  INIT_FIELD(LineAppearanceCode, m_manager.sipEP->GetDefaultAppearanceCode());
  INIT_FIELD(SIPUserInputMode, m_manager.sipEP->GetSendUserInputMode());
  INIT_FIELD(SIPPRACKMode, m_manager.sipEP->GetDefaultPRACKMode());
  if (m_SIPUserInputMode >= OpalConnection::SendUserInputAsProtocolDefault)
    m_SIPUserInputMode = OpalConnection::SendUserInputAsRFC2833;
  m_SIPUserInputMode--; // No SendUserInputAsQ931 mode, so decrement

#if OPAL_PTLIB_SSL
  m_SIPSignalingSecurity = 0;
  if (m_manager.FindEndPoint("sip") != NULL)
    m_SIPSignalingSecurity |= 1;
  if (m_manager.FindEndPoint("sips") != NULL)
    m_SIPSignalingSecurity |= 2;
  --m_SIPSignalingSecurity;
  FindWindowByName(SIPSignalingSecurityKey)->SetValidator(wxGenericValidator(&m_SIPSignalingSecurity));
  m_SIPMediaCryptoSuites = FindWindowByNameAs<wxCheckListBox>(this, SIPMediaCryptoSuitesKey);
  m_SIPMediaCryptoSuiteUp = FindWindowByNameAs<wxButton>(this, wxT("SIPMediaCryptoSuiteUp"));
  m_SIPMediaCryptoSuiteDown = FindWindowByNameAs<wxButton>(this, wxT("SIPMediaCryptoSuiteDown"));
  InitSecurityFields(m_manager.sipEP, m_SIPMediaCryptoSuites, m_SIPSignalingSecurity > 0);
#else
  FindWindowByName(SIPSignalingSecurityKey)->Disable();
  FindWindowByName(SIPMediaCryptoSuitesKey)->Disable();
#endif // OPAL_PTLIB_SSL

  m_SelectedRegistration = INT_MAX;

  m_Registrations = FindWindowByNameAs<wxListCtrl>(this, wxT("Registrars"));
  m_Registrations->InsertColumn(0, _T("Item"));
  m_Registrations->InsertColumn(1, _T("Status"));
  m_Registrations->InsertColumn(2, _T("Type"));
  m_Registrations->InsertColumn(3, _T("User"));
  m_Registrations->InsertColumn(4, _T("Domain/Host"));
  m_Registrations->InsertColumn(5, _T("Auth ID"));
  m_Registrations->InsertColumn(6, _T("Refresh"));
  for (RegistrationList::iterator registration = m_manager.m_registrations.begin(); registration != m_manager.m_registrations.end(); ++registration)
    RegistrationToList(true, new RegistrationInfo(*registration), INT_MAX);
  m_Registrations->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
  m_Registrations->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
  m_Registrations->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
  m_Registrations->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
  m_Registrations->SetColumnWidth(4, wxLIST_AUTOSIZE_USEHEADER);
  m_Registrations->SetColumnWidth(5, wxLIST_AUTOSIZE_USEHEADER);
  m_Registrations->SetColumnWidth(6, 60);

  m_ChangeRegistration = FindWindowByNameAs<wxButton>(this, wxT("ChangeRegistrar"));
  m_RemoveRegistration = FindWindowByNameAs<wxButton>(this, wxT("RemoveRegistrar"));
  m_MoveUpRegistration = FindWindowByNameAs<wxButton>(this, wxT("MoveUpRegistrar"));
  m_MoveDownRegistration = FindWindowByNameAs<wxButton>(this, wxT("MoveDownRegistrar"));
#endif // OPAL_SIP


  ////////////////////////////////////////
  // Routing fields
  INIT_FIELD(ForwardingAddress, m_manager.m_ForwardingAddress);
  INIT_FIELD(ForwardingTimeout, m_manager.m_ForwardingTimeout);

  m_SelectedRoute = INT_MAX;

  m_RouteDevice = FindWindowByNameAs<wxTextCtrl>(this, wxT("RouteDevice"));
  m_RoutePattern = FindWindowByNameAs<wxTextCtrl>(this, wxT("RoutePattern"));
  m_RouteDestination = FindWindowByNameAs<wxTextCtrl>(this, wxT("RouteDestination"));

  m_AddRoute = FindWindowByNameAs<wxButton>(this, wxT("AddRoute"));
  m_AddRoute->Disable();
  m_RemoveRoute = FindWindowByNameAs<wxButton>(this, wxT("RemoveRoute"));
  m_RemoveRoute->Disable();
  m_MoveUpRoute = FindWindowByNameAs<wxButton>(this, wxT("MoveUpRoute"));
  m_MoveUpRoute->Disable();
  m_MoveDownRoute = FindWindowByNameAs<wxButton>(this, wxT("MoveDownRoute"));
  m_MoveDownRoute->Disable();

  // Fill list box with active routes
  m_Routes = FindWindowByNameAs<wxListCtrl>(this, wxT("Routes"));
  m_Routes->InsertColumn(0, _T("Source"));
  m_Routes->InsertColumn(1, _T("Dev/If"));
  m_Routes->InsertColumn(2, _T("Pattern"));
  m_Routes->InsertColumn(3, _T("Destination"));
  const OpalManager::RouteTable & routeTable = m_manager.GetRouteTable();
  for (i = 0; i < routeTable.GetSize(); i++)
    AddRouteTableEntry(routeTable[i]);

  for (i = 0; i < m_Routes->GetColumnCount(); i++)
    m_Routes->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);

  {
    OpalEndPoint * ep = m_manager.FindEndPoint("tel");
    if (ep != NULL)
      m_telURI = ep->GetPrefixName();
    combo = FindWindowByNameAs<wxComboBox>(this, telURIKey);
    combo->Append(wxEmptyString);
    combo->SetValidator(wxGenericValidator(&m_telURI));
  }

  // Fill combo box with possible protocols
  m_RouteSource = FindWindowByNameAs<wxComboBox>(this, wxT("RouteSource"));
  m_RouteSource->Append(AllRouteSources);
  PList<OpalEndPoint> endpoints = m_manager.GetEndPoints();
  for (i = 0; i < endpoints.GetSize(); i++) {
    PwxString prefix(endpoints[i].GetPrefixName());
    m_RouteSource->Append(prefix);
    if (endpoints[i].HasAttribute(OpalEndPoint::SupportsE164))
      combo->Append(prefix);
  }
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
  INIT_FIELD(TraceObjectInstance, (PTrace::GetOptions()&PTrace::ObjectInstance) != 0);
  INIT_FIELD(TraceContextId, (PTrace::GetOptions()&PTrace::ContextIdentifier) != 0);
  INIT_FIELD(TraceFileName, m_manager.m_traceFileName);
#else
  RemoveNotebookPage(this, wxT("Tracing"));
#endif // PTRACING

  PTRACE(4, "OpenPhone\tInitialised options dialog");
}


OptionsDialog::~OptionsDialog()
{
  if (m_TestVideoThread != NULL) {
    m_TestVideoGrabber->Close();
    m_TestVideoDisplay->Close();
    m_TestVideoThread->WaitForTermination();
    delete m_TestVideoThread;
  }

  long i;
  for (i = 0; i < m_Presentities->GetItemCount(); ++i)
    delete (OpalPresentity *)m_Presentities->GetItemData(i);
  for (i = 0; i < m_Registrations->GetItemCount(); ++i)
    delete (RegistrationInfo *)m_Registrations->GetItemData(i);
}


#define SAVE_FIELD(name, set) \
  set(m_##name); \
  config->Write(name##Key, m_##name)

#define SAVE_FIELD_STR(name, set) \
  set(m_##name.p_str()); \
  config->Write(name##Key, m_##name)

#define SAVE_FIELD2(name1, name2, set) \
  set(m_##name1, m_##name2); \
  config->Write(name1##Key, m_##name1); \
  config->Write(name2##Key, m_##name2)


#if OPAL_PTLIB_SSL
static void SaveSecurityFields(OpalEndPoint * ep, wxCheckListBox * listbox, bool securedSignaling,
                               wxConfigBase * config, const wxChar * securedSignalingKey, const wxChar * mediaCryptoSuitesKey)
{
  config->Write(securedSignalingKey, securedSignaling);

  PStringArray allMethods = ep->GetAllMediaCryptoSuites();
  PStringArray enabledMethods;
  for (unsigned item = 0; item < listbox->GetCount(); ++item) {
    if (listbox->IsChecked(item)) {
      PString description = PwxString(listbox->GetString(item));
      for (PINDEX i = 0; i < allMethods.GetSize(); ++i) {
        OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(allMethods[i]);
        if (cryptoSuite != NULL && description == cryptoSuite->GetDescription()) {
          enabledMethods.AppendString(allMethods[i]);
          break;
        }
      }
    }
  }

  ep->SetMediaCryptoSuites(enabledMethods);
  PStringStream strm;
  strm << setfill(',') << enabledMethods;
  config->Write(mediaCryptoSuitesKey, PwxString(strm));
}
#endif // OPAL_PTLIB_SSL


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
  SAVE_FIELD(RingSoundDeviceName, m_manager.m_RingSoundDeviceName = AudioDeviceNameFromScreen);
  SAVE_FIELD(RingSoundFileName, m_manager.m_RingSoundFileName = );
  SAVE_FIELD(AutoAnswer, m_manager.m_autoAnswer = );

  OpalProductInfo productInfo = m_manager.GetProductInfo();
  SAVE_FIELD_STR(VendorName, productInfo.vendor = );
  SAVE_FIELD_STR(ProductName, productInfo.name = );
  SAVE_FIELD_STR(ProductVersion, productInfo.version = );
  m_manager.SetProductInfo(productInfo);

#if OPAL_IVR
  SAVE_FIELD(IVRScript, m_manager.ivrEP->SetDefaultVXML);
#endif

  SAVE_FIELD(AudioRecordingMode, m_manager.m_recordingOptions.m_stereo = 0 != );
  m_manager.m_recordingOptions.m_audioFormat = m_AudioRecordingFormat.p_str();
  config->Write(AudioRecordingFormatKey, m_AudioRecordingFormat);
  SAVE_FIELD(VideoRecordingMode, m_manager.m_recordingOptions.m_videoMixing = (OpalRecordManager::VideoMode));
  PVideoFrameInfo::ParseSize(m_VideoRecordingSize,
                             m_manager.m_recordingOptions.m_videoWidth,
                             m_manager.m_recordingOptions.m_videoHeight);
  config->Write(VideoRecordingSizeKey, m_VideoRecordingSize);

  ////////////////////////////////////////
  // Networking fields
  config->SetPath(NetworkingGroup);
  int adjustedBandwidth = (int)(floatBandwidth*10);
#if OPAL_H323
  m_manager.h323EP->SetInitialBandwidth(adjustedBandwidth);
#endif
  config->Write(BandwidthKey, adjustedBandwidth);
  SAVE_FIELD(RTPTOS, m_manager.SetMediaTypeOfService);
  SAVE_FIELD(MaxRtpPayloadSize, m_manager.SetMaxRtpPayloadSize);
#if OPAL_PTLIB_SSL
  SAVE_FIELD(CertificateAuthority, m_manager.SetSSLCertificateAuthorityFile);
  SAVE_FIELD(LocalCertificate, m_manager.SetSSLCertificateFile);
  SAVE_FIELD(PrivateKey, m_manager.SetSSLPrivateKeyFile);
#endif
  SAVE_FIELD2(TCPPortBase, TCPPortMax, m_manager.SetTCPPorts);
  SAVE_FIELD2(UDPPortBase, UDPPortMax, m_manager.SetUDPPorts);
  SAVE_FIELD2(RTPPortBase, RTPPortMax, m_manager.SetRtpIpPorts);
  m_manager.m_NATHandling = m_STUNServerRadio->GetValue() ? 2 : m_NATRouterRadio->GetValue() ? 1 : 0;
  config->Write(NATHandlingKey, m_manager.m_NATHandling);
  SAVE_FIELD(STUNServer, m_manager.m_STUNServer = );
  SAVE_FIELD(NATRouter, m_manager.m_NATRouter = );
  m_manager.SetNATHandling();

  config->DeleteGroup(LocalInterfacesGroup);
  config->SetPath(LocalInterfacesGroup);
  vector<PwxString> newInterfaces(m_LocalInterfaces->GetCount());
  bool changed = m_manager.m_LocalInterfaces.size() != newInterfaces.size();
  for (size_t i = 0; i < newInterfaces.size(); i++) {
    newInterfaces[i] = m_LocalInterfaces->GetString(i);
    vector<PwxString>::iterator old = std::find(m_manager.m_LocalInterfaces.begin(),
                                                m_manager.m_LocalInterfaces.end(),
                                                newInterfaces[i]);
    if (old == m_manager.m_LocalInterfaces.end() || newInterfaces[i] != *old)
      changed = true;
    wxString key;
    key.sprintf(wxT("%u"), i+1);
    config->Write(key, PwxString(newInterfaces[i]));
  }
  if (changed) {
    m_manager.m_LocalInterfaces = newInterfaces;
    m_manager.StartAllListeners();
  }

  ////////////////////////////////////////
  // Sound fields
  config->SetPath(AudioGroup);
  if (m_manager.pcssEP->SetSoundChannelPlayDevice(AudioDeviceNameFromScreen(m_SoundPlayer)))
    config->Write(SoundPlayerKey, PwxString(m_manager.pcssEP->GetSoundChannelPlayDevice()));
  else
    wxMessageBox(wxT("Could not use sound player device."), wxT("OpenPhone Options"), wxCANCEL|wxICON_EXCLAMATION);
  if (m_manager.pcssEP->SetSoundChannelRecordDevice(AudioDeviceNameFromScreen(m_SoundRecorder)))
    config->Write(SoundRecorderKey, PwxString(m_manager.pcssEP->GetSoundChannelRecordDevice()));
  else
    wxMessageBox(wxT("Could not use sound recorder device."), wxT("OpenPhone Options"), wxCANCEL|wxICON_EXCLAMATION);
  SAVE_FIELD(SoundBufferTime, m_manager.pcssEP->SetSoundChannelBufferTime);
  SAVE_FIELD2(MinJitter, MaxJitter, m_manager.SetAudioJitterDelay);

  OpalSilenceDetector::Params silenceParams;
  SAVE_FIELD(SilenceSuppression, silenceParams.m_mode=(OpalSilenceDetector::Mode));
  SAVE_FIELD(SilenceThreshold, silenceParams.m_threshold=);
  SAVE_FIELD(SignalDeadband, silenceParams.m_signalDeadband=);
  SAVE_FIELD(SilenceDeadband, silenceParams.m_silenceDeadband=);
  m_manager.SetSilenceDetectParams(silenceParams);

  SAVE_FIELD(DisableDetectInBandDTMF, m_manager.DisableDetectInBandDTMF);

  if (m_LineInterfaceDevice.StartsWith(wxT("<<")) && m_LineInterfaceDevice.EndsWith(wxT(">>")))
    m_LineInterfaceDevice.Empty();
  config->Write(LineInterfaceDeviceKey, m_LineInterfaceDevice);
  config->Write(AECKey, m_AEC);
  config->Write(CountryKey, m_Country);
  m_manager.StartLID();

  ////////////////////////////////////////
  // Video fields
  config->SetPath(VideoGroup);
  PVideoDevice::OpenArgs grabber = m_manager.GetVideoInputDevice();
  SAVE_FIELD_STR(VideoGrabDevice, grabber.deviceName = );
  SAVE_FIELD(VideoGrabFormat, grabber.videoFormat = (PVideoDevice::VideoFormat));
  --m_VideoGrabSource;
  SAVE_FIELD(VideoGrabSource, grabber.channelNumber = );
  SAVE_FIELD(VideoGrabFrameRate, grabber.rate = );
  SAVE_FIELD(VideoGrabFrameSize, m_manager.m_VideoGrabFrameSize = );
  SAVE_FIELD(VideoFlipLocal, grabber.flip = );
  m_manager.SetVideoInputDevice(grabber);
  delete m_manager.m_primaryVideoGrabber;
  m_manager.m_primaryVideoGrabber = PVideoInputDevice::CreateOpenedDevice(grabber, false);
  if (m_manager.m_primaryVideoGrabber != NULL)
    m_manager.m_primaryVideoGrabber->Stop();

  SAVE_FIELD(VideoGrabPreview, m_manager.m_VideoGrabPreview = );
  SAVE_FIELD(VideoAutoTransmit, m_manager.SetAutoStartTransmitVideo);
  SAVE_FIELD(VideoAutoReceive, m_manager.SetAutoStartReceiveVideo);
//  SAVE_FIELD(VideoFlipRemote, );
  SAVE_FIELD(VideoMinFrameSize, m_manager.m_VideoMinFrameSize = );
  SAVE_FIELD(VideoMaxFrameSize, m_manager.m_VideoMaxFrameSize = );
  SAVE_FIELD(VideoGrabBitRate, m_manager.m_VideoTargetBitRate = );
  SAVE_FIELD(VideoMaxBitRate, m_manager.m_VideoMaxBitRate = );

  ////////////////////////////////////////
  // Fax fields
#if OPAL_FAX
  config->SetPath(FaxGroup);
  SAVE_FIELD(FaxStationIdentifier, m_manager.m_faxEP->SetDefaultDisplayName);
  SAVE_FIELD(FaxReceiveDirectory, m_manager.m_faxEP->SetDefaultDirectory);
  SAVE_FIELD(FaxAutoAnswerMode, m_manager.m_defaultAnswerMode = (MyManager::FaxAnswerModes));
#endif

  ////////////////////////////////////////
  // Presence fields
  PStringList activePresentities = m_manager.GetPresentities();
  config->DeleteGroup(PresenceGroup);
  for (int item = 0; item < m_Presentities->GetItemCount(); ++item) {
    PwxString aor = m_Presentities->GetItemText(item);

    wxString group;
    group.sprintf(wxT("%s/%04u"), PresenceGroup, item);
    config->SetPath(group);
    config->Write(PresenceAORKey, aor);

    OpalPresentity * presentity = (OpalPresentity *)m_Presentities->GetItemData(item);
    PStringArray attributeNames = presentity->GetAttributeNames();
    attributeNames.AppendString(PresenceActiveKey);
    for (PINDEX i = 0; i < attributeNames.GetSize(); ++i) {
      PString name = attributeNames[i];
      if (presentity->GetAttributes().Has(name))
        config->Write(PwxString(name), PwxString(presentity->GetAttributes().Get(name)));
    }

    PSafePtr<OpalPresentity> activePresentity = m_manager.AddPresentity(aor);
    if (activePresentity != NULL) {
      activePresentity->GetAttributes() = presentity->GetAttributes();
      if (presentity->GetAttributes().GetBoolean(PresenceActiveKey))
        LogWindow << (activePresentity->Open() ? "Establishing" : "Could not establish")
                  << " presence for identity " << aor << endl;
      else if (activePresentity->IsOpen()) {
        activePresentity->Close();
        LogWindow << "Stopping presence for identity " << aor << endl;
      }
    }

    PStringList::iterator pos = activePresentities.find(aor.p_str());
    if (pos != activePresentities.end())
      activePresentities.erase(pos);
  }
  for (PStringList::iterator it = activePresentities.begin(); it != activePresentities.end(); ++it)
    m_manager.RemovePresentity(*it);

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
      groupName.sprintf(wxT("%s/%04u"), CodecsGroup, mm->preferenceOrder);
      config->SetPath(groupName);
      config->Write(CodecNameKey, PwxString(mm->mediaFormat));
      for (PINDEX i = 0; i < mm->mediaFormat.GetOptionCount(); i++) {
        const OpalMediaOption & option = mm->mediaFormat.GetOption(i);
        if (!option.IsReadOnly())
          config->Write(PwxString(option.GetName()), PwxString(option.AsString()));
      }
    }
  }


#if OPAL_H323
  ////////////////////////////////////////
  // H.323 fields
  config->DeleteGroup(H323AliasesGroup);
  config->SetPath(H323AliasesGroup);
  m_manager.h323EP->SetLocalUserName(m_Username);
  PStringList aliases = m_manager.h323EP->GetAliasNames();
  for (size_t i = 0; i < m_Aliases->GetCount(); i++) {
    PwxString alias = m_Aliases->GetString(i);
    m_manager.h323EP->AddAliasName(alias);
    wxString key;
    key.sprintf(wxT("%u"), i+1);
    config->Write(key, alias);
  }

  config->SetPath(H323Group);
  m_manager.h323EP->SetSendUserInputMode((H323Connection::SendUserInputModes)m_DTMFSendMode);
  config->Write(DTMFSendModeKey, m_DTMFSendMode);
#if OPAL_450
  SAVE_FIELD(CallIntrusionProtectionLevel, m_manager.h323EP->SetCallIntrusionProtectionLevel);
#endif
  SAVE_FIELD(DisableFastStart, m_manager.h323EP->DisableFastStart);
  SAVE_FIELD(DisableH245Tunneling, m_manager.h323EP->DisableH245Tunneling);
  SAVE_FIELD(DisableH245inSETUP, m_manager.h323EP->DisableH245inSetup);

  SAVE_FIELD(ExtendedVideoRoles, m_manager.m_ExtendedVideoRoles = (MyManager::ExtendedVideoRoles));
  SAVE_FIELD(EnableH239Control, m_manager.h323EP->SetDefaultH239Control);

#if OPAL_PTLIB_SSL
  SaveSecurityFields(m_manager.h323EP, m_H323MediaCryptoSuites, m_H323SignalingSecurity > 0,
                     config, H323SignalingSecurityKey, H323MediaCryptoSuitesKey);
  m_manager.SetSignalingSecurity(m_manager.h323EP, m_H323SignalingSecurity, "h323", "h323s");
  config->Write(H323SignalingSecurityKey, m_H323SignalingSecurity);
#endif

  config->Write(GatekeeperTTLKey, m_GatekeeperTTL);
  m_manager.h323EP->SetGatekeeperTimeToLive(PTimeInterval(0, m_GatekeeperTTL));

  if (m_manager.m_gatekeeperMode != m_GatekeeperMode ||
      m_manager.m_gatekeeperAddress != m_GatekeeperAddress ||
      m_manager.m_gatekeeperIdentifier != m_GatekeeperIdentifier ||
      PwxString(m_manager.h323EP->GetGatekeeperUsername()) != m_GatekeeperLogin ||
      PwxString(m_manager.h323EP->GetGatekeeperPassword()) != m_GatekeeperPassword) {
    SAVE_FIELD(GatekeeperMode, m_manager.m_gatekeeperMode = );
    SAVE_FIELD(GatekeeperAddress, m_manager.m_gatekeeperAddress = );
    SAVE_FIELD(GatekeeperIdentifier, m_manager.m_gatekeeperIdentifier = );
    SAVE_FIELD2(GatekeeperPassword, GatekeeperLogin, m_manager.h323EP->SetGatekeeperPassword);

    if (!m_manager.StartGatekeeper())
      m_manager.Close();
  }
#endif

#if OPAL_SIP
  ////////////////////////////////////////
  // SIP fields
  config->SetPath(SIPGroup);
  SAVE_FIELD(SIPProxyUsed, m_manager.m_SIPProxyUsed =);
  m_manager.sipEP->SetProxy(m_SIPProxy, m_SIPProxyUsername, m_SIPProxyPassword);
  config->Write(SIPProxyKey, m_SIPProxy);
  config->Write(SIPProxyUsernameKey, m_SIPProxyUsername);
  config->Write(SIPProxyPasswordKey, m_SIPProxyPassword);
  SAVE_FIELD(LineAppearanceCode, m_manager.sipEP->SetDefaultAppearanceCode);
  m_manager.sipEP->SetSendUserInputMode((OpalConnection::SendUserInputModes)(m_SIPUserInputMode+1));
  config->Write(SIPUserInputModeKey, m_SIPUserInputMode+1);
  m_manager.sipEP->SetDefaultPRACKMode((SIPConnection::PRACKMode)m_SIPPRACKMode);
  config->Write(SIPPRACKModeKey, m_SIPPRACKMode);

#if OPAL_PTLIB_SSL
  SaveSecurityFields(m_manager.sipEP, m_SIPMediaCryptoSuites, m_SIPSignalingSecurity > 0,
                     config, SIPSignalingSecurityKey, SIPMediaCryptoSuitesKey);
  m_manager.SetSignalingSecurity(m_manager.sipEP, m_SIPSignalingSecurity, "sip", "sips");
  config->Write(SIPSignalingSecurityKey, m_SIPSignalingSecurity);
#endif

  RegistrationList newRegistrations;

  for (int i = 0; i < m_Registrations->GetItemCount(); ++i)
    newRegistrations.push_back(*(RegistrationInfo *)m_Registrations->GetItemData(i));

  if (newRegistrations != m_manager.m_registrations) {
    config->DeleteEntry(RegistrarUsedKey);
    config->DeleteEntry(RegistrarNameKey);
    config->DeleteEntry(RegistrarUsernameKey);
    config->DeleteEntry(RegistrarPasswordKey);
    config->DeleteEntry(RegistrarProxyKey);
    config->DeleteGroup(RegistrarGroup);

    int registrationIndex = 1;
    RegistrationList::iterator iterReg;
    for (iterReg = newRegistrations.begin(); iterReg != newRegistrations.end(); ++iterReg) {
      wxString group;
      group.sprintf(wxT("%s/%04u"), RegistrarGroup, registrationIndex++);
      config->SetPath(group);
      iterReg->Write(*config);
    }

    m_manager.ReplaceRegistrations(newRegistrations);
  }
#endif // OPAL_SIP

  ////////////////////////////////////////
  // Routing fields
  config->SetPath(GeneralGroup);
  SAVE_FIELD(ForwardingAddress, m_manager.m_ForwardingAddress =);
  SAVE_FIELD(ForwardingTimeout, m_manager.m_ForwardingTimeout =);

  config->Write(telURIKey, m_telURI);
  m_manager.DetachEndPoint("tel");
  if (!m_telURI.empty())
    m_manager.AttachEndPoint(m_manager.FindEndPoint(m_telURI), "tel");

  config->DeleteGroup(RoutingGroup);
  config->SetPath(RoutingGroup);
  PStringArray routeSpecs;
  for (int i = 0; i < m_Routes->GetItemCount(); i++) {
    PwxString spec;
    wxListItem item;
    item.m_itemId = i;
    item.m_mask = wxLIST_MASK_TEXT;
    m_Routes->GetItem(item);
    spec += (item.m_text == wxT("<ALL>")) ? wxT(".*") : item.m_text;
    spec += ':';
    item.m_col++;
    m_Routes->GetItem(item);
    spec += item.m_text.empty() ? wxT(".*") : item.m_text;
    spec += '\t';
    item.m_col++;
    m_Routes->GetItem(item);
    spec += item.m_text;
    spec += '=';
    item.m_col++;
    m_Routes->GetItem(item);
    spec += item.m_text;
    routeSpecs.AppendString(spec);

    wxString key;
    key.sprintf(wxT("%04u"), i+1);
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
  if (m_TraceObjectInstance)
    traceOptions |= PTrace::ObjectInstance;
  if (m_TraceContextId)
    traceOptions |= PTrace::ContextIdentifier;
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
    PTrace::Initialise(m_TraceLevelThreshold, m_TraceFileName.ToUTF8(), traceOptions);
  else {
    PTrace::SetLevel(m_TraceLevelThreshold);
    PTrace::SetOptions(traceOptions);
    PTrace::ClearOptions(~traceOptions);
  }

  m_manager.m_enableTracing = m_EnableTracing;
  m_manager.m_traceFileName = m_TraceFileName;
#endif // PTRACING

  m_manager.AdjustVideoFormats();

  config->Flush();

  ::wxEndBusyCursor();

  return true;
}


////////////////////////////////////////
// General fields

void OptionsDialog::BrowseSoundFile(wxCommandEvent & /*event*/)
{
  wxString newFile = wxFileSelector(wxT("Sound file to play on incoming calls"),
                                    wxT(""),
                                    m_RingSoundFileName,
                                    wxT(".wav"),
                                    wxT("WAV files (*.wav)|*.wav"),
                                    wxFD_OPEN|wxFD_FILE_MUST_EXIST);
  if (!newFile.empty()) {
    m_RingSoundFileName = newFile;
    TransferDataToWindow();
  }
}


void OptionsDialog::PlaySoundFile(wxCommandEvent & /*event*/)
{
  if (!wxDialog::TransferDataFromWindow())
    return;

  PSoundChannel speaker(m_RingSoundDeviceName, PSoundChannel::Player);
  speaker.PlayFile(m_RingSoundFileName);
}


////////////////////////////////////////
// Networking fields

void OptionsDialog::BandwidthClass(wxCommandEvent & event)
{
  static const wxChar * bandwidthClasses[] = {
    wxT("14.4"), wxT("28.8"), wxT("64.0"), wxT("128"), wxT("1500"), wxT("10000")
  };

  m_Bandwidth = bandwidthClasses[event.GetSelection()];
  TransferDataToWindow();
}


void OptionsDialog::FindCertificateAuthority(wxCommandEvent &)
{
  wxString newFile = wxFileSelector(wxT("File or directory for Certificate Authority"),
                                    wxT(""),
                                    m_CertificateAuthority,
                                    wxT(".cer"),
                                    wxT("CER files (*.cer)|*.cer|PEM files (*.pem)|*.pem"),
                                    wxFD_OPEN|wxFD_FILE_MUST_EXIST);
  if (!newFile.empty()) {
    m_CertificateAuthority = newFile;
    TransferDataToWindow();
  }
}


void OptionsDialog::FindLocalCertificate(wxCommandEvent &)
{
  wxString newFile = wxFileSelector(wxT("File or directory for local Certificate"),
                                    wxT(""),
                                    m_LocalCertificate,
                                    wxT(".cer"),
                                    wxT("CER files (*.cer)|*.cer|PEM files (*.pem)|*.pem"),
                                    wxFD_OPEN|wxFD_FILE_MUST_EXIST);
  if (!newFile.empty()) {
    m_LocalCertificate = newFile;
    TransferDataToWindow();
  }
}


void OptionsDialog::FindPrivateKey(wxCommandEvent &)
{
  wxString newFile = wxFileSelector(wxT("File or directory for private key"),
                                    wxT(""),
                                    m_PrivateKey,
                                    wxT(".key"),
                                    wxT("KEY files (*.key)|*.key|PEM files (*.pem)|*.pem"),
                                    wxFD_OPEN|wxFD_FILE_MUST_EXIST);
  if (!newFile.empty()) {
    m_PrivateKey = newFile;
    TransferDataToWindow();
  }
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
  PwxString iface = m_InterfaceAddress->GetValue();
  if (iface.IsEmpty())
    enab = false;
  else if (iface != "*") {
    PIPSocket::Address test;
    if (!test.FromString(iface))
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
  "all:", "h323:tcp$", "h323:tls$", "sip:udp$", "sip:tcp$", "sip:tls$"
};

void OptionsDialog::AddInterface(wxCommandEvent & /*event*/)
{
  int proto = m_InterfaceProtocol->GetSelection();
  PwxString iface = InterfacePrefixes[proto];
  iface += m_InterfaceAddress->GetValue();
  if (proto > 0) 
    iface += wxT(":") + m_InterfacePort->GetValue();
  m_LocalInterfaces->Append(iface);
}


void OptionsDialog::RemoveInterface(wxCommandEvent & /*event*/)
{
  wxString iface = m_LocalInterfaces->GetStringSelection();

  for (int i = 0; i < PARRAYSIZE(InterfacePrefixes); i++) {
    if (iface.StartsWith(PwxString(InterfacePrefixes[i]))) {
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

static void SetComboValue(wxComboBox * combo, const wxString & value)
{
  // For some bizarre reason combo->SetValue() just does not work for Windows
  int idx = combo->FindString(value);
  if (idx < 0)
    idx = combo->Append(value);
  combo->SetSelection(idx);
}

void OptionsDialog::ChangedSoundPlayer(wxCommandEvent & /*event*/)
{
  PwxString device = m_soundPlayerCombo->GetValue();
  if (device[0] != '*')
    return;

 device = wxFileSelector(wxT("Select Sound File"),
                          wxT(""),
                          wxT(""),
                          device.Mid(1),
                          device,
                          wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
  if (!device.empty())
    m_SoundPlayer = device;

  SetComboValue(m_soundPlayerCombo, m_SoundPlayer);
}


void OptionsDialog::ChangedSoundRecorder(wxCommandEvent & /*event*/)
{
  PwxString device = m_soundRecorderCombo->GetValue();
  if (device[0] != '*')
    return;

 device = wxFileSelector(wxT("Select Sound File"),
                          wxT(""),
                          wxT(""),
                          device.Mid(1),
                          device,
                          wxFD_OPEN|wxFD_FILE_MUST_EXIST);
  if (!device.empty())
    m_SoundRecorder = device;

  SetComboValue(m_soundRecorderCombo, m_SoundRecorder);
}


void OptionsDialog::SelectedLID(wxCommandEvent & /*event*/)
{
  bool enabled = m_selectedLID->GetSelection() > 0;
  m_selectedAEC->Enable(enabled);
  m_selectedCountry->Enable(enabled);

  if (enabled) {
    PwxString devName = m_selectedLID->GetValue();
    OpalLineInterfaceDevice * lidToDelete = NULL; 
    const OpalLineInterfaceDevice * lid = m_manager.potsEP->GetDeviceByName(devName);
    if (lid == NULL)
      lid = lidToDelete = OpalLineInterfaceDevice::CreateAndOpen(devName);
    if (lid != NULL) {
      m_selectedAEC->SetSelection(lid->GetAEC(0));

      m_selectedCountry->Clear();
      PStringList countries = lid->GetCountryCodeNameList();
      for (PStringList::iterator country = countries.begin(); country != countries.end(); ++country)
        m_selectedCountry->Append(PwxString(*country));
      m_selectedCountry->SetValue(PwxString(lid->GetCountryCodeName()));
    }
    delete lidToDelete;
  }
}


////////////////////////////////////////
// Video fields

void OptionsDialog::AdjustVideoControls(const PwxString & newDevice)
{
  PwxString device = newDevice;
  if (newDevice[0] == '*') {
    device = wxFileSelector(wxT("Select Video File"),
                            wxT(""),
                            wxT(""),
                            device.Mid(1),
                            device,
                            wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (device.empty())
      device = m_VideoGrabDevice;
    else
      m_VideoGrabDevice = device;
    SetComboValue(m_videoGrabDeviceCombo, device);
  }

  unsigned numChannels = 1;
  PVideoInputDevice * grabber = PVideoInputDevice::CreateDeviceByName(device);
  if (grabber != NULL) {
    numChannels = grabber->GetNumChannels()+1;
    delete grabber;
  }

  while (m_videoSourceChoice->GetCount() > numChannels)
    m_videoSourceChoice->Delete(m_videoSourceChoice->GetCount()-1);
  while (m_videoSourceChoice->GetCount() < numChannels)
    m_videoSourceChoice->Append(wxString((wxChar)(m_videoSourceChoice->GetCount()+'A'-1)));
}


void OptionsDialog::ChangeVideoGrabDevice(wxCommandEvent & /*event*/)
{
  AdjustVideoControls(m_videoGrabDeviceCombo->GetValue());
}


void OptionsDialog::TestVideoCapture(wxCommandEvent & /*event*/)
{
  if (m_TestVideoThread != NULL) {
    m_TestVideoGrabber->Close();
    m_TestVideoDisplay->Close();
    m_TestVideoThread->WaitForTermination();
    delete m_TestVideoThread;
    m_TestVideoThread = NULL;
    m_TestVideoCapture->SetLabel(wxT("Test Video"));
    return;
  }

  if (!wxDialog::TransferDataFromWindow())
    return;

  PVideoDevice::OpenArgs grabberArgs;
  grabberArgs.deviceName = m_VideoGrabDevice.mb_str(wxConvUTF8);
  grabberArgs.videoFormat = (PVideoDevice::VideoFormat)m_VideoGrabFormat;
  grabberArgs.channelNumber = m_VideoGrabSource-1;
  grabberArgs.rate = m_VideoGrabFrameRate;
  PVideoFrameInfo::ParseSize(m_VideoGrabFrameSize, grabberArgs.width, grabberArgs.height);
  grabberArgs.flip = m_VideoFlipLocal;

  PVideoInputDevice * grabber = PVideoInputDevice::CreateOpenedDevice(grabberArgs);
  if (grabber == NULL) {
    wxMessageBox(wxT("Could not open video capture."), wxT("OpenPhone Video Test"), wxCANCEL|wxICON_EXCLAMATION);
    return;
  }

  wxRect box(GetRect().GetBottomLeft(), wxSize(grabberArgs.width, grabberArgs.height));

  PVideoDevice::OpenArgs displayArgs;
  displayArgs.deviceName = psprintf(VIDEO_WINDOW_DEVICE" TITLE=\"Video Test\" X=%i Y=%i", box.GetLeft(), box.GetTop());
  displayArgs.width = grabberArgs.width;
  displayArgs.height = grabberArgs.height;

  PVideoOutputDevice * display = PVideoOutputDevice::CreateOpenedDevice(displayArgs, true);
  if (PAssertNULL(display) == NULL) {
    delete grabber;
    return;
  }

  if (grabber->Start()) {
    m_TestVideoCapture->SetLabel(wxT("Stop Video"));
    m_TestVideoGrabber = grabber;
    m_TestVideoDisplay = display;
    m_TestVideoThread = new PThreadObj<OptionsDialog>(*this, &OptionsDialog::TestVideoThreadMain, false, "TestVideo");
  }
  else {
    wxMessageBox(wxT("Could not start video capture."), wxT("OpenPhone Video Test"), wxCANCEL|wxICON_EXCLAMATION);
    delete display;
    delete grabber;
  }
}


void OptionsDialog::TestVideoThreadMain()
{
  PBYTEArray frame;
  unsigned frameCount = 0;
  while (m_TestVideoGrabber->GetFrame(frame) &&
         m_TestVideoDisplay->SetFrameData(0, 0,
                                          m_TestVideoGrabber->GetFrameWidth(),
                                          m_TestVideoGrabber->GetFrameHeight(),
                                          frame))
    frameCount++;

  delete m_TestVideoDisplay;
  m_TestVideoDisplay = NULL;

  delete m_TestVideoGrabber;
  m_TestVideoGrabber = NULL;
}


////////////////////////////////////////
// Fax fields

void OptionsDialog::BrowseFaxDirectory(wxCommandEvent & /*event*/)
{
  wxDirDialog dlg(this, wxT("Select Receive Directory for Faxes"), m_FaxReceiveDirectory);
  if (dlg.ShowModal() == wxID_OK) {
    m_FaxReceiveDirectory = dlg.GetPath();
    FindWindowByNameAs<wxTextCtrl>(this, wxT("FaxReceiveDirectory"))->SetLabel(m_FaxReceiveDirectory);
  }
}


////////////////////////////////////////
// Presence fields

static const wxChar NewPresenceStr[] = wxT("<new presence>");

void OptionsDialog::AddPresentity(wxCommandEvent & /*event*/)
{
  long pos = m_Presentities->InsertItem(INT_MAX, NewPresenceStr);
  if (pos >= 0)
    m_Presentities->EditLabel(pos);
}


void OptionsDialog::RemovePresentity(wxCommandEvent & /*event*/)
{
  long index = m_Presentities->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (index < 0)
    return;

  delete (OpalPresentity *)m_Presentities->GetItemData(index);
  m_Presentities->DeleteItem(index);
  FillPresentityAttributes(NULL);
}


void OptionsDialog::SelectedPresentity(wxListEvent & evt)
{
  PwxString aor = evt.GetText();
  if (aor != NewPresenceStr)
    FillPresentityAttributes((OpalPresentity *)evt.GetItem().m_data);
}


void OptionsDialog::DeselectedPresentity(wxListEvent & /*event*/)
{
  FillPresentityAttributes(NULL);
}


void OptionsDialog::EditedPresentity(wxListEvent & evt)
{
  long index = evt.GetIndex();
  wxString oldAOR = m_Presentities->GetItemText(index);
  PwxString newAOR = evt.GetText();

  if (!evt.IsEditCancelled() && !newAOR.IsEmpty() && newAOR != oldAOR) {
    OpalPresentity * presentity = OpalPresentity::Create(m_manager, newAOR);
    presentity->GetAttributes().Set(PresenceActiveKey, "Yes"); // OpenPhone extra attribute
    if (FillPresentityAttributes(presentity)) {
      delete (OpalPresentity *)m_Presentities->GetItemData(index);
      m_Presentities->SetItemPtrData(index, (wxUIntPtr)presentity);
      return;
    }

    wxMessageBox(wxT("Cannot create presence identity ")+newAOR);
  }

  if (oldAOR == NewPresenceStr)
    RemovePresentity(evt);
  else
    evt.Veto();
}


static PConstString DefaultAttributeValue("<<default>>");

bool OptionsDialog::FillPresentityAttributes(OpalPresentity * presentity)
{
  if (presentity == NULL) {
    int rows = m_PresentityAttributes->GetNumberRows();
    if (rows > 0)
      m_PresentityAttributes->DeleteRows(0, rows);
    m_PresentityAttributes->Disable();
    m_RemovePresentity->Disable();
    return false;
  }

  m_RemovePresentity->Enable();
  m_PresentityAttributes->Enable();

  PStringArray attributeNames = presentity->GetAttributeNames();
  PStringArray attributeTypes = presentity->GetAttributeTypes();
  attributeNames.InsertAt(0, new PString(PresenceActiveKey));
  attributeTypes.InsertAt(0, new PString("Enum\nNo,Yes\nYes"));

  m_PresentityAttributes->InsertRows(0, attributeNames.GetSize());
  for (PINDEX i = 0; i < attributeNames.GetSize(); ++i) {
    PString name = attributeNames[i];
    PStringArray attribute = attributeTypes[i].Lines();

    wxGridCellEditor * editor = NULL;
    PCaselessString attribType = attribute[0];
    if (attribType == "Integer")
      editor = new wxGridCellNumberEditor;
    else if (attribType == "Enum")
      editor = new wxGridCellChoiceEditor;
    if (editor != NULL) {
      if (attribute.GetSize() > 1)
        editor->SetParameters(PwxString(attribute[1]));
      m_PresentityAttributes->SetCellEditor(i, 0, editor);
    }

    PwxString value = presentity->GetAttributes().Get(name,
                            attribute.GetSize() > 2 ? attribute[2] : DefaultAttributeValue);
    m_PresentityAttributes->SetRowLabelValue(i, PwxString(name));
    m_PresentityAttributes->SetCellValue(value, i, 0);
  }
  m_PresentityAttributes->SetRowLabelSize(wxGRID_AUTOSIZE);
  m_PresentityAttributes->SetColSize(0, m_PresentityAttributes->GetSize().GetWidth() -
                                        m_PresentityAttributes->GetRowLabelSize() - 8);

  return true;
}


void OptionsDialog::ChangedPresentityAttribute(wxGridEvent & evt)
{
  long index = m_Presentities->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (index < 0)
    return;

  OpalPresentity * presentity = (OpalPresentity *)m_Presentities->GetItemData(index);

  PString name = PwxString(m_PresentityAttributes->GetRowLabelValue(evt.GetRow()));
  PwxString value = m_PresentityAttributes->GetCellValue(evt.GetRow(), 0);
  if (value != DefaultAttributeValue)
    presentity->GetAttributes().Set(name, value);
  else if (presentity->GetAttributes().Has(name))
    presentity->GetAttributes().RemoveAt(name);
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
    value.Replace(SIPonly, wxT(""));
    value.Replace(H323only, wxT(""));
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
  m_codecOptionValue->SetValue(wxT(""));
  m_codecOptionValue->Disable();
  m_CodecOptionValueLabel->Disable();

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
  m_CodecOptionValueLabel->Enable(!item.m_data);
  if (!item.m_data)
    m_codecOptionValue->SetValue(item.m_text);
}


void OptionsDialog::DeselectedCodecOption(wxListEvent & /*event*/)
{
  m_codecOptionValue->SetValue(wxT(""));
  m_codecOptionValue->Disable();
  m_CodecOptionValueLabel->Disable();
}


void OptionsDialog::ChangedCodecOptionValue(wxCommandEvent & /*event*/)
{
  wxListItem item;
  item.m_itemId = m_codecOptions->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (item.m_itemId < 0)
    return;

  item.m_mask = wxLIST_MASK_TEXT;
  item.m_col = 1;
  m_codecOptions->GetItem(item);

  PwxString newValue = m_codecOptionValue->GetValue();
  if (item.m_text == newValue)
    return;

  wxArrayInt selections;
  PAssert(m_selectedCodecs->GetSelections(selections) == 1, PLogicError);
  MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selections[0]);
  if (!PAssert(media != NULL, PLogicError))
    return;

  item.m_col = 0;
  m_codecOptions->GetItem(item);
  bool ok = media->mediaFormat.SetOptionValue(PwxString(item.m_text), newValue);
  if (ok) {
    item.m_col = 1;
    item.m_text = newValue;
    m_codecOptions->SetItem(item);
  }

  m_CodecOptionValueError->Show(!ok);
}


////////////////////////////////////////

#if OPAL_PTLIB_SSL
static void MediaCryptoSuiteChanged(wxCheckListBox * listbox, wxButton * up, wxButton * down)
{
  int selection = listbox->GetSelection();
  up->Enable(selection > 0);
  down->Enable((unsigned)selection < listbox->GetCount()-1);
}


static void MediaCryptoSuiteMove(wxCheckListBox * listbox, wxButton * up, wxButton * down, int dir)
{
  int selection = listbox->GetSelection();
  if (dir < 0 ? (selection <= 0) : ((unsigned)selection >= listbox->GetCount()-1))
    return;

  wxString str = listbox->GetString(selection);
  bool check = listbox->IsChecked(selection);

  listbox->Delete(selection);

  selection += dir;

  listbox->Insert(str, selection);
  listbox->Check(selection, check);

  MediaCryptoSuiteChanged(listbox, up, down);
}
#endif // OPAL_PTLIB_SSL


////////////////////////////////////////
// H.323 fields

#if OPAL_PTLIB_SSL
void OptionsDialog::H323SignalingSecurityChanged(wxCommandEvent & /*event*/)
{
  if (!wxDialog::TransferDataFromWindow())
    return;

  InitSecurityFields(m_manager.h323EP, m_H323MediaCryptoSuites, m_H323SignalingSecurity > 0);
  MediaCryptoSuiteChanged(m_H323MediaCryptoSuites, m_H323MediaCryptoSuiteUp, m_H323MediaCryptoSuiteDown);
}


void OptionsDialog::H323MediaCryptoSuiteChanged(wxCommandEvent & /*event*/)
{
  MediaCryptoSuiteChanged(m_H323MediaCryptoSuites, m_H323MediaCryptoSuiteUp, m_H323MediaCryptoSuiteDown);
}


void OptionsDialog::H323MediaCryptoSuiteUp(wxCommandEvent & /*event*/)
{
  MediaCryptoSuiteMove(m_H323MediaCryptoSuites, m_H323MediaCryptoSuiteUp, m_H323MediaCryptoSuiteDown, -1);
}


void OptionsDialog::H323MediaCryptoSuiteDown(wxCommandEvent & /*event*/)
{
  MediaCryptoSuiteMove(m_H323MediaCryptoSuites, m_H323MediaCryptoSuiteUp, m_H323MediaCryptoSuiteDown, +1);
}
#endif


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

#if OPAL_PTLIB_SSL
void OptionsDialog::SIPSignalingSecurityChanged(wxCommandEvent & /*event*/)
{
  if (!wxDialog::TransferDataFromWindow())
    return;

  InitSecurityFields(m_manager.sipEP, m_SIPMediaCryptoSuites, m_SIPSignalingSecurity > 0);
  MediaCryptoSuiteChanged(m_SIPMediaCryptoSuites, m_SIPMediaCryptoSuiteUp, m_SIPMediaCryptoSuiteDown);
}


void OptionsDialog::SIPMediaCryptoSuiteChanged(wxCommandEvent & /*event*/)
{
  MediaCryptoSuiteChanged(m_SIPMediaCryptoSuites, m_SIPMediaCryptoSuiteUp, m_SIPMediaCryptoSuiteDown);
}


void OptionsDialog::SIPMediaCryptoSuiteUp(wxCommandEvent & /*event*/)
{
  MediaCryptoSuiteMove(m_SIPMediaCryptoSuites, m_SIPMediaCryptoSuiteUp, m_SIPMediaCryptoSuiteDown, -1);
}


void OptionsDialog::SIPMediaCryptoSuiteDown(wxCommandEvent & /*event*/)
{
  MediaCryptoSuiteMove(m_SIPMediaCryptoSuites, m_SIPMediaCryptoSuiteUp, m_SIPMediaCryptoSuiteDown, +1);
}
#endif


static void RenumberList(wxListCtrl * list, int position)
{
  while (position < list->GetItemCount()) {
    wxString str;
    str.sprintf(wxT("%3u"), position+1);
    list->SetItem(position, 0, str);
    ++position;
  }
}


void OptionsDialog::RegistrationToList(bool create, RegistrationInfo * registration, int position)
{
  if (create) {
    position = m_Registrations->InsertItem(position, wxT("999"));
    RenumberList(m_Registrations, position);
    m_Registrations->SetItemPtrData(position, (wxUIntPtr)registration);
  }


  m_Registrations->SetItem(position, 2, RegistrationInfoTable[registration->m_Type].m_name);
  m_Registrations->SetItem(position, 3, registration->m_User);
  m_Registrations->SetItem(position, 4, registration->m_Domain);
  m_Registrations->SetItem(position, 5, registration->m_AuthID);

  wxString str;
  str.sprintf(wxT("%u:%02u"), registration->m_TimeToLive/60, registration->m_TimeToLive%60);
  m_Registrations->SetItem(position, 6, str);

  m_Registrations->SetItem(position, 1, registration->m_Active ? wxT("ACTIVE") : wxT("disabled"));
}


void OptionsDialog::AddRegistration(wxCommandEvent &)
{
  RegistrationDialog dlg(this, NULL);
  if (dlg.ShowModal() == wxID_OK) {
    RegistrationToList(true, new RegistrationInfo(dlg.GetInfo()), m_SelectedRegistration);
    if (m_SelectedRegistration != INT_MAX) {
      m_Registrations->SetItemState(m_SelectedRegistration, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
      m_Registrations->SetItemState(m_SelectedRegistration+1, 0, wxLIST_STATE_SELECTED);
    }
  }
}


void OptionsDialog::ChangeRegistration(wxCommandEvent &)
{
  RegistrationInfo * registration = (RegistrationInfo *)m_Registrations->GetItemData(m_SelectedRegistration);

  RegistrationDialog dlg(this, registration);
  if (dlg.ShowModal() == wxID_OK) {
    *registration = dlg.GetInfo();
    RegistrationToList(false, registration, m_SelectedRegistration);
  }
}


void OptionsDialog::RemoveRegistration(wxCommandEvent &)
{
  int position = m_SelectedRegistration;
  delete (RegistrationInfo *)m_Registrations->GetItemData(position);
  m_Registrations->DeleteItem(position);
  RenumberList(m_Registrations, position);
  m_ChangeRegistration->Disable();
  m_RemoveRegistration->Disable();
  m_MoveUpRegistration->Disable();
  m_MoveDownRegistration->Disable();
}


void OptionsDialog::MoveRegistration(int delta)
{
  int position = m_SelectedRegistration;

  RegistrationInfo * info = (RegistrationInfo *)m_Registrations->GetItemData(position);
  m_Registrations->DeleteItem(position);

  position += delta;

  RegistrationToList(true, info, position);
  RenumberList(m_Registrations, position-delta);

  m_Registrations->SetItemState(position, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}


void OptionsDialog::MoveUpRegistration(wxCommandEvent &)
{
  if (m_SelectedRegistration > 0) {
    MoveRegistration(-1);
    m_MoveUpRegistration->Enable(m_SelectedRegistration > 0);
  }
}


void OptionsDialog::MoveDownRegistration(wxCommandEvent &)
{
  int lastPosition = m_Registrations->GetItemCount()-1;
  if (m_SelectedRegistration < lastPosition) {
    MoveRegistration(1);
    m_MoveDownRegistration->Enable(m_SelectedRegistration < lastPosition);
  }
}


void OptionsDialog::SelectedRegistration(wxListEvent & listEvent)
{
  m_SelectedRegistration = listEvent.GetIndex();
  m_ChangeRegistration->Enable();
  m_RemoveRegistration->Enable();
  m_MoveUpRegistration->Enable(m_SelectedRegistration > 0);
  m_MoveDownRegistration->Enable(m_SelectedRegistration < m_Registrations->GetItemCount()-1);
}


void OptionsDialog::DeselectedRegistration(wxListEvent &)
{
  m_SelectedRegistration = INT_MAX;
  m_ChangeRegistration->Disable();
  m_RemoveRegistration->Disable();
  m_MoveUpRegistration->Disable();
  m_MoveDownRegistration->Disable();
}


void OptionsDialog::ActivateRegistration(wxListEvent & listEvent)
{
  ChangeRegistration(listEvent);
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


void OptionsDialog::RestoreDefaultRoutes(wxCommandEvent &)
{
  m_Routes->DeleteAllItems();

  for (PINDEX i = 0; i < PARRAYSIZE(DefaultRoutes); i++)
    AddRouteTableEntry(OpalManager::RouteEntry(DefaultRoutes[i]));
}


void OptionsDialog::AddRouteTableEntry(OpalManager::RouteEntry entry)
{
  PAssert(entry.IsValid(), PInvalidParameter);

  PString partyA = entry.GetPartyA();
  PINDEX colon = partyA.Find(':');

  PwxString source, device;
  if (colon == P_MAX_INDEX) {
    source = AllRouteSources;
    device = partyA;
  }
  else {
    source = partyA.Left(colon);
    if (source == ".*")
      source = AllRouteSources;
    device = partyA.Mid(colon+1);
    if (device == ".*")
      device = "";
  }

  int pos = m_Routes->InsertItem(INT_MAX, source);
  m_Routes->SetItem(pos, 1, device);
  m_Routes->SetItem(pos, 2, PwxString(entry.GetPartyB()));
  m_Routes->SetItem(pos, 3, PwxString(entry.GetDestination()));
}


#if PTRACING
////////////////////////////////////////
// Tracing fields

void OptionsDialog::BrowseTraceFile(wxCommandEvent & /*event*/)
{
  wxString newFile = wxFileSelector(wxT("Trace log file"),
                                    wxT(""),
                                    m_TraceFileName,
                                    wxT(".log"),
                                    wxT("Log Files (*.log)|*.log|Text Files (*.txt)|*.txt||"),
                                    wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
  if (!newFile.empty()) {
    m_TraceFileName = newFile;
    TransferDataToWindow();
  }
}
#endif



///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(PresenceDialog, wxDialog)
END_EVENT_TABLE()

PresenceDialog::PresenceDialog(MyManager * manager)
  : m_manager(*manager)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("PresenceDialog"));

  wxComboBox * states = FindWindowByNameAs<wxComboBox>(this, wxT("PresenceState"));
  states->SetValidator(wxGenericValidator(&m_status));

  wxChoice * addresses = FindWindowByNameAs<wxChoice>(this, wxT("PresenceAddress"));
  addresses->SetValidator(wxGenericValidator(&m_address));
  PStringList presences = manager->GetPresentities();
  if (presences.IsEmpty()) {
    addresses->Disable();
    states->Disable();
    FindWindow(wxID_OK)->Disable();
  }
  else {
    for (PStringList::iterator i = presences.begin(); i != presences.end(); ++i)
      addresses->AppendString(PwxString(*i));
    addresses->SetSelection(0);
  }
}


bool PresenceDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  PSafePtr<OpalPresentity> presentity = m_manager.GetPresentity(m_address.p_str());
  if (presentity == NULL) {
    wxMessageBox(wxT("Presence identity disappeared!"));
    return false;
  }

  OpalPresenceInfo::State state = OpalPresenceInfo::FromString(m_status);
  if (state != OpalPresenceInfo::InternalError)
    presentity->SetLocalPresence(state);
  else {
    presentity->SetLocalPresence(OpalPresenceInfo::Available, m_status.p_str());
    state = OpalPresenceInfo::Available;
  }

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(PresenceGroup);

  int presIndex = 0;
  for (;;) {
    wxString groupName;
    groupName.sprintf(wxT("%04u"), presIndex);
    if (!config->HasGroup(groupName))
      break;

    config->SetPath(groupName);
    PwxString aor;
    if (config->Read(PresenceAORKey, &aor) && aor == presentity->GetAOR()) {
      config->Write(LastPresenceStateKey, (int)state);
      config->Flush();
      break;
    }
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(AudioDevicesDialog, wxDialog)
END_EVENT_TABLE()

AudioDevicesDialog::AudioDevicesDialog(MyManager * manager, const OpalPCSSConnection & connection)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("AudioDevicesDialog"));

  wxComboBox * combo = FindWindowByNameAs<wxComboBox>(this, wxT("PlayDevice"));
  combo->SetValidator(wxGenericValidator(&m_playDevice));
  FillAudioDeviceComboBox(combo, PSoundChannel::Player);
  m_playDevice = AudioDeviceNameToScreen(connection.GetSoundChannelPlayDevice());

  combo = FindWindowByNameAs<wxComboBox>(this, wxT("RecordDevice"));
  combo->SetValidator(wxGenericValidator(&m_recordDevice));
  FillAudioDeviceComboBox(combo, PSoundChannel::Recorder);
  m_recordDevice = AudioDeviceNameToScreen(connection.GetSoundChannelRecordDevice());
}


PString AudioDevicesDialog::GetTransferAddress() const
{
  return "pc:" + AudioDeviceNameFromScreen(m_playDevice)
        + '\n' + AudioDeviceNameFromScreen(m_recordDevice);
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(VideoControlDialog, wxDialog)
END_EVENT_TABLE()

VideoControlDialog::VideoControlDialog(MyManager * manager, bool remote)
  : m_manager(*manager)
  , m_remote(remote)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("VideoControlDialog"));

  m_TargetBitRate = FindWindowByNameAs<wxSlider>(this, wxT("VideoBitRate"));
  m_FrameRate = FindWindowByNameAs<wxSlider>(this, wxT("FrameRate"));
  m_TSTO = FindWindowByNameAs<wxSlider>(this, wxT("TSTO"));

  OpalMediaStreamPtr stream = GetStream();
  if (stream != NULL) {
    OpalMediaFormat mediaFormat = stream->GetMediaFormat();
    m_TargetBitRate->SetMax(mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption())/1000);
    m_TargetBitRate->SetValue(mediaFormat.GetOptionInteger(OpalMediaFormat::TargetBitRateOption())/1000);
#if wxCHECK_VERSION(2,9,0)
    m_TargetBitRate->SetTickFreq(m_TargetBitRate->GetMax()/10);
#else
    m_TargetBitRate->SetTickFreq(m_TargetBitRate->GetMax()/10, 1);
#endif

    m_FrameRate->SetMax(30);
    m_FrameRate->SetValue(mediaFormat.GetClockRate()/mediaFormat.GetFrameTime());

    m_TSTO->SetValue(mediaFormat.GetOptionInteger(OpalVideoFormat::TemporalSpatialTradeOffOption()));
  }
}


bool VideoControlDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  OpalMediaStreamPtr stream = GetStream();
  if (stream != NULL) {
    if (m_remote) {
      stream->ExecuteCommand(OpalMediaFlowControl(m_TargetBitRate->GetValue()*1000));
      stream->ExecuteCommand(OpalTemporalSpatialTradeOff(m_FrameRate->GetValue()));
    }
    else {
      OpalMediaFormat mediaFormat = stream->GetMediaFormat();
      mediaFormat.SetOptionInteger(OpalVideoFormat::TargetBitRateOption(), m_TargetBitRate->GetValue()*1000);
      mediaFormat.SetOptionInteger(OpalMediaFormat::FrameTimeOption(), mediaFormat.GetClockRate()/m_FrameRate->GetValue());
      mediaFormat.SetOptionInteger(OpalVideoFormat::TemporalSpatialTradeOffOption(), m_TSTO->GetValue());
      stream->UpdateMediaFormat(mediaFormat);
    }
  }

  return true;
}


OpalMediaStreamPtr VideoControlDialog::GetStream() const
{
  PSafePtr<OpalConnection> connection = m_manager.GetConnection(false, PSafeReadOnly);
  return connection != NULL ? connection->GetMediaStream(OpalMediaType::Video(), m_remote) : NULL;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(StartVideoDialog, wxDialog)
END_EVENT_TABLE()

StartVideoDialog::StartVideoDialog(MyManager * manager, bool secondary)
  : m_preview(true)
  , m_contentRole(OpalVideoFormat::eNoRole)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("StartVideoDialog"));

  wxComboBox * combo = FindWindowByNameAs<wxComboBox>(this, wxT("VideoGrabDevice"));
  PStringArray devices = PVideoInputDevice::GetDriversDeviceNames("*");
  for (PINDEX i = 0; i < devices.GetSize(); i++)
    combo->Append(PwxString(devices[i]));
  combo->SetValidator(wxGenericValidator(&m_device));

  FindWindowByName(wxT("VideoPreview"))->SetValidator(wxGenericValidator(&m_preview));

  wxChoice * choice = FindWindowByNameAs<wxChoice>(this, wxT("VideoContentRole"));
  choice->SetValidator(wxGenericValidator(&m_contentRole));
  choice->Enable(secondary);
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(RegistrationDialog, wxDialog)
  EVT_CHOICE(wxXmlResource::GetXRCID(RegistrationTypeKey), RegistrationDialog::Changed)
  EVT_CHOICE(wxXmlResource::GetXRCID(RegistrarCompatibilityKey), RegistrationDialog::Changed)
  EVT_TEXT(wxXmlResource::GetXRCID(RegistrarUsernameKey), RegistrationDialog::Changed)
  EVT_TEXT(wxXmlResource::GetXRCID(RegistrarDomainKey), RegistrationDialog::Changed)
  EVT_TEXT(wxXmlResource::GetXRCID(RegistrarContactKey), RegistrationDialog::Changed)
  EVT_TEXT(wxXmlResource::GetXRCID(RegistrarAuthIDKey), RegistrationDialog::Changed)
  EVT_TEXT(wxXmlResource::GetXRCID(RegistrarPasswordKey), RegistrationDialog::Changed)
  EVT_TEXT(wxXmlResource::GetXRCID(RegistrarProxyKey), RegistrationDialog::Changed)
  EVT_TEXT(wxXmlResource::GetXRCID(RegistrarTimeToLiveKey), RegistrationDialog::Changed)
  EVT_BUTTON(wxXmlResource::GetXRCID(RegistrarUsedKey), RegistrationDialog::Changed)
END_EVENT_TABLE()

RegistrationDialog::RegistrationDialog(wxDialog * parent, const RegistrationInfo * info)
  : m_user(NULL)
{
  if (info != NULL)
    m_info = *info;

  wxXmlResource::Get()->LoadDialog(this, parent, wxT("RegistrationDialog"));

  m_ok = FindWindowByNameAs<wxButton>(this, wxT("wxID_OK"));
  m_ok->Disable();

  m_user = FindWindowByNameAs<wxTextCtrl>(this, RegistrarUsernameKey);
  m_user->SetValidator(wxGenericValidator(&m_info.m_User));
  m_domain = FindWindowByNameAs<wxTextCtrl>(this, RegistrarDomainKey);
  m_domain->SetValidator(wxGenericValidator(&m_info.m_Domain));

  wxChoice * choice = FindWindowByNameAs<wxChoice>(this, RegistrationTypeKey);
  for (PINDEX i = 0; i < RegistrationInfo::NumRegType; ++i)
    choice->Append(RegistrationInfoTable[i].m_name);
  choice->SetValidator(wxGenericValidator((int *)&m_info.m_Type));

  FindWindowByNameAs<wxCheckBox>(this, RegistrarUsedKey         )->SetValidator(wxGenericValidator(&m_info.m_Active));
  FindWindowByNameAs<wxTextCtrl>(this, RegistrarContactKey      )->SetValidator(wxGenericValidator(&m_info.m_Contact));
  FindWindowByNameAs<wxTextCtrl>(this, RegistrarAuthIDKey       )->SetValidator(wxGenericValidator(&m_info.m_AuthID));
  FindWindowByNameAs<wxTextCtrl>(this, RegistrarPasswordKey     )->SetValidator(wxGenericValidator(&m_info.m_Password));
  FindWindowByNameAs<wxTextCtrl>(this, RegistrarProxyKey        )->SetValidator(wxGenericValidator(&m_info.m_Proxy));
  FindWindowByNameAs<wxSpinCtrl>(this, RegistrarTimeToLiveKey   )->SetValidator(wxGenericValidator(&m_info.m_TimeToLive));
  FindWindowByNameAs<wxChoice  >(this, RegistrarCompatibilityKey)->SetValidator(wxGenericValidator((int *)&m_info.m_Compatibility));
}


void RegistrationDialog::Changed(wxCommandEvent & /*event*/)
{
  if (m_user == NULL)
    return;

  wxString user = m_user->GetValue();
  if (user.empty())
    m_ok->Disable();
  else {
    size_t at = user.find('@');
    if (at != wxString::npos && at > 0)
      m_ok->Enable(at < user.length()-1);
    else
      m_ok->Enable(!m_domain->GetValue().IsEmpty());
  }
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CallDialog, wxDialog)
  EVT_BUTTON(XRCID("wxID_OK"), CallDialog::OnOK)
  EVT_TEXT(XRCID("Address"), CallDialog::OnAddressChange)
END_EVENT_TABLE()

CallDialog::CallDialog(MyManager * manager, bool hideHandset, bool hideFax)
  : m_UseHandset(manager->HasHandset())
  , m_FaxMode(0)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("CallDialog"));

  m_ok = FindWindowByNameAs<wxButton>(this, wxT("wxID_OK"));
  m_ok->Disable();

  wxCheckBox * useHandset = FindWindowByNameAs<wxCheckBox>(this, wxT("UseHandset"));
  useHandset->SetValidator(wxGenericValidator(&m_UseHandset));
  if (hideHandset)
    useHandset->Hide();
  else
    useHandset->Enable(m_UseHandset);

  wxRadioBox * faxMode = FindWindowByNameAs<wxRadioBox>(this, wxT("FaxMode"));
  faxMode->SetValidator(wxGenericValidator(&m_FaxMode));
  if (hideFax)
    faxMode->Hide();

  m_AddressCtrl = FindWindowByNameAs<wxComboBox>(this, wxT("Address"));
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


void CallDialog::OnOK(wxCommandEvent &)
{
  TransferDataFromWindow();

  wxConfigBase * config = wxConfig::Get();
  config->DeleteGroup(RecentCallsGroup);
  config->SetPath(RecentCallsGroup);

  config->Write(wxT("1"), m_Address);

  unsigned keyNumber = 1;
  for (size_t i = 0; i < m_AddressCtrl->GetCount() && keyNumber < MaxSavedRecentCalls; ++i) {
    wxString entry = m_AddressCtrl->GetString(i);

    if (m_Address != entry) {
      wxString key;
      key.sprintf(wxT("%u"), ++keyNumber);
      config->Write(key, entry);
    }
  }

  config->Flush();

  EndModal(wxID_OK);
}


void CallDialog::OnAddressChange(wxCommandEvent & WXUNUSED(event))
{
  TransferDataFromWindow();
  m_ok->Enable(!m_Address.IsEmpty());
}

///////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_IM

void MyManager::OnConversation(OpalIMContext &, OpalIMContext::ConversationInfo info)
{
  PwxString id = info.m_context->GetID();

  if (info.m_opening)
    m_imDialogMap[id] = new IMDialog(this, *info.m_context);
  else {
    m_imDialogMap[id]->Destroy();
    m_imDialogMap.erase(id);
  }
}


BEGIN_EVENT_TABLE(IMDialog, wxDialog)
  EVT_BUTTON(XRCID("Send"), IMDialog::OnSend)
  EVT_TEXT_ENTER(XRCID("EnteredText"), IMDialog::OnTextEnter)
  EVT_TEXT(XRCID("EnteredText"), IMDialog::OnText)
  EVT_CLOSE(IMDialog::OnCloseWindow)
  EVT_COMMAND(ID_ASYNC_NOTIFICATION,  wxEvtAsyncNotification,  IMDialog::OnEvtAsyncNotification)
END_EVENT_TABLE()

IMDialog::IMDialog(MyManager * manager, OpalIMContext & context)
  : m_manager(*manager)
  , m_context(context) 
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("IMDialog"));

  PStringStream str;
  str << "Conversation with ";
  str << context.GetRemoteURL();
  SetTitle(PwxString(str));

  m_textArea    = FindWindowByNameAs<wxTextCtrl>(this, wxT("TextArea"));
  m_enteredText = FindWindowByNameAs<wxTextCtrl>(this, wxT("EnteredText"));
  m_enteredText->SetFocus();

  m_compositionIndication = FindWindowByNameAs<wxStaticText>(this, wxT("CompositionIndication"));

  m_defaultStyle = m_textArea->GetDefaultStyle();
  m_ourStyle = m_defaultStyle;
  m_theirStyle = m_defaultStyle;
  m_ourStyle.SetTextColour(*wxRED);
  m_theirStyle.SetTextColour(wxColour(0, 0xc0, 0));

  context.SetMessageDispositionNotifier(PCREATE_MessageDispositionNotifier(OnMessageDisposition));
  context.SetMessageReceivedNotifier(PCREATE_MessageReceivedNotifier(OnMessageReceived));
  context.SetCompositionIndicationNotifier(PCREATE_CompositionIndicationNotifier(OnCompositionIndication));

  Show();
}


IMDialog::~IMDialog()
{
}


void IMDialog::OnCloseWindow(wxCloseEvent & WXUNUSED(event))
{
  m_context.Close();
  Hide();
}


void IMDialog::AsyncNotifierSignal()
{
  wxCommandEvent theEvent(wxEvtAsyncNotification, ID_ASYNC_NOTIFICATION);
  theEvent.SetEventObject(this);
  GetEventHandler()->AddPendingEvent(theEvent);
}


void IMDialog::OnEvtAsyncNotification(wxCommandEvent &)
{
  while (AsyncNotifierExecute())
    ;
}


void IMDialog::OnSend(wxCommandEvent &)
{
  SendCurrentText();
}


void IMDialog::OnTextEnter(wxCommandEvent & WXUNUSED(event))
{
  SendCurrentText();
}


void IMDialog::OnText(wxCommandEvent & WXUNUSED(event))
{
  SendCompositionIndication();
}


void IMDialog::SendCurrentText()
{
  PwxString text = m_enteredText->GetValue();
  m_enteredText->SetValue(wxT(""));
  AddTextToScreen(text, true);

  OpalIM * im = new OpalIM;
  im->m_bodies.SetAt(PMIMEInfo::TextPlain(), text.p_str());
  ShowDisposition(m_context.Send(im));
}


void IMDialog::SendCompositionIndication()
{
  OpalIMContext::CompositionInfo info(OpalIMContext::CompositionIndicationActive(), PMIMEInfo::TextPlain());
  m_context.SendCompositionIndication(info);
}


void IMDialog::OnMessageDisposition(OpalIMContext &, OpalIMContext::DispositionInfo info)
{
  ShowDisposition(info.m_disposition);
}


void IMDialog::OnMessageReceived(OpalIMContext &, OpalIM im)
{
  AddTextToScreen(im.m_bodies(PMIMEInfo::TextPlain()), false);
}


void IMDialog::OnCompositionIndication(OpalIMContext &, OpalIMContext::CompositionInfo info)
{
  if (info.m_state == OpalIMContext::CompositionIndicationActive())
    m_compositionIndication->SetLabel(PwxString(m_context.GetRemoteName()) + wxT(" is typing"));
  else
    m_compositionIndication->SetLabel(wxT(""));
}


void IMDialog::ShowDisposition(OpalIMContext::MessageDisposition status)
{
  if (status < OpalIMContext::DispositionErrors)
    return;

  switch (status) {
    case OpalIMContext::ConversationClosed :
      AddTextToScreen(wxT("Closed"), false);
      break;

    case OpalIMContext::TransportFailure :
      AddTextToScreen(wxT("Network error"), false);
      break;

    case OpalIMContext::DestinationUnknown :
      AddTextToScreen(wxT("User unkown"), false);
      break;

    case OpalIMContext::DestinationUnavailable :
      AddTextToScreen(wxT("Unavailable"), false);
      break;

    default :
      AddTextToScreen(wxT("Sent message failed"), false);
  }
}


void IMDialog::AddTextToScreen(const PwxString & text, bool fromUs)
{
  m_textArea->SetDefaultStyle(fromUs ? m_ourStyle : m_theirStyle);
  m_textArea->AppendText(PwxString(PTime().AsString("[hh:mm] ")));
  m_textArea->AppendText(PwxString(fromUs ? m_context.GetLocalURL().AsString() : m_context.GetRemoteName()));
  m_textArea->AppendText(wxT(": "));
  m_textArea->SetDefaultStyle(m_defaultStyle);
  m_textArea->AppendText(text);
  m_textArea->AppendText(wxT("\n"));
  Show();
}

///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CallIMDialog, wxDialog)
  EVT_BUTTON(XRCID("wxID_OK"), CallIMDialog::OnOK)
  EVT_CHOICE(XRCID("Presentity"), CallIMDialog::OnChanged)
  EVT_TEXT(XRCID("Address"), CallIMDialog::OnChanged)
END_EVENT_TABLE()

CallIMDialog::CallIMDialog(MyManager * manager)
  : m_manager(*manager)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("CallIMDialog"));

  m_ok = FindWindowByNameAs<wxButton>(this, wxT("wxID_OK"));
  m_ok->Disable();

  m_AddressCtrl = FindWindowByNameAs<wxComboBox>(this, wxT("Address"));
  m_AddressCtrl->SetValidator(wxGenericValidator(&m_Address));

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(RecentIMCallsGroup);
  wxString entryName;
  long entryIndex;
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      wxString address;
      if (config->Read(entryName, &address))
        m_AddressCtrl->AppendString(address);
    } while (config->GetNextEntry(entryName, entryIndex));
  }

  PStringList presentities = manager->GetPresentities();
  wxChoice * choice = FindWindowByNameAs<wxChoice>(this, wxT("Presentity"));
  for (PStringList::iterator it = presentities.begin(); it != presentities.end(); ++it)
    choice->Append(PwxString(*it));
  choice->SetValidator(wxGenericValidator(&m_Presentity));
}


void CallIMDialog::OnOK(wxCommandEvent &)
{
  wxConfigBase * config = wxConfig::Get();
  config->DeleteGroup(RecentIMCallsGroup);
  config->SetPath(RecentIMCallsGroup);

  config->Write(wxT("1"), m_Address);

  unsigned keyNumber = 1;
  for (size_t i = 0; i < m_AddressCtrl->GetCount() && keyNumber < MaxSavedRecentIMCalls; ++i) {
    wxString entry = m_AddressCtrl->GetString(i);

    if (m_Address != entry) {
      wxString key;
      key.sprintf(wxT("%u"), ++keyNumber);
      config->Write(key, entry);
    }
  }

  config->Flush();

  EndModal(wxID_OK);
}


void CallIMDialog::OnChanged(wxCommandEvent & WXUNUSED(event))
{
  TransferDataFromWindow();
  m_ok->Enable(!m_Presentity.IsEmpty() && !m_Address.IsEmpty());
}

#endif // OPAL_HAS_IM


///////////////////////////////////////////////////////////////////////////////

CallPanelBase::CallPanelBase(MyManager & manager,
                             const PwxString & token,
                             wxWindow * parent,
                             const wxChar * resource)
  : m_manager(manager)
  , m_token(token)
{
  wxXmlResource::Get()->LoadPanel(this, parent, resource);
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(AnswerPanel, CallPanelBase)
  EVT_BUTTON(XRCID("AnswerCall"), AnswerPanel::OnAnswer)
  EVT_BUTTON(XRCID("RejectCall"), AnswerPanel::OnReject)
#if OPAL_FAX
  EVT_RADIOBOX(XRCID("AnswerAs"), AnswerPanel::OnChangeAnswerMode)
#endif
END_EVENT_TABLE()

AnswerPanel::AnswerPanel(MyManager & manager, const PwxString & token, wxWindow * parent)
  : CallPanelBase(manager, token, parent, wxT("AnswerPanel"))
{
}


void AnswerPanel::SetPartyNames(const PwxString & calling, const PwxString & called)
{
  FindWindowByNameAs<wxStaticText>(this, wxT("CallingParty"))->SetLabel(calling);
  FindWindowByNameAs<wxStaticText>(this, wxT("CalledParty"))->SetLabel(called);
  FindWindowByNameAs<wxStaticText>(this, wxT("CalledParty"))->SetLabel(called);
}


void AnswerPanel::OnAnswer(wxCommandEvent & /*event*/)
{
  m_manager.AnswerCall();
}


void AnswerPanel::OnReject(wxCommandEvent & /*event*/)
{
  m_manager.RejectCall();
}


#if OPAL_FAX
void AnswerPanel::OnChangeAnswerMode(wxCommandEvent & theEvent)
{
  m_manager.m_currentAnswerMode = (MyManager::FaxAnswerModes)theEvent.GetSelection();
}
#endif // OPAL_FAX


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CallingPanel, CallPanelBase)
  EVT_BUTTON(XRCID("HangUpCall"), CallingPanel::OnHangUp)
END_EVENT_TABLE()

CallingPanel::CallingPanel(MyManager & manager, const PwxString & token, wxWindow * parent)
  : CallPanelBase(manager, token, parent, wxT("CallingPanel"))
{
}


void CallingPanel::OnHangUp(wxCommandEvent & /*event*/)
{
  m_manager.HangUpCall();
}


///////////////////////////////////////////////////////////////////////////////

const int VU_UPDATE_TIMER_ID = 1000;

BEGIN_EVENT_TABLE(InCallPanel, CallPanelBase)
  EVT_BUTTON(XRCID("HangUp"), InCallPanel::OnHangUp)
  EVT_BUTTON(XRCID("Hold"), InCallPanel::OnHoldRetrieve)
  EVT_BUTTON(XRCID("Conference"), InCallPanel::OnConference)
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


InCallPanel::InCallPanel(MyManager & manager, const PwxString & token, wxWindow * parent)
  : CallPanelBase(manager, token, parent, wxT("InCallPanel"))
  , m_vuTimer(this, VU_UPDATE_TIMER_ID)
  , m_updateStatistics(0)
{
  m_Hold = FindWindowByNameAs<wxButton>(this, wxT("Hold"));
  m_Conference = FindWindowByNameAs<wxButton>(this, wxT("Conference"));
  m_SpeakerHandset = FindWindowByNameAs<wxButton>(this, wxT("SpeakerHandset"));
  m_SpeakerMute = FindWindowByNameAs<wxCheckBox>(this, wxT("SpeakerMute"));
  m_MicrophoneMute = FindWindowByNameAs<wxCheckBox>(this, wxT("MicrophoneMute"));
  m_SpeakerVolume = FindWindowByNameAs<wxSlider>(this, wxT("SpeakerVolume"));
  m_MicrophoneVolume = FindWindowByNameAs<wxSlider>(this, wxT("MicrophoneVolume"));
  m_vuSpeaker = FindWindowByNameAs<wxGauge>(this, wxT("SpeakerGauge"));
  m_vuMicrophone = FindWindowByNameAs<wxGauge>(this, wxT("MicrophoneGauge"));
  m_callTime = FindWindowByNameAs<wxStaticText>(this, wxT("CallTime"));

  OnHoldChanged(false);

  m_vuTimer.Start(250);

  m_pages[RxAudio].Init(this, RxAudio, OpalMediaType::Audio(), true );
  m_pages[TxAudio].Init(this, TxAudio, OpalMediaType::Audio(), false);
  m_pages[RxVideo].Init(this, RxVideo, OpalMediaType::Video(), true );
  m_pages[TxVideo].Init(this, TxVideo, OpalMediaType::Video(), false);
  m_pages[RxFax  ].Init(this, RxFax  , OpalMediaType::Fax(),   false);
  m_pages[TxFax  ].Init(this, TxFax  , OpalMediaType::Fax(),   true);

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
    bool mute = false;
    config->Read(SpeakerMuteKey, &mute);
    m_SpeakerMute->SetValue(!mute);
    SetVolume(false, value, mute);

    value = 50;
    config->Read(MicrophoneVolumeKey, &value);
    m_MicrophoneVolume->SetValue(value);
    mute = false;
    config->Read(MicrophoneMuteKey, &mute);
    m_MicrophoneMute->SetValue(!mute);
    SetVolume(true, value, mute);
  }

  return wxPanel::Show(show);
}


void InCallPanel::OnStreamsChanged()
{
  // Must do this before getting lock on OpalCall to avoid deadlock
  m_SpeakerHandset->Enable(m_manager.HasHandset());

  UpdateStatistics();

  PSafePtr<OpalConnection> connection = m_manager.GetConnection(true, PSafeReadOnly);
  if (connection != NULL && connection->GetMediaStream(OpalMediaType::Fax(), true) != NULL)
    FindWindowByNameAs<wxNotebook>(this, wxT("Statistics"))->SetSelection(RxFax);
}


void InCallPanel::OnHangUp(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalCall> activeCall = m_manager.FindCallWithLock(m_token, PSafeReadOnly);
  if (PAssertNULL(activeCall) != NULL) {
    LogWindow << "Hanging up \"" << *activeCall << '"' << endl;
    activeCall->Clear();
  }
}


void InCallPanel::OnHoldChanged(bool onHold)
{
  PSafePtr<OpalCall> call = m_manager.FindCallWithLock(m_token, PSafeReadOnly);
  if (PAssertNULL(call) == NULL)
    return;

  bool notConferenced = call->GetConnectionAs<OpalMixerConnection>() == NULL;

  m_Hold->SetLabel(onHold ? wxT("Retrieve") : wxT("Hold"));
  m_Hold->Enable(notConferenced);
  m_Conference->Enable(!onHold && !m_manager.m_callsOnHold.empty() && notConferenced);
}


void InCallPanel::OnHoldRetrieve(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalCall> call = m_manager.FindCallWithLock(m_token, PSafeReadWrite);
  if (PAssertNULL(call) == NULL)
    return;

  if (call->IsOnHold()) {
    PSafePtr<OpalCall> activeCall = m_manager.GetCall(PSafeReadWrite);
    if (activeCall == NULL) {
      if (!call->Retrieve())
        return;
    }
    else {
      m_manager.m_switchHoldToken = m_token;
      if (!activeCall->Hold())
        return;
    }
  }
  else {
    if (!call->Hold())
      return;
  }

  m_Hold->SetLabel(wxT("In Progress"));
  m_Hold->Enable(false);
}


void InCallPanel::OnConference(wxCommandEvent & /*event*/)
{
  if (m_manager.m_activeCall != NULL && m_token == m_manager.m_activeCall->GetToken()) {
    m_manager.AddToConference(*m_manager.m_callsOnHold.front().m_call);
    m_Conference->Enable(false);
    m_Hold->Enable(false);
  }
}


void InCallPanel::OnSpeakerMute(wxCommandEvent & cmdEvent)
{
  bool muted = !cmdEvent.IsChecked();
  SetVolume(false, m_SpeakerVolume->GetValue(), muted);
  wxConfigBase * config = wxConfig::Get();
  config->Write(SpeakerMuteKey, muted);
  config->Flush();
}


void InCallPanel::OnMicrophoneMute(wxCommandEvent & cmdEvent)
{
  bool muted = !cmdEvent.IsChecked();
  SetVolume(true, m_MicrophoneVolume->GetValue(), muted);
  wxConfigBase * config = wxConfig::Get();
  config->Write(MicrophoneMuteKey, muted);
  config->Flush();
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


void InCallPanel::SpeakerVolume(wxScrollEvent & scrollEvent)
{
  int newValue = scrollEvent.GetPosition();
  SetVolume(false, newValue, !m_SpeakerMute->GetValue());
  wxConfigBase * config = wxConfig::Get();
  config->Write(SpeakerVolumeKey, newValue);
  config->Flush();
}


void InCallPanel::MicrophoneVolume(wxScrollEvent & scrollEvent)
{
  int newValue = scrollEvent.GetPosition();
  SetVolume(true, newValue, !m_MicrophoneMute->GetValue());
  wxConfigBase * config = wxConfig::Get();
  config->Write(MicrophoneVolumeKey, newValue);
  config->Flush();
}


void InCallPanel::SetVolume(bool isMicrophone, int value, bool muted)
{
  PSafePtr<OpalConnection> connection = m_manager.GetConnection(true, PSafeReadOnly);
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
    if (++m_updateStatistics % 8 == 0)
      UpdateStatistics();

    int micLevel = -1;
    int spkLevel = -1;
    PSafePtr<OpalConnection> connection = m_manager.GetConnection(true, PSafeReadOnly);
    if (connection != NULL) {
      spkLevel = connection->GetAudioSignalLevel(false);
      micLevel = connection->GetAudioSignalLevel(true);

      if (m_updateStatistics % 3 == 0) {
        PTime established = connection->GetPhaseTime(OpalConnection::EstablishedPhase);
        if (established.IsValid())
          m_callTime->SetLabel(PwxString((PTime() - established).AsString(0, PTimeInterval::NormalFormat, 5)));
      }
    }

    SetGauge(m_vuSpeaker, spkLevel);
    SetGauge(m_vuMicrophone, micLevel);
  }
}


void InCallPanel::UpdateStatistics()
{
  PSafePtr<OpalConnection> connection = m_manager.GetConnection(false, PSafeReadOnly);
  if (connection == NULL)
    return;

  PINDEX i;
  for (i = 0; i < RxFax; i++)
    m_pages[i].UpdateSession(connection);

  connection = m_manager.GetConnection(true, PSafeReadOnly);
  if (connection == NULL)
    return;

  for (; i < NumPages; i++)
    m_pages[i].UpdateSession(connection);
}


static vector<StatisticsField *> StatisticsFieldTemplates;

StatisticsField::StatisticsField(const wxChar * name, StatisticsPages page)
  : m_name(name)
  , m_page(page)
  , m_staticText(NULL)
  , m_lastBytes(0)
  , m_lastFrames(0)
{
  StatisticsFieldTemplates.push_back(this);
}


void StatisticsField::Init(wxWindow * panel)
{
  m_staticText = FindWindowByNameAs<wxStaticText>(panel, m_name);
  m_printFormat = m_staticText->GetLabel();
  Clear();
}


void StatisticsField::Clear()
{
  m_staticText->SetLabel(NotAvailableString);
  m_lastBandwidthTick = 0;
}


double StatisticsField::CalculateBandwidth(PUInt64 bytes)
{
  PTimeInterval tick = PTimer::Tick();

  double value;
  if (m_lastBandwidthTick != 0)
    value = 8.0 * (bytes - m_lastBytes) / (tick - m_lastBandwidthTick).GetMilliSeconds(); // Ends up as kilobits/second
  else
    value = 0;

  m_lastBandwidthTick = tick;
  m_lastBytes = bytes;

  return value;
}


double StatisticsField::CalculateFrameRate(DWORD frames)
{
  PTimeInterval tick = PTimer::Tick();

  double value;
  if (m_lastFrameTick != 0)
    value = (frames - m_lastFrames) / (double)(tick - m_lastFrameTick).GetSeconds(); // Ends up as frames/second
  else
    value = 0;

  m_lastFrameTick = tick;
  m_lastFrames = frames;

  return value;
}


void StatisticsField::Update(const OpalConnection & connection, const OpalMediaStream & stream, const OpalMediaStatistics & statistics)
{
  wxString value;
  GetValue(connection, stream, statistics, value);
  m_staticText->SetLabel(value);
}


#define STATISTICS_FIELD_BEG(type, name) \
  class type##name##StatisticsField : public StatisticsField { \
  public: type##name##StatisticsField() : StatisticsField(wxT(#type) wxT(#name), type) { } \
    virtual ~type##name##StatisticsField() { } \
    virtual StatisticsField * Clone() const { return new type##name##StatisticsField(*this); } \
    virtual void GetValue(const OpalConnection & connection, const OpalMediaStream & stream, const OpalMediaStatistics & statistics, wxString & value) {

#define STATISTICS_FIELD_END(type, name) \
    } } Static##type##name##StatisticsField;

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

STATISTICS_FIELD_BEG(RxAudio, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(RxAudio, Bandwidth)

STATISTICS_FIELD_BEG(RxAudio, MinTime)
  value.sprintf(m_printFormat, statistics.m_minimumPacketTime);
STATISTICS_FIELD_END(RxAudio, MinTime)

STATISTICS_FIELD_BEG(RxAudio, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(RxAudio, Bytes)

STATISTICS_FIELD_BEG(RxAudio, AvgTime)
  value.sprintf(m_printFormat, statistics.m_averagePacketTime);
STATISTICS_FIELD_END(RxAudio, AvgTime)

STATISTICS_FIELD_BEG(RxAudio, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(RxAudio, Packets)

STATISTICS_FIELD_BEG(RxAudio, MaxTime)
  value.sprintf(m_printFormat, statistics.m_maximumPacketTime);
STATISTICS_FIELD_END(RxAudio, MaxTime)

STATISTICS_FIELD_BEG(RxAudio, Lost)
  value.sprintf(m_printFormat, statistics.m_packetsLost);
STATISTICS_FIELD_END(RxAudio, Lost)

STATISTICS_FIELD_BEG(RxAudio, AvgJitter)
  value.sprintf(m_printFormat, statistics.m_averageJitter);
STATISTICS_FIELD_END(RxAudio, AvgJitter)

STATISTICS_FIELD_BEG(RxAudio, OutOfOrder)
  value.sprintf(m_printFormat, statistics.m_packetsOutOfOrder);
STATISTICS_FIELD_END(RxAudio, OutOfOrder)

STATISTICS_FIELD_BEG(RxAudio, MaxJitter)
  value.sprintf(m_printFormat, statistics.m_maximumJitter);
STATISTICS_FIELD_END(RxAudio, MaxJitter)

STATISTICS_FIELD_BEG(RxAudio, TooLate)
  value.sprintf(m_printFormat, statistics.m_packetsTooLate);
STATISTICS_FIELD_END(RxAudio, TooLate)

STATISTICS_FIELD_BEG(RxAudio, Overruns)
  value.sprintf(m_printFormat, statistics.m_packetOverruns);
STATISTICS_FIELD_END(RxAudio, Overruns)

STATISTICS_FIELD_BEG(RxAudio, JitterDelay)
  value.sprintf(m_printFormat, statistics.m_jitterBufferDelay);
STATISTICS_FIELD_END(RxAudio, JitterDelay)

STATISTICS_FIELD_BEG(TxAudio, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(TxAudio, Bandwidth)

STATISTICS_FIELD_BEG(TxAudio, MinTime)
  value.sprintf(m_printFormat, statistics.m_minimumPacketTime);
STATISTICS_FIELD_END(TxAudio, MinTime)

STATISTICS_FIELD_BEG(TxAudio, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(TxAudio, Bytes)

STATISTICS_FIELD_BEG(TxAudio, AvgTime)
  value.sprintf(m_printFormat, statistics.m_averagePacketTime);
STATISTICS_FIELD_END(TxAudio, AvgTime)

STATISTICS_FIELD_BEG(TxAudio, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(TxAudio, Packets)

STATISTICS_FIELD_BEG(TxAudio, MaxTime)
  value.sprintf(m_printFormat, statistics.m_maximumPacketTime);
STATISTICS_FIELD_END(TxAudio, MaxTime)

STATISTICS_FIELD_BEG(TxAudio, Lost)
  value.sprintf(m_printFormat, statistics.m_packetsLost);
STATISTICS_FIELD_END(TxAudio, Lost)

STATISTICS_FIELD_BEG(TxAudio, AvgJitter)
  value.sprintf(m_printFormat, statistics.m_averageJitter);
STATISTICS_FIELD_END(TxAudio, AvgJitter)

STATISTICS_FIELD_BEG(RxVideo, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(RxVideo, Bandwidth)

STATISTICS_FIELD_BEG(RxVideo, MinTime)
  value.sprintf(m_printFormat, statistics.m_minimumPacketTime);
STATISTICS_FIELD_END(RxVideo, MinTime)

STATISTICS_FIELD_BEG(RxVideo, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(RxVideo, Bytes)

STATISTICS_FIELD_BEG(RxVideo, AvgTime)
  value.sprintf(m_printFormat, statistics.m_averagePacketTime);
STATISTICS_FIELD_END(RxVideo, AvgTime)

STATISTICS_FIELD_BEG(RxVideo, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(RxVideo, Packets)

STATISTICS_FIELD_BEG(RxVideo, MaxTime)
  value.sprintf(m_printFormat, statistics.m_maximumPacketTime);
STATISTICS_FIELD_END(RxVideo, MaxTime)

STATISTICS_FIELD_BEG(RxVideo, Lost)
  value.sprintf(m_printFormat, statistics.m_packetsLost);
STATISTICS_FIELD_END(RxVideo, Lost)

STATISTICS_FIELD_BEG(RxVideo, AvgJitter)
  value.sprintf(m_printFormat, statistics.m_averageJitter);
STATISTICS_FIELD_END(RxVideo, AvgJitter)

STATISTICS_FIELD_BEG(RxVideo, OutOfOrder)
  value.sprintf(m_printFormat, statistics.m_packetsOutOfOrder);
STATISTICS_FIELD_END(RxVideo, OutOfOrder)

STATISTICS_FIELD_BEG(RxVideo, MaxJitter)
  value.sprintf(m_printFormat, statistics.m_maximumJitter);
STATISTICS_FIELD_END(RxVideo, MaxJitter)

STATISTICS_FIELD_BEG(RxVideo, Frames)
  value.sprintf(m_printFormat, statistics.m_totalFrames);
STATISTICS_FIELD_END(RxVideo, Frames)

STATISTICS_FIELD_BEG(RxVideo, KeyFrames)
  value.sprintf(m_printFormat, statistics.m_keyFrames);
STATISTICS_FIELD_END(RxVideo, KeyFrames)

STATISTICS_FIELD_BEG(RxVideo, FrameRate)
  value.sprintf(m_printFormat, CalculateFrameRate(statistics.m_totalFrames));
STATISTICS_FIELD_END(RxVideo, FrameRate)

STATISTICS_FIELD_BEG(RxVideo, VFU)
  value.sprintf(m_printFormat, connection.GetVideoUpdateRequestsSent());
STATISTICS_FIELD_END(RxVideo, VFU)

STATISTICS_FIELD_BEG(TxVideo, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(TxVideo, Bandwidth)

STATISTICS_FIELD_BEG(TxVideo, MinTime)
  value.sprintf(m_printFormat, statistics.m_minimumPacketTime);
STATISTICS_FIELD_END(TxVideo, MinTime)

STATISTICS_FIELD_BEG(TxVideo, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(TxVideo, Bytes)

STATISTICS_FIELD_BEG(TxVideo, AvgTime)
  value.sprintf(m_printFormat, statistics.m_averagePacketTime);
STATISTICS_FIELD_END(TxVideo, AvgTime)

STATISTICS_FIELD_BEG(TxVideo, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(TxVideo, Packets)

STATISTICS_FIELD_BEG(TxVideo, Lost)
  value.sprintf(m_printFormat, statistics.m_packetsLost);
STATISTICS_FIELD_END(TxVideo, Lost)

STATISTICS_FIELD_BEG(TxVideo, MaxTime)
  value.sprintf(m_printFormat, statistics.m_maximumPacketTime);
STATISTICS_FIELD_END(TxVideo, MaxTime)

STATISTICS_FIELD_BEG(TxVideo, Frames)
  value.sprintf(m_printFormat, statistics.m_totalFrames);
STATISTICS_FIELD_END(TxVideo, Frames)

STATISTICS_FIELD_BEG(TxVideo, KeyFrames)
  value.sprintf(m_printFormat, statistics.m_keyFrames);
STATISTICS_FIELD_END(TxVideo, KeyFrames)

STATISTICS_FIELD_BEG(TxVideo, FrameRate)
  value.sprintf(m_printFormat, CalculateFrameRate(statistics.m_totalFrames));
STATISTICS_FIELD_END(TxVideo, FrameRate)

STATISTICS_FIELD_BEG(RxFax, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(RxFax, Bandwidth)

STATISTICS_FIELD_BEG(RxFax, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(RxFax, Bytes)

STATISTICS_FIELD_BEG(RxFax, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(RxFax, Packets)

#if OPAL_FAX
STATISTICS_FIELD_BEG(RxFax, Pages)
  if (statistics.m_fax.m_rxPages < 0)
    value.Empty();
  else {
    switch (statistics.m_fax.m_result) {
      case -2 :
        value = NotAvailableString;
        break;
      case -1 :
        value.sprintf(m_printFormat, statistics.m_fax.m_rxPages+1);
        break;
      default :
        value.sprintf(m_printFormat, statistics.m_fax.m_rxPages);
    }
  }
STATISTICS_FIELD_END(RxFax, Pages)

STATISTICS_FIELD_BEG(TxFax, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(TxFax, Bandwidth)

STATISTICS_FIELD_BEG(TxFax, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(TxFax, Bytes)

STATISTICS_FIELD_BEG(TxFax, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(TxFax, Packets)

STATISTICS_FIELD_BEG(TxFax, Pages)
  if (statistics.m_fax.m_txPages < 0)
    value.Empty();
  else {
    switch (statistics.m_fax.m_result) {
      case -2 :
        value = NotAvailableString;
        break;
      case -1 :
        if (statistics.m_fax.m_totalPages == 0)
          value = NotAvailableString;
        else if (statistics.m_fax.m_txPages >= statistics.m_fax.m_totalPages)
          value.sprintf(m_printFormat, statistics.m_fax.m_totalPages, statistics.m_fax.m_totalPages);
        else
          value.sprintf(m_printFormat, statistics.m_fax.m_txPages+1, statistics.m_fax.m_totalPages);
        break;
      default :
        value.sprintf(m_printFormat, statistics.m_fax.m_txPages, statistics.m_fax.m_totalPages);
    }
  }
STATISTICS_FIELD_END(TxFax, Pages)
#endif // OPAL_FAX


#ifdef _MSC_VER
#pragma warning(default:4100)
#endif


StatisticsPage::StatisticsPage()
  : m_page(NumPages)
  , m_receiver(false)
  , m_isActive(false)
  , m_window(NULL)
{
}


StatisticsPage::~StatisticsPage()
{
  for (size_t i = 0; i < m_fields.size(); i++)
    delete m_fields[i];
}


void StatisticsPage::Init(InCallPanel * panel,
                          StatisticsPages page,
                          const OpalMediaType & mediaType,
                          bool receiver)
{
  m_page = page;
  m_mediaType = mediaType;
  m_receiver = receiver;
  m_isActive = false;

  wxNotebook * book = FindWindowByNameAs<wxNotebook>(panel, wxT("Statistics"));
  m_window = book->GetPage(page > RxTxFax ? RxTxFax : page);

  for (size_t i = 0; i < StatisticsFieldTemplates.size(); i++) {
    if (StatisticsFieldTemplates[i]->m_page == page) {
      StatisticsField * field = StatisticsFieldTemplates[i]->Clone();
      field->Init(panel);
      m_fields.push_back(field);
    }
  }
}


void StatisticsPage::UpdateSession(const OpalConnection * connection)
{
  if (connection == NULL)
    m_isActive = false;
  else {
    OpalMediaStreamPtr stream = connection->GetMediaStream(m_mediaType, m_receiver);
    m_isActive = stream != NULL && stream->Open();
    if (m_isActive) {
      OpalMediaStatistics statistics;
      stream->GetStatistics(statistics);
      for (size_t i = 0; i < m_fields.size(); i++)
        m_fields[i]->Update(*connection, *stream, statistics);
    }
  }

  m_window->Enable(m_isActive);

  if (!m_isActive) {
    for (size_t i = 0; i < m_fields.size(); i++)
      m_fields[i]->Clear();
  }
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(SpeedDialDialog, wxDialog)
  EVT_TEXT(XRCID("SpeedDialName"), SpeedDialDialog::OnChange)
  EVT_TEXT(XRCID("SpeedDialNumber"), SpeedDialDialog::OnChange)
  EVT_TEXT(XRCID("SpeedDialAddress"), SpeedDialDialog::OnChange)
END_EVENT_TABLE()

SpeedDialDialog::SpeedDialDialog(MyManager * manager, const SpeedDialInfo & info)
  : SpeedDialInfo(info)
  , m_manager(*manager)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("SpeedDialDialog"));

  m_ok = FindWindowByNameAs<wxButton>(this, wxT("wxID_OK"));

  m_nameCtrl = FindWindowByNameAs<wxTextCtrl>(this, wxT("SpeedDialName"));
  m_nameCtrl->SetValidator(wxGenericValidator(&m_Name));

  m_numberCtrl = FindWindowByNameAs<wxTextCtrl>(this, wxT("SpeedDialNumber"));
  m_numberCtrl->SetValidator(wxGenericValidator(&m_Number));

  m_addressCtrl = FindWindowByNameAs<wxTextCtrl>(this, wxT("SpeedDialAddress"));
  m_addressCtrl->SetValidator(wxGenericValidator(&m_Address));

  wxChoice * choice = FindWindowByNameAs<wxChoice>(this, wxT("SpeedDialStateURL"));
  choice->SetValidator(wxGenericValidator(&m_Presentity));
  choice->Append(wxString());
  PStringList presentities = manager->GetPresentities();
  for (PStringList::iterator it = presentities.begin(); it != presentities.end(); ++it)
    choice->Append(PwxString(*it));

  FindWindowByName(wxT("SpeedDialDescription"))->SetValidator(wxGenericValidator(&m_Description));

  m_inUse = FindWindowByNameAs<wxStaticText>(this, wxT("SpeedDialInUse"));
  m_ambiguous = FindWindowByNameAs<wxStaticText>(this, wxT("SpeedDialAmbiguous"));
}


void SpeedDialDialog::OnChange(wxCommandEvent & WXUNUSED(event))
{
  wxString newName = m_nameCtrl->GetValue();
  bool inUse = newName != m_Name && m_manager.HasSpeedDialName(newName);
  m_inUse->Show(inUse);

  m_ok->Enable(!inUse && !newName.IsEmpty() && !m_addressCtrl->GetValue().IsEmpty());

  m_ambiguous->Show(m_manager.HasSpeedDialNumber(m_numberCtrl->GetValue(), m_Number));
}


///////////////////////////////////////////////////////////////////////////////

MyPCSSEndPoint::MyPCSSEndPoint(MyManager & manager)
  : OpalPCSSEndPoint(manager),
    m_manager(manager)
{
}


PBoolean MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  m_manager.PostEvent(wxEvtRinging, ID_RINGING, connection.GetCall().GetToken());
  return true;
}


PBoolean MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  PTime now;
  LogWindow << connection.GetRemotePartyName() << " is ringing on "
            << now.AsString("w h:mma") << " ..." << endl;
  return true;
}


OpalMediaFormatList MyPCSSEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList list = OpalPCSSEndPoint::GetMediaFormats();
#if OPAL_FAX
  list += OpalT38;  // We can deal with T.38 so include it in list
#endif
  return list;
}


PSoundChannel * MyPCSSEndPoint::CreateSoundChannel(const OpalPCSSConnection & connection,
                                                   const OpalMediaFormat & mediaFormat,
                                                   PBoolean isSource)
{
  PSoundChannel * channel = OpalPCSSEndPoint::CreateSoundChannel(connection, mediaFormat, isSource);
  if (channel != NULL)
    return channel;

  LogWindow << "Could not open ";
  if (isSource)
    LogWindow << "record device \"" << connection.GetSoundChannelRecordDevice();
  else
    LogWindow << "player device \"" << connection.GetSoundChannelPlayDevice();
  LogWindow << '"' << endl;

  return NULL;
}


#if OPAL_H323
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

#endif
///////////////////////////////////////////////////////////////////////////////

#if OPAL_SIP

MySIPEndPoint::MySIPEndPoint(MyManager & manager)
  : SIPEndPoint(manager),
    m_manager(manager)
{
}


void MySIPEndPoint::OnRegistrationStatus(const RegistrationStatus & status)
{
  SIPEndPoint::OnRegistrationStatus(status);

  switch (status.m_reason) {
    default:
      break;

    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Information_Trying :
      return;

    case SIP_PDU::Successful_OK :
      if (status.m_reRegistering)
        return;
  }

  SIPURL aor = status.m_addressofRecord;
  aor.Sanitise(SIPURL::ExternalURI);

  LogWindow << "SIP ";
  if (!status.m_wasRegistering)
    LogWindow << "un";
  LogWindow << "registration of " << aor << ' ';
  switch (status.m_reason) {
    case SIP_PDU::Successful_OK :
      LogWindow << "successful";
      break;

    case SIP_PDU::Failure_RequestTimeout :
      LogWindow << "timed out";
      break;

    default :
      LogWindow << "failed (" << status.m_reason << ')';
  }
  LogWindow << '.' << endl;

  if (!status.m_wasRegistering)
    m_manager.StartRegistrations();
}


void MySIPEndPoint::OnSubscriptionStatus(const SubscriptionStatus & status)
{
  SIPEndPoint::OnSubscriptionStatus(status);

  switch (status.m_reason) {
    default:
      break;
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Information_Trying :
      return;

    case SIP_PDU::Successful_OK :
      if (status.m_reSubscribing)
        return;
  }

  SIPURL uri = status.m_addressofRecord;
  uri.Sanitise(SIPURL::ExternalURI);

  LogWindow << "SIP ";
  if (!status.m_wasSubscribing)
    LogWindow << "un";
  LogWindow << "subscription of " << uri << " to " << status.m_handler->GetEventPackage() << " events ";
  switch (status.m_reason) {
    case SIP_PDU::Successful_OK :
      LogWindow << "successful";
      break;

    case SIP_PDU::Failure_RequestTimeout :
      LogWindow << "timed out";
      break;

    default :
      LogWindow << "failed (" << status.m_reason << ')';
  }
  LogWindow << '.' << endl;

  if (!status.m_wasSubscribing)
    m_manager.StartRegistrations();
}


void MySIPEndPoint::OnDialogInfoReceived(const SIPDialogNotification & info)
{
  SIPEndPoint::OnDialogInfoReceived(info);

  switch (info.m_state) {
    case SIPDialogNotification::Terminated :
      LogWindow << "Line " << info.m_entity << " available." << endl;
      break;

    case SIPDialogNotification::Trying :
      LogWindow << "Line " << info.m_entity << " in use." << endl;
      break;

    case SIPDialogNotification::Confirmed :
      LogWindow << "Line " << info.m_entity << " connected";
      if (info.m_remote.m_URI.IsEmpty())
        LogWindow << " to " << info.m_remote.m_URI;
      LogWindow << '.' << endl;
      break;

    default :
      break;
  }
}

#endif // OPAL_SIP


#if OPAL_FAX

void MyFaxEndPoint::OnFaxCompleted(OpalFaxConnection & connection, bool failed)
{
  OpalMediaStatistics stats;
  connection.GetStatistics(stats);

  LogWindow << "FAX call ";
  switch (stats.m_fax.m_result) {
    case -2 :
      LogWindow << "failed to establish.";
      break;

    case -1 :
      LogWindow << "partially ";
    case 0 :
      LogWindow << "successful ";
      if (connection.IsReceive())
        LogWindow << "receipt, " << stats.m_fax.m_rxPages;
      else
        LogWindow << "send, " << stats.m_fax.m_txPages;
      LogWindow << " of " << stats.m_fax.m_totalPages << " pages, "
                << stats.m_fax.m_imageSize << " bytes";
      if (connection.IsReceive())
        LogWindow << ", " << stats.m_fax.m_badRows << " bad rows";
      break;

    case 41 :
      LogWindow << "failed to open TIFF file " << connection.GetFileName();
      break;

    case 42 :
    case 43 :
    case 44 :
    case 45 :
    case 46 :
      LogWindow << "illegal TIFF file " << connection.GetFileName();
      break;

    default :
      LogWindow << "failed with T.30 error code " << stats.m_fax.m_result;
  }

  if (stats.m_fax.m_result > 0 && !stats.m_fax.m_errorText.IsEmpty())
    LogWindow << ": " << stats.m_fax.m_errorText;

  LogWindow << endl;

  OpalFaxEndPoint::OnFaxCompleted(connection, failed);
}

#endif // OPAL_FAX


// End of File ///////////////////////////////////////////////////////////////
