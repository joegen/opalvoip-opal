/*
 * opalmixer.h
 *
 * OPAL media mixers
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2007 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): Craig Southeren (craigs@postincrement.com)
 *                 Robert Jongbloed (robertj@voxlucida.com.au)
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#ifndef OPAL_OPAL_OPALMIXER_H
#define OPAL_OPAL_OPALMIXER_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#include <queue>

#include <ptlib/psync.h>
#include <ptclib/delaychan.h>

#include <rtp/rtp.h>
#include <codec/opalwavfile.h>
#include <codec/vidcodec.h>


///////////////////////////////////////////////////////////////////////////////

/** Class base for a media mixer.

    The mixer operates by re-buffering the input media into chunks each with an
    associated timestamp. A main mixer thread then reads from each  stream at
    regular intervals, mixes the media and creates the output buffer.
 
    Note the timestamps of the input media are extremely important as they are
    used so that breaks or too fast data in the input media is dealt with correctly.
  */
class OpalBaseMixer
{
  public:
    OpalBaseMixer(
      bool pushThread,    ///< Indicate if the push thread should be started
      unsigned periodMS,  ///< The output buffer time in milliseconds
      unsigned periodTS   ///< The output buffer time in RTP timestamp units
    );

    virtual ~OpalBaseMixer() { }

    typedef std::string Key_T;

    /**Add a stream to mixer using the specified key.
      */
    virtual bool AddStream(
      const Key_T & key   ///< key for mixer stream
    );

    /** Remove an input stream from mixer.
      */
    virtual void RemoveStream(
      const Key_T & key   ///< key for mixer stream
    );

    /** Remove all input streams from mixer.
      */
    virtual void RemoveAllStreams();

    /**Write an RTP data frame to mixer.
       A copy of the RTP data frame is created. This function is generally
       quite fast as the actual mixing is done in a different thread so
       minimal interference with the normal media stream processing occurs.
      */
    virtual bool WriteStream(
      const Key_T & key,          ///< key for mixer stream
      const RTP_DataFrame & input ///< Input RTP data for media
    );

    /**Read media from mixer.
       A pull model system would call this function to get the mixed media
       from the mixer. Note the stream indicated by the streamToIgnore key is
       not included in the mixing operation, allowing for example, the member
       of a conference to not hear themselves.

       Note this function is the one that does all the "heavy lifting" for the
       mixer.
      */
    virtual RTP_DataFrame * ReadMixed(
      const Key_T & streamToIgnore = Key_T()  ///< Stream to not include in mixing
    );

    /**Mixed data is now available.
       For a push model system, this is called with mixed data as returned by
       ReadMixed().

       The "mixed" parameter is a reference to a pointer, so if the consumer
       wishes to take responsibility for deleting the pointer to an RTP data
       frame, then they can set it to NULL.

       If false is returned then the push thread is exited.
      */
    virtual bool OnMixed(
      RTP_DataFrame * & mixed   ///, Poitner to mixed media.
    );

    /**Start the push thread.
       Normally called internally.
      */
    void StartPushThread();

    /**Stop the push thread.
       This will wait for th epush thread to terminate, so care must be taken
       to avoid deadlocks when calling.
      */
    void StopPushThread();

    /**Get the period for mxiing in RTP timestamp units.
      */
    unsigned GetPeriodTS() const { return m_periodTS; }

  protected:
    struct Stream {
      Stream(OpalBaseMixer & mixer);

      OpalBaseMixer      & m_mixer;
      queue<RTP_DataFrame> m_queue;
      unsigned             m_nextTimeStamp;
    };
    typedef std::map<Key_T, Stream *> StreamMap_T;

    virtual Stream * CreateStream() = 0;
    virtual bool MixStreams(const Key_T & streamToIgnore, RTP_DataFrame & frame) = 0;
    virtual size_t GetOutputSize() const = 0;
    void PushThreadMain();

    bool      m_pushThread;      ///< true if to use a thread to push data out
    unsigned  m_periodMS;        ///< Mixing interval in milliseconds
    unsigned  m_periodTS;        ///< Mixing interval in timestamp units

    PThread * m_workerThread;    ///< reader thread handle
    bool      m_threadRunning;   ///< used to stop reader thread
    unsigned  m_outputTimestamp; ///< RTP timestamp for output data

    StreamMap_T m_streams;
    PMutex      m_mutex;          ///< mutex for list of streams and thread handle
};

///////////////////////////////////////////////////////////////////////////////

/** Class for an audio mixer.
    This takes raw PCM-16 data and sums all the input data streams to produce
    a single PCM-16 sample value.

    For 2 or less channels, they may be mixed as stereo where 16 bit PCM
    samples are placed in adjacent pairs in the output, rather than summing
    them.
  */
class OpalAudioMixer : public OpalBaseMixer
{
  public:
    OpalAudioMixer(
      bool stereo = false,
      unsigned sampleRate = 8000,
      bool pushThread = true,
      unsigned period = 10
    );

