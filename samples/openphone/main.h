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
#include <h323/h323ep.h>
#include <sip/sipep.h>
#include <opal/pcss.h>


class OpenPhoneApp : public wxApp, public PProcess
{
    PCLASSINFO(OpenPhoneApp, PProcess);

public:
  OpenPhoneApp();

  void Main(); // Dummy function

    // FUnction from wxWindows
  virtual bool OnInit();
};


class MainWindow : public wxFrame
{
public:
    MainWindow();
    ~MainWindow();

protected:
    void OnSize(wxSizeEvent& event);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

private:
    wxPanel    * m_panel;
    wxTextCtrl * m_logWindow;

    DECLARE_EVENT_TABLE()
};


enum {
  MENU_FILE_QUIT,
  MENU_FILE_ABOUT
};

#endif


// End of File ///////////////////////////////////////////////////////////////
