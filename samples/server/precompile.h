/*
 * precompile.h
 *
 * PWLib application source file for OPAL Server
 *
 * Precompiled header generation file.
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 */

#include <ptlib.h>
#include <ptclib/podbc.h>

#include <opal/console_mgr.h>
#include <opal/patch.h>
#include <h323/gkclient.h>
#include <h323/gkserver.h>
#include <ep/sockep.h>

#if OPAL_PTLIB_SSL
#include <ptclib/shttpsvc.h>
typedef PSecureHTTPServiceProcess MyProcessAncestor;
#else
#include <ptclib/httpsvc.h>
typedef PHTTPServiceProcess MyProcessAncestor;
#endif


// End of File ///////////////////////////////////////////////////////////////
