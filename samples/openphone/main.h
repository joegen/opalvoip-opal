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
 */

#ifndef _OpenPhone_MAIN_H
#define _OpenPhone_MAIN_H

#if WXVER>29
  #define WXUSINGDLL 1
#endif

#include <ptlib_wx.h>
#include <opal/manager.h>

#ifndef OPAL_PTLIB_AUDIO
  #error Cannot compile without PTLib sound channel support!
#endif

#ifndef OPAL_VIDEO
  #error Cannot compile without video support!
#endif

#include <wx/taskbar.h>
#include <wx/dataobj.h>

#include <ptlib/notifier_ext.h>
#include <ptclib/pssl.h>

#include <h323/h323.h>
#include <h323/gkclient.h>
#include <sip/sip.h>
#include <ep/pcss.h>
#include <t38/t38proto.h>
#include <im/im_ep.h>
#include <h224/h224.h>

// Note, include this after everything else so gets all the conversions
#include <ptlib/wxstring.h>


class MyManager;

class OpalLineEndPoint;
class OpalCapiEndPoint;
class OpalIVREndPoint;
class OpalMixerEndPoint;
class OpalFaxEndPoint;
class OpalH281Client;

class wxSplitterWindow;
class wxSplitterEvent;
class wxSpinCtrl;
class wxListCtrl;
class wxListEvent;
class wxListItem;
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

    virtual PSoundChannel * CreateSoundChannel(const OpalPCSSConnection & connection, const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource);

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

    PStringList m_configuredAliasNames;

  private:
    virtual void OnGatekeeperStatus(H323Gatekeeper & gk, H323Gatekeeper::RegistrationFailReasons);

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


#if OPAL_HAS_PRESENCE
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
#endif // OPAL_HAS_PRESENCE


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
  virtual void GetValue(const OpalConnection & connection, const OpalMediaStream & stream, const OpalMediaStatistics & statistics, PwxString & value) = 0;

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

class InCallPanel : public CallPanelBase, PAsyncNotifierTarget
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

    // PAsyncNotifierTarget override
    virtual void AsyncNotifierSignal();
    void OnEvtAsyncNotification(wxCommandEvent & /*event*/);

#if OPAL_HAS_H281
    void OnMouseFECC(wxMouseEvent & /*event*/);
    PDECLARE_ASYNC_NOTIFIER(OpalH281Client, InCallPanel, OnChangedFECC);
#endif

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
    wxGauge      * m_SpeakerGauge;
    wxGauge      * m_MicrophoneGauge;
    wxTimer        m_vuTimer;
    wxStaticText * m_CallTime;

#if OPAL_HAS_H281
    wxButton     * m_fecc[PVideoControlInfo::NumTypes][2];
#endif

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
#if OPAL_HAS_PRESENCE
  PwxString m_Presentity;
#endif
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


class MyMedia
{
public:
  MyMedia();
  MyMedia(const OpalMediaFormat & format);

  bool operator<(const MyMedia & other) const
  {
    return preferenceOrder < other.preferenceOrder;
  }

  OpalMediaFormat mediaFormat;
  const wxChar  * validProtocols;
  int             preferenceOrder;
};

typedef std::list<MyMedia> MyMediaList;


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


class NetOptionsDialog : public wxDialog
{
  public:
    NetOptionsDialog(MyManager *manager, OpalRTPEndPoint * ep);

  private:
    MyManager & m_manager;
    OpalRTPEndPoint * m_endpoint;

#if OPAL_PTLIB_SSL
    int              m_SignalingSecurity;
    wxRadioBox     * m_SignalingSecurityButtons;
    wxCheckListBox * m_MediaCryptoSuites;
    wxButton       * m_MediaCryptoSuiteUp;
    wxButton       * m_MediaCryptoSuiteDown;
    void InitSecurityFields();
    void SaveSecurityFields(wxConfigBase * config, const wxChar * securedSignalingKey, const wxChar * mediaCryptoSuitesKey);
    void SignalingSecurityChanged(wxCommandEvent & /*event*/);
    void MediaCryptoSuiteChanged(wxCommandEvent & /*event*/);
    void MediaCryptoSuiteUp(wxCommandEvent & /*event*/);
    void MediaCryptoSuiteDown(wxCommandEvent & /*event*/);
    void MediaCryptoSuiteMove(int dir);
#endif

    wxGrid * m_stringOptions;
    void SaveOptions(wxConfigBase * config, const wxChar * stringOptionsGroup);

    friend class OptionsDialog;

