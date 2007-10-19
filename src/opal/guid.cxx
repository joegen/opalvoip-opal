/*
 * guid.cxx
 *
 * Globally Unique Identifier
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * $Log: guid.cxx,v $
 * Revision 2.5  2006/09/20 05:07:22  csoutheren
 * Changed to use PGloballyUniqueID
 *
 * Revision 2.4  2004/02/19 10:47:06  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.3  2002/11/10 11:33:19  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.2  2002/09/04 06:01:49  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.1  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.15  2003/04/15 03:04:08  robertj
 * Fixed string constructor being able to build non null GUID.
 *
 * Revision 1.14  2002/10/10 05:33:18  robertj
 * VxWorks port, thanks Martijn Roest
 *
 * Revision 1.13  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.12  2001/10/03 03:18:29  robertj
 * Changed to only get (or fake) MAC address once.
 *
 * Revision 1.11  2001/04/05 01:45:13  robertj
 * Fixed MSVC warning.
 *
 * Revision 1.10  2001/04/04 07:46:13  robertj
 * Fixed erros in rading GUID hex string.
 *
 * Revision 1.9  2001/04/04 06:46:39  robertj
 * Fixed errors in time calculation used in GUID.
 *
 * Revision 1.8  2001/03/19 05:52:24  robertj
 * Fixed problem with reading a GUID if there is leading white space.
 * If get error reading GUID then set the stream fail bit.
 *
 * Revision 1.7  2001/03/15 00:25:12  robertj
 * Fixed problem with hex output sign extending unsigned values.
 *
 * Revision 1.6  2001/03/14 05:03:37  robertj
 * Fixed printing of GUID to have bytes as hex instead of characters.
 *
 * Revision 1.5  2001/03/03 00:54:48  yurik
 * Proper fix for filetime routines used in guid calc done for WinCE
 *
 * Revision 1.4  2001/03/02 23:25:49  yurik
 * fixed typo
 *
 * Revision 1.3  2001/03/02 22:50:37  yurik
 * Used PTime for WinCE port instead of non-portable function
 *
 * Revision 1.2  2001/03/02 07:17:41  robertj
 * Compensated for stupid GNU compiler bug.
 *
 * Revision 1.1  2001/03/02 06:59:59  robertj
 * Enhanced the globally unique identifier class.
 *
 */

// replaced by class PGUID in pwlib

#include <ptlib.h>

/////////////////////////////////////////////////////////////////////////////
