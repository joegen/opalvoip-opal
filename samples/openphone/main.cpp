/*
 * main.cpp
 *
 * Open Phone Client application source file for OPAL
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
 * Contributor(s): 
 */

#ifdef __GNUG__
#pragma implementation
#pragma interface
#endif

#include "main.h"
#include "version.h"


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_APP(OpenPhoneApp)

OpenPhoneApp::OpenPhoneApp()
  : PProcess("Equivalence", "OpenPhone",
              MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}


void OpenPhoneApp::Main()
{
  // Dummy function
}


bool OpenPhoneApp::OnInit()
{
  // Create the main frame window
  SetTopWindow(new MainWindow());

  return true;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
  EVT_SIZE(MainWindow::OnSize)

  EVT_MENU(MENU_FILE_QUIT, MainWindow::OnQuit)
  EVT_MENU(MENU_FILE_ABOUT, MainWindow::OnAbout)
END_EVENT_TABLE()

MainWindow::MainWindow()
  : wxFrame((wxFrame *)NULL, -1, wxT("wxListCtrl Test"), wxPoint(50, 50), wxSize(450, 340)),
    m_logWindow(NULL)
{
  // Give it an icon
  SetIcon( wxICON(OpenOphone) );

  // Make a menubar
  wxMenuBar *menubar = new wxMenuBar;

  wxMenu *menu = new wxMenu;
  menu->Append(MENU_FILE_ABOUT, _T("&About"));
  menu->AppendSeparator();
  menu->Append(MENU_FILE_QUIT, _T("E&xit\tAlt-X"));
  menubar->Append(menu, _T("&File"));

  SetMenuBar(menubar);

  m_panel = new wxPanel(this, -1);
  m_logWindow = new wxTextCtrl(m_panel, -1, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxSUNKEN_BORDER);

  // Show the frame
  Show(TRUE);
}


MainWindow::~MainWindow()
{
}


void MainWindow::OnSize(wxSizeEvent& event)
{
  if (m_logWindow == NULL)
    return;

  wxSize size = GetClientSize();
  m_logWindow->SetSize(0, 0, size.x, size.y);

  event.Skip();
}


void MainWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(TRUE);
}


void MainWindow::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageDialog dialog(this,
                           _T("OpenPhone\nPost Increment (c) 2004"),
                           _T("About list test"),
                           wxOK|wxCANCEL);
    dialog.ShowModal();
}


// End of File ///////////////////////////////////////////////////////////////
