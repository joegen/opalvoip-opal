/*
 * g711codec.h
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
 * $Log: allcodecs.h,v $
 * Revision 1.2004  2002/11/10 11:33:16  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.2  2002/07/01 04:56:29  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.1  2001/08/01 05:03:26  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 *
 */

#ifndef __CODEC_ALLCODECS_H
#define __CODEC_ALLCODECS_H

#include <codec/g711codec.h>
OPAL_REGISTER_G711();

#include <codec/g729codec.h>
OPAL_REGISTER_G729();

#include <codec/gsmcodec.h>
OPAL_REGISTER_GSM0610();

#include <codec/mscodecs.h>
OPAL_REGISTER_MSCODECS();

#include <codec/lpc10codec.h>
OPAL_REGISTER_LPC10();

#include <codec/speexcodec.h>
OPAL_REGISTER_SPEEX();

#include <codec/h261codec.h>
OPAL_REGISTER_H261();


#endif // __CODEC_ALLCODECS_H


/////////////////////////////////////////////////////////////////////////////