    DECLARE_EVENT_TABLE()
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
    PwxString    m_Username;
    PwxString    m_DisplayName;
    PwxString    m_RingSoundDeviceName;
    PwxString    m_RingSoundFileName;
    bool         m_AutoAnswer;
    bool         m_OverrideProductInfo;
    PwxString    m_VendorName;
    PwxString    m_ProductName;
    PwxString    m_ProductVersion;
    wxTextCtrl * m_VendorNameCtrl;
    wxTextCtrl * m_ProductNameCtrl;
    wxTextCtrl * m_ProductVersionCtrl;
    bool         m_HideMinimised;

    void BrowseSoundFile(wxCommandEvent & /*event*/);
    void PlaySoundFile(wxCommandEvent & /*event*/);
    void OnOverrideProductInfo(wxCommandEvent & /*event*/);

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
    wxListBox     * m_LocalInterfaces;
    wxChoice      * m_InterfaceProtocol;
    wxComboBox    * m_InterfaceAddress;
    wxTextCtrl    * m_InterfacePort;
    wxButton      * m_AddInterface;
    wxButton      * m_RemoveInterface;
    wxListBox     * m_NatMethods;
    int             m_NatMethodSelected;

    struct NatWrapper : wxClientData
    {
      PwxString m_method;
      bool      m_active;
      PwxString m_server;
      int       m_priority;
    } m_NatInfo;

    void BandwidthClass(wxCommandEvent & /*event*/);
#if OPAL_PTLIB_SSL
    void FindCertificateAuthority(wxCommandEvent & /*event*/);
    void FindLocalCertificate(wxCommandEvent & /*event*/);
    void FindPrivateKey(wxCommandEvent & /*event*/);
#endif
    void NATMethodSelect(wxCommandEvent & /*event*/);
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
    PwxString m_LocalRingbackTone;
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
    PwxString m_MusicOnHold;
    PwxString m_AudioOnRing;

    wxComboBox * m_soundPlayerCombo;
    wxComboBox * m_soundRecorderCombo;
    wxComboBox * m_musicOnHoldCombo;
    wxComboBox * m_audioOnRingCombo;
    wxComboBox * m_selectedLID;
    wxChoice   * m_selectedAEC;
    wxComboBox * m_selectedCountry;

    void ChangedSoundPlayer(wxCommandEvent & /*event*/);
    void ChangedSoundRecorder(wxCommandEvent & /*event*/);
    void TestPlayer(wxCommandEvent & /*event*/);
    void TestRecorder(wxCommandEvent & /*event*/);
    void SelectedLID(wxCommandEvent & /*event*/);
    void ChangedMusicOnHold(wxCommandEvent & /*event*/);
    void ChangedAudioOnRing(wxCommandEvent & /*event*/);

    ////////////////////////////////////////
    // Video fields
    PwxString m_VideoGrabDevice;
    int       m_VideoGrabFormat;
    int       m_VideoGrabSource;
    int       m_VideoGrabFrameRate;
    PwxString m_VideoGrabFrameSize;
    bool      m_VideoGrabPreview;
    PwxString m_VideoWatermarkDevice;
    bool      m_VideoFlipLocal;
    bool      m_VideoAutoTransmit;
    bool      m_VideoAutoReceive;
    bool      m_VideoFlipRemote;
    PwxString m_VideoMinFrameSize;
    PwxString m_VideoMaxFrameSize;
    int       m_VideoGrabBitRate;
    int       m_VideoMaxBitRate;
    PwxString m_VideoOnHold;
    PwxString m_VideoOnRing;

    wxComboBox * m_VideoGrabDeviceCtrl;
    wxComboBox * m_VideoWatermarkDeviceCtrl;
    wxComboBox * m_VideoOnHoldDeviceCtrl;
    wxComboBox * m_VideoOnRingDeviceCtrl;
    wxChoice   * m_VideoGrabSourceCtrl;

    wxButton           * m_TestVideoCapture;
    PThread            * m_TestVideoThread;
    PVideoInputDevice  * m_TestVideoGrabber;
    PVideoOutputDevice * m_TestVideoDisplay;

    void AdjustVideoControls();
    void ChangeVideoGrabDevice(wxCommandEvent & /*event*/);
    void ChangeVideoWatermarkDevice(wxCommandEvent & /*event*/);
    void TestVideoCapture(wxCommandEvent & /*event*/);
    void OnTestVideoEnded(wxCommandEvent & /*event*/);
    void TestVideoThreadMain();
    void StopTestVideo();
    void ChangedVideoOnHold(wxCommandEvent & /*event*/);
    void ChangedVideoOnRing(wxCommandEvent & /*event*/);

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

#if OPAL_HAS_PRESENCE
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
#endif // OPAL_HAS_PRESENCE

