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

#include <opal/ivr.h>

#include <opal/call.h>
#include <codec/opalwavfile.h>


///////////////////////////////////////////////////////////////

#if OPAL_PTLIB_VXML

OpalVXMLSession::OpalVXMLSession(OpalIVRConnection & conn, PTextToSpeech * tts, PBoolean autoDelete)
  : PVXMLSession(tts, autoDelete),
    m_connection(conn)
{
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
  m_connection.Release();
}


void OpalVXMLSession::OnTransfer(const PString & destination, bool bridged)
{
  if (!bridged)
    m_connection.GetCall().Transfer(destination, &m_connection);
  else {
    PSafePtr<OpalConnection> otherConnection = m_connection.GetOtherPartyConnection();
    if (otherConnection != NULL)
      m_connection.GetCall().Transfer(destination, &*otherConnection);
    else {
      PTRACE(1, "IVR\tAttempt to make transfer when no second connecion in call");
    }
  }
}


PWAVFile * OpalVXMLSession::CreateWAVFile(const PFilePath & fn, PFile::OpenMode mode, int opts, unsigned fmt)
{ 
  return new OpalWAVFile(fn, mode, opts, fmt); 
}


#endif // #if OPAL_PTLIB_EXPAT


// End of File /////////////////////////////////////////////////////////////
