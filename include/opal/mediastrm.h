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
 * $Log: mediastrm.h,v $
 * Revision 1.2010  2002/02/11 07:39:15  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.8  2002/01/22 05:10:58  robertj
 * Removed redundant code
 *
 * Revision 2.7  2002/01/22 05:09:00  robertj
 * Removed payload mismatch detection from RTP media stream.
 * Added function to get media patch from media stream.
 *
 * Revision 2.6  2002/01/14 02:27:32  robertj
 * Added ability to turn jitter buffer off in media stream to allow for patches
 *   that do not require it.
 *
 * Revision 2.5  2001/10/15 04:28:35  robertj
 * Added delayed start of media patch threads.
 *
 * Revision 2.4  2001/10/04 05:44:00  craigs
 * Changed to start media patch threads in Paused state
 *
 * Revision 2.3  2001/10/04 00:41:20  robertj
 * Removed GetMediaFormats() function as is not useful.
 *
 * Revision 2.2  2001/08/21 01:10:35  robertj
 * Fixed propagation of sound channel buffers through media stream.
 *
 * Revision 2.1  2001/08/01 05:51:47  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __OPAL_MEDIASTRM_H
#define __OPAL_MEDIASTRM_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/mediafmt.h>


class RTP_Session;
class OpalMediaPatch;
class OpalLine;


/**This class describes a media stream as used in the OPAL system. A media
   stream is the channel through which media data is trasferred between OPAL
   entities. For example, data being sent to an RTP session over a network
   would be through a media stream.
  */
