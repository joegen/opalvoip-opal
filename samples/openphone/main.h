/*
 * main.h
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
 * Contributor(s): ______________________________________.
 *
 * $Log: main.h,v $
 * Revision 1.22  2005/07/09 07:05:16  rjongbloed
 * Changed so resources are included in compile and not separate file at run time.
 * General code clean ups.
 *
 * Revision 1.21  2005/06/02 13:21:49  rjongbloed
 * Save and restore media format options to registry.
 * Added check for valid value for media format option in dialog.
 *
 * Revision 1.20  2005/02/21 12:19:50  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 1.19  2004/10/06 13:08:19  rjongbloed
 * Implemented partial support for LIDs
 *
 * Revision 1.18  2004/10/03 14:16:34  rjongbloed
 * Added panels for calling, answering and in call phases.
 *
 * Revision 1.17  2004/09/29 12:47:39  rjongbloed
 * Added gatekeeper support
 *
 * Revision 1.16  2004/09/29 12:02:40  rjongbloed
 * Added popup menu to edit Speed DIals
 *
 * Revision 1.15  2004/09/28 23:00:18  rjongbloed
 * Added ability to add and edit Speed DIals
 *
 * Revision 1.14  2004/08/22 12:27:45  rjongbloed
 * More work on SIP registration, time to live refresh and deregistration on exit.
 *
 * Revision 1.13  2004/07/17 08:21:24  rjongbloed
 * Added ability to manipulate codec lists
 *
 * Revision 1.12  2004/07/14 13:17:42  rjongbloed
 * Added saving of the width of columns in the speed dial list.
 * Fixed router display in options dialog so is empty if IP address invalid.
 *
 * Revision 1.11  2004/07/04 12:53:09  rjongbloed
 * Added support for route editing.
 *
 * Revision 1.10  2004/06/05 14:37:03  rjongbloed
 * More implemntation of options dialog.
 *
 * Revision 1.9  2004/05/25 12:56:07  rjongbloed
 * Added all silence suppression modes to Options dialog.
 *
 * Revision 1.8  2004/05/24 13:44:04  rjongbloed
 * More implementation on OPAL OpenPhone.
 *
 * Revision 1.7  2004/05/15 12:18:23  rjongbloed
 * More work on wxWindows based OpenPhone
 *
 * Revision 1.6  2004/05/12 12:41:38  rjongbloed
 * More work on wxWindows based OpenPhone
 *
 * Revision 1.5  2004/05/06 13:23:43  rjongbloed
 * Work on wxWindows based OpenPhone
 *
 * Revision 1.4  2004/05/01 13:38:05  rjongbloed
 * Some early implementation of wxWIndows based OPAL GUI client.
 *
 */

#ifndef _OpenPhone_MAIN_H
#define _OpenPhone_MAIN_H

#include <wx/wx.h>

#include <ptlib.h>

#include <opal/manager.h>
#include <opal/pcss.h>

#if OPAL_H323
#include <h323/h323.h>
#include <h323/gkclient.h>
#endif

#if OPAL_SIP
#include <sip/sip.h>
#endif


#include <list>


class MyManager;

class OpalPOTSEndPoint;
class OpalIVREndPoint;

class wxSplitterWindow;
class wxSplitterEvent;
class wxListCtrl;
class wxListEvent;
class wxGrid;


class PwxString : public wxString
{
  public:
    PwxString() { }
    PwxString(const wxString & str) : wxString(str) { }
    PwxString(const PString & str) : wxString((const char *)str) { }
    PwxString(const char * str) : wxString(str) { }

    PwxString & operator=(const wxString & str) { wxString::operator=(str); return *this; }
    PwxString & operator=(const PString & str) { wxString::operator=((const char *)str); return *this; }
    PwxString & operator=(const char * str) { wxString::operator=(str); return *this; }

    operator PString() const { return c_str(); }
    operator PIPSocket::Address() const { return PIPSocket::Address(PString(c_str())); }
    friend ostream & operator<<(ostream & stream, const PwxString & string) { return stream << string.c_str(); }
};


class MyPCSSEndPoint : public OpalPCSSEndPoint
{
  PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);

  public:
    MyPCSSEndPoint(MyManager & manager);

  private:
    virtual PString OnGetDestination(const OpalPCSSConnection & connection);
    virtual void OnShowIncoming(const OpalPCSSConnection & connection);
    virtual BOOL OnShowOutgoing(const OpalPCSSConnection & connection);

    MyManager & m_manager;
};


