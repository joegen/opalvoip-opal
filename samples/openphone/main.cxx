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
 */

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
#include <wx/progdlg.h>
#include <wx/xrc/xmlres.h>
#include <wx/cmdline.h>

#include <ptclib/pstun.h>
#include <ptclib/pwavfile.h>
#include <ptclib/dtmf.h>

#include <opal/mediasession.h>
#include <opal/patch.h>
#include <opal/transcoders.h>
#include <ep/ivr.h>
#include <ep/opalmixer.h>
#include <ep/skinnyep.h>
#include <sip/sippres.h>
#include <lids/lidep.h>
#include <lids/capi_ep.h>
#include <codec/vidcodec.h>
#include <rtp/srtp_session.h>

#include <algorithm>

#ifdef WIN32
  #include <shellapi.h>
#endif


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
#endif

#if defined(_WIN32) && 0
  #define VIDEO_WINDOW_DRIVER P_MSWIN_VIDEO_DRIVER
  #define VIDEO_WINDOW_DEVICE P_MSWIN_VIDEO_PREFIX" STYLE=0x80C80000"  // WS_POPUP|WS_BORDER|WS_SYSMENU|WS_CAPTION
  #define VIDEO_WINDOW_AUTO_DELETE 1
#else
  #include <ptclib/wxWinVidDev.h>
  #define VIDEO_WINDOW_DRIVER P_WXWINDOWS_DEVICE_NAME
  #define VIDEO_WINDOW_DEVICE P_WXWINDOWS_DEVICE_NAME
  #define VIDEO_WINDOW_AUTO_DELETE 0
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

static const wxChar GeneralGroup[] = wxT("/General");
DEF_FIELD(Username);
DEF_FIELD(DisplayName);
DEF_FIELD(RingSoundDeviceName);
DEF_FIELD(RingSoundFileName);
DEF_FIELD(AutoAnswer);
DEF_FIELD(HideMinimised);
DEF_FIELD(OverrideProductInfo);
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
DEF_FIELD(RxBandwidth);
DEF_FIELD(TxBandwidth);
DEF_FIELD(RTPTOS);
DEF_FIELD(DiffServAudio);
DEF_FIELD(DiffServVideo);
DEF_FIELD(MaxRtpPayloadSize);
DEF_FIELD(CertificateAuthority);
DEF_FIELD(LocalCertificate);
DEF_FIELD(PrivateKey);
DEF_FIELD(TCPPortBase);
DEF_FIELD(TCPPortMax);
DEF_FIELD(UDPPortBase);
DEF_FIELD(UDPPortMax);
DEF_FIELD(RTPPortBase);
DEF_FIELD(RTPPortMax);

static const wxChar NatMethodsGroup[] = wxT("/Networking/NAT Methods");

DEF_FIELD(NATActive);
DEF_FIELD(NATServer);
DEF_FIELD(NATPriority);

DEF_FIELD(NATHandling);
DEF_FIELD(NATRouter);
DEF_FIELD(STUNServer);

static const wxChar LocalInterfacesGroup[] = wxT("/Networking/Interfaces");

static const wxChar AudioGroup[] = wxT("/Audio");
DEF_FIELD(SoundPlayer);
DEF_FIELD(SoundRecorder);
DEF_FIELD(SoundBufferTime);
DEF_FIELD(EchoCancellation);
DEF_FIELD(LocalRingbackTone);
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
DEF_FIELD(MusicOnHold);
DEF_FIELD(AudioOnRing);

static const wxChar VideoGroup[] = wxT("/Video");
DEF_FIELD(VideoGrabDevice);
DEF_FIELD(VideoGrabFormat);
DEF_FIELD(VideoGrabSource);
DEF_FIELD(VideoGrabFrameRate);
DEF_FIELD(VideoGrabFrameSize);
DEF_FIELD(VideoGrabPreview);
DEF_FIELD(VideoGrabBitRate);
DEF_FIELD(VideoWatermarkDevice);
DEF_FIELD(VideoMaxBitRate);
DEF_FIELD(VideoFlipLocal);
DEF_FIELD(VideoAutoTransmit);
DEF_FIELD(VideoAutoReceive);
DEF_FIELD(VideoFlipRemote);
DEF_FIELD(VideoMinFrameSize);
DEF_FIELD(VideoMaxFrameSize);
DEF_FIELD(VideoOnHold);
DEF_FIELD(VideoOnRing);

static const wxChar * const VideoWindowKeyBase[2][OpalVideoFormat::NumContentRole] = {
  { wxT("RemoteVideoFrame"), wxT("PresentationVideoFrame"), wxT("RemoteVideoFrame"), wxT("SpeakerVideoFrame"), wxT("SignLanguageVideoFrame") },
  { wxT("LocalVideoFrame"), wxT("PresentationPreviewFrame"), wxT("LocalVideoFrame"), wxT("SpeakerPreviewFrame"), wxT("SignLanguagePreviewFrame") }
};

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
DEF_FIELD(GatekeeperInterface);
DEF_FIELD(GatekeeperSuppressGRQ);
DEF_FIELD(GatekeeperIdentifier);
DEF_FIELD(GatekeeperTTL);
DEF_FIELD(GatekeeperLogin);
DEF_FIELD(GatekeeperPassword);
DEF_FIELD(H323TerminalType);
DEF_FIELD(DTMFSendMode);
#if OPAL_450
DEF_FIELD(CallIntrusionProtectionLevel);
#endif
DEF_FIELD(DisableFastStart);
DEF_FIELD(DisableH245Tunneling);
DEF_FIELD(DisableH245inSETUP);
DEF_FIELD(ForceSymmetricTCS);
DEF_FIELD(ExtendedVideoRoles);
DEF_FIELD(EnableH239Control);
#if OPAL_PTLIB_SSL
DEF_FIELD(H323SignalingSecurity);
DEF_FIELD(H323MediaCryptoSuites);
#endif

static const wxChar H323AliasesGroup[] = wxT("/H.323/Aliases");
static const wxChar H323StringOptionsGroup[] = wxT("/H.323/Options");

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

static const wxChar SIPStringOptionsGroup[] = wxT("/SIP/Options");

static const wxChar RegistrarGroup[] = wxT("/SIP/Registrars");
DEF_FIELD(RegistrationType);
DEF_FIELD(RegistrarUsed);
DEF_FIELD(RegistrarName);
DEF_FIELD(RegistrarDomain);
DEF_FIELD(RegistrarContact);
DEF_FIELD(RegistrarInstanceId);
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
static const wxChar OpenPhoneString[] = wxT("OpenPhone");
static const wxChar OpenPhoneErrorString[] = wxT("OpenPhone Error");


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
    ".*:.*\t.*:(fax|329)@.*|(fax|329) = fax:*;receive",
    ".*:.*\t.*:(t38|838)@.*|(t38|838) = t38:*;receive",
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


static PConstString DefaultAttributeValue("-default-");


// Menu and command identifiers
#define DEF_XRCID(name) static int ID_##name = XRCID(#name)
DEF_XRCID(MenuCall);
DEF_XRCID(MenuCallLastDialed);
DEF_XRCID(MenuCallLastReceived);
DEF_XRCID(MenuAnswer);
DEF_XRCID(MenuHangUp);
DEF_XRCID(MenuHold);
DEF_XRCID(MenuTransfer);
DEF_XRCID(MenuStartRecording);
DEF_XRCID(MenuStopRecording);
DEF_XRCID(CallSpeedDialAudio);
DEF_XRCID(CallSpeedDialHandset);
DEF_XRCID(MenuSendFax);
DEF_XRCID(SendFaxSpeedDial);
DEF_XRCID(NewSpeedDial);
DEF_XRCID(ViewLarge);
DEF_XRCID(ViewSmall);
DEF_XRCID(ViewList);
DEF_XRCID(ViewDetails);
DEF_XRCID(EditSpeedDial);
DEF_XRCID(MenuCut);
DEF_XRCID(MenuCopy);
DEF_XRCID(MenuPaste);
DEF_XRCID(MenuDelete);
DEF_XRCID(MenuSendAudioFile);
DEF_XRCID(MenuAudioDevice);
DEF_XRCID(MenuStartVideo);
DEF_XRCID(MenuStopVideo);
DEF_XRCID(MenuSendVFU);
DEF_XRCID(MenuSendIntra);
DEF_XRCID(MenuTxVideoControl);
DEF_XRCID(MenuRxVideoControl);
DEF_XRCID(MenuPresentationRole);
DEF_XRCID(MenuDefVidWinPos);
DEF_XRCID(MenuPresence);
#if OPAL_HAS_IM
DEF_XRCID(MenuStartIM);
DEF_XRCID(MenuInCallMessage);
DEF_XRCID(SendIMSpeedDial);
#endif // OPAL_HAS_IM
DEF_XRCID(SubMenuAudio);
DEF_XRCID(SubMenuVideo);
DEF_XRCID(SubMenuRetrieve);
DEF_XRCID(SubMenuConference);
DEF_XRCID(SubMenuTransfer);
DEF_XRCID(SubMenuSound);
DEF_XRCID(SubMenuGrabber);
DEF_XRCID(SubMenuSpeedDial);


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
  ID_VIDEO_DEVICE_MENU_BASE,
  ID_VIDEO_DEVICE_MENU_TOP = ID_VIDEO_DEVICE_MENU_BASE+99,
  ID_VIDEO_CODEC_MENU_BASE,
  ID_VIDEO_CODEC_MENU_TOP = ID_VIDEO_CODEC_MENU_BASE+99,
  ID_SPEEDDIAL_MENU_BASE,
  ID_SPEEDDIAL_MENU_TOP = ID_SPEEDDIAL_MENU_BASE+999
};


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
DEFINE_USER_COMMAND(wxEvtStreamsChanged);
DEFINE_USER_COMMAND(wxEvtRinging);
DEFINE_USER_COMMAND(wxEvtEstablished);
DEFINE_USER_COMMAND(wxEvtOnHold);
DEFINE_USER_COMMAND(wxEvtCleared);
DEFINE_USER_COMMAND(wxEvtSetTrayTipText);
DEFINE_USER_COMMAND(wxEvtTestVideoEnded);
DEFINE_USER_COMMAND(wxEvtGetSSLPassword);

DEFINE_USER_COMMAND(wxEvtAsyncNotification);

static void PostAsyncNotification(wxWindow & wnd)
{
  wxCommandEvent theEvent(wxEvtAsyncNotification);
  theEvent.SetEventObject(&wnd);
  wnd.GetEventHandler()->AddPendingEvent(theEvent);
}


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


void RemoveNotebookPage(wxWindow * window, const wxChar * name)
{
  wxNotebook * book;
  if (FindWindowByNameAs(book, window, wxT("OptionsNotebook")) == NULL)
    return;

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


static PwxString MediaDeviceNameToScreen(const PString & name)
{
  PwxString str = name;
  str.Replace(wxT("\t"), wxT(": "));
  return str;
}

static PString MediaDeviceNameFromScreen(const wxString & name)
{
  PwxString str = name;
  str.Replace(wxT(": "), wxT("\t"));
  return str.p_str();
}

static void FillAudioDeviceComboBox(wxItemContainer * list, PSoundChannel::Directions dir)
{
  PStringArray devices = PSoundChannel::GetDeviceNames(dir);
  for (PINDEX i = 0; i < devices.GetSize(); i++)
    list->Append(MediaDeviceNameToScreen(devices[i]));
}


///////////////////////////////////////////////////////////////////////////////

class TextCtrlChannel : public PChannel
{
    PCLASSINFO(TextCtrlChannel, PChannel)
  private:
    static wxFrame * sm_frame;

  public:
    TextCtrlChannel()
      { }

    virtual PString GetName() const
    {
      return "TextCtrl";
    }

    static void SetFrame(wxFrame * frame)
    {
      sm_frame = frame;
    }

    virtual PBoolean Write(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
    ) {
      if (sm_frame == NULL)
        return false;

      wxCommandEvent theEvent(wxEvtLogMessage);
      theEvent.SetEventObject(sm_frame);
      PwxString str(wxString::FromUTF8((const char *)buf, (size_t)len-1));
      theEvent.SetString(str);
      sm_frame->GetEventHandler()->AddPendingEvent(theEvent);
      PTRACE(3, "OpenPhone\t" << str);
      return true;
    }

    static PThreadLocalStorage<TextCtrlChannel> sm_instances;
};

wxFrame * TextCtrlChannel::sm_frame;
PThreadLocalStorage<TextCtrlChannel> TextCtrlChannel::sm_instances;

#define LogWindow *TextCtrlChannel::sm_instances


///////////////////////////////////////////////////////////////////////////////

MyManager::VideoWindowInfo::VideoWindowInfo(const PVideoOutputDevice * device)
{
  if (device != NULL && device->GetPosition(x, y))
    size = device->GetChannel();
  else
    x = y = size = INT_MIN;
}


bool MyManager::VideoWindowInfo::operator==(const VideoWindowInfo & info) const
{
  return x == info.x && y == info.y && size == info.size;
}


bool MyManager::VideoWindowInfo::ReadConfig(wxConfigBase * config, const wxString & keyBase)
{
  return config->Read(keyBase + wxT("X"), &x) &&
         config->Read(keyBase + wxT("Y"), &y) &&
         config->Read(keyBase + wxT("Size"), &size);
}


void MyManager::VideoWindowInfo::WriteConfig(wxConfigBase * config, const wxString & keyBase)
{
  config->Write(keyBase + wxT("X"), x);
  config->Write(keyBase + wxT("Y"), y);
  config->Write(keyBase + wxT("Size"), size);
}


///////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_APP(OpenPhoneApp);

OpenPhoneApp::OpenPhoneApp()
  : PProcess(MANUFACTURER_TEXT, PRODUCT_NAME_TEXT,
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
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
      wxCMD_LINE_DESC(wxCMD_LINE_PARAM,  "", "", "URI to call", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL),
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
    main->MakeCall(cmdLine.GetParam());

  wxEndBusyCursor();
  return ok;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyManager, wxFrame)
  EVT_CLOSE(MyManager::OnClose)
  EVT_ICONIZE(MyManager::OnIconize)

  EVT_MENU_OPEN(MyManager::OnAdjustMenus)

  EVT_MENU(wxID_EXIT,               MyManager::OnMenuQuit)
  EVT_MENU(wxID_ABOUT,              MyManager::OnMenuAbout)
  EVT_MENU(wxID_PREFERENCES,        MyManager::OnOptions)
  EVT_MENU(ID_MenuCall,             MyManager::OnMenuCall)
  EVT_MENU(ID_MenuCallLastDialed,   MyManager::OnMenuCallLastDialed)
  EVT_MENU(ID_MenuCallLastReceived, MyManager::OnMenuCallLastReceived)
  EVT_MENU(ID_MenuAnswer,           MyManager::OnMenuAnswer)
  EVT_MENU(ID_MenuHangUp,           MyManager::OnMenuHangUp)
  EVT_MENU(ID_MenuHold,             MyManager::OnRequestHold)
  EVT_MENU(ID_MenuTransfer,         MyManager::OnTransfer)
  EVT_MENU(ID_MenuStartRecording,   MyManager::OnStartRecording)
  EVT_MENU(ID_MenuStopRecording,    MyManager::OnStopRecording)
  EVT_MENU(ID_CallSpeedDialAudio,   MyManager::OnCallSpeedDialAudio)
  EVT_MENU(ID_CallSpeedDialHandset, MyManager::OnCallSpeedDialHandset)
  EVT_MENU(ID_MenuSendFax,          MyManager::OnSendFax)
  EVT_MENU(ID_SendFaxSpeedDial,     MyManager::OnSendFaxSpeedDial)
  EVT_MENU(ID_NewSpeedDial,         MyManager::OnNewSpeedDial)
  EVT_MENU(ID_ViewLarge,            MyManager::OnViewLarge)
  EVT_MENU(ID_ViewSmall,            MyManager::OnViewSmall)
  EVT_MENU(ID_ViewList,             MyManager::OnViewList)
  EVT_MENU(ID_ViewDetails,          MyManager::OnViewDetails)
  EVT_MENU(ID_EditSpeedDial,        MyManager::OnEditSpeedDial)
  EVT_MENU(ID_MenuCut,              MyManager::OnCutSpeedDial)
  EVT_MENU(ID_MenuCopy,             MyManager::OnCopySpeedDial)
  EVT_MENU(ID_MenuPaste,            MyManager::OnPasteSpeedDial)
  EVT_MENU(ID_MenuDelete,           MyManager::OnDeleteSpeedDial)
  EVT_MENU(ID_MenuSendAudioFile,    MyManager::OnSendAudioFile)
  EVT_MENU(ID_MenuAudioDevice,      MyManager::OnAudioDevicePair)
  EVT_MENU(ID_MenuStartVideo,       MyManager::OnStartVideo)
  EVT_MENU(ID_MenuStopVideo,        MyManager::OnStopVideo)
  EVT_MENU(ID_MenuSendVFU,          MyManager::OnSendVFU)
  EVT_MENU(ID_MenuSendIntra,        MyManager::OnSendIntra)
  EVT_MENU(ID_MenuTxVideoControl,   MyManager::OnTxVideoControl)
  EVT_MENU(ID_MenuRxVideoControl,   MyManager::OnRxVideoControl)
  EVT_MENU(ID_MenuPresentationRole, MyManager::OnMenuPresentationRole)
  EVT_MENU(ID_MenuDefVidWinPos,     MyManager::OnDefVidWinPos)
#if OPAL_HAS_PRESENCE
  EVT_MENU(ID_MenuPresence,         MyManager::OnMyPresence)
#if OPAL_HAS_IM
  EVT_MENU(ID_MenuStartIM,          MyManager::OnStartIM)
  EVT_MENU(ID_MenuInCallMessage,    MyManager::OnInCallIM)
  EVT_MENU(ID_SendIMSpeedDial,      MyManager::OnSendIMSpeedDial)
#endif // OPAL_HAS_IM
#endif // OPAL_HAS_PRESENCE

  EVT_MENU_RANGE(ID_RETRIEVE_MENU_BASE,     ID_RETRIEVE_MENU_TOP,     MyManager::OnRetrieve)
  EVT_MENU_RANGE(ID_TRANSFER_MENU_BASE,     ID_TRANSFER_MENU_TOP,     MyManager::OnTransfer)
  EVT_MENU_RANGE(ID_CONFERENCE_MENU_BASE,   ID_CONFERENCE_MENU_TOP,   MyManager::OnConference)
  EVT_MENU_RANGE(ID_AUDIO_DEVICE_MENU_BASE, ID_AUDIO_DEVICE_MENU_TOP, MyManager::OnAudioDeviceChange)
  EVT_MENU_RANGE(ID_AUDIO_CODEC_MENU_BASE,  ID_AUDIO_CODEC_MENU_TOP,  MyManager::OnNewCodec)
  EVT_MENU_RANGE(ID_VIDEO_DEVICE_MENU_BASE, ID_VIDEO_DEVICE_MENU_TOP, MyManager::OnVideoDeviceChange)
  EVT_MENU_RANGE(ID_VIDEO_CODEC_MENU_BASE,  ID_VIDEO_CODEC_MENU_TOP,  MyManager::OnNewCodec)

  EVT_SPLITTER_SASH_POS_CHANGED(SplitterID, MyManager::OnSashPositioned)
  EVT_LIST_ITEM_ACTIVATED(SpeedDialsID,     MyManager::OnSpeedDialActivated)
  EVT_LIST_COL_END_DRAG(SpeedDialsID,       MyManager::OnSpeedDialColumnResize)
  EVT_LIST_ITEM_RIGHT_CLICK(SpeedDialsID,   MyManager::OnSpeedDialRightClick)
  EVT_LIST_END_LABEL_EDIT(SpeedDialsID,     MyManager::OnSpeedDialEndEdit)

  EVT_USER_COMMAND(wxEvtLogMessage,         MyManager::OnLogMessage)
  EVT_USER_COMMAND(wxEvtRinging,            MyManager::OnEvtRinging)
  EVT_USER_COMMAND(wxEvtEstablished,        MyManager::OnEvtEstablished)
  EVT_USER_COMMAND(wxEvtOnHold,             MyManager::OnEvtOnHold)
  EVT_USER_COMMAND(wxEvtCleared,            MyManager::OnEvtCleared)
  EVT_USER_COMMAND(wxEvtStreamsChanged,     MyManager::OnStreamsChanged)
  EVT_USER_COMMAND(wxEvtAsyncNotification,  MyManager::OnEvtAsyncNotification)
  EVT_USER_COMMAND(wxEvtSetTrayTipText,     MyManager::OnSetTrayTipText)
#if OPAL_PTLIB_SSL
  EVT_USER_COMMAND(wxEvtGetSSLPassword,     MyManager::OnEvtGetSSLPassword)
#endif

END_EVENT_TABLE()


MyManager::MyManager()
  : wxFrame(NULL, -1, OpenPhoneString, wxDefaultPosition, wxSize(640, 480))
#if OPAL_FAX
  , m_currentAnswerMode(AnswerDetect)
  , m_defaultAnswerMode(AnswerDetect)
#endif // OPAL_FAX
  , m_hideMinimised(false)
  , m_taskBarIcon(NULL)
  , m_splitter(NULL)
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
  , m_OverrideProductInfo(false)
  , m_ForwardingTimeout(30)
  , m_VideoGrabPreview(true)
  , m_primaryVideoGrabber(NULL)
  , m_SecondaryVideoGrabPreview(false)
  , m_SecondaryVideoOpening(false)
  , m_VideoMaxBitRate(1024)
  , m_VideoTargetBitRate(256)
  , m_ExtendedVideoRoles(e_DisabledExtendedVideoRoles)
#if PTRACING
  , m_enableTracing(false)
#endif
  , m_lastRecordFile("*.wav")
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

  TextCtrlChannel::SetFrame(this);
}