    ////////////////////////////////////////
    // Codec fields
    wxButton     * m_AddCodec;
    wxButton     * m_RemoveCodec;
    wxButton     * m_MoveUpCodec;
    wxButton     * m_MoveDownCodec;
    wxListCtrl   * m_allCodecs;
    wxListBox    * m_selectedCodecs;
    wxListCtrl   * m_codecOptions;
    wxTextCtrl   * m_codecOptionValue;
    wxStaticText * m_CodecOptionValueLabel;
    wxStaticText * m_CodecOptionValueError;
    wxButton     * m_SetDefaultCodecOption;

    void AddCodec(wxCommandEvent & /*event*/);
    void RemoveCodec(wxCommandEvent & /*event*/);
    void MoveUpCodec(wxCommandEvent & /*event*/);
    void MoveDownCodec(wxCommandEvent & /*event*/);
    void SelectedCodecToAdd(wxListEvent & /*event*/);
    void SelectedCodec(wxCommandEvent & /*event*/);
    void SelectedCodecOption(wxListEvent & /*event*/);
    void DeselectedCodecOption(wxListEvent & /*event*/);
    void ChangedCodecOptionValue(wxCommandEvent & /*event*/);
    void SetDefaultCodecOption(wxCommandEvent & /*event*/);
    void SetCodecOptionValue(bool useDefault, PwxString newValue);
    MyMedia * GetCodecOptionInfo(wxListItem & item, PwxString & optionName, PwxString & defaultValue);

    ////////////////////////////////////////
    // H.323 fields
    int          m_GatekeeperMode;
    PwxString    m_GatekeeperAddress;
    PwxString    m_GatekeeperInterface;
    bool         m_GatekeeperSuppressGRQ;
    PwxString    m_GatekeeperIdentifier;
    int          m_GatekeeperTTL;
    PwxString    m_GatekeeperLogin;
    PwxString    m_GatekeeperPassword;

    wxString     m_H323TerminalType;
    int          m_DTMFSendMode;
    int          m_CallIntrusionProtectionLevel;
    bool         m_DisableFastStart;
    bool         m_DisableH245Tunneling;
    bool         m_DisableH245inSETUP;
    bool         m_ForceSymmetricTCS;
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

    void MoreOptionsH323(wxCommandEvent & /*event*/);
    NetOptionsDialog m_H323options;

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

    void MoreOptionsSIP(wxCommandEvent & /*event*/);
    NetOptionsDialog m_SIPoptions;

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
    bool AnswerCall();
    void RejectCall();
    void HangUpCall();
    void SendUserInput(char tone);

    PSafePtr<OpalCall>       GetCall(PSafetyMode mode);
    PSafePtr<OpalConnection> GetNetworkConnection(PSafetyMode mode);

    template<class T> bool GetConnection(PSafePtr<T> & ptr, PSafetyMode mode)
    {
      return m_activeCall != NULL && (ptr = m_activeCall->GetConnectionAs<T>(0, mode)) != NULL;
    }

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
      const wxCommandEvent & cmdEvent,
      const PString & str = PString::Empty(),
      const void * data = NULL
    );

#if OPAL_PTLIB_SSL
    void SetSignalingSecurity(OpalEndPoint * ep, int security);
