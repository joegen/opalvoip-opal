/*
 * h460.h
 *
 * Virteos H.460 Implementation for the h323plus Library.
 *
 * Virteos is a Trade Mark of ISVO (Asia) Pte Ltd.
 *
 * Copyright (c) 2004 ISVO (Asia) Pte Ltd. All Rights Reserved.
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
 * OpenH323 Project (www.openh323.org/)
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte Ltd.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *                 Robert Jongbloed (robertj@voxlucida.com.au).
 */

#ifndef OPAL_H460_H460_H
#define OPAL_H460_H460_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_H460

#include <h323/q931.h>

class H460_MessageType
{
  public:
    enum Code {
      e_gatekeeperRequest,
      e_gatekeeperConfirm,
      e_gatekeeperReject,
      e_registrationRequest,
      e_lightweightRegistrationRequest,
      e_registrationConfirm,
      e_registrationReject,
      e_admissionRequest,
      e_admissionConfirm,
      e_admissionReject,
      e_locationRequest,
      e_locationConfirm,
      e_locationReject,
      e_nonStandardMessage,
      e_serviceControlIndication,
      e_serviceControlResponse,
      e_unregistrationRequest,
      e_inforequest,
      e_inforequestresponse,
      e_disengagerequest,
      e_disengageconfirm,
      e_setup                       = 0x100|Q931::SetupMsg,
      e_callProceeding	            = 0x100|Q931::CallProceedingMsg,
      e_connect	                    = 0x100|Q931::ConnectMsg,
      e_alerting                    = 0x100|Q931::AlertingMsg,
      e_facility                    = 0x100|Q931::FacilityMsg,
      e_releaseComplete	            = 0x100|Q931::ReleaseCompleteMsg
    } m_code;

    H460_MessageType(Code code) : m_code(code) { }
    H460_MessageType(Q931::MsgTypes code) : m_code((Code)(0x100|code)) { }

    operator Code() const { return m_code; }

#if PTRACING
    friend ostream & operator<<(ostream & strm, H460_MessageType pduType);
#endif
};

#endif // OPAL_H460

#endif // OPAL_H460_H460_H
