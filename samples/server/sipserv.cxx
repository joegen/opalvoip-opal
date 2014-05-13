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

#define REGISTRATIONS_SECTION "SIP Registrations"
#define REGISTRATIONS_KEY     REGISTRATIONS_SECTION"\\Registration %u\\"

static const char SIPAddressofRecordKey[] = "Address of Record";
static const char SIPAuthIDKey[] = "Auth ID";
static const char SIPPasswordKey[] = "Password";


///////////////////////////////////////////////////////////////

MySIPEndPoint::MySIPEndPoint(MyManager & mgr)
  : SIPConsoleEndPoint(mgr)
  , m_manager(mgr)
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
  registrationsFields->Append(new PHTTPStringField(SIPAddressofRecordKey, 0, NULL, NULL, 1, 40));
  registrationsFields->Append(new PHTTPStringField(SIPAuthIDKey, 0, NULL, NULL, 1, 25));
  registrationsFields->Append(new PHTTPPasswordField(SIPPasswordKey, 15));
  PHTTPFieldArray * registrationsArray = new PHTTPFieldArray(registrationsFields, false);
  rsrc->Add(registrationsArray);

  if (!registrationsArray->LoadFromConfig(cfg)) {
    for (PINDEX i = 0; i < registrationsArray->GetSize(); ++i) {
      PHTTPCompositeField & item = dynamic_cast<PHTTPCompositeField &>((*registrationsArray)[i]);

      SIPRegister::Params registrar;
      registrar.m_addressOfRecord = item[0].GetValue();
      if (!registrar.m_addressOfRecord.IsEmpty()) {
        registrar.m_authID = item[1].GetValue();
        registrar.m_password = item[2].GetValue();
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

#endif // OPAL_SIP

// End of File ///////////////////////////////////////////////////////////////
