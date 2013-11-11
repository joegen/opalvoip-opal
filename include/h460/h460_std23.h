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
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_H460_STD23_H
#define OPAL_H460_STD23_H

#include <opal_config.h>

#if OPAL_H460_NAT

#include <h460/h4601.h>

#if _MSC_VER
#pragma once
#endif

class H323EndPoint;
class H460_FeatureStd23;


class PNatMethod_H46024  : public PNatMethod
{
    PCLASSINFO(PNatMethod_H46024, PNatMethod);
  public:
    enum { DefaultPriority = 40 };
    PNatMethod_H46024(unsigned priority = DefaultPriority);

    static const char * MethodName();
    virtual PCaselessString GetMethodName() const;

    virtual PString GetServer() const;
    virtual bool IsAvailable(const PIPSocket::Address & binding);

  protected:
    virtual void InternalUpdate();

    H323EndPoint     * m_endpoint;
  friend class H460_FeatureStd23;
};


///////////////////////////////////////////////////////////////////////////////

class H460_FeatureStd23 : public H460_Feature
{
    PCLASSINFO(H460_FeatureStd23, H460_Feature);

  public:
    H460_FeatureStd23();

    // Universal Declarations Every H460 Feature should have the following
    static Purpose GetPluginPurpose()      { return ForGatekeeper; };
    virtual Purpose GetPurpose() const     { return GetPluginPurpose(); };

    virtual bool Initialise(H323EndPoint & ep, H323Connection * con);

    // H.225.0 Messages
    virtual bool OnSendGatekeeperRequest(H460_FeatureDescriptor & pdu);
    virtual bool OnSendRegistrationRequest(H460_FeatureDescriptor & pdu);
    virtual void OnReceiveRegistrationConfirm(const H460_FeatureDescriptor & pdu);

  #ifdef H323_UPnP
    void InitialiseUPnP();
    bool UsePnP();
  #endif

  protected:
    bool DetectALG(const PIPSocket::Address & detectAddress);

    void DelayedReRegistration();

    PNatMethod_H46024 * m_natMethod;
    bool                m_alg;
};


////////////////////////////////////////////////////////

class H460_FeatureStd24 : public H460_Feature
{
    PCLASSINFO(H460_FeatureStd24, H460_Feature);

  public:
    H460_FeatureStd24();

    // Universal Declarations Every H460 Feature should have the following
    static Purpose GetPluginPurpose()  { return ForConnection; };
    virtual Purpose GetPurpose() const { return GetPluginPurpose(); };

    virtual bool IsNegotiated() const;

    enum NatInstruct {
      e_unknown,
      e_noassist,
      e_localMaster,
      e_remoteMaster,
      e_localProxy,
      e_remoteProxy,
      e_natFullProxy,
      e_natAnnexA,                // Same NAT
      e_natAnnexB,                // NAT Offload
      e_natFailure = 100
    };

    static PString GetNATStrategyString(NatInstruct method);

    enum H46024NAT {
      e_default,        // This will use the underlying NAT Method
      e_enable,        // Use H.460.24 method (STUN)
      e_AnnexA,       // Disable H.460.24 method but initiate AnnexA
      e_AnnexB,        // Disable H.460.24 method but initiate AnnexB
      e_disable        // Disable all and remote will do the NAT help        
    };

    // Messages
    virtual PBoolean OnSendAdmissionRequest(H460_FeatureDescriptor & pdu);
    virtual void OnReceiveAdmissionConfirm(const H460_FeatureDescriptor & pdu);
    virtual void OnReceiveAdmissionReject(const H460_FeatureDescriptor & pdu);

    virtual PBoolean OnSendSetup_UUIE(H460_FeatureDescriptor & pdu);
    virtual void OnReceiveSetup_UUIE(const H460_FeatureDescriptor & pdu);

  protected:
    void HandleNATInstruction(NatInstruct natconfig);
    void SetNATMethods(H46024NAT state);

    NatInstruct natconfig;
    PMutex h460mute;
    int nattype;
    bool useAlternate;
};


inline ostream & operator<<(ostream & strm, H460_FeatureStd24::NatInstruct method) { return strm << H460_FeatureStd24::GetNATStrategyString(method); }


#endif // OPAL_H460_NAT

#endif // OPAL_H460_STD23_H