MyManager::~MyManager()
{
  while (AsyncNotifierExecute())
    ;

  TextCtrlChannel::SetFrame(NULL);

  ShutDownEndpoints();

  delete m_primaryVideoGrabber;

  delete m_taskBarIcon;

  wxMenuBar * menubar = GetMenuBar();
  SetMenuBar(NULL);
  delete menubar;

  delete m_imageListNormal;
  delete m_imageListSmall;

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


void MyManager::AsyncNotifierSignal()
{
  PostAsyncNotification(*this);
}


void MyManager::OnEvtAsyncNotification(wxCommandEvent &)
{
  while (AsyncNotifierExecute())
    ;
}


bool MyManager::Initialise(bool startMinimised)
{
  wxImage::AddHandler(new wxGIFHandler);
  wxXmlResource::Get()->InitAllHandlers();
  InitXmlResource();

  // Task bar icon
  m_taskBarIcon = new MyTaskBarIcon(*this);
  m_taskBarIcon->SetIcon(wxICON(AppIcon), wxEmptyString);
  SetTrayTipText("Initialising");

  // Make a menubar
  wxMenuBar * menubar;
  {
    PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE;
    if ((menubar = wxXmlResource::Get()->LoadMenuBar(wxT("MenuBar"))) == NULL)
      return false;
    SetMenuBar(menubar);
  }

  wxAcceleratorEntry accelEntries[] = {
      wxAcceleratorEntry(wxACCEL_CTRL,  'E',         ID_EditSpeedDial),
      wxAcceleratorEntry(wxACCEL_CTRL,  'X',         ID_MenuCut),
      wxAcceleratorEntry(wxACCEL_CTRL,  'C',         ID_MenuCopy),
      wxAcceleratorEntry(wxACCEL_CTRL,  'V',         ID_MenuPaste),
      wxAcceleratorEntry(wxACCEL_NORMAL, WXK_DELETE, ID_MenuDelete)
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
  {
    config->SetPath(SpeedDialsGroup);
    SpeedDialInfo info;
    long groupIndex;
    if (config->GetFirstGroup(info.m_Name, groupIndex)) {
      do {
        config->SetPath(info.m_Name);
        if (config->Read(SpeedDialAddressKey, &info.m_Address) && !info.m_Address.empty()) {
          config->Read(SpeedDialNumberKey, &info.m_Number);
          #if OPAL_HAS_PRESENCE
          if (!config->Read(SpeedDialPresentityKey, &info.m_Presentity))
            config->Read(SpeedDialStateURLKey, &info.m_Presentity);
          #endif // OPAL_HAS_PRESENCE
          config->Read(SpeedDialDescriptionKey, &info.m_Description);
          m_speedDialInfo.insert(info);
        }
        config->SetPath(wxT(".."));
      } while (config->GetNextGroup(info.m_Name, groupIndex));
    }
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
  if (startMinimised)
    Iconize();
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

#if OPAL_SKINNY
  new OpalSkinnyEndPoint(*this);
#endif

#if OPAL_CAPI
  capiEP = new OpalCapiEndPoint(*this);
#endif

#if OPAL_IVR
  ivrEP = new OpalIVREndPoint(*this);
#endif

#if OPAL_HAS_MIXER
  m_mcuEP = new OpalMixerEndPoint(*this);
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
  config->Read(HideMinimisedKey, &m_hideMinimised);
  config->Read(LastDialedKey, &m_LastDialed);


  config->Read(OverrideProductInfoKey, &m_OverrideProductInfo);
  m_overriddenProductInfo = GetProductInfo();
  if (config->Read(VendorNameKey, &str))
    m_overriddenProductInfo.vendor = str.p_str();
  if (config->Read(ProductNameKey, &str) && !str.IsEmpty())
    m_overriddenProductInfo.name = str.p_str();
  if (config->Read(ProductVersionKey, &str))
    m_overriddenProductInfo.version = str.p_str();
  if (m_OverrideProductInfo)
    SetProductInfo(m_overriddenProductInfo);

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
    h323EP->SetInitialBandwidth(OpalBandwidth::RxTx, value1);

  double float1;
  if (config->Read(RxBandwidthKey, &float1))
    h323EP->SetInitialBandwidth(OpalBandwidth::Rx, (unsigned)(float1*1000));

  if (config->Read(TxBandwidthKey, &float1))
    h323EP->SetInitialBandwidth(OpalBandwidth::Tx, (unsigned)(float1*1000));
#endif
  if (config->Read(RTPTOSKey, &value1)) {
    SetMediaTypeOfService(value1);
    config->DeleteEntry(RTPTOSKey);
  }
  if (config->Read(DiffServAudioKey, &value1)) {
    PIPSocket::QoS qos = GetMediaQoS(OpalMediaType::Audio());
    qos.m_dscp = value1;
    SetMediaQoS(OpalMediaType::Audio(), qos);
  }
  if (config->Read(DiffServVideoKey, &value1)) {
    PIPSocket::QoS qos = GetMediaQoS(OpalMediaType::Video());
    qos.m_dscp = value1;
    SetMediaQoS(OpalMediaType::Video(), qos);
  }
  if (config->Read(MaxRtpPayloadSizeKey, &value1))
    SetMaxRtpPayloadSize(value1);
#if OPAL_PTLIB_SSL
  if (config->Read(CertificateAuthorityKey, &str))
    SetSSLCertificateAuthorityFiles(str);
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

  if (config->Read(NATRouterKey, &str)) {
    config->DeleteEntry(NATRouterKey);
    config->SetPath(NatMethodsGroup);
    config->SetPath(PwxString(PNatMethod_Fixed::MethodName()));
    config->Write(NATServerKey, str);
    config->SetPath(NetworkingGroup);
  }

  if (config->Read(STUNServerKey, &str)) {
    config->DeleteEntry(STUNServerKey);
    config->SetPath(NatMethodsGroup);
    config->SetPath(PwxString(PSTUNClient::MethodName()));
    config->Write(NATServerKey, str);
    config->SetPath(NetworkingGroup);
  }

  if (config->Read(NATHandlingKey, &value1) && value1 > 0) {
    config->DeleteEntry(NATHandlingKey);
    config->SetPath(NatMethodsGroup);
    config->SetPath(PwxString(value1 == 2 ? PSTUNClient::MethodName() : PNatMethod_Fixed::MethodName()));
    config->Write(NATActiveKey, true);
    config->SetPath(NetworkingGroup);
  }

  {
    PNatMethods & methods = GetNatMethods();
    std::list<PwxString> methodNames;
    for (PNatMethods::iterator it = methods.begin(); it != methods.end(); ++it)
      methodNames.push_back(it->GetMethodName());
    for (std::list<PwxString>::iterator method = methodNames.begin(); method != methodNames.end(); ++method) {
      PNatMethod * it = methods.GetMethodByName(method->p_str());
      if (PAssertNULL(it)) {
        config->SetPath(NatMethodsGroup);
        config->SetPath(*method);
        config->Read(NATActiveKey, &onoff, it->IsActive());
        config->Read(NATPriorityKey, &value1, it->GetPriority());
        SetNAT(*method, onoff, config->Read(NATServerKey), value1);
      }
    }
  }


  config->SetPath(LocalInterfacesGroup);
  PwxString entryName;
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
  if (config->Read(MusicOnHoldKey, &str))
    pcssEP->SetSoundChannelOnHoldDevice(str);
  if (config->Read(AudioOnRingKey, &str))
    pcssEP->SetSoundChannelOnRingDevice(str);

#if OPAL_AEC
  OpalEchoCanceler::Params aecParams = GetEchoCancelParams();
  config->Read(EchoCancellationKey, &aecParams.m_enabled);
  SetEchoCancelParams(aecParams);
#endif

  if (config->Read(LocalRingbackToneKey, &str))
    pcssEP->SetLocalRingbackTone(str);

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

  UpdateAudioVideoDevices();
  StartLID();


  ////////////////////////////////////////
  // Video fields
  config->SetPath(VideoGroup);
  PVideoDevice::OpenArgs videoArgs = GetVideoInputDevice();
  if (config->Read(VideoGrabDeviceKey, &str))
    videoArgs.deviceName = str.p_str();
  if (config->Read(VideoGrabFormatKey, &value1))
    videoArgs.videoFormat = PVideoDevice::VideoFormatFromInt(value1);
  if (config->Read(VideoGrabSourceKey, &value1) && value1 >= -1)
    videoArgs.channelNumber = value1;
  if (config->Read(VideoGrabFrameRateKey, &value1))
    videoArgs.rate = value1;
  if (videoArgs.rate == 0)
    videoArgs.rate = 15;
  config->Read(VideoFlipLocalKey, &videoArgs.flip);
  SetVideoInputDevice(videoArgs);
  m_SecondaryVideoGrabber = videoArgs;

  if (config->Read(VideoWatermarkDeviceKey, &str))
    m_VideoWatermarkDevice = str.p_str();

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

  videoArgs = pcssEP->GetVideoOnHoldDevice();
  if (config->Read(VideoOnHoldKey, &str))
    videoArgs.deviceName = str.p_str();
  pcssEP->SetVideoOnHoldDevice(videoArgs);

  videoArgs = pcssEP->GetVideoOnRingDevice();
  if (config->Read(VideoOnRingKey, &str))
    videoArgs.deviceName = str.p_str();
  pcssEP->SetVideoOnRingDevice(videoArgs);

  for (int preview = 0; preview < 2; ++preview) {
    for (OpalVideoFormat::ContentRole role = OpalVideoFormat::BeginContentRole; role < OpalVideoFormat::EndContentRole; ++role)
      m_videoWindowInfo[preview][role].ReadConfig(config, VideoWindowKeyBase[preview][role]);
  }

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

#if OPAL_HAS_PRESENCE
  ////////////////////////////////////////
  // Presence fields
  config->SetPath(PresenceGroup);
  for (int presIndex = 0; ; ++presIndex) {
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
          presentity->SetLocalPresence(OpalPresenceInfo(PwxString(config->Read(LastPresenceStateKey, wxT("Available")))));
        }
      }
    }

    config->SetPath(wxT(".."));
  }
#endif // OPAL_HAS_PRESENCE

  ////////////////////////////////////////
  // Codec fields
  {
    OpalMediaFormatList mediaFormats;
    PList<OpalEndPoint> endpoints = GetEndPoints();
    for (PList<OpalEndPoint>::iterator it = endpoints.begin(); it != endpoints.end(); ++it)
      mediaFormats += it->GetMediaFormats();

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
        if (codecName == mm->mediaFormat.GetName()) {
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
    PStringSet mediaFormatMask = GetMediaFormatMask();
    PStringArray mediaFormatOrder = GetMediaFormatOrder();
    for (PINDEX i = 0; i < mediaFormatOrder.GetSize(); i++) {
      if (mediaFormatMask.Contains(mediaFormatOrder[i]))
        continue;
      for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
        if (mm->mediaFormat == mediaFormatOrder[i])
          mm->preferenceOrder = codecIndex++;
      }
    }
    for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
      if (!mediaFormatMask.Contains(mm->mediaFormat.GetName()) && mm->preferenceOrder < 0)
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
      if (config->Read(entryName, &alias) && !alias.empty()) {
        h323EP->m_configuredAliasNames.AppendString(alias);
        h323EP->AddAliasName(alias);
      }
    } while (config->GetNextEntry(entryName, entryIndex));
  }

  config->SetPath(H323Group);

  if (config->Read(H323TerminalTypeKey, &value1) && value1 > 0 && value1 < 256)
    h323EP->SetTerminalType((H323EndPoint::TerminalTypes)value1);
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
  if (config->Read(ForceSymmetricTCSKey, &onoff))
    h323EP->ForceSymmetricTCS(onoff);

#if OPAL_H239
  value1 = m_ExtendedVideoRoles;
  config->Read(ExtendedVideoRolesKey, &value1);
  m_ExtendedVideoRoles = (ExtendedVideoRoles)value1;
  h323EP->SetDefaultH239Control(config->Read(EnableH239ControlKey, h323EP->GetDefaultH239Control()));
#endif

#if OPAL_PTLIB_SSL
  if (!config->Read(H323SignalingSecurityKey, &value1))
    value1 = 0;
  SetSignalingSecurity(h323EP, value1);
  if (config->Read(H323MediaCryptoSuitesKey, &str))
    h323EP->SetMediaCryptoSuites(str.p_str().Tokenise(','));
#endif

  config->SetPath(H323StringOptionsGroup);
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      PwxString value;
      if (config->Read(entryName, &value))
        h323EP->SetDefaultStringOption(entryName, value);
    } while (config->GetNextEntry(entryName, entryIndex));
  }

  config->SetPath(H323Group);
  config->Read(GatekeeperModeKey, &m_gatekeeperMode, 0);
  if (config->Read(GatekeeperTTLKey, &value1))
    h323EP->SetGatekeeperTimeToLive(PTimeInterval(0, value1));

  config->Read(GatekeeperLoginKey, &username, wxEmptyString);
  config->Read(GatekeeperPasswordKey, &password, wxEmptyString);
  h323EP->SetGatekeeperPassword(password, username);

  config->Read(GatekeeperAddressKey, &m_gatekeeperAddress, wxEmptyString);
  config->Read(GatekeeperInterfaceKey, &m_gatekeeperInterface, wxEmptyString);
  config->Read(GatekeeperSuppressGRQKey, &onoff);
  h323EP->SetSendGRQ(!onoff);
  config->Read(GatekeeperIdentifierKey, &m_gatekeeperIdentifier, wxEmptyString);
  if (!StartGatekeeper())
    return false;
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
  SetSignalingSecurity(sipEP, value1);
  if (config->Read(SIPMediaCryptoSuitesKey, &str))
    sipEP->SetMediaCryptoSuites(str.p_str().Tokenise(','));
#endif

  config->SetPath(SIPStringOptionsGroup);
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      PwxString value;
      if (config->Read(entryName, &value))
        sipEP->SetDefaultStringOption(entryName.p_str(), value.p_str());
    } while (config->GetNextEntry(entryName, entryIndex));
  }

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

  {
    config->SetPath(RegistrarGroup);
    wxString groupName;
    long groupIndex;
    if (config->GetFirstGroup(groupName, groupIndex)) {
      do {
        config->SetPath(groupName);
        if (registration.Read(*config))
          m_registrations.push_back(registration);
        config->SetPath(wxT(".."));
      } while (config->GetNextGroup(groupName, groupIndex));
    }
  }

  StartRegistrations();

#if OPAL_HAS_PRESENCE
  int count = m_speedDials->GetItemCount();
  for (int i = 0; i < count; i++) {
    SpeedDialInfo * info = (SpeedDialInfo *)m_speedDials->GetItemData(i);
    if (info != NULL && MonitorPresence(info->m_Presentity, info->m_Address, true) && m_speedDialDetail)
      m_speedDials->SetItem(i, e_StatusColumn, IconStatusNames[Icon_Unknown]);
  }
#endif // OPAL_HAS_PRESENCE
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

  OpalEndPoint * ep = FindEndPoint(PwxString(config->Read(telURIKey, wxT("sip"))));
  if (ep != NULL)
    AttachEndPoint(ep, "tel");

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
  SetTrayTipText("Online");
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


void MyManager::SetNAT(const PwxString & method, bool active, const PwxString & server, int priority)
{
  if (!server.empty()) {
    LogWindow << method << " server \"" << server << "\" being contacted ..." << endl;
    wxGetApp().ProcessPendingEvents();
    Update();
  }

  if (SetNATServer(method, server, active, priority))
    LogWindow << *GetNatMethods().GetMethodByName(method) << endl;
  else if (!server.empty())
    LogWindow << method << " server at " << server << " offline." << endl;
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
void MyManager::SetSignalingSecurity(OpalEndPoint * ep, int security)
{
  switch (security) {
    case 0 :
      AttachEndPoint(ep, ep->GetPrefixName());
      DetachEndPoint(ep->GetPrefixName() + 's');
      break;

    case 1 :
      AttachEndPoint(ep, ep->GetPrefixName() + 's');
      DetachEndPoint(ep->GetPrefixName());
      break;

    default :
      AttachEndPoint(ep, ep->GetPrefixName());
      AttachEndPoint(ep, ep->GetPrefixName() + 's');
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


void MyManager::OnClose(wxCloseEvent & /*event*/)
{
  ::wxBeginBusyCursor();
  SetTrayTipText("Exiting");

  wxProgressDialog progress(OpenPhoneString, wxT("Exiting ..."));
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
  m_tabs->DeleteAllPages();
  ShutDownEndpoints();

  m_taskBarIcon->RemoveIcon();

  Destroy();
}


void MyManager::OnIconize(wxIconizeEvent & evt)
{
  if (m_hideMinimised &&
#if wxCHECK_VERSION(2,9,2)
                         evt.IsIconized())
#else
                         evt.Iconized())
#endif
    Hide();
}


void MyManager::OnLogMessage(wxCommandEvent & theEvent)
{
  m_logWindow->AppendText(theEvent.GetString());
  m_logWindow->AppendText(wxChar('\n'));
#if defined(__WXMAC__) && wxCHECK_VERSION(2,9,2)
  // Workaround for OSX (2.9.2) bug
  m_logWindow->SetStyle(0, m_logWindow->GetLastPosition(), wxTextAttr(*wxGREEN, *wxBLACK));
#endif
}


bool MyManager::CanDoFax() const
{
#if OPAL_FAX
  return GetMediaFormatMask().GetValuesIndex(OpalT38.GetName()) == P_MAX_INDEX &&
         OpalMediaFormat(OPAL_FAX_TIFF_FILE).IsValid();
#else
  return false;
#endif
}


#if OPAL_HAS_PRESENCE && OPAL_HAS_IM
bool MyManager::CanDoIM() const
{
  SpeedDialInfo * info = GetSelectedSpeedDial();
  return info != NULL && !info->m_Presentity.IsEmpty();
}
#endif // OPAL_HAS_PRESENCE && OPAL_HAS_IM


void MyManager::OnAdjustMenus(wxMenuEvent & WXUNUSED(event))
{
  int id;

  wxMenuBar * menubar = GetMenuBar();
  menubar->Enable(ID_MenuCall,            m_activeCall == NULL);
  menubar->Enable(ID_MenuCallLastDialed,  m_activeCall == NULL && !m_LastDialed.IsEmpty());
  menubar->Enable(ID_MenuCallLastReceived,m_activeCall == NULL && !m_LastReceived.IsEmpty());
  menubar->Enable(ID_MenuAnswer,          !m_incomingToken.IsEmpty());
  menubar->Enable(ID_MenuHangUp,          m_activeCall != NULL);
  menubar->Enable(ID_MenuHold,            m_activeCall != NULL);
  menubar->Enable(ID_MenuTransfer,        m_activeCall != NULL);
  menubar->Enable(ID_MenuStartRecording,  m_activeCall != NULL && !m_activeCall->IsRecording());
  menubar->Enable(ID_MenuStopRecording,   m_activeCall != NULL &&  m_activeCall->IsRecording());
  menubar->Enable(ID_MenuSendAudioFile,   m_activeCall != NULL);
  menubar->Enable(ID_MenuInCallMessage,   m_activeCall != NULL);

  menubar->Enable(ID_MenuSendFax,         CanDoFax());

  for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
    menubar->Enable(it->m_retrieveMenuId,   m_activeCall == NULL);
    menubar->Enable(it->m_conferenceMenuId, m_activeCall != NULL);
    menubar->Enable(it->m_transferMenuId,   m_activeCall != NULL);
  }

  int count = m_speedDials->GetSelectedItemCount();
  menubar->Enable(ID_MenuCut,       count >= 1);
  menubar->Enable(ID_MenuCopy,      count >= 1);
  menubar->Enable(ID_MenuDelete,    count >= 1);
  menubar->Enable(ID_EditSpeedDial, count == 1);

  bool hasFormat = false;
  if (wxTheClipboard->Open()) {
    hasFormat = wxTheClipboard->IsSupported(m_ClipboardFormat) || wxTheClipboard->IsSupported(wxDF_TEXT);
    wxTheClipboard->Close();
  }
  menubar->Enable(ID_MenuPaste, hasFormat);

  wxString deviceName;
  menubar->Enable(ID_SubMenuSound, m_activeCall != NULL);
  PSafePtr<OpalPCSSConnection> pcss;
  if (GetConnection(pcss, PSafeReadOnly))
    deviceName = MediaDeviceNameToScreen(pcss->GetSoundChannelPlayDevice());
  for (id = ID_AUDIO_DEVICE_MENU_BASE; id <= ID_AUDIO_DEVICE_MENU_TOP; id++) {
    wxMenuItem * item = menubar->FindItem(id);
    if (item != NULL)
      item->Check(item->GetItemLabelText() == deviceName);
  }

  bool hasStartVideo = false;
  bool hasTxVideo = false;
  bool hasRxVideo = false;
  PwxString audioFormat, videoFormat;

  PSafePtr<OpalConnection> connection = GetNetworkConnection(PSafeReadOnly);
  if (connection != NULL) {
    // Get ID of open audio to check the menu item
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Audio(), true);
    if (stream != NULL)
      audioFormat = stream->GetMediaFormat().GetName();

    stream = connection->GetMediaStream(OpalMediaType::Video(), false);
    if (stream != NULL) {
      OpalVideoMediaStream * vidstream = dynamic_cast<OpalVideoMediaStream *>(&*stream);
      if (vidstream != NULL)
        deviceName = PwxString(vidstream->GetVideoInputDevice()->GetDeviceName());
      hasTxVideo = stream->IsOpen();
    }

    stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream != NULL) {
      videoFormat = stream->GetMediaFormat().GetName();
      hasRxVideo = stream->IsOpen();
    }

    // Determine if video is startable
    hasStartVideo = connection->GetMediaFormats().HasType(OpalMediaType::Video());
  }

  menubar->Enable(ID_SubMenuAudio, m_activeCall != NULL);
  for (id = ID_AUDIO_CODEC_MENU_BASE; id <= ID_AUDIO_CODEC_MENU_TOP; id++) {
    wxMenuItem * item = menubar->FindItem(id);
    if (item != NULL)
      item->Check(item->GetItemLabelText() == audioFormat);
  }

  menubar->Enable(ID_SubMenuGrabber, hasTxVideo);
  for (id = ID_VIDEO_DEVICE_MENU_BASE; id <= ID_VIDEO_DEVICE_MENU_TOP; id++) {
    wxMenuItem * item = menubar->FindItem(id);
    if (item != NULL)
      item->Check(item->GetItemLabelText() == deviceName);
  }

  menubar->Enable(ID_SubMenuVideo, !videoFormat.IsEmpty() && m_activeCall != NULL);
  for (id = ID_VIDEO_CODEC_MENU_BASE; id <= ID_VIDEO_CODEC_MENU_TOP; id++) {
    wxMenuItem * item = menubar->FindItem(id);
    if (item != NULL)
      item->Check(item->GetItemLabelText() == videoFormat);
  }

  menubar->Enable(ID_MenuStartVideo, hasStartVideo);
  menubar->Enable(ID_MenuStopVideo, hasTxVideo);
  menubar->Enable(ID_MenuSendVFU, hasRxVideo);
  menubar->Enable(ID_MenuSendIntra, hasRxVideo);
  menubar->Enable(ID_MenuTxVideoControl, hasTxVideo);
  menubar->Enable(ID_MenuRxVideoControl, hasRxVideo);
  menubar->Enable(ID_MenuPresentationRole, connection != NULL);
  menubar->Check(ID_MenuPresentationRole, connection != NULL && connection->HasPresentationRole());
  menubar->Enable(ID_MenuDefVidWinPos, hasRxVideo || hasTxVideo);

  menubar->Enable(ID_SubMenuRetrieve, !m_callsOnHold.empty());
  menubar->Enable(ID_SubMenuConference, !m_callsOnHold.empty());

#if !OPAL_HAS_PRESENCE
  menubar->Enable(ID_MenuPresence, false);
#endif // OPAL_HAS_PRESENCE
}


void MyManager::OnMenuQuit(wxCommandEvent & WXUNUSED(event))
{
  Close(true);
}


void MyManager::OnMenuAbout(wxCommandEvent & WXUNUSED(event))
{
  PwxString text;
  PTime compiled(__DATE__);
  text  << PRODUCT_NAME_TEXT " Version " << PProcess::Current().GetVersion() << "\n"
           "\n"
           "Copyright (c) 2007-" << compiled.GetYear() << " " COPYRIGHT_HOLDER ", All rights reserved.\n"
           "\n"
           "This application may be used for any purpose so long as it is not sold "
           "or distributed for profit on it's own, or it's ownership by " COPYRIGHT_HOLDER
           " disguised or hidden in any way.\n"
           "\n"
           "Part of the Open Phone Abstraction Library, http://www.opalvoip.org\n"
           "  OPAL Version: " << OpalGetVersion() << "\n"
           "  PTLib Version: " << PProcess::GetLibVersion() << '\n';
  wxMessageDialog dialog(this, text, wxT("About ..."), wxOK);
  dialog.ShowModal();
}


void MyManager::OnMenuCall(wxCommandEvent & WXUNUSED(event))
{
  CallDialog dlg(this, false, true);
  if (dlg.ShowModal() == wxID_OK)
    MakeCall(dlg.m_Address, dlg.m_UseHandset ? "pots:*" : "pc:*");
}


void MyManager::OnMenuCallLastDialed(wxCommandEvent & WXUNUSED(event))
{
  MakeCall(m_LastDialed);
}


void MyManager::OnMenuCallLastReceived(wxCommandEvent & WXUNUSED(event))
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


#if OPAL_HAS_PRESENCE
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
      wxMessageBox(wxString(wxT("Cannot send IMs to ")) + dlg.m_Address, OpenPhoneErrorString);
  }
}


void MyManager::OnInCallIM(wxCommandEvent & /*event*/)
{
  if (m_activeCall == NULL)
    return;

  if (m_imEP->CreateContext(*m_activeCall) == NULL)
    wxMessageBox(wxT("Cannot send IMs to active call"), OpenPhoneErrorString);
}


void MyManager::OnSendIMSpeedDial(wxCommandEvent & /*event*/)
{
  SpeedDialInfo * info = GetSelectedSpeedDial();
  if (info != NULL && !info->m_Presentity.IsEmpty() && !info->m_Address.IsEmpty()) {
    if (m_imEP->CreateContext(info->m_Presentity, info->m_Address) == NULL)
      wxMessageBox(wxString(wxT("Cannot send IMs to")) + info->m_Address, OpenPhoneErrorString);
  }
}
#endif // OPAL_HAS_IM
#endif // OPAL_HAS_PRESENCE


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


void MyManager::OnMenuAnswer(wxCommandEvent & WXUNUSED(event))
{
  AnswerCall();
}


void MyManager::OnMenuHangUp(wxCommandEvent & WXUNUSED(event))
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


void MyManager::OnNewSpeedDial(wxCommandEvent & WXUNUSED(event))
{
  SpeedDialInfo info;
  info.m_Name = MakeUniqueSpeedDialName(m_speedDials, wxT("New Speed Dial"));
  EditSpeedDial(INT_MAX);
}


void MyManager::OnViewLarge(wxCommandEvent & event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewLarge);
}


void MyManager::OnViewSmall(wxCommandEvent & event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewSmall);
}


void MyManager::OnViewList(wxCommandEvent & event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewList);
}


void MyManager::OnViewDetails(wxCommandEvent & event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewDetails);
}


void MyManager::OnEditSpeedDial(wxCommandEvent & WXUNUSED(event))
{
  EditSpeedDial(m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED));
}


void MyManager::OnCutSpeedDial(wxCommandEvent & event)
{
  OnCopySpeedDial(event);
  OnDeleteSpeedDial(event);
}


