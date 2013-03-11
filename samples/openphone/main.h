/*
 * main.h
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

#ifndef _OpenPhone_MAIN_H
#define _OpenPhone_MAIN_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif


#include <wx/wx.h>
#include <wx/taskbar.h>
#include <wx/dataobj.h>

#include <opal/manager.h>

#ifndef OPAL_PTLIB_AUDIO
#error Cannot compile without PTLib sound channel support!
#endif

#include <ep/pcss.h>

#include <h323/h323.h>
#include <h323/gkclient.h>
#include <sip/sip.h>
#include <t38/t38proto.h>
#include <im/im_ep.h>
#include <ptlib/wxstring.h>
#include <ptlib/notifier_ext.h>


#include <list>


class MyManager;

class OpalLineEndPoint;
class OpalCapiEndPoint;
class OpalIVREndPoint;
class OpalMixerEndPoint;
class OpalFaxEndPoint;

class wxSplitterWindow;
class wxSplitterEvent;
class wxSpinCtrl;
class wxListCtrl;
class wxListEvent;
class wxCheckListBox;
class wxNotebook;
class wxGrid;
class wxGridEvent;
class wxConfigBase;
class wxImageList;


class MyPCSSEndPoint : public OpalPCSSEndPoint
{
  PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);

  public:
    MyPCSSEndPoint(MyManager & manager);

    virtual PSoundChannel * CreateSoundChannel(const OpalPCSSConnection & connection, const OpalMediaFormat & mediaFormat, PBoolean isSource);

  private:
    virtual PBoolean OnShowIncoming(const OpalPCSSConnection & connection);
    virtual PBoolean OnShowOutgoing(const OpalPCSSConnection & connection);

    MyManager & m_manager;
};

class IMDialog;

struct PresenceInfo {
  PString entity;
  PString status;
};

#if OPAL_H323
class MyH323EndPoint : public H323EndPoint
{
  public:
    MyH323EndPoint(MyManager & manager);

  private:
    virtual void OnGatekeeperStatus(H323Gatekeeper::RegistrationFailReasons);

    MyManager & m_manager;
};
#endif


#if OPAL_SIP
class MySIPEndPoint : public SIPEndPoint
{
  public:
    MySIPEndPoint(MyManager & manager);

  private:
    virtual void OnRegistrationStatus(
      const RegistrationStatus & status
    );
    virtual void OnSubscriptionStatus(
      const SubscriptionStatus & status
    );
    virtual void OnDialogInfoReceived(
      const SIPDialogNotification & info  ///< Information on dialog state change
    );

    MyManager & m_manager;
};
#endif // OPAL_SIP


#if OPAL_FAX
class MyFaxEndPoint : public OpalFaxEndPoint
{
    PCLASSINFO(MyFaxEndPoint, OpalFaxEndPoint)

  public:
    MyFaxEndPoint(OpalManager & manager) : OpalFaxEndPoint(manager) { }
    virtual void OnFaxCompleted(OpalFaxConnection & connection, bool timeout);
};
#endif // OPAL_FAX


class PresenceDialog : public wxDialog
{
  public:
    PresenceDialog(MyManager * manager);

  private:
    bool TransferDataFromWindow();

    MyManager & m_manager;
    PwxString   m_address;
    PwxString   m_status;

    DECLARE_EVENT_TABLE()
};


class AudioDevicesDialog : public wxDialog
{
  public:
    AudioDevicesDialog(MyManager * manager, const OpalPCSSConnection & connection);

    PString GetTransferAddress() const;

  private:
    PwxString     m_playDevice;
    PwxString     m_recordDevice;

    DECLARE_EVENT_TABLE()
};


class VideoControlDialog : public wxDialog
{
  public:
    VideoControlDialog(MyManager * manager, bool remote);

  private:
    bool TransferDataFromWindow();
    OpalMediaStreamPtr GetStream() const;

    MyManager & m_manager;
    bool        m_remote;
    wxSlider  * m_TargetBitRate;
    wxSlider  * m_FrameRate;
    wxSlider  * m_TSTO;

    DECLARE_EVENT_TABLE()
};


class StartVideoDialog : public wxDialog
{
  public:
    StartVideoDialog(MyManager * manager, bool secondary);

    PwxString m_device;
    bool      m_preview;
    int       m_contentRole;

  private:
    DECLARE_EVENT_TABLE()
};


class CallDialog : public wxDialog
{
  public:
    CallDialog(MyManager * manager, bool hideHandset, bool hideFax);

    PwxString m_Address;
    bool      m_UseHandset;
    int       m_FaxMode;

  private:
    void OnOK(wxCommandEvent & /*event*/);
    void OnAddressChange(wxCommandEvent & /*event*/);

    wxComboBox * m_AddressCtrl;
    wxButton   * m_ok;

    DECLARE_EVENT_TABLE()
};


#if OPAL_HAS_IM

class CallIMDialog : public wxDialog
{
  public:
    CallIMDialog(MyManager * manager);

    PwxString m_Presentity;
    PwxString m_Address;

  private:
    void OnOK(wxCommandEvent & /*event*/);
    void OnChanged(wxCommandEvent & /*event*/);

    MyManager  & m_manager;
    wxButton   * m_ok;
    wxComboBox * m_AddressCtrl;

    DECLARE_EVENT_TABLE()
};


class IMDialog : public wxDialog, PAsyncNotifierTarget
{
  public:
    IMDialog(MyManager * manager, OpalIMContext & context);
    ~IMDialog();

