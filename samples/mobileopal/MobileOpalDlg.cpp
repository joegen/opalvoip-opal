/*
 * MobileOpalDlg.h
 *
 * Sample Windows Mobile application.
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include "stdafx.h"
#include "MobileOPAL.h"
#include "MobileOpalDlg.h"
#include "OptionsGeneral.h"
#include "OptionsH323.h"
#include "OptionsSIP.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static UINT TimerID = 1;

static TCHAR const OptionsSection[]   = { 'O','p','t','i','o','n','s',0 };
static TCHAR const UserNameKey[]      = { 'U','s','e','r','N','a','m','e',0 };
static TCHAR const DisplayNameKey[]   = { 'D','i','s','p','l','a','y','N','a','m','e',0 };
static TCHAR const STUNServerKey[]    = { 'S','T','U','N','S','e','r','v','e','r',0 };
static TCHAR const GkTypeKey[]        = { 'G','a','t','e','k','e','e','p','e','r','T','y','p','e',0 };
static TCHAR const GkIdKey[]          = { 'G','a','t','e','k','e','e','p','e','r','I','d','e','n','t','i','f','e','r',0 };
static TCHAR const GkHostKey[]        = { 'G','a','t','e','k','e','e','p','e','r','H','o','s','t',0 };
static TCHAR const GkAliasKey[]       = { 'G','a','t','e','k','e','e','p','e','r','A','l','i','a','s',0 };
static TCHAR const GkAuthUserKey[]    = { 'G','a','t','e','k','e','e','p','e','r','U','s','e','r',0 };
static TCHAR const GkPasswordKey[]    = { 'G','a','t','e','k','e','e','p','e','r','P','a','s','s','w','o','r','d',0 };
static TCHAR const RegistrarAorKey[]  = { 'R','e','g','i','s','t','r','a','r','A','O','R',0 };
static TCHAR const RegistrarHostKey[] = { 'R','e','g','i','s','t','r','a','r','H','o','s','t',0 };
static TCHAR const RegistrarUserKey[] = { 'R','e','g','i','s','t','r','a','r','U','s','e','r',0 };
static TCHAR const RegistrarPassKey[] = { 'R','e','g','i','s','t','r','a','r','P','a','s','s','w','o','r','d',0 };
static TCHAR const RegistrarRealmKey[]= { 'R','e','g','i','s','t','r','a','r','R','e','a','l','m',0 };


static CString GetOptionString(LPCTSTR szKey)
{
  return AfxGetApp()->GetProfileString(OptionsSection, szKey);
}


static CStringA GetOptionStringA(LPCTSTR szKey)
{
  CStringA str((const TCHAR *)AfxGetApp()->GetProfileString(OptionsSection, szKey));
  return str;
}


static UINT GetOptionInt(LPCTSTR szKey)
{
  return AfxGetApp()->GetProfileInt(OptionsSection, szKey, 0);
}


static void SetOptionString(LPCTSTR szKey, LPCTSTR szValue)
{
  AfxGetApp()->WriteProfileString(OptionsSection, szKey, szValue);
}


static void SetOptionInt(LPCTSTR szKey, UINT szValue)
{
  AfxGetApp()->WriteProfileInt(OptionsSection, szKey, szValue);
}


///////////////////////////////////////////////////////////////////////////////

// CMobileOpalDlg dialog

CMobileOpalDlg::CMobileOpalDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CMobileOpalDlg::IDD, pParent)
  , m_opal(NULL)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


CMobileOpalDlg::~CMobileOpalDlg()
{
  OpalShutDown(m_opal); // Fail safe shutdown, can handle NULL
}


void CMobileOpalDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_ADDRESS, m_callAddress);
  DDX_Control(pDX, IDC_ADDRESS, m_ctrlAddress);
  DDX_Control(pDX, IDC_STATUS, m_ctrlStatus);
}


BEGIN_MESSAGE_MAP(CMobileOpalDlg, CDialog)
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
  ON_WM_SIZE()
#endif
  //}}AFX_MSG_MAP
  ON_WM_TIMER()
  ON_COMMAND(ID_CALL_ANSWER, &CMobileOpalDlg::OnCallAnswer)
  ON_EN_CHANGE(IDC_ADDRESS, &CMobileOpalDlg::OnChangedAddress)
  ON_COMMAND(IDM_OPTIONS_GENERAL, &CMobileOpalDlg::OnMenuOptionsGeneral)
  ON_COMMAND(IDM_OPTIONS_H323, &CMobileOpalDlg::OnMenuOptionsH323)
  ON_COMMAND(IDM_OPTIONS_SIP, &CMobileOpalDlg::OnMenuOptionsSIP)
  ON_COMMAND(IDM_EXIT, &CMobileOpalDlg::OnExit)
END_MESSAGE_MAP()


// CMobileOpalDlg message handlers

BOOL CMobileOpalDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);			// Set big icon
  SetIcon(m_hIcon, FALSE);		// Set small icon

  if (!m_dlgCommandBar.Create(this) || !m_dlgCommandBar.InsertMenuBar(IDR_MAINFRAME)) {
    TRACE0("Failed to create CommandBar\n");
    EndDialog(IDCANCEL);
    return FALSE;      // fail to create
  }

  SetCallButton(false);

  InitialiseOPAL();

  SetTimer(TimerID, 100, NULL);

  SetStatusText(IDS_READY);

  return TRUE;  // return TRUE  unless you set the focus to a control
}


#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void CMobileOpalDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
  if (AfxIsDRAEnabled())
  {
    DRA::RelayoutDialog(AfxGetResourceHandle(),
                        this->m_hWnd,
                        DRA::GetDisplayMode() != DRA::Portrait
                            ? MAKEINTRESOURCE(IDD_MOBILEOPAL_DIALOG_WIDE)
                            : MAKEINTRESOURCE(IDD_MOBILEOPAL_DIALOG));
  }
}
#endif


void CMobileOpalDlg::InitialiseOPAL()
{
  /////////////////////////////////////////////////////////////////////
  // Start up and initialise OPAL

  if (m_opal == NULL) {
    m_opal = OpalInitialise("pc h323 sip TraceLevel=2");
    if (m_opal == NULL) {
      ErrorBox(IDS_INIT_FAIL);
      EndDialog(IDCANCEL);
      return;
    }
  }

  OpalMessage command;
  OpalMessage * response;

  // General options
  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdSetGeneralParameters;

  CStringA strStunServer = GetOptionStringA(STUNServerKey);
  command.m_param.m_general.m_stunServer = strStunServer;

  if ((response = OpalSendMessage(m_opal, &command)) == NULL || response->m_type == OpalIndCommandError)
    ErrorBox(IDS_CONFIGURATION_FAIL);
  OpalFreeMessage(response);

  // Options across all protocols
  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdSetProtocolParameters;

  CStringA strUserName = GetOptionStringA(UserNameKey);
  command.m_param.m_protocol.m_name = strUserName;

  CStringA strDisplayName = GetOptionStringA(UserNameKey);
  command.m_param.m_protocol.m_displayName = strDisplayName;

  command.m_param.m_protocol.m_interfaceAddresses = "*";

  if ((response = OpalSendMessage(m_opal, &command)) == NULL || response->m_type == OpalIndCommandError)
    ErrorBox(IDS_LISTEN_FAIL);
  OpalFreeMessage(response);

  // H.323 gatekeeper regisration
  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdRegistration;

  UINT gkType = GetOptionInt(GkTypeKey);
  if (gkType > 0) {
    command.m_param.m_registrationInfo.m_protocol = "h323";

    CStringA strAliasName = GetOptionStringA(GkAliasKey);
    command.m_param.m_registrationInfo.m_identifier = strAliasName;

    CStringA strGkId = GetOptionStringA(GkIdKey);
    command.m_param.m_registrationInfo.m_adminEntity = strGkId;

    CStringA strGkHost = GetOptionStringA(GkHostKey);
    command.m_param.m_registrationInfo.m_hostName = strGkHost;

    CStringA strGkAuthUser = GetOptionStringA(GkAuthUserKey);
    command.m_param.m_registrationInfo.m_authUserName = strGkAuthUser;

    CStringA strGkPassword = GetOptionStringA(GkPasswordKey);
    command.m_param.m_registrationInfo.m_password = strGkPassword;

    if ((response = OpalSendMessage(m_opal, &command)) == NULL || response->m_type == OpalIndCommandError)
      ErrorBox(IDS_REGISTRATION_FAIL);
    OpalFreeMessage(response);
  }

  // SIP registrar regisration
  CStringA strAor = GetOptionStringA(RegistrarAorKey);
  if (!strAor.IsEmpty()) {
    memset(&command, 0, sizeof(command));
    command.m_type = OpalCmdRegistration;

    command.m_param.m_registrationInfo.m_protocol = "sip";

    command.m_param.m_registrationInfo.m_identifier = strAor;

    CStringA strHost = GetOptionStringA(RegistrarHostKey);
    command.m_param.m_registrationInfo.m_hostName = strHost;

    CStringA strAuthUser = GetOptionStringA(RegistrarUserKey);
    command.m_param.m_registrationInfo.m_authUserName = strAuthUser;

    CStringA strPassword = GetOptionStringA(RegistrarPassKey);
    command.m_param.m_registrationInfo.m_password = strPassword;

    CStringA strRealm = GetOptionStringA(RegistrarRealmKey);
    command.m_param.m_registrationInfo.m_adminEntity = strRealm;

    command.m_param.m_registrationInfo.m_timeToLive = 300;

    if ((response = OpalSendMessage(m_opal, &command)) == NULL || response->m_type == OpalIndCommandError)
      ErrorBox(IDS_REGISTRATION_FAIL);
    OpalFreeMessage(response);
  }
}


void CMobileOpalDlg::ErrorBox(UINT strId)
{
  CString text(MAKEINTRESOURCE(strId));
  MessageBox(text, NULL, MB_OK|MB_ICONEXCLAMATION);
}


void CMobileOpalDlg::SetStatusText(UINT ids)
{
  CString text(MAKEINTRESOURCE(ids));
  m_ctrlStatus.SetWindowText(text);
  m_ctrlStatus.UpdateWindow();
}


void CMobileOpalDlg::SetCallButton(bool enabled, UINT strId)
{
  HWND hWndMB = SHFindMenuBar(GetSafeHwnd());
  if (hWndMB == NULL)
    return;

  TBBUTTONINFO tbi;
  memset (&tbi, 0, sizeof (tbi));
  tbi.cbSize = sizeof (tbi);
  tbi.dwMask = TBIF_STATE;
  if (!::SendMessage(hWndMB, TB_GETBUTTONINFO, ID_CALL_ANSWER, (LPARAM)&tbi)) 
    return;

  if (enabled)
    tbi.fsState |= TBSTATE_ENABLED;
  else
    tbi.fsState &= ~TBSTATE_ENABLED;

  CString text;
  if (strId != 0 && text.LoadString(strId)) {
    tbi.dwMask |= TBIF_TEXT;
    tbi.pszText = (LPWSTR)(const TCHAR *)text;
  }
  ::SendMessage(hWndMB, TB_SETBUTTONINFO, ID_CALL_ANSWER, (LPARAM)&tbi);
}


void CMobileOpalDlg::OnTimer(UINT_PTR nIDEvent)
{
  if (nIDEvent == TimerID && m_opal != NULL) {
    OpalMessage * message = OpalGetMessage(m_opal, 0);
    if (message != NULL) {
      switch (message->m_type) {
        case OpalIndRegistration :
          if (message->m_param.m_registrationStatus.m_error == NULL ||
              message->m_param.m_registrationStatus.m_error[0] == '\0')
            SetStatusText(IDS_REGISTERED);
          else {
            CString text(message->m_param.m_registrationStatus.m_error);
            m_ctrlStatus.SetWindowText(text);
          }
          break;

        case OpalIndIncomingCall :
          SetStatusText(IDS_INCOMING_CALL);
          SetCallButton(true, IDS_ANSWER);
          m_ctrlAddress.EnableWindow(false);
          ShowWindow(true);
          BringWindowToTop();
          break;

        case OpalIndAlerting :
          SetStatusText(IDS_RINGING);
          break;

        case OpalIndEstablished :
          SetStatusText(IDS_ESTABLISHED);
          break;

        case OpalIndUserInput :
          if (message->m_param.m_userInput.m_userInput != NULL) {
            CString text(message->m_param.m_callCleared.m_reason);
            m_ctrlStatus.SetWindowText(text);
          }
          break;

        case OpalIndCallCleared :
          SetCallButton(true, IDS_CALL);
          m_ctrlAddress.EnableWindow(true);
          m_currentCallToken.Empty();

          if (message->m_param.m_callCleared.m_reason == NULL)
            SetStatusText(IDS_READY);
          else {
            CString text(message->m_param.m_callCleared.m_reason);
            m_ctrlStatus.SetWindowText(text);
          }
      }
      OpalFreeMessage(message);
    }
  }

  CDialog::OnTimer(nIDEvent);
}


void CMobileOpalDlg::OnOK()
{
#if WIN32_PLATFORM_WFSP
  // For Smartphone this is the centre button
  OnCallAnswer();
#else
  // For Pocket PC this is the OK button in the top line
  OnCancel();
#endif
}


void CMobileOpalDlg::OnCancel()
{
  CString msg(MAKEINTRESOURCE(IDS_ASK_EXIT));
  if (MessageBox(msg, NULL, MB_YESNO|MB_ICONQUESTION) == IDYES)
    OnExit();
}


void CMobileOpalDlg::OnExit()
{
  if (m_opal != NULL) {
    KillTimer(TimerID);
    m_ctrlAddress.EnableWindow(false);
    SetStatusText(IDS_SHUTTING_DOWN);
    UpdateWindow();

    OpalShutDown(m_opal);
    m_opal = NULL;
  }

  CDialog::OnCancel();
}


void CMobileOpalDlg::OnCallAnswer()
{
  if (!UpdateData())
    return;

  if (m_callAddress.IsEmpty())
    return;

  CStringA utfAddress((const TCHAR *)m_callAddress);

  OpalMessage command;
  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdSetUpCall;
  command.m_param.m_callSetUp.m_partyB = utfAddress;
  OpalMessage * response = OpalSendMessage(m_opal, &command);
  if (response != NULL) {
    if (response->m_type == OpalIndCommandError)
      ErrorBox(IDS_CALL_START_FAIL);
    else {
      m_currentCallToken = response->m_param.m_callSetUp.m_callToken;
      SetStatusText(IDS_CALLING);
      SetCallButton(true, IDS_HANGUP);
    }
    OpalFreeMessage(response);
  }
}


void CMobileOpalDlg::OnChangedAddress()
{
  SetCallButton(m_ctrlAddress.GetWindowTextLength() > 0);
}


void CMobileOpalDlg::OnMenuOptionsGeneral()
{
  COptionsGeneral dlg;
  dlg.m_strUsername = GetOptionString(UserNameKey);
  dlg.m_strDisplayName = GetOptionString(DisplayNameKey);
  dlg.m_strStunServer = GetOptionString(STUNServerKey);
  if (dlg.DoModal() == IDOK) {
    SetOptionString(UserNameKey, dlg.m_strUsername);
    SetOptionString(DisplayNameKey, dlg.m_strDisplayName);
    SetOptionString(STUNServerKey, dlg.m_strStunServer);
    InitialiseOPAL();
  }
}


void CMobileOpalDlg::OnMenuOptionsH323()
{
  COptionsH323 dlg;
  dlg.m_uiGatekeeperType = GetOptionInt(GkTypeKey);
  dlg.m_strGkId = GetOptionString(GkIdKey);
  dlg.m_strGkHost = GetOptionString(GkHostKey);
  dlg.m_strAlias = GetOptionString(GkAliasKey);
  dlg.m_strAuthUser = GetOptionString(GkAuthUserKey);
  dlg.m_strPassword = GetOptionString(GkPasswordKey);
  if (dlg.DoModal() == IDOK) {
    SetOptionInt(GkTypeKey, dlg.m_uiGatekeeperType);
    SetOptionString(GkIdKey, dlg.m_strGkId);
    SetOptionString(GkHostKey, dlg.m_strGkHost);
    SetOptionString(GkAliasKey, dlg.m_strAlias);
    SetOptionString(GkAuthUserKey, dlg.m_strAuthUser);
    SetOptionString(GkPasswordKey, dlg.m_strPassword);
    InitialiseOPAL();
  }
}


void CMobileOpalDlg::OnMenuOptionsSIP()
{
  COptionsSIP dlg;
  dlg.m_strAddressOfRecord = GetOptionString(RegistrarAorKey);
  dlg.m_strHostName = GetOptionString(RegistrarHostKey);
  dlg.m_strAuthUser = GetOptionString(RegistrarUserKey);
  dlg.m_strPassword = GetOptionString(RegistrarPassKey);
  dlg.m_strRealm = GetOptionString(RegistrarRealmKey);
  if (dlg.DoModal() == IDOK) {
    SetOptionString(RegistrarAorKey, dlg.m_strAddressOfRecord);
    SetOptionString(RegistrarHostKey, dlg.m_strHostName);
    SetOptionString(RegistrarUserKey, dlg.m_strAuthUser);
    SetOptionString(RegistrarPassKey, dlg.m_strPassword);
    SetOptionString(RegistrarRealmKey, dlg.m_strRealm);
    InitialiseOPAL();
  }
}