void MyManager::OnCopySpeedDial(wxCommandEvent & WXUNUSED(event))
{
  if (!wxTheClipboard->Open())
    return;

  PwxString tabbedText;
  int pos = -1;
  while ((pos = m_speedDials->GetNextItem(pos, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) >= 0) {
    SpeedDialInfo * info = (SpeedDialInfo *)m_speedDials->GetItemData(pos);
    if (PAssertNULL(info) != NULL) {
      tabbedText << info->m_Name << '\t'
                 << info->m_Number << '\t'
                 << info->m_Address << '\t'
                 << info->m_Description << '\t'
#if OPAL_HAS_PRESENCE
                 << info->m_Presentity
#endif
                 << "\r\n";
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


void MyManager::OnPasteSpeedDial(wxCommandEvent & WXUNUSED(event))
{
  wxString data;

  if (wxTheClipboard->Open()) {
    wxTextDataObject myFormatData;
    myFormatData.SetFormat(m_ClipboardFormat);
    if (wxTheClipboard->GetData(myFormatData))
      data = myFormatData.GetText();
    else {
      wxTextDataObject textData;
      if (wxTheClipboard->GetData(textData))
        data = textData.GetText();
    }
    wxTheClipboard->Close();
  }

  if (!data.empty()) {
    wxStringTokenizer tabbedLines(data, wxT("\r\n"));
    while (tabbedLines.HasMoreTokens()) {
      wxStringTokenizer tabbedText(tabbedLines.GetNextToken(), wxT("\t"), wxTOKEN_RET_EMPTY_ALL);
      if (tabbedText.CountTokens() >= 5) {
        SpeedDialInfo info;
        info.m_Name = MakeUniqueSpeedDialName(m_speedDials, tabbedText.GetNextToken().c_str());
        info.m_Number = tabbedText.GetNextToken();
        info.m_Address = tabbedText.GetNextToken();
        info.m_Description = tabbedText.GetNextToken();
#if OPAL_HAS_PRESENCE
        info.m_Presentity = tabbedText.GetNextToken();
#endif

        UpdateSpeedDial(INT_MAX, info, true);
      }
    }
  }
}


void MyManager::OnDeleteSpeedDial(wxCommandEvent & WXUNUSED(event))
{
  int count = m_speedDials->GetSelectedItemCount();
  if (count == 0)
    return;

  PwxString msg;
  msg << "Delete " << count << " item";
  if (count > 1)
    msg << 's';
  msg << '?';
  wxMessageDialog dlg(this, msg, wxT("OpenPhone Speed Dials"), wxYES_NO);
  if (dlg.ShowModal() != wxID_YES)
    return;

  SpeedDialInfo * info;
  while ((info = GetSelectedSpeedDial()) != NULL) {
    wxConfigBase * config = wxConfig::Get();
    config->SetPath(SpeedDialsGroup);
    config->DeleteGroup(info->m_Name);
    m_speedDials->DeleteItem(m_speedDials->FindItem(-1, info->m_Name, false));
    m_speedDialInfo.erase(*info);
  }
}


void MyManager::OnSashPositioned(wxSplitterEvent & event)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);
  config->Write(SashPositionKey, event.GetSashPosition());
  config->Flush();
}


void MyManager::OnSpeedDialActivated(wxListEvent & event)
{
  SpeedDialInfo * info = (SpeedDialInfo *)m_speedDials->GetItemData(event.GetIndex());
  if (info != NULL)
    MakeCall(info->m_Address);
}


void MyManager::OnSpeedDialColumnResize(wxListEvent & event)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);
  wxString key;
  key.sprintf(ColumnWidthKey, event.GetColumn());
  config->Write(key, m_speedDials->GetColumnWidth(event.GetColumn()));
  config->Flush();
}


void MyManager::OnSpeedDialRightClick(wxListEvent & event)
{
  wxMenuBar * menuBar = wxXmlResource::Get()->LoadMenuBar(wxT("SpeedDialMenu"));
  wxMenu * menu = menuBar->Remove(0);
  delete menuBar;

  menu->Enable(ID_CallSpeedDialHandset, HasHandset());
  menu->Enable(ID_SendFaxSpeedDial,     CanDoFax());
#if OPAL_HAS_PRESENCE && OPAL_HAS_IM
  menu->Enable(ID_SendIMSpeedDial,      CanDoIM());
#else
  menu->Enable(ID_SendIMSpeedDial, false);
#endif
  PopupMenu(menu, event.GetPoint());
  delete menu;
}


void MyManager::OnSpeedDialEndEdit(wxListEvent & theEvent)
{
  if (theEvent.IsEditCancelled())
    return;

  int index = theEvent.GetIndex();
  SpeedDialInfo * oldInfo = (SpeedDialInfo *)m_speedDials->GetItemData(index);
  if (oldInfo == NULL)
    return;

  if (oldInfo->m_Name.CmpNoCase(theEvent.GetLabel()) == 0)
    return;

  if (HasSpeedDialName(theEvent.GetLabel())) {
    theEvent.Veto();
    return;
  }

  SpeedDialInfo newInfo(*oldInfo);
  newInfo.m_Name = theEvent.GetLabel();
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

#if OPAL_HAS_PRESENCE
  if (info->m_Presentity != dlg.m_Presentity) {
    if (!info->m_Presentity.empty())
      MonitorPresence(info->m_Presentity, dlg.m_Address, false);

    if (!dlg.m_Presentity.empty())
      MonitorPresence(dlg.m_Presentity, dlg.m_Address, true);
  }
#endif // OPAL_HAS_PRESENCE

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
#if OPAL_HAS_PRESENCE
    config->Write(SpeedDialPresentityKey, newInfo.m_Presentity);
#endif
    config->Write(SpeedDialDescriptionKey, newInfo.m_Description);
    config->Flush();
  }

  m_speedDials->SetItemImage(index, Icon_Unknown);

  if (m_speedDialDetail) {
    m_speedDials->SetItem(index, e_NumberColumn, newInfo.m_Number);
    m_speedDials->SetItem(index, e_StatusColumn, IconStatusNames[Icon_Unknown]);
    m_speedDials->SetItem(index, e_AddressColumn, newInfo.m_Address);
#if OPAL_HAS_PRESENCE
    m_speedDials->SetItem(index, e_PresentityColumn, newInfo.m_Presentity);
#endif
    m_speedDials->SetItem(index, e_DescriptionColumn, newInfo.m_Description);
  }

  *oldInfo = newInfo;
  return true;
}


bool MyManager::HasSpeedDialName(const wxString & name) const
{
  return m_speedDials->FindItem(-1, name) >= 0;
}


bool MyManager::HasSpeedDialNumber(const wxString & number, const wxString & ignoredNumber) const
{
  int count = m_speedDials->GetItemCount();
  for (int index = 0; index < count; index++) {
    SpeedDialInfo * info = (SpeedDialInfo *)m_speedDials->GetItemData(index);
    if (info != NULL && info->m_Number == number && info->m_Number != ignoredNumber)
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

  if (m_activeCall != NULL)
    m_activeCall->Hold();

  if (m_primaryVideoGrabber != NULL)
    m_primaryVideoGrabber->Start();
  else
    m_primaryVideoGrabber = PVideoInputDevice::CreateOpenedDevice(GetVideoInputDevice(), true);

  m_activeCall = SetUpCall(from, address, NULL, 0, options);
  if (m_activeCall == NULL)
    LogWindow << "Could not call \"" << address << '"' << endl;
  else {
    LogWindow << "Calling \"" << address << '"' << endl;
    m_tabs->AddPage(new CallingPanel(*this, m_activeCall, m_tabs), wxT("Calling"), true);
  }
}


bool MyManager::AnswerCall()
{
  if (PAssert(!m_incomingToken.IsEmpty(), PLogicError)) {
    StopRingSound();

    if (m_activeCall != NULL)
      m_activeCall->Hold();

    PString token = m_incomingToken.p_str();
    m_incomingToken.clear();

    return pcssEP->AcceptIncomingConnection(token);
  }

  return false;
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
  PSafePtr<OpalCall> call = FindCallWithLock(m_incomingToken, PSafeReference);
  if (call == NULL)
    return; // Call disappeared before we could get to it

  PSafePtr<OpalConnection> connection = call->GetConnection(0, PSafeReadOnly);
  if (!PAssert(connection != NULL, PLogicError))
    return;

  PString alertingType;
  PSafePtr<OpalConnection> network = connection->GetOtherPartyConnection();
  if (network != NULL)
    alertingType = network->GetAlertingType();

  PTime now;
  LogWindow << "\nIncoming call at " << now.AsString("w h:mma");

  PString from = connection->GetRemotePartyName();
  PwxString balloon;
  balloon << "Incoming call";
  if (!from.IsEmpty()) {
    balloon << " from \"" << from << '"';
    LogWindow << " from " << from;
  }

  if (!alertingType.IsEmpty())
    LogWindow << ", type=" << alertingType;

  LogWindow << endl;
  SetBalloonText(balloon);

  m_LastReceived = connection->GetRemotePartyURL();
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

  if (m_primaryVideoGrabber == NULL)
    m_primaryVideoGrabber = PVideoInputDevice::CreateOpenedDevice(GetVideoInputDevice(), true);

  if ((m_autoAnswer && m_activeCall == NULL) || connection->GetStringOptions().Contains("Auto-Answer")) {
#if OPAL_FAX
    m_currentAnswerMode = m_defaultAnswerMode;
#endif
    m_tabs->AddPage(new CallingPanel(*this, call, m_tabs), wxT("Answering"), true);

    pcssEP->AcceptIncomingConnection(m_incomingToken);
    m_incomingToken.clear();
  }
  else {
    AnswerPanel * answerPanel = new AnswerPanel(*this, call, m_tabs);

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


void MyManager::OnRingSoundAgain(PTimer &, P_INT_PTR)
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
  if (usingHandset) {
    LogWindow << "Line interface device \"" << connection.GetRemotePartyName() << "\" has gone off hook." << endl;
    SetTrayTipText("Off hook");
  }

  if (!OpalManager::OnIncomingConnection(connection, options, stringOptions))
    return false;

  if (usingHandset) {
    m_activeCall = &connection.GetCall();
    PostEvent(wxEvtRinging, m_activeCall->GetToken());
  }

  return true;
}


void MyManager::OnEstablishedCall(OpalCall & call)
{
  m_activeCall = &call;

  LogWindow << "Established call from " << call.GetPartyA() << " to " << call.GetPartyB() << endl;
  PostEvent(wxEvtEstablished, call.GetToken());

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
  SetBalloonText(PString::Empty());

  PwxString token(theEvent.GetString());

  if (m_activeCall == NULL) {
    // Retrieve call from hold
    RemoveCallOnHold(token);
    m_activeCall = FindCallWithLock(token, PSafeReference);
    return;
  }

  size_t tabIndex = 0;
  for (tabIndex = 0; tabIndex < m_tabs->GetPageCount(); ++tabIndex) {
    CallPanelBase * panel = dynamic_cast<CallPanelBase *>(m_tabs->GetPage(tabIndex));
    if (panel != NULL && panel->GetToken() == token) {
      if (dynamic_cast<InCallPanel *>(panel) == NULL)
        break;

      RemoveCallOnHold(token);
      return;
    }
  }

  // Because this can take some time, construct it before we delete the previous
  // page to avoid ugly scenes on the screen
  InCallPanel * inCallPanel = new InCallPanel(*this, m_activeCall, m_tabs);

  if (tabIndex < m_tabs->GetPageCount())
    m_tabs->DeletePage(tabIndex);

  PwxString title = m_activeCall->IsNetworkOriginated() ? m_activeCall->GetPartyA() : m_activeCall->GetPartyB();
  m_tabs->AddPage(inCallPanel, title, true);
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

  PostEvent(wxEvtCleared, call.GetToken());

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
  SetBalloonText(PString::Empty());

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

  delete m_primaryVideoGrabber;
  m_primaryVideoGrabber = NULL;

  m_activeCall.SetNULL();
}


#if OPAL_PTLIB_SSL
bool MyManager::ApplySSLCredentials(const OpalEndPoint & ep, PSSLContext & context, bool create) const
{
  context.SetPasswordNotifier(PCREATE_NOTIFIER_EXT(const_cast<MyManager *>(this), MyManager, OnSSLPassword));
  return OpalManager::ApplySSLCredentials(ep, context, create);
}


void MyManager::OnSSLPassword(PString & password, bool)
{
  if (PThread::Current() != &PProcess::Current())
    PostEvent(wxEvtGetSSLPassword);
  else {
    wxCommandEvent dummy;
    OnEvtGetSSLPassword(dummy);
  }

  m_gotSSLPassword.Wait();
  password = m_SSLPassword.p_str();
}


void MyManager::OnEvtGetSSLPassword(wxCommandEvent & /*event*/)
{
  m_SSLPassword.erase();

  wxPasswordEntryDialog dlg(this, wxT("Enter certificate private key password"), wxT("Open Phone"));
  if (dlg.ShowModal() == wxID_OK)
    m_SSLPassword = dlg.GetValue();

  m_gotSSLPassword.Signal();
}
#endif // OPAL_PTLIB_SSL


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

  PostEvent(onHold ? wxEvtOnHold : wxEvtEstablished, connection.GetCall().GetToken());
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


void MyManager::OnEvtOnHold(wxCommandEvent & theEvent)
{
  PSafePtr<OpalCall> call = FindCallWithLock(PwxString(theEvent.GetString()).p_str(), PSafeReadOnly);

  if (call != NULL) {
    AddCallOnHold(*call);
    if (call == m_activeCall)
      m_activeCall.SetNULL();
  }
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


void MyManager::OnStartMediaPatch(OpalConnection & connection, OpalMediaPatch & patch)
{
  OpalManager::OnStartMediaPatch(connection, patch);

  if (connection.IsNetworkConnection()) {
    OpalMediaStreamPtr stream = &patch.GetSource().GetConnection() == &connection
                                    ? OpalMediaStreamPtr(&patch.GetSource()) : patch.GetSink();
    if (stream != NULL)
      stream->PrintDetail(LogWindow, "Started", OpalMediaStream::DetailNAT |
                                                OpalMediaStream::DetailSecured |
                                                OpalMediaStream::DetailFEC |
                                                OpalMediaStream::DetailAudio |
                                                OpalMediaStream::DetailEOL);
  }
  else if (!m_VideoWatermarkDevice.IsEmpty()) {
    OpalVideoMediaStream * videoStream = dynamic_cast<OpalVideoMediaStream *>(&patch.GetSource());
    if (videoStream != NULL) {
      PVideoInputDevice * device = PVideoInputDevice::CreateOpenedDevice(m_VideoWatermarkDevice, false);
      if (device != NULL)
        videoStream->SetVideoWatermarkDevice(device);
    }
  }

  PostEvent(wxEvtStreamsChanged, connection.GetCall().GetToken());
}


void MyManager::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalManager::OnClosedMediaStream(stream);

  if (stream.GetConnection().IsNetworkConnection())
    stream.PrintDetail(LogWindow, "Stopped", OpalMediaStream::DetailMinimum | OpalMediaStream::DetailEOL);
  PostEvent(wxEvtStreamsChanged);

  const OpalVideoMediaStream * videoStream = dynamic_cast<const OpalVideoMediaStream *>(&stream);
  if (videoStream == NULL)
    return;

  PVideoOutputDevice * device = videoStream->GetVideoOutputDevice();
  VideoWindowInfo newWindowInfo(device);
  device->Close();

  if (newWindowInfo.x == INT_MIN)
    return;

  bool isPreview = stream.IsSource();
  OpalVideoFormat::ContentRole role = stream.GetMediaFormat().GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole);
  if (m_videoWindowInfo[isPreview][role] == newWindowInfo)
    return;

  m_videoWindowInfo[isPreview][role] = newWindowInfo;

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(VideoGroup);
  newWindowInfo.WriteConfig(config, VideoWindowKeyBase[isPreview][role]);
  config->Flush();
}


PSafePtr<OpalCall> MyManager::GetCall(PSafetyMode mode)
{
  if (m_activeCall == NULL)
    return NULL;

  PSafePtr<OpalCall> call = m_activeCall;
  return call.SetSafetyMode(mode) ? call : NULL;
}


PSafePtr<OpalConnection> MyManager::GetNetworkConnection(PSafetyMode mode)
{
  if (m_activeCall == NULL)
    return NULL;

  PSafePtr<OpalConnection> connection = m_activeCall->GetConnection(0, PSafeReference);
  while (connection != NULL && !connection->IsNetworkConnection())
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
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(ID_SubMenuRetrieve);
  wxMenu * menu = PAssertNULL(item)->GetSubMenu();
  PAssertNULL(menu)->Append(m_callsOnHold.back().m_retrieveMenuId, otherParty);
  item = menu->FindItemByPosition(0);
  if (item->IsSeparator())
    menu->Delete(item);

  item = menubar->FindItem(ID_SubMenuConference);
  menu = PAssertNULL(item)->GetSubMenu();
  PAssertNULL(menu)->Append(m_callsOnHold.back().m_conferenceMenuId, otherParty);
  item = menu->FindItemByPosition(0);
  if (item->IsSeparator())
    menu->Delete(item);

  item = menubar->FindItem(ID_SubMenuTransfer);
  menu = PAssertNULL(item)->GetSubMenu();
  PAssertNULL(menu)->Append(m_callsOnHold.back().m_transferMenuId, otherParty);

  OnHoldChanged(call.GetToken(), true);

  if (!m_switchHoldToken.IsEmpty()) {
    PSafePtr<OpalCall> heldCall = FindCallWithLock(m_switchHoldToken, PSafeReference);
    if (heldCall != NULL)
      heldCall->Retrieve();
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
  PSafePtr<OpalLocalConnection> connection;
  if (GetConnection(connection, PSafeReadWrite))
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
  if (toupper(tone) == 'X' && m_currentAnswerMode == AnswerDetect && duration > 0)
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


void MyManager::OnRequestHold(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalCall> call = GetCall(PSafeReference);
  if (call != NULL)
    call->Hold();
}


void MyManager::OnRetrieve(wxCommandEvent & theEvent)
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


void MyManager::OnConference(wxCommandEvent & theEvent)
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
    if (!SetUpConference(*m_activeCall)) {
      LogWindow << "Could not conference \"" << m_activeCall->GetRemoteParty() << "\"." << endl;
      return;
    }
    LogWindow << "Added \"" << m_activeCall->GetRemoteParty() << "\" to conference." << endl;
    m_activeCall.SetNULL();
  }

  if (SetUpConference(call))
    LogWindow << "Added \"" << call.GetRemoteParty() << "\" to conference." << endl;
  else
    LogWindow << "Could not conference \"" << call.GetRemoteParty() << "\"." << endl;
}


void MyManager::OnTransfer(wxCommandEvent & theEvent)
{
  if (PAssert(m_activeCall != NULL, PLogicError)) {
    for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
      if (theEvent.GetId() == it->m_transferMenuId) {
        m_activeCall->Transfer(it->m_call->GetToken(), GetNetworkConnection(PSafeReference));
        return;
      }
    }

    CallDialog dlg(this, true, true);
    dlg.SetTitle(wxT("Transfer Call"));
    if (dlg.ShowModal() == wxID_OK)
      m_activeCall->Transfer(dlg.m_Address, GetNetworkConnection(PSafeReference));
  }
}


void MyManager::OnStartRecording(wxCommandEvent & /*event*/)
{
  wxString wildcard;
  OpalRecordManager::Factory::KeyList_T keyList = OpalRecordManager::Factory::GetKeyList();
  for (OpalRecordManager::Factory::KeyList_T::iterator it = keyList.begin(); it != keyList.end(); ++it) {
    if (it != keyList.begin())
      wildcard << '|';
    PwxString ext = it->Mid(1);
    wildcard << ext.Upper() << " Files (*." << ext << ")|*." << ext;
  }
  wxFileDialog dlg(this,
                   wxT("Save call to file"),
                   wxEmptyString,
                   m_lastRecordFile,
                   wildcard,
                   wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() == wxID_OK && m_activeCall != NULL) {
    m_lastRecordFile = dlg.GetPath();
    if (!m_activeCall->StartRecording(m_lastRecordFile, m_recordingOptions))
      wxMessageBox(wxT("Cannot record to ")+m_lastRecordFile,
                   OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);
  }
}


void MyManager::OnStopRecording(wxCommandEvent & /*event*/)
{
  if (m_activeCall != NULL)
    m_activeCall->StopRecording();
}


void MyManager::OnSendAudioFile(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalPCSSConnection> connection;
  if (!GetConnection(connection, PSafeReference))
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
                   OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);
  }
}


void MyManager::OnAudioDevicePair(wxCommandEvent & /*theEvent*/)
{
  PSafePtr<OpalPCSSConnection> connection;
  if (GetConnection(connection, PSafeReference)) {
    AudioDevicesDialog dlg(this, *connection);
    if (dlg.ShowModal() == wxID_OK)
      m_activeCall->Transfer(dlg.GetTransferAddress(), connection);
  }
}


void MyManager::OnAudioDeviceChange(wxCommandEvent & theEvent)
{
  m_activeCall->Transfer("pc:"+MediaDeviceNameFromScreen(GetMenuBar()->FindItem(theEvent.GetId())->GetItemLabelText()));
}


void MyManager::OnVideoDeviceChange(wxCommandEvent & theEvent)
{
  PwxString deviceName = GetMenuBar()->FindItem(theEvent.GetId())->GetItemLabelText();
  if (deviceName[0] == '*') {
    deviceName = wxFileSelector(wxT("Select Video File"),
                                wxEmptyString,
                                wxEmptyString,
                                deviceName.Mid(1),
                                deviceName,
                                wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (deviceName.empty())
      return;
  }

  PSafePtr<OpalLocalConnection> local;
  if (GetConnection(local, PSafeReadWrite)) {
    PVideoDevice::OpenArgs args = GetVideoInputDevice();
    args.deviceName = deviceName.p_str();
    if (!local->ChangeVideoInputDevice(args))
      wxMessageBox(wxT("Cannot switch to video input device ")+deviceName,
                    OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);
  }
}


void MyManager::OnNewCodec(wxCommandEvent & theEvent)
{
  OpalMediaFormat mediaFormat(PwxString(GetMenuBar()->FindItem(theEvent.GetId())->GetItemLabelText()).p_str());
  if (mediaFormat.IsValid()) {
    PSafePtr<OpalLocalConnection> connection;
    if (GetConnection(connection, PSafeReadWrite)) {
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
  PSafePtr<OpalLocalConnection> connection;
  if (!GetConnection(connection, PSafeReadWrite))
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
    PVideoDevice::OpenArgs args = GetVideoInputDevice();
    StartVideoDialog dlg(this, false);
    dlg.m_device = args.deviceName;
    dlg.m_preview = m_VideoGrabPreview;
    if (dlg.ShowModal() != wxID_OK)
      return;

    args.deviceName = dlg.m_device.p_str();
    SetVideoInputDevice(args);
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
  PSafePtr<OpalLocalConnection> connection;
  if (GetConnection(connection, PSafeReadWrite)) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream != NULL)
      stream->Close();
  }
}


void MyManager::OnSendVFU(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalLocalConnection> connection;
  if (GetConnection(connection, PSafeReadOnly)) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), false);
    if (stream != NULL)
      stream->ExecuteCommand(OpalVideoUpdatePicture());
  }
}


void MyManager::OnSendIntra(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalLocalConnection> connection;
  if (GetConnection(connection, PSafeReadOnly)) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream != NULL)
      stream->ExecuteCommand(OpalVideoUpdatePicture());
  }
}


void MyManager::OnTxVideoControl(wxCommandEvent & /*event*/)
{
  VideoControlDialog dlg(this, false);
  dlg.ShowModal();
}


void MyManager::OnRxVideoControl(wxCommandEvent & /*event*/)
{
  VideoControlDialog dlg(this, true);
  dlg.ShowModal();
}


void MyManager::OnMenuPresentationRole(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalConnection> conn = GetNetworkConnection(PSafeReadOnly);
  if (conn != NULL && !conn->RequestPresentationRole(conn->HasPresentationRole()))
    LogWindow << "Remote does not support presentation role request" << endl;
}


bool MyManager::OnChangedPresentationRole(OpalConnection & connection,
                                           const PString & newChairURI,
                                           bool request)
{
  LogWindow << "Presentation role token now owned by ";
  if (newChairURI.IsEmpty())
    LogWindow << "nobody";
  else if (newChairURI == connection.GetLocalPartyURL())
    LogWindow << "local user";
  else
    LogWindow << newChairURI;
  LogWindow << endl;

  return OpalManager::OnChangedPresentationRole(connection, newChairURI, request);
}


