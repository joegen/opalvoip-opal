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
 * Revision 1.1  2003/03/07 06:33:28  craigs
 * More implementation
 *
 */

#ifndef _OpenPhone_MAIN_H
#define _OpenPhone_MAIN_H

#include <ptlib.h>

#include <opal/manager.h>
#include <h323/h323ep.h>
#include <sip/sipep.h>
#include <opal/pcss.h>

#include "version.h"
#include "mainwindow.h"
#include "trace.h"

class MyOptions : public PObject
{
  PCLASSINFO(MyOptions, PObject);
  public:
    MyOptions();
    BOOL Save();
    BOOL Load();

    // general
    BOOL autoAnswer;
    BOOL enableH323;
    BOOL enableSIP;

    // audio device
    enum AudioDevice {
      SoundCard,
      Quicknet,
      VOIPBlaster
    } audioDevice;
    PString soundcardRecordDevice;
    PString soundcardPlaybackDevice;

    // audio codec
    unsigned minJitter;
    unsigned maxJitter;
    BOOL silenceDetection;

    // trace
    BOOL traceEnabled;
    unsigned traceLevel;
    PFilePath traceFilename;
    unsigned traceOptions;
};

class MyManager;

class MyPCSSEndPoint : public OpalPCSSEndPoint
{
  PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);

  public:
    MyPCSSEndPoint(MyManager & manager);

    virtual PString OnGetDestination(const OpalPCSSConnection & connection);
    virtual void OnShowIncoming(const OpalPCSSConnection & connection);
    virtual BOOL OnShowOutgoing(const OpalPCSSConnection & connection);

    BOOL SetSoundDevice(const PString & name, PSoundChannel::Directions dir);

    PString destinationAddress;
    PString incomingConnectionToken;
};

class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager);

  public:
    MyManager();
    ~MyManager();

    BOOL Initialise(const MyOptions & options);

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

    BOOL MakeCall(const PString & dest, PString & token);

  protected:
    //OpalPOTSEndPoint * potsEP;
    MyPCSSEndPoint   * pcssEP;
    H323EndPoint     * h323EP;
    SIPEndPoint      * sipEP;
};

class MainWindow : public MainWindowBase
{
  public:
    MainWindow();
    ~MainWindow();

    void OnMakeCall();
    void OnViewOptions();
    void UpdateCallStatus();

  protected:
    MyOptions options;
    MyManager * opal;
    PString currentCallToken;

#if PTRACING
    BOOL OpenTraceFile(const MyOptions & config);
    PTextFile * myTraceFile;
#endif
};

class OpenPhoneApp : public PProcess
{
  PCLASSINFO(OpenPhoneApp, PProcess);
  OpenPhoneApp()
    : PProcess("Equivalence", "OpenPhone",
               MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
    { }

  void Main()
    { } // Dummy function
};

#endif