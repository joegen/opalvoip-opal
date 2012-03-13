/*
 * mediastrm.h
 *
 * Media Stream classes
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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

#ifndef OPAL_OPAL_MEDIASTRM_H
#define OPAL_OPAL_MEDIASTRM_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <ptclib/delaychan.h>

#include <opal/mediafmt.h>
#include <opal/mediacmd.h>
#include <ptlib/safecoll.h>
#include <ptclib/guid.h>


class OpalRTPSession;
class OpalMediaPatch;
class OpalLine;
class OpalConnection;
class OpalRTPConnection;
class OpalMediaStatistics;

/*!< \page pageOpalMediaStream Media streams in the OPAL library

\section secOverviewM Overview
This page of the documentation is designed to provide an understanding of the
use of the OpalMediaStream class in the OPAL library. 

A voip call that is handled in OPAL consists of two instances of the
OpalConnection class (or derivative) plus instances of the
OpalMediaStream class (or derivatives). Consequently, for handling a
H.323 call which is directed to the PC speaker/microphone, there will
H323Connection and a PCSSConnection. OPAL requires that media can be
transferred between these two clases. To transfer the media, OPAL uses
the OpalMediaStream.

Key to understanding the OpalMediaStream class is that this class is
responsible for transferring media from one instance of an
OpalConnection to another instance of an OpalConnection.

\section secDescriptionM Description

From \ref pageOpalConnections, we see that derivatives of the
OpalConnection class take many different forms (depending on the
protocol). Further, the media from those protocols may well have
different forms. For example, H.323 and PCSS have quite different
format for the media. The class OpalMediaStream (and derivatives) is
responsible for making the format of the media (from each OpalConnection
instance) consistent.

The standard output format of an OpalMediaStream (or derivative) is a
RTP_DataFrame. Consequently, different OpalConnection instances (or
derivatives) can be connected together and exchange media (where the
media is encapsulated inside instances of the RTP_DataFrame class).

When handling an audio only call, each OpalConnection instance (or
derivative) has two attached OpalMediaStream instances (or
derivatives). When the call contains video and audio, each
OpalConnection instance (or derivative) will have four attached
OpalMediaStream instances (or derivatives).

For handling a H.323 call, which is an audio only call, that goes the speaker/microphone of the box, there will be 
\li one instance of a H323Connection
\li one instance of a OpalPCSSConnection
\li two instances of a OpalAudioMediaStream
\li two instances of a OpalRTPMediaStream

Note that an OpalMediaStream instance is designed to take media in one
direction.

*/

/**This class describes a media stream as used in the OPAL system. A media
   stream is the channel through which media data is trasferred between OPAL
   entities. For example, data being sent to an RTP session over a network
   would be through a media stream.
  */
