/*
 * main.h
 *
 * Recording Calls demonstration for OPAL
 *
 * Copyright (c) 2014 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#ifndef _RecodingCalls_MAIN_H
#define _RecodingCalls_MAIN_H

#define EXTERNAL_SCHEME "record"

class MyLocalEndPoint;
class PMediaFile;


class MyLocalEndPoint : public OpalLocalEndPoint
{
    PCLASSINFO(MyLocalEndPoint, OpalLocalEndPoint)
  public:
    MyLocalEndPoint(OpalManagerConsole & manager);

    // Overrides from OpalLocalEndPoint
    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour returns the most basic media formats, PCM audio
       and YUV420P video.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Call back to indicate that there is an incoming call.
       Note this function should not block or it will impede the operation of
       the stack.

       The default implementation returns true;

       @return false is returned the call is aborted with status of EndedByLocalBusy.
      */
    virtual bool OnIncomingCall(
      OpalLocalConnection & connection ///<  Connection having event
    );

    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual bool OnOpenMediaStream(
      OpalConnection & connection,  ///<  Connection that owns the media stream
      OpalMediaStream & stream      ///<  New media stream being opened
    );

    /**Call back to handle received media data.
       If false is returned then OnWriteMediaData() is called.

       Care with the handling of real time is required, see GetSynchronicity
       for more details.

       Note it is the responsibility of this function to update the \p frame
       timestamp for correct operation of the jitter buffer.

       The default implementation returns false.
      */
    virtual bool OnWriteMediaFrame(
      const OpalLocalConnection & connection, ///<  Connection for media
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      RTP_DataFrame & frame                   ///<  RTP frame for data
    );

    // New functions
    bool Initialise(PArgList & args);

  protected:
    OpalManagerConsole & m_manager;

    struct Mixer : public OpalAudioMixer
    {
      ~Mixer() { StopPushThread(); }

      virtual bool OnMixed(RTP_DataFrame * & output);

      PSmartPtr<PMediaFile> m_mediaFile;
    } m_mixer;
};


class MyManager : public OpalManagerConsole
{
    PCLASSINFO(MyManager, OpalManagerConsole)

  public:
    virtual PString GetArgumentSpec() const;
    virtual void Usage(ostream & strm, const PArgList & args);
    virtual bool Initialise(
      PArgList & args,
      bool verbose,
      const PString & defaultRoute = EXTERNAL_SCHEME":"
    );

    /**A call back function whenever a call is completed.
       In telephony terminology a completed call is one where there is an
       established link between two parties.

       This called from the OpalCall::OnEstablished() function.

       The default behaviour does nothing.
      */
    virtual void OnEstablishedCall(
      OpalCall & call   ///<  Call that was completed
    );

  protected:
    PDirectory m_outputDir;
    PString    m_fileTemplate;
    PString    m_fileType;
    OpalRecordManager::Options m_options;
};


#endif  // _RecodingCalls_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
