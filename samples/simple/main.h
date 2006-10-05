/*
 * main.h
 *
 * A simple OPAL "net telephone" application.
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 *
 * $Log: main.h,v $
 * Revision 1.2025  2006/10/05 07:11:49  csoutheren
 * Add --disable-lid option
 *
 * Revision 2.23  2006/08/29 01:37:11  csoutheren
 * Change secure URLs to use h323s and tcps to be inline with sips
 *
 * Revision 2.22  2006/08/21 05:30:48  csoutheren
 * Add support for sh323
 *
 * Revision 2.21  2006/07/21 00:38:31  csoutheren
 * Applied 1483215 - Opal simpleOPAL deadlock patch & DTMF support
 * Thanks to Mike T
 *
 * Revision 2.20  2006/04/30 14:36:54  csoutheren
 * backports from PLuginBranch
 *
 * Revision 2.18.2.2  2006/04/30 14:28:25  csoutheren
 * Added disableui and srcep options
 *
 * Revision 2.18.2.1  2006/03/20 02:25:26  csoutheren
 * Backports from CVS head
 *
 * Revision 2.19  2006/03/07 11:24:15  csoutheren
 * Add --disable-grq flag
 *
 * Revision 2.18  2006/01/23 22:56:57  csoutheren
 * Added 2 second pause before dialling outgoing SIP calls from command line args when
 *  registrar used
 *
 * Revision 2.17  2005/09/06 04:58:42  dereksmithies
 * Add console input options. This is an initial release, and some "refinement"
 * help immensely.
 *
 * Revision 2.16  2005/07/30 07:42:15  csoutheren
 * Added IAX2 functions
 *
 * Revision 2.15  2004/04/26 07:06:08  rjongbloed
 * Removed some ancient pieces of code and used new API's for them.
 *
 * Revision 2.14  2004/03/22 10:20:34  rjongbloed
 * Changed to use UseGatekeeper() function so can select by gk-id as well as host.
 *
 * Revision 2.13  2004/03/11 06:54:28  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.12  2004/02/24 11:37:01  rjongbloed
 * More work on NAT support, manual external address translation and STUN
 *
 * Revision 2.11  2004/02/21 02:40:09  rjongbloed
 * Tidied up the translate address to utilise more of the library infrastructure.
 *
 * Revision 2.10  2004/02/17 11:00:10  csoutheren
 * Added --translate, --port-base and --port-max options
 *
 * Revision 2.9  2003/03/19 02:30:45  robertj
 * Added removal of IVR stuff if EXPAT is not installed on system.
 *
 * Revision 2.8  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.7  2002/09/06 02:44:52  robertj
 * Added routing table system to route calls by regular expressions.
 *
 * Revision 2.6  2002/03/27 05:34:15  robertj
 * Removed redundent busy forward field
 *
 * Revision 2.5  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 * Revision 2.4  2002/01/22 05:34:58  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.3  2001/08/21 11:18:55  robertj
 * Added conditional compile for xJack code.
 *
 * Revision 2.2  2001/08/17 08:35:41  robertj
 * Changed OnEstablished() to OnEstablishedCall() to be consistent.
 * Moved call end reasons enum from OpalConnection to global.
 * Used LID management in lid EP.
 * More implementation.
 *
 * Revision 2.1  2001/08/01 06:19:00  robertj
 * Added flags for disabling H.323 or Quicknet endpoints.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.5  2001/03/21 04:52:40  robertj
 * Added H.235 security to gatekeepers, thanks Fürbass Franz!
 *
 * Revision 1.4  2001/03/20 23:42:55  robertj
 * Used the new PTrace::Initialise function for starting trace code.
 *
 * Revision 1.3  2000/07/31 14:08:09  robertj
 * Added fast start and H.245 tunneling flags to the H323Connection constructor so can
 *    disabled these features in easier manner to overriding virtuals.
 *
 * Revision 1.2  2000/06/07 05:47:56  robertj
 * Added call forwarding.
 *
 * Revision 1.1  2000/05/11 04:05:57  robertj
 * Simple sample program.
 *
 */

#ifndef _SimpleOpal_MAIN_H
#define _SimpleOpal_MAIN_H

#include <ptclib/ipacl.h>
#include <opal/manager.h>
#include <opal/pcss.h>
#include <opal/ivr.h>


class MyManager;
class SIPEndPoint;
class H323EndPoint;
class H323SEndPoint;
class IAX2EndPoint;

class MyPCSSEndPoint : public OpalPCSSEndPoint
{
  PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);

  public:
    MyPCSSEndPoint(MyManager & manager);

    virtual PString OnGetDestination(const OpalPCSSConnection & connection);
    virtual void OnShowIncoming(const OpalPCSSConnection & connection);
    virtual BOOL OnShowOutgoing(const OpalPCSSConnection & connection);

    BOOL SetSoundDevice(PArgList & args, const char * optionName, PSoundChannel::Directions dir);

    PString destinationAddress;
    PString incomingConnectionToken;
	PString remoteConnectionToken;
    BOOL    autoAnswer;
};


class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager);

  public:
    MyManager();
    ~MyManager();

    BOOL Initialise(PArgList & args);
    void Main(PArgList & args);

    virtual void OnEstablishedCall(
      OpalCall & call   /// Call that was completed
    );
    virtual void OnClearedCall(
      OpalCall & call   /// Connection that was established
    );
    virtual BOOL OnOpenMediaStream(
      OpalConnection & connection,  /// Connection that owns the media stream
      OpalMediaStream & stream    /// New media stream being opened
    );
    virtual void OnUserInputString(
      OpalConnection & connection,  /// Connection input has come from
      const PString & value         /// String value of indication
    );

    OpalConnection::AnswerCallResponse
      OnAnswerCall(OpalConnection & connection,
		   const PString & caller);

  protected:
    BOOL InitialiseH323EP(PArgList & args, const PString & listenOption, H323EndPoint * h323EP);

    PString currentCallToken;

#if OPAL_LID
    OpalPOTSEndPoint * potsEP;
#endif
    MyPCSSEndPoint   * pcssEP;
#if OPAL_H323
    H323EndPoint     * h323EP;
#if P_SSL
    H323SecureEndPoint    * h323sEP;
#endif
#endif
#if OPAL_SIP
    SIPEndPoint      * sipEP;
#endif
#if OPAL_IAX2
    IAX2EndPoint      * iax2EP;
#endif
#if P_EXPAT
    OpalIVREndPoint  * ivrEP;
#endif

    BOOL pauseBeforeDialing;
    PString srcEP;

    void HangupCurrentCall();
    void ListSpeedDials();
    void StartCall(const PString & ostr);
    void AnswerCall(OpalConnection::AnswerCallResponse response);
    void NewSpeedDial(const PString & ostr);
    void SendMessageToRemoteNode(const PString & ostr);
    void SendTone(const char tone);
};


class SimpleOpalProcess : public PProcess
{
  PCLASSINFO(SimpleOpalProcess, PProcess)

  public:
    SimpleOpalProcess();

    void Main();

  protected:
    MyManager * opal;
};


#endif  // _SimpleOpal_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
