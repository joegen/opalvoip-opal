// MfcEndPoint.cpp: implementation of the CMfcEndPoint class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mfc.h"
#include "MfcEndPoint.h"
#include "MfcDlg.h"

#include <gsmcodec.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMfcEndPoint::CMfcEndPoint()
{
  m_dialog = NULL;
}

CMfcEndPoint::~CMfcEndPoint()
{

}

BOOL CMfcEndPoint::Initialise(CMfcDlg *dlg)
{
  m_dialog = dlg;

  SetCapability(0, 0, new H323_GSM0610Capability);
  SetCapability(0, 0, new H323_G711Capability(H323_G711Capability::muLaw));
  SetCapability(0, 0, new H323_G711Capability(H323_G711Capability::ALaw));
  H323_UserInputCapability::AddAllCapabilities(capabilities, 0, 0);

  return StartListener(new H323ListenerTCP(*this));
}

H323Connection * CMfcEndPoint::CreateConnection(unsigned int refID)
{
  return new H323Connection(*this, refID);
}

void CMfcEndPoint::OnConnectionEstablished(H323Connection & connection, const PString & token)
{
  m_dialog->m_token = token;
  m_dialog->m_call.EnableWindow(FALSE);
  m_dialog->m_answer.EnableWindow(FALSE);
  m_dialog->m_refuse.EnableWindow(FALSE);
  m_dialog->m_hangup.EnableWindow();
  m_dialog->m_caller.SetWindowText("In call with " + connection.GetRemotePartyName());
}

void CMfcEndPoint::OnConnectionCleared(H323Connection &, const PString &)
{
  m_dialog->m_answer.EnableWindow(FALSE);
  m_dialog->m_refuse.EnableWindow(FALSE);
  m_dialog->m_hangup.EnableWindow(FALSE);
  m_dialog->m_call.EnableWindow();
  m_dialog->m_caller.SetWindowText("");
}

H323Connection::AnswerCallResponse CMfcEndPoint::OnAnswerCall(H323Connection & connection, const PString &caller, const H323SignalPDU &, H323SignalPDU &)
{
  m_dialog->m_token = connection.GetCallToken();
  m_dialog->m_caller.SetWindowText(caller + " is calling.");
  m_dialog->m_answer.EnableWindow();
  m_dialog->m_refuse.EnableWindow();
  m_dialog->m_call.EnableWindow(FALSE);
  return H323Connection::AnswerCallPending;
}