    void AddTextToScreen(const PwxString & text, bool fromUs);

  private:
    // PAsyncNotifierTarget override
    virtual void AsyncNotifierSignal();
    void OnEvtAsyncNotification(wxCommandEvent & /*event*/);

    PDECLARE_ASYNC_MessageDispositionNotifier(IMDialog, OnMessageDisposition);
    PDECLARE_ASYNC_MessageReceivedNotifier(IMDialog, OnMessageReceived);
    PDECLARE_ASYNC_CompositionIndicationNotifier(IMDialog, OnCompositionIndication);

    void OnSend(wxCommandEvent & /*event*/);
    void OnTextEnter(wxCommandEvent & /*event*/);
    void OnText(wxCommandEvent & /*event*/);
    void OnCloseWindow(wxCloseEvent & /*event*/);

    void SendCurrentText();
    void SendCompositionIndication();
    void ShowDisposition(OpalIMContext::MessageDisposition status);

    MyManager     & m_manager;
    OpalIMContext & m_context;

    wxTextCtrl   * m_enteredText;
    wxTextCtrl   * m_textArea;
    wxTextAttr     m_defaultStyle;
    wxTextAttr     m_ourStyle;
    wxTextAttr     m_theirStyle;
    wxStaticText * m_compositionIndication;

    DECLARE_EVENT_TABLE()
};

#endif // OPAL_HAS_IM


class CallPanelBase : public wxPanel
{
  protected:
    CallPanelBase(
      MyManager & manager,
      const PSafePtr<OpalCall> & call,
      wxWindow * parent,
      const wxChar * resource
    );

  public:
    PwxString GetToken() const { return m_call->GetToken(); }

  protected:
    MyManager & m_manager;
    PSafePtr<OpalCall> m_call;
};


class CallingPanel : public CallPanelBase
{
  public:
    CallingPanel(MyManager & manager, const PSafePtr<OpalCall> & call, wxWindow * parent);

  private:
    void OnHangUp(wxCommandEvent & /*event*/);

    DECLARE_EVENT_TABLE()
};


class AnswerPanel : public CallPanelBase
{
  public:
    AnswerPanel(MyManager & manager, const PSafePtr<OpalCall> & call, wxWindow * parent);

    void SetPartyNames(const PwxString & calling, const PwxString & called);

  private:
    void OnAnswer(wxCommandEvent & /*event*/);
    void OnReject(wxCommandEvent & /*event*/);
#if OPAL_FAX
    void OnChangeAnswerMode(wxCommandEvent & /*event*/);
#endif

    DECLARE_EVENT_TABLE()
};


class InCallPanel;

enum StatisticsPages {
  RxAudio,
  TxAudio,
  RxVideo,
  TxVideo,
  RxTxFax,
  RxFax = RxTxFax,
  TxFax,
  NumPages
};

struct StatisticsField
{
  StatisticsField(const wxChar * name, StatisticsPages page);
  virtual ~StatisticsField() { }
  void Init(wxWindow * panel);
  void Clear();
  double CalculateBandwidth(PUInt64 bytes);
  double CalculateFrameRate(DWORD frames);
  virtual StatisticsField * Clone() const = 0;
  virtual void Update(const OpalConnection & connection, const OpalMediaStream & stream, const OpalMediaStatistics & statistics);
  virtual void GetValue(const OpalConnection & connection, const OpalMediaStream & stream, const OpalMediaStatistics & statistics, wxString & value) = 0;

  const wxChar    * m_name;
  StatisticsPages m_page;
  wxStaticText  * m_staticText;
  wxString        m_printFormat;

  PTimeInterval   m_lastBandwidthTick;
  PUInt64         m_lastBytes;

  PTimeInterval   m_lastFrameTick;
  DWORD           m_lastFrames;
};

class StatisticsPage
{
  public:
    StatisticsPage();
    ~StatisticsPage();

    void Init(
      InCallPanel         * panel,
      StatisticsPages       page,
      const OpalMediaType & mediaType,
      bool                  receiver
    );

    void UpdateSession(const OpalConnection * connection);
    bool IsActive() const { return m_isActive; }

  private:
    StatisticsPages   m_page;
    OpalMediaType     m_mediaType;
    bool              m_receiver;
    bool              m_isActive;
    wxWindow        * m_window;

    vector<StatisticsField *> m_fields;

    StatisticsPage(const StatisticsPage &) { }
    void operator=(const StatisticsPage &) { }
};

class InCallPanel : public CallPanelBase
{
  public:
    InCallPanel(MyManager & manager, const PSafePtr<OpalCall> & call, wxWindow * parent);
    virtual bool Show(bool show = true);

    void OnStreamsChanged();
    void OnHoldChanged(bool onHold);

  private:
    void OnHangUp(wxCommandEvent & /*event*/);
    void OnHoldRetrieve(wxCommandEvent & /*event*/);
    void OnConference(wxCommandEvent & /*event*/);
    void OnSpeakerMute(wxCommandEvent & /*event*/);
    void OnMicrophoneMute(wxCommandEvent & /*event*/);
    void OnUserInput1(wxCommandEvent & /*event*/);
    void OnUserInput2(wxCommandEvent & /*event*/);
    void OnUserInput3(wxCommandEvent & /*event*/);
    void OnUserInput4(wxCommandEvent & /*event*/);
    void OnUserInput5(wxCommandEvent & /*event*/);
    void OnUserInput6(wxCommandEvent & /*event*/);
    void OnUserInput7(wxCommandEvent & /*event*/);
    void OnUserInput8(wxCommandEvent & /*event*/);
    void OnUserInput9(wxCommandEvent & /*event*/);
    void OnUserInput0(wxCommandEvent & /*event*/);
    void OnUserInputStar(wxCommandEvent & /*event*/);
    void OnUserInputHash(wxCommandEvent & /*event*/);
    void OnUserInputFlash(wxCommandEvent & /*event*/);
    void OnUpdateVU(wxTimerEvent & /*event*/);

