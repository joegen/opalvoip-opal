/*
 * main.h
 *
 * OPAL application source file for GStreamer OPAL sample
 *
 * Copyright (c) 2013 Vox Lucida Pty. Ltd.
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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _GstOPAL_MAIN_H
#define _GstOPAL_MAIN_H

#if OPAL_GSTREAMER
#else
#error Cannot compile GStreamer test program without OPAL_GSTREAMER set!
#endif


class MyGstEndPoint : public GstEndPoint
{
    PCLASSINFO(MyGstEndPoint, GstEndPoint)

  public:
    MyGstEndPoint(OpalManager & manager) : GstEndPoint(manager) { }
};


class MyManager : public OpalManagerConsole
{
    PCLASSINFO(MyManager, OpalManagerConsole)

  public:
    virtual PString GetArgumentSpec() const;
    virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute = PString::Empty());
    virtual void Usage(ostream & strm, const PArgList & args);
    virtual void OnClearedCall(OpalCall & call); // Callback override
};


#endif  // _GstOPAL_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
