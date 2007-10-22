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
 * $Log: opalvxml.cxx,v $
 * Revision 2.13  2007/09/18 09:53:31  rjongbloed
 * Fixed memory leak in IVR connection
 *
 * Revision 2.12  2006/08/11 07:52:01  csoutheren
 * Fix problem with media format factory in VC 2005
 * Fixing problems with Speex codec
 * Remove non-portable usages of PFactory code
 *
 * Revision 2.11  2004/07/17 09:45:06  rjongbloed
 * Fixed problem if Close() is called before Open(), would shut down connection.
 *
 * Revision 2.10  2004/06/30 12:48:23  rjongbloed
 * Rewrite of plug in system to use single global variable for all factories to avoid all sorts
 *   of issues with startup orders and Windows DLL multiple instances.
 *
 * Revision 2.9  2004/06/22 11:01:16  csoutheren
 * Fixed to compile on Linux
 *
 * Revision 2.8  2004/06/22 06:27:58  csoutheren
 * Updated for new abstract factory implementation
 *
 * Revision 2.7  2004/04/18 13:36:15  rjongbloed
 * Added autmatic inclusion of a text to speach converter, if available.
 *
 * Revision 2.6  2003/03/18 04:43:27  robertj
 * Fixed pragma implementation
 *
 * Revision 2.5  2003/03/17 23:36:19  robertj
 * Fixed pragma
 *
 * Revision 2.4  2003/03/07 05:48:52  robertj
 * Added OnEndSession() to automatically close connection.
 *
 * Revision 2.3  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.2  2002/11/11 06:51:12  robertj
 * Fixed errors after adding P_EXPAT flag to build.
 *
 * Revision 2.1  2002/11/10 11:33:20  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 1.18  2002/12/10 23:50:25  robertj
 * Fixed some tracing issues
 *
 * Revision 1.17  2002/08/27 02:21:31  craigs
 * Added silence detection capability to fake G.723.1codec
 *
 * Revision 1.16  2002/08/15 04:55:26  robertj
 * Fixed shutdown problems with closing vxml session, leaks a thread.
 * Fixed potential problems with indirect channel Close() function.
 *
 * Revision 1.15  2002/08/15 02:19:42  robertj
 * Adjusted trace log levels for G.723.1 file codec read/write tracking.
 *
 * Revision 1.14  2002/08/07 13:53:05  craigs
 * Fixed problem with included opalvxml.h thanks to Vladmir Toncar
 *
 * Revision 1.13  2002/08/06 05:10:59  craigs
 * Moved most of stuff to ptclib
 *
 * Revision 1.12  2002/08/05 09:43:30  robertj
 * Added pragma interface/implementation
 * Moved virtual into .cxx file
 *
 * Revision 1.11  2002/07/29 15:10:36  craigs
 * Added autodelete option to PlayFile
 *
 * Revision 1.10  2002/07/29 12:54:42  craigs
 * Removed usages of cerr
 *
 * Revision 1.9  2002/07/18 04:17:12  robertj
 * Moved virtuals to source and changed name of G.723.1 file capability
 *
 * Revision 1.8  2002/07/10 13:16:58  craigs
 * Moved some VXML classes from Opal back into PTCLib
 * Added ability to repeat outputted data
 *
 * Revision 1.7  2002/07/09 08:48:41  craigs
 * Fixed trace messages
 *
 * Revision 1.6  2002/07/05 06:34:04  craigs
 * Changed comments and trace messages
 *
 * Revision 1.5  2002/07/03 04:58:13  robertj
 * Changed for compatibility with older GNU compilers
 *
 * Revision 1.4  2002/07/02 06:32:51  craigs
 * Added recording functions
 *
 * Revision 1.3  2002/06/28 02:42:11  craigs
 * Fixed problem with G.723.1 file naming conventions
 * Fixed problem with incorrect file open mode
 *
 * Revision 1.2  2002/06/28 01:24:03  robertj
 * Fixe dproblem with compiling without expat library.
 *
 * Revision 1.1  2002/06/27 05:44:11  craigs
 * Initial version
 *
 * Revision 1.2  2002/06/26 09:05:28  csoutheren
 * Added ability to utter various "sayas" types within prompts
 *
 * Revision 1.1  2002/06/26 01:13:53  csoutheren
 * Disassociated VXML and Opal/OpenH323 specific elements
 *
 * Revision 1.2  2002/06/21 08:18:22  csoutheren
 * Added start of grammar handling
 *
 * Revision 1.1  2002/06/20 06:35:44  csoutheren
 * Initial version
 *
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

OpalVXMLSession::OpalVXMLSession(OpalConnection * _conn, PTextToSpeech * tts, BOOL autoDelete)
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


BOOL OpalVXMLSession::Close()
{
  if (!IsOpen())
    return TRUE;

  BOOL ok = PVXMLSession::Close();
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