    void SpeakerVolume(wxScrollEvent & /*event*/);
    void MicrophoneVolume(wxScrollEvent & /*event*/);
    void SetVolume(bool microphone, int value, bool muted);
    void UpdateStatistics();

    wxButton     * m_Hold;
    wxButton     * m_Conference;
    wxButton     * m_SpeakerHandset;
    wxCheckBox   * m_SpeakerMute;
    wxCheckBox   * m_MicrophoneMute;
    wxSlider     * m_SpeakerVolume;
    wxSlider     * m_MicrophoneVolume;
    wxGauge      * m_vuSpeaker;
    wxGauge      * m_vuMicrophone;
    wxTimer        m_vuTimer;
    wxStaticText * m_callTime;
    unsigned       m_updateStatistics;
    bool           m_FirstTime;

    StatisticsPage m_pages[NumPages];

    DECLARE_EVENT_TABLE()
};


struct SpeedDialInfo
{
  PwxString m_Name;
  PwxString m_Number;
  PwxString m_Address;
  PwxString m_Presentity;
  PwxString m_Description;

  bool operator<(const SpeedDialInfo & info) const { return m_Name < info.m_Name; }
};


class SpeedDialDialog : public wxDialog, public SpeedDialInfo
{
  public:
    SpeedDialDialog(MyManager *parent, const SpeedDialInfo & info);

  private:
    void OnChange(wxCommandEvent & /*event*/);

    MyManager & m_manager;

    wxButton     * m_ok;
    wxTextCtrl   * m_nameCtrl;
    wxTextCtrl   * m_numberCtrl;
    wxTextCtrl   * m_addressCtrl;
    wxStaticText * m_inUse;
    wxStaticText * m_ambiguous;

    DECLARE_EVENT_TABLE()
};


class RegistrationInfo
{
  public:
    // these must match the drop-down box
    // on the Registration/Subcription dialog box
    enum Types {
      Register,
      SubscribeMWI,
      SubcribePresence,
      SubscribeMLA,
      PublishPresence,
      PresenceWatcher,
      OthersRegistration,
      NumRegType
    };

    RegistrationInfo();

    bool operator==(const RegistrationInfo & other) const;

    bool Read(wxConfigBase & config);
    void Write(wxConfigBase & config);
    bool Start(SIPEndPoint & sipEP);
    bool Stop(SIPEndPoint & sipEP);

    Types     m_Type;
    bool      m_Active;
    PwxString m_User;
    PwxString m_Domain;
    PwxString m_Contact;
    PwxString m_InstanceId;
    PwxString m_AuthID;
    PwxString m_Password;
    int       m_TimeToLive;
    PwxString m_Proxy;
    SIPRegister::CompatibilityModes m_Compatibility;

    PString   m_aor;
};

typedef list<RegistrationInfo> RegistrationList;


class RegistrationDialog : public wxDialog
{
  public:
    RegistrationDialog(wxDialog *parent, const RegistrationInfo * info);

    const RegistrationInfo & GetInfo() const { return m_info; }

  private:
    DECLARE_EVENT_TABLE()

    void Changed(wxCommandEvent & /*event*/);

    wxButton       * m_ok;
    wxTextCtrl     * m_user;
    wxTextCtrl     * m_domain;
    RegistrationInfo m_info;
};


class OptionsDialog : public wxDialog
{
  public:
    OptionsDialog(MyManager *parent);
    ~OptionsDialog();
    virtual bool TransferDataFromWindow();

  private:
    MyManager & m_manager;

    ////////////////////////////////////////
    // General fields
    PwxString m_Username;
    PwxString m_DisplayName;
    PwxString m_RingSoundDeviceName;
    PwxString m_RingSoundFileName;
    bool      m_AutoAnswer;
    PwxString m_VendorName;
    PwxString m_ProductName;
    PwxString m_ProductVersion;
    bool      m_HideMinimised;

    void BrowseSoundFile(wxCommandEvent & /*event*/);
    void PlaySoundFile(wxCommandEvent & /*event*/);

