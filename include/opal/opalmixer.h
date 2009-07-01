/*
 * opalmixer.h
 *
 * OPAL audio mixer
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
 * Contributor(s): ______________________________________.
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

template <typename Locker_T = PSyncNULL>
class PMemBuffer
{
  public:
    struct Common {
      Common(size_t size)
        : base(size)
      { 
        refCount = 1; 
      }

      Common(BYTE * ptr, size_t size)
        : base(ptr, size)
      { 
        refCount = 1; 
      }

      mutable int refCount;
      mutable Locker_T mutex;
      mutable PBYTEArray base;
    };

    Common * common;

  protected:
    BYTE * data;
    PINDEX dataLen;

  public:
    PMemBuffer()
    { 
      common  = NULL;
      data    = NULL;
      dataLen = 0;
    }

    PMemBuffer(PINDEX size)
    { 
      common = new Common(size);
      data    = common->base.GetPointer();
      dataLen = size;
    }

    PMemBuffer(BYTE * ptr, size_t size)
    { 
      common = new Common(ptr, size);
      data    = common->base.GetPointer();
      dataLen = size;
    }

    PMemBuffer(const PBYTEArray & obj)
    { 
      common = new Common(obj.GetPointer(), obj.GetSize());
      data    = common->base.GetPointer();
      dataLen = obj.GetSize();
    }

    PMemBuffer(const PMemBuffer & obj)
    { 
      PWaitAndSignal m(obj.common->mutex);
      common = obj.common;
      ++common->refCount;
      data    = obj.data;
      dataLen = obj.dataLen;
    }

    ~PMemBuffer()
    {
      if (common != NULL) {
        common->mutex.Wait();
        PBoolean last = common->refCount == 1;
        if (last) {
          common->mutex.Signal();
          delete common;
        } 
        else {
          --common->refCount;
          common->mutex.Signal();
        }
        common = NULL;
        data    = NULL;
        dataLen = 0;
      }
    }

    PMemBuffer & operator = (const PMemBuffer & obj)
    {
      if (&obj == this)
        return *this;

      if (common != NULL) {
        common->mutex.Wait();
        PBoolean last = common->refCount == 1;
        if (last) {
          common->mutex.Signal();
          delete common;
        }
        else
        {
          --common->refCount;
          common->mutex.Signal();
        }
        common = NULL;
        data    = NULL;
        dataLen = 0;
      }
      {
        PWaitAndSignal m(obj.common->mutex);
        common = obj.common;
        ++common->refCount;
        data    = obj.data;
        dataLen = obj.dataLen;
      }

      return *this;
    }

    void MakeUnique()
    {
      PWaitAndSignal m(common->mutex);
      if (common->refCount == 1) 
        return;

      Common * newCommon = new Common(common->base.GetPointer(), common->base.GetSize());
      data = newCommon->base.GetPointer() + (data - common->base.GetPointer());
      --common->refCount;
      common = newCommon;
    }

    // set absolute base of data
    // length is unchanged
    void SetBase(PINDEX offs)
    { 
      PWaitAndSignal m(common->mutex);
      data = common->base.GetPointer() + offs;
      if (offs + dataLen > common->base.GetSize())
        dataLen = common->base.GetSize() - offs;
    }

    // adjust base of data relative to current base
    // length is unchanged
    void Rebase(PINDEX offs)
    { 
      PWaitAndSignal m(common->mutex);
      SetBase(offs + data - common->base.GetPointer());
    }

    // set the sbsolute length of the data
    void SetSize(PINDEX size)
    { 
      if (common == NULL) {
        common = new Common(size);
        data    = common->base.GetPointer();
        dataLen = size;
      }
      else {
        PWaitAndSignal m(common->mutex);
        if (size < dataLen)
          dataLen = size;
        else {
          PINDEX offs = data - common->base.GetPointer();
          if (offs + size < common->base.GetSize())
            dataLen = size;
          else
            dataLen = common->base.GetSize() - offs;
        }
      }
    }

    BYTE * GetPointerAndLock()
    { 
      PAssert(common != NULL, "NULL pointer");
      common->mutex.Wait();
      return data; 
    }

    inline const BYTE * GetPointerAndLock() const
    { 
      PAssert(common != NULL, "NULL pointer");
      common->mutex.Wait();
      return data; 
    }

    inline PINDEX GetSize() const
    { return dataLen; }

    inline void Lock() const
    {
      common->mutex.Wait();
    }

    inline void Unlock() const
    {
      common->mutex.Signal();
    }

    inline PSync & GetMutex()
    {
      return common->mutex;
    }
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//  the mixer operates by re-buffering the input streams into 10ms chunks
//  each with an associated timestamp. A main mixer thread then reads from each 
//  stream at regular intervals, mixes the audio and creates the output
//
//  There are several complications:
//
//    1) the timestamps must be used so that breaks in the input audio are 
//       dealt with correctly
//
//    2) Using a single worker thread to read all of the streams doesn't work because
//       it tends to get starved of CPU time and the output either gets behind or has
//       breaks in it. To avoid this, the creation of the output data is triggered 
//       by whatever thread (write or read) occurs after each 10ms interval
//

/////////////////////////////////////////////////////////////////////////////
//
//  define a class that encapsulates an audio stream for the purposes of the mixer
//

class OpalAudioMixerStream {
  public:
    class StreamFrame : public PMemBuffer<PMutex> {
      public:
        DWORD timestamp;
        unsigned channelNumber;

        StreamFrame()
          : timestamp(0)
          , channelNumber(0)
        { }

        StreamFrame(const RTP_DataFrame & rtp);
    };
    typedef std::queue<StreamFrame> StreamFrameQueue_T;

    PMutex mutex;
    StreamFrameQueue_T frameQueue;
    StreamFrame frameCache;
    DWORD cacheTimeStamp;
    DWORD writtenTimeStamp;

    PBoolean active;
    PBoolean first;
    unsigned channelNumber;

    OpalAudioMixerStream();
    void WriteFrame(const StreamFrame & frame);
    void FillSilence(StreamFrame & retFrame, PINDEX ms);
    void PopFrame(StreamFrame & retFrame, PINDEX ms);
    PBoolean ReadFrame(StreamFrame & retFrame, PINDEX ms);
};

/////////////////////////////////////////////////////////////////////////////
//
//  Define the audio mixer. This class extracts audio from a list of 
//  OpalAudioMixerStream instances
//

class OpalAudioMixer
{
  public:
    typedef std::string Key_T;
    typedef std::map<Key_T, OpalAudioMixerStream *> StreamInfoMap_T;
    typedef std::map<Key_T, OpalAudioMixerStream::StreamFrame> MixerPCMMap_T;

    class MixerFrame
    {
      protected:
        MixerPCMMap_T channelData;

        PINDEX frameLengthSamples;
        mutable PIntArray mixedData;
        mutable PMutex mutex;

      public:
		MixerFrame(PINDEX _frameLength);
        void CreateMixedData() const;
        PBoolean GetMixedFrame(OpalAudioMixerStream::StreamFrame & frame) const;
        PBoolean GetStereoFrame(OpalAudioMixerStream::StreamFrame & frame) const;
        PBoolean GetChannelFrame(Key_T key, OpalAudioMixerStream::StreamFrame & frame) const;
        void InsertFrame(Key_T key, OpalAudioMixerStream::StreamFrame & frame);
	};

  protected:
    PINDEX frameLengthMs;                  ///< size of each audio chunk in milliseconds

    PMutex mutex;                          ///< mutex for list of streams and thread handle
    StreamInfoMap_T streamInfoMap;         ///< list of streams
    unsigned channelNumber;                ///< counter for channels

    PBoolean realTime;                         ///< PTrue if realtime mixing
    PBoolean pushThread;                       ///< PTrue if to use a thread to push data out
    PThread * mixerWorkerThread;                      ///< reader thread handle
    PBoolean threadRunning;                    ///< used to stop reader thread

    PBoolean audioStarted;                     ///< PTrue if output audio is running
    PBoolean firstRead;                        ///< PTrue if first use of CheckForRead

    PTime timeOfNextRead;                  ///< absolute timestamp for next scheduled read
    DWORD outputTimestamp;                 ///< RTP timestamp for output data

  public:
    OpalAudioMixer(PBoolean realTime = PTrue, PBoolean _pushThread = PTrue);
    virtual ~OpalAudioMixer() { }
    virtual PBoolean OnWriteAudio(const MixerFrame &);
    void AddStream(const Key_T & key, OpalAudioMixerStream * stream);
    void RemoveStream(const Key_T & key);
    void RemoveAllStreams();
    void StartThread();
    void ThreadMain();
    void ReadRoutine();
    void WriteMixedFrame();
    PBoolean Write(const Key_T & key, const RTP_DataFrame & rtp);
};

#endif // OPAL_OPAL_OPAL_MIXER