class OpalMediaStream : public PSafeObject
{
    PCLASSINFO(OpalMediaStream, PSafeObject);
  protected:
  /**@name Construction */
  //@{
    /**Construct a new media stream.
      */
    OpalMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );

  public:
    /**Destroy the media stream.
       Make sure the patch, if present, has been stopped and deleted.
      */
    ~OpalMediaStream();
  //@}

  public:
  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Get the currently selected media format.
       The media data format is a string representation of the format being
       transferred by the media channel. It is typically a value as provided
       by the RTP_PayloadType class.

       The default behaviour simply returns the member variable "mediaFormat".
      */
    virtual OpalMediaFormat GetMediaFormat() const;

    /**Set a new media format for the stream.
       Unlike UpdateMediaFormat() this will shut down the patch and attempt to
       create new transcoders to meet the requirement.

       @returns false if the no transcoding can be found.
      */
    virtual bool SetMediaFormat(
      const OpalMediaFormat & mediaFormat   ///<  New media format
    );

    /**Update the media format. This can be used to adjust the
       parameters of a codec at run time. Note you cannot change the basic
       media format, eg change GSM0610 to G.711, only options for that
       format, eg 6k3 mode to 5k3 mode in G.723.1. If the formats are
       different then a OpalMediaFormat::Merge() is performed.

       The default behaviour updates the mediaFormat member variable and
       pases the value on to the OpalMediaPatch.
      */
    bool UpdateMediaFormat(
      const OpalMediaFormat & mediaFormat   ///<  New media format
    );

    /**Execute the command specified to the transcoder. The commands are
       highly context sensitive, for example OpalVideoUpdatePicture would only
       apply to a video transcoder.

       The default behaviour passes the command on to the OpalMediaPatch.

       @returns true if command is handled.
      */
    virtual PBoolean ExecuteCommand(
      const OpalMediaCommand & command    ///<  Command to execute.
    );

    /**Open the media stream using the media format.

       The default behaviour simply sets the isOpen variable to true.
      */
    virtual PBoolean Open();

    /**Start the media stream.

       The default behaviour calls Resume() on the associated OpalMediaPatch
       thread if it was suspended.
      */
    virtual PBoolean Start();

    /**Close the media stream.

       The default marks the stream as closed and calls
       OpalConnection::OnClosedMediaStream().
      */
    virtual PBoolean Close();

    /**Callback that is called on the source stream once the media patch has started.
       The default behaviour calls OpalConnection::OnMediaPatchStart()
      */
    virtual void OnStartMediaPatch();

    /**Callback that is called on the source stream once the media patch has started.
       The default behaviour calls OpalConnection::OnMediaPatchStop()
      */
    virtual void OnStopMediaPatch(
      OpalMediaPatch & patch    ///< Media patch that is stopping
    );

    /**Write a list of RTP frames of data to the sink media stream.
       The default behaviour simply calls WritePacket() on each of the
       elements in the list.
      */
    virtual PBoolean WritePackets(
      RTP_DataFrameList & packets
    );

    /**Read an RTP frame of data from the source media stream.
       The default behaviour simply calls ReadData() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The default behaviour simply calls WriteData() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Read raw media data from the source media stream.
       The default behaviour simply calls ReadPacket() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour calls WritePacket() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Pushes a frame to the patch
      */
    bool PushPacket(
      RTP_DataFrame & packet
    );

    /**Set the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.

       The default behaviour does nothing.
      */
    virtual PBoolean SetDataSize(
      PINDEX dataSize,  ///< New data size (in total)
      PINDEX frameTime  ///< Individual frame time (if applicable)
    );

    /**Get the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.
      */
    PINDEX GetDataSize() const { return defaultDataSize; }

    /**Indicate if the media stream is synchronous.
       If this returns true then the media stream will block of the amount of
       time it takes to annunciate the data. For example if the media stream
       is over a sound card, and 480 bytes of data are to be written it will
       take 30 milliseconds to complete.
      */
    virtual PBoolean IsSynchronous() const = 0;

    /**Indicate if the media stream requires a OpalMediaPatch thread (active patch).
       This is called on the source/sink stream and is passed the sink/source
       stream that the patch will initially be using. The function could
       conditionally require the patch thread to execute a thread reading and
       writing data, or prevent  it from doing so as it can do so in hardware
       in some way, e.g. both streams are on the same OpalLineInterfaceDevice.

       The default behaviour simply returns true.
      */
    virtual PBoolean RequiresPatchThread(
      OpalMediaStream * stream  ///< Other stream in patch
    ) const;
    virtual PBoolean RequiresPatchThread() const; // For backward compatibility

    /**Enable jitter buffer for the media stream.
       Returns true if a jitter buffer is enabled/disabled. Returns false if
       no jitter buffer exists for the media stream.

       The default behaviour does nothing and returns false.
      */
    virtual bool EnableJitterBuffer(bool enab = true) const;
  //@}

  /**@name Member variable access */
  //@{
    /** Get the owner connection.
     */
    OpalConnection & GetConnection() const { return connection; }

    /**Determine of media stream is a source or a sink.
      */
    bool IsSource() const { return isSource; }

    /**Determine of media stream is a source or a sink.
      */
    bool IsSink() const { return !isSource; }

    /**Get the session number of the stream.
     */
    unsigned GetSessionID() const { return sessionID; }

    /**Get the session number of the stream.
     */
    void SetSessionID(unsigned id) { sessionID = id; }

    /**  Get the ID associated with this stream. Used for detecting two 
      *  the streams associated with a bidirectional media channel
      */
    PString GetID() const { return identifier; }

    /**Get the timestamp of last read.
      */
    unsigned GetTimestamp() const { return timestamp; }

    /**Set timestamp for next write.
      */
    void SetTimestamp(unsigned ts) { timestamp = ts; }

    /**Get the marker bit of last read.
      */
    bool GetMarker() const { return marker; }

    /**Set marker bit for next write.
      */
    void SetMarker(bool m) { marker = m; }

    /**Get the paused state for stream.
      */
    bool IsPaused() const { return m_paused; }

    /**Set the paused state for stream.
       This will stop reading/writing data from the stream.
       Returns true if the pause state was changed
      */
    virtual bool SetPaused(
      bool pause,             ///< Indicate that the stream should be paused
      bool fromPatch = false  ///<  Is being called from OpalMediaPatch
    );

    /**Returns true if the media stream is open.
      */
    bool IsOpen() const { return isOpen; }
    
    /**Set the patch thread that is using this stream.
      */
    virtual PBoolean SetPatch(
      OpalMediaPatch * patch  ///<  Media patch thread
    );

    /**Get the patch thread that is using the stream.
      */
    OpalMediaPatch * GetPatch() const { return m_mediaPatch; }

    /**Add a filter to the owning patch safely.
      */
    void AddFilter(
      const PNotifier & filter,   ///< Filter notifier to be called.
      const OpalMediaFormat & stage = OpalMediaFormat() ///< Stage in codec pipeline to call filter
    ) const;

    /**Remove a filter from the owning patch safely.
      */
    bool RemoveFilter(
      const PNotifier & filter,   ///< Filter notifier to be called.
      const OpalMediaFormat & stage = OpalMediaFormat() ///< Stage in codec pipeline to call filter
    ) const;

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool fromPatch = false) const;
#endif
  //@}

  protected:
    void IncrementTimestamp(PINDEX size);
    bool InternalWriteData(const BYTE * data, PINDEX length, PINDEX & written);
    virtual bool InternalUpdateMediaFormat(const OpalMediaFormat & mediaFormat);

    /**Close any internal components of the stream.
       This should be used in preference to overriding the Close() function as
       it is guaranteed to be called exactly once and avoids race conditions in
       the shut down sequence of a media stream.
      */
    virtual void InternalClose() { }

    OpalConnection & connection;
    unsigned         sessionID;
    PString          identifier;
    OpalMediaFormat  mediaFormat;
    bool             m_paused;
    bool             isSource;
    bool             isOpen;
    PINDEX           defaultDataSize;
    unsigned         timestamp;
    bool             marker;

    typedef PSafePtr<OpalMediaPatch, PSafePtrMultiThreaded> PatchPtr;
    PatchPtr m_mediaPatch;

    RTP_DataFrame::PayloadTypes m_payloadType;
    unsigned                    m_frameTime;
    PINDEX                      m_frameSize;

  private:
    P_REMOVE_VIRTUAL_VOID(OnPatchStart());
    P_REMOVE_VIRTUAL_VOID(OnPatchStop());
    P_REMOVE_VIRTUAL_VOID(OnStopMediaPatch());
    P_REMOVE_VIRTUAL_VOID(RemovePatch(OpalMediaPatch *));

  friend class OpalMediaPatch;
};