#if OPAL_H323
class MyH323EndPoint : public H323EndPoint
{
  public:
    MyH323EndPoint(MyManager & manager);

  private:
    virtual void OnRegistrationConfirm();

    MyManager & m_manager;
};
#endif


#if OPAL_SIP
class MySIPEndPoint : public SIPEndPoint
{
  public:
    MySIPEndPoint(MyManager & manager);

  private:
    virtual void OnRegistered();

    MyManager & m_manager;
};
#endif


class CallDialog : public wxDialog
{
  public:
    CallDialog(wxFrame * parent);

    PwxString m_Address;

  private:
    void OnAddressChange(wxCommandEvent & event);

    wxComboBox * m_AddressCtrl;
    wxButton   * m_ok;

    DECLARE_EVENT_TABLE()
};


class CallingPanel : public wxPanel
{
  public:
    CallingPanel(MyManager & manager, wxWindow * parent);

  private:
    void OnHangUp(wxCommandEvent & event);

    MyManager & m_manager;

    DECLARE_EVENT_TABLE()
};


class AnswerPanel : public wxPanel
{
  public:
    AnswerPanel(MyManager & manager, wxWindow * parent);

  private:
    void OnAnswer(wxCommandEvent & event);
    void OnReject(wxCommandEvent & event);

    MyManager & m_manager;

    DECLARE_EVENT_TABLE()
};


class InCallPanel : public wxPanel
{
  public:
    InCallPanel(MyManager & manager, wxWindow * parent);

  private:
    void OnHangUp(wxCommandEvent & event);
    void OnUserInput1(wxCommandEvent & event);
    void OnUserInput2(wxCommandEvent & event);
    void OnUserInput3(wxCommandEvent & event);
    void OnUserInput4(wxCommandEvent & event);
    void OnUserInput5(wxCommandEvent & event);
    void OnUserInput6(wxCommandEvent & event);
    void OnUserInput7(wxCommandEvent & event);
    void OnUserInput8(wxCommandEvent & event);
    void OnUserInput9(wxCommandEvent & event);
    void OnUserInput0(wxCommandEvent & event);
    void OnUserInputStar(wxCommandEvent & event);
    void OnUserInputHash(wxCommandEvent & event);
    void OnUserInputFlash(wxCommandEvent & event);

    MyManager & m_manager;

    DECLARE_EVENT_TABLE()
};


class SpeedDialDialog : public wxDialog
{
  public:
    SpeedDialDialog(MyManager *parent);

    wxString m_Name;
    wxString m_Number;
    wxString m_Address;
    wxString m_Description;

  private:
    void OnChange(wxCommandEvent & event);

    MyManager & m_manager;

    wxButton     * m_ok;
    wxTextCtrl   * m_nameCtrl;
    wxTextCtrl   * m_numberCtrl;
    wxStaticText * m_inUse;
    wxStaticText * m_ambiguous;

    DECLARE_EVENT_TABLE()
};


class OptionsDialog : public wxDialog
{
  public:
    OptionsDialog(MyManager *parent);
    virtual bool TransferDataFromWindow();

  private:
    MyManager & m_manager;

    ////////////////////////////////////////
    // General fields
    PwxString m_Username;
    PwxString m_DisplayName;
    PwxString m_RingSoundFileName;
    bool      m_AutoAnswer;
#if P_EXPAT
    PwxString m_IVRScript;
#endif

    void BrowseSoundFile(wxCommandEvent & event);
    void PlaySoundFile(wxCommandEvent & event);

    ////////////////////////////////////////
    // Networking fields
    int       m_Bandwidth;
    int       m_TCPPortBase;
    int       m_TCPPortMax;
    int       m_UDPPortBase;
    int       m_UDPPortMax;
    int       m_RTPPortBase;
    int       m_RTPPortMax;
    int       m_RTPTOS;
    PwxString m_STUNServer;
    PwxString m_NATRouter;
    PwxString m_InterfaceToAdd;
    void BandwidthClass(wxCommandEvent & event);
    void SelectedLocalInterface(wxCommandEvent & event);
    void AddInterface(wxCommandEvent & event);
    void RemoveInterface(wxCommandEvent & event);