class OpalMediaStream : public PChannel
{
    PCLASSINFO(OpalMediaStream, PChannel);
  protected:
  /**@name Construction */
  //@{
    /**Construct a new media stream.
      */
    OpalMediaStream(
      BOOL isSourceStream,  /// Direction of I/O for stream
      unsigned sessionID    /// Session ID for media stream
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
      ostream & strm    /// Stream to output text representation
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

    /**Open the media stream using the media format.

       The default behaviour simply sets the member variable "mediaFormat"
       and "defaultDataSize".
      */
    virtual BOOL Open(
      const OpalMediaFormat & format /// Media format to select
    );

    /**Start the media stream.

       The default behaviour calls Resume() on the associated OpalMediaPatch
       thread if it was suspended.
      */
    virtual BOOL Start();

    /**Close the media stream.

       The default does nothing.
      */
    virtual BOOL Close();

    /**Read an RTP frame of data from the source media stream.
       The default behaviour simply calls Read() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual BOOL ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The default behaviour simply calls Write() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual BOOL WritePacket(
      RTP_DataFrame & packet
    );

    /**Set the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.

       The defafault behaviour does nothing.
      */
    virtual BOOL SetDataSize(
      PINDEX dataSize  /// New data size
    );

    /**Indicate if the media stream is synchronous.
       If this returns TRUE then the media stream will block of the amount of
       time it takes to annunciate the data. For example if the media stream
       is over a sound card, and 480 bytes of data are to be written it will
       take 30 milliseconds to complete.
      */
    virtual BOOL IsSynchronous() const = 0;

    /**Indicate if the media stream requires a OpalMediaPatch thread.
       The default behaviour returns TRUE.
      */
    virtual BOOL RequiresPatchThread() const;

    /**Enable jitter buffer for the media stream.

       The default behaviour does nothing.
      */
    virtual void EnableJitterBuffer() const;
  //@}

  /**@name Member variable access */
  //@{
    /**Determine of media stream is a source or a sink.
      */
    BOOL IsSource() const { return isSource; }

    /**Determine of media stream is a source or a sink.
      */
    BOOL IsSink() const { return !isSource; }

    /**Get the session number of the stream.
     */
    unsigned GetSessionID() const { return sessionID; }

    /**Get the timestamp of last read.
      */
    unsigned GetTimestamp() const { return timestamp; }

    /**Set timestamp for next write.
      */
    void SetTimestamp(unsigned ts) { timestamp = ts; }

    /**Get the marker bit of last read.
      */
    BOOL GetMarker() const { return marker; }

    /**Set marker bit for next write.
      */
    void SetMarker(BOOL m) { marker = m; }

    /**Set the patch thread that is using this stream.
      */
    void SetPatch(
      OpalMediaPatch * patch  /// Media patch thread
    );

    /**Get the patch thread that is using the stream.
      */
    OpalMediaPatch * GetPatch() const { return patchThread; }
  //@}

  protected:
    BOOL            isSource;
    unsigned        sessionID;
    OpalMediaFormat mediaFormat;
    unsigned        defaultDataSize;
    unsigned        timestamp;
    BOOL            marker;

    OpalMediaPatch * patchThread;
};

PLIST(OpalMediaStreamList, OpalMediaStream);


/**This class describes a media stream that is used for media bypass.
  */
class OpalNullMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalNullMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for RTP sessions.
      */
    OpalNullMediaStream(
      BOOL isSourceStream,  /// Direction of I/O for stream
      unsigned sessionID    /// Session ID for media stream
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Indicate if the media stream requires a OpalMediaPatch thread.
       The default behaviour returns FALSE.
      */
    virtual BOOL RequiresPatchThread() const;

    /**Indicate if the media stream is synchronous.
       Returns FALSE.
      */
    virtual BOOL IsSynchronous() const;
  //@}
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
      */
    OpalRTPMediaStream(
      BOOL isSourceStream,         /// Direction of I/O for stream
      RTP_Session & rtpSession     /// RTP session to stream to/from
    );
  //@}

  /**@name Overrides of PChannel class */
  //@{
    /**Close the media stream.

       The default does nothing.
      */
    virtual BOOL Close();

    /** Low level read from the media stream. This overrides the PChannel
        class behaviour.

        The behaviour here is to create a RTP_DataFrame and read that from the
        RTP session. The internal variables (eg timestamp) are set from that
        RTP_DataFrame and the payload copied to the "buf" pointer.
     */
    virtual BOOL Read(
      void * buf,   /// Pointer to a block of memory to receive the read bytes.
      PINDEX len    /// Maximum number of bytes to read into the buffer.
    );

    /** Low level write to the media stream. This function overrides the
        PChannel class behaviour.

       The behaviour here is to create a single RTP_DataFrame, copy the
       internal variables (eg timestamp) to it and copy the "buf" variable to
       its payload. The 
     */
    virtual BOOL Write(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Read an RTP frame of data from the source media stream.
       The new behaviour simply calls RTP_Session::ReadData().
      */
    virtual BOOL ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The new behaviour simply calls RTP_Session::WriteData().
      */
    virtual BOOL WritePacket(
      RTP_DataFrame & packet
    );

    /**Indicate if the media stream is synchronous.
       Returns FALSE for RTP streams.
      */
    virtual BOOL IsSynchronous() const;

    /**Enable jitter buffer for the media stream.

       The default behaviour does nothing.
      */
    virtual void EnableJitterBuffer() const;
  //@}

  protected:
    RTP_Session & rtpSession;
};



/**This class describes a media stream that transfers data to/from a Line
   Interface Device.
  */
class OpalLineMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalLineMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for Line Interface Devices.
      */
    OpalLineMediaStream(
      BOOL isSourceStream,  /// Direction of I/O for stream
      unsigned sessionID,   /// Session ID for media stream
      OpalLine & line       /// LID line to stream to/from
    );
  //@}

  /**@name Overrides of PChannel class */
  //@{
    /** Low level read from the media stream. This overrides the PChannel
        class behaviour.

       The new behaviour simply calls OpalLineInterfaceDevice::ReadBlock().
     */
    virtual BOOL Read(
      void * buf,   /// Pointer to a block of memory to receive the read bytes.
      PINDEX len    /// Maximum number of bytes to read into the buffer.
    );

    /** Low level write to the media stream. This function overrides the
        PChannel class behaviour.

       The new behaviour simply calls OpalLineInterfaceDevice::WriteBlock().
     */
    virtual BOOL Write(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Select the data format this channel is to operate.

       The default behaviour simply sets the member variable "mediaFormat".
      */
    virtual BOOL Open(
      const OpalMediaFormat & format /// Media format to select
    );

    /**Close the media stream.

       The default does nothing.
      */
    virtual BOOL Close();

    /**Indicate if the media stream is synchronous.
       Returns TRUE for LID streams.
      */
    virtual BOOL IsSynchronous() const;
  //@}

  protected:
    OpalLine & line;
    BOOL       useDeblocking;
    unsigned   missedCount;
    BYTE       lastSID[4];
    BOOL       lastFrameWasSignal;
};



/**This class describes a media stream that transfers data to/from a PChannel.
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
      BOOL isSourceStream, /// Direction of I/O for stream
      unsigned sessionID,  /// Session ID for media stream
      PChannel * channel,  /// I/O channel to stream to/from
      BOOL autoDelete      /// Automatically delete channel
    );

    ~OpalRawMediaStream();
  //@}

  public:
  /**@name Overrides of PChannel class */
  //@{
    /** Low level read from the media stream. This overrides the PChannel
        class behaviour.

       The new behaviour simply calls PChannel::ReadBlock().
     */
    virtual BOOL Read(
      void * buf,   /// Pointer to a block of memory to receive the read bytes.
      PINDEX len    /// Maximum number of bytes to read into the buffer.
    );

    /** Low level write to the media stream. This function overrides the
        PChannel class behaviour.

       The new behaviour simply calls PChannel::WriteBlock().
     */
    virtual BOOL Write(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
    );

    /**Close the media stream.

       The default does nothing.
      */
    virtual BOOL Close();
  //@}

  protected:
    PChannel * channel;
    BOOL       autoDelete;
};



/**This class describes a media stream that transfers data to/from a file.
  */
class OpalFileMediaStream : public OpalRawMediaStream
{
    PCLASSINFO(OpalFileMediaStream, OpalRawMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for files.
      */
    OpalFileMediaStream(
      BOOL isSourceStream,    /// Direction of I/O for stream
      unsigned sessionID,     /// Session ID for media stream
      PFile * file,           /// File to stream to/from
      BOOL autoDelete = TRUE  /// Automatically delete file
    );

    /**Construct a new media stream for files.
      */
    OpalFileMediaStream(
      BOOL isSourceStream,    /// Direction of I/O for stream
      unsigned sessionID,     /// Session ID for media stream
      const PFilePath & path  /// File path to stream to/from
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Indicate if the media stream is synchronous.
       Returns TRUE for LID streams.
      */
    virtual BOOL IsSynchronous() const;
  //@}

  protected:
    PFile file;
};


/**This class describes a media stream that transfers data to/from a audio
   PSoundChannel.
  */
class OpalAudioMediaStream : public OpalRawMediaStream
{
    PCLASSINFO(OpalAudioMediaStream, OpalRawMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for audio.
      */
    OpalAudioMediaStream(
      BOOL isSourceStream,      /// Direction of I/O for stream
      unsigned sessionID,       /// Session ID for media stream
      PINDEX buffers,           /// Number of buffers on sound channel
      PSoundChannel * channel,  /// Audio device to stream to/from
      BOOL autoDelete = TRUE    /// Automatically delete PSoundChannel
    );

    /**Construct a new media stream for audio.
      */
    OpalAudioMediaStream(
      BOOL isSourceStream,        /// Direction of I/O for stream
      unsigned sessionID,         /// Session ID for media stream
      PINDEX buffers,             /// Number of buffers on sound channel
      const PString & deviceName  /// Name of audio device to stream to/from
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Set the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.

       The defafault behaviour does nothing.
      */
    virtual BOOL SetDataSize(
      PINDEX dataSize  /// New data size
    );

    /**Indicate if the media stream is synchronous.
       Returns TRUE for LID streams.
      */
    virtual BOOL IsSynchronous() const;
  //@}

  protected:
    PINDEX soundChannelBuffers;
};


/**This class describes a media stream that transfers data to/from a
   PVideoDevice.
  */
class OpalVideoMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalVideoMediaStream, OpalMediaStream);
  protected:
  /**@name Construction */
  //@{
    /**Construct a new media stream for channel.
      */
    OpalVideoMediaStream(
      BOOL isSourceStream,    /// Direction of I/O for stream
      unsigned sessionID,     /// Session ID for media stream
      PVideoDevice & device   /// Device to use for video
    );
  //@}

  public:
  /**@name Overrides of PChannel class */
  //@{
    /** Low level read from the media stream. This overrides the PChannel
        class behaviour.

       The new behaviour simply calls PChannel::ReadBlock().
     */
    virtual BOOL Read(
      void * buf,   /// Pointer to a block of memory to receive the read bytes.
      PINDEX len    /// Maximum number of bytes to read into the buffer.
    );

    /** Low level write to the media stream. This function overrides the
        PChannel class behaviour.

       The new behaviour simply calls PChannel::WriteBlock().
     */
    virtual BOOL Write(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
    );
  //@}

  protected:
    PVideoDevice & device;
};



#endif //__OPAL_MEDIASTRM_H


// End of File ///////////////////////////////////////////////////////////////
