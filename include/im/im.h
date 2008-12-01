/*
 * im_mf.h
 *
 * Media formats for Instant Messaging
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

#ifndef OPAL_IM_IM_MF_H
#define OPAL_IM_IM_MF_H

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_SIP

/////////////////////////////////////////////////////////
//
//  SDP media description for IM
//

#include <sip/sdp.h>

class SDPIMMediaDescription : public SDPMediaDescription
{
  PCLASSINFO(SDPIMMediaDescription, SDPMediaDescription);
  public:
    SDPIMMediaDescription(const OpalTransportAddress & address);

    virtual PString GetSDPMediaType() const;
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual PString GetSDPPortList() const;
    virtual bool PrintOn(ostream & str, const PString & connectString) const;
    void SetAttribute(const PString & attr, const PString & value);
    void ProcessMediaOptions(SDPMediaFormat & /*sdpFormat*/, const OpalMediaFormat & mediaFormat);
  
  protected:
    PStringToString msrpAttributes;
};

#endif

#endif // OPAL_MSRP_MSRP_H
