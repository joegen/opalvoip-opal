/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

/** A class representing a sound. A sound is a highly platform dependent
   entity that is abstracted for use here. Very little manipulation of the
   sounds are possible.

   The most common sound to use is the static function #Beep()# which
   emits the system standard "warning" or "attention" sound.
 */
class PSound
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\sound.h>
%End
  public:
  /**@name Construction */
  //@{
    /**Create a new sound, using the parameters provided.
       It is expected that the "lowest common denominator" encoding, linear PCM,
       is used.

       All other values for the encoding are platform dependent.
     */
    PSound(
      unsigned numChannels = 1,    ///< Number of channels eg mono/stereo
      unsigned sampleRate = 8000,  ///< Samples per second
      unsigned bitsPerSample = 16, ///< Number of bits per sample
      PINDEX   bufferSize = 0,     ///< Size of data
      const BYTE * data = NULL     ///< Pointer to initial data
    );

    /**Create a new sound, reading from a platform dependent file.
     */
    PSound(
      const PFilePath & filename   ///< Sound file to load.
    );

    /**Set new data bytes for the sound.
     */
    // PSound & operator=(
      // const PBYTEArray & data  ///< New data for sound
    // );
  //@}

  /**@name File functions */
  //@{
    /**Load a platform dependent sound file (eg .WAV file for Win32) into the
       object. Note the whole file must able to be loaded into memory.

       Also note that not all possible files are playable by this library. No
       format conversions between file and driver are performed.

       @return
       PTrue if the sound is loaded successfully.
     */
    PBoolean Load(
      const PFilePath & filename   ///< Sound file to load.
    );

    /**Save a platform dependent sound file (eg .WAV file for Win32) from the
       object.

       @return
       PTrue if the sound is saved successfully.
     */
    PBoolean Save(
      const PFilePath & filename   ///< Sound file to load.
    );
  //@}

  /**@name Access functions */
  //@{
    /// Play the sound on the default sound device.
    PBoolean Play();

    /// Play the sound to the specified sound device.
    PBoolean Play(const PString & device);

    /**Set the internal sound format to linear PCM at the specification in
       the parameters.
     */
    void SetFormat(
      unsigned numChannels,   ///< Number of channels eg mono/stereo
      unsigned sampleRate,    ///< Samples per second
      unsigned bitsPerSample  ///< Number of bits per sample
    );

    /**Get the current encoding. A value of 0 indicates linear PCM, any other
       value is platform dependent.
     */
    unsigned GetEncoding()   const;

    /// Get  the number of channels (mono/stereo) in the sound.
    unsigned GetChannels()   const;

    /// Get the sample rate in samples per second.
    unsigned GetSampleRate() const;

    /// Get the sample size in bits per sample.
    unsigned GetSampleSize() const;

    /// Get the platform dependent error code from the last file load.
    unsigned long GetErrorCode()  const;

    /// Get the size of the platform dependent format info.
    PINDEX   GetFormatInfoSize()  const;

    /// Get pointer to the platform dependent format info.
    const void * GetFormatInfoData() const;  //@}

  /**@name Miscellaneous functions */
  //@{
    /**Play a sound file to the default device. If the #wait#
       parameter is PTrue then the function does not return until the file has
       been played. If PFalse then the sound play is begun asynchronously and
       the function returns immediately.

       @return
       PTrue if the sound is playing or has played.
     */
    static PBoolean PlayFile(
      const PFilePath & file, ///< Sound file to play.
      PBoolean wait = PTrue        ///< Flag to play sound synchronously.
    );

     // Play the "standard" warning beep for the platform.
     //static void Beep();
  //@}   
};


