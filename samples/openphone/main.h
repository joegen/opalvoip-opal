/*
 * main.h
 *
 * An OPAL GUI phone application.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2004 Post Increment
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
 * The Original Code is Open Phone client.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: main.h,v $
 * Revision 1.6  2004/05/12 12:41:38  rjongbloed
 * More work on wxWindows based OpenPhone
 *
 * Revision 1.5  2004/05/06 13:23:43  rjongbloed
 * Work on wxWindows based OpenPhone
 *
 * Revision 1.4  2004/05/01 13:38:05  rjongbloed
 * Some early implementation of wxWIndows based OPAL GUI client.
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

class wxSplitterWindow;
class wxListCtrl;


class PwxString : public wxString
{
  public:
    PwxString() { }
    PwxString(const wxString & str) : wxString(str) { }
    PwxString(const PString & str) : wxString((const char *)str) { }
    PwxString(const char * str) : wxString(str) { }

    PwxString & operator=(const wxString & str) { wxString::operator=(str); return *this; }
    PwxString & operator=(const PString & str) { wxString::operator=((const char *)str); return *this; }
    PwxString & operator=(const char * str) { wxString::operator=(str); return *this; }

    operator PString() const { return c_str(); }
};

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


class CallDialog : public wxDialog
{
  public:
    CallDialog(wxWindow *parent);

    PwxString m_Address;

  private:
    void OnAddressChange(wxCommandEvent & event);

    wxComboBox * m_AddressCtrl;
    wxButton   * m_ok;

    DECLARE_EVENT_TABLE()
};


class OptionsDialog : public wxDialog
{
  public:
    OptionsDialog(wxWindow *parent);

    PwxString m_SoundPlayer;
    PwxString m_SoundRecorder;

  private:
    DECLARE_EVENT_TABLE()
};


class MyFrame : public wxFrame, public OpalManager
{
  public:
    MyFrame();
    ~MyFrame();

    bool Initialise();

  private:
    void OnMenuQuit(wxCommandEvent& event);
    void OnMenuAbout(wxCommandEvent& event);
    void OnMenuCall(wxCommandEvent& event);
    void OnViewLarge(wxCommandEvent& event);
    void OnViewSmall(wxCommandEvent& event);
    void OnViewList(wxCommandEvent& event);
    void OnViewDetails(wxCommandEvent& event);
    void OnOptions(wxCommandEvent& event);

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

    void RecreateSpeedDials(
      long flags
    );

    wxSplitterWindow         * m_splitter;
    wxListCtrl               * m_speedDials;
    wxTextCtrl               * m_logWindow;

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