void MyManager::OnDefVidWinPos(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalLocalConnection> connection;
  if (!GetConnection(connection, PSafeReadOnly))
    return;

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(VideoGroup);

  wxRect rect(GetPosition(), GetSize());

  int x = rect.GetLeft();
  for (OpalVideoFormat::ContentRole role = OpalVideoFormat::BeginContentRole; role < OpalVideoFormat::EndContentRole; ++role) {
    int y = rect.GetBottom();
    for (int preview = 0; preview < 2; ++preview) {
      m_videoWindowInfo[preview][role].x = x;
      m_videoWindowInfo[preview][role].y = y;
      m_videoWindowInfo[preview][role].WriteConfig(config, VideoWindowKeyBase[preview][role]);

      PSafePtr<OpalVideoMediaStream> stream = ::PSafePtrCast<OpalMediaStream, OpalVideoMediaStream>(connection->GetMediaStream(OpalMediaType::Video(), preview));
      if (stream != NULL && stream->GetMediaFormat().GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole) == role) {
        PVideoOutputDevice * device = stream->GetVideoOutputDevice();
        if (device != NULL)
          device->SetPosition(x, y);
      }
      x += 352;
    }
  }

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
  if (!m_SecondaryVideoOpening)
    return OpalManager::CreateVideoInputDevice(connection, mediaFormat, device, autoDelete);

  mediaFormat.AdjustVideoArgs(m_SecondaryVideoGrabber);

  autoDelete = true;
  device = PVideoInputDevice::CreateOpenedDevice(m_SecondaryVideoGrabber, false);
  PTRACE_IF(2, device == NULL, "OpenPhone\tCould not open secondary video device \"" << m_SecondaryVideoGrabber.deviceName << '"');

  m_SecondaryVideoOpening = false;

  return device != NULL;
}


bool MyManager::CreateVideoInputDevice(const OpalConnection & connection,
                                       const PVideoDevice::OpenArgs & args,
                                       PVideoInputDevice * & device,
                                       PBoolean & autoDelete)
{
  if (m_primaryVideoGrabber == NULL || m_primaryVideoGrabber->GetDeviceName() != args.deviceName)
    return OpalManager::CreateVideoInputDevice(connection, args, device, autoDelete);

  device = m_primaryVideoGrabber;
  autoDelete = false;
  return true;
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

  OpalVideoFormat::ContentRole role = mediaFormat.GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole);

  static PConstString const PreviewWindowTitle("Preview");
  PString title = preview ? PreviewWindowTitle : connection.GetRemotePartyName();
  if (openChannelCount > 0)
    title.sprintf(" (%u)", openChannelCount);

  PVideoDevice::OpenArgs videoArgs = preview ? GetVideoPreviewDevice() : GetVideoOutputDevice();
  videoArgs.channelNumber = m_videoWindowInfo[preview][role].size;
  videoArgs.deviceName = psprintf(VIDEO_WINDOW_DEVICE" TITLE=\"%s\" X=%i Y=%i", (const char *)title,
                                  m_videoWindowInfo[preview][role].x, m_videoWindowInfo[preview][role].y);

  mediaFormat.AdjustVideoArgs(videoArgs);

#if VIDEO_WINDOW_AUTO_DELETE
  autoDelete = true;
#else
  autoDelete = false;
#endif
  device = PVideoOutputDevice::CreateOpenedDevice(videoArgs, false);
  return device != NULL;
}


void MyManager::OnForwardingTimeout(PTimer &, P_INT_PTR)
{
  if (m_incomingToken.IsEmpty())
    return;

  // Transfer the incoming call to the forwarding address
  PSafePtr<OpalCall> call = FindCallWithLock(m_incomingToken, PSafeReference);
  if (call == NULL)
    return;

  PSafePtr<OpalConnection> network = call->GetConnection(0, PSafeReference);
  PString remote = network->GetRemotePartyURL();
  PString forward = m_ForwardingAddress;
  if (forward[0] == '*')
    forward.Splice(remote.Left(remote.Find(':')), 0, 1);

  if (call->Transfer(forward, network))
    LogWindow << "Forwarded \"" << remote << "\" to \"" << forward << '"' << endl;
  else {
    LogWindow << "Could not forward \"" << remote << "\" to \"" << forward << '"' << endl;
    call->Clear();
  }

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
    if (h323EP->UseGatekeeper(m_gatekeeperAddress, m_gatekeeperIdentifier, m_gatekeeperInterface)) {
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

  bool gotOne = false;

  for (RegistrationList::iterator iter = m_registrations.begin(); iter != m_registrations.end(); ++iter) {
    if (iter->Start(*sipEP))
      gotOne = true;
  }

  if (gotOne)
    SetTrayTipText("Registering");
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


#if OPAL_HAS_PRESENCE
bool MyManager::MonitorPresence(const PString & aor, const PString & uri, bool start)
{
  if (aor.IsEmpty() || uri.IsEmpty())
    return false;

  PSafePtr<OpalPresentity> presentity = GetPresentity(aor);
  if (presentity == NULL || !presentity->IsOpen()) {
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


void MyManager::OnPresenceChange(OpalPresentity &, std::auto_ptr<OpalPresenceInfo> info)
{
  int count = m_speedDials->GetItemCount();
  for (int index = 0; index < count; index++) {
    SpeedDialInfo * sdInfo = (SpeedDialInfo *)m_speedDials->GetItemData(index);

    SIPURL speedDialURL(sdInfo->m_Address.p_str());
    if ( info->m_entity == speedDialURL ||
        (info->m_entity.GetScheme() == "pres" &&
         info->m_entity.GetUserName() == speedDialURL.GetUserName() &&
         info->m_entity.GetHostName() == speedDialURL.GetHostName())) {
      PwxString status = info->m_note;

      wxListItem item;
      item.m_itemId = index;
      item.m_mask = wxLIST_MASK_IMAGE;
      if (!m_speedDials->GetItem(item))
        continue;

      IconStates oldIcon = (IconStates)item.m_image;

      IconStates newIcon;
      switch (info->m_state) {
        case OpalPresenceInfo::Unchanged :
          newIcon = oldIcon;
          break;

        case OpalPresenceInfo::NoPresence :
          newIcon = Icon_Unavailable;
          break;

        case OpalPresenceInfo::Unavailable :
          newIcon = Icon_Away;
          break;

        default :
          if (info->m_activities.Contains("busy") || status.CmpNoCase(wxT("busy")) == 0)
            newIcon = Icon_Busy;
          else if (info->m_activities.Contains("away") || status.CmpNoCase(wxT("away")) == 0)
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
        LogWindow << "Presence notification received for " << info->m_entity << ", \"" << status << '"' << endl;
      }
      break;
    }
  }
}
#endif // OPAL_HAS_PRESENCE


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


void MyManager::UpdateAudioVideoDevices()
{
  wxMenuBar * menubar = GetMenuBar();
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(ID_SubMenuSound);
  wxMenu * subMenu = PAssertNULL(item)->GetSubMenu();
  while (subMenu->GetMenuItemCount() > 2)
    subMenu->Delete(subMenu->FindItemByPosition(2));

  unsigned id = ID_AUDIO_DEVICE_MENU_BASE;
  PStringList playDevices = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
  PStringList recordDevices = PSoundChannel::GetDeviceNames(PSoundChannel::Recorder);
  for (PINDEX i = 0; i < playDevices.GetSize(); i++) {
    if (recordDevices.GetValuesIndex(playDevices[i]) != P_MAX_INDEX)
      subMenu->Append(id++, MediaDeviceNameToScreen(playDevices[i]), wxEmptyString, true);
  }

  item = PAssertNULL(menubar)->FindItem(ID_SubMenuGrabber);
  subMenu = PAssertNULL(item)->GetSubMenu();
  while (subMenu->GetMenuItemCount() > 0)
    subMenu->Delete(subMenu->FindItemByPosition(0));

  id = ID_VIDEO_DEVICE_MENU_BASE;
  PStringArray grabDevices = PVideoInputDevice::GetDriversDeviceNames("*\t*");
  for (PINDEX i = 0; i < grabDevices.GetSize(); i++)
    subMenu->Append(id++, MediaDeviceNameToScreen(grabDevices[i]), wxEmptyString, true);
}


void MyManager::ApplyMediaInfo()
{
  PStringList mediaFormatOrder, mediaFormatMask;

  m_mediaInfo.sort();

  wxMenuBar * menubar = GetMenuBar();
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(ID_SubMenuAudio);
  wxMenu * audioMenu = PAssertNULL(item)->GetSubMenu();
  while (audioMenu->GetMenuItemCount() > 0)
    audioMenu->Delete(audioMenu->FindItemByPosition(0));

  item = PAssertNULL(menubar)->FindItem(ID_SubMenuVideo);
  wxMenu * videoMenu = PAssertNULL(item)->GetSubMenu();
  while (videoMenu->GetMenuItemCount() > 0)
    videoMenu->Delete(videoMenu->FindItemByPosition(0));

  for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
    if (mm->preferenceOrder < 0)
      mediaFormatMask.AppendString(mm->mediaFormat);
    else {
      mediaFormatOrder.AppendString(mm->mediaFormat);
      PwxString formatName(mm->mediaFormat.GetName());
      if (mm->mediaFormat.GetMediaType() == OpalMediaType::Audio())
        audioMenu->Append(mm->preferenceOrder+ID_AUDIO_CODEC_MENU_BASE, formatName, wxEmptyString, true);
      else if (mm->mediaFormat.GetMediaType() == OpalMediaType::Video())
        videoMenu->Append(mm->preferenceOrder+ID_VIDEO_CODEC_MENU_BASE, formatName, wxEmptyString, true);
    }
  }

  if (!mediaFormatOrder.IsEmpty()) {
    PTRACE(3, "OpenPhone\tMedia order:\n"<< setfill('\n') << mediaFormatOrder << setfill(' '));
    SetMediaFormatOrder(mediaFormatOrder);
    PTRACE(3, "OpenPhone\tMedia mask:\n"<< setfill('\n') << mediaFormatMask << setfill(' '));
    SetMediaFormatMask(mediaFormatMask);
  }
}


void MyManager::OnSetTrayTipText(wxCommandEvent & theEvent)
{
  SetTrayTipText(theEvent.GetString());
}


void MyManager::SetTrayTipText(const PwxString & tip)
{
  if (PThread::Current() != &PProcess::Current())
    PostEvent(wxEvtSetTrayTipText, tip);
  else if (m_taskBarIcon->IsIconInstalled()) {
    PwxString text;
    text << PProcess::Current().GetName() << " - " << tip;
    m_taskBarIcon->SetIcon(wxICON(AppIcon), text);
  }
}


#ifdef __WXMSW__
void MyManager::SetBalloonText(const PwxString & text)
{
#if wxUSE_TASKBARICON_BALLOONS
  if (m_taskBarIcon->IsIconInstalled())
    m_taskBarIcon->ShowBalloon(PwxString(PProcess::Current().GetName()), text);
#else
  NOTIFYICONDATA notify;
  memset(&notify, 0, sizeof(notify));
  notify.cbSize = sizeof(NOTIFYICONDATA);
  notify.hWnd = (HWND)GetHWND();
  notify.uID = 99;
#ifdef NIF_REALTIME
  notify.uFlags = NIF_INFO|NIF_REALTIME;
#else
  notify.uFlags = NIF_INFO;
#endif
  notify.dwInfoFlags = NIIF_NOSOUND;
  notify.uVersion = NOTIFYICON_VERSION;
  wxStrncpy(notify.szInfoTitle, PwxString(PProcess::Current().GetName()), WXSIZEOF(notify.szInfoTitle));
  wxStrncpy(notify.szInfo, text, WXSIZEOF(notify.szInfo));
  Shell_NotifyIcon(NIM_MODIFY, &notify);
#endif
#else
void MyManager::SetBalloonText(const PwxString &)
{
#endif
}


wxMenu * MyManager::CreateTrayMenu()
{
  wxMenuBar * menubar = wxXmlResource::Get()->LoadMenuBar(wxT("TrayMenu"));
  wxMenu * menu = menubar->Remove(0);
  delete menubar;

  menu->Enable(ID_MenuCall,             m_activeCall == NULL);
  menu->Enable(ID_MenuCallLastDialed,   m_activeCall == NULL && !m_LastDialed.IsEmpty());
  menu->Enable(ID_MenuCallLastReceived, m_activeCall == NULL && !m_LastReceived.IsEmpty());
  menu->Enable(ID_MenuAnswer,          !m_incomingToken.IsEmpty());
  menu->Enable(ID_MenuHangUp,           m_activeCall != NULL);
  menu->Enable(ID_MenuHold,             m_activeCall != NULL);
  menu->Enable(ID_MenuTransfer,         m_activeCall != NULL);
  menu->Enable(ID_MenuStartRecording,   m_activeCall != NULL && !m_activeCall->IsRecording());
  menu->Enable(ID_MenuStopRecording,    m_activeCall != NULL &&  m_activeCall->IsRecording());
  menu->Enable(ID_SubMenuRetrieve,     !m_callsOnHold.empty());
  menu->Enable(ID_SubMenuTransfer,     !m_callsOnHold.empty());

  wxMenuItem * item = menu->FindItem(ID_SubMenuRetrieve);
  wxMenu * retrieveSubmenu = PAssertNULL(item)->GetSubMenu();
  item = menu->FindItem(ID_SubMenuTransfer);
  wxMenu * transferSubmenu = PAssertNULL(item)->GetSubMenu();

  for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
    PwxString otherParty = it->m_call->GetPartyA();

    retrieveSubmenu->Append(m_callsOnHold.back().m_retrieveMenuId, otherParty);
    menu->Enable(it->m_retrieveMenuId, m_activeCall == NULL);

    transferSubmenu->Append(m_callsOnHold.back().m_transferMenuId, otherParty);
    menu->Enable(it->m_transferMenuId, m_activeCall != NULL);
  }

  item = menu->FindItem(ID_SubMenuSpeedDial);
  wxMenu * speedDialSubmenu = PAssertNULL(item)->GetSubMenu();
  int menuID = ID_SPEEDDIAL_MENU_BASE;
  for (set<SpeedDialInfo>::iterator it = m_speedDialInfo.begin(); it != m_speedDialInfo.end(); ++it)
    speedDialSubmenu->Append(menuID++, it->m_Name);

  return menu;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyTaskBarIcon, wxTaskBarIcon)
  EVT_TASKBAR_LEFT_DCLICK(MyTaskBarIcon::OnDoubleClick)
END_EVENT_TABLE()


MyTaskBarIcon::MyTaskBarIcon(MyManager & manager)
  : m_manager(manager)
{
}


wxMenu * MyTaskBarIcon::CreatePopupMenu()
{
  return m_manager.CreateTrayMenu();
}


void MyTaskBarIcon::OnDoubleClick(wxTaskBarIconEvent &)
{
  m_manager.Show();
  if (m_manager.IsIconized())
    m_manager.Iconize(false);
  m_manager.Raise();
}


bool MyTaskBarIcon::ProcessEvent(wxEvent & evt)
{
#if wxCHECK_VERSION(2,9,2)
  return wxTaskBarIcon::ProcessEvent(evt);
#else
  // This passes menu commands on to the main frame window.
  return wxTaskBarIcon::ProcessEvent(evt) || m_manager.ProcessEvent(evt);
#endif
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
  config.Read(RegistrarInstanceIdKey, &m_InstanceId);
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
  config.Write(RegistrarInstanceIdKey, m_InstanceId);
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

    if (m_Compatibility == SIPRegister::e_RFC5626 && m_InstanceId.empty())
      m_InstanceId = PGloballyUniqueID().AsString();

    SIPRegister::Params param;
    param.m_addressOfRecord = m_User.p_str();
    param.m_registrarAddress = m_Domain.p_str();
    param.m_contactAddress = m_Contact.p_str();
    param.m_instanceId = m_InstanceId.p_str();
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
                 OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);
    return false;
  }
};


///////////////////////////////////////////////////////////////////////////////

class wxTerminalTypeValidator: public wxGenericValidator
{
public:
  wxTerminalTypeValidator(wxString* val)
    : wxGenericValidator(val)
  {
  }

  virtual wxObject *Clone() const
  {
    return new wxTerminalTypeValidator(*this);
  }

  virtual bool Validate(wxWindow *)
  {
    wxComboBox *control = (wxComboBox *) m_validatorWindow;

    unsigned long terminalType;
    control->GetValue().ToULong(&terminalType);
    if (terminalType > 0 && terminalType < 256)
      return true;

    wxMessageBox(wxT("Illegal value \"") + control->GetValue() + wxT("\" for H.323 terminal type."),
                 OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);
    return false;
  }
};


///////////////////////////////////////////////////////////////////////////////

void MyManager::OnOptions(wxCommandEvent & /*event*/)
{
  PTRACE(4, "OpenPhone\tOpening options dialog");
  OptionsDialog dlg(this);
  dlg.ShowModal();
}

DEF_XRCID(AllCodecs);
DEF_XRCID(SelectedCodecs);
DEF_XRCID(CodecOptionsList);
DEF_XRCID(CodecOptionValue);
DEF_XRCID(SetDefaultCodecOption);
DEF_XRCID(AddCodec);
DEF_XRCID(RemoveCodec);
DEF_XRCID(MoveUpCodec);
DEF_XRCID(MoveDownCodec);

BEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
  ////////////////////////////////////////
  // General fields
  EVT_BUTTON(XRCID("BrowseSoundFile"), OptionsDialog::BrowseSoundFile)
  EVT_BUTTON(XRCID("PlaySoundFile"), OptionsDialog::PlaySoundFile)
  EVT_CHECKBOX(wxXmlResource::GetXRCID(OverrideProductInfoKey), OptionsDialog::OnOverrideProductInfo)

  ////////////////////////////////////////
  // Networking fields
  EVT_CHOICE(XRCID("BandwidthClass"), OptionsDialog::BandwidthClass)
  EVT_LISTBOX(XRCID("NatMethods"), OptionsDialog::NATMethodSelect)
#if OPAL_PTLIB_SSL
  EVT_BUTTON(XRCID("FindCertificateAuthority"), OptionsDialog::FindCertificateAuthority)
  EVT_BUTTON(XRCID("FindLocalCertificate"), OptionsDialog::FindLocalCertificate)
  EVT_BUTTON(XRCID("FindPrivateKey"), OptionsDialog::FindPrivateKey)
#endif
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
  EVT_BUTTON(XRCID("TestPlayer"), OptionsDialog::TestPlayer)
  EVT_BUTTON(XRCID("TestRecorder"), OptionsDialog::TestRecorder)
  EVT_COMBOBOX(XRCID("MusicOnHold"), OptionsDialog::ChangedMusicOnHold)
  EVT_COMBOBOX(XRCID("AudioOnRing"), OptionsDialog::ChangedAudioOnRing)

  ////////////////////////////////////////
  // Video fields
  EVT_COMBOBOX(XRCID("VideoGrabDevice"), OptionsDialog::ChangeVideoGrabDevice)
  EVT_COMBOBOX(XRCID("VideoWatermarkDevice"), OptionsDialog::ChangeVideoWatermarkDevice)
  EVT_BUTTON(XRCID("TestVideoCapture"), OptionsDialog::TestVideoCapture)
  EVT_USER_COMMAND(wxEvtTestVideoEnded, OptionsDialog::OnTestVideoEnded)
  EVT_COMBOBOX(XRCID("VideoOnHold"), OptionsDialog::ChangedVideoOnHold)
  EVT_COMBOBOX(XRCID("VideoOnRing"), OptionsDialog::ChangedVideoOnRing)

  ////////////////////////////////////////
  // Fax fields
  EVT_BUTTON(XRCID("FaxBrowseReceiveDirectory"), OptionsDialog::BrowseFaxDirectory)

#if OPAL_HAS_PRESENCE
  ////////////////////////////////////////
  // Presence fields
  EVT_BUTTON(XRCID("AddPresentity"), OptionsDialog::AddPresentity)
  EVT_BUTTON(XRCID("RemovePresentity"), OptionsDialog::RemovePresentity)
  EVT_LIST_ITEM_SELECTED(XRCID("Presentities"), OptionsDialog::SelectedPresentity)
  EVT_LIST_ITEM_DESELECTED(XRCID("Presentities"), OptionsDialog::DeselectedPresentity)
  EVT_LIST_END_LABEL_EDIT(XRCID("Presentities"), OptionsDialog::EditedPresentity)
  EVT_GRID_CMD_CELL_CHANGED(XRCID("PresentityAttributes"), OptionsDialog::ChangedPresentityAttribute)
#endif // OPAL_HAS_PRESENCE

  ////////////////////////////////////////
  // Codec fields
  EVT_LIST_ITEM_SELECTED(ID_AllCodecs, OptionsDialog::SelectedCodecToAdd)
  EVT_LIST_ITEM_DESELECTED(ID_AllCodecs, OptionsDialog::SelectedCodecToAdd)
  EVT_LISTBOX(ID_SelectedCodecs, OptionsDialog::SelectedCodec)
  EVT_BUTTON(ID_AddCodec, OptionsDialog::AddCodec)
  EVT_BUTTON(ID_RemoveCodec, OptionsDialog::RemoveCodec)
  EVT_BUTTON(ID_MoveUpCodec, OptionsDialog::MoveUpCodec)
  EVT_BUTTON(ID_MoveDownCodec, OptionsDialog::MoveDownCodec)
  EVT_LIST_ITEM_SELECTED(ID_CodecOptionsList, OptionsDialog::SelectedCodecOption)
  EVT_LIST_ITEM_DESELECTED(ID_CodecOptionsList, OptionsDialog::DeselectedCodecOption)
  EVT_TEXT(ID_CodecOptionValue, OptionsDialog::ChangedCodecOptionValue)
  EVT_BUTTON(ID_SetDefaultCodecOption, OptionsDialog::SetDefaultCodecOption)

  ////////////////////////////////////////
  // H.323 fields
  EVT_BUTTON(XRCID("MoreOptionsH323"), OptionsDialog::MoreOptionsH323)
  EVT_LISTBOX(XRCID("Aliases"), OptionsDialog::SelectedAlias)
  EVT_BUTTON(XRCID("AddAlias"), OptionsDialog::AddAlias)
  EVT_BUTTON(XRCID("RemoveAlias"), OptionsDialog::RemoveAlias)
  EVT_TEXT(XRCID("NewAlias"), OptionsDialog::ChangedNewAlias)

  ////////////////////////////////////////
  // SIP fields
  EVT_BUTTON(XRCID("MoreOptionsSIP"), OptionsDialog::MoreOptionsSIP)
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
  // Tracing fields
#if PTRACING
  EVT_BUTTON(XRCID("BrowseTraceFile"), OptionsDialog::BrowseTraceFile)
#endif
END_EVENT_TABLE()


#define INIT_FIELD(name, value) \
  m_##name = value; \
  FindWindowByName(name##Key)->SetValidator(wxGenericValidator(&m_##name))

#define INIT_FIELD_CTRL(name, value) \
  m_##name = value; \
  FindWindowByNameAs(m_##name##Ctrl, this, name##Key)->SetValidator(wxGenericValidator(&m_##name))


#if wxCHECK_VERSION(2,9,0)
static int wxCALLBACK MediaFormatSort(wxIntPtr item1, wxIntPtr item2, wxIntPtr)
#else
static int wxCALLBACK MediaFormatSort(long item1, long item2, long)
#endif
{
  MyMedia * media1 = (MyMedia *)item1;
  MyMedia * media2 = (MyMedia *)item2;
  PCaselessString name1 = media1->mediaFormat.GetMediaType().c_str() + media1->mediaFormat.GetName();
  PCaselessString name2 = media2->mediaFormat.GetMediaType().c_str() + media2->mediaFormat.GetName();
  return name1.Compare(name2);
}


OptionsDialog::OptionsDialog(MyManager * manager)
  : m_manager(*manager)
  , m_TestVideoThread(NULL)
  , m_TestVideoGrabber(NULL)
  , m_TestVideoDisplay(NULL)
  , m_H323options(manager, manager->h323EP)
  , m_SIPoptions(manager, manager->sipEP)
{
  PINDEX i;
  wxChoice * choice;
  wxComboBox * combo;

  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("OptionsDialog"));

  PTRACE(4, "OpenPhone\tLoaded options dialog");

  ////////////////////////////////////////
  // General fields
  INIT_FIELD(Username, m_manager.GetDefaultUserName());
  INIT_FIELD(DisplayName, m_manager.GetDefaultDisplayName());

  FindWindowByNameAs(choice, this, RingSoundDeviceNameKey)->SetValidator(wxGenericValidator(&m_RingSoundDeviceName));
  FillAudioDeviceComboBox(choice, PSoundChannel::Player);
  m_RingSoundDeviceName = MediaDeviceNameToScreen(m_manager.m_RingSoundDeviceName);
  INIT_FIELD(RingSoundFileName, m_manager.m_RingSoundFileName);

  INIT_FIELD(AutoAnswer, m_manager.m_autoAnswer);
  INIT_FIELD(HideMinimised, m_manager.m_hideMinimised);

  INIT_FIELD(OverrideProductInfo, m_manager.m_OverrideProductInfo);
  INIT_FIELD_CTRL(VendorName, m_manager.m_overriddenProductInfo.vendor);
  INIT_FIELD_CTRL(ProductName, m_manager.m_overriddenProductInfo.name);
  INIT_FIELD_CTRL(ProductVersion, m_manager.m_overriddenProductInfo.version);
  m_VendorNameCtrl->Enable(m_OverrideProductInfo);
  m_ProductNameCtrl->Enable(m_OverrideProductInfo);
  m_ProductVersionCtrl->Enable(m_OverrideProductInfo);

  ////////////////////////////////////////
  // Networking fields
#if OPAL_H323
  OpalBandwidth rxBandwidth = m_manager.h323EP->GetInitialBandwidth(OpalBandwidth::Rx);
  m_RxBandwidth.sprintf(wxT("%.1f"), rxBandwidth/1000.0);
  FindWindowByName(RxBandwidthKey)->SetValidator(wxTextValidator(wxFILTER_NUMERIC, &m_RxBandwidth));

  OpalBandwidth txBandwidth = m_manager.h323EP->GetInitialBandwidth(OpalBandwidth::Tx);
  m_TxBandwidth.sprintf(wxT("%.1f"), txBandwidth/1000.0);
  FindWindowByName(TxBandwidthKey)->SetValidator(wxTextValidator(wxFILTER_NUMERIC, &m_TxBandwidth));

  int bandwidthClass;
  if (rxBandwidth <= 33600 && txBandwidth <= 33600)
    bandwidthClass = 0;
  else if (rxBandwidth <= 64000 && txBandwidth <= 64000)
    bandwidthClass = 1;
  else if (rxBandwidth <= 128000 && txBandwidth <= 128000)
    bandwidthClass = 2;
  else if (rxBandwidth <= 1500000 && txBandwidth <= 128000)
    bandwidthClass = 3;
  else if (rxBandwidth <= 4000000 && txBandwidth <= 512000)
    bandwidthClass = 4;
  else if (rxBandwidth <= 1472000 && txBandwidth <= 1472000)
    bandwidthClass = 5;
  else if (rxBandwidth <= 1920000 && txBandwidth <= 1920000)
    bandwidthClass = 6;
  else
    bandwidthClass = 7;
  FindWindowByNameAs(choice, this, wxT("BandwidthClass"))->SetSelection(bandwidthClass);
#endif

  INIT_FIELD(DiffServAudio, m_manager.GetMediaQoS(OpalMediaType::Audio()).m_dscp);
  INIT_FIELD(DiffServVideo, m_manager.GetMediaQoS(OpalMediaType::Video()).m_dscp);
  INIT_FIELD(MaxRtpPayloadSize, m_manager.GetMaxRtpPayloadSize());
#if OPAL_PTLIB_SSL
  INIT_FIELD(CertificateAuthority, m_manager.GetSSLCertificateAuthorityFiles());
  INIT_FIELD(LocalCertificate, m_manager.GetSSLCertificateFile());
  INIT_FIELD(PrivateKey, m_manager.GetSSLPrivateKeyFile());
#else
  FindWindow(CertificateAuthorityKey)->Disable();
  FindWindow(LocalCertificateKey)->Disable();
  FindWindow(PrivateKeyKey)->Disable();
#endif
  INIT_FIELD(TCPPortBase, m_manager.GetTCPPortBase());
  INIT_FIELD(TCPPortMax, m_manager.GetTCPPortMax());
  INIT_FIELD(UDPPortBase, m_manager.GetUDPPortBase());
  INIT_FIELD(UDPPortMax, m_manager.GetUDPPortMax());
  INIT_FIELD(RTPPortBase, m_manager.GetRtpIpPortBase());
  INIT_FIELD(RTPPortMax, m_manager.GetRtpIpPortMax());

  m_NatMethodSelected= -1;
  FindWindowByNameAs(m_NatMethods, this, wxT("NatMethods"));
  for (PNatMethods::iterator it = m_manager.GetNatMethods().begin(); it != m_manager.GetNatMethods().end(); ++it) {
    int pos = m_NatMethods->Append(PwxString(it->GetFriendlyName()));
    NatWrapper * wrap = new NatWrapper;
    wrap->m_method = it->GetMethodName();
    wrap->m_active = it->IsActive();
    wrap->m_server = it->GetServer();
    wrap->m_priority = it->GetPriority();
    m_NatMethods->SetClientObject(pos, wrap);
  }
  FindWindow(wxT("NatActive"))->SetValidator(wxGenericValidator(&m_NatInfo.m_active));
  FindWindow(wxT("NatServer"))->SetValidator(wxGenericValidator(&m_NatInfo.m_server));
  FindWindow(wxT("NatPriority"))->SetValidator(wxGenericValidator(&m_NatInfo.m_priority));

  FindWindowByNameAs(m_AddInterface, this, wxT("AddInterface"))->Disable();
  FindWindowByNameAs(m_RemoveInterface, this, wxT("RemoveInterface"))->Disable();
  FindWindowByNameAs(m_InterfaceProtocol, this, wxT("InterfaceProtocol"));
  FindWindowByNameAs(m_InterfacePort, this, wxT("InterfacePort"));
  FindWindowByNameAs(m_InterfaceAddress, this, wxT("InterfaceAddress"))->Append(wxT("*"));
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
  FindWindowByNameAs(m_LocalInterfaces, this, wxT("LocalInterfaces"));
  for (i = 0; (size_t)i < m_manager.m_LocalInterfaces.size(); i++)
    m_LocalInterfaces->Append(PwxString(m_manager.m_LocalInterfaces[i]));

  ////////////////////////////////////////
  // Sound fields
  INIT_FIELD(SoundBufferTime, m_manager.pcssEP->GetSoundChannelBufferTime());
#if OPAL_AEC
  INIT_FIELD(EchoCancellation, m_manager.GetEchoCancelParams().m_enabled);
#endif
  INIT_FIELD(LocalRingbackTone, m_manager.pcssEP->GetLocalRingbackTone());
  INIT_FIELD(MinJitter, m_manager.GetMinAudioJitterDelay());
  INIT_FIELD(MaxJitter, m_manager.GetMaxAudioJitterDelay());
  INIT_FIELD(SilenceSuppression, m_manager.GetSilenceDetectParams().m_mode);
  INIT_FIELD(SilenceThreshold, m_manager.GetSilenceDetectParams().m_threshold);
  INIT_FIELD(SignalDeadband, m_manager.GetSilenceDetectParams().m_signalDeadband);
  INIT_FIELD(SilenceDeadband, m_manager.GetSilenceDetectParams().m_silenceDeadband);
  INIT_FIELD(DisableDetectInBandDTMF, m_manager.DetectInBandDTMFDisabled());

  // Fill sound player combo box with available devices and set selection
  FindWindowByNameAs(m_soundPlayerCombo, this, SoundPlayerKey)->SetValidator(wxGenericValidator(&m_SoundPlayer));
  FillAudioDeviceComboBox(m_soundPlayerCombo, PSoundChannel::Player);
  m_SoundPlayer = MediaDeviceNameToScreen(m_manager.pcssEP->GetSoundChannelPlayDevice());

  // Fill sound recorder combo box with available devices and set selection
  FindWindowByNameAs(m_soundRecorderCombo, this, SoundRecorderKey)->SetValidator(wxGenericValidator(&m_SoundRecorder));
  FillAudioDeviceComboBox(m_soundRecorderCombo, PSoundChannel::Recorder);
  m_SoundRecorder = MediaDeviceNameToScreen(m_manager.pcssEP->GetSoundChannelRecordDevice());

  // Fill sound on-hold combo box with available devices and set selection
  FindWindowByNameAs(m_musicOnHoldCombo, this, MusicOnHoldKey)->SetValidator(wxGenericValidator(&m_MusicOnHold));
  FillAudioDeviceComboBox(m_musicOnHoldCombo, PSoundChannel::Recorder);
  m_MusicOnHold = MediaDeviceNameToScreen(m_manager.pcssEP->GetSoundChannelOnHoldDevice());

  // Fill sound on-ring combo box with available devices and set selection
  FindWindowByNameAs(m_audioOnRingCombo, this, AudioOnRingKey)->SetValidator(wxGenericValidator(&m_AudioOnRing));
  FillAudioDeviceComboBox(m_audioOnRingCombo, PSoundChannel::Recorder);
  m_AudioOnRing = MediaDeviceNameToScreen(m_manager.pcssEP->GetSoundChannelOnRingDevice());

  // Fill line interface combo box with available devices and set selection
  FindWindowByNameAs(m_selectedAEC, this, AECKey);
  FindWindowByNameAs(m_selectedCountry, this, CountryKey);
  m_selectedCountry->SetValidator(wxGenericValidator(&m_Country));
  FindWindowByNameAs(m_selectedLID, this, LineInterfaceDeviceKey);
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
  INIT_FIELD_CTRL(VideoGrabSource, m_manager.GetVideoInputDevice().channelNumber+1);
  INIT_FIELD(VideoGrabFrameRate, m_manager.GetVideoInputDevice().rate);
  INIT_FIELD(VideoFlipLocal, m_manager.GetVideoInputDevice().flip != false);
  INIT_FIELD(VideoGrabPreview, m_manager.m_VideoGrabPreview);
  INIT_FIELD(VideoWatermarkDevice, MediaDeviceNameToScreen(m_manager.m_VideoWatermarkDevice));
  INIT_FIELD(VideoAutoTransmit, m_manager.CanAutoStartTransmitVideo() != false);
  INIT_FIELD(VideoAutoReceive, m_manager.CanAutoStartReceiveVideo() != false);
  INIT_FIELD(VideoFlipRemote, m_manager.GetVideoOutputDevice().flip != false);
  INIT_FIELD(VideoGrabBitRate, m_manager.m_VideoTargetBitRate);
  INIT_FIELD(VideoMaxBitRate, m_manager.m_VideoMaxBitRate);
  INIT_FIELD(VideoOnHold, m_manager.pcssEP->GetVideoOnHoldDevice().deviceName);
  INIT_FIELD(VideoOnRing, m_manager.pcssEP->GetVideoOnRingDevice().deviceName);

  PStringArray knownSizes = PVideoFrameInfo::GetSizeNames();
  m_VideoGrabFrameSize = m_manager.m_VideoGrabFrameSize;
  FindWindowByNameAs(combo, this, VideoGrabFrameSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoGrabFrameSize));
  for (i = 0; i < knownSizes.GetSize(); ++i)
    combo->Append(PwxString(knownSizes[i]));

  m_VideoMinFrameSize = m_manager.m_VideoMinFrameSize;
  FindWindowByNameAs(combo, this, VideoMinFrameSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoMinFrameSize));
  for (i = 0; i < knownSizes.GetSize(); ++i)
    combo->Append(PwxString(knownSizes[i]));

  m_VideoMaxFrameSize = m_manager.m_VideoMaxFrameSize;
  FindWindowByNameAs(combo, this, VideoMaxFrameSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoMaxFrameSize));
  for (i = 0; i < knownSizes.GetSize(); ++i)
    combo->Append(PwxString(knownSizes[i]));

  FindWindowByNameAs(m_VideoGrabDeviceCtrl, this, wxT("VideoGrabDevice"));
  devices = PVideoInputDevice::GetDriversDeviceNames("*\t*");
  for (i = 0; i < devices.GetSize(); i++)
    m_VideoGrabDeviceCtrl->Append(MediaDeviceNameToScreen(devices[i]));

  FindWindowByNameAs(m_VideoWatermarkDeviceCtrl, this, wxT("VideoWatermarkDevice"));
  devices = PVideoInputDevice::GetDriversDeviceNames("*\t*");
  for (i = 0; i < devices.GetSize(); i++)
    m_VideoWatermarkDeviceCtrl->Append(MediaDeviceNameToScreen(devices[i]));

  FindWindowByNameAs(m_VideoOnHoldDeviceCtrl, this, wxT("VideoOnHold"));
  for (i = 0; i < devices.GetSize(); i++)
    m_VideoOnHoldDeviceCtrl->Append(MediaDeviceNameToScreen(devices[i]));

  FindWindowByNameAs(m_VideoOnRingDeviceCtrl, this, wxT("VideoOnRing"));
  for (i = 0; i < devices.GetSize(); i++)
    m_VideoOnRingDeviceCtrl->Append(MediaDeviceNameToScreen(devices[i]));

  AdjustVideoControls();

  FindWindowByNameAs(m_TestVideoCapture, this, wxT("TestVideoCapture"));

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
  FindWindowByNameAs(combo, this, VideoRecordingSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoRecordingSize));
  for (i = 0; i < knownSizes.GetSize(); ++i)
    combo->Append(PwxString(knownSizes[i]));

  FindWindowByNameAs(choice, this, AudioRecordingFormatKey);
  PWAVFileFormatByFormatFactory::KeyList_T wavFileFormats = PWAVFileFormatByFormatFactory::GetKeyList();
  for (PWAVFileFormatByFormatFactory::KeyList_T::iterator iterFmt = wavFileFormats.begin(); iterFmt != wavFileFormats.end(); ++iterFmt)
    choice->Append(PwxString(*iterFmt));

  ////////////////////////////////////////
  // Presence fields

