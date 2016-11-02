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

#include <ptlib.h>

#include <ep/lyncep.h>


#if OPAL_LYNC

///////////////////////////////////////////////////////////////

OpalLyncEndPoint::OpalLyncEndPoint(OpalManager & manager, const char *prefix)
  : OpalEndPoint(manager, prefix, IsNetworkEndPoint)
  , OpalLyncShim(PProcess::Current().GetName())
{
}


OpalLyncEndPoint::~OpalLyncEndPoint()
{
}


void OpalLyncEndPoint::ShutDown()
{
}


OpalMediaFormatList OpalLyncEndPoint::GetMediaFormats() const
{
  return OpalMediaFormatList();
}


PSafePtr<OpalConnection> OpalLyncEndPoint::MakeConnection(OpalCall & call,
                                                          const PString & party,
                                                          void * userData,
                                                          unsigned int options,
                                                          OpalConnection::StringOptions * stringOptions)
{
  return new OpalLyncConnection(call, *this, party, userData, options, stringOptions);
}


PBoolean OpalLyncEndPoint::GarbageCollection()
{
  return true;
}


bool OpalLyncEndPoint::Register(const PString & uri)
{
  PWaitAndSignal lock(m_registrationMutex);

  RegistrationMap::iterator it = m_registrations.find(uri);
  if (it != m_registrations.end())
    return false;

  UserEndpoint * user = CreateUserEndpoint(uri);
  if (user == NULL)
    return false;

  m_registrations[uri] = user;
  return true;
}


bool OpalLyncEndPoint::Unregister(const PString & uri)
{
  PWaitAndSignal lock(m_registrationMutex);

  RegistrationMap::iterator it = m_registrations.find(uri);
  if (it == m_registrations.end())
    return false;

  DestroyUserEndpoint(it->second);
  m_registrations.erase(it);
  return true;
}


///////////////////////////////////////////////////////////////////////

OpalLyncConnection::OpalLyncConnection(OpalCall & call,
                                       OpalLyncEndPoint & ep,
                                       const PString & /*dialNumber*/,
                                       void * /*userData*/,
                                       unsigned options,
                                       OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, ep.GetManager().GetNextToken('L'), options, stringOptions)
{
}


PBoolean OpalLyncConnection::SetUpConnection()
{
  return false;
}


void OpalLyncConnection::OnReleased()
{
}


OpalMediaFormatList OpalLyncConnection::GetMediaFormats() const
{
  return m_endpoint.GetMediaFormats();
}


PBoolean OpalLyncConnection::SetAlerting(const PString & /*calleeName*/, PBoolean /*withMedia*/)
{
  return false;
}


PBoolean OpalLyncConnection::SetConnected()
{
  return false;
}


OpalTransportAddress OpalLyncConnection::GetRemoteAddress() const
{
  return OpalTransportAddress();
}



#endif // OPAL_LYNC

// End of File /////////////////////////////////////////////////////////////
