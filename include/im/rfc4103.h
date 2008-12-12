/*
 * rfc4103.h
 *
 * Implementation of RFC 4103 RTP Payload for Text Conversation
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-13 10:24:41 +1100 (Mon, 13 Oct 2008) $
 */

#ifndef OPAL_IM_RFC4103_H
#define OPAL_IM_RFC4103_H

#include <ptlib.h>
#include <opal/buildopts.h>

#include <im/t140.h>
#include <rtp/rtp.h>

class RFC4103Frame : public RTP_DataFrame
{
  public:
    RFC4103Frame();
    RFC4103Frame(const T140String & t140);
};

#endif // OPAL_IM_RFC4103_H
