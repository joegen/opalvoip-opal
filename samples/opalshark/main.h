/*
 * main.h
 *
 * An OPAL analyser/player for RTP.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2015 Vox Lucida
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
 * The Original Code is Opal Shark.
 *
 * The Initial Developer of the Original Code is Robert Jongbloed
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _OpalShark_MAIN_H
#define _OpalShark_MAIN_H

#if WXVER>29
  #define WXUSINGDLL 1
#endif

#include <ptlib_wx.h>
#include <opal/manager.h>

#ifndef OPAL_PTLIB_AUDIO
  #error Cannot compile without PTLib sound channel support!
#endif

// Note, include this after everything else so gets all the conversions
#include <ptlib/wxstring.h>


class MyManager;


struct MyOptions
{
  PwxString m_AudioDevice;
  PwxString m_VideoDevice;
};


class OptionsDialog : public wxDialog
{
  public:
    OptionsDialog(MyManager *manager, const MyOptions & options);

    const MyOptions & GetOptions() const { return m_options; }

  private:
    virtual bool TransferDataFromWindow();

    MyManager & m_manager;
    MyOptions   m_options;


    DECLARE_EVENT_TABLE()
};


class MyPlayer : public wxMDIChildFrame
{
  public:
    MyPlayer(MyManager * manager, const PFilePath & filename);

    void Play();

  private:
    void OnCloseWindow(wxCloseEvent &);
    void OnClose(wxCommandEvent &);

    MyManager & m_manager;
    PFilePath   m_pcapFile;

    DECLARE_EVENT_TABLE()
};


class MyManager : public wxMDIParentFrame, public OpalManager
{
  public:
    MyManager();
    ~MyManager();

    bool Initialise(bool startMinimised);
    void Play(const PwxString & fname);

    void PostEvent(
      const wxCommandEvent & cmdEvent,
      const PString & str = PString::Empty(),
      const void * data = NULL
    );

  private:
    void OnClose(wxCloseEvent &);
    void OnMenuQuit(wxCommandEvent &);
    void OnMenuAbout(wxCommandEvent &);
    void OnMenuOptions(wxCommandEvent &);
    void OnMenuOpenPCAP(wxCommandEvent &);
    void OnMenuCloseAll(wxCommandEvent &);
    void OnMenuFullScreen(wxCommandEvent &);

    MyOptions m_options;

    DECLARE_EVENT_TABLE()
};


class OpalSharkApp : public wxApp, public PProcess
{
    PCLASSINFO(OpalSharkApp, PProcess);

  public:
    OpalSharkApp();

    void Main(); // Dummy function

    // Function from wxWindows
    virtual bool OnInit();
};


#endif


// End of File ///////////////////////////////////////////////////////////////