typedef PSafePtr<OpalMediaStream> OpalMediaStreamPtr;


/**This is a helper class to delay the right time for non I/O bound streams.
  */
class OpalMediaStreamPacing
{
  public:
    OpalMediaStreamPacing(
      const OpalMediaFormat & mediaFormat ///<  Media format for stream
    );

    /// Delay appropriate time for the written bytes
    void Pace(
      bool reading,     ///< Are reading from medium
      PINDEX bytes,     ///< Bytes read/written
      bool & marker     ///< RTP Marker
    );

    bool UpdateMediaFormat(
      const OpalMediaFormat & mediaFormat   ///<  New media format
    );

  protected:
    bool           m_timeOnMarkers;
    unsigned       m_frameTime;
    PINDEX         m_frameSize;
    unsigned       m_timeUnits;
    PAdaptiveDelay m_delay;
};


/**This class describes a media stream that is used for media bypass.
  */
class OpalNullMediaStream : public OpalMediaStream, public OpalMediaStreamPacing
{
    PCLASSINFO(OpalNullMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for RTP sessions.
      */
    OpalNullMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      bool isSynchronous = false           ///<  Can accept data and block accordingly
    );
    OpalNullMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      bool usePacingDelay,                 ///<  Use delay to pace stream I/O
      bool requiresPatchThread             ///<  Requires a patch thread to execute
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Read raw media data from the source media stream.
       The default behaviour does nothing and returns false.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour does nothing and returns false.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );
	
    /**Indicate if the media stream requires a OpalMediaPatch thread (active patch).
       The default behaviour returns the value of m_isSynchronous.
      */
    virtual PBoolean RequiresPatchThread() const;

    /**Indicate if the media stream is synchronous.
       Returns m_isSynchronous.
      */
    virtual PBoolean IsSynchronous() const;
  //@}

  protected:
    virtual void InternalClose() { }
    virtual bool InternalUpdateMediaFormat(const OpalMediaFormat & newMediaFormat);

    bool m_isSynchronous;
    bool m_requiresPatchThread;
};


