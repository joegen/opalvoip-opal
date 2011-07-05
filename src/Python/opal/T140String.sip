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
#include <im/t140.h>
%End

class OpalT140MediaType : OpalRTPAVPMediaType 
{
  public:
    OpalT140MediaType();
  
	//this function is not actually implemented in t140.cxx
    //static const OpalMediaType & MediaType();

    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
};


class T140String
{
  public:
    enum {
      ZERO_WIDTH_NO_BREAK = 0xfeff,
      UTF_NEWLINE         = 0x2028,
    };

    T140String();
    T140String(const BYTE * data, PINDEX len);
    T140String(const PBYTEArray & bytes);
    T140String(const char * chars);
    T140String(const PString & str);

    PINDEX GetLength() const;

    PINDEX GetUTFLen(WORD c);
    PINDEX GetUTF(const BYTE * ptr, PINDEX len, WORD & ch);
    PINDEX GetUTF(PINDEX pos, WORD & ch);

    PINDEX AppendUnicode16(WORD c);
    PINDEX AppendUTF(const BYTE * utf, PINDEX utfLen);

    bool AsString(PString & str);

  protected:
    PINDEX SetUTF(BYTE * ptr, WORD c);
};


