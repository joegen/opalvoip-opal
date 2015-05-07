/*
 * opalvxml.h
 *
 * Header file for IVR code
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

#ifndef OPAL_OPAL_OPALVXML_H
#define OPAL_OPAL_OPALVXML_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#include <ptclib/vxml.h>


class OpalConnection;


//////////////////////////////////////////////////////////////////


#if OPAL_PTLIB_VXML

class PTextToSpeech;
class OpalIVRConnection;


class OpalVXMLSession : public PVXMLSession 
{
  PCLASSINFO(OpalVXMLSession, PVXMLSession);
  public:
    OpalVXMLSession(
      OpalIVRConnection & conn,
      PTextToSpeech * tts = NULL,
      PBoolean autoDelete = false
    );

    virtual void OnEndDialog();
    virtual void OnEndSession();
    virtual bool OnTransfer(const PString & destination, TransferType type);

  protected:
    OpalIVRConnection & m_connection;
};

#endif

#endif // OPAL_OPAL_OPALVXML_H


// End of File ///////////////////////////////////////////////////////////////
