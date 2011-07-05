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

%ModuleHeaderCode
#include <ptlib.h>
#include <rtp/jitter.h>
%End


///////////////////////////////////////////////////////////////////////////////
/**This is an Abstract jitter buffer, which can be used simply in any
   application. The user is required to use a descendant of this class, and
   provide a "OnReadPacket" method, so that network packets can be placed in
   this class instance */
class OpalJitterBuffer : PSafeObject
{

  public:
  /**@name Construction */
  //@{
    /**Constructor for this jitter buffer. The size of this buffer can be
       altered later with the SetDelay method
      */
    OpalJitterBuffer(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );
    
    /** Destructor, which closes this down and deletes the internal list of frames
      */
    virtual ~OpalJitterBuffer();
  //@}

  /**@name Operations */
  //@{
    /**Set the maximum delay the jitter buffer will operate to.
      */
    void SetDelay(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );

    /**Reset jitter buffer.
      */
    void Reset();

    /**Write data frame from the RTP channel.
      */
    virtual PBoolean WriteData(
      const RTP_DataFrame & frame,   ///< Frame to feed into jitter buffer
      const PTimeInterval & tick = 0 ///< Real time tick for packet arrival
    );

    /**Read a data frame from the jitter buffer.
       This function never blocks. If no data is available, an RTP packet
       with zero payload size is returned.
      */
    virtual PBoolean ReadData(
      RTP_DataFrame & frame,  ///<  Frame to extract from jitter buffer
      const PTimeInterval & tick = 0 ///< Real time tick for packet removal
    );

    /**Get current delay for jitter buffer.
      */
    DWORD GetCurrentJitterDelay() const;
    
    /**Get minimum delay for jitter buffer.
      */
    DWORD GetMinJitterDelay() const;
    
    /**Get maximum delay for jitter buffer.
      */
    DWORD GetMaxJitterDelay() const;

    /**Get time units.
      */
    unsigned GetTimeUnits() const;
    
    /**Get total number received packets too late to go into jitter buffer.
      */
    DWORD GetPacketsTooLate() const;

    /**Get total number received packets that overran the jitter buffer.
      */
    DWORD GetBufferOverruns() const;

    /**Get maximum consecutive marker bits before buffer starts to ignore them.
      */
    DWORD GetMaxConsecutiveMarkerBits() const;

    /**Set maximum consecutive marker bits before buffer starts to ignore them.
      */
    void SetMaxConsecutiveMarkerBits(DWORD max);
  //@}

  protected:
    DWORD CalculateRequiredTimestamp(DWORD playOutTimestamp) const;
};


/**A descendant of the OpalJitterBuffer that starts a thread to read
   from something continuously and feed it into the jitter buffer.
  */
class OpalJitterBufferThread : OpalJitterBuffer /Abstract/
{
 public:
    OpalJitterBufferThread(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );
    ~OpalJitterBufferThread();

    /**Read a data frame from the jitter buffer.
       This function never blocks. If no data is available, an RTP packet
       with zero payload size is returned.

       Override of base class so can terminate caller when shutting down.
      */
    virtual PBoolean ReadData(
      RTP_DataFrame & frame   ///<  Frame to extract from jitter buffer
    );

    /**This class instance collects data from the outside world in this
       method.

       @return true on successful read, false on faulty read. */
    virtual PBoolean OnReadPacket(
      RTP_DataFrame & frame  ///<  Frame read from the RTP session
    ) = 0;
	
 protected:
    //PDECLARE_NOTIFIER(PThread, OpalJitterBufferThread, JitterThreadMain);

    /// Internal function to be called from derived class constructor
    void StartThread() /ReleaseGIL/;

    /// Internal function to be called from derived class destructor
    void WaitForThreadTermination() /ReleaseGIL/;
	
	/// SIP Function to access m_running method.
    /*bool running();
%MethodCode
	return m_running;
%End*/
};


/////////////////////////////////////////////////////////////////////////////
/**A descendant of the OpalJitterBuffer that reads RTP_DataFrame instances
   from the RTP_Sessions
  */
class RTP_JitterBuffer : OpalJitterBufferThread
{
 public:
    RTP_JitterBuffer(
      RTP_Session & session,   ///<  Associated RTP session tor ead data from
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );
    ~RTP_JitterBuffer();

    /**This class instance collects data from the outside world in this
       method.

       @return true on successful read, false on faulty read. */
    virtual PBoolean OnReadPacket(
      RTP_DataFrame & frame  ///<  Frame read from the RTP session
    );

};



/////////////////////////////////////////////////////////////////////////////
