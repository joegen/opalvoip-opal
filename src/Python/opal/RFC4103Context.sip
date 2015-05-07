/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <im/rfc4103.h>
%End

class RFC4103Context : PObject
{
  public:
    RFC4103Context();
    RFC4103Context(const OpalMediaFormat & fmt);
    void SetMediaFormat(const OpalMediaFormat & fmt);
    RTP_DataFrameList ConvertToFrames(const PString & contentType, const T140String & body);

    OpalMediaFormat m_mediaFormat;
    PMutex m_mutex;
    WORD   m_sequence;
    DWORD  m_baseTimeStamp;
    PTime  m_baseTime;
};

