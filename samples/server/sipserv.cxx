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
 */

#include "precompile.h"
#include "main.h"


#if OPAL_SIP

static const char SIPUsernameKey[] = "SIP User Name";
static const char SIPPrackKey[] = "SIP Provisional Responses";
static const char SIPProxyKey[] = "SIP Proxy URL";
static const char SIPLocalRegistrarKey[] = "SIP Local Registrar Domains";
static const char SIPCiscoDeviceTypeKey[] = "SIP Cisco Device Type";
static const char SIPCiscoDevicePatternKey[] = "SIP Cisco Device Pattern";
#if OPAL_H323
static const char SIPAutoRegisterH323Key[] = "SIP Auto-Register H.323";
#endif
#if OPAL_SKINNY
static const char SIPAutoRegisterSkinnyKey[] = "SIP Auto-Register Skinny";
#endif

#define REGISTRATIONS_SECTION "SIP Registrations"
#define REGISTRATIONS_KEY     REGISTRATIONS_SECTION"\\Registration %u\\"

static const char SIPAddressofRecordKey[] = "Address of Record";
static const char SIPAuthIDKey[] = "Auth ID";
static const char SIPPasswordKey[] = "Password";
static const char SIPRegTTLKey[] = "TTL";
static const char SIPCompatibilityKey[] = "Compatibility";


PCREATE_SERVICE_MACRO_BLOCK(SIPEndPointStatus,resource,P_EMPTY,block)
{
  RegistrarStatusPage * status = dynamic_cast<RegistrarStatusPage *>(resource.m_resource);
  return PAssertNULL(status)->m_registrar.OnLoadEndPointStatus(block);
}


///////////////////////////////////////////////////////////////

MySIPEndPoint::MySIPEndPoint(MyManager & mgr)
  : SIPConsoleEndPoint(mgr)
#if OPAL_H323
  , m_autoRegisterH323(false)
#endif
#if OPAL_SKINNY
  , m_autoRegisterSkinny(false)
#endif
  , m_manager(mgr)
#if OPAL_SKINNY
  , m_ciscoDeviceType(OpalSkinnyEndPoint::DefaultDeviceType)
#else
  , m_ciscoDeviceType(0)
#endif
  , m_ciscoDevicePattern("SEPFFFFFFFFFFFF")
{
}


