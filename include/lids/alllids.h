/*
 * alllids.h
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: alllids.h,v $
 * Revision 1.2002  2003/03/24 07:18:29  robertj
 * Added registration system for LIDs so can work with various LID types by
 *   name instead of class instance.
 *
 */

#ifndef __LIDS_ALLLIDS_H
#define __LIDS_ALLLIDS_H

#if HAS_IXJ
#include <lids/ixjlid.h>
OPAL_REGISTER_IXJ();
#endif

#if HAS_VPB
#include <lids/vpblib.h>
OPAL_REGISTER_VPB();
#endif

#if HAS_VBLASTER
#include <lids/vblasterlid.h>
OPAL_REGISTER_VBLASTER();
#endif


#endif // __LIDS_ALLLIDS_H


/////////////////////////////////////////////////////////////////////////////