    ////////////////////////////////////////
    // Sound fields
    PwxString m_SoundPlayer;
    PwxString m_SoundRecorder;
    int       m_SoundBuffers;
    PwxString m_LineInterfaceDevice;
    int       m_AEC;
    PwxString m_Country;
    int       m_MinJitter;
    int       m_MaxJitter;
    int       m_SilenceSuppression;
    int       m_SilenceThreshold;
    int       m_SignalDeadband;
    int       m_SilenceDeadband;

    wxComboBox * m_selectedLID;
    wxComboBox * m_selectedAEC;
    wxTextCtrl * m_selectedCountry;
    void SelectedLID(wxCommandEvent & event);

    ////////////////////////////////////////
    // Video fields
    PwxString m_VideoGrabber;
    int       m_VideoGrabFormat;
    int       m_VideoGrabSource;
    int       m_VideoEncodeQuality;
    int       m_VideoGrabFrameRate;
    int       m_VideoEncodeMaxBitRate;
    bool      m_VideoGrabPreview;
    bool      m_VideoFlipLocal;
    bool      m_VideoAutoTransmit;
    bool      m_VideoAutoReceive;
    bool      m_VideoFlipRemote;

    ////////////////////////////////////////
    // Codec fields
    wxButton   * m_AddCodec;
    wxButton   * m_RemoveCodec;
    wxButton   * m_MoveUpCodec;
    wxButton   * m_MoveDownCodec;
    wxListBox  * m_allCodecs;
    wxListBox  * m_selectedCodecs;
    wxListCtrl * m_codecOptions;
    wxTextCtrl * m_codecOptionValue;

    void AddCodec(wxCommandEvent & event);
    void RemoveCodec(wxCommandEvent & event);
    void MoveUpCodec(wxCommandEvent & event);
    void MoveDownCodec(wxCommandEvent & event);
    void SelectedCodecToAdd(wxCommandEvent & event);
    void SelectedCodec(wxCommandEvent & event);
    void SelectedCodecOption(wxCommandEvent & event);
    void DeselectedCodecOption(wxCommandEvent & event);
    void ChangedCodecOptionValue(wxCommandEvent & event);

    ////////////////////////////////////////
    // H.323 fields
    int       m_GatekeeperMode;
    PwxString m_GatekeeperAddress;
    PwxString m_GatekeeperIdentifier;
    int       m_GatekeeperTTL;
    PwxString m_GatekeeperLogin;
    PwxString m_GatekeeperPassword;
    int       m_DTMFSendMode;
    int       m_CallIntrusionProtectionLevel;
    bool      m_DisableFastStart;
    bool      m_DisableH245Tunneling;
    bool      m_DisableH245inSETUP;

    void AddAlias(wxCommandEvent & event);
    void RemoveAlias(wxCommandEvent & event);

    ////////////////////////////////////////
    // SIP fields
    bool      m_SIPProxyUsed;
    PwxString m_SIPProxy;
    PwxString m_SIPProxyUsername;
    PwxString m_SIPProxyPassword;
    bool      m_RegistrarUsed;
    PwxString m_RegistrarName;
    PwxString m_RegistrarUsername;
    PwxString m_RegistrarPassword;
    int       m_RegistrarTimeToLive;

    ////////////////////////////////////////
    // Routing fields
    wxListCtrl * m_Routes;
    int          m_SelectedRoute;
    wxComboBox * m_RouteSource;
    wxTextCtrl * m_RoutePattern;
    wxTextCtrl * m_RouteDestination;
    wxButton   * m_AddRoute;
    wxButton   * m_RemoveRoute;

    void AddRoute(wxCommandEvent & event);
    void RemoveRoute(wxCommandEvent & event);
    void SelectedRoute(wxListEvent & event);
    void DeselectedRoute(wxListEvent & event);
    void ChangedRouteInfo(wxCommandEvent & event);

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
    PwxString m_TraceFileName;
#endif

    DECLARE_EVENT_TABLE()
};


class MyMedia
{
public:
  MyMedia(
    const char * source,
    const PString & format
  ) : sourceProtocol(source),
      mediaFormat(format),
      preferenceOrder(-1), // -1 indicates disabled
      dirty(false)
  { }

