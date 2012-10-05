/*
 * vxml.cxx
 *
 * VXML control for for Opal
 *
 * A H.323 IVR application.
 *
 * Copyright (C) 2002 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef P_USE_PRAGMA
#pragma implementation "opalvxml.h"
#endif

#include <ptlib.h>

#include <opal/buildopts.h>

#include <ep/ivr.h>

#include <opal/call.h>
#include <codec/opalwavfile.h>


///////////////////////////////////////////////////////////////

#if OPAL_IVR

OpalVXMLSession::OpalVXMLSession(OpalIVRConnection & conn, PTextToSpeech * tts, PBoolean autoDelete)
  : PVXMLSession(tts, autoDelete),
    m_connection(conn)
{
  PTRACE_CONTEXT_ID_FROM(conn);

  if (tts == NULL)
    SetTextToSpeech(PString::Empty());
}


void OpalVXMLSession::OnEndDialog()
{
  m_connection.OnEndDialog();
  PVXMLSession::OnEndDialog();
}


void OpalVXMLSession::OnEndSession()
{
  PTRACE(3, "IVR\tEnd of session, releasing connection.");
  m_connection.Release();
}


bool OpalVXMLSession::OnTransfer(const PString & destination, TransferType type)
{
  switch (type) {
    case BridgedTransfer :
      // We do not make a distinction between bridged and blind transfers

    case BlindTransfer :
      if (m_connection.GetCall().Transfer(destination))
        return true;
      return m_connection.GetCall().Transfer(destination, &m_connection);

    case ConsultationTransfer :
      break;
  }

  // Unsupported
  return false;
}


#endif // OPAL_IVR


// End of File /////////////////////////////////////////////////////////////
