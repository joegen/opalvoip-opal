/*
 * h224mediafmt.h
 *
 * H.224 Media Format implementation for the OpenH323 Project.
 *
 * Copyright (c) 2007 Network for Educational Technology, ETH Zurich.
 * Written by Hannes Friederich.
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
 * Contributor(s): ______________________________________.
 *
 * $Log: h224mediafmt.h,v $
 * Revision 1.1.2.1  2007/02/15 15:25:36  hfriederich
 * Add media format and media type for H.224
 *
 *
 */

#ifndef __OPAL_H224_MEDIAFMT_H
#define __OPAL_H224_MEDIAFMT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/mediafmt.h>

#define OPAL_H224_NAME "H.224"

extern const OpalMediaFormat & GetOpalH224();
#define OpalH224 GetOpalH224()

#endif // __OPAL_H224_MEDIAFMT_H

