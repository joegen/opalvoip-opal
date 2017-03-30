/*
 * h460_std23.h
 *
 * Copyright (c) 2009 ISVO (Asia) Pte Ltd. All Rights Reserved.
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
 * The Original Code is derived from and used in conjunction with the 
 * H323Plus Project (www.h323plus.org/)
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte Ltd.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *                 Robert Jongbloed (robertj@voxlucida.com.au).
 */

#ifndef OPAL_H460_STD23_H
#define OPAL_H460_STD23_H

#include <opal_config.h>

#if OPAL_H460_NAT

#include <h460/h4601.h>

#if _MSC_VER
#pragma once
#endif


class PNatMethod_H46024;


class H460_FeatureStd23 : public H460_Feature
{
    PCLASSINFO_WITH_CLONE(H460_FeatureStd23, H460_Feature);
  public:
    H460_FeatureStd23();

    static const H460_FeatureID & ID();
    virtual bool Initialise(H323EndPoint & ep, H323Connection * con);

    // H.225.0 Messages
    virtual bool OnSendGatekeeperRequest(H460_FeatureDescriptor & pdu);
    virtual bool OnSendRegistrationRequest(H460_FeatureDescriptor & pdu, bool lightweight);
    virtual void OnReceiveRegistrationConfirm(const H460_FeatureDescriptor & pdu);

  #ifdef H323_UPnP
    void InitialiseUPnP();
    bool UsePnP();
  #endif

  protected:
#if OPAL_H460_24
    PNatMethod_H46024 * m_natMethod;
#endif
    bool                m_applicationLevelGateway;
};


#endif // OPAL_H460_NAT

#endif // OPAL_H460_STD23_H
