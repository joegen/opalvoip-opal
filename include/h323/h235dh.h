/*
 * h235du.h
 *
 * H.235 Diffie-Hellman key exchange PDU's
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2013 Vox Lucida Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_H323_H235DH_H
#define OPAL_H323_H235DH_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_H235_6

#include <ptclib/pssl.h>


class H225_ArrayOf_ClearToken;
class OpalMediaCryptoSuite;


/** This embodies a collection of Diffie-Hellman "groups" as per H.235.6
*/
class H235DiffieHellman : public PDictionary<PString, PSSLDiffieHellman>
{
    typedef PDictionary<PString, PSSLDiffieHellman> BaseClass;
    PCLASSINFO(H235DiffieHellman, BaseClass);
  public:
    H235DiffieHellman() : m_version3(true) { }

    virtual bool AddForAlgorithm(const OpalMediaCryptoSuite & cryptoSuite);
    virtual PBYTEArray FindMasterKey(const OpalMediaCryptoSuite & cryptoSuite) const;

    virtual bool ToTokens(H225_ArrayOf_ClearToken & tokens) const;
    virtual bool FromTokens(const H225_ArrayOf_ClearToken & tokens);

    bool IsVersion3() const { return m_version3; }

  protected:
    bool m_version3;
};

#endif // OPAL_H235_6

#endif //OPAL_H323_H235AUTH_H


/////////////////////////////////////////////////////////////////////////////