  bool operator<(const MyMedia & other) { return preferenceOrder < other.preferenceOrder; }

  const char    * sourceProtocol;
  OpalMediaFormat mediaFormat;
  int             preferenceOrder;
  bool            dirty;
};

typedef std::list<MyMedia> MyMediaList;


class MyManager : public wxFrame, public OpalManager
{
  public:
    MyManager();
    ~MyManager();

    bool Initialise();

    bool HasSpeedDialName(const wxString & name) const;
    bool HasSpeedDialNumber(const char * number, const char * ignore) const;

    void MakeCall(const PwxString & address);
    void AnswerCall();
    void RejectCall();
    void HangUpCall();
    void SendUserInput(char tone);
    void OnRinging(const OpalPCSSConnection & connection);

    PSafePtr<OpalCall> GetCall() { return FindCallWithLock(m_currentCallToken); }

  private:
    // Controls on main frame
    enum {
      SplitterID = 100,
      SpeedDialsID
    };

    void OnClose(wxCloseEvent& event);

    void OnAdjustMenus(wxMenuEvent& event);

    void OnMenuQuit(wxCommandEvent& event);
    void OnMenuAbout(wxCommandEvent& event);
    void OnMenuCall(wxCommandEvent& event);
    void OnMenuAnswer(wxCommandEvent& event);
    void OnMenuHangUp(wxCommandEvent& event);
    void OnNewSpeedDial(wxCommandEvent& event);
    void OnViewLarge(wxCommandEvent& event);
    void OnViewSmall(wxCommandEvent& event);
    void OnViewList(wxCommandEvent& event);
    void OnViewDetails(wxCommandEvent& event);
    void OnEditSpeedDial(wxCommandEvent& event);
    void OnOptions(wxCommandEvent& event);
    void OnSashPositioned(wxSplitterEvent& event);
    void OnSpeedDialActivated(wxListEvent& event);
    void OnSpeedDialColumnResize(wxListEvent& event);
    void OnRightClick(wxListEvent& event);

    virtual void OnEstablishedCall(
      OpalCall & call   /// Call that was completed
    );
    virtual void OnClearedCall(
      OpalCall & call   /// Connection that was established
    );
    virtual BOOL OnOpenMediaStream(
      OpalConnection & connection,  /// Connection that owns the media stream
      OpalMediaStream & stream    /// New media stream being opened
    );
    virtual void OnUserInputString(
      OpalConnection & connection,  /// Connection input has come from
      const PString & value         /// String value of indication
    );

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

    enum {
      e_NameColumn,
      e_NumberColumn,
      e_AddressColumn,
      e_DescriptionColumn,
      e_NumColumns
    };

    wxSplitterWindow * m_splitter;
    wxTextCtrl       * m_logWindow;
    wxListCtrl       * m_speedDials;
    wxPanel          * m_answerPanel;
    wxPanel          * m_callingPanel;
    wxPanel          * m_inCallPanel;

    MyPCSSEndPoint   * pcssEP;
    OpalPOTSEndPoint * potsEP;

#if OPAL_H323
    MyH323EndPoint * h323EP;
    int              m_gatekeeperMode;
    PwxString        m_gatekeeperAddress;
    PwxString        m_gatekeeperIdentifier;
    bool StartGatekeeper();
#endif

#if OPAL_SIP
    MySIPEndPoint * sipEP;
    bool            m_SIPProxyUsed;
    bool            m_registrarUsed;
    PwxString       m_registrarName;
    PwxString       m_registrarUser;
    PwxString       m_registrarPassword;
    bool StartRegistrar();
#endif

#if P_EXPAT
    OpalIVREndPoint  * ivrEP;
#endif

    bool m_autoAnswer;

    MyMediaList m_mediaInfo;
    void InitMediaInfo(const char * source, const OpalMediaFormatList & formats);
    void ApplyMediaInfo();

#if PTRACING
    bool      m_enableTracing;
    wxString  m_traceFileName;
#endif

    enum CallState {
      IdleState,
      CallingState,
      RingingState,
      AnsweringState,
      InCallState
    } m_callState;
    void SetState(CallState newState);

    PString m_currentConnectionToken;
    PString m_currentCallToken;

    DECLARE_EVENT_TABLE()

  friend class OptionsDialog;
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
