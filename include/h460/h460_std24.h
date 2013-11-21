/*
 * h460_std24.h
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
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_H460_STD24_H
#define OPAL_H460_STD24_H

#include <opal_config.h>

#if OPAL_H460_NAT

#include <h460/h4601.h>
#include <ptclib/pstun.h>


#if _MSC_VER
#pragma once
#endif


class PSTUNClient;


class PNatMethod_H46024 : public PSTUNClient
{
  PCLASSINFO(PNatMethod_H46024, PSTUNClient);
  public:
    enum
    {
      DefaultPriority = 10
    };
    PNatMethod_H46024(unsigned priority = DefaultPriority);

    static const char * MethodName();
    virtual PCaselessString GetMethodName() const;

    virtual bool IsAvailable(const PIPSocket::Address & binding, PObject * context);
};


///////////////////////////////////////////////////////////////////////////////

class H460_FeatureStd24 : public H460_Feature
{
    PCLASSINFO_WITH_CLONE(H460_FeatureStd24, H460_Feature);
  public:
    H460_FeatureStd24();

    static const H460_FeatureID & ID();
    virtual bool Initialise(H323EndPoint & ep, H323Connection * con);

    /// Values as per H.460.24 Table 9
    enum MediaStrategyIndicator
    {
      e_Unknown,
      e_NoAssist,
      e_LocalMediaMaster,
      e_RemoteMediaMaster,
      e_LocalProxy,
      e_RemoteProxy,
      e_FullProxy,
      e_ProbeSameNAT,      // Annex A
      e_ProbeExternalNAT,  // Annex B
      e_MediaFailure = 100
    };

    // Messages
    virtual bool OnSendAdmissionRequest(H460_FeatureDescriptor & pdu);
    virtual void OnReceiveAdmissionConfirm(const H460_FeatureDescriptor & pdu);
    virtual void OnReceiveAdmissionReject(const H460_FeatureDescriptor & pdu);

    virtual bool OnSendSetup_UUIE(H460_FeatureDescriptor & pdu);
    virtual void OnReceiveSetup_UUIE(const H460_FeatureDescriptor & pdu);

    MediaStrategyIndicator GetStrategy() const { return m_strategy; }
    bool IsDisabledH46019() const;

  protected:
    void HandleStrategy(const H460_FeatureDescriptor & pdu);

    PNatMethod_H46024    * m_natMethod;
    MediaStrategyIndicator m_strategy;
};


#endif // OPAL_H460_NAT

#endif // OPAL_H460_STD24_H