#if OPAL_HAS_PRESENCE
  FindWindowByNameAs(m_Presentities, this, wxT("Presentities"))->InsertColumn(0, _T("Address"));
  PStringArray presentities = m_manager.GetPresentities();
  for (i = 0; i < presentities.GetSize(); ++i) {
    long pos = m_Presentities->InsertItem(INT_MAX, PwxString(presentities[i]));
    m_Presentities->SetItemPtrData(pos, (wxUIntPtr)m_manager.GetPresentity(presentities[i])->Clone());
  }
  m_Presentities->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);

  FindWindowByNameAs(m_AddPresentity, this, wxT("AddPresentity"));
  FindWindowByNameAs(m_RemovePresentity, this, wxT("RemovePresentity"))->Disable();

  FindWindowByNameAs(m_PresentityAttributes, this, wxT("PresentityAttributes"));
  m_PresentityAttributes->Disable();
  m_PresentityAttributes->CreateGrid(0, 1);
  m_PresentityAttributes->SetColLabelValue(0, wxT("Attribute Value"));
  m_PresentityAttributes->SetColLabelSize(wxGRID_AUTOSIZE);
  m_PresentityAttributes->AutoSizeColLabelSize(0);
  m_PresentityAttributes->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_TOP);
#else
  RemoveNotebookPage(this, wxT("Presence"));
#endif // OPAL_HAS_PRESENCE

  ////////////////////////////////////////
  // Codec fields
  FindWindowByNameAs(m_AddCodec, this, ID_AddCodec)->Disable();
  FindWindowByNameAs(m_RemoveCodec, this, ID_RemoveCodec)->Disable();
  FindWindowByNameAs(m_MoveUpCodec, this, ID_MoveUpCodec)->Disable();
  FindWindowByNameAs(m_MoveDownCodec, this, ID_MoveDownCodec)->Disable();

  FindWindowByNameAs(m_allCodecs, this, ID_AllCodecs);
  m_allCodecs->InsertColumn(0, wxT("Name"), wxLIST_FORMAT_LEFT);
  m_allCodecs->InsertColumn(1, wxT("Description"), wxLIST_FORMAT_LEFT);
  FindWindowByNameAs(m_selectedCodecs, this, ID_SelectedCodecs);
  for (MyMediaList::iterator mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm) {
    PwxString details;
    details << mm->mediaFormat.GetMediaType().c_str() << ": " << mm->mediaFormat.GetName();
    if (mm->validProtocols != NULL)
      details << mm->validProtocols;
    long index = m_allCodecs->InsertItem(INT_MAX, details);
    m_allCodecs->SetItemPtrData(index, (wxUIntPtr)&*mm);
    m_allCodecs->SetItem(index, 1, PwxString(mm->mediaFormat.GetDescription()));

    PwxString str(mm->mediaFormat.GetName());
    if (mm->preferenceOrder >= 0 && m_selectedCodecs->FindString(str) < 0)
      m_selectedCodecs->Append(str, &*mm);
  }
  m_allCodecs->SortItems(MediaFormatSort, 0);
  m_allCodecs->SetColumnWidth(0, wxLIST_AUTOSIZE);
  m_allCodecs->SetColumnWidth(1, wxLIST_AUTOSIZE);

  FindWindowByNameAs(m_codecOptions, this, ID_CodecOptionsList);
  int columnWidth = (m_codecOptions->GetClientSize().GetWidth()-30)/2;
  m_codecOptions->InsertColumn(0, wxT("Option"), wxLIST_FORMAT_LEFT, columnWidth);
  m_codecOptions->InsertColumn(1, wxT("Value"), wxLIST_FORMAT_LEFT, columnWidth);
  FindWindowByNameAs(m_codecOptionValue, this, ID_CodecOptionValue)->Disable();
  FindWindowByNameAs(m_CodecOptionValueLabel, this, wxT("CodecOptionValueLabel"))->Disable();
  FindWindowByNameAs(m_CodecOptionValueError, this, wxT("CodecOptionValueError"))->Show(false);
  FindWindowByNameAs(m_SetDefaultCodecOption, this, ID_SetDefaultCodecOption)->Disable();

#if OPAL_H323
  ////////////////////////////////////////
  // H.323 fields
  FindWindowByNameAs(m_AddAlias, this, wxT("AddAlias"))->Disable();
  FindWindowByNameAs(m_RemoveAlias, this, wxT("RemoveAlias"))->Disable();
  FindWindowByNameAs(m_NewAlias, this, wxT("NewAlias"));
  FindWindowByNameAs(m_Aliases, this, wxT("Aliases"));
  for (i = 0; i < m_manager.h323EP->m_configuredAliasNames.GetSize(); i++)
    m_Aliases->Append(PwxString(m_manager.h323EP->m_configuredAliasNames[i]));

  FindWindowByNameAs(combo, this, wxT("H323TerminalType"))->SetValidator(wxTerminalTypeValidator(&m_H323TerminalType));
  m_H323TerminalType << m_manager.h323EP->GetTerminalType() << wxChar(' ');
  for (i = 0; i < (PINDEX)combo->GetCount(); ++i) {
    wxString entry = combo->GetString(i);
    if (entry.StartsWith(m_H323TerminalType)) {
      m_H323TerminalType = entry;
      break;
    }
  }

  INIT_FIELD(DTMFSendMode, m_manager.h323EP->GetSendUserInputMode());
  if (m_DTMFSendMode >= OpalConnection::SendUserInputAsProtocolDefault)
    m_DTMFSendMode = OpalConnection::SendUserInputAsString;
#if OPAL_450
  INIT_FIELD(CallIntrusionProtectionLevel, m_manager.h323EP->GetCallIntrusionProtectionLevel());
#endif
  INIT_FIELD(DisableFastStart, m_manager.h323EP->IsFastStartDisabled() != false);
  INIT_FIELD(DisableH245Tunneling, m_manager.h323EP->IsH245TunnelingDisabled() != false);
  INIT_FIELD(DisableH245inSETUP, m_manager.h323EP->IsH245inSetupDisabled() != false);
  INIT_FIELD(ForceSymmetricTCS, m_manager.h323EP->IsForcedSymmetricTCS() != false);

  INIT_FIELD(ExtendedVideoRoles, m_manager.m_ExtendedVideoRoles);
  INIT_FIELD(EnableH239Control, m_manager.h323EP->GetDefaultH239Control());

  INIT_FIELD(GatekeeperMode, m_manager.m_gatekeeperMode);
  INIT_FIELD(GatekeeperAddress, m_manager.m_gatekeeperAddress);
  INIT_FIELD(GatekeeperInterface, m_manager.m_gatekeeperInterface);
  INIT_FIELD(GatekeeperSuppressGRQ, !m_manager.h323EP->GetSendGRQ());
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

  m_SelectedRegistration = INT_MAX;

  FindWindowByNameAs(m_Registrations, this, wxT("Registrars"));
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

  FindWindowByNameAs(m_ChangeRegistration, this, wxT("ChangeRegistrar"));
  FindWindowByNameAs(m_RemoveRegistration, this, wxT("RemoveRegistrar"));
  FindWindowByNameAs(m_MoveUpRegistration, this, wxT("MoveUpRegistrar"));
  FindWindowByNameAs(m_MoveDownRegistration, this, wxT("MoveDownRegistrar"));
#endif // OPAL_SIP


  ////////////////////////////////////////
  // Routing fields
  INIT_FIELD(ForwardingAddress, m_manager.m_ForwardingAddress);
  INIT_FIELD(ForwardingTimeout, m_manager.m_ForwardingTimeout);

  m_SelectedRoute = INT_MAX;

  FindWindowByNameAs(m_RouteDevice, this, wxT("RouteDevice"));
  FindWindowByNameAs(m_RoutePattern, this, wxT("RoutePattern"));
  FindWindowByNameAs(m_RouteDestination, this, wxT("RouteDestination"));

  FindWindowByNameAs(m_AddRoute, this, wxT("AddRoute"))->Disable();
  FindWindowByNameAs(m_RemoveRoute, this, wxT("RemoveRoute"))->Disable();
  FindWindowByNameAs(m_MoveUpRoute, this, wxT("MoveUpRoute"))->Disable();
  FindWindowByNameAs(m_MoveDownRoute, this, wxT("MoveDownRoute"))->Disable();

  // Fill list box with active routes
  FindWindowByNameAs(m_Routes, this, wxT("Routes"));
  m_Routes->InsertColumn(0, _T("Source"));
  m_Routes->InsertColumn(1, _T("Dev/If"));
  m_Routes->InsertColumn(2, _T("Pattern"));
  m_Routes->InsertColumn(3, _T("Destination"));
  const OpalManager::RouteTable & routeTable = m_manager.GetRouteTable();
  for (i = 0; i < routeTable.GetSize(); i++)
    AddRouteTableEntry(routeTable[i]);

  for (i = 0; i < (PINDEX)m_Routes->GetColumnCount(); i++)
    m_Routes->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);

  {
    OpalEndPoint * ep = m_manager.FindEndPoint("tel");
    if (ep != NULL)
      m_telURI = ep->GetPrefixName();
    FindWindowByNameAs(combo, this, telURIKey)->SetValidator(wxGenericValidator(&m_telURI));
    combo->Append(wxEmptyString);
  }

  // Fill combo box with possible protocols
  FindWindowByNameAs(m_RouteSource, this, wxT("RouteSource"))->Append(AllRouteSources);
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
  StopTestVideo();

  long i;
#if OPAL_HAS_PRESENCE
  for (i = 0; i < m_Presentities->GetItemCount(); ++i)
    delete (OpalPresentity *)m_Presentities->GetItemData(i);
#endif
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


bool OptionsDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  double floatRxBandwidth;
  if (!m_RxBandwidth.ToDouble(&floatRxBandwidth) || floatRxBandwidth < 10)
    return false;

  double floatTxBandwidth;
  if (!m_TxBandwidth.ToDouble(&floatTxBandwidth) || floatTxBandwidth < 10)
    return false;

  ::wxBeginBusyCursor();

  wxConfigBase * config = wxConfig::Get();

  ////////////////////////////////////////
  // General fields
  config->SetPath(GeneralGroup);
  SAVE_FIELD(Username, m_manager.SetDefaultUserName);
  SAVE_FIELD(DisplayName, m_manager.SetDefaultDisplayName);
  SAVE_FIELD(RingSoundDeviceName, m_manager.m_RingSoundDeviceName = MediaDeviceNameFromScreen);
  SAVE_FIELD(RingSoundFileName, m_manager.m_RingSoundFileName = );
  SAVE_FIELD(AutoAnswer, m_manager.m_autoAnswer = );
  SAVE_FIELD(HideMinimised, m_manager.m_hideMinimised = );

  SAVE_FIELD(OverrideProductInfo, m_manager.m_OverrideProductInfo = );
  if (m_OverrideProductInfo) {
    SAVE_FIELD_STR(VendorName, m_manager.m_overriddenProductInfo.vendor = );
    SAVE_FIELD_STR(ProductName, m_manager.m_overriddenProductInfo.name = );
    SAVE_FIELD_STR(ProductVersion, m_manager.m_overriddenProductInfo.version = );
    m_manager.SetProductInfo(m_manager.m_overriddenProductInfo);
  }
  else {
    OpalProductInfo productInfo = m_manager.GetProductInfo();
    productInfo.vendor = PProcess::Current().GetManufacturer();
    productInfo.name = PProcess::Current().GetName();
    productInfo.version = PProcess::Current().GetVersion();
    m_manager.SetProductInfo(productInfo);
  }

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
#if OPAL_H323
  m_manager.h323EP->SetInitialBandwidth(OpalBandwidth::Rx, (unsigned)(floatRxBandwidth*1000));
  m_manager.h323EP->SetInitialBandwidth(OpalBandwidth::Tx, (unsigned)(floatTxBandwidth*1000));