bool MySIPEndPoint::Configure(PConfig & cfg, PConfigPage * rsrc)
{
  if (!m_manager.ConfigureCommon(this, "SIP", cfg, rsrc))
    return false;

  // Add SIP parameters
  SetDefaultLocalPartyName(rsrc->AddStringField(SIPUsernameKey, 25, GetDefaultLocalPartyName(), "SIP local user name"));

  SIPConnection::PRACKMode prack = cfg.GetEnum(SIPPrackKey, GetDefaultPRACKMode());
  static const char * const prackModes[] = { "Disabled", "Supported", "Required" };
  rsrc->Add(new PHTTPRadioField(SIPPrackKey, PARRAYSIZE(prackModes), prackModes, prack,
    "SIP provisional responses (100rel) handling."));
  SetDefaultPRACKMode(prack);

  SetProxy(rsrc->AddStringField(SIPProxyKey, 100, GetProxy().AsString(), "SIP outbound proxy IP/hostname", 1, 30));

  // Registrars
  PHTTPCompositeField * registrationsFields = new PHTTPCompositeField(REGISTRATIONS_KEY, REGISTRATIONS_SECTION,
                                                  "Registration of SIP username at domain/hostname/IP address");
  registrationsFields->Append(new PHTTPStringField(SIPAddressofRecordKey, 0, NULL, NULL, 1, 20));
  registrationsFields->Append(new PHTTPStringField(SIPAuthIDKey, 0, NULL, NULL, 1, 10));
  registrationsFields->Append(new PHTTPIntegerField(SIPRegTTLKey, 1, 86400, 300));
  registrationsFields->Append(new PHTTPPasswordField(SIPPasswordKey, 10));
  static const char * const compatibilityModes[] = {
      "Compliant", "Single Contact", "No Private", "ALGw", "RFC 5626", "Cisco"
  };
  registrationsFields->Append(new PHTTPEnumField<SIPRegister::CompatibilityModes>(SIPCompatibilityKey,
                                                   PARRAYSIZE(compatibilityModes), compatibilityModes));
  PHTTPFieldArray * registrationsArray = new PHTTPFieldArray(registrationsFields, false);
  rsrc->Add(registrationsArray);

  SetRegistrarDomains(rsrc->AddStringArrayField(SIPLocalRegistrarKey, false, 25,
    PStringArray(m_registrarDomains), "SIP local registrar domain names"));

  m_ciscoDeviceType = rsrc->AddIntegerField(SIPCiscoDeviceTypeKey, 1, 32767, m_ciscoDeviceType, "",
    "Device type for SIP Cisco Devices. Default 30016 = Cisco IP Communicator.");

  m_ciscoDevicePattern = rsrc->AddStringField(SIPCiscoDevicePatternKey, 0, m_ciscoDevicePattern,
    "Pattern used to register SIP Cisco Devices. Default SEPFFFFFFFFFFFF", 1, 80);

#if OPAL_H323
  m_autoRegisterH323 = rsrc->AddBooleanField(SIPAutoRegisterH323Key, m_autoRegisterH323,
                                             "Auto-register H.323 alias of same name as incoming SIP registration");
#endif
#if OPAL_SKINNY
  m_autoRegisterSkinny = rsrc->AddBooleanField(SIPAutoRegisterSkinnyKey, m_autoRegisterSkinny,
                                               "Auto-register SCCP device of same name as incoming SIP registration");
#endif

  if (!registrationsArray->LoadFromConfig(cfg)) {
    for (PINDEX i = 0; i < registrationsArray->GetSize(); ++i) {
      PHTTPCompositeField & item = dynamic_cast<PHTTPCompositeField &>((*registrationsArray)[i]);

      SIPRegister::Params registrar;
      registrar.m_addressOfRecord = item[0].GetValue();
      if (!registrar.m_addressOfRecord.IsEmpty()) {
        registrar.m_authID = item[1].GetValue();
        registrar.m_expire = item[2].GetValue().AsUnsigned();
        registrar.m_password = PHTTPPasswordField::Decrypt(item[3].GetValue());
        registrar.m_compatibility = (SIPRegister::CompatibilityModes)item[4].GetValue().AsUnsigned();
        registrar.m_ciscoDeviceType = PString(m_ciscoDeviceType);
        registrar.m_ciscoDevicePattern = m_ciscoDevicePattern;
        PString aor;
        if (Register(registrar, aor))
          PSYSTEMLOG(Info, "Started register of " << aor);
        else
          PSYSTEMLOG(Error, "Could not register " << registrar.m_addressOfRecord);
      }
    }
  }

  return true;
}


PString MySIPEndPoint::OnLoadEndPointStatus(const PString & htmlBlock)
{
  PString substitution;

  for (PSafePtr<RegistrarAoR> ua(m_registeredUAs); ua != NULL; ++ua) {
    // make a copy of the repeating html chunk
    PString insert = htmlBlock;

    PServiceHTML::SpliceMacro(insert, "status EndPointIdentifier", ua->GetAoR());

    SIPURLList contacts = ua->GetContacts();
    PStringStream addresses;
    for (SIPURLList::iterator it = contacts.begin(); it != contacts.end(); ++it) {
      if (it != contacts.begin())
        addresses << "<br>";
      addresses << *it;
    }
    PServiceHTML::SpliceMacro(insert, "status CallSignalAddresses", addresses);

    PString str = "<i>Name:</i> " + ua->GetProductInfo().AsString();
    str.Replace("\t", "<BR><i>Version:</i> ");
    str.Replace("\t", " <i>Vendor:</i> ");
    PServiceHTML::SpliceMacro(insert, "status Application", str);

    PServiceHTML::SpliceMacro(insert, "status ActiveCalls", "N/A");

    // Then put it into the page, moving insertion point along after it.
    substitution += insert;
  }

  return substitution;
}


