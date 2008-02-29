/*
 * allcodecs.h
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
 * Revision 1.2010.4.1  2006/03/16 07:06:00  csoutheren
 * Initial support for audio plugins
 *
 * Revision 2.9  2006/01/01 19:19:32  dsandras
 * Added RFC2190 H.263 codec thanks to Salyens. Many thanks!
 *
 * Revision 2.8  2004/05/15 12:53:40  rjongbloed
 * Fixed incorrect laoding of H.323 capability for G.726
 *
 * Revision 2.7  2004/02/19 10:46:43  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.6  2004/02/17 08:48:57  csoutheren
 * Disabled VoiceAge G.729 codec on Linux
 *
 * Revision 2.5  2003/06/02 04:04:54  rjongbloed
 * Changed to use new autoconf system
 *
 * Revision 2.4  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.3  2002/11/10 11:33:16  robertj
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

#include <opal/buildopts.h>

#include <codec/g711codec.h>
OPAL_REGISTER_G711();

//#if VOICE_AGE_G729A
//#include <codec/g729codec.h>
//OPAL_REGISTER_G729();
//#endif

//#include <codec/gsmcodec.h>
//OPAL_REGISTER_GSM0610();

//#include <codec/g726codec.h>
//OPAL_REGISTER_G726();

//#include <codec/mscodecs.h>
//OPAL_REGISTER_MSCODECS();

//#include <codec/lpc10codec.h>
//OPAL_REGISTER_LPC10();

//#include <codec/speexcodec.h>
//OPAL_REGISTER_SPEEX();

//#include <codec/ilbccodec.h>
//OPAL_REGISTER_iLBC();

//#include <codec/vidcodec.h>
//OPAL_REGISTER_UNCOMPRESSED_VIDEO();

#include <codec/h261codec.h>
OPAL_REGISTER_H261();

#ifdef RFC2190_AVCODEC
#include <codec/h263codec.h>
OPAL_REGISTER_H263();
#endif // RFC2190_AVCODEC

#endif // __CODEC_ALLCODECS_H


/////////////////////////////////////////////////////////////////////////////