/**
   Abstract class for a generalised sound channel, 
   and an implementation of PSoundChannel for old code that is not plugin-aware.
   When instantiated, it selects the first plugin of the base class 
   "PSoundChannel"

   As an abstract class, this represents a sound schannel. Drivers for real, 
   platform dependent sound hardware will be ancestors of this class and 
   can be found in the plugins section of PTLib.

   A sound channel is either playing or recording. If simultaneous
   playing and recording is desired, two instances of PSoundChannel
   must be created. It is an error for the same thread to attempt to
   both read and write audio data to once instance of a PSoundChannel
   class.

   PSoundChanel instances are designed to be reentrant. The actual
   usage model employed is left to the developer. One model could be
   where one thread is responsible for 
   construction, setup, opening and read/write operations. After creating and
   eventually opening the channel this thread is responsible for
   handling read/writes fast enough to avoid gaps in the generated  audio
   stream.

   Remaining operations may beinvoked from other threads.
   This includes Close() and actually gathering the necessary data to
   be sent to the device.

   Besides the basic I/O task, the Read()/Write(() functions have well
   defined timing characteristics. When a PSoundChannel instance is
   used from Opal, the read/write operations are designed to also act
   as timers so as to nicely space the generated network packets of
   audio/ sound packets to the speaker.


   Read and Writes of audio data to a PSoundChannel are blocking. The
   length of time required to read/write a block of audio from/to a
   PSoundChannel instance is equal to the time required for that block
   of audio to record/play. So for a sound rate of 8khz, 240 samples,
   it is going to take 30ms to do a read/write.

   Since the Read()/Write(() functions have well defined
   timing characteristics; they are designed to also act as timers in a loop
   involving data transfers to/from the codecs.

   The sound is buffered and the size and number of buffers should be set
   before playing/recording. Each call to Write() will use one buffer, so care
   needs to be taken not to use a large number of small writes but tailor the
   buffers to the size of each write you make.

   Similarly for reading, an entire buffer must be read before any of it is
   available to a Read() call. Note that once a buffer is filled you can read
   it a byte at a time if desired, but as soon as all the data in the buffer
   is used returned, the next read will wait until the entire next buffer is
   read from the hardware. So again, tailor the number and size of buffers to
   the application. To avoid being blocked until the buffer fills, you can use
   the StartRecording() function to initiate the buffer filling, and the
   IsRecordingBufferFull() function to determine when the Read() function will
   no longer block.

   Note that this sound channel is implicitly a linear PCM channel. No data
   conversion is performed on data to/from the channel.

 */
