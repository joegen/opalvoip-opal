/*
 * main.h
 *
 * Jester - a tester of the jitter buffer
 *
 * Copyright (c) 2006 Derek J Smithies
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
 * The Original Code is Jester
 *
 * The Initial Developer of the Original Code is Derek J Smithies
 *
 * Contributor(s): Robert Jongbloed.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _Jester_MAIN_H
#define _Jester_MAIN_H


#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptclib/pwavfile.h>
#include <ptlib/sound.h>

#include <rtp/jitter.h>
#include <rtp/rtp.h>
#include <rtp/pcapfile.h>

#include <ptclib/delaychan.h>
#include <ptclib/random.h>

#if !OPAL_IAX2
#error Cannot compile without IAX2
#endif


typedef map<DWORD, DWORD> JitterProfileMap;

/////////////////////////////////////////////////////////////////////////////
/**we use this class primarily to access variables in the OpalJitterBuffer*/
class JesterJitterBuffer : public OpalJitterBuffer
{
    PCLASSINFO(JesterJitterBuffer, OpalJitterBuffer);
 public:
    JesterJitterBuffer();

    PINDEX GetCurrentDepth() const { return m_frames.size(); }
    DWORD GetAverageFrameTime() const { return m_averageFrameTime; }
};

/////////////////////////////////////////////////////////////////////////////

/** The main class that is instantiated to do things */
class JesterProcess : public PProcess
{
  PCLASSINFO(JesterProcess, PProcess)

  public:
    JesterProcess();

    void Main();

  protected:

#ifdef DOC_PLUS_PLUS
    /**Generate the Udp packets that we could have read from the internet. In
       other words, place packets in the jitter buffer. */
    virtual void GeneratePackets(PThread &, INT);
#else
    PDECLARE_NOTIFIER(PThread, JesterProcess, GeneratePackets);
#endif

#ifdef DOC_PLUS_PLUS
    /**Read in the Udp packets (from the output of the jitter buffer), that we
       could have read from the internet. In other words, extract packets from
       the jitter buffer. */
    virtual void ConsumePackets(PThread &, INT);
#else
    PDECLARE_NOTIFIER(PThread, JesterProcess, ConsumePackets);
#endif

    void Report();
    bool GenerateFrame(RTP_DataFrame & frame, PTimeInterval & delay);

    /**Handle user input, which is keys to describe the status of the program,
       while the different loops run. The program will not finish until this
       method is completed.*/
    void ManageUserInput();

    /**The number of bytes of data in each simulated RTP_DataFrame */
    PINDEX m_bytesPerBlock;

    /**Flag to indicate if we do, or do not, simulate silence suppression. If
       true, we do silence suppresion and send packets in bursts of onnnn,
       nothing, onnn, nothing..*/
    bool m_silenceSuppression;

    /**Flag to indicate if we do, or do not, simulate dropped packets.*/
    bool m_dropPackets;

    /**Flag to indicate if we do, or do not fiddle with the operaiton of
       silence suppression function. When doing silence suppression, the start
       of each talk burst has the marker bit turned on. If this flag is set
       true, then some of those marker bits (half of them) are
       suppressed. This flag therefore tests the operation of the jitter
       buffer, to see if it copes with the dropping of the first packet in
       each voice stream */
    bool m_markerSuppression;

    /**Difference in milliseconds between generation and playback.
      */
    int m_startTimeDelta;

    /**A descendant of the OpalJitterBuffer, which means we have the minimum
       of code to write to test OpalJitterBuffer. Further, we can now access
       variables in the OpalJitterBuffer */
    JesterJitterBuffer m_jitterBuffer;

    /**The sound channel that we will write audio to*/
    PSoundChannel m_player;

    /**Name of the wavfile containing the audio we will read in */
    PWAVFile m_wavFile;

    /**Maximum generated jitter */
    JitterProfileMap m_generateJitter;

    /**the timestamp, as used by the generate thread */
    DWORD m_generateTimestamp;
    DWORD m_initialTimestamp;

    /**the sequence number, as used by the generate thread */
    WORD m_generateSequenceNumber;
    WORD m_maxSequenceNumber;

    /**The timestamp the jitter buffer consumer is expecting */
    DWORD m_playbackTimestamp;

    /** Flag for running the test */
    bool m_keepRunning;

    bool  m_lastFrameWasSilence;
    DWORD m_talkBurstTimestamp;
    DWORD m_lastSilentTimestamp;
    DWORD m_lastGeneratedJitter;
    PTimeInterval m_initialTick;

    OpalPCAPFile m_pcap;
    PTime        m_lastFrameTime;
};


#endif  // _Jester_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
