/*
 * main.h
 *
 * An OPAL GUI phone application.
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * Revision 1.3  2004/04/26 07:36:22  rjongbloed
 * Some more implementation, still a long way to go!
 *
 * Revision 1.2  2004/04/25 13:12:22  rjongbloed
 * Converted OpenPhone v2 to use wxWindows
 *
 * Revision 1.1  2003/03/07 06:33:28  craigs
 * More implementation
 *
 */

#ifndef _OpenPhone_MAIN_H
#define _OpenPhone_MAIN_H

#include <wx/wx.h>

#include <ptlib.h>

#include <opal/manager.h>
#include <opal/pcss.h>


class MyFrame;

class SIPEndPoint;
class H323EndPoint;
class OpalPOTSEndPoint;
class OpalIVREndPoint;


class MyPCSSEndPoint : public OpalPCSSEndPoint
{
  PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);

  public:
    MyPCSSEndPoint(MyFrame & frame);

    virtual PString OnGetDestination(const OpalPCSSConnection & connection);
    virtual void OnShowIncoming(const OpalPCSSConnection & connection);
    virtual BOOL OnShowOutgoing(const OpalPCSSConnection & connection);

    PString destinationAddress;
    PString incomingConnectionToken;
    BOOL    autoAnswer;

    MyFrame & frame;
};


enum MainMenuItems {
  MENU_FILE_QUIT,
  MENU_FILE_ABOUT
};


class MyFrame : public wxFrame, public OpalManager
{
  public:
    MyFrame();
    ~MyFrame();

  protected:
    void OnSize(wxSizeEvent& event);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

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

  private:
    wxPanel    * m_panel;
    wxTextCtrl * m_logWindow;

    MyPCSSEndPoint   * pcssEP;
    OpalPOTSEndPoint * potsEP;
#if OPAL_H323
    H323EndPoint     * h323EP;
#endif
#if OPAL_SIP
    SIPEndPoint      * sipEP;
#endif
#if P_EXPAT
    OpalIVREndPoint  * ivrEP;
#endif

    PString currentCallToken;

    DECLARE_EVENT_TABLE()
};


class OpenPhoneApp : public wxApp, public PProcess
{
    PCLASSINFO(OpenPhoneApp, PProcess);

  public:
    OpenPhoneApp();

    void Main(); // Dummy function

      // FUnction from wxWindows
    virtual bool OnInit();
};


#endif


// End of File ///////////////////////////////////////////////////////////////