class PSoundChannel : PChannel
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\sound.h>
%End
  public:
  /**@name Construction */
  //@{
    enum Directions {
      Recorder,
      Player
    };

    /// Create a sound channel.
    PSoundChannel();

    /** Create a sound channel.
        Create a reference to the sound drivers for the platform.
      */
    PSoundChannel(
      const PString & device,       ///< Name of sound driver/device
      Directions dir,               ///< Sound I/O direction
      unsigned numChannels = 1,     ///< Number of channels eg mono/stereo
      unsigned sampleRate = 8000,   ///< Samples per second
      unsigned bitsPerSample = 16   ///< Number of bits per sample
    );
    // 

    virtual ~PSoundChannel();
    // Destroy and close the sound driver
  //@}

  /**@name Open functions */
  //@{
    /**Get the list of available sound drivers (plug-ins)
     */
    static PStringArray GetDriverNames(
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /**Get sound devices that correspond to the specified driver name.
       If driverName is an empty string or the value "*" then GetAllDeviceNames()
       is used.
     */
    static PStringArray GetDriversDeviceNames(
      const PString & driverName,         ///< Name of driver
      Directions direction,               ///< Direction for device (record or play)
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

	
    /**Create the sound channel that corresponds to the specified driver name.
     */
    static PSoundChannel * CreateChannel (
      const PString & driverName,         ///< Name of driver
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /* Create the matching sound channel that corresponds to the device name.
       So, for "fake" return a device that will generate fake video.
       For "Phillips 680 webcam" (eg) will return appropriate grabber.
       Note that Phillips will return the appropriate grabber also.

       This is typically used with the return values from GetDeviceNames().
     */
    static PSoundChannel * CreateChannelByName(
      const PString & deviceName,         ///< Name of device
      Directions direction,               ///< Direction for device (record or play)
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /**Create an opened sound channel that corresponds to the specified names.
       If the driverName parameter is an empty string or "*" then CreateChannelByName
       is used with the deviceName parameter which is assumed to be a value returned
       from GetAllDeviceNames().
     */
    static PSoundChannel * CreateOpenedChannel(
      const PString & driverName,         ///< Name of driver
      const PString & deviceName,         ///< Name of device
      Directions direction,               ///< Direction for device (record or play)
      unsigned numChannels = 1,           ///< Number of channels 1=mon, 2=stereo
      unsigned sampleRate = 8000,         ///< Sample rate
      unsigned bitsPerSample = 16,        ///< Bits per sample
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /**Get the name for the default sound devices/driver that is on this
       platform. Note that a named device may not necessarily do both
       playing and recording so the arrays returned with the #dir#
       parameter in each value is not necessarily the same.

       This will return a list of uniqie device names across all of the available
       drivers. If two drivers have identical names for devices, then the string
       returned will be of the form driver+'\\t'+device.

       @return
       A platform dependent string for the sound player/recorder.
     */
    static PString GetDefaultDevice(
      Directions dir    // Sound I/O direction
    );

    /**Get the list of all devices name for the default sound devices/driver that is on this
       platform. Note that a named device may not necessarily do both
       playing and recording so the arrays returned with the #dir#
       parameter in each value is not necessarily the same.

       @return
       Platform dependent strings for the sound player/recorder.
     */
    static PStringArray GetDeviceNames(
      Directions direction,               ///< Direction for device (record or play)
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /**Open the specified device for playing or recording. The device name is
       platform specific and is as returned in the GetDevices() function.

       @return
       PTrue if the sound device is valid for playing/recording.
     */
    virtual PBoolean Open(
      const PString & device,       ///< Name of sound driver/device
      Directions dir,               ///< Sound I/O direction
      unsigned numChannels = 1,     ///< Number of channels eg mono/stereo
      unsigned sampleRate = 8000,   ///< Samples per second
      unsigned bitsPerSample = 16   ///< Number of bits per sample
    );

    /**Test if this instance of PSoundChannel is open.

       @return
       PTrue if this instance is open.
     */
    virtual PBoolean IsOpen() const;

    /** Close the channel, shutting down the link to the data source. 

       @return PTrue if the channel successfully closed.
     */
    virtual PBoolean Close();

    /**Get the OS specific handle for the PSoundChannel.

       @return
       integer value of the handle.
     */
    virtual int GetHandle() const;

    /// Get the name of the open channel
    virtual PString GetName() const;

    /** Abort the background playing/recording of the sound channel.
	There will be a logic assertion if you attempt to Abort a
	sound channel operation, when the device is currently closed.

       @return
       PTrue if the sound has successfully been aborted.
     */
    virtual PBoolean Abort();
  //@}

  /**@name Channel set up functions */
  //@{
    /**Set the format for play/record. Note that linear PCM data is the only
       one supported at this time.

       Note that if the PlayFile() function is used, this may be overridden
       by information in the file being played.

       @return
       PTrue if the format is valid.
     */
    virtual PBoolean SetFormat(
      unsigned numChannels = 1,     ///< Number of channels eg mono/stereo
      unsigned sampleRate = 8000,   ///< Samples per second
      unsigned bitsPerSample = 16   ///< Number of bits per sample
    );

    /// Get  the number of channels (mono/stereo) in the sound.
    virtual unsigned GetChannels() const;

    /// Get the sample rate in samples per second.
    virtual unsigned GetSampleRate() const;

    /// Get the sample size in bits per sample.
    virtual unsigned GetSampleSize() const;

    /**Set the internal buffers for the sound channel I/O. 

       Note that with Linux OSS, the size is always rounded up to the nearest
       power of two, so 20000 => 32768. 

       @return
       PTrue if the sound device is valid for playing/recording.
     */
    virtual PBoolean SetBuffers(
      PINDEX size,      ///< Size of each buffer
      PINDEX count = 2  ///< Number of buffers
    );

    /**Get the internal buffers for the sound channel I/O. 

       @return
       PTrue if the buffer size were obtained.
     */
    virtual PBoolean GetBuffers(
      PINDEX & size,    // Size of each buffer
      PINDEX & count    // Number of buffers
    );

    enum {
      MaxVolume = 100
    };

    /**Set the volume of the play/read process.
       The volume range is 0 == quiet, 100 == LOUDEST. The volume is a
       logarithmic scale mapped from the lowest gain possible on the device to
       the highest gain
        
       @return
       PTrue if there were no errors.
    */
    virtual PBoolean SetVolume(
      unsigned volume   ///< New volume level
    );

    /**Get the volume of the play/read process.
       The volume range is 0 == quiet, 100 == LOUDEST. The volume is a
       logarithmic scale mapped from the lowest gain possible on the device to
       the highest gain.

       @return
       PTrue if there were no errors.
    */
    virtual PBoolean GetVolume(
      unsigned & volume   ///< Variable to receive volume level.
    );
  //@}

  /**@name Play functions */
  //@{

    /** Low level write (or play) to the channel. 

	It will generate a logical assertion if you attempt write to a
	channel set up for recording.

       @param buf is a pointer to the data to be written to the
       channel.  It is an error for this pointer to be NULL. A logical
       assert will be generated when buf is NULL.

       @param len Nr of bytes to send. If len equals the buffer size
        set by SetBuffers() it will block for
        (1000*len)/(samplesize*samplerate) ms. Typically, the sample
        size is 2 bytes.  If len == 0, this will return immediately,
        where the return value is equal to the value of IsOpen().
 
       @return PTrue if len bytes were written to the channel,
         otherwise PFalse. The GetErrorCode() function should be 
        consulted after Write() returns PFalse to determine what 
        caused the failure.

     */
    // virtual PBoolean Write(const void * buf, PINDEX len);

    /** Get number of bytes written in last Write() operation. */
    virtual PINDEX GetLastWriteCount() const;

    /**Play a sound to the open device. If the #wait# parameter is
       PTrue then the function does not return until the file has been played.
       If PFalse then the sound play is begun asynchronously and the function
       returns immediately.

       Note:  if the driver is closed while playing the sound, the play 
       operation stops immediately.

       Also note that not all possible sounds and sound files are playable by
       this library. No format conversions between sound object and driver are
       performed.

       @return
       PTrue if the sound is playing or has played.
     */

    virtual PBoolean PlaySound(
      const PSound & sound,   ///< Sound to play.
      PBoolean wait = PTrue        ///< Flag to play sound synchronously.
    );

    /**Play a sound file to the open device. If the #wait#
       parameter is PTrue then the function does not return until the file has
       been played. If PFalse then the sound play is begun asynchronously and
       the function returns immediately.

       Note if the driver is closed of the object destroyed then the sound
       play is aborted.

       Also note that not all possible sounds and sound files are playable by
       this library. No format conversions between sound object and driver are
       performed.

       @return
       PTrue if the sound is playing or has played.
     */
    virtual PBoolean PlayFile(
      const PFilePath & file, ///< Sound file to play.
      PBoolean wait = PTrue        ///< Flag to play sound synchronously.
    );

    /**Indicate if the sound play begun with PlayBuffer() or PlayFile() has
       completed.

       @return
       PTrue if the sound has completed playing.
     */
    virtual PBoolean HasPlayCompleted();

    /**Block calling thread until the sound play begun with PlaySound() or
       PlayFile() has completed. 

       @return
       PTrue if the sound has successfully completed playing.
     */
    virtual PBoolean WaitForPlayCompletion();

  //@}

  /**@name Record functions */
  //@{
    /** Low level read from the channel. This function may block until the
       requested number of characters were read or the read timeout was
       reached. The GetLastReadCount() function returns the actual number
       of bytes read.

	It will generate a logical assertion if you attempt to read
	from a PSoundChannel that is setup for playing.

       The GetErrorCode() function should be consulted after Read() returns
       PFalse to determine what caused the failure.

        @param len Nr of bytes to endeaveour to read from the sound
        device. If len equals the buffer size set by SetBuffers() it
        will block for (1000*len)/(samplesize*samplerate)
        ms. Typically, the sample size is 2 bytes.  If len == 0, this
        will return immediately, where the return value is equal to
        the value of IsOpen().

       @param buf is a pointer to the empty data area, which will
       contain the data collected from the sound device.  It is an
       error for this pointer to be NULL. A logical assert will be
       generated when buf is NULL.

       @return PTrue indicates that at least one character was read
       from the channel.  PFalse means no bytes were read due to some
       I/O error, (which includes timeout or some other thread closed
       the device).
     */
    // virtual PBoolean Read(
      // void * buf /Out/,   ///< Pointer to a block of memory to receive the read bytes.
      // PINDEX len    ///< Maximum number of bytes to read into the buffer.
    // );

    /** Return number of bytes read in last Read() call. */
    PINDEX GetLastReadCount() const;

    /**Record into the sound object all of the buffer's of sound data. Use the
       SetBuffers() function to determine how long the recording will be made.

       For the Win32 platform, the most efficient way to record a PSound is to
       use the SetBuffers() function to set a single buffer of the desired
       size and then do the recording. For Linux OSS this can cause problems
       as the buffers are rounded up to a power of two, so to gain more
       accuracy you need a number of smaller buffers.

       Note that this function will block until all of the data is buffered.
       If you wish to do this asynchronously, use StartRecording() and
       AreAllrecordBuffersFull() to determine when you can call RecordSound()
       without blocking.

       @return
       PTrue if the sound has been recorded.
     */
    virtual PBoolean RecordSound(
      PSound & sound ///< Sound recorded
    );

    /**Record into the platform dependent sound file all of the buffer's of
       sound data. Use the SetBuffers() function to determine how long the
       recording will be made.

       Note that this function will block until all of the data is buffered.
       If you wish to do this asynchronously, use StartRecording() and
       AreAllrecordBuffersFull() to determine when you can call RecordSound()
       without blocking.

       @return
       PTrue if the sound has been recorded.
     */
    virtual PBoolean RecordFile(
      const PFilePath & file ///< Sound file recorded
    );

    /**Start filling record buffers. The first call to Read() will also
       initiate the recording.

       @return
       PTrue if the sound driver has successfully started recording.
     */
    virtual PBoolean StartRecording();

    /**Determine if a record buffer has been filled, so that the next Read()
       call will not block. Provided that the amount of data read is less than
       the buffer size.

       @return
       PTrue if the sound driver has filled a buffer.
     */
    virtual PBoolean IsRecordBufferFull();

    /**Determine if all of the record buffer allocated has been filled. There
       is an implicit Abort() of the recording if this occurs and recording is
       stopped. The channel may need to be closed and opened again to start
       a new recording.

       @return
       PTrue if the sound driver has filled a buffer.
     */
    virtual PBoolean AreAllRecordBuffersFull();

    /**Block the thread until a record buffer has been filled, so that the
       next Read() call will not block. Provided that the amount of data read
       is less than the buffer size.

       @return
       PTrue if the sound driver has filled a buffer.
     */
    virtual PBoolean WaitForRecordBufferFull();

    /**Block the thread until all of the record buffer allocated has been
       filled. There is an implicit Abort() of the recording if this occurs
       and recording is stopped. The channel may need to be closed and opened
       again to start a new recording.

       @return
       PTrue if the sound driver has filled a buffer.
     */
    virtual PBoolean WaitForAllRecordBuffersFull();
  //@}
  
  private:
	PSoundChannel(const PSoundChannel &);
};