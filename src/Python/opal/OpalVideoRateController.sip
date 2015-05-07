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

/**  This class is used to calculate the instantaneous bit rate of a data stream
  *  using a one second sliding window 
 */

class OpalBitRateCalculator
{
%TypeHeaderCode
#include <ptlib.h>
#include <codec/ratectl.h>
%End   
  public:
    /**  Create the calculator
    */
    OpalBitRateCalculator();

    /** Reset the statistics
    */
    void Reset();

    /** Set the quanta (usually the frame time)
    */
    void SetQuanta(
      unsigned quanta_
    );

    /** Get the quanta (usually the frame time)
    */
    unsigned GetQuanta() const;

    /** Add a new packet to the bit rate calculations
    */
    void AddPacket(PINDEX size, bool marker);

    /** Get the instantaneous bit rate
    */
    unsigned GetBitRate();

    /** Get the average bit rate since SetQuanta was called
    */
    unsigned GetAverageBitRate();

    /** return the bit rate if the specific data size was transmitted
    */
    unsigned GetTrialBitRate(PINDEX size);

    /** Return total bytes sent since SetQuanta was called
    */
    PInt64 GetTotalSize() const;

    /** Return total miliseconds since SetQuanta was called
    */
    PInt64 GetTotalTime() const;

    /** Return number of frames in history
    */
    size_t GetHistoryCount() const;

    /** Return number of bytes in history
    */
    unsigned GetHistorySize() const;

    /** Return earlist timestamp in history
    */
    PInt64 GetEarliestHistoryTime() const;

    /** Return number of marker bits in history
    */
    unsigned GetHistoryFrames() const;

    // flush old data from history
    void Flush();

    // used to get "now"
    static PInt64 GetNow();
};

//
//  Declare a generic video rate controller class.
//  A rate controller seeks to maintain a constant bit rate by manipulating
//  the parameters of the video stream
//
//  Before encoding a potential output frame, use the SkipFrame function to determine if the 
//  frame should be skipped. 
//
//  If the frame is not skipped, encode the frame and call PushFrame to add the frame to the rate controller queue
//  PopFrame can then be called to retreive frames to transmit
//
//  PushFrame must always be called with packets from a single video frame, but PopFrame may return packets 
//  from multiple video frames
//



class OpalVideoRateController /Abstract/
{
%TypeHeaderCode
#include <ptlib.h>
#include <codec/ratectl.h>
%End  
  public:
    OpalVideoRateController();

    virtual ~OpalVideoRateController();

    /** Open the rate controller with the specific parameters
      */
    virtual void Open(
      const OpalMediaFormat & mediaFormat
    );

    /** Determine if the next frame should be skipped.
      * The rate controller can also indicate whether the next frame should
      * be encoded as an I-frame, which is useful if many frames have been skipped
      */
    virtual bool SkipFrame(
      bool & forceIFrame
    ) = 0;

    /** push encoded frames into the rate controller queue
      */
    /*virtual void Push(
      RTP_DataFrameList & inputFrames, 
      bool iFrame
    );*/

    /** retreive encoded frames from the rate controller queue
      */
    /*virtual bool Pop(
      RTP_DataFrameList & outputPackets, 
      bool & iFrame, 
      bool force
    );*/

    /** Bit rate calculator used by rate controller
      */
    OpalBitRateCalculator m_bitRateCalc;

};