    ////////////////////////////////////////
    // Networking fields
    wxString        m_RxBandwidth;
    wxString        m_TxBandwidth;
    int             m_DiffServAudio;
    int             m_DiffServVideo;
    int             m_MaxRtpPayloadSize;
#if OPAL_PTLIB_SSL
    PwxString       m_CertificateAuthority;
    PwxString       m_LocalCertificate;
    PwxString       m_PrivateKey;
#endif
    int             m_TCPPortBase;
    int             m_TCPPortMax;
    int             m_UDPPortBase;
    int             m_UDPPortMax;
    int             m_RTPPortBase;
    int             m_RTPPortMax;
    wxRadioButton * m_NoNATUsedRadio;
    wxRadioButton * m_NATRouterRadio;
    wxRadioButton * m_STUNServerRadio;
    PwxString       m_NATRouter;
    wxTextCtrl    * m_NATRouterWnd;
    PwxString       m_STUNServer;
    wxTextCtrl    * m_STUNServerWnd;
    wxListBox     * m_LocalInterfaces;
    wxChoice      * m_InterfaceProtocol;
    wxComboBox    * m_InterfaceAddress;
    wxTextCtrl    * m_InterfacePort;
    wxButton      * m_AddInterface;
    wxButton      * m_RemoveInterface;
    void BandwidthClass(wxCommandEvent & /*event*/);

#if OPAL_PTLIB_SSL
    void FindCertificateAuthority(wxCommandEvent & /*event*/);
    void FindLocalCertificate(wxCommandEvent & /*event*/);
    void FindPrivateKey(wxCommandEvent & /*event*/);
#endif
    void NATHandling(wxCommandEvent & /*event*/);
    void SelectedLocalInterface(wxCommandEvent & /*event*/);
    void ChangedInterfaceInfo(wxCommandEvent & /*event*/);
    void AddInterface(wxCommandEvent & /*event*/);
    void RemoveInterface(wxCommandEvent & /*event*/);

    ////////////////////////////////////////
    // Sound fields
    PwxString m_SoundPlayer;
    PwxString m_SoundRecorder;
    int       m_SoundBufferTime;
#if OPAL_AEC
    bool      m_EchoCancellation;
#endif
    PwxString m_LineInterfaceDevice;
    int       m_AEC;
    PwxString m_Country;
    int       m_MinJitter;
    int       m_MaxJitter;
    int       m_SilenceSuppression;
    int       m_SilenceThreshold;
    int       m_SignalDeadband;
    int       m_SilenceDeadband;
    bool      m_DisableDetectInBandDTMF;

    wxComboBox * m_soundPlayerCombo;
    wxComboBox * m_soundRecorderCombo;
    wxComboBox * m_selectedLID;
    wxChoice   * m_selectedAEC;
    wxComboBox * m_selectedCountry;

    void ChangedSoundPlayer(wxCommandEvent & /*event*/);
    void ChangedSoundRecorder(wxCommandEvent & /*event*/);
    void SelectedLID(wxCommandEvent & /*event*/);

    ////////////////////////////////////////
    // Video fields
    PwxString m_VideoGrabDevice;
    int       m_VideoGrabFormat;
    int       m_VideoGrabSource;
    int       m_VideoGrabFrameRate;
    PwxString m_VideoGrabFrameSize;
    bool      m_VideoGrabPreview;
    bool      m_VideoFlipLocal;
    bool      m_VideoAutoTransmit;
    bool      m_VideoAutoReceive;
    bool      m_VideoFlipRemote;
    PwxString m_VideoMinFrameSize;
    PwxString m_VideoMaxFrameSize;
    int       m_VideoGrabBitRate;
    int       m_VideoMaxBitRate;

    wxComboBox * m_videoGrabDeviceCombo;
    wxChoice   * m_videoSourceChoice;

    wxButton           * m_TestVideoCapture;
    PThread            * m_TestVideoThread;
    PVideoInputDevice  * m_TestVideoGrabber;
    PVideoOutputDevice * m_TestVideoDisplay;

    void AdjustVideoControls(const PwxString & device);
    void ChangeVideoGrabDevice(wxCommandEvent & /*event*/);
    void TestVideoCapture(wxCommandEvent & /*event*/);
    void TestVideoThreadMain();
    void StopTestVideo();

    ////////////////////////////////////////
    // Fax fields
    PwxString m_FaxStationIdentifier;
    PwxString m_FaxReceiveDirectory;
    int       m_FaxAutoAnswerMode;
    void BrowseFaxDirectory(wxCommandEvent & /*event*/);

    ////////////////////////////////////////
    // IVR fields
#if OPAL_IVR
    PwxString m_IVRScript;
#endif
    int       m_AudioRecordingMode;
    PwxString m_AudioRecordingFormat;
    int       m_VideoRecordingMode;
    PwxString m_VideoRecordingSize;

    ////////////////////////////////////////
    // Presence fields
    wxListCtrl * m_Presentities;
    wxGrid     * m_PresentityAttributes;
    wxButton   * m_AddPresentity;
    wxButton   * m_RemovePresentity;
    void AddPresentity(wxCommandEvent & /*event*/);
    void RemovePresentity(wxCommandEvent & /*event*/);
    void SelectedPresentity(wxListEvent & /*event*/);
    void DeselectedPresentity(wxListEvent & /*event*/);
    void EditedPresentity(wxListEvent & /*event*/);
    bool FillPresentityAttributes(OpalPresentity * presentity);
    void ChangedPresentityAttribute(wxGridEvent & /*event*/);

    ////////////////////////////////////////
    // Codec fields
    wxButton     * m_AddCodec;
    wxButton     * m_RemoveCodec;
    wxButton     * m_MoveUpCodec;
    wxButton     * m_MoveDownCodec;
    wxListBox    * m_allCodecs;
    wxListBox    * m_selectedCodecs;
    wxListCtrl   * m_codecOptions;
    wxTextCtrl   * m_codecOptionValue;
    wxStaticText * m_CodecOptionValueLabel;
    wxStaticText * m_CodecOptionValueError;

    void AddCodec(wxCommandEvent & /*event*/);
    void RemoveCodec(wxCommandEvent & /*event*/);
    void MoveUpCodec(wxCommandEvent & /*event*/);
    void MoveDownCodec(wxCommandEvent & /*event*/);
    void SelectedCodecToAdd(wxCommandEvent & /*event*/);
    void SelectedCodec(wxCommandEvent & /*event*/);
    void SelectedCodecOption(wxListEvent & /*event*/);
    void DeselectedCodecOption(wxListEvent & /*event*/);
    void ChangedCodecOptionValue(wxCommandEvent & /*event*/);