#endif
  config->Write(RxBandwidthKey, floatRxBandwidth);
  config->Write(TxBandwidthKey, floatTxBandwidth);

  {
    PIPSocket::QoS qos = m_manager.GetMediaQoS(OpalMediaType::Audio());
    SAVE_FIELD(DiffServAudio, qos.m_dscp = );
    m_manager.SetMediaQoS(OpalMediaType::Audio(), qos);
    qos = m_manager.GetMediaQoS(OpalMediaType::Video());
    SAVE_FIELD(DiffServVideo, qos.m_dscp = );
    m_manager.SetMediaQoS(OpalMediaType::Video(), qos);
  }

  SAVE_FIELD(MaxRtpPayloadSize, m_manager.SetMaxRtpPayloadSize);
#if OPAL_PTLIB_SSL
  SAVE_FIELD(CertificateAuthority, m_manager.SetSSLCertificateAuthorityFiles);
  SAVE_FIELD(LocalCertificate, m_manager.SetSSLCertificateFile);
  SAVE_FIELD(PrivateKey, m_manager.SetSSLPrivateKeyFile);
#endif
  SAVE_FIELD2(TCPPortBase, TCPPortMax, m_manager.SetTCPPorts);
  SAVE_FIELD2(UDPPortBase, UDPPortMax, m_manager.SetUDPPorts);
  SAVE_FIELD2(RTPPortBase, RTPPortMax, m_manager.SetRtpIpPorts);

  if (m_NatMethodSelected >= 0)
    dynamic_cast<NatWrapper &>(*m_NatMethods->GetClientObject(m_NatMethodSelected)) = m_NatInfo;

  for (size_t i = 0; i < m_NatMethods->GetCount(); i++) {
    NatWrapper & wrap = dynamic_cast<NatWrapper &>(*m_NatMethods->GetClientObject(i));
    config->SetPath(NatMethodsGroup);
    config->SetPath(wrap.m_method);
    config->Write(NATActiveKey, wrap.m_active);
    config->Write(NATServerKey, wrap.m_server);
    config->Write(NATPriorityKey, wrap.m_priority);
    m_manager.SetNAT(wrap.m_method, wrap.m_active, wrap.m_server, wrap.m_priority);
  }

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
    key << (i+1);
    config->Write(key, PwxString(newInterfaces[i]));
  }
  if (changed) {
    m_manager.m_LocalInterfaces = newInterfaces;
    m_manager.StartAllListeners();
  }

  ////////////////////////////////////////
  // Sound fields
  config->SetPath(AudioGroup);

  if (m_manager.pcssEP->SetSoundChannelPlayDevice(MediaDeviceNameFromScreen(m_SoundPlayer)))
    config->Write(SoundPlayerKey, PwxString(m_manager.pcssEP->GetSoundChannelPlayDevice()));
  else
    wxMessageBox(wxT("Could not use sound player device."), OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);

  if (m_manager.pcssEP->SetSoundChannelRecordDevice(MediaDeviceNameFromScreen(m_SoundRecorder)))
    config->Write(SoundRecorderKey, PwxString(m_manager.pcssEP->GetSoundChannelRecordDevice()));
  else
    wxMessageBox(wxT("Could not use sound recorder device."), OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);

  if (m_manager.pcssEP->SetSoundChannelOnHoldDevice(MediaDeviceNameFromScreen(m_MusicOnHold)))
    config->Write(MusicOnHoldKey, PwxString(m_manager.pcssEP->GetSoundChannelOnHoldDevice()));
  else
    wxMessageBox(wxT("Could not use sound recorder device for hold."), OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);

  if (m_manager.pcssEP->SetSoundChannelOnRingDevice(MediaDeviceNameFromScreen(m_AudioOnRing)))
    config->Write(AudioOnRingKey, PwxString(m_manager.pcssEP->GetSoundChannelOnRingDevice()));
  else
    wxMessageBox(wxT("Could not use sound recorder device for ring."), OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);

  SAVE_FIELD(SoundBufferTime, m_manager.pcssEP->SetSoundChannelBufferTime);

#if OPAL_AEC
  OpalEchoCanceler::Params aecParams;
  SAVE_FIELD(EchoCancellation, aecParams.m_enabled =);
  m_manager.SetEchoCancelParams(aecParams);
#endif

  SAVE_FIELD(LocalRingbackTone, m_manager.pcssEP->SetLocalRingbackTone);
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
  PVideoDevice::OpenArgs videoDevice = m_manager.GetVideoInputDevice();
  SAVE_FIELD_STR(VideoGrabDevice, videoDevice.deviceName = );
  SAVE_FIELD(VideoGrabFormat, videoDevice.videoFormat = (PVideoDevice::VideoFormat));
  --m_VideoGrabSource;
  SAVE_FIELD(VideoGrabSource, videoDevice.channelNumber = );
  SAVE_FIELD(VideoGrabFrameRate, videoDevice.rate = );
  SAVE_FIELD(VideoGrabFrameSize, m_manager.m_VideoGrabFrameSize = );
  SAVE_FIELD(VideoFlipLocal, videoDevice.flip = );
  m_manager.SetVideoInputDevice(videoDevice);

  SAVE_FIELD(VideoWatermarkDevice, m_manager.m_VideoWatermarkDevice = MediaDeviceNameFromScreen);

  SAVE_FIELD(VideoGrabPreview, m_manager.m_VideoGrabPreview = );
  SAVE_FIELD(VideoAutoTransmit, m_manager.SetAutoStartTransmitVideo);
  SAVE_FIELD(VideoAutoReceive, m_manager.SetAutoStartReceiveVideo);
//  SAVE_FIELD(VideoFlipRemote, );
  SAVE_FIELD(VideoMinFrameSize, m_manager.m_VideoMinFrameSize = );
  SAVE_FIELD(VideoMaxFrameSize, m_manager.m_VideoMaxFrameSize = );
  SAVE_FIELD(VideoGrabBitRate, m_manager.m_VideoTargetBitRate = );
  SAVE_FIELD(VideoMaxBitRate, m_manager.m_VideoMaxBitRate = );

  videoDevice = m_manager.pcssEP->GetVideoOnHoldDevice();
  SAVE_FIELD_STR(VideoOnHold, videoDevice.deviceName = );
  m_manager.pcssEP->SetVideoOnHoldDevice(videoDevice);

  videoDevice = m_manager.pcssEP->GetVideoOnRingDevice();
  SAVE_FIELD_STR(VideoOnRing, videoDevice.deviceName = );
  m_manager.pcssEP->SetVideoOnRingDevice(videoDevice);

  ////////////////////////////////////////
  // Fax fields
#if OPAL_FAX
  config->SetPath(FaxGroup);
  SAVE_FIELD(FaxStationIdentifier, m_manager.m_faxEP->SetDefaultDisplayName);
  SAVE_FIELD(FaxReceiveDirectory, m_manager.m_faxEP->SetDefaultDirectory);
  SAVE_FIELD(FaxAutoAnswerMode, m_manager.m_defaultAnswerMode = (MyManager::FaxAnswerModes));
#endif

#if OPAL_HAS_PRESENCE
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
      bool active = presentity->GetAttributes().GetBoolean(PresenceActiveKey);
      if (activePresentity->IsOpen() != active) {
        if (active)
          LogWindow << (activePresentity->Open() ? "Establishing" : "Could not establish")
                    << " presence for identity " << aor << endl;
        else {
          activePresentity->Close();
          LogWindow << "Stopping presence for identity " << aor << endl;
        }
      }
    }

    PStringList::iterator pos = activePresentities.find(aor.p_str());
    if (pos != activePresentities.end())
      activePresentities.erase(pos);
  }
  for (PStringList::iterator it = activePresentities.begin(); it != activePresentities.end(); ++it)
    m_manager.RemovePresentity(*it);
