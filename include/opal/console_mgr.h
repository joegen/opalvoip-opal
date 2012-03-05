/*
 * console_mgs.h
 *
 * An OpalManager derived class for use in a console application, providing
 * a standard set of command line arguments for configuring many system
 * parameters. Used by the sample applications such as faxopal, ovropal etc.
 *
 * Copyright (c) 2010 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_OPAL_CONSOLE_MGR_H
#define OPAL_OPAL_CONSOLE_MGR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/manager.h>


class SIPEndPoint;
class H323EndPoint;
class OpalLineEndPoint;
class OpalCapiEndPoint;


/**Opal manager class for console applications.
   An OpalManager derived class for use in a console application, providing
   a standard set of command line arguments for configuring many system
   parameters. Used by the sample applications such as faxopal, ovropal etc.
  */

class OpalManagerConsole : public OpalManager
{
    PCLASSINFO(OpalManagerConsole, OpalManager);

  public:
    OpalManagerConsole();

    PString GetArgumentSpec() const;
    PString GetArgumentUsage() const;

    bool Initialise(PArgList & args, bool verbose);

  protected:
#if OPAL_SIP
    SIPEndPoint * CreateSIPEndPoint();
#endif
#if OPAL_H323
    H323EndPoint * CreateH323EndPoint();
#endif
#if OPAL_LID
    OpalLineEndPoint * CreateLineEndPoint();
#endif
#if OPAL_CAPI
    OpalCapiEndPoint * CreateCapiEndPoint();
#endif
};


#endif // OPAL_OPAL_CONSOLE_MGR_H


/////////////////////////////////////////////////////////////////////////////