    ////////////////////////////////////////
    // H.323 fields
    int          m_GatekeeperMode;
    PwxString    m_GatekeeperAddress;
    PwxString    m_GatekeeperIdentifier;
    int          m_GatekeeperTTL;
    PwxString    m_GatekeeperLogin;
    PwxString    m_GatekeeperPassword;

    int          m_DTMFSendMode;
    int          m_CallIntrusionProtectionLevel;
    bool         m_DisableFastStart;
    bool         m_DisableH245Tunneling;
    bool         m_DisableH245inSETUP;
    int          m_ExtendedVideoRoles;
    bool         m_EnableH239Control;

    wxListBox  * m_Aliases;
    wxTextCtrl * m_NewAlias;
    wxButton   * m_AddAlias;
    wxButton   * m_RemoveAlias;

    void SelectedAlias(wxCommandEvent & /*event*/);
    void ChangedNewAlias(wxCommandEvent & /*event*/);
    void AddAlias(wxCommandEvent & /*event*/);
    void RemoveAlias(wxCommandEvent & /*event*/);

#if OPAL_PTLIB_SSL
    int              m_H323SignalingSecurity;
    wxCheckListBox * m_H323MediaCryptoSuites;
    wxButton       * m_H323MediaCryptoSuiteUp;
    wxButton       * m_H323MediaCryptoSuiteDown;
    void H323SignalingSecurityChanged(wxCommandEvent & /*event*/);
    void H323MediaCryptoSuiteChanged(wxCommandEvent & /*event*/);
    void H323MediaCryptoSuiteUp(wxCommandEvent & /*event*/);
    void H323MediaCryptoSuiteDown(wxCommandEvent & /*event*/);
#endif

    ////////////////////////////////////////
    // SIP fields
    bool      m_SIPProxyUsed;
    PwxString m_SIPProxy;
    PwxString m_SIPProxyUsername;
    PwxString m_SIPProxyPassword;

    int       m_LineAppearanceCode;
    int       m_SIPUserInputMode;
    int       m_SIPPRACKMode;

    wxListCtrl * m_Registrations;
    int          m_SelectedRegistration;
    wxButton   * m_ChangeRegistration;
    wxButton   * m_RemoveRegistration;
    wxButton   * m_MoveUpRegistration;
    wxButton   * m_MoveDownRegistration;

    void RegistrationToList(bool create, RegistrationInfo * registration, int position);
    void AddRegistration(wxCommandEvent & /*event*/);
    void ChangeRegistration(wxCommandEvent & /*event*/);
    void RemoveRegistration(wxCommandEvent & /*event*/);
    void MoveUpRegistration(wxCommandEvent & /*event*/);
    void MoveDownRegistration(wxCommandEvent & /*event*/);
    void MoveRegistration(int delta);
    void SelectedRegistration(wxListEvent & /*event*/);
    void DeselectedRegistration(wxListEvent & /*event*/);
    void ActivateRegistration(wxListEvent & /*event*/);

#if OPAL_PTLIB_SSL
    int              m_SIPSignalingSecurity;
    wxCheckListBox * m_SIPMediaCryptoSuites;
    wxButton       * m_SIPMediaCryptoSuiteUp;
    wxButton       * m_SIPMediaCryptoSuiteDown;
    void SIPSignalingSecurityChanged(wxCommandEvent & /*event*/);
    void SIPMediaCryptoSuiteChanged(wxCommandEvent & /*event*/);
    void SIPMediaCryptoSuiteUp(wxCommandEvent & /*event*/);
    void SIPMediaCryptoSuiteDown(wxCommandEvent & /*event*/);
#endif

    ////////////////////////////////////////
    // Routing fields
    wxString     m_ForwardingAddress;
    int          m_ForwardingTimeout;
    PwxString    m_telURI;
    wxListCtrl * m_Routes;
    int          m_SelectedRoute;
    wxComboBox * m_RouteSource;
    wxTextCtrl * m_RouteDevice;
    wxTextCtrl * m_RoutePattern;
    wxTextCtrl * m_RouteDestination;
    wxButton   * m_AddRoute;
    wxButton   * m_RemoveRoute;
    wxButton   * m_MoveUpRoute;
    wxButton   * m_MoveDownRoute;

    void AddRoute(wxCommandEvent & /*event*/);
    void RemoveRoute(wxCommandEvent & /*event*/);
    void MoveUpRoute(wxCommandEvent & /*event*/);
    void MoveDownRoute(wxCommandEvent & /*event*/);
    void SelectedRoute(wxListEvent & /*event*/);
    void DeselectedRoute(wxListEvent & /*event*/);
    void ChangedRouteInfo(wxCommandEvent & /*event*/);
    void RestoreDefaultRoutes(wxCommandEvent & /*event*/);
    void AddRouteTableEntry(OpalManager::RouteEntry entry);

#if PTRACING
    ////////////////////////////////////////
    // Tracing fields
    bool      m_EnableTracing;
    int       m_TraceLevelThreshold;
    bool      m_TraceLevelNumber;
    bool      m_TraceFileLine;
    bool      m_TraceBlocks;
    bool      m_TraceDateTime;
    bool      m_TraceTimestamp;
    bool      m_TraceThreadName;
    bool      m_TraceThreadAddress;
    bool      m_TraceObjectInstance;
    bool      m_TraceContextId;
    PwxString m_TraceFileName;