#endif

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
    virtual void OnStartMediaPatch(
      OpalConnection & connection,  ///< Connection patch is in
      OpalMediaPatch & patch        ///< Media patch being started
    );
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     ///<  Stream being closed
    );
    virtual bool OnChangedPresentationRole(
      OpalConnection & connection,   ///< COnnection that has had the change
      const PString & newChairURI,   ///< URI for new confernce chair
      bool request                   ///< Indicates change is requested
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
    virtual bool CreateVideoInputDevice(
      const OpalConnection & connection,    ///<  Connection needing created video device
      const PVideoDevice::OpenArgs & args,  ///< Device to change to
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
    void OnAudioDeviceChange(wxCommandEvent & /*event*/);
    void OnNewCodec(wxCommandEvent & /*event*/);
    void OnVideoDeviceChange(wxCommandEvent & /*event*/);
    void OnStartVideo(wxCommandEvent & /*event*/);
    void OnStopVideo(wxCommandEvent & /*event*/);
    void OnSendVFU(wxCommandEvent & /*event*/);
    void OnSendIntra(wxCommandEvent & /*event*/);
    void OnTxVideoControl(wxCommandEvent & /*event*/);
    void OnRxVideoControl(wxCommandEvent & /*event*/);
    void OnMenuPresentationRole(wxCommandEvent & /*event*/);
    void OnDefVidWinPos(wxCommandEvent & /*event*/);
    void OnSashPositioned(wxSplitterEvent & /*event*/);
    void OnSpeedDialActivated(wxListEvent & /*event*/);
    void OnSpeedDialColumnResize(wxListEvent & /*event*/);
    void OnSpeedDialRightClick(wxListEvent & /*event*/);
    void OnSpeedDialEndEdit(wxListEvent & /*event*/);

#if OPAL_HAS_PRESENCE
    void OnMyPresence(wxCommandEvent & /*event*/);
#if OPAL_HAS_IM
    void OnStartIM(wxCommandEvent & /*event*/);
    void OnInCallIM(wxCommandEvent & /*event*/);
    void OnSendIMSpeedDial(wxCommandEvent & /*event*/);
#endif // OPAL_HAS_IM
#endif // OPAL_HAS_PRESENCE

    void OnEvtRinging(wxCommandEvent & /*event*/ theEvent);
    void OnEvtEstablished(wxCommandEvent & /*event*/ theEvent);
    void OnEvtOnHold(wxCommandEvent & /*event*/ theEvent);
    void OnEvtCleared(wxCommandEvent & /*event*/ theEvent);
    void OnSetTrayTipText(wxCommandEvent & /*event*/);

    bool CanDoFax() const;
#if OPAL_HAS_PRESENCE && OPAL_HAS_IM
    bool CanDoIM() const;
#endif

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

    void SetNAT(const PwxString & method, bool active, const PwxString & server, int priority);

    vector<PwxString> m_LocalInterfaces;
    void StartAllListeners();

#if OPAL_H323
    MyH323EndPoint * h323EP;
    int              m_gatekeeperMode;
    PwxString        m_gatekeeperAddress;
    PwxString        m_gatekeeperInterface;
    PwxString        m_gatekeeperIdentifier;
#endif
    bool StartGatekeeper();

#if OPAL_SIP
    friend class MySIPEndPoint;
    MySIPEndPoint  * sipEP;
    bool             m_SIPProxyUsed;
    RegistrationList m_registrations;
    PMutex           m_registrationsMutex;

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

    bool            m_OverrideProductInfo;
    OpalProductInfo m_overriddenProductInfo;

    PwxString m_ForwardingAddress;
    int       m_ForwardingTimeout;
    PTimer    m_ForwardingTimer;
    PDECLARE_NOTIFIER(PTimer, MyManager, OnForwardingTimeout);

    struct VideoWindowInfo
    {
      int x, y, size;

      VideoWindowInfo() : x(wxDefaultCoord), y(wxDefaultCoord), size(0)
      {
      }
      VideoWindowInfo(const PVideoOutputDevice * device);

      bool operator==(const VideoWindowInfo & info) const;
      bool ReadConfig(wxConfigBase * config, const wxString & keyBase);
      void WriteConfig(wxConfigBase * config, const wxString & keyBase);
    };
    VideoWindowInfo m_videoWindowInfo[2][OpalVideoFormat::NumContentRole];

    bool                m_VideoGrabPreview;
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

    PString m_VideoWatermarkDevice;

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
    void UpdateAudioVideoDevices();

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

#if OPAL_HAS_PRESENCE
    // Presence
    bool MonitorPresence(const PString & presentity, const PString & uri, bool start);
    PDECLARE_ASYNC_PresenceChangeNotifier(MyManager, OnPresenceChange);
#endif

#if OPAL_HAS_IM
    // Instant messaging
    PDECLARE_ASYNC_ConversationNotifier(MyManager, OnConversation);

    // cannot use PDictionary as IMDialog is not descended from PObject
    typedef std::map<PString, IMDialog *> IMDialogMap;
    IMDialogMap m_imDialogMap;
#endif

#if OPAL_PTLIB_SSL
    virtual bool ApplySSLCredentials(
      const OpalEndPoint & ep,  ///< Endpoint transport is based on.
      PSSLContext & context,    ///< Context on which to set certificates
      bool create               ///< Create self signed cert/key if required
    ) const;

    PDECLARE_SSLPasswordNotifier(MyManager, OnSSLPassword);
    void OnEvtGetSSLPassword(wxCommandEvent & /*event*/);

    PSyncPoint m_gotSSLPassword;
    PwxString m_SSLPassword;
#endif // OPAL_PTLIB_SSL

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
