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
 * $Id$
 */

#ifndef _OPAL_OPALVXML_H
#define _OPAL_OPALVXML_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <ptclib/vxml.h>


class OpalConnection;


//////////////////////////////////////////////////////////////////


#if P_EXPAT

class PTextToSpeech;

class OpalVXMLSession : public PVXMLSession 
{
  PCLASSINFO(OpalVXMLSession, PVXMLSession);
  public:
    OpalVXMLSession(
      OpalConnection * _conn,
      PTextToSpeech * tts = NULL,
      BOOL autoDelete = FALSE
    );

    virtual BOOL Close();
    virtual void OnEndSession();

    virtual PWAVFile * CreateWAVFile(
      const PFilePath & fn,
      PFile::OpenMode mode,
      int opts,
      unsigned fmt
    );

  protected:
    OpalConnection * conn;
};

#endif

#endif // _OPAL_OPALVXML_H


// End of File ///////////////////////////////////////////////////////////////