/**This class describes a media stream that transfers data to/from a RTP
   session.
  */
class OpalRTPMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalRTPMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for RTP sessions.
       This will add a reference to the rtpSession passed in.
      */
    OpalRTPMediaStream(
      OpalRTPConnection & conn,            ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      bool isSource,                       ///<  Is a source stream
      OpalRTPSession & rtpSession,         ///<  RTP session to stream to/from
      unsigned minAudioJitterDelay,        ///<  Minimum jitter buffer size (if applicable)
      unsigned maxAudioJitterDelay         ///<  Maximum jitter buffer size (if applicable)
    );

    /**Destroy the media stream for RTP sessions.
       This will release the reference to the rtpSession passed into the constructor.
      */
    ~OpalRTPMediaStream();
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream using the media format.

       The default behaviour simply sets the isOpen variable to true.
      */
    virtual PBoolean Open();

    /**Set the paused state for stream.
       This will stop reading/writing data from the stream.
      */
    virtual bool SetPaused(
      bool pause,             ///< Indicate that the stream should be paused
      bool fromPatch = false  ///<  Is being called from OpalMediaPatch
    );

    /**Read an RTP frame of data from the source media stream.
       The new behaviour simply calls OpalRTPSession::ReadData().
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The new behaviour simply calls OpalRTPSession::WriteData().
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Set the data size in bytes that is expected to be used.
      */
    virtual PBoolean SetDataSize(
      PINDEX dataSize,  ///< New data size (in total)
      PINDEX frameTime  ///< Individual frame time (if applicable)
    );

    /**Indicate if the media stream is synchronous.
       Returns false for RTP streams.
      */
    virtual PBoolean IsSynchronous() const;

    /**Indicate if the media stream requires a OpalMediaPatch thread (active patch).
       The default behaviour dermines if the media will be flowing between two
       RTP sessions within the same process. If so the
       OpalRTPConnection::OnLocalRTP() is called, and if it returns true
       indicating local handling then this function returns faklse to disable
       the patch thread.
      */
    virtual PBoolean RequiresPatchThread() const;

    /**Enable jitter buffer for the media stream.
       Returns true if a jitter buffer is enabled/disabled. Returns false if
       no jitter buffer exists for the media stream.

       The default behaviour sets the RTP_Session jitter buffer size according
       to the connection parameters, then returns true.
      */
    virtual bool EnableJitterBuffer(bool enab = true) const;

    /**Set the patch thread that is using this stream.
      */
    virtual PBoolean SetPatch(
      OpalMediaPatch * patch  ///<  Media patch thread
    );

    /** Return current RTP session
      */
    virtual OpalRTPSession & GetRtpSession() const
    { return rtpSession; }

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool fromPatch = false) const;
#endif
  //@}

  protected:
    virtual void InternalClose();

    OpalRTPSession & rtpSession;
    unsigned         minAudioJitterDelay;
    unsigned         maxAudioJitterDelay;
};