#endif // OPAL_HAS_PRESENCE

  ////////////////////////////////////////
  // Codec fields
  MyMediaList::iterator mm;
  for (mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm)
    mm->preferenceOrder = -1;

  size_t codecIndex;
  for (codecIndex = 0; codecIndex < m_selectedCodecs->GetCount(); codecIndex++) {
    PwxString selectedFormat = m_selectedCodecs->GetString(codecIndex);
    for (mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm) {
      if (selectedFormat == mm->mediaFormat.GetName()) {
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
      config->Write(CodecNameKey, PwxString(mm->mediaFormat.GetName()));
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

  for (PStringList::iterator it = m_manager.h323EP->m_configuredAliasNames.begin(); it != m_manager.h323EP->m_configuredAliasNames.end(); ++it) {
    if (m_Aliases->FindString(PwxString(*it)) < 0)
      m_manager.h323EP->RemoveAliasName(*it);
  }

  m_manager.h323EP->m_configuredAliasNames.RemoveAll();
  for (size_t i = 0; i < m_Aliases->GetCount(); i++) {
    PwxString alias = m_Aliases->GetString(i);
    m_manager.h323EP->m_configuredAliasNames.AppendString(alias);
    m_manager.h323EP->AddAliasName(alias);
    wxString key;
    key.sprintf(wxT("%u"), (unsigned)i+1);
    config->Write(key, alias);
  }

  config->SetPath(H323Group);
  long terminalType;
  m_H323TerminalType.ToLong(&terminalType);
  m_manager.h323EP->SetTerminalType((H323EndPoint::TerminalTypes)terminalType);
  config->Write(H323TerminalTypeKey, (int)m_manager.h323EP->GetTerminalType());
  m_manager.h323EP->SetSendUserInputMode((H323Connection::SendUserInputModes)m_DTMFSendMode);
  config->Write(DTMFSendModeKey, m_DTMFSendMode);
#if OPAL_450
  SAVE_FIELD(CallIntrusionProtectionLevel, m_manager.h323EP->SetCallIntrusionProtectionLevel);
#endif
  SAVE_FIELD(DisableFastStart, m_manager.h323EP->DisableFastStart);
  SAVE_FIELD(DisableH245Tunneling, m_manager.h323EP->DisableH245Tunneling);
  SAVE_FIELD(DisableH245inSETUP, m_manager.h323EP->DisableH245inSetup);
  SAVE_FIELD(ForceSymmetricTCS, m_manager.h323EP->ForceSymmetricTCS);

  SAVE_FIELD(ExtendedVideoRoles, m_manager.m_ExtendedVideoRoles = (MyManager::ExtendedVideoRoles));
  SAVE_FIELD(EnableH239Control, m_manager.h323EP->SetDefaultH239Control);

#if OPAL_PTLIB_SSL
  m_H323options.SaveSecurityFields(config, H323SignalingSecurityKey, H323MediaCryptoSuitesKey);
#endif
  m_H323options.SaveOptions(config, H323StringOptionsGroup);

  config->Write(GatekeeperTTLKey, m_GatekeeperTTL);
  m_manager.h323EP->SetGatekeeperTimeToLive(PTimeInterval(0, m_GatekeeperTTL));

  bool suppressGRQ = !m_manager.h323EP->GetSendGRQ();
  if (m_manager.m_gatekeeperMode != m_GatekeeperMode ||
      m_manager.m_gatekeeperAddress != m_GatekeeperAddress ||
      m_manager.m_gatekeeperInterface != m_GatekeeperInterface ||
      suppressGRQ != m_GatekeeperSuppressGRQ ||
      m_manager.m_gatekeeperIdentifier != m_GatekeeperIdentifier ||
      PwxString(m_manager.h323EP->GetGatekeeperUsername()) != m_GatekeeperLogin ||
      PwxString(m_manager.h323EP->GetGatekeeperPassword()) != m_GatekeeperPassword) {
    SAVE_FIELD(GatekeeperMode, m_manager.m_gatekeeperMode = );
    SAVE_FIELD(GatekeeperAddress, m_manager.m_gatekeeperAddress = );
    SAVE_FIELD(GatekeeperInterface, m_manager.m_gatekeeperInterface = );
    SAVE_FIELD(GatekeeperSuppressGRQ, suppressGRQ = );
    SAVE_FIELD(GatekeeperIdentifier, m_manager.m_gatekeeperIdentifier = );
    SAVE_FIELD2(GatekeeperPassword, GatekeeperLogin, m_manager.h323EP->SetGatekeeperPassword);

    m_manager.h323EP->SetSendGRQ(!suppressGRQ);
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
  m_SIPoptions.SaveSecurityFields(config, SIPSignalingSecurityKey, SIPMediaCryptoSuitesKey);
#endif
  m_SIPoptions.SaveOptions(config, SIPStringOptionsGroup);

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


static void SetComboValue(wxComboBox * combo, const wxString & value)
{
  // For some bizarre reason combo->SetValue() just does not work for Windows
  int idx = combo->FindString(value);
  if (idx < 0)
    idx = combo->Append(value);
  combo->SetSelection(idx);
}


static bool SelectFileDevice(wxComboBox * combo, wxString & device, const wxChar * prompt, bool save)
{
  PwxString wildcard = combo->GetValue();
  size_t star = wildcard.find('*');
  if (star == wxString::npos)
    return false;

  wxString selectedFile = wxFileSelector(prompt,
                                         wxEmptyString,
                                         wxEmptyString,
                                         wildcard.Mid(star+1),
                                         wildcard.Mid(star),
                                         save ? (wxFD_SAVE|wxFD_OVERWRITE_PROMPT)
                                              : (wxFD_OPEN|wxFD_FILE_MUST_EXIST));
  if (selectedFile.empty())
    return false;

  device = selectedFile;
  SetComboValue(combo, device);
  return true;
}


////////////////////////////////////////
// General fields

void OptionsDialog::BrowseSoundFile(wxCommandEvent & /*event*/)
{
  wxString newFile = wxFileSelector(wxT("Sound file to play on incoming calls"),
                                    wxEmptyString,
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


void OptionsDialog::OnOverrideProductInfo(wxCommandEvent & /*event*/)
{
  if (!wxDialog::TransferDataFromWindow())
    return;

  m_VendorNameCtrl->Enable(m_OverrideProductInfo);
  m_ProductNameCtrl->Enable(m_OverrideProductInfo);
  m_ProductVersionCtrl->Enable(m_OverrideProductInfo);
}


////////////////////////////////////////
// Networking fields

void OptionsDialog::BandwidthClass(wxCommandEvent & event)
{
  static const wxChar * rxBandwidthClasses[] = {
    wxT("33.6"), wxT("64"), wxT("128"), wxT("1500"), wxT("4000"), wxT("1472"), wxT("1920"), wxT("10000")
  };
  static const wxChar * txBandwidthClasses[] = {
    wxT("33.6"), wxT("64"), wxT("128"), wxT("128"), wxT("512"), wxT("1472"), wxT("1920"), wxT("10000")
  };

  m_RxBandwidth = rxBandwidthClasses[event.GetSelection()];
  m_TxBandwidth = txBandwidthClasses[event.GetSelection()];
  TransferDataToWindow();
}


#if OPAL_PTLIB_SSL
void OptionsDialog::FindCertificateAuthority(wxCommandEvent &)
{
  wxString newFile = wxFileSelector(wxT("File or directory for Certificate Authority"),
                                    wxEmptyString,
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
                                    wxEmptyString,
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
                                    wxEmptyString,
                                    m_PrivateKey,
                                    wxT(".key"),
                                    wxT("KEY files (*.key)|*.key|PEM files (*.pem)|*.pem"),
                                    wxFD_OPEN|wxFD_FILE_MUST_EXIST);
  if (!newFile.empty()) {
    m_PrivateKey = newFile;
    TransferDataToWindow();
  }
}
#endif // OPAL_PTLIB_SSL


void OptionsDialog::NATMethodSelect(wxCommandEvent &)
{
  if (m_NatMethodSelected >= 0) {
    wxDialog::TransferDataFromWindow();
    dynamic_cast<NatWrapper &>(*m_NatMethods->GetClientObject(m_NatMethodSelected)) = m_NatInfo;
  }

  m_NatMethodSelected = m_NatMethods->GetSelection();

  if (m_NatMethodSelected >= 0) {
    m_NatInfo = dynamic_cast<NatWrapper &>(*m_NatMethods->GetClientObject(m_NatMethodSelected));
    TransferDataToWindow();
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
  "all:", "h323:tcp$", "h323:tls$", "sip:udp$", "sip:tcp$", "sip:tls$", "sip:ws$", "sip:wss$"
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

  for (PINDEX i = 0; i < PARRAYSIZE(InterfacePrefixes); i++) {
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

void OptionsDialog::ChangedSoundPlayer(wxCommandEvent & /*event*/)
{
  SelectFileDevice(m_soundPlayerCombo, m_SoundPlayer, wxT("Select Sound File to Save"), true);
}


void OptionsDialog::ChangedSoundRecorder(wxCommandEvent & /*event*/)
{
  SelectFileDevice(m_soundRecorderCombo, m_SoundRecorder, wxT("Select Sound File to Send"), false);
}


void OptionsDialog::ChangedMusicOnHold(wxCommandEvent & /*event*/)
{
  SelectFileDevice(m_musicOnHoldCombo, m_MusicOnHold, wxT("Select Sound File for Hold"), false);
}


void OptionsDialog::ChangedAudioOnRing(wxCommandEvent & /*event*/)
{
  SelectFileDevice(m_audioOnRingCombo, m_AudioOnRing, wxT("Select Sound File for Hold"), false);
}


class SoundProgressDialog
{
  private:
    bool               m_playerTest;
    wxProgressDialog * m_dialog;
  public:
    SoundProgressDialog(bool playerTest) : m_playerTest(playerTest), m_dialog(NULL) { }
    ~SoundProgressDialog() { delete m_dialog; }

    PDECLARE_NOTIFIER(PSoundChannel, SoundProgressDialog, Progress);
    PNotifier GetNotifier() { return PCREATE_NOTIFIER(Progress); }
};

void SoundProgressDialog::Progress(PSoundChannel & channel, P_INT_PTR count)
{
  if (m_dialog != NULL && channel.IsOpen()) {
    const wxChar * msg;
    if (m_playerTest)
      msg = wxT("Testing player ...");
    else if (channel.GetDirection() == PSoundChannel::Recorder)
      msg = wxT("Testing recorder ...");
    else
      msg = wxT("Playing back test recording ...");

    m_dialog->Update(count, msg);
  }
  else {
    delete m_dialog;
    m_dialog = new wxProgressDialog(OpenPhoneString, wxEmptyString, count);
  }
}

void OptionsDialog::TestPlayer(wxCommandEvent & /*event*/)
{
  if (!wxDialog::TransferDataFromWindow())
    return;

  PSoundChannel::Params params(PSoundChannel::Player, MediaDeviceNameFromScreen(m_SoundPlayer));
  params.m_bufferCount = (m_SoundBufferTime*params.m_sampleRate/1000*2+params.m_bufferSize-1)/params.m_bufferSize;
  SoundProgressDialog progress(true);
  PwxString result = PSoundChannel::TestPlayer(params, progress.GetNotifier());
  if (result.StartsWith(wxT("Error")))
    wxMessageBox(result, OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);
  else
    wxMessageBox(result, OpenPhoneString, wxOK);
}


void OptionsDialog::TestRecorder(wxCommandEvent & /*event*/)
{
  if (!wxDialog::TransferDataFromWindow())
    return;

  PSoundChannel::Params recorderParams(PSoundChannel::Recorder, MediaDeviceNameFromScreen(m_SoundRecorder));
  PSoundChannel::Params playerPrams(PSoundChannel::Player, MediaDeviceNameFromScreen(m_SoundPlayer));
  playerPrams.m_bufferCount = 16;
  SoundProgressDialog progress(false);
  PwxString result = PSoundChannel::TestRecorder(recorderParams, playerPrams, progress.GetNotifier());
  if (result.StartsWith(wxT("Error")))
    wxMessageBox(result, OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);
  else
    wxMessageBox(result, OpenPhoneString, wxOK);
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

void OptionsDialog::AdjustVideoControls()
{
  PStringArray channelNames;
  PVideoInputDevice * grabber = PVideoInputDevice::CreateDeviceByName(MediaDeviceNameFromScreen(m_VideoGrabDevice));
  if (grabber != NULL) {
    channelNames = grabber->GetChannelNames();
    delete grabber;
  }

  int selection = m_VideoGrabSourceCtrl->GetSelection();
  m_VideoGrabSourceCtrl->Clear();
  m_VideoGrabSourceCtrl->Append(wxT("Default"));
  for (PINDEX i = 0; i < channelNames.GetSize(); ++i)
    m_VideoGrabSourceCtrl->Append(PwxString(channelNames[i]));
  m_VideoGrabSourceCtrl->SetSelection(selection);
  if (m_VideoGrabSourceCtrl->GetSelection() < 0)
    m_VideoGrabSourceCtrl->SetSelection(0);
}


void OptionsDialog::ChangeVideoGrabDevice(wxCommandEvent & /*event*/)
{
  SelectFileDevice(m_VideoGrabDeviceCtrl, m_VideoGrabDevice, wxT("Select Video File for Sending"), false);
  AdjustVideoControls();
}


void OptionsDialog::ChangeVideoWatermarkDevice(wxCommandEvent & /*event*/)
{
  SelectFileDevice(m_VideoWatermarkDeviceCtrl, m_VideoWatermarkDevice, wxT("Select Video File for Sending"), false);
}


void OptionsDialog::StopTestVideo()
{
  if (m_TestVideoGrabber != NULL)
    m_TestVideoGrabber->Close();
  if (m_TestVideoDisplay != NULL)
    m_TestVideoDisplay->Close();

  PThread::WaitAndDelete(m_TestVideoThread);

#if VIDEO_WINDOW_AUTO_DELETE
  delete m_TestVideoDisplay;
#endif
  m_TestVideoDisplay = NULL;

  delete m_TestVideoGrabber;
  m_TestVideoGrabber = NULL;

  m_TestVideoCapture->SetLabel(wxT("Test Video"));
}


void OptionsDialog::OnTestVideoEnded(wxCommandEvent & theEvent)
{
  StopTestVideo();
  if (theEvent.GetId())
    wxMessageBox(theEvent.GetString(), OpenPhoneString, wxOK);
  else
    wxMessageBox(theEvent.GetString(), OpenPhoneErrorString, wxOK|wxICON_EXCLAMATION);
}


void OptionsDialog::TestVideoCapture(wxCommandEvent & /*event*/)
{
  if (m_TestVideoThread != NULL)
    StopTestVideo();
  else if (wxDialog::TransferDataFromWindow()) {
    m_TestVideoCapture->SetLabel(wxT("Stop Video"));
    m_TestVideoThread = new PThreadObj<OptionsDialog>(*this, &OptionsDialog::TestVideoThreadMain, false, "TestVideo");
  }
}


void OptionsDialog::TestVideoThreadMain()
{
  wxCommandEvent theEvent(wxEvtTestVideoEnded);

  PVideoDevice::OpenArgs grabberArgs;
  grabberArgs.deviceName = MediaDeviceNameFromScreen(m_VideoGrabDevice);
  grabberArgs.videoFormat = (PVideoDevice::VideoFormat)m_VideoGrabFormat;
  grabberArgs.channelNumber = m_VideoGrabSource-1;
  grabberArgs.rate = m_VideoGrabFrameRate;
  PVideoFrameInfo::ParseSize(m_VideoGrabFrameSize, grabberArgs.width, grabberArgs.height);
  grabberArgs.flip = m_VideoFlipLocal;

  PVideoDevice::OpenArgs displayArgs;
  displayArgs.deviceName = psprintf(VIDEO_WINDOW_DEVICE" TITLE=\"Video Test\" X=%i Y=%i",
                                    m_manager.m_videoWindowInfo[1][0].x, m_manager.m_videoWindowInfo[1][0].y);
  displayArgs.width = grabberArgs.width;
  displayArgs.height = grabberArgs.height;

  if ((m_TestVideoDisplay = PVideoOutputDevice::CreateOpenedDevice(displayArgs, true)) == NULL)
    theEvent.SetString(wxT("Could not open video display."));
  else if ((m_TestVideoGrabber = PVideoInputDevice::CreateOpenedDevice(grabberArgs)) == NULL)
    theEvent.SetString(wxT("Could not open video capture."));
  else if (!m_TestVideoGrabber->Start())
    theEvent.SetString(wxT("Could not start video capture."));
  else {
    PSimpleTimer timer;
    PBYTEArray frame;
    unsigned frameCount = 0;
    while (m_TestVideoGrabber->GetFrame(frame) &&
           m_TestVideoDisplay->SetFrameData(0, 0,
                                            m_TestVideoGrabber->GetFrameWidth(),
                                            m_TestVideoGrabber->GetFrameHeight(),
                                            frame))
      frameCount++;
    PStringStream text;
    text << "Grabbed " << frameCount << " frames at " << (frameCount*1000.0/timer.GetElapsed().GetMilliSeconds()) << " fps.";
    theEvent.SetString(PwxString(text));
    theEvent.SetId(1);
  }

  theEvent.SetEventObject(this);
  GetEventHandler()->AddPendingEvent(theEvent);
}


void OptionsDialog::ChangedVideoOnHold(wxCommandEvent & /*event*/)
{
  SelectFileDevice(m_VideoOnHoldDeviceCtrl, m_VideoOnHold, wxT("Select Video File for Hold"), false);
}


void OptionsDialog::ChangedVideoOnRing(wxCommandEvent & /*event*/)
{
  SelectFileDevice(m_VideoOnRingDeviceCtrl, m_VideoOnRing, wxT("Select Video File for Ring"), false);
}


////////////////////////////////////////
// Fax fields

void OptionsDialog::BrowseFaxDirectory(wxCommandEvent & /*event*/)
{
  wxDirDialog dlg(this, wxT("Select Receive Directory for Faxes"), m_FaxReceiveDirectory);
  if (dlg.ShowModal() == wxID_OK) {
    m_FaxReceiveDirectory = dlg.GetPath();
    FindWindowByName(wxT("FaxReceiveDirectory"))->SetLabel(m_FaxReceiveDirectory);
  }
}


////////////////////////////////////////
// Presence fields

#if OPAL_HAS_PRESENCE

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
    if (presentity != NULL) {
      presentity->GetAttributes().Set(PresenceActiveKey, "Yes"); // OpenPhone extra attribute
      if (FillPresentityAttributes(presentity)) {
        delete (OpalPresentity *)m_Presentities->GetItemData(index);
        m_Presentities->SetItemPtrData(index, (wxUIntPtr)presentity);
        return;
      }
    }

    wxMessageBox(wxT("Cannot create presence identity ")+newAOR, OpenPhoneErrorString);
  }

  if (oldAOR == NewPresenceStr)
    RemovePresentity(evt);
  else
    evt.Veto();
}


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
    m_PresentityAttributes->SetCellValue(i, 0, value);
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

#endif // OPAL_HAS_PRESENCE


////////////////////////////////////////
// Codec fields

void OptionsDialog::AddCodec(wxCommandEvent & /*event*/)
{
  int insertionPoint = -1;
  wxArrayInt destinationSelections;
  if (m_selectedCodecs->GetSelections(destinationSelections) > 0)
    insertionPoint = destinationSelections[0];

  int pos = -1;
  while ((pos = m_allCodecs->GetNextItem(pos, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) >= 0) {
    MyMedia * media = (MyMedia *)m_allCodecs->GetItemData(pos);

    PwxString name = media->mediaFormat.GetName();
    if (m_selectedCodecs->FindString(name) < 0) {
      if (insertionPoint < 0)
        m_selectedCodecs->Append(name, media);
      else {
        m_selectedCodecs->InsertItems(1, &name, insertionPoint);
        m_selectedCodecs->SetClientData(insertionPoint, media);
      }
    }
    m_allCodecs->SetItemState(pos, 0, wxLIST_STATE_SELECTED);
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


void OptionsDialog::SelectedCodecToAdd(wxListEvent & /*event*/)
{
  m_AddCodec->Enable(m_allCodecs->GetSelectedItemCount() > 0);
}


void OptionsDialog::SelectedCodec(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  size_t count = m_selectedCodecs->GetSelections(selections);
  m_RemoveCodec->Enable(count > 0);
  m_MoveUpCodec->Enable(count == 1 && selections[0] > 0);
  m_MoveDownCodec->Enable(count == 1 && selections[0] < (int)m_selectedCodecs->GetCount()-1);

  m_codecOptions->DeleteAllItems();
  m_codecOptionValue->SetValue(wxEmptyString);
  m_codecOptionValue->Disable();
  m_CodecOptionValueLabel->Disable();
  m_SetDefaultCodecOption->Disable();

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


MyMedia * OptionsDialog::GetCodecOptionInfo(wxListItem & item, PwxString & optionName, PwxString & defaultValue)
{
  wxArrayInt selections;
  if (m_selectedCodecs->GetSelections(selections) != 1)
    return NULL;

  MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selections[0]);
  if (!PAssert(media != NULL, PLogicError))
    return NULL;

  item.m_itemId = m_codecOptions->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (item.m_itemId < 0)
    return NULL;

  item.m_mask = wxLIST_MASK_TEXT;
  m_codecOptions->GetItem(item);
  optionName = item.m_text;

  PString val;
  OpalMediaFormat(media->mediaFormat.GetName()).GetOptionValue(optionName, val);
  defaultValue = val;

  item.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_DATA;
  item.m_col = 1;
  m_codecOptions->GetItem(item);

  return media;
}


void OptionsDialog::SelectedCodecOption(wxListEvent & /*event*/)
{
  wxListItem item;
  PwxString optionName, defaultValue;
  if (GetCodecOptionInfo(item, optionName, defaultValue) == NULL)
    return;

  m_codecOptionValue->Enable(!item.m_data);
  m_CodecOptionValueLabel->Enable(!item.m_data);
  m_SetDefaultCodecOption->Enable(!item.m_data && item.m_text != defaultValue);
  if (!item.m_data)
    m_codecOptionValue->SetValue(item.m_text);
}


void OptionsDialog::DeselectedCodecOption(wxListEvent & /*event*/)
{
  m_codecOptionValue->SetValue(wxEmptyString);
  m_codecOptionValue->Disable();
  m_CodecOptionValueLabel->Disable();
  m_SetDefaultCodecOption->Disable();
}


void OptionsDialog::ChangedCodecOptionValue(wxCommandEvent & /*event*/)
{
  SetCodecOptionValue(false, m_codecOptionValue->GetValue());
}


void OptionsDialog::SetDefaultCodecOption(wxCommandEvent & /*event*/)
{
  SetCodecOptionValue(true, wxEmptyString);
}


void OptionsDialog::SetCodecOptionValue(bool useDefault, PwxString newValue)
{
  wxListItem item;
  PwxString optionName, defaultValue;
  MyMedia * media = GetCodecOptionInfo(item, optionName, defaultValue);
  if (media == NULL)
    return;

  if (useDefault)
    newValue = defaultValue;

  if (item.m_text == newValue)
    return;

  bool ok = media->mediaFormat.SetOptionValue(optionName, newValue);
  if (ok) {
    item.m_text = newValue;
    m_codecOptions->SetItem(item);
    m_codecOptionValue->SetValue(newValue);
  }

  m_CodecOptionValueError->Show(!ok);
}


////////////////////////////////////////
// H.323 fields

void OptionsDialog::MoreOptionsH323(wxCommandEvent & /*event*/)
{
  m_H323options.ShowModal();
}


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

void OptionsDialog::MoreOptionsSIP(wxCommandEvent & /*event*/)
{
  m_SIPoptions.ShowModal();
}


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
    cols.Add(item.m_text.c_str());
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
                                    wxEmptyString,
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

BEGIN_EVENT_TABLE(NetOptionsDialog, wxDialog)
#if OPAL_PTLIB_SSL
  EVT_RADIOBOX(XRCID("SignalingSecurity"), NetOptionsDialog::SignalingSecurityChanged)
  EVT_LISTBOX(XRCID("MediaCryptoSuites"),  NetOptionsDialog::MediaCryptoSuiteChanged)
  EVT_BUTTON(XRCID("MediaCryptoSuiteUp"),     NetOptionsDialog::MediaCryptoSuiteUp)
  EVT_BUTTON(XRCID("MediaCryptoSuiteDown"),   NetOptionsDialog::MediaCryptoSuiteDown)
#endif
END_EVENT_TABLE()

NetOptionsDialog::NetOptionsDialog(MyManager * manager, OpalRTPEndPoint * ep)
  : m_manager(*manager)
  , m_endpoint(ep)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("NetOptionsDialog"));

#if OPAL_PTLIB_SSL
  m_SignalingSecurity = 0;
  if (m_manager.FindEndPoint(ep->GetPrefixName()) != NULL)
    m_SignalingSecurity |= 1;
  if (m_manager.FindEndPoint(ep->GetPrefixName()+'s') != NULL)
    m_SignalingSecurity |= 2;
  --m_SignalingSecurity;

  FindWindowByNameAs(m_SignalingSecurityButtons, this, wxT("SignalingSecurity"))->SetValidator(wxGenericValidator(&m_SignalingSecurity));
  for (size_t i = 0; i < m_SignalingSecurityButtons->GetRowCount(); ++i) {
    wxString str = m_SignalingSecurityButtons->GetString(i);
    str.Replace(wxT("XXX"), PwxString(ep->GetPrefixName()));
    m_SignalingSecurityButtons->SetString(i, str);
  }

  FindWindowByNameAs(m_MediaCryptoSuiteUp, this, wxT("MediaCryptoSuiteUp"));
  FindWindowByNameAs(m_MediaCryptoSuiteDown, this, wxT("MediaCryptoSuiteDown"));
  FindWindowByNameAs(m_MediaCryptoSuites, this, wxT("MediaCryptoSuites"));
  InitSecurityFields();
#else
  FindWindowByName(wxT("SignalingSecurity"))->Disable();
  FindWindowByName(wxT("MediaCryptoSuites"))->Disable();
  FindWindowByName(wxT("MediaCryptoSuiteUp"))->Disable();
  FindWindowByName(wxT("MediaCryptoSuiteDown"))->Disable();
#endif // OPAL_PTLIB_SSL

  const PStringToString & defaultOptions = m_endpoint->GetDefaultStringOptions();
  PStringList optionNames = m_endpoint->GetAvailableStringOptions();

  FindWindowByNameAs(m_stringOptions, this, wxT("StringOptions"));
  m_stringOptions->CreateGrid(optionNames.GetSize(), 1);
  m_stringOptions->SetColLabelValue(0, wxT("Option Value"));
  m_stringOptions->SetColLabelSize(wxGRID_AUTOSIZE);
  m_stringOptions->AutoSizeColLabelSize(0);
  m_stringOptions->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_TOP);

  int row = 0;
  for (PStringList::iterator it = optionNames.begin(); it != optionNames.end(); ++it, ++row) {
    m_stringOptions->SetRowLabelValue(row, PwxString(*it));
    m_stringOptions->SetCellValue(row, 0, PwxString(defaultOptions(*it, DefaultAttributeValue)));
  }

  m_stringOptions->SetRowLabelSize(wxGRID_AUTOSIZE);
  m_stringOptions->SetColSize(0, wxGRID_AUTOSIZE);
  m_stringOptions->SetSize(m_stringOptions->GetRowLabelSize()+m_stringOptions->GetColSize(0)+4,
                           m_stringOptions->GetSize().GetHeight());
  Fit();
}


void NetOptionsDialog::SaveOptions(wxConfigBase * config, const wxChar * stringOptionsGroup)
{
  wxString oldPath = config->GetPath();
  config->DeleteGroup(stringOptionsGroup);
  config->SetPath(stringOptionsGroup);
  for (int row = 0; row < m_stringOptions->GetNumberRows(); ++row) {
    PwxString key = m_stringOptions->GetRowLabelValue(row);
    PwxString value = m_stringOptions->GetCellValue(row, 0);
    if (value == DefaultAttributeValue)
      m_endpoint->RemoveDefaultStringOption(key);
    else {
      config->Write(m_stringOptions->GetRowLabelValue(row), value);
      m_endpoint->SetDefaultStringOption(key, value);
    }
  }
  config->SetPath(oldPath);
}


#if OPAL_PTLIB_SSL
void NetOptionsDialog::InitSecurityFields()
{
  m_SignalingSecurityButtons->SetSelection(m_SignalingSecurity);

  PStringArray allMethods = m_endpoint->GetAllMediaCryptoSuites();
  PStringArray enabledMethods = m_endpoint->GetMediaCryptoSuites();

  m_MediaCryptoSuites->Clear();

  bool noneEnabled = true;
  for (PINDEX i = 0; i < enabledMethods.GetSize(); ++i) {
    OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(enabledMethods[i]);
    if (cryptoSuite != NULL) {
      m_MediaCryptoSuites->Check(m_MediaCryptoSuites->Append(PwxString(cryptoSuite->GetDescription())), m_SignalingSecurity > 0);
      noneEnabled = false;
    }
  }

  for (PINDEX i = 0; i < allMethods.GetSize(); ++i) {
    if (enabledMethods.GetValuesIndex(allMethods[i]) != P_MAX_INDEX)
      continue;

    OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(allMethods[i]);
    if (cryptoSuite == NULL)
      continue;

    m_MediaCryptoSuites->Check(m_MediaCryptoSuites->Append(PwxString(cryptoSuite->GetDescription())), false);
  }

  if (noneEnabled || m_SignalingSecurity == 0)
    m_MediaCryptoSuites->Check(m_MediaCryptoSuites->FindString(PwxString(OpalMediaCryptoSuite::ClearText())));

  m_MediaCryptoSuites->Enable(m_SignalingSecurity > 0 && allMethods.GetSize() > 1);
}


void NetOptionsDialog::SaveSecurityFields(wxConfigBase * config, const wxChar * securedSignalingKey, const wxChar * mediaCryptoSuitesKey)
{
  config->Write(securedSignalingKey, m_SignalingSecurity);

  PStringArray allMethods = m_endpoint->GetAllMediaCryptoSuites();
  PStringArray enabledMethods;
  for (unsigned item = 0; item < m_MediaCryptoSuites->GetCount(); ++item) {
    if (m_MediaCryptoSuites->IsChecked(item)) {
      PString description = PwxString(m_MediaCryptoSuites->GetString(item));
      for (PINDEX i = 0; i < allMethods.GetSize(); ++i) {
        OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(allMethods[i]);
        if (cryptoSuite != NULL && description == cryptoSuite->GetDescription()) {
          enabledMethods.AppendString(allMethods[i]);
          break;
        }
      }
    }
  }

  m_endpoint->SetMediaCryptoSuites(enabledMethods);
  PStringStream strm;
  strm << setfill(',') << enabledMethods;
  config->Write(mediaCryptoSuitesKey, PwxString(strm));

  m_manager.SetSignalingSecurity(m_endpoint, m_SignalingSecurity);
}


void NetOptionsDialog::SignalingSecurityChanged(wxCommandEvent & theEvent)
{
  if (!wxDialog::TransferDataFromWindow())
    return;
  wxRadioBox *signalingSecurityRadio;
  FindWindowByNameAs(signalingSecurityRadio, this, wxT("SignalingSecurity"));
  m_SignalingSecurity = signalingSecurityRadio->GetSelection();
  InitSecurityFields();
  MediaCryptoSuiteChanged(theEvent);
}


void NetOptionsDialog::MediaCryptoSuiteChanged(wxCommandEvent & /*event*/)
{
  int selection = m_MediaCryptoSuites->GetSelection();
  m_MediaCryptoSuiteUp->Enable(selection > 0);
  m_MediaCryptoSuiteDown->Enable((unsigned)selection < m_MediaCryptoSuites->GetCount() - 1);
}


void NetOptionsDialog::MediaCryptoSuiteUp(wxCommandEvent & /*event*/)
{
  MediaCryptoSuiteMove(-1);
}


void NetOptionsDialog::MediaCryptoSuiteDown(wxCommandEvent & /*event*/)
{
  MediaCryptoSuiteMove(+1);
}


void NetOptionsDialog::MediaCryptoSuiteMove(int dir)
{
  int selection = m_MediaCryptoSuites->GetSelection();
  if (dir < 0 ? (selection <= 0) : ((unsigned)selection >= m_MediaCryptoSuites->GetCount() - 1))
    return;

  wxString str = m_MediaCryptoSuites->GetString(selection);
  bool check = m_MediaCryptoSuites->IsChecked(selection);

  m_MediaCryptoSuites->Delete(selection);

  selection += dir;

  m_MediaCryptoSuites->Insert(str, selection);
  m_MediaCryptoSuites->Check(selection, check);

  wxCommandEvent dummy;
  MediaCryptoSuiteChanged(dummy);
}
#endif



///////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_PRESENCE

BEGIN_EVENT_TABLE(PresenceDialog, wxDialog)
END_EVENT_TABLE()

PresenceDialog::PresenceDialog(MyManager * manager)
  : m_manager(*manager)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("PresenceDialog"));

  wxComboBox * states;
  FindWindowByNameAs(states, this, wxT("PresenceState"))->SetValidator(wxGenericValidator(&m_status));

  wxChoice * addresses;
  FindWindowByNameAs(addresses, this, wxT("PresenceAddress"))->SetValidator(wxGenericValidator(&m_address));
  PStringList presences = manager->GetPresentities();
  for (PStringList::iterator i = presences.begin(); i != presences.end(); ++i) {
    PSafePtr<OpalPresentity> presentity = manager->GetPresentity(*i);
    if (presentity->GetAttributes().GetBoolean(PresenceActiveKey))
      addresses->AppendString(PwxString(*i));
  }
  if (addresses->GetCount() > 0)
    addresses->SetSelection(0);
  else {
    addresses->Disable();
    states->Disable();
    FindWindow(wxID_OK)->Disable();
  }
}


bool PresenceDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  PSafePtr<OpalPresentity> presentity = m_manager.GetPresentity(m_address.p_str());
  if (presentity == NULL) {
    wxMessageBox(wxT("Presence identity disappeared!"), OpenPhoneErrorString);
    return false;
  }

  presentity->SetLocalPresence(OpalPresenceInfo(m_status));

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(PresenceGroup);

  for (int presIndex = 0; ; ++presIndex) {
    wxString groupName;
    groupName.sprintf(wxT("%04u"), presIndex);
    if (!config->HasGroup(groupName))
      break;

    config->SetPath(groupName);
    PwxString aor;
    if (config->Read(PresenceAORKey, &aor) && aor == presentity->GetAOR()) {
      config->Write(LastPresenceStateKey, m_status);
      config->Flush();
      break;
    }

    config->SetPath(wxT(".."));
  }

  return true;
}

#endif // OPAL_HAS_PRESENCE


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(AudioDevicesDialog, wxDialog)
END_EVENT_TABLE()

AudioDevicesDialog::AudioDevicesDialog(MyManager * manager, const OpalPCSSConnection & connection)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("AudioDevicesDialog"));

  wxComboBox * combo;
  FindWindowByNameAs(combo, this, wxT("PlayDevice"))->SetValidator(wxGenericValidator(&m_playDevice));
  FillAudioDeviceComboBox(combo, PSoundChannel::Player);
  m_playDevice = MediaDeviceNameToScreen(connection.GetSoundChannelPlayDevice());

  FindWindowByNameAs(combo, this, wxT("RecordDevice"))->SetValidator(wxGenericValidator(&m_recordDevice));
  FillAudioDeviceComboBox(combo, PSoundChannel::Recorder);
  m_recordDevice = MediaDeviceNameToScreen(connection.GetSoundChannelRecordDevice());
}


PString AudioDevicesDialog::GetTransferAddress() const
{
  return "pc:" + MediaDeviceNameFromScreen(m_playDevice)
        + '\n' + MediaDeviceNameFromScreen(m_recordDevice);
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(VideoControlDialog, wxDialog)
END_EVENT_TABLE()

VideoControlDialog::VideoControlDialog(MyManager * manager, bool remote)
  : m_manager(*manager)
  , m_remote(remote)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("VideoControlDialog"));

  FindWindowByNameAs(m_TargetBitRate, this, wxT("VideoBitRate"));
  FindWindowByNameAs(m_FrameRate, this, wxT("FrameRate"));
  FindWindowByNameAs(m_TSTO, this, wxT("TSTO"));

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
      stream->ExecuteCommand(OpalMediaFlowControl(m_TargetBitRate->GetValue()*1000, OpalMediaType::Video()));
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
  PSafePtr<OpalConnection> connection = m_manager.GetNetworkConnection(PSafeReadOnly);
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

  wxComboBox * combo;
  FindWindowByNameAs(combo, this, wxT("VideoGrabDevice"))->SetValidator(wxGenericValidator(&m_device));
  PStringArray devices = PVideoInputDevice::GetDriversDeviceNames("*\t*");
  for (PINDEX i = 0; i < devices.GetSize(); i++)
    combo->Append(MediaDeviceNameToScreen(devices[i]));

  FindWindowByName(wxT("VideoPreview"))->SetValidator(wxGenericValidator(&m_preview));

  wxChoice * choice;
  FindWindowByNameAs(choice, this, wxT("VideoContentRole"))->SetValidator(wxGenericValidator(&m_contentRole));
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

  FindWindowByNameAs(m_ok, this, (int)wxID_OK)->Disable();

  FindWindowByNameAs(m_user, this, RegistrarUsernameKey)->SetValidator(wxGenericValidator(&m_info.m_User));
  FindWindowByNameAs(m_domain, this, RegistrarDomainKey)->SetValidator(wxGenericValidator(&m_info.m_Domain));

  wxChoice * choice;
  FindWindowByNameAs(choice, this, RegistrationTypeKey);
  for (PINDEX i = 0; i < RegistrationInfo::NumRegType; ++i)
    choice->Append(RegistrationInfoTable[i].m_name);
  choice->SetValidator(wxGenericValidator((int *)&m_info.m_Type));

  FindWindowByName(RegistrarUsedKey         )->SetValidator(wxGenericValidator(&m_info.m_Active));
  FindWindowByName(RegistrarContactKey      )->SetValidator(wxGenericValidator(&m_info.m_Contact));
  FindWindowByName(RegistrarAuthIDKey       )->SetValidator(wxGenericValidator(&m_info.m_AuthID));
  FindWindowByName(RegistrarPasswordKey     )->SetValidator(wxGenericValidator(&m_info.m_Password));
  FindWindowByName(RegistrarProxyKey        )->SetValidator(wxGenericValidator(&m_info.m_Proxy));
  FindWindowByName(RegistrarTimeToLiveKey   )->SetValidator(wxGenericValidator(&m_info.m_TimeToLive));
  FindWindowByName(RegistrarCompatibilityKey)->SetValidator(wxGenericValidator((int *)&m_info.m_Compatibility));
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
  EVT_BUTTON(wxID_OK, CallDialog::OnOK)
  EVT_TEXT(XRCID("Address"), CallDialog::OnAddressChange)
  EVT_COMBOBOX(XRCID("Address"), CallDialog::OnAddressChange)
END_EVENT_TABLE()

CallDialog::CallDialog(MyManager * manager, bool hideHandset, bool hideFax)
  : m_UseHandset(manager->HasHandset())
  , m_FaxMode(0)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("CallDialog"));

  FindWindowByNameAs(m_ok, this, (int)wxID_OK)->Disable();

  wxCheckBox * useHandset;
  FindWindowByNameAs(useHandset, this, wxT("UseHandset"))->SetValidator(wxGenericValidator(&m_UseHandset));
  if (hideHandset)
    useHandset->Hide();
  else
    useHandset->Enable(m_UseHandset);

  wxRadioBox * faxMode = FindWindowByNameAs(faxMode, this, wxT("FaxMode"));
  if (hideFax)
    faxMode->Hide();
  else
    faxMode->SetValidator(wxGenericValidator(&m_FaxMode));

  FindWindowByNameAs(m_AddressCtrl, this, wxT("Address"))->SetValidator(wxGenericValidator(&m_Address));

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
  if (info.m_opening)
    m_imDialogMap[info.m_conversationId] = new IMDialog(this, *info.m_context);
  else {
    m_imDialogMap[info.m_conversationId]->Destroy();
    m_imDialogMap.erase(info.m_conversationId);
  }
}


BEGIN_EVENT_TABLE(IMDialog, wxDialog)
  EVT_BUTTON(XRCID("Send"), IMDialog::OnSend)
  EVT_TEXT_ENTER(XRCID("EnteredText"), IMDialog::OnTextEnter)
  EVT_TEXT(XRCID("EnteredText"), IMDialog::OnText)
  EVT_CLOSE(IMDialog::OnCloseWindow)
  EVT_USER_COMMAND(wxEvtAsyncNotification,  IMDialog::OnEvtAsyncNotification)
END_EVENT_TABLE()

IMDialog::IMDialog(MyManager * manager, OpalIMContext & context)
  : m_manager(*manager)
  , m_context(context)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("IMDialog"));

  PStringStream str;
  str << "Conversation with " << context.GetRemoteURL();
  SetTitle(PwxString(str));

  FindWindowByNameAs(m_textArea, this, wxT("TextArea"));
  FindWindowByNameAs(m_enteredText, this, wxT("EnteredText"))->SetFocus();
  FindWindowByNameAs(m_compositionIndication, this, wxT("CompositionIndication"));

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
  PostAsyncNotification(*this);
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
  m_enteredText->SetValue(wxEmptyString);
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
    m_compositionIndication->SetLabel(wxEmptyString);
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
  EVT_BUTTON(wxID_OK, CallIMDialog::OnOK)
  EVT_CHOICE(XRCID("Presentity"), CallIMDialog::OnChanged)
  EVT_TEXT(XRCID("Address"), CallIMDialog::OnChanged)
