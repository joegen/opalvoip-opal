/*
 * lyncep.cxx
 *
 * Header file for Lync (UCMA) interface
 *
 * Copyright (C) 2016 Vox Lucida Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#include "lyncshim.h"

#if OPAL_LYNC

#include <vcclr.h>
#using OPAL_LYNC_LIBRARY

using namespace Microsoft::Rtc;

struct OpalLyncSim::Impl
{
  gcroot<Collaboration::CollaborationPlatform^> m_pCollaborationPlatform;
  gcroot<Collaboration::ApplicationEndpoint^> m_pApplicationEndpoint;

  Impl()
  {
    Collaboration::ClientPlatformSettings^ cps = gcnew Collaboration::ClientPlatformSettings(nullptr, Signaling::SipTransportType::Tcp);
    m_pCollaborationPlatform = gcnew Collaboration::CollaborationPlatform(cps);

    Collaboration::ApplicationEndpointSettings^ aes = gcnew Collaboration::ApplicationEndpointSettings(nullptr);
    m_pApplicationEndpoint = gcnew Collaboration::ApplicationEndpoint(m_pCollaborationPlatform, aes);
  }

  ~Impl()
  {
  }
};


OpalLyncSim::OpalLyncSim()
  : m_impl(new Impl)
{
}


OpalLyncSim::~OpalLyncSim()
{
  delete m_impl;
}


#endif // OPAL_LYNC

// End of File /////////////////////////////////////////////////////////////