    ~OpalAudioMixer() { StopPushThread(); }

    /** Remove an input stream from mixer.
      */
    virtual void RemoveStream(
      const Key_T & key   ///< key for mixer stream
    );

    /** Remove all input streams from mixer.
      */
    virtual void RemoveAllStreams();

    /**Return flag for mixing stereo audio data.
      */
    bool IsStereo() const { return m_stereo; }

    /**Get sample rate for audio.
      */
    unsigned GetSampleRate() const { return m_sampleRate; }

    /**Set sample rate for audio data.
       Note that all streams must have the same sample rate.

       Returns false if attempts to set sample rate to something different to
       existing streams.
      */
    bool SetSampleRate(
      unsigned rate   ///< New rate
    );

  protected:
    struct AudioStream : public Stream
    {
      AudioStream(OpalAudioMixer & mixer);
      const short * GetAudioDataPtr();

      PShortArray m_cacheSamples;
      size_t      m_samplesUsed;
    };

    virtual Stream * CreateStream();
    virtual bool MixStreams(const Key_T & streamToIgnore, RTP_DataFrame & frame);
    virtual size_t GetOutputSize() const;

    bool MixStereo(RTP_DataFrame & frame);
    bool MixAdditive(const Key_T & streamToIgnore, RTP_DataFrame & frame);

  protected:
    bool     m_stereo;
    unsigned m_sampleRate;

    AudioStream * m_left;
    AudioStream * m_right;
};


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

/**Video mixer.
   This takes raw YUV420P frames with a PluginCodec_Video_FrameHeader in the
   RTP data frames, scales them and places them in particular positions of the
   output data frame. A number of different patterns for positioning the sub
   images are available in the Styles enum.
  */
class OpalVideoMixer : public OpalBaseMixer
{
  public:
    enum Styles {
      eSideBySideLetterbox, /**< Two images side by side with black bars top and bottom.
                                 It is expected that the input frames and output are all
                                 the same aspect ratio, e.g. 4:3. Works well if inputs
                                 are QCIF and output is CIF for example. */
      eSideBySideScaled,    /**< Two images side by side, scaled to fit halves of output
                                 frame. It is expected that the output frame be double
                                 the width of the input data to maintain aspect ratio.
                                 e.g. for CIF inputs, output would be 704x288. */
      eStackedPillarbox,    /**< Two images, one on top of the other with black bars down
                                 the sides. It is expected that the input frames and output
                                 are all the same aspect ratio, e.g. 4:3. Works well if
                                 inputs are QCIF and output is CIF for example. */
      eStackedScaled,       /**< Two images, one on top of the other, scaled to fit halves
                                 of output frame. It is expected that the output frame be
                                 double the height of the input data to maintain aspect
                                 ratio. e.g. for CIF inputs, output would be 352x576. */
      eGrid2x2,             /**< Up to four images scaled into quarters of the output frame. */
      eGrid3x3,             /**< Up to nine images scaled into a 3x3 grid. */
      eGrid4x4              /**< Up to sixteen images scaled into 4x4 grid. */
    };

    OpalVideoMixer(
      Styles style,           ///< Style for mixing video
      unsigned width,         ///< Width of output frame
      unsigned height,        ///< Height of output frame
      unsigned rate = 15,     ///< Frames per second for output
      bool pushThread = true  ///< A push thread is to be created
    );

    ~OpalVideoMixer() { StopPushThread(); }

    /**Get output video frame width.
      */
    unsigned GetFrameWidth() const { return m_width; }

    /**Get output video frame height.
      */
    unsigned GetFrameHeight() const { return m_height; }

    /**Get output video frame rate (frames per second)
      */
    unsigned GetFrameRate() const { return 1000/m_periodMS; }

    /**Set output video frame rate.
       May be dynamically changed at any time.
      */
    bool SetFrameRate(
      unsigned rate   // New frames per second.
    );

    /**Set the output video frame width and height.
       May be dynamically changed at any time.
      */
    bool SetFrameSize(
      unsigned width,   ///< New width
      unsigned height   ///< new height
    );

  protected:
    struct VideoStream : public Stream
    {
      VideoStream(OpalVideoMixer & mixer);
      void InsertVideoFrame(unsigned x, unsigned y, unsigned w, unsigned h);
      void IncrementTimestamp();

      OpalVideoMixer & m_mixer;
    };

    friend struct VideoStream;

    virtual Stream * CreateStream();
    virtual bool MixStreams(const Key_T & streamToIgnore, RTP_DataFrame & frame);
    virtual size_t GetOutputSize() const;

  protected:
    Styles     m_style;
    unsigned   m_width, m_height;
    PBYTEArray m_frameStore;
};

#endif // OPAL_VIDEO

#endif // OPAL_OPAL_OPAL_MIXER


///////////////////////////////////////////////////////////////////////////////