/**This class describes a media stream that transfers PCM-16 data to/from a PChannel.
  */
class OpalRawMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalRawMediaStream, OpalMediaStream);
  protected:
  /**@name Construction */
  //@{
    /**Construct a new media stream for channel.
      */
    OpalRawMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      PChannel * channel,                  ///<  I/O channel to stream to/from
      bool autoDelete                      ///<  Automatically delete channel
    );

    /**Delete attached channel if autoDelete enabled.
      */
    ~OpalRawMediaStream();
  //@}

  public:
  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Read raw media data from the source media stream.
       The default behaviour reads from the PChannel object.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the PChannel object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Return the associated PChannel 
     */
    PChannel * GetChannel() { return m_channel; }

    /**Set a new channel for raw PCM stream.
      */
    bool SetChannel(
      PChannel * channel,     ///< New channel
      bool autoDelete = true  ///< Auto delete channel on exit or replacement
    );

    /**Get average signal level in last frame.
      */
    virtual unsigned GetAverageSignalLevel();
  //@}

  protected:
    virtual void InternalClose();

    PChannel * m_channel;
    bool       m_autoDelete;
    PMutex     m_channelMutex;

    PBYTEArray m_silence;

    PUInt64    m_averageSignalSum;
    unsigned   m_averageSignalSamples;
    PMutex     m_averagingMutex;

    void CollectAverage(const BYTE * buffer, PINDEX size);
};



/**This class describes a media stream that transfers data to/from a file.
  */
class OpalFileMediaStream : public OpalRawMediaStream, public OpalMediaStreamPacing
{
    PCLASSINFO(OpalFileMediaStream, OpalRawMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for files.
      */
    OpalFileMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      PFile * file,                        ///<  File to stream to/from
      bool autoDelete = true               ///<  Automatically delete file
    );

    /**Construct a new media stream for files.
      */
    OpalFileMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      const PFilePath & path               ///<  File path to stream to/from
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Indicate if the media stream is synchronous.
       Returns true for LID streams.
      */
    virtual PBoolean IsSynchronous() const;

    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the PChannel object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );
  //@}

  protected:
    PFile file;
};


#if OPAL_PTLIB_AUDIO

/**This class describes a media stream that transfers data to/from a audio
   PSoundChannel.
  */
class PSoundChannel;

class OpalAudioMediaStream : public OpalRawMediaStream
{
    PCLASSINFO(OpalAudioMediaStream, OpalRawMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for audio.
      */
    OpalAudioMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      PINDEX buffers,                      ///<  Number of buffers on sound channel
      unsigned bufferTime,                 ///<  Buffering time on sound channel (milliseconds)
      PSoundChannel * channel,             ///<  Audio device to stream to/from
      bool autoDelete = true               ///<  Automatically delete PSoundChannel
    );

    /**Construct a new media stream for audio.
      */
    OpalAudioMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      PINDEX buffers,                      ///<  Number of buffers on sound channel
      unsigned bufferTime,                 ///<  Buffering time on sound channel (milliseconds)
      const PString & deviceName           ///<  Name of audio device to stream to/from
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Set the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.

       The defafault simply sets teh member variable defaultDataSize.
      */
    virtual PBoolean SetDataSize(
      PINDEX dataSize,  ///< New data size (in total)
      PINDEX frameTime  ///< Individual frame time (if applicable)
    );

