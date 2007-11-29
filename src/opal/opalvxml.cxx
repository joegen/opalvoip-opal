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

#include <opal/opalvxml.h>

#include <opal/connection.h>
#include <codec/opalwavfile.h>


///////////////////////////////////////////////////////////////

#if P_EXPAT

OpalVXMLSession::OpalVXMLSession(OpalConnection * _conn, PTextToSpeech * tts, PBoolean autoDelete)
  : PVXMLSession(tts, autoDelete),
    conn(_conn)
{
  if (tts == NULL) {
    PFactory<PTextToSpeech>::KeyList_T engines = PFactory<PTextToSpeech>::GetKeyList();
    if (engines.size() != 0) {
      PFactory<PTextToSpeech>::Key_T name;
#ifdef _WIN32
      name = "Microsoft SAPI";
      if (std::find(engines.begin(), engines.end(), name) == engines.end())
#endif
        name = engines[0];
      SetTextToSpeech(name);
    }
  }
}


PBoolean OpalVXMLSession::Close()
{
  if (!IsOpen())
    return PTrue;

  PBoolean ok = PVXMLSession::Close();
  conn->Release();
  return ok;
}


void OpalVXMLSession::OnEndSession()
{
  conn->Release();
}


PWAVFile * OpalVXMLSession::CreateWAVFile(const PFilePath & fn, PFile::OpenMode mode, int opts, unsigned fmt)
{ 
  return new OpalWAVFile(fn, mode, opts, fmt); 
}


#endif // P_EXPAT


// End of File /////////////////////////////////////////////////////////////
