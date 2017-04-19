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
 */

#ifndef _OpalShark_MAIN_H
#define _OpalShark_MAIN_H

#if WXVER>29
  #define WXUSINGDLL 1
#endif

#include <ptlib_wx.h>
#include <opal/manager.h>
#include <rtp/pcapfile.h>

#ifndef OPAL_PTLIB_AUDIO
  #error Cannot compile without PTLib sound channel support!
#endif

// Note, include this after everything else so gets all the conversions
#include <ptlib/wxstring.h>


class MyManager;
class wxProgressDialog;
class wxGrid;
class wxGridEvent;
class wxListCtrl;
class wxSpinCtrl;


struct MyOptions
{
  PwxString m_AudioDevice;
  int       m_VideoTiming;
  OpalPCAPFile::PayloadMap m_mappings;

  MyOptions()
    : m_AudioDevice("Default")
    , m_VideoTiming(0)
  { }
};


class OptionsDialog : public wxDialog
{
  public:
    OptionsDialog(MyManager *manager, const MyOptions & options);

    const MyOptions & GetOptions() const { return m_options; }

  private:
    virtual bool TransferDataFromWindow();
    void RefreshMappings();

    MyManager & m_manager;
    MyOptions   m_options;
    PwxString   m_screenAudioDevice;
    wxGrid    * m_mappings;

  wxDECLARE_EVENT_TABLE();
};


class VideoOutputWindow : public wxScrolledWindow
{
  public:
    VideoOutputWindow();
    ~VideoOutputWindow();

    void OutputVideo(const RTP_DataFrame & data);
    void OnVideoUpdate();
    void OnPaint(wxPaintEvent &);

    PColourConverter * m_converter;
    wxBitmap   m_bitmap;
    PMutex     m_mutex;

  wxDECLARE_DYNAMIC_CLASS(VideoOutputWindow);
  wxDECLARE_EVENT_TABLE();
};


class MyPlayer : public wxMDIChildFrame
{
  public:
    MyPlayer(MyManager * manager, const PFilePath & filename);
    ~MyPlayer();

    void OnCloseWindow(wxCloseEvent &);
    void OnClose(wxCommandEvent &);
    void OnListChanged(wxGridEvent &);
    void OnPlay(wxCommandEvent &);
    void OnStop(wxCommandEvent &);
    void OnPause(wxCommandEvent &);
    void OnResume(wxCommandEvent &);
    void OnStep(wxCommandEvent &);
    void OnAnalyse(wxCommandEvent &);
    void OnAnalysisUpdate(wxString, bool);

    void Discover();
    PDECLARE_NOTIFIER2(OpalPCAPFile, MyPlayer, DiscoverProgress, OpalPCAPFile::Progress &);

    enum Controls
    {
      CtlIdle,
      CtlRunning,
      CtlPause,
      CtlStep,
      CtlStop
    };
    void StartPlaying(Controls ctrl);
    void OnPaused();
    void OnPlayEnded();

    void PlayAudio();
    void PlayVideo();

    MyManager        & m_manager;
    OpalPCAPFile       m_pcapFile;

    VideoOutputWindow * m_videoOutput;

    PThread          * m_discoverThread;
    wxProgressDialog * m_discoverProgress;
    OpalPCAPFile::DiscoveredRTP m_discoveredRTP;
    unsigned m_packetCount;

    enum
    {
      ColSrcIP,
      ColSrcPort,
      ColDstIP,
      ColDstPort,
      ColSSRC,
      ColPayloadType,
      ColFormat,
      ColPlay,
      NumCols
    };
    wxGrid * m_rtpList;
    unsigned m_selectedRTP;

    wxListCtrl * m_analysisList;

    Controls  m_playThreadCtrl;
    unsigned  m_pausePacket;
    PThread * m_playThread;

    wxButton   * m_play;
    wxButton   * m_stop;
    wxButton   * m_pause;
    wxButton   * m_resume;
    wxButton   * m_step;
    wxButton   * m_analyse;
    wxSpinCtrl * m_playToPacket;

  wxDECLARE_EVENT_TABLE();
};


class MyManager : public wxMDIParentFrame, public OpalManager
{
  public:
    MyManager();
    ~MyManager();

    bool Initialise(bool startMinimised);
    void Load(const PwxString & fname);

    const MyOptions GetOptions() const { return m_options; }

  private:
    void OnClose(wxCloseEvent &);
    void OnMenuQuit(wxCommandEvent &);
    void OnMenuAbout(wxCommandEvent &);
    void OnMenuOptions(wxCommandEvent &);
    void OnMenuOpenPCAP(wxCommandEvent &);
    void OnMenuCloseAll(wxCommandEvent &);
    void OnMenuFullScreen(wxCommandEvent &);

    MyOptions m_options;

  wxDECLARE_EVENT_TABLE();
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