    void BrowseTraceFile(wxCommandEvent & /*event*/);
#endif

    DECLARE_EVENT_TABLE()
};


class MyMedia
{
public:
  MyMedia();
  MyMedia(const OpalMediaFormat & format);

  bool operator<(const MyMedia & other) { return preferenceOrder < other.preferenceOrder; }

  OpalMediaFormat mediaFormat;
  const wxChar  * validProtocols;
  int             preferenceOrder;
};

typedef std::list<MyMedia> MyMediaList;


class MyTaskBarIcon : public wxTaskBarIcon
{
  public:
    MyTaskBarIcon(MyManager & manager);

    virtual wxMenu * CreatePopupMenu();
    virtual void OnDoubleClick(wxTaskBarIconEvent & /*event*/);

  protected:
    virtual bool ProcessEvent(wxEvent & /*event*/);

    MyManager & m_manager;

    DECLARE_EVENT_TABLE()
};


class MyManager : public wxFrame, public OpalManager, public PAsyncNotifierTarget
{
  public:
    MyManager();
    ~MyManager();

    bool Initialise(bool startMinimised);

    void SetTrayTipText(const PwxString & text);
    void SetBalloonText(const PwxString & text);
    wxMenu * CreateTrayMenu();

    bool HasSpeedDialName(const wxString & name) const;
    bool HasSpeedDialNumber(const wxString & number, const wxString & ignore) const;

    void MakeCall(const PwxString & address, const PwxString & local = wxEmptyString, OpalConnection::StringOptions * options = NULL);
    void AnswerCall();
    void RejectCall();
    void HangUpCall();
    void SendUserInput(char tone);

    PSafePtr<OpalCall>       GetCall(PSafetyMode mode);
    PSafePtr<OpalConnection> GetConnection(bool user, PSafetyMode mode);

    bool HasHandset() const;

#if OPAL_FAX
    enum FaxAnswerModes {
      AnswerVoice,
      AnswerFax,
      AnswerDetect
    };
    FaxAnswerModes m_currentAnswerMode;
    FaxAnswerModes m_defaultAnswerMode;
    void SwitchToFax();
#endif // OPAL_FAX

    MyPCSSEndPoint & GetPCSSEP() { return *pcssEP; }

    void PostEvent(
      const wxEventType & type,
      unsigned id,
      const PString & str = PString::Empty(),
      const void * data = NULL
    );

  private:
    // PAsyncNotifierTarget override
    virtual void AsyncNotifierSignal();
    void OnEvtAsyncNotification(wxCommandEvent & /*event*/);

    // OpalManager overrides
    virtual PBoolean OnIncomingConnection(
      OpalConnection & connection,   ///<  Connection that is calling
      unsigned options,              ///<  options for new connection (can't use default as overrides will fail)
      OpalConnection::StringOptions * stringOptions
    );
    virtual void OnEstablishedCall(
      OpalCall & call   /// Call that was completed
    );
    virtual void OnClearedCall(
      OpalCall & call   /// Connection that was established
    );
    virtual void OnHold(
      OpalConnection & connection,   ///<  Connection that was held/retrieved
      bool fromRemote,               ///<  Indicates remote has held local connection
      bool onHold                    ///<  Indicates have just been held/retrieved.
    );
    virtual bool OnTransferNotify(
      OpalConnection & connection,  ///< Connection being transferred.
      const PStringToString & info  ///< Information on the transfer
    );
    virtual void AdjustMediaFormats(
      bool local,                         ///<  Media formats a local ones to be presented to remote
      const OpalConnection & connection,  ///<  Connection that is about to use formats
      OpalMediaFormatList & mediaFormats  ///<  Media formats to use
    ) const;
    virtual PBoolean OnOpenMediaStream(
      OpalConnection & connection,  /// Connection that owns the media stream
      OpalMediaStream & stream    /// New media stream being opened
    );
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     ///<  Stream being closed
    );
    virtual void OnUserInputString(
      OpalConnection & connection,  /// Connection input has come from
      const PString & value         /// String value of indication
    );
#if OPAL_FAX
    virtual void OnUserInputTone(
      OpalConnection & connection,  ///<  Connection input has come from
      char tone,                    ///<  Tone received
      int duration                  ///<  Duration of tone
    );