    /**Indicate if the media stream is synchronous.
       Returns true for LID streams.
      */
    virtual PBoolean IsSynchronous() const;
  //@}

  protected:
    PINDEX   m_soundChannelBuffers;
    unsigned m_soundChannelBufferTime;
};

#endif // OPAL_PTLIB_AUDIO

#if OPAL_VIDEO

/**This class describes a media stream that transfers data to/from a
   PVideoDevice.
  */
class PVideoInputDevice;
class PVideoOutputDevice;

class OpalVideoMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalVideoMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for channel.
      */
    OpalVideoMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PVideoInputDevice * inputDevice,     ///<  Device to use for video grabbing
      PVideoOutputDevice * outputDevice,   ///<  Device to use for video display
      bool autoDeleteInput = true,         ///<  Automatically delete PVideoInputDevice
      bool autoDeleteOutput = true         ///<  Automatically delete PVideoOutputDevice
    );

    /**Delete attached channel if autoDelete enabled.
      */
    ~OpalVideoMediaStream();
  //@}

  /**@name Overrides of PChannel class */
  //@{
    /**Open the media stream.

       The default behaviour sets the OpalLineInterfaceDevice format and
       calls Resume() on the associated OpalMediaPatch thread.
      */
    virtual PBoolean Open();

    /**Execute the command specified to the transcoder. The commands are
       highly context sensitive, for example OpalVideoUpdatePicture would only
       apply to a video transcoder.

       The default behaviour checks for video fast update and then
       passes the command on to the OpalMediaPatch.
      */
    virtual PBoolean ExecuteCommand(
      const OpalMediaCommand & command    ///<  Command to execute.
    );

    /**Read raw media data from the source media stream.
       The default behaviour simply calls ReadPacket() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour calls WritePacket() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Indicate if the media stream is synchronous.
       Returns true for LID streams.
      */
    virtual PBoolean IsSynchronous() const;

    /** Override size of frame header is included
      */
    virtual PBoolean SetDataSize(
      PINDEX dataSize,  ///< New data size (in total)
      PINDEX frameTime  ///< Individual frame time (if applicable)
    );

    /** Get the input device (e.g. for statistics)
      */
    virtual PVideoInputDevice * GetVideoInputDevice() const
    {
      return m_inputDevice;
    }

    /** Get the output device (e.g. for statistics)
      */
    virtual PVideoOutputDevice * GetVideoOutputDevice() const
    {
      return m_outputDevice;
    }

  //@}

  protected:
    virtual void InternalClose();
    virtual bool InternalUpdateMediaFormat(const OpalMediaFormat & newMediaFormat);

    PVideoInputDevice  * m_inputDevice;
    PVideoOutputDevice * m_outputDevice;
    bool                 m_autoDeleteInput;
    bool                 m_autoDeleteOutput;
    PTimeInterval        m_lastGrabTime;
    bool                 m_needKeyFrame;
};

#endif // OPAL_VIDEO

class OpalTransportUDP;

/** Media stream that uses UDP.
 */
class OpalUDPMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalUDPMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for channel.
      */
    OpalUDPMediaStream(
      OpalConnection & conn,               ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      OpalTransportUDP & transport         ///<  UDP transport instance
    );
  //@}

    ~OpalUDPMediaStream();

  /**@name Overrides of OpalMediaStream class */
  //@{

    /**Read an RTP frame of data from the source media stream.
       The new behaviour simply calls OpalTransportUDP::ReadPDU().
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The new behaviour simply calls OpalTransportUDP::Write().
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Indicate if the media stream is synchronous.
       Returns false.
      */
    virtual PBoolean IsSynchronous() const;
  //@}

  private:
    virtual void InternalClose();

    OpalTransportUDP & udpTransport;
};


#endif //OPAL_OPAL_MEDIASTRM_H


// End of File ///////////////////////////////////////////////////////////////
