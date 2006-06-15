// H460.h:
/*
 * Virteos H.460 Implementation for the OpenH323 Project.
 *
 * Virteos is a Trade Mark of ISVO (Asia) Pte Ltd.
 *
 * Copyright (c) 2004 ISVO (Asia) Pte Ltd. All Rights Reserved.
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
 * The Original Code is derived from and used in conjunction with the 
 * OpenH323 Project (www.openh323.org/)
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte Ltd.
 *
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: h460.h,v $
 * Revision 1.2  2006/06/15 15:34:25  shorne
 * More updates
 *
 * Revision 1.1  2006/05/27 07:24:00  hfriederich
 * Initial port of H.460 files from OpenH323 to OPAL
 *
*/


#ifdef P_USE_PRAGMA
#pragma interface
#endif

class H460_MessageType
{
  public:
    enum {
      e_gatekeeperRequest           = 0xf0,
      e_gatekeeperConfirm           = 0xf1,
      e_gatekeeperReject            = 0xf2,
      e_registrationRequest         = 0xf3,
      e_registrationConfirm         = 0xf4, 
      e_registrationReject          = 0xf5,
      e_admissionRequest            = 0xf6,
      e_admissionConfirm            = 0xf7,
      e_admissionReject             = 0xf8,
      e_locationRequest             = 0xf9,
      e_locationConfirm             = 0xfa,
      e_locationReject              = 0xfb,
      e_nonStandardMessage          = 0xfc,
      e_serviceControlIndication    = 0xfd,
      e_serviceControlResponse      = 0xfe,
	  e_setup						= 0x05,   // Match Q931 message id
      e_callProceeding				= 0x02,   // Match Q931 message id
      e_connect						= 0x07,   // Match Q931 message id
      e_alerting					= 0x01,   // Match Q931 message id
      e_facility					= 0x62,   // Match Q931 message id
	  e_releaseComplete				= 0x5a,   // Match Q931 message id
	  e_unallocated					= 0xff
    };
};