bool MySIPEndPoint::ForceUnregister(const PString id)
{
  return m_registeredUAs.RemoveAt(id);
}


#if OPAL_H323 || OPAL_SKINNY
void MySIPEndPoint::OnChangedRegistrarAoR(RegistrarAoR & ua)
{
  m_manager.OnChangedRegistrarAoR(ua.GetAoR(), ua.HasBindings());
}


void MySIPEndPoint::AutoRegisterCisco(const PString & server, const PString & wildcard, const PString & deviceType, bool registering)
{
  PStringArray names, servers;
  ExpandWildcards(wildcard, server, names, servers);

  for (PINDEX i = 0; i < names.GetSize(); ++i) {
    PString addressOfRecord = "sip:" + names[i] + "@" + server;

    if (registering) {
      SIPRegister::Params registrar;
      registrar.m_addressOfRecord = addressOfRecord;
      registrar.m_compatibility = SIPRegister::e_Cisco;
      if (deviceType.IsEmpty())
        registrar.m_ciscoDeviceType = PString(m_ciscoDeviceType);
      else
        registrar.m_ciscoDeviceType = deviceType;

      registrar.m_ciscoDevicePattern = m_ciscoDevicePattern;
      PString aor;
      if (Register(registrar, aor))
        PSYSTEMLOG(Info, "Started register of " << aor);
      else
        PSYSTEMLOG(Error, "Could not register " << registrar.m_addressOfRecord);
    }
    else {
      Unregister(addressOfRecord);
    }
  }
}


void MyManager::OnChangedRegistrarAoR(const PURL & aor, bool registering)
{
#if OPAL_H323
  if (aor.GetScheme() == "h323") {
    if (aor.GetUserName().IsEmpty())
      GetH323EndPoint().AutoRegister(aor.GetHostName(), PString::Empty(), registering);
    else if (aor.GetHostName().IsEmpty())
      GetH323EndPoint().AutoRegister(aor.GetUserName(), PString::Empty(), registering);
    else if (aor.GetParamVars()("type") *= "gk")
      GetH323EndPoint().AutoRegister(aor.GetUserName(), aor.GetHostName(), registering);
    else
      GetH323EndPoint().AutoRegister(PSTRSTRM(aor.GetUserName() << '@' << aor.GetHostName()), PString::Empty(), registering);
  }
  else if (GetSIPEndPoint().m_autoRegisterH323 && aor.GetScheme().NumCompare("sip") == EqualTo)
    GetH323EndPoint().AutoRegister(aor.GetUserName(), PString::Empty(), registering);
#endif // OPAL_H323

#if OPAL_SKINNY
  if (aor.GetScheme() == "sccp")
    GetSkinnyEndPoint().AutoRegister(aor.GetHostPort(), aor.GetUserName(), aor.GetParamVars()("interface"), registering);
  else if (GetSIPEndPoint().m_autoRegisterSkinny && aor.GetScheme().NumCompare("sip") == EqualTo)
    GetSkinnyEndPoint().AutoRegister(PString::Empty(), aor.GetUserName(), aor.GetParamVars()("interface"), registering);
#endif // OPAL_SKINNY

  if (aor.GetScheme() == "sip" && (aor.GetParamVars()("type") *= "cisco"))
    GetSIPEndPoint().AutoRegisterCisco(aor.GetHostPort(), aor.GetUserName(), aor.GetParamVars()("device"), registering);
}
#endif // OPAL_H323 || OPAL_SKINNY


#endif // OPAL_SIP

// End of File ///////////////////////////////////////////////////////////////