#endif // OPAL_FAX
    virtual PString ReadUserInput(
      OpalConnection & connection,        ///<  Connection to read input from
      const char * terminators = "#\r\n", ///<  Characters that can terminte input
      unsigned lastDigitTimeout = 4,      ///<  Timeout on last digit in string
      unsigned firstDigitTimeout = 30     ///<  Timeout on receiving any digits
    );
    virtual PBoolean CreateVideoInputDevice(
      const OpalConnection & connection,    ///<  Connection needing created video device
      const OpalMediaFormat & mediaFormat,  ///<  Media format for stream
      PVideoInputDevice * & device,         ///<  Created device
      PBoolean & autoDelete                     ///<  Flag for auto delete device
    );
    virtual PBoolean CreateVideoOutputDevice(
      const OpalConnection & connection,    ///<  Connection needing created video device
      const OpalMediaFormat & mediaFormat,  ///<  Media format for stream
      PBoolean preview,                         ///<  Flag indicating is a preview output
      PVideoOutputDevice * & device,        ///<  Created device
      PBoolean & autoDelete                     ///<  Flag for auto delete device
    );

    void OnClose(wxCloseEvent & /*event*/);
    void OnIconize(wxIconizeEvent & /*event*/);
    void OnLogMessage(wxCommandEvent & /*event*/);
    void OnAdjustMenus(wxMenuEvent & /*event*/);
    void OnStateChange(wxCommandEvent & /*event*/);
    void OnStreamsChanged(wxCommandEvent & /*event*/);

    void OnMenuQuit(wxCommandEvent & /*event*/);
    void OnMenuAbout(wxCommandEvent & /*event*/);
    void OnMenuCall(wxCommandEvent & /*event*/);
    void OnMenuCallLastDialed(wxCommandEvent & /*event*/);
    void OnMenuCallLastReceived(wxCommandEvent & /*event*/);
    void OnCallSpeedDialAudio(wxCommandEvent & /*event*/);
    void OnCallSpeedDialHandset(wxCommandEvent & /*event*/);
    void OnSendFax(wxCommandEvent & /*event*/);
    void OnSendFaxSpeedDial(wxCommandEvent & /*event*/);
    void OnMenuAnswer(wxCommandEvent & /*event*/);
    void OnMenuHangUp(wxCommandEvent & /*event*/);
    void OnNewSpeedDial(wxCommandEvent & /*event*/);
    void OnViewLarge(wxCommandEvent & /*event*/);
    void OnViewSmall(wxCommandEvent & /*event*/);
    void OnViewList(wxCommandEvent & /*event*/);
    void OnViewDetails(wxCommandEvent & /*event*/);
    void OnEditSpeedDial(wxCommandEvent & /*event*/);
    void OnCutSpeedDial(wxCommandEvent & /*event*/);
    void OnCopySpeedDial(wxCommandEvent & /*event*/);
    void OnPasteSpeedDial(wxCommandEvent & /*event*/);
    void OnDeleteSpeedDial(wxCommandEvent & /*event*/);
    void OnOptions(wxCommandEvent & /*event*/);
    void OnRequestHold(wxCommandEvent & /*event*/);
    void OnRetrieve(wxCommandEvent & /*event*/);
    void OnConference(wxCommandEvent & /*event*/);
    void OnTransfer(wxCommandEvent & /*event*/);
    void OnStartRecording(wxCommandEvent & /*event*/);
    void OnStopRecording(wxCommandEvent & /*event*/);
    void OnSendAudioFile(wxCommandEvent & /*event*/);
    void OnAudioDevicePair(wxCommandEvent & /*event*/);
    void OnAudioDevicePreset(wxCommandEvent & /*event*/);
    void OnNewCodec(wxCommandEvent & /*event*/);
    void OnStartVideo(wxCommandEvent & /*event*/);
    void OnStopVideo(wxCommandEvent & /*event*/);
    void OnSendVFU(wxCommandEvent & /*event*/);
    void OnSendIntra(wxCommandEvent & /*event*/);
    void OnTxVideoControl(wxCommandEvent & /*event*/);
    void OnRxVideoControl(wxCommandEvent & /*event*/);
    void OnDefVidWinPos(wxCommandEvent & /*event*/);
    void OnSashPositioned(wxSplitterEvent & /*event*/);
    void OnSpeedDialActivated(wxListEvent & /*event*/);
    void OnSpeedDialColumnResize(wxListEvent & /*event*/);
    void OnSpeedDialRightClick(wxListEvent & /*event*/);
    void OnSpeedDialEndEdit(wxListEvent & /*event*/);

    void OnMyPresence(wxCommandEvent & /*event*/);
#if OPAL_HAS_IM
    void OnStartIM(wxCommandEvent & /*event*/);
    void OnInCallIM(wxCommandEvent & /*event*/);
    void OnSendIMSpeedDial(wxCommandEvent & /*event*/);
#endif // OPAL_HAS_IM

    void OnEvtRinging(wxCommandEvent & /*event*/ theEvent);
    void OnEvtEstablished(wxCommandEvent & /*event*/ theEvent);
    void OnEvtOnHold(wxCommandEvent & /*event*/ theEvent);
    void OnEvtCleared(wxCommandEvent & /*event*/ theEvent);

    bool CanDoFax() const;
    bool CanDoIM() const;

    enum SpeedDialViews {
      e_ViewLarge,
      e_ViewSmall,
      e_ViewList,
      e_ViewDetails,
      e_NumViews
    };

    void RecreateSpeedDials(
      SpeedDialViews view
    );
    void EditSpeedDial(
      int index
    );
    bool UpdateSpeedDial(
      int index,
      const SpeedDialInfo & info,
      bool saveConfig
    );

    enum {
      e_NameColumn,
      e_StatusColumn,
      e_NumberColumn,
      e_AddressColumn,
      e_PresentityColumn,
      e_DescriptionColumn,
      e_NumColumns
    };

    // Controls on main frame
    enum {
      SplitterID = 100,
      TabsID,
      SpeedDialsID
    };

    wxIcon             m_appIcon;
    bool               m_hideMinimised; // Task bar icon gets it back
    MyTaskBarIcon    * m_taskBarIcon;
    wxSplitterWindow * m_splitter;
    wxNotebook       * m_tabs;
    wxTextCtrl       * m_logWindow;
    wxListCtrl       * m_speedDials;
    bool               m_speedDialDetail;
    wxImageList      * m_imageListNormal;
    wxImageList      * m_imageListSmall;
    wxDataFormat       m_ClipboardFormat;

    set<SpeedDialInfo> m_speedDialInfo;
    SpeedDialInfo * GetSelectedSpeedDial() const;

    MyPCSSEndPoint   * pcssEP;
    OpalLineEndPoint * potsEP;
    void StartLID();

    int       m_NATHandling;
    PwxString m_NATRouter;
    PwxString m_STUNServer;
    void SetNATHandling();

    vector<PwxString> m_LocalInterfaces;
    void StartAllListeners();
