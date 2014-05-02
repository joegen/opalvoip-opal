/*
 * status.cxx
 *
 * OPAL Server status pages
 *
 * Copyright (c) 2014 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Vox Lucida Pty. Ltd.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include "precompile.h"
#include "main.h"


#if OPAL_SIP

static const char SIPUsernameKey[] = "SIP User Name";
static const char SIPPrackKey[] = "SIP Provisional Responses";
static const char SIPProxyKey[] = "SIP Proxy URL";
static const char SIPListenersKey[] = "SIP Interfaces";
#if OPAL_PTLIB_SSL
static const char SIPSignalingSecurityKey[] = "SIP Security";
static const char SIPMediaCryptoSuitesKey[] = "SIP Crypto Suites";
#endif
static const char SIPLocalRegistrarKey[] = "SIP Local Registrar Domains";
#if OPAL_H323
static const char SIPAutoRegisterH323Key[] = "SIP Auto-Register H.323";
#endif
#if OPAL_SKINNY
static const char SIPAutoRegisterSkinnyKey[] = "SIP Auto-Register Skinny";
#endif

#define REGISTRATIONS_SECTION "SIP Registrations"
#define REGISTRATIONS_KEY "Registration"

static const char SIPAddressofRecordKey[] = "Address of Record";
static const char SIPAuthIDKey[] = "Auth ID";
static const char SIPPasswordKey[] = "Password";


///////////////////////////////////////////////////////////////

MySIPEndPoint::MySIPEndPoint(MyManager & mgr)
  : SIPConsoleEndPoint(mgr)
  , m_manager(mgr)
#if OPAL_H323
  , m_autoRegisterH323(false)
#endif
#if OPAL_SKINNY
  , m_autoRegisterSkinny(false)
#endif
{
}


bool MySIPEndPoint::Configure(PConfig & cfg, PConfigPage * rsrc)
{
  // Add SIP parameters
  SetDefaultLocalPartyName(rsrc->AddStringField(SIPUsernameKey, 25, GetDefaultLocalPartyName(), "SIP local user name"));

  if (!StartListeners(rsrc->AddStringArrayField(SIPListenersKey, false, 25, PStringArray(),
    "Local network interfaces to listen for SIP, blank means all"))) {
    PSYSTEMLOG(Error, "Could not open any SIP listeners!");
  }

  SIPConnection::PRACKMode prack = cfg.GetEnum(SIPPrackKey, GetDefaultPRACKMode());
  static const char * const prackModes[] = { "Disabled", "Supported", "Required" };
  rsrc->Add(new PHTTPRadioField(SIPPrackKey, PARRAYSIZE(prackModes), prackModes, prack,
    "SIP provisional responses (100rel) handling."));
  SetDefaultPRACKMode(prack);

  SetProxy(rsrc->AddStringField(SIPProxyKey, 100, GetProxy().AsString(), "SIP outbound proxy IP/hostname", 1, 30));

#if OPAL_PTLIB_SSL
  m_manager.ConfigureSecurity(this, SIPSignalingSecurityKey, SIPMediaCryptoSuitesKey, cfg, rsrc);
#endif

  // Registrar
  PString defaultSection = cfg.GetDefaultSection();
  list<SIPRegister::Params> registrations;
  PINDEX arraySize = cfg.GetInteger(REGISTRATIONS_SECTION, REGISTRATIONS_KEY" Array Size");
  for (PINDEX i = 0; i < arraySize; i++) {
    SIPRegister::Params registrar;
    cfg.SetDefaultSection(psprintf(REGISTRATIONS_SECTION"\\"REGISTRATIONS_KEY" %u", i + 1));
    registrar.m_addressOfRecord = cfg.GetString(SIPAddressofRecordKey);
    registrar.m_authID = cfg.GetString(SIPAuthIDKey);
    registrar.m_password = cfg.GetString(SIPPasswordKey);
    if (!registrar.m_addressOfRecord.IsEmpty())
      registrations.push_back(registrar);
  }
  cfg.SetDefaultSection(defaultSection);

  PHTTPCompositeField * registrationsFields = new PHTTPCompositeField(
                            REGISTRATIONS_SECTION"\\"REGISTRATIONS_KEY" %u\\", REGISTRATIONS_SECTION,
                            "Registration of SIP username at domain/hostname/IP address");
  registrationsFields->Append(new PHTTPStringField(SIPAddressofRecordKey, 0, NULL, NULL, 1, 40));
  registrationsFields->Append(new PHTTPStringField(SIPAuthIDKey, 0, NULL, NULL, 1, 25));
  registrationsFields->Append(new PHTTPPasswordField(SIPPasswordKey, 15));
  rsrc->Add(new PHTTPFieldArray(registrationsFields, false));

  for (list<SIPRegister::Params>::iterator it = registrations.begin(); it != registrations.end(); ++it) {
    PString aor;
    if (Register(*it, aor))
      PSYSTEMLOG(Info, "Started register of " << aor);
    else
      PSYSTEMLOG(Error, "Could not register " << it->m_addressOfRecord);
  }

  SetRegistrarDomains(rsrc->AddStringArrayField(SIPLocalRegistrarKey, false, 25,
                      PStringArray(m_registrarDomains), "SIP local registrar domain names"));

#if OPAL_H323
  m_autoRegisterH323 = rsrc->AddBooleanField(SIPAutoRegisterH323Key, m_autoRegisterH323,
                                             "Auto-register H.323 alias of same name as incoming SIP registration");
#endif
#if OPAL_SKINNY
  m_autoRegisterSkinny = rsrc->AddBooleanField(SIPAutoRegisterSkinnyKey, m_autoRegisterSkinny,
                                               "Auto-register SCCP device of same name as incoming SIP registration");
#endif
    return true;
}


#if OPAL_H323 || OPAL_SKINNY
void MySIPEndPoint::OnChangedRegistrarAoR(RegistrarAoR & ua)
{
  m_manager.OnChangedRegistrarAoR(ua.GetAoR(), ua.HasBindings());
}

void MyManager::OnChangedRegistrarAoR(const PURL & aor, bool registering)
{
#if OPAL_H323
  if (aor.GetScheme() == "h323") {
    if (aor.GetUserName().IsEmpty())
      GetH323EndPoint().AutoRegister(aor.GetHostName(), registering);
    else if (aor.GetHostName().IsEmpty())
      GetH323EndPoint().AutoRegister(aor.GetUserName(), registering);
    else if (!aor.GetHostName().IsEmpty())
      GetH323EndPoint().AutoRegister(PSTRSTRM(aor.GetUserName() << '@' << aor.GetHostName()), registering);
  }
  else if (GetSIPEndPoint().m_autoRegisterH323 && aor.GetScheme().NumCompare("sip") == EqualTo)
    GetH323EndPoint().AutoRegister(aor.GetUserName(), registering);
#endif // OPAL_H323

#if OPAL_SKINNY
  if (aor.GetScheme() == "sccp")
    GetSkinnyEndPoint().AutoRegister(aor.GetHostPort(), aor.GetUserName(), registering);
  else if (GetSIPEndPoint().m_autoRegisterSkinny && aor.GetScheme().NumCompare("sip") == EqualTo)
    GetSkinnyEndPoint().AutoRegister(PString::Empty(), aor.GetUserName(), registering);
#endif // OPAL_SKINNY
}
#endif // OPAL_H323 || OPAL_SKINNY


#endif // OPAL_SIP

// End of File ///////////////////////////////////////////////////////////////
