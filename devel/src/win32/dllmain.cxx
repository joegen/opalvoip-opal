/*
 * dllmain.cxx
 *
 * DLL main entry point for OpenH323.dll
 *
 * Open H323 Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: dllmain.cxx,v $
 * Revision 2.7  2007/04/08 02:59:23  ykiryanov
 * *** empty log message ***
 *
 * Revision 2.6  2006/10/02 13:30:52  rjongbloed
 * Added LID plug ins
 *
 * Revision 2.5  2003/03/24 07:18:30  robertj
 * Added registration system for LIDs so can work with various LID types by
 *   name instead of class instance.
 *
 * Revision 2.4  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.3  2002/11/11 06:54:30  robertj
 * Added correct flag for including static global variables.
 *
 * Revision 2.2  2002/11/10 23:04:29  robertj
 * Added flag to assure linking of static variables (codecs etc).
 *
 * Revision 2.1  2001/08/01 05:53:31  robertj
 * Fixed loading of transcoders from static library.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.3  2002/11/10 23:14:40  robertj
 * Made sure static variables are included in DLL
 *
 * Revision 1.2  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.1  2000/04/13 00:02:01  robertj
 * Added ability to create DLL version of library.
 *
 */

#include <ptlib.h>

///////////////////////////////////////////////////////////////////////////////


#ifndef _WIN32_WCE
HINSTANCE PDllInstance;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
#else
HANDLE PDllInstance;
BOOL WINAPI DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID)
#endif
{
  if (fdwReason == DLL_PROCESS_ATTACH)
    PDllInstance = hinstDLL;
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