#if OPAL_PTLIB_SSL
    void SetSignalingSecurity(OpalEndPoint * ep, int security, const char * unsecure, const char * secure);
#endif

#if OPAL_H323
    MyH323EndPoint * h323EP;
    int              m_gatekeeperMode;
    PwxString        m_gatekeeperAddress;
    PwxString        m_gatekeeperIdentifier;
#endif
    bool StartGatekeeper();

#if OPAL_SIP
    friend class MySIPEndPoint;
    MySIPEndPoint  * sipEP;
    bool             m_SIPProxyUsed;
    RegistrationList m_registrations;

    void StartRegistrations();
    void ReplaceRegistrations(const RegistrationList & newRegistrations);
#endif

#if OPAL_CAPI
    OpalCapiEndPoint  * capiEP;
#endif

#if OPAL_IVR
    OpalIVREndPoint  * ivrEP;
#endif

#if OPAL_HAS_MIXER
    OpalMixerEndPoint * m_mcuEP;
#endif

#if OPAL_FAX
    OpalFaxEndPoint  * m_faxEP;
#endif

#if OPAL_HAS_IM
    OpalIMEndPoint * m_imEP;
#endif

    bool      m_autoAnswer;
    PwxString m_LastDialed;
    PwxString m_LastReceived;

    PwxString m_ForwardingAddress;
    int       m_ForwardingTimeout;
    PTimer    m_ForwardingTimer;
    PDECLARE_NOTIFIER(PTimer, MyManager, OnForwardingTimeout);

    bool m_VideoGrabPreview;
    int  m_localVideoFrameX;
    int  m_localVideoFrameY;
    int  m_remoteVideoFrameX;
    int  m_remoteVideoFrameY;

    PVideoInputDevice * m_primaryVideoGrabber;

    PVideoDevice::OpenArgs m_SecondaryVideoGrabber;
    bool                   m_SecondaryVideoGrabPreview;
    bool                   m_SecondaryVideoOpening;

    PwxString m_VideoGrabFrameSize;
    PwxString m_VideoMinFrameSize;
    PwxString m_VideoMaxFrameSize;
    int       m_VideoMaxBitRate;
    int       m_VideoTargetBitRate;
    bool AdjustVideoFormats();

    enum ExtendedVideoRoles {
      e_DisabledExtendedVideoRoles,
      e_ExtendedVideoRolePerOption,
      e_ForcePresentationVideoRole,
      e_ForceLiveVideoRole
    } m_ExtendedVideoRoles;

    MyMediaList m_mediaInfo;
    void InitMediaInfo(const OpalMediaFormatList & formats);
    void ApplyMediaInfo();

#if PTRACING
    bool      m_enableTracing;
    wxString  m_traceFileName;
#endif

    PwxString     m_RingSoundDeviceName;
    PwxString     m_RingSoundFileName;
    PSoundChannel m_RingSoundChannel;
    PTimer        m_RingSoundTimer;
    PDECLARE_NOTIFIER(PTimer, MyManager, OnRingSoundAgain);
    void StopRingSound();
    void UpdateAudioDevices();

    PwxString          m_incomingToken;
    PSafePtr<OpalCall> m_activeCall;

    void AddCallOnHold(
      OpalCall & call
    );
    bool RemoveCallOnHold(
      const PString & token
    );
    void OnHoldChanged(
      const PString & token,
      bool onHold
    );
    void AddToConference(OpalCall & call);

    struct CallsOnHold {
      CallsOnHold() { }
      CallsOnHold(OpalCall & call);

      PSafePtr<OpalCall> m_call;
      int                m_retrieveMenuId;
      int                m_conferenceMenuId;
      int                m_transferMenuId;
    };
    list<CallsOnHold>    m_callsOnHold;
    PwxString            m_switchHoldToken;

    OpalRecordManager::Options m_recordingOptions;
    PwxString                  m_lastRecordFile;
    PwxString                  m_lastPlayFile;

    // Presence
    bool MonitorPresence(const PString & presentity, const PString & uri, bool start);
    PDECLARE_ASYNC_PresenceChangeNotifier(MyManager, OnPresenceChange);

#if OPAL_HAS_IM
    // Instant messaging
    PDECLARE_ASYNC_ConversationNotifier(MyManager, OnConversation);

    // cannot use PDictionary as IMDialog is not descended from PObject
    typedef std::map<wxString, IMDialog *> IMDialogMap;
    IMDialogMap m_imDialogMap;
#endif

    DECLARE_EVENT_TABLE()

  friend class OptionsDialog;
  friend class InCallPanel;
  friend class IMDialog;
};


class OpenPhoneApp : public wxApp, public PProcess
{
    PCLASSINFO(OpenPhoneApp, PProcess);

  public:
    OpenPhoneApp();

    void Main(); // Dummy function

      // FUnction from wxWindows
    virtual bool OnInit();
};


#endif


// End of File ///////////////////////////////////////////////////////////////
