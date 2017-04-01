/*
 * main.h
 *
 * OPAL application source file for EXternal RTP demonstration for OPAL
 *
 * Copyright (c) 2011 Vox Lucida Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 */

#ifndef _ExtRTP_MAIN_H
#define _ExtRTP_MAIN_H


#define EXTERNAL_SCHEME "ext"

class MyLocalEndPoint : public OpalLocalEndPoint
{
    PCLASSINFO(MyLocalEndPoint, OpalLocalEndPoint)

  public:
    MyLocalEndPoint(OpalManager & manager)
      : OpalLocalEndPoint(manager, EXTERNAL_SCHEME) { }
};


class MyManager : public OpalManagerConsole
{
    PCLASSINFO(MyManager, OpalManagerConsole)

  public:
    virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute = PString::Empty());
    virtual bool GetMediaTransportAddresses(const OpalConnection &, const OpalConnection &, unsigned, const OpalMediaType &, OpalTransportAddressArray &) const;
    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);
    virtual void OnClearedCall(OpalCall & call);
};


#endif  // _ExtRTP_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