END_EVENT_TABLE()

CallIMDialog::CallIMDialog(MyManager * manager)
  : m_manager(*manager)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("CallIMDialog"));

  FindWindowByNameAs(m_ok, this, (int)wxID_OK)->Disable();
  FindWindowByNameAs(m_AddressCtrl, this, wxT("Address"))->SetValidator(wxGenericValidator(&m_Address));

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

#if OPAL_HAS_PRESENCE
  PStringList presentities = manager->GetPresentities();
  wxChoice * choice;
  FindWindowByNameAs(choice, this, wxT("Presentity"))->SetValidator(wxGenericValidator(&m_Presentity));
  for (PStringList::iterator it = presentities.begin(); it != presentities.end(); ++it)
    choice->Append(PwxString(*it));
#endif // OPAL_HAS_PRESENCE
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
                             const PSafePtr<OpalCall> & call,
                             wxWindow * parent,
                             const wxChar * resource)
  : m_manager(manager)
  , m_call(call)
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

AnswerPanel::AnswerPanel(MyManager & manager, const PSafePtr<OpalCall> & call, wxWindow * parent)
  : CallPanelBase(manager, call, parent, wxT("AnswerPanel"))
{
}


void AnswerPanel::SetPartyNames(const PwxString & calling, const PwxString & called)
{
  FindWindowByName(wxT("CallingParty"))->SetLabel(calling);
  FindWindowByName(wxT("CalledParty"))->SetLabel(called);
  FindWindowByName(wxT("CalledParty"))->SetLabel(called);
}


void AnswerPanel::OnAnswer(wxCommandEvent & /*event*/)
{
  FindWindowByName(wxT("AnswerCall"))->Disable();
  if (m_manager.AnswerCall())
    FindWindowByName(wxT("RejectCall"))->SetLabel(wxT("Hang Up"));
  else
    FindWindowByName(wxT("RejectCall"))->Disable();
}


void AnswerPanel::OnReject(wxCommandEvent & /*event*/)
{
  FindWindowByName(wxT("AnswerCall"))->Disable();
  FindWindowByName(wxT("RejectCall"))->Disable();
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

CallingPanel::CallingPanel(MyManager & manager, const PSafePtr<OpalCall> & call, wxWindow * parent)
  : CallPanelBase(manager, call, parent, wxT("CallingPanel"))
{
}


void CallingPanel::OnHangUp(wxCommandEvent & /*event*/)
{
  FindWindowByName(wxT("HangUpCall"))->Disable();
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
  EVT_USER_COMMAND(wxEvtAsyncNotification, InCallPanel::OnEvtAsyncNotification)
END_EVENT_TABLE()

#define SET_CTRL_VAR(name) FindWindowByNameAs(m_##name, this, wxT(#name));

InCallPanel::InCallPanel(MyManager & manager, const PSafePtr<OpalCall> & call, wxWindow * parent)
  : CallPanelBase(manager, call, parent, wxT("InCallPanel"))
  , m_vuTimer(this, VU_UPDATE_TIMER_ID)
  , m_updateStatistics(0)
{
  SET_CTRL_VAR(Hold);
  SET_CTRL_VAR(Conference);
  SET_CTRL_VAR(SpeakerHandset);
  SET_CTRL_VAR(SpeakerMute);
  SET_CTRL_VAR(MicrophoneMute);
  SET_CTRL_VAR(SpeakerVolume);
  SET_CTRL_VAR(MicrophoneVolume);
  SET_CTRL_VAR(SpeakerGauge);
  SET_CTRL_VAR(MicrophoneGauge);
  SET_CTRL_VAR(CallTime);

#if OPAL_HAS_H281
  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
    for (unsigned dir = 0; dir < 2; ++dir) {
      static const wxChar * const feccNames[PVideoControlInfo::NumTypes][2] = {
        { wxT("PanLeft"),  wxT("PanRight")  },
        { wxT("TiltDown"), wxT("TiltUp")  },
        { wxT("ZoomWide"), wxT("ZoomTight") },
        { wxT("FocusIn"),  wxT("FocusOut")  },
      };
      wxButton * btn = FindWindowByNameAs(m_fecc[type][dir], this, feccNames[type][dir]);
      btn->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(InCallPanel::OnMouseFECC), NULL, this);
      btn->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(InCallPanel::OnMouseFECC), NULL, this);
      btn->Connect(wxEVT_ENTER_WINDOW, wxMouseEventHandler(InCallPanel::OnMouseFECC), NULL, this);
      btn->Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(InCallPanel::OnMouseFECC), NULL, this);
    }
  }
#endif // OPAL_HAS_H281

  OnHoldChanged(false);

  m_vuTimer.Start(250);

  m_pages[RxAudio].Init(this, RxAudio, OpalMediaType::Audio(), true );
  m_pages[TxAudio].Init(this, TxAudio, OpalMediaType::Audio(), false);
  m_pages[RxVideo].Init(this, RxVideo, OpalMediaType::Video(), true );
  m_pages[TxVideo].Init(this, TxVideo, OpalMediaType::Video(), false);
#if OPAL_FAX
  m_pages[RxFax  ].Init(this, RxFax  , OpalMediaType::Fax(),   false);
  m_pages[TxFax  ].Init(this, TxFax  , OpalMediaType::Fax(),   true);
#endif

  m_FirstTime = true;

  PSafePtr<OpalLocalConnection> connection = call->GetConnectionAs<OpalLocalConnection>();
  if (connection != NULL)
    connection->SetFarEndCameraCapabilityChangedNotifier(PCREATE_NOTIFIER(OnChangedFECC));
}


#if OPAL_HAS_H281
void InCallPanel::OnMouseFECC(wxMouseEvent & theEvent)
{
  theEvent.Skip();  // Still process the event normally

  bool start = theEvent.LeftDown() ||  (theEvent.Entering() && theEvent.LeftIsDown());
  if (!start && !theEvent.LeftUp() && !(theEvent.Leaving()  && theEvent.LeftIsDown()))
    return;

  PSafePtr<OpalLocalConnection> connection;
  if (m_manager.GetConnection(connection, PSafeReadOnly)) {
    for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
      for (unsigned dir = 0; dir < 2; ++dir) {
        if (theEvent.GetEventObject() == m_fecc[type][dir]) {
          PTRACE(4, "OpenPhone\tFar end camera control mouse " << (start ? "started" : "ended"));
          connection->FarEndCameraControl(type, start ? dir*2-1 : 0);
          return;
        }
      }
    }
  }

  PAssertAlways(PLogicError);
}


void InCallPanel::OnChangedFECC(OpalH281Client &, P_INT_PTR)
{
  OnStreamsChanged();
}
#endif // OPAL_HAS_H281


void InCallPanel::AsyncNotifierSignal()
{
  PostAsyncNotification(*this);
}


void InCallPanel::OnEvtAsyncNotification(wxCommandEvent &)
{
  while (AsyncNotifierExecute())
    ;
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

  OnStreamsChanged();

  return wxPanel::Show(show);
}


void InCallPanel::OnStreamsChanged()
{
  // Must do this before getting lock on OpalCall to avoid deadlock
  m_SpeakerHandset->Enable(m_manager.HasHandset());

  UpdateStatistics();

#if OPAL_FAX || OPAL_HAS_H281
  PSafePtr<OpalLocalConnection> connection;
  if (!m_manager.GetConnection(connection, PSafeReadOnly))
    return;
#endif

#if OPAL_FAX
  if (connection->GetMediaStream(OpalMediaType::Fax(), true) != NULL) {
    wxNotebook * book;
    FindWindowByNameAs(book, this, wxT("Statistics"))->SetSelection(RxFax);
  }
#endif

#if OPAL_HAS_H281
  if (connection->GetMediaStream(OpalH224MediaType(), true) != NULL) {
    PTRACE(4, "OpenPhone\tDetected H.224 channel, setting FECC capabilities");
    for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
      if (connection->FarEndCameraControl(type, 0)) {
        for (unsigned dir = 0; dir < 2; ++dir)
          m_fecc[type][dir]->Enable();
      }
    }
  }
  else {
    PTRACE(4, "OpenPhone\tNo H.224 channel available");
  }
#endif
}


void InCallPanel::OnHangUp(wxCommandEvent & /*event*/)
{
  LogWindow << "Hanging up \"" << *m_call << '"' << endl;
  m_call->Clear();
  FindWindowByName(wxT("HangUp"))->Disable();
  m_Hold->Disable();
  m_Conference->Disable();
}


void InCallPanel::OnHoldChanged(bool onHold)
{
  bool notConferenced = m_call->GetConnectionAs<OpalMixerConnection>() == NULL;

  m_Hold->SetLabel(onHold ? wxT("Retrieve") : wxT("Hold"));
  m_Hold->Enable(notConferenced);
  m_Conference->Enable(!onHold && !m_manager.m_callsOnHold.empty() && notConferenced);
}


void InCallPanel::OnHoldRetrieve(wxCommandEvent & /*event*/)
{
  if (m_call->IsOnHold()) {
    PSafePtr<OpalCall> activeCall = m_manager.GetCall(PSafeReference);
    if (activeCall == NULL) {
      if (!m_call->Retrieve())
        return;
    }
    else {
      m_manager.m_switchHoldToken = m_call->GetToken();
      if (!activeCall->Hold())
        return;
    }
  }
  else {
    if (!m_call->Hold())
      return;
  }

  m_Hold->SetLabel(wxT("In Progress"));
  m_Hold->Enable(false);
}


void InCallPanel::OnConference(wxCommandEvent & /*event*/)
{
  if (m_call == m_manager.m_activeCall) {
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
  PSafePtr<OpalLocalConnection> connection;
  if (m_manager.GetConnection(connection, PSafeReadOnly)) {
    connection->SetAudioVolume(isMicrophone, muted ? 0 : value);
    connection->SetAudioMute(isMicrophone, muted);
  }
}


static void SetGauge(wxGauge * gauge, int level)
{
  if (level < 0 || level > 32767) {
    gauge->Show(false);
    return;
  }
  gauge->Show();
  int val = (int)(100*log10(1.0 + 9.0*level/8192.0)); // Convert to logarithmic scale
  if (val < 0)
    val = 0;
  else if (val > 100)
    val = 100;
  gauge->SetValue(val);
}


void InCallPanel::OnUpdateVU(wxTimerEvent & WXUNUSED(event))
{
  if (IsShown()) {
    if (++m_updateStatistics % 8 == 0)
      UpdateStatistics();

    int micLevel = -1;
    int spkLevel = -1;
    PSafePtr<OpalLocalConnection> connection;
    if (m_manager.GetConnection(connection, PSafeReadOnly)) {
      spkLevel = connection->GetAudioSignalLevel(false);
      micLevel = connection->GetAudioSignalLevel(true);

      if (m_updateStatistics % 3 == 0) {
        PTime established = connection->GetPhaseTime(OpalConnection::EstablishedPhase);
        if (established.IsValid())
          m_CallTime->SetLabel(PwxString((PTime() - established).AsString(0, PTimeInterval::NormalFormat, 5)));
      }
    }

    SetGauge(m_SpeakerGauge, spkLevel);
    SetGauge(m_MicrophoneGauge, micLevel);
  }
}


void InCallPanel::UpdateStatistics()
{
  PSafePtr<OpalConnection> network = m_manager.GetNetworkConnection(PSafeReadOnly);
  if (network == NULL)
    return;

  PINDEX i;
  for (i = 0; i < RxFax; i++)
    m_pages[i].UpdateSession(network);

  PSafePtr<OpalLocalConnection> local;
  if (m_manager.GetConnection(local, PSafeReadOnly)) {
    for (; i < NumPages; i++)
      m_pages[i].UpdateSession(local);
  }
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
  m_printFormat = FindWindowByNameAs(m_staticText, panel, m_name)->GetLabel();
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
    value = 1000.0 * (frames - m_lastFrames) / (double)(tick - m_lastFrameTick).GetMilliSeconds(); // Ends up as frames/second
  else
    value = 0;

  m_lastFrameTick = tick;
  m_lastFrames = frames;

  return value;
}


void StatisticsField::Update(const OpalConnection & connection, const OpalMediaStream & stream, const OpalMediaStatistics & statistics)
{
  PwxString value;
  GetValue(connection, stream, statistics, value);
  m_staticText->SetLabel(value);
}


#define STATISTICS_FIELD_BEG(type, name) \
  class type##name##StatisticsField : public StatisticsField { \
  public: type##name##StatisticsField() : StatisticsField(wxT(#type) wxT(#name), type) { } \
    virtual ~type##name##StatisticsField() { } \
    virtual StatisticsField * Clone() const { return new type##name##StatisticsField(*this); } \
    virtual void GetValue(const OpalConnection & connection, const OpalMediaStream & stream, const OpalMediaStatistics & statistics, PwxString & value) {

#define STATISTICS_FIELD_END(type, name) \
    } } Static##type##name##StatisticsField;

#define STATISTICS_FIELD_BANDWIDTH(type, name, field) \
  STATISTICS_FIELD_BEG(type, name) \
    value.sprintf(m_printFormat, CalculateBandwidth(statistics.field)); \
  STATISTICS_FIELD_END(type, name)

#define STATISTICS_FIELD_SPRINTF(type, name, field) \
  STATISTICS_FIELD_BEG(type, name) \
    if (statistics.field >= 0) \
      value.sprintf(m_printFormat, statistics.field); \
    else \
      value.clear(); \
  STATISTICS_FIELD_END(type, name)

#define STATISTICS_FIELD_SPRINTF_NZ(type, name, field) \
  STATISTICS_FIELD_BEG(type, name) \
    if (statistics.field > 0) \
      value.sprintf(m_printFormat, statistics.field); \
    else \
      value.clear(); \
  STATISTICS_FIELD_END(type, name)

#ifdef _MSC_VER
#pragma warning(disable:4100)
#else
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

STATISTICS_FIELD_BANDWIDTH(RxAudio, Bandwidth, m_totalBytes)
STATISTICS_FIELD_SPRINTF(RxAudio, MinTime, m_minimumPacketTime)
STATISTICS_FIELD_SPRINTF(RxAudio, Bytes, m_totalBytes)
STATISTICS_FIELD_SPRINTF(RxAudio, AvgTime, m_averagePacketTime)
STATISTICS_FIELD_SPRINTF(RxAudio, Packets, m_totalPackets)
STATISTICS_FIELD_SPRINTF(RxAudio, MaxTime, m_maximumPacketTime)
STATISTICS_FIELD_SPRINTF(RxAudio, Lost, m_packetsLost)
STATISTICS_FIELD_SPRINTF_NZ(RxAudio, AvgJitter, m_averageJitter)
STATISTICS_FIELD_SPRINTF(RxAudio, OutOfOrder, m_packetsOutOfOrder)
STATISTICS_FIELD_SPRINTF_NZ(RxAudio, MaxJitter, m_maximumJitter)
STATISTICS_FIELD_SPRINTF(RxAudio, TooLate, m_packetsTooLate)
STATISTICS_FIELD_SPRINTF(RxAudio, Overruns, m_packetOverruns)
STATISTICS_FIELD_SPRINTF_NZ(RxAudio, JitterDelay, m_jitterBufferDelay)
STATISTICS_FIELD_BANDWIDTH(TxAudio, Bandwidth, m_totalBytes)
STATISTICS_FIELD_SPRINTF(TxAudio, MinTime, m_minimumPacketTime)
STATISTICS_FIELD_SPRINTF(TxAudio, Bytes, m_totalBytes)
STATISTICS_FIELD_SPRINTF(TxAudio, AvgTime, m_averagePacketTime)
STATISTICS_FIELD_SPRINTF(TxAudio, Packets, m_totalPackets)
STATISTICS_FIELD_SPRINTF(TxAudio, MaxTime, m_maximumPacketTime)
STATISTICS_FIELD_SPRINTF(TxAudio, Lost, m_packetsLost)
STATISTICS_FIELD_SPRINTF(TxAudio, AvgJitter, m_averageJitter)
STATISTICS_FIELD_SPRINTF_NZ(TxAudio, RoundTripTime, m_roundTripTime)
STATISTICS_FIELD_BANDWIDTH(RxVideo, Bandwidth, m_totalBytes)
STATISTICS_FIELD_SPRINTF(RxVideo, MinTime, m_minimumPacketTime)
STATISTICS_FIELD_SPRINTF(RxVideo, Bytes, m_totalBytes)
STATISTICS_FIELD_SPRINTF(RxVideo, AvgTime, m_averagePacketTime)
STATISTICS_FIELD_SPRINTF(RxVideo, Packets, m_totalPackets)
STATISTICS_FIELD_SPRINTF(RxVideo, MaxTime, m_maximumPacketTime)
STATISTICS_FIELD_SPRINTF(RxVideo, Lost, m_packetsLost)
STATISTICS_FIELD_SPRINTF_NZ(RxVideo, AvgJitter, m_averageJitter)
STATISTICS_FIELD_SPRINTF(RxVideo, OutOfOrder, m_packetsOutOfOrder)
STATISTICS_FIELD_SPRINTF(RxVideo, MaxJitter, m_maximumJitter)
STATISTICS_FIELD_SPRINTF(RxVideo, Frames, m_totalFrames)
STATISTICS_FIELD_SPRINTF(RxVideo, KeyFrames, m_keyFrames)

STATISTICS_FIELD_BEG(RxVideo, FrameRate)
  value.sprintf(m_printFormat, CalculateFrameRate(statistics.m_totalFrames));
STATISTICS_FIELD_END(RxVideo, FrameRate)

STATISTICS_FIELD_BEG(RxVideo, VFU)
  value.sprintf(m_printFormat, statistics.m_fullUpdateRequests+statistics.m_pictureLossRequests);
STATISTICS_FIELD_END(RxVideo, VFU)

STATISTICS_FIELD_BEG(RxVideo, Resolution)
  if (statistics.m_frameWidth > 0 && statistics.m_frameHeight > 0)
    value = PVideoFrameInfo::AsString(statistics.m_frameWidth, statistics.m_frameHeight).GetPointer();
STATISTICS_FIELD_END(RxVideo, Resolution)

STATISTICS_FIELD_BEG(RxVideo, Quality)
  if (statistics.m_videoQuality >= 0)
    value.sprintf(wxT("%i"), statistics.m_videoQuality);
STATISTICS_FIELD_END(RxVideo, Quality)

STATISTICS_FIELD_BANDWIDTH(TxVideo, Bandwidth, m_totalBytes)
STATISTICS_FIELD_SPRINTF(TxVideo, MinTime, m_minimumPacketTime)
STATISTICS_FIELD_SPRINTF(TxVideo, Bytes, m_totalBytes)
STATISTICS_FIELD_SPRINTF(TxVideo, AvgTime, m_averagePacketTime)
STATISTICS_FIELD_SPRINTF(TxVideo, Packets, m_totalPackets)
STATISTICS_FIELD_SPRINTF(TxVideo, Lost, m_packetsLost)
STATISTICS_FIELD_SPRINTF(TxVideo, MaxTime, m_maximumPacketTime)
STATISTICS_FIELD_SPRINTF_NZ(TxVideo, RoundTripTime, m_roundTripTime)
STATISTICS_FIELD_SPRINTF(TxVideo, Frames, m_totalFrames)
STATISTICS_FIELD_SPRINTF(TxVideo, KeyFrames, m_keyFrames)

STATISTICS_FIELD_BEG(TxVideo, FrameRate)
  value.sprintf(m_printFormat, CalculateFrameRate(statistics.m_totalFrames));
STATISTICS_FIELD_END(TxVideo, FrameRate)

STATISTICS_FIELD_BEG(TxVideo, VFU)
  value.sprintf(m_printFormat, statistics.m_fullUpdateRequests+statistics.m_pictureLossRequests);
STATISTICS_FIELD_END(TxVideo, VFU)

STATISTICS_FIELD_BEG(TxVideo, Resolution)
  if (statistics.m_frameWidth > 0 && statistics.m_frameHeight > 0)
    value = PVideoFrameInfo::AsString(statistics.m_frameWidth, statistics.m_frameHeight).GetPointer();
STATISTICS_FIELD_END(TxVideo, Resolution)

STATISTICS_FIELD_BEG(TxVideo, Quality)
  if (statistics.m_videoQuality >= 0)
    value.sprintf(wxT("%i"), statistics.m_videoQuality);
STATISTICS_FIELD_END(TxVideo, Quality)

STATISTICS_FIELD_BANDWIDTH(RxFax, Bandwidth, m_totalBytes)
STATISTICS_FIELD_SPRINTF(RxFax, Bytes, m_totalBytes)
STATISTICS_FIELD_SPRINTF(RxFax, Packets, m_totalPackets)

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

STATISTICS_FIELD_BANDWIDTH(TxFax, Bandwidth, m_totalBytes)
STATISTICS_FIELD_SPRINTF(TxFax, Bytes, m_totalBytes)
STATISTICS_FIELD_SPRINTF(TxFax, Packets, m_totalPackets)

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

  wxNotebook * book;
  m_window = FindWindowByNameAs(book, panel, wxT("Statistics"))->GetPage(page > RxTxFax ? RxTxFax : page);

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
    m_isActive = stream != NULL && stream->IsOpen();
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

  FindWindowByNameAs(m_ok, this, (int)wxID_OK);
  FindWindowByNameAs(m_nameCtrl, this, wxT("SpeedDialName"))->SetValidator(wxGenericValidator(&m_Name));
  FindWindowByNameAs(m_numberCtrl, this, wxT("SpeedDialNumber"))->SetValidator(wxGenericValidator(&m_Number));
  FindWindowByNameAs(m_addressCtrl, this, wxT("SpeedDialAddress"))->SetValidator(wxGenericValidator(&m_Address));

#if OPAL_HAS_PRESENCE
  wxChoice * choice;
  FindWindowByNameAs(choice, this, wxT("SpeedDialStateURL"))->SetValidator(wxGenericValidator(&m_Presentity));
  choice->Append(wxString());

  PStringList presentities = manager->GetPresentities();
  for (PStringList::iterator it = presentities.begin(); it != presentities.end(); ++it)
    choice->Append(PwxString(*it));
#else
  FindWindowByName(wxT("SpeedDialStateURL"))->Hide();
#endif // OPAL_HAS_PRESENCE

  FindWindowByName(wxT("SpeedDialDescription"))->SetValidator(wxGenericValidator(&m_Description));

  FindWindowByNameAs(m_inUse, this, wxT("SpeedDialInUse"));
  FindWindowByNameAs(m_ambiguous, this, wxT("SpeedDialAmbiguous"));
}


void SpeedDialDialog::OnChange(wxCommandEvent & WXUNUSED(event))
{
  wxString newName = m_nameCtrl->GetValue();
  bool inUse = newName.CmpNoCase(m_Name) != 0 && m_manager.HasSpeedDialName(newName);
  m_inUse->Show(inUse);

  m_ok->Enable(!inUse && !newName.IsEmpty() && newName.find_first_of(wxT("/\\:")) == wxString::npos && !m_addressCtrl->GetValue().IsEmpty());

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
  m_manager.PostEvent(wxEvtRinging, connection.GetCall().GetToken());
  return true;
}


PBoolean MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  PTime now;
  LogWindow << connection.GetRemotePartyName() << " is ringing on "
            << now.AsString("w h:mma") << " ..." << endl;
  return true;
}


PSoundChannel * MyPCSSEndPoint::CreateSoundChannel(const OpalPCSSConnection & connection,
                                                   const OpalMediaFormat & mediaFormat,
                                                   unsigned sessionID,
                                                   bool isSource)
{
  PSoundChannel * channel = OpalPCSSEndPoint::CreateSoundChannel(connection, mediaFormat, sessionID, isSource);
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


void MyH323EndPoint::OnGatekeeperStatus(H323Gatekeeper & gk, H323Gatekeeper::RegistrationFailReasons status)
{
  LogWindow << "H.323 registration: " << gk << " - " << status << endl;
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

  unsigned reasonClass = status.m_reason/100;
  if (reasonClass == 1 || (status.m_reRegistering && reasonClass == 2))
    return;

  PStringStream str;
  str << status << endl;
  LogWindow << str << endl;
  m_manager.SetTrayTipText(str);

  if (!status.m_wasRegistering)
    m_manager.StartRegistrations();
}


void MySIPEndPoint::OnSubscriptionStatus(const SubscriptionStatus & status)
{
  SIPEndPoint::OnSubscriptionStatus(status);

  unsigned reasonClass = status.m_reason/100;
  if (reasonClass == 1 || (status.m_reSubscribing && reasonClass == 2))
    return;

  LogWindow << status << endl;

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
