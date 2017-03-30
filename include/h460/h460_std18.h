/*
 * h460_std18.h
 *
 * Copyright (c) 2008 ISVO (Asia) Pte. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * triple-IT. http://www.triple-it.nl.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *                 Robert Jongbloed (robertj@voxlucida.com.au).
 */

#ifndef OPAL_H460_STD18_H
#define OPAL_H460_STD18_H

#include <h460/h4601.h>

#if OPAL_H460_NAT

#if _MSC_VER
#pragma once
#endif 


class H323SignalPDU;


/////////////////////////////////////////////////////////////////

class H460_FeatureStd18 : public H460_Feature
{
    PCLASSINFO_WITH_CLONE(H460_FeatureStd18, H460_Feature);
  public:
    H460_FeatureStd18();

    static const H460_FeatureID & ID();
    virtual bool Initialise(H323EndPoint & ep, H323Connection * con);
    virtual bool IsNegotiated() const;

    // H.225.0 Messages
    virtual bool OnSendGatekeeperRequest  (H460_FeatureDescriptor & /*pdu*/                  ) { return true; }
    virtual bool OnSendRegistrationRequest(H460_FeatureDescriptor & /*pdu*/, bool /*lightweight*/) { return true; /*!lightweight; - Bug in GnuGK if we do this */ }
    virtual bool OnSendSetup_UUIE         (H460_FeatureDescriptor & /*pdu*/                  ) { return true; }

    virtual void OnReceiveServiceControlIndication(const H460_FeatureDescriptor & pdu);

    // Custom functions
    bool OnStartControlChannel();

  protected:
    void ConnectThreadMain(H323TransportAddress address, OpalGloballyUniqueID callId);

    OpalGloballyUniqueID m_lastCallIdentifer;
};


#endif // OPAL_H460_NAT

#endif // OPAL_H460_STD18_H